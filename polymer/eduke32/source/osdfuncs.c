//-------------------------------------------------------------------------
/*
Copyright (C) 2010 EDuke32 developers and contributors

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
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
*/
//-------------------------------------------------------------------------

#include "duke3d.h"
#include "build.h"
#include "namesdyn.h"
#include "osdfuncs.h"
#include "premap.h"

int32_t osdhightile = 0;

static int32_t GAME_isspace(int32_t ch)
{
    return (ch==32 || ch==9);
}

static int32_t GAME_getchartile(int32_t ch)
{
    int32_t ac = ch-'!'+STARTALPHANUM;
    if (ac < STARTALPHANUM || ac > ENDALPHANUM)
        ac = -1;
    return ac;
}

void GAME_drawosdchar(int32_t x, int32_t y, char ch, int32_t shade, int32_t pal)
{
    int16_t ac;
#ifndef USE_OPENGL
    int32_t usehightile = 0;
#endif
    int32_t ht = usehightile;

    if (GAME_isspace(ch)) return;
    if ((ac = GAME_getchartile(ch)) == -1)
        return;

    usehightile = (osdhightile && ht);
    rotatesprite_fs((9*x)<<16, (y<<3)<<16, 65536, 0, ac, shade, pal, 8|16);
    usehightile = ht;
}

void GAME_drawosdstr(int32_t x, int32_t y, const char *ch, int32_t len, int32_t shade, int32_t pal)
{
    int16_t ac;
    const char *const ptr = OSD_GetTextPtr();
    const char *const fmt = OSD_GetFmtPtr();
    const int32_t use_format = (ch > ptr && ch < (ptr + TEXTSIZE));
#ifdef USE_OPENGL
    const int32_t ht = usehightile;
    usehightile = (osdhightile && ht);
#endif

    x *= 9;

    do
    {
        if (!GAME_isspace(*ch))
            if ((ac = GAME_getchartile(*ch)) >= 0)
            {
                // use the format byte if the text falls within the bounds of the console buffer
                const int32_t tshade = use_format ? (fmt[ch-ptr]&~0x1F)>>4 : shade;
                const int32_t tpal = use_format ? fmt[ch-ptr]&~0xE0 : pal;

                rotatesprite_fs(x<<16, (y<<3)<<16, 65536, 0, ac, tshade, tpal, 8|16);
            }

        x += OSDCHAR_WIDTH+1;
        ch++;
    }
    while (--len);

#ifdef USE_OPENGL
    usehightile = ht;
#endif
}

void GAME_drawosdcursor(int32_t x, int32_t y, int32_t type, int32_t lastkeypress)
{
    int16_t ac;

    if (type) ac = SMALLFNTCURSOR;
    else ac = '_'-'!'+STARTALPHANUM;

    if (((GetTime()-lastkeypress) & 0x40)==0)
        rotatesprite_fs((9*x)<<16, ((y<<3)+(type?-1:2))<<16, 65536, 0, ac, 0, 8, 8|16);
}

int32_t GAME_getcolumnwidth(int32_t w)
{
    return w/9;
}

int32_t GAME_getrowheight(int32_t w)
{
    return w>>3;
}

void GAME_onshowosd(int32_t shown)
{
    G_UpdateScreenArea();

    UNREFERENCED_PARAMETER(shown);
    // XXX: it's weird to fake a keypress like this.
//    if (numplayers == 1 && ((shown && !ud.pause_on) || (!shown && ud.pause_on)))
//        KB_KeyDown[sc_Pause] = 1;
}
