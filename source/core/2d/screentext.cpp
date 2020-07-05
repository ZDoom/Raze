//-------------------------------------------------------------------------
/*
Copyright (C) 2016 EDuke32 developers and contributors
Copyright (C) 2019 Christoph Oelckers

This file is part of Raze.

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

#include "screentext.h"
#include "build.h"

inline void SetIfGreater(int32_t *variable, int32_t potentialValue)
{
    if (potentialValue > *variable)
        *variable = potentialValue;
}

// get the string length until the next '\n'
int32_t G_GetStringLineLength(const char *text, const char *end, const int32_t iter)
{
    int32_t length = 0;

    while (text != end && *text != '\n')
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

// This function requires you to Xfree() the returned char*.
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

#define NUMHACKACTIVE ((f & TEXT_GAMETEXTNUMHACK) && t >= '0' && t <= '9')

#define USERQUOTE_RIGHTOFFSET 14

static int GetStringTile(int font, const char* t, int f)
{
    int ret = gi->GetStringTile(font, t, f);
    auto tex = tileGetTexture(ret);
    if (!tex || !tex->isValid())
    {
        if (*t >= 'a' && *t <= 'z')
        {
            char tt = *t - 32;
            ret = gi->GetStringTile(font, &tt, f);
        }
    }
    return ret;
}


// qstrdim
vec2_t G_ScreenTextSize(const int32_t font,
    int32_t x, int32_t y, const int32_t z, const int32_t blockangle,
    const char *str, const int32_t o,
    int32_t xspace, int32_t yline, int32_t xbetween, int32_t ybetween,
    const int32_t f,
    int32_t x1, int32_t y1, int32_t x2, int32_t y2)
{
    vec2_t size = { 0, 0, }; // eventually the return value
    vec2_t pos = { 0, 0, }; // holds the coordinate position as we draw each character tile of the string
    vec2_t extent = { 0, 0, }; // holds the x-width of each character and the greatest y-height of each line
    vec2_t offset = { 0, 0, }; // temporary; holds the last movement made in both directions

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
    xspace = mulscale16(xspace, z);
    yline = mulscale16(yline, z);
    xbetween = mulscale16(xbetween, z);
    ybetween = mulscale16(ybetween, z);
    // size/width/height/spacing/offset values should be multiplied or scaled by $z, zoom (since 100% is 65536, the same as 1<<16)

    // loop through the string
    while (text != end && (t = *text))
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
        tile = GetStringTile(font, &t, f);

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
                tile = GetStringTile(font, &space, f);

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
                    tile = GetStringTile(font, &line, f);

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
                    tile = GetStringTile(font, &line, f);

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

            if (NUMHACKACTIVE)
            {
                char numeral = '0'; // this is subject to change as an implementation detail
                extent.x = (tilesiz[GetStringTile(font, &numeral, f)].x-1) * z;
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
        if (!NUMHACKACTIVE
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
                        tile = GetStringTile(font, &line, f);

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
    coords->x += mulscale14(magnitude, unitDirection->x);
    coords->y += mulscale14(magnitude, unitDirection->y);
}

// screentext
vec2_t G_ScreenText(const int32_t font,
    int32_t x, int32_t y, const int32_t z, const int32_t blockangle, const int32_t charangle,
    const char *str, const int32_t shade, int32_t pal, int32_t o, int32_t alpha,
    int32_t xspace, int32_t yline, int32_t xbetween, int32_t ybetween, const int32_t f,
    const int32_t x1, const int32_t y1, const int32_t x2, const int32_t y2)
{
    vec2_t size = { 0, 0, }; // eventually the return value
    vec2_t origin = { 0, 0, }; // where to start, depending on the alignment
    vec2_t pos = { 0, 0, }; // holds the coordinate position as we draw each character tile of the string
    vec2_t extent = { 0, 0, }; // holds the x-width of each character and the greatest y-height of each line
    const vec2_t Xdirection = { sintable[(blockangle+512)&2047], sintable[blockangle&2047], };
    const vec2_t Ydirection = { sintable[(blockangle+1024)&2047], sintable[(blockangle+512)&2047], };
    const int32_t z2 = (f & TEXT_RRMENUTEXTHACK) ? 26214 : z;

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

    int32_t const xspace_orig = xspace;
    int32_t const yline_orig = yline;
    int32_t const xbetween_orig = xbetween;
    int32_t const ybetween_orig = ybetween;
    // handle zooming where applicable
    xspace = mulscale16(xspace, z);
    yline = mulscale16(yline, z);
    xbetween = mulscale16(xbetween, z);
    ybetween = mulscale16(ybetween, z);
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

                linewidth = G_ScreenTextSize(font, x, y, z, blockangle, line, o | ROTATESPRITE_FULL16, xspace_orig, yline_orig, (f & TEXT_XJUSTIFY) ? 0 : xbetween_orig, (f & TEXT_YJUSTIFY) ? 0 : ybetween_orig, f & ~(TEXT_XJUSTIFY|TEXT_YJUSTIFY|TEXT_BACKWARDS), x1, y1, x2, y2).x;

                Xfree(line);
            }

            if (f & TEXT_XJUSTIFY)
            {
                size.x = xbetween;

                xbetween = (length == 1) ? 0 : tabledivide32_noinline((xbetween - linewidth), (length - 1));

                linewidth = size.x;
            }

            if (f & TEXT_XRIGHT)
                origin.x = -(linewidth/z*z);
            else if (f & TEXT_XCENTER)
                origin.x = -(linewidth/2/z*z);
        }

        if (f & TEXT_YJUSTIFY)
        {
            const int32_t tempswap = ybetween;
            ybetween = (lines == 1) ? 0 : tabledivide32_noinline(ybetween - size.y, lines - 1);
            size.y = tempswap;
        }

        if (f & TEXT_YBOTTOM)
            origin.y = -(size.y/z*z);
        else if (f & TEXT_YCENTER)
            origin.y = -(size.y/2/z*z);
    }

    // loop through the string
    while (text != end && (t = *text))
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
        tile = GetStringTile(font, &t, f);

        switch (t)
        {
        case '\t':
        case ' ':
        case '\n':
        case '\x7F':
            break;

        default:
        {
            vec2_t location = { x, y, };

            G_AddCoordsFromRotation(&location, &Xdirection, origin.x);
            G_AddCoordsFromRotation(&location, &Ydirection, origin.y);

            G_AddCoordsFromRotation(&location, &Xdirection, pos.x);
            G_AddCoordsFromRotation(&location, &Ydirection, pos.y);

            rotatesprite_(location.x, location.y, z2, angle, tile, shade, pal, orientation, alpha, blendidx, x1, y1, x2, y2);

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
                tile = GetStringTile(font, &space, f);

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
                    tile = GetStringTile(font, &line, f);

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
                    tile = GetStringTile(font, &line, f);

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

                int32_t linewidth = G_ScreenTextSize(font, x, y, z, blockangle, line, o | ROTATESPRITE_FULL16, xspace_orig, yline_orig, (f & TEXT_XJUSTIFY) ? 0 : xbetween_orig, (f & TEXT_YJUSTIFY) ? 0 : ybetween_orig, f & ~(TEXT_XJUSTIFY|TEXT_YJUSTIFY|TEXT_BACKWARDS), x1, y1, x2, y2).x;

                Xfree(line);

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

            if (NUMHACKACTIVE)
            {
                char numeral = '0'; // this is subject to change as an implementation detail
                extent.x = (tilesiz[GetStringTile(font, &numeral, f)].x-1) * z;
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
            if (!NUMHACKACTIVE
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
                            tile = GetStringTile(font, &line, f);

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
    vec2_t size = { 0, 0, }; // eventually the return value

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

    G_ScreenText(font, x + mulscale16(sx, z), y + mulscale16(sy, z), z, blockangle, charangle, str, 127, 4, o|ROTATESPRITE_FULL16, alpha, xspace, yline, xbetween, ybetween, f, x1, y1, x2, y2);

    size = G_ScreenText(font, x, y, z, blockangle, charangle, str, shade, pal, o|ROTATESPRITE_FULL16, alpha, xspace, yline, xbetween, ybetween, f, x1, y1, x2, y2);

    // return values in the same manner we receive them
    if (!(o & ROTATESPRITE_FULL16))
    {
        size.x >>= 16;
        size.y >>= 16;
    }

    return size;
}
