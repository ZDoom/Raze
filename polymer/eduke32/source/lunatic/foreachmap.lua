#!/usr/local/bin/luajit

-- Generic map iterator.

-- The first cmdline arg is a name of a lua file (may be sans .lua) which must
-- be a module and is `require'd into this script, e.g. "stats" or "stats.lua".
-- First, a key named .init is looked up in the loaded module and if it exists,
-- it is run like .init(arg)  (thus allowing it to parse the command-line
-- arguments and then potentially remove the ones it used).
-- If .init returns non-nil, this script aborts.
-- Otherwise, for each 2nd and following argument, if map loading succeeds,
--  .success(map, filename) is run, otherwise
--  .failure(filename, errmsg) is run if that key exists, or a standard error
--  message is printed to stderr.
-- Finally, if there is a .finish field in the module, it is run with no args.


local B = require "build"
local string = require "string"
local io = require "io"
local os = require "os"

if (#arg < 1) then
    io.stdout:write("Usage: luajit ./foreachmap <module[.lua]> [init args...] <filename.map> ...\n\n")
    return
end

local modname = string.gsub(arg[1], "%.lua$", "")
local mod = require(modname)

if (mod.init) then
    if (mod.init(arg) ~= nil) then
        os.exit(1)
    end
end

for i=2,#arg do
    local fn = arg[i]
    local map, errmsg = B.loadboard(fn)

    if (map ~= nil) then
        mod.success(map, fn)
    else
        if (mod.failure) then
            mod.failure(fn, errmsg)
        else
            io.stderr:write(string.format("--- %s: %s\n", fn, errmsg))
        end
    end
end

if (mod.finish) then
    mod.finish()
end
