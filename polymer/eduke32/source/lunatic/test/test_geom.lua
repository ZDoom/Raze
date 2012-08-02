#!/usr/bin/env luajit

local math = require("math")
local os = require("os")

local geom = require("geom")


local N = os.exit and tostring(arg[1]) or 1e6

local A,B = {}, {}
local V,W = {}, {}

local function randvec()
    return geom.vec2(math.random(), math.random())
end

local t1 = os.clock()

-- init random points and vectors
for i=1,N do
    A[i] = randvec()
    B[i] = randvec()
    V[i] = randvec()
    W[i] = randvec()
end

local t2 = os.clock()

local v = geom.vec2(0, 0)
for i=1,N do
    v = v + geom.intersect(A[i],V[i], B[i],W[i], true)
end

local t3 = os.clock()

print(1000*(t2-t1))
print(1000*(t3-t2))
print(v)

return {}  -- appease Lunatic's require
