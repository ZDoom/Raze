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
        arc("enemy", w.pEnemy)
			("countdown", w.nCountdown)
            .Array("sprites", w.pSprites, kSnakeSprites)
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
        auto pSnake = SnakeList[nSnake].pSprites[i];
        auto pSprite = &pSnake->s();

        runlist_DoSubRunRec(pSprite->lotag - 1);
        runlist_DoSubRunRec(pSprite->owner);

        DeleteActor(pSnake);
    }

    SnakeList.Release(nSnake);

    if (nSnake == nSnakeCam)
    {
        nSnakeCam = -1;
    }
}

void ExplodeSnakeSprite(DExhumedActor* pActor, short nPlayer)
{
    auto pSprite = &pActor->s();
    short nDamage = BulletInfo[kWeaponStaff].nDamage;

    if (PlayerList[nPlayer].nDouble > 0) {
        nDamage *= 2;
    }

    // take a copy of this, to revert after call to runlist_RadialDamageEnemy()
    auto nOwner = pActor->pTarget;
    pActor->pTarget = PlayerList[nPlayer].pActor;

    runlist_RadialDamageEnemy(pActor, nDamage, BulletInfo[kWeaponStaff].nRadius);

    pActor->pTarget = nOwner;

    BuildAnim(nullptr, 23, 0, pSprite->x, pSprite->y, pSprite->z, pSprite->sectnum, 40, 4);

    AddFlash(pSprite->sectnum, pSprite->x, pSprite->y, pSprite->z, 128);

    StopActorSound(pActor);
}

void BuildSnake(short nPlayer, short zVal)
{

    zVal -= 1280;

    auto pPlayerActor = PlayerList[nPlayer].Actor();
    auto pPlayerSprite = &pPlayerActor->s();
    short nViewSect = PlayerList[nPlayer].nPlayerViewSect;
    short nPic = seq_GetSeqPicnum(kSeqSnakBody, 0, 0);

    int x = pPlayerSprite->x;
    int y = pPlayerSprite->y;
    int z = (pPlayerSprite->z + zVal) - 2560;
    short nAngle = pPlayerSprite->ang;

    short hitsect;
    int hitx, hity, hitz;
	DExhumedActor* hitactor;

    vec3_t pos = { x, y, z };
    hitdata_t hitData;
    hitscan(&pos, pPlayerSprite->sectnum, bcos(nAngle), bsin(nAngle), 0, &hitData, CLIPMASK1);

    hitx = hitData.pos.x;
    hity = hitData.pos.y;
    hitz = hitData.pos.z;
    hitsect = hitData.sect;
    hitactor = hitData.sprite == -1? nullptr : &exhumedActors[hitData.sprite];

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
        auto pActor = insertActor(hitsect, 202);
		auto pSprite = &pActor->s();
        pSprite->x = hitx;
        pSprite->y = hity;
        pSprite->z = hitz;

        ExplodeSnakeSprite(pActor, nPlayer);
        DeleteActor(pActor);
        return;
    }
    else
    {
        DExhumedActor* pTarget = nullptr;

        if (hitactor && hitactor->s().statnum >= 90 && hitactor->s().statnum <= 199) {
            pTarget = hitactor;
        }
        else if (sPlayerInput[nPlayer].pTarget != nullptr) 
        {
            pTarget = sPlayerInput[nPlayer].pTarget;
        }

        short nSnake = GrabSnake();
        if (nSnake == -1) return;

        //		GrabTimeSlot(3);

        DExhumedActor* sprt;

        for (int i = 0; i < kSnakeSprites; i++)
        {
            auto pActor = insertActor(nViewSect, 202);
            auto pSprite = &pActor->s();

			pActor->pTarget = pPlayerActor;
            //pSprite->owner = nPlayerSprite;
            pSprite->picnum = nPic;

            if (i == 0)
            {
                pSprite->x = pPlayerSprite->x;
                pSprite->y = pPlayerSprite->y;
                pSprite->z = pPlayerSprite->z + zVal;
                pSprite->xrepeat = 32;
                pSprite->yrepeat = 32;
                nViewSect = pSprite->sectnum;
                sprt = pActor;
            }
            else
            {
                pSprite->x = sprt->s().x;
                pSprite->y = sprt->s().y;
                pSprite->z = sprt->s().z;
                pSprite->xrepeat = 40 - 3 * i;
                pSprite->yrepeat = 40 - 3 * i;
            }

            pSprite->clipdist = 10;
            pSprite->cstat = 0;
            pSprite->shade = -64;
            pSprite->pal = 0;
            pSprite->xoffset = 0;
            pSprite->yoffset = 0;
            pSprite->ang = pPlayerSprite->ang;
            pSprite->xvel = 0;
            pSprite->yvel = 0;
            pSprite->zvel = 0;
            pSprite->hitag = 0;
            pSprite->extra = -1;
            pSprite->lotag = runlist_HeadRun() + 1;
            pSprite->backuppos();

            SnakeList[nSnake].pSprites[i] = pActor;

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
        SnakeList[nSnake].pEnemy = pTarget;
		SnakeList[nSnake].nCountdown = 0;
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

        D3PlayFX(StaticSound[kSound6], sprt);
    }
}

DExhumedActor* FindSnakeEnemy(short nSnake)
{
    short nPlayer = SnakeList[nSnake].nSnakePlayer;
	auto pPlayerActor = PlayerList[nPlayer].Actor();
	auto pPlayerSprite = &pPlayerActor->s();
	
    auto pActor = SnakeList[nSnake].pSprites[0]; // CHECKME
    auto pSprite = &pActor->s();

    short nAngle = pSprite->ang;
    short nSector = pSprite->sectnum;

    int esi = 2048;

	DExhumedActor* pEnemy = nullptr;

    ExhumedSectIterator it(nSector);
    while (auto pAct2 = it.Next())
    {
		auto pSpr2 = &pAct2->s();
        if (pSpr2->statnum >= 90 && pSpr2->statnum < 150 && (pSpr2->cstat & 0x101))
        {
            if (pAct2 != pPlayerActor && !(pSpr2->cstat & 0x8000))
            {
                int nAngle2 = (nAngle - GetAngleToSprite(pActor, pAct2)) & kAngleMask;
                if (nAngle2 < esi)
                {
                    pEnemy = pAct2;
                    esi = nAngle2;
                }
            }
        }
    }

    if (pEnemy)
    {
        SnakeList[nSnake].pEnemy = pEnemy;
        SnakeList[nSnake].nCountdown = 0;
    }
    else
    {
        SnakeList[nSnake].nCountdown--;
        if (SnakeList[nSnake].nCountdown < -25)
        {
            pEnemy = pPlayerActor;
            SnakeList[nSnake].pEnemy = pPlayerActor;
        }
    }

    return pEnemy;
}

void AISnake::Tick(RunListEvent* ev)
{
    short nSnake = RunData[ev->nRun].nObjIndex;
    assert(nSnake >= 0 && nSnake < kMaxSnakes);

    auto pActor = SnakeList[nSnake].pSprites[0];
    auto pSprite = &pActor->s();

    seq_MoveSequence(pActor, SeqOffsets[kSeqSnakehed], 0);

    auto pEnemySprite = SnakeList[nSnake].pEnemy;

    Collision nMov;
    int zVal;

    if (pEnemySprite == nullptr)
    {
    SEARCH_ENEMY:
        nMov = movesprite(pActor,
            600 * bcos(pSprite->ang),
            600 * bsin(pSprite->ang),
            bsin(SnakeList[nSnake].sE, -5),
            0, 0, CLIPMASK1);

        FindSnakeEnemy(nSnake);

        zVal = 0;
    }
    else
    {
        if (!(pEnemySprite->s().cstat & 0x101))
        {
            SnakeList[nSnake].pEnemy = nullptr;
            goto SEARCH_ENEMY;
        }

        zVal = pSprite->z;

        nMov = AngleChase(pActor, pEnemySprite, 1200, SnakeList[nSnake].sE, 32);

        zVal = pSprite->z - zVal;
    }

    if (nMov.type || nMov.exbits)
    {
        short nPlayer = SnakeList[nSnake].nSnakePlayer;
        ExplodeSnakeSprite(SnakeList[nSnake].pSprites[0], nPlayer);

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
            auto pActor2 = SnakeList[nSnake].pSprites[i];
			auto pSprite2 = &pActor2->s();

            pSprite2->ang = nAngle;
            pSprite2->x = x;
            pSprite2->y = y;
            pSprite2->z = z;

            ChangeActorSect(pActor2, nSector);

            int eax = (bsin(var_20) * SnakeList[nSnake].c[i]) >> 9;

            movesprite(pActor2, var_30 + var_30 * i + eax * bcos(var_28), var_30 + var_34 * i + eax * bsin(var_28),
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

    ev->pTSprite->owner = -1;
}

END_PS_NS
