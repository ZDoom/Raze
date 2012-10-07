/* The Lunatic Interpreter, part of EDuke32/Mapster32 */

#include <stdint.h>

#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>

#include "lunatic_m32.h"
#include "lunatic_priv.h"


Em_State *Em_CreateState(void)
{
    lua_State *L = luaL_newstate();

    if (L == NULL)
        return NULL;

    luaL_openlibs(L);
    setup_debug_traceback(L);

    return L;
}

// -1: alloc failure
// 0: success
// 1: didn't find file
// 2: couldn't read whole file
// 3: syntax error in lua file
// 4: runtime error while executing lua file
// 5: empty file
int32_t Em_RunOnce(Em_State *L, const char *fn)
{
    return lunatic_run_once(L, fn, "file");
}

int32_t Em_RunStringOnce(Em_State *L, const char *str)
{
    return lunatic_run_string(L, (char *)str, 0, "string");
}
