#include "emu-array.h"
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

void
emu_blocked_array_init(struct emu_blocked_array * self, size_t num_elements, size_t element_size)
{
    assert(!self->data);
    self->element_size = element_size;

    // We will allocate one block on each nodelet
    self->num_blocks = NODELETS();

    // How many elements in each block?
    size_t elements_per_block = num_elements / self->num_blocks;
    // Round up to nearest power of two
    elements_per_block = 1 << (PRIORITY(elements_per_block) + 1);
    self->log2_elements_per_block = PRIORITY(elements_per_block);

    // Allocate a block on each nodelet
    self->data = xmw_calloc2d(self->num_blocks, elements_per_block * element_size);
    assert(self->data);
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