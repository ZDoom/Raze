-- Pseudo random number generation module for Lunatic

local ffi = require("ffi")
local ffiC = ffi.C

local rawset = rawset

local type = type
local gv = gv_tmp  -- temporarily set in defs.c

local print = print  -- for commented out debug block in new() below

module(...)


-- NOTE: PRNG state struct and functions are declared in defs.ilua

ffi.cdef[[
typedef union { unsigned char u[16]; double d[2]; } uchar_double_u_t;
typedef union { unsigned char u[16]; uint32_t i[4]; } uchar_uint_u_t;
]]

local mt = {
    __tostring = function(s)
        return "rand.new("..s.x..","..s.y..","..s.z..","..s.c..")"
    end,

    __index = {
        getu32 = ffiC.rand_jkiss_u32,
        getdbl = ffiC.rand_jkiss_dbl,

        -- Initialize the JKISS PRNG using the MD4 of the lower bits of the
        -- profiling timer
        init_time_md4 = function(s)
            local tin = ffi.new("uchar_double_u_t")
            local tout = ffi.new("uchar_uint_u_t")

            repeat
                tin.d[0] = gv.gethitickms() % 1
                tin.d[1] = gv.gethitickms() % 1

                ffiC.md4once(tin.u, 16, tout.u)

                s.y = tout.u[1]
            until (s.y ~= 0)  -- y must not be zero!

            s.x = tout.u[0]
            s.z = tout.u[2]
            s.c = tout.u[3] % 698769068 + 1  -- Should be less than 698769069
        end,
    },
}
local jkiss = ffi.metatype("rng_jkiss_t", mt)

function new(x,y,z,c)
    local s
    if (x == nil or type(x)=="boolean") then
        -- initialize with arbitrary but fixed state
        s = jkiss(123456789, 987654321, 43219876, 6543217)
        if (x) then
            s:init_time_md4()
        end
    else
        s = jkiss(x,y,z,c)
    end
--[[
    print("TEST")
    local r=ffi.new("rng_jkiss_t")
    r.x = 123456789; r.y = 987654321; r.z = 43219876; r.c = 6543217;

    t=gv.gethitickms()
    for i=1,4*2*1e6 do
        ffiC.rand_jkiss_dbl(r)
    end
    print("TIME: "..gv.gethitickms()-t)  -- x86_64: approx. 100 ms
--]]

    return s
end
