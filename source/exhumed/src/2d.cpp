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
#include "v_draw.h"
#include "m_random.h"

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

void DrawAbs(int tile, double x, double y)
{
    DrawTexture(twod, tileGetTexture(tile), x, y, DTA_FullscreenScale, FSMode_ScaleToFit43, DTA_VirtualWidth, 320, DTA_VirtualHeight, 200, DTA_TopLeft, true, TAG_DONE);
}

void DrawRel(int tile, double x, double y)
{
    // This is slightly different than what the backend does here, but critical for some graphics.
    int offx = (tileWidth(tile) >> 1) + tileLeftOffset(tile);
    int offy = (tileHeight(tile) >> 1) + tileTopOffset(tile);
    DrawAbs(tile, x - offx, y - offy);
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

// this might be static within the DoPlasma function?
static uint8_t* PlasmaBuffer;
static int nPlasmaTile = kTile4092;
static int nLogoTile;
static unsigned int nSmokeBottom;
static unsigned int nSmokeRight;
static unsigned int nSmokeTop;
static unsigned int nSmokeLeft;
static int nextPlasmaTic;
static int plasma_A[5] = { 0 };
static int plasma_B[5] = { 0 };
static int plasma_C[5] = { 0 };
static FRandom rnd_plasma;

enum
{
    kPlasmaWidth = 320,
    kPlasmaHeight = 80,
};

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void menu_DoPlasma()
{
    int ptile = nPlasmaTile;
    if (totalclock >= nextPlasmaTic || !PlasmaBuffer)
    {
        nextPlasmaTic = (int)totalclock + 4;

        if (!nLogoTile)
            nLogoTile = EXHUMED ? kExhumedLogo : kPowerslaveLogo;

        if (!PlasmaBuffer)
        {
            auto pixels = TileFiles.tileCreate(kTile4092, kPlasmaWidth, kPlasmaHeight);
            memset(pixels, 96, kPlasmaWidth * kPlasmaHeight);

            PlasmaBuffer = TileFiles.tileCreate(kTile4093, kPlasmaWidth, kPlasmaHeight);
            memset(PlasmaBuffer, 96, kPlasmaWidth * kPlasmaHeight);

            nSmokeLeft = 160 - tilesiz[nLogoTile].x / 2;
            nSmokeRight = nSmokeLeft + tilesiz[nLogoTile].x;

            nSmokeTop = 40 - tilesiz[nLogoTile].y / 2;
            nSmokeBottom = nSmokeTop + tilesiz[nLogoTile].y - 1;

            for (int i = 0; i < 5; i++)
            {
                int logoWidth = tilesiz[nLogoTile].x;
                plasma_C[i] = (nSmokeLeft + rand() % logoWidth) << 16;
                plasma_B[i] = (rnd_plasma.GenRand32() % 327680) + 0x10000;

                if (rnd_plasma.GenRand32()&1) {
                    plasma_B[i] = -plasma_B[i];
                }

                plasma_A[i] = rnd_plasma.GenRand32() & 1;
            }
        }

        uint8_t* plasmapix = tileData(nPlasmaTile);
        uint8_t* r_ebx = plasmapix + 81;
        const uint8_t* r_edx = tileData(nPlasmaTile ^ 1) + 81; // flip between value of 4092 and 4093 with xor

        for (int x = 0; x < kPlasmaWidth - 2; x++)
        {
            for (int y = 0; y < kPlasmaHeight - 2; y++)
            {
                uint8_t al = *r_edx;

                if (al != 96)
                {
                    if (al > 158) {
                        *r_ebx = al - 1;
                    }
                    else {
                        *r_ebx = 96;
                    }
                }
                else
                {
                    if (rnd_plasma.GenRand32() & 1) {
                        *r_ebx = *r_edx;
                    }
                    else
                    {
                        uint8_t al = *(r_edx + 1);
                        uint8_t cl = *(r_edx - 1);

                        if (al <= cl) {
                            al = cl;
                        }

                        cl = al;
                        al = *(r_edx - 80);
                        if (cl <= al) {
                            cl = al;
                        }

                        al = *(r_edx + 80);
                        if (cl <= al) {
                            cl = al;
                        }

                        al = *(r_edx + 80);
                        if (cl <= al) {
                            cl = al;
                        }

                        al = *(r_edx + 80);
                        if (cl <= al) {
                            cl = al;
                        }

                        al = *(r_edx - 79);
                        if (cl > al) {
                            al = cl;
                        }

                        cl = *(r_edx - 81);
                        if (al <= cl) {
                            al = cl;
                        }

                        cl = al;

                        if (al <= 159) {
                            *r_ebx = 96;
                        }
                        else
                        {
                            if (!(rnd_plasma.GenRand32() & 1)) 
                            {
                                cl--;
                            }

                            *r_ebx = cl;
                        }
                    }
                }

                // before restarting inner loop
                r_edx++;
                r_ebx++;
            }

            // before restarting outer loop
            r_edx += 2;
            r_ebx += 2;
        }

        auto logopix = tilePtr(nLogoTile);

        for (int j = 0; j < 5; j++)
        {
            int pB = plasma_B[j];
            int pC = plasma_C[j];
            int badOffset = (pC >> 16) < nSmokeLeft || (pC >> 16) >= nSmokeRight;

            const uint8_t* ptr3 = (logopix + ((pC >> 16) - nSmokeLeft) * tilesiz[nLogoTile].y);

            plasma_C[j] += plasma_B[j];

            if ((pB > 0 && (plasma_C[j] >> 16) >= nSmokeRight) || (pB < 0 && (plasma_C[j] >> 16) <= nSmokeLeft))
            {
                int esi = plasma_A[j];
                plasma_B[j] = -plasma_B[j];
                plasma_A[j] = esi == 0;
            }

            if (badOffset)
                continue;

            unsigned int nSmokeOffset = 0;

            if (plasma_A[j])
            {
                nSmokeOffset = nSmokeTop;

                while (nSmokeOffset < nSmokeBottom)
                {
                    uint8_t al = *ptr3;
                    if (al != TRANSPARENT_INDEX && al != 96) {
                        break;
                    }

                    nSmokeOffset++;
                    ptr3++;
                }
            }
            else
            {
                nSmokeOffset = nSmokeBottom;

                ptr3 += tilesiz[nLogoTile].y - 1;

                while (nSmokeOffset > nSmokeTop)
                {
                    uint8_t al = *ptr3;
                    if (al != TRANSPARENT_INDEX && al != 96) {
                        break;
                    }

                    nSmokeOffset--;
                    ptr3--;
                }
            }

            uint8_t* v28 = plasmapix + (80 * (plasma_C[j] >> 16));
            v28[nSmokeOffset] = 175;
        }

        tileInvalidate(nPlasmaTile, -1, -1);

        // flip between tile 4092 and 4093
        if (nPlasmaTile == kTile4092) {
            nPlasmaTile = kTile4093;
        }
        else if (nPlasmaTile == kTile4093) {
            nPlasmaTile = kTile4092;
        }
    }
    DrawAbs(ptile, 0, 0);
    DrawRel(nLogoTile, 160, 40);

    // draw the fire urn/lamp thingies
    int dword_9AB5F = ((int)totalclock / 16) & 3;

    DrawRel(kTile3512 + dword_9AB5F, 50, 150);
    DrawRel(kTile3512 + ((dword_9AB5F + 2) & 3), 270, 150);
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

class DLobotomyScreen : public DImageScreen
{
public:
	DLobotomyScreen(FGameTexture *tex, int fade) : DImageScreen(tex, fade)
	{}
	
    int Frame(uint64_t clock, bool skiprequest) override
    {
		if (clock == 0) PlayLocalSound(StaticSound[kSoundJonLaugh2], 7000, false, CHANF_UI);
		return DImageScreen::Frame(clock, skiprequest);
	}
};

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

static const short skullDurations[] = { 6, 25, 43, 50, 68, 78, 101, 111, 134, 158, 173, 230, 600 };

class DMainTitle : public DScreenJob
{
    int String_Copyright;
    const char* a;
    const char* b;
    int state = 0;
    int var_18;
    int var_4 = 0;
    int esi = 130;
    int nCount = 0;
    int start;


public:
    DMainTitle() : DScreenJob(fadein)
    {
        String_Copyright = FindGString("COPYRIGHT");
        a = gString[String_Copyright];
        b = gString[String_Copyright + 1];
        var_18 = skullDurations[0];
    }

    int Frame(uint64_t clock, bool skiprequest) override
    {
        int ticker = clock * 120 / 1'000'000'000;
        if (clock == 0)
        {
            PlayLocalSound(StaticSound[59], 0, true, CHANF_UI);
            playCDtrack(19, true);
        }
        if (clock > 1'000'000 && state == 0 && !soundEngine->IsSourcePlayingSomething(SOURCE_None, nullptr,CHAN_AUTO, -1))
        {
            if (time(0) & 0xF)
                PlayGameOverSound();
            else 
                PlayLocalSound(StaticSound[61], 0, false, CHANF_UI);
            state = 1;
            start = ticker;
        }

        menu_DoPlasma();

        DrawRel(kSkullHead, 160, 100);
        switch (state)
        {
        case 0:
            DrawRel(kSkullJaw, 161, 130);
            break;

        case 1:
        {
            int nStringWidth = SmallFont->StringWidth(a);
            DrawText(twod, SmallFont, CR_UNTRANSLATED, 160 - nStringWidth / 2, 200 - 24, a, DTA_FullscreenScale, FSMode_ScaleToFit43, DTA_VirtualWidth, 320, DTA_VirtualHeight, 200, TAG_DONE);
            nStringWidth = SmallFont->StringWidth(b);
            DrawText(twod, SmallFont, CR_UNTRANSLATED, 160 - nStringWidth / 2, 200 - 16, b, DTA_FullscreenScale, FSMode_ScaleToFit43, DTA_VirtualWidth, 320, DTA_VirtualHeight, 200, TAG_DONE);

            if (ticker > var_18)
            {
                nCount++;
                if (nCount > 12) return 0;
                var_18 = start + skullDurations[nCount];
                var_4 = var_4 == 0;
            }

            short nTile = kSkullJaw;

            if (var_4)
            {
                if (esi >= 135) nTile = kTile3583;
                else esi += 5;
            }
            else if (esi <= 130) esi = 130;
            else esi -= 2;

            int y;

            if (nTile == kTile3583)
            {
                y = 131;
            }
            else
            {
                y = esi;
                if (y > 135) y = 135;
            }

            DrawRel(nTile, 161, y);
            break;
        }
        }
        return skiprequest? -1 : 1;
    }
};

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
    jobs[job++] = { Create<DLobotomyScreen>(tileGetTexture(seq_GetSeqPicnum(kSeqScreens, 0, 0)), DScreenJob::fadein | DScreenJob::fadeout) };
    jobs[job++] = { PlayMovie("book.mov") };
    jobs[job++] = { Create<DMainTitle>() };

    RunScreenJob(jobs, job, completion);

}

END_PS_NS
