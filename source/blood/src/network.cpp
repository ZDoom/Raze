//-------------------------------------------------------------------------
/*
Copyright (C) 2010-2019 EDuke32 developers and contributors
Copyright (C) 2019 Nuke.YKT

This file is part of NBlood.

NBlood is free software; you can redistribute it and/or
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
#include "ns.h"	// Must come before everything else!

#include "build.h"
#include "mmulti.h"
#include "pragmas.h"
#include "compat.h"
#include "controls.h"
#include "globals.h"
#include "network.h"
#include "player.h"
#include "seq.h"
#include "sound.h"
#include "view.h"
#include "menu.h"
#include "gamestate.h"

extern bool gHaveNetworking;

BEGIN_BLD_NS

char packet[576];
MapRecord *gStartNewGame = 0;
PACKETMODE gPacketMode = PACKETMODE_1;
ClockTicks gNetFifoClock = 0;
int gNetFifoTail = 0;
int gNetFifoHead[8];
int gPredictTail = 0;
int gNetFifoMasterTail = 0;
GINPUT gFifoInput[256][8];
int myMinLag[8];
int otherMinLag = 0;
int myMaxLag = 0;
unsigned int gChecksum[4];
unsigned int gCheckFifo[256][8][4];
int gCheckHead[8];
int gSendCheckTail = 0;
int gCheckTail = 0;
int gInitialNetPlayers = 0;
int gBufferJitter = 1;
int gPlayerReady[8];
bool bNoResend = true;
bool gRobust = false;
bool bOutOfSync = false;
bool ready2send = false;

NETWORKMODE gNetMode = NETWORK_NONE;
char gNetAddress[32];
// PORT-TODO: Use different port?
int gNetPort = kNetDefaultPort;

const short word_1328AC = 0x214;

void netResetToSinglePlayer(void)
{
    myconnectindex = connecthead = 0;
    gInitialNetPlayers = gNetPlayers = numplayers = 1;
    connectpoint2[0] = -1;
    gGameOptions.nGameType = 0;
    gNetMode = NETWORK_NONE;
    UpdateNetworkMenus();
}

void netSendPacket(int nDest, char *pBuffer, int nSize)
{
}

void netSendPacketAll(char *pBuffer, int nSize)
{
}

void netReset(void)
{
    gNetFifoClock = gFrameClock = totalclock = 0;
    gNetFifoMasterTail = 0;
    gPredictTail = 0;
    gNetFifoTail = 0;
    memset(gNetFifoHead, 0, sizeof(gNetFifoHead));
    memset(gCheckFifo, 0, sizeof(gCheckFifo));
    memset(myMinLag, 0, sizeof(myMinLag));
    otherMinLag = 0;
    myMaxLag = 0;
    memset(gCheckHead, 0, sizeof(gCheckHead));
    gSendCheckTail = 0;
    gCheckTail = 0;
    bOutOfSync = 0;
    gBufferJitter = 1;
}

void CalcGameChecksum(void)
{
}

void netCheckSync(void)
{
}

short netGetPacket(short *pSource, char *pMessage)
{
    return 0;
}

void netGetPackets(void)
{
}

void netBroadcastPlayerLogoff(int nPlayer)
{
}

void netBroadcastMyLogoff(bool bRestart)
{
}

void netBroadcastPlayerInfo(int nPlayer)
{
    PROFILE *pProfile = &gProfile[nPlayer];
    strcpy(pProfile->name, playername);
    pProfile->skill = gSkill;
    pProfile->nAutoAim = cl_autoaim;
    pProfile->nWeaponSwitch = cl_weaponswitch;
    if (numplayers < 2)
        return;
}

void netBroadcastNewGame(void)
{
}

void netWaitForEveryone(char a1)
{
}

void netMasterUpdate(void)
{
    if (myconnectindex != connecthead)
        return;
    char v4 = 0;
    do
    {
        for (int p = connecthead; p >= 0; p = connectpoint2[p])
            if (gNetFifoMasterTail >= gNetFifoHead[p])
            {
                if (v4)
                    return;
                char *pPacket = packet;
                PutPacketByte(pPacket, 254);
                for (; p >= 0; p = connectpoint2[p])
                    netSendPacket(p, packet, pPacket-packet);
                return;
            }
        v4 = 1;
        char *pPacket = packet;
        PutPacketByte(pPacket, 0);
        for (int p = connecthead; p >= 0; p = connectpoint2[p])
        {
            GINPUT *pInput = &gFifoInput[gNetFifoMasterTail&255][p];
            if (pInput->buttonFlags.byte)
                pInput->syncFlags.buttonChange = 1;
            if (pInput->keyFlags.word)
                pInput->syncFlags.keyChange = 1;
            if (pInput->useFlags.byte)
                pInput->syncFlags.useChange = 1;
            if (pInput->newWeapon)
                pInput->syncFlags.weaponChange = 1;
            if (pInput->q16mlook)
                pInput->syncFlags.mlookChange = 1;
            PutPacketByte(pPacket, pInput->syncFlags.byte);
            PutPacketWord(pPacket, pInput->forward);
            PutPacketDWord(pPacket, pInput->q16turn);
            PutPacketWord(pPacket, pInput->strafe);
            if (pInput->syncFlags.buttonChange)
                PutPacketByte(pPacket, pInput->buttonFlags.byte);
            if (pInput->syncFlags.keyChange)
                PutPacketWord(pPacket, pInput->keyFlags.word);
            if (pInput->syncFlags.useChange)
                PutPacketByte(pPacket, pInput->useFlags.byte);
            if (pInput->syncFlags.weaponChange)
                PutPacketByte(pPacket, pInput->newWeapon);
            if (pInput->syncFlags.mlookChange)
                PutPacketDWord(pPacket, pInput->q16mlook);
        }
        if ((gNetFifoMasterTail&15) == 0)
        {
            for (int p = connectpoint2[connecthead]; p >= 0; p = connectpoint2[p])
                PutPacketByte(pPacket, ClipRange(myMinLag[p], -128, 127));
            for (int p = connecthead; p >= 0; p = connectpoint2[p])
                myMinLag[p] = 0x7fffffff;
        }
        while (gSendCheckTail != gCheckHead[myconnectindex])
        {
            PutPacketBuffer(pPacket, gCheckFifo[gSendCheckTail&255][myconnectindex], 16);
            gSendCheckTail++;
        }
        for (int p = connectpoint2[connecthead]; p >= 0; p = connectpoint2[p])
            netSendPacket(p, packet, pPacket-packet);
        gNetFifoMasterTail++;
    } while (1);
}

void netGetInput(void)
{
    if (numplayers > 1)
        netGetPackets();
    for (int p = connecthead; p >= 0; p = connectpoint2[p])
        if (gNetFifoHead[myconnectindex]-200 > gNetFifoHead[p])
            return;
    GINPUT &input = gFifoInput[gNetFifoHead[myconnectindex]&255][myconnectindex];
    input = gNetInput;
    gNetFifoHead[myconnectindex]++;
    if (gGameOptions.nGameType == 0 || numplayers == 1)
    {
        for (int p = connecthead; p >= 0; p = connectpoint2[p])
        {
            if (p != myconnectindex)
            {
                GINPUT *pInput1 = &gFifoInput[(gNetFifoHead[p]-1)&255][p];
                GINPUT *pInput2 = &gFifoInput[gNetFifoHead[p]&255][p];
                memcpy(pInput2, pInput1, sizeof(GINPUT));
                gNetFifoHead[p]++;
            }
        }
        return;
    }
    for (int p = connecthead; p >= 0; p = connectpoint2[p])
    {
        if (p != myconnectindex)
        {
            int nLag = gNetFifoHead[myconnectindex]-1-gNetFifoHead[p];
            myMinLag[p] = ClipHigh(nLag, myMinLag[p]);
            myMaxLag = ClipLow(nLag, myMaxLag);
        }
    }
    if (((gNetFifoHead[myconnectindex]-1)&15) == 0)
    {
        int t = myMaxLag-gBufferJitter;
        myMaxLag = 0;
        if (t > 0)
            gBufferJitter += (3+t)>>2;
        else if (t < 0)
            gBufferJitter -= (1-t)>>2;
    }
    if (myconnectindex != connecthead)
    {
        char *pPacket = packet;
        PutPacketByte(pPacket, 1);
        if (input.buttonFlags.byte)
            input.syncFlags.buttonChange = 1;
        if (input.keyFlags.word)
            input.syncFlags.keyChange = 1;
        if (input.useFlags.byte)
            input.syncFlags.useChange = 1;
        if (input.newWeapon)
            input.syncFlags.weaponChange = 1;
        if (input.q16mlook)
            input.syncFlags.mlookChange = 1;
        PutPacketByte(pPacket, input.syncFlags.byte);
        PutPacketWord(pPacket, input.forward);
        PutPacketDWord(pPacket, input.q16turn);
        PutPacketWord(pPacket, input.strafe);
        if (input.syncFlags.buttonChange)
            PutPacketByte(pPacket, input.buttonFlags.byte);
        if (input.syncFlags.keyChange)
            PutPacketWord(pPacket, input.keyFlags.word);
        if (input.syncFlags.useChange)
            PutPacketByte(pPacket, input.useFlags.byte);
        if (input.syncFlags.weaponChange)
            PutPacketByte(pPacket, input.newWeapon);
        if (input.syncFlags.mlookChange)
            PutPacketDWord(pPacket, input.q16mlook);
        if (((gNetFifoHead[myconnectindex]-1)&15) == 0)
        {
            int t = myMinLag[connecthead]-otherMinLag;
            if (klabs(t) > 2)
            {
                if (klabs(t) > 8)
                {
                    if (t < 0)
                        t++;
                    t >>= 1;
                }
                else
                    t = ksgn(t);
                totalclock -= t<<2;
                otherMinLag += t;
                myMinLag[connecthead] -= t;
            }
            for (int p = connecthead; p >= 0; p = connectpoint2[p])
                myMinLag[p] = 0x7fffffff;
        }
        while (gSendCheckTail != gCheckHead[myconnectindex])
        {
            PutPacketBuffer(pPacket, gCheckFifo[gSendCheckTail&255][myconnectindex], 16);
            gSendCheckTail++;
        }
        netSendPacket(connecthead, packet, pPacket-packet);
        return;
    }
    netMasterUpdate();
}

void netInitialize(bool bConsole)
{
    memset(gPlayerReady, 0, sizeof(gPlayerReady));
    netReset();
    netResetToSinglePlayer();
}

void netDeinitialize(void)
{
}

void netPlayerQuit(int nPlayer)
{
}

END_BLD_NS
