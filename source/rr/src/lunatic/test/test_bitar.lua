#!/usr/bin/env luajit

-- Usage: luajit bittest.lua <number or "x"> [-ffi] [-bchk]

local require = require
local string = require "string"
local math = require "math"

local bitar = require "bitar"

local print = print
local tonumber = tonumber

local getticks

if (string.dump) then
    -- stand-alone
    local os = require "os"
    function getticks()
        return os.clock()*1000
    end
else
    -- embedded
    getticks = gv.gethiticks

    module(...)
end

-- based on example from http://bitop.luajit.org/api.html

local m = string.dump and tonumber(arg[1]) or 1e7
local maxidx = math.floor(m/32)

local ffiar_p, boundchk_p = false, false

if (string.dump) then
    if (arg[2]=="-ffi" or arg[3]=="-ffi") then
        ffiar_p = true
    end

    if (arg[2]=="-bchk" or arg[3]=="-bchk") then
        boundchk_p = true
    end
end

function sieve()
    local count = 0
    local p = {}

    if (ffiar_p) then
        -- stand-alone using unchecked int32_t array: on x86_64 approx. 80 ms
        -- for m = 1e7 (enabling bound checking makes it be around 100 ms)
        local ffi = require "ffi"
        local pp = ffi.new("int32_t [?]", maxidx + 1)

        p = pp

        if (boundchk_p) then
            local mt = {
                __index = function(tab,idx)
                    if (idx >= 0 and idx <= maxidx) then
                        return pp[idx]
                    end
                end,

                __newindex = function(tab,idx,val)
                    if (idx >= 0 and idx <= maxidx) then
                        pp[idx] = val
                    end
                end,
            }

            p = setmetatable({}, mt)
        end

        for i=0,maxidx do p[i] = -1; end
    else
        p = bitar.new(m, 1)
    end

    local t = getticks()

    if (ffiar_p) then
        local bit = require "bit"

        for i=2,m do
            if (bit.band(p[bit.rshift(i, 5)], bit.lshift(1, i)) ~= 0) then
                count = count + 1
                for j=i+i,m,i do
                    local jx = bit.rshift(j, 5)
                    p[jx] = bit.band(p[jx], bit.rol(0xfffffffe, j));
                end
            end
        end
    else
        for i=2,m do
            if (p:isset(i)) then
                count = count + 1
                for j=i+i,m,i do p:set0(j); end
            end
        end
    end

    -- When using bitar module: x86_64: approx. 110 ms
    print(string.format("[%s] Found %d primes up to %d (%.02f ms)",
                        ffiar_p and "ffi-ar"..(boundchk_p and ", bchk" or "") or "tab-ar",
                        count, m, getticks()-t))

    return p, count
end

if (string.dump) then
    local function printf(fmt, ...) print(string.format(fmt, ...)) end

    local p, count = sieve()
    local t = getticks()

    if (ffiar_p) then
        return
    end

    -- test serialization
    local ser = tostring(p)
    local maxbidx_str = string.match(ser, '%(([0-9]+),')
    local p2 = bitar.new(tonumber(maxbidx_str), string.match(ser, "'(.*)'"))
    printf("serialization + new: %.02f ms", tostring(getticks()-t))

    assert(p==p2)
    if (m >= 2) then
        assert(#p == count+2)  -- +2 is because 0 and 1 are set even though they're not primes
    end

    if (not ffiar_p) then
        math.randomseed(os.time())
        local maxbidx = math.random(0, 65536)
        local p3 = bitar.new(maxbidx, 1)
        assert(#p3 == maxbidx+1)  -- bits 0 to maxbidx inclusive are set
    end
--[[
    print(p)
    print(p-p)  -- test set difference
    print(-p)
--]]

    -- Set difference of self with self is the same as set intersection of self
    -- with complement of self:
    assert(p-p == p*(-p))
end
