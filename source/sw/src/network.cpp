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
#include "baselayer.h"
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

void getinput(SW_PACKET *, SWBOOL);

static uint8_t tempbuf[576], packbuf[576];
int PlayClock;

gNET gNet;
extern short PlayerQuitMenuLevel;

#define TIMERUPDATESIZ 32

//SW_PACKET fsync;

//Local multiplayer variables
// should move this to a local scope of faketimerhandler - do it when able to test
SW_PACKET loc;

//SW_PACKET oloc;

SWBOOL ready2send = 0;

SWBOOL CommEnabled = FALSE;
uint8_t CommPlayers = 0;
int movefifoplc, movefifosendplc; //, movefifoend[MAX_SW_PLAYERS];
unsigned int MoveThingsCount;

//int myminlag[MAX_SW_PLAYERS];
int mymaxlag, otherminlag, bufferjitter = 1;
extern char sync_first[MAXSYNCBYTES][60];
extern int sync_found;

//
// Tic Duplication - so you can move multiple times per packet
//
typedef struct
{
    int32_t vel;
    int32_t svel;
    fix16_t q16angvel;
    fix16_t q16aimvel;
    fix16_t q16ang;
    fix16_t q16horiz;
    int32_t bits;
} SW_AVERAGE_PACKET;

int MovesPerPacket = 1;
SW_AVERAGE_PACKET AveragePacket;

// GAME.C sync state variables
uint8_t syncstat[MAXSYNCBYTES];
//int syncvalhead[MAX_SW_PLAYERS];
int syncvaltail, syncvaltottail;
void GetSyncInfoFromPacket(uint8_t *packbuf, int packbufleng, int *j, int otherconnectindex);

// when you set totalclock to 0 also set this one
int ototalclock;
int smoothratio;
int save_totalclock;

// must start out as 0

void
ResumeGame(void)
{
    if (paused)
        return;

    if (numplayers < 2)
        paused = 0;
}

void
InitNetPlayerOptions(void)
{
//    short pnum;
    PLAYERp pp = Player + myconnectindex;

    strncpy(pp->PlayerName, playername, 32);

    // myconnectindex palette
    pp->TeamColor = gs.NetColor;
    pp->SpriteP->pal = PALETTE_PLAYER0 + pp->TeamColor;
    User[pp->SpriteP - sprite]->spal = pp->SpriteP->pal;
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
        Player[pnum].syncvalhead = 0;
        memset(pp->inputfifo,0,sizeof(pp->inputfifo));
    }
    movefifoplc = 0;
    movefifosendplc = 0;
    syncvaltail = 0;
    syncvaltottail = 0;
    predictmovefifoplc = 0;

    memset(&syncstat, 0, sizeof(syncstat));
    memset(sync_first, 0, sizeof(sync_first));
    sync_found = FALSE;

    TRAVERSE_CONNECT(pnum)
    {
        Player[pnum].myminlag = 0;
    }

    otherminlag = mymaxlag = 0;
}

void
InitTimingVars(void)
{
    PlayClock = 0;

    // resettiming();
    totalsynctics = 0;
    totalclock = 0;
    ototalclock = 0;
    randomseed = 17L;

    MoveSkip8 = 2;
    MoveSkip2 = 0;
    MoveSkip4 = 1;                      // start slightly offset so these
    // don't move the same
    // as the Skip2's
    MoveThingsCount = 0;

    // CTW REMOVED
    //if (gTenActivated)
    //	tenResetClock();
    // CTW REMOVED END

}


void
UpdateInputs(void)
{
    int i, j, k;
    PLAYERp pp;

    ototalclock += synctics;

    //getpackets();

    if (Player[myconnectindex].movefifoend - movefifoplc >= 100)
        return;

    getinput(&loc, FALSE);

    AveragePacket.vel += loc.vel;
    AveragePacket.svel += loc.svel;
    AveragePacket.q16angvel += loc.q16angvel;
    AveragePacket.q16aimvel += loc.q16aimvel;
    AveragePacket.q16ang = Player[myconnectindex].camq16ang;
    AveragePacket.q16horiz = Player[myconnectindex].camq16horiz;
    SET(AveragePacket.bits, loc.bits);

    Bmemset(&loc, 0, sizeof(loc));

    pp = Player + myconnectindex;

    if (pp->movefifoend & (MovesPerPacket-1))
    {
        memcpy(&pp->inputfifo[pp->movefifoend & (MOVEFIFOSIZ - 1)],
               &pp->inputfifo[(pp->movefifoend-1) & (MOVEFIFOSIZ - 1)],
               sizeof(SW_PACKET));

        pp->movefifoend++;
        return;
    }

    loc.vel = AveragePacket.vel / MovesPerPacket;
    loc.svel = AveragePacket.svel / MovesPerPacket;
    loc.q16angvel = fix16_div(AveragePacket.q16angvel, fix16_from_int(MovesPerPacket));
    loc.q16aimvel = fix16_div(AveragePacket.q16aimvel, fix16_from_int(MovesPerPacket));
    loc.q16ang = AveragePacket.q16ang;
    loc.q16horiz = AveragePacket.q16horiz;
    loc.bits = AveragePacket.bits;

    memset(&AveragePacket, 0, sizeof(AveragePacket));

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
