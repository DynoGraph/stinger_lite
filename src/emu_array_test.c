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

#include "layout.h"

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


static size_t
view1_nodelet_id(void * ptr)
{
    return ((size_t)ptr & __MW_VIEW1_NODE_MASK__) >> __MW_NODE_BITS__;
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
        size_t hint = i << __MW_NODE_BITS__;
        size_t location = emu_blocked_array_allocate_local(&array, 1, hint);
        void * element1 = emu_blocked_array_index(&array, location);
        ASSERT_EQ(i, view1_nodelet_id(element1));

        // The next item allocated should be adjacent on the same nodelet
        location = emu_blocked_array_allocate_local(&array, 1, hint);
        void * element2 = emu_blocked_array_index(&array, location);
        ASSERT_EQ(i, view1_nodelet_id(element2));

        assert(pointers_are_on_same_nodelet(element1, element2));
    }

    // Deallocate array
    emu_blocked_array_deinit(&array);
}


replicated long r1;
replicated long r2;


void force_migration()
{
    // Set local copy to 5
    r1 = 5;
    printf("r1 = %lu\n", r1);
    // Get pointer to local copy
    long *ptr = &r1;
    printf("ptr = "); print_emu_pointer(ptr); printf("\n");
    // Get pointer to remote copy
    ptr = mw_get_nth(ptr, 2);
    printf("ptr = "); print_emu_pointer(ptr); printf("\n");
    // Set remote copy to 3 (causes migration)
    *ptr = 3;
    // Print remote copy
    printf("*ptr = %lu\n", *ptr);
    // Print local copy
    printf("r1 = %lu\n", r1);
    // Get pointer to local copy
    ptr = &r1;
    printf("ptr = "); print_emu_pointer(ptr); printf("\n");
}

void print_pointers()
{
    void * ptr;

    ptr = mw_mallocstripe(1024);
    printf("mallocstripe: "); print_emu_pointer(ptr); printf("\n");

    // Manual recommended way: add BYTES_PER_NODELET() to get to next pointer
    printf("+ BYTES_PER_NODELET: "); print_emu_pointer(ptr + BYTES_PER_NODELET()); printf("\n");


    printf("__MW_VIEW1_NODE_MASK__: %lx\n", __MW_VIEW1_NODE_MASK__ >> __MW_NODE_BITS__);


    // Alternate way: use get_nth
    printf("get_nth(ptr, 1): "); print_emu_pointer(mw_get_nth(ptr, 1)); printf("\n");
    mw_free(ptr);

    ptr = mw_localmalloc(1024, 0);
    printf("localmalloc: "); print_emu_pointer(ptr); printf("\n");
    mw_localfree(ptr);

    ptr = mw_malloc1dlong(1024);
    printf("malloc1d: "); print_emu_pointer(ptr); printf("\n");
    mw_free(ptr);

    ptr = mw_malloc2d(1024, 1024);
    printf("malloc2d: "); print_emu_pointer(ptr); printf("\n");
    printf("malloc2d[2]: "); print_emu_pointer(((void**)ptr)[2]); printf("\n");
    printf("malloc2d + 2: "); print_emu_pointer(ptr + 2); printf("\n");
    printf("malloc2d + 16: "); print_emu_pointer(ptr + 16); printf("\n");
    mw_free(ptr);

    long x;
    ptr = &x;
    printf("local: "); print_emu_pointer(ptr); printf("\n");

    ptr = &r1;
    printf("r1: "); print_emu_pointer(ptr); printf("\n");
    ptr = mw_get_nth(&r1, 0);
    printf("get_nth(r1, 0): "); print_emu_pointer(ptr); printf("\n");
    ptr = mw_get_nth(&r1, 2);
    printf("get_nth(r1, 2): "); print_emu_pointer(ptr); printf("\n");
    ptr = mw_get_localto(&r1, mw_get_nth(&r2, 0));
    printf("get_localto(r1, get_nth(r2, 0): "); print_emu_pointer(ptr); printf("\n");
    ptr = mw_get_localto(&r1, mw_get_nth(&r2, 2));
    printf("get_localto(r1, get_nth(r2, 2): "); print_emu_pointer(ptr); printf("\n");
}


int main(int argc, char *argv[])
{

    // force_migration();
    // print_pointers();

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