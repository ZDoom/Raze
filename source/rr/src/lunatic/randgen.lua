-- Pseudo random number generation module for Lunatic

local ffi = require("ffi")
local ffiC = ffi.C

local rawset = rawset

local type = type
local decl = decl  -- comes from above (_defs_game.lua)

local print = print  -- for commented out debug block in new() below


module(...)


-- PRNG state struct
ffi.cdef[[
typedef struct {
    uint32_t x, y, z, c;
} rng_jkiss_t;

typedef union { unsigned char u[16]; double d[2]; } uchar_double_u_t;
]]

-- PRNG functions
decl[[
uint32_t rand_jkiss_u32(rng_jkiss_t *s);
double rand_jkiss_dbl(rng_jkiss_t *s);

uint32_t Bcrc32(const void* data, size_t length, uint32_t crc);
]]

local function get_rand_u32(tin)
    tin.d[0] = ffiC.gethiticks() % 1
    tin.d[1] = ffiC.gethiticks() % 1
    return ffiC.Bcrc32(tin.u, 16, 0)
end

local mt = {
    __tostring = function(s)
        return "rand.new("..s.x..","..s.y..","..s.z..","..s.c..")"
    end,

    __index = {
        getu32 = ffiC.rand_jkiss_u32,
        getdbl = ffiC.rand_jkiss_dbl,

        -- Initialize the JKISS PRNG using the CRC32 of the result of several
        -- profiling timer calls interpreted as byte sequence.
        init_time_md4 = function(s)
            local tin = ffi.new("uchar_double_u_t")
            local tout = ffi.new("uint32_t [4]")

            repeat
                s.y = get_rand_u32(tin)
            until (s.y ~= 0)  -- y must not be zero!

            s.x = get_rand_u32(tin)
            s.z = get_rand_u32(tin)
            s.c = get_rand_u32(tin) % 698769068 + 1  -- Should be less than 698769069
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
    local r=ffi.new("rng_jkiss_t")
    r.x = 123456789; r.y = 987654321; r.z = 43219876; r.c = 6543217;

    local N = 1e7
    local t=ffiC.gethiticks()
    for i=1,N do
        ffiC.rand_jkiss_dbl(r)
    end
    print("RANDGEN TIME: "..ffiC.gethiticks()-t)  -- x86_64: approx. 100 ms
--]]

    return s
end
