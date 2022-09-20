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
void DrawOverheadMap(const DVector2& plxy, const DAngle pl_angle, double const smoothratio);
bool AM_Responder(event_t* ev, bool last);
void drawlinergb(const double x1, const double y1, const double x2, const double y2, PalEntry p);

inline DVector2 OutAutomapVector(const DVector2& pos, const double sine, const double cosine, const double zoom = 1., const DVector2& xydim = { 0, 0 })
{
	return pos.Rotated(cosine, sine).Rotated90CW() * zoom + xydim;
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
