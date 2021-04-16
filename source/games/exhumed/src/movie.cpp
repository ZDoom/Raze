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
#include "c_bind.h"
#include "sound.h"
#include "v_2ddrawer.h"
#include "animtexture.h"
#include "s_music.h"
#include "screenjob.h"
#include "v_draw.h"

BEGIN_PS_NS


class LMFPlayer
{
    enum
    {
        kFramePalette = 0,
        kFrameSound,
        kFrameImage,
        kFrameDone
    };

    enum
    {
        kSampleRate = 22050,
        kSampleSize = 2205,
        numAudioBlocks = 20
    };

    struct AudioData
    {
        int16_t samples[kSampleSize * numAudioBlocks]; // must be a multiple of the stream buffer size
        int nWrite;
        int nRead;
    };

    uint8_t palette[768];
    uint8_t CurFrame[64000];

    SoundStream* stream = nullptr;
    AudioData audio{};
    AnimTextures animtex;

    int nFrame = 0;

public:
    int ReadFrame(FileReader& fp)
    {
        nFrame++;

        uint8_t nType;
        uint8_t var_1C;
        int nSize;
        uint16_t yOffset;
        uint8_t xOffset;
        uint8_t nPixels;

        while (1)
        {
            if (fp.Read(&nType, sizeof(nType)) == 0)
            {
                return 0;
            }

            fp.Read(&nSize, sizeof(nSize));

            nType--;
            if (nType > 3)
            {
                continue;
            }

            switch (nType)
            {
            case kFramePalette:
            {
                fp.Read(palette, 768);
                fp.Read(&var_1C, sizeof(var_1C));

                for (unsigned i = 0; i < 768; i++)
                    palette[i] <<= 2;

                memset(CurFrame, 0, 4); //sizeof(CurFrame));
                continue;
            }
            case kFrameSound:
            {
                auto sfxx = soundEngine->GetSounds();
                auto buffer = fp.Read(nSize);
                assert(buffer.Size() == kSampleSize);
                auto wbuffer = audio.samples + audio.nWrite * kSampleSize;
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
                if (nSize == 0)
                {
                    continue;
                }

                uint8_t* pFrame = CurFrame;

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
                animtex.SetFrame(palette, CurFrame);

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


    static bool StreamCallbackFunc(SoundStream* stream, void* buff, int len, void* userdata)
    {
        LMFPlayer* pId = (LMFPlayer*)userdata;
        memcpy(buff, &pId->audio.samples[pId->audio.nRead], len);
        pId->audio.nRead += len / 2;
        if (pId->audio.nRead >= countof(pId->audio.samples)) pId->audio.nRead = 0;
        return true;
    }

    void Open(FileReader& fp)
    {

        uint8_t header[32];
        fp.Read(header, sizeof(header));
        audio.nWrite = 5; // play 5 blocks (i.e. half a second) of silence to get ahead of the stream. For this video it isn't necessary to sync it perfectly.

        // start audio playback
        stream = S_CreateCustomStream(kSampleSize * 2, kSampleRate, 1, StreamCallbackFunc, this); // size must be doubled here or dropouts can be heard.
        animtex.SetSize(AnimTexture::Paletted, 200, 320);
    }

    void Close()
    {
        S_StopCustomStream(stream);
    }

    AnimTextures& animTex()
    {
        return animtex;
    }
};

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

class DLmfPlayer : public DSkippableScreenJob
{
    LMFPlayer decoder;
    double angle = 1536;
    double z = 0;
    uint64_t nextclock = 0, lastclock = 0;
    FileReader fp;

public:

    DLmfPlayer(FileReader& fr)
    {
        decoder.Open(fr);
        lastclock = 0;
        nextclock = 0;
        fp = std::move(fr);
        pausable = false;
    }

    //---------------------------------------------------------------------------
    //
    // 
    //
    //---------------------------------------------------------------------------

    void Draw(double smoothratio) override
    {
        uint64_t clock = (ticks + smoothratio) * 1'000'000'000. / GameTicRate;
        if (clock >= nextclock)
        {
            nextclock += 100'000'000;
            if (decoder.ReadFrame(fp) == 0)
            {
                state = finished;
                return;
            }
        }

        double duration = (clock - lastclock) * double(120. / 8'000'000'000);
        if (z < 65536) { // Zoom - normal zoom is 65536.
            z += 2048 * duration;
        }
        if (z > 65536) z = 65536;
        if (angle != 0) {
            angle += 16. * duration;
            if (angle >= 2048) {
                angle = 0;
            }
        }
        assert(z < 65536);

        {
            twod->ClearScreen();
            DrawTexture(twod, decoder.animTex().GetFrame(), 160, 100, DTA_FullscreenScale, FSMode_Fit320x200,
                DTA_CenterOffset, true, DTA_FlipY, true, DTA_ScaleX, z / 65536., DTA_ScaleY, z / 65536., DTA_Rotate, (-angle - 512) * BAngToDegree, TAG_DONE);
        }
        
        lastclock = clock;
    }

    void OnDestroy() override
    {
        decoder.Close();
        fp.Close();
    }
};



DScreenJob* PlayMovie(const char* fileName)
{
    // clear keys
    inputState.ClearAllInput();

    auto fp = fileSystem.OpenFileReader(fileName);
    if (!fp.isOpen())
    {
        return Create<DBlackScreen>(1);
    }
    char buffer[4];
    fp.Read(buffer, 4);
    if (memcmp(buffer, "LMF ", 4))
    {
        fp.Close();
        // Allpw replacement with more modern formats.
        return PlayVideo(fileName);
    }
    fp.Seek(0, FileReader::SeekSet);
    return Create<DLmfPlayer>(fp);
}

END_PS_NS
