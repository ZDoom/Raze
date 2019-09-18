-- INTERNAL
-- definitions of BUILD and game types for the Lunatic Interpreter

local require = require
local ffi = require("ffi")
local ffiC = ffi.C

-- Lua C API functions.
local CF = CF

local bit = bit
local coroutine = coroutine
local string = string
local table = table
local math = math

local assert = assert
local error = error
local getfenv = getfenv
local getmetatable = getmetatable
local ipairs = ipairs
local loadstring = loadstring
local pairs = pairs
local pcall = pcall
local rawget = rawget
local rawset = rawset
local select = select
local setmetatable = setmetatable
local setfenv = setfenv
local tonumber = tonumber
local tostring = tostring
local type = type

-- Create a new module for passing stuff to other modules.
local lprivate = {}
require("package").loaded.lprivate = lprivate

require("jit.opt").start("maxmcode=10240")  -- in KiB

-- The "gv" global will provide access to C global *scalars* and safe functions.
-- NOTE: This exposes C library functions from e.g. the global C namespace, but
-- without their declarations, they will be sitting there like a stone.
local gv_ = {}
-- [key]=<boolean> forbids, [key]=<non-boolean (e.g. table, function)> overrides
local gv_access = {}

-- This is for declarations of arrays or pointers which should not be
-- accessible through the "gv" global. The "defs_common" module will
-- use this function.
--
-- Notes: do not declare multiple scalars on one line (this is bad:
-- "int32_t a, b"). Do not name array arguments (or add a space
-- between the identifier and the '[' instead).
function decl(str, ...)
    -- NOTE that the regexp also catches non-array/non-function identifiers
    -- like "user_defs ud;"
    for varname in string.gmatch(str, "([%a_][%w_]*)[[(;]") do
        if (ffiC._DEBUG_LUNATIC ~= 0) then
            print("FORBID "..varname)
        end
        gv_access[varname] = true
    end

    ffi.cdef(str, ...)
end

lprivate.decl = decl

ffi.cdef[[
enum {
    LUNATIC_CLIENT_MAPSTER32 = 0,
    LUNATIC_CLIENT_EDUKE32 = 1,

    LUNATIC_CLIENT = LUNATIC_CLIENT_EDUKE32
}
]]

-- Load the definitions common to the game's and editor's Lua interface.
local defs_c = require("defs_common")
local cansee = defs_c.cansee
local strip_const = defs_c.strip_const
local setmtonce = defs_c.setmtonce

-- Must be after loading "defs_common" which redefines "print" to use
-- OSD_Printf()
local print, printf = print, defs_c.printf


---=== EDuke32 game definitions ===---

local INV_NAMES = {
    "STEROIDS",
    "SHIELD",
    "SCUBA",
    "HOLODUKE",
    "JETPACK",
    "DUMMY1",
    "ACCESS",
    "HEATS",
    "DUMMY2",
    "FIRSTAID",
    "BOOTS",
}

local WEAPON_NAMES = {
    "KNEE",
    "PISTOL",
    "SHOTGUN",
    "CHAINGUN",
    "RPG",
    "HANDBOMB",
    "SHRINKER",
    "DEVISTATOR",
    "TRIPBOMB",
    "FREEZE",
    "HANDREMOTE",
    "GROW",
}

---- game structs ----

lprivate.GET = defs_c.conststruct(INV_NAMES)
lprivate.WEAPON = defs_c.conststruct(WEAPON_NAMES)

ffi.cdef([[
enum {
    GET_MAX = 11,
    MAX_WEAPONS = 12,
    MAXPLAYERS = 16,
    GTICSPERSEC = 30,  // The real number of movement updates per second
};
]])

ffi.cdef[[
struct action {
    int16_t startframe, numframes;
    int16_t viewtype, incval, delay;
    uint16_t flags;
};

struct move {
    int16_t hvel, vvel;
};

#pragma pack(push,1)
typedef struct { int32_t id; struct move mv; } con_move_t;
typedef struct { int32_t id; struct action ac; } con_action_t;
#pragma pack(pop)

typedef struct {
    int32_t id;
    con_action_t act;
    con_move_t mov;
    int32_t movflags;
} con_ai_t;
]]

defs_c.bitint_new_struct_type("int16_t", "SBit16")
defs_c.bitint_new_struct_type("int32_t", "SBit32")
defs_c.bitint_new_struct_type("uint32_t", "UBit32")

-- Struct template for actor_t. It already has 'const' fields (TODO: might need
-- to make more 'const'), but still has array members exposed, so is unsuited
-- for external exposure.
local ACTOR_STRUCT = [[
struct {
    const int32_t t_data[10];
    const struct move mv;
    const struct action ac;
    const uint16_t actiontics;

]]..defs_c.bitint_member("SBit32", "flags")..[[
    vec3_t bpos; //12b
    int32_t floorz,ceilingz,lastvx,lastvy; //16b
    int32_t lasttransport; //4b

    const int16_t picnum;
    int16_t ang, extra;
    const int16_t owner;
    // NOTE: not to be confused with .movflags:
]]..defs_c.bitint_member("SBit16", "_movflag")..[[
    int16_t tempang, timetosleep;

    int16_t stayputsect;
    const int16_t dispicnum;
    // Movement flags, sprite[i].hitag in C-CON.
    // XXX: more research where it was used in EDuke32's C code? (also .lotag <-> actiontics)
    // XXX: what if CON code knew of the above implementation detail?
]]..defs_c.bitint_member("UBit16", "movflags")..[[
    int16_t cgg;

    const int16_t lightId, lightcount, lightmaxrange;
    // NOTE: on 32-bit, C's lightptr+filler <=> this dummy:
    const union { intptr_t ptr; uint64_t dummy; } _light;
}
]]

local bcarray = require("bcarray")

local bcheck = require("bcheck")
local check_sector_idx, check_tile_idx = bcheck.sector_idx, bcheck.tile_idx
local check_sprite_idx = bcheck.sprite_idx
local check_weapon_idx, check_inventory_idx = bcheck.weapon_idx, bcheck.inventory_idx
local check_sound_idx = bcheck.sound_idx
local check_number = bcheck.number
local check_type = bcheck.type

bcarray.new("int16_t", 64, "loogie", "int16_x_64")  -- TODO: randomize member names
bcarray.new("int16_t", ffiC.MAX_WEAPONS, "weapon", "int16_x_MAX_WEAPONS", WEAPON_NAMES)
bcarray.new("int16_t", ffiC.GET_MAX, "inventory", "int16_x_GET_MAX", INV_NAMES)

-- NOTE: writing e.g. "ps.jetpack_on" in Lua when "ps.jetpack_on~=0" was meant
-- is probably one of the most commonly committed errors, so we make it a bool
-- type instead of uint8_t. The only issue is that if CON coders used these
-- fields to store more than just one bit, we're in trouble.
-- This will need to be documented and frozen for release.
local DUKEPLAYER_STRUCT = [[
__attribute__((packed)) struct {
    vec3_t pos, opos, vel, npos;
    vec2_t bobpos, fric;
    int32_t truefz, truecz, player_par;
    int32_t randomflamex, exitx, exity;
    int32_t runspeed, max_player_health, max_shield_amount;
    int32_t autostep, autostep_sbw;

    uint32_t interface_toggle_flag;

    int32_t pipebombControl, pipebombLifetime, pipebombLifetimeVar;
    int32_t tripbombControl, tripbombLifetime, tripbombLifetimeVar;

    int32_t zrange;
    int16_t angrange, autoaimang;

    uint16_t max_actors_killed, actors_killed;
]]..defs_c.bitint_member("UBit16", "gotweapon")..[[
    uint16_t zoom;

    int16_x_64 loogiex;
    int16_x_64 loogiey;
    int16_t sbs, sound_pitch;

    int16_t ang, oang, angvel;
    const<S> int16_t cursectnum;
    int16_t look_ang, last_extra, subweapon;
    int16_x_MAX_WEAPONS max_ammo_amount;
    int16_x_MAX_WEAPONS ammo_amount;
    int16_x_GET_MAX inv_amount;
    const<I-> int16_t wackedbyactor;
    int16_t pyoff, opyoff;

    int16_t horiz, horizoff, ohoriz, ohorizoff;
    const<I-> int16_t newowner;
    int16_t jumping_counter, airleft;
    int16_t fta;
    const<Q> int16_t ftq;
    const int16_t access_wallnum, access_spritenum;
    int16_t got_access, weapon_ang, visibility;
    int16_t somethingonplayer, on_crane;
    const int16_t i;
    const int16_t one_parallax_sectnum;
    int16_t random_club_frame, one_eighty_count;
    const<I-> int16_t dummyplayersprite;
    int16_t extra_extra8;
    int16_t actorsqu, timebeforeexit;
    const<X-> int16_t customexitsound;
    int16_t last_pissed_time;

    int16_x_MAX_WEAPONS weaprecs;
    int16_t weapon_sway, crack_time, bobcounter;

    int16_t orotscrnang, rotscrnang, dead_flag;   // JBF 20031220: added orotscrnang
    int16_t holoduke_on, pycount;
    int16_t transporter_hold;

    uint8_t max_secret_rooms, secret_rooms;
    uint8_t frag, fraggedself, quick_kick, last_quick_kick;
    uint8_t return_to_center;
    bool reloading;
    const uint8_t weapreccnt;
    uint8_t aim_mode, auto_aim, weaponswitch, movement_lock, team;
    uint8_t tipincs, hbomb_hold_delay;
    const<P> uint8_t frag_ps;
    uint8_t kickback_pic;

    uint8_t gm;
    bool on_warping_sector;
    uint8_t footprintcount, hurt_delay;
    bool hbomb_on, jumping_toggle, rapid_fire_hold, on_ground;
    // NOTE: there's array indexing with inven_icon, but always after a
    // bound check:
    uint8_t inven_icon, buttonpalette;
    bool over_shoulder_on;
    uint8_t show_empty_weapon;

    bool jetpack_on, spritebridge;
    uint8_t lastrandomspot;  // unused
    bool scuba_on;
    uint8_t footprintpal;
    bool heat_on;
    uint8_t invdisptime;

    bool holster_weapon;
    uint8_t falling_counter, footprintshade;
    uint8_t refresh_inventory;
    const<W> uint8_t last_full_weapon;

    const uint8_t toggle_key_flag;
    uint8_t knuckle_incs, knee_incs, access_incs;
    uint8_t walking_snd_toggle, palookup, hard_landing, fist_incs;

    int8_t numloogs, loogcnt;
    const int8_t scream_voice;
    const<W-> int8_t last_weapon;
    const int8_t cheat_phase;
    int8_t weapon_pos;
    const<W-> int8_t wantweaponfire;
    const<W> int8_t curr_weapon;

    const uint8_t palette;
    palette_t _pals;
    int8_t _palsfadespeed, _palsfadenext, _palsfadeprio, _padding2;

    // NOTE: In C, the struct type has no name. We only have it here to define
    // a metatype later.
    const weaponaccess_t weapon;
    const int8_t _padding;
}
]]

local PROJECTILE_STRUCT = [[
struct {
    int32_t workslike, cstat;
    int32_t hitradius, range, flashcolor;
    const int16_t spawns;
    const int16_t sound, isound;
    int16_t vel;
    const int16_t decal, trail;
    int16_t tnum, drop;
    int16_t offset, bounces;
    const int16_t bsound;
    int16_t toffset;
    int16_t extra, extra_rand;
    int8_t sxrepeat, syrepeat, txrepeat, tyrepeat;
    int8_t shade, xrepeat, yrepeat, pal;
    int8_t movecnt;
    uint8_t clipdist;
    int8_t filler[2];
    int32_t userdata;
}
]]

-- KEEPINSYNC weapondata_mt below.
local WEAPONDATA_STRUCT = "struct {"..table.concat(con_lang.wdata_members, ';').."; }"

local randgen = require("randgen")

local ma_rand = randgen.new(true)  -- initialize to "random" (time-based) seed
local ma_count = nil

local function ma_replace_array(typestr, neltstr)
    local nelts = tonumber(neltstr)
    if (nelts==nil) then
        nelts = ffiC[neltstr]
        assert(type(nelts)=="number")
    end

    local strtab = { "const ", typestr.." " }
    for i=1,nelts do
        local ch1 = 97 + (ma_rand:getu32() % 25)  -- 'a'..'z'
        strtab[i+2] = string.format("_%c%x%s", ch1, ma_count, (i<nelts) and "," or ";")
        ma_count = ma_count+1
    end

    return table.concat(strtab)
end

---=== Protection of scalars in (currently only) DukePlayer_t struct. ===---
-- This is more convenient than writing dozens of set-member methods.
local prot_scalar_chkfunc = {
    S = check_sector_idx,
    I = check_sprite_idx,

    P = bcheck.player_idx,
    W = check_weapon_idx,
    X = check_sound_idx,
    Q = bcheck.quote_idx,
}

local DukePlayer_prot_allowneg = {}  -- [<member name>] = true if setting <0 allowed
local DukePlayer_prot_chkfunc = {}  -- [<member name>] = <checking function>

local function ma_replace_scalar(what, typestr, membname)
    DukePlayer_prot_chkfunc[membname] = assert(prot_scalar_chkfunc[what:sub(1,1)])
    DukePlayer_prot_allowneg[membname] = (what:sub(2)=="-")
    return ma_replace_array(typestr, 1)
end

-- Converts a template struct definition to an external one, in which arrays
-- have been substituted by randomly named scalar fields.
-- <also_scalars>: also handle protected scalars like "const<W-> ..." etc.
local function mangle_arrays(structstr, also_scalars)
    ma_count = 0
    -- NOTE: regexp only works for non-nested arrays and for one array per line.
    structstr = structstr:gsub("const%s+([%w_]+)[^\n]+%[([%w_]+)%];", ma_replace_array)

    if (also_scalars) then
        -- One protected scalar per line, too.
        structstr = structstr:gsub("const<(.-)>%s+([%w_]-)%s+([%w_]-);", ma_replace_scalar)
    end

    return structstr
end

--print(mangle_arrays(DUKEPLAYER_STRUCT, true))

--- default defines etc.
local con_lang = require("con_lang")
local xmath = require("xmath")

ffi.cdef([[
typedef struct { int32_t _p; } weaponaccess_t;

typedef struct {
]]..defs_c.bitint_member("UBit32", "bits")..[[
    int16_t fvel, svel, avel;
    int8_t horz, extbits;
} input_t;

typedef
]].. mangle_arrays(ACTOR_STRUCT) ..[[
actor_t;

typedef
]].. mangle_arrays(DUKEPLAYER_STRUCT, true) ..[[
DukePlayer_t;

typedef __attribute__((packed)) struct {
    DukePlayer_t *ps;
    input_t *sync;

    int32_t netsynctime;
    int16_t ping, filler;
    int32_t pcolor, pteam;
    uint8_t frags[MAXPLAYERS], wchoice[MAX_WEAPONS];

    char vote, gotvote, pingcnt, playerquitflag, ready;
    char user_name[32];
    uint32_t revision;
} playerdata_t;

typedef struct {
    int32_t cur, count;
    int32_t gunposx, lookhalfang;
    int32_t gunposy, lookhoriz;
    int32_t shade;
} hudweapon_t;

typedef
]].. WEAPONDATA_STRUCT ..[[
weapondata_t;

typedef
]].. PROJECTILE_STRUCT ..[[
projectile_t;

typedef struct {
    uint32_t _flags;  // XXX: do we want to have this accessible at game time?
    int32_t _cacherange;
    projectile_t *_proj;
    const projectile_t *_defproj;
} tiledata_t;

typedef struct {
    vec3_t pos;
    int32_t dist, clock;
    int16_t ang, horiz;
    int16_t sect;  // NOTE: protected in camera_mt's __newindex
} camera_t;

enum
{
    MAXMOUSEBUTTONS = 10,
    MAXMOUSEAXES = 2,
    MAXJOYBUTTONS = 32,
    MAXJOYBUTTONSANDHATS = (32+4),
    MAXJOYAXES = 9,

    NUMGAMEFUNCTIONS = 56,

    // game.h
    MAXRIDECULE = 10,
    MAXRIDECULELENGTH = 40,
    MAXSAVEGAMENAME = 22,
    MAXPWLOCKOUT = 128,
    MAXRTSNAME = 128,
};

typedef struct {
    int32_t const_visibility,uw_framerate;
    int32_t camera_time,folfvel,folavel,folx,foly,fola;
    int32_t reccnt,crosshairscale;

    int32_t runkey_mode,statusbarscale,mouseaiming,weaponswitch,drawweapon;   // JBF 20031125
    int32_t democams,color,msgdisptime,statusbarmode;
    int32_t m_noexits,noexits,autovote,automsg,idplayers;
    int32_t team, viewbob, weaponsway, althud, weaponscale, textscale;

    int32_t entered_name,screen_tilting,shadows,fta_on,executions,auto_run;
    int32_t coords,tickrate,levelstats,m_coop,coop,screen_size,lockout,crosshair;
    int32_t playerai,angleinterpolation,obituaries;

    int32_t respawn_monsters,respawn_items,respawn_inventory,recstat,monsters_off,brightness;
    int32_t m_respawn_items,m_respawn_monsters,m_respawn_inventory,m_recstat,m_monsters_off,detail;
    int32_t m_ffire,ffire,m_player_skill,m_level_number,m_volume_number,multimode;
    int32_t player_skill,level_number,volume_number,m_marker,marker,mouseflip;

    vec2_t m_origin;
    int32_t playerbest;

    int32_t configversion, bgstretch;

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

        const int32_t scripthandle;
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
} user_defs;

typedef struct {
    int32_t partime, designertime;
    char *name, *filename, *musicfn;
    void *savedstate;
} map_t;
]])

bcarray.new("weapondata_t", ffiC.MAX_WEAPONS, "weapon", "weapondata_x_MAX_WEAPONS", WEAPON_NAMES)
bcarray.new("int32_t", con_lang.MAXSESSIONVARS, "sessionvar", "int32_x_MAXSESSIONVARS")

-- EXTERNALLY EXPOSED GAME VARIABLES
ffi.cdef[[
const int32_t screenpeek;
hudweapon_t hudweap;
int32_t g_logoFlags;
]]

-- INTERNAL VARIABLES/FUNCTIONS
decl("map_t g_mapInfo[$*$];", con_lang.MAXVOLUMES+1, con_lang.MAXLEVELS)
decl("char g_volumeNames[$][33];", con_lang.MAXVOLUMES)

decl[[
const int32_t myconnectindex;
int32_t g_RETURN;
int32_t g_elCONSize;
char *g_elCON;
void El_SetCON(const char *conluacode);
void El_OnError(const char *str);

char *g_elSavecode;
void El_FreeSaveCode(void);
const char *(*El_SerializeGamevars)(int32_t *slenptr, int32_t levelnum);
int32_t (*El_RestoreGamevars)(const char *savecode);
int32_t (*El_GetLabelValue)(const char *label);

const char *s_buildRev;
const char *g_sizes_of_what[];
int32_t g_sizes_of[];
int32_t g_elFirstTime;
int32_t g_elCallDepth;
int32_t block_deletesprite;
const char **g_elModules;
char g_modDir[];
int32_x_MAXSESSIONVARS g_elSessionVar;
actor_t actor[MAXSPRITES];
camera_t g_camera;
user_defs ud;
playerdata_t *const g_player;
DukePlayer_t *g_player_ps[MAXPLAYERS];
weapondata_x_MAX_WEAPONS g_playerWeapon[MAXPLAYERS];
weapondata_t g_weaponOverridden[MAX_WEAPONS];
int16_t WeaponPickupSprites[MAX_WEAPONS];
tiledata_t g_tile[MAXTILES];
projectile_t SpriteProjectile[MAXSPRITES];

int32_t g_noResetVars;
void (*A_ResetVars)(int32_t iActor);

// Used from lunacon.lua for dynamic {tile,sound} remapping:
struct
{
    const char *str;
    int32_t *dynvalptr;
    const int16_t staticval;
} g_dynTileList[], g_dynSoundList[];

char *apStrings[];

const int32_t g_mostConcurrentPlayers;
int16_t g_deleteQueueSize;
int16_t g_blimpSpawnItems[15];
int32_t g_scriptVersion;
const int32_t g_frameRate;
const int32_t g_currentMenu;
uint16_t g_earthquakeTime;
uint32_t g_moveThingsCount;
char CheatKeys[2];

// Must not have functions here that may call events directly or
// indirectly. See lunatic_game.c.

int32_t A_IncurDamage(int32_t sn);  // not bound-checked!
int32_t G_CheckActivatorMotion(int32_t lotag);
int32_t A_Dodge(spritetype *s);
int32_t A_MoveSpriteClipdist(int32_t spritenum, const vec3_t *change, uint32_t cliptype, int32_t clipdist);
void P_DoQuote(int32_t q, DukePlayer_t *p);
void P_SetGamePalette(DukePlayer_t *player, uint32_t palid, int32_t set);
void G_AddUserQuote(const char *daquote);
void G_ClearCameraView(DukePlayer_t *ps);
void VM_DrawTileGeneric(int32_t x, int32_t y, int32_t zoom, int32_t tilenum,
                       int32_t shade, int32_t orientation, int32_t p);
void G_InitTimer(int32_t ticspersec);
void G_GetTimeDate(int32_t *vals);
int32_t G_ToggleWallInterpolation(int32_t w, int32_t doset);
int32_t G_StartTrack(int32_t level);
int32_t VM_CheckSquished2(int32_t i, int32_t snum);
void Menu_Change(int32_t cm);

const char *KB_ScanCodeToString(uint8_t scancode);

int32_t A_CheckAnySoundPlaying(int32_t i);
int32_t S_CheckSoundPlaying(int32_t i, int32_t num);
void S_StopEnvSound(int32_t num, int32_t i);
void S_StopAllSounds(void);
void S_ChangeSoundPitch(int32_t num, int32_t i, int32_t pitchoffset);
int32_t S_GetMusicPosition(void);
void S_SetMusicPosition(int32_t position);

int32_t minitext_(int32_t x,int32_t y,const char *t,int32_t s,int32_t p,int32_t sb);
void G_DrawTXDigiNumZ(int32_t starttile, int32_t x,int32_t y,int32_t n,int32_t s,int32_t pal,
                      int32_t cs,int32_t x1, int32_t y1, int32_t x2, int32_t y2, int32_t z);
void G_PrintGameText(int32_t tile, int32_t x, int32_t y, const char *t,
                     int32_t s, int32_t p, int32_t o,
                     int32_t x1, int32_t y1, int32_t x2, int32_t y2,
                     int32_t z, int32_t a);
vec2_t G_ScreenText(const int32_t font,
                    int32_t x, int32_t y, const int32_t z, const int32_t blockangle, const int32_t charangle,
                    const char *str, const int32_t shade, int32_t pal, int32_t o, int32_t alpha,
                    int32_t xspace, int32_t yline, int32_t xbetween, int32_t ybetween, const int32_t f,
                    const int32_t x1, const int32_t y1, const int32_t x2, const int32_t y2);
vec2_t G_ScreenTextSize(const int32_t font,
                        int32_t x, int32_t y, const int32_t z, const int32_t blockangle,
                        const char *str, const int32_t o,
                        int32_t xspace, int32_t yline, int32_t xbetween, int32_t ybetween,
                        const int32_t f,
                        int32_t x1, int32_t y1, int32_t x2, int32_t y2);
const char* G_PrintYourTime(void);
const char* G_PrintParTime(void);
const char* G_PrintDesignerTime(void);
const char* G_PrintBestTime(void);

void G_UpdateScreenArea(void);
void G_SaveMapState(void);
void G_RestoreMapState(void);
void G_FreeMapState(int32_t mapnum);
]]

decl[[
int32_t kopen4loadfrommod(const char *filename, char searchfirst);

char **g_scriptModules;
int32_t g_scriptModulesNum;

const char *G_ConFile(void);
void G_DoGameStartup(const int32_t *params);
int32_t C_DefineSound(int32_t sndidx, const char *fn, int32_t args [5]);
void C_DefineMusic(int32_t vol, int32_t lev, const char *fn);
void C_DefineQuote(int32_t qnum, const char *qstr);
void C_DefineVolumeName(int32_t vol, const char *name);
void C_DefineVolumeFlags(int32_t vol, int32_t flags);
void C_UndefineVolume(int32_t vol);
void C_DefineSkillName(int32_t skill, const char *name);
void C_UndefineSkill(int32_t skill);
void C_DefineLevelName(int32_t vol, int32_t lev, const char *fn,
                       int32_t partime, int32_t designertime,
                       const char *levelname);
void C_UndefineLevel(int32_t vol, int32_t lev);
void C_DefineProjectile(int32_t j, int32_t what, int32_t val);
void C_DefineGameFuncName(int32_t idx, const char *name);
void C_DefineGameType(int32_t idx, int32_t flags, const char *name);
int32_t C_SetDefName(const char *name);
void C_SetCfgName(const char *cfgname);

int32_t SCRIPT_GetNumber(int32_t scripthandle, const char *sectionname, const char *entryname, int32_t *number);
void SCRIPT_PutNumber(int32_t scripthandle, const char *sectionname, const char *entryname, int32_t number,
                      int32_t hexadecimal, int32_t defaultvalue);
]]


-- http://lua-users.org/wiki/SandBoxes says "potentially unsafe"
-- as it allows to see implementations of functions.
--local string_dump = string.dump
string.dump = nil


-- sanity-check struct type sizes
local good = true
for i=0,10 do
    local what = ffi.string(ffiC.g_sizes_of_what[i])
    local fsz = ffi.sizeof(what)
    local csz = ffiC.g_sizes_of[i]
    if (ffiC._DEBUG_LUNATIC ~= 0) then
        print(i..": "..what..": C sizeof = "..tostring(csz)..", FFI sizeof = "..tostring(fsz))
    end
    if (fsz ~= csz) then
        good = false;
    end
end

if (not good) then
    error("Some sizes don't match between C and LuaJIT/FFI.")
end


--== "player" global, needed by the "control" module ==--

local player_static_members = defs_c.static_members_tab()

--[[
player_static_members._INPUT_BITS = defs_c.conststruct
{
    JUMP =                      1,
    CROUCH =                    2,
    FIRE =                      4,
    AIM_UP =                    8,
    AIM_DOWN =                 16,
    RUNNING =                  32,
    LOOK_LEFT =                64,
    LOOK_RIGHT =              128,
    -- weapons...
    STEROIDS =               4096,
    LOOK_UP =                8192,
    LOOK_DOWN =             16384,
    NIGHTVISION =           32768,
    MEDKIT =                65536,
    RESERVED =             131072,
    CENTER_VIEW =          262144,
    HOLSTER_WEAPON =       524288,
    INVENTORY_LEFT =      1048576,
    PAUSE =               2097152,
    QUICK_KICK =          4194304,
    AIM_MODE =            8388608,
    HOLODUKE =           16777216,
    JETPACK =            33554432,
    QUIT =               67108864,
    INVENTORY_RIGHT =   134217728,
    TURN_AROUND =       268435456,
    OPEN =              536870912,
    INVENTORY =        1073741824,
    ESC =              2147483648,
}

player_static_members._INPUT_EXT_BITS = defs_c.conststruct
{
    MOVE_FORWARD =  1,
    MOVE_BACKWARD = 2,
    STRAFE_LEFT =   4,
    STRAFE_RIGHT =  8,
    TURN_LEFT =    16,
    TURN_RIGHT =   32,
}
--]]

local band = bit.band
local lsh = bit.lshift
local ivec3 = xmath.ivec3

do
    -- player.all() iterator
    local function iter_allplayers(_nil, pli)
        if (pli+1 < ffiC.g_mostConcurrentPlayers) then
            return pli+1
        end
    end

    function player_static_members.all()
        return iter_allplayers, nil, -1
    end

    -- player.holdskey(pli, keyname)
    local KEYS = {  -- SK_CROUCH etc. -- "sync keys"
        CROUCH = lsh(1,1),
        RUN = lsh(1,5),
        OPEN = lsh(1,29),
    }

    function player_static_members.holdskey(pli, keyname)
        bcheck.player_idx(pli)
        if (KEYS[keyname] == nil) then
            error("invalid key name: "..tostring(keyname), 2)
        end

        return ffiC.g_player[pli].sync.bitsbits:test(KEYS[keyname])
    end
end

local player_holdskey = player_static_members.holdskey

-- Actor flags
local actor_static_members = defs_c.static_members_tab()
do
    local our_SFLAG = {}
    local ext_SFLAG = con_lang.labels[4]  -- external actor flags only
    local USER_MASK = 0

    for name, flag in pairs(ext_SFLAG) do
        our_SFLAG[name:sub(7)] = flag  -- strip "SFLAG_"
        USER_MASK = bit.bor(USER_MASK, flag)
    end

    -- Add a couple of convenience defines.
    our_SFLAG.enemy = con_lang.SFLAG.SFLAG_BADGUY
    our_SFLAG.enemystayput = con_lang.SFLAG.SFLAG_BADGUY + con_lang.SFLAG.SFLAG_BADGUYSTAYPUT
    our_SFLAG.rotfixed = con_lang.SFLAG.SFLAG_ROTFIXED

    -- Callback function chaining control flags.
    our_SFLAG.replace = 0x08000000
    our_SFLAG.replace_soft = 0x08000000  -- compat
    our_SFLAG.replace_hard = 0x08000000  -- compat, deprecated
    our_SFLAG.chain_beg = 0x20000000
    our_SFLAG.chain_end = 0x40000000
    our_SFLAG._CHAIN_MASK_ACTOR = 0x78000000
    our_SFLAG._CHAIN_MASK_EVENT = 0x68000000

    -- XXX: CON doesn't export BADGUYSTAYPUT or ROTFIXED SFLAGs, but they are considered
    -- external for Lunatic.
    our_SFLAG.USER_MASK = bit.bor(USER_MASK, our_SFLAG.enemystayput, our_SFLAG.rotfixed)

    actor_static_members.FLAGS = defs_c.conststruct(our_SFLAG)

    -- Sprite status numbers. Kept in 'actor', because it's more of a game-side
    -- concept (event though status lists are implemented in the engine), and
    -- to prevent confusion with sprite.CSTAT.
    local our_STAT = {}

    for name, statnum in pairs(con_lang.STAT) do
        -- Strip 'STAT_'.
        our_STAT[name:sub(6)] = statnum
    end

    actor_static_members.STAT = defs_c.conststruct(our_STAT)

    actor_static_members.MOVFLAGS = defs_c.conststruct
    {
        -- NOTE: no underscores, like in DEFS.CON.
        faceplayer = 1,
        geth = 2,
        getv = 4,
        randomangle = 8,
        faceplayerslow = 16,
        spin = 32,
        faceplayersmart = 64,
        fleeenemy = 128,
        jumptoplayer_only = 256,
        jumptoplayer_bits = 257,  -- NOTE: two bits set!
        jumptoplayer = 257,
        seekplayer = 512,
        furthestdir = 1024,
        dodgebullet = 4096,
    }
end

function actor_static_members.fall(i)
    check_sprite_idx(i)
    CF.VM_FallSprite(i)
end

-- actor.move(i, vec, cliptype [, clipdist])
function actor_static_members.move(i, vec, cliptype, clipdist)
    check_sprite_idx(i)
    local vel = ivec3(vec.x, vec.y, vec.z)
    return ffiC.A_MoveSpriteClipdist(i, vel, cliptype, clipdist or -1)
end

-- Delete sprite with index <i>.
function actor_static_members.delete(i)
    check_sprite_idx(i)

    if (ffiC.sprite[i].statnum == ffiC.MAXSTATUS) then
        error("Attempt to delete a sprite already not in the game world", 2)
    end

    if (ffiC.block_deletesprite ~= 0) then
        error("Attempt to delete sprite in EVENT_EGS", 2)
    end

    -- TODO_MP
    if (ffiC.g_player_ps[0].i == i) then
        error("Attempt to delete player 0's APLAYER sprite", 2)
    end

    CF.A_DeleteSprite(i)
end

local tile_static_members = defs_c.static_members_tab()
do
    tile_static_members.sizx = defs_c.creategtab_membidx(ffiC.tilesiz, "x", ffiC.MAXTILES, "tilesizx[]")
    tile_static_members.sizy = defs_c.creategtab_membidx(ffiC.tilesiz, "y", ffiC.MAXTILES, "tilesizy[]")
end

-- XXX: error message will say "g_player_ps"
player = setmtonce({}, defs_c.GenStructMetatable("g_player_ps", "g_mostConcurrentPlayers", player_static_members))

-- needed by "control"
actor = setmtonce({}, defs_c.GenStructMetatable("actor", "MAXSPRITES", actor_static_members))
-- Some bitwise NOTs of various actor flag masks.
local BNOT = {
    USER_MASK = bit.bnot(actor.FLAGS.USER_MASK),
    CHAIN_MASK_ACTOR = bit.bnot(actor.FLAGS._CHAIN_MASK_ACTOR),
    CHAIN_MASK_EVENT = bit.bnot(actor.FLAGS._CHAIN_MASK_EVENT),
}

local projectile = defs_c.creategtab_membidx_ptr(ffiC.g_tile, "_proj", ffiC.MAXTILES, "projectile")
local g_tile = setmtonce({}, defs_c.GenStructMetatable("g_tile", "MAXTILES", tile_static_members))

--== Custom operations for BUILD data structures ==--
-- Among other things, declares struct action and struct move, and their
-- ID-wrapped types con_action_t and con_move_t
local con = require("control")

do
    local isenemytile = con.isenemytile

    -- Add game-side metamethods to "spritetype" and register it with "metatype"
    local spr_mt_index_add = {
        isenemy = function(s)
            return isenemytile(s.picnum)
        end,
    }

    defs_c.finish_spritetype(spr_mt_index_add)
end

-- Check a literal numeric action or move value.
local function check_literal_am(am, typename)
    if (type(am) ~= "number") then
        error("bad argument: expected number or "..typename, 3)
    end

    -- Negative values are generated as con.action/con.move IDs.
    if (not (am >= 0 and am <= 32767)) then
        error("bad argument: expected number in [0 .. 32767]", 3)
    end
end

-- An unrestricted actor_t pointer, for internal use:
local actor_ptr_ct = ffi.typeof("$ *", ffi.typeof(strip_const(ACTOR_STRUCT)))
local player_ptr_ct = ffi.typeof("$ *", ffi.typeof(strip_const(DUKEPLAYER_STRUCT)))
local projectile_ptr_ct = ffi.typeof("$ *", ffi.typeof(strip_const(PROJECTILE_STRUCT)))
-- An unrestricted weapondata_t pointer, but with the member names stripped of
-- the leading underscore, too:
local weapondata_ptr_ct = ffi.typeof("$ *", ffi.typeof((strip_const(WEAPONDATA_STRUCT):gsub(" _"," "))))

local con_action_ct = ffi.typeof("const con_action_t")
local con_move_ct = ffi.typeof("const con_move_t")
local con_ai_ct = ffi.typeof("const con_ai_t")

-- All-zero bare action and move.
local nullac, nullmv = ffi.new("const struct action"), ffi.new("const struct move")
-- All-zero action and move with IDs. Mostly for CON support.
local literal_act = { [0]=con_action_ct(0), [1]=con_action_ct(1) }
local literal_mov = { [0]=con_move_ct(0), [1]=con_move_ct(1) }

local function get_actor_idx(a)
    local i = ffi.cast(actor_ptr_ct, a)-ffi.cast(actor_ptr_ct, ffiC.actor)
--    assert(not (i >= ffiC.MAXSPRITES+0ULL))
    return i
end

local actor_methods = {
    -- action
    set_action = function(a, act)
        a = ffi.cast(actor_ptr_ct, a)

        if (ffi.istype(con_action_ct, act)) then
            a.t_data[4] = act.id
            a.ac = act.ac
        else
            check_literal_am(act, "action")
            a.t_data[4] = act
            a.ac = nullac
        end

        a.t_data[2] = 0
        a.t_data[3] = 0
    end,

    has_action = function(a, act)
        a = ffi.cast(actor_ptr_ct, a)

        if (ffi.istype(con_action_ct, act)) then
            return (a.t_data[4]==act.id)
        else
            check_literal_am(act, "action")
            return (a.t_data[4]==act)
        end
    end,

    -- count
    set_count = function(a, count)
        ffi.cast(actor_ptr_ct, a).t_data[0] = count
    end,

    get_count = function(a)
        return ffi.cast(actor_ptr_ct, a).t_data[0]
    end,

    -- action count
    reset_acount = function(a)
        ffi.cast(actor_ptr_ct, a).t_data[2] = 0
    end,

    get_acount = function(a)
        return ffi.cast(actor_ptr_ct, a).t_data[2]
    end,

    -- Override action delay. The action ID is kept.
    set_action_delay = function(a, delay)
        ffi.cast(actor_ptr_ct, a).ac.delay = delay
    end,

    -- move
    set_move = function(a, mov, movflags)
        a = ffi.cast(actor_ptr_ct, a)

        if (ffi.istype(con_move_ct, mov)) then
            a.t_data[1] = mov.id
            a.mv = mov.mv
        else
            check_literal_am(mov, "move")
            a.t_data[1] = mov
            a.mv = nullmv
        end

        a.t_data[0] = 0
        a.movflags = movflags or 0
        local spr = ffiC.sprite[get_actor_idx(a)]

        if (not spr:isenemy() or spr.extra > 0) then
            if (bit.band(a.movflags, 8) ~= 0) then  -- random_angle
                spr.ang = bit.band(ffiC.krand(), 2047)
            end
        end
    end,

    has_move = function(a, mov)
        a = ffi.cast(actor_ptr_ct, a)

        if (ffi.istype(con_move_ct, mov)) then
            return (a.t_data[1]==mov.id)
        else
            check_literal_am(mov, "move")
            return (a.t_data[1]==mov)
        end
    end,

    -- Override velocity, keeping move ID.
    set_hvel = function(a, hvel)
        ffi.cast(actor_ptr_ct, a).mv.hvel = hvel
    end,

    set_vvel = function(a, vvel)
        ffi.cast(actor_ptr_ct, a).mv.vvel = vvel
    end,

    -- ai
    set_ai = function(a, ai)
        local oa = a
        a = ffi.cast(actor_ptr_ct, a)

        -- TODO: literal number AIs?
        if (not ffi.istype(con_ai_ct, ai)) then
            error("bad argument: expected ai", 2)
        end

        -- NOTE: compare with gameexec.c, "CON_AI:"
        a.t_data[5] = ai.id

        oa:set_action(ai.act)
        oa:set_move(ai.mov, ai.movflags)

        -- Already reset with set_move():
--        a.t_data[0] = 0
    end,

    has_ai = function(a, ai)
        a = ffi.cast(actor_ptr_ct, a)

        if (ffi.istype(con_ai_ct, ai)) then
            return (a.t_data[5]==ai.id)
        else
            check_literal_am(ai, "ai")
            return (a.t_data[5]==ai)
        end
    end,

    -- Getters/setters.
    _get_t_data = function(a, idx)
        if (not (idx >= 0 and idx < 10)) then
            error("invalid t_data index "..idx, 2)
        end
        return ffi.cast(actor_ptr_ct, a).t_data[idx]
    end,

    _set_t_data = function(a, idx, val)
        if (not (idx >= 0 and idx < 10)) then
            error("invalid t_data index "..idx, 2)
        end
        ffi.cast(actor_ptr_ct, a).t_data[idx] = val
    end,

    set_picnum = function(a, picnum)
        if (not (picnum < 0)) then
            check_tile_idx(picnum)
        end
        ffi.cast(actor_ptr_ct, a).picnum = picnum
    end,

    set_owner = function(a, owner)
        -- XXX: is it permissible to set to -1?
        check_sprite_idx(owner)
        ffi.cast(actor_ptr_ct, a).owner = owner
    end,

    --- Custom methods ---

    -- Checkers for whether the movement update made the actor hit
    -- something.

    checkhit = function(a)
        -- Check whether we hit *anything*, including ceiling/floor.
        return a._movflagbits:test(49152)
    end,

    checkbump = function(a)
        -- Check whether we bumped into a wall or sprite.
        return (a._movflagbits:mask(49152) >= 32768)
    end,

    hitwall = function(a)
        if (a._movflagbits:mask(49152) == 32768) then
            return a._movflagbits:mask(32767)
        end
    end,

    hitsprite = function(a)
        if (a._movflagbits:mask(49152) == 49152) then
            return a._movflagbits:mask(32767)
        end
    end,

    -- NOTE: no 'hitsector' or 'hitceiling' / 'hitfloor' for now because
    -- more research is needed as to what the best way of checking c/f is.
}

local actor_mt = {
    __index = function(a, key)
        if (actor_methods[key] ~= nil) then
            return actor_methods[key]
        elseif (key == "proj") then
            return ffiC.SpriteProjectile[get_actor_idx(a)]
        else
            error("invalid indexing key to actor object", 2)
        end
    end,
}

ffi.metatype("actor_t", actor_mt)


--- PER-PLAYER WEAPON SETTINGS
local wd_sound_member = {}
for _, declstr in pairs(con_lang.wdata_members) do
    local member = declstr:match("const int32_t _(.*sound)$")
    if (member) then
        wd_sound_member[member] = true
        if (ffiC._DEBUG_LUNATIC ~= 0) then
            printf("weapondata_t member %s denotes a sound", member)
        end
    end
end

local weapondata_mt = {
    __index = function(wd, member)
        -- Handle protected members that are renamed (e.g. shoots/_shoots).
        return ffi.cast(weapondata_ptr_ct, wd)[0][member]
    end,

    __newindex = function(wd, member, val)
        -- Set to 'true' if we set a tile or sound member.
        local didit = false

        check_type(member, "string")  -- MEMBER_IS_STRING
        check_number(val)

        if (wd_sound_member[member]) then  -- XXX: sound2time is a time, not a sound
            if (val < 0) then
                val = 0  -- Set to 0 if negative (e.g. CrackDown).
            else
                check_sound_idx(val)
            end
            didit = true
        elseif (member=="workslike") then
            check_weapon_idx(val)
        elseif (member=="spawn" or member=="shoots") then
            if (val < 0) then
                -- Set to 0 if oob (e.g. CrackDown). This is a bit problematic
                -- for .shoots because it's used unconditionally except in one
                -- case (see player.c).
                val = 0
            else
                check_tile_idx(val)
            end
            didit = true
        end

        -- DEBUG:
--        printf("assigning %s to weapon's %s", tostring(val), member)

        -- NOTE: we're indexing a *pointer* with the user-supplied 'member',
        -- which could be dangerouns if it could be a number. However, we have
        -- assured that is is not in MEMBER_IS_STRING above.
        ffi.cast(weapondata_ptr_ct, wd)[member] = val

        if (didit and ffiC.g_elCallDepth==0) then
            -- Signal that we overrode this member at CON translation time.

            -- Get weapon index as pointer difference first. PLAYER_0.
            local wi = ffi.cast(weapondata_ptr_ct, wd)
                     - ffi.cast(weapondata_ptr_ct, ffiC.g_playerWeapon)
            assert(wi >= 0 and wi < ffiC.MAX_WEAPONS)

            -- Set g_weaponOverridden[wi][member], but without invoking
            -- weapondata_t's __newindex metamethod (i.e. us)!
            ffi.cast(weapondata_ptr_ct, ffiC.g_weaponOverridden[wi])[member] = 1
        end
    end,
}
ffi.metatype("weapondata_t", weapondata_mt)

local weaponaccess_mt = {
    -- Syntax like "player[0].weapon.KNEE.shoots" possible because
    -- g_playerWeapon[] is declared as an array of corresponding bcarray types
    -- for us.
    __index = function(wa, key)
        if (type(key)~="number" and type(key)~="string") then
            error("must access weapon either by number or by name")
        end

        return ffiC.g_playerWeapon[wa._p][key]
    end,
}
ffi.metatype("weaponaccess_t", weaponaccess_mt)


local function clamp(num, min, max)
    return num < min and min
        or num > max and max
        or num
end

local function clamp0to1(num)
    check_number(num, 4)
    return clamp(num, 0, 1)
end

local player_methods = {
    -- CON-like addammo/addweapon, but without the non-local control flow
    -- (returns true if weapon's ammo was at the max. instead).
    addammo = con._addammo,
    addweapon = con._addweapon,

    stomp = con._pstomp,

    holdskey = function(p, keyname)
        -- XXX: on invalid <keyname>, error will point to this next line:
        return player_holdskey(p.weapon._p, keyname)
    end,

    has_weapon = function(p, weap)
        return p.gotweaponbits:test(lsh(1,weap))
    end,

    give_weapon = function(p, weap)
        p.gotweaponbits:set(lsh(1,weap))
    end,

    take_weapon = function(p, weap)
        p.gotweaponbits:clear(lsh(1,weap))
    end,

    -- Give or take weapon, for implementation of CON's .gotweapon access.
    _gt_weapon = function(p, weap, give_p)
        if (give_p ~= 0) then
            p:give_weapon(weap)
        else
            p:take_weapon(weap)
        end
    end,

    whack = function(p, no_return_to_center)
        p.horiz = p.horiz + 64
        if (not no_return_to_center) then
            p.return_to_center = 9
        end
        local n = bit.arshift(128-band(ffiC.krand(),255), 1)
        p.rotscrnang = n
        p.look_ang = n
    end,

    -- External, improved 'palfrom'.
    -- <speed>: possibly fractional speed of tint fading, in pals.f decrements per gametic.
    --          XXX: exposes internals.
    -- <prio>: a value from -128 to 127, higher ones trump lower or equal ones
    fadecol = function(p, fadefrac, r, g, b, speed, prio)
        -- Validate inargs: clamp f,r,g,b to [0 .. 1] first and multiply by
        -- 63 for the internal handling.
        fadefrac = clamp0to1(fadefrac)*63
        -- NOTE: a fadefrac of now <1 is allowed, e.g. for clearing the tint.
        r = clamp0to1(r)*63
        g = clamp0to1(g)*63
        b = clamp0to1(b)*63

        if (speed ~= nil) then
            check_number(speed)
            -- Clamp to sensible values; the speed is stored in an int8_t
            -- (see below).
            speed = clamp(speed, 1/128, 127)
        else
            speed = 1
        end

        if (prio ~= nil) then
            check_number(prio)

            if (not (prio >= -128 and prio < 127)) then
                error("invalid argument #6 (priority): must be in [-128 .. 127]", 2)
            end
        else
            prio = 0
        end

        -- Check if a currently active tint has higher priority.
        if (p._pals.f > 0 and p._palsfadeprio > prio) then
            return
        end

        -- The passed tint can be applied now.
        p:_palfrom(fadefrac, r, g, b)
        p._palsfadeprio = prio

        -- Calculate .palsfade{speed,next}
        if (speed >= 1) then
            -- Will round to the nearest integer ("banker's
            -- rounding"). NOTE: This is not correct for all numbers, see
            -- http://blog.frama-c.com/index.php?post/2013/05/02/nearbyintf1
            p._palsfadespeed = speed + 0.5
            p._palsfadenext = 0
        else
            -- NOTE: Values that round to 0 have are equivalent behavior to
            -- passing a <speed> of 1.
            local negspeedrecip = -((1/speed) + 0.5)  -- [-128.5 .. 1/127+0.5]
            p._palsfadespeed = negspeedrecip
            p._palsfadenext = negspeedrecip
        end
    end,

    -- INTERNAL and CON-only.
    _palfrom = function(p, f, r,g,b)
        local pals = p._pals
        -- Assume that CON's palfrom starts with prio 0 and speed 0.
        if (pals.f == 0 or p._palsfadeprio <= 0) then
            pals.f = f
            pals.r, pals.g, pals.b = r, g, b
            p._palsfadespeed, p._palsfadenext = 0, 0
        end
    end,
}

local player_mt = {
    __index = function(p, key)
        if (player_methods[key] ~= nil) then
            return player_methods[key]
        elseif (key == "_input") then
            return ffiC.g_player[p.weapon._p].sync[0]
        else
            -- Read access to protected player members.
            return ffi.cast(player_ptr_ct, p)[0][key]
        end
    end,

    __newindex = function(p, key, val)
        -- Write access to protected player members.

        local allowneg = DukePlayer_prot_allowneg[key]
        assert(type(allowneg)=="boolean")

        if (allowneg==false or not (val == -1)) then
            DukePlayer_prot_chkfunc[key](val)
        end
        ffi.cast(player_ptr_ct, p)[key] = val
    end,
}

ffi.metatype("DukePlayer_t", player_mt)

local function GenProjectileSetFunc(Member, checkfunc)
    return function(self, idx)
        if (not (idx == -1)) then
            checkfunc(idx)
        end
        ffi.cast(projectile_ptr_ct, self)[Member] = idx
    end
end

local projectile_mt = {
    __index = {
        set_spawns = GenProjectileSetFunc("spawns", check_tile_idx),
        set_decal = GenProjectileSetFunc("decal", check_tile_idx),
        set_trail = GenProjectileSetFunc("trail", check_tile_idx),

        set_sound = GenProjectileSetFunc("sound", check_sound_idx),
        set_isound = GenProjectileSetFunc("isound", check_sound_idx),
        set_bsound = GenProjectileSetFunc("bsound", check_sound_idx),
    },
}
ffi.metatype("projectile_t", projectile_mt)

local user_defs_mt = {
    __index = {
        set_screen_size = function(ud, screen_size)
            if (ud.screen_size ~= screen_size) then
                ud.screen_size = screen_size
                ffiC.G_UpdateScreenArea()
            end
        end,

        set_volume_number = function(ud, volume_number)
            -- NOTE: allow volume_number==MAXVOLUMES.
            if (not (volume_number==con_lang.MAXVOLUMES)) then
                bcheck.volume_idx(volume_number)
            end
            ud.volume_number = volume_number
        end,

        set_m_volume_number = function(ud, volume_number)
            -- NOTE: allow volume_number==MAXVOLUMES.
            if (not (volume_number==con_lang.MAXVOLUMES)) then
                bcheck.volume_idx(volume_number)
            end
            ud.m_volume_number = volume_number
        end,

        set_level_number = function(ud, level_number)
            bcheck.level_idx(level_number)
            ud.level_number = level_number
        end,
    },
}
ffi.metatype("user_defs", user_defs_mt)

--- CUSTOM "gv" VARIABLES
local camera_mt = {
    -- TODO: "set position" method, which also updates the sectnum
    __index = ffiC.g_camera,
    __newindex = function(_, key, val)
        if (key=="sect") then
            check_sector_idx(val)
        end
        ffiC.g_camera[key] = val
    end,
}

gv_access.cam = setmtonce({}, camera_mt)
gv_access._ud = ffiC.ud

-- Support for some CON global system gamevars. RETURN handled separately.
gv_access._csv = ffi.new "struct { int32_t LOTAG, HITAG, TEXTURE; }"

gv_access.REND = defs_c.conststruct
{
    CLASSIC = 0,
    POLYMOST = 3,
    POLYMER = 4,
}

gv_access.WEAPON = lprivate.WEAPON
gv_access.GET = lprivate.GET

function gv_access._get_yxaspect()
    return ffiC.yxaspect
end

function gv_access._get_viewingrange()
    return ffiC.viewingrange
end

function gv_access._currentFramerate()
    return ffiC.g_frameRate
end

function gv_access._currentMenu()
    return ffiC.g_currentMenu
end

function gv_access._changeMenu(cm)
    ffiC.Menu_Change(cm)
end

function gv_access._set_guniqhudid(id)
    local MAXUNIQHUDID = 256  -- KEEPINSYNC build.h
    if (not (id >= 0 and id < MAXUNIQHUDID)) then
        error("invalid unique HUD ID "..id)
    end
    ffiC.guniqhudid = id
end

function gv_access.currentEpisode()
    return ffiC.ud.volume_number + 1
end

function gv_access.currentLevel()
    return ffiC.ud.level_number + 1
end

function gv_access.doQuake(gametics, snd)
    ffiC.g_earthquakeTime = gametics
    if (snd ~= nil) then
        con._globalsound(ffiC.screenpeek, snd)
    end
end

-- Declare all con_lang.labels constants in the global FFI namespace.
for i=1,#con_lang.labels do
    if (getmetatable(con_lang.labels[i]) ~= "noffiC") then
        local strbuf = {"enum {"}
        for label, val in pairs(con_lang.labels[i]) do
            strbuf[#strbuf+1] = string.format("%s = %d,", label, val)
        end
        strbuf[#strbuf+1] = "};"

        ffi.cdef(table.concat(strbuf))
    end
end


---=== Set up restricted global environment ===---

local allowed_modules = {
    coroutine=coroutine, bit=bit, table=table, math=math, string=string,

    os = {
        clock = function() return gv_.gethiticks()*0.001 end,
    },

    randgen = randgen,
    engine = require("engine"),
    stat = require("stat"),
    bitar = require("bitar"),
    xmath = xmath,
    fs = require("fs"),

    con = con,
}

do
    local ctype_cansave = {}

    -- Register as "serializeable" the type of cdata object <v>.
    local function reg_serializable_cv(v)
        assert(type(v)=="cdata")
        assert(type(v._serialize)=="function")
        -- NOTE: tonumber() on a ctype cdata object gets its LuaJIT-internal
        -- ID, the one that would be shown with tostring(), e.g.
        -- ctype<struct 95>
        ctype_cansave[tonumber(ffi.typeof(v))] = true
    end

    function lprivate.cansave_cdata(v)
        return type(v)=="cdata" and ctype_cansave[tonumber(ffi.typeof(v))]
    end

    reg_serializable_cv(xmath.vec3())
    reg_serializable_cv(ivec3())
end

-- Protect base modules.
local function basemod_newidx()
    error("modifying base module table forbidden", 2)
end

for modname, themodule in pairs(allowed_modules) do
    local mt = {
        __index = themodule,
        __newindex = basemod_newidx,
    }

    allowed_modules[modname] = setmtonce({}, mt)
end


---=== Module stuff ===---

local package_loaded = {}  -- [<modname>] = false/true/table
local modname_stack = {}  -- [<depth>]=string
local module_gamevars = {}  -- [<modname>] = { <gvname1>, <gvname2>, ... }
local module_gvlocali = {}  -- [<modname>] = { <localidx_beg>, <localidx_end> }
local module_thread = {}  -- [<modname>] = <module_thread>

local function getcurmodname(thisfuncname)
    if (#modname_stack == 0) then
        error("'"..thisfuncname.."' must be called at the top level of a require'd file", 3)
        -- ... as opposed to "at runtime".
    end

    return modname_stack[#modname_stack]
end


local function errorf(level, fmt, ...)
    local errmsg = string.format(fmt, ...)
    error(errmsg, level+1)
end

local function readintostr_mod(fn)
    -- TODO: g_loadFromGroupOnly?
    local fd = ffiC.kopen4loadfrommod(fn, 0)
    if (fd < 0) then
        return nil
    end

    local ret = defs_c.readintostr(fd)
    ffiC.kclose(fd)
    return ret
end


local debug = require("debug")

-- Get the number of active locals in the function that calls the function
-- calling this one.
local function getnumlocals(l)
    -- 200 is the max. number of locals at one level
    for i=1,200 do
        -- level:
        -- 0 is debug.getlocal() itself.
        -- 1 is this function (getnumlocals).
        -- 2 is the function calling getnumlocals()
        -- 3 is the function calling that one.
        if (debug.getlocal(3, i) == nil) then
            return i-1
        end
    end
end

local function error_on_nil_read(_, varname)
    error("attempt to read nil variable '"..varname.."'", 2)
end

local required_module_mt = {
    __index = error_on_nil_read,

    __newindex = function()
        error("modifying module table forbidden", 2)
    end,

    __metatable = true,
}

-- Will contain a function to restore gamevars when running from savegame
-- restoration. See SAVEFUNC_ARGS for its arguments.
local g_restorefunc = nil

-- Local gamevar restoration function run from
-- our_require('end_gamevars') <- [user module].
local function restore_local(li, lval)
    -- level:
    -- 0 is getlocal() itself.
    -- 1 is this function (restore_local).
    -- 2 is the function calling restore_local(), the savecode.
    -- 3 is the function calling the savecode, our_require.
    -- 4 is the function calling our_require, the module function.
    if (ffiC._DEBUG_LUNATIC ~= 0) then
        printf("Restoring index #%d (%s) with value %s",
               li, debug.getlocal(4, li), tostring(lval))
    end

    assert(debug.setlocal(4, li, lval))
end

-- The "require" function accessible to Lunatic code.
-- Base modules in allowed_modules are wrapped so that they cannot be
-- modified, user modules are searched in the EDuke32 search
-- path.  Also, our require
--  * never messes with the global environment, it only returns the module.
--  * allows passing varargs beyond the name to the module.
local function our_require(modname, ...)
    local ERRLEV = 5

    -- Check module name is valid first.
    -- TODO: restrict valid names?
    if (type(modname) ~= "string" or #modname==0) then
        error("module name must be a nonempty string", 2)
    end

    -- For _LUNATIC_DBG
    if (modname:match("^_LUNATIC") and ffiC._DEBUG_LUNATIC == 0) then
        return nil
    end

    -- Handle the section between module() and require("end_gamevars").
    if (modname == "end_gamevars") then
        local thismodname = getcurmodname("require")

        if (module_gamevars[thismodname] ~= nil) then
            error("\"require 'end_gamevars'\" must be called at most once per require'd file", 2)
        end

        local gvnames = {}

        for name in pairs(getfenv(2)) do
            gvnames[#gvnames+1] = name
            if (ffiC._DEBUG_LUNATIC ~= 0) then
                printf("MODULE %s GAMEVAR %s", thismodname, name)
            end
        end

        module_gamevars[thismodname] = gvnames
        local gvmodi = module_gvlocali[thismodname]
        gvmodi[2] = getnumlocals()

        if (ffiC._DEBUG_LUNATIC ~= 0) then
            local numlocals = gvmodi[2]-gvmodi[1]+1
            if (numlocals > 0) then
                printf("Module '%s' has %d locals, index %d to %d",
                       thismodname, numlocals, gvmodi[1], gvmodi[2])
            end
        end

        -- Potentially restore gamevars.
        if (g_restorefunc) then
            local modtab = package_loaded[thismodname]
            assert(type(modtab)=="table")
            -- SAVEFUNC_ARGS.
            g_restorefunc(thismodname, modtab, restore_local)
        end

        -- Return whether we're NOT running from a savegame restore in the
        -- second outarg. (Lunatic-private!)
        return nil, (g_restorefunc==nil)
    end

    -- See whether it's a base module name.
    if (allowed_modules[modname] ~= nil) then
        return allowed_modules[modname]
    end

    --- Search user modules...

    if (modname:find("[/\\]")) then
        error("Module name must not contain directory separators", ERRLEV-1)
    end
    -- Instead, dots are translated to directory separators. For EDuke32's
    -- virtual file system, this is always a forward slash. Keep the original
    -- module name for passing to the module function.
    local omodname = modname
    modname = modname:gsub("%.", "/")

    local omod = package_loaded[modname]
    if (omod ~= nil) then
        if (omod==false) then
            error("Loop while loading modules", ERRLEV-1)
        end

        -- already loaded
        assert(omod==true or type(omod)=="table")
        return omod
    end

    local modfn = modname .. ".lua"
    local str = readintostr_mod(modfn)
    if (str == nil) then
        errorf(ERRLEV-1, "Couldn't open file \"%s\"", modfn)
    end

    -- Implant code that yields the module thread just before it would return
    -- otherwise.
    str = str.."\nrequire('coroutine').yield()"

    local modfunc, errmsg = loadstring(str, modfn)
    if (modfunc == nil) then
        errorf(ERRLEV-1, "Couldn't load \"%s\": %s", modname, errmsg)
    end

    package_loaded[modname] = false  -- 'not yet loaded'
    table.insert(modname_stack, modname)

    -- Run the module code in a separate Lua thread!
    local modthread = coroutine.create(modfunc)
    local ok, retval = coroutine.resume(modthread, omodname, ...)

    if (not ok) then
        errorf(ERRLEV-1, "Failed running \"%s\": %s\n%s", modname,
               retval, debug.traceback(modthread))
    end

    table.remove(modname_stack)

    local modtab = package_loaded[modname]

    if (type(modtab) ~= "table") then
        -- The module didn't call our 'module'. Check if it returned a table.
        -- In that case, the coroutine has finished its main function and has
        -- not reached our implanted 'yield'.
        if (coroutine.status(modthread)=="dead" and type(retval)=="table") then
            modtab = retval
            package_loaded[modname] = modtab
        else
            package_loaded[modname] = true
        end
    end

    if (type(modtab) == "table") then
        -- Protect module table in any case (i.e. either if the module used our
        -- 'module' or if it returned a table).
        setmetatable(modtab, required_module_mt)
    end

    local gvmodi = module_gvlocali[modname]

    if (gvmodi and gvmodi[2] and gvmodi[2]>=gvmodi[1]) then
        if (coroutine.status(modthread)=="suspended") then
            -- Save off the suspended thread so that we may get its locals later on.
            -- It is never resumed, but only ever used for debug.getlocal().
            module_thread[modname] = modthread

            if (ffiC._DEBUG_LUNATIC ~= 0) then
                printf("Keeping coroutine for module \"%s\"", modname)
            end
        end
    end

    return modtab
end


local module_mt = {
    __index = error_on_nil_read,
}

-- Our 'module' replacement doesn't get the module name from the function args
-- since a malicious user could remove other loaded modules this way.
-- Also, our 'module' takes no varargs ("option functions" in Lua).
-- TODO: make transactional?
local function our_module()
    if (#modname_stack == 0) then
        error("'module' must be called at the top level of a require'd file", 2)
        -- ... as opposed to "at runtime".
    end

    local modname = getcurmodname("module")

    if (package_loaded[modname]) then
        error("'module' must be called at most once per require'd file", 2)
    end

    local M = setmetatable({}, module_mt)
    package_loaded[modname] = M
    -- change the environment of the function which called us:
    setfenv(2, M)

    module_gvlocali[modname] = { getnumlocals()+1 }
end

-- overridden 'error' that always passes a string to the base 'error'
local function our_error(errmsg, level)
    if (type(errmsg) ~= "string") then
        error("error using 'error': error message must be a string", 2)
    end

    if (level) then
        if (type(level) ~= "number") then
            error("error using 'error': error level must be a number", 2)
        end

        error(errmsg, level==0 and 0 or level+1)
    end

    error(errmsg, 2)
end


-- _G tweaks -- pull in only 'safe' stuff
local G_ = {}  -- our soon-to-be global environment

G_.assert = assert
G_.error = our_error
G_.ipairs = ipairs
G_.pairs = pairs
G_.pcall = pcall
G_.print = print
G_.module = our_module
G_.next = next
G_.require = our_require
G_.select = select
G_.tostring = tostring
G_.tonumber = tonumber
G_.type = type
G_.unpack = unpack
G_.xpcall = xpcall
G_._VERSION = _VERSION

-- Available through our 'require':
-- bit, coroutine, math, string, table

-- Not available:
-- collectgarbage, debug, dofile, gcinfo (DEPRECATED), getfenv, getmetatable,
-- jit, load, loadfile, loadstring, newproxy (NOT STD?), package, rawequal,
-- rawget, rawset, setfenv, setmetatable

G_._G = G_

-- Chain together two functions taking 3 input args.
local function chain_func3(func1, func2)
    if (func1==nil or func2==nil) then
        return assert(func1 or func2)
    end

    -- Return a function that runs <func1> first and then tail-calls <func2>.
    return function(aci, pli, dist)
        func1(aci, pli, dist)
        return func2(aci, pli, dist)
    end
end

-- Determines the last numeric index of a table using *pairs*, so that in
-- arg-lists with "holes" (e.g. {1, 2, nil, function() end}) are handled
-- properly.
local function ourmaxn(tab)
    local maxi = 0
    for i in pairs(tab) do
        if (type(i)=="number") then
            maxi = math.max(i, maxi)
        end
    end
    assert(tab[maxi] ~= nil)
    return maxi
end

-- Running for the very first time?
local g_firstRun = (ffiC.g_elCONSize == 0)

-- Actor functions, saved for actor chaining
local actor_funcs = {}
-- Event functions, saved for event chaining
local event_funcs = {}

-- Per-actor sprite animation callbacks
local animsprite_funcs = {}

local gameactor_internal = gameactor_internal  -- included in lunatic.c
local gameevent_internal = gameevent_internal  -- included in lunatic.c

local function animate_all_sprites()
    for i=0,ffiC.spritesortcnt-1 do
        local tspr = ffiC.tsprite[i]

        if (tspr.owner < ffiC.MAXSPRITES+0ULL) then
            local spr = tspr:getspr()
            local animfunc = animsprite_funcs[spr.picnum]

            if (animfunc) then
                animfunc(tspr)
            end
        end
    end
end


local function check_arg_number(name, argpos, val)
    if (type(val) ~= "number") then
        errorf(3, "invalid '%s' argument (#%d) to gameactor: must be a number", name, argpos)
    end
end

-- gameactor{tilenum [, flags [, strength [, action [, move [, movflags]]]]], func}
-- Every arg may be positional OR key=val (with the name indicated above as key),
-- but not both.
local function our_gameactor(args)
    bcheck.top_level("gameactor")

    if (type(args)~="table") then
        error("invalid gameactor call: must be passed a table")
    end

    local tilenum = args[1]
    if (type(tilenum) ~= "number") then
        error("invalid argument #1 to gameactor: must be a number", 2)
    end
    if (not (tilenum >= 0 and tilenum < ffiC.MAXTILES)) then
        error("invalid argument #1 to gameactor: must be a tile number [0..gv.MAXTILES-1]", 2)
    end

    local lastargi = ourmaxn(args)
    local func = args[lastargi]
    if (type(func) ~= "function") then
        func = args.func
        lastargi = 1/0
    end
    if (type(func) ~= "function") then
        error("invalid gameactor call: must provide a function with last numeric arg or .func", 2)
    end

    local flags = (lastargi > 2 and args[2]) or args.flags or 0
    check_arg_number("flags", 2, flags)

    local AF = actor.FLAGS
    local chainflags = band(flags, AF._CHAIN_MASK_ACTOR)
    flags = band(flags, BNOT.CHAIN_MASK_ACTOR)

    if (chainflags == 0) then
        -- Default chaining behavior: don't, replace the old actor instead.
        chainflags = AF.replace
    elseif (band(chainflags, chainflags-1) ~= 0) then
        error("invalid chaining control flags to gameactor", 2)
    end

    local replacep = (chainflags==AF.replace)
    if (not replacep and not actor_funcs[tilenum]) then
        error("attempt to chain code to nonexistent actor tile "..tilenum, 2)
    end

    local flags_rbits = band(flags, BNOT.USER_MASK)
    if (flags_rbits ~= 0) then
        error("invalid 'flags' argument (#2) to gameactor: must not set reserved bits (0x"
              ..(bit.tohex(flags_rbits))..")", 2)
    end

    local strength = ((lastargi > 3 and args[3]) or args.strength) or (replacep and 0 or nil)
    if (replacep or strength~=nil) then
        check_arg_number("strength", 3, strength)
    end

    local act = ((lastargi > 4 and args[4]) or args.action) or (replacep and literal_act[0] or nil)
    if (replacep or act ~= nil) then
        if (type(act)=="number" and (act==0 or act==1)) then
            act = literal_act[act]
        elseif (not ffi.istype(con_action_ct, act)) then
            error("invalid 'action' argument (#4) to gameactor: must be an action", 2)
        end
    end

    local mov = ((lastargi > 5 and args[5]) or args.move) or (replacep and literal_mov[0] or nil)
    if (replacep or mov ~= nil) then
        if (type(mov)=="number" and (mov==0 or mov==1)) then
            mov = literal_mov[mov]
        elseif (not ffi.istype(con_move_ct, mov)) then
            error("invalid 'move' argument (#5) to gameactor: must be a move", 2)
        end
    end

    local movflags = ((lastargi > 6 and args[6]) or args.movflags) or (replacep and 0 or nil)
    if (replacep or movflags ~= nil) then
        check_arg_number("movflags", 6, movflags)
    end

    -- Register a potentially passed drawn sprite animation callback function.
    -- TODO: allow registering without main actor execution callback.
    local animfunc = args.animate
    if (animfunc ~= nil) then
        if (type(animfunc) ~= "function") then
            error("invalid 'animate' argument to gameactor: must be a function", 2)
        end

        animsprite_funcs[tilenum] = replacep and func
            or (chainflags==AF.chain_beg) and chain_func3(animfunc, animsprite_funcs[tilenum])
            or (chainflags==AF.chain_end) and chain_func3(animsprite_funcs[tilenum], animfunc)
            or assert(false)

        -- Register our EVENT_ANIMATEALLSPRITES only now so that it is not
        -- called if there are no 'animate' definitions.
        gameevent_internal(97, animate_all_sprites)  -- EVENT_ANIMATEALLSPRITES
    end

    -- All good, bitwise-OR the tile bits and register the actor!
    ffiC.g_tile[tilenum]._flags = bit.bor(ffiC.g_tile[tilenum]._flags, flags)

    local newfunc = replacep and func
        or (chainflags==AF.chain_beg) and chain_func3(func, actor_funcs[tilenum])
        or (chainflags==AF.chain_end) and chain_func3(actor_funcs[tilenum], func)
        or assert(false)

    gameactor_internal(tilenum, strength, act, mov, movflags, newfunc)
    actor_funcs[tilenum] = newfunc
end


-- gameevent{<event idx or string> [, flags], <event function>}
local function our_gameevent(args)
    bcheck.top_level("gameevent")

    if (type(args)~="table") then
        error("invalid gameevent call: must be passed a table")
    end

    local event = args[1]

    if (type(event) == "string") then
        if (event:sub(1,6) ~= "EVENT_") then
            event = "EVENT_"..event
        end
        local eventidx = con_lang.EVENT[event]
        if (eventidx == nil) then
            errorf(2, "gameevent: invalid event label %q", event)
        end
        event = eventidx
    end
    if (type(event) ~= "number") then
        error("invalid argument #1 to gameevent: must be a number or event label", 2)
    end
    if (not (event >= 0 and event < con_lang.MAXEVENTS)) then
        error("invalid argument #1 to gameevent: must be an event number (0 .. MAXEVENTS-1)", 2)
    end

    local AF = actor.FLAGS

    -- Kind of CODEDUP from our_gameactor.
    local lastargi = ourmaxn(args)
    local func = args[lastargi]
    if (type(func) ~= "function") then
        func = args.func
        lastargi = 1/0
    end
    if (type(func) ~= "function") then
        error("invalid gameevent call: must provide a function with last numeric arg or .func", 2)
    end

    -- Event chaining: in Lunatic, chaining at the *end* is the default.
    local flags = (lastargi > 2 and args[2]) or args.flags or AF.chain_end
    if (type(flags) ~= "number") then
        error("invalid 'flags' argument (#2) to gameevent: must be a number", 2)
    end

    if (band(flags, BNOT.CHAIN_MASK_EVENT) ~= 0) then
        error("invalid 'flags' argument to gameevent: must not set reserved bits", 2)
    end

    local newfunc = (flags==AF.replace) and func
        or (flags==AF.chain_beg) and chain_func3(func, event_funcs[event])
        or (flags==AF.chain_end) and chain_func3(event_funcs[event], func)
        or assert(false)

    gameevent_internal(event, newfunc)
    event_funcs[event] = newfunc
end

--- non-default data and functions
G_.gameevent = our_gameevent
G_.gameactor = our_gameactor
-- These come from above:
G_.player = player
G_.actor = actor
G_.projectile = projectile
G_.g_tile = g_tile

G_.LUNATIC_FIRST_TIME = (ffiC.g_elFirstTime ~= 0)

-- A table that can be used for temporary data when debugging from the OSD.
G_.d = {}


---=== Lunatic translator setup ===---

read_into_string = readintostr_mod  -- for lunacon
local lunacon = require("lunacon")

local concode, lineinfo

--- Get Lua code for CON (+ mutator) code.
if (g_firstRun) then
    -- Compiling CON for the first time.
    local confn = { ffi.string(ffiC.G_ConFile()) }

    local nummods = ffiC.g_scriptModulesNum
    if (nummods > 0) then
        assert(ffiC.g_scriptModules ~= nil)

        for i=1,nummods do
            confn[i+1] = ffi.string(ffiC.g_scriptModules[i-1])
        end
    end

    concode, lineinfo = lunacon.compile(confn)

    if (concode == nil) then
        error("Failure compiling CON code, exiting.", 0)
    end
    assert(lineinfo)

    -- Back up the translated code on the C side.
    assert(type(concode)=="string")
    ffiC.El_SetCON(concode)
else
    -- CON was already compiled.
    concode = ffi.string(ffiC.g_elCON, ffiC.g_elCONSize)
    lineinfo = lunacon.get_lineinfo(concode)
end

if (ffiC._DEBUG_LUNATIC ~= 0) then
    -- XXX: lineinfo of 2nd up time has one line less.
    printf("CON line info has %d Lua lines", #lineinfo.llines)
end

do
    -- Translate one Lua line number to a CON file name + line number
    local function transline(lnum)
        return string.format("%s:%d", lineinfo:getfline(tonumber(lnum)))
    end

    -- Register the function that tweaks an error message, looking out for
    -- errors from CON and translating the line numbers.
    local function tweak_traceback_msg(errmsg)
        return errmsg:gsub('%[string "CON"%]:([0-9]+)', transline)
    end

    lprivate.tweak_traceback_msg = tweak_traceback_msg

    set_tweak_traceback_internal(tweak_traceback_msg)
end

-- XXX: May still be require'd from user code, we don't want that (at least not
-- under this name).
local CON_MODULE_NAME = "_CON\0"

-- Set up Lunatic gamevar serialization.
do
    local savegame = require("lunasave")

    -- Callback for: const char *(int32_t *slenptr, int32_t levelnum);
    ffiC.El_SerializeGamevars = function(slenptr, levelnum)
        local sb = savegame.savebuffer()

        -- Module name, module table, restore_local. See SAVEFUNC_ARGS.
        sb:addraw("local N,M,F=...")
        -- A local to temporarily hold module locals.
        sb:addraw("local L")

        -- XXX: System gamevars? Most of them ought to be saved with C data.
        for modname, modvars in pairs(module_gamevars) do
            local isCON = (modname==CON_MODULE_NAME)

            sb:startmod(modname)

            -- Handle global gamevars first.
            for i=1,#modvars do
                local varname = modvars[i]
                local excludedVars = isCON and varname=="_V" and
                    package_loaded[CON_MODULE_NAME]._V._IS_NORESET_GAMEVAR or nil

                -- Serialize gamevar named 'varname' from module named 'modname'.
                -- XXX: May error. This will terminate EDuke32 since this callback
                -- is run unprotected.
                if (sb:add("M."..varname, package_loaded[modname][varname], excludedVars)) then
                    -- We couldn't serialize that gamevar.
                    slenptr[0] = -1
                    -- Signal which gamevar that was.
                    return (isCON and "<CON>" or modname).."."..varname
                end
            end

            local modthread = module_thread[modname]

            if (modthread) then
                -- Handle local gamevars.
                local gvmodi = module_gvlocali[modname]

                for li=gvmodi[1],gvmodi[2] do
                    -- Serialize local with index <li>. Get its value first.
                    local lname, lval = debug.getlocal(modthread, 1, li)

                    if (sb:add("L", lval)) then
                        -- We couldn't serialize that gamevar.
                        slenptr[0] = -1
                        return "local "..modname.."."..lname
                    end

                    -- Emit code to call restore_local.
                    sb:addrawf("F(%d,L)", li)
                end
            end

            sb:endmod()
        end

        -- Get the whole code as a string.
        local savecode = sb:getcode()

        if (ffiC._DEBUG_LUNATIC ~= 0) then
            -- Dump the code if Lunatic debugging is enabled (-Lopts=diag) and
            -- there is a LUNATIC_SAVECODE_FN variable in the environment.
            local os = require("os")
            local fn = os.getenv("LUNATIC_SAVECODE_FN")

            if (fn ~= nil) then
                if (levelnum >= 0) then
                    fn = fn .. levelnum
                end

                local io = require("io")
                local f = io.open(fn, "w")

                if (f ~= nil) then
                    f:write(savecode)
                    f:close()
                    printf("Wrote Lunatic gamevar restoration code to \"%s\".", fn)
                end
            end
        end

        -- Set the size of the code and return the code to C.
        slenptr[0] = #savecode
        return savecode
    end
end

-- change the environment of this chunk to the table G_
-- NOTE: all references to global variables from this point on
-- (also in functions created after this point) refer to G_ !
setfenv(1, G_)

-- Print keys and values of a table.
local function printkv(label, table)
    print("========== Keys and values of "..label.." ("..tostring(table)..")")
    for k,v in pairs(table) do
        print(k .. ": " .. tostring(v))
    end
    print("----------")
end

--printkv('_G AFTER SETFENV', _G)


---=== Restricted access to C variables from Lunatic ===---

-- error(..., 2) is to blame the caller and get its line numbers

-- Map of 'gv' variable names to C ones.
local varnameMap = {
    gametic = "g_moveThingsCount",
    RETURN = "g_RETURN",
    _sessionVar = "g_elSessionVar",
}

gv_access.gametic = true
gv_access.RETURN = true
gv_access._sessionVar = true

local tmpmt = {
    __index = function(_, key)
        if (gv_access[key] == nil) then
            -- Read access allowed.
            return ffiC[key]
        elseif (type(gv_access[key])~="boolean") then
            -- Overridden 'gv' pseudo-member...
            return gv_access[key]
        elseif (varnameMap[key]) then
            -- Variable known under a different name on the C side.
            return ffiC[varnameMap[key]]
        end
        error("access forbidden", 2)
    end,

    __newindex = function(_, key, val)
        if (gv_access[key] == nil) then
            -- Variables declared 'const' are handled by LuaJIT.
            ffiC[key] = val
        elseif (varnameMap[key]) then
            ffiC[varnameMap[key]] = val
        else
            error("write access forbidden", 2)
        end
    end,
}
gv = setmtonce(gv_, tmpmt)

-- This will create 'sprite', 'wall', etc. HERE, i.e. in the environment of this chunk
defs_c.create_globals(_G)

-- REMOVE this for release
if (ffiC._DEBUG_LUNATIC ~= 0) then
    local DBG_ = {}
    DBG_.debug = require("debug")
    DBG_.printkv = printkv
    DBG_.loadstring = loadstring
    DBG_.oom = function()
        local s = "1"
        for i=1,math.huge do
            s = s..s
        end
    end

    -- Test reading from all struct members
    DBG_.testmembread = function()
        for _1, sac in pairs { con_lang.StructAccessCode, con_lang.StructAccessCode2 } do
            for what, labels in pairs(sac) do
                if (what~="tspr") then
                    for _3, membaccode in pairs(labels) do
                        if (type(membaccode)=="table") then
                            membaccode = membaccode[1]
                        end
                        if (membaccode) then
                            local codestr = [[
do
    local _bit,_math=require'bit',require'math'
    local _con=require'con'
    local _band,_gv=_bit.band,gv
local tmp=]]..
                            membaccode:gsub("^_gud%(_pli%)", "_con._get_userdef(0)"):gsub("%%s","0").." end"
                            local code = assert(loadstring(codestr))
                            code()
                        end
                    end
                end
            end
        end
    end

    allowed_modules._LUNATIC_DBG = DBG_
end

---=== Finishing environment setup ===---

--printkv('_G AFTER DECLS', _G)

local index_error_mt = {
    __index = function(_, key)
        error("attempt to read undeclared variable '"..key.."'", 2)
    end,

    __metatable = true,
}

-- PiL 14.2 continued
-- We need this at the end because we were previously doing just that!
setmetatable(G_, index_error_mt)

local global_mt = {
    __index = G_,

    __newindex = function()
        error("attempt to write into the global environment")
    end,

    __metatable = true,
}

-- Change the environment of the running Lua thread so that everything
-- what we've set up will be available when this chunk is left.
-- In particular, we need the globals defined after setting this chunk's
-- environment earlier.
setfenv(0, setmetatable({}, global_mt))

do
    -- If we're running from a savegame restoration, create the restoration
    -- function. Must be here, after the above setfenv(), because it must be
    -- created in this protected ('user') context!
    local cstr = ffiC.g_elSavecode
    if (cstr~=nil) then
        local restorecode = ffi.string(cstr)
        ffiC.El_FreeSaveCode()

        g_restorefunc = assert(loadstring(restorecode))
    end
end

-- Restore CON gamevars from loadmapstate.
-- TODO: non-user-defined gamevars.
-- TODO: savegames.
-- int32_t El_RestoreGamevars(const char *)
ffiC.El_RestoreGamevars = function(savecode)
    savecode = ffi.string(savecode)
    local restorefunc = assert(loadstring(savecode))
    restorefunc(CON_MODULE_NAME, package_loaded[CON_MODULE_NAME], nil)
    return 0
end

-- Run the CON code translated into Lua.
if (assert(concode ~= nil)) then
    local confunc, conerrmsg = loadstring(concode, "CON")
    if (confunc == nil) then
        error("Failure loading translated CON code: "..conerrmsg, 0)
    end

    -- Emulate our 'require' for the CON module when running it, for
    -- our_module() which is called from the generated Lua code.
    table.insert(modname_stack, CON_MODULE_NAME)
    local conlabels, conaction, conmove, conai = confunc()
    table.remove(modname_stack)

    local function protect_con_table(tab)
        -- NOTE: Some of our code specifically excepts the name tables to be
        -- indexable with nonexistent keys. See e.g. control.lua: _A_SpawnGlass()
        if (ffiC._LUNATIC_STRICT ~= 0) then
            tab = setmetatable(tab, index_error_mt)
        end
        return setmtonce({}, { __index=tab, __newindex=basemod_newidx })
    end

    -- Set up CON.* modules, providing access to diffenrent kinds of labels
    -- defined in CON from Lua. See CONCODE_RETURN in lunacon.lua.
    allowed_modules["CON.DEFS"] = protect_con_table(conlabels)
    allowed_modules["CON.ACTION"] = protect_con_table(conaction)
    allowed_modules["CON.MOVE"] = protect_con_table(conmove)
    allowed_modules["CON.AI"] = protect_con_table(conai)

    -- Propagate potentially remapped defines to the control module.
    con._setuplabels(conlabels)

    local function getLabelValue(str, doupper)
        return conlabels[doupper and string.upper(str) or str]
    end

    ffiC.El_GetLabelValue = function(label)
        local str = ffi.string(label)
        local ok, val = pcall(getLabelValue, str, false)
        if (not ok or type(val)~="number") then
            ok, val = pcall(getLabelValue, str, true)
        end
        return (ok and type(val)=="number") and val or bit.tobit(0x80000000)
    end
end

-- When starting a map, load Lua modules given on the command line.
if (not g_firstRun) then
    local i=0
    while (ffiC.g_elModules[i] ~= nil) do
        -- Get the module name and strip the trailing extension.
        local modname = ffi.string(ffiC.g_elModules[i]):gsub("%.lua$","")

        if (modname:find("%.")) then
            -- Because they will be replaced by dirseps in our_require().
            error("Dots are not permitted in module names", 0)
        end
        -- Allow forward slashes in module names from the cmdline.
        our_require((modname:gsub("%.lua$",""):gsub("/",".")))

        i = i+1
    end

    ffiC.g_elFirstTime = 0
end

if (g_restorefunc) then
    -- Clear it so that it may be garbage-collected.
    g_restorefunc = nil
end
