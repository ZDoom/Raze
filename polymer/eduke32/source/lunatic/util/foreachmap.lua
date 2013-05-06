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

-- forxcode example: print sprite numbers with lotag < -1 (lotag is signed for us),
-- and for each matching sprite also print its lotag and picnum:
--  $ ./findmaps.sh ~/.eduke32/ "sprite: .lotag < -1 :: io. write(', '.. .lotag .. ' ' .. .picnum)"

-- The local 'd' provides defs loaded from ../../names.h, example:
--  $ ./findmaps.sh ~/.eduke32/ "sprite: .picnum>=d. CRACK1 and .picnum<=d. CRACK4"

-- Print all V9 maps along with their number of bunches and max(ceilings of a bunch)
--  $ prog='if (map.version==9) then print(map.numbunches.." ".. math.max(unpack(map.sectsperbunch[0],0)) .." "..fn) end'
--  $ ./findmaps.sh ~/.eduke32 "$prog" |sort -n -k 2

local B = require "build"
local string = require "string"
local io = require "io"
local os = require "os"

if (#arg < 1) then
    local wr = function(s) io.stdout:write(s) end
    wr("Usage: ./foreachmap.lua <module[.lua]> [init args...] <filename.map> ...\n")
    wr("       ./foreachmap.lua -e\"some_lua_code ...\" <filename.map> ...\n")
    wr("       ./foreachmap.lua -e\"[sector|wall|sprite]: <condition on .<field>>\" <fn.map> ...\n\n")
    wr("In the second form, the code is run as body of a function(map, fn)\n")
    wr("and num{sectors,walls,sprites} and {sector,wall,sprite} do not\n")
    wr("need to be qualified with the \"map.\" prefix.\n")
    wr("The third form is a shortcut for quickly finding sectors/walls/sprites\n")
    wr("satisfying a certain condition (see example below)\n\n")
    wr("Examples: ./foreachmap.lua -e\"if map.numbunches==1 then print(fn) end\" ~/.eduke32/*.map\n")
    wr("          ./foreachmap.lua -e\"sprite: .picnum==10 and .lotag==2563\" *.map\n")
    wr("          ./foreachmap.lua -e\"sprite:: ...  -- (only prints the file names)\n")
    wr("(See foreachmap.lua for an example of the \"forxcode\" feature.)\n\n")
    return
end

local modname = string.gsub(arg[1], "%.lua$", "")

function sum(tab, initidx)
    local s = 0
    for i=(initidx or 1),#tab do
        s = s + tab[i]
    end
    return s
end

local mod
if (modname:sub(1,2) == "-e") then
    local body = modname:sub(3)

    -- sector/wall/sprite finder shortcut
    local b, e, what = body:find("^([a-z]+)::?")
    if (what) then
        local onlyfiles = (body:sub(e-1,e)=="::")  -- "::" means "only list files" (like grep -l)
        body = body:sub(e+1)  -- clip off "bla::"
        body = body:gsub("%.[a-z]+", what.."[i]%0")  -- e.g. .lotag --> sprite[i].lotag

        local perxcode
        -- look for additional "print" code to be executed for each match
        b, e, perxcode = body:find("::(.*)")
        if (perxcode) then
            body = body:sub(1,b-1)
            if (onlyfiles) then
                error("Per-x code can only be used with the ':' syntax (list each match)")
            end
        else
            perxcode = ""
        end

        assert(what=="sector" or what=="wall" or what=="sprite")

        body =
            "for i=0,num"..what.."s-1 do\n"..
            "  if ("..body..") then\n"..
            (onlyfiles and "io.write(fn); return\n" or "io.write(fn..': '..i)\n") ..
            perxcode .. "io.write('\\n')\n"..
            "  end\n"..
            "end\n"
    end

    local successfunc, errmsg = loadstring([[
        local d=require('build').readdefs('../../names.h') or error('Need ../../names.h')  -- XXX
        local numsectors, numwalls, numsprites, sector, wall, sprite
        return function (map, fn)
            numsectors, numwalls, numsprites = map.numsectors, map.numwalls, map.numsprites
            sector, wall, sprite = map.sector, map.wall, map.sprite
        ]]..body
    .." end")

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
