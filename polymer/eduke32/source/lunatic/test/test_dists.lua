
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

local vec2 = ffi.typeof("struct { int32_t x, y; }")

local numpoints = 1e4
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
