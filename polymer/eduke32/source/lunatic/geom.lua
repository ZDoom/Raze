-- Geometry module for Lunatic.

local ffi = require("ffi")
local math = require("math")

local type = type
local error = error


module(...)


-- This has no metamethods, but can be useful for calculations expecting
-- integer values, e.g. geom.ivec3(x, y, z) is a reasonable way to round
-- a vec3.  It can be also used as the RHS to the vec2/vec3 arithmetic
-- methods.
-- NOTE: We must have a typedef with that exact name, because for
-- Lunatic (i.e. not stand-alone), it is a duplicate (and ignored)
-- declaration for an already metatype'd type.
ffi.cdef "typedef struct { int32_t x, y, z; } vec3_t;"
ivec3 = ffi.typeof("vec3_t")


local dvec2_t = ffi.typeof("struct { double x, y; }")
local dvec3_t = ffi.typeof("struct { double x, y, z; }")


local vec2_
local vec2_mt = {
    __add = function(a, b) return vec2_(a.x+b.x, a.y+b.y) end,
    __sub = function(a, b) return vec2_(a.x-b.x, a.y-b.y) end,
    __unm = function(a) return vec2_(-a.x, -a.y) end,

    __mul = function(a,b)
        if (type(a)=="number") then
            return vec2_(a*b.x, a*b.y)
        end

        if (type(b)~="number") then
            error("number expected in vec2 multiplication", 2)
        end
        return vec2_(a.x*b, a.y*b)
    end,

    __div = function(a,b)
        if (type(b)~="number") then
            error("number expected in vec2 division", 2)
        end
        return vec2_(a.x/b, a.y/b)
    end,

    __eq = function(a,b)
        -- XXX: will error if <a> is not a ctype (can only happen if __eq was
        -- called by <b>)
        return (ffi.istype(a,b) and a.x==b.x and a.y==b.y)
    end,

    __len = function(a) return math.sqrt(a.x*a.x + a.y*a.y) end,

    __tostring = function(a) return "vec2("..a.x..", "..a.y..")" end,

    __index = {
        lensq = function(a) return a.x*a.x + a.y*a.y end,
        -- Manhattan distance:
        len1 = function(a) return math.abs(a.x)+math.abs(a.y) end,
    },
}

local vec3_
local vec3_mt = {
    __add = function(a, b) return vec3_(a.x+b.x, a.y+b.y, a.z+b.z) end,
    __sub = function(a, b) return vec3_(a.x-b.x, a.y-b.y, a.z-b.z) end,
    __unm = function(a) return vec3_(-a.x, -a.y, -a.z) end,

    __mul = function(a,b)
        if (type(a)=="number") then
            return vec3_(a*b.x, a*b.y, a*b.z)
        end

        if (type(b)~="number") then
            error("number expected in vec3 multiplication", 2)
        end
        return vec2_(a.x*b, a.y*b, a.z*b)
    end,

    __div = function(a,b)
        if (type(b)~="number") then
            error("number expected in vec3 division", 2)
        end
        return vec2_(a.x/b, a.y/b, a.z/b)
    end,

    __eq = function(a,b)
        -- XXX: see vec2
        return (ffi.istype(a,b) and a.x==b.x and a.y==b.y and a.z==b.z)
    end,

    __len = function(a) return math.sqrt(a.x*a.x + a.y*a.y + a.z*a.z) end,

    __tostring = function(a) return "vec3("..a.x..", "..a.y..", "..a.z..")" end,

    __index = {
        lensq = function(a) return a.x*a.x + a.y*a.y + a.z*a.z end,
        -- Manhattan distance:
        len1 = function(a) return math.abs(a.x)+math.abs(a.y)+math.abs(a.z) end,

        toivec3 = function(v)
            return ivec3(v.x, v.y, v.z)
        end,
    },
}

-- VEC2 user data constructor.
--  * vec2(<table>), <table> should be indexable with "x" and "y"
--  * vec2(x, y), assuming that x and y are numbers
vec2_ = ffi.metatype(dvec2_t, vec2_mt)
vec2 = vec2_

-- Returns a vec2 from anything indexable with "x" and "y"
-- (vec2(t) works if t is such a table, but not if it's a vec2 or a cdata of
-- different type)
function tovec2(t) return vec2(t.x, t.y) end

-- Same for vec3
vec3_ = ffi.metatype(dvec3_t, vec3_mt)
vec3 = vec3_
function tovec3(t) return vec3(t.x, t.y, t.z) end


-- Two-element vector cross product.
-- Anti-commutative, distributive.
function cross2(v, w)
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
