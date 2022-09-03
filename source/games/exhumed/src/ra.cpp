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
#include "exhumed.h"
#include "player.h"
#include "sequence.h"
#include "input.h"
#include <string.h>

BEGIN_PS_NS

/* bjd - the content of the ra.* files originally resided in gun.c I think... */

RA Ra[kMaxPlayers]; // one Ra for each player

static actionSeq RaSeq[] = {
    {2, 1},
    {0, 0},
    {1, 0},
    {2, 0}
};

size_t MarkRa()
{
    for (auto& r : Ra)
    {
        GC::Mark(r.pActor);
        GC::Mark(r.pTarget);
    }
    return 2 * kMaxPlayers;
}

FSerializer& Serialize(FSerializer& arc, const char* keyname, RA& w, RA* def)
{
    if (arc.BeginObject(keyname))
    {
        arc ("frame", w.nFrame)
            ("action", w.nAction)
            ("sprite", w.pActor)
            ("target", w.pTarget)
            ("run", w.nRun)
            ("atc", w.nState)
            ("player", w.nPlayer)
            .EndObject();
    }
    return arc;
}


void SerializeRa(FSerializer& arc)
{
    arc.Array("ra", Ra, PlayerCount);
}

void FreeRa(int nPlayer)
{
    int nRun = Ra[nPlayer].nRun;
    DExhumedActor* pActor = Ra[nPlayer].pActor;
    if (!pActor) return;

    runlist_SubRunRec(nRun);
    runlist_DoSubRunRec(pActor->spr.intowner);
    runlist_FreeRun(pActor->spr.lotag - 1);

    DeleteActor(pActor);
    Ra[nPlayer] = {};
}

void BuildRa(int nPlayer)
{
    auto pPlayerActor = PlayerList[nPlayer].pActor;

    auto pActor = insertActor(pPlayerActor->sector(), 203);

    pActor->spr.cstat = CSTAT_SPRITE_INVISIBLE;
    pActor->vel.X = 0;
    pActor->vel.Y = 0;
    pActor->vel.Z = 0;
    pActor->spr.extra = -1;
    pActor->spr.lotag = runlist_HeadRun() + 1;
    pActor->spr.hitag = 0;
    pActor->spr.intowner = runlist_AddRunRec(pActor->spr.lotag - 1, nPlayer, 0x210000);
    pActor->spr.pal = 1;
    pActor->spr.xrepeat = 64;
    pActor->spr.yrepeat = 64;
    pActor->spr.pos = pPlayerActor->spr.pos;

//	GrabTimeSlot(3);

    Ra[nPlayer].pActor = pActor;

    Ra[nPlayer].nRun = runlist_AddRunRec(NewRun, nPlayer, 0x210000);
    Ra[nPlayer].pTarget = nullptr;
    Ra[nPlayer].nFrame  = 0;
    Ra[nPlayer].nAction = 0;
    Ra[nPlayer].nState = 0;
    Ra[nPlayer].nPlayer = nPlayer;
}

void InitRa()
{
    memset(Ra, 0, sizeof(RA) * kMaxPlayers);
}

void MoveRaToEnemy(int nPlayer)
{
    DExhumedActor* pTarget = Ra[nPlayer].pTarget;
    DExhumedActor* pActor = Ra[nPlayer].pActor;
    if (!pActor) return;
    int nAction = Ra[nPlayer].nAction;

    if (pTarget)
    {
        if (!(pTarget->spr.cstat & CSTAT_SPRITE_BLOCK_ALL) || pTarget->spr.statnum == MAXSTATUS)
        {
            Ra[nPlayer].pTarget = nullptr;
            if (nAction == 0 || nAction == 3) {
                return;
            }

            Ra[nPlayer].nAction = 3;
            Ra[nPlayer].nFrame  = 0;
            return;
        }
        else
        {
            if (pActor->sector() != pTarget->sector()) {
                ChangeActorSect(pActor, pTarget->sector());
            }
        }
    }
    else
    {
        if (nAction == 1 || nAction == 2)
        {
            Ra[nPlayer].nAction = 3;
            Ra[nPlayer].nFrame  = 0;
            return;
        }

        if (nAction) {
            return;
        }

        pActor->spr.cstat = CSTAT_SPRITE_INVISIBLE;
        pTarget = PlayerList[nPlayer].pActor;
    }

    pActor->spr.pos = pTarget->spr.pos.plusZ(-GetActorHeightF(pTarget));

    if (pActor->sector() != pTarget->sector()) {
        ChangeActorSect(pActor, pTarget->sector());
    }
}

void AIRa::Tick(RunListEvent* ev)
{
    int nPlayer = RunData[ev->nRun].nObjIndex;
    int nCurrentWeapon = PlayerList[nPlayer].nCurrentWeapon;

    int nSeq = SeqOffsets[kSeqEyeHit] + RaSeq[Ra[nPlayer].nAction].a;
    DExhumedActor* pActor = Ra[nPlayer].pActor;
    if (!pActor) return;

    bool bVal = false;

    Ra[nPlayer].pTarget = sPlayerInput[nPlayer].pTarget;
    pActor->spr.picnum = seq_GetSeqPicnum2(nSeq, Ra[nPlayer].nFrame);

    if (Ra[nPlayer].nAction)
    {
        seq_MoveSequence(pActor, nSeq, Ra[nPlayer].nFrame);

        Ra[nPlayer].nFrame++;
        if (Ra[nPlayer].nFrame >= SeqSize[nSeq])
        {
            Ra[nPlayer].nFrame = 0;
            bVal = true;
        }
    }

    switch (Ra[nPlayer].nAction)
    {
    case 0:
    {
        MoveRaToEnemy(nPlayer);

        if (!Ra[nPlayer].nState || Ra[nPlayer].pTarget == nullptr)
        {
            pActor->spr.cstat = CSTAT_SPRITE_INVISIBLE;
        }
        else
        {
            pActor->spr.cstat &= ~CSTAT_SPRITE_INVISIBLE;
            Ra[nPlayer].nAction = 1;
            Ra[nPlayer].nFrame = 0;
        }

        return;
    }

    case 1:
    {
        if (!Ra[nPlayer].nState)
        {
            Ra[nPlayer].nAction = 3;
            Ra[nPlayer].nFrame = 0;
        }
        else
        {
            if (bVal) {
                Ra[nPlayer].nAction = 2;
            }

            MoveRaToEnemy(nPlayer);
        }

        return;
    }

    case 2:
    {
        MoveRaToEnemy(nPlayer);

        if (nCurrentWeapon != kWeaponRing)
        {
            Ra[nPlayer].nAction = 3;
            Ra[nPlayer].nFrame = 0;
        }
        else
        {
            if (Ra[nPlayer].nFrame || Ra[nPlayer].pTarget == nullptr)
            {
                if (!bVal) {
                    return;
                }

                Ra[nPlayer].nAction = 3;
                Ra[nPlayer].nFrame = 0;
            }
            else
            {
                if (PlayerList[nPlayer].nAmmo[kWeaponRing] > 0)
                {
                    runlist_DamageEnemy(Ra[nPlayer].pTarget, PlayerList[Ra[nPlayer].nPlayer].pActor, BulletInfo[kWeaponRing].nDamage);
                    AddAmmo(nPlayer, kWeaponRing, -WeaponInfo[kWeaponRing].d);
                    SetQuake(pActor, 100);
                }
                else
                {
                    Ra[nPlayer].nAction = 3;
                    Ra[nPlayer].nFrame = 0;
                    SelectNewWeapon(nPlayer);
                }
            }
        }

        return;
    }

    case 3:
    {
        if (bVal)
        {
            pActor->spr.cstat |= CSTAT_SPRITE_INVISIBLE;
            Ra[nPlayer].nAction = 0;
            Ra[nPlayer].nFrame = 0;
            Ra[nPlayer].nState = 0;
        }

        return;
    }

    default:
        return;
    }
}

void AIRa::Draw(RunListEvent* ev)
{
    int nPlayer = RunData[ev->nRun].nObjIndex;
    int nSeq = SeqOffsets[kSeqEyeHit] + RaSeq[Ra[nPlayer].nAction].a;

    seq_PlotSequence(ev->nParam, nSeq, Ra[nPlayer].nFrame, 1);
    ev->pTSprite->ownerActor = nullptr;
}


END_PS_NS
