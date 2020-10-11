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

BEGIN_BLD_NS

struct VIEW {
    int TotalKills;
    int Kills;
    int at8; // bob height
    int atc; // bob width
    int at10;
    int at14;
    int at18; // bob sway y
    int at1c; // bob sway x
    fixedhoriz at24; // horiz
    fixedhoriz at28; // horizoff
    int at2c;
    binangle at30; // angle
    int at34; // weapon z
    int at38; // view z
    int at3c;
    int at40;
    int at44;
    int at48; // posture
    lookangle at4c; // spin
    int at50; // x
    int at54; // y
    int at58; // z
    int at5c; //xvel
    int at60; //yvel
    int at64; //zvel
    short at68; // sectnum
    unsigned int at6a; // floordist
    char at6e; // look center
    char at6f;
    char at70; // run
    char at71; // jump
    char at72; // underwater
    short at73; // sprite flags
    SPRITEHIT at75;
    lookangle look_ang;
    lookangle rotscrnang;
};

extern VIEW gPrevView[kMaxPlayers];

extern VIEW predict, predictOld;
extern bool gPrediction;

enum VIEW_EFFECT {
    VIEW_EFFECT_0 = 0,
    VIEW_EFFECT_1,
    VIEW_EFFECT_2,
    VIEW_EFFECT_3,
    VIEW_EFFECT_4,
    VIEW_EFFECT_5,
    VIEW_EFFECT_6,
    VIEW_EFFECT_7,
    VIEW_EFFECT_8,
    VIEW_EFFECT_9,
    VIEW_EFFECT_10,
    VIEW_EFFECT_11,
    VIEW_EFFECT_12,
    VIEW_EFFECT_13,
    VIEW_EFFECT_14,
    VIEW_EFFECT_15,
    VIEW_EFFECT_16,
    VIEW_EFFECT_17,
    VIEW_EFFECT_18,
};

enum VIEWPOS {
    VIEWPOS_0 = 0,
    VIEWPOS_1
};

enum INTERPOLATE_TYPE {
    INTERPOLATE_TYPE_INT = 0,
    INTERPOLATE_TYPE_SHORT,
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
extern char gInterpolateSprite[];
extern char gInterpolateWall[];
extern char gInterpolateSector[];
extern LOCATION gPrevSpriteLoc[kMaxSprites];
extern int gLastPal;
extern double gInterpolate;

void hudDraw(PLAYER* gView, VIEW *pView, int nSectnum, double bobx, double boby, double zDelta, int basepal, double smoothratio);
void viewInitializePrediction(void);
void viewUpdatePrediction(InputPacket *pInput);
void viewCorrectPrediction(void);
void viewBackupView(int nPlayer);
void viewCorrectViewOffsets(int nPlayer, vec3_t const *oldpos);
void viewClearInterpolations(void);
void viewAddInterpolation(void *data, INTERPOLATE_TYPE type);
void CalcInterpolations(void);
void RestoreInterpolations(void);
void viewDrawText(int nFont, const char *pString, int x, int y, int nShade, int nPalette, int position, char shadow, unsigned int nStat = 0, uint8_t alpha = 255);
void InitStatusBar(void);
void UpdateStatusBar();
void viewInit(void);
void viewProcessSprites(int32_t cX, int32_t cY, int32_t cZ, int32_t cA, int32_t smooth);
void viewSetMessage(const char *pMessage, const int pal = 0, const MESSAGE_PRIORITY priority = MESSAGE_PRIORITY_NORMAL);


void viewSetErrorMessage(const char *pMessage);
void DoLensEffect(void);
void UpdateDacs(int nPalette, bool bNoTint = false);
void viewDrawScreen(bool sceneonly = false);
void viewUpdateDelirium(void);
void viewUpdateShake(void);
void viewSetSystemMessage(const char* pMessage, ...);

inline void viewInterpolateSector(int nSector, sectortype *pSector)
{
    if (!TestBitString(gInterpolateSector, nSector))
    {
        viewAddInterpolation(&pSector->floorz, INTERPOLATE_TYPE_INT);
        viewAddInterpolation(&pSector->ceilingz, INTERPOLATE_TYPE_INT);
        viewAddInterpolation(&pSector->floorheinum, INTERPOLATE_TYPE_SHORT);
        SetBitString(gInterpolateSector, nSector);
    }
}

inline void viewInterpolateWall(int nWall, walltype *pWall)
{
    if (!TestBitString(gInterpolateWall, nWall))
    {
        viewAddInterpolation(&pWall->x, INTERPOLATE_TYPE_INT);
        viewAddInterpolation(&pWall->y, INTERPOLATE_TYPE_INT);
        SetBitString(gInterpolateWall, nWall);
    }
}

inline void viewBackupSpriteLoc(int nSprite, spritetype *pSprite)
{
    if (!TestBitString(gInterpolateSprite, nSprite))
    {
        LOCATION *pPrevLoc = &gPrevSpriteLoc[nSprite];
        pPrevLoc->x = pSprite->x;
        pPrevLoc->y = pSprite->y;
        pPrevLoc->z = pSprite->z;
        pPrevLoc->ang = pSprite->ang;
        SetBitString(gInterpolateSprite, nSprite);
    }
}

END_BLD_NS
