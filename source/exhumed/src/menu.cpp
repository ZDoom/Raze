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
#include "compat.h"
#include "build.h"
#include "exhumed.h"
#include "typedefs.h"
#include "player.h"
#include "sequence.h"
#include "menu.h"
#include "names.h"
#include "engine.h"
#include "c_bind.h"
#include "status.h"
#include "random.h"
#include "sound.h"
#include "names.h"
#include "init.h"
#include "ps_input.h"
#include "gun.h"
#include "view.h"
#include "object.h"
#include "light.h"
#include "cd.h"
#include "raze_sound.h"
#include "menu/menu.h"
#include "v_2ddrawer.h"
#include <string>

#include <assert.h>

#ifdef __WATCOMC__
#include <stdlib.h>
#endif

BEGIN_PS_NS


#define kSaveFileName       "savgamea.sav"
#define kMaxSaveSlots		5
#define kMaxSaveSlotChars	25

GameStat GameStats;

short nCinemaSeen[30];

// this might be static within the DoPlasma function?
uint8_t * PlasmaBuffer;

uint8_t energytile[66 * 66] = {0};

short nLeft[50] = {0};
int line;

short SavePosition = -1;

uint8_t *cur;
uint8_t *dest;

unsigned int nSmokeBottom;
unsigned int nSmokeRight;
unsigned int nSmokeTop;
unsigned int nSmokeLeft;

unsigned int nRandom = 0x41C6167E;
int dword_9AB57 = 0x1F;
short word_9AB5B = 0;

int keytimer = 0;

int plasma_A[5] = {0};
int plasma_B[5] = {0};
int plasma_C[5] = {0};

short nMenuKeys[] = { sc_N, sc_L, sc_M, sc_V, sc_Q, sc_None }; // select a menu item using the keys. 'N' for New Gane, 'V' for voume etc. 'M' picks Training for some reason...


void menu_ResetKeyTimer();

enum {
    kMenuNewGame = 0,
    kMenuLoadGame,
    kMenuTraining,
    kMenuVolume,
    kMenuQuitGame,
    kMenuMaxItems
};


void ClearCinemaSeen()
{
    memset(nCinemaSeen, 0, sizeof(nCinemaSeen));
}

unsigned int menu_RandomBit2()
{
    unsigned int result = nRandom & 1;

    if ( --dword_9AB57 > 0 )
    {
        nRandom = (result << 31) | (nRandom >> 1);
    }
    else
    {
        dword_9AB57 = 31;
        nRandom ^= nRandom >> 4;
    }
    return result;
}

int menu_RandomLong2()
{
    int randLong = 0;

    for (int i = 0; i < 32; i++)
    {
        int val = menu_RandomBit2();
        randLong *= 2;
        randLong |= val;
    }

    return randLong;
}

void InitEnergyTile()
{
    memset(energytile, 96, sizeof(energytile));
}

void DoEnergyTile()
{
    nButtonColor += nButtonColor < 0 ? 8 : 0;

	auto energy1 = TileFiles.tileMakeWritable(kEnergy1);
	auto energy2 = TileFiles.tileMakeWritable(kEnergy2);
	uint8_t *ptr1 = energy1 + 1984;
    uint8_t *ptr2 = energy2 + 2048;

    short nColor = nButtonColor + 161;

    int i, j;

    for (i = 0; i < 32; i++)
    {
        memset(ptr1, nColor, 64);
        memset(ptr2, nColor, 64);

        ptr1 -= 64;
        ptr2 += 64;

        nColor++;

        if (nColor >= 168) {
            nColor = 160;
        }
    }

    tileInvalidate(kEnergy1, -1, -1);

    if (nSmokeSparks)
    {
        uint8_t *c = &energytile[67]; // skip a line
        uint8_t *ptrW = energy2;

        for (i = 0; i < 64; i++)
        {
            for (j = 0; j < 64; j++)
            {
                uint8_t val = *c;

                if (val != 96)
                {
                    if (val > 158) {
                        *ptrW = val - 1;
                    }
                    else {
                        *ptrW = 96;
                    }
                }
                else
                {
                    if (menu_RandomBit2()) {
                        *ptrW = *c;
                    }
                    else
                    {
                        uint8_t al = *(c + 1);
                        uint8_t ah = *(c - 1);

                        if (al <= ah) {
                            al = ah;
                        }

                        uint8_t cl = al;

                        al = *(c - 66);
                        if (cl <= al) {
                            cl = al;
                        }

                        al = *(c + 66);
                        if (cl <= al) {
                            cl = al;
                        }

                        al = *(c + 66);
                        if (cl <= al) {
                            cl = al;
                        }

                        al = *(c + 66);
                        if (cl <= al) {
                            cl = al;
                        }

                        al = *(c - 65);
                        if (cl <= al) {
                            cl = al;
                        }

                        al = *(c - 67);
                        if (cl > al) {
                            al = cl;
                        }

                        cl = al;

                        if (al <= 159) {
                            *ptrW = 96;
                        }
                        else
                        {
                            if (!menu_RandomBit2()) {
                                cl--;
                            }

                            *ptrW = cl;
                        }
                    }     
                }

                c++;
                ptrW++;
            }

            c += 2;
        }

        c = &energytile[67];
        ptrW = energy2;

        // copy back to energytile[]
        for (i = 0; i < 64; i++)
        {
            memcpy(c, ptrW, 64);
            c += 66;
            ptrW += 64;
        }

        ptrW = energy2;

        // kEnergy2 is 64 x 64
        for (i = 0; i < 4096; i++)
        {
            if (ptrW[i] == 96) {
                ptrW[i] = 255; // -1?
            }
        }

        word_9AB5B--;
        if (word_9AB5B <= 0)
        {
            int randSize  = (RandomSize(5) & 0x1F) + 16;
            int randSize2 = (RandomSize(5) & 0x1F) + 16;

            int val = randSize << 5;
            val += randSize;
            val *= 2;
            val += randSize2;

            assert(val < 4356);

            energytile[val] = 175;
            word_9AB5B = 1;
        }
        tileInvalidate(kEnergy2, -1, -1);
    }
}

int nPlasmaTile = kTile4092;
int nLogoTile;

#define kPlasmaWidth	320
#define kPlasmaHeight	80

int nextPlasmaTic;

void menu_DoPlasma()
{
    int ptile = nPlasmaTile;
    if (totalclock >= nextPlasmaTic || !PlasmaBuffer)
    {
        nextPlasmaTic = (int)totalclock + 4;

        if (!nLogoTile)
            nLogoTile = EXHUMED ? kExhumedLogo : kPowerslaveLogo;

        if (!PlasmaBuffer)
        {
            auto pixels = TileFiles.tileCreate(kTile4092, kPlasmaWidth, kPlasmaHeight);
            memset(pixels, 96, kPlasmaWidth * kPlasmaHeight);

            PlasmaBuffer = TileFiles.tileCreate(kTile4093, kPlasmaWidth, kPlasmaHeight);
            memset(PlasmaBuffer, 96, kPlasmaWidth * kPlasmaHeight);

            nSmokeLeft = 160 - tilesiz[nLogoTile].x / 2;
            nSmokeRight = nSmokeLeft + tilesiz[nLogoTile].x;

            nSmokeTop = 40 - tilesiz[nLogoTile].y / 2;
            nSmokeBottom = nSmokeTop + tilesiz[nLogoTile].y - 1;

            //uint32_t t = time(0) << 16;
            //uint32_t t2 = time(0) | t;
            nRandom = timerGetTicksU64();

            for (int i = 0; i < 5; i++)
            {
                int logoWidth = tilesiz[nLogoTile].x;
                plasma_C[i] = (nSmokeLeft + rand() % logoWidth) << 16;
                plasma_B[i] = (menu_RandomLong2() % 327680) + 0x10000;

                if (menu_RandomBit2()) {
                    plasma_B[i] = -plasma_B[i];
                }

                plasma_A[i] = menu_RandomBit2();
            }
        }

        videoClearScreen(overscanindex);


        uint8_t* plasmapix = const_cast<uint8_t*>(tilePtr(nPlasmaTile));
        uint8_t* r_ebx = plasmapix + 81;
        const uint8_t* r_edx = tilePtr(nPlasmaTile ^ 1) + 81; // flip between value of 4092 and 4093 with xor

        for (int x = 0; x < kPlasmaWidth - 2; x++)
            //	for (int x = 1; x < 318; x++)
        {
            //		for (int y = 1; y < 79; y++)
            for (int y = 0; y < kPlasmaHeight - 2; y++)
            {
                uint8_t al = *r_edx;

                if (al != 96)
                {
                    if (al > 158) {
                        *r_ebx = al - 1;
                    }
                    else {
                        *r_ebx = 96;
                    }
                }
                else
                {
                    if (menu_RandomBit2()) {
                        *r_ebx = *r_edx;
                    }
                    else
                    {
                        uint8_t al = *(r_edx + 1);
                        uint8_t cl = *(r_edx - 1);

                        if (al <= cl) {
                            al = cl;
                        }

                        cl = al;
                        al = *(r_edx - 80);
                        if (cl <= al) {
                            cl = al;
                        }

                        al = *(r_edx + 80);
                        if (cl <= al) {
                            cl = al;
                        }

                        al = *(r_edx + 80);
                        if (cl <= al) {
                            cl = al;
                        }

                        al = *(r_edx + 80);
                        if (cl <= al) {
                            cl = al;
                        }

                        al = *(r_edx - 79);
                        if (cl > al) {
                            al = cl;
                        }

                        cl = *(r_edx - 81);
                        if (al <= cl) {
                            al = cl;
                        }

                        cl = al;

                        if (al <= 159) {
                            *r_ebx = 96;
                        }
                        else
                        {
                            if (!menu_RandomBit2()) {
                                cl--;
                            }

                            *r_ebx = cl;
                        }
                    }
                }

                // before restarting inner loop
                r_edx++;
                r_ebx++;
            }

            // before restarting outer loop
            r_edx += 2;
            r_ebx += 2;
        }

        auto logopix = tilePtr(nLogoTile);

        for (int j = 0; j < 5; j++)
        {
            int pB = plasma_B[j];
            int pC = plasma_C[j];
            int badOffset = (pC >> 16) < nSmokeLeft || (pC >> 16) >= nSmokeRight;

            const uint8_t* ptr3 = (logopix + ((pC >> 16) - nSmokeLeft)* tilesiz[nLogoTile].y);

            plasma_C[j] += plasma_B[j];

            if ((pB > 0 && (plasma_C[j] >> 16) >= nSmokeRight) || (pB < 0 && (plasma_C[j] >> 16) <= nSmokeLeft))
            {
                int esi = plasma_A[j];
                plasma_B[j] = -plasma_B[j];
                plasma_A[j] = esi == 0;
            }

            if (badOffset)
                continue;

            unsigned int nSmokeOffset = 0;

            if (plasma_A[j])
            {
                nSmokeOffset = nSmokeTop;

                while (nSmokeOffset < nSmokeBottom)
                {
                    uint8_t al = *ptr3;
                    if (al != TRANSPARENT_INDEX && al != 96) {
                        break;
                    }

                    nSmokeOffset++;
                    ptr3++;
                }
            }
            else
            {
                nSmokeOffset = nSmokeBottom;

                ptr3 += tilesiz[nLogoTile].y - 1;

                while (nSmokeOffset > nSmokeTop)
                {
                    uint8_t al = *ptr3;
                    if (al != TRANSPARENT_INDEX && al != 96) {
                        break;
                    }

                    nSmokeOffset--;
                    ptr3--;
                }
            }

            uint8_t* v28 = plasmapix + (80 * (plasma_C[j] >> 16));
            v28[nSmokeOffset] = 175;
        }

        tileInvalidate(nPlasmaTile, -1, -1);

        // flip between tile 4092 and 4093
        if (nPlasmaTile == kTile4092) {
            nPlasmaTile = kTile4093;
        }
        else if (nPlasmaTile == kTile4093) {
            nPlasmaTile = kTile4092;
        }
    }
    overwritesprite(0,   0,  ptile,  0, 2, kPalNormal);
    overwritesprite(160, 40, nLogoTile, 0, 3, kPalNormal);

    // draw the fire urn/lamp thingies
    int dword_9AB5F = ((int)totalclock/16) & 3;

    overwritesprite(50,  150, kTile3512 + dword_9AB5F, 0, 3, kPalNormal);
    overwritesprite(270, 150, kTile3512 + ((dword_9AB5F + 2) & 3), 0, 3, kPalNormal);
}


int8_t MapLevelOffsets[] = { 0, 50, 10, 20, 0, 45, -20, 20, 5, 0, -10, 10, 30, -20, 0, 20, 0, 0, 0, 0 };

struct TILEFRAMEDEF
{
    short nTile;
    short xOffs;
    short yOffs;
};

// 22 bytes
struct MapNamePlaque
{
    short xPos;
    short yPos;
    TILEFRAMEDEF tiles[2];
    TILEFRAMEDEF text;
};

MapNamePlaque mapNamePlaques[] = {
    { 100, 170, kTile3376, 0, 0, kTile3377, 0, 0, kTile3411, 18, 6 },
    { 230, 10,  kTile3378, 0, 0, kTile3379, 0, 0, kTile3414, 18, 6 }, // DENDUR (level 2)
    { 180, 125, kTile3380, 0, 0, kTile3381, 0, 0, kTile3417, 18, 6 }, // Kalabash
    { 10,  95,  kTile3382, 0, 0, kTile3383, 0, 0, kTile3420, 18, 6 },
    { 210, 160, kTile3384, 0, 0, kTile3385, 0, 0, kTile3423, 18, 6 },
    { 10,  110, kTile3371, 0, 0, kTile3386, 0, 0, kTile3426, 18, 6 },
    { 10,  50,  kTile3387, 0, 0, kTile3388, 0, 0, kTile3429, 18, 6 },
    { 140, 0,   kTile3389, 0, 0, kTile3390, 0, 0, kTile3432, 18, 6 },
    { 30,  20,  kTile3391, 0, 0, kTile3392, 0, 0, kTile3435, 18, 6 },
    { 200, 150, kTile3409, 0, 0, kTile3410, 0, 0, kTile3418, 20, 4 },
    { 145, 170, kTile3393, 0, 0, kTile3394, 0, 0, kTile3438, 18, 6 },
    { 80,  80,  kTile3395, 0, 0, kTile3396, 0, 0, kTile3441, 18, 6 },
    { 15,  0,   kTile3397, 0, 0, kTile3398, 0, 0, kTile3444, 18, 5 },
    { 220, 35,  kTile3399, 0, 0, kTile3400, 0, 0, kTile3447, 18, 6 },
    { 190, 40,  kTile3401, 0, 0, kTile3402, 0, 0, kTile3450, 18, 6 },
    { 20,  130, kTile3403, 0, 0, kTile3404, 0, 0, kTile3453, 19, 6 },
    { 220, 160, kTile3405, 0, 0, kTile3406, 0, 0, kTile3456, 18, 6 },
    { 20,  10,  kTile3407, 0, 0, kTile3408, 0, 0, kTile3459, 18, 6 },
    { 200, 10,  kTile3412, 0, 0, kTile3413, 0, 0, kTile3419, 18, 5 },
    { 20,  10,  kTile3415, 0, 0, kTile3416, 0, 0, kTile3421, 19, 4 }
};

// 3 different types of fire, each with 4 frames
TILEFRAMEDEF FireTiles[3][4] = {
    {{ kTile3484,0,3 },{ kTile3485,0,0 },{ kTile3486,0,3 },{ kTile3487,0,0 }},
    {{ kTile3488,1,0 },{ kTile3489,1,0 },{ kTile3490,0,1 },{ kTile3491,1,1 }},
    {{ kTile3492,1,2 },{ kTile3493,1,0 },{ kTile3494,1,2 },{ kTile3495,1,0 }}
};

struct Fire
{
    short nFireType;
    short xPos;
    short yPos;
};

// 20 bytes
struct MapFire
{
    short nFires;
    Fire fires[3];
};

/*
 level 1 - 3 fires
 level 2 - 3 fires
 level 3 - 1 fire

*/

MapFire MapLevelFires[] = {
    3, {{0, 107, 95}, {1, 58,  140}, {2, 28,   38}},
    3, {{2, 240,  0}, {0, 237,  32}, {1, 200,  30}},
    2, {{2, 250, 57}, {0, 250,  43}, {2, 200,  70}},
    2, {{1, 82,  59}, {2, 84,   16}, {0, 10,   95}},
    2, {{2, 237, 50}, {1, 215,  42}, {1, 210,  50}},
    3, {{0, 40,   7}, {1, 75,    6}, {2, 100,  10}},
    3, {{0, 58,  61}, {1, 85,   80}, {2, 111,  63}},
    3, {{0, 260, 65}, {1, 228,   0}, {2, 259,  15}},
    2, {{0, 81,  38}, {2, 58,   38}, {2, 30,   20}},
    3, {{0, 259, 49}, {1, 248,  76}, {2, 290,  65}},
    3, {{2, 227, 66}, {0, 224,  98}, {1, 277,  30}},
    2, {{0, 100, 10}, {2, 48,   76}, {2, 80,   80}},
    3, {{0, 17,   2}, {1, 29,   49}, {2, 53,   28}},
    3, {{0, 266, 42}, {1, 283,  99}, {2, 243, 108}},
    2, {{0, 238, 19}, {2, 240,  92}, {2, 190,  40}},
    2, {{0, 27,   0}, {1, 70,   40}, {0, 20,  130}},
    3, {{0, 275, 65}, {1, 235,   8}, {2, 274,   6}},
    3, {{0, 75,  45}, {1, 152, 105}, {2, 24,   68}},
    3, {{0, 290, 25}, {1, 225,  63}, {2, 260, 110}},
    0, {{1, 20,  10}, {1, 20,   10}, {1, 20,   10}}
};

int menu_DrawTheMap(int nLevel, int nLevelNew, int nLevelBest)
{
    int i;
    int x = 0;
    int var_2C = 0;
    int nIdleSeconds = 0;
    int bFadeDone = kFalse;

    int startTime = (int)totalclock;

    inputState.ClearAllInput();
    UnMaskStatus();
    videoSetViewableArea(0, 0, xdim - 1, ydim - 1);

    // 0-offset the level numbers
    nLevel--;
    nLevelNew--;
    nLevelBest--;

    if (nLevel >= kMap20) { // max single player levels
        return -1;
    }

    if (nLevelNew >= kMap20) {
        return -1;
    }

    if (nLevel < 0) {
        nLevel = 0;
    }

    if (nLevelNew < 0) {
        nLevelNew = nLevel;
    }

    int curYPos = MapLevelOffsets[nLevel] + (200 * (nLevel / 2));
    int destYPos = MapLevelOffsets[nLevelNew] + (200 * (nLevelNew / 2));

    if (curYPos < destYPos) {
        var_2C = 2;
    }

    if (curYPos > destYPos) {
        var_2C = -2;
    }

    int runtimer = (int)totalclock;

    // Trim smoke in widescreen
    vec2_t mapwinxy1 = windowxy1, mapwinxy2 = windowxy2;
    int32_t width = mapwinxy2.x - mapwinxy1.x + 1, height = mapwinxy2.y - mapwinxy1.y + 1;
    if (3 * width > 4 * height)
    {
        mapwinxy1.x += (width - 4 * height / 3) / 2;
        mapwinxy2.x -= (width - 4 * height / 3) / 2;
    }

    // User has 12 seconds to do something on the map screen before loading the current level
    while (nIdleSeconds < 12)
    {
        HandleAsync();
        twod->ClearScreen();

        if (((int)totalclock - startTime) / kTimerTicks)
        {
            nIdleSeconds++;
            startTime = (int)totalclock;
        }

        int tileY = curYPos;

        // Draw the background screens
        for (i = 0; i < 10; i++)
        {
            overwritesprite(x, tileY, kTile3353 + i, 0, 2, kPalNormal);
            tileY -= 200;
        }

        // for each level - drawing the 'level completed' on-fire smoke markers
        for (i = 0; i < kMap20; i++)
        {
            int screenY = (i >> 1) * -200;

            if (nLevelBest >= i) // check if the player has finished this level
            {
                for (int j = 0; j < MapLevelFires[i].nFires; j++)
                {
                    int nFireFrame = (((int)totalclock >> 4) & 3);
                    assert(nFireFrame >= 0 && nFireFrame < 4);

                    int nFireType = MapLevelFires[i].fires[j].nFireType;
                    assert(nFireType >= 0 && nFireType < 3);

                    int nTile = FireTiles[nFireType][nFireFrame].nTile;
                    int smokeX = MapLevelFires[i].fires[j].xPos + FireTiles[nFireType][nFireFrame].xOffs;
                    int smokeY = MapLevelFires[i].fires[j].yPos + FireTiles[nFireType][nFireFrame].yOffs + curYPos + screenY;

                    // Use rotatesprite to trim smoke in widescreen
                    rotatesprite(smokeX << 16, smokeY << 16, 65536L, 0,
                                 nTile, 0, kPalNormal, 16 + 2, mapwinxy1.x, mapwinxy1.y, mapwinxy2.x, mapwinxy2.y);
//                    overwritesprite(smokeX, smokeY, nTile, 0, 2, kPalNormal);
                }
            }

            int t = ((((int)totalclock & 16) >> 4));

            int nTile = mapNamePlaques[i].tiles[t].nTile;

            int nameX = mapNamePlaques[i].xPos + mapNamePlaques[i].tiles[t].xOffs;
            int nameY = mapNamePlaques[i].yPos + mapNamePlaques[i].tiles[t].yOffs + curYPos + screenY;

            // Draw level name plaque
            overwritesprite(nameX, nameY, nTile, 0, 2, kPalNormal);

            int8_t shade = 96;

            if (nLevelNew == i)
            {
                shade = (Sin(16 * (int)totalclock) + 31) >> 8;
            }
            else if (nLevelBest >= i)
            {
                shade = 31;
            }

            int textY = mapNamePlaques[i].yPos + mapNamePlaques[i].text.yOffs + curYPos + screenY;
            int textX = mapNamePlaques[i].xPos + mapNamePlaques[i].text.xOffs;
            nTile = mapNamePlaques[i].text.nTile;

            // draw the text, alternating between red and black
            overwritesprite(textX, textY, nTile, shade, 2, kPalNormal);
        }

        videoNextPage();
        if (!bFadeDone)
        {
            bFadeDone = kTrue;
            FadeIn();
        }

        if (curYPos == destYPos)
        {
            if (inputState.GetKeyStatus(sc_UpArrow))
            {
                inputState.ClearKeyStatus(sc_UpArrow);

                if (nLevelNew <= nLevelBest)
                {
                    nLevelNew++;
                    assert(nLevelNew < 20);

                    destYPos = MapLevelOffsets[nLevelNew] + (200 * (nLevelNew / 2));

                    if (curYPos <= destYPos) {
                        var_2C = 2;
                    }
                    else {
                        var_2C = -2;
                    }

                    nIdleSeconds = 0;
                }
            }

            if (inputState.GetKeyStatus(sc_DownArrow))
            {
                inputState.ClearKeyStatus(sc_DownArrow);

                if (nLevelNew > 0)
                {
                    nLevelNew--;
                    assert(nLevelNew >= 0);

                    destYPos = MapLevelOffsets[nLevelNew] + (200 * (nLevelNew / 2));

                    if (curYPos <= destYPos) {
                        var_2C = 2;
                    }
                    else {
                        var_2C = -2;
                    }

                    nIdleSeconds = 0;
                }
            }

            if (inputState.CheckAllInput())
            {
                return nLevelNew + 1;
            }
        }
        else
        {
            // scroll the map every couple of ms
            if (totalclock - runtimer >= (kTimerTicks / 32)) {
                curYPos += var_2C;
                runtimer = (int)totalclock;
            }

			if (inputState.CheckAllInput())
			{
				if (var_2C < 8) {
                    var_2C *= 2;
                }

            }

            if (curYPos > destYPos&& var_2C > 0) {
                curYPos = destYPos;
            }

            if (curYPos < destYPos && var_2C < 0) {
                curYPos = destYPos;
            }

            nIdleSeconds = 0;
        }
    }

    MySetView(nViewLeft, nViewTop, nViewRight, nViewBottom);
    return nLevelNew + 1;
}

int menu_NewGameMenu()
{

    return 0;
}

int menu_LoadGameMenu()
{
    return 0;
}

void menu_GameLoad2(FILE *fp, bool bIsDemo)
{
    if (bIsDemo)
    {
        demo_header header;
        fread(&header, 1, sizeof(demo_header), fp);

        GameStats.nMap = header.nMap;
        GameStats.nWeapons = header.nWeapons;
        GameStats.nCurrentWeapon = header.nCurrentWeapon;
        GameStats.clip = header.clip;
        GameStats.items = header.items;
        GameStats.player.nHealth = header.nHealth;
        GameStats.player.field_2 = header.field_2;
        GameStats.player.nAction = header.nAction;
        GameStats.player.nSprite = header.nSprite;
        GameStats.player.bIsMummified = header.bIsMummified;
        GameStats.player.someNetVal = header.someNetVal;
        GameStats.player.invincibility = header.invincibility;
        GameStats.player.nAir = header.nAir;
        GameStats.player.nSeq = header.nSeq;
        GameStats.player.nMaskAmount = header.nMaskAmount;
        GameStats.player.keys = header.keys;
        GameStats.player.nMagic = header.nMagic;
        Bmemcpy(GameStats.player.items, header.item, sizeof(header.item));
        Bmemcpy(GameStats.player.nAmmo, header.nAmmo, sizeof(header.nAmmo));
        Bmemcpy(GameStats.player.pad, header.pad, sizeof(header.pad));
        GameStats.player.nCurrentWeapon = header.nCurrentWeapon2;
        GameStats.player.field_3FOUR = header.field_3FOUR;
        GameStats.player.bIsFiring = header.bIsFiring;
        GameStats.player.field_38 = header.field_38;
        GameStats.player.field_3A = header.field_3A;
        GameStats.player.field_3C = header.field_3C;
        GameStats.player.nRun = header.nRun;
        GameStats.nLives = header.nLives;
    }
    else
        fread(&GameStats, sizeof(GameStats), 1, fp);

    nPlayerWeapons[nLocalPlayer] = GameStats.nWeapons;

    PlayerList[nLocalPlayer].nCurrentWeapon = GameStats.nCurrentWeapon;
    nPlayerClip[nLocalPlayer] = GameStats.clip;

    int nPistolBullets = PlayerList[nLocalPlayer].nAmmo[kWeaponPistol];
    if (nPistolBullets >= 6) {
        nPistolBullets = 6;
    }

    nPistolClip[nLocalPlayer] = nPistolBullets;

    memcpy(&PlayerList[nLocalPlayer], &GameStats.player, sizeof(Player));

    nPlayerItem[nLocalPlayer]  = GameStats.items;
    nPlayerLives[nLocalPlayer] = GameStats.nLives;

    SetPlayerItem(nLocalPlayer, nPlayerItem[nLocalPlayer]);
    CheckClip(nLocalPlayer);
}

short menu_GameLoad(int nSlot)
{
    memset(&GameStats, 0, sizeof(GameStats));

    FILE *fp = fopen(kSaveFileName, "rb");
    if (fp == NULL) {
        return 0;
    }

    fseek(fp, 125, SEEK_SET);
    fseek(fp, nSlot * sizeof(GameStats), SEEK_CUR);

    menu_GameLoad2(fp);
    fclose(fp);

    return GameStats.nMap;
}

void menu_GameSave2(FILE *fp)
{
    memset(&GameStats, 0, sizeof(GameStats));

    GameStats.nMap = (uint8_t)levelnew;
    GameStats.nWeapons = nPlayerWeapons[nLocalPlayer];
    GameStats.nCurrentWeapon = PlayerList[nLocalPlayer].nCurrentWeapon;
    GameStats.clip   = nPlayerClip[nLocalPlayer];
    GameStats.items  = nPlayerItem[nLocalPlayer];
    GameStats.nLives = nPlayerLives[nLocalPlayer];

    memcpy(&GameStats.player, &PlayerList[nLocalPlayer], sizeof(GameStats.player));

    fwrite(&GameStats, sizeof(GameStats), 1, fp);
}

void menu_GameSave(int nSaveSlot)
{
    if (nSaveSlot < 0) {
        return;
    }

    FILE *fp = fopen(kSaveFileName, "rb+");
    if (fp != NULL)
    {
        fseek(fp, 125, SEEK_SET); // skip save slot names
        fseek(fp, sizeof(GameStat) * nSaveSlot, SEEK_CUR);
        menu_GameSave2(fp);
        fclose(fp);
    }
}

#define kMaxCinemaPals	16
const char *cinpalfname[kMaxCinemaPals] = {
    "3454.pal",
    "3452.pal",
    "3449.pal",
    "3445.pal",
    "set.pal",
    "3448.pal",
    "3446.pal",
    "hsc1.pal",
    "2972.pal",
    "2973.pal",
    "2974.pal",
    "2975.pal",
    "2976.pal",
    "heli.pal",
    "2978.pal",
    "terror.pal"
};

int linecount;
int nextclock;
short nHeight;
short nCrawlY;
short cinematile;
int currentCinemaPalette;


void uploadCinemaPalettes()
{
    for (int i = 0; i < countof(cinpalfname); i++)
    {
        uint8_t palette[768] = {};
        auto hFile = fileSystem.OpenFileReader(cinpalfname[i]);
        if (hFile.isOpen())
            hFile.Read(palette, 768);
        for (auto& c : palette)
            c <<= 2;
        paletteSetColorTable(ANIMPAL+i, palette, false, true);
    }
}

//int IncrementCinemaFadeIn()
//{
//    dest = cinemapal;
//    cur = curpal;
//
//    int ebx = 0;
//
//    for (int i = 0; i < 768; i++)
//    {
//        ebx++;
//
//        if (*cur < *dest)
//        {
//            (*cur)++;
//        }
//        else if (*cur == *dest)
//        {
//            ebx--;
//        }
//        else
//        {
//            (*cur)--;
//        }
//
//        cur++;
//        dest++;
//    }
//
//    MySetPalette(curpal);
//    return ebx;
//}

void CinemaFadeIn()
{
    BlackOut();

    //videoSetPalette(0, ANIMPAL, Pal_Fullscreen);

#ifdef USE_OPENGL
    if (videoGetRenderMode() >= REND_POLYMOST)
    {
        videoNextPage();
        return;
    }
#endif

    int val;

    do
    {
        val = DoFadeIn();
        WaitTicks(2);

        // need to page flip in each iteration of the loop for non DOS version
        videoNextPage();

    } while (val > 0);
}

void ComputeCinemaText(int nLine)
{
    linecount = 0;

    while (1)
    {
        if (!strcmp(gString[linecount + nLine], "END")) {
            break;
        }

        int nWidth = MyGetStringWidth(gString[linecount + nLine]);
        nLeft[linecount] = 160 - nWidth / 2;

        linecount++;
    }

    nCrawlY = 199;
    nHeight = linecount * 10;

    inputState.ClearAllInput();
}

void ReadyCinemaText(uint16_t nVal)
{
    line = FindGString("CINEMAS");
    if (line < 0) {
        return;
    }

    while (nVal)
    {
        while (strcmp(gString[line], "END")) {
            line++;
        }

        line++;
        nVal--;
    }

    ComputeCinemaText(line);
}

bool AdvanceCinemaText()
{
    bool bDoText = nHeight + nCrawlY > 0;

    if (bDoText || CDplaying())
    {
        nextclock = (int)totalclock + 15; // NOTE: Value was 14 in original code but seems a touch too fast now

        if (bDoText)
        {
            short y = nCrawlY;
            int i = 0;

            while (i < linecount && y <= 199)
            {
                if (y >= -10) {
                    myprintext(nLeft[i], y, gString[line + i], 0, currentCinemaPalette);
                }

                i++;
                y += 10;
            }

            nCrawlY--;
        }

        while (1)
        {
            HandleAsync();

            if (inputState.CheckAllInput())
            {
                break;
            }

            if (nextclock <= (int)totalclock) {
                return true;
            }
        }
    }

    return false;
}

void DoCinemaText(short nVal)
{
    ReadyCinemaText(nVal);

    bool bContinue = true;

    while (bContinue)
    {
        overwritesprite(0, 0, cinematile, 0, 2, kPalNormal, currentCinemaPalette);

        bContinue = AdvanceCinemaText();

        WaitVBL();
        videoNextPage();
    }
}

void GoToTheCinema(int nVal)
{
    UnMaskStatus();

    switch (nVal - 1)
    {
        default:
            return;

        case 0:
        {
            cinematile = 3454;
            break;
        }

        case 1:
        {
            cinematile = 3452;
            break;
        }

        case 2:
        {
            cinematile = 3449;
            break;
        }

        case 3:
        {
            cinematile = 3445;
            break;
        }

        case 4:
        {
            cinematile = 3451;
            break;
        }

        case 5:
        {
            cinematile = 3448;
            break;
        }

        case 6:
        {
            cinematile = 3446;
            break;
        }
    }
    currentCinemaPalette = nVal;

#if 0
    if (ISDEMOVER) {
        //???
        if (tilesiz[cinematile].x * tilesiz[cinematile].y == 0)
            TileFiles.tileCreate(cinematile, 320, 200);
    }
#endif

    FadeOut(kFalse);
    StopAllSounds();
    NoClip();

    overwritesprite(0, 0, kMovieTile, 100, 2, kPalNormal, currentCinemaPalette);
    videoNextPage();

//	int386(16, (const union REGS *)&val, (union REGS *)&val)

    overwritesprite(0, 0, cinematile, 0, 2, kPalNormal, currentCinemaPalette);
    videoNextPage();

    CinemaFadeIn();
    inputState.ClearAllInput();

    int ebx = -1;
    int edx = -1;

    switch (nVal - 1)
    {
        default:
            WaitAnyKey(10);
            break;

        case 0:
            ebx = 4;
            edx = ebx;
            break;

        case 1:
            ebx = 0;
            break;

        case 2:
            ebx = 2;
            edx = ebx;
            break;

        case 3:
            ebx = 7;
            break;

        case 4:
            ebx = 3;
            edx = ebx;
            break;

        case 5:
            ebx = 8;
            edx = ebx;
            break;

        case 6:
            ebx = 6;
            edx = ebx;
            break;
    }

    if (ebx != -1)
    {
        if (edx != -1)
        {
            if (CDplaying()) {
                fadecdaudio();
            }

            playCDtrack(edx + 2, false);
        }

        DoCinemaText(ebx);
    }

    FadeOut(kTrue);

    overwritesprite(0, 0, kMovieTile, 100, 2, kPalNormal, currentCinemaPalette);
    videoNextPage();

    GrabPalette();
    Clip();

    // quit the game if we've finished level 4 and displayed the advert text
    if (ISDEMOVER && nVal == 3) {
        ExitGame();
    }
}


short nBeforeScene[] = { 0, 0, 0, 0, 0, 3, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0 };


void CheckBeforeScene(int nLevel)
{
    if (nLevel == kMap20)
    {
        DoLastLevelCinema();
        return;
    }

    short nScene = nBeforeScene[nLevel];

    if (nScene)
    {
        if (!nCinemaSeen[nScene])
        {
            GoToTheCinema(nScene);
            nCinemaSeen[nScene] = 1;
        }
    }
}

int showmap(short nLevel, short nLevelNew, short nLevelBest)
{
    FadeOut(0);
    EraseScreen(overscanindex);
    GrabPalette();
    BlackOut();

    if (nLevelNew != 11) {
        CheckBeforeScene(nLevelNew);
    }

    int selectedLevel = menu_DrawTheMap(nLevel, nLevelNew, nLevelBest);
    if (selectedLevel == 11) {
        CheckBeforeScene(selectedLevel);
    }

    return selectedLevel;
}

void DoAfterCinemaScene(int nLevel)
{
    short nAfterScene[] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 5, 0, 0, 0, 0, 7, 0, 0, 0, 0, 6 };

    if (nAfterScene[nLevel]) {
        GoToTheCinema(nAfterScene[nLevel]);
    }
}

void DoFailedFinalScene()
{
    videoSetViewableArea(0, 0, xdim - 1, ydim - 1);

    if (CDplaying()) {
        fadecdaudio();
    }

    playCDtrack(9, false);
    FadeToWhite();

    GoToTheCinema(4);
}

int FindGString(const char *str)
{
    int i = 0;

    while (1)
    {
        if (!strcmp(gString[i], str))
            return i + 1;

        if (!strcmp(gString[i], "EOF"))
            break;

        i++;
    }

    return -1;
}

uint8_t CheckForEscape()
{
    return inputState.CheckAllInput();
}

void DoStatic(int a, int b)
{
    RandomLong(); // nothing done with the result of this?

    auto pixels = TileFiles.tileMakeWritable(kTileLoboLaptop);

    int v2 = 160 - a / 2;
    int v4 = 81  - b / 2;

    int var_18 = v2 + a;
    int v5 = v4 + b;

    auto pTile = (pixels + (200 * v2)) + v4;

    tileInvalidate(kTileLoboLaptop, -1, -1);

    while (v2 < var_18)
    {
        uint8_t *pStart = pTile;
        pTile += 200;

        int v7 = v4;

        while (v7 < v5)
        {
            *pStart = RandomBit() * 16;

            v7++;
            pStart++;
        }
        v2++;
    }

	tileInvalidate(kTileLoboLaptop, 0, 0);
    overwritesprite(0, 0, kTileLoboLaptop, 0, 2, kPalNormal);
    videoNextPage();
}

void DoLastLevelCinema()
{
    FadeOut(0);
    UnMaskStatus();

    videoSetViewableArea(0, 0, xdim - 1, ydim - 1);

    EraseScreen(-1);
    RestorePalette();

    int nString = FindGString("LASTLEVEL");

    PlayLocalSound(StaticSound[kSound75], 0, false, CHANF_UI);

	auto pixels = TileFiles.tileMakeWritable(kTileLoboLaptop);
	// uh, what?
    //memcpy((void*)waloff[kTileLoboLaptop], (void*)waloff[kTileLoboLaptop], tilesiz[kTileLoboLaptop].x * tilesiz[kTileLoboLaptop].y);

    int var_24 = 16;
    int var_28 = 12;

    int nEndTime = (int)totalclock + 240;

    while (inputState.keyBufferWaiting()) {
        inputState.keyGetChar();
    }

    while (nEndTime > (int)totalclock)
    {
        HandleAsync();

        if (var_24 >= 116)
        {
            if (var_28 < 192)
                var_28 += 20;
        }
        else
        {
            var_24 += 20;
        }

        DoStatic(var_28, var_24);

        // WaitVBL();
        int time = (int)totalclock + 4;
        while ((int)totalclock < time) {
            HandleAsync();
        }
    }

    // loc_3AD75

    do
    {  
    LABEL_11:

        HandleAsync();

        if (strlen(gString[nString]) == 0)
            break;

        int esi = nString;

        while (strlen(gString[esi]) != 0)
            esi++;

        int ebp = esi;

        ebp -= nString;
        ebp <<= 2;
        ebp = 81 - ebp;

        int var_1C = esi - nString;

        // loc_3ADD7
        while (1)
        {
            HandleAsync();

            if (strlen(gString[nString]) == 0)
                break;

            int xPos = 70;

            const char *nChar = gString[nString];

            nString++;

			TileFiles.tileMakeWritable(kTileLoboLaptop);
            while (*nChar)
            {
                HandleAsync();

                if (*nChar != ' ') {
                    PlayLocalSound(StaticSound[kSound71], 0, false, CHANF_UI);
                }

                xPos += CopyCharToBitmap(*nChar, kTileLoboLaptop, xPos, ebp);
                nChar++;

                overwritesprite(0, 0, kTileLoboLaptop, 0, 2, kPalNormal);
                videoNextPage();

                // WaitVBL();
                int time = (int)totalclock + 4;
                while ((int)totalclock < time) {
                    HandleAsync();
                }

                if (CheckForEscape())
                    goto LABEL_28;
            }

            ebp += 8;
        }

        nString++;

        inputState.ClearAllInput();

        int v11 = (kTimerTicks * (var_1C + 2)) + (int)totalclock;

        do
        {
            HandleAsync();

            if (v11 <= (int)totalclock)
                goto LABEL_11;
        } while (!inputState.keyBufferWaiting());
    }
    while (inputState.keyGetChar() != 27);

LABEL_28:
    PlayLocalSound(StaticSound[kSound75], 0, false, CHANF_UI);

    nEndTime = (int)totalclock + 240;

    while (nEndTime > (int)totalclock)
    {
        HandleAsync();

        DoStatic(var_28, var_24);

        // WaitVBL();
        int time = (int)totalclock + 4;
        while ((int)totalclock < time) {
            HandleAsync();
        }

        if (var_28 > 20) {
            var_28 -= 20;
            continue;
        }

        if (var_24 > 20) {
            var_24 -= 20;
            continue;
        }

        break;
    }

    EraseScreen(-1);
    tileLoad(kTileLoboLaptop);
    FadeOut(0);
    MySetView(nViewLeft, nViewTop, nViewRight, nViewBottom);
    MaskStatus();
}

static SavegameHelper sgh("menu",
    SA(nCinemaSeen),
    SA(energytile),
    SV(nButtonColor),
    SV(word_9AB5B),
    nullptr);

END_PS_NS
