// SDL interface layer
// for the Build Engine
// by Jonathon Fowler (jf@jonof.id.au)

#ifndef build_interface_layer_
#define build_interface_layer_ SDL

#include "baselayer.h"
#include "compat.h"


inline void idle(int const msec = 1)
{
#ifdef _WIN32
    Sleep(msec);
#else
    timespec const req = { 0, msec * 1000000 };
    do { } while (nanosleep(&req, &req));
#endif
}

#else
#if (build_interface_layer_ != SDL)
#error "Already using the " build_interface_layer_ ". Can't now use SDL."
#endif
#endif // build_interface_layer_

