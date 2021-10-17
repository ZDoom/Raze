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
#include "input.h"
#include "sound.h"
#include <string.h>
#include <assert.h>

BEGIN_PS_NS

FreeListArray<Snake, kMaxSnakes> SnakeList;

short nPlayerSnake[kMaxPlayers];

FSerializer& Serialize(FSerializer& arc, const char* keyname, Snake& w, Snake* def)
{
    if (arc.BeginObject(keyname))
    {
        arc("enemy", w.nEnemy)
            .Array("sprites", w.nSprites, kSnakeSprites)
            ("sc", w.sC)
            ("run", w.nRun)
            .Array("c", w.c, countof(w.c))
            ("se", w.sE)
            ("player", w.nSnakePlayer)
            .EndObject();
    }
    return arc;
}

void SerializeSnake(FSerializer& arc)
{
    arc("snake", SnakeList);
    arc.Array("playersnake", nPlayerSnake, PlayerCount);
}


void InitSnakes()
{
    SnakeList.Clear();
    memset(nPlayerSnake, 0, sizeof(nPlayerSnake));
}

short GrabSnake()
{
    return SnakeList.Get();
}

void DestroySnake(int nSnake)
{
    short nRun = SnakeList[nSnake].nRun;
    runlist_SubRunRec(nRun);

    for (int i = 0; i < kSnakeSprites; i++)
    {
        short nSprite = SnakeList[nSnake].nSprites[i];
        auto pSprite = &sprite[nSprite];

        runlist_DoSubRunRec(pSprite->lotag - 1);
        runlist_DoSubRunRec(pSprite->owner);

        mydeletesprite(nSprite);
    }

    SnakeList.Release(nSnake);

    if (nSnake == nSnakeCam)
    {
        nSnakeCam = -1;
    }
}

void ExplodeSnakeSprite(int nSprite, short nPlayer)
{
    auto pSprite = &sprite[nSprite];
    short nDamage = BulletInfo[kWeaponStaff].nDamage;

    if (PlayerList[nPlayer].nDouble > 0) {
        nDamage *= 2;
    }

    // take a copy of this, to revert after call to runlist_RadialDamageEnemy()
    short nOwner = pSprite->owner;
    pSprite->owner = PlayerList[nPlayer].nSprite;

    runlist_RadialDamageEnemy(nSprite, nDamage, BulletInfo[kWeaponStaff].nRadius);

    pSprite->owner = nOwner;

    BuildAnim(-1, 23, 0, pSprite->x, pSprite->y, pSprite->z, pSprite->sectnum, 40, 4);

    AddFlash(pSprite->sectnum, pSprite->x, pSprite->y, pSprite->z, 128);

    StopSpriteSound(nSprite);
}

void BuildSnake(short nPlayer, short zVal)
{

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

    uint32_t xDiff = abs(hitx - x);
    uint32_t yDiff = abs(hity - y);

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
        auto pSprite = &sprite[nSprite];
        pSprite->x = hitx;
        pSprite->y = hity;
        pSprite->z = hitz;

        ExplodeSnakeSprite(nSprite, nPlayer);
        mydeletesprite(nSprite);
        return;
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
        if (nSnake == -1) return;

        //		GrabTimeSlot(3);

        short var_24;

        for (int i = 0; i < kSnakeSprites; i++)
        {
            nSprite = insertsprite(nViewSect, 202);
            auto pSprite = &sprite[nSprite];
            assert(nSprite >= 0 && nSprite < kMaxSprites);

            pSprite->owner = nPlayerSprite;
            pSprite->picnum = nPic;

            if (i == 0)
            {
                pSprite->x = sprite[nPlayerSprite].x;
                pSprite->y = sprite[nPlayerSprite].y;
                pSprite->z = sprite[nPlayerSprite].z + zVal;
                pSprite->xrepeat = 32;
                pSprite->yrepeat = 32;
                nViewSect = pSprite->sectnum;
                var_24 = nSprite;
            }
            else
            {
                pSprite->x = sprite[var_24].x;
                pSprite->y = sprite[var_24].y;
                pSprite->z = sprite[var_24].z;
                pSprite->xrepeat = 40 - 3 * i;
                pSprite->yrepeat = 40 - 3 * i;
            }

            pSprite->clipdist = 10;
            pSprite->cstat = 0;
            pSprite->shade = -64;
            pSprite->pal = 0;
            pSprite->xoffset = 0;
            pSprite->yoffset = 0;
            pSprite->ang = sprite[nPlayerSprite].ang;
            pSprite->xvel = 0;
            pSprite->yvel = 0;
            pSprite->zvel = 0;
            pSprite->hitag = 0;
            pSprite->extra = -1;
            pSprite->lotag = runlist_HeadRun() + 1;
            pSprite->backuppos();

            SnakeList[nSnake].nSprites[i] = nSprite;

            pSprite->owner = runlist_AddRunRec(pSprite->lotag - 1, ((nSnake << 8) | i), 0x110000);
        }

        SnakeList[nSnake].nRun = runlist_AddRunRec(NewRun, nSnake, 0x110000);
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
        SnakeList[nSnake].nSnakePlayer = nPlayer;
        nPlayerSnake[nPlayer] = nSnake;

        if (bSnakeCam)
        {
            if (nSnakeCam < 0) {
                nSnakeCam = nSnake;
            }
        }

        D3PlayFX(StaticSound[kSound6], var_24);
    }
}

int FindSnakeEnemy(short nSnake)
{
    short nPlayer = SnakeList[nSnake].nSnakePlayer;
    short nPlayerSprite = PlayerList[nPlayer].nSprite;

    short nSprite = SnakeList[nSnake].nSprites[0]; // CHECKME
    auto pSprite = &sprite[nSprite];

    short nAngle = pSprite->ang;
    short nSector = pSprite->sectnum;

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

void AISnake::Tick(RunListEvent* ev)
{
    short nSnake = RunData[ev->nRun].nObjIndex;
    assert(nSnake >= 0 && nSnake < kMaxSnakes);

    short nSprite = SnakeList[nSnake].nSprites[0];
    auto pSprite = &sprite[nSprite];

    seq_MoveSequence(nSprite, SeqOffsets[kSeqSnakehed], 0);

    short nEnemySprite = SnakeList[nSnake].nEnemy;

    int nMov;
    int zVal;

    if (nEnemySprite < 0)
    {
    SEARCH_ENEMY:
        nMov = movesprite(nSprite,
            600 * bcos(pSprite->ang),
            600 * bsin(pSprite->ang),
            bsin(SnakeList[nSnake].sE, -5),
            0, 0, CLIPMASK1);

        FindSnakeEnemy(nSnake);

        zVal = 0;
    }
    else
    {
        if (!(sprite[nEnemySprite].cstat & 0x101))
        {
            SnakeList[nSnake].nEnemy = -1;
            goto SEARCH_ENEMY;
        }

        zVal = pSprite->z;

        nMov = AngleChase(nSprite, nEnemySprite, 1200, SnakeList[nSnake].sE, 32);

        zVal = pSprite->z - zVal;
    }

    if (nMov)
    {
        short nPlayer = SnakeList[nSnake].nSnakePlayer;
        ExplodeSnakeSprite(SnakeList[nSnake].nSprites[0], nPlayer);

        nPlayerSnake[nPlayer] = -1;
        SnakeList[nSnake].nSnakePlayer = -1;

        DestroySnake(nSnake);
    }
    else
    {
        short nAngle = pSprite->ang;
        int var_30 = -bcos(nAngle, 6);
        int var_34 = -bsin(nAngle, 6);

        int var_20 = SnakeList[nSnake].sE;

        SnakeList[nSnake].sE = (SnakeList[nSnake].sE + 64) & 0x7FF;

        int var_28 = (nAngle + 512) & kAngleMask;
        short nSector = pSprite->sectnum;

        int x = pSprite->x;
        int y = pSprite->y;
        int z = pSprite->z;

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
                -zVal * (i - 1), 0, 0, CLIPMASK1);

            var_20 = (var_20 + 128) & kAngleMask;
        }
    }
}

void AISnake::Draw(RunListEvent* ev)
{
    short nSnake = RunData[ev->nRun].nObjIndex;
    short nSprite = ev->nParam;

    if ((nSnake & 0xFF) == 0) {
        seq_PlotSequence(nSprite, SeqOffsets[kSeqSnakehed], 0, 0);
    }
    else {
        seq_PlotSequence(nSprite, SeqOffsets[kSeqSnakBody], 0, 0);
    }

    mytsprite[nSprite].owner = -1;
}


void FuncSnake(int nObject, int nMessage, int nDamage, int nRun)
{
    AISnake ai;
    runlist_DispatchEvent(&ai, nObject, nMessage, nDamage, nRun);
}
END_PS_NS
