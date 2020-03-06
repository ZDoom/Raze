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
#ifndef NETCODE_DISABLE
#include "enet.h"
#endif

#include "build.h"
#include "mmulti.h"
#include "pragmas.h"
#include "compat.h"
#include "config.h"
#include "controls.h"
#include "globals.h"
#include "network.h"
#include "gamemenu.h"
#include "player.h"
#include "seq.h"
#include "sound.h"
#include "view.h"
#include "menu/menu.h"

extern bool gHaveNetworking;

BEGIN_BLD_NS

char packet[576];
bool gStartNewGame = 0;
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
int gSyncRate = 1;
bool bNoResend = true;
bool gRobust = false;
bool bOutOfSync = false;
bool ready2send = false;

NETWORKMODE gNetMode = NETWORK_NONE;
char gNetAddress[32];
// PORT-TODO: Use different port?
int gNetPort = kNetDefaultPort;

const short word_1328AC = 0x214;

struct struct28E3B0
{
    int at0;
    int at4;
    int at8;
    int atc;
    int at10;
    int at14;
    int at18;
    char at1c;
    int at1d;
};

struct28E3B0 byte_28E3B0;

PKT_STARTGAME gPacketStartGame;

#ifndef NETCODE_DISABLE
ENetAddress gNetENetAddress;
ENetHost *gNetENetServer;
ENetHost *gNetENetClient;
ENetPeer *gNetENetPeer;
ENetPeer *gNetPlayerPeer[kMaxPlayers];
bool gNetENetInit = false;

#define kENetFifoSize 2048

//ENetPacket *gNetServerPacketFifo[kENetFifoSize];
//int gNetServerPacketHead, gNetServerPacketTail;
ENetPacket *gNetPacketFifo[kENetFifoSize];
int gNetPacketHead, gNetPacketTail;

enum BLOOD_ENET_CHANNEL {
    BLOOD_ENET_SERVICE = 0,
    BLOOD_ENET_GAME,
    BLOOD_ENET_CHANNEL_MAX
};

enum BLOOD_SERVICE {
    BLOOD_SERVICE_CONNECTINFO = 0,
    BLOOD_SERVICE_CONNECTID,
    BLOOD_SERVICE_SENDTOID,
};

struct PKT_CONNECTINFO {
    short numplayers;
    short connecthead;
    short connectpoint2[kMaxPlayers];
};

void netServerDisconnect(void)
{
    ENetEvent event;
    if (gNetMode != NETWORK_SERVER)
        return;
    for (int p = 0; p < kMaxPlayers; p++)
        if (gNetPlayerPeer[p])
        {
            bool bDisconnectStatus = false;
            enet_peer_disconnect_later(gNetPlayerPeer[p], 0);
            if (enet_host_service(gNetENetServer, &event, 3000) > 0)
            {
                switch (event.type)
                {
                case ENET_EVENT_TYPE_DISCONNECT:
                    bDisconnectStatus = true;
                    break;
                default:
                    break;
                }
            }
            if (!bDisconnectStatus)
                enet_peer_reset(gNetPlayerPeer[p]);
            gNetPlayerPeer[p] = NULL;
        }
}

void netClientDisconnect(void)
{
    ENetEvent event;
    if (gNetMode != NETWORK_CLIENT || gNetENetPeer == NULL)
        return;
    enet_peer_disconnect_later(gNetENetPeer, 0);
    bool bDisconnectStatus = false;
    if (enet_host_service(gNetENetClient, &event, 3000) > 0)
    {
        switch (event.type)
        {
        case ENET_EVENT_TYPE_DISCONNECT:
            bDisconnectStatus = true;
            break;
        default:
            break;
        }
    }
    if (!bDisconnectStatus)
        enet_peer_reset(gNetENetPeer);
    gNetENetPeer = NULL;
}
#endif

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
#ifndef NETCODE_DISABLE
    if (gNetMode == NETWORK_NONE)
        return;
    netUpdate();

    if (gNetMode == NETWORK_SERVER)
    {
        if (gNetPlayerPeer[nDest] != NULL)
        {
            ENetPacket *pNetPacket = enet_packet_create(NULL, nSize + 1, ENET_PACKET_FLAG_RELIABLE);
            char *pPBuffer = (char*)pNetPacket->data;
            PutPacketByte(pPBuffer, myconnectindex);
            PutPacketBuffer(pPBuffer, pBuffer, nSize);
            enet_peer_send(gNetPlayerPeer[nDest], BLOOD_ENET_GAME, pNetPacket);
            enet_host_service(gNetENetServer, NULL, 0);
        }
    }
    else
    {
        if (nDest == 0)
        {
            ENetPacket *pNetPacket = enet_packet_create(NULL, nSize + 1, ENET_PACKET_FLAG_RELIABLE);
            char *pPBuffer = (char*)pNetPacket->data;
            PutPacketByte(pPBuffer, myconnectindex);
            PutPacketBuffer(pPBuffer, pBuffer, nSize);
            enet_peer_send(gNetENetPeer, BLOOD_ENET_GAME, pNetPacket);
        }
        else
        {
            ENetPacket *pNetPacket = enet_packet_create(NULL, nSize + 3, ENET_PACKET_FLAG_RELIABLE);
            char *pPBuffer = (char*)pNetPacket->data;
            PutPacketByte(pPBuffer, BLOOD_SERVICE_SENDTOID);
            PutPacketByte(pPBuffer, nDest);
            PutPacketByte(pPBuffer, myconnectindex);
            PutPacketBuffer(pPBuffer, pBuffer, nSize);
            enet_peer_send(gNetENetPeer, BLOOD_ENET_SERVICE, pNetPacket);
        }
        enet_host_service(gNetENetClient, NULL, 0);
    }

    netUpdate();
#endif
}

void netSendPacketAll(char *pBuffer, int nSize)
{
    for (int p = connecthead; p >= 0; p = connectpoint2[p])
        if (p != myconnectindex)
            netSendPacket(p, pBuffer, nSize);
}

void sub_79760(void)
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
    memset(&byte_28E3B0, 0, sizeof(byte_28E3B0));
    bOutOfSync = 0;
    gBufferJitter = 1;
}

void CalcGameChecksum(void)
{
    memset(gChecksum, 0, sizeof(gChecksum));
    gChecksum[0] = wrand();
    for (int p = connecthead; p >= 0; p = connectpoint2[p])
    {
        int *pBuffer = &gPlayer[p].used1;
        int sum = 0;
        int length = ((char*)&gPlayer[p+1]-(char*)pBuffer)/4;
        while (length--)
        {
            sum += *pBuffer++;
        }
        gChecksum[1] ^= sum;
        pBuffer = (int*)gPlayer[p].pSprite;
        sum = 0;
        length = sizeof(spritetype)/4;
        while (length--)
        {
            sum += *pBuffer++;
        }
        gChecksum[2] ^= sum;
        pBuffer = (int*)gPlayer[p].pXSprite;
        sum = 0;
        length = sizeof(XSPRITE)/4;
        while (length--)
        {
            sum += *pBuffer++;
        }
        gChecksum[3] ^= sum;
    }
}

void netCheckSync(void)
{
    char buffer[80];
    if (gGameOptions.nGameType == 0)
        return;
    if (numplayers == 1)
        return;
    if (bOutOfSync)
        return;
    while (1)
    {
        for (int p = connecthead; p >= 0; p = connectpoint2[p])
        {
            if (gCheckTail >= gCheckHead[p])
                return;
        }

        for (int p = connecthead; p >= 0; p = connectpoint2[p])
        {
            if (p != myconnectindex)
            {
                int status = memcmp(gCheckFifo[gCheckTail&255][p], gCheckFifo[gCheckTail&255][connecthead], 16);
                if (status)
                {
                    sprintf(buffer, "OUT OF SYNC (%d)", p);
                    char *pBuffer = buffer + strlen(buffer);
                    for (unsigned int i = 0; i < 4; i++)
                    {
                        if (gCheckFifo[gCheckTail&255][p][i] != gCheckFifo[gCheckTail&255][connecthead][i])
                            pBuffer += sprintf(pBuffer, " %d", i);
                    }
                    viewSetErrorMessage(buffer);
                    bOutOfSync = 1;
                }
            }
        }
        gCheckTail++;
    }
}

short netGetPacket(short *pSource, char *pMessage)
{
#ifndef NETCODE_DISABLE
    if (gNetMode == NETWORK_NONE)
        return 0;
    netUpdate();
    if (gNetPacketTail != gNetPacketHead)
    {
        ENetPacket *pEPacket = gNetPacketFifo[gNetPacketTail];
        gNetPacketTail = (gNetPacketTail+1)%kENetFifoSize;
        char *pPacket = (char*)pEPacket->data;
        *pSource = GetPacketByte(pPacket);
        int nSize = pEPacket->dataLength-1;
        memcpy(pMessage, pPacket, nSize);
        enet_packet_destroy(pEPacket);
        netUpdate();
        return nSize;
    }
    netUpdate();
#endif
    return 0;
}

void netGetPackets(void)
{
    short nPlayer;
    short nSize;
    char buffer[128];
    while ((nSize = netGetPacket(&nPlayer, packet)) > 0)
    {
        char *pPacket = packet;
        switch (GetPacketByte(pPacket))
        {
        case 0: // From master
            for (int p = connecthead; p >= 0; p = connectpoint2[p])
            {
                if (p != myconnectindex)
                {
                    GINPUT *pInput = &gFifoInput[gNetFifoHead[p]&255][p];
                    memset(pInput, 0, sizeof(GINPUT));
                    pInput->syncFlags.byte = GetPacketByte(pPacket);
                    pInput->forward = GetPacketWord(pPacket);
                    pInput->q16turn = GetPacketDWord(pPacket);
                    pInput->strafe = GetPacketWord(pPacket);
                    if (pInput->syncFlags.buttonChange)
                        pInput->buttonFlags.byte = GetPacketByte(pPacket);
                    if (pInput->syncFlags.keyChange)
                        pInput->keyFlags.word = GetPacketWord(pPacket);
                    if (pInput->syncFlags.useChange)
                        pInput->useFlags.byte = GetPacketByte(pPacket);
                    if (pInput->syncFlags.weaponChange)
                        pInput->newWeapon = GetPacketByte(pPacket);
                    if (pInput->syncFlags.mlookChange)
                        pInput->q16mlook = GetPacketDWord(pPacket);
                    gNetFifoHead[p]++;
                }
                else
                {
                    SYNCFLAGS syncFlags;
                    syncFlags.byte = GetPacketByte(pPacket);
                    pPacket += 2+4+2;
                    if (syncFlags.buttonChange)
                        pPacket++;
                    if (syncFlags.keyChange)
                        pPacket+=2;
                    if (syncFlags.useChange)
                        pPacket++;
                    if (syncFlags.weaponChange)
                        pPacket++;
                    if (syncFlags.mlookChange)
                        pPacket+=4;
                }
            }
            if (((gNetFifoHead[connecthead]-1)&15)==0)
            {
                for (int p = connectpoint2[connecthead]; p >= 0; p = connectpoint2[p])
                {
                    int nLag = (signed char)GetPacketByte(pPacket);
                    if (p == myconnectindex)
                        otherMinLag = nLag;
                }
            }
            while (pPacket < packet+nSize)
            {
                int checkSum[4];
                GetPacketBuffer(pPacket, checkSum, sizeof(checkSum));
                for (int p = connecthead; p >= 0; p = connectpoint2[p])
                {
                    if (p != myconnectindex)
                    {
                        memcpy(gCheckFifo[gCheckHead[p]&255][p], checkSum, sizeof(checkSum));
                        gCheckHead[p]++;
                    }
                }
            }
            break;
        case 1: // From slave
        {
            GINPUT *pInput = &gFifoInput[gNetFifoHead[nPlayer]&255][nPlayer];
            memset(pInput, 0, sizeof(GINPUT));
            pInput->syncFlags.byte = GetPacketByte(pPacket);
            pInput->forward = GetPacketWord(pPacket);
            pInput->q16turn = GetPacketDWord(pPacket);
            pInput->strafe = GetPacketWord(pPacket);
            if (pInput->syncFlags.buttonChange)
                pInput->buttonFlags.byte = GetPacketByte(pPacket);
            if (pInput->syncFlags.keyChange)
                pInput->keyFlags.word = GetPacketWord(pPacket);
            if (pInput->syncFlags.useChange)
                pInput->useFlags.byte = GetPacketByte(pPacket);
            if (pInput->syncFlags.weaponChange)
                pInput->newWeapon = GetPacketByte(pPacket);
            if (pInput->syncFlags.mlookChange)
                pInput->q16mlook = GetPacketDWord(pPacket);
            gNetFifoHead[nPlayer]++;
            while (pPacket < packet+nSize)
            {
                int checkSum[4];
                GetPacketBuffer(pPacket, checkSum, sizeof(checkSum));
                memcpy(gCheckFifo[gCheckHead[nPlayer]&255][nPlayer], checkSum, sizeof(checkSum));
                gCheckHead[nPlayer]++;
            }
            break;
        }
        case 2:
        {
            if (nPlayer == connecthead && (gNetFifoHead[nPlayer]&15) == 0)
            {
                for (int p = connectpoint2[connecthead]; p >= 0; p = connectpoint2[p])
                {
                    int nLag = (signed char)GetPacketByte(pPacket);
                    if (p == myconnectindex)
                        otherMinLag = nLag;
                }
            }
            GINPUT *pInput = &gFifoInput[gNetFifoHead[nPlayer]&255][nPlayer];
            memset(pInput, 0, sizeof(GINPUT));
            pInput->syncFlags.byte = GetPacketByte(pPacket);
            pInput->forward = GetPacketWord(pPacket);
            pInput->q16turn = GetPacketDWord(pPacket);
            pInput->strafe = GetPacketWord(pPacket);
            if (pInput->syncFlags.buttonChange)
                pInput->buttonFlags.byte = GetPacketByte(pPacket);
            if (pInput->syncFlags.keyChange)
                pInput->keyFlags.word = GetPacketWord(pPacket);
            if (pInput->syncFlags.useChange)
                pInput->useFlags.byte = GetPacketByte(pPacket);
            if (pInput->syncFlags.weaponChange)
                pInput->newWeapon = GetPacketByte(pPacket);
            if (pInput->syncFlags.mlookChange)
                pInput->q16mlook = GetPacketDWord(pPacket);
            gNetFifoHead[nPlayer]++;
            for (int i = gSyncRate; i > 1; i--)
            {
                GINPUT *pInput2 = &gFifoInput[gNetFifoHead[nPlayer]&255][nPlayer];
                memcpy(pInput2, pInput, sizeof(GINPUT));
                pInput2->keyFlags.word = 0;
                pInput2->useFlags.byte = 0;
                pInput2->newWeapon = 0;
                pInput2->syncFlags.weaponChange = 0;
                gNetFifoHead[nPlayer]++;
            }
            while (pPacket < packet+nSize)
            {
                int checkSum[4];
                GetPacketBuffer(pPacket, checkSum, sizeof(checkSum));
                memcpy(gCheckFifo[gCheckHead[nPlayer]&255][nPlayer], checkSum, sizeof(checkSum));
                gCheckHead[nPlayer]++;
            }
            break;
        }
        case 3:
            pPacket += 4;
            if (*pPacket != '/' || (*pPacket == 0 && *(pPacket+1) == 0) || (*(pPacket+1) >= '1' && *(pPacket+1) <= '8' && *(pPacket+1)-'1' == myconnectindex))
            {
                sprintf(buffer, VanillaMode() ? "%s : %s" : "%s: %s", gProfile[nPlayer].name, pPacket);
                viewSetMessage(buffer, VanillaMode() ? 0 : 10); // 10: dark blue
                sndStartSample("DMRADIO", 128, -1);
            }
            break;
        case 4:
            sndStartSample(4400+GetPacketByte(pPacket), 128, 1, 0);
            break;
        case 7:
            nPlayer = GetPacketDWord(pPacket);
            dassert(nPlayer != myconnectindex);
            netWaitForEveryone(0);
            netPlayerQuit(nPlayer);
            netWaitForEveryone(0);
            break;
        case 249:
            nPlayer = GetPacketDWord(pPacket);
            dassert(nPlayer != myconnectindex);
            netPlayerQuit(nPlayer);
            netWaitForEveryone(0);
            break;
        case 250:
            gPlayerReady[nPlayer]++;
            break;
        case 251:
            memcpy(&gProfile[nPlayer], pPacket, sizeof(PROFILE));
            break;
        case 252:
            pPacket += 4;
            memcpy(&gPacketStartGame, pPacket, sizeof(PKT_STARTGAME));
            if (gPacketStartGame.version != word_1328AC)
                ThrowError("\nThese versions of Blood cannot play together.\n");
            gStartNewGame = 1;
            break;
        case 255:
            // What are we trying to do here? Opening the menu, maybe?
			//inputState.SetKeyStatus(sc_Escape);
            M_StartControlPanel(false);
            M_SetMenu(NAME_MainMenu);
            break;
        }
    }
}

void netBroadcastPlayerLogoff(int nPlayer)
{
    if (numplayers < 2)
        return;
    netWaitForEveryone(0);
    netPlayerQuit(nPlayer);
    if (nPlayer != myconnectindex)
        netWaitForEveryone(0);
}

void netBroadcastMyLogoff(bool bRestart)
{
    if (numplayers < 2)
        return;
    char *pPacket = packet;
    PutPacketByte(pPacket, 7);
    PutPacketDWord(pPacket, myconnectindex);
    netSendPacketAll(packet, pPacket - packet);
    netWaitForEveryone(0);
    ready2send = 0;
    gQuitGame = true;
    if (bRestart)
        gRestartGame = true;
    netDeinitialize();
    netResetToSinglePlayer();
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
    char *pPacket = packet;
    PutPacketByte(pPacket, 251);
    PutPacketBuffer(pPacket, pProfile, sizeof(PROFILE));
    netSendPacketAll(packet, pPacket-packet);
}

void netBroadcastNewGame(void)
{
    if (numplayers < 2)
        return;
    gPacketStartGame.version = word_1328AC;
    char *pPacket = packet;
    PutPacketByte(pPacket, 252);
    PutPacketDWord(pPacket, myconnectindex);
    PutPacketBuffer(pPacket, &gPacketStartGame, sizeof(PKT_STARTGAME));
    netSendPacketAll(packet, pPacket-packet);
}

void netBroadcastTaunt(int nPlayer, int nTaunt)
{
    UNREFERENCED_PARAMETER(nPlayer);
    if (numplayers > 1)
    {
        char *pPacket = packet;
        PutPacketByte(pPacket, 4);
        PutPacketByte(pPacket, nTaunt);
        netSendPacketAll(packet, pPacket-packet);
    }
    sndStartSample(4400+nTaunt, 128, 1, 0);
}

void netBroadcastMessage(int nPlayer, const char *pzMessage)
{
    if (numplayers > 1)
    {
        int nSize = strlen(pzMessage);
        char *pPacket = packet;
        PutPacketByte(pPacket, 3);
        PutPacketDWord(pPacket, nPlayer);
        PutPacketBuffer(pPacket, pzMessage, nSize+1);
        netSendPacketAll(packet, pPacket-packet);
    }
}

void netWaitForEveryone(char a1)
{
    if (numplayers < 2)
        return;
    char *pPacket = packet;
    PutPacketByte(pPacket, 250);
    netSendPacketAll(packet, pPacket-packet);
    gPlayerReady[myconnectindex]++;
    int p;
    do
    {
        if (inputState.EscapePressed() && a1)
            Bexit(0);
        gameHandleEvents();
        faketimerhandler();
        for (p = connecthead; p >= 0; p = connectpoint2[p])
            if (gPlayerReady[p] < gPlayerReady[myconnectindex])
                break;
        if (gRestartGame || gNetPlayers <= 1)
            break;
    } while (p >= 0);
}

void sub_7AC28(const char *pzString)
{
    if (numplayers < 2)
        return;
    if (pzString)
    {
        int nLength = strlen(pzString);
        if (nLength > 0)
        {
            char *pPacket = packet;
            PutPacketByte(pPacket, 5);
            PutPacketBuffer(pPacket, pzString, nLength+1);
            netSendPacketAll(packet, pPacket-packet);
        }
    }
}

void netSendEmptyPackets(void)
{
    ClockTicks nClock = totalclock;
    char *pPacket = packet;
    PutPacketByte(pPacket, 254);
    for (int i = 0; i < 8; i++)
    {
        if (nClock <= totalclock)
        {
            nClock = totalclock+4;
            netSendPacketAll(packet, pPacket-packet);
        }
    }
}

void sub_7AD90(GINPUT *pInput)
{
    byte_28E3B0.at0 |= pInput->syncFlags.byte;
    byte_28E3B0.at4 += pInput->forward;
    byte_28E3B0.at8 += pInput->q16turn;
    byte_28E3B0.atc += pInput->strafe;
    byte_28E3B0.at10 |= pInput->buttonFlags.byte;
    byte_28E3B0.at14 |= pInput->keyFlags.word;
    byte_28E3B0.at18 |= pInput->useFlags.byte;
    if (pInput->newWeapon)
        byte_28E3B0.at1c = pInput->newWeapon;
    byte_28E3B0.at1d = pInput->q16mlook;
}

void sub_7AE2C(GINPUT *pInput)
{
    pInput->syncFlags.byte = byte_28E3B0.at0;
    pInput->forward = byte_28E3B0.at4;
    pInput->q16turn = byte_28E3B0.at8;
    pInput->strafe = byte_28E3B0.atc;
    pInput->buttonFlags.byte = byte_28E3B0.at10;
    pInput->keyFlags.word = byte_28E3B0.at14;
    pInput->useFlags.byte = byte_28E3B0.at18;
    pInput->newWeapon = byte_28E3B0.at1c;
    pInput->q16mlook = byte_28E3B0.at1d;
    memset(&byte_28E3B0, 0, sizeof(byte_28E3B0));
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
    ctrlGetInput();
    sub_7AD90(&gInput);
    if (gNetFifoHead[myconnectindex]&(gSyncRate-1))
    {
        GINPUT *pInput1 = &gFifoInput[gNetFifoHead[myconnectindex]&255][myconnectindex];
        GINPUT *pInput2 = &gFifoInput[(gNetFifoHead[myconnectindex]-1)&255][myconnectindex];
        memcpy(pInput1, pInput2, sizeof(GINPUT));
        pInput1->keyFlags.word = 0;
        pInput1->useFlags.byte = 0;
        pInput1->newWeapon = 0;
        pInput1->syncFlags.weaponChange = 0;
        gNetFifoHead[myconnectindex]++;
        return;
    }
    GINPUT *pInput = &gFifoInput[gNetFifoHead[myconnectindex]&255][myconnectindex];
    sub_7AE2C(pInput);
    memcpy(&gInput, pInput, sizeof(GINPUT));
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
    if (gPacketMode == PACKETMODE_2)
    {
        char *pPacket = packet;
        PutPacketByte(pPacket, 2);
        if (((gNetFifoHead[myconnectindex]-1)&15) == 0)
        {
            if (myconnectindex == connecthead)
            {
                for (int p = connectpoint2[connecthead]; p >= 0; p = connectpoint2[p])
                    PutPacketByte(pPacket, ClipRange(myMinLag[p], -128, 127));
            }
            else
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
            }
            for (int p = connecthead; p >= 0; p = connectpoint2[p])
                myMinLag[p] = 0x7fffffff;
        }
        if (gInput.buttonFlags.byte)
            gInput.syncFlags.buttonChange = 1;
        if (gInput.keyFlags.word)
            gInput.syncFlags.keyChange = 1;
        if (gInput.useFlags.byte)
            gInput.syncFlags.useChange = 1;
        if (gInput.newWeapon)
            gInput.syncFlags.weaponChange = 1;
        if (gInput.q16mlook)
            gInput.syncFlags.mlookChange = 1;
        PutPacketByte(pPacket, gInput.syncFlags.byte);
        PutPacketWord(pPacket, gInput.forward);
        PutPacketDWord(pPacket, gInput.q16turn);
        PutPacketWord(pPacket, gInput.strafe);
        if (gInput.syncFlags.buttonChange)
            PutPacketByte(pPacket, gInput.buttonFlags.byte);
        if (gInput.syncFlags.keyChange)
            PutPacketWord(pPacket, gInput.keyFlags.word);
        if (gInput.syncFlags.useChange)
            PutPacketByte(pPacket, gInput.useFlags.byte);
        if (gInput.syncFlags.weaponChange)
            PutPacketByte(pPacket, gInput.newWeapon);
        if (gInput.syncFlags.mlookChange)
            PutPacketDWord(pPacket, gInput.q16mlook);
        while (gSendCheckTail != gCheckHead[myconnectindex])
        {
            unsigned int *checkSum = gCheckFifo[gSendCheckTail&255][myconnectindex];
            PutPacketBuffer(pPacket, checkSum, 16);
            gSendCheckTail++;
        }
        netSendPacketAll(packet, pPacket-packet);
        return;
    }
    if (myconnectindex != connecthead)
    {
        char *pPacket = packet;
        PutPacketByte(pPacket, 1);
        if (gInput.buttonFlags.byte)
            gInput.syncFlags.buttonChange = 1;
        if (gInput.keyFlags.word)
            gInput.syncFlags.keyChange = 1;
        if (gInput.useFlags.byte)
            gInput.syncFlags.useChange = 1;
        if (gInput.newWeapon)
            gInput.syncFlags.weaponChange = 1;
        if (gInput.q16mlook)
            gInput.syncFlags.mlookChange = 1;
        PutPacketByte(pPacket, gInput.syncFlags.byte);
        PutPacketWord(pPacket, gInput.forward);
        PutPacketDWord(pPacket, gInput.q16turn);
        PutPacketWord(pPacket, gInput.strafe);
        if (gInput.syncFlags.buttonChange)
            PutPacketByte(pPacket, gInput.buttonFlags.byte);
        if (gInput.syncFlags.keyChange)
            PutPacketWord(pPacket, gInput.keyFlags.word);
        if (gInput.syncFlags.useChange)
            PutPacketByte(pPacket, gInput.useFlags.byte);
        if (gInput.syncFlags.weaponChange)
            PutPacketByte(pPacket, gInput.newWeapon);
        if (gInput.syncFlags.mlookChange)
            PutPacketDWord(pPacket, gInput.q16mlook);
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
    netDeinitialize();
    memset(gPlayerReady, 0, sizeof(gPlayerReady));
    sub_79760();
#ifndef NETCODE_DISABLE
    char buffer[128];
    gNetENetServer = gNetENetClient = NULL;
    //gNetServerPacketHead = gNetServerPacketTail = 0;
    gNetPacketHead = gNetPacketTail = 0;
    if (gNetMode == NETWORK_NONE)
    {
        netResetToSinglePlayer();
        return;
    }
    if (!gHaveNetworking)
    {
        initprintf("An error occurred while initializing ENet.\n");
        netResetToSinglePlayer();
        return;
    }
    if (gNetMode == NETWORK_SERVER)
    {
        memset(gNetPlayerPeer, 0, sizeof(gNetPlayerPeer));
        ENetEvent event;
        gNetENetAddress.host = ENET_HOST_ANY;
        gNetENetAddress.port = gNetPort;
        gNetENetServer = enet_host_create(&gNetENetAddress, 8, BLOOD_ENET_CHANNEL_MAX, 0, 0);
        if (!gNetENetServer)
        {
            initprintf("An error occurred while trying to create an ENet server host.\n");
            netDeinitialize();
            netResetToSinglePlayer();
            return;
        }

        numplayers = 1;
        // Wait for clients
        if (!bConsole)
        {
            char buffer[128];
            sprintf(buffer, "Waiting for players (%i\\%i)", numplayers, gNetPlayers);
            viewLoadingScreen(2518, "Network Game", NULL, buffer);
            videoNextPage();
        }
        while (numplayers < gNetPlayers)
        {
            handleevents();

            if (!bConsole && inputState.GetKeyStatus(sc_Escape))
            {
                netServerDisconnect();
                netDeinitialize();
                netResetToSinglePlayer();
                return;
            }
            enet_host_service(gNetENetServer, NULL, 0);
            if (enet_host_check_events(gNetENetServer, &event) > 0)
            {
                switch (event.type)
                {
                case ENET_EVENT_TYPE_CONNECT:
                {
                    char ipaddr[32];

                    enet_address_get_host_ip(&event.peer->address, ipaddr, sizeof(ipaddr));
                    initprintf("Client connected: %s:%u\n", ipaddr, event.peer->address.port);
                    numplayers++;
                    for (int i = 1; i < kMaxPlayers; i++)
                    {
                        if (gNetPlayerPeer[i] == NULL)
                        {
                            gNetPlayerPeer[i] = event.peer;
                            break;
                        }
                    }
                    if (!bConsole)
                    {
                        char buffer[128];
                        sprintf(buffer, "Waiting for players (%i\\%i)", numplayers, gNetPlayers);
                        viewLoadingScreen(2518, "Network Game", NULL, buffer);
                        videoNextPage();
                    }
                    break;
                }
                case ENET_EVENT_TYPE_DISCONNECT:
                {
                    char ipaddr[32];

                    enet_address_get_host_ip(&event.peer->address, ipaddr, sizeof(ipaddr));
                    initprintf("Client disconnected: %s:%u\n", ipaddr, event.peer->address.port);
                    numplayers--;
                    for (int i = 1; i < kMaxPlayers; i++)
                    {
                        if (gNetPlayerPeer[i] == event.peer)
                        {
                            gNetPlayerPeer[i] = NULL;
                            for (int j = kMaxPlayers-1; j > i; j--)
                            {
                                if (gNetPlayerPeer[j])
                                {
                                    gNetPlayerPeer[i] = gNetPlayerPeer[j];
                                    gNetPlayerPeer[j] = NULL;
                                    break;
                                }
                            }
                        }
                    }
                    if (!bConsole)
                    {
                        char buffer[128];
                        sprintf(buffer, "Waiting for players (%i\\%i)", numplayers, gNetPlayers);
                        viewLoadingScreen(2518, "Network Game", NULL, buffer);
                        videoNextPage();
                    }
                    break;
                }
                default:
                    break;
                }
            }
            enet_host_service(gNetENetServer, NULL, 0);
        }
        initprintf("All clients connected\n");

        dassert(numplayers >= 1);

        gInitialNetPlayers = gNetPlayers = numplayers;
        connecthead = 0;
        for (int i = 0; i < numplayers-1; i++)
            connectpoint2[i] = i+1;
        connectpoint2[numplayers-1] = -1;

        enet_host_service(gNetENetServer, NULL, 0);

        // Send connect info
        char *pPacket = packet;
        PutPacketByte(pPacket, BLOOD_SERVICE_CONNECTINFO);
        PKT_CONNECTINFO connectinfo;
        connectinfo.numplayers = numplayers;
        connectinfo.connecthead = connecthead;
        for (int i = 0; i < numplayers; i++)
            connectinfo.connectpoint2[i] = connectpoint2[i];
        PutPacketBuffer(pPacket, &connectinfo, sizeof(connectinfo));
        ENetPacket *pEPacket = enet_packet_create(packet, pPacket-packet, ENET_PACKET_FLAG_RELIABLE);
        enet_host_broadcast(gNetENetServer, BLOOD_ENET_SERVICE, pEPacket);
        //enet_packet_destroy(pEPacket);
        
        enet_host_service(gNetENetServer, NULL, 0);

        // Send id
        myconnectindex = 0;
        for (int i = 1; i < numplayers; i++)
        {
            if (!gNetPlayerPeer[i])
                continue;
            char *pPacket = packet;
            PutPacketByte(pPacket, BLOOD_SERVICE_CONNECTID);
            PutPacketByte(pPacket, i);
            ENetPacket *pEPacket = enet_packet_create(packet, pPacket-packet, ENET_PACKET_FLAG_RELIABLE);
            enet_peer_send(gNetPlayerPeer[i], BLOOD_ENET_SERVICE, pEPacket);

            enet_host_service(gNetENetServer, NULL, 0);
        }
        
        enet_host_service(gNetENetServer, NULL, 0);
    }
    else if (gNetMode == NETWORK_CLIENT)
    {
        ENetEvent event;
        sprintf(buffer, "Connecting to %s:%u", gNetAddress, gNetPort);
        initprintf("%s\n", buffer);
        if (!bConsole)
        {
            viewLoadingScreen(2518, "Network Game", NULL, buffer);
            videoNextPage();
        }
        gNetENetClient = enet_host_create(NULL, 1, BLOOD_ENET_CHANNEL_MAX, 0, 0);
        enet_address_set_host(&gNetENetAddress, gNetAddress);
        gNetENetAddress.port = gNetPort;
        gNetENetPeer = enet_host_connect(gNetENetClient, &gNetENetAddress, BLOOD_ENET_CHANNEL_MAX, 0);
        if (!gNetENetPeer)
        {
            initprintf("No available peers for initiating an ENet connection.\n");
            netDeinitialize();
            netResetToSinglePlayer();
            return;
        }
        if (enet_host_service(gNetENetClient, &event, 5000) > 0 && event.type == ENET_EVENT_TYPE_CONNECT)
            initprintf("Connected to %s:%i\n", gNetAddress, gNetPort);
        else
        {
            initprintf("Could not connect to %s:%i\n", gNetAddress, gNetPort);
            netDeinitialize();
            return;
        }
        bool bWaitServer = true;
        if (!bConsole)
        {
            viewLoadingScreen(2518, "Network Game", NULL, "Waiting for server response");
            videoNextPage();
        }
        while (bWaitServer)
        {
            handleevents();

            if (!bConsole && inputState.GetKeyStatus(sc_Escape))
            {
                netClientDisconnect();
                netDeinitialize();
                netResetToSinglePlayer();
                return;
            }
            enet_host_service(gNetENetClient, NULL, 0);
            if (enet_host_check_events(gNetENetClient, &event) > 0)
            {
                switch (event.type)
                {
                case ENET_EVENT_TYPE_DISCONNECT:
                    initprintf("Lost connection to server\n");
                    netDeinitialize();
                    netResetToSinglePlayer();
                    return;
                case ENET_EVENT_TYPE_RECEIVE:
                    //initprintf("NETEVENT\n");
                    if (event.channelID == BLOOD_ENET_SERVICE)
                    {
                        char *pPacket = (char*)event.packet->data;
                        switch (GetPacketByte(pPacket))
                        {
                        case BLOOD_SERVICE_CONNECTINFO:
                        {
                            PKT_CONNECTINFO *connectinfo = (PKT_CONNECTINFO*)pPacket;
                            gInitialNetPlayers = gNetPlayers = numplayers = connectinfo->numplayers;
                            connecthead = connectinfo->connecthead;
                            for (int i = 0; i < numplayers; i++)
                                connectpoint2[i] = connectinfo->connectpoint2[i];
                            //initprintf("BLOOD_SERVICE_CONNECTINFO\n");
                            break;
                        }
                        case BLOOD_SERVICE_CONNECTID:
                            dassert(numplayers > 1);
                            myconnectindex = GetPacketByte(pPacket);
                            bWaitServer = false;
                            //initprintf("BLOOD_SERVICE_CONNECTID\n");
                            break;
                        }
                    }
                    enet_packet_destroy(event.packet);
                    break;
                default:
                    break;
                }
            }
        }
        enet_host_service(gNetENetClient, NULL, 0);
        initprintf("Successfully connected to server\n");
    }
    gNetENetInit = true;
    gGameOptions.nGameType = 2;
#endif
}

void netDeinitialize(void)
{
#ifndef NETCODE_DISABLE
    if (!gNetENetInit)
        return;
    gNetENetInit = false;
    if (gNetMode != NETWORK_NONE)
    {
        if (gNetENetServer)
        {
            netServerDisconnect();
            enet_host_destroy(gNetENetServer);
        }
        else if (gNetENetClient)
        {
            netClientDisconnect();
            enet_host_destroy(gNetENetClient);
        }
        enet_deinitialize();
    }
    gNetENetServer = gNetENetClient = NULL;
#endif
}

#ifndef NETCODE_DISABLE
void netPostEPacket(ENetPacket *pEPacket)
{
    //static int number;
    //initprintf("netPostEPacket %i\n", number++);
    gNetPacketFifo[gNetPacketHead] = pEPacket;
    gNetPacketHead = (gNetPacketHead+1)%kENetFifoSize;
}
#endif

void netUpdate(void)
{
#ifndef NETCODE_DISABLE
    ENetEvent event;
    if (gNetMode == NETWORK_NONE)
        return;

    if (gNetENetServer)
    {
        enet_host_service(gNetENetServer, NULL, 0);
        while (enet_host_check_events(gNetENetServer, &event) > 0)
        {
            switch (event.type)
            {
            case ENET_EVENT_TYPE_DISCONNECT:
            {
                int nPlayer;
                for (nPlayer = connectpoint2[connecthead]; nPlayer >= 0; nPlayer = connectpoint2[nPlayer])
                    if (gNetPlayerPeer[nPlayer] == event.peer)
                        break;

                for (int p = 0; p < kMaxPlayers; p++)
                    if (gNetPlayerPeer[p] == event.peer)
                        gNetPlayerPeer[p] = NULL;
                if (nPlayer >= 0)
                {
                    // TODO: Game most likely will go out of sync here...
                    char *pPacket = packet;
                    PutPacketByte(pPacket, 249);
                    PutPacketDWord(pPacket, nPlayer);
                    netSendPacketAll(packet, pPacket - packet);
                    netPlayerQuit(nPlayer);
                    netWaitForEveryone(0);
                }
                if (gNetPlayers <= 1)
                {
                    netDeinitialize();
                    netResetToSinglePlayer();
                    return;
                }
                break;
            }
            case ENET_EVENT_TYPE_RECEIVE:
                switch (event.channelID)
                {
                case BLOOD_ENET_SERVICE:
                {
                    char *pPacket = (char*)event.packet->data;
                    if (GetPacketByte(pPacket) != BLOOD_SERVICE_SENDTOID)
                        ThrowError("Packet error\n");
                    int nDest = GetPacketByte(pPacket);
                    int nSource = GetPacketByte(pPacket);
                    int nSize = event.packet->dataLength-3;
                    if (gNetPlayerPeer[nDest] != NULL)
                    {
                        ENetPacket *pNetPacket = enet_packet_create(NULL, nSize + 1, ENET_PACKET_FLAG_RELIABLE);
                        char *pPBuffer = (char*)pNetPacket->data;
                        PutPacketByte(pPBuffer, nSource);
                        PutPacketBuffer(pPBuffer, pPacket, nSize);
                        enet_peer_send(gNetPlayerPeer[nDest], BLOOD_ENET_GAME, pNetPacket);
                        enet_host_service(gNetENetServer, NULL, 0);
                    }
                    enet_packet_destroy(event.packet);
                    break;
                }
                case BLOOD_ENET_GAME:
                    netPostEPacket(event.packet);
                    break;
                }
            default:
                break;
            }
            enet_host_service(gNetENetServer, NULL, 0);
        }
        enet_host_service(gNetENetServer, NULL, 0);
    }
    else if (gNetENetClient)
    {
        enet_host_service(gNetENetClient, NULL, 0);
        while (enet_host_check_events(gNetENetClient, &event) > 0)
        {
            switch (event.type)
            {
            case ENET_EVENT_TYPE_DISCONNECT:
                initprintf("Lost connection to server\n");
                netDeinitialize();
                netResetToSinglePlayer();
                gQuitGame = gRestartGame = true;
                return;
            case ENET_EVENT_TYPE_RECEIVE:
                switch (event.channelID)
                {
                case BLOOD_ENET_SERVICE:
                {
                    ThrowError("Packet error\n");
                    break;
                }
                case BLOOD_ENET_GAME:
                    netPostEPacket(event.packet);
                    break;
                }
            default:
                break;
            }
            enet_host_service(gNetENetClient, NULL, 0);
        }
        enet_host_service(gNetENetClient, NULL, 0);
    }
#endif
}

void faketimerhandler(void)
{
#ifndef NETCODE_DISABLE
    if (gNetMode != NETWORK_NONE && gNetENetInit)
        netUpdate();
#endif
    //if (gNetMode != NETWORK_NONE && gNetENetInit)
    //    enet_host_service(gNetMode == NETWORK_SERVER ? gNetENetServer : gNetENetClient, NULL, 0);
}

void GameInterface::faketimerhandler()
{
	::Blood::faketimerhandler();
}

void netPlayerQuit(int nPlayer)
{
    char buffer[128];
    sprintf(buffer, "%s left the game with %d frags.", gProfile[nPlayer].name, gPlayer[nPlayer].fragCount);
    viewSetMessage(buffer);
    if (gGameStarted)
    {
        seqKill(3, gPlayer[nPlayer].pSprite->extra);
        actPostSprite(gPlayer[nPlayer].nSprite, kStatFree);
        if (nPlayer == gViewIndex)
            gViewIndex = myconnectindex;
        gView = &gPlayer[gViewIndex];
    }
    if (nPlayer == connecthead)
    {
        connecthead = connectpoint2[connecthead];
        //if (gPacketMode == PACKETMODE_1)
        {
            //byte_27B2CC = 1;
            gQuitGame = true;
            gRestartGame = true;
            gNetPlayers = 1;
            //gQuitRequest = 1;
        }
    }
    else
    {
        for (int p = connecthead; p >= 0; p = connectpoint2[p])
        {
            if (connectpoint2[p] == nPlayer)
                connectpoint2[p] = connectpoint2[nPlayer];
        }
#ifndef NETCODE_DISABLE
        gNetPlayerPeer[nPlayer] = NULL;
#endif
    }
    gNetPlayers--;
    numplayers = ClipLow(numplayers-1, 1);
    if (gNetPlayers <= 1)
    {
        netDeinitialize();
        netResetToSinglePlayer();
    }
}

END_BLD_NS
