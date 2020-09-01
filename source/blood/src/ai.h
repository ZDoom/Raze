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
#include "common_game.h"
#include "actor.h"
#include "db.h"

BEGIN_BLD_NS


struct AISTATE {
    int stateType; // By NoOne: current type of state. Basically required for kModernDudeTargetChanger, but can be used for something else.
    int seqId;
    int funcId; // seq callback
    int stateTicks;
    void(*enterFunc)(spritetype *, XSPRITE *);
    void(*moveFunc)(spritetype *, XSPRITE *);
    void(*thinkFunc)(spritetype *, XSPRITE *);
    AISTATE *nextState;
};
extern AISTATE aiState[];
extern AISTATE genIdle;
extern AISTATE genRecoil;

enum AI_SFX_PRIORITY {
    AI_SFX_PRIORITY_0 = 0,
    AI_SFX_PRIORITY_1,
    AI_SFX_PRIORITY_2,
};


struct DUDEEXTRA_at6_u1
{
    int TotalKills;
    int Kills;
    char at8;
};

struct DUDEEXTRA_at6_u2
{
    int TotalKills;
    char Kills;
};

struct DUDEEXTRA
{
    int TotalKills;
    char Kills;
    AI_SFX_PRIORITY at5;
    union
    {
        DUDEEXTRA_at6_u1 u1;
        DUDEEXTRA_at6_u2 u2;
    } at6;
    //DUDEEXTRA_at6 at6;
};

struct TARGETTRACK {
    int TotalKills;
    int Kills;
    int at8; // view angle
    int atc;
    int at10; // Move predict
};

extern const int dword_138BB0[5];
extern DUDEEXTRA gDudeExtra[];
extern int gDudeSlope[];
extern int cumulDamage[];

bool sub_5BDA8(spritetype *pSprite, int nSeq);
void aiPlay3DSound(spritetype *pSprite, int a2, AI_SFX_PRIORITY a3, int a4);
void aiNewState(spritetype *pSprite, XSPRITE *pXSprite, AISTATE *pAIState);
void aiChooseDirection(spritetype *pSprite, XSPRITE *pXSprite, int a3);
void aiMoveForward(spritetype *pSprite, XSPRITE *pXSprite);
void aiMoveTurn(spritetype *pSprite, XSPRITE *pXSprite);
void aiMoveDodge(spritetype *pSprite, XSPRITE *pXSprite);
void aiActivateDude(spritetype *pSprite, XSPRITE *pXSprite);
void aiSetTarget(XSPRITE *pXSprite, int x, int y, int z);
void aiSetTarget(XSPRITE *pXSprite, int nTarget);
int aiDamageSprite(spritetype *pSprite, XSPRITE *pXSprite, int nSource, DAMAGE_TYPE nDmgType, int nDamage);
void aiThinkTarget(spritetype *pSprite, XSPRITE *pXSprite);
void sub_5F15C(spritetype *pSprite, XSPRITE *pXSprite);
void aiProcessDudes(void);
void aiInit(void);
void aiInitSprite(spritetype *pSprite);
bool CanMove(spritetype* pSprite, int a2, int nAngle, int nRange);
void RecoilDude(spritetype* pSprite, XSPRITE* pXSprite);
END_BLD_NS
