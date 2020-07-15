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

#include "build.h"
#include "compat.h"
#include "duke3d.h"
#include "mmulti.h"
#include "quotemgr.h"
#include "sounds.h"
#include "constants.h"

BEGIN_DUKE_NS


#define VOLUMEALL           (g_Shareware == 0)
#define PLUTOPAK            (true)//g_scriptVersion >= 14)
#define VOLUMEONE           (g_Shareware == 1)

#define MOVEFIFOSIZ         256

#define MAXGAMETYPES        16

enum {
    MUS_INTRO = 0,
    MUS_BRIEFING = 1,
    MUS_LOADING = 2,
};


#define MAXMINECARTS 16
#define MAXJAILDOORS 32
#define MAXLIGHTNINSECTORS 64
#define MAXTORCHSECTORS 64
#define MAXGEOSECTORS 64

#ifdef global_c_
    #define G_EXTERN
#else
    #define G_EXTERN extern
#endif

#define MAXINTERPOLATIONS MAXSPRITES


G_EXTERN int32_t duke3d_globalflags;

// KEEPINSYNC astub.c (used values only)
enum DUKE3D_GLOBALFLAGS {
    DUKE3D_NO_WIDESCREEN_PINNING = 1<<0,
    DUKE3D_NO_HARDCODED_FOGPALS = 1<<1,
    DUKE3D_NO_PALETTE_CHANGES = 1<<2,
};

struct animwalltype
{
    int16_t wallnum, tag;
};


G_EXTERN animwalltype animwall[MAXANIMWALLS];
enum
{
    MAXLABELLEN = 64
};

G_EXTERN char g_loadFromGroupOnly;
G_EXTERN char pus,pub;
G_EXTERN char ready2send;
#define MAXPLAYERNAME 32
G_EXTERN char tempbuf[MAXSECTORS<<1],buf[1024];


G_EXTERN input_t localInput;

G_EXTERN int32_t avgfvel, avgsvel, avgbits;
G_EXTERN fix16_t avgavel, avghorz;
G_EXTERN int8_t avgextbits;

G_EXTERN int32_t movefifosendplc;

G_EXTERN int32_t predictfifoplc;

G_EXTERN int32_t g_networkBroadcastMode;

G_EXTERN int32_t g_animWallCnt;
#define numanimwalls g_animWallCnt
G_EXTERN int32_t g_animateCnt;
#define animatecnt g_animateCnt
G_EXTERN int32_t numclouds;
G_EXTERN int32_t camsprite;
G_EXTERN int32_t g_frameRate;
G_EXTERN int32_t g_cyclerCnt;
#define numcyclers g_cyclerCnt
G_EXTERN int32_t g_damageCameras;
#define camerashitable g_damageCameras
G_EXTERN int32_t g_defaultLabelCnt;
G_EXTERN int32_t g_doQuickSave;
G_EXTERN int32_t g_earthquakeTime;
#define earthquaketime g_earthquakeTime
G_EXTERN int32_t g_freezerSelfDamage;
#define freezerhurtowner g_freezerSelfDamage
G_EXTERN int32_t g_gameQuit;
G_EXTERN int32_t global_random;
G_EXTERN int32_t impact_damage;
G_EXTERN int32_t g_maxPlayerHealth;
G_EXTERN int32_t mirrorcnt;
G_EXTERN int32_t playerswhenstarted;
G_EXTERN int32_t g_musicSize;
G_EXTERN int32_t numplayersprites;
G_EXTERN int32_t g_scriptDebug;
G_EXTERN int32_t show_shareware;
G_EXTERN int32_t g_spriteDeleteQueuePos;
G_EXTERN int32_t max_player_health;
G_EXTERN int32_t max_armour_amount;
G_EXTERN int32_t lasermode;
G_EXTERN int32_t screenpeek;

G_EXTERN int16_t g_animateSect[MAXANIMATES];
#define animatesect g_animateSect
G_EXTERN int32_t *g_animatePtr[MAXANIMATES];
#define animateptr g_animatePtr
G_EXTERN int32_t g_animateGoal[MAXANIMATES];
#define animategoal g_animateGoal
G_EXTERN int32_t g_animateVel[MAXANIMATES];
#define animatevel g_animateVel

G_EXTERN int16_t clouds[256];
G_EXTERN int16_t cloudx;
G_EXTERN int16_t cloudy;
G_EXTERN ClockTicks cloudtotalclock;

G_EXTERN int16_t SpriteDeletionQueue[1024];
G_EXTERN int16_t g_cyclers[MAXCYCLERS][6];
#define cyclers g_cyclers
G_EXTERN int16_t mirrorsector[64];
G_EXTERN int16_t mirrorwall[64];
G_EXTERN ClockTicks lockclock;
G_EXTERN ClockTicks ototalclock;

G_EXTERN int32_t wupass;
G_EXTERN int32_t chickenplant;
G_EXTERN int32_t thunderon;
G_EXTERN int32_t g_ufoSpawn;
#define ufospawn g_ufoSpawn
G_EXTERN int32_t g_ufoCnt;
#define ufocnt g_ufoCnt
G_EXTERN int32_t g_hulkSpawn;
#define hulkspawn g_hulkSpawn
G_EXTERN int32_t g_lastLevel;
#define lastlevel g_lastLevel


G_EXTERN int32_t geosectorwarp[MAXGEOSECTORS];
G_EXTERN int32_t geosectorwarp2[MAXGEOSECTORS];
G_EXTERN int32_t geosector[MAXGEOSECTORS];
G_EXTERN int32_t geox[MAXGEOSECTORS];
G_EXTERN int32_t geoy[MAXGEOSECTORS];
G_EXTERN int32_t geox2[MAXGEOSECTORS];
G_EXTERN int32_t geoy2[MAXGEOSECTORS];
G_EXTERN uint32_t geocnt;

G_EXTERN int32_t g_thunderFlash;
G_EXTERN int32_t g_thunderTime;
G_EXTERN int32_t g_winderFlash;
G_EXTERN int32_t g_winderTime;
G_EXTERN int32_t g_brightness;

G_EXTERN int16_t ambientlotag[64];
G_EXTERN int16_t ambienthitag[64];
G_EXTERN uint32_t ambientfx;


G_EXTERN vec2_t g_origins[MAXANIMPOINTS];
struct msx_
{
    int &operator[](int v) { return g_origins[v].x; }
};
struct msy_
{
    int &operator[](int v) { return g_origins[v].y; }
};
G_EXTERN msx_ msx;
G_EXTERN msy_ msy;

G_EXTERN int32_t WindTime, WindDir;
G_EXTERN int16_t fakebubba_spawn, mamaspawn_count, banjosound, g_bellTime, BellSprite;
#define BellTime g_bellTime
#define word_119BE0 BellSprite
G_EXTERN uint8_t g_spriteExtra[MAXSPRITES], g_sectorExtra[MAXSECTORS]; // move these back into the base structs!
G_EXTERN uint8_t enemysizecheat, ufospawnsminion, pistonsound, chickenphase, RRRA_ExitedLevel, fogactive;
extern int32_t g_cdTrack;
#define raat607 enemysizecheat // only as a reminder
#define raat605 chickenphase
#define at59d yeehaa_timer

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
G_EXTERN player_orig po[MAXPLAYERS];
#pragma pack(pop)

G_EXTERN uint32_t everyothertime;
G_EXTERN double g_gameUpdateTime;
G_EXTERN double g_gameUpdateAndDrawTime;
#define GAMEUPDATEAVGTIMENUMSAMPLES 100
extern float g_gameUpdateAvgTime;

#ifndef global_c_
extern char CheatKeys[2];
extern char g_gametypeNames[MAXGAMETYPES][33];

extern int32_t respawnactortime;
extern int32_t bouncemineblastradius;
extern int32_t g_deleteQueueSize;
extern int32_t g_gametypeCnt;
extern int32_t respawnitemtime;
extern int32_t g_morterRadius;
#define morterblastradius g_morterRadius
extern int32_t numfreezebounces;
extern int32_t g_pipebombRadius;
#define pipebombblastradius g_pipebombRadius
extern int32_t dukefriction;
extern int32_t rpgblastradius;
extern int32_t g_scriptSize;
extern int32_t g_seenineRadius;
#define seenineblastradius g_seenineRadius
extern int32_t g_shrinkerRadius;
#define shrinkerblastradius g_shrinkerRadius
extern int32_t g_spriteGravity;
extern int32_t g_timerTicsPerSecond;
extern int32_t g_tripbombRadius;
#define tripbombblastradius g_tripbombRadius
#define powderkegblastradius g_tripbombRadius
extern int32_t g_volumeCnt;
#define gc g_spriteGravity

extern int16_t weaponsandammosprites[15];
extern int32_t g_gametypeFlags[MAXGAMETYPES];

#endif  

enum
{
    EF_HIDEFROMSP = 1<<0,
};

// Interpolation code is the same in all games with slightly different naming - this needs to be unified and cleaned up.
extern int32_t numinterpolations;
extern int32_t* curipos[MAXINTERPOLATIONS];
extern int32_t bakipos[MAXINTERPOLATIONS];


// old names as porting help.
void updateinterpolations();
void restoreinterpolations();
void setinterpolation(int* posptr);
void stopinterpolation(int* posptr);
void dointerpolations(int smoothratio);


extern player_struct ps[MAXPLAYERS];


extern int spriteqamount;
#define spriteq SpriteDeletionQueue
#define spriteqloc g_spriteDeleteQueuePos



enum
{
    kHitTypeMask = 0xC000,
    //kHitIndexMask = 0x3FFF,
    kHitSector = 0x4000,
    kHitWall = 0x8000,
    kHitSprite = 0xC000,
};

extern uint8_t shadedsector[MAXSECTORS];




END_DUKE_NS

#include "inlines.h"

#endif
