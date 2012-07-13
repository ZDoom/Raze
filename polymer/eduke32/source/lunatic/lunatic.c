/* The Lunatic Interpreter, part of EDuke32 */

#include <stdint.h>

#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>

#include "cache1d.h"
#include "osd.h"

#include "gameexec.h"
#include "gamedef.h"  // EventNames[], MAXEVENTS
#include "lunatic.h"

// this serves two purposes:
// the values as booleans and the addresses as keys to the Lua registry
uint8_t g_elEvents[MAXEVENTS];

// same thing for actors:
uint8_t g_elActors[MAXTILES];

// Lua-registry key for debug.traceback
static uint8_t debug_traceback_key;


// forward-decls...
static int32_t SetEvent_luacf(lua_State *L);
static int32_t SetActor_luacf(lua_State *L);

// in lpeg.o
extern int luaopen_lpeg(lua_State *L);


static void check_and_register_function(lua_State *L, void *keyaddr)
{
    luaL_checktype(L, -1, LUA_TFUNCTION);

    lua_pushlightuserdata(L, keyaddr);  // 3, push address
    lua_pushvalue(L, -2);  // 4, push copy of lua function

    lua_settable(L, LUA_REGISTRYINDEX);  // "registry[keyaddr] = <lua function>", pop 2
}


// 0: success, <0: failure
int32_t El_CreateState(El_State *estate, const char *name)
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

    luaL_openlibs(L);  // NOTE: we set up the sandbox in defs.ilua
    luaopen_lpeg(L);
    lua_pop(L, lua_gettop(L));  // pop off whatever lpeg leaves on the stack

    // get debug.traceback
    lua_getglobal(L, "debug");
    lua_getfield(L, -1, "traceback");
    Bassert(lua_isfunction(L, -1));
    check_and_register_function(L, &debug_traceback_key);
    lua_pop(L, 2);

    // create misc. global functions in the Lua state
    lua_pushcfunction(L, SetEvent_luacf);
    lua_setglobal(L, "gameevent");
    lua_pushcfunction(L, SetActor_luacf);
    lua_setglobal(L, "gameactor");

    Bassert(lua_gettop(L)==0);

    return 0;
}

void El_DestroyState(El_State *estate)
{
    if (!estate->L)
        return;

    Bfree(estate->name);
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

    lua_State *const L = estate->L;

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
    Bassert(lua_gettop(L)==0);

    // get debug.traceback
    lua_pushlightuserdata(L, &debug_traceback_key);
    lua_gettable(L, LUA_REGISTRYINDEX);
    Bassert(lua_isfunction(L, -1));

    i = luaL_loadstring(L, buf);
    Bassert(lua_gettop(L)==2);
    Bfree(buf);

    if (i == LUA_ERRMEM)
    {
        lua_pop(L, 2);
        return -1;
    }

    if (i == LUA_ERRSYNTAX)
    {
        OSD_Printf("state \"%s\" syntax error: %s\n", estate->name,
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
        Bassert(lua_type(L, -1)==LUA_TSTRING);
        OSD_Printf("state \"%s\" runtime error: %s\n", estate->name,
                   lua_tostring(L, -1));  // get err msg
        lua_pop(L, 2);  // pop errmsg and debug.traceback
        return 4;
    }

    lua_pop(L, 1);

    return 0;
}


////////// Lua_CFunctions //////////

// gameevent(EVENT_..., lua_function)
static int32_t SetEvent_luacf(lua_State *L)
{
    int32_t eventidx;

    if (lua_gettop(L) != 2)
        luaL_error(L, "gameevent: must pass exactly two arguments");

    eventidx = luaL_checkint(L, 1);

    luaL_argcheck(L, (unsigned)eventidx < MAXEVENTS, 1, "must be an event number (0 .. MAXEVENTS-1)");
    check_and_register_function(L, &g_elEvents[eventidx]);
    g_elEvents[eventidx] = 1;

    return 0;
}

// gameactor(<actortile>, lua_function)
static int32_t SetActor_luacf(lua_State *L)
{
    int32_t actortile;

    if (lua_gettop(L) != 2)
        luaL_error(L, "gameactor: must pass exactly two arguments");

    actortile = luaL_checkint(L, 1);

    luaL_argcheck(L, (unsigned)actortile < MAXTILES, 1, "must be an tile number (0 .. MAXTILES-1)");
    check_and_register_function(L, &g_elActors[actortile]);
    g_elActors[actortile] = 1;

    return 0;
}

//////////////////////////////

static int32_t call_registered_function3(lua_State *L, void *keyaddr,
                                         int32_t iActor, int32_t iPlayer, int32_t lDist)
{
    int32_t i;

    // get the Lua function from the registry
    lua_pushlightuserdata(L, keyaddr);
    lua_gettable(L, LUA_REGISTRYINDEX);

    lua_pushinteger(L, iActor);
    lua_pushinteger(L, iPlayer);
    lua_pushinteger(L, lDist);

    // -- call it! --

    i = lua_pcall(L, 3, 0, 0);
    if (i == LUA_ERRMEM)
    {
        lua_pop(L, 1);
        // XXX: should be more sophisticated.  Clean up stack? Do GC?
    }

    return i;
}

int32_t El_CallEvent(El_State *estate, int32_t eventidx, int32_t iActor, int32_t iPlayer, int32_t lDist)
{
    // XXX: estate must be the one where the events were registered...
    //      make a global?

    lua_State *const L = estate->L;

    int32_t i = call_registered_function3(L, &g_elEvents[eventidx], iActor, iPlayer, lDist);

    if (i == LUA_ERRRUN)
    {
        OSD_Printf("event \"%s\" (state \"%s\") runtime error: %s\n", EventNames[eventidx].text,
                   estate->name, lua_tostring(L, -1));  // get err msg
        lua_pop(L, 1);
        return 4;
    }

    return 0;
}

int32_t El_CallActor(El_State *estate, int32_t actortile, int32_t iActor, int32_t iPlayer, int32_t lDist)
{
    lua_State *const L = estate->L;

    int32_t i = call_registered_function3(L, &g_elActors[actortile], iActor, iPlayer, lDist);

    if (i == LUA_ERRRUN)
    {
        OSD_Printf("actor %d (sprite %d, state \"%s\") runtime error: %s\n", actortile, iActor,
                   estate->name, lua_tostring(L, -1));  // get err msg
        lua_pop(L, 1);
        return 4;
    }

    return 0;
}
