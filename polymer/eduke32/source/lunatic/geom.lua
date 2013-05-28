-- Geometry module for Lunatic.

local require = require
local ffi = require("ffi")
local math = require("math")

local type = type
local error = error


module(...)


-- The integer 3-vector can be useful for calculations expecting integer
-- values, e.g. geom.ivec3(x, y, z) is a reasonable way to round a vec3. It can
-- also be used as the RHS to the vec2/vec3 arithmetic methods.
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

    __len = function(a) return math.sqrt(a.x*a.x + a.y*a.y) end,

    __tostring = function(a) return "vec2("..a.x..", "..a.y..")" end,

    __index = {
        lensq = function(a) return a.x*a.x + a.y*a.y end,

        mhlen = function(a) return math.abs(a.x)+math.abs(a.y) end,
    },
}

local arshift = require("bit").arshift

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
        return v(v.x, v.y, v.z-zofs)
    end,

    -- The # operator returns the Euclidean length.
    -- TODO: REMOVE.
    __len = function(a) return math.sqrt(a.x*a.x + a.y*a.y + a.z*a.z) end,

    -- INTERNAL: Calling a vector calls the constructor of its type.
    __call = function(v, ...)
        return v:_isi() and ivec3_t(...) or dvec3_t(...)
    end,

    -- INTERNAL
    __tostring = function(a)
        return (a:_isi() and "i" or "").."vec3("..a.x..", "..a.y..", "..a.z..")"
    end,

    __index = {
        -- Euclidean 3D length.
        len = function(a) return math.sqrt(a.x*a.x + a.y*a.y + a.z*a.z) end,
        -- Euclidean 3D squared length.
        lensq = function(a) return a.x*a.x + a.y*a.y + a.z*a.z end,

        -- Euclidean 2D length.
        len2 = function(a) return math.sqrt(a.x*a.x + a.y*a.y) end,
        -- Euclidean 2D squared length.
        len2sq = function(a) return a.x*a.x + a.y*a.y end,

        -- Manhattan-distance 3D length:
        mhlen = function(a) return math.abs(a.x)+math.abs(a.y)+math.abs(a.z) end,

        toivec3 = function(v) return ivec3_t(v.x, v.y, v.z) end,

        -- BUILD-coordinate (z scaled by 16) <-> uniform conversions.
        touniform = function(v)
            return v:_isi()
                and v(v.x, v.y, arshift(v.z, 4))
                or v(v.x, v.y, v.z/4)
        end,

        tobuild = function(v) return v(v.x, v.y, 16*v.z) end,

        -- Is <v> integer vec3? INTERNAL.
        _isi = function(v)
            return ffi.istype(ivec3_t, v)
        end,
    },
}

-- VEC2 user data constructor.
--  * vec2(<table>), <table> should be indexable with "x" and "y"
--  * vec2(x, y), assuming that x and y are numbers
vec2 = ffi.metatype(dvec2_t, vec2_mt)
vec3 = ffi.metatype(dvec3_t, vec3_mt)

ivec3 = ffi.metatype("vec3_t", vec3_mt)

-- Returns a vec2 from anything indexable with "x" and "y"
-- (vec2(t) works if t is such a table, but not if it's a vec2 or a cdata of
-- different type)
function tovec2(t) return dvec2_t(t.x, t.y) end
function tovec3(t) return dvec3_t(t.x, t.y, t.z) end


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
        local btoa = tovec2(a)-tovec2(b)
        local cv, cw = cross2(w, btoa)/vxw, cross2(v, btoa)/vxw

        if (retpoint_p) then
            return tovec2(a)+cv*tovec2(v)
        else
            return cv, cw
        end
    end

    -- return nil if v and w parallel (or either of them is a point), or if
    -- they contain NaNs
end
