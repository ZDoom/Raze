#pragma once

#include "gamefuncs.h"

enum EInterpolationType
{
	Interp_Sect_Floorz,
	Interp_Sect_Ceilingz,
	Interp_Sect_Floorheinum,
	Interp_Sect_Ceilingheinum,
	
	Interp_Wall_X,
	Interp_Wall_Y,
	
	Interp_Sprite_Z,

	Interp_Pan_First,
	Interp_Sect_FloorPanX = Interp_Pan_First,
	Interp_Sect_FloorPanY,
	Interp_Sect_CeilingPanX,
	Interp_Sect_CeilingPanY,
	Interp_Wall_PanX,
	Interp_Wall_PanY,
};

void StartInterpolation(int index, int type);
void StopInterpolation(int index, int type);
void UpdateInterpolations();
void ClearInterpolations();
void ClearMovementInterpolations();
void DoInterpolations(double smoothratio);
void RestoreInterpolations();
void SerializeInterpolations(FSerializer& arc);
void clearsectinterpolate(int sectnum);
void setsectinterpolate(int sectnum);

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
