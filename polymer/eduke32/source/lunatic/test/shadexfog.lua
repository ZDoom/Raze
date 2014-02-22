--[[
 Usage: in Mapster32,
  > lua "shadexfog=reload'shadexfog'"
  -- for example
  > lua "shadexfog.create(100, 63,63,63)"
  > lua "shadexfog.translate(100, 2)"
 In EDuke32, simply pass this module at the command line.
--]]

local assert = assert
local error = error
local print = print
local printf = printf
local type = type

local bit = require("bit")
local math = require("math")
local min, max = math.min, math.max
local floor = math.floor

local sector, wall, sprite = sector, wall, sprite

local engine = require("engine")
local gv = gv

----------

local shadexfog = {}

-- Example:
--  lua "shadexfog.createremap(30, {[2]=0, [3]=1, [12]=0, [13]=1})"
-- creates a pal 30 which maps the blue and orange ramps to the gray ones.
-- (Compare with the rows of http://wiki.eduke32.com/wiki/File:Pala.png)
--
-- Sexdecatuple remappings of Duke3D pals loaded from LOOKUP.DAT:
-- Remappings that are not expressible as such and identity maps (pal 3 and 9)
-- omitted.
--
--  2:  { [0]=8, [1]=13, [2]=8, [3]=13, [4]=13, [5]=8, [6]=8, [7]=13, [9]=8, [10]=8, [11]=13, [12]=8, [14]=8, }
--  5:  { [8]=2, [13]=3, }
--  7:  { [0]=10, [1]=9, [2]=10, [3]=9, [4]=9, [5]=10, [6]=10, [7]=9, [8]=10, [11]=9, [12]=9, [13]=9, [14]=9, }
--  8:  { [0]=6, [1]=7, [2]=6, [3]=7, [4]=7, [5]=6, [8]=6, [9]=7, [10]=6, [11]=7, [12]=7, [13]=7, [14]=6, }
-- 11:  { [4]=7, [5]=6, }
-- 12:  { [4]=1, [5]=0, }
-- 15:  { [4]=3, [5]=2, }
-- 17:  { [2]=5, [3]=4, [4]=7, [5]=6, [6]=5, [7]=4, [12]=5, [14]=4, }
-- 18:  { [4]=1, [5]=0, }
-- 19:  { [2]=8, [3]=13, [4]=1, [5]=0, [6]=8, [7]=13, [12]=8, [14]=13, }
-- 20:  { [2]=5, [3]=4, [4]=1, [5]=0, [6]=5, [7]=4, [12]=5, [14]=4, }
-- 21:  { [4]=13, [5]=8, }
-- 22:  { [4]=7, [5]=6, }
-- 25:  { [6]=8, [7]=13, }
function shadexfog.createremap(palnum, remaptab)
    local sht = engine.getshadetab(0)
    engine.setshadetab(palnum, sht:remap16(remaptab))
end

-- Create 32 palookups corrensponding to different *shade levels* of a fog
-- palookup, called a "shade-x-fog" palookup set in the following.
--
-- Pals <startpalnum> .. <startpalnum>+31 will be taken.
-- <fogr>, <fogg>, <fogb>: intensities of the fog color, [0 .. 63]
function shadexfog.create(startpalnum, fogr, fogg, fogb)
    local MAXPALNUM = 255-31-engine.RESERVEDPALS
    if (not (startpalnum >= 1 and startpalnum <= MAXPALNUM)) then
        error("invalid startpalnum, max="..MAXPALNUM, 2)
    end

    local basesht = engine.getshadetab(0)

    -- Encode the shade in different pal numbers! The shade tables are
    -- constructed with a fog in their place.
    for dummyshade=0,31 do
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
    if (what.pal >= startpalnum and what.pal <= startpalnum+31) then
        -- Auto-detect earlier translation with the same <startpalnum>.
        what.shade = what.pal - startpalnum
    end

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
--  - auto-detects when the translation has been applied with the *same*
--   <startpalnum> (if a different one is desired, must reload map).
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
    -- Wrapper around engine.savePaletteDat() that errors on unexpected events.
    function shadexfog.save(filename, palnum, blendnum, moreblends)
        local ok, errmsg = engine.savePaletteDat(filename, palnum, blendnum, moreblends)
        if (not ok) then
            error(errmsg)
        end

        printf('Wrote base palette, shade and translucency tables to "%s".', filename)
        if (moreblends ~= nil) then
            printf("  Also wrote additional translucency tables.")
        end
    end

    function shadexfog.saveLookupDat(filename, lookups)
        if (lookups == nil) then
            lookups = {
                -- Duke3D 1.5 LOOKUP.DAT order
                1,2,6,7,8, 3,4,5,9,10,
                12,13,15,16,18, 19,11,14,17,20,
                21,22,23,24,25
            }
        end
        assert(engine.saveLookupDat(filename, lookups))
        printf('Wrote lookup tables and 5 base palettes to "%s".', filename)
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

-- shadexfog.create_trans(startblendidx, func [, numtables [, fullbrightsOK]])
--
-- <func>: must be
--  rr, gg, bb = f(r, g, b, R, G, B, level, numtables)
--  ('level' is the table index, from 1 to <numtables>)
-- <numtables>: number of tables to create, from <startblendidx> on. Default: 1
function shadexfog.create_trans(startblendidx, func, numtables, fullbrightsOK)
    numtables = numtables or 1
    local lastokcol = fullbrightsOK and 255 or 255-16

    local tab = engine.blendtab()

    for level=1,numtables do
        for i=0,255 do
            local r,g,b = engine.getrgb(i)
            for j=0,255 do
                local R,G,B = engine.getrgb(j)

                local rr, gg, bb = func(r,g,b, R,G,B, level, numtables)
                tab[i][j] = engine.nearcolor(rr, gg, bb, lastokcol)
            end
        end

        engine.setblendtab(startblendidx + level-1, tab)
    end
end

local function check_numtables(numtables)
    if (numtables ~= nil) then
        if (type(numtables) ~= "number" or not (numtables >= 1 and numtables <= 128)) then
            error("invalid argument #2: must be a number in [1 .. 128]", 2)
        end

        if (bit.band(numtables, numtables-1) ~= 0) then
            error("invalid argument #2: must be a power of two", 2)
        end
    end
end

-- shadexfog.create_alpha_trans(startblendidx [, numtables [, fullbrightsOK]])
--
-- Creates <numtables> blending tables of smooth alpha translucency, with
-- fractions 1/(2*numtables), 2/(2*numtables) ... numtables/(2*numtables).
-- <numtables> must be a power of two in [1 .. 128].
function shadexfog.create_alpha_trans(startblendidx, numtables, fullbrightsOK)
    check_numtables(numtables)

    shadexfog.create_trans(
        startblendidx,

        function(r,g,b, R,G,B, alpha, numtabs)
            local f = alpha/(2*numtabs)
            local F = 1-f
            return f*r+F*R, f*g+F*G, f*b+F*B
        end,

        numtables, fullbrightsOK
    )
end

-- shadexfog.create_additive_trans(startblendidx [, numtables [, fullbrightsOK]])
function shadexfog.create_additive_trans(startblendidx, numtables, fullbrightsOK)
    shadexfog.create_trans(
        startblendidx,

        function(r,g,b, R,G,B, level, numtabs)
            local f = level/numtabs
            return min(f*r+R, 63), min(f*g+G, 63), min(f*b+B, 63)
        end,

        numtables, fullbrightsOK
    )
end


do
    return shadexfog
end
