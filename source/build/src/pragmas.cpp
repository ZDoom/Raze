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

#ifndef pragmas_have_clearbuf
void clearbuf(void *d, int32_t c, int32_t a)
{
    auto p = (int32_t *)d;

#if 0
    if (a == 0)
    {
        clearbufbyte(d, c<<2, 0);
        return;
    }
#endif

    while (c--)
        *p++ = a;
}
#endif

#ifndef pragmas_have_copybuf
void copybuf(const void *s, void *d, int32_t c)
{
    auto p = (const int32_t *) s;
    auto q = (int32_t *) d;

    while (c--)
        *q++ = *p++;
}
#endif

#ifndef pragmas_have_swaps
void swapbuf4(void *a, void *b, int32_t c)
{
    auto p = (int32_t *) a;
    auto q = (int32_t *) b;

    while ((c--) > 0)
    {
        int x = *q, y = *p;
        *(q++) = y;
        *(p++) = x;
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


// copybufreverse() is a special case: use the assembly version for GCC on x86
// *and* x86_64, and the C version otherwise.
// XXX: we don't honor NOASM in the x86_64 case.

#if defined(__GNUC__) && defined(__x86_64__)
// NOTE: Almost CODEDUP from x86 GCC assembly version, except that
// - %%esi -> %%rsi
// - %%edi -> %%rdi
// - (dec,inc,sub,add)l suffix removed where necessary
void copybufreverse(const void *S, void *D, int32_t c)
{
    __asm__ __volatile__(
        "shrl $1, %%ecx\n\t"
        "jnc 0f\n\t"		// jnc skipit1
        "movb (%%rsi), %%al\n\t"
        "dec %%rsi\n\t"
        "movb %%al, (%%rdi)\n\t"
        "inc %%rdi\n\t"
        "0:\n\t"		// skipit1:
        "shrl $1, %%ecx\n\t"
        "jnc 1f\n\t"		// jnc skipit2
        "movw -1(%%rsi), %%ax\n\t"
        "sub $2, %%rsi\n\t"
        "rorw $8, %%ax\n\t"
        "movw %%ax, (%%rdi)\n\t"
        "add $2, %%rdi\n\t"
        "1:\n\t"		// skipit2
        "testl %%ecx, %%ecx\n\t"
        "jz 3f\n\t"		// jz endloop
        "2:\n\t"		// begloop
        "movl -3(%%rsi), %%eax\n\t"
        "sub $4, %%rsi\n\t"
        "bswapl %%eax\n\t"
        "movl %%eax, (%%rdi)\n\t"
        "add $4, %%rdi\n\t"
        "decl %%ecx\n\t"
        "jnz 2b\n\t"		// jnz begloop
        "3:"
    : "+S"(S), "+D"(D), "+c"(c) :
            : "eax", "memory", "cc"
        );
}
#elif !defined pragmas_have_copybufreverse
void copybufreverse(const void *s, void *d, int32_t c)
{
    auto src = (const char *)s;
    auto dst = (char *)d;

    while (c--)
        *dst++ = *src--;
}
#endif
