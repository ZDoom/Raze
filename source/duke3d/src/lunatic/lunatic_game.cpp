/* The Lunatic Interpreter, part of EDuke32. Game-side stuff. */

#include "compat.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifdef USE_LUAJIT_2_1
# include <luajit-2.1/lualib.h>
# include <luajit-2.1/lauxlib.h>
#else
# include <luajit-2.0/lualib.h>
# include <luajit-2.0/lauxlib.h>
#endif

#ifdef __cplusplus
}
#endif

#include "build.h"  // printext256

#include "lunatic_game.h"

#include "osd.h"
#include "gamedef.h"  // EventNames[]


L_State g_ElState;


// this serves two purposes:
// the values as booleans and the addresses as keys to the Lua registry
uint8_t g_elEvents[MAXEVENTS];

// same thing for actors:
el_actor_t g_elActors[MAXTILES];

// Session variable. Never restored except by 'readgamevar'.
int32_t g_elSessionVar[8];  // MAXSESSIONVARS, KEEPINSYNC con_lang.lua

// Set to 1 on error in event.
int32_t g_elEventError;

// Will be set to 0 after the first time that user Lua modules are run.
int32_t g_elFirstTime = 1;

int32_t g_elCallDepth = 0;
int32_t g_RETURN;

// for timing events and actors
static int32_t g_timingInited = 0;
uint32_t g_eventCalls[MAXEVENTS], g_actorCalls[MAXTILES];
double g_eventTotalMs[MAXEVENTS], g_actorTotalMs[MAXTILES], g_actorMinMs[MAXTILES], g_actorMaxMs[MAXTILES];

// Used as Lua registry key to the tweak_traceback_msg() function, set to 1 if
// such a function has been registered.
static uint8_t g_tweakTracebackMsg = 0;

// forward-decls...
static int32_t SetEvent_CF(lua_State *L);
static int32_t SetActor_CF(lua_State *L);


#ifdef __cplusplus
extern "C" {
#endif

// in lpeg.o
extern int luaopen_lpeg(lua_State *L);

#ifdef __cplusplus
}
#endif



// See: Good Practice in (Pseudo) Random Number Generation for
//      Bioinformatics Applications, by David Jones
ATTRIBUTE_OPTIMIZE("O2")
LUNATIC_EXTERN uint32_t rand_jkiss_u32(rng_jkiss_t *s)
{
    uint64_t t;
    s->x = 314527869 * s->x + 1234567;
    s->y ^= s->y << 5; s->y ^= s->y >> 7; s->y ^= s->y << 22;
    t = 4294584393ULL * s->z + s->c; s->c = t >> 32; s->z = t;
    return s->x + s->y + s->z;
}

ATTRIBUTE_OPTIMIZE("O2")
LUNATIC_EXTERN double rand_jkiss_dbl(rng_jkiss_t *s)
{
    double x;
    unsigned int a, b;
    a = rand_jkiss_u32(s) >> 6; /* Upper 26 bits */
    b = rand_jkiss_u32(s) >> 5; /* Upper 27 bits */
    x = (a * 134217728.0 + b) / 9007199254740992.0;
    return x;
}


void El_PrintTimes(void)
{
    int32_t i;
    const char nn = Bstrlen("EVENT_");

    // Try environment variable specifying the base name (sans ".actors.csv" or
    // ".events.csv") for a CSV file to output, for further processing in e.g.
    // GSL shell: http://www.nongnu.org/gsl-shell/
    const char *basefn = getenv("LUNATIC_TIMING_BASEFN");

    if (basefn != NULL)
    {
        const int32_t baselen = Bstrlen(basefn);
        const int32_t addnlen = Bstrlen(".actors.csv");  // MUST equal that of ".events.csv"

        char *fullfn = (char *)Xmalloc(baselen + addnlen + 1);
        BFILE *outf;

        if (fullfn == NULL)
            return;

        Bmemcpy(fullfn, basefn, baselen);

        // EVENTS
        Bmemcpy(fullfn+baselen, ".events.csv", addnlen+1);
        outf = Bfopen(fullfn, "w");
        if (outf == NULL)
        {
            OSD_Printf("Couldn't open \"%s\" for writing timing data: %s", fullfn, strerror(errno));
            goto finish;
        }

        Bfprintf(outf, "evtname,numcalls,total_ms,mean_us\n");  // times in usecs are per-call
        for (i=0; i<MAXEVENTS; i++)
            if (g_eventCalls[i])
                Bfprintf(outf, "%s,%d,%f,%f\n", EventNames[i]+nn, g_eventCalls[i], g_eventTotalMs[i],
                         1000*g_eventTotalMs[i]/g_eventCalls[i]);
        Bfclose(outf);

        // ACTORS
        Bmemcpy(fullfn+baselen, ".actors.csv", addnlen+1);
        outf = Bfopen(fullfn, "w");
        if (outf == NULL)
        {
            OSD_Printf("Couldn't open \"%s\" for writing timing data: %s", fullfn, strerror(errno));
            goto finish;
        }

        Bfprintf(outf, "tilenum,numcalls,total_ms,min_us,mean_us,max_us\n");
        for (i=0; i<MAXTILES; i++)
            if (g_actorCalls[i])
                Bfprintf(outf, "%d,%d,%f,%f,%f,%f\n", i, g_actorCalls[i], g_actorTotalMs[i],
                         1000*g_actorMinMs[i],
                         1000*g_actorTotalMs[i]/g_actorCalls[i],
                         1000*g_actorMaxMs[i]);
        Bfclose(outf);

        OSD_Printf("Wrote timing data to \"%s.*.csv\"\n", basefn);
finish:
        Bfree(fullfn);
        return;
    }
    else
    {
        // If not writing out CSV files, print timing data to log instead.

        char buf[32];
        int32_t maxlen = 0;
        int32_t haveev=0, haveac=0;

        for (i=0; i<MAXEVENTS; i++)
        {
            int32_t len = Bstrlen(EventNames[i]+nn);
            Bassert(len < (int32_t)sizeof(buf));
            maxlen = max(len, maxlen);
        }

        for (i=0; i<MAXEVENTS; i++)
            if (g_eventCalls[i])
            {
                int32_t n=Bsprintf(buf, "%s", EventNames[i]+nn);

                if (!haveev)
                {
                    haveev = 1;
                    OSD_Printf("\n  -- event times: [event]={ total calls, total time [ms], mean time/call [us] }\n");
                }

                for (; n<maxlen; n++)
                    buf[n] = ' ';
                buf[maxlen] = 0;

                OSD_Printf("  [%s]={ %8d, %10.3f, %10.3f },\n",
                           buf, g_eventCalls[i], g_eventTotalMs[i],
                           1000*g_eventTotalMs[i]/g_eventCalls[i]);
            }

        for (i=0; i<MAXTILES; i++)
            if (g_actorCalls[i])
            {
                if (!haveac)
                {
                    haveac = 1;
                    OSD_Printf("\n  -- actor times: [tile]={ total calls, total time [ms], {min,mean,max} time/call [us] }\n");
                }

                OSD_Printf("  [%5d]={ %8d, %9.3f, %9.3f, %9.3f, %9.3f },\n",
                           i, g_actorCalls[i], g_actorTotalMs[i],
                           1000*g_actorMinMs[i],
                           1000*g_actorTotalMs[i]/g_actorCalls[i],
                           1000*g_actorMaxMs[i]);
            }
    }
}

////////// ERROR REPORTING //////////
#define EL_MAXERRORS 20
static int32_t el_numErrors=0, el_tooMuchErrors;
static char *el_errorMsgs[EL_MAXERRORS];
int8_t el_addNewErrors = 1;  // add new errors to display?

// Compare against all other error messages.
// Strictly seen, this is quadratic-time, but EL_MAXERRORS is small and
// errors should be fixed anyway.
static int32_t cmp_against_others(const char *str, int32_t slen)
{
    int32_t i;
    for (i=0; i<el_numErrors; i++)
        if (!Bstrncmp(str, el_errorMsgs[i], slen))
            return 1;
    return 0;
}

LUNATIC_EXTERN void El_OnError(const char *str)
{
    if (el_addNewErrors && !el_tooMuchErrors)
    {
        char *errstr = NULL;
        const char *nl = Bstrchr(str, '\n');

        // First, check whether the error message matches an already saved one
        if (nl)
        {
            // cut off string after the newline
            if (cmp_against_others(str, nl-str))
                return;
        }
        else
        {
            // save string fully
            if (cmp_against_others(str, Bstrlen(str)))
                return;
        }

        // If the (EL_MAXERRORS+1)'th distinct error appeared, we have too many.
        if (el_numErrors==EL_MAXERRORS)
        {
            el_tooMuchErrors = 1;
            return;
        }

        // Otherwise, allocate storage for the potentially clipped error string...
        if (nl)
        {
            errstr = (char *)Xmalloc(nl-str+1);
            Bmemcpy(errstr, str, nl-str);
            errstr[nl-str] = 0;
        }
        else
        {
            errstr = Xstrdup(str);
        }

        // ...and save it:
        el_errorMsgs[el_numErrors++] = errstr;
    }
}

void El_ClearErrors(void)
{
    int32_t i;
    for (i=0; i<EL_MAXERRORS; i++)
        DO_FREE_AND_NULL(el_errorMsgs[i]);
    el_numErrors = el_tooMuchErrors = 0;
}

void El_DisplayErrors(void)
{
    int32_t i;
    for (i=0; i<el_numErrors; i++)
        printext256(8, 8+8*i, 242, 0, el_errorMsgs[i], 0);
    if (el_tooMuchErrors)
        printext256(8, 8+8*EL_MAXERRORS, 242, 0, "(more distinct errors ...)", 0);
}


////////// STATE CREATION/DESTRUCTIION //////////

static int our_traceback_CF(lua_State *L)
{
    Bassert(lua_gettop(L)==1);

    if (lua_type(L, 1)==LUA_TBOOLEAN)
    {
        lua_pushvalue(L, 1);  // duplicate it
        return 1;  // and tell Lua to return it
    }

    Bassert(lua_type(L, 1)==LUA_TSTRING);

    // call debug.traceback with the string
    L_PushDebugTraceback(L);
    lua_pushvalue(L, 1);
    lua_call(L, 1, 1);
    Bassert(lua_gettop(L)==2);  // Lua will pop off args

    if (g_tweakTracebackMsg)
    {
        // Get tweak_traceback_msg() onto the stack.
        lua_pushlightuserdata(L, &g_tweakTracebackMsg);
        lua_gettable(L, LUA_REGISTRYINDEX);

        lua_pushvalue(L, -2);  // push copy of error message string
        Bassert(lua_type(L, -1)==LUA_TSTRING);

        // Call tweak_traceback_msg(). CAREFUL, it's unprotected!
        lua_call(L, 1, 1);
    }

    return 1;
}

// Registers a function: str = tweak_traceback_msg(str)
static int32_t SetTweakTracebackMsg_CF(lua_State *L)
{
    Bassert(lua_gettop(L)==1);
    L_CheckAndRegisterFunction(L, &g_tweakTracebackMsg);
    g_tweakTracebackMsg = 1;
    return 0;
}


////// Lua C-API interfaces for C game functions that may call events.
// http://www.freelists.org/post/luajit/intermitten-lua-pcall-crash-on-x86-64-linux,1

// Some of these are duplicate declarations:
#ifdef __cplusplus
extern "C" {
#endif
extern void P_AddWeaponMaybeSwitchI(int32_t snum, int32_t weap);
void P_CheckWeaponI(int playerNum);
extern int32_t A_ShootWithZvel(int32_t i, int32_t atwith, int32_t override_zvel);
extern int A_Spawn(int j, int pn);
extern void VM_FallSprite(int32_t i);
extern int32_t VM_ResetPlayer2(int32_t snum, int32_t flags);
extern void A_RadiusDamage(int32_t i, int32_t r, int32_t, int32_t, int32_t, int32_t);
extern void G_OperateSectors(int sectNum, int spriteNum);
extern void G_OperateActivators(int32_t low,int32_t snum);
int32_t A_InsertSprite(int16_t whatsect,int32_t s_x,int32_t s_y,int32_t s_z,int16_t s_pn,int8_t s_s,
                              uint8_t s_xr,uint8_t s_yr,int16_t s_a,int16_t s_ve,int16_t s_zv,int16_t s_ow,int16_t s_ss);
extern void A_AddToDeleteQueue(int32_t i);
extern int32_t A_PlaySound(uint32_t num, int32_t i);
void A_DeleteSprite(int spriteNum);
extern void G_ShowView(vec3_t vec, int32_t a, int32_t horiz, int32_t sect,
                       int32_t x1, int32_t y1, int32_t x2, int32_t y2, int32_t unbiasedp);
extern void G_GameExit(const char *msg);
#ifdef __cplusplus
}
#endif

static void G_ShowViewXYZ(int32_t x, int32_t y, int32_t z, int32_t a, int32_t horiz, int32_t sect,
                          int32_t x1, int32_t y1, int32_t x2, int32_t y2, int32_t unbiasedp)
{
    vec3_t vec = { x, y, z };
    G_ShowView(vec, a, horiz, sect, x1, y1, x2, y2, unbiasedp);
}

#define LARG(index) lua_tointeger(L, index)

#define ONE_ARG LARG(1)
#define TWO_ARGS LARG(1), LARG(2)
#define THREE_ARGS LARG(1), LARG(2), LARG(3)

#define CALL_WITH_RET(Name, ...) \
    int32_t ret = Name(__VA_ARGS__); \
    lua_pushinteger(L, ret); \
    return 1

#define CALL_WITHOUT_RET(Name, ...) \
    Name(__VA_ARGS__); \
    return 0

#define DEFINE_RET_CFUNC(Name, ...) \
static int32_t Name##_CF(lua_State *L) \
{ \
    CALL_WITH_RET(Name, __VA_ARGS__); \
}

#define DEFINE_VOID_CFUNC(Name, ...) \
static int32_t Name##_CF(lua_State *L) \
{ \
    CALL_WITHOUT_RET(Name, __VA_ARGS__); \
}

// NOTE: player struct -> player index -> player struct ugliness because
// pointers to FFI cdata apparently can't be reliably passed via lua_getpointer().
// Not to mention that lua_getpointer() returns _const_ void*.
DEFINE_VOID_CFUNC(P_AddWeaponMaybeSwitchI, TWO_ARGS)
DEFINE_VOID_CFUNC(P_CheckWeaponI, ONE_ARG)
DEFINE_RET_CFUNC(A_ShootWithZvel, THREE_ARGS)
DEFINE_RET_CFUNC(A_Spawn, TWO_ARGS)
DEFINE_VOID_CFUNC(VM_FallSprite, ONE_ARG)
DEFINE_RET_CFUNC(VM_ResetPlayer2, TWO_ARGS)
DEFINE_VOID_CFUNC(A_RadiusDamage, LARG(1), LARG(2), LARG(3), LARG(4), LARG(5), LARG(6))
DEFINE_VOID_CFUNC(G_OperateSectors, TWO_ARGS)
DEFINE_VOID_CFUNC(G_OperateActivators, TWO_ARGS)
DEFINE_RET_CFUNC(A_InsertSprite, LARG(1), LARG(2), LARG(3), LARG(4), LARG(5), LARG(6),
                 LARG(7), LARG(8), LARG(9), LARG(10), LARG(11), LARG(12), LARG(13))
DEFINE_VOID_CFUNC(A_AddToDeleteQueue, ONE_ARG)
DEFINE_RET_CFUNC(A_PlaySound, TWO_ARGS)
DEFINE_VOID_CFUNC(A_DeleteSprite, ONE_ARG)
DEFINE_VOID_CFUNC(G_ShowViewXYZ, LARG(1), LARG(2), LARG(3), LARG(4), LARG(5), LARG(6),
                  LARG(7), LARG(8), LARG(9), LARG(10), LARG(11))

#define CFUNC_REG(Name) { #Name, Name##_CF }

static struct { const char *name; lua_CFunction func; } cfuncs[] =
{
    CFUNC_REG(P_AddWeaponMaybeSwitchI),
    CFUNC_REG(P_CheckWeaponI),
    CFUNC_REG(A_ShootWithZvel),
    CFUNC_REG(A_Spawn),
    CFUNC_REG(VM_FallSprite),
    CFUNC_REG(VM_ResetPlayer2),
    CFUNC_REG(A_RadiusDamage),
    CFUNC_REG(G_OperateSectors),
    CFUNC_REG(G_OperateActivators),
    CFUNC_REG(A_InsertSprite),
    CFUNC_REG(A_Spawn),
    CFUNC_REG(A_AddToDeleteQueue),
    CFUNC_REG(A_PlaySound),
    CFUNC_REG(A_DeleteSprite),
    CFUNC_REG(G_ShowViewXYZ),
};

// Creates a global table "CF" containing the functions from cfuncs[].
static void El_PushCFunctions(lua_State *L)
{
    int32_t i;

    lua_newtable(L);

    for (i=0; i<(signed)sizeof(cfuncs)/(signed)sizeof(cfuncs[0]); i++)
    {
        lua_pushstring(L, cfuncs[i].name);
        lua_pushcfunction(L, cfuncs[i].func);
        lua_settable(L, -3);
    }

    lua_setglobal(L, "CF");
}

//////

LUNATIC_CB int32_t (*El_RestoreGamevars)(const char *savecode);

static void El_StateSetup(lua_State *L)
{
    luaopen_lpeg(L);
    lua_pop(L, lua_gettop(L));  // pop off whatever lpeg leaves on the stack

    // create misc. global functions in the Lua state
    lua_pushcfunction(L, SetEvent_CF);
    lua_setglobal(L, "gameevent_internal");
    lua_pushcfunction(L, SetActor_CF);
    lua_setglobal(L, "gameactor_internal");
    lua_pushcfunction(L, SetTweakTracebackMsg_CF);
    lua_setglobal(L, "set_tweak_traceback_internal");

    El_PushCFunctions(L);

    Bassert(lua_gettop(L)==0);

    // This is for engine-side Lua:
    lua_pushcfunction(L, &our_traceback_CF);
}

static void El_OnOutOfMem(void)
{
    G_GameExit("Out of memory in Lunatic.");
}

// 0: success, <0: failure
int32_t El_CreateState(L_State *estate, const char *name)
{
    int32_t i;

    if (!g_timingInited)
    {
        g_timingInited = 1;
        for (i=0; i<MAXTILES; i++)
            g_actorMinMs[i] = 1e308;
    }

    L_ErrorFunc = El_OnError;
    L_OutOfMemFunc = El_OnOutOfMem;

    return L_CreateState(estate, name, &El_StateSetup);
}

void El_DestroyState(L_State *estate)
{
    L_DestroyState(estate);

    g_tweakTracebackMsg = 0;

    // XXX: It would be cleaner to also clear stuff like g_elEvents[], but
    // currently, when the game Lua state is recreated, the array should have
    // the same values as before, so we're skipping that for now.
}


////////// Lua_CFunctions //////////

// gameevent(EVENT_..., lua_function)
static int32_t SetEvent_CF(lua_State *L)
{
    int32_t eventidx;

    Bassert(lua_gettop(L) == 2);
    eventidx = luaL_checkint(L, 1);
    Bassert((unsigned)eventidx < MAXEVENTS);

    L_CheckAndRegisterFunction(L, &g_elEvents[eventidx]);
    g_elEvents[eventidx] = 1;

    return 0;
}

// gameactor(actortile, strength, act, mov, movflags, lua_function)
static int32_t SetActor_CF(lua_State *L)
{
    int32_t actortile;
    el_actor_t *a;

    Bassert(lua_gettop(L) == 6);

    actortile = luaL_checkint(L, 1);
    Bassert((unsigned)actortile < MAXTILES);

    a = &g_elActors[actortile];
    L_CheckAndRegisterFunction(L, a);

    // Set actor properties. They can only be nil if we're chaining actor code.

    if (!lua_isnil(L, 2))
        a->strength = luaL_checkint(L, 2);
    if (!lua_isnil(L, 5))
        a->movflags = luaL_checkint(L, 5);

    if (!lua_isnil(L, 3))
        Bmemcpy(&a->act, lua_topointer(L, 3), sizeof(con_action_t));
    if (!lua_isnil(L, 4))
        Bmemcpy(&a->mov, lua_topointer(L, 4), sizeof(con_move_t));

    a->haveit = 1;

    return 0;
}

//////////////////////////////

static int32_t call_regd_function3(lua_State *L, void *keyaddr,
                                   int32_t spriteNum, int32_t playerNum, int32_t lDist)
{
#if !defined NDEBUG
    const int32_t top = lua_gettop(L);
#endif
    lua_pushcfunction(L, &our_traceback_CF);

    // get the Lua function from the registry
    lua_pushlightuserdata(L, keyaddr);
    lua_gettable(L, LUA_REGISTRYINDEX);

    lua_pushinteger(L, spriteNum);
    lua_pushinteger(L, playerNum);
    lua_pushinteger(L, lDist);

    // -- call it! --
    {
        const int32_t i = lua_pcall(L, 3, 0, -5);
        const int32_t haveerr = (i != 0);

        Bassert(lua_iscfunction(L, -1-haveerr));
        lua_remove(L, -1-haveerr);

        Bassert(lua_gettop(L) == top+haveerr);

        return i;
    }
}

static int32_t g_eventIdx = 0;
static void El_EventErrorPrint(const char *errmsg)
{
    OSD_Printf(OSD_ERROR "event \"%s\" runtime error: %s\n",
               EventNames[g_eventIdx], errmsg);
}

int32_t El_CallEvent(L_State *estate, int32_t eventidx, int32_t spriteNum, int32_t playerNum, int32_t lDist, int32_t *iReturn)
{
    // XXX: estate must be the one where the events were registered...
    //      make a global?

    lua_State *const L = estate->L;
    int32_t i;

    const int32_t o_RETURN = g_RETURN;
    g_RETURN = *iReturn;

    g_elCallDepth++;
    i = call_regd_function3(L, &g_elEvents[eventidx], spriteNum, playerNum, lDist);
    g_elCallDepth--;

    *iReturn = g_RETURN;
    g_RETURN = o_RETURN;

    if (i != 0)
    {
        g_elEventError = 1;
        g_eventIdx = eventidx;
        return L_HandleError(L, i, &El_EventErrorPrint);
    }

    return 0;
}

static int32_t g_actorTile, g_iActor;
static void El_ActorErrorPrint(const char *errmsg)
{
    OSD_Printf(OSD_ERROR "actor %d (sprite %d) runtime error: %s\n",
               g_actorTile, g_iActor, errmsg);
}

int32_t El_CallActor(L_State *estate, int32_t actortile, int32_t spriteNum, int32_t playerNum, int32_t lDist)
{
    lua_State *const L = estate->L;
    int32_t i;

    g_elCallDepth++;
    i = call_regd_function3(L, &g_elActors[actortile], spriteNum, playerNum, lDist);
    g_elCallDepth--;

    if (i != 0)
    {
        g_actorTile = actortile;
        g_iActor = spriteNum;
        return L_HandleError(L, i, &El_ActorErrorPrint);
    }

    return 0;
}
