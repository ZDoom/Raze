#!/usr/bin/env luajit

local io = require("io")
local os = require("os")
local string = require("string")
local format = string.format
local table = require("table")

local B = require("build")

local ffi = require("ffi")
local C = ffi.C

ffi.cdef[[
int memcmp(const void *s1, const void *s2, size_t n);
]]

local fn1, fn2 = arg[1], arg[2]
local noOnlyLeft = (arg[3]=="-l")  -- don't show diff lines where left has tile, right hasn't

local function err(str)
    io.stderr:write(str, "\n")
    os.exit(1)
end

local function printf(fmt, ...)
    io.stdout:write(string.format(fmt, ...), "\n")
end

if (not fn1 or not fn2) then
    err("Usage: "..arg[0].." </path/to/tilesXXX.ART> </path/to/tilesYYY.ART> [-l]")
end

local af1, errmsg = B.artfile(fn1)
if (errmsg) then
    err(errmsg)
end

local af2, errmsg = B.artfile(fn2)
if (errmsg) then
    err(errmsg)
end

-- Check for same start/end

if (af1.tbeg ~= af2.tbeg) then
    err("tile starts differ: "..af1.tbeg.." "..af2.tbeg)
end

if (af1.tend ~= af2.tend) then
    err("tile ends differ: "..af1.tend.." "..af2.tend)
end

-- Compare the two ART files

local SIZEOF_PICANM = ffi.sizeof(af1.picanm[0])

for i=0,af1.numtiles-1 do
    local sx1, sx2 = af1.sizx[i], af2.sizx[i]
    local sxdif = (sx1 ~= sx2)

    local sy1, sy2 = af1.sizy[i], af2.sizy[i]
    local sydif = (sy1 ~= sy2)

    local sizedif = (sxdif or sydif)
    local picanmdif = (C.memcmp(af1.picanm[i], af2.picanm[i], SIZEOF_PICANM) ~= 0)

    local datadif = false
    -- compare data
    if (not sizedif) then
        local pic1 = af1:getpic(i)
        local pic2 = af2:getpic(i)

        if (pic1 and pic2 and C.memcmp(pic1, pic2, sx1*sy1) ~= 0) then
            datadif = true
        end
    end

    if (sizedif or picanmdif or datadif) then
        local strbuf = {}

        local s1str = sx1*sy1==0 and "none" or format("(%d,%d)", sx1, sy1)
        local s2str = sx2*sy2==0 and "none" or format("(%d,%d)", sx2, sy2)
        if ((sxdif or sydif) and (not noOnlyLeft or s2str ~= "none")) then
            strbuf[#strbuf+1] = format("size %s %s", s1str, s2str)
        end

        if (picanmdif) then
            strbuf[#strbuf+1] = "picanm"
        end

        if (not sizedif and datadif) then
            strbuf[#strbuf+1] = "data"
        end

        if (#strbuf > 0) then
            io.stdout:write(i..": "..table.concat(strbuf, ", "), "\n")
        end
    end
end
