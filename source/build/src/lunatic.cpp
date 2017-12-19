/* The Lunatic Interpreter, part of EDuke32. Common, engine-side stuff. */

#ifdef __cplusplus
extern "C" {
#endif

#ifdef USE_LUAJIT_2_1
# include <luajit-2.1/lua.h>
# include <luajit-2.1/lualib.h>
# include <luajit-2.1/lauxlib.h>
#else
# include <luajit-2.0/lua.h>
# include <luajit-2.0/lualib.h>
# include <luajit-2.0/lauxlib.h>
#endif

#ifdef __cplusplus
}
#endif

#include "build.h"
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

static void L_SetupDebugTraceback(lua_State *L)
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
    {
        kclose(fid);
        return 5;
    }

    buf = (char *)Xmalloc(flen+1);

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

    estate->name = Xstrdup(name);

    L = estate->L = luaL_newstate();

    if (!estate->L)
    {
        DO_FREE_AND_NULL(estate->name);
        return -2;
    }

    luaL_openlibs(L);
    L_SetupDebugTraceback(L);
    if (StateSetupFunc)
        StateSetupFunc(L);

    if (lua_gettop(L)==0)
        L_PushDebugTraceback(L);
    // Otherwise, it is assumed that StateSetupFunc pushed a custom traceback
    // function onto the stack.

    Bassert(lua_gettop(L)==1);

    return 0;
}

void L_DestroyState(L_State *estate)
{
    if (!L_IsInitialized(estate))
        return;

    DO_FREE_AND_NULL(estate->name);

    lua_close(estate->L);
    estate->L = NULL;
}

static void L_OnOutOfMem(void)
{
    OSD_Printf("Out of memory in Lunatic.\n");
    uninitengine();
    exit(127);
}

void (*L_ErrorFunc)(const char *) = NULL;
void (*L_OutOfMemFunc)(void) = L_OnOutOfMem;

int L_HandleError(lua_State *L, int errcode, void (*ErrorPrintFunc)(const char *))
{
    if (errcode == LUA_ERRMEM)
        L_OutOfMemFunc();

    if (errcode == LUA_ERRRUN || errcode == LUA_ERRERR)
    {
        if (lua_isboolean(L, -1))
        {
            int32_t killit = lua_toboolean(L, -1);
            lua_pop(L, 1);
            return killit;
        }
        else
        {
            const char *errstr = (lua_type(L, -1)==LUA_TSTRING) ?
                lua_tostring(L, -1) : "??? (error message not a string)";

            ErrorPrintFunc(errstr);
            if (L_ErrorFunc)
                L_ErrorFunc(errstr);
            lua_pop(L, 1);
            return -1;
        }
    }

    /* unreachable */
#ifndef NDEBUG
    Bassert(0);
#endif

    return 0;
}

static void L_ErrorPrint(const char *errmsg)
{
    OSD_Printf(OSD_ERROR "runtime error: %s\n", errmsg);
}

// size < 0: length of <buf> is determined using strlen()
// size >= 0: size given, for loading of LuaJIT bytecode
int L_RunString(L_State *estate, char const *buf, int size, const char *name)
{
    int32_t i;
    lua_State *L = estate->L;

    // -- lua --
    Bassert(lua_gettop(L)==1);
    // on top: a traceback function
    Bassert(lua_iscfunction(L, 1));

    if (size < 0)
        i = luaL_loadstring(L, buf);
    else
        i = luaL_loadbuffer(L, buf, size, name);
    Bassert(lua_gettop(L)==2);

    if (i == LUA_ERRMEM)
        L_OutOfMemFunc();

    if (i == LUA_ERRSYNTAX)
    {
        OSD_Printf(OSD_ERROR "state \"%s\" syntax error: %s\n", estate->name,
                   lua_tostring(L, -1));  // get err msg
        lua_pop(L, 1);  // pop errmsg
        return 3;
    }

    // call the lua chunk!
    i = lua_pcall(L, 0, 0, 1);
    Bassert(lua_gettop(L) == 1 + (i!=0));

    if (i != 0)
        L_HandleError(L, i, &L_ErrorPrint);

    Bassert(lua_gettop(L)==1);

    return i ? 4 : 0;
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

    int const retval = L_RunString(estate, buf, -1, fn);
    Bfree(buf);
    return retval;
}
