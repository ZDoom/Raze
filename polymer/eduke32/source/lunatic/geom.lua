-- Geometry module for Lunatic.

local ffi = require("ffi")
local math = require("math")

local type = type
local assert = assert


module(...)


ffi.cdef[[
typedef struct { double x, y; } dvec2_t;
]]

local vec2_
local mt = {
    __add = function(a, b) return vec2_(a.x+b.x, a.y+b.y) end,
    __sub = function(a, b) return vec2_(a.x-b.x, a.y-b.y) end,
    __unm = function(a) return vec2_(-a.x, -b.x) end,

    __mul = function(a,b)
        if (type(a)=="number") then
            return vec2_(a*b.x, a*b.y)
        end

        assert(type(b)=="number")
        return vec2_(a.x*b, a.y*b)
    end,

    __div = function(a,b)
        assert(type(b)=="number")
        return vec2_(a.x/b, a.y/b)
    end,

    __eq = function(a,b)
        return (a.x==b.x and a.y==b.y)
    end,

    __len = function(a) return math.sqrt(a.x*a.x + a.y*a.y) end,

    __tostring = function(a) return "vec2("..a.x..", "..a.y..")" end,

    __index = {
        lensq = function(a) return a.x*a.x + a.y*a.y end,
    },
}

-- VEC2 user data constructor.
--  * vec2(<table or ctype>), the arg should be indexable with "x" and "y"
--  * vec2(x, y), assuming that x and y are numbers
vec2_ = ffi.metatype("dvec2_t", mt)
vec2 = vec2_

-- Returns a vec2 from anything indexable with "x" and "y"
-- (vec2(t) works if t is such a table, but not if it's a vec2)
function tovec2(t)
    return vec2(t.x, t.y)
end

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
