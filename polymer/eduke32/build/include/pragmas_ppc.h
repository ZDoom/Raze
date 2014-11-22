// GCC Inline Assembler version (PowerPC)

#ifdef pragmas_h_
#ifndef pragmas_ppc_h_
#define pragmas_ppc_h_

#define EDUKE32_SCALER_PRAGMA(x) \
static inline int32_t mulscale##x(int32_t a, int32_t d) \
{ \
	int32_t mullo, mulhi; \
	__asm__ ( \
		" mullw  %0, %2, %3\n" \
		" mulhw  %1, %2, %3\n" \
		" srwi   %0, %0, %4\n" \
		" insrwi %0, %1, %4, 0\n" \
		: "=&r"(mullo), "=r"(mulhi) \
		: "r"(a), "r"(d), "i"(x) \
	); \
	return mullo; \
} \
static inline int32_t dmulscale##x(int32_t a, int32_t d, int32_t S, int32_t D) \
{ \
	int32_t mulhi, mullo, sumhi, sumlo; \
	__asm__ ( \
		" mullw  %0, %4, %5\n" \
		" mulhw  %1, %4, %5\n" \
		" mullw  %2, %6, %7\n" \
		" mulhw  %3, %6, %7\n" \
		" addc   %0, %0, %2\n" \
		" adde   %1, %1, %3\n" \
		" srwi   %0, %0, %8\n" \
		" insrwi %0, %1, %8, 0\n" \
		: "=&r"(sumlo), "=&r"(sumhi), "=&r"(mullo), "=r"(mulhi) \
		: "r"(a), "r"(d), "r"(S), "r"(D), "i"(x) \
		: "xer" \
	); \
	return sumlo; \
}

EDUKE32_GENERATE_PRAGMAS
#undef EDUKE32_SCALER_PRAGMA

static inline int32_t mulscale(int32_t a, int32_t d, int32_t c)
{
    int32_t mullo, mulhi;
    __asm__(
        " mullw  %0, %2, %3\n"
        " mulhw  %1, %2, %3\n"
        " srw    %0, %0, %4\n"
        " slw    %1, %1, %5\n"
        " or     %0, %0, %1\n"
        : "=&r"(mullo), "=&r"(mulhi)
        : "r"(a), "r"(d), "r"(c), "r"(32-c)
        : "xer"
        );
    return mullo;
}

static inline int32_t mulscale32(int32_t a, int32_t d)
{
    int32_t mulhi;
    __asm__(
        " mulhw %0, %1, %2\n"
        : "=r"(mulhi)
        : "r"(a), "r"(d)
        );
    return mulhi;
}

static inline int32_t dmulscale(int32_t a, int32_t d, int32_t S, int32_t D, int32_t c)
{
    int32_t mulhi, mullo, sumhi, sumlo;
    __asm__(
        " mullw  %0, %4, %5\n"
        " mulhw  %1, %4, %5\n"
        " mullw  %2, %6, %7\n"
        " mulhw  %3, %6, %7\n"
        " addc   %0, %0, %2\n"
        " adde   %1, %1, %3\n"
        " srw    %0, %0, %8\n"
        " slw    %1, %1, %9\n"
        " or     %0, %0, %1\n"
        : "=&r"(sumlo), "=&r"(sumhi), "=&r"(mullo), "=&r"(mulhi)
        : "r"(a), "r"(d), "r"(S), "r"(D), "r"(c), "r"(32-c)
        : "xer"
        );
    return sumlo;
}

static inline int32_t dmulscale32(int32_t a, int32_t d, int32_t S, int32_t D)
{
    int32_t mulhi, mullo, sumhi, sumlo;
    __asm__(\
        " mullw  %0, %4, %5\n" \
        " mulhw  %1, %4, %5\n" \
        " mullw  %2, %6, %7\n" \
        " mulhw  %3, %6, %7\n" \
        " addc   %0, %0, %2\n" \
        " adde   %1, %1, %3\n" \
        : "=&r"(sumlo), "=&r"(sumhi), "=&r"(mullo), "=r"(mulhi)
        : "r"(a), "r"(d), "r"(S), "r"(D)
        : "xer"
        );
    return sumhi;
}

static inline char readpixel(void *d)
{
    return *(char*) d;
}

static inline void drawpixel(void *d, char a)
{
    *(char*) d = a;
}

void clearbufbyte(void *d, int32_t c, int32_t a);

static inline void clearbuf(void *d, int32_t c, int32_t a)
{
    int32_t *p = (int32_t*) d;
    if (a==0) {
        clearbufbyte(d, c<<2, 0);
        return;
    }
    while (c--) {
        *p++ = a;
    }
}

static inline void copybuf(void *s, void *d, int32_t c)
{
    int32_t *p = (int32_t*) s, *q = (int32_t*) d;
    while (c--) {
        *q++ = *p++;
    }
}

static inline void copybufbyte(void *s, void *d, int32_t c)
{
    uint8_t *src = (uint8_t*) s, *dst = (uint8_t*) d;
    while (c--) {
        *dst++ = *src++;
    }
}

static inline void copybufreverse(void *s, void *d, int32_t c)
{
    uint8_t *src = (uint8_t*) s, *dst = (uint8_t*) d;
    while (c--) {
        *dst++ = *src--;
    }
}

static inline void qinterpolatedown16(intptr_t bufptr, int32_t num, int32_t val, int32_t add)
{
    int i;
    int32_t *lptr = (int32_t *) bufptr;
    for (i=0; i<num; i++) {
        lptr[i] = (val>>16);
        val += add;
    }
}

static inline void qinterpolatedown16short(intptr_t bufptr, int32_t num, int32_t val, int32_t add)
{
    int i;
    int16_t *sptr = (int16_t *) bufptr;
    for (i=0; i<num; i++) {
        sptr[i] = (val>>16);
        val += add;
    }
}

static inline int32_t klabs(int32_t a)
{
    int32_t mask;
    __asm__(
        " srawi  %0, %1, 31\n"
        " xor    %1, %0, %1\n"
        " subf   %1, %0, %1\n"
        : "=&r"(mask), "+r"(a)
        :
        : "xer"
        );
    return a;
}

static inline int32_t ksgn(int32_t a)
{
    int32_t s, t;
    __asm__(
        " neg    %1, %2\n"
        " srawi  %0, %2, 31\n"
        " srwi   %1, %1, 31\n"
        " or	 %1, %1, %0\n"
        : "=r"(t), "=&r"(s)
        : "r"(a)
        : "xer"
        );
    return s;
}

static inline void swapchar(void *a, void *b)
{
    char t = *(char*) a;
    *(char*) a = *(char*) b;
    *(char*) b = t;
}

static inline void swapchar2(void *a, void *b, int32_t s)
{
    swapchar(a, b);
    swapchar((char*) a+1, (char*) b+s);
}

static inline void swapshort(void *a, void *b)
{
    int16_t t = *(int16_t*) a;
    *(int16_t*) a = *(int16_t*) b;
    *(int16_t*) b = t;
}

static inline void swaplong(void *a, void *b)
{
    int32_t t = *(int32_t*) a;
    *(int32_t*) a = *(int32_t*) b;
    *(int32_t*) b = t;
}

static inline void swapfloat(void *a, void *b)
{
    float t = *(float*) a;
    *(float*) a = *(float*) b;
    *(float*) b = t;
}

static inline void swap64bit(void *a, void *b)
{
    double t = *(double*) a;
    *(double*) a = *(double*) b;
    *(double*) b = t;
}

static inline int32_t krecipasm(int32_t i)
{
    // Ken did this
    float f = (float) i; i = *(int32_t *) &f;
    return((reciptable[(i>>12)&2047]>>(((i-0x3f800000)>>23)&31))^(i>>31));
}

#endif // pragmas_ppc_h_
#endif // pragmas_h_
