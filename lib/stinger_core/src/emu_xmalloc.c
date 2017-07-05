/**
* Provides self-checking wrappers for emu memory functions
*/

#include <stdio.h>
#include "emu_xmalloc.h"

#if defined(__le64__)
// Include definitions of mw_malloc functions
#include <distributed.h>
#include <memoryweb.h>

// HACK Fix broken perror on emu
#define perror(X) fprintf(stderr, X "\n")


#else
// Provide reference implementation for non-emu platforms

void * FNATTR_MALLOC
mw_malloc2d(size_t nelem, size_t sz)
{
    // We need an 8-byte pointer for each element, plus the array of elements
    size_t bytes = nelem * 8 + nelem * sz;
    void ** ptrs = xcalloc(bytes, 1);
    // Skip past the pointers to get to the raw array
    void * data = (void*) ptrs + (nelem * 8);
    // Assign pointer to each element
    for (size_t i = 0; i < nelem; ++i)
    {
        ptrs[i] = data + i * sz;
    }
    return ptrs;
}

void * FNATTR_MALLOC
mw_malloc_1d(size_t nelem) {
    return xcalloc(nelem, 8);
}

void *
mw_arrayindex(void ** array2d, unsigned long i, unsigned long numelements, size_t eltsize)
{
    (void)eltsize;
    if (i >= numelements) { return NULL; }
    return array2d[i];
}

void
mw_free(void * ptr)
{
    free(ptr);
}
#endif

void *
xmw_malloc1d(size_t nelem)
{
    return xmw_malloc2d(nelem, 0);
}

void *
xmw_malloc2d(size_t nelem, size_t sz)
{
  void * out;
  out = mw_malloc2d(nelem, sz);
  if (!out) {
    perror("Failed xmw_malloc2d: ");
    abort();
  }

  return out;
}