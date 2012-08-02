#!/usr/bin/env luajit

local os = require("os")

local geom = require("geom")


local N = os.exit and (arg[1] and tostring(arg[1])) or 1e6

local A,B = {}, {}
local V,W = {}, {}

local randvec

if (os.exit) then
    local math = require("math")

    randvec = function()
        return geom.vec2(math.random(), math.random())
    end
else
    local randgen = require("randgen")
    local s = randgen.new(true)

    -- NOTE: factoring out the inner s:getdbl() into a separate function
    -- reduces performance seriously (about an order of magnitude!)
    randvec = function()
        return geom.vec2(s:getdbl(), s:getdbl())
    end
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

local v = geom.vec2(0, 0)
for i=1,N do
    v = v + geom.intersect(A[i],V[i], B[i],W[i], true)
end

local t4 = os.clock()

-- x86_64 (embedded): approx. 200 ms (vs. the 100 ms of direct
-- ffiC.rand_jkiss_dbl()):
print(1000*(t2-t1))
print(1000*(t3-t2))  -- x86_64: approx. 500 ms
print(1000*(t4-t3))  -- x86_64: approx. 35 ms
print(v)

return {}  -- appease Lunatic's require
