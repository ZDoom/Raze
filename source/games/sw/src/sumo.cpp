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
#include "ns.h"
#include "build.h"

#include "names2.h"
#include "panel.h"
#include "game.h"
#include "tags.h"
#include "ai.h"
#include "misc.h"
#include "weapon.h"
#include "sector.h"
#include "gamecontrol.h"
#include "mapinfo.h"
#include "v_draw.h"

BEGIN_SW_NS

extern int InitSumoNapalm(DSWActor*);
extern int InitSumoStompAttack(DSWActor*);
extern int InitMiniSumoClap(DSWActor*);
extern int InitSumoSkull(DSWActor*);

extern uint8_t playTrack;
bool bosswasseen[3];

DSWActor* BossSpriteNum[3];


DECISION SumoBattle[] =
{
    {690,   &AF(InitActorMoveCloser)   },
    {692,   &AF(InitActorSetDecide)   },
    {1024,  &AF(InitActorAttack    )   }
};

DECISION SumoOffense[] =
{
    {690,   &AF(InitActorMoveCloser)   },
    {692,   &AF(InitActorSetDecide)   },
    {1024,  &AF(InitActorAttack    )   }
};

DECISIONB SumoBroadcast[] =
{
    {2,    attr_alert },
    {4,    attr_ambient  },
    {1024, 0 }
};

DECISION SumoSurprised[] =
{
    {700,   &AF(InitActorMoveCloser)   },
    {703,   &AF(InitActorSetDecide)   },
    {1024,  &AF(InitActorDecide    )   }
};

DECISION SumoEvasive[] =
{
    {1024, &AF(InitActorAttack) }
};

DECISION SumoLostTarget[] =
{
    {900,   &AF(InitActorFindPlayer  )       },
    {1024,  &AF(InitActorWanderAround)       }
};

DECISION SumoCloseRange[] =
{
    {1024,  &AF(InitActorAttack)         }
};

PERSONALITY SumoPersonality =
{
    SumoBattle,
    SumoOffense,
    SumoBroadcast,
    SumoSurprised,
    SumoEvasive,
    SumoLostTarget,
    SumoCloseRange,
    SumoCloseRange
};

ATTRIBUTE SumoAttrib =
{
    {160, 180, 180, 180},             // Speeds
    {3, 0, 0, 0},                     // Tic Adjusts
    3,                               // MaxWeapons;
    {
        DIGI_SUMOAMBIENT, DIGI_SUMOALERT, DIGI_SUMOSCREAM,
        DIGI_SUMOPAIN, DIGI_SUMOSCREAM, 0,0,0,0,0
    }
};


//////////////////////
//
// SUMO RUN
//
//////////////////////

#define SUMO_RATE 24

STATE s_SumoRun[5][4] =
{
    {
        {SUMO_RUN_R0 + 0, SUMO_RATE, &AF(DoSumoMove), &s_SumoRun[0][1]},
        {SUMO_RUN_R0 + 1, SUMO_RATE, &AF(DoSumoMove), &s_SumoRun[0][2]},
        {SUMO_RUN_R0 + 2, SUMO_RATE, &AF(DoSumoMove), &s_SumoRun[0][3]},
        {SUMO_RUN_R0 + 3, SUMO_RATE, &AF(DoSumoMove), &s_SumoRun[0][0]}
    },
    {
        {SUMO_RUN_R1 + 0, SUMO_RATE, &AF(DoSumoMove), &s_SumoRun[1][1]},
        {SUMO_RUN_R1 + 1, SUMO_RATE, &AF(DoSumoMove), &s_SumoRun[1][2]},
        {SUMO_RUN_R1 + 2, SUMO_RATE, &AF(DoSumoMove), &s_SumoRun[1][3]},
        {SUMO_RUN_R1 + 3, SUMO_RATE, &AF(DoSumoMove), &s_SumoRun[1][0]}
    },
    {
        {SUMO_RUN_R2 + 0, SUMO_RATE, &AF(DoSumoMove), &s_SumoRun[2][1]},
        {SUMO_RUN_R2 + 1, SUMO_RATE, &AF(DoSumoMove), &s_SumoRun[2][2]},
        {SUMO_RUN_R2 + 2, SUMO_RATE, &AF(DoSumoMove), &s_SumoRun[2][3]},
        {SUMO_RUN_R2 + 3, SUMO_RATE, &AF(DoSumoMove), &s_SumoRun[2][0]}
    },
    {
        {SUMO_RUN_R3 + 0, SUMO_RATE, &AF(DoSumoMove), &s_SumoRun[3][1]},
        {SUMO_RUN_R3 + 1, SUMO_RATE, &AF(DoSumoMove), &s_SumoRun[3][2]},
        {SUMO_RUN_R3 + 2, SUMO_RATE, &AF(DoSumoMove), &s_SumoRun[3][3]},
        {SUMO_RUN_R3 + 3, SUMO_RATE, &AF(DoSumoMove), &s_SumoRun[3][0]}
    },
    {
        {SUMO_RUN_R4 + 0, SUMO_RATE, &AF(DoSumoMove), &s_SumoRun[4][1]},
        {SUMO_RUN_R4 + 1, SUMO_RATE, &AF(DoSumoMove), &s_SumoRun[4][2]},
        {SUMO_RUN_R4 + 2, SUMO_RATE, &AF(DoSumoMove), &s_SumoRun[4][3]},
        {SUMO_RUN_R4 + 3, SUMO_RATE, &AF(DoSumoMove), &s_SumoRun[4][0]},
    }
};

STATE* sg_SumoRun[] =
{
    &s_SumoRun[0][0],
    &s_SumoRun[1][0],
    &s_SumoRun[2][0],
    &s_SumoRun[3][0],
    &s_SumoRun[4][0]
};



//////////////////////
//
// SUMO STAND
//
//////////////////////

STATE s_SumoStand[5][1] =
{
    {
        {SUMO_RUN_R0 + 0, SUMO_RATE, &AF(DoSumoMove), &s_SumoStand[0][0]}
    },
    {
        {SUMO_RUN_R1 + 0, SUMO_RATE, &AF(DoSumoMove), &s_SumoStand[1][0]}
    },
    {
        {SUMO_RUN_R2 + 0, SUMO_RATE, &AF(DoSumoMove), &s_SumoStand[2][0]}
    },
    {
        {SUMO_RUN_R3 + 0, SUMO_RATE, &AF(DoSumoMove), &s_SumoStand[3][0]}
    },
    {
        {SUMO_RUN_R4 + 0, SUMO_RATE, &AF(DoSumoMove), &s_SumoStand[4][0]}
    }
};

STATE* sg_SumoStand[] =
{
    &s_SumoStand[0][0],
    &s_SumoStand[1][0],
    &s_SumoStand[2][0],
    &s_SumoStand[3][0],
    &s_SumoStand[4][0]
};

//////////////////////
//
// SUMO PAIN
//
//////////////////////

#define SUMO_PAIN_RATE 30

STATE s_SumoPain[5][2] =
{
    {
        {SUMO_PAIN_R0 + 0, SUMO_PAIN_RATE, &AF(NullSumo), &s_SumoPain[0][1]},
        {SUMO_PAIN_R0 + 0, 0|SF_QUICK_CALL, &AF(InitActorDecide), &s_SumoPain[0][0]}
    },
    {
        {SUMO_PAIN_R1 + 0, SUMO_PAIN_RATE, &AF(NullSumo), &s_SumoPain[1][1]},
        {SUMO_PAIN_R1 + 0, 0|SF_QUICK_CALL, &AF(InitActorDecide), &s_SumoPain[1][0]}
    },
    {
        {SUMO_PAIN_R2 + 0, SUMO_PAIN_RATE, &AF(NullSumo), &s_SumoPain[2][1]},
        {SUMO_PAIN_R2 + 0, 0|SF_QUICK_CALL, &AF(InitActorDecide), &s_SumoPain[2][0]}
    },
    {
        {SUMO_PAIN_R3 + 0, SUMO_PAIN_RATE, &AF(NullSumo), &s_SumoPain[3][1]},
        {SUMO_PAIN_R3 + 0, 0|SF_QUICK_CALL, &AF(InitActorDecide), &s_SumoPain[3][0]}
    },
    {
        {SUMO_PAIN_R4 + 0, SUMO_PAIN_RATE, &AF(NullSumo), &s_SumoPain[4][1]},
        {SUMO_PAIN_R4 + 0, 0|SF_QUICK_CALL, &AF(InitActorDecide), &s_SumoPain[4][0]}
    }
};

STATE* sg_SumoPain[] =
{
    &s_SumoPain[0][0],
    &s_SumoPain[1][0],
    &s_SumoPain[2][0],
    &s_SumoPain[3][0],
    &s_SumoPain[4][0]
};

//////////////////////
//
// SUMO FART
//
//////////////////////

#define SUMO_FART_RATE 12

STATE s_SumoFart[5][6] =
{
    {
        {SUMO_FART_R0 + 0, SUMO_FART_RATE, &AF(NullSumo), &s_SumoFart[0][1]},
        {SUMO_FART_R0 + 0, SF_QUICK_CALL, &AF(InitSumoFart), &s_SumoFart[0][2]},
        {SUMO_FART_R0 + 1, SUMO_FART_RATE, &AF(NullSumo), &s_SumoFart[0][3]},
        {SUMO_FART_R0 + 2, SUMO_FART_RATE, &AF(NullSumo), &s_SumoFart[0][4]},
        {SUMO_FART_R0 + 3, SUMO_FART_RATE*10, &AF(NullSumo), &s_SumoFart[0][5]},
        {SUMO_FART_R0 + 3, SF_QUICK_CALL, &AF(InitActorDecide), &s_SumoFart[0][0]}
    },
    {
        {SUMO_FART_R1 + 0, SUMO_FART_RATE, &AF(NullSumo), &s_SumoFart[1][1]},
        {SUMO_FART_R1 + 0, SF_QUICK_CALL, &AF(InitSumoFart), &s_SumoFart[1][2]},
        {SUMO_FART_R1 + 1, SUMO_FART_RATE, &AF(NullSumo), &s_SumoFart[1][3]},
        {SUMO_FART_R1 + 2, SUMO_FART_RATE, &AF(NullSumo), &s_SumoFart[1][4]},
        {SUMO_FART_R1 + 3, SUMO_FART_RATE*10, &AF(NullSumo), &s_SumoFart[1][5]},
        {SUMO_FART_R1 + 0, SF_QUICK_CALL, &AF(InitActorDecide), &s_SumoFart[1][0]}
    },
    {
        {SUMO_FART_R2 + 0, SUMO_FART_RATE, &AF(NullSumo), &s_SumoFart[2][1]},
        {SUMO_FART_R2 + 0, SF_QUICK_CALL, &AF(InitSumoFart), &s_SumoFart[2][2]},
        {SUMO_FART_R2 + 1, SUMO_FART_RATE, &AF(NullSumo), &s_SumoFart[2][3]},
        {SUMO_FART_R2 + 2, SUMO_FART_RATE, &AF(NullSumo), &s_SumoFart[2][4]},
        {SUMO_FART_R2 + 3, SUMO_FART_RATE*10, &AF(NullSumo), &s_SumoFart[2][5]},
        {SUMO_FART_R2 + 0, SF_QUICK_CALL, &AF(InitActorDecide), &s_SumoFart[2][0]}
    },
    {
        {SUMO_FART_R3 + 0, SUMO_FART_RATE, &AF(NullSumo), &s_SumoFart[3][1]},
        {SUMO_FART_R3 + 0, SF_QUICK_CALL, &AF(InitSumoFart), &s_SumoFart[3][2]},
        {SUMO_FART_R3 + 1, SUMO_FART_RATE, &AF(NullSumo), &s_SumoFart[3][3]},
        {SUMO_FART_R3 + 2, SUMO_FART_RATE, &AF(NullSumo), &s_SumoFart[3][4]},
        {SUMO_FART_R3 + 3, SUMO_FART_RATE*10, &AF(NullSumo), &s_SumoFart[3][5]},
        {SUMO_FART_R3 + 0, SF_QUICK_CALL, &AF(InitActorDecide), &s_SumoFart[3][0]}
    },
    {
        {SUMO_FART_R4 + 0, SUMO_FART_RATE, &AF(NullSumo), &s_SumoFart[4][1]},
        {SUMO_FART_R4 + 0, SF_QUICK_CALL, &AF(InitSumoFart), &s_SumoFart[4][2]},
        {SUMO_FART_R4 + 1, SUMO_FART_RATE, &AF(NullSumo), &s_SumoFart[4][3]},
        {SUMO_FART_R4 + 2, SUMO_FART_RATE, &AF(NullSumo), &s_SumoFart[4][4]},
        {SUMO_FART_R4 + 3, SUMO_FART_RATE*10, &AF(NullSumo), &s_SumoFart[4][5]},
        {SUMO_FART_R4 + 0, SF_QUICK_CALL, &AF(InitActorDecide), &s_SumoFart[4][0]}
    }
};

STATE* sg_SumoFart[] =
{
    &s_SumoFart[0][0],
    &s_SumoFart[1][0],
    &s_SumoFart[2][0],
    &s_SumoFart[3][0],
    &s_SumoFart[4][0]
};

//////////////////////
//
// SUMO CLAP
//
//////////////////////

#define SUMO_CLAP_RATE 12

STATE s_SumoClap[5][6] =
{
    {
        {SUMO_CLAP_R0 + 0, SUMO_CLAP_RATE, &AF(NullSumo), &s_SumoClap[0][1]},
        {SUMO_CLAP_R0 + 1, SUMO_CLAP_RATE, &AF(NullSumo), &s_SumoClap[0][2]},
        {SUMO_CLAP_R0 + 2, SUMO_CLAP_RATE, &AF(NullSumo), &s_SumoClap[0][3]},
        {SUMO_CLAP_R0 + 2, SF_QUICK_CALL, &AF(InitSumoClap), &s_SumoClap[0][4]},
        {SUMO_CLAP_R0 + 3, SUMO_CLAP_RATE*10, &AF(NullSumo), &s_SumoClap[0][5]},
        {SUMO_CLAP_R0 + 3, SF_QUICK_CALL, &AF(InitActorDecide), &s_SumoClap[0][5]}
    },
    {
        {SUMO_CLAP_R1 + 0, SUMO_CLAP_RATE, &AF(NullSumo), &s_SumoClap[1][1]},
        {SUMO_CLAP_R1 + 1, SUMO_CLAP_RATE, &AF(NullSumo), &s_SumoClap[1][2]},
        {SUMO_CLAP_R1 + 2, SUMO_CLAP_RATE, &AF(NullSumo), &s_SumoClap[1][3]},
        {SUMO_CLAP_R1 + 2, SF_QUICK_CALL, &AF(InitSumoClap), &s_SumoClap[1][4]},
        {SUMO_CLAP_R1 + 3, SUMO_CLAP_RATE*10, &AF(NullSumo), &s_SumoClap[1][5]},
        {SUMO_CLAP_R1 + 3, SF_QUICK_CALL, &AF(InitActorDecide), &s_SumoClap[1][5]}
    },
    {
        {SUMO_CLAP_R2 + 0, SUMO_CLAP_RATE, &AF(NullSumo), &s_SumoClap[2][1]},
        {SUMO_CLAP_R2 + 1, SUMO_CLAP_RATE, &AF(NullSumo), &s_SumoClap[2][2]},
        {SUMO_CLAP_R2 + 2, SUMO_CLAP_RATE, &AF(NullSumo), &s_SumoClap[2][3]},
        {SUMO_CLAP_R2 + 2, SF_QUICK_CALL, &AF(InitSumoClap), &s_SumoClap[2][4]},
        {SUMO_CLAP_R2 + 3, SUMO_CLAP_RATE*10, &AF(NullSumo), &s_SumoClap[2][5]},
        {SUMO_CLAP_R2 + 3, SF_QUICK_CALL, &AF(InitActorDecide), &s_SumoClap[2][5]}
    },
    {
        {SUMO_CLAP_R3 + 0, SUMO_CLAP_RATE, &AF(NullSumo), &s_SumoClap[3][1]},
        {SUMO_CLAP_R3 + 1, SUMO_CLAP_RATE, &AF(NullSumo), &s_SumoClap[3][2]},
        {SUMO_CLAP_R3 + 2, SUMO_CLAP_RATE, &AF(NullSumo), &s_SumoClap[3][3]},
        {SUMO_CLAP_R3 + 2, SF_QUICK_CALL, &AF(InitSumoClap), &s_SumoClap[3][4]},
        {SUMO_CLAP_R3 + 3, SUMO_CLAP_RATE*10, &AF(NullSumo), &s_SumoClap[3][5]},
        {SUMO_CLAP_R3 + 3, SF_QUICK_CALL, &AF(InitActorDecide), &s_SumoClap[3][5]}
    },
    {
        {SUMO_CLAP_R4 + 0, SUMO_CLAP_RATE, &AF(NullSumo), &s_SumoClap[4][1]},
        {SUMO_CLAP_R4 + 1, SUMO_CLAP_RATE, &AF(NullSumo), &s_SumoClap[4][2]},
        {SUMO_CLAP_R4 + 2, SUMO_CLAP_RATE, &AF(NullSumo), &s_SumoClap[4][3]},
        {SUMO_CLAP_R4 + 2, SF_QUICK_CALL, &AF(InitSumoClap), &s_SumoClap[4][4]},
        {SUMO_CLAP_R4 + 3, SUMO_CLAP_RATE*10, &AF(NullSumo), &s_SumoClap[4][5]},
        {SUMO_CLAP_R4 + 3, SF_QUICK_CALL, &AF(InitActorDecide), &s_SumoClap[4][5]}
    }
};

STATE* sg_SumoClap[] =
{
    &s_SumoClap[0][0],
    &s_SumoClap[1][0],
    &s_SumoClap[2][0],
    &s_SumoClap[3][0],
    &s_SumoClap[4][0]
};

//////////////////////
//
// SUMO STOMP
//
//////////////////////

#define SUMO_STOMP_RATE 30

STATE s_SumoStomp[5][6] =
{
    {
        {SUMO_STOMP_R0 + 0, SUMO_STOMP_RATE, &AF(NullSumo), &s_SumoStomp[0][1]},
        {SUMO_STOMP_R0 + 1, SUMO_STOMP_RATE*3, &AF(NullSumo), &s_SumoStomp[0][2]},
        {SUMO_STOMP_R0 + 2, SUMO_STOMP_RATE, &AF(NullSumo), &s_SumoStomp[0][3]},
        {SUMO_STOMP_R0 + 2, 0|SF_QUICK_CALL, &AF(InitSumoStomp), &s_SumoStomp[0][4]},
        {SUMO_STOMP_R0 + 2, 8, &AF(NullSumo), &s_SumoStomp[0][5]},
        {SUMO_STOMP_R0 + 2, 0|SF_QUICK_CALL, &AF(InitActorDecide), &s_SumoStomp[0][5]}
    },
    {
        {SUMO_STOMP_R1 + 0, SUMO_STOMP_RATE, &AF(NullSumo), &s_SumoStomp[1][1]},
        {SUMO_STOMP_R1 + 1, SUMO_STOMP_RATE*3, &AF(NullSumo), &s_SumoStomp[1][2]},
        {SUMO_STOMP_R1 + 2, SUMO_STOMP_RATE, &AF(NullSumo), &s_SumoStomp[1][3]},
        {SUMO_STOMP_R1 + 2, 0|SF_QUICK_CALL, &AF(InitSumoStomp), &s_SumoStomp[1][4]},
        {SUMO_STOMP_R1 + 2, 8, &AF(NullSumo), &s_SumoStomp[1][5]},
        {SUMO_STOMP_R1 + 2, 0|SF_QUICK_CALL, &AF(InitActorDecide), &s_SumoStomp[1][5]}
    },
    {
        {SUMO_STOMP_R2 + 0, SUMO_STOMP_RATE, &AF(NullSumo), &s_SumoStomp[2][1]},
        {SUMO_STOMP_R2 + 1, SUMO_STOMP_RATE*3, &AF(NullSumo), &s_SumoStomp[2][2]},
        {SUMO_STOMP_R2 + 2, SUMO_STOMP_RATE, &AF(NullSumo), &s_SumoStomp[2][3]},
        {SUMO_STOMP_R2 + 2, 0|SF_QUICK_CALL, &AF(InitSumoStomp), &s_SumoStomp[2][4]},
        {SUMO_STOMP_R2 + 2, 8, &AF(NullSumo), &s_SumoStomp[2][5]},
        {SUMO_STOMP_R2 + 2, 0|SF_QUICK_CALL, &AF(InitActorDecide), &s_SumoStomp[2][5]}
    },
    {
        {SUMO_STOMP_R3 + 0, SUMO_STOMP_RATE, &AF(NullSumo), &s_SumoStomp[3][1]},
        {SUMO_STOMP_R3 + 1, SUMO_STOMP_RATE*3, &AF(NullSumo), &s_SumoStomp[3][2]},
        {SUMO_STOMP_R3 + 2, SUMO_STOMP_RATE, &AF(NullSumo), &s_SumoStomp[3][3]},
        {SUMO_STOMP_R3 + 2, 0|SF_QUICK_CALL, &AF(InitSumoStomp), &s_SumoStomp[3][4]},
        {SUMO_STOMP_R3 + 2, 8, &AF(NullSumo), &s_SumoStomp[3][5]},
        {SUMO_STOMP_R3 + 2, 0|SF_QUICK_CALL, &AF(InitActorDecide), &s_SumoStomp[3][5]}
    },
    {
        {SUMO_STOMP_R4 + 0, SUMO_STOMP_RATE, &AF(NullSumo), &s_SumoStomp[4][1]},
        {SUMO_STOMP_R4 + 1, SUMO_STOMP_RATE*3, &AF(NullSumo), &s_SumoStomp[4][2]},
        {SUMO_STOMP_R4 + 2, SUMO_STOMP_RATE, &AF(NullSumo), &s_SumoStomp[4][3]},
        {SUMO_STOMP_R4 + 2, 0|SF_QUICK_CALL, &AF(InitSumoStomp), &s_SumoStomp[4][4]},
        {SUMO_STOMP_R4 + 2, 8, &AF(NullSumo), &s_SumoStomp[4][5]},
        {SUMO_STOMP_R4 + 2, 0|SF_QUICK_CALL, &AF(InitActorDecide), &s_SumoStomp[4][5]}
    }
};

STATE* sg_SumoStomp[] =
{
    &s_SumoStomp[0][0],
    &s_SumoStomp[1][0],
    &s_SumoStomp[2][0],
    &s_SumoStomp[3][0],
    &s_SumoStomp[4][0]
};


//////////////////////
//
// SUMO DIE
//
//////////////////////

#define SUMO_DIE_RATE 30

STATE s_SumoDie[] =
{
    {SUMO_DIE + 0, SUMO_DIE_RATE*2, &AF(NullSumo), &s_SumoDie[1]},
    {SUMO_DIE + 1, SUMO_DIE_RATE, &AF(NullSumo), &s_SumoDie[2]},
    {SUMO_DIE + 2, SUMO_DIE_RATE, &AF(NullSumo), &s_SumoDie[3]},
    {SUMO_DIE + 3, SUMO_DIE_RATE, &AF(NullSumo), &s_SumoDie[4]},
    {SUMO_DIE + 4, SUMO_DIE_RATE, &AF(NullSumo), &s_SumoDie[5]},
    {SUMO_DIE + 5, SUMO_DIE_RATE, &AF(NullSumo), &s_SumoDie[6]},
    {SUMO_DIE + 6, SUMO_DIE_RATE, &AF(NullSumo), &s_SumoDie[7]},
    {SUMO_DIE + 6, SUMO_DIE_RATE*3, &AF(NullSumo), &s_SumoDie[8]},
    {SUMO_DIE + 7, SUMO_DIE_RATE, &AF(NullSumo), &s_SumoDie[9]},
    {SUMO_DIE + 6, SUMO_DIE_RATE, &AF(NullSumo), &s_SumoDie[10]},
    {SUMO_DIE + 7, SUMO_DIE_RATE, &AF(NullSumo), &s_SumoDie[11]},
    {SUMO_DIE + 6, SUMO_DIE_RATE-8, &AF(NullSumo), &s_SumoDie[12]},
    {SUMO_DIE + 7, SUMO_DIE_RATE, &AF(NullSumo), &s_SumoDie[13]},
    {SUMO_DIE + 7, SF_QUICK_CALL, &AF(DoSumoDeathMelt), &s_SumoDie[14]},
    {SUMO_DIE + 6, SUMO_DIE_RATE-15, &AF(NullSumo), &s_SumoDie[15]},
    {SUMO_DEAD, SF_QUICK_CALL, &AF(QueueFloorBlood), &s_SumoDie[16]},
    {SUMO_DEAD, SUMO_DIE_RATE, &AF(DoActorDebris), &s_SumoDie[16]}
};

STATE* sg_SumoDie[] =
{
    s_SumoDie
};

STATE s_SumoDead[] =
{
    {SUMO_DEAD, SUMO_DIE_RATE, &AF(DoActorDebris), &s_SumoDead[0]},
};

STATE* sg_SumoDead[] =
{
    s_SumoDead
};

ACTOR_ACTION_SET SumoActionSet =
{
    sg_SumoStand,
    sg_SumoRun,
    nullptr,
    nullptr,
    nullptr,
    nullptr,
    nullptr,
    nullptr,
    nullptr,
    nullptr,
    nullptr, //climb
    sg_SumoPain, //pain
    sg_SumoDie,
    nullptr,
    sg_SumoDead,
    nullptr,
    nullptr,
    {sg_SumoStomp,sg_SumoFart},
    {800,1024},
    {sg_SumoClap,sg_SumoStomp,sg_SumoFart},
    {400,750,1024},
    {nullptr},
    nullptr,
    nullptr
};

ACTOR_ACTION_SET MiniSumoActionSet =
{
    sg_SumoStand,
    sg_SumoRun,
    nullptr,
    nullptr,
    nullptr,
    nullptr,
    nullptr,
    nullptr,
    nullptr,
    nullptr,
    nullptr, //climb
    sg_SumoPain, //pain
    sg_SumoDie,
    nullptr,
    sg_SumoDead,
    nullptr,
    nullptr,
    {sg_SumoClap},
    {1024},
    {sg_SumoClap},
    {1024},
    {nullptr},
    nullptr,
    nullptr
};


//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

int SetupSumo(DSWActor* actor)
{
    if (!(actor->spr.cstat & CSTAT_SPRITE_RESTORE))
    {
        SpawnUser(actor,SUMO_RUN_R0,s_SumoRun[0]);
        actor->user.Health = 6000;
    }

    if (Skill == 0) actor->user.Health = 2000;
    if (Skill == 1) actor->user.Health = 4000;

    ChangeState(actor,s_SumoRun[0]);
    actor->user.__legacyState.Attrib = &SumoAttrib;
    DoActorSetSpeed(actor, NORM_SPEED);
    actor->user.__legacyState.StateEnd = s_SumoDie;
    actor->user.__legacyState.Rot = sg_SumoRun;

    EnemyDefaults(actor, &SumoActionSet, &SumoPersonality);

    actor->clipdist = 32;
    if (actor->spr.pal == 16)
    {
        // Mini Sumo
        actor->spr.scale = DVector2(0.671875, 0.453125);
        actor->user.__legacyState.ActorActionSet = &MiniSumoActionSet;
        actor->user.Health = 500;
    }
    else
    {
        actor->spr.scale = DVector2(1.796875, 1.171875);
    }

    //actor->user.Flags |= (SPR_XFLIP_TOGGLE);

    return 0;
}

DEFINE_ACTION_FUNCTION(DSWSumo, Initialize)
{
    PARAM_SELF_PROLOGUE(DSWActor);
    SetupSumo(self);
    return 0;
}


//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

int NullSumo(DSWActor* actor)
{
    if (!(actor->user.Flags & SPR_CLIMBING))
        KeepActorOnFloor(actor);

    DoActorSectorDamage(actor);

    return 0;
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

int DoSumoMove(DSWActor* actor)
{
    if (actor->user.track >= 0)
        ActorFollowTrack(actor, ACTORMOVETICS);
    else
        actor->callAction();

    KeepActorOnFloor(actor);

    if (DoActorSectorDamage(actor))
    {
        return 0;
    }

    return 0;
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

int DoSumoRumble(DSWActor* actor)
{
    SetSumoQuake(actor);
    return 0;
}

int InitSumoFart(DSWActor* actor)
{
    PlaySound(DIGI_SUMOFART, actor, v3df_follow);
    InitChemBomb(actor);
    SetSumoFartQuake(actor);
    InitSumoNapalm(actor);
    return 0;
}

int InitSumoStomp(DSWActor* actor)
{
    PlaySound(DIGI_SUMOSTOMP, actor, v3df_none);
    SetSumoQuake(actor);
    InitSumoStompAttack(actor);
    return 0;
}

int InitSumoClap(DSWActor* actor)
{
    if (actor->spr.pal == 16 && RandomRange(1000) <= 800)
        InitMiniSumoClap(actor);
    else
        InitSumoSkull(actor);
    return 0;
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

int DoSumoDeathMelt(DSWActor* actor)
{
    PlaySound(DIGI_SUMOFART, actor, v3df_follow);

    actor->user.ID = SUMO_RUN_R0;
    InitChemBomb(actor);
    actor->user.ID = 0;

    DoMatchEverything(nullptr, actor->spr.lotag, 1);
    if (!SW_SHAREWARE)
    {
        // Resume the regular music - in a hack-free fashion.
        PlaySong(currentLevel->music, currentLevel->cdSongId);
    }

    BossSpriteNum[1] = nullptr; // Sprite is gone, set it back to keep it valid!

    return 0;
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void BossHealthMeter(void)
{
    SWPlayer* pp = Player + myconnectindex;
    short color=0,metertics,meterunit;
    int y;
    extern bool NoMeters;
    short health;
    static bool triedplay = false;

    if (NoMeters) return;

    if (!(currentLevel->gameflags & (LEVEL_SW_BOSSMETER_SERPENT | LEVEL_SW_BOSSMETER_SUMO | LEVEL_SW_BOSSMETER_ZILLA))) return;

    // Don't draw bar for other players
    if (pp != &Player[myconnectindex])
        return;

    // all enemys
    if (currentLevel->gameflags & (LEVEL_SW_BOSSMETER_SERPENT|LEVEL_SW_BOSSMETER_SUMO|LEVEL_SW_BOSSMETER_ZILLA) &&
        BossSpriteNum[0] == nullptr && BossSpriteNum[1] == nullptr && BossSpriteNum[2] == nullptr)
    {
        SWStatIterator it(STAT_ENEMY);
        while (auto itActor = it.Next())
        {
            if ((itActor->user.ID == SERP_RUN_R0 || itActor->user.ID == SUMO_RUN_R0 || itActor->user.ID == ZILLA_RUN_R0) && itActor->spr.pal != 16)
            {
                if (itActor->user.ID == SERP_RUN_R0 && (currentLevel->gameflags & LEVEL_SW_BOSSMETER_SERPENT))
                    BossSpriteNum[0] = itActor;
                else if (itActor->user.ID == SUMO_RUN_R0 && (currentLevel->gameflags & LEVEL_SW_BOSSMETER_SUMO))
                    BossSpriteNum[1] = itActor;
                else if (itActor->user.ID == ZILLA_RUN_R0 && (currentLevel->gameflags & LEVEL_SW_BOSSMETER_ZILLA))
                    BossSpriteNum[2] = itActor;
            }
        }
    }

    if (BossSpriteNum[0] == nullptr && BossSpriteNum[1] == nullptr && BossSpriteNum[2] == nullptr)
        return;


    // Only show the meter when you can see the boss
    for (int i=0; i<3; i++)
    {
        DSWActor* actor = BossSpriteNum[i];
        if (actor != nullptr && !bosswasseen[i])
        {
            if (cansee(ActorVectOfTop(actor), actor->sector(), pp->GetActor()->getPosWithOffsetZ().plusZ(-40), pp->cursector))
            {
                if (i == 0 && !bosswasseen[0])
                {
                    bosswasseen[0] = true;
                    if (!SW_SHAREWARE)
                    {
                        PlaySong(ThemeSongs[2], ThemeTrack[2], true);
                    }
                }
                else if (i == 1 && !bosswasseen[1])
                {
                    bosswasseen[1] = true;
                    if (!SW_SHAREWARE)
                    {
                        PlaySong(ThemeSongs[3], ThemeTrack[3], true);
                    }
                }
                else if (i == 2 && !bosswasseen[2])
                {
                    bosswasseen[2] = true;
                    if (!SW_SHAREWARE)
                    {
                        PlaySong(ThemeSongs[4], ThemeTrack[4], true);
                    }
                }
            }
        }
    }


    for (int i=0; i<3; i++)
    {
        DSWActor* actor = BossSpriteNum[i];
        if ((!bosswasseen[i] || actor == nullptr))
            continue;

        if (actor->user.ID == SERP_RUN_R0 && bosswasseen[0])
        {
            if (Skill == 0) health = 1100;
            else if (Skill == 1) health = 2200;
            else
                health = HEALTH_SERP_GOD;
            meterunit = health / 30;
        }
        else if (actor->user.ID == SUMO_RUN_R0 && bosswasseen[1])
        {
            if (Skill == 0) health = 2000;
            else if (Skill == 1) health = 4000;
            else
                health = 6000;
            meterunit = health / 30;
        }
        else if (actor->user.ID == ZILLA_RUN_R0 && bosswasseen[2])
        {
            if (Skill == 0) health = 2000;
            else if (Skill == 1) health = 4000;
            else
                health = 6000;
            meterunit = health / 30;
        }
        else
            continue;

        if (meterunit > 0)
        {
            if (actor->user.Health < meterunit && actor->user.Health > 0)
                metertics = 1;
            else
                metertics = actor->user.Health / meterunit;
        }
        else
            continue;

        if (metertics <= 0)
        {
            continue;
        }

        if (numplayers < 2) y = 10;
        else if (numplayers >=2 && numplayers <= 4) y = 20;
        else
            y = 30;

        if ((currentLevel->gameflags & (LEVEL_SW_BOSSMETER_SUMO|LEVEL_SW_BOSSMETER_ZILLA)) == (LEVEL_SW_BOSSMETER_SUMO | LEVEL_SW_BOSSMETER_ZILLA) && numplayers >= 2)
        {
            if (actor->user.ID == SUMO_RUN_R0 && bosswasseen[1]) y += 10;
            else if (actor->user.ID == ZILLA_RUN_R0 && bosswasseen[2]) y += 20;
        }

        if (metertics <= 12 && metertics > 6)
            color = 20;
        else if (metertics <= 6)
            color = 25;
        else
            color = 22;

        DrawTexture(twod, tileGetTexture(5407, true), 85, y, DTA_FullscreenScale, FSMode_Fit320x200,
            DTA_CenterOffsetRel, 2, DTA_TranslationIndex, TRANSLATION(Translation_Remap, 1), TAG_DONE);

        DrawTexture(twod, tileGetTexture(5406 - metertics, true), 147, y, DTA_FullscreenScale, FSMode_Fit320x200,
            DTA_CenterOffsetRel, 2, DTA_TranslationIndex, TRANSLATION(Translation_Remap, color), TAG_DONE);
    }

}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------


#include "saveable.h"

static saveable_data saveable_sumo_data[] =
{
    SAVE_DATA(SumoPersonality),

    SAVE_DATA(SumoAttrib),

    SAVE_DATA(s_SumoRun),
    SAVE_DATA(sg_SumoRun),
    SAVE_DATA(s_SumoStand),
    SAVE_DATA(sg_SumoStand),
    SAVE_DATA(s_SumoPain),
    SAVE_DATA(sg_SumoPain),
    SAVE_DATA(s_SumoFart),
    SAVE_DATA(sg_SumoFart),
    SAVE_DATA(s_SumoClap),
    SAVE_DATA(sg_SumoClap),
    SAVE_DATA(s_SumoStomp),
    SAVE_DATA(sg_SumoStomp),
    SAVE_DATA(s_SumoDie),
    SAVE_DATA(sg_SumoDie),
    SAVE_DATA(s_SumoDead),
    SAVE_DATA(sg_SumoDead),

    SAVE_DATA(SumoActionSet),
    SAVE_DATA(MiniSumoActionSet),
};

saveable_module saveable_sumo =
{
    // code
    nullptr, 0,

    // data
    saveable_sumo_data,
    SIZ(saveable_sumo_data)
};
END_SW_NS
