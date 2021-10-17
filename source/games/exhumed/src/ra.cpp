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

FSerializer& Serialize(FSerializer& arc, const char* keyname, RA& w, RA* def)
{
    if (arc.BeginObject(keyname))
    {
        arc ("frame", w.nFrame)
            ("action", w.nAction)
            ("sprite", w.nSprite)
            ("target", w.nTarget)
            ("run", w.nRun)
            ("ata", w.field_A)
            ("atc", w.field_C)
            ("player", w.nPlayer)
            .EndObject();
    }
    return arc;
}


void SerializeRa(FSerializer& arc)
{
    arc.Array("ra", Ra, PlayerCount);
}

void FreeRa(short nPlayer)
{
    int nRun = Ra[nPlayer].nRun;
    int nSprite = Ra[nPlayer].nSprite;
	auto pSprite = &sprite[nSprite];

    runlist_SubRunRec(nRun);
    runlist_DoSubRunRec(pSprite->owner);
    runlist_FreeRun(pSprite->lotag - 1);

    mydeletesprite(nSprite);
}

void BuildRa(short nPlayer)
{
    short nPlayerSprite = PlayerList[nPlayer].nSprite;

    int nSprite = insertsprite(sprite[nPlayerSprite].sectnum, 203);
	auto pSprite = &sprite[nSprite];

    pSprite->cstat = 0x8000;
    pSprite->xvel = 0;
    pSprite->yvel = 0;
    pSprite->zvel = 0;
    pSprite->extra = -1;
    pSprite->lotag = runlist_HeadRun() + 1;
    pSprite->hitag = 0;
    pSprite->owner = runlist_AddRunRec(pSprite->lotag - 1, nPlayer, 0x210000);
    pSprite->pal = 1;
    pSprite->xrepeat = 64;
    pSprite->yrepeat = 64;
    pSprite->x = sprite[nPlayerSprite].x;
    pSprite->y = sprite[nPlayerSprite].y;
    pSprite->z = sprite[nPlayerSprite].z;

//	GrabTimeSlot(3);

    Ra[nPlayer].nSprite = nSprite;

    Ra[nPlayer].nRun = runlist_AddRunRec(NewRun, nPlayer, 0x210000);
    Ra[nPlayer].nTarget = -1;
    Ra[nPlayer].nFrame  = 0;
    Ra[nPlayer].nAction = 0;
    Ra[nPlayer].field_C = 0;
    Ra[nPlayer].nPlayer = nPlayer;
}

void InitRa()
{
    memset(Ra, 0, sizeof(RA) * kMaxPlayers);
}

void MoveRaToEnemy(short nPlayer)
{
    short nTarget = Ra[nPlayer].nTarget;
    short nSprite = Ra[nPlayer].nSprite;
    short nAction = Ra[nPlayer].nAction;
	auto pSprite = &sprite[nSprite];

    if (nTarget != -1)
    {
        if (!(sprite[nTarget].cstat & 0x101) || sprite[nTarget].sectnum == MAXSECTORS)
        {
            Ra[nPlayer].nTarget = -1;
            if (nAction == 0 || nAction == 3) {
                return;
            }

            Ra[nPlayer].nAction = 3;
            Ra[nPlayer].nFrame  = 0;
            return;
        }
        else
        {
            if (pSprite->sectnum != sprite[nTarget].sectnum) {
                mychangespritesect(nSprite, sprite[nTarget].sectnum);
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

        pSprite->cstat = 0x8000;
        nTarget = PlayerList[nPlayer].nSprite;
    }

    pSprite->x = sprite[nTarget].x;
    pSprite->y = sprite[nTarget].y;
    pSprite->z = sprite[nTarget].z - GetSpriteHeight(nTarget);

    if (pSprite->sectnum != sprite[nTarget].sectnum) {
        mychangespritesect(nSprite, sprite[nTarget].sectnum);
    }
}

void AIRa::Tick(RunListEvent* ev)
{
    short nPlayer = RunData[ev->nRun].nObjIndex;
    short nCurrentWeapon = PlayerList[nPlayer].nCurrentWeapon;

    short nSeq = SeqOffsets[kSeqEyeHit] + RaSeq[Ra[nPlayer].nAction].a;
    short nSprite = Ra[nPlayer].nSprite;
    auto pSprite = &sprite[nSprite];

    bool bVal = false;

    Ra[nPlayer].nTarget = sPlayerInput[nPlayer].nTarget;
    pSprite->picnum = seq_GetSeqPicnum2(nSeq, Ra[nPlayer].nFrame);

    if (Ra[nPlayer].nAction)
    {
        seq_MoveSequence(nSprite, nSeq, Ra[nPlayer].nFrame);

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

        if (!Ra[nPlayer].field_C || Ra[nPlayer].nTarget <= -1)
        {
            pSprite->cstat = 0x8000;
        }
        else
        {
            pSprite->cstat &= 0x7FFF;
            Ra[nPlayer].nAction = 1;
            Ra[nPlayer].nFrame = 0;
        }

        return;
    }

    case 1:
    {
        if (!Ra[nPlayer].field_C)
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
            if (Ra[nPlayer].nFrame || Ra[nPlayer].nTarget <= -1)
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
                    runlist_DamageEnemy(Ra[nPlayer].nTarget, PlayerList[Ra[nPlayer].nPlayer].nSprite, BulletInfo[kWeaponRing].nDamage);
                    AddAmmo(nPlayer, kWeaponRing, -WeaponInfo[kWeaponRing].d);
                    SetQuake(nSprite, 100);
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
            pSprite->cstat |= 0x8000;
            Ra[nPlayer].nAction = 0;
            Ra[nPlayer].nFrame = 0;
            Ra[nPlayer].field_C = 0;
        }

        return;
    }

    default:
        return;
    }
}

void AIRa::Draw(RunListEvent* ev)
{
    short nPlayer = RunData[ev->nRun].nObjIndex;
    short nSeq = SeqOffsets[kSeqEyeHit] + RaSeq[Ra[nPlayer].nAction].a;

    seq_PlotSequence(ev->nParam, nSeq, Ra[nPlayer].nFrame, 1);
    mytsprite[ev->nParam].owner = -1;
}

void FuncRa(int nObject, int nMessage, int nDamage, int nRun)
{
    AIRa ai;
    runlist_DispatchEvent(&ai, nObject, nMessage, nDamage, nRun);

}

END_PS_NS
