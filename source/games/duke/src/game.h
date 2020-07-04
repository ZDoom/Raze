//-------------------------------------------------------------------------
/*
Copyright (C) 2016 EDuke32 developers and contributors

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

#pragma once

#ifndef ONLY_USERDEFS
#include "premap.h" // XXX
#endif

#include "fix16.h"
#include "gamedef.h"
#include "net.h"
#include "mmulti.h"
#include "palette.h"
#include "cmdlib.h"
#include "screenjob.h"

BEGIN_DUKE_NS

#ifndef ONLY_USERDEFS

// Compile game-side legacy Room over Room code?
#define LEGACY_ROR 1

enum GametypeFlags_t {
    GAMETYPE_COOP                   = 0x00000001,
    GAMETYPE_WEAPSTAY               = 0x00000002,
    GAMETYPE_FRAGBAR                = 0x00000004,
    GAMETYPE_SCORESHEET             = 0x00000008,
    GAMETYPE_DMSWITCHES             = 0x00000010,
    GAMETYPE_COOPSPAWN              = 0x00000020,
    GAMETYPE_ACCESSCARDSPRITES      = 0x00000040,
    GAMETYPE_COOPVIEW               = 0x00000080,
    GAMETYPE_COOPSOUND              = 0x00000100,
    GAMETYPE_OTHERPLAYERSINMAP      = 0x00000200,
    GAMETYPE_ITEMRESPAWN            = 0x00000400,
    GAMETYPE_MARKEROPTION           = 0x00000800,
    GAMETYPE_PLAYERSFRIENDLY        = 0x00001000,
    GAMETYPE_FIXEDRESPAWN           = 0x00002000,
    GAMETYPE_ACCESSATSTART          = 0x00004000,
    GAMETYPE_PRESERVEINVENTORYDEATH = 0x00008000,
    GAMETYPE_TDM                    = 0x00010000,
    GAMETYPE_TDMSPAWN               = 0x00020000
};

// logo control
enum LogoFlags_t {
    LOGO_ENABLED           = 0x00000001,
    LOGO_PLAYANIM          = 0x00000002,
    LOGO_PLAYMUSIC         = 0x00000004,
    LOGO_3DRSCREEN         = 0x00000008,
    LOGO_TITLESCREEN       = 0x00000010,
    LOGO_DUKENUKEM         = 0x00000020,
    LOGO_THREEDEE          = 0x00000040,
    LOGO_PLUTOPAKSPRITE    = 0x00000080,
    LOGO_SHAREWARESCREENS  = 0x00000100,
    LOGO_TENSCREEN         = 0x00000200,
    LOGO_STOPANIMSOUNDS    = 0x00000400,
    LOGO_NOE4CUTSCENE      = 0x00000800,
    LOGO_NOE1BONUSSCENE    = 0x00001000,
    LOGO_NOE2BONUSSCENE    = 0x00002000,
    LOGO_NOE3BONUSSCENE    = 0x00004000,
    LOGO_NOE4BONUSSCENE    = 0x00008000,
    LOGO_NOE1ENDSCREEN     = 0x00010000,
    LOGO_NOE2ENDSCREEN     = 0x00020000,
    LOGO_NOE3RADLOGO       = 0x00040000,
    LOGO_NODUKETEAMTEXT    = 0x00080000,
    LOGO_NODUKETEAMPIC     = 0x00100000,
    LOGO_STOPMISCSOUNDS    = 0x00200000,
    LOGO_NOGAMETITLE       = 0x00400000,
    LOGO_NOTITLEBAR        = 0x00800000,
    LOGO_HIDEEPISODE       = 0x01000000,
    LOGO_NOHELP            = 0x02000000,
    LOGO_NOCREDITS         = 0x04000000,
};

enum {
    STATUSBAR_NONONE    = 0x00000001,
    STATUSBAR_NOMINI    = 0x00000002,
    STATUSBAR_NOFULL    = 0x00000004,
    STATUSBAR_NOSHRINK  = 0x00000008,
    STATUSBAR_NOFRAGBAR = 0x00000010,
    STATUSBAR_NOOVERLAY = 0x00000020,
    STATUSBAR_NOMODERN  = 0x00000040,
};


# define CAMERA(Membname) (ud.camera ## Membname)
# define CAMERADIST cameradist
# define CAMERACLOCK cameraclock

#endif

#define MAXSAVEGAMENAMESTRUCT 32
#define MAXSAVEGAMENAME (MAXSAVEGAMENAMESTRUCT-1)
#define MAXPWLOCKOUT 128
#define MAXRTSNAME 128

#define MAX_RETURN_VALUES 6

// KEEPINSYNC lunatic/_defs_game.lua

typedef struct {
    vec3_t camerapos;
    int32_t const_visibility,uw_framerate;
    int32_t camera_time,folfvel,folavel,folx,foly,fola;
    int32_t reccnt;

    int32_t statusbarscale,weaponswitch;   // JBF 20031125
    int32_t statusbarmode;
	int32_t noexits,automsg;
    int32_t althud;
    int32_t statusbarflags, statusbarrange;

    int32_t entered_name,screen_tilting;
    int32_t coop,screen_size,lockout,crosshair;
    int32_t angleinterpolation;

    int32_t respawn_monsters,respawn_items,respawn_inventory,recstat,monsters_off,brightness;
    int32_t m_respawn_items,m_respawn_monsters,m_respawn_inventory,m_recstat,m_monsters_off;
    int32_t ffire,m_player_skill,m_level_number,m_volume_number,multimode;
    int32_t player_skill,level_number,volume_number,marker;

    //int32_t returnvar[MAX_RETURN_VALUES-1];

    uint32_t userbytever;

    fix16_t cameraq16ang, cameraq16horiz;
    int16_t camerasect;
    int16_t pause_on,from_bonus;
    int16_t camerasprite,last_camsprite;
    int16_t last_level,secretlevel;

    int8_t menutitle_pal, slidebar_palselected, slidebar_paldisabled;

    struct {
        int32_t AutoAim;
        int32_t ShowOpponentWeapons;
    } config;

    char overhead_on,last_overhead,showweapons;
    char god,warp_on,cashman,eog;
    char scrollmode,clipping;
    char display_bonus_screen;
    char show_level_text;

    uint8_t user_map;
    uint8_t screenfade, menubackground;
    uint8_t shadow_pal;
    uint8_t wchoice[MAXPLAYERS][MAX_WEAPONS];
} user_defs;

extern user_defs ud;

#ifndef ONLY_USERDEFS

// this is checked against http://eduke32.com/VERSION
extern const char *s_buildDate;

extern char boardfilename[BMAX_PATH];
#define USERMAPMUSICFAKEVOLUME MAXVOLUMES
#define USERMAPMUSICFAKELEVEL (MAXLEVELS-1)
#define USERMAPMUSICFAKESLOT ((USERMAPMUSICFAKEVOLUME * MAXLEVELS) + USERMAPMUSICFAKELEVEL)

static inline int G_HaveUserMap(void)
{
    return (boardfilename[0] != 0 && ud.level_number == 7 && ud.volume_number == 0);
}

static inline int Menu_HaveUserMap(void)
{
    return (boardfilename[0] != 0 && m_level_number == 7 && ud.m_volume_number == 0);
}

extern const char *defaultrtsfilename[GAMECOUNT];
extern const char *G_DefaultRtsFile(void);

extern int32_t g_Debug;
extern int32_t g_Shareware;
extern int32_t cameraclock;
extern int32_t cameradist;
extern int32_t g_crosshairSum;
extern int32_t g_doQuickSave;
extern int32_t g_levelTextTime;
extern int32_t g_quitDeadline;
extern int32_t restorepalette;
extern int32_t tempwallptr;
extern int32_t ticrandomseed;
extern int32_t vote_map;
extern int32_t voting;

//extern int8_t cheatbuf[MAXCHEATLEN],cheatbuflen;

short EGS(short whatsect, int s_x, int s_y, int s_z, short s_pn, signed char s_s, signed char s_xr, signed char s_yr, short s_a, short s_ve, int s_zv, short s_ow, signed char s_ss);
#define A_InsertSprite EGS
int G_DoMoveThings(void);
//int32_t G_EndOfLevel(void);

#ifdef YAX_ENABLE
void Yax_SetBunchZs(int32_t sectnum, int32_t cf, int32_t daz);
#else
#define Yax_SetBunchZs(sectnum, cf, daz)
#endif

void G_PostCreateGameState(void);

void ceilingglass(int spriteNum,int sectNum,int glassCnt);
void spriteglass(int spriteNum,int glassCnt);
void lotsofcolourglass(int spriteNum,int wallNum,int glassCnt);
void lotsofglass(int spriteNum,int wallnum,int glassCnt);

void G_BackToMenu(void);
void G_DumpDebugInfo(void);

const char* G_PrintYourTime(void);
const char* G_PrintParTime(void);
const char* G_PrintDesignerTime(void);
const char* G_PrintBestTime(void);
void G_BonusScreen(int32_t bonusonly);
void G_BonusScreenRRRA(int32_t bonusonly);
//void G_CheatGetInv(void);
void G_DisplayRest(int32_t smoothratio);
void G_DoSpriteAnimations(int32_t ourx, int32_t oury, int32_t ourz, int32_t oura, int32_t smoothratio);
void drawbackground(void);
void G_DrawFrags(void);
void G_HandleMirror(int32_t x, int32_t y, int32_t z, fix16_t a, fix16_t horiz, int32_t smoothratio);
void G_DrawRooms(int32_t playerNum,int32_t smoothratio);
void G_DrawTXDigiNumZ(int32_t starttile,int32_t x,int32_t y,int32_t n,int32_t s,int32_t pal,int32_t cs,int32_t x1,int32_t y1,int32_t x2,int32_t y2,int32_t z);
void G_HandleLocalKeys(void);
void G_HandleSpecialKeys(void);
//void G_SE40(int32_t smoothratio);
void G_UpdatePlayerFromMenu(void);
void FTA(int q, struct player_struct* p);

void P_SetGamePalette(DukePlayer_t* player, uint32_t palid, ESetPalFlags flags);
void OnMotorcycle(DukePlayer_t *pPlayer, int spriteNum);
void OffMotorcycle(DukePlayer_t *pPlayer);
void OnBoat(DukePlayer_t *pPlayer, int spriteNum);
void OffBoat(DukePlayer_t *pPlayer);

// Cstat protection mask for (currently) spawned MASKWALL* sprites.
// TODO: look at more cases of cstat=(cstat&PROTECTED)|ADDED in fi.spawn()?
// 2048+(32+16)+8+4
#define SPAWN_PROTECT_CSTAT_MASK (CSTAT_SPRITE_NOSHADE|CSTAT_SPRITE_ALIGNMENT_SLAB|CSTAT_SPRITE_XFLIP|CSTAT_SPRITE_YFLIP);

void G_InitTimer(int32_t ticspersec);

inline int32_t G_GetTeamPalette(int32_t team)
{
    int8_t pal[] = { 3, 10, 11, 12 };

    if ((unsigned)team >= ARRAY_SIZE(pal))
        return 0;

    return pal[team];
}

enum
{
    TFLAG_WALLSWITCH = 1
};
// for now just flags not related to actors, may get more info later.
struct TileInfo
{
    int flags;
};
extern TileInfo tileinfo[MAXTILES];

inline int actorflag(int spritenum, int mask)
{
    return (((actorinfo[sprite[spritenum].picnum].flags/* ^ actor[spritenum].flags*/) & mask) != 0);
}

inline int actorfella(int spnum)
{
    return actorflag(spnum, SFLAG_KILLCOUNT);
}

inline void setflag(int flag, const std::initializer_list<short>& types)
{
    for (auto val : types)
    {
        actorinfo[val].flags |= flag;
    }
}

inline bool inventory(spritetype* S)
{
    return !!(actorinfo[S->picnum].flags & SFLAG_INVENTORY);
}


inline void settileflag(int flag, const std::initializer_list<short>& types)
{
    for (auto val : types)
    {
        tileinfo[val].flags |= flag;
    }
}

inline bool wallswitchcheck(int s)
{
    return !!(tileinfo[s].flags & TFLAG_WALLSWITCH);
}

// (unsigned)iPicnum check: AMC TC Rusty Nails, bayonet MG alt. fire, iPicnum == -1 (via aplWeaponShoots)
#define A_CheckSpriteTileFlags(iPicnum, iType) (((unsigned)iPicnum < MAXTILES) && (actorinfo[iPicnum].flags & iType) != 0)
#define S_StopSound(num) S_StopEnvSound(num, -1)

extern int G_StartRTS(int lumpNum, int localPlayer);

extern void G_MaybeAllocPlayer(int32_t pnum);

static inline void G_HandleAsync(void)
{
    handleevents();
}

static inline int32_t calc_smoothratio(ClockTicks totalclk, ClockTicks ototalclk)
{
    if (!(((!g_netServer && ud.multimode < 2) && ((g_player[myconnectindex].ps->gm & MODE_MENU) == 0)) ||
          (g_netServer || ud.multimode > 1) ||
          ud.recstat == 2) ||
        ud.pause_on)
    {
        return 65536;
    }

    return CalcSmoothRatio(totalclk, ototalclk, REALGAMETICSPERSEC);
}

// sector effector lotags
enum
{
    SE_0_ROTATING_SECTOR              = 0,
    SE_1_PIVOT                        = 1,
    SE_2_EARTHQUAKE                   = 2,
    SE_3_RANDOM_LIGHTS_AFTER_SHOT_OUT = 3,
    SE_4_RANDOM_LIGHTS                = 4,
    SE_5_BOSS                         = 5,
    SE_6_SUBWAY                       = 6,
    // ^^ potentially incomplete substitution in code
    // vv almost surely complete substitution
    SE_7_TELEPORT                      = 7,
    SE_8_UP_OPEN_DOOR_LIGHTS           = 8,
    SE_9_DOWN_OPEN_DOOR_LIGHTS         = 9,
    SE_10_DOOR_AUTO_CLOSE              = 10,
    SE_11_SWINGING_DOOR                = 11,
    SE_12_LIGHT_SWITCH                 = 12,
    SE_13_EXPLOSIVE                    = 13,
    SE_14_SUBWAY_CAR                   = 14,
    SE_15_SLIDING_DOOR                 = 15,
    SE_16_REACTOR                      = 16,
    SE_17_WARP_ELEVATOR                = 17,
    SE_18_INCREMENTAL_SECTOR_RISE_FALL = 18,
    SE_19_EXPLOSION_LOWERS_CEILING     = 19,
    SE_20_STRETCH_BRIDGE               = 20,
    SE_21_DROP_FLOOR                   = 21,
    SE_22_TEETH_DOOR                   = 22,
    SE_23_ONE_WAY_TELEPORT             = 23,
    SE_24_CONVEYOR                     = 24,
    SE_25_PISTON                       = 25,
    SE_26                              = 26,
    SE_27_DEMO_CAM                     = 27,
    SE_28_LIGHTNING                    = 28,
    SE_29_WAVES                        = 29,
    SE_30_TWO_WAY_TRAIN                = 30,
    SE_31_FLOOR_RISE_FALL              = 31,
    SE_32_CEILING_RISE_FALL            = 32,
    SE_33_QUAKE_DEBRIS                 = 33,
    SE_34                              = 34,  // XXX
    SE_35                              = 35,  // XXX
    SE_36_PROJ_SHOOTER                 = 36,
    SE_47_LIGHT_SWITCH                 = 47,
    SE_48_LIGHT_SWITCH                 = 48,
    SE_49_POINT_LIGHT                  = 49,
    SE_50_SPOT_LIGHT                   = 50,
    SE_130                             = 130,
    SE_131                             = 131,
};

// sector lotags
enum
{
    ST_0_NO_EFFECT   = 0,
    ST_1_ABOVE_WATER = 1,
    ST_2_UNDERWATER  = 2,
    ST_3             = 3,
    // ^^^ maybe not complete substitution in code
    ST_9_SLIDING_ST_DOOR     = 9,
    ST_15_WARP_ELEVATOR      = 15,
    ST_16_PLATFORM_DOWN      = 16,
    ST_17_PLATFORM_UP        = 17,
    ST_18_ELEVATOR_DOWN      = 18,
    ST_19_ELEVATOR_UP        = 19,
    ST_20_CEILING_DOOR       = 20,
    ST_21_FLOOR_DOOR         = 21,
    ST_22_SPLITTING_DOOR     = 22,
    ST_23_SWINGING_DOOR      = 23,
    ST_25_SLIDING_DOOR       = 25,
    ST_26_SPLITTING_ST_DOOR  = 26,
    ST_27_STRETCH_BRIDGE     = 27,
    ST_28_DROP_FLOOR         = 28,
    ST_29_TEETH_DOOR         = 29,
    ST_30_ROTATE_RISE_BRIDGE = 30,
    ST_31_TWO_WAY_TRAIN      = 31,
    // left: ST 32767, 65534, 65535
};


static inline void G_NewGame_EnterLevel(void)
{
    G_NewGame(ud.m_volume_number, m_level_number, ud.m_player_skill);

    if (G_EnterLevel(MODE_GAME))
        G_BackToMenu();
}

extern void G_PrintCurrentMusic(void);

void addspritetodelete(int spnum);
void checkavailinven(struct player_struct* p);

int initspriteforspawn(int j, int pn, const std::initializer_list<int> &excludes);
void spawninitdefault(int j, int i);
void spawntransporter(int j, int i, bool beam);
int spawnbloodpoolpart1(int j, int i);
void initfootprint(int j, int i);
void initshell(int j, int i, bool isshell);
void initcrane(int j, int i, int CRANEPOLE);
void initwaterdrip(int j, int i);
int initreactor(int j, int i, bool isrecon);
void spawneffector(int i);

struct Dispatcher
{
    // global stuff
    void (*ShowLogo)(CompletionFunc completion);
    void (*InitFonts)();
	void (*PrintPaused)();

	// sectors_?.cpp
    void (*think)();
	void (*initactorflags)();
	bool (*isadoorwall)(int dapic);
	void (*animatewalls)();
	void (*operaterespawns)(int low);
	void (*operateforcefields)(int s, int low);
	bool (*checkhitswitch)(int snum, int w, int switchtype);
	void (*activatebysector)(int sect, int j);
	void (*checkhitwall)(int spr, int dawallnum, int x, int y, int z, int atwith);
    void (*checkplayerhurt)(struct player_struct* p, int j);
	bool (*checkhitceiling)(int sn);
	void (*checkhitsprite)(int i, int sn);
	void (*checksectors)(int low);

	bool (*ceilingspace)(int sectnum);
	bool (*floorspace)(int sectnum);
	void (*addweapon)(struct player_struct *p, int weapon);
	void (*hitradius)(short i, int  r, int  hp1, int  hp2, int  hp3, int  hp4);
	int  (*movesprite)(short spritenum, int xchange, int ychange, int zchange, unsigned int cliptype);
	void (*lotsofmoney)(spritetype *s, short n);
	void (*lotsofmail)(spritetype *s, short n);
	void (*lotsofpaper)(spritetype *s, short n);
	void (*guts)(spritetype* s, short gtype, short n, short p);
	void (*gutsdir)(spritetype* s, short gtype, short n, short p);
	int  (*ifhitsectors)(int sectnum);
	int  (*ifhitbyweapon)(int sectnum);
	void (*fall)(int g_i, int g_p);
    bool (*spawnweapondebris)(int picnum, int dnum);
    void (*respawnhitag)(spritetype* g_sp);
    void (*checktimetosleep)(int g_i);
    void (*move)(int g_i, int g_p, int g_x);
	int (*spawn)(int j, int pn);
    void (*check_fta_sounds)(int i);

    // player
    void (*incur_damage)(struct player_struct* p);
    void (*shoot)(int, int);
    void (*selectweapon)(int snum, int j);
    int (*doincrements)(struct player_struct* p);
    void (*checkweapons)(struct player_struct* p);
    void (*processinput)(int snum);
    void (*displayweapon)(int snum);
    void (*displaymasks)(int snum);


};

extern Dispatcher fi;

#endif

END_DUKE_NS
