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

#include "build.h"
#include "automap.h"
#include "savegamehelp.h"

#include "blood.h" 
#include "render.h"

BEGIN_BLD_NS

int mirrorcnt, mirrorsector, mirrorwall[4];

MIRROR mirror[16];

void InitMirrors(void)
{
    r_rortexture = 4080;
    r_rortexturerange = 16;

    mirrorcnt = 0;
    tileDelete(504);
    portalClear();
    
	for (int i = 0; i < 16; i++)
	{
		tileDelete(4080 + i);
	}
    for (int i = numwalls - 1; i >= 0; i--)
    {
        auto pWalli = &wall[i];
        if (mirrorcnt == 16)
            break;
        int nTile = 4080+mirrorcnt;
        if (pWalli->overpicnum == 504)
        {
            if (pWalli->extra > 0 && GetWallType(i) == kWallStack)
            {
                pWalli->overpicnum = nTile;

                mirror[mirrorcnt].wallnum = i;
                mirror[mirrorcnt].type = 0;
                pWalli->cstat |= 32;
                int tmp = pWalli->xw().data;
                int j;
                for (j = numwalls - 1; j >= 0; j--)
                {
                    if (j == i)
                        continue;
                    auto pWallj = &wall[j];
                    if (pWallj->extra > 0 && GetWallType(i) == kWallStack)
                    {
                        if (tmp != pWallj->xw().data)
                            continue;
                        pWalli->hitag = j;
                        pWallj->hitag = i;
                        mirror[mirrorcnt].link = j;
                        break;
                    }
                }
                if (j < 0)
                {
                    Printf(PRINT_HIGH, "wall[%d] has no matching wall link! (data=%d)\n", i, tmp);
                }
                else
                {
                    mirrorcnt++;
                    pWalli->portalflags = PORTAL_WALL_VIEW;
                    pWalli->portalnum = j;
                }
            }
            continue;
        }
        if (pWalli->picnum == 504)
        {
            mirror[mirrorcnt].link = i;
            mirror[mirrorcnt].wallnum = i;
            pWalli->picnum = nTile;
            mirror[mirrorcnt].type = 0;
            pWalli->cstat |= 32;
            pWalli->portalflags = PORTAL_WALL_MIRROR;
            mirrorcnt++;
            continue;
        }
    }
    for (int i = numsectors - 1; i >= 0; i--)
    {
        if (mirrorcnt >= 15)
            break;

        if (sector[i].floorpicnum == 504)
        {
            auto link = getUpperLink(i);
            if (link == nullptr)
                continue;
            auto link2 = link->GetOwner();
            if (link2 == nullptr)
                continue;
            int j = link2->s().sectnum;
            if (sector[j].ceilingpicnum != 504)
                I_Error("Lower link sector %d doesn't have mirror picnum\n", j);
            mirror[mirrorcnt].type = 2;
            mirror[mirrorcnt].dx = link2->s().x - link->s().x;
            mirror[mirrorcnt].dy = link2->s().y - link->s().y;
            mirror[mirrorcnt].dz = link2->s().z - link->s().z;
            mirror[mirrorcnt].wallnum = i;
            mirror[mirrorcnt].link = j;
            sector[i].floorpicnum = 4080 + mirrorcnt;
            sector[i].portalflags = PORTAL_SECTOR_FLOOR;
            sector[i].portalnum = portalAdd(PORTAL_SECTOR_FLOOR, j, mirror[mirrorcnt].dx, mirror[mirrorcnt].dy, mirror[mirrorcnt].dz);
            mirrorcnt++;
            mirror[mirrorcnt].type = 1;
            mirror[mirrorcnt].dx = link->s().x - link2->s().x;
            mirror[mirrorcnt].dy = link->s().y - link2->s().y;
            mirror[mirrorcnt].dz = link->s().z - link2->s().z;
            mirror[mirrorcnt].wallnum = j;
            mirror[mirrorcnt].link = i;
            sector[j].ceilingpicnum = 4080 + mirrorcnt;
            sector[j].portalflags = PORTAL_SECTOR_CEILING;
            sector[j].portalnum = portalAdd(PORTAL_SECTOR_CEILING, i, mirror[mirrorcnt].dx, mirror[mirrorcnt].dy, mirror[mirrorcnt].dz);
            mirrorcnt++;
        }
    }
    mirrorsector = numsectors;
    mergePortals();
#if 1 // The new backend won't need this shit anymore.
    for (int i = 0; i < 4; i++)
    {
        mirrorwall[i] = numwalls+i;
        auto pWall = &wall[mirrorwall[i]];
        pWall->picnum = 504;
        pWall->overpicnum = 504;
        pWall->cstat = 0;
        pWall->nextsector = -1;
        pWall->nextwall = -1;
        pWall->point2 = numwalls+i+1;
    }
    wall[mirrorwall[3]].point2 = mirrorwall[0];
    sector[mirrorsector].ceilingpicnum = 504;
    sector[mirrorsector].floorpicnum = 504;
    sector[mirrorsector].wallptr = mirrorwall[0];
    sector[mirrorsector].wallnum = 4;
#endif
}

void TranslateMirrorColors(int nShade, int nPalette)
{
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

FSerializer& Serialize(FSerializer& arc, const char* keyname, MIRROR& w, MIRROR* def)
{
	if (arc.BeginObject(keyname))
	{
		arc ("type", w.type)
			("link", w.link)
			("dx", w.dx)
			("dy", w.dy)
			("dz", w.dz)
			("wallnum", w.wallnum)
			.EndObject();
	}
	return arc;
}

void SerializeMirrors(FSerializer& arc)
{
	if (arc.BeginObject("mirror"))
	{
		arc("mirrorcnt", mirrorcnt)
			("mirrorsector", mirrorsector)
			.Array("mirror", mirror, countof(mirror))
			.Array("mirrorwall", mirrorwall, countof(mirrorwall))
			.EndObject();
	}

	if (arc.isReading())
	{

		tileDelete(504);

#ifdef USE_OPENGL
		r_rortexture = 4080;
		r_rortexturerange = 16;

#endif

		for (int i = 0; i < 16; i++)
		{
			tileDelete(4080 + i);
		}
		for (int i = 0; i < 4; i++)
		{
            auto pWall = &wall[mirrorwall[i]];
            pWall->picnum = 504;
			pWall->overpicnum = 504;
			pWall->cstat = 0;
			pWall->nextsector = -1;
			pWall->nextwall = -1;
			pWall->point2 = numwalls + i + 1;
		}
		wall[mirrorwall[3]].point2 = mirrorwall[0];
		sector[mirrorsector].ceilingpicnum = 504;
		sector[mirrorsector].floorpicnum = 504;
		sector[mirrorsector].wallptr = mirrorwall[0];
		sector[mirrorsector].wallnum = 4;
	}
}

END_BLD_NS
