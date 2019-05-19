// "Build Engine & Tools" Copyright (c) 1993-1997 Ken Silverman
// Ken Silverman's official web site: "http://www.advsys.net/ken"
// See the included license file "BUILDLIC.TXT" for license info.
//
// This file has been modified from Ken Silverman's original release
// by Jonathon Fowler (jf@jonof.id.au)
// by the EDuke32 team (development@voidpoint.com)

#ifndef klzw_h_
#define klzw_h_

#include "compat.h"

#ifdef __cplusplus
extern "C" {
#endif

// These two follow the argument order of the C functions "read" and "write":
// handle, buffer, length.
typedef int32_t (*klzw_readfunc)(intptr_t, void *, int32_t);
typedef void (*klzw_writefunc)(intptr_t, void const *, int32_t);

int32_t klzw_read_compressed(void *buffer, int dasizeof, int count, intptr_t const f, klzw_readfunc readfunc);
void klzw_write_compressed(const void * const buffer, int dasizeof, int count, intptr_t const f, klzw_writefunc writefunc);

#ifdef __cplusplus
}
#endif

#endif // klzw_h_
