//-------------------------------------------------------------------------
/*
Copyright (C) 1996, 2003 - 3D Realms Entertainment
Copyright (C) 2000, 2003 - Matt Saettler (EDuke Enhancements)
Copyright (C) 2004, 2007 - EDuke32 developers

This file is part of EDuke32

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
#include "duke3d.h"

const char *s_buildDate = "20081219";
char *MusicPtr = NULL;
int g_musicSize;

short g_globalRandom;
short neartagsector, neartagwall, neartagsprite;

int neartaghitdist,lockclock,g_startArmorAmount;
// JBF: g_spriteGravity modified to default to Atomic ed. default when using 1.3d CONs
int g_spriteGravity=176;

// int temp_data[MAXSPRITES][6];
ActorData_t ActorExtra[MAXSPRITES];

short SpriteDeletionQueue[1024],g_spriteDeleteQueuePos,g_spriteDeleteQueueSize=64;
animwalltype animwall[MAXANIMWALLS];
short g_numAnimWalls;
int *animateptr[MAXANIMATES];
int animategoal[MAXANIMATES], animatevel[MAXANIMATES], g_animateCount;
// int oanimateval[MAXANIMATES];
short animatesect[MAXANIMATES];
int msx[2048],msy[2048];
short cyclers[MAXCYCLERS][6],g_numCyclers;

char *ScriptQuotes[MAXQUOTES], *ScriptQuoteRedefinitions[MAXQUOTES];

char tempbuf[2048], packbuf[576], menutextbuf[128];

char buf[1024];

short camsprite;
short g_mirrorWall[64], g_mirrorSector[64], g_mirrorCount;

int g_currentMenu;

map_t MapInfo[(MAXVOLUMES+1)*MAXLEVELS]; // +1 volume for "intro", "briefing" music

char EpisodeNames[MAXVOLUMES][33] = { "L.A. MELTDOWN", "LUNAR APOCALYPSE", "SHRAPNEL CITY" };
char SkillNames[5][33] = { "PIECE OF CAKE", "LET'S ROCK", "COME GET SOME", "DAMN I'M GOOD" };

char GametypeNames[MAXGAMETYPES][33] = { "DUKEMATCH (SPAWN)","COOPERATIVE PLAY","DUKEMATCH (NO SPAWN)","TEAM DM (SPAWN)","TEAM DM (NO SPAWN)"};

int GametypeFlags[MAXGAMETYPES] =
{
    4+8+16+1024+2048+16384,
    1+2+32+64+128+256+512+4096+8192+32768,
    2+4+8+16+16384,
    4+8+16+1024+2048+16384+65536+131072,
    2+4+8+16+16384+65536+131072
};
char g_numGametypes = 5;

int g_currentFrameRate;

char g_numVolumes = 3;

int g_timerTicsPerSecond=120;
//fx_device device;

sound_t g_sounds[ MAXSOUNDS ];

char g_numPlayerSprites,g_loadFromGroupOnly=0,g_earthquakeTime;

int fricxv,fricyv;
playerdata_t g_player[MAXPLAYERS];
input_t inputfifo[MOVEFIFOSIZ][MAXPLAYERS];
PlayerSpawn_t g_playerSpawnPoints[MAXPLAYERS];
user_defs ud;

char pus, pub;
char syncstat[MAXSYNCBYTES];
int syncvaltail, syncvaltottail;

input_t loc;
input_t recsync[RECSYNCBUFSIZ];
int avgfvel, avgsvel, avgavel, avghorz, avgbits, avgextbits;

int movefifosendplc;

//Multiplayer syncing variables
int screenpeek;

//Game recording variables

char ready2send;
int vel, svel, angvel, horiz, ototalclock, g_actorRespawnTime=768, g_itemRespawnTime=768, g_groupFileHandle;

intptr_t *g_scriptPtr,*insptr,*labelcode,*labeltype;
int g_numLabels,g_numDefaultLabels;
intptr_t *actorscrptr[MAXTILES],*g_parsingActorPtr;
char *label;
char ActorType[MAXTILES];
intptr_t *script = NULL;

int g_scriptSize = 16384;

char typebuflen,typebuf[141];

char g_musicIndex;
char EnvMusicFilename[MAXVOLUMES+1][BMAX_PATH];
char g_RTSPlaying;


short BlimpSpawnSprites[15] =
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

int g_impactDamage;
int g_scriptDebug;

//GLOBAL.C - replace the end "my's" with this
int myx, omyx, myxvel, myy, omyy, myyvel, myz, omyz, myzvel;
short myhoriz, omyhoriz, myhorizoff, omyhorizoff;
short myang, omyang, mycursectnum, myjumpingcounter;

char myjumpingtoggle, myonground, myhardlanding, myreturntocenter;
signed char multiwho, multipos, multiwhat, multiflag;

int predictfifoplc,movefifoplc;
int myxbak[MOVEFIFOSIZ], myybak[MOVEFIFOSIZ], myzbak[MOVEFIFOSIZ];
int myhorizbak[MOVEFIFOSIZ],g_playerFriction = 0xcc00, g_showShareware;

short myangbak[MOVEFIFOSIZ];
char szPlayerName[32];
int g_damageCameras,g_freezerSelfDamage=0,g_tripbombLaserMode=0;
int g_networkBroadcastMode = 255, g_movesPerPacket = 1,g_gameQuit = 0,everyothertime;
int g_numFreezeBounces=3,g_rpgBlastRadius,g_pipebombBlastRadius,g_tripbombBlastRadius,
                       g_shrinkerBlastRadius,g_morterBlastRadius,g_bouncemineBlastRadius,g_seenineBlastRadius;
STATUSBARTYPE sbar;

int mymaxlag, otherminlag, bufferjitter = 1;
short g_numClouds,clouds[128],cloudx[128],cloudy[128];
int cloudtotalclock = 0,totalmemory = 0;
int g_numInterpolations = 0, startofdynamicinterpolations = 0;
int oldipos[MAXINTERPOLATIONS];
int bakipos[MAXINTERPOLATIONS];
int *curipos[MAXINTERPOLATIONS];

int nextvoxid = 0;

int SpriteFlags[MAXTILES];

projectile_t ProjectileData[MAXTILES], DefaultProjectileData[MAXTILES];

char CheatKeys[2] = { sc_D, sc_N };
char setupfilename[BMAX_PATH]= SETUPFILENAME;
// char datetimestring[] = ""__DATE__" "__TIME__"";

int g_doQuickSave = 0;
unsigned int g_moveThingsCount = 0;

