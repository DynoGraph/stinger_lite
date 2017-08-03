#ifndef EMU_ARRAY_H
#define EMU_ARRAY_H

#include <stddef.h>

/*
If you store these types as a pointer, use new/free
If you store theses types on the stack, use init/deinit
*/

struct emu_striped_array
{
    void ** data;
    size_t num_elements;
    size_t element_size;
};

struct emu_striped_array * emu_striped_array_new(size_t num_elements, size_t element_size);
void emu_striped_array_init(struct emu_striped_array * self, size_t num_elements, size_t element_size);
void emu_striped_array_deinit(struct emu_striped_array * self);
void emu_striped_array_free(struct emu_striped_array * self);
void * emu_striped_array_index(struct emu_striped_array * self, size_t i);
size_t emu_striped_array_size();


struct emu_blocked_array
{
    void ** data;
    size_t num_blocks;
    size_t element_size;
    size_t log2_elements_per_block;
    // mutable, per-nodelet
    size_t * block_tail;
};

struct emu_blocked_array * emu_blocked_array_new(size_t num_elements, size_t element_size);
void emu_blocked_array_init(struct emu_blocked_array * self, size_t num_elements, size_t element_size);
void emu_blocked_array_deinit(struct emu_blocked_array * self);
void emu_blocked_array_free(struct emu_blocked_array * self);
void * emu_blocked_array_index(struct emu_blocked_array * self, size_t i);
size_t emu_blocked_array_size();
size_t emu_blocked_array_allocate_local(struct emu_blocked_array * self, size_t k, void* locality_hint);

#endif