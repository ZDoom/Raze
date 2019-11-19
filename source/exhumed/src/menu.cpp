#include "compat.h"
#include "build.h"
#include "exhumed.h"
#include "typedefs.h"
#include "player.h"
#include "sequence.h"
#include "menu.h"
#include "names.h"
#include "engine.h"
#include "keyboard.h"
#include "status.h"
#include "random.h"
#include "sound.h"
#include "names.h"
#include "init.h"
#include "input.h"
#include "gun.h"
#include "view.h"
#include "object.h"
#include "light.h"
#include "cd.h"
#include "cdaudio.h"
#include <string>

#include <assert.h>

#ifdef __WATCOMC__
#include <stdlib.h>
#endif

#define kSaveFileName       "savgamea.sav"
#define kMaxSaveSlots		5
#define kMaxSaveSlotChars	25

GameStat GameStats;

short nCinemaSeen[30];

// this might be static within the DoPlasma function?
uint8_t plasmaBuffer[25600];

uint8_t energytile[66 * 66] = {0};

uint8_t cinemapal[768];
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

int zoomsize = 0;

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
    tileLoad(kEnergy1);
    tileLoad(kEnergy2);

    nButtonColor += nButtonColor < 0 ? 8 : 0;

    uint8_t *ptr1 = (uint8_t*)(waloff[kEnergy1] + 1984);
    uint8_t *ptr2 = (uint8_t*)(waloff[kEnergy1] + 2048);

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
        uint8_t *ptrW = (uint8_t*)waloff[kEnergy2];

        for (i = 0; i < 64; i++)
        {
            for (j = 0; j < 64; j++)
            {
                if (*c != 96)
                {
                    if (*c <= 158) {
                        *ptrW = 96;
                    }
                    else {
                        *ptrW = (*c) - 1;
                    }
                    //continue;
                }
                else
                {
                    if (menu_RandomBit2()) {
                        *ptrW = *c;
                        c++;
                        ptrW++;
                        continue;
                    }

                    char al = *(c + 1);
                    char ah = *(c - 1);

                    if (al <= ah) {
                        al = ah;
                    }

                    char cl = al;
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
                        //continue;
                    }
                    else 
                    {
                        if (!menu_RandomBit2())
                        {
                            cl--;
                        }

                        *ptrW = cl;
                    }
                }

                c++;
                ptrW++;
            }

            c += 2;
        }

        c = &energytile[67];
        ptrW = (uint8_t*)waloff[kEnergy2];

        for (i = 0; i < 64; i++)
        {
            memcpy(c, ptrW, 64);
            c += 66;
            ptrW += 64;
        }

        ptrW = (uint8_t*)waloff[kEnergy2];
        
        for (i = 0; i < 4096; i++)
        {
            if ((*ptrW) == 96) {
                *ptrW = 255; // -1?
            }
            
            ptrW++;
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

            energytile[val] = 175;
            word_9AB5B = 1;
        }
        tileInvalidate(kEnergy2, -1, -1);
    }
}

int nPlasmaTile = kTile4092;

#define kPlasmaWidth	320
#define kPlasmaHeight	80

void menu_DoPlasma()
{
    if (waloff[kTile4092] == 0)
    {
        tileCreate(kTile4092, kPlasmaWidth, kPlasmaHeight);

        memset((void*)waloff[kTile4092], 96, kPlasmaWidth*kPlasmaHeight);

        waloff[kTile4093] = (intptr_t)plasmaBuffer;
        memset(plasmaBuffer, 96, sizeof(plasmaBuffer));

        nSmokeLeft = 160 - tilesiz[kExhumedLogo].x / 2;
        nSmokeRight = nSmokeLeft + tilesiz[kExhumedLogo].x;

        tilesiz[kTile4093].x = kPlasmaWidth;
        tilesiz[kTile4093].y = kPlasmaHeight;

        nSmokeTop    = 40 - tilesiz[kExhumedLogo].y / 2;
        nSmokeBottom = nSmokeTop + tilesiz[kExhumedLogo].y - 1;

        //uint32_t t = time(0) << 16;
        //uint32_t t2 = time(0) | t;
        nRandom = timerGetTicksU64();

        for (int i = 0; i < 5; i++)
        {
            int logoWidth = tilesiz[kExhumedLogo].x;
#if 1
            plasma_C[i] = (nSmokeLeft + rand() % logoWidth) << 16;
            plasma_B[i] = (menu_RandomLong2() % 327680) + 0x10000;
#else
            int r = rand();
            int rand2 = menu_RandomLong2();

            __asm {
                mov		ebx, i
                mov		ecx, logoWidth
                mov     eax, r
                mov		edx, eax
                sar     edx, 31
                idiv    ecx

                add     edx, nSmokeLeft
                shl     edx, 16
                mov     ecx, 327680
                mov     plasma_C[ebx * 4], edx
                xor     edx, edx
                mov		eax, rand2
//				call    menu_RandomLong2
                div     ecx
                add     edx, 10000h
                mov     plasma_B[ebx * 4], edx
            };
#endif

            if (menu_RandomBit2()) {
                plasma_B[i] = -plasma_B[i];
            }

            plasma_A[i] = menu_RandomBit2();
        }
    }

    videoClearScreen(overscanindex);

    uint8_t *r_ebx = (uint8_t*)waloff[nPlasmaTile] + 81;
    uint8_t *r_edx = (uint8_t*)waloff[nPlasmaTile ^ 1] + 81; // flip between value of 4092 and 4093 with xor

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

    tileLoad(kExhumedLogo);

    for (int j = 0; j < 5; j++)
    {
        int pB = plasma_B[j];
        int pC = plasma_C[j];
        int badOffset =  (pC>>16) < nSmokeLeft || (pC>>16) >= nSmokeRight;

        uint8_t *ptr3 = (uint8_t*)(waloff[kExhumedLogo] + ((pC >> 16) - nSmokeLeft) * tilesiz[kExhumedLogo].y);

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
                if (al != 255 && al != 96) {
                    break;
                }
                
                nSmokeOffset++;
                ptr3++;
            }
        }
        else
        {
            nSmokeOffset = nSmokeBottom;

            ptr3 += tilesiz[kExhumedLogo].y - 1;

            while (nSmokeOffset > nSmokeTop)
            {
                uint8_t al = *ptr3;
                if (al != 255 && al != 96) {
                    break;
                }

                nSmokeOffset--;
                ptr3--;
            }
        }

        uint8_t *v28 = (uint8_t*)(80 * (plasma_C[j] >> 16) + waloff[nPlasmaTile]);
        v28[nSmokeOffset] = 175;
    }

    tileInvalidate(nPlasmaTile,-1,-1);

    overwritesprite(0,   0,  nPlasmaTile,  0, 2, kPalNormal);
    overwritesprite(160, 40, kExhumedLogo, 0, 3, kPalNormal);
    
    // flip between tile 4092 and 4093
    if (nPlasmaTile == kTile4092) {
        nPlasmaTile = kTile4093;
    }
    else if (nPlasmaTile == kTile4093) {
        nPlasmaTile = kTile4092;
    }

    // draw the fire urn/lamp thingies
    int dword_9AB5F = ((int)totalclock/16) & 3;

    overwritesprite(50,  150, kTile3512 + dword_9AB5F, 0, 3, kPalNormal);
    overwritesprite(270, 150, kTile3512 + ((dword_9AB5F + 2) & 3), 0, 3, kPalNormal);

    // TEMP
    int time = (int)totalclock + 4;
    while ((int)totalclock < time) {
        HandleAsync();
    }
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

    ClearAllKeys();
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

    // User has 12 seconds to do something on the map screen before loading the current level
    while (nIdleSeconds < 12)
    {
        HandleAsync();

        if (((int)totalclock - startTime) / kTimerTicks)
        {
            nIdleSeconds++;
            startTime = (int)totalclock;
        }

        int moveTimer = (int)totalclock;

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

                    overwritesprite(smokeX, smokeY, nTile, 0, 2, kPalNormal);
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
            moveTimer = (int)totalclock;
        }

        if (curYPos == destYPos)
        {
            if (KB_KeyDown[sc_UpArrow])
            {
                KB_KeyDown[sc_UpArrow] = 0;

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

            if (KB_KeyDown[sc_DownArrow])
            {
                KB_KeyDown[sc_DownArrow] = 0;

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

            if (KB_KeyDown[sc_Escape] || KB_KeyDown[sc_Space] || KB_KeyDown[sc_Return])
            {
                KB_KeyDown[sc_Escape] = 0;
                KB_KeyDown[sc_Return] = 0;
                KB_KeyDown[sc_Space] = 0;
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

            //curYPos += var_2C * (((int)totalclock - moveTimer) / 2);

            if (KB_KeyDown[sc_Escape] || KB_KeyDown[sc_Space] || KB_KeyDown[sc_Return])
            {
                if (var_2C < 8) {
                    var_2C *= 2;
                }

                KB_KeyDown[sc_Escape] = 0;
                KB_KeyDown[sc_Return] = 0;
                KB_KeyDown[sc_Space] = 0;
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

void menu_AdjustVolume()
{
    int nOption = 1;
    int var_8 = 0;

    while (1)
    {
        HandleAsync();

        menu_DoPlasma();

        overwritesprite(80, 50, kMenuMusicTile, (Sin((int)totalclock << 4) >> 9) * (nOption == 0), 2, kPalNormal);
        overwritesprite(55, 75, kMenuBlankTitleTile, 0, 2, kPalNormal);

        seq_DrawGunSequence(
            SeqOffsets[kSeqSlider], // eax
            gMusicVolume % 3, // pick one of 3 frames?
            (gMusicVolume >> 1) - 93, // ebx. must be x???
            -22,
            0,
            0);

        overwritesprite(80, 110, kMenuSoundFxTile, (Sin((int)totalclock << 4) >> 9) * (nOption == 1), 2, kPalNormal);
        overwritesprite(55, 135, kMenuBlankTitleTile, 0, 2, kPalNormal);

        seq_DrawGunSequence(
            SeqOffsets[kSeqSlider], 
            gFXVolume % 3, 
            (gFXVolume / 2) - 93,
            38,
            0,
            0);

        int y = (60 * nOption) + 38;

        overwritesprite(60,  y, kMenuCursorTile, 0, 2, kPalNormal);
        overwritesprite(206, y, kMenuCursorTile, 0, 10, kPalNormal);

        videoNextPage();

        if (KB_KeyDown[sc_Escape] || KB_KeyDown[sc_Return] || KB_KeyDown[sc_Space])
        {
            PlayLocalSound(StaticSound[kSound33], 0);
            KB_KeyDown[sc_Escape] = 0;
            KB_KeyDown[sc_Space]  = 0;
            KB_KeyDown[sc_Return] = 0;
            return;
        }

        if (KB_KeyDown[sc_UpArrow])
        {
            if (nOption > 0)
            {
                nOption--;
                PlayLocalSound(StaticSound[kSound35], 0);
            }

            KB_KeyDown[sc_UpArrow] = 0;
        }

        if (KB_KeyDown[sc_DownArrow])
        {
            if (nOption < 1)
            {
                nOption++;
                PlayLocalSound(StaticSound[kSound35], 0);
            }

            KB_KeyDown[sc_DownArrow] = 0;
        }

        if ((int)totalclock <= var_8) {
            continue;
        }

        var_8 = (int)totalclock + 5;

        if (KB_KeyDown[sc_LeftArrow])
        {
            switch (nOption)
            {
                case 0:
                {
                    if (gMusicVolume > 3) {
                        gMusicVolume -= 4;
                    }

// TODO				SetMusicVolume();
// TODO				setCDaudiovolume(gMusicVolume);
                    continue;
                }

                case 1:
                {
                    if (gFXVolume > 3) {
                        gFXVolume -= 4;
                    }

                    if (LocalSoundPlaying()) {
                        UpdateLocalSound();
                    }
                    else {
                        PlayLocalSound(StaticSound[kSound23], 0);
                    }
                    continue;
                }
            }
        }

        if (KB_KeyDown[sc_RightArrow])
        {
            switch (nOption)
            {
                case 0:
                {
                    if (gMusicVolume < 252) {
                        gMusicVolume += 4;
                    }

// TODO				SetMusicVolume();
// TODO				setCDaudiovolume(gMusicVolume);
                    continue;
                }

                case 1:
                {
                    if (gFXVolume < 252) {
                        gFXVolume += 4;
                    }

                    if (LocalSoundPlaying()) {
                        UpdateLocalSound();
                    }
                    else {
                        PlayLocalSound(StaticSound[kSound23], 0);
                    }
                    continue;
                }
            }
        }

        if (GetLocalSound() != 23) {
            continue;
        }
        else {
            StopLocalSound();
        }
    }
}

int menu_NewGameMenu()
{
    const char endMark = 0xF;
    char nameList[5][25];
    int nNameLen = sizeof(nameList);

    int nNameOffset = 0; // char index into slot name string

    //int nPages = numpages;

    int arg_3E = tilesiz[kMenuBlankTitleTile].x - 10;

    int nSlot = 0;

    FILE *fp = fopen(kSaveFileName, "rb");
    if (fp == NULL)
    {
        memset(nameList, 0, nNameLen);
        memset(&GameStats, 0, sizeof(GameStat));

        fp = fopen(kSaveFileName, "wb+");
        if (fp != NULL)
        {
            fwrite(nameList, nNameLen, 1, fp);
            fwrite(&GameStats, 75, 1, fp); //fwrite(&GameStats, 75, 5, fp); // CHECKME! the size
            fwrite(&endMark, sizeof(endMark), 1, fp);

            fclose(fp);
        }
    }
    else
    {
        int nRead = fread(nameList, 1, nNameLen, fp);
        if (nRead != nNameLen)
        {
            memset(nameList, 0, nNameLen);
        }

        fclose(fp);
    }

    //	while (1)
    {
        ClearAllKeys();

        while (1)
        {
            HandleAsync();
            menu_DoPlasma();

            int y = (tilesiz[kMenuBlankTitleTile].y - (tilesiz[kMenuBlankTitleTile].y / 2) / 2) + 65;
            rotatesprite(160 << 16, y << 16, 0x10000, 0, kMenuNewGameTile, 0, 0, 2, 0, 0, xdim, ydim);

            int edi = 0;

            int arg_4A = 90;
            int arg_4E = 98;

            // Loop #3
            for (int i = 0; i < 5; i++)
            {
                // CHECKME
                int8_t shade = ((Sin((int)totalclock << 4) >> 9) * (i == nSlot)) + ((i != nSlot) * 31);

                overwritesprite(55, arg_4A, kMenuBlankTitleTile, shade, 2, kPalNormal);
                myprintext(63, arg_4E, nameList[i], 0);

                arg_4E += 22;
                arg_4A += 22;

                edi++;
            }

            edi = nSlot * 22;

            // draw selection markers
            overwritesprite(35, edi + 78, kMenuCursorTile, 0, 2, kPalNormal);
            overwritesprite(233, edi + 78, kMenuCursorTile, 0, 10, kPalNormal);
            videoNextPage();

            //nPages--;
            //if (nPages > 0) {
            //    continue;
            //}

            if (KB_KeyDown[sc_Escape])
            {
                PlayLocalSound(StaticSound[kSound33], 0);
                KB_KeyDown[sc_Escape] = 0;
                return -1;
            }

            if (KB_KeyDown[sc_UpArrow])
            {
                PlayLocalSound(StaticSound[kSound35], 0);
                if (nSlot <= 0) {
                    nSlot = 4;
                }
                else {
                    nSlot--;
                }

                KB_KeyDown[sc_UpArrow] = 0;
                ClearAllKeys();
                continue;
            }

            if (KB_KeyDown[sc_DownArrow])
            {
                PlayLocalSound(StaticSound[kSound35], 0);
                if (nSlot >= 4) {
                    nSlot = 0;
                }
                else {
                    nSlot++;
                }

                KB_KeyDown[sc_DownArrow] = 0;
                ClearAllKeys();
                continue;
            }

            if (KB_KeyDown[sc_Return] || KB_KeyWaiting())
            {
                break;
            }
        }
    }

    PlayLocalSound(StaticSound[kSound33], 0);
    if (KB_KeyDown[sc_Return]) {
        ClearAllKeys();
    }

    char *pName = nameList[nSlot];
    int nNameLength = strlen(pName);

    memset(pName, 0, nNameLength);

    menu_DoPlasma();
    overwritesprite(55, (nSlot * 22) + 90, kMenuBlankTitleTile, 0, 2, kPalNormal);

    int arg_5A = 90;
    int arg_52 = 98;

    for (int i = 0; i < 5; i++)
    {
        overwritesprite(55, arg_5A, kMenuBlankTitleTile, (i != nSlot) * 31, 2, kPalNormal);
        myprintext(63, arg_52, nameList[i], 0);

        arg_52 += 22;
        arg_5A += 22;
    }

    int x = 35;
    int y = (nSlot * 22) + 78;

    while (1)
    {
        HandleAsync();

        overwritesprite(x, y, kMenuCursorTile, 0, 2, kPalNormal);
        overwritesprite(233, y, kMenuCursorTile, 0, 10, kPalNormal);
        videoNextPage();

        char ch = 0;

check_keys:
        if (KB_KeyWaiting())
        {
            HandleAsync();

            ch = KB_GetCh();
            if (!ch)
            {
                KB_GetCh();
                goto check_keys;
            }

            // handle key input
            if (ch == asc_Enter)
            {
                // loc_39ACA:
                nameList[nSlot][nNameOffset] = 0;

                PlayLocalSound(StaticSound[kSound33], 0);
                KB_KeyDown[sc_Return] = 0;

                if (nameList[nSlot][0] == 0) {
                    return -1;
                }

                if (nNameLength) // does the save slot already exist?
                {
                    menu_DoPlasma();
                    if (Query(2, 4, "Overwrite existing game?", "Y/N", 'Y', 13, 'N', 27) >= 2) {
                        return -1;
                    }
                }

                FILE *fp = fopen(kSaveFileName, "rb+");
                if (fp == NULL) {
                    return -1;
                }

                memset(&GameStats, 0, sizeof(GameStat));
                GameStats.nWeapons = 1;
                GameStats.nMap = 1;

                fwrite(nameList, sizeof(nameList), 1, fp);
                fseek(fp, sizeof(nameList), SEEK_SET);
                fseek(fp, nSlot * sizeof(GameStat), SEEK_CUR);
                fwrite(&GameStats, sizeof(GameStat), 1, fp);
                fclose(fp);
                return nSlot;
            }
            else
            {
                // Enter wasn't pressed
                PlayLocalSound(4, 0); // ??

                if (ch == asc_BackSpace)
                {
                    nameList[nSlot][nNameOffset] = 0;

                    if (nNameOffset > 0) {
                        nNameOffset--;
                    }

                    nameList[nSlot][nNameOffset] = 0;
                }
                else if (ch == asc_Escape)
                {
                    PlayLocalSound(StaticSound[kSound33], 0);
                    KB_ClearKeysDown();
                    KB_FlushKeyboardQueue();
                    KB_KeyDown[sc_Escape] = 0;
                    return -1;
                }
                else 
                {
                    // check if a slot name is being typed
                    if ((ch >= '0' && ch <= '9')
                    ||  (ch >= 'A' && ch <= 'Z')
                    ||  (ch >= 'a' && ch <= 'z')
                    ||  (ch == ' '))	
                    {
                        ch = toupper(ch);
                        if (nNameOffset < 24) // n chars per slot name
                        {
                            nameList[nSlot][nNameOffset] = ch;
                            nNameOffset++;
                            nameList[nSlot][nNameOffset] = '\0'; // null terminate in the new offset

                            int nLen = MyGetStringWidth(nameList[nSlot]);
                            if (nLen > arg_3E)
                            {
                                nNameOffset--;
                                nameList[nSlot][nNameOffset] = '\0';
                            }
                        }
                    }
                }
            }
        }

        // loc_399FD:
        menu_DoPlasma();

        int arg_5E = ((int)totalclock / 30) & 1;

        int y = 90;
        int arg_42 = 98;

        for (int i = 0; i < 5; i++)
        {
            overwritesprite(55, y, kMenuBlankTitleTile, (i != nSlot) * 31, 2, kPalNormal);
            int nTextWidth = myprintext(63, arg_42, nameList[i], 0);

            // flash a full-stop to show the current typing position
            if (arg_5E != 0 && nSlot == i)
            {
                myprintext(nTextWidth, arg_42, ".", 0);
            }

            arg_42 += 22;
            y += 22;
        }
    }
}

int menu_LoadGameMenu()
{
    char nameList[5][25];

    int nSlot = 0;

    FILE *fp = fopen(kSaveFileName, "rb");
    if (fp == NULL)
    {
        memset(nameList, 0, sizeof(nameList));
    }
    else
    {
        fread(nameList, sizeof(nameList), 1, fp);
        fclose(fp);
    }

    while (1)
    {
        menu_DoPlasma();

        HandleAsync();

        overwritesprite(80, 65, kMenuLoadGameTile, 0, 2, kPalNormal);

        int spriteY = 90;
        int textY = 98;
        
        for (int i = 0; i < kMaxSaveSlots; i++)
        {
            // TODO - shade flashing
            overwritesprite(55, spriteY, kMenuBlankTitleTile, 0, 2, kPalNormal);

            myprintext(63, textY, nameList[i], 0);
            textY += 22;
            spriteY += 22;
        }

        int y = (nSlot * 22) + 78;

        overwritesprite(35,  y, kMenuCursorTile, 0, 2, kPalNormal);
        overwritesprite(233, y, kMenuCursorTile, 0, 10, kPalNormal);
        videoNextPage();

        if (KB_KeyDown[sc_Escape])
        {
            PlayLocalSound(StaticSound[kSound33], 0);
            KB_KeyDown[sc_Escape] = 0;
            return -1;
        }

        if (KB_KeyDown[sc_UpArrow])
        {
            PlayLocalSound(StaticSound[kSound35], 0);
            if (nSlot > 0) {
                nSlot--;
            }
            else {
                nSlot = kMaxSaveSlots - 1;
            }

            KB_KeyDown[sc_UpArrow] = 0;
        }

        if (KB_KeyDown[sc_DownArrow]) // checkme - is 0x5b in disassembly
        {
            PlayLocalSound(StaticSound[kSound35], 0);
            if (nSlot < kMaxSaveSlots - 1) {
                nSlot++;
            }
            else {
                nSlot = 0;
            }

            KB_KeyDown[sc_DownArrow] = 0;
        }

        if (!KB_KeyDown[sc_Return]) {
            continue;
        }

        PlayLocalSound(StaticSound[kSound33], 0);
        KB_KeyDown[sc_Return] = 0;
        KB_ClearKeysDown();
        KB_FlushKeyboardQueue();

        if (nameList[nSlot][0] != '\0')
        {
            PlayLocalSound(StaticSound[33], 0);
            return nSlot;
        }

        PlayLocalSound(4, 0);
    }
}

void menu_ResetKeyTimer()
{
    keytimer = (int)totalclock + 2400;
}

void menu_GameLoad2(FILE *fp)
{
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

void menu_ResetZoom()
{
    zoomsize = 0;
    PlayLocalSound(StaticSound[kSound31], 0);
}

int menu_Menu(int nVal)
{
    GrabPalette();

    int var_1C = 0;

    videoSetViewableArea(0, 0, xdim - 1, ydim - 1);

    KB_KeyDown[sc_Escape] = 0;

    StopAllSounds();
    StopLocalSound();

    menu_ResetKeyTimer();

    KB_FlushKeyboardQueue();
    KB_ClearKeysDown();

    menu_ResetZoom();

    short ptr[5];
    memset(ptr, 1, sizeof(ptr));

    // disable new game and load game if in multiplayer?
    if (nNetPlayerCount)
    {
        ptr[1] = 0;
        ptr[0] = 0;
    }

    // denote which menu item we've currently got selected
    int nMenu = 0;

    while (1)
    {
        HandleAsync();

        // skip any disabled menu items so we're selecting the first active one
        while (!ptr[nMenu])
        {
            nMenu++;
            if (nMenu == 5) {
                nMenu = 0;
            }
        }

        // handle the menu zoom-in
        if (zoomsize < 0x10000)
        {
            zoomsize += 4096;
            if (zoomsize >= 0x10000) {
                zoomsize = 0x10000;
            }
        }

        // menu idle timer
        if (!nVal && (int)totalclock > keytimer) {
            return 9;
        }

        // loc_39F54:
        menu_DoPlasma();

        int y = 65 - tilesiz[kMenuNewGameTile].y / 2;

        // YELLOW loop - Draw the 5 menu options (NEW GAME, TRAINING etc)
        for (int j = 0; j < 5; j++)
        {
            int8_t shade;

            if (nMenu == j) { // currently selected menu item
                shade = Sin((int)totalclock << 4) >> 9;
            }
            else if (ptr[j]) {
                shade = 0;
            }
            else {
                shade = 25;
            }

            picanm[j + kMenuNewGameTile].xofs = 0;
            picanm[j + kMenuNewGameTile].yofs = 0;
            rotatesprite(160 << 16, (y + tilesiz[j + kMenuNewGameTile].y) << 16, zoomsize, 0, kMenuNewGameTile + j, shade, 0, 2, 0, 0, xdim, ydim);

            y += 22;
        }

        // tilesizx is 51
        // tilesizy is 33

        int markerY = (22 * nMenu) + 53;
        overwritesprite(62,       markerY, kMenuCursorTile, 0, 2, kPalNormal);
        overwritesprite(62 + 146, markerY, kMenuCursorTile, 0, 10, kPalNormal);

        videoNextPage();

        int l = 0; // edi

        // ORANGE loop
        for (l = 0; ; l++)
        {
            int nKey = nMenuKeys[l];
            if (!nKey) {
                break;
            }

            if (KB_KeyDown[nKey])
            {
                goto LABEL_21; // TEMP
            }
        }

        // loc_3A0A7
        while (KB_KeyDown[sc_Escape])
        {
            HandleAsync();

            PlayLocalSound(StaticSound[kSound33], 0);
            KB_KeyDown[sc_Escape] = 0;

            if (nVal)
            {
                StopAllSounds();
                PlayLocalSound(StaticSound[kSound33], 0);
                MySetView(nViewLeft, nViewTop, nViewRight, nViewBottom);
                return -1;
            }

            l = 4;
LABEL_21:

            menu_ResetKeyTimer();

            if (l != nMenu)
            {
                PlayLocalSound(StaticSound[kSound35], 0);
                KB_KeyDown[nMenuKeys[l]] = 0;
                nMenu = l;
            }
        }

        if (KB_KeyDown[sc_Space] || KB_KeyDown[sc_Return])
        {
            var_1C = 1;
        }
        else if (var_1C)
        {
            var_1C = 0;

            PlayLocalSound(StaticSound[kSound33], 0);

            switch (nMenu) // TODO - change var name?
            {
                case kMenuNewGame:
                {
                    if (nTotalPlayers > 1) {
                        menu_ResetZoom();
                        menu_ResetKeyTimer();
                        break;
                    }

                    SavePosition = menu_NewGameMenu();
                    if (SavePosition == -1) {
                        menu_ResetZoom();
                        menu_ResetKeyTimer();
                        break;
                    }

                    FadeOut(1);
                    StopAllSounds();

                    StopAllSounds();
                    PlayLocalSound(StaticSound[kSound33], 0);
                    MySetView(nViewLeft, nViewTop, nViewRight, nViewBottom);
                    return 1;
                }

                case kMenuLoadGame:
                {
                    if (nTotalPlayers > 1) {
                        menu_ResetZoom();
                        menu_ResetKeyTimer();
                        break;
                    }

                    SavePosition = menu_LoadGameMenu();

                    if (SavePosition == -1) {
                        menu_ResetZoom();
                        menu_ResetKeyTimer();
                        break;
                    }

                    StopAllSounds();

                    StopAllSounds();
                    PlayLocalSound(StaticSound[kSound33], 0);
                    MySetView(nViewLeft, nViewTop, nViewRight, nViewBottom);
                    return 2;
                }

                case kMenuTraining:
                {
                    if (nTotalPlayers > 1) {
                        menu_ResetZoom();
                        menu_ResetKeyTimer();
                        break;
                    }

                    StopAllSounds();
                    PlayLocalSound(StaticSound[kSound33], 0);
                    MySetView(nViewLeft, nViewTop, nViewRight, nViewBottom);
                    return 3;
                }

                case kMenuVolume:
                {
                    menu_AdjustVolume();
                    menu_ResetZoom();
                    menu_ResetKeyTimer();
                    break;
                }

                case kMenuQuitGame:
                {
                    StopAllSounds();
                    StopAllSounds();
                    PlayLocalSound(StaticSound[kSound33], 0);
                    MySetView(nViewLeft, nViewTop, nViewRight, nViewBottom);
                    return 0;
                }

                default: 
                    menu_ResetZoom();
                    menu_ResetKeyTimer();
                    break;
            }
        }

        if (KB_KeyDown[sc_UpArrow])
        {
            PlayLocalSound(StaticSound[kSound35], 0);
            if (nMenu <= 0) {
                nMenu = 4;
            }
            else {
                nMenu--;
            }

            KB_KeyDown[sc_UpArrow] = 0;
            menu_ResetKeyTimer();
        }

        if (KB_KeyDown[sc_DownArrow]) // FIXME - is this down arrow? value is '5B' in disassembly
        {
            PlayLocalSound(StaticSound[kSound35], 0);
            if (nMenu >= 4) {
                nMenu = 0;
            }
            else {
                nMenu++;
            }

            KB_KeyDown[sc_DownArrow] = 0;
            menu_ResetKeyTimer();
        }

        // TODO - change to #defines
        if (KB_KeyDown[0x5c]) {
            KB_KeyDown[0x5c] = 0;
        }

        if (KB_KeyDown[0x5d]) {
            KB_KeyDown[0x5d] = 0;
        }
    }

    return 0;// todo
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


// TODO - moveme
int LoadCinemaPalette(int nPal)
{
    nPal--;

    if (nPal < 0 || nPal >= kMaxCinemaPals) {
        return -2;
    }

    // original code strcpy'd into a buffer first...

    int hFile = kopen4load(cinpalfname[nPal], 1);
    if (hFile < 0) {
        return -2;
    }

    kread(hFile, cinemapal, sizeof(cinemapal));

    for (auto &c : cinemapal)
        c <<= 2;

    kclose(hFile);

    return nPal;
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

    paletteSetColorTable(ANIMPAL, cinemapal);
    videoSetPalette(0, ANIMPAL, 2+8);

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
    
    ClearAllKeys();
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

uint8_t AdvanceCinemaText()
{
    int var_1C = nCDTrackLength;
    int tmp = nHeight + nCrawlY > 0;

    if (tmp || nCDTrackLength && nCDTrackLength > 0)
    {
        nextclock = (int)totalclock + 14;

        if (tmp > 0)
        {
            short y = nCrawlY;
            int edi = 0;

            while (edi < linecount && y <= 199)
            {
                if (y >= -10) {
                    myprintext(nLeft[edi], y, gString[line + edi], 0);
                }

                edi++;
                y += 10;
            }

            nCrawlY--;
        }

        while (1)
        {
            HandleAsync();

            if (KB_KeyDown[sc_Escape] || KB_KeyDown[sc_Return] || KB_KeyDown[sc_Space]) {
                break;
            }

            if (var_1C || nCDTrackLength)
            {
                if (nextclock <= (int)totalclock) {
                    return kTrue;
                }
            }
            else
            {
                return kTrue;
            }
        }
    }

    return kFalse;
}

void DoCinemaText(short nVal)
{
    ReadyCinemaText(nVal);

    while (1)
    {
        overwritesprite(0, 0, cinematile, 0, 2, kPalNormal);

        uint8_t bContinue = AdvanceCinemaText();

        WaitVBL();
        videoNextPage();

        // TEMP
        int time = (int)totalclock + 8;
        while ((int)totalclock < time) {
            HandleAsync();
        }

        if (!bContinue) {
            return;
        }
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
            LoadCinemaPalette(1);
            cinematile = 3454;
            break;
        }

        case 1:
        {
            LoadCinemaPalette(2);
            cinematile = 3452;
            break;
        }

        case 2:
        {
            LoadCinemaPalette(3);
            cinematile = 3449;
            break;
        }

        case 3:
        {
            LoadCinemaPalette(4);
            cinematile = 3445;
            break;
        }

        case 4:
        {
            LoadCinemaPalette(5);
            cinematile = 3451;
            break;
        }

        case 5:
        {
            LoadCinemaPalette(6);
            cinematile = 3448;
            break;
        }

        case 6:
        {
            LoadCinemaPalette(7);
            cinematile = 3446;
            break;
        }
    }

    FadeOut(kFalse);
    StopAllSounds();
    NoClip();

    overwritesprite(0, 0, kMovieTile, 100, 2, kPalNormal);
    videoNextPage();

//	int386(16, (const union REGS *)&val, (union REGS *)&val)

    overwritesprite(0, 0, cinematile, 0, 2, kPalNormal);
    videoNextPage();

    CinemaFadeIn();
    ClearAllKeys();

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

            playCDtrack(edx + 2); // , 1);
        }

        DoCinemaText(ebx);
    }

    FadeOut(kTrue);

    overwritesprite(0, 0, 764, 100, 2, kPalNormal);
    videoNextPage();

    GrabPalette();
    Clip();
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
    short word_9ABD5[] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 5, 0, 0, 0, 0, 7, 0, 0, 0, 0, 6 };

    if (word_9ABD5[nLevel]) {
        GoToTheCinema(word_9ABD5[nLevel]);
    }
}

void DoFailedFinalScene()
{
    videoSetViewableArea(0, 0, xdim - 1, ydim - 1);

    if (CDplaying()) {
        fadecdaudio();
    }

    playCDtrack(9);
    FadeToWhite();

// TODO	GoToTheCinema(word_9ABFF);
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
    if (!KB_KeyWaiting() || (KB_GetCh() != 27)) {
        return kFalse;
    }
    
    return kTrue;
}

void DoStatic(int a, int b)
{
    RandomLong(); // nothing done with the result of this?

    tileLoad(kTileLoboLaptop);

    int v2 = 160 - a / 2;
    int v4 = 81  - b / 2;

    int var_18 = v2 + a;
    int v5 = v4 + b;

    uint8_t *pTile = (uint8_t*)(waloff[kTileLoboLaptop] + (200 * v2)) + v4;

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

    PlayLocalSound(StaticSound[kSound75], 0);

    tileLoad(kTileLoboLaptop);

    memcpy((void*)waloff[kTileLoboLaptop], (void*)waloff[kTileLoboLaptop], tilesiz[kTileLoboLaptop].x * tilesiz[kTileLoboLaptop].y);

    int var_24 = 16;
    int var_28 = 12;

    int nEndTime = (int)totalclock + 240;

    while (KB_KeyWaiting()) {
        KB_GetCh();
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
    }

//	loadtilelockmode = 1;
    tileLoad(kTileLoboLaptop);
//	loadtilelockmode = 0;

    // loc_3AD75

    do
    {
        HandleAsync();
LABEL_11:
        if (strlen(gString[nString]) == 0)
            break;

        int esi = nString;

        tileLoad(kTileLoboLaptop);

        while (strlen(gString[esi]) != 0)
            esi++;

        int ebp = esi;

        ebp -= nString;
        ebp = 81 - (ebp <<= 2);

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

            while (*nChar)
            {
                HandleAsync();

                if (*nChar != ' ') {
                    PlayLocalSound(StaticSound[kSound71], 0);
                }

                xPos += CopyCharToBitmap(*nChar, kTileLoboLaptop, xPos, ebp);
                nChar++;

                overwritesprite(0, 0, kTileLoboLaptop, 0, 2, kPalNormal);
                videoNextPage();

                WaitVBL();
                if (CheckForEscape())
                    goto LABEL_28;
            }

            ebp += 8;
        }

        nString++;

        KB_FlushKeyboardQueue();
        KB_ClearKeysDown();

        int v11 = kTimerTicks * (var_1C + 2) + (int)totalclock;

        do
        {
            HandleAsync();

            if (v11 <= (int)totalclock)
                goto LABEL_11;
        } while (!KB_KeyWaiting());
    } 
    while (KB_GetCh() != 27);

LABEL_28:
    PlayLocalSound(StaticSound[kSound75], 0);

    while (1)
    {
        HandleAsync();

        DoStatic(var_28, var_24);

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
