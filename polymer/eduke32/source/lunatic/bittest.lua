#!/usr/bin/env luajit

-- Usage: luajit bittest.lua <number or "x"> [-ffi] [-bchk]

local string = require "string"

local getticks

local bit = require("bit")
local bitar = require "bitar"

local print = print
local tonumber = tonumber

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

-- based on example from http://bitop.luajit.org/api.html

local isset, set0 = bitar.isset, bitar.set0

local m = string.dump and tonumber(arg[1]) or 1e7

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
        -- stand-alone using unchecked int32_t array instead of table:
        -- on x86_64 approx. 100 vs. 160 ms for m = 1e7
        -- (enabling bound checking makes it be around 170 ms)
        local ffi = require "ffi"
        local pp = ffi.new("int32_t [?]", (m+31)/32 + 1)

        p = pp

        if (boundchk_p) then
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
        end

        for i=0,(m+31)/32 do p[i] = -1; end
    else
        p = bitar.new(m, 1)
    end

    local t = getticks()

    for i=2,m do
        if (isset(p, i)) then
            count = count + 1
            for j=i+i,m,i do set0(p, j); end
        end
    end

    print(string.format("[%s] Found %d primes up to %d (%.02f ms)",
                        ffiar_p and "ffi-ar"..(boundchk_p and ", bchk" or "") or "tab-ar",
                        count, m, getticks()-t))
end

if (string.dump) then
    sieve()
end
