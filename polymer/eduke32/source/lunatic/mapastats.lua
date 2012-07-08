
-- Print out some aggregate statistics for passed BUILD maps,
-- foreachmap module.

local string = require "string"
local math = require "math"

local print = print

module(...)


local function printf(fmt, ...)
    print(string.format(fmt, ...))
end


local N = 0

local sumnumsectors = 0
local sumnumwalls = 0

local sumratio = 0
local sumsqratio = 0

function success(map, fn)
    local ns = map.numsectors
    local nw = map.numwalls

    N = N+1

    sumratio = sumratio + nw/ns
    sumsqratio = sumsqratio + (nw/ns)^2

    sumnumsectors = sumnumsectors+ns
    sumnumwalls = sumnumwalls+nw
end

function finish()
    printf("%d maps\n", N)
    printf("total sectors: %d", sumnumsectors)
    printf("total walls: %d", sumnumwalls)
    printf("total walls / total sectors: %.02f", sumnumwalls/sumnumsectors)
    printf("")
    printf("Walls/sector")

    local meanwpers = sumratio/N
    printf(" mean: %.02f", meanwpers)
    printf(" stdev: %.02f", math.sqrt(sumsqratio/N - meanwpers^2))
end
