// This file's main purpose is to be recompiled whenever the Makefile slams rev.h (usually always) so the timestamp gets updated, even for a partial recompile.

const char *
    #include "rev.h"

const char* s_buildTimestamp = __DATE__ " " __TIME__;
