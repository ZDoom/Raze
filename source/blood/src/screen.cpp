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
#include "rendering/v_video.h"

BEGIN_BLD_NS

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


static RGB toRGB;
static RGB *palTable[5];
static int curPalette;
bool gFogMode = false;

void scrResetPalette(void)
{
    paletteSetColorTable(0, (uint8_t*)palTable[0]);
}

void scrLoadPLUs(void)
{
    // load default palookups
    for (int i = 0; i < 15; i++) {
        DICTNODE *pPlu = gSysRes.Lookup(PLU[i].name, "PLU");
        if (!pPlu)
            ThrowError("%s.PLU not found", PLU[i].name);
        if (pPlu->Size() / 256 != 64)
            ThrowError("Incorrect PLU size");
        palookup[PLU[i].id] = (char*)gSysRes.Lock(pPlu);
    }

    // by NoOne: load user palookups
    for (int i = kUserPLUStart; i < MAXPALOOKUPS; i++) {
        DICTNODE* pPlu = gSysRes.Lookup(i, "PLU");
        if (!pPlu) continue;
        else if (pPlu->Size() / 256 != 64) { consoleSysMsg("Incorrect filesize of PLU#%d", i); }
        else palookup[i] = (char*)gSysRes.Lock(pPlu);
    }

#ifdef USE_OPENGL
    palookupfog[1].r = 255;
    palookupfog[1].g = 255;
    palookupfog[1].b = 255;
	palookupfog[1].f = 1;
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
#ifdef USE_OPENGL
	for (auto& x : glblend)
		x = bloodglblend;
#endif

    paletteloaded = 0;
    Printf("Loading palettes\n");
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
    V_Init2();
    videoClearViewableArea(0);
    videoNextPage();
    scrSetPalette(curPalette);
}

END_BLD_NS
