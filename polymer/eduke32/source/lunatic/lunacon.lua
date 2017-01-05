-- LunaCON CON to Lunatic translator
-- requires LPeg, http://www.inf.puc-rio.br/~roberto/lpeg/lpeg.html

local require = require
local lpeg = require("lpeg")

local bit
local math = require("math")
local string = require("string")
local table = require("table")


local arg = arg

local assert = assert
local error = error
local ipairs = ipairs
local loadstring = loadstring
local pairs = pairs
local pcall = pcall
local print = print
local setmetatable = setmetatable
local tonumber = tonumber
local tostring = tostring
local type = type
local unpack = unpack

-- non-nil if running from EDuke32
-- (read_into_string~=nil  iff  string.dump==nil)
local read_into_string = read_into_string
local ffi, ffiC

if (string.dump) then  -- running stand-alone
    local ljp = pcall(function() require("ffi") end)
    -- "lbit" is the same module as LuaJIT's "bit" (LuaBitOp:
    -- http://bitop.luajit.org/), but under a different name for (IMO) less
    -- confusion. Useful for running with Rio Lua for cross-checking.
    bit = ljp and require("bit") or require("lbit")
    require("strict")
else
    bit = require("bit")
    ffi = require("ffi")
    ffiC = ffi.C
end



module("lunacon")


-- I think that the "too many pending calls/choices" is unavoidable in general.
-- This limit is of course still arbitrary, but writing long if/else cascades
-- in CON isn't pretty either (though sometimes necessary because nested switches
-- don't work?)
-- See also:  http://lua-users.org/lists/lua-l/2010-03/msg00086.html
lpeg.setmaxstack(1024);


local Pat, Set, Range, Var = lpeg.P, lpeg.S, lpeg.R, lpeg.V
local POS, Cc, Ctab = lpeg.Cp, lpeg.Cc, lpeg.Ct

-- CON language definitions (among other things, all keywords pattern).
local conl = require("con_lang")


local function match_until(matchsp, untilsp)  -- (!untilsp matchsp)* in PEG
    -- sp: string or pattern
    return (matchsp - Pat(untilsp))^0
end

local format = string.format
--[[
format = function(fmt, ...)
    local ok, res = pcall(string.format, fmt, ...)
    if (not ok) then
        error(string.format("FAILED format(%q, ...) | message: %s", fmt, res))
    end
    return res
end
--]]

local function printf(fmt, ...)
    print(format(fmt, ...))
end

--- some constants

local C = {
    -- These two are not used except for predefined labels.
    -- NOTE: in-game, MAXSPRITES may be 4096 for a V7 build!
    MAXSTATUS = ffiC and ffiC.MAXSTATUS or 1024,
    MAXSPRITES = ffiC and ffiC.MAXSPRITES or 16384,

    MAXTILES = ffiC and ffiC.MAXTILES or 30720,
    MAX_WEAPONS = ffiC and ffiC.MAX_WEAPONS or 12,
}

---=== semantic action functions ===---

local inf = 1/0
local NaN = 0/0

-- Last keyword position, for error diagnosis.
local g_lastkwpos = nil
local g_lastkw = nil
local g_badids = {}  -- maps bad id strings to 'true'

local g_recurslevel = -1  -- 0: base CON file, >0 included
local g_filename = "???"
local g_directory = ""  -- with trailing slash if not empty
local g_maxerrors = 20
local g_numerrors = 0

-- Default directory to search for GAME.CON etc.
-- Stand-alone LunaCON only.
local g_defaultDir = nil

-- Warning options. Key names are the same as cmdline options, e.g.
-- -Wno-bad-identifier for disabling the "bad identifier" warning.
local g_warn = { ["not-redefined"]=true, ["bad-identifier"]=false,
                 ["number-conversion"]=true, ["system-gamevar"]=true,
                 ["error-bad-getactorvar"]=false, ["chained-loadactor"]=true,
                 ["never-used-gamevar"]=false, ["never-read-gamevar"]=false, }

-- Code generation and output options.
local g_cgopt = { ["no"]=false, ["debug-lineinfo"]=false, ["gendir"]=nil,
                  ["cache-sap"]=false, ["error-nostate"]=true,
                  ["playervar"]=true, ["trapv"]=false, ["wrapv"]=false,
                  ["bad-getactorvar-use-pli"]=true,
                  ["error-nonlocal-userdef"]=true,
                  ["error-negative-tag-write"]=false, }

if (string.dump) then
    g_cgopt["names"] = false
end

-- For -fnames mode.
local g_actorTileToName = {}

local function csapp() return g_cgopt["cache-sap"] end

local function handle_cmdline_arg(str)
    if (str:sub(1,1)=="-") then
        if (#str == 1) then
            printf("Warning: input from stdin not supported")
        else
            local ok = false
            local kind = str:sub(2,2)

            -- -W(no-)*: warnings
            if (kind=="W" and #str >= 3) then
                local val = true
                local warnstr = str:sub(3)

                if (warnstr == "all") then
                    -- Enable all warnings.
                    for wopt in pairs(g_warn) do
                        g_warn[wopt] = true
                    end
                    ok = true
                else
                    -- Enable or disable a particular warning.
                    if (warnstr:sub(1,3)=="no-") then
                        val = false
                        warnstr = warnstr:sub(4)
                    end

                    if (type(g_warn[warnstr])=="boolean") then
                        g_warn[warnstr] = val
                        ok = true
                    end
                end

            -- -fno* special handling
            elseif (str:sub(2)=="fno") then
                -- Disable printing code entirely.
                g_cgopt["no"] = true
                ok = true
            elseif (str:sub(2)=="fno=onlycheck") then
                -- Disable printing code, only do syntax check of gen'd code.
                g_cgopt["no"] = "onlycheck"
                ok = true

            -- -fgendir=<directory>: specify directory for generated code
            elseif (str:sub(2,9)=="fgendir=" and #str >= 10) then
                g_cgopt["gendir"] = str:sub(10)
                ok = true

            -- -f(no-)*: code generation options
            elseif (kind=="f" and #str >= 3) then
                local val = true
                local cgstr = str:sub(3)

                if (cgstr:sub(1,3)=="no-") then
                    val = false
                    cgstr = cgstr:sub(4)
                end

                if (type(g_cgopt[cgstr])=="boolean") then
                    g_cgopt[cgstr] = val
                    ok = true
                end

            -- -I<directory>: default search directory (only ONCE, not search path)
            elseif (kind=="I" and #str >= 3) then
                g_defaultDir = str:sub(3)
                ok = true
            end

            if (not ffi and not ok) then
                printf("Warning: Unrecognized option %s", str)
            end
        end

        return true
    end
end

-- Handle command line arguments. Has to happen before pattern construction,
-- because some of them depend on codegen options (specifically, -ftrapv,
-- -fwrapv).
if (string.dump) then
    -- running stand-alone
    local i = 1
    while (arg[i]) do
        if (handle_cmdline_arg(arg[i])) then
            table.remove(arg, i)  -- remove processed cmdline arg
        else
            i = i+1
        end
    end
else
    -- running from EDuke32
    local i=0
    while (ffiC.g_argv[i] ~= nil) do
        handle_cmdline_arg(ffi.string(ffiC.g_argv[i]))
        i = i+1
    end
end

if (g_cgopt["error-negative-tag-write"]) then
    conl.setup_negative_tag_check("_st")
end

-- Stack with *true* on top if the innermost block is a "whilevar*n".
local g_isWhile = {}
-- Sequence number of 'while' statements, used to implement CON "break" inside
-- whilevar*n, which really behaves like what sane languages call "continue"...
local g_whilenum = 0

---=== Code generation ===---
local GVFLAG = {
    PERPLAYER=1, PERACTOR=2, PERX_MASK=3,
    SYSTEM   = 0x00000800,
    READONLY = 0x00001000,

    NODEFAULT = 0x00000400,  -- don't reset on actor spawn
    NORESET   = 0x00020000,  -- don't reset when restoring map state

    CON_PERPLAYER = 0x40000000,  -- LunaCON internal
}

-- NOTE: This differs from enum GamevarFlags_t's GAMEVAR_USER_MASK
GVFLAG.USER_MASK = GVFLAG.PERX_MASK + GVFLAG.NODEFAULT + GVFLAG.NORESET

-- CON --> mangled Lua function name, also existence check:
local g_funcname = {}
-- while parsing a block, it is a table of "gencode" tables:
local g_switchCode = nil
-- Global number of switch statements:
local g_switchCount = 0
-- Number of session gamevars:
local g_numSessionVars = 0
-- [identifier] = { name=<mangled name / code>, flags=<gamevar flags> }
local g_gamevar = {}
-- [identifier] = { name=<mangled name / code>, size=<initial size> }
local g_gamearray = {}

-- * nil if dynamic tile remapping disabled
-- * {} if enabled but no remappings made
-- * else, a nonempty table { [name]=<g_dynTileList index> }
local g_dyntilei = nil
-- Analogously for sounds.
local g_dynsoundi = nil

local g_have_file = {}  -- [filename]=true
local g_curcode = nil  -- a table of string pieces or other "gencode" tables

-- will be a table, see reset.codegen()
local g_code = nil


local function ACS(s) return (csapp() and "_a" or "actor[_aci]")..s end
local function SPS(s) return (csapp() and "_spr" or "sprite[_aci]")..s end
local function PLS(s) return (csapp() and "_ps" or "player[_pli]")..s end
local function PLSX(s) return "player[_pli]"..s end


local function getlinecol(pos) end -- fwd-decl

local function new_initial_codetab()
    -- NOTE: Keep this one line per line to not confuse the Lua->CON line
    -- mapping system.
    return {
        -- Requires.
        "local require=require",
        "local _con, _bit, _math = require'con', require'bit', require'math'",
        "local _xmath = require'xmath'",

        -- Cache globals into locals.
        "local sector, sprite, wall, spriteext, _atsprite = sector, sprite, wall, spriteext, _atsprite",
        "local actor, player, projectile, g_tile = actor, player, projectile, g_tile",
        "local gameactor, gameevent, _gv = gameactor, gameevent, gv",
        "local updatesector, updatesectorz, cansee = updatesector, updatesectorz, cansee",
        "local print, printf = print, printf",

        -- Cache a couple of often-used functions.
        "local _div, _mod, _mulTR, _mulWR = _con._div, _con._mod, _con._mulTR, _con._mulWR",
        "local _band, _bor, _bxor = _bit.band, _bit.bor, _bit.bxor",
        "local _lsh, _rsh, _arsh = _bit.lshift, _bit.rshift, _bit.arshift",
        "local _setsprite,_ssp = _con._setsprite,_con._ssp",
        g_cgopt["error-nonlocal-userdef"]
            and "local _gud=_con._get_userdef_check" or "local _gud=_con._get_userdef",
        "local _st=_con._err_if_negative",

        -- * CON "states" (subroutines) and
        -- * Switch function table, indexed by global switch sequence number:
        "local _F,_SW = {},{}",

        -- CON gamevars and gamearrays (see mangle_name()), set up for
        -- restoration from savegames.
        "module(...)",
        "_V,_A={},{}",
        "-- NOTE to the reader: This require's result is Lunatic-private API! DO NOT USE!",
        "local _dummy,_S=require'end_gamevars'",
-- XXX: Currently commented out because of gamevar restoration from loadmapstate.
--        "local _V,_A=_V,_A",
        "local _C,_M,_I={},{},{}",  -- actions, moves, ais

        -- Static ivec3s so that no allocations need to be made.
        "local _IVEC = { _xmath.ivec3(), _xmath.ivec3() }",
        "local function _IV(num, x, y, z)",
        "  local v=_IVEC[num]; v.x=x; v.y=y; v.z=z; return v;",
        "end",
           }
end

-- CON global system gamevar
local function CSV(var) return "_gv._csv"..var end

-- Creates the table of predefined game variables.
-- KEEPINSYNC gamevars.c: Gv_AddSystemVars()
local function new_initial_gvartab()
    local wmembers = conl.wdata_members

    local function GamevarCreationFunc(addflags)
        return function(varname)
            -- 'used' is a bitmask: 1 is 'was read', 2 is 'was written to'
            return { name=varname, flags=GVFLAG.SYSTEM+addflags, used=3 }
        end
    end

    local RW = GamevarCreationFunc(0)
    local RO = GamevarCreationFunc(GVFLAG.READONLY)
    local PRW = GamevarCreationFunc(GVFLAG.PERPLAYER)
    local PRO = GamevarCreationFunc(GVFLAG.READONLY+GVFLAG.PERPLAYER)

    local gamevar = {
        -- NOTE: THISACTOR can mean different things in some contexts.
        THISACTOR = RO "_aci",

        RETURN = RW "_gv.RETURN",
        HITAG = RW(CSV".HITAG"),
        LOTAG = RW(CSV".LOTAG"),
        TEXTURE = RW(CSV".TEXTURE"),

        -- This will warn when defining from CON, but it's the most
        -- straightforward implementation.
        LOGO_FLAGS = RW "_gv.g_logoFlags",

        xdim = RO "_gv.xdim",
        ydim = RO "_gv.ydim",
        windowx1 = RO "_gv.windowxy1.x",
        windowy1 = RO "_gv.windowxy1.y",
        windowx2 = RO "_gv.windowxy2.x",
        windowy2 = RO "_gv.windowxy2.y",

        yxaspect = RO "_gv._get_yxaspect()",
        viewingrange = RO "_gv._get_viewingrange()",
        -- TODO: gravitationalconstant, gametype_flags

        numsectors = RO "_gv.numsectors",
        NUMSECTORS = RO "_gv.numsectors",
        NUMWALLS = RO "_gv.numwalls",
        Numsprites = RO "_gv.Numsprites",

        randomseed = RW "_gv.randomseed",
        totalclock = RO "_gv.totalclock",
        framerate = RO "_gv._currentFramerate()",
        current_menu = RO "_gv._currentMenu()",
        rendmode = RO "_gv.rendmode",

        screenpeek = RO "_gv.screenpeek",

        camerax = RW "_gv.cam.pos.x",
        cameray = RW "_gv.cam.pos.y",
        cameraz = RW "_gv.cam.pos.z",
        cameraang = RW "_gv.cam.ang",
        camerahoriz = RW "_gv.cam.horiz",
        camerasect = RW "_gv.cam.sect",
        cameradist = RW "_gv.cam.dist",
        cameraclock = RW "_gv.cam.clock",

        -- HUD weapon gamevars
        currentweapon = RW "_gv.hudweap.cur",
        weaponcount = RW "_gv.hudweap.count",
        weapon_xoffset = RW "_gv.hudweap.gunposx",
        looking_angSR1 = RW "_gv.hudweap.lookhalfang",
        gun_pos = RW "_gv.hudweap.gunposy",
        looking_arc = RW "_gv.hudweap.lookhoriz",
        gs = RW "_gv.hudweap.shade",

        -- Some per-player gamevars
        ZRANGE = PRW(PLSX".zrange"),
        ANGRANGE = PRW(PLSX".angrange"),
        AUTOAIMANGLE = PRW(PLSX".autoaimang"),

        PIPEBOMB_CONTROL = PRW(PLSX".pipebombControl"),
        GRENADE_LIFETIME = PRW(PLSX".pipebombLifetime"),
        GRENADE_LIFETIME_VAR = PRW(PLSX".pipebombLifetimeVar"),
        TRIPBOMB_CONTROL = PRW(PLSX".tripbombControl"),
        STICKYBOMB_LIFETIME = PRW(PLSX".tripbombLifetime"),
        STICKYBOMB_LIFETIME_VAR = PRW(PLSX".tripbombLifetimeVar"),

        -- Some *writable* system gamevars relating to multiplayer.
        -- TODO_MP.
        RESPAWN_MONSTERS = RO "0",
        RESPAWN_ITEMS = RO "0",
        RESPAWN_INVENTORY = RO "0",
        MONSTERS_OFF = RO "0",
        MARKER = RO "0",

        -- These are not 100% authentic (they're only updated in certain
        -- circumstances, see player.c: P_SetWeaponGamevars()). But IMO it's
        -- more useful like this.
        WEAPON = PRO(PLSX".curr_weapon"),
        WORKSLIKE = PRO(format(PLSX".weapon[%s].workslike", PLSX".curr_weapon")),

        VOLUME = RO "_gv._ud.volume_number",
        LEVEL = RO "_gv._ud.level_number",
    }

    -- Reserved bits
    gamevar.LOGO_FLAGS.rbits = bit.bnot(0x001fffff)

    for w=0,C.MAX_WEAPONS-1 do
        for i=1,#wmembers do
            local member = wmembers[i]:gsub(".*_t ","")  -- strip e.g. "const int32_t "
                                      :gsub("^_","")  -- strip potentially leading underscore
            local name = format("WEAPON%d_%s", w, member:upper())
            gamevar[name] = PRW(format(PLSX".weapon[%d].%s", w, member))

            if (member=="flags") then
                gamevar[name].rbits = bit.bnot(0x1ffff)
            end
        end
    end

    return gamevar
end

local reset = {}

function reset.codegen()
    g_funcname = {}
    g_switchCode = nil
    g_switchCount = 0
    g_numSessionVars = 0
    g_gamevar = new_initial_gvartab()
    g_gamearray = {
        -- SYSTEM_GAMEARRAY
        tilesizx = { name="g_tile.sizx", size=C.MAXTILES, sysp=true },
        tilesizy = { name="g_tile.sizy", size=C.MAXTILES, sysp=true },
    }

    g_dyntilei = nil
    g_dynsoundi = nil

    g_have_file = {}
    g_curcode = new_initial_codetab()
    -- actor, event, loadactor: [{actor, event, actor}num] = gencode_table
    --
    -- aflagsloc[actornum]: location of '(user)actor' token, 'spriteflags' or
    -- 'sprite*' command; result of getLocation(<kind>, <pos>)
    g_code = { actor={}, event={}, loadactor={}, aflagsloc={} }

    g_recurslevel = -1
    g_numerrors = 0
end

-- Is SYSTEM_GAMEARRAY?
local function issysgar(str)
    return str:match("^g_tile.siz[xy]")
end

local function addcode(x)
    assert(type(x)=="string" or type(x)=="table")
    g_curcode[#g_curcode+1] = x
end

local function addcodef(fmt, ...)
    addcode(format(fmt, ...))
end

local function paddcodef(pos, fmt, ...)
    addcodef(fmt.."--"..getlinecol(pos), ...)
end

local function add_code_and_end(codetab, endstr)
    assert(type(codetab)=="table")
    addcode(codetab)
    addcode(endstr)
end

local function get_cache_sap_code()
    return csapp() and "local _spr,_a,_ps=_con._getsap(_aci,_pli)" or ""
end

-- fwd-decls
local warnprintf, errprintf, pwarnprintf, perrprintf, contprintf
local getLocation

local on = {}

-- Map from CON actor usertype to SFLAGs.
local MAP_ACTOR_FLAGS = {
    [0] = 0,
    [1] = conl.SFLAG.SFLAG_BADGUY,
    [2] = conl.SFLAG.SFLAG_BADGUY + conl.SFLAG.SFLAG_BADGUYSTAYPUT,
    [3] = conl.SFLAG.SFLAG_BADGUY + conl.SFLAG.SFLAG_BADGUYSTAYPUT,
}
for i=4,7 do
    MAP_ACTOR_FLAGS[i] = MAP_ACTOR_FLAGS[i-4] + conl.SFLAG.SFLAG_ROTFIXED
end


-- Table of functions doing various lookups (label, gamevar, ...)
local lookup = {}

-- For -fnames mode.
function on.fnames_tilenum_label(tilenum)
    if (g_cgopt["names"] and type(tilenum)=="string") then
        -- <tilenum> may be a string (define label)
        -- HANDLE_RAWDEFINE
        local pos, minus, label = tilenum:match("(.-):(.-):(.+)")
        local realtilenum = lookup.defined_label(tonumber(pos), minus, label)

        g_actorTileToName[realtilenum] = label
        return true
    end
end

function on.actor_end(pos, usertype, tsamm, codetab)
    local tilenum = tsamm[1]
    local flags = 0

    if (on.fnames_tilenum_label(tilenum)) then
        return
    end

    if (usertype ~= nil) then  -- useractor
        if (not (bit.band(usertype, bit.bnot(7)) == 0)) then
            perrprintf(pos, "invalid usertype: must be bitwise OR of 1, 2 and/or 4")
        else
            flags = MAP_ACTOR_FLAGS[usertype]
        end
    end

    -- 0x08000000: actor.FLAGS.replace
    flags = bit.bor(flags, 0x08000000)

    local str = flags..","
    for i=2,math.min(#tsamm,4) do
        str = str .. tostring(tsamm[i])..","
    end
    if (#tsamm >= 5) then
        local movflags = bit.bor(unpack(tsamm, 5))
        str = str .. movflags..","
    end

    paddcodef(pos, "gameactor{%d,%sfunction(_aci,_pli,_dist)", tilenum, str)
    addcode(get_cache_sap_code())
    add_code_and_end(codetab, "end}")

    if (g_code.actor[tilenum] ~= nil) then
        pwarnprintf(pos, "redefined actor %d", tilenum)
    end
    g_code.actor[tilenum] = codetab
    g_code.aflagsloc[tilenum] = getLocation("definition of actor", pos)
end

-- NOTE: in C-CON, the slash and backslash can also be part of an identifier,
-- but this is likely to support file names in other places.
local BAD_ID_CHARS0 = "_*?"  -- allowed 1st identifier chars
local BAD_ID_CHARS1 = "_*-+?."  -- allowed following identifier chars

local function truetab(tab)
    local ttab = {}
    for i=1,#tab do
        ttab[tab[i]] = true
    end
    return ttab
end

-- Lua 5.2 keywords. Not 5.1 because we use "goto" for codegen.
local LUA_KEYW = truetab {
    "and", "break", "do", "else", "elseif", "end",
    "false", "for", "function", "goto", "if", "in",
    "local", "nil", "not", "or", "repeat", "return",
    "then", "true", "until", "while"
}

-- Return the Lua code by which the CON object <name> is referenced in the
-- translated code.
local function mangle_name(name, prefix)
    if (name:match("^[A-Za-z_][A-Za-z_0-9]*$") and not LUA_KEYW[name]) then
        return format("_%s.%s", prefix, name)
    else
        return format("_%s[%q]", prefix, name)
    end
end

function on.state_begin_Cmt(_subj, _pos, statename)
    -- We must register the state name early (Cmt) because otherwise, it won't
    -- be found in a recursive state. XXX: The real issue seems to be the use
    -- of "Cmt"s in other places, which messes up the sequence of running the
    -- semantic actions.
    local ourname = mangle_name(statename, "F")
    g_funcname[statename] = ourname
    return true, ourname
end

function on.state_end(pos, funcname, codetab)
    paddcodef(pos, "%s=function(_aci,_pli,_dist)", funcname)
    addcode(get_cache_sap_code())
    add_code_and_end(codetab, "end")
end

function on.event_end(pos, eventidx, codetab)
    assert(type(codetab)=="table")
    -- 0x20000000: actor.FLAGS.chain_beg
    paddcodef(pos, "gameevent{%d,0x20000000,function(_aci,_pli,_dist)", eventidx)
    addcode(get_cache_sap_code())
    addcode(codetab)
    addcode("end}")

    g_code.event[eventidx] = codetab
end

function on.appendevent_end(pos, eventidx, codetab)
    assert(type(codetab)=="table")
    -- 0x40000000: actor.FLAGS.chain_end
    paddcodef(pos, "gameevent{%d,0x40000000,function(_aci,_pli,_dist)", eventidx)
    addcode(get_cache_sap_code())
    addcode(codetab)
    addcode("end}")

    -- XXX: appendevent needs different behavior? g_code.event doesn't appear to be used anywhere, for now.
    g_code.event[eventidx] = codetab
end

function on.eventloadactor_end(pos, tilenum, codetab)
    if (on.fnames_tilenum_label(tilenum)) then
        return
    end

    -- Translate eventloadactor into a chained EVENT_LOADACTOR block
    paddcodef(pos, "gameevent{'LOADACTOR',function(_aci,_pli,_dist)")
    addcode(get_cache_sap_code())
    addcodef("if (%s==%d) then", SPS".picnum", tilenum)
    addcode(codetab)
    addcode("end")
    addcode("end}")

    if (g_code.loadactor[tilenum] ~= nil and g_warn["chained-loadactor"]) then
        -- NOTE: C-CON redefines loadactor code if encountered multiple times.
        pwarnprintf(pos, "chained additional loadactor %d code", tilenum)
    end
    g_code.loadactor[tilenum] = codetab
end

----------

local function linecolstr(pos)
    local line, col = getlinecol(pos)
    return format("%d:%d", line, col)
end

local function increment_numerrors()
    g_numerrors = g_numerrors+1
    if (g_numerrors == g_maxerrors) then
        g_numerrors = inf
        printf("Too many errors (%d), aborting...", g_maxerrors)
    end
end

function perrprintf(pos, fmt, ...)
    printf("%s %s: error: "..fmt, g_filename,
           pos and linecolstr(pos) or "???", ...)
    increment_numerrors()
end

function errprintf(fmt, ...)
    perrprintf(g_lastkwpos, fmt, ...)
end

function pwarnprintf(pos, fmt, ...)
    printf("%s %s: warning: "..fmt, g_filename,
           pos and linecolstr(pos) or "???", ...)
end

function warnprintf(fmt, ...)
    pwarnprintf(g_lastkwpos, fmt, ...)
end

-- Print a continuation line to an error or warning.
function contprintf(iserr, fmt, ...)
    printf("%s %s: %s  "..fmt, g_filename,
           g_lastkwpos and linecolstr(g_lastkwpos) or "???",
           iserr and "     " or "       ", ...)
end

local function parse_number(pos, numstr)
    -- <numstr> is a full number string, potentially prefixed with a minus sign.
    local num = tonumber((numstr:gsub("h$", "")))
--    local onum = num
    local hex = numstr:match("0[xX]([^h]*)h?")  -- get hex digits, if any

    -- num==nil for Rio Lua, which doesn't handle large hex literals.
    if (num==nil or not (num >= -0x80000000 and num <= 0xffffffff)) then
        -- number is <INT32_MIN or >UINT32_MAX or NaN
        if (hex and #hex>8 and hex:sub(1,#hex-8):match("^[fF]$")) then
            -- Too many hex digits, but they're all Fs.
            pwarnprintf(pos, "number %s truncated to 32 bits", numstr)
            num = bit.tobit(num)
        else
            perrprintf(pos, "number %s out of the range of a 32-bit integer", numstr)
            -- Be careful not to write bound checks like
            -- "if (i<LOWBOUND or i>HIGHBOUND) then error('...') end":
            num = NaN
        end
    elseif (num >= 0x80000000) then
        num = bit.tobit(num)
        if (not hex and g_warn["number-conversion"]) then
            pwarnprintf(pos, "number %s converted to %d", numstr, num)
        end
    end

--    printf("numstr:%s, num=%d (0x%s) '%s', resnum=%d (0x%s)",
--           numstr, onum, bit.tohex(onum), hex, num, bit.tohex(num))
    return num
end

-- Bound checking functions that generate a compilation error on failure.
local check = {}

function check.tile_idx(tilenum)
    if (not (tilenum >= 0 and tilenum < C.MAXTILES)) then
        errprintf("invalid tile number %d", tilenum)
        return false
    end
    return true
end

function check.sound_idx(sidx)
    if (not (sidx >= 0 and sidx < conl.MAXSOUNDS)) then
        errprintf("invalid sound number %d", sidx)
        return false
    end
    return true
end


-- Mapping of various "define" types to the respective number of members and
-- vice versa
local LABEL = { MOVE=2, AI=3, ACTION=5, [2]="move", [3]="ai", [5]="action",
                NUMBER=1, [1]="number" }

-- Function names in the 'con' module:
local LABEL_FUNCNAME = { [2]="move", [3]="ai", [5]="action" }
local LABEL_PREFIX = { [2]="M", [3]="I", [5]="C" }  -- _C, _M, _I in the gen'd code

local g_labeldef = {}  -- Lua numbers for numbers, strings for composites
local g_labeltype = {}
local g_labelspecial = {}  -- [<label>] = true
local g_labelloc = {}  -- [<label>] = { filename, linenum, colnum }

-- Get location table for use in continued warning/error reporting.
--[[ local --]]
function getLocation(kind, pos)
    local loc = { g_filename, getlinecol(pos or g_lastkwpos) }
    loc[4] = kind
    return loc
end

function reset.labels()
    g_badids = {}

    -- NO is also a valid `move', `ai' or `action', but they are handled
    -- separately in lookup.composite().
    g_labeldef = {
        NO = 0,
        -- NOTE: these are read-only gamevars in C-CON
        CLIPMASK0 = 65536+1,  -- blocking
        CLIPMASK1 = (256*65536)+64,  -- hittable
        -- TODO_MP
        COOP = 0,
        MULTIMODE = 1,
        numplayers = 1,
        myconnectindex = 0,
        -- Predefined constants
        MAXSTATUS = C.MAXSTATUS,
        MAXSPRITES = C.MAXSPRITES,
        MAX_WEAPONS = C.MAX_WEAPONS,
    }

    g_labeltype = {}
    g_labelspecial = {}
    g_labelloc = {}

    for varname,_ in pairs(g_labeldef) do
        g_labeltype[varname] = LABEL.NUMBER
        g_labelspecial[varname] = true
    end

    -- Initialize default defines.
    for i=1,#conl.labels do
        for label, val in pairs(conl.labels[i]) do
            g_labeldef[label] = val
            g_labeltype[label] = LABEL.NUMBER
        end
    end
end

function lookup.defined_label(pos, maybe_minus_str, identifier)
    local num = g_labeldef[identifier]

    if (num == nil) then
        perrprintf(pos, "label \"%s\" is not defined", identifier)
        return -inf  -- return a number for type cleanness
    end

    if (g_labeltype[identifier] ~= LABEL.NUMBER) then
        perrprintf(pos, "label \"%s\" is not a `define'd number", identifier)
        return -inf
    end

    assert(type(num)=="number")

    return (maybe_minus_str=="" and 1 or -1) * num
end

assert(not BAD_ID_CHARS1:find(":"))
function lookup.raw_defined_label(pos, maybe_minus_str, identifier)
    return pos..":"..maybe_minus_str..":"..identifier
end

local dynmap = {}
-- When necessary, initialize dynamic {tile,sound} mapping list.
function dynmap.maybe_init(dyni, dynList)
    if (dyni[1]==nil) then
        dyni[1] = true
        -- Init name -> g_dyn*List index mapping
        for i=0,math.huge do
            local str = dynList[i].str
            if (str==nil) then
                break
            end

            dyni[ffi.string(str)] = i
        end
    end
end

-- Potentially process one dynamic {tile,sound} remapping.
function dynmap.maybe_process(dyni, dynList, identifier, num)
    if (dyni[identifier]) then
        local di = dynList[dyni[identifier]]

        if (ffiC._DEBUG_LUNATIC~=0 and di.staticval~=num) then
            printf("REMAP %s (%d) --> %d", ffi.string(di.str), di.staticval, num)
        end
        di.dynvalptr[0] = num
    end
end

-- The 'check' table is also used to hold a couple of misc checkers.

function check.sysvar_def_attempt(identifier)
    if (identifier=="actorvar") then
        errprintf("cannot define reserved symbol `actorvar'")
        return true
    end
    if (identifier=="_IS_NORESET_GAMEVAR") then
        errprintf("cannot define reserved symbol `_IS_NORESET_GAMEVAR'")
        return true
    end
end


local inform = {}

function inform.common(loc, iserr, prefix)
    if (loc) then
        contprintf(iserr, prefix.." is at %s %d:%d", loc[1], loc[2], loc[3])
    else
        contprintf(iserr, prefix.." is built-in")
    end
end

function inform.olddef_location(identifier, iserr)
    inform.common(g_labelloc[identifier], iserr, "Old definition")
end

function inform.oldgv_location(identifier, iserr)
    inform.common(g_gamevar[identifier].loc, iserr, "Old definition")
end

function inform.gv_location(identifier, iserr)
    inform.common(g_gamevar[identifier].loc, iserr, "Definition")
end


local Define = {}

function Define.label(identifier, num)
    if (check.sysvar_def_attempt(identifier)) then
        return
    end

    local oldtype = g_labeltype[identifier]
    local oldval = g_labeldef[identifier]

    if (oldval) then
        if (oldtype ~= LABEL.NUMBER) then
            errprintf("Refusing to overwrite `%s' label \"%s\" with a `define'd number.",
                      LABEL[oldtype], identifier)
            inform.olddef_location(identifier, true)
        else
            -- conl.labels[...]: don't warn for wrong PROJ_ redefinitions
            if (g_warn["not-redefined"]) then
                if (oldval ~= num and conl.PROJ[identifier]==nil) then
                    warnprintf("Label \"%s\" not redefined with new value %d (old: %d).",
                               identifier, num, oldval)
                    inform.olddef_location(identifier, false)
                end
            end
        end
    else
        if (g_gamevar[identifier]) then
            warnprintf("symbol `%s' already used for game variable", identifier)
            inform.oldgv_location(identifier, false)
        end

        if (ffi and g_dyntilei and (num>=0 and num<C.MAXTILES)) then
            dynmap.maybe_init(g_dyntilei, ffiC.g_dynTileList)
            dynmap.maybe_process(g_dyntilei, ffiC.g_dynTileList, identifier, num)
        end

        -- New definition of a label
        g_labeldef[identifier] = num
        g_labeltype[identifier] = LABEL.NUMBER
        g_labelloc[identifier] = getLocation()
    end
end

function check.composite_literal(labeltype, pos, num)
    if (num==0 or num==1) then
        return (num==0) and "0" or "1"
    else
        perrprintf(pos, "literal `%s' number must be either 0 or 1", LABEL[labeltype])
        return "_INVALIT"
    end
end

function lookup.composite(labeltype, pos, identifier)
    if (identifier=="NO") then
        -- NO is a special case and is valid for move, action and ai,
        -- being the same as passing a literal 0.
        return "0"
    end

    local val = g_labeldef[identifier]
    local typ = g_labeltype[identifier]

    if (val == nil) then
        perrprintf(pos, "label \"%s\" is not defined", identifier)
        return "_NOTDEF"
    elseif (typ ~= labeltype) then
        if (identifier=="randomangle" and labeltype==LABEL.MOVE and typ==LABEL.NUMBER) then
            -- Be forgiving with a 1.3/1.5 GAME.CON type error.
            pwarnprintf(pos, "label \"randomangle\" is not a `move' value, assuming 0")
            return "0"
        elseif (identifier=="BLIMPRESPAWNTIME" and labeltype==LABEL.ACTION and typ==LABEL.NUMBER) then
            -- Be forgiving with a 1.3 GAME.CON type error.
            pwarnprintf(pos, "label \"BLIMPRESPAWNTIME\" is not an `action' value, assuming 0")
            return "0"
        else
            perrprintf(pos, "label \"%s\" is not a%s `%s' value", identifier,
                       labeltype==LABEL.MOVE and "" or "n", LABEL[labeltype])
            return "_WRONGTYPE"
        end
    end

    return val
end

function check.reserved_bits(flags, allowedbits, suffix)
    local rbits = bit.bnot(allowedbits)
    if (bit.band(flags, rbits) ~= 0) then
        warnprintf("set one or more reserved bits (0x%s) "..suffix,
                   bit.tohex(bit.band(flags, rbits)))
    end
end

-- KEEPINSYNC control.lua
Define.ALLOWED_VIEWTYPE = truetab { 0, 1, 2, 3,4, 5, 7, 8, -5, -7, -8 }

function Define.composite(labeltype, identifier, ...)
    local oldtype = g_labeltype[identifier]
    local oldval = g_labeldef[identifier]

    if (oldval) then
        if (oldtype ~= labeltype) then
            errprintf("Refusing to overwrite `%s' label \"%s\" with a `%s' value.",
                      LABEL[oldtype], identifier, LABEL[labeltype])
            inform.olddef_location(identifier, true)
        else
            warnprintf("Duplicate `%s' definition of \"%s\" ignored.",
                       LABEL[labeltype], identifier)
            inform.olddef_location(identifier, false)
        end
        return
    end

    -- Fill up omitted arguments denoting composites with zeros.
    local isai = (labeltype == LABEL.AI)
    local args = {...}
    for i=#args+1,labeltype do
        -- Passing nil/nothing as remaining args to con.ai will make the
        -- action/move the null one.
        args[i] = (isai and i<=2) and "nil" or 0
    end

    if (isai) then
        assert(type(args[1])=="string")
        assert(type(args[2])=="string")

        -- OR together the flags
        for i=#args,LABEL.AI+1, -1 do
            args[LABEL.AI] = bit.bor(args[LABEL.AI], args[i])
            args[i] = nil
        end

        -- Check whether movflags use reserved bits.
        check.reserved_bits(args[LABEL.AI], 4096+2047, "for ai's movflags")
    end

    if (labeltype == LABEL.ACTION) then
        -- Sanity-check action members.
        -- KEEPINSYNC with ACTOR_CHECK in control.lua for consistency.
        if (not (args[2] >= 0)) then
            errprintf("action \"%s\" has negative number of frames", identifier)
        end
        if (Define.ALLOWED_VIEWTYPE[args[3]] == nil) then
            errprintf("action \"%s\" has disallowed viewtype %d", identifier, args[3])
        end
        if (not (args[4] >= -1 and args[4] <= 1)) then
            warnprintf("action \"%s\" has incval different from -1, 0 or 1", identifier)
        end
    end

    -- Make a string out of that.
    for i=1+(isai and 2 or 0),#args do
        args[i] = format("%d", args[i])
    end

    local refcode = mangle_name(identifier, LABEL_PREFIX[labeltype])
    addcodef(isai and "%s=_con.%s(%s)" or "%s=_con.%s{%s}",  -- ai has parens
             refcode, LABEL_FUNCNAME[labeltype], table.concat(args, ","))

    g_labeldef[identifier] = refcode
    g_labeltype[identifier] = labeltype
    g_labelloc[identifier] = getLocation()
end


local function parse(contents) end -- fwd-decl

local function do_include_file(dirname, filename, isroot)
    assert(type(filename)=="string")

    if (g_have_file[filename] ~= nil) then
        printf("[%d] Fatal error: infinite loop including \"%s\"", g_recurslevel, filename)
        g_numerrors = inf
        return
    end

    local contents

    if (read_into_string) then
        -- running from EDuke32
        contents = read_into_string(filename)
    else
        -- running stand-alone
        local io = require("io")

        local fd, msg = io.open(dirname..filename)
        while (fd == nil and not isroot and filename:find("/")) do
            -- strip up to and including first slash:
            filename = filename:gsub("^.-/", "")
            fd, msg = io.open(dirname..filename)
        end

        -- As a last resort, try the "default directory"
        if (fd==nil and not isroot and g_defaultDir) then
            -- strip up to and including last slash (if any):
            filename = filename:gsub("^.*/", "")
            dirname = g_defaultDir.."/"
            fd, msg = io.open(dirname..filename)
        end

        if (fd == nil) then
            printf("[%d] Fatal error: couldn't open %s", g_recurslevel, msg)
            g_numerrors = inf
            return
        end

        contents = fd:read("*all")
        fd:close()
    end

    if (contents == nil) then
        -- maybe that file name turned out to be a directory or other
        -- special file accidentally
        printf("[%d] Fatal error: couldn't read from \"%s\"",
               g_recurslevel, dirname..filename)
        g_numerrors = inf
        return
    end

    printf("%s[%d] Translating file \"%s\"", (g_recurslevel==-1 and "\n---- ") or "",
           g_recurslevel+1, dirname..filename);

    local oldfilename = g_filename
    g_filename = filename
    parse(contents)
    g_filename = oldfilename
end

-- Table of various outer command handling functions.
local Cmd = {}

function Cmd.NYI(msg)
    return function()
        errprintf(msg.." not yet implemented")
    end
end

function Cmd.nyi(msg)
    return function()
        warnprintf(msg.." not yet implemented")
    end
end

function Cmd.include(filename)
    do_include_file(g_directory, filename, false)
end

--- Per-module game data
local g_data = {}
local EPMUL = conl.MAXLEVELS

function reset.gamedata()
    g_data = {}

    -- [EPMUL*ep + lev] = { ptime=<num>, dtime=<num>, fn=<str>, name=<str> }
    g_data.level = {}
    -- [ep] = <str>
    g_data.volname = {}
    -- [skillnum] = <str>
    g_data.skillname = {}
    -- [quotenum] = <str>
    g_data.quote = {}
    -- table of length 26 or 30 containg numbers
    g_data.startup = {}
    -- [soundnum] = { fn=<str>, params=<table of length 5> }
    g_data.sound = {}
    -- [volnum] = <table of length numlevels (<= MAXLEVELS) of <str>>
    g_data.music = {}
end

-- TODO: PRE13 has no <dtstr> (3D Realms time).
function Cmd.definelevelname(vol, lev, fn, ptstr, dtstr, levname)
    if (not (vol >= 0 and vol < conl.MAXVOLUMES)) then
        errprintf("volume number exceeds maximum volume count.")
        return
    end

    if (not (lev >= 0 and lev < conl.MAXLEVELS)) then
        errprintf("level number exceeds maximum number of levels per episode.")
        return
    end

    -- TODO: Bcorrectfilename(fn)

    local function secs(tstr)
        local m, s = string.match(tstr, ".+:.+")
        m, s = tonumber(m), tonumber(s)
        return (m and s) and m*60+s or 0
    end

    local map = {
        ptime=secs(ptstr), dtime=secs(dtstr), fn="/"..fn, name=levname
    }

    if (ffi) then
        ffiC.C_DefineLevelName(vol, lev, map.fn, map.ptime, map.dtime, map.name)
    end

    g_data.level[EPMUL*vol+lev] = map
end

function Cmd.undefinelevel(vol, lev)
    if (not (vol >= 0 and vol < conl.MAXVOLUMES)) then
        errprintf("volume number exceeds maximum volume count.")
        return
    end

    if (not (lev >= 0 and lev < conl.MAXLEVELS)) then
        errprintf("level number exceeds maximum number of levels per episode.")
        return
    end

    if (ffi) then
        ffiC.C_UndefineLevel(vol, lev)
    end
end

local function defineXname(what, ffiCfuncname, X, name)
    if (ffi) then
        ffiC[ffiCfuncname](X, name)
        if (#name > 32) then
            warnprintf("%s %d name truncated to 32 characters.", what, X)
        end
    end
    return name
end

function Cmd.defineskillname(skillnum, name)
    if (not (skillnum >= 0 and skillnum < conl.MAXSKILLS)) then
        errprintf("skill number is negative or exceeds maximum skill count.")
        return
    end

    name = defineXname("skill", "C_DefineSkillName", skillnum, name)
    g_data.skillname[skillnum] = name
end

function Cmd.undefineskill(skillnum)
    if (not (skillnum >= 0 and skillnum < conl.MAXSKILLS)) then
        errprintf("skill number is negative or exceeds maximum skill count.")
        return
    end

    if (ffi) then
        ffiC.C_UndefineSkill(skillnum)
    end
end

function Cmd.definevolumename(vol, name)
    if (not (vol >= 0 and vol < conl.MAXVOLUMES)) then
        errprintf("volume number is negative or exceeds maximum volume count.")
        return
    end

    name = defineXname("volume", "C_DefineVolumeName", vol, name)
    g_data.volname[vol] = name
end

function Cmd.definevolumeflags(vol, flags)
    if (not (vol >= 0 and vol < conl.MAXVOLUMES)) then
        errprintf("volume number is negative or exceeds maximum volume count.")
        return
    end

    if (ffi) then
        ffiC.C_DefineVolumeFlags(vol, flags)
    end
end

function Cmd.undefinevolume(vol)
    if (not (vol >= 0 and vol < conl.MAXVOLUMES)) then
        errprintf("volume number is negative or exceeds maximum volume count.")
        return
    end

    if (ffi) then
        ffiC.C_UndefineVolume(vol)
    end
end

function Cmd.definegamefuncname(idx, name)
    local NUMGAMEFUNCTIONS = (ffi and ffiC.NUMGAMEFUNCTIONS or 56)
    if (not (idx >= 0 and idx < NUMGAMEFUNCTIONS)) then
        errprintf("function number exceeds number of game functions.")
        return
    end

    assert(type(name)=="string")
    -- XXX: in place of C-CON's "invalid character in function name" report:
    name = name:gsub("[^A-Za-z0-9]", "_")

    if (ffi) then
        ffiC.C_DefineGameFuncName(idx, name)
    end
end

function Cmd.definegametype(idx, flags, name)
    if (not (idx >= 0 and idx < conl.MAXGAMETYPES)) then
        errprintf("gametype number exceeds maximum gametype count.")
        return
    end

    if (ffi) then
        ffiC.C_DefineGameType(idx, flags, name)
    end
end

-- strip whitespace from front and back
local function stripws(str)
    return str:match("^%s*(.*)%s*$")
end

function Cmd.definequote(qnum, quotestr)
    if (not (qnum >= 0 and qnum < conl.MAXQUOTES)) then
        errprintf("quote number is negative or exceeds limit of %d.", conl.MAXQUOTES-1)
        return ""
    end

    quotestr = stripws(quotestr)

    if (#quotestr >= conl.MAXQUOTELEN) then
        -- NOTE: Actually, C_DefineQuote takes care of this! That is,
        -- standalone, the string isn't truncated.
        warnprintf("quote %d truncated to %d characters.", qnum, conl.MAXQUOTELEN-1)
    end

    if (ffi) then
        ffiC.C_DefineQuote(qnum, quotestr)
    end

    g_data.quote[qnum] = quotestr
    return ""
end

local PROJ = {}
for key, val in pairs(conl.PROJ) do
    -- Strip "PROJ_"
    PROJ[key:sub(6)] = val
end

function Cmd.defineprojectile(tilenum, what, val)
    local ok = check.tile_idx(tilenum)

    if (what==PROJ.WORKSLIKE) then
        check.reserved_bits(val, 2^21-1, "for PROJ_WORKSLIKE")
    elseif (what==PROJ.SOUND or what==PROJ.ISOUND or what==PROJ.BSOUND) then
        ok = ok and (val==-1 or check.sound_idx(val))
    elseif (what==PROJ.SPAWNS or what==PROJ.DECAL or what==PROJ.TRAIL) then
        ok = ok and (val==-1 or check.tile_idx(val))
    end

    if (ffi and ok) then
        ffiC.C_DefineProjectile(tilenum, what, val)
    end
end

-- <override>: override-set flags? The default is to bitwise OR with existing.
function Cmd.xspriteflags(tilenum, flags, override)
    local ok = check.tile_idx(tilenum)
    check.reserved_bits(flags, conl.user_sflags, "for sprite flags")

    local loc = g_code.aflagsloc[tilenum]

    if (override and loc ~= nil) then
        warnprintf("'spriteflags' after %s %d", loc[4], tilenum)
        contprintf(false, "at %s %d:%d", loc[1], loc[2], loc[3])
    end

    -- Mark the last 'spriteflags' or 'sprite*' directive for the given actor.
    g_code.aflagsloc[tilenum] = getLocation(format("'%s' for actor", g_lastkw))

    if (ffi and ok) then
        local tile = ffiC.g_tile[tilenum]
        tile._flags = bit.bor(override and 0 or tile._flags, flags)
    end
end

function Cmd.precache(tilenum0, tilenum1, flagnum)
    local ok = check.tile_idx(tilenum0) and check.tile_idx(tilenum1)

    if (ffi and ok) then
        local tile = ffiC.g_tile[tilenum0]
        tile._cacherange = tilenum1;
        if (flagnum) then
            tile._flags = bit.bor(tile._flags, conl.SFLAG.SFLAG_CACHE)
        end
    end
end

function Cmd.cheatkeys(sc1, sc2)
    if (ffi) then
        ffiC.CheatKeys[0] = sc1
        ffiC.CheatKeys[1] = sc2
    end
end

function Cmd.setdefname(filename)
    assert(type(filename)=="string")
    if (ffi) then
        if (ffiC.C_SetDefName(filename) ~= 0) then
            error("OUT OF MEMORY", 0)
        end
    end
end

function Cmd.setcfgname(filename)
    assert(type(filename)=="string")
    if (ffi) then
        ffiC.C_SetCfgName(filename)
    end
end

function Cmd.gamestartup(...)
    local args = {...}

    -- TODO: PRE13: detection of other g_scriptVersion.
    if (#args ~= 26 and #args ~= 30) then
        errprintf("must pass either 26 (1.3D) or 30 (1.5) values")
        return
    end

    if (ffi) then
        -- running from EDuke32
        if (#args == 30) then
            ffiC.g_scriptVersion = 14
        end
        local params = ffi.new("int32_t [30]", args)
        ffiC.G_DoGameStartup(params)
    end

    g_data.startup = args  -- TODO: sanity-check them
end

function Cmd.definesound(sndlabel, fn, ...)
    local sndnum

    if (type(sndlabel)=="string") then
        -- HANDLE_RAWDEFINE
        local pos, minus, label = sndlabel:match("(.-):(.-):(.+)")
        sndnum = lookup.defined_label(tonumber(pos), minus, label)

        if (ffi and g_dynsoundi and (sndnum>=0 and sndnum<conl.MAXSOUNDS)) then
            dynmap.maybe_init(g_dynsoundi, ffiC.g_dynSoundList)
            dynmap.maybe_process(g_dynsoundi, ffiC.g_dynSoundList, label, sndnum)
        end
    else
        assert(type(sndlabel)=="number")
        sndnum = sndlabel
    end

    if (not (sndnum >= 0 and sndnum < conl.MAXSOUNDS)) then
        errprintf("sound number is negative or exceeds sound limit of %d", conl.MAXSOUNDS-1)
        return
    end

    local params = {...}  -- TODO: sanity-check them some more
    check.reserved_bits(params[4], 31+128, "for sound flags")

    if (ffi) then
        local cparams = ffi.new("int32_t [5]", params)
        assert(type(fn)=="string")
        ffiC.C_DefineSound(sndnum, fn, cparams)
    end

    g_data.sound[sndnum] = { fn=fn, params=params }
end

function Cmd.music(volnum, ...)
    if (not (volnum >= 0 and volnum <= conl.MAXVOLUMES+1)) then
        -- The passed volume number is 1-based.
        -- Both 0 and MAXVOLUMES+1 means "special music"
        errprintf("volume number must be between 0 and MAXVOLUMES+1=%d", conl.MAXVOLUMES+1)
        return
    elseif (volnum == conl.MAXVOLUMES+1) then
        warnprintf("volume number MAXVOLUMES+1 is discouraged, use 0 instead")
    end

    if (volnum == 0) then
        volnum = conl.MAXVOLUMES+1  -- special music
    end

    local filenames = {...}
    local MAXFNS = conl.MAXLEVELS

    if (#filenames > MAXFNS) then
        warnprintf("ignoring extraneous %d music file names", #filenames-MAXFNS)
        for i=MAXFNS+1,#filenames do
            filenames[i] = nil
        end
    end

    if (ffi) then
        for i=1,#filenames do
            assert(type(filenames[i])=="string")
            ffiC.C_DefineMusic(volnum-1, i-1, "/"..filenames[i])
        end
    end

    g_data.music[volnum] = filenames
end


--- GAMEVARS / GAMEARRAYS

function Cmd.gamearray(identifier, initsize)
    if (check.sysvar_def_attempt(identifier)) then
        return
    end

    if (not (initsize >= 0 and initsize < 0x7fffffff)) then
        errprintf("invalid initial size %d for gamearray `%s'", initsize, identifier)
        return
    end

    local oga = g_gamearray[identifier]
    if (oga) then
        if (oga.sysp) then
            errprintf("attempt to define system gamearray `%s'", identifier)
            return
        elseif (initsize ~= oga.size) then
            errprintf("duplicate gamearray definition `%s' has different size", identifier)
            return
        else
            warnprintf("duplicate gamearray definition `%s' ignored", identifier)
            return
        end
    end

    if (g_gamevar[identifier]) then
        warnprintf("symbol `%s' already used for game variable", identifier)
        inform.oldgv_location(identifier, false)
    end

    local ga = { name=mangle_name(identifier, "A"), size=initsize }
    g_gamearray[identifier] = ga

    addcode("if _S then")
    addcodef("%s=_con._gamearray(%d)", ga.name, initsize)
    addcode("end")
end

function Cmd.gamevar(identifier, initval, flags)
    if (check.sysvar_def_attempt(identifier)) then
        return
    end

    if (bit.band(flags, bit.bnot(GVFLAG.USER_MASK)) ~= 0) then
        -- TODO: a couple of the presumably safe ones
        errprintf("gamevar flags other than 1, 2, 1024 or 131072: NYI or forbidden")
        return
    end

    local perPlayer = (bit.band(flags, GVFLAG.PERPLAYER) ~= 0)
    local perActor = (bit.band(flags, GVFLAG.PERACTOR) ~= 0)

    if (perPlayer and perActor) then
        errprintf("invalid gamevar flags: must be either PERPLAYER or PERACTOR, not both")
        return
    end

    local ogv = g_gamevar[identifier]
    -- handle NORESET or NODEFAULT
    local isSessionVar = (bit.band(flags, GVFLAG.NODEFAULT) ~= 0)
    local storeWithSavegames = (bit.band(flags, GVFLAG.NORESET) == 0)

    local actorVarSuffix = ""

    if (isSessionVar and (perPlayer or perActor)) then
        if (perActor) then
            actorVarSuffix = ",nil,true"
--            flags = bit.band(flags, bit.bnot(GVFLAG.NODEFAULT))
            isSessionVar = false
        elseif (perPlayer) then
            flags = bit.band(flags, bit.bnot(GVFLAG.PERPLAYER))
            perPlayer = false

            if (ogv == nil) then  -- warn only once per gamevar
                warnprintf("per-player session gamevar `%s': NYI, made global", identifier)
            end
        end
    end

    if (ogv ~= nil) then
        local oflags = bit.band(ogv.flags, bit.bnot(GVFLAG.CON_PERPLAYER))

        if (oflags ~= flags) then
            if (bit.band(oflags, GVFLAG.SYSTEM) ~= 0 and not isSessionVar) then
                -- Attempt to override a system gamevar. See if it's read-only...
                if (bit.band(oflags, GVFLAG.READONLY) ~= 0) then
                    errprintf("attempt to override read-only system gamevar `%s'", identifier)
                    return
                end

                local flagsnosys = bit.band(oflags, bit.bnot(GVFLAG.SYSTEM))
                if (flagsnosys ~= flags and g_warn["system-gamevar"]) then
                    warnprintf("overrode initial value of `%s', but kept "..
                               "flags (%d)", identifier, flagsnosys)
                end

                if (ogv.rbits and bit.band(ogv.rbits, initval)~=0) then
                    warnprintf("set one or more reserved bits (0x%s) in overriding `%s'",
                               bit.tohex(bit.band(ogv.rbits, initval)), identifier)
                end

                local linestr = "--"..getlinecol(g_lastkwpos)

                -- Emit code to set the variable at Lua parse time.
                -- XXX: How does this interact with savegame restoration?
                if (bit.band(oflags, GVFLAG.PERPLAYER) ~= 0) then
                    -- Replace player index by 0. PLAYER_0.
                    -- TODO_MP: init for all players.
                    local pvar, numrepls = ogv.name:gsub("_pli", "0")
                    assert(numrepls>=1)
                    addcodef("%s=%d%s", pvar, initval, linestr)
                else
                    addcodef("%s=%d%s", ogv.name, initval, linestr)
                end
                return
            end

            errprintf("duplicate definition of gamevar `%s' has different flags (new: %x, old: %x)", identifier, flags, oflags)
            inform.oldgv_location(identifier, true)
            return
        else
            warnprintf("duplicate definition of gamevar `%s' ignored", identifier)
            inform.oldgv_location(identifier, false)
            return
        end
    end

    local ltype = g_labeltype[identifier]
    if (ltype ~= nil) then
        warnprintf("Symbol `%s' already used for a defined %s.", identifier, LABEL[ltype])
        inform.olddef_location(identifier, false)
    end

    if (isSessionVar) then
        if (g_numSessionVars == conl.MAXSESSIONVARS) then
            errprintf("Declared too many session gamevars (flag 1024), can have at most %d.",
                      conl.MAXSESSIONVARS)
            return
        end

        -- Declare new session gamevar.
        local gv = { name=format("_gv._sessionVar[%d]", g_numSessionVars),
                     flags=flags, loc=getLocation(), used=0 }
        g_numSessionVars = g_numSessionVars+1

        g_gamevar[identifier] = gv;
        -- Initialize it (i.e. set to the declared initial value) on first run,
        -- but not from savegames.
        addcodef("if _S then %s=%d end", gv.name, initval)

        return
    end

    local gv = { name=mangle_name(identifier, "V"), flags=flags, loc=getLocation(), used=0 }
    g_gamevar[identifier] = gv

    if (storeWithSavegames) then
        addcode("if _S then")
    end

    if (perActor) then
        addcodef("%s=_con.actorvar(%d%s)", gv.name, initval, actorVarSuffix)
    elseif (perPlayer and g_cgopt["playervar"]) then
        gv.flags = bit.bor(gv.flags, GVFLAG.CON_PERPLAYER)
        addcodef("%s=_con.playervar(%d)", gv.name, initval)
    else
        addcodef("%s=%d", gv.name, initval)
    end

    if (storeWithSavegames) then
        addcode("end")
    end
end

function Cmd.dynamicremap()
    if (g_dyntilei==nil) then
        print("Using dynamic tile remapping");
        g_dyntilei = {};
    end
end

function Cmd.dynamicsoundremap()
    if (g_dynsoundi==nil) then
        print("Using dynamic sound remapping");
        g_dynsoundi = {};
    end
end

function lookup.gamearray(identifier)
    local ga = g_gamearray[identifier]
    if (ga == nil) then
        errprintf("symbol `%s' is not a game array", identifier)
        return "_INVALIDGA"
    end
    return ga.name
end

local function thisactor_to_pli(var)
    return (var=="_aci") and "_pli" or var
end

function lookup.error_not_gamevar(identifier)
    errprintf("symbol `%s' is not a game variable", identifier)
    return "_INVALIDGV"
end

-- <aorpvar>: code for actor or player index
function lookup.gamevar(identifier, aorpvar, writable)
    local gv = g_gamevar[identifier]

    if (gv == nil) then
        return lookup.error_not_gamevar(identifier)
    end

    if (writable and bit.band(gv.flags, GVFLAG.READONLY) ~= 0) then
        errprintf("gamevar `%s' is read-only", identifier)
        return "_READONLYGV"
    end

    gv.used = bit.bor(gv.used, writable and 2 or 1)

    if (bit.band(gv.flags, GVFLAG.PERACTOR)~=0) then
        return format("%s[%s]", gv.name, aorpvar)
    elseif (bit.band(gv.flags, GVFLAG.CON_PERPLAYER)~=0 and g_cgopt["playervar"]) then
        return format("%s[%s]", gv.name, thisactor_to_pli(aorpvar))
    else
        return gv.name
    end
end

local function maybe_gamevar_Cmt(subj, pos, identifier)
    if (g_gamevar[identifier]) then
        return true, lookup.gamevar(identifier, "_aci", false)
    end
end


----==== patterns ====----

---- basic ones
-- Windows, *nix and Mac newlines all exist in the wild!
local newline = "\r"*Pat("\n")^-1 + "\n"
local EOF = Pat(-1)
local anychar = Pat(1)
-- comments
local comment = "/*" * match_until(anychar, "*/") * "*/"
local linecomment = "//" * match_until(anychar, newline)
local whitespace = Var("whitespace")
local sp0 = whitespace^0
-- This "WS+" pattern matches EOF too, so that a forgotten newline at EOF is
-- properly handled
local sp1 = whitespace^1 + EOF
local alpha = Range("AZ", "az")  -- locale?
local alphanum = alpha + Range("09")
--local alnumtok = alphanum + Set("{}/\\*-_.")  -- see isaltok() in gamedef.c

--- Basic lexical elements ("tokens"). See the final grammar ("Grammar") for
--- their definitions.
local tok =
{
    maybe_minus = (Pat("-") * sp0)^-1,
    number = Var("t_number"),

    -- Valid identifier names are disjoint from keywords!
    -- XXX: CON is more permissive with identifier name characters:
    identifier = Var("t_identifier"),
    -- This one matches keywords, too:
    identifier_all = Var("t_identifier_all"),

    define = Var("t_define"),
    rawdefine = Var("t_rawdefine"),
    actordefine = g_cgopt["names"] and Var("t_rawdefine") or Var("t_define"),

    move = Var("t_move"),
    ai = Var("t_ai"),
    action = Var("t_action"),

    -- NOTE: no chance to whitespace and double quotes in filenames:
    filename = lpeg.C((anychar-Set(" \t\r\n\""))^1),
    newline_term_str = match_until(anychar, newline),

    rvar = Var("t_rvar"),
    wvar = Var("t_wvar"),
    gamearray = Var("t_gamearray"),

    -- for definelevelname
    time = lpeg.C(alphanum*alphanum^-1*":"*alphanum*alphanum^-1),

    state_ends = Pat("ends")
        + POS() * "else" * sp1 * "ends"
        / function(pos) pwarnprintf(pos, "stray `else' at end of state") end,
}


---- helper patterns / pattern constructing functions
local maybe_quoted_filename = ('"' * tok.filename * '"' + tok.filename)
-- empty string is handled too; we must not eat the newline then!
local newline_term_string = (#newline + EOF)*lpeg.Cc("")
    + (whitespace-newline)^1 * lpeg.C(tok.newline_term_str)


-- (sp1 * tok.define) repeated exactly n times
local function n_defines(n)  -- works well only for small n
    local pat = Pat(true)
    for i=1,n do
        pat = sp1 * tok.define * pat
    end
    return pat
end


local D, R, W, I, GARI, AC, MV, AI = -1, -2, -3, -4, -5, -6, -7, -8
local TOKEN_PATTERN = { [D]=tok.define, [R]=tok.rvar, [W]=tok.wvar,
                        [I]=tok.identifier, [GARI]=tok.gamearray,
                        [AC]=tok.action, [MV]=tok.move, [AI]=tok.ai }

-- Generic command pattern, types given by varargs.
-- The command name to be matched is attached later.
-- Example:
--  "command" writtenvar readvar def def:  gencmd(W,R,D,D)
--    -->  sp1 * tok.wvar * sp1 * tok.rvar * sp1 * tok.define * sp1 * tok.define
--  "command_with_no_args":  gencmd()
--    --> Pat(true)
local function cmd(...)
    local pat = Pat(true)
    local vartypes = {...}

    for i=1,#vartypes do
        pat = pat * sp1 * assert(TOKEN_PATTERN[vartypes[i]])
    end

    return pat
end


-- The command names will be attached to the front of the patterns later!

--== Top level CON commands ==--
-- XXX: many of these are also allowed inside actors/states/events in CON.
local Couter = {
    --- 1. Preprocessor
    include = sp1 * maybe_quoted_filename
        / Cmd.include,
    includedefault = cmd()
        / Cmd.NYI("`includedefault'"),
    define = cmd(I,D)
        / Define.label,

    --- 2. Defines and Meta-Settings
    dynamicremap = cmd()
        / Cmd.dynamicremap,
    dynamicsoundremap = cmd()
        / Cmd.dynamicsoundremap,
    setcfgname = sp1 * tok.filename
        / Cmd.setcfgname,
    setdefname = sp1 * tok.filename
        / Cmd.setdefname,
    setgamename = newline_term_string
        / Cmd.nyi("`setgamename'"),

    precache = cmd(D,D,D)
        / Cmd.precache,
    scriptsize = cmd(D)
        / "",  -- no-op
    cheatkeys = cmd(D,D)
        / Cmd.cheatkeys,

    definecheat = newline_term_string  -- XXX: actually tricker syntax (TS)
        , -- / Cmd.nyi("`definecheat'"),
    definegamefuncname = sp1 * tok.define * newline_term_string  -- XXX: TS?
        / Cmd.definegamefuncname,
    definegametype = n_defines(2) * newline_term_string
        / Cmd.definegametype,
    definelevelname = n_defines(2) * sp1 * tok.filename * sp1 * tok.time * sp1 * tok.time *
        newline_term_string
        / Cmd.definelevelname,
    defineskillname = sp1 * tok.define * newline_term_string
        / Cmd.defineskillname,
    definevolumename = sp1 * tok.define * newline_term_string
        / Cmd.definevolumename,

    definequote = sp1 * tok.define * newline_term_string
        / Cmd.definequote,
    defineprojectile = cmd(D,D,D)
        / Cmd.defineprojectile,
    definesound = sp1 * tok.rawdefine * sp1 * maybe_quoted_filename * n_defines(5)
        / Cmd.definesound,

    -- NOTE: gamevar.ogg and the like is OK, too
    music = sp1 * tok.define * match_until(sp1 * tok.filename, sp1 * conl.keyword * sp1)
        / Cmd.music,

    definevolumeflags = cmd(D,D)
        / Cmd.definevolumeflags,

    undefinelevel = cmd(D,D)
        / Cmd.undefinelevel,
    undefineskill = cmd(D)
        / Cmd.undefineskill,
    undefinevolume = cmd(D)
        / Cmd.undefinevolume,

    --- 3. Game Settings
    -- gamestartup has 26/30 fixed defines, depending on 1.3D/1.5 version:
    gamestartup = (sp1 * tok.define)^26
        / Cmd.gamestartup,
    spritenopal = cmd(D)
        / function(tilenum, flags) Cmd.xspriteflags(tilenum, conl.SFLAG.SFLAG_NOPAL) end,
    spritenoshade = cmd(D)
        / function(tilenum, flags) Cmd.xspriteflags(tilenum, conl.SFLAG.SFLAG_NOSHADE) end,
    spritenvg = cmd(D)
        / function(tilenum, flags) Cmd.xspriteflags(tilenum, conl.SFLAG.SFLAG_NVG) end,
    spriteshadow = cmd(D)
        / function(tilenum, flags) Cmd.xspriteflags(tilenum, conl.SFLAG.SFLAG_SHADOW) end,

    spriteflags = (sp1 * tok.define)^2  -- also see inner
        / function(tilenum, ...) Cmd.xspriteflags(tilenum, bit.bor(...), true) end,

    --- 4. Game Variables / Arrays
    gamevar = cmd(I,D,D)
        / Cmd.gamevar,
    gamearray = cmd(I,D)
        / Cmd.gamearray,

    --- 5. Top level commands that are also run-time commands
    move = sp1 * tok.identifier * (sp1 * tok.define)^-2  -- hvel, vvel
        / function(...) Define.composite(LABEL.MOVE, ...) end,

    -- startframe, numframes, viewtype, incval, delay, flags:
    action = sp1 * tok.identifier * (sp1 * tok.define)^-6 -- ACTION_PARAM_COUNT
        / function(...) Define.composite(LABEL.ACTION, ...) end,

    -- action, move, flags...:
    ai = sp1 * tok.identifier * (sp1 * tok.action *
                                 (sp1 * tok.move * (sp1 * tok.define)^0)^-1
                                )^-1
        / function(...) Define.composite(LABEL.AI, ...) end,

    --- 6. Deprecated TLCs
    betaname = newline_term_string,
    enhanced = cmd(D),
}


--== Run time CON commands ==--
--- 1. Gamevar Operators
local Op = {}
Op.var = cmd(W,D)
Op.varvar = cmd(W,R)

function Op.var_common(thecmd, defaultop, trapop, wrapop)
    local theop =
        g_cgopt["trapv"] and trapop or
        g_cgopt["wrapv"] and wrapop or
        assert(defaultop)

    if (#theop <= 2) then
        return thecmd / ("%1=%1"..theop.."%2")
    else
        return thecmd / ("%1="..theop.."(%1,%2)")
    end
end

function Op.varf(...)
    return Op.var_common(Op.var, ...)
end

function Op.varvarf(...)
    return Op.var_common(Op.varvar, ...)
end

-- Allow nesting... stuff like
--   ifvarl actorvar[sprite[THISACTOR].owner].burning 0
-- is kinda breaking the classic "no array nesting" rules
-- (if there ever were any) but making our life harder else.
local arraypat = sp0 * "[" * sp0 * tok.rvar * sp0 * "]"
-- For {get,set}userdef:
local arraypat_maybe_empty = sp0 * "[" * sp0 * (tok.rvar * sp0)^-1 * "]"

-- Table of various patterns that are (parts of) more complex inner commands.
local patt = {}

-- Have to bite the bullet here and list actor/player members with second
-- parameters, even though it's ugly to make it part of the syntax.  Also,
-- stuff like
--   actor[xxx].loogiex parm2 x
-- will be wrongly accepted at the parsing stage (loogiex is player's member)
-- because we don't discriminate between actor and player here.
patt.parm2member = lpeg.C(Pat("htg_t") + "loogiex" + "loogiey" + "ammo_amount" +
                          "weaprecs" + "gotweapon" + "pals" + "Pals" + "max_ammo_amount") * sp1 * tok.rvar
-- XXX: "pals" + "Pals": this sucks! It means that we for this list of members
-- requiring second parameters, we will have to enumerate all lower/uppercase
-- instances encountered in the wild.

-- The member name must match keywords, too (_all), because e.g. cstat is a
-- member of sprite[].
patt.bothmember = sp0 * "." * sp0 * lpeg.Ct(patt.parm2member + tok.identifier_all)
patt.singlemember = sp0 * "." * sp0 * tok.identifier_all

patt.cmdgetstruct =  -- get<structname>[<idx>].<member> (<parm2>)? <<var>>
    arraypat * patt.bothmember * sp1 * tok.wvar

patt.cmdsetstruct =  -- set<structname>[<idx>].<<member>> (<parm2>)? <var>
    arraypat * patt.bothmember * sp1 * tok.rvar

patt.cmdgetperxvar =  -- get<actor/player>var[<idx>].<varname> <<var>>
    arraypat * patt.singlemember * sp1 * tok.wvar

patt.cmdsetperxvar = -- set<actor/player>var[<idx>].<<varname>> <var>
    arraypat * patt.singlemember * sp1 * tok.rvar

-- Function generating code for a struct read/write access.
local function StructAccess(Structname, writep, index, membertab)
    assert(type(membertab)=="table")
    -- Lowercase the member name for CON compatibility
    local member, parm2 = membertab[1]:lower(), membertab[2]

    local MemberCode = conl.StructAccessCode[Structname] or conl.StructAccessCode2[Structname]
    -- Look up array+member name first, e.g. "spriteext[%s].angoff".
    local armembcode = MemberCode[member]
    if (armembcode == nil) then
        errprintf("%s: invalid %s member `.%s'", g_lastkw, Structname, member)
        return "_MEMBINVALID"
    end

    -- Function checking a literal number for being OK for assignment to this
    -- member. Can also be a table {min, max}. See con_lang.lua, LITERAL_CHECKING.
    local lit_ok_func_or_table

    if (type(armembcode)=="table") then
        -- Read and write accesses differ.
        if (writep) then
            lit_ok_func_or_table = armembcode[3]
        end
        armembcode = armembcode[writep and 2 or 1]
        if (armembcode==nil) then
            errprintf("%s access to %s[].%s is not available",
                      writep and "write" or "read", Structname, member)
            return "_MEMBNOACCESS"
        end
    end

    if (Structname~="userdef") then
        -- Count number of parameters ("%s"), don't count "%%s".
        local _, numparms = armembcode:gsub("[^%%]%%s", "", 2)
        if (#membertab ~= numparms) then
            local nums = { "one", "two" }
            errprintf("%s[].%s has %s parameter%s, but %s given", Structname,
                      member, nums[numparms], numparms==1 and "" or "s",
                      nums[#membertab])
            return "_MEMBINVPARM"
        end
    end

    -- THISACTOR special meanings
    if (Structname=="player" or Structname=="input") then
        index = thisactor_to_pli(index)
    elseif (Structname=="sector") then
        if (index=="_aci") then
            index = SPS".sectnum"
        end
    end

    -- METHOD_MEMBER
    local ismethod = (armembcode:find("%%s",1,true)~=nil)
    -- If ismethod is true, then the formatted string will now have an "%s"
    local code

    if (Structname=="userdef") then
--        assert(index==nil)
        assert(parm2==nil)
        code = format(armembcode, parm2)
    else
        code = format(armembcode, index, parm2)
    end

    if (csapp()) then
        if (Structname=="player") then
            code = code:gsub("^player%[_pli%]", "_ps")
        elseif (Structname=="sprite") then
            code = code:gsub("^actor%[_aci%]", "_a")
            code = code:gsub("^sprite%[_aci%]", "_spr")
        end
    end

    return code, ismethod, lit_ok_func_or_table
end

function lookup.array_expr(writep, structname, index, membertab)
    if (conl.StructAccessCode[structname] == nil) then
        -- Try a gamearray
        local ganame = g_gamearray[structname] and lookup.gamearray(structname)
        if (ganame == nil) then
            if (structname=="actorvar") then
                -- actorvar[] inline array expr
                -- XXX: kind of CODEDUP with GetOrSetPerxvarCmd() factory
                local gv = g_gamevar[structname]
                if (gv and bit.band(gv.flags, GVFLAG.PERX_MASK)~=GVFLAG.PERACTOR) then
                    errprintf("gamevar `%s' is not per-actor", structname, "actor")
                    -- TODO: inform.gv_location()?
                end

                if (membertab == nil) then
                    errprintf("actorvar[] requires a pseudo member (gamevar) name")
                    return "_INVALIDAV"
                end

                if (#membertab > 1) then
                    errprintf("actorvar[] cannot be used with a second parameter")
                    return "_INVALIDAV"
                end

                if (gv) then
                    gv.used = bit.bor(gv.used, writep and 2 or 1)
                end

                assert(#membertab == 1)
                return lookup.gamevar(membertab[1], index, writep)
            end

            errprintf("symbol `%s' is neither a struct nor a gamearray", structname)
            return "_INVALIDAR"
        end

        if (membertab ~= nil) then
            errprintf("gamearrays cannot be indexed with member names")
            return "_INVALIDAR"
        end

        assert(type(ganame)=="string")
        return format("%s[%s]", ganame, index)
    end

    local membercode, ismethod = StructAccess(structname, writep, index, membertab)
    -- Written METHOD_MEMBER syntax not supported as "qwe:method(asd) = val"
    -- isn't valid Lua syntax.
    assert(not (writep and ismethod))
    return membercode
end

local Access =
{
    sector = function(...) return StructAccess("sector", ...) end,
    wall = function(...) return StructAccess("wall", ...) end,
    xsprite = function(...) return StructAccess("sprite", ...) end,
    player = function(...) return StructAccess("player", ...) end,

    tspr = function(...) return StructAccess("tspr", ...) end,
    projectile = function(...) return StructAccess("projectile", ...) end,
    thisprojectile = function(...) return StructAccess("thisprojectile", ...) end,
    userdef = function(...) return StructAccess("userdef", ...) end,
    input = function(...) return StructAccess("input", ...) end,
}

local function GetStructCmd(accessfunc, pattern)
    return (pattern or patt.cmdgetstruct) /
      function(idx, memb, var)
        return format("%s=%s", var, accessfunc(false, idx, memb))
      end
end

local function SetStructCmd(accessfunc, pattern)
    local function capfunc(idx, memb, var)
        -- litok: function or table
        local membercode, ismethod, litok = accessfunc(true, idx, memb)

        -- Light static checking for literal values being OK for member
        -- assignment. LITERAL_CHECKING.
        if (type(var)=="number" and litok) then
            if (type(litok)=="table" and not (var>=litok[1] and var<=litok[2]) or
                    type(litok)=="function" and not litok(var)) then
                local member = memb[1]:lower()
                warnprintf("setting member '.%s' to %d will fail at game time",
                           member, var)
            end
        end

        if (ismethod) then
            -- METHOD_MEMBER syntax

            -- BE EXTRA CAREFUL! We must be sure that percent characters have
            -- not been smuggled into the member code string via variable names
            -- etc.
            local _, numpercents = membercode:gsub("%%", "", 2)
            assert(numpercents==1)

            return format(membercode, var)
        else
            return format("%s=%s", membercode, var)
        end
    end

    return (pattern or patt.cmdsetstruct) / capfunc
end

-- <Setp>: whether the perxvar is set
local function GetOrSetPerxvarCmd(Setp, Actorp)
    local EXPECTED_PERX_BIT = Actorp and GVFLAG.PERACTOR or GVFLAG.PERPLAYER
    local pattern = (Setp and patt.cmdsetperxvar or patt.cmdgetperxvar)

    local function capfunc(idx, perxvarname, var)
        local gv = g_gamevar[perxvarname]
        if (gv and bit.band(gv.flags, GVFLAG.PERX_MASK)~=EXPECTED_PERX_BIT) then
            -- [gs]set*var for wrong gamevar type. See if it's a getactorvar,
            -- in which case we may only warn and access that instead. Note
            -- that accesses of player gamevars with actor indices are usually
            -- meaningless.
            local warnp = not Setp and Actorp and not g_warn["error-bad-getactorvar"]
            local xprintf = warnp and warnprintf or errprintf

            xprintf("gamevar `%s' is not per-%s", perxvarname, Actorp and "actor" or "player")
            inform.gv_location(perxvarname, not warnp)

            if (warnp and bit.band(gv.flags, GVFLAG.PERX_MASK)==GVFLAG.PERPLAYER
                    and g_cgopt["bad-getactorvar-use-pli"]) then
                -- For getactorvar[] accesses to per-player gamevars, if
                -- -fbad-getactorvar-use-pli is provided, use current player
                -- index, for compatibility with CON.
                idx = "_pli"
            end
        end

        if (not Actorp) then
            -- THISACTOR -> player index for {g,s}etplayervar
            idx = thisactor_to_pli(idx)
        end

        if (gv) then
            gv.used = bit.bor(gv.used, Setp and 2 or 1)
        end

        if (Setp) then
            return format("%s=%s", lookup.gamevar(perxvarname, idx, true), var)
        else
            return format("%s=%s", var, lookup.gamevar(perxvarname, idx, false))
        end
    end

    return pattern / capfunc
end


local function n_s_fmt(n)
    return string.rep("%s,", n-1).."%s"
end

-- Various inner command handling functions / string capture strings.
local handle =
{
    NYI = function()
        errprintf("command `%s' not yet implemented", g_lastkw)
        return ""
    end,

    dynNYI = function()
        return format([[print(%q..":%d: `%s' not yet implemented")]],
                      g_filename, getlinecol(g_lastkwpos), g_lastkw)
    end,

    addlog = function()
        return format("print(%q..':%d: addlog')", g_filename, getlinecol(g_lastkwpos))
    end,

    addlogvar = function(val)
        return format("printf(%q..':%d: addlogvar %%s', %s)", g_filename, getlinecol(g_lastkwpos), val)
    end,

    debug = function(val)
        return format("print(%q..':%d: debug %d')", g_filename, getlinecol(g_lastkwpos), val)
    end,

    getzrange = function(...)
        local v = {...}
        assert(#v == 10)  -- 4R 4W 2R
        return format("%s,%s,%s,%s=_con._getzrange(%s,%s,%s,%s,%s,%s)",
                      v[5], v[6], v[7], v[8],  -- outargs
                      v[1], v[2], v[3], v[4], v[9], v[10])  -- inargs
    end,

    hitscan = function(...)
        local v = {...}
        assert(#v == 14)  -- 7R 6W 1R
        local vals = {
            v[8], v[9], v[10], v[11], v[12], v[13],  -- outargs
            v[1], v[2], v[3], v[4], v[5], v[6], v[7], v[14]  -- inargs
        }
        return format("%s,%s,%s,%s,%s,%s=_con._hitscan(%s,%s,%s,%s,%s,%s,%s,%s)",
                     unpack(vals))
    end,

    neartag = function(...)
        local v = {...}
        assert(#v == 11)  -- 5R 4W 2R
        local vals = {
            v[6], v[7], v[8], v[9],  -- outargs
            v[1], v[2], v[3], v[4], v[5], v[10], v[11]  -- inargs
        }
        return format("%s,%s,%s,%s=_con._neartag(%s,%s,%s,%s,%s,%s,%s)",
                      unpack(vals))
    end,

    clipmove = function(noslidep, ...)
        local v = {...}
        assert(#v == 11)  -- 3W 1R 1W 6R
        local vals = {
            v[1], v[2], v[3], v[5],  -- outargs
            v[2], v[3], v[4], v[5], v[6], v[7], v[8], v[9], v[10], v[11],  -- inargs
            noslidep
        }
        return format("%s,%s,%s,%s=_con._clipmovex("..n_s_fmt(11)..")",
                     unpack(vals))
    end,

    palfrom = function(...)
        local v = {...}
        return format(PLS":_palfrom(%d,%d,%d,%d)",
                      v[1] or 0, v[2] or 0, v[3] or 0, v[4] or 0)
    end,

    qsprintf = function(qdst, qsrc, ...)
        local codes = {...}
        return format("_con._qsprintf(%s,%s%s%s)", qdst, qsrc,
                      #codes>0 and "," or "", table.concat(codes, ','))
    end,

    move = function(mv, ...)
        local flags = {...}
        return format(ACS":set_move(%s,%d)", mv, (flags[1] and bit.bor(...)) or 0)
    end,

    rotatesprite = function(...)
        return format("_con._rotspr(%s,%s,%s,%s,%s,%s,%s,%s,0,%s,%s,%s,%s)", ...)
    end,

    rotatesprite16 = function(...)  -- (orientation|ROTATESPRITE_FULL16)
        return format("_con._rotspr(%s,%s,%s,%s,%s,%s,%s,_bor(%s,2048),0,%s,%s,%s,%s)", ...)
    end,

    rotatespritea = function(...)
        return format("_con._rotspr(%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s)", ...)
    end,

    -- <fmt>: format string, number of %s's must match number of varargs
    arraycmd = function(fmt, dstargi, ...)
        local args = {...}
        if (issysgar(args[dstargi])) then
            errprintf("%s: system gamearray not supported", g_lastkw)
        end
        return format(fmt, ...)
    end,

    -- readgamevar or savegamevar
    RSgamevar = function(identifier, dosave)
        -- check identifier for sanity
        if (not identifier:match("^[A-Za-z][A-Za-z0-9_%-]*$")) then
            errprintf("%s: bad identifier `%s' for config file persistence",
                      g_lastkw, identifier)
            return "_BADRSGV()"
        end

        local gv = g_gamevar[identifier]
        if (gv == nil) then
            return lookup.error_not_gamevar(identifier)
        end

        -- For per-actor or per-player gamevars, the value at the current actor or
        -- player index gets saved / loaded.
        local gvkind = bit.band(gv.flags, GVFLAG.PERX_MASK)
        local index = (gvkind==GVFLAG.PERACTOR) and "_aci" or
            (gvkind==GVFLAG.PERPLAYER) and "_pli" or nil

        -- NOTE: more strict than C-CON: we require the gamevar being writable
        -- even if we're saving it.
        local code = lookup.gamevar(identifier, index, true)

        gv.used = bit.bor(gv.used, not dosave and 2 or 1)

        if (dosave) then
            return format("_con._savegamevar(%q,%s)", identifier, code)
        else
            return format("%s=_con._readgamevar(%q,%s)", code, identifier, code)
        end
    end,

    state = function(statename)
        if (g_funcname[statename]==nil) then
            local warn = not g_cgopt["error-nostate"]
            local xprintf = warn and warnprintf or errprintf

            xprintf("state `%s' not found.", statename)
            return warn and "" or "_NULLSTATE()"
        end
        return format("%s(_aci,_pli,_dist)", g_funcname[statename])
    end,

    addweapon = format("if (%s) then _con.longjmp() end", PLS":addweapon(%1,%2)"),

    -- Sound commands
    sound = "_con._sound(_aci,%1)",
    globalsound = "_con._globalsound(_pli,%1)",
    stopsound = "_con._stopsound(_aci,%1)",
    soundonce = "_con._soundonce(_aci,%1)",
}

local userdef_common_pat = (arraypat_maybe_empty + sp1)/{}
    * lpeg.Cc(0) * lpeg.Ct(patt.singlemember) * sp1

-- NOTE about prefixes: most is handled by all_alt_pattern(), however commands
-- that have no arguments and that are prefixes of other commands MUST be
-- suffixed with a "* #sp1" pattern.

local Cinner = {
    -- these can appear anywhere in the script
    ["break"] = cmd()
        / function()
              return g_isWhile[#g_isWhile]
                  and format("goto l%d", #g_isWhile)
                  or "do return end"
          end,
    ["return"] = cmd()  -- NLCF
        / "_con.longjmp()",

    state = cmd(I)
        / handle.state,

    --- 1. get*, set*
    getsector = GetStructCmd(Access.sector),
    getwall = GetStructCmd(Access.wall),
    getactor = GetStructCmd(Access.xsprite),
    getplayer = GetStructCmd(Access.player),

    getinput = GetStructCmd(Access.input),
    getprojectile = GetStructCmd(Access.projectile),
    getthisprojectile = GetStructCmd(Access.thisprojectile),
    gettspr = GetStructCmd(Access.tspr),
    -- NOTE: {get,set}userdef is the only struct that can be accessed without
    -- an "array part", e.g.  H266MOD has "setuserdef .weaponswitch 0" (space
    -- between keyword and "." is mandatory).
    -- NOTE2: userdef has at least three members with a second parameter:
    -- user_name, ridecule, savegame. Then there's wchoice. Given that they're
    -- arrays, I highly doubt that they worked (much less were safe) in CON.
    -- We disallow them, recent EDuke32 versions didn't expose them either.
    getuserdef = GetStructCmd(Access.userdef, userdef_common_pat * tok.wvar),

    getplayervar = GetOrSetPerxvarCmd(false, false),  -- THISACTOR
    getactorvar = GetOrSetPerxvarCmd(false, true),

    setsector = SetStructCmd(Access.sector),
    setwall = SetStructCmd(Access.wall),
    setactor = SetStructCmd(Access.xsprite),
    setplayer = SetStructCmd(Access.player),

    setinput = SetStructCmd(Access.input),
    setprojectile = SetStructCmd(Access.projectile),
    setthisprojectile = SetStructCmd(Access.thisprojectile),
    settspr = SetStructCmd(Access.tspr),
    setuserdef = SetStructCmd(Access.userdef, userdef_common_pat * tok.rvar),

    setplayervar = GetOrSetPerxvarCmd(true, false),  -- THISACTOR
    setactorvar = GetOrSetPerxvarCmd(true, true),

    setvarvar = Op.varvar / "%1=%2",
    addvarvar = Op.varvarf "+",
    -- NOTE the space after the minus sign so that e.g. "subvar x -1" won't get
    -- translated to "x=x--1" (-- being the Lua line comment start).
    subvarvar = Op.varvarf "- ",
    mulvarvar = Op.varvarf("*", "_mulTR", "_mulWR"),
    divvarvar = Op.varvarf "_div",
    modvarvar = Op.varvarf "_mod",
    andvarvar = Op.varvarf "_band",
    orvarvar = Op.varvarf "_bor",
    xorvarvar = Op.varvarf "_bxor",
    randvarvar = Op.varvar / "%1=_con._rand(%2)",

    setvar = Op.var / "%1=%2",
    addvar = Op.varf "+",
    subvar = Op.varf "- ",
    mulvar = Op.varf("*", "_mulTR", "_mulWR"),
    divvar = Op.varf "_div",
    modvar = Op.varf "_mod",
    andvar = Op.varf "_band",
    orvar = Op.varf "_bor",
    xorvar = Op.varf "_bxor",
    randvar = Op.var / "%1=_con._rand(%2)",
    shiftvarl = Op.varf "_lsh",
    shiftvarr = Op.varf "_arsh",

    --- 2. Math operations
    sqrt = cmd(R,W)
        / "%2=_gv.ksqrt(%1)",
    calchypotenuse = cmd(W,R,R)
        / "%1=_con._hypot(%2,%3)",
    sin = cmd(W,R)
        / "%1=_xmath.ksin(%2)",
    cos = cmd(W,R)
        / "%1=_xmath.kcos(%2)",
    mulscale = cmd(W,R,R,R)
        / "%1=_gv.Mulscale(%2,%3,%4)",
    getangle = cmd(W,R,R)
        / "%1=_gv.getangle(%2,%3)",
    getincangle = cmd(W,R,R)
        / "%1=_con._angdiff(%2,%3)",

    --- 3. Actors
    action = cmd(AC)
        / ACS":set_action(%1)",
    ai = cmd(AI)
        / ACS":set_ai(%1)",
    move = sp1 * tok.move * (sp1 * tok.define)^0
        / handle.move,

    cactor = cmd(D)
        / SPS":set_picnum(%1)",
    count = cmd(D)
        / ACS":set_count(%1)",
    cstator = cmd(D)
        / (SPS".cstat=_bor(%1,"..SPS".cstat)"),
    cstat = cmd(D)
        / SPS".cstat=%1",
    clipdist = cmd(D)
        / SPS".clipdist=%1",
    shadeto = cmd(D) /  -- PRE13
        "",
    sizeto = cmd(D,D)
        / "_con._sizeto(_aci,%1,%2)",  -- TODO: see control.lua:_sizeto
    sizeat = cmd(D,D)
        / (SPS".xrepeat,"..SPS".yrepeat=%1,%2"),
    strength = cmd(D)
        / SPS".extra=%1",
    addstrength = cmd(D)
        / (SPS".extra="..SPS".extra+%1"),
    spritepal = cmd(D)
        / SPS".pal=%1",

    hitradius = cmd(D,D,D,D,D)
        / "_con._A_RadiusDamage(_aci,%1,%2,%3,%4,%5)",
    hitradiusvar = cmd(R,R,R,R,R)
        / "_con._A_RadiusDamage(_aci,%1,%2,%3,%4,%5)",

    -- some commands taking read vars
    operaterespawns = cmd(R)
        / "_con._G_OperateRespawns(%1)",
    operatemasterswitches = cmd(R)
        / "_con._G_OperateMasterSwitches(%1)",
    checkactivatormotion = cmd(R)
        / "_gv.RETURN=_con._checkactivatormotion(%1)",
    time = cmd(R)  -- no-op
        / "",
    inittimer = cmd(R)
        / "_con._inittimer(%1)",
    lockplayer = cmd(R)
        / PLS".transporter_hold=%1",
    quake = cmd(R)
        / "_gv.doQuake(%1,81)",  -- TODO: dynsound (EARTHQUAKE)
    jump = cmd(R)
        / handle.NYI,  -- will never be
    cmenu = cmd(R)
        / "_gv._changeMenu(%1)",
    checkavailweapon = cmd(R)  -- THISACTOR
        / function(pli)
              return format("_con._checkavailweapon(%s)", thisactor_to_pli(pli))
          end,
    checkavailinven = cmd(R)  -- THISACTOR
        / function(pli)
              return format("_con._selectnextinv(player[%s])", thisactor_to_pli(pli))
          end,
    guniqhudid = cmd(R)
        / "_gv._set_guniqhudid(%1)",
    echo = cmd(R)
        / "_con._echo(%1)",
    activatecheat = cmd(R)
        / handle.NYI,
    setgamepalette = cmd(R)
        / "_con._setgamepalette(_pli,%1)",

    -- Sound commands
    sound = cmd(D)
        / handle.sound,
    soundvar = cmd(R)
        / handle.sound,
    globalsound = cmd(D)
        / handle.globalsound,
    globalsoundvar = cmd(R)
        / handle.globalsound,
    stopsound = cmd(D)
        / handle.stopsound,
    stopsoundvar = cmd(R)
        / handle.stopsound,
    soundonce = cmd(D)
        / handle.soundonce,
    soundoncevar = cmd(R)
        / handle.soundonce,
    stopactorsound = cmd(R,R)
        / "_con._stopactorsound(%1,%2)",
    stopallsounds = cmd()
        / "_con._stopallsounds()",
    screensound = cmd(R)
        / "_con._screensound(%1)",
    mikesnd = cmd()
        / format("_con._soundonce(_aci,%s)", SPS".yvel"),
    setactorsoundpitch = cmd(R,R,R)
        / "_con._setactorsoundpitch(%1,%2,%3)",

    -- some commands taking defines
    addammo = cmd(D,D)  -- NLCF
        / format("if (%s) then _con.longjmp() end", PLS":addammo(%1,%2)"),
    addweapon = cmd(D,D)  -- NLCF
        / handle.addweapon,
    debris = cmd(D,D)
        / "_con._debris(_aci, %1, %2)",
    addinventory = cmd(D,D)
        / format("_con._addinventory(%s,%%1,%%2,_aci)", PLS""),
    guts = cmd(D,D)
        / "_con._A_DoGuts(_aci,%1,%2)",

    spawn = cmd(D)
        / "_con.spawn(%1,_aci)",
    espawn = cmd(D)
        / "_gv.RETURN=_con.spawn(%1,_aci)",
    espawnvar = cmd(R)
        / "_gv.RETURN=_con.spawn(%1,_aci)",
    qspawn = cmd(D)
        / "_con.spawn(%1,_aci,true)",
    qspawnvar = cmd(R)
        / "_con.spawn(%1,_aci,true)",
    eqspawn = cmd(D)
        / "_gv.RETURN=_con.spawn(%1,_aci,true)",
    eqspawnvar = cmd(R)
        / "_gv.RETURN=_con.spawn(%1,_aci,true)",

    angoff = cmd(D)
        / "spriteext[_aci].angoff=%1",
    angoffvar = cmd(R)
        / "spriteext[_aci].angoff=%1",

    -- cont'd
    addkills = cmd(D)
        / (PLS".actors_killed="..PLS".actors_killed+%1;"..ACS".stayputsect=-1"),
    addphealth = cmd(D)
        / format("_con._addphealth(%s,_aci,%%1)", PLS""),
    debug = cmd(D)
        / handle.debug,
    endofgame = cmd(D)
        / "_con._endofgame(_pli,%1)",
    endoflevel = cmd(D)  -- PRE13
        / "_con._endofgame(_pli,%1)",
    lotsofglass = cmd(D)
        / "_con._A_SpawnGlass(_aci,%1)",
    mail = cmd(D)
        / "_con._spawnmany(_aci,'MAIL',%1)",
    money = cmd(D)
        / "_con._spawnmany(_aci,'MONEY',%1)",
    paper = cmd(D)
        / "_con._spawnmany(_aci,'PAPER',%1)",
    sleeptime = cmd(D)
        / ACS".timetosleep=%1",

    eshoot = cmd(D)
        / "_gv.RETURN=_con.shoot(%1,_aci)",
    eshootvar = cmd(R)
        / "_gv.RETURN=_con.shoot(%1,_aci)",
    ezshoot = cmd(R,D)
        / "_gv.RETURN=_con.shoot(%2,_aci,%1)",
    ezshootvar = cmd(R,R)
        / "_gv.RETURN=_con.shoot(%2,_aci,%1)",
    shoot = cmd(D)
        / "_con.shoot(%1,_aci)",
    shootvar = cmd(R)
        / "_con.shoot(%1,_aci)",
    zshoot = cmd(R,D)
        / "_con.shoot(%2,_aci,%1)",
    zshootvar = cmd(R,R)
        / "_con.shoot(%2,_aci,%1)",

    fall = cmd()
        / "actor.fall(_aci)",
    flash = cmd()
        / format("_con._flash(%s,%s)", SPS"", PLS""),
    getlastpal = cmd()
        / "_con._getlastpal(_aci)",
    insertspriteq = cmd()
        / "_con._addtodelqueue(_aci)",
    killit = cmd()  -- NLCF
        / "_con.killit()",
    nullop = cmd()
        / "",  -- NOTE: really generate no code
    pkick = cmd()
        / format("_con._pkick(%s,%s)", PLS"", ACS""),
    pstomp = cmd()
        / PLS":stomp(_aci)",
    resetactioncount = cmd()
        / ACS":reset_acount()",
    resetcount = cmd()
        / ACS":set_count(0)",
    resetplayer = cmd() * #sp1  -- NLCF
        / "if (_con._VM_ResetPlayer2(_pli,0)) then _con.longjmp() end",
    resetplayerflags = cmd(R)  -- NLCF
        / "if (_con._VM_ResetPlayer2(_pli,%1)) then _con.longjmp() end",
    respawnhitag = cmd()
        / format("_con._respawnhitag(%s)", SPS""),
    tip = cmd()
        / PLS".tipincs=26",
    tossweapon = cmd()
        / "_con._tossweapon(_pli)",
    wackplayer = cmd()
        / PLS":whack()",

    -- player/sprite searching
    findplayer = cmd(W)
        / "_gv.RETURN,%1=_con._findplayer(_pli,_aci)",  -- player index, distance
    findotherplayer = cmd(W)
        / "_gv.RETURN,%1=0,0x7fffffff",  -- TODO_MP
    findnearspritezvar = cmd(D,R,R,W)
        / "%4=_con._findnear(_aci,true,'z',%1,%2,%3)",
    findnearspritez = cmd(D,D,D,W)
        / "%4=_con._findnear(_aci,true,'z',%1,%2,%3)",
    findnearsprite3dvar = cmd(D,R,W)
        / "%3=_con._findnear(_aci,true,'d3',%1,%2)",
    findnearsprite3d = cmd(D,D,W)
        / "%3=_con._findnear(_aci,true,'d3',%1,%2)",
    findnearspritevar = cmd(D,R,W)
        / "%3=_con._findnear(_aci,true,'d2',%1,%2)",
    findnearsprite = cmd(D,D,W)
        / "%3=_con._findnear(_aci,true,'d2',%1,%2)",
    findnearactorzvar = cmd(D,R,R,W)
        / "%4=_con._findnear(_aci,false,'z',%1,%2,%3)",
    findnearactorz = cmd(D,D,D,W)
        / "%4=_con._findnear(_aci,false,'z',%1,%2,%3)",
    findnearactor3dvar = cmd(D,R,W)
        / "%3=_con._findnear(_aci,false,'d3',%1,%2)",
    findnearactor3d = cmd(D,D,W)
        / "%3=_con._findnear(_aci,false,'d3',%1,%2)",
    findnearactorvar = cmd(D,R,W)
        / "%3=_con._findnear(_aci,false,'d2',%1,%2)",
    findnearactor = cmd(D,D,W)
        / "%3=_con._findnear(_aci,false,'d2',%1,%2)",

    -- quotes
    qsprintf = sp1 * tok.rvar * sp1 * tok.rvar * (sp1 * tok.rvar)^-32
        / handle.qsprintf,
    qgetsysstr = cmd(R,R)
        / "_con._qgetsysstr(%1,%2,_pli)",
    qstrcat = cmd(R,R)
        / "_con._qstrcat(%1,%2)",
    qstrcpy = cmd(R,R)
        / "_con._qstrcpy(%1,%2)",
    qstrlen = cmd(W,R)
        / "%1=_con._qstrlen(%2)",
    qstrncat = cmd(R,R,R)
        / "_con._qstrcat(%1,%2,%3)",
    qsubstr = cmd(R,R,R,R)
        / "_con._qsubstr(%1,%2,%3,%4)",
    quote = cmd(D)
        / "_con._quote(_pli,%1)",
    userquote = cmd(R)
        / "_con._userquote(%1)",
    getkeyname = cmd(R,R,R)
        / "_con._getkeyname(%1,%2,%3)",
    getpname = cmd(R,R)  -- THISACTOR
        / function(qnum, pli)
              return format("_con._getpname(%s,%s)", qnum, thisactor_to_pli(pli))
          end,

    -- array stuff
    copy = sp1 * tok.gamearray * arraypat * sp1 * tok.gamearray * arraypat * sp1 * tok.rvar
        / function(...) return handle.arraycmd("_con._gar_copy(%s,%s,%s,%s,%s)", 3, ...) end,
    setarray = sp1 * tok.gamearray * arraypat * sp1 * tok.rvar
        / function(...) return handle.arraycmd("%s[%s]=%s", 1, ...) end,
    resizearray = cmd(GARI,R)
        / function(...) return handle.arraycmd("%s:resize(%s)", 1, ...) end,
    getarraysize = cmd(GARI,W)
        / function(ar, dst)
              return format("%s=%s", dst, issysgar(ar) and tostring(C.MAXTILES) or ar.."._size")
          end,
    readarrayfromfile = cmd(GARI,D)
        / function(...)  -- false: error on no file, nil: don't.
              return handle.arraycmd("%s:read(%s,nil)", 1, ...)
          end,
    writearraytofile = cmd(GARI,D)
        / function(...)
              return handle.arraycmd("%s:write(%s)", 1, ...)
          end,

    -- Persistence
    clearmapstate = cmd(R)
        / "_con._clearmapstate(%1)",
    loadmapstate = cmd()
        / "_con._loadmapstate()",
    savemapstate = cmd()
        / "_con._savemapstate()",
    savegamevar = cmd(I)
        / function(id) return handle.RSgamevar(id, true) end,
    readgamevar = cmd(I)
        / function(id) return handle.RSgamevar(id, false) end,
    savenn = cmd(D)
        / handle.dynNYI,
    save = cmd(D)
        / handle.dynNYI,

    addlogvar = cmd(R)
        / handle.addlogvar,
    addlog = cmd() * #sp1
        / handle.addlog,
    addweaponvar = cmd(R,R)  -- NLCF
        / handle.addweapon,
    cansee = cmd(R,R,R,R,R,R,R,R,W)
        / "%9=cansee(_IV(1,%1,%2,%3),%4, _IV(2,%5,%6,%7),%8) and 1 or 0",
    canseespr = cmd(R,R,W)
        / "%3=_con._canseespr(%1,%2)",
    changespritesect = cmd(R,R)
        / "sprite.changesect(%1,%2,true)",
    changespritestat = cmd(R,R)
        / "sprite.changestat(%1,%2,true)",
    displayrand = cmd(W)
        / "%1=_con._displayrand(32767)",
    displayrandvar = cmd(W,D)
        / "%1=_con._displayrand(%2)",
    displayrandvarvar = cmd(W,R)
        / "%1=_con._displayrand(%2)",
    dist = cmd(W,R,R)
        / "%1=_xmath.dist(sprite[%2],sprite[%3])",
    ldist = cmd(W,R,R)
        / "%1=_xmath.ldist(sprite[%2],sprite[%3])",
    dragpoint = cmd(R,R,R)
        / "wall.dragto(%1,_IV(1,%2,%3,0))",
    rotatepoint = cmd(R,R,R,R,R,W,W)
        / "%6,%7=_con._rotatepoint(%1,%2,%3,%4,%5)",

    -- collision detection etc.
    hitscan = cmd(R,R,R,R,R,R,R,W,W,W,W,W,W,R)  -- 7R 6W 1R
        / handle.hitscan,
    clipmove = cmd(W,W,W,R,W,R,R,R,R,R,R)  -- 3W 1R 1W 6R
        / function(...) return handle.clipmove(0, ...) end,
    clipmovenoslide = cmd(W,W,W,R,W,R,R,R,R,R,R)  -- 3W 1R 1W 6R
        / function(...) return handle.clipmove(1, ...) end,
    lineintersect = cmd(R,R,R,R,R,R,R,R,R,R,W,W,W,W)  -- 10R 4W
        / handle.NYI,
    rayintersect = cmd(R,R,R,R,R,R,R,R,R,R,W,W,W,W)  -- 10R 4W
        / handle.NYI,
    movesprite = cmd(R,R,R,R,R,W)
        / "%6=_con._movesprite(%1,%2,%3,%4,%5)",
    neartag = cmd(R,R,R,R,R,W,W,W,W,R,R)  -- 5R 4W 2R
        / handle.neartag,
    getzrange = cmd(R,R,R,R,W,W,W,W,R,R)
        / handle.getzrange,

    -- screen text and numbers display
    qstrdim = cmd(W,W,R,R,R,R,R,R,R,R,R,R,R,R,R,R,R,R)  -- 2W 16R
        / function(...)
              return format("%s,%s=_con._qstrdim("..n_s_fmt(16)..")", ...)
          end,
    screentext = cmd(R,R,R,R,R,R,R,R,R,R,R,R,R,R,R,R,R,R,R,R)  -- 20 R
        / function(...)
              return format("_con._screentext("..n_s_fmt(20)..")", ...)
          end,
    gametext = cmd(R,R,R,R,R,R,R,R,R,R,R)  -- 11 R
        / function(...)
              return format("_con._gametext("..n_s_fmt(11)..",65536)", ...)
          end,
    gametextz = cmd(R,R,R,R,R,R,R,R,R,R,R,R)  -- 12 R
        / function(...)
              return format("_con._gametext("..n_s_fmt(12)..")", ...)
          end,
    digitalnumber = cmd(R,R,R,R,R,R,R,R,R,R,R)  -- 11R
        / function(...)
              return format("_con._digitalnumber("..n_s_fmt(11)..",65536)", ...)
          end,
    digitalnumberz = cmd(R,R,R,R,R,R,R,R,R,R,R,R)  -- 12R
        / function(...)
              return format("_con._digitalnumber("..n_s_fmt(12)..")", ...)
          end,
    minitext = cmd(R,R,R,R,R)
        / "_con._minitext(%1,%2,%3,%4,%5)",

    palfrom = (sp1 * tok.define)^-4
        / handle.palfrom,

    activatebysector = cmd(R,R)
        / "_con._activatebysector(%1,%2)",
    activate = cmd(D)  -- PRE13, THISACTOR already translated to cur. player
        / "_con._operateactivators(%d,_pli)",
    operateactivators = cmd(R,R)  -- THISACTOR
        / function(tag, pli)
              return format("_con._operateactivators(%s,%s)", tag, thisactor_to_pli(pli))
          end,
    operatesectors = cmd(R,R)
        / "_con._operatesectors(%1,%2)",
    operate = cmd() * #sp1
        / "_con._operate(_aci)",

    myos = cmd(R,R,R,R,R)
        / "_con._myos(%1,%2,65536,%3,%4,%5)",
    myosx = cmd(R,R,R,R,R)
        / "_con._myos(%1,%2,32768,%3,%4,%5)",
    myospal = cmd(R,R,R,R,R,R)
        / "_con._myos(%1,%2,65536,%3,%4,%5,%6)",
    myospalx = cmd(R,R,R,R,R,R)
        / "_con._myos(%1,%2,32768,%3,%4,%5,%6)",

    headspritesect = cmd(W,R)
        / "%1=sprite._headspritesect[%2]",
    headspritestat = cmd(W,R)
        / "%1=sprite._headspritestat[%2]",
    nextspritesect = cmd(W,R)
        / "%1=sprite._nextspritesect[%2]",
    nextspritestat = cmd(W,R)
        / "%1=sprite._nextspritestat[%2]",
    prevspritesect = cmd(W,R)
        / "%1=sprite._prevspritesect[%2]",
    prevspritestat = cmd(W,R)
        / "%1=sprite._prevspritestat[%2]",

    -- NOTE: Yup, it's also an inner command. Do this one concession to
    -- cleanness for backward compatibility (e.g. Sonic3D v0.3).
    definequote = sp1 * tok.define * newline_term_string
        / Cmd.definequote,

    redefinequote = sp1 * tok.define * newline_term_string
        / function(qnum, qstr) return format("_con._definequote(%d,%q)", qnum, stripws(qstr)) end,
    rotatesprite = cmd(R,R,R,R,R,R,R,R,R,R,R,R)  -- 12R
        / handle.rotatesprite,
    rotatesprite16 = cmd(R,R,R,R,R,R,R,R,R,R,R,R)  -- 12R
        / handle.rotatesprite16,
    rotatespritea = cmd(R,R,R,R,R,R,R,R,R,R,R,R,R)  -- 13R
        / handle.rotatespritea,
    sectorofwall = cmd(W,R,R)
        / handle.NYI,
    sectclearinterpolation = cmd(R)
        / "_con._togglesectinterp(%1,0)",
    sectsetinterpolation = cmd(R)
        / "_con._togglesectinterp(%1,1)",

    sectgethitag = cmd()
        / (CSV".HITAG=sector["..SPS".sectnum].hitag"),
    sectgetlotag = cmd()
        / (CSV".LOTAG=sector["..SPS".sectnum].lotag"),
    spgethitag = cmd()
        / (CSV".HITAG="..SPS".hitag"),
    spgetlotag = cmd()
        / (CSV".LOTAG="..SPS".lotag"),
    gettextureceiling = cmd()
        / (CSV".TEXTURE=sector["..SPS".sectnum].ceilingpicnum"),
    gettexturefloor = cmd()
        / (CSV".TEXTURE=sector["..SPS".sectnum].floorpicnum"),

    startlevel = cmd(R,R)
        / "_con._startlevel(%1,%2)",
    starttrack = cmd(D)
        / "_con._starttrack(%1)",
    starttrackvar = cmd(R)
        / "_con._starttrack(%1)",
    startcutscene = cmd(R)
        / handle.NYI,

    getmusicposition = cmd(W)
        / "%1=_con._getmusicposition()",
    setmusicposition = cmd(R)
        / "_con._setmusicposition(%1)",

    setaspect = cmd(R,R)
        / "_con._setaspect(%1,%2)",
    showview = cmd(R,R,R,R,R,R,R,R,R,R)  -- 10R
        / function(...) return format("_con._showview("..n_s_fmt(10)..",0)", ...) end,
    showviewunbiased = cmd(R,R,R,R,R,R,R,R,R,R)  -- 10R
        / function(...) return format("_con._showview("..n_s_fmt(10)..",1)", ...) end,
    smaxammo = cmd(R,R)
        / PLS".max_ammo_amount[%1]=%2",
    gmaxammo = cmd(R,W)
        / ("%2="..PLS".max_ammo_amount[%1]"),
    spriteflags = cmd(R)  -- also see outer
        / ACS".flags=%1",
    ssp = cmd(R,R)
        / "_ssp(%1,%2)",
    setsprite = cmd(R,R,R,R)
        / "_setsprite(%1,_IV(1,%2,%3,%4))",
    updatesector = cmd(R,R,W)
        / format("%%3=updatesector(_IV(1,%%1,%%2,0),%s)", SPS".sectnum"),
    updatesectorz = cmd(R,R,R,W)
        / format("%%4=updatesectorz(_IV(1,%%1,%%2,%%3),%s)", SPS".sectnum"),

    getactorangle = cmd(W)
        / ("%1="..SPS".ang"),
    setactorangle = cmd(R)
        / SPS".ang=_band(%1,2047)",
    getplayerangle = cmd(W)
        / ("%1="..PLS".ang"),
    setplayerangle = cmd(R)
        / PLS".ang=_band(%1,2047)",
    getangletotarget = cmd(W)
        / "%1=_con._angtotarget(_aci)",

    getceilzofslope = cmd(R,R,R,W)
        / "%4=sector[%1]:ceilingzat(_IV(1,%2,%3,0))",
    getflorzofslope = cmd(R,R,R,W)
        / "%4=sector[%1]:floorzat(_IV(1,%2,%3,0))",
    getcurraddress = cmd(W)
        / handle.NYI,  -- will never be
    getticks = cmd(W)
        / "%1=_gv.getticks()",
    gettimedate = cmd(W,W,W,W,W,W,W,W)
        / "%1,%2,%3,%4,%5,%6,%7,%8=_con._gettimedate()",
}

local Cif = {
    ifai = cmd(AI)
        / ACS":has_ai(%1)",
    ifaction = cmd(AC)
        / ACS":has_action(%1)",
    ifmove = cmd(MV)
        / ACS":has_move(%1)",

    ifrnd = cmd(D)
        / "_con.rnd(%1)",
    ifpdistl = cmd(D)  -- DEFER
        / function(val) return { "_dist<"..val, nil, "_con._sleepcheck(_aci,_dist)" } end,
    ifpdistg = cmd(D)  -- DEFER
        / function(val) return { "_dist>"..val, nil, "_con._sleepcheck(_aci,_dist)" } end,
    ifactioncount = cmd(D)
        / ACS":get_acount()>=%1",
    ifcount = cmd(D)
        / ACS":get_count()>=%1",
    ifactor = cmd(D)
        / SPS".picnum==%1",
    ifstrength = cmd(D)
        / SPS".extra<=%1",
    ifspawnedby = cmd(D)
        / ACS".picnum==%1",
    ifwasweapon = cmd(D)
        / ACS".picnum==%1",
    ifgapzl = cmd(D)  -- factor into a con.* function?
        / format("_arsh(%s-%s,8)<%%1", ACS".floorz", ACS".ceilingz"),
    iffloordistl = cmd(D)
        / format("(%s-%s)<=256*%%1", ACS".floorz", SPS".z"),
    ifceilingdistl = cmd(D)
        / format("(%s-%s)<=256*%%1", SPS".z", ACS".ceilingz"),
    ifphealthl = cmd(D)
        / format("sprite[%s].extra<%%1", PLS".i"),
    ifplayersl = cmd(D)  -- PRE13
        / "1<%1",  -- TODO_MP
    ifspritepal = cmd(D)
        / SPS".pal==%1",
    ifgotweaponce = cmd(D)
        / "false",  -- TODO_MP
    ifangdiffl = cmd(D)
        / format("_con._angdiffabs(%s,%s)<=%%1", PLS".ang", SPS".ang"),
    ifsound = cmd(D)
        / "_con._soundplaying(_aci,%1)",
    ifpinventory = cmd(D,D)
        / format("_con._checkpinventory(%s,%%1,%%2,_aci)", PLS""),

    ifvarl = cmd(R,D)
        / "%1<%2",
    ifvarg = cmd(R,D)
        / "%1>%2",
    ifvare = cmd(R,D)
        / "%1==%2",
    ifvarn = cmd(R,D)
        / "%1~=%2",
    ifvarand = cmd(R,D)
        / "_band(%1,%2)~=0",
    ifvaror = cmd(R,D)
        / "_bor(%1,%2)~=0",
    ifvarxor = cmd(R,D)
        / "_bxor(%1,%2)~=0",
    ifvareither = cmd(R,D)
        / "%1~=0 or %2~=0",

    ifvarvarl = cmd(R,R)
        / "%1<%2",
    ifvarvarg = cmd(R,R)
        / "%1>%2",
    ifvarvare = cmd(R,R)
        / "%1==%2",
    ifvarvarn = cmd(R,R)
        / "%1~=%2",
    ifvarvarand = cmd(R,R)
        / "_band(%1,%2)~=0",
    ifvarvaror = cmd(R,R)
        / "_bor(%1,%2)~=0",
    ifvarvarxor = cmd(R,R)
        / "_bxor(%1,%2)~=0",
    ifvarvareither = cmd(R,R)
        / "%1~=0 or %2~=0",

    ifactorsound = cmd(R,R)
        / "_con._soundplaying(%1,%2)",
    ifcutscene = cmd(R)
        / function(cs)
            handle.NYI()
            return "false"
          end,

    ifp = (sp1 * tok.define)^1
        / function(...) return format("_con._ifp(%d,_pli,_aci)", bit.bor(...)) end,
    ifsquished = cmd()
        / "_con._squished(_aci,_pli)",
    ifserver = cmd()
        / "false",  -- TODO_MP
    ifrespawn = cmd()
        / format("_con._checkrespawn(%s)", SPS""),
    ifoutside = cmd()
        / format("_band(sector[%s].ceilingstat,1)~=0", SPS".sectnum"),
    ifonwater = cmd()
        / format("sector[%s].lotag==1 and _math.abs(%s-sector[%s].floorz)<32*256",
                 SPS".sectnum", SPS".z", SPS".sectnum"),
    ifnotmoving = cmd()
        / "_band(actor[_aci]._movflag,49152)>16384",
    ifnosounds = cmd()
        / "not _con._ianysound(_aci)",
    ifmultiplayer = cmd()
        / "false",  -- TODO_MP
    ifinwater = cmd()
        / format("sector[%s].lotag==2", SPS".sectnum"),
    ifinspace = cmd()
        / format("_con._checkspace(%s,false)", SPS".sectnum"),
    ifinouterspace = cmd()
        / format("_con._checkspace(%s,true)", SPS".sectnum"),
    ifhitweapon = cmd()
        / "_con._A_IncurDamage(_aci)>=0",
    ifhitspace = cmd()
        / "player.holdskey(_pli,'OPEN')",
    ifdead = cmd()
        / SPS".extra<=0",
    ifclient = cmd()
        / "false",  -- TODO_MP
    ifcanshoottarget = cmd()
        / "_con._canshoottarget(_dist,_aci)",
    ifcanseetarget = cmd()  -- DEFER -- XXX: 1536 is SLEEPTIME
        / function()
              return { format("_con._canseetarget(%s,%s)", SPS"", PLS""), ACS".timetosleep=1536" }
          end,
    ifcansee = cmd() * #sp1
        / format("_con._cansee(_aci,%s)", PLS""),
    ifbulletnear = cmd()
        / "_con._bulletnear(_aci)",
    ifawayfromwall = cmd()
        / format("_con._awayfromwall(%s,108)", SPS""),
    ifactornotstayput = cmd()
        / ACS".stayputsect==-1",
}


----==== Tracing and reporting ====----

-- g_newlineidxs will contain the 1-based file offsets to "\n" characters
local g_newlineidxs = {}

-- Returns index into the sorted table tab such that
--   tab[index] <= searchelt < tab[index+1].
-- Preconditions:
--  tab[i] < tab[i+1] for 0 <= i < #tab
--  tab[0] <= searchelt < tab[#tab]
-- If #tab is less than 2, returns 0. This plays nicely with newline index
-- tables like { [0]=0, [1]=len+1 }, e.g. if the file doesn't contain any.
local function bsearch(tab, searchelt)
--    printf("bsearch(tab, %d)", searchelt)
    local l, r = 0, #tab
    local i

    if (r < 2) then
        return 0
    end

    while (l ~= r) do
        i = l + math.ceil((r-l)/2)  -- l < i <= r
        assert(l < i and i <= r)
        local elt = tab[i]
--        printf("l=%d tab[%d]=%d r=%d", l, i, elt, r)

        if (searchelt == elt) then
            return i
        end

        if (searchelt < elt) then
            r = i-1
        else  -- (searchelt > elt)
            l = i
        end
    end

--    printf("return tab[%d]=%d", l, tab[l])
    return l
end

function getlinecol(pos)  -- local
    assert(type(pos)=="number")
    local line = bsearch(g_newlineidxs, pos)
    assert(line and g_newlineidxs[line]<=pos and pos<g_newlineidxs[line+1])
    local col = pos-g_newlineidxs[line]
    if (col == 0) then
        -- XXX: we probably have an off-by-one error somewhere and it would
        -- need to be fixed instead of doing these ugly workarounds.
        line = line-1
        col = pos-g_newlineidxs[line]
    end
    assert(col >= 1)
    return line+1, col-1
end

-- A generic trace function, prints a position together with the match content.
-- The 'doit' parameter can be used to temporarily enable/disable a particular
-- tracing function.
local function TraceFunc(pat, label, doit)
    assert(doit ~= nil)
    pat = Pat(pat)

    if (doit) then
        local function tfunc(subj, pos, a)
            printf("%s:%s: %s", linecolstr(pos), label, a)
            return true
        end
        pat = lpeg.Cmt(pat, tfunc)
    elseif (label=="kw") then  -- HACK
        local function tfunc(subj, pos, a)
            g_lastkwpos = pos
            g_lastkw = a
            return true
        end
        -- XXX: is there a better way?
        pat = lpeg.Cmt(pat, tfunc)
    end

    return pat
end

local function BadIdent(pat)
    local function tfunc(subj, pos, a)
        if (g_warn["bad-identifier"] and not g_badids[a]) then
            warnprintf("bad identifier: %s", a)
            g_badids[a] = true
        end
        return true
    end
    return lpeg.Cmt(Pat(pat), tfunc)
end

-- These are tracers for specific patterns which can be disabled
-- if desired.
local function Keyw(kwname) return TraceFunc(kwname, "kw", false) end
--local function NotKeyw(text) return TraceFunc(text, "!kw", false) end
--local function Ident(idname) return TraceFunc(idname, "id", false) end
local function Stmt(cmdpat) return TraceFunc(cmdpat, "st", false) end

--local function Temp(kwname) return TraceFunc(kwname, "temp", true) end
--Cinner["myosx"] = Temp(Cinner["myosx"])

----==== Translator continued ====----
local function attachlinenum(capts, pos)
    capts[1] = capts[1].."--"..getlinecol(pos)
    return capts[1]
end

local function after_inner_cmd_Cmt(subj, pos, ...)
    if (g_numerrors == inf) then
        return nil
    end

    local capts = {...}
    assert(type(capts[1])=="string" and capts[2]==nil)
    return true, attachlinenum(capts, pos)
end

local function after_if_cmd_Cmt(subj, pos, ...)
    if (g_numerrors == inf) then
        return nil
    end

    local capts = {...}
    assert(capts[1] ~= nil)
    assert(#capts <= 3)

    for i=#capts,1, -1 do
        assert(type(capts[i])=="string" or type(capts[i])=="table")
    end

    -- IF_LINE_NUMBERING
    local firstistab = (type(capts[1])=="table")
    attachlinenum(firstistab and capts[1] or capts, pos)

    return true, unpack(capts)
end

local function after_cmd_Cmt(subj, pos, ...)
    if (g_numerrors == inf) then
--        print("Aborting parsing...")
        return nil  -- make the match fail, bail out of parsing
    end

    return true  -- don't return any captures
end

-- Attach the command names at the front!
local function attachnames(kwtab, matchtimefunc)
    for cmdname,cmdpat in pairs(kwtab) do
        -- The match-time function capture at the end is so that every command
        -- acts as a barrier to captures to delay (but not fully prevent) stack
        -- overflow (and to make lpeg.match return a subject position at the
        -- end)
        local newpat = Keyw(cmdname) * cmdpat
        if (cmdname~="break") then
            kwtab[cmdname] = lpeg.Cmt(newpat, matchtimefunc)
        else
            -- Must not attack a Cmt to "break" because it would break the
            -- while/switch sequencing.
            kwtab[cmdname] = newpat
        end
    end
end

attachnames(Couter, after_cmd_Cmt)
attachnames(Cinner, after_inner_cmd_Cmt)
attachnames(Cif, after_if_cmd_Cmt)


-- Takes one or more tables and +'s all its patterns together in reverse
-- lexicographical order.
-- Each such PATTAB must be a table that maps command names to their patterns.
local function all_alt_pattern(...)
    local cmds = {}

    local args = {...}
    assert(#args <= 2)

    for argi=1,#args do
        local pattab = args[argi]

        -- pairs() iterates in undefined order, so we first fill in the names...
        for cmdname,_ in pairs(pattab) do
            cmds[#cmds+1] = cmdname
        end
    end

    -- ...and then sort them in ascending lexicographical order
    table.sort(cmds)

    local pat = Pat(false)

    for i=1,#cmds do
        local ourpat = args[1][cmds[i]] or args[2][cmds[i]]
        -- shorter commands go at the end!
        pat = pat + ourpat
    end

    return pat
end

-- actor ORGANTIC is greeting!
function on.lonely_else(pos)
    pwarnprintf(pos, "found `else' with no `if'")
end

local con_inner_command = all_alt_pattern(Cinner)
local con_if_begs = all_alt_pattern(Cif)

local lone_else = (POS() * "else" * sp1)/on.lonely_else

local stmt_list = Var("stmt_list")
-- possibly empty statement list:
local stmt_list_or_eps = lpeg.Ct((stmt_list * sp1)^-1)
local stmt_list_nosp_or_eps = lpeg.Ct((stmt_list * (sp1 * stmt_list)^0)^-1)

-- Reused LPeg patterns
local common = {}

-- common to actor and useractor: <name/tilenum> [<strength> [<action> [<move> [<flags>... ]]]]
common.actor_end = sp1 * lpeg.Ct(tok.actordefine *
    (sp1 * tok.define *
     (sp1 * tok.action *
      (sp1 * tok.move *
       (sp1 * tok.define)^0
      )^-1
     )^-1
    )^-1)
* sp1 * stmt_list_or_eps * "enda"

common.block_begin = lpeg.Cc(nil) / function()
    g_switchCode = {}
end

common.block_end = lpeg.Cc(nil) / function()
    if (#g_switchCode > 0) then
        addcode(g_switchCode)
    end
    g_switchCode = nil
end

--== block delimiters (no syntactic recursion) ==--
local Cblock = {
    -- actor (...)
    actor = POS() * lpeg.Cc(nil) * common.actor_end / on.actor_end,
    -- useractor <actortype> (...)
    useractor = POS() * sp1 * tok.define * common.actor_end / on.actor_end,
    -- eventloadactor <name/tilenum>
    eventloadactor = POS() * sp1 * tok.actordefine * sp1 * stmt_list_or_eps * "enda"
        / on.eventloadactor_end,

    onevent = POS() * sp1 * tok.define * sp1 * stmt_list_or_eps * "endevent"
        / on.event_end,
    appendevent = POS() * sp1 * tok.define * sp1 * stmt_list_or_eps * "endevent"
        / on.appendevent_end,

    state = POS() * sp1 * (lpeg.Cmt(tok.identifier, on.state_begin_Cmt))
                  * sp1 * stmt_list_or_eps * tok.state_ends
        / on.state_end,
    defstate = POS() * sp1 * (lpeg.Cmt(tok.identifier, on.state_begin_Cmt))
                     * sp1 * stmt_list_or_eps * tok.state_ends
        / on.state_end,
}

for cmdname, cmdpat in pairs(Cblock) do
    Cblock[cmdname] = common.block_begin * cmdpat * common.block_end
end

attachnames(Cblock, after_cmd_Cmt)


local t_good_identifier = Range("AZ", "az", "__") * Range("AZ", "az", "__", "09")^0

-- CON isaltok also has chars in "{}.", but these could potentially
-- interfere with *CON* syntax.  The "]" is so that the number in e.g. array[80]
-- isn't considered a broken identifier.
-- "-" is somewhat problematic, but we allow it only as 2nd and up character, so
-- there's no ambiguity with unary minus.  (Commands must be separated by spaces
-- in CON, so a trailing "-" is "OK", too.)
-- This is broken in itself, so we ought to make a compatibility/modern CON switch.
local t_broken_identifier = BadIdent(-((tok.number + t_good_identifier) * (sp1 + Set("[]:"))) *
                                     (alphanum + Set(BAD_ID_CHARS0)) * (alphanum + Set(BAD_ID_CHARS1))^0)

---
local function do_flatten_codetab(code, intotab)
    for i=1,math.huge do
        local elt = code[i]

        if (type(elt)=="string") then
            intotab[#intotab+1] = elt
        elseif (type(elt)=="table") then
            do_flatten_codetab(elt, intotab)
        else
            assert(elt==nil)
            return
        end
    end
end

-- Return a "string buffer" table that can be table.concat'ed
-- to get the code string.
local function flatten_codetab(codetab)
    local tmpcode = {}
    do_flatten_codetab(codetab, tmpcode)
    return tmpcode
end

function on.if_else_end(ifconds, ifstmt, elsestmt, ...)
    assert(#{...}==0)
    assert(type(ifconds)=="table" and #ifconds>=1)

    -- A condition may be a table carrying "deferred" code to add either
    --  [1] after the 'if' or
    --  [2] after the whole if/else block.
    -- In CON, it's always the same code for the same kind of "deferedness",
    -- and it's always idempotent (executing it multiple times has the same
    -- effect as executing it once), so generate code for it only once, too.
    local deferred = { nil, nil }
    local linenum = ""

    local ifcondstr = {}
    for i=1,#ifconds do
        local cond = ifconds[i]
        local hasmore = type(cond=="table")

        ifcondstr[i] = hasmore and cond[1] or cond
        assert(type(ifcondstr[i])=="string")

        -- IF_LINE_NUMBERING
        local tlinum = assert(ifcondstr[i]:match("^.*(%-%-[0-9]+)$"))
        ifcondstr[i] = assert(ifcondstr[i]:match("^(.*)%-%-[0-9]+$"))
        if (linenum == "") then
            linenum = tlinum
        end

        if (hasmore) then
            for i=1,2 do
                if (deferred[i]==nil) then
                    deferred[i] = cond[i+1]
                end
            end
        end
    end

    -- Construct a string of ANDed conditions
    local conds = "(" .. table.concat(ifcondstr, ")and(") .. ")"

    local code = {
        format("if %s then%s", conds, linenum),
        assert(ifstmt),
    }

    code[#code+1] = deferred[1]

    if (elsestmt~=nil) then
        local elseifp = false

        if (type(elsestmt)=="table") then
            elsestmt = flatten_codetab(elsestmt)

            if (#elsestmt>=2 and elsestmt[1]:match("^if ") and elsestmt[#elsestmt]=="end") then
                elsestmt[1] = elsestmt[1]:sub(4)
                elsestmt[#elsestmt] = nil
                elseifp = true
            end
        end

        code[#code+1] = elseifp and "elseif" or "else"
        code[#code+1] = elsestmt
    end

    code[#code+1] = "end"
    code[#code+1] = deferred[2]

    return code
end

function on.while_begin(v1, v2)
    table.insert(g_isWhile, true)
    return format("while (%s~=%s) do", v1, v2)
end

function on.while_end()
    local whilenum = #g_isWhile
    table.remove(g_isWhile)
    return format("::l%d:: end", whilenum)
end

function on.switch_begin()
    table.insert(g_isWhile, false)
end

function on.switch_end(testvar, blocks)
    local SW = format("_SW[%d]", g_switchCount)
    local swcode = { format("%s={", SW) }
    local have = {}
    local havedefault = false

    table.remove(g_isWhile)

    for i=1,#blocks do
        local block = blocks[i]
        assert(#block >= 1)
        local isdefault = (#block==1)
        local index = isdefault and "'default'" or tostring(block[1])

        if (have[index]) then
            if (isdefault) then
                errprintf("duplicate 'default' block in switch statement")
                return "_INVALIDSW()"
            else
                warnprintf("duplicate case %s in switch statement", index)
            end
        end
        have[index] = true

        swcode[#swcode+1] = format("[%s]=function(_aci,_pli,_dist)", index)
        swcode[#swcode+1] = get_cache_sap_code()
        -- insert the case/default code:
        swcode[#swcode+1] = block[#block]
        swcode[#swcode+1] = "end,"
    end

    swcode[#swcode+1] = "}"

    -- insert additional case test numbers (e.g. case 0: >>> case 1 <<<: <code...>)
    for i=1,#blocks do
        local block = blocks[i]
        for j=2,#block-1 do
            local index = tostring(block[j])
            swcode[#swcode+1] = format("%s[%d]=%s[%d]", SW, index, SW, tostring(block[1]))
        end
    end

    assert(g_switchCode ~= nil)
    g_switchCode[#g_switchCode+1] = swcode

    -- The code for the switch statement itself:
    local code = format("_con._switch(_SW[%d], %s, _aci,_pli,_dist)", g_switchCount, testvar)
    g_switchCount = g_switchCount+1
    return code
end

function on.case_colon(pos)
    pwarnprintf(pos, "encountered deprecated ':' after 'case'")
end


--- The final grammar!
local Grammar = Pat{
    -- The starting symbol.
    -- A translation unit is a (possibly empty) sequence of outer CON
    -- commands, separated by at least one whitespace which may be
    -- omitted at the EOF.
    sp0 * (all_alt_pattern(Couter, Cblock) * sp1)^0,

    -- Some often-used terminals follow.  These appear here because we're
    -- hitting a limit with LPeg else.
    -- http://lua-users.org/lists/lua-l/2008-11/msg00462.html

    -- NOTE: NW demo (NWSNOW.CON) contains a Ctrl-Z char (decimal 26)
    whitespace = Set(" \t\r\26") + newline + Set("(),;") + comment + linecomment,

    t_number = POS() * lpeg.C(
        tok.maybe_minus * ((Pat("0x") + "0X") * Range("09", "af", "AF")^1 * Pat("h")^-1
                           + Range("09")^1)
                             ) / parse_number,

    -- TODO: negated gamevars. Currently, "-var" is parsed as a negated label.

    t_identifier_all = lpeg.C(t_broken_identifier + t_good_identifier),
    -- NOTE: -conl.keyword alone would be wrong, e.g. "state breakobject":
    -- NOTE 2: The + "[" is so that stuff like
    --   getactor[THISACTOR].x x
    --   getactor[THISACTOR].y y
    -- is parsed correctly.  (Compared with this:)
    --   getactor[THISACTOR].x x
    --   getactor [THISACTOR].y y
    -- This is in need of cleanup!
    t_identifier = -(conl.keyword * (sp1 + "[")) * tok.identifier_all,
    -- TODO?: SST TC has e.g. "1267AT", relying on it to be parsed as a number "1267".
    -- However, this conflicts with bad-identifiers, so it should be checked last.
    -- This would also handle LNGA2's "00000000h", though would give problems with
    -- e.g. "800h" (hex 0x800 or decimal 800?).
    t_define = (POS() * lpeg.C(tok.maybe_minus) * tok.identifier / lookup.defined_label) + tok.number,
    -- A defined label token, but returning the label if one was passed
    -- (specially shoehorned into a string):
    t_rawdefine = (POS() * lpeg.C(tok.maybe_minus) * tok.identifier / lookup.raw_defined_label) + tok.number,

    -- Defines and constants can take the place of vars that are only read.
    -- XXX: now, when tok.rvar fails, the tok.define failure message is printed.
    t_rvar = Var("t_botharrayexp") + lpeg.Cmt(tok.identifier, maybe_gamevar_Cmt) + tok.define,
    -- For written-to vars, only (non-parm2) array exprs and writable gamevars
    -- are permitted.  NOTE: C-CON doesn't support inline array exprs here.
    t_wvar = Var("t_singlearrayexp") / function() errprintf("t_wvar: array exprs NYI") return "_NYIVAR" end
        + (tok.identifier / function(id) return lookup.gamevar(id, "_aci", true) end),

    t_gamearray = Var("t_identifier") / lookup.gamearray,

    t_move =
        POS()*tok.identifier / function(...) return lookup.composite(LABEL.MOVE, ...) end +
        POS()*tok.number / function(...) return check.composite_literal(LABEL.MOVE, ...) end,

    t_ai =
        POS()*tok.identifier / function(...) return lookup.composite(LABEL.AI, ...) end +
        POS()*tok.number / function(...) return check.composite_literal(LABEL.AI, ...) end,

    t_action =
        POS()*tok.identifier / function(...) return lookup.composite(LABEL.ACTION, ...) end +
        POS()*tok.number / function(...) return check.composite_literal(LABEL.ACTION, ...) end,

    -- New-style inline arrays and structures.
    t_botharrayexp = tok.identifier * arraypat * patt.bothmember^-1
        / function(...) return lookup.array_expr(false, ...) end,
    t_singlearrayexp = tok.identifier * arraypat * patt.singlemember^-1,

    -- SWITCH
    switch_stmt = Keyw("switch") * sp1 * tok.rvar * (lpeg.Cc(nil)/on.switch_begin) *
        lpeg.Ct((Var("case_block") + Var("default_block"))^0) * sp1 * "endswitch"
        / on.switch_end,

    -- NOTE: some old DNWMD has "case: PIGCOP".  I don't think I'll allow that.
    case_block = lpeg.Ct((sp1 * Keyw("case") * (POS()*Pat(":") / on.case_colon)^-1
                         * sp1 * tok.define * (sp0*":")^-1)^1 * sp1 *
                         stmt_list_nosp_or_eps), -- * "break",

    default_block = lpeg.Ct(sp1 * Keyw("default") * (sp0*":"*sp0 + sp1) *
                            stmt_list_nosp_or_eps),  -- * "break",

    if_stmt = lpeg.Ct((con_if_begs * sp1)^1) * Var("single_stmt")
        * (sp1 * Keyw("else") * sp1 * Var("single_stmt"))^-1 / on.if_else_end,

    while_stmt = Keyw("whilevarvarn") * sp1 * tok.rvar * sp1 * tok.rvar / on.while_begin
          * sp1 * Var("single_stmt") * (lpeg.Cc(nil) / on.while_end)
        + Keyw("whilevarn") * sp1 * tok.rvar * sp1 * tok.define / on.while_begin
          * sp1 * Var("single_stmt") * (lpeg.Cc(nil) / on.while_end),

    stmt_common = Keyw("{") * sp1 * "}" / ""  -- space separation of commands in CON is for a reason!
        -- XXX: this do...end can lead to exceeding Lua nesting limits, see nightstrike's tan.con
        + lpeg.Ct(Keyw("{")/"do" * sp1 * stmt_list * sp1 * (Keyw("}")/"end"))
        + con_inner_command + Var("switch_stmt") + lpeg.Ct(Var("while_stmt")),

    single_stmt = Stmt( lone_else^-1 * (Var("stmt_common") + Var("if_stmt")) ),

    -- a non-empty statement/command list
    stmt_list = Var("single_stmt") * (sp1 * Var("single_stmt"))^0,
}


local function setup_newlineidxs(contents)
    local newlineidxs = {}
    for i in string.gmatch(contents, "()\n") do
        newlineidxs[#newlineidxs+1] = i
    end
    if (#newlineidxs == 0) then
        -- try CR only (old Mac)
        for i in string.gmatch(contents, "()\r") do
            newlineidxs[#newlineidxs+1] = i
        end
--        if (#newlineidxs > 0) then print('CR-only lineends detected.') end
    end
    -- dummy newlines at beginning and end
    newlineidxs[#newlineidxs+1] = #contents+1
    newlineidxs[0] = 0

    return newlineidxs
end


--== Lua -> CON line number mapping for error messages ==--

local lineinfo_mt = {
    __index = {
        -- Get CON file name and CON line number from Lua line number.
        getfline = function(self, lualine)
            local llines, lfiles = self.llines, self.lfiles
            assert(lualine >= 1 and lualine <= #llines)

            -- Get the CON line number: a simple lookup.
            local conline = llines[lualine]

            -- Find the CON file name next.
            local confile = nil
            for i=1,#lfiles do
                if (lfiles[i].line > lualine) then
                    break
                end
                -- Shorten the file name by stripping the directory parts.
                confile = lfiles[i].name:match("[^/]+$")
            end

            return confile or "???", conline
        end,
    },

    __metatable = true,
}

-- Handle a line of translated CON->Lua code. Return its CON line number.
local function lineinfo_handle_line(i, code, curline, curfile, lfiles)
    local lnumstr = code:match("%-%-([0-9]+)$")
    local begfn = lnumstr and nil or code:match("^%-%- BEGIN (.+)$")
    local endfn = lnumstr and nil or code:match("^%-%- END (.+)$")

    if (lnumstr) then
        curline[#curline] = assert(tonumber(lnumstr))
    elseif (begfn) then
        curfile[#curfile+1] = begfn
        curline[#curline+1] = 1
        -- Begin an included file.
        lfiles[#lfiles+1] = { line=i, name=begfn }
    elseif (endfn) then
        assert(endfn==curfile[#curfile])  -- assert proper nesting
        curfile[#curfile] = nil
        curline[#curline] = nil
        -- End an included file, so reset the name to the includer's one.
        lfiles[#lfiles+1] = { line=i, name=curfile[#curfile] }
    end

    return assert(curline[#curline])
end

-- Construct Lua->CON line mapping info.  This walks the generated code and
-- looks for our inserted comment strings, so it's kind of hackish.
function get_lineinfo(flatcode)
    local curline, curfile = { 0 }, { "<none>" }  -- stacks
    -- llines: [<Lua code line number>] = <CON code line number>
    -- lfiles: [<sequence number>] = { line=<Lua line number>, name=<filename> }
    local llines, lfiles = {}, {}

    if (type(flatcode)=="table") then
        for i=1,#flatcode do
            llines[i] = lineinfo_handle_line(i, flatcode[i], curline, curfile, lfiles)
        end
    else
        -- Already concat'ed code given.
        assert(type(flatcode)=="string")
        local olinestart = 1

        for i=1,math.huge do
            local curnli = flatcode:find("\n", olinestart, true)
            local line

            if (curnli ~= nil) then
                line = flatcode:sub(olinestart, curnli-1)
                olinestart = curnli+1
            else
                -- Last line
                line = flatcode:sub(olinestart, -1)
                break
            end

            llines[i] = lineinfo_handle_line(i, line, curline, curfile, lfiles)
        end
    end

    return setmetatable({ llines=llines, lfiles=lfiles }, lineinfo_mt)
end

-- <lineinfop>: Get line info?
local function get_code_string(codetab, lineinfop)
    -- Create meta-info gamevar: which gamevars have bit NORESET set?
    codetab[#codetab+1] = "_V._IS_NORESET_GAMEVAR={"
    for identifier, gv in pairs(g_gamevar) do
        if (bit.band(gv.flags, GVFLAG.NORESET) ~= 0) then
            codetab[#codetab+1] = format("[%q]=true,", identifier)
        end
    end
    codetab[#codetab+1] = "}"

    -- Return defined labels in a table...
    codetab[#codetab+1] = "return {"
    for label, val in pairs(g_labeldef) do
        -- ... skipping 'NO' and those that are gamevars in C-CON.
        if (g_labeltype[label]==LABEL.NUMBER and not g_labelspecial[label]) then
            codetab[#codetab+1] = format("[%q]=%d,", label, val)
        end
    end
    codetab[#codetab+1] = "},_C,_M,_I"  -- CONCODE_RETURN

    local flatcode = flatten_codetab(codetab)
    local lineinfo = lineinfop and get_lineinfo(flatcode)
    return table.concat(flatcode, "\n"), lineinfo
end

function on.parse_begin()
    g_isWhile = {}
    g_have_file[g_filename] = true

    -- set up new state
    -- TODO: pack into one "parser state" table?
    g_lastkw, g_lastkwpos, g_numerrors = nil, nil, 0
    g_recurslevel = g_recurslevel+1
end


---=== EXPORTED FUNCTIONS ===---

function parse(contents)  -- local
    -- save outer state
    local lastkw, lastkwpos, numerrors = g_lastkw, g_lastkwpos, g_numerrors
    local newlineidxs = g_newlineidxs

    on.parse_begin()

    g_newlineidxs = setup_newlineidxs(contents)

    addcodef("-- BEGIN %s", g_filename)

    local idx = lpeg.match(Grammar, contents)

    if (not idx) then
        printf("[%d] Match failed.", g_recurslevel)
        g_numerrors = inf
    elseif (idx == #contents+1) then
        if (g_numerrors ~= 0) then
            printf("[%d] Matched whole contents (%d errors).",
                   g_recurslevel, g_numerrors)
        elseif (g_recurslevel==0) then
            printf("[0] Matched whole contents.")
        end
    else
        local i, col = getlinecol(idx)
        local bi, ei = g_newlineidxs[i-1]+1, g_newlineidxs[i]-1

        printf("[%d] Match succeeded up to line %d, col %d (pos=%d, len=%d)",
               g_recurslevel, i, col, idx, #contents)
        g_numerrors = inf

--        printf("Line goes from %d to %d", bi, ei)
        local suffix = ""
        if (ei-bi > 76) then
            ei = bi+76
            suffix = " (...)"
        end
        printf("%s%s", string.sub(contents, bi, ei), suffix)

        if (g_lastkwpos) then
            i, col = getlinecol(g_lastkwpos)
            printf("Last keyword was at line %d, col %d: %s", i, col, g_lastkw)
        end
    end

    -- Check read/written status of all user gamevars.
    if (idx == #contents+1 and g_recurslevel==0) then
        local gvs = {}
        for identifier, gv in pairs(g_gamevar) do
            if (gv.used ~= 3) then
                -- NOTE: read but not written to gamevar (gv.used == 1) has its
                -- use in C-CON
                if (gv.used == 0 and g_warn["never-used-gamevar"] or
                        gv.used == 2 and g_warn["never-read-gamevar"]) then
                    gv.id = identifier
                    gvs[#gvs+1] = gv
                end
            end
        end

        local function compare_gv(gva, gvb)
            if (gva.loc[1] ~= gvb.loc[1]) then
                return gva.loc[1] < gvb.loc[1]
            end
            return (gva.loc[2] < gvb.loc[2])
        end

        table.sort(gvs, compare_gv)

        for i=1,#gvs do
            local gv = gvs[i]

            local loc = gv.loc
            local locstr = loc and format("%s %d:%d: ", loc[1], loc[2], loc[3]) or ""

            local perActor = (bit.band(gv.flags, GVFLAG.PERACTOR) ~= 0)
            local perPlayer = (bit.band(gv.flags, GVFLAG.PERPLAYER) ~= 0)
            local kindstr = perActor and "per-actor " or (perPlayer and "per-player " or "")

            if (gv.used == 0) then
                printf("%sWarning: never used %sgamevar `%s'", locstr,
                       kindstr, gv.id)
            else
                printf("%sWarning: never %s %sgamevar `%s'", locstr,
                       gv.used == 1 and "written to" or "read",
                       kindstr, gv.id)
            end
        end
    end

    addcodef("-- END %s", g_filename)
    g_recurslevel = g_recurslevel-1

    -- restore outer state
    g_lastkw, g_lastkwpos = lastkw, lastkwpos
    g_numerrors = math.max(g_numerrors, numerrors)
    g_newlineidxs = newlineidxs
end

function reset.all()
    reset.labels()
    reset.gamedata()
    reset.codegen()
end

local function print_on_failure(msg)
    if (g_lastkwpos ~= nil) then
        printf("LAST KEYWORD POSITION: %s, %s", linecolstr(g_lastkwpos), g_lastkw)
    end
    print(msg)
end

if (string.dump) then
    -- running stand-alone
    local io = require("io")

    local function compile(filename)
        reset.all()

        -- Construct file name for the output code: (...)/xxx/qwe.con -->
        -- xxx_qwe.con, so that common root CON file names like EDUKE.CON will
        -- result in distinct file names for different mods. From that name,
        -- strip the extension.
        local codedir = g_cgopt["gendir"]
        local codefn = codedir and
            codedir.."/"..filename:match("[^/]+/[^/]+$"):gsub('/','_')..".lua"

        -- Get the directory part...
        g_directory = filename:match(".*/") or ""
        -- ...and the file name alone.
        filename = filename:sub(#g_directory+1, -1)

        -- NOTE: xpcall isn't useful here since the traceback won't give us
        -- anything inner to the lpeg.match call
        local ok, msg = pcall(do_include_file, g_directory, filename, true)
        -- ^v Swap commenting (comment top, uncomment bottom line) to get backtraces
--        local ok, msg = true, do_include_file(g_directory, filename, true)

        if (not ok) then
            print_on_failure(msg)
        end

        if (not (g_cgopt["no"]==true)) then
            local onlycheck = (g_cgopt["no"] == "onlycheck")
            -- The file for the output messages:
            local msgfile = onlycheck and io.stdout or io.stderr

            local code, lineinfo = get_code_string(g_curcode, g_cgopt["debug-lineinfo"])
            local func, errmsg = loadstring(code, "CON")

--            msgfile:write(format("-- GENERATED CODE for \"%s\":\n", filename))
            if (func == nil) then
                msgfile:write(format("-- %s%s: INVALID Lua CODE: %s\n",
                                     g_directory, filename, errmsg))
            end

            if (g_cgopt["names"]) then
                for i=0,C.MAXTILES-1 do
                    if (g_actorTileToName[i]) then
                        msgfile:write(format("#define %s %d\n", g_actorTileToName[i], i))
                    end
                end
            elseif (lineinfo) then
                for i=1,#lineinfo.llines do
                    msgfile:write(format("%d -> %s:%d\n", i, lineinfo:getfline(i)))
                end
            elseif (not onlycheck) then
                -- The file for the generated code:
                local codefile = codefn and assert(io.open(codefn, "w+")) or msgfile

                codefile:write(code)
                codefile:write("\n")
            end
        end
    end

    local havelists = false

    for argi=1,#arg do
        local filename = arg[argi]

        if (filename=="@") then
            -- Start file list processing from the next positional argument on.
            havelists = true
        elseif (havelists) then
            printf("\n------ Handling list of CON files \"%s\"", filename)
            for fn in io.lines(filename) do
                -- A hash at the beginning of a line denotes a comment, an
                -- empty line is skipped.
                if (#fn>0 and not fn:match("^#")) then
                    compile(fn)
                end
            end
        else
            compile(filename)
        end
    end
else
    -- running from EDuke32

    function compile(filenames)
        -- TODO: pathsearchmode=1 set in G_CompileScripts

        reset.all()

        for _, fname in ipairs(filenames) do
            local ok, msg = pcall(do_include_file, "", fname, true)
            if (not ok or g_numerrors > 0) then
                if (not ok) then
                    -- Unexpected error in the Lua code (i.e. a bug here).
                    print_on_failure(msg)
                end
                return nil
            end
        end

        return get_code_string(g_curcode, true)
    end
end
