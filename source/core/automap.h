#pragma once

#include "tarray.h"
#include "build.h"
#include "c_cvars.h"

class FSerializer;
struct event_t;

extern bool automapping;
extern bool gFullMap;
extern FixedBitArray<MAXSECTORS> show2dsector;
extern FixedBitArray<MAXWALLS> show2dwall;
extern FixedBitArray<MAXSPRITES> show2dsprite;

void SerializeAutomap(FSerializer& arc);
void ClearAutomap();
void MarkSectorSeen(int sect);
void DrawOverheadMap(int x, int y, int ang);
bool AM_Responder(event_t* ev, bool last);

enum AM_Mode
{
	am_off,
	am_overlay,
	am_full,
	am_count
};
extern int automapMode;

EXTERN_CVAR(Bool, am_followplayer)
