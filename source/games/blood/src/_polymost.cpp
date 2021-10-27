#include "polymost.h"

BEGIN_BLD_NS


// leftover bits needed to keep Polymost running through the transition.
// This is mainly the game side part of the portal renderer.

void collectTSpritesForPortal(int x, int y, int i, int interpolation)
{
    int nSector = mirror[i].link;
    int nSector2 = mirror[i].wallnum;
    int nSprite;
    SectIterator it(nSector);
    while ((nSprite = it.NextIndex()) >= 0)
    {
        spritetype* pSprite = &sprite[nSprite];
        if (pSprite == gView->pSprite)
            continue;
        int top, bottom;
        GetSpriteExtents(pSprite, &top, &bottom);
        int zCeil, zFloor;
        getzsofslope(nSector, pSprite->x, pSprite->y, &zCeil, &zFloor);
        if (pSprite->statnum == kStatDude && (top < zCeil || bottom > zFloor))
        {
            int j = i;
            if (mirror[i].type == 2)
                j++;
            else
                j--;
            int dx = mirror[j].dx;
            int dy = mirror[j].dy;
            int dz = mirror[j].dz;
            if (pm_spritesortcnt < MAXSPRITESONSCREEN)
            {
                tspritetype* pTSprite = &pm_tsprite[pm_spritesortcnt++];
                *pTSprite = {};
                pTSprite->type = pSprite->type;
                pTSprite->index = pSprite->index;
                pTSprite->sectnum = nSector2;
                pTSprite->x = pSprite->x + dx;
                pTSprite->y = pSprite->y + dy;
                pTSprite->z = pSprite->z + dz;
                pTSprite->ang = pSprite->ang;
                pTSprite->picnum = pSprite->picnum;
                pTSprite->shade = pSprite->shade;
                pTSprite->pal = pSprite->pal;
                pTSprite->xrepeat = pSprite->xrepeat;
                pTSprite->yrepeat = pSprite->yrepeat;
                pTSprite->xoffset = pSprite->xoffset;
                pTSprite->yoffset = pSprite->yoffset;
                pTSprite->cstat = pSprite->cstat;
                pTSprite->statnum = kStatDecoration;
                pTSprite->owner = pSprite->index;
                pTSprite->extra = pSprite->extra;
                pTSprite->flags = pSprite->hitag | 0x200;
                pTSprite->x = dx + interpolatedvalue(pSprite->ox, pSprite->x, interpolation);
                pTSprite->y = dy + interpolatedvalue(pSprite->oy, pSprite->y, interpolation);
                pTSprite->z = dz + interpolatedvalue(pSprite->oz, pSprite->z, interpolation);
                pTSprite->ang = pSprite->interpolatedang(interpolation);

                int nAnim = 0;
                switch (picanm[pTSprite->picnum].extra & 7)
                {
                case 1:
                {
                    int dX = x - pTSprite->x;
                    int dY = y - pTSprite->y;
                    RotateVector(&dX, &dY, 128 - pTSprite->ang);
                    nAnim = GetOctant(dX, dY);
                    if (nAnim <= 4)
                    {
                        pTSprite->cstat &= ~4;
                    }
                    else
                    {
                        nAnim = 8 - nAnim;
                        pTSprite->cstat |= 4;
                    }
                    break;
                }
                case 2:
                {
                    int dX = x - pTSprite->x;
                    int dY = y - pTSprite->y;
                    RotateVector(&dX, &dY, 128 - pTSprite->ang);
                    nAnim = GetOctant(dX, dY);
                    break;
                }
                }
                while (nAnim > 0)
                {
                    pTSprite->picnum += picanm[pTSprite->picnum].num + 1;
                    nAnim--;
                }

                pm_spritesortcnt++;
            }
        }
    }

}

void processSpritesOnOtherSideOfPortal(int x, int y, int interpolation)
{
    if (pm_spritesortcnt == 0) return;
    int nViewSprites = pm_spritesortcnt-1;
    for (int nTSprite = nViewSprites; nTSprite >= 0; nTSprite--)
    {
        tspritetype *pTSprite = &pm_tsprite[nTSprite];
        pTSprite->xrepeat = pTSprite->yrepeat = 0;
    }
    for (int i = mirrorcnt-1; i >= 0; i--)
    {
        int nTile = 4080+i;
        if (testgotpic(nTile))
        {
            if (mirror[i].type == 1 || mirror[i].type == 2)
            {
                collectTSpritesForPortal(x, y, i, interpolation);
            }
        }
    }
}


void render3DViewPolymost(int nSectnum, int cX, int cY, int cZ, binangle cA, fixedhoriz cH)
{
    int yxAspect = yxaspect;
    int viewingRange = viewingrange;
    videoSetCorrectedAspect();

    int v1 = xs_CRoundToInt(double(viewingrange) * tan(r_fov * (pi::pi() / 360.)));

    renderSetAspect(v1, yxaspect);


    int ceilingZ, floorZ;
    getzsofslope(nSectnum, cX, cY, &ceilingZ, &floorZ);
    if (cZ >= floorZ)
    {
        cZ = floorZ - (gUpperLink[nSectnum] >= 0 ? 0 : (8 << 8));
    }
    if (cZ <= ceilingZ)
    {
        cZ = ceilingZ + (gLowerLink[nSectnum] >= 0 ? 0 : (8 << 8));
    }
    cH = q16horiz(ClipRange(cH.asq16(), gi->playerHorizMin(), gi->playerHorizMax()));
RORHACK:
    bool ror_status[16];
    for (int i = 0; i < 16; i++)
        ror_status[i] = testgotpic(4080 + i);
    fixed_t deliriumPitchI = interpolatedvalue(IntToFixed(deliriumPitchO), IntToFixed(deliriumPitch), gInterpolate);
    DrawMirrors(cX, cY, cZ, cA.asq16(), cH.asq16() + deliriumPitchI, int(gInterpolate), gViewIndex);
    int bakCstat = gView->pSprite->cstat;
    if (gViewPos == 0)
    {
        gView->pSprite->cstat |= 32768;
    }
    else
    {
        gView->pSprite->cstat |= 514;
    }

    renderDrawRoomsQ16(cX, cY, cZ, cA.asq16(), cH.asq16() + deliriumPitchI, nSectnum);
    viewProcessSprites(pm_tsprite, pm_spritesortcnt, cX, cY, cZ, cA.asbuild(), int(gInterpolate));
    bool do_ror_hack = false;
    for (int i = 0; i < 16; i++)
        if (ror_status[i] != testgotpic(4080 + i))
            do_ror_hack = true;
    if (do_ror_hack)
    {
        gView->pSprite->cstat = bakCstat;
        pm_spritesortcnt = 0;
        goto RORHACK;
    }
    setPortalFlags(1);
    int nSpriteSortCnt = pm_spritesortcnt;
    renderDrawMasks();
    pm_spritesortcnt = nSpriteSortCnt;
    setPortalFlags(0);
    processSpritesOnOtherSideOfPortal(cX, cY, int(gInterpolate));
    renderDrawMasks();
    gView->pSprite->cstat = bakCstat;

}

// hack the portal planes with the sky flag for rendering. Only Polymost needs this hack.
void setPortalFlags(char mode)
{
    for (int i = mirrorcnt - 1; i >= 0; i--)
    {
        int nTile = 4080 + i;
        if (testgotpic(nTile))
        {
            switch (mirror[i].type)
            {
            case 1:
                if (mode)
                    sector[mirror[i].wallnum].ceilingstat |= 1;
                else
                    sector[mirror[i].wallnum].ceilingstat &= ~1;
                break;
            case 2:
                if (mode)
                    sector[mirror[i].wallnum].floorstat |= 1;
                else
                    sector[mirror[i].wallnum].floorstat &= ~1;
                break;
            }
        }
    }
}


void DrawMirrors(int x, int y, int z, fixed_t a, fixed_t horiz, int smooth, int viewPlayer)
{
    for (int i = mirrorcnt - 1; i >= 0; i--)
    {
        int nTile = 4080 + i;
        if (testgotpic(nTile, true))
        {
            switch (mirror[i].type)
            {
            case 0:
            {
                int nWall = mirror[i].link;
                int nSector = sectorofwall(nWall);
                walltype* pWall = &wall[nWall];
                int nNextWall = pWall->nextwall;
                int nNextSector = pWall->nextsector;
                pWall->nextwall = mirrorwall[0];
                pWall->nextsector = mirrorsector;
                wall[mirrorwall[0]].nextwall = nWall;
                wall[mirrorwall[0]].nextsector = nSector;
                wall[mirrorwall[0]].x = wall[pWall->point2].x;
                wall[mirrorwall[0]].y = wall[pWall->point2].y;
                wall[mirrorwall[1]].x = pWall->x;
                wall[mirrorwall[1]].y = pWall->y;
                wall[mirrorwall[2]].x = wall[mirrorwall[1]].x + (wall[mirrorwall[1]].x - wall[mirrorwall[0]].x) * 16;
                wall[mirrorwall[2]].y = wall[mirrorwall[1]].y + (wall[mirrorwall[1]].y - wall[mirrorwall[0]].y) * 16;
                wall[mirrorwall[3]].x = wall[mirrorwall[0]].x + (wall[mirrorwall[0]].x - wall[mirrorwall[1]].x) * 16;
                wall[mirrorwall[3]].y = wall[mirrorwall[0]].y + (wall[mirrorwall[0]].y - wall[mirrorwall[1]].y) * 16;
                sector[mirrorsector].floorz = sector[nSector].floorz;
                sector[mirrorsector].ceilingz = sector[nSector].ceilingz;
                int cx, cy, ca;
                if (GetWallType(nWall) == kWallStack)
                {
                    cx = x - (wall[pWall->hitag].x - wall[pWall->point2].x);
                    cy = y - (wall[pWall->hitag].y - wall[pWall->point2].y);
                    ca = a;
                }
                else
                {
                    renderPrepareMirror(x, y, z, a, horiz, nWall, &cx, &cy, &ca);
                }
                int32_t didmirror = renderDrawRoomsQ16(cx, cy, z, ca, horiz, mirrorsector | MAXSECTORS);
                viewProcessSprites(pm_tsprite, pm_spritesortcnt, cx, cy, z, FixedToInt(ca), smooth);
                renderDrawMasks();
                if (GetWallType(nWall) != kWallStack)
                    renderCompleteMirror();
                if (wall[nWall].pal != 0 || wall[nWall].shade != 0)
                    TranslateMirrorColors(wall[nWall].shade, wall[nWall].pal);
                pWall->nextwall = nNextWall;
                pWall->nextsector = nNextSector;
                return;
            }
            case 1:
            {
                r_rorphase = 1;
                int nSector = mirror[i].link;
                int bakCstat;
                if (viewPlayer >= 0)
                {
                    bakCstat = gPlayer[viewPlayer].pSprite->cstat;
                    if (gViewPos == 0)
                    {
                        gPlayer[viewPlayer].pSprite->cstat |= 32768;
                    }
                    else
                    {
                        gPlayer[viewPlayer].pSprite->cstat |= 514;
                    }
                }
                renderDrawRoomsQ16(x + mirror[i].dx, y + mirror[i].dy, z + mirror[i].dz, a, horiz, nSector | MAXSECTORS);
                viewProcessSprites(pm_tsprite, pm_spritesortcnt, x + mirror[i].dx, y + mirror[i].dy, z + mirror[i].dz, FixedToInt(a), smooth);
                short fstat = sector[nSector].floorstat;
                sector[nSector].floorstat |= 1;
                renderDrawMasks();
                sector[nSector].floorstat = fstat;
                for (int i = 0; i < 16; i++)
                    cleargotpic(4080 + i);
                if (viewPlayer >= 0)
                {
                    gPlayer[viewPlayer].pSprite->cstat = bakCstat;
                }
                r_rorphase = 0;
                return;
            }
            case 2:
            {
                r_rorphase = 1;
                int nSector = mirror[i].link;
                int bakCstat;
                if (viewPlayer >= 0)
                {
                    bakCstat = gPlayer[viewPlayer].pSprite->cstat;
                    if (gViewPos == 0)
                    {
                        gPlayer[viewPlayer].pSprite->cstat |= 32768;
                    }
                    else
                    {
                        gPlayer[viewPlayer].pSprite->cstat |= 514;
                    }
                }
                renderDrawRoomsQ16(x + mirror[i].dx, y + mirror[i].dy, z + mirror[i].dz, a, horiz, nSector | MAXSECTORS);
                viewProcessSprites(pm_tsprite, pm_spritesortcnt, x + mirror[i].dx, y + mirror[i].dy, z + mirror[i].dz, FixedToInt(a), smooth);
                short cstat = sector[nSector].ceilingstat;
                sector[nSector].ceilingstat |= 1;
                renderDrawMasks();
                sector[nSector].ceilingstat = cstat;
                for (int i = 0; i < 16; i++)
                    cleargotpic(4080 + i);
                if (viewPlayer >= 0)
                {
                    gPlayer[viewPlayer].pSprite->cstat = bakCstat;
                }
                r_rorphase = 0;
                return;
            }
            }
        }
    }
}



END_BLD_NS
