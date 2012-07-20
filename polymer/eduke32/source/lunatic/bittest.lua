#!/usr/bin/env luajit

local string = require "string"

local getticks

local bit = require("bit")
local print = print

if (string.dump) then
    -- stand-alone
    local os = require "os"
    function getticks()
        return os.clock()*1000
    end
else
    -- embedded
    getticks = gv.gethitickms

    module(...)
end

-- from http://bitop.luajit.org/api.html

local band, bxor = bit.band, bit.bxor
local rshift, rol = bit.rshift, bit.rol

local m = string.dump and arg[1] or 1e7

function sieve()
    local count = 0
    local p = {}

    if (string.dump) then
        -- stand-alone using unchecked int32_t array instead of table:
        -- on x86_64 approx. 100 vs. 160 ms for m = 1e7
        -- (enabling bound checking makes it be around 170 ms)
        local ffi = require "ffi"
        local pp = ffi.new("int32_t [?]", (m+31)/32 + 1)

        p = pp
--[[
        local mt = {
            __index = function(tab,idx)
                if (idx >= 0 and idx <= (m+31)/32) then
                    return pp[idx]
                end
            end,

            __newindex = function(tab,idx,val)
                if (idx >= 0 and idx <= (m+31)/32) then
                    pp[idx] = val
                end
            end,
        }

        p = setmetatable({}, mt)
--]]
    end

    for i=0,(m+31)/32 do p[i] = -1 end

    local t = getticks()

    -- See http://luajit.org/ext_ffi_tutorial.html,
    -- "To Cache or Not to Cache"
    -- bit. - qualified version: with m=1e7 ond x86_64:
    -- 165 ms embedded, 100 ms stand-alone
---[[
    for i=2,m do
        if bit.band(bit.rshift(p[bit.rshift(i, 5)], i), 1) ~= 0 then
            count = count + 1
            for j=i+i,m,i do
                local jx = bit.rshift(j, 5)
                p[jx] = bit.band(p[jx], bit.rol(-2, j))
            end
        end
    end
--]]

    -- local var version: with m=1e7 on x86_64:
    -- 205 ms embedded, 90 ms stand-alone
--[[
    for i=2,m do
        if band(rshift(p[rshift(i, 5)], i), 1) ~= 0 then
            count = count + 1
            for j=i+i,m,i do
                local jx = rshift(j, 5)
                p[jx] = band(p[jx], rol(-2, j))
            end
        end
    end
--]]

    print(string.format("Found %d primes up to %d (%.02f ms)", count, m,
                        getticks()-t))
end

if (string.dump) then
    sieve()
end
