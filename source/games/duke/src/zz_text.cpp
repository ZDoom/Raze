//-------------------------------------------------------------------------
/*
Copyright (C) 2016 EDuke32 developers and contributors

This file is part of EDuke32.

EDuke32 is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License version 2
as published by the Free Software Foundation.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/
//-------------------------------------------------------------------------
#include "ns.h"	// Must come before everything else!

#include "duke3d.h"
#include "compat.h"
#include "sbar.h"
#include "menus.h"
#include "gstrings.h"

BEGIN_DUKE_NS


// common font types
// tilenums are set after namesdyn runs.
// These are also modifiable by scripts.
//                                      emptychar x,y       between x,y         zoom                cursorLeft          cursorCenter        cursorScale          textflags
//                                      tilenum             shade_deselected    shade_disabled      pal                 pal_selected        pal_deselected       pal_disabled
MenuFont_t MF_Redfont = { { 5 << 16, 15 << 16 },  { 0, 0 },           65536,              20 << 16,             110 << 16,            65536, 65536, 65536, TEXT_BIGALPHANUM | TEXT_UPPERCASE,
                                        -1,                 10,                 0,                  0,                  0,                  0,                   1,
                                        0,                  0,                  1 };
MenuFont_t MF_Bluefont = { { 5 << 16, 7 << 16 },   { 0, 0 },           65536,              10 << 16,             110 << 16,            32768, 65536, 65536, 0,
                                        -1,                 10,                 0,                  0,                  10,                 10,                  16,
                                        0,                  0,                  16 };
MenuFont_t MF_Minifont = { { 4 << 16, 5 << 16 },   { 1 << 16, 1 << 16 },   65536,              10 << 16,             110 << 16,            32768, 65536, 65536, 0,
                                        -1,                 10,                 0,                  0,                  2,                  2,                   0,
                                        0,                  0,                  16 };


/*
This function prepares data after ART and CON have been processed.
It also initializes some data in loops rather than statically at compile time.
*/

void Menu_Init(void)
{

    // prepare menu fonts
    // check if tilenum is -1 in case it was set in EVENT_SETDEFAULTS
    if ((unsigned)MF_Redfont.tilenum >= MAXTILES) MF_Redfont.tilenum = TILE_BIGALPHANUM;
    if ((unsigned)MF_Bluefont.tilenum >= MAXTILES) MF_Bluefont.tilenum = TILE_STARTALPHANUM;
    if ((unsigned)MF_Minifont.tilenum >= MAXTILES) MF_Minifont.tilenum = TILE_MINIFONT;
    MF_Redfont.emptychar.y = tilesiz[MF_Redfont.tilenum].y << 16;
    MF_Bluefont.emptychar.y = tilesiz[MF_Bluefont.tilenum].y << 16;
    MF_Minifont.emptychar.y = tilesiz[MF_Minifont.tilenum].y << 16;
    if (!minitext_lowercase)
        MF_Minifont.textflags |= TEXT_UPPERCASE;



    if (RR)
    {
        MF_Redfont.zoom = 32768;
        MF_Redfont.emptychar.x <<= 1;
        MF_Redfont.cursorScale = 13107;
        MF_Redfont.cursorScale2 = 6553;
        //MF_Redfont.emptychar.y <<= 1;
        MF_Bluefont.zoom = 32768;
        MF_Bluefont.emptychar.x <<= 1;
        MF_Bluefont.cursorScale = 6553;
        MF_Bluefont.cursorScale2 = 6553;
        //MF_Bluefont.emptychar.y <<= 1;
        MF_Minifont.zoom = 32768;
        MF_Minifont.emptychar.x <<= 1;
        MF_Minifont.cursorScale = 6553;
        MF_Minifont.cursorScale2 = 6553;
        //MF_Minifont.emptychar.y <<= 1;
    }

}



// assign the character's tilenum
int GameInterface::GetStringTile(int font, const char* t, int f)
{
    if (f & TEXT_DIGITALNUMBER)
        return *t - '0' + font; // copied from digitalnumber
    else if (f & (TEXT_BIGALPHANUM|TEXT_GRAYFONT))
    {
        int32_t offset = (f & TEXT_GRAYFONT) ? 26 : 0;

        if (*t >= '0' && *t <= '9')
            return *t - '0' + font + ((f & TEXT_GRAYFONT) ? 26 : -10);
        else if (*t >= 'a' && *t <= 'z')
            return *t - 'a' + font + ((f & TEXT_GRAYFONT) ? -26 : 26);
        else if (*t >= 'A' && *t <= 'Z')
            return *t - 'A' + font;
        else switch (*t)
        {
        case '_':
        case '-':
            return font - (11 + offset);
            break;
        case '.':
            return font + (TILE_BIGPERIOD - (TILE_BIGALPHANUM + offset));
            break;
        case ',':
            return font + (TILE_BIGCOMMA - (TILE_BIGALPHANUM + offset));
            break;
        case '!':
            return font + (TILE_BIGX_ - (TILE_BIGALPHANUM + offset));
            break;
        case '?':
            return font + (TILE_BIGQ - (TILE_BIGALPHANUM + offset));
            break;
        case ';':
            return font + (TILE_BIGSEMI - (TILE_BIGALPHANUM + offset));
            break;
        case ':':
            return font + (TILE_BIGCOLIN - (TILE_BIGALPHANUM + offset));
            break;
        case '\\':
        case '/':
            return font + (68 - offset); // 3008-2940
            break;
        case '%':
            return font + (69 - offset); // 3009-2940
            break;
        case '`':
        case '\"': // could be better hacked in
        case '\'':
            return font + (TILE_BIGAPPOS - (TILE_BIGALPHANUM + offset));
            break;
        default: // unknown character
            fallthrough__;
        case '\t':
        case ' ':
        case '\n':
        case '\x7F':
            return font;
            break;
        }
    }
    else
    {
        int tt = *t;
        if (tt >= 'a' && tt <= 'z') tt -= 32;
        return tt - '!' + font; // uses ASCII order
    }
}


vec2_t gametext_(int32_t x, int32_t y, const char *t, int32_t s, int32_t p, int32_t o, int32_t a, int32_t f)
{
    return G_ScreenText(MF_Bluefont.tilenum, x, y, MF_Bluefont.zoom, 0, 0, t, s, p, o|2|8|16|ROTATESPRITE_FULL16, a, MF_Bluefont.emptychar.x, MF_Bluefont.emptychar.y, MF_Bluefont.between.x, MF_Bluefont.between.y, MF_Bluefont.textflags|f, 0, 0, xdim-1, ydim-1);
}
static int32_t sbarx16(int32_t x)
{
    if (ud.screen_size == 4) return sbarsc(x);
    return (((320 << 16) - sbarsc(320 << 16)) >> 1) + sbarsc(x);
}

static int32_t sbarxr16(int32_t x)
{
    if (ud.screen_size == 4) return (320 << 16) - sbarsc(x);
    return (((320 << 16) - sbarsc(320 << 16)) >> 1) + sbarsc(x);
}

static int32_t sbary16(int32_t y)
{
    return (100 << 16) - sbarsc(200 << 16) + sbarsc(y);
}

// minitext_yofs: in hud_scale-independent, (<<16)-scaled, 0-200-normalized y coords,
// (sb&ROTATESPRITE_MAX) only.
int32_t minitext_yofs = 0;
int32_t minitext_lowercase = 0;
int32_t minitext_(int32_t x, int32_t y, const char *t, int32_t s, int32_t p, int32_t sb)
{
    vec2_t dim;
    int32_t z = MF_Minifont.zoom;

    if (t == NULL)
    {
        Printf("minitext: NULL text!\n");
        return 0;
    }

    if (!(sb & ROTATESPRITE_FULL16))
    {
        x<<=16;
        y<<=16;
    }

    if (sb & ROTATESPRITE_MAX)
    {
        if (sb & RS_ALIGN_R)
            x = sbarxr16(x);
        else
            x = sbarx16(x);
        y = minitext_yofs+sbary16(y);
        z = sbarsc(z);
    }

    sb &= (ROTATESPRITE_MAX-1)|RS_CENTERORIGIN;

    dim = G_ScreenText(MF_Minifont.tilenum, x, y, z, 0, 0, t, s, p, sb|ROTATESPRITE_FULL16, 0, MF_Minifont.emptychar.x, MF_Minifont.emptychar.y, MF_Minifont.between.x, MF_Minifont.between.y, MF_Minifont.textflags, 0, 0, xdim-1, ydim-1);

    x += dim.x;

    if (!(sb & ROTATESPRITE_FULL16))
        x >>= 16;

    return x;
}

void menutext_(int32_t x, int32_t y, int32_t s, char const *t, int32_t o, int32_t f)
{
    if (RR) f |= TEXT_RRMENUTEXTHACK;
    G_ScreenText(MF_Redfont.tilenum, x, y - (12<<16), MF_Redfont.zoom, 0, 0, t, s, MF_Redfont.pal, o|ROTATESPRITE_FULL16, 0, MF_Redfont.emptychar.x, MF_Redfont.emptychar.y, MF_Redfont.between.x, MF_Redfont.between.y, f|MF_Redfont.textflags|TEXT_LITERALESCAPE, 0, 0, xdim-1, ydim-1);
}


END_DUKE_NS
