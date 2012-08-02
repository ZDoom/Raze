
-- "Bit array" module based on LuaJIT's BitOp.

local ffi = require "ffi"
local bit = require "bit"

local error = error
local assert = assert
local type = type

local setmetatable=setmetatable


module(...)


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

local ops = { isset=isset, set0=set0, set1=set1 }
local mt
mt = {
    __index=ops,

    -- set ops disguised as arithmetic ones...

    __mul = function(ar1, ar2)  -- set intersection
        assert(#ar1 == #ar2)
        local p = {}
        for i=0,#ar1 do
            p[i] = bit.band(ar1[i], ar2[i])
        end
        return setmetatable(p, mt)
    end,

    __add = function(ar1, ar2)  -- set union
        assert(#ar1 == #ar2)
        local p = {}
        for i=0,#ar1 do
            p[i] = bit.bor(ar1[i], ar2[i])
        end
        return setmetatable(p, mt)
    end,

    __sub = function(ar1, ar2)  -- set difference
        assert(#ar1 == #ar2)
        local p = {}
        for i=0,#ar1 do
            p[i] = bit.band(ar1[i], bit.bnot(ar2[i]))
        end
        return setmetatable(p, mt)
    end,

    -- serialization
    __tostring = function(ar)
        local maxidx=#ar
        local size=maxidx+1

        local hdr = "bitar.new('"
        local ofs = #hdr
        local totalstrlen = ofs+8*size+2
        local str = ffi.new("char [?]", totalstrlen)

        ffi.copy(str, hdr, ofs)

        for i=0,maxidx do
            -- 'a' is ASCII 97
            for nib=0,7 do
                str[ofs + 8*i + nib] = 97 + bit.band(bit.rshift(ar[i], 4*nib), 0x0000000f)
            end
        end

        ffi.copy(str+totalstrlen-2, "')", 2)

        return ffi.string(str, totalstrlen)
    end,
}

-- Create new bit array.
-- Returns a table p in which entries p[0] through p[floor((maxbidx+31)/32)]
-- are set to an initialization value: 0 if 0 has been passed, -1 if 1
-- has been passed.
-- Storage: 4 bits/bit + O(1)? (per 32 bits: 64 bits key, 64 bits value)
function new(maxbidx, initval)
    local p = {}

    if (type(maxbidx)=="string") then
        -- string containing hex digits (a..p) given, internal
        local lstr = maxbidx

        local numnibs = #lstr
        assert(numnibs%8 == 0)

        local size = numnibs/8
        local maxidx = size-1

        local str = ffi.new("char [?]", numnibs)
        ffi.copy(str, lstr, numnibs)

        for i=0,maxidx do
            p[i] = 0

            for nib=0,7 do
                local hexdig = str[8*i + nib]
                assert(hexdig >= 97 and hexdig < 97+16)
                p[i] = bit.bor(p[i], bit.lshift(hexdig-97, 4*nib))
            end
        end
    else
        if (type(maxbidx) ~= "number" or not (maxbidx >= 0)) then
            error("bad argument #1 to newarray (must be a nonnegative number)", 2)
        end

        if (initval ~= 0 and initval ~= 1) then
            error("bad argument #2 to newarray (must be either 0 or 1)", 2)
        end

        for i=0,maxbidx/32 do
            p[i] = -initval
        end
    end

    return setmetatable(p, mt)
end
