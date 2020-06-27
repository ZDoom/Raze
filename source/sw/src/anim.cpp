//-------------------------------------------------------------------------
/*
Copyright (C) 1997, 2005 - 3D Realms Entertainment

This file is part of Shadow Warrior version 1.2

Shadow Warrior is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.

Original Source: 1997 - Frank Maddin and Jim Norwood
Prepared for public release: 03/28/2005 - Charlie Wiederhold, 3D Realms
*/
//-------------------------------------------------------------------------

#include "ns.h"
#include "build.h"


#include "keys.h"
#include "mytypes.h"
#include "gamedefs.h"

#include "config.h"
#include "sounds.h"
#include "gamecontrol.h"

#include "game.h"
#include "colormap.h"
#include "network.h"

#include "animlib.h"
#include "anim.h"
#include "../glbackend/glbackend.h"
#include "v_2ddrawer.h"
#include "animtexture.h"

#include "common_game.h"

BEGIN_SW_NS

#define MAX_ANMS 10
anim_t *anm_ptr[MAX_ANMS];

int ANIMnumframes;
unsigned char ANIMpal[3*256];
unsigned char ANIMnum = 0;
short SoundState;
static TArray<uint8_t> buffer;

const char *ANIMname[] =
{
    "sw.anm",
    "swend.anm",
    "sumocinm.anm",
    "zfcin.anm",
};

#define ANIM_TILE(num) (MAXTILES-11 + (num))

void AnimShareIntro(int frame, int numframes)
{
    int zero=0;

    if (frame == numframes-1)
        ototalclock += 120;
    else if (frame == 1)
    {
        PlaySound(DIGI_NOMESSWITHWANG, v3df_none, CHAN_BODY, CHANF_UI);
        ototalclock += 120*3;
    }
    else
        ototalclock += 8;

    if (frame == 5)
    {
        PlaySound(DIGI_INTRO_SLASH, v3df_none, CHAN_BODY, CHANF_UI);
    }
    else if (frame == 15)
    {
        PlaySound(DIGI_INTRO_WHIRL, v3df_none, CHAN_BODY, CHANF_UI);
    }
}

void AnimSerp(int frame, int numframes)
{
    int zero=0;
    ototalclock += 16;

    if (frame == numframes-1)
        ototalclock += 1*120;

    if (frame == 1)
    {
        PlaySound(DIGI_SERPTAUNTWANG, v3df_none, CHAN_BODY, CHANF_UI);
    }
    else if (frame == 16)
    {
        PlaySound(DIGI_SHAREND_TELEPORT, v3df_none, CHAN_BODY, CHANF_UI);
    }
    else if (frame == 35)
    {
        SoundState++;
        PlaySound(DIGI_WANGTAUNTSERP1, v3df_none, CHAN_BODY, CHANF_UI);
    }
    else if (frame == 51)
    {
        SoundState++;
        PlaySound(DIGI_SHAREND_UGLY1, v3df_none, CHAN_BODY, CHANF_UI);
    }
    else if (frame == 64)
    {
        SoundState++;
        PlaySound(DIGI_SHAREND_UGLY2, v3df_none, CHAN_BODY, CHANF_UI);
    }
}

void AnimSumo(int frame, int numframes)
{
    int zero=0;
    ototalclock += 10;

    if (frame == numframes-1)
        ototalclock += 1*120;

    if (frame == 1)
        ototalclock += 30;

    if (frame == 2)
    {
        // hungry
        PlaySound(DIGI_JG41012, v3df_none, CHAN_BODY, CHANF_UI);
    }
    else if (frame == 30)
    {
        PlaySound(DIGI_HOTHEADSWITCH, v3df_none, CHAN_BODY, CHANF_UI);
    }
    else if (frame == 42)
    {
        PlaySound(DIGI_HOTHEADSWITCH, v3df_none, CHAN_BODY, CHANF_UI);
    }
    else if (frame == 59)
    {
        PlaySound(DIGI_JG41028, v3df_none, CHAN_BODY, CHANF_UI);
    }
}

void AnimZilla(int frame, int numframes)
{
    int zero=0;
    ototalclock += 16;

    if (frame == numframes-1)
        ototalclock += 1*120;

    if (frame == 1)
    {
        PlaySound(DIGI_ZC1, v3df_none, CHAN_BODY, CHANF_UI);
    }
    else if (frame == 5)
    {
        PlaySound(DIGI_JG94024, v3df_none, CHAN_BODY, CHANF_UI);
    }
    else if (frame == 14)
    {
        PlaySound(DIGI_ZC2, v3df_none, CHAN_BODY, CHANF_UI);
    }
    else if (frame == 30)
    {
        PlaySound(DIGI_ZC3, v3df_none, CHAN_BODY, CHANF_UI);
    }
    else if (frame == 32)
    {
        PlaySound(DIGI_ZC4, v3df_none, CHAN_BODY, CHANF_UI);
    }
    else if (frame == 37)
    {
        PlaySound(DIGI_ZC5, v3df_none, CHAN_BODY, CHANF_UI);
    }
    else if (frame == 63)
    {
        PlaySound(DIGI_Z16043, v3df_none, CHAN_BODY, CHANF_UI);
        PlaySound(DIGI_ZC6, v3df_none, CHAN_BODY, CHANF_UI);
        PlaySound(DIGI_ZC7, v3df_none, CHAN_BODY, CHANF_UI);
    }
    else if (frame == 72)
    {
        PlaySound(DIGI_ZC7, v3df_none, CHAN_BODY, CHANF_UI);
    }
    else if (frame == 73)
    {
        PlaySound(DIGI_ZC4, v3df_none, CHAN_BODY, CHANF_UI);
    }
    else if (frame == 77)
    {
        PlaySound(DIGI_ZC5, v3df_none, CHAN_BODY, CHANF_UI);
    }
    else if (frame == 87)
    {
        PlaySound(DIGI_ZC8, v3df_none, CHAN_BODY, CHANF_UI);
    }
    else if (frame == 103)
    {
        PlaySound(DIGI_ZC7, v3df_none, CHAN_BODY, CHANF_UI);
    }
    else if (frame == 108)
    {
        PlaySound(DIGI_ZC9, v3df_none, CHAN_BODY, CHANF_UI);
    }
    else if (frame == 120)
    {
        PlaySound(DIGI_JG94039, v3df_none, CHAN_BODY, CHANF_UI);
    }
}

void
playanm(short anim_num)
{
    unsigned char *animbuf;
    int i, length = 0, numframes = 0;
    int32_t handle = -1;
    
    ANIMnum = anim_num;

    inputState.ClearAllInput();

    DSPRINTF(ds,"PlayAnm");
    MONO_PRINT(ds);

    DSPRINTF(ds,"PlayAnm");
    MONO_PRINT(ds);

    TArray<uint8_t> buffer;
    auto fr = fileSystem.OpenFileReader(ANIMname[ANIMnum]);

    if (!fr.isOpen())
        goto ENDOFANIMLOOP;

    buffer = fr.ReadPadded(1);
    fr.Close();

    DSPRINTF(ds,"PlayAnm - Palette Stuff");
    MONO_PRINT(ds);
    anim_t anm;

    if (ANIM_LoadAnim(&anm, buffer.Data(), buffer.Size() - 1) < 0)
    {
        Printf("Error: malformed ANM file \"%s\".\n", ANIMname[ANIMnum]);
        goto ENDOFANIMLOOP;
    }

    ANIM_LoadAnim(&anm, buffer.Data(), buffer.Size()-1);
    ANIMnumframes = ANIM_NumFrames(&anm);
    numframes = ANIMnumframes;


    videoClearViewableArea(0L);

    {
        AnimTextures animtex;
        animtex.SetSize(AnimTexture::Paletted, 320, 200);
        if (ANIMnum == 1)
        {
            // draw the first frame
            animtex.SetFrame(ANIM_GetPalette(&anm), ANIM_DrawFrame(&anm, 1));
            rotatesprite_fs(160 << 16, 100 << 16, 65536, 0, -1, 0, 0, 2 | 8 | 64, animtex.GetFrame());
        }

        SoundState = 0;
        //ototalclock = totalclock + 120*2;
        ototalclock = (int32_t)totalclock;

        for (i = 1; i < numframes; i++)
        {
            while (totalclock < ototalclock)
            {
                handleevents();
                switch (ANIMnum)
                {
                case ANIM_INTRO:
                case ANIM_SERP:
                    if (inputState.CheckAllInput())
                    {
                        goto ENDOFANIMLOOP;
                    }
                    break;
                }

                getpackets();
            }

            switch (ANIMnum)
            {
            case ANIM_INTRO:
                AnimShareIntro(i, numframes);
                break;
            case ANIM_SERP:
                AnimSerp(i, numframes);
                break;
            case ANIM_SUMO:
                AnimSumo(i, numframes);
                break;
            case ANIM_ZILLA:
                AnimZilla(i, numframes);
                break;
            }

	        videoClearViewableArea(0L);
            animtex.SetFrame(ANIM_GetPalette(&anm), ANIM_DrawFrame(&anm, i));
            rotatesprite_fs(160 << 16, 100 << 16, 65536, 0, -1, 0, 0, 2 | 8 | 64, animtex.GetFrame());
            videoNextPage();
            handleevents();
            if (inputState.CheckAllInput())
                break;
        }
    }

    // pause on final frame
    while (totalclock < ototalclock)
    {
        handleevents();
        getpackets();
    }

ENDOFANIMLOOP:

    twod->ClearScreen();
    videoNextPage();

    inputState.ClearAllInput();
}
END_SW_NS
