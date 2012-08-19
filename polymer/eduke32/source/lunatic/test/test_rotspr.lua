
local con = require("con")
local bit = require("bit")
local math = require("math")

local rs = con.rotatesprite

local DOT1x5 = 3135
local BAR1x5 = 3163

local function draw_hline_dotted(x1, x2, y, pal,stat)
    for x=x1,x2,2 do
        local tile = (x==x1 or x==x2) and BAR1x5 or DOT1x5
        if (player[0].curr_weapon==2) then
            x = x + 16*math.sin(2*math.pi*gv.totalclock/120)
        end
        rs(x,y, 65536, 0, tile, 0,pal,stat, 0,0,gv.xdim-1,gv.ydim-1)
    end
end

local function draw_two_hlines(y, stat)
    if (player[0].curr_weapon ~= 3) then
        stat = bit.bor(stat, 8)
    end
    draw_hline_dotted(0,160, y, 0, stat)
    -- XXX: rotatesprite(*, 320, ...) with stat 1024 and classic draws out of
    -- bounds (wraps to lower left corner).  (Or is it the left one? In any case,
    -- weird stuff happens.)
    draw_hline_dotted(162,318, y, 2, stat)
end


local function rotatesprite_test()
    local stats = { [0]=0, 256, 512, 1024 }
    for i=0,3 do
        local stat = stats[i]
        draw_two_hlines(50+25*i, stats[i])
    end

    -- NUKEBUTTON
    rs(30,170, 32768, 2047*((gv.totalclock/240)%1), 142, 0,0,8+1024, 0,0,gv.xdim-1,gv.ydim-1)
end

gameevent(gv.EVENT_DISPLAYREST, rotatesprite_test)
