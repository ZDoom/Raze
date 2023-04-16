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
#include "sound.h"
#include <string.h>
#include <assert.h>

BEGIN_PS_NS

FreeListArray<Snake, kMaxSnakes> SnakeList;

int16_t nPlayerSnake[kMaxPlayers];

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

size_t MarkSnake()
{
    for (int i = 0; i < kMaxSnakes; i++)
    {
        GC::Mark(SnakeList[i].pEnemy);
        GC::MarkArray(SnakeList[i].pSprites, kSnakeSprites);
    }
    return kMaxSnakes * (1 + kSnakeSprites);
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

FSerializer& Serialize(FSerializer& arc, const char* keyname, Snake& w, Snake* def)
{
    if (arc.BeginObject(keyname))
    {
        arc("enemy", w.pEnemy)
			("countdown", w.nCountdown)
            .Array("sprites", w.pSprites, kSnakeSprites)
            ("run", w.nRun)
            .Array("c", w.c, countof(w.c))
            ("se", w.nAngle)
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

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void InitSnakes()
{
    SnakeList.Clear();
    memset(nPlayerSnake, 0, sizeof(nPlayerSnake));
}

int GrabSnake()
{
    return SnakeList.Get();
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void DestroySnake(int nSnake)
{
    int nRun = SnakeList[nSnake].nRun;
    runlist_SubRunRec(nRun);

    for (int i = 0; i < kSnakeSprites; i++)
    {
        DExhumedActor* pSnake = SnakeList[nSnake].pSprites[i];
        if (!pSnake) continue;

        runlist_DoSubRunRec(pSnake->spr.lotag - 1);
        runlist_DoSubRunRec(pSnake->spr.intowner);

        DeleteActor(pSnake);
    }

    SnakeList.Release(nSnake);

    if (nSnake == nSnakeCam)
    {
        nSnakeCam = -1;
    }
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void ExplodeSnakeSprite(DExhumedActor* pActor, int nPlayer)
{
    int nDamage = BulletInfo[kWeaponStaff].nDamage;

    if (PlayerList[nPlayer].nDouble > 0) {
        nDamage *= 2;
    }

    // take a copy of this, to revert after call to runlist_RadialDamageEnemy()
    DExhumedActor* nOwner = pActor->pTarget;
    pActor->pTarget = PlayerList[nPlayer].pActor;

    runlist_RadialDamageEnemy(pActor, nDamage, BulletInfo[kWeaponStaff].nRadius);

    pActor->pTarget = nOwner;

    BuildAnim(nullptr, "cobrapow", 0, pActor->spr.pos, pActor->sector(), 0.625, 4);

    AddFlash(pActor->sector(), pActor->spr.pos, 128);

    StopActorSound(pActor);
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void BuildSnake(int nPlayer, double zVal)
{
    zVal -= 5;

    auto pPlayerActor = PlayerList[nPlayer].pActor;
    auto pViewSect = PlayerList[nPlayer].pPlayerViewSect;
    int nPic = seq_GetSeqPicnum(kSeqSnakBody, 0, 0);

	auto pos = pPlayerActor->spr.pos.plusZ(zVal - 10);

    HitInfo hit{};
    hitscan(pos, pPlayerActor->sector(), DVector3(pPlayerActor->spr.Angles.Yaw.ToVector() * 1024, 0), hit, CLIPMASK1);

	double nSize = (hit.hitpos.XY() - pos.XY()).Length();

    if (nSize < 64)
    {
		hit.hitpos -= pPlayerActor->spr.Angles.Yaw.ToVector() * 0.5;
        auto pActor = insertActor(hit.hitSector, 202);
        pActor->spr.pos = hit.hitpos;

        ExplodeSnakeSprite(pActor, nPlayer);
        DeleteActor(pActor);
        return;
    }
    else
    {
        DExhumedActor* pTarget = nullptr;
        auto hitactor = hit.actor();
        if (hitactor && hitactor->spr.statnum >= 90 && hitactor->spr.statnum <= 199) {
            pTarget = hitactor;
        }
        else if (PlayerList[nPlayer].pTarget != nullptr)
        {
            pTarget = PlayerList[nPlayer].pTarget;
        }

        int nSnake = GrabSnake();
        if (nSnake == -1) return;

        //		GrabTimeSlot(3);

        DExhumedActor* sprt = nullptr;

        for (int i = 0; i < kSnakeSprites; i++)
        {
            auto pActor = insertActor(pViewSect, 202);

			pActor->pTarget = pPlayerActor;
            //pActor->spr.intowner = nPlayerSprite;
            pActor->spr.picnum = nPic;

            if (i == 0)
            {
                pActor->spr.pos = pPlayerActor->spr.pos.plusZ(zVal);
                pActor->spr.scale = DVector2(0.5, 0.5);
                pViewSect = pActor->sector();
                sprt = pActor;
            }
            else
            {
				pActor->spr.pos = sprt->spr.pos;
				double s = 0.625 + 0.046875 * i;
				pActor->spr.scale = DVector2(s, s);
            }

			pActor->clipdist = 2.5;
            pActor->spr.cstat = 0;
            pActor->spr.shade = -64;
            pActor->spr.pal = 0;
            pActor->spr.xoffset = 0;
            pActor->spr.yoffset = 0;
            pActor->spr.Angles.Yaw = pPlayerActor->spr.Angles.Yaw;
            pActor->vel.X = 0;
            pActor->vel.Y = 0;
            pActor->vel.Z = 0;
            pActor->spr.hitag = 0;
            pActor->spr.extra = -1;
            pActor->spr.lotag = runlist_HeadRun() + 1;
            pActor->backuppos();

            SnakeList[nSnake].pSprites[i] = pActor;

            pActor->spr.intowner = runlist_AddRunRec(pActor->spr.lotag - 1, ((nSnake << 8) | i), 0x110000);
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
        SnakeList[nSnake].nAngle = 0;
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

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

DExhumedActor* FindSnakeEnemy(int nSnake)
{
    int nPlayer = SnakeList[nSnake].nSnakePlayer;
	auto pPlayerActor = PlayerList[nPlayer].pActor;

    DExhumedActor* pActor = SnakeList[nSnake].pSprites[0]; // CHECKME
    if (!pActor) return nullptr;

    DAngle nAngle = pActor->spr.Angles.Yaw;
    auto pSector =pActor->sector();

    DAngle maxangle = DAngle360;

	DExhumedActor* pEnemy = nullptr;

    ExhumedSectIterator it(pSector);
    while (auto pAct2 = it.Next())
    {
        if (pAct2->spr.statnum >= 90 && pAct2->spr.statnum < 150 && (pAct2->spr.cstat & CSTAT_SPRITE_BLOCK_ALL))
        {
            if (pAct2 != pPlayerActor && !(pAct2->spr.cstat & CSTAT_SPRITE_INVISIBLE))
            {
                DAngle nAngle2 = absangle(nAngle, (pAct2->spr.pos.XY() - pActor->spr.pos.XY()).Angle());
                if (nAngle2 < maxangle)
                {
                    pEnemy = pAct2;
                    maxangle = nAngle2;
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

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void AISnake::Tick(RunListEvent* ev)
{
    int nSnake = RunData[ev->nRun].nObjIndex;
    assert(nSnake >= 0 && nSnake < kMaxSnakes);

    DExhumedActor* pActor = SnakeList[nSnake].pSprites[0];
    if (!pActor) return;

    playFrameSound(pActor, getSequence("snakehed")[0]);

    DExhumedActor* pEnemySprite = SnakeList[nSnake].pEnemy;

    Collision nMov;
    double zVal;

    if (pEnemySprite == nullptr)
    {
    SEARCH_ENEMY:
        auto vec = pActor->spr.Angles.Yaw.ToVector() * 37.5;
        nMov = movesprite(pActor, vec, BobVal(SnakeList[nSnake].nAngle) * 2, 0, CLIPMASK1);

        FindSnakeEnemy(nSnake);

        zVal = 0;
    }
    else
    {
        if (!(pEnemySprite->spr.cstat & CSTAT_SPRITE_BLOCK_ALL))
        {
            SnakeList[nSnake].pEnemy = nullptr;
            goto SEARCH_ENEMY;
        }

        zVal = pActor->spr.pos.Z;

        nMov = AngleChase(pActor, pEnemySprite, 1200, SnakeList[nSnake].nAngle, DAngle22_5 / 4);

        zVal = pActor->spr.pos.Z - zVal;
    }

    if (nMov.type || nMov.exbits)
    {
        int nPlayer = SnakeList[nSnake].nSnakePlayer;
        ExplodeSnakeSprite(SnakeList[nSnake].pSprites[0], nPlayer);

        nPlayerSnake[nPlayer] = -1;
        SnakeList[nSnake].nSnakePlayer = -1;

        DestroySnake(nSnake);
    }
    else
    {
        DAngle nAngle = pActor->spr.Angles.Yaw;
        double cosang = -nAngle.Cos() * 4;
        double sinang = -nAngle.Sin() * 4;

        DAngle snakeang = DAngle::fromBuild(SnakeList[nSnake].nAngle);

        SnakeList[nSnake].nAngle = (SnakeList[nSnake].nAngle + 64) & 0x7FF;

        DAngle normalang = (nAngle + DAngle90);
        auto pSector = pActor->sector();

        for (int i = 7; i > 0; i--)
        {
            DExhumedActor* pActor2 = SnakeList[nSnake].pSprites[i];
            if (!pActor2) continue;

            pActor2->spr.Angles.Yaw = nAngle;
			pActor2->spr.pos = pActor->spr.pos;

            ChangeActorSect(pActor2, pSector);

            double eax = snakeang.Sin() * 2 * SnakeList[nSnake].c[i];

            DVector2 vect;
            vect.X = cosang + cosang * i + eax * normalang.Cos();
            vect.Y = cosang + sinang * i + eax * normalang.Sin();
            movesprite(pActor2, vect, - zVal * (i - 1), 0, CLIPMASK1);

            snakeang += DAngle22_5;
        }
    }
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void AISnake::Draw(RunListEvent* ev)
{
    const int nSnake = RunData[ev->nRun].nObjIndex;
    seq_PlotSequence(ev->nParam, (nSnake & 0xFF) == 0 ? "snakehed" : "snakbody", 0, 0, 0);

    ev->pTSprite->ownerActor = nullptr;
}

END_PS_NS
