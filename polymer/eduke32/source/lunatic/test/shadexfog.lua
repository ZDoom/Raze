--[[
 Usage: in Mapster32,
  > lua "shadexfog=reload'shadexfog'"
  -- for example
  > lua "shadexfog.create(100, 63,63,63)"
  > lua "shadexfog.translate(100, 2)"
 In EDuke32, simply pass this module at the command line.
--]]

local error = error

local math = require("math")
local min, max = math.min, math.max
local floor = math.floor

local sector, wall, sprite = sector, wall, sprite

local engine = require("engine")
local gv = gv

----------

local shadexfog = {}

-- Create 32 palookups corrensponding to different *shade levels* of a fog
-- palookup, called a "shade-x-fog" palookup set in the following.
--
-- Pals <tartpalnum> .. <startpalnum>+31 will be taken.
-- <fogr>, <fogg>, <fogb>: intensities of the fog color, [0 ..63]
function shadexfog.create(startpalnum, fogr, fogg, fogb)
    local MAXPALNUM = 255-31-engine.RESERVEDPALS
    if (not (startpalnum >= 1 and startpalnum <= MAXPALNUM)) then
        error("invalid startpalnum, max="..MAXPALNUM, 2)
    end

    local basesht = engine.getshadetab(0)

    -- Encode the shade in different pal numbers! The shade tables are
    -- constructed with a fog in their place.
    for dummyshade=1,31 do
        local sht = engine.shadetab()

        for f=0,31 do
            for i=0,255 do
                local r, g, b = engine.getrgb(basesht[dummyshade][i])

                local nr, ng, nb =
                    (r*(32-f) + fogr*f) / 32,
                    (g*(32-f) + fogg*f) / 32,
                    (b*(32-f) + fogb*f) / 32

                sht[f][i] = engine.nearcolor(nr, ng, nb)
            end
        end

        engine.setshadetab(startpalnum + dummyshade, sht)
    end
end

local function trans(what, startpalnum, fogintensity)
    local shade = min(max(what.shade, 0), 31)
    what.pal = startpalnum + shade
    what.shade = fogintensity
end

-- shadexfog.translate(startpalnum, fogintensity [, vis])
--
-- Translate the whole map for use with a shade-x-fog palookup set.
--  .pal becomes the <startpalnum> + former .shade
--  .shade becomes the <fogintensity> [0 .. 31]
-- If <vis> is passed, set all sector's visibility to that value.
--
-- Notes:
--  - works only a single time (TODO: autodetection if already applied)
--  - if shades < 0 or > 31 present, loss of information
function shadexfog.translate(startpalnum, fogintensity, vis)
    for i=0,gv.numsectors-1 do
        trans(sector[i].ceiling, startpalnum, fogintensity)
        trans(sector[i].floor, startpalnum, fogintensity)
        if (vis) then
            sector[i].visibility = vis
        end
    end

    for i=0,gv.numwalls-1 do
        trans(wall[i], startpalnum, fogintensity)
    end
end

if (gv.LUNATIC_CLIENT == gv.LUNATIC_CLIENT_EDUKE32 and LUNATIC_FIRST_TIME) then
    shadexfog.create(100, 63,63,63)
    print("created shadexfog palookups")
end

---------- BASE SHADE TABLE TESTS ----------

-- Basic test of whether for a color index i corresponding to a color (r,g,b),
-- getclosestcol() returns a color index ii corresponding to the same color.
-- (In the Duke3D palette, there are duplicates, so the requirement i==ii is
-- too strict.)
function shadexfog.test_nearcolor()
    for i=0,255 do
        local r, g, b = engine.getrgb(i)
        local ii = engine.nearcolor(r, g, b)
        local rr, gg, bb = engine.getrgb(ii)

        if (r~=rr or g~=gg or b~=bb) then
            printf("diff %d: %d,%d,%d  %d,%d,%d", i, r,g,b, rr,gg,bb)
        end
    end
end

-- Change the .pal member of all sector ceilings/floors, walls and sprites to
-- <pal>.
function shadexfog.challpal(palnum)
    for i=0,gv.numsectors-1 do
        sector[i].ceilingpal = palnum
        sector[i].floorpal = palnum
    end
    for i=0,gv.numwalls-1 do
        wall[i].pal = palnum
    end
    for i in sprite.all() do
        sprite[i].pal = palnum
    end
end

-- Create our version of the base shade table (palookup 0)
--
-- NOTE: Nope, the base shade table is NOT created by applying a linear ramp to
-- the base palette colors!!!
local function create_base_shtab(basesht)
    local basesht = basesht or engine.getshadetab(0)

    local sht = engine.shadetab()
    sht[0] = basesht[0]
    for sh=1,31 do
        for i=0,255-16 do
            -- NOTE that this fails, see BASESHT_0_NOT_IDENTITY:
--            assert(basesht[0][i] == i)
            local r, g, b = engine.getrgb(i)
            local f = 1
            r = ((32-f*sh+0.5)*r)/32
            g = ((32-f*sh+0.5)*g)/32
            b = ((32-f*sh+0.5)*b)/32
            r, g, b = max(0,r), max(0,g), max(0,b)  -- if f is > 1
            sht[sh][i] = engine.nearcolor(r, g, b)
        end

        for i=255-16+1,255 do
            -- fullbrights
            sht[sh][i] = basesht[0][i]
        end
    end

    return sht
end

local function create_base_shtab_2(basesht)
    local basesht = basesht or engine.getshadetab(0)

    local perm16 = { [0]=0,1, 2,3, 5,4, 6,7, 8,13, 10,11, 12,9, 14,15 }
    basesht = basesht:remap16(perm16)

    local iperm16 = {}
    for i=0,15 do
        iperm16[perm16[i]] = i
    end

    local iperm = {}
    for i=0,255 do
        iperm[i] = 16*(iperm16[floor(i/16)]) + i%16
    end

    local baseidx = {}
    for i=0,255-16 do
        baseidx[i] = i < 192 and 32*floor(i/32) or 16*floor(i/16)
    end

    local sht = engine.shadetab()

    for sh=0,31 do
        for i=0,255-16 do
            local bi = baseidx[i]
            local cidx = bi + floor(((31-sh)*(i - bi))/31)
            sht[sh][i] = iperm[cidx]
        end

        for i=255-16+1,255 do
            -- fullbrights
            sht[sh][i] = basesht[0][i]
        end
    end

    return sht:remap16(iperm16)
end

if (gv.LUNATIC_CLIENT == gv.LUNATIC_CLIENT_MAPSTER32) then
    -- NOTE: It shouldn't be assumed that these will stay loadable in the future:
    local ffi = require("ffi")
    local io = require("io")

    ffi.cdef[[size_t fwrite(const void * restrict ptr, size_t size, size_t nmemb, void * restrict stream);]]

    function shadexfog.save(filename, palnum_or_sht)
        local sht = type(palnum_or_sht)~="number" and palnum_or_sht
            or engine.getshadetab(palnum_or_sht)
        if (sht == nil) then
            error("No such shade table")
        end

        local f, errmsg = io.open(filename, "w+")
        if (f == nil) then
            error(errmsg, 2)
        end

        -- XXX: ought to be in engine.lua
        local basepal = ffi.new("uint8_t [256][3]")
        for i=0,255 do
            local r, g, b = engine.getrgb(i)
            basepal[i][0], basepal[i][1], basepal[i][2] = r, g, b
        end

        local n1 = ffi.C.fwrite(basepal, 3, 256, f)
        f:write("\032\000")
        local n3 = ffi.C.fwrite(sht, 256, 32, f)

        f:close()
        if (n1 ~= 256 or n3 ~= 32) then
            error("didn't fully write out palette file")
        end
        printf("Wrote base palette + shade table (but NOT transluc table) to '%s'", filename)
    end
end

-- Create our (failed) version of the base shade table at set it to palookup
-- number <palnum>.
-- <secver>: use second attempt?
function shadexfog.create0(palnum, secver)
    local sht0 = secver and create_base_shtab_2() or create_base_shtab()
    engine.setshadetab(palnum, sht0)
end

function shadexfog.test_create0()
    local basesht = engine.getshadetab(0)

    for i=0,255 do
        if (basesht[0][i] ~= i) then
            -- BASESHT_0_NOT_IDENTITY
            printf("Base shade table at index %d: %d", i, basesht[0][i])
        end
    end

    local sht = create_base_shtab(basesht)

    local ok = true
    for sh=1,31 do
        for i=0,255 do
            local ouri, origi = sht[sh][i], basesht[sh][i]
--            if (sht[sh][i] ~= basesht[sh][i]) then
            if (math.abs(ouri - origi) > 1) then
                printf("Constructed shade table DIFFERS AT shade %d index %d: orig %d ours %d",
                       sh, i, basesht[sh][i], sht[sh][i])
                ok = false
                goto out
            end
        end
    end

    ::out::
    if (ok) then
        printf("Constructed shade table IDENTICAL WITH original one")
    end
end

---------- Blending table tests ----------

function shadexfog.create_128_trans(startblendidx)
    for alpha=1,128 do
        local f = alpha/256
        local F = 1-f

        local tab = engine.blendtab()

        for i=0,255 do
            for j=0,255 do
                local r,g,b = engine.getrgb(i)
                local R,G,B = engine.getrgb(j)

                tab[i][j] = engine.nearcolor(f*r+F*R, f*g+F*G, f*b+F*B)
            end
        end

        engine.setblendtab(startblendidx + alpha-1, tab)
    end
end

function shadexfog.create_additive_trans(blendidx)
    local tab = engine.blendtab()

    for i=0,255 do
        for j=0,255 do
            local r,g,b = engine.getrgb(i)
            local R,G,B = engine.getrgb(j)

            tab[i][j] = engine.nearcolor(min(r+R, 63), min(g+G, 63), min(b+B, 63), 255-16)
        end
    end

    engine.setblendtab(blendidx, tab)
end


do
    return shadexfog
end
