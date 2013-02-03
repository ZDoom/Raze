
-- This file contains LuaJIT definitions of stuff that's common to the game and
-- editor.  The 'decl' function is expected to be defined in the global
-- environment.
-- See the included license file "BUILDLIC.TXT" for license info.

local ffi = require("ffi")
local ffiC = ffi.C

ffi.cdef "enum { _DEBUG_LUNATIC=1 }"

local bit = require("bit")
local string = require("string")

local assert = assert
local error = error
local pairs = pairs
local require = require
local setmetatable = setmetatable
local tostring = tostring
local type = type

local decl = decl
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


--== Core engine structs ==--
local SECTOR_STRUCT = [[
struct {
    const int16_t wallptr, wallnum;
    int32_t ceilingz, floorz;
    uint16_t ceilingstat, floorstat;
    const int16_t ceilingpicnum;
    int16_t ceilingheinum;
    int8_t ceilingshade;
    uint8_t ceilingpal, ceilingxpanning, ceilingypanning;
    const int16_t floorpicnum;
    int16_t floorheinum;
    int8_t floorshade;
    uint8_t floorpal, floorxpanning, floorypanning;
    uint8_t visibility, filler;
    int16_t lotag, hitag, extra;
}]]

local SPRITE_STRUCT = [[
struct {
    int32_t x, y, z;
    uint16_t cstat;
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
    uint16_t cstat;
    const int16_t picnum, overpicnum;
    int8_t shade;
    uint8_t pal, xrepeat, yrepeat, xpanning, ypanning;
    int16_t lotag, hitag, extra;
}]]

-- Converts a template struct definition to an internal, unrestricted one.
function strip_const(structstr)
    return (string.gsub(structstr, "const ", ""));
end

-- NOTE for FFI definitions: we're compiling EDuke32 with -funsigned-char, so
-- we need to take care to declare chars as unsigned whenever it matters, for
-- example if it represents a palette index.  (I think it's harmless for stuff
-- like passing a function argument, but it should be done there for clarity.)

-- TODO: provide getters for unsigned {hi,lo}tag?
ffi.cdef([[
#pragma pack(push,1)
typedef $ sectortype;
typedef $ walltype;
typedef $ spritetype;
typedef $ tspritetype;

typedef struct {
    const uint32_t mdanimtims;
    const int16_t mdanimcur;
    int16_t angoff, pitch, roll;
    // TODO: make into an ivec3_t
    int32_t xoff, yoff, zoff;
    uint8_t flags;
    uint8_t xpanning, ypanning;
    const uint8_t filler;
    float alpha;
    union {
        const intptr_t _tspr;
        struct { const int32_t _dummy0, _dummy1; };
    };
} spriteext_t;

typedef struct {
    int32_t x, y, z;
} vec3_t;

typedef struct {
    vec3_t pos;
    int16_t sprite, wall, sect;
} hitdata_t;
#pragma pack(pop)
]],
ffi.typeof(SECTOR_STRUCT), ffi.typeof(WALL_STRUCT),
ffi.typeof(SPRITE_STRUCT), ffi.typeof(SPRITE_STRUCT))
-- NOTE: spritetype and tspritetype are different types with the same layout.
-- (XXX: is there a better way?)

-- Define the "palette_t" type, which for us has .{r,g,b} fields and a
-- bound-checking array of length 3 overlaid.
-- TODO: bcarray really should allow to simply declare the struct with
-- passed member names instead of "hidden" ones... because wrapping it
-- in a union like this is doing things inside-out really.
local rgbarray_t = require("bcarray").new("uint8_t", 3, "RGB array")
ffi.cdef("typedef union { \
    struct { uint8_t r, g, b, f; }; \
    $ col; \
} palette_t", rgbarray_t)
assert(ffi.alignof("palette_t")==1)

local vec3_ct = ffi.typeof("vec3_t")
local hitdata_ct = ffi.typeof("hitdata_t")

decl[[const int32_t engine_main_arrays_are_static, engine_v8;]]


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

    MAXBUNCHES = 256,
    CEILING = 0,
    FLOOR = 1,

    CLIPMASK0 = (1<<16)+1,  // blocking
    CLIPMASK1 = (256<<16)+64,  // hittable
};
]]

ffi.cdef[[
const int16_t numsectors, numwalls;
const int32_t numyaxbunches;  // XXX
const int32_t totalclock;
int32_t randomseed;  // DEPRECATED
const int32_t xdim, ydim;
const int32_t windowx1, windowy1, windowx2, windowy2;
]]

decl[[
int32_t spritesortcnt;
const int32_t rendmode;
const int16_t headspritesect[MAXSECTORS+1], headspritestat[MAXSTATUS+1];
const int16_t prevspritesect[MAXSPRITES], prevspritestat[MAXSPRITES];
const int16_t nextspritesect[MAXSPRITES], nextspritestat[MAXSPRITES];

const int16_t headsectbunch[2][MAXBUNCHES], nextsectbunch[2][MAXSECTORS];

int16_t yax_getbunch(int16_t i, int16_t cf);

int32_t   getceilzofslopeptr(const sectortype *sec, int32_t dax, int32_t day);
int32_t   getflorzofslopeptr(const sectortype *sec, int32_t dax, int32_t day);
void   getzsofslopeptr(const sectortype *sec, int32_t dax, int32_t day,
                       int32_t *ceilz, int32_t *florz);

int32_t hitscan(const vec3_t *sv, int16_t sectnum, int32_t vx, int32_t vy, int32_t vz,
                hitdata_t *hitinfo, uint32_t cliptype);
int32_t cansee(int32_t x1, int32_t y1, int32_t z1, int16_t sect1,
               int32_t x2, int32_t y2, int32_t z2, int16_t sect2);
void neartag(int32_t xs, int32_t ys, int32_t zs, int16_t sectnum, int16_t ange, int16_t *neartagsector, int16_t *neartagwall,
             int16_t *neartagsprite, int32_t *neartaghitdist, int32_t neartagrange, uint8_t tagsearch,
             int32_t (*blacklist_sprite_func)(int32_t));

int32_t ldist(const spritetype *s1, const spritetype *s2);
int32_t dist(const spritetype *s1, const spritetype *s2);

int32_t inside(int32_t x, int32_t y, int16_t sectnum);
void updatesector(int32_t x, int32_t y, int16_t *sectnum);
void updatesectorbreadth(int32_t x, int32_t y, int16_t *sectnum);
void updatesectorz(int32_t x, int32_t y, int32_t z, int16_t *sectnum);

void rotatesprite(int32_t sx, int32_t sy, int32_t z, int16_t a, int16_t picnum,
                  int8_t dashade, unsigned char dapalnum, int32_t dastat,
                  int32_t cx1, int32_t cy1, int32_t cx2, int32_t cy2);
]]

-- misc. functions
ffi.cdef[[
double gethitickms(void);

int32_t krand(void);
int32_t ksqrt(uint32_t num);
int32_t __fastcall getangle(int32_t xvect, int32_t yvect);
]]

local bcheck = require("bcheck")
local check_sector_idx = bcheck.sector_idx
local check_sprite_idx = bcheck.sprite_idx
local check_tile_idx = bcheck.tile_idx

local ivec3_
local ivec3_mt = {
    -- '^' is the "translate upwards" operator
    __pow = function(v, zofs)
        return ivec3_(v.x, v.y, v.z-zofs)
    end,
}
ivec3_ = ffi.metatype(vec3_ct, ivec3_mt)

local xor = bit.bxor
local wallsofsec  -- fwd-decl

local sectortype_ptr_ct = ffi.typeof("$ *", ffi.typeof(strip_const(SECTOR_STRUCT)))

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

        -- inside() port
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
            -- XXX: this disallows making a red wall white
            bcheck.wall_idx(nextwall)
            ffi.cast(walltype_ptr_ct, w).nextwall = nextwall
        end,

        _set_nextsector = function(w, nextsector)
            -- XXX: this disallows making a red wall white
            check_sector_idx(nextsector)
            ffi.cast(walltype_ptr_ct, w).nextsector = nextsector
        end,

        --- Predicates
        isblocking = function(w)
            return (bit.band(w.cstat, 1)~=0)
        end,

        ismasked = function(w)
            return (bit.band(w.cstat, 16)~=0)
        end,

        isoneway = function(w)
            return (bit.band(w.cstat, 32)~=0)
        end,

        ishittable = function(w)
            return (bit.band(w.cstat, 64)~=0)
        end,
    }
}
ffi.metatype("walltype", walltype_mt)

local spriteext_mt = {
    __index = {
        -- Enable EVENT_ANIMATESPRITES for this sprite.
        make_animated = function(sx)
            sx.flags = bit.bor(sx.flags, 16)
        end,
    },
}
ffi.metatype("spriteext_t", spriteext_mt)

local spritetype_ptr_ct = ffi.typeof("$ *", ffi.typeof(strip_const(SPRITE_STRUCT)))

local spritetype_mt = {
    __pow = function(s, zofs)
        return ivec3_(s.x, s.y, s.z-zofs)
    end,

    __index = {
        --- Setters
        set_picnum = function(s, tilenum)
            check_tile_idx(tilenum)
            ffi.cast(spritetype_ptr_ct, s).picnum = tilenum
        end,

        set_yvel = function(s, yvel)
            -- XXX: for now, no checking
            ffi.cast(spritetype_ptr_ct, s).yvel = yvel
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

-- Methods that are specific to tsprites
-- XXX: doesn't work with LuaJIT git f772bed34b39448e3a9ab8d07f6d5c0c26300e1b
function tspritetype_mt.__index.dup(tspr)
    if (ffiC.spritesortcnt >= ffiC.MAXSPRITESONSCREEN+0ULL) then
        return nil
    end

    local newtspr = ffi.tsprite[ffiC.spritesortcnt]
    ffi.copy(newtspr, tspr, ffi.sizeof(tspr))
    ffiC.spritesortcnt = ffiC.spritesortcnt+1

    return newtspr
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
local sector_mt = {
    __index = function(tab, key)
        if (key >= 0 and key < ffiC.numsectors) then return ffiC.sector[key] end
        error('out-of-bounds sector[] read access', 2)
    end,

    __newindex = function() error('cannot write directly to sector[]', 2) end,
}

local wall_mt = {
    __index = function(tab, key)
        if (key >= 0 and key < ffiC.numwalls) then return ffiC.wall[key] end
        error('out-of-bounds wall[] read access', 2)
    end,

    __newindex = function() error('cannot write directly to wall[]', 2) end,
}

local atsprite_mt = {
    __index = function(tab, idx)
        check_sprite_idx(idx)

        local tspr = ffi.cast(spritetype_ptr_ct, ffiC.spriteext[idx]._tspr)
        if (tspr == nil) then
            error("tsprite of actor "..idx.." unavailable", 2)
        end

        -- Return a reference to a tsprite[] element.
        return tspr[0]
    end,

    __newindex = function() error('cannot write directly to atsprite[]', 2) end,
}

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
sprite = creategtab(ffiC.sprite, ffiC.MAXSPRITES, 'sprite[]')
spriteext = creategtab(ffiC.spriteext, ffiC.MAXSPRITES, 'spriteext[]')
atsprite = setmtonce({}, atsprite_mt)

headspritesect = creategtab(ffiC.headspritesect, ffiC.MAXSECTORS, 'headspritesect[]')
-- TODO: allow sprite freelist access via the status list for CON compatibility?
headspritestat = creategtab(ffiC.headspritestat, ffiC.MAXSTATUS, 'headspritestat[]')
nextspritesect = creategtab(ffiC.nextspritesect, ffiC.MAXSPRITES, 'nextspritesect[]')
nextspritestat = creategtab(ffiC.nextspritestat, ffiC.MAXSPRITES, 'nextspritestat[]')
prevspritesect = creategtab(ffiC.prevspritesect, ffiC.MAXSPRITES, 'prevspritesect[]')
prevspritestat = creategtab(ffiC.prevspritestat, ffiC.MAXSPRITES, 'prevspritestat[]')

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

function spritesofsect(sect)
    check_sector_idx(sect)
    return iter_spritesofsect, sect, -1
end

local function iter_spritesofstat(stat, i)
    if (i < 0) then
        i = ffiC.headspritestat[stat]
    else
        i = ffiC.nextspritestat[i]
    end

    if (i >= 0) then return i end
end

function spritesofstat(stat)
    if (stat >= ffiC.MAXSTATUS+0ULL) then
        error("passed invalid statnum to spritesofstat iterator", 2)
    end

    return iter_spritesofstat, stat, -1
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

function getbunch(sectnum, cf)
    check_sector_idx(sectnum)
    if (not (cf == 0 or cf == 1)) then
        error("passed invalid 'cf' to getbunch, must be 0 or 1", 2)
    end

    return ffiC.yax_getbunch(sectnum, cf)
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
    if (sect1 >= ffiC.numsectors+0ULL) then
        error("passed out-of-bounds first sector number "..sect1, 2)
    end
    if (sect2 >= ffiC.numsectors+0ULL) then
        error("passed out-of-bounds second sector number "..sect2, 2)
    end

    local ret = ffiC.cansee(pos1.x,pos1.y,pos1.z, sect1,
                            pos2.x,pos2.y,pos2.z, sect2)
    return (ret~=0)
end

ffi.cdef[[
typedef struct {
    int32_t sector, wall, sprite;
    int32_t dist;
} neartag_ret_t;
]]
local neartag_ret_ct = ffi.typeof("const neartag_ret_t")

-- TODO: make tagsearch something more convenient
function neartag(pos, sectnum, ang, range, tagsearch)
    check_sector_idx(sectnum)
    local newar = function() return ffi.new("int16_t [1]") end
    local a, b, c, d = newar(), newar(), newar(), ffi.new("int32_t [1]")
    ffiC.neartag(pos.x, pos.y, pos.z, sectnum, ang, a, b, c, d, range, tagsearch, nil)
    return neartag_ret_ct(a[0], b[0], c[0], d[0])
end

function inside(pos, sectnum)
    check_sector_idx(sectnum)
    return (ffiC.inside(pos.x, pos.y, sectnum)==1)
end

-- TODO: should these rather be one function, and the specific kind of updating
-- controlled by an argument?
function updatesector(pos, sectnum)
    local sect = ffi.new("int16_t [1]")
    sect[0] = sectnum
    ffiC.updatesector(pos.x, pos.y, sect)
    return sect[0]
end

function updatesectorbreadth(pos, sectnum)
    local sect = ffi.new("int16_t [1]")
    sect[0] = sectnum
    ffiC.updatesectorbreadth(pos.x, pos.y, sect)
    return sect[0]
end

function updatesectorz(pos, sectnum)
    local sect = ffi.new("int16_t [1]")
    sect[0] = sectnum
    ffiC.updatesectorz(pos.x, pos.y, pos.z, sect)
    return sect[0]
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
