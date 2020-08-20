//-------------------------------------------------------------------------
/*
Copyright (C) 2010-2019 EDuke32 developers and contributors
Copyright (C) 2019 sirlemonhead, Nuke.YKT
This file is part of PCExhumed.
PCExhumed is free software; you can redistribute it and/or
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
#include "ns.h"
#include "compat.h"
#include "build.h"
#include "exhumed.h"
#include "menu.h"
#include "names.h"
#include "engine.h"
#include "c_bind.h"
#include "status.h"
#include "sound.h"
#include "names.h"
#include "ps_input.h"
#include "view.h"
#include "raze_sound.h"
#include "menu.h"
#include "v_2ddrawer.h"
#include "v_font.h"
#include "texturemanager.h"
#include "gamestate.h"
#include "multipatchtexture.h"
#include "screenjob.h"
#include "sequence.h"

#include <string>

#include <assert.h>

BEGIN_PS_NS

int SyncScreenJob()
{
    while (gamestate == GS_INTERMISSION || gamestate == GS_INTRO)
    {
        UpdateSounds();
        HandleAsync();
        updatePauseStatus();
        D_ProcessEvents();
        ControlInfo info;
        CONTROL_GetInput(&info);
        C_RunDelayedCommands();

        RunScreenJobFrame();	// This handles continuation through its completion callback.
        videoNextPage();
    }
    return 0;
}


//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void InitFonts()
{
    GlyphSet fontdata;
    fontdata.Insert(127, TexMan.FindGameTexture("TINYBLAK")); // this is only here to widen the color range of the font to produce a better translation.

    auto tile = tileGetTexture(159);
    auto pixels = tile->GetTexture()->Get8BitPixels(false);
    // Convert the sheet font to a proportional font by checking the actual character sizes.
    for (int i = 1; i < 128; i++)
    {
        int xpos = (i % 16) * 8;
        int ypos = (i / 16) * 8;
        bool rowset[8]{};
        for (int x = 0; x < 8; x++)
        {
            for (int y = 0; y < 8; y++)
            {
                int pixel = pixels[xpos + x + 128 * (ypos + y)];
                if (pixel)
                {
                    rowset[x] = true;
                    break;
                }
            }
        }
        int left = 0;
        int right = 7;
        /* probably not such a good idea after all...
        while (left <= right)
        {
            bool didit = false;
            if (!rowset[left]) left++, didit = true;
            if (!rowset[right]) right--, didit = true;
            if (!didit) break;
        }*/
        if (left < right)
        {
            xpos += left;
            int width = right - left + 1;
            int height = 8;

            TArray<TexPartBuild> part(1, true);
            part[0].OriginX = -xpos;
            part[0].OriginY = -ypos;
            part[0].TexImage = static_cast<FImageTexture*>(tile->GetTexture());
            FMultiPatchTexture* image = new FMultiPatchTexture(width, height, part, false, false);
            FImageTexture* tex = new FImageTexture(image);
            auto gtex = MakeGameTexture(tex, nullptr, ETextureType::FontChar);
            gtex->SetWorldPanning(true);
            gtex->SetOffsets(0, 0, 0);
            gtex->SetOffsets(1, 0, 0);
            TexMan.AddGameTexture(gtex);
            fontdata.Insert(i, gtex);
        }
    }
    SmallFont2 = new ::FFont("SmallFont2", nullptr, "defsmallfont2", 0, 0, 0, -1, 4, false, false, false, &fontdata);
    SmallFont2->SetKerning(1);
    fontdata.Clear();

    for (int i = 0; i < 26; i++)
    {
        fontdata.Insert('A' + i, tileGetTexture(3522 + i));
    }
    for (int i = 0; i < 10; i++)
    {
        fontdata.Insert('0' + i, tileGetTexture(3555 + i));
    }
    fontdata.Insert('.', tileGetTexture(3548));
    fontdata.Insert('!', tileGetTexture(3549));
    fontdata.Insert('?', tileGetTexture(3550));
    fontdata.Insert(',', tileGetTexture(3551));
    fontdata.Insert('`', tileGetTexture(3552));
    fontdata.Insert('"', tileGetTexture(3553));
    fontdata.Insert('-', tileGetTexture(3554));
    fontdata.Insert('_', tileGetTexture(3554));
    fontdata.Insert(127, TexMan.FindGameTexture("TINYBLAK")); // this is only here to widen the color range of the font to produce a better translation.
    GlyphSet::Iterator it(fontdata);
    GlyphSet::Pair* pair;
    while (it.NextPair(pair)) pair->Value->SetOffsetsNotForFont();
    SmallFont = new ::FFont("SmallFont", nullptr, "defsmallfont", 0, 0, 0, -1, 4, false, false, false, &fontdata);
    SmallFont->SetKerning(1);
    fontdata.Clear();

}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

DScreenJob *PlayMovie(const char* fileName);

void DoTitle(CompletionFunc completion)
{
    JobDesc jobs[5];
    int job = 0;

    jobs[job++] = { Create<DImageScreen>(tileGetTexture(EXHUMED ? kTileBMGLogo : kTilePIELogo), DScreenJob::fadein | DScreenJob::fadeout) };
    jobs[job++] = { Create<DImageScreen>(tileGetTexture(seq_GetSeqPicnum(kSeqScreens, 0, 0)), DScreenJob::fadein | DScreenJob::fadeout) };
    jobs[job++] = { PlayMovie("book.mov") };

    RunScreenJob(jobs, job, completion);

#if 0
    short skullDurations[] = { 6, 25, 43, 50, 68, 78, 101, 111, 134, 158, 173, 230, 6000 };
    videoSetViewableArea(0, 0, xdim - 1, ydim - 1);


    if (videoGetRenderMode() == REND_CLASSIC)
        FadeOut(0);

    GrabPalette();

    PlayLocalSound(StaticSound[59], 0, true, CHANF_UI);

    EraseScreen(4);

    playCDtrack(19, true);

    videoNextPage();
    FadeIn();
    WaitVBL();

    int String_Copyright = FindGString("COPYRIGHT");

    const char* a = gString[String_Copyright];
    const char* b = gString[String_Copyright + 1];

    menu_DoPlasma();

    int nTile = kSkullHead;

    overwritesprite(160, 100, kSkullHead, 0, 3, kPalNormal);
    overwritesprite(161, 130, kSkullJaw, 0, 3, kPalNormal);
    videoNextPage();

    WaitNoKey(2, KeyFn1);

    if (time(0) & 0xF) {
        PlayGameOverSound();
    }
    else {
        PlayLocalSound(StaticSound[61], 0, false, CHANF_UI);
    }

    int nStartTime = (int)totalclock;
    int nCount = 0;
    int var_18 = (int)totalclock + skullDurations[0];
    int var_4 = 0;

    int esi = 130;



    inputState.ClearAllInput();
    while (LocalSoundPlaying())
    {
        HandleAsync();
        if (inputState.CheckAllInput()) break;

        menu_DoPlasma();
        overwritesprite(160, 100, nTile, 0, 3, kPalNormal);

        int nStringWidth = MyGetStringWidth(a);

        int y = 200 - 24;
        myprintext((320 / 2 - nStringWidth / 2), y, a, 0);

        nStringWidth = MyGetStringWidth(b);

        y = 200 - 16;
        myprintext((320 / 2 - nStringWidth / 2), y, b, 0);

        if ((int)totalclock > var_18)
        {
            nCount++;

            if (nCount > 12) break;
            var_18 = nStartTime + skullDurations[nCount];
            var_4 = var_4 == 0;
        }

        short nTile = kSkullJaw;

        if (var_4)
        {
            if (esi >= 135) {
                nTile = kTile3583;
            }
            else {
                esi += 5;
            }
        }
        else if (esi <= 130)
        {
            esi = 130;
        }
        else
        {
            esi -= 2;
        }

        y = 0;

        if (nTile == kTile3583)
        {
            y = 131;
        }
        else
        {
            y = esi;

            if (y > 135) {
                y = 135;
            }
        }

        overwritesprite(161, y, nTile, 0, 3, kPalNormal);
        videoNextPage();
    }

    WaitNoKey(1, KeyFn1);
#endif
}





END_PS_NS
