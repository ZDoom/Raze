//-------------------------------------------------------------------------
/*
Copyright (C) 1996, 2003 - 3D Realms Entertainment
Copyright (C) 2000, 2003 - Matt Saettler (EDuke Enhancements)
Copyright (C) 2020 - Christoph Oelckers

This file is part of Enhanced Duke Nukem 3D version 1.5 - Atomic Edition

Duke Nukem 3D is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

Original Source: 1996 - Todd Replogle
Prepared for public release: 03/21/2003 - Charlie Wiederhold, 3D Realms

EDuke enhancements integrated: 04/13/2003 - Matt Saettler

Note: EDuke source was in transition.  Changes are in-progress in the
source as it is released.

*/
//-------------------------------------------------------------------------

#include "ns.h"
#include "global.h"
#include "build.h"
#include "names_d.h"
#include "dukeactor.h"
#include "precache.h"

BEGIN_DUKE_NS 

inline void tloadtile(int tilenum, int palnum = 0)
{
	assert(tilenum < MAXTILES);
	markTileForPrecache(tilenum, palnum);
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

static void cachespritenum(DDukeActor* actor)
{
	int maxc;
	int j;
	int pal = actor->spr.pal;

	if(ud.monsters_off && badguy(actor)) return;

	maxc = 1;

	switch(actor->spr.picnum)
	{
		case DTILE_HYDRENT:
			tloadtile(DTILE_BROKEFIREHYDRENT);
			for(j = DTILE_TOILETWATER; j < (DTILE_TOILETWATER+4); j++)
				tloadtile(j, pal);
			break;
		case DTILE_TOILET:
			tloadtile(DTILE_TOILETBROKE);
			for(j = DTILE_TOILETWATER; j < (DTILE_TOILETWATER+4); j++)
				tloadtile(j, pal);
			break;
		case DTILE_STALL:
			tloadtile(DTILE_STALLBROKE);
			for(j = DTILE_TOILETWATER; j < (DTILE_TOILETWATER+4); j++)
				tloadtile(j, pal);
			break;
		case DTILE_RUBBERCAN:
			maxc = 2;
			break;
		case DTILE_TOILETWATER:
			maxc = 4;
			break;
		case DTILE_FEMPIC1:
			maxc = 44;
			break;
		case DTILE_LIZTROOP:
		case DTILE_LIZTROOPRUNNING:
		case DTILE_LIZTROOPSHOOT:
		case DTILE_LIZTROOPJETPACK:
		case DTILE_LIZTROOPONTOILET:
		case DTILE_LIZTROOPDUCKING:
			for(j = DTILE_LIZTROOP; j < (DTILE_LIZTROOP+72); j++)
				   tloadtile(j, pal);
			for(j=DTILE_HEADJIB1;j<DTILE_LEGJIB1+3;j++)
					tloadtile(j, pal);
			maxc = 0;
			break;
		case DTILE_WOODENHORSE:
			maxc = 5;
			for(j = DTILE_HORSEONSIDE; j < (DTILE_HORSEONSIDE+4); j++)
					tloadtile(j, pal);
			break;
		case DTILE_NEWBEAST:
		case DTILE_NEWBEASTSTAYPUT:
			maxc = 90;
			break;
		case DTILE_BOSS1:
		case DTILE_BOSS2:
		case DTILE_BOSS3:
			maxc = 30;
			break;
		case DTILE_OCTABRAIN:
		case DTILE_OCTABRAINSTAYPUT:
		case DTILE_COMMANDER:
		case DTILE_COMMANDERSTAYPUT:
			maxc = 38;
			break;
		case DTILE_RECON:
			maxc = 13;
			break;
		case DTILE_PIGCOP:
		case DTILE_PIGCOPDIVE:
			maxc = 61;
			break;
		case DTILE_SHARK:
			maxc = 30;
			break;
		case DTILE_LIZMAN:
		case DTILE_LIZMANSPITTING:
		case DTILE_LIZMANFEEDING:
		case DTILE_LIZMANJUMP:
			for(j=DTILE_LIZMANHEAD1;j<DTILE_LIZMANLEG1+3;j++)
					tloadtile(j, pal);
			maxc = 80;
			break;
		case DTILE_APLAYER:
			maxc = 0;
			if(ud.multimode > 1)
			{
				maxc = 5;
				for(j = 1420;j < 1420+106; j++)
						tloadtile(j, pal);
			}
			break;
		case DTILE_ATOMICHEALTH:
			maxc = 14;
			break;
		case DTILE_DRONE:
			maxc = 10;
			break;
		case DTILE_EXPLODINGBARREL:
			maxc = 3;
			break;
		case DTILE_NUKEBARREL:
			maxc = 5;
			break;
	}

	for(j = actor->spr.picnum; j < (actor->spr.picnum+maxc); j++)
			tloadtile(j, pal);
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

static void cachegoodsprites(void)
{
	int i;

	tloadtile(DTILE_BOTTOMSTATUSBAR);
	if (ud.multimode > 1)
	{
		tloadtile(DTILE_FRAGBAR);
	}

	for(i=DTILE_FOOTPRINTS;i<DTILE_FOOTPRINTS+3;i++)
			tloadtile(i);

	for( i = DTILE_BURNING; i < DTILE_BURNING+14; i++)
			tloadtile(i);

	for( i = DTILE_BURNING2; i < DTILE_BURNING2+14; i++)
			tloadtile(i);

	for( i = DTILE_CRACKKNUCKLES; i < DTILE_CRACKKNUCKLES+4; i++)
			tloadtile(i);

	for( i = DTILE_FIRSTGUN; i < DTILE_FIRSTGUN+3 ; i++ )
			tloadtile(i);

	for( i = DTILE_EXPLOSION2; i < DTILE_EXPLOSION2+21 ; i++ )
			tloadtile(i);

	tloadtile(DTILE_BULLETHOLE);

	for( i = DTILE_FIRSTGUNRELOAD; i < DTILE_FIRSTGUNRELOAD+8 ; i++ )
			tloadtile(i);

	tloadtile(DTILE_FOOTPRINTS);

	for( i = DTILE_JIBS1; i < (DTILE_JIBS5+5); i++)
			tloadtile(i);

	for( i = DTILE_SCRAP1; i < (DTILE_SCRAP1+19); i++)
			tloadtile(i);

	for( i = DTILE_SMALLSMOKE; i < (DTILE_SMALLSMOKE+4); i++)
			tloadtile(i);
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void cacheit_d(void)
{
	if (!r_precache) return;

	cachegoodsprites();

	for (auto& wal : wall)
	{
		tloadtile(wal.wallpicnum, wal.pal);
		if (wal.overpicnum >= 0)
			tloadtile(wal.overpicnum, wal.pal);
	}

	for (auto& sect: sector)
	{
		tloadtile(sect.floorpicnum, sect.floorpal);
		tloadtile(sect.ceilingpicnum, sect.ceilingpal);

		DukeSectIterator it(&sect);
		while (auto act = it.Next())
		{
			if (act->spr.scale.X != 0 && act->spr.scale.Y != 0 && (act->spr.cstat & CSTAT_SPRITE_INVISIBLE) == 0)
				cachespritenum(act);
		}
	}

	precacheMarkedTiles();
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------
void spriteinit_d(DDukeActor* actor, TArray<DDukeActor*>& actors)
{
	bool res = initspriteforspawn(actor);
	if (res) spawninit_d(nullptr, actor, &actors);
}

void prelevel_d(int g, TArray<DDukeActor*>& actors)
{
	int i, j, lotaglist;
	TArray<short> lotags;

	prelevel_common(g);

	DukeStatIterator it(STAT_DEFAULT);
	while (auto ac = it.Next())
	{
		LoadActor(ac, -1, -1);

		if (ac->spr.lotag == -1 && (ac->spr.cstat & CSTAT_SPRITE_ALIGNMENT_WALL))
		{
			ps[0].Exit = ac->spr.pos.XY();
		}
		else
			premapcontroller(ac);
	}


	for (auto actor : actors)
	{
		if (actor->exists())
		{
			if (iseffector(actor) && actor->spr.lotag == SE_14_SUBWAY_CAR)
				continue;
			spriteinit_d(actor, actors);
		}
	}

	for (auto actor : actors)
	{
		if (actor->exists())
		{
			if (iseffector(actor) && actor->spr.lotag == SE_14_SUBWAY_CAR)
				spriteinit_d(actor, actors);
		}
	}
	lotaglist = 0;

	it.Reset(STAT_DEFAULT);
	while (auto actor = it.Next())
	{
		switch (actor->spr.picnum)
		{
		case DTILE_DIPSWITCHON:
		case DTILE_DIPSWITCH2ON:
		case DTILE_PULLSWITCHON:
		case DTILE_HANDSWITCHON:
		case DTILE_SLOTDOORON:
		case DTILE_LIGHTSWITCHON:
		case DTILE_SPACELIGHTSWITCHON:
		case DTILE_SPACEDOORSWITCHON:
		case DTILE_FRANKENSTINESWITCHON:
		case DTILE_LIGHTSWITCH2ON:
		case DTILE_POWERSWITCH1ON:
		case DTILE_LOCKSWITCH1ON:
		case DTILE_POWERSWITCH2ON:
			j = lotags.Find(actor->spr.lotag);
			if (j == lotags.Size())
			{
				lotags.Push(actor->spr.lotag);
				DukeStatIterator it1(STAT_EFFECTOR);
				while (auto ac = it1.Next())
				{
					if (ac->spr.lotag == SE_12_LIGHT_SWITCH && ac->spr.hitag == actor->spr.lotag)
						ac->temp_data[0] = 1;
				}
			}
			break;
		}
	}

	mirrorcnt = 0;

	for (auto& wal : wall)
	{
		if (wal.overpicnum == DTILE_MIRROR && (wal.cstat & CSTAT_WALL_1WAY) != 0)
		{
			auto sectp = wal.nextSector();

			if (mirrorcnt > 63)
				I_Error("Too many mirrors (64 max.)");
			if (sectp && sectp->ceilingpicnum != DTILE_MIRROR)
			{
				sectp->ceilingpicnum = DTILE_MIRROR;
				sectp->floorpicnum = DTILE_MIRROR;
				mirrorwall[mirrorcnt] = &wal;
				mirrorsector[mirrorcnt] = sectp;
				mirrorcnt++;
				continue;
			}
		}

		if (numanimwalls >= MAXANIMWALLS)
			I_Error("Too many 'anim' walls (max 512.)");

		animwall[numanimwalls].tag = 0;
		animwall[numanimwalls].wall = nullptr;

		switch (wal.overpicnum)
		{
		case DTILE_FANSHADOW:
		case DTILE_FANSPRITE:
			//wal.cstat |= CSTAT_WALL_BLOCK | CSTAT_WALL_BLOCK_HITSCAN; Original code assigned this to 'wall', i.e. wall[0]
			animwall[numanimwalls].wall = &wal;
			numanimwalls++;
			break;

		case DTILE_W_FORCEFIELD:
			for (int jj = 0; jj < 3; jj++)
				tloadtile(DTILE_W_FORCEFIELD + jj);
			[[fallthrough]];
		case DTILE_W_FORCEFIELD + 1:
		case DTILE_W_FORCEFIELD + 2:
			if (wal.shade > 31)
				wal.cstat = 0;
			else wal.cstat |= CSTAT_WALL_BLOCK | CSTAT_WALL_ALIGN_BOTTOM | CSTAT_WALL_MASKED | CSTAT_WALL_BLOCK_HITSCAN | CSTAT_WALL_YFLIP;

			if (wal.lotag && wal.twoSided())
				wal.nextWall()->lotag = wal.lotag;
			[[fallthrough]];

		case DTILE_BIGFORCE:

			animwall[numanimwalls].wall = &wal;
			numanimwalls++;

			continue;
		}

		wal.extra = -1;

		switch (wal.wallpicnum)
		{
		case DTILE_W_TECHWALL1:
		case DTILE_W_TECHWALL2:
		case DTILE_W_TECHWALL3:
		case DTILE_W_TECHWALL4:
			animwall[numanimwalls].wall = &wal;
			//                animwall[numanimwalls].tag = -1;
			numanimwalls++;
			break;
		case DTILE_SCREENBREAK6:
		case DTILE_SCREENBREAK7:
		case DTILE_SCREENBREAK8:
			for (int jj = DTILE_SCREENBREAK6; jj < DTILE_SCREENBREAK9; jj++)
				tloadtile(jj);
			animwall[numanimwalls].wall = &wal;
			animwall[numanimwalls].tag = -1;
			numanimwalls++;
			break;

		case DTILE_FEMPIC1:
		case DTILE_FEMPIC2:
		case DTILE_FEMPIC3:

			wal.extra = wal.wallpicnum;
			animwall[numanimwalls].tag = -1;

			animwall[numanimwalls].wall = &wal;
			animwall[numanimwalls].tag = wal.wallpicnum;
			numanimwalls++;
			break;

		case DTILE_SCREENBREAK1:
		case DTILE_SCREENBREAK2:
		case DTILE_SCREENBREAK3:
		case DTILE_SCREENBREAK4:
		case DTILE_SCREENBREAK5:

		case DTILE_SCREENBREAK9:
		case DTILE_SCREENBREAK10:
		case DTILE_SCREENBREAK11:
		case DTILE_SCREENBREAK12:
		case DTILE_SCREENBREAK13:
		case DTILE_SCREENBREAK14:
		case DTILE_SCREENBREAK15:
		case DTILE_SCREENBREAK16:
		case DTILE_SCREENBREAK17:
		case DTILE_SCREENBREAK18:
		case DTILE_SCREENBREAK19:

			animwall[numanimwalls].wall = &wal;
			animwall[numanimwalls].tag = wal.wallpicnum;
			numanimwalls++;
			break;
		}
	}

	//Invalidate textures in sector behind mirror
	for (i = 0; i < mirrorcnt; i++)
	{
		for (auto& wal : mirrorsector[i]->walls)
		{
			wal.wallpicnum = DTILE_MIRROR;
			wal.overpicnum = DTILE_MIRROR;
		}
	}
}

END_DUKE_NS
