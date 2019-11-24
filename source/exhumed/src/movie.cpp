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
#include "cdrom.h"
#include "light.h"
#include <cstdio>
#include <cstring>
#include "baselayer.h"
#include "typedefs.h"
#include "keyboard.h"
#include "cache1d.h"

BEGIN_PS_NS

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
                videoSetPalette(0, ANIMPAL, 2+8);

                memset(CurFrame, overscanindex, 4); //sizeof(CurFrame));
                continue;
            }
            case kFrameSound:
            {
                Printf("Reading sound block size %d...\n", nSize);
                // TODO - just skip for now
                fp.Seek(nSize, FileReader::SeekCur);
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

void PlayMovie(const char *fileName)
{
    int bDoFade = 1;

#if 0	// What's the point of preserving this? Let's just read the movie from the game directory and ignore the other locations!
	char buffer[256];

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

	FILE *fp = fopen(buffer, "rb");
    if (fp == NULL)
    {
        Printf("Can't open movie file '%s' on CD-ROM\n", buffer);
        fp = fopen(fileName, "rb");
        if (fp == NULL)
        {
            Printf("Can't open movie file on hard drive\n");
            return;
        }
    }
#else
	auto fp = kopenFileReader(fileName, 0);
	if (!fp.isOpen())
	{
		Printf("Unable to open %s\n", fileName);
		return;
	}
#endif

	tileLoad(kMovieTile);
	CurFrame = TileFiles.tileMakeWritable(kMovieTile);


    fp.Read(lh, sizeof(lh));
    memset(streambuf, 0, sizeof(streambuf));
    memset(byte_1C6DF5, 0, sizeof(byte_1C6DF5));

    // sound stuff

    // clear keys
    inputState.keyFlushChars();
    inputState.ClearAllKeyStatus();

    if (bDoFade) {
        StartFadeIn();
    }

    int angle = 1536;
    int z = 0;

    videoSetPalette(0, ANIMPAL, 2+8);

    if (ReadFrame(fp))
    {
        while (!inputState.keyBufferWaiting())
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

    if (inputState.keyBufferWaiting()) {
        inputState.keyGetChar();
    }
}
END_PS_NS
