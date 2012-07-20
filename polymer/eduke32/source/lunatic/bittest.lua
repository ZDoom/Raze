
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

local m = 1000000

function sieve()
    local count = 0
    local p = {}

    for i=0,(m+31)/32 do p[i] = -1 end

    local t = getticks()

    for i=2,m do
        if band(rshift(p[rshift(i, 5)], i), 1) ~= 0 then
            count = count + 1
            for j=i+i,m,i do
                local jx = rshift(j, 5)
                p[jx] = band(p[jx], rol(-2, j))
            end
        end
    end

    print(string.format("Found %d primes up to %d (%.02f ms)", count, m,
                        getticks()-t))
end

if (string.dump) then
    sieve()
end
