-- MAPTEXT
-- Lunatic: routines for reading and writing map-text.

local ffi = require("ffi")
local ffiC = ffi.C

local print = print
local tonumber = tonumber
local type = type

local io = require("io")
local string = require("string")


ffi.cdef[[
int32_t (*saveboard_maptext)(const char *filename, const vec3_t *dapos, int16_t daang, int16_t dacursectnum);
]]


--== COMMON ==--

local sector_members = {
    -- Mandatory positional members first, [pos]=<name>.
    "wallnum",
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
    _ = "filler",
    o = "lotag", i = "hitag", e = { "extra", -1 }
}

-- Defines the order in which the members are written out. A space denotes that
-- a newline should appear in the output.
local sector_ord = { mand="1 23 45 67 ", opt="Bb Ff Hh Pp Xx Yy v _ oie" }


local wall_members = {
    -- mandatory
    "point2",  -- special: 0 or 1 in map-text
    "x", "y",
    "nextwall",
    "picnum",
    "shade",
    "xrepeat", "yrepeat",
    "xpanning", "ypanning";

    -- optional
    f = "cstat",
    m = "overpicnum",
    p = "pal",
    o = "lotag", i = "hitag", e = { "extra", -1 }
}

local wall_ord = { mand="1 23 4 5 6 78 90 ", opt="f m p oie" }


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
    _ = "filler",
    x = "xoffset", y = "yoffset",
    s = "statnum",
    w = { "owner", -1 },
    X = "xvel", Y = "yvel", Z = "zvel",
    o = "lotag", i = "hitag", e = { "extra", -1 }
}

local sprite_ord = { mand="123 4 5 6 7 8 90 ", opt="p c _ xy s w XYZ oie" }


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

    f:write(str.."},\n")
end

-- In map-text, instead of saving wall[].point2, we store whether a particular
-- wall is the last one in its loop instead.
local function save_tweak_point2()
    local lastloopstart = 0

    -- Check first.
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

    -- Do it for real.
    lastloopstart = 0

    for i=0,ffiC.numwalls-1 do
        local wal = ffiC.wall[i]

        if (wal.point2 == i+1) then
            wal.point2 = 0
        else
            wal.point2 = 1  -- last point in loop
            lastloopstart = i+1
        end
    end
end

local function save_restore_point2()
    local lastloopstart = 0

    for i=0,ffiC.numwalls-1 do
        local wal = ffiC.wall[i]
        local islast = (wal.point2~=0)

        if (not islast) then
            wal.point2 = i+1
        else
            wal.point2 = lastloopstart
            lastloopstart = i+1
        end
    end
end

local function saveboard_maptext(filename, pos, ang, cursectnum)
    -- First, temporarily tweak wall[].point2.
    if (save_tweak_point2()) then
        return -1
    end

    -- We open in binary mode so that newlines get written out as one byte even
    -- on Windows.
    local f, msg = io.open(ffi.string(filename), "wb")

    if (f == nil) then
        print(string.format("Couldn't open \"%s\" for writing: %s\n", filename, msg))
        save_restore_point2()
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
    f:write("}\n\n")

    -- Walls.
    f:write("wall={\n")
    for i=0,ffiC.numwalls-1 do
        write_struct(f, ffiC.wall[i], wall_members, wall_ord)
    end
    f:write("}\n\n")

    -- Sprites.
    f:write("sprite={\n")
    for i=0,ffiC.MAXSPRITES-1 do
        if (ffiC.sprite[i].statnum ~= ffiC.MAXSTATUS) then
            write_struct(f, ffiC.sprite[i], sprite_members, sprite_ord)
        end
    end
    f:write("}\n\n")

    f:write("}\n");

    -- Done.
    f:close()
    save_restore_point2()
    return 0
end


-- Register our Lua functions as callbacks from C.
ffiC.saveboard_maptext = saveboard_maptext
