//-------------------------------------------------------------------------
/*
Copyright (C) 1997, 2005 - 3D Realms Entertainment

This file is part of Shadow Warrior version 1.2

Shadow Warrior is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.

Original Source: 1997 - Frank Maddin and Jim Norwood
Prepared for public release: 03/28/2005 - Charlie Wiederhold, 3D Realms
*/
//-------------------------------------------------------------------------
#include "build.h"

#include "keys.h"
#include "names2.h"
#include "game.h"
#include "tags.h"
#include "ai.h"
#include "actor.h"
#include "track.h"
#include "sector.h"

extern uint8_t RedBookSong[40];
extern short BossSpriteNum[3];

DECISION SerpBattle[] =
{
    {670,   InitActorMoveCloser         },
    {700,   InitActorAmbientNoise         },
    {710,   InitActorRunAway            },
    {1024,  InitActorAttack             }
};

DECISION SerpOffense[] =
{
    {775,   InitActorMoveCloser         },
    {800,   InitActorAmbientNoise         },
    {1024,  InitActorAttack             }
};

DECISION SerpBroadcast[] =
{
    //{21,    InitActorAlertNoise         },
    {10,    InitActorAmbientNoise       },
    {1024,  InitActorDecide             }
};

DECISION SerpSurprised[] =
{
    {701,   InitActorMoveCloser        },
    {1024,  InitActorDecide            }
};

DECISION SerpEvasive[] =
{
    {10,   InitActorEvade  },
    {1024, NULL            }
};

DECISION SerpLostTarget[] =
{
    {900,   InitActorFindPlayer         },
    {921,   InitActorAmbientNoise       },
    {1024,  InitActorWanderAround       }
};

DECISION SerpCloseRange[] =
{
    {700,   InitActorAttack             },
    {1024,  InitActorReposition         }
};

PERSONALITY SerpPersonality =
{
    SerpBattle,
    SerpOffense,
    SerpBroadcast,
    SerpSurprised,
    SerpEvasive,
    SerpLostTarget,
    SerpCloseRange,
    SerpCloseRange
};

ATTRIBUTE SerpAttrib =
{
    {200, 220, 240, 270},                 // Speeds
    {3, 0, -2, -3},                     // Tic Adjusts
    3,                                  // MaxWeapons;
    {
        DIGI_SERPAMBIENT, DIGI_SERPALERT, DIGI_SERPSWORDATTACK,
        DIGI_SERPPAIN, DIGI_SERPSCREAM, DIGI_SERPMAGICLAUNCH,
        DIGI_SERPSUMMONHEADS,DIGI_SERPTAUNTYOU,DIGI_SERPDEATHEXPLODE,0
    }
};

ATTRIBUTE SerpPissedAttrib =
{
    {200, 220, 240, 270},                 // Speeds
    {3, 0, -2, -3},                     // Tic Adjusts
    3,                                  // MaxWeapons;
    {
        DIGI_SERPAMBIENT, DIGI_SERPALERT, DIGI_SERPSWORDATTACK,
        DIGI_SERPPAIN, DIGI_SERPSCREAM, DIGI_SERPMAGICLAUNCH,
        DIGI_SERPSUMMONHEADS,DIGI_SERPTAUNTYOU,DIGI_SERPDEATHEXPLODE,0
    }
};


//////////////////////
//
// SERP RUN
//
//////////////////////

#define SERP_RUN_RATE 24

ANIMATOR DoSerpMove,NullSerp,DoActorDebris,NullSerp;

STATE s_SerpRun[5][4] =
{
    {
        {SERP_RUN_R0 + 0, SERP_RUN_RATE, DoSerpMove, &s_SerpRun[0][1]},
        {SERP_RUN_R0 + 1, SERP_RUN_RATE, DoSerpMove, &s_SerpRun[0][2]},
        {SERP_RUN_R0 + 2, SERP_RUN_RATE, DoSerpMove, &s_SerpRun[0][3]},
        {SERP_RUN_R0 + 1, SERP_RUN_RATE, DoSerpMove, &s_SerpRun[0][0]},
    },
    {
        {SERP_RUN_R1 + 0, SERP_RUN_RATE, DoSerpMove, &s_SerpRun[1][1]},
        {SERP_RUN_R1 + 1, SERP_RUN_RATE, DoSerpMove, &s_SerpRun[1][2]},
        {SERP_RUN_R1 + 2, SERP_RUN_RATE, DoSerpMove, &s_SerpRun[1][3]},
        {SERP_RUN_R1 + 1, SERP_RUN_RATE, DoSerpMove, &s_SerpRun[1][0]},
    },
    {
        {SERP_RUN_R2 + 0, SERP_RUN_RATE, DoSerpMove, &s_SerpRun[2][1]},
        {SERP_RUN_R2 + 1, SERP_RUN_RATE, DoSerpMove, &s_SerpRun[2][2]},
        {SERP_RUN_R2 + 2, SERP_RUN_RATE, DoSerpMove, &s_SerpRun[2][3]},
        {SERP_RUN_R2 + 1, SERP_RUN_RATE, DoSerpMove, &s_SerpRun[2][0]},
    },
    {
        {SERP_RUN_R3 + 0, SERP_RUN_RATE, DoSerpMove, &s_SerpRun[3][1]},
        {SERP_RUN_R3 + 1, SERP_RUN_RATE, DoSerpMove, &s_SerpRun[3][2]},
        {SERP_RUN_R3 + 2, SERP_RUN_RATE, DoSerpMove, &s_SerpRun[3][3]},
        {SERP_RUN_R3 + 1, SERP_RUN_RATE, DoSerpMove, &s_SerpRun[3][0]},
    },
    {
        {SERP_RUN_R4 + 0, SERP_RUN_RATE, DoSerpMove, &s_SerpRun[4][1]},
        {SERP_RUN_R4 + 1, SERP_RUN_RATE, DoSerpMove, &s_SerpRun[4][2]},
        {SERP_RUN_R4 + 2, SERP_RUN_RATE, DoSerpMove, &s_SerpRun[4][3]},
        {SERP_RUN_R4 + 1, SERP_RUN_RATE, DoSerpMove, &s_SerpRun[4][0]},
    }
};


STATEp sg_SerpRun[] =
{
    &s_SerpRun[0][0],
    &s_SerpRun[1][0],
    &s_SerpRun[2][0],
    &s_SerpRun[3][0],
    &s_SerpRun[4][0]
};

//////////////////////
//
// SERP SLASH
//
//////////////////////

#define SERP_SLASH_RATE 9
ANIMATOR InitActorDecide;
ANIMATOR InitSerpSlash;

STATE s_SerpSlash[5][10] =
{
    {
        {SERP_SLASH_R0 + 2, SERP_SLASH_RATE, NullSerp, &s_SerpSlash[0][1]},
        {SERP_SLASH_R0 + 1, SERP_SLASH_RATE, NullSerp, &s_SerpSlash[0][2]},
        {SERP_SLASH_R0 + 0, SERP_SLASH_RATE*2, NullSerp, &s_SerpSlash[0][3]},
        {SERP_SLASH_R0 + 1, SERP_SLASH_RATE, NullSerp, &s_SerpSlash[0][4]},
        {SERP_SLASH_R0 + 2, SERP_SLASH_RATE, NullSerp, &s_SerpSlash[0][5]},
        {SERP_SLASH_R0 + 3, SF_QUICK_CALL, InitSerpSlash, &s_SerpSlash[0][6]},
        {SERP_SLASH_R0 + 3, SERP_SLASH_RATE, NullSerp, &s_SerpSlash[0][7]},
        {SERP_SLASH_R0 + 4, SERP_SLASH_RATE, NullSerp, &s_SerpSlash[0][8]},
        {SERP_SLASH_R0 + 4, 0|SF_QUICK_CALL, InitActorDecide, &s_SerpSlash[0][9]},
        {SERP_SLASH_R0 + 4, SERP_SLASH_RATE, DoSerpMove, &s_SerpSlash[0][9]},
    },
    {
        {SERP_SLASH_R1 + 2, SERP_SLASH_RATE, NullSerp, &s_SerpSlash[1][1]},
        {SERP_SLASH_R1 + 1, SERP_SLASH_RATE, NullSerp, &s_SerpSlash[1][2]},
        {SERP_SLASH_R1 + 0, SERP_SLASH_RATE*2, NullSerp, &s_SerpSlash[1][3]},
        {SERP_SLASH_R1 + 1, SERP_SLASH_RATE, NullSerp, &s_SerpSlash[1][4]},
        {SERP_SLASH_R1 + 2, SERP_SLASH_RATE, NullSerp, &s_SerpSlash[1][5]},
        {SERP_SLASH_R1 + 3, SF_QUICK_CALL, InitSerpSlash, &s_SerpSlash[1][6]},
        {SERP_SLASH_R1 + 3, SERP_SLASH_RATE, NullSerp, &s_SerpSlash[1][7]},
        {SERP_SLASH_R1 + 4, SERP_SLASH_RATE, NullSerp, &s_SerpSlash[1][8]},
        {SERP_SLASH_R1 + 4, 0|SF_QUICK_CALL, InitActorDecide, &s_SerpSlash[1][9]},
        {SERP_SLASH_R1 + 4, SERP_SLASH_RATE, DoSerpMove, &s_SerpSlash[1][9]},
    },
    {
        {SERP_SLASH_R2 + 2, SERP_SLASH_RATE, NullSerp, &s_SerpSlash[2][1]},
        {SERP_SLASH_R2 + 1, SERP_SLASH_RATE, NullSerp, &s_SerpSlash[2][2]},
        {SERP_SLASH_R2 + 0, SERP_SLASH_RATE*2, NullSerp, &s_SerpSlash[2][3]},
        {SERP_SLASH_R2 + 1, SERP_SLASH_RATE, NullSerp, &s_SerpSlash[2][4]},
        {SERP_SLASH_R2 + 2, SERP_SLASH_RATE, NullSerp, &s_SerpSlash[2][5]},
        {SERP_SLASH_R2 + 3, SF_QUICK_CALL, InitSerpSlash, &s_SerpSlash[2][6]},
        {SERP_SLASH_R2 + 3, SERP_SLASH_RATE, NullSerp, &s_SerpSlash[2][7]},
        {SERP_SLASH_R2 + 4, SERP_SLASH_RATE, NullSerp, &s_SerpSlash[2][8]},
        {SERP_SLASH_R2 + 4, 0|SF_QUICK_CALL, InitActorDecide, &s_SerpSlash[2][9]},
        {SERP_SLASH_R2 + 4, SERP_SLASH_RATE, DoSerpMove, &s_SerpSlash[2][9]},
    },
    {
        {SERP_SLASH_R3 + 2, SERP_SLASH_RATE, NullSerp, &s_SerpSlash[3][1]},
        {SERP_SLASH_R3 + 1, SERP_SLASH_RATE, NullSerp, &s_SerpSlash[3][2]},
        {SERP_SLASH_R3 + 0, SERP_SLASH_RATE*2, NullSerp, &s_SerpSlash[3][3]},
        {SERP_SLASH_R3 + 1, SERP_SLASH_RATE, NullSerp, &s_SerpSlash[3][4]},
        {SERP_SLASH_R3 + 2, SERP_SLASH_RATE, NullSerp, &s_SerpSlash[3][5]},
        {SERP_SLASH_R3 + 3, SF_QUICK_CALL, InitSerpSlash, &s_SerpSlash[3][6]},
        {SERP_SLASH_R3 + 3, SERP_SLASH_RATE, NullSerp, &s_SerpSlash[3][7]},
        {SERP_SLASH_R3 + 4, SERP_SLASH_RATE, NullSerp, &s_SerpSlash[3][8]},
        {SERP_SLASH_R3 + 4, 0|SF_QUICK_CALL, InitActorDecide, &s_SerpSlash[3][9]},
        {SERP_SLASH_R3 + 4, SERP_SLASH_RATE, DoSerpMove, &s_SerpSlash[3][9]},
    },
    {
        {SERP_SLASH_R4 + 2, SERP_SLASH_RATE, NullSerp, &s_SerpSlash[4][1]},
        {SERP_SLASH_R4 + 1, SERP_SLASH_RATE, NullSerp, &s_SerpSlash[4][2]},
        {SERP_SLASH_R4 + 0, SERP_SLASH_RATE*2, NullSerp, &s_SerpSlash[4][3]},
        {SERP_SLASH_R4 + 1, SERP_SLASH_RATE, NullSerp, &s_SerpSlash[4][4]},
        {SERP_SLASH_R4 + 2, SERP_SLASH_RATE, NullSerp, &s_SerpSlash[4][5]},
        {SERP_SLASH_R4 + 3, SF_QUICK_CALL, InitSerpSlash, &s_SerpSlash[4][6]},
        {SERP_SLASH_R4 + 3, SERP_SLASH_RATE, NullSerp, &s_SerpSlash[4][7]},
        {SERP_SLASH_R4 + 4, SERP_SLASH_RATE, NullSerp, &s_SerpSlash[4][8]},
        {SERP_SLASH_R4 + 4, 0|SF_QUICK_CALL, InitActorDecide, &s_SerpSlash[4][9]},
        {SERP_SLASH_R4 + 4, SERP_SLASH_RATE, DoSerpMove, &s_SerpSlash[4][9]},
    }
};


STATEp sg_SerpSlash[] =
{
    &s_SerpSlash[0][0],
    &s_SerpSlash[1][0],
    &s_SerpSlash[2][0],
    &s_SerpSlash[3][0],
    &s_SerpSlash[4][0]
};


//////////////////////
//
// SERP SKULL SPELL
//
//////////////////////

#define SERP_SKULL_SPELL_RATE 18
ANIMATOR InitSerpSkullSpell;
ANIMATOR InitSerpRing;

STATE s_SerpSkullSpell[5][8] =
{
    {
        {SERP_SPELL_R0 + 2, SERP_SKULL_SPELL_RATE*2, NullSerp, &s_SerpSkullSpell[0][1]},
        {SERP_SPELL_R0 + 1, SERP_SKULL_SPELL_RATE, NullSerp, &s_SerpSkullSpell[0][2]},
        {SERP_SPELL_R0 + 0, SERP_SKULL_SPELL_RATE*2, NullSerp, &s_SerpSkullSpell[0][3]},
        {SERP_SPELL_R0 + 0, SF_QUICK_CALL, InitSerpRing, &s_SerpSkullSpell[0][4]},
        {SERP_SPELL_R0 + 0, SERP_SKULL_SPELL_RATE, NullSerp, &s_SerpSkullSpell[0][5]},
        {SERP_SPELL_R0 + 1, SERP_SKULL_SPELL_RATE, NullSerp, &s_SerpSkullSpell[0][6]},
        {SERP_SPELL_R0 + 1, SF_QUICK_CALL,   InitActorDecide, &s_SerpSkullSpell[0][7]},
        {SERP_SPELL_R0 + 1, SERP_SKULL_SPELL_RATE, DoSerpMove, &s_SerpSkullSpell[0][7]},
    },
    {
        {SERP_SPELL_R1 + 2, SERP_SKULL_SPELL_RATE*2, NullSerp, &s_SerpSkullSpell[1][1]},
        {SERP_SPELL_R1 + 1, SERP_SKULL_SPELL_RATE, NullSerp, &s_SerpSkullSpell[1][2]},
        {SERP_SPELL_R1 + 0, SERP_SKULL_SPELL_RATE*2, NullSerp, &s_SerpSkullSpell[1][3]},
        {SERP_SPELL_R1 + 0, SF_QUICK_CALL, InitSerpRing, &s_SerpSkullSpell[1][4]},
        {SERP_SPELL_R1 + 0, SERP_SKULL_SPELL_RATE, NullSerp, &s_SerpSkullSpell[1][5]},
        {SERP_SPELL_R1 + 1, SERP_SKULL_SPELL_RATE, NullSerp, &s_SerpSkullSpell[1][6]},
        {SERP_SPELL_R1 + 1, SF_QUICK_CALL, InitActorDecide, &s_SerpSkullSpell[1][7]},
        {SERP_SPELL_R1 + 1, SERP_SKULL_SPELL_RATE, DoSerpMove, &s_SerpSkullSpell[1][7]},
    },
    {
        {SERP_SPELL_R2 + 2, SERP_SKULL_SPELL_RATE*2, NullSerp, &s_SerpSkullSpell[2][1]},
        {SERP_SPELL_R2 + 1, SERP_SKULL_SPELL_RATE, NullSerp, &s_SerpSkullSpell[2][2]},
        {SERP_SPELL_R2 + 0, SERP_SKULL_SPELL_RATE*2, NullSerp, &s_SerpSkullSpell[2][3]},
        {SERP_SPELL_R2 + 0, SF_QUICK_CALL, InitSerpRing, &s_SerpSkullSpell[2][4]},
        {SERP_SPELL_R2 + 0, SERP_SKULL_SPELL_RATE, NullSerp, &s_SerpSkullSpell[2][5]},
        {SERP_SPELL_R2 + 1, SERP_SKULL_SPELL_RATE, NullSerp, &s_SerpSkullSpell[2][6]},
        {SERP_SPELL_R2 + 1, SF_QUICK_CALL, InitActorDecide, &s_SerpSkullSpell[2][7]},
        {SERP_SPELL_R2 + 1, SERP_SKULL_SPELL_RATE, DoSerpMove, &s_SerpSkullSpell[2][7]},
    },
    {
        {SERP_SPELL_R3 + 2, SERP_SKULL_SPELL_RATE*2, NullSerp, &s_SerpSkullSpell[3][1]},
        {SERP_SPELL_R3 + 1, SERP_SKULL_SPELL_RATE, NullSerp, &s_SerpSkullSpell[3][2]},
        {SERP_SPELL_R3 + 0, SERP_SKULL_SPELL_RATE*2, NullSerp, &s_SerpSkullSpell[3][3]},
        {SERP_SPELL_R3 + 0, SF_QUICK_CALL, InitSerpRing, &s_SerpSkullSpell[3][4]},
        {SERP_SPELL_R3 + 0, SERP_SKULL_SPELL_RATE, NullSerp, &s_SerpSkullSpell[3][5]},
        {SERP_SPELL_R3 + 1, SERP_SKULL_SPELL_RATE, NullSerp, &s_SerpSkullSpell[3][6]},
        {SERP_SPELL_R3 + 1, SF_QUICK_CALL, InitActorDecide, &s_SerpSkullSpell[3][7]},
        {SERP_SPELL_R3 + 1, SERP_SKULL_SPELL_RATE, DoSerpMove, &s_SerpSkullSpell[3][7]},
    },
    {
        {SERP_SPELL_R4 + 2, SERP_SKULL_SPELL_RATE*2, NullSerp, &s_SerpSkullSpell[4][1]},
        {SERP_SPELL_R4 + 1, SERP_SKULL_SPELL_RATE, NullSerp, &s_SerpSkullSpell[4][2]},
        {SERP_SPELL_R4 + 0, SERP_SKULL_SPELL_RATE*2, NullSerp, &s_SerpSkullSpell[4][3]},
        {SERP_SPELL_R4 + 0, SF_QUICK_CALL, InitSerpRing, &s_SerpSkullSpell[4][4]},
        {SERP_SPELL_R4 + 0, SERP_SKULL_SPELL_RATE, NullSerp, &s_SerpSkullSpell[4][5]},
        {SERP_SPELL_R4 + 1, SERP_SKULL_SPELL_RATE, NullSerp, &s_SerpSkullSpell[4][6]},
        {SERP_SPELL_R4 + 1, SF_QUICK_CALL, InitActorDecide, &s_SerpSkullSpell[4][7]},
        {SERP_SPELL_R4 + 1, SERP_SKULL_SPELL_RATE, DoSerpMove, &s_SerpSkullSpell[4][7]},
    }
};


STATEp sg_SerpSkullSpell[] =
{
    &s_SerpSkullSpell[0][0],
    &s_SerpSkullSpell[1][0],
    &s_SerpSkullSpell[2][0],
    &s_SerpSkullSpell[3][0],
    &s_SerpSkullSpell[4][0]
};



//////////////////////
//
// SERP SPELL
//
//////////////////////

#define SERP_SPELL_RATE 18
ANIMATOR InitSerpSpell;

STATE s_SerpSpell[5][8] =
{
    {
        {SERP_SPELL_R0 + 2, SERP_SPELL_RATE*2, NullSerp, &s_SerpSpell[0][1]},
        {SERP_SPELL_R0 + 1, SERP_SPELL_RATE, NullSerp, &s_SerpSpell[0][2]},
        {SERP_SPELL_R0 + 0, SERP_SPELL_RATE*2, NullSerp, &s_SerpSpell[0][3]},
        {SERP_SPELL_R0 + 0, SF_QUICK_CALL, InitSerpSpell, &s_SerpSpell[0][4]},
        {SERP_SPELL_R0 + 0, SERP_SPELL_RATE, NullSerp, &s_SerpSpell[0][5]},
        {SERP_SPELL_R0 + 1, SERP_SPELL_RATE, NullSerp, &s_SerpSpell[0][6]},
        {SERP_SPELL_R0 + 1, SF_QUICK_CALL,   InitActorDecide, &s_SerpSpell[0][7]},
        {SERP_SPELL_R0 + 1, SERP_SPELL_RATE, DoSerpMove, &s_SerpSpell[0][7]},
    },
    {
        {SERP_SPELL_R1 + 2, SERP_SPELL_RATE*2, NullSerp, &s_SerpSpell[1][1]},
        {SERP_SPELL_R1 + 1, SERP_SPELL_RATE, NullSerp, &s_SerpSpell[1][2]},
        {SERP_SPELL_R1 + 0, SERP_SPELL_RATE*2, NullSerp, &s_SerpSpell[1][3]},
        {SERP_SPELL_R1 + 0, SF_QUICK_CALL, InitSerpSpell, &s_SerpSpell[1][4]},
        {SERP_SPELL_R1 + 0, SERP_SPELL_RATE, NullSerp, &s_SerpSpell[1][5]},
        {SERP_SPELL_R1 + 1, SERP_SPELL_RATE, NullSerp, &s_SerpSpell[1][6]},
        {SERP_SPELL_R1 + 1, SF_QUICK_CALL, InitActorDecide, &s_SerpSpell[1][7]},
        {SERP_SPELL_R1 + 1, SERP_SPELL_RATE, DoSerpMove, &s_SerpSpell[1][7]},
    },
    {
        {SERP_SPELL_R2 + 2, SERP_SPELL_RATE*2, NullSerp, &s_SerpSpell[2][1]},
        {SERP_SPELL_R2 + 1, SERP_SPELL_RATE, NullSerp, &s_SerpSpell[2][2]},
        {SERP_SPELL_R2 + 0, SERP_SPELL_RATE*2, NullSerp, &s_SerpSpell[2][3]},
        {SERP_SPELL_R2 + 0, SF_QUICK_CALL, InitSerpSpell, &s_SerpSpell[2][4]},
        {SERP_SPELL_R2 + 0, SERP_SPELL_RATE, NullSerp, &s_SerpSpell[2][5]},
        {SERP_SPELL_R2 + 1, SERP_SPELL_RATE, NullSerp, &s_SerpSpell[2][6]},
        {SERP_SPELL_R2 + 1, SF_QUICK_CALL, InitActorDecide, &s_SerpSpell[2][7]},
        {SERP_SPELL_R2 + 1, SERP_SPELL_RATE, DoSerpMove, &s_SerpSpell[2][7]},
    },
    {
        {SERP_SPELL_R3 + 2, SERP_SPELL_RATE*2, NullSerp, &s_SerpSpell[3][1]},
        {SERP_SPELL_R3 + 1, SERP_SPELL_RATE, NullSerp, &s_SerpSpell[3][2]},
        {SERP_SPELL_R3 + 0, SERP_SPELL_RATE*2, NullSerp, &s_SerpSpell[3][3]},
        {SERP_SPELL_R3 + 0, SF_QUICK_CALL, InitSerpSpell, &s_SerpSpell[3][4]},
        {SERP_SPELL_R3 + 0, SERP_SPELL_RATE, NullSerp, &s_SerpSpell[3][5]},
        {SERP_SPELL_R3 + 1, SERP_SPELL_RATE, NullSerp, &s_SerpSpell[3][6]},
        {SERP_SPELL_R3 + 1, SF_QUICK_CALL, InitActorDecide, &s_SerpSpell[3][7]},
        {SERP_SPELL_R3 + 1, SERP_SPELL_RATE, DoSerpMove, &s_SerpSpell[3][7]},
    },
    {
        {SERP_SPELL_R4 + 2, SERP_SPELL_RATE*2, NullSerp, &s_SerpSpell[4][1]},
        {SERP_SPELL_R4 + 1, SERP_SPELL_RATE, NullSerp, &s_SerpSpell[4][2]},
        {SERP_SPELL_R4 + 0, SERP_SPELL_RATE*2, NullSerp, &s_SerpSpell[4][3]},
        {SERP_SPELL_R4 + 0, SF_QUICK_CALL, InitSerpSpell, &s_SerpSpell[4][4]},
        {SERP_SPELL_R4 + 0, SERP_SPELL_RATE, NullSerp, &s_SerpSpell[4][5]},
        {SERP_SPELL_R4 + 1, SERP_SPELL_RATE, NullSerp, &s_SerpSpell[4][6]},
        {SERP_SPELL_R4 + 1, SF_QUICK_CALL, InitActorDecide, &s_SerpSpell[4][7]},
        {SERP_SPELL_R4 + 1, SERP_SPELL_RATE, DoSerpMove, &s_SerpSpell[4][7]},
    }
};


STATEp sg_SerpSpell[] =
{
    &s_SerpSpell[0][0],
    &s_SerpSpell[1][0],
    &s_SerpSpell[2][0],
    &s_SerpSpell[3][0],
    &s_SerpSpell[4][0]
};

//////////////////////
//
// SERP SPELL MONSTER
//
//////////////////////

ANIMATOR InitSerpMonstSpell;

STATE s_SerpMonstSpell[5][8] =
{
    {
        {SERP_SPELL_R0 + 2, SERP_SPELL_RATE*2, NullSerp, &s_SerpMonstSpell[0][1]},
        {SERP_SPELL_R0 + 1, SERP_SPELL_RATE, NullSerp, &s_SerpMonstSpell[0][2]},
        {SERP_SPELL_R0 + 0, SERP_SPELL_RATE*2, NullSerp, &s_SerpMonstSpell[0][3]},
        {SERP_SPELL_R0 + 0, SF_QUICK_CALL, InitSerpMonstSpell, &s_SerpMonstSpell[0][4]},
        {SERP_SPELL_R0 + 0, SERP_SPELL_RATE, NullSerp, &s_SerpMonstSpell[0][5]},
        {SERP_SPELL_R0 + 1, SERP_SPELL_RATE, NullSerp, &s_SerpMonstSpell[0][6]},
        {SERP_SPELL_R0 + 1, SF_QUICK_CALL,   InitActorDecide, &s_SerpMonstSpell[0][7]},
        {SERP_SPELL_R0 + 1, SERP_SPELL_RATE, DoSerpMove, &s_SerpMonstSpell[0][7]},
    },
    {
        {SERP_SPELL_R1 + 2, SERP_SPELL_RATE*2, NullSerp, &s_SerpMonstSpell[1][1]},
        {SERP_SPELL_R1 + 1, SERP_SPELL_RATE, NullSerp, &s_SerpMonstSpell[1][2]},
        {SERP_SPELL_R1 + 0, SERP_SPELL_RATE*2, NullSerp, &s_SerpMonstSpell[1][3]},
        {SERP_SPELL_R1 + 0, SF_QUICK_CALL, InitSerpMonstSpell, &s_SerpMonstSpell[1][4]},
        {SERP_SPELL_R1 + 0, SERP_SPELL_RATE, NullSerp, &s_SerpMonstSpell[1][5]},
        {SERP_SPELL_R1 + 1, SERP_SPELL_RATE, NullSerp, &s_SerpMonstSpell[1][6]},
        {SERP_SPELL_R1 + 1, SF_QUICK_CALL, InitActorDecide, &s_SerpMonstSpell[1][7]},
        {SERP_SPELL_R1 + 1, SERP_SPELL_RATE, DoSerpMove, &s_SerpMonstSpell[1][7]},
    },
    {
        {SERP_SPELL_R2 + 2, SERP_SPELL_RATE*2, NullSerp, &s_SerpMonstSpell[2][1]},
        {SERP_SPELL_R2 + 1, SERP_SPELL_RATE, NullSerp, &s_SerpMonstSpell[2][2]},
        {SERP_SPELL_R2 + 0, SERP_SPELL_RATE*2, NullSerp, &s_SerpMonstSpell[2][3]},
        {SERP_SPELL_R2 + 0, SF_QUICK_CALL, InitSerpMonstSpell, &s_SerpMonstSpell[2][4]},
        {SERP_SPELL_R2 + 0, SERP_SPELL_RATE, NullSerp, &s_SerpMonstSpell[2][5]},
        {SERP_SPELL_R2 + 1, SERP_SPELL_RATE, NullSerp, &s_SerpMonstSpell[2][6]},
        {SERP_SPELL_R2 + 1, SF_QUICK_CALL, InitActorDecide, &s_SerpMonstSpell[2][7]},
        {SERP_SPELL_R2 + 1, SERP_SPELL_RATE, DoSerpMove, &s_SerpMonstSpell[2][7]},
    },
    {
        {SERP_SPELL_R3 + 2, SERP_SPELL_RATE*2, NullSerp, &s_SerpMonstSpell[3][1]},
        {SERP_SPELL_R3 + 1, SERP_SPELL_RATE, NullSerp, &s_SerpMonstSpell[3][2]},
        {SERP_SPELL_R3 + 0, SERP_SPELL_RATE*2, NullSerp, &s_SerpMonstSpell[3][3]},
        {SERP_SPELL_R3 + 0, SF_QUICK_CALL, InitSerpMonstSpell, &s_SerpMonstSpell[3][4]},
        {SERP_SPELL_R3 + 0, SERP_SPELL_RATE, NullSerp, &s_SerpMonstSpell[3][5]},
        {SERP_SPELL_R3 + 1, SERP_SPELL_RATE, NullSerp, &s_SerpMonstSpell[3][6]},
        {SERP_SPELL_R3 + 1, SF_QUICK_CALL, InitActorDecide, &s_SerpMonstSpell[3][7]},
        {SERP_SPELL_R3 + 1, SERP_SPELL_RATE, DoSerpMove, &s_SerpMonstSpell[3][7]},
    },
    {
        {SERP_SPELL_R4 + 2, SERP_SPELL_RATE*2, NullSerp, &s_SerpMonstSpell[4][1]},
        {SERP_SPELL_R4 + 1, SERP_SPELL_RATE, NullSerp, &s_SerpMonstSpell[4][2]},
        {SERP_SPELL_R4 + 0, SERP_SPELL_RATE*2, NullSerp, &s_SerpMonstSpell[4][3]},
        {SERP_SPELL_R4 + 0, SF_QUICK_CALL, InitSerpMonstSpell, &s_SerpMonstSpell[4][4]},
        {SERP_SPELL_R4 + 0, SERP_SPELL_RATE, NullSerp, &s_SerpMonstSpell[4][5]},
        {SERP_SPELL_R4 + 1, SERP_SPELL_RATE, NullSerp, &s_SerpMonstSpell[4][6]},
        {SERP_SPELL_R4 + 1, SF_QUICK_CALL, InitActorDecide, &s_SerpMonstSpell[4][7]},
        {SERP_SPELL_R4 + 1, SERP_SPELL_RATE, DoSerpMove, &s_SerpMonstSpell[4][7]},
    }
};


STATEp sg_SerpMonstSpell[] =
{
    &s_SerpMonstSpell[0][0],
    &s_SerpMonstSpell[1][0],
    &s_SerpMonstSpell[2][0],
    &s_SerpMonstSpell[3][0],
    &s_SerpMonstSpell[4][0]
};


//////////////////////
//
// SERP RAPID SPELL
//
//////////////////////

#define SERP_SPELL_RATE 18
ANIMATOR InitSerpRapidSpell;

STATE s_SerpRapidSpell[5][10] =
{
    {
        {SERP_SPELL_R0 + 2, SERP_SPELL_RATE*2, NullSerp, &s_SerpRapidSpell[0][1]},
        {SERP_SPELL_R0 + 1, SERP_SPELL_RATE, NullSerp, &s_SerpRapidSpell[0][2]},
        {SERP_SPELL_R0 + 0, SERP_SPELL_RATE*2, NullSerp, &s_SerpRapidSpell[0][3]},
        {SERP_SPELL_R0 + 0, SF_QUICK_CALL, InitSerpSpell, &s_SerpRapidSpell[0][4]},
        {SERP_SPELL_R0 + 0, SERP_SPELL_RATE*2, NullSerp, &s_SerpRapidSpell[0][5]},
        {SERP_SPELL_R0 + 0, SF_QUICK_CALL, InitSerpSpell, &s_SerpRapidSpell[0][6]},
        {SERP_SPELL_R0 + 0, SERP_SPELL_RATE, NullSerp, &s_SerpRapidSpell[0][7]},
        {SERP_SPELL_R0 + 1, SERP_SPELL_RATE, NullSerp, &s_SerpRapidSpell[0][8]},
        {SERP_SPELL_R0 + 1, SF_QUICK_CALL,   InitActorDecide, &s_SerpRapidSpell[0][9]},
        {SERP_SPELL_R0 + 1, SERP_SPELL_RATE, DoSerpMove, &s_SerpRapidSpell[0][9]},
    },
    {
        {SERP_SPELL_R1 + 2, SERP_SPELL_RATE*2, NullSerp, &s_SerpRapidSpell[1][1]},
        {SERP_SPELL_R1 + 1, SERP_SPELL_RATE, NullSerp, &s_SerpRapidSpell[1][2]},
        {SERP_SPELL_R1 + 0, SERP_SPELL_RATE*2, NullSerp, &s_SerpRapidSpell[1][3]},
        {SERP_SPELL_R1 + 0, SF_QUICK_CALL, InitSerpSpell, &s_SerpRapidSpell[1][4]},
        {SERP_SPELL_R1 + 0, SERP_SPELL_RATE*2, NullSerp, &s_SerpRapidSpell[1][5]},
        {SERP_SPELL_R1 + 0, SF_QUICK_CALL, InitSerpSpell, &s_SerpRapidSpell[1][6]},
        {SERP_SPELL_R1 + 0, SERP_SPELL_RATE, NullSerp, &s_SerpRapidSpell[1][7]},
        {SERP_SPELL_R1 + 1, SERP_SPELL_RATE, NullSerp, &s_SerpRapidSpell[1][8]},
        {SERP_SPELL_R1 + 1, SF_QUICK_CALL, InitActorDecide, &s_SerpRapidSpell[1][9]},
        {SERP_SPELL_R1 + 1, SERP_SPELL_RATE, DoSerpMove, &s_SerpRapidSpell[1][9]},
    },
    {
        {SERP_SPELL_R2 + 2, SERP_SPELL_RATE*2, NullSerp, &s_SerpRapidSpell[2][1]},
        {SERP_SPELL_R2 + 1, SERP_SPELL_RATE, NullSerp, &s_SerpRapidSpell[2][2]},
        {SERP_SPELL_R2 + 0, SERP_SPELL_RATE*2, NullSerp, &s_SerpRapidSpell[2][3]},
        {SERP_SPELL_R2 + 0, SF_QUICK_CALL, InitSerpSpell, &s_SerpRapidSpell[2][4]},
        {SERP_SPELL_R2 + 0, SERP_SPELL_RATE*2, NullSerp, &s_SerpRapidSpell[2][5]},
        {SERP_SPELL_R2 + 0, SF_QUICK_CALL, InitSerpSpell, &s_SerpRapidSpell[2][6]},
        {SERP_SPELL_R2 + 0, SERP_SPELL_RATE, NullSerp, &s_SerpRapidSpell[2][7]},
        {SERP_SPELL_R2 + 1, SERP_SPELL_RATE, NullSerp, &s_SerpRapidSpell[2][8]},
        {SERP_SPELL_R2 + 1, SF_QUICK_CALL, InitActorDecide, &s_SerpRapidSpell[2][9]},
        {SERP_SPELL_R2 + 1, SERP_SPELL_RATE, DoSerpMove, &s_SerpRapidSpell[2][9]},
    },
    {
        {SERP_SPELL_R3 + 2, SERP_SPELL_RATE*2, NullSerp, &s_SerpRapidSpell[3][1]},
        {SERP_SPELL_R3 + 1, SERP_SPELL_RATE, NullSerp, &s_SerpRapidSpell[3][2]},
        {SERP_SPELL_R3 + 0, SERP_SPELL_RATE*2, NullSerp, &s_SerpRapidSpell[3][3]},
        {SERP_SPELL_R3 + 0, SF_QUICK_CALL, InitSerpSpell, &s_SerpRapidSpell[3][4]},
        {SERP_SPELL_R3 + 0, SERP_SPELL_RATE*2, NullSerp, &s_SerpRapidSpell[3][5]},
        {SERP_SPELL_R3 + 0, SF_QUICK_CALL, InitSerpSpell, &s_SerpRapidSpell[3][6]},
        {SERP_SPELL_R3 + 0, SERP_SPELL_RATE, NullSerp, &s_SerpRapidSpell[3][7]},
        {SERP_SPELL_R3 + 1, SERP_SPELL_RATE, NullSerp, &s_SerpRapidSpell[3][8]},
        {SERP_SPELL_R3 + 1, SF_QUICK_CALL, InitActorDecide, &s_SerpRapidSpell[3][9]},
        {SERP_SPELL_R3 + 1, SERP_SPELL_RATE, DoSerpMove, &s_SerpRapidSpell[3][9]},
    },
    {
        {SERP_SPELL_R4 + 2, SERP_SPELL_RATE*2, NullSerp, &s_SerpRapidSpell[4][1]},
        {SERP_SPELL_R4 + 1, SERP_SPELL_RATE, NullSerp, &s_SerpRapidSpell[4][2]},
        {SERP_SPELL_R4 + 0, SERP_SPELL_RATE*2, NullSerp, &s_SerpRapidSpell[4][3]},
        {SERP_SPELL_R4 + 0, SF_QUICK_CALL, InitSerpSpell, &s_SerpRapidSpell[4][4]},
        {SERP_SPELL_R4 + 0, SERP_SPELL_RATE*2, NullSerp, &s_SerpRapidSpell[4][5]},
        {SERP_SPELL_R4 + 0, SF_QUICK_CALL, InitSerpSpell, &s_SerpRapidSpell[4][6]},
        {SERP_SPELL_R4 + 0, SERP_SPELL_RATE, NullSerp, &s_SerpRapidSpell[4][7]},
        {SERP_SPELL_R4 + 1, SERP_SPELL_RATE, NullSerp, &s_SerpRapidSpell[4][8]},
        {SERP_SPELL_R4 + 1, SF_QUICK_CALL, InitActorDecide, &s_SerpRapidSpell[4][9]},
        {SERP_SPELL_R4 + 1, SERP_SPELL_RATE, DoSerpMove, &s_SerpRapidSpell[4][9]},
    }
};


STATEp sg_SerpRapidSpell[] =
{
    &s_SerpRapidSpell[0][0],
    &s_SerpRapidSpell[1][0],
    &s_SerpRapidSpell[2][0],
    &s_SerpRapidSpell[3][0],
    &s_SerpRapidSpell[4][0]
};

//////////////////////
//
// SERP STAND
//
//////////////////////

#define SERP_STAND_RATE 12

STATE s_SerpStand[5][1] =
{
    {
        {SERP_RUN_R0 + 0, SERP_STAND_RATE, DoSerpMove, &s_SerpStand[0][0]},
    },
    {
        {SERP_RUN_R1 + 0, SERP_STAND_RATE, DoSerpMove, &s_SerpStand[1][0]},
    },
    {
        {SERP_RUN_R2 + 0, SERP_STAND_RATE, DoSerpMove, &s_SerpStand[2][0]},
    },
    {
        {SERP_RUN_R3 + 0, SERP_STAND_RATE, DoSerpMove, &s_SerpStand[3][0]},
    },
    {
        {SERP_RUN_R4 + 0, SERP_STAND_RATE, DoSerpMove, &s_SerpStand[4][0]},
    },
};


STATEp sg_SerpStand[] =
{
    s_SerpStand[0],
    s_SerpStand[1],
    s_SerpStand[2],
    s_SerpStand[3],
    s_SerpStand[4]
};

//////////////////////
//
// SERP DIE
//
//////////////////////

#define SERP_DIE_RATE 20
ANIMATOR DoDeathSpecial;

STATE s_SerpDie[] =
{
    {SERP_DIE + 0, SERP_DIE_RATE, NullSerp, &s_SerpDie[1]},
    {SERP_DIE + 1, SERP_DIE_RATE, NullSerp, &s_SerpDie[2]},
    {SERP_DIE + 2, SERP_DIE_RATE, NullSerp, &s_SerpDie[3]},
    {SERP_DIE + 3, SERP_DIE_RATE, NullSerp, &s_SerpDie[4]},
    {SERP_DIE + 4, SERP_DIE_RATE, NullSerp, &s_SerpDie[5]},
    {SERP_DIE + 5, SERP_DIE_RATE, NullSerp, &s_SerpDie[6]},
    {SERP_DIE + 6, SERP_DIE_RATE, NullSerp, &s_SerpDie[7]},
    {SERP_DIE + 7, SERP_DIE_RATE, NullSerp, &s_SerpDie[8]},
    {SERP_DIE + 8, SERP_DIE_RATE, NullSerp, &s_SerpDie[9]},
    {SERP_DIE + 8, SF_QUICK_CALL, DoDeathSpecial, &s_SerpDie[10]},
    {SERP_DEAD,    SERP_DIE_RATE, DoActorDebris, &s_SerpDie[10]}
};

STATE s_SerpDead[] =
{
    {SERP_DEAD, SERP_DIE_RATE, DoActorDebris, &s_SerpDead[0]},
};

STATEp sg_SerpDie[] =
{
    s_SerpDie
};

STATEp sg_SerpDead[] =
{
    s_SerpDead
};

/*
STATEp *Stand[MAX_WEAPONS];
STATEp *Run;
STATEp *Jump;
STATEp *Fall;
STATEp *Crawl;
STATEp *Swim;
STATEp *Fly;
STATEp *Rise;
STATEp *Sit;
STATEp *Look;
STATEp *Climb;
STATEp *Pain;
STATEp *Death1;
STATEp *Death2;
STATEp *Dead;
STATEp *DeathJump;
STATEp *DeathFall;
STATEp *CloseAttack[2];
STATEp *Attack[6];
STATEp *Special[2];
*/


ACTOR_ACTION_SET SerpActionSet =
{
    sg_SerpStand,
    sg_SerpRun,
    NULL, //sg_SerpJump,
    NULL, //sg_SerpFall,
    NULL, //sg_SerpCrawl,
    NULL, //sg_SerpSwim,
    NULL, //sg_SerpFly,
    NULL, //sg_SerpRise,
    NULL, //sg_SerpSit,
    NULL, //sg_SerpLook,
    NULL, //climb
    NULL, //pain
    sg_SerpDie,
    NULL, //sg_SerpHariKari,
    sg_SerpDead,
    NULL, //sg_SerpDeathJump,
    NULL, //sg_SerpDeathFall,
    {sg_SerpSlash},
    {1024},
    {sg_SerpSlash, sg_SerpSpell, sg_SerpRapidSpell, sg_SerpRapidSpell},
    {256, 724, 900, 1024},
    {NULL},
    NULL,
    NULL
};

int
SetupSerp(short SpriteNum)
{
    SPRITEp sp = &sprite[SpriteNum];
    USERp u;
    ANIMATOR DoActorDecide;

    if (TEST(sp->cstat, CSTAT_SPRITE_RESTORE))
    {
        u = User[SpriteNum];
        ASSERT(u);
    }
    else
    {
        User[SpriteNum] = u = SpawnUser(SpriteNum,SERP_RUN_R0,s_SerpRun[0]);
        u->Health = HEALTH_SERP_GOD;
    }

    if (Skill == 0) u->Health = 1100;
    if (Skill == 1) u->Health = 2200;

    ChangeState(SpriteNum, s_SerpRun[0]);
    u->Attrib = &SerpAttrib;
    DoActorSetSpeed(SpriteNum, NORM_SPEED);
    u->StateEnd = s_SerpDie;
    u->Rot = sg_SerpRun;

    EnemyDefaults(SpriteNum, &SerpActionSet, &SerpPersonality);

    // Mini-Boss Serp
    if (sp->pal == 16)
    {
        u->Health = 1000;
        sp->yrepeat = 74;
        sp->xrepeat = 74;
    }
    else
    {
        sp->yrepeat = 100;
        sp->xrepeat = 128;
    }

    sp->clipdist = (512) >> 2;
    SET(u->Flags, SPR_XFLIP_TOGGLE|SPR_ELECTRO_TOLERANT);

    u->loz = sp->z;

    // amount to move up for clipmove
    u->zclip = Z(80);
    // size of step can walk off of
    u->lo_step = Z(40);

    u->floor_dist = u->zclip - u->lo_step;
    u->ceiling_dist = SPRITEp_SIZE_Z(sp) - u->zclip;

    return 0;
}

int NullSerp(short SpriteNum)
{
    SPRITEp sp = &sprite[SpriteNum];
    USERp u = User[SpriteNum];

    if (TEST(u->Flags,SPR_SLIDING))
        DoActorSlide(SpriteNum);

    KeepActorOnFloor(SpriteNum);

    //DoActorSectorDamage(SpriteNum);
    return 0;
}

int DoSerpMove(short SpriteNum)
{
    SPRITEp sp = &sprite[SpriteNum];
    USERp u = User[SpriteNum];

    if (TEST(u->Flags,SPR_SLIDING))
        DoActorSlide(SpriteNum);

    if (u->track >= 0)
        ActorFollowTrack(SpriteNum, ACTORMOVETICS);
    else
        (*u->ActorActionFunc)(SpriteNum);

    // serp ring
    if (sp->pal != 16)
    {
        switch (u->Counter2)
        {
        case 0:
            if (u->Health != u->MaxHealth)
            {
                NewStateGroup(SpriteNum, sg_SerpSkullSpell);
                u->Counter2++;
            }
            break;

        case 1:
            //if (u->Health <= DIV2(u->MaxHealth))
        {
            if (u->Counter <= 0)
                NewStateGroup(SpriteNum, sg_SerpSkullSpell);
        }
        break;
        }
    }

    KeepActorOnFloor(SpriteNum);

    //DoActorSectorDamage(SpriteNum);
    return 0;
}

int DoDeathSpecial(short SpriteNum)
{
    SPRITEp sp = &sprite[SpriteNum];
    USERp u = User[SpriteNum];
    static SWBOOL alreadydid = FALSE;

    DoMatchEverything(NULL, sp->lotag, ON);

    if (!SW_SHAREWARE && gs.MusicOn && !alreadydid)
    {
        PlaySong(0, RedBookSong[Level], TRUE, TRUE);
        alreadydid = TRUE;
    }

    BossSpriteNum[0] = -2;
    return 0;
}


#include "saveable.h"

static saveable_code saveable_serp_code[] =
{
    SAVE_CODE(SetupSerp),
    SAVE_CODE(NullSerp),
    SAVE_CODE(DoSerpMove),
    SAVE_CODE(DoDeathSpecial),
};

static saveable_data saveable_serp_data[] =
{
    SAVE_DATA(SerpBattle),
    SAVE_DATA(SerpOffense),
    SAVE_DATA(SerpBroadcast),
    SAVE_DATA(SerpSurprised),
    SAVE_DATA(SerpEvasive),
    SAVE_DATA(SerpLostTarget),
    SAVE_DATA(SerpCloseRange),

    SAVE_DATA(SerpPersonality),

    SAVE_DATA(SerpAttrib),
    SAVE_DATA(SerpPissedAttrib),

    SAVE_DATA(s_SerpRun),
    SAVE_DATA(sg_SerpRun),
    SAVE_DATA(s_SerpSlash),
    SAVE_DATA(sg_SerpSlash),
    SAVE_DATA(s_SerpSkullSpell),
    SAVE_DATA(sg_SerpSkullSpell),
    SAVE_DATA(s_SerpSpell),
    SAVE_DATA(sg_SerpSpell),
    SAVE_DATA(s_SerpMonstSpell),
    SAVE_DATA(sg_SerpMonstSpell),
    SAVE_DATA(s_SerpRapidSpell),
    SAVE_DATA(sg_SerpRapidSpell),
    SAVE_DATA(s_SerpStand),
    SAVE_DATA(sg_SerpStand),
    SAVE_DATA(s_SerpDie),
    SAVE_DATA(s_SerpDead),
    SAVE_DATA(sg_SerpDie),
    SAVE_DATA(sg_SerpDead),

    SAVE_DATA(SerpActionSet),
};

saveable_module saveable_serp =
{
    // code
    saveable_serp_code,
    SIZ(saveable_serp_code),

    // data
    saveable_serp_data,
    SIZ(saveable_serp_data)
};

