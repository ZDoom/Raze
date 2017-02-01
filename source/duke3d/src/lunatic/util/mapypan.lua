
-- Display information about problematic wall ypannings,
-- foreachmap module.

local string = require "string"
local table = require "table"
local bit = require "bit"

local print = print
local pairs = pairs
local rawget = rawget
local setmetatable = setmetatable

local B = require "build"

module(...)


local function printf(fmt, ...)
    print(string.format(fmt, ...))
end


local tile

function init(arg)
    local artargend = nil
    for i=2,#arg do
        if (arg[i]=="--") then
            artargend = i
            break
        end
    end

    if (artargend==nil or artargend==0) then
        printf("Usage: luajit ./foreachmap.lua %s <tilesXXX.ART> [, ...] -- <filename1.map> ...\n", arg[1])
        return -1
    end

    local artfns = {}
    for i=2,artargend-1 do
        artfns[#artfns+1] = arg[i]
    end

    local i = 2
    local j = artargend+1

    while (arg[j]) do
        arg[i] = arg[j]
        arg[j] = nil

        i = i+1
        j = j+1
    end

    local tile_, errmsg = B.loadarts(artfns)
    if (tile_ == nil) then
        printf("%s", errmsg)
        return -2
    end

    tile = tile_  -- set 'tile' file-scope var
end

local table_default0_mt = {
    __index = function(tab, idx)
        if (rawget(tab, idx)==nil) then
            return 0
        end
        return rawget(tab, idx)
    end
}


local sector, wall, sprite

function success(map, fn)
    -- set file-scope vars for convenience
    sector = map.sector
    wall = map.wall
    sprite = map.sprite

    -- counts of non-pow2 tiles, per tilenum
    local np2tile = setmetatable({}, table_default0_mt)
    -- walls/overwall indices with non-pow2 tiles, [walidx]=true
    local np2wall = {}
    -- [i]=wallidx
    local np2walls = {}

    local badoverpicnum = false

    for i=0,map.numsectors-1 do
        local startwall = sector[i].wallptr
        local endwall = startwall+sector[i].wallnum-1

        for w=startwall,endwall do
            for n=1,2 do
                local pic, ysiz

                if (wall[w].nextwall < 0) then
                    -- We don't care for white walls
                elseif (n==1) then
                    pic = wall[w].picnum
                    ysiz = tile.sizy[pic]
                else
                    pic = wall[w].overpicnum
                    if (pic < 0 or pic > 30720) then  -- MAXTILES
                        badoverpicnum = true
                    else
                        if (bit.band(wall[w].cstat, 16+32)==0) then
                            -- we don't care about non-masked/1-way walls
                            ysiz = nil
                        else
                            ysiz = tile.sizy[pic]
                        end
                    end
                end

                if (pic == 560) then  -- Don't care for MIRROR
                    ysiz = nil
                end

                if (ysiz~=nil and ysiz > 0 and bit.band(ysiz, bit.bnot(ysiz-1))~=ysiz) then
                    -- non-pow2 ysize

                    np2tile[pic] = np2tile[pic]+1

                    if (not np2wall[w]) then
                        np2wall[w] = true
                        np2walls[#np2walls+1] = w
                    end
                end
            end
        end
    end

    -- report our findings

    if (#np2walls == 0) then
        return
    end

    -- sort in wall index order
    table.sort(np2walls)

    printf("--- %s:", fn)

--[[
    printf(" Walls:")
    for i=1,#np2walls do
        printf("  %d", np2walls[i])
    end
    printf("")
--]]
    printf(" %d red walls with non-pow2 ysize tiles", #np2walls)
    if (badoverpicnum) then
        printf(" (some red walls have out-of-bounds overpicnums)")
    end

    local np2tiles = {}
    for tilenum,_ in pairs(np2tile) do
        np2tiles[#np2tiles+1] = tilenum
    end
    table.sort(np2tiles)

    printf(" Tiles:")
    for i=1,#np2tiles do
        printf("  %d", np2tiles[i])
    end

    printf("")
end
