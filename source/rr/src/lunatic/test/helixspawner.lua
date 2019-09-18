
-- A spawner (NUKEBUTTON+3) of colored TRANSPORTERSTAR+4 sprites in a helical
-- arrangement.

local require = require
local math = require("math")

local con = require("con")
local xmath = require("xmath")

local gv = gv
local actor = actor
local player = player
local sprite = sprite

local gameactor = gameactor


module(...)

-- Dual-typed per-actor array: false if a broken nuke switch is not enabled,
-- start game tic of when it was enabled otherwise.
-- NOTE: for objects that are not supposed to be deleted such as this one, it
-- would also be OK to use a plain table.
local nukeswStart = con.actorvar(false)

-- This one should be a per-actor variable because it holds info about
-- "volatile" actors.
local starPal = con.actorvar(0)

-- Color per decasecond, can be changed from outside.
COLOR = { 1, 2, 6, 7, 8 }

require("end_gamevars")


local bangvec = xmath.bangvec
local angvec = xmath.angvec


local D = require("CON.DEFS")
local GTICSPERSEC = gv.GTICSPERSEC

gameactor
{
    D.TRANSPORTERSTAR+4,

    flags = actor.FLAGS.NOCLIP,

    move = con.move{100},
    movflags = actor.MOVFLAGS.geth,

    func = function(aci)
        local spr = sprite[aci]
        -- NOTE: this is prettier than calling it 'a', even if 'act' is used to
        -- denote an action in other places:
        local act = actor[aci]

        if (act:has_action(0)) then
            act:set_action(1)  -- TODO: actor constructors, i.e. 'init' callbacks

            local decasec = math.floor((gv.gametic - nukeswStart[spr.owner])/(GTICSPERSEC*10)) % 12

            local pal = COLOR[decasec+1]
            if (pal ~= nil) then
                starPal[aci] = pal
            end

            -- Every 2nd minute, we stop coloring the spawned stars. This tests
            -- per-actor variable resetting to the default value.
            spr.pal = starPal[aci]
        end

        if (act:checkbump()) then
            con.killit()
        end

        -- Test spr:changesect() vs. sprite.changesect()
        local sectnum = spr.sectnum

        for i=0,gv.numsectors-1 do
            if (spr.pal ~= 2 and spr.pal ~= 7) then
                sprite.changesect(aci, i)  -- noticeably faster...
            else
                spr:changesect(i)  -- ...than this
            end
        end

        sprite.changesect(aci, sectnum)
    end
}

local CS = sprite.CSTAT
local SPAWNSPERTIC = 10  --> 300/second --> 18000 per minute
local TWOPI = 2*math.pi

gameactor
{
    D.NUKEBUTTON+3,  -- destroyed end-of-level nuke switch

    function(aci)
        local spr = sprite[aci]

        for pi in player.all() do
            -- XXX: how to emulate "use switch" distance checking code, but in
            -- an actor-centric fashion?
            if (not nukeswStart[aci] and player.holdskey(pi, "OPEN")
                    and (player[pi].pos - spr):len2sq() < 256^2) then
                -- Enable us.
                nukeswStart[aci] = gv.gametic
                spr.cstatbits:clear(CS.TRANS_BITMASK)
                spr.cstatbits:set(CS.TRANS1)
                break
            end
        end

        local startgtic = nukeswStart[aci]
        if (not startgtic) then
            return
        end

        local hei, zofs = spr:getheightofs()
        local radius = hei/2

        for i=0,SPAWNSPERTIC-1 do
            -- Make one second go once round the circle, spawning
            -- SPAWNSPERTIC*GTICSPERSEC stars.
            local ii = ((gv.gametic*SPAWNSPERTIC)%(GTICSPERSEC*SPAWNSPERTIC)) + i
            local v = (radius/16)*angvec(ii*TWOPI/(GTICSPERSEC*SPAWNSPERTIC))
            local circvec = xmath.vec3(0, v.x, 16*v.y):rotate(spr.ang)
            local pos = spr^(zofs + radius) + 256*bangvec(spr.ang) + circvec

            con.insertsprite{D.TRANSPORTERSTAR+4, pos, spr.sectnum, actor.STAT.ACTOR, aci,
                             xrepeat=3, yrepeat=3, ang=spr.ang}
        end
    end
}
