/* The Lunatic Interpreter, part of EDuke32 */

#include <stdint.h>

#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>

#include "cache1d.h"
#include "osd.h"

#include "gameexec.h"
#include "gamedef.h"  // EventNames[]
#include "lunatic.h"

// this serves two purposes:
// the values as booleans and the addresses as keys to the Lua registry
static uint8_t g_elEvents[MAXEVENTS];


// forward-decls...
static int32_t SetEvent_luacf(lua_State *L);

// in lpeg.o
extern int luaopen_lpeg(lua_State *L);


// 0: success, <0: failure
int32_t El_CreateState(El_State *estate, const char *name)
{
    estate->name = Bstrdup(name);
    if (!estate->name)
        return -1;

    estate->L = luaL_newstate();

    if (!estate->L)
    {
        Bfree((char *)estate->name);
        estate->name = NULL;
        return -2;
    }

    luaL_openlibs(estate->L);  // XXX: only for internal use and testing, obviously
    luaopen_lpeg(estate->L);

    // create misc. global functions in the Lua state
    lua_pushcfunction(estate->L, SetEvent_luacf);
    lua_setglobal(estate->L, "setevent");

    return 0;
}

void El_DestroyState(El_State *estate)
{
    if (!estate->L)
        return;

    Bfree((char *)estate->name);
    estate->name = NULL;

    lua_close(estate->L);
    estate->L = NULL;
}

// -1: alloc failure
// 0: success
// 1: didn't find file
// 2: couldn't read whole file
// 3: syntax error in lua file
// 4: runtime error while executing lua file
int32_t El_RunOnce(El_State *estate, const char *fn)
{
    int32_t fid, flen, i;
    char *buf;

    fid = kopen4load(fn, 0);

    if (fid < 0)
        return 1;

    flen = kfilelength(fid);
    if (flen == 0)
        return 0;  // empty script ...

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

    // -- lua --

    i = luaL_loadstring(estate->L, buf);
    Bfree(buf);

    if (i == LUA_ERRMEM)
        return -1;

    if (i == LUA_ERRSYNTAX)
    {
        OSD_Printf("state \"%s\" syntax error: %s\n", estate->name, lua_tostring(estate->L, 1));  // get err msg
        lua_pop(estate->L, 1);
        return 3;
    }

    // -- call the lua chunk! --

    i = lua_pcall(estate->L, 0, 0, 0);
    if (i == LUA_ERRMEM)  // XXX: should be more sophisticated.  Clean up stack? Do GC?
        return -1;

    if (i == LUA_ERRRUN)
    {
        assert(lua_type(estate->L, -1)==LUA_TSTRING);
        OSD_Printf("state \"%s\" runtime error: %s\n", estate->name, lua_tostring(estate->L, -1));  // get err msg
        lua_pop(estate->L, 1);
        return 4;
    }

    return 0;
}

// setupevent(EVENT_..., lua_function)
static int32_t SetEvent_luacf(lua_State *L)
{
    int32_t eventidx = luaL_checkint(L, 1);

    luaL_argcheck(L, (unsigned)eventidx < MAXEVENTS, 1, "must be an event number (0 .. MAXEVENTS-1)");
    luaL_checktype(L, 2, LUA_TFUNCTION);

    lua_pushlightuserdata(L, &g_elEvents[eventidx]);  // 3, push address
    lua_pushvalue(L, 2);  // 4, push copy of lua function

    lua_settable(L, LUA_REGISTRYINDEX);  // "registry[&g_elEvents[eventidx]] = <lua function>"
    g_elEvents[eventidx] = 1;

    return 0;
}

int32_t El_CallEvent(El_State *estate, int32_t eventidx)
{
    // XXX: estate must be the one where the events were registered...
    //      make a global?

    int32_t i;

    if (!g_elEvents[eventidx])
        return 0;

    lua_pushlightuserdata(estate->L, &g_elEvents[eventidx]);  // push address
    lua_gettable(estate->L, LUA_REGISTRYINDEX);  // get lua function

    // -- call it! --

    i = lua_pcall(estate->L, 0, 0, 0);
    if (i == LUA_ERRMEM)  // XXX: should be more sophisticated.  Clean up stack? Do GC?
        return -1;

    if (i == LUA_ERRRUN)
    {
        OSD_Printf("event \"%s\" (state \"%s\") runtime error: %s\n", EventNames[eventidx].text,
                   estate->name, lua_tostring(estate->L, 1));  // get err msg
        lua_pop(estate->L, 1);
        return 4;
    }

    return 0;
}
