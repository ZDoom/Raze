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

#include "ns.h"	// Must come before everything else!

#include "build.h"
#include "savegamehelp.h"

#include "blood.h"

BEGIN_BLD_NS

void RecoilDude(DBloodActor* actor);

int cumulDamage[kMaxXSprites];
DUDEEXTRA gDudeExtra[kMaxXSprites];

AISTATE genIdle = {kAiStateGenIdle, 0, -1, 0, NULL, NULL, NULL, NULL };
AISTATE genRecoil = {kAiStateRecoil, 5, -1, 20, NULL, NULL, NULL, &genIdle };

const int dword_138BB0[5] = {0x2000, 0x4000, 0x8000, 0xa000, 0xe000};

bool dudeIsPlayingSeq(spritetype *pSprite, int nSeq)
{
    if (pSprite->statnum == kStatDude && pSprite->type >= kDudeBase && pSprite->type < kDudeMax)
    {
        DUDEINFO *pDudeInfo = getDudeInfo(pSprite->type);
        if (seqGetID(3, pSprite->extra) == pDudeInfo->seqStartID + nSeq && seqGetStatus(3, pSprite->extra) >= 0)
            return true;
    }
    return false;
}

void aiPlay3DSound(spritetype *pSprite, int a2, AI_SFX_PRIORITY a3, int a4)
{
    DUDEEXTRA *pDudeExtra = &gDudeExtra[pSprite->extra];
    if (a3 == AI_SFX_PRIORITY_0)
        sfxPlay3DSound(pSprite, a2, a4, 2);
    else if (a3 > pDudeExtra->prio || pDudeExtra->time <= PlayClock)
    {
        sfxKill3DSound(pSprite, -1, -1);
        sfxPlay3DSound(pSprite, a2, a4, 0);
        pDudeExtra->prio = a3;
        pDudeExtra->time = PlayClock+120;
    }
}

void aiNewState(DBloodActor* actor, AISTATE *pAIState)
{
    auto pXSprite = &actor->x();
    auto pSprite = &actor->s();
    DUDEINFO *pDudeInfo = getDudeInfo(pSprite->type);
    pXSprite->stateTimer = pAIState->stateTicks;
    pXSprite->aiState = pAIState;
    int seqStartId = pDudeInfo->seqStartID;

    if (pAIState->seqId >= 0) {
        seqStartId += pAIState->seqId;
        if (getSequence(seqStartId))
            seqSpawn(seqStartId, 3, pSprite->extra, pAIState->funcId);
    }
    
    if (pAIState->enterFunc) 
        pAIState->enterFunc(&bloodActors[pXSprite->reference]);
}

bool isImmune(spritetype* pSprite, int dmgType, int minScale) 
{

    if (dmgType >= kDmgFall && dmgType < kDmgMax && pSprite->extra >= 0 && xsprite[pSprite->extra].locked != 1) 
    {
        if (pSprite->type >= kThingBase && pSprite->type < kThingMax)
            return (thingInfo[pSprite->type - kThingBase].dmgControl[dmgType] <= minScale);
        else if (IsDudeSprite(pSprite)) 
        {
            if (IsPlayerSprite(pSprite)) return (gPlayer[pSprite->type - kDudePlayer1].godMode || gPlayer[pSprite->type - kDudePlayer1].damageControl[dmgType] <= minScale);
            else return (dudeInfo[pSprite->type - kDudeBase].damageVal[dmgType] <= minScale);
        }
    }

    return true;
}

bool CanMove(spritetype *pSprite, int a2, int nAngle, int nRange)
{
    int top, bottom;
    GetSpriteExtents(pSprite, &top, &bottom);
    int x = pSprite->x;
    int y = pSprite->y;
    int z = pSprite->z;
    HitScan(pSprite, z, CosScale16(nAngle), SinScale16(nAngle), 0, CLIPMASK0, nRange);
    int nDist = approxDist(x-gHitInfo.hitx, y-gHitInfo.hity);
    if (nDist - (pSprite->clipdist << 2) < nRange)
    {
        if (gHitInfo.hitsprite < 0 || a2 != gHitInfo.hitsprite)
            return false;
        return true;
    }
    x += MulScale(nRange, Cos(nAngle), 30);
    y += MulScale(nRange, Sin(nAngle), 30);
    int nSector = pSprite->sectnum;
    assert(nSector >= 0 && nSector < kMaxSectors);
    if (!FindSector(x, y, z, &nSector))
        return false;
    int floorZ = getflorzofslope(nSector, x, y);
    int nXSector = sector[nSector].extra;
    char Underwater = 0; char Water = 0; char Depth = 0; char Crusher = 0;
    XSECTOR* pXSector = NULL;
    if (nXSector > 0)
    {
        pXSector = &xsector[nXSector];
        if (pXSector->Underwater)
            Underwater = 1;
        if (pXSector->Depth)
            Depth = 1;
        if (sector[nSector].type == kSectorDamage || pXSector->damageType > 0)
            Crusher = 1;
    }
    int nUpper = gUpperLink[nSector];
    int nLower = gLowerLink[nSector];
    if (nUpper >= 0)
    {
        if (sprite[nUpper].type == kMarkerUpWater || sprite[nUpper].type == kMarkerUpGoo)
            Water = Depth = 1;
    }
    if (nLower >= 0)
    {
        if (sprite[nLower].type == kMarkerLowWater || sprite[nLower].type == kMarkerLowGoo)
            Depth = 1;
    }
    switch (pSprite->type) {
    case kDudeGargoyleFlesh:
    case kDudeGargoyleStone:
    case kDudeBat:
        if (pSprite->clipdist > nDist)
            return 0;
        if (Depth)
        {
            // Ouch...
            if (Depth)
                return false;
            if (Crusher)
                return false;
        }
        break;
    case kDudeBoneEel:
        if (Water)
            return false;
        if (!Underwater)
            return false;
        if (Underwater)
            return true;
        break;
    case kDudeCerberusTwoHead:
    case kDudeCerberusOneHead:
        // by NoOne: a quick fix for Cerberus spinning in E3M7-like maps, where damage sectors is used.
        // It makes ignore danger if enemy immune to N damageType. As result Cerberus start acting like
        // in Blood 1.0 so it can move normally to player. It's up to you for adding rest of enemies here as
        // i don't think it will broke something in game.
        if (!VanillaMode() && Crusher && isImmune(pSprite, pXSector->damageType, 16)) return true;
        fallthrough__;
    case kDudeZombieButcher:
    case kDudeSpiderBrown:
    case kDudeSpiderRed:
    case kDudeSpiderBlack:
    case kDudeSpiderMother:
    case kDudeHellHound:
    case kDudeRat:
    case kDudeInnocent:
        if (Crusher)
            return false;
        if (Depth || Underwater)
            return false;
        if (floorZ - bottom > 0x2000)
            return false;
        break;
    #ifdef NOONE_EXTENSIONS
    case kDudeModernCustom:
    case kDudeModernCustomBurning:
        if ((Crusher && !nnExtIsImmune(pSprite, pXSector->damageType)) || ((Water || Underwater) && !canSwim(pSprite))) return false;
        return true;
        fallthrough__;
    #endif
    case kDudeZombieAxeNormal:
    case kDudePhantasm:
    case kDudeGillBeast:
    default:
        if (Crusher)
            return false;
        if ((nXSector < 0 || (!xsector[nXSector].Underwater && !xsector[nXSector].Depth)) && floorZ - bottom > 0x2000)
            return false;
        break;
    }
    return 1;
}

void aiChooseDirection(spritetype *pSprite, XSPRITE *pXSprite, int a3)
{
    int nSprite = pSprite->index;
    assert(pSprite->type >= kDudeBase && pSprite->type < kDudeMax);
    int vc = ((a3+1024-pSprite->ang)&2047)-1024;
    int nCos = Cos(pSprite->ang);
    int nSin = Sin(pSprite->ang);
    int dx = xvel[nSprite];
    int dy = yvel[nSprite];
    int t1 = DMulScale(dx, nCos, dy, nSin, 30);
    int vsi = ((t1*15)>>12) / 2;
    int v8 = 341;
    if (vc < 0)
        v8 = -341;
    if (CanMove(pSprite, pXSprite->target, pSprite->ang+vc, vsi))
        pXSprite->goalAng = pSprite->ang+vc;
    else if (CanMove(pSprite, pXSprite->target, pSprite->ang+vc/2, vsi))
        pXSprite->goalAng = pSprite->ang+vc/2;
    else if (CanMove(pSprite, pXSprite->target, pSprite->ang-vc/2, vsi))
        pXSprite->goalAng = pSprite->ang-vc/2;
    else if (CanMove(pSprite, pXSprite->target, pSprite->ang+v8, vsi))
        pXSprite->goalAng = pSprite->ang+v8;
    else if (CanMove(pSprite, pXSprite->target, pSprite->ang, vsi))
        pXSprite->goalAng = pSprite->ang;
    else if (CanMove(pSprite, pXSprite->target, pSprite->ang-v8, vsi))
        pXSprite->goalAng = pSprite->ang-v8;
    //else if (pSprite->flags&2)
        //pXSprite->goalAng = pSprite->ang+341;
    else // Weird..
        pXSprite->goalAng = pSprite->ang+341;
    if (Chance(0x8000))
        pXSprite->dodgeDir = 1;
    else
        pXSprite->dodgeDir = -1;
    if (!CanMove(pSprite, pXSprite->target, pSprite->ang+pXSprite->dodgeDir*512, 512))
    {
        pXSprite->dodgeDir = -pXSprite->dodgeDir;
        if (!CanMove(pSprite, pXSprite->target, pSprite->ang+pXSprite->dodgeDir*512, 512))
            pXSprite->dodgeDir = 0;
    }
}

void aiMoveForward(DBloodActor* actor)
{
    auto pXSprite = &actor->x();
    auto pSprite = &actor->s();
    assert(pSprite->type >= kDudeBase && pSprite->type < kDudeMax);
    DUDEINFO *pDudeInfo = getDudeInfo(pSprite->type);
    int nAng = ((pXSprite->goalAng+1024-pSprite->ang)&2047)-1024;
    int nTurnRange = (pDudeInfo->angSpeed<<2)>>4;
    pSprite->ang = (pSprite->ang+ClipRange(nAng, -nTurnRange, nTurnRange))&2047;
    if (abs(nAng) > 341)
        return;
    actor->xvel() += MulScale(pDudeInfo->frontSpeed, Cos(pSprite->ang), 30);
    actor->yvel() += MulScale(pDudeInfo->frontSpeed, Sin(pSprite->ang), 30);
}

void aiMoveTurn(DBloodActor* actor)
{
    auto pXSprite = &actor->x();
    auto pSprite = &actor->s();
    assert(pSprite->type >= kDudeBase && pSprite->type < kDudeMax);
    DUDEINFO *pDudeInfo = getDudeInfo(pSprite->type);
    int nAng = ((pXSprite->goalAng+1024-pSprite->ang)&2047)-1024;
    int nTurnRange = (pDudeInfo->angSpeed<<2)>>4;
    pSprite->ang = (pSprite->ang+ClipRange(nAng, -nTurnRange, nTurnRange))&2047;
}

void aiMoveDodge(DBloodActor* actor)
{
    auto pXSprite = &actor->x();
    auto pSprite = &actor->s();
    assert(pSprite->type >= kDudeBase && pSprite->type < kDudeMax);
    DUDEINFO *pDudeInfo = getDudeInfo(pSprite->type);
    int nAng = ((pXSprite->goalAng+1024-pSprite->ang)&2047)-1024;
    int nTurnRange = (pDudeInfo->angSpeed<<2)>>4;
    pSprite->ang = (pSprite->ang+ClipRange(nAng, -nTurnRange, nTurnRange))&2047;
    if (pXSprite->dodgeDir)
    {
        int nCos = Cos(pSprite->ang);
        int nSin = Sin(pSprite->ang);
        int dx = actor->xvel();
        int dy = actor->yvel();
        int t1 = DMulScale(dx, nCos, dy, nSin, 30);
        int t2 = DMulScale(dx, nSin, -dy, nCos, 30);
        if (pXSprite->dodgeDir > 0)
            t2 += pDudeInfo->sideSpeed;
        else
            t2 -= pDudeInfo->sideSpeed;

        actor->xvel() = DMulScale(t1, nCos, t2, nSin, 30);
        actor->yvel() = DMulScale(t1, nSin, -t2, nCos, 30);
    }
}

void aiActivateDude(DBloodActor* actor)
{
    auto pXSprite = &actor->x();
    auto pSprite = &actor->s();
    assert(pSprite->type >= kDudeBase && pSprite->type < kDudeMax);
    if (!pXSprite->state) {
        aiChooseDirection(pSprite, pXSprite, getangle(pXSprite->targetX-pSprite->x, pXSprite->targetY-pSprite->y));
        pXSprite->state = 1;
    }
    switch (pSprite->type) {
    case kDudePhantasm:
    {
        DUDEEXTRA_at6_u1 *pDudeExtraE = &gDudeExtra[pSprite->extra].at6.u1;
        pDudeExtraE->xval2 = 0;
        pDudeExtraE->xval3 = 1;
        pDudeExtraE->xval1 = 0;
        if (pXSprite->target == -1)
            aiNewState(actor, &ghostSearch);
        else
        {
            aiPlay3DSound(pSprite, 1600, AI_SFX_PRIORITY_1, -1);
            aiNewState(actor, &ghostChase);
        }
        break;
    }
    case kDudeCultistTommy:
    case kDudeCultistShotgun:
    case kDudeCultistTesla:
    case kDudeCultistTNT:
    case kDudeCultistBeast:
    {
        DUDEEXTRA_at6_u1 *pDudeExtraE = &gDudeExtra[pSprite->extra].at6.u1;
        pDudeExtraE->xval3 = 1;
        pDudeExtraE->xval1 = 0;
        if (pXSprite->target == -1) {
            switch (pXSprite->medium) {
                case kMediumNormal:
                    aiNewState(actor, &cultistSearch);
                    if (Chance(0x8000)) {
                        if (pSprite->type == kDudeCultistTommy) aiPlay3DSound(pSprite, 4008+Random(5), AI_SFX_PRIORITY_1, -1);
                        else aiPlay3DSound(pSprite, 1008+Random(5), AI_SFX_PRIORITY_1, -1);
                    }
                    break;
                case kMediumWater:
                case kMediumGoo:
                    aiNewState(actor, &cultistSwimSearch);
                    break;
            }
        } else {
            if (Chance(0x8000)) {
                if (pSprite->type == kDudeCultistTommy) aiPlay3DSound(pSprite, 4003+Random(4), AI_SFX_PRIORITY_1, -1);
                else aiPlay3DSound(pSprite, 1003+Random(4), AI_SFX_PRIORITY_1, -1);
            }
            switch (pXSprite->medium) {
                case kMediumNormal:
                    if (pSprite->type == kDudeCultistTommy) aiNewState(actor, &fanaticChase);
                    else aiNewState(actor, &cultistChase);
                    break;
                case kMediumWater:
                case kMediumGoo:
                    aiNewState(actor, &cultistSwimChase);
                    break;
            }
        }
        break;
    }
#ifdef NOONE_EXTENSIONS
    case kDudeModernCustom:
    {
        DUDEEXTRA_at6_u1* pDudeExtraE = &gDudeExtra[pSprite->extra].at6.u1;
        pDudeExtraE->xval3 = 1;
        pDudeExtraE->xval1 = 0;
        if (pXSprite->target == -1) {
            if (spriteIsUnderwater(pSprite, false))  aiGenDudeNewState(pSprite, &genDudeSearchW);
            else aiGenDudeNewState(pSprite, &genDudeSearchL);
        } else {
            if (Chance(0x4000)) playGenDudeSound(pSprite, kGenDudeSndTargetSpot);
            if (spriteIsUnderwater(pSprite, false)) aiGenDudeNewState(pSprite, &genDudeChaseW);
            else aiGenDudeNewState(pSprite, &genDudeChaseL);
        }
        break;
    }
    case kDudeModernCustomBurning:
        if (pXSprite->target == -1) aiGenDudeNewState(pSprite, &genDudeBurnSearch);
        else aiGenDudeNewState(pSprite, &genDudeBurnChase);
    break;
#endif
    case kDudeCultistTommyProne: {
        DUDEEXTRA_at6_u1 *pDudeExtraE = &gDudeExtra[pSprite->extra].at6.u1;
        pDudeExtraE->xval3 = 1; pDudeExtraE->xval1 = 0;
        pSprite->type = kDudeCultistTommy;
        if (pXSprite->target == -1) {
            switch (pXSprite->medium) {
                case 0:
                    aiNewState(actor, &cultistSearch);
                    if (Chance(0x8000))
                        aiPlay3DSound(pSprite, 4008+Random(5), AI_SFX_PRIORITY_1, -1);
                    break;
                case kMediumWater:
                case kMediumGoo:
                    aiNewState(actor, &cultistSwimSearch);
                    break;
            }
        } else {
            if (Chance(0x8000))
                aiPlay3DSound(pSprite, 4008+Random(5), AI_SFX_PRIORITY_1, -1);
            
            switch (pXSprite->medium) {
                case kMediumNormal:
                    aiNewState(actor, &cultistProneChase);
                    break;
                case kMediumWater:
                case kMediumGoo:
                    aiNewState(actor, &cultistSwimChase);
                    break;
            }
        }
        break;
    }
    case kDudeCultistShotgunProne:
    {
        DUDEEXTRA_at6_u1 *pDudeExtraE = &gDudeExtra[pSprite->extra].at6.u1;
        pDudeExtraE->xval3 = 1;
        pDudeExtraE->xval1 = 0;
        pSprite->type = kDudeCultistShotgun;
        if (pXSprite->target == -1)
        {
            switch (pXSprite->medium)
            {
            case kMediumNormal:
                aiNewState(actor, &cultistSearch);
                if (Chance(0x8000))
                    aiPlay3DSound(pSprite, 1008+Random(5), AI_SFX_PRIORITY_1, -1);
                break;
            case kMediumWater:
            case kMediumGoo:
                aiNewState(actor, &cultistSwimSearch);
                break;
            }
        }
        else
        {
            if (Chance(0x8000))
                aiPlay3DSound(pSprite, 1003+Random(4), AI_SFX_PRIORITY_1, -1);
            switch (pXSprite->medium)
            {
            case kMediumNormal:
                aiNewState(actor, &cultistProneChase);
                break;
            case kMediumWater:
            case kMediumGoo:
                aiNewState(actor, &cultistSwimChase);
                break;
            }
        }
        break;
    }
    case kDudeBurningCultist:
        if (pXSprite->target == -1)
            aiNewState(actor, &cultistBurnSearch);
        else
            aiNewState(actor, &cultistBurnChase);
        break;
    case kDudeBat:
    {
        DUDEEXTRA_at6_u1 *pDudeExtraE = &gDudeExtra[pSprite->extra].at6.u1;
        pDudeExtraE->xval2 = 0;
        pDudeExtraE->xval3 = 1;
        pDudeExtraE->xval1 = 0;
        if (!pSprite->flags)
            pSprite->flags = 9;
        if (pXSprite->target == -1)
            aiNewState(actor, &batSearch);
        else
        {
            if (Chance(0xa000))
                aiPlay3DSound(pSprite, 2000, AI_SFX_PRIORITY_1, -1);
            aiNewState(actor, &batChase);
        }
        break;
    }
    case kDudeBoneEel:
    {
        DUDEEXTRA_at6_u1 *pDudeExtraE = &gDudeExtra[pSprite->extra].at6.u1;
        pDudeExtraE->xval2 = 0;
        pDudeExtraE->xval3 = 1;
        pDudeExtraE->xval1 = 0;
        if (pXSprite->target == -1)
            aiNewState(actor, &eelSearch);
        else
        {
            if (Chance(0x8000))
                aiPlay3DSound(pSprite, 1501, AI_SFX_PRIORITY_1, -1);
            else
                aiPlay3DSound(pSprite, 1500, AI_SFX_PRIORITY_1, -1);
            aiNewState(actor, &eelChase);
        }
        break;
    }
    case kDudeGillBeast: {
        DUDEEXTRA_at6_u1 *pDudeExtraE = &gDudeExtra[pSprite->extra].at6.u1;
        XSECTOR *pXSector = NULL;
        if (sector[pSprite->sectnum].extra > 0)
            pXSector = &xsector[sector[pSprite->sectnum].extra];
        pDudeExtraE->xval1 = 0;
        pDudeExtraE->xval2 = 0;
        pDudeExtraE->xval3 = 1;
        if (pXSprite->target == -1)
        {
            if (pXSector && pXSector->Underwater)
                aiNewState(actor, &gillBeastSwimSearch);
            else
                aiNewState(actor, &gillBeastSearch);
        }
        else
        {
            if (Chance(0x4000))
                aiPlay3DSound(pSprite, 1701, AI_SFX_PRIORITY_1, -1);
            else
                aiPlay3DSound(pSprite, 1700, AI_SFX_PRIORITY_1, -1);
            if (pXSector && pXSector->Underwater)
                aiNewState(actor, &gillBeastSwimChase);
            else
                aiNewState(actor, &gillBeastChase);
        }
        break;
    }
    case kDudeZombieAxeNormal: {
        DUDEEXTRA_at6_u2 *pDudeExtraE = &gDudeExtra[pSprite->extra].at6.u2;
        pDudeExtraE->xval2 = 1;
        pDudeExtraE->xval1 = 0;
        if (pXSprite->target == -1)
            aiNewState(actor, &zombieASearch);
        else
        {
            if (Chance(0xa000))
            {
                switch (Random(3))
                {
                default:
                case 0:
                case 3:
                    aiPlay3DSound(pSprite, 1103, AI_SFX_PRIORITY_1, -1);
                    break;
                case 1:
                    aiPlay3DSound(pSprite, 1104, AI_SFX_PRIORITY_1, -1);
                    break;
                case 2:
                    aiPlay3DSound(pSprite, 1105, AI_SFX_PRIORITY_1, -1);
                    break;
                }
            }
            aiNewState(actor, &zombieAChase);
        }
        break;
    }
    case kDudeZombieAxeBuried:
    {
        DUDEEXTRA_at6_u2 *pDudeExtraE = &gDudeExtra[pSprite->extra].at6.u2;
        pDudeExtraE->xval2 = 1;
        pDudeExtraE->xval1 = 0;
        if (pXSprite->aiState == &zombieEIdle)
            aiNewState(actor, &zombieEUp);
        break;
    }
    case kDudeZombieAxeLaying:
    {
        DUDEEXTRA_at6_u2 *pDudeExtraE = &gDudeExtra[pSprite->extra].at6.u2;
        pDudeExtraE->xval2 = 1;
        pDudeExtraE->xval1 = 0;
        if (pXSprite->aiState == &zombieSIdle)
            aiNewState(actor, &zombie13AC2C);
        break;
    }
    case kDudeZombieButcher: {
        DUDEEXTRA_at6_u2 *pDudeExtraE = &gDudeExtra[pSprite->extra].at6.u2;
        pDudeExtraE->xval2 = 1;
        pDudeExtraE->xval1 = 0;
        if (pXSprite->target == -1)
            aiNewState(actor, &zombieFSearch);
        else
        {
            if (Chance(0x4000))
                aiPlay3DSound(pSprite, 1201, AI_SFX_PRIORITY_1, -1);
            else
                aiPlay3DSound(pSprite, 1200, AI_SFX_PRIORITY_1, -1);
            aiNewState(actor, &zombieFChase);
        }
        break;
    }
    case kDudeBurningZombieAxe:
        if (pXSprite->target == -1)
            aiNewState(actor, &zombieABurnSearch);
        else
            aiNewState(actor, &zombieABurnChase);
        break;
    case kDudeBurningZombieButcher:
        if (pXSprite->target == -1)
            aiNewState(actor, &zombieFBurnSearch);
        else
            aiNewState(actor, &zombieFBurnChase);
        break;
    case kDudeGargoyleFlesh: {
        DUDEEXTRA_at6_u1 *pDudeExtraE = &gDudeExtra[pSprite->extra].at6.u1;
        pDudeExtraE->xval2 = 0;
        pDudeExtraE->xval3 = 1;
        pDudeExtraE->xval1 = 0;
        if (pXSprite->target == -1)
            aiNewState(actor, &gargoyleFSearch);
        else
        {
            if (Chance(0x4000))
                aiPlay3DSound(pSprite, 1401, AI_SFX_PRIORITY_1, -1);
            else
                aiPlay3DSound(pSprite, 1400, AI_SFX_PRIORITY_1, -1);
            aiNewState(actor, &gargoyleFChase);
        }
        break;
    }
    case kDudeGargoyleStone:
    {
        DUDEEXTRA_at6_u1 *pDudeExtraE = &gDudeExtra[pSprite->extra].at6.u1;
        pDudeExtraE->xval2 = 0;
        pDudeExtraE->xval3 = 1;
        pDudeExtraE->xval1 = 0;
        if (pXSprite->target == -1)
            aiNewState(actor, &gargoyleFSearch);
        else
        {
            if (Chance(0x4000))
                aiPlay3DSound(pSprite, 1451, AI_SFX_PRIORITY_1, -1);
            else
                aiPlay3DSound(pSprite, 1450, AI_SFX_PRIORITY_1, -1);
            aiNewState(actor, &gargoyleFChase);
        }
        break;
    }
    case kDudeGargoyleStatueFlesh:
    case kDudeGargoyleStatueStone:
        
        #ifdef NOONE_EXTENSIONS
        // play gargoyle statue breaking animation if data1 = 1.
        if (gModernMap && pXSprite->data1 == 1) {
            if (pSprite->type == kDudeGargoyleStatueFlesh) aiNewState(actor, &statueFBreakSEQ);
            else aiNewState(actor, &statueSBreakSEQ);
        } else {
            if (Chance(0x4000)) aiPlay3DSound(pSprite, 1401, AI_SFX_PRIORITY_1, -1);
            else aiPlay3DSound(pSprite, 1400, AI_SFX_PRIORITY_1, -1);
            
            if (pSprite->type == kDudeGargoyleStatueFlesh) aiNewState(actor, &gargoyleFMorph);
            else aiNewState(actor, &gargoyleSMorph);
        }
        #else
        if (Chance(0x4000)) aiPlay3DSound(pSprite, 1401, AI_SFX_PRIORITY_1, -1);
        else aiPlay3DSound(pSprite, 1400, AI_SFX_PRIORITY_1, -1);

        if (pSprite->type == kDudeGargoyleStatueFlesh) aiNewState(actor, &gargoyleFMorph);
        else aiNewState(actor, &gargoyleSMorph);
        #endif
        break;
    case kDudeCerberusTwoHead:
        if (pXSprite->target == -1)
            aiNewState(actor, &cerberusSearch);
        else
        {
            aiPlay3DSound(pSprite, 2300, AI_SFX_PRIORITY_1, -1);
            aiNewState(actor, &cerberusChase);
        }
        break;
    case kDudeCerberusOneHead:
        if (pXSprite->target == -1)
            aiNewState(actor, &cerberus2Search);
        else
        {
            aiPlay3DSound(pSprite, 2300, AI_SFX_PRIORITY_1, -1);
            aiNewState(actor, &cerberus2Chase);
        }
        break;
    case kDudeHellHound:
        if (pXSprite->target == -1)
            aiNewState(actor, &houndSearch);
        else
        {
            aiPlay3DSound(pSprite, 1300, AI_SFX_PRIORITY_1, -1);
            aiNewState(actor, &houndChase);
        }
        break;
    case kDudeHand:
        if (pXSprite->target == -1)
            aiNewState(actor, &handSearch);
        else
        {
            aiPlay3DSound(pSprite, 1900, AI_SFX_PRIORITY_1, -1);
            aiNewState(actor, &handChase);
        }
        break;
    case kDudeRat:
        if (pXSprite->target == -1)
            aiNewState(actor, &ratSearch);
        else
        {
            aiPlay3DSound(pSprite, 2100, AI_SFX_PRIORITY_1, -1);
            aiNewState(actor, &ratChase);
        }
        break;
    case kDudeInnocent:
        if (pXSprite->target == -1)
            aiNewState(actor, &innocentSearch);
        else
        {
            if (pXSprite->health > 0)
                aiPlay3DSound(pSprite, 7000+Random(6), AI_SFX_PRIORITY_1, -1);
            aiNewState(actor, &innocentChase);
        }
        break;
    case kDudeTchernobog:
        if (pXSprite->target == -1)
            aiNewState(actor, &tchernobogSearch);
        else
        {
            aiPlay3DSound(pSprite, 2350+Random(7), AI_SFX_PRIORITY_1, -1);
            aiNewState(actor, &tchernobogChase);
        }
        break;
    case kDudeSpiderBrown:
    case kDudeSpiderRed:
    case kDudeSpiderBlack:
        pSprite->flags |= 2;
        pSprite->cstat &= ~8;
        if (pXSprite->target == -1)
            aiNewState(actor, &spidSearch);
        else
        {
            aiPlay3DSound(pSprite, 1800, AI_SFX_PRIORITY_1, -1);
            aiNewState(actor, &spidChase);
        }
        break;
    case kDudeSpiderMother: {
        DUDEEXTRA_at6_u1 *pDudeExtraE = &gDudeExtra[pSprite->extra].at6.u1;
        pDudeExtraE->xval3 = 1;
        pDudeExtraE->xval1 = 0;
        pSprite->flags |= 2;
        pSprite->cstat &= ~8;
        if (pXSprite->target == -1) 
            aiNewState(actor, &spidSearch);
        else
        {
            aiPlay3DSound(pSprite, 1853+Random(1), AI_SFX_PRIORITY_1, -1);
            aiNewState(actor, &spidChase);
        }
        break;
    }
    case kDudeTinyCaleb:
    {
        DUDEEXTRA_at6_u2 *pDudeExtraE = &gDudeExtra[pSprite->extra].at6.u2;
        pDudeExtraE->xval2 = 1;
        pDudeExtraE->xval1 = 0;
        if (pXSprite->target == -1)
        {
            switch (pXSprite->medium)
            {
            case kMediumNormal:
                aiNewState(actor, &tinycalebSearch);
                break;
            case kMediumWater:
            case kMediumGoo:
                aiNewState(actor, &tinycalebSwimSearch);
                break;
            }
        }
        else
        {
            switch (pXSprite->medium)
            {
            case kMediumNormal:
                aiNewState(actor, &tinycalebChase);
                break;
            case kMediumWater:
            case kMediumGoo:
                aiNewState(actor, &tinycalebSwimChase);
                break;
            }
        }
        break;
    }
    case kDudeBeast:
    {
        DUDEEXTRA_at6_u2 *pDudeExtraE = &gDudeExtra[pSprite->extra].at6.u2;
        pDudeExtraE->xval2 = 1;
        pDudeExtraE->xval1 = 0;
        if (pXSprite->target == -1)
        {
            switch (pXSprite->medium)
            {
            case kMediumNormal:
                aiNewState(actor, &beastSearch);
                break;
            case kMediumWater:
            case kMediumGoo:
                aiNewState(actor, &beastSwimSearch);
                break;
            }
        }
        else
        {
            aiPlay3DSound(pSprite, 9009+Random(2), AI_SFX_PRIORITY_1, -1);
            switch (pXSprite->medium)
            {
            case kMediumNormal:
                aiNewState(actor, &beastChase);
                break;
            case kMediumWater:
            case kMediumGoo:
                aiNewState(actor, &beastSwimChase);
                break;
            }
        }
        break;
    }
    case kDudePodGreen:
    case kDudePodFire:
        if (pXSprite->target == -1)
            aiNewState(actor, &podSearch);
        else
        {
            if (pSprite->type == kDudePodFire)
                aiPlay3DSound(pSprite, 2453, AI_SFX_PRIORITY_1, -1);
            else
                aiPlay3DSound(pSprite, 2473, AI_SFX_PRIORITY_1, -1);
            aiNewState(actor, &podChase);
        }
        break;
    case kDudeTentacleGreen:
    case kDudeTentacleFire:
        if (pXSprite->target == -1)
            aiNewState(actor, &tentacleSearch);
        else
        {
            aiPlay3DSound(pSprite, 2503, AI_SFX_PRIORITY_1, -1);
            aiNewState(actor, &tentacleChase);
        }
        break;
    }
}

void aiSetTarget(XSPRITE *pXSprite, int x, int y, int z)
{
    pXSprite->target = -1;
    pXSprite->targetX = x;
    pXSprite->targetY = y;
    pXSprite->targetZ = z;
}

void aiSetTarget(XSPRITE *pXSprite, int nTarget)
{
    assert(nTarget >= 0 && nTarget < kMaxSprites);
    spritetype *pTarget = &sprite[nTarget];
    if (pTarget->type >= kDudeBase && pTarget->type < kDudeMax)
    {
        if (sprite[pXSprite->reference].owner != nTarget)
        {
            pXSprite->target = nTarget;
            DUDEINFO *pDudeInfo = getDudeInfo(pTarget->type);
            pXSprite->targetX = pTarget->x;
            pXSprite->targetY = pTarget->y;
            pXSprite->targetZ = pTarget->z-((pDudeInfo->eyeHeight*pTarget->yrepeat)<<2);
        }
    }
}


int aiDamageSprite(DBloodActor* source, DBloodActor* actor, DAMAGE_TYPE nDmgType, int nDamage)
{
    auto pSprite = &actor->s();
    XSPRITE* pXSprite = &actor->x();

    if (!pXSprite->health)
        return 0;
    pXSprite->health = ClipLow(pXSprite->health - nDamage, 0);
    cumulDamage[pSprite->extra] += nDamage;
    DUDEINFO *pDudeInfo = getDudeInfo(pSprite->type);
    int nSprite = pXSprite->reference;
    if (source)
    {
        spritetype *pSource = &source->s();
        int nSource = pSource->index;
        if (pSprite == pSource) return 0;
        else if (pXSprite->target == -1 || (nSource != pXSprite->target && Chance(pSprite->type == pSource->type ? nDamage*pDudeInfo->changeTargetKin : nDamage*pDudeInfo->changeTarget)))
        {
            aiSetTarget(pXSprite, nSource);
            aiActivateDude(&bloodActors[pXSprite->reference]);
        }

        #ifdef NOONE_EXTENSIONS
        if (gModernMap) {
            
            // for enemies in patrol mode
            if (aiInPatrolState(pXSprite->aiState)) {

                aiPatrolStop(pSprite, pSource->index, pXSprite->dudeAmbush);

                PLAYER* pPlayer = getPlayerById(pSource->type);
                if (!pPlayer) return nDamage;
                if (powerupCheck(pPlayer, kPwUpShadowCloak)) pPlayer->pwUpTime[kPwUpShadowCloak] = 0;
                if (readyForCrit(pSource, pSprite)) {
                    nDamage += aiDamageSprite(actor, source, nDmgType, nDamage * (10 - gGameOptions.nDifficulty));
                    if (pXSprite->health > 0) {
                        int fullHp = (pXSprite->sysData2 > 0) ? ClipRange(pXSprite->sysData2 << 4, 1, 65535) : getDudeInfo(pSprite->type)->startHealth << 4;
                        if (((100 * pXSprite->health) / fullHp) <= 75) {
                            cumulDamage[pSprite->extra] += nDamage << 4; // to be sure any enemy will play the recoil animation
                            RecoilDude(&bloodActors[pXSprite->reference]);
                        }
                    }

                    DPrintf(DMSG_SPAMMY, "Player #%d does the critical damage to patrol dude #%d!", pPlayer->nPlayer + 1, pSprite->index);
                }

                return nDamage;
            }

            if (pSprite->type == kDudeModernCustomBurning) {

                if (Chance(0x2000) && gDudeExtra[pSprite->extra].time < PlayClock) {
                    playGenDudeSound(pSprite, kGenDudeSndBurning);
                    gDudeExtra[pSprite->extra].time = PlayClock + 360;
                }

                if (pXSprite->burnTime == 0) pXSprite->burnTime = 2400;
                if (spriteIsUnderwater(pSprite, false)) {
                    pSprite->type = kDudeModernCustom;
                    pXSprite->burnTime = 0;
                    pXSprite->health = 1; // so it can be killed with flame weapons while underwater and if already was burning dude before.
                    aiGenDudeNewState(pSprite, &genDudeGotoW);
                }
                
                return nDamage;

            }

            if (pSprite->type == kDudeModernCustom) {

                GENDUDEEXTRA* pExtra = genDudeExtra(pSprite);
                if (nDmgType == kDamageBurn) {

                    if (pXSprite->health > (uint32_t)pDudeInfo->fleeHealth) return nDamage;
                    else if (pXSprite->txID <= 0 || getNextIncarnation(pXSprite) == NULL) {
                        removeDudeStuff(pSprite);

                        if (pExtra->weaponType == kGenDudeWeaponKamikaze)
                            doExplosion(pSprite, pXSprite->data1 - kTrapExploder);

                        if (spriteIsUnderwater(pSprite)) {
                            pXSprite->health = 0;
                            return nDamage;
                        }

                        if (pXSprite->burnTime <= 0)
                            pXSprite->burnTime = 1200;

                        if (pExtra->canBurn && pExtra->availDeaths[kDamageBurn] > 0) {

                            aiPlay3DSound(pSprite, 361, AI_SFX_PRIORITY_0, -1);
                            playGenDudeSound(pSprite, kGenDudeSndBurning);
                            pSprite->type = kDudeModernCustomBurning;

                            if (pXSprite->data2 == kGenDudeDefaultSeq) // don't inherit palette for burning if using default animation
                                pSprite->pal = 0;

                            aiGenDudeNewState(pSprite, &genDudeBurnGoto);
                            actHealDude(pXSprite, dudeInfo[55].startHealth, dudeInfo[55].startHealth);
                            gDudeExtra[pSprite->extra].time = PlayClock + 360;
                            evKill(nSprite, 3, kCallbackFXFlameLick);

                        }

                    } else {
                        actKillDude(nSource, pSprite, kDamageFall, 65535);
                    }

                } else if (canWalk(pSprite) && !inDodge(pXSprite->aiState) && !inRecoil(pXSprite->aiState)) {

                    if (!dudeIsMelee(pXSprite)) {
                        if (inIdle(pXSprite->aiState) || Chance(getDodgeChance(pSprite))) {
                            if (!spriteIsUnderwater(pSprite)) {
                                if (!canDuck(pSprite) || !dudeIsPlayingSeq(pSprite, 14))  aiGenDudeNewState(pSprite, &genDudeDodgeShortL);
                                else aiGenDudeNewState(pSprite, &genDudeDodgeShortD);

                                if (Chance(0x0200))
                                    playGenDudeSound(pSprite, kGenDudeSndGotHit);

                            } else if (dudeIsPlayingSeq(pSprite, 13)) {
                                aiGenDudeNewState(pSprite, &genDudeDodgeShortW);
                            }
                        }
                    } else if (Chance(0x0200)) {
                        playGenDudeSound(pSprite, kGenDudeSndGotHit);
                    }

                }
                
                return nDamage;

            }
        }
        #endif

        if (nDmgType == kDamageTesla)
        {
            DUDEEXTRA *pDudeExtra = &gDudeExtra[pSprite->extra];
            pDudeExtra->recoil = 1;
        }
        switch (pSprite->type)
        {
        case kDudeCultistTommy:
        case kDudeCultistShotgun:
        case kDudeCultistTesla:
        case kDudeCultistTNT:
            if (nDmgType != kDamageBurn)
            {
                if (!dudeIsPlayingSeq(pSprite, 14) && !pXSprite->medium)
                    aiNewState(actor, &cultistDodge);
                else if (dudeIsPlayingSeq(pSprite, 14) && !pXSprite->medium)
                    aiNewState(actor, &cultistProneDodge);
                else if (dudeIsPlayingSeq(pSprite, 13) && (pXSprite->medium == kMediumWater || pXSprite->medium == kMediumGoo))
                    aiNewState(actor, &cultistSwimDodge);
            }
            else if (nDmgType == kDamageBurn && pXSprite->health <= (unsigned int)pDudeInfo->fleeHealth/* && (pXSprite->at17_6 != 1 || pXSprite->at17_6 != 2)*/)
            {
                pSprite->type = kDudeBurningCultist;
                aiNewState(actor, &cultistBurnGoto);
                aiPlay3DSound(pSprite, 361, AI_SFX_PRIORITY_0, -1);
                aiPlay3DSound(pSprite, 1031+Random(2), AI_SFX_PRIORITY_2, -1);
                gDudeExtra[pSprite->extra].time = PlayClock+360;
                actHealDude(pXSprite, dudeInfo[40].startHealth, dudeInfo[40].startHealth);
                evKill(nSprite, 3, kCallbackFXFlameLick);
            }
            break;
        case kDudeInnocent:
            if (nDmgType == kDamageBurn && pXSprite->health <= (unsigned int)pDudeInfo->fleeHealth/* && (pXSprite->at17_6 != 1 || pXSprite->at17_6 != 2)*/)
            {
                pSprite->type = kDudeBurningInnocent;
                aiNewState(actor, &cultistBurnGoto);
                aiPlay3DSound(pSprite, 361, AI_SFX_PRIORITY_0, -1);
                gDudeExtra[pSprite->extra].time = PlayClock+360;
                actHealDude(pXSprite, dudeInfo[39].startHealth, dudeInfo[39].startHealth);
                evKill(nSprite, 3, kCallbackFXFlameLick);
            }
            break;
        case kDudeBurningCultist:
            if (Chance(0x4000) && gDudeExtra[pSprite->extra].time < PlayClock)
            {
                aiPlay3DSound(pSprite, 1031+Random(2), AI_SFX_PRIORITY_2, -1);
                gDudeExtra[pSprite->extra].time = PlayClock+360;
            }
            if (Chance(0x600) && (pXSprite->medium == kMediumWater || pXSprite->medium == kMediumGoo))
            {
                pSprite->type = kDudeCultistTommy;
                pXSprite->burnTime = 0;
                aiNewState(actor, &cultistSwimGoto);
            }
            else if (pXSprite->medium == kMediumWater || pXSprite->medium == kMediumGoo)
            {
                pSprite->type = kDudeCultistShotgun;
                pXSprite->burnTime = 0;
                aiNewState(actor, &cultistSwimGoto);
            }
            break;
        case kDudeGargoyleFlesh:
            aiNewState(actor, &gargoyleFChase);
            break;
        case kDudeZombieButcher:
            if (nDmgType == kDamageBurn && pXSprite->health <= (unsigned int)pDudeInfo->fleeHealth) {
                aiPlay3DSound(pSprite, 361, AI_SFX_PRIORITY_0, -1);
                aiPlay3DSound(pSprite, 1202, AI_SFX_PRIORITY_2, -1);
                pSprite->type = kDudeBurningZombieButcher;
                aiNewState(actor, &zombieFBurnGoto);
                actHealDude(pXSprite, dudeInfo[42].startHealth, dudeInfo[42].startHealth);
                evKill(nSprite, 3, kCallbackFXFlameLick);
            }
            break;
        case kDudeTinyCaleb:
            if (nDmgType == kDamageBurn && pXSprite->health <= (unsigned int)pDudeInfo->fleeHealth/* && (pXSprite->at17_6 != 1 || pXSprite->at17_6 != 2)*/)
            {
                pSprite->type = kDudeBurningInnocent;
                aiNewState(actor, &cultistBurnGoto);
                aiPlay3DSound(pSprite, 361, AI_SFX_PRIORITY_0, -1);
                gDudeExtra[pSprite->extra].time = PlayClock+360;
                actHealDude(pXSprite, dudeInfo[39].startHealth, dudeInfo[39].startHealth);
                evKill(nSprite, 3, kCallbackFXFlameLick);
            }
            break;
        case kDudeCultistBeast:
            if (pXSprite->health <= (unsigned int)pDudeInfo->fleeHealth)
            {
                pSprite->type = kDudeBeast;
                aiPlay3DSound(pSprite, 9008, AI_SFX_PRIORITY_1, -1);
                aiNewState(actor, &beastMorphFromCultist);
                actHealDude(pXSprite, dudeInfo[51].startHealth, dudeInfo[51].startHealth);
            }
            break;
        case kDudeZombieAxeNormal:
        case kDudeZombieAxeBuried:
            if (nDmgType == kDamageBurn && pXSprite->health <= (unsigned int)pDudeInfo->fleeHealth)
            {
                aiPlay3DSound(pSprite, 361, AI_SFX_PRIORITY_0, -1);
                aiPlay3DSound(pSprite, 1106, AI_SFX_PRIORITY_2, -1);
                pSprite->type = kDudeBurningZombieAxe;
                aiNewState(actor, &zombieABurnGoto);
                actHealDude(pXSprite, dudeInfo[41].startHealth, dudeInfo[41].startHealth);
                evKill(nSprite, 3, kCallbackFXFlameLick);
            }
            break;
        }
    }
    return nDamage;
}

void RecoilDude(DBloodActor* actor)
{
    auto pXSprite = &actor->x();
    auto pSprite = &actor->s();
    char v4 = Chance(0x8000);
    DUDEEXTRA *pDudeExtra = &gDudeExtra[pSprite->extra];
    if (pSprite->statnum == kStatDude && (pSprite->type >= kDudeBase && pSprite->type < kDudeMax)) {
        DUDEINFO *pDudeInfo = getDudeInfo(pSprite->type);
        switch (pSprite->type) {
#ifdef NOONE_EXTENSIONS
        case kDudeModernCustom: {
            GENDUDEEXTRA* pExtra = genDudeExtra(pSprite); int rChance = getRecoilChance(pSprite);
            if (pExtra->canElectrocute && pDudeExtra->recoil && !spriteIsUnderwater(pSprite, false)) {
                
                if (Chance(rChance << 3) || (dudeIsMelee(pXSprite) && Chance(rChance << 4))) aiGenDudeNewState(pSprite, &genDudeRecoilTesla);
                else if (pExtra->canRecoil && Chance(rChance)) aiGenDudeNewState(pSprite, &genDudeRecoilL);
                else if (canWalk(pSprite)) {
                    
                    if (Chance(rChance >> 2)) aiGenDudeNewState(pSprite, &genDudeDodgeL);
                    else if (Chance(rChance >> 1)) aiGenDudeNewState(pSprite, &genDudeDodgeShortL);

                }

            } else if (pExtra->canRecoil && Chance(rChance)) {
                
                if (inDuck(pXSprite->aiState) && Chance(rChance >> 2)) aiGenDudeNewState(pSprite, &genDudeRecoilD);
                else if (spriteIsUnderwater(pSprite, false)) aiGenDudeNewState(pSprite, &genDudeRecoilW);
                else aiGenDudeNewState(pSprite, &genDudeRecoilL);

            }
            
            short rState = inRecoil(pXSprite->aiState);
            if (rState > 0) {
                
                if (!canWalk(pSprite)) {
                    if (rState == 1) pXSprite->aiState->nextState = &genDudeChaseNoWalkL;
                    else if (rState == 2) pXSprite->aiState->nextState = &genDudeChaseNoWalkD;
                    else pXSprite->aiState->nextState = &genDudeChaseNoWalkW;

                } else if (!dudeIsMelee(pXSprite) || Chance(rChance >> 2)) {
                    if (rState == 1) pXSprite->aiState->nextState = (Chance(rChance) ? &genDudeDodgeL : &genDudeDodgeShortL);
                    else if (rState == 2) pXSprite->aiState->nextState = (Chance(rChance) ? &genDudeDodgeD : &genDudeDodgeShortD);
                    else if (rState == 3) pXSprite->aiState->nextState = (Chance(rChance) ? &genDudeDodgeW : &genDudeDodgeShortW);

                }
                else if (rState == 1) pXSprite->aiState->nextState = &genDudeChaseL;
                else if (rState == 2) pXSprite->aiState->nextState = &genDudeChaseD;
                else pXSprite->aiState->nextState = &genDudeChaseW;

                playGenDudeSound(pSprite, kGenDudeSndGotHit);

            }

            pDudeExtra->recoil = 0;
            break;
        }
#endif
        case kDudeCultistTommy:
        case kDudeCultistShotgun:
        case kDudeCultistTesla:
        case kDudeCultistTNT:
        case kDudeCultistBeast:
            if (pSprite->type == kDudeCultistTommy) aiPlay3DSound(pSprite, 4013+Random(2), AI_SFX_PRIORITY_2, -1);
            else aiPlay3DSound(pSprite, 1013+Random(2), AI_SFX_PRIORITY_2, -1);
            
            if (!v4 && pXSprite->medium == kMediumNormal) {
                if (pDudeExtra->recoil) aiNewState(actor, &cultistTeslaRecoil);
                else aiNewState(actor, &cultistRecoil);

            } else if (v4 && pXSprite->medium == kMediumNormal) {
                if (pDudeExtra->recoil) aiNewState(actor, &cultistTeslaRecoil);
                else if (gGameOptions.nDifficulty > 0) aiNewState(actor, &cultistProneRecoil);
                else aiNewState(actor, &cultistRecoil);
            }
            else if (pXSprite->medium == kMediumWater || pXSprite->medium == kMediumGoo)
                aiNewState(actor, &cultistSwimRecoil);
            else
            {
                if (pDudeExtra->recoil)
                    aiNewState(actor, &cultistTeslaRecoil);
                else
                    aiNewState(actor, &cultistRecoil);
            }
            break;
        case kDudeBurningCultist:
            aiNewState(actor, &cultistBurnGoto);
            break;
#ifdef NOONE_EXTENSIONS
        case kDudeModernCustomBurning:
            aiGenDudeNewState(pSprite, &genDudeBurnGoto);
            break;
#endif
        case kDudeZombieButcher:
            aiPlay3DSound(pSprite, 1202, AI_SFX_PRIORITY_2, -1);
            if (pDudeExtra->recoil)
                aiNewState(actor, &zombieFTeslaRecoil);
            else
                aiNewState(actor, &zombieFRecoil);
            break;
        case kDudeZombieAxeNormal:
        case kDudeZombieAxeBuried:
            aiPlay3DSound(pSprite, 1106, AI_SFX_PRIORITY_2, -1);
            if (pDudeExtra->recoil && pXSprite->data3 > pDudeInfo->startHealth/3)
                aiNewState(actor, &zombieATeslaRecoil);
            else if (pXSprite->data3 > pDudeInfo->startHealth/3)
                aiNewState(actor, &zombieARecoil2);
            else
                aiNewState(actor, &zombieARecoil);
            break;
        case kDudeBurningZombieAxe:
            aiPlay3DSound(pSprite, 1106, AI_SFX_PRIORITY_2, -1);
            aiNewState(actor, &zombieABurnGoto);
            break;
        case kDudeBurningZombieButcher:
            aiPlay3DSound(pSprite, 1202, AI_SFX_PRIORITY_2, -1);
            aiNewState(actor, &zombieFBurnGoto);
            break;
        case kDudeGargoyleFlesh:
        case kDudeGargoyleStone:
            aiPlay3DSound(pSprite, 1402, AI_SFX_PRIORITY_2, -1);
            aiNewState(actor, &gargoyleFRecoil);
            break;
        case kDudeCerberusTwoHead:
            aiPlay3DSound(pSprite, 2302+Random(2), AI_SFX_PRIORITY_2, -1);
            if (pDudeExtra->recoil && pXSprite->data3 > pDudeInfo->startHealth/3)
                aiNewState(actor, &cerberusTeslaRecoil);
            else
                aiNewState(actor, &cerberusRecoil);
            break;
        case kDudeCerberusOneHead:
            aiPlay3DSound(pSprite, 2302+Random(2), AI_SFX_PRIORITY_2, -1);
            aiNewState(actor, &cerberus2Recoil);
            break;
        case kDudeHellHound:
            aiPlay3DSound(pSprite, 1302, AI_SFX_PRIORITY_2, -1);
            if (pDudeExtra->recoil)
                aiNewState(actor, &houndTeslaRecoil);
            else
                aiNewState(actor, &houndRecoil);
            break;
        case kDudeTchernobog:
            aiPlay3DSound(pSprite, 2370+Random(2), AI_SFX_PRIORITY_2, -1);
            aiNewState(actor, &tchernobogRecoil);
            break;
        case kDudeHand:
            aiPlay3DSound(pSprite, 1902, AI_SFX_PRIORITY_2, -1);
            aiNewState(actor, &handRecoil);
            break;
        case kDudeRat:
            aiPlay3DSound(pSprite, 2102, AI_SFX_PRIORITY_2, -1);
            aiNewState(actor, &ratRecoil);
            break;
        case kDudeBat:
            aiPlay3DSound(pSprite, 2002, AI_SFX_PRIORITY_2, -1);
            aiNewState(actor, &batRecoil);
            break;
        case kDudeBoneEel:
            aiPlay3DSound(pSprite, 1502, AI_SFX_PRIORITY_2, -1);
            aiNewState(actor, &eelRecoil);
            break;
        case kDudeGillBeast: {
            XSECTOR *pXSector = NULL;
            if (sector[pSprite->sectnum].extra > 0)
                pXSector = &xsector[sector[pSprite->sectnum].extra];
            aiPlay3DSound(pSprite, 1702, AI_SFX_PRIORITY_2, -1);
            if (pXSector && pXSector->Underwater)
                aiNewState(actor, &gillBeastSwimRecoil);
            else
                aiNewState(actor, &gillBeastRecoil);
            break;
        }
        case kDudePhantasm:
            aiPlay3DSound(pSprite, 1602, AI_SFX_PRIORITY_2, -1);
            if (pDudeExtra->recoil)
                aiNewState(actor, &ghostTeslaRecoil);
            else
                aiNewState(actor, &ghostRecoil);
            break;
        case kDudeSpiderBrown:
        case kDudeSpiderRed:
        case kDudeSpiderBlack:
            aiPlay3DSound(pSprite, 1802+Random(1), AI_SFX_PRIORITY_2, -1);
            aiNewState(actor, &spidDodge);
            break;
        case kDudeSpiderMother:
            aiPlay3DSound(pSprite, 1851+Random(1), AI_SFX_PRIORITY_2, -1);
            aiNewState(actor, &spidDodge);
            break;
        case kDudeInnocent:
            aiPlay3DSound(pSprite, 7007+Random(2), AI_SFX_PRIORITY_2, -1);
            if (pDudeExtra->recoil)
                aiNewState(actor, &innocentTeslaRecoil);
            else
                aiNewState(actor, &innocentRecoil);
            break;
        case kDudeTinyCaleb:
            if (pXSprite->medium == kMediumNormal)
            {
                if (pDudeExtra->recoil)
                    aiNewState(actor, &tinycalebTeslaRecoil);
                else
                    aiNewState(actor, &tinycalebRecoil);
            }
            else if (pXSprite->medium == kMediumWater || pXSprite->medium == kMediumGoo)
                aiNewState(actor, &tinycalebSwimRecoil);
            else
            {
                if (pDudeExtra->recoil)
                    aiNewState(actor, &tinycalebTeslaRecoil);
                else
                    aiNewState(actor, &tinycalebRecoil);
            }
            break;
        case kDudeBeast:
            aiPlay3DSound(pSprite, 9004+Random(2), AI_SFX_PRIORITY_2, -1);
            if (pXSprite->medium == kMediumNormal)
            {
                if (pDudeExtra->recoil)
                    aiNewState(actor, &beastTeslaRecoil);
                else
                    aiNewState(actor, &beastRecoil);
            }
            else if (pXSprite->medium == kMediumWater || pXSprite->medium == kMediumGoo)
                aiNewState(actor, &beastSwimRecoil);
            else
            {
                if (pDudeExtra->recoil)
                    aiNewState(actor, &beastTeslaRecoil);
                else
                    aiNewState(actor, &beastRecoil);
            }
            break;
        case kDudePodGreen:
        case kDudePodFire:
            aiNewState(actor, &podRecoil);
            break;
        case kDudeTentacleGreen:
        case kDudeTentacleFire:
            aiNewState(actor, &tentacleRecoil);
            break;
        default:
            aiNewState(actor, &genRecoil);
            break;
        }
        pDudeExtra->recoil = 0;
    }
}

void aiThinkTarget(DBloodActor* actor)
{
    auto pXSprite = &actor->x();
    auto pSprite = &actor->s();
    assert(pSprite->type >= kDudeBase && pSprite->type < kDudeMax);
    DUDEINFO *pDudeInfo = getDudeInfo(pSprite->type);
    if (Chance(pDudeInfo->alertChance))
    {
        for (int p = connecthead; p >= 0; p = connectpoint2[p])
        {
            PLAYER *pPlayer = &gPlayer[p];
            if (pSprite->owner == pPlayer->nSprite || pPlayer->pXSprite->health == 0 || powerupCheck(pPlayer, kPwUpShadowCloak) > 0)
                continue;
            int x = pPlayer->pSprite->x;
            int y = pPlayer->pSprite->y;
            int z = pPlayer->pSprite->z;
            int nSector = pPlayer->pSprite->sectnum;
            int dx = x-pSprite->x;
            int dy = y-pSprite->y;
            int nDist = approxDist(dx, dy);
            if (nDist > pDudeInfo->seeDist && nDist > pDudeInfo->hearDist)
                continue;
            if (!cansee(x, y, z, nSector, pSprite->x, pSprite->y, pSprite->z-((pDudeInfo->eyeHeight*pSprite->yrepeat)<<2), pSprite->sectnum))
                continue;
            int nDeltaAngle = ((getangle(dx,dy)+1024-pSprite->ang)&2047)-1024;
            if (nDist < pDudeInfo->seeDist && abs(nDeltaAngle) <= pDudeInfo->periphery)
            {
                aiSetTarget(pXSprite, pPlayer->nSprite);
                aiActivateDude(&bloodActors[pXSprite->reference]);
                return;
            }
            else if (nDist < pDudeInfo->hearDist)
            {
                aiSetTarget(pXSprite, x, y, z);
                aiActivateDude(&bloodActors[pXSprite->reference]);
                return;
            }
        }
    }
}

void sub_5F15C(spritetype *pSprite, XSPRITE *pXSprite)
{
    assert(pSprite->type >= kDudeBase && pSprite->type < kDudeMax);
    DUDEINFO *pDudeInfo = getDudeInfo(pSprite->type);
    if (Chance(pDudeInfo->alertChance))
    {
        for (int p = connecthead; p >= 0; p = connectpoint2[p])
        {
            PLAYER *pPlayer = &gPlayer[p];
            if (pSprite->owner == pPlayer->nSprite || pPlayer->pXSprite->health == 0 || powerupCheck(pPlayer, kPwUpShadowCloak) > 0)
                continue;
            int x = pPlayer->pSprite->x;
            int y = pPlayer->pSprite->y;
            int z = pPlayer->pSprite->z;
            int nSector = pPlayer->pSprite->sectnum;
            int dx = x-pSprite->x;
            int dy = y-pSprite->y;
            int nDist = approxDist(dx, dy);
            if (nDist > pDudeInfo->seeDist && nDist > pDudeInfo->hearDist)
                continue;
            if (!cansee(x, y, z, nSector, pSprite->x, pSprite->y, pSprite->z-((pDudeInfo->eyeHeight*pSprite->yrepeat)<<2), pSprite->sectnum))
                continue;
            int nDeltaAngle = ((getangle(dx,dy)+1024-pSprite->ang)&2047)-1024;
            if (nDist < pDudeInfo->seeDist && abs(nDeltaAngle) <= pDudeInfo->periphery)
            {
                aiSetTarget(pXSprite, pPlayer->nSprite);
                aiActivateDude(&bloodActors[pXSprite->reference]);
                return;
            }
            else if (nDist < pDudeInfo->hearDist)
            {
                aiSetTarget(pXSprite, x, y, z);
                aiActivateDude(&bloodActors[pXSprite->reference]);
                return;
            }
        }
        if (pXSprite->state)
        {
            uint8_t va4[(kMaxSectors+7)>>3];
            GetClosestSpriteSectors(pSprite->sectnum, pSprite->x, pSprite->y, 400, va4);

            int nSprite2;
            StatIterator it(kStatDude);
            while ((nSprite2 = it.NextIndex()) >= 0)
            {
                spritetype *pSprite2 = &sprite[nSprite2];
                int dx = pSprite2->x-pSprite->x;
                int dy = pSprite2->y-pSprite->y;
                int nDist = approxDist(dx, dy);
                if (pSprite2->type == kDudeInnocent)
                {
                    DUDEINFO *pDudeInfo = getDudeInfo(pSprite2->type);
                    if (nDist > pDudeInfo->seeDist && nDist > pDudeInfo->hearDist)
                        continue;
                    aiSetTarget(pXSprite, pSprite2->index);
                    aiActivateDude(&bloodActors[pXSprite->reference]);
                    return;
                }
            }
        }
    }
}

void aiProcessDudes(void) {
    int nSprite;
    StatIterator it(kStatDude);
    while ((nSprite = it.NextIndex()) >= 0)
    {
        spritetype *pSprite = &sprite[nSprite];
        if (pSprite->flags & 32) continue;
        int nXSprite = pSprite->extra;
        XSPRITE *pXSprite = &xsprite[nXSprite]; 
        DUDEINFO *pDudeInfo = getDudeInfo(pSprite->type);
        auto actor = &bloodActors[pXSprite->reference];
        if (IsPlayerSprite(pSprite) || pXSprite->health == 0) continue;
        pXSprite->stateTimer = ClipLow(pXSprite->stateTimer-4, 0);

        if (pXSprite->aiState && pXSprite->aiState->moveFunc)
            pXSprite->aiState->moveFunc(&bloodActors[pXSprite->reference]);

        if (pXSprite->aiState && pXSprite->aiState->thinkFunc && (gFrameCount & 3) == (nSprite & 3))
            pXSprite->aiState->thinkFunc(&bloodActors[pXSprite->reference]);

        switch (pSprite->type) {
            #ifdef NOONE_EXTENSIONS
            case kDudeModernCustom:
            case kDudeModernCustomBurning: {
                GENDUDEEXTRA* pExtra = &gGenDudeExtra[pSprite->index];
                if (pExtra->slaveCount > 0) updateTargetOfSlaves(pSprite);
                if (pExtra->nLifeLeech >= 0) updateTargetOfLeech(pSprite);
                if (pXSprite->stateTimer == 0 && pXSprite->aiState && pXSprite->aiState->nextState
                    && (pXSprite->aiState->stateTicks > 0 || seqGetStatus(3, pSprite->extra) < 0)) {
                    aiGenDudeNewState(pSprite, pXSprite->aiState->nextState);
                }
                int hinder = ((pExtra->isMelee) ? 25 : 5) << 4;
                if (pXSprite->health <= 0 || hinder > cumulDamage[pSprite->extra]) break;
                pXSprite->data3 = cumulDamage[pSprite->extra];
                RecoilDude(&bloodActors[pXSprite->reference]);
                break;
            }
            #endif
            default:
                if (pXSprite->stateTimer == 0 && pXSprite->aiState && pXSprite->aiState->nextState) {
                    if (pXSprite->aiState->stateTicks > 0)
                        aiNewState(actor, pXSprite->aiState->nextState);
                    else if (seqGetStatus(3, nXSprite) < 0)
                        aiNewState(actor, pXSprite->aiState->nextState);
                }

                if (pXSprite->health > 0 && ((pDudeInfo->hinderDamage << 4) <= cumulDamage[nXSprite])) {
                    pXSprite->data3 = cumulDamage[nXSprite];
                    RecoilDude(&bloodActors[pXSprite->reference]);
                }
                break;
        }
    }
    memset(cumulDamage, 0, sizeof(cumulDamage));
}

void aiInit(void)
{
    int nSprite;
    StatIterator it(kStatDude);
    while ((nSprite = it.NextIndex()) >= 0)
    {
        aiInitSprite(&sprite[nSprite]);
    }
}

void aiInitSprite(spritetype *pSprite)
{
    int nXSprite = pSprite->extra;
    XSPRITE *pXSprite = &xsprite[nXSprite];
    auto actor = &bloodActors[pXSprite->reference];
    int nSector = pSprite->sectnum;
    int nXSector = sector[nSector].extra;
    XSECTOR *pXSector = NULL;
    if (nXSector > 0)
        pXSector = &xsector[nXSector];
    DUDEEXTRA *pDudeExtra = &gDudeExtra[pSprite->extra];
    pDudeExtra->recoil = 0;
    pDudeExtra->time = 0;
    
    #ifdef NOONE_EXTENSIONS
    int stateTimer = -1, targetMarker = -1;
    int targetX = 0, targetY = 0, targetZ = 0;
    
    // dude patrol init
    if (gModernMap) {
        
        // must keep it in case of loading save
        if (pXSprite->dudeFlag4 && spriRangeIsFine(pXSprite->target) && sprite[pXSprite->target].type == kMarkerPath) {
            stateTimer = pXSprite->stateTimer; targetMarker = pXSprite->target;
            targetX = pXSprite->targetX; targetY = pXSprite->targetY;
            targetZ = pXSprite->targetZ;
        }

    }
    #endif

    switch (pSprite->type) {
    case kDudeCultistTommy:
    case kDudeCultistShotgun:
    case kDudeCultistTesla:
    case kDudeCultistTNT:
    case kDudeCultistBeast:
    {
        DUDEEXTRA_at6_u1 *pDudeExtraE = &gDudeExtra[nXSprite].at6.u1;
        pDudeExtraE->xval3 = 0;
        pDudeExtraE->xval1 = 0;
        aiNewState(actor, &cultistIdle);
        break;
    }
    case kDudeCultistTommyProne:
    {
        DUDEEXTRA_at6_u1 *pDudeExtraE = &gDudeExtra[nXSprite].at6.u1;
        pDudeExtraE->xval3 = 0;
        pDudeExtraE->xval1 = 0;
        aiNewState(actor, &fanaticProneIdle);
        break;
    }
    case kDudeCultistShotgunProne:
    {
        DUDEEXTRA_at6_u1 *pDudeExtraE = &gDudeExtra[nXSprite].at6.u1;
        pDudeExtraE->xval3 = 0;
        pDudeExtraE->xval1 = 0;
        aiNewState(actor, &cultistProneIdle);
        break;
    }
    case kDudeZombieButcher: {
        DUDEEXTRA_at6_u2 *pDudeExtraE = &gDudeExtra[nXSprite].at6.u2;
        pDudeExtraE->xval2 = 0;
        pDudeExtraE->xval1 = 0;
        aiNewState(actor, &zombieFIdle);
        break;
    }
    case kDudeZombieAxeNormal: {
        DUDEEXTRA_at6_u2 *pDudeExtraE = &gDudeExtra[nXSprite].at6.u2;
        pDudeExtraE->xval2 = 0;
        pDudeExtraE->xval1 = 0;
        aiNewState(actor, &zombieAIdle);
        break;
    }
    case kDudeZombieAxeLaying:
    {
        DUDEEXTRA_at6_u2 *pDudeExtraE = &gDudeExtra[nXSprite].at6.u2;
        pDudeExtraE->xval2 = 0;
        pDudeExtraE->xval1 = 0;
        aiNewState(actor, &zombieSIdle);
        pSprite->flags &= ~1;
        break;
    }
    case kDudeZombieAxeBuried: {
        DUDEEXTRA_at6_u2 *pDudeExtraE = &gDudeExtra[nXSprite].at6.u2;
        pDudeExtraE->xval2 = 0;
        pDudeExtraE->xval1 = 0;
        aiNewState(actor, &zombieEIdle);
        break;
    }
    case kDudeGargoyleFlesh:
    case kDudeGargoyleStone: {
        DUDEEXTRA_at6_u1 *pDudeExtraE = &gDudeExtra[nXSprite].at6.u1;
        pDudeExtraE->xval2 = 0;
        pDudeExtraE->xval3 = 0;
        pDudeExtraE->xval1 = 0;
        aiNewState(actor, &gargoyleFIdle);
        break;
    }
    case kDudeGargoyleStatueFlesh:
    case kDudeGargoyleStatueStone:
        aiNewState(actor, &gargoyleStatueIdle);
        break;
    case kDudeCerberusTwoHead: {
        DUDEEXTRA_at6_u2 *pDudeExtraE = &gDudeExtra[nXSprite].at6.u2;
        pDudeExtraE->xval2 = 0;
        pDudeExtraE->xval1 = 0;
        aiNewState(actor, &cerberusIdle);
        break;
    }
    case kDudeHellHound:
        aiNewState(actor, &houndIdle);
        break;
    case kDudeHand:
        aiNewState(actor, &handIdle);
        break;
    case kDudePhantasm:
    {
        DUDEEXTRA_at6_u1 *pDudeExtraE = &gDudeExtra[nXSprite].at6.u1;
        pDudeExtraE->xval2 = 0;
        pDudeExtraE->xval3 = 0;
        pDudeExtraE->xval1 = 0;
        aiNewState(actor, &ghostIdle);
        break;
    }
    case kDudeInnocent:
        aiNewState(actor, &innocentIdle);
        break;
    case kDudeRat:
        aiNewState(actor, &ratIdle);
        break;
    case kDudeBoneEel:
    {
        DUDEEXTRA_at6_u1 *pDudeExtraE = &gDudeExtra[nXSprite].at6.u1;
        pDudeExtraE->xval2 = 0;
        pDudeExtraE->xval3 = 0;
        pDudeExtraE->xval1 = 0;
        aiNewState(actor, &eelIdle);
        break;
    }
    case kDudeGillBeast:
        aiNewState(actor, &gillBeastIdle);
        break;
    case kDudeBat:
    {
        DUDEEXTRA_at6_u1 *pDudeExtraE = &gDudeExtra[nXSprite].at6.u1;
        pDudeExtraE->xval2 = 0;
        pDudeExtraE->xval3 = 0;
        pDudeExtraE->xval1 = 0;
        aiNewState(actor, &batIdle);
        break;
    }
    case kDudeSpiderBrown:
    case kDudeSpiderRed:
    case kDudeSpiderBlack:
    {
        DUDEEXTRA_at6_u1 *pDudeExtraE = &gDudeExtra[nXSprite].at6.u1;
        pDudeExtraE->xval3 = 0;
        pDudeExtraE->xval2 = 0;
        pDudeExtraE->xval1 = 0;
        aiNewState(actor, &spidIdle);
        break;
    }
    case kDudeSpiderMother:
    {
        DUDEEXTRA_at6_u1 *pDudeExtraE = &gDudeExtra[nXSprite].at6.u1;
        pDudeExtraE->xval3 = 0;
        pDudeExtraE->xval2 = 0;
        pDudeExtraE->xval1 = 0;
        aiNewState(actor, &spidIdle);
        break;
    }
    case kDudeTchernobog:
    {
        DUDEEXTRA_at6_u2 *pDudeExtraE = &gDudeExtra[nXSprite].at6.u2;
        pDudeExtraE->xval2 = 0;
        pDudeExtraE->xval1 = 0;
        aiNewState(actor, &tchernobogIdle);
        break;
    }
    case kDudeTinyCaleb:
        aiNewState(actor, &tinycalebIdle);
        break;
    case kDudeBeast:
        aiNewState(actor, &beastIdle);
        break;
    case kDudePodGreen:
    case kDudePodFire:
        aiNewState(actor, &podIdle);
        break;
    case kDudeTentacleGreen:
    case kDudeTentacleFire:
        aiNewState(actor, &tentacleIdle);
        break;
    default:
        aiNewState(actor, &genIdle);
        break;
    }
    aiSetTarget(pXSprite, 0, 0, 0);
    pXSprite->stateTimer = 0;
    switch (pSprite->type)
    {
    case kDudeSpiderBrown:
    case kDudeSpiderRed:
    case kDudeSpiderBlack:
        if (pSprite->cstat&8) pSprite->flags |= 9;
        else pSprite->flags = 15;
        break;
    case kDudeGargoyleFlesh:
    case kDudeGargoyleStone:
    case kDudePhantasm:
    case kDudeBoneEel:
    case kDudeBat:
        pSprite->flags |= 9;
        break;
    case kDudeGillBeast:
        if (pXSector && pXSector->Underwater) pSprite->flags |= 9;
        else pSprite->flags = 15;
        break;
    case kDudeZombieAxeBuried:
    case kDudeZombieAxeLaying:
        pSprite->flags = 7;
        break;
    #ifdef NOONE_EXTENSIONS
    case kDudePodMother: // FakeDude type
        if (gModernMap) break;
        fallthrough__;
    // Allow put pods and tentacles on ceilings if sprite is y-flipped.
    case kDudePodGreen:
    case kDudeTentacleGreen:
    case kDudePodFire:
    case kDudeTentacleFire:
    case kDudeTentacleMother:
        if (gModernMap && (pSprite->cstat & CSTAT_SPRITE_YFLIP)) {
            if (!(pSprite->flags & kModernTypeFlag1)) // don't add autoaim for player if hitag 1 specified in editor.
                pSprite->flags = kHitagAutoAim;
            break;
        }
        fallthrough__;
    // go default
    #endif
    default:
        pSprite->flags = 15;
        break;
    }

    #ifdef NOONE_EXTENSIONS
    if (gModernMap) {

        if (pSprite->type == kDudeModernCustom) {
            aiGenDudeInitSprite(pSprite, pXSprite);
            genDudePrepare(pSprite, kGenDudePropertyAll);
        }

        if (pXSprite->dudeFlag4) {

            // restore dude's path
            if (spriRangeIsFine(targetMarker)) {
                pXSprite->target = targetMarker;
                pXSprite->targetX = targetX;
                pXSprite->targetY = targetY;
                pXSprite->targetZ = targetZ;
            }

            // reset target spot progress
            pXSprite->data3 = 0;

            // make dude follow the markers
            bool uwater = spriteIsUnderwater(pSprite);
            if (pXSprite->target <= 0 || sprite[pXSprite->target].type != kMarkerPath) {
                pXSprite->target = -1; aiPatrolSetMarker(pSprite, pXSprite);
            }

            if (stateTimer > 0) {
                if (uwater) aiPatrolState(pSprite, kAiStatePatrolWaitW);
                else if (pXSprite->unused1 & kDudeFlagCrouch) aiPatrolState(pSprite, kAiStatePatrolWaitC);
                else aiPatrolState(pSprite, kAiStatePatrolWaitL);
                pXSprite->stateTimer = stateTimer; // restore state timer
            }
            else if (uwater) aiPatrolState(pSprite, kAiStatePatrolMoveW);
            else if (pXSprite->unused1 & kDudeFlagCrouch) aiPatrolState(pSprite, kAiStatePatrolMoveC);
            else aiPatrolState(pSprite, kAiStatePatrolMoveL);

        }

    }
    #endif

}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

FSerializer& Serialize(FSerializer& arc, const char* keyname, DUDEEXTRA& w, DUDEEXTRA* def)
{
    int empty = 0;
    char empty2 = 0;
    if (arc.isReading()) w = {};

	if (arc.BeginObject(keyname))
	{
		arc("time", w.time, &empty)
			("recoil", w.recoil, &empty)
			("prio", w.prio, &empty)
			("x1", w.at6.u1.xval1, &empty)
			("x2", w.at6.u1.xval2, &empty)
			("x3", w.at6.u1.xval3, &empty2)
			.EndObject();
	}
	return arc;
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void SerializeAI(FSerializer& arc)
{
	if (arc.BeginObject("ai"))
	{
		arc.SparseArray("dudeextra", gDudeExtra, kMaxSprites, activeXSprites)
		    .EndObject();
	}
}


END_BLD_NS
