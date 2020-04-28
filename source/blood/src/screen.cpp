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
#include "ns.h"	// Must come before everything else!

#include <string.h>
#include "build.h"
#include "common_game.h"

#include "globals.h"
#include "config.h"
#include "resource.h"
#include "screen.h"
#include "palettecontainer.h"
#include "rendering/v_video.h"

BEGIN_BLD_NS

const char * PLU[15] = {
    "NORMAL.PLU",
    "SATURATE.PLU",
    "BEAST.PLU",
    "TOMMY.PLU",
    "SPIDER3.PLU",
    "GRAY.PLU",
    "GRAYISH.PLU",
    "SPIDER1.PLU",
    "SPIDER2.PLU",
    "FLAME.PLU",
    "COLD.PLU",
    "P1.PLU",
    "P2.PLU",
    "P3.PLU",
    "P4.PLU"
};

const char *PAL[5] = {
    "BLOOD.PAL", 
    "WATER.PAL",
    "BEAST.PAL",
    "SEWER.PAL",
    "INVULN1.PAL"
};


static RGB toRGB;
static RGB *palTable[5];
static int curPalette;
bool gFogMode = false;

void scrLoadPLUs(void)
{
    // load lookup tables
    for (int i = 0; i < MAXPALOOKUPS; i++) 
    {
        int lump = i < 15 ? fileSystem.FindFile(PLU[i]) : fileSystem.FindResource(i, "PLU");
        if (lump < 0)
        {
            if (i < 15) I_FatalError("%s.PLU not found", PLU[i]);
            else continue;
        }
        auto data = fileSystem.GetFileData(lump);
        if (data.Size() != 64 * 256)
            I_FatalError("Incorrect PLU size");
        paletteSetLookupTable(i, data.Data());
    }

    palookupfog[1].r = 255;
    palookupfog[1].g = 255;
    palookupfog[1].b = 255;
	palookupfog[1].f = 1;
}

glblend_t const bloodglblend =
{
    {
        { 1.f/3.f, STYLEALPHA_Src, STYLEALPHA_InvSrc, 0 },
        { 2.f/3.f, STYLEALPHA_Src, STYLEALPHA_InvSrc, 0 },
    },
};

void scrLoadPalette(void)
{
	for (auto& x : glblend)
		x = bloodglblend;

    GPalette.Init(MAXPALOOKUPS + 1);    // one slot for each translation, plus a separate one for the base palettes.
    paletteloaded = 0;
    Printf("Loading palettes\n");
    for (int i = 0; i < 5; i++)
    {
        auto pal = fileSystem.LoadFile(PAL[i]);
        if (pal.Size() < 768) I_FatalError("%s: file too small", PAL[i]);
        paletteSetColorTable(i, pal.Data(), false, false);
    }
    numshades = 64;
    paletteloaded |= PALETTE_MAIN;
    scrLoadPLUs();
    paletteloaded |= PALETTE_SHADE;
    Printf("Loading translucency table\n");
    DICTNODE *pTrans = gSysRes.Lookup("TRANS", "TLU");
    if (!pTrans)
        ThrowError("TRANS.TLU not found");
    paletteloaded |= PALETTE_TRANSLUC;

    palettePostLoadTables();

}

void scrSetPalette(int palId)
{
    curPalette = palId;
}

void scrInit(void)
{
    glrendmode = REND_POLYMOST;
    engineInit();
    curPalette = 0;
}

void scrSetGameMode(int vidMode, int XRes, int YRes, int nBits)
{
    videoInit();
    videoClearViewableArea(0);
    videoNextPage();
    scrSetPalette(curPalette);
}

END_BLD_NS
