
-- Print out some statistics for a BUILD map,
-- foreachmap module.

local string = require "string"
local print = print

module(...)


local function printf(fmt, ...)
    print(string.format(fmt, ...))
end


function success(map, fn)
    printf("--- %s:", fn)

    printf("  version: %d", map.version)
    printf("  numsectors: %d\n  numwalls: %d\n  numsprites: %d",
           map.numsectors, map.numwalls, map.numsprites)
end

function failure(fn, errmsg)
    printf("--- %s: %s", fn, errmsg)
end
