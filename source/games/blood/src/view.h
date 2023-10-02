//-------------------------------------------------------------------------
/*
Copyright (C) 2010-2019 EDuke32 developers and contributors
Copyright (C) 2019 Nuke.YKT

This file is part of NBlood.

NBlood is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License version 2
as published by the Free Software Foundation.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/
//-------------------------------------------------------------------------
#pragma once
#include "build.h"
#include "palette.h"
#include "common_game.h"
#include "messages.h"
#include "player.h"
#include "interpolate.h"
#include "bloodactor.h"

BEGIN_BLD_NS

extern bool gPrediction;

enum VIEW_EFFECT {
	kViewEffectShadow = 0,
	kViewEffectFlareHalo,
	kViewEffectCeilGlow,
	kViewEffectFloorGlow,
	kViewEffectTorchHigh,
	kViewEffectTorchLow,
	kViewEffectSmokeHigh,
	kViewEffectSmokeLow,
	kViewEffectFlame,
	kViewEffectSpear,
	kViewEffectTrail,
	kViewEffectPhase,
	kViewEffectShowWeapon,
	kViewEffectReflectiveBall,
	kViewEffectShoot,
	kViewEffectTesla,
	kViewEffectFlag,
	kViewEffectBigFlag,
	kViewEffectAtom,
	kViewEffectSpotProgress,
};

enum VIEWPOS {
	viewFirstPerson = 0,
	viewThirdPerson
};

enum { kFontNum = 5 };

extern FFont* gFont[kFontNum];
extern VIEWPOS gViewPos;
extern int gViewIndex;
extern int deliriumTilt, deliriumPitch;
extern int deliriumPitchO;
extern DAngle deliriumTurnO, deliriumTurn;
extern DAngle gScreenTiltO, gScreenTilt;
extern int gShowFrameRate;
extern int gLastPal;

void hudDraw(DBloodPlayer* pPlayer, sectortype* pSector, double bobx, double boby, double zDelta, DAngle angle, int basepal, double interpfrac);
void viewInitializePrediction(void);
void viewUpdatePrediction(InputPacket* pInput);
void viewCorrectPrediction(void);
void viewBackupView(int nPlayer);
void InitStatusBar(void);
void UpdateStatusBar(DBloodPlayer* pPlayer);
void viewInit(void);
void viewprocessSprites(tspriteArray& tsprites, int32_t cX, int32_t cY, int32_t cZ, int32_t cA, int32_t smooth);
void viewSetMessage(const char* pMessage, const char* color = nullptr, const MESSAGE_PRIORITY priority = MESSAGE_PRIORITY_NORMAL);


void viewSetErrorMessage(const char* pMessage);
void DoLensEffect(void);
void UpdateDacs(int nPalette, bool bNoTint = false);
void viewDrawScreen(bool sceneonly = false);
void viewUpdateDelirium(DBloodPlayer* pPlayer);
void viewSetSystemMessage(const char* pMessage, ...);

inline void viewInterpolateSector(sectortype* pSector)
{
	StartInterpolation(pSector, Interp_Sect_Floorz);
	StartInterpolation(pSector, Interp_Sect_Ceilingz);
	StartInterpolation(pSector, Interp_Sect_Floorheinum);
}

inline void viewInterpolateWall(walltype* pWall)
{
	StartInterpolation(pWall, Interp_Wall_X);
	StartInterpolation(pWall, Interp_Wall_Y);
}

inline void viewBackupSpriteLoc(DBloodActor* actor)
{
	if (!actor->interpolated)
	{
		actor->backuploc();
		actor->interpolated = true;
	}
}


END_BLD_NS
