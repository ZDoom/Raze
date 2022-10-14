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
#include "vm.h"

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

    FileReader fp;

    int nFrame = 0;

    uint64_t nextclock = 0;

public:

    LMFPlayer(const char *filename)
    {
        fp = fileSystem.OpenFileReader(filename);
        Open(fp);
    }

    bool Frame(uint64_t clock)
    {
        if (clock >= nextclock)
        {
            nextclock += 100'000'000;
            if (ReadFrame(fp) == 0)
            {
                return true;
            }
        }

        return false;
    }

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

                int nRead = (int)fp.Read(&yOffset, sizeof(yOffset));
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
                        nRead = (int)fp.Read(pFrame, nPixels);
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
        if (pId->audio.nRead >= (int)countof(pId->audio.samples)) pId->audio.nRead = 0;
        return true;
    }

    void Open(FileReader& fp)
    {

        uint8_t header[32];
        fp.Read(header, sizeof(header));
        audio.nWrite = 5; // play 5 blocks (i.e. half a second) of silence to get ahead of the stream. For this video it isn't necessary to sync it perfectly.

        // start audio playback
        stream = S_CreateCustomStream(kSampleSize * 2, kSampleRate, 1, MusicSamples16bit, StreamCallbackFunc, this); // size must be doubled here or dropouts can be heard.
        animtex.SetSize(AnimTexture::Paletted, 200, 320);
    }

    void Close()
    {
        S_StopCustomStream(stream);
    }

    FTextureID GetTexture()
    {
        return animtex.GetFrameID();
    }
};

int IdentifyLMF(const FString* fn)
{
    auto fp = fileSystem.OpenFileReader(*fn);
    if (!fp.isOpen()) return false;
    char buffer[4];
    fp.Read(buffer, 4);
    return (0 == memcmp(buffer, "LMF ", 4));
}

DEFINE_ACTION_FUNCTION(_LMFDecoder, Create)
{
    PARAM_PROLOGUE;
    PARAM_STRING(fn);
    ACTION_RETURN_POINTER(new LMFPlayer(fn));
}

DEFINE_ACTION_FUNCTION_NATIVE(_LMFDecoder, Identify, IdentifyLMF)
{
    PARAM_PROLOGUE;
    PARAM_STRING(fn);
    ACTION_RETURN_BOOL(IdentifyLMF(&fn));
}

DEFINE_ACTION_FUNCTION(_LMFDecoder, Frame)
{
    PARAM_SELF_STRUCT_PROLOGUE(LMFPlayer);
    PARAM_FLOAT(clock);
    ACTION_RETURN_BOOL(self->Frame(uint64_t(clock)));
}

DEFINE_ACTION_FUNCTION(_LMFDecoder, GetTexture)
{
    PARAM_SELF_STRUCT_PROLOGUE(LMFPlayer);
    ACTION_RETURN_INT(self->GetTexture().GetIndex());
}

DEFINE_ACTION_FUNCTION(_LMFDecoder, Close)
{
    PARAM_SELF_STRUCT_PROLOGUE(LMFPlayer);
    self->Close();
    delete self;
    return 0;
}



END_PS_NS
