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
#include "compat.h"
#include "duke3d.h"
#include "build.h"
#include "namesdyn.h"
#include "osdfuncs.h"
#include "premap.h"

int32_t osdhightile = 1;
int32_t osdshown = 0;

#ifdef EDUKE32_TOUCH_DEVICES
float osdscale = 2.f, osdrscale = 0.5f;
#else
float osdscale = 1.f, osdrscale = 1.f;
#endif

#define OSD_SCALE(x) (int32_t)(osdscale != 1.f ? Blrintf(osdscale*(float)(x)) : (x))
#define OSD_SCALEDIV(x) (int32_t)Blrintf((float)(x) * osdrscale)

#define OSDCHAR_WIDTH (tilesiz[STARTALPHANUM + 'A' - '!'].x)
#define OSDCHAR_HEIGHT (tilesiz[STARTALPHANUM + 'A' - '!'].y + 1)

static inline int32_t GAME_isspace(int32_t ch)
{
    return (ch==32 || ch==9);
}

static inline int32_t GAME_getchartile(int32_t ch)
{
    const int32_t ac = ch-'!'+STARTALPHANUM;
    return (ac < STARTALPHANUM || ac > ENDALPHANUM) ? -1 : ac;
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
    rotatesprite_fs(OSD_SCALE((OSDCHAR_WIDTH*x)<<16),
        OSD_SCALE((y*OSDCHAR_HEIGHT)<<16),
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

    x *= OSDCHAR_WIDTH;

    do
    {
        if (!GAME_isspace(*ch))
            if ((ac = GAME_getchartile(*ch)) >= 0)
            {
                OSD_GetShadePal(ch, &shade, &pal);
                rotatesprite_fs(OSD_SCALE(x<<16), OSD_SCALE((y*OSDCHAR_HEIGHT)<<16),
                    OSD_SCALE(65536.f), 0, ac, shade, pal, 8|16);
            }

        x += OSDCHAR_WIDTH;
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
        rotatesprite_fs(OSD_SCALE((OSDCHAR_WIDTH*x)<<16),
        OSD_SCALE(((y*OSDCHAR_HEIGHT)+(type?-1:2))<<16),
        OSD_SCALE(65536.f), 0, ac, 0, 8, 8|16);
}

int32_t GAME_getcolumnwidth(int32_t w)
{
     return OSD_SCALEDIV(w/OSDCHAR_WIDTH);
}

int32_t GAME_getrowheight(int32_t h)
{
    return OSD_SCALEDIV(h/OSDCHAR_HEIGHT);
}

void GAME_onshowosd(int32_t shown)
{
    G_UpdateScreenArea();

    AppGrabMouse((!shown) + 2);

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
        const int32_t i8n8 = OSD_SCALE(OSDCHAR_HEIGHT*numrows);
//        bglPushAttrib(GL_FOG_BIT);
        bglDisable(GL_FOG);

        setpolymost2dview();
        bglColor4f(0.f, 0.f, 0.f, 0.67f);
        bglEnable(GL_BLEND);
        bglRecti(0, 0, xdim, i8n8+OSDCHAR_HEIGHT);
        bglColor4f(0.f, 0.f, 0.f, 1.f);
        bglRecti(0, i8n8+4, xdim, i8n8+OSDCHAR_HEIGHT);
        if (!nofog)
            bglEnable(GL_FOG);
//        bglPopAttrib();

        return;
    }
# endif

    CLEARLINES2D(0, min(ydim, OSD_SCALE(numrows*OSDCHAR_HEIGHT+OSDCHAR_HEIGHT)), editorcolors[16]);
}

#undef OSD_SCALE
