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
#include "animlib.h"
#include "mouse.h"
#include "compat.h"

void endanimsounds(int32_t fr)
{
    switch (ud.volume_number)
    {
    case 0:
        break;
    case 1:
        switch (fr)
        {
        case 1:
            S_PlaySound(WIND_AMBIENCE);
            break;
        case 26:
            S_PlaySound(ENDSEQVOL2SND1);
            break;
        case 36:
            S_PlaySound(ENDSEQVOL2SND2);
            break;
        case 54:
            S_PlaySound(THUD);
            break;
        case 62:
            S_PlaySound(ENDSEQVOL2SND3);
            break;
        case 75:
            S_PlaySound(ENDSEQVOL2SND4);
            break;
        case 81:
            S_PlaySound(ENDSEQVOL2SND5);
            break;
        case 115:
            S_PlaySound(ENDSEQVOL2SND6);
            break;
        case 124:
            S_PlaySound(ENDSEQVOL2SND7);
            break;
        }
        break;
    case 2:
        switch (fr)
        {
        case 1:
            S_PlaySound(WIND_REPEAT);
            break;
        case 98:
            S_PlaySound(DUKE_GRUNT);
            break;
        case 82+20:
            S_PlaySound(THUD);
            S_PlaySound(SQUISHED);
            break;
        case 104+20:
            S_PlaySound(ENDSEQVOL3SND3);
            break;
        case 114+20:
            S_PlaySound(ENDSEQVOL3SND2);
            break;
        case 158:
            S_PlaySound(PIPEBOMB_EXPLODE);
            break;
        }
        break;
    }
}

void logoanimsounds(int32_t fr)
{
    switch (fr)
    {
    case 1:
        S_PlaySound(FLY_BY);
        break;
    case 19:
        S_PlaySound(PIPEBOMB_EXPLODE);
        break;
    }
}

void intro4animsounds(int32_t fr)
{
    switch (fr)
    {
    case 1:
        S_PlaySound(INTRO4_B);
        break;
    case 12:
    case 34:
        S_PlaySound(SHORT_CIRCUIT);
        break;
    case 18:
        S_PlaySound(INTRO4_5);
        break;
    }
}

void first4animsounds(int32_t fr)
{
    switch (fr)
    {
    case 1:
        S_PlaySound(INTRO4_1);
        break;
    case 12:
        S_PlaySound(INTRO4_2);
        break;
    case 7:
        S_PlaySound(INTRO4_3);
        break;
    case 26:
        S_PlaySound(INTRO4_4);
        break;
    }
}

void intro42animsounds(int32_t fr)
{
    switch (fr)
    {
    case 10:
        S_PlaySound(INTRO4_6);
        break;
    }
}

void endanimvol41(int32_t fr)
{
    switch (fr)
    {
    case 3:
        S_PlaySound(DUKE_UNDERWATER);
        break;
    case 35:
        S_PlaySound(VOL4ENDSND1);
        break;
    }
}

void endanimvol42(int32_t fr)
{
    switch (fr)
    {
    case 11:
        S_PlaySound(DUKE_UNDERWATER);
        break;
    case 20:
        S_PlaySound(VOL4ENDSND1);
        break;
    case 39:
        S_PlaySound(VOL4ENDSND2);
        break;
    case 50:
        FX_StopAllSounds();
        break;
    }
}

void endanimvol43(int32_t fr)
{
    switch (fr)
    {
    case 1:
        S_PlaySound(BOSS4_DEADSPEECH);
        break;
    case 40:
        S_PlaySound(VOL4ENDSND1);
        S_PlaySound(DUKE_UNDERWATER);
        break;
    case 50:
        S_PlaySound(BIGBANG);
        break;
    }
}

void G_PlayAnim(const char *fn,char t)
{
    char *animbuf;
    int32_t i, length=0, numframes=0;
#if defined(POLYMOST) && defined(USE_OPENGL)
    int32_t ogltexfiltermode=gltexfiltermode;
#endif
    int32_t handle=-1;
    int32_t frametime = 0;

    //    return;

    if (t != 7 && t != 9 && t != 10 && t != 11)
        KB_FlushKeyboardQueue();

    if (KB_KeyWaiting())
    {
        FX_StopAllSounds();
        goto ENDOFANIMLOOP;
    }

    handle = kopen4load((char *)fn,0);
    if (handle == -1) return;
    length = kfilelength(handle);

    walock[TILE_ANIM] = 219+t;

    allocache((intptr_t *)&animbuf,length+1,&walock[TILE_ANIM]);

    tilesizx[TILE_ANIM] = 200;
    tilesizy[TILE_ANIM] = 320;

    kread(handle,animbuf,length);
    kclose(handle);

    ANIM_LoadAnim(animbuf);
    numframes = ANIM_NumFrames();
    animpal = ANIM_GetPalette();

    //setpalette(0L,256L,tempbuf);
    //setbrightness(ud.brightness>>2,tempbuf,2);
    P_SetGamePalette(g_player[myconnectindex].ps,animpal,10);

#if defined(POLYMOST) && defined(USE_OPENGL)
    gltexfiltermode = 0;
    gltexapplyprops();
#endif

    ototalclock = totalclock + 10;

    for (i=1; i<numframes; i++)
    {
        if (i > 4 && totalclock > frametime + 60)
        {
            OSD_Printf("WARNING: slowdown in %s, skipping playback\n",fn);
            goto ENDOFANIMLOOP;
        }

        frametime = totalclock;

        waloff[TILE_ANIM] = (intptr_t)ANIM_DrawFrame(i);
        invalidatetile(TILE_ANIM, 0, 1<<4);  // JBF 20031228

        while (totalclock < ototalclock)
        {
            handleevents();
            Net_GetPackets();

            if (KB_KeyWaiting() || MOUSE_GetButtons()&LEFT_MOUSE)
                goto ENDOFANIMLOOP;

            if (g_restorePalette == 1)
            {
                P_SetGamePalette(g_player[myconnectindex].ps,animpal,0);
                g_restorePalette = 0;
            }

            rotatesprite(0<<16,0<<16,65536L,512,TILE_ANIM,0,0,2+4+8+16+64+(ud.bgstretch?1024:0), 0,0,xdim-1,ydim-1);
            nextpage();
        }

        if (t == 10) ototalclock += 14;
        else if (t == 9) ototalclock += 10;
        else if (t == 7) ototalclock += 18;
        else if (t == 6) ototalclock += 14;
        else if (t == 5) ototalclock += 9;
        else if (ud.volume_number == 3) ototalclock += 10;
        else if (ud.volume_number == 2) ototalclock += 10;
        else if (ud.volume_number == 1) ototalclock += 18;
        else                           ototalclock += 10;

        if (t == 8) endanimvol41(i);
        else if (t == 10) endanimvol42(i);
        else if (t == 11) endanimvol43(i);
        else if (t == 9) intro42animsounds(i);
        else if (t == 7) intro4animsounds(i);
        else if (t == 6) first4animsounds(i);
        else if (t == 5) logoanimsounds(i);
        else if (t < 4) endanimsounds(i);
    }

ENDOFANIMLOOP:
#if defined(POLYMOST) && defined(USE_OPENGL)
    gltexfiltermode = ogltexfiltermode;
    gltexapplyprops();
#endif
    MOUSE_ClearButton(LEFT_MOUSE);
    ANIM_FreeAnim();
    walock[TILE_ANIM] = 1;
}
