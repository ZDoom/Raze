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

#include "duke3d.h"
#include "compat.h"
#include "sbar.h"

int32_t quotebot, quotebotgoal;
int32_t user_quote_time[MAXUSERQUOTES];
static char user_quote[MAXUSERQUOTES][178];
int32_t hud_glowingquotes = 1;

#define FTAOPAQUETIME 30

// alpha increments of 8 --> 256 / 8 = 32 --> round up to power of 2 --> 32 --> divide by 2 --> 16 alphatabs required
#define ftapulseshade                                                                                                  \
    ((hud_glowingquotes && ((getrendermode() == REND_CLASSIC && numalphatabs < 15) || ps->fta >= FTAOPAQUETIME)) ?     \
     (sintable[((uint32_t)ps->fta << 7) & 2047] >> 11) :                                                               \
     (sintable[((uint32_t)FTAOPAQUETIME << 7) & 2047] >> 11))

int32_t textsc(int32_t sc)
{
    // prevent ridiculousness to a degree
    if (xdim <= 320) return sc;
    else if (xdim <= 640) return scale(sc, min(200, ud.textscale), 100);
    else if (xdim <= 800) return scale(sc, min(300, ud.textscale), 100);
    else if (xdim <= 1024) return scale(sc, min(350, ud.textscale), 100);
    return scale(sc, ud.textscale, 100);
}

// get the string length until the next '\n'
int32_t G_GetStringLineLength(const char *text, const char *end, const int32_t iter)
{
    int32_t length = 0;

    while (*text != '\n' && text != end)
    {
        ++length;

        text += iter;
    }

    return length;
}

int32_t G_GetStringNumLines(const char *text, const char *end, const int32_t iter)
{
    int32_t count = 1;

    while (text != end)
    {
        if (*text == '\n')
            ++count;
        text += iter;
    }

    return count;
}
// Note: Neither of these care about TEXT_LINEWRAP. This is intended.

// This function requires you to Bfree() the returned char*.
char* G_GetSubString(const char *text, const char *end, const int32_t iter, const int32_t length)
{
    char *line = (char*) Xmalloc((length+1) * sizeof(char));
    int32_t counter = 0;

    while (counter < length && text != end)
    {
        line[counter] = *text;

        text += iter;
        ++counter;
    }

    line[counter] = '\0';

    return line;
}

// assign the character's tilenum
int32_t G_GetStringTile(int32_t font, char *t, int32_t f)
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
            return font + (BIGPERIOD - (BIGALPHANUM + offset));
            break;
        case ',':
            return font + (BIGCOMMA - (BIGALPHANUM + offset));
            break;
        case '!':
            return font + (BIGX_ - (BIGALPHANUM + offset));
            break;
        case '?':
            return font + (BIGQ - (BIGALPHANUM + offset));
            break;
        case ';':
            return font + (BIGSEMI - (BIGALPHANUM + offset));
            break;
        case ':':
            return font + (BIGCOLIN - (BIGALPHANUM + offset));
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
            return font + (BIGAPPOS - (BIGALPHANUM + offset));
            break;
        default: // unknown character
            *t = ' '; // whitespace-ize
        case '\n':
            return font;
            break;
        }
    }
    else
        return *t - '!' + font; // uses ASCII order
}

#define NUMHACKACTIVE ((f & TEXT_GAMETEXTNUMHACK) && t >= '0' && t <= '9')

// qstrdim
vec2_t G_ScreenTextSize(const int32_t font,
    int32_t x, int32_t y, const int32_t z, const int32_t blockangle,
    const char *str, const int32_t o,
    int32_t xspace, int32_t yline, int32_t xbetween, int32_t ybetween,
    const int32_t f,
    int32_t x1, int32_t y1, int32_t x2, int32_t y2)
{
    vec2_t size ={ 0, 0, }; // eventually the return value
    vec2_t pos ={ 0, 0, }; // holds the coordinate position as we draw each character tile of the string
    vec2_t extent ={ 0, 0, }; // holds the x-width of each character and the greatest y-height of each line
    vec2_t offset ={ 0, 0, }; // temporary; holds the last movement made in both directions

    int32_t tile;
    char t;

    // set the start and end points depending on direction
    int32_t iter = (f & TEXT_BACKWARDS) ? -1 : 1; // iteration direction

    const char *end;
    const char *text;

    if (str == NULL)
        return size;

    end = (f & TEXT_BACKWARDS) ? str-1 : Bstrchr(str, '\0');
    text = (f & TEXT_BACKWARDS) ? Bstrchr(str, '\0')-1 : str;

    // optimization: justification in both directions
    if ((f & TEXT_XJUSTIFY) && (f & TEXT_YJUSTIFY))
    {
        size.x = xbetween;
        size.y = ybetween;
        return size;
    }

    // for best results, we promote 320x200 coordinates to full precision before any math
    if (!(o & ROTATESPRITE_FULL16))
    {
        x <<= 16;
        y <<= 16;
        xspace <<= 16;
        yline <<= 16;
        xbetween <<= 16;
        ybetween <<= 16;
    }
    // coordinate values should be shifted left by 16

    // handle zooming where applicable
    xspace = scale(xspace, z, 65536);
    yline = scale(yline, z, 65536);
    xbetween = scale(xbetween, z, 65536);
    ybetween = scale(ybetween, z, 65536);
    // size/width/height/spacing/offset values should be multiplied or scaled by $z, zoom (since 100% is 65536, the same as 1<<16)

    // loop through the string
    while ((t = *text) && text != end)
    {
        // handle escape sequences
        if (t == '^' && Bisdigit(*(text+iter)) && !(f & TEXT_LITERALESCAPE))
        {
            text += iter + iter;
            if (Bisdigit(*text))
                text += iter;
            continue;
        }

        // handle case bits
        if (f & TEXT_UPPERCASE)
        {
            if (f & TEXT_INVERTCASE) // optimization...?
            { // v^ important that these two ifs remain separate due to the else below
                if (Bisupper(t))
                    t = Btolower(t);
            }
            else if (Bislower(t))
                t = Btoupper(t);
        }
        else if (f & TEXT_INVERTCASE)
        {
            if (Bisupper(t))
                t = Btolower(t);
            else if (Bislower(t))
                t = Btoupper(t);
        }

        // translate the character to a tilenum
        tile = G_GetStringTile(font, &t, f);

        // reset this here because we haven't printed anything yet this loop
        extent.x = 0;

        // reset this here because the act of printing something on this line means that we include the margin above in the total size
        offset.y = 0;

        // handle each character itself in the context of screen drawing
        switch (t)
        {
        case '\t':
        case ' ':
            // width
            extent.x = xspace;

            if (f & (TEXT_INTERNALSPACE|TEXT_TILESPACE))
            {
                char space = '.'; // this is subject to change as an implementation detail
                if (f & TEXT_TILESPACE)
                    space = '\x7F'; // tile after '~'
                tile = G_GetStringTile(font, &space, f);

                extent.x += (tilesiz[tile].x * z);
            }

            // prepare the height // near-CODEDUP the other two near-CODEDUPs for this section
            {
                int32_t tempyextent = yline;

                if (f & (TEXT_INTERNALLINE|TEXT_TILELINE))
                {
                    char line = 'A'; // this is subject to change as an implementation detail
                    if (f & TEXT_TILELINE)
                        line = '\x7F'; // tile after '~'
                    tile = G_GetStringTile(font, &line, f);

                    tempyextent += tilesiz[tile].y * z;
                }

                SetIfGreater(&extent.y, tempyextent);
            }

            if (t == '\t')
                extent.x <<= 2; // *= 4

            break;

        case '\n': // near-CODEDUP "if (wrap)"
            extent.x = 0;

            // save the position
            if (!(f & TEXT_XOFFSETZERO)) // we want the entire offset to count as the character width
                pos.x -= offset.x;
            SetIfGreater(&size.x, pos.x);

            // reset the position
            pos.x = 0;

            // prepare the height
            {
                int32_t tempyextent = yline;

                if (f & (TEXT_INTERNALLINE|TEXT_TILELINE))
                {
                    char line = 'A'; // this is subject to change as an implementation detail
                    if (f & TEXT_TILELINE)
                        line = '\x7F'; // tile after '~'
                    tile = G_GetStringTile(font, &line, f);

                    tempyextent += tilesiz[tile].y * z;
                }

                SetIfGreater(&extent.y, tempyextent);
            }

            // move down the line height
            if (!(f & TEXT_YOFFSETZERO))
                pos.y += extent.y;

            // reset the current height
            extent.y = 0;

            // line spacing
            offset.y = (f & TEXT_YJUSTIFY) ? 0 : ybetween; // ternary to prevent overflow
            pos.y += offset.y;

            break;

        default:
            // width
            extent.x = tilesiz[tile].x * z;

            // obnoxious hardcoded functionality from gametext
            if (NUMHACKACTIVE)
            {
                char numeral = '0'; // this is subject to change as an implementation detail
                extent.x = (tilesiz[G_GetStringTile(font, &numeral, f)].x-1) * z;
            }

            // height
            SetIfGreater(&extent.y, (tilesiz[tile].y * z));

            break;
        }

        // incrementing the coordinate counters
        offset.x = 0;

        // advance the x coordinate
        if (!(f & TEXT_XOFFSETZERO) || NUMHACKACTIVE)
            offset.x += extent.x;

        // account for text spacing
        if (!NUMHACKACTIVE // this "if" line ONLY == replicating hardcoded stuff
            && t != '\n'
            && !(f & TEXT_XJUSTIFY)) // to prevent overflow
            offset.x += xbetween;

        // line wrapping
        if ((f & TEXT_LINEWRAP) && !(f & TEXT_XRIGHT) && !(f & TEXT_XCENTER) && blockangle % 512 == 0)
        {
            int32_t wrap = 0;
            const int32_t ang = blockangle % 2048;

            // this is the only place in qstrdim where angle actually affects direction, but only in the wrapping measurement
            switch (ang)
            {
            case 0:
                wrap = (x + (pos.x + offset.x) > ((o & 2) ? (320<<16) : ((x2 - USERQUOTE_RIGHTOFFSET)<<16)));
                break;
            case 512:
                wrap = (y + (pos.x + offset.x) > ((o & 2) ? (200<<16) : ((y2 - USERQUOTE_RIGHTOFFSET)<<16)));
                break;
            case 1024:
                wrap = (x - (pos.x + offset.x) < ((o & 2) ? 0 : ((x1 + USERQUOTE_RIGHTOFFSET)<<16)));
                break;
            case 1536:
                wrap = (y - (pos.x + offset.x) < ((o & 2) ? 0 : ((y1 + USERQUOTE_RIGHTOFFSET)<<16)));
                break;
            }
            if (wrap) // near-CODEDUP "case '\n':"
            {
                // save the position
                SetIfGreater(&size.x, pos.x);

                // reset the position
                pos.x = 0;

                // prepare the height
                {
                    int32_t tempyextent = yline;

                    if (f & (TEXT_INTERNALLINE|TEXT_TILELINE))
                    {
                        char line = 'A'; // this is subject to change as an implementation detail
                        if (f & TEXT_TILELINE)
                            line = '\x7F'; // tile after '~'
                        tile = G_GetStringTile(font, &line, f);

                        tempyextent += tilesiz[tile].y * z;
                    }

                    SetIfGreater(&extent.y, tempyextent);
                }

                // move down the line height
                if (!(f & TEXT_YOFFSETZERO))
                    pos.y += extent.y;

                // reset the current height
                extent.y = 0;

                // line spacing
                offset.y = (f & TEXT_YJUSTIFY) ? 0 : ybetween; // ternary to prevent overflow
                pos.y += offset.y;
            }
            else
                pos.x += offset.x;
        }
        else
            pos.x += offset.x;

        // save some trouble with calculation in case the line breaks
        if (!(f & TEXT_XOFFSETZERO) || NUMHACKACTIVE)
            offset.x -= extent.x;

        // iterate to the next character in the string
        text += iter;
    }

    // calculate final size
    if (!(f & TEXT_XOFFSETZERO))
        pos.x -= offset.x;

    if (!(f & TEXT_YOFFSETZERO))
    {
        pos.y -= offset.y;
        pos.y += extent.y;
    }
    else
        pos.y += ybetween;

    SetIfGreater(&size.x, pos.x);
    SetIfGreater(&size.y, pos.y);

    // justification where only one of the two directions is set, so we have to iterate
    if (f & TEXT_XJUSTIFY)
        size.x = xbetween;
    if (f & TEXT_YJUSTIFY)
        size.y = ybetween;

    // return values in the same manner we receive them
    if (!(o & ROTATESPRITE_FULL16))
    {
        size.x >>= 16;
        size.y >>= 16;
    }

    return size;
}

void G_AddCoordsFromRotation(vec2_t *coords, const vec2_t *unitDirection, const int32_t magnitude)
{
    coords->x += scale(magnitude, unitDirection->x, 16384);
    coords->y += scale(magnitude, unitDirection->y, 16384);
}

// screentext
vec2_t G_ScreenText(const int32_t font,
    int32_t x, int32_t y, const int32_t z, const int32_t blockangle, const int32_t charangle,
    const char *str, const int32_t shade, int32_t pal, int32_t o, int32_t alpha,
    int32_t xspace, int32_t yline, int32_t xbetween, int32_t ybetween, const int32_t f,
    const int32_t x1, const int32_t y1, const int32_t x2, const int32_t y2)
{
    vec2_t size ={ 0, 0, }; // eventually the return value
    vec2_t origin ={ 0, 0, }; // where to start, depending on the alignment
    vec2_t pos ={ 0, 0, }; // holds the coordinate position as we draw each character tile of the string
    vec2_t extent ={ 0, 0, }; // holds the x-width of each character and the greatest y-height of each line
    const vec2_t Xdirection ={ sintable[(blockangle+512)&2047], sintable[blockangle&2047], };
    const vec2_t Ydirection ={ sintable[(blockangle+1024)&2047], sintable[(blockangle+512)&2047], };

    int32_t blendidx=0, tile;
    char t;

    // set the start and end points depending on direction
    int32_t iter = (f & TEXT_BACKWARDS) ? -1 : 1; // iteration direction

    const char *end;
    const char *text;

    if (str == NULL)
        return size;

    NEG_ALPHA_TO_BLEND(alpha, blendidx, o);

    end = (f & TEXT_BACKWARDS) ? str-1 : Bstrchr(str, '\0');
    text = (f & TEXT_BACKWARDS) ? Bstrchr(str, '\0')-1 : str;

    // for best results, we promote 320x200 coordinates to full precision before any math
    if (!(o & ROTATESPRITE_FULL16))
    {
        x <<= 16;
        y <<= 16;
        xspace <<= 16;
        yline <<= 16;
        xbetween <<= 16;
        ybetween <<= 16;
    }
    // coordinate values should be shifted left by 16

    // eliminate conflicts, necessary here to get the correct size value
    // especially given justification's special handling in G_ScreenTextSize()
    if ((f & TEXT_XRIGHT) || (f & TEXT_XCENTER) || (f & TEXT_XJUSTIFY) || (f & TEXT_YJUSTIFY) || blockangle % 512 != 0)
        o &= ~TEXT_LINEWRAP;

    // size is the return value, and we need it for alignment
    size = G_ScreenTextSize(font, x, y, z, blockangle, str, o | ROTATESPRITE_FULL16, xspace, yline, (f & TEXT_XJUSTIFY) ? 0 : xbetween, (f & TEXT_YJUSTIFY) ? 0 : ybetween, f & ~(TEXT_XJUSTIFY|TEXT_YJUSTIFY), x1, y1, x2, y2);

    // handle zooming where applicable
    xspace = scale(xspace, z, 65536);
    yline = scale(yline, z, 65536);
    xbetween = scale(xbetween, z, 65536);
    ybetween = scale(ybetween, z, 65536);
    // size/width/height/spacing/offset values should be multiplied or scaled by $z, zoom (since 100% is 65536, the same as 1<<16)

    // alignment
    // near-CODEDUP "case '\n':"
    {
        int32_t lines = G_GetStringNumLines(text, end, iter);

        if ((f & TEXT_XJUSTIFY) || (f & TEXT_XRIGHT) || (f & TEXT_XCENTER))
        {
            const int32_t length = G_GetStringLineLength(text, end, iter);

            int32_t linewidth = size.x;

            if (lines != 1)
            {
                char *line = G_GetSubString(text, end, iter, length);

                linewidth = G_ScreenTextSize(font, x, y, z, blockangle, line, o | ROTATESPRITE_FULL16, xspace, yline, (f & TEXT_XJUSTIFY) ? 0 : xbetween, (f & TEXT_YJUSTIFY) ? 0 : ybetween, f & ~(TEXT_XJUSTIFY|TEXT_YJUSTIFY|TEXT_BACKWARDS), x1, y1, x2, y2).x;

                Bfree(line);
            }

            if (f & TEXT_XJUSTIFY)
            {
                size.x = xbetween;

                xbetween = (length == 1) ? 0 : tabledivide32_noinline((xbetween - linewidth), (length - 1));

                linewidth = size.x;
            }

            if (f & TEXT_XRIGHT)
                origin.x = -linewidth;
            else if (f & TEXT_XCENTER)
                origin.x = -(linewidth / 2);
        }

        if (f & TEXT_YJUSTIFY)
        {
            const int32_t tempswap = ybetween;
            ybetween = (lines == 1) ? 0 : tabledivide32_noinline(ybetween - size.y, lines - 1);
            size.y = tempswap;
        }

        if (f & TEXT_YBOTTOM)
            origin.y = -size.y;
        else if (f & TEXT_YCENTER)
            origin.y = -(size.y / 2);
    }

    // loop through the string
    while ((t = *text) && text != end)
    {
        int32_t orientation = o;
        int32_t angle = blockangle + charangle;

        // handle escape sequences
        if (t == '^' && Bisdigit(*(text+iter)) && !(f & TEXT_LITERALESCAPE))
        {
            char smallbuf[4];

            text += iter;
            smallbuf[0] = *text;

            text += iter;
            if (Bisdigit(*text))
            {
                smallbuf[1] = *text;
                smallbuf[2] = '\0';
                text += iter;
            }
            else
                smallbuf[1] = '\0';

            if (!(f & TEXT_IGNOREESCAPE))
                pal = Batoi(smallbuf);

            continue;
        }

        // handle case bits
        if (f & TEXT_UPPERCASE)
        {
            if (f & TEXT_INVERTCASE) // optimization...?
            { // v^ important that these two ifs remain separate due to the else below
                if (Bisupper(t))
                    t = Btolower(t);
            }
            else if (Bislower(t))
                t = Btoupper(t);
        }
        else if (f & TEXT_INVERTCASE)
        {
            if (Bisupper(t))
                t = Btolower(t);
            else if (Bislower(t))
                t = Btoupper(t);
        }

        // translate the character to a tilenum
        tile = G_GetStringTile(font, &t, f);

        switch (t)
        {
        case '\t':
        case ' ':
        case '\n':
        case '\x7F':
            break;

        default:
        {
            vec2_t location ={ x, y, };

            G_AddCoordsFromRotation(&location, &Xdirection, origin.x);
            G_AddCoordsFromRotation(&location, &Ydirection, origin.y);

            G_AddCoordsFromRotation(&location, &Xdirection, pos.x);
            G_AddCoordsFromRotation(&location, &Ydirection, pos.y);

            rotatesprite_(location.x, location.y, z, angle, tile, shade, pal, orientation, alpha, blendidx, x1, y1, x2, y2);

            break;
        }
        }

        // reset this here because we haven't printed anything yet this loop
        extent.x = 0;

        // handle each character itself in the context of screen drawing
        switch (t)
        {
        case '\t':
        case ' ':
            // width
            extent.x = xspace;

            if (f & (TEXT_INTERNALSPACE|TEXT_TILESPACE))
            {
                char space = '.'; // this is subject to change as an implementation detail
                if (f & TEXT_TILESPACE)
                    space = '\x7F'; // tile after '~'
                tile = G_GetStringTile(font, &space, f);

                extent.x += (tilesiz[tile].x * z);
            }

            // prepare the height // near-CODEDUP the other two near-CODEDUPs for this section
            {
                int32_t tempyextent = yline;

                if (f & (TEXT_INTERNALLINE|TEXT_TILELINE))
                {
                    char line = 'A'; // this is subject to change as an implementation detail
                    if (f & TEXT_TILELINE)
                        line = '\x7F'; // tile after '~'
                    tile = G_GetStringTile(font, &line, f);

                    tempyextent += tilesiz[tile].y * z;
                }

                SetIfGreater(&extent.y, tempyextent);
            }

            if (t == '\t')
                extent.x <<= 2; // *= 4

            break;

        case '\n': // near-CODEDUP "if (wrap)"
            extent.x = 0;

            // reset the position
            pos.x = 0;

            // prepare the height
            {
                int32_t tempyextent = yline;

                if (f & (TEXT_INTERNALLINE|TEXT_TILELINE))
                {
                    char line = 'A'; // this is subject to change as an implementation detail
                    if (f & TEXT_TILELINE)
                        line = '\x7F'; // tile after '~'
                    tile = G_GetStringTile(font, &line, f);

                    tempyextent += tilesiz[tile].y * z;
                }

                SetIfGreater(&extent.y, tempyextent);
            }

            // move down the line height
            if (!(f & TEXT_YOFFSETZERO))
                pos.y += extent.y;

            // reset the current height
            extent.y = 0;

            // line spacing
            pos.y += ybetween;

            // near-CODEDUP "alignments"
            if ((f & TEXT_XJUSTIFY) || (f & TEXT_XRIGHT) || (f & TEXT_XCENTER))
            {
                const int32_t length = G_GetStringLineLength(text+1, end, iter);

                char *line = G_GetSubString(text+1, end, iter, length);

                int32_t linewidth = G_ScreenTextSize(font, x, y, z, blockangle, line, o | ROTATESPRITE_FULL16, xspace, yline, (f & TEXT_XJUSTIFY) ? 0 : xbetween, (f & TEXT_YJUSTIFY) ? 0 : ybetween, f & ~(TEXT_XJUSTIFY|TEXT_YJUSTIFY|TEXT_BACKWARDS), x1, y1, x2, y2).x;

                Bfree(line);

                if (f & TEXT_XJUSTIFY)
                {
                    xbetween = (length == 1) ? 0 : tabledivide32_noinline(xbetween - linewidth, length - 1);

                    linewidth = size.x;
                }

                if (f & TEXT_XRIGHT)
                    origin.x = -linewidth;
                else if (f & TEXT_XCENTER)
                    origin.x = -(linewidth / 2);
            }

            break;

        default:
            // width
            extent.x = tilesiz[tile].x * z;

            // obnoxious hardcoded functionality from gametext
            if (NUMHACKACTIVE)
            {
                char numeral = '0'; // this is subject to change as an implementation detail
                extent.x = (tilesiz[G_GetStringTile(font, &numeral, f)].x-1) * z;
            }

            // height
            SetIfGreater(&extent.y, (tilesiz[tile].y * z));

            break;
        }

        // incrementing the coordinate counters
        {
            int32_t xoffset = 0;

            // advance the x coordinate
            if (!(f & TEXT_XOFFSETZERO) || NUMHACKACTIVE)
                xoffset += extent.x;

            // account for text spacing
            if (!NUMHACKACTIVE // this "if" line ONLY == replicating hardcoded stuff
                && t != '\n')
                xoffset += xbetween;

            // line wrapping
            if (f & TEXT_LINEWRAP)
            {
                int32_t wrap = 0;
                const int32_t ang = blockangle % 2048;

                // it's safe to make some assumptions and not go through G_AddCoordsFromRotation() since we limit to four directions
                switch (ang)
                {
                case 0:
                    wrap = (x + (pos.x + xoffset) > ((orientation & 2) ? (320<<16) : ((x2 - USERQUOTE_RIGHTOFFSET)<<16)));
                    break;
                case 512:
                    wrap = (y + (pos.x + xoffset) > ((orientation & 2) ? (200<<16) : ((y2 - USERQUOTE_RIGHTOFFSET)<<16)));
                    break;
                case 1024:
                    wrap = (x - (pos.x + xoffset) < ((orientation & 2) ? 0 : ((x1 + USERQUOTE_RIGHTOFFSET)<<16)));
                    break;
                case 1536:
                    wrap = (y - (pos.x + xoffset) < ((orientation & 2) ? 0 : ((y1 + USERQUOTE_RIGHTOFFSET)<<16)));
                    break;
                }
                if (wrap) // near-CODEDUP "case '\n':"
                {
                    // reset the position
                    pos.x = 0;

                    // prepare the height
                    {
                        int32_t tempyextent = yline;

                        if (f & (TEXT_INTERNALLINE|TEXT_TILELINE))
                        {
                            char line = 'A'; // this is subject to change as an implementation detail
                            if (f & TEXT_TILELINE)
                                line = '\x7F'; // tile after '~'
                            tile = G_GetStringTile(font, &line, f);

                            tempyextent += tilesiz[tile].y * z;
                        }

                        SetIfGreater(&extent.y, tempyextent);
                    }

                    // move down the line height
                    if (!(f & TEXT_YOFFSETZERO))
                        pos.y += extent.y;

                    // reset the current height
                    extent.y = 0;

                    // line spacing
                    pos.y += ybetween;
                }
                else
                    pos.x += xoffset;
            }
            else
                pos.x += xoffset;
        }

        // iterate to the next character in the string
        text += iter;
    }

    // return values in the same manner we receive them
    if (!(o & ROTATESPRITE_FULL16))
    {
        size.x >>= 16;
        size.y >>= 16;
    }

    return size;
}

vec2_t G_ScreenTextShadow(int32_t sx, int32_t sy,
    const int32_t font,
    int32_t x, int32_t y, const int32_t z, const int32_t blockangle, const int32_t charangle,
    const char *str, const int32_t shade, int32_t pal, int32_t o, const int32_t alpha,
    int32_t xspace, int32_t yline, int32_t xbetween, int32_t ybetween, const int32_t f,
    const int32_t x1, const int32_t y1, const int32_t x2, const int32_t y2)
{
    vec2_t size ={ 0, 0, }; // eventually the return value

    if (!(o & ROTATESPRITE_FULL16))
    {
        sx <<= 16;
        sy <<= 16;
        x <<= 16;
        y <<= 16;
        xspace <<= 16;
        yline <<= 16;
        xbetween <<= 16;
        ybetween <<= 16;
    }

    G_ScreenText(font, x + scale(sx, z, 65536), y + scale(sy, z, 65536), z, blockangle, charangle, str, 127, 4, o|ROTATESPRITE_FULL16, alpha, xspace, yline, xbetween, ybetween, f, x1, y1, x2, y2);

    size = G_ScreenText(font, x, y, z, blockangle, charangle, str, shade, pal, o|ROTATESPRITE_FULL16, alpha, xspace, yline, xbetween, ybetween, f, x1, y1, x2, y2);

    // return values in the same manner we receive them
    if (!(o & ROTATESPRITE_FULL16))
    {
        size.x >>= 16;
        size.y >>= 16;
    }

    return size;
}

// flags
//  4: small font, wrap strings?
int32_t G_PrintGameText(int32_t hack, int32_t tile, int32_t x, int32_t y, const char *t,
    int32_t s, int32_t p, int32_t o,
    int32_t x1, int32_t y1, int32_t x2, int32_t y2, int32_t z, int32_t a)
{
    vec2_t dim;
    int32_t f = TEXT_GAMETEXTNUMHACK;
    int32_t xbetween = 0;
    const int32_t orient = (hack & 4) || (hack & 1) ? (8|16|(o&1)|(o&32)) : (2|o);

    if (t == NULL)
        return -1;

    if (!(o & ROTATESPRITE_FULL16))
    {
        x <<= 16;
        y <<= 16;
    }

    if (hack & 4)
    {
        x = textsc(x);
        z = textsc(z);
        f |= TEXT_LINEWRAP;
    }

    if (hack & 8)
    {
        f |= TEXT_XOFFSETZERO;
        xbetween = 8;
    }

    // order is important, this bit comes after the rest
    if ((hack & 2) && !NAM_WW2GI) // squishtext
        --xbetween;

    if (x == (160<<16))
        f |= TEXT_XCENTER;

    dim = G_ScreenText(tile, x, y, z, 0, 0, t, s, p, orient|ROTATESPRITE_FULL16, a, (5<<16), (8<<16), (xbetween<<16), 0, f, x1, y1, x2, y2);

    x += dim.x;

    if (!(o & ROTATESPRITE_FULL16))
        x >>= 16;

    return x;
}

int32_t G_GameTextLen(int32_t x, const char *t)
{
    vec2_t dim;

    if (t == NULL)
        return -1;

    dim = G_ScreenTextSize(STARTALPHANUM, x, 0, textsc(65536L), 0, t, 2, 5, 8, 0, 0, TEXT_GAMETEXTNUMHACK, 0, 0, xdim-1, ydim-1);

    x += dim.x;

    return x;
}

// minitext_yofs: in hud_scale-independent, (<<16)-scaled, 0-200-normalized y coords,
// (sb&ROTATESPRITE_MAX) only.
int32_t minitext_yofs = 0;
int32_t minitext_lowercase = 0;
int32_t minitext_(int32_t x, int32_t y, const char *t, int32_t s, int32_t p, int32_t sb)
{
    vec2_t dim;
    int32_t z = 65536L;
    int32_t f = 0;

    if (t == NULL)
    {
        OSD_Printf("minitext: NULL text!\n");
        return 0;
    }

    if (!(sb & ROTATESPRITE_FULL16))
    {
        x<<=16;
        y<<=16;
    }

    if (!minitext_lowercase)
        f |= TEXT_UPPERCASE;

    if (sb & ROTATESPRITE_MAX)
    {
        x = sbarx16(x);
        y = minitext_yofs+sbary16(y);
        z = sbarsc(z);
    }

    sb &= (ROTATESPRITE_MAX-1)|RS_CENTERORIGIN;

    dim = G_ScreenText(MINIFONT, x, y, z, 0, 0, t, s, p, sb|ROTATESPRITE_FULL16, 0, (4<<16), (8<<16), (1<<16), 0, f, 0, 0, xdim-1, ydim-1);

    x += dim.x;

    if (!(sb & ROTATESPRITE_FULL16))
        x >>= 16;

    return x;
}

void G_AddUserQuote(const char *daquote)
{
    int32_t i;

    for (i=MAXUSERQUOTES-1; i>0; i--)
    {
        Bstrcpy(user_quote[i], user_quote[i-1]);
        user_quote_time[i] = user_quote_time[i-1];
    }
    Bstrcpy(user_quote[0], daquote);
    OSD_Printf("%s\n", daquote);

    user_quote_time[0] = ud.msgdisptime;
    pub = NUMPAGES;
}

// orientation flags depending on time that a quote has still to be displayed
static inline int32_t texto(int32_t t)
{
    if (t > 4) return 2+8+16;
    if (t > 2) return 2+8+16+1;
    return 2+8+16+1+32;
}

static inline int32_t texta(int32_t t)
{
    if (getrendermode() == REND_CLASSIC && numalphatabs < 15)
    {
        if (t > 4) return 0;
        if (t > 2) return 85;
        return 169;
    }

    t = clamp(t<<3, 0, 255);
    return 255 - t;
}

static int32_t calc_ybase(int32_t begy)
{
    int32_t k = begy;

    if (GTFLAGS(GAMETYPE_FRAGBAR) && ud.screen_size > 0
#ifdef SPLITSCREEN_MOD_HACKS
        && !g_fakeMultiMode
#endif
        && (g_netServer || ud.multimode > 1))
    {
        int32_t i, j = 0;

        k += 8;
        for (TRAVERSE_CONNECT(i))
            if (i > j)
                j = i;

        if (j >= 4 && j <= 8) k += 8;
        else if (j > 8 && j <= 12) k += 16;
        else if (j > 12) k += 24;
    }

    return k;
}

// this handles both multiplayer and item pickup message type text
// both are passed on to gametext
void G_PrintGameQuotes(int32_t snum)
{
    int32_t i, j, k;

    const DukePlayer_t *const ps = g_player[snum].ps;
    const int32_t reserved_quote = (ps->ftq >= QUOTE_RESERVED && ps->ftq <= QUOTE_RESERVED3);
    // NOTE: QUOTE_RESERVED4 is not included.

    k = calc_ybase(1);

    if (ps->fta > 1 && !reserved_quote)
    {
        k += min(7, ps->fta);
    }

    j = scale(k, ydim, 200);

    for (i=MAXUSERQUOTES-1; i>=0; i--)
    {
        int32_t sh, l;

        if (user_quote_time[i] <= 0)
            continue;

        k = user_quote_time[i];

        sh = hud_glowingquotes ? (sintable[((totalclock+(i<<2))<<5)&2047]>>11) : 0;

        mpgametext(j, user_quote[i], sh, texto(k));
        j += textsc(k > 4 ? 8 : (k<<1));

        l = G_GameTextLen(USERQUOTE_LEFTOFFSET, OSD_StripColors(tempbuf, user_quote[i]));
        while (l > (ud.config.ScreenWidth - USERQUOTE_RIGHTOFFSET))
        {
            l -= (ud.config.ScreenWidth-USERQUOTE_RIGHTOFFSET);
            j += textsc(k > 4 ? 8 : (k<<1));
        }
    }

    if (klabs(quotebotgoal-quotebot) <= 16 && ud.screen_size <= 8)
        quotebot += ksgn(quotebotgoal-quotebot);
    else
        quotebot = quotebotgoal;

    if (ps->fta <= 1)
        return;

    if (EDUKE32_PREDICT_FALSE(apStrings[ps->ftq] == NULL))
    {
        OSD_Printf(OSD_ERROR "%s %d null quote %d\n", __FILE__, __LINE__, ps->ftq);
        return;
    }

    k = calc_ybase(0);

    if (k == 0)
    {
        if (reserved_quote)
        {
#ifdef SPLITSCREEN_MOD_HACKS
            if (!g_fakeMultiMode)
                k = 140;  // quotebot-8-4;
            else
                k = 50;
#else
            k = 140;
#endif
        }
        else
        {
            if (ud.althud == 2)
                k = 32;
            else
#ifdef GEKKO
                k = 16;
#elif defined EDUKE32_TOUCH_DEVICES
                k = 24;
#else
                k = 1;
#endif
        }
    }

    int32_t pal = 0;

#ifdef SPLITSCREEN_MOD_HACKS
    if (g_fakeMultiMode)
    {
        pal = g_player[snum].pcolor;

        if (snum == 1)
        {
            const int32_t sidebyside = (ud.screen_size != 0);

            // NOTE: setting gametext's x -= 80 doesn't do the expected thing.
            // Needs looking into.
            if (sidebyside)
                k += 9;
            else
                k += 101;
        }
    }
#endif

    gametextpalbits(160, k, apStrings[ps->ftq], ftapulseshade, pal, 2 + 8 + 16, texta(ps->fta));
}

void P_DoQuote(int32_t q, DukePlayer_t *p)
{
    int32_t cq = 0;

    if (ud.fta_on == 0 || q < 0 || !(p->gm & MODE_GAME))
        return;

    if (q & MAXQUOTES)
    {
        cq = 1;
        q &= ~MAXQUOTES;
    }

    if (EDUKE32_PREDICT_FALSE(apStrings[q] == NULL))
    {
        OSD_Printf(OSD_ERROR "%s %d null quote %d\n", __FILE__, __LINE__, q);
        return;
    }

    if (p->fta > 0 && q != QUOTE_RESERVED && q != QUOTE_RESERVED2)
        if (p->ftq == QUOTE_RESERVED || p->ftq == QUOTE_RESERVED2) return;

    p->fta = 100;

    if (p->ftq != q)
    {
        if (p == g_player[screenpeek].ps
            && Bstrcmp(apStrings[q], "")) // avoid printing blank quotes
        {
            if (cq) OSD_Printf(OSDTEXT_BLUE "%s\n", apStrings[q]);
            else OSD_Printf("%s\n", apStrings[q]);
        }

        p->ftq = q;
    }
    pub = NUMPAGES;
    pus = NUMPAGES;
}
