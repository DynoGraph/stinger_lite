/**
* Provides self-checking wrappers for emu memory functions
*/

#include "emu_xmalloc.h"
#include <distributed.h>
#include <memoryweb.h>


// HACK Fix broken perror on emu
#if defined(__le64__)
  #include <stdio.h>
  #define perror(X) fprintf(stderr, X "\n")
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
