
local bit = require("bit")
local band = bit.band

local tostring = tostring

local gv = gv
local actor = actor
local sector, wall, sprite = sector, wall, sprite

local printf = printf
local sectorsofbunch = sectorsofbunch

local con = require("con")

local D = require("CON.DEFS")

----------

local TROR_GLASSBREAKER = 2959  -- red 'T'
-- Actor controlling the timing of a TROR hplane breaking.
gameactor
{
    TROR_GLASSBREAKER,

    function(aci, pli, dist)
        local spr = sprite[aci]
        if (not (spr.lotag == 712 and spr.hitag == 119)) then  -- check BREAKER_MAGIC
            sprite.changestat(aci, actor.STAT.DEFAULT)
            return
        end

        local cnt = actor[aci]:get_count()
        local finish = (cnt >= 6)

        if (cnt == 0) then
            -- NOTE: INTERNAL interface, DON'T USE!
            con._sound(aci, D.GLASS_BREAKING)
        end

        local bunchnum = spr.extra
        for sectnum, what in sectorsofbunch(bunchnum, gv.BOTH_CF) do
            local cf = sector[sectnum][what]
            cf:set_picnum(D.GLASS2 + cnt)
            cf.statbits:clear(sector.STAT.BLOCK + sector.STAT.HITSCAN)
            if (finish) then
                cf.statbits:clear(sector.STAT.TRANS_BITMASK)
            end
        end

        if (finish) then
            con.killit()
        end
    end
}


local DHP = sector.DAMAGEHPLANE

gameevent
{
    "DAMAGEHPLANE",

    function(aci, pli, RETURN)
        local what, sectnum = sector.damagehplane_whatsect(RETURN)
        local sec = sector[sectnum]

        -- Part I: make various screens breakable when it's a ceiling picnum.
        if (what == "ceiling") then
            -- hit ceiling
            if (sec.ceilingpicnum >= 263 and sec.ceilingpicnum <= 275) then
                sec:set_ceilingpicnum(D.W_SCREENBREAK + gv.krand()%3)
                gv.RETURN = DHP.GLASSBREAK
                return con.longjmp()
            end
        end

        gv.RETURN = DHP.DEFAULT
    end
}

gameevent
{
    "DAMAGEHPLANE",

    function(aci, pli, RETURN)
        local what, sectnum = sector.damagehplane_whatsect(RETURN)
        local sec = sector[sectnum]

        -- Part II: breakable TROR hplanes
        local cf = sec[what]
--        printf("damage %s of sector %d (pic %d, bunch %d, hittable: %s)", what, sectnum,
--               cf.picnum, cf.bunch, tostring(cf.statbits:test(sector.STAT.HITSCAN)))

        if (cf.bunch >= 0 and (cf.picnum==198 or cf.picnum==D.GLASS2) and
                cf.statbits:test(sector.STAT.HITSCAN)) then
            local bi = con.insertsprite(TROR_GLASSBREAKER, wall[sec.wallptr], sectnum, actor.STAT.ACTOR)
            local breaker = sprite[bi]

            breaker.cstat = sprite.CSTAT.INVISIBLE
            breaker.lotag, breaker.hitag = 712, 119  -- BREAKER_MAGIC
            breaker.extra = cf.bunch

            gv.RETURN = DHP.SUPPRESS
            return con.longjmp()
        end

        gv.RETURN = DHP.DEFAULT
    end
}
