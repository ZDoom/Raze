// GCC Inline Assembler version (PowerPC)

#ifdef pragmas_h_
#ifndef pragmas_ppc_h_
#define pragmas_ppc_h_

#define pragmas_have_mulscale

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
} \
static inline int32_t tmulscale##x(int32_t a, int32_t d, int32_t b, int32_t c, int32_t S, int32_t D) \
{ \
    int32_t mulhi, mullo, sumhi, sumlo; \
    __asm__( \
        " mullw  %0, %4, %5\n" \
        " mulhw  %1, %4, %5\n" \
        " mullw  %2, %6, %7\n" \
        " mulhw  %3, %6, %7\n" \
        " addc   %0, %0, %2\n" \
        " adde   %1, %1, %3\n" \
        " mullw  %2, %8, %9\n" \
        " mulhw  %3, %8, %9\n" \
        " addc   %0, %0, %2\n" \
        " adde   %1, %1, %3\n" \
        " srwi   %0, %0, %10\n" \
        " insrwi %0, %1, %10, 0\n" \
        : "=&r"(sumlo), "=&r"(sumhi), "=&r"(mullo), "=&r"(mulhi) \
        : "r"(a), "r"(d), "r"(b), "r"(c), "r"(S), "r"(D), "i"(x) \
        : "xer" \
        ); \
    return sumlo; \
} \

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

static inline int32_t tmulscale32(int32_t a, int32_t d, int32_t b, int32_t c, int32_t S, int32_t D)
{
    int32_t mulhi, mullo, sumhi, sumlo;
    __asm__(
        " mullw  %0, %4, %5\n"
        " mulhw  %1, %4, %5\n"
        " mullw  %2, %6, %7\n"
        " mulhw  %3, %6, %7\n"
        " addc   %0, %0, %2\n"
        " adde   %1, %1, %3\n"
        " mullw  %2, %8, %9\n"
        " mulhw  %3, %8, %9\n"
        " addc   %0, %0, %2\n"
        " adde   %1, %1, %3\n"
        : "=&r"(sumlo), "=&r"(sumhi), "=&r"(mullo), "=&r"(mulhi)
        : "r"(a), "r"(d), "r"(b), "r"(c), "r"(S), "r"(D)
        : "xer"
    );
    return sumhi;
}

#define pragmas_have_klabs

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

#define pragmas_have_ksgn

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

#endif // pragmas_ppc_h_
#endif // pragmas_h_
