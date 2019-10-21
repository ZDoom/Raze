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

    spritetype *pPlayerSprite = &sprite[nPlayerSprite];

    besttarget = -1;

    int x = pPlayerSprite->x;
    int y = pPlayerSprite->y;

    int z = pPlayerSprite->z - (GetSpriteHeight(nPlayerSprite) / 2);

    short nSector = pPlayerSprite->sectnum;

    int nAngle = (2048 - pPlayerSprite->ang) & kAngleMask;

    int nTSprite;
    tspritetype *pTSprite;

//	int var_20 = var_24; 

    for (nTSprite = spritesortcnt-1, pTSprite = &tsprite[nTSprite]; nTSprite >= 0; nTSprite--, pTSprite--)
    {
        int nSprite = pTSprite->owner;
        spritetype *pSprite = &sprite[nSprite];

        if (pTSprite->sectnum >= 0)
        {
            sectortype *pSector = &sector[pTSprite->sectnum];
            int nSectShade = (pSector->ceilingstat & 1) ? pSector->ceilingshade : pSector->floorshade;
            int nShade = pTSprite->shade + nSectShade + 6;
            pTSprite->shade = clamp(nShade, -128, 127);
        }

        if (pSprite->statnum > 0)
        {
            runlist_SignalRun(pSprite->lotag - 1, nTSprite | 0x90000);

            if ((pSprite->statnum < 150) && (pSprite->cstat & 0x101) && (nSprite != nPlayerSprite))
            {
                int xval = pSprite->x - x;
                int yval = pSprite->y - y;

                int vcos = Cos(nAngle);
                int vsin = Sin(nAngle);


                int edx = ((vcos * yval) + (xval * vsin)) >> 14;


                int ebx = klabs(((vcos * xval) - (yval * vsin)) >> 14);

                if (!ebx)
                    continue;

                edx = (klabs(edx) * 32) / ebx;
                if (ebx < 1000 && ebx < var_2C && edx < 10)
                {
                    besttarget = nSprite;
                    var_38 = edx;
                    var_2C = ebx;
                }
                else if (ebx < 30000)
                {
                    int t = var_38 - edx;
                    if (t > 3 || (ebx < var_2C && klabs(t) < 5))
                    {
                        var_38 = edx;
                        var_2C = ebx;
                        besttarget = nSprite;
                    }
                }
            }
        }
    }
    if (besttarget != -1)
    {
        spritetype *pTarget = &sprite[besttarget];

        nCreepyTimer = 450;

        if (!cansee(x, y, z, nSector, pTarget->x, pTarget->y, pTarget->z - GetSpriteHeight(besttarget), pTarget->sectnum))
        {
            besttarget = -1;
        }
    }
}

void ResetView()
{
    videoSetGameMode(gSetup.fullscreen, gSetup.xdim, gSetup.ydim, gSetup.bpp, 0);
    DoOverscanSet(overscanindex);
    EraseScreen(overscanindex);
    memcpy(curpalettefaded, curpalette, sizeof(curpalette));
    videoUpdatePalette(0, 256);
#ifdef USE_OPENGL
    videoTintBlood(0, 0, 0);
#endif

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

    for (int y = 0; y < ydim; y += tileY)
    {
        for (int x = 0; x < xdim; x += tileX)
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
