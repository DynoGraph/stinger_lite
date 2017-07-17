// May be included multiple times
// Must define EMU_ARRAY

// Macro substitution doesn't happen when the concatenation operator ## is present
// So we need another layer of indirection to make sure macro arguments get expanded
#define CONCAT_MACRO(X, Y) X ## Y
#define CONCAT(X, Y) CONCAT_MACRO(X, Y)

#define emu_array             EMU_ARRAY
#define emu_array_test        CONCAT(EMU_ARRAY, _test)
#define emu_array_new         CONCAT(EMU_ARRAY, _new)
#define emu_array_init        CONCAT(EMU_ARRAY, _init)
#define emu_array_deinit      CONCAT(EMU_ARRAY, _deinit)
#define emu_array_free        CONCAT(EMU_ARRAY, _free)
#define emu_array_index       CONCAT(EMU_ARRAY, _index)
#define emu_array_size        CONCAT(EMU_ARRAY, _size)

void emu_array_test ()
{
    
// TEST 1
    // Allocate array of ints
    size_t n1 = 100;
    struct emu_array array1 = {0};
    emu_array_init(&array1, n1, sizeof(int));

    // Go through the entire array
    for (size_t i = 0; i < n1; ++i)
    {
        int * x = emu_array_index(&array1, i);
        // Check that values were initialized to zero
        assert(*x == 0);
        // Set value to index position
        *x = i;
    }

    // Make sure the values were set correctly on a second pass
    for (size_t i = 0; i < n1; ++i)
    {
        int * x = emu_array_index(&array1, i);
        assert(*x == i);
    }  

    // Deallocate array
    emu_array_deinit(&array1);

}


#undef emu_array_test
#undef emu_array_new
#undef emu_array_init
#undef emu_array_deinit
#undef emu_array_free
#undef emu_array_index
#undef emu_array_size
