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

G_EXTERN int32_t g_numInterpolations;
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

enum {
    DUKE3D_NO_WIDESCREEN_PINNING = 1<<0,
    DUKE3D_NO_HARDCODED_FOGPALS = 1<<1,
};

G_EXTERN DukeStatus_t sbar;
G_EXTERN actor_t actor[MAXSPRITES];
// g_tile: tile-specific data THAT DOES NOT CHANGE during the course of a game
G_EXTERN tiledata_t g_tile[MAXTILES];
G_EXTERN animwalltype animwall[MAXANIMWALLS];
G_EXTERN char *ScriptQuotes[MAXQUOTES],*ScriptQuoteRedefinitions[MAXQUOTES];
G_EXTERN char *label;
G_EXTERN int32_t g_musicIndex;
G_EXTERN char g_loadFromGroupOnly;
G_EXTERN char g_numSkills;
G_EXTERN char myjumpingtoggle,myonground,myhardlanding,myreturntocenter;
G_EXTERN char pus,pub;
G_EXTERN char ready2send;
#define MAXPLAYERNAME 32
G_EXTERN char szPlayerName[MAXPLAYERNAME];
// XXX: menutextbuf overflow possible?
G_EXTERN char tempbuf[MAXSECTORS<<1],packbuf[PACKBUF_SIZE],menutextbuf[128],buf[1024];
#define TYPEBUFSIZE 141
G_EXTERN char typebuf[TYPEBUFSIZE];
G_EXTERN input_t avg;
G_EXTERN input_t loc;
G_EXTERN input_t recsync[RECSYNCBUFSIZ];
G_EXTERN int16_t SpriteDeletionQueue[1024],g_spriteDeleteQueuePos;
G_EXTERN int16_t animatesect[MAXANIMATES];
G_EXTERN int16_t g_curViewscreen;
G_EXTERN int16_t cyclers[MAXCYCLERS][6],g_numCyclers;
G_EXTERN int16_t g_globalRandom;
G_EXTERN int16_t g_mirrorWall[64],g_mirrorSector[64],g_mirrorCount;
G_EXTERN int16_t g_numAnimWalls;
G_EXTERN int16_t g_numClouds,clouds[256],cloudx,cloudy;
G_EXTERN int16_t myang,omyang,mycursectnum,myjumpingcounter;
G_EXTERN int16_t myhoriz,omyhoriz,myhorizoff,omyhorizoff;
G_EXTERN int32_t *animateptr[MAXANIMATES];
G_EXTERN int32_t animategoal[MAXANIMATES],animatevel[MAXANIMATES],g_animateCount;
G_EXTERN int32_t cloudtotalclock;
G_EXTERN int32_t g_currentFrameRate;
G_EXTERN int32_t g_damageCameras,g_freezerSelfDamage;
G_EXTERN int32_t g_doQuickSave;
G_EXTERN uint16_t g_earthquakeTime;
G_EXTERN int32_t g_gameQuit;
G_EXTERN int32_t g_impactDamage,g_maxPlayerHealth;
G_EXTERN int32_t g_musicSize;
G_EXTERN int32_t g_numLabels,g_numDefaultLabels;
G_EXTERN int32_t g_scriptDebug;
G_EXTERN int32_t g_showShareware;
G_EXTERN int8_t g_numPlayerSprites;
G_EXTERN int32_t g_tripbombLaserMode;
G_EXTERN int32_t msx[2048],msy[2048];
G_EXTERN int32_t neartaghitdist,lockclock,g_startArmorAmount;
G_EXTERN int32_t playerswhenstarted;
G_EXTERN int32_t screenpeek;
G_EXTERN int32_t startofdynamicinterpolations;
G_EXTERN int32_t ototalclock;
G_EXTERN intptr_t *g_parsingActorPtr;
G_EXTERN intptr_t *g_scriptPtr;
G_EXTERN int32_t *labelcode,*labeltype;
G_EXTERN intptr_t *script;
G_EXTERN map_t MapInfo[(MAXVOLUMES+1)*MAXLEVELS];  // +1 volume for "intro", "briefing" and "loading" music

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

G_EXTERN projectile_t SpriteProjectile[MAXSPRITES];
G_EXTERN sound_t g_sounds[MAXSOUNDS];
G_EXTERN uint32_t everyothertime;
G_EXTERN uint32_t g_moveThingsCount;
G_EXTERN vec3_t my,omy,myvel;
G_EXTERN volatile char g_soundlocks[MAXSOUNDS];
G_EXTERN int32_t g_restorePalette;
G_EXTERN int32_t g_screenCapture;
G_EXTERN int32_t g_noEnemies;

#ifndef global_c_
extern const char *s_buildDate;
extern int32_t g_spriteGravity;
extern int16_t g_spriteDeleteQueueSize;
extern char EpisodeNames[MAXVOLUMES][33];
extern int32_t EpisodeFlags[MAXVOLUMES];
extern char SkillNames[MAXSKILLS][33];
extern char GametypeNames[MAXGAMETYPES][33];
extern int32_t GametypeFlags[MAXGAMETYPES];
extern char g_numGametypes;
extern char g_numVolumes;
extern int32_t g_timerTicsPerSecond;
extern int32_t g_actorRespawnTime;
extern int32_t g_itemRespawnTime;
extern int32_t g_scriptSize;
extern int16_t BlimpSpawnSprites[15];
extern int32_t g_playerFriction;
extern int32_t g_numFreezeBounces;
extern int32_t g_lastSaveSlot;
extern int32_t g_rpgBlastRadius;
extern int32_t g_pipebombBlastRadius;
extern int32_t g_tripbombBlastRadius;
extern int32_t g_shrinkerBlastRadius;
extern int32_t g_morterBlastRadius;
extern int32_t g_bouncemineBlastRadius;
extern int32_t g_seenineBlastRadius;
extern char CheatKeys[2];
extern char setupfilename[BMAX_PATH];
#endif

enum
{
    EF_HIDEFROMSP = 1<<0,
    // EF_HIDEFROMMP = 1<<1,
};

#ifdef __cplusplus
}
#endif

#endif
