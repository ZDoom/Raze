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
#include "light.h"
#include "engine.h"
#include "exhumed.h"
#include "view.h"
#include "cd.h"
#include "lighting.h"
#include "../glbackend/glbackend.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

BEGIN_PS_NS

#define kMaxGrads	12

const char *GradList[kMaxGrads] = {
    "normal.rmp",
    "nodim.rmp",
    "torch.rmp",
    "notorch.rmp",
    "brite.rmp",
    "redbrite.rmp",
    "grnbrite.rmp",
    "normal.rmp",
    "nodim.rmp",
    "torch.rmp",
    "notorch.rmp",
    "brite.rmp"
};

int rtint = 0;
int gtint = 0;
int btint = 0;
//char *origpalookup[kMaxPalookups];
//unsigned char curpal[768];
//unsigned char kenpal[768];
palette_t *fadedestpal;
palette_t *fadecurpal;
short nPalDelay;
short nPalDiff;
short overscanindex;
int bGreenPal = 0;

// keep a local copy of the palette that would have been sent to the VGA display adapter
uint8_t vgaPalette[768];


void MyLoadPalette()
{
    //int hFile = kopen4load("PALETTE.DAT", 1);
    //if (hFile == -1)
    //{
    //    Printf("Error reading palette 'PALETTE.DAT'\n");
    //    return;
    //}
    //
    //kread(hFile, kenpal, sizeof(kenpal));
    //kclose(hFile);
    videoSetPalette(0, BASEPAL, 0);
    SetOverscan(BASEPAL);
}

int LoadPaletteLookups()
{
    uint8_t buffer[256*64];
    numshades = 64;

    for (int i = 0; i < kMaxGrads; i++)
    {
        auto hFile = fileSystem.OpenFileReader(GradList[i]);
        if (!hFile.isOpen())
        {
            Printf("Error reading palette lookup '%s'\n", GradList[i]);
            return 0;
        }

        hFile.Read(buffer, 256*64);
        // TODO: dumb hack
        if (lookuptables[i])
            ALIGNED_FREE_AND_NULL(lookuptables[i]);
        paletteSetLookupTable(i, buffer);
        
        bGreenPal = 0;

#ifdef USE_OPENGL
        // These 3 tables do not have normal gradients. The others work without adjustment.
        // Other changes than altering the fog gradient are not necessary.
        shadediv[kPalTorch] = shadediv[kPalTorch2] = 1 / 20.f;
        shadediv[kPalNoTorch] = shadediv[kPalNoTorch2] = 0.25f;
        shadediv[kPalBrite] = shadediv[kPalBrite] = 1 / 128.f;
#endif

    }

    return 1;
}

void SetGreenPal()
{
    bGreenPal = 1;
}

void RestoreGreenPal()
{
    bGreenPal = 0;
}

int HavePLURemap()
{
    return bGreenPal || bTorch;
}

uint8_t RemapPLU(uint8_t pal)
{
    if (bGreenPal)
    {
        if (pal != kPalRedBrite)
            pal = kPalGreenBrite;
    }
    else if (bTorch)
    {
        switch (pal)
        {
        case kPalTorch:
            pal = kPalNoTorch;
            break;
        case kPalNoTorch:
            pal = kPalTorch;
            break;
        case kPalTorch2:
            pal = kPalNoTorch2;
            break;
        case kPalNoTorch2:
            pal = kPalTorch2;
            break;
        }
    }
    return pal;
}

void WaitVBL()
{
#ifdef __WATCOMC__
    while (!(inp(0x3da) & 8));
#endif
}


void GrabPalette()
{
    SetOverscan(BASEPAL);

    videoSetPalette(0, BASEPAL, 0);

    nPalDiff  = 0;
    nPalDelay = 0;

    btint = 0;
    gtint = 0;
    rtint = 0;
    videoTintBlood(0, 0, 0);
}

void BlackOut()
{
    g_lastpalettesum = -1;
    videoTintBlood(0, 0, 0);
}

void RestorePalette()
{
    videoSetPalette(0, BASEPAL, 0);
    videoTintBlood(0, 0, 0);
}

void WaitTicks(int nTicks)
{
    if (htimer)
    {
        nTicks += (int)totalclock;
        while (nTicks > (int)totalclock) { HandleAsync(); }
    }
    else
    {
        while (nTicks > 0) {
            nTicks--;
            WaitVBL();
        }
    }
}

// unused
void DoFadeToRed()
{
    // fixme
    videoTintBlood(-255, -255, -255);
    videoNextPage();
}

void FadeToWhite()
{
    // fixme
    videoTintBlood(255, 255, 255);
    videoNextPage();
}

void FadeOut(int bFadeMusic)
{
    if (bFadeMusic) {
        StartfadeCDaudio();
    }


    videoTintBlood(-255, -255, -255);
    videoNextPage();

    if (bFadeMusic) {
        while (StepFadeCDaudio() != 0) {}
    }

    EraseScreen(overscanindex);
}

void StartFadeIn()
{
    //fadecurpal = curpal;
}

int DoFadeIn()
{
    paletteSetColorTable(curbasepal, basepaltable[BASEPAL]);
    videoSetPalette(0, curbasepal, 0);
    videoNextPage();
    return 0;
}

void FadeIn()
{
    videoSetPalette(0, BASEPAL, 0);
    videoNextPage();

}

void FixPalette()
{
    if (!nPalDiff) {
        return;
    }

    if (nPalDelay > 0)
    {
        nPalDelay--;
        return;
    }

    nPalDelay = 5;

    nPalDiff -= 20;
    gtint -= 20;
    rtint -= 20;
    btint -= 20;

    if (gtint < 0) {
        gtint = 0;
    }

    if (rtint < 0) {
        rtint = 0;
    }

    if (btint < 0) {
        btint = 0;
    }

    if (nPalDiff < 0) {
        nPalDiff = 0;
    }

#ifdef USE_OPENGL
    if (videoGetRenderMode() >= REND_POLYMOST) videoTintBlood(rtint, gtint, btint);
    else
#endif
    {
        
        //videoUpdatePalette(0, 256);
        g_lastpalettesum = -1;
    }
}

void TintPalette(int r, int g, int b)
{
    if (bCamera) {
        return;
    }

    // range limit R between 20 and 255 if positive
    if (r > 255)
    {
        r = 255;
    }
    else
    {
        if (r && r < 20) {
            r = 20;
        }
    }

    // range limit G between 20 and 255 if positive
    if (g > 255)
    {
        g = 255;
    }
    else
    {
        if (g && g < 20) {
            g = 20;
        }
    }

    // range limit B between 20 and 255 if positive
    if (b > 255)
    {
        b = 255;
    }
    else
    {
        if (b && b < 20) {
            b = 20;
        }
    }

    // loc_17EFA
    if (g && gtint > 32) {
        return;
    }

    gtint += g;

    if (r && rtint > 256) {
        return;
    }

    rtint += r;

    btint += b;

    // do not modify r, g or b variables from this point on
    int nVal;

    // loc_17F49
    if (klabs(r) > klabs(g)) {
        nVal = klabs(r);
    }
    else {
        nVal = klabs(g);
    }

    if (nVal < klabs(b)) {
        nVal = klabs(b);
    }

    nPalDiff += nVal;

    videoTintBlood(rtint, gtint, btint);

    nPalDelay = 0;
}

void DoOverscanSet(short someval)
{
}

// unused
void SetWhiteOverscan()
{

}

void SetOverscan(int id)
{
}
END_PS_NS
