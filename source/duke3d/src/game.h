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

#ifndef game_h_
#define game_h_

#ifndef ONLY_USERDEFS
#include "premap.h" // XXX
#endif

#ifdef __cplusplus
extern "C" {
#endif

#ifndef ONLY_USERDEFS

// Compile game-side legacy Room over Room code?
#define LEGACY_ROR 1

#define USERQUOTE_LEFTOFFSET    5
#define USERQUOTE_RIGHTOFFSET   14

#if defined(GEKKO) || defined(__OPENDINGUX__)
# define VIEWSCREENFACTOR 0
#elif defined(__ANDROID__)
# define VIEWSCREENFACTOR 1
#else
# define VIEWSCREENFACTOR 2
#endif

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
};

void A_DeleteSprite(int spriteNum);

static inline int32_t G_GetLogoFlags(void)
{
#if !defined LUNATIC
    return Gv_GetVarByLabel("LOGO_FLAGS",255, -1, -1);
#else
    extern int32_t g_logoFlags;
    return g_logoFlags;
#endif
}

#ifdef LUNATIC
typedef struct {
    vec3_t pos;
    int32_t dist, clock;
    int16_t ang, horiz, sect;
} camera_t;

extern camera_t g_camera;

# define CAMERA(Membname) (g_camera.Membname)
# define CAMERADIST (g_camera.dist)
# define CAMERACLOCK (g_camera.clock)
#else
# define CAMERA(Membname) (ud.camera ## Membname)
# define CAMERADIST g_cameraDistance
# define CAMERACLOCK g_cameraClock
#endif

#endif

#define MAXRIDECULE 10
#define MAXRIDECULELENGTH 40
#define MAXSAVEGAMENAMESTRUCT 32
#define MAXSAVEGAMENAME (MAXSAVEGAMENAMESTRUCT-1)
#define MAXPWLOCKOUT 128
#define MAXRTSNAME 128

// KEEPINSYNC lunatic/_defs_game.lua
typedef struct {
#if !defined LUNATIC
    vec3_t camerapos;
#endif
    int32_t const_visibility,uw_framerate;
    int32_t camera_time,folfvel,folavel,folx,foly,fola;
    int32_t reccnt,crosshairscale;

    int32_t runkey_mode,statusbarscale,mouseaiming,weaponswitch,drawweapon;   // JBF 20031125
    int32_t democams,color,msgdisptime,statusbarmode;
    int32_t m_noexits,noexits,autovote,automsg,idplayers;
    int32_t team, viewbob, weaponsway, althud, weaponscale, textscale;
    int32_t screenarea_x1, screenarea_y1, screenarea_x2, screenarea_y2;

    int32_t entered_name,screen_tilting,shadows,fta_on,executions,auto_run;
    int32_t coords,showfps,levelstats,m_coop,coop,screen_size,lockout,crosshair;
    int32_t playerai,angleinterpolation,obituaries;

    int32_t respawn_monsters,respawn_items,respawn_inventory,recstat,monsters_off,brightness;
    int32_t m_respawn_items,m_respawn_monsters,m_respawn_inventory,m_recstat,m_monsters_off,detail;
    int32_t m_ffire,ffire,m_player_skill,m_level_number,m_volume_number,multimode;
    int32_t player_skill,level_number,volume_number,m_marker,marker,mouseflip;
    int32_t music_episode, music_level;

    vec2_t m_origin;
    int32_t playerbest;

    int32_t configversion, bgstretch;
#if !defined LUNATIC
    int16_t cameraang, camerasect, camerahoriz;
#endif
    int16_t pause_on,from_bonus;
    int16_t camerasprite,last_camsprite;
    int16_t last_level,secretlevel;

    struct {
        int32_t UseJoystick;
        int32_t UseMouse;
        int32_t AutoAim;
        int32_t ShowOpponentWeapons;
        int32_t MouseDeadZone,MouseBias;
        int32_t SmoothInput;

        // JBF 20031211: Store the input settings because
        // (currently) mact can't regurgitate them
        int32_t MouseFunctions[MAXMOUSEBUTTONS][2];
        int32_t MouseDigitalFunctions[MAXMOUSEAXES][2];
        int32_t MouseAnalogueAxes[MAXMOUSEAXES];
        int32_t MouseAnalogueScale[MAXMOUSEAXES];
        int32_t JoystickFunctions[MAXJOYBUTTONSANDHATS][2];
        int32_t JoystickDigitalFunctions[MAXJOYAXES][2];
        int32_t JoystickAnalogueAxes[MAXJOYAXES];
        int32_t JoystickAnalogueScale[MAXJOYAXES];
        int32_t JoystickAnalogueDead[MAXJOYAXES];
        int32_t JoystickAnalogueSaturate[MAXJOYAXES];
        uint8_t KeyboardKeys[NUMGAMEFUNCTIONS][2];

        //
        // Sound variables
        //
        int32_t MasterVolume;
        int32_t FXVolume;
        int32_t MusicVolume;
        int32_t SoundToggle;
        int32_t MusicToggle;
        int32_t VoiceToggle;
        int32_t AmbienceToggle;

        int32_t NumVoices;
        int32_t NumChannels;
        int32_t NumBits;
        int32_t MixRate;

        int32_t ReverseStereo;

        //
        // Screen variables
        //

        int32_t ScreenMode;

        int32_t ScreenWidth;
        int32_t ScreenHeight;
        int32_t ScreenBPP;

        int32_t ForceSetup;
        int32_t NoAutoLoad;

        int32_t scripthandle;
        int32_t setupread;

        int32_t CheckForUpdates;
        int32_t LastUpdateCheck;
        int32_t useprecache;
    } config;

    char overhead_on,last_overhead,showweapons;
    char god,warp_on,cashman,eog,showallmap;
    char show_help,scrollmode,noclip;
    char ridecule[MAXRIDECULE][MAXRIDECULELENGTH];
    char pwlockout[MAXPWLOCKOUT],rtsname[MAXRTSNAME];
    char display_bonus_screen;
    char show_level_text;
    char wchoice[MAX_WEAPONS];

    uint8_t user_map;
    uint8_t screenfade;
} user_defs;

extern user_defs ud;

#ifndef ONLY_USERDEFS

// this is checked against http://eduke32.com/VERSION
extern const char *s_buildDate;

extern const char *g_rtsNamePtr;

extern char boardfilename[BMAX_PATH], currentboardfilename[BMAX_PATH];
extern char boardfilename[BMAX_PATH];

extern const char *defaultrtsfilename[GAMECOUNT];
extern const char *G_DefaultRtsFile(void);

#ifdef LEGACY_ROR
extern char ror_protectedsectors[MAXSECTORS];
#endif

extern float r_ambientlight;

extern int32_t g_Debug;
extern int32_t g_Shareware;
#if !defined LUNATIC
extern int32_t g_cameraClock;
extern int32_t g_cameraDistance;
#endif
extern int32_t g_crosshairSum;
extern int32_t g_doQuickSave;
extern int32_t g_forceWeaponChoice;
extern int32_t g_fakeMultiMode;
extern int32_t g_levelTextTime;
extern int32_t g_quitDeadline;
extern int32_t g_restorePalette;
extern int32_t hud_glowingquotes;
extern int32_t hud_showmapname;
extern int32_t r_maxfps;
extern int32_t tempwallptr;
extern int32_t ticrandomseed;
extern int32_t vote_map;
extern int32_t voting;

extern int32_t MAXCACHE1DSIZE;
//extern int8_t cheatbuf[MAXCHEATLEN],cheatbuflen;

#define CROSSHAIR_PAL (MAXPALOOKUPS-RESERVEDPALS-1)

extern palette_t CrosshairColors;
extern palette_t DefaultCrosshairColors;

extern uint64_t g_frameDelay;

int32_t A_CheckInventorySprite(spritetype *s);
int32_t A_InsertSprite(int16_t whatsect, int32_t s_x, int32_t s_y, int32_t s_z, int16_t s_pn, int8_t s_s, uint8_t s_xr,
                       uint8_t s_yr, int16_t s_a, int16_t s_ve, int16_t s_zv, int16_t s_ow, int16_t s_ss);
int A_Spawn(int j,int pn);
int G_DoMoveThings(void);
//int32_t G_EndOfLevel(void);

#ifdef YAX_ENABLE
void Yax_SetBunchZs(int32_t sectnum, int32_t cf, int32_t daz);
#else
#define Yax_SetBunchZs(sectnum, cf, daz)
#endif

#ifdef LUNATIC
void El_CreateGameState(void);
#endif
void G_PostCreateGameState(void);

void A_SpawnCeilingGlass(int spriteNum,int sectNum,int glassCnt);
void A_SpawnGlass(int spriteNum,int glassCnt);
void A_SpawnRandomGlass(int spriteNum,int wallNum,int glassCnt);
void A_SpawnWallGlass(int i,int wallnum,int n);
void G_AddUserQuote(const char *daquote);
void G_BackToMenu(void);
void G_DumpDebugInfo(void);

const char* G_PrintYourTime(void);
const char* G_PrintParTime(void);
const char* G_PrintDesignerTime(void);
const char* G_PrintBestTime(void);
void G_BonusScreen(int32_t bonusonly);
//void G_CheatGetInv(void);
void G_DisplayRest(int32_t smoothratio);
void G_DoSpriteAnimations(int32_t ourx, int32_t oury, int32_t oura, int32_t smoothratio);
void G_DrawBackground(void);
void G_DrawFrags(void);
void G_HandleMirror(int32_t x, int32_t y, int32_t z, int32_t a, int32_t horiz, int32_t smoothratio);
void G_DrawRooms(int32_t snum,int32_t smoothratio);
void G_DrawTXDigiNumZ(int32_t starttile,int32_t x,int32_t y,int32_t n,int32_t s,int32_t pal,int32_t cs,int32_t x1,int32_t y1,int32_t x2,int32_t y2,int32_t z);
int G_FPSLimit(void);
void G_GameExit(const char *t) ATTRIBUTE((noreturn));
void G_GameQuit(void);
void G_GetCrosshairColor(void);
void G_HandleLocalKeys(void);
void G_HandleSpecialKeys(void);
void G_UpdateAppTitle(void);
void G_PrintGameQuotes(int32_t snum);
//void G_SE40(int32_t smoothratio);
void G_SetCrosshairColor(int32_t r,int32_t g,int32_t b);
void G_Shutdown(void);
void G_UpdatePlayerFromMenu(void);
void M32RunScript(const char *s);
void P_DoQuote(int32_t q,DukePlayer_t *p);
void P_SetGamePalette(DukePlayer_t *player, uint32_t palid, int32_t set);

#define NEG_ALPHA_TO_BLEND(alpha, blend, orientation) do { \
    if (alpha < 0) { blend = -alpha; alpha = 0; orientation |= RS_TRANS1; } \
} while (0)

// Cstat protection mask for (currently) spawned MASKWALL* sprites.
// TODO: look at more cases of cstat=(cstat&PROTECTED)|ADDED in A_Spawn()?
// 2048+(32+16)+8+4
#define SPAWN_PROTECT_CSTAT_MASK (CSTAT_SPRITE_NOSHADE|CSTAT_SPRITE_SLAB|CSTAT_SPRITE_XFLIP|CSTAT_SPRITE_YFLIP);

void fadepal(int32_t r,int32_t g,int32_t b,int32_t start,int32_t end,int32_t step);
//void fadepaltile(int32_t r,int32_t g,int32_t b,int32_t start,int32_t end,int32_t step,int32_t tile);
void G_InitTimer(int32_t ticpersec);

static inline int32_t G_GetTeamPalette(int32_t team)
{
    int8_t pal[] = { 3, 10, 11, 12 };

    if ((unsigned)team >= ARRAY_SIZE(pal))
        return 0;

    return pal[team];
}

#define A_CheckSpriteFlags(spriteNum, iType) (((g_tile[sprite[spriteNum].picnum].flags^actor[spriteNum].flags) & iType) != 0)
// (unsigned)iPicnum check: AMC TC Rusty Nails, bayonet MG alt. fire, iPicnum == -1 (via aplWeaponShoots)
#define A_CheckSpriteTileFlags(iPicnum, iType) (((unsigned)iPicnum < MAXTILES) && (g_tile[iPicnum].flags & iType) != 0)
#define S_StopSound(num) S_StopEnvSound(num, -1)

extern int G_StartRTS(int lumpNum, int localPlayer);

extern void G_MaybeAllocPlayer(int32_t pnum);

static inline void G_HandleAsync(void)
{
    handleevents();
    Net_GetPackets();
}

static inline int32_t calc_smoothratio(int32_t totalclk, int32_t ototalclk)
{
    return clamp((totalclk-ototalclk)*(65536/TICSPERFRAME), 0, 65536);
}

// sector effector lotags
enum
{
    SE_0_ROTATING_SECTOR              = 0,
    SE_1_PIVOT                        = 1,
    SE_2_EARTHQUAKE                   = 2,
    SE_3_RANDOM_LIGHTS_AFTER_SHOT_OUT = 3,
    SE_4_RANDOM_LIGHTS                = 4,
    SE_5                              = 5,
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


#define G_ModDirSnprintf(buf, size, basename, ...)                                                                          \
    \
(((g_modDir[0] != '/') ? Bsnprintf(buf, size, "%s/" basename, g_modDir, ##__VA_ARGS__)                                      \
                       : Bsnprintf(buf, size, basename, ##__VA_ARGS__)) >= ((int32_t)size) - 1\
)
#define G_ModDirSnprintfLite(buf, size, basename)                                                                          \
    \
((g_modDir[0] != '/') ? Bsnprintf(buf, size, "%s/%s", g_modDir, basename)                                      \
                      : Bsnprintf(buf, size, basename))

static inline void G_NewGame_EnterLevel(void)
{
    G_NewGame(ud.m_volume_number, ud.m_level_number, ud.m_player_skill);

    if (G_EnterLevel(MODE_GAME))
        G_BackToMenu();
}

static inline int G_GetMusicIdx(const char *str)
{
    int32_t lev, ep;
    char    b1, b2;

    int numMatches = sscanf(str, "%c%d%c%d", &b1,&ep, &b2,&lev);

    if (numMatches != 4 || Btoupper(b1) != 'E' || Btoupper(b2) != 'L')
        return -1;

    if ((unsigned)--lev >= MAXLEVELS || (unsigned)--ep >= MAXVOLUMES)
        return -2;

    return (ep * MAXLEVELS) + lev;
}

static inline int G_GetViewscreenSizeShift(const uspritetype *tspr)
{
#if VIEWSCREENFACTOR == 0
    UNREFERENCED_PARAMETER(tspr);
    return VIEWSCREENFACTOR;
#else
    static const int mask = (1<<VIEWSCREENFACTOR)-1;
    const int rem = (tspr->xrepeat & mask) | (tspr->yrepeat & mask);

    for (bssize_t i=0; i < VIEWSCREENFACTOR; i++)
        if (rem & (1<<i))
            return i;

    return VIEWSCREENFACTOR;
#endif
}

extern void G_StartMusic(void);

#ifdef LUNATIC
void El_SetCON(const char *conluacode);
#endif

EXTERN_INLINE_HEADER void G_SetStatusBarScale(int32_t sc);

EXTERN_INLINE_HEADER void SetIfGreater(int32_t *variable, int32_t potentialValue);

#endif

#ifdef __cplusplus
}
#endif

#ifndef ONLY_USERDEFS

#if defined game_c_ || !defined DISABLE_INLINING

EXTERN_INLINE void G_SetStatusBarScale(int32_t sc)
{
    ud.statusbarscale = clamp(sc, 36, 100);
    G_UpdateScreenArea();
}

// the point of this is to prevent re-running a function or calculation passed to potentialValue
// without making a new variable under each individual circumstance
EXTERN_INLINE void SetIfGreater(int32_t *variable, int32_t potentialValue)
{
    if (potentialValue > *variable)
        *variable = potentialValue;
}

#endif

#endif

#endif
