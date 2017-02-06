/**
* Provides self-checking wrappers for emu memory functions
*/

#include "emu_xmalloc.h"

#if defined(__le64__)
// Include definitions of mw_malloc functions
#include <distributed.h>
#include <memoryweb.h>

// HACK Fix broken perror on emu
#include <stdio.h>
#define perror(X) fprintf(stderr, X "\n")

// Provide reference implementation for non-emu platforms
#else
void * mw_malloc2d(size_t nelem, size_t sz) FNATTR_MALLOC;

void *
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
