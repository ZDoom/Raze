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

#include "ra.h"
#include "runlist.h"
#include "engine.h"
#include "exhumed.h"
#include "player.h"
#include "move.h"
#include "sequence.h"
#include "input.h"
#include "gun.h"
#include "bullet.h"
#include <string.h>

/* bjd - the content of the ra.* files originally resided in gun.c I think... */

//#define kMaxRA		8

RA Ra[kMaxPlayers]; // one Ra for each player
short RaCount;

static actionSeq ActionSeq[] = {
    {2, 1}, {0, 0}, {1, 0}, {2, 0}
};

void FreeRa(short nPlayer)
{
    int nRun = Ra[nPlayer].field_4;
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

    Ra[nPlayer].field_4 = runlist_AddRunRec(NewRun, nPlayer | 0x210000);
    Ra[nPlayer].nTarget = -1;
    Ra[nPlayer].field_2 = 0;
    Ra[nPlayer].field_0 = 0;
    Ra[nPlayer].field_C = 0;
    Ra[nPlayer].field_E = nPlayer;

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
    short field_0 = Ra[nPlayer].field_0;

    if (nTarget != -1)
    {
        if (!(sprite[nTarget].cstat & 0x101) || sprite[nTarget].sectnum == kMaxSectors)
        {
            Ra[nPlayer].nTarget = -1;
            if (!field_0 || field_0 == 3) {
                return;
            }

            Ra[nPlayer].field_0 = 3;
            Ra[nPlayer].field_2 = 0;
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
        if (field_0 == 1 || field_0 == 2)
        {
            Ra[nPlayer].field_0 = 3;
            Ra[nPlayer].field_2 = 0;
            return;
        }

        if (field_0) {
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

void FuncRa(int a, int nDamage, int nRun)
{
    short nPlayer = RunData[nRun].nVal;
    short nCurrentWeapon = PlayerList[nPlayer].nCurrentWeapon;

    int var_14 = 0;

    short edx = SeqOffsets[kSeqEyeHit] + ActionSeq[Ra[nPlayer].field_0].a;
    short nSprite = Ra[nPlayer].nSprite;

    int nMessage = a & 0x7F0000;

    switch (nMessage)
    {
        default:
        {
            DebugOut("unknown msg %d for Ra\n", a & 0x7F0000);
            return;
        }

        case 0x30000:
        case 0xA0000:
            return;

        case 0x20000:
        {
            Ra[nPlayer].nTarget = sPlayerInput[nPlayer].nTarget;
            sprite[nSprite].picnum = seq_GetSeqPicnum2(edx, Ra[nPlayer].field_2);

            if (Ra[nPlayer].field_0)
            {
                seq_MoveSequence(nSprite, edx, Ra[nPlayer].field_2);

                Ra[nPlayer].field_2++;
                if (Ra[nPlayer].field_2 >= SeqSize[edx])
                {
                    Ra[nPlayer].field_2 = 0;
                    var_14 = 1;
                }
            }

            switch (Ra[nPlayer].field_0)
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
                        Ra[nPlayer].field_0 = 1;
                        Ra[nPlayer].field_2 = 0;
                    }

                    return;
                }

                case 1:
                {
                    if (!Ra[nPlayer].field_C)
                    {
                        Ra[nPlayer].field_0 = 3;
                        Ra[nPlayer].field_2 = 0;
                    }
                    else
                    {
                        if (var_14) {
                            Ra[nPlayer].field_0 = 2;
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
                        Ra[nPlayer].field_0 = 3;
                        Ra[nPlayer].field_2 = 0;
                    }
                    else
                    {
                        if (Ra[nPlayer].field_2 || Ra[nPlayer].nTarget <= -1)
                        {
                            if (!var_14) {
                                return;
                            }

                            Ra[nPlayer].field_0 = 3;
                            Ra[nPlayer].field_2 = 0;
                        }
                        else
                        {
                            if (PlayerList[nPlayer].nAmmo[kWeaponRing] > 0)
                            {
                                runlist_DamageEnemy(Ra[nPlayer].nTarget, PlayerList[Ra[nPlayer].field_E].nSprite, BulletInfo[kWeaponRing].nDamage);
                                AddAmmo(nPlayer, kWeaponRing, -WeaponInfo[kWeaponRing].d);
                                SetQuake(nSprite, 100);
                            }
                            else
                            {
                                Ra[nPlayer].field_0 = 3;
                                Ra[nPlayer].field_2 = 0;
                                SelectNewWeapon(nPlayer);
                            }
                        }
                    }

                    return;
                }

                case 3:
                {
                    if (var_14)
                    {
                        sprite[nSprite].cstat |= 0x8000;
                        Ra[nPlayer].field_0 = 0;
                        Ra[nPlayer].field_2 = 0;
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
            seq_PlotSequence(nSprite2, edx, Ra[nPlayer].field_2, 1);
            tsprite[nSprite2].owner = -1;
            return;
        }
    }
}
