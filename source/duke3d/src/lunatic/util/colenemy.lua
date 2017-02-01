
-- Search for enemies with nonzero pal.
-- foreachmap module.

local build = require("build")
local D = build.readdefs("../../names.h") or error("Need ../../names.h")

local assert = assert
local print = print
local string = require("string")

local io = require("io")

local function printf(fmt, ...)
    print(string.format(fmt, ...))
end


module(...)


local ENEMY_NAME = {
--[[
    [D.BOSS1] = "BOSS1",
    [D.BOSS1STAYPUT] = "BOSS1STAYPUT",
--]]
    [D.BOSS2] = "BOSS2",
    [D.BOSS3] = "BOSS3",
    [D.BOSS4] = "BOSS4",
    [D.BOSS4STAYPUT] = "BOSS4STAYPUT",
    [D.COMMANDER] = "COMMANDER",
    [D.COMMANDERSTAYPUT] = "COMMANDERSTAYPUT",
    [D.DRONE] = "DRONE",
    [D.GREENSLIME] = "GREENSLIME",
    [D.LIZMAN] = "LIZMAN",
    [D.LIZMANFEEDING] = "LIZMANFEEDING",
    [D.LIZMANJUMP] = "LIZMANJUMP",
    [D.LIZMANSPITTING] = "LIZMANSPITTING",
    [D.LIZMANSTAYPUT] = "LIZMANSTAYPUT",
    [D.LIZTROOP] = "LIZTROOP",
    [D.LIZTROOPDUCKING] = "LIZTROOPDUCKING",
    [D.LIZTROOPJETPACK] = "LIZTROOPJETPACK",
    [D.LIZTROOPJUSTSIT] = "LIZTROOPJUSTSIT",
    [D.LIZTROOPONTOILET] = "LIZTROOPONTOILET",
    [D.LIZTROOPRUNNING] = "LIZTROOPRUNNING",
    [D.LIZTROOPSHOOT] = "LIZTROOPSHOOT",
    [D.LIZTROOPSTAYPUT] = "LIZTROOPSTAYPUT",
    [D.OCTABRAIN] = "OCTABRAIN",
    [D.OCTABRAINSTAYPUT] = "OCTABRAINSTAYPUT",
    [D.ORGANTIC] = "ORGANTIC",
    [D.PIGCOP] = "PIGCOP",
    [D.PIGCOPDIVE] = "PIGCOPDIVE",
    [D.PIGCOPSTAYPUT] = "PIGCOPSTAYPUT",
    [D.RAT] = "RAT",
    [D.ROTATEGUN] = "ROTATEGUN",
    [D.SHARK] = "SHARK",
}

local uniq = false

function init(args)
    if (#args == 1) then
        local wr = function(s) io.stderr:write(s) end
        wr("Usage: "..args[0].." "..args[1].." [-u] *.map ...\n")
        wr("  -u: print only one pal-x-tilenum combination\n")
    end

    if (args[2]:sub(1,1)=="-") then
        assert(args[2]=="-u", "Unknown option "..args[2])
        uniq = true
        return 3
    end
end

function success(map, fn)
    -- Have one at all?
    local haveone = false

    -- Have pal-x-tile combination?
    -- [tile*256 + pal] = true
    local havept = {}

    for i=0,map.numsprites-1 do
        local spr = map.sprite[i]
        local picnum, pal = spr.picnum, spr.pal
        local name = ENEMY_NAME[picnum]

        if (name and pal~=0) then
            if (not (name:match("^LIZTROOP") and pal==21)) then  -- those are handled by CON
                if (not uniq or not havept[picnum*256 + pal]) then
                    if (not haveone) then
                        printf("%s:", fn)
                        haveone = true
                    end
                    printf("%5d %3d %s", i, pal, name)
                    havept[picnum*256 + pal] = true
                end
            end
        end
    end
    if (haveone) then
        print("")
    end
end
