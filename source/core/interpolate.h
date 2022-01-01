#pragma once

#include "gamefuncs.h"

enum EInterpolationType
{
	Interp_Invalid = -1,
	Interp_Sect_Floorz = 0,
	Interp_Sect_Ceilingz,
	Interp_Sect_Floorheinum,
	Interp_Sect_Ceilingheinum,

	Interp_Wall_X,
	Interp_Wall_Y,

	Interp_Sprite_Z,

	Interp_Pan_First,
	// order of the following 4 flags must match the corresponding sector flags.
	Interp_Sect_FloorPanX = Interp_Pan_First,
	Interp_Sect_FloorPanY,
	Interp_Sect_CeilingPanX,
	Interp_Sect_CeilingPanY,
	Interp_Wall_PanX,
	Interp_Wall_PanY,
};

void StartInterpolation(int index, int type);
void StopInterpolation(int index, int type);
void StartInterpolation(DCoreActor* actor, int type);
void StopInterpolation(DCoreActor* actor, int type);
void UpdateInterpolations();
void ClearInterpolations();
void ClearMovementInterpolations();
void DoInterpolations(double smoothratio);
void RestoreInterpolations();
void SerializeInterpolations(FSerializer& arc);
void clearsectinterpolate(sectortype* sectnum);
void setsectinterpolate(sectortype* sectnum);

inline void StartInterpolation(walltype* wall, int type)
{
	assert(type == Interp_Wall_X || type == Interp_Wall_Y || type == Interp_Wall_PanX || type == Interp_Wall_PanY);
	return StartInterpolation(wallnum(wall), type);
}

inline void StartInterpolation(sectortype* sect, int type)
{
	assert(type == Interp_Sect_Floorz || type == Interp_Sect_Ceilingz || type == Interp_Sect_Floorheinum || type == Interp_Sect_Ceilingheinum ||
		type == Interp_Sect_FloorPanX || type == Interp_Sect_FloorPanY || type == Interp_Sect_CeilingPanX || type == Interp_Sect_CeilingPanY);
	return StartInterpolation(sectnum(sect), type);
}

inline void StopInterpolation(walltype* wall, int type)
{
	assert(type == Interp_Wall_X || type == Interp_Wall_Y || type == Interp_Wall_PanX || type == Interp_Wall_PanY);
	return StopInterpolation(wallnum(wall), type);
}

inline void StopInterpolation(sectortype* sect, int type)
{
	assert(type == Interp_Sect_Floorz || type == Interp_Sect_Ceilingz || type == Interp_Sect_Floorheinum || type == Interp_Sect_Ceilingheinum ||
		type == Interp_Sect_FloorPanX || type == Interp_Sect_FloorPanY || type == Interp_Sect_CeilingPanX || type == Interp_Sect_CeilingPanY);
	return StopInterpolation(sectnum(sect), type);
}
