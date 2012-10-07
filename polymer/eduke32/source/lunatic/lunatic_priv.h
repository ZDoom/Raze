/* Private, game/editor-common Lunatic routines.
 * This file is intended to be included exactly ONCE in the game and editor
 * Lunatic sources. */

#include <lua.h>
#include <lauxlib.h>

#include "cache1d.h"
#include "osd.h"


// Lua-registry key for debug.traceback
static uint8_t debug_traceback_key;

static void check_and_register_function(lua_State *L, void *regkeyaddr)
{
    luaL_checktype(L, -1, LUA_TFUNCTION);

    lua_pushlightuserdata(L, regkeyaddr);  // 3, push address
    lua_pushvalue(L, -2);  // 4, push copy of lua function

    lua_settable(L, LUA_REGISTRYINDEX);  // "registry[regkeyaddr] = <lua function>", pop 2
}

static void setup_debug_traceback(lua_State *L)
{
    // get debug.traceback
    lua_getglobal(L, "debug");
    lua_getfield(L, -1, "traceback");
    Bassert(lua_isfunction(L, -1));
    check_and_register_function(L, &debug_traceback_key);
    lua_pop(L, 2);
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

static int32_t lunatic_run_string(lua_State *L, char *buf, int32_t dofreebuf, const char *statename)
{
    int32_t i;

    // -- lua --
    Bassert(lua_gettop(L)==0);

    // get debug.traceback
    lua_pushlightuserdata(L, &debug_traceback_key);
    lua_gettable(L, LUA_REGISTRYINDEX);
    Bassert(lua_isfunction(L, -1));

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
        OSD_Printf("state \"%s\" syntax error: %s\n", statename,
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
        int32_t stringp = (lua_type(L, -1)==LUA_TSTRING);

        // get error message if possible
        OSD_Printf("state \"%s\" runtime error: %s\n", statename,
                   stringp ? lua_tostring(L, -1) : "??? (errmsg not a string)");
        lua_pop(L, 2);  // pop errmsg and debug.traceback
        return 4;
    }

    lua_pop(L, 1);

    return 0;
}

static int32_t lunatic_run_once(lua_State *L, const char *fn, const char *statename)
{
    int32_t i;
    char *buf;

    i = read_whole_file(fn, &buf);
    if (i != 0)
        return i;

    return lunatic_run_string(L, buf, 1, statename);
}
