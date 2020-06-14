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

#include "ns.h"	// Must come before everything else!

#include "baselayer.h"
#include "baselayer.h"
#include "duke3d.h"
#include "animlib.h"
#include "compat.h"
#include "cmdlib.h"
#include "v_2ddrawer.h"
#include "animtexture.h"
#include "v_video.h"


#include "anim.h"

#ifdef USE_LIBVPX
# include "animvpx.h"
#endif

BEGIN_RR_NS

// animsound_t.sound

TArray<dukeanim_t> g_Animations;
dukeanim_t * g_animPtr;

dukeanim_t* Anim_Find(const char* s)
{
    auto index = g_Animations.FindEx([=](dukeanim_t& anm) { return !anm.name.CompareNoCase(s);  });
    if (index == g_Animations.Size())
    {
        FString fname = s;
        DefaultExtension(fname, ".anm");
        index = g_Animations.FindEx([=](dukeanim_t& anm) { return !anm.name.CompareNoCase(fname);  });
        if (index == g_Animations.Size())
        {
            fname = s;
            DefaultExtension(fname, ".ivf");
            index = g_Animations.FindEx([=](dukeanim_t& anm) { return !anm.name.CompareNoCase(fname);  });
            if (index == g_Animations.Size()) return nullptr;
        }
    }
    return &g_Animations[index];
}

dukeanim_t * Anim_Create(char const * fn)
{
    g_Animations.Reserve(1);
    auto p = &g_Animations.Last();
    p->name = fn;
    return p;
}

#ifdef DYNSOUNDREMAP_ENABLE
static int32_t const StopAllSounds = -1;
#else
# define StopAllSounds -1
#endif

void Anim_Init(void)
{
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

    static defaultanmsound const rr_intro[] =
    {
        { 29, 1 },
    };

    static defaultanmsound const redneck[] =
    {
        { 478, 1 },
    };

    static defaultanmsound const xatlogo[] =
    {
        { 479, 1 },
    };

    static defaultanmsound const turdmov[] =
    {
        { 82, 1 },
    };

    static defaultanmsound const rr_outro[] =
    {
        { 35, 1 },
    };

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
        { "rr_intro.anm", anmsnd(rr_intro), 9 },
        { "redneck.anm", anmsnd(redneck), 9 },
        { "xatlogo.anm", anmsnd(xatlogo), 9 },
        { "turdmov.anm", anmsnd(turdmov), 9 },
        { "rr_outro.anm", anmsnd(rr_outro), 9 },
        { "lvl1.anm", NULL, 0, 20 },
        { "lvl2.anm", NULL, 0, 20 },
        { "lvl3.anm", NULL, 0, 20 },
        { "lvl4.anm", NULL, 0, 20 },
        { "lvl5.anm", NULL, 0, 20 },
        { "lvl6.anm", NULL, 0, 20 },
        { "lvl7.anm", NULL, 0, 20 },
        { "lvl8.anm", NULL, 0, 20 },
        { "lvl9.anm", NULL, 0, 20 },
        { "lvl10.anm", NULL, 0, 20 },
        { "lvl11.anm", NULL, 0, 20 },
        { "lvl12.anm", NULL, 0, 20 },
        { "lvl13.anm", NULL, 0, 20 },
    };
#undef anmsnd

    for (defaultanm const& anm : anms)
    {
        dukeanim_t* anim = Anim_Create(anm.fn);
        anim->framedelay = anm.fdelay;

        if (anm.numsounds)
        {
            anim->Sounds.Resize(anm.numsounds);
            int const numsounds = anm.numsounds;
            for (int i = 0; i < numsounds; ++i)
            {
                defaultanmsound const& src = anm.sounds[i];
                animsound_t& dst = anim->Sounds[i];

                dst.sound = src.sound;
                dst.frame = src.frame;
            }
        }

        anim->frameflags = 0;
    }
}

int32_t Anim_Play(const char *fn)
{
    dukeanim_t *anim = Anim_Find(fn);

    if (!anim)
    {
        Printf("Animation %s is undefined!\n", fn);
        return 0;
    }

    uint16_t soundidx = 0;  // custom anim sounds
    int32_t running = 1, i;

    inputState.ClearAllInput();

#ifdef USE_LIBVPX
    uint16_t framenum = 0;
    while (videoGetRenderMode() >= REND_POLYMOST)  // if, really
    {
        char const * dot = Bstrrchr(fn, '.');
        if (!dot)
            break;

        dukeanim_t const * origanim = anim;
		FileReader handle;
        if (!Bstrcmp(dot, ".ivf"))
        {
            handle = fileSystem.OpenFileReader(fn);
            if (!handle.isOpen())
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

            handle = fileSystem.OpenFileReader(vpxfn);
			if (!handle.isOpen())
				break;

            anim = Anim_Find(vpxfn);
        }

        animvpx_ivf_header_t info;
        i = animvpx_read_ivf_header(handle, &info);

        if (i)
        {
            Printf("Failed reading IVF file: %s\n", animvpx_read_ivf_header_errmsg[i]);
            return 0;
        }

        if (anim)
            animvpx_setup_glstate(anim->frameflags);
        else
            animvpx_setup_glstate(origanim->frameflags);

        animvpx_codec_ctx codec;

        if (animvpx_init_codec(&info, handle, &codec))
        {
            Printf("Error initializing VPX codec.\n");
            animvpx_restore_glstate();
            return 0;
        }


        uint32_t const convnumer = 120 * info.fpsdenom;
        uint32_t const convdenom = info.fpsnumer * origanim->framedelay;

        uint32_t const msecsperframe = scale(info.fpsdenom, 1000, info.fpsnumer);
        uint32_t nextframetime = timerGetTicks();
        uint8_t *pic;

        //        Printf("msecs per frame: %d\n", msecsperframe);

        do
        {
            nextframetime += msecsperframe;

            i = animvpx_nextpic(&codec, &pic);
            if (i)
            {
                Printf("Failed getting next pic: %s\n", animvpx_nextpic_errmsg[i]);
                if (codec.errmsg)
                {
                    Printf("  %s\n", codec.errmsg);
                    if (codec.errmsg_detail)
                        Printf("  detail: %s\n", codec.errmsg_detail);
                }
                break;
            }

            if (!pic)
                break;  // no more pics!

            twod->ClearScreen();

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

            // after rendering the frame but before displaying: maybe play sound...
            framenum++;
            if (anim)
            {
                while (soundidx < anim->Sounds.Size() && anim->Sounds[soundidx].frame <= framenum)
                {
                    int16_t sound = anim->Sounds[soundidx].sound;
                    if (sound == -1)
                        FX_StopAllSounds();
                    else
                        S_PlaySound(sound, CHAN_AUTO, CHANF_UI);

                    soundidx++;
                }
            }
            else
            {
                uint16_t convframenum = scale(framenum, convnumer, convdenom);
                while (soundidx < origanim->Sounds.Size() && origanim->Sounds[soundidx].frame <= convframenum)
                {
                    int16_t sound = origanim->Sounds[soundidx].sound;
                    if (sound == -1)
                        FX_StopAllSounds();
                    else
                        S_PlaySound(sound, CHAN_AUTO, CHANF_UI);

                    soundidx++;
                }
            }

            videoNextPage();
            do
            {
                G_HandleAsync();

                if (inputState.CheckAllInput())
                {
                    running = 0;
                    break;
                }
            } while (timerGetTicks() < nextframetime);
        } while (running);

		#ifdef DEBUGGINGAIDS
				animvpx_print_stats(&codec);
		#endif

        //
        animvpx_restore_glstate();
        animvpx_uninit_codec(&codec);

        inputState.ClearAllInput();
        return !running;  // done with playing VP8!
    }
#endif
// ANM playback --- v v v ---

    int32_t ogltexfiltermode = gl_texture_filter;
	auto fr = fileSystem.OpenFileReader(fn);

    if (!fr.isOpen())
        return 0;

	auto buffer = fr.ReadPadded(1);
	fr.Close();

    if (buffer.Size() <= 5)
    {
        Printf("Warning: skipping playback of empty ANM file \"%s\".\n", fn);
        goto end_anim;
    }

	anim->animbuf = buffer.Data();

    uint32_t firstfour;
    Bmemcpy(&firstfour, anim->animbuf, 4);

    // "DKIF" (.ivf)
    if (firstfour == B_LITTLE32(0x46494B44))
        goto end_anim;

    int32_t numframes;

    // "LPF " (.anm)
    if (firstfour != B_LITTLE32(0x2046504C) ||
        ANIM_LoadAnim(anim->animbuf, buffer.Size()-1) < 0 ||
        (numframes = ANIM_NumFrames()) <= 0)
    {
        // XXX: ANM_LoadAnim() still checks less than the bare minimum,
        // e.g. ANM file could still be too small and not contain any frames.
        Printf("Error: malformed ANM file \"%s\".\n", fn);
        goto end_anim;
    }

    ototalclock = totalclock;

    i = 1;
    int32_t frametime; frametime = 0;

    {
        AnimTextures animtex;
        animtex.SetSize(AnimTexture::Paletted, 320, 200);

        do
        {
            if (i > 4 && totalclock > frametime + 60)
            {
                Printf("WARNING: slowdown in %s, skipping playback\n", fn);
                goto end_anim_restore_gl;
            }

            G_HandleAsync();

            if (totalclock < ototalclock - 1)
                continue;

            animtex.SetFrame(ANIM_GetPalette(), ANIM_DrawFrame(i));

            if (inputState.CheckAllInput())
            {
                running = 0;
                goto end_anim_restore_gl;
            }

            if (g_restorePalette == 1)
            {
                P_SetGamePalette(g_player[myconnectindex].ps, ANIMPAL, 0);
                g_restorePalette = 0;
            }

            frametime = (int32_t)totalclock;

            twod->ClearScreen();

            int32_t z;
#if 0
            if (anim->frameaspect1 > 0 && anim->frameaspect2 > 0 && ((anim->frameaspect1 / anim->frameaspect2) != (tilesiz[TILE_ANIM].y / (tilesiz[TILE_ANIM].x * 1.2))))
            {
                int32_t const oyxaspect = yxaspect;
                if ((anim->frameaspect1 / anim->frameaspect2) >= ((decltype(anim->frameaspect1))xdim / ydim))
                    z = divscale16(320, tilesiz[TILE_ANIM].y);
                else
                    z = divscale16(lrint(320 * ydim * anim->frameaspect1), lrint(tilesiz[TILE_ANIM].y * xdim * anim->frameaspect2));
                int32_t aspect = divscale16(lrint(tilesiz[TILE_ANIM].y * anim->frameaspect2), lrint(tilesiz[TILE_ANIM].x * anim->frameaspect1));
                renderSetAspect(viewingrange, aspect);
                rotatesprite_fs(160 << 16, 100 << 16, z, 512, TILE_ANIM, 0, 0, 2 | 4 | 8 | 64 | 1024);
                renderSetAspect(viewingrange, oyxaspect);
            }
            else
#endif
            {
                if ((320 / (200 * 1.2f)) > (1.f * xdim / ydim))
                    z = divscale16(320 * xdim * 3, 320 * ydim * 4);
                else
                    z = divscale16(200, 200);
                rotatesprite_fs(160 << 16, 100 << 16, z, 0, -1, 0, 0, 2 | 8 | 64, animtex.GetFrame());
            }

            g_animPtr = NULL;

            videoNextPage();

            inputState.ClearAllInput();

            ototalclock += anim->framedelay;

            while (soundidx < anim->Sounds.Size() && anim->Sounds[soundidx].frame <= (uint16_t)i)
            {
                int16_t sound = anim->Sounds[soundidx].sound;
                if (sound == -1)
                    FX_StopAllSounds();
                else
                    S_PlaySound(sound, CHAN_AUTO, CHANF_UI);

                soundidx++;
            }

            ++i;
        } while (i < numframes);
    }

end_anim_restore_gl:
    gl_texture_filter = ogltexfiltermode;
    screen->SetTextureFilterMode();
end_anim:
    inputState.ClearAllInput();
	anim->animbuf = nullptr;
    ANIM_FreeAnim();
	tileDelete(TILE_ANIM);

    return !running;
}
END_RR_NS
