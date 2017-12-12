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

#ifndef gamedef_h_
#define gamedef_h_

#include "build.h"  // hashtable_t
#include "common.h"  // tokenlist
#include "player.h"  // projectile_t

#ifdef __cplusplus
extern "C" {
#endif

#define LABEL_HASPARM2  1
#define LABEL_ISSTRING  2

#define MAXCHEATLEN             20
#define NUMCHEATCODES           (int32_t)ARRAY_SIZE(CheatStrings)

#define VM_INSTMASK 0xfff

#define C_CUSTOMERROR(Text, ...)                                                                                                           \
    do                                                                                                                                     \
    {                                                                                                                                      \
        C_ReportError(-1);                                                                                                                 \
        initprintf("%s:%d: error: " Text "\n", g_scriptFileName, g_lineNumber, ##__VA_ARGS__);                                             \
        g_errorCnt++;                                                                                                                      \
    } while (0)

#define C_CUSTOMWARNING(Text, ...)                                                                                                         \
    do                                                                                                                                     \
    {                                                                                                                                      \
        C_ReportError(-1);                                                                                                                 \
        initprintf("%s:%d: warning: " Text "\n", g_scriptFileName, g_lineNumber, ##__VA_ARGS__);                                           \
        g_warningCnt++;                                                                                                                    \
    } while (0)

#if !defined LUNATIC
extern intptr_t const * insptr;
extern void VM_ScriptInfo(intptr_t const *ptr, int range);
#endif

extern hashtable_t h_gamefuncs;

#if !defined LUNATIC
extern hashtable_t h_gamevars;
extern hashtable_t h_arrays;
extern hashtable_t h_labels;

extern int32_t g_aimAngleVarID;   // var ID of "AUTOAIMANGLE"
extern int32_t g_angRangeVarID;   // var ID of "ANGRANGE"
extern int32_t g_hitagVarID;      // var ID of "HITAG"
extern int32_t g_lotagVarID;      // var ID of "LOTAG"
extern int32_t g_returnVarID;     // var ID of "RETURN"
extern int32_t g_textureVarID;    // var ID of "TEXTURE"
extern int32_t g_thisActorVarID;  // var ID of "THISACTOR"
extern int32_t g_weaponVarID;     // var ID of "WEAPON"
extern int32_t g_worksLikeVarID;  // var ID of "WORKSLIKE"
extern int32_t g_zRangeVarID;     // var ID of "ZRANGE"

// KEEPINSYNC gamevars.c: "special vars for struct access"
enum QuickStructureAccess_t
{
    STRUCT_SPRITE,
    STRUCT_SECTOR,
    STRUCT_WALL,
    STRUCT_PLAYER,
    STRUCT_ACTORVAR,
    STRUCT_PLAYERVAR,
    STRUCT_TSPR,
    STRUCT_PROJECTILE,
    STRUCT_THISPROJECTILE,
    STRUCT_USERDEF,
    STRUCT_INPUT,
    STRUCT_TILEDATA,
    STRUCT_PALDATA,
    NUMQUICKSTRUCTS,
};

extern int32_t g_structVarIDs;

#include "events_defs.h"
extern intptr_t apScriptEvents[MAXEVENTS];
#endif

extern char CheatStrings[][MAXCHEATLEN];
extern char g_scriptFileName[BMAX_PATH];

extern const uint32_t CheatFunctionFlags[];
extern const uint8_t  CheatFunctionIDs[];

extern int32_t g_errorCnt;
extern int32_t g_lineNumber;
extern int32_t g_numXStrings;
extern int32_t g_scriptVersion;
extern int32_t g_totalLines;
extern int32_t g_warningCnt;

extern int32_t otherp;

extern const char *EventNames[];  // MAXEVENTS

#if !defined LUNATIC
extern intptr_t *g_scriptPtr;

typedef struct
{
    const char *name;
    int lId, flags, maxParm2;
} memberlabel_t;

extern const memberlabel_t ActorLabels[];
extern const memberlabel_t InputLabels[];
extern const memberlabel_t PalDataLabels[];
extern const memberlabel_t PlayerLabels[];
extern const memberlabel_t ProjectileLabels[];
extern const memberlabel_t SectorLabels[];
extern const memberlabel_t TileDataLabels[];
extern const memberlabel_t TsprLabels[];
extern const memberlabel_t UserdefsLabels[];
extern const memberlabel_t WallLabels[];
#endif

typedef projectile_t defaultprojectile_t;

extern defaultprojectile_t DefaultProjectile;
int32_t C_AllocQuote(int32_t qnum);
void C_AllocProjectile(int32_t j);
void C_FreeProjectile(int32_t j);
void C_InitQuotes(void);
void C_InitProjectiles(void);

extern int32_t g_numProjectiles;

typedef struct {
    int spriteNum;
    int playerNum;
    int playerDist;
    int flags;

    union {
        spritetype * pSprite;
        uspritetype *pUSprite;
    };

    int32_t *     pData;
    DukePlayer_t *pPlayer;
    actor_t *     pActor;
} vmstate_t;

extern vmstate_t vm;

void G_DoGameStartup(const int32_t *params);
void C_DefineMusic(int volumeNum, int levelNum, const char *fileName);

void C_DefineVolumeFlags(int32_t vol, int32_t flags);
void C_UndefineVolume(int32_t vol);
void C_UndefineSkill(int32_t skill);
void C_UndefineLevel(int32_t vol, int32_t lev);
#if defined LUNATIC
void C_DefineSound(int32_t sndidx, const char *fn, int32_t args[5]);
void C_DefineQuote(int32_t qnum, const char *qstr);
void C_DefineVolumeName(int32_t vol, const char *name);
void C_DefineSkillName(int32_t skill, const char *name);
void C_DefineLevelName(int32_t vol, int32_t lev, const char *fn,
                       int32_t partime, int32_t designertime,
                       const char *levelname);
void C_DefineGameFuncName(int32_t idx, const char *name);
void C_DefineGameType(int32_t idx, int32_t flags, const char *name);
int32_t C_SetDefName(const char *name);
void C_DefineProjectile(int32_t j, int32_t what, int32_t val);
void C_SetCfgName(const char *cfgname);
#else
void C_ReportError(int32_t iError);
void C_Compile(const char *filenam);

extern int32_t g_errorLineNum;
extern int32_t g_tw;

typedef struct {
    const char* token;
    int32_t val;
} tokenmap_t;

extern const tokenmap_t iter_tokens[];

extern char const * VM_GetKeywordForID(int32_t id);

// KEEPINSYNC lunatic/con_lang.lua
enum SystemString_t {
    STR_MAPNAME,
    STR_MAPFILENAME,
    STR_PLAYERNAME,
    STR_VERSION,
    STR_GAMETYPE,
    STR_VOLUMENAME,
    STR_YOURTIME,
    STR_PARTIME,
    STR_DESIGNERTIME,
    STR_BESTTIME,
};

enum ScriptError_t
{
    ERROR_CLOSEBRACKET,
    ERROR_EVENTONLY,
    ERROR_EXCEEDSMAXTILES,
    ERROR_EXPECTEDKEYWORD,
    ERROR_FOUNDWITHIN,
    ERROR_ISAKEYWORD,
    ERROR_NOENDSWITCH,
    ERROR_NOTAGAMEDEF,
    ERROR_NOTAGAMEVAR,
    ERROR_NOTAGAMEARRAY,
    ERROR_GAMEARRAYBNC,
    ERROR_GAMEARRAYBNO,
    ERROR_INVALIDARRAYWRITE,
    ERROR_OPENBRACKET,
    ERROR_PARAMUNDEFINED,
    ERROR_NOTAMEMBER,
    ERROR_SYNTAXERROR,
    ERROR_VARREADONLY,
    ERROR_ARRAYREADONLY,
    ERROR_VARTYPEMISMATCH,
    WARNING_BADGAMEVAR,
    WARNING_DUPLICATECASE,
    WARNING_DUPLICATEDEFINITION,
    WARNING_EVENTSYNC,
    WARNING_LABELSONLY,
    WARNING_NAMEMATCHESVAR,
    WARNING_VARMASKSKEYWORD,
    WARNING_ARRAYMASKSKEYWORD
};

enum PlayerLabel_t
{
    PLAYER_ZOOM,
    PLAYER_EXITX,
    PLAYER_EXITY,
    PLAYER_LOOGIEX,
    PLAYER_LOOGIEY,
    PLAYER_NUMLOOGS,
    PLAYER_LOOGCNT,
    PLAYER_POSX,
    PLAYER_POSY,
    PLAYER_POSZ,
    PLAYER_HORIZ,
    PLAYER_OHORIZ,
    PLAYER_OHORIZOFF,
    PLAYER_INVDISPTIME,
    PLAYER_BOBPOSX,
    PLAYER_BOBPOSY,
    PLAYER_OPOSX,
    PLAYER_OPOSY,
    PLAYER_OPOSZ,
    PLAYER_PYOFF,
    PLAYER_OPYOFF,
    PLAYER_POSXV,
    PLAYER_POSYV,
    PLAYER_POSZV,
    PLAYER_LAST_PISSED_TIME,
    PLAYER_TRUEFZ,
    PLAYER_TRUECZ,
    PLAYER_PLAYER_PAR,
    PLAYER_VISIBILITY,
    PLAYER_BOBCOUNTER,
    PLAYER_WEAPON_SWAY,
    PLAYER_PALS_TIME,
    PLAYER_RANDOMFLAMEX,
    PLAYER_CRACK_TIME,
    PLAYER_AIM_MODE,
    PLAYER_ANG,
    PLAYER_OANG,
    PLAYER_ANGVEL,
    PLAYER_CURSECTNUM,
    PLAYER_LOOK_ANG,
    PLAYER_LAST_EXTRA,
    PLAYER_SUBWEAPON,
    PLAYER_AMMO_AMOUNT,
    PLAYER_WACKEDBYACTOR,
    PLAYER_FRAG,
    PLAYER_FRAGGEDSELF,
    PLAYER_CURR_WEAPON,
    PLAYER_LAST_WEAPON,
    PLAYER_TIPINCS,
    PLAYER_HORIZOFF,
    PLAYER_WANTWEAPONFIRE,
    PLAYER_HOLODUKE_AMOUNT,
    PLAYER_NEWOWNER,
    PLAYER_HURT_DELAY,
    PLAYER_HBOMB_HOLD_DELAY,
    PLAYER_JUMPING_COUNTER,
    PLAYER_AIRLEFT,
    PLAYER_KNEE_INCS,
    PLAYER_ACCESS_INCS,
    PLAYER_FTA,
    PLAYER_FTQ,
    PLAYER_ACCESS_WALLNUM,
    PLAYER_ACCESS_SPRITENUM,
    PLAYER_KICKBACK_PIC,
    PLAYER_GOT_ACCESS,
    PLAYER_WEAPON_ANG,
    PLAYER_FIRSTAID_AMOUNT,
    PLAYER_SOMETHINGONPLAYER,
    PLAYER_ON_CRANE,
    PLAYER_I,
    PLAYER_ONE_PARALLAX_SECTNUM,
    PLAYER_OVER_SHOULDER_ON,
    PLAYER_RANDOM_CLUB_FRAME,
    PLAYER_FIST_INCS,
    PLAYER_ONE_EIGHTY_COUNT,
    PLAYER_CHEAT_PHASE,
    PLAYER_DUMMYPLAYERSPRITE,
    PLAYER_EXTRA_EXTRA8,
    PLAYER_QUICK_KICK,
    PLAYER_HEAT_AMOUNT,
    PLAYER_ACTORSQU,
    PLAYER_TIMEBEFOREEXIT,
    PLAYER_CUSTOMEXITSOUND,
    PLAYER_WEAPRECS,
    PLAYER_WEAPRECCNT,
    PLAYER_INTERFACE_TOGGLE_FLAG,
    PLAYER_ROTSCRNANG,
    PLAYER_DEAD_FLAG,
    PLAYER_SHOW_EMPTY_WEAPON,
    PLAYER_SCUBA_AMOUNT,
    PLAYER_JETPACK_AMOUNT,
    PLAYER_STEROIDS_AMOUNT,
    PLAYER_SHIELD_AMOUNT,
    PLAYER_HOLODUKE_ON,
    PLAYER_PYCOUNT,
    PLAYER_WEAPON_POS,
    PLAYER_FRAG_PS,
    PLAYER_TRANSPORTER_HOLD,
    PLAYER_CLIPDIST,
    PLAYER_LAST_FULL_WEAPON,
    PLAYER_FOOTPRINTSHADE,
    PLAYER_BOOT_AMOUNT,
    PLAYER_SCREAM_VOICE,
    PLAYER_GM,
    PLAYER_ON_WARPING_SECTOR,
    PLAYER_FOOTPRINTCOUNT,
    PLAYER_HBOMB_ON,
    PLAYER_JUMPING_TOGGLE,
    PLAYER_RAPID_FIRE_HOLD,
    PLAYER_ON_GROUND,
    PLAYER_NAME,
    PLAYER_INVEN_ICON,
    PLAYER_BUTTONPALETTE,
    PLAYER_JETPACK_ON,
    PLAYER_SPRITEBRIDGE,
    PLAYER_LASTRANDOMSPOT,
    PLAYER_SCUBA_ON,
    PLAYER_FOOTPRINTPAL,
    PLAYER_HEAT_ON,
    PLAYER_HOLSTER_WEAPON,
    PLAYER_FALLING_COUNTER,
    PLAYER_GOTWEAPON,
    PLAYER_REFRESH_INVENTORY,
    PLAYER_PALETTE,
    PLAYER_TOGGLE_KEY_FLAG,
    PLAYER_KNUCKLE_INCS,
    PLAYER_WALKING_SND_TOGGLE,
    PLAYER_PALOOKUP,
    PLAYER_HARD_LANDING,
    PLAYER_MAX_SECRET_ROOMS,
    PLAYER_SECRET_ROOMS,
    PLAYER_PALS,
    PLAYER_MAX_ACTORS_KILLED,
    PLAYER_ACTORS_KILLED,
    PLAYER_RETURN_TO_CENTER,
    PLAYER_RUNSPEED,
    PLAYER_SBS,
    PLAYER_RELOADING,
    PLAYER_AUTO_AIM,
    PLAYER_MOVEMENT_LOCK,
    PLAYER_SOUND_PITCH,
    PLAYER_WEAPONSWITCH,
    PLAYER_TEAM,
    PLAYER_MAX_PLAYER_HEALTH,
    PLAYER_MAX_SHIELD_AMOUNT,
    PLAYER_MAX_AMMO_AMOUNT,
    PLAYER_LAST_QUICK_KICK,
    PLAYER_AUTOSTEP,
    PLAYER_AUTOSTEP_SBW,
    PLAYER_HUDPAL,
    PLAYER_INDEX,
    PLAYER_CONNECTED,
    PLAYER_FRAGS,
    PLAYER_DEATHS,
    PLAYER_END
};

enum UserdefsLabel_t
{
    USERDEFS_GOD,
    USERDEFS_WARP_ON,
    USERDEFS_CASHMAN,
    USERDEFS_EOG,
    USERDEFS_SHOWALLMAP,
    USERDEFS_SHOW_HELP,
    USERDEFS_SCROLLMODE,
    USERDEFS_CLIPPING,
    USERDEFS_USER_NAME,
    USERDEFS_RIDECULE,
    USERDEFS_SAVEGAME,
    USERDEFS_PWLOCKOUT,
    USERDEFS_RTSNAME,
    USERDEFS_OVERHEAD_ON,
    USERDEFS_LAST_OVERHEAD,
    USERDEFS_SHOWWEAPONS,
    USERDEFS_PAUSE_ON,
    USERDEFS_FROM_BONUS,
    USERDEFS_CAMERASPRITE,
    USERDEFS_LAST_CAMSPRITE,
    USERDEFS_LAST_LEVEL,
    USERDEFS_SECRETLEVEL,
    USERDEFS_CONST_VISIBILITY,
    USERDEFS_UW_FRAMERATE,
    USERDEFS_CAMERA_TIME,
    USERDEFS_FOLFVEL,
    USERDEFS_FOLAVEL,
    USERDEFS_FOLX,
    USERDEFS_FOLY,
    USERDEFS_FOLA,
    USERDEFS_RECCNT,
    USERDEFS_ENTERED_NAME,
    USERDEFS_SCREEN_TILTING,
    USERDEFS_SHADOWS,
    USERDEFS_FTA_ON,
    USERDEFS_EXECUTIONS,
    USERDEFS_AUTO_RUN,
    USERDEFS_COORDS,
    USERDEFS_TICKRATE,
    USERDEFS_M_COOP,
    USERDEFS_COOP,
    USERDEFS_SCREEN_SIZE,
    USERDEFS_LOCKOUT,
    USERDEFS_CROSSHAIR,
    USERDEFS_WCHOICE,
    USERDEFS_PLAYERAI,
    USERDEFS_RESPAWN_MONSTERS,
    USERDEFS_RESPAWN_ITEMS,
    USERDEFS_RESPAWN_INVENTORY,
    USERDEFS_RECSTAT,
    USERDEFS_MONSTERS_OFF,
    USERDEFS_BRIGHTNESS,
    USERDEFS_M_RESPAWN_ITEMS,
    USERDEFS_M_RESPAWN_MONSTERS,
    USERDEFS_M_RESPAWN_INVENTORY,
    USERDEFS_M_RECSTAT,
    USERDEFS_M_MONSTERS_OFF,
    USERDEFS_DETAIL,
    USERDEFS_M_FFIRE,
    USERDEFS_FFIRE,
    USERDEFS_M_PLAYER_SKILL,
    USERDEFS_M_LEVEL_NUMBER,
    USERDEFS_M_VOLUME_NUMBER,
    USERDEFS_MULTIMODE,
    USERDEFS_PLAYER_SKILL,
    USERDEFS_LEVEL_NUMBER,
    USERDEFS_VOLUME_NUMBER,
    USERDEFS_M_MARKER,
    USERDEFS_MARKER,
    USERDEFS_MOUSEFLIP,
    USERDEFS_STATUSBARSCALE,
    USERDEFS_DRAWWEAPON,
    USERDEFS_MOUSEAIMING,
    USERDEFS_WEAPONSWITCH,
    USERDEFS_DEMOCAMS,
    USERDEFS_COLOR,
    USERDEFS_MSGDISPTIME,
    USERDEFS_STATUSBARMODE,
    USERDEFS_M_NOEXITS,
    USERDEFS_NOEXITS,
    USERDEFS_AUTOVOTE,
    USERDEFS_AUTOMSG,
    USERDEFS_IDPLAYERS,
    USERDEFS_TEAM,
    USERDEFS_VIEWBOB,
    USERDEFS_WEAPONSWAY,
    USERDEFS_ANGLEINTERPOLATION,
    USERDEFS_OBITUARIES,
    USERDEFS_LEVELSTATS,
    USERDEFS_CROSSHAIRSCALE,
    USERDEFS_ALTHUD,
    USERDEFS_DISPLAY_BONUS_SCREEN,
    USERDEFS_SHOW_LEVEL_TEXT,
    USERDEFS_WEAPONSCALE,
    USERDEFS_TEXTSCALE,
    USERDEFS_RUNKEY_MODE,
    USERDEFS_M_ORIGIN_X,
    USERDEFS_M_ORIGIN_Y,
    USERDEFS_PLAYERBEST,
    USERDEFS_MUSICTOGGLE,
    USERDEFS_USEVOXELS,
    USERDEFS_USEHIGHTILE,
    USERDEFS_USEMODELS,
    USERDEFS_GAMETYPEFLAGS,
    USERDEFS_M_GAMETYPEFLAGS,
    USERDEFS_GLOBALFLAGS,
    USERDEFS_GLOBALGAMEFLAGS,
    USERDEFS_VM_PLAYER,
    USERDEFS_VM_SPRITE,
    USERDEFS_VM_DISTANCE,
    USERDEFS_SOUNDTOGGLE,
    USERDEFS_GAMETEXT_TRACKING,
    USERDEFS_MGAMETEXT_TRACKING,
    USERDEFS_MENUTEXT_TRACKING,
    USERDEFS_MAXSPRITESONSCREEN,
    USERDEFS_SCREENAREA_X1,
    USERDEFS_SCREENAREA_Y1,
    USERDEFS_SCREENAREA_X2,
    USERDEFS_SCREENAREA_Y2,
    USERDEFS_SCREENFADE,
    USERDEFS_END
};

enum SectorLabel_t
{
    SECTOR_WALLPTR,
    SECTOR_WALLNUM,
    SECTOR_CEILINGZ,
    SECTOR_CEILINGZGOAL,
    SECTOR_CEILINGZVEL,
    SECTOR_FLOORZ,
    SECTOR_FLOORZGOAL,
    SECTOR_FLOORZVEL,
    SECTOR_CEILINGSTAT,
    SECTOR_FLOORSTAT,
    SECTOR_CEILINGPICNUM,
    SECTOR_CEILINGSLOPE,
    SECTOR_CEILINGSHADE,
    SECTOR_CEILINGPAL,
    SECTOR_CEILINGXPANNING,
    SECTOR_CEILINGYPANNING,
    SECTOR_FLOORPICNUM,
    SECTOR_FLOORSLOPE,
    SECTOR_FLOORSHADE,
    SECTOR_FLOORPAL,
    SECTOR_FLOORXPANNING,
    SECTOR_FLOORYPANNING,
    SECTOR_VISIBILITY,
    SECTOR_FOGPAL,
    SECTOR_LOTAG,
    SECTOR_HITAG,
    SECTOR_EXTRA,
    SECTOR_CEILINGBUNCH,
    SECTOR_FLOORBUNCH,
    SECTOR_ULOTAG,
    SECTOR_UHITAG,
    SECTOR_END
};

enum WallLabel_t
{
    WALL_X,
    WALL_Y,
    WALL_POINT2,
    WALL_NEXTWALL,
    WALL_NEXTSECTOR,
    WALL_CSTAT,
    WALL_PICNUM,
    WALL_OVERPICNUM,
    WALL_SHADE,
    WALL_PAL,
    WALL_XREPEAT,
    WALL_YREPEAT,
    WALL_XPANNING,
    WALL_YPANNING,
    WALL_LOTAG,
    WALL_HITAG,
    WALL_EXTRA,
    WALL_ULOTAG,
    WALL_UHITAG,
    WALL_BLEND,
    WALL_END
};

enum ActorLabel_t
{
    ACTOR_X,
    ACTOR_Y,
    ACTOR_Z,
    ACTOR_CSTAT,
    ACTOR_PICNUM,
    ACTOR_SHADE,
    ACTOR_PAL,
    ACTOR_CLIPDIST,
    ACTOR_DETAIL,
    ACTOR_XREPEAT,
    ACTOR_YREPEAT,
    ACTOR_XOFFSET,
    ACTOR_YOFFSET,
    ACTOR_SECTNUM,
    ACTOR_STATNUM,
    ACTOR_ANG,
    ACTOR_OWNER,
    ACTOR_XVEL,
    ACTOR_YVEL,
    ACTOR_ZVEL,
    ACTOR_LOTAG,
    ACTOR_HITAG,
    ACTOR_EXTRA,
    ACTOR_HTCGG,
    ACTOR_HTPICNUM,
    ACTOR_HTANG,
    ACTOR_HTEXTRA,
    ACTOR_HTOWNER,
    ACTOR_HTMOVFLAG,
    ACTOR_HTTEMPANG,
    ACTOR_HTACTORSTAYPUT,
    ACTOR_HTDISPICNUM,
    ACTOR_HTTIMETOSLEEP,
    ACTOR_HTFLOORZ,
    ACTOR_HTCEILINGZ,
    ACTOR_HTLASTVX,
    ACTOR_HTLASTVY,
    ACTOR_HTBPOSX,
    ACTOR_HTBPOSY,
    ACTOR_HTBPOSZ,
    ACTOR_HTG_T,
    ACTOR_ANGOFF,
    ACTOR_PITCH,
    ACTOR_ROLL,
    ACTOR_MDXOFF,
    ACTOR_MDYOFF,
    ACTOR_MDZOFF,
    ACTOR_MDFLAGS,
    ACTOR_XPANNING,
    ACTOR_YPANNING,
    ACTOR_HTFLAGS,
    ACTOR_ALPHA,
    ACTOR_ULOTAG,
    ACTOR_UHITAG,
    ACTOR_ISVALID,
    ACTOR_END
};

enum InputLabel_t
{
    INPUT_AVEL,
    INPUT_HORZ,
    INPUT_FVEL,
    INPUT_SVEL,
    INPUT_BITS,
    INPUT_EXTBITS,
    INPUT_END
};

enum TileDataLabel_t
{
    TILEDATA_XSIZE,
    TILEDATA_YSIZE,
    TILEDATA_ANIMFRAMES,
    TILEDATA_XOFFSET,
    TILEDATA_YOFFSET,
    TILEDATA_ANIMSPEED,
    TILEDATA_ANIMTYPE,
    TILEDATA_GAMEFLAGS,
    TILEDATA_END
};

enum PalDataLabel_t
{
    PALDATA_NOFLOORPAL,
    PALDATA_DUMMY, // so the hash table is size 1. remove when another member is added.
    PALDATA_END
};

#endif
// KEEPINSYNC lunatic/con_lang.lua
enum ProjectileLabel_t
{
    PROJ_WORKSLIKE,  // 0
    PROJ_SPAWNS,
    PROJ_SXREPEAT,
    PROJ_SYREPEAT,
    PROJ_SOUND,
    PROJ_ISOUND,  // 5
    PROJ_VEL,
    PROJ_EXTRA,
    PROJ_DECAL,
    PROJ_TRAIL,
    PROJ_TXREPEAT,  // 10
    PROJ_TYREPEAT,
    PROJ_TOFFSET,
    PROJ_TNUM,
    PROJ_DROP,
    PROJ_CSTAT,  // 15
    PROJ_CLIPDIST,
    PROJ_SHADE,
    PROJ_XREPEAT,
    PROJ_YREPEAT,
    PROJ_PAL,  // 20
    PROJ_EXTRA_RAND,
    PROJ_HITRADIUS,
    PROJ_MOVECNT,
    PROJ_OFFSET,
    PROJ_BOUNCES,  // 25
    PROJ_BSOUND,
    PROJ_RANGE,
    PROJ_FLASH_COLOR,
    PROJ_USERDATA,
    PROJ_END
};
#if !defined LUNATIC

enum IterationTypes_t
{
    ITER_ALLSPRITES,
    ITER_ALLSECTORS,
    ITER_ALLWALLS,
    ITER_ACTIVELIGHTS,
    ITER_DRAWNSPRITES,
    // ---
    ITER_SPRITESOFSECTOR,
    ITER_SPRITESOFSTATUS,
    ITER_WALLSOFSECTOR,
    ITER_LOOPOFWALL,
    ITER_RANGE,
    ITER_END
};

enum ScriptKeywords_t
{
    CON_ELSE,               // 0
    CON_ACTOR,              // 1
    CON_ADDAMMO,            // 2
    CON_IFRND,              // 3
    CON_ENDA,               // 4
    CON_IFCANSEE,           // 5
    CON_IFHITWEAPON,        // 6
    CON_ACTION,             // 7
    CON_IFPDISTL,           // 8
    CON_IFPDISTG,           // 9
    CON_DEFINELEVELNAME,    // 10
    CON_STRENGTH,           // 11
    CON_BREAK,              // 12
    CON_SHOOT,              // 13
    CON_PALFROM,            // 14
    CON_SOUND,              // 15
    CON_FALL,               // 16
    CON_STATE,              // 17
    CON_ENDS,               // 18
    CON_DEFINE,             // 19
    CON_RETURN,             // 20
    CON_IFAI,               // 21
    CON_KILLIT,             // 22
    CON_ADDWEAPON,          // 23
    CON_AI,                 // 24
    CON_ADDPHEALTH,         // 25
    CON_IFDEAD,             // 26
    CON_IFSQUISHED,         // 27
    CON_SIZETO,             // 28
    CON_LEFTBRACE,          // 29
    CON_RIGHTBRACE,         // 30
    CON_SPAWN,              // 31
    CON_MOVE,               // 32
    CON_IFWASWEAPON,        // 33
    CON_IFACTION,           // 34
    CON_IFACTIONCOUNT,      // 35
    CON_RESETACTIONCOUNT,   // 36
    CON_DEBRIS,             // 37
    CON_PSTOMP,             // 38
    CON_BLOCKCOMMENT,       // 39 deprecated
    CON_CSTAT,              // 40
    CON_IFMOVE,             // 41
    CON_RESETPLAYER,        // 42
    CON_IFONWATER,          // 43
    CON_IFINWATER,          // 44
    CON_IFCANSHOOTTARGET,   // 45
    CON_IFCOUNT,            // 46
    CON_RESETCOUNT,         // 47
    CON_ADDINVENTORY,       // 48
    CON_IFACTORNOTSTAYPUT,  // 49
    CON_HITRADIUS,          // 50
    CON_IFP,                // 51
    CON_COUNT,              // 52
    CON_IFACTOR,            // 53
    CON_MUSIC,              // 54
    CON_INCLUDE,            // 55
    CON_IFSTRENGTH,         // 56
    CON_DEFINESOUND,        // 57
    CON_GUTS,               // 58
    CON_IFSPAWNEDBY,        // 59
    CON_GAMESTARTUP,        // 60
    CON_WACKPLAYER,         // 61
    CON_IFGAPZL,            // 62
    CON_IFHITSPACE,         // 63
    CON_IFOUTSIDE,          // 64
    CON_IFMULTIPLAYER,      // 65
    CON_OPERATE,            // 66
    CON_IFINSPACE,          // 67
    CON_DEBUG,              // 68
    CON_ENDOFGAME,          // 69
    CON_IFBULLETNEAR,       // 70
    CON_IFRESPAWN,          // 71
    CON_IFFLOORDISTL,       // 72
    CON_IFCEILINGDISTL,     // 73
    CON_SPRITEPAL,          // 74
    CON_IFPINVENTORY,       // 75
    CON_BETANAME,           // 76
    CON_CACTOR,             // 77
    CON_IFPHEALTHL,         // 78
    CON_DEFINEQUOTE,        // 79
    CON_QUOTE,              // 80
    CON_IFINOUTERSPACE,     // 81
    CON_IFNOTMOVING,        // 82
    CON_RESPAWNHITAG,       // 83
    CON_TIP,                // 84
    CON_IFSPRITEPAL,        // 85
    CON_MONEY,              // 86
    CON_SOUNDONCE,          // 87
    CON_ADDKILLS,           // 88
    CON_STOPSOUND,          // 89
    CON_IFAWAYFROMWALL,     // 90
    CON_IFCANSEETARGET,     // 91
    CON_GLOBALSOUND,        // 92
    CON_LOTSOFGLASS,        // 93
    CON_IFGOTWEAPONCE,      // 94
    CON_GETLASTPAL,         // 95
    CON_PKICK,              // 96
    CON_MIKESND,            // 97
    CON_USERACTOR,          // 98
    CON_SIZEAT,             // 99
    CON_ADDSTRENGTH,        // 100
    CON_CSTATOR,            // 101
    CON_MAIL,               // 102
    CON_PAPER,              // 103
    CON_TOSSWEAPON,         // 104
    CON_SLEEPTIME,          // 105
    CON_NULLOP,             // 106
    CON_DEFINEVOLUMENAME,   // 107
    CON_DEFINESKILLNAME,    // 108
    CON_IFNOSOUNDS,         // 109
    CON_CLIPDIST,           // 110
    CON_IFANGDIFFL,         // 111
    CON_GAMEVAR,            // 112
    CON_IFVARL,             // 113
    CON_IFVARG,             // 114
    CON_SETVARVAR,          // 115
    CON_SETVAR,             // 116
    CON_ADDVARVAR,          // 117
    CON_ADDVAR,             // 118
    CON_IFVARVARL,          // 119
    CON_IFVARVARG,          // 120
    CON_ADDLOGVAR,          // 121
    CON_ADDLOG,             // 122
    CON_ONEVENT,            // 123
    CON_ENDEVENT,           // 124
    CON_IFVARE,             // 125
    CON_IFVARVARE,          // 126
    CON_SPGETLOTAG,         // 127
    CON_SPGETHITAG,         // 128
    CON_SECTGETLOTAG,       // 129
    CON_SECTGETHITAG,       // 130
    CON_IFSOUND,            // 131
    CON_GETTEXTUREFLOOR,    // 132
    CON_GETTEXTURECEILING,  // 133
    CON_INITTIMER,          // 134
    CON_STARTTRACK,         // 135
    CON_RANDVAR,            // 136
    CON_ENHANCED,           // 137
    CON_GETANGLETOTARGET,   // 138
    CON_GETACTORANGLE,      // 139
    CON_SETACTORANGLE,      // 140
    CON_MULVAR,             // 141
    CON_MULVARVAR,          // 142
    CON_DIVVAR,             // 143
    CON_DIVVARVAR,          // 144
    CON_MODVAR,             // 145
    CON_MODVARVAR,          // 146
    CON_ANDVAR,             // 147
    CON_ANDVARVAR,          // 148
    CON_ORVAR,              // 149
    CON_ORVARVAR,           // 150
    CON_GETPLAYERANGLE,     // 151
    CON_SETPLAYERANGLE,     // 152
    CON_LOCKPLAYER,         // 153
    CON_SETSECTOR,          // 154
    CON_GETSECTOR,          // 155
    CON_SETACTOR,           // 156
    CON_GETACTOR,           // 157
    CON_SETWALL,            // 158
    CON_GETWALL,            // 159
    CON_FINDNEARACTOR,      // 160
    CON_FINDNEARACTORVAR,   // 161
    CON_SETACTORVAR,        // 162
    CON_GETACTORVAR,        // 163
    CON_ESPAWN,             // 164
    CON_GETPLAYER,          // 165
    CON_SETPLAYER,          // 166
    CON_SQRT,               // 167
    CON_EVENTLOADACTOR,     // 168
    CON_ESPAWNVAR,          // 169
    CON_GETUSERDEF,         // 170
    CON_SETUSERDEF,         // 171
    CON_SUBVARVAR,          // 172
    CON_SUBVAR,             // 173
    CON_IFVARN,             // 174
    CON_IFVARVARN,          // 175
    CON_IFVARAND,           // 176
    CON_IFVARVARAND,        // 177
    CON_MYOS,               // 178
    CON_MYOSPAL,            // 179
    CON_DISPLAYRAND,        // 180
    CON_SIN,                // 181
    CON_XORVARVAR,          // 182
    CON_XORVAR,             // 183
    CON_RANDVARVAR,         // 184
    CON_MYOSX,              // 185
    CON_MYOSPALX,           // 186
    CON_GMAXAMMO,           // 187
    CON_SMAXAMMO,           // 188
    CON_STARTLEVEL,         // 189
    CON_ESHOOT,             // 190
    CON_QSPAWN,             // 191
    CON_ROTATESPRITE,       // 192
    CON_DEFINEPROJECTILE,   // 193
    CON_SPRITESHADOW,       // 194
    CON_COS,                // 195
    CON_ESHOOTVAR,          // 196
    CON_FINDNEARACTOR3D,    // 197
    CON_FINDNEARACTOR3DVAR, // 198
    CON_FLASH,              // 199
    CON_QSPAWNVAR,          // 200
    CON_EQSPAWN,            // 201
    CON_EQSPAWNVAR,         // 202
    CON_MINITEXT,           // 203
    CON_GAMETEXT,           // 204
    CON_DIGITALNUMBER,      // 205
    CON_ADDWEAPONVAR,       // 206
    CON_SETPROJECTILE,      // 207
    CON_ANGOFF,             // 208
    CON_UPDATESECTOR,       // 209
    CON_INSERTSPRITEQ,      // 210
    CON_ANGOFFVAR,          // 211
    CON_WHILEVARN,          // 212
    CON_SWITCH,             // 213
    CON_CASE,               // 214
    CON_DEFAULT,            // 215
    CON_ENDSWITCH,          // 216
    CON_SHOOTVAR,           // 217
    CON_SOUNDVAR,           // 218
    CON_FINDPLAYER,         // 219
    CON_FINDOTHERPLAYER,    // 220
    CON_ACTIVATEBYSECTOR,   // 221
    CON_OPERATESECTORS,     // 222
    CON_OPERATERESPAWNS,    // 223
    CON_OPERATEACTIVATORS,  // 224
    CON_OPERATEMASTERSWITCHES,  // 225
    CON_CHECKACTIVATORMOTION,   // 226
    CON_ZSHOOT,             // 227
    CON_DIST,               // 228
    CON_LDIST,              // 229
    CON_SHIFTVARL,          // 230
    CON_SHIFTVARR,          // 231
    CON_SPRITENVG,          // 232
    CON_GETANGLE,           // 233
    CON_WHILEVARVARN,       // 234
    CON_HITSCAN,            // 235
    CON_TIME,               // 236
    CON_GETPLAYERVAR,       // 237
    CON_SETPLAYERVAR,       // 238
    CON_MULSCALE,           // 239
    CON_SETASPECT,          // 240
    CON_EZSHOOT,            // 241
    CON_SPRITENOSHADE,      // 242
    CON_MOVESPRITE,         // 243
    CON_CHECKAVAILWEAPON,   // 244
    CON_SOUNDONCEVAR,       // 245
    CON_UPDATESECTORZ,      // 246
    CON_STOPALLSOUNDS,      // 247
    CON_SSP,                // 248
    CON_STOPSOUNDVAR,       // 249
    CON_DISPLAYRANDVAR,     // 250
    CON_DISPLAYRANDVARVAR,  // 251
    CON_CHECKAVAILINVEN,    // 252
    CON_GLOBALSOUNDVAR,     // 253
    CON_GUNIQHUDID,         // 254
    CON_GETPROJECTILE,      // 255
    CON_GETTHISPROJECTILE,  // 256
    CON_SETTHISPROJECTILE,  // 257
    CON_DEFINECHEAT,        // 258
    CON_CHEATKEYS,          // 259
    CON_USERQUOTE,          // 260
    CON_PRECACHE,           // 261
    CON_DEFINEGAMEFUNCNAME, // 262
    CON_REDEFINEQUOTE,      // 263
    CON_QSPRINTF,           // 264
    CON_GETPNAME,           // 265
    CON_QSTRCAT,            // 266
    CON_QSTRCPY,            // 267
    CON_SETSPRITE,          // 268
    CON_ROTATEPOINT,        // 269
    CON_DRAGPOINT,          // 270
    CON_GETZRANGE,          // 271
    CON_CHANGESPRITESTAT,   // 272
    CON_GETCEILZOFSLOPE,    // 273
    CON_GETFLORZOFSLOPE,    // 274
    CON_NEARTAG,            // 275
    CON_DEFINEGAMETYPE,     // 276
    CON_CHANGESPRITESECT,   // 277
    CON_SPRITEFLAGS,        // 278
    CON_SAVEGAMEVAR,        // 279
    CON_READGAMEVAR,        // 280
    CON_FINDNEARSPRITE,     // 281
    CON_FINDNEARSPRITEVAR,  // 282
    CON_FINDNEARSPRITE3D,   // 283
    CON_FINDNEARSPRITE3DVAR,// 284
    CON_DYNAMICREMAP,       // 285
    CON_SETINPUT,           // 286
    CON_GETINPUT,           // 287
    CON_SAVE,               // 288
    CON_CANSEE,             // 289
    CON_CANSEESPR,          // 290
    CON_FINDNEARACTORZ,     // 291
    CON_FINDNEARACTORZVAR,  // 292
    CON_FINDNEARSPRITEZ,    // 293
    CON_FINDNEARSPRITEZVAR, // 294
    CON_ZSHOOTVAR,          // 295
    CON_EZSHOOTVAR,         // 296
    CON_GETCURRADDRESS,     // 297
    CON_JUMP,               // 298
    CON_QSTRLEN,            // 299
    CON_GETINCANGLE,        // 300
    CON_QUAKE,              // 301
    CON_SHOWVIEW,           // 302
    CON_HEADSPRITESTAT,     // 303
    CON_PREVSPRITESTAT,     // 304
    CON_NEXTSPRITESTAT,     // 305
    CON_HEADSPRITESECT,     // 306
    CON_PREVSPRITESECT,     // 307
    CON_NEXTSPRITESECT,     // 308
    CON_GETKEYNAME,         // 309
    CON_QSUBSTR,            // 310
    CON_GAMETEXTZ,          // 311
    CON_DIGITALNUMBERZ,     // 312
    CON_SPRITENOPAL,        // 313
    CON_HITRADIUSVAR,       // 314
    CON_ROTATESPRITE16,     // 315
    CON_GAMEARRAY,          // 316
    CON_SETARRAY,           // 317
    CON_RESIZEARRAY,        // 318
    CON_WRITEARRAYTOFILE,   // 319
    CON_READARRAYFROMFILE,  // 320
    CON_STARTTRACKVAR,      // 321
    CON_QGETSYSSTR,         // 322
    CON_GETTICKS,           // 323
    CON_GETTSPR,            // 324
    CON_SETTSPR,            // 325
    CON_SAVEMAPSTATE,       // 326
    CON_LOADMAPSTATE,       // 327
    CON_CLEARMAPSTATE,      // 328
    CON_SCRIPTSIZE,         // 329
    CON_SETGAMENAME,        // 330
    CON_CMENU,              // 331
    CON_GETTIMEDATE,        // 332
    CON_ACTIVATECHEAT,      // 333
    CON_SETGAMEPALETTE,     // 334
    CON_SETDEFNAME,         // 335
    CON_SETCFGNAME,         // 336
    CON_IFVAROR,            // 337
    CON_IFVARVAROR,         // 338
    CON_IFVARXOR,           // 339
    CON_IFVARVARXOR,        // 340
    CON_IFVAREITHER,        // 341
    CON_IFVARVAREITHER,     // 342
    CON_GETARRAYSIZE,       // 343
    CON_SAVENN,             // 344
    CON_COPY,               // 345
    CON_INV,                // 346
    CON_SECTOROFWALL,       // 347
    CON_QSTRNCAT,           // 348
    CON_IFACTORSOUND,       // 349
    CON_STOPACTORSOUND,     // 350
    CON_IFCLIENT,           // 351
    CON_IFSERVER,           // 352
    CON_SECTSETINTERPOLATION, // 353
    CON_SECTCLEARINTERPOLATION, // 354
    CON_CLIPMOVE,           // 355
    CON_LINEINTERSECT,      // 356
    CON_RAYINTERSECT,       // 357
    CON_CALCHYPOTENUSE,     // 358
    CON_CLIPMOVENOSLIDE,    // 359
    CON_INCLUDEDEFAULT,     // 360
    CON_SETACTORSOUNDPITCH, // 361
    CON_ECHO,               // 362
    CON_SHOWVIEWUNBIASED,   // 363
    CON_ROTATESPRITEA,      // 364
    CON_SHADETO,            // 365
    CON_ENDOFLEVEL,         // 366
    CON_IFPLAYERSL,         // 367
    CON_ACTIVATE,           // 368
    CON_QSTRDIM,            // 369
    CON_SCREENTEXT,         // 370
    CON_DYNAMICSOUNDREMAP,  // 371
    CON_SCREENSOUND,        // 372
    CON_GETMUSICPOSITION,   // 373
    CON_SETMUSICPOSITION,   // 374
    CON_UNDEFINEVOLUME,     // 375
    CON_UNDEFINESKILL,      // 376
    CON_UNDEFINELEVEL,      // 377
    CON_STARTCUTSCENE,      // 378
    CON_IFCUTSCENE,         // 379
    CON_DEFINEVOLUMEFLAGS,  // 380
    CON_RESETPLAYERFLAGS,   // 381
    CON_APPENDEVENT,        // 382
    CON_DEFSTATE,           // 383
    CON_SHIFTVARVARL,       // 384
    CON_SHIFTVARVARR,       // 385
    CON_IFVARVARLE,         // 386
    CON_IFVARVARGE,         // 387
    CON_IFVARVARBOTH,       // 388
    CON_WHILEVARL,          // 389
    CON_WHILEVARVARL,       // 390
    CON_KLABS,              // 391
    CON_IFVARLE,            // 392
    CON_IFVARGE,            // 393
    CON_IFVARBOTH,          // 394
    CON_MOVESECTOR,         // 395
    CON_FOR,                // 396
    CON_NEXTSECTORNEIGHBORZ,// 397
    CON_CLAMP,              // 398
    CON_IFPLAYBACKON,       // 399
    CON_DIVSCALE,           // 400
    CON_SCALEVAR,           // 401
    CON_UNDEFINEGAMEFUNC,   // 402
    CON_GETCLOSESTCOL,      // 403
    CON_DRAWLINE256,        // 404
    CON_DRAWLINERGB,        // 405
    CON_STARTTRACKSLOT,     // 406
    CON_STOPALLMUSIC,       // 407
    CON_ACTORSOUND,         // 408
    CON_STARTSCREEN,        // 409
    CON_SCREENPAL,          // 410
    CON_END
};
// KEEPINSYNC with the keyword list in lunatic/con_lang.lua

#endif

#ifdef __cplusplus
}
#endif

#endif // gamedef_h_
