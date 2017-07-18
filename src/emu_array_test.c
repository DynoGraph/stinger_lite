#include <stinger_core/emu_array.h>

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>



// Define test functions
#undef EMU_ARRAY
#define EMU_ARRAY emu_blocked_array
#include "emu_array_test.inc"
#undef EMU_ARRAY

#undef EMU_ARRAY
#define EMU_ARRAY emu_striped_array
#include "emu_array_test.inc"
#undef EMU_ARRAY



int main(int argc, char *argv[])
{
    fprintf(stderr, "Beginning emu_striped_array_test...\n");
    emu_striped_array_test();
    fprintf(stderr, "Beginning emu_blocked_array_test...\n");
    emu_blocked_array_test();
    printf("Done\n");
    return 0;
}