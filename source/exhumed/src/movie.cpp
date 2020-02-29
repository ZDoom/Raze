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
#include "names.h"
#include "movie.h"
#include "light.h"
#include <cstdio>
#include <cstring>
#include "baselayer.h"
#include "typedefs.h"
#include "c_bind.h"
#include "sound.h"
#include "v_2ddrawer.h"

BEGIN_PS_NS

enum {
    kFramePalette = 0,
    kFrameSound,
    kFrameImage,
    kFrameDone
};

#define kSampleRate     22050
#define kSampleSize     2205

uint8_t bankbuf[kSampleRate];
uint32_t bankptr = 0;
uint32_t banktail = 0;

uint32_t lSoundBytesRead = 0;
uint32_t lSoundBytesUsed = 0;

uint8_t lh[32] = { 0 };

static uint8_t* CurFrame = NULL;

bool bServedSample = false;
palette_t moviepal[256];

int ReadFrame(FileReader &fp)
{
    static int nFrame = 0;
    Printf("Reading frame %d...\n", nFrame);
    nFrame++;

    uint8_t nType;
    uint8_t var_1C;
    int nSize;
    uint16_t yOffset;
    uint8_t xOffset;
    uint8_t nPixels;
    uint8_t palette[768];

    while (1)
    {
        if (fp.Read(&nType, sizeof(nType)) == 0) {
            return 0;
        }

		fp.Read(&nSize, sizeof(nSize));

        nType--;
        if (nType > 3) {
            continue;
        }

        switch (nType)
        {
            case kFramePalette:
            {
                fp.Read(palette, sizeof(palette));
                fp.Read(&var_1C, sizeof(var_1C));

                for (auto &c : palette)
                    c <<= 2;

                paletteSetColorTable(ANIMPAL, palette);
                videoSetPalette(0, ANIMPAL, Pal_Fullscreen);

                memset(CurFrame, overscanindex, 4); //sizeof(CurFrame));
                continue;
            }
            case kFrameSound:
            {
                Printf("Reading sound block size %d...\n", nSize);

                if (lSoundBytesRead - lSoundBytesUsed >= kSampleRate)
                {
                    DebugOut("SOUND BUF FULL!\n");
                fp.Seek(nSize, FileReader::SeekCur);
                }
                else
                {
                    //mutex_lock(&mutex);

                    int nRead = fp.Read((char*)bankbuf + bankptr, nSize);

                    lSoundBytesRead += nRead;
                    bankptr += nSize;

                    assert(nSize == nRead);
                    assert(bankptr <= kSampleRate);

                    if (bankptr >= kSampleRate) {
                        bankptr -= kSampleRate; // loop back to start
                    }

                    //mutex_unlock(&mutex);
                }

                continue;
            }
            case kFrameImage:
            {
                Printf("Reading image block size %d...\n", nSize);
                if (nSize == 0) {
                    continue;
                }

                uint8_t *pFrame = CurFrame;

                int nRead = fp.Read(&yOffset, sizeof(yOffset));
                nSize -= nRead;

                pFrame += yOffset * 200; // row position

                while (nSize > 0)
                {
                    fp.Read(&xOffset, sizeof(xOffset));
                    fp.Read(&nPixels, sizeof(nPixels));
                    nSize -= 2;

                    pFrame += xOffset;

                    if (nPixels)
                    {
                        int nRead = fp.Read(pFrame, nPixels);
                        pFrame += nRead;
                        nSize -= nRead;
                    }
                }

                tileInvalidate(kMovieTile, -1, -1);
                break;
            }
            case kFrameDone:
            {
                return 1;
                break;
            }
        }
    }
}

static void ServeSample(const char** ptr, uint32_t* length)
{
    //mutex_lock(&mutex);

    *ptr = (char*)bankbuf + banktail;
    *length = kSampleSize;

    banktail += kSampleSize;
    if (banktail >= kSampleRate) {
        banktail -= kSampleRate; // rotate back to start
    }

    lSoundBytesUsed += kSampleSize;
    bServedSample = true;

    //mutex_unlock(&mutex);
}

void PlayMovie(const char* fileName)
{
    CurFrame = TileFiles.tileCreate(kMovieTile, 320, 200);
    if (CurFrame == nullptr) return;

    int bDoFade = kTrue;
    int hFx = -1;
	auto fp = fileSystem.OpenFileReader(fileName, 0);
	if (!fp.isOpen())
	{
		Printf("Unable to open %s\n", fileName);
		return;
	}

    fp.Read(lh, sizeof(lh));

    // sound stuff
    bankptr = 0;
    banktail = 0;

    // clear keys
    inputState.ClearAllInput();

    if (bDoFade) {
        StartFadeIn();
    }

    int angle = 1536;
    int z = 0;

    videoSetPalette(0, ANIMPAL, Pal_Fullscreen);

    // Read a frame in first
    if (ReadFrame(fp))
    {
        // start audio playback (fixme)
#if 0
        hFx = FX_StartDemandFeedPlayback(ServeSample, kSampleRate, 0, snd_fxvolume, snd_fxvolume, snd_fxvolume, FX_MUSIC_PRIORITY, fix16_one, -1);
#else
        hFx = -1;
#endif

        while (!inputState.keyBufferWaiting())
        {
            HandleAsync();

            // audio is king for sync - if the backend doesn't need any more samples yet, 
            // don't process any more movie file data.
            if (!bServedSample && hFx > 0) {
                continue;
            }

            bServedSample = false;

            if (z < 65536) { // Zoom - normal zoom is 65536.
                z += 2048;
            }
            if (angle != 0) {
                angle += 16;
                if (angle == 2048) {
                    angle = 0;
                }
            }

            twod->ClearScreen();
            rotatesprite(160 << 16, 100 << 16, z, angle, kMovieTile, 0, 1, 2, 0, 0, xdim - 1, ydim - 1);

            if (bDoFade) {
                bDoFade = DoFadeIn();
            }

            videoNextPage();

            if (ReadFrame(fp) == 0) {
                break;
            }
        }
    }

    if (hFx > 0) {
        //FX_StopSound(hFx);
    }

    if (inputState.keyBufferWaiting()) {
        inputState.keyGetChar();
    }
}
END_PS_NS
