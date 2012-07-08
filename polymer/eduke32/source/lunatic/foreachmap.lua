#!/usr/local/bin/luajit

-- Generic map iterator.

-- The first cmdline arg is a name of a lua file (may be sans .lua) which must
-- be a module and is `require'd into this script, e.g. "stats" or "stats.lua".
-- Then, for each 2nd and following argument, if map loading succeeds,
-- .success(map, filename) is run, otherwise .failure(filename, errmsg) --
-- these must be functions in the loaded module.

-- TODO: aggregate batch mode


if (#arg < 1) then
    error("Usage: luajit ./foreachmap <module[.lua]> <filename.map> ...")
end

local B = require "build"
local string = require "string"

local modname = string.gsub(arg[1], "%.lua$", "")
local mod = require(modname)


for i=2,#arg do
    local fn = arg[i]
    local map, errmsg = B.loadboard(fn)

    if (map ~= nil) then
        mod.success(map, fn)
    else
        mod.failure(fn, errmsg)
    end
end
