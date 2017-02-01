
-- Print out some aggregate statistics for passed BUILD maps,
-- foreachmap module.

local string = require "string"
local math = require "math"

local print = print
local type = type

local stat = require "stat"


module(...)


local function printf(fmt, ...)
    print(string.format(fmt, ...))
end

local sumnumsectors = 0
local sumnumwalls = 0

local s = stat.new()

function success(map, fn)
    local ns = map.numsectors
    local nw = map.numwalls

    s:add(nw/ns)

    sumnumsectors = sumnumsectors+ns
    sumnumwalls = sumnumwalls+nw
end

function finish()
    res = s:getstats()

    printf("%d maps\n", res.n)
    printf("total sectors: %d", sumnumsectors)
    printf("total walls: %d", sumnumwalls)
    printf("total walls / total sectors: %.02f", sumnumwalls/sumnumsectors)
    printf("")
    printf("Walls/sector")
    print(res)
end
