
-- Loaders for various BUILD structures for LuaJIT


local ffi = require "ffi"
local io = require "io"
local bit = require "bit"
local string = require "string"
local table = require "table"

local error = error
local assert = assert
local print = print
local setmetatable = setmetatable
local tostring = tostring
local tonumber = tonumber

module(...)

ffi.cdef[[
#pragma pack(push,1)
typedef struct
{
    int16_t wallptr, wallnum;
    int32_t ceilingz, floorz;
    uint16_t ceilingstat, floorstat;
    int16_t ceilingpicnum, ceilingheinum;
    int8_t ceilingshade;
    uint8_t ceilingpal, ceilingxpanning, ceilingypanning;
    int16_t floorpicnum, floorheinum;
    int8_t floorshade;
    uint8_t floorpal, floorxpanning, floorypanning;
    uint8_t visibility, filler;
    int16_t lotag, hitag, extra;
} sectortype;

typedef struct
{
    int32_t x, y;
    int16_t point2, nextwall, nextsector;
    uint16_t cstat;
    int16_t picnum, overpicnum;
    int8_t shade;
    uint8_t pal, xrepeat, yrepeat, xpanning, ypanning;
    int16_t lotag, hitag, extra;
} walltype;

typedef struct
{
    int32_t x, y, z;
    uint16_t cstat;
    int16_t picnum;
    int8_t shade;
    uint8_t pal, clipdist, filler;
    uint8_t xrepeat, yrepeat;
    int8_t xoffset, yoffset;
    int16_t sectnum, statnum;
    int16_t ang, owner, xvel, yvel, zvel;
    int16_t lotag, hitag, extra;
} spritetype;
#pragma pack(pop)
]]

local C = ffi.C


MAX =
{
    SECTORS = { [7]=1024, [8]=4096, [9]=4096 },
    WALLS = { [7]=8192, [8]=16384, [9]=16384 },
    SPRITES = { [7]=4096, [8]=16384, [9]=16384 },

    TILES = 30720,
}

local function doread(fh, basectype, numelts)
    local cd = ffi.new(basectype.."[?]", numelts)
    local size = ffi.sizeof(cd)

    if (numelts==0) then
        return nil
    end

    assert(size % numelts == 0)
    local datstr = fh:read(size)

    if (datstr == nil or #datstr < size) then
        fh:close()
        return nil
    end

    ffi.copy(cd, datstr, size)

    return cd
end

local function set_secwalspr_mt(structar, maxidx)
    local mt = {
        __index = function(tab, idx)
            if (idx < 0 or idx >= maxidx) then
                error("Invalid structure array read access", 2)
            end
            return structar[idx]
        end,

        __newindex = function(tab, idx, newval)
            error('cannot write directly to structure array', 2)
        end,
    }

    return setmetatable({}, mt)
end


local function get_numyaxbunches(map)
    if (map.version < 9) then
        return 0
    end

    local numbunches = 0
    local sectsperbunch = { [0]={}, [1]={} }

    for i=0,map.numsectors-1 do
        for cf=0,1 do
            local sec = map.sector[i]
            local stat = (cf==0) and sec.ceilingstat or sec.floorstat
            local xpan = (cf==0) and sec.ceilingxpanning or sec.floorxpanning

            if (bit.band(stat, 1024) ~= 0) then
                if (xpan+1 > numbunches) then
                    numbunches = xpan+1
                end

                if (sectsperbunch[cf][xpan]==nil) then
                    sectsperbunch[cf][xpan] = 1
                else
                    sectsperbunch[cf][xpan] = sectsperbunch[cf][xpan]+1
                end
            end
        end
    end

    map.numbunches = numbunches
    map.sectsperbunch = sectsperbunch
end

--== sprite canonicalizer ==--
local function sprite2str(s)
    local FMT = "%+11d_"
    -- NOTE: this canonicalization isn't very useful except for debugging
    -- copy-paste in the editor.
    -- tostring(s): make sort stable
    return string.format(FMT:rep(4).."%s", s.x, s.y, s.z, s.ang, tostring(s))
end

local function canonicalize_sprite_order(map)
    local numsprites = map.numsprites

    map.spriten2o = {}  -- mapping of new to old sprite index

    if (numsprites == 0) then
        return
    end

    local spriteidx = {}

    for i=0,numsprites-1 do  -- 0->1 based indexing
        spriteidx[i+1] = i
    end

    table.sort(spriteidx,
               function(i1, i2)
                   return sprite2str(map.sprite[i1]) < sprite2str(map.sprite[i2])
               end)

    -- deep-copied sprite structs
    local spritedup = {}

    for i=0,numsprites-1 do
        -- save sorting permutation (0-based -> 0-based)
        map.spriten2o[i] = assert(spriteidx[i+1])

        -- back up struct
        spritedup[i] = ffi.new("spritetype")
        ffi.copy(spritedup[i], map.sprite[i], ffi.sizeof("spritetype"))
    end

    for i=0,numsprites-1 do  -- do the actual rearrangement
        map.sprite[i] = spritedup[spriteidx[i+1]]
    end
end

--== LOADBOARD ==--
-- returns:
--  on failure, nil, errmsg
--  on success, a table
--    {
--      version = <num>,
--      numsectors=<num>, numwalls=<num>, numsprites=<num>,
--      sector=<cdata (array of sectortype)>,
--      wall=<cdata (array of walltype)>,
--      sprite=nil or <cdata> (array of spritetype),
--      start =
--        { x=<num>, y=<num>, z=<num>, ang=<num>, sectnum=<num> },
--      numbunches = <num>,
--      sectsperbunch = {
--          [0] = { [<bunchnum>]=<number of ceilings> },
--          [1] = { [<bunchnum>]=<number of floors> }
--      }
--    }
function loadboard(filename, do_canonicalize_sprite)
    local fh, errmsg = io.open(filename)

    if (fh==nil) then
        return nil, errmsg
    end

    local cd = doread(fh, "int32_t", 4)
    if (cd==nil) then
        return nil, "Couldn't read header"
    end

    -- The table we'll return on success
    local map = {
        version = cd[0],
        start = { x=cd[1], y=cd[2], z=cd[3] },
    }

    if (map.version < 7 or map.version > 9) then
        fh:close()
        return nil, "Invalid map version"
    end

    cd = doread(fh, "int16_t", 3)
    if (cd==nil) then
        return nil, "Couldn't read header (2)"
    end

    map.start.ang = cd[0]
    map.start.sectnum = cd[1]

    -- sectors
    map.numsectors = cd[2]
    if (map.numsectors == nil) then
        return nil, "Couldn't read number of sectors"
    end
    if (map.numsectors <= 0 or map.numsectors > MAX.SECTORS[map.version]) then
        fh:close()
        return nil, "Invalid number of sectors"
    end

    map.sector = doread(fh, "sectortype", map.numsectors)
    if (map.sector == nil) then
        return nil, "Couldn't read sectors"
    end

    -- walls
    cd = doread(fh, "int16_t", 1)
    if (cd == nil) then
        return nil, "Couldn't read number of walls"
    end
    map.numwalls = cd[0]
    if (map.numwalls <= 0 or map.numwalls > MAX.WALLS[map.version]) then
        fh:close()
        return nil, "Invalid number of walls"
    end

    map.wall = doread(fh, "walltype", map.numwalls)
    if (map.wall == nil) then
        return nil, "Couldn't read walls"
    end

    -- sprites
    cd = doread(fh, "int16_t", 1)
    if (cd == nil) then
        return nil, "Couldn't read number of sprites"
    end
    map.numsprites = cd[0]
    if (map.numsprites < 0 or map.numsprites > MAX.SPRITES[map.version]) then
        fh:close()
        return nil, "Invalid number of sprites"
    end

    map.sprite = doread(fh, "spritetype", map.numsprites)
    if (map.numsprites~=0 and map.sprite == nil) then
        return nil, "Couldn't read sprites"
    end
    fh:close()

    if (do_canonicalize_sprite) then
        -- must do this before setting metatable
        canonicalize_sprite_order(map)
    end

    map.sector = set_secwalspr_mt(map.sector, map.numsectors)
    map.wall = set_secwalspr_mt(map.wall, map.numwalls)
    map.sprite = set_secwalspr_mt(map.sprite, map.numsprites)

    get_numyaxbunches(map)

    -- done
    return map
end


local function set_sizarray_mt(sizar)
    local mt = {
        __index = function(tab, idx)
            if (idx < 0 or idx >= MAX.TILES) then
                error("Invalid tile size array read access", 2)
            end
            return sizar[idx]
        end,

        __newindex = function(tab, idx, newval)
            if (idx < 0 or idx >= MAX.TILES) then
                error("Invalid tile size array write access", 2)
            end
            sizar[idx] = newval
        end,
    }

    return setmetatable({}, mt)
end


--== LOADARTS (currently tilesizx and tilesizy only) ==--
-- filenames: a table with ART file names
-- returns:
--  on failure, nil, errmsg
--  on success, a table
--    {
--      sizx = <cdata (array of length MAXTILES)>
--      sizy = <cdata (array of length MAXTILES)>
--    }
function loadarts(filenames)
    local tile = {
        sizx = ffi.new("int16_t [?]", MAX.TILES),
        sizy = ffi.new("int16_t [?]", MAX.TILES),
    }

    for fni=1,#filenames do
        local fn = filenames[fni]
        local fh, errmsg = io.open(fn)

        if (fh==nil) then
            return nil, errmsg
        end

        local cd = doread(fh, "int32_t", 4)
        -- artversion, numtiles, localtilestart, localtileend
        if (cd==nil) then
            fh:close()
            return nil, fn..": Couldn't read header"
        end

        local localtilestart = cd[2]
        local numtileshere = cd[3]-localtilestart+1
--        print(fn.. ": "..cd[2].. ", "..cd[3])

        if (numtileshere < 0 or localtilestart+numtileshere >= MAX.TILES) then
            fh:close()
            return nil, fn..": Invalid tile start/end"
        end

        if (numtileshere==0) then
            fh:close()
        else
            -- X sizes
            cd = doread(fh, "int16_t", numtileshere)
            if (cd == nil) then
                fh:close()
                return nil, fn..": Couldn't read tilesizx array"
            end

            ffi.copy(tile.sizx+localtilestart, cd, numtileshere*2)

            -- Y sizes
            cd = doread(fh, "int16_t", numtileshere)
            if (cd == nil) then
                fh:close()
                return nil, fn..": Couldn't read tilesizy array"
            end

            ffi.copy(tile.sizy+localtilestart, cd, numtileshere*2)
        end
    end

    tile.sizx = set_sizarray_mt(tile.sizx)
    tile.sizy = set_sizarray_mt(tile.sizy)

    return tile
end

function readdefs(fn)
    local fh, errmsg = io.open(fn)

    if (fh==nil) then
        return nil, errmsg
    end

    local defs = {}

    for line in fh:lines() do
        local defname, numstr = string.match(line, "#?%s*define%s+([%a_][%w_]+)%s+([0-9]+)")
        if (defname) then
            defs[defname] = tonumber(numstr)
        end
    end

    fh:close()
    return defs
end
