
-- Search for maps that have parallaxed ceilings with different tile numbers.
-- foreachmap module.

local bit = require "bit"
local string = require "string"
local table = require "table"
local io = require "io"

local print = print
local tostring = tostring
local type = type

local function printf(fmt, ...)
    print(string.format(fmt, ...))
end


module(...)


local MULTI_PSKY_TILES = { 80, 84, 89 }

local IS_MULTI_PSKY_TILE = {
    [80] = true,  -- MOONSKY1
    [84] = true,  -- BIGORBIT1
    [89] = true,  -- LA
}

local tilecnt

-- foreachmap initialization
local opts = { p=false, s=false }  -- pretty? simple?

function init(args)
    if (#args == 1) then
        local wr = function(s) io.stderr:write(s) end
        wr("Usage: "..args[0].." "..args[1].." [-s] [-p] *.map ...\n")
        wr("  -s: short format\n")
        wr("  -p: pretty-print using ANSI escapes\n\n")
        wr("The long format is as follows:\n")
        wr("  <mapname.map>: <num-distinct-psky-tiles> (<of-those-multi-pskies>): <item1> <item2> ...\n")
        wr("Each <item> reads\n")
        wr("  #<tilenum>:<num-occurrences>(s<sectnum>)\n")
        wr("where <sectnum> is the index of an arbitrary sector containing such a psky ceiling.\n")
        return
    end

    for i=2,#args do
        local arg = args[i]

        if (arg:sub(1,1)=="-") then
            local letter = arg:sub(2,2)

            if (#arg==2 and type(opts[letter])=="boolean") then
                opts[letter] = true
            else
                io.stderr:write("Unrecognized option "..arg.."\n")
            end
        else
            return i
        end
    end
end

-- Sorting function for tiles, true if <tile1> "less-than" <tile2>.
-- "less-than" here means appearing earlier in the resulting table.
local function sortskytiles(tile1, tile2)
    local ismulti1 = IS_MULTI_PSKY_TILE[tile1]
    local ismulti2 = IS_MULTI_PSKY_TILE[tile2]

    -- First, check if <tile1> is a multi-psky tile and <tile2> not.
    if (ismulti1 and not ismulti2) then
        return true
    end

    -- Next, check if <tile1> appears more often in the map.
    if (tilecnt[tile1] > tilecnt[tile2]) then
        return true
    end

    -- Now, (tilecnt[tile1] >= tilecnt[tile2]) and (not ismulti1 or ismulti2).
    return false
end

local function bold(s)
    return opts.p and ("\x1b[1m"..s.."\x1b[0m") or tostring(s)
end

function success(map, fn)
    -- [<tilenum>] = <count of psky ceilings with that tile number>
    tilecnt = {}

    -- [<idx>] = <tilenum>
    local tiles = {}
    -- [<tilenum>] = <arbitrary sectnum with that psky ceiling tile>
    local tilesect = {}

    for i=0,map.numsectors-1 do
        local sec = map.sector[i]

        if (bit.band(sec.ceilingstat, 1) ~= 0) then
            local tile = sec.ceilingpicnum

            if (tilecnt[tile] == nil) then
                tiles[#tiles+1] = tile
            end

            tilecnt[tile] = 1 + (tilecnt[tile] or 0)
            tilesect[tile] = i
        end
    end

    if (#tiles > 0) then
        -- Do an in-place sort.
        table.sort(tiles, sortskytiles)

        local strbuf = {}

        for i=1,#tiles do
            local tile = tiles[i]
            local ismulti = IS_MULTI_PSKY_TILE[tile]

            if (i == 4) then
                strbuf[#strbuf+1] = "..."
                break
            end

            strbuf[#strbuf+1] = (ismulti and "*" or "#")..tile..
                ":"..tilecnt[tile]..
                (opts.s and "" or ("(s"..tilesect[tile]..")"))
        end

        local nummultiskies = 0
        for i=1,#MULTI_PSKY_TILES do
            if (tilecnt[MULTI_PSKY_TILES[i]]) then
                nummultiskies = nummultiskies+1
            end
        end

        -- DCPT: distinct ceiling-psky tiles
        printf("%s: %s%sDCPT: %s", fn, bold(#tiles),
               opts.s and " " or bold(" ("..nummultiskies..") "),
               table.concat(strbuf," "))
    end
end
