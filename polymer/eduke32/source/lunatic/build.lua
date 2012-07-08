
-- Loaders for various BUILD structures for LuaJIT

-- TODO: bound-checking, load ART


local ffi = require "ffi"
local io = require "io"

local assert = assert
local print = print

module(...)

ffi.cdef[[
#pragma pack(push,1)
typedef struct
{
    int16_t wallptr, wallnum;
    int32_t ceilingz, floorz;
    int16_t ceilingstat, floorstat;
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
    int16_t cstat;
    int16_t picnum, overpicnum;
    int8_t shade;
    uint8_t pal, xrepeat, yrepeat, xpanning, ypanning;
    int16_t lotag, hitag, extra;
} walltype;

typedef struct
{
    int32_t x, y, z;
    int16_t cstat, picnum;
    int8_t shade;
    uint8_t pal, clipdist, filler;
    uint8_t xrepeat, yrepeat;
    int8_t xoffset, yoffset;
    int16_t sectnum, statnum;
    int16_t ang, owner, xvel, yvel, zvel;
    int16_t lotag, hitag, extra;
} spritetype;
]]

local C = ffi.C


local MAX =
{
    SECTORS = { [7]=1024, [8]=4096, [9]=4096 },
    WALLS = { [7]=8192, [8]=16384, [9]=16384 },
    SPRITES = { [7]=4096, [8]=16384, [9]=16384 },
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
--        { x=<num>, y=<num>, z=<num>, ang=<num>, sectnum=<num> }
--    }
function loadboard(filename)
    local fh, errmsg = io.open(filename)

    if (fh==nil) then
        return nil, errmsg
    end

    -- The table we'll return on success
    local map = { start={} }

    local cd = doread(fh, "int32_t", 4)
    if (cd==nil) then
        return nil, "Couldn't read header"
    end

    map.version = cd[0]
    map.start.x = cd[1]
    map.start.y = cd[2]
    map.start.z = cd[3]

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
    map.numsprites = cd[0]
    if (map.numsprites < 0 or map.numsprites > MAX.SPRITES[map.version]) then
        fh:close()
        return nil, "Invalid number of sprites"
    end

    map.sprite = doread(fh, "spritetype", map.numsprites)
    if (map.numsprites~=0 and map.sprite == nil) then
        return nil, "Couldn't read sprites"
    end

    -- done
    fh:close()
    return map
end
