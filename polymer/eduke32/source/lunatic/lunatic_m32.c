/* The Lunatic Interpreter, part of EDuke32. Editor stuff. */

#include <lualib.h>

#include "lunatic_m32.h"


int Em_CreateState(L_State *estate)
{
    return L_CreateState(estate, "m32", NULL);
}

void Em_DestroyState(L_State *estate)
{
    L_DestroyState(estate);
}
