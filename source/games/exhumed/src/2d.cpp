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
#include "names.h"
#include "engine.h"
#include "c_bind.h"
#include "status.h"
#include "sound.h"
#include "names.h"
#include "input.h"
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

#include <string>

#include <assert.h>

BEGIN_PS_NS

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void DrawAbs(int tile, double x, double y, int shade = 0)
{
    DrawTexture(twod, tileGetTexture(tile), x, y, DTA_FullscreenScale, FSMode_Fit320x200, DTA_TopLeft, true, DTA_Color, shadeToLight(shade), TAG_DONE);
}

void DrawRel(int tile, double x, double y, int shade)
{
    // This is slightly different than what the backend does here, but critical for some graphics.
    int offx = (tileWidth(tile) >> 1) + tileLeftOffset(tile);
    int offy = (tileHeight(tile) >> 1) + tileTopOffset(tile);
    DrawAbs(tile, x - offx, y - offy, shade);
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

// this might be static within the DoPlasma function?
static uint8_t* PlasmaBuffer;
static int nPlasmaTile = kTile4092;
static int nSmokeBottom;
static int nSmokeRight;
static int nSmokeTop;
static int nSmokeLeft;
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
    auto nLogoTile = GameLogo();
    int lw = tileWidth(nLogoTile);
    int lh = tileHeight(nLogoTile);

    int ptile = nPlasmaTile;
    int pclock = I_GetBuildTime();
    while (pclock >= nextPlasmaTic || !PlasmaBuffer)
    {
        nextPlasmaTic += 4;

        if (!PlasmaBuffer)
        {
            auto pixels = TileFiles.tileCreate(kTile4092, kPlasmaWidth, kPlasmaHeight);
            memset(pixels, 96, kPlasmaWidth * kPlasmaHeight);

            PlasmaBuffer = TileFiles.tileCreate(kTile4093, kPlasmaWidth, kPlasmaHeight);
            memset(PlasmaBuffer, 96, kPlasmaWidth * kPlasmaHeight);


            nSmokeLeft = 160 - lw / 2;
            nSmokeRight = nSmokeLeft + lw;

            nSmokeTop = 40 - lh / 2;
            nSmokeBottom = nSmokeTop + lh - 1;

            for (int i = 0; i < 5; i++)
            {
                int logoWidth = lw;
                plasma_C[i] = IntToFixed(nSmokeLeft + rand() % logoWidth);
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
            int badOffset = FixedToInt(pC) < nSmokeLeft || FixedToInt(pC) >= nSmokeRight;

            const uint8_t* ptr3 = (logopix + (FixedToInt(pC) - nSmokeLeft) * lh);

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

                ptr3 += lh - 1;

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

            uint8_t* v28 = plasmapix + (80 * FixedToInt(plasma_C[j]));
            v28[nSmokeOffset] = 175;
        }

        TileFiles.InvalidateTile(nPlasmaTile);

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
    int dword_9AB5F = (pclock / 16) & 3;

    DrawRel(kTile3512 + dword_9AB5F, 50, 150);
    DrawRel(kTile3512 + ((dword_9AB5F + 2) & 3), 270, 150);
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
    screentext = label.Split("\n");
    ComputeCinemaText();
}

void TextOverlay::DisplayText()
{
    if (nHeight + nCrawlY > 0)
    {
        double y = nCrawlY;
        unsigned int i = 0;

        while (i < screentext.Size() && y <= 199)
        {
            if (y >= -10) {
                DrawText(twod, font, CR_NATIVEPAL, nLeft[i], y, screentext[i], DTA_FullscreenScale, FSMode_Fit320x200, DTA_TranslationIndex, TRANSLATION(Translation_BasePalettes, currentCinemaPalette), TAG_DONE);
            }

            i++;
            y += 10;
        }
    }
}

bool TextOverlay::AdvanceCinemaText(double clock)
{
    if (nHeight + nCrawlY > 0 || CDplaying())
    {
        nCrawlY-= (clock - lastclock) / 15.;   // do proper interpolation.
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

static const char * const cinpalfname[] = {
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
    TileFiles.tileMakeWritable(kTileLoboLaptop);
    auto tex = dynamic_cast<FRestorableTile*>(tileGetTexture(kTileLoboLaptop)->GetTexture()->GetImage());
    if (tex) tex->Reload();
        auto pixels = TileFiles.tileMakeWritable(kTileLoboLaptop);

    int y = 160 - a / 2;
    int left = 81 - b / 2;

    int bottom = y + a;
    int right = left + b;

    auto pTile = (pixels + (200 * y)) + left;

        TileFiles.InvalidateTile(kTileLoboLaptop);

    for(;y < bottom; y++)
        {
        uint8_t* pixel = pTile;
            pTile += 200;

        for (int x = left; x < right; x++)
            {
            *pixel++ = RandomBit() * 16;
            }
        }
    return tileGetTexture(kTileLoboLaptop)->GetID().GetIndex();
}

static int UndoStatic()
{
        auto tex = dynamic_cast<FRestorableTile*>(tileGetTexture(kTileLoboLaptop)->GetTexture()->GetImage());
        if (tex) tex->Reload();
        TileFiles.InvalidateTile(kTileLoboLaptop);
    return tileGetTexture(kTileLoboLaptop)->GetID().GetIndex();
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
