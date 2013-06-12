
local require = require
local con = require "con"

local gv = gv
local sprite = sprite
local actor = actor
local player = player

local assert = assert

local printf = printf
local gameevent = gameevent
local spritesofstat = spritesofstat

local Inf = 0/1


module(...)


-- Insert MUSICANDSFX? (Delete it otherwise.)
insp = false

-- Hitag and lotag of last deleted MUSICANDSFX sprite.
tag = {}
tag.hi, tag.lo = 0, 0

-- Preliminary dummy of a local gamevar.
local ournumjumps = 0

require "end_gamevars"

-- We may cache globals defined in the gamevar section afterwards, but not
-- refer to locals defined prior to the gamevar section in it.
local tag = tag


gameevent{"JUMP", actor.FLAGS.chain_beg,
function(aci, pli)
    local ps = player[pli]

    ournumjumps = ournumjumps+1

    if (insp) then
        -- Insert MUSICANDSFX sprite with same lo-/hitag as last deleted one.
        printf("delmusicsfx: jump count=%d, inserting", ournumjumps)

        local spr = sprite[con.spawn(aci, 5)]
        spr.lotag, spr.hitag = tag.lo, tag.hi
    else
        -- Delete nearest MUSICANDSFX sprite.

        local nearestdst = Inf
        local nearesti = -1

        for i in spritesofstat(actor.STAT.FX) do
            local dst = (sprite[i]-ps.pos):len2()
            if (nearesti == -1 or (dst < nearestdst and dst < sprite[i].hitag)) then
                printf("MSFX %d dist %d", i, dst)
                nearesti = i
                nearestdst = dst
            end
        end

        if (nearesti >= 0) then
            local spr = sprite[nearesti]
            tag.lo, tag.hi = spr.lotag, spr.hitag
            actor.delete(nearesti)
        end

        assert(nearesti < 0 or sprite[nearesti].picnum==5)
        printf("delmusicsfx: jump count=%d, deleting sprite %d", ournumjumps, nearesti)
    end

    insp = not insp
end}
