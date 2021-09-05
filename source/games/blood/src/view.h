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
#include "compat.h"
#include "build.h"
#include "palette.h"
#include "common_game.h"
#include "messages.h"
#include "player.h"
#include "interpolate.h"
#include "bloodactor.h"

BEGIN_BLD_NS

struct VIEW {
    int bobPhase;
    int Kills;
    int bobHeight; // bob height
    int bobWidth; // bob width
    int at10;
    int at14;
    int shakeBobY; // bob sway y
    int shakeBobX; // bob sway x
    fixedhoriz horiz; // horiz
    fixedhoriz horizoff; // horizoff
    int at2c;
    binangle angle; // angle
    int weaponZ; // weapon z
    int viewz; // view z
    int at3c;
    int at40;
    int at44;
    int at48; // posture
    double spin; // spin
    union {
        struct
        {
            int32_t x, y, z;
        };
        vec3_t pos;
    };
    int xvel; //xvel
    int yvel; //yvel
    int zvel; //zvel
    int sectnum; // sectnum
    unsigned int floordist; // floordist
    char at6e; // look center
    char at6f;
    char at70; // run
    char at71; // jump
    char at72; // underwater
    short at73; // sprite flags
    SPRITEHIT at75;
    binangle look_ang;
    binangle rotscrnang;
};

extern VIEW gPrevView[kMaxPlayers];

extern VIEW predict, predictOld;
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
    VIEWPOS_0 = 0,
    VIEWPOS_1
};

enum
{
    kBackTile = 253,

    kCrosshairTile = 2319,
    kLoadScreen = 2049,
    kLoadScreenWideBack = 9216,
    kLoadScreenWideLeft = 9217,
    kLoadScreenWideRight = 9218,
    kLoadScreenWideMiddle = 9219,

    kSBarNumberHealth = 9220,
    kSBarNumberAmmo = 9230,
    kSBarNumberInv = 9240,
    kSBarNumberArmor1 = 9250,
    kSBarNumberArmor2 = 9260,
    kSBarNumberArmor3 = 9270,
};

enum { kFontNum = 5 };

extern FFont *gFont[kFontNum];
extern VIEWPOS gViewPos;
extern int gViewIndex;
extern int gScreenTilt;
extern int deliriumTilt, deliriumTurn, deliriumPitch;
extern int gScreenTiltO, deliriumTurnO, deliriumPitchO;
extern int gShowFrameRate;
extern int gLastPal;
extern double gInterpolate;

void hudDraw(PLAYER* gView, int nSectnum, double bobx, double boby, double zDelta, int basepal, double smoothratio);
void viewInitializePrediction(void);
void viewUpdatePrediction(InputPacket *pInput);
void viewCorrectPrediction(void);
void viewBackupView(int nPlayer);
void viewCorrectViewOffsets(int nPlayer, vec3_t const *oldpos);
void InitStatusBar(void);
void UpdateStatusBar();
void viewInit(void);
void viewProcessSprites(spritetype* tsprite, int& spritesortcnt, int32_t cX, int32_t cY, int32_t cZ, int32_t cA, int32_t smooth);
void viewSetMessage(const char *pMessage, const int pal = 0, const MESSAGE_PRIORITY priority = MESSAGE_PRIORITY_NORMAL);


void viewSetErrorMessage(const char *pMessage);
void DoLensEffect(void);
void UpdateDacs(int nPalette, bool bNoTint = false);
void viewDrawScreen(bool sceneonly = false);
void viewUpdateDelirium(void);
void viewSetSystemMessage(const char* pMessage, ...);

inline void viewInterpolateSector(int nSector, sectortype *pSector)
{
    StartInterpolation(nSector, Interp_Sect_Floorz);
    StartInterpolation(nSector, Interp_Sect_Ceilingz);
    StartInterpolation(nSector, Interp_Sect_Floorheinum);
}

inline void viewInterpolateWall(int nWall, walltype *pWall)
{
    StartInterpolation(nWall, Interp_Wall_X);
    StartInterpolation(nWall, Interp_Wall_Y);
}

inline void viewBackupSpriteLoc(DBloodActor* actor)
{
    if (!actor->interpolated)
    {
        actor->s().backuploc();
        actor->interpolated = true;
    }
}


END_BLD_NS
