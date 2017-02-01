
local ffi = require("ffi")
local ffiC = ffi.C

local sector = sector
local inside = inside

local math = require("math")
local xmath = require("xmath")
local stat = require("stat")

local function resetseed()
    math.randomseed(834572183572)
end

local function getmapbounds()
    local inf = 1/0
    local min = { x=inf, y=inf }
    local max = { x=-inf, y=-inf }

    for i=0,ffiC.numsectors-1 do
        for w in wallsofsect(i) do
            local wal = wall[w]

            min.x = math.min(wal.x, min.x)
            max.x = math.max(wal.x, max.x)
            min.y = math.min(wal.y, min.y)
            max.y = math.max(wal.y, max.y)
        end
    end

    return min, max
end

local function getpoints(n, min, max)
    local posns, sects = {}, {}

    resetseed()
    for i=1,n do
        local x = math.random(min.x, max.x)
        local y= math.random(min.y, max.y)
        posns[i] = xmath.vec2(x, y)
        sects[i] = math.random(0, ffiC.numsectors-1)
    end

    return posns, sects
end

-- Compare 'inside' implementations
--
-- N: number of calls
function compinside(n)
    if (type(n) ~= "number") then
        error("N must be a number")
    end

    local sti = stat.new()
    local sts = stat.new()

    local min, max = getmapbounds()
    local posns, sects = getpoints(n, min, max)

    local isi, iss = {}, {}

    for i=1,n do
        local t = ffiC.gethiticks()
        isi[i] = inside(posns[i], sects[i])
        t = ffiC.gethiticks()-t
        sti:add(t)

        local t = ffiC.gethiticks()
        iss[i] = sector[sects[i]]:contains(posns[i])
        t = ffiC.gethiticks()-t
        sts:add(t)

--        if (isi[i]~=iss[i]) then
--            print("unequal: "..i.." "..sects[i].." "..posns[i].x.." "..posns[i].y.." ("..tostring(isi[i])..","..tostring(iss[i])..")")
--        end
        assert(isi[i]==iss[i])
    end

    print("====================")
    print("inside(): " .. sti:getstatstr())
    print("contains(): " .. sts:getstatstr())
end
