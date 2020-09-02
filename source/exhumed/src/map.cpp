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
#include <string.h>
#include "player.h"
#include "engine.h"
#include "exhumed.h"
#include "view.h"
#include "v_2ddrawer.h"

BEGIN_PS_NS


short bShowTowers = false;
int ldMapZoom;
int lMapZoom;

void MarkSectorSeen(short nSector);


void InitMap()
{
    show2dsector.Zero();
    memset(show2dwall,   0, sizeof(show2dwall));
    memset(show2dsprite, 0, sizeof(show2dsprite));

    ldMapZoom = 64;
    lMapZoom  = 1000;
}

void GrabMap()
{
    for (int i = 0; i < numsectors; i++) {
        MarkSectorSeen(i);
    }
}

void MarkSectorSeen(short nSector)
{
    if (!show2dsector[nSector])
    {
        show2dsector.Set(nSector);

        short startwall = sector[nSector].wallptr;
        short nWalls = sector[nSector].wallnum;
        short endwall = startwall + nWalls;

        while (startwall <= endwall)
        {
            show2dwall[startwall >> 3] = (1 << (startwall & 7)) | show2dwall[startwall >> 3];
            startwall++;
        }
    }
}

void drawoverheadmap(int cposx, int cposy, int czoom, short cang)
{
#ifndef __WATCOMC__ // FIXME - Won't compile on Watcom
    int xvect = sintable[(2048 - cang) & 2047] * czoom;
    int yvect = sintable[(1536 - cang) & 2047] * czoom;
    int xvect2 = mulscale(xvect, yxaspect, 16);
    int yvect2 = mulscale(yvect, yxaspect, 16);

    // draw player position arrow
    renderDrawLine(xdim << 11, (ydim << 11) - 20480, xdim << 11, (ydim << 11) + 20480, 24);
    renderDrawLine((xdim << 11) - 20480, ydim << 11, xdim << 11, (ydim << 11) - 20480, 24);
    renderDrawLine((xdim << 11) + 20480, ydim << 11, xdim << 11, (ydim << 11) - 20480, 24);

    short nPlayerSprite = PlayerList[nLocalPlayer].nSprite;

    int nPlayerZ = sprite[nPlayerSprite].z;

    for (int nSector = 0; nSector < numsectors; nSector++)
    {
        short startwall = sector[nSector].wallptr;
        short nWalls = sector[nSector].wallnum;
        short endwall = startwall + nWalls - 1;

        int nCeilZ = sector[nSector].ceilingz;
        int nFloorZ = sector[nSector].floorz;

        int nZVal = nFloorZ - nPlayerZ;
        if (nZVal < 0) {
            nZVal = -nZVal;
        }

        int var_10 = nZVal >> 13;
        if (var_10 > 12) {
            var_10 = 12;
        }

        var_10 = 111 - var_10;

        // int startwallB = startwall;

        for (int nWall = startwall; nWall <= endwall; nWall++)
        {
            short nextwall = wall[nWall].nextwall;

            if (nextwall >= 0)
            {
                if (show2dwall[nWall >> 3] & (1 << (nWall & 7)))
                {
                    if (nextwall <= nWall || (show2dwall[nextwall >> 3] & (1 << (nextwall & 7))) <= 0)
                    {
                        if (nCeilZ != sector[wall[nWall].nextsector].ceilingz ||
                            nFloorZ != sector[wall[nWall].nextsector].floorz ||
                            ((wall[nextwall].cstat | wall[nWall].cstat) & 0x30))
                        {
                            int ox = wall[nWall].x - cposx;
                            int oy = wall[nWall].y - cposy;

                            int x1 = mulscale(ox, xvect, 16) - mulscale(oy, yvect, 16);
                            int y1 = mulscale(oy, xvect2, 16) + mulscale(ox, yvect2, 16);

                            int nWall2 = wall[nWall].point2;
                            ox = wall[nWall2].x - cposx;
                            oy = wall[nWall2].y - cposy;
                            int x2 = mulscale(ox, xvect, 16) - mulscale(oy, yvect, 16);
                            int y2 = mulscale(oy, xvect2, 16) + mulscale(ox, yvect2, 16);

                            renderDrawLine(x1 + (xdim << 11), y1 + (ydim << 11), x2 + (xdim << 11), y2 + (ydim << 11), var_10);

                            /*
                            drawline256(
                                ((unsigned __int64)(v4 * (signed __int64)v12) >> 16)
                                - ((unsigned __int64)(v5 * (signed __int64)v13) >> 16)
                                + (xdim << 11),
                                ((unsigned __int64)(v42 * (signed __int64)v12) >> 16)
                                + ((unsigned __int64)(v43 * (signed __int64)v13) >> 16)
                                + (ydim << 11),
                                (build_xdim << 11)
                                + ((unsigned __int64)(v4 * (signed __int64)(*v14 - v31)) >> 16)
                                - ((unsigned __int64)(v5 * (signed __int64)(v14[1] - v30)) >> 16),
                                ydim << 11)
                                + ((unsigned __int64)(v43 * (signed __int64)(v14[1] - v30)) >> 16)
                                + ((unsigned __int64)(v42 * (signed __int64)(*v14 - v31)) >> 16),
                                v48);
                            */
                        }
                    }
                }
            }
        }
    }

//	int var_4C = 0;
//	int var_48 = 0;

    for (int nSector = 0; nSector < numsectors; nSector++)
    {
        int startwall = sector[nSector].wallptr;
        int nWalls = sector[nSector].wallnum;
        int endwall = startwall + nWalls - 1;

        int nFloorZ = sector[nSector].floorz;

        int nVal = nFloorZ - nPlayerZ;
        if (nVal < 0) {
            nVal = -nVal;
        }

        int var_14 = nVal >> 13;

        if (var_14 <= 15)
        {
            var_14 = 111 - var_14;

            for (int nWall = startwall; nWall <= endwall; nWall++)
            {
                if (wall[nWall].nextwall < 0)
                {
                    if (show2dwall[nWall >> 3] & (1 << (nWall & 7)))
                    {
                        if (tilesiz[wall[nWall].picnum].x && tilesiz[wall[nWall].picnum].y)
                        {
                            int ox = wall[nWall].x - cposx;
                            int oy = wall[nWall].y - cposy;
                            int x1 = mulscale(ox, xvect, 16) - mulscale(oy, yvect, 16);
                            int y1 = mulscale(oy, xvect2, 16) + mulscale(ox, yvect2, 16);

                            int nWall2 = wall[nWall].point2;
                            ox = wall[nWall2].x - cposx;
                            oy = wall[nWall2].y - cposy;
                            int x2 = mulscale(ox, xvect, 16) - mulscale(oy, yvect, 16);
                            int y2 = mulscale(oy, xvect2, 16) + mulscale(ox, yvect2, 16);

                            renderDrawLine(x1 + (xdim << 11), y1 + (ydim << 11), x2 + (xdim << 11), y2 + (ydim << 11), 24);

/*

                            v19 = *v17 - v31;
                            v20 = v17[1] - v30;
                            v21 = &wall[8 * *((_WORD *)v17 + 4)];

                            build_drawline256(
                                (build_xdim << 11)
                                + ((unsigned __int64)(v4 * (signed __int64)v19) >> 16)
                                - ((unsigned __int64)(v5 * (signed __int64)v20) >> 16),
                                (build_ydim << 11)
                                + ((unsigned __int64)(v42 * (signed __int64)v19) >> 16)
                                + ((unsigned __int64)(v43 * (signed __int64)v20) >> 16),
                                (build_xdim << 11)
                                + ((unsigned __int64)(v4 * (signed __int64)(*v21 - v31)) >> 16)
                                - ((unsigned __int64)(v5 * (signed __int64)(v21[1] - v30)) >> 16),
                                (build_ydim << 11)
                                + ((unsigned __int64)(v42 * (signed __int64)(*v21 - v31)) >> 16)
                                + ((unsigned __int64)(v43 * (signed __int64)(v21[1] - v30)) >> 16),
                                v46);
*/
                        }
                    }
                }
            }

            if (bShowTowers)
            {
                for (int nSprite = headspritestat[406]; nSprite != -1; nSprite = nextspritestat[nSprite])
                {
                    int ox = sprite[nSprite].x - cposx; // var_64
                    int oy = sprite[nSprite].y - cposx; // var_68

                    // int var_58 = mulscale(var_64, xvect, 16) - mulscale(var_68, yvect, 16);
                    int x1 = mulscale(ox, xvect, 16) - mulscale(oy, yvect, 16);
                    int y1 = mulscale(oy, xvect2, 16) + mulscale(ox, yvect2, 16);

                    //int var_58 = mulscale(var_64, xvect, 16) - mulscale(var_68, yvect, 16);
                    //int esi = mulscale(var_68, xvect2, 16) + mulscale(var_65, yvect2, 16)

                    //v25 = ((unsigned __int64)(v4 * (signed __int64)ox) >> 16)
                    //	- ((unsigned __int64)(v5 * (signed __int64)oy) >> 16);

                    //v26 = ((unsigned __int64)(v42 * (signed __int64)ox) >> 16)
                    //	+ ((unsigned __int64)(v43 * (signed __int64)oy) >> 16);

                    //v27 = v26 + 2048;
                    //v28 = v26 + 2048 + (ydim << 11);
                    //v26 -= 2048;

                    // v25 is x1
                    // v26 is y1
                    // v27 is y1 + 2048
                    // v28 is y1 + 2048 + (ydim << 1);

                    renderDrawLine(
                        x1 - 2048 + (xdim << 11),
                        y1 - 2048 + (ydim << 11),
                        x1 - 2048 + (xdim << 11),
                        y1 + 2048 + (ydim << 1),
                        170);

                    renderDrawLine(
                        x1 + (xdim << 11),
                        y1 + (ydim << 11),
                        x1 + (xdim << 11),
                        y1 + 2048 + (ydim << 11),
                        170);

                    renderDrawLine(
                        x1 + 2048 + (xdim << 11),
                        y1 + (ydim << 11),
                        x1 + 2048 + (xdim << 11),
                        y1 + 2048 + (ydim << 11),
                        170);
                }
            }
        }
    }
#endif
}

#ifdef _MSC_VER
#pragma warning(disable:4101) // this function produces a little bit too much noise
#endif

static void G_DrawOverheadMap(int32_t cposx, int32_t cposy, int32_t czoom, int16_t cang)
{
    int32_t i, j, k, x1, y1, x2=0, y2=0, ox, oy;
    int32_t z1, z2, startwall, endwall;
    int32_t xvect, yvect, xvect2, yvect2;
    char col;
    uwallptr_t wal, wal2;

    int32_t tmpydim = (xdim*5)/8;

    renderSetAspect(65536, divscale16(tmpydim*320, xdim*200));

    xvect = sintable[(-cang)&2047] * czoom;
    yvect = sintable[(1536-cang)&2047] * czoom;
    xvect2 = mulscale16(xvect, yxaspect);
    yvect2 = mulscale16(yvect, yxaspect);

    //renderDisableFog();

    // draw player position arrow
    renderDrawLine(xdim << 11, (ydim << 11) - 20480, xdim << 11, (ydim << 11) + 20480, 24);
    renderDrawLine((xdim << 11) - 20480, ydim << 11, xdim << 11, (ydim << 11) - 20480, 24);
    renderDrawLine((xdim << 11) + 20480, ydim << 11, xdim << 11, (ydim << 11) - 20480, 24);

    short nPlayerSprite = PlayerList[nLocalPlayer].nSprite;

    int nPlayerZ = sprite[nPlayerSprite].z;

    //Draw red lines
    for (i=numsectors-1; i>=0; i--)
    {
        if (!gFullMap && !show2dsector[i]) continue;

        startwall = sector[i].wallptr;
        endwall = sector[i].wallptr + sector[i].wallnum;

        z1 = sector[i].ceilingz;
        z2 = sector[i].floorz;

        for (j=startwall, wal=(uwallptr_t)&wall[startwall]; j<endwall; j++, wal++)
        {
            k = wal->nextwall;
            if (k < 0) continue;

            if (sector[wal->nextsector].ceilingz == z1 && sector[wal->nextsector].floorz == z2)
                    if (((wal->cstat|wall[wal->nextwall].cstat)&(16+32)) == 0) continue;

            if (automapMode == am_full)
                col = 111;
            else
                col = 111 - min(klabs(z2 - nPlayerZ) >> 13, 12);

            ox = wal->x-cposx;
            oy = wal->y-cposy;
            x1 = dmulscale16(ox, xvect, -oy, yvect)+(xdim<<11);
            y1 = dmulscale16(oy, xvect2, ox, yvect2)+(ydim<<11);

            wal2 = (uwallptr_t)&wall[wal->point2];
            ox = wal2->x-cposx;
            oy = wal2->y-cposy;
            x2 = dmulscale16(ox, xvect, -oy, yvect)+(xdim<<11);
            y2 = dmulscale16(oy, xvect2, ox, yvect2)+(ydim<<11);

            renderDrawLine(x1, y1, x2, y2, col);
        }
    }

    //Draw white lines
    for (i=numsectors-1; i>=0; i--)
    {
        if (!gFullMap && !show2dsector[i]) continue;

        startwall = sector[i].wallptr;
        endwall = sector[i].wallptr + sector[i].wallnum;
        z2 = sector[i].floorz;

        if (automapMode == am_full)
        {
            col = 111;
        }
        else
        {
            col = klabs(z2 - nPlayerZ) >> 13;
            if (col > 15)
                continue;
            col = 111 - col;
        }

        k = -1;
        for (j=startwall, wal=(uwallptr_t)&wall[startwall]; j<endwall; j++, wal++)
        {
            if (wal->nextwall >= 0) continue;

            if (!tileGetTexture(wal->picnum)->isValid()) continue;

            if (j == k)
            {
                x1 = x2;
                y1 = y2;
            }
            else
            {
                ox = wal->x-cposx;
                oy = wal->y-cposy;
                x1 = dmulscale16(ox, xvect, -oy, yvect)+(xdim<<11);
                y1 = dmulscale16(oy, xvect2, ox, yvect2)+(ydim<<11);
            }

            k = wal->point2;
            wal2 = (uwallptr_t)&wall[k];
            ox = wal2->x-cposx;
            oy = wal2->y-cposy;
            x2 = dmulscale16(ox, xvect, -oy, yvect)+(xdim<<11);
            y2 = dmulscale16(oy, xvect2, ox, yvect2)+(ydim<<11);

            renderDrawLine(x1, y1, x2, y2, col);
        }
    }

    //renderEnableFog();

    videoSetCorrectedAspect();

#if 0
    for (TRAVERSE_CONNECT(p))
    {
        if (automapFollow && p == screenpeek) continue;

        auto const pPlayer = &ps[p];
        auto const pSprite = (uspriteptr_t)&sprite[pPlayer->i];

        ox = pSprite->x - cposx;
        oy = pSprite->y - cposy;
        daang = (pSprite->ang - cang) & 2047;
        if (p == screenpeek)
        {
            ox = 0;
            oy = 0;
            daang = 0;
        }
        x1 = mulscale16(ox, xvect) - mulscale16(oy, yvect);
        y1 = mulscale16(oy, xvect2) + mulscale16(ox, yvect2);

        if (p == screenpeek || GTFLAGS(GAMETYPE_OTHERPLAYERSINMAP))
        {
            if (pSprite->xvel > 16 && pPlayer->on_ground)
                i = APLAYERTOP+(((int32_t) leveltime>>4)&3);
            else
                i = APLAYERTOP;

            i = VM_OnEventWithReturn(EVENT_DISPLAYOVERHEADMAPPLAYER, pPlayer->i, p, i);

            if (i < 0)
                continue;

            j = klabs(pPlayer->truefz - pPlayer->pos.z) >> 8;
            j = mulscale16(czoom * (pSprite->yrepeat + j), yxaspect);

            if (j < 22000) j = 22000;
            else if (j > (65536<<1)) j = (65536<<1);

            rotatesprite_win((x1<<4)+(xdim<<15), (y1<<4)+(ydim<<15), j, daang, i, pSprite->shade,
                P_GetOverheadPal(pPlayer), 0);
        }
    }
#endif
}

void UpdateMap()
{
    if (sector[initsect].ceilingpal != 3 || (nPlayerTorch[nLocalPlayer] != 0)) {
        MarkSectorSeen(initsect);
    }
}

void DrawMap()
{
    if (!nFreeze && automapMode != am_off) {
        //drawoverheadmap(initx, inity, lMapZoom, inita);
        if (automapMode == am_full)
        {
            twod->ClearScreen();
            renderDrawMapView(initx, inity, lMapZoom, inita);
        }
        G_DrawOverheadMap(initx, inity, lMapZoom, inita);
    }
}
END_PS_NS
