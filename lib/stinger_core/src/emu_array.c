#include "emu_array.h"
#include "emu_xmalloc.h"

#if defined(__le64__)
#include <memoryweb.h>
#else
#define NODELETS() 1
#endif

#include <assert.h>


struct emu_striped_array *
emu_striped_array_new(size_t num_elements, size_t element_size)
{
    struct emu_striped_array * rtn = xcalloc(sizeof(struct emu_striped_array), 1);
    emu_striped_array_init(rtn, num_elements, element_size);
    return rtn;
};

void
emu_striped_array_init(struct emu_striped_array * self, size_t num_elements, size_t element_size)
{
    assert(!self->data);
    self->num_elements = num_elements;
    self->element_size = element_size;
    self->data = xmw_calloc2d(num_elements, element_size);
    assert(self->data);
}

void
emu_striped_array_deinit(struct emu_striped_array * self)
{
    assert(self->data);
    mw_free(self->data);
    self->data = 0;
    self->num_elements = 0;
    self->element_size = 0;
}

void
emu_striped_array_free(struct emu_striped_array * self)
{
    emu_striped_array_deinit(self);
    free(self);
}

void *
emu_striped_array_index(struct emu_striped_array * self, size_t i)
{
    assert(self->data);
    assert(i < self->num_elements);
    return mw_arrayindex(self->data, i, self->num_elements, self->element_size);
}

size_t
emu_striped_array_size(struct emu_striped_array * self)
{
    assert(self->data);
    return self->num_elements;
}


// Blocked array type

struct emu_blocked_array *
emu_blocked_array_new(size_t num_elements, size_t element_size)
{
    struct emu_blocked_array * rtn = xcalloc(sizeof(struct emu_blocked_array), 1);
    emu_blocked_array_init(rtn, num_elements, element_size);
    return rtn;
}


static size_t
div_round_up(size_t num, size_t den)
{
    assert(den != 0);
    return (num + den - 1) / den;
}

static size_t
log2_round_up(size_t x)
{
    assert(x != 0);
    if (x == 1) { return 0; }
    else { return PRIORITY(x - 1) + 1; }
}

void
emu_blocked_array_init(struct emu_blocked_array * self, size_t num_elements, size_t element_size)
{
    assert(!self->data);
    assert(num_elements > 0);

    self->element_size = element_size;

    // We will allocate one block on each nodelet
    self->num_blocks = NODELETS();

    // How many elements in each block?
    size_t elements_per_block = div_round_up(num_elements, self->num_blocks);
    // Round up to nearest power of two
    self->log2_elements_per_block = log2_round_up(elements_per_block);
    elements_per_block = 1 << self->log2_elements_per_block;

    // Allocate a block on each nodelet
    self->data = xmw_calloc2d(self->num_blocks, elements_per_block * element_size);
    assert(self->data);

    // Initialize array of next indices to allocate from each block
    // Each element should end up on the nodelet it controls
    self->block_tail = xmw_malloc1d(self->num_blocks);
    for (size_t i = 0; i < self->num_blocks; ++i)
    {
        self->block_tail[i] = i * elements_per_block;
    }

}

void
emu_blocked_array_deinit(struct emu_blocked_array * self)
{
    assert(self->data);
    mw_free(self->data);
    self->data = 0;
    self->num_blocks = 0;
    self->element_size = 0;
    self->log2_elements_per_block = 0;
}

void
emu_blocked_array_free(struct emu_blocked_array * self)
{
    emu_blocked_array_deinit(self);
    free(self);
}


void *
emu_blocked_array_index(struct emu_blocked_array * self, size_t i)
{
    assert(self->data);

    // Array bounds check
    size_t elements_per_block = 1 << self->log2_elements_per_block;
    assert(i < self->num_blocks * elements_per_block);

    // First we determine which block holds element i
    // i / N
    size_t block = i >> self->log2_elements_per_block;

    // Then calculate the position of element i within the block
    // i % N
    size_t mask = elements_per_block - 1;
    size_t offset = i & mask;

    // return data[i / N][i % N]
    void * base = mw_arrayindex(self->data, block, self->num_blocks, elements_per_block * self->element_size);
    void * ptr = base + (offset * self->element_size);
    return ptr;
}


size_t
emu_blocked_array_size(struct emu_blocked_array * self)
{
    assert(self->data);
    return (1 << self->log2_elements_per_block) * self->num_blocks;
}


/*
Reserves <k> contiguous elements from the blocked array <self>.
Returns the index of the first element.
Tries to get elements from the same block as the index <local_hint>.
*/
size_t
emu_blocked_array_allocate_local(struct emu_blocked_array * self, size_t k, size_t local_hint)
{
    assert(self->data);
    // Calculate local block
    size_t local_block = local_hint >> self->log2_elements_per_block;
    size_t elements_per_block = 1 << self->log2_elements_per_block;
    size_t num_blocks_mask = self->num_blocks - 1;

    // Remember which block we started on
    size_t target_block = local_block;

    // Get pointer to the tail value for this block
    volatile size_t * block_tail = &self->block_tail[target_block];
    size_t index, next_index;

    do {
        // How much room will be left if we grab k items from this block?
        index = *block_tail;
        next_index = index + k;

        // Will this push us past the end of the block?
        size_t last_index = (target_block + 1) * elements_per_block;

        if (next_index > last_index) {
            // Move to next block
            target_block = (target_block + 1) % self->num_blocks;
            if (target_block != local_block) {
                block_tail = &self->block_tail[target_block];
                continue;
            } else {
                // If we get here, we've tried all the blocks and ended up at the one we started at
                // There is no empty space left, so crash
                assert(0 && "Ran out of edge blocks!");
            }
        }
        // Attempt to atomically reserve k items
        // If we fail, another thread grabbed them first so we reload and check again
    } while (index != ATOMIC_CAS(block_tail, next_index, index));

    // We have overwritten block_tail with the new tail value
    return index;
}