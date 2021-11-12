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

static void cachespritenum(spritetype *spr)
{
	int maxc;
	int j;
	int pal = spr->pal;

	if(ud.monsters_off && badguy(spr)) return;

	maxc = 1;

	switch(spr->picnum)
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

	for(j = spr->picnum; j < (spr->picnum+maxc); j++)
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

	tloadtile(VIEWSCREEN);

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
	int i;

	cachegoodsprites();

	for (i = 0; i < numwalls; i++)
	{
		tloadtile(wall[i].picnum, wall[i].pal);
		if (wall[i].overpicnum >= 0)
			tloadtile(wall[i].overpicnum, wall[i].pal);
	}

	for (i = 0; i < numsectors; i++)
	{
		auto sectp = &sector[i];
		tloadtile(sectp->floorpicnum, sectp->floorpal);
		tloadtile(sectp->ceilingpicnum, sectp->ceilingpal);
		if (sectp->ceilingpicnum == LA)
		{
			tloadtile(LA + 1);
			tloadtile(LA + 2);
		}

		DukeSectIterator it(i);
		while (auto j = it.Next())
		{
			if (j->s->xrepeat != 0 && j->s->yrepeat != 0 && (j->s->cstat & 32768) == 0)
				cachespritenum(j->s);
		}
	}

	precacheMarkedTiles();
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void prelevel_d(int g)
{
	int i, j, startwall, endwall, lotaglist;
	short lotags[65];

	prelevel_common(g);

	DukeStatIterator it(STAT_DEFAULT);
	while (auto ac = it.Next())
	{
		auto si = ac->s;
		LoadActor(ac, -1, -1);

		if (si->lotag == -1 && (si->cstat & 16))
		{
			ps[0].exitx = si->x;
			ps[0].exity = si->y;
		}
		else switch (si->picnum)
		{
		case GPSPEED:
			si->sector()->extra = si->lotag;
			deletesprite(ac);
			break;

		case CYCLER:
			if (numcyclers >= MAXCYCLERS)
				I_Error("Too many cycling sectors.");
			cyclers[numcyclers].sectnum = si->sectnum;
			cyclers[numcyclers].lotag = si->lotag;
			cyclers[numcyclers].shade1 = si->shade;
			cyclers[numcyclers].shade2 = si->sector()->floorshade;
			cyclers[numcyclers].hitag = si->hitag;
			cyclers[numcyclers].state = (si->ang == 1536);
			numcyclers++;
			deletesprite(ac);
			break;
		}
	}


	for (i = 0; i < MAXSPRITES; i++)
	{
		auto spr = &sprite[i];
		if (spr->statnum < MAXSTATUS)
		{
			if (spr->picnum == SECTOREFFECTOR && spr->lotag == SE_14_SUBWAY_CAR)
				continue;
			spawn(nullptr, i);
		}
	}

	for (i = 0; i < MAXSPRITES; i++)
	{
		auto spr = &sprite[i];
		if (spr->statnum < MAXSTATUS)
		{
			if (spr->picnum == SECTOREFFECTOR && spr->lotag == SE_14_SUBWAY_CAR)
				spawn(nullptr, i);
		}
	}
	lotaglist = 0;

	it.Reset(STAT_DEFAULT);
	while ((i = it.NextIndex()) >= 0)
	{
		auto spr = &sprite[i];
		switch (spr->picnum)
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
				if (spr->lotag == lotags[j])
					break;

			if (j == lotaglist)
			{
				lotags[lotaglist] = spr->lotag;
				lotaglist++;
				if (lotaglist > 64)
					I_Error("Too many switches (64 max).");

				DukeStatIterator it1(STAT_EFFECTOR);
				while (auto ac = it1.Next())
				{
					if (ac->s->lotag == 12 && ac->s->hitag == spr->lotag)
						ac->temp_data[0] = 1;
				}
			}
			break;
		}
	}

	mirrorcnt = 0;

	for (i = 0; i < numwalls; i++)
	{
		walltype* wal;
		wal = &wall[i];

		if (wal->overpicnum == MIRROR && (wal->cstat & 32) != 0)
		{
			j = wal->nextsector;
			auto sectp = &sector[j];

			if (mirrorcnt > 63)
				I_Error("Too many mirrors (64 max.)");
			if ((j >= 0) && sectp->ceilingpicnum != MIRROR)
			{
				sectp->ceilingpicnum = MIRROR;
				sectp->floorpicnum = MIRROR;
				mirrorwall[mirrorcnt] = i;
				mirrorsector[mirrorcnt] = j;
				mirrorcnt++;
				continue;
			}
		}

		if (numanimwalls >= MAXANIMWALLS)
			I_Error("Too many 'anim' walls (max 512.)");

		animwall[numanimwalls].tag = 0;
		animwall[numanimwalls].wallnum = 0;

		switch (wal->overpicnum)
		{
		case FANSHADOW:
		case FANSPRITE:
			wal->cstat |= 65;
			animwall[numanimwalls].wallnum = i;
			numanimwalls++;
			break;

		case W_FORCEFIELD:
			for (j = 0; j < 3; j++)
				tloadtile(W_FORCEFIELD + j);
		case W_FORCEFIELD + 1:
		case W_FORCEFIELD + 2:
			if (wal->shade > 31)
				wal->cstat = 0;
			else wal->cstat |= 85 + 256;

			if (wal->lotag && wal->nextwall >= 0)
				wal->nextWall()->lotag = wal->lotag;

		case BIGFORCE:

			animwall[numanimwalls].wallnum = i;
			numanimwalls++;

			continue;
		}

		wal->extra = -1;

		switch (wal->picnum)
		{
		case W_TECHWALL1:
		case W_TECHWALL2:
		case W_TECHWALL3:
		case W_TECHWALL4:
			animwall[numanimwalls].wallnum = i;
			//                animwall[numanimwalls].tag = -1;
			numanimwalls++;
			break;
		case SCREENBREAK6:
		case SCREENBREAK7:
		case SCREENBREAK8:
			for (j = SCREENBREAK6; j < SCREENBREAK9; j++)
				tloadtile(j);
			animwall[numanimwalls].wallnum = i;
			animwall[numanimwalls].tag = -1;
			numanimwalls++;
			break;

		case FEMPIC1:
		case FEMPIC2:
		case FEMPIC3:

			wal->extra = wal->picnum;
			animwall[numanimwalls].tag = -1;

			animwall[numanimwalls].wallnum = i;
			animwall[numanimwalls].tag = wal->picnum;
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

			animwall[numanimwalls].wallnum = i;
			animwall[numanimwalls].tag = wal->picnum;
			numanimwalls++;
			break;
		}
	}

	//Invalidate textures in sector behind mirror
	for (i = 0; i < mirrorcnt; i++)
	{
		startwall = sector[mirrorsector[i]].wallptr;
		endwall = startwall + sector[mirrorsector[i]].wallnum;
		for (j = startwall; j < endwall; j++)
		{
			wall[j].picnum = MIRROR;
			wall[j].overpicnum = MIRROR;
		}
	}
}

END_DUKE_NS