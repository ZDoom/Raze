#ifndef EDUKE32_LUNATIC_M32_H_
#define EDUKE32_LUNATIC_M32_H_

#include <lua.h>


typedef lua_State Em_State;


Em_State *Em_CreateState(void);

static inline void Em_DestroyState(Em_State *L)
{
    lua_close(L);
}

int32_t Em_RunOnce(Em_State *L, const char *fn);
int32_t Em_RunStringOnce(Em_State *L, const char *str);

#endif
