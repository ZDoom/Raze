#!/usr/bin/env luajit

local B = require("build")
local string = require("string")

local opt, fn
local do_canonicalize_sprite

if (arg[1] and arg[1]:sub(1,1)=="-") then
    opt = arg[1]
    fn = arg[2]
else
    fn = arg[1]
end

if (opt) then
    if (opt=="-c" or opt=="-C") then
        -- -C means to canonicalize but without adding the new->old mapping
        -- comment
        do_canonicalize_sprite = opt
    else
        print("Error: unrecognized option "..opt)
        return 1
    end
end

if (fn==nil) then
    print("Usage: map2text [-c/-C] <somefile.map>")
    return 1
end

local function printf(fmt, ...)
    print(string.format(fmt, ...))
end

local map, errmsg = B.loadboard(fn, do_canonicalize_sprite)
if (map == nil) then
    printf("Couldn't load \"%s\": %s", fn, errmsg)
    return 1
end


printf("numsectors = %d", map.numsectors)
printf("numwalls = %d", map.numwalls)
printf("numsprites = %d", map.numsprites)

printf("startpos = { %d, %d, %d }", map.start.x, map.start.y, map.start.z)
printf("startang = %d", map.start.ang)
printf("startsectnum = %d", map.start.sectnum)

local sector_members = {
    "wallptr", "wallnum",
    "ceilingz", "floorz",
    "ceilingstat", "floorstat",
    "ceilingpicnum", "ceilingheinum",
    "ceilingshade",
    "ceilingpal", "ceilingxpanning", "ceilingypanning",
    "floorpicnum", "floorheinum",
    "floorshade",
    "floorpal", "floorxpanning", "floorypanning",
    "visibility", "fogpal",
    "lotag", "hitag", "extra",
}

local wall_members = {
    "x", "y",
    "point2", "nextwall", "nextsector",
    "cstat",
    "picnum", "overpicnum",
    "shade",
    "pal", "xrepeat", "yrepeat", "xpanning", "ypanning",
    "lotag", "hitag", "extra",
}

local sprite_members = {
    "x", "y", "z",
    "cstat", "picnum",
    "shade",
    "pal", "clipdist", "blend",
    "xrepeat", "yrepeat",
    "xoffset", "yoffset",
    "sectnum", "statnum",
    "ang", "owner", "xvel", "yvel", "zvel",
    "lotag", "hitag", "extra",
}

local function print_members(map, struct, members)
    printf("%s = {", struct)
    for i=0,map["num"..struct.."s"]-1 do
        local comment = ""
        if (struct=="sprite" and do_canonicalize_sprite=="-c") then
            comment = "  --"..tostring(map.spriten2o[i])
        end

        printf("[%d]={%s", i, comment)

        for j=1,#members do
            local member = members[j]
            printf("%s = %d", member, map[struct][i][member])
        end

        print("}")
    end

    print("}")
end

print_members(map, "sector", sector_members)
print_members(map, "wall", wall_members)
print_members(map, "sprite", sprite_members)
