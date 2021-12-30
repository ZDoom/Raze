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
#include "engine.h"
#include "exhumed.h"
#include "view.h"
#include "aistuff.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

BEGIN_PS_NS

enum { kMaxGrads = 12 };

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

palette_t *fadedestpal;
palette_t *fadecurpal;
int nPalDelay;
int nPalDiff;
int bGreenPal = 0;

// keep a local copy of the palette that would have been sent to the VGA display adapter
uint8_t vgaPalette[768];

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
        lookups.setTable(i, buffer);

        bGreenPal = 0;

        // These 3 tables do not have normal gradients. The others work without adjustment.
        // Other changes than altering the fog gradient are not necessary.
        lookups.tables[kPalTorch].ShadeFactor = lookups.tables[kPalTorch2].ShadeFactor = (numshades - 2) / 20.f;
        lookups.tables[kPalNoTorch].ShadeFactor = lookups.tables[kPalNoTorch2].ShadeFactor = (numshades - 2) / 4.f;
        lookups.tables[kPalBrite].ShadeFactor = lookups.tables[kPalBrite2].ShadeFactor = (numshades - 2) / 128.f;
        lookups.setFadeColor(kPalRedBrite, 255, 0, 0);

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

void GrabPalette()
{
    nPalDiff  = 0;
    nPalDelay = 0;

    btint = 0;
    gtint = 0;
    rtint = 0;
    videoTintBlood(0, 0, 0);
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

    videoTintBlood(rtint, gtint, btint);
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
    if (abs(r) > abs(g)) {
        nVal = abs(r);
    }
    else {
        nVal = abs(g);
    }

    if (nVal < abs(b)) {
        nVal = abs(b);
    }

    nPalDiff += nVal;

    videoTintBlood(rtint, gtint, btint);

    nPalDelay = 0;
}


END_PS_NS
