/* The Lunatic Interpreter, part of EDuke32. Common, engine-side stuff. */

#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>

#include "cache1d.h"
#include "osd.h"

#include "lunatic.h"


////////// HELPER FUNCTIONS //////////

// Lua-registry key for debug.traceback
static uint8_t debug_traceback_key;

void L_CheckAndRegisterFunction(lua_State *L, void *regkeyaddr)
{
    luaL_checktype(L, -1, LUA_TFUNCTION);

    lua_pushlightuserdata(L, regkeyaddr);  // 3, push address
    lua_pushvalue(L, -2);  // 4, push copy of lua function

    lua_settable(L, LUA_REGISTRYINDEX);  // "registry[regkeyaddr] = <lua function>", pop 2
}

void L_SetupDebugTraceback(lua_State *L)
{
    // get debug.traceback
    lua_getglobal(L, "debug");
    lua_getfield(L, -1, "traceback");
    Bassert(lua_isfunction(L, -1));
    L_CheckAndRegisterFunction(L, &debug_traceback_key);
    lua_pop(L, 2);
}

void L_PushDebugTraceback(lua_State *L)
{
    // get debug.traceback
    lua_pushlightuserdata(L, &debug_traceback_key);
    lua_gettable(L, LUA_REGISTRYINDEX);
    Bassert(lua_isfunction(L, -1));
}


static int32_t read_whole_file(const char *fn, char **retbufptr)
{
    int32_t fid, flen, i;
    char *buf;

    *retbufptr = NULL;

    fid = kopen4load(fn, 0);  // TODO: g_loadFromGroupOnly, kopen4loadfrommod ?

    if (fid < 0)
        return 1;

    flen = kfilelength(fid);
    if (flen == 0)
        return 5;

    buf = Bmalloc(flen+1);
    if (!buf)
    {
        kclose(fid);
        return -1;
    }

    i = kread(fid, buf, flen);
    kclose(fid);

    if (i != flen)
    {
        Bfree(buf);
        return 2;
    }

    buf[flen] = 0;
    *retbufptr = buf;

    return 0;
}


////////// EXTERNAL FUNCTIONS //////////

// 0: success, <0: failure
int L_CreateState(L_State *estate, const char *name, void (*StateSetupFunc)(lua_State *))
{
    lua_State *L;

    estate->name = Bstrdup(name);
    if (!estate->name)
        return -1;

    L = estate->L = luaL_newstate();

    if (!estate->L)
    {
        Bfree(estate->name);
        estate->name = NULL;
        return -2;
    }

    StateSetupFunc(L);

    return 0;
}

void L_DestroyState(L_State *estate)
{
    if (!estate->L)
        return;

    Bfree(estate->name);
    estate->name = NULL;

    lua_close(estate->L);
    estate->L = NULL;
}

void (*L_ErrorFunc)(const char *) = NULL;

int L_RunString(L_State *estate, char *buf, int dofreebuf)
{
    int32_t i;
    lua_State *L = estate->L;

    // -- lua --
    Bassert(lua_gettop(L)==0);

    L_PushDebugTraceback(L);

    i = luaL_loadstring(L, buf);
    Bassert(lua_gettop(L)==2);
    if (dofreebuf)
        Bfree(buf);

    if (i == LUA_ERRMEM)
    {
        lua_pop(L, 2);
        return -1;
    }

    if (i == LUA_ERRSYNTAX)
    {
        OSD_Printf(OSD_ERROR "state \"%s\" syntax error: %s\n", estate->name,
                   lua_tostring(L, -1));  // get err msg
        lua_pop(L, 2);  // pop errmsg and debug.traceback
        return 3;
    }

    // -- call the lua chunk! --

    i = lua_pcall(L, 0, 0, -2);
    Bassert(lua_gettop(L) == 1+!!i);
    Bassert(i != LUA_ERRERR);  // we expect debug.traceback not to fail

    if (i == LUA_ERRMEM)  // XXX: should be more sophisticated.  Clean up stack? Do GC?
    {
        lua_pop(L, 2);
        return -1;
    }

    if (i == LUA_ERRRUN)
    {
        // get error message if possible
        const char *errstr = (lua_type(L, -1)==LUA_TSTRING) ?
            lua_tostring(L, -1) : "??? (errmsg not a string)";

        OSD_Printf(OSD_ERROR "state \"%s\" runtime error: %s\n", estate->name, errstr);
        if (L_ErrorFunc)
            L_ErrorFunc(errstr);
        lua_pop(L, 2);  // pop errmsg and debug.traceback
        return 4;
    }

    lua_pop(L, 1);

    return 0;
}

// -1: alloc failure
// 0: success
// 1: didn't find file
// 2: couldn't read whole file
// 3: syntax error in lua file
// 4: runtime error while executing lua file
// 5: empty file
int L_RunOnce(L_State *estate, const char *fn)
{
    char *buf;
    int32_t i = read_whole_file(fn, &buf);

    if (i != 0)
        return i;

    return L_RunString(estate, buf, 1);
}
