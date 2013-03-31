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
#include "input.h"

#include "anim.h"

#ifdef USE_LIBVPX
# include "animvpx.h"

uint16_t anim_hi_numsounds[NUM_HARDCODED_ANIMS], *anim_hi_sounds[NUM_HARDCODED_ANIMS];
#endif

static void endanimsounds(int32_t fr)
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

static void logoanimsounds(int32_t fr)
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

static void intro4animsounds(int32_t fr)
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

static void first4animsounds(int32_t fr)
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

static void intro42animsounds(int32_t fr)
{
    switch (fr)
    {
    case 10:
        S_PlaySound(INTRO4_6);
        break;
    }
}

static void endanimvol41(int32_t fr)
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

static void endanimvol42(int32_t fr)
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

static void endanimvol43(int32_t fr)
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

void G_PlayAnim(const char *fn, char t)
{
    uint8_t *animbuf;
    int32_t i, length=0, numframes=0;
#ifdef USE_OPENGL
    int32_t ogltexfiltermode=gltexfiltermode;
#endif
    int32_t handle=-1;
    int32_t frametime = 0;

    // t parameter:
    //
    // 1: cineov2
    // 2: cineov3
    // 3: RADLOGO
    // 4: DUKETEAM
    // 5: logo
    // 6: vol41a
    // 7: vol42a
    // 8: vol4e1
    // 9: vol43a
    // 10: vol4e2
    // 11: vol4e3

    if (t != 7 && t != 9 && t != 10 && t != 11)
        I_ClearAllInput();

    if (I_CheckAllInput())
    {
        FX_StopAllSounds();
        goto end_anim;
    }

#ifdef USE_LIBVPX
    while (getrendermode() >= REND_POLYMOST && glinfo.glsl)  // if, really
    {
        char vpxfn[BMAX_PATH], *dot;
        animvpx_ivf_header_t info;

        animvpx_codec_ctx codec;
        uint8_t *pic;
        uint32_t msecsperframe, nextframetime;
        int32_t running = 1;
        int32_t animidx, framenum=0, soundidx=0, numtotalsounds=0;  // custom anim sounds

        Bstrncpyz(vpxfn, fn, BMAX_PATH);

        dot = Bstrrchr(vpxfn, '.');
        if (!dot || (dot-vpxfn)+4 >= BMAX_PATH)
            break;

        dot[1] = 'i';
        dot[2] = 'v';
        dot[3] = 'f';
        dot[4] = 0;

        handle = kopen4loadfrommod(vpxfn, 0);
        if (handle == -1)
            break;

        i = animvpx_read_ivf_header(handle, &info);
        if (i)
        {
            OSD_Printf("Failed reading IVF file: %s\n",
                       animvpx_read_ivf_header_errmsg[i]);
            kclose(handle);
            return;
        }

        animvpx_setup_glstate();
        if (animvpx_init_codec(&info, handle, &codec))
        {
            OSD_Printf("Error initializing VPX codec.\n");
            animvpx_restore_glstate();
            return;
        }

        animidx = t-1;
        if ((unsigned)animidx < NUM_HARDCODED_ANIMS && anim_hi_sounds[animidx])
            numtotalsounds = anim_hi_numsounds[animidx];

        msecsperframe = ((uint64_t)info.fpsdenom*1000)/info.fpsnumer;
//        OSD_Printf("msecs per frame: %d\n", msecsperframe);

        nextframetime = getticks();

        while (running)
        {
            nextframetime += msecsperframe;

            i = animvpx_nextpic(&codec, &pic);
            if (i)
            {
                OSD_Printf("Failed getting next pic: %s\n",
                           animvpx_nextpic_errmsg[i]);
                if (codec.errmsg)
                {
                    OSD_Printf("  %s\n", codec.errmsg);
                    if (codec.errmsg_detail)
                        OSD_Printf("  detail: %s\n", codec.errmsg_detail);
                }
                break;
            }

            if (!pic)
                break;  // no more pics!

            animvpx_render_frame(&codec);

            // after rendering the frame but before displaying: maybe play sound...
            framenum++;
            while (soundidx < numtotalsounds && anim_hi_sounds[animidx][2*soundidx] == framenum)
            {
                S_PlaySound(anim_hi_sounds[animidx][2*soundidx+1]);
                soundidx++;
            }

            // this and showframe() instead of nextpage() are so that
            // nobody tramples on our carefully set up GL state!
            palfadedelta = 0;
            showframe(0);

//            I_ClearAllInput();

            do
            {
                G_HandleAsync();

                if (I_CheckAllInput())
                {
                    running = 0;
                    break;
                }
            }
            while (getticks() < nextframetime);
        }

        animvpx_print_stats(&codec);

        //
        kclose(handle);
        animvpx_restore_glstate();
        animvpx_uninit_codec(&codec);

        I_ClearAllInput();
        return;  // done with playing VP8!
    }
#endif
    // ANM playback --- v v v ---

    handle = kopen4load(fn, 0);
    if (handle == -1)
        return;

    length = kfilelength(handle);
    if (length == 0)
    {
        OSD_Printf("Warning: skipping playback of empty ANM file \"%s\".\n", fn);
        goto end_anim;
    }

    walock[TILE_ANIM] = 219+t;

    allocache((intptr_t *)&animbuf, length+1, &walock[TILE_ANIM]);

    tilesizx[TILE_ANIM] = 200;
    tilesizy[TILE_ANIM] = 320;

    kread(handle, animbuf, length);
    kclose(handle);

    if (ANIM_LoadAnim(animbuf, length) < 0)
    {
        // XXX: ANM_LoadAnim() still checks less than the bare minimum,
        // e.g. ANM file could still be too small and not contain any frames.
        OSD_Printf("Error: malformed ANM file \"%s\".\n", fn);
        goto end_anim;
    }

    numframes = ANIM_NumFrames();

    basepaltable[ANIMPAL] = ANIM_GetPalette();

    //setpalette(0L,256L,tempbuf);
    //setbrightness(ud.brightness>>2,tempbuf,2);
    P_SetGamePalette(g_player[myconnectindex].ps, ANIMPAL, 8+2);

#ifdef USE_OPENGL
    gltexfiltermode = 0;
    gltexapplyprops();
#endif

    ototalclock = totalclock + 10;

    for (i=1; i<numframes; i++)
    {
        if (i > 4 && totalclock > frametime + 60)
        {
            OSD_Printf("WARNING: slowdown in %s, skipping playback\n", fn);
            goto end_anim_restore_gl;
        }

        frametime = totalclock;

        waloff[TILE_ANIM] = (intptr_t)ANIM_DrawFrame(i);
        invalidatetile(TILE_ANIM, 0, 1<<4);  // JBF 20031228

        I_ClearAllInput();

        while (totalclock < ototalclock)
        {
            G_HandleAsync();

            if (I_CheckAllInput())
                goto end_anim_restore_gl;

            if (g_restorePalette == 1)
            {
                P_SetGamePalette(g_player[myconnectindex].ps, ANIMPAL, 0);
                g_restorePalette = 0;
            }

            clearallviews(0);

            rotatesprite_fs(0<<16,0<<16,65536L,512,TILE_ANIM,0,0,2+4+8+16+64+(ud.bgstretch?1024:0));
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

end_anim_restore_gl:
#ifdef USE_OPENGL
    gltexfiltermode = ogltexfiltermode;
    gltexapplyprops();
#endif
end_anim:
    I_ClearAllInput();
    ANIM_FreeAnim();
    walock[TILE_ANIM] = 1;
}
