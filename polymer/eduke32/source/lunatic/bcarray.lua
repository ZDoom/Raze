-- Implementation of a bound-checked array type factory for LuaJIT 2.0 or later.
--
-- Usage example:
--
-- > bcarray.new("int8_t", 3, "test", "three_pigs")
-- > a = ffi.new("struct { int32_t a; three_pigs p; int16_t b; }")
-- > =ffi.sizeof(a)  --> 12
-- > b = ffi.new("__attribute__((packed)) struct { int32_t a; three_pigs p; int16_t b; }")
-- > =ffi.sizeof(b)  --> 9

local ffi = require("ffi")

local string = require("string")
local table = require("table")

local assert = assert
local error = error
local pairs = pairs
local type = type


local bcarray = {}


-- Generate C decl for a sequence of <nelts> const struct members.
-- For example, for 4 elements,
--  "const $ _r1, _f2, _u3, _n4;"
local function flatten_array(nelts, rng)
    local strtab = { "$ " }

    if (rng and rng.getu32==nil) then
        assert(type(rng)=="table")

        for i=1,#rng do
            strtab[i+1] = rng[i]..((i<#rng) and "," or ";")
        end
    else
        for i=1,nelts do
            local ch = 97 + (rng and (rng:getu32() % 25) or 0)  -- 'a'..'z'
            strtab[i+1] = string.format("_%c%x%s", ch, i, (i<nelts) and "," or ";")
        end
    end

    return table.concat(strtab)
end

-- ctype = bcarray.new(basetype, numelts, showname [, typename] [, rng] [, mtadd])
-- (optional fields may be nil)
--
-- <numelts>: Number of elements in array (small number)
-- <showname>: The name to be shown for the derived type in error messages
-- <typename>: If non-nil, the name under which the derived type is typedef'd
-- <rng>: Random generator state + method :getu32(). If nil, then members are
--  named _a1, _a2, ...
--  It also may be a table containing member names at numeric indices 1..#rng.
-- <mtadd>: A table containing functions __index and/or __newindex. They are
--  called first and the bound-checking ones are tail-called then. If the
--  custom __index one returns something, it is returned by the composite one.
function bcarray.new(basetype, numelts, showname, typename, rng, mtadd)
    local eltptr_t = ffi.typeof("$ *", ffi.typeof(basetype))

    local mt = {
        __index = function(ar, idx)
            if (not (idx >= 0 and idx < numelts)) then
                error("out-of-bounds "..showname.." read access", 2)
            end
            return ffi.cast(eltptr_t, ar)[idx]
        end,

        -- NOTE: this function will be dead code if the prefixed __newindex
        -- errors out unconditionally or the bcarray is declared 'const'.
        __newindex = function(ar, idx, val)
            if (not (idx >= 0 and idx < numelts)) then
                error("out-of-bounds "..showname.." write access", 2)
            end
            ffi.cast(eltptr_t, ar)[idx] = val
        end,
    }

    if (mtadd ~= nil) then
        local curindexf, curnewindexf = mt.__index, mt.__newindex
        local addindexf, addnewindexf = mtadd.__index, mtadd.__newindex

        if (addindexf) then
            -- Additional __index metamethod given.
            mt.__index = function(ar, idx)
                local sth = addindexf(ar, idx)
                if (sth ~= nil) then
                    return sth
                end
                return curindexf(ar, idx)
            end
        end

        if (addnewindexf) then
            -- Additional __newindex metamethod given.
            mt.__newindex = function(ar, idx, val)
                addnewindexf(ar, idx, val)
                return curnewindexf(ar, idx, val)
            end
        end
    end

    local cdeclstr = "struct {"..flatten_array(numelts, rng).."}"
    local bcarray_t = ffi.typeof(cdeclstr, ffi.typeof(basetype));

    bcarray_t = ffi.metatype(bcarray_t, mt)
    if (not (rng and rng.getu32==nil)) then
        -- When passing a member name table, it is allowed to have a different
        -- number of named members than array elements.
        assert(ffi.sizeof(bcarray_t) == ffi.sizeof(basetype)*numelts)
    end

    if (typename ~= nil) then
        -- Register the type name in the global namespace.
        assert(type(typename)=="string")
        ffi.cdef("typedef $ $;", bcarray_t, typename)
    end

    return bcarray_t
end


return bcarray
