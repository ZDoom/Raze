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
			return !(g_gameType & GAMEFLAG_RRALL);

		case RR_MOONSKY1:
		case RR_BIGORBIT1:
			return !!(g_gameType & GAMEFLAG_RRALL);
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
			return !(g_gameType & GAMEFLAG_RRALL);

		case RR_MOONSKY1:
		case RR_BIGORBIT1:
			return !!(g_gameType & GAMEFLAG_RRALL);
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
			if (g_gameType & GAMEFLAG_RRRA) p->ammo_amount[GROW_WEAPON] = 1;
		}
		if (g_gameType & GAMEFLAG_RRRA)
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

		if (!(g_gameType & GAMEFLAG_RRALL) || weapon != HANDBOMB_WEAPON)
			cw = weapon;
	}
	else
		cw = weapon;

	if ((g_gameType & GAMEFLAG_RRALL) && weapon == HANDBOMB_WEAPON)
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
	int max = ((g_gameType & GAMEFLAG_RRALL) ? DEVISTATOR_WEAPON : FREEZE_WEAPON);
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
	if (g_gameType & GAMEFLAG_WW2GI)
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
		if (!(g_gameType & GAMEFLAG_RRALL))
		{
			if (s->picnum == RPG) goto SKIPWALLCHECK;
		}
		else
		{
			if (s->picnum == RR_CROSSBOW || ((g_gameType & GAMEFLAG_RRRA) && s->picnum == RR_CHIKENCROSSBOW)) goto SKIPWALLCHECK;
		}
	}

	if ((g_gameType & GAMEFLAG_RRALL) || s->picnum != SHRINKSPARK)
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

	int val = (g_gameType & GAMEFLAG_RRALL) ? 24 : 16;
	q = -(val << 8) + (krand() & ((32 << 8) - 1));

	for (x = 0; x < 7; x++)
	{
		j = headspritestat[statlist[x]];
		while (j >= 0)
		{
			nextj = nextspritestat[j];
			sj = &sprite[j];

			if (x == 0 || x >= 5 || AFLAMABLE(sj->picnum))
			{
				if ((!(g_gameType & GAMEFLAG_RRALL) && s->picnum != SHRINKSPARK) || (sj->cstat & 257))
					if (dist(s, sj) < r)
					{
						if (badguy(sj) && !cansee(sj->x, sj->y, sj->z + q, sj->sectnum, s->x, s->y, s->z + q, s->sectnum))
							goto BOLT;
						checkhitsprite(j, i);
					}
			}
			else if (!(g_gameType & GAMEFLAG_RRALL))
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
						else
						{
							if (s->picnum == SHRINKSPARK)
								hittype[j].picnum = SHRINKSPARK;
							else hittype[j].picnum = RADIUSEXPLOSION;
						}

						if (s->picnum != SHRINKSPARK)
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
					if ((g_gameType & GAMEFLAG_RRRA) && s->picnum == RR_CHEERBOMB && j == s->owner)
					{
						j = nextj;
						continue;
					}

					if (sj->picnum == APLAYER) sj->z -= PHEIGHT;
					d = dist(s, sj);
					if (sj->picnum == APLAYER) sj->z += PHEIGHT;

					if (d < r && cansee(sj->x, sj->y, sj->z - (8 << 8), sj->sectnum, s->x, s->y, s->z - (12 << 8), s->sectnum))
					{
						if ((g_gameType & GAMEFLAG_RRRA) && sprite[j].picnum == RR_MINION && sprite[j].pal == 19)
						{
							j = nextj;
							continue;
						}

						hittype[j].ang = getangle(sj->x - s->x, sj->y - s->y);

						if (s->picnum == RR_CROSSBOW && sj->extra > 0)
							hittype[j].picnum = RR_CROSSBOW;
						else if ((g_gameType & GAMEFLAG_RRRA) && s->picnum == RR_CHIKENCROSSBOW && sj->extra > 0)
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
						if ((g_gameType & GAMEFLAG_RRRA)? 
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
	short retval, dasectnum, a, cd;
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
			if (g_gameType & GAMEFLAG_RRALL)
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

		bool rr = (g_gameType & GAMEFLAG_RRALL);
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
// lotsofmoney -> MONEY
// lotsofmail -> MAIL
// lotsofpaper -> PAPER
//
//---------------------------------------------------------------------------

void lotsofstuff(spritetype* s, short n, int spawntype)
{
	short i, j;
	for (i = n; i > 0; i--)
	{
		j = EGS(s->sectnum, s->x, s->y, s->z - (krand() % (47 << 8)), spawntype, -32, 8, 8, krand() & 2047, 0, 0, 0, 5);
		sprite[j].cstat = krand() & 12;
	}
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

bool ifsquished(int i, int p) 
{
	if (g_gameType & GAMEFLAG_RRALL) return false;	// this function is a no-op in RR's source.

	bool squishme = false;
	if (sprite[i].picnum == TILE_APLAYER && ud.clipping)
		return false;

	auto &sc = sector[sprite[i].sectnum];
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


END_DUKE_NS
