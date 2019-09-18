-- test script for ELua/Lunatic Interpreter

-- error=nil -- must not affect "require"
local require = require
local string = require("string")
local bit = require("bit")
local math = require("math")

local pcall = pcall
local DBG_ = require("_LUNATIC_DBG")

local gv, sector, wall, sprite, spriteext = gv, sector, wall, sprite, spriteext
local actor, player, projectile = actor, player, projectile
local gameevent, gameactor = gameevent, gameactor

local spritesofsect = spritesofsect
local hitscan = hitscan

local assert, error, print, tostring = assert, error, print, tostring

local D = require("CON.DEFS")

print('---=== ELua Test script ===---')

local function printf(fmt, ...)
    print(string.format(fmt, ...))
end

local function checkfail(funcstr, expectedmsg)
    if (DBG_ == nil) then
        return
    end

    local status, errmsg = pcall(DBG_.loadstring(funcstr))
    if (status) then
        print('^21ERROR:^O '..funcstr.." DIDN'T fail")
    else
        if (expectedmsg==nil or string.find(errmsg, expectedmsg, 1, true)) then
            print("^11SUCCESS:^O "..funcstr.." failed: "..errmsg)
        else
            -- XXX: beginning with "^10" is counted as error in OSD_Printf()
            print("^10ERROR*:^O "..funcstr.." failed: "..errmsg..
                  ", but expected error message was: "..expectedmsg)
        end
    end
end

gameevent
{
    "ENTERLEVEL",

    function()
        local vol, lev = gv.currentEpisode(), gv.currentLevel()
        printf('volume=%d, level=%d', vol, lev)

        if (vol ~= 4) then
            -- Tweak some sector pals.
            print('tweaking sector pals')
            print('numsectors: ' .. gv.numsectors .. ' of ' .. gv.MAXSECTORS)

            local SF = sector.STAT
            for i = 0, gv.numsectors/2 do
                local sec = sector[i]
                sec.floorpal = 1;
                sector[i].floor.shade = sec.floor.shade + 4
                sector[i].ceilingpal = 2;
                local ceil = sec.ceiling
                ceil.shade = sector[i].ceiling.shade + 8
                ceil.statbits:flip(SF.SMOOSH)
                sec.floorstatbits:flip(SF.SWAPXY)
            end
        end

        if (vol==1 and lev==1) then  -- E1L1
            print('tweaking some sprites 2')
            local i = 562
            spriteext[i].alpha = 0.5;
            sprite[i].cstat = bit.bor(sprite[i].cstat, 2+512);
            spriteext[i].pitch = 128;
            spriteext[i].roll = 256;

            i = 107  -- pistol ammo at rooftop
            spriteext[i].pitch = 128;
            spriteext[i].roll = 256;

            for spr in spritesofsect(307) do  -- some fence sprites in E1L1
                printf('spr %d', spr)
                sprite[spr].pal = 6
            end

            actor[562].flags = bit.bor(actor[562].flags, 2);   -- pal 6 with goggles on front SEENINE
        end
---[[
        if (vol==1 and lev==8) then
            local havebunch = false
            for i=0,gv.numsectors-1 do
                havebunch = havebunch or (sector[i].ceilingbunch >= 0)
            end
            if (havebunch) then
                print('tweaking bunch 1');
                -- trueror1.map
                for i, what in sectorsofbunch(1, gv.BOTH_CF) do
                    sector[i][what].z = sector[i][what].z - 3*1024;
                end
            end
        end
--]]
    end
}

gameevent
{
    "JUMP",

    function()
        print("tweaking forcefield with lotag 34 (E2L1)")

        for w=0,gv.numwalls-1 do
            local wal = wall[w]
            if (wal.overpicnum == D.W_FORCEFIELD or wal.overpicnum == D.W_FORCEFIELD+1) then
                if (wal.lotag==34) then
                    wal.cstat = wal.cstatbits:test(85) and 0 or 85
                end
            end
        end
    end
}

local unsafe = pcall(function() string.UNSAFE=true; end)

checkfail("tostring = nil", "attempt to write into the global environment")
--DBG_.printkv('_G in test.elua', _G)

-- direct gv array access forbidden
checkfail('gv.sprite[0].yrepeat = 100', "access forbidden")

checkfail('print(sprite[100000].ceilingpal)', "out-of-bounds sprite[] read access")

checkfail('print(gv.sprite[0])', "access forbidden")

-- set metatable forbidden
checkfail('setmetatable(sprite, {})', "attempt to read undeclared variable 'setmetatable'")

gameevent
{
    "ENTERLEVEL",

    function()
        -- OOB write access.
        -- Note that indexing ("reading") sector fails, even if the user wants to
        -- assign to a sector member. Potentially confusing error message.
        checkfail('sector[-1].ceilingpal = 4', "out-of-bounds sector[] read access")

        -- wallnum member is read-only
        checkfail('sector[0].wallnum = 0', "attempt to write to constant location")  -- this comes from LJ/FFI

        -- direct sector write access forbidden
        checkfail('sector[4] = sector[6]', "cannot write directly to sector[]")

        -- creating new keys forbidden... handled by LuaJIT
        checkfail('wall[4].QWE = 123', "has no member named 'QWE'")

        -- no pointer arithmetic!
        checkfail('local spr = sprite[0]; local x=spr+1', "attempt to perform arithmetic on")

        -- actor[].t_data[] is not accessible for now
        checkfail('local i = actor[0].t_data[15]', "has no member named 't_data'")

        -- sprite.picnum may happen as a thinko/typo kind of error (spr.picnum was meant)
        checkfail("local pic = sprite.picnum", "invalid access to static data")

        checkfail("require('engine').setshadetab(200, nil)",
                  "setshadetab() may be run only while LUNATIC_FIRST_TIME is true")

        checkfail("sprite[0]:set_picnum(-10)", "invalid tile number")
    end
}

-- gv.numsectors is read-only
checkfail('gv.numsectors = 4', "attempt to write to constant location")

-- cannot create new fields in 'gv'
checkfail('gv.QWE = 4', "write access forbidden")

-- that would be horrible...
checkfail('sprite._nextspritesect[4] = -666', "cannot write directly to nextspritesect[]")

-- we're indexing a plain array!
checkfail('print(sprite._nextspritesect[4].whatfield)', "attempt to index a number value")

-- our 'require' has only safe stuff
--checkfail("require('os')")

-- gamevars are created using a special different mechanism
checkfail("new_global = 345", "attempt to write into the global environment")
-- Can't reassign to existing vars either
assert(actor ~= nil)
checkfail("actor = 345", "attempt to write into the global environment")

-- can't redefine constants in 'gv'
checkfail('gv.CEILING = 3', "attempt to write to constant location")

-- string.dump is unavailable
checkfail('local s=require[[string]]; local tmp=s.dump(gameevent)',
          "attempt to call field 'dump' (a nil value)")

if (not unsafe) then
    -- changing base module tables is disallowed
    checkfail('local s=require[[string]]; s.format=nil', "modifying base module table forbidden")
else
    print('WARNING: RUNNING WITH UNPROTECTED BASE MODULES')
end

print('')
-- This is problematic, even though pretty much every access will yield a
-- "missing declaration" error.
-- See http://luajit.org/ext_ffi_api.html#ffi_C about what stuff ffi.C contains.
checkfail('gv.luaJIT_setmode(nil, 0, 0)', "missing declaration for symbol 'luaJIT_setmode'")

checkfail('gv.luaJIT_BC_con_lang', "attempt to call a nil value")
checkfail('gv.gethiticks = nil', "attempt to write to constant location")

checkfail('gameactor{1680, 0}', "must provide a function with last numeric arg or .func")

checkfail("do local bt=require'test.test_bitar'; bt.QWE=1; end", "modifying module table forbidden")
-- the cdata returned by player[] can't be made into a pointer!
checkfail("do local pl=player[0]; i=pl[1]; end")
checkfail("do local ud=gv.ud.camerasprite; end", "access forbidden")  -- test for proper decl()
checkfail("gv.g_sizes_of=nil; print(gv.g_sizes_of[0])", "write access forbidden")
checkfail("gv.cam.sect=-1", "invalid sector number")
checkfail("local flag=gv.SFLAG_NULL", "missing declaration")

-- NOTE: player[0] is forbidden at file scope, this is just for testing purposes.
player[0].wackedbyactor = -1  -- should succeed
checkfail("player[0].curr_weapon = -1", "Invalid weapon ID")
player[0].curr_weapon = 1
checkfail("local w = player[0].weapon[-1]", "out-of-bounds weapon read access")

-- XXX: This gives a very strange error message: "attempt to write to constant location". Why?
-- (note how I forgot to index weapon with a weapon index or name)
--player[0].weapon.firesound = 1e5
checkfail("player[0].weapon.SHOTGUN.firesound = 1e5", "invalid sound number")
checkfail("player[0].weapon.SHOTGUN.firesound = 0/0", "must be a non-NaN number")
checkfail("player[0].weapon.SHOTGUN.firesound = 1/0", "invalid sound number")
checkfail("gameactor{1680, action=require('con').action{numframes=-10}, function() end}",
          "action has negative number of frames")
-- NOTE: It should only be relied on that setting e.g. .firesound to -1 sets it
-- to 0, not other negative values.
player[0].weapon.SHOTGUN.firesound = -1/0
assert(player[0].weapon.SHOTGUN.firesound == 0)

gameevent{gv.EVENT_JUMP,
          function(actori, playeri, dist)
              printf("jump i=%d p=%d d=%d", actori, playeri, dist)
              error("greetings from EVENT_JUMP")
          end
         }

--[[
gameevent
{
    "PROCESSINPUT",

    -- Input test.
    -- NOTE: I don't think that exposing g_player[].sync (aka "input") is a good idea...
    func = function(actori, playeri, dist)
        local IB = player._INPUT_BITS
        local input = player[playeri]._input
        if (bit.band(input.bits, IB.JUMP) ~= 0) then
            print("JUMPED")
            -- ... because for example this doesn't work
            -- (P_HandleSharedKeys, where the JETPACK bit is tested, runs
            -- before P_ProcessInput):
            input.bits = bit.bor(input.bits, IB.JETPACK)
        end
    end
}
--]]

local WEAPON = gv.WEAPON

-- Bad hack to test out pitch/roll: EVENT_GAME is discouraged from Lunatic.
local PITCH_PICNUM = { [D.SEENINE]=true, }
gameevent{ "GAME",
    function(aci)
        local spr = sprite[aci]
        if (PITCH_PICNUM[spr.picnum]) then
            local height = spr:getheightofs()

            local sexy = spriteext[aci]
            sexy.pitch = gv.totalclock
            sexy.mdoff.x = -height/16  -- test xoff + pitch
            sexy.mdoff.z = -height  -- test zoff + pitch

            -- Test roll + yoff
            sexy.roll = gv.totalclock
            sexy.mdoff.y = -height/16
        end
    end
}

-- test event chaining
gameevent
{
    "JUMP",

    flags = actor.FLAGS.chain_beg,

    function(actori, playeri, dist)
        local ps = player[playeri]
        print("\n--- I'm first!")
--        DBG_.oom()
        local pistol = ps.weapon.PISTOL
        if (pistol.shoots ~= D.RPG) then
            pistol.shoots = D.RPG
        else
            pistol.shoots = D.SHOTSPARK1
        end
        ps.weapon[WEAPON.PISTOL].firesound = D.LIGHTNING_SLAP

        -- This succeeds, because sound2time is a time, not a sound.
        ps.weapon.SHOTGUN.sound2time = 5000
        printf("set shotgun's sound2time to %d", ps.weapon.SHOTGUN.sound2time)

        -- Set pipebomb and tripbomb to timed mode.
        -- XXX: Provide either named constants or methods?
        -- XXX: These are probably reset to default on new game.
        ps.pipebombControl = 2
        ps.tripbombControl = 2

        -- Test of INTERNAL member _pals.
        -- NOTE: setting colors partially is bad! E.g. after an item is
        -- picked up, col[0] and col[1] remain and tint everything greenish.
        if (DBG_ ~= nil) then
            ps._pals[2] = 20
            ps._pals.f = 30
        end
    end
}

local xmath = require "xmath"

gameevent
{
    gv.EVENT_ENTERLEVEL,

    function()
        if (DBG_ ~= nil) then
            DBG_.testmembread()
        end

        -- NOTE: times are for helixhorned (Core2Duo 3GHz)
        local i
        local N = 1e6
        local t = gv.gethiticks()

        for i=3,N do
            gv.gethiticks()
        end

        t = gv.gethiticks()-t

        -- x86_64: 35ns/call, x86: 280 ns/call
        -- Windows 32-bit: about 1 us/call?
        printf("%d gethiticks() calls took %.03f ms (%.03f us/call)",
               N, t, (t*1000)/N)

        local sum=0
        t = gv.gethiticks()
        for i=1,N do sum = sum+gv.ksqrt(i) end
        t = gv.gethiticks()-t
        -- x86_64: 14ns/call
        printf("%d ksqrt() calls took %.03f ms (%.03f us/call) [sum=%f]",
               N, t, (t*1000)/N, sum)

        sum=0
        t = gv.gethiticks()
        for i=1,N do sum = sum+math.sqrt(i) end
        t = gv.gethiticks()-t
        -- x86_64: 7ns/call
        printf("%d math.sqrt() calls took %.03f ms (%.03f us/call) [sum=%f]",
               N, t, (t*1000)/N, sum)

        printf("sqrt(0xffffffff) = %f(ksqrt) %f(math.sqrt)",
               gv.ksqrt(0xffffffff), math.sqrt(0xffffffff))

        local pl = player[0]
        -- MAX < current is "allowed"
        pl.max_ammo_amount[WEAPON.RPG] = 17
        pl:give_weapon(WEAPON.RPG)
        pl.ammo_amount[WEAPON.RPG] = 54

        pl:give_weapon(WEAPON.SHRINKER)
        -- This looks much prettier:
        pl.ammo_amount.SHRINKER = 2

        -- MORTER2 from test/weaponvars.con
        local PNUM = 1653
        local proj = projectile[PNUM]
        if (proj ~= nil) then
            printf('Have projectile %d', PNUM)
            player[0].weapon.SHOTGUN.shoots = PNUM
            proj.drop = 0
            proj:set_trail(D.SMALLSMOKE)
        else
            printf('^10Do NOT have projectile %d, test/weaponvars.con not loaded?', PNUM)
        end

        if (gv._LUNATIC_STRICT == 0) then
            t = gv.gethiticks()
            local N=1
            for n=1,N do
                for i=0,gv.MAXSPRITES-1 do
                    sprite[i].blend = 1
                end
                for i=gv.MAXSPRITES-1,0,-1 do
                    sprite[i].shade = 1
                end
                for i=0,gv.MAXSPRITES-1 do
                    sprite[i].xoffset = 0
                end
                for i=gv.MAXSPRITES-1,0,-1 do
                    sprite[i].yoffset = 1
                end
            end
            t = gv.gethiticks()-t
            printf("%d x four 0..MAXSPRITES-1 iterations took %.03f us per outer iteration", N, (1000*t)/N)
            -- Results on x86:
            -- N=1: 480-1000 us (too large variance)
            -- N=10: 190-210 us * 10 = 1.9-2.1 ms
            -- N=100: about 160 us * 100 = about 16 ms
        end

        -- Make the DUKECAR in E1L1 into a zombie actor (temporarily)
        -- NOTE: Use static value (not the one from 'D').
        if (sprite[24].picnum==2491) then
            sprite.changestat(24, actor.STAT.ZOMBIEACTOR)
        end

        checkfail("gameevent('GAME', function() print('qwe') end)",
                  "must be called from top level")

        -- Test vec3 + wall. Pseudo wall member 'z' will be accessed.
        local mpos = xmath.vec3()
        for i=0,gv.numwalls-1 do
            mpos = mpos + wall[i]
        end
        mpos = mpos/gv.numwalls
        local impos = xmath.ivec3(mpos)^20  -- test ivec3 with dvec3 arg, test '^' op
        assert(impos.z == -20)
        printf("Map center point: (%d,%f)", mpos.x, impos.y)
    end
}

gameevent{"LOADACTOR", function(i)
    local spr = sprite[i]
    if (i==614 and spr.picnum==930) then
        -- "police line" ribbon in E1L1
        -- clear the hitag so that it doesn't spawn as FALLER from premap
        -- Rather a HACK: relies on an implementation detail (A_Spawn()
        -- "hard-wired" code).
        spr.hitag = 0
    end
end}

gameactor
{
    -- "police line" ribbon
    930, nil, 1,

    func = function(i)
        local spr = sprite[i]
        local r = math.random
        local d = 20
        -- NOTE: __add metamethod is called because of the RHS:
        local v = spr + xmath.vec3(r(-d,d), r(-d,d))
        spr:setpos(v):updatesect()

        -- Test vec3 constructor with cdata.
        local tempvec = xmath.vec3(player[0].pos)
    end
}

local stat = require("stat")
local hs = stat.new()

local con = require("con")
local AC, MV = con.AC, con.MV

local CAC, CMV, CAI = require("CON.ACTION"), require("CON.MOVE"), require("CON.AI")
assert(CAC); assert(CMV); assert(CAI)

local AC, MV = {}, {}

AC.TROOPSTAND = assert(CAC.ATROOPSTAND) -- or con.action{0,1,5,1,1}
AC.TROOPFLINTCH = con.action{50, 1, 1, 1, 6}
MV.SHRUNKVELS = con.move{hvel=32}
con.ai(AC.TROOPFLINTCH, MV.SHRUNKVELS, 0)  -- unused; TODO: test

local TROOPSTRENGTH = 30

local AF = actor.FLAGS
local CS = sprite.CSTAT

-- Crosshair sprite.
-- NOTE: This ought to be a gamevar -- if a savegame is restored, a new one
-- will be spawned.
local chair

gameactor{ D.APLAYER, AF.chain_end,
    function(aci, pli)
        if (chair == nil) then
            chair = con.spawn(555, aci)
            printf("Spawned our crosshair: sprite %d", chair)
            local spr = sprite[chair]
            -- Set to STAT_MISC because otherwise interpolation goes crazy (old
            -- value never updated; dunno why...)
            sprite.changestat(chair, actor.STAT.MISC)
            spr.xrepeat, spr.yrepeat = 96, 96
            spr.cstatbits:set(CS.CENTER)
        end

        local ps = player[pli]
        local ray = xmath.kangvec(ps.ang, -(ps.horiz-100)*2048)

        local hit = hitscan(ps.pos, ps.cursectnum, ray, 0)
        if (hit.sector >= 0) then
            sprite[chair]:setpos(hit.pos)
            sprite.changesect(chair, hit.sector)
        end
    end
}

-- Add NODAMAGEPUSH flag to NEWBEAST.
gameactor { D.NEWBEAST, AF.chain_end + AF.NODAMAGEPUSH, function() end }

-- Also test actor code chaining: strength is doubled.
gameactor
{
    D.LIZTROOP, AF.chain_end+AF.enemy, 2*TROOPSTRENGTH,

    action = AC.TROOPSTAND,

    func = function(i, playeri, dist)
        sprite[i].pal = math.random(32)
--        sprite[i].ang = bit.band(sprite[i].ang-20, 2047)

        local spr = sprite[i]

        local t = gv.gethiticks()
        local hit = hitscan(spr, spr.sectnum, {x=10, y=10, z=0}, gv.CLIPMASK0)

        hs:add(1000*(gv.gethiticks()-t))

        if (hs.n == 300) then
            printf("hitscan: %s", hs:getstatstr())
            hs:reset()
            error("greetings from LIZTROOP actor")
        end

        local actr = actor[i]
        if (actr:get_count() % 30 == 0) then
            spr.cstatbits:flip(CS.YFLIP)
        end

        -- Test of bitint's ":test()" for actor[].flags.
        actr.flagsbits:test(AF.NVG)

        if (dist < 4096) then
            -- Duke Vader / Anakin Nukewalker?
            actor[i]:set_action(AC.TROOPFLINTCH)
            actor[i]:set_move(MV.SHRUNKVELS)

            if (dist < 1024) then
                con.killit()
            end
        end

        if (actr:has_action(CAC.ATROOPWALKING)) then
            if (actr:get_count() % 50 == 0) then
                actr.movflagsbits:flip(actor.MOVFLAGS.spin)
            end
        end
    end,

    -- NOTE: the animate callback is not yet documented and thus not official API!
    animate = function(tspr)
        local tspr2 = tspr:dup()
        if (tspr2) then
            tspr2.x = tspr2.x + 512*math.cos(gv.totalclock/60)
            tspr2.y = tspr2.y + 512*math.sin(gv.totalclock/60)
            tspr2.cstatbits:set(CS.TRANS_BITMASK)
        end

        -- XXX: inserted tsprites have floor shadow in classic! (r_shadow)
        -- G_DoSpriteAnimations() is passed as callback to the engine on occasion,
        -- in other words, created tsprites may be fed back to G_DoSpriteAnimations()!
        -- classic: shows shadow for both "ghost" liztroop and aim "reticle"
        -- Polymost: only for "ghost"
        -- Polymer: none
        local aimv = 256*xmath.bangvec(tspr.ang)
        local hit = hitscan(tspr^(16*256), tspr.sectnum, aimv, gv.CLIPMASK1)

        if (hit.wall >= 0) then
            local aimtspr = tspr:dup()
            if (aimtspr) then
                aimtspr.pal = 2
                aimtspr:set_picnum(555)
                aimtspr:setpos(hit.pos, hit.sector)
            end
        end
    end,
}

gameactor
{
    1275,  -- newspaper, E4L6 sprite #513

    action = con.action{0, 4, delay=20},  -- Same as {0, 4, 1, 1, 20}
    move = 1,  -- so that the ID is nonzero and it will move

    func = function(aci)
        local a = actor[aci]
        local delay = math.sin(0.1 * 2*math.pi*gv.totalclock/120)
        a:set_action_delay(20 + 10*delay)
        if (sprite[aci].pal ~= 0) then
            a:set_hvel(1024/30)
            a:set_vvel(-1024/30)
        else
            a:set_hvel(50*delay)
        end
        a.movflags = actor.MOVFLAGS.geth + actor.MOVFLAGS.getv
    end,
}

gameevent
{
    "DISPLAYROOMS",

    function(aci, pli)
        local ps = player[pli]
        local cam = gv.cam

        if (ps.newowner < 0) then
            cam.pos.z = cam.pos.z + 64*16*math.sin(gv.totalclock/30)
        end

        if (ps.cursectnum >= 0) then
            local hit = sector[ps.cursectnum]:zrangeat(cam.pos)
            if (gv.totalclock%200==0) then
                printf("hit %s %d at z=%d, %s %d at z=%d",
                       hit.c.spritep and "sprite" or "sector", hit.c.num, hit.c.z,
                       hit.f.spritep and "sprite" or "sector", hit.f.num, hit.f.z)
            end
        end
    end
}

gameevent
{
    "DISPLAYREST",

    function()
        for i=0,10 do
            for j=1,100 do
                -- XXX: This is slower in the Polymodes than with classic!
--                con._gametext(2822, j, i, 12, 0, 0, 16, 0,0,gv.xdim,gv.ydim,8192)
            end
        end

        -- Clear showing every sector with the pavement floor tile. (Unless we're
        -- in such a sector or an adjoining one.)
        -- XXX: We need a better place to do this, maybe an event in
        -- G_DisplayRest() where show2dsector[] is tweaked?
        -- NOT YET OFFICIAL API.
        local show2dsector = sector.showbitmap
        for i=0,gv.numsectors-1 do
            if (sector[i].floorpicnum==815) then
                show2dsector:set0(i)
            end
        end
    end
}

gameactor
{
    D.APLAYER, actor.FLAGS.chain_beg,

    function(aci, pli)
        if (con._squished(aci, pli)) then
            printf("Squished LunaCON test")
        end
    end
}

gameactor
{
    -- Innocent sign, similar to test/weaponvars.con actor 909 (tree trunk)
    1211, actor.FLAGS.replace,

    function(aci, pli)
        local a = actor[aci]

        if (a:get_count() >= 120) then
            local i = con.spawn(D.TRANSPORTERSTAR, aci)
            sprite[i].z = sprite[i].z - 1024*16
            con.shoot(D.MORTER, aci, -4096)
            a:set_count(0)
        end
    end
}

local function testbit(num, b)
    return bit.band(num,b)~=0 and 1 or 0
end

local FADE_SPEED = {
    [WEAPON.KNEE] = 1/2.5,

    1/128,
    1/5,
    1/3,
    1/2,
    1,  -- pipebomb: ~1 sec
    2,
    3,
    5,
    127,  -- freezer; such a fast fade is not visible, but it clears any
          -- existing one (if of higher priority)
    [WEAPON.GROW] = 9.9,  -- test banker's rounding -- should be like 10
}

-- Test player[]:fadecol(), a better palfrom.
gameevent
{
    "CHANGEWEAPON",

    function (aci, pli)
        local ps = player[pli]
        local neww, curw = gv.RETURN, ps.curr_weapon

        local r, g, b = testbit(neww, 1), testbit(neww, 2), testbit(neww, 4)
        local speed = FADE_SPEED[neww] or 1
        player[pli]:fadecol(0.5, r, g, b, speed, neww-5)
    end
}

-- Time the above p:fadecol() test for verification of the <speed> argument.
local last_f, last_t = 0, 0
local last_secs = nil
gameevent
{
    "DISPLAYREST",

    function(aci, pli)
        local ps = player[pli]
        -- WARNING: _pals in INTERNAL and off-limits to users!
        local curf = ps._pals.f
        if (curf > last_f) then
            -- Starting a tint
            last_secs = nil
            last_f = curf
            last_t = gv.getticks()
        elseif (last_t > 0 and curf==0) then
            -- Fade has ended
            last_secs = (gv.getticks()-last_t)/1000
            last_f, last_t = 0, 0
        end

        if (last_secs ~= nil) then
            con.minitext(10, 10, string.format("Last fade time: %.03f s (%.03f gametics)",
                                               last_secs, last_secs*30))
        end
    end,
}

printf("EVENT_INIT = %d", gv.EVENT_INIT)  -- tests default defines

local bittest = require "test.test_bitar"
bittest.sieve()

require("test.test_geom", 123123)
require("test.test_rotspr")

do
    -- Test ksin vs. sinb
    local xmath = require "xmath"
    local sum = 0

    local N = 1000
    local t = gv.gethiticks()
    for i=0,N*2048-1 do
        sum = sum+xmath.ksin(i)
    end
    t = gv.gethiticks()-t
    sum = sum*1e12
    printf("ksin: sum*1e12=%.03f, %.03fus per 0-2047 cycle", sum, t)

    sum = 0
    local t = gv.gethiticks()
    for i=0,N*2048-1 do
        sum = sum+xmath.sinb(i)
    end
    t = gv.gethiticks()-t
    sum = sum*1e12
    printf("sinb: sum*1e12=%.03f, %.03fus per 0-2047 cycle", sum, t)

    -- Results (helixhorned x86):
    -- ksin: sum*1e12=0.000, 3.801us per 0-2047 cycle
    -- sinb: sum*1e12=0.052, 2.886us per 0-2047 cycle
end

do
    -- Test getflorzofslopeptr()/sector[]:floorzat()
    local N = 100
    local t = gv.gethiticks()

    for n=1,N do
        for i=0,gv.numsectors-1 do
            local sec = sector[i]
            local w1 = sec.wallptr
            sec:floorzat(wall[w1])
        end
    end

    printf("%d x %d floorzat: %.03f us", N, gv.numsectors, (gv.gethiticks()-t)*1000)

    -- Results for 100 x 325 floorzat (helixhorned x86):
    -- cdecl getflorzofslope(): 572.165 us
    -- __fastcall getflorzofslope(): 830.147 us (sic, but tested only once!)
end

print('---=== END TEST SCRIPT ===---')

--[[
function check_sector_idx()
    error("bla")
end
spritesofsect(0)
--]]

--DBG_.oom()

-- This will complain about wrong usage of 'error'. In particular,
-- the nil must not propagate to C!
checkfail('error(nil)', "error using 'error': error message must be a string")

local D = require("CON.DEFS")
checkfail('require("CON.DEFS").APLAYER=123', "modifying base module table forbidden")
-- Test with lunatic/test/rotfixed_actor.con.
print("DUKECAR="..tostring(D.DUKECAR))

do
    print('---------- getangle test')

    local function CreateGetAngFunc(roundfunc)
        return function(x, y)
            local ang = (1024/math.pi)*math.atan2(y, x)  -- note the swapped args
            return bit.band(roundfunc(ang), 2047)
        end
    end

    local ourgetang = CreateGetAngFunc(math.ceil)
    local ourgetangf = CreateGetAngFunc(math.floor)

    local function printang(x, y)
        printf('%4d,%4d: %13d, %16d, %16d', x, y,
               gv.getangle(x, y), ourgetang(x, y), ourgetangf(x, y))
    end

    print "   x,   y: getangle(x, y) | math.atan2/ceil | math.atan2/floor"
    printang(10, 100)
    printang(10, -100)
    printang(-10, -100)
    printang(-10, 100)

    printang(0, 0)

    printang(1, 0)
    printang(0, 1)
    printang(-1, 0)
    printang(0, -1)

    local N=1e5
    local r = 0

    local t = gv.gethiticks()
    for i=1,N do
        r = r + gv.getangle(10, i)
    end
    local t1 = gv.gethiticks()-t

    r = 0
    t = gv.gethiticks()
    for i=1,N do
        r = r + ourgetang(10, i)
    end
    local t2 = gv.gethiticks()-t

    printf('Time for %d runs: getangle: %.03f ms, math.atan2: %.03f ms', N, t1, t2)
    print('----------')
end

if (LUNATIC_FIRST_TIME) then
    require("test.shadexfog").test_create0()
end
