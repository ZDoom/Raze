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
#include "dukeactor.h"
#include "buildtiles.h"

BEGIN_DUKE_NS

void ballreturn(DDukeActor *ball)
{
	DukeStatIterator it(STAT_BOWLING);
	while (auto act = it.Next())
	{
		if (act->s->picnum == RRTILE281 && ball->s->sectnum == act->s->sectnum)
		{
			DukeStatIterator it2(STAT_BOWLING);
			while (auto act2 = it2.Next())
			{
				if (act2->s->picnum == BOWLINGBALLSPOT && act->s->hitag == act2->s->hitag)
					spawn(act2, BOWLINGBALLSPRITE);
				if (act2->s->picnum == BOWLINGPINSPOT && act->s->hitag == act2->s->hitag && act2->s->lotag == 0)
				{
					act2->s->lotag = 100;
					act2->s->extra++;
					pinsectorresetdown(act2->s->sectnum);
				}
			}
		}
	}
}

short pinsectorresetdown(short sect)
{
	int j = getanimationgoal(anim_ceilingz, sect);

	if (j == -1)
	{
		j = sector[sect].floorz;
		setanimation(sect, anim_ceilingz, sect, j, 64);
		return 1;
	}
	return 0;
}

short pinsectorresetup(short sect)
{
	int j = getanimationgoal(anim_ceilingz, sect);

	if (j == -1)
	{
		j = sector[nextsectorneighborz(sect, sector[sect].ceilingz, -1, -1)].ceilingz;
		setanimation(sect, anim_ceilingz, sect, j, 64);
		return 1;
	}
	return 0;
}

short checkpins(short sect)
{
	short i, pin;
	int  x, y;
	short pins[10];
	short tag;

	pin = 0;
	for (i = 0; i < 10; i++) pins[i] = 0;

	DukeSectIterator it(sect);
	while (auto a2 = it.Next())
	{
		if (a2->s->picnum == BOWLINGPIN)
		{
			pin++;
			pins[a2->s->lotag] = 1;
		}
		if (a2->s->picnum == BOWLINGPINSPOT)
		{
			tag = a2->s->hitag;
		}
	}

	if (tag)
	{
		tag += LANEPICS + 1;
		TileFiles.tileMakeWritable(tag);
		tileCopySection(LANEPICS + 1, 0, 0, 128, 64, tag, 0, 0);
		for (i = 0; i < 10; i++)
		{
			if (pins[i] == 1)
			{
				switch (i)
				{
				case 0:
					x = 64;
					y = 48;
					break;
				case 1:
					x = 56;
					y = 40;
					break;
				case 2:
					x = 72;
					y = 40;
					break;
				case 3:
					x = 48;
					y = 32;
					break;
				case 4:
					x = 64;
					y = 32;
					break;
				case 5:
					x = 80;
					y = 32;
					break;
				case 6:
					x = 40;
					y = 24;
					break;
				case 7:
					x = 56;
					y = 24;
					break;
				case 8:
					x = 72;
					y = 24;
					break;
				case 9:
					x = 88;
					y = 24;
					break;
				}
				tileCopySection(LANEPICS, 0, 0, 8, 8, tag, x - 4, y - 10);
			}
		}
	}

	return pin;
}

void resetpins(short sect)
{
	int i, tag;
	int x, y;
	DukeSectIterator it(sect);
	while (auto a2 = it.Next())
	{
		if (a2->s->picnum == BOWLINGPIN)
			deletesprite(a2);
	}
	it.Reset(sect);
	while (auto a2 = it.Next())
	{
		if (a2->s->picnum == 283)
		{
			auto spawned = spawn(a2, BOWLINGPIN);
			spawned->s->lotag = a2->s->lotag;
			if (spawned->s->lotag == 3 || spawned->s->lotag == 5)
			{
				spawned->s->clipdist = (1 + (krand() % 1)) * 16 + 32;
			}
			else
			{
				spawned->s->clipdist = (1 + (krand() % 1)) * 16 + 32;
			}
			spawned->s->ang -= ((krand() & 32) - (krand() & 64)) & 2047;
		}
		if (a2->s->picnum == 280)
			tag = a2->s->hitag;
	}
	if (tag)
	{
		tag += LANEPICS + 1;
		TileFiles.tileMakeWritable(tag);
		tileCopySection(LANEPICS + 1, 0, 0, 128, 64, tag, 0, 0);
		for (i = 0; i < 10; i++)
		{
			switch (i)
			{
			case 0:
				x = 64;
				y = 48;
				break;
			case 1:
				x = 56;
				y = 40;
				break;
			case 2:
				x = 72;
				y = 40;
				break;
			case 3:
				x = 48;
				y = 32;
				break;
			case 4:
				x = 64;
				y = 32;
				break;
			case 5:
				x = 80;
				y = 32;
				break;
			case 6:
				x = 40;
				y = 24;
				break;
			case 7:
				x = 56;
				y = 24;
				break;
			case 8:
				x = 72;
				y = 24;
				break;
			case 9:
				x = 88;
				y = 24;
				break;
			}
			tileCopySection(LANEPICS, 0, 0, 8, 8, tag, x - 4, y - 10);
		}
	}
}

void resetlanepics(void)
{
	if (!isRR()) return;
	int x, y;
	short i;
	short tag, pic;
	for (tag = 0; tag < 4; tag++)
	{
		pic = tag + 1;
		if (pic == 0) continue;
		pic += LANEPICS + 1;
		TileFiles.tileMakeWritable(pic);
		tileCopySection(LANEPICS + 1, 0, 0, 128, 64, pic, 0, 0);
		for (i = 0; i < 10; i++)
		{
			switch (i)
			{
			case 0:
				x = 64;
				y = 48;
				break;
			case 1:
				x = 56;
				y = 40;
				break;
			case 2:
				x = 72;
				y = 40;
				break;
			case 3:
				x = 48;
				y = 32;
				break;
			case 4:
				x = 64;
				y = 32;
				break;
			case 5:
				x = 80;
				y = 32;
				break;
			case 6:
				x = 40;
				y = 24;
				break;
			case 7:
				x = 56;
				y = 24;
				break;
			case 8:
				x = 72;
				y = 24;
				break;
			case 9:
				x = 88;
				y = 24;
				break;
			}
			tileCopySection(LANEPICS, 0, 0, 8, 8, pic, x - 4, y - 10);
		}
	}
}

END_DUKE_NS

