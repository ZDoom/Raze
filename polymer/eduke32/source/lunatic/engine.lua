
local ffi = require("ffi")
local C = ffi.C

local bcarray = require("bcarray")

local error = error
local type = type

local decl = assert(decl)  -- comes from above (defs.ilua or defs_m32.lua)

local ismapster32 = (C.LUNATIC_CLIENT == C.LUNATIC_CLIENT_MAPSTER32)

----------

decl[[
int32_t getclosestcol(int32_t r, int32_t g, int32_t b);
char *palookup[256];  // MAXPALOOKUPS
uint8_t palette[768];

int32_t setpalookup(int32_t palnum, const uint8_t *shtab);
]]

----------

-- The API table
local engine = {}

local pal256_t = bcarray.new("uint8_t", 256, "shade table 256-tuple")
-- The shade table type, effectively a bound-checked uint8_t [32][256]:
local shtab_t = bcarray.new(pal256_t, 32, "shade table")
local SIZEOF_SHTAB = ffi.sizeof(shtab_t)

local RESERVEDPALS = 8  -- KEEPINSYNC build.h: assure that ours is >= theirs
engine.RESERVEDPALS = RESERVEDPALS

local function check_palidx(i)
    if (type(i) ~= "number" or not (i >= 0 and i <= 255-RESERVEDPALS)) then
        error("invalid argument #1: palette index must be in the range [0 .. "..255-RESERVEDPALS.."]", 3)
    end
end

local function err_uncommon_shade_table(ret)
    if (ret == -1) then
        error("loaded engine shade tables don't have 32 gradients of shade", 3)
    end
end

local function palookup_isdefault(palnum)  -- KEEPINSYNC engine.c
    return (C.palookup[palnum] == nil or (palnum ~= 0 and C.palookup[palnum] == C.palookup[0]))
end

function engine.shadetab()
    return shtab_t()
end

function engine.getshadetab(palidx)
    check_palidx(palidx)
    if (palookup_isdefault(palidx)) then
        return nil
    end

    local ret = C.setpalookup(palidx, nil)
    err_uncommon_shade_table(ret)

    local sht = shtab_t()
    ffi.copy(sht, C.palookup[palidx], SIZEOF_SHTAB)
    return sht
end

function engine.setshadetab(palidx, shtab)
    if (not ismapster32 and C.g_elFirstTime == 0) then
        error("setshadetab() may be run only while LUNATIC_FIRST_TIME is true", 2)
    end

    check_palidx(palidx)
    if (not ffi.istype(shtab_t, shtab_t)) then
        error("invalid argument #2: must be a shade table obtained by shadetab()", 2)
    end

    if (not ismapster32 and not palookup_isdefault(palidx)) then
        error("attempt to override already defined shade table", 2)
    end

    local ret = C.setpalookup(palidx, ffi.cast("uint8_t *", shtab))
    err_uncommon_shade_table(ret)
end


local function check_colcomp(a)
    if (type(a) ~= "number" or not (a >= 0 and a <= 63)) then
        error("color component must be in the range [0 .. 63]", 3)
    end
end


-- TODO: other base palettes?
function engine.getrgb(colidx)
    if (type(colidx) ~= "number" or not (colidx >= 0 and colidx <= 255)) then
        error("color index must be in the range [0 .. 255]", 2)
    end

    local rgbptr = C.palette + 3*colidx
    return rgbptr[0], rgbptr[1], rgbptr[2]
end

-- TODO: flag whether fullbrights are OK
function engine.nearcolor(r, g, b)
    check_colcomp(r)
    check_colcomp(g)
    check_colcomp(b)
    return C.getclosestcol(r, g, b)
end


-- Done!
return engine
