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
    runlist_DoSubRunRec(pActor->spr.lotag - 1);

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

    DExhumedActor* pActor = PlayerList[nPlayer].pPlayerGrenade;
    auto pPlayerActor = PlayerList[nPlayer].pActor;

    int nAngle = pPlayerActor->int_ang();

    ChangeActorSect(pActor, PlayerList[nPlayer].pPlayerViewSect);

    pActor->set_int_pos(pPlayerActor->int_pos());

    if (nAngle < 0) {
        nAngle = pPlayerActor->int_ang();
    }

    pActor->spr.cstat &= ~CSTAT_SPRITE_INVISIBLE;
    pActor->set_int_ang(nAngle);

    if (push1 >= -3000)
    {
        int nVel = PlayerList[nPlayer].totalvel << 5;

        pActor->nTurn = ((90 - pActor->nIndex2) * (90 - pActor->nIndex2)) + nVel;
        pActor->spr.zvel = (-64 * push1) - 4352;

        auto nMov = movesprite(pActor, bcos(nAngle) * (pPlayerActor->spr.clipdist << 3), bsin(nAngle) * (pPlayerActor->spr.clipdist << 3), ecx, 0, 0, CLIPMASK1);
        if (nMov.type == kHitWall)
        {
            nAngle = GetWallNormal(nMov.hitWall);
            BounceGrenade(pActor, nAngle);
        }
    }
    else
    {
        pActor->nTurn = 0;
        pActor->spr.zvel = pPlayerActor->spr.zvel;
    }

    pActor->x = bcos(nAngle, -4) * pActor->nTurn;
    pActor->y = bsin(nAngle, -4) * pActor->nTurn;

    PlayerList[nPlayer].pPlayerGrenade = nullptr;

    return;
}

void BuildGrenade(int nPlayer)
{
    auto pActor = insertActor(PlayerList[nPlayer].pPlayerViewSect, 201);

	auto pPlayerActor = PlayerList[nPlayer].pActor;

    pActor->set_int_pos({ pPlayerActor->int_pos().X, pPlayerActor->int_pos().Y, pPlayerActor->int_pos().Z - 3840 });
    pActor->spr.shade = -64;
    pActor->spr.xrepeat = 20;
    pActor->spr.yrepeat = 20;
    pActor->spr.cstat = CSTAT_SPRITE_INVISIBLE;
    pActor->spr.picnum = 1;
    pActor->spr.pal = 0;
    pActor->spr.clipdist = 30;
    pActor->spr.xoffset = 0;
    pActor->spr.yoffset = 0;
    pActor->spr.angle = pPlayerActor->spr.angle;
    pActor->spr.intowner = nPlayer;
    pActor->spr.xvel = 0;
    pActor->spr.yvel = 0;
    pActor->spr.zvel = 0;
    pActor->spr.hitag = 0;
    pActor->spr.lotag = runlist_HeadRun() + 1;
    pActor->spr.extra = -1;
    pActor->backuppos();

//	GrabTimeSlot(3);

    pActor->nIndex2 = 90;
    pActor->nHealth = 0;
    pActor->nCount = 16;
    pActor->nTurn = -1;
    pActor->nIndex = 0;
    pActor->nFrame = 0;
    pActor->nPhase = runlist_AddRunRec(pActor->spr.lotag - 1, pActor, 0x0F0000);
    pActor->nRun = runlist_AddRunRec(NewRun, pActor, 0x0F0000);

    PlayerList[nPlayer].pPlayerGrenade = pActor;
}

void ExplodeGrenade(DExhumedActor* pActor)
{
    int var_28, var_20;

    int nPlayer = pActor->spr.intowner;
    auto pGrenadeSect = pActor->sector();

    pActor->nFrame = 1;

    if (pGrenadeSect->Flag & kSectUnderwater)
    {
        var_28 = 75;
        var_20 = 60;
    }
    else
    {
        if (pActor->int_pos().Z < pGrenadeSect->int_floorz())
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
        auto pPlayerActor = PlayerList[nPlayer].pActor;
        int nAngle = pPlayerActor->int_ang();

        pActor->set_int_pos({ bcos(nAngle, -5) + pPlayerActor->int_pos().X, bsin(nAngle, -5) + pPlayerActor->int_pos().Y, pPlayerActor->int_pos().Z });

        ChangeActorSect(pActor, pPlayerActor->sector());

        if (!PlayerList[nPlayer].invincibility) {
            PlayerList[nPlayer].nHealth = 1;
        }
    }

    int nDamage = BulletInfo[kWeaponGrenade].nDamage;

    if (PlayerList[nPlayer].nDouble > 0) {
        nDamage *= 2;
    }

    runlist_RadialDamageEnemy(pActor, nDamage, BulletInfo[kWeaponGrenade].nRadius);

    BuildAnim(nullptr, var_28, 0, pActor->int_pos().X, pActor->int_pos().Y, pActor->int_pos().Z, pActor->sector(), var_20, 4);
    AddFlash(pActor->sector(), pActor->int_pos().X, pActor->int_pos().Y, pActor->int_pos().Z, 128);

    DestroyGrenade(pActor);
}

void AIGrenade::Draw(RunListEvent* ev)
{
    auto pActor = ev->pObjActor;
    if (!pActor) return;

    int nSeq = pActor->nFrame ? SeqOffsets[kSeqGrenBoom] : SeqOffsets[kSeqGrenRoll] + pActor->nIndex;
    seq_PlotSequence(ev->nParam, nSeq, pActor->nHealth >> 8, 1);
}


void AIGrenade::Tick(RunListEvent* ev)
{
    auto pActor = ev->pObjActor;
    if (!pActor) return;

    int nSeq = pActor->nFrame ? SeqOffsets[kSeqGrenBoom] : SeqOffsets[kSeqGrenRoll] + pActor->nIndex;

    seq_MoveSequence(pActor, nSeq, pActor->nHealth >> 8);
    pActor->spr.picnum = seq_GetSeqPicnum2(nSeq, pActor->nHealth >> 8);

    pActor->nIndex2--;
    if (!pActor->nIndex2)
    {
        int nPlayer = pActor->spr.intowner;

        if (pActor->nTurn < 0)
        {
            PlayerList[nPlayer].nState = 0;
            PlayerList[nPlayer].nSeqSize2 = 0;

            if (PlayerList[nPlayer].nAmmo[kWeaponGrenade])
            {
                PlayerList[nPlayer].bIsFiring = false;
            }
            else
            {
                SelectNewWeapon(nPlayer);

                PlayerList[nPlayer].nCurrentWeapon = PlayerList[nPlayer].nNextWeapon;
                PlayerList[nPlayer].nNextWeapon = -1;
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

        int zVel = pActor->spr.zvel;

        Gravity(pActor);
        auto nMov = movesprite(pActor, pActor->x, pActor->y, pActor->spr.zvel, pActor->spr.clipdist >> 1, pActor->spr.clipdist >> 1, CLIPMASK1);

        if (!nMov.type && !nMov.exbits)
            return;

        if (nMov.exbits & kHitAux2)
        {
            if (zVel)
            {
                if (pActor->sector()->Damage > 0)
                {
                    ExplodeGrenade(pActor);
                    return;
                }

                pActor->nCount = (uint8_t)totalmoves; // limit to 8bits?

                D3PlayFX(StaticSound[kSound3], pActor);

                pActor->spr.zvel = -(zVel >> 1);

                if (pActor->spr.zvel > -1280)
                {
                    D3PlayFX(StaticSound[kSound5], pActor);
                    pActor->nCount = 0;
                    pActor->nHealth = 0;
                    pActor->spr.zvel = 0;
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
            BounceGrenade(pActor, GetWallNormal(nMov.hitWall));
        }
        else if (nMov.type == kHitSprite)
        {
            BounceGrenade(pActor, nMov.actor()->int_ang());
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
