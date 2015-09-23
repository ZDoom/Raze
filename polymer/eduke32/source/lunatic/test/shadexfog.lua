--[[
 Usage: in Mapster32,
  > lua "shadexfog=reload'shadexfog'"
  -- for example
  > lua "shadexfog.create(100, 255,255,255)"
  > lua "shadexfog.translate(100, 2)"
 In EDuke32, simply pass this module at the command line.
--]]

local assert = assert
local error = error
local print = print
local printf = printf
local tonumber = tonumber
local type = type
local unpack = unpack

local bit = require("bit")
local math = require("math")
local string = require("string")
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
-- <fogr>, <fogg>, <fogb>: intensities of the fog color, [0 .. 255]
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
-- If <vis> is passed and >= 0, set all sector's visibility to that value.
--
-- Notes:
--  - auto-detects when the translation has been applied with the *same*
--   <startpalnum> (if a different one is desired, must reload map).
--  - if shades < 0 or > 31 present, loss of information
function shadexfog.translate(startpalnum, fogintensity, vis)
    for i=0,gv.numsectors-1 do
        trans(sector[i].ceiling, startpalnum, fogintensity)
        trans(sector[i].floor, startpalnum, fogintensity)
        if (vis and vis >= 0) then
            sector[i].visibility = vis
        end
    end

    for i=0,gv.numwalls-1 do
        trans(wall[i], startpalnum, fogintensity)
    end
end

if (gv.LUNATIC_CLIENT == gv.LUNATIC_CLIENT_EDUKE32 and LUNATIC_FIRST_TIME) then
    shadexfog.create(100, 255,255,255)
    print("created shadexfog palookups")
end

---------- BASE SHADE TABLE TESTS ----------

-- sht = shadexfog.create_depth_shtab([palnum])
function shadexfog.create_depth_shtab(palnum)
    local sht = engine.shadetab()

    for s=0,31 do
        for i=0,255 do
            sht[s][i] = s
        end
    end

    if (palnum) then
        engine.setshadetab(palnum, sht)
    end
    return sht
end

function shadexfog.create_vismarker_shtab(palnum)
    local sht = engine.getshadetab(0)

    for i=0,255 do
        sht[1][i] = 242
        sht[30][i] = 245
    end

    if (palnum) then
        engine.setshadetab(palnum, sht)
    end
    return sht
end

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
-- <palnum>.
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

local ismapster32 = (gv.LUNATIC_CLIENT == gv.LUNATIC_CLIENT_MAPSTER32)

if (ismapster32) then
    -- Wrapper around engine.savePaletteDat() that errors on unexpected events.
    function shadexfog.save(filename, palnum, blendnum, moreblends, lognumalphatabs)
        local ok, errmsg, nummoreblends = engine.savePaletteDat(
            filename, palnum, blendnum, moreblends, lognumalphatabs)
        if (not ok) then
            error(errmsg)
        end

        printf('Wrote base palette, shade and translucency tables to "%s".', filename)
        if (nummoreblends > 0) then
            printf("  Also wrote %d additional translucency tables.", nummoreblends)
        end
    end

    function shadexfog.saveLookupDat(filename, lookups)
        local ok, errmsg, numlookups = engine.saveLookupDat(filename, lookups)
        if (not ok) then
            error(errmsg)
        end

        printf('Wrote %d lookup tables and 5 base palettes to "%s".',
               numlookups, filename)
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
--  rr, gg, bb = f(r,g,b, R,G,B, level, numtables)
--  If reverse translucency bit clear, (r,g,b) is background and (R,G,B) is
--  foreground (incoming).
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
            return min(f*r+R, 255), min(f*g+G, 255), min(f*b+B, 255)
        end,

        numtables, fullbrightsOK
    )
end

-- shadexfog.create_brightpass_trans(startblendidx [, numtables [, fullbrightsOK]])
function shadexfog.create_brightpass_trans(startblendidx, numtables, fullbrightsOK)
    shadexfog.create_trans(
        startblendidx,

        function(r,g,b, R,G,B, alpha, numtabs)
            local a = alpha/numtabs
            local F = 1 - min(a, (R+G+B) / (3*255))
            local f = 1 - F
            return f*r+F*R, f*g+F*G, f*b+F*B
        end,

        numtables, fullbrightsOK
    )
end

if (not ismapster32) then
    return shadexfog
end

--========== Mapster32 Lua menu hooks ==========--

local getnumber16 = engine.getnumber16
local GNF = engine.GETNUMFLAG
local GNF_BOOL = GNF.NEXTFREE

local df = GNF.RET_M1_ON_CANCEL  -- default getnumber16() flags

local MAXUSERPALOOKUP = 256-1-8  -- KEEPINSYNC engine.lua:check_palidx()

-- wrapped_func = CreateMenuFunction(argdesc)
--
-- <argdesc>: table with [0]=<func> and then, entries { name, init, max [, noret] }
local function CreateMenuFunction(argdesc)
    return function()
        local func = argdesc[0]
        assert(type(func) == "function")
        local args = {}

        for i=1,#argdesc do
            local ad = argdesc[i]
            assert(type(ad) == "table" and #ad == 3 or #ad == 4)

            local moreflags = ad[4] or 0
            args[i] = getnumber16(ad[1]..": ", ad[2], ad[3], bit.bor(df, moreflags))
            if (bit.band(moreflags, GNF.NEG_ALLOWED)==0 and args[i] < 0) then
                return
            end
            if (bit.band(moreflags, GNF_BOOL)~=0) then
                args[i] = (args[i] > 0)
            end
        end

        func(unpack(args))
    end
end

-- Replace chevrons (angle brackets!) with printext16 markup.
local function replchev(matchstr) return "^15"..matchstr:sub(2,-2).."^O" end
-- Replace ASCII code escapes like \XXX. We can't use these escapes in Lua [[ ... ]] strings.
local function replascii(matchstr) return string.char(tonumber(matchstr)) end
-- Format a whole string for the menu system:
local function formatHelp(str)
    return str:gsub(
        "(%b<>)", replchev):gsub(
        "%*(.-)%*", "^014%1^O"):gsub(
        "%\\(%d+)", replascii)
end

----------

engine.clearMenu()

engine.registerMenuFunc(
    "Create shadexfog palset",
    CreateMenuFunction{
        [0] = shadexfog.create,
        { "Starting palnum", 100, MAXUSERPALOOKUP-31 },
        { "Red fog color [0-255]", 0, 255 },
        { "Green fog color [0-255]", 0, 255 },
        { "Blue fog color [0-255]", 0, 255 },
    },

formatHelp
[[
<shadexfog.create(startpalnum, fogr, fogg, fogb)>
<_______________________________________________>

Creates 32 shade tables corresponding to different *shade levels*
of a fog palookup, together called a *shade-x-fog* palookup set.

 Pals <startpalnum> to <startpalnum>+31 will be taken.
 <fogr>, <fogg>, <fogb>: intensities of the fog color, [0 .. 255]
]]
)

engine.registerMenuFunc(
    "Translate map for shxfog",
    CreateMenuFunction{
        [0] = shadexfog.translate,
        { "Starting palnum", 100, MAXUSERPALOOKUP-31 },
        { "Fog intensity [0-31]", 0, 31 },
        { "Change all sectors' visibility to (Esc: don't)", 0, 255, GNF.NEG_ALLOWED },
    },

formatHelp
[[
<shadexfog.translate(startpalnum, fogintensity [, vis])>
<______________________________________________________>

Translates the whole map for use with a shade-x-fog palookup set.

 .pal becomes the <startpalnum> + former .shade
 .shade becomes the <fogintensity> [0 .. 31]

 If <vis> is passed and >= 0, set all sector's visibility to that
 value.

 *Notes:*
  - auto-detects when the translation has been applied with the *same*
    <startpalnum> (if a different one is desired, must reload map).
  - if shades > 31 or < 0 present, there is loss of information
]]
)

engine.registerMenuFunc(
    "Change pal of everything",
    CreateMenuFunction{
        [0] = shadexfog.challpal,
        { "Pal to change to", 0, MAXUSERPALOOKUP },
    },

formatHelp
[[
<shadexfog.challpal(palnum)>
<__________________________>

Changes the .pal member of all sector ceilings/floors, walls and
sprites to <palnum>.
]]
)

engine.registerMenuFunc(
    "Create alpha trans. tabs",
    CreateMenuFunction{
        [0] = shadexfog.create_alpha_trans,
        { "Starting blendnum", 1, 255 },
        { "Number of blending tables", 32, 255 },
        { "Fullbright result colors OK?", 0, 1, GNF_BOOL },
    },

formatHelp
[=[
<shadexfog.create_alpha_trans(startblend [, numtables [, fullbriOK]])>
<____________________________________________________________________>

Creates <numtables> blending tables of smooth alpha translucency,
starting with the blending number <startblend>, with values of alpha

  1/(2\255<numtables>), 2/(2\255<numtables>) ... <numtables>/(2\255<numtables>).

 <numtables> must be a power of two in [1 .. 128].

 <fullbriOK>: should fullbright color indices (>= 240) be permitted as
            the blending result of two color indices?
]=]
)

engine.registerMenuFunc(
    "Create addtv. trans. tabs",
    CreateMenuFunction{
        [0] = shadexfog.create_additive_trans,
        { "Starting blendnum", 1, 255 },
        { "Number of blending tables", 32, 255 },
        { "Fullbright result colors OK?", 0, 1, GNF_BOOL },
    },

formatHelp
[=[
<shadexfog.create_additive_trans(startbl [, numtables [, fullbriOK]])>
<____________________________________________________________________>

Creates <numtables> blending tables of smooth additive translucency,
starting with the blending number <startbl>, with factors for the
background color

  1/<numtables>, 2/<numtables> ... <numtables>/<numtables>.

 <numtables> must be a power of two in [1 .. 128].

 <fullbriOK>: should fullbright color indices (>= 240) be permitted as
            the blending result of two color indices?
]=]
)

engine.registerMenuFunc(
    "Create bri.pass tr. tabs",
    CreateMenuFunction{
        [0] = shadexfog.create_brightpass_trans,
        { "Starting blendnum", 1, 255 },
        { "Number of blending tables", 32, 255 },
        { "Fullbright result colors OK?", 0, 1, GNF_BOOL },
    },

formatHelp
[=[
<shadexfog.create_brightpass_trans(startbl [, numtabs [, fullbriOK]])>
<____________________________________________________________________>

Creates <numtabs> blending tables of "brightpass" translucency,
starting with the blending number <startbl>, with fractions

  1/<numtables>, 2/<numtables> ... <numtables>/<numtables>.

 <fullbriOK>: should fullbright color indices (>= 240) be permitted as
            the blending result of two color indices?
]=]
)

engine.registerMenuFunc(
    "Create base shade table",
    CreateMenuFunction{
        [0] = shadexfog.create0,
        { "Pal number", 100, MAXUSERPALOOKUP },
        { "Second attempt?", 1, 1, GNF_BOOL },
    },

formatHelp
[[
<shadexfog.create0(palnum, secver)>
<_________________________________>

Creates our version of the base shade table at set it to palookup
number <palnum>.

 <secver>: use second attempt instead of the first? This one is more
   similar to the base shade table shipped with Duke3D, but still
   shows significant differences.
]]
)

engine.registerMenuFunc(
    "Create c.index remapping",
    function()
        local palnum = getnumber16("Pal number: ", 100, MAXUSERPALOOKUP)
        if (palnum < 0) then return end

        local remaptab = {}
        while (true) do
            local srchex = getnumber16("Source hexadecatuple (0: finish): ", 0, 14)
            if (srchex < 0) then return end
            local dsthex = getnumber16("Destn. hexadecatuple (0: finish): ", 0, 14)
            if (dsthex < 0) then return end

            if (srchex == 0 and dsthex == 0) then
                break
            end

            remaptab[srchex] = dsthex
        end

        shadexfog.createremap(palnum, remaptab)
    end,

formatHelp
[[
<shadexfog.createremap(palnum, remaptab)>
<_______________________________________>

Creates a color index remapping expressed as mappings of sexdeca-
tuples (16-tuples) of the base palette at pal <palnum>.

 Duke3D's default base palette can be considered to consist of six
 ramps of 32 colors each, three ramps of 16 colors each and a
 remaining fullbright color set. The sexdecatuples are as follows:

<  0,  1>: gray ramp
<  2,  3>: skin color ramp
<  5,  4>: blue ramp (note that the 16-tuples are in reverse order)
<  6,  7>: nightvision yellow/green
<  8, 13>: red ramp
< 10, 11>: almost gray ramp, but with a slight red hue

    <  9>: yellow (slightly more red than green)
    < 12>: "dirty" orange
    < 14>: blue-purple-red
]]
)

local function getNumberRange(what, whating)
    local str = engine.getstring("Additional "..what.." numbers (e.g. '64,100-131,255'): ")
    if (str == nil) then return end
    if (str == "") then return {} end

    if (not str:find("^[%d,%-]+$")) then
        error("Additional "..whating.." numbers string must contain only digits or ',' or '-'", 2)
    end

    local moreblends = {}
    local didnumstr = {}

    for n1, n2 in str:gmatch("(%d+)%-(%d+)") do  -- parse number ranges
        moreblends[#moreblends+1] = { tonumber(n1), tonumber(n2) }
        didnumstr[n1] = true
        didnumstr[n2] = true
    end

    for n in str:gmatch("%d+") do  -- parse single numbers
        if (not didnumstr[n]) then
            moreblends[#moreblends+1] = tonumber(n)
        end
    end

    return moreblends
end

engine.registerMenuFunc(
    "Save pal+sh+trans DAT f.",
    function()
        local filename = engine.getstring("File name: ")
        if (filename == nil) then return end

        local palnum = getnumber16("Pal number of base shade table: ", 0, MAXUSERPALOOKUP)
        if (palnum < 0) then return end
        local blendnum = getnumber16("Blendnum of base transluc. table: ", 0, 255)
        if (blendnum < 0) then return end

        local moreblends = getNumberRange("blend", "blending")
        if (moreblends == nil) then return end

        local lognumalphatabs
        if (#moreblends > 0) then
            lognumalphatabs = getnumber16("log2 of last alpha blending table index (1-7, 0: none): ", 0, 7)
            if (lognumalphatabs < 0) then return end
            if (lognumalphatabs == 0) then lognumalphatabs = nil end
        end

        shadexfog.save(filename, palnum, blendnum, moreblends, lognumalphatabs)
    end,

formatHelp
[[
<shadexfog.save(filename, palnum, blendnum, moreblends, lognumalpha)>
<___________________________________________________________________>

Writes out a full PALETTE.DAT-formatted file named <filename> with the
base shade table numbered <palnum> and the base translucency table
numbered <blendnum>.

Finally, you are asked to specify additional blending tables that can
be stored in EDuke32's extended PALETTE.DAT format. If one or more
additional blending table is specified, you are also queried for the
log2 of the last alpha blending table index, <lognumalpha>. Since alpha
blending tables are assumed to be set up at indices 1 to
exp(2, <lognumalpha>), it is also the log2 of their total count.
]]
)

engine.registerMenuFunc(
    "Save lookups DAT file",
    function()
        local filename = engine.getstring("File name: ")
        if (filename ~= nil and filename ~= "") then
            local lookups = {
                -- Duke3D 1.5 LOOKUP.DAT order
                1,2,6,7,8, 3,4,5,9,10,
                12,13,15,16,18, 19,11,14,17,20,
                21,22,23,24,25
            }

            local morelookups = getNumberRange("lookup", "lookup")
            if (morelookups == nil) then return end

            if (#morelookups > 0) then
                for i=1,#morelookups do
                    lookups[#lookups+1] = morelookups[i]
                end
            end

            shadexfog.saveLookupDat(filename, lookups)
        end
    end,

formatHelp
[[
<shadexfog.saveLookupDat(filename, lookups)>
<__________________________________________>

Saves the color index lookups (i.e. first 256 values of each shade
table) of the pal numbers which have lookups in Duke3D's unaltered
LOOKUP.DAT, plus optional ones provided by the user.

The default ones are, in this order:
1,2,6,7,8, 3,4,5,9,10, 12,13,15,16,18, 19,11,14,17,20, 21,22,23,24,25.
(All pal numbers from 1 to 25 are present.)

 <filename>: the name of the LOOKUP.DAT-formatted file to create
]]
)

engine.registerMenuFunc("_________DEBUG_________", function() end
--[[
,
" \01\02\03\04\05\06\07\08\09\10\11\12\13\14\15\
\16\17\18\19\20\21\22\23\24\25\26\27\28\29\30\31\
\128\129\130\131\132\133\134\135\136\137\138\139\140\141\142\143\
\144\145\146\147\148\149\150\151\152\153\154\155\156\157\158\159\
\160\161\162\163\164\165\166\167\168\169\170\171\172\173\174\175\
\176\177\178\179\180\181\182\183\184\185\186\187\188\189\190\191\
\192\193\194\195\196\197\198\199\200\201\202\203\204\205\206\207\
\208\209\210\211\212\213\214\215\216\217\218\219\220\221\222\223\
\224\225\226\227\228\229\230\231\232\233\234\235\236\237\238\239\
\240\241\242\243\244\245\246\247\248\249\250\251\252\253\254\255"
--]]
)

engine.registerMenuFunc("Setup dbg. water basepal", engine.setupDebugBasePal,
formatHelp
[[
<engine.setupDebugBasePal()>
<__________________________>

Overwrites the water base palette with one where each 16-tuple
(except the fullbrights) consists of a single representative
color.

This can be used to get a quick glimpse about what ramps are present
in particular tiles. With this information, custom lookups can be
created more directedly with the <Create c.index remapping> menu
entry.
]]
)

engine.registerMenuFunc(
    "Create depth shade tab",
    CreateMenuFunction{
        [0] = shadexfog.create_depth_shtab,
        { "Pal number", 100, MAXUSERPALOOKUP },
    },

formatHelp
[[
<shadexfog.create_depth_shtab(palnum)>
<____________________________________>

Creates a shade table for debugging purposes at pal <palnum>.

 For every color index, the shade table maps shade index <i> to
 color index <i> (of the first ramp of gray colors, assuming Build
 has loaded a base shade table with 32 shade levels).
]]
)

engine.registerMenuFunc(
    "Create vismarker sh. tab",
    CreateMenuFunction{
        [0] = shadexfog.create_vismarker_shtab,
        { "Pal number", 100, MAXUSERPALOOKUP },
    },

formatHelp
[[
<shadexfog.create_vismarker_shtab(palnum)>
<________________________________________>

Creates a shade table for debugging purposes at pal <palnum>.

 For every color index, the shade table maps shade index 1 to
 a ^14bright yellow^O color and shade index 30 to a ^13purple^O color.
 Thus, it can be useful in visualizing the limits of the
 fog/visibility attenuation.
]]
)

engine.registerMenuFunc("Linearize default basep.", engine.linearizeBasePal,
formatHelp
[[
<engine.linearizeBasePal()>
<_________________________>

Overwrites the default base palette with one where certain ramps have
their attenuation linearized. This is mainly useful for debugging
purposes as it excludes the effect of this nonlinearity for
comparison of fog/visibility between classic and OpenGL modes.
]]
)


do
    return shadexfog
end
