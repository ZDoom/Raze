
#include "engine.h"

#include <io.h>
#include <fcntl.h>
#include <malloc.h>

// static int globhiz, globloz, globhihit, globlohit;


void overwritesprite(int thex, int they, short tilenum, signed char shade, char stat, char dapalnum)
{
#if 0
    rotatesprite(thex << 16, they << 16, 0x10000, (short)((flags & 8) << 7), tilenum, shade, dapalnum,
        (char)(((flags & 1 ^ 1) << 4) + (flags & 2) + ((flags & 4) >> 2) + ((flags & 16) >> 2) ^ ((flags & 8) >> 1)),
        windowx1, windowy1, windowx2, windowy2);
#endif

    rotatesprite(thex << 16, they << 16, 65536L, (stat & 8) << 7, tilenum, shade, dapalnum,
        (((stat & 1) ^ 1) << 4) + (stat & 2) + ((stat & 4) >> 2) + (((stat & 16) >> 2) ^ ((stat & 8) >> 1)),
        windowxy1.x, windowxy1.y, windowxy2.x, windowxy2.y);
}

void permanentwritesprite(int thex, int they, short tilenum, signed char shade, int cx1, int cy1, int cx2, int cy2, char dapalnum)
{
    rotatesprite(thex << 16, they << 16, 65536L, 0, tilenum, shade, dapalnum, 8 + 16, cx1, cy1, cx2, cy2);
}

void resettiming()
{
    numframes = 0L;
    totalclock = 0L;
// TODO	totalclocklock = 0L;
}

void kensetpalette(unsigned char *vgapal)
{
    //setbrightness(0, (char*)vgapal, 4 | 2);
    // TODO
    Bmemcpy(palette, vgapal, 768);
    videoSetPalette(0, 0, 4 | 2);
#if 0
    char vesapal[1024];

    for(int i = 0; i < 256; i++)
    {
        vesapal[i*4+0] = vgapal[i*3+2];
        vesapal[i*4+1] = vgapal[i*3+1];
        vesapal[i*4+2] = vgapal[i*3+0];
        vesapal[i*4+3] = 0;
    }
#ifndef __WATCOMC__
     (0L, 256L, vesapal);
#endif

#endif
}

void printext(int x, int y, const char *buffer, short tilenum, char invisiblecol)
{
    int i;
    unsigned char ch;

    for (i = 0; buffer[i] != 0; i++)
    {
        ch = (unsigned char)buffer[i];
        rotatesprite((x - ((ch & 15) << 3)) << 16, (y - ((ch >> 4) << 3)) << 16, 65536L, 0, tilenum, 0, 0, 8 + 16 + 128, x, y, x + 7, y + 7);
        x += 8;
    }

#if 0
    int i;
    char ch;

    for(i=0;buffer[i]!=0;i++)
    {
        ch = buffer[i];
        rotatesprite((x-((8&15)<<3))<<16,(y-((8>>4)<<3))<<16,65536L,0,tilenum,0,0,8+16+64+128,x,y,x+7,y+7);
        rotatesprite((x-((ch&15)<<3))<<16,(y-((ch>>4)<<3))<<16,65536L,0,tilenum,0,0,8+16+128,x,y,x+7,y+7);
        x += 8;
    }
#endif
}

void precache()
{
    int i;

    for (i = 0; i < numsectors; i++)
    {
        short j = sector[i].ceilingpicnum;
        if (waloff[j] == 0) tileLoad(j);
        j = sector[i].floorpicnum;
        if (waloff[j] == 0) tileLoad(j);
    }

    for (i = 0; i < numwalls; i++)
    {
        short j = wall[i].picnum;
        if (waloff[j] == 0) tileLoad(j);
    }

    for (i = 0; i < kMaxSprites; i++)
    {
        if (sprite[i].statnum < kMaxStatus)
        {
            short j = sprite[i].picnum;
            if (waloff[j] == 0) tileLoad(j);
        }
    }
}
