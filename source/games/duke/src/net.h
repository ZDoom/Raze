//-------------------------------------------------------------------------
/*
Copyright (C) 2010 EDuke32 developers and contributors

This file is part of EDuke32.

EDuke32 is free software; you can redistribute it and/or
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

#ifndef netplay_h_
#define netplay_h_

struct ENetHost;
struct ENetPeer;
struct ENetEvent;
struct ENetPacket;

BEGIN_DUKE_NS

extern void       *g_netServer;

#define MAXSYNCBYTES 16
#define SYNCFIFOSIZ 1024

// TENSW: on really bad network connections, the sync FIFO queue can overflow if it is the
// same size as the move fifo.
#if MOVEFIFOSIZ >= SYNCFIFOSIZ
#error "MOVEFIFOSIZ is greater than or equal to SYNCFIFOSIZ!"
#endif

extern char syncstat[MAXSYNCBYTES];
extern char g_szfirstSyncMsg[MAXSYNCBYTES][60];

extern int g_numSyncBytes;
extern int g_foundSyncError;
extern int syncvaltail, syncvaltottail;

void    Net_ClearFIFO(void);
void    Net_GetInput(void);
void    Net_GetPackets(void);
/* NETCODE_ENABLE is not defined */

// Connect/Disconnect
#define Net_Connect(...) ((void)0)
#define Net_Disconnect(...) ((void)0)
#define Net_ReceiveDisconnect(...) ((void)0)

// Packet Handlers
#define Net_HandleServerPackets(...) ((void)0)
#define Net_HandleClientPackets(...) ((void)0)
#define Net_ParseClientPacket(...) ((void)0)
#define Net_ParseServerPacket(...) ((void)0)
#define Net_ParsePacketCommon(...) ((void)0)

#define Net_SendAcknowledge(...) ((void)0)
#define Net_ReceiveAcknowledge(...) ((void)0)

#define Net_SendChallenge(...) ((void)0)
#define Net_ReceiveChallenge(...) ((void)0)

#define Net_SendNewPlayer(...) ((void)0)
#define Net_ReceiveNewPlayer(...) ((void)0)

#define Net_SendPlayerIndex(...) ((void)0)
#define Net_ReceivePlayerIndex(...) ((void)0)

#define Net_SendClientInfo(...) ((void)0)
#define Net_ReceiveClientInfo(...) ((void)0)

#define Net_SendUserMapName(...) ((void)0)
#define Net_ReceiveUserMapName(...) ((void)0)

#define Net_SendClientSync(...) ((void)0)
#define Net_ReceiveClientSync(...) ((void)0)

#define Net_SendMapUpdate(...) ((void)0)
#define Net_ReceiveMapUpdate(...) ((void)0)

#define Net_FillPlayerUpdate(...) ((void)0)
#define Net_ExtractPlayerUpdate(...) ((void)0)

#define Net_SendServerUpdates(...) ((void)0)
#define Net_ReceiveServerUpdate(...) ((void)0)

#define Net_SendClientUpdate(...) ((void)0)
#define Net_ReceiveClientUpdate(...) ((void)0)

#define Net_SendMessage(...) ((void)0)
#define Net_ReceiveMessage(...) ((void)0)

#define Net_StartNewGame(...) ((void)0)
#define Net_SendNewGame(...) ((void)0)
#define Net_ReceiveNewGame(...) ((void)0)

#define Net_FillNewGame(...) ((void)0)
#define Net_ExtractNewGame(...) ((void)0)

#define Net_SendMapVoteInitiate(...) ((void)0)
#define Net_ReceiveMapVoteInitiate(...) ((void)0)

#define Net_SendMapVote(...) ((void)0)
#define Net_ReceiveMapVote(...) ((void)0)
#define Net_CheckForEnoughVotes(...) ((void)0)

#define Net_SendMapVoteCancel(...) ((void)0)
#define Net_ReceiveMapVoteCancel(...) ((void)0)

//////////

#define Net_ResetPrediction(...) ((void)0)
#define Net_DoPrediction(...) ((void)0)
#define Net_CorrectPrediction(...) ((void)0)
#define Net_RestoreMapState(...) ((void)0)
#define Net_SyncPlayer(...) ((void)0)
#define Net_WaitForServer(...) ((void)0)

#define Net_ActorsAreDifferent(...) 0
#define Net_IsRelevantSprite(...) 0
#define Net_IsRelevantStat(...) 0
#define Net_InsertSprite(...) 0
#define Net_DeleteSprite(...) ((void)0)

#define Net_NotifyNewGame(...) ((void)0)

#define Net_SendTaunt(...) ((void)0)
#define Net_SendRTS(...) ((void)0)

#define Net_WaitForEverybody(...) ((void)0)
#define initsynccrc(...) ((void)0)
#define Net_GetSyncStat(...) ((void)0)
#define Net_DisplaySyncMsg(...) ((void)0)
#define Net_ClearFIFO(...) ((void)0)
#define Net_GetInput(...) ((void)0)

END_DUKE_NS

#endif // netplay_h_
