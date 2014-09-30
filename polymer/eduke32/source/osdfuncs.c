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
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/
//-------------------------------------------------------------------------

#include <math.h>
#include "compat.h"
#include "duke3d.h"
#include "build.h"
#include "namesdyn.h"
#include "osdfuncs.h"
#include "premap.h"

int32_t osdhightile = 1;
int32_t osdshown = 0;

#ifdef __ANDROID__
float osdscale = 2.f;
#else
float osdscale = 1.f;
#endif

#define OSD_SCALE(x) (int32_t)(osdscale != 1.f ? lround(osdscale*(x)) : (x))
#define OSD_SCALEDIV(x) (int32_t)lround((x)/osdscale)

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
    rotatesprite_fs(OSD_SCALE(9*x<<16),
        OSD_SCALE((y<<3)<<16),
        OSD_SCALE(65536.f), 0, ac, shade, pal, 8|16);
    usehightile = ht;
}

void GAME_drawosdstr(int32_t x, int32_t y, const char *ch, int32_t len, int32_t shade, int32_t pal)
{
    int16_t ac;
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
                OSD_GetShadePal(ch, &shade, &pal);
                rotatesprite_fs(OSD_SCALE(x<<16), OSD_SCALE((y<<3)<<16),
                    OSD_SCALE(65536.f), 0, ac, shade, pal, 8|16);
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

    if (((BGetTime()-lastkeypress) & 0x40)==0)
        rotatesprite_fs(OSD_SCALE(9*x<<16),
        OSD_SCALE(((y<<3)+(type?-1:2))<<16),
        OSD_SCALE(65536.f), 0, ac, 0, 8, 8|16);
}

int32_t GAME_getcolumnwidth(int32_t w)
{
     return OSD_SCALEDIV(w/9);
}

int32_t GAME_getrowheight(int32_t h)
{
    return OSD_SCALEDIV(h>>3);
}

void GAME_onshowosd(int32_t shown)
{
    G_UpdateScreenArea();

    osdshown = shown;

    // XXX: it's weird to fake a keypress like this.
//    if (numplayers == 1 && ((shown && !ud.pause_on) || (!shown && ud.pause_on)))
//        KB_KeyDown[sc_Pause] = 1;
}

void GAME_clearbackground(int32_t numcols, int32_t numrows)
{
    UNREFERENCED_PARAMETER(numcols);

# ifdef USE_OPENGL
    if (getrendermode() >= REND_POLYMOST && qsetmode==200)
    {
        bglPushAttrib(GL_FOG_BIT);
        bglDisable(GL_FOG);

        setpolymost2dview();
        bglColor4f(0, 0, 0, 0.67f);
        bglEnable(GL_BLEND);
        bglRectd(0, 0, xdim, OSD_SCALE(8*numrows+8));
        bglColor4f(0, 0, 0, 1);
        bglRectd(0, OSD_SCALE(8*numrows+4), xdim, OSD_SCALE(8*numrows+8));

        bglPopAttrib();

        return;
    }
# endif

    CLEARLINES2D(0, min(ydim, OSD_SCALE(numrows*8+8)), editorcolors[16]);
}

#undef OSD_SCALE
