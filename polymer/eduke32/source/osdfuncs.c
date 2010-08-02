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

void GAME_drawosdchar(int32_t x, int32_t y, char ch, int32_t shade, int32_t pal)
{
    int16_t ac;
#if !defined(POLYMOST) || !defined(USE_OPENGL)
    int32_t usehightile = 0;
#endif
    int32_t ht = usehightile;

    if (ch == 32) return;
    ac = ch-'!'+STARTALPHANUM;
    if (ac < STARTALPHANUM || ac > ENDALPHANUM) return;
    usehightile = (osdhightile && ht);
    rotatesprite(((x<<3)+x)<<16, (y<<3)<<16, 65536l, 0, ac, shade, pal, 8|16, 0, 0, xdim-1, ydim-1);
    usehightile = ht;
}

void GAME_drawosdstr(int32_t x, int32_t y, char *ch, int32_t len, int32_t shade, int32_t pal)
{
    int16_t ac;
    char *ptr = OSD_GetTextPtr();
    char *fmt = OSD_GetFmtPtr();
#if !defined(POLYMOST) || !defined(USE_OPENGL)
    int32_t usehightile = 0;
#endif
    int32_t ht = usehightile;

    usehightile = (osdhightile && ht);
    x = (x<<3)+x;

    if (ch > ptr && ch < (ptr + TEXTSIZE))
    {
        do
        {
            if (*ch == 32)
            {
                x += OSDCHAR_WIDTH+1;
                ch++;
                continue;
            }
            ac = *ch-'!'+STARTALPHANUM;
            if (ac < STARTALPHANUM || ac > ENDALPHANUM) { usehightile = ht; return; }

            // use the format byte if the text falls within the bounds of the console buffer
            rotatesprite(x<<16, (y<<3)<<16, 65536, 0, ac, (*(ch-ptr+fmt)&~0x1F)>>4,
                         *(ch-ptr+fmt)&~0xE0, 8|16, 0, 0, xdim-1, ydim-1);
            x += OSDCHAR_WIDTH+1;
            ch++;
        }
        while (--len);

        usehightile = ht;
        return;
    }

    do
    {
        if (*ch == 32)
        {
            x += OSDCHAR_WIDTH+1;
            ch++;
            continue;
        }
        ac = *ch-'!'+STARTALPHANUM;
        if (ac < STARTALPHANUM || ac > ENDALPHANUM) { usehightile = ht; return; }

        rotatesprite(x<<16, (y<<3)<<16, 65536, 0, ac, shade, pal, 8|16, 0, 0, xdim-1, ydim-1);
        x += OSDCHAR_WIDTH+1;
        ch++;
    }
    while (--len);

    usehightile = ht;
}

void GAME_drawosdcursor(int32_t x, int32_t y, int32_t type, int32_t lastkeypress)
{
    int16_t ac;

    if (type) ac = SMALLFNTCURSOR;
    else ac = '_'-'!'+STARTALPHANUM;

    if (!((GetTime()-lastkeypress) & 0x40l))
        rotatesprite(((x<<3)+x)<<16, ((y<<3)+(type?-1:2))<<16, 65536l, 0, ac, 0, 8, 8|16, 0, 0, xdim-1, ydim-1);
}

int32_t GAME_getcolumnwidth(int32_t w)
{
    return w/9;
}

int32_t GAME_getrowheight(int32_t w)
{
    return w>>3;
}

//#define BGTILE 311
//#define BGTILE 1156
#define BGTILE 1141	// BIGHOLE
#define BGTILE_SIZEX 128
#define BGTILE_SIZEY 128
#define BORDTILE 3250	// VIEWBORDER
#define BITSTH 1+32+8+16	// high translucency
#define BITSTL 1+8+16	// low translucency
#define BITS 8+16+64		// solid
#define SHADE 0
#define PALETTE 4

void GAME_onshowosd(int32_t shown)
{
    // fix for TCs like Layre which don't have the BGTILE for some reason
    // most of this is copied from my dummytile stuff in defs.c
    if (!tilesizx[BGTILE] || !tilesizy[BGTILE])
    {
        int32_t j;

        tilesizx[BGTILE] = BGTILE_SIZEX;
        tilesizy[BGTILE] = BGTILE_SIZEY;
        faketilesiz[BGTILE] = -1;
        picanm[BGTILE] = 0;

        j = 15; while ((j > 1) && (pow2long[j] > BGTILE_SIZEX)) j--;
        picsiz[BGTILE] = ((uint8_t)j);
        j = 15; while ((j > 1) && (pow2long[j] > BGTILE_SIZEY)) j--;
        picsiz[BGTILE] += ((uint8_t)(j<<4));
    }

    G_UpdateScreenArea();

    if (numplayers == 1 && ((shown && !ud.pause_on) || (!shown && ud.pause_on)))
        KB_KeyDown[sc_Pause] = 1;
}

void GAME_clearbackground(int32_t c, int32_t r)
{
    int32_t x, y, xsiz, ysiz, tx2, ty2;
    int32_t daydim, bits;

    UNREFERENCED_PARAMETER(c);

    if (getrendermode() < 3) bits = BITS;
    else bits = BITSTL;

    daydim = r<<3;

    xsiz = tilesizx[BGTILE];
    tx2 = xdim/xsiz;
    ysiz = tilesizy[BGTILE];
//    ty2 = ydim/ysiz;
    ty2 = daydim/ysiz;

    for (x=tx2; x>=0; x--)
        for (y=ty2; y>=0; y--)
//        for (y=ty2+1;y>=1;y--)
//            rotatesprite(x*xsiz<<16,((daydim-ydim)+(y*ysiz))<<16,65536L,0,BGTILE,SHADE,PALETTE,bits,0,0,xdim,daydim);
            rotatesprite(x*xsiz<<16,y*ysiz<<16,65536L,0,BGTILE,SHADE,PALETTE,bits,0,0,xdim,daydim);

    xsiz = tilesizy[BORDTILE];
    tx2 = xdim/xsiz;
    ysiz = tilesizx[BORDTILE];

    for (x=tx2; x>=0; x--)
        rotatesprite(x*xsiz<<16,(daydim+ysiz+1)<<16,65536L,1536,BORDTILE,SHADE-12,PALETTE,BITS,0,0,xdim,daydim+ysiz+1);
}

