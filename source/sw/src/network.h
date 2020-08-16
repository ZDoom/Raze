#pragma once
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
BEGIN_SW_NS

#define SYNC_TEST 0
#define MAXSYNCBYTES 16

#pragma pack(push,1)

typedef struct
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

#pragma pack(pop)

extern uint8_t syncstat[MAXSYNCBYTES];
extern SWBOOL PredictionOn;
extern PLAYER PredictPlayer;
extern PLAYERp ppp;
extern short predictangpos[MOVEFIFOSIZ];
extern int predictmovefifoplc;
extern SWBOOL Prediction;

void InitPrediction(PLAYERp pp);
void DoPrediction(PLAYERp ppp);
void CorrectPrediction(int actualfifoplc);


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

void UpdateInputs(void);
void InitNetVars(void);
void InitTimingVars(void);
void waitforeverybody(void);
void InitNetPlayerOptions(void);
inline void SW_SendMessage(short, const char*) {}
void ResumeGame(void);

END_SW_NS
