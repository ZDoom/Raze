#ifndef _INCLUDE_PLATFORM_H_
#define _INCLUDE_PLATFORM_H_

#if (!defined __EXPORT__)
#define __EXPORT__
#endif

#if (defined __WATCOMC__)
#define snprintf _snprintf
#endif

static __inline uint16_t _swap16(uint16_t D)
{
#if PLATFORM_MACOSX
    register uint16_t returnValue;
    __asm__ volatile("lhbrx %0,0,%1"
        : "=r" (returnValue)
        : "r" (&D)
    );
    return returnValue;
#else
    return((D<<8)|(D>>8));
#endif
}

static __inline uint32_t _swap32(uint32_t D)
{
#if PLATFORM_MACOSX
    register uint32_t returnValue;
    __asm__ volatile("lwbrx %0,0,%1"
        : "=r" (returnValue)
        : "r" (&D)
    );
    return returnValue;
#else
    return((D<<24)|((D<<8)&0x00FF0000)|((D>>8)&0x0000FF00)|(D>>24));
#endif
}

#if PLATFORM_MACOSX
#define PLATFORM_BIGENDIAN 1
#define BUILDSWAP_INTEL16(x) _swap16(x)
#define BUILDSWAP_INTEL32(x) _swap32(x)
#else
#if __BYTE_ORDER == __LITTLE_ENDIAN
#define PLATFORM_LITTLEENDIAN 1
#define BUILDSWAP_INTEL16(x) (x)
#define BUILDSWAP_INTEL32(x) (x)
#else
#define PLATFORM_BIGENDIAN 1
#define BUILDSWAP_INTEL16(x) _swap16(x)
#define BUILDSWAP_INTEL32(x) _swap32(x)
#endif
#endif

extern int32_t has_altivec;  /* PowerPC-specific. */

#endif  /* !defined _INCLUDE_PLATFORM_H_ */

/* end of platform.h ... */

