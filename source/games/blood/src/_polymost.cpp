#include "polymost.h"

BEGIN_BLD_NS


// leftover bits needed to keep Polymost running through the transition.
// This is mainly the game side part of the portal renderer.

void collectTSpritesForPortal(int x, int y, int i, int interpolation)
{
    int nSector = mirror[i].link;
    int nSector2 = mirror[i].wallnum;
    BloodSectIterator it(nSector);
    while (auto actor = it.Next())
    {
        spritetype* pSprite = &actor->s();
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
                pTSprite->owner = actor->GetSpriteIndex();
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
    videoSetCorrectedAspect();

    int v1 = xs_CRoundToInt(double(viewingrange) * tan(r_fov * (pi::pi() / 360.)));

    renderSetAspect(v1, yxaspect);

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

    renderDrawRoomsQ16(cX, cY, cZ, cA.asq16(), cH.asq16() + deliriumPitchI, nSectnum, false);
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
void setPortalFlags(int mode)
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

// Note: debug range checks on wall [] need to be disabled because this deliberately writes beyond the regular part.

void DrawMirrors(int x, int y, int z, fixed_t a, fixed_t horiz, int smooth, int viewPlayer)
{
    auto wallarr = wall.Data(); // this disables the range checks for the wall TArray,
    for (int i = mirrorcnt - 1; i >= 0; i--)
    {
        int nTile = 4080 + i;
        if (testgotpic(nTile, true))
        {
            switch (mirror[i].type)
            {
            case 0:
            {
                numwalls += 4; // hack alert. Blood adds some dummy walls and sectors that must not be among the counter, but here they have to be valid.
                numsectors++;
                int nWall = mirror[i].link;
                walltype* pWall = &wall[nWall];
                int nSector = pWall->sector;
                int nNextWall = pWall->nextwall;
                int nNextSector = pWall->nextsector;
                pWall->nextwall = mirrorwall[0];
                pWall->nextsector = mirrorsector;
                wallarr[mirrorwall[0]].nextwall = nWall;
                wallarr[mirrorwall[0]].nextsector = nSector;
                wallarr[mirrorwall[0]].x = pWall->point2Wall()->x;
                wallarr[mirrorwall[0]].y = pWall->point2Wall()->y;
                wallarr[mirrorwall[1]].x = pWall->x;
                wallarr[mirrorwall[1]].y = pWall->y;
                wallarr[mirrorwall[2]].x = wallarr[mirrorwall[1]].x + (wallarr[mirrorwall[1]].x - wallarr[mirrorwall[0]].x) * 16;
                wallarr[mirrorwall[2]].y = wallarr[mirrorwall[1]].y + (wallarr[mirrorwall[1]].y - wallarr[mirrorwall[0]].y) * 16;
                wallarr[mirrorwall[3]].x = wallarr[mirrorwall[0]].x + (wallarr[mirrorwall[0]].x - wallarr[mirrorwall[1]].x) * 16;
                wallarr[mirrorwall[3]].y = wallarr[mirrorwall[0]].y + (wallarr[mirrorwall[0]].y - wallarr[mirrorwall[1]].y) * 16;
                sector.Data()[mirrorsector].floorz = sector[nSector].floorz;
                sector.Data()[mirrorsector].ceilingz = sector[nSector].ceilingz;
                int cx, cy, ca;
                if (pWall->type == kWallStack)
                {
                    cx = x - (wall[pWall->hitag].x - pWall->point2Wall()->x);
                    cy = y - (wall[pWall->hitag].y - pWall->point2Wall()->y);
                    ca = a;
                }
                else
                {
                    renderPrepareMirror(x, y, z, a, horiz, nWall, &cx, &cy, &ca);
                }
                renderDrawRoomsQ16(cx, cy, z, ca, horiz, mirrorsector, true);
                viewProcessSprites(pm_tsprite, pm_spritesortcnt, cx, cy, z, FixedToInt(ca), smooth);
                renderDrawMasks();
                if (pWall->type != kWallStack)
                    renderCompleteMirror();
                pWall->nextwall = nNextWall;
                pWall->nextsector = nNextSector;
                numwalls -= 4;
                numsectors--;

                return;
            }
            case 1:
            {
                r_rorphase = 1;
                int nSector = mirror[i].link;
                int bakCstat = 0;
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
                renderDrawRoomsQ16(x + mirror[i].dx, y + mirror[i].dy, z + mirror[i].dz, a, horiz, nSector, true);
                viewProcessSprites(pm_tsprite, pm_spritesortcnt, x + mirror[i].dx, y + mirror[i].dy, z + mirror[i].dz, FixedToInt(a), smooth);
                auto fstat = sector[nSector].floorstat;
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
                int bakCstat = 0;
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
                renderDrawRoomsQ16(x + mirror[i].dx, y + mirror[i].dy, z + mirror[i].dz, a, horiz, nSector, true);
                viewProcessSprites(pm_tsprite, pm_spritesortcnt, x + mirror[i].dx, y + mirror[i].dy, z + mirror[i].dz, FixedToInt(a), smooth);
                auto cstat = sector[nSector].ceilingstat;
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


void InitPolymostMirrorHack()
{
    mirrorsector = numsectors;
    for (int i = 0; i < 4; i++)
    {
        mirrorwall[i] = numwalls + i;
        auto pWall = &(wall.Data()[mirrorwall[i]]);
        pWall->picnum = 504;
        pWall->overpicnum = 504;
        pWall->cstat = 0;
        pWall->nextsector = -1;
        pWall->nextwall = -1;
        pWall->point2 = numwalls + i + 1;
    }
    wall.Data()[mirrorwall[3]].point2 = mirrorwall[0];
    sector.Data()[mirrorsector].ceilingpicnum = 504;
    sector.Data()[mirrorsector].floorpicnum = 504;
    sector.Data()[mirrorsector].wallptr = mirrorwall[0];
    sector.Data()[mirrorsector].wallnum = 4;
}

void PolymostAllocFakeSector()
{
    // these additional entries are needed by Blood's mirror code. We must get them upon map load to avoid a later occuring reallocation. Ugh...
    // We do not want to actually increase the array size for this, though because it may screw with the savegame code. 
    // Before rendering this will temporarily be bumped up.
    sector.Reserve(1);
    wall.Reserve(4);
    sector.Resize(numsectors);
    wall.Resize(numwalls);
}
END_BLD_NS
