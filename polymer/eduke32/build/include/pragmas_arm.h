//
// GCC Inline Assembler version (ARM)
//

#ifndef pragmas_arm_h_
#define pragmas_arm_h_

#define EDUKE32_SCALER_PRAGMA(a)                                                                                       \
    FORCE_INLINE int32_t mulscale##a(int32_t eax, int32_t edx) { return dw((qw(eax) * edx) >> by(a)); }                \
    FORCE_INLINE int32_t dmulscale##a(int32_t eax, int32_t edx, int32_t esi, int32_t edi)                              \
    {                                                                                                                  \
        return dw(((qw(eax) * edx) + (qw(esi) * edi)) >> by(a));                                                       \
    }

EDUKE32_GENERATE_PRAGMAS EDUKE32_SCALER_PRAGMA(32)

#undef EDUKE32_SCALER_PRAGMA

FORCE_INLINE void swapchar(void *a, void *b)
{
    char t = *((char *)b);
    *((char *)b) = *((char *)a);
    *((char *)a) = t;
}
FORCE_INLINE void swapchar2(void *a, void *b, int32_t s)
{
    swapchar(a, b);
    swapchar((char *)a + 1, (char *)b + s);
}
FORCE_INLINE void swapshort(void *a, void *b)
{
    int16_t const t = *((int16_t *)b);
    *((int16_t *)b) = *((int16_t *)a);
    *((int16_t *)a) = t;
}
FORCE_INLINE void swaplong(void *a, void *b)
{
    int32_t const t = *((int32_t *)b);
    *((int32_t *)b) = *((int32_t *)a);
    *((int32_t *)a) = t;
}
FORCE_INLINE void swapfloat(void *a, void *b)
{
    float const t = *((float *)b);
    *((float *)b) = *((float *)a);
    *((float *)a) = t;
}
FORCE_INLINE void swapdouble(void *a, void *b)
{
    double const t = *((double *) b);
    *((double *) b) = *((double *) a);
    *((double *) a) = t;
}
FORCE_INLINE void swap64bit(void *a, void *b)
{
    uint64_t const t = *((uint64_t *)b);
    *((uint64_t *)b) = *((uint64_t *)a);
    *((uint64_t *)a) = t;
}

FORCE_INLINE char readpixel(void *s) { return (*((char *)(s))); }
FORCE_INLINE void drawpixel(void *s, char a) { *((char *)(s)) = a; }

FORCE_INLINE int32_t klabs(int32_t a)
{
    const uint32_t m = a >> (sizeof(uint32_t) * CHAR_BIT - 1);
    return (a ^ m) - m;
}
FORCE_INLINE int32_t ksgn(int32_t a) { return (a > 0) - (a < 0); }

FORCE_INLINE int32_t mulscale(int32_t eax, int32_t edx, int32_t ecx) { return dw((qw(eax) * qw(edx)) >> by(ecx)); }
FORCE_INLINE int32_t dmulscale(int32_t eax, int32_t edx, int32_t esi, int32_t edi, int32_t ecx)
{
    return dw(((qw(eax) * qw(edx)) + (qw(esi) * qw(edi))) >> by(ecx));
}

void qinterpolatedown16(intptr_t bufptr, int32_t num, int32_t val, int32_t add);
void qinterpolatedown16short(intptr_t bufptr, int32_t num, int32_t val, int32_t add);

void clearbuf(void *d, int32_t c, int32_t a);
void copybuf(const void *s, void *d, int32_t c);
void swapbuf4(void *a, void *b, int32_t c);

void clearbufbyte(void *D, int32_t c, int32_t a);
void copybufbyte(const void *S, void *D, int32_t c);
void copybufreverse(const void *S, void *D, int32_t c);

static inline int32_t krecipasm(int32_t i)
{
    // Ken did this
    float const f = (float const)i;
    i = *(int32_t const *)&f;
    return ((reciptable[(i >> 12) & 2047] >> (((i - 0x3f800000) >> 23) & 31)) ^ (i >> 31));
}
#endif
