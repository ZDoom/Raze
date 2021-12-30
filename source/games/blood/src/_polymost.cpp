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
        if (actor == gView->actor)
            continue;
        int top, bottom;
        GetActorExtents(actor, &top, &bottom);
        int zCeil, zFloor;
        getzsofslopeptr(&sector[nSector], actor->spr.pos.X, actor->spr.pos.Y, &zCeil, &zFloor);
        if (actor->spr.statnum == kStatDude && (top < zCeil || bottom > zFloor))
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
                pTSprite->type = actor->spr.type;
                pTSprite->sectp = &sector[nSector2];
                pTSprite->pos.X = actor->spr.pos.X + dx;
                pTSprite->pos.Y = actor->spr.pos.Y + dy;
                pTSprite->pos.Z = actor->spr.pos.Z + dz;
                pTSprite->ang = actor->spr.ang;
                pTSprite->picnum = actor->spr.picnum;
                pTSprite->shade = actor->spr.shade;
                pTSprite->pal = actor->spr.pal;
                pTSprite->xrepeat = actor->spr.xrepeat;
                pTSprite->yrepeat = actor->spr.yrepeat;
                pTSprite->xoffset = actor->spr.xoffset;
                pTSprite->yoffset = actor->spr.yoffset;
                pTSprite->cstat = actor->spr.cstat;
                pTSprite->statnum = kStatDecoration;
                pTSprite->ownerActor = actor;
                pTSprite->flags = actor->spr.hitag | 0x200;
                pTSprite->pos.X = dx + interpolatedvalue(actor->opos.X, actor->spr.pos.X, interpolation);
                pTSprite->pos.Y = dy + interpolatedvalue(actor->opos.Y, actor->spr.pos.Y, interpolation);
                pTSprite->pos.Z = dz + interpolatedvalue(actor->opos.Z, actor->spr.pos.Z, interpolation);
                pTSprite->ang = actor->interpolatedang(interpolation);

                int nAnim = 0;
                switch (picanm[pTSprite->picnum].extra & 7)
                {
                case 1:
                {
                    int dX = x - pTSprite->pos.X;
                    int dY = y - pTSprite->pos.Y;
                    RotateVector(&dX, &dY, 128 - pTSprite->ang);
                    nAnim = GetOctant(dX, dY);
                    if (nAnim <= 4)
                    {
                        pTSprite->cstat &= ~CSTAT_SPRITE_XFLIP;
                    }
                    else
                    {
                        nAnim = 8 - nAnim;
                        pTSprite->cstat |= CSTAT_SPRITE_XFLIP;
                    }
                    break;
                }
                case 2:
                {
                    int dX = x - pTSprite->pos.X;
                    int dY = y - pTSprite->pos.Y;
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
    auto bakCstat = gView->actor->spr.cstat;
    if (gViewPos == 0)
    {
        gView->actor->spr.cstat |= CSTAT_SPRITE_INVISIBLE;
    }
    else
    {
        gView->actor->spr.cstat |= CSTAT_SPRITE_TRANSLUCENT |  CSTAT_SPRITE_TRANS_FLIP;
    }

    renderDrawRoomsQ16(cX, cY, cZ, cA.asq16(), cH.asq16() + deliriumPitchI, nSectnum, false);
    viewProcessSprites(pm_tsprite, pm_spritesortcnt, cX, cY, cZ, cA.asbuild(), int(gInterpolate));
    bool do_ror_hack = false;
    for (int i = 0; i < 16; i++)
        if (ror_status[i] != testgotpic(4080 + i))
            do_ror_hack = true;
    if (do_ror_hack)
    {
        gView->actor->spr.cstat = bakCstat;
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
    gView->actor->spr.cstat = bakCstat;

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
                    sector[mirror[i].wallnum].ceilingstat |= CSTAT_SECTOR_SKY;
                else
                    sector[mirror[i].wallnum].ceilingstat &= ~CSTAT_SECTOR_SKY;
                break;
            case 2:
                if (mode)
                    sector[mirror[i].wallnum].floorstat |= CSTAT_SECTOR_SKY;
                else
                    sector[mirror[i].wallnum].floorstat &= ~CSTAT_SECTOR_SKY;
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
                // gross hack alert. Blood adds some dummy walls and sectors that must not be among the counted, but here they have to be valid.
                wall.Reserve(4);
                sector.Reserve(1);
                int nWall = mirror[i].link;
                walltype* pWall = &wall[nWall];
                int nSector = pWall->sector;
                int nNextWall = pWall->nextwall;
                int nNextSector = pWall->nextsector;
                pWall->nextwall = mirrorwall[0];
                pWall->nextsector = mirrorsector;
                wallarr[mirrorwall[0]].nextwall = nWall;
                wallarr[mirrorwall[0]].nextsector = nSector;
                wallarr[mirrorwall[0]].pos.X = pWall->point2Wall()->pos.X;
                wallarr[mirrorwall[0]].pos.Y = pWall->point2Wall()->pos.Y;
                wallarr[mirrorwall[1]].pos.X = pWall->pos.X;
                wallarr[mirrorwall[1]].pos.Y = pWall->pos.Y;
                wallarr[mirrorwall[2]].pos.X = wallarr[mirrorwall[1]].pos.X + (wallarr[mirrorwall[1]].pos.X - wallarr[mirrorwall[0]].pos.X) * 16;
                wallarr[mirrorwall[2]].pos.Y = wallarr[mirrorwall[1]].pos.Y + (wallarr[mirrorwall[1]].pos.Y - wallarr[mirrorwall[0]].pos.Y) * 16;
                wallarr[mirrorwall[3]].pos.X = wallarr[mirrorwall[0]].pos.X + (wallarr[mirrorwall[0]].pos.X - wallarr[mirrorwall[1]].pos.X) * 16;
                wallarr[mirrorwall[3]].pos.Y = wallarr[mirrorwall[0]].pos.Y + (wallarr[mirrorwall[0]].pos.Y - wallarr[mirrorwall[1]].pos.Y) * 16;
                sector.Data()[mirrorsector].setfloorz(sector[nSector].floorz, true);
                sector.Data()[mirrorsector].setceilingz(sector[nSector].ceilingz, true);
                int cx, cy, ca;
                if (pWall->type == kWallStack)
                {
                    cx = x - (wall[pWall->hitag].pos.X - pWall->point2Wall()->pos.X);
                    cy = y - (wall[pWall->hitag].pos.Y - pWall->point2Wall()->pos.Y);
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
                wall.Clamp(wall.Size() - 4);
                sector.Clamp(sector.Size() - 1);

                return;
            }
            case 1:
            {
                r_rorphase = 1;
                int nSector = mirror[i].link;
                ESpriteFlags bakCstat = 0;
                if (viewPlayer >= 0)
                {
                    bakCstat = gPlayer[viewPlayer].actor->spr.cstat;
                    if (gViewPos == 0)
                    {
                        gPlayer[viewPlayer].actor->spr.cstat |= CSTAT_SPRITE_INVISIBLE;
                    }
                    else
                    {
                        gPlayer[viewPlayer].actor->spr.cstat |= CSTAT_SPRITE_TRANSLUCENT |  CSTAT_SPRITE_TRANS_FLIP;
                    }
                }
                renderDrawRoomsQ16(x + mirror[i].dx, y + mirror[i].dy, z + mirror[i].dz, a, horiz, nSector, true);
                viewProcessSprites(pm_tsprite, pm_spritesortcnt, x + mirror[i].dx, y + mirror[i].dy, z + mirror[i].dz, FixedToInt(a), smooth);
                auto fstat = sector[nSector].floorstat;
                sector[nSector].floorstat |= CSTAT_SECTOR_SKY;
                renderDrawMasks();
                sector[nSector].floorstat = fstat;
                for (int ii = 0; ii < 16; ii++)
                    gotpic.Clear(4080 + ii);
                if (viewPlayer >= 0)
                {
                    gPlayer[viewPlayer].actor->spr.cstat = bakCstat;
                }
                r_rorphase = 0;
                return;
            }
            case 2:
            {
                r_rorphase = 1;
                int nSector = mirror[i].link;
                ESpriteFlags bakCstat = 0;
                if (viewPlayer >= 0)
                {
                    bakCstat = gPlayer[viewPlayer].actor->spr.cstat;
                    if (gViewPos == 0)
                    {
                        gPlayer[viewPlayer].actor->spr.cstat |= CSTAT_SPRITE_INVISIBLE;
                    }
                    else
                    {
                        gPlayer[viewPlayer].actor->spr.cstat |= CSTAT_SPRITE_TRANSLUCENT |  CSTAT_SPRITE_TRANS_FLIP;
                    }
                }
                renderDrawRoomsQ16(x + mirror[i].dx, y + mirror[i].dy, z + mirror[i].dz, a, horiz, nSector, true);
                viewProcessSprites(pm_tsprite, pm_spritesortcnt, x + mirror[i].dx, y + mirror[i].dy, z + mirror[i].dz, FixedToInt(a), smooth);
                auto cstat = sector[nSector].ceilingstat;
                sector[nSector].ceilingstat |= CSTAT_SECTOR_SKY;
                renderDrawMasks();
                sector[nSector].ceilingstat = cstat;
                for (int ii = 0; ii < 16; ii++)
                    gotpic.Clear(4080 + ii);
                if (viewPlayer >= 0)
                {
                    gPlayer[viewPlayer].actor->spr.cstat = bakCstat;
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
    mirrorsector = sector.Size();
    for (int i = 0; i < 4; i++)
    {
        mirrorwall[i] = wall.Size() + i;
        auto pWall = &(wall.Data()[mirrorwall[i]]);
        pWall->picnum = 504;
        pWall->overpicnum = 504;
        pWall->cstat = 0;
        pWall->nextsector = -1;
        pWall->nextwall = -1;
        pWall->point2 = wall.Size() + i + 1;
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
    // Note that this depends on the resize operation not deleting and altering the new entries! 
    sector.Reserve(1);
    wall.Reserve(4);
    wall.Clamp(wall.Size() - 4);
    sector.Clamp(sector.Size() - 1);
}
END_BLD_NS
