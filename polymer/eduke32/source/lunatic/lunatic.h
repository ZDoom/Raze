/* The Lunatic Interpreter, part of EDuke32 */

#ifndef EDUKE32_LUNATIC_H_
#define EDUKE32_LUNATIC_H_

#include <lua.h>

#include "gamedef.h"  // EventNames[], MAXEVENTS


typedef struct
{
    char *name;
    lua_State *L;
} El_State;

extern uint8_t g_elEvents[MAXEVENTS];  // shouldn't be used directly
extern uint8_t g_elActors[MAXTILES];  // shouldn't be used directly

// -- functions --
int32_t El_CreateState(El_State *estate, const char *name);
void El_DestroyState(El_State *estate);
int32_t El_RunOnce(El_State *estate, const char *fn);
int32_t El_CallEvent(El_State *estate, int32_t eventidx, int32_t iActor, int32_t iPlayer, int32_t lDist);
int32_t El_CallActor(El_State *estate, int32_t actortile, int32_t iActor, int32_t iPlayer, int32_t lDist);

static inline int32_t El_IsInitialized(const El_State *estate) { return (estate->L != NULL); }
static inline int32_t El_HaveEvent(int32_t eventidx) { return g_elEvents[eventidx]!=0; }
static inline int32_t El_HaveActor(int32_t actortile) { return g_elActors[actortile]!=0; }

#endif
