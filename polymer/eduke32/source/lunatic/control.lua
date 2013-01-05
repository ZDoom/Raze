-- Game control module for Lunatic.

local ffi = require("ffi")
local ffiC = ffi.C

local bit = require("bit")
local math = require("math")
local geom = require("geom")
local con_lang = require("con_lang")

local setmetatable = setmetatable

local assert = assert
local error = error
local print = print
local type = type
local unpack = unpack

local actor, player = assert(actor), assert(player)
local dc = require("defs_common")
local cansee, hitscan, neartag = dc.cansee, dc.hitscan, dc.neartag
local inside = dc.inside

local sector, wall, sprite = dc.sector, dc.wall, dc.sprite
local spritesofsect, spritesofstat = dc.spritesofsect, dc.spritesofstat


module(...)


local lastid = { action=0, move=0, ai=0 }
local def = {
    action = { NO=ffi.new("con_action_t") },
    move = { NO=ffi.new("con_move_t") },
    ai = { NO=ffi.new("con_ai_t") },
}

local function forbidden() error("newindex forbidden", 2) end

AC = setmetatable({}, { __index=def.action, __newindex=forbidden })
MV = setmetatable({}, { __index=def.move, __newindex=forbidden })
AI = setmetatable({}, { __index=def.ai, __newindex=forbidden })

local function check_name(name, what, errlev)
    if (type(name)~="string" or #name > 63) then
        error("bad argument #1 to "..what..": must be a string of length <= 63", errlev+1)
    end
end

local function action_or_move(what, numargs, tab, name, ...)
    if (lastid[what] <= -(2^31)) then
        error("Too many "..what.."s defined", 3);
    end
    check_name(name, what, 3)

    local args = {...}
    if (#args > numargs) then
        error("Too many arguments passed to "..what, 3)
    end

    for i=1,#args do
        local n = args[i]
        if (type(n)~="number" or not (n >= -32768 and n <= 32767)) then
            error("bad argument #".. i+1 .." to "..what..
                  ": must be numbers in [-32768..32767]", 3)
        end
    end
    -- missing fields are initialized to 0 by ffi.new

    -- Named actions or moves have negative ids so that non-negative ones
    -- can be used as (different) placeholders for all-zero ones.
    lastid[what] = lastid[what]-1

    -- ffi.new takes either for initialization: varargs, a table with numeric
    -- indices, or a table with key-value pairs
    -- See http://luajit.org/ext_ffi_semantics.html#init_table
    tab[name] = ffi.new("const con_"..what.."_t", lastid[what], args)
end

---=== ACTION / MOVE / AI ===---

function action(name, ...)
    action_or_move("action", 5, def.action, name, ...)
end

function move(name, ...)
    action_or_move("move", 2, def.move, name, ...)
end


local function get_action_or_move(what, val, argi)
    if (val == nil) then
        return {}  -- will init the struct to all zeros
    elseif (type(val)=="string") then
        local am = def[what][val]
        if (am==nil) then
            error("no "..what.." '"..val.."' defined", 3)
        end
        return am
    elseif (ffi.istype("con_"..what.."_t", val)) then
        return val
    end

    -- TODO: literal number actions/moves?
    error("bad argument #"..argi.." to ai: must be string or "..what, 3)
end

function ai(name, action, move, flags)
    if (lastid.ai <= -(2^31)) then
        error("Too many AIs defined", 2);
    end
    check_name(name, "ai", 2)

    lastid.ai = lastid.ai-1

    local act = get_action_or_move("action", action, 2)
    local mov = get_action_or_move("move", move, 3)

    if (flags~=nil) then
        if (type(flags)~="number" or not (flags>=0 and flags<=32767)) then
            error("bad argument #4 to ai: must be a number in [0..32767]", 2)
        end
    else
        flags = 0
    end

    def.ai[name] = ffi.new("const con_ai_t", lastid.ai, act, mov, flags)
end


---=== RUNTIME CON FUNCTIONS ===---

-- TODO: also check whether sprite exists in the game world (statnum != MAXSTATUS)
local function check_sprite_idx(i)
    if (i >= ffiC.MAXSPRITES+0ULL) then
        error("invalid argument: must be a valid sprite index", 3)
    end
end

local function check_tile_idx(tilenum)
    if (tilenum >= ffiC.MAXTILES+0ULL) then
        error("invalid argument: must be a valid tile number", 3)
    end
end

local function krandand(mask)
    return bit.band(ffiC.krand(), mask)
end

-- Lunatic's "insertsprite" is a wrapper around the game "A_InsertSprite", not
-- the engine "insertsprite".
--
-- Forms:
--  1. table-call: insertsprite{tilenum, pos, sectnum [, owner [, statnum]] [, key=val...]}
--     valid keys are: owner, statnum, shade, xrepeat, yrepeat, xvel, zvel
--  2. position-call: insertsprite(tilenum, pos, sectnum [, owner [, statnum]])
function insertsprite(tab_or_tilenum, ...)
    local tilenum, pos, sectnum  -- mandatory
    -- optional with defaults:
    local owner, statnum
    local shade, xrepeat, yrepeat, ang, xvel, zvel = 0, 48, 48, 0, 0, 0

    if (type(tab_or_tilenum)=="table") then
        local tab = tab_or_tilenum
        tilenum, pos, sectnum = unpack(tab, 1, 3)
        owner = tab[4] or tab.owner or -1
        statnum = tab[5] or tab.statnum or 0
        shade = tab.shade or shade
        xrepeat = tab.xrepeat or xrepeat
        yrepeat = tab.yrepeat or yrepeat
        ang = tab.ang or ang
        xvel = tab.xvel or xvel
        zvel = tab.zvel or zvel
    else
        tilenum = tab_or_tilenum
        local args = {...}
        pos, sectnum = unpack(args, 1, 2)
        owner = args[3] or -1
        statnum = args[4] or 0
    end

    if (type(sectnum)~="number" or type(tilenum) ~= "number") then
        error("invalid insertsprite call: 'sectnum' and 'tilenum' must be numbers", 2)
    end
    check_tile_idx(tilenum)
    dc.check_sector_idx(sectnum)
    if (statnum >= ffiC.MAXSTATUS) then
        error("invalid 'statnum' argument to insertsprite: must be a status number (0 .. MAXSTATUS-1)", 2)
    end

    return ffiC.A_InsertSprite(sectnum, pos.x, pos.y, pos.z, tilenum,
                              shade, xrepeat, yrepeat, ang, xvel, zvel,
                              owner, statnum)
end

-- INTERNAL USE ONLY.
function _addtodelqueue(spritenum)
    check_sprite_idx(spritenum)
    ffiC.A_AddToDeleteQueue(spritenum)
end

-- This corresponds to the first (spawn from parent sprite) form of A_Spawn().
function spawn(parentspritenum, tilenum, addtodelqueue)
    check_sprite_idx(parentspritenum)
    check_tile_idx(tilenum)

    if (addtodelqueue and ffiC.g_spriteDeleteQueueSize == 0) then
        return -1
    end

    local i = ffiC.A_Spawn(parentspritenum, tilenum)
    if (addtodelqueue) then
        ffiC.A_AddToDeleteQueue(i)
    end
    return i
end

-- This is the second A_Spawn() form. INTERNAL USE ONLY.
function _spawnexisting(spritenum)
    check_sprite_idx(spritenum)
    return ffiC.A_Spawn(-1, spritenum)
end

-- A_SpawnMultiple clone
-- ow: parent sprite number
function _spawnmany(ow, tilenum, n)
    local spr = sprite[ow]

    for i=n,1, -1 do
        local j = insertsprite{ tilenum, spr^(ffiC.krand()%(47*256)), spr.sectnum, ow, 5,
                                shade=-32, xrepeat=8, yrepeat=8, ang=krandand(2047) }
        _spawnexisting(j)
        sprite[j].cstat = krandand(8+4)
    end
end

function isenemytile(tilenum)
    return (bit.band(ffiC.g_tile[tilenum].flags, ffiC.SFLAG_BADGUY)~=0)
end

function rotatesprite(x, y, zoom, ang, tilenum, shade, pal, orientation,
                      cx1, cy1, cx2, cy2)
    if (type(tilenum) ~= "number" or tilenum >= ffiC.MAXTILES+0ULL) then
        error("bad argument #5 to rotatesprite: must be number in [0.."..ffiC.MAXTILES.."]", 2)
    end

    ffiC.rotatesprite(65536*x, 65536*y, zoom, ang, tilenum, shade, pal, bit.bor(2,orientation),
                      cx1, cy1, cx2, cy2)
end

function rnd(x)
    return (bit.rshift(ffiC.krand(), 8) >= (255-x))
end


---=== Weapon stuff ===---


--- Helper functions (might be exported later) ---

local function have_weapon(ps, weap)
    return (bit.band(ps.gotweapon, bit.lshift(1, weap)) ~= 0)
end

local function have_ammo_at_max(ps, weap)
    return (ps:get_ammo_amount(weap) >= ps:get_max_ammo_amount(weap))
end

local function P_AddAmmo(ps, weap, amount)
    if (not have_ammo_at_max(ps, weap)) then
        local curamount = ps:get_ammo_amount(weap)
        local maxamount = ps:get_max_ammo_amount(weap)
        -- NOTE: no clamping towards the bottom
        ps:set_ammo_amount(weap, math.min(curamount+amount, maxamount))
    end
end

local function P_AddWeaponAmmoCommon(ps, weap, amount)
    P_AddAmmo(ps, weap, amount)

    if (ps.curr_weapon==ffiC.KNEE_WEAPON and have_weapon(ps, weap)) then
        ffiC.P_AddWeaponMaybeSwitch(ps, weap);
    end
end


--- Functions that must be exported because they are used by LunaCON generated code,
--- but which are off limits to users.  (That is, we need to think about how to
--- expose the functionality in a better fashion than merely giving access to
--- the C functions.)

local MAXQUOTES = con_lang.MAXQUOTES

local function check_quote_idx(qnum)
    if (qnum >= MAXQUOTES+0ULL) then
        error("invalid quote number "..qnum, 3)
    end

    if (ffiC.ScriptQuotes[qnum] == nil) then
        error("null quote "..qnum, 3)
    end
end

function _definequote(qnum, quotestr)
    check_quote_idx(qnum)
    assert(type(quotestr)=="string")
    ffiC.C_DefineQuote(qnum, quotestr)
    return (#quotestr >= con_lang.MAXQUOTELEN)
end

function _quote(pli, qnum)
    check_quote_idx(qnum)

    local p = player[pli]  -- bound-check
    ffiC.P_DoQuote(qnum+MAXQUOTES, ffiC.g_player[pli].ps)
end

function _echo(qnum)
    check_quote_idx(qnum)
    -- XXX: ugly round-trip
    print(ffi.string(ffiC.ScriptQuotes[qnum]))
end

local D = {
    -- TODO: dynamic tile remapping
    ACTIVATOR = 2,
    RESPAWN = 9,
    APLAYER = 1405,

    FIRSTAID = 53,
    STEROIDS = 55,
    AIRTANK = 56,
    JETPACK = 57,
    HEATSENSOR = 59,
    BOOTS = 61,
    HOLODUKE = 1348,

    STATUE = 753,
    NAKED1 = 603,
    PODFEM1 = 1294,
    FEM1 = 1312,
    FEM2 = 1317,
    FEM3 = 1321,
    FEM5 = 1323,
    FEM4 = 1325,
    FEM6 = 1334,
    FEM8 = 1336,
    FEM7 = 1395,
    FEM9 = 3450,
    FEM10 = 4864,

    ATOMICHEALTH = 100,
    GLASSPIECES = 1031,
    TRANSPORTERSTAR = 1630,
    COMMANDER = 1920,
    JIBS2 = 2250,
    SCRAP1 = 2400,
    BLIMP = 3400,
}

function _A_DoGuts(i, gutstile, n)
    check_tile_idx(gutstile)
    local spr = sprite[i]
    local smallguts = spr.xrepeat < 16 and spr:isenemy()
    local xsz = smallguts and 8 or 32
    local ysz = xsz
    local z = math.min(spr.z, sector[spr.sectnum]:floorzat(spr)) - 8*256

    if (spr.picnum == D.COMMANDER) then
        z = z - (24*256)
    end

    for i=n,1, -1 do
        local pos = geom.vec3(spr.x+krandand(255)-128, spr.y+krandand(255)-128, z-krandand(8191))
        local j = insertsprite{ gutstile, pos, spr.sectnum, i, 5, shade=-32, xrepeat=xsz, yrepeat=ysz,
                                ang=krandand(2047), xvel=48+krandand(31), zvel=-512-krandand(2047) }
        local newspr = sprite[j]
        if (newspr.picnum==D.JIBS2) then
            -- This looks silly, but EVENT_EGS code could have changed the size
            -- between the insertion and here.
            newspr.xrepeat = newspr.xrepeat/4
            newspr.yrepeat = newspr.yrepeat/4
        end
        newspr.pal = spr.pal
    end
end

function _debris(i, dtile, n)
    local spr = sprite[i]
    if (spr.sectnum >= ffiC.numsectors+0ULL) then
        return
    end

    for j=n-1,0, -1 do
        local isblimpscrap = (spr.picnum==D.BLIMP and dtile==D.SCRAP1)
        local picofs = isblimpscrap and 0 or krandand(3)
        local pos = spr + geom.vec3(krandand(255)-128, krandand(255)-128, -(8*256)-krandand(8191))
        local jj = insertsprite{ dtile+picofs, pos, spr.sectnum, i, 5,
                                 shade=spr.shade, xrepeat=32+krandand(15), yrepeat=32+krandand(15),
                                 ang=krandand(2047), xvel=32+krandand(127), zvel=-krandand(2047) }
        -- NOTE: BlimpSpawnSprites[14] (its array size is 15) will never be chosen
        sprite[jj]:set_yvel(isblimpscrap and ffiC.BlimpSpawnSprites[math.mod(jj, 14)] or -1)
        sprite[jj].pal = spr.pal
    end
end

function _A_SpawnGlass(i, n)
    local spr = sprite[i]

    for j=n,1, -1 do
        local k = insertsprite{ D.GLASSPIECES+n%3, spr^(256*krandand(16)), spr.sectnum, i, 5,
                                shade=krandand(15), xrepeat=36, yrepeat=36, ang=krandand(2047),
                                xvel=32+krandand(63), zvel=-512-krandand(2047) }
        sprite[k].pal = spr.pal
    end
end

function _A_Shoot(i, atwith)
    check_sprite_idx(i)
    check_tile_idx(atwith)
    return ffiC.A_Shoot(i, atwith)
end

function _A_IncurDamage(sn)
    check_sprite_idx(sn)
    return ffiC.A_IncurDamage(sn)
end

function _VM_FallSprite(i)
    check_sprite_idx(i)
    ffiC.VM_FallSprite(i)
end

function _sizeto(i, xr, yr)
    local spr = sprite[i]
    local dr = (xr-spr.xrepeat)
    -- NOTE: could "overflow" (e.g. goal repeat is 256, gets converted to 0)
    spr.xrepeat = spr.xrepeat + ((dr == 0) and 0 or (dr < 0 and -1 or 1))
    -- TODO: y stretching is conditional
    dr = (yr-spr.yrepeat)
    spr.yrepeat = spr.yrepeat + ((dr == 0) and 0 or (dr < 0 and -1 or 1))
end

-- NOTE: function args of the C function have overloaded meaning
function _A_Spawn(j, pn)
    local bound_check = sector[sprite[j].sectnum]  -- two in one whack
    check_tile_idx(pn)
    return ffiC.A_Spawn(j, pn)
end

function _pstomp(ps, i)
    if (ps.knee_incs == 0 and sprite[ps.i].xrepeat >= 40) then
        local spr = sprite[i]
        if (cansee(spr^(4*256), spr.sectnum, ps.pos^(-16*256), sprite[ps.i].sectnum)) then
            for j=ffiC.playerswhenstarted-1,0 do
                if (player[j].actorsqu == i) then
                    return
                end
            end
            ps.actorsqu = i
            ps.knee_incs = 1
            if (ps.weapon_pos == 0) then
                ps.weapon_pos = -1
            end
        end
    end
end

function _pkick(ps, spr)
    -- TODO: multiplayer branch
    if (spr.picnum~=D.APLAYER and ps.quick_kick==0) then
        ps.quick_kick = 14
    end
end

function _VM_ResetPlayer2(snum)
    local bound_check = player[snum]
    return (ffiC.VM_ResetPlayer2(snum)~=0)
end

local PALBITS = { [0]=1, [21]=2, [23]=4 }
local ICONS = {
    [ffiC.GET_FIRSTAID] = 1,  -- ICON_FIRSTAID
    [ffiC.GET_STEROIDS] = 2,
    [ffiC.GET_HOLODUKE] = 3,
    [ffiC.GET_JETPACK] = 4,
    [ffiC.GET_HEATS] = 5,
    [ffiC.GET_SCUBA] = 6,
    [ffiC.GET_BOOTS] = 7,
}

-- XXX
function _addinventory(ps, inv, amount, pal)
    if (inv == ffiC.GET_ACCESS) then
        if (PALBITS[pal]) then
            ps.got_access = bit.bor(ps.got_access, PALBITS[pal])
        end
    else
        if (ICONS[inv]) then
            ps.inven_icon = ICONS[inv]
        end

        if (inv == ffiC.GET_SHIELD) then
            amount = math.min(ps.max_shield_amount, amount)
        end
        -- NOTE: this is more permissive than CON, e.g. allows
        -- GET_DUMMY1 too.
        ps:set_inv_amount(inv, amount)
    end
end

-- For GET_ACCESS: returns logical: whether player has card given by PAL
-- Else: returns inventory amount
function _getinventory(ps, inv, i)
    if (inv == ffiC.GET_ACCESS) then
        if (PALBITS[sprite[i].pal]) then
            return (bit.band(ps.got_access, PALBITS[sprite[i].pal])~=0)
        end
        return false
    else
        return ps:get_inv_amount(inv)
    end
end

function _addphealth(ps, spr, hlthadd)
    if (ps.newowner >= 0) then
        ffiC.G_ClearCameraView(ps)
    end

    if (ffiC.ud.god ~= 0) then
        return
    end

    local notatomic = (spr.picnum ~= D.ATOMICHEALTH)
    local j = sprite[ps.i].extra

    if (notatomic and j > ps.max_player_health and hlthadd > 0) then
        return
    end

    if (j > 0) then
        j = j + hlthadd
    end

    if (notatomic) then
        if (hlthadd > 0) then
            j = math.min(j, ps.max_player_health)
        end
    else
        j = math.min(j, 2*ps.max_player_health)
    end

    j = math.max(j, 0)

    if (hlthadd > 0) then
        local qmaxhlth = bit.rshift(ps.max_player_health, 2)
        if (j-hlthadd < qmaxhlth and j >= qmaxhlth) then
            -- TODO
            --A_PlaySound(DUKE_GOTHEALTHATLOW, ps->i)
        end

        ps.last_extra = j
    end

    sprite[ps.i].extra = j
end

-- The return value is true iff the ammo was at the weapon's max.
-- In that case, no action is taken.
function _addammo(ps, weap, amount)
    return have_ammo_at_max(ps, weap) or P_AddWeaponAmmoCommon(ps, weap, amount)
end

function _addweapon(ps, weap, amount)
    if (weap >= ffiC.MAX_WEAPONS+0ULL) then
        error("Invalid weapon ID "..weap, 2)
    end

    if (not have_weapon(ps, weap)) then
        ffiC.P_AddWeaponMaybeSwitch(ps, weap);
    elseif (have_ammo_at_max(ps, weap)) then
        return true
    end

    P_AddWeaponAmmoCommon(ps, weap, amount)
end

function _A_RadiusDamage(i, r, hp1, hp2, hp3, hp4)
    check_sprite_idx(i)
    ffiC.A_RadiusDamage(i, r, hp1, hp2, hp3, hp4)
end

function _testkey(pli, synckey)
    local bound_check = player[pli]
    if (synckey >= 32ULL) then
        error("Invalid argument #2 to _testkey: must be in [0..31]", 2)
    end
    local bits = ffiC.g_player[pli].sync.bits
    return (bit.band(bits, bit.lshift(1,synckey)) ~= 0)
end

function _operate(spritenum)
    local NEAROP = {
        [9] = true,
        [15] = true,
        [16] = true,
        [17] = true,
        [18] = true,
        [19] = true,
        [20] = true,
        [21] = true,
        [22] = true,
        [23] = true,
        [25] = true,
        [26] = true,
        [29] = true,
    }

    local spr = sprite[spritenum]

    if (sector[spr.sectnum].lotag == 0) then
        local tag = neartag(spr^(32*256), spr.sectnum, spr.ang, 768, 4+1)
        if (tag.sector >= 0) then
            local sect = sector[tag.sector]
            local lotag = sect.lotag
            if (NEAROP[bit.band(lotag, 0xff)]) then
                if (lotag==23 or sect.floorz==sect.ceilingz) then
                    if (bit.band(lotag, 32768+16384) == 0) then
                        for j in spritesofsect(tag.sector) do
                            if (sprite[j].picnum==D.ACTIVATOR) then
                                return
                            end
                        end
                        ffiC.G_OperateSectors(tag.sector, spritenum)
                    end
                end
            end
        end
    end
end

function _endofgame(pli, timebeforeexit)
    player[pli].timebeforeexit = timebeforeexit
    player[pli].customexitsound = -1
    ffiC.ud.eog = 1
end

function _bulletnear(i)
    return (ffiC.A_Dodge(sprite[i]) == 1)
end

-- d is a distance
function _awayfromwall(spr, d)
    local vec2 = geom.vec2
    local vecs = { vec2(d,d), vec2(-d,-d), vec2(d,-d), vec2(-d,d) }
    for i=1,4 do
        if (not inside(vecs[i]+spr, spr.sectnum)) then
            return false
        end
    end
    return true
end

local BANG2RAD = math.pi/1024

local function cossinb(bang)
    -- XXX: better use the precalc'd arrays instead?
    local ang = BANG2RAD*bang
    return 16384*math.cos(ang), 16384*math.sin(ang)
end

local function manhatdist(v1, v2)
    return math.abs(v1.x-v2.x) + math.abs(v1.y-v2.y)
end

-- "otherspr" is either player or holoduke sprite
local function A_FurthestVisiblePoint(aci, otherspr)
    if (bit.band(actor[aci]:get_t_data(0), 63) ~= 0) then
        return
    end

    local angincs = (ffiC.ud.player_skill < 3) and 1024 or 2048/(1+krandand(1))

    local j = 0
    repeat
        local c, s = cossinb(otherspr.ang + j)
        local hit = hitscan(otherspr^(16*256), otherspr.sectnum,
                            c, s, 16384-krandand(32767), ffiC.CLIPMASK1)
        local dother = manhatdist(hit.pos, otherspr)
        local dactor = manhatdist(hit.pos, sprite[aci])

        if (dother < dactor and hit.sect >= 0) then
            if (cansee(hit.pos, hit.sect, otherspr^(16*256), otherspr.sectnum)) then
                return hit
            end
        end

        j = j + (angincs - krandand(511))
    until (j >= 2048)
end

local MAXSLEEPDIST = 16384
local SLEEPTIME = 1536

function _cansee(aci, ps)
    -- Select sprite for monster to target.
    local spr = sprite[aci]
    local s = sprite[ps.i]

    if (ps.holoduke_on >= 0) then
        -- If holoduke is on, let them target holoduke first.
        local hs = sprite[ps.holoduke_on]

        if (cansee(spr^krandand(8191), spr.sectnum, s, s.sectnum)) then
            s = hs
        end
    end

    -- Can they see player (or player's holoduke)?
    local can = cansee(spr^krandand(47*256), spr.sectnum, s^(24*256), s.sectnum)

    if (not can) then
        -- Search around for target player.
        local hit = A_FurthestVisiblePoint(aci, s)
        if (hit ~= nil) then
            can = true
            actor[aci].lastvx = hit.pos.x
            actor[aci].lastvy = hit.pos.y
        end
    else
        -- Else, they did see it. Save where we were looking...
        actor[aci].lastvx = s.x
        actor[aci].lastvy = s.y
    end

    if (can and (spr.statnum==ffiC.STAT_ACTOR or spr.statnum==ffiC.STAT_STANDABLE)) then
        actor[aci].timetosleep = SLEEPTIME
    end

    return can
end

function _sleepcheck(aci, dist)
    local acs = actor[aci]
    if (dist > MAXSLEEPDIST and acs.timetosleep == 0) then
        acs.timetosleep = SLEEPTIME
    end
end

function _canseetarget(spr, ps)
    -- NOTE: &41 ?
    return cansee(spr^(256*krandand(41)), spr.sectnum,
                  ps.pos, sprite[ps.i].sectnum)
end

local function A_CheckHitSprite(spr, angadd)
    local zoff = (spr:isenemy() and 42*256) or (spr.picnum==D.APLAYER and 39*256) or 0

    local c, s = cossinb(spr.ang+angadd)
    local hit = hitscan(spr^zoff, spr.sectnum, c, s, 0, ffiC.CLIPMASK1)
    if (hit.wall >= 0 and wall[hit.wall]:ismasked() and spr:isenemy()) then
        return -1, nil
    end

    local dx = hit.pos.x-spr.x
    local dy = hit.pos.y-spr.y
    return hit.sprite, math.sqrt(dx*dx+dy*dy)  -- TODO: use "ldist" approximation for authenticity
end

function _canshoottarget(dist, aci)
    if (dist > 1024) then
        local spr = sprite[aci]

        local hitspr, hitdist = A_CheckHitSprite(spr, 0)
        if (hitdist == nil) then
            return true
        end

        local bigenemy = (spr:isenemy() and spr.xrepeat > 56)

        local sclip = bigenemy and 3084 or 768
        local angdif = bigenemy and 48 or 16

        local sclips = { sclip, sclip, 768 }
        local angdifs = { 0, angdif, -angdif }

        for i=1,3 do
            if (i > 1) then
                hitspr, hitdist = A_CheckHitSprite(spr, angdifs[i])
            end

            if (hitspr >= 0 and sprite[hitspr].picnum == spr.picnum) then
                if (hitdist > sclips[i]) then
                    return false
                end
            end
        end
    end

    return true
end

function _getlastpal(spritenum)
    local spr = sprite[spritenum]
    if (spr.picnum == D.APLAYER) then
        spr.pal = player[spr.yvel].palookup
    else
        if (spr.pal == 1 and spr.extra == 0) then  -- hack for frozen
            spr.extra = spr.extra+1
        end
        spr.pal = actor[spritenum].tempang
    end
    actor[spritenum].tempang = 0
end

-- abs(G_GetAngleDelta(a1, a2))
function _angdiffabs(a1, a2)
    a1 = bit.band(a1, 2047)
    a2 = bit.band(a2, 2047)
    -- a1 and a2 are in [0, 2047]
    if (math.abs(a2-a1) < 1024) then
        return math.abs(a2-a1)
    end
    -- |a2-a1| >= 1024
    if (a2 > 1024) then a2=a2-2048 end
    if (a1 > 1024) then a1=a1-2048 end
    -- a1 and a2 is in [-1023, 1024]
    return math.abs(a2-a1)
end

local SK = {
    CROUCH = 1,
    RUN = 5,
}

function _ifp(flags, pli, aci)
    local l = flags
    local ps = player[pli]
    local vel = sprite[ps.i].xvel
    local band = bit.band

    if (band(l,8)~=0 and ps.on_ground and _testkey(pli, SK.CROUCH)) then
        return true
    elseif (band(l,16)~=0 and ps.jumping_counter == 0 and not ps.on_ground and ps.vel.z > 2048) then
        return true
    elseif (band(l,32)~=0 and ps.jumping_counter > 348) then
        return true
    elseif (band(l,1)~=0 and vel >= 0 and vel < 8) then
        return true
    elseif (band(l,2)~=0 and vel >= 8 and not _testkey(pli, SK.RUN)) then
        return true
    elseif (band(l,4)~=0 and vel >= 8 and _testkey(pli, SK.RUN)) then
        return true
    elseif (band(l,64)~=0 and ps.pos.z < (sprite[aci].z-(48*256))) then
        return true
    elseif (band(l,128)~=0 and vel <= -8 and not _testkey(pli, SK.RUN)) then
        return true
    elseif (band(l,256)~=0 and vel <= -8 and _testkey(pli, SK.RUN)) then
        return true
    elseif (band(l,512)~=0 and (ps.quick_kick > 0 or (ps.curr_weapon == ffiC.KNEE_WEAPON and ps.kickback_pic > 0))) then
        return true
    elseif (band(l,1024)~=0 and sprite[ps.i].xrepeat < 32) then
        return true
    elseif (band(l,2048)~=0 and ps.jetpack_on) then
        return true
    elseif (band(l,4096)~=0 and ps:get_inv_amount(ffiC.GET_STEROIDS) > 0 and ps:get_inv_amount(ffiC.GET_STEROIDS) < 400) then
        return true
    elseif (band(l,8192)~=0 and ps.on_ground) then
        return true
    elseif (band(l,16384)~=0 and sprite[ps.i].xrepeat > 32 and sprite[ps.i].extra > 0 and ps.timebeforeexit == 0) then
        return true
    elseif (band(l,32768)~=0 and sprite[ps.i].extra <= 0) then
        return true
    elseif (band(l,65536)~=0) then
        -- TODO: multiplayer branch
        if (_angdiffabs(ps.ang, ffiC.getangle(sprite[aci].x-ps.pos.x, sprite[aci].y-ps.pos.y)) < 128) then
            return true
        end
    end

    return false
end

function _checkspace(sectnum, floorp)
    local sect = sector[sectnum]
    local picnum = floorp and sect.floorpicnum or sect.ceilingpicnum
    local stat = floorp and sect.floorstat or sect.ceilingstat
    return bit.band(stat,1)~=0 and sect.ceilingpal == 0 and
        (picnum==D.MOONSKY1 or picnum==D.BIGORBIT1)
end

function _flash(spr, ps)
   spr.shade = -127
   ps.visibility = -127  -- XXX
   ffiC.lastvisinc = ffiC.totalclock+32
end

local function G_OperateRespawns(tag)
    for i in spritesofstat(ffiC.STAT_FX) do
        local spr = sprite[i]

        if (spr.lotag==tag and spr.picnum==D.RESPAWN) then
            if (ffiC.ud.monsters_off~=0 and isenemytile(spr.hitag)) then
                return
            end

            local j = spawn(i, D.TRANSPORTERSTAR)
            sprite[j].z = sprite[j].z - (32*256)

            -- Just a way to killit (see G_MoveFX(): RESPAWN__STATIC)
            spr.extra = 66-12
        end
    end
end

local RESPAWN_USE_YVEL =
{
    [D.STATUE] = true,
    [D.NAKED1] = true,
    [D.PODFEM1] = true,
    [D.FEM1] = true,
    [D.FEM2] = true,
    [D.FEM3] = true,
    [D.FEM5] = true,
    [D.FEM4] = true,
    [D.FEM6] = true,
    [D.FEM8] = true,
    [D.FEM7] = true,
    [D.FEM9] = true,
    [D.FEM10] = true,
}

function _respawnhitag(spr)
    if (RESPAWN_USE_YVEL[spr.picnum]) then
        if (spr.yvel ~= 0) then
            G_OperateRespawns(spr.yvel)
        end
    else
        G_OperateRespawns(spr.hitag)
    end
end

local INVENTILE = {
    [D.FIRSTAID] = true,
    [D.STEROIDS] = true,
    [D.AIRTANK] = true,
    [D.JETPACK] = true,
    [D.HEATSENSOR] = true,
    [D.BOOTS] = true,
    [D.HOLODUKE] = true,
}

function _checkrespawn(spr)
    if (spr:isenemy()) then
        return (ffiC.ud.respawn_monsters~=0)
    end
    if (INVENTILE[spr.picnum]) then
        return (ffiC.ud.respawn_inventory~=0)
    end
    return (ffiC.ud.respawn_items~=0)
end

-- SOUNDS

local function check_sound_idx(sndidx)
    if (sndidx >= con_lang.MAXSOUNDS+0ULL) then
        error("invalid sound number "..sndidx, 2)
    end
end

function _ianysound(aci)
    check_sprite_idx(aci)
    return (ffiC.A_CheckAnySoundPlaying(aci)~=0)
end

function _sound(aci, sndidx)
    check_sprite_idx(aci)
    ffiC.A_PlaySound(sndidx, aci)
end

function _globalsound(pli, sndidx)
    -- TODO: conditional on coop, fake multimode
    if (pli==ffiC.screenpeek) then
        _sound(player[pli].i, sndidx)
    end
end

-- This is a macro for EDuke32 (game.h)
local function S_StopSound(sndidx)
    ffiC.S_StopEnvSound(sndidx, -1)
end

function _stopsound(aci, sndidx)
    check_sprite_idx(aci)
    check_sound_idx(sndidx)
    -- XXX: This is weird: the checking is done wrt a sprite, but the sound not.
    -- NOTE: S_StopSound() stops sound <sndidx> that started playing most recently.
    if (ffiC.S_CheckSoundPlaying(aci, sndidx) ~= 0) then
        S_StopSound(sndidx)
    end
end

function _soundonce(aci, sndidx)
    check_sound_idx(sndidx)
    if (ffiC.S_CheckSoundPlaying(aci, sndidx) == 0) then
        _sound(aci, sndidx)
    end
end


--- Exported functions ---

-- Non-local control flow. These ones call the original error(), not our
-- redefinition in defs.ilua.
function longjmp()
    error(false)
end

function killit()
    -- TODO: guard against deletion of player sprite?
    error(true)
end
