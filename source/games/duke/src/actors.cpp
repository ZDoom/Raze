//-------------------------------------------------------------------------
/*
Copyright (C) 1996, 2003 - 3D Realms Entertainment
Copyright (C) 2000, 2003 - Matt Saettler (EDuke Enhancements)
Copyright (C) 2017-2019 - Nuke.YKT
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

This file is a combination of code from the following sources:
- EDuke 2 by Matt Saettler
- JFDuke by Jonathon Fowler (jf@jonof.id.au),
- DukeGDX and RedneckGDX by Alexander Makarov-[M210] (m210-2007@mail.ru)
- Redneck Rampage reconstructed source by Nuke.YKT

*/
//-------------------------------------------------------------------------

#include "ns.h"
#include "global.h"
#include "names.h"

BEGIN_DUKE_NS

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

bool ceilingspace(int sectnum)
{
	if ((sector[sectnum].ceilingstat & 1) && sector[sectnum].ceilingpal == 0)
	{
		switch (sector[sectnum].ceilingpicnum)
		{
		case MOONSKY1:
		case BIGORBIT1:
			return !isRR();

		case RR_MOONSKY1:
		case RR_BIGORBIT1:
			return isRR();
		}
	}
	return 0;
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

bool floorspace(int sectnum)
{
	if ((sector[sectnum].floorstat & 1) && sector[sectnum].ceilingpal == 0)
	{
		switch (sector[sectnum].floorpicnum)
		{
		case MOONSKY1:
		case BIGORBIT1:
			return !isRR();

		case RR_MOONSKY1:
		case RR_BIGORBIT1:
			return !!isRR();
		}
	}
	return 0;
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void addammo(short weapon, struct player_struct* p, short amount)
{
	p->ammo_amount[weapon] += amount;

	if (p->ammo_amount[weapon] > max_ammo_amount[weapon])
		p->ammo_amount[weapon] = max_ammo_amount[weapon];
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void addweapon(struct player_struct* p, int weapon)
{
	short cw = p->curr_weapon;
	if (p->OnMotorcycle || p->OnBoat)
	{
		p->gotweapon.Set(weapon);
		if (weapon == SHRINKER_WEAPON)
		{
			p->gotweapon.Set(GROW_WEAPON);
			p->ammo_amount[GROW_WEAPON] = 1;
		}
		else if (weapon == RPG_WEAPON)
		{
			p->gotweapon.Set(RA16_WEAPON);
		}
		else if (weapon == RA15_WEAPON)
		{
			p->ammo_amount[RA15_WEAPON] = 1;
		}
		return;
	}

	if (p->gotweapon[weapon] == 0)
	{
		p->gotweapon.Set(weapon);
		if (weapon == SHRINKER_WEAPON)
		{
			p->gotweapon.Set(GROW_WEAPON);
			if (isRRRA()) p->ammo_amount[GROW_WEAPON] = 1;
		}
		if (isRRRA())
		{
			if (weapon == RPG_WEAPON)
			{
				p->gotweapon.Set(RA16_WEAPON);
			}
			if (weapon == RA15_WEAPON)
			{
				p->ammo_amount[RA15_WEAPON] = 50;
			}
		}

		if (!isRR() || weapon != HANDBOMB_WEAPON)
			cw = weapon;
	}
	else
		cw = weapon;

	if (isRR() && weapon == HANDBOMB_WEAPON)
		p->last_weapon = -1;

	p->random_club_frame = 0;

	if (p->holster_weapon == 0)
	{
		p->weapon_pos = -1;
		p->last_weapon = p->curr_weapon;
	}
	else
	{
		p->weapon_pos = 10;
		p->holster_weapon = 0;
		p->last_weapon = -1;
	}

	p->kickback_pic = 0;
#ifdef EDUKE
	if (p->curr_weapon != cw)
	{
		short snum;
		snum = sprite[p->i].yvel;

		SetGameVarID(g_iWeaponVarID, cw, p->i, snum);
		if (p->curr_weapon >= 0)
		{
			SetGameVarID(g_iWorksLikeVarID, aplWeaponWorksLike[weapon][snum], p->i, snum);
		}
		else
		{
			SetGameVarID(g_iWorksLikeVarID, -1, p->i, snum);
		}
		SetGameVarID(g_iReturnVarID, 0, -1, snum);
		OnEvent(EVENT_CHANGEWEAPON, p->i, snum, -1);
		if (GetGameVarID(g_iReturnVarID, -1, snum) == 0)
		{
			p->curr_weapon = cw;
		}
	}
#else
	p->curr_weapon = cw;
#endif

	switch (weapon)
	{
	case RA15_WEAPON:
	case KNEE_WEAPON:
	case TRIPBOMB_WEAPON:
	case HANDREMOTE_WEAPON:
	case HANDBOMB_WEAPON:     
		break;
	case SHOTGUN_WEAPON:      
		spritesound(SHOTGUN_COCK, p->i); 
		break;
	case PISTOL_WEAPON:       
		spritesound(INSERT_CLIP, p->i); 
		break;
	default:      
		spritesound(SELECT_WEAPON, p->i); 
		break;
	}
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void checkavailinven(struct player_struct* p)
{

	if (p->firstaid_amount > 0)
		p->inven_icon = ICON_FIRSTAID;
	else if (p->steroids_amount > 0)
		p->inven_icon = ICON_STEROIDS;
	else if (p->holoduke_amount > 0)
		p->inven_icon = ICON_HOLODUKE;
	else if (p->jetpack_amount > 0)
		p->inven_icon = ICON_JETPACK;
	else if (p->heat_amount > 0)
		p->inven_icon = ICON_HEATS;
	else if (p->scuba_amount > 0)
		p->inven_icon = ICON_SCUBA;
	else if (p->boot_amount > 0)
		p->inven_icon = ICON_BOOTS;
	else p->inven_icon = ICON_NONE;
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void checkavailweapon(struct player_struct* p)
{
	short i, snum;
	int weap;

	if (p->wantweaponfire >= 0)
	{
		weap = p->wantweaponfire;
		p->wantweaponfire = -1;

		if (weap == p->curr_weapon) return;
		else if (p->gotweapon[weap] && p->ammo_amount[weap] > 0)
		{
			addweapon(p, weap);
			return;
		}
	}

	weap = p->curr_weapon;
	if (p->gotweapon[weap] && p->ammo_amount[weap] > 0)
		return;

	snum = sprite[p->i].yvel;

	// Note: RedNukem has this restriction, but the original source and RedneckGDX do not.
#if 1 // TRANSITIONAL
	int max = ((isRR()) ? DEVISTATOR_WEAPON : FREEZE_WEAPON);
#else
	int max = FREEZE_WEAPON;
#endif
	for (i = 0; i < 10; i++)
	{
		weap = ud.wchoice[snum][i];
		if ((g_gameType & GAMEFLAG_SHAREWARE) && weap > 6) continue;

		if (weap == 0) weap = max;
		else weap--;

		if (weap == KNEE_WEAPON || (p->gotweapon[weap] && p->ammo_amount[weap] > 0))
			break;
	}

	if (i == HANDREMOTE_WEAPON) weap = KNEE_WEAPON;

	// Found the weapon

	p->last_weapon = p->curr_weapon;
	p->random_club_frame = 0;
	p->curr_weapon = weap;
	if (isWW2GI())
	{
		SetGameVarID(g_iWeaponVarID, p->curr_weapon, p->i, snum);
		if (p->curr_weapon >= 0)
		{
			SetGameVarID(g_iWorksLikeVarID, aplWeaponWorksLike[p->curr_weapon][snum], p->i, snum);
		}
		else
		{
			SetGameVarID(g_iWorksLikeVarID, -1, p->i, snum);
		}
		OnEvent(EVENT_CHANGEWEAPON, p->i, snum, -1);
	}

	p->kickback_pic = 0;
	if (p->holster_weapon == 1)
	{
		p->holster_weapon = 0;
		p->weapon_pos = 10;
	}
	else p->weapon_pos = -1;
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

bool ifsquished(int i, int p)
{
	if (isRR()) return false;	// this function is a no-op in RR's source.

	bool squishme = false;
	if (sprite[i].picnum == TILE_APLAYER && ud.clipping)
		return false;

	auto& sc = sector[sprite[i].sectnum];
	int floorceildist = sc.floorz - sc.ceilingz;

	if (sc.lotag != ST_23_SWINGING_DOOR)
	{
		if (sprite[i].pal == 1)
			squishme = floorceildist < (32 << 8) && (sc.lotag & 32768) == 0;
		else
			squishme = floorceildist < (12 << 8);
	}

	if (squishme)
	{
		FTA(QUOTE_SQUISHED, &ps[p]);

		if (badguy(&sprite[i]))
			sprite[i].xvel = 0;

		if (sprite[i].pal == 1)
		{
			hittype[i].picnum = SHOTSPARK1;
			hittype[i].extra = 1;
			return false;
		}

		return true;
	}
	return false;
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void clearcamera(player_struct* ps)
{
	ps->newowner = -1;
	ps->posx = ps->oposx;
	ps->posy = ps->oposy;
	ps->posz = ps->oposz;
	ps->q16ang = ps->oq16ang;
	updatesector(ps->posx, ps->posy, &ps->cursectnum);
	setpal(ps);

	int k = headspritestat[1];
	while (k >= 0)
	{
		if (sprite[k].picnum == CAMERA1)
			sprite[k].yvel = 0;
		k = nextspritestat[k];
	}
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void hitradius(short i, int  r, int  hp1, int  hp2, int  hp3, int  hp4)
{
	spritetype* s, * sj;
	walltype* wal;
	int d, q, x1, y1;
	int sectcnt, sectend, dasect, startwall, endwall, nextsect;
	short j, p, x, nextj, sect;
	static const uint8_t statlist[] = { STAT_DEFAULT, STAT_ACTOR, STAT_STANDABLE, STAT_PLAYER, STAT_FALLER, STAT_ZOMBIEACTOR, STAT_MISC };
	short tempshort[MAXSECTORS];	// originally hijacked a global buffer which is bad. Q: How many do we really need? RedNukem says 64.

	s = &sprite[i];

	if (s->xrepeat < 11)
	{
		if (!isRR())
		{
			if (s->picnum == RPG) goto SKIPWALLCHECK;
		}
		else
		{
			if (s->picnum == RR_CROSSBOW || ((isRRRA()) && s->picnum == RR_CHIKENCROSSBOW)) goto SKIPWALLCHECK;
		}
	}

	if ((isRR()) || s->picnum != SHRINKSPARK)
	{
		tempshort[0] = s->sectnum;
		dasect = s->sectnum;
		sectcnt = 0; sectend = 1;

		do
		{
			dasect = tempshort[sectcnt++];
			if (((sector[dasect].ceilingz - s->z) >> 8) < r)
			{
				d = abs(wall[sector[dasect].wallptr].x - s->x) + abs(wall[sector[dasect].wallptr].y - s->y);
				if (d < r)
					checkhitceiling(dasect);
				else
				{
					// ouch...
					d = abs(wall[wall[wall[sector[dasect].wallptr].point2].point2].x - s->x) + abs(wall[wall[wall[sector[dasect].wallptr].point2].point2].y - s->y);
					if (d < r)
						checkhitceiling(dasect);
				}
			}

			startwall = sector[dasect].wallptr;
			endwall = startwall + sector[dasect].wallnum;
			for (x = startwall, wal = &wall[startwall]; x < endwall; x++, wal++)
				if ((abs(wal->x - s->x) + abs(wal->y - s->y)) < r)
				{
					nextsect = wal->nextsector;
					if (nextsect >= 0)
					{
						for (dasect = sectend - 1; dasect >= 0; dasect--)
							if (tempshort[dasect] == nextsect) break;
						if (dasect < 0) tempshort[sectend++] = nextsect;
					}
					x1 = (((wal->x + wall[wal->point2].x) >> 1) + s->x) >> 1;
					y1 = (((wal->y + wall[wal->point2].y) >> 1) + s->y) >> 1;
					updatesector(x1, y1, &sect);
					if (sect >= 0 && cansee(x1, y1, s->z, sect, s->x, s->y, s->z, s->sectnum))
						checkhitwall(i, x, wal->x, wal->y, s->z, s->picnum);
				}
		} while (sectcnt < sectend);
	}

SKIPWALLCHECK:

	int val = (isRR()) ? 24 : 16;
	q = -(val << 8) + (krand() & ((32 << 8) - 1));

	for (x = 0; x < 7; x++)
	{
		j = headspritestat[statlist[x]];
		while (j >= 0)
		{
			nextj = nextspritestat[j];
			sj = &sprite[j];

			if (isWorldTour())
			{
				if (sprite[s->owner].picnum == APLAYER && sj->picnum == APLAYER && ud.coop != 0 && ud.ffire == 0 && s->owner != j)
					continue;

				if (s->picnum == FLAMETHROWERFLAME && ((sprite[s->owner].picnum == FIREFLY && sj->picnum == FIREFLY) || (sprite[s->owner].picnum == BOSS5 && sj->picnum == BOSS5)))
					continue;
			}

			if (x == 0 || x >= 5 || AFLAMABLE(sj->picnum))
			{
				if ((!isRR() && s->picnum != SHRINKSPARK) || (sj->cstat & 257))
					if (dist(s, sj) < r)
					{
						if (badguy(sj) && !cansee(sj->x, sj->y, sj->z + q, sj->sectnum, s->x, s->y, s->z + q, s->sectnum))
							goto BOLT;
						checkhitsprite(j, i);
					}
			}
			else if (!isRR())
			{
				if (sj->extra >= 0 && sj != s && (sj->picnum == TRIPBOMB || badguy(sj) || sj->picnum == QUEBALL || sj->picnum == STRIPEBALL || (sj->cstat & 257) || sj->picnum == DUKELYINGDEAD))
				{
					if (s->picnum == SHRINKSPARK && sj->picnum != SHARK && (j == s->owner || sj->xrepeat < 24))
					{
						j = nextj;
						continue;
					}
					if (s->picnum == MORTER && j == s->owner)
					{
						j = nextj;
						continue;
					}

					if (sj->picnum == APLAYER) sj->z -= PHEIGHT;
					d = dist(s, sj);
					if (sj->picnum == APLAYER) sj->z += PHEIGHT;

					if (d < r && cansee(sj->x, sj->y, sj->z - (8 << 8), sj->sectnum, s->x, s->y, s->z - (12 << 8), s->sectnum))
					{
						hittype[j].ang = getangle(sj->x - s->x, sj->y - s->y);

						if (s->picnum == RPG && sj->extra > 0)
							hittype[j].picnum = RPG;
						else if (!isWorldTour())
						{
							if (s->picnum == SHRINKSPARK)
								hittype[j].picnum = SHRINKSPARK;
							else hittype[j].picnum = RADIUSEXPLOSION;
						}
						else
						{
							if (s->picnum == SHRINKSPARK || s->picnum == FLAMETHROWERFLAME)
								hittype[j].picnum = s->picnum;
							else if (s->picnum != FIREBALL || sprite[s->owner].picnum != APLAYER)
							{
								if (s->picnum == LAVAPOOL)
									hittype[j].picnum = FLAMETHROWERFLAME;
								else
									hittype[j].picnum = RADIUSEXPLOSION;
							}
							else
								hittype[j].picnum = FLAMETHROWERFLAME;
						}

						if (s->picnum != SHRINKSPARK && (!isWorldTour() && s->picnum != LAVAPOOL))
						{
							if (d < r / 3)
							{
								if (hp4 == hp3) hp4++;
								hittype[j].extra = hp3 + (krand() % (hp4 - hp3));
							}
							else if (d < 2 * r / 3)
							{
								if (hp3 == hp2) hp3++;
								hittype[j].extra = hp2 + (krand() % (hp3 - hp2));
							}
							else if (d < r)
							{
								if (hp2 == hp1) hp2++;
								hittype[j].extra = hp1 + (krand() % (hp2 - hp1));
							}

							if (sprite[j].picnum != TANK && sprite[j].picnum != ROTATEGUN && sprite[j].picnum != RECON && sprite[j].picnum != BOSS1 && sprite[j].picnum != BOSS2 && sprite[j].picnum != BOSS3 && sprite[j].picnum != BOSS4)
							{
								if (sj->xvel < 0) sj->xvel = 0;
								sj->xvel += (s->extra << 2);
							}

							if (sj->picnum == PODFEM1 || sj->picnum == FEM1 ||
								sj->picnum == FEM2 || sj->picnum == FEM3 ||
								sj->picnum == FEM4 || sj->picnum == FEM5 ||
								sj->picnum == FEM6 || sj->picnum == FEM7 ||
								sj->picnum == FEM8 || sj->picnum == FEM9 ||
								sj->picnum == FEM10 || sj->picnum == STATUE ||
								sj->picnum == STATUEFLASH || sj->picnum == SPACEMARINE || sj->picnum == QUEBALL || sj->picnum == STRIPEBALL)
								checkhitsprite(j, i);
						}
						else if (s->extra == 0) hittype[j].extra = 0;

						if (sj->picnum != RADIUSEXPLOSION &&
							s->owner >= 0 && sprite[s->owner].statnum < MAXSTATUS)
						{
							if (sj->picnum == APLAYER)
							{
								p = sj->yvel;

								if (isWorldTour() && hittype[j].picnum == FLAMETHROWERFLAME && sprite[s->owner].picnum == APLAYER) 
								{
									ps[p].numloogs = -1 - s->yvel;
								}

								if (ps[p].newowner >= 0)
								{
									clearcamera(&ps[p]);
								}
							}
							hittype[j].owner = s->owner;
						}
					}
				}
			}
			else
			{
				if (sj->extra >= 0 && sj != s && (badguy(sj) || sj->picnum == RR_QUEBALL || sj->picnum == RR_3440 || sj->picnum == RR_STRIPEBALL || (sj->cstat & 257) || sj->picnum == RR_LNRDLYINGDEAD))
				{
					if (s->picnum == RR_MORTER && j == s->owner)
					{
						j = nextj;
						continue;
					}
					if ((isRRRA()) && s->picnum == RR_CHEERBOMB && j == s->owner)
					{
						j = nextj;
						continue;
					}

					if (sj->picnum == APLAYER) sj->z -= PHEIGHT;
					d = dist(s, sj);
					if (sj->picnum == APLAYER) sj->z += PHEIGHT;

					if (d < r && cansee(sj->x, sj->y, sj->z - (8 << 8), sj->sectnum, s->x, s->y, s->z - (12 << 8), s->sectnum))
					{
						if ((isRRRA()) && sprite[j].picnum == RR_MINION && sprite[j].pal == 19)
						{
							j = nextj;
							continue;
						}

						hittype[j].ang = getangle(sj->x - s->x, sj->y - s->y);

						if (s->picnum == RR_CROSSBOW && sj->extra > 0)
							hittype[j].picnum = RR_CROSSBOW;
						else if ((isRRRA()) && s->picnum == RR_CHIKENCROSSBOW && sj->extra > 0)
							hittype[j].picnum = RR_CROSSBOW;
						else
							hittype[j].picnum = RR_RADIUSEXPLOSION;

						if (d < r / 3)
						{
							if (hp4 == hp3) hp4++;
							hittype[j].extra = hp3 + (krand() % (hp4 - hp3));
						}
						else if (d < 2 * r / 3)
						{
							if (hp3 == hp2) hp3++;
							hittype[j].extra = hp2 + (krand() % (hp3 - hp2));
						}
						else if (d < r)
						{
							if (hp2 == hp1) hp2++;
							hittype[j].extra = hp1 + (krand() % (hp2 - hp1));
						}

						int pic = sprite[j].picnum;
						if ((isRRRA())? 
							(pic != RR_HULK && pic != RR_MAMAJACKOLOPE && pic != RR_GUITARBILLY && pic != RR_BANJOCOOTER && pic != RR_MAMACLOUD) :
							(pic != RR_HULK && pic != RR_SBMOVE))
						{
							if (sprite[j].xvel < 0) sprite[j].xvel = 0;
							sprite[j].xvel += (sprite[j].extra << 2);
						}

						if (sj->picnum == RR_STATUEFLASH || sj->picnum == RR_QUEBALL ||
							sj->picnum == RR_STRIPEBALL || sj->picnum == RR_3440)
							checkhitsprite(j, i);

						if (sprite[j].picnum != RR_RADIUSEXPLOSION &&
							s->owner >= 0 && sprite[s->owner].statnum < MAXSTATUS)
						{
							if (sprite[j].picnum == APLAYER)
							{
								p = sprite[j].yvel;
								if (ps[p].newowner >= 0)
								{
									clearcamera(&ps[p]);
								}
							}
							hittype[j].owner = s->owner;
						}
					}
				}
			}
		BOLT:
			j = nextj;
		}
	}
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

int movesprite(short spritenum, int xchange, int ychange, int zchange, unsigned int cliptype)
{
	int daz, h, oldx, oldy;
	short retval, dasectnum, cd;
	char bg;

	bg = badguy(&sprite[spritenum]);

	if (sprite[spritenum].statnum == 5 || (bg && sprite[spritenum].xrepeat < 4))
	{
		sprite[spritenum].x += (xchange * TICSPERFRAME) >> 2;
		sprite[spritenum].y += (ychange * TICSPERFRAME) >> 2;
		sprite[spritenum].z += (zchange * TICSPERFRAME) >> 2;
		if (bg)
			setsprite(spritenum, sprite[spritenum].x, sprite[spritenum].y, sprite[spritenum].z);
		return 0;
	}

	dasectnum = sprite[spritenum].sectnum;

	daz = sprite[spritenum].z;
	h = ((tilesiz[sprite[spritenum].picnum].y * sprite[spritenum].yrepeat) << 1);
	daz -= h;

	if (bg)
	{
		oldx = sprite[spritenum].x;
		oldy = sprite[spritenum].y;

		if (sprite[spritenum].xrepeat > 60)
			retval = clipmove(&sprite[spritenum].x, &sprite[spritenum].y, &daz, &dasectnum, ((xchange * TICSPERFRAME) << 11), ((ychange * TICSPERFRAME) << 11), 1024L, (4 << 8), (4 << 8), cliptype);
		else 
		{
			if (isRR())
				cd = 192;
			else if (sprite[spritenum].picnum == LIZMAN)
				cd = 292;
#if 0	// TRANSITIONAL the needed infrastructure for this is too different for now
			else if ((actortype[sprite[spritenum].picnum] & 3))
#else
			else if (A_CheckSpriteFlags(spritenum, SFLAG_BADGUY))
#endif
				cd = sprite[spritenum].clipdist << 2;
			else
				cd = 192;

			retval = clipmove(&sprite[spritenum].x, &sprite[spritenum].y, &daz, &dasectnum, ((xchange * TICSPERFRAME) << 11), ((ychange * TICSPERFRAME) << 11), cd, (4 << 8), (4 << 8), cliptype);
		}

		bool rr = (isRR());
		// conditional code from hell...
		if (dasectnum < 0 || (dasectnum >= 0 &&
			((hittype[spritenum].actorstayput >= 0 && hittype[spritenum].actorstayput != dasectnum) ||
				(!rr && 
					(
						((sprite[spritenum].picnum == BOSS2) && sprite[spritenum].pal == 0 && sector[dasectnum].lotag != 3) ||
						((sprite[spritenum].picnum == BOSS1 || sprite[spritenum].picnum == BOSS2) && sector[dasectnum].lotag == ST_1_ABOVE_WATER) ||
						(sector[dasectnum].lotag == ST_1_ABOVE_WATER && (sprite[spritenum].picnum == LIZMAN || (sprite[spritenum].picnum == LIZTROOP && sprite[spritenum].zvel == 0)))
					)
				)
			)))
		{
			sprite[spritenum].x = oldx;
			sprite[spritenum].y = oldy;
			if (sector[dasectnum].lotag == ST_1_ABOVE_WATER && (rr || sprite[spritenum].picnum == LIZMAN))
				sprite[spritenum].ang = (krand() & 2047);
			else if ((hittype[spritenum].temp_data[0] & 3) == 1 && (rr || sprite[spritenum].picnum != COMMANDER))
				sprite[spritenum].ang = (krand() & 2047);
			setsprite(spritenum, oldx, oldy, sprite[spritenum].z);
			if (dasectnum < 0) dasectnum = 0;
			return (16384 + dasectnum);
		}
		if ((retval & 49152) >= 32768 && (hittype[spritenum].cgg == 0)) sprite[spritenum].ang += 768;
	}
	else
	{
		if (sprite[spritenum].statnum == 4)
			retval =
			clipmove(&sprite[spritenum].x, &sprite[spritenum].y, &daz, &dasectnum, ((xchange * TICSPERFRAME) << 11), ((ychange * TICSPERFRAME) << 11), 8L, (4 << 8), (4 << 8), cliptype);
		else
			retval =
			clipmove(&sprite[spritenum].x, &sprite[spritenum].y, &daz, &dasectnum, ((xchange * TICSPERFRAME) << 11), ((ychange * TICSPERFRAME) << 11), (int)(sprite[spritenum].clipdist << 2), (4 << 8), (4 << 8), cliptype);
	}

	if (dasectnum >= 0)
		if ((dasectnum != sprite[spritenum].sectnum))
			changespritesect(spritenum, dasectnum);
	daz = sprite[spritenum].z + ((zchange * TICSPERFRAME) >> 3);
	if ((daz > hittype[spritenum].ceilingz) && (daz <= hittype[spritenum].floorz))
		sprite[spritenum].z = daz;
	else
		if (retval == 0)
			return(16384 + dasectnum);

	return(retval);
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

int ssp(short i, unsigned int cliptype) //The set sprite function
{
	spritetype* s;
	int movetype;

	s = &sprite[i];

	movetype = movesprite(i,
		(s->xvel * (sintable[(s->ang + 512) & 2047])) >> 14,
		(s->xvel * (sintable[s->ang & 2047])) >> 14, s->zvel,
		cliptype);

	return (movetype == 0);
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void insertspriteq(int i)
{
	if (spriteqamount > 0)
	{
		if (spriteq[spriteqloc] >= 0)
			sprite[spriteq[spriteqloc]].xrepeat = 0;
		spriteq[spriteqloc] = i;
		spriteqloc = (spriteqloc + 1) % spriteqamount;
	}
	else sprite[i].xrepeat = sprite[i].yrepeat = 0;
}

//---------------------------------------------------------------------------
//
// consolidation of several nearly identical functions
// lotsofmoney -> MONEY / RR_FEATHERS
// lotsofmail -> MAIL
// lotsofpaper -> PAPER
//
//---------------------------------------------------------------------------

void lotsofstuff(spritetype* s, short n, int spawntype)
{
	short i, j;
	for (i = n; i > 0; i--)
	{
		short r1 = krand(), r2 = krand();	// using the RANDCORRECT version from RR.
		// TRANSITIONAL RedNukem sets the spawner as owner.
		j = EGS(s->sectnum, s->x, s->y, s->z - (r2 % (47 << 8)), spawntype, -32, 8, 8, r1 & 2047, 0, 0, 0, 5);
		sprite[j].cstat = krand() & 12;
	}
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void guts(spritetype* s, short gtype, short n, short p)
{
	int gutz, floorz;
	int i=0, j;
	int sx, sy;
	uint8_t pal;

	if (badguy(s) && s->xrepeat < 16)
		sx = sy = 8;
	else sx = sy = 32;

	gutz = s->z - (8 << 8);
	floorz = getflorzofslope(s->sectnum, s->x, s->y);

	if (gutz > (floorz - (8 << 8)))
		gutz = floorz - (8 << 8);

	if (!isRR() && s->picnum == COMMANDER)
		gutz -= (24 << 8);

	if (badguy(s) && s->pal == 6)
		pal = 6;
	else
	{
		pal = 0;
		if (isRRRA())
		{
			if (s->picnum == RR_MINION && (s->pal == 8 || s->pal == 19)) pal = s->pal;
		}
	}

	if (isRR())
	{
		sx >>= 1;
		sy >>= 1;
	}

	for (j = 0; j < n; j++)
	{
		// RANDCORRECT version from RR.
		int a = krand() & 2047;
		int r1 = krand();
		int r2 = krand();
		int r3 = krand();
		int r4 = krand();
		int r5 = krand();
		// TRANSITIONAL: owned by a player???
		i = EGS(s->sectnum, s->x + (r5 & 255) - 128, s->y + (r4 & 255) - 128, gutz - (r3 & 8191), gtype, -32, sx, sy, a, 48 + (r2 & 31), -512 - (r1 & 2047), ps[p].i, 5); 
		if (!isRR() && sprite[i].picnum == JIBS2)
		{
			sprite[i].xrepeat >>= 2;
			sprite[i].yrepeat >>= 2;
		}
		if (pal != 0)
			sprite[i].pal = pal;
	}
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void gutsdir(spritetype* s, short gtype, short n, short p)
{
	int gutz, floorz;
	short i, j;
	char sx, sy;

	if (badguy(s) && s->xrepeat < 16)
		sx = sy = 8;
	else sx = sy = 32;

	gutz = s->z - (8 << 8);
	floorz = getflorzofslope(s->sectnum, s->x, s->y);

	if (gutz > (floorz - (8 << 8)))
		gutz = floorz - (8 << 8);

	if (!isRR() && s->picnum == COMMANDER)
		gutz -= (24 << 8);

	for (j = 0; j < n; j++)
	{
		int a = krand() & 2047;
		int r1 = krand();
		int r2 = krand();
		// TRANSITIONAL: owned by a player???
		i = EGS(s->sectnum, s->x, s->y, gutz, gtype, -32, sx, sy, a, 256 + (r2 & 127), -512 - (r1 & 2047), ps[p].i, 5);
	}
}

//---------------------------------------------------------------------------
//
// movesector - why is this in actors.cpp?
//
//---------------------------------------------------------------------------

void ms(short i)
{
	//T1,T2 and T3 are used for all the sector moving stuff!!!

	short startwall, endwall, x;
	int tx, ty;
	spritetype* s;

	s = &sprite[i];

	s->x += (s->xvel * (sintable[(s->ang + 512) & 2047])) >> 14;
	s->y += (s->xvel * (sintable[s->ang & 2047])) >> 14;

	int j = hittype[i].temp_data[1];
	int k = hittype[i].temp_data[2];

	startwall = sector[s->sectnum].wallptr;
	endwall = startwall + sector[s->sectnum].wallnum;
	for (x = startwall; x < endwall; x++)
	{
		rotatepoint(
			0, 0,
			msx[j], msy[j],
			k & 2047, &tx, &ty);

		dragpoint(x, s->x + tx, s->y + ty);

		j++;
	}
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void movefta(void)
{
	int x, px, py, sx, sy;
	short i, j, p, psect, ssect, nexti;
	spritetype* s;

	i = headspritestat[STAT_ZOMBIEACTOR];
	while (i >= 0)
	{
		nexti = nextspritestat[i];

		s = &sprite[i];
		p = findplayer(s, &x);

		ssect = psect = s->sectnum;

		if (sprite[ps[p].i].extra > 0)
		{
			if (x < 30000)
			{
				hittype[i].timetosleep++;
				if (hittype[i].timetosleep >= (x >> 8))
				{
					if (badguy(s))
					{
						px = ps[p].oposx + 64 - (krand() & 127);
						py = ps[p].oposy + 64 - (krand() & 127);
						updatesector(px, py, &psect);
						if (psect == -1)
						{
							i = nexti;
							continue;
						}
						sx = s->x + 64 - (krand() & 127);
						sy = s->y + 64 - (krand() & 127);
						updatesector(px, py, &ssect);
						if (ssect == -1)
						{
							i = nexti;
							continue;
						}

						if (!isRR() || s->pal == 33 || s->type == RR_VIXEN ||
							((isRRRA()) && isIn(s->type, RR_COOT, RR_COOTSTAYPUT, RR_BIKERSTAND, RR_BIKERRIDE, 
																			RR_BIKERRIDEDAISY, RR_MINIONAIRBOAT, RR_HULKAIRBOAT,
																			RR_DAISYAIRBOAT, RR_JACKOLOPE, RR_BANJOCOOTER, 
																			RR_GUITARBILLY, RR_MAMAJACKOLOPE, RR_BIKERBV, 
																			RR_MAKEOUT, RR_CHEER, RR_CHEERSTAYPUT)) ||
							 (sintable[(s->ang + 512) & 2047] * (px - sx) + sintable[s->ang & 2047] * (py - sy) >= 0))
						{
							int r1 = krand();
							int r2 = krand();
							j = cansee(sx, sy, s->z - (r2 % (52 << 8)), s->sectnum, px, py, ps[p].oposz - (r1 % (32 << 8)), ps[p].cursectnum);
						}
					}
					else
					{
						int r1 = krand();
						int r2 = krand();
						j = cansee(s->x, s->y, s->z - ((r2 & 31) << 8), s->sectnum, ps[p].oposx, ps[p].oposy, ps[p].oposz - ((r1 & 31) << 8), ps[p].cursectnum);
					}

					if (j)
					{
						bool res = (!isRR()) ?
							isIn(s->picnum,
								RUBBERCAN,
								EXPLODINGBARREL,
								WOODENHORSE,
								HORSEONSIDE,
								CANWITHSOMETHING,
								CANWITHSOMETHING2,
								CANWITHSOMETHING3,
								CANWITHSOMETHING4,
								FIREBARREL,
								FIREVASE,
								NUKEBARREL,
								NUKEBARRELDENTED,
								NUKEBARRELLEAKED,
								TRIPBOMB) :
							isIn(s->picnum,
								RR_1251,
								RR_1268,
								RR_1187,
								RR_1304,
								RR_1305,
								RR_1306,
								RR_1309,
								RR_1315,
								RR_1317,
								RR_1388);


						if (res)
						{
							if (sector[s->sectnum].ceilingstat & 1)
								s->shade = sector[s->sectnum].ceilingshade;
							else s->shade = sector[s->sectnum].floorshade;

							hittype[i].timetosleep = 0;
							changespritestat(i, STAT_STANDABLE);
						}
						else
						{
#if 0
							// TRANSITIONAL: RedNukem has this here. Needed?
							if (A_CheckSpriteFlags(spriteNum, SFLAG_USEACTIVATOR) && sector[sprite[spriteNum].sectnum].lotag & 16384) break;
#endif
							hittype[i].timetosleep = 0;
							check_fta_sounds(i);
							changespritestat(i, STAT_ACTOR);
						}
					}
					else hittype[i].timetosleep = 0;
				}
			}
			if ((!isRR() || !j) && badguy(s))
			{
				if (sector[s->sectnum].ceilingstat & 1)
					s->shade = sector[s->sectnum].ceilingshade;
				else s->shade = sector[s->sectnum].floorshade;

				if (s->picnum != RR_HEN || s->picnum != RR_COW || s->picnum != RR_PIG || s->picnum != RR_DOGRUN || ((isRRRA()) && s->picnum != RR_JACKOLOPE))
					if (wakeup(i, p))
					{
						hittype[i].timetosleep = 0;
						check_fta_sounds(i);
						changespritestat(i, STAT_ACTOR);
					}

			}
		}
		i = nexti;
	}
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

int ifhitsectors(int sectnum)
{
	int i = headspritestat[STAT_MISC];
	if (!isRR())
	{
		while (i >= 0)
		{
			if (sprite[i].picnum == EXPLOSION2 && sectnum == sprite[i].sectnum)
				return i;
			i = nextspritestat[i];
		}
	}
	else
	{
		while (i >= 0)
		{
			if (sprite[i].picnum == RR_EXPLOSION2 || (sprite[i].picnum == RR_EXPLOSION3 && sectnum == sprite[i].sectnum))
				return i;
			i = nextspritestat[i];
		}
	}

	return -1;
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

int ifhitbyweapon(int sn)
{
	short j, p;
	spritetype* npc;

	if (hittype[sn].extra >= 0)
	{
		if (sprite[sn].extra >= 0)
		{
			npc = &sprite[sn];

			if (npc->picnum == APLAYER)
			{
				if (ud.god && (isRR() || hittype[sn].picnum != SHRINKSPARK)) return -1;

				p = npc->yvel;
				j = hittype[sn].owner;

				if (j >= 0 &&
					sprite[j].picnum == APLAYER &&
					ud.coop == 1 &&
					ud.ffire == 0)
					return -1;

				npc->extra -= hittype[sn].extra;

				if (j >= 0)
				{
					if (npc->extra <= 0 && hittype[sn].picnum != (isRR()? RR_ALIENBLAST : FREEZEBLAST))
					{
						npc->extra = 0;

						ps[p].wackedbyactor = j;

						if (sprite[hittype[sn].owner].picnum == APLAYER && p != sprite[hittype[sn].owner].yvel)
						{
							// yvel contains player ID
							ps[p].frag_ps = sprite[j].yvel;
						}

						hittype[sn].owner = ps[p].i;
					}
				}

				bool res = !isRR() ?
					isIn(hittype[sn].picnum, RADIUSEXPLOSION, RPG, HYDRENT, HEAVYHBOMB, SEENINE, OOZFILTER, EXPLODINGBARREL) :
					(isIn(hittype[sn].picnum, RR_DYNAMITE, RR_POWDERKEGSPRITE, RR_1228, RR_1273, RR_1315, RR_SEENINE, RR_RADIUSEXPLOSION, RR_CROSSBOW) ||
						(isRRRA() && hittype[sn].picnum == RR_CHIKENCROSSBOW));

				int shift = res ? 2 : 1;
				ps[p].posxv += hittype[sn].extra * (sintable[(hittype[sn].ang + 512) & 2047]) << shift;
				ps[p].posyv += hittype[sn].extra * (sintable[hittype[sn].ang & 2047]) << shift;
			}
			else
			{
				if (hittype[sn].extra == 0)
					if ((isRR() || hittype[sn].picnum == SHRINKSPARK) && npc->xrepeat < 24)
						return -1;

				if (isWorldTour() && hittype[sn].picnum == FIREFLY && npc->xrepeat < 48) 
				{
					if (hittype[sn].picnum != RADIUSEXPLOSION && hittype[sn].picnum != RPG)
						return -1;
				}

				npc->extra -= hittype[sn].extra;
				if (npc->picnum != (isRR()? RR_4989 : RECON) && npc->owner >= 0 && sprite[npc->owner].statnum < MAXSTATUS)
					npc->owner = hittype[sn].owner;
			}

			hittype[sn].extra = -1;
			return hittype[sn].picnum;
		}
	}


	if (ud.multimode < 2 || !isWorldTour()
		|| hittype[sn].picnum != FLAMETHROWERFLAME
		|| hittype[sn].extra >= 0
		|| sprite[sn].extra > 0
		|| sprite[sn].picnum != APLAYER
		|| ps[sprite[sn].yvel].numloogs > 0
		|| hittype[sn].owner < 0) 
	{
		hittype[sn].extra = -1;
		return -1;
	}
	else 
	{
		p = sprite[sn].yvel;
		sprite[sn].extra = 0;
		ps[p].wackedbyactor = (short)hittype[sn].owner;

		if (sprite[hittype[sn].owner].picnum == APLAYER && p != hittype[sn].owner)
			ps[p].frag_ps = (short)hittype[sn].owner;

		hittype[sn].owner = ps[p].i;
		hittype[sn].extra = -1;

		return FLAMETHROWERFLAME;
	}


	hittype[sn].extra = -1;
	return -1;
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void movecyclers(void)
{
	short q, j, x, t, s, * c;
	walltype* wal;
	char cshade;

	for (q = numcyclers - 1; q >= 0; q--)
	{

		c = &cyclers[q][0];
		s = c[0];

		t = c[3];
		j = t + (sintable[c[1] & 2047] >> 10);
		cshade = c[2];

		if (j < cshade) j = cshade;
		else if (j > t)  j = t;

		c[1] += sector[s].extra;
		if (c[5])
		{
			wal = &wall[sector[s].wallptr];
			for (x = sector[s].wallnum; x > 0; x--, wal++)
				if (wal->hitag != 1)
				{
					wal->shade = j;

					if ((wal->cstat & 2) && wal->nextwall >= 0)
						wall[wal->nextwall].shade = j;

				}
			sector[s].floorshade = sector[s].ceilingshade = j;
		}
	}
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void movedummyplayers(void)
{
	short i, p, nexti;

	i = headspritestat[STAT_DUMMYPLAYER];
	while (i >= 0)
	{
		nexti = nextspritestat[i];

		p = sprite[sprite[i].owner].yvel;

		if ((!isRR() && ps[p].on_crane >= 0) || sector[ps[p].cursectnum].lotag != 1 || sprite[ps[p].i].extra <= 0)
		{
			ps[p].dummyplayersprite = -1;
			deletesprite(i);
			i = nexti;
			continue;
		}
		else
		{
			if (ps[p].on_ground && ps[p].on_warping_sector == 1 && sector[ps[p].cursectnum].lotag == 1)
			{
				sprite[i].cstat = 257;
				sprite[i].z = sector[sprite[i].sectnum].ceilingz + (27 << 8);
				sprite[i].ang = ps[p].q16ang >> FRACBITS;
				if (hittype[i].temp_data[0] == 8)
					hittype[i].temp_data[0] = 0;
				else hittype[i].temp_data[0]++;
			}
			else
			{
				if (sector[sprite[i].sectnum].lotag != 2) sprite[i].z = sector[sprite[i].sectnum].floorz;
				sprite[i].cstat = (short)32768;
			}
		}

		sprite[i].x += (ps[p].posx - ps[p].oposx);
		sprite[i].y += (ps[p].posy - ps[p].oposy);
		setsprite(i, sprite[i].x, sprite[i].y, sprite[i].z);
		i = nexti;
	}
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

int otherp;
void moveplayers(void) //Players
{
	short i, nexti;
	int otherx;
	spritetype* s;
	struct player_struct* p;

	i = headspritestat[STAT_PLAYER];
	while (i >= 0)
	{
		nexti = nextspritestat[i];

		s = &sprite[i];
		p = &ps[s->yvel];
		if (s->owner >= 0)
		{
			if (p->newowner >= 0) //Looking thru the camera
			{
				s->x = p->oposx;
				s->y = p->oposy;
				hittype[i].bposz = s->z = p->oposz + PHEIGHT;
				s->ang = p->oq16ang >> FRACBITS;
				setsprite(i, s->x, s->y, s->z);
			}
			else
			{
				if (ud.multimode > 1)
					otherp = findotherplayer(s->yvel, &otherx);
				else
				{
					otherp = s->yvel;
					otherx = 0;
				}

				execute(i, s->yvel, otherx);

				p->oq16ang = p->q16ang;

				if (ud.multimode > 1)
					if (sprite[ps[otherp].i].extra > 0)
					{
						if (s->yrepeat > 32 && sprite[ps[otherp].i].yrepeat < 32)
						{
							if (otherx < 1400 && p->knee_incs == 0)
							{
								p->knee_incs = 1;
								p->weapon_pos = -1;
								p->actorsqu = ps[otherp].i;
							}
						}
					}
				if (ud.god)
				{
					s->extra = p->max_player_health;
					s->cstat = 257;
					if (!isWW2GI() && !isRR())
						p->jetpack_amount = 1599;
				}


				if (s->extra > 0)
				{
					// currently alive...

					hittype[i].owner = i;

					if (ud.god == 0)
						if (ceilingspace(s->sectnum) || floorspace(s->sectnum))
							quickkill(p);
				}
				else
				{

					p->posx = s->x;
					p->posy = s->y;
					p->posz = s->z - (20 << 8);

					p->newowner = -1;

					if (p->wackedbyactor >= 0 && sprite[p->wackedbyactor].statnum < MAXSTATUS)
					{
						int ang = p->q16ang >> FRACBITS;
						ang += getincangle(ang, getangle(sprite[p->wackedbyactor].x - p->posx, sprite[p->wackedbyactor].y - p->posy)) >> 1;
						ang &= 2047;
						p->q16ang = ang << FRACBITS;
					}

				}
				s->ang = p->q16ang >> FRACBITS;
			}
		}
		else
		{
			if (p->holoduke_on == -1)
			{
				deletesprite(i);
				i = nexti;
				continue;
			}

			hittype[i].bposx = s->x;
			hittype[i].bposy = s->y;
			hittype[i].bposz = s->z;

			s->cstat = 0;

			if (s->xrepeat < 42)
			{
				s->xrepeat += 4;
				s->cstat |= 2;
			}
			else s->xrepeat = 42;
			if (s->yrepeat < 36)
				s->yrepeat += 4;
			else
			{
				s->yrepeat = 36;
				if (sector[s->sectnum].lotag != ST_2_UNDERWATER)
					makeitfall(i);
				if (s->zvel == 0 && sector[s->sectnum].lotag == ST_1_ABOVE_WATER)
					s->z += (32 << 8);
			}

			if (s->extra < 8)
			{
				s->xvel = 128;
				s->ang = p->q16ang >> FRACBITS;
				s->extra++;
				//IFMOVING;		// JBF 20040825: is really "if (ssp(i,CLIPMASK0)) ;" which is probably
				ssp(i, CLIPMASK0);	// not the safest of ideas because a zealous optimiser probably sees
							// it as redundant, so I'll call the "ssp(i,CLIPMASK0)" explicitly.
			}
			else
			{
				s->ang = 2047 - (p->q16ang >> FRACBITS);
				setsprite(i, s->x, s->y, s->z);
			}
		}

		if (sector[s->sectnum].ceilingstat & 1)
			s->shade += (sector[s->sectnum].ceilingshade - s->shade) >> 1;
		else
			s->shade += (sector[s->sectnum].floorshade - s->shade) >> 1;

		i = nexti;
	}
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void movefx(void)
{
	short i, j, nexti, p;
	int x, ht;
	spritetype* s;

	i = headspritestat[STAT_FX];
	while (i >= 0)
	{
		s = &sprite[i];

		nexti = nextspritestat[i];

		switch (s->picnum)
		{
		case RESPAWN:
			if (sprite[i].extra == 66)
			{
				j = spawn(i, sprite[i].hitag);
				if (isRRRA())
				{
					sprite[j].pal = sprite[i].pal;
					if (sprite[j].picnum == RR_MAMAJACKOLOPE)
					{
						if (sprite[j].pal == 30)
						{
							sprite[j].xrepeat = 26;
							sprite[j].yrepeat = 26;
							sprite[j].clipdist = 75;
						}
						else if (sprite[j].pal == 31)
						{
							sprite[j].xrepeat = 36;
							sprite[j].yrepeat = 36;
							sprite[j].clipdist = 100;
						}
						else if (sprite[j].pal == 32)
						{
							sprite[j].xrepeat = 50;
							sprite[j].yrepeat = 50;
							sprite[j].clipdist = 100;
						}
						else
						{
							sprite[j].xrepeat = 50;
							sprite[j].yrepeat = 50;
							sprite[j].clipdist = 100;
						}
					}

					if (sprite[j].pal == 8)
					{
						sprite[j].cstat |= 2;
					}

					if (sprite[j].pal != 6)
					{
						deletesprite(i);
						i = nexti;
						continue;
					}
					sprite[i].extra = (66 - 13);
					sprite[j].pal = 0;
				}
				else
				{
					deletesprite(i);
					i = nexti;
					continue;
				}
			}
			else if (sprite[i].extra > (66 - 13))
				sprite[i].extra++;
			break;

		case MUSICANDSFX:

			ht = s->hitag;

			if (hittype[i].temp_data[1] != (int)SoundEnabled())
			{
				hittype[i].temp_data[1] = SoundEnabled();
				hittype[i].temp_data[0] = 0;
			}

			if (s->lotag >= 1000 && s->lotag < 2000)
			{
				x = ldist(&sprite[ps[screenpeek].i], s);
				if (x < ht && hittype[i].temp_data[0] == 0)
				{
					FX_SetReverb(s->lotag - 1000);
					hittype[i].temp_data[0] = 1;
				}
				if (x >= ht && hittype[i].temp_data[0] == 1)
				{
					FX_SetReverb(0);
					FX_SetReverbDelay(0);
					hittype[i].temp_data[0] = 0;
				}
			}
			else if (s->lotag < 999 && (unsigned)sector[s->sectnum].lotag < ST_9_SLIDING_ST_DOOR && snd_ambience && sector[sprite[i].sectnum].floorz != sector[sprite[i].sectnum].ceilingz)
			{
				auto flags = S_GetUserFlags(s->lotag);
				if (flags & SF_MSFX)
				{
					int x = dist(&sprite[ps[screenpeek].i], s);

					if (x < ht && hittype[i].temp_data[0] == 0)
					{
						// Start playing an ambience sound.
						A_PlaySound(s->lotag, i, CHAN_AUTO, CHANF_LOOP);
						hittype[i].temp_data[0] = 1;  // AMBIENT_SFX_PLAYING
					}
					else if (x >= ht && hittype[i].temp_data[0] == 1)
					{
						// Stop playing ambience sound because we're out of its range.
						S_StopEnvSound(s->lotag, i);
					}
				}

				if ((flags & (SF_GLOBAL | SF_DTAG)) == SF_GLOBAL)
				{
					if (hittype[i].temp_data[4] > 0) hittype[i].temp_data[4]--;
					else for (p = connecthead; p >= 0; p = connectpoint2[p])
						if (p == myconnectindex && ps[p].cursectnum == s->sectnum)
						{
							S_PlaySound(s->lotag + (unsigned)global_random % (s->hitag + 1));
							hittype[i].temp_data[4] = 26 * 40 + (global_random % (26 * 40));
						}
				}
			}
			break;
		}
		i = nexti;
	}
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void movefallers(void)
{
	short i, nexti, sect, j;
	spritetype* s;
	int x;

	i = headspritestat[STAT_FALLER];
	while (i >= 0)
	{
		nexti = nextspritestat[i];
		s = &sprite[i];

		sect = s->sectnum;

		if (hittype[i].temp_data[0] == 0)
		{
			s->z -= (16 << 8);
			hittype[i].temp_data[1] = s->ang;
			x = s->extra;
			j = ifhitbyweapon(i);
			if (j >= 0) 
			{
				bool res = !isRR() ?
					isIn(j, FIREEXT, RPG, RADIUSEXPLOSION, SEENINE, OOZFILTER) :
					(isIn(j, RR_CROSSBOW, RR_RADIUSEXPLOSION, RR_SEENINE, RR_OOZFILTER) || (isRRRA() && j == RR_CHIKENCROSSBOW));

				if (res)
				{
					if (s->extra <= 0)
					{
						hittype[i].temp_data[0] = 1;
						j = headspritestat[12];
						while (j >= 0)
						{
							if (sprite[j].hitag == sprite[i].hitag)
							{
								hittype[j].temp_data[0] = 1;
								sprite[j].cstat &= (65535 - 64);
								if (!isRR() ? isIn(j, CEILINGSTEAM, STEAM) : isIn(j, RR_CEILINGSTEAM, RR_STEAM))
									sprite[j].cstat |= 32768;
							}
							j = nextspritestat[j];
						}
					}
				}
				else
				{
					hittype[i].extra = 0;
					s->extra = x;
				}
			}
			s->ang = hittype[i].temp_data[1];
			s->z += (16 << 8);
		}
		else if (hittype[i].temp_data[0] == 1)
		{
			if (s->lotag > 0)
			{
				s->lotag -= 3;
				if (isRR())
				{
					s->xvel = (64 + krand()) & 127;
					s->zvel = -(1024 + (krand() & 1023));
				}
				else if (s->lotag <= 0)
				{
					s->xvel = (32 + (krand() & 63));
					s->zvel = -(1024 + (krand() & 1023));
				}
			}
			else
			{
				if (s->xvel > 0)
				{
					s->xvel -= isRR()? 2 : 8;
					ssp(i, CLIPMASK0);
				}

				if (floorspace(s->sectnum)) x = 0;
				else
				{
					if (ceilingspace(s->sectnum))
						x = gc / 6;
					else
						x = gc;
				}

				if (s->z < (sector[sect].floorz - FOURSLEIGHT))
				{
					s->zvel += x;
					if (s->zvel > 6144)
						s->zvel = 6144;
					s->z += s->zvel;
				}
				if ((sector[sect].floorz - s->z) < (16 << 8))
				{
					j = 1 + (krand() & 7);
					for (x = 0; x < j; x++) RANDOMSCRAP(s, i);
					deletesprite(i);
				}
			}
		}

		i = nexti;
	}
}

//---------------------------------------------------------------------------
//
// split out of movestandables
//
//---------------------------------------------------------------------------

static void movecrane(int i)
{
	auto t = &hittype[i].temp_data[0];
	auto s = &sprite[i];
	int sect = s->sectnum;
	int x;
	int crane = pPick(CRANE);

	//t[0] = state
	//t[1] = checking sector number

	if (s->xvel) getglobalz(i);

	if (t[0] == 0) //Waiting to check the sector
	{
		int j = headspritesect[t[1]];
		while (j >= 0)
		{
			int nextj = nextspritesect[j];
			switch (sprite[j].statnum)
			{
			case STAT_ACTOR:
			case STAT_ZOMBIEACTOR:
			case STAT_STANDABLE:
			case STAT_PLAYER:
				s->ang = getangle(msx[t[4] + 1] - s->x, msy[t[4] + 1] - s->y);
				setsprite(j, msx[t[4] + 1], msy[t[4] + 1], sprite[j].z);
				t[0]++;
				deletesprite(i);
				return;
			}
			j = nextj;
		}
	}

	else if (t[0] == 1)
	{
		if (s->xvel < 184)
		{
			s->picnum = crane + 1;
			s->xvel += 8;
		}
		//IFMOVING;	// JBF 20040825: see my rant above about this
		ssp(i, CLIPMASK0);
		if (sect == t[1])
			t[0]++;
	}
	else if (t[0] == 2 || t[0] == 7)
	{
		s->z += (1024 + 512);

		if (t[0] == 2)
		{
			if ((sector[sect].floorz - s->z) < (64 << 8))
				if (s->picnum > crane) s->picnum--;

			if ((sector[sect].floorz - s->z) < (4096 + 1024))
				t[0]++;
		}
		if (t[0] == 7)
		{
			if ((sector[sect].floorz - s->z) < (64 << 8))
			{
				if (s->picnum > crane) s->picnum--;
				else
				{
					if (s->owner == -2)
					{
						auto p = findplayer(s, &x);
						spritesound(isRR() ? 390 : DUKE_GRUNT, ps[p].i);
						if (ps[p].on_crane == i)
							ps[p].on_crane = -1;
					}
					t[0]++;
					s->owner = -1;
				}
			}
		}
	}
	else if (t[0] == 3)
	{
		s->picnum++;
		if (s->picnum == (crane + 2))
		{
			auto p = checkcursectnums(t[1]);
			if (p >= 0 && ps[p].on_ground)
			{
				s->owner = -2;
				ps[p].on_crane = i;
				spritesound(isRR() ? 390 : DUKE_GRUNT, ps[p].i);
				ps[p].q16ang = (s->ang + 1024) << FRACBITS;
			}
			else
			{
				int j = headspritesect[t[1]];
				while (j >= 0)
				{
					switch (sprite[j].statnum)
					{
					case 1:
					case 6:
						s->owner = j;
						break;
					}
					j = nextspritesect[j];
				}
			}

			t[0]++;//Grabbed the sprite
			t[2] = 0;
			return;
		}
	}
	else if (t[0] == 4) //Delay before going up
	{
		t[2]++;
		if (t[2] > 10)
			t[0]++;
	}
	else if (t[0] == 5 || t[0] == 8)
	{
		if (t[0] == 8 && s->picnum < (crane + 2))
			if ((sector[sect].floorz - s->z) > 8192)
				s->picnum++;

		if (s->z < msx[t[4] + 2])
		{
			t[0]++;
			s->xvel = 0;
		}
		else
			s->z -= (1024 + 512);
	}
	else if (t[0] == 6)
	{
		if (s->xvel < 192)
			s->xvel += 8;
		s->ang = getangle(msx[t[4]] - s->x, msy[t[4]] - s->y);
		//IFMOVING;	// JBF 20040825: see my rant above about this
		ssp(i, CLIPMASK0);
		if (((s->x - msx[t[4]]) * (s->x - msx[t[4]]) + (s->y - msy[t[4]]) * (s->y - msy[t[4]])) < (128 * 128))
			t[0]++;
	}

	else if (t[0] == 9)
		t[0] = 0;

	setsprite(msy[t[4] + 2], s->x, s->y, s->z - (34 << 8));

	if (s->owner != -1)
	{
		auto p = findplayer(s, &x);

		int j = ifhitbyweapon(i);
		if (j >= 0)
		{
			if (s->owner == -2)
				if (ps[p].on_crane == i)
					ps[p].on_crane = -1;
			s->owner = -1;
			s->picnum = crane;
			return;
		}

		if (s->owner >= 0)
		{
			setsprite(s->owner, s->x, s->y, s->z);

			hittype[s->owner].bposx = s->x;
			hittype[s->owner].bposy = s->y;
			hittype[s->owner].bposz = s->z;

			s->zvel = 0;
		}
		else if (s->owner == -2)
		{
			auto ang = ps[p].q16ang >> FRACBITS;
			ps[p].oposx = ps[p].posx = s->x - (sintable[(ang + 512) & 2047] >> 6);
			ps[p].oposy = ps[p].posy = s->y - (sintable[ang & 2047] >> 6);
			ps[p].oposz = ps[p].posz = s->z + (2 << 8);
			setsprite(ps[p].i, ps[p].posx, ps[p].posy, ps[p].posz);
			ps[p].cursectnum = sprite[ps[p].i].sectnum;
		}
	}
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

static void movefountain(int i)
{
	auto t = &hittype[i].temp_data[0];
	auto s = &sprite[i];
	int x;
	if (t[0] > 0)
	{
		if (t[0] < 20)
		{
			t[0]++;

			s->picnum++;

			if (s->picnum == (pPick(WATERFOUNTAIN) + 3))
				s->picnum = pPick(WATERFOUNTAIN) + 1;
		}
		else
		{
			findplayer(s, &x);

			if (x > 512)
			{
				t[0] = 0;
				s->picnum = pPick(WATERFOUNTAIN);
			}
			else t[0] = 1;
		}
	}
}
//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

static void moveflammable(int i)
{
	auto s = &sprite[i];
	int j;
	if (hittype[i].temp_data[0] == 1)
	{
		hittype[i].temp_data[1]++;
		if ((hittype[i].temp_data[1] & 3) > 0) return;

		if (!isRR() && s->picnum == TIRE && hittype[i].temp_data[1] == 32)
		{
			s->cstat = 0;
			j = spawn(i, BLOODPOOL);
			sprite[j].shade = 127;
		}
		else
		{
			if (s->shade < 64) s->shade++;
			else
			{
				deletesprite(i);
				return;
			}
		}

		j = s->xrepeat - (krand() & 7);
		if (j < 10) 
		{
			deletesprite(i);
			return;
		}

		s->xrepeat = j;

		j = s->yrepeat - (krand() & 7);
		if (j < 4) 
		{
			deletesprite(i);
			return;
		}
		s->yrepeat = j;
	}
	if (!isRR() && s->picnum == BOX)
	{
		makeitfall(i);
		hittype[i].ceilingz = sector[s->sectnum].ceilingz;
	}
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

static void movetripbomb(int i)
{
	auto s = &sprite[i];
	int j, x;
	int lTripBombControl = GetGameVar("TRIPBOMB_CONTROL", TRIPBOMB_TRIPWIRE, -1, -1);
	if (lTripBombControl & TRIPBOMB_TIMER)
	{
		// we're on a timer....
		if (s->extra >= 0)
		{
			s->extra--;
			if (s->extra == 0)
			{
				hittype[i].temp_data[2] = 16;
				spritesound(LASERTRIP_ARMING, i);
			}
		}
	}
	if (hittype[i].temp_data[2] > 0)
	{
		hittype[i].temp_data[2]--;
		if (hittype[i].temp_data[2] == 8)
		{
			spritesound(LASERTRIP_EXPLODE, i);
			for (j = 0; j < 5; j++) RANDOMSCRAP(s, i);
			x = s->extra;
			hitradius(i, tripbombblastradius, x >> 2, x >> 1, x - (x >> 2), x);

			j = spawn(i, EXPLOSION2);
			sprite[j].ang = s->ang;
			sprite[j].xvel = 348;
			ssp(j, CLIPMASK0);

			j = headspritestat[5];
			while (j >= 0)
			{
				if (sprite[j].picnum == LASERLINE && s->hitag == sprite[j].hitag)
					sprite[j].xrepeat = sprite[j].yrepeat = 0;
				j = nextspritestat[j];
			}
			deletesprite(i);
		}
		return;
	}
	else
	{
		x = s->extra;
		s->extra = 1;
		int16_t l = s->ang;
		j = ifhitbyweapon(i);
		if (j >= 0)
		{ 
			hittype[i].temp_data[2] = 16; 
		}
		s->extra = x;
		s->ang = l;
	}

	if (hittype[i].temp_data[0] < 32)
	{
		findplayer(s, &x);
		if (x > 768) hittype[i].temp_data[0]++;
		else if (hittype[i].temp_data[0] > 16) hittype[i].temp_data[0]++;
	}
	if (hittype[i].temp_data[0] == 32)
	{
		int16_t l = s->ang;
		s->ang = hittype[i].temp_data[5];

		hittype[i].temp_data[3] = s->x; hittype[i].temp_data[4] = s->y;
		s->x += sintable[(hittype[i].temp_data[5] + 512) & 2047] >> 9;
		s->y += sintable[(hittype[i].temp_data[5]) & 2047] >> 9;
		s->z -= (3 << 8);
		setsprite(i, s->x, s->y, s->z);

		int16_t m;
		x = hitasprite(i, &m);

		hittype[i].lastvx = x;

		s->ang = l;

		int k = 0;

		if (lTripBombControl & TRIPBOMB_TRIPWIRE)
		{
			// we're on a trip wire
			while (x > 0)
			{
				j = spawn(i, LASERLINE);
				setsprite(j, sprite[j].x, sprite[j].y, sprite[j].z);
				sprite[j].hitag = s->hitag;
				hittype[j].temp_data[1] = sprite[j].z;

				s->x += sintable[(hittype[i].temp_data[5] + 512) & 2047] >> 4;
				s->y += sintable[(hittype[i].temp_data[5]) & 2047] >> 4;

				if (x < 1024)
				{
					sprite[j].xrepeat = x >> 5;
					break;
				}
				x -= 1024;
			}
		}

		hittype[i].temp_data[0]++;
		s->x = hittype[i].temp_data[3]; s->y = hittype[i].temp_data[4];
		s->z += (3 << 8);
		setsprite(i, s->x, s->y, s->z);
		hittype[i].temp_data[3] = 0;
		if (m >= 0 && lTripBombControl & TRIPBOMB_TRIPWIRE)
		{
			hittype[i].temp_data[2] = 13;
			spritesound(LASERTRIP_ARMING, i);
		}
		else hittype[i].temp_data[2] = 0;
	}
	if (hittype[i].temp_data[0] == 33)
	{
		hittype[i].temp_data[1]++;


		hittype[i].temp_data[3] = s->x; hittype[i].temp_data[4] = s->y;
		s->x += sintable[(hittype[i].temp_data[5] + 512) & 2047] >> 9;
		s->y += sintable[(hittype[i].temp_data[5]) & 2047] >> 9;
		s->z -= (3 << 8);
		setsprite(i, s->x, s->y, s->z);

		int16_t m;
		x = hitasprite(i, &m);

		s->x = hittype[i].temp_data[3]; s->y = hittype[i].temp_data[4];
		s->z += (3 << 8);
		setsprite(i, s->x, s->y, s->z);

		if (hittype[i].lastvx != x && lTripBombControl & TRIPBOMB_TRIPWIRE)
		{
			hittype[i].temp_data[2] = 13;
			spritesound(LASERTRIP_ARMING, i);
		}
	}
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

static void detonate(int i)
{
	auto s = &sprite[i];
	auto t = &hittype[i].temp_data[0];
	earthquaketime = 16;

	int j = headspritestat[STAT_EFFECTOR];
	while (j >= 0)
	{
		if (s->hitag == sprite[j].hitag)
		{
			if (sprite[j].lotag == SE_13_EXPLOSIVE)
			{
				if (hittype[j].temp_data[2] == 0)
					hittype[j].temp_data[2] = 1;
			}
			else if (sprite[j].lotag == SE_8_UP_OPEN_DOOR_LIGHTS)
				hittype[j].temp_data[4] = 1;
			else if (sprite[j].lotag == SE_18_INCREMENTAL_SECTOR_RISE_FALL)
			{
				if (hittype[j].temp_data[0] == 0)
					hittype[j].temp_data[0] = 1;
			}
			else if (sprite[j].lotag == SE_21_DROP_FLOOR)
				hittype[j].temp_data[0] = 1;
		}
		j = nextspritestat[j];
	}

	s->z -= (32 << 8);

	if ((t[3] == 1 && s->xrepeat) || s->lotag == -99)
	{
		int x = s->extra;
		spawn(i, EXPLOSION2);
		hitradius(i, seenineblastradius, x >> 2, x - (x >> 1), x - (x >> 2), x);
		spritesound(PIPEBOMB_EXPLODE, i);
	}

	if (s->xrepeat)
		for (int x = 0; x < 8; x++) RANDOMSCRAP(s, i);

	deletesprite(i);

}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

static void movecrack(int i)
{
	auto s = &sprite[i];
	auto t = &hittype[i].temp_data[0];
	if (s->hitag > 0)
	{
		t[0] = s->cstat;
		t[1] = s->ang;
		int j = ifhitbyweapon(i);

		bool res = !isRR() ?
			isIn(j, FIREEXT, RPG, RADIUSEXPLOSION, SEENINE, OOZFILTER) :
			(isIn(j, RR_CROSSBOW, RR_RADIUSEXPLOSION, RR_SEENINE, RR_OOZFILTER) || (isRRRA() && j == RR_CHIKENCROSSBOW));

		if (res)
		{
			j = headspritestat[STAT_STANDABLE];
			while (j >= 0)
			{
				if (s->hitag == sprite[j].hitag && (isRR() ? isIn(sprite[j].picnum, RR_OOZFILTER, RR_SEENINE) : isIn(sprite[j].picnum, OOZFILTER, SEENINE)))
					if (sprite[j].shade != -32)
						sprite[j].shade = -32;
				j = nextspritestat[j];
			}
			detonate(i);
		}
		else
		{
			s->cstat = t[0];
			s->ang = t[1];
			s->extra = 0;
		}
	}
}

//---------------------------------------------------------------------------
//
// Duke only
//
//---------------------------------------------------------------------------

static void movefireext(int i)
{
	int j = ifhitbyweapon(i);
	if (j == -1) return;

	auto s = &sprite[i];
	auto t = &hittype[i].temp_data[0];

	for (int k = 0; k < 16; k++)
	{
		j = EGS(sprite[i].sectnum, sprite[i].x, sprite[i].y, sprite[i].z - (krand() % (48 << 8)), SCRAP3 + (krand() & 3), -8, 48, 48, krand() & 2047, (krand() & 63) + 64, -(krand() & 4095) - (sprite[i].zvel >> 2), i, 5);
		sprite[j].pal = 2;
	}

	spawn(i, EXPLOSION2);
	spritesound(PIPEBOMB_EXPLODE, i);
	spritesound(GLASS_HEAVYBREAK, i);

	if (s->hitag > 0)
	{
		j = headspritestat[6];
		while (j >= 0)
		{
			if (s->hitag == sprite[j].hitag && (sprite[j].picnum == OOZFILTER || sprite[j].picnum == SEENINE))
				if (sprite[j].shade != -32)
					sprite[j].shade = -32;
			j = nextspritestat[j];
		}

		int x = s->extra;
		spawn(i, EXPLOSION2);
		hitradius(i, pipebombblastradius, x >> 2, x - (x >> 1), x - (x >> 2), x);
		spritesound(PIPEBOMB_EXPLODE, i);
		detonate(i);
	}
	else
	{
		hitradius(i, seenineblastradius, 10, 15, 20, 25);
		deletesprite(i);
	}
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

static void moveooz(int i)
{
	auto s = &sprite[i];
	auto t = &hittype[i].temp_data[0];
	int j;
	if (s->shade != -32 && s->shade != -33)
	{
		if (s->xrepeat)
			j = (ifhitbyweapon(i) >= 0);
		else
			j = 0;

		if (j || s->shade == -31)
		{
			if (j) s->lotag = 0;

			t[3] = 1;

			j = headspritestat[STAT_STANDABLE];
			while (j >= 0)
			{
				if (s->hitag == sprite[j].hitag && (sprite[j].picnum == pPick(SEENINE) || sprite[j].picnum == pPick(OOZFILTER)))
					sprite[j].shade = -32;
				j = nextspritestat[j];
			}
		}
	}
	else
	{
		if (s->shade == -32)
		{
			if (s->lotag > 0)
			{
				s->lotag -= 3;
				if (s->lotag <= 0) s->lotag = -99;
			}
			else
				s->shade = -33;
		}
		else
		{
			if (s->xrepeat > 0)
			{
				hittype[i].temp_data[2]++;
				if (hittype[i].temp_data[2] == 3)
				{
					if (s->picnum == pPick(OOZFILTER))
					{
						hittype[i].temp_data[2] = 0;
						detonate(i);
						return;
					}
					if (s->picnum != (pPick(SEENINEDEAD) + 1))
					{
						hittype[i].temp_data[2] = 0;

						if (s->picnum == pPick(SEENINEDEAD)) s->picnum++;
						else if (s->picnum == pPick(SEENINE))
							s->picnum = pPick(SEENINEDEAD);
					}
					else
					{
						detonate(i);
						return;
					}
				}
				return;
			}
			detonate(i);
			return;
		}
	}
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void movemasterswitch(int i)
{
	auto s = &sprite[i];
	auto t = &hittype[i].temp_data[0];
	if (s->yvel == 1)
	{
		s->hitag--;
		if (s->hitag <= 0)
		{
			operatesectors(s->sectnum, i);

			int j = headspritesect[s->sectnum];
			while (j >= 0)
			{
				if (sprite[j].statnum == 3)
				{
					switch (sprite[j].lotag)
					{
					case SE_2_EARTHQUAKE:
					case SE_21_DROP_FLOOR:
					case SE_31_FLOOR_RISE_FALL:
					case SE_32_CEILING_RISE_FALL:
					case SE_36_PROJ_SHOOTER:
						hittype[j].temp_data[0] = 1;
						break;
					case SE_3_RANDOM_LIGHTS_AFTER_SHOT_OUT:
						hittype[j].temp_data[4] = 1;
						break;
					}
				}
				else if (sprite[j].statnum == 6)
				{
					if (sprite[j].picnum == pPick(SEENINE) || sprite[j].picnum == pPick(OOZFILTER))
					{
						sprite[j].shade = -31;
					}
				}
				j = nextspritesect[j];
			}
			deletesprite(i);
		}
	}
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

static void moveviewscreen(int i)
{
	auto s = &sprite[i];
	if (s->xrepeat == 0) deletesprite(i);
	else
	{
		int x;
		findplayer(s, &x);

		if (x < 2048)
		{
#if 0
			if (SP == 1)
				camsprite = i;
#endif
		}
		else if (camsprite != -1 && hittype[i].temp_data[0] == 1)
		{
			camsprite = -1;
			s->yvel = 0;
			hittype[i].temp_data[0] = 0;
		}
	}
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

static void movetrash(int i)
{
	auto s = &sprite[i];
	if (s->xvel == 0) s->xvel = 1;
	if (ssp(i, CLIPMASK0))
	{
		makeitfall(i);
		if (krand() & 1) s->zvel -= 256;
		if (klabs(s->xvel) < 48)
			s->xvel += (krand() & 3);
	}
	else deletesprite(i);
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

static void movesidebolt(int i)
{
	auto s = &sprite[i];
	auto t = &hittype[i].temp_data[0];
	int x;
	int sect = s->sectnum;

	auto p = findplayer(s, &x);
	if (x > 20480) return;

CLEAR_THE_BOLT2:
	if (t[2])
	{
		t[2]--;
		return;
	}
	if ((s->xrepeat | s->yrepeat) == 0)
	{
		s->xrepeat = t[0];
		s->yrepeat = t[1];
	}
	if ((krand() & 8) == 0)
	{
		t[0] = s->xrepeat;
		t[1] = s->yrepeat;
		t[2] = global_random & 4;
		s->xrepeat = s->yrepeat = 0;
		goto CLEAR_THE_BOLT2;
	}
	s->picnum++;

#if 0
	// content of l was undefined.
	if (l & 1) s->cstat ^= 2;
#endif

	if ((krand() & 1) && sector[sect].floorpicnum == HURTRAIL)
		spritesound(SHORT_CIRCUIT, i);

	if (s->picnum == SIDEBOLT1 + 4) s->picnum = SIDEBOLT1;
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

static void movebolt(int i)
{
	auto s = &sprite[i];
	auto t = &hittype[i].temp_data[0];
	int x;
	int sect = s->sectnum;

	auto p = findplayer(s, &x);
	if (x > 20480) return;

	if (t[3] == 0)
		t[3] = sector[sect].floorshade;

CLEAR_THE_BOLT:
	if (t[2])
	{
		t[2]--;
		sector[sect].floorshade = 20;
		sector[sect].ceilingshade = 20;
		return;
	}
	if ((s->xrepeat | s->yrepeat) == 0)
	{
		s->xrepeat = t[0];
		s->yrepeat = t[1];
	}
	else if ((krand() & 8) == 0)
	{
		t[0] = s->xrepeat;
		t[1] = s->yrepeat;
		t[2] = global_random & 4;
		s->xrepeat = s->yrepeat = 0;
		goto CLEAR_THE_BOLT;
	}
	s->picnum++;

	int l = global_random & 7;
	s->xrepeat = l + 8;

	if (l & 1) s->cstat ^= 2;

	auto bolt1 = pPick(BOLT1);
	if (s->picnum == (bolt1 + 1) && (isRR() ? (krand() & 1) != 0 : (krand() & 7) == 0) && sector[sect].floorpicnum == pPick(HURTRAIL) )
		spritesound(SHORT_CIRCUIT, i);

	if (s->picnum == bolt1 + 4) s->picnum = bolt1;

	if (s->picnum & 1)
	{
		sector[sect].floorshade = 0;
		sector[sect].ceilingshade = 0;
	}
	else
	{
		sector[sect].floorshade = 20;
		sector[sect].ceilingshade = 20;
	}
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

static void movewaterdrip(int i)
{
	auto s = &sprite[i];
	auto t = &hittype[i].temp_data[0];
	int sect = s->sectnum;

	if (t[1])
	{
		t[1]--;
		if (t[1] == 0)
			s->cstat &= 32767;
	}
	else
	{
		makeitfall(i);
		ssp(i, CLIPMASK0);
		if (s->xvel > 0) s->xvel -= 2;

		if (s->zvel == 0)
		{
			s->cstat |= 32768;

			if (s->pal != 2 && (isRR() || s->hitag == 0))
				spritesound(SOMETHING_DRIPPING, i);

			if (sprite[s->owner].picnum != pPick(WATERDRIP))
			{
				deletesprite(i);
			}
			else
			{
				hittype[i].bposz = s->z = t[0];
				t[1] = 48 + (krand() & 31);
			}
		}
	}
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

static void movedoorshock(int i)
{
	auto s = &sprite[i];
	int sect = s->sectnum;
	int j = abs(sector[sect].ceilingz - sector[sect].floorz) >> 9;
	s->yrepeat = j + 4;
	s->xrepeat = 16;
	s->z = sector[sect].floorz;
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

static void movetouchplate(int i)
{
	auto s = &sprite[i];
	auto t = &hittype[i].temp_data[0];
	int sect = s->sectnum;
	int x;
	int p;

	if (t[1] == 1 && s->hitag >= 0) //Move the sector floor
	{
		x = sector[sect].floorz;

		if (t[3] == 1)
		{
			if (x >= t[2])
			{
				sector[sect].floorz = x;
				t[1] = 0;
			}
			else
			{
				sector[sect].floorz += sector[sect].extra;
				p = checkcursectnums(sect);
				if (p >= 0) ps[p].posz += sector[sect].extra;
			}
		}
		else
		{
			if (x <= s->z)
			{
				sector[sect].floorz = s->z;
				t[1] = 0;
			}
			else
			{
				sector[sect].floorz -= sector[sect].extra;
				p = checkcursectnums(sect);
				if (p >= 0)
					ps[p].posz -= sector[sect].extra;
			}
		}
		return;
	}

	if (t[5] == 1) return;

	p = checkcursectnums(sect);
	if (p >= 0 && (ps[p].on_ground || s->ang == 512))
	{
		if (t[0] == 0 && !check_activator_motion(s->lotag))
		{
			t[0] = 1;
			t[1] = 1;
			t[3] = !t[3];
			operatemasterswitches(s->lotag);
			operateactivators(s->lotag, p);
			if (s->hitag > 0)
			{
				s->hitag--;
				if (s->hitag == 0) t[5] = 1;
			}
		}
	}
	else t[0] = 0;

	if (t[1] == 1)
	{
		int j = headspritestat[STAT_STANDABLE];
		while (j >= 0)
		{
			if (j != i && sprite[j].picnum == TOUCHPLATE && sprite[j].lotag == s->lotag)
			{
				hittype[j].temp_data[1] = 1;
				hittype[j].temp_data[3] = t[3];
			}
			j = nextspritestat[j];
		}
	}
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

static void movecanwithsomething(int i)
{
	auto s = &sprite[i];
	makeitfall(i);
	int j = ifhitbyweapon(i);
	if (j >= 0)
	{
		spritesound(VENT_BUST, i);
		for (j = 0; j < 10; j++)
			RANDOMSCRAP(s, i);

		if (s->lotag) spawn(i, s->lotag);

		deletesprite(i);
	}
}

//---------------------------------------------------------------------------
//
// this has been broken up into lots of smaller subfunctions
//
//---------------------------------------------------------------------------

void movestandables(void)
{
	int nexti;
	
	for (int i = headspritestat[STAT_STANDABLE]; i >= 0; i = nexti)
	{
		nexti = nextspritestat[i];

		auto s = &sprite[i];

		if (s->sectnum < 0)
		{
			deletesprite(i);
			continue;
		}

		hittype[i].bposx = s->x;
		hittype[i].bposy = s->y;
		hittype[i].bposz = s->z;

		if (s->picnum >= pPick(CRANE) && s->picnum <= pPick(CRANE) +3)
		{
			movecrane(i);
		}

		else if (s->picnum >= pPick(WATERFOUNTAIN) && s->picnum <= pPick(WATERFOUNTAIN) + 3)
		{
			movefountain(i);
		}

		else if (AFLAMABLE(s->picnum))
		{
			moveflammable(i);
		}

		else if (!isRR() && s->picnum == TRIPBOMB)
		{
			movetripbomb(i);
		}

		else if (s->picnum >= pPick(CRACK1) && s->picnum <= pPick(CRACK1)+3)
		{
			movecrack(i);
		}

		else if (!isRR() && s->picnum == FIREEXT)
		{
			movefireext(i);
		}

		else if (s->picnum == pPick(OOZFILTER) || s->picnum == pPick(SEENINE) || s->picnum == pPick(SEENINEDEAD) || s->picnum == (pPick(SEENINEDEAD) + 1))
		{
			moveooz(i);
		}

		else if (s->picnum == MASTERSWITCH)
		{
			movemasterswitch(i);
		}

		else if (!isRR() && (s->picnum == VIEWSCREEN || s->picnum == VIEWSCREEN2))
		{
			moveviewscreen(i);
		}

		else if (s->picnum == pPick(TRASH))
		{
			movetrash(i);
		}

		else if (!isRR() && s->picnum >= SIDEBOLT1 && s->picnum <= SIDEBOLT1 + 3)
		{
			movesidebolt(i);
		}

		else if (s->picnum >= pPick(BOLT1) && s->picnum <= pPick(BOLT1) + 3)
		{
			movebolt(i);
		}

		else if (s->picnum == pPick(WATERDRIP))
		{
			movewaterdrip(i);
		}

		else if (s->picnum == pPick(DOORSHOCK))
		{
			movedoorshock(i);
		}

		else if (s->picnum == TOUCHPLATE)
		{
			movetouchplate(i);
		}

		else if (isRR() ? s->picnum == RR_CANWITHSOMETHING : isIn(s->picnum, CANWITHSOMETHING, CANWITHSOMETHING2, CANWITHSOMETHING3, CANWITHSOMETHING4))
		{
			movecanwithsomething(i);
		}

		else if (!isRR() ?
			isIn(s->picnum,
				EXPLODINGBARREL,
				WOODENHORSE,
				HORSEONSIDE,
				FLOORFLAME,
				FIREBARREL,
				FIREVASE,
				NUKEBARREL,
				NUKEBARRELDENTED,
				NUKEBARRELLEAKED,
				TOILETWATER,
				RUBBERCAN,
				STEAM,
				CEILINGSTEAM,
				WATERBUBBLEMAKER) :
			isIn(s->picnum,
				RR_1187,
				RR_1196,
				RR_1251,
				RR_1268,
				RR_1304,
				RR_1305,
				RR_1306,
				RR_1315,
				RR_1317,
				RR_1388,
				RR_STEAM,
				RR_CEILINGSTEAM,
				RR_WATERBUBBLEMAKER)
			)
		{
			int x;
			int p = findplayer(s, &x);
			execute(i, p, x);
		}
	}
}



END_DUKE_NS
