/* The Lunatic Interpreter, part of EDuke32. Editor stuff. */

#include <lualib.h>

#include "lunatic_m32.h"


static void Em_StateSetup(lua_State *L)
{
    luaL_openlibs(L);
    L_SetupDebugTraceback(L);
}


int Em_CreateState(L_State *estate)
{
    return L_CreateState(estate, "m32", &Em_StateSetup);
}

void Em_DestroyState(L_State *estate)
{
    L_DestroyState(estate);
}
