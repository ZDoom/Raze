// Compatibility declarations for the tools to avoid linking to the entire engine.

#include "compat.h"

#ifdef __cplusplus
extern "C" {
#endif

//
// initprintf() -- prints a string
//
void initprintf(const char *f, ...)
{
    va_list va;
    char buf[2048];

    va_start(va, f);
    Bvsnprintf(buf, sizeof(buf), f, va);
    va_end(va);
}

int initputs (const char * str) { return puts(str); }

int16_t editstatus = 1;

#ifdef __cplusplus
}
#endif
