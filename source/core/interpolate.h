#pragma once


enum EInterpolationType
{
	Interp_Sect_Floorz,
	Interp_Sect_Ceilingz,
	Interp_Sect_Floorheinum,
	Interp_Sect_Ceilingheinum,
	Interp_Sect_FloorPanX,
	Interp_Sect_FloorPanY,	
	Interp_Sect_CeilingPanX,
	Interp_Sect_CeilingPanY,
	
	Interp_Wall_X,
	Interp_Wall_Y,
	Interp_Wall_PanX,
	Interp_Wall_PanY,
	
	Interp_Sprite_Z,
};

void StartInterpolation(int index, int type);
void StopInterpolation(int index, int type);
void UpdateInterpolations();
void ClearInterpolations();
void DoInterpolations(double smoothratio);
void RestoreInterpolations();
void SerializeInterpolations(FSerializer& arc);
