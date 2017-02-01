
#define DIRECTINPUT_VERSION 0x0700
#include "dx/dinput.h"

#undef DEFINE_GUID
#define DEFINE_GUID(n,a,b,c,d,e,f,g,h,i,j,k) const GUID n = {a,b,c,{d,e,f,g,h,i,j,k}}

/* dsound.h */
DEFINE_GUID(IID_IDirectSoundNotify, 0xb0210783, 0x89cd, 0x11d0, 0xaf, 0x8, 0x0, 0xa0, 0xc9, 0x25, 0xcd, 0x16);
