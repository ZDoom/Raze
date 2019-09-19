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
#include "build.h"
#include "compat.h"
#include "SmackerDecoder.h"
#include "fx_man.h"
#include "keyboard.h"
#include "common_game.h"
#include "blood.h"
#include "config.h"
#include "controls.h"
#include "globals.h"
#include "resource.h"
#include "screen.h"
#include "sound.h"
#include "view.h"

char exitCredits = 0;

char Wait(int nTicks)
{
    gGameClock = 0;
    while (gGameClock < nTicks)
    {
        timerUpdate();
        char key = keyGetScan();
        if (key)
        {
            if (key == sc_Escape) // sc_Escape
                exitCredits = 1;
            return 0;
        }
    }
    return 1;
}

char DoFade(char r, char g, char b, int nTicks)
{
    dassert(nTicks > 0);
    scrSetupFade(r, g, b);
    gGameClock = gFrameClock = 0;
    do
    {
        while (gGameClock < gFrameClock) { timerUpdate();};
        gFrameClock += 2;
        scrNextPage();
        scrFadeAmount(divscale16(ClipHigh(gGameClock, nTicks), nTicks));
        if (keyGetScan())
            return 0;
    } while (gGameClock <= nTicks);
    return 1;
}

char DoUnFade(int nTicks)
{
    dassert(nTicks > 0);
    scrSetupUnfade();
    gGameClock = gFrameClock = 0;
    do
    {
        while (gGameClock < gFrameClock) { timerUpdate(); };
        scrNextPage();
        scrFadeAmount(0x10000-divscale16(ClipHigh(gGameClock, nTicks), nTicks));
        if (keyGetScan())
            return 0;
    } while (gGameClock <= nTicks);
    return 1;
}

void credLogosDos(void)
{
    char bShift = keystatus[sc_LeftShift] | keystatus[sc_RightShift];
    videoSetViewableArea(0, 0, xdim-1, ydim-1);
    DoUnFade(1);
    videoClearScreen(0);
    if (bShift)
        return;
    {
        //CSMKPlayer smkPlayer;
        //if (smkPlayer.PlaySMKWithWAV("LOGO.SMK", 300) == 1)
        //{
            rotatesprite(160<<16, 100<<16, 65536, 0, 2050, 0, 0, 0x4a, 0, 0, xdim-1, ydim-1);
            sndStartSample("THUNDER2", 128, -1);
            scrNextPage();
            if (!Wait(360))
                return;
            if (!DoFade(0, 0, 0, 60))
                return;
        //}
        //if (smkPlayer.PlaySMKWithWAV("GTI.SMK", 301) == 1)
        //{
            videoClearScreen(0);
            rotatesprite(160<<16, 100<<16, 65536, 0, 2052, 0, 0, 0x0a, 0, 0, xdim-1, ydim-1);
            scrNextPage();
            DoUnFade(1);
            sndStartSample("THUNDER2", 128, -1);
            if (!Wait(360))
                return;
        //}
    }
    sndPlaySpecialMusicOrNothing(MUS_INTRO);
    sndStartSample("THUNDER2", 128, -1);
    if (!DoFade(0, 0, 0, 60))
        return;
    videoClearScreen(0);
    scrNextPage();
    if (!DoUnFade(1))
        return;
    videoClearScreen(0);
    rotatesprite(160<<16, 100<<16, 65536, 0, 2518, 0, 0, 0x4a, 0, 0, xdim-1, ydim-1);
    scrNextPage();
    Wait(360);
    sndFadeSong(4000);
}

void credReset(void)
{
    videoClearScreen(0);
    scrNextPage();
    DoFade(0,0,0,1);
    scrSetupUnfade();
    DoUnFade(1);
}

int credKOpen4Load(char *&pzFile)
{
    int nLen = strlen(pzFile);
    for (int i = 0; i < nLen; i++)
    {
        if (pzFile[i] == '\\')
            pzFile[i] = '/';
    }
    int nHandle = kopen4loadfrommod(pzFile, 0);
    if (nHandle == -1)
    {
        // Hack
        if (nLen >= 3 && isalpha(pzFile[0]) && pzFile[1] == ':' && pzFile[2] == '/')
        {
            pzFile += 3;
            nHandle = kopen4loadfrommod(pzFile, 0);
        }
    }
    return nHandle;
}

#define kSMKPal 5
#define kSMKTile (MAXTILES-1)

void credPlaySmk(const char *_pzSMK, const char *_pzWAV, int nWav)
{
#if 0
    CSMKPlayer smkPlayer;
    if (dword_148E14 >= 0)
    {
        if (toupper(*pzSMK) == 'A'+dword_148E14)
        {
            if (Redbook.sub_82258() == 0 || Redbook.sub_82258() > 20)
                return;
        }
        Redbook.sub_82554();
    }
    smkPlayer.sub_82E6C(pzSMK, pzWAV);
#endif
    if (Bstrlen(_pzSMK) == 0)
        return;
    char *pzSMK = Xstrdup(_pzSMK);
    char *pzWAV = Xstrdup(_pzWAV);
    char *pzSMK_ = pzSMK;
    char *pzWAV_ = pzWAV;
    int nHandleSMK = credKOpen4Load(pzSMK);
    if (nHandleSMK == -1)
    {
        Bfree(pzSMK_);
        Bfree(pzWAV_);
        return;
    }
    kclose(nHandleSMK);
    SmackerHandle hSMK = Smacker_Open(pzSMK);
    if (!hSMK.isValid)
    {
        Bfree(pzSMK_);
        Bfree(pzWAV_);
        return;
    }
    uint32_t nWidth, nHeight;
    Smacker_GetFrameSize(hSMK, nWidth, nHeight);
    uint8_t palette[768];
    uint8_t *pFrame = (uint8_t*)Xmalloc(nWidth*nHeight);
    waloff[kSMKTile] = (intptr_t)pFrame;
    tilesiz[kSMKTile].y = nWidth;
    tilesiz[kSMKTile].x = nHeight;
    if (!pFrame)
    {
        Smacker_Close(hSMK);
        Bfree(pzSMK_);
        Bfree(pzWAV_);
        return;
    }
    int nFrameRate = Smacker_GetFrameRate(hSMK);
    int nFrames = Smacker_GetNumFrames(hSMK);

    Smacker_GetPalette(hSMK, palette);
    paletteSetColorTable(kSMKPal, palette);
    videoSetPalette(gBrightness>>2, kSMKPal, 8+2);

    int nScale;

    if ((nWidth / (nHeight * 1.2f)) > (1.f * xdim / ydim))
        nScale = divscale16(320 * xdim * 3, nWidth * ydim * 4);
    else
        nScale = divscale16(200, nHeight);

    if (nWav)
        sndStartWavID(nWav, FXVolume);
    else
    {
        int nHandleWAV = credKOpen4Load(pzWAV);
        if (nHandleWAV != -1)
        {
            kclose(nHandleWAV);
            sndStartWavDisk(pzWAV, FXVolume);
        }
    }

    UpdateDacs(0, true);

    timerUpdate();
    int32_t nStartTime = totalclock;

    ctrlClearAllInput();
    
    int nFrame = 0;
    do
    {
        G_HandleAsync();
        if (scale(totalclock-nStartTime, nFrameRate, kTicRate) < nFrame)
            continue;

        if (ctrlCheckAllInput())
            break;

        videoClearScreen(0);
        Smacker_GetPalette(hSMK, palette);
        paletteSetColorTable(kSMKPal, palette);
        videoSetPalette(gBrightness >> 2, kSMKPal, 0);
        tileInvalidate(kSMKTile, 0, 1 << 4);  // JBF 20031228
        Smacker_GetFrame(hSMK, pFrame);
        rotatesprite_fs(160<<16, 100<<16, nScale, 512, kSMKTile, 0, 0, 2|4|8|64);

        videoNextPage();

        ctrlClearAllInput();
        nFrame++;
        Smacker_GetNextFrame(hSMK);
    } while(nFrame < nFrames);

    Smacker_Close(hSMK);
    ctrlClearAllInput();
    FX_StopAllSounds();
    videoSetPalette(gBrightness >> 2, 0, 8+2);
    Bfree(pFrame);
    Bfree(pzSMK_);
    Bfree(pzWAV_);
}
