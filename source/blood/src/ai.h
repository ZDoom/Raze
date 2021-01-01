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
    void(*enterFunc)(DBloodActor *);
    void(*moveFunc)(DBloodActor *);
    void(*thinkFunc)(DBloodActor *);
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
    int xval1;
    int xval2;
    char xval3;
};

struct DUDEEXTRA_at6_u2
{
    int xval1;
    char xval2;
};

struct DUDEEXTRA
{
    int time;
    int recoil;
    int prio;
    // Broken by design... :?
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

bool dudeIsPlayingSeq(spritetype *pSprite, int nSeq);
void aiPlay3DSound(spritetype *pSprite, int a2, AI_SFX_PRIORITY a3, int a4);
void aiNewState(DBloodActor* actor, AISTATE *pAIState);
void aiChooseDirection(spritetype *pSprite, XSPRITE *pXSprite, int a3);
void aiMoveForward(DBloodActor*pXSprite);
void aiMoveTurn(DBloodActor*pXSprite);
void aiMoveDodge(DBloodActor *actor);
void aiActivateDude(DBloodActor *actor);
void aiSetTarget(XSPRITE *pXSprite, int x, int y, int z);
void aiSetTarget(XSPRITE *pXSprite, int nTarget);
int aiDamageSprite(spritetype *pSprite, XSPRITE *pXSprite, int nSource, DAMAGE_TYPE nDmgType, int nDamage);
void aiThinkTarget(DBloodActor* actor);
void sub_5F15C(spritetype *pSprite, XSPRITE *pXSprite);
void aiProcessDudes(void);
void aiInit(void);
void aiInitSprite(spritetype *pSprite);
bool CanMove(spritetype* pSprite, int a2, int nAngle, int nRange);
END_BLD_NS
