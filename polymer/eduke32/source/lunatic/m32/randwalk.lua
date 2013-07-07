-- Random walker as updatesector() timing test, for Lunatic-m32
-- Load with: lua "require 'randwalk'"

local ffi = require "ffi"
local ffiC = ffi.C

local xmath = require "xmath"
local stat = require "stat"


-- set to nil to disable saving positions
g_posns = {}

local USF_BREADTH = sector.UPDATE_FLAGS.BREADTH

local function updatesectorbreadth(pos, sectnum)
    return updatesector(pos, sectnum, USF_BREADTH)
end

-- [STATS, TIMES] = RANDWALK(N, SPRITENUM, MINLEN, MAXLEN [, RANDOFS [, FUNCI [, LOGFN]]])
function randwalk(N, spritenum, minlen, maxlen, randofs, funci, logfn)
    -- Set the random seed to an arbitrary but fixed value so that the "random" walk
    -- is deterministically reproducible for particular arguments.
    math.randomseed(834572183572)

    randofs = randofs or 0

    for i=1,randofs do
        math.random()
    end

    local funcnames = { "updatesector", "updatesectorbreadth", "updatesectorz" }
    local funcs = { updatesector, updatesectorbreadth, updatesectorz }
    local updatesectorfunc = funcs[funci]

    -- stats
    local st = { all=stat.new(), succ=stat.new(), fail=stat.new() }

    local times = {}
    local successp = {}

    local pos = xmath.vec3(sprite[spritenum])
    local sectnum = sprite[spritenum].sectnum

    for i=1,N do
--[[
        -- get random integers for the displacement
        local ax,ay,az = math.random(minlen,maxlen), math.random(minlen,maxlen), math.random(minlen,maxlen)

        -- random signs
        local sx,sy,sz = math.random(0,1), math.random(0,1), math.random(0,1)
        sx, sy, sz = 2*sx-1, 2*sy-1, 2*sz-1  -- {0,1} -> {-1,1}
        ax, ay, az = sx*ax, sy*ay, sz*az
--]]
---[[
        local ang = 2*math.pi*math.random()
        local len = math.random(minlen, maxlen)
        local ax, ay, az = len*math.cos(ang), len*math.sin(ang), 0
--]]
        local newpos = pos + xmath.ivec3(ax,ay,az)

        local t = ffiC.gethiticks()
        local newsect = updatesectorfunc(newpos, sectnum)
        t = ffiC.gethiticks()-t

        st.all:add(t)
        times[i] = t

        successp[i] = (newsect >= 0)

        if (newsect >= 0) then
            st.succ:add(t)

            pos = newpos
            sectnum = newsect
        else
            st.fail:add(t)
        end

        if (g_posns ~= nil) then
            g_posns[i] = pos
        end
    end

    local stallres = st.all:getstats()

    print("====================")
    printf("Random %s walk starting with sprite %d",
           funcnames[funci], spritenum)
    printf("Hop length: [%d .. %d],  RNG offset: %d,  %s",
           minlen, maxlen, randofs, logfn)
    printf("-- Times [ms] (total=%f)", N*stallres.mean)
    print("All: ".. tostring(stallres))
    print("Succeeded: ".. st.succ:getstatstr())
    print("Failed: ".. st.fail:getstatstr())

    if (logfn ~= nil) then
        local logfile, errmsg = io.open(logfn, "w")
        if (logfile == nil) then
            printf("Couldn't open log file \"%s\" for writing: %s",
                   logfn, errmsg)
        else
            logfile:write(" \t \t  \t\t\n")
            for i=1,N do
                if (g_posns ~= nil) then
                    logfile:write(string.format("%s %d  %d %d %d\n",
                                                tostring(times[i]), successp[i] and 1 or 0,
                                                g_posns[i].x, g_posns[i].y, g_posns[i].z))
                else
                    logfile:write(string.format("%s %d\n", tostring(times[i]), successp[i] and 1 or 0))
                end
            end

            logfile:close()
        end
    end

    return st, ts
end
