// This file's main purpose is to be recompiled unconditionally so the timestamp gets updated, even for a partial recompile.

#if !defined REV
# define REV "r(?)"
#endif

const char* s_buildRev = REV;
const char* s_buildTimestamp = __DATE__ " " __TIME__;
