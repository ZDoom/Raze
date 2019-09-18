-- "Extended" math module for Lunatic.

local ffi = require("ffi")

local bit = require("bit")
local math = require("math")

local arshift = bit.arshift
local abs, sqrt = math.abs, math.sqrt

local assert = assert
local error = error
local type = type

local OUR_REQUIRE_STRING = [[
 local _xm=require'xmath'
 local _v,_iv=_xm.vec3,_xm.ivec3
]]
local function our_get_require()
    return OUR_REQUIRE_STRING
end


module(...)


---=== TRIGONOMETRY ===---

local BANG2RAD = math.pi/1024
local isintab = ffi.new("int16_t [?]", 2048)
local dsintab = ffi.new("double [?]", 2048)

for a=0,511 do
    local s = math.sin(a*BANG2RAD)
    isintab[a] = 16384*s
    dsintab[a] = s
end

isintab[512] = 16384
dsintab[512] = 1

for i=513,1023 do
    isintab[i] = isintab[1024-i];
    dsintab[i] = dsintab[1024-i];
end

for i=1024,2047 do
    isintab[i] = -isintab[i-1024];
    dsintab[i] = -dsintab[i-1024];
end


local band = bit.band

local function ksc_common(ang)
    ang = band(ang, 2047)
    assert(ang >= 0 and ang < 2048)  -- might have been passed NaN
    return ang
end

-- k{sin,cos}: 16384-scaled output, 2048-based angle input
function ksin(ang)
    return isintab[ksc_common(ang)]
end

function kcos(ang)
    return isintab[ksc_common(ang+512)]
end


local sin, cos = math.sin, math.cos

-- {sin,cos}b: [-1..1] output, 2048-based angle input
function sinb(ang)
    return dsintab[ksc_common(ang)]
end

function cosb(ang)
    return dsintab[ksc_common(ang+512)]
end

local cosb, sinb = cosb, sinb


---=== Approximations to 2D and 3D Euclidean distances ===---
-- (also see common.c)

local function dist_common(pos1, pos2)
    local x = abs(pos1.x - pos2.x)
    local y = abs(pos1.y - pos2.y)
    if (x < y) then
        x, y = y, x
    end
    return x, y
end

function ldist(pos1, pos2)
    local x, y = dist_common(pos1, pos2)

    local t = y + arshift(y,1)
    return x - arshift(x,5) - arshift(x,7) + arshift(t,2) + arshift(t,6)
end

function dist(pos1, pos2)
    local x, y = dist_common(pos1, pos2)
    local z = abs(arshift(pos1.z - pos2.z, 4))

    if (x < z) then
        x, z = z, x
    end

    local t = y + z
    return x - arshift(x,4) + arshift(t,2) + arshift(t,3)
end


---=== VECTOR TYPES ===---


-- The integer 3-vector can be useful for calculations expecting integer
-- values, e.g. ivec3(x, y, z) is a reasonable way to round a vec3. It can also
-- be used as the RHS to the vec2/vec3 arithmetic methods.
-- NOTE: We must have a typedef with that exact name, because for Lunatic
-- (i.e. not stand-alone), the type was already declared in defs_common.lua.
ffi.cdef "typedef struct { int32_t x, y, z; } vec3_t;"
local ivec3_t = ffi.typeof("vec3_t")


local dvec2_t = ffi.typeof("struct { double x, y; }")
local dvec3_t = ffi.typeof("struct { double x, y, z; }")

local vec2_mt = {
    __add = function(a, b) return dvec2_t(a.x+b.x, a.y+b.y) end,
    __sub = function(a, b) return dvec2_t(a.x-b.x, a.y-b.y) end,
    __unm = function(a) return dvec2_t(-a.x, -a.y) end,

    __mul = function(a,b)
        if (type(a)=="number") then
            return dvec2_t(a*b.x, a*b.y)
        end

        if (type(b)~="number") then
            error("number expected in vec2 multiplication", 2)
        end
        return dvec2_t(a.x*b, a.y*b)
    end,

    __div = function(a,b)
        if (type(b)~="number") then
            error("number expected in vec2 division", 2)
        end
        return dvec2_t(a.x/b, a.y/b)
    end,

    __tostring = function(a) return "vec2("..a.x..", "..a.y..")" end,

    __index = {
        lensq = function(a) return a.x*a.x + a.y*a.y end,

        mhlen = function(a) return abs(a.x)+abs(a.y) end,
    },
}

local l_rotate  -- fwd-decl (XXX: could be the other way around)

-- The vec3 metatable is shared between the integer- and double-based 3-vector
-- types. However, some operations are slightly different.
local vec3_mt = {
    -- Arithmetic operations. Note that they always return a dvec3.
    __add = function(a, b) return dvec3_t(a.x+b.x, a.y+b.y, a.z+b.z) end,
    __sub = function(a, b) return dvec3_t(a.x-b.x, a.y-b.y, a.z-b.z) end,
    __unm = function(a) return dvec3_t(-a.x, -a.y, -a.z) end,

    __mul = function(a,b)
        if (type(a)=="number") then
            return dvec3_t(a*b.x, a*b.y, a*b.z)
        end

        if (type(b)~="number") then
            error("number expected in vec3 multiplication", 2)
        end
        return dvec3_t(a.x*b, a.y*b, a.z*b)
    end,

    __div = function(a,b)
        if (type(b)~="number") then
            error("number expected in vec3 division", 2)
        end
        return dvec3_t(a.x/b, a.y/b, a.z/b)
    end,

    -- '^' is the "translate upwards" operator, returns same-typed vector.
    __pow = function(v, zofs)
        return v:_ctor(v.x, v.y, v.z-zofs)
    end,

    -- Convenience for human-readable display.
    __tostring = function(a)
        return (a:_isi() and "i" or "").."vec3("..a.x..", "..a.y..", "..a.z..")"
    end,

    __index = {
        -- Euclidean 3D length.
        len = function(a) return sqrt(a.x*a.x + a.y*a.y + a.z*a.z) end,
        -- Euclidean 3D squared length.
        lensq = function(a) return a.x*a.x + a.y*a.y + a.z*a.z end,

        -- Euclidean 2D length.
        len2 = function(a) return sqrt(a.x*a.x + a.y*a.y) end,
        -- Euclidean 2D squared length.
        len2sq = function(a) return a.x*a.x + a.y*a.y end,

        -- Manhattan-distance 3D length:
        mhlen = function(a) return abs(a.x)+abs(a.y)+abs(a.z) end,

        toivec3 = function(v) return ivec3_t(v.x, v.y, v.z) end,

        -- BUILD-coordinate (z scaled by 16) <-> uniform conversions.
        touniform = function(v)
            return v:_isi()
                and v:_ctor(v.x, v.y, arshift(v.z, 4))
                or v:_ctor(v.x, v.y, v.z/16)
        end,

        tobuild = function(v) return v:_ctor(v.x, v.y, 16*v.z) end,

        rotate = function(v, ang, pivot) return l_rotate(v, ang, pivot) end,

        -- PRIVATE methods --

        -- Get the type constructor for this vector.
        _ctor = function(v, ...)
            return v:_isi() and ivec3_t(...) or dvec3_t(...)
        end,
        -- Is <v> integer vec3? INTERNAL.
        _isi = function(v)
            return ffi.istype(ivec3_t, v)
        end,

        --- Serialization ---
        _get_require = our_get_require,

        _serialize = function(v)
            return (v:_isi() and "_iv" or "_v").."("..v.x..","..v.y..","..v.z..")"
        end,
    },
}

ffi.metatype(dvec2_t, vec2_mt)
ffi.metatype(dvec3_t, vec3_mt)
ffi.metatype(ivec3_t, vec3_mt)

-- VEC2 user data constructor.
--  * vec2([x [, y]]), assuming that x and y are numbers. Vacant positions are
--    assumed to be 0.
--  * vec2(<compound>), <compound> can be anything indexable with "x" and "y"
function vec2(...)
    local x, y = ...
    if (type(x)=="number" or x==nil) then
        return dvec2_t(...)
    else
        return dvec2_t(x.x, x.y)
    end
end

-- VEC3 user data constructor.
--  Analogous to VEC2.
function vec3(...)
    local x, y, z = ...
    if (type(x)=="number" or x==nil) then
        return dvec3_t(...)
    else
        return dvec3_t(x.x, x.y, x.z)
    end
end

-- IVEC3 user data constructor.
function ivec3(...)
    local x, y, z = ...
    if (type(x)=="number" or x==nil) then
        return ivec3_t(...)
    else
        return ivec3_t(x.x, x.y, x.z)
    end
end

local vec2, vec3 = vec2, vec3


---=== MISCELLANEOUS MATH ===---

local intarg = ffi.new("int32_t [1]")
function bangvec(bang)
    intarg[0] = bang  -- round towards zero
    return dvec3_t(cosb(intarg[0]), sinb(intarg[0]))
end

function kangvec(bang, z)
    intarg[0] = bang  -- round towards zero
    return ivec3_t(kcos(intarg[0]), ksin(intarg[0]), z or 0)
end

function angvec(ang)
    return dvec3_t(cos(ang), sin(ang))
end


local zerovec = vec3()
-- Point rotation. Note the different order of arguments from engine function.
-- XXX: passing mixed vec2/vec3 is problematic. Get rid of vec2?
-- <ang>: BUILD angle (0-2047 based)
function rotate(pos, ang, pivot)
    pivot = pivot or zerovec
    local p = vec3(pos)-pivot
    local c, s = cosb(ang), sinb(ang)
    local x, y = p.x, p.y
    p.x = pivot.x + (c*x - s*y)
    p.y = pivot.y + (c*y + s*x)
    return p
end

l_rotate = rotate


-- Two-element vector cross product.
-- Anti-commutative, distributive.
local function cross2(v, w)
    return v.y*w.x - v.x*w.y
end


-- Finds the intersection point of two lines given by
-- point a and vector v
--  and
-- point b and vector w
--
-- Returns:
--  if <TODO>, nil
--  if retpoint_p evaluates to a non-true value, coefficients cv and cw such that <TODO>
--  else, the intersection point
function intersect(a,v, b,w, retpoint_p)
    local vxw = cross2(v,w)

    if (vxw ~= 0) then
        local btoa = vec2(a) - vec2(b)
        local cv, cw = cross2(w, btoa)/vxw, cross2(v, btoa)/vxw

        if (retpoint_p) then
            return vec2(a) + cv*vec2(v)
        else
            return cv, cw
        end
    end

    -- return nil if v and w parallel (or either of them is a point), or if
    -- they contain NaNs
end
