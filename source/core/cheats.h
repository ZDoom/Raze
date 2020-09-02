#pragma once

#include <stdint.h>
#include "c_cvars.h"
#include "d_protocol.h"

EXTERN_CVAR(Bool, sv_cheats)

void genericCheat(int player, uint8_t** stream, bool skip);
