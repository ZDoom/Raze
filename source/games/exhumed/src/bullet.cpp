//-------------------------------------------------------------------------
/*
Copyright (C) 2010-2019 EDuke32 developers and contributors
Copyright (C) 2019 sirlemonhead, Nuke.YKT
This file is part of PCExhumed.
PCExhumed is free software; you can redistribute it and/or
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
#include "ns.h"
#include "engine.h"
#include "aistuff.h"
#include "sequence.h"
#include "exhumed.h"
#include "sound.h"
#include "player.h"
#include "names.h"
#include <string.h>
#include <assert.h>
#ifndef __WATCOMC__
//#include <cmath>
#else
//#include <math.h>
#include <stdlib.h>
#endif

BEGIN_PS_NS

enum { kMaxBullets		= 500 };


// 32 bytes
struct Bullet
{
    short nSeq;
    short nFrame;
    short nSprite;
    short field_6;
    short field_8;
    short nType;
    short field_C;
    short field_E;
    uint16_t field_10;
    uint8_t field_12;
    uint8_t field_13;
    int x;
    int y;
    int z;
    int enemy;
};

FreeListArray<Bullet, kMaxBullets> BulletList;
int lasthitz, lasthitx, lasthity;
short lasthitsect, lasthitsprite, lasthitwall;

short nRadialBullet = 0;

FSerializer& Serialize(FSerializer& arc, const char* keyname, Bullet& w, Bullet* def)
{
    static Bullet nul;
    if (!def)
    {
        def = &nul;
        if (arc.isReading()) w = {};
    }
    if (arc.BeginObject(keyname))
    {
        arc("seq", w.nSeq, def->nSeq)
            ("frame", w.nFrame, def->nFrame)
            ("sprite", w.nSprite, def->nSprite)
            ("type", w.nType, def->nType)
            ("x", w.x, def->x)
            ("y", w.y, def->y)
            ("z", w.z, def->z)
            ("at6", w.field_6, def->field_6)
            ("at8", w.field_8, def->field_8)
            ("atc", w.field_C, def->field_C)
            ("ate", w.field_E, def->field_E)
            ("at10", w.field_10, def->field_10)
            ("at12", w.field_12, def->field_12)
            ("at13", w.field_13, def->field_13)
            ("enemy", w.enemy, def->enemy)
            .EndObject();
    }
    return arc;
}

void SerializeBullet(FSerializer& arc)
{
    if (arc.BeginObject("bullets"))
    {
        arc ("list", BulletList)
            ("lasthitx", lasthitx)
            ("lasthity", lasthity)
            ("lasthitz", lasthitz)
            ("lasthitsect", lasthitsect)
            ("lasthitspr", lasthitsprite)
            ("lasthitwall", lasthitwall)
            ("radialbullet", nRadialBullet)
            .EndObject();
    }
}

bulletInfo BulletInfo[] = {
    { 25,   1,    20, -1, -1, 13, 0,  0, -1 },
    { 25,  -1, 65000, -1, 31, 73, 0,  0, -1 },
    { 15,  -1, 60000, -1, 31, 73, 0,  0, -1 },
    { 5,   15,  2000, -1, 14, 38, 4,  5,  3 },
    { 250, 100, 2000, -1, 33, 34, 4, 20, -1 },
    { 200, -1,  2000, -1, 20, 23, 4, 10, -1 },
    { 200, -1, 60000, 68, 68, -1, -1, 0, -1 },
    { 300,  1,     0, -1, -1, -1, 0, 50, -1 },
    { 18,  -1,  2000, -1, 18, 29, 4,  0, -1 },
    { 20,  -1,  2000, 37, 11, 30, 4,  0, -1 },
    { 25,  -1,  3000, -1, 44, 36, 4, 15, 90 },
    { 30,  -1,  1000, -1, 52, 53, 4, 20, 48 },
    { 20,  -1,  3500, -1, 54, 55, 4, 30, -1 },
    { 10,  -1,  5000, -1, 57, 76, 4,  0, -1 },
    { 40,  -1,  1500, -1, 63, 38, 4, 10, 40 },
    { 20,  -1,  2000, -1, 60, 12, 0,  0, -1 },
    { 5,   -1, 60000, -1, 31, 76, 0,  0, -1 }
};


void InitBullets()
{
    BulletList.Clear();
}

int GrabBullet()
{
    int grabbed = BulletList.Get();
    if (grabbed < 0) return -1;
    BulletList[grabbed].enemy = -1;
    return grabbed;
}

void DestroyBullet(short nBullet)
{
    short nSprite = BulletList[nBullet].nSprite;

    runlist_DoSubRunRec(BulletList[nBullet].field_6);
    runlist_DoSubRunRec(sprite[nSprite].lotag - 1);
    runlist_SubRunRec(BulletList[nBullet].field_8);

    StopSpriteSound(nSprite);

    mydeletesprite(nSprite);
    BulletList.Release(nBullet);
}

void IgniteSprite(int nSprite)
{
    sprite[nSprite].hitag += 2;

    int nAnim = BuildAnim(-1, 38, 0, sprite[nSprite].x, sprite[nSprite].y, sprite[nSprite].z, sprite[nSprite].sectnum, 40, 20);
    short nAnimSprite = GetAnimSprite(nAnim);

    sprite[nAnimSprite].hitag = nSprite;
    changespritestat(nAnimSprite, kStatIgnited);

    short yRepeat = (tileHeight(sprite[nAnimSprite].picnum) * 32) / nFlameHeight;
    if (yRepeat < 1)
        yRepeat = 1;

    sprite[nAnimSprite].yrepeat = (uint8_t)yRepeat;
}

void BulletHitsSprite(Bullet *pBullet, short nBulletSprite, short nHitSprite, int x, int y, int z, int nSector)
{
    assert(nSector >= 0 && nSector < kMaxSectors);

    bulletInfo *pBulletInfo = &BulletInfo[pBullet->nType];

    short nStat = sprite[nHitSprite].statnum;

    switch (pBullet->nType)
    {
        case 3:
        {
            if (nStat > 107 || nStat == kStatAnubisDrum) {
                return;
            }

            sprite[nHitSprite].hitag++;

            if (sprite[nHitSprite].hitag == 15) {
                IgniteSprite(nHitSprite);
            }

            if (!RandomSize(2)) {
                BuildAnim(-1, pBulletInfo->field_C, 0, x, y, z, nSector, 40, pBulletInfo->nFlags);
            }

            return;
        }
        case 14:
        {
            if (nStat > 107 || nStat == kStatAnubisDrum) {
                return;
            }
            // else - fall through to below cases
            fallthrough__;
        }
        case 1:
        case 2:
        case 8:
        case 9:
        case 12:
        case 13:
        case 15:
        case 16:
        {
            // loc_29E59
            if (!nStat || nStat > 98) {
                break;
            }

            short nSprite = pBullet->nSprite;
            spritetype *pSprite = &sprite[nSprite];
            spritetype *pHitSprite = &sprite[nHitSprite];

            if (nStat == kStatAnubisDrum)
            {
                short nAngle = (pSprite->ang + 256) - RandomSize(9);

                pHitSprite->xvel = bcos(nAngle, 1);
                pHitSprite->yvel = bsin(nAngle, 1);
                pHitSprite->zvel = (-(RandomSize(3) + 1)) << 8;
            }
            else
            {
                int xVel = pHitSprite->xvel;
                int yVel = pHitSprite->yvel;

                pHitSprite->xvel = bcos(pSprite->ang, -2);
                pHitSprite->yvel = bsin(pSprite->ang, -2);

                MoveCreature(nHitSprite);

                pHitSprite->xvel = xVel;
                pHitSprite->yvel = yVel;
            }

            break;
        }

        default:
            break;
    }

    // BHS_switchBreak:
    short nDamage = pBulletInfo->nDamage;

    if (pBullet->field_13 > 1) {
        nDamage *= 2;
    }

    runlist_DamageEnemy(nHitSprite, nBulletSprite, nDamage);

    if (nStat <= 90 || nStat >= 199)
    {
        BuildAnim(-1, pBulletInfo->field_C, 0, x, y, z, nSector, 40, pBulletInfo->nFlags);
        return;
    }

    switch (nStat)
    {
        case kStatDestructibleSprite:
            break;
        case kStatAnubisDrum:
        case 102:
        case kStatExplodeTrigger:
        case kStatExplodeTarget:
            BuildAnim(-1, 12, 0, x, y, z, nSector, 40, 0);
            break;
        default:
            BuildAnim(-1, 39, 0, x, y, z, nSector, 40, 0);
            if (pBullet->nType > 2)
            {
                BuildAnim(-1, pBulletInfo->field_C, 0, x, y, z, nSector, 40, pBulletInfo->nFlags);
            }
            break;
    }
}


void BackUpBullet(int *x, int *y, short nAngle)
{
    *x -= bcos(nAngle, -11);
    *y -= bsin(nAngle, -11);
}

int MoveBullet(short nBullet)
{
    short hitsect = -1;
    short hitwall = -1;
    short hitsprite = -1;

    Bullet *pBullet = &BulletList[nBullet];
    short nType = pBullet->nType;
    bulletInfo *pBulletInfo = &BulletInfo[nType];

    short nSprite = BulletList[nBullet].nSprite;
    spritetype *pSprite = &sprite[nSprite];

    int x = pSprite->x;
    int y = pSprite->y;
    int z = pSprite->z; // ebx
    short nSectFlag = SectFlag[pSprite->sectnum];

    int x2, y2, z2;

    int nVal;

    if (pBullet->field_10 < 30000)
    {
        short nEnemySprite = BulletList[nBullet].enemy;
        if (nEnemySprite > -1)
        {
            if (!(sprite[nEnemySprite].cstat & 0x101))
                BulletList[nBullet].enemy = -1;
            else
            {
                nVal = AngleChase(nSprite, nEnemySprite, pBullet->field_10, 0, 16);
                goto MOVEEND;
            }
        }
        if (nType == 3)
        {
            if (pBullet->field_E < 8)
            {
                pSprite->xrepeat -= 1;
                pSprite->yrepeat += 8;

                pBullet->z -= 200;

                if (pSprite->shade < 90) {
                    pSprite->shade += 35;
                }

                if (pBullet->field_E == 3)
                {
                    pBullet->nSeq = 45;
                    pBullet->nFrame = 0;
                    pSprite->xrepeat = 40;
                    pSprite->yrepeat = 40;
                    pSprite->shade = 0;
                    pSprite->z += 512;
                }
            }
            else
            {
                pSprite->xrepeat += 4;
                pSprite->yrepeat += 4;
            }
        }

        nVal = movesprite(nSprite, pBullet->x, pBullet->y, pBullet->z, pSprite->clipdist >> 1, pSprite->clipdist >> 1, CLIPMASK1);

MOVEEND:
        if (nVal)
        {
            x2 = pSprite->x;
            y2 = pSprite->y;
            z2 = pSprite->z;
            hitsect = pSprite->sectnum;

            if (nVal & 0x30000)
            {
                hitwall = nVal & 0x3FFF;
                goto HITWALL;
            }
            else
            {
                switch (nVal & 0xc000)
                {
                case 0x8000:
                    hitwall = nVal & 0x3FFF;
                    goto HITWALL;
                case 0xc000:
                    hitsprite = nVal & 0x3FFF;
                    goto HITSPRITE;
                }
            }
        }

        // sprite[nSprite].sectnum may have changed since we set nSectFlag ?
        short nFlagVal = nSectFlag ^ SectFlag[pSprite->sectnum];
        if (nFlagVal & kSectUnderwater)
        {
            DestroyBullet(nBullet);
            nVal = 1;
        }

        if (nVal == 0 && nType != 15 && nType != 3)
        {
            AddFlash(sprite[nSprite].sectnum, sprite[nSprite].x, sprite[nSprite].y, sprite[nSprite].z, 0);

            if (sprite[nSprite].pal != 5) {
                sprite[nSprite].pal = 1;
            }
        }
    }
    else
    {
        nVal = 1;

        if (BulletList[nBullet].enemy > -1)
        {
            hitsprite = BulletList[nBullet].enemy;
            x2 = sprite[hitsprite].x;
            y2 = sprite[hitsprite].y;
            z2 = sprite[hitsprite].z - (GetSpriteHeight(hitsprite) >> 1);
            hitsect = sprite[hitsprite].sectnum;
        }
        else
        {
            vec3_t startPos = { x, y, z };
            hitdata_t hitData;
            int dz;
            if (bVanilla)
                dz = -bsin(pBullet->field_C, 3);
            else
                dz = -pBullet->field_C * 512;
            hitscan(&startPos, pSprite->sectnum, bcos(pSprite->ang), bsin(pSprite->ang), dz, &hitData, CLIPMASK1);
            x2 = hitData.pos.x;
            y2 = hitData.pos.y;
            z2 = hitData.pos.z;
            hitsprite = hitData.sprite;
            hitsect = hitData.sect;
            hitwall = hitData.wall;
        }

        lasthitx = x2;
        lasthity = y2;
        lasthitz = z2;
        lasthitsect = hitsect;
        lasthitwall = hitwall;
        lasthitsprite = hitsprite;

        if (hitsprite > -1)
        {
HITSPRITE:
            if (pSprite->pal == 5 && sprite[hitsprite].statnum == 100)
            {
                short nPlayer = GetPlayerFromSprite(hitsprite);
                if (!PlayerList[nPlayer].bIsMummified)
                {
                    PlayerList[nPlayer].bIsMummified = true;
                    SetNewWeapon(nPlayer, kWeaponMummified);
                }
            }
            else
            {
                BulletHitsSprite(pBullet, pSprite->owner, hitsprite, x2, y2, z2, hitsect);
            }
        }
        else if (hitwall > -1)
        {
HITWALL:
            if (wall[hitwall].picnum == kEnergy1)
            {
                short nSector = wall[hitwall].nextsector;
                if (nSector > -1)
                {
                    short nDamage = BulletInfo[pBullet->nType].nDamage;
                    if (pBullet->field_13 > 1) {
                        nDamage *= 2;
                    }

                    short nNormal = GetWallNormal(hitwall) & kAngleMask;

                    runlist_DamageEnemy(sector[nSector].extra, nNormal, nDamage);
                }
            }
        }

        if (hitsect > -1) // NOTE: hitsect can be -1. this check wasn't in original code. TODO: demo compatiblity?
        {
            if (hitsprite < 0 && hitwall < 0)
            {
                if ((SectBelow[hitsect] >= 0 && (SectFlag[SectBelow[hitsect]] & kSectUnderwater)) || SectDepth[hitsect])
                {
                    pSprite->x = x2;
                    pSprite->y = y2;
                    pSprite->z = z2;
                    BuildSplash(nSprite, hitsect);
                }
                else
                {
                    BuildAnim(-1, pBulletInfo->field_C, 0, x2, y2, z2, hitsect, 40, pBulletInfo->nFlags);
                }
            }
            else
            {
                if (hitwall >= 0)
                {
                    BackUpBullet(&x2, &y2, pSprite->ang);

                    if (nType != 3 || RandomSize(2) == 0)
                    {
                        int zOffset = RandomSize(8) << 3;

                        if (!RandomBit()) {
                            zOffset = -zOffset;
                        }

                        // draws bullet puff on walls when they're shot
                        BuildAnim(-1, pBulletInfo->field_C, 0, x2, y2, z2 + zOffset + -4096, hitsect, 40, pBulletInfo->nFlags);
                    }
                }
                else
                {
                    pSprite->x = x2;
                    pSprite->y = y2;
                    pSprite->z = z2;

                    mychangespritesect(nSprite, hitsect);
                }

                if (BulletInfo[nType].nRadius)
                {
                    nRadialBullet = nType;

                    runlist_RadialDamageEnemy(nSprite, pBulletInfo->nDamage, pBulletInfo->nRadius);

                    nRadialBullet = -1;

                    AddFlash(pSprite->sectnum, pSprite->x, pSprite->y, pSprite->z, 128);
                }
            }
        }

        DestroyBullet(nBullet);
    }

    return nVal;
}

void SetBulletEnemy(short nBullet, short nEnemy)
{
    if (nBullet >= 0) {
        BulletList[nBullet].enemy = nEnemy;
    }
}

int BuildBullet(short nSprite, int nType, int, int, int val1, int nAngle, int val2, int val3)
{
    Bullet sBullet;
    bulletInfo *pBulletInfo = &BulletInfo[nType];

    if (pBulletInfo->field_4 > 30000)
    {
        if (val2 >= 10000)
        {
            val2 -= 10000;

            short nTargetSprite = val2;
            spritetype *pTargetSprite = &sprite[nTargetSprite];

//			assert(sprite[nTargetSprite].sectnum <= kMaxSectors);

            if (pTargetSprite->cstat & 0x101)
            {
                sBullet.nType = nType;
                sBullet.field_13 = val3;

                sBullet.nSprite = insertsprite(sprite[nSprite].sectnum, 200);
                sprite[sBullet.nSprite].ang = nAngle;

                int nHeight = GetSpriteHeight(nTargetSprite);

                assert(sprite[nTargetSprite].sectnum >= 0 && sprite[nTargetSprite].sectnum < kMaxSectors);

                BulletHitsSprite(&sBullet, nSprite, nTargetSprite, pTargetSprite->x, pTargetSprite->y, pTargetSprite->z - (nHeight >> 1), pTargetSprite->sectnum);
                mydeletesprite(sBullet.nSprite);
                return -1;
            }
            else
            {
                val2 = 0;
            }
        }
    }

    int nBullet = GrabBullet();
    if (nBullet < 0) {
        return -1;
    }

    short nSector;

    if (sprite[nSprite].statnum == 100)
    {
        nSector = nPlayerViewSect[GetPlayerFromSprite(nSprite)];
    }
    else
    {
        nSector = sprite[nSprite].sectnum;
    }

    short nBulletSprite = insertsprite(nSector, 200);
    int nHeight = GetSpriteHeight(nSprite);
    nHeight = nHeight - (nHeight >> 2);

    if (val1 == -1) {
        val1 = -nHeight;
    }

    sprite[nBulletSprite].x = sprite[nSprite].x;
    sprite[nBulletSprite].y = sprite[nSprite].y;
    sprite[nBulletSprite].z = sprite[nSprite].z;

    // why is this done here???
    assert(nBulletSprite >= 0 && nBulletSprite < kMaxSprites);

    Bullet *pBullet = &BulletList[nBullet];

    pBullet->enemy = -1;

    sprite[nBulletSprite].cstat = 0;
    sprite[nBulletSprite].shade = -64;

    if (pBulletInfo->nFlags & 4) {
        sprite[nBulletSprite].pal = 4;
    }
    else {
        sprite[nBulletSprite].pal = 0;
    }

    sprite[nBulletSprite].clipdist = 25;

    short nRepeat = pBulletInfo->xyRepeat;
    if (nRepeat < 0) {
        nRepeat = 30;
    }

    sprite[nBulletSprite].xrepeat = nRepeat;
    sprite[nBulletSprite].yrepeat = nRepeat;
    sprite[nBulletSprite].xoffset = 0;
    sprite[nBulletSprite].yoffset = 0;
    sprite[nBulletSprite].ang = nAngle;
    sprite[nBulletSprite].xvel = 0;
    sprite[nBulletSprite].yvel = 0;
    sprite[nBulletSprite].zvel = 0;
    sprite[nBulletSprite].owner = nSprite;
    sprite[nBulletSprite].lotag = runlist_HeadRun() + 1;
    sprite[nBulletSprite].extra = -1;
    sprite[nBulletSprite].hitag = 0;

//	GrabTimeSlot(3);

    pBullet->field_10 = 0;
    pBullet->field_E = pBulletInfo->field_2;
    pBullet->nFrame  = 0;

    short nSeq;

    if (pBulletInfo->field_8 != -1)
    {
        pBullet->field_12 = 0;
        nSeq = pBulletInfo->field_8;
    }
    else
    {
        pBullet->field_12 = 1;
        nSeq = pBulletInfo->nSeq;
    }

    pBullet->nSeq = nSeq;

    sprite[nBulletSprite].picnum = seq_GetSeqPicnum(nSeq, 0, 0);

    if (nSeq == kSeqBullet) {
        sprite[nBulletSprite].cstat |= 0x8000;
    }

    pBullet->field_C = val2;
    pBullet->nType = nType;
    pBullet->nSprite = nBulletSprite;
    pBullet->field_6 = runlist_AddRunRec(sprite[nBulletSprite].lotag - 1, nBullet | 0xB0000);
    pBullet->field_8 = runlist_AddRunRec(NewRun, nBullet | 0xB0000);
    pBullet->field_13 = val3;
    sprite[nBulletSprite].z += val1;
    sprite[nBulletSprite].backuppos();

    int var_18;

    nSector = sprite[nBulletSprite].sectnum;

    while (sprite[nBulletSprite].z < sector[nSector].ceilingz)
    {
        if (SectAbove[nSector] == -1)
        {
            sprite[nBulletSprite].z = sector[nSector].ceilingz;
            break;
        }

        nSector = SectAbove[nSector];
        mychangespritesect(nBulletSprite, nSector);
    }

    if (val2 < 10000)
    {
        var_18 = (-bsin(val2) * pBulletInfo->field_4) >> 11;
    }
    else
    {
        val2 -= 10000;

        short nTargetSprite = val2;

        if ((unsigned int)pBulletInfo->field_4 > 30000)
        {
            BulletList[nBullet].enemy = nTargetSprite;
        }
        else
        {
            nHeight = GetSpriteHeight(nTargetSprite);

            if (sprite[nTargetSprite].statnum == 100)
            {
                nHeight -= nHeight >> 2;
            }
            else
            {
                nHeight -= nHeight >> 1;
            }

            int var_20 = sprite[nTargetSprite].z - nHeight;

            int x, y;

            if (nSprite != -1 && sprite[nSprite].statnum != 100)
            {
                x = sprite[nTargetSprite].x;
                y = sprite[nTargetSprite].y;

                if (sprite[nTargetSprite].statnum != 100)
                {
                    x += (sprite[nTargetSprite].xvel * 20) >> 6;
                    y += (sprite[nTargetSprite].yvel * 20) >> 6;
                }
                else
                {
                    int nPlayer = GetPlayerFromSprite(nTargetSprite);
                    if (nPlayer > -1)
                    {
                        x += nPlayerDX[nPlayer] * 15;
                        y += nPlayerDY[nPlayer] * 15;
                    }
                }

                x -= sprite[nBulletSprite].x;
                y -= sprite[nBulletSprite].y;

                nAngle = GetMyAngle(x, y);
                sprite[nSprite].ang = nAngle;
            }
            else
            {
                // loc_2ABA3:
                x = sprite[nTargetSprite].x - sprite[nBulletSprite].x;
                y = sprite[nTargetSprite].y - sprite[nBulletSprite].y;
            }

            int nSqrt = lsqrt(y*y + x*x);
            if ((unsigned int)nSqrt > 0)
            {
                var_18 = ((var_20 - sprite[nBulletSprite].z) * pBulletInfo->field_4) / nSqrt;
            }
            else
            {
                var_18 = 0;
            }
        }
    }

    pBullet->z = 0;
    pBullet->x = (sprite[nSprite].clipdist << 2) * bcos(nAngle);
    pBullet->y = (sprite[nSprite].clipdist << 2) * bsin(nAngle);
    BulletList[nBullet].enemy = -1;

    if (MoveBullet(nBullet))
    {
        nBulletSprite = -1;
    }
    else
    {
        pBullet->field_10 = pBulletInfo->field_4;
        pBullet->x = bcos(nAngle, -3) * pBulletInfo->field_4;
        pBullet->y = bsin(nAngle, -3) * pBulletInfo->field_4;
        pBullet->z = var_18 >> 3;
    }

    return nBulletSprite | IntToFixed(nBullet);
}

void FuncBullet(int a, int, int nRun)
{
    short nBullet = RunData[nRun].nVal;
    assert(nBullet >= 0 && nBullet < kMaxBullets);

    short nSeq = SeqOffsets[BulletList[nBullet].nSeq];
    short nSprite = BulletList[nBullet].nSprite;

    int nMessage = a & kMessageMask;

    switch (nMessage)
    {
        case 0x20000:
        {
            short nFlag = FrameFlag[SeqBase[nSeq] + BulletList[nBullet].nFrame];

            seq_MoveSequence(nSprite, nSeq, BulletList[nBullet].nFrame);

            if (nFlag & 0x80)
            {
                BuildAnim(-1, 45, 0, sprite[nSprite].x, sprite[nSprite].y, sprite[nSprite].z, sprite[nSprite].sectnum, sprite[nSprite].xrepeat, 0);
            }

            BulletList[nBullet].nFrame++;
            if (BulletList[nBullet].nFrame >= SeqSize[nSeq])
            {
                if (!BulletList[nBullet].field_12)
                {
                    BulletList[nBullet].nSeq = BulletInfo[BulletList[nBullet].nType].nSeq;
                    BulletList[nBullet].field_12++;
                }

                BulletList[nBullet].nFrame = 0;
            }

            if (BulletList[nBullet].field_E != -1 && --BulletList[nBullet].field_E == 0)
            {
                DestroyBullet(nBullet);
            }
            else
            {
                MoveBullet(nBullet);
            }
            break;
        }

        case 0x90000:
        {
            short nSprite2 = a & 0xFFFF;
            tsprite[nSprite2].statnum = 1000;

            if (BulletList[nBullet].nType == 15)
            {
                seq_PlotArrowSequence(nSprite2, nSeq, BulletList[nBullet].nFrame);
            }
            else
            {
                seq_PlotSequence(nSprite2, nSeq, BulletList[nBullet].nFrame, 0);
                tsprite[nSprite2].owner = -1;
            }
            break;
        }

        case 0xA0000:
            break;

        default:
        {
            Printf("unknown msg %d for bullet\n", nMessage);
            return;
        }
    }
}
END_PS_NS
