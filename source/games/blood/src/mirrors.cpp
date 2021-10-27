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
        if (mirrorcnt == 16)
            break;
        int nTile = 4080+mirrorcnt;
        if (wall[i].overpicnum == 504)
        {
            if (wall[i].extra > 0 && GetWallType(i) == kWallStack)
            {
                wall[i].overpicnum = nTile;

                mirror[mirrorcnt].wallnum = i;
                mirror[mirrorcnt].type = 0;
                wall[i].cstat |= 32;
                int tmp = xwall[wall[i].extra].data;
                int j;
                for (j = numwalls - 1; j >= 0; j--)
                {
                    if (j == i)
                        continue;
                    if (wall[j].extra > 0 && GetWallType(i) == kWallStack)
                    {
                        if (tmp != xwall[wall[j].extra].data)
                            continue;
                        wall[i].hitag = j;
                        wall[j].hitag = i;
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
                    wall[i].portalflags = PORTAL_WALL_VIEW;
                    wall[i].portalnum = j;
                }
            }
            continue;
        }
        if (wall[i].picnum == 504)
        {
            mirror[mirrorcnt].link = i;
            mirror[mirrorcnt].wallnum = i;
            wall[i].picnum = nTile;
            mirror[mirrorcnt].type = 0;
            wall[i].cstat |= 32;
            wall[i].portalflags = PORTAL_WALL_MIRROR;
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
            int nLink = gUpperLink[i];
            if (nLink < 0)
                continue;
            int nLink2 = sprite[nLink].owner /*& 0xfff*/;
            int j = sprite[nLink2].sectnum;
            if (sector[j].ceilingpicnum != 504)
                I_Error("Lower link sector %d doesn't have mirror picnum\n", j);
            mirror[mirrorcnt].type = 2;
            mirror[mirrorcnt].dx = sprite[nLink2].x-sprite[nLink].x;
            mirror[mirrorcnt].dy = sprite[nLink2].y-sprite[nLink].y;
            mirror[mirrorcnt].dz = sprite[nLink2].z-sprite[nLink].z;
            mirror[mirrorcnt].wallnum = i;
            mirror[mirrorcnt].link = j;
            sector[i].floorpicnum = 4080+mirrorcnt;
            sector[i].portalflags = PORTAL_SECTOR_FLOOR;
            sector[i].portalnum = portalAdd(PORTAL_SECTOR_FLOOR, j, mirror[mirrorcnt].dx, mirror[mirrorcnt].dy, mirror[mirrorcnt].dz);
            mirrorcnt++;
            mirror[mirrorcnt].type = 1;
            mirror[mirrorcnt].dx = sprite[nLink].x-sprite[nLink2].x;
            mirror[mirrorcnt].dy = sprite[nLink].y-sprite[nLink2].y;
            mirror[mirrorcnt].dz = sprite[nLink].z-sprite[nLink2].z;
            mirror[mirrorcnt].wallnum = j;
            mirror[mirrorcnt].link = i;
            sector[j].ceilingpicnum = 4080+mirrorcnt;
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
        wall[mirrorwall[i]].picnum = 504;
        wall[mirrorwall[i]].overpicnum = 504;
        wall[mirrorwall[i]].cstat = 0;
        wall[mirrorwall[i]].nextsector = -1;
        wall[mirrorwall[i]].nextwall = -1;
        wall[mirrorwall[i]].point2 = numwalls+i+1;
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
			wall[mirrorwall[i]].picnum = 504;
			wall[mirrorwall[i]].overpicnum = 504;
			wall[mirrorwall[i]].cstat = 0;
			wall[mirrorwall[i]].nextsector = -1;
			wall[mirrorwall[i]].nextwall = -1;
			wall[mirrorwall[i]].point2 = numwalls + i + 1;
		}
		wall[mirrorwall[3]].point2 = mirrorwall[0];
		sector[mirrorsector].ceilingpicnum = 504;
		sector[mirrorsector].floorpicnum = 504;
		sector[mirrorsector].wallptr = mirrorwall[0];
		sector[mirrorsector].wallnum = 4;
	}
}

END_BLD_NS
