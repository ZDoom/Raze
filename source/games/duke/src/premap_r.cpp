//-------------------------------------------------------------------------
/*
Copyright (C) 1996, 2003 - 3D Realms Entertainment
Copyright (C) 2017-2019 Nuke.YKT
Copyright (C) 2020 - Christoph Oelckers

This file is part of Duke Nukem 3D version 1.5 - Atomic Edition

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
*/
//-------------------------------------------------------------------------

#include "ns.h"
#include "global.h"
#include "names_r.h"
#include "mapinfo.h"
#include "dukeactor.h"
#include "precache.h"

BEGIN_DUKE_NS 

static inline void tloadtile(int tilenum, int palnum = 0)
{
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

	if (ud.monsters_off && badguy(spr)) return;

	maxc = 1;

	switch (spr->picnum)
	{
	case HYDRENT:
		tloadtile(BROKEFIREHYDRENT);
		for (j = TOILETWATER; j < (TOILETWATER + 4); j++)
			tloadtile(j, pal);
		break;
	case RRTILE2121:
	case RRTILE2122:
		tloadtile(spr->picnum, pal);
		break;
	case TOILET:
		tloadtile(TOILETBROKE);
		for (j = TOILETWATER; j < (TOILETWATER + 4); j++)
			tloadtile(j, pal);
		break;
	case STALL:
		tloadtile(STALLBROKE);
		for (j = TOILETWATER; j < (TOILETWATER + 4); j++)
			tloadtile(j, pal);
		break;
	case FORCERIPPLE:
		maxc = 9;
		break;
	case RUBBERCAN:
		maxc = 2;
		break;
	case TOILETWATER:
		maxc = 4;
		break;
	case BUBBASTAND:
		for (j = BUBBASCRATCH; j <= (BUBBASCRATCH + 47); j++)
			tloadtile(j, pal);
		maxc = 0;
		break;
	case SBSWIPE:
		if (isRRRA())
			for (j = SBSWIPE; j <= (SBSWIPE + 29); j++)
				tloadtile(j, pal);
		maxc = 0;
		break;

	case COOT:
		for (j = COOT; j <= (COOT + 217); j++)
			tloadtile(j, pal);
		for (j = COOTJIBA; j < COOTJIBC + 4; j++)
			tloadtile(j, pal);
		maxc = 0;
		break;
	case LTH:
		maxc = 105;
		for (j = LTH; j < (LTH + maxc); j++)
			tloadtile(j, pal);
		maxc = 0;
		break;
	case BILLYRAY:
		maxc = 144;
		for (j = BILLYWALK; j < (BILLYWALK + maxc); j++)
			tloadtile(j, pal);
		for (j = BILLYJIBA; j <= BILLYJIBB + 4; j++)
			tloadtile(j, pal);
		maxc = 0;
		break;
	case COW:
		maxc = 56;
		for (j = spr->picnum; j < (spr->picnum + maxc); j++)
			tloadtile(j, pal);
		maxc = 0;
		break;
	case DOGRUN:
		for (j = DOGATTACK; j <= DOGATTACK + 35; j++)
			tloadtile(j, pal);
		for (j = DOGRUN; j <= DOGRUN + 80; j++)
			tloadtile(j, pal);
		maxc = 0;
		break;
	case RABBIT:
		if (isRRRA())
		{
			for (j = RABBIT; j <= RABBIT + 54; j++)
				tloadtile(j, pal);
			for (j = RABBIT + 56; j <= RABBIT + 56 + 49; j++)
				tloadtile(j, pal);
			for (j = RABBIT + 56; j <= RABBIT + 56 + 49; j++)
				tloadtile(j, pal);
			maxc = 0;
		}
		break;
	case BIKERB:
	case BIKERBV2:
		if (isRRRA())
		{
			for (j = BIKERB; j <= BIKERB + 104; j++)
				tloadtile(j, pal);
			maxc = 0;
		}
		break;
	case BIKER:
		if (isRRRA())
		{
			for (j = BIKER; j <= BIKER + 116; j++)
				tloadtile(j, pal);
			for (j = BIKER + 150; j <= BIKER + 150 + 104; j++)
				tloadtile(j, pal);
			maxc = 0;
		}
		break;
	case CHEER:
		if (isRRRA())
		{
			for (j = CHEER; j <= CHEER + 44; j++)
				tloadtile(j, pal);
			for (j = CHEER + 47; j <= CHEER + 47 + 211; j++)
				tloadtile(j, pal);
			for (j = CHEER + 262; j <= CHEER + 262 + 72; j++)
				tloadtile(j, pal);
			maxc = 0;
		}
		break;
	case CHEERB:
		if (isRRRA())
		{
			for (j = CHEERB; j <= CHEERB + 83; j++)
				tloadtile(j, pal);
			for (j = CHEERB + 157; j <= CHEERB + 157 + 83; j++)
				tloadtile(j, pal);
			maxc = 0;
		}
		break;
	case MAMA:
		if (isRRRA())
		{
			for (j = MAMA; j <= MAMA + 78; j++)
				tloadtile(j, pal);
			for (j = MAMA + 80; j <= MAMA + 80 + 7; j++)
				tloadtile(j, pal);
			for (j = MAMA + 90; j <= MAMA + 90 + 94; j++)
				tloadtile(j, pal);
			maxc = 0;
		}
		break;
	case CHEERBOAT:
		if (isRRRA())
		{
			tloadtile(CHEERBOAT);
			maxc = 0;
		}
		break;
	case HULKBOAT:
		if (isRRRA())
		{
			tloadtile(HULKBOAT);
			maxc = 0;
		}
		break;
	case MINIONBOAT:
		if (isRRRA())
		{
			tloadtile(MINIONBOAT);
			maxc = 0;
		}
		break;
	case BILLYPLAY:
		if (isRRRA())
		{
			for (j = BILLYPLAY; j <= BILLYPLAY + 2; j++)
				tloadtile(j, pal);
			maxc = 0;
		}
		break;
	case COOTPLAY:
		if (isRRRA())
		{
			for (j = COOTPLAY; j <= COOTPLAY + 4; j++)
				tloadtile(j, pal);
			maxc = 0;
		}
		break;
	case PIG:
	case PIGSTAYPUT:
		maxc = 68;
		break;
	case TORNADO:
		maxc = 7;
		break;
	case HEN:
	case HENSTAND:
		maxc = 34;
		break;
	case APLAYER:
		maxc = 0;
		if (ud.multimode > 1)
		{
			maxc = 5;
			for (j = APLAYER; j < APLAYER + 220; j++)
				tloadtile(j, pal);
			for (j = DUKEGUN; j < DUKELEG + 4; j++)
				tloadtile(j, pal);
		}
		break;
	case ATOMICHEALTH:
		maxc = 14;
		break;
	case DRONE:
		maxc = 6;
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
	case VIXEN:
		maxc = 214;
		for (j = spr->picnum; j < spr->picnum + maxc; j++)
			tloadtile(j, pal);
		maxc = 0;
		break;
	case SBMOVE:
		if (!isRRRA())
		{

			maxc = 54;
			for (j = spr->picnum; j < spr->picnum + maxc; j++)
				tloadtile(j, pal);
			maxc = 100;
			for (j = SBMOVE; j < SBMOVE + maxc; j++)
				tloadtile(j, pal);
			maxc = 0;
		}
		break;
	case HULK:
		maxc = 40;
		for (j = spr->picnum - 41; j < spr->picnum + maxc - 41; j++)
			tloadtile(j, pal);
		for (j = HULKJIBA; j <= HULKJIBC + 4; j++)
			tloadtile(j, pal);
		maxc = 0;
		break;
	case MINION:
		maxc = 141;
		for (j = spr->picnum; j < spr->picnum + maxc; j++)
			tloadtile(j, pal);
		for (j = MINJIBA; j <= MINJIBC + 4; j++)
			tloadtile(j, pal);
		maxc = 0;
		break;


	}

	for (j = spr->picnum; j < (spr->picnum + maxc); j++)
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
	}

	//tloadtile(VIEWSCREEN);

	for (i = FOOTPRINTS; i < FOOTPRINTS + 3; i++)
		tloadtile(i);

	for (i = BURNING; i < BURNING + 14; i++)
		tloadtile(i);

	for (i = FIRSTGUN; i < FIRSTGUN + 10; i++)
		tloadtile(i);

	for (i = EXPLOSION2; i < EXPLOSION2 + 21; i++)
		tloadtile(i);

	tloadtile(BULLETHOLE);

	for (i = SHOTGUN; i < SHOTGUN + 8; i++)
		tloadtile(i);

	tloadtile(FOOTPRINTS);

	for (i = JIBS1; i < (JIBS5 + 5); i++)
		tloadtile(i);

	for (i = SCRAP1; i < (SCRAP1 + 19); i++)
		tloadtile(i);

	for (i = SMALLSMOKE; i < (SMALLSMOKE + 4); i++)
		tloadtile(i);

}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void cacheit_r(void)
{
	if (!r_precache) return;

	cachegoodsprites();

	for (auto& wal : walls())
	{
			tloadtile(wal.picnum, wal.pal);
		if(wal.overpicnum >= 0)
			tloadtile(wal.overpicnum, wal.pal);
	}

	for (auto& sect : sectors())
	{
		tloadtile(sect.floorpicnum, sect.floorpal);
		tloadtile(sect.ceilingpicnum, sect.ceilingpal);
		if (sect.ceilingpicnum == LA)
		{
			tloadtile(LA + 1);
			tloadtile(LA + 2);
		}

		DukeSectIterator it(&sect);
		while (auto j = it.Next())
		{
			if(j->s->xrepeat != 0 && j->s->yrepeat != 0 && (j->s->cstat&32768) == 0)
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

void spriteinit_r(int i)
{
	i = initspriteforspawn(i, { CRACK1, CRACK2, CRACK3, CRACK4 });
	if ((i & 0x1000000)) spawninit_r(nullptr, &hittype[i & 0xffffff]);
}

void prelevel_r(int g)
{
	struct player_struct* p;
	int i;
	int j;
	int lotaglist;
	short lotags[65];
	int speed = 0;
	int dist;
	int sound;
	sound = 0;

	prelevel_common(g);
	p = &ps[screenpeek];

	if (currentLevel->gameflags & LEVEL_RR_CLEARMOONSHINE)
		ps[myconnectindex].steroids_amount = 0;

	if (isRRRA())
	{

		for (j = 0; j < MAXSPRITES; j++)
		{
			auto spr = &sprite[j];
			if (spr->pal == 100)
			{
				if (numplayers > 1)
					deletesprite(j);
				else
					spr->pal = 0;
			}
			else if (spr->pal == 101)
			{
				spr->extra = 0;
				spr->hitag = 1;
				spr->pal = 0;
				changespritestat(j, 118);
			}
		}
	}

	for (auto&sect : sectors())
	{
		auto sectp = &sect;
		if (sectp->ceilingpicnum == RRTILE2577)
			thunderon = 1;

		switch (sectp->lotag)
		{
		case 41:
		{
			DukeSectIterator it(sectp);
			dist = 0;
			while (auto act = it.Next())
			{
				auto spr = act->s;
				if (spr->picnum == RRTILE11)
				{
					dist = spr->lotag << 4;
					speed = spr->hitag;
					deletesprite(act);
				}
				if (spr->picnum == RRTILE38)
				{
					sound = spr->lotag;
					deletesprite(act);
				}
			}
			for(auto& osect : sectors())
			{
				if (sectp->hitag == osect.hitag && &osect != sectp)
				{
					// & 32767 to avoid some ordering issues here. 
					// Other code assumes that the lotag is always a sector effector type and can mask the high bit in.
					addjaildoor(dist, speed, sectp->hitag, osect.lotag & 32767, sound, &osect);
				}
			}
			break;
		}
		case 42:
		{
			int ii;
			sectortype* childsectnum = nullptr;
			dist = 0;
			speed = 0;
			DukeSectIterator it(sectp);
			while (auto act = it.Next())
			{
				auto sj = act->s;
				if (sj->picnum == RRTILE64)
				{
					dist = sj->lotag << 4;
					speed = sj->hitag;
					for (ii = 0; ii < MAXSPRITES; ii++)
					{
						auto spr = &sprite[ii];
						if (spr->picnum == RRTILE66)
							if (spr->lotag == sj->sectnum)
							{
								childsectnum = spr->sector();
								deletesprite(ii);
							}
					}
					deletesprite(act);
				}
				if (sj->picnum == RRTILE65)
				{
					sound = sj->lotag;
					deletesprite(act);
				}
			}
			addminecart(dist, speed, sectp, sectp->hitag, sound, childsectnum);
			break;
		}
		}
	}

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
		case NUKEBUTTON:
			chickenplant = 1;
			break;

		case GPSPEED:
			si->sector()->extra = si->lotag;
			deletesprite(ac);
			break;

		case CYCLER:
			if (numcyclers >= MAXCYCLERS)
				I_Error("Too many cycling sectors.");
			cyclers[numcyclers].sector = si->sector();
			cyclers[numcyclers].lotag = si->lotag;
			cyclers[numcyclers].shade1 = si->shade;
			cyclers[numcyclers].shade2 = si->sector()->floorshade;
			cyclers[numcyclers].hitag = si->hitag;
			cyclers[numcyclers].state = (si->ang == 1536);
			numcyclers++;
			deletesprite(ac);
			break;

		case RRTILE18:
			addtorch(si);
			deletesprite(ac);
			break;

		case RRTILE35:
			addlightning(si);
			deletesprite(ac);
			break;

		case RRTILE68:
			shadedsector[si->sectnum] = 1;
			deletesprite(ac);
			break;

		case RRTILE67:
			si->cstat |= 32768;
			break;

		case SOUNDFX:
			if (ambientfx >= 64)
				I_Error("Too many ambient effects");
			else
			{
				ambienthitag[ambientfx] = si->hitag;
				ambientlotag[ambientfx] = si->lotag;
				si->ang = ambientfx;
				ambientfx++;
				si->lotag = 0;
				si->hitag = 0;
			}
			break;
		}
	}

	for (i = 0; i < MAXSPRITES; i++)
	{
		auto spr = &sprite[i];
		if (spr->picnum == RRTILE19)
		{
			if (geocnt > 64)
				I_Error("Too many geometry effects");
			if (spr->hitag == 0)
			{
				geosector[geocnt] = spr->sectnum;
				for (j = 0; j < MAXSPRITES; j++)
				{
					auto spj = &sprite[j];
					if (spr->lotag == spj->lotag && j != i && spj->picnum == RRTILE19)
					{
						if (spj->hitag == 1)
						{
							geosectorwarp[geocnt] = spj->sectnum;
							geox[geocnt] = spr->x - spj->x;
							geoy[geocnt] = spr->y - spj->y;
							//geoz[geocnt] = spr->z - spj->z;
						}
						if (spj->hitag == 2)
						{
							geosectorwarp2[geocnt] = spj->sectnum;
							geox2[geocnt] = spr->x - spj->x;
							geoy2[geocnt] = spr->y - spj->y;
							//geoz2[geocnt] = spr->z - spj->z;
						}
					}
				}
				geocnt++;
			}
		}
	}

	for (i = 0; i < MAXSPRITES; i++)
	{
		auto spr = &sprite[i];
		if (spr->statnum < MAXSTATUS)
		{
			if (spr->picnum == SECTOREFFECTOR && spr->lotag == SE_14_SUBWAY_CAR)
				continue;
			spriteinit_r(i);
		}
	}

	for (i = 0; i < MAXSPRITES; i++)
	{
		auto spr = &sprite[i];
		if (spr->statnum < MAXSTATUS)
		{
			if (spr->picnum == SECTOREFFECTOR && spr->lotag == SE_14_SUBWAY_CAR)
				spriteinit_r(i);
		}
		if (spr->picnum == RRTILE19)
			deletesprite(i);
		if (spr->picnum == RRTILE34)
		{
			sectorextra[spr->sectnum] = uint8_t(spr->lotag);
			deletesprite(i);
		}
	}

	lotaglist = 0;

	it.Reset(STAT_DEFAULT);
	while (auto ac = it.Next())
	{
		auto spr = ac->s;
		switch (spr->picnum)
		{
		case RRTILE8464 + 1:
			if (!isRRRA()) break;
			[[fallthrough]];
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
		case NUKEBUTTON:
		case NUKEBUTTON + 1:

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
				while (auto j = it1.Next())
				{
					if (j->s->lotag == 12 && j->s->hitag == spr->lotag)
						j->temp_data[0] = 1;
				}
			}
			break;
		}
	}

	mirrorcnt = 0;

	for (auto& wl : walls())
	{
		walltype* wal = &wl;

		
		if (wal->overpicnum == MIRROR && (wal->cstat & 32) != 0)
		{
			auto sectp = wal->nextSector();

			if (mirrorcnt > 63)
				I_Error("Too many mirrors (64 max.)");
			if (sectp && sectp->ceilingpicnum != MIRROR)
			{
				sectp->ceilingpicnum = MIRROR;
				sectp->floorpicnum = MIRROR;
				mirrorwall[mirrorcnt] = wal;
				mirrorsector[mirrorcnt] = sectp;
				mirrorcnt++;
				continue;
			}
		}

		if (numanimwalls >= MAXANIMWALLS)
			I_Error("Too many 'anim' walls (max 512.)");

		animwall[numanimwalls].tag = 0;
		animwall[numanimwalls].wall = nullptr;

		switch (wal->overpicnum)
		{
		case FANSPRITE:
			wal->cstat |= 65;
			animwall[numanimwalls].wall = wal;
			numanimwalls++;
			break;
		case BIGFORCE:
			animwall[numanimwalls].wall = wal;
			numanimwalls++;
			continue;
		}

		wal->extra = -1;

		switch (wal->picnum)
		{
		case SCREENBREAK6:
		case SCREENBREAK7:
		case SCREENBREAK8:
			for (j = SCREENBREAK6; j <= SCREENBREAK8; j++)
				tloadtile(j);
			animwall[numanimwalls].wall = wal;
			animwall[numanimwalls].tag = -1;
			numanimwalls++;
			break;
		}
	}

	//Invalidate textures in sector behind mirror
	for (i = 0; i < mirrorcnt; i++)
	{
		for (auto& mwal : wallsofsector(mirrorsector[i]))
		{
			mwal.picnum = MIRROR;
			mwal.overpicnum = MIRROR;
		}
	}
	thunder_brightness = 0;
	if (!thunderon)
	{
		g_visibility = p->visibility;
	}
 }


END_DUKE_NS
