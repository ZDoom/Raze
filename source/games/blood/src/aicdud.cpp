//-------------------------------------------------------------------------
/*
Copyright (C) 2010-2019 EDuke32 developers and contributors
Copyright (C) 2019 Nuke.YKT
Copyright (C) NoOne

*****************************************************************
NoOne: AI code for Custom Dude system.
*****************************************************************

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
#ifdef NOONE_EXTENSIONS
#include "ai.h"
#include "nnexts.h"
#include "globals.h"
#include "player.h"
#include "endgame.h"
#include "view.h"
//#include "aicdud.h"

BEGIN_BLD_NS

#define SEQOFFS(x) (kCdudeDefaultSeq + x)

#pragma pack(push, 1)
struct TARGET_INFO
{
    DBloodActor* pSpr;
    unsigned int nDist;
    DAngle nAng;
    DAngle nDang;
    int nCode;
};
#pragma pack(pop)

void resetTarget(DBloodActor* pXSpr)            { pSpr->xspr.xspr.target = nullptr; }
void moveStop(DBloodActor* pSpr)                { pSpr->vel.XY().Zero(); }
static char THINK_CLOCK(int nSpr, int nClock = 3)               { return ((gFrameCount & nClock) == (nSpr & nClock)); }
static int qsSortTargets(TARGET_INFO* ref1, TARGET_INFO* ref2)  { return ref1->nDist > ref2->nDist? 1 : ref1->nDist < ref2->nDist? -1 : 0; }

// This set of functions needs to be exported for scripting later to allow extension of this list.
static DBloodActor* weaponShotDummy(CUSTOMDUDE*, CUSTOMDUDE_WEAPON*, DVector3& offs, DVector3& vel) { return nullptr; }
static DBloodActor* weaponShotHitscan(CUSTOMDUDE* pDude, CUSTOMDUDE_WEAPON* pWeap, DVector3& offs, DVector3& vel);
static DBloodActor* weaponShotMissile(CUSTOMDUDE* pDude, CUSTOMDUDE_WEAPON* pWeap, DVector3& offs, DVector3& vel);
static DBloodActor* weaponShotThing(CUSTOMDUDE* pDude, CUSTOMDUDE_WEAPON* pWeap, DVector3& offs, DVector3& vel);
static DBloodActor* weaponShotSummon(CUSTOMDUDE* pDude, CUSTOMDUDE_WEAPON* pWeap, DVector3& offs, DVector3& vel);
static DBloodActor* weaponShotKamikaze(CUSTOMDUDE* pDude, CUSTOMDUDE_WEAPON* pWeap, DVector3& offs, DVector3& vel);
static DBloodActor* weaponShotSpecialBeastStomp(CUSTOMDUDE* pDude, CUSTOMDUDE_WEAPON*, DVector3& offs, DVector3& vel);

DBloodActor* (*gWeaponShotFunc[])(CUSTOMDUDE* pDude, CUSTOMDUDE_WEAPON* pWeap, DVector3& offs, DVector3& vel) =
{
    weaponShotDummy,    // none
    weaponShotHitscan,
    weaponShotMissile,
    weaponShotThing,
    weaponShotSummon,   // vanilla dude
    weaponShotSummon,   // custom  dude
    weaponShotKamikaze,
    weaponShotSpecialBeastStomp,
};

static AISTATE gCdudeStateDeath = { kAiStateOther, -1, nullptr, 0, &AF(enterDeath), NULL, NULL, NULL }; // just turns dude to a gib

// Land, Crouch, Swim (proper order matters!)
AISTATE gCdudeStateTemplate[kCdudeStateNormalMax][kCdudePostureMax] =
{
    // idle (don't change pos or patrol gets broken!)
    {
        { kAiStateIdle,     SEQOFFS(0),   nullptr, 0, &AF(resetTarget), NULL, &AF(thinkTarget), NULL },
        { kAiStateIdle,     SEQOFFS(17),  nullptr, 0, &AF(resetTarget), NULL, &AF(thinkTarget), NULL },
        { kAiStateIdle,     SEQOFFS(13),  nullptr, 0, &AF(resetTarget), NULL, &AF(thinkTarget), NULL },
    },

    // search (don't change pos or patrol gets broken!)
    {
        { kAiStateSearch,   SEQOFFS(9),  nullptr, 800, NULL, &AF(moveForward), &AF(thinkSearch), &gCdudeStateTemplate[kCdudeStateIdle][kCdudePostureL] },
        { kAiStateSearch,   SEQOFFS(14), nullptr, 800, NULL, &AF(moveForward), &AF(thinkSearch), &gCdudeStateTemplate[kCdudeStateIdle][kCdudePostureC] },
        { kAiStateSearch,   SEQOFFS(13), nullptr, 800, NULL, &AF(moveForward), &AF(thinkSearch), &gCdudeStateTemplate[kCdudeStateIdle][kCdudePostureW] },
    },

    // dodge
    {
        { kAiStateMove,     SEQOFFS(9),  nullptr, 90, NULL,  &AF(moveDodge),	NULL, &gCdudeStateTemplate[kCdudeStateChase][kCdudePostureL] },
        { kAiStateMove,     SEQOFFS(14), nullptr, 90, NULL,  &AF(moveDodge),	NULL, &gCdudeStateTemplate[kCdudeStateChase][kCdudePostureC] },
        { kAiStateMove,     SEQOFFS(13), nullptr, 90, NULL,  &AF(moveDodge),	NULL, &gCdudeStateTemplate[kCdudeStateChase][kCdudePostureW] },
    },

    // chase
    {
        { kAiStateChase,    SEQOFFS(9),  nullptr, 30, NULL,	&AF(moveForward), &AF(thinkChase), NULL },
        { kAiStateChase,    SEQOFFS(14), nullptr, 30, NULL,	&AF(moveForward), &AF(thinkChase), NULL },
        { kAiStateChase,    SEQOFFS(13), nullptr, 30, NULL,	&AF(moveForward), &AF(thinkChase), NULL },
    },

    // flee
    {
        { kAiStateMove,    SEQOFFS(9),  nullptr, 256, NULL,	&AF(moveForward), &AF(thinkFlee), &gCdudeStateTemplate[kCdudeStateSearch][kCdudePostureL] },
        { kAiStateMove,    SEQOFFS(14), nullptr, 256, NULL,	&AF(moveForward), &AF(thinkFlee), &gCdudeStateTemplate[kCdudeStateSearch][kCdudePostureC] },
        { kAiStateMove,    SEQOFFS(13), nullptr, 256, NULL,	&AF(moveForward), &AF(thinkFlee), &gCdudeStateTemplate[kCdudeStateSearch][kCdudePostureW] },
    },

    // recoil normal
    {
        { kAiStateRecoil,   SEQOFFS(5), nullptr, 0, NULL, NULL, NULL, &gCdudeStateTemplate[kCdudeStateChase][kCdudePostureL] },
        { kAiStateRecoil,   SEQOFFS(5), nullptr, 0, NULL, NULL, NULL, &gCdudeStateTemplate[kCdudeStateChase][kCdudePostureC] },
        { kAiStateRecoil,   SEQOFFS(5), nullptr, 0, NULL, NULL, NULL, &gCdudeStateTemplate[kCdudeStateChase][kCdudePostureW] },
    },

    // recoil tesla
    {
        { kAiStateRecoil,   SEQOFFS(4), nullptr, 0, NULL, NULL, NULL, &gCdudeStateTemplate[kCdudeStateChase][kCdudePostureL] },
        { kAiStateRecoil,   SEQOFFS(4), nullptr, 0, NULL, NULL, NULL, &gCdudeStateTemplate[kCdudeStateChase][kCdudePostureC] },
        { kAiStateRecoil,   SEQOFFS(4), nullptr, 0, NULL, NULL, NULL, &gCdudeStateTemplate[kCdudeStateChase][kCdudePostureW] },
    },

    // burn search
    {
        { kAiStateSearch,   SEQOFFS(3), nullptr, 3600, &AF(enterBurnSearchWater), &AF(aiMoveForward), &AF(maybeThinkSearch), &gCdudeStateTemplate[kCdudeBurnStateSearch][kCdudePostureL]},
        { kAiStateSearch,   SEQOFFS(3), nullptr, 3600, &AF(enterBurnSearchWater), &AF(aiMoveForward), &AF(maybeThinkSearch), &gCdudeStateTemplate[kCdudeBurnStateSearch][kCdudePostureC] },
        { kAiStateSearch,   SEQOFFS(3), nullptr, 3600, &AF(enterBurnSearchWater), &AF(aiMoveForward), &AF(maybeThinkSearch), &gCdudeStateTemplate[kCdudeBurnStateSearch][kCdudePostureW] },
    },

    // morph (put thinkFunc in moveFunc because it supposed to work fast)
    {
        { kAiStateOther,   SEQOFFS(18), nullptr, 0, &AF(enterMorph), &AF(thinkMorph), NULL, NULL },
        { kAiStateOther,   SEQOFFS(18), nullptr, 0, &AF(enterMorph), &AF(thinkMorph), NULL, NULL },
        { kAiStateOther,   SEQOFFS(18), nullptr, 0, &AF(enterMorph), &AF(thinkMorph), NULL, NULL },
    },

    // knock enter
    {
        { kAiStateKnockout,   SEQOFFS(0), nullptr, 0, NULL, NULL,         NULL, &gCdudeStateTemplate[kCdudeStateKnock][kCdudePostureL] },
        { kAiStateKnockout,   SEQOFFS(0), nullptr, 0, NULL, NULL,         NULL, &gCdudeStateTemplate[kCdudeStateKnock][kCdudePostureC] },
        { kAiStateKnockout,   SEQOFFS(0), nullptr, 0, NULL, &AF(moveKnockout), NULL, &gCdudeStateTemplate[kCdudeStateKnock][kCdudePostureW] },
    },

    // knock
    {
        { kAiStateKnockout,   SEQOFFS(0), nullptr, 0, NULL, NULL,         NULL, &gCdudeStateTemplate[kCdudeStateKnockExit][kCdudePostureL] },
        { kAiStateKnockout,   SEQOFFS(0), nullptr, 0, NULL, NULL,         NULL, &gCdudeStateTemplate[kCdudeStateKnockExit][kCdudePostureC] },
        { kAiStateKnockout,   SEQOFFS(0), nullptr, 0, NULL, &AF(moveKnockout), NULL, &gCdudeStateTemplate[kCdudeStateKnockExit][kCdudePostureW] },
    },

    // knock exit
    {
        { kAiStateKnockout,   SEQOFFS(0), nullptr, 0, NULL, &AF(turnToTarget), NULL, &gCdudeStateTemplate[kCdudeStateSearch][kCdudePostureL] },
        { kAiStateKnockout,   SEQOFFS(0), nullptr, 0, NULL, &AF(turnToTarget), NULL, &gCdudeStateTemplate[kCdudeStateSearch][kCdudePostureC] },
        { kAiStateKnockout,   SEQOFFS(0), nullptr, 0, NULL, &AF(turnToTarget), NULL, &gCdudeStateTemplate[kCdudeStateSearch][kCdudePostureW] },
    },

    // sleep
    {
        { kAiStateIdle,     SEQOFFS(0),  nullptr, 0, &AF(enterSleep), NULL, &AF(thinkTarget), NULL },
        { kAiStateIdle,     SEQOFFS(0),  nullptr, 0, &AF(enterSleep), NULL, &AF(thinkTarget), NULL },
        { kAiStateIdle,     SEQOFFS(0),  nullptr, 0, &AF(enterSleep), NULL, &AF(thinkTarget), NULL },
    },

    // wake
    {
        { kAiStateIdle,     SEQOFFS(0),  nullptr, 0, &AF(enterWake), &AF(turnToTarget), NULL, &gCdudeStateTemplate[kCdudeStateSearch][kCdudePostureL] },
        { kAiStateIdle,     SEQOFFS(0),  nullptr, 0, &AF(enterWake), &AF(turnToTarget), NULL, &gCdudeStateTemplate[kCdudeStateSearch][kCdudePostureC] },
        { kAiStateIdle,     SEQOFFS(0),  nullptr, 0, &AF(enterWake), &AF(turnToTarget), NULL, &gCdudeStateTemplate[kCdudeStateSearch][kCdudePostureW] },
    },

    // generic idle (ai fight compat.)
    {
        { kAiStateGenIdle,     SEQOFFS(0),   nullptr, 0, &AF(resetTarget), NULL, NULL, NULL },
        { kAiStateGenIdle,     SEQOFFS(17),  nullptr, 0, &AF(resetTarget), NULL, NULL, NULL },
        { kAiStateGenIdle,     SEQOFFS(13),  nullptr, 0, &AF(resetTarget), NULL, NULL, NULL },
    },
};

// Land, Crouch, Swim
AISTATE gCdudeStateAttackTemplate[kCdudePostureMax] =
{
    // attack (put thinkFunc in moveFunc because it supposed to work fast)
    { kAiStateAttack,   SEQOFFS(6), &AF(weaponShot), 0, &AF(moveStop), &AF(thinkChase), NULL, &gCdudeStateAttackTemplate[kCdudePostureL] },
    { kAiStateAttack,   SEQOFFS(8), &AF(weaponShot), 0, &AF(moveStop), &AF(thinkChase), NULL, &gCdudeStateAttackTemplate[kCdudePostureC] },
    { kAiStateAttack,   SEQOFFS(8), &AF(weaponShot), 0, &AF(moveStop), &AF(thinkChase), NULL, &gCdudeStateAttackTemplate[kCdudePostureW] },
};

// Random pick
AISTATE gCdudeStateDyingTemplate[kCdudePostureMax] =
{
    // dying
    { kAiStateOther,   SEQOFFS(1), nullptr, 0, &AF(enterDying), NULL, &AF(thinkDying), &gCdudeStateDeath },
    { kAiStateOther,   SEQOFFS(1), nullptr, 0, &AF(enterDying), NULL, &AF(thinkDying), &gCdudeStateDeath },
    { kAiStateOther,   SEQOFFS(1), nullptr, 0, &AF(enterDying), NULL, &AF(thinkDying), &gCdudeStateDeath },
};

// for kModernThingThrowableRock
static const short gCdudeDebrisPics[6] =
{
    2406, 2280, 2185, 2155, 2620, 3135
};

static DBloodActor* weaponShotHitscan(CUSTOMDUDE* pDude, CUSTOMDUDE_WEAPON* pWeap, DVector3& pOffs, DVector3& vel)
{
    const VECTORDATA* pVect = &gVectorData[pWeap->id];
    auto pSpr = pDude->pSpr;

    // ugly hack to make it fire at required distance was removed because we have a better solution! :P
    actFireVector(pSpr, pOffs.X, pOffs.Y, vel, (VECTOR_TYPE)pWeap->id, pWeap->GetDistanceF());

    return nullptr;
}

static DBloodActor* weaponShotMissile(CUSTOMDUDE* pDude, CUSTOMDUDE_WEAPON* pWeap, DVector3& pOffs, DVector3& vel)
{
    DBloodActor* pSpr = pDude->pSpr, *pShot;

    pShot = nnExtFireMissile(pSpr, pOffs.X, pOffs.Y, vel, pWeap->id);
    if (pShot)
    {
        nnExtOffsetSprite(pShot, DVector3(0, pOffs.Y, 0));

        if (pWeap->shot.clipdist)
            pShot->clipdist = pWeap->shot.clipdist;

        if (pWeap->HaveVelocity())
        {
            pShot->xspr.target = nullptr; // have to erase, so vanilla won't set velocity back
            nnExtScaleVelocity(pShot, pWeap->shot._velocity, vel);
        }

        pWeap->shot.appearance.Set(pShot);

        if (pWeap->shot.targetFollow != nullAngle)
        {
            pShot->xspr.goalAng = pWeap->shot.targetFollow;
            gFlwSpritesList.Push(MakeObjPtr(pShot));
            pShot->prevmarker = pSpr->xspr.target;
            pShot->xspr.target = nullptr; // have own target follow code
        }


        return pShot;
    }

    return nullptr;
}

static DBloodActor* weaponShotThing(CUSTOMDUDE* pDude, CUSTOMDUDE_WEAPON* pWeap, DVector3& pOffs, DVector3& vel)
{
    DBloodActor* pSpr = pDude->pSpr;
    DBloodActor* pLeech = pDude->pLeech, *pShot, *pTarget = pSpr->xspr.target;
    
    if (!pTarget)
        return nullptr;

    auto dv = pTarget->spr.pos - pSpr->spr.pos;
    auto nDist = dv.Length();
    int nDiv = 540, nSlope = 12000;
    bool impact = true;
    int nHealth = 0;

    switch (pWeap->id)
    {
        case kModernThingEnemyLifeLeech:
        case kThingDroppedLifeLeech:
            if (!pDude->IsLeechBroken())
            {
                if (pLeech)
                {
                    if (xsprIsFine(pLeech))
                        nHealth = pLeech->xspr.health;

                    pDude->LeechPickup(); // pickup it before throw
                }
            }
            break;
    }
    
    nSlope = (pWeap->HaveSlope()) ? pWeap->shot.slope : (int(vel.Z * 2) - nSlope);

    // fixed point math sucks
    //nVel = divscale23(nDist / nDiv, 120);
    double nVel = (nDist * worldtoint * 128 / nDiv) / 120.;

    pShot = actFireThing(pSpr, -pOffs.X, pOffs.Z, FixedToFloat(nSlope), pWeap->id, nVel);
    if (pShot)
    {
        nnExtOffsetSprite(pShot, DVector3(0, pOffs.Y, 0));

        pShot->ownerActor            = pSpr;

        switch (pWeap->id)
        {
            case kModernThingTNTProx:
            case kThingArmedProxBomb:
            case kModernThingThrowableRock:
            case kModernThingEnemyLifeLeech:
            case kThingDroppedLifeLeech:
            case kThingBloodBits:
            case kThingBloodChunks:
                switch (pWeap->id)
                {
                    case kModernThingThrowableRock:
                        pShot->spr.setspritetexture(tileGetTextureID(gCdudeDebrisPics[Random(countof(gCdudeDebrisPics))]));
                        pShot->spr.scale.X  = pShot->spr.scale.Y = (24 + Random(42)) * REPEAT_SCALE;
                        pShot->spr.cstat    |= CSTAT_SPRITE_BLOCK;
                        pShot->spr.pal      = 5;

                        if (Chance(0x5000)) pShot->spr.cstat |= CSTAT_SPRITE_XFLIP;
                        if (Chance(0x5000)) pShot->spr.cstat |= CSTAT_SPRITE_YFLIP;

                        if (pShot->spr.scale.X > 60 * REPEAT_SCALE)       pShot->xspr.data1 = 43;
                        else if (pShot->spr.scale.X > 40 * REPEAT_SCALE)  pShot->xspr.data1 = 33;
                        else if (pShot->spr.scale.X > 30 * REPEAT_SCALE)  pShot->xspr.data1 = 23;
                        else                           pShot->xspr.data1 = 12;
                        break;
                    case kThingArmedProxBomb:
                    case kModernThingTNTProx:
                        pShot->xspr.state = 0;
                        pShot->xspr.Proximity = true;
                        break;
                    case kThingBloodBits:
                    case kThingBloodChunks:
                        DudeToGibCallback1(pShot);
                        break;
                    default:
                        if (pLeech)
                        {
                            pShot->xspr.health = nHealth;
                        }
                        else
                        {
                            pShot->xspr.health = ((pShot->IntVar("defhealth") << 4) * ClipLow(gGameOptions.nDifficulty, 1)) >> 1;
                        }

                        pShot->spr.cstat        &= ~CSTAT_SPRITE_BLOCK;
                        pShot->spr.pal          = 6;
                        pShot->spr.clipdist     = 0;
                        pShot->xspr.data3       = 512 / (gGameOptions.nDifficulty + 1);
                        pShot->xspr.target      = pTarget;
                        pShot->xspr.Proximity   = true;
                        pShot->xspr.stateTimer  = 1;

                        evPostActor(pShot, 80, AF(LeechStateTimer));
                        pDude->pLeech = pShot;
                        break;
                }
                impact = false;
                break;
            case kThingNapalmBall:
                pShot->spr.scale.X = pShot->spr.scale.Y = 24 * REPEAT_SCALE;
                pShot->xspr.data4 = 3 + Random2(2);
                pShot->xspr.Impact = true;
                break;
        }

        if (pWeap->shot.clipdist)               pShot->clipdist = pWeap->shot.clipdist * CLIPDIST_SCALE;
        if (pWeap->HaveVelocity())              nnExtScaleVelocity(pShot, pWeap->shot._velocity * 8, vel, 0x01);
        
        pWeap->shot.appearance.Set(pShot);

        if (pWeap->shot.targetFollow != nullAngle)
        {
            pShot->xspr.goalAng = pWeap->shot.targetFollow;
            pShot->prevmarker = pSpr->xspr.target;
            gFlwSpritesList.Push(MakeObjPtr(pShot));
        }

        if (pWeap->shot.impact > 1)
        {
            if (impact)
                pShot->xspr.Impact = (pShot->xspr.Impact != 0 && nDist <= 7680);
        }
        else
        {
            pShot->xspr.Impact = pWeap->shot.impact;
        }

        if (!pShot->xspr.Impact)
            evPostActor(pShot, 120 * Random(2) + 120, kCmdOn, pSpr);

        return pShot;
    }

    return nullptr;
}

static bool posObstructed(DVector3& pos, double nRadius)
{
    int i;
    for (i = sector.SSize() - 1; i >= 0; i--)
    {
        if (inside(pos.X, pos.Y, &sector[i]))
            break;
    }
    if (i < 0)
        return true;

    BloodSpriteIterator it;
    while (auto pSpr = it.Next())
    {
        if ((pSpr->spr.flags & kHitagFree) || (pSpr->spr.flags & kHitagRespawn)) continue;
        if ((pSpr->spr.cstat & CSTAT_SPRITE_ALIGNMENT_MASK) != CSTAT_SPRITE_ALIGNMENT_FACING)
            continue;

        if (!(pSpr->spr.cstat & CSTAT_SPRITE_BLOCK))
        {
            if (!pSpr->IsDudeActor() || !dudeIsAlive(pSpr))
                continue;
        }
        else
        {
            auto tex = TexMan.GetGameTexture(pSpr->spr.spritetexture());
            if (!tex) continue;
            float w = tex->GetDisplayWidth();
            float h = tex->GetDisplayHeight();

            if (w <= 0 || h <= 0)
                continue;
        }

        if (CheckProximityPoint(pSpr->spr.pos.X, pSpr->spr.pos.Y, pSpr->spr.pos.Z, pos.X, pos.Y, pos.Z, nRadius))
            return true;
    }

    return false;
}



static DBloodActor* weaponShotSummon(CUSTOMDUDE* pDude, CUSTOMDUDE_WEAPON* pWeap, DVector3& offs, DVector3& vel)
{
    DBloodActor* pShot, *pSpr = pDude->pSpr;

    DVector3 pos = pSpr->spr.pos;
    DAngle a = nullAngle;
    
    int nDude = pWeap->id;
    if (pWeap->type == kCdudeWeaponSummonCdude)
        nDude = kDudeModernCustom;

    DVector3 pOffs(offs.X, max(offs.Y, 800.), offs.Z);
    nnExtOffsetPos(pOffs, pSpr->spr.Angles.Yaw, pos);

    while (a < DAngle180)
    {
        if (!posObstructed(pos, 2.))
        {
            if ((pShot = nnExtSpawnDude(pSpr, nDude, pos)) != NULL)
            {
                if (nDude == kDudeModernCustom)
                    pShot->xspr.data1 = pWeap->id;

                if (pWeap->shot.clipdist)
                    pShot->clipdist = pWeap->shot.clipdist * CLIPDIST_SCALE;

                if (pWeap->HaveVelocity())
                    nnExtScaleVelocity(pShot, pWeap->shot._velocity, vel);

                pWeap->shot.appearance.Set(pShot);

                aiInitSprite(pShot);

                pShot->xspr.TargetPos  = pSpr->xspr.TargetPos;
                pShot->xspr.target     = pSpr->xspr.target;
                pShot->spr.Angles      = pSpr->spr.Angles;

                aiActivateDude(pShot);

                pDude->pSlaves.Push(MakeObjPtr(pShot));
                if (AllowedKillType(pShot))
                    Level.addKillCount();

                return pShot;
            }
        }
        else
        {
            pos.XY() = rotatepoint(pSpr->spr.pos.XY(), pos.XY(), a);
            a += DAngle15;
            continue;
        }

        break;
    }

    return nullptr;
}

static DBloodActor* weaponShotKamikaze(CUSTOMDUDE* pDude, CUSTOMDUDE_WEAPON* pWeap, DVector3& pOffs, DVector3& vel)
{
    DBloodActor* pSpr = pDude->pSpr;
    DBloodActor* pShot = actSpawnSprite(pSpr->sector(), pSpr->spr.pos, kStatExplosion, true);

    if (pShot)
    {
        int nType = pWeap->id - kTrapExploder;
        const EXPLOSION* pExpl = &explodeInfo[nType];
        const EXPLOSION_EXTRA* pExtra = &gExplodeExtra[nType];

        pShot->spr.lotag = nType; // this may not call ChangeType!
        pShot->spr.cstat |= CSTAT_SPRITE_INVISIBLE;
        pShot->ownerActor = pSpr;
        pShot->spr.shade = -127;
        pShot->spr.scale.X = pShot->spr.scale.Y = pExpl->repeat * REPEAT_SCALE;
        pShot->spr.Angles.Yaw = pSpr->spr.Angles.Yaw;
        
        pShot->xspr.data1 = pExpl->ticks;
        pShot->xspr.data2 = pExpl->quakeEffect;
        pShot->xspr.data3 = pExpl->flashEffect;
        pShot->xspr.data4 = ClipLow(pWeap->GetDistance() >> 4, pExpl->radius);

        seqSpawn(pExtra->seq, pShot);

        if (pExtra->ground)
           pShot->spr.pos.Z = getflorzofslopeptr(pShot->sector(), pShot->spr.pos.X, pShot->spr.pos.Y);

        pWeap->shot.appearance.Set(pShot);

        clampSprite(pShot);
        nnExtOffsetSprite(pShot, pOffs); // offset after default sprite placement
    }

    if (pSpr->xspr.health)
    {
        pSpr->xspr.health = 0; // it supposed to attack once
        pDude->Kill(pSpr, kDamageExplode, 0x10000);
    }
    
    return pShot;
}

static DBloodActor* weaponShotSpecialBeastStomp(CUSTOMDUDE* pDude, CUSTOMDUDE_WEAPON* pWeapon, DVector3& pOffs, DVector3& vel)
{
    DBloodActor* pSpr = pDude->pSpr;
    
    int vc = 400;
    int v1c = 7 * gGameOptions.nDifficulty;
    int v10 = 55 * gGameOptions.nDifficulty;

    for (auto stat : { kStatDude, kStatThing })
    {
        BloodStatIterator it(stat);
        while (auto pSpr2 = it.Next())
        {
            if (pSpr2 == pSpr || !xsprIsFine(pSpr2) || pSpr2->ownerActor == pSpr)
                continue;

            if (CheckProximity(pSpr2, pSpr->spr.pos, pSpr->sector(), pWeapon->GetDistance()))
            {
                double nDist2 = (pSpr->spr.pos.XY() - pSpr2->spr.pos.XY()).LengthSquared();
                if (nDist2 <= vc * vc)
                {
                    int nDamage;
                    if (!nDist2)
                        nDamage = v1c + v10;
                    else
                        nDamage = v1c + ((vc - nDist2) * v10) / vc;
                        
                    if (pSpr2->IsPlayerActor())
                    {
                        auto pPlayer = getPlayer(pSpr2);
                        pPlayer->quakeEffect = ClipHigh(pPlayer->quakeEffect + (nDamage << 2), 1280);
                    }

                    actDamageSprite(pSpr, pSpr2, kDamageFall, nDamage << 4);
                }
            }
        }
    }

    return nullptr;
}



void weaponShot(DBloodActor* pSpr)
{
    // most of the fixed point math in here has been kept for reasons of simplicity.
    if (!pSpr->hasX())
        return;

    CUSTOMDUDE* pDude = cdudeGet(pSpr);
    CUSTOMDUDE_WEAPON *pCurWeap = pDude->pWeapon, *pWeap;
    DBloodActor *pShot;
    POINT3D *pStyleOffs;
    DVector3 shotoffs;

    int nShots, nTime;
    int dz1;
    int dx2, dy2, dz2;
    int dx3, dy3, dz3;
    int i, j;

    int txof; char hxof;
    int sang; int  hsht;
    int tang; char styled;
    

    const int dx1 = int(pSpr->spr.Angles.Yaw.Cos() * 16384);
    const int dy1 = int(pSpr->spr.Angles.Yaw.Sin() * 16384);

    if (pCurWeap)
    {
        for (i = 0; i < pDude->numWeapons; i++)
        {
            pWeap = &pDude->weapons[i];
            if (pWeap->available)
            {
                if (pCurWeap != pWeap)
                {
                    // check if this weapon could be used in conjunction with current
                    if (!pCurWeap->sharedId || pCurWeap->sharedId != pWeap->sharedId)
                        continue;
                }

                nShots = pWeap->GetNumshots(); pWeap->ammo.Dec(nShots);
                styled = (nShots > 1 && pWeap->style.available);
                shotoffs = pWeap->shot.offset;

                if (styled)
                {
                    pStyleOffs = &pWeap->style.offset; hsht = nShots >> 1;
                    sang = pWeap->style.angle / nShots;
                    hxof = 0;
                    tang = 0;
                }

                dz1 = (pWeap->shot.slope == INT32_MAX) ?
                        pDude->AdjustSlope(pSpr->xspr.target, pWeap->shot.offset.Z) : pWeap->shot.slope;

                for (j = nShots; j > 0; j--)
                {
                    if (!styled || j == nShots)
                    {
                        dx3 = Random3(pWeap->dispersion[0]);
                        dy3 = Random3(pWeap->dispersion[0]);
                        dz3 = Random3(pWeap->dispersion[1]);

                        dx2 = dx1 + dx3;
                        dy2 = dy1 + dy3;
                        dz2 = dz1 + dz3;
                    }

                    DVector3 dv(dx2 * inttoworld, dy2 * inttoworld, dz2 * zinttoworld);
                    pShot = gWeaponShotFunc[pWeap->type](pDude, pWeap, shotoffs, dv);
                    if (pShot)
                    {
                        // override removal timer
                        if ((nTime = pWeap->shot.remTime) >= 0)
                        {
                            evKillActor(pShot, AF(RemoveActor));
                            if (nTime)
                                evPostActor(pShot, nTime, AF(RemoveActor));
                        }

                        // setup style
                        if (styled)
                        {
                            if (pStyleOffs->X)
                            {
                                txof = pStyleOffs->X;
                                if (j <= hsht)
                                {
                                    if (!hxof)
                                    {
                                        shotoffs.X = pWeap->shot.offset.X;
                                        hxof = 1;
                                    }

                                    txof = -txof;
                                }

                                shotoffs.X += txof * inttoworld;
                            }

                            shotoffs.Y += pStyleOffs->Y * inttoworld;
                            shotoffs.Z += pStyleOffs->Z * zinttoworld;

                            if (pWeap->style.angle)
                            {
                                // for sprites
                                if (pShot)
                                {
                                    if (j <= hsht && sang > 0)
                                    {
                                        sang = -sang;
                                        tang = 0;
                                    }

                                    tang += sang;
                                    pShot->vel.XY() = rotatepoint(pShot->vel.XY(), pSpr->spr.pos.XY(), DAngle::fromBuild(tang)); // formula looks broken
                                    //pShot->vel.XY() = rotatepoint(pShot->vel.XY(), DVector2(0, 0), DAngle::fromBuild(tang)); // what it probably should be!
                                    pShot->spr.Angles.Yaw = pShot->vel.Angle();
                                }
                                // for hitscan
                                else
                                {
                                    if (j <= hsht && sang > 0)
                                    {
                                        dx2 = dx1 + dx3; dy2 = dy1 +  dy3;
                                        sang = -sang;
                                    }

                                    auto dv = rotatepoint(DVector2(dx2 * inttoworld, dy2 * inttoworld), pSpr->spr.pos.XY(), DAngle::fromBuild(sang)); // formula looks broken
                                    //auto dv = rotatepoint(DVector2(dx2 * inttoworld, dy2 * inttoworld), DVector2(0, 0), DAngle::fromBuild(sang)); // what it probably should be!
                                    dx2 = int(dv.X * worldtoint);
                                    dy2 = int(dv.Y * worldtoint);
                                }
                            }
                        }
                    }
                }

                pWeap->sound.Play(pSpr);
                if (pWeap->cooldown.Check())
                    pWeap->available = 0;
            }
        }
    }
}

static int checkTarget(CUSTOMDUDE* pDude, DBloodActor* pTarget, TARGET_INFO* pOut)
{
    DBloodActor* pSpr = pDude->pSpr;
    if (!xspriRangeIsFine(pTarget->extra))
        return -1;

    XSPRITE* pXTarget = &xsprite[pTarget->extra];
    if (pSpr->owner == pTarget->index || pXTarget->health <= 0)
        return -2;

    if (IsPlayerSprite(pTarget))
    {
        PLAYER* pPlayer = &gPlayer[pTarget->type - kDudePlayer1];
        if (powerupCheck(pPlayer, kPwUpShadowCloak) > 0)
            return -3;
    }

    int x = pTarget->x;
    int y = pTarget->y;
    int z = pTarget->z;
    int nSector = pTarget->sectnum;
    int dx = x - pSpr->x;
    int dy = y - pSpr->y;
    int nDist = approxDist(dx, dy);
    char s = (nDist < pDude->seeDist);
    char h = (nDist < pDude->hearDist);
    
    if (!s && !h)
        return -4;

    DUDEINFO* pInfo = pDude->pInfo;
    if (!cansee(x, y, z, nSector, pSpr->x, pSpr->y, pSpr->z - ((pInfo->eyeHeight * pSpr->yrepeat) << 2), pSpr->sectnum))
        return -5;

    int nAng = getangle(dx, dy);
    if (s)
    {
        int nDang = klabs(((nAng + kAng180 - pSpr->ang) & kAngMask) - kAng180);
        if (nDang <= pDude->periphery)
        {
            pOut->pSpr  = pTarget;
            pOut->nDist = nDist;
            pOut->nDang = nDang;
            pOut->nAng  = nAng;
            pOut->nCode = 1;
            return 1;
        }
    }
    
    if (h)
    {
        pOut->pSpr  = pTarget;
        pOut->nDist = nDist;
        pOut->nDang = 0;
        pOut->nAng  = nAng;
        pOut->nCode = 2;
        return 2;
    }

    return -255;
}

void thinkTarget(DBloodActor* pSpr)
{
    int i; 
    spritetype* pTarget;
    TARGET_INFO targets[kMaxPlayers], *pInfo = targets;
    CUSTOMDUDE* pDude = cdudeGet(pSpr);
    int numTargets = 0;

    if (Chance(pDude->pInfo->alertChance))
    {
        for (i = connecthead; i >= 0; i = connectpoint2[i])
        {
            PLAYER* pPlayer = &gPlayer[i];
            if (checkTarget(pDude, pPlayer->pSprite, &targets[numTargets]) > 0)
                numTargets++;
        }

        if (numTargets)
        {
            if (numTargets > 1) // closest first
                qsort(targets, numTargets, sizeof(targets[0]), (int(*)(const void*, const void*))qsSortTargets);

            pTarget = pInfo->pSpr;
            if (pDude->pExtra->stats.active)
            {
                if (pSpr->xspr.target != pTarget->index || Chance(0x0400))
                    pDude->PlaySound(kCdudeSndTargetSpot);
            }
            
            pSpr->xspr.goalAng = pInfo->nAng & kAngMask;
            if (pInfo->nCode == 1) aiSetTarget(pXSpr, pTarget->index);
            else aiSetTarget(pXSpr, pTarget->x, pTarget->y, pTarget->z);
            aiActivateDude(pSpr);
        }
    }
}

void thinkFlee(DBloodActor* pSpr)
{
    int nAng = getangle(pSpr->x - pSpr->xspr.targetX, pSpr->y - pSpr->xspr.targetY);
    int nDang = klabs(((nAng + kAng180 - pSpr->ang) & kAngMask) - kAng180);
    if (nDang > kAng45)
        pSpr->xspr.goalAng = (nAng + (kAng15 * Random2(2))) & kAngMask;

    aiChooseDirection(pSpr, pXSpr, pSpr->xspr.goalAng);

}

void thinkSearch(DBloodActor* pSpr)
{
    aiChooseDirection(pSpr, pSpr->xspr.goalAng);
    thinkTarget(pSpr);
}

void maybeThinkSearch(DBloodActor* pSpr)
{
    // this originally edited the state's callback, but that's inherently non-serializable so another way had to be chosen.
    if (pSpr->chasehackflag)
    {
        aiChooseDirection(pSpr, pSpr->xspr.goalAng);
        thinkTarget(pSpr);
    }
}

void thinkChase(DBloodActor* pSpr)
{
    CUSTOMDUDE* pDude = cdudeGet(pSpr); HITINFO* pHit = &gHitInfo; DUDEINFO* pInfo = pDude->pInfo;
    int nDist, nHeigh, dx, dy, nDAng, nSlope = 0;
    char thinkTime = THINK_CLOCK(pSpr->index);
    char turn2target = 0, interrupt = 0;
    char inAttack = pDude->IsAttacking();
    char changePos = 0;

    if (!spriRangeIsFine(pSpr->xspr.target))
    {
        pDude->NewState(kCdudeStateSearch);
        return;
    }

    spritetype* pTarget = &sprite[pSpr->xspr.target];
    if (pTarget->owner == pSpr->index || !IsDudeSprite(pTarget) || !xsprIsFine(pTarget)) // target lost
    {
        pDude->NewState(kCdudeStateSearch);
        return;
    }

    XSPRITE* pXTarget = &xsprite[pTarget->extra];
    if (pXTarget->health <= 0) // target is dead
    {
        PLAYER* pPlayer = NULL;
        if ((!IsPlayerSprite(pTarget)) || ((pPlayer = getPlayerById(pTarget->type)) != NULL && pPlayer->fraggerId == pSpr->index))
            pDude->PlaySound(kCdudeSndTargetDead);
        
        if (inAttack) pDude->NextState(kCdudeStateSearch);
        else pDude->NewState(kCdudeStateSearch);
        return;
    }

    if (IsPlayerSprite(pTarget))
    {
        PLAYER* pPlayer = &gPlayer[pTarget->type - kDudePlayer1];
        if (powerupCheck(pPlayer, kPwUpShadowCloak) > 0)
        {
            pDude->NewState(kCdudeStateSearch);
            return;
        }
    }

    // check target
    dx = pTarget->x - pSpr->x;
    dy = pTarget->y - pSpr->y;
    nDist = approxDist(dx, dy);

    nDAng = klabs(((getangle(dx, dy) + kAng180 - pSpr->ang) & kAngMask) - kAng180);
    nHeigh = (pInfo->eyeHeight * pSpr->yrepeat) << 2;

    if (thinkTime && !inAttack)
        aiChooseDirection(pSpr, pXSpr, getangle(dx, dy));

    // is the target visible?
    if (nDist > pInfo->seeDist || !cansee(pTarget->x, pTarget->y, pTarget->z, pTarget->sectnum, pSpr->x, pSpr->y, pSpr->z - nHeigh, pSpr->sectnum))
    {
        if (inAttack) pDude->NextState(kCdudeStateSearch);
        else pDude->NewState(kCdudeStateSearch);
        return;
    }
    else if (nDAng > pInfo->periphery)
    {
        if (inAttack) pDude->NextState(kCdudeStateChase);
        else pDude->NewState(kCdudeStateChase);
        return;
    }

    ARG_PICK_WEAPON* pPickArg = new ARG_PICK_WEAPON(pSpr, pXSpr, pTarget, pXTarget, nDist, nDAng);
    CUSTOMDUDE_WEAPON* pWeapon = pDude->pWeapon;
    if (pWeapon)
    {
        nSlope      = pDude->AdjustSlope(pSpr->xspr.target, pWeapon->shot.offset.z);
        turn2target = pWeapon->turnToTarget;
        interrupt   = pWeapon->interruptable;
    }

    if (thinkTime && Chance(0x2000))
        pDude->PlaySound(kCdudeSndTargetChase);

    // in attack
    if (inAttack)
    {
        if (turn2target)
        {
            pSpr->xspr.goalAng = getTargetAng(pSpr);
            moveTurn(pSpr);
        }

        if (pSpr->xspr.aiState->stateTicks) // attack timer set
        {
            if (!pSpr->xspr.stateTimer)
            {
                pWeapon = pDude->PickWeapon(pPickArg);
                if (pWeapon && pWeapon == pDude->pWeapon)
                {
                    pDude->pWeapon = pWeapon;
                    pDude->NewState(pWeapon->stateID);
                }
                else
                    pDude->NewState(kCdudeStateChase);
            }
            else if (interrupt)
            {
                pDude->PickWeapon(pPickArg);
                if (!pWeapon->available)
                    pDude->NewState(kCdudeStateChase);
            }

            return;
        }

        if (!pDude->SeqPlaying()) // final frame
        {
            pWeapon = pDude->PickWeapon(pPickArg);
            if (!pWeapon)
            {
                pDude->NewState(kCdudeStateChase);
                return;
            }
            else
            {
                pDude->pWeapon = pWeapon;
            }
        }
        else // playing previous animation
        {
            if (!interrupt)
            {
                if (!pWeapon)
                {
                    pDude->NextState(kCdudeStateChase);
                }

                return;
            }
            else
            {
                pDude->PickWeapon(pPickArg);
                if (!pWeapon->available)
                {
                    pDude->NewState(kCdudeStateChase);
                    return;
                }
            }
        }
    }
    else
    {
        // enter attack
        pWeapon = pDude->PickWeapon(pPickArg);
        if (pWeapon)
            pDude->pWeapon = pWeapon;
    }

    if (pWeapon)
    {
        switch (pWeapon->type)
        {
            case kCdudeWeaponNone:
                if (pDude->CanMove()) pDude->NextState(kCdudeStateFlee);
                else pDude->NextState(kCdudeStateSearch);
                return;
            case kCdudeWeaponHitscan:
            case kCdudeWeaponMissile:
            case kCdudeWeaponThrow:
                if (pDude->CanMove())
                {
                    HitScan(pSpr, pSpr->z, dx, dy, nSlope, pWeapon->clipMask, nDist);
                    if (pHit->hitsprite != pSpr->xspr.target && !pDude->AdjustSlope(nDist, &nSlope))
                    {
                        changePos = 1;
                        if (spriRangeIsFine(pHit->hitsprite))
                        {
                            spritetype* pHitSpr = &sprite[pHit->hitsprite];
                            XSPRITE* pXHitSpr = NULL;
                            if (xsprIsFine(pHitSpr))
                                pXHitSpr = &xsprite[pHitSpr->extra];

                            if (IsDudeSprite(pHitSpr))
                            {
                                if (pXHitSpr)
                                {
                                    if (pXHitSpr->target == pSpr->index)
                                        return;

                                    if (pXHitSpr->dodgeDir > 0)
                                        pSpr->xspr.dodgeDir = -pXHitSpr->dodgeDir;
                                }
                            }
                            else if (pHitSpr->owner == pSpr->index) // projectiles, things, fx etc...
                            {
                                if (!pXHitSpr || !pXHitSpr->health)
                                    changePos = 0;
                            }
                            
                            if (changePos)
                            {
                                // prefer dodge
                                if (pDude->dodge.onAimMiss.Allow())
                                {
                                    pDude->NewState(kCdudeStateDodge, 30 * (Random(2) + 1));
                                    return;
                                }
                            }
                        }

                        if (changePos)
                        {
                            // prefer chase
                            pDude->NewState(kCdudeStateChase);
                            return;
                        }
                    }
                }
                fallthrough__;
            default:
                pDude->NewState(pWeapon->stateID);
                pDude->NextState(pWeapon->nextStateID);
                return;
        }
    }

    if (!pDude->CanMove())
        pDude->NextState(kCdudeStateSearch);
}

int getTargetAng(DBloodActor* pSpr)
{
    int x, y;
    if (spriRangeIsFine(pSpr->xspr.target))
    {
        spritetype* pTarg = &sprite[pSpr->xspr.target];
        x = pTarg->x;
        y = pTarg->y;
    }
    else
    {
        x = pSpr->xspr.targetX;
        y = pSpr->xspr.targetY;
    }

    return getangle(x - pSpr->x, y - pSpr->y);
}

void turnToTarget(DBloodActor* pSpr)
{
    pSpr->ang = getTargetAng(pSpr);
    pSpr->xspr.goalAng = pSpr->ang;
}

void moveTurn(DBloodActor* pSpr)
{
    CUSTOMDUDE* pDude = cdudeGet(pSpr);
    int nVelTurn = pDude->GetVelocity(kParVelocityTurn);
    int nAng = ((kAng180 + pSpr->xspr.goalAng - pSpr->ang) & kAngMask) - kAng180;
    pSpr->ang = ((pSpr->ang + ClipRange(nAng, -nVelTurn, nVelTurn)) & kAngMask);
}

void moveDodge(DBloodActor* pSpr)
{
    CUSTOMDUDE* pDude = cdudeGet(pSpr);
    moveTurn(pSpr);

    if (pSpr->xspr.dodgeDir && pDude->CanMove())
    {
        int nVelDodge = pDude->GetVelocity(kParVelocityDodge);
        int nCos = Cos(pSpr->ang);                  int nSin = Sin(pSpr->ang);
        int dX = xvel[pSpr->index];                 int dY = yvel[pSpr->index];
        int t1 = dmulscale30(dX, nCos, dY, nSin);   int t2 = dmulscale30(dX, nSin, -dY, nCos);

        if (pSpr->xspr.dodgeDir > 0)
        {
            t2 += nVelDodge;
        }
        else
        {
            t2 -= nVelDodge;
        }

        xvel[pSpr->index] = dmulscale30(t1, nCos, t2, nSin);
        yvel[pSpr->index] = dmulscale30(t1, nSin, -t2, nCos);
    }
}

void moveKnockout(DBloodActor* pSpr)
{
    int zv = pSpr->vel.Z;
    pSpr->vel.Z = ClipRange(zv + mulscale16(zv, 0x3000), 0x1000, 0x40000);
}

void moveForward(DBloodActor* pSpr)
{
    CUSTOMDUDE* pDude = cdudeGet(pSpr);
    int nVelTurn    = pDude->GetVelocity(kParVelocityTurn);
    int nVelForward = pDude->GetVelocity(kParVelocityForward);
    int nAng = ((kAng180 + pSpr->xspr.goalAng - pSpr->ang) & kAngMask) - kAng180;
    pSpr->ang = ((pSpr->ang + ClipRange(nAng, -nVelTurn, nVelTurn)) & kAngMask);
    int z = 0;

    if (pDude->CanMove())
    {
        if (pDude->IsUnderwater())
        {
            if (spriRangeIsFine(pSpr->xspr.target))
            {
                spritetype* pTarget = &sprite[pSpr->xspr.target];
                if (spriteIsUnderwater(pTarget, true))
                    z = (pTarget->z - pSpr->z) + (10 << Random(12));
            }
            else
            {
                z = (pSpr->xspr.targetZ - pSpr->z);
            }

            if (Chance(0x0500))
                z <<= 1;

            pSpr->vel.Z += z;
        }
        
        // don't move forward if trying to turn around
        if (klabs(nAng) <= kAng60)
        {
            xvel[pSpr->index] += mulscale30(Cos(pSpr->ang), nVelForward);
            yvel[pSpr->index] += mulscale30(Sin(pSpr->ang), nVelForward);
        }
    }
}

void enterSleep(DBloodActor* pSpr)
{
    CUSTOMDUDE* pDude = cdudeGet(pSpr);
    pDude->StatusSet(kCdudeStatusSleep);
    resetTarget(pSpr);
    moveStop(pSpr);

    // reduce distances while sleeping
    pDude->seeDist      = kCdudeMinSeeDist;
    pDude->hearDist     = kCdudeMinHearDist;
    pDude->periphery    = kAng360;
}

void enterWake(DBloodActor* pSpr)
{
    CUSTOMDUDE* pDude = cdudeGet(pSpr);
    if (pDude->StatusTest(kCdudeStatusSleep))
    {
        pDude->StatusRem(kCdudeStatusSleep);

        // restore distances when awaked
        pDude->seeDist      = pDude->pInfo->seeDist;
        pDude->hearDist     = pDude->pInfo->hearDist;
        pDude->periphery    = pDude->pInfo->periphery;
    }

    pDude->PlaySound(kCdudeSndWake);
}


void enterDying(DBloodActor* pSpr)
{
    CUSTOMDUDE* pDude = cdudeGet(pSpr);
    if (pDude->mass > 48)
        pDude->mass = ClipLow(pDude->mass >> 2, 48);
}

void thinkDying(DBloodActor* pSpr)
{
    SPRITEHIT* pHit = &pSpr->hit;
    if (pHit->florhit.type == kHitNone && spriteIsUnderwater(pSpr, true))
        pSpr->vel.Z = max(pSpr->vel.Z, 1024.);
}

void enterDeath(DBloodActor* pSpr)
{
    // don't let the data fields gets overwritten!
    if (!(pSpr->spr.flags & kHitagRespawn))
        DudeToGibCallback1(pSpr);

    pSpr->ChangeType(kThingBloodChunks);
    actPostSprite(pSpr, kStatThing);
}

void enterMorph(DBloodActor* pSpr)
{
    CUSTOMDUDE* pDude = cdudeGet(pSpr);
    if (!pDude->IsMorphing())
    {
        pDude->PlaySound(kCdudeSndTransforming);
        pDude->StatusSet(kCdudeStatusMorph); // set morph status
        pSpr->xspr.locked = 1; // lock it while morphing

        pSpr->spr.flags &= ~kPhysMove;
        moveStop(pSpr);
        if (pSpr->xspr.aiState->seqId <= 0)
            seqKill(pSpr);
    }
}

void thinkMorph(DBloodActor* pSpr)
{
    int nTarget; char triggerOn, triggerOff;
    CUSTOMDUDE* pDude = cdudeGet(pSpr);

    if (pDude->SeqPlaying())
    {
        moveStop(pSpr);
        return;
    }
    
    pDude->ClearEffectCallbacks();
    pDude->StatusRem(kCdudeStatusMorph);    // clear morph status
    pSpr->xspr.burnSource = -1;
    pSpr->xspr.burnTime = 0;
    pSpr->xspr.locked = 0;
    pSpr->xspr.scale = 0;

    if (pDude->NextDude)
    {
        // classic morphing to already inserted sprite by TX ID
        DBloodActor* pNext = pDude->NextDude;

        pSpr->xspr.key = pSpr->xspr.dropMsg = 0;

        // save incarnation's going on and off options
        triggerOn = pNext->xspr.triggerOn, triggerOff = pNext->xspr.triggerOff;

        // then remove it from incarnation so it won't send the commands
        pNext->xspr.triggerOn = pNext->xspr.triggerOff = 0;

        // trigger dude death before morphing
        trTriggerSprite(pSpr, kCmdOff, pSpr);

        pSpr->ChangeType(pSpr->spr.inittype = pNext->GetType());
        pSpr->spr.flags = pNext->spr.flags;
        pSpr->spr.pal = pNext->spr.pal;
        pSpr->spr.shade = pNext->spr.shade;
        pSpr->clipdist = pNext->clipdist;
        pSpr->spr.scale = pNext->spr.scale;

        pSpr->xspr.txID = pNext->xspr.txID;
        pSpr->xspr.command = pNext->xspr.command;
        pSpr->xspr.triggerOn = triggerOn;
        pSpr->xspr.triggerOff = triggerOff;
        pSpr->xspr.busyTime = pNext->xspr.busyTime;
        pSpr->xspr.waitTime = pNext->xspr.waitTime;

        // inherit respawn properties
        pSpr->xspr.respawn = pNext->xspr.respawn;
        pSpr->xspr.respawnPending = pNext->xspr.respawnPending;

        pSpr->xspr.data1    = pNext->xspr.data1;                        // for v1 this is weapon id, v2 - descriptor id
        pSpr->xspr.data2    = pNext->xspr.data2;                        // for v1 this is seqBase id
        pSpr->xspr.data3    = pSpr->xspr.sysData1 = pNext->xspr.sysData1;   // for v1 this is soundBase id
        pSpr->xspr.data4    = pSpr->xspr.sysData2 = pNext->xspr.sysData2;   // start hp

        // inherit dude flags
        pSpr->xspr.dudeGuard = pNext->xspr.dudeGuard;
        pSpr->xspr.dudeDeaf = pNext->xspr.dudeDeaf;
        pSpr->xspr.dudeAmbush = pNext->xspr.dudeAmbush;
        pSpr->xspr.dudeFlag4 = pNext->xspr.dudeFlag4;
        pSpr->xspr.modernFlags = pNext->xspr.modernFlags;

        pSpr->xspr.dropMsg = pNext->xspr.dropMsg;
        pSpr->xspr.key = pNext->xspr.key;

        pSpr->xspr.Decoupled = pNext->xspr.Decoupled;
        pSpr->xspr.locked = pNext->xspr.locked;

        // set health
        pSpr->xspr.health = nnExtDudeStartHealth(pSpr, pSpr->xspr.data4);

        // restore values for incarnation
        pNext->xspr.triggerOn = triggerOn;
        pNext->xspr.triggerOff = triggerOff;
    }
    else
    {
        int nNextDudeType = pDude->NextDudeType;

        // v2 morphing
        if (nNextDudeType > 0)
        {
            // morph to another custom dude
            pSpr->xspr.data1 = nNextDudeType - 1;
        }
        else if (nNextDudeType < 0)
        {
            // morph to some vanilla dude
            pSpr->ChangeType(-nNextDudeType - 1);
            pSpr->clipdist  = getDudeInfo(pSpr)->clipdist * CLIPDIST_SCALE;
            pSpr->xspr.data1    = 0;
        }

        pSpr->spr.inittype  = pSpr->GetType();
        pSpr->xspr.health   = nnExtDudeStartHealth(pSpr, 0);
        pSpr->xspr.data4    = pSpr->xspr.sysData2 = 0;
        pSpr->xspr.data2    = 0;
        pSpr->xspr.data3    = 0;
    }

    // clear init status
    pDude->initialized = 0;

    DBloodActor* pTarget = pSpr->xspr.target;        // save target
    aiInitSprite(pSpr);             // re-init sprite with all new settings

    switch (pSpr->GetType())
    {
        case kDudePodMother:        // fake dude
        case kDudeTentacleMother:   // fake dude
            break;
        default:
            if (pSpr->xspr.dudeFlag4) break;
            else if (nTarget) aiSetTarget(pSpr, pTarget); // try to restore target
            else aiSetTarget(pSpr, pSpr->spr.pos);
            aiActivateDude(pSpr); // finally activate it
            break;
    }
}

// get closest visible underwater sector it can fall in
void enterBurnSearchWater(DBloodActor* pSpr)
{
    double nClosest = FLT_MAX;
    int nDist, s, e;

    auto p1 = pSpr->spr.pos.XY();
    double z1, z2;
    double x2, y2;

    // this originally edited the state function, which is unsafe.
    pSpr->chasehackflag = false;
    if (!Chance(0x8000))
    {
        pSpr->chasehackflag = true; // try follow to the target
        return;
    }

    GetActorExtents(pSpr, &z1, &z2);

    for(int i = sector.SSize() - 1; i >= 0; i--)
    {
        auto sect = &sector[i];
        if (sect->upperLink == nullptr)
            continue;

        DBloodActor* pUp = barrier_cast<DBloodActor*>(sect->upperLink);
        DBloodActor* pLow = pUp->ownerActor;
        if (pLow && IsUnderwaterSector(pLow->sector()))
        {
            for(auto& wal : sect->walls)
            {

                if (!cansee(DVector3(p1, z1), pSpr->sector(), DVector3(wal.center(), z1), sect))
                    continue;

                double sqDist = SquareDistToWall(p1.X, p1.Y, &wal);
                if (sqDist < nClosest)
                {
                    pSpr->xspr.goalAng = (wal.center() - p1).Angle();
                    nClosest = sqDist;
                }
            }
        }
    }

    if (Chance(0xB000) && pSpr->xspr.target)
    {
        DBloodActor* pTarget = pSpr->xspr.target;
        auto dv = (p1 - pTarget->spr.pos.XY());
        if (dv.LengthSquared() < nClosest)  // water sector is not closer than target
        {
            pSpr->xspr.goalAng = dv.Angle();
            pSpr->chasehackflag = true;
            return;
        }
    }
}

void cdudeDoExplosion(CUSTOMDUDE* pDude)
{
    static DVector3 nulvec;
    CUSTOMDUDE_WEAPON* pWeap = pDude->pWeapon;
    if (pWeap && pWeap->type == kCdudeWeaponKamikaze)
        weaponShotKamikaze(pDude, pWeap, pWeap->shot.offset, nulvec);
}

END_BLD_NS
#endif
