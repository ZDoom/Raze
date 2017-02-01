
-- "Bit array" module based on LuaJIT's BitOp.

local bit = require "bit"
local math = require "math"

local ffi = require "ffi"

local assert = assert
local error = error
local type = type

local tostring = tostring


module(...)


local bitar_ct = ffi.typeof[[
struct {
    const double maxbidx;  // last permissible bit index
    const double maxidx;   // last permissible int32_t index
    const intptr_t arptr;  // address of the int32_t array
}]]
local ptr_to_int = ffi.typeof("int32_t *")

local anchor = {}

-- population count of a nibble
local nibpop = ffi.new("double [?]", 16,
                       { 0, 1, 1, 2, 1, 2, 2, 3,
                         1, 2, 2, 3, 2, 3, 3, 4 })
-- ...and of a byte
local bytepop = ffi.new("double [?]", 256)
for i=0,255 do
    bytepop[i] = nibpop[bit.band(i, 15)] + nibpop[bit.rshift(i, 4)]
end
nibpop = nil

local function bitar_from_intar(maxbidx, maxidx, ar)
    -- We need to have the int32_t[?] array be reachable so that it will not be
    -- garbage collected
    local ar_intptr = ffi.cast("intptr_t", ar)
    anchor[tostring(ar_intptr)] = ar

    -- Leaving the (potential) high trailing bits at 0 lets us not worry
    -- about them in the population count calculation (__len metamethod).
    -- Also, this is correct for maxbidx%32 == 0, since BitOp's shifts
    -- mask the 5 lower bits of the counts.
    local numremain = bit.band(maxbidx+1, 31)
    ar[maxidx] = bit.band(ar[maxidx], bit.rshift(-1, 32-numremain))

    return bitar_ct(maxbidx, maxidx, ar_intptr)
end

local function setop_common_rel(s1, s2)
    if (s1.maxbidx ~= s2.maxbidx) then
        error("bad arguments to bit array set op: must be of same length", 4)
    end

    local ar1 = ffi.cast(ptr_to_int, s1.arptr)
    local ar2 = ffi.cast(ptr_to_int, s2.arptr)

    return ar1, ar2
end

local function setop_common(s1, s2)
    if (not ffi.istype(bitar_ct, s1) or not ffi.istype(bitar_ct, s2)) then
        error("bad arguments to bit array set op: both must be 'bitar' types", 3)
    end

    local ar1, ar2 = setop_common_rel(s1, s2)
    local ar = ffi.new("int32_t [?]", s1.maxidx+1)

    return ar, ar1, ar2
end

local mt = {
    --- Operational methods

    __add = function(s1, s2)  -- set union
        local ar, ar1, ar2 = setop_common(s1, s2)
        for i=0,s1.maxidx do
            ar[i] = bit.bor(ar1[i], ar2[i])
        end
        return bitar_from_intar(s1.maxbidx, s1.maxidx, ar)
    end,

    __mul = function(s1, s2)  -- set intersection
        local ar, ar1, ar2 = setop_common(s1, s2)
        for i=0,s1.maxidx do
            ar[i] = bit.band(ar1[i], ar2[i])
        end
        return bitar_from_intar(s1.maxbidx, s1.maxidx, ar)
    end,

    __sub = function(s1, s2)  -- set difference
        local ar, ar1, ar2 = setop_common(s1, s2)
        for i=0,s1.maxidx do
            ar[i] = bit.band(ar1[i], bit.bnot(ar2[i]))
        end
        return bitar_from_intar(s1.maxbidx, s1.maxidx, ar)
    end,

    __unm = function(s)  -- bitwise NOT
        local newar = ffi.new("int32_t [?]", s.maxidx+1)
        local oldar = ffi.cast(ptr_to_int, s.arptr)
        for i=0,s.maxidx do
            newar[i] = bit.bnot(oldar[i])
        end
        return bitar_from_intar(s.maxbidx, s.maxidx, newar)
    end,


    --- Additional operations

    __index = {
        -- TODO: Rename to 'testi', 'seti', 'cleari'; add 'flipi'?

        -- Is bit i set?
        isset = function(s, i)
            if (not (i >= 0 and i<=s.maxbidx)) then
                error("bad bit index for isset: must be in [0.."..s.maxbidx.."]", 2)
            end
            s = ffi.cast(ptr_to_int, s.arptr)
            return (bit.band(s[bit.rshift(i, 5)], bit.lshift(1, i)) ~= 0)
        end,

        -- Clear bit i.
        set0 = function(s, i)
            if (not (i >= 0 and i<=s.maxbidx)) then
                error("bad bit index for set0: must be in [0.."..s.maxbidx.."]", 2)
            end
            s = ffi.cast(ptr_to_int, s.arptr)
            local jx = bit.rshift(i, 5)
            s[jx] = bit.band(s[jx], bit.rol(0xfffffffe, i))
        end,

        -- Set bit i.
        set1 = function(s, i)
            if (not (i >= 0 and i<=s.maxbidx)) then
                error("bad bit index for set1: must be in [0.."..s.maxbidx.."]", 2)
            end
            s = ffi.cast(ptr_to_int, s.arptr)
            local jx = bit.rshift(i, 5)
            s[jx] = bit.bor(s[jx], bit.rol(0x00000001, i))
        end
    },


    --- Relational methods

    __eq = function(s1, s2)  -- set identity
        local ar1, ar2 = setop_common_rel(s1, s2)
        for i=0,s1.maxidx do
            if (bit.bxor(ar1[i], ar2[i]) ~= 0) then
                return false
            end
        end
        return true
    end,

    __le = function(s1, s2)
        local ar1, ar2 = setop_common_rel(s1, s2)
        for i=0,s1.maxidx do
            if (bit.band(ar1[i], bit.bnot(ar2[i])) ~= 0) then
                return false
            end
        end
        return true
    end,

    __lt = function(s1, s2)
        return s1 <= s2 and not (s2 == s1)
    end,

    -- The length operator gets the population count of the bit array, i.e. the
    -- number of set bits.
    __len = function(s)
        local ar = ffi.cast(ptr_to_int, s.arptr)
        local popcnt = 0
        for i=0,s.maxidx do
            popcnt = popcnt + bytepop[bit.band(ar[i], 255)] +
                bytepop[bit.band(bit.rshift(ar[i], 8), 255)] +
                bytepop[bit.band(bit.rshift(ar[i], 16), 255)] +
                bytepop[bit.rshift(ar[i], 24)]
        end
        return popcnt
    end,

    -- serialization
    __tostring = function(s)
        local size=s.maxidx+1
        local ar = ffi.cast(ptr_to_int, s.arptr)

        local hdr = "bitar.new("..s.maxbidx..", '"
        local ofs = #hdr
        local totalstrlen = ofs+8*size+2
        local str = ffi.new("char [?]", totalstrlen)

        ffi.copy(str, hdr, ofs)

        for i=0,s.maxidx do
            -- 'a' is ASCII 97
            for nib=0,7 do
                str[ofs + 8*i + nib] = 97 + bit.band(bit.rshift(ar[i], 4*nib), 0x0000000f)
            end
        end

        ffi.copy(str+totalstrlen-2, "')", 2)

        return ffi.string(str, totalstrlen)
    end,

    -- On garbage collection of the bitar, clear the array's anchor so that it
    -- can be collected too.
    __gc = function(s)
        anchor[tostring(s.arptr)] = nil
    end,

    __metatable = true,
}

ffi.metatype(bitar_ct, mt)


-- Create new bit array.
function new(maxbidx, initval)
    if (type(maxbidx) ~= "number" or not (maxbidx >= 0 and maxbidx <= (2^31)-2)) then
        -- NOTE: Uh-oh, we can't write '2^31' because that would be interpreted
        -- as color by OSD_Printf.
        error("bad argument #1 to bitar.new (must be a number in [0..(2**31)-2])", 2)
    end

    if (math.floor(maxbidx) ~= maxbidx) then
        error("bad argument #1 to bitar.new (must be an integral number)")
    end

    if (ffi.istype(ptr_to_int, initval)) then
        -- Initialization from an array on the C side. INTERNAL.
        -- Cannot be reached by user code since there's no access to the FFI
        -- (and thus no way to create pointers).

        return bitar_from_intar(maxbidx, (maxbidx+1)/32-1, initval)

    elseif (type(initval)=="string") then
        -- String containing hex digits (a..p) given, for INTERNAL use only.
        -- XXX: Can be reached by user code.

        local lstr = initval

        local numnibs = #lstr
        assert(numnibs%8 == 0)

        local size = numnibs/8
        local maxidx = size-1
        local ar = ffi.new("int32_t [?]", size)

        local str = ffi.new("char [?]", numnibs)
        ffi.copy(str, lstr, numnibs)

        for i=0,maxidx do
            ar[i] = 0

            for nib=0,7 do
                local hexdig = str[8*i + nib]
                assert(hexdig >= 97 and hexdig < 97+16)
                ar[i] = bit.bor(ar[i], bit.lshift(hexdig-97, 4*nib))
            end
        end

        -- NOTE: <maxbidx> cannot be extracted from the string, use the passed one.
        return bitar_from_intar(maxbidx, maxidx, ar)

    else
        -- User-requested bitar creation.

        if (initval ~= 0 and initval ~= 1) then
            error("bad argument #2 to bitar.new (must be either 0 or 1)", 2)
        end

        local maxidx = math.floor(maxbidx/32)
        local size = maxidx+1

        local ar = ffi.new("int32_t [?]", size)

        if (initval==1) then
            ffi.fill(ar, size*4, -1)
        end

        return bitar_from_intar(maxbidx, maxidx, ar)
    end
end
