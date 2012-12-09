-- Game control module for Lunatic.

local ffi = require("ffi")
local ffiC = ffi.C

local bit = require("bit")
local math = require("math")
local geom = require("geom")

local setmetatable = setmetatable

local error = error
local type = type

local player = assert(player)
local defs_c = require("defs_common")
local cansee = defs_c.cansee
local neartag = defs_c.neartag
local inside = defs_c.inside

-- NOTE FOR RELEASE: the usually global stuff like "sprite" etc. ought to be
-- accessed as locals here

module(...)


local lastid = { action=0, move=0, ai=0 }
local def = { action={}, move={}, ai={} }

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

function rotatesprite(x, y, zoom, ang, tilenum, shade, pal, orientation,
                      cx1, cy1, cx2, cy2)
    if (type(tilenum) ~= "number" or not (tilenum >= 0 and tilenum < ffiC.MAXTILES)) then
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

    if (ps.curr_weapon==ffiC.KNEE_WEAPON and have_weapon(weap)) then
        ffiC.P_AddWeaponMaybeSwitch(ps, weap);
    end
end


--- Functions that must be exported because they are used by LunaCON generated code,
--- but which are off limits to users.  (That is, we need to think about how to
--- expose the functionality in a better fashion than merely giving access to
--- the C functions.)

local D = {
    -- TODO: dynamic tile remapping
    ACTIVATOR = 2,
    APLAYER = 1405,

    FIRSTAID = 53,
    STEROIDS = 55,
    AIRTANK = 56,
    JETPACK = 57,
    HEATSENSOR = 59,
    BOOTS = 61,
    HOLODUKE = 1348,
}

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

-- NOTE: function args have overloaded meaning
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
            ps.knee_incs = -1
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
    local bits = ffiC.player[pli].sync.bits
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

local function krandand(mask)
    return bit.band(ffiC.krand(), mask)
end

local BANG2RAD = math.pi/1024

local function cossinb(bang)
    -- XXX: better use the precalc'd arrays instead?
    local ang = BANG2RAD*(bang)
    return 16384*math.cos(ang), 16384*math.sin(ang)
end

local function manhatdist(v1, v2)
    return math.abs(v1.x-v2.x) + math.abs(v1.y-v2.y)
end

-- "otherspr" is either player or holoduke sprite
local function A_GetFurthestVisiblePoint(aci, otherspr)
    if (bit.band(actor[aci].get_t_data(0), 63) ~= 0) then
        return
    end

    local angincs = (ud.player_skill < 3) and 1024 or 2048/(1+krandand(1))

    local j = 0
    repeat
        local c, s = cossinb(otherspr.ang + j)
        local hit = hitscan(otherspr^(16*256), otherspr.sectnum,
                            c, s, 16384-krandand(32767), ffiC.CLIPMASK1)
        local dother = manhatdist(hit.pos, otherspr)
        local dactor = manhatdist(hit.pos, spr)

        if (dother < dactor and hit.sect >= 0) then
            if (cansee(hit.pos, hit.sect, otherspr^(16*256), otherspr.sectnum)) then
                return hit
            end
        end

        j = j + (angincs - krandand(511))
    until (j >= 2048)
end

local SLEEPTIME = 1536

function _cansee(aci, ps)
    -- Select sprite for monster to target.
    local spr = sprite[aci]
    local s = sprite[ps.i]

    if (ps.holoduke_on) then
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
        local hit = A_GetFurthestVisiblePoint(aci, s)
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

function _canseetarget(spr, ps)
    -- NOTE: &41 ?
    return cansee(spr^krandand(41), spr.sectnum,
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
                hitspr, hitdist = A_CheckHitSprite(aci, angdifs[i])
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

local function _ifp(flags, pli, aci)
    local l = flags
    local ps = player[pli]
    local vel = sprite[ps.i].xvel
    local band = bit.band
    local j = false

    if (band(l,8)~=0 and ps.on_ground and _testkey(pli, SK.CROUCH)) then
        j = true
    elseif (band(l,16)~=0 and ps.jumping_counter == 0 and not ps.on_ground and ps.vel.z > 2048) then
        j = true
    elseif (band(l,32)~=0 and ps.jumping_counter > 348) then
        j = true
    elseif (band(l,1)~=0 and vel >= 0 and vel < 8) then
        j = true
    elseif (band(l,2)~=0 and vel >= 8 and not _testkey(pli, SK.RUN)) then
        j = true
    elseif (band(l,4)~=0 and vel >= 8 and _testkey(pli, SK.RUN)) then
        j = true
    elseif (band(l,64)~=0 and ps.pos.z < (sprite[_aci].z-(48*256))) then
        j = true
    elseif (band(l,128)~=0 and vel <= -8 and not _testkey(pli, SK.RUN)) then
        j = true
    elseif (band(l,256)~=0 and vel <= -8 and _testkey(pli, SK.RUN)) then
        j = true
    elseif (band(l,512)~=0 and (ps.quick_kick > 0 or (ps.curr_weapon == ffiC.KNEE_WEAPON and ps.kickback_pic > 0))) then
        j = true
    elseif (band(l,1024)~=0 and sprite[ps.i].xrepeat < 32) then
        j = true
    elseif (band(l,2048)~=0 and ps.jetpack_on) then
        j = true
    elseif (band(l,4096)~=0 and ps:get_inv_amount(ffiC.GET_STEROIDS) > 0 and ps:get_inv_amount(ffiC.GET_STEROIDS) < 400) then
        j = true
    elseif (band(l,8192)~=0 and ps.on_ground) then
        j = true
    elseif (band(l,16384)~=0 and sprite[ps.i].xrepeat > 32 and sprite[ps.i].extra > 0 and ps.timebeforeexit == 0) then
        j = true
    elseif (band(l,32768)~=0 and sprite[ps.i].extra <= 0) then
        j = true
    elseif (band(l,65536)~=0) then
        -- TODO: multiplayer branch
        if (_angdiffabs(ps.ang, ffiC.getangle(sprite[_aci].x-ps.pos.x, sprite[_aci].y-ps.pos.y)) < 128) then
            j = true
        end
    end

    return j
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
   ps.visibility = -127
   ffiC.lastvisinc = ffiC.totalclock+32
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
        return (ud.respawn_monsters~=0)
    end
    if (INVENTILE[spr.picnum]) then
        return (ud.respawn_inventory~=0)
    end
    return (ud.respawn_items~=0)
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
