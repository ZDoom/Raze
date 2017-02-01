/* The Lunatic Interpreter, part of EDuke32. Editor stuff. */

#ifndef EDUKE32_LUNATIC_M32_H_
#define EDUKE32_LUNATIC_M32_H_

#include "lunatic.h"

#ifdef __cplusplus
extern "C" {
#endif

int Em_CreateState(L_State *estate);
void Em_DestroyState(L_State *estate);

#ifdef __cplusplus
}
#endif

#endif
