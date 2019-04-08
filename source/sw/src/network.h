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

#define PACKET_TYPE_MASTER_TO_SLAVE                 0
#define PACKET_TYPE_SLAVE_TO_MASTER                 1
//#define PACKET_TYPE_PLAYER_NAME                     3
#define PACKET_TYPE_MESSAGE                         4
//#define PACKET_TYPE_GAME_INFO                       8
#define PACKET_TYPE_BROADCAST                       17
#define SERVER_GENERATED_BROADCAST                  18
#define PACKET_TYPE_PROXY                           19

#define PACKET_TYPE_NEW_GAME                        30
//#define PACKET_TYPE_NEW_LEVEL                       31
#define PACKET_TYPE_PLAYER_OPTIONS                  32
#define PACKET_TYPE_RTS                             33
#define PACKET_TYPE_DUMMY                           34
#define PACKET_TYPE_MENU_LEVEL_QUIT                 35
#define PACKET_TYPE_NAME_CHANGE                     36
#define PACKET_TYPE_VERSION                         38

#define PACKET_TYPE_NULL_PACKET                     127
#define PACKET_TYPE_PLAYER_READY                    250
#define PACKET_TYPE_DONT_USE                        255  // old logoff

#define BIT_CODEC TRUE
#define SYNC_TEST TRUE
#define MAXSYNCBYTES 16

#ifdef __GNUC__
# define PACKED __attribute__ ((packed))
#else
# define PACKED
# ifdef _MSC_VER
#  pragma pack(1)
# endif
# ifdef __WATCOMC__
#  pragma pack(push,1);
# endif
#endif

// Slave->Master: PlayerIndex = who to send the packet to (-1 = all)
// Master->Slave: PlayerIndex = who sent the packet originally
typedef struct PACKED
{
    uint8_t PacketType;  // first byte is always packet type
    uint8_t PlayerIndex;
} PACKET_PROXY,*PACKET_PROXYp;

typedef struct PACKED
{
    uint8_t PacketType;  // first byte is always packet type
    uint8_t FirstPlayerIndex;
    SWBOOL AutoAim;
    uint8_t Level;
    uint8_t Episode;
    int8_t Skill;
    uint8_t GameType;
    SWBOOL HurtTeammate;
    SWBOOL SpawnMarkers;
    SWBOOL TeamPlay;
    uint8_t KillLimit;
    uint8_t TimeLimit;
    SWBOOL Nuke;
} PACKET_NEW_GAME,*PACKET_NEW_GAMEp;

typedef struct PACKED
{
    uint8_t PacketType;  // first byte is always packet type
    SWBOOL AutoRun;
    uint8_t Color;
    char PlayerName[32];
} PACKET_OPTIONS,*PACKET_OPTIONSp;

typedef struct PACKED
{
    uint8_t PacketType;  // first byte is always packet type
    char PlayerName[32];
} PACKET_NAME_CHANGE,*PACKET_NAME_CHANGEp;

typedef struct PACKED
{
    uint8_t PacketType;  // first byte is always packet type
    uint8_t RTSnum;
} PACKET_RTS,*PACKET_RTSp;

typedef struct PACKED
{
    uint8_t PacketType;  // first byte is always packet type
    int Version;
} PACKET_VERSION,*PACKET_VERSIONp;

#undef PACKED
#ifdef _MSC_VER
# pragma pack()
#endif
#ifdef __WATCOMC__
# pragma pack(pop);
#endif

extern uint8_t syncstat[MAXSYNCBYTES];
extern SWBOOL PredictionOn;
extern PLAYER PredictPlayer;
extern PLAYERp ppp;
extern short predictangpos[MOVEFIFOSIZ];
extern int predictmovefifoplc;
extern SWBOOL Prediction;
extern short NumSyncBytes;

void InitPrediction(PLAYERp pp);
void DoPrediction(PLAYERp ppp);
void CorrectPrediction(int actualfifoplc);

//TENSW: safe packet senders
void netsendpacket(int ind, uint8_t* buf, int len);
void netbroadcastpacket(uint8_t* buf, int len);
int netgetpacket(int *ind, uint8_t* buf);


enum MultiGameTypes
{
    MULTI_GAME_NONE,
    MULTI_GAME_COMMBAT,
    MULTI_GAME_COMMBAT_NO_RESPAWN, // JUST a place holder for menus. DO NOT USE!!!
    MULTI_GAME_COOPERATIVE,
    MULTI_GAME_AI_BOTS
};

//extern int16_t MultiGameType;    // defaults to NONE

// global net vars
// not saved in .CFG file
// used for current game
typedef struct
{
    int32_t KillLimit;
    int32_t TimeLimit;
    int32_t TimeLimitClock;
    int16_t MultiGameType; // used to be a stand alone global
    SWBOOL TeamPlay;
    SWBOOL HurtTeammate;
    SWBOOL SpawnMarkers;
    SWBOOL AutoAim;
    SWBOOL NoRespawn; // for commbat type games
    SWBOOL Nuke;
} gNET,*gNETp;

extern gNET gNet;

typedef struct
{
    int Rules,
        Level,
        Enemy,
        Markers,
        Team,
        HurtTeam,
        Kill,
        Time,
        Color,
        Nuke;
} AUTO_NET, *AUTO_NETp;

extern AUTO_NET Auto;
extern SWBOOL AutoNet;

void getpackets(void);
void SendMulitNameChange(char *new_name);
void InitNetVars(void);
void InitTimingVars(void);
void PauseAction(void);
void ResumeAction(void);
void ErrorCorrectionQuit(void);
void Connect(void);
void waitforeverybody(void);
SWBOOL MenuCommPlayerQuit(short quit_player);
void SendVersion(int version);
void InitNetPlayerOptions(void);
void CheckVersion(int GameVersion);
void SW_SendMessage(short pnum,const char *text);
void PauseGame(void);
void ResumeGame(void);

