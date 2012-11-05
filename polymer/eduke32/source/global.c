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
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
*/
//-------------------------------------------------------------------------

#define __global_c__
#include "global.h"
#include "duke3d.h"

void initialize_globals(void)
{
    #include "rev.h"
    s_buildDate = "20120522";

    // JBF: g_spriteGravity modified to default to Atomic ed. default when using 1.3d CONs
    g_spriteGravity = 176;

    g_spriteDeleteQueueSize = 64;

    strcpy(EpisodeNames[0], "L.A. Meltdown");
    strcpy(EpisodeNames[1], "Lunar Apocalypse");
    strcpy(EpisodeNames[2], "Shrapnel City");

    strcpy(SkillNames[0], "Piece Of Cake");
    strcpy(SkillNames[1], "Let's Rock");
    strcpy(SkillNames[2], "Come Get Some");
    strcpy(SkillNames[3], "Damn I'm Good");

    strcpy(GametypeNames[0], "DukeMatch (Spawn)");
    strcpy(GametypeNames[1], "Cooperative Play");
    strcpy(GametypeNames[2], "DukeMatch (No Spawn)");
    strcpy(GametypeNames[3], "Team DM (Spawn)");
    strcpy(GametypeNames[4], "Team DM (No Spawn)");

    GametypeFlags[0] = /*4+*/8+16+1024+2048+16384;
    GametypeFlags[1] = 1+2+32+64+128+256+512+4096+8192+32768;
    GametypeFlags[2] = 2+/*4+*/8+16+16384;
    GametypeFlags[3] = /*4+*/8+16+1024+2048+16384+65536+131072;
    GametypeFlags[4] = 2+/*4+*/8+16+16384+65536+131072;

    g_numGametypes = 5;

    g_numVolumes = 3;

    g_timerTicsPerSecond = TICRATE;

    g_actorRespawnTime = 768;
    g_itemRespawnTime = 768;

    g_scriptSize = 1048576;

    BlimpSpawnSprites[0] = RPGSPRITE__STATIC;
    BlimpSpawnSprites[1] = CHAINGUNSPRITE__STATIC;
    BlimpSpawnSprites[2] = DEVISTATORAMMO__STATIC;
    BlimpSpawnSprites[3] = RPGAMMO__STATIC;
    BlimpSpawnSprites[4] = RPGAMMO__STATIC;
    BlimpSpawnSprites[5] = JETPACK__STATIC;
    BlimpSpawnSprites[6] = SHIELD__STATIC;
    BlimpSpawnSprites[7] = FIRSTAID__STATIC;
    BlimpSpawnSprites[8] = STEROIDS__STATIC;
    BlimpSpawnSprites[9] = RPGAMMO__STATIC;
    BlimpSpawnSprites[10] = RPGAMMO__STATIC;
    BlimpSpawnSprites[11] = RPGSPRITE__STATIC;
    BlimpSpawnSprites[12] = RPGAMMO__STATIC;
    BlimpSpawnSprites[13] = FREEZESPRITE__STATIC;
    BlimpSpawnSprites[14] = FREEZEAMMO__STATIC;

    g_playerFriction = 0xcc00;

    g_numFreezeBounces=3;

    g_lastSaveSlot = -1;

    CheatKeys[0] = sc_D;
    CheatKeys[1] = sc_N;

    strcpy(setupfilename, SETUPFILENAME);
}
