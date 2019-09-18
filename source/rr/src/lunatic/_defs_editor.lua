-- INTERNAL
-- definitions of BUILD and game types for the Lunatic Interpreter

local ffi = require("ffi")
local ffiC = ffi.C

ffi.cdef[[
enum {
    LUNATIC_CLIENT_MAPSTER32 = 0,
    LUNATIC_CLIENT_EDUKE32 = 1,

    LUNATIC_CLIENT = LUNATIC_CLIENT_MAPSTER32
}
]]

--== First, load the definitions common to the game's and editor's Lua interface.
decl = ffi.cdef
local defs_c = require("defs_common")
defs_c.finish_spritetype({})

defs_c.create_globals(_G)

-- TODO: provide access to only a subset, restict access to ffiC?
gv = ffiC

--== Mapster32-specific initialization

ffi.cdef "char *listsearchpath(int32_t initp);"

-- Add the search path directories to the Lua load path.
local initp = 1
while (true) do
    local sp_c = ffiC.listsearchpath(initp)

    if (sp_c == nil) then
        break
    end

    local sp = ffi.string(sp_c)
    assert(sp:sub(-1)=='/')
    package.path = sp..'?.lua;'..package.path

    initp = 0
end

-- Helper functions
local package = package
local require = require

function reload(modname)
    package.loaded[modname] = nil
    return require(modname)
end

--print('Lua load path: '..package.path)
