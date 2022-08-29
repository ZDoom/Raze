#pragma once

#include "tarray.h"
#include "build.h"
#include "c_cvars.h"
#include "palentry.h"

class FSerializer;
struct event_t;

extern bool automapping;
extern bool gFullMap;
extern BitArray show2dsector;
extern BitArray show2dwall;

void SerializeAutomap(FSerializer& arc);
void ClearAutomap();
void MarkSectorSeen(sectortype* sect);
void DrawOverheadMap(int pl_x, int pl_y, const DAngle pl_angle, double const smoothratio);
bool AM_Responder(event_t* ev, bool last);
void drawlinergb(const double x1, const double y1, const double x2, const double y2, PalEntry p);

inline void drawlinergb(int32_t x1, int32_t y1, int32_t x2, int32_t y2, PalEntry p)
{
	drawlinergb(x1 / 4096., y1 / 4096., x2 / 4096., y2 / 4096., p);
}

enum AM_Mode
{
	am_off,
	am_overlay,
	am_full,
	am_count
};
extern int automapMode;

EXTERN_CVAR(Bool, am_followplayer)
