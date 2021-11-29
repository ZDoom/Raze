//-------------------------------------------------------------------------
/*
Copyright (C) 1997, 2005 - 3D Realms Entertainment

This file is part of Shadow Warrior version 1.2

Shadow Warrior is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.

Original Source: 1997 - Frank Maddin and Jim Norwood
Prepared for public release: 03/28/2005 - Charlie Wiederhold, 3D Realms
*/
//-------------------------------------------------------------------------

/****************************************
Removed sounds that were causing "Could
not load" error messages.
****************************************/
#include "ns.h"

#include "build.h"

#include "names2.h"
#include "game.h"
#include "tags.h"
#include "break.h"
#include "pal.h"
#include "misc.h"
#include "sounds.h"
#include "network.h"
#include "precache.h"

BEGIN_SW_NS

// Run the game with the -CACHEPRINT option and redirect to a file.
// It will save out the tile and sound number every time one caches.
//
// sw -map $bullet -cacheprint > foofile

void PreCacheRange(int start_pic, int end_pic, int pal = 0)
{
	for (int j = start_pic; j <= end_pic; j++)
	{
		markTileForPrecache(j, pal);
	}
}

void PreCacheOverride(void)
{
	SWStatIterator it(STAT_CEILING_FLOOR_PIC_OVERRIDE);
	while (auto actor = it.Next())
	{
		auto sp = &actor->s();
		int j = SP_TAG2(sp);
		if(j >= 0 && j <= MAXTILES)
			markTileForPrecache(j, 0);
	}
}

void precacheMap(void)
{
	int i;
	int j;
	SECTORp sectp;
	WALLp wp;
	SPRITEp sp;

	for(auto& sec : sectors())
	{
		j = sec.ceilingpicnum;
		markTileForPrecache(j, sec.ceilingpal);

		if (TEST(picanm[j].sf, PICANM_ANIMTYPE_MASK) >> PICANM_ANIMTYPE_SHIFT)
		{
			for (i = 1; i <= picanm[j].num; i++)
			{
				markTileForPrecache(j + i, sec.ceilingpal);
			}
		}

		j = sec.floorpicnum;

		markTileForPrecache(j, sec.floorpal);

		if (TEST(picanm[j].sf, PICANM_ANIMTYPE_MASK) >> PICANM_ANIMTYPE_SHIFT)
		{
			for (i = 1; i <= picanm[j].num; i++)
			{
				markTileForPrecache(j + i, sec.floorpal);
			}
		}

	}

	for (auto& wal : walls())
	{
		j = wal.picnum;

		markTileForPrecache(j, wal.pal);

		if (TEST(picanm[j].sf, PICANM_ANIMTYPE_MASK) >> PICANM_ANIMTYPE_SHIFT)
		{
			for (i = 1; i <= picanm[j].num; i++)
			{
				markTileForPrecache(j + i, wal.pal);
			}
		}

		if (wal.overpicnum > 0 && wal.overpicnum < MAXTILES)
		{
			j = wal.overpicnum;
			markTileForPrecache(j, wal.pal);

			if (TEST(picanm[j].sf, PICANM_ANIMTYPE_MASK) >> PICANM_ANIMTYPE_SHIFT)
			{
				for (i = 1; i <= picanm[j].num; i++)
				{
					markTileForPrecache(j + i, wal.pal);
				}
			}
		}
	}
}

void SetupPreCache(void)
{
	precacheMap();

	   
	// actors cache ranges are called from SpriteSetup
	// only caches the actor if its on the level

	// weapons
	PreCacheRange(2000, 2227);
	PreCacheRange(4090, 4093);
	// Explosions
	PreCacheRange(3072, 3225);
	// ninja player character
	PreCacheRange(1024, 1175);
	// console
	PreCacheRange(2380, 2409);
	PreCacheRange(3600, 3645);
	PreCacheRange(2434, 2435);
	// common
	PreCacheRange(204, 208);
	// message font
	PreCacheRange(4608, 4701);
	// gibs
	PreCacheRange(1150,1568);
	PreCacheRange(1685,1690);
	PreCacheRange(900,944);
	PreCacheRange(1670,1681);
	// blood
	PreCacheRange(1710,1715);
	PreCacheRange(2410,2425);
	PreCacheRange(389,389); // blood puddle by itself in art file
	PreCacheRange(2500,2503);
	// shrap
	PreCacheRange(3840,3911);
	PreCacheRange(3924,3947);
	PreCacheRange(1397,1398);
	// water *** animated tiles, can be deleted now ***
	// PreCacheRange(780,794);
	// switches
	PreCacheRange(561,584);
	PreCacheRange(551,552);
	PreCacheRange(1846,1847);
	PreCacheRange(1850,1859);
	// bullet smoke
	PreCacheRange(1748,1753);
	// gas can
	PreCacheRange(3038,3042);
	// lava *** animated tiles, can be deleted now ***
	// PreCacheRange(175,182);
	// gas clouds & teleport effect
	PreCacheRange(3240,3277);
	// nuke mushroom cloud
	PreCacheRange(3280,3300);
	// blood drops
	PreCacheRange(1718,1721);
	// smoke
	PreCacheRange(3948,3968);
	// footprints
	PreCacheRange(2490,2492);
	// player fists
	PreCacheRange(4070,4077);
	PreCacheRange(4050,4051);
	PreCacheRange(4090,4093);
	// fish actor
	PreCacheRange(3760,3771);
	PreCacheRange(3780,3795);
	// coins
	PreCacheRange(2531,2533);
	// respawn markers & console keys
	PreCacheRange(2440,2467);
	// light/torch sprites
	PreCacheRange(537,548);
	PreCacheRange(521,528);
	PreCacheRange(512,515);
	PreCacheRange(396,399);
	PreCacheRange(443,446);
	// bubbles
	PreCacheRange(716,720);
	// bullet splashes
	PreCacheRange(772,776);
}

void PreCacheRipper(int pal)
{
	PreCacheRange(1580, 1644, pal);
}

void PreCacheRipper2(int pal)
{
	PreCacheRange(4320, 4427, pal);
}

void PreCacheGhost(int pal)
{
	PreCacheRange(4277, 4312, pal);
}

void PreCacheCoolie(int pal)
{
	PreCacheGhost(pal);
	PreCacheRange(1400, 1440, pal);
	PreCacheRange(4260, 4276, pal); // coolie explode
}

void PreCacheSerpent(int pal)
{
	PreCacheRange(960, 1016, pal);
	PreCacheRange(1300, 1314, pal);
}

void PreCacheGuardian(int pal)
{
	PreCacheRange(1469,1497, pal);
}

void PreCacheNinja(int pal)
{
	PreCacheRange(4096, 4239, pal);
}

void PreCacheNinjaGirl(int pal)
{
	PreCacheRange(5162, 5260, pal);
}

void PreCacheSumo(int pal)
{
	PreCacheRange(4490, 4544, pal);
}

void PreCacheZilla(int pal)
{
	PreCacheRange(4490, 4544, pal);
}

void PreCacheEel(int pal)
{
	PreCacheRange(4430, 4479, pal);
}

void PreCacheToiletGirl(int pal)
{
	PreCacheRange(5023, 5027, pal);
}

void PreCacheWashGirl(int pal)
{
	PreCacheRange(5032, 5035, pal);
}

void PreCacheCarGirl(int pal)
{
	PreCacheRange(4594,4597, pal);
}

void PreCacheMechanicGirl(int pal)
{
	PreCacheRange(4590,4593, pal);
}

void PreCacheSailorGirl(int pal)
{
	PreCacheRange(4600,4602, pal);
}

void PreCachePruneGirl(int pal)
{
	PreCacheRange(4604,4604, pal);
}

void PreCacheTrash(int pal)
{
	PreCacheRange(2540, 2546, pal);
}

void PreCacheBunny(int pal)
{
	PreCacheRange(4550, 4584, pal);
}

void PreCacheSkel(int pal)
{
	PreCacheRange(1320, 1396, pal);
}

void PreCacheHornet(int pal)
{
	PreCacheRange(800, 811, pal);
}

void PreCacheSkull(int pal)
{
	PreCacheRange(820, 854, pal);
}

void PreCacheBetty(int pal)
{
	PreCacheRange(817, 819, pal);
}

void PreCachePachinko(int pal)
{
	PreCacheRange(618,623, pal);
	PreCacheRange(618,623, pal);
	PreCacheRange(4768,4790, pal);
	PreCacheRange(4792,4814, pal);
	PreCacheRange(4816,4838, pal);
	PreCacheRange(4840,4863, pal);
}

void
PreCacheActor(void)
{
	int pic;

	SWSpriteIterator it;
	while (auto actor = it.Next())
	{
		auto pUsr = actor->u();
		auto pSpr = &actor->s();
		if (pSpr->statnum >= MAXSTATUS)
			continue;

		if (pUsr)
			pic = pUsr->ID;
		else
			pic = pSpr->picnum;

		switch (pic)
		{
		case COOLIE_RUN_R0:
			PreCacheCoolie(pSpr->pal);
			break;

		case NINJA_RUN_R0:
		case NINJA_CRAWL_R0:
			PreCacheNinja(pSpr->pal);
			break;

		case GORO_RUN_R0:
			PreCacheGuardian(pSpr->pal);
			break;

		case 1441:
		case COOLG_RUN_R0:
			PreCacheGhost(pSpr->pal);
			break;

		case EEL_RUN_R0:
			PreCacheEel(pSpr->pal);
			break;

		case SUMO_RUN_R0:
			PreCacheZilla(pSpr->pal);
			break;

		case ZILLA_RUN_R0:
			PreCacheSumo(pSpr->pal);
			break;

		case TOILETGIRL_R0:
			PreCacheToiletGirl(pSpr->pal);
			break;

		case WASHGIRL_R0:
			PreCacheWashGirl(pSpr->pal);
			break;

		case CARGIRL_R0:
			PreCacheCarGirl(pSpr->pal);
			break;

		case MECHANICGIRL_R0:
			PreCacheMechanicGirl(pSpr->pal);
			break;

		case SAILORGIRL_R0:
			PreCacheSailorGirl(pSpr->pal);
			break;

		case PRUNEGIRL_R0:
			PreCachePruneGirl(pSpr->pal);
			break;

		case TRASHCAN:
			PreCacheTrash(pSpr->pal);
			break;

		case BUNNY_RUN_R0:
			PreCacheBunny(pSpr->pal);
			break;

		case RIPPER_RUN_R0:
			PreCacheRipper(pSpr->pal);
			break;

		case RIPPER2_RUN_R0:
			PreCacheRipper2(pSpr->pal);
			break;

		case SERP_RUN_R0:
			PreCacheSerpent(pSpr->pal);
			break;

		case LAVA_RUN_R0:
			break;

		case SKEL_RUN_R0:
			PreCacheSkel(pSpr->pal);
			break;

		case HORNET_RUN_R0:
			PreCacheHornet(pSpr->pal);
			break;

		case SKULL_R0:
			PreCacheSkull(pSpr->pal);
			break;

		case BETTY_R0:
			PreCacheBetty(pSpr->pal);
			break;

		case GIRLNINJA_RUN_R0:
			PreCacheNinjaGirl(pSpr->pal);
			break;

		case 623:   // Pachinko win light
		case PACHINKO1:
		case PACHINKO2:
		case PACHINKO3:
		case PACHINKO4:
			PreCachePachinko(pSpr->pal);
			break;

		default:
			markTileForPrecache(pic, pSpr->pal);
		}
	}
}


void DoTheCache(void)
{
	if (r_precache)
	{
		SetupPreCache();
		PreCacheActor();
		PreCacheOverride();
		precacheMarkedTiles();
	}
}



END_SW_NS
