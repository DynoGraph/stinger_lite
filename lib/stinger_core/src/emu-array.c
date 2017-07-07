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
    self->data = xmw_malloc2d(num_elements, element_size);
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
    self->num_elements = num_elements;
    self->element_size = element_size;
    self->elements_per_nodelet = ((num_elements + NODELETS()) / NODELETS()) - 1; // round up
    self->data = xmw_malloc2d(NODELETS(), self->elements_per_nodelet * element_size);
    assert(self->data);
}

void
emu_blocked_array_deinit(struct emu_blocked_array * self)
{
    assert(self->data);
    mw_free(self->data);
    self->data = 0;
    self->num_elements = 0;
    self->element_size = 0;
    self->elements_per_nodelet = 0;
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
    assert(i < self->num_elements);

    void * base = mw_arrayindex(self->data, i/self->elements_per_nodelet, self->num_elements, self->element_size);
    return base + (i % self->elements_per_nodelet) * self->element_size; // TODO rewrite using shifts, ensure powers of 2
}


size_t
emu_blocked_array_size(struct emu_blocked_array * self)
{
    assert(self->data);
    return self->num_elements;
}