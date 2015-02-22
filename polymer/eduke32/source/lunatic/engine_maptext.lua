-- MAPTEXT
-- Lunatic: routines for reading and writing map-text.

local ffi = require("ffi")
local ffiC = ffi.C

local assert = assert
local loadstring = loadstring
local pairs = pairs
local pcall = pcall
local print = print
local setfenv = setfenv
local tonumber = tonumber
local type = type

local readintostr = assert(string.readintostr)

local io = require("io")
local string = require("string")


ffi.cdef[[
int32_t (*saveboard_maptext)(const char *filename, const vec3_t *dapos, int16_t daang, int16_t dacursectnum);
int32_t (*loadboard_maptext)(int32_t fil, vec3_t *dapos, int16_t *daang, int16_t *dacursectnum);
]]


--== COMMON ==--

local sector_members = {
    -- Mandatory positional members first, [pos]=<name>.
    "ceilingz", "floorz",
    "ceilingpicnum", "floorpicnum",
    "ceilingshade", "floorshade";

    -- If other positional members are to be added, they must be optional
    -- for backwards compatibility.

    -- Optional key/value members next.
    B = { "ceilingbunch", -1 }, b = { "floorbunch", -1 },  -- default: -1
    F = "ceilingstat", f = "floorstat",  -- default: 0
    H = "ceilingheinum", h = "floorheinum",
    P = "ceilingpal", p = "floorpal",
    X = "ceilingxpanning", x = "floorxpanning",
    Y = "ceilingypanning", y = "floorypanning",

    v = "visibility",
    _ = "fogpal",
    o = "lotag", i = "hitag", e = { "extra", -1 }
}

-- Defines the order in which the members are written out. A space denotes that
-- a newline should appear in the output. KEEPINSYNC with sector_members.
local sector_ord = { mand="12 34 56 ", opt="Bb Ff Hh Pp Xx Yy v _ oie" }

-- KEEPINSYNC with sector_members.
local sector_default = ffi.new("const sectortype", { ceilingbunch=-1, floorbunch=-1, extra=-1 })


local wall_members = {
    -- mandatory
    "point2",  -- special: 0, 1 or 2 in map-text
    "x", "y",
    "nextwall",
    "picnum",
    "shade",
    "xrepeat", "yrepeat",
    "xpanning", "ypanning";

    -- optional
    f = "cstat",
    m = "overpicnum", b = "blend",
    p = "pal",
    w = { "upwall", -1 }, W = { "dnwall", -1 },
    o = "lotag", i = "hitag", e = { "extra", -1 }
}

local wall_ord = { mand="1 23 4 5 6 78 90 ", opt="f mb p wW oie" }
local wall_default = ffi.new("const walltype", { extra = -1, upwall=-1, dnwall=-1 })


local sprite_members = {
    -- mandatory
    "x", "y", "z",
    "ang",
    "sectnum",
    "picnum",
    "cstat",
    "shade",
    "xrepeat", "yrepeat",

    -- optional
    p = "pal",
    c = { "clipdist", 32 },
    b = "blend",
    x = "xoffset", y = "yoffset",
    s = "statnum",
    w = { "owner", -1 },
    X = "xvel", Y = "yvel", Z = "zvel",
    o = "lotag", i = "hitag", e = { "extra", -1 }
}

local sprite_ord = { mand="123 4 5 6 7 8 90 ", opt="p c b xy s w XYZ oie" }
local sprite_default = ffi.new("const spritetype", { clipdist=32, owner=-1, extra=-1 })


--== SAVING ==--

local function write_struct(f, struct, members, ord)
    -- Write mandatory members first.
    local str = ord.mand:gsub(".",
        function(s)
            local num = (s=="0") and 10 or tonumber(s)
            return (s==" ") and "\n" or (struct[members[num]]..",")
        end)
    f:write("{"..str)

    local havesth = false

    -- Write optional members next.
    str = ord.opt:gsub(".",
        function(s)
            if (s==" ") then
                local ohavesth = havesth
                havesth = false
                return ohavesth and "\n" or ""
            end

            local memb = members[s]
            local mname = (type(memb)=="table") and memb[1] or memb
            local mdefault = (type(memb)=="table") and memb[2] or 0

            local val = struct[mname]

            if (val~=mdefault) then
                havesth = true
                return s.."="..val..","
            else
                return ""
            end
        end)

    local neednl = (#str>0 and str:sub(-1)~="\n")
    f:write(str..(neednl and "\n" or "").."},\n")
end

-- common
local function check_bad_point2()
    local lastloopstart = 0

    for i=0,ffiC.numwalls-1 do
        local p2 = ffiC.wall[i].point2

        if (not (p2 == i+1 or (p2 ~= i and p2 == lastloopstart))) then
            -- If we hit this, the map is seriously corrupted!
            print(string.format("INTERNAL ERROR: wall[%d].point2=%d invalid", i, p2))
            return true
        end

        if (p2 ~= i+1) then
            lastloopstart = i+1
        end
    end
end

local function lastwallofsect(s)
    return ffiC.sector[s].wallptr + ffiC.sector[s].wallnum - 1
end

-- In map-text, instead of saving wall[].point2, we store whether a particular
-- wall is the last one in its loop instead: the on-disk wall[i].point2 is
--  * 2 if wall i is last of its sector (no need to save sector's .wallnum),
--  * 1 if wall i is last of its loop,
--  * 0 otherwise.
-- This function prepares saving to map-text by tweaking the wall[].point2
-- members in-place.
local function save_tweak_point2()
    -- Check first.
    if (check_bad_point2()) then
        return true
    end

    -- Do it for real.
    local lastloopstart = 0
    local cursect, curlastwall = 0, lastwallofsect(0)

    for i=0,ffiC.numwalls-1 do
        local wal = ffiC.wall[i]

        if (wal.point2 == i+1) then
            wal.point2 = 0
        else
            -- Wall i is last point in loop.

            if (i==curlastwall) then
                -- ... and also last wall of sector.
                cursect = cursect+1
                curlastwall = lastwallofsect(cursect)

                wal.point2 = 2
            else
                wal.point2 = 1
            end

            lastloopstart = i+1
        end
    end
end

-- Common: restore tweaked point2 members to actual wall indices.
-- If <alsosectorp> is true, also set sector's .wallptr and .wallnum members.
local function restore_point2(alsosectorp)
    local lastloopstart = 0
    local cursect, curfirstwall = 0, 0

    for i=0,ffiC.numwalls-1 do
        local wal = ffiC.wall[i]
        local islast = (wal.point2~=0)

        if (not islast) then
            wal.point2 = i+1
        else
            -- Wall i is last point in loop.

            if (alsosectorp and wal.point2 == 2) then
                -- ... and also last wall of sector.

                if (cursect==ffiC.MAXSECTORS) then
                    return true  -- Too many sectors.
                end

                ffiC.sector[cursect].wallptr = curfirstwall
                ffiC.sector[cursect].wallnum = i-curfirstwall+1
                cursect = cursect+1
                curfirstwall = i+1
            end

            wal.point2 = lastloopstart
            lastloopstart = i+1
        end
    end
end

local function saveboard_maptext(filename, pos, ang, cursectnum)
    assert(ffiC.numsectors > 0)

    -- First, temporarily tweak wall[].point2.
    if (save_tweak_point2()) then
        return -1
    end

    -- We open in binary mode so that newlines get written out as one byte even
    -- on Windows.
    local f, msg = io.open(ffi.string(filename), "wb")

    if (f == nil) then
        print(string.format("Couldn't open \"%s\" for writing: %s\n", filename, msg))
        restore_point2(false)
        return -1
    end

    -- Write header.
    f:write(string.format("--EDuke32 map\n"..
            "return {\n"..
            "version=10,\n\n"..

            "pos={%d,%d,%d},\n"..
            "sectnum=%d,\n"..
            "ang=%d,\n\n",

            pos.x, pos.y, pos.z,
            cursectnum,
            ang))

    -- Sectors.
    f:write("sector={\n")
    for i=0,ffiC.numsectors-1 do
        write_struct(f, ffiC.sector[i], sector_members, sector_ord)
    end
    f:write("},\n\n")

    -- Walls.
    f:write("wall={\n")
    for i=0,ffiC.numwalls-1 do
        write_struct(f, ffiC.wall[i], wall_members, wall_ord)
    end
    f:write("},\n\n")

    -- Sprites.
    f:write("sprite={\n")
    for i=0,ffiC.MAXSPRITES-1 do
        if (ffiC.sprite[i].statnum ~= ffiC.MAXSTATUS) then
            write_struct(f, ffiC.sprite[i], sprite_members, sprite_ord)
        end
    end
    f:write("},\n\n")

    f:write("}\n");

    -- Done.
    f:close()
    restore_point2(false)
    return 0
end


--== LOADING ==--

local function isnum(v)
    return (type(v)=="number")
end

local function istab(v)
    return (type(v)=="table")
end

-- Checks whether <tab> is a table all values of <tab> are of type <extype>.
local function allxtab(tab, extype)
    if (not istab(tab)) then
        return false
    end

    for _,val in pairs(tab) do
        if (type(val) ~= extype) then
            return false
        end
    end

    return true
end

-- Is table of all numbers?
local function allnumtab(tab) return allxtab(tab, "number") end
-- Is table of all tables?
local function alltabtab(tab) return allxtab(tab, "table") end

-- Is table of tables of all numbers? Additionally, each must contain exactly
-- as many mandatory positional entries as given by the <members> table.
local function tabofnumtabs(tab, members)
    for i=1,#tab do
        if (not allnumtab(tab[i])) then
            return false
        end

        local nummand = #members  -- number of mandatory entries
        if (#tab[i] ~= nummand) then
            return false
        end
    end

    return true
end


-- Read data from Lua table <stab> into C struct <cs>, using the struct
-- description <members>.
-- Returns true on error.
local function read_struct(cs, stab, members, defaults)
    -- Clear struct to default values.
    ffi.copy(cs, defaults, ffi.sizeof(defaults))

    -- Read mandatory positional members.
    for i=1,#members do
        cs[members[i]] = stab[i]
    end

    -- Read optional key/value members.
    for k,val in pairs(stab) do
        if (members[k]==nil) then
            -- No such member abbreviation for the given struct.
            return true
        end

        local memb = istab(members[k]) and members[k][1] or members[k]
        cs[memb] = val
    end
end


local RETERR = -4

local function loadboard_maptext(fil, posptr, angptr, cursectnumptr)
    -- Read the whole map-text as string.
    local str = readintostr(fil)

    if (str == nil) then
        return RETERR
    end

    -- Strip all one-line comments (currently, only the header).
    str = str:gsub("%-%-.-\n", "")

    --- Preliminary (pseudo-syntactical) validation ---

    -- Whitelist approach: map-text may only contain certain characters. This
    -- excludes various potentially 'bad' operations (such as calling a
    -- function) in one blow. Also, this assures (by exclusion) that the Lua
    -- code contains no long comments, strings, or function calls.
    if (not str:find("^[ A-Za-z_0-9{},%-\n=]+$")) then
        return RETERR-1
    end

    -- The map-text code must return a single table.
    if (not str:find("^return %b{}\n$")) then
        return RETERR-2
    end

    local func, errmsg = loadstring(str, "maptext")
    if (func == nil) then
        print("Error preloading map-text Lua code: "..errmsg)
        return RETERR-3
    end

    -- Completely empty the function's environment as an additional safety
    -- measure, then run the chunk protected! (XXX: currently a bit pointless
    -- because of the asserts below.)
    local ok, map = pcall(setfenv(func, {}))
    if (not ok) then
        print("Error executing map-text Lua code: "..map)
        return RETERR-4
    end

    assert(istab(map))
    -- OK, now 'map' contains the map data.

    --- Structural validation ---

    -- Check types.
    if (not isnum(map.version) or not allnumtab(map.pos) or #map.pos~=3 or
        not isnum(map.sectnum) or not isnum(map.ang))
    then
        return RETERR-5
    end

    if (not (map.version <= 10)) then
        return RETERR-6
    end

    local msector, mwall, msprite = map.sector, map.wall, map.sprite

    if (not alltabtab(msector) or not alltabtab(mwall) or not alltabtab(msprite)) then
        return RETERR-7
    end

    if (not tabofnumtabs(msector, sector_members) or
        not tabofnumtabs(mwall, wall_members) or
        not tabofnumtabs(msprite, sprite_members))
    then
        return RETERR-8
    end

    local numsectors, numwalls, numsprites = #msector, #mwall, #msprite
    local sector, wall, sprite = ffiC.sector, ffiC.wall, ffiC.sprite

    if (numsectors == 0 or numsectors > ffiC.MAXSECTORS or
        numwalls > ffiC.MAXWALLS or numsprites > ffiC.MAXSPRITES)
    then
        return RETERR-9
    end

    --- From here on, start filling out C structures. ---

    ffiC.numsectors = numsectors
    ffiC.numwalls = numwalls

    -- Header.
    posptr.x = map.pos[1]
    posptr.y = map.pos[2]
    posptr.z = map.pos[3]

    angptr[0] = map.ang
    cursectnumptr[0] = map.sectnum

    -- Sectors.
    for i=0,numsectors-1 do
        if (read_struct(sector[i], msector[i+1], sector_members, sector_default)) then
            return RETERR-10
        end
    end

    -- Walls.
    for i=0,numwalls-1 do
        if (read_struct(wall[i], mwall[i+1], wall_members, wall_default)) then
            return RETERR-11
        end
    end

    -- Sprites.
    for i=0,numsprites-1 do
        if (read_struct(sprite[i], msprite[i+1], sprite_members, sprite_default)) then
            return RETERR-12
        end
    end

    -- XXX: need to consistency-check much more here! Basically, all of
    -- astub.c's CheckMapCorruption() for corruption level >=4?
    -- See NOTNICE below.

    --- Tweakery: mostly setting dependent members. ---

    -- sector[]: .wallptr calculated from .wallnum.
    local numw = 0
    for i=0,numsectors-1 do
        assert(numw >= 0 and numw < numwalls)  -- NOTNICE, cheap check instead of real one.
        sector[i].wallptr = numw
        numw = numw + sector[i].wallnum
    end

    -- .point2 in {0, 1} --> wall index, sector[].wallptr/.wallnum
    if (restore_point2(true)) then
        return RETERR-13
    end

    -- Check .point2 at least.
    if (check_bad_point2()) then
        return RETERR-14
    end

    -- wall[]: .nextsector calculated by using engine's sectorofwall_noquick()
    for i=0,numwalls-1 do
        local nw = wall[i].nextwall

        if (nw >= 0) then
            assert(nw >= 0 and nw < numwalls)  -- NOTNICE
            wall[i].nextsector = ffiC.sectorofwall_noquick(nw)
        else
            wall[i].nextsector = -1
        end
    end

    -- All OK, return the number of sprites for further engine loading code.
    return numsprites
end


-- Register our Lua functions as callbacks from C.
ffiC.saveboard_maptext = saveboard_maptext
ffiC.loadboard_maptext = loadboard_maptext
