/* The Lunatic Interpreter, part of EDuke32 */

#ifndef EDUKE32_LUNATIC_H_
#define EDUKE32_LUNATIC_H_

#include <lua.h>

typedef struct
{
    const char *name;
    lua_State *L;
} El_State;


// -- functions --
int32_t El_CreateState(El_State *estate, const char *name);
void El_DestroyState(El_State *estate);
static inline int32_t El_IsInitialized(const El_State *estate) { return (estate->L != NULL); }
int32_t El_RunOnce(El_State *estate, const char *fn);
int32_t El_CallEvent(El_State *estate, int32_t eventidx);

#endif
