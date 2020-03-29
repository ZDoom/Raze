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

uint32_t divideu32_noinline(uint32_t n, uint32_t d) { return divideu32(n, d); }
int32_t tabledivide32_noinline(int32_t n, int32_t d) { return tabledivide32(n, d); }
int64_t tabledivide64_noinline(int64_t n, int64_t d) { return tabledivide64(n, d); }


//
// Generic C version
//

#ifndef pragmas_have_qinterpolatedown16
void qinterpolatedown16(intptr_t bufptr, int32_t num, int32_t val, int32_t add)
{
    auto lptr = (int32_t *)bufptr;
    for (size_t i = 0, i_end = num; i < i_end; ++i)
    {
        lptr[i] = val>>16;
        val += add;
    }
}

void qinterpolatedown16short(intptr_t bufptr, int32_t num, int32_t val, int32_t add)
{
    auto sptr = (int16_t *)bufptr;
    for (size_t i = 0, i_end = num; i < i_end; ++i)
    {
        sptr[i] = val>>16;
        val += add;
    }
}
#endif

#ifndef pragmas_have_clearbufbyte
void clearbufbyte(void *D, int32_t c, int32_t a)
{
    // Cringe City
    constexpr int32_t m[4] = { 0xffl, 0xff00l, 0xff0000l, (int32_t)0xff000000l };
    int   z = 0;
    auto p = (char *)D;

    while ((c--) > 0)
    {
        *(p++) = (uint8_t)((a & m[z])>>(z<<3));
        z=(z+1)&3;
    }
}
#endif

#ifndef pragmas_have_copybufbyte
void copybufbyte(const void *s, void *d, int32_t c)
{
    auto src = (const char *)s;
    auto dst = (char *)d;

    while (c--)
        *dst++ = *src++;
}
#endif


