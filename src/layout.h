#ifndef EMU_LAYOUT_H
#define EMU_LAYOUT_H

#include <memoryweb.h>

struct emu_pointer {
    uint64_t view;
    uint64_t node_id;
    uint64_t nodelet_id;
    uint64_t nodelet_addr;
    uint64_t byte_offset;
};

struct emu_pointer
examine_emu_pointer(void * ptr);

void print_emu_pointer(void * ptr);

int pointers_are_on_same_nodelet(void * a, void *b);

#endif