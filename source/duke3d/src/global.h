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

#ifndef global_h_
#define global_h_

#ifdef global_c_
    #define G_EXTERN
#else
    #define G_EXTERN extern
#endif

#define MAXINTERPOLATIONS MAXSPRITES
// KEEPINSYNC lunatic/con_lang.lua
#define MAXSKILLS 7

// duke3d global soup :(

// XXX: we don't #include everything we need.
#include "compat.h"
#include "build.h"

G_EXTERN int32_t g_interpolationCnt;
G_EXTERN int32_t g_interpolationLock;
G_EXTERN int32_t oldipos[MAXINTERPOLATIONS];
G_EXTERN int32_t *curipos[MAXINTERPOLATIONS];
G_EXTERN int32_t bakipos[MAXINTERPOLATIONS];

#include "mmulti.h"

#include "duke3d.h"
#include "sector.h"
#include "quotes.h"

#ifdef __cplusplus
extern "C" {
#endif

G_EXTERN int32_t duke3d_globalflags;

// KEEPINSYNC astub.c (used values only)
enum DUKE3D_GLOBALFLAGS {
    DUKE3D_NO_WIDESCREEN_PINNING = 1<<0,
    DUKE3D_NO_HARDCODED_FOGPALS = 1<<1,
    DUKE3D_NO_PALETTE_CHANGES = 1<<2,
};

G_EXTERN DukeStatus_t sbar;
G_EXTERN actor_t actor[MAXSPRITES];
// g_tile: tile-specific data THAT DOES NOT CHANGE during the course of a game
G_EXTERN tiledata_t g_tile[MAXTILES];
G_EXTERN animwalltype animwall[MAXANIMWALLS];
G_EXTERN char *apStrings[MAXQUOTES],*apXStrings[MAXQUOTES];
G_EXTERN char *label;
G_EXTERN int32_t g_musicIndex;
G_EXTERN char g_loadFromGroupOnly;
G_EXTERN char g_skillCnt;
G_EXTERN char pus,pub;
G_EXTERN char ready2send;
#define MAXPLAYERNAME 32
G_EXTERN char szPlayerName[MAXPLAYERNAME];
G_EXTERN char tempbuf[MAXSECTORS<<1],packbuf[PACKBUF_SIZE],buf[1024];
#define TYPEBUFSIZE 141
G_EXTERN char typebuf[TYPEBUFSIZE];


G_EXTERN input_t localInput;
G_EXTERN input_t recsync[RECSYNCBUFSIZ];

G_EXTERN int32_t g_animWallCnt;
G_EXTERN int32_t g_animateCnt;
G_EXTERN int32_t g_cloudCnt;
G_EXTERN int32_t g_curViewscreen;
G_EXTERN int32_t g_frameRate;
G_EXTERN int32_t g_cyclerCnt;
G_EXTERN int32_t g_damageCameras;
G_EXTERN int32_t g_defaultLabelCnt;
G_EXTERN int32_t g_doQuickSave;
G_EXTERN int32_t g_earthquakeTime;
G_EXTERN int32_t g_freezerSelfDamage;
G_EXTERN int32_t g_gameQuit;
G_EXTERN int32_t g_globalRandom;
G_EXTERN int32_t g_impactDamage;
G_EXTERN int32_t g_labelCnt;
G_EXTERN int32_t g_maxPlayerHealth;
G_EXTERN int32_t g_mirrorCount;
G_EXTERN int32_t g_mostConcurrentPlayers;
G_EXTERN int32_t g_musicSize;
G_EXTERN int32_t g_playerSpawnCnt;
G_EXTERN int32_t g_scriptDebug;
G_EXTERN int32_t g_showShareware;
G_EXTERN int32_t g_spriteDeleteQueuePos;
G_EXTERN int32_t g_startArmorAmount;
G_EXTERN int32_t g_tripbombLaserMode;
G_EXTERN int32_t screenpeek;

G_EXTERN int16_t g_animateSect[MAXANIMATES];
G_EXTERN int32_t *g_animatePtr[MAXANIMATES];
G_EXTERN int32_t g_animateGoal[MAXANIMATES];
G_EXTERN int32_t g_animateVel[MAXANIMATES];

G_EXTERN int16_t g_cloudSect[256];
G_EXTERN int16_t g_cloudX;
G_EXTERN int16_t g_cloudY;
G_EXTERN int32_t g_cloudClock;

G_EXTERN int16_t SpriteDeletionQueue[1024];
G_EXTERN int16_t g_cyclers[MAXCYCLERS][6];
G_EXTERN int16_t g_mirrorSector[64];
G_EXTERN int16_t g_mirrorWall[64];
G_EXTERN int32_t *labelcode;
G_EXTERN int32_t *labeltype;
G_EXTERN int32_t lockclock;
G_EXTERN int32_t ototalclock;

G_EXTERN intptr_t *apScript;
G_EXTERN intptr_t *g_scriptPtr;

G_EXTERN map_t g_mapInfo[(MAXVOLUMES + 1) * MAXLEVELS];  // +1 volume for "intro", "briefing" and "loading" music
G_EXTERN vec2_t g_origins[MAXANIMPOINTS];

// XXX: I think this pragma pack is meaningless here.
// MSDN (https://msdn.microsoft.com/en-us/library/2e70t5y1%28VS.80%29.aspx) says:
// "pack takes effect at the first struct, union, or class declaration after
//  the pragma is seen; pack has no effect on definitions."
#pragma pack(push,1)
#ifdef global_c_
static playerdata_t g_player_s[1 + MAXPLAYERS];
playerdata_t *const g_player = &g_player_s[1];
#else
extern playerdata_t *const g_player;
#endif
G_EXTERN playerspawn_t g_playerSpawnPoints[MAXPLAYERS];
G_EXTERN input_t inputfifo[MOVEFIFOSIZ][MAXPLAYERS];
#pragma pack(pop)

G_EXTERN char g_soundlocks[MAXSOUNDS];
G_EXTERN int32_t g_noEnemies;
G_EXTERN int32_t g_restorePalette;
G_EXTERN int32_t g_screenCapture;
G_EXTERN projectile_t SpriteProjectile[MAXSPRITES];
G_EXTERN sound_t g_sounds[MAXSOUNDS];
G_EXTERN uint32_t everyothertime;
G_EXTERN uint32_t g_moveThingsCount;

#ifndef global_c_
extern char CheatKeys[2];
extern char g_gametypeNames[MAXGAMETYPES][33];
extern char g_setupFileName[BMAX_PATH];
extern char g_skillNames[MAXSKILLS][33];
extern char g_volumeNames[MAXVOLUMES][33];

extern int32_t g_actorRespawnTime;
extern int32_t g_bouncemineRadius;
extern int32_t g_deleteQueueSize;
extern int32_t g_gametypeCnt;
extern int32_t g_itemRespawnTime;
extern int32_t g_morterRadius;
extern int32_t g_numFreezeBounces;
extern int32_t g_pipebombRadius;
extern int32_t g_playerFriction;
extern int32_t g_rpgRadius;
extern int32_t g_scriptSize;
extern int32_t g_seenineRadius;
extern int32_t g_shrinkerRadius;
extern int32_t g_spriteGravity;
extern int32_t g_timerTicsPerSecond;
extern int32_t g_tripbombRadius;
extern int32_t g_volumeCnt;

extern int16_t g_blimpSpawnItems[15];
extern int32_t g_gametypeFlags[MAXGAMETYPES];
extern int32_t g_volumeFlags[MAXVOLUMES];

extern const char *s_buildDate;
#endif

enum
{
    EF_HIDEFROMSP = 1<<0,
    // EF_HIDEFROMMP = 1<<1,
};

EXTERN_INLINE_HEADER void G_UpdateInterpolations(void);
EXTERN_INLINE_HEADER void G_RestoreInterpolations(void);

#ifdef __cplusplus
}
#endif

#if defined global_c_ || !defined DISABLE_INLINING

EXTERN_INLINE void G_UpdateInterpolations(void)  //Stick at beginning of G_DoMoveThings
{
    for (bssize_t i=g_interpolationCnt-1; i>=0; i--) oldipos[i] = *curipos[i];
}

EXTERN_INLINE void G_RestoreInterpolations(void)  //Stick at end of drawscreen
{
    int32_t i=g_interpolationCnt-1;

    if (--g_interpolationLock)
        return;

    for (; i>=0; i--) *curipos[i] = bakipos[i];
}

#endif

#endif
