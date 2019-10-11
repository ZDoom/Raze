#include "compat.h"
#include "keyboard.h"
#include "control.h"
#include "engine.h"
#include "config.h"
#include "names.h"
#include "view.h"
#include "status.h"
#include "exhumed.h"
#include "player.h"
#include "snake.h"
#include "gun.h"
#include "light.h"
#include "init.h"
#include "menu.h"
#include "keyboard.h"
#include "cd.h"
#include "cdaudio.h"
#include "typedefs.h"
#include "move.h"
#include "sound.h"
#include "engine.h"
#include "trigdat.h"
#include "runlist.h"
#include <string.h>

short bSubTitles = kTrue;

int zbob;

short nDestVertPan[kMaxPlayers] = { 0 };
short dVertPan[kMaxPlayers];
short nVertPan[kMaxPlayers];
int nCamerax;
int nCameray;
int nCameraz;

short bTouchFloor;

short nQuake[kMaxPlayers] = { 0 };

short nChunkTotal = 0;

short nCameraa;
short nCamerapan;
short nViewTop;
short bClip = kFalse;
short nViewBottom;
short nViewRight;
short besttarget;
short nViewLeft;
short bCamera = kFalse;

short nViewy;

int viewz;

short enemy;

short nEnemyPal = 0;


void InitView()
{
    screensize = (short)xdim;
}

// NOTE - not to be confused with Ken's analyzesprites()
static void analyzesprites()
{
    short nPlayerSprite = PlayerList[nLocalPlayer].nSprite;

    int var_38 = 20;
    int var_2C = 30000;

    besttarget = -1;

    int x = sprite[nPlayerSprite].x;
    int y = sprite[nPlayerSprite].y;

    int z = sprite[nPlayerSprite].z - (GetSpriteHeight(nPlayerSprite) / 2);

    short nSector = sprite[nPlayerSprite].sectnum;

    int nAngle = (2048 - sprite[nPlayerSprite].ang) & kAngleMask;

    int edi = spritesortcnt;
    int nSprite = spritesortcnt - 1;

//	int var_20 = var_24; 

    while (1)
    {
        edi--;
        if (edi < 0)
        {
            if (besttarget != -1)
            {
                nCreepyTimer = 450;

                if (!cansee(x, y, z, nSector, sprite[besttarget].x, sprite[besttarget].y, sprite[besttarget].z - GetSpriteHeight(besttarget), sprite[besttarget].sectnum))
                {
                    besttarget = -1;
                }
            }

            return;
        }
        else
        {
            int nSprite2 = tsprite[nSprite].owner;

            if (sprite[nSprite2].statnum > 0)
            {
                runlist_SignalRun(sprite[nSprite2].lotag - 1, edi | 0x90000);

                if ((sprite[nSprite2].statnum < 150) && (sprite[nSprite2].cstat & 0x101) && (nSprite2 != nPlayerSprite))
                {
                    int xval = sprite[nSprite2].x - x;
                    int yval = sprite[nSprite2].y - y;

                    int var_40 = xval * Sin(nAngle + 512);
                    int var_44 = sintable[nAngle];


                    int edx = ((Sin(nAngle + 512) * yval) + (xval * var_44)) >> 14;


                    int eax = (var_40 - (yval * var_44)) >> 14;


                    if (eax < 0) {
                        eax = -eax;
                    }

                    int ebx = eax;
                    int ecx = eax;

                    if (eax)
                    {
                        eax = edx;

                        if (eax < 0) {
                            eax = -eax;
                        }

                        edx = (eax * 32) / ebx;

                        if (ebx >= 1000 || ebx >= var_2C || edx >= 10)
                        {
                            // loc_170A1
                            if (ecx < 30000)
                            {
                                eax = var_38 - edx;
                                if (eax <= 3)
                                {
                                    if (ecx < var_2C)
                                    {
                                        if (eax < 0) {
                                            eax = -eax;
                                        }

                                        if (eax < 5)
                                        {
                                            var_38 = edx;
                                            var_2C = ecx;
                                            besttarget = nSprite2;
                                        }
                                    }
                                }
                                else
                                {
                                    var_38 = edx;
                                    var_2C = ecx;
                                    besttarget = nSprite2;
                                }
                            }
                        }
                        else
                        {
                            besttarget = nSprite2;
                            var_38 = edx;
                            var_2C = ebx;
                        }
                    }
                }
            }
        }

        nSprite--;
    }
}

void ResetView()
{
    uint8_t blankPal[768];
    memset(blankPal, 0, sizeof(blankPal));
    MySetPalette(blankPal);

    EraseScreen(0);

    // FIXME
#ifdef __WATCOMC__
    setgamemode(2, 320, 200);
#else
    videoSetGameMode(gSetup.fullscreen, gSetup.xdim, gSetup.ydim, gSetup.bpp, 0);
#endif

    MySetPalette(blankPal);

    DoOverscanSet(overscanindex);
    EraseScreen(overscanindex);

    MySetPalette(kenpal);

    LoadStatus();
}

void SetView1()
{
}

void FlushMessageLine()
{
    int tileX = tilesiz[nBackgroundPic].x;
    int nTileOffset = 0;

    int xPos = 0;

    while (xPos < xdim)
    {
        overwritesprite(xPos, 0, nBackgroundPic + nTileOffset, -32, 0, kPalNormal);

        nTileOffset = nTileOffset == 0;

        xPos += tileX;
    }
}

void RefreshBackground()
{
    int nTileOffset = 0;
    int tileX = tilesiz[nBackgroundPic].x;
    int tileY = tilesiz[nBackgroundPic].y;

    videoSetViewableArea(0, 0, xdim - 1, ydim - 1);

    MaskStatus();

    for (int y = 0; y < xdim; y += tileY)
    {
        for (int x = 0; x < ydim; x += tileX)
        {
            overwritesprite(x, y, nBackgroundPic + nTileOffset, -32, 0, kPalNormal);
            if (nTileOffset == 0) {
                nTileOffset = 1;
            }
            else {
                nTileOffset = 0;
            }
        }
    }

    videoSetViewableArea(nViewLeft, nViewTop, nViewRight, nViewBottom);
}

void MySetView(int x1, int y1, int x2, int y2)
{
    if (!bFullScreen) {
        MaskStatus();
    }

    nViewLeft = x1;
    nViewTop = y1;
    nViewRight = x2;
    nViewBottom = y2;

    videoSetViewableArea(x1, y1, x2, y2);

    nViewy = y1;
}

// unused function
void TestLava()
{
}

void DrawView()
{
    int playerX;
    int playerY;
    int playerZ;
    short nSector;
    int nAngle;
    short pan;

#if 0
    if (bgpages <= 0)
    {
        if (textpages > 0)
        {
            textpages--;
            FlushMessageLine();
        }
    }
    else
    {
        RefreshBackground();
        bgpages--;
    }
#else
    FlushMessageLine();
    RefreshBackground();
#endif

    if (!bFullScreen) {
        MaskStatus();
    }

    zbob = Sin(2 * bobangle) >> 3;

    int nPlayerSprite = PlayerList[nLocalPlayer].nSprite;

    if (nSnakeCam >= 0)
    {
        int nSprite = SnakeList[nSnakeCam].nSprites[0];

        playerX = sprite[nSprite].x;
        playerY = sprite[nSprite].y;
        playerZ = sprite[nSprite].z;
        nSector = sprite[nSprite].sectnum;
        nAngle = sprite[nSprite].ang;

        SetGreenPal();
        UnMaskStatus();

        enemy = SnakeList[nSnakeCam].nEnemy;

        if (enemy <= -1 || totalmoves & 1)
        {
            nEnemyPal = -1;
        }
        else
        {
            nEnemyPal = sprite[enemy].pal;
            sprite[enemy].pal = 5;
        }
    }
    else
    {
        playerX = sprite[nPlayerSprite].x;
        playerY = sprite[nPlayerSprite].y;
        playerZ = sprite[nPlayerSprite].z + eyelevel[nLocalPlayer];
        nSector = nPlayerViewSect[nLocalPlayer];
        nAngle = sprite[nPlayerSprite].ang;
    }

    nCameraa = nAngle;

    if (!bCamera || nFreeze)
    {
        if (nSnakeCam >= 0)
        {
            pan = 92;
            viewz = playerZ;
        }
        else
        {
            viewz = playerZ + nQuake[nLocalPlayer];
            int floorZ = sector[sprite[nPlayerSprite].sectnum].floorz;

            pan = nVertPan[nLocalPlayer];

            if (viewz > floorZ)
                viewz = floorZ;

            nCameraa += (nQuake[nLocalPlayer] >> 7) % 31;
        }
    }
    else
    {
        clipmove_old((int32_t*)&playerX, (int32_t*)&playerY, (int32_t*)&playerZ, &nSector,
            -2000 * Sin(inita + 512), 
            -2000 * Sin(inita),
            4, 0, 0, CLIPMASK1);

        pan = 92;
        viewz = playerZ;
    }

    nCamerax = playerX;
    nCameray = playerY;
    nCameraz = playerZ;

    int Z = sector[nSector].ceilingz + 256;
    if (Z <= viewz)
    {
        Z = sector[nSector].floorz - 256;

        if (Z < viewz)
            viewz = Z;
    }
    else {
        viewz = Z;
    }

    nCamerapan = pan;

    if (nFreeze == 2 || nFreeze == 1)
    {
        nSnakeCam = -1;
        videoSetViewableArea(0, 0, xdim - 1, ydim - 1);
        UnMaskStatus();
    }

    if (nFreeze != 3)
    {
        int const viewingRange = viewingrange;

        if (r_usenewaspect)
        {
            newaspect_enable = 1;
            videoSetCorrectedAspect();
        }

        drawrooms(nCamerax, nCameray, viewz, nCameraa, nCamerapan, nSector);
        analyzesprites();
        renderDrawMasks();

        if (r_usenewaspect)
        {
            newaspect_enable = 0;
            renderSetAspect(viewingRange, tabledivide32_noinline(65536 * ydim * 8, xdim * 5));
        }

        if (nFreeze)
        {
            nSnakeCam = -1;

            if (nFreeze == 2)
            {
                if (nHeadStage == 4)
                {
                    nHeadStage = 5;

                    sprite[nPlayerSprite].cstat |= 0x8000;

                    int ang2 = nCameraa - sprite[nPlayerSprite].ang;
                    if (ang2 < 0)
                        ang2 = -ang2;
                    
                    if (ang2 > 10)
                    {
                        inita -= (ang2 >> 3);
                        return;
                    }

                    if (bSubTitles)
                    {
                        if (levelnum == 1)
                            ReadyCinemaText(1);
                        else 
                            ReadyCinemaText(5);
                    }
                }
                else
                {
                    if ((bSubTitles && !AdvanceCinemaText()) || KB_KeyDown[sc_Escape] || KB_KeyDown[sc_Return] || KB_KeyDown[sc_Space])
                    {
                        levelnew = levelnum + 1;

                        if (CDplaying())
                            fadecdaudio();
                    }

                    videoSetViewableArea(nViewLeft, nViewTop, nViewRight, nViewBottom);
                }
            }
        }
        else
        {
            if (nSnakeCam < 0)
            {
                DrawWeapons();
                DrawStatus();
            }
            else
            {
                RestoreGreenPal();
                if (nEnemyPal > -1) {
                    sprite[enemy].pal = nEnemyPal;
                }

                if (!bFullScreen) {
                    MaskStatus();
                }
            }
        }
    }
    else
    {
        videoClearScreen(overscanindex);
        DrawStatus();
    }

    flash = 0;
}

void NoClip()
{
    videoSetViewableArea(0, 0, xdim - 1, ydim - 1);

    bClip = kFalse;
}

void Clip()
{
    videoSetViewableArea(nViewLeft, nViewTop, nViewRight, nViewBottom);
    if (!bFullScreen) {
        MaskStatus();
    }

    bClip = kTrue;
}
