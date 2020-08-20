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
#include <cstdio>
#include <cstring>
#include "baselayer.h"
#include "c_bind.h"
#include "sound.h"
#include "v_2ddrawer.h"
#include "animtexture.h"
#include "s_music.h"

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

uint8_t lh[32] = { 0 };

static uint8_t* CurFrame = NULL;

bool bServedSample = false;
palette_t moviepal[256];
const int numAudioBlocks = 20;
int sfxnum = -1;

struct AudioData
{
    int16_t samples[2205 * numAudioBlocks]; // must be a multiple of the stream buffer size
    int nWrite;
    int nRead;
};

SoundStream* stream;
AudioData audio;
bool bAudioStarted;

SoundHandle shandle;

int ReadFrame(FileReader &fp, uint8_t *palette)
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
                fp.Read(palette, 768);
                fp.Read(&var_1C, sizeof(var_1C));

                for (unsigned i = 0; i < 768;i++)
                    palette[i] <<= 2;

                memset(CurFrame, overscanindex, 4); //sizeof(CurFrame));
                continue;
            }
            case kFrameSound:
            {
                auto sfxx = soundEngine->GetSounds();
                auto buffer = fp.Read(nSize);
                assert(buffer.Size() == 2205);
                auto wbuffer = audio.samples + audio.nWrite * 2205;
                for (int i = 0; i < 2205; i++)
                {
                    wbuffer[i] = (buffer[i] - 128) << 8;
                }
                audio.nWrite++;
                if (audio.nWrite >= numAudioBlocks) audio.nWrite = 0;
                continue;
            }
            case kFrameImage:
            {
                //Printf("Reading image block size %d...\n", nSize);
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

#if 0
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
#endif

static bool StreamCallbackFunc(SoundStream* stream, void* buff, int len, void* userdata)
{
    memcpy(buff, &audio.samples[audio.nRead], len);
    audio.nRead += len / 2;
    if (audio.nRead >= countof(audio.samples)) audio.nRead = 0;
    return true;
}


void PlayMovie(const char* fileName)
{
    uint8_t palette[768];
    TArray<uint8_t> f(64000, true);
    CurFrame = f.Data();

    int bDoFade = true;
	auto fp = fileSystem.OpenFileReader(fileName);
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

    bAudioStarted = false;
    if (bDoFade) {
        StartFadeIn();
    }
    memset(audio.samples, 0, sizeof(audio.samples));
    audio.nWrite = 5; // play 5 blocks (i.e. half a second) of silence to get ahead of the stream. For this video it isn't necessary to sync it perfectly.

    double angle = 1536;
    double z = 0;

    AnimTextures animtex;
    animtex.SetSize(AnimTexture::Paletted, 200, 320);

    auto sfxx = soundEngine->GetSounds();

    if (!bAudioStarted)
    {
        // start audio playback
        stream = S_CreateCustomStream(4410, 22050, 1, StreamCallbackFunc, nullptr);
        bAudioStarted = true;
    }

    // Read a frame in first
    if (ReadFrame(fp, palette))
    {
        int fn = 0;
        int ototalclock = totalclock + 12;
        int lastclock = totalclock;
        while (true)// !inputState.keyBufferWaiting())
        {
            HandleAsync();

            if (z < 65536) { // Zoom - normal zoom is 65536.
                z += 2048 * (totalclock - lastclock) / 8.;
            }
            if (z > 65536) z = 65536;
            if (angle != 0) {
                angle += 16. * (totalclock-lastclock) / 8. ;
                if (angle >= 2048) {
                    angle = 0;
                }
            }
            lastclock = totalclock;

            // I have no idea why this needs double buffering now.
            fn ^= 1;
            animtex.SetFrame(palette, CurFrame);


            rotatesprite(160 << 16, 100 << 16, int(z), int(angle+512), -1, 0, 1, RS_AUTO | RS_YFLIP, 0, 0, xdim - 1, ydim - 1, animtex.GetFrame());

#if 0
            if (bDoFade) {
                bDoFade = DoFadeIn();
            }
#endif

            videoNextPage();

            if (totalclock >= ototalclock)
            {
                ototalclock += 12;
                if (ReadFrame(fp, palette) == 0) {
                    break;
                }
            }
        }
    }

    S_StopCustomStream(stream);
    if (inputState.keyBufferWaiting()) {
        inputState.keyGetChar();
    }
}

END_PS_NS
