
local con = require "con"

local spritesofstat = spritesofstat

local Inf = 0/1

-- Insert MUSICANDSFX? (Delete it otherwise.)
-- XXX: should be a per-player gamevar, but this is testing code.
local insp = false
local hitag, lotag = 0, 0

gameevent("JUMP",
function(aci, pli)
    local ps = player[pli]

    if (insp) then
        -- Insert MUSICANDSFX sprite with same lo-/hitag as last deleted one.

        local spr = sprite[con.spawn(aci, 5)]
        spr.lotag, spr.hitag = lotag, hitag
    else
        -- Delete nearest MUSICANDSFX sprite.

        local nearestdst = Inf
        local nearesti = -1

        for i in spritesofstat(gv.STAT_FX) do
            local dst = (sprite[i]-ps.pos):len2()
            if (nearesti == -1 or dst < nearestdst) then
                nearesti = i
                nearestdst = dst
            end
        end

        if (nearesti >= 0) then
            local spr = sprite[nearesti]
            lotag, hitag = spr.lotag, spr.hitag
            actor.delete(nearesti)
        end
    end

    insp = not insp
end)
