#ifndef EMU_ARRAY_H
#define EMU_ARRAY_H

struct emu_striped_array
{
    void ** data;
    size_t num_elements;
    size_t element_size;
};


void emu_striped_array_init(struct emu_striped_array * self, size_t num_elements, size_t element_size);
void emu_striped_array_free(struct emu_striped_array * self);
void * emu_striped_array_index(struct emu_striped_array * self, size_t i);
size_t emu_striped_array_size();


struct emu_blocked_array
{
    void ** data;
    size_t num_elements;
    size_t element_size;
    size_t elements_per_nodelet;
};

void emu_blocked_array_init(struct emu_blocked_array * self, size_t num_elements, size_t element_size);
void emu_blocked_array_free(struct emu_blocked_array * self);
void * emu_blocked_array_index(struct emu_blocked_array * self, size_t i);
size_t emu_blocked_array_size();

#endif