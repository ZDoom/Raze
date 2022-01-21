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
		case HYDRENT:
			tloadtile(BROKEFIREHYDRENT);
			for(j = TOILETWATER; j < (TOILETWATER+4); j++)
				tloadtile(j, pal);
			break;
		case TOILET:
			tloadtile(TOILETBROKE);
			for(j = TOILETWATER; j < (TOILETWATER+4); j++)
				tloadtile(j, pal);
			break;
		case STALL:
			tloadtile(STALLBROKE);
			for(j = TOILETWATER; j < (TOILETWATER+4); j++)
				tloadtile(j, pal);
			break;
		case RUBBERCAN:
			maxc = 2;
			break;
		case TOILETWATER:
			maxc = 4;
			break;
		case FEMPIC1:
			maxc = 44;
			break;
		case LIZTROOP:
		case LIZTROOPRUNNING:
		case LIZTROOPSHOOT:
		case LIZTROOPJETPACK:
		case LIZTROOPONTOILET:
		case LIZTROOPDUCKING:
			for(j = LIZTROOP; j < (LIZTROOP+72); j++)
				   tloadtile(j, pal);
			for(j=HEADJIB1;j<LEGJIB1+3;j++)
					tloadtile(j, pal);
			maxc = 0;
			break;
		case WOODENHORSE:
			maxc = 5;
			for(j = HORSEONSIDE; j < (HORSEONSIDE+4); j++)
					tloadtile(j, pal);
			break;
		case NEWBEAST:
		case NEWBEASTSTAYPUT:
			maxc = 90;
			break;
		case BOSS1:
		case BOSS2:
		case BOSS3:
			maxc = 30;
			break;
		case OCTABRAIN:
		case OCTABRAINSTAYPUT:
		case COMMANDER:
		case COMMANDERSTAYPUT:
			maxc = 38;
			break;
		case RECON:
			maxc = 13;
			break;
		case PIGCOP:
		case PIGCOPDIVE:
			maxc = 61;
			break;
		case SHARK:
			maxc = 30;
			break;
		case LIZMAN:
		case LIZMANSPITTING:
		case LIZMANFEEDING:
		case LIZMANJUMP:
			for(j=LIZMANHEAD1;j<LIZMANLEG1+3;j++)
					tloadtile(j, pal);
			maxc = 80;
			break;
		case APLAYER:
			maxc = 0;
			if(ud.multimode > 1)
			{
				maxc = 5;
				for(j = 1420;j < 1420+106; j++)
						tloadtile(j, pal);
			}
			break;
		case ATOMICHEALTH:
			maxc = 14;
			break;
		case DRONE:
			maxc = 10;
			break;
		case EXPLODINGBARREL:
		case SEENINE:
		case OOZFILTER:
			maxc = 3;
			break;
		case NUKEBARREL:
		case CAMERA1:
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

	tloadtile(BOTTOMSTATUSBAR);
	if (ud.multimode > 1)
	{
		tloadtile(FRAGBAR);
		for (i = MINIFONT; i < MINIFONT + 63; i++)
			tloadtile(i);
	}

	for(i=FOOTPRINTS;i<FOOTPRINTS+3;i++)
			tloadtile(i);

	for( i = BURNING; i < BURNING+14; i++)
			tloadtile(i);

	for( i = BURNING2; i < BURNING2+14; i++)
			tloadtile(i);

	for( i = CRACKKNUCKLES; i < CRACKKNUCKLES+4; i++)
			tloadtile(i);

	for( i = FIRSTGUN; i < FIRSTGUN+3 ; i++ )
			tloadtile(i);

	for( i = EXPLOSION2; i < EXPLOSION2+21 ; i++ )
			tloadtile(i);

	tloadtile(BULLETHOLE);

	for( i = FIRSTGUNRELOAD; i < FIRSTGUNRELOAD+8 ; i++ )
			tloadtile(i);

	tloadtile(FOOTPRINTS);

	for( i = JIBS1; i < (JIBS5+5); i++)
			tloadtile(i);

	for( i = SCRAP1; i < (SCRAP1+19); i++)
			tloadtile(i);

	for( i = SMALLSMOKE; i < (SMALLSMOKE+4); i++)
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
		tloadtile(wal.picnum, wal.pal);
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
			if (act->spr.xrepeat != 0 && act->spr.yrepeat != 0 && (act->spr.cstat & CSTAT_SPRITE_INVISIBLE) == 0)
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
	short lotags[65];

	prelevel_common(g);

	DukeStatIterator it(STAT_DEFAULT);
	while (auto ac = it.Next())
	{
		LoadActor(ac, -1, -1);

		if (ac->spr.lotag == -1 && (ac->spr.cstat & CSTAT_SPRITE_ALIGNMENT_WALL))
		{
			ps[0].exit.X = ac->spr.pos.X;
			ps[0].exit.Y = ac->spr.pos.Y;
		}
		else switch (ac->spr.picnum)
		{
		case GPSPEED:
			ac->sector()->extra = ac->spr.lotag;
			deletesprite(ac);
			break;

		case CYCLER:
			if (numcyclers >= MAXCYCLERS)
				I_Error("Too many cycling sectors.");
			cyclers[numcyclers].sector = ac->sector();
			cyclers[numcyclers].lotag = ac->spr.lotag;
			cyclers[numcyclers].shade1 = ac->spr.shade;
			cyclers[numcyclers].shade2 = ac->sector()->floorshade;
			cyclers[numcyclers].hitag = ac->spr.hitag;
			cyclers[numcyclers].state = (ac->spr.ang == 1536);
			numcyclers++;
			deletesprite(ac);
			break;
		}
	}


	for (auto actor : actors)
	{
		if (actor->exists())
		{
			if (actor->spr.picnum == SECTOREFFECTOR && actor->spr.lotag == SE_14_SUBWAY_CAR)
				continue;
			spriteinit_d(actor, actors);
		}
	}

	for (auto actor : actors)
	{
		if (actor->exists())
		{
			if (actor->spr.picnum == SECTOREFFECTOR && actor->spr.lotag == SE_14_SUBWAY_CAR)
				spriteinit_d(actor, actors);
		}
	}
	lotaglist = 0;

	it.Reset(STAT_DEFAULT);
	while (auto actor = it.Next())
	{
		switch (actor->spr.picnum)
		{
		case DIPSWITCH + 1:
		case DIPSWITCH2 + 1:
		case PULLSWITCH + 1:
		case HANDSWITCH + 1:
		case SLOTDOOR + 1:
		case LIGHTSWITCH + 1:
		case SPACELIGHTSWITCH + 1:
		case SPACEDOORSWITCH + 1:
		case FRANKENSTINESWITCH + 1:
		case LIGHTSWITCH2 + 1:
		case POWERSWITCH1 + 1:
		case LOCKSWITCH1 + 1:
		case POWERSWITCH2 + 1:
			for (j = 0; j < lotaglist; j++)
				if (actor->spr.lotag == lotags[j])
					break;

			if (j == lotaglist)
			{
				lotags[lotaglist] = actor->spr.lotag;
				lotaglist++;
				if (lotaglist > 64)
					I_Error("Too many switches (64 max).");

				DukeStatIterator it1(STAT_EFFECTOR);
				while (auto ac = it1.Next())
				{
					if (ac->spr.lotag == 12 && ac->spr.hitag == actor->spr.lotag)
						ac->temp_data[0] = 1;
				}
			}
			break;
		}
	}

	mirrorcnt = 0;

	for (auto& wal : wall)
	{
		if (wal.overpicnum == MIRROR && (wal.cstat & CSTAT_WALL_1WAY) != 0)
		{
			auto sectp = wal.nextSector();

			if (mirrorcnt > 63)
				I_Error("Too many mirrors (64 max.)");
			if (sectp && sectp->ceilingpicnum != MIRROR)
			{
				sectp->ceilingpicnum = MIRROR;
				sectp->floorpicnum = MIRROR;
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
		case FANSHADOW:
		case FANSPRITE:
			wal.cstat |= CSTAT_WALL_BLOCK | CSTAT_WALL_BLOCK_HITSCAN;
			animwall[numanimwalls].wall = &wal;
			numanimwalls++;
			break;

		case W_FORCEFIELD:
			for (int jj = 0; jj < 3; jj++)
				tloadtile(W_FORCEFIELD + jj);
			[[fallthrough]];
		case W_FORCEFIELD + 1:
		case W_FORCEFIELD + 2:
			if (wal.shade > 31)
				wal.cstat = 0;
			else wal.cstat |= CSTAT_WALL_BLOCK | CSTAT_WALL_ALIGN_BOTTOM | CSTAT_WALL_MASKED | CSTAT_WALL_BLOCK_HITSCAN | CSTAT_WALL_YFLIP;

			if (wal.lotag && wal.twoSided())
				wal.nextWall()->lotag = wal.lotag;
			[[fallthrough]];

		case BIGFORCE:

			animwall[numanimwalls].wall = &wal;
			numanimwalls++;

			continue;
		}

		wal.extra = -1;

		switch (wal.picnum)
		{
		case W_TECHWALL1:
		case W_TECHWALL2:
		case W_TECHWALL3:
		case W_TECHWALL4:
			animwall[numanimwalls].wall = &wal;
			//                animwall[numanimwalls].tag = -1;
			numanimwalls++;
			break;
		case SCREENBREAK6:
		case SCREENBREAK7:
		case SCREENBREAK8:
			for (int jj = SCREENBREAK6; jj < SCREENBREAK9; jj++)
				tloadtile(jj);
			animwall[numanimwalls].wall = &wal;
			animwall[numanimwalls].tag = -1;
			numanimwalls++;
			break;

		case FEMPIC1:
		case FEMPIC2:
		case FEMPIC3:

			wal.extra = wal.picnum;
			animwall[numanimwalls].tag = -1;

			animwall[numanimwalls].wall = &wal;
			animwall[numanimwalls].tag = wal.picnum;
			numanimwalls++;
			break;

		case SCREENBREAK1:
		case SCREENBREAK2:
		case SCREENBREAK3:
		case SCREENBREAK4:
		case SCREENBREAK5:

		case SCREENBREAK9:
		case SCREENBREAK10:
		case SCREENBREAK11:
		case SCREENBREAK12:
		case SCREENBREAK13:
		case SCREENBREAK14:
		case SCREENBREAK15:
		case SCREENBREAK16:
		case SCREENBREAK17:
		case SCREENBREAK18:
		case SCREENBREAK19:

			animwall[numanimwalls].wall = &wal;
			animwall[numanimwalls].tag = wal.picnum;
			numanimwalls++;
			break;
		}
	}

	//Invalidate textures in sector behind mirror
	for (i = 0; i < mirrorcnt; i++)
	{
		for (auto& wal : wallsofsector(mirrorsector[i]))
		{
			wal.picnum = MIRROR;
			wal.overpicnum = MIRROR;
		}
	}
}

END_DUKE_NS
