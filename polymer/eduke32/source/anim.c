//-------------------------------------------------------------------------
/*
Copyright (C) 2005 - EDuke32 team

This file is part of EDuke32

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
#include "animlib.h"

void endanimsounds(long fr)
{
    switch(ud.volume_number)
    {
    case 0:break;
    case 1:
        switch(fr)
        {
        case 1:
            sound(WIND_AMBIENCE);
            break;
        case 26:
            sound(ENDSEQVOL2SND1);
            break;
        case 36:
            sound(ENDSEQVOL2SND2);
            break;
        case 54:
            sound(THUD);
            break;
        case 62:
            sound(ENDSEQVOL2SND3);
            break;
        case 75:
            sound(ENDSEQVOL2SND4);
            break;
        case 81:
            sound(ENDSEQVOL2SND5);
            break;
        case 115:
            sound(ENDSEQVOL2SND6);
            break;
        case 124:
            sound(ENDSEQVOL2SND7);
            break;
        }
        break;
    case 2:
        switch(fr)
        {
        case 1:
            sound(WIND_REPEAT);
            break;
        case 98:
            sound(DUKE_GRUNT);
            break;
        case 82+20:
            sound(THUD);
            sound(SQUISHED);
            break;
        case 104+20:
            sound(ENDSEQVOL3SND3);
            break;
        case 114+20:
            sound(ENDSEQVOL3SND2);
            break;
        case 158:
            sound(PIPEBOMB_EXPLODE);
            break;
        }
        break;
    }
}

void logoanimsounds(long fr)
{
    switch(fr)
    {
    case 1:
        sound(FLY_BY);
        break;
    case 19:
        sound(PIPEBOMB_EXPLODE);
        break;
    }
}

void intro4animsounds(long fr)
{
    switch(fr)
    {
    case 1:
        sound(INTRO4_B);
        break;
    case 12:
    case 34:
        sound(SHORT_CIRCUIT);
        break;
    case 18:
        sound(INTRO4_5);
        break;
    }
}

void first4animsounds(long fr)
{
    switch(fr)
    {
    case 1:
        sound(INTRO4_1);
        break;
    case 12:
        sound(INTRO4_2);
        break;
    case 7:
        sound(INTRO4_3);
        break;
    case 26:
        sound(INTRO4_4);
        break;
    }
}

void intro42animsounds(long fr)
{
    switch(fr)
    {
    case 10:
        sound(INTRO4_6);
        break;
    }
}

void endanimvol41(long fr)
{
    switch(fr)
    {
    case 3:
        sound(DUKE_UNDERWATER);
        break;
    case 35:
        sound(VOL4ENDSND1);
        break;
    }
}

void endanimvol42(long fr)
{
    switch(fr)
    {
    case 11:
        sound(DUKE_UNDERWATER);
        break;
    case 20:
        sound(VOL4ENDSND1);
        break;
    case 39:
        sound(VOL4ENDSND2);
        break;
    case 50:
        FX_StopAllSounds();
        break;
    }
}

void endanimvol43(long fr)
{
    switch(fr)
    {
    case 1:
        sound(BOSS4_DEADSPEECH);
        break;
    case 40:
        sound(VOL4ENDSND1);
        sound(DUKE_UNDERWATER);
        break;
    case 50:
        sound(BIGBANG);
        break;
    }
}

void playanm(char *fn,char t)
{
    char *animbuf, *palptr;
    long i, j, k, length=0, numframes=0;
    int32 handle=-1;

    //    return;

    if(t != 7 && t != 9 && t != 10 && t != 11)
        KB_FlushKeyboardQueue();

    if( KB_KeyWaiting() )
    {
        FX_StopAllSounds();
        goto ENDOFANIMLOOP;
    }

    handle = kopen4load(fn,0);
    if(handle == -1) return;
    length = kfilelength(handle);

    walock[TILE_ANIM] = 219+t;

    allocache((long *)&animbuf,length,&walock[TILE_ANIM]);

    tilesizx[TILE_ANIM] = 200;
    tilesizy[TILE_ANIM] = 320;

    kread(handle,animbuf,length);
    kclose(handle);

    ANIM_LoadAnim (animbuf);
    numframes = ANIM_NumFrames();

    palptr = ANIM_GetPalette();
    for(i=0;i<256;i++)
    {
        /*
        j = (i<<2); k = j-i;
        tempbuf[j+0] = (palptr[k+2]>>2);
        tempbuf[j+1] = (palptr[k+1]>>2);
        tempbuf[j+2] = (palptr[k+0]>>2);
        tempbuf[j+3] = 0;
        */
        j = i*3;
        tempbuf[j+0] = (palptr[j+0]>>2);
        tempbuf[j+1] = (palptr[j+1]>>2);
        tempbuf[j+2] = (palptr[j+2]>>2);
    }

    //setpalette(0L,256L,tempbuf);
    //setbrightness(ud.brightness>>2,tempbuf,2);
    setgamepalette(&ps[myconnectindex],tempbuf,2);

    ototalclock = totalclock + 10;

    for(i=1;i<numframes;i++)
    {
        while(totalclock < ototalclock)
        {
            if( KB_KeyWaiting() )
                goto ENDOFANIMLOOP;
            handleevents(); getpackets();
        }

        if(t == 10) ototalclock += 14;
        else if(t == 9) ototalclock += 10;
        else if(t == 7) ototalclock += 18;
        else if(t == 6) ototalclock += 14;
        else if(t == 5) ototalclock += 9;
        else if(ud.volume_number == 3) ototalclock += 10;
        else if(ud.volume_number == 2) ototalclock += 10;
        else if(ud.volume_number == 1) ototalclock += 18;
        else                           ototalclock += 10;

        waloff[TILE_ANIM] = FP_OFF(ANIM_DrawFrame(i));
        invalidatetile(TILE_ANIM, 0, 1<<4);  // JBF 20031228
        rotatesprite(0<<16,0<<16,65536L,512,TILE_ANIM,0,0,2+4+8+16+64, 0,0,xdim-1,ydim-1);
        nextpage();

        if(t == 8) endanimvol41(i);
        else if(t == 10) endanimvol42(i);
        else if(t == 11) endanimvol43(i);
        else if(t == 9) intro42animsounds(i);
        else if(t == 7) intro4animsounds(i);
        else if(t == 6) first4animsounds(i);
        else if(t == 5) logoanimsounds(i);
        else if(t < 4) endanimsounds(i);
    }

ENDOFANIMLOOP:

    ANIM_FreeAnim ();
    walock[TILE_ANIM] = 1;
}
