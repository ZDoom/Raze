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
void DrawOverheadMap(const DVector2& plxy, const DAngle pl_angle, double const interpfrac);
bool AM_Responder(event_t* ev, bool last);
void drawlinergb(const DVector2& v1, const DVector2& v2, PalEntry p);
void DrawAutomapAlignmentFacing(const spritetype& spr, const DVector2& bpos, const DVector2& cangvect, const double czoom, const DVector2& xydim, const PalEntry& col);
void DrawAutomapAlignmentWall(const spritetype& spr, const DVector2& bpos, const DVector2& cangvect, const double czoom, const DVector2& xydim, const PalEntry& col);
void DrawAutomapAlignmentFloor(const spritetype& spr, const DVector2& bpos, const DVector2& cangvect, const double czoom, const DVector2& xydim, const PalEntry& col);

inline DVector2 OutAutomapVector(const DVector2& pos, const DVector2& angvect, const double zoom = 1., const DVector2& xydim = { 0, 0 })
{
	return -pos.Rotated(angvect.Y, angvect.X) * zoom + xydim;
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
