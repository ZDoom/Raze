
#include "typedefs.h"
#include <string.h>
#include "player.h"
#include "init.h"
#include "engine.h"
#include "exhumed.h"

short bShowTowers = kTrue; //kFalse; REVERT to kFalse
long ldMapZoom;
long lMapZoom;

void MarkSectorSeen(short nSector);


void InitMap()
{
	memset(show2dsector, 0, sizeof(show2dsector));
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
	if (!((1 << (nSector & 7)) & show2dsector[nSector >> 3]))
	{
		show2dsector[nSector >> 3] |= 1 << (nSector & 7);

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

void drawoverheadmap(long cposx, long cposy, long czoom, short cang)
{
#ifndef __WATCOMC__ // FIXME - Won't compile on Watcom
	long xvect = sintable[(2048 - cang) & 2047] * czoom;
	long yvect = sintable[(1536 - cang) & 2047] * czoom;
	long xvect2 = mulscale(xvect, yxaspect, 16);
	long yvect2 = mulscale(yvect, yxaspect, 16);

	// draw player position arrow
	drawline256(xdim << 11, (ydim << 11) - 20480, xdim << 11, (ydim << 11) + 20480, 24);
	drawline256((xdim << 11) - 20480, ydim << 11, xdim << 11, (ydim << 11) - 20480, 24);
	drawline256((xdim << 11) + 20480, ydim << 11, xdim << 11, (ydim << 11) - 20480, 24);

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

		int startwallB = startwall;

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
							long ox = wall[nWall].x - cposx;
							long oy = wall[nWall].y - cposy;

							long x1 = mulscale(ox, xvect, 16) - mulscale(oy, yvect, 16);
							long y1 = mulscale(oy, xvect2, 16) + mulscale(ox, yvect2, 16);

							int nWall2 = wall[nWall].point2;
							ox = wall[nWall2].x - cposx;
							oy = wall[nWall2].y - cposy;
							long x2 = mulscale(ox, xvect, 16) - mulscale(oy, yvect, 16);
							long y2 = mulscale(oy, xvect2, 16) + mulscale(ox, yvect2, 16);

							drawline256(x1 + (xdim << 11), y1 + (ydim << 11), x2 + (xdim << 11), y2 + (ydim << 11), var_10);

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
						if (tilesizx[wall[nWall].picnum] && tilesizy[wall[nWall].picnum])
						{
							long ox = wall[nWall].x - cposx;
							long oy = wall[nWall].y - cposy;
							long x1 = mulscale(ox, xvect, 16) - mulscale(oy, yvect, 16);
							long y1 = mulscale(oy, xvect2, 16) + mulscale(ox, yvect2, 16);

							int nWall2 = wall[nWall].point2;
							ox = wall[nWall2].x - cposx;
							oy = wall[nWall2].y - cposy;
							long x2 = mulscale(ox, xvect, 16) - mulscale(oy, yvect, 16);
							long y2 = mulscale(oy, xvect2, 16) + mulscale(ox, yvect2, 16);

							drawline256(x1 + (xdim << 11), y1 + (ydim << 11), x2 + (xdim << 11), y2 + (ydim << 11), 24);

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
					long ox = sprite[nSprite].x - cposx; // var_64
					long oy = sprite[nSprite].y - cposx; // var_68

					// int var_58 = mulscale(var_64, xvect, 16) - mulscale(var_68, yvect, 16);
					long x1 = mulscale(ox, xvect, 16) - mulscale(oy, yvect, 16);
					long y1 = mulscale(oy, xvect2, 16) + mulscale(ox, yvect2, 16);

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

					drawline256(
						x1 - 2048 + (xdim << 11),
						y1 - 2048 + (ydim << 11),
						x1 - 2048 + (xdim << 11),
						y1 + 2048 + (ydim << 1),
						170);

					drawline256(
						x1 + (xdim << 11),
						y1 + (ydim << 11),
						x1 + (xdim << 11),
						y1 + 2048 + (ydim << 11),
						170);

					drawline256(
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

void UpdateMap()
{
	if (sector[initsect].ceilingpal != 3 || (nPlayerTorch[nLocalPlayer] != 0)) {
		MarkSectorSeen(initsect);
	}
}

void DrawMap()
{
	if (!nFreeze) {
		drawoverheadmap(initx, inity, lMapZoom, inita);
	}
}
