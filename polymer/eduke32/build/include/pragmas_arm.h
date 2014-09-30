//
// GCC Inline Assembler version (ARM)
//

#ifndef __pragmas_arm_h__
#define __pragmas_arm_h__

#define _scaler(a) \
static inline int32_t mulscale##a(int32_t eax, int32_t edx) \
{ \
	return dw((qw(eax) * qw(edx)) >> by(a)); \
} \
\
static inline int32_t dmulscale##a(int32_t eax, int32_t edx, int32_t esi, int32_t edi) \
{ \
	return dw(((qw(eax) * qw(edx)) + (qw(esi) * qw(edi))) >> by(a)); \
} \
\
static inline int32_t tmulscale##a(int32_t eax, int32_t edx, int32_t ebx, int32_t ecx, int32_t esi, int32_t edi) \
{ \
	return dw(((qw(eax) * qw(edx)) + (qw(ebx) * qw(ecx)) + (qw(esi) * qw(edi))) >> by(a)); \
} \

PRAGMA_FUNCS _scaler(32)

#undef _scaler

static inline void swapchar(void* a, void* b)  { char t = *((char*) b); *((char*) b) = *((char*) a); *((char*) a) = t; }
static inline void swapchar2(void* a, void* b, int32_t s) { swapchar(a, b); swapchar((char*) a+1, (char*) b+s); }
static inline void swapshort(void* a, void* b) { int16_t t = *((int16_t*) b); *((int16_t*) b) = *((int16_t*) a); *((int16_t*) a) = t; }
static inline void swaplong(void* a, void* b)  { int32_t t = *((int32_t*) b); *((int32_t*) b) = *((int32_t*) a); *((int32_t*) a) = t; }
static inline void swap64bit(void* a, void* b) { int64_t t = *((int64_t*) b); *((int64_t*) b) = *((int64_t*) a); *((int64_t*) a) = t; }

static inline char readpixel(void* s)    { return (*((char*) (s))); }
static inline void drawpixel(void* s, char a)    { *((char*) (s)) = a; }
static inline void drawpixels(void* s, int16_t a)  { *((int16_t*) (s)) = a; }
static inline void drawpixelses(void* s, int32_t a) { *((int32_t*) (s)) = a; }

static inline int32_t mul3(int32_t a) { return (a<<1)+a; }
static inline int32_t mul5(int32_t a) { return (a<<2)+a; }
static inline int32_t mul9(int32_t a) { return (a<<3)+a; }

static inline int32_t divmod(int32_t a, int32_t b) { uint32_t _a=(uint32_t) a, _b=(uint32_t) b; dmval = _a%_b; return _a/_b; }
static inline int32_t moddiv(int32_t a, int32_t b) { uint32_t _a=(uint32_t) a, _b=(uint32_t) b; dmval = _a/_b; return _a%_b; }

static inline int32_t klabs(int32_t a) { if (a < 0) return -a; return a; }
static inline int32_t ksgn(int32_t a)  { if (a > 0) return 1; if (a < 0) return -1; return 0; }

static inline int32_t umin(int32_t a, int32_t b) { if ((uint32_t) a < (uint32_t) b) return a; return b; }
static inline int32_t umax(int32_t a, int32_t b) { if ((uint32_t) a < (uint32_t) b) return b; return a; }
static inline int32_t kmin(int32_t a, int32_t b) { if ((int32_t) a < (int32_t) b) return a; return b; }
static inline int32_t kmax(int32_t a, int32_t b) { if ((int32_t) a < (int32_t) b) return b; return a; }

static inline int32_t sqr(int32_t eax) { return (eax) * (eax); }
static inline int32_t scale(int32_t eax, int32_t edx, int32_t ecx) { return dw((qw(eax) * qw(edx)) / qw(ecx)); }
static inline int32_t mulscale(int32_t eax, int32_t edx, int32_t ecx) { return dw((qw(eax) * qw(edx)) >> by(ecx)); }
static inline int32_t dmulscale(int32_t eax, int32_t edx, int32_t esi, int32_t edi, int32_t ecx) { return dw(((qw(eax) * qw(edx)) + (qw(esi) * qw(edi))) >> by(ecx)); }

void qinterpolatedown16(intptr_t bufptr, int32_t num, int32_t val, int32_t add);
void qinterpolatedown16short(intptr_t bufptr, int32_t num, int32_t val, int32_t add);

void clearbuf(void* d, int32_t c, int32_t a);
void copybuf(const void* s, void* d, int32_t c);
void swapbuf4(void* a, void* b, int32_t c);

void clearbufbyte(void *D, int32_t c, int32_t a);
void copybufbyte(const void *S, void *D, int32_t c);
void copybufreverse(const void *S, void *D, int32_t c);
#endif
