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

#include "vfs.h"

// animsound_t.sound
EDUKE32_STATIC_ASSERT(INT16_MAX >= MAXSOUNDS);

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

        Xfree(str);
    }

    return (dukeanim_t *)(ptr == -1 ? NULL : (dukeanim_t *)ptr);
}

dukeanim_t * Anim_Create(char const * fn)
{
    dukeanim_t * anim = (dukeanim_t *)Xcalloc(1, sizeof(dukeanim_t));

    hash_add(&h_dukeanim, fn, (intptr_t)anim, 0);

    return anim;
}

#ifndef EDUKE32_STANDALONE
#ifdef DYNSOUNDREMAP_ENABLE
static int32_t const StopAllSounds = -1;
#else
# define StopAllSounds -1
#endif
#endif

void Anim_Init(void)
{
    hash_init(&h_dukeanim);


    struct defaultanmsound {
#ifdef DYNSOUNDREMAP_ENABLE
        int32_t const & sound;
#else
        int16_t sound;
#endif
        uint8_t frame;
    };

    static defaultanmsound const logo[] =
    {
        { FLY_BY, 1 },
        { PIPEBOMB_EXPLODE, 19 },
    };

#ifndef EDUKE32_STANDALONE
    static defaultanmsound const cineov2[] =
    {
        { WIND_AMBIENCE, 1 },
        { ENDSEQVOL2SND1, 26 },
        { ENDSEQVOL2SND2, 36 },
        { THUD, 54 },
        { ENDSEQVOL2SND3, 62 },
        { ENDSEQVOL2SND4, 75 },
        { ENDSEQVOL2SND5, 81 },
        { ENDSEQVOL2SND6, 115 },
        { ENDSEQVOL2SND7, 124 },
    };

    static defaultanmsound const cineov3[] =
    {
        { WIND_REPEAT, 1 },
        { DUKE_GRUNT, 98 },
        { THUD, 82+20 },
        { SQUISHED, 82+20 },
        { ENDSEQVOL3SND3, 104+20 },
        { ENDSEQVOL3SND2, 114+20 },
        { PIPEBOMB_EXPLODE, 158 },
    };

    static defaultanmsound const vol42a[] =
    {
        { INTRO4_B, 1 },
        { SHORT_CIRCUIT, 12 },
        { INTRO4_5, 18 },
        { SHORT_CIRCUIT, 34 },
    };

    static defaultanmsound const vol41a[] =
    {
        { INTRO4_1, 1 },
        { INTRO4_3, 7 },
        { INTRO4_2, 12 },
        { INTRO4_4, 26 },
    };

    static defaultanmsound const vol43a[] =
    {
        { INTRO4_6, 10 },
    };

    static defaultanmsound const vol4e1[] =
    {
        { DUKE_UNDERWATER, 3 },
        { VOL4ENDSND1, 35 },
    };

    static defaultanmsound const vol4e2[] =
    {
        { DUKE_UNDERWATER, 11 },
        { VOL4ENDSND1, 20 },
        { VOL4ENDSND2, 39 },
        { StopAllSounds, 50 },
    };

    static defaultanmsound const vol4e3[] =
    {
        { BOSS4_DEADSPEECH, 1 },
        { VOL4ENDSND1, 40 },
        { DUKE_UNDERWATER, 40 },
        { BIGBANG, 50 },
    };
#endif

    struct defaultanm {
        char const *fn;
        defaultanmsound const *sounds;
        uint8_t numsounds;
        uint8_t fdelay;
    };

#define anmsnd(x) (x), ARRAY_SIZE(x)
    static defaultanm const anms[] =
    {
        { "logo.anm", anmsnd(logo), 9 },
        { "3dr.anm", NULL, 0, 10 },
#ifndef EDUKE32_STANDALONE
        { "vol4e1.anm", anmsnd(vol4e1), 10 },
        { "vol4e2.anm", anmsnd(vol4e2), 14 },
        { "vol4e3.anm", anmsnd(vol4e3), 10 },
        { "vol41a.anm", anmsnd(vol41a), 14 },
        { "vol42a.anm", anmsnd(vol42a), 18 },
        { "vol43a.anm", anmsnd(vol43a), 10 },
        { "duketeam.anm", NULL, 0, 10 },
        { "radlogo.anm", NULL, 0, 10 },
        { "cineov2.anm", anmsnd(cineov2), 18 },
        { "cineov3.anm", anmsnd(cineov3), 10 },
#endif
    };
#undef anmsnd

    for (defaultanm const & anm : anms)
    {
        dukeanim_t * anim = Anim_Create(anm.fn);
        anim->framedelay = anm.fdelay;

        if (anm.numsounds)
        {
            anim->sounds = (animsound_t *)Xmalloc(anm.numsounds * sizeof(animsound_t));
            int const numsounds = anm.numsounds;
            for (int i = 0; i < numsounds; ++i)
            {
                defaultanmsound const & src = anm.sounds[i];
                animsound_t & dst = anim->sounds[i];

                dst.sound = src.sound;
                dst.frame = src.frame;
            }
            anim->numsounds = numsounds;
        }

        anim->frameflags = 0;
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
    while (videoGetRenderMode() >= REND_POLYMOST)  // if, really
    {
        char const * dot = Bstrrchr(fn, '.');
        if (!dot)
            break;

        dukeanim_t const * origanim = anim;
        buildvfs_kfd handle = buildvfs_kfd_invalid;
        if (!Bstrcmp(dot, ".ivf"))
        {
            handle = kopen4loadfrommod(fn, 0);
            if (handle == buildvfs_kfd_invalid)
                break;
        }
        else
        {
            char vpxfn[BMAX_PATH];
            Bstrncpyz(vpxfn, fn, BMAX_PATH);

            ptrdiff_t dotpos = dot - fn;
            if (dotpos + 4 >= BMAX_PATH)
                break;

            char *vpxfndot = vpxfn + dotpos;
            vpxfndot[1] = 'i';
            vpxfndot[2] = 'v';
            vpxfndot[3] = 'f';
            vpxfndot[4] = '\0';

            handle = kopen4loadfrommod(vpxfn, 0);
            if (handle == buildvfs_kfd_invalid)
                break;

            anim = Anim_Find(vpxfn);
        }

        animvpx_ivf_header_t info;
        i = animvpx_read_ivf_header(handle, &info);

        if (i)
        {
            OSD_Printf("Failed reading IVF file: %s\n", animvpx_read_ivf_header_errmsg[i]);
            kclose(handle);
            return 0;
        }

        if (anim)
            animvpx_setup_glstate(anim->frameflags);
        else
            animvpx_setup_glstate(origanim->frameflags);

        animvpx_codec_ctx codec;

        if (animvpx_init_codec(&info, handle, &codec))
        {
            OSD_Printf("Error initializing VPX codec.\n");
            animvpx_restore_glstate();
            kclose(handle);
            return 0;
        }


        uint32_t const convnumer = 120 * info.fpsdenom;
        uint32_t const convdenom = info.fpsnumer * origanim->framedelay;

        uint32_t const msecsperframe = scale(info.fpsdenom, 1000, info.fpsnumer);
        uint32_t nextframetime = timerGetTicks();
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

            VM_OnEventWithReturn(EVENT_PRECUTSCENE, g_player[screenpeek].ps->i, screenpeek, framenum);

            videoClearScreen(0);

            ototalclock = totalclock + 1; // pause game like ANMs

            if (anim)
            {
                if (anim->frameaspect1 == 0 || anim->frameaspect2 == 0)
                    animvpx_render_frame(&codec, 0);
                else
                    animvpx_render_frame(&codec, anim->frameaspect1 / anim->frameaspect2);
            }
            else
            {
                if (origanim->frameaspect1 == 0 || origanim->frameaspect2 == 0)
                    animvpx_render_frame(&codec, 0);
                else
                    animvpx_render_frame(&codec, origanim->frameaspect1 / origanim->frameaspect2);
            }

            VM_OnEventWithReturn(EVENT_CUTSCENE, g_player[screenpeek].ps->i, screenpeek, framenum);

            // after rendering the frame but before displaying: maybe play sound...
            framenum++;
            if (anim)
            {
                while (soundidx < anim->numsounds && anim->sounds[soundidx].frame <= framenum)
                {
                    int16_t sound = anim->sounds[soundidx].sound;
                    if (sound == -1)
                        FX_StopAllSounds();
                    else
                        S_PlaySound(sound);

                    soundidx++;
                }
            }
            else
            {
                uint16_t convframenum = scale(framenum, convnumer, convdenom);
                while (soundidx < origanim->numsounds && origanim->sounds[soundidx].frame <= convframenum)
                {
                    int16_t sound = origanim->sounds[soundidx].sound;
                    if (sound == -1)
                        FX_StopAllSounds();
                    else
                        S_PlaySound(sound);

                    soundidx++;
                }
            }

            // this and showframe() instead of nextpage() are so that
            // nobody tramples on our carefully set up GL state!
            palfadedelta = 0;
            videoShowFrame(0);

            //            I_ClearAllInput();

            do
            {
                G_HandleAsync();

                if (VM_OnEventWithReturn(EVENT_SKIPCUTSCENE, g_player[screenpeek].ps->i, screenpeek, I_GeneralTrigger()))
                {
                    running = 0;
                    break;
                }
            } while (timerGetTicks() < nextframetime);
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
    buildvfs_kfd handle = kopen4load(fn, 0);

    if (handle == buildvfs_kfd_invalid)
        return 0;

    int32_t length = kfilelength(handle);

    if (length <= 4)
    {
        OSD_Printf("Warning: skipping playback of empty ANM file \"%s\".\n", fn);
        goto end_anim;
    }

    anim->animlock = 255;

    if (!anim->animbuf)
        cacheAllocateBlock((intptr_t *)&anim->animbuf, length + 1, &anim->animlock);

    kread(handle, anim->animbuf, length);
    kclose(handle);

    uint32_t firstfour;
    Bmemcpy(&firstfour, anim->animbuf, 4);

    // "DKIF" (.ivf)
    if (firstfour == B_LITTLE32(0x46494B44))
        goto end_anim;

    int32_t numframes;

    // "LPF " (.anm)
    if (firstfour != B_LITTLE32(0x2046504C) ||
        ANIM_LoadAnim(anim->animbuf, length) < 0 ||
        (numframes = ANIM_NumFrames()) <= 0)
    {
        // XXX: ANM_LoadAnim() still checks less than the bare minimum,
        // e.g. ANM file could still be too small and not contain any frames.
        OSD_Printf("Error: malformed ANM file \"%s\".\n", fn);
        goto end_anim;
    }

    paletteSetColorTable(ANIMPAL, ANIM_GetPalette());

    // setpalette(0L,256L,tempbuf);
    // setbrightness(ud.brightness>>2,tempbuf,2);
    P_SetGamePalette(g_player[myconnectindex].ps, ANIMPAL, 8 + 2);

#ifdef USE_OPENGL
    if ((anim->frameflags & CUTSCENE_TEXTUREFILTER && gltexfiltermode == TEXFILTER_ON) || anim->frameflags & CUTSCENE_FORCEFILTER)
        gltexfiltermode = TEXFILTER_ON;
    else
        gltexfiltermode = TEXFILTER_OFF;
    gltexapplyprops();
#endif

    timerUpdate();
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

        i = VM_OnEventWithReturn(EVENT_PRECUTSCENE, g_player[screenpeek].ps->i, screenpeek, i);

        walock[TILE_ANIM] = 255;
        waloff[TILE_ANIM] = (intptr_t)ANIM_DrawFrame(i);
        tileSetSize(TILE_ANIM, 200, 320);
        tileInvalidate(TILE_ANIM, 0, 1 << 4);  // JBF 20031228

        if (VM_OnEventWithReturn(EVENT_SKIPCUTSCENE, g_player[screenpeek].ps->i, screenpeek, I_GeneralTrigger()))
        {
            running = 0;
            goto end_anim_restore_gl;
        }

        if (g_restorePalette == 1)
        {
            P_SetGamePalette(g_player[myconnectindex].ps, ANIMPAL, 0);
            g_restorePalette = 0;
        }

        frametime = (int32_t) totalclock;

        videoClearScreen(0);

        int32_t z;
        if (anim->frameaspect1 > 0 && anim->frameaspect2 > 0 && ((anim->frameaspect1 / anim->frameaspect2) != (tilesiz[TILE_ANIM].y / (tilesiz[TILE_ANIM].x * 1.2))))
        {
            int32_t const oyxaspect = yxaspect;
            if ((anim->frameaspect1 / anim->frameaspect2) >= ((decltype(anim->frameaspect1))xdim / ydim))
                z = divscale16(320, tilesiz[TILE_ANIM].y);
            else
                z = divscale16(lrint(320 * ydim * anim->frameaspect1), lrint(tilesiz[TILE_ANIM].y * xdim * anim->frameaspect2));
            int32_t aspect = divscale16(lrint(tilesiz[TILE_ANIM].y * anim->frameaspect2), lrint(tilesiz[TILE_ANIM].x * anim->frameaspect1));
            renderSetAspect(viewingrange, aspect);
            rotatesprite_fs(160<<16, 100<<16, z, 512, TILE_ANIM, 0, 0, 2|4|8|64|1024);
            renderSetAspect(viewingrange, oyxaspect);
        }
        else
        {
            if ((tilesiz[TILE_ANIM].y / (tilesiz[TILE_ANIM].x * 1.2f)) > (1.f * xdim / ydim))
                z = divscale16(320 * xdim * 3, tilesiz[TILE_ANIM].y * ydim * 4);
            else
                z = divscale16(200, tilesiz[TILE_ANIM].x);
            rotatesprite_fs(160<<16, 100<<16, z, 512, TILE_ANIM, 0, 0, 2|4|8|64);
        }

        g_animPtr = anim;
        i = VM_OnEventWithReturn(EVENT_CUTSCENE, g_player[screenpeek].ps->i, screenpeek, i);
        g_animPtr = NULL;

        videoNextPage();

        I_ClearAllInput();

        ototalclock += anim->framedelay;

        while (soundidx < anim->numsounds && anim->sounds[soundidx].frame <= (uint16_t)i)
        {
            int16_t sound = anim->sounds[soundidx].sound;
            if (sound == -1)
                FX_StopAllSounds();
            else
                S_PlaySound(sound);

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

    tileSetSize(TILE_ANIM, 0, 0);
    walock[TILE_ANIM] = 0;
    waloff[TILE_ANIM] = 0;

    // this is the lock for anim->animbuf
    anim->animlock = 1;

    return !running;
}
