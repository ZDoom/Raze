-- Statistics module for Lunatic.

local ffi = require("ffi")

local math = require("math")
local string = require("string")

local tostring = tostring


module(...)


ffi.cdef[[
typedef struct {
    double n;
    // vvv internal vvv
    double m, s;
    double min, max;
} runningstat_t;

typedef struct {
    const double n;
    const double mean, var, std;
    const double min, max;
} runningstat_res_t;
]]


local NaN = 0/0

local res_mt = {
    __tostring = function(s)
        return
            string.format("N=%d; mean=%.5g, std=%.5g; min=%.5g, max=%.5g",
                         s.n, s.mean, s.std, s.min, s.max)
    end
}
local rstatres = ffi.metatype("runningstat_res_t", res_mt)


local mt = {
    __tostring = function(s)
        -- XXX: with this and the other serializing of ffi types, take care of
        -- NaN and Infs when reading back (e.g. by "nan=0/0" in that context)
        return "stat.new("..s.n..","..s.m..","..s.s..","..s.min..","..s.max..")"
    end,

    -- See: Accurately computing running variance, by John D. Cook
    -- http://www.johndcook.com/standard_deviation.html
    __index = {
        add = function(s, num)
            if (s.n > 0) then
                -- N>0, recurrence
                s.n = s.n+1

                local lastm = s.m
                s.m = lastm + (num-lastm)/s.n
                s.s = s.s + (num-lastm)*(num-s.m)

                if (num < s.min) then s.min = num end
                if (num > s.max) then s.max = num end
            else
                -- N==0, initialization
                s.n = 1

                s.m = num
                s.s = 0

                s.min = num
                s.max = num
            end
        end,

        getstats = function(s)
            local var = s.n > 1 and s.s/(s.n-1) or NaN
            return rstatres(s.n, s.m, var, math.sqrt(var), s.min, s.max)
        end,

        getstatstr = function(s)
            return tostring(s:getstats())
        end,

        reset = function(s)
            s.n = 0
            s.m, s.s, s.min, s.max = NaN, NaN, NaN, NaN
        end,
    },
}
local rstat = ffi.metatype("runningstat_t", mt)

function new(n,m,s, min,max)
    if (n == nil) then
        -- initialization containing no elements
        return rstat(0, NaN, NaN, NaN, NaN)
    elseif (m == nil) then
        -- same as initialization with N==0 above (one element)
        return rstat(1, n, 0, n, n)
    else
        -- generic initialization (internal use only)
        return rstat(n,m,s, min,max)
    end
end
