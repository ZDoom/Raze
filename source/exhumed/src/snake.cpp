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
#include "exhumed.h"
#include "aistuff.h"
#include "status.h"
#include "player.h"
#include "sequence.h"
#include "ps_input.h"
#include "sound.h"
#include <string.h>
#include <assert.h>

BEGIN_PS_NS

enum { kMaxSnakes	= 50 };

int nSnakeCount = 0;
int nSnakesFree;

short SnakeFree[kMaxSnakes];
short nPlayerSnake[kMaxPlayers];

Snake SnakeList[kMaxSnakes];
short nSnakePlayer[kMaxSnakes];

static SavegameHelper sghsnake("snake",
    SV(nSnakeCount),
    SV(nSnakesFree),
    SA(SnakeFree),
    SA(nPlayerSnake),
    SA(SnakeList),
    SA(nSnakePlayer),
    nullptr);

void InitSnakes()
{
    nSnakeCount = 0;

    for (int i = 0; i < kMaxSnakes; i++) {
        SnakeFree[i] = i;
    }

    nSnakesFree = kMaxSnakes;
    memset(nPlayerSnake, 0, sizeof(nPlayerSnake));
}

short GrabSnake()
{
    nSnakesFree--;
    return SnakeFree[nSnakesFree];
}

void DestroySnake(int nSnake)
{
    short nRun = SnakeList[nSnake].nRun;
    runlist_SubRunRec(nRun);

    for (int i = 0; i < kSnakeSprites; i++)
    {
        short nSprite = SnakeList[nSnake].nSprites[i];

        runlist_DoSubRunRec(sprite[nSprite].lotag - 1);
        runlist_DoSubRunRec(sprite[nSprite].owner);

        mydeletesprite(nSprite);
    }

    SnakeFree[nSnakesFree] = nSnake;
    nSnakesFree++;

    if (nSnake == nSnakeCam)
    {
        nSnakeCam = -1;
    }
}

void ExplodeSnakeSprite(int nSprite, short nPlayer)
{
    short nDamage = BulletInfo[kWeaponStaff].nDamage;

    if (nPlayerDouble[nPlayer] > 0) {
        nDamage *= 2;
    }

    // take a copy of this, to revert after call to runlist_RadialDamageEnemy()
    short nOwner = sprite[nSprite].owner;
    sprite[nSprite].owner = PlayerList[nPlayer].nSprite;

    runlist_RadialDamageEnemy(nSprite, nDamage, BulletInfo[kWeaponStaff].nRadius);

    sprite[nSprite].owner = nOwner;

    BuildAnim(-1, 23, 0, sprite[nSprite].x, sprite[nSprite].y, sprite[nSprite].z, sprite[nSprite].sectnum, 40, 4);

    AddFlash(sprite[nSprite].sectnum, sprite[nSprite].x, sprite[nSprite].y, sprite[nSprite].z, 128);

    StopSpriteSound(nSprite);
}

int BuildSnake(short nPlayer, short zVal)
{
    if (!nSnakesFree)
        return -1;

    zVal -= 1280;

    short nPlayerSprite = PlayerList[nPlayer].nSprite;
    short nViewSect = nPlayerViewSect[nPlayer];
    short nPic = seq_GetSeqPicnum(kSeqSnakBody, 0, 0);

    int x = sprite[nPlayerSprite].x;
    int y = sprite[nPlayerSprite].y;
    int z = (sprite[nPlayerSprite].z + zVal) - 2560;
    short nAngle = sprite[nPlayerSprite].ang;

    short hitsect, hitsprite;
    int hitx, hity, hitz;

    short nSprite;

    vec3_t pos = { x, y, z };
    hitdata_t hitData;
    hitscan(&pos, sprite[nPlayerSprite].sectnum, bcos(nAngle), bsin(nAngle), 0, &hitData, CLIPMASK1);

    hitx = hitData.pos.x;
    hity = hitData.pos.y;
    hitz = hitData.pos.z;
    hitsect = hitData.sect;
    hitsprite = hitData.sprite;

    uint32_t xDiff = klabs(hitx - x);
    uint32_t yDiff = klabs(hity - y);

    uint32_t sqrtNum = xDiff * xDiff + yDiff * yDiff;

    if (sqrtNum > INT_MAX)
    {
        DPrintf(DMSG_WARNING, "%s %d: overflow\n", __func__, __LINE__);
        sqrtNum = INT_MAX;
    }

    int nSqrt = ksqrt(sqrtNum);

    if (nSqrt < bsin(512, -4))
    {
        BackUpBullet(&hitx, &hity, nAngle);
        nSprite = insertsprite(hitsect, 202);
        sprite[nSprite].x = hitx;
        sprite[nSprite].y = hity;
        sprite[nSprite].z = hitz;

        ExplodeSnakeSprite(nSprite, nPlayer);
        mydeletesprite(nSprite);
        return -1;
    }
    else
    {
        short nTarget;

        if (hitsprite >= 0 && sprite[hitsprite].statnum >= 90 && sprite[hitsprite].statnum <= 199) {
            nTarget = hitsprite;
        }
        else {
            nTarget = sPlayerInput[nPlayer].nTarget;
        }

        short nSnake = GrabSnake();

//		GrabTimeSlot(3);

        short var_24;

        for (int i = 0; i < kSnakeSprites; i++)
        {
            nSprite = insertsprite(nViewSect, 202);
            assert(nSprite >= 0 && nSprite < kMaxSprites);

            sprite[nSprite].owner = nPlayerSprite;
            sprite[nSprite].picnum = nPic;

            if (i == 0)
            {
                sprite[nSprite].x = sprite[nPlayerSprite].x;
                sprite[nSprite].y = sprite[nPlayerSprite].y;
                sprite[nSprite].z = sprite[nPlayerSprite].z + zVal;
                sprite[nSprite].xrepeat = 32;
                sprite[nSprite].yrepeat = 32;
                nViewSect = sprite[nSprite].sectnum;
                var_24 = nSprite;
            }
            else
            {
                sprite[nSprite].x = sprite[var_24].x;
                sprite[nSprite].y = sprite[var_24].y;
                sprite[nSprite].z = sprite[var_24].z;
                sprite[nSprite].xrepeat = 40 - 3*i;
                sprite[nSprite].yrepeat = 40 - 3*i;
            }

            sprite[nSprite].clipdist = 10;
            sprite[nSprite].cstat = 0;
            sprite[nSprite].shade = -64;
            sprite[nSprite].pal = 0;
            sprite[nSprite].xoffset = 0;
            sprite[nSprite].yoffset = 0;
            sprite[nSprite].ang = sprite[nPlayerSprite].ang;
            sprite[nSprite].xvel = 0;
            sprite[nSprite].yvel = 0;
            sprite[nSprite].zvel = 0;
            sprite[nSprite].hitag = 0;
            sprite[nSprite].extra = -1;
            sprite[nSprite].lotag = runlist_HeadRun() + 1;

            SnakeList[nSnake].nSprites[i] = nSprite;

            sprite[nSprite].owner = runlist_AddRunRec(sprite[nSprite].lotag - 1, ((nSnake << 8) | i) | 0x110000);
        }

        SnakeList[nSnake].nRun = runlist_AddRunRec(NewRun, nSnake | 0x110000);
        SnakeList[nSnake].c[1] = 2;
        SnakeList[nSnake].c[5] = 5;
        SnakeList[nSnake].c[2] = 4;
        SnakeList[nSnake].c[3] = 6;
        SnakeList[nSnake].c[4] = 7;
        SnakeList[nSnake].c[6] = 6;
        SnakeList[nSnake].c[7] = 7;
        SnakeList[nSnake].nEnemy = nTarget;
        SnakeList[nSnake].sC = 1200;
        SnakeList[nSnake].sE = 0;
        nSnakePlayer[nSnake] = nPlayer;
        nPlayerSnake[nPlayer] = nSnake;

        if (bSnakeCam)
        {
            if (nSnakeCam < 0) {
                nSnakeCam = nSnake;
            }
        }

        D3PlayFX(StaticSound[kSound6], var_24);
    }

    return nSprite;
}

int FindSnakeEnemy(short nSnake)
{
    short nPlayer = nSnakePlayer[nSnake];
    short nPlayerSprite = PlayerList[nPlayer].nSprite;

    short nSprite = SnakeList[nSnake].nSprites[0]; // CHECKME

    short nAngle = sprite[nSprite].ang;
    short nSector = sprite[nSprite].sectnum;

    int esi = 2048;

    int nEnemy = -1;

    int i;
    SectIterator it(nSector);
    while ((i = it.NextIndex()) >= 0)
    {
        if (sprite[i].statnum >= 90 && sprite[i].statnum < 150 && (sprite[i].cstat & 0x101))
        {
            if (i != nPlayerSprite && !(sprite[i].cstat & 0x8000))
            {
                int nAngle2 = (nAngle - GetAngleToSprite(nSprite, i)) & kAngleMask;
                if (nAngle2 < esi)
                {
                    nEnemy = i;
                    esi = nAngle2;
                }
            }
        }
    }

    if (nEnemy != -1)
    {
        SnakeList[nSnake].nEnemy = nEnemy;
    }
    else
    {
        SnakeList[nSnake].nEnemy--;
        if (SnakeList[nSnake].nEnemy < -25)
        {
            nEnemy = nPlayerSprite;
            SnakeList[nSnake].nEnemy = nPlayerSprite;
        }
    }

    return nEnemy;
}

void FuncSnake(int a, int, int nRun)
{
    int nMessage = a & kMessageMask;

    switch (nMessage)
    {
        case 0x20000:
        {
            short nSnake = RunData[nRun].nVal;
            assert(nSnake >= 0 && nSnake < kMaxSnakes);

            short nSprite = SnakeList[nSnake].nSprites[0];

            seq_MoveSequence(nSprite, SeqOffsets[kSeqSnakehed], 0);

            short nEnemySprite = SnakeList[nSnake].nEnemy;

            int nMov;
            int zVal;

            if (nEnemySprite < 0)
            {
SEARCH_ENEMY:
                nMov = movesprite(nSprite,
                    600 * bcos(sprite[nSprite].ang),
                    600 * bsin(sprite[nSprite].ang),
                    bsin(SnakeList[nSnake].sE, -5),
                    0, 0, CLIPMASK1);

                FindSnakeEnemy(nSnake);

                zVal = 0;
            }
            else
            {
                if (!(sprite[nEnemySprite].cstat&0x101))
                {
                    SnakeList[nSnake].nEnemy = -1;
                    goto SEARCH_ENEMY;
                }

                zVal = sprite[nSprite].z;

                nMov = AngleChase(nSprite, nEnemySprite, 1200, SnakeList[nSnake].sE, 32);

                zVal = sprite[nSprite].z - zVal;
            }

            if (nMov)
            {
                short nPlayer = nSnakePlayer[nSnake];
                ExplodeSnakeSprite(SnakeList[nSnake].nSprites[0], nPlayer);

                nPlayerSnake[nPlayer] = -1;
                nSnakePlayer[nSnake] = -1;

                DestroySnake(nSnake);
            }
            else
            {
                short nAngle = sprite[nSprite].ang;
                int var_30 = -bcos(nAngle, 6);
                int var_34 = -bsin(nAngle, 6);

                int var_20 = SnakeList[nSnake].sE;

                SnakeList[nSnake].sE = (SnakeList[nSnake].sE + 64) & 0x7FF;

                int var_28 = (nAngle + 512) & kAngleMask;
                short nSector = sprite[nSprite].sectnum;

                int x = sprite[nSprite].x;
                int y = sprite[nSprite].y;
                int z = sprite[nSprite].z;

                for (int i = 7; i > 0; i--)
                {
                    int nSprite2 = SnakeList[nSnake].nSprites[i];

                    sprite[nSprite2].ang = nAngle;
                    sprite[nSprite2].x = x;
                    sprite[nSprite2].y = y;
                    sprite[nSprite2].z = z;

                    mychangespritesect(nSprite2, nSector);

                    int eax = (bsin(var_20) * SnakeList[nSnake].c[i]) >> 9;

                    movesprite(nSprite2, var_30 + var_30 * i + eax * bcos(var_28), var_30 + var_34 * i + eax * bsin(var_28),
                        -zVal*(i-1), 0, 0, CLIPMASK1);

                    var_20 = (var_20 + 128) & kAngleMask;
                }
            }
            break;
        }

        case 0x90000:
        {
            short nSnake = RunData[nRun].nVal;
            short nSprite = a & 0xFFFF;

            if ((nSnake & 0xFF) == 0) {
                seq_PlotSequence(nSprite, SeqOffsets[kSeqSnakehed], 0, 0);
            }
            else {
                seq_PlotSequence(nSprite, SeqOffsets[kSeqSnakBody], 0, 0);
            }

            tsprite[nSprite].owner = -1;
            break;
        }

        case 0xA0000:
        {
            break;
        }

        default:
        {
            Printf("unknown msg %x for bullet\n", nMessage);
            break;
        }
    }
}
END_PS_NS
