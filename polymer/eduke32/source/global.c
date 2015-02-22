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

// JBF: g_spriteGravity modified to default to Atomic ed. default when using 1.3d CONs
int32_t g_spriteGravity=176;

int16_t g_spriteDeleteQueueSize = 64;

char EpisodeNames[MAXVOLUMES][33] = { "L.A. Meltdown", "Lunar Apocalypse", "Shrapnel City" };
int32_t EpisodeFlags[MAXVOLUMES]; // all initialize to zero

char SkillNames[MAXSKILLS][33] = { "Piece Of Cake", "Let's Rock", "Come Get Some", "Damn I'm Good" };

char GametypeNames[MAXGAMETYPES][33] = { "DukeMatch (Spawn)", "Cooperative Play", "DukeMatch (No Spawn)", "Team DM (Spawn)", "Team DM (No Spawn)" };

int32_t GametypeFlags[MAXGAMETYPES] =
{
    /*4+*/8+16+1024+2048+16384,
    1+2+32+64+128+256+512+4096+8192+32768,
    2+/*4+*/8+16+16384,
    /*4+*/8+16+1024+2048+16384+65536+131072,
    2+/*4+*/8+16+16384+65536+131072
};
char g_numGametypes = 5;

char g_numVolumes = 3;

int32_t g_timerTicsPerSecond = TICRATE;

int32_t g_actorRespawnTime = 768;
int32_t g_itemRespawnTime = 768;
int32_t g_rpgBlastRadius = 1780;
int32_t g_pipebombBlastRadius = 2500;
int32_t g_shrinkerBlastRadius = 650;
int32_t g_tripbombBlastRadius = 3880;
int32_t g_morterBlastRadius = 2500;
int32_t g_bouncemineBlastRadius = 2500;
int32_t g_seenineBlastRadius = 2048;

int32_t g_scriptSize = 1048576;

int16_t BlimpSpawnSprites[15] =
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

int32_t g_playerFriction = 0xCFD0;

int32_t g_numFreezeBounces = 3;

int32_t g_lastSaveSlot = -1;

char CheatKeys[2] = { sc_D, sc_N };

char setupfilename[BMAX_PATH] = SETUPFILENAME;

#ifdef __cplusplus
}
#endif

