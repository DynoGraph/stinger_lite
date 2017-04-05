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
    void ** ptrs = xcalloc(nelem, 8);
    void * data = xcalloc(nelem, sz);
    for (size_t i = 0; i < nelem; ++i)
    {
        ptrs[i] = data + i * sz;
    }
    return ptrs;
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