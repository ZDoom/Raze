#ifndef PROCESSRGB_H_
#define PROCESSRGB_H_

#if !defined __cplusplus || __cplusplus < 201103L
# include <stdint.h>
#else
# include <cstdint>
#endif

#ifdef __cplusplus
extern "C" {
#endif

uint64_t ProcessRGB( const uint8_t * src );
uint64_t ProcessRGB_ETC2( const uint8_t * src );

#ifdef __cplusplus
}
#endif

#endif
