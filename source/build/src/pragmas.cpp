// Function-wrapped Watcom pragmas
// by Jonathon Fowler (jf@jonof.id.au)
//
// These functions represent some of the more longer-winded pragmas
// from the original pragmas.h wrapped into functions for easier
// use since many jumps and whatnot make it harder to write macro-
// inline versions. I'll eventually convert these to macro-inline
// equivalents.		--Jonathon

#include "compat.h"
#include "pragmas.h"

libdivide::libdivide_s64_t divtable64[DIVTABLESIZE];
libdivide::libdivide_s32_t divtable32[DIVTABLESIZE];

void initdivtables(void)
{
    for (int i = 1; i < DIVTABLESIZE; ++i)
    {
        divtable64[i] = libdivide::libdivide_s64_gen(i);
        divtable32[i] = libdivide::libdivide_s32_gen(i);
    }
}

