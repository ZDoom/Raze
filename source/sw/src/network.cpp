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
#include "mmulti.h"

#include "gamecontrol.h"

#include "game.h"
#include "tags.h"
#include "names2.h"
#include "network.h"
#include "pal.h"

#include "weapon.h"
#include "menus.h"
#include "printf.h"

BEGIN_SW_NS

void getinput(InputPacket *, SWBOOL);

static uint8_t tempbuf[576], packbuf[576];
int PlayClock;

gNET gNet;

#define TIMERUPDATESIZ 32

//InputPacket fsync;

//Local multiplayer variables
// should move this to a local scope of faketimerhandler - do it when able to test
InputPacket loc;

//InputPacket oloc;

SWBOOL ready2send = 0;

SWBOOL CommEnabled = FALSE;
uint8_t CommPlayers = 0;
int movefifoplc, movefifosendplc; //, movefifoend[MAX_SW_PLAYERS];

int bufferjitter = 1;

int ogameclock;
double smoothratio;

// must start out as 0

void
InitNetPlayerOptions(void)
{
//    short pnum;
    PLAYERp pp = Player + myconnectindex;

    strncpy(pp->PlayerName, playername, 32);

    // myconnectindex palette
    pp->TeamColor = gs.NetColor;
    if (pp->SpriteP)
    {
        pp->SpriteP->pal = PALETTE_PLAYER0 + pp->TeamColor;
        User[pp->SpriteP - sprite]->spal = pp->SpriteP->pal;
    }
}

void
waitforeverybody(void)
{
}



void
InitNetVars(void)
{
    short pnum;
    PLAYERp pp;

    memset(&loc, 0, sizeof(loc));

    TRAVERSE_CONNECT(pnum)
    {
        pp = Player + pnum;
        pp->movefifoend = 0;
        memset(pp->inputfifo,0,sizeof(pp->inputfifo));
    }
    movefifoplc = 0;
    movefifosendplc = 0;
    predictmovefifoplc = 0;
}

void
InitTimingVars(void)
{
    PlayClock = 0;

    // resettiming();
    totalsynctics = 0;
    randomseed = 17L;

    MoveSkip8 = 2;
    MoveSkip2 = 0;
    MoveSkip4 = 1;                      // start slightly offset so these
}


void
UpdateInputs(void)
{
    int i, j, k;
    PLAYERp pp;

    //getpackets();

    if (Player[myconnectindex].movefifoend - movefifoplc >= 100)
        return;

    getinput(&loc, FALSE);


    pp = Player + myconnectindex;

    loc.q16ang = pp->camq16ang;
    loc.q16horiz = pp->camq16horiz;

    pp->inputfifo[Player[myconnectindex].movefifoend & (MOVEFIFOSIZ - 1)] = loc;
    pp->movefifoend++;
    Bmemset(&loc, 0, sizeof(loc));

    if (!CommEnabled)
    {
        TRAVERSE_CONNECT(i)
        {
            if (i != myconnectindex)
            {
                memset(&Player[i].inputfifo[Player[i].movefifoend & (MOVEFIFOSIZ - 1)], 0, sizeof(Player[i].inputfifo[0]));
                Player[i].movefifoend++;
            }
        }
        return;
    }
}

END_SW_NS
