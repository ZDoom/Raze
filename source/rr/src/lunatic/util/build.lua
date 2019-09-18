
-- Loaders for various BUILD structures for LuaJIT


local ffi = require "ffi"
local io = require "io"
local bit = require "bit"
local string = require "string"
local table = require "table"

local error = error
local assert = assert
local pairs = pairs
local print = print
local setmetatable = setmetatable
local tostring = tostring
local tonumber = tonumber

module(...)

local STRUCTDEF = {
    sector = [[
    int16_t wallptr, wallnum;
    int32_t ceilingz, floorz;
    uint16_t ceilingstat, floorstat;
    int16_t ceilingpicnum, ceilingheinum;
    int8_t ceilingshade;
    uint8_t ceilingpal, ceilingxpanning, ceilingypanning;
    int16_t floorpicnum, floorheinum;
    int8_t floorshade;
    uint8_t floorpal, floorxpanning, floorypanning;
    uint8_t visibility, fogpal;
    int16_t lotag, hitag, extra;
]],

    wall = [[
    int32_t x, y;
    int16_t point2, nextwall, nextsector;
    uint16_t cstat;
    int16_t picnum, overpicnum;
    int8_t shade;
    uint8_t pal, xrepeat, yrepeat, xpanning, ypanning;
    int16_t lotag, hitag, extra;
]],

    sprite = [[
    int32_t x, y, z;
    uint16_t cstat;
    int16_t picnum;
    int8_t shade;
    uint8_t pal, clipdist, blend;
    uint8_t xrepeat, yrepeat;
    int8_t xoffset, yoffset;
    int16_t sectnum, statnum;
    int16_t ang, owner, xvel, yvel, zvel;
    int16_t lotag, hitag, extra;
]],
}

ffi.cdef([[
typedef struct
{
]]..STRUCTDEF.sector..[[
} sectortype;

typedef struct
{
]]..STRUCTDEF.wall..[[
} walltype;

typedef struct
{
]]..STRUCTDEF.sprite..[[
} spritetype;
]])

ffi.cdef[[
size_t fread(void *ptr, size_t size, size_t nmemb, void *stream);
]]

local C = ffi.C

-- [<sector/wall/sprite>].<membername> = true
local is_member_tab = {
    sector = {},
    wall = {},
    sprite = {},
}

do
    for what, sdef in pairs(STRUCTDEF) do
        for membname in string.gmatch(sdef, "([a-z0-9_]+)[,;]") do
            is_member_tab[what][membname] = true
        end
    end
end

function ismember(what, membname)
    return (is_member_tab[what][membname] ~= nil)
end

MAX =
{
    SECTORS = { [7]=1024, [8]=4096, [9]=4096 },
    WALLS = { [7]=8192, [8]=16384, [9]=16384 },
    SPRITES = { [7]=4096, [8]=16384, [9]=16384 },

    TILES = 30720,
}
local MAX = MAX


-- <dontclose>: if true, don't close file on error
local function doread(fh, basectype, numelts, dontclose)
    assert(numelts > 0)
    local typ = ffi.typeof("$ [?]", ffi.typeof(basectype))
    local cd = ffi.new(typ, numelts)

    if (C.fread(cd, ffi.sizeof(basectype), numelts, fh) ~= numelts) then
        if (not dontclose) then
            fh:close()
        end
        return nil, "Failed reading"
    end

    return cd
end

-- Read base palette (i.e. first 768 bytes as R,G,B triplets) from a PALETTE.DAT.
-- <noquad>: if true, don't multiply components by 4
-- Returns:
--  on success: <uint8_t [768]> cdata (palette values scaled by 4)
--  on failure: nil, <errmsg>
function read_basepal(filename, noquad)
    local fh, errmsg = io.open(filename, "rb")
    if (fh == nil) then
        return nil, errmsg
    end

    local palette, errmsg = doread(fh, "uint8_t", 768, true)
    fh:close()

    local f = noquad and 1 or 4

    for i=0,768-1 do
        palette[i] = f*palette[i]
    end

    return palette, errmsg
end

local function set_secwalspr_mt(structar, maxidx)
    local mt = {
        __index = function(tab, idx)
            if (not (idx >= 0 and idx < maxidx)) then
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
    local fh, errmsg = io.open(filename, "rb")

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

    if (map.numsprites ~= 0) then
        map.sprite = doread(fh, "spritetype", map.numsprites)
        if (map.sprite == nil) then
            return nil, "Couldn't read sprites"
        end
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


local picanm_t = ffi.typeof[[
struct {
    uint8_t num;  // animate number
    int8_t xofs, yofs;
    uint8_t sf;  // anim. speed and flags
}
]]

local artfile_mt = {
    __index = {
        -- Global -> local tile index (-1 if gtile is not in this ART file)
        toltile = function(self, gtile)
            if (self.tbeg <= gtile and gtile <= self.tend) then
                return gtile - self.tbeg
            end
            return -1
        end,

        _check_ltile = function(self, ltile)
            if (not (ltile >= 0 and ltile < self.numtiles)) then
                error("Invalid local tile number "..ltile, 3)
            end
        end,

        _getofs = function(self, ltile)
            return self.tiledataofs + self.offs[ltile]
        end,

        dims = function(self, ltile)
            self:_check_ltile(ltile)
            return self.sizx[ltile], self.sizy[ltile]
        end,

        getpic = function(self, ltile)
            local sx, sy = self:dims(ltile)
            if (sx == 0 or sy == 0) then
                -- Tile nonexistent/empty in this ART file
                return nil, "Empty tile"
            end

            assert(self.fh:seek("set", self:_getofs(ltile)))
            return doread(self.fh, "uint8_t", sx*sy, self.grpfh ~= false)  -- GRPFH_FALSE
        end,
    },

    __metatable = true,
}

-- af, errmsg = artfile(filename [, grpfh, grpofs])
--
-- <filename>: File name of the file to get data from, expect if <grpfh>
--  passed. Always closed on error. Kept open on success.
-- <grpfh>: io.open() file handle to grouping file containing ART
--  uncompressed. Never closed, even on error.
-- <grpofs>: offset of the ART file in file given by <grpfh>
--
-- Returns:
--  * on error: nil, <errmsg>
--  * on success:
function artfile(filename, grpfh, grpofs)
    local ogrpofs
    local fh

    if (grpfh) then
        ogrpofs = grpfh:seek()
        assert(grpfh:seek("set", grpofs))
        fh = grpfh
    else
        local errmsg
        fh, errmsg = io.open(filename, "rb")
        if (fh == nil) then
            return nil, errmsg
        end
    end

    -- Close file on error?
    local dontclose = (grpfh ~= nil)

    -- Maybe close file handle and return error message <msg>
    local function err(msg, ...)
        if (not dontclose) then
            fh:close()
        end
        return nil, string.format(msg, ...)
    end

    local hdr = doread(fh, "int32_t", 4, dontclose)
    -- artversion, numtiles, localtilestart, localtileend
    if (hdr == nil) then
        return err("Couldn't read header")
    end

    local af = {
        filename = filename,
        fh = fh,
        grpfh = grpfh or false,  -- GRPFH_FALSE

        tbeg = hdr[2],
        tend = hdr[3],
        numtiles = hdr[3]-hdr[2]+1,

        -- Members inserted later:
        -- sizx, sizy: picanm: arrays of length af.numtiles
        -- tiledataofs: byte offset in `fh' to beginning of tile data
        -- offs: local byte offsets of each tile, relative of af.tiledataofs
        --
        -- Thus,
        --  af.tiledataofs + af.offs[localtilenum]
        -- is the byte offset of global tile
        --  af.tbeg + localtilenum
        -- in `fh'.
    }

    if (af.numtiles <= 0) then
        return err("Invalid tile start/end or empty ART file")
    end

    local lasttile = af.tbeg + af.numtiles
    if (lasttile >= MAX.TILES) then
        return err("Last tile %d beyond MAXTILES-1 (%d)", lasttile, MAX.TILES-1)
    end

    af.sizx = doread(fh, "int16_t", af.numtiles, dontclose)
    af.sizy = doread(fh, "int16_t", af.numtiles, dontclose)

    if (af.sizx==nil or af.sizy==nil) then
        return err("Couldn't read sizx or sizy arrays")
    end

    af.picanm = doread(fh, picanm_t, af.numtiles, dontclose)

    if (af.picanm == nil) then
        return err("Couldn't read picanm array")
    end

    af.tiledataofs = assert(fh:seek())
    af.offs = ffi.new("uint32_t [?]", af.numtiles)

    local curofs = 0

    for i=0,af.numtiles-1 do
        local sx, sy = af.sizx[i], af.sizy[i]

        if (sx > 0 and sy > 0) then
            af.offs[i] = curofs
            curofs = curofs + sx*sy
        elseif (sx < 0 or sy < 0) then
            -- Let's sanity-check stuff a little
            return err("Local tile %d has negative x or y size", i)
        else
            af.offs[i] = -1
        end
    end

    return setmetatable(af, artfile_mt)
end


local function set_sizarray_mt(sizar)
    local mt = {
        __index = function(tab, idx)
            if (not (idx >= 0 and idx < MAX.TILES)) then
                error("Invalid tile size array read access", 2)
            end
            return sizar[idx]
        end,

        __newindex = function(tab, idx, newval)
            if (not (idx >= 0 and idx < MAX.TILES)) then
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
        local fh, errmsg = io.open(fn, "rb")

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

-- defs [, rdefs] = readdefs(fn [, alsoreverse])
function readdefs(fn, alsoreverse)
    local fh, errmsg = io.open(fn)

    if (fh==nil) then
        return nil, errmsg
    end

    local defs, rdefs = {}, nil

    for line in fh:lines() do
        local defname, numstr = string.match(line, "#?%s*define%s+([%a_][%w_]+)%s+([0-9]+)")
        if (defname) then
            defs[defname] = tonumber(numstr)
        end
    end

    if (alsoreverse) then
        rdefs = {}
        for defname, num in pairs(defs) do
            rdefs[num] = defname
        end
    end

    fh:close()
    return defs, rdefs
end
