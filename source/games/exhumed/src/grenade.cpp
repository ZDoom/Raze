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
#include "aistuff.h"
#include "engine.h"
#include "player.h"
#include "exhumed.h"
#include "sound.h"
#include "sequence.h"
#include <assert.h>

BEGIN_PS_NS



void DestroyGrenade(DExhumedActor* pActor)
{
    runlist_DoSubRunRec(pActor->nPhase);
    runlist_SubRunRec(pActor->nRun);
    runlist_DoSubRunRec(pActor->s().lotag - 1);

    DeleteActor(pActor);
}

void BounceGrenade(DExhumedActor* pActor, int nAngle)
{
    pActor->nTurn >>= 1;

    pActor->x = bcos(nAngle, -5) * pActor->nTurn;
    pActor->y = bsin(nAngle, -5) * pActor->nTurn;

    D3PlayFX(StaticSound[kSound3], pActor);
}

void ThrowGrenade(int nPlayer, int, int, int ecx, int push1)
{
    if (PlayerList[nPlayer].pPlayerGrenade == nullptr)
        return;

    auto pActor = PlayerList[nPlayer].pPlayerGrenade;
    auto pGrenadeSprite = &pActor->s();
    auto pPlayerActor = PlayerList[nPlayer].Actor();
    auto pPlayerSprite = &pPlayerActor->s();

    int nAngle = pPlayerSprite->ang;

    ChangeActorSect(pActor, PlayerList[nPlayer].nPlayerViewSect);

    pGrenadeSprite->x = pPlayerSprite->x;
    pGrenadeSprite->y = pPlayerSprite->y;
    pGrenadeSprite->z = pPlayerSprite->z;

    if (nAngle < 0) {
        nAngle = pPlayerSprite->ang;
    }

    pGrenadeSprite->cstat &= 0x7FFF;
    pGrenadeSprite->ang = nAngle;

    if (push1 >= -3000)
    {
        int nVel = PlayerList[nPlayer].totalvel << 5;

        pActor->nTurn = ((90 - pActor->nIndex2) * (90 - pActor->nIndex2)) + nVel;
        pGrenadeSprite->zvel = (-64 * push1) - 4352;

        auto nMov = movesprite(pActor, bcos(nAngle) * (pPlayerSprite->clipdist << 3), bsin(nAngle) * (pPlayerSprite->clipdist << 3), ecx, 0, 0, CLIPMASK1);
        if (nMov.type == kHitWall)
        {
            nAngle = GetWallNormal(nMov.index);
            BounceGrenade(pActor, nAngle);
        }
    }
    else
    {
        pActor->nTurn = 0;
        pGrenadeSprite->zvel = pPlayerSprite->zvel;
    }

    pActor->x = bcos(nAngle, -4) * pActor->nTurn;
    pActor->y = bsin(nAngle, -4) * pActor->nTurn;

    PlayerList[nPlayer].pPlayerGrenade = nullptr;

    return;
}

void BuildGrenade(int nPlayer)
{
    auto pActor = insertActor(PlayerList[nPlayer].nPlayerViewSect, 201);
	auto pSprite = &pActor->s();

	auto pPlayerSprite = &PlayerList[nPlayer].Actor()->s();

    pSprite->x = pPlayerSprite->x;
    pSprite->y = pPlayerSprite->y;
    pSprite->z = pPlayerSprite->z - 3840;
    pSprite->shade = -64;
    pSprite->xrepeat = 20;
    pSprite->yrepeat = 20;
    pSprite->cstat = 0x8000;
    pSprite->picnum = 1;
    pSprite->pal = 0;
    pSprite->clipdist = 30;
    pSprite->xoffset = 0;
    pSprite->yoffset = 0;
    pSprite->ang = pPlayerSprite->ang;
    pSprite->owner = nPlayer;
    pSprite->xvel = 0;
    pSprite->yvel = 0;
    pSprite->zvel = 0;
    pSprite->hitag = 0;
    pSprite->lotag = runlist_HeadRun() + 1;
    pSprite->extra = -1;
    pSprite->backuppos();

//	GrabTimeSlot(3);

    pActor->nIndex2 = 90;
    pActor->nHealth = 0;
    pActor->nCount = 16;
    pActor->nTurn = -1;
    pActor->nIndex = 0;
    pActor->nFrame = 0;
    pActor->nPhase = runlist_AddRunRec(pSprite->lotag - 1, pActor, 0x0F0000);
    pActor->nRun = runlist_AddRunRec(NewRun, pActor, 0x0F0000);

    PlayerList[nPlayer].pPlayerGrenade = pActor;
}

void ExplodeGrenade(DExhumedActor* pActor)
{
    int var_28, var_20;

    auto pGrenadeSprite = &pActor->s();
    int nPlayer = pGrenadeSprite->owner;
    int nGrenadeSect = pGrenadeSprite->sectnum;

    pActor->nFrame = 1;

    if (SectFlag[nGrenadeSect] & kSectUnderwater)
    {
        var_28 = 75;
        var_20 = 60;
    }
    else
    {
        if (pGrenadeSprite->z < sector[nGrenadeSect].floorz)
        {
            var_20 = 200;
            var_28 = 36;

// TODO		MonoOut("GRENPOW\n");
        }
        else
        {
            var_28 = 34;
            var_20 = 150;

// TODO		MonoOut("GRENBOOM\n");
        }
    }

    if (pActor->nTurn < 0)
    {
        auto pPlayerActor = PlayerList[nPlayer].Actor();
		auto pPlayerSprite = &pPlayerActor->s();
        int nAngle = pPlayerSprite->ang;

        pGrenadeSprite->z = pPlayerSprite->z;
        pGrenadeSprite->x = bcos(nAngle, -5) + pPlayerSprite->x;
        pGrenadeSprite->y = bsin(nAngle, -5) + pPlayerSprite->y;

        ChangeActorSect(pActor, pPlayerSprite->sectnum);

        if (!PlayerList[nPlayer].invincibility) {
            PlayerList[nPlayer].nHealth = 1;
        }
    }

    short nDamage = BulletInfo[kWeaponGrenade].nDamage;

    if (PlayerList[nPlayer].nDouble > 0) {
        nDamage *= 2;
    }

    runlist_RadialDamageEnemy(pActor, nDamage, BulletInfo[kWeaponGrenade].nRadius);

    BuildAnim(nullptr, var_28, 0, pGrenadeSprite->x, pGrenadeSprite->y, pGrenadeSprite->z, pGrenadeSprite->sectnum, var_20, 4);
    AddFlash(pGrenadeSprite->sectnum, pGrenadeSprite->x, pGrenadeSprite->y, pGrenadeSprite->z, 128);

    DestroyGrenade(pActor);
}

void AIGrenade::Draw(RunListEvent* ev)
{
    auto pActor = ev->pObjActor;
    if (!pActor) return;

    short nSeq = pActor->nFrame ? SeqOffsets[kSeqGrenBoom] : SeqOffsets[kSeqGrenRoll] + pActor->nIndex;
    seq_PlotSequence(ev->nParam, nSeq, pActor->nHealth >> 8, 1);
}


void AIGrenade::Tick(RunListEvent* ev)
{
    auto pActor = ev->pObjActor;
    if (!pActor) return;

    auto pGrenadeSprite = &pActor->s();
    short nSeq = pActor->nFrame ? SeqOffsets[kSeqGrenBoom] : SeqOffsets[kSeqGrenRoll] + pActor->nIndex;

    seq_MoveSequence(pActor, nSeq, pActor->nHealth >> 8);
    pGrenadeSprite->picnum = seq_GetSeqPicnum2(nSeq, pActor->nHealth >> 8);

    pActor->nIndex2--;
    if (!pActor->nIndex2)
    {
        int nPlayer = pGrenadeSprite->owner;

        if (pActor->nTurn < 0)
        {
            PlayerList[nPlayer].field_3A = 0;
            PlayerList[nPlayer].field_3FOUR = 0;

            if (PlayerList[nPlayer].nAmmo[kWeaponGrenade])
            {
                PlayerList[nPlayer].bIsFiring = false;
            }
            else
            {
                SelectNewWeapon(nPlayer);

                PlayerList[nPlayer].nCurrentWeapon = PlayerList[nPlayer].field_38;
                PlayerList[nPlayer].field_38 = -1;
            }
        }

        ExplodeGrenade(pActor);
        return;
    }
    else
    {
        if (pActor->nTurn < 0) {
            return;
        }

        int ebp = (pActor->nHealth + pActor->nCount) >> 8;

        pActor->nHealth += pActor->nCount;

        if (ebp < 0)
        {
            pActor->nHealth += SeqSize[nSeq] << 8;
        }
        else
        {
            if (ebp >= SeqSize[nSeq])
            {
                if (pActor->nFrame)
                {
                    DestroyGrenade(pActor);
                    return;
                }
                else
                {
                    pActor->nHealth = pActor->nFrame;
                }
            }
        }

        if (pActor->nFrame) {
            return;
        }

        int zVel = pGrenadeSprite->zvel;

        Gravity(pActor);
        auto nMov = movesprite(pActor, pActor->x, pActor->y, pGrenadeSprite->zvel, pGrenadeSprite->clipdist >> 1, pGrenadeSprite->clipdist >> 1, CLIPMASK1);

        if (!nMov.type && !nMov.exbits)
            return;

        if (nMov.exbits & kHitAux2)
        {
            if (zVel)
            {
                if (SectDamage[pGrenadeSprite->sectnum] > 0)
                {
                    ExplodeGrenade(pActor);
                    return;
                }

                pActor->nCount = (uint8_t)totalmoves; // limit to 8bits?

                D3PlayFX(StaticSound[kSound3], pActor);

                pGrenadeSprite->zvel = -(zVel >> 1);

                if (pGrenadeSprite->zvel > -1280)
                {
                    D3PlayFX(StaticSound[kSound5], pActor);
                    pActor->nCount = 0;
                    pActor->nHealth = 0;
                    pGrenadeSprite->zvel = 0;
                    pActor->nIndex = 1;
                }
            }

            pActor->nCount = 255 - (RandomByte() * 2);
            pActor->x -= (pActor->x >> 4);
            pActor->y -= (pActor->y >> 4);
        }

        // loc_2CF60:
        if (nMov.type == kHitWall)
        {
            BounceGrenade(pActor, GetWallNormal(nMov.index));
        }
        else if (nMov.type == kHitSprite)
        {
            BounceGrenade(pActor, nMov.actor->s().ang);
        }

        pActor->nHealth = 0;
    }
}

void AIGrenade::RadialDamage(RunListEvent* ev)
{
    auto pActor = ev->pObjActor;
    if (!pActor) return;

    if (pActor != ev->pRadialActor && !pActor->nFrame)
    {
        if (runlist_CheckRadialDamage(pActor) > 280)
        {
            pActor->nIndex2 = RandomSize(4) + 1;
        }
    }
}


END_PS_NS
