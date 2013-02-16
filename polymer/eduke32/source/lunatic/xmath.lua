-- "Extended" math module for Lunatic.

local ffi = require("ffi")

local bit = require("bit")
local math = require("math")

local assert = assert


module(...)


local BANG2RAD = math.pi/1024
local isintab = ffi.new("int16_t [?]", 2048)
local dsintab = ffi.new("double [?]", 2048)

for a=0,511 do
    local s = math.sin(a*BANG2RAD)
    isintab[a] = 16384*s
    dsintab[a] = s
end

isintab[512] = 16384
dsintab[512] = 1

for i=513,1023 do
    isintab[i] = isintab[1024-i];
    dsintab[i] = dsintab[1024-i];
end

for i=1024,2047 do
    isintab[i] = -isintab[i-1024];
    dsintab[i] = -dsintab[i-1024];
end


local band = bit.band

local function ksc_common(ang)
    ang = band(ang, 2047)
    assert(ang < 2048+0ULL)  -- might have been passed NaN
    return ang
end

-- k{sin,cos}: 16384-scaled output, 2048-based angle input
function ksin(ang)
    return isintab[ksc_common(ang)]
end

function kcos(ang)
    return isintab[ksc_common(ang+512)]
end


local sin, cos = math.sin, math.cos

-- {sin,cos}b: [-1..1] output, 2048-based angle input
function sinb(ang)
    return dsintab[ksc_common(ang)]
end

function cosb(ang)
    return dsintab[ksc_common(ang+512)]
end


-- Approximations to 2D and 3D Euclidean distances (also see common.c)
local abs = math.abs
local arshift = bit.arshift

local function dist_common(pos1, pos2)
    local x = abs(pos1.x - pos2.x)
    local y = abs(pos1.y - pos2.y)
    if (x < y) then
        x, y = y, x
    end
    return x, y
end

function ldist(pos1, pos2)
    local x, y = dist_common(pos1, pos2)

    local t = y + arshift(y,1)
    return x - arshift(x,5) - arshift(x,7) + arshift(t,2) + arshift(t,6)
end

function dist(pos1, pos2)
    local x, y = dist_common(pos1, pos2)
    local z = abs(arshift(pos1.z - pos2.z, 4))

    if (x < z) then
        x, z = z, x
    end

    local t = y + z
    return x - arshift(x,4) + arshift(t,2) + arshift(t,3)
end
