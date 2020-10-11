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
#include "ps_input.h"
#include <string.h>

BEGIN_PS_NS

/* bjd - the content of the ra.* files originally resided in gun.c I think... */

RA Ra[kMaxPlayers]; // one Ra for each player
short RaCount;

static actionSeq RaSeq[] = {
    {2, 1},
    {0, 0},
    {1, 0},
    {2, 0}
};

static SavegameHelper sghra("ra",
    SA(Ra),
    SV(RaCount),
    nullptr);


void FreeRa(short nPlayer)
{
    int nRun = Ra[nPlayer].nRun;
    int nSprite = Ra[nPlayer].nSprite;

    runlist_SubRunRec(nRun);
    runlist_DoSubRunRec(sprite[nSprite].owner);
    runlist_FreeRun(sprite[nSprite].lotag - 1);

    mydeletesprite(nSprite);
}

int BuildRa(short nPlayer)
{
    short nPlayerSprite = PlayerList[nPlayer].nSprite;

    int nSprite = insertsprite(sprite[nPlayerSprite].sectnum, 203);

    sprite[nSprite].cstat = 0x8000;
    sprite[nSprite].xvel = 0;
    sprite[nSprite].yvel = 0;
    sprite[nSprite].zvel = 0;
    sprite[nSprite].extra = -1;
    sprite[nSprite].lotag = runlist_HeadRun() + 1;
    sprite[nSprite].hitag = 0;
    sprite[nSprite].owner = runlist_AddRunRec(sprite[nSprite].lotag - 1, nPlayer | 0x210000);
    sprite[nSprite].pal = 1;
    sprite[nSprite].xrepeat = 64;
    sprite[nSprite].yrepeat = 64;
    sprite[nSprite].x = sprite[nPlayerSprite].x;
    sprite[nSprite].y = sprite[nPlayerSprite].y;
    sprite[nSprite].z = sprite[nPlayerSprite].z;

//	GrabTimeSlot(3);

    Ra[nPlayer].nSprite = nSprite;

    Ra[nPlayer].nRun = runlist_AddRunRec(NewRun, nPlayer | 0x210000);
    Ra[nPlayer].nTarget = -1;
    Ra[nPlayer].nFrame  = 0;
    Ra[nPlayer].nAction = 0;
    Ra[nPlayer].field_C = 0;
    Ra[nPlayer].nPlayer = nPlayer;

    return nPlayer | 0x210000;
}

void InitRa()
{
    RaCount = 0;
    memset(Ra, 0, sizeof(RA) * kMaxPlayers);
}

void MoveRaToEnemy(short nPlayer)
{
    short nTarget = Ra[nPlayer].nTarget;
    short nSprite = Ra[nPlayer].nSprite;
    short nAction = Ra[nPlayer].nAction;

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
            if (sprite[nSprite].sectnum != sprite[nTarget].sectnum) {
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

        sprite[nSprite].cstat = 0x8000;
        nTarget = PlayerList[nPlayer].nSprite;
    }

    sprite[nSprite].x = sprite[nTarget].x;
    sprite[nSprite].y = sprite[nTarget].y;
    sprite[nSprite].z = sprite[nTarget].z - GetSpriteHeight(nTarget);

    if (sprite[nSprite].sectnum != sprite[nTarget].sectnum) {
        mychangespritesect(nSprite, sprite[nTarget].sectnum);
    }
}

void FuncRa(int a, int, int nRun)
{
    short nPlayer = RunData[nRun].nVal;
    short nCurrentWeapon = PlayerList[nPlayer].nCurrentWeapon;

    short nSeq = SeqOffsets[kSeqEyeHit] + RaSeq[Ra[nPlayer].nAction].a;
    short nSprite = Ra[nPlayer].nSprite;

    bool bVal = false;

    int nMessage = a & kMessageMask;

    switch (nMessage)
    {
        default:
        {
            Printf("unknown msg %d for Ra\n", nMessage);
            return;
        }

        case 0x30000:
        case 0xA0000:
            return;

        case 0x20000:
        {
            Ra[nPlayer].nTarget = sPlayerInput[nPlayer].nTarget;
            sprite[nSprite].picnum = seq_GetSeqPicnum2(nSeq, Ra[nPlayer].nFrame);

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
                        sprite[nSprite].cstat = 0x8000;
                    }
                    else
                    {
                        sprite[nSprite].cstat &= 0x7FFF;
                        Ra[nPlayer].nAction = 1;
                        Ra[nPlayer].nFrame  = 0;
                    }

                    return;
                }

                case 1:
                {
                    if (!Ra[nPlayer].field_C)
                    {
                        Ra[nPlayer].nAction = 3;
                        Ra[nPlayer].nFrame  = 0;
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
                        Ra[nPlayer].nFrame  = 0;
                    }
                    else
                    {
                        if (Ra[nPlayer].nFrame || Ra[nPlayer].nTarget <= -1)
                        {
                            if (!bVal) {
                                return;
                            }

                            Ra[nPlayer].nAction = 3;
                            Ra[nPlayer].nFrame  = 0;
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
                                Ra[nPlayer].nFrame  = 0;
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
                        sprite[nSprite].cstat |= 0x8000;
                        Ra[nPlayer].nAction = 0;
                        Ra[nPlayer].nFrame  = 0;
                        Ra[nPlayer].field_C = 0;
                    }

                    return;
                }

                default:
                    return;
            }
        }

        case 0x90000:
        {
            short nSprite2 = a & 0xFFFF;
            seq_PlotSequence(nSprite2, nSeq, Ra[nPlayer].nFrame, 1);
            tsprite[nSprite2].owner = -1;
            return;
        }
    }
}
END_PS_NS
