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

#include "baselayer.h"
#include "renderlayer.h"
#include "duke3d.h"
#include "animlib.h"
#include "mouse.h"
#include "compat.h"
#include "input.h"

#include "anim.h"

#ifdef USE_LIBVPX
# include "animvpx.h"
#endif

#include "animsounds.h"

hashtable_t h_dukeanim = { 8, NULL };
dukeanim_t * g_animPtr;

dukeanim_t *Anim_Find(const char *s)
{
    intptr_t ptr = hash_findcase(&h_dukeanim, s);

    if (ptr == -1)
    {
        int const siz = Bstrlen(s) + 5;
        char * const str = (char *)Xcalloc(1, siz);

        maybe_append_ext(str, siz, s, ".anm");
        ptr = hash_findcase(&h_dukeanim, str);

        if (ptr == -1)
        {
            maybe_append_ext(str, siz, s, ".ivf");
            ptr = hash_findcase(&h_dukeanim, str);
        }

        Bfree(str);
    }

    return (dukeanim_t *)(ptr == -1 ? NULL : (dukeanim_t *)ptr);
}

dukeanim_t * Anim_Create(char const * fn)
{
    dukeanim_t * anim = (dukeanim_t *)Xcalloc(1, sizeof(dukeanim_t));

    hash_add(&h_dukeanim, fn, (intptr_t)anim, 0);

    return anim;
}

void Anim_Init(void)
{
    hash_init(&h_dukeanim);

    struct defaultanm {
        char const *fn;
        void (*sound_func)(int32_t);
        uint8_t fdelay;
    };

    static defaultanm const anms[] =
    {
        { "logo.anm", logoanimsounds, 9 },
        { "3dr.anm", NULL, 10 },
#ifndef EDUKE32_STANDALONE
        { "vol4e1.anm", endanimvol41, 10 },
        { "vol4e2.anm", endanimvol42, 14 },
        { "vol4e3.anm", endanimvol43, 10 },
        { "vol41a.anm", first4animsounds, 14 },
        { "vol42a.anm", intro4animsounds, 18 },
        { "vol43a.anm", intro42animsounds, 10 },
        { "duketeam.anm", NULL, 10 },
        { "radlogo.anm", NULL, 10 },
        { "cineov2.anm", endanimsounds, 18 },
        { "cineov3.anm", endanimsounds, 10 },
#endif
    };

    for (defaultanm const & anm : anms)
    {
        dukeanim_t * anim = Anim_Create(anm.fn);
        anim->framedelay = anm.fdelay;
        anim->sound_func = anm.sound_func;
    }
}

int32_t Anim_Play(const char *fn)
{
    dukeanim_t *anim = Anim_Find(fn);

    if (!anim)
    {
        OSD_Printf("Animation %s is undefined!\n", fn);
        return 0;
    }

    uint16_t soundidx = 0;  // custom anim sounds
    int32_t running = 1, i;

    I_ClearAllInput();

#ifdef USE_LIBVPX
    uint16_t framenum = 0;
    while (getrendermode() >= REND_POLYMOST)  // if, really
    {
        char vpxfn[BMAX_PATH];
        Bstrncpyz(vpxfn, fn, BMAX_PATH);

        char *dot = Bstrrchr(vpxfn, '.');
        if (!dot || (dot - vpxfn) + 4 >= BMAX_PATH)
            break;

        dot[1] = 'i';
        dot[2] = 'v';
        dot[3] = 'f';
        dot[4] = 0;

        int32_t handle = kopen4loadfrommod(vpxfn, 0);
        if (handle == -1)
            break;

        animvpx_ivf_header_t info;
        i = animvpx_read_ivf_header(handle, &info);

        if (i)
        {
            OSD_Printf("Failed reading IVF file: %s\n", animvpx_read_ivf_header_errmsg[i]);
            kclose(handle);
            return 0;
        }

        animvpx_setup_glstate();

        animvpx_codec_ctx codec;

        if (animvpx_init_codec(&info, handle, &codec))
        {
            OSD_Printf("Error initializing VPX codec.\n");
            animvpx_restore_glstate();
            return 0;
        }

        uint32_t msecsperframe = ((uint64_t)info.fpsdenom * 1000) / info.fpsnumer;
        uint32_t nextframetime = getticks();
        uint8_t *pic;

        //        OSD_Printf("msecs per frame: %d\n", msecsperframe);

        do
        {
            nextframetime += msecsperframe;

            i = animvpx_nextpic(&codec, &pic);
            if (i)
            {
                OSD_Printf("Failed getting next pic: %s\n", animvpx_nextpic_errmsg[i]);
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
            while (soundidx < anim->numsounds && anim->sounds[soundidx].frame == framenum)
            {
                S_PlaySound(anim->sounds[soundidx].sound);
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
            } while (getticks() < nextframetime);
        } while (running);

        animvpx_print_stats(&codec);

        //
        kclose(handle);
        animvpx_restore_glstate();
        animvpx_uninit_codec(&codec);

        I_ClearAllInput();
        return !running;  // done with playing VP8!
    }
#endif
// ANM playback --- v v v ---

#ifdef USE_OPENGL
    int32_t ogltexfiltermode = gltexfiltermode;
#endif
    int32_t handle = kopen4load(fn, 0);

    if (handle == -1)
        return 0;

    int32_t length = kfilelength(handle);

    if (length == 0)
    {
        OSD_Printf("Warning: skipping playback of empty ANM file \"%s\".\n", fn);
        goto end_anim;
    }

    walock[TILE_ANIM] = 219;
    anim->animlock = 1;

    if (!anim->animbuf)
        allocache((intptr_t *)&anim->animbuf, length + 1, &anim->animlock);

    tilesiz[TILE_ANIM].x = 200;
    tilesiz[TILE_ANIM].y = 320;

    kread(handle, anim->animbuf, length);
    kclose(handle);

    int32_t numframes;

    if (ANIM_LoadAnim(anim->animbuf, length) < 0 || (numframes = ANIM_NumFrames()) <= 0)
    {
        // XXX: ANM_LoadAnim() still checks less than the bare minimum,
        // e.g. ANM file could still be too small and not contain any frames.
        OSD_Printf("Error: malformed ANM file \"%s\".\n", fn);
        goto end_anim;
    }

    setbasepal(ANIMPAL, ANIM_GetPalette());

    // setpalette(0L,256L,tempbuf);
    // setbrightness(ud.brightness>>2,tempbuf,2);
    P_SetGamePalette(g_player[myconnectindex].ps, ANIMPAL, 8 + 2);

#ifdef USE_OPENGL
    gltexfiltermode = 0;
    gltexapplyprops();
#endif

    ototalclock = totalclock;

    i = 1;
    int32_t frametime; frametime = 0;

    do
    {
        if (i > 4 && totalclock > frametime + 60)
        {
            OSD_Printf("WARNING: slowdown in %s, skipping playback\n", fn);
            goto end_anim_restore_gl;
        }

        G_HandleAsync();

        if (totalclock < ototalclock - 1)
            continue;

        waloff[TILE_ANIM] = (intptr_t)ANIM_DrawFrame(i);
        invalidatetile(TILE_ANIM, 0, 1 << 4);  // JBF 20031228

        if (I_CheckAllInput())
        {
            running = 0;
            goto end_anim_restore_gl;
        }

        if (g_restorePalette == 1)
        {
            P_SetGamePalette(g_player[myconnectindex].ps, ANIMPAL, 0);
            g_restorePalette = 0;
        }

        frametime = totalclock;

        clearallviews(0);

        rotatesprite_fs(0 << 16, 0 << 16, 65536L, 512, TILE_ANIM, 0, 0, 2 + 4 + 8 + 16 + 64 + BGSTRETCH);

        g_animPtr = anim;
        i = VM_OnEventWithReturn(EVENT_CUTSCENE, -1, myconnectindex, i);
        g_animPtr = NULL;

        nextpage();

        I_ClearAllInput();

        ototalclock += anim->framedelay;

        if (!anim->numsounds && anim->sound_func)
            anim->sound_func(i);

        while (soundidx < anim->numsounds && anim->sounds[soundidx].frame == (uint16_t)i)
        {
            S_PlaySound(anim->sounds[soundidx].sound);
            soundidx++;
        }

        ++i;
    } while (i < numframes);

end_anim_restore_gl:
#ifdef USE_OPENGL
    gltexfiltermode = ogltexfiltermode;
    gltexapplyprops();
#endif
end_anim:
    I_ClearAllInput();
    ANIM_FreeAnim();
    walock[TILE_ANIM] = 1;
    anim->animlock = 0;

    return !running;
}
