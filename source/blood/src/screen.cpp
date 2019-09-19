//-------------------------------------------------------------------------
/*
Copyright (C) 2010-2019 EDuke32 developers and contributors
Copyright (C) 2019 Nuke.YKT

This file is part of NBlood.

NBlood is free software; you can redistribute it and/or
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
#include <string.h>
#include "a.h"
#include "build.h"
#include "colmatch.h"
#include "common_game.h"

#include "blood.h"
#include "config.h"
#include "resource.h"
#include "screen.h"

LOADITEM PLU[15] = {
    { 0, "NORMAL" },
    { 1, "SATURATE" },
    { 2, "BEAST" },
    { 3, "TOMMY" },
    { 4, "SPIDER3" },
    { 5, "GRAY" },
    { 6, "GRAYISH" },
    { 7, "SPIDER1" },
    { 8, "SPIDER2" },
    { 9, "FLAME" },
    { 10, "COLD" },
    { 11, "P1" },
    { 12, "P2" },
    { 13, "P3" },
    { 14, "P4" }
};

LOADITEM PAL[5] = {
    { 0, "BLOOD" },
    { 1, "WATER" },
    { 2, "BEAST" },
    { 3, "SEWER" },
    { 4, "INVULN1" }
};


bool DacInvalid = true;
static char(*gammaTable)[256];
RGB curDAC[256];
RGB baseDAC[256];
static RGB fromDAC[256];
static RGB toRGB;
static RGB *palTable[5];
static int curPalette;
static int curGamma;
int gGammaLevels;
bool gFogMode = false;

void scrResetPalette(void)
{
    paletteSetColorTable(0, (uint8_t*)palTable[0]);
}

void gSetDacRange(int start, int end, RGB *pPal)
{
    UNREFERENCED_PARAMETER(start);
    UNREFERENCED_PARAMETER(end);
    if (videoGetRenderMode() == REND_CLASSIC)
    {
        memcpy(palette, pPal, sizeof(palette));
        videoSetPalette(gBrightness>>2, 0, 0);
    }
}

void scrLoadPLUs(void)
{
    if (gFogMode)
    {
        DICTNODE *pFog = gSysRes.Lookup("FOG", "FLU");
        if (!pFog)
            ThrowError("FOG.FLU not found");
        palookup[0] = (char*)gSysRes.Lock(pFog);
        for (int i = 0; i < 15; i++)
            palookup[PLU[i].id] = palookup[0];
        parallaxvisibility = 3072;
        return;
    }
    for (int i = 0; i < 15; i++)
    {
        DICTNODE *pPlu = gSysRes.Lookup(PLU[i].name, "PLU");
        if (!pPlu)
            ThrowError("%s.PLU not found", PLU[i].name);
        if (pPlu->size / 256 != 64)
            ThrowError("Incorrect PLU size");
        palookup[PLU[i].id] = (char*)gSysRes.Lock(pPlu);
    }
#ifdef USE_OPENGL
    palookupfog[1].r = 255;
    palookupfog[1].g = 255;
    palookupfog[1].b = 255;
#endif
}

#ifdef USE_OPENGL
glblend_t const bloodglblend =
{
    {
        { 1.f/3.f, BLENDFACTOR_SRC_ALPHA, BLENDFACTOR_ONE_MINUS_SRC_ALPHA, 0 },
        { 2.f/3.f, BLENDFACTOR_SRC_ALPHA, BLENDFACTOR_ONE_MINUS_SRC_ALPHA, 0 },
    },
};
#endif

void scrLoadPalette(void)
{
    paletteloaded = 0;
    initprintf("Loading palettes\n");
    for (int i = 0; i < 5; i++)
    {
        DICTNODE *pPal = gSysRes.Lookup(PAL[i].name, "PAL");
        if (!pPal)
            ThrowError("%s.PAL not found (RFF files may be wrong version)", PAL[i].name);
        palTable[PAL[i].id] = (RGB*)gSysRes.Lock(pPal);
        paletteSetColorTable(PAL[i].id, (uint8_t*)palTable[PAL[i].id]);
    }
    memcpy(palette, palTable[0], sizeof(palette));
    numshades = 64;
    paletteloaded |= PALETTE_MAIN;
    scrLoadPLUs();
    paletteloaded |= PALETTE_SHADE;
    initprintf("Loading translucency table\n");
    DICTNODE *pTrans = gSysRes.Lookup("TRANS", "TLU");
    if (!pTrans)
        ThrowError("TRANS.TLU not found");
    blendtable[0] = (char*)gSysRes.Lock(pTrans);
    paletteloaded |= PALETTE_TRANSLUC;

#ifdef USE_OPENGL
    for (auto & x : glblend)
        x = bloodglblend;
#endif

    initfastcolorlookup_palette(palette);
    palettePostLoadTables();
}

void scrSetPalette(int palId)
{
    curPalette = palId;
    scrSetGamma(0/*curGamma*/);
}

void scrSetGamma(int nGamma)
{
    dassert(nGamma < gGammaLevels);
    curGamma = nGamma;
    for (int i = 0; i < 256; i++)
    {
        baseDAC[i].red = gammaTable[curGamma][palTable[curPalette][i].red];
        baseDAC[i].green = gammaTable[curGamma][palTable[curPalette][i].green];
        baseDAC[i].blue = gammaTable[curGamma][palTable[curPalette][i].blue];
    }
    DacInvalid = 1;
}

void scrSetupFade(char red, char green, char blue)
{
    memcpy(fromDAC, curDAC, sizeof(fromDAC));
    toRGB.red = red;
    toRGB.green = green;
    toRGB.blue = blue;
}

void scrSetupUnfade(void)
{
    memcpy(fromDAC, baseDAC, sizeof(fromDAC));
}

void scrFadeAmount(int amount)
{
	for (int i = 0; i < 256; i++)
	{
		curDAC[i].red = interpolate(fromDAC[i].red, toRGB.red, amount);
        curDAC[i].green = interpolate(fromDAC[i].green, toRGB.green, amount);
        curDAC[i].blue = interpolate(fromDAC[i].blue, toRGB.blue, amount);
	}
	gSetDacRange(0, 256, curDAC);
}

void scrSetDac(void)
{
	if (DacInvalid)
		gSetDacRange(0, 256, baseDAC);
	DacInvalid = 0;
}

void scrInit(void)
{
    initprintf("Initializing engine\n");
#ifdef USE_OPENGL
    glrendmode = REND_POLYMOST;
#endif
    engineInit();
    curPalette = 0;
    curGamma = 0;
    initprintf("Loading gamma correction table\n");
    DICTNODE *pGamma = gSysRes.Lookup("gamma", "DAT");
    if (!pGamma)
        ThrowError("Gamma table not found");
    gGammaLevels = pGamma->size / 256;
    gammaTable = (char(*)[256])gSysRes.Lock(pGamma);
}

void scrUnInit(void)
{
    memset(palookup, 0, sizeof(palookup));
    memset(blendtable, 0, sizeof(blendtable));
    engineUnInit();
}


void scrSetGameMode(int vidMode, int XRes, int YRes, int nBits)
{
    videoResetMode();
    //videoSetGameMode(vidMode, XRes, YRes, nBits, 0);
    if (videoSetGameMode(vidMode, XRes, YRes, nBits, 0) < 0)
    {
        initprintf("Failure setting video mode %dx%dx%d %s! Trying next mode...\n", XRes, YRes,
                    nBits, vidMode ? "fullscreen" : "windowed");

        int resIdx = 0;

        for (int i=0; i < validmodecnt; i++)
        {
            if (validmode[i].xdim == XRes && validmode[i].ydim == YRes)
            {
                resIdx = i;
                break;
            }
        }

        int const savedIdx = resIdx;
        int bpp = nBits;

        while (videoSetGameMode(0, validmode[resIdx].xdim, validmode[resIdx].ydim, bpp, 0) < 0)
        {
            initprintf("Failure setting video mode %dx%dx%d windowed! Trying next mode...\n",
                        validmode[resIdx].xdim, validmode[resIdx].ydim, bpp);

            if (++resIdx == validmodecnt)
            {
                if (bpp == 8)
                    ThrowError("Fatal error: unable to set any video mode!");

                resIdx = savedIdx;
                bpp = 8;
            }
        }

        gSetup.xdim = validmode[resIdx].xdim;
        gSetup.ydim = validmode[resIdx].ydim;
        gSetup.bpp  = bpp;
    }
    videoClearViewableArea(0);
    scrNextPage();
    scrSetPalette(curPalette);
}

void scrNextPage(void)
{
    videoNextPage();
}
