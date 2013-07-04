
-- This file contains LuaJIT definitions of stuff that's common to the game and
-- editor.  The 'decl' function is expected to be defined in the global
-- environment.
-- See the included license file "BUILDLIC.TXT" for license info.

-- Will be 'true' if running from engine Lua state:
local _LUNATIC_AUX = _LUNATIC_AUX

local ffi = require("ffi")
local ffiC = ffi.C
local bit = require("bit")

-- Lunatic debugging (mostly bitfield):
-- ~=0: print diagnostic information
--   2: disable JIT compilation
--   4: load LuaJIT's 'v' module, printing trace info
--   8: load LuaJIT's 'dump' module, printing generated IR/machine code
ffi.cdef "enum { _DEBUG_LUNATIC=1 }"

if (bit.band(ffiC._DEBUG_LUNATIC, 2)~=0) then
    require("jit").off()
end

if (not _LUNATIC_AUX) then
    if (bit.band(ffiC._DEBUG_LUNATIC, 8)~=0) then
        require("dump").on("+rs")
    elseif (bit.band(ffiC._DEBUG_LUNATIC, 4)~=0) then
        require("v").on()
    end
end

local math = require("math")
local string = require("string")
local table = require("table")

local assert = assert
local error = error
local pairs = pairs
local require = require
local setmetatable = setmetatable
local tostring = tostring
local type = type

local decl = assert(decl)
local getfenv = getfenv

decl "void OSD_Printf(const char *fmt, ...);"
print = function(str)
    -- our "print" doesn't use the global "tostring", but the initial one
    str = tostring(str)
    if (type(str) ~= "string") then
        error("invalid argument to print: must be convertible to a string")
    end
    ffiC.OSD_Printf("%s\n", str)
end

local print=print


module(...)


local band = bit.band
local bor = bit.bor
local bnot = bit.bnot
local lshift = bit.lshift
local rshift = bit.rshift
local xor = bit.bxor


--== bitint type factory ==--

-- Metatable for an integer type that is treated as bitfield. The integer
-- itself must be named '_v'.
local bitint_mt = {
    __index = {
        set = function(self, bits)
            self._v = bor(self._v, bits)
        end,

        clear = function(self, bits)
            self._v = band(self._v, bnot(bits))
        end,

        flip = function(self, bits)
            self._v = xor(self._v, bits)
        end,

        test = function(self, bits)
            return (band(self._v, bits) ~= 0)
        end,

        mask = function(self, bits)
            return band(self._v, bits)
        end,
    },

    __metatable = true,
}

local bitint_to_base_type = {}

function bitint_new_struct_type(basetypename, newtypename)
    assert(bitint_to_base_type[newtypename] == nil)
    assert(type(basetypename)=="string")
    assert(type(newtypename)=="string")

    local bitint_struct_t = ffi.typeof("struct { $ _v; }", ffi.typeof(basetypename))
    ffi.metatype(bitint_struct_t, bitint_mt)
    ffi.cdef("typedef $ $;", bitint_struct_t, newtypename)

    bitint_to_base_type[newtypename] = basetypename
end

function bitint_member(bitint_struct_typename, membname)
    return string.format("union { %s %s; %s %sbits; };",
                         bitint_to_base_type[bitint_struct_typename], membname,
                         bitint_struct_typename, membname)
end

bitint_new_struct_type("uint8_t", "UBit8")
bitint_new_struct_type("uint16_t", "UBit16")


--== Core engine structs ==--

local CF_MEMBERS = [[
    const int16_t ~picnum;
    int16_t ~heinum;
    const int16_t ~bunch;
]]..bitint_member("UBit16", "~stat")..[[
    int32_t ~z;
    int8_t ~shade;
    uint8_t ~pal, ~xpanning, ~ypanning;
]]

ffi.cdef("typedef struct { "..CF_MEMBERS:gsub("~","").." } ceiling_or_floor_t;")

local SECTOR_STRUCT = [[
struct {
    const int16_t wallptr, wallnum;
]]..
string.format([[
    union {
      struct { ceiling_or_floor_t ceiling, floor; };
      struct { %s %s };
    };
]], CF_MEMBERS:gsub("~","ceiling"), CF_MEMBERS:gsub("~","floor"))
..[[
    uint8_t visibility, filler;
    int16_t lotag, hitag;  // NOTE: signed for Lunatic
    int16_t extra;
}]]

local SPRITE_STRUCT = [[
struct {
    int32_t x, y, z;
]]..bitint_member("UBit16", "cstat")..[[
    const int16_t picnum;
    int8_t shade;
    uint8_t pal, clipdist, filler;
    uint8_t xrepeat, yrepeat;
    int8_t xoffset, yoffset;
    const int16_t sectnum, statnum;
    int16_t ang;

    const int16_t owner;
    int16_t xvel;
    // NOTE: yvel is often used as player index in game code.
    const int16_t yvel;
    int16_t zvel;

    int16_t lotag, hitag, extra;
}]]

local WALL_STRUCT = [[
struct {
    int32_t x, y;
    const int16_t point2, nextwall, nextsector;
    const int16_t upwall, dnwall;
]]..bitint_member("UBit16", "cstat")..[[
    const int16_t picnum, overpicnum;
    int8_t shade;
    uint8_t pal, xrepeat, yrepeat, xpanning, ypanning;
    int16_t lotag, hitag, extra;
}]]

-- Converts a template struct definition to an internal, unrestricted one.
-- NOTE: "[^ ]*" for const decorations in defs.ilua.
function strip_const(structstr)
    return (string.gsub(structstr, "const[^ ]* ", ""));
end

local function maybe_strip_const(str)
    return _LUNATIC_AUX and strip_const(str) or str
end

-- NOTE for FFI definitions: we're compiling EDuke32 with -funsigned-char, so
-- we need to take care to declare chars as unsigned whenever it matters, for
-- example if it represents a palette index.  (I think it's harmless for stuff
-- like passing a function argument, but it should be done there for clarity.)

-- TODO: provide getters for unsigned {hi,lo}tag?
ffi.cdef([[
typedef $ sectortype;
typedef $ walltype;
// NOTE: spritetype and tspritetype are different types with the same data members.
typedef $ spritetype;
typedef struct { spritetype; } tspritetype;

typedef struct {
    int32_t x, y, z;
} vec3_t;

typedef struct {
    const uint32_t mdanimtims;
    const int16_t mdanimcur;
    int16_t angoff, pitch, roll;
    vec3_t mdoff;
]]..bitint_member("UBit8", "flags")..[[
    uint8_t xpanning, ypanning;
    const uint8_t filler;
    float alpha;
    // NOTE: const aggregate fixed with LuaJIT git fe9934feea0a8d580de1
    // ("FFI: Fix handling of qualified transparent structs/unions.")
    const union {
        intptr_t _tspr;
        struct { int32_t _dummy0, _dummy1; };
    };
} spriteext_t;

typedef struct {
    vec3_t pos;
    int16_t sprite, wall, sect;
} hitdata_t;
]],
ffi.typeof(maybe_strip_const(SECTOR_STRUCT)),
ffi.typeof(maybe_strip_const(WALL_STRUCT)),
ffi.typeof(maybe_strip_const(SPRITE_STRUCT)))

if (not _LUNATIC_AUX) then
    -- Define the "palette_t" type, which for us has .{r,g,b} fields and a
    -- bound-checking array of length 3 overlaid.
    local rgbarray_t = require("bcarray").new("uint8_t", 3, "RGB array", "palette_t",
                                              { "r", "g", "b", "f" })
    ffi.cdef("typedef union { \
        struct { uint8_t r, g, b, f; }; \
        $ col; \
    } palette_t", rgbarray_t)

    assert(ffi.alignof("palette_t")==1)
end

local vec3_ct = ffi.typeof("vec3_t")  -- will be metatype'd in xmath.lua:

if (not _LUNATIC_AUX) then
    require("xmath")
end

local hitdata_ct = ffi.typeof("hitdata_t")

decl[[
const int32_t engine_main_arrays_are_static;
const int32_t engine_v8;
]]


--== Engine data and functions ==--


-- NOTE TO SELF: This is not C, never EVER write
--    if (x)
-- when checking a C variable x for 'thuthiness'
if (ffiC.engine_main_arrays_are_static ~= 0) then
    decl[[
    sectortype sector[];
    walltype wall[];
    spritetype sprite[];
    tspritetype tsprite[];
    spriteext_t spriteext[];
    ]]
else
    decl[[
    sectortype *sector;
    walltype *wall;
    spritetype *sprite;
    tspritetype *tsprite;
    spriteext_t *spriteext;
    ]]
end

if (ffiC.engine_v8 == 0) then
    -- V7
    ffi.cdef[[
enum
{
    MAXSECTORS = 1024,
    MAXWALLS = 8192,
    MAXSPRITES = 4096,
}
]]
else
    -- V8
    ffi.cdef[[
enum
{
    MAXSECTORS = 4096,
    MAXWALLS = 16384,
    MAXSPRITES = 16384,
}
]]
end

ffi.cdef[[
enum {
    MAXSTATUS = 1024,
    MAXTILES = 30720,
    MAXSPRITESONSCREEN = 4096,

    MAXBUNCHES = 512,
    CEILING = 0,
    FLOOR = 1,

    CLIPMASK0 = (1<<16)+1,  // blocking
    CLIPMASK1 = (256<<16)+64,  // hittable
};
]]

ffi.cdef(maybe_strip_const("const int16_t numsectors, numwalls;"))

ffi.cdef[[
const int32_t numyaxbunches;  // XXX
const int32_t totalclock;
int32_t randomseed;  // DEPRECATED
const int32_t xdim, ydim;
const int32_t windowx1, windowy1, windowx2, windowy2;
]]

decl[[
int32_t kopen4load(const char *filename, char searchfirst);
int32_t kfilelength(int32_t handle);
void kclose(int32_t handle);
int32_t kread(int32_t handle, void *buffer, int32_t leng);
int32_t klseek(int32_t handle, int32_t offset, int32_t whence);

int32_t sectorofwall_noquick(int16_t theline);
]]

-- Reads the whole file given by the k* file descriptor into a Lua string.
-- Always closes the file descriptor.
function readintostr(fd, kopen4load_func)
    -- XXX: this is pretty much the same as the code in L_RunOnce()

    local sz = ffiC.kfilelength(fd)
    if (sz == 0) then
        ffiC.kclose(fd)
        return ""
    end

    if (sz < 0) then
        ffi.kclose(fd)
        error("INTERNAL ERROR: kfilelength() returned negative length")
    end

    local str = ffi.new("char [?]", sz)
    local readlen = ffiC.kread(fd, str, sz)

    ffiC.kclose(fd); fd=-1

    if (readlen ~= sz) then
        error("INTERNAL ERROR: couldn't read file wholly")
    end

    return ffi.string(str, sz)
end

if (_LUNATIC_AUX) then
    -- XXX: The global doesn't show up in 'engine_maptext'.
    -- I guess I still haven't fully grokked globals in Lua.
    string.readintostr = readintostr
    require "engine_maptext"
    return
end

ffi.cdef "const int32_t rendmode;"

decl[[
int32_t yxaspect;
int32_t viewingrange;
int32_t spritesortcnt;
int32_t guniqhudid;
const int16_t headspritesect[MAXSECTORS+1], headspritestat[MAXSTATUS+1];
const int16_t prevspritesect[MAXSPRITES], prevspritestat[MAXSPRITES];
const int16_t nextspritesect[MAXSPRITES], nextspritestat[MAXSPRITES];
const int16_t tilesizx[MAXTILES], tilesizy[MAXTILES];

uint8_t show2dsector[(MAXSECTORS+7)>>3];

const int16_t headsectbunch[2][MAXBUNCHES], nextsectbunch[2][MAXSECTORS];

int16_t yax_getbunch(int16_t i, int16_t cf);

int32_t   getceilzofslopeptr(const sectortype *sec, int32_t dax, int32_t day);
int32_t   getflorzofslopeptr(const sectortype *sec, int32_t dax, int32_t day);
void   getzsofslopeptr(const sectortype *sec, int32_t dax, int32_t day,
                       int32_t *ceilz, int32_t *florz);
int32_t spriteheightofsptr(const spritetype *spr, int32_t *height, int32_t alsotileyofs);

int32_t changespritesect(int16_t spritenum, int16_t newsectnum);
int32_t changespritestat(int16_t spritenum, int16_t newstatnum);

int32_t hitscan(const vec3_t *sv, int16_t sectnum, int32_t vx, int32_t vy, int32_t vz,
                hitdata_t *hitinfo, uint32_t cliptype);
int32_t cansee(int32_t x1, int32_t y1, int32_t z1, int16_t sect1,
               int32_t x2, int32_t y2, int32_t z2, int16_t sect2);
void neartag(int32_t xs, int32_t ys, int32_t zs, int16_t sectnum, int16_t ange, int16_t *neartagsector, int16_t *neartagwall,
             int16_t *neartagsprite, int32_t *neartaghitdist, int32_t neartagrange, uint8_t tagsearch,
             int32_t (*blacklist_sprite_func)(int32_t));
void dragpoint(int16_t pointhighlight, int32_t dax, int32_t day, uint8_t flags);
void getzrange(const vec3_t *pos, int16_t sectnum,
               int32_t *ceilz, int32_t *ceilhit, int32_t *florz, int32_t *florhit,
               int32_t walldist, uint32_t cliptype);
int32_t clipmovex(vec3_t *pos, int16_t *sectnum, int32_t xvect, int32_t yvect,
                  int32_t walldist, int32_t ceildist, int32_t flordist, uint32_t cliptype,
                  uint8_t noslidep);

int32_t ldist(const spritetype *s1, const spritetype *s2);
int32_t dist(const spritetype *s1, const spritetype *s2);

int32_t inside(int32_t x, int32_t y, int16_t sectnum);
void updatesector(int32_t x, int32_t y, int16_t *sectnum);
void updatesectorbreadth(int32_t x, int32_t y, int16_t *sectnum);
void updatesectorz(int32_t x, int32_t y, int32_t z, int16_t *sectnum);

void rotatesprite_(int32_t sx, int32_t sy, int32_t z, int16_t a, int16_t picnum,
                  int8_t dashade, unsigned char dapalnum, int32_t dastat, uint8_t alpha,
                  int32_t cx1, int32_t cy1, int32_t cx2, int32_t cy2);

void setaspect(int32_t daxrange, int32_t daaspect);
]]

-- misc. functions
ffi.cdef[[
uint32_t getticks(void);
double gethitickms(void);

int32_t krand(void);
int32_t ksqrt(uint32_t num);
int32_t __fastcall getangle(int32_t xvect, int32_t yvect);
int32_t Mulscale(int32_t a, int32_t b, int32_t sh);
]]

local bcheck = require("bcheck")
local check_sector_idx = bcheck.sector_idx
local check_wall_idx = bcheck.wall_idx
local check_sprite_idx = bcheck.sprite_idx
local check_tile_idx = bcheck.tile_idx

local wallsofsec  -- fwd-decl

local sectortype_ptr_ct = ffi.typeof("$ *", ffi.typeof(strip_const(SECTOR_STRUCT)))

local function get_sector_idx(sec)
    local i = ffi.cast(sectortype_ptr_ct, sec)-ffi.cast(sectortype_ptr_ct, ffiC.sector)
    assert(not (i >= ffiC.numsectors+0ULL))
    return i
end

local zret = ffi.new("int32_t [4]")
local zret_t = ffi.typeof[[const struct {
    struct {
        bool spritep;
        int32_t num;  // number of sector or sprite
        int32_t z;
    } c, f;
}]]

local sectortype_mt = {
    __index = {
        --- Setters
        set_ceilingpicnum = function(s, picnum)
            check_tile_idx(picnum)
            ffi.cast(sectortype_ptr_ct, s).ceilingpicnum = picnum
        end,

        set_floorpicnum = function(s, picnum)
            check_tile_idx(picnum)
            ffi.cast(sectortype_ptr_ct, s).floorpicnum = picnum
        end,

        --- Other methods
        ceilingzat = function(s, pos)
            return ffiC.getceilzofslopeptr(s, pos.x, pos.y)
        end,

        floorzat = function(s, pos)
            return ffiC.getflorzofslopeptr(s, pos.x, pos.y)
        end,

        -- getzrange() interface
        zrangeat = function(s, pos, walldist, cliptype)
            local sectnum = get_sector_idx(s)
            local ipos = vec3_ct(pos.x, pos.y, pos.z)
            walldist = walldist or 128
            cliptype = cliptype or ffiC.CLIPMASK0

            ffiC.getzrange(ipos, sectnum, zret+0, zret+1, zret+2, zret+3,
                           walldist, cliptype)
            local ceilz, ceilhit, florz, florhit = zret[0], zret[1], zret[2], zret[3]

            return zret_t({ ceilhit>=49152, bit.band(ceilhit,16383), ceilz },
                          { florhit>=49152, bit.band(florhit,16383), florz })
        end,

        -- inside() port, OUTOFSYNC with engine.c
        contains = function(s, pos)
            local x, y = pos.x, pos.y
            local cnt = 0

            for w in wallsofsec(s) do
                local wal2 = ffiC.wall[ffiC.wall[w].point2]
                local y1, y2 = ffiC.wall[w].y-y, wal2.y-y

                if (xor(y1, y2) < 0) then
                    local x1, x2 = ffiC.wall[w].x-x, wal2.x-x

                    if (xor(x1, x2)>=0) then
                        cnt = xor(cnt, x1)
                    else
                        cnt = xor(cnt, xor(x1*y2-x2*y1, y2))
                    end
                end
            end

            return (cnt < 0)
        end,
    }
}
ffi.metatype("sectortype", sectortype_mt)

local walltype_ptr_ct = ffi.typeof("$ *", ffi.typeof(strip_const(WALL_STRUCT)))

local walltype_mt = {
    __index = {
        --- Setters
        set_picnum = function(w, picnum)
            check_tile_idx(picnum)
            ffi.cast(walltype_ptr_ct, w).picnum = picnum
        end,

        set_overpicnum = function(w, picnum)
            check_tile_idx(picnum)
            ffi.cast(walltype_ptr_ct, w).overpicnum = picnum
        end,

        _set_nextwall = function(w, nextwall)
            -- NOTE: Allow setting a wall to white too, but no checking of the
            -- consistency invariant ".nextwall>=0 iff .nextsector>=0".
            if (not (nextwall < 0)) then
                check_wall_idx(nextwall)
            end
            ffi.cast(walltype_ptr_ct, w).nextwall = nextwall
        end,

        _set_nextsector = function(w, nextsector)
            if (not (nextsector < 0)) then
                check_sector_idx(nextsector)
            end
            ffi.cast(walltype_ptr_ct, w).nextsector = nextsector
        end,

        --- Predicates
        isblocking = function(w)
            return (band(w.cstat, 1)~=0)
        end,

        ismasked = function(w)
            return (band(w.cstat, 16)~=0)
        end,

        isoneway = function(w)
            return (band(w.cstat, 32)~=0)
        end,

        ishittable = function(w)
            return (band(w.cstat, 64)~=0)
        end,

        -- Indexing a wall with 'z' gets 0, so that you can e.g. use a wall as
        -- RHS to vec3_t addition.
        z = 0,
    }
}
ffi.metatype("walltype", walltype_mt)

local spriteext_mt = {
    __index = {
        -- Enable EVENT_ANIMATESPRITES for this sprite.
        make_animated = function(sx)
            sx.flags = bor(sx.flags, 16)
        end,
    },
}
ffi.metatype("spriteext_t", spriteext_mt)

local spritetype_ptr_ct = ffi.typeof("$ *", ffi.typeof(strip_const(SPRITE_STRUCT)))
-- NOTE: this is the *protected* tspritetype pointer.
local tspritetype_ptr_ct = ffi.typeof("$ *", ffi.typeof("tspritetype"))

local intarg = ffi.new("int32_t[1]")

local spritetype_mt = {
    __pow = function(s, zofs)
        return vec3_ct(s.x, s.y, s.z-zofs)
    end,

    __index = {
        --- Setters
        set_picnum = function(s, tilenum)
            check_tile_idx(tilenum)
            ffi.cast(spritetype_ptr_ct, s).picnum = tilenum
        end,

        _set_yvel = function(s, yvel)
            -- XXX: no protection against malicious use (might set picnum to
            -- another one temporarily)
            -- XXX: this belongs into game-side Lunatic
            if (s.picnum==1405) then  -- APLAYER
                error("setting yvel on an APLAYER sprite forbidden", 2)
            end
            ffi.cast(spritetype_ptr_ct, s).yvel = yvel
        end,

        _set_owner = function(s, owner)
            -- XXX: AMC TC sets owner to -1 in the cutscene.
            check_sprite_idx(owner)
            ffi.cast(spritetype_ptr_ct, s).owner = owner
        end,

        --- Custom methods ---

        getheightofs = function(s)
            -- XXX: better reimplement in Lua?
            local zofs = ffiC.spriteheightofsptr(s, intarg, 0)
            return intarg[0], zofs
        end,
    },
}

local function deep_copy(tab)
    local ntab = {}
    for key, val in pairs(tab) do
        if (type(val)=="table") then
            ntab[key] = deep_copy(val)
        else
            assert(type(val)=="function")
            ntab[key] = val
        end
    end
    return ntab
end

local tspritetype_mt = deep_copy(spritetype_mt)

local function get_sprite_idx(spr)
    local i = ffi.cast(spritetype_ptr_ct, spr)-ffi.cast(spritetype_ptr_ct, ffiC.sprite)
    assert(not (i >= ffiC.MAXSPRITES+0ULL))
    return i
end

-- Methods that are specific to sprites
function spritetype_mt.__index.setpos(spr, pos)  -- setsprite() clone
    spr.x, spr.y, spr.z = pos.x, pos.y, pos.z

    local newsect = updatesector(spr, spr.sectnum)
    if (newsect < 0) then
        return -1
    end

    if (spr.sectnum ~= newsect) then
        ffiC.changespritesect(get_sprite_idx(spr), newsect)
    end
    return newsect
end

-- Methods that are specific to tsprites
function tspritetype_mt.__index.dup(tspr)
    if (ffiC.spritesortcnt >= ffiC.MAXSPRITESONSCREEN+0ULL) then
        return nil
    end

    local newtspr = ffiC.tsprite[ffiC.spritesortcnt]
    ffi.copy(newtspr, tspr, ffi.sizeof(tspr))
    ffiC.spritesortcnt = ffiC.spritesortcnt+1

    return newtspr
end

function tspritetype_mt.__index.getspr(tspr)
    return sprite[tspr.owner]
end


-- The user of this module can insert additional "spritetype" index
-- methods and register them with "ffi.metatype".
function finish_spritetype(mt_index)
    for name, func in pairs(mt_index) do
        spritetype_mt.__index[name] = func
        tspritetype_mt.__index[name] = func
    end
    ffi.metatype("spritetype", spritetype_mt)
    ffi.metatype("tspritetype", tspritetype_mt)
end


---=== Restricted access to C variables from Lunatic ===---

-- set metatable and forbid setting it further
function setmtonce(tab, mt)
    mt.__metatable = true
    return setmetatable(tab, mt)
end

---- indirect C array access ----

-- create a safe indirection for an ffi.C array
function creategtab(ctab, maxidx, name)
    local tab = {}
    local tmpmt = {
        __index = function(tab, key)
            if (key>=0 and key < maxidx) then
                return ctab[key]
            end
            error('out-of-bounds '..name..' read access', 2)
        end,
        __newindex = function()
            error('cannot write directly to '..name, 2)
        end,
    }

    return setmtonce(tab, tmpmt)
end

-- Construct const struct from table
function conststruct(tab)
    local strtab = { "const struct { int32_t " }
    local vals = {}

    for member, val in pairs(tab) do
        strtab[#strtab+1] = member..","
        vals[#vals+1] = val
    end
    strtab[#strtab] = strtab[#strtab]:gsub(',',';')
    strtab[#strtab+1] = "}"

    return ffi.new(table.concat(strtab), vals)
end

-- Static, non-instance members. Used to hold constants, for example
-- sprite.CSTAT.TRANS1
local static_members = { sector={}, wall={}, sprite={} }

static_members.sector.STAT = conststruct
{
    PARALLAX = 1,
    SLOPE = 2,
    SWAPXY = 4,
    SMOOSH = 8,
    FLIPX = 16,
    FLIPY = 32,
    RELATIVE = 64,
    MASK = 128,
    -- NOTE the reversed order
    TRANS2 = 128,
    TRANS1 = 256,
    BLOCK = 512,
    HITSCAN = 1024,

    FLIP_BITMASK = 16+32,
    ORIENT_BITMASK = 4+16+32,
    TRANS_BITMASK = 128+256,
}

static_members.sector.NEARTAG_FLAGS = conststruct
{
    LOTAG = 1,
    HITAG = 2,
    NOSPRITES = 4,
}

static_members.sector.UPDATE_FLAGS = conststruct
{
    BREADTH = 1,
}

static_members.wall.CSTAT = conststruct
{
    BLOCK = 1,
    BOTTOMSWAP = 2,
    ALIGNBOTTOM = 4,
    FLIPX = 8,
    MASK = 16,
    ONEWAY = 32,
    HITSCAN = 64,
    TRANS1 = 128,
    FLIPY = 256,
    TRANS2 = 512,

    FLIP_BITMASK = 8+256,
    TRANS_BITMASK = 128+512,
}

static_members.sprite.CSTAT = conststruct
{
    BLOCK = 1,
    TRANS1 = 2,
    XFLIP = 4,
    YFLIP = 8,
    ALIGNWALL = 16,
    ALIGNFLOOR = 32,
    ONESIDE = 64,
    CENTER = 128,
    HITSCAN = 256,
    TRANS2 = 512,

    ALIGN_BITMASK = 16+32,
    TRANS_BITMASK = 2+512,
}

local bitar = require("bitar")
-- XXX: bitar uses int32_t arrays, while show2dsector[] is a uint8_t
-- array. Potential unaligned access. Also, only works on little-endian
-- machines. This sucks.
static_members.sector.showbitmap = bitar.new(ffiC.MAXSECTORS-1, ffi.cast("int32_t *", ffiC.show2dsector))

local sms = static_members.sprite
sms._headspritesect = creategtab(ffiC.headspritesect, ffiC.MAXSECTORS, 'headspritesect[]')
-- NOTE: don't allow freelist access
sms._headspritestat = creategtab(ffiC.headspritestat, ffiC.MAXSTATUS, 'headspritestat[]')
sms._nextspritesect = creategtab(ffiC.nextspritesect, ffiC.MAXSPRITES, 'nextspritesect[]')
sms._nextspritestat = creategtab(ffiC.nextspritestat, ffiC.MAXSPRITES, 'nextspritestat[]')
sms._prevspritesect = creategtab(ffiC.prevspritesect, ffiC.MAXSPRITES, 'prevspritesect[]')
sms._prevspritestat = creategtab(ffiC.prevspritestat, ffiC.MAXSPRITES, 'prevspritestat[]')

function static_members.wall.dragto(wallnum, pos)
    check_wall_idx(wallnum)

    -- TODO: some kind of validation of the position?
    ffiC.dragpoint(wallnum, pos.x, pos.y, 0)
end

function static_members.sprite.changesect(spritenum, sectnum, noerr)
    check_sprite_idx(spritenum)
    check_sector_idx(sectnum)
    if (ffiC.changespritesect(spritenum, sectnum)==-1 and not noerr) then
        error("cannot change sector number of sprite not in the game world", 2)
    end
end

function static_members.sprite.changestat(spritenum, statnum, noerr)
    -- TODO: see gameexec.c's CON_CHANGESPRITESTAT.
    check_sprite_idx(spritenum)
    if (statnum >= ffiC.MAXSTATUS+0ULL) then
        error("invalid status number "..statnum, 2)
    end
    if (ffiC.changespritestat(spritenum, statnum)==-1 and not noerr) then
        error("cannot change status number of sprite not in the game world", 2)
    end
end

function GenStructMetatable(Structname, Boundname, StaticMembersTab)
    StaticMembersTab = StaticMembersTab or static_members[Structname]

    return {
        __index = function(tab, key)
            if (type(key)=="number") then
                if (key >= 0 and key < ffiC[Boundname]) then
                    return ffiC[Structname][key]
                end
                error("out-of-bounds "..Structname.."[] read access with index "..key, 2)
            elseif (type(key)=="string") then
                return StaticMembersTab[key]
            end
        end,

        __newindex = function() error("cannot write directly to "..Structname.."[]", 2) end,
    }
end

local sector_mt = GenStructMetatable("sector", "numsectors")
local wall_mt = GenStructMetatable("wall", "numwalls")
local sprite_mt = GenStructMetatable("sprite", "MAXSPRITES")

local atsprite_mt = {
    __index = function(tab, idx)
        check_sprite_idx(idx)

        local tspr = ffi.cast(tspritetype_ptr_ct, ffiC.spriteext[idx]._tspr)
        if (tspr == nil) then
            error("tsprite of actor "..idx.." unavailable", 2)
        end

        -- Return a reference to a tsprite[] element.
        return tspr[0]
    end,

    __newindex = function() error('cannot write directly to atsprite[]', 2) end,
}


local vars_to_ignore = {}
for varname,_ in pairs(getfenv(1)) do
    if (ffiC._DEBUG_LUNATIC ~= 0) then
        print("IGNORE "..varname)
    end
    vars_to_ignore[varname] = true
end

--== ALL GLOBALS FROM HERE ON ARE EXPORTED UPWARDS (see create_globals() below) ==--

sector = setmtonce({}, sector_mt)
wall = setmtonce({}, wall_mt)
sprite = setmtonce({}, sprite_mt)
spriteext = creategtab(ffiC.spriteext, ffiC.MAXSPRITES, 'spriteext[]')
atsprite = setmtonce({}, atsprite_mt)

local function iter_wallsofsec(endwall, w)
    w = w+1
    if (w < endwall) then
        return w
    end
end

wallsofsec = function(sec)  -- local
    return iter_wallsofsec, sec.wallptr+sec.wallnum, sec.wallptr-1
end

function wallsofsect(sect)
    check_sector_idx(sect)
    return iter_wallsofsec, sector[sect].wallptr+sector[sect].wallnum, sector[sect].wallptr-1
end

--== Per-sector/per-statnum sprite iterators ==--
local function iter_spritesofsect(sect, i)
    if (i < 0) then
        i = ffiC.headspritesect[sect]
    else
        i = ffiC.nextspritesect[i]
    end

    if (i >= 0) then return i end
end

-- sprites of sectnum iterator that allows deleting the iterated sprite
local function iter_spritesofsect_safe(tab, i)
    if (i < 0) then
        i = ffiC.headspritesect[-i]
    else
        i = tab[1]
    end

    if (i >= 0) then
        tab[1] = ffiC.nextspritesect[i]
        return i
    end
end

function spritesofsect(sect, maydelete)
    check_sector_idx(sect)

    if (maydelete) then
        return iter_spritesofsect_safe, { -1 }, -sect
    else
        return iter_spritesofsect, sect, -1
    end
end

local function iter_spritesofstat(stat, i)
    if (i < 0) then
        i = ffiC.headspritestat[stat]
    else
        i = ffiC.nextspritestat[i]
    end

    if (i >= 0) then return i end
end

-- sprites of statnum iterator that allows deleting the iterated sprite
local function iter_spritesofstat_safe(tab, i)
    if (i < 0) then
        i = ffiC.headspritestat[-i]
    else
        i = tab[1]
    end

    if (i >= 0) then
        tab[1] = ffiC.nextspritestat[i]
        return i
    end
end

function spritesofstat(stat, maydelete)
    if (stat >= ffiC.MAXSTATUS+0ULL) then
        error("passed invalid statnum to spritesofstat iterator", 2)
    end

    if (maydelete) then
        return iter_spritesofstat_safe, { -1 }, -stat
    else
        return iter_spritesofstat, stat, -1
    end
end

--== TROR iterators ==--
local function iter_sectorsofbunch(cf, i)
    if (i < 0) then
        i = ffiC.headsectbunch[cf][-i-1];
    else
        i = ffiC.nextsectbunch[cf][i];
    end

    if (i >= 0) then return i end
end

function sectorsofbunch(bunchnum, cf)
    if (bunchnum >= ffiC.numyaxbunches+0ULL) then
        error("passed invalid bunchnum to sectorsofbunch iterator", 2)
    end
    if (not (cf == 0 or cf == 1)) then
        error("passed invalid 'cf' to sectorsofbunch iterator, must be 0 or 1", 2)
    end

    return iter_sectorsofbunch, cf, -bunchnum-1
end


---=== Engine functions, wrapped for Lua convenience ===---

-- returns a hitdata_ct
-- TODO: make v[xyz] be passed as one aggregate, too?
-- Additionally, permit different coordinates? (ang&horiz, ...)
function hitscan(pos, sectnum, vx,vy,vz, cliptype)
    check_sector_idx(sectnum)
    local vec = vec3_ct(pos.x, pos.y, pos.z)
    local hitdata = hitdata_ct()

    ffiC.hitscan(vec, sectnum, vx,vy,vz, hitdata, cliptype)
    return hitdata
end

function cansee(pos1,sect1, pos2,sect2)
    check_sector_idx(sect1)
    check_sector_idx(sect2)

    local ret = ffiC.cansee(pos1.x,pos1.y,pos1.z, sect1,
                            pos2.x,pos2.y,pos2.z, sect2)
    return (ret~=0)
end

local neartag_ret_ct = ffi.typeof[[
const struct {
    int32_t sector, wall, sprite;
    int32_t dist;
}
]]

local function newar() return ffi.new("int16_t [1]") end
-- NOTE: <tagsearch> flags are in sector.NEARTAG_FLAGS
function neartag(pos, sectnum, ang, range, tagsearch)
    check_sector_idx(sectnum)
    local a, b, c, d = newar(), newar(), newar(), ffi.new("int32_t [1]")
    ffiC.neartag(pos.x, pos.y, pos.z, sectnum, ang, a, b, c, d, range, tagsearch, nil)
    return neartag_ret_ct(a[0], b[0], c[0], d[0])
end

function inside(pos, sectnum)
    check_sector_idx(sectnum)
    return (ffiC.inside(pos.x, pos.y, sectnum)==1)
end

local us_retsect = ffi.new("int16_t [1]")
local USF = sector.UPDATE_FLAGS

function updatesector(pos, sectnum, flags)
    if (sectnum ~= -1) then
        check_sector_idx(sectnum)
    end

    us_retsect[0] = sectnum

    if (flags==nil or flags==0) then
        ffiC.updatesector(pos.x, pos.y, us_retsect)
    elseif (flags==USF.BREADTH) then
        ffiC.updatesectorbreadth(pos.x, pos.y, us_retsect)
    else
        error("invalid argument #3 (flags)", 2)
    end

    return us_retsect[0]
end

function updatesectorz(pos, sectnum, flags)
    if (sectnum ~= -1) then
        check_sector_idx(sectnum)
    end
    if (flags ~= nil) then
        error("invalid argument #3 (flags)", 2)
    end

    us_retsect[0] = sectnum
    ffiC.updatesectorz(pos.x, pos.y, pos.z, us_retsect)
    return us_retsect[0]
end

function printf(fmt, ...)
    print(string.format(fmt, ...))
end


-- This is supposed to be run from the file that 'require's this module to take
-- over the non-local variables from here into its global environment.
function create_globals(_G_their)
    local _G_our = getfenv(1)
    vars_to_ignore["create_globals"] = true

    for varname,obj in pairs(_G_our) do
        if (not vars_to_ignore[varname]) then
            if (ffiC._DEBUG_LUNATIC ~= 0) then
                print("EXPORT "..varname)
            end
            _G_their[varname] = obj
        end
    end
end
