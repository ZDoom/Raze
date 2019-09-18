#!/usr/bin/env luajit

local ffi = require "ffi"
local io = require "io"
local os = require "os"
local math = require "math"

local tablesfn = arg[1]
local taylor_n = tonumber(arg[2])

if (tablesfn==nil) then
    -- taylor_n is a number: use that n for Taylor approximation of atan
    -- taylor_n==nil: use math.atan
    print("Usage: ./tables.lua ../path/to/TABLES.DAT [talyor_n]")
    return 1
end

local fd, errmsg = io.open(tablesfn)
if (fd==nil) then
    print("Failed opening "..tablesfn..": "..errmsg)
    return 1  -- XXX: not working, neither os.exit(1) as of 20120901
end

local sintab = ffi.new("int16_t [?]", 2048)
local sinstr = fd:read(2048*2)
if (sinstr==nil or #sinstr<2048*2) then
    print("Failed reading whole sine table")
    fd:close()
    return 1
end

ffi.copy(sintab, sinstr, 2048*2)
sinstr = nil

--== sintable

local oursin = ffi.new("int16_t [?]", 2048)
local BANG2RAD = math.pi/1024
for a=0,2047 do
    local s = 16384*math.sin(a*BANG2RAD)
    oursin[a] = s

--    if (a < 1024) then print(a..": "..s) end
end
--print("")

for a=0,2047 do
    local dif = oursin[a]-sintab[a]
    assert(dif == 0)
--[[
    if (dif ~= 0) then
        print(a..": "..dif)
    end
--]]
    if (a < 512) then
        assert(sintab[a] == sintab[1024-a])
    end

    if (a >= 1024) then
        assert(sintab[a] == -sintab[a-1024])
    end
end

--== radarang

local ratab = ffi.new("int16_t [?]", 640)
local rastr = fd:read(640*2)
fd:close()

ffi.copy(ratab, rastr, 640*2)
rastr = nil

local function atan_taylor(x, n)
    local absx = math.abs(x)

    local at = 0
    for i=0,n do
        local twoip2 = 2*i+1
        local xp = x^twoip2
        local pmone = (-1)^i

        if (absx < 1) then
            at = at + (pmone * xp)/twoip2
        else
            at = at + pmone/(xp * twoip2)
        end
    end

    if (absx > 1) then
        at = math.pi/2 - at
    end

    return at
end

local atan
if (taylor_n==nil) then
    atan = math.atan
else
    atan = function (x) return atan_taylor(x, taylor_n) end
end


for i=0,639 do
    local at = -64*atan((640-0.5-i)/160)/BANG2RAD
    local ra = ratab[i]

    at = math.ceil(at)

--    print(i ..": ".. ra .." ".. at .." ".. at/ra .." ".. at-ra)
end

--[[
local ourra = ffi.new("int16_t [?]", 640)
--]]
