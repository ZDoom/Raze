
#include "engine.h"
#include "exhumed.h"
#include "names.h"
#include "movie.h"
#include "cdrom.h"
#include "light.h"
#include <cstdio>
#include <cstring>
#include "baselayer.h"
#include "typedefs.h"
#include "keyboard.h"

enum {
    kFramePalette = 0,
    kFrameSound,
    kFrameImage,
    kFrameDone
};

uint8_t lh[32] = { 0 };
char streambuf[2205];
char byte_1C6DF5[2205];

static uint8_t* CurFrame = NULL;


int serve_sample()
{
//	if (!SoundCardActive()) {
//		return 1;
//	}

    return 0;
}

int ReadFrame(FILE *fp)
{
    static int nFrame = 0;
    DebugOut("Reading frame %d...\n", nFrame);
    nFrame++;

    uint8_t nType;
    uint8_t var_1C;
    int nSize;
    ushort yOffset;
    uint8_t xOffset;
    uint8_t nPixels;
    uint8_t palette[768];

    while (1)
    {
        if (fread(&nType, 1, sizeof(nType), fp) == 0) {
            return 0;
        }

        fread(&nSize, sizeof(nSize), 1, fp);

        nType--;
        if (nType > 3) {
            continue;
        }

        switch (nType)
        {
            case kFramePalette:
            {
                fread(palette, sizeof(palette[0]), sizeof(palette) / sizeof(palette[0]), fp);
                fread(&var_1C, sizeof(var_1C), 1, fp);

                MySetPalette(palette);

                memset(CurFrame, overscanindex, 4); //sizeof(CurFrame));
                continue;
            }
            case kFrameSound:
            {
                DebugOut("Reading sound block size %d...\n", nSize);
                // TODO - just skip for now
                fseek(fp, nSize, SEEK_CUR);
                continue;
            }
            case kFrameImage:
            {
                DebugOut("Reading image block size %d...\n", nSize);
                if (nSize == 0) {
                    continue;
                }

                uint8_t *pFrame = CurFrame;

                int nRead = fread(&yOffset, 1, sizeof(yOffset), fp);
                nSize -= nRead;

                pFrame += yOffset * 200; // row position

                while (nSize > 0)
                {
                    fread(&xOffset, sizeof(xOffset), 1, fp);
                    fread(&nPixels, sizeof(nPixels), 1, fp);
                    nSize -= 2;

                    pFrame += xOffset;

                    if (nPixels)
                    {
                        int nRead = fread(pFrame, 1, nPixels, fp);
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

void PlayMovie(const char *fileName)
{
    char buffer[256];

    int bDoFade = 1;

    if (bNoCDCheck)
    {
        sprintf(buffer, "C:\\PS\\%s", fileName);
    }
    else
    {
        char driveLetter = GetCDDriveLetter();
        if (!driveLetter) {
            driveLetter = 'C';
        }
        sprintf(buffer, "%c:%s", driveLetter, fileName);
    }

    tileLoad(kMovieTile);
    CurFrame = (uint8_t*)waloff[kMovieTile];

    FILE *fp = fopen(buffer, "rb");
    if (fp == NULL)
    {
        DebugOut("Can't open movie file '%s' on CD-ROM\n", buffer);
        fp = fopen(fileName, "rb");
        if (fp == NULL)
        {
            DebugOut("Can't open movie file on hard drive\n");
            return;
        }
    }

    fread(lh, sizeof(lh), 1, fp);
    memset(streambuf, 0, sizeof(streambuf));
    memset(byte_1C6DF5, 0, sizeof(byte_1C6DF5));

    // sound stuff

    // clear keys
    KB_FlushKeyboardQueue();
    KB_ClearKeysDown();

    if (bDoFade) {
        StartFadeIn();
    }

    int angle = 1536;
    int z = 0;

    if (ReadFrame(fp))
    {
        while (!KB_KeyWaiting())
        {
            handleevents();

            if (z < 65536) { // Zoom - normal zoom is 65536.
                z += 2048;
            }
            if (angle != 0) {
                angle += 16;
                if (angle == 2048) {
                    angle = 0;
                }
            }

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

    if (KB_KeyWaiting()) {
        KB_GetCh();
    }

    fclose(fp);
}
