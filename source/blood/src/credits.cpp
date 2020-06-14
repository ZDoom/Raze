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

#include "build.h"
#include "compat.h"
#include "SmackerDecoder.h"
#include "common_game.h"
#include "blood.h"
#include "config.h"
#include "controls.h"
#include "globals.h"
#include "resource.h"
#include "screen.h"
#include "sound.h"
#include "view.h"
#include "animtexture.h"
#include "../glbackend/glbackend.h"
#include "raze_sound.h"
#include "v_2ddrawer.h"

BEGIN_BLD_NS

char exitCredits = 0;

char Wait(int nTicks)
{
    totalclock = 0;
    while (totalclock < nTicks)
    {
        gameHandleEvents();
        auto key = inputState.keyGetScan();
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
    return 1;
}

char DoUnFade(int nTicks)
{
    return 1;
}

void credPlaySmk(const char* _pzSMK, const char* _pzWAV, int nWav);

void credLogosDos(void)
{
    char bShift = inputState.ShiftPressed();
    videoSetViewableArea(0, 0, xdim-1, ydim-1);
    DoUnFade(1);
    if (bShift)
        return;
    {
        if (fileSystem.FindFile("logo.smk"))
        {
            credPlaySmk("logo.smk", "logo.wav", -1);
        }
        else
        {
            twod->ClearScreen();
            rotatesprite(160<<16, 100<<16, 65536, 0, 2050, 0, 0, 0x4a, 0, 0, xdim-1, ydim-1);
            sndStartSample("THUNDER2", 128, -1);
            videoNextPage();
            if (!Wait(360))
                return;
            if (!DoFade(0, 0, 0, 60))
                return;
        }
        if (fileSystem.FindFile("gti.smk"))
        {
            credPlaySmk("gti.smk", "gt.wav", -1);
        }
        else
        {
            twod->ClearScreen();
            rotatesprite(160<<16, 100<<16, 65536, 0, 2052, 0, 0, 0x0a, 0, 0, xdim-1, ydim-1);
            videoNextPage();
            DoUnFade(1);
            sndStartSample("THUNDER2", 128, -1);
            if (!Wait(360))
                return;
        }
    }
    sndPlaySpecialMusicOrNothing(MUS_INTRO);
    sndStartSample("THUNDER2", 128, -1);
    if (!DoFade(0, 0, 0, 60))
        return;
    twod->ClearScreen();
    videoNextPage();
    if (!DoUnFade(1))
        return;
    twod->ClearScreen();
    rotatesprite(160<<16, 100<<16, 65536, 0, 2518, 0, 0, 0x4a, 0, 0, xdim-1, ydim-1);
    videoNextPage();
    Wait(360);
    //Mus_Fade(4000);
}

void credReset(void)
{
    twod->ClearScreen();
    videoNextPage();
    DoFade(0,0,0,1);
    DoUnFade(1);
}

bool credKOpen4Load(FString &pzFile)
{
    int nLen = strlen(pzFile);
    FixPathSeperator(pzFile);
    auto nHandle = fileSystem.FindFile(pzFile);
    if (nHandle < 0)
    {
        // Strip the drive letter and retry.
        if (nLen >= 3 && isalpha(pzFile[0]) && pzFile[1] == ':' && pzFile[2] == '/')
        {
            pzFile = pzFile.Mid(3);
            nHandle = fileSystem.FindFile(pzFile);
        }
    }
    return nHandle;
}

void credPlaySmk(const char *_pzSMK, const char *_pzWAV, int nWav)
{
    if (!_pzSMK || !*_pzSMK)
        return;
    FString pzSMK = _pzSMK;
    FString pzWAV = _pzWAV;
    auto nHandleSMK = credKOpen4Load(pzSMK);
    if (!nHandleSMK)
    {
        return;
    }
    SmackerHandle hSMK = Smacker_Open(pzSMK);
    if (!hSMK.isValid)
    {
        return;
    }
    uint32_t nWidth, nHeight;
    Smacker_GetFrameSize(hSMK, nWidth, nHeight);
    uint8_t palette[768];
    AnimTextures animtex;
    TArray<uint8_t> pFrame(nWidth * nHeight + std::max(nWidth, nHeight), true);
    animtex.SetSize(AnimTexture::Paletted, nWidth, nHeight);
    int nFrameRate = Smacker_GetFrameRate(hSMK);
    int nFrames = Smacker_GetNumFrames(hSMK);

    Smacker_GetPalette(hSMK, palette);
    Mus_Stop();

    int nScale;
    int nStat;

    if (nWidth <= 320 && nHeight <= 200)
    {
        if ((nWidth / (nHeight * 1.2f)) > (1.f * xdim / ydim))
            nScale = divscale16(320 * xdim * 3, nWidth * ydim * 4);
        else
            nScale = divscale16(200, nHeight);
        nStat = 2 | 8 | 64;
    }
    else
    {
        // DOS Blood v1.11: 320x240, 320x320, 640x400, and 640x480 SMKs all display 1:1 and centered in a 640x480 viewport
        nScale = tabledivide32(scale(65536, ydim << 2, xdim * 3), ((max(nHeight, 240 + 1u) + 239) / 240));
        nStat = 2 | 8 | 64 | 1024;
        renderSetAspect(viewingrange, 65536);
    }


    if (nWav > 0)
        sndStartWavID(nWav, 255);
    else
    {
        auto nHandleWAV = credKOpen4Load(pzWAV);
        if (nHandleWAV)
        {
            sndStartWavDisk(pzWAV, 255);
        }
    }

    UpdateDacs(0, true);

    gameHandleEvents();
    ClockTicks nStartTime = totalclock;

    inputState.ClearAllInput();
    
    int nFrame = 0;
    hw_int_useindexedcolortextures = false;
    do
    {
        gameHandleEvents();
        if (scale((int)(totalclock-nStartTime), nFrameRate, kTicRate) < nFrame)
            continue;

        if (inputState.CheckAllInput())
            break;

        twod->ClearScreen();
        Smacker_GetPalette(hSMK, palette);
        Smacker_GetFrame(hSMK, pFrame.Data());
        animtex.SetFrame(palette, pFrame.Data());
        rotatesprite_fs(160<<16, 100<<16, nScale, 0, -1, 0, 0, nStat, animtex.GetFrame());

        videoNextPage();

        nFrame++;
        Smacker_GetNextFrame(hSMK);
    } while(nFrame < nFrames);
    hw_int_useindexedcolortextures = hw_useindexedcolortextures;

    Smacker_Close(hSMK);
    inputState.ClearAllInput();
    soundEngine->StopAllChannels();
}

END_BLD_NS
