-- Game control module for Lunatic.

local ffi = require("ffi")

local setmetatable = setmetatable

local assert = assert
local error = error
local type = type


module(...)


local lastid = { action=0, move=0 }

local function forbidden() error("newindex forbidden", 2) end
local ac, mv = {}, {}

AC = setmetatable({}, { __index=ac, __newindex=forbidden })
MV = setmetatable({}, { __index=mv, __newindex=forbidden })

local function action_or_move(what, numargs, tab, name, ...)
    assert(lastid[what] > -(2^31))
    if (type(name)~="string" or #name > 63) then
        error("bad argument #1 to "..what..": must be a string of length <= 63", 3)
    end

    local args = {...}
    assert(#args <= numargs)

    for i=1,#args do
        local n = args[i]
        assert(type(n)=="number" and (n >= -32768 and n <= 32767))
    end
    -- missing fields are initialized to 0 by ffi.new

    -- Named actions or moves have negative ids so that non-negative ones
    -- can be used as (different) placeholders for all-zero ones.
    lastid[what] = lastid[what]-1

    -- ffi.new takes either for initialization: varargs, a table with numeric
    -- indices, or a table with key-value pairs
    -- See http://luajit.org/ext_ffi_semantics.html#init_table
    tab[name] = ffi.new("const con_"..what.."_t", lastid[what], args)
end

function action(name, ...)
    action_or_move("action", 5, ac, name, ...)
end

function move(name, ...)
    action_or_move("move", 2, mv, name, ...)
end
