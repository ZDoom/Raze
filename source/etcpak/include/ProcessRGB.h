#ifndef PROCESSRGB_HPP__
#define PROCESSRGB_HPP__

#include "Types.hpp"

#ifdef __cplusplus
extern "C" {
#endif

uint64 ProcessRGB( const uint8* src );
uint64 ProcessRGB_ETC2( const uint8* src );

#ifdef __cplusplus
}
#endif

#endif
