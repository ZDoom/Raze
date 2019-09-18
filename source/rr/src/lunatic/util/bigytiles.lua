#!/usr/bin/env luajit

if (arg[1]==nil) then
    print("Usage: "..arg[0].." ../path/to/*.ART")
    return 1
end

B = require "build"

tile = B.loadarts(arg)

for i=0,B.MAX.TILES-1 do
    if (tile.sizy[i] > 256) then
        print(i..": "..tile.sizy[i])
    end
end
