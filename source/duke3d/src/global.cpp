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

#define global_c_
#include "global.h"
#include "duke3d.h"

user_defs ud;

const char *s_buildDate = "20120522";

#ifdef __cplusplus
extern "C" {
#endif

#ifndef EDUKE32_STANDALONE
char    g_volumeNames[MAXVOLUMES][33] = { "L.A. Meltdown", "Lunar Apocalypse", "Shrapnel City" };
char    g_skillNames[MAXSKILLS][33] = { "Piece Of Cake", "Let's Rock", "Come Get Some", "Damn I'm Good" };
char    g_gametypeNames[MAXGAMETYPES][33]
= { "DukeMatch (Spawn)", "Cooperative Play", "DukeMatch (No Spawn)", "Team DM (Spawn)", "Team DM (No Spawn)" };
#else
char    g_volumeNames[MAXVOLUMES][33];
char    g_skillNames[MAXSKILLS][33];
char    g_gametypeNames[MAXGAMETYPES][33]
= { "Deathmatch (Spawn)", "Cooperative Play", "Deathmatch (No Spawn)", "Team DM (Spawn)", "Team DM (No Spawn)" };
#endif

int32_t g_volumeFlags[MAXVOLUMES];

int32_t g_gametypeFlags[MAXGAMETYPES] =
{
    /*4+*/8+16+1024+2048+16384,
    1+2+32+64+128+256+512+4096+8192+32768,
    2+/*4+*/8+16+16384,
    /*4+*/8+16+1024+2048+16384+65536+131072,
    2+/*4+*/8+16+16384+65536+131072
};

int g_actorRespawnTime   = 768;
int g_bouncemineRadius   = 2500;
int g_deleteQueueSize    = 64;
int g_itemRespawnTime    = 768;
int g_lastSaveSlot       = -1;
int g_morterRadius       = 2500;
int g_numFreezeBounces   = 3;
int g_gametypeCnt       = 5;
int g_volumeCnt         = 3;
int g_pipebombRadius     = 2500;
int g_playerFriction     = 0xCFD0;
int g_rpgRadius          = 1780;
int g_scriptSize         = 1048576;
int g_seenineRadius      = 2048;
int g_shrinkerRadius     = 650;
int g_spriteGravity      = 176;
int g_timerTicsPerSecond = TICRATE;
int g_tripbombRadius     = 3880;

int16_t g_blimpSpawnItems[15] =
{
    RPGSPRITE__STATIC,
    CHAINGUNSPRITE__STATIC,
    DEVISTATORAMMO__STATIC,
    RPGAMMO__STATIC,
    RPGAMMO__STATIC,
    JETPACK__STATIC,
    SHIELD__STATIC,
    FIRSTAID__STATIC,
    STEROIDS__STATIC,
    RPGAMMO__STATIC,
    RPGAMMO__STATIC,
    RPGSPRITE__STATIC,
    RPGAMMO__STATIC,
    FREEZESPRITE__STATIC,
    FREEZEAMMO__STATIC
};

char CheatKeys[2]       = { sc_D, sc_N };

char g_setupFileName[BMAX_PATH] = SETUPFILENAME;

#ifdef __cplusplus
}
#endif

