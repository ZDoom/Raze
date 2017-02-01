/* The Lunatic Interpreter, part of EDuke32. Editor stuff. */

#ifdef USE_LUAJIT_2_1
# include <luajit-2.1/lualib.h>
#else
# include <luajit-2.0/lualib.h>
#endif

#include "lunatic_editor.h"


int Em_CreateState(L_State *estate)
{
    return L_CreateState(estate, "m32", NULL);
}

void Em_DestroyState(L_State *estate)
{
    L_DestroyState(estate);
}
