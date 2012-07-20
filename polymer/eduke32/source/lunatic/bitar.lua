
-- "Bit array" module based on LuaJIT's BitOp.

local bit = require "bit"

local error = error
local type = type

module(...)

-- Create new bit array.
-- Returns a table p in which entries p[0] through p[floor((size+31)/32)]
-- are set to an initialization value: 0 if 0 has been passed, -1 if 1
-- has been passed.
function new(size, initval)
    if (type(size) ~= "number" or size < 0) then
        error("bad argument #1 to newarray (must be a nonnegative number)", 2)
    end

    if (initval ~= 0 and initval ~= 1) then
        error("bad argument #2 to newarray (must be either 0 or 1)", 2)
    end

    local p = {}
    for i=0,(size+31)/32 do
        p[i] = -initval
    end

    return p
end

-- Is bit i set in bit array ar?
function isset(ar, i)
    return bit.band(ar[bit.rshift(i, 5)], bit.lshift(1, i)) ~= 0
end

-- Set bit j in bit array ar.
function set0(ar, j)
    local jx = bit.rshift(j, 5)
    ar[jx] = bit.band(ar[jx], bit.rol(0xfffffffe, j))
end

-- Clear bit j in bit array ar.
function set1(ar, j)
    local jx = bit.rshift(j, 5)
    ar[jx] = bit.bor(ar[jx], bit.rol(0x00000001, j))
end
