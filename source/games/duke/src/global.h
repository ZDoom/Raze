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
#include "quotes.h"
#include "sector.h"
#include "sounds.h"

BEGIN_DUKE_NS

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
// KEEPINSYNC lunatic/con_lang.lua

// duke3d global soup :(


G_EXTERN int32_t duke3d_globalflags;

// KEEPINSYNC astub.c (used values only)
enum DUKE3D_GLOBALFLAGS {
    DUKE3D_NO_WIDESCREEN_PINNING = 1<<0,
    DUKE3D_NO_HARDCODED_FOGPALS = 1<<1,
    DUKE3D_NO_PALETTE_CHANGES = 1<<2,
};

G_EXTERN actor_t actor[MAXSPRITES];
// actorinfo: tile-specific data THAT DOES NOT CHANGE during the course of a game
G_EXTERN ActorInfo actorinfo[MAXTILES];
G_EXTERN animwalltype animwall[MAXANIMWALLS];
extern char *label;
G_EXTERN char g_loadFromGroupOnly;
G_EXTERN char g_skillCnt;
G_EXTERN char pus,pub;
G_EXTERN char ready2send;
#define MAXPLAYERNAME 32
G_EXTERN char tempbuf[MAXSECTORS<<1],buf[1024];
G_EXTERN uint8_t packbuf[PACKBUF_SIZE];


G_EXTERN input_t localInput;
G_EXTERN input_t recsync[RECSYNCBUFSIZ];

//G_EXTERN uint8_t syncstat, syncval[MAXPLAYERS][MOVEFIFOSIZ];
//G_EXTERN int32_t syncvalhead[MAXPLAYERS], syncvaltail, syncvaltottail;

G_EXTERN int32_t avgfvel, avgsvel, avgbits;
G_EXTERN fix16_t avgavel, avghorz;
G_EXTERN int8_t avgextbits;

G_EXTERN int32_t movefifosendplc;
G_EXTERN int32_t movefifoplc;

G_EXTERN int32_t predictfifoplc;
G_EXTERN vec3_t mypos, omypos, myvel;
G_EXTERN fix16_t myhoriz, omyhoriz, myhorizoff, omyhorizoff, myang, omyang;
G_EXTERN int16_t mycursectnum, myjumpingcounter;
G_EXTERN uint8_t myjumpingtoggle, myonground, myhardlanding, myreturntocenter;
G_EXTERN int16_t my_MotoSpeed;
G_EXTERN uint8_t my_NotOnWater, my_MotoOnGround;
G_EXTERN uint8_t my_moto_do_bump, my_moto_bump_fast, my_moto_on_oil, my_moto_on_mud;
G_EXTERN int16_t my_moto_bump, my_moto_bump_target, my_moto_turb;
G_EXTERN int32_t my_stairs;

G_EXTERN vec3_t myposbak[MOVEFIFOSIZ];
G_EXTERN fix16_t myhorizbak[MOVEFIFOSIZ], myangbak[MOVEFIFOSIZ];
G_EXTERN int32_t myminlag[MAXPLAYERS], mymaxlag, otherminlag, bufferjitter;

G_EXTERN int32_t g_networkBroadcastMode, g_movesPerPacket;

G_EXTERN int32_t g_animWallCnt;
#define numanimwalls g_animWallCnt
G_EXTERN int32_t g_animateCnt;
#define animatecnt g_animateCnt
G_EXTERN int32_t g_cloudCnt;
G_EXTERN int32_t g_curViewscreen;
#define camsprite g_curViewscreen
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
G_EXTERN int32_t g_globalRandom;
#define global_random g_globalRandom
G_EXTERN int32_t impact_damage;
extern int32_t labelcnt;
G_EXTERN int32_t g_maxPlayerHealth;
G_EXTERN int32_t g_mirrorCount;
G_EXTERN int32_t g_mostConcurrentPlayers;
G_EXTERN int32_t g_musicSize;
G_EXTERN int32_t g_playerSpawnCnt;
G_EXTERN int32_t g_scriptDebug;
G_EXTERN int32_t g_showShareware;
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

G_EXTERN int16_t g_cloudSect[256];
G_EXTERN int16_t g_cloudX;
G_EXTERN int16_t g_cloudY;
G_EXTERN ClockTicks g_cloudClock;

G_EXTERN int16_t SpriteDeletionQueue[1024];
G_EXTERN int16_t g_cyclers[MAXCYCLERS][6];
#define cyclers g_cyclers
G_EXTERN int16_t g_mirrorSector[64];
G_EXTERN int16_t g_mirrorWall[64];
G_EXTERN int32_t *labelcode;
#if 0
G_EXTERN int32_t *labeltype;
#endif
G_EXTERN ClockTicks lockclock;
G_EXTERN ClockTicks ototalclock;

G_EXTERN int32_t wupass;
G_EXTERN int32_t g_chickenPlant;
#define chickenplant g_chickenPlant
G_EXTERN int32_t g_thunderOn;
G_EXTERN int32_t g_ufoSpawn;
#define ufospawn g_ufoSpawn
G_EXTERN int32_t g_ufoCnt;
#define ufocnt g_ufoCnt
G_EXTERN int32_t g_hulkSpawn;
#define hulkspawn g_hulkSpawn
G_EXTERN int32_t g_vixenLevel;
G_EXTERN int32_t g_lastLevel;
#define lastlevel g_lastLevel
G_EXTERN int32_t g_turdLevel;


G_EXTERN int32_t g_geoSectorWarp[MAXGEOSECTORS];
G_EXTERN int32_t g_geoSectorWarp2[MAXGEOSECTORS];
G_EXTERN int32_t g_geoSector[MAXGEOSECTORS];
G_EXTERN int32_t g_geoSectorX[MAXGEOSECTORS];
G_EXTERN int32_t g_geoSectorY[MAXGEOSECTORS];
G_EXTERN int32_t g_geoSectorX2[MAXGEOSECTORS];
G_EXTERN int32_t g_geoSectorY2[MAXGEOSECTORS];
G_EXTERN uint32_t g_geoSectorCnt;

G_EXTERN int32_t g_thunderFlash;
G_EXTERN int32_t g_thunderTime;
G_EXTERN int32_t g_winderFlash;
G_EXTERN int32_t g_winderTime;
G_EXTERN int32_t g_brightness;

G_EXTERN int16_t ambientlotag[64];
G_EXTERN int16_t ambienthitag[64];
G_EXTERN uint32_t g_ambientCnt;

inline int32_t G_HaveActor(int spriteNum)
{
    return actorinfo[spriteNum].scriptaddress != NULL;
}

inline int32_t G_DefaultActorHealth(int spriteNum)	// rename!
{
    return G_HaveActor(spriteNum) ? ScriptCode[actorinfo[spriteNum].scriptaddress] : 0;
}




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
G_EXTERN int16_t fakebubba_spawn, mamaspawn_count, banjosound, g_bellTime, g_bellSprite;
#define BellTime g_bellTime
#define word_119BE0 g_bellSprite
G_EXTERN uint8_t g_spriteExtra[MAXSPRITES], g_sectorExtra[MAXSECTORS]; // move these back into the base structs!
G_EXTERN uint8_t enemysizecheat, ufospawnsminion, pistonsound, chickenphase, RRRA_ExitedLevel, RRRA_EndEpisode, fogactive;
G_EXTERN int32_t g_cdTrack;
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
G_EXTERN playerspawn_t g_playerSpawnPoints[MAXPLAYERS];
#define po g_playerSpawnPoints
G_EXTERN input_t inputfifo[MOVEFIFOSIZ][MAXPLAYERS];
#pragma pack(pop)

G_EXTERN int32_t g_noEnemies;
G_EXTERN int32_t restorepalette;
G_EXTERN uint32_t everyothertime;
G_EXTERN uint32_t g_moveThingsCount;
G_EXTERN double g_gameUpdateTime;
G_EXTERN double g_gameUpdateAndDrawTime;
#define GAMEUPDATEAVGTIMENUMSAMPLES 100
extern float g_gameUpdateAvgTime;

#ifndef global_c_
extern char CheatKeys[2];
extern char g_gametypeNames[MAXGAMETYPES][33];

extern int32_t respawnactortime;
extern int32_t g_bouncemineRadius;
#define bouncemineblastradius g_bouncemineRadius
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

extern const char *s_buildDate;
#endif  

inline void clearfriction()
{
    for (int playerNum = 0; playerNum != -1; playerNum = connectpoint2[playerNum])
    {
        vec2_t& fric = g_player[playerNum].ps->fric;
        fric.x = fric.y = 0;
    }
}

enum
{
    EF_HIDEFROMSP = 1<<0,
};

// Interpolation code is the same in all games with slightly different naming - this needs to be unified and cleaned up.
extern int32_t g_interpolationCnt;
extern int32_t* curipos[MAXINTERPOLATIONS];
extern int32_t bakipos[MAXINTERPOLATIONS];
void G_UpdateInterpolations(void);
void G_RestoreInterpolations(void);
int G_SetInterpolation(int32_t* const posptr);
void G_StopInterpolation(const int32_t* const posptr);
void G_DoInterpolations(int smoothRatio);

// old names as porting help.
inline void updateinterpolations()
{
    G_UpdateInterpolations();
}
inline void restoreinterpolations()
{
    G_RestoreInterpolations();
}
inline int setinterpolation(int32_t* posptr)
{
   return G_SetInterpolation(posptr);
}
inline void stopinterpolation(int32_t* posptr)
{
    G_SetInterpolation(posptr);
}
inline void dointerpolations(int smoothratio)
{
    G_DoInterpolations(smoothratio);
}


// Hack struct to allow old code to access the EDuke-style player data without changing it.
struct psaccess
{
    DukePlayer_t& operator[](int index)
    {
        return *g_player[index].ps;
    }
};
extern psaccess ps;

#define spriteqamount g_deleteQueueSize
#define spriteq SpriteDeletionQueue
#define spriteqloc g_spriteDeleteQueuePos

inline bool isIn(int value, int first)
{
    return value == first;
}

template<typename... Args>
bool isIn(int value, int first, Args... args) 
{
    return value == first || isIn(value, args...);
}

inline bool isIn(int value, const std::initializer_list<int> &list)
{
    for (auto v : list) if (v == value) return true;
    return false;
}


// these are mainly here to avoid directly accessing the input data so that it can be more easily refactored later.
inline bool PlayerInput(int pl, int bit)
{
    return TEST_SYNC_KEY(g_player[pl].input->bits, bit);
}

inline void PlayerSetInput(int pl, int bit)
{
    g_player[pl].input->bits |= (1 << bit);
}

inline void PlayerClearInput(int pl, int bit)
{
    g_player[pl].input->bits &= ~(1 << bit);
}

inline int PlayerInputBits(int pl, int bits)
{
    return (g_player[pl].input->bits & bits);
}

inline bool PlayerInputSideVel(int pl)
{
    return g_player[pl].input->svel;
}

inline bool PlayerInputForwardVel(int pl)
{
    return g_player[pl].input->fvel;
}

enum
{
    kHitTypeMask = 0xC000,
    //kHitIndexMask = 0x3FFF,
    kHitSector = 0x4000,
    kHitWall = 0x8000,
    kHitSprite = 0xC000,
};

END_DUKE_NS

#endif
