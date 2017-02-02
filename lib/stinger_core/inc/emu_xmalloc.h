#if !defined(EMU_XMALLOC_H_)
#define EMU_XMALLOC_H_

#include "xmalloc.h"

void *
xmw_malloc2d(size_t nelem, size_t sz) FNATTR_MALLOC;

#endif