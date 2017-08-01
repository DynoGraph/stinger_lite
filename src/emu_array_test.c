#include <stinger_core/emu_array.h>

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#define ASSERT_EQ(X, Y) \
do {                    \
    if (X != Y) {       \
        printf("%llu != %llu\n", (size_t)X, (size_t)Y); \
        assert(X == Y); \
    }                   \
} while (0)

#if defined(__le64__)
#include <memoryweb.h>
#else
#define NODELETS() 1
#define BYTES_PER_NODELET() stinger_max_memsize()
#define replicated
#endif


// Define test functions
#undef EMU_ARRAY
#define EMU_ARRAY emu_blocked_array
#include "emu_array_test.inc"
#undef EMU_ARRAY

#undef EMU_ARRAY
#define EMU_ARRAY emu_striped_array
#include "emu_array_test.inc"
#undef EMU_ARRAY

void emu_blocked_array_integrity_test()
{
    // Allocate arrays of varying sizes and assert that internal sizes are sane
    size_t test_sizes[] = {1, 2, 8, 100, 128, 150, 1024};

    for (size_t i = 0; i < sizeof(test_sizes)/sizeof(test_sizes[0]); ++i)
    {
        size_t n = test_sizes[i];
        struct emu_blocked_array array = {0};
        emu_blocked_array_init(&array, n, sizeof(int));
        assert(emu_blocked_array_size(&array) >= n);
    }
}


void emu_blocked_array_allocation_test()
{
    // Allocate array of ints
    size_t n = 128;
    struct emu_blocked_array array = {0};
    emu_blocked_array_init(&array, n, sizeof(int));

    // With locality hint '0', elements allocated in order
    for (size_t i = 0; i < n; ++i)
    {
        size_t location = emu_blocked_array_allocate_local(&array, 1, 0);
        ASSERT_EQ(location, i);
    }

    // Reinitialize for another test
    emu_blocked_array_deinit(&array);
    emu_blocked_array_init(&array, n, sizeof(int));

    // Ask for one element from each nodelet
    for (size_t i = 0; i < NODELETS(); ++i)
    {
        size_t hint = i << array.log2_elements_per_block;
        size_t location = emu_blocked_array_allocate_local(&array, 1, hint);
        ASSERT_EQ(location, hint);
        // The next item allocated should be adjacent on the same nodelet
        location = emu_blocked_array_allocate_local(&array, 1, hint);
        ASSERT_EQ(location, hint + 1);
    }

    // Deallocate array
    emu_blocked_array_deinit(&array);
}


int main(int argc, char *argv[])
{
    fprintf(stderr, "Beginning emu_striped_array_test...\n");
    emu_striped_array_test();

    fprintf(stderr, "Beginning emu_blocked_array_integrity_test...\n");
    emu_blocked_array_integrity_test();
    fprintf(stderr, "Beginning emu_blocked_array_test...\n");
    emu_blocked_array_test();
    fprintf(stderr, "Beginning emu_blocked_array_allocation_test...\n");
    emu_blocked_array_allocation_test();

    printf("Done\n");
    return 0;
}