//-------------------------------------------------------------------------
/*
Copyright (C) 2010-2019 EDuke32 developers and contributors
Copyright (C) 2019 Nuke.YKT

This file is part of NBlood.

NBlood is free software; you can redistribute it and/or
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

#include "ns.h"	// Must come before everything else!

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "build.h"
#include "blood.h"


BEGIN_BLD_NS

POINT2D baseWall[kMaxWalls];
int baseFloor[kMaxSectors];
int baseCeil[kMaxSectors];
int velFloor[kMaxSectors];
int velCeil[kMaxSectors];
DBloodActor* gUpperLink[kMaxSectors];
DBloodActor* gLowerLink[kMaxSectors];
HITINFO gHitInfo;

bool AreSectorsNeighbors(int sect1, int sect2)
{
    assert(sect1 >= 0 && sect1 < kMaxSectors);
    assert(sect2 >= 0 && sect2 < kMaxSectors);
    if (sector[sect1].wallnum < sector[sect2].wallnum)
    {
        for (int i = 0; i < sector[sect1].wallnum; i++)
        {
            if (wall[sector[sect1].wallptr+i].nextsector == sect2)
            {
                return 1;
            }
        }
    }
    else
    {
        for (int i = 0; i < sector[sect2].wallnum; i++)
        {
            if (wall[sector[sect2].wallptr+i].nextsector == sect1)
            {
                return 1;
            }
        }
    }
    return 0;
}

bool FindSector(int nX, int nY, int nZ, int *nSector)
{
    int32_t nZFloor, nZCeil;
    assert(*nSector >= 0 && *nSector < kMaxSectors);
    if (inside(nX, nY, *nSector))
    {
        getzsofslope(*nSector, nX, nY, &nZCeil, &nZFloor);
        if (nZ >= nZCeil && nZ <= nZFloor)
        {
            return 1;
        }
    }
    walltype *pWall = &wall[sector[*nSector].wallptr];
    for (int i = sector[*nSector].wallnum; i > 0; i--, pWall++)
    {
        int nOSector = pWall->nextsector;
        if (nOSector >= 0 && inside(nX, nY, nOSector))
        {
            getzsofslope(nOSector, nX, nY, &nZCeil, &nZFloor);
            if (nZ >= nZCeil && nZ <= nZFloor)
            {
                *nSector = nOSector;
                return 1;
            }
        }
    }
    for (int i = 0; i < numsectors; i++)
    {
        if (inside(nX, nY, i))
        {
            getzsofslope(i, nX, nY, &nZCeil, &nZFloor);
            if (nZ >= nZCeil && nZ <= nZFloor)
            {
                *nSector = i;
                return 1;
            }
        }
    }
    return 0;
}

bool FindSector(int nX, int nY, int *nSector)
{
    assert(*nSector >= 0 && *nSector < kMaxSectors);
    if (inside(nX, nY, *nSector))
    {
        return 1;
    }
    walltype *pWall = &wall[sector[*nSector].wallptr];
    for (int i = sector[*nSector].wallnum; i > 0; i--, pWall++)
    {
        int nOSector = pWall->nextsector;
        if (nOSector >= 0 && inside(nX, nY, nOSector))
        {
            *nSector = nOSector;
            return 1;
        }
    }
    for (int i = 0; i < numsectors; i++)
    {
        if (inside(nX, nY, i))
        {
            *nSector = i;
            return 1;
        }
    }
    return 0;
}

bool CheckProximity(DBloodActor *actor, int nX, int nY, int nZ, int nSector, int nDist)
{
    assert(actor != NULL);
    auto pSprite = &actor->s();
    int oX = abs(nX-pSprite->x)>>4;
    if (oX >= nDist) return 0;

    int oY = abs(nY-pSprite->y)>>4;
    if (oY >= nDist) return 0;

    int oZ = abs(nZ-pSprite->z)>>8;
    if (oZ >= nDist) return 0;

    if (approxDist(oX, oY) >= nDist) return 0;

    int bottom, top;
    GetActorExtents(actor, &top, &bottom);
    if (cansee(pSprite->x, pSprite->y, pSprite->z, pSprite->sectnum, nX, nY, nZ, nSector))
        return 1;
    if (cansee(pSprite->x, pSprite->y, bottom, pSprite->sectnum, nX, nY, nZ, nSector))
        return 1;
    if (cansee(pSprite->x, pSprite->y, top, pSprite->sectnum, nX, nY, nZ, nSector))
        return 1;
    return 0;
}

bool CheckProximityPoint(int nX1, int nY1, int nZ1, int nX2, int nY2, int nZ2, int nDist)
{
    int oX = abs(nX2-nX1)>>4;
    if (oX >= nDist)
        return 0;
    int oY = abs(nY2-nY1)>>4;
    if (oY >= nDist)
        return 0;
    if (nZ2 != nZ1)
    {
        int oZ = abs(nZ2-nZ1)>>8;
        if (oZ >= nDist)
            return 0;
    }
    if (approxDist(oX, oY) >= nDist) return 0;
    return 1;
}

bool CheckProximityWall(int nWall, int x, int y, int nDist)
{
    int x1 = wall[nWall].x;
    int y1 = wall[nWall].y;
    int x2 = wall[wall[nWall].point2].x;
    int y2 = wall[wall[nWall].point2].y;
    nDist <<= 4;
    if (x1 < x2)
    {
        if (x <= x1 - nDist || x >= x2 + nDist)
        {
            return 0;
        }
    }
    else
    {
        if (x <= x2 - nDist || x >= x1 + nDist)
        {
            return 0;
        }
        if (x1 == x2)
        {
            int px1 = x - x1;
            int py1 = y - y1;
            int px2 = x - x2;
            int py2 = y - y2;
            int dist1 = px1 * px1 + py1 * py1;
            int dist2 = px2 * px2 + py2 * py2;
            if (y1 < y2)
            {
                if (y <= y1 - nDist || y >= y2 + nDist)
                {
                    return 0;
                }
                if (y < y1)
                {
                    return dist1 < nDist * nDist;
                }
                if (y > y2)
                {
                    return dist2 < nDist * nDist;
                }
            }
            else
            {
                if (y <= y2 - nDist || y >= y1 + nDist)
                {
                    return 0;
                }
                if (y < y2)
                {
                    return dist2 < nDist * nDist;
                }
                if (y > y1)
                {
                    return dist1 < nDist * nDist;
                }
            }
            return 1;
        }
    }
    if (y1 < y2)
    {
        if (y <= y1 - nDist || y >= y2 + nDist)
        {
            return 0;
        }
    }
    else
    {
        if (y <= y2 - nDist || y >= y1 + nDist)
        {
            return 0;
        }
        if (y1 == y2)
        {
            int px1 = x - x1;
            int py1 = y - y1;
            int px2 = x - x2;
            int py2 = y - y2;
            int check1 = px1 * px1 + py1 * py1;
            int check2 = px2 * px2 + py2 * py2;
            if (x1 < x2)
            {
                if (x <= x1 - nDist || x >= x2 + nDist)
                {
                    return 0;
                }
                if (x < x1)
                {
                    return check1 < nDist * nDist;
                }
                if (x > x2)
                {
                    return check2 < nDist * nDist;
                }
            }
            else
            {
                if (x <= x2 - nDist || x >= x1 + nDist)
                {
                    return 0;
                }
                if (x < x2)
                {
                    return check2 < nDist * nDist;
                }
                if (x > x1)
                {
                    return check1 < nDist * nDist;
                }
            }
        }
    }

    int dx = x2 - x1;
    int dy = y2 - y1;
    int px = x - x2;
    int py = y - y2;
    int side = px * dx + dy * py;
    if (side >= 0)
    {
        return px * px + py * py < nDist * nDist;
    }
    px = x - x1;
    py = y - y1;
    side = px * dx + dy * py;
    if (side <= 0)
    {
        return px * px + py * py < nDist * nDist;
    }
    int check1 = px * dy - dx * py;
    int check2 = dy * dy + dx * dx;
    return check1 * check1 < check2 * nDist * nDist;
}

int GetWallAngle(int nWall)
{
    int nWall2 = wall[nWall].point2;
    return getangle(wall[nWall2].x - wall[nWall].x, wall[nWall2].y - wall[nWall].y);
}

int GetWallAngle(walltype* pWall)
{
    int nWall2 = pWall->point2;
    return getangle(wall[nWall2].x - pWall->x, wall[nWall2].y - pWall->y);
}

void GetWallNormal(int nWall, int *pX, int *pY)
{
    assert(nWall >= 0 && nWall < kMaxWalls);
    int nWall2 = wall[nWall].point2;
    int dX = -(wall[nWall2].y - wall[nWall].y);
    dX >>= 4;
    int dY = wall[nWall2].x - wall[nWall].x;
    dY >>= 4;
    int nLength = ksqrt(dX*dX+dY*dY);
    if (nLength <= 0)
        nLength = 1;
    *pX = DivScale(dX, nLength, 16);
    *pY = DivScale(dY, nLength, 16);
}

bool IntersectRay(int wx, int wy, int wdx, int wdy, int x1, int y1, int z1, int x2, int y2, int z2, int *ix, int *iy, int *iz)
{
    int dX = x1 - x2;
    int dY = y1 - y2;
    int dZ = z1 - z2;
    int side = wdx * dY - wdy * dX;
    int dX2 = x1 - wx;
    int dY2 = y1 - wy;
    int check1 = dX2 * dY - dY2 * dX;
    int check2 = wdx * dY2 - wdy * dX2;
    if (side >= 0)
    {
        if (!side)
            return 0;
        if (check1 < 0)
            return 0;
        if (check2 < 0 || check2 >= side)
            return 0;
    }
    else
    {
        if (check1 > 0)
            return 0;
        if (check2 > 0 || check2 <= side)
            return 0;
    }
    int nScale = DivScale(check2, side, 16);
    *ix = x1 + MulScale(dX, nScale, 16);
    *iy = y1 + MulScale(dY, nScale, 16);
    *iz = z1 + MulScale(dZ, nScale, 16);
    return 1;
}

int HitScan(DBloodActor *actor, int z, int dx, int dy, int dz, unsigned int nMask, int nRange)
{
    assert(actor != NULL);
    auto pSprite = &actor->s();
    assert(dx != 0 || dy != 0);
    gHitInfo.clearObj();
    int x = pSprite->x;
    int y = pSprite->y;
    int nSector = pSprite->sectnum;
    int bakCstat = pSprite->cstat;
    pSprite->cstat &= ~256;
    if (nRange)
    {
        hitscangoal.x = x + MulScale(nRange << 4, Cos(pSprite->ang), 30);
        hitscangoal.y = y + MulScale(nRange << 4, Sin(pSprite->ang), 30);
    }
    else
    {
        hitscangoal.x = hitscangoal.y = 0x1ffffff;
    }
    vec3_t pos = { x, y, z };
    hitdata_t hitData;
    hitData.pos.z = gHitInfo.hitz;
    hitscan(&pos, nSector, dx, dy, dz << 4, &hitData, nMask);
    gHitInfo.set(&hitData);
    hitscangoal.x = hitscangoal.y = 0x1ffffff;
    pSprite->cstat = bakCstat;
    if (gHitInfo.hitwall >= kMaxWalls || gHitInfo.hitsect >= kMaxSectors)
        return -1;
    if (gHitInfo.hitactor != nullptr)
        return 3;
    if (gHitInfo.hitwall >= 0)
    {
        if (wall[gHitInfo.hitwall].nextsector == -1)
            return 0;
        int nZCeil, nZFloor;
        getzsofslope(wall[gHitInfo.hitwall].nextsector, gHitInfo.hitx, gHitInfo.hity, &nZCeil, &nZFloor);
        if (gHitInfo.hitz <= nZCeil || gHitInfo.hitz >= nZFloor)
            return 0;
        return 4;
    }
    if (gHitInfo.hitsect >= 0)
        return 1 + (z < gHitInfo.hitz);
    return -1;
}

int VectorScan(DBloodActor *actor, int nOffset, int nZOffset, int dx, int dy, int dz, int nRange, int ac)
{
    assert(actor != NULL);
    auto pSprite = &actor->s();

    int nNum = 256;
    assert(pSprite != NULL);
    gHitInfo.clearObj();
    int x1 = pSprite->x+MulScale(nOffset, Cos(pSprite->ang+512), 30);
    int y1 = pSprite->y+MulScale(nOffset, Sin(pSprite->ang+512), 30);
    int z1 = pSprite->z+nZOffset;
    int bakCstat = pSprite->cstat;
    pSprite->cstat &= ~256;
    int nSector = pSprite->sectnum;
    if (nRange)
    {
        hitscangoal.x = x1+MulScale(nRange<<4, Cos(pSprite->ang), 30);
        hitscangoal.y = y1+MulScale(nRange<<4, Sin(pSprite->ang), 30);
    }
    else
    {
        hitscangoal.x = hitscangoal.y = 0x1fffffff;
    }
    vec3_t pos = { x1, y1, z1 };
    hitdata_t hitData;
    hitData.pos.z = gHitInfo.hitz;
    hitscan(&pos, nSector, dx, dy, dz << 4, &hitData, CLIPMASK1);
    gHitInfo.set(&hitData);
    hitscangoal.x = hitscangoal.y = 0x1ffffff;
    pSprite->cstat = bakCstat;
    while (nNum--)
    {
        if (gHitInfo.hitwall >= kMaxWalls || gHitInfo.hitsect >= kMaxSectors)
            return -1;
        if (nRange && approxDist(gHitInfo.hitx - pSprite->x, gHitInfo.hity - pSprite->y) > nRange)
            return -1;
        if (gHitInfo.hitactor != nullptr)
        {
            spritetype *pOther = &gHitInfo.hitactor->s();
            if ((pOther->flags & 8) && !(ac & 1))
                return 3;
            if ((pOther->cstat & 0x30) != 0)
                return 3;
            int nPicnum = pOther->picnum;
            if (tileWidth(nPicnum) == 0 || tileHeight(nPicnum) == 0)
                return 3;
            int height = (tileHeight(nPicnum)*pOther->yrepeat)<<2;
            int otherZ = pOther->z;
            if (pOther->cstat & 0x80)
                otherZ += height / 2;
            int nOffset = tileTopOffset(nPicnum);
            if (nOffset)
                otherZ -= (nOffset*pOther->yrepeat)<<2;
            assert(height > 0);
            int height2 = scale(otherZ-gHitInfo.hitz, tileHeight(nPicnum), height);
            if (!(pOther->cstat & 8))
                height2 = tileHeight(nPicnum)-height2;
            if (height2 >= 0 && height2 < tileHeight(nPicnum))
            {
                int width = (tileWidth(nPicnum)*pOther->xrepeat)>>2;
                width = (width*3)/4;
                int check1 = ((y1 - pOther->y)*dx - (x1 - pOther->x)*dy) / ksqrt(dx*dx+dy*dy);
                assert(width > 0);
                int width2 = scale(check1, tileWidth(nPicnum), width);
                int nOffset = tileLeftOffset(nPicnum);
                width2 += nOffset + tileWidth(nPicnum) / 2;
                if (width2 >= 0 && width2 < tileWidth(nPicnum))
                {
                    auto pData = tilePtr(nPicnum);
                    if (pData[width2*tileHeight(nPicnum)+height2] != TRANSPARENT_INDEX)
                        return 3;
                }
            }
            int bakCstat = pOther->cstat;
            pOther->cstat &= ~256;
            gHitInfo.clearObj();
            x1 = gHitInfo.hitx;
            y1 = gHitInfo.hity;
            z1 = gHitInfo.hitz;
            pos = { x1, y1, z1 };
            hitData.pos.z = gHitInfo.hitz;
            hitscan(&pos, pOther->sectnum, dx, dy, dz << 4, &hitData, CLIPMASK1);
            gHitInfo.set(&hitData);
            pOther->cstat = bakCstat;
            continue;
        }
        if (gHitInfo.hitwall >= 0)
        {
            walltype *pWall = &wall[gHitInfo.hitwall];
            if (pWall->nextsector == -1)
                return 0;
            sectortype *pSector = &sector[gHitInfo.hitsect];
            sectortype *pSectorNext = &sector[pWall->nextsector];
            int nZCeil, nZFloor;
            getzsofslope(pWall->nextsector, gHitInfo.hitx, gHitInfo.hity, &nZCeil, &nZFloor);
            if (gHitInfo.hitz <= nZCeil)
                return 0;
            if (gHitInfo.hitz >= nZFloor)
            {
                if (!(pSector->floorstat&1) || !(pSectorNext->floorstat&1))
                    return 0;
                return 2;
            }
            if (!(pWall->cstat & 0x30))
                return 0;
            int nOffset;
            if (pWall->cstat & 4)
                nOffset = ClipHigh(pSector->floorz, pSectorNext->floorz);
            else
                nOffset = ClipLow(pSector->ceilingz, pSectorNext->ceilingz);
            nOffset = (gHitInfo.hitz - nOffset) >> 8;
            if (pWall->cstat & 256)
                nOffset = -nOffset;

            int nPicnum = pWall->overpicnum;
            int nSizX = tileWidth(nPicnum);
            int nSizY = tileHeight(nPicnum);
            if (!nSizX || !nSizY)
                return 0;

            nOffset = (nOffset*pWall->yrepeat) / 8;
            nOffset += int((nSizY*pWall->ypan_) / 256);
            int nLength = approxDist(pWall->x - wall[pWall->point2].x, pWall->y - wall[pWall->point2].y);
            int nHOffset;
            if (pWall->cstat & 8)
                nHOffset = approxDist(gHitInfo.hitx - wall[pWall->point2].x, gHitInfo.hity - wall[pWall->point2].y);
            else
                nHOffset = approxDist(gHitInfo.hitx - pWall->x, gHitInfo.hity - pWall->y);

            nHOffset = pWall->xpan() + ((nHOffset*pWall->xrepeat) << 3) / nLength;
            nHOffset %= nSizX;
            nOffset %= nSizY;
            auto pData = tilePtr(nPicnum);
            int nPixel;
            nPixel = nHOffset*nSizY + nOffset;

            if (pData[nPixel] == TRANSPARENT_INDEX)
            {
                int bakCstat = pWall->cstat;
                pWall->cstat &= ~64;
                int bakCstat2 = wall[pWall->nextwall].cstat;
                wall[pWall->nextwall].cstat &= ~64;
                gHitInfo.clearObj();
                x1 = gHitInfo.hitx;
                y1 = gHitInfo.hity;
                z1 = gHitInfo.hitz;
                pos = { x1, y1, z1 };
                hitData.pos.z = gHitInfo.hitz;
                hitscan(&pos, pWall->nextsector,
                    dx, dy, dz << 4, &hitData, CLIPMASK1);
                gHitInfo.set(&hitData);
                pWall->cstat = bakCstat;
                wall[pWall->nextwall].cstat = bakCstat2;
                continue;
            }
            return 4;
        }
        if (gHitInfo.hitsect >= 0)
        {
            if (dz > 0)
            {
                auto actor = getUpperLink(gHitInfo.hitsect);
                if (!actor) return 2;
                auto link = actor->GetOwner();
                gHitInfo.clearObj();
                vec3_t pos = link->s().pos - actor->s().pos + vec3_t(gHitInfo.hitx, gHitInfo.hity, gHitInfo.hitz);
                hitData.pos.z = gHitInfo.hitz;
                hitscan(&pos, link->s().sectnum, dx, dy, dz<<4, &hitData, CLIPMASK1);
                gHitInfo.set(&hitData);
                continue;
            }
            else
            {
                auto actor = getLowerLink(gHitInfo.hitsect);
                if (!actor) return 1;
                auto link = actor->GetOwner();
                gHitInfo.clearObj();
                vec3_t pos = link->s().pos - actor->s().pos + vec3_t(gHitInfo.hitx, gHitInfo.hity, gHitInfo.hitz);
                hitData.pos.z = gHitInfo.hitz;
                hitscan(&pos, link->s().sectnum, dx, dy, dz<<4, &hitData, CLIPMASK1);
                gHitInfo.set(&hitData);
                continue;
            }
        }
        return -1;
    }
    return -1;
}

void GetZRange(DBloodActor *actor, int *ceilZ, Collision *ceilColl, int *floorZ, Collision *floorColl, int nDist, unsigned int nMask, unsigned int nClipParallax)
{
    assert(actor != NULL);
    auto pSprite = &actor->s();

    int floorHit, ceilHit;
    int bakCstat = pSprite->cstat;
    int32_t nTemp1, nTemp2;
    pSprite->cstat &= ~257;
    getzrange(&pSprite->pos, pSprite->sectnum, (int32_t*)ceilZ, &ceilHit, (int32_t*)floorZ, &floorHit, nDist, nMask);
    ceilColl->setFromEngine(ceilHit);
    floorColl->setFromEngine(floorHit);
    if (floorColl->type == kHitSector)
    {
        int nSector = floorColl->index;
        if ((nClipParallax & PARALLAXCLIP_FLOOR) == 0 && (sector[nSector].floorstat & 1))
            *floorZ = 0x7fffffff;
        if (sector[nSector].extra > 0)
        {
            XSECTOR *pXSector = &xsector[sector[nSector].extra];
            *floorZ += pXSector->Depth << 10;
        }
        auto actor = getUpperLink(nSector);
        if (actor)
        {
            auto link = actor->GetOwner();
            vec3_t lpos = pSprite->pos + link->s().pos - actor->s().pos;
            getzrange(&lpos, link->s().sectnum, &nTemp1, &nTemp2, (int32_t*)floorZ, &floorHit, nDist, nMask);
            *floorZ -= link->s().z - actor->s().z;
            floorColl->setFromEngine(floorHit);
        }
    }
    if (ceilColl->type == kHitSector)
    {
        int nSector = ceilColl->index;
        if ((nClipParallax & PARALLAXCLIP_CEILING) == 0 && (sector[nSector].ceilingstat & 1))
            *ceilZ = 0x80000000;
        auto actor = getLowerLink(nSector);
        if (actor)
        {
            auto link = actor->GetOwner();
            vec3_t lpos = pSprite->pos + link->s().pos - actor->s().pos;
            getzrange(&lpos, link->s().sectnum, (int32_t*)ceilZ, &ceilHit, &nTemp1, &nTemp2, nDist, nMask);
            *ceilZ -= link->s().z - actor->s().z;
            ceilColl->setFromEngine(ceilHit);
        }
    }
    pSprite->cstat = bakCstat;
}

void GetZRangeAtXYZ(int x, int y, int z, int nSector, int *ceilZ, Collision* ceilColl, int* floorZ, Collision* floorColl, int nDist, unsigned int nMask, unsigned int nClipParallax)
{
    int ceilHit, floorHit;
    int32_t nTemp1, nTemp2;
    vec3_t lpos = { x, y, z };
    getzrange(&lpos, nSector, (int32_t*)ceilZ, &ceilHit, (int32_t*)floorZ, &floorHit, nDist, nMask);
    ceilColl->setFromEngine(ceilHit);
    floorColl->setFromEngine(floorHit);
    if (floorColl->type == kHitSector)
    {
        int nSector = floorColl->index;
        if ((nClipParallax & PARALLAXCLIP_FLOOR) == 0 && (sector[nSector].floorstat & 1))
            *floorZ = 0x7fffffff;
        if (sector[nSector].extra > 0)
        {
            XSECTOR *pXSector = &xsector[sector[nSector].extra];
            *floorZ += pXSector->Depth << 10;
        }
        auto actor = getUpperLink(nSector);
        if (actor)
        {
            auto link = actor->GetOwner();
            vec3_t newpos = lpos + link->s().pos - actor->s().pos;
            getzrange(&newpos, link->s().sectnum, &nTemp1, &nTemp2, (int32_t*)floorZ, &floorHit, nDist, nMask);
            floorColl->setFromEngine(floorHit);
            *floorZ -= link->s().z - actor->s().z;
        }
    }
    if (ceilColl->type == kHitSector)
    {
        int nSector = ceilColl->index;
        if ((nClipParallax & PARALLAXCLIP_CEILING) == 0 && (sector[nSector].ceilingstat & 1))
            *ceilZ = 0x80000000;
        auto actor = getLowerLink(nSector);
        if (actor)
        {
            auto link = actor->GetOwner();
            vec3_t newpos = lpos + link->s().pos - actor->s().pos;
            getzrange(&newpos, link->s().sectnum, (int32_t*)ceilZ, &ceilHit, &nTemp1, &nTemp2, nDist, nMask);
            ceilColl->setFromEngine(ceilHit);
            *ceilZ -= link->s().z - actor->s().z;
        }
    }
}

int GetDistToLine(int x1, int y1, int x2, int y2, int x3, int y3)
{
    int check = (y1-y3)*(x3-x2);
    int check2 = (x1-x2)*(y3-y2);
    if (check2 > check)
        return -1;
    int v8 = DMulScale(x1-x2,x3-x2,y1-y3,y3-y2,4);
    int vv = DMulScale(x3-x2,x3-x2,y3-y2,y3-y2,4);
    int t1, t2;
    if (v8 <= 0)
    {
        t1 = x2;
        t2 = y2;
    }
    else if (vv > v8)
    {
        t1 = x2+scale(x3-x2,v8,vv);
        t2 = y2+scale(y3-y2,v8,vv);
    }
    else
    {
        t1 = x3;
        t2 = y3;
    }
    return approxDist(t1-x1, t2-y1);
}

unsigned int ClipMove(vec3_t *pos, int *nSector, int xv, int yv, int wd, int cd, int fd, unsigned int nMask, int tracecount)
{
    auto opos = *pos;
    int bakSect = *nSector;
    unsigned int nRes = clipmove(pos, &bakSect, xv<<14, yv<<14, wd, cd, fd, nMask, tracecount);
    if (bakSect == -1)
    {
        *pos = opos;
    }
    else
    {
        *nSector = bakSect;
    }
    return nRes;
}

int GetClosestSectors(int nSector, int x, int y, int nDist, short *pSectors, char *pSectBit)
{
    char sectbits[(kMaxSectors+7)>>3];
    assert(pSectors != NULL);
    memset(sectbits, 0, sizeof(sectbits));
    pSectors[0] = nSector;
    SetBitString(sectbits, nSector);
    int n = 1;
    int i = 0;
    if (pSectBit)
    {
        memset(pSectBit, 0, (kMaxSectors+7)>>3);
        SetBitString(pSectBit, nSector);
    }
    while (i < n)
    {
        int nCurSector = pSectors[i];
        int nStartWall = sector[nCurSector].wallptr;
        int nEndWall = nStartWall + sector[nCurSector].wallnum;
        walltype *pWall = &wall[nStartWall];
        for (int j = nStartWall; j < nEndWall; j++, pWall++)
        {
            int nNextSector = pWall->nextsector;
            if (nNextSector < 0)
                continue;
            if (TestBitString(sectbits, nNextSector))
                continue;
            SetBitString(sectbits, nNextSector);
            int dx = abs(wall[pWall->point2].x - x)>>4;
            int dy = abs(wall[pWall->point2].y - y)>>4;
            if (dx < nDist && dy < nDist)
            {
                if (approxDist(dx, dy) < nDist)
                {
                    if (pSectBit)
                        SetBitString(pSectBit, nNextSector);
                    pSectors[n++] = nNextSector;
                }
            }
        }
        i++;
    }
    pSectors[n] = -1;
    return n;
}

int GetClosestSpriteSectors(int nSector, int x, int y, int nDist, uint8_t *pSectBit, short *pWalls, bool newSectCheckMethod)
{
    // by default this function fails with sectors that linked with wide spans, or there was more than one link to the same sector. for example...
    // E6M1: throwing TNT on the stone footpath while standing on the brown road will fail due to the start/end points of the span being too far away. it'll only do damage at one end of the road
    // E1M2: throwing TNT at the double doors while standing on the train platform
    // by setting newSectCheckMethod to true these issues will be resolved
    static short pSectors[kMaxSectors];
    uint8_t sectbits[(kMaxSectors+7)>>3];
    memset(sectbits, 0, sizeof(sectbits));
    pSectors[0] = nSector;
    SetBitString(sectbits, nSector);
    int n = 1, m = 0;
    int i = 0;
    if (pSectBit)
    {
        memset(pSectBit, 0, (kMaxSectors+7)>>3);
        SetBitString(pSectBit, nSector);
    }
    while (i < n) // scan through sectors
    {
        const int nCurSector = pSectors[i];
        const int nStartWall = sector[nCurSector].wallptr;
        const int nEndWall = nStartWall + sector[nCurSector].wallnum;
        for (int j = nStartWall; j < nEndWall; j++) // scan each wall of current sector for new sectors
        {
            const walltype *pWall = &wall[j];
            const int nNextSector = pWall->nextsector;
            if (nNextSector < 0) // if next wall isn't linked to a sector, skip
                continue;
            if (TestBitString(sectbits, nNextSector)) // if we've already checked this sector, skip
                continue;
            bool setSectBit = true;
            bool withinRange = false;
            if (!newSectCheckMethod) // original method
            {
                withinRange = CheckProximityWall(pWall->point2, x, y, nDist);
            }
            else // new method - first test edges and then wall span midpoints
            {
                for (int k = (j+1); k < nEndWall; k++) // scan through the rest of the sector's walls
                {
                    if (wall[k].nextsector == nNextSector) // if the next walls still reference the sector, then don't flag the sector as checked (yet)
                    {
                        setSectBit = false;
                        break;
                    }
                }
                const int nWallA = j;
                const int nWallB = wall[nWallA].point2;
                int x1 = wall[nWallA].x, y1 = wall[nWallA].y;
                int x2 = wall[nWallB].x, y2 = wall[nWallB].y;
                int point1Dist = approxDist(x-x1, y-y1); // setup edge distance needed for below loop (determines which point to shift closer to center)
                int point2Dist = approxDist(x-x2, y-y2);
                int nLength = approxDist(x1-x2, y1-y2);
                const int nDist4 = nDist<<4;
                nLength = ClipRange(nLength / (nDist4+(nDist4>>1)), 1, 4); // always test midpoint at least once, and never split more than 4 times
                for (int k = 0; true; k++) // check both points of wall and subdivide span into smaller chunks towards target
                {
                    withinRange = (point1Dist < nDist4) || (point2Dist < nDist4); // check if both points of span is within radius
                    if (withinRange)
                        break;
                    if (k == nLength) // reached end
                        break;
                    const int xcenter = (x1+x2)>>1, ycenter = (y1+y2)>>1;
                    if (point1Dist < point2Dist) // shift closest side of wall towards target point, and refresh point distance values
                    {
                        x2 = xcenter, y2 = ycenter;
                        point2Dist = approxDist(x-x2, y-y2);
                    }
                    else
                    {
                        x1 = xcenter, y1 = ycenter;
                        point1Dist = approxDist(x-x1, y-y1);
                    }
                }
            }
            if (withinRange) // if new sector is within range, set to current sector and test walls
            {
                setSectBit = true; // sector is within range, set the sector as checked
                if (pSectBit)
                    SetBitString(pSectBit, nNextSector);
                pSectors[n++] = nNextSector;
                if (pWalls && pWall->extra > 0)
                {
                    XWALL *pXWall = &xwall[pWall->extra];
                    if (pXWall->triggerVector && !pXWall->isTriggered && !pXWall->state)
                        pWalls[m++] = j;
                }
            }
            if (setSectBit)
                SetBitString(sectbits, nNextSector);
        }
        i++;
    }
    pSectors[n] = -1;
    if (pWalls) pWalls[m] = -1;
    return n;
}

int picWidth(short nPic, short repeat) {
    return ClipLow((tileWidth(nPic) * repeat) << 2, 0);
}

int picHeight(short nPic, short repeat) {
    return ClipLow((tileHeight(nPic) * repeat) << 2, 0);
}



END_BLD_NS
