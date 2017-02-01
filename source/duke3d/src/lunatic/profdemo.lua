#!/usr/bin/env luajit

local stat = require "stat"

if (#arg ~= 2) then
    print("Usage: profdemo.lua <eduke32> <demo_arg>")
    print("Example: ./profdemo.lua ../../eduke32 -d3:1,8")
    print("         (for 8 repetitions of demo 3 at 1 frame per gametic)")
    print("")
    os.exit(1)
end

local eduke32 = arg[1]
local demoarg = arg[2]

local numrepstr = demoarg:match(",[0-9]+$")
local numreps = tonumber(numrepstr and numrepstr:sub(2)) or 1
local st = { game=stat.new(), drawrooms=stat.new(), drawrest=stat.new() }
local stperx = { game=stat.new(), drawrooms=stat.new(), drawrest=stat.new() }

local unit, unitperx = {}, {}

for i=1,numreps do
    local fh = io.popen(eduke32.." "..demoarg)

    while (true) do
        local str = fh:read("*l")
        if (str == nil) then
            break
        end

        local NUMRE = "([0-9]+%.[0-9]+)"
        local UNITRE = "[mu]?s"

        local RE = "(== demo [0-9]+ ([a-z]+) times: "..NUMRE.." ("..UNITRE..") %("..NUMRE.." ("..UNITRE.."/[a-z]+)%))"
        local wholematch, whatstr, time1, unit1, time2, unit2 = str:match(RE)

        if (wholematch ~= nil) then
            if (numreps==1) then
                print(wholematch)
            else
                st[whatstr]:add(tonumber(time1))
                stperx[whatstr]:add(tonumber(time2))

                unit[whatstr] = unit1
                unitperx[whatstr] = unit2
            end
        end
    end
end

if (numreps > 1) then
    local keys = { "game", "drawrooms", "drawrest" }

    for i=1,#keys do
        local key = keys[i]
        if (unit[key] ~= nil) then
            print("== "..key.." times:")
            print("   "..st[key]:getstatstr().."  ["..unit[key].."]")
            print("   "..stperx[key]:getstatstr().."  ["..unitperx[key].."]")
        end
    end
end
