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
#include "build.h"
#include "exhumed.h"
#include "names.h"
#include "engine.h"
#include "c_bind.h"
#include "sound.h"
#include "names.h"
#include "view.h"
#include "raze_sound.h"
#include "v_2ddrawer.h"
#include "v_font.h"
#include "texturemanager.h"
#include "gamestate.h"
#include "multipatchtexture.h"
#include "screenjob.h"
#include "sequence.h"
#include "v_draw.h"
#include "m_random.h"
#include "gstrings.h"
#include "gamefuncs.h"
#include "c_bind.h"
#include "vm.h"
#include "razefont.h"
#include "buildtiles.h"
#include <assert.h>

BEGIN_PS_NS

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void DrawAbs(FGameTexture* tex, double x, double y, int shade = 0)
{
    DrawTexture(twod, tex, x, y, DTA_FullscreenScale, FSMode_Fit320x200, DTA_TopLeft, true, DTA_Color, shadeToLight(shade), TAG_DONE);
}

void DrawRel(FGameTexture* tex, double x, double y, int shade)
{
    // This is slightly different than what the backend does here, but critical for some graphics.
    int offx = (int(tex->GetDisplayWidth()) >> 1) + int(tex->GetDisplayLeftOffset());
    int offy = (int(tex->GetDisplayHeight()) >> 1) + int(tex->GetDisplayTopOffset());
    DrawTexture(twod, tex, x - offx, y - offy, DTA_FullscreenScale, FSMode_Fit320x200, DTA_TopLeft, true, DTA_Color, shadeToLight(shade), TAG_DONE);
}

enum
{
    kPlasmaWidth = 320,
    kPlasmaHeight = 80,
};

void DrawLogo()
{
    const auto pLogoTex = TexMan.GetGameTexture(GameLogo());
    DrawRel(pLogoTex, 160, 40);

    // draw the fire urn/lamp thingies
    const int urnclock = (I_GetBuildTime() / 16) & 3;
    static int urnidx[] = { kTexUrn1, kTexUrn2, kTexUrn3, kTexUrn4, kTexUrn1, kTexUrn2 };
    DrawRel(TexMan.GetGameTexture(aTexIds[urnidx[urnclock]]), 50, 150);
    DrawRel(TexMan.GetGameTexture(aTexIds[urnidx[urnclock + 2]]), 270, 150);

}
//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------
static FRandom rnd_plasma;

void menu_DoPlasma()
{
    static uint8_t* PlasmaBuffer;
    static FGameTexture* nPlasmaTile;
    static FGameTexture* nPlasmaTileAlt;
    static int nSmokeBottom;
    static int nSmokeRight;
    static int nSmokeTop;
    static int nSmokeLeft;
    static int nextPlasmaTic;
    static int plasma_A[5];
    static int plasma_B[5];
    static int plasma_C[5];

    if (!nPlasmaTile)
    {
        nPlasmaTile = TexMan.GetGameTexture(aTexIds[kTexPlasmaTile1]);
        nPlasmaTileAlt = TexMan.GetGameTexture(aTexIds[kTexPlasmaTile2]);
        memset(plasma_A, 0, sizeof(plasma_A));
        memset(plasma_B, 0, sizeof(plasma_B));
        memset(plasma_C, 0, sizeof(plasma_C));
    }

    const auto nLogoTexid = GameLogo();
    const auto pLogoTex = TexMan.GetGameTexture(nLogoTexid);
    const int logowidth = (int)pLogoTex->GetDisplayWidth();
    const int logoheight = (int)pLogoTex->GetDisplayHeight();
    const auto ptile = nPlasmaTile;
    const int pclock = I_GetBuildTime();

    while (pclock >= nextPlasmaTic || !PlasmaBuffer)
    {
        nextPlasmaTic += 4;

        if (!PlasmaBuffer)
        {
            auto pixels = GetWritablePixels(aTexIds[kTexPlasmaTile1]);
            memset(pixels, 96, kPlasmaWidth * kPlasmaHeight);

            PlasmaBuffer = GetWritablePixels(aTexIds[kTexPlasmaTile2]);
            memset(PlasmaBuffer, 96, kPlasmaWidth * kPlasmaHeight);

            nSmokeLeft = 160 - logowidth / 2;
            nSmokeRight = nSmokeLeft + logowidth;
            nSmokeTop = 40 - logoheight / 2;
            nSmokeBottom = nSmokeTop + logoheight - 1;

            for (int i = 0; i < 5; i++)
            {
                plasma_C[i] = IntToFixed(nSmokeLeft + rand() % logowidth);
                plasma_B[i] = (rnd_plasma.GenRand32() % 327680) + 0x10000;

                if (rnd_plasma.GenRand32()&1)
                    plasma_B[i] = -plasma_B[i];

                plasma_A[i] = rnd_plasma.GenRand32() & 1;
            }
        }

        uint8_t* plasmapix = GetWritablePixels(nPlasmaTile->GetID());
        uint8_t* r_ebx = plasmapix + 81;
        const uint8_t* r_edx = GetWritablePixels(nPlasmaTileAlt->GetID()) + 81; // flip between two instances

        for (int x = 0; x < kPlasmaWidth - 2; x++)
        {
            for (int y = 0; y < kPlasmaHeight - 2; y++)
            {
                uint8_t al = *r_edx;
                uint8_t cl;

                if (al != 96)
                {
                    *r_ebx = (al > 158) ? (al - 1) : 96;
                }
                else
                {
                    if (rnd_plasma.GenRand32() & 1)
                    {
                        *r_ebx = *r_edx;
                    }
                    else
                    {
                        al = *(r_edx + 1);
                        cl = *(r_edx - 1);
                        if (al <= cl) al = cl;

                        cl = al;
                        al = *(r_edx - 80);
                        if (cl <= al) cl = al;

                        al = *(r_edx + 80);
                        if (cl <= al) cl = al;

                        al = *(r_edx + 80);
                        if (cl <= al) cl = al;

                        al = *(r_edx + 80);
                        if (cl <= al) cl = al;

                        al = *(r_edx - 79);
                        if (cl > al) al = cl;

                        cl = *(r_edx - 81);
                        if (al <= cl) al = cl;

                        cl = al;

                        if (al <= 159)
                        {
                            *r_ebx = 96;
                        }
                        else
                        {
                            if (!(rnd_plasma.GenRand32() & 1)) 
                                cl--;

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

        const auto logopix = GetRawPixels(nLogoTexid);

        for (int j = 0; j < 5; j++)
        {
            const int pB = plasma_B[j];
            const int pC = plasma_C[j];
            const int badOffset = FixedToInt(pC) < nSmokeLeft || FixedToInt(pC) >= nSmokeRight;
            const uint8_t* ptr3 = (logopix + (FixedToInt(pC) - nSmokeLeft) * logoheight);

            plasma_C[j] += plasma_B[j];

            if ((pB > 0 && FixedToInt(plasma_C[j]) >= nSmokeRight) || (pB < 0 && FixedToInt(plasma_C[j]) <= nSmokeLeft))
            {
                int esi = plasma_A[j];
                plasma_B[j] = -plasma_B[j];
                plasma_A[j] = esi == 0;
            }

            if (badOffset)
                continue;

            int nSmokeOffset = 0;

            if (plasma_A[j])
            {
                nSmokeOffset = nSmokeTop;

                while (nSmokeOffset < nSmokeBottom)
                {
                    uint8_t al = *ptr3;

                    if (al != TRANSPARENT_INDEX && al != 96)
                        break;

                    nSmokeOffset++;
                    ptr3++;
                }
            }
            else
            {
                nSmokeOffset = nSmokeBottom;
                ptr3 += logoheight - 1;

                while (nSmokeOffset > nSmokeTop)
                {
                    uint8_t al = *ptr3;

                    if (al != TRANSPARENT_INDEX && al != 96)
                        break;

                    nSmokeOffset--;
                    ptr3--;
                }
            }

            uint8_t* v28 = plasmapix + (80 * FixedToInt(plasma_C[j]));
            v28[nSmokeOffset] = 175;
        }

        // flip between both instances
        std::swap(nPlasmaTile, nPlasmaTileAlt);
    }

    DrawAbs(ptile, 0, 0);
    DrawLogo();
}


DEFINE_ACTION_FUNCTION(_Exhumed, DrawPlasma)
{
    menu_DoPlasma();
    return 0;
}

//---------------------------------------------------------------------------
//
// text overlay (native version still needed for Ramses texts.
//
//---------------------------------------------------------------------------

void TextOverlay::Create(const FString& text, int pal)
{
    lastclock = 0;
    FString ttext = GStrings(text);
    font = PickSmallFont(ttext);
    screentext = ttext.Split("\n");
    ComputeCinemaText();
}

void TextOverlay::Start(double starttime)
{
    lastclock = starttime;
}

void TextOverlay::ComputeCinemaText()
{
    int i = 0;
    for (auto &line : screentext)
    { 
        int nWidth = SmallFont->StringWidth(line);
        nLeft[i++] = 160 - nWidth / 2;
    }

    nCrawlY = 199;
    nHeight = screentext.Size() * 10;
}

void TextOverlay::ReadyCinemaText(const char* nVal)
{
    FString label = nVal[0] == '$'? GStrings(nVal +1) : nVal;
    Create(label, 0);
}

void TextOverlay::DisplayText()
{
    if (nHeight + nCrawlY > 0)
    {
        double y = nCrawlY;
        unsigned int i = 0;

        while (i < screentext.Size() && y <= 199)
        {
            if (y >= -10)
                DrawText(twod, font, CR_NATIVEPAL, nLeft[i], y, screentext[i], DTA_FullscreenScale, FSMode_Fit320x200, DTA_TranslationIndex, TRANSLATION(Translation_BasePalettes, currentCinemaPalette), TAG_DONE);

            i++;
            y += 10;
        }
    }
}

bool TextOverlay::AdvanceCinemaText(double clock)
{
    if (nHeight + nCrawlY > 0 || CDplaying())
    {
        nCrawlY -= min(clock - lastclock, 1.5) / 15.;   // do proper interpolation.
        lastclock = clock;
        return true;
    }

    return false;
}


//---------------------------------------------------------------------------
//
// cinema
//
//---------------------------------------------------------------------------

static const char* const cinpalfname[] = {
    "3454.pal",
    "3452.pal",
    "3449.pal",
    "3445.pal",
    "set.pal",
    "3448.pal",
    "3446.pal",
    "hsc1.pal",
    "2972.pal",
    "2973.pal",
    "2974.pal",
    "2975.pal",
    "2976.pal",
    "heli.pal",
    "2978.pal",
    "terror.pal"
};

void uploadCinemaPalettes()
{
    for (unsigned i = 0; i < countof(cinpalfname); i++)
    {
        uint8_t palette[768] = {};
        auto hFile = fileSystem.OpenFileReader(cinpalfname[i]);

        if (hFile.isOpen())
            hFile.Read(palette, 768);

        for (auto& c : palette)
            c <<= 2;

        paletteSetColorTable(ANIMPAL+i, palette, false, true);
    }
}

//---------------------------------------------------------------------------
//
// this accesses the tile data and needs to remain native.
//
//---------------------------------------------------------------------------

static int DoStatic(int a, int b)
{
    int y = 160 - a / 2;
    const int left = 81 - b / 2;
    const int bottom = y + a;
    const int right = left + b;

    const auto pixels = GetWritablePixels(aTexIds[kTexTileLoboLaptop], true);
    auto pTile = (pixels + (200 * y)) + left;

    for(;y < bottom; y++)
    {
        uint8_t* pixel = pTile;
        pTile += 200;

        for (int x = left; x < right; x++)
        {
            *pixel++ = RandomBit() * 16;
        }
    }
    return aTexIds[kTexTileLoboLaptop].GetIndex();
}

static int UndoStatic()
{
    const auto texid = aTexIds[kTexTileLoboLaptop];
    GetWritablePixels(texid, true);
    return texid.GetIndex();
}

DEFINE_ACTION_FUNCTION_NATIVE(DLastLevelCinema, DoStatic, DoStatic)
{
    PARAM_PROLOGUE;
    PARAM_INT(x);
    PARAM_INT(y);
    ACTION_RETURN_INT(DoStatic(x, y));
}

DEFINE_ACTION_FUNCTION_NATIVE(DLastLevelCinema, UndoStatic, UndoStatic)
{
    ACTION_RETURN_INT(UndoStatic());
}

END_PS_NS
