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
#include "build.h"
#include "pragmas.h"
#include "mmulti.h"
#include "common_game.h"
#include "actor.h"
#include "ai.h"
#include "aibat.h"
#include "aibeast.h"
#include "aiboneel.h"
#include "aiburn.h"
#include "aicaleb.h"
#include "aicerber.h"
#include "aicult.h"
#include "aigarg.h"
#include "aighost.h"
#include "aigilbst.h"
#include "aihand.h"
#include "aihound.h"
#include "aiinnoc.h"
#include "aipod.h"
#include "airat.h"
#include "aispid.h"
#include "aitchern.h"
#include "aizomba.h"
#include "aizombf.h"
#include "aiunicult.h" // By NoOne: add custom dude
#include "blood.h"
#include "db.h"
#include "dude.h"
#include "eventq.h"
#include "fx.h"
#include "gameutil.h"
#include "gib.h"
#include "globals.h"
#include "levels.h"
#include "loadsave.h"
#include "player.h"
#include "seq.h"
#include "sound.h"
#include "sfx.h"
#include "trig.h"
#include "triggers.h"

int cumulDamage[kMaxXSprites];
int gDudeSlope[kMaxXSprites];
DUDEEXTRA gDudeExtra[kMaxXSprites];

AISTATE genIdle = {kAiStateGenIdle, 0, -1, 0, NULL, NULL, NULL, NULL };
AISTATE genRecoil = {kAiStateRecoil, 5, -1, 20, NULL, NULL, NULL, &genIdle };

int dword_138BB0[5] = {0x2000, 0x4000, 0x8000, 0xa000, 0xe000};

void aiSetGenIdleState(spritetype* pSprite, XSPRITE* pXSprite) {
    aiNewState(pSprite, pXSprite, &genIdle);
}

bool sub_5BDA8(spritetype *pSprite, int nSeq)
{
    if (pSprite->statnum == 6 && pSprite->type >= kDudeBase && pSprite->type < kDudeMax)
    {
        DUDEINFO *pDudeInfo = &dudeInfo[pSprite->type-kDudeBase];
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
    else if (a3 > pDudeExtra->at5 || pDudeExtra->at0 <= gFrameClock)
    {
        sfxKill3DSound(pSprite, -1, -1);
        sfxPlay3DSound(pSprite, a2, a4, 0);
        pDudeExtra->at5 = a3;
        pDudeExtra->at0 = gFrameClock+120;
    }
}

void aiNewState(spritetype *pSprite, XSPRITE *pXSprite, AISTATE *pAIState)
{
    DUDEINFO *pDudeInfo = &dudeInfo[pSprite->type-kDudeBase];
    pXSprite->stateTimer = pAIState->at8;
    pXSprite->aiState = pAIState;
    int seqStartId = pDudeInfo->seqStartID;

    if (pAIState->at0 >= 0) {
        // By NoOne: Custom dude uses data2 to keep it's seqStartId
        switch (pSprite->type) {
            case kGDXDudeUniversalCultist:
            case kGDXGenDudeBurning:
                seqStartId = pXSprite->data2;
                break;
        }
        seqStartId += pAIState->at0;
        if (gSysRes.Lookup(seqStartId, "SEQ"))
            seqSpawn(seqStartId, 3, pSprite->extra, pAIState->at4);
    }
    
    if (pAIState->atc) pAIState->atc(pSprite, pXSprite); // entry function
}
bool dudeIsImmune(spritetype* pSprite, int dmgType) {
    if (dmgType < 0 || dmgType > 6) return true;
    else if (dudeInfo[pSprite->type - kDudeBase].startDamage[dmgType] == 0) return true;
    else if (pSprite->extra >= 0 && xsprite[pSprite->extra].locked == 1) return true;  // if dude is locked, it immune to any dmg.
    return false;
}
bool CanMove(spritetype *pSprite, int a2, int nAngle, int nRange)
{
    int top, bottom;
    GetSpriteExtents(pSprite, &top, &bottom);
    int x = pSprite->x;
    int y = pSprite->y;
    int z = pSprite->z;
    HitScan(pSprite, z, Cos(nAngle)>>16, Sin(nAngle)>>16, 0, CLIPMASK0, nRange);
    int nDist = approxDist(x-gHitInfo.hitx, y-gHitInfo.hity);
    if (nDist - (pSprite->clipdist << 2) < nRange)
    {
        if (gHitInfo.hitsprite < 0 || a2 != gHitInfo.hitsprite)
            return false;
        return true;
    }
    x += mulscale30(nRange, Cos(nAngle));
    y += mulscale30(nRange, Sin(nAngle));
    int nSector = pSprite->sectnum;
    dassert(nSector >= 0 && nSector < kMaxSectors);
    if (!FindSector(x, y, z, &nSector))
        return false;
    int floorZ = getflorzofslope(nSector, x, y);
    int UNUSED(ceilZ) = getceilzofslope(nSector, x, y);
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
        if (sector[nSector].lotag == kSecDamage || pXSector->damageType > 0) {
            // By NoOne: a quick fix for Cerberus spinning in E3M7-like maps, where damage sectors is used.
            // It makes ignore danger if enemy immune to N damageType. As result Cerberus start acting like
            // in Blood 1.0 so it can move normally to player. It's up to you for adding rest of enemies here as
            // i don't think it will broke something in game.
            switch (pSprite->type) {
                case 227: // Cerberus
                case 228: // 1 Head Cerberus
                    if (VanillaMode() || !dudeIsImmune(pSprite, pXSector->damageType))
                        Crusher = 1;
                    break;
                default:
                    Crusher = 1;
                    break;
            }
        }
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
    switch (pSprite->type)
    {
    case 206:
    case 207:
    case 219:
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
    case 218:
        if (Water)
            return false;
        if (!Underwater)
            return false;
        if (Underwater)
            return true;
        break;
    case 204:
    case 213:
    case 214:
    case 215:
    case 216:
    case 211:
    case 220:
    case 227:
    case 245:
        if (Crusher)
            return false;
        if (Depth || Underwater)
            return false;
        if (floorZ - bottom > 0x2000)
            return false;
        break;
    case 203:
    case 210:
    case 217:
    case kGDXDudeUniversalCultist:
    case kGDXGenDudeBurning:
        if ((Crusher && !dudeIsImmune(pSprite, pXSector->damageType)) || xsprite[pSprite->extra].dudeGuard) return false;
        return true;
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
    dassert(pSprite->type >= kDudeBase && pSprite->type < kDudeMax);
    int vc = ((a3+1024-pSprite->ang)&2047)-1024;
    int nCos = Cos(pSprite->ang);
    int nSin = Sin(pSprite->ang);
    int dx = xvel[nSprite];
    int dy = yvel[nSprite];
    int t1 = dmulscale30(dx, nCos, dy, nSin);
    int UNUSED(t2) = dmulscale30(dx, nSin, -dy, nCos);
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
    else if (pSprite->hitag&2)
        pXSprite->goalAng = pSprite->ang+341;
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

void aiMoveForward(spritetype *pSprite, XSPRITE *pXSprite)
{
    int nSprite = pSprite->index;
    dassert(pSprite->type >= kDudeBase && pSprite->type < kDudeMax);
    DUDEINFO *pDudeInfo = &dudeInfo[pSprite->type-kDudeBase];
    int nAng = ((pXSprite->goalAng+1024-pSprite->ang)&2047)-1024;
    int nTurnRange = (pDudeInfo->angSpeed<<2)>>4;
    pSprite->ang = (pSprite->ang+ClipRange(nAng, -nTurnRange, nTurnRange))&2047;
    if (klabs(nAng) > 341)
        return;
    xvel[nSprite] += mulscale30(pDudeInfo->frontSpeed, Cos(pSprite->ang));
    yvel[nSprite] += mulscale30(pDudeInfo->frontSpeed, Sin(pSprite->ang));
}

void aiMoveTurn(spritetype *pSprite, XSPRITE *pXSprite)
{
    dassert(pSprite->type >= kDudeBase && pSprite->type < kDudeMax);
    DUDEINFO *pDudeInfo = &dudeInfo[pSprite->type-kDudeBase];
    int nAng = ((pXSprite->goalAng+1024-pSprite->ang)&2047)-1024;
    int nTurnRange = (pDudeInfo->angSpeed<<2)>>4;
    pSprite->ang = (pSprite->ang+ClipRange(nAng, -nTurnRange, nTurnRange))&2047;
}

void aiMoveDodge(spritetype *pSprite, XSPRITE *pXSprite)
{
    int nSprite = pSprite->index;
    dassert(pSprite->type >= kDudeBase && pSprite->type < kDudeMax);
    DUDEINFO *pDudeInfo = &dudeInfo[pSprite->type-kDudeBase];
    int nAng = ((pXSprite->goalAng+1024-pSprite->ang)&2047)-1024;
    int nTurnRange = (pDudeInfo->angSpeed<<2)>>4;
    pSprite->ang = (pSprite->ang+ClipRange(nAng, -nTurnRange, nTurnRange))&2047;
    if (pXSprite->dodgeDir)
    {
        int nCos = Cos(pSprite->ang);
        int nSin = Sin(pSprite->ang);
        int dx = xvel[nSprite];
        int dy = yvel[nSprite];
        int t1 = dmulscale30(dx, nCos, dy, nSin);
        int t2 = dmulscale30(dx, nSin, -dy, nCos);
        if (pXSprite->dodgeDir > 0)
            t2 += pDudeInfo->sideSpeed;
        else
            t2 -= pDudeInfo->sideSpeed;

        xvel[nSprite] = dmulscale30(t1, nCos, t2, nSin);
        yvel[nSprite] = dmulscale30(t1, nSin, -t2, nCos);
    }
}

void aiActivateDude(spritetype *pSprite, XSPRITE *pXSprite)
{
    dassert(pSprite->type >= kDudeBase && pSprite->type < kDudeMax);
    if (!pXSprite->state)
    {
        aiChooseDirection(pSprite, pXSprite, getangle(pXSprite->targetX-pSprite->x, pXSprite->targetY-pSprite->y));
        pXSprite->state = 1;
    }
    switch (pSprite->type)
    {
    case 210:
    {
        DUDEEXTRA_at6_u1 *pDudeExtraE = &gDudeExtra[pSprite->extra].at6.u1;
        pDudeExtraE->at4 = 0;
        pDudeExtraE->at8 = 1;
        pDudeExtraE->at0 = 0;
        if (pXSprite->target == -1)
            aiNewState(pSprite, pXSprite, &ghostSearch);
        else
        {
            aiPlay3DSound(pSprite, 1600, AI_SFX_PRIORITY_1, -1);
            aiNewState(pSprite, pXSprite, &ghostChase);
        }
        break;
    }
    case 201:
    case 202:
    case 247:
    case 248:
    case 249:
    {
        DUDEEXTRA_at6_u1 *pDudeExtraE = &gDudeExtra[pSprite->extra].at6.u1;
        pDudeExtraE->at8 = 1;
        pDudeExtraE->at0 = 0;
        if (pXSprite->target == -1)
        {
            switch (pXSprite->medium)
            {
            case 0:
                aiNewState(pSprite, pXSprite, &cultistSearch);
                if (Chance(0x8000))
                {
                    if (pSprite->type == 201)
                        aiPlay3DSound(pSprite, 4008+Random(5), AI_SFX_PRIORITY_1, -1);
                    else
                        aiPlay3DSound(pSprite, 1008+Random(5), AI_SFX_PRIORITY_1, -1);
                }
                break;
            case 1:
            case 2:
                aiNewState(pSprite, pXSprite, &cultistSwimSearch);
                break;
            }
        }
        else
        {
            if (Chance(0x8000))
            {
                if (pSprite->type == 201)
                    aiPlay3DSound(pSprite, 4003+Random(4), AI_SFX_PRIORITY_1, -1);
                else
                    aiPlay3DSound(pSprite, 1003+Random(4), AI_SFX_PRIORITY_1, -1);
            }
            switch (pXSprite->medium)
            {
            case 0:
                if (pSprite->type == 201)
                    aiNewState(pSprite, pXSprite, &fanaticChase);
                else
                    aiNewState(pSprite, pXSprite, &cultistChase);
                break;
            case 1:
            case 2:
                aiNewState(pSprite, pXSprite, &cultistSwimChase);
                break;
            }
        }
        break;
    }

    case kGDXDudeUniversalCultist:
    {
        DUDEEXTRA_at6_u1* pDudeExtraE = &gDudeExtra[pSprite->extra].at6.u1;
        pDudeExtraE->at8 = 1;
        pDudeExtraE->at0 = 0;
        if (pXSprite->target == -1) {
            if (spriteIsUnderwater(pSprite, false))
                aiNewState(pSprite, pXSprite, &GDXGenDudeSearchW);
            else {
                aiNewState(pSprite, pXSprite, &GDXGenDudeSearchL);
                if (Chance(0x4000))
                    sfxPlayGDXGenDudeSound(pSprite, 0, pXSprite->data3);
            }
        }
        else {
            if (Chance(0x4000))
                sfxPlayGDXGenDudeSound(pSprite, 0, pXSprite->data3);

            if (spriteIsUnderwater(pSprite, false))
                aiNewState(pSprite, pXSprite, &GDXGenDudeChaseW);
            else
                aiNewState(pSprite, pXSprite, &GDXGenDudeChaseL);

        }
        break;
    }
    case kGDXGenDudeBurning:
        if (pXSprite->target == -1)
            aiNewState(pSprite, pXSprite, &GDXGenDudeBurnSearch);
        else
            aiNewState(pSprite, pXSprite, &GDXGenDudeBurnChase);
    break;
    case 230:
    {
        DUDEEXTRA_at6_u1 *pDudeExtraE = &gDudeExtra[pSprite->extra].at6.u1;
        pDudeExtraE->at8 = 1;
        pDudeExtraE->at0 = 0;
        pSprite->type = 201;
        if (pXSprite->target == -1)
        {
            switch (pXSprite->medium)
            {
            case 0:
                aiNewState(pSprite, pXSprite, &cultistSearch);
                if (Chance(0x8000))
                    aiPlay3DSound(pSprite, 4008+Random(5), AI_SFX_PRIORITY_1, -1);
                break;
            case 1:
            case 2:
                aiNewState(pSprite, pXSprite, &cultistSwimSearch);
                break;
            }
        }
        else
        {
            if (Chance(0x8000))
                aiPlay3DSound(pSprite, 4008+Random(5), AI_SFX_PRIORITY_1, -1);
            switch (pXSprite->medium)
            {
            case 0:
                aiNewState(pSprite, pXSprite, &cultistProneChase);
                break;
            case 1:
            case 2:
                aiNewState(pSprite, pXSprite, &cultistSwimChase);
                break;
            }
        }
        break;
    }
    case 246:
    {
        DUDEEXTRA_at6_u1 *pDudeExtraE = &gDudeExtra[pSprite->extra].at6.u1;
        pDudeExtraE->at8 = 1;
        pDudeExtraE->at0 = 0;
        pSprite->type = 202;
        if (pXSprite->target == -1)
        {
            switch (pXSprite->medium)
            {
            case 0:
                aiNewState(pSprite, pXSprite, &cultistSearch);
                if (Chance(0x8000))
                    aiPlay3DSound(pSprite, 1008+Random(5), AI_SFX_PRIORITY_1, -1);
                break;
            case 1:
            case 2:
                aiNewState(pSprite, pXSprite, &cultistSwimSearch);
                break;
            }
        }
        else
        {
            if (Chance(0x8000))
                aiPlay3DSound(pSprite, 1003+Random(4), AI_SFX_PRIORITY_1, -1);
            switch (pXSprite->medium)
            {
            case 0:
                aiNewState(pSprite, pXSprite, &cultistProneChase);
                break;
            case 1:
            case 2:
                aiNewState(pSprite, pXSprite, &cultistSwimChase);
                break;
            }
        }
        break;
    }
    case 240:
        if (pXSprite->target == -1)
            aiNewState(pSprite, pXSprite, &cultistBurnSearch);
        else
            aiNewState(pSprite, pXSprite, &cultistBurnChase);
        break;
    case 219:
    {
        DUDEEXTRA_at6_u1 *pDudeExtraE = &gDudeExtra[pSprite->extra].at6.u1;
        pDudeExtraE->at4 = 0;
        pDudeExtraE->at8 = 1;
        pDudeExtraE->at0 = 0;
        if (!pSprite->hitag)
            pSprite->hitag = 9;
        if (pXSprite->target == -1)
            aiNewState(pSprite, pXSprite, &batSearch);
        else
        {
            if (Chance(0xa000))
                aiPlay3DSound(pSprite, 2000, AI_SFX_PRIORITY_1, -1);
            aiNewState(pSprite, pXSprite, &batChase);
        }
        break;
    }
    case 218:
    {
        DUDEEXTRA_at6_u1 *pDudeExtraE = &gDudeExtra[pSprite->extra].at6.u1;
        pDudeExtraE->at4 = 0;
        pDudeExtraE->at8 = 1;
        pDudeExtraE->at0 = 0;
        if (pXSprite->target == -1)
            aiNewState(pSprite, pXSprite, &eelSearch);
        else
        {
            if (Chance(0x8000))
                aiPlay3DSound(pSprite, 1501, AI_SFX_PRIORITY_1, -1);
            else
                aiPlay3DSound(pSprite, 1500, AI_SFX_PRIORITY_1, -1);
            aiNewState(pSprite, pXSprite, &eelChase);
        }
        break;
    }
    case 217:
    {
        DUDEEXTRA_at6_u1 *pDudeExtraE = &gDudeExtra[pSprite->extra].at6.u1;
        XSECTOR *pXSector = NULL;
        if (sector[pSprite->sectnum].extra > 0)
            pXSector = &xsector[sector[pSprite->sectnum].extra];
        pDudeExtraE->at0 = 0;
        pDudeExtraE->at4 = 0;
        pDudeExtraE->at8 = 1;
        if (pXSprite->target == -1)
        {
            if (pXSector && pXSector->Underwater)
                aiNewState(pSprite, pXSprite, &gillBeastSwimSearch);
            else
                aiNewState(pSprite, pXSprite, &gillBeastSearch);
        }
        else
        {
            if (Chance(0x4000))
                aiPlay3DSound(pSprite, 1701, AI_SFX_PRIORITY_1, -1);
            else
                aiPlay3DSound(pSprite, 1700, AI_SFX_PRIORITY_1, -1);
            if (pXSector && pXSector->Underwater)
                aiNewState(pSprite, pXSprite, &gillBeastSwimChase);
            else
                aiNewState(pSprite, pXSprite, &gillBeastChase);
        }
        break;
    }
    case 203:
    {
        DUDEEXTRA_at6_u2 *pDudeExtraE = &gDudeExtra[pSprite->extra].at6.u2;
        pDudeExtraE->at4 = 1;
        pDudeExtraE->at0 = 0;
        if (pXSprite->target == -1)
            aiNewState(pSprite, pXSprite, &zombieASearch);
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
            aiNewState(pSprite, pXSprite, &zombieAChase);
        }
        break;
    }
    case 205:
    {
        DUDEEXTRA_at6_u2 *pDudeExtraE = &gDudeExtra[pSprite->extra].at6.u2;
        pDudeExtraE->at4 = 1;
        pDudeExtraE->at0 = 0;
        if (pXSprite->aiState == &zombieEIdle)
            aiNewState(pSprite, pXSprite, &zombieEUp);
        break;
    }
    case 244:
    {
        DUDEEXTRA_at6_u2 *pDudeExtraE = &gDudeExtra[pSprite->extra].at6.u2;
        pDudeExtraE->at4 = 1;
        pDudeExtraE->at0 = 0;
        if (pXSprite->aiState == &zombieSIdle)
            aiNewState(pSprite, pXSprite, &zombie13AC2C);
        break;
    }
    case 204:
    {
        DUDEEXTRA_at6_u2 *pDudeExtraE = &gDudeExtra[pSprite->extra].at6.u2;
        pDudeExtraE->at4 = 1;
        pDudeExtraE->at0 = 0;
        if (pXSprite->target == -1)
            aiNewState(pSprite, pXSprite, &zombieFSearch);
        else
        {
            if (Chance(0x4000))
                aiPlay3DSound(pSprite, 1201, AI_SFX_PRIORITY_1, -1);
            else
                aiPlay3DSound(pSprite, 1200, AI_SFX_PRIORITY_1, -1);
            aiNewState(pSprite, pXSprite, &zombieFChase);
        }
        break;
    }
    case 241:
        if (pXSprite->target == -1)
            aiNewState(pSprite, pXSprite, &zombieABurnSearch);
        else
            aiNewState(pSprite, pXSprite, &zombieABurnChase);
        break;
    case 242:
        if (pXSprite->target == -1)
            aiNewState(pSprite, pXSprite, &zombieFBurnSearch);
        else
            aiNewState(pSprite, pXSprite, &zombieFBurnChase);
        break;
    case 206:
    {
        DUDEEXTRA_at6_u1 *pDudeExtraE = &gDudeExtra[pSprite->extra].at6.u1;
        pDudeExtraE->at4 = 0;
        pDudeExtraE->at8 = 1;
        pDudeExtraE->at0 = 0;
        if (pXSprite->target == -1)
            aiNewState(pSprite, pXSprite, &gargoyleFSearch);
        else
        {
            if (Chance(0x4000))
                aiPlay3DSound(pSprite, 1401, AI_SFX_PRIORITY_1, -1);
            else
                aiPlay3DSound(pSprite, 1400, AI_SFX_PRIORITY_1, -1);
            aiNewState(pSprite, pXSprite, &gargoyleFChase);
        }
        break;
    }
    case 207:
    {
        DUDEEXTRA_at6_u1 *pDudeExtraE = &gDudeExtra[pSprite->extra].at6.u1;
        pDudeExtraE->at4 = 0;
        pDudeExtraE->at8 = 1;
        pDudeExtraE->at0 = 0;
        if (pXSprite->target == -1)
            aiNewState(pSprite, pXSprite, &gargoyleFSearch);
        else
        {
            if (Chance(0x4000))
                aiPlay3DSound(pSprite, 1451, AI_SFX_PRIORITY_1, -1);
            else
                aiPlay3DSound(pSprite, 1450, AI_SFX_PRIORITY_1, -1);
            aiNewState(pSprite, pXSprite, &gargoyleFChase);
        }
        break;
    }
    case 208:
    case 209:
        // By NoOne: play gargoyle statue breaking animation if data1 = 1.
        if (pXSprite->data1 == 1) {
            if (pSprite->type == 208)
                aiNewState(pSprite, pXSprite, &statueFBreakSEQ);
            else
                aiNewState(pSprite, pXSprite, &statueSBreakSEQ);
        } else {
            if (Chance(0x4000)) aiPlay3DSound(pSprite, 1401, AI_SFX_PRIORITY_1, -1);
            else aiPlay3DSound(pSprite, 1400, AI_SFX_PRIORITY_1, -1);
            
            if (pSprite->type == 208) aiNewState(pSprite, pXSprite, &gargoyleFMorph);
            else aiNewState(pSprite, pXSprite, &gargoyleSMorph);
        }
        break;
    case 227:
        if (pXSprite->target == -1)
            aiNewState(pSprite, pXSprite, &cerberusSearch);
        else
        {
            aiPlay3DSound(pSprite, 2300, AI_SFX_PRIORITY_1, -1);
            aiNewState(pSprite, pXSprite, &cerberusChase);
        }
        break;
    case 228:
        if (pXSprite->target == -1)
            aiNewState(pSprite, pXSprite, &cerberus2Search);
        else
        {
            aiPlay3DSound(pSprite, 2300, AI_SFX_PRIORITY_1, -1);
            aiNewState(pSprite, pXSprite, &cerberus2Chase);
        }
        break;
    case 211:
        if (pXSprite->target == -1)
            aiNewState(pSprite, pXSprite, &houndSearch);
        else
        {
            aiPlay3DSound(pSprite, 1300, AI_SFX_PRIORITY_1, -1);
            aiNewState(pSprite, pXSprite, &houndChase);
        }
        break;
    case 212:
        if (pXSprite->target == -1)
            aiNewState(pSprite, pXSprite, &handSearch);
        else
        {
            aiPlay3DSound(pSprite, 1900, AI_SFX_PRIORITY_1, -1);
            aiNewState(pSprite, pXSprite, &handChase);
        }
        break;
    case 220:
        if (pXSprite->target == -1)
            aiNewState(pSprite, pXSprite, &ratSearch);
        else
        {
            aiPlay3DSound(pSprite, 2100, AI_SFX_PRIORITY_1, -1);
            aiNewState(pSprite, pXSprite, &ratChase);
        }
        break;
    case 245:
        if (pXSprite->target == -1)
            aiNewState(pSprite, pXSprite, &innocentSearch);
        else
        {
            if (pXSprite->health > 0)
                aiPlay3DSound(pSprite, 7000+Random(6), AI_SFX_PRIORITY_1, -1);
            aiNewState(pSprite, pXSprite, &innocentChase);
        }
        break;
    case 229:
        if (pXSprite->target == -1)
            aiNewState(pSprite, pXSprite, &tchernobogSearch);
        else
        {
            aiPlay3DSound(pSprite, 2350+Random(7), AI_SFX_PRIORITY_1, -1);
            aiNewState(pSprite, pXSprite, &tchernobogChase);
        }
        break;
    case 213:
    case 214:
    case 215:
        pSprite->hitag |= 2;
        pSprite->cstat &= ~8;
        if (pXSprite->target == -1)
            aiNewState(pSprite, pXSprite, &spidSearch);
        else
        {
            aiPlay3DSound(pSprite, 1800, AI_SFX_PRIORITY_1, -1);
            aiNewState(pSprite, pXSprite, &spidChase);
        }
        break;
    case 216:
    {
        DUDEEXTRA_at6_u1 *pDudeExtraE = &gDudeExtra[pSprite->extra].at6.u1;
        pDudeExtraE->at8 = 1;
        pDudeExtraE->at0 = 0;
        pSprite->hitag |= 2;
        pSprite->cstat &= ~8;
        if (pXSprite->target == -1)
            aiNewState(pSprite, pXSprite, &spidSearch);
        else
        {
            aiPlay3DSound(pSprite, 1853+Random(1), AI_SFX_PRIORITY_1, -1);
            aiNewState(pSprite, pXSprite, &spidChase);
        }
        break;
    }
    case 250:
    {
        DUDEEXTRA_at6_u2 *pDudeExtraE = &gDudeExtra[pSprite->extra].at6.u2;
        pDudeExtraE->at4 = 1;
        pDudeExtraE->at0 = 0;
        if (pXSprite->target == -1)
        {
            switch (pXSprite->medium)
            {
            case 0:
                aiNewState(pSprite, pXSprite, &tinycalebSearch);
                break;
            case 1:
            case 2:
                aiNewState(pSprite, pXSprite, &tinycalebSwimSearch);
                break;
            }
        }
        else
        {
            switch (pXSprite->medium)
            {
            case 0:
                aiNewState(pSprite, pXSprite, &tinycalebChase);
                break;
            case 1:
            case 2:
                aiNewState(pSprite, pXSprite, &tinycalebSwimChase);
                break;
            }
        }
        break;
    }
    case 251:
    {
        DUDEEXTRA_at6_u2 *pDudeExtraE = &gDudeExtra[pSprite->extra].at6.u2;
        pDudeExtraE->at4 = 1;
        pDudeExtraE->at0 = 0;
        if (pXSprite->target == -1)
        {
            switch (pXSprite->medium)
            {
            case 0:
                aiNewState(pSprite, pXSprite, &beastSearch);
                break;
            case 1:
            case 2:
                aiNewState(pSprite, pXSprite, &beastSwimSearch);
                break;
            }
        }
        else
        {
            aiPlay3DSound(pSprite, 9009+Random(2), AI_SFX_PRIORITY_1, -1);
            switch (pXSprite->medium)
            {
            case 0:
                aiNewState(pSprite, pXSprite, &beastChase);
                break;
            case 1:
            case 2:
                aiNewState(pSprite, pXSprite, &beastSwimChase);
                break;
            }
        }
        break;
    }
    case 221:
    case 223:
        if (pXSprite->target == -1)
            aiNewState(pSprite, pXSprite, &podSearch);
        else
        {
            if (pSprite->type == 223)
                aiPlay3DSound(pSprite, 2453, AI_SFX_PRIORITY_1, -1);
            else
                aiPlay3DSound(pSprite, 2473, AI_SFX_PRIORITY_1, -1);
            aiNewState(pSprite, pXSprite, &podChase);
        }
        break;
    case 222:
    case 224:
        if (pXSprite->target == -1)
            aiNewState(pSprite, pXSprite, &tentacleSearch);
        else
        {
            aiPlay3DSound(pSprite, 2503, AI_SFX_PRIORITY_1, -1);
            aiNewState(pSprite, pXSprite, &tentacleChase);
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
    dassert(nTarget >= 0 && nTarget < kMaxSprites);
    spritetype *pTarget = &sprite[nTarget];
    if (pTarget->type >= kDudeBase && pTarget->type < kDudeMax)
    {
        if (actSpriteOwnerToSpriteId(&sprite[pXSprite->reference]) != nTarget)
        {
            pXSprite->target = nTarget;
            DUDEINFO *pDudeInfo = &dudeInfo[pTarget->type-kDudeBase];
            pXSprite->targetX = pTarget->x;
            pXSprite->targetY = pTarget->y;
            pXSprite->targetZ = pTarget->z-((pDudeInfo->eyeHeight*pTarget->yrepeat)<<2);
        }
    }
}


int aiDamageSprite(spritetype *pSprite, XSPRITE *pXSprite, int nSource, DAMAGE_TYPE nDmgType, int nDamage)
{
    dassert(nSource < kMaxSprites);
    if (!pXSprite->health)
        return 0;
    pXSprite->health = ClipLow(pXSprite->health - nDamage, 0);
    cumulDamage[pSprite->extra] += nDamage;
    DUDEINFO *pDudeInfo = &dudeInfo[pSprite->type-kDudeBase];
    int nSprite = pXSprite->reference;
    if (nSource >= 0)
    {
        spritetype *pSource = &sprite[nSource];
        if (pSprite == pSource)
            return 0;
        if (pXSprite->target == -1 || (nSource != pXSprite->target && Chance(pSprite->type == pSource->type ? nDamage*pDudeInfo->changeTargetKin : nDamage*pDudeInfo->changeTarget)))
        {
            aiSetTarget(pXSprite, nSource);
            aiActivateDude(pSprite, pXSprite);
        }
        if (nDmgType == DAMAGE_TYPE_6)
        {
            DUDEEXTRA *pDudeExtra = &gDudeExtra[pSprite->extra];
            pDudeExtra->at4 = 1;
        }
        switch (pSprite->type)
        {
        case 201:
        case 202:
        case 247:
        case 248:
            if (nDmgType != DAMAGE_TYPE_1)
            {
                if (!sub_5BDA8(pSprite, 14) && !pXSprite->medium)
                    aiNewState(pSprite, pXSprite, &cultistDodge);
                else if (sub_5BDA8(pSprite, 14) && !pXSprite->medium)
                    aiNewState(pSprite, pXSprite, &cultistProneDodge);
                else if (sub_5BDA8(pSprite, 13) && (pXSprite->medium == 1 || pXSprite->medium == 2))
                    aiNewState(pSprite, pXSprite, &cultistSwimDodge);
            }
            else if (nDmgType == DAMAGE_TYPE_1 && pXSprite->health <= (unsigned int)pDudeInfo->fleeHealth/* && (pXSprite->at17_6 != 1 || pXSprite->at17_6 != 2)*/)
            {
                pSprite->type = 240;
                aiNewState(pSprite, pXSprite, &cultistBurnGoto);
                aiPlay3DSound(pSprite, 361, AI_SFX_PRIORITY_0, -1);
                aiPlay3DSound(pSprite, 1031+Random(2), AI_SFX_PRIORITY_2, -1);
                gDudeExtra[pSprite->extra].at0 = gFrameClock+360;
                actHealDude(pXSprite, dudeInfo[40].startHealth, dudeInfo[40].startHealth);
                evKill(nSprite, 3, CALLBACK_ID_0);
            }
            break;
        case 245: // innocent
            if (nDmgType == DAMAGE_TYPE_1 && pXSprite->health <= (unsigned int)pDudeInfo->fleeHealth/* && (pXSprite->at17_6 != 1 || pXSprite->at17_6 != 2)*/)
            {
                pSprite->type = 239;
                aiNewState(pSprite, pXSprite, &cultistBurnGoto);
                aiPlay3DSound(pSprite, 361, AI_SFX_PRIORITY_0, -1);
                gDudeExtra[pSprite->extra].at0 = gFrameClock+360;
                actHealDude(pXSprite, dudeInfo[39].startHealth, dudeInfo[39].startHealth);
                evKill(nSprite, 3, CALLBACK_ID_0);
            }
            break;
        case 240:
            if (Chance(0x4000) && gDudeExtra[pSprite->extra].at0 < gFrameClock)
            {
                aiPlay3DSound(pSprite, 1031+Random(2), AI_SFX_PRIORITY_2, -1);
                gDudeExtra[pSprite->extra].at0 = gFrameClock+360;
            }
            if (Chance(0x600) && (pXSprite->medium == 1 || pXSprite->medium == 2))
            {
                pSprite->type = 201;
                pXSprite->burnTime = 0;
                aiNewState(pSprite, pXSprite, &cultistSwimGoto);
            }
            else if (pXSprite->medium == 1 || pXSprite->medium == 2)
            {
                pSprite->type = 202;
                pXSprite->burnTime = 0;
                aiNewState(pSprite, pXSprite, &cultistSwimGoto);
            }
            break;
        case 206:
            aiNewState(pSprite, pXSprite, &gargoyleFChase);
            break;
        case 204:
            if (nDmgType == DAMAGE_TYPE_1 && pXSprite->health <= (unsigned int)pDudeInfo->fleeHealth)
            {
                aiPlay3DSound(pSprite, 361, AI_SFX_PRIORITY_0, -1);
                aiPlay3DSound(pSprite, 1202, AI_SFX_PRIORITY_2, -1);
                pSprite->type = 242;
                aiNewState(pSprite, pXSprite, &zombieFBurnGoto);
                actHealDude(pXSprite, dudeInfo[42].startHealth, dudeInfo[42].startHealth);
                evKill(nSprite, 3, CALLBACK_ID_0);
            }
            break;
        case 250: // tiny Caleb
            if (nDmgType == DAMAGE_TYPE_1 && pXSprite->health <= (unsigned int)pDudeInfo->fleeHealth/* && (pXSprite->at17_6 != 1 || pXSprite->at17_6 != 2)*/)
            {
                pSprite->type = 239;
                if (!VanillaMode())
                    pXSprite->scale = -4; // need to change this to 64 later
                aiNewState(pSprite, pXSprite, &cultistBurnGoto);
                aiPlay3DSound(pSprite, 361, AI_SFX_PRIORITY_0, -1);
                gDudeExtra[pSprite->extra].at0 = gFrameClock+360;
                actHealDude(pXSprite, dudeInfo[39].startHealth, dudeInfo[39].startHealth);
                evKill(nSprite, 3, CALLBACK_ID_0);
            }
            break;
        case kGDXGenDudeBurning:
            if (Chance(0x2000) && gDudeExtra[pSprite->extra].at0 < gFrameClock) {
                sfxPlayGDXGenDudeSound(pSprite, 3, pXSprite->data3);
                gDudeExtra[pSprite->extra].at0 = gFrameClock + 360;
            }
            if (pXSprite->burnTime == 0) pXSprite->burnTime = 2400;
            if (spriteIsUnderwater(pSprite, false)) {
                pSprite->type = kGDXDudeUniversalCultist;
                pXSprite->burnTime = 0;
                pXSprite->health = 1; // so it can be killed with flame weapons while underwater and if already was burning dude before.
                aiNewState(pSprite, pXSprite, &GDXGenDudeGotoW);
            }
            break;
        case kGDXDudeUniversalCultist:
        {
            if (nDmgType == DAMAGE_TYPE_1) {
                if (pXSprite->health <= pDudeInfo->fleeHealth) {
                    if (getNextIncarnation(pXSprite) == NULL) {
                        removeDudeStuff(pSprite);
                        
                        if (pXSprite->data1 >= 459 && pXSprite->data1 < (459 + kExplodeMax) - 1)
                            doExplosion(pSprite, pXSprite->data1 - 459);
                        
                        if (spriteIsUnderwater(pSprite, false)) {
                            pXSprite->health = 0;
                            break;
                        }
                        
                        if (pXSprite->burnTime <= 0) 
                            pXSprite->burnTime = 1200;

                        if ((gSysRes.Lookup(pXSprite->data2 + 15, "SEQ") || gSysRes.Lookup(pXSprite->data2 + 16, "SEQ"))
                            && gSysRes.Lookup(pXSprite->data2 + 3, "SEQ")) {
                            
                            aiPlay3DSound(pSprite, 361, AI_SFX_PRIORITY_0, -1);
                            sfxPlayGDXGenDudeSound(pSprite, 3, pXSprite->data3);
                            pSprite->type = kGDXGenDudeBurning;

                            if (pXSprite->data2 == 11520) // don't inherit palette for burning if using default animation
                                pSprite->pal = 0;

                            aiNewState(pSprite, pXSprite, &GDXGenDudeBurnGoto);
                            actHealDude(pXSprite, dudeInfo[55].startHealth, dudeInfo[55].startHealth);
                            gDudeExtra[pSprite->extra].at0 = gFrameClock + 360;
                            evKill(nSprite, 3, CALLBACK_ID_0);

                        }

                    } else {
                        actKillDude(nSource, pSprite, nDmgType, nDamage);
                    }
                }
            } else if (pXSprite->aiState != &GDXGenDudeDodgeDmgD && pXSprite->aiState != &GDXGenDudeDodgeDmgL
                 && pXSprite->aiState != &GDXGenDudeDodgeDmgW) {
                    
                if (Chance(getDodgeChance(pSprite))) {
                    if (!spriteIsUnderwater(pSprite, false)) {
                        if (!sub_5BDA8(pSprite, 14)) aiNewState(pSprite, pXSprite, &GDXGenDudeDodgeDmgL);
                        else aiNewState(pSprite, pXSprite, &GDXGenDudeDodgeDmgD);
                    }
                    else if (sub_5BDA8(pSprite, 13) && spriteIsUnderwater(pSprite, false))
                        aiNewState(pSprite, pXSprite, &GDXGenDudeDodgeDmgW);
                }
            }

            break;
        }
        case 249:
            if (pXSprite->health <= (unsigned int)pDudeInfo->fleeHealth)
            {
                pSprite->type = 251;
                aiPlay3DSound(pSprite, 9008, AI_SFX_PRIORITY_1, -1);
                aiNewState(pSprite, pXSprite, &beastMorphFromCultist);
                actHealDude(pXSprite, dudeInfo[51].startHealth, dudeInfo[51].startHealth);
            }
            break;
        case 203:
        case 205:
            if (nDmgType == DAMAGE_TYPE_1 && pXSprite->health <= (unsigned int)pDudeInfo->fleeHealth)
            {
                aiPlay3DSound(pSprite, 361, AI_SFX_PRIORITY_0, -1);
                aiPlay3DSound(pSprite, 1106, AI_SFX_PRIORITY_2, -1);
                pSprite->type = 241;
                aiNewState(pSprite, pXSprite, &zombieABurnGoto);
                actHealDude(pXSprite, dudeInfo[41].startHealth, dudeInfo[41].startHealth);
                evKill(nSprite, 3, CALLBACK_ID_0);
            }
            break;
        }
    }
    return nDamage;
}

void RecoilDude(spritetype *pSprite, XSPRITE *pXSprite)
{
    char v4 = Chance(0x8000);
    DUDEEXTRA *pDudeExtra = &gDudeExtra[pSprite->extra];
    if (pSprite->statnum == 6 && (pSprite->type >= kDudeBase && pSprite->type < kDudeMax))
    {
        DUDEINFO *pDudeInfo = &dudeInfo[pSprite->type-kDudeBase];
        switch (pSprite->type)
        {
        case kGDXDudeUniversalCultist:
        {
            int mass = getDudeMassBySpriteSize(pSprite); int chance3 = getRecoilChance(pSprite);
            if ((mass < 155 && !spriteIsUnderwater(pSprite, false) && pDudeExtra->at4) || (mass > 155 && Chance(chance3)))
            {
                sfxPlayGDXGenDudeSound(pSprite, 1, pXSprite->data3);
                
                if (gSysRes.Lookup(pXSprite->data2 + 4, "SEQ")) aiNewState(pSprite, pXSprite, &GDXGenDudeRTesla);
                else if (!v4 || (v4 && gGameOptions.nDifficulty == 0)) aiNewState(pSprite, pXSprite, &GDXGenDudeRecoilD);
                else if (spriteIsUnderwater(pSprite, false)) aiNewState(pSprite, pXSprite, &GDXGenDudeRecoilW);
                else aiNewState(pSprite, pXSprite, &GDXGenDudeRecoilL);

                return;
            }

            if (pXSprite->aiState == &GDXGenDudeDodgeDmgW || pXSprite->aiState == &GDXGenDudeDodgeDmgD 
                || pXSprite->aiState == &GDXGenDudeDodgeDmgL) {
                    if (Chance(chance3)) sfxPlayGDXGenDudeSound(pSprite, 1, pXSprite->data3);
                    return;
            }

            if ((!dudeIsMelee(pXSprite) && mass < 155) || Chance(chance3)) {

                sfxPlayGDXGenDudeSound(pSprite, 1, pXSprite->data3);

                if (!v4 || (v4 && gGameOptions.nDifficulty == 0)) aiNewState(pSprite, pXSprite, &GDXGenDudeRecoilD);
                else if (spriteIsUnderwater(pSprite, false)) aiNewState(pSprite, pXSprite, &GDXGenDudeRecoilW);
                else aiNewState(pSprite, pXSprite, &GDXGenDudeRecoilL);

            }

            break;
        }
        case 201:
        case 202:
        case 247:
        case 248:
        case 249:
            if (pSprite->type == 201)
                aiPlay3DSound(pSprite, 4013+Random(2), AI_SFX_PRIORITY_2, -1);
            else
                aiPlay3DSound(pSprite, 1013+Random(2), AI_SFX_PRIORITY_2, -1);
            if (!v4 && pXSprite->medium == 0)
            {
                if (pDudeExtra->at4)
                    aiNewState(pSprite, pXSprite, &cultistTeslaRecoil);
                else
                    aiNewState(pSprite, pXSprite, &cultistRecoil);
            }
            else if (v4 && pXSprite->medium == 0)
            {
                if (pDudeExtra->at4)
                    aiNewState(pSprite, pXSprite, &cultistTeslaRecoil);
                else if (gGameOptions.nDifficulty > 0)
                    aiNewState(pSprite, pXSprite, &cultistProneRecoil);
                else
                    aiNewState(pSprite, pXSprite, &cultistRecoil);
            }
            else if (pXSprite->medium == 1 || pXSprite->medium == 2)
                aiNewState(pSprite, pXSprite, &cultistSwimRecoil);
            else
            {
                if (pDudeExtra->at4)
                    aiNewState(pSprite, pXSprite, &cultistTeslaRecoil);
                else
                    aiNewState(pSprite, pXSprite, &cultistRecoil);
            }
            break;
        case 240:
            aiNewState(pSprite, pXSprite, &cultistBurnGoto);
            break;
        case kGDXGenDudeBurning:
            aiNewState(pSprite, pXSprite, &GDXGenDudeBurnGoto);
            break;
        case 204:
            aiPlay3DSound(pSprite, 1202, AI_SFX_PRIORITY_2, -1);
            if (pDudeExtra->at4)
                aiNewState(pSprite, pXSprite, &zombieFTeslaRecoil);
            else
                aiNewState(pSprite, pXSprite, &zombieFRecoil);
            break;
        case 203:
        case 205:
            aiPlay3DSound(pSprite, 1106, AI_SFX_PRIORITY_2, -1);
            if (pDudeExtra->at4 && pXSprite->cumulDamage > pDudeInfo->startHealth/3)
                aiNewState(pSprite, pXSprite, &zombieATeslaRecoil);
            else if (pXSprite->cumulDamage > pDudeInfo->startHealth/3)
                aiNewState(pSprite, pXSprite, &zombieARecoil2);
            else
                aiNewState(pSprite, pXSprite, &zombieARecoil);
            break;
        case 241:
            aiPlay3DSound(pSprite, 1106, AI_SFX_PRIORITY_2, -1);
            aiNewState(pSprite, pXSprite, &zombieABurnGoto);
            break;
        case 242:
            aiPlay3DSound(pSprite, 1202, AI_SFX_PRIORITY_2, -1);
            aiNewState(pSprite, pXSprite, &zombieFBurnGoto);
            break;
        case 206:
        case 207:
            aiPlay3DSound(pSprite, 1402, AI_SFX_PRIORITY_2, -1);
            aiNewState(pSprite, pXSprite, &gargoyleFRecoil);
            break;
        case 227:
            aiPlay3DSound(pSprite, 2302+Random(2), AI_SFX_PRIORITY_2, -1);
            if (pDudeExtra->at4 && pXSprite->cumulDamage > pDudeInfo->startHealth/3)
                aiNewState(pSprite, pXSprite, &cerberusTeslaRecoil);
            else
                aiNewState(pSprite, pXSprite, &cerberusRecoil);
            break;
        case 228:
            aiPlay3DSound(pSprite, 2302+Random(2), AI_SFX_PRIORITY_2, -1);
            aiNewState(pSprite, pXSprite, &cerberus2Recoil);
            break;
        case 211:
            aiPlay3DSound(pSprite, 1302, AI_SFX_PRIORITY_2, -1);
            if (pDudeExtra->at4)
                aiNewState(pSprite, pXSprite, &houndTeslaRecoil);
            else
                aiNewState(pSprite, pXSprite, &houndRecoil);
            break;
        case 229:
            aiPlay3DSound(pSprite, 2370+Random(2), AI_SFX_PRIORITY_2, -1);
            aiNewState(pSprite, pXSprite, &tchernobogRecoil);
            break;
        case 212:
            aiPlay3DSound(pSprite, 1902, AI_SFX_PRIORITY_2, -1);
            aiNewState(pSprite, pXSprite, &handRecoil);
            break;
        case 220:
            aiPlay3DSound(pSprite, 2102, AI_SFX_PRIORITY_2, -1);
            aiNewState(pSprite, pXSprite, &ratRecoil);
            break;
        case 219:
            aiPlay3DSound(pSprite, 2002, AI_SFX_PRIORITY_2, -1);
            aiNewState(pSprite, pXSprite, &batRecoil);
            break;
        case 218:
            aiPlay3DSound(pSprite, 1502, AI_SFX_PRIORITY_2, -1);
            aiNewState(pSprite, pXSprite, &eelRecoil);
            break;
        case 217:
        {
            XSECTOR *pXSector = NULL;
            if (sector[pSprite->sectnum].extra > 0)
                pXSector = &xsector[sector[pSprite->sectnum].extra];
            aiPlay3DSound(pSprite, 1702, AI_SFX_PRIORITY_2, -1);
            if (pXSector && pXSector->Underwater)
                aiNewState(pSprite, pXSprite, &gillBeastSwimRecoil);
            else
                aiNewState(pSprite, pXSprite, &gillBeastRecoil);
            break;
        }
        case 210:
            aiPlay3DSound(pSprite, 1602, AI_SFX_PRIORITY_2, -1);
            if (pDudeExtra->at4)
                aiNewState(pSprite, pXSprite, &ghostTeslaRecoil);
            else
                aiNewState(pSprite, pXSprite, &ghostRecoil);
            break;
        case 213:
        case 214:
        case 215:
            aiPlay3DSound(pSprite, 1802+Random(1), AI_SFX_PRIORITY_2, -1);
            aiNewState(pSprite, pXSprite, &spidDodge);
            break;
        case 216:
            aiPlay3DSound(pSprite, 1851+Random(1), AI_SFX_PRIORITY_2, -1);
            aiNewState(pSprite, pXSprite, &spidDodge);
            break;
        case 245:
            aiPlay3DSound(pSprite, 7007+Random(2), AI_SFX_PRIORITY_2, -1);
            if (pDudeExtra->at4)
                aiNewState(pSprite, pXSprite, &innocentTeslaRecoil);
            else
                aiNewState(pSprite, pXSprite, &innocentRecoil);
            break;
        case 250:
            if (pXSprite->medium == 0)
            {
                if (pDudeExtra->at4)
                    aiNewState(pSprite, pXSprite, &tinycalebTeslaRecoil);
                else
                    aiNewState(pSprite, pXSprite, &tinycalebRecoil);
            }
            else if (pXSprite->medium == 1 || pXSprite->medium == 2)
                aiNewState(pSprite, pXSprite, &tinycalebSwimRecoil);
            else
            {
                if (pDudeExtra->at4)
                    aiNewState(pSprite, pXSprite, &tinycalebTeslaRecoil);
                else
                    aiNewState(pSprite, pXSprite, &tinycalebRecoil);
            }
            break;
        case 251:
            aiPlay3DSound(pSprite, 9004+Random(2), AI_SFX_PRIORITY_2, -1);
            if (pXSprite->medium == 0)
            {
                if (pDudeExtra->at4)
                    aiNewState(pSprite, pXSprite, &beastTeslaRecoil);
                else
                    aiNewState(pSprite, pXSprite, &beastRecoil);
            }
            else if (pXSprite->medium == 1 || pXSprite->medium == 2)
                aiNewState(pSprite, pXSprite, &beastSwimRecoil);
            else
            {
                if (pDudeExtra->at4)
                    aiNewState(pSprite, pXSprite, &beastTeslaRecoil);
                else
                    aiNewState(pSprite, pXSprite, &beastRecoil);
            }
            break;
        case 221:
        case 223:
            aiNewState(pSprite, pXSprite, &podRecoil);
            break;
        case 222:
        case 224:
            aiNewState(pSprite, pXSprite, &tentacleRecoil);
            break;
        default:
            aiNewState(pSprite, pXSprite, &genRecoil);
            break;
        }
        pDudeExtra->at4 = 0;
    }
}

void aiThinkTarget(spritetype *pSprite, XSPRITE *pXSprite)
{
    dassert(pSprite->type >= kDudeBase && pSprite->type < kDudeMax);
    DUDEINFO *pDudeInfo = &dudeInfo[pSprite->type-kDudeBase];
    if (Chance(pDudeInfo->alertChance))
    {
        for (int p = connecthead; p >= 0; p = connectpoint2[p])
        {
            PLAYER *pPlayer = &gPlayer[p];
            if (pSprite->owner == pPlayer->at5b || pPlayer->pXSprite->health == 0 || powerupCheck(pPlayer, 13) > 0)
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
            if (nDist < pDudeInfo->seeDist && klabs(nDeltaAngle) <= pDudeInfo->periphery)
            {
                aiSetTarget(pXSprite, pPlayer->at5b);
                aiActivateDude(pSprite, pXSprite);
                return;
            }
            else if (nDist < pDudeInfo->hearDist)
            {
                aiSetTarget(pXSprite, x, y, z);
                aiActivateDude(pSprite, pXSprite);
                return;
            }
        }
    }
}

void sub_5F15C(spritetype *pSprite, XSPRITE *pXSprite)
{
    dassert(pSprite->type >= kDudeBase && pSprite->type < kDudeMax);
    DUDEINFO *pDudeInfo = &dudeInfo[pSprite->type-kDudeBase];
    if (Chance(pDudeInfo->alertChance))
    {
        for (int p = connecthead; p >= 0; p = connectpoint2[p])
        {
            PLAYER *pPlayer = &gPlayer[p];
            if (pSprite->owner == pPlayer->at5b || pPlayer->pXSprite->health == 0 || powerupCheck(pPlayer, 13) > 0)
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
            if (nDist < pDudeInfo->seeDist && klabs(nDeltaAngle) <= pDudeInfo->periphery)
            {
                aiSetTarget(pXSprite, pPlayer->at5b);
                aiActivateDude(pSprite, pXSprite);
                return;
            }
            else if (nDist < pDudeInfo->hearDist)
            {
                aiSetTarget(pXSprite, x, y, z);
                aiActivateDude(pSprite, pXSprite);
                return;
            }
        }
        if (pXSprite->state)
        {
            char va4[(kMaxSectors+7)>>3];
            gAffectedSectors[0] = 0;
            gAffectedXWalls[0] = 0;
            GetClosestSpriteSectors(pSprite->sectnum, pSprite->x, pSprite->y, 400, gAffectedSectors, va4, gAffectedXWalls);
            for (int nSprite2 = headspritestat[6]; nSprite2 >= 0; nSprite2 = nextspritestat[nSprite2])
            {
                spritetype *pSprite2 = &sprite[nSprite2];
                int dx = pSprite2->x-pSprite->x;
                int dy = pSprite2->y-pSprite->y;
                int nDist = approxDist(dx, dy);
                if (pSprite2->type == 245)
                {
                    DUDEINFO *pDudeInfo = &dudeInfo[pSprite2->type-kDudeBase];
                    if (nDist > pDudeInfo->seeDist && nDist > pDudeInfo->hearDist)
                        continue;
                    int UNUSED(nAngle) = getangle(dx,dy);
                    aiSetTarget(pXSprite, pSprite2->index);
                    aiActivateDude(pSprite, pXSprite);
                    return;
                }
            }
        }
    }
}

void aiProcessDudes(void)
{
    for (int nSprite = headspritestat[6]; nSprite >= 0; nSprite = nextspritestat[nSprite])
    {
        spritetype *pSprite = &sprite[nSprite];
        if (pSprite->hitag&32)
            continue;
        int nXSprite = pSprite->extra;
        XSPRITE *pXSprite = &xsprite[nXSprite];
        DUDEINFO *pDudeInfo = &dudeInfo[pSprite->type-kDudeBase];
        if (pSprite->type >= kDudePlayer1 && pSprite->type <= kDudePlayer8)
            continue;
        if (pXSprite->health == 0)
            continue;
        pXSprite->stateTimer = ClipLow(pXSprite->stateTimer-4, 0);
        if (pXSprite->aiState->at10)
            pXSprite->aiState->at10(pSprite, pXSprite);
        if (pXSprite->aiState->at14 && (gFrame&3) == (nSprite&3))
            pXSprite->aiState->at14(pSprite, pXSprite);
        if (pXSprite->stateTimer == 0 && pXSprite->aiState->at18)
        {
            if (pXSprite->aiState->at8 > 0)
                aiNewState(pSprite, pXSprite, pXSprite->aiState->at18);
            else if (seqGetStatus(3, nXSprite) < 0)
                aiNewState(pSprite, pXSprite, pXSprite->aiState->at18);
        }
        if (pXSprite->health > 0 && ((pDudeInfo->hinderDamage<<4) <= cumulDamage[nXSprite]))
        {
            pXSprite->cumulDamage = cumulDamage[nXSprite];
            RecoilDude(pSprite, pXSprite);
        }
    }
    memset(cumulDamage, 0, sizeof(cumulDamage));
}

void aiInit(void)
{
    for (int nSprite = headspritestat[6]; nSprite >= 0; nSprite = nextspritestat[nSprite])
    {
        aiInitSprite(&sprite[nSprite]);
    }
}

void aiInitSprite(spritetype *pSprite)
{
    int nXSprite = pSprite->extra;
    XSPRITE *pXSprite = &xsprite[nXSprite];
    int nSector = pSprite->sectnum;
    int nXSector = sector[nSector].extra;
    XSECTOR *pXSector = NULL;
    if (nXSector > 0)
        pXSector = &xsector[nXSector];
    DUDEEXTRA *pDudeExtra = &gDudeExtra[pSprite->extra];
    pDudeExtra->at4 = 0;
    pDudeExtra->at0 = 0;
    switch (pSprite->type)
    {
    case kGDXDudeUniversalCultist:
    {
        DUDEEXTRA_at6_u1* pDudeExtraE = &gDudeExtra[nXSprite].at6.u1;
        pDudeExtraE->at8 = 0;
        pDudeExtraE->at0 = 0;
        pDudeExtraE->at4 = 0;
        aiNewState(pSprite, pXSprite, &GDXGenDudeIdleL);
        break;
    }
    case kGDXGenDudeBurning:
        aiNewState(pSprite, pXSprite, &GDXGenDudeBurnGoto);
        pXSprite->burnTime = 1200;
        break;
    case 201:
    case 202:
    case 247:
    case 248:
    case 249:
    {
        DUDEEXTRA_at6_u1 *pDudeExtraE = &gDudeExtra[nXSprite].at6.u1;
        pDudeExtraE->at8 = 0;
        pDudeExtraE->at0 = 0;
        aiNewState(pSprite, pXSprite, &cultistIdle);
        break;
    }
    case 230:
    {
        DUDEEXTRA_at6_u1 *pDudeExtraE = &gDudeExtra[nXSprite].at6.u1;
        pDudeExtraE->at8 = 0;
        pDudeExtraE->at0 = 0;
        aiNewState(pSprite, pXSprite, &fanaticProneIdle);
        break;
    }
    case 246:
    {
        DUDEEXTRA_at6_u1 *pDudeExtraE = &gDudeExtra[nXSprite].at6.u1;
        pDudeExtraE->at8 = 0;
        pDudeExtraE->at0 = 0;
        aiNewState(pSprite, pXSprite, &cultistProneIdle);
        break;
    }
    case 204:
    {
        DUDEEXTRA_at6_u2 *pDudeExtraE = &gDudeExtra[nXSprite].at6.u2;
        pDudeExtraE->at4 = 0;
        pDudeExtraE->at0 = 0;
        aiNewState(pSprite, pXSprite, &zombieFIdle);
        break;
    }
    case 203:
    {
        DUDEEXTRA_at6_u2 *pDudeExtraE = &gDudeExtra[nXSprite].at6.u2;
        pDudeExtraE->at4 = 0;
        pDudeExtraE->at0 = 0;
        aiNewState(pSprite, pXSprite, &zombieAIdle);
        break;
    }
    case 244:
    {
        DUDEEXTRA_at6_u2 *pDudeExtraE = &gDudeExtra[nXSprite].at6.u2;
        pDudeExtraE->at4 = 0;
        pDudeExtraE->at0 = 0;
        aiNewState(pSprite, pXSprite, &zombieSIdle);
        pSprite->hitag &= ~1;
        break;
    }
    case 205:
    {
        DUDEEXTRA_at6_u2 *pDudeExtraE = &gDudeExtra[nXSprite].at6.u2;
        pDudeExtraE->at4 = 0;
        pDudeExtraE->at0 = 0;
        aiNewState(pSprite, pXSprite, &zombieEIdle);
        break;
    }
    case 206:
    case 207:
    {
        DUDEEXTRA_at6_u1 *pDudeExtraE = &gDudeExtra[nXSprite].at6.u1;
        pDudeExtraE->at4 = 0;
        pDudeExtraE->at8 = 0;
        pDudeExtraE->at0 = 0;
        aiNewState(pSprite, pXSprite, &gargoyleFIdle);
        break;
    }
    case 208:
    case 209:
        aiNewState(pSprite, pXSprite, &gargoyleStatueIdle);
        break;
    case 227:
    {
        DUDEEXTRA_at6_u2 *pDudeExtraE = &gDudeExtra[nXSprite].at6.u2;
        pDudeExtraE->at4 = 0;
        pDudeExtraE->at0 = 0;
        aiNewState(pSprite, pXSprite, &cerberusIdle);
        break;
    }
    case 211:
        aiNewState(pSprite, pXSprite, &houndIdle);
        break;
    case 212:
        aiNewState(pSprite, pXSprite, &handIdle);
        break;
    case 210:
    {
        DUDEEXTRA_at6_u1 *pDudeExtraE = &gDudeExtra[nXSprite].at6.u1;
        pDudeExtraE->at4 = 0;
        pDudeExtraE->at8 = 0;
        pDudeExtraE->at0 = 0;
        aiNewState(pSprite, pXSprite, &ghostIdle);
        break;
    }
    case 245:
        aiNewState(pSprite, pXSprite, &innocentIdle);
        break;
    case 220:
        aiNewState(pSprite, pXSprite, &ratIdle);
        break;
    case 218:
    {
        DUDEEXTRA_at6_u1 *pDudeExtraE = &gDudeExtra[nXSprite].at6.u1;
        pDudeExtraE->at4 = 0;
        pDudeExtraE->at8 = 0;
        pDudeExtraE->at0 = 0;
        aiNewState(pSprite, pXSprite, &eelIdle);
        break;
    }
    case 217:
        if (pXSector && pXSector->Underwater)
            aiNewState(pSprite, pXSprite, &gillBeastIdle);
        else
            aiNewState(pSprite, pXSprite, &gillBeastIdle);
        break;
    case 219:
    {
        DUDEEXTRA_at6_u1 *pDudeExtraE = &gDudeExtra[nXSprite].at6.u1;
        pDudeExtraE->at4 = 0;
        pDudeExtraE->at8 = 0;
        pDudeExtraE->at0 = 0;
        aiNewState(pSprite, pXSprite, &batIdle);
        break;
    }
    case 213:
    case 214:
    case 215:
    {
        DUDEEXTRA_at6_u1 *pDudeExtraE = &gDudeExtra[nXSprite].at6.u1;
        pDudeExtraE->at8 = 0;
        pDudeExtraE->at4 = 0;
        pDudeExtraE->at0 = 0;
        aiNewState(pSprite, pXSprite, &spidIdle);
        break;
    }
    case 216:
    {
        DUDEEXTRA_at6_u1 *pDudeExtraE = &gDudeExtra[nXSprite].at6.u1;
        pDudeExtraE->at8 = 0;
        pDudeExtraE->at4 = 0;
        pDudeExtraE->at0 = 0;
        aiNewState(pSprite, pXSprite, &spidIdle);
        break;
    }
    case 229:
    {
        DUDEEXTRA_at6_u2 *pDudeExtraE = &gDudeExtra[nXSprite].at6.u2;
        pDudeExtraE->at4 = 0;
        pDudeExtraE->at0 = 0;
        aiNewState(pSprite, pXSprite, &tchernobogIdle);
        break;
    }
    case 250:
        aiNewState(pSprite, pXSprite, &tinycalebIdle);
        break;
    case 251:
        aiNewState(pSprite, pXSprite, &beastIdle);
        break;
    case 221:
    case 223:
        aiNewState(pSprite, pXSprite, &podIdle);
        break;
    case 222:
    case 224:
        aiNewState(pSprite, pXSprite, &tentacleIdle);
        break;
    default:
        aiNewState(pSprite, pXSprite, &genIdle);
        break;
    }
    aiSetTarget(pXSprite, 0, 0, 0);
    pXSprite->stateTimer = 0;
    switch (pSprite->type)
    {
    case 213:
    case 214:
    case 215:
        if (pSprite->cstat&8)
            pSprite->hitag |= 9;
        else
            pSprite->hitag = 15;
        break;
    case 206:
    case 207:
    case 210:
    case 218:
    case 219:
        pSprite->hitag |= 9;
        break;
    case 217:
        if (pXSector && pXSector->Underwater)
            pSprite->hitag |= 9;
        else
            pSprite->hitag = 15;
        break;
    case 205:
    case 244:
        pSprite->hitag = 7;
        break;
    // By NoOne: Allow put pods and tentacles on ceilings if sprite is y-flipped.
    case 221:
    case 222:
    case 223:
    case 224:
    case 225:
    case 226:
        if ((pSprite->cstat & kSprFlipY) != 0) {
            if (!(pSprite->hitag & kHitagExtBit)) // don't add autoaim for player if hitag 1 specified in editor.
                pSprite->hitag = kHitagAutoAim;
            break;
        }
        fallthrough__;
    // go default
    default:
        pSprite->hitag = 15;
        break;
    }
}

class AILoadSave : public LoadSave
{
    virtual void Load(void);
    virtual void Save(void);
};

void AILoadSave::Load(void)
{
    Read(cumulDamage, sizeof(cumulDamage));
    Read(gDudeSlope, sizeof(gDudeSlope));
}

void AILoadSave::Save(void)
{
    Write(cumulDamage, sizeof(cumulDamage));
    Write(gDudeSlope, sizeof(gDudeSlope));
}

static AILoadSave *myLoadSave;

void AILoadSaveConstruct(void)
{
    myLoadSave = new AILoadSave();
}

