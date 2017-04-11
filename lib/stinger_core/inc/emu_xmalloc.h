#if !defined(EMU_XMALLOC_H_)
#define EMU_XMALLOC_H_

#include "xmalloc.h"

void *
xmw_malloc1d(size_t nelem) FNATTR_MALLOC;

void *
xmw_malloc2d(size_t nelem, size_t sz) FNATTR_MALLOC;

#ifndef __le64__
void
mw_free(void * ptr);

void *
mw_arrayindex(void ** array2d, unsigned long i, unsigned long numelements, size_t eltsize);

#endif
#endif // EMU_XMALLOC_H_