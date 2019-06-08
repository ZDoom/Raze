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

#include "actors.h"
#include "build.h"  // hashtable_t
#include "cheats.h"
#include "common.h"  // tokenlist
#include "player.h"  // projectile_t

#ifdef __cplusplus
extern "C" {
#endif

enum
{
    LABEL_ANY    = -1,
    LABEL_DEFINE = 1,
    LABEL_STATE  = 2,
    LABEL_ACTOR  = 4,
    LABEL_ACTION = 8,
    LABEL_AI     = 16,
    LABEL_MOVE   = 32,
    LABEL_EVENT  = 64,
};

#define LABEL_HASPARM2  1
#define LABEL_ISSTRING  2

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

extern intptr_t const * insptr;
extern void VM_ScriptInfo(intptr_t const *ptr, int range);

extern hashtable_t h_gamefuncs;

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

extern char g_scriptFileName[BMAX_PATH];

extern const uint32_t CheatFunctionFlags[];
extern const uint8_t  CheatFunctionIDs[];

extern int32_t g_errorCnt;
extern int32_t g_lineNumber;
extern int32_t g_scriptVersion;
extern int32_t g_totalLines;
extern int32_t g_warningCnt;
extern uint32_t g_scriptcrc;
extern int32_t otherp;

extern const char *EventNames[];  // MAXEVENTS

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

typedef projectile_t defaultprojectile_t;

extern defaultprojectile_t DefaultProjectile;
int32_t C_AllocQuote(int32_t qnum);
void C_InitQuotes(void);

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
void C_ReportError(int32_t iError);
void C_Compile(const char *filenam);

extern int32_t g_errorLineNum;
extern int32_t g_tw;

typedef struct {
    const char* token;
    int32_t val;
} tokenmap_t;

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
    STR_USERMAPFILENAME,
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
    PLAYER_LAST_USED_WEAPON,
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
    USERDEFS_MENUBACKGROUND,
    USERDEFS_STATUSBARFLAGS,
    USERDEFS_STATUSBARRANGE,
    USERDEFS_STATUSBARCUSTOM,
    USERDEFS_HUDONTOP,
    USERDEFS_MENU_SLIDEBARZ,
    USERDEFS_MENU_SLIDEBARMARGIN,
    USERDEFS_MENU_SLIDECURSORZ,
    USERDEFS_GLOBAL_R,
    USERDEFS_GLOBAL_G,
    USERDEFS_GLOBAL_B,
    USERDEFS_DEFAULT_VOLUME,
    USERDEFS_DEFAULT_SKILL,
    USERDEFS_MENU_SHADEDESELECTED,
    USERDEFS_MENU_SHADEDISABLED,
    USERDEFS_MENUTEXT_ZOOM,
    USERDEFS_MENUTEXT_XSPACE,
    USERDEFS_MENUTEXT_PAL,
    USERDEFS_MENUTEXT_PALSELECTED,
    USERDEFS_MENUTEXT_PALDESELECTED,
    USERDEFS_MENUTEXT_PALDISABLED,
    USERDEFS_MENUTEXT_PALSELECTED_RIGHT,
    USERDEFS_MENUTEXT_PALDESELECTED_RIGHT,
    USERDEFS_MENUTEXT_PALDISABLED_RIGHT,
    USERDEFS_GAMETEXT_ZOOM,
    USERDEFS_GAMETEXT_XSPACE,
    USERDEFS_GAMETEXT_PAL,
    USERDEFS_GAMETEXT_PALSELECTED,
    USERDEFS_GAMETEXT_PALDESELECTED,
    USERDEFS_GAMETEXT_PALDISABLED,
    USERDEFS_GAMETEXT_PALSELECTED_RIGHT,
    USERDEFS_GAMETEXT_PALDESELECTED_RIGHT,
    USERDEFS_GAMETEXT_PALDISABLED_RIGHT,
    USERDEFS_MINITEXT_ZOOM,
    USERDEFS_MINITEXT_XSPACE,
    USERDEFS_MINITEXT_TRACKING,
    USERDEFS_MINITEXT_PAL,
    USERDEFS_MINITEXT_PALSELECTED,
    USERDEFS_MINITEXT_PALDESELECTED,
    USERDEFS_MINITEXT_PALDISABLED,
    USERDEFS_MINITEXT_PALSELECTED_RIGHT,
    USERDEFS_MINITEXT_PALDESELECTED_RIGHT,
    USERDEFS_MINITEXT_PALDISABLED_RIGHT,
    USERDEFS_MENUTITLE_PAL,
    USERDEFS_SLIDEBAR_PALSELECTED,
    USERDEFS_SLIDEBAR_PALDISABLED,
    USERDEFS_USER_MAP,
    USERDEFS_M_USER_MAP,
    USERDEFS_MUSIC_EPISODE,
    USERDEFS_MUSIC_LEVEL,
    USERDEFS_SHADOW_PAL,
    USERDEFS_MENU_SCROLLBARTILENUM,
    USERDEFS_MENU_SCROLLBARZ,
    USERDEFS_MENU_SCROLLCURSORZ,
    USERDEFS_RETURN,
    USERDEFS_USERBYTEVERSION,
    USERDEFS_AUTOSAVE,
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
    INPUT_Q16AVEL,
    INPUT_HORZ,
    INPUT_Q16HORZ,
    INPUT_FVEL,
    INPUT_SVEL,
    INPUT_BITS,
    INPUT_EXTBITS,
    INPUT_END
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
    CON_COMMENT,            // 20 deprecated
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
    CON_IFNOCOVER,          // 112
    CON_IFHITTRUCK,         // 113
    CON_IFTIPCOW,           // 114
    CON_ISDRUNK,            // 115
    CON_ISEAT,              // 116
    CON_DESTROYIT,          // 117
    CON_LARRYBIRD,          // 118
    CON_STRAFELEFT,         // 119
    CON_STRAFERIGHT,        // 120
    CON_IFACTORHEALTHG,     // 121
    CON_IFACTORHEALTHL,     // 122
    CON_SLAPPLAYER,         // 123
    CON_IFPDRUNK,           // 124
    CON_TEARITUP,           // 125
    CON_SMACKBUBBA,         // 126
    CON_SOUNDTAGONCE,       // 127
    CON_SOUNDTAG,           // 128
    CON_IFSOUNDID,          // 129
    CON_IFSOUNDDIST,        // 130
    CON_IFONMUD,            // 131
    CON_IFCOOP,             // 132
    CON_IFMOTOFAST,         // 133
    CON_IFWIND,             // 134
    CON_SMACKSPRITE,        // 135
    CON_IFONMOTO,           // 136
    CON_IFONBOAT,           // 137
    CON_FAKEBUBBA,          // 138
    CON_MAMATRIGGER,        // 139
    CON_MAMASPAWN,          // 140
    CON_MAMAQUAKE,          // 141
    CON_MAMAEND,            // 142
    CON_NEWPIC,             // 143
    CON_GARYBANJO,          // 144
    CON_MOTOLOOPSND,        // 145
    CON_IFSIZEDOWN,         // 146
    CON_RNDMOVE,            // 147
    CON_END
};
// KEEPINSYNC with the keyword list in lunatic/con_lang.lua

#ifdef __cplusplus
}
#endif

#endif // gamedef_h_
