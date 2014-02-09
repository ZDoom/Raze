
local ffi = require("ffi")
local C = ffi.C

local bcarray = require("bcarray")

local assert = assert
local error = error
local type = type

local decl = assert(decl)  -- comes from above (defs.ilua or defs_m32.lua)

local ismapster32 = (C.LUNATIC_CLIENT == C.LUNATIC_CLIENT_MAPSTER32)

----------

decl[[
int32_t getclosestcol_lim(int32_t r, int32_t g, int32_t b, int32_t lastokcol);
char *palookup[256];  // MAXPALOOKUPS
uint8_t palette[768];

const char *getblendtab(int32_t blend);
void setblendtab(int32_t blend, const char *tab);

int32_t setpalookup(int32_t palnum, const uint8_t *shtab);
]]

----------


-- The API table
local engine = {}


local shtab_t  -- forward-decl

local function cast_u8ptr(sth)
    return ffi.cast("uint8_t *", sth)
end

local shtab_methods = {
    -- Remap consecutive blocks of 16 color indices and return this new shade
    -- table.
    --
    -- <idxs16>: table with idxs16[0] .. idxs16[15] >= 0 and <= 15
    --  (i.e. 0-based indices of such 16-tuples)
    --
    -- For example, the table
    --  { [0]=0,1, 2,3, 5,4, 6,7, 8,13, 10,11, 12,9, 14,15 }
    -- TODO (...)
    remap16 = function(sht, idxs16)
        if (type(idxs16) ~= "table" or idxs16[0]==nil or #idxs16 ~= 15) then
            error("invalid argument #2: must be a [0]-table with 16 elements", 2)
        end

        for i=0,15 do
            if (not (idxs16[i] >= 0 and idxs16[i] <= 15)) then
                error("invalid reordering table: elements must be in [0 .. 15]", 2)
            end
        end

        local newsht = shtab_t()
        for sh=0,31 do
            for i=0,15 do
                ffi.copy(cast_u8ptr(newsht[sh]) + 16*i,
                         cast_u8ptr(sht[sh]) + 16*idxs16[i], 16)
            end
        end
        return newsht
    end,
}

local function shtab_mt__index(sht, idx)
    local method = shtab_methods[idx]
    if (method) then
        return method
    end
end

local pal256_t = bcarray.new("uint8_t", 256, "color index 256-tuple")
-- The shade table type, effectively a bound-checked uint8_t [32][256]:
shtab_t = bcarray.new(pal256_t, 32, "shade table", nil, nil, { __index = shtab_mt__index })
local SIZEOF_SHTAB = ffi.sizeof(shtab_t)

local blendtab_t = bcarray.new(pal256_t, 256, "blending table")
local SIZEOF_BLENDTAB = ffi.sizeof(blendtab_t)

local RESERVEDPALS = 8  -- KEEPINSYNC build.h: assure that ours is >= theirs
engine.RESERVEDPALS = RESERVEDPALS

local MAXBLENDTABS = 256  -- KEEPINSYNC build.h

local function check_palidx(i)
    if (type(i) ~= "number" or not (i >= 0 and i <= 255-RESERVEDPALS)) then
        error("invalid argument #1: palette swap index must be in the range [0 .. "..255-RESERVEDPALS.."]", 3)
    end
end

local function check_blendidx(i)
    if (type(i) ~= "number" or not (i >= 0 and i <= MAXBLENDTABS-1)) then
        error("invalid argument #1: blending table index must be in the range [0 .. ".. MAXBLENDTABS-1 .."]", 3)
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

function engine.blendtab()
    return blendtab_t()
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

function engine.getblendtab(blendidx)
    check_blendidx(blendidx)

    local ptr = C.getblendtab(blendidx)
    if (ptr == nil) then
        return nil
    end

    local tab = blendtab_t()
    ffi.copy(tab, ptr, SIZEOF_BLENDTAB)
    return tab
end


local function check_first_time()
    if (not ismapster32 and C.g_elFirstTime == 0) then
        error("may be called only while LUNATIC_FIRST_TIME is true", 3)
    end
end

function engine.setshadetab(palidx, shtab)
    check_first_time()
    check_palidx(palidx)

    if (not ffi.istype(shtab_t, shtab)) then
        error("invalid argument #2: must be a shade table obtained by shadetab()", 2)
    end

    if (not ismapster32 and not palookup_isdefault(palidx)) then
        error("attempt to override already defined shade table", 2)
    end

    local ret = C.setpalookup(palidx, cast_u8ptr(shtab))
    err_uncommon_shade_table(ret)
end

function engine.setblendtab(blendidx, tab)
    check_first_time()
    check_blendidx(blendidx)

    if (not ffi.istype(blendtab_t, tab)) then
        error("invalid argument #2: must be a blending table obtained by blendtab()", 2)
    end

    if (not ismapster32 and C.getblendtab(blendidx) ~= nil) then
        error("attempt to override already defined blending table", 2)
    end

    C.setblendtab(blendidx, cast_u8ptr(tab))
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

    -- NOTE: In the game, palette[255*{0..2}] is set to 0 in
    -- G_LoadExtraPalettes() via G_Startup(). However, that's after Lua state
    -- initialization (i.e. when LUNATIC_FIRST_TIME would be true), and in the
    -- editor, it's never changed from the purple color. Therefore, I think
    -- it's more useful to always return the fully black color here.
    if (colidx == 255) then
        return 0, 0, 0
    end

    local rgbptr = C.palette + 3*colidx
    return rgbptr[0], rgbptr[1], rgbptr[2]
end

function engine.nearcolor(r, g, b, lastokcol)
    check_colcomp(r)
    check_colcomp(g)
    check_colcomp(b)

    if (lastokcol == nil) then
        lastokcol = 255
    elseif (type(lastokcol)~="number" or not (lastokcol >= 0 and lastokcol <= 255)) then
        error("invalid argument #4 <lastokcol>: must be in the range [0 .. 255]", 2)
    end

    return C.getclosestcol_lim(r, g, b, lastokcol)
end


---------- Mapster32-only functions ----------

if (ismapster32) then
    local io = require("io")
    local math = require("math")
    local string = require("string")

    ffi.cdef[[size_t fwrite(const void * restrict ptr, size_t size, size_t nmemb, void * restrict stream);]]

    local function validate_more_blendtabs(moreblends)
        if (moreblends == nil) then
            return nil, nil
        end

        -- Additional blending tables: validate <moreblends> table.
        if (type(moreblends) ~= "table") then
            error("invalid argument #4: must be a table", 3)
        end

        local haveblend = { [0]=true }
        local blendnumtab, blendptrtab = {}, {}

        for i=1,#moreblends do
            local tmp = moreblends[i]
            local blendspec = (type(tmp) == "number") and { tmp, tmp } or tmp

            if (not (type(blendspec) == "table" and #blendspec == 2)) then
                error("invalid argument #4: must contain numbers or 2-tables", 3)
            end

            local blend1, blend2 = math.floor(blendspec[1]), math.floor(blendspec[2])

            if (not (type(blend1)=="number" and blend1 >= 1 and blend1 <= 255 and
                     type(blend2)=="number" and blend2 >= 1 and blend2 <= 255)) then
                error("invalid argument #4: blending table numbers must be in [1 .. 255]", 3)
            end

            for bi=blend1,blend2 do
                if (haveblend[bi]) then
                    error("invalid argument #4: duplicate blending table number "..bi, 3)
                end
                haveblend[bi] = true

                local ptr = C.getblendtab(bi)
                if (ptr == nil) then
                    error("invalid argument #4: blending table for number "..bi.." is void", 3)
                end

                blendnumtab[#blendnumtab+1] = bi
                blendptrtab[#blendptrtab+1] = ptr
            end
        end

        assert(#blendnumtab <= 255)
        return blendnumtab, blendptrtab
    end

    -- [ok, errmsg] = engine.savePaletteDat(filename [, palnum [, blendnum [, moreblends]]])
    function engine.savePaletteDat(filename, palnum, blendnum, moreblends)
        local sht = engine.getshadetab(palnum or 0)
        local tab = engine.getblendtab(blendnum or 0)

        if (sht == nil) then
            return nil, "no shade table with number "..palnum
        elseif (tab == nil) then
            return nil, "no blending table with number "..blendnum
        end

        local blendnumtab, blendptrtab = validate_more_blendtabs(moreblends)

        local f, errmsg = io.open(filename, "w+")
        if (f == nil) then
            return nil, errmsg
        end

        local n1 = C.fwrite(C.palette, 3, 256, f)
        f:write("\032\000")  -- int16_t numshades
        local n3 = C.fwrite(sht, 256, 32, f)
        local n4 = C.fwrite(tab, 256, 256, f)

        if (n1 ~= 256 or n3 ~= 32 or n4 ~= 256) then
            return nil, "failed writing classic PALETTE.DAT data"
        end

        if (blendnumtab ~= nil) then
            f:write("MoreBlendTab")
            f:write(string.char(#blendnumtab))

            for i=1,#blendnumtab do
                f:write(string.char(blendnumtab[i]))
                if (C.fwrite(blendptrtab[i], 256, 256, f) ~= 256) then
                    return nil, "failed writing additional blending table"
                end
            end
        end

        f:close()

        return true
    end
end


-- Done!
return engine
