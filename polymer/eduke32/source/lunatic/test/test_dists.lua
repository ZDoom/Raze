#!/usr/bin/env luajit

local ffi = require "ffi"
local math = require "math"
local os = require "os"

local xmath = require "xmath"

local ldist = xmath.ldist
local sqrt = math.sqrt

local function printf(fmt, ...)
    print(string.format(fmt, ...))
end

local function edist(p1, p2)
    return sqrt(p1.x*p1.x + p2.x*p2.x)
end

-- z dummy is so that there's no error with xmath.rotate()
local vec2 = ffi.typeof("struct { int32_t x, y, z /* dummy */; }")

local numpoints = tonumber(arg[1]) or 1e4
local Nsq = numpoints*numpoints
printf("number of points: %d, testing %d distances", numpoints, Nsq)

local B = 8192

local pts = {}
for i=1,numpoints do
    pts[i] = vec2(math.random(-B, B), math.random(B, -B))
end

-- test edist
local t = os.clock()

local sum = 0
for i=1,numpoints do
    for j=1,numpoints do
        sum = sum+edist(pts[i], pts[j])
    end
end

t = os.clock()-t
printf("edist: %.03fns per call, mean=%.03f", (1e9*t)/Nsq, sum/Nsq)

-- test ldist
t = os.clock()

local sum = 0
for i=1,numpoints do
    for j=1,numpoints do
        sum = sum+ldist(pts[i], pts[j])
    end
end

t = os.clock()-t
printf("ldist: %.03fns per call, mean=%.03f", (1e9*t)/Nsq, sum/Nsq)


-- test rotation
t = os.clock()

-- from control.lua (the CON version of rotatepoint)
local function _rotatepoint(pivotx, pivoty, posx, posy, ang)
    local pos = xmath.ivec3(posx, posy)
    local pivot = xmath.ivec3(pivotx, pivoty)
    pos = xmath.rotate(pos, ang, pivot):toivec3()
    return pos.x, pos.y
end

sum = 0
for i=1,numpoints do
    for j=1,numpoints do
--        local p = xmath.rotate(pts[i], j, pts[j])
--        sum = sum+p.x
        sum = sum + _rotatepoint(pts[j].x, pts[j].y, pts[i].x, pts[i].y, j)
    end
end

t = os.clock()-t

printf("rotate: %.03fns per call", (1e9)/Nsq)

-- Results (helixhorned x86, x86_64)
-- number of points: 10000, testing 100000000 distances
-- edist: 6.300ns per call, mean=6286.597
-- ldist: 17.600ns per call, mean=8692.612
-- rotate: 10.000ns per call  [even with _rotatepoint()!]
