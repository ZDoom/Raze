
local xmath = require("xmath")

local sprite = sprite

--

local maputil = {}

-- csfunc = maputil.CreateCrossSpriteFunc(use_sprite_i_func, use_sprite_j_func)
--
-- use_sprite_i_func: function(spri, ud), where
--   <spri> is a reference to sprite i
-- use_sprite_j_func: function(sprj, spri, ud), where
--   <spri> is a reference to sprite i
--   <sprj> is a reference to sprite j
--
-- csfunc: function(userdata, process_sprite_j_func), where
--   <userdata> is passed as <ud> to the functions described above
--   <process_sprite_j_func>: function(sprj, spri, ud), the user-provided
--     function that processes sprite j in some way
function maputil.CreateCrossSpriteFunc(use_sprite_i_func, use_sprite_j_func)
    return function(userdata, process_sprite_j_func)
        for i in sprite.all() do
            if (use_sprite_i_func(sprite[i], userdata)) then
                for j in sprite.all() do
                    if (use_sprite_j_func(sprite[j], sprite[i], userdata)) then
                        process_sprite_j_func(sprite[j], sprite[i], userdata)
                    end
                end
            end
        end
    end
end


-- Functions for "for all sprites Y of tile X that are closer than D, ..."
-- Passed userdefs: { picnumi, picnumj, maxldist }
local function snearpic_usei(spr, ud)
    return spr.picnum == ud[1]
end

local function snearpic_usej(sprj, spri, ud)
    return sprj.picnum == ud[2] and xmath.ldist(spri, sprj) <= ud[3]
end

local snearpic_func = maputil.CreateCrossSpriteFunc(snearpic_usei, snearpic_usej)

-- maputil.for_sprites_near_picnum(picnumi, picnumj, dist, process_sprite_func)
--
-- Runs the following loop:
--
-- for all sprites i with tile <picnumi>,
--   for all sprites j of tile <picnumj> that are closer to sprite i than <dist> [*],
--     call process_sprite_func(sprite[j], sprite[i])
--
-- [*] using xmath.ldist()
function maputil.for_sprites_near_picnum(picnumi, picnumj, dist, process_sprite_func)
    snearpic_func({picnumi, picnumj, dist}, process_sprite_func)
end


return maputil
