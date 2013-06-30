-- Bound-checking functions for engine and game "things".

local ffiC = require("ffi").C
local type = type
local error = error

local con_lang = require("con_lang")

local bcheck = {}

--== ENGINE ==--

function bcheck.sector_idx(sectnum)
    if (sectnum >= ffiC.numsectors+0ULL) then
        error("invalid sector number "..sectnum, 3)
    end
end

function bcheck.wall_idx(wallnum)
    if (wallnum >= ffiC.numwalls+0ULL) then
        error("invalid wall number "..wallnum, 3)
    end
end

-- TODO: Provide another function that also checks whether the sprite exists in
-- the game world (statnum != MAXSTATUS).
function bcheck.sprite_idx(spritenum)
    if (spritenum >= ffiC.MAXSPRITES+0ULL) then
        error("invalid sprite number "..spritenum, 3)
    end
end

function bcheck.tile_idx(tilenum)
    if (tilenum >= ffiC.MAXTILES+0ULL) then
        error("invalid tile number "..tilenum, 3)
    end
end


--== GAME ==--

function bcheck.player_idx(snum)
    if (snum >= ffiC.playerswhenstarted+0ULL) then
        error("invalid player number "..snum, 3)
    end
end

function bcheck.sound_idx(sndidx)
    if (sndidx >= con_lang.MAXSOUNDS+0ULL) then
        error("invalid sound number "..sndidx, 3)
    end
end

function bcheck.weapon_idx(weap)
    if (weap >= ffiC.MAX_WEAPONS+0ULL) then
        error("Invalid weapon ID "..weap, 3)
    end
end

function bcheck.inventory_idx(inv)
    if (inv >= ffiC.GET_MAX+0ULL) then
        error("Invalid inventory ID "..inv, 3)
    end
end

function bcheck.volume_idx(volume)
    if (volume >= con_lang.MAXVOLUMES+0ULL) then
        error("invalid volume number "..volume, 3)
    end
end

function bcheck.level_idx(level)
    if (level >= con_lang.MAXLEVELS+0ULL) then
        error("invalid level number "..level, 3)
    end
end

function bcheck.quote_idx(qnum, onlyidx)
    if (qnum >= con_lang.MAXQUOTES+0ULL) then
        error("invalid quote number "..qnum, 3)
    end

    local cstr = ffiC.ScriptQuotes[qnum]
    if (onlyidx) then
        return cstr
    end

    if (cstr == nil) then
        error("null quote "..qnum, 3)
    end

    return cstr
end

function bcheck.top_level(funcname)
    if (ffiC.g_elCallDepth > 0) then
        error("Invalid use of "..funcname..": must be called from top level", errlev or 3)
    end
end

function bcheck.number(val, errlev)
    if (type(val)~="number" or val~=val) then
        error("invalid argument: must be a non-NaN number", errlev or 3)
    end
end

function bcheck.type(val, typestr, errlev)
    if (type(val)~=typestr) then
        error("invalid argument: must be a "..typestr, errlev or 3)
    end
end

return bcheck
