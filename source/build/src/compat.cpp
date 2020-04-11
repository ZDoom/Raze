/*
 * Playing-field leveler for Build
 */

#define LIBDIVIDE_BODY
#include "compat.h"
#include "debugbreak.h"


#include "baselayer.h"

////////// PANICKING ALLOCATION FUNCTIONS //////////

static void (*g_MemErrHandler)(int32_t line, const char *file, const char *func);

#ifdef DEBUGGINGAIDS
static const char *g_MemErrFunc = "???";
static const char *g_MemErrFile = "???";
static int32_t g_MemErrLine;

void xalloc_set_location(int32_t line, const char *file, const char *func)
{
    g_MemErrLine = line;
    g_MemErrFile = file;

    if (func)
        g_MemErrFunc = func;
}
#endif

void *handle_memerr(void *p)
{
    UNREFERENCED_PARAMETER(p);
    debug_break();

    if (g_MemErrHandler)
    {
#ifdef DEBUGGINGAIDS
        g_MemErrHandler(g_MemErrLine, g_MemErrFile, g_MemErrFunc);
#else
        g_MemErrHandler(0, "???", "???");
#endif
    }

    Bexit(EXIT_FAILURE);
    EDUKE32_UNREACHABLE_SECTION(return &handle_memerr);
}

void set_memerr_handler(void(*handlerfunc)(int32_t, const char *, const char *))
{
    g_MemErrHandler = handlerfunc;
}



char *Bstrtolower(char *str)
{
    if (str) for (int i = 0; str[i]; i++) str[i] = tolower(str[i]);
    return str;
}
