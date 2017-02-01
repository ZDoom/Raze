#!/usr/bin/env luajit

local os = require("os")

local xmath = require("xmath")


-- XXX: perf regression? (See below PERF_REGRESSION)
-- No, happens only with Clang build. (Why?)
local N = os.exit and (arg[1] and tostring(arg[1])) or 1e5  --1e6

local A,B = {}, {}
local V,W = {}, {}

local randvec

local args = { ... }
local ourname = args[1]

if (os.exit) then
    local math = require("math")

    randvec = function()
        return xmath.vec2(math.random(), math.random())
    end

    print("Running stand-alone. ourname: "..tostring(ourname))
else
    local randgen = require("randgen")
    local s = randgen.new(true)

    -- NOTE: factoring out the inner s:getdbl() into a separate function
    -- reduces performance seriously (about an order of magnitude!)
    randvec = function()
        return xmath.vec2(s:getdbl(), s:getdbl())
    end

    -- Test optional arguments from our_require().
    printf("Running %s embedded with opt arg %s", ourname, tostring(args[2]))
end

local t1 = os.clock()

if (os.exit == nil) then
    local randgen = require("randgen")
    local r = randgen.new(true)

    for i=1,4*2*N do
        -- This is to test the performance compared to a direct
        -- ffiC.rand_jkiss_dbl() call in randgen.lua
        r:getdbl()
    end
end

local t2 = os.clock()

-- init random points and vectors
for i=1,N do
    A[i] = randvec()
    B[i] = randvec()
    V[i] = randvec()
    W[i] = randvec()
end

local t3 = os.clock()

local v = xmath.vec2(0, 0)
for i=1,N do
    local intersp = xmath.intersect(A[i],V[i], B[i],W[i], true)
    if (intersp ~= nil) then
        v = v + intersp
    end
end

local t4 = os.clock()

-- x86_64 (embedded): approx. 200 ms (vs. the 100 ms of direct
-- ffiC.rand_jkiss_dbl()):
-- x86: 170 ms
print("getdbl:    ".. 1000*(t2-t1))
print("genpoints: ".. 1000*(t3-t2))  -- x86_64: 500 ms, x86: 700 ms
print("intersect: ".. 1000*(t4-t3))  -- x86_64, x86: about 35 ms  <- thanks to allocation sinking (else, about 500 ms?)
print("result:    ".. tostring(v))

-- PERF_REGRESSION: with N==1e6 getdbl, genpoints now about 1000ms from EDuke32!
