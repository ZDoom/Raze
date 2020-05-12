#pragma once

struct sprite_type;
BEGIN_DUKE_NS

#define cmd(a) concmd_ ## a,
#define cmdx(a, b) concmd_ ## a,
#define cmda(a,b)

enum EConCommands
{
#include "condef.h"
};

#undef cmd
#undef cmdx
#undef cmda

END_DUKE_NS
