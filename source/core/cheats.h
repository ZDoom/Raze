#pragma once

#include <stdint.h>
#include "c_cvars.h"
#include "d_protocol.h"

EXTERN_CVAR(Bool, sv_cheats)

void genericCheat(int player, TArrayView<uint8_t>& stream, bool skip);
void changeMap(int player, TArrayView<uint8_t>& stream, bool skip);
void endScreenJob(int player, TArrayView<uint8_t>& stream, bool skip);
void startSaveGame(int player, TArrayView<uint8_t>& stream, bool skip);
