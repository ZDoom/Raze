-- CON language definitions

local lpeg = require("lpeg")

local pairs = pairs
local print = print
local setmetatable = setmetatable
local type = type


module(...)


MAXVOLUMES = 7
MAXLEVELS = 64
MAXGAMETYPES = 16

MAXSKILLS = 7

MAXSOUNDS = 4096

MAXSESSIONVARS = 8  -- KEEPINSYNC lunatic_game.c

-- KEEPINSYNC quotes.h

-- For Lunatic, MAXQUOTES is OBITQUOTEINDEX because starting from that index
-- are obituary and suicide quotes which are passed as *format strings* to
-- sprintf() in C.
REALMAXQUOTES = 16384
MAXQUOTES = REALMAXQUOTES-128
MAXQUOTELEN = 128

local STR = {
    STR_MAPNAME = 0,
    STR_MAPFILENAME = 1,
    STR_PLAYERNAME = 2,
    STR_VERSION = 3,
    STR_GAMETYPE = 4,
    STR_VOLUMENAME = 5,
    STR_YOURTIME = 6,
    STR_PARTIME = 7,
    STR_DESIGNERTIME = 8,
    STR_BESTTIME = 9,
}

PROJ = {
    PROJ_WORKSLIKE = 0,
    PROJ_SPAWNS = 1,
    PROJ_SXREPEAT = 2,
    PROJ_SYREPEAT = 3,
    PROJ_SOUND = 4,
    PROJ_ISOUND = 5,
    PROJ_VEL = 6,
    PROJ_EXTRA = 7,
    PROJ_DECAL = 8,
    PROJ_TRAIL = 9,
    PROJ_TXREPEAT = 10,
    PROJ_TYREPEAT = 11,
    PROJ_TOFFSET = 12,
    PROJ_TNUM = 13,
    PROJ_DROP = 14,
    PROJ_CSTAT = 15,
    PROJ_CLIPDIST = 16,
    PROJ_SHADE = 17,
    PROJ_XREPEAT = 18,
    PROJ_YREPEAT = 19,
    PROJ_PAL = 20,
    PROJ_EXTRA_RAND = 21,
    PROJ_HITRADIUS = 22,
    PROJ_VEL_MULT = 23,  -- NAME (PROJ_MOVECNT)
    PROJ_OFFSET = 24,
    PROJ_BOUNCES = 25,
    PROJ_BSOUND = 26,
    PROJ_RANGE = 27,
    PROJ_FLASH_COLOR = 28,
    PROJ_USERDATA = 29,
}

EVENT = {
    EVENT_INIT = 0,
    EVENT_ENTERLEVEL = 1,
    EVENT_RESETWEAPONS = 2,
    EVENT_RESETINVENTORY = 3,
    EVENT_HOLSTER = 4,
    EVENT_LOOKLEFT = 5,
    EVENT_LOOKRIGHT = 6,
    EVENT_SOARUP = 7,
    EVENT_SOARDOWN = 8,
    EVENT_CROUCH = 9,
    EVENT_JUMP = 10,
    EVENT_RETURNTOCENTER = 11,
    EVENT_LOOKUP = 12,
    EVENT_LOOKDOWN = 13,
    EVENT_AIMUP = 14,
    EVENT_FIRE = 15,
    EVENT_CHANGEWEAPON = 16,
    EVENT_GETSHOTRANGE = 17,
    EVENT_GETAUTOAIMANGLE = 18,
    EVENT_GETLOADTILE = 19,
    EVENT_CHEATGETSTEROIDS = 20,
    EVENT_CHEATGETHEAT = 21,
    EVENT_CHEATGETBOOT = 22,
    EVENT_CHEATGETSHIELD = 23,
    EVENT_CHEATGETSCUBA = 24,
    EVENT_CHEATGETHOLODUKE = 25,
    EVENT_CHEATGETJETPACK = 26,
    EVENT_CHEATGETFIRSTAID = 27,
    EVENT_QUICKKICK = 28,
    EVENT_INVENTORY = 29,
    EVENT_USENIGHTVISION = 30,
    EVENT_USESTEROIDS = 31,
    EVENT_INVENTORYLEFT = 32,
    EVENT_INVENTORYRIGHT = 33,
    EVENT_HOLODUKEON = 34,
    EVENT_HOLODUKEOFF = 35,
    EVENT_USEMEDKIT = 36,
    EVENT_USEJETPACK = 37,
    EVENT_TURNAROUND = 38,
    EVENT_DISPLAYWEAPON = 39,
    EVENT_FIREWEAPON = 40,
    EVENT_SELECTWEAPON = 41,
    EVENT_MOVEFORWARD = 42,
    EVENT_MOVEBACKWARD = 43,
    EVENT_TURNLEFT = 44,
    EVENT_TURNRIGHT = 45,
    EVENT_STRAFELEFT = 46,
    EVENT_STRAFERIGHT = 47,
    EVENT_WEAPKEY1 = 48,
    EVENT_WEAPKEY2 = 49,
    EVENT_WEAPKEY3 = 50,
    EVENT_WEAPKEY4 = 51,
    EVENT_WEAPKEY5 = 52,
    EVENT_WEAPKEY6 = 53,
    EVENT_WEAPKEY7 = 54,
    EVENT_WEAPKEY8 = 55,
    EVENT_WEAPKEY9 = 56,
    EVENT_WEAPKEY10 = 57,
    EVENT_DRAWWEAPON = 58,
    EVENT_DISPLAYCROSSHAIR = 59,
    EVENT_DISPLAYREST = 60,
    EVENT_DISPLAYSBAR = 61,
    EVENT_RESETPLAYER = 62,
    EVENT_INCURDAMAGE = 63,
    EVENT_AIMDOWN = 64,
    EVENT_GAME = 65,
    EVENT_PREVIOUSWEAPON = 66,
    EVENT_NEXTWEAPON = 67,
    EVENT_SWIMUP = 68,
    EVENT_SWIMDOWN = 69,
    EVENT_GETMENUTILE = 70,
    EVENT_SPAWN = 71,
    EVENT_LOGO = 72,
    EVENT_EGS = 73,
    EVENT_DOFIRE = 74,
    EVENT_PRESSEDFIRE = 75,
    EVENT_USE = 76,
    EVENT_PROCESSINPUT = 77,
    EVENT_FAKEDOMOVETHINGS = 78,
    EVENT_DISPLAYROOMS = 79,
    EVENT_KILLIT = 80,
    EVENT_LOADACTOR = 81,
    EVENT_DISPLAYBONUSSCREEN = 82,
    EVENT_DISPLAYMENU = 83,
    EVENT_DISPLAYMENUREST = 84,
    EVENT_DISPLAYLOADINGSCREEN = 85,
    EVENT_ANIMATESPRITES = 86,
    EVENT_NEWGAME = 87,
    EVENT_SOUND = 88,
    EVENT_CHECKTOUCHDAMAGE = 89,
    EVENT_CHECKFLOORDAMAGE = 90,
    EVENT_LOADGAME = 91,
    EVENT_SAVEGAME = 92,
    EVENT_PREGAME = 93,
    EVENT_CHANGEMENU = 94,
    EVENT_DAMAGEHPLANE = 95,
    EVENT_ACTIVATECHEAT = 96,
    EVENT_DISPLAYINACTIVEMENU = 97,
    EVENT_DISPLAYINACTIVEMENUREST = 98,
    EVENT_CUTSCENE = 99,
    EVENT_DISPLAYCURSOR = 100,
    EVENT_DISPLAYLEVELSTATS = 101,
    EVENT_DISPLAYCAMERAOSD = 102,
    EVENT_DISPLAYROOMSCAMERA = 103,
    EVENT_DISPLAYSTART = 104,
    EVENT_WORLD = 105,
    EVENT_PREWORLD = 106,
    EVENT_PRELEVEL = 107,
    EVENT_DISPLAYSPIT = 108,
    EVENT_DISPLAYFIST = 109,
    EVENT_DISPLAYKNEE = 110,
    EVENT_DISPLAYKNUCKLES = 111,
    EVENT_DISPLAYSCUBA = 112,
    EVENT_DISPLAYTIP = 113,
    EVENT_DISPLAYACCESS = 114,
    EVENT_RESETGOTPICS = 115,
--    EVENT_ANIMATEALLSPRITES = previous+1,  -- internal
-- KEEPINSYNC with MAXEVENTS below
}

MAXEVENTS = 116  -- KEEPINSYNC with above EVENT_* list

-- NOTE: negated values are not exported to the ffi.C namespace or CON.
-- See TWEAK_SFLAG below.
SFLAG = {
    SFLAG_SHADOW           = 0x00000001,
    SFLAG_NVG              = 0x00000002,
    SFLAG_NOSHADE          = 0x00000004,
    SFLAG_PROJECTILE       = -0x00000008,
    SFLAG_DECAL            = -0x00000010,
    SFLAG_BADGUY           = 0x00000020,
    SFLAG_NOPAL            = 0x00000040,
    SFLAG_NOEVENTS         = 0x00000080,  -- NAME
    SFLAG_NOLIGHT          = 0x00000100,
    SFLAG_USEACTIVATOR     = 0x00000200,
    SFLAG_NULL             = -0x00000400,
    SFLAG_NOCLIP           = 0x00000800,
    SFLAG_NOFLOORSHADOW    = -0x00001000,
    SFLAG_SMOOTHMOVE       = 0x00002000,
    SFLAG_NOTELEPORT       = 0x00004000,
    SFLAG_BADGUYSTAYPUT    = -0x00008000,
    SFLAG_CACHE            = -0x00010000,
    SFLAG_ROTFIXED         = -0x00020000,
    SFLAG_HARDCODED_BADGUY = -0x00040000,
    SFLAG_DIDNOSE7WATER    = -0x00080000,
    SFLAG_NODAMAGEPUSH     = 0x00100000,
    SFLAG_NOWATERDIP       = 0x00200000,
    SFLAG_HURTSPAWNBLOOD   = 0x00400000,
    -- RESERVED for actor.FLAGS.chain_*/replace_*:
    -- 0x08000000, 0x10000000, 0x20000000, 0x40000000
}

STAT = {
    STAT_DEFAULT = 0,
    STAT_ACTOR = 1,
    STAT_ZOMBIEACTOR = 2,
    STAT_EFFECTOR = 3,
    STAT_PROJECTILE = 4,
    STAT_MISC = 5,
    STAT_STANDABLE = 6,
    STAT_LOCATOR = 7,
    STAT_ACTIVATOR = 8,
    STAT_TRANSPORT = 9,
    STAT_PLAYER = 10,
    STAT_FX = 11,
    STAT_FALLER = 12,
    STAT_DUMMYPLAYER = 13,
    STAT_LIGHT = 14,
--    STAT_NETALLOC = 1023,  -- MAXSTATUS-1
}

local GAMEFUNC = {
    GAMEFUNC_MOVE_FORWARD = 0,
    GAMEFUNC_MOVE_BACKWARD = 1,
    GAMEFUNC_TURN_LEFT = 2,
    GAMEFUNC_TURN_RIGHT = 3,
    GAMEFUNC_STRAFE = 4,
    GAMEFUNC_FIRE = 5,
    GAMEFUNC_OPEN = 6,
    GAMEFUNC_RUN = 7,
    GAMEFUNC_AUTORUN = 8,
    GAMEFUNC_JUMP = 9,
    GAMEFUNC_CROUCH = 10,
    GAMEFUNC_LOOK_UP = 11,
    GAMEFUNC_LOOK_DOWN = 12,
    GAMEFUNC_LOOK_LEFT = 13,
    GAMEFUNC_LOOK_RIGHT = 14,
    GAMEFUNC_STRAFE_LEFT = 15,
    GAMEFUNC_STRAFE_RIGHT = 16,
    GAMEFUNC_AIM_UP = 17,
    GAMEFUNC_AIM_DOWN = 18,
    GAMEFUNC_WEAPON_1 = 19,
    GAMEFUNC_WEAPON_2 = 20,
    GAMEFUNC_WEAPON_3 = 21,
    GAMEFUNC_WEAPON_4 = 22,
    GAMEFUNC_WEAPON_5 = 23,
    GAMEFUNC_WEAPON_6 = 24,
    GAMEFUNC_WEAPON_7 = 25,
    GAMEFUNC_WEAPON_8 = 26,
    GAMEFUNC_WEAPON_9 = 27,
    GAMEFUNC_WEAPON_10 = 28,
    GAMEFUNC_INVENTORY = 29,
    GAMEFUNC_INVENTORY_LEFT = 30,
    GAMEFUNC_INVENTORY_RIGHT = 31,
    GAMEFUNC_HOLO_DUKE = 32,
    GAMEFUNC_JETPACK = 33,
    GAMEFUNC_NIGHTVISION = 34,
    GAMEFUNC_MEDKIT = 35,
    GAMEFUNC_TURNAROUND = 36,
    GAMEFUNC_SENDMESSAGE = 37,
    GAMEFUNC_MAP = 38,
    GAMEFUNC_SHRINK_SCREEN = 39,
    GAMEFUNC_ENLARGE_SCREEN = 40,
    GAMEFUNC_CENTER_VIEW = 41,
    GAMEFUNC_HOLSTER_WEAPON = 42,
    GAMEFUNC_SHOW_OPPONENTS_WEAPON = 43,
    GAMEFUNC_MAP_FOLLOW_MODE = 44,
    GAMEFUNC_SEE_COOP_VIEW = 45,
    GAMEFUNC_MOUSE_AIMING = 46,
    GAMEFUNC_TOGGLE_CROSSHAIR = 47,
    GAMEFUNC_STEROIDS = 48,
    GAMEFUNC_QUICK_KICK = 49,
    GAMEFUNC_NEXT_WEAPON = 50,
    GAMEFUNC_PREVIOUS_WEAPON = 51,
--    GAMEFUNC_SHOW_CONSOLE = 52,
    GAMEFUNC_SHOW_DUKEMATCH_SCORES = 53,
    GAMEFUNC_DPAD_SELECT = 54,
    GAMEFUNC_DPAD_AIMING = 55,
}

local function shallow_copy(tab)
    local t = {}
    for k,v in pairs(tab) do
        t[k] = v
    end
    return t
end

-- KEEPINSYNC with gamedef.c:C_AddDefaultDefinitions() and the respective
-- defines. These are exported to the ffi.C namespace (except STAT) and as
-- literal defines in lunacon.lua.
labels =
{
    STR,
    PROJ,
    EVENT,
    shallow_copy(SFLAG),
    setmetatable(STAT, { __metatable="noffiC" }),
    GAMEFUNC,
}

user_sflags = 0
-- TWEAK_SFLAG
for name, flag in pairs(SFLAG) do
    if (flag > 0) then
        user_sflags = user_sflags + flag
    else
        SFLAG[name] = -flag
        labels[4][name] = nil
    end
end

-- KEEPINSYNC player.h
wdata_members =
{
    -- NOTE: they are lowercased for Lunatic
    -- NOTE: members _*sound*, _spawn and _shoots assume *zero* to mean "none"
    --  (-1 would be more logical).
    "const int32_t _workslike",
    "int32_t clip",
    "int32_t reload",
    "int32_t firedelay",
    "int32_t totaltime",
    "int32_t holddelay",
    "int32_t flags",
    "const int32_t _shoots",
    "int32_t spawntime",
    "const int32_t _spawn",
    "int32_t shotsperburst",
    "const int32_t _initialsound",
    "const int32_t _firesound",
    "int32_t sound2time",  -- NOTE: this is a time number, not a sound
    "const int32_t _sound2sound",
    "const int32_t _reloadsound1",
    "const int32_t _reloadsound2",
    "const int32_t _selectsound",
    "int32_t flashcolor",
}


local SP = function(memb) return "sprite[%s]"..memb end
local ATSP = function(memb) return "_atsprite[%s]"..memb end
local AC = function(memb) return "actor[%s]"..memb end
local SX = function(memb) return "spriteext[%s]"..memb end

-- Generate code to access a signed member as unsigned.
local function s2u(label)
    return "(_band("..label.."+65536,65535))"
end

local function S2U(label)
    return { s2u(label), label }
end

-- Some literal checker functions (LITERAL_CHECKING).
-- KEEPINSYNC with the actual setter code.
local function litok_gem1(lit)
    return (lit >= -1)
end

local function litok_ge0(lit)
    return (lit >= 0)
end

local ActorLabels = {
    x = SP".x",
    y = SP".y",
    z = SP".z",
    cstat = SP".cstat",
    picnum = { SP".picnum", SP":set_picnum(%%s)", litok_ge0 },
    shade = SP".shade",
    pal = SP".pal",
    clipdist = SP".clipdist",
--    filler = SP".filler",
--    detail = SP".filler",  -- NAME
    blend = SP".blend",
    xrepeat = SP".xrepeat",
    yrepeat = SP".yrepeat",
    xoffset = SP".xoffset",
    yoffset = SP".yoffset",
    sectnum = { SP".sectnum", SP":changesect(%%s)", litok_ge0 },  -- set: for tsprite
    statnum = { SP".statnum" },
    ang = SP".ang",
    owner = { SP".owner", SP":_set_owner(%%s)", litok_ge0 },
    xvel = SP".xvel",
    yvel = { SP".yvel", SP":set_yvel(%%s)" },
    zvel = SP".zvel",
    lotag = SP".lotag",
    hitag = SP".hitag",
    extra = SP".extra",

    ulotag = S2U(SP".lotag"),
    uhitag = S2U(SP".hitag"),

    -- ActorExtra labels...
    htcgg = AC".cgg",
    -- XXX: why <0 allowed?
    htpicnum = { AC".picnum", AC":set_picnum(%%s)" },
    htang = AC".ang",
    htextra = AC".extra",
    htowner = { AC".owner", AC":set_owner(%%s)", litok_ge0 },
    htmovflag = AC"._movflag",
    httempang = AC".tempang",
    htactorstayput = AC".stayputsect",  -- NAME
    htdispicnum = { AC".dispicnum" },
    -- NOTE: no access for .shootzvel
    httimetosleep = AC".timetosleep",
    htfloorz = AC".floorz",
    htceilingz = AC".ceilingz",
    htlastvx = AC".lastvx",
    htlastvy = AC".lastvy",
    htbposx = AC".bpos.x",
    htbposy = AC".bpos.y",
    htbposz = AC".bpos.z",
    -- Read access differs from write ({ get, set }):
    htg_t = { AC":_get_t_data(%s)", AC":_set_t_data(%s,%%s)" },
    htflags = AC".flags",
    movflags = AC".movflags",

    -- (mostly) model-related flags
    angoff = SX".angoff",
    pitch = SX".pitch",
    roll = SX".roll",
    mdxoff = SX".mdoff.x",  -- NAME
    mdyoff = SX".mdoff.y",
    mdzoff = SX".mdoff.z",
    mdflags = SX".flags",
    xpanning = SX".xpanning",
    ypanning = SX".ypanning",

    alpha = { "_math.floor(spriteext[%s].alpha*255)", "spriteext[%s].alpha=(%%s)/255" },

    isvalid = { "_con._isvalid(%s)" },
}

local function spr2tspr(code)
    if (code and code:find(SP"", 1, true)==1) then
        return ATSP(code:sub(#SP"" + 1))
    end
    -- else return nothing
end

local TspriteLabels = {}

for member, code in pairs(ActorLabels) do
    if (type(code)=="string") then
        TspriteLabels["tspr"..member] = spr2tspr(code)
    else
        TspriteLabels["tspr"..member] = { spr2tspr(code[1]), spr2tspr(code[2]) }
    end
end

-- Sprites set stat- and sectnum via sprite.change{stat,sect} functions.
ActorLabels.sectnum[2] = "sprite.changesect(%s,%%s,true)"
ActorLabels.statnum[2] = "sprite.changestat(%s,%%s,true)"

local PL = function(memb) return "player[%s]"..memb end
-- Access to DukePlayer_t's bool members: they must be read as numbers.
local PLBOOL = function(memb) return { "("..PL(memb).." and 1 or 0)", PL(memb) } end

local empty_table = {}
local DISABLED_PL = function() return empty_table end
local DISABLED = DISABLED_PL

local PlayerLabels = {
    posx = PL".pos.x",
    posy = PL".pos.y",
    posz = PL".pos.z",
    oposx = PL".opos.x",
    oposy = PL".opos.y",
    oposz = PL".opos.z",
    posxv = PL".vel.x",  -- NAME
    posyv = PL".vel.y",
    poszv = PL".vel.z",
    -- NOTE: no access for .npos
    bobposx = DISABLED_PL".bobposx",
    bobposy = DISABLED_PL".bobposy",

    truefz = DISABLED_PL".truefz",
    truecz = DISABLED_PL".truecz",
    player_par = PL".player_par",

    randomflamex = DISABLED_PL".randomflamex",
    exitx = DISABLED_PL".exitx",
    exity = DISABLED_PL".exity",

    runspeed = PL".runspeed",
    max_player_health = PL".max_player_health",
    max_shield_amount = PL".max_shield_amount",

    autostep = PL".autostep",
    autostep_sbw = PL".autostep_sbw",

    interface_toggle_flag = DISABLED_PL".interface_toggle_flag",

    -- NOTE: *bombControl etc. are accessed by gamevars in CON

    max_actors_killed = PL".max_actors_killed",
    actors_killed = PL".actors_killed",

    -- NOTE the special case; "%%s" is used to mark settable members
    -- with METHOD_MEMBER syntax, it's the value to be set.
    gotweapon = { "("..PL":has_weapon(%s) and 1 or 0)", PL":_gt_weapon(%s,%%s)" },
    zoom = PL".zoom",

    loogiex = {},
    loogiey = {},

    sbs = PL".sbs",
    sound_pitch = PL".sound_pitch",

    ang = PL".ang",
    oang = PL".oang",
    angvel = PL".angvel",

    cursectnum = PL".cursectnum",

    look_ang = PL".look_ang",
    last_extra = PL".last_extra",
    subweapon = PL".subweapon",

    max_ammo_amount = PL".max_ammo_amount[%s]",
    ammo_amount = PL".ammo_amount[%s]",
    -- NOTE: no direct access for .inv_amount (but see end)

    wackedbyactor = PL".wackedbyactor",
    pyoff = PL".pyoff",
    opyoff = PL".opyoff",

    horiz = PL".horiz",
    horizoff = PL".horizoff",
    ohoriz = PL".ohoriz",
    ohorizoff = PL".ohorizoff",

    newowner = PL".newowner",

    jumping_counter = PL".jumping_counter",
    airleft = PL".airleft",

    fta = PL".fta",
    ftq = PL".ftq",
    access_wallnum = { PL".access_wallnum" },
    access_spritenum = { PL".access_spritenum" },

    got_access = PL".got_access",
    weapon_ang = PL".weapon_ang",
    visibility = PL".visibility",

    somethingonplayer = PL".somethingonplayer",
    on_crane = PL".on_crane",

    i = { PL".i" },
    index = { "%s" },

    one_parallax_sectnum = DISABLED{ PL".one_parallax_sectnum" },

    random_club_frame = PL".random_club_frame",
    one_eighty_count = PL".one_eighty_count",

    dummyplayersprite = DISABLED_PL".dummyplayersprite",
    extra_extra8 = PL".extra_extra8",

    actorsqu = PL".actorsqu",
    timebeforeexit = PL".timebeforeexit",
    customexitsound = { PL".customexitsound" },

    last_pissed_time = PL".last_pissed_time",

    weaprecs = PL".weaprecs[%s]",

    weapon_sway = PL".weapon_sway",
    crack_time = PL".crack_time",
    bobcounter = PL".bobcounter",

    -- NOTE: no access for .orotscrnang
    rotscrnang = PL".rotscrnang",
    dead_flag = PL".dead_flag",

    holoduke_on = PL".holoduke_on",
    pycount = PL".pycount",
    transporter_hold = PL".transporter_hold",

    max_secret_rooms = PL".max_secret_rooms",
    secret_rooms = PL".secret_rooms",

    frag = PL".frag",
    fraggedself = PL".fraggedself",
    quick_kick = PL".quick_kick",
    last_quick_kick = PL".last_quick_kick",

    return_to_center = DISABLED_PL".return_to_center",
    reloading = PLBOOL".reloading",
    weapreccnt = { PL".weapreccnt" },

    aim_mode = PL".aim_mode",
    auto_aim = PL".auto_aim",
    weaponswitch = PL".weaponswitch",
    movement_lock = PL".movement_lock",
    team = PL".team",

    tipincs = PL".tipincs",
    hbomb_hold_delay = PL".hbomb_hold_delay",
    frag_ps = PL".frag_ps",
    kickback_pic = PL".kickback_pic",

    gm = PL".gm",
    on_warping_sector = PLBOOL".on_warping_sector",
    footprintcount = PL".footprintcount",
    hurt_delay = PL".hurt_delay",

    hbomb_on = PLBOOL".hbomb_on",
    jumping_toggle = PLBOOL".jumping_toggle",
    rapid_fire_hold = PLBOOL".rapid_fire_hold",
    on_ground = PLBOOL".on_ground",

    inven_icon = PL".inven_icon",
    buttonpalette = PL".buttonpalette",
    over_shoulder_on = PLBOOL".over_shoulder_on",
    show_empty_weapon = PL".show_empty_weapon",

    jetpack_on = PLBOOL".jetpack_on",
    spritebridge = PLBOOL".spritebridge",
    lastrandomspot = DISABLED_PL".lastrandomspot",

    scuba_on = PLBOOL".scuba_on",
    footprintpal = PL".footprintpal",
    heat_on = PLBOOL".heat_on",
    invdisptime = PL".invdisptime",
    holster_weapon = PLBOOL".holster_weapon",
    falling_counter = PL".falling_counter",
    footprintshade = PL".footprintshade",

    refresh_inventory = PL".refresh_inventory",
    last_full_weapon = PL".last_full_weapon",

    walking_snd_toggle = PL".walking_snd_toggle",
    palookup = PL".palookup",
    hard_landing = PL".hard_landing",
    fist_incs = PL".fist_incs",

    toggle_key_flag = { PL".toggle_key_flag" },
    knuckle_incs = PL".knuckle_incs",
    knee_incs = PL".knee_incs",
    access_incs = PL".access_incs",

    numloogs = DISABLED_PL".numloogs",
    loogcnt = PL".loogcnt",
    scream_voice = { PL".scream_voice" },

    last_weapon = PL".last_weapon",
    cheat_phase = { PL".cheat_phase" },
    weapon_pos = PL".weapon_pos",
    wantweaponfire = PL".wantweaponfire",

    curr_weapon = PL".curr_weapon",

    palette = { PL".palette" },

    -- NOTE the special case:
    pals = PL"._pals[%s]",
    pals_time = PL"._pals.f",

    name = {},

    -- Access to .inv_amount
    steroids_amount = PL".inv_amount[0]",
    shield_amount = PL".inv_amount[1]",
    scuba_amount = PL".inv_amount[2]",
    holoduke_amount = PL".inv_amount[3]",
    jetpack_amount = PL".inv_amount[4]",
    -- 5: dummy
    -- 6: no "access_amount"
    heat_amount = PL".inv_amount[7]",
    -- 8: dummy
    firstaid_amount = PL".inv_amount[9]",
    boot_amount = PL".inv_amount[10]",
}

local SEC = function(memb) return "sector[%s]"..memb end
local SECRO = function(memb) return { "sector[%s]"..memb } end

local SectorLabels = {
    wallptr = SECRO".wallptr",
    wallnum = SECRO".wallnum",

    ceilingz = SEC".ceilingz",
    floorz = SEC".floorz",

    ceilingstat = SEC".ceilingstat",
    floorstat = SEC".floorstat",

    -- CEILING
    ceilingpicnum = { SEC".ceilingpicnum", SEC":set_ceilingpicnum(%%s)", litok_ge0 },

    ceilingslope = SEC".ceilingheinum",  -- NAME
    ceilingshade = SEC".ceilingshade",

    ceilingpal = SEC".ceilingpal",
    ceilingxpanning = SEC".ceilingxpanning",
    ceilingypanning = SEC".ceilingypanning",

    -- FLOOR
    floorpicnum = { SEC".floorpicnum", SEC":set_floorpicnum(%%s)", litok_ge0 },

    floorslope = SEC".floorheinum",  -- NAME
    floorshade = SEC".floorshade",

    floorpal = SEC".floorpal",
    floorxpanning = SEC".floorxpanning",
    floorypanning = SEC".floorypanning",

    visibility = SEC".visibility",
    fogpal = SEC".fogpal",
    alignto = SEC".fogpal",  -- NAME

    lotag = SEC".lotag",
    hitag = SEC".hitag",
    extra = SEC".extra",

    ceilingbunch = { SEC".ceilingbunch" },
    floorbunch = { SEC".floorbunch" },

    ulotag = S2U(SEC".lotag"),
    uhitag = S2U(SEC".hitag"),
}

local WAL = function(memb) return "wall[%s]"..memb end
local WALRO = function(memb) return { "wall[%s]"..memb } end

local WallLabels = {
    x = WAL".x",
    y = WAL".y",
    point2 = WALRO".point2",
    nextwall = { WAL".nextwall", WAL":_set_nextwall(%%s)" },
    nextsector = { WAL".nextsector", WAL":_set_nextsector(%%s)" },
    cstat = WAL".cstat",
    picnum = { WAL".picnum", WAL":set_picnum(%%s)", litok_ge0 },
    overpicnum = { WAL".overpicnum", WAL":set_overpicnum(%%s)", litok_ge0 },
    shade = WAL".shade",
    pal = WAL".pal",
    xrepeat = WAL".xrepeat",
    yrepeat = WAL".yrepeat",
    xpanning = WAL".xpanning",
    ypanning = WAL".ypanning",
    lotag = WAL".lotag",
    hitag = WAL".hitag",
    extra = WAL".extra",
    blend = WAL".blend",

    ulotag = S2U(WAL".lotag"),
    uhitag = S2U(WAL".hitag"),
}

local function tonegtag(LabelsTab, member, funcname)
    local memb = LabelsTab[member]
    LabelsTab[member] = { memb, memb.."="..funcname.."(%%s)" }
end

function setup_negative_tag_check(funcname)
    tonegtag(TspriteLabels, "tsprlotag", funcname)
    tonegtag(TspriteLabels, "tsprhitag", funcname)
    tonegtag(ActorLabels, "lotag", funcname)
    tonegtag(ActorLabels, "hitag", funcname)
    tonegtag(WallLabels, "lotag", funcname)
    tonegtag(WallLabels, "hitag", funcname)
    tonegtag(SectorLabels, "lotag", funcname)
    tonegtag(SectorLabels, "hitag", funcname)
end

local PROJ = function(memb) return "projectile[%s]"..memb end
local THISPROJ = function(memb) return "actor[%s].proj"..memb end

local ProjectileLabels = {
    workslike = PROJ".workslike",
    cstat = PROJ".cstat",
    hitradius = PROJ".hitradius",
    range = PROJ".range",
    flashcolor = PROJ".flashcolor",
    spawns = { PROJ".spawns", PROJ":set_spawns(%%s)", litok_gem1 },
    sound = { PROJ".sound", PROJ":set_sound(%%s)", litok_gem1 },
    isound = { PROJ".isound", PROJ":set_isound(%%s)", litok_gem1 },
    vel = PROJ".vel",
    decal = { PROJ".decal", PROJ":set_decal(%%s)", litok_gem1 },
    trail = { PROJ".trail", PROJ":set_trail(%%s)", litok_gem1 },
    tnum = PROJ".tnum",
    drop = PROJ".drop",
    offset = PROJ".offset",
    bounces = PROJ".bounces",
    bsound = { PROJ".bsound", PROJ":set_bsound(%%s)", litok_gem1 },
    toffset = PROJ".toffset",
    extra = PROJ".extra",
    extra_rand = PROJ".extra_rand",
    sxrepeat = PROJ".sxrepeat",
    syrepeat = PROJ".syrepeat",
    txrepeat = PROJ".txrepeat",
    tyrepeat = PROJ".tyrepeat",
    shade = PROJ".shade",
    xrepeat = PROJ".xrepeat",
    yrepeat = PROJ".yrepeat",
    pal = PROJ".pal",
    velmult = PROJ".movecnt",  -- NAME
    clipdist = PROJ".clipdist",
    userdata = PROJ".userdata",
}

-- XXX: kind of CODEDUP form spr2tspr
local function proj2thisproj(code)
    if (code and code:find(PROJ"", 1, true)==1) then
        return THISPROJ(code:sub(#PROJ"" + 1))
    end
    -- else return nothing
end

local SpriteProjectileLabels = {}

for member, code in pairs(ProjectileLabels) do
    if (type(code)=="string") then
        SpriteProjectileLabels[member] = proj2thisproj(code)
    else
        SpriteProjectileLabels[member] = { proj2thisproj(code[1]), proj2thisproj(code[2]) }
    end
end

local UD = function(memb) return "_gud(_pli)"..memb end
local UDRO = function(memb) return { UD(memb) } end

-- NOTE: Only members that are actually encountered in existing mods are added here.
-- TODO: r5043, r5044
local UserdefLabels = {
    althud = UD".althud",
    auto_run = UD".auto_run",
    camerasprite = UDRO".camerasprite",
    cashman = UDRO".cashman",
    clipping = UD".noclip",  -- NAME
    color = UD".color",
    const_visibility = UD".const_visibility",
    crosshair = UD".crosshair",
    crosshairscale = UDRO".crosshairscale",
    detail = { "1" },
    display_bonus_screen = UD".display_bonus_screen",
    drawweapon = UDRO".drawweapon",
    eog = UD".eog",
    ffire = UDRO".ffire",
    fta_on = UD".fta_on",
    god = UD".god",
    idplayers = UDRO".idplayers",
    last_level = UDRO".last_level",
    level_number = { UD".level_number", UD":set_level_number(%%s)", {0, MAXLEVELS-1} },
    levelstats = UD".levelstats",
    lockout = UDRO".lockout",
    m_origin_x = UD".m_origin.x",
    m_origin_y = UD".m_origin.y",
    m_player_skill = UDRO".m_player_skill",
    m_volume_number = { UD".m_volume_number", UD":set_m_volume_number(%%s)", {0, MAXVOLUMES} },
    mouseaiming = UD".mouseaiming",
    pause_on = UDRO".pause_on",
    player_skill = UD".player_skill",
    playerbest = UDRO".playerbest",
    mouseflip = UDRO".mouseflip",
    multimode = { "1" },
    musictoggle = UDRO".config.MusicToggle",
    noexits = UDRO".noexits",
    overhead_on = UD".overhead_on",
    recstat = UDRO".recstat",
    runkey_mode = UD".runkey_mode",
    show_level_text = UD".show_level_text",
    screen_size = { UD".screen_size", UD":set_screen_size(%%s)" },
    screen_tilting = UD".screen_tilting",
    showallmap = UD".showallmap",
    showweapons = UDRO".showweapons",
    statusbarmode = UDRO".statusbarmode",
    statusbarscale = UDRO".statusbarscale",
    volume_number = { UD".volume_number", UD":set_volume_number(%%s)", {0, MAXVOLUMES} },
    weaponscale = UDRO".weaponscale",
    weaponswitch = UD".weaponswitch",
}

local INP = function(memb) return PL"._input"..memb end

local InputLabels = {
    avel = INP".avel",
    horz = INP".horz",
    fvel = INP".fvel",
    svel = INP".svel",
    bits = INP".bits",
    extbits = INP".extbits",
}

local TileDataLabels = {
    -- tilesiz[]
    xsize = "g_tile.sizx[%s]",
    ysize = "g_tile.sizy[%s]",

    -- picanm[]
--    "animframes",
--    "xoffset",
--    "yoffset",
--    "animspeed",
--    "animtype",

    -- g_tile[]
    gameflags = { "g_tile[%s]._flags" },
}

StructAccessCode =
{
    sector = SectorLabels,
    wall = WallLabels,
    sprite = ActorLabels,
    player = PlayerLabels,
    tspr = TspriteLabels,
    projectile = ProjectileLabels,
    thisprojectile = SpriteProjectileLabels,
    userdef = UserdefLabels,
    input = InputLabels,
    tiledata = TileDataLabels,
-- TODO: tiledata picanm[] members, paldata
}

-- NOTE: These MUST be in reverse lexicographical order!
-- Per CON syntax, valid identifiers names are disjoint from keywords,
-- so that a rule like
--      t_identifier = -con_keyword * (sp1 + "[") * t_identifier_all
-- (from the final grammar in lunacon.lua) must match the longest
-- possible keyword name, else the negation might wrongly not fail.

keyword =

lpeg.P(false) +
"}" +
"{" +
"zshootvar" +
"zshoot" +
"xorvarvar" +
"xorvar" +
"writearraytofile" +
"whilevarvarn" +
"whilevarn" +
"wackplayer" +
"userquote" +
"useractor" +
"updatesectorz" +
"updatesector" +
"undefinevolume" +
"undefineskill" +
"undefinelevel" +
"tossweapon" +
"tip" +
"time" +
"switch" +
"subvarvar" +
"subvar" +
"strength" +
"stopsoundvar" +
"stopsound" +
"stopallsounds" +
"stopactorsound" +
"state" +
"starttrackvar" +
"starttrack" +
"startlevel" +
"startcutscene" +
"ssp" +
"sqrt" +
"spriteshadow" +
"spritepal" +
"spritenvg" +
"spritenoshade" +
"spritenopal" +
"spriteflags" +
"spgetlotag" +
"spgethitag" +
"spawn" +
"soundvar" +
"soundoncevar" +
"soundonce" +
"sound" +
"smaxammo" +
"sleeptime" +
"sizeto" +
"sizeat" +
"sin" +
"showviewunbiased" +
"showview" +
"shootvar" +
"shoot" +
"shiftvarr" +
"shiftvarl" +
"shadeto" +
"setwall" +
"setvarvar" +
"setvar" +
"setuserdef" +
"settspr" +
"setthisprojectile" +
"setsprite" +
"setsector" +
"setprojectile" +
"setplayervar" +
"setplayerangle" +
"setplayer" +
"setmusicposition" +
"setinput" +
"setgamepalette" +
"setgamename" +
"setdefname" +
"setcfgname" +
"setaspect" +
"setarray" +
"setactorvar" +
"setactorsoundpitch" +
"setactorangle" +
"setactor" +
"sectsetinterpolation" +
"sectorofwall" +
"sectgetlotag" +
"sectgethitag" +
"sectclearinterpolation" +
"scriptsize" +
"screentext" +
"screensound" +
"savenn" +
"savemapstate" +
"savegamevar" +
"save" +
"rotatespritea" +
"rotatesprite16" +
"rotatesprite" +
"rotatepoint" +
"return" +
"respawnhitag" +
"resizearray" +
"resetplayerflags" +
"resetplayer" +
"resetcount" +
"resetactioncount" +
"redefinequote" +
"readgamevar" +
"readarrayfromfile" +
"rayintersect" +
"randvarvar" +
"randvar" +
"quote" +
"quake" +
"qsubstr" +
"qstrncat" +
"qstrlen" +
"qstrdim" +
"qstrcpy" +
"qstrcat" +
"qsprintf" +
"qspawnvar" +
"qspawn" +
"qgetsysstr" +
"pstomp" +
"prevspritestat" +
"prevspritesect" +
"precache" +
"pkick" +
"paper" +
"palfrom" +
"orvarvar" +
"orvar" +
"operatesectors" +
"operaterespawns" +
"operatemasterswitches" +
"operateactivators" +
"operate" +
"onevent" +
"nullop" +
"nextspritestat" +
"nextspritesect" +
"neartag" +
"myosx" +
"myospalx" +
"myospal" +
"myos" +
"music" +
"mulvarvar" +
"mulvar" +
"mulscale" +
"movesprite" +
"move" +
"money" +
"modvarvar" +
"modvar" +
"minitext" +
"mikesnd" +
"mail" +
"lotsofglass" +
"lockplayer" +
"loadmapstate" +
"lineintersect" +
"ldist" +
"killit" +
"jump" +
"insertspriteq" +
"inittimer" +
"includedefault" +
"include" +
"ifwasweapon" +
"ifvarxor" +
"ifvarvarxor" +
"ifvarvaror" +
"ifvarvarn" +
"ifvarvarl" +
"ifvarvarg" +
"ifvarvareither" +
"ifvarvare" +
"ifvarvarand" +
"ifvaror" +
"ifvarn" +
"ifvarl" +
"ifvarg" +
"ifvareither" +
"ifvare" +
"ifvarand" +
"ifstrength" +
"ifsquished" +
"ifspritepal" +
"ifspawnedby" +
"ifsound" +
"ifserver" +
"ifrnd" +
"ifrespawn" +
"ifplayersl" +
"ifpinventory" +
"ifphealthl" +
"ifpdistl" +
"ifpdistg" +
"ifp" +
"ifoutside" +
"ifonwater" +
"ifnotmoving" +
"ifnosounds" +
"ifmultiplayer" +
"ifmove" +
"ifinwater" +
"ifinspace" +
"ifinouterspace" +
"ifhitweapon" +
"ifhitspace" +
"ifgotweaponce" +
"ifgapzl" +
"iffloordistl" +
"ifdead" +
"ifcutscene" +
"ifcount" +
"ifclient" +
"ifceilingdistl" +
"ifcanshoottarget" +
"ifcanseetarget" +
"ifcansee" +
"ifbulletnear" +
"ifawayfromwall" +
"ifangdiffl" +
"ifai" +
"ifactorsound" +
"ifactornotstayput" +
"ifactor" +
"ifactioncount" +
"ifaction" +
"hitscan" +
"hitradiusvar" +
"hitradius" +
"headspritestat" +
"headspritesect" +
"guts" +
"guniqhudid" +
"gmaxammo" +
"globalsoundvar" +
"globalsound" +
"getzrange" +
"getwall" +
"getuserdef" +
"gettspr" +
"gettimedate" +
"getticks" +
"getthisprojectile" +
"gettexturefloor" +
"gettextureceiling" +
"getsector" +
"getprojectile" +
"getpname" +
"getplayervar" +
"getplayerangle" +
"getplayer" +
"getmusicposition" +
"getlastpal" +
"getkeyname" +
"getinput" +
"getincangle" +
"getflorzofslope" +
"getcurraddress" +
"getceilzofslope" +
"getarraysize" +
"getangletotarget" +
"getangle" +
"getactorvar" +
"getactorangle" +
"getactor" +
"gamevar" +
"gametextz" +
"gametext" +
"gamestartup" +
"gamearray" +
"flash" +
"findplayer" +
"findotherplayer" +
"findnearspritezvar" +
"findnearspritez" +
"findnearspritevar" +
"findnearsprite3dvar" +
"findnearsprite3d" +
"findnearsprite" +
"findnearactorzvar" +
"findnearactorz" +
"findnearactorvar" +
"findnearactor3dvar" +
"findnearactor3d" +
"findnearactor" +
"fall" +
"ezshootvar" +
"ezshoot" +
"eventloadactor" +
"espawnvar" +
"espawn" +
"eshootvar" +
"eshoot" +
"eqspawnvar" +
"eqspawn" +
"enhanced" +
"endswitch" +
"ends" +
"endoflevel" +
"endofgame" +
"endevent" +
"enda" +
"else" +
"echo" +
"dynamicsoundremap" +
"dynamicremap" +
"dragpoint" +
"divvarvar" +
"divvar" +
"dist" +
"displayrandvarvar" +
"displayrandvar" +
"displayrand" +
"digitalnumberz" +
"digitalnumber" +
"defstate" +
"definevolumename" +
"definevolumeflags" +
"definesound" +
"defineskillname" +
"definequote" +
"defineprojectile" +
"definelevelname" +
"definegametype" +
"definegamefuncname" +
"definecheat" +
"define" +
"default" +
"debug" +
"debris" +
"cstator" +
"cstat" +
"count" +
"cos" +
"copy" +
"cmenu" +
"clipmovenoslide" +
"clipmove" +
"clipdist" +
"clearmapstate" +
"checkavailweapon" +
"checkavailinven" +
"checkactivatormotion" +
"cheatkeys" +
"changespritestat" +
"changespritesect" +
"case" +
"canseespr" +
"cansee" +
"calchypotenuse" +
"cactor" +
"break" +
"betaname" +
"appendevent" +
"angoffvar" +
"angoff" +
"andvarvar" +
"andvar" +
"ai" +
"addweaponvar" +
"addweapon" +
"addvarvar" +
"addvar" +
"addstrength" +
"addphealth" +
"addlogvar" +
"addlog" +
"addkills" +
"addinventory" +
"addammo" +
"actor" +
"activatecheat" +
"activatebysector" +
"activate" +
"action" +
lpeg.P(false)
