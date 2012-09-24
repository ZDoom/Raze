#!/usr/bin/env luajit

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
    local wr = function(s) io.stdout:write(s) end
    wr("Usage: ./foreachmap <module[.lua]> [init args...] <filename.map> ...\n")
    wr("       ./foreachmap -e\"some_lua_code ...\" <filename.map> ...\n")
    wr("       ./foreachmap -e\"[sector|wall|sprite]: <condition on .<field>>\" <fn.map> ...\n\n")
    wr("In the second form, the code is run as body of a function(map, fn)\n")
    wr("and num{sectors,walls,sprites} and {sector,wall,sprite} do not\n")
    wr("need to be qualified with the \"map.\" prefix.\n")
    wr("The third form is a shortcut for quickly finding sectors/walls/sprites\n")
    wr("satisfying a certain condition (see example below)\n\n")
    wr("Examples: ./foreachmap.lua -e\"if map.numbunches==1 then print(fn) end\" ~/.eduke32/*.map\n")
    wr("          ./foreachmap.lua -e\"sprite: .picnum==10 and .lotag==2563\" *.map\n\n")
    return
end

local modname = string.gsub(arg[1], "%.lua$", "")

local mod
if (modname:sub(1,2) == "-e") then
    local body = modname:sub(3)

    -- sector/wall/sprite finder shortcut
    local b, e, what = body:find("^([a-z]+):")
    if (what) then
        body = body:sub(e+1)
        body = body:gsub("%.[a-z]+", what.."[i]%0")
        body =
            "for i=0,num"..what.."s-1 do"..
            "  if ("..body..") then print(fn..': '..i) end "..
            "end"
    end

    local successfunc, errmsg = loadstring(
        "local numsectors, numwalls, numsprites\n"
            .."local sector, wall, sprite\n"
            .."return function (map, fn) \n"
            .."  numsectors, numwalls, numsprites = map.numsectors, map.numwalls, map.numsprites\n"
            .."  sector, wall, sprite = map.sector, map.wall, map.sprite\n"
            ..body.."\n"
            .."end"
    )

    if (successfunc==nil) then
        io.stderr:write("Error loading string: "..errmsg.."\n")
        os.exit(1)
    end

    mod = { success=successfunc() }
else
    mod = require(modname)
end

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
