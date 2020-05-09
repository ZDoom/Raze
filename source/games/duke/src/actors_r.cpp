//-------------------------------------------------------------------------
/*
Copyright (C) 1996, 2003 - 3D Realms Entertainment
Copyright (C) 2017-2019 Nuke.YKT

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
#include "actors.h"
#include "names_rr.h"
#include "mmulti.h"

BEGIN_DUKE_NS

void dojaildoor();
void moveminecart();

void ballreturn(short spr);
short pinsectorresetdown(short sect);
short pinsectorresetup(short sect);
short checkpins(short sect);
void resetpins(short sect);
void resetlanepics(void);


struct FireProj
{
	int x, y, z;
	int xv, yv, zv;
};

static TMap<int, FireProj> fire;

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

bool ceilingspace_r(int sectnum)
{
	if( (sector[sectnum].ceilingstat&1) && sector[sectnum].ceilingpal == 0 )
	{
		switch(sector[sectnum].ceilingpicnum)
		{
			case MOONSKY1:
			case BIGORBIT1:
				return 1;
		}
	}
	return 0;
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

bool floorspace_r(int sectnum)
{
	if( (sector[sectnum].floorstat&1) && sector[sectnum].ceilingpal == 0 )
	{
		switch(sector[sectnum].floorpicnum)
		{
			case MOONSKY1:
			case BIGORBIT1:
				return 1;
		}
	}
	return 0;
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void addweapon_r(struct player_struct* p, int weapon)
{
	short cw = p->curr_weapon;
	if (p->OnMotorcycle || p->OnBoat)
	{
		p->gotweapon.Set(weapon);
		if (weapon == THROWSAW_WEAPON)
		{
			p->gotweapon.Set(BUZZSAW_WEAPON);
			p->ammo_amount[BUZZSAW_WEAPON] = 1;
		}
		else if (weapon == CROSSBOW_WEAPON)
		{
			p->gotweapon.Set(CHICKEN_WEAPON);
		}
		else if (weapon == SLINGBLADE_WEAPON)
		{
			p->ammo_amount[SLINGBLADE_WEAPON] = 1;
		}
		return;
	}

	if (p->gotweapon[weapon] == 0)
	{
		p->gotweapon.Set(weapon);
		if (weapon == THROWSAW_WEAPON)
		{
			p->gotweapon.Set(BUZZSAW_WEAPON);
			if (isRRRA()) p->ammo_amount[BUZZSAW_WEAPON] = 1;
		}
		if (isRRRA())
		{
			if (weapon == CROSSBOW_WEAPON)
			{
				p->gotweapon.Set(CHICKEN_WEAPON);
			}
			if (weapon == SLINGBLADE_WEAPON)
			{
				p->ammo_amount[SLINGBLADE_WEAPON] = 50;
			}
		}

		if (weapon != DYNAMITE_WEAPON)
			cw = weapon;
	}
	else
		cw = weapon;

	if (weapon == DYNAMITE_WEAPON)
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
	p->curr_weapon = cw;

	switch (weapon)
	{
	case SLINGBLADE_WEAPON:
		if (!isRRRA()) break;
	case KNEE_WEAPON:
 	case DYNAMITE_WEAPON:     
	case TRIPBOMB_WEAPON:
	case HANDREMOTE_WEAPON:
		break;
	case SHOTGUN_WEAPON:      
		spritesound(SHOTGUN_COCK, p->i); 
		break;
	case PISTOL_WEAPON:       
		spritesound(INSERT_CLIP, p->i); 
		break;
	default:      
		spritesound(EJECT_CLIP, p->i); 
		break;
	}
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void hitradius_r(short i, int  r, int  hp1, int  hp2, int  hp3, int  hp4)
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
		if (s->picnum == RPG || ((isRRRA()) && s->picnum == RPG2)) goto SKIPWALLCHECK;
	}

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

SKIPWALLCHECK:

	q = -(24 << 8) + (krand() & ((32 << 8) - 1));

	for (x = 0; x < 7; x++)
	{
		j = headspritestat[statlist[x]];
		while (j >= 0)
		{
			nextj = nextspritestat[j];
			sj = &sprite[j];

			if (x == 0 || x >= 5 || AFLAMABLE(sj->picnum))
			{
				if (sj->cstat & 257)
					if (dist(s, sj) < r)
					{
						if (badguy(sj) && !cansee(sj->x, sj->y, sj->z + q, sj->sectnum, s->x, s->y, s->z + q, s->sectnum))
							continue;
						checkhitsprite(j, i);
					}
			}
			else if (sj->extra >= 0 && sj != s && (badguy(sj) || sj->picnum == QUEBALL || sj->picnum == RRTILE3440 || sj->picnum == STRIPEBALL || (sj->cstat & 257) || sj->picnum == DUKELYINGDEAD))
			{
				if (s->picnum == MORTER && j == s->owner)
				{
					j = nextj;
					continue;
				}
				if ((isRRRA()) && s->picnum == CHEERBOMB && j == s->owner)
				{
					j = nextj;
					continue;
				}

				if (sj->picnum == APLAYER) sj->z -= PHEIGHT;
				d = dist(s, sj);
				if (sj->picnum == APLAYER) sj->z += PHEIGHT;

				if (d < r && cansee(sj->x, sj->y, sj->z - (8 << 8), sj->sectnum, s->x, s->y, s->z - (12 << 8), s->sectnum))
				{
					if ((isRRRA()) && sprite[j].picnum == MINION && sprite[j].pal == 19)
					{
						j = nextj;
						continue;
					}

					hittype[j].ang = getangle(sj->x - s->x, sj->y - s->y);

					if (s->picnum == RPG && sj->extra > 0)
						hittype[j].picnum = RPG;
					else if ((isRRRA()) && s->picnum == RPG2 && sj->extra > 0)
						hittype[j].picnum = RPG;
					else
						hittype[j].picnum = RADIUSEXPLOSION;

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
						(pic != HULK && pic != MAMA && pic != BILLYPLAY && pic != COOTPLAY && pic != MAMACLOUD) :
						(pic != HULK && pic != SBMOVE))
					{
						if (sprite[j].xvel < 0) sprite[j].xvel = 0;
						sprite[j].xvel += (sprite[j].extra << 2);
					}

					if (sj->picnum == STATUEFLASH || sj->picnum == QUEBALL ||
						sj->picnum == STRIPEBALL || sj->picnum == RRTILE3440)
						checkhitsprite(j, i);

					if (sprite[j].picnum != RADIUSEXPLOSION &&
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
			j = nextj;
		}
	}
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

int movesprite_r(short spritenum, int xchange, int ychange, int zchange, unsigned int cliptype)
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
			cd = 192;
			retval = clipmove(&sprite[spritenum].x, &sprite[spritenum].y, &daz, &dasectnum, ((xchange * TICSPERFRAME) << 11), ((ychange * TICSPERFRAME) << 11), cd, (4 << 8), (4 << 8), cliptype);
		}

		if (dasectnum < 0 || (dasectnum >= 0 &&
			hittype[spritenum].actorstayput >= 0 && hittype[spritenum].actorstayput != dasectnum))
		{
			sprite[spritenum].x = oldx;
			sprite[spritenum].y = oldy;
			if (sector[dasectnum].lotag == ST_1_ABOVE_WATER)
				sprite[spritenum].ang = (krand() & 2047);
			else if ((hittype[spritenum].temp_data[0] & 3) == 1)
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
			clipmove(&sprite[spritenum].x, &sprite[spritenum].y, &daz, &dasectnum, ((xchange * TICSPERFRAME) << 11), ((ychange * TICSPERFRAME) << 11), 128L, (4 << 8), (4 << 8), cliptype);
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

void lotsoffeathers_r(spritetype *s, short n)
{
	lotsofstuff(s, n, MONEY);
}


//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void guts_r(spritetype* s, short gtype, short n, short p)
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

	if (badguy(s) && s->pal == 6)
		pal = 6;
	else
	{
		pal = 0;
		if (isRRRA())
		{
			if (s->picnum == MINION && (s->pal == 8 || s->pal == 19)) pal = s->pal;
		}
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
		i = EGS(s->sectnum, s->x + (r5 & 255) - 128, s->y + (r4 & 255) - 128, gutz - (r3 & 8191), gtype, -32, sx >> 1, sy >> 1, a, 48 + (r2 & 31), -512 - (r1 & 2047), ps[p].i, 5);
		if (pal != 0)
			sprite[i].pal = pal;
	}
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void gutsdir_r(spritetype* s, short gtype, short n, short p)
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
// 
//
//---------------------------------------------------------------------------

void movefta_r(void)
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
		j = 0;

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

						if (s->pal == 33 || s->picnum == VIXEN ||  
							(isRRRA() && (isIn(s->picnum, COOT, COOTSTAYPUT, BIKER, BIKERB, BIKERBV2, CHEER, CHEERB,
								CHEERSTAYPUT, MINIONBOAT, HULKBOAT, CHEERBOAT, RABBIT, COOTPLAY, BILLYPLAY, MAKEOUT, MAMA)
								|| (s->picnum == MINION && s->pal == 8)))
							 || (sintable[(s->ang + 512) & 2047] * (px - sx) + sintable[s->ang & 2047] * (py - sy) >= 0))
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


					if (j) switch (s->picnum)
					{
					case RUBBERCAN:
					case EXPLODINGBARREL:
					case WOODENHORSE:
					case HORSEONSIDE:
					case CANWITHSOMETHING:
					case FIREBARREL:
					case FIREVASE:
					case NUKEBARREL:
					case NUKEBARRELDENTED:
					case NUKEBARRELLEAKED:
						if (sector[s->sectnum].ceilingstat & 1)
							s->shade = sector[s->sectnum].ceilingshade;
						else s->shade = sector[s->sectnum].floorshade;

						hittype[i].timetosleep = 0;
						changespritestat(i, STAT_STANDABLE);
						break;
					default:
#if 0
						// TRANSITIONAL: RedNukem has this here. Needed?
						if (A_CheckSpriteFlags(spriteNum, SFLAG_USEACTIVATOR) && sector[sprite[spriteNum].sectnum].lotag & 16384) break;
#endif
						hittype[i].timetosleep = 0;
						check_fta_sounds(i);
						changespritestat(i, STAT_ACTOR);
						break;
					}
					else hittype[i].timetosleep = 0;
				}
			}
			if (/*!j &&*/ badguy(s)) // this is like RedneckGDX. j is uninitialized here, i.e. most likely not 0.
			{
				if (sector[s->sectnum].ceilingstat & 1)
					s->shade = sector[s->sectnum].ceilingshade;
				else s->shade = sector[s->sectnum].floorshade;

				if (s->picnum != HEN || s->picnum != COW || s->picnum != PIG || s->picnum != DOGRUN || ((isRRRA()) && s->picnum != RABBIT))
				{
					if (wakeup(i, p))
					{
						hittype[i].timetosleep = 0;
						check_fta_sounds(i);
						changespritestat(i, STAT_ACTOR);
					}
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

int ifhitsectors_r(int sectnum)
{
	int i = headspritestat[STAT_MISC];
	while (i >= 0)
	{
		if (sprite[i].picnum == EXPLOSION2 || (sprite[i].picnum == EXPLOSION3 && sectnum == sprite[i].sectnum))
			return i;
		i = nextspritestat[i];
	}
	return -1;
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

int ifhitbyweapon_r(int sn)
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
				if (ud.god) return -1;

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
					if (npc->extra <= 0 && hittype[sn].picnum != FREEZEBLAST)
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

				int pn = hittype[sn].picnum;
				if (pn == RPG2 && isRRRA()) pn = 0; // avoid messing around with gotos.
				switch (pn)
				{
				case RADIUSEXPLOSION:
				case RPG:
				case HYDRENT:
				case HEAVYHBOMB:
				case SEENINE:
				case OOZFILTER:
				case EXPLODINGBARREL:
				case TRIPBOMBSPRITE:
				case RPG2:
					ps[p].posxv +=
						hittype[sn].extra * (sintable[(hittype[sn].ang + 512) & 2047]) << 2;
					ps[p].posyv +=
						hittype[sn].extra * (sintable[hittype[sn].ang & 2047]) << 2;
					break;
				default:
					ps[p].posxv +=
						hittype[sn].extra * (sintable[(hittype[sn].ang + 512) & 2047]) << 1;
					ps[p].posyv +=
						hittype[sn].extra * (sintable[hittype[sn].ang & 2047]) << 1;
					break;
				}
			}
			else
			{
				if (hittype[sn].extra == 0)
					if (npc->xrepeat < 24)
						return -1;

				npc->extra -= hittype[sn].extra;
				if (npc->picnum != RECON && npc->owner >= 0 && sprite[npc->owner].statnum < MAXSTATUS)
					npc->owner = hittype[sn].owner;
			}

			hittype[sn].extra = -1;
			return hittype[sn].picnum;
		}
	}

	hittype[sn].extra = -1;
	return -1;
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void respawn_rrra(int i, int j)
{
	sprite[j].pal = sprite[i].pal;
	if (sprite[j].picnum == MAMA)
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
		return;
	}
	sprite[i].extra = (66 - 13);
	sprite[j].pal = 0;
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void movefallers_r(void)
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
			j = ifhitbyweapon_r(i);
			if (j >= 0) 
			{
				if (j == RPG || (isRRRA() && j == RPG2) || j == RADIUSEXPLOSION || j == SEENINE || j == OOZFILTER)
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
								if (sprite[j].picnum == CEILINGSTEAM || sprite[j].picnum == STEAM)
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
				s->xvel = ((64 + krand()) & 127);
				s->zvel = -(1024 + (krand() & 1023));
			}
			else
			{
				if (s->xvel > 0)
				{
					s->xvel -= 2;
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
		int j = ifhitbyweapon_r(i);
		if (j == RPG || (isRRRA() && j == RPG2) || j == RADIUSEXPLOSION || j == SEENINE || j == OOZFILTER)
		{
			j = headspritestat[STAT_STANDABLE];
			while (j >= 0)
			{
				if (s->hitag == sprite[j].hitag && (sprite[j].picnum == OOZFILTER || sprite[j].picnum == SEENINE))
					if (sprite[j].shade != -32)
						sprite[j].shade = -32;
				j = nextspritestat[j];
			}
			detonate(i, EXPLOSION2);
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

	if (s->picnum == (BOLT1 + 1) && (krand() & 1) && sector[sect].floorpicnum == HURTRAIL)
		spritesound(SHORT_CIRCUIT, i);

	if (s->picnum == BOLT1 + 4) s->picnum = BOLT1;

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
// this has been broken up into lots of smaller subfunctions
//
//---------------------------------------------------------------------------

void movestandables_r(void)
{
	int nexti;
	
	for (int i = headspritestat[STAT_STANDABLE]; i >= 0; i = nexti)
	{
		nexti = nextspritestat[i];

		auto s = &sprite[i];
		int picnum = s->picnum;

		if (s->sectnum < 0)
		{
			deletesprite(i);
			continue;
		}

		hittype[i].bposx = s->x;
		hittype[i].bposy = s->y;
		hittype[i].bposz = s->z;


		if (picnum >= CRANE && picnum <= CRANE +3)
		{
			movecrane(i, CRANE);
		}

		else if (picnum >= WATERFOUNTAIN && picnum <= WATERFOUNTAIN + 3)
		{
			movefountain(i, WATERFOUNTAIN);
		}

		else if (AFLAMABLE(picnum))
		{
			moveflammable(i, TIRE, BOX, BLOODPOOL);
		}


		else if (picnum >= CRACK1 && picnum <= CRACK1 + 3)
		{
			movecrack(i);
		}

		else if (picnum == OOZFILTER || picnum == SEENINE || picnum == SEENINEDEAD || picnum == (SEENINEDEAD + 1))
		{
			moveooz(i, SEENINE, SEENINEDEAD, OOZFILTER, EXPLOSION2);
		}

		else if (picnum == MASTERSWITCH)
		{
			movemasterswitch(i, SEENINE, OOZFILTER);
		}

		else if (picnum == TRASH)
		{
			movetrash(i);
		}

		else if (picnum >= BOLT1 && picnum <= BOLT1 + 3)
		{
			movebolt(i);
		}

		else if (picnum == WATERDRIP)
		{
			movewaterdrip(i, WATERDRIP);
		}

		else if (picnum == DOORSHOCK)
		{
			movedoorshock(i);
		}

		else if (picnum == TOUCHPLATE)
		{
			movetouchplate(i, TOUCHPLATE);
		}

		else if (picnum == CANWITHSOMETHING)
		{
			movecanwithsomething(i);
		}

		else if (isIn(picnum,
				EXPLODINGBARREL,
				WOODENHORSE,
				HORSEONSIDE,
				FIREBARREL,
				FIREVASE,
				NUKEBARREL,
				NUKEBARRELDENTED,
				NUKEBARRELLEAKED,
				TOILETWATER,
				RUBBERCAN,
				STEAM,
				CEILINGSTEAM))
		{
			int x;
			int p = findplayer(s, &x);
			execute(i, p, x);
		}
	}
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void moveweapons_r(void)
{
	int j, k, nexti, p;
	int dax, day, daz, x, ll;
	unsigned int qq;
	spritetype* s;

	for (int i = headspritestat[STAT_PROJECTILE];  i >= 0; i = nexti)
	{
		nexti = nextspritestat[i];
		s = &sprite[i];

		if (s->sectnum < 0)
		{
			deletesprite(i);
			continue;
		}

		hittype[i].bposx = s->x;
		hittype[i].bposy = s->y;
		hittype[i].bposz = s->z;

		switch (s->picnum)
		{
		case RADIUSEXPLOSION:
			deletesprite(i);
			continue;
		case TONGUE:
			movetongue(i, TONGUE, INNERJAW);
			continue;

		case FREEZEBLAST:
			if (s->yvel < 1 || s->extra < 2 || (s->xvel | s->zvel) == 0)
			{
				j = spawn(i, TRANSPORTERSTAR);
				sprite[j].pal = 1;
				sprite[j].xrepeat = 32;
				sprite[j].yrepeat = 32;
				deletesprite(i);
				continue;
			}
		case RPG2:
		case RRTILE1790:
			if (!isRRRA()) continue;
		case SHRINKSPARK:
		case RPG:
		case FIRELASER:
		case SPIT:
		case COOLEXPLOSION1:
		case OWHIP:
		case UWHIP:
			p = -1;

			if (s->picnum == RPG && sector[s->sectnum].lotag == 2)
			{
				k = s->xvel >> 1;
				ll = s->zvel >> 1;
			}
			else if (isRRRA() && s->picnum == RPG2 && sector[s->sectnum].lotag == 2)
			{
				k = s->xvel >> 1;
				ll = s->zvel >> 1;
			}
			else
			{
				k = s->xvel;
				ll = s->zvel;
			}

			dax = s->x; day = s->y; daz = s->z;

			getglobalz(i);
			qq = CLIPMASK1;

			switch (s->picnum)
			{
			case RPG:
				if (hittype[i].picnum != BOSS2 && s->xrepeat >= 10 && sector[s->sectnum].lotag != 2)
				{
					j = spawn(i, SMALLSMOKE);
					sprite[j].z += (1 << 8);
				}
				break;
			case RPG2:
				if (!isRRRA()) break;
				s->hitag++;
				if (hittype[i].picnum != BOSS2 && s->xrepeat >= 10 && sector[s->sectnum].lotag != 2)
				{
					j = spawn(i, SMALLSMOKE);
					sprite[j].z += (1 << 8);
					if ((krand() & 15) == 2)
					{
						j = spawn(i, 1310);
					}
				}
				if (sprite[s->lotag].extra <= 0)
					s->lotag = 0;
				if (s->lotag != 0 && s->hitag > 5)
				{
					spritetype* ts;
					int ang, ang2, ang3;
					ts = &sprite[s->lotag];
					ang = getangle(ts->x - s->x, ts->y - s->y);
					ang2 = ang - s->ang;
					ang3 = abs(ang2);
					if (ang2 < 100)
					{
						if (ang3 > 1023)
							s->ang += 51;
						else
							s->ang -= 51;
					}
					else if (ang2 > 100)
					{
						if (ang3 > 1023)
							s->ang -= 51;
						else
							s->ang += 51;
					}
					else
						s->ang = ang;

					if (s->hitag > 180)
						if (s->zvel <= 0)
							s->zvel += 200;
				}
				break;
			case RRTILE1790:
				if (!isRRRA()) break;
				if (s->extra)
				{
					s->zvel = -(s->extra * 250);
					s->extra--;
				}
				else
					makeitfall(i);
				if (s->xrepeat >= 10 && sector[s->sectnum].lotag != 2)
				{
					j = spawn(i, SMALLSMOKE);
					sprite[j].z += (1 << 8);
				}
				break;
			}

			j = movesprite_r(i,
				(k * (sintable[(s->ang + 512) & 2047])) >> 14,
				(k * (sintable[s->ang & 2047])) >> 14, ll, qq);

			if ((s->picnum == RPG || (isRRRA() && isIn(s->picnum, RPG2, RRTILE1790))) && s->yvel >= 0)
				if (FindDistance2D(s->x - sprite[s->yvel].x, s->y - sprite[s->yvel].y) < 256)
					j = 49152 | s->yvel;

			if (s->sectnum < 0) // || (isRR() && sector[s->sectnum].filler == 800))
			{
				deletesprite(i);
				continue;
			}

			if ((j&49152) != 49152 && s->picnum != FREEZEBLAST)
			{
				if (s->z < hittype[i].ceilingz)
				{
					j = 16384|(s->sectnum);
					s->zvel = -1;
				}
				else
					if (s->z > hittype[i].floorz)
					{
						j = 16384 | (s->sectnum);
						if (sector[s->sectnum].lotag != 1)
							s->zvel = 1;
					}
			}

			if (s->picnum == FIRELASER)
			{
				for (k = -3; k < 2; k++)
				{
					x = EGS(s->sectnum,
						s->x + ((k * sintable[(s->ang + 512) & 2047]) >> 9),
						s->y + ((k * sintable[s->ang & 2047]) >> 9),
						s->z + ((k * ksgn(s->zvel)) * abs(s->zvel / 24)), FIRELASER, -40 + (k << 2),
						s->xrepeat, s->yrepeat, 0, 0, 0, s->owner, 5);

					sprite[x].cstat = 128;
					sprite[x].pal = s->pal;
				}
			}
			else if (s->picnum == SPIT) if (s->zvel < 6144)
				s->zvel += gc - 112;

			if (j != 0)
			{

				//if ((j & kHitTypeMask) == kHitSprite) j &= kHitIndexMask; reminder for later.
				if ((j & 49152) == 49152)
				{
					j &= (MAXSPRITES - 1);

					if (isRRRA())
					{
						if (sprite[j].picnum == MINION
							&& (s->picnum == RPG || s->picnum == RPG2)
							&& sprite[j].pal == 19)
						{
							spritesound(RPG_EXPLODE, i);
							k = spawn(i, EXPLOSION2);
							sprite[k].x = dax;
							sprite[k].y = day;
							sprite[k].z = daz;
							continue;
						}
					}
					else if (s->picnum == FREEZEBLAST && sprite[j].pal == 1)
						if (badguy(&sprite[j]) || sprite[j].picnum == APLAYER)
					{
						j = spawn(i,TRANSPORTERSTAR);
						sprite[j].pal = 1;
						sprite[j].xrepeat = 32;
						sprite[j].yrepeat = 32;

						deletesprite(i);
						continue;
					}

					checkhitsprite(j,i);

					if (sprite[j].picnum == APLAYER)
					{
						p = sprite[j].yvel;
						spritesound(PISTOL_BODYHIT,j);

						if (s->picnum == SPIT)
						{
							if (isRRRA() && sprite[s->owner].picnum == MAMA)
							{
								guts_r(s, RABBITJIBA, 2, myconnectindex);
								guts_r(s, RABBITJIBB, 2, myconnectindex);
								guts_r(s, RABBITJIBC, 2, myconnectindex);
							}
							ps[p].addhoriz(32);
							ps[p].return_to_center = 8;

							if (ps[p].loogcnt == 0)
							{
								if (!A_CheckSoundPlaying(ps[p].i, DUKE_LONGTERM_PAIN))
									A_PlaySound(DUKE_LONGTERM_PAIN, ps[p].i);

								j = 3 + (krand() & 3);
								ps[p].numloogs = j;
								ps[p].loogcnt = 24 * 4;
								for (x = 0; x < j; x++)
								{
									ps[p].loogiex[x] = krand() % xdim;
									ps[p].loogiey[x] = krand() % ydim;
								}
							}
						}
					}
				}
				else if ((j & 49152) == 32768)
				{
					j &= (MAXWALLS - 1);

					if (isRRRA() && sprite[s->owner].picnum == MAMA)
					{
						guts_r(s, RABBITJIBA, 2, myconnectindex);
						guts_r(s, RABBITJIBB, 2, myconnectindex);
						guts_r(s, RABBITJIBC, 2, myconnectindex);
					}

					if (s->picnum != RPG && (!isRRRA() || s->picnum != RPG2) && s->picnum != FREEZEBLAST && s->picnum != SPIT && s->picnum != SHRINKSPARK && (wall[j].overpicnum == MIRROR || wall[j].picnum == MIRROR))
					{
						k = getangle(
							wall[wall[j].point2].x - wall[j].x,
							wall[wall[j].point2].y - wall[j].y);
						s->ang = ((k << 1) - s->ang) & 2047;
						s->owner = i;
						spawn(i, TRANSPORTERSTAR);
						continue;
					}
					else
					{
						setsprite(i, dax, day, daz);
						checkhitwall(i, j, s->x, s->y, s->z, s->picnum);

						if (!isRRRA() && s->picnum == FREEZEBLAST)
						{
							if (wall[j].overpicnum != MIRROR && wall[j].picnum != MIRROR)
							{
								s->extra >>= 1;
								if (s->xrepeat > 8)
									s->xrepeat -= 2;
								if (s->yrepeat > 8)
									s->yrepeat -= 2;
								s->yvel--;
							}

							k = getangle(
								wall[wall[j].point2].x - wall[j].x,
								wall[wall[j].point2].y - wall[j].y);
							s->ang = ((k << 1) - s->ang) & 2047;
							continue;
						}
						if (s->picnum == SHRINKSPARK)
						{
							if (wall[j].picnum >= RRTILE3643 && wall[j].picnum < RRTILE3643 + 3) 
							{
								deletesprite(i);
							}
							if (s->extra <= 0)
							{
								s->x += sintable[(s->ang + 512) & 2047] >> 7;
								s->y += sintable[s->ang & 2047] >> 7;
								if (!isRRRA() || (sprite[s->owner].picnum != CHEER && sprite[s->owner].picnum != CHEERSTAYPUT))
								{
									j = spawn(i, CIRCLESTUCK);
									sprite[j].xrepeat = 8;
									sprite[j].yrepeat = 8;
									sprite[j].cstat = 16;
									sprite[j].ang = (sprite[j].ang + 512) & 2047;
									sprite[j].clipdist = mulscale7(s->xrepeat, tilesiz[s->picnum].x);
								}
								deletesprite(i);
								continue;
							}
							if (wall[j].overpicnum != MIRROR && wall[j].picnum != MIRROR)
							{
								s->extra -= 20;
								s->yvel--;
							}

							k = getangle(
								wall[wall[j].point2].x - wall[j].x,
								wall[wall[j].point2].y - wall[j].y);
							s->ang = ((k << 1) - s->ang) & 2047;
							continue;
						}
					}
				}
				else if ((j & 49152) == 16384)
				{
					setsprite(i, dax, day, daz);

					if (isRRRA() && sprite[s->owner].picnum == MAMA)
					{
						guts_r(s, RABBITJIBA, 2, myconnectindex);
						guts_r(s, RABBITJIBB, 2, myconnectindex);
						guts_r(s, RABBITJIBC, 2, myconnectindex);
					}

					if (s->zvel < 0)
					{
						if (sector[s->sectnum].ceilingstat & 1)
							if (sector[s->sectnum].ceilingpal == 0)
							{
								deletesprite(i);
								continue;
							}

						checkhitceiling(s->sectnum);
					}

					if (!isRRRA() && s->picnum == FREEZEBLAST)
					{
						bounce(i);
						ssp(i, qq);
						s->extra >>= 1;
						if (s->xrepeat > 8)
							s->xrepeat -= 2;
						if (s->yrepeat > 8)
							s->yrepeat -= 2;
						s->yvel--;
						continue;
					}
				}

				if (s->picnum != SPIT)
				{
					if (s->picnum == RPG)
					{
						k = spawn(i, EXPLOSION2);
						sprite[k].x = dax;
						sprite[k].y = day;
						sprite[k].z = daz;

						if (s->xrepeat < 10)
						{
							sprite[k].xrepeat = 6;
							sprite[k].yrepeat = 6;
						}
						else if ((j & 49152) == 16384)
						{
							sprite[k].cstat |= 8;
							sprite[k].z += (48 << 8);
						}
					}
					else if (isRRRA() && s->picnum == RPG2)
					{
						k = spawn(i, EXPLOSION2);
						sprite[k].x = dax;
						sprite[k].y = day;
						sprite[k].z = daz;

						if (s->xrepeat < 10)
						{
							sprite[k].xrepeat = 6;
							sprite[k].yrepeat = 6;
						}
						else if ((j & 49152) == 16384)
						{
							sprite[k].cstat |= 8;
							sprite[k].z += (48 << 8);
						}
					}
					else if (isRRRA() && s->picnum == RRTILE1790)
					{
						s->extra = 160;
						k = spawn(i, EXPLOSION2);
						sprite[k].x = dax;
						sprite[k].y = day;
						sprite[k].z = daz;

						if (s->xrepeat < 10)
						{
							sprite[k].xrepeat = 6;
							sprite[k].yrepeat = 6;
						}
						else if ((j & 49152) == 16384)
						{
							sprite[k].cstat |= 8;
							sprite[k].z += (48 << 8);
						}
					}
					else if (s->picnum != FREEZEBLAST && s->picnum != FIRELASER && s->picnum != SHRINKSPARK)
					{
						k = spawn(i, 1441);
						sprite[k].xrepeat = sprite[k].yrepeat = s->xrepeat >> 1;
						if ((j & 49152) == 16384)
						{
							if (s->zvel < 0)
							{
								sprite[k].cstat |= 8; sprite[k].z += (72 << 8);
							}
						}
					}
					if (s->picnum == RPG)
					{
						spritesound(RPG_EXPLODE, i);

						if (s->xrepeat >= 10)
						{
							x = s->extra;
							hitradius(i, rpgblastradius, x >> 2, x >> 1, x - (x >> 2), x);
						}
						else
						{
							x = s->extra + (global_random & 3);
							hitradius(i, (rpgblastradius >> 1), x >> 2, x >> 1, x - (x >> 2), x);
						}
					}
					else if (isRRRA() && s->picnum == RPG2)
					{
						s->extra = 150;
						spritesound(247, i);

						if (s->xrepeat >= 10)
						{
							x = s->extra;
							hitradius(i, rpgblastradius, x >> 2, x >> 1, x - (x >> 2), x);
						}
						else
						{
							x = s->extra + (global_random & 3);
							hitradius(i, (rpgblastradius >> 1), x >> 2, x >> 1, x - (x >> 2), x);
						}
					}
					else if (isRRRA() && s->picnum == RRTILE1790)
					{
						s->extra = 160;
						spritesound(RPG_EXPLODE, i);

						if (s->xrepeat >= 10)
						{
							x = s->extra;
							hitradius(i, rpgblastradius, x >> 2, x >> 1, x - (x >> 2), x);
						}
						else
						{
							x = s->extra + (global_random & 3);
							hitradius(i, (rpgblastradius >> 1), x >> 2, x >> 1, x - (x >> 2), x);
						}
					}
				}
				deletesprite(i);
				continue;
			}
			if ((s->picnum == RPG || (isRRRA() && s->picnum == RPG2)) && sector[s->sectnum].lotag == 2 && s->xrepeat >= 10 && rnd(184))
				spawn(i, WATERBUBBLE);

			continue;


		case SHOTSPARK1:
			p = findplayer(s, &x);
			execute(i, p, x);
			continue;
		}
	}
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void movetransports_r(void)
{
	char warpdir, warpspriteto;
	short i, j, k, p, sect, sectlotag, nexti, nextj;
	long ll2, ll, onfloorz;

	i = headspritestat[STAT_TRANSPORT]; //Transporters

	while (i >= 0)
	{
		sect = sprite[i].sectnum;
		sectlotag = sector[sect].lotag;

		nexti = nextspritestat[i];

		auto& OW = sprite[i].owner;
		auto PN = sprite[i].picnum;
		if (OW == i)
		{
			i = nexti;
			continue;
		}

		onfloorz = hittype[i].temp_data[4];

		if (hittype[i].temp_data[0] > 0) hittype[i].temp_data[0]--;

		j = headspritesect[sect];
		while (j >= 0)
		{
			nextj = nextspritesect[j];

			switch (sprite[j].statnum)
			{
			case STAT_PLAYER:    // Player

				if (sprite[j].owner != -1)
				{
					p = sprite[j].yvel;

					ps[p].on_warping_sector = 1;

					if (ps[p].transporter_hold == 0 && ps[p].jumping_counter == 0)
					{
						if (ps[p].on_ground && sectlotag == 0 && onfloorz && ps[p].jetpack_on == 0)
						{
							spawn(i, TRANSPORTERBEAM);
							spritesound(TELEPORTER, i);

							for (k = connecthead; k >= 0; k = connectpoint2[k])// connectpoinhittype[i].temp_data[1][k])
								if (ps[k].cursectnum == sprite[OW].sectnum)
								{
									ps[k].frag_ps = p;
									sprite[ps[k].i].extra = 0;
								}

							ps[p].setang(sprite[OW].ang);

							if (sprite[OW].owner != OW)
							{
								hittype[i].temp_data[0] = 13;
								hittype[OW].temp_data[0] = 13;
								ps[p].transporter_hold = 13;
							}

							ps[p].bobposx = ps[p].oposx = ps[p].posx = sprite[OW].x;
							ps[p].bobposy = ps[p].oposy = ps[p].posy = sprite[OW].y;
							ps[p].oposz = ps[p].posz = sprite[OW].z - (PHEIGHT - (4 << 8));

							changespritesect(j, sprite[OW].sectnum);
							ps[p].cursectnum = sprite[j].sectnum;

							k = spawn(OW, TRANSPORTERBEAM);
							spritesound(TELEPORTER, k);

							break;
						}
					}
					else break;

					if (onfloorz == 0 && abs(sprite[i].z - ps[p].posz) < 6144)
						if ((ps[p].jetpack_on == 0) || (ps[p].jetpack_on && PlayerInput(p, SK_JUMP)) ||
							(ps[p].jetpack_on && PlayerInput(p, SK_CROUCH)))
						{
							ps[p].oposx = ps[p].posx += sprite[OW].x - sprite[i].x;
							ps[p].oposy = ps[p].posy += sprite[OW].y - sprite[i].y;

							if (ps[p].jetpack_on && (PlayerInput(p, SK_JUMP) || ps[p].jetpack_on < 11))
								ps[p].posz = sprite[OW].z - 6144;
							else ps[p].posz = sprite[OW].z + 6144;
							ps[p].oposz = ps[p].posz;

							changespritesect(j, sprite[OW].sectnum);
							ps[p].cursectnum = sprite[OW].sectnum;

							break;
						}

					k = 0;

					if (isRRRA())
					{
						if (onfloorz && sectlotag == 160 && ps[p].posz > (sector[sect].floorz - (48 << 8)))
						{
							k = 2;
							ps[p].oposz = ps[p].posz =
								sector[sprite[OW].sectnum].ceilingz + (7 << 8);
						}

						if (onfloorz && sectlotag == 161 && ps[p].posz < (sector[sect].ceilingz + (6 << 8)))
						{
							k = 2;
							if (sprite[ps[p].i].extra <= 0) break;
							ps[p].oposz = ps[p].posz =
								sector[sprite[OW].sectnum].floorz - (49 << 8);
						}
					}

					if ((onfloorz && sectlotag == ST_1_ABOVE_WATER && ps[p].posz > (sector[sect].floorz - (6 << 8))) ||
						(onfloorz && sectlotag == ST_1_ABOVE_WATER && ps[p].OnMotorcycle))
					{
						if (ps[p].OnBoat) break;
						k = 1;
						if (screenpeek == p)
						{
							FX_StopAllSounds();
						}
						spritesound(DUKE_UNDERWATER, ps[p].i);
						ps[p].oposz = ps[p].posz =
							sector[sprite[OW].sectnum].ceilingz + (7 << 8);
						if (ps[p].OnMotorcycle)
							ps[p].moto_underwater = 1;
					}

					if (onfloorz && sectlotag == ST_2_UNDERWATER && ps[p].posz < (sector[sect].ceilingz + (6 << 8)))
					{
						k = 1;
						if (sprite[ps[p].i].extra <= 0) break;
						if (screenpeek == p)
						{
							FX_StopAllSounds();
						}
						spritesound(DUKE_GASP, ps[p].i);

						ps[p].oposz = ps[p].posz =
							sector[sprite[OW].sectnum].floorz - (7 << 8);
					}

					if (k == 1)
					{
						ps[p].oposx = ps[p].posx += sprite[OW].x - sprite[i].x;
						ps[p].oposy = ps[p].posy += sprite[OW].y - sprite[i].y;

						if (sprite[OW].owner != OW)
							ps[p].transporter_hold = -2;
						ps[p].cursectnum = sprite[OW].sectnum;

						changespritesect(j, sprite[OW].sectnum);

						setpal(&ps[p]);

						if ((krand() & 255) < 32)
							spawn(ps[p].i, WATERSPLASH2);
					}
					else if (isRRRA() && k == 2)
					{
						ps[p].oposx = ps[p].posx += sprite[OW].x - sprite[i].x;
						ps[p].oposy = ps[p].posy += sprite[OW].y - sprite[i].y;

						if (sprite[OW].owner != OW)
							ps[p].transporter_hold = -2;
						ps[p].cursectnum = sprite[OW].sectnum;

						changespritesect(j, sprite[OW].sectnum);
					}
				}
				break;

			case STAT_ACTOR:
				if (PN == SHARK ||
					(isRRRA() && (PN == CHEERBOAT || PN == HULKBOAT || PN == MINIONBOAT || PN == UFO1_RRRA)) ||
					(!isRRRA() && (PN == UFO1_RR || PN == UFO2 || PN == UFO3 || PN == UFO4 || PN == UFO5))) goto JBOLT;
			case STAT_PROJECTILE:
			case STAT_MISC:
			case STAT_DUMMYPLAYER:

				ll = abs(sprite[j].zvel);
				if (isRRRA())
				{
					if (sprite[j].zvel >= 0)
						warpdir = 2;
					else
						warpdir = 1;
				}

				{
					warpspriteto = 0;
					if (ll && sectlotag == ST_2_UNDERWATER && sprite[j].z < (sector[sect].ceilingz + ll))
						warpspriteto = 1;

					if (ll && sectlotag == ST_1_ABOVE_WATER && sprite[j].z > (sector[sect].floorz - ll))
						if (!isRRRA() || (sprite[j].picnum != CHEERBOAT && sprite[j].picnum != HULKBOAT && sprite[j].picnum != MINIONBOAT))
							warpspriteto = 1;

					if (isRRRA())
					{
						if (ll && sectlotag == 161 && sprite[j].z < (sector[sect].ceilingz + ll) && warpdir == 1)
						{
							warpspriteto = 1;
							ll2 = ll - abs(sprite[j].z - sector[sect].ceilingz);
						}
						else if (sectlotag == 161 && sprite[j].z < (sector[sect].ceilingz + 1000) && warpdir == 1)
						{
							warpspriteto = 1;
							ll2 = 1;
						}
						if (ll && sectlotag == 160 && sprite[j].z > (sector[sect].floorz - ll) && warpdir == 2)
						{
							warpspriteto = 1;
							ll2 = ll - abs(sector[sect].floorz - sprite[j].z);
						}
						else if (sectlotag == 160 && sprite[j].z > (sector[sect].floorz - 1000) && warpdir == 2)
						{
							warpspriteto = 1;
							ll2 = 1;
						}
					}

					if (sectlotag == 0 && (onfloorz || abs(sprite[j].z - sprite[i].z) < 4096))
					{
						if (sprite[OW].owner != OW && onfloorz && hittype[i].temp_data[0] > 0 && sprite[j].statnum != 5)
						{
							hittype[i].temp_data[0]++;
							continue;
						}
						warpspriteto = 1;
					}

					if (warpspriteto) switch (sprite[j].picnum)
					{
					case TRANSPORTERSTAR:
					case TRANSPORTERBEAM:
					case BULLETHOLE:
					case WATERSPLASH2:
					case BURNING:
					case FIRE:
					case MUD:
						goto JBOLT;
					case PLAYERONWATER:
						if (sectlotag == ST_2_UNDERWATER)
						{
							sprite[j].cstat &= 32767;
							break;
						}
					default:
						if (sprite[j].statnum == 5 && !(sectlotag == ST_1_ABOVE_WATER || sectlotag == ST_2_UNDERWATER || (isRRRA() && (sectlotag == 160 || sectlotag == 161))))
							break;

					case WATERBUBBLE:
						if (rnd(192) && sprite[j].picnum == WATERBUBBLE)
							break;

						if (sectlotag > 0)
						{
							k = spawn(j, WATERSPLASH2);
							if (sectlotag == 1 && sprite[j].statnum == 4)
							{
								sprite[k].xvel = sprite[j].xvel >> 1;
								sprite[k].ang = sprite[j].ang;
								ssp(k, CLIPMASK0);
							}
						}

						switch (sectlotag)
						{
						case ST_0_NO_EFFECT:
							if (onfloorz)
							{
								if (checkcursectnums(sect) == -1 && checkcursectnums(sprite[OW].sectnum) == -1)
								{
									sprite[j].x += (sprite[OW].x - sprite[i].x);
									sprite[j].y += (sprite[OW].y - sprite[i].y);
									sprite[j].z -= sprite[i].z - sector[sprite[OW].sectnum].floorz;
									sprite[j].ang = sprite[OW].ang;

									hittype[j].bposx = sprite[j].x;
									hittype[j].bposy = sprite[j].y;
									hittype[j].bposz = sprite[j].z;

									k = spawn(i, TRANSPORTERBEAM);
									spritesound(TELEPORTER, k);

									k = spawn(OW, TRANSPORTERBEAM);
									spritesound(TELEPORTER, k);

									if (sprite[OW].owner != OW)
									{
										hittype[i].temp_data[0] = 13;
										hittype[OW].temp_data[0] = 13;
									}

									changespritesect(j, sprite[OW].sectnum);
								}
							}
							else
							{
								sprite[j].x += (sprite[OW].x - sprite[i].x);
								sprite[j].y += (sprite[OW].y - sprite[i].y);
								sprite[j].z = sprite[OW].z + 4096;

								hittype[j].bposx = sprite[j].x;
								hittype[j].bposy = sprite[j].y;
								hittype[j].bposz = sprite[j].z;

								changespritesect(j, sprite[OW].sectnum);
							}
							break;
						case ST_1_ABOVE_WATER:
							sprite[j].x += (sprite[OW].x - sprite[i].x);
							sprite[j].y += (sprite[OW].y - sprite[i].y);
							sprite[j].z = sector[sprite[OW].sectnum].ceilingz + ll;

							hittype[j].bposx = sprite[j].x;
							hittype[j].bposy = sprite[j].y;
							hittype[j].bposz = sprite[j].z;

							changespritesect(j, sprite[OW].sectnum);

							break;
						case ST_2_UNDERWATER:
							sprite[j].x += (sprite[OW].x - sprite[i].x);
							sprite[j].y += (sprite[OW].y - sprite[i].y);
							sprite[j].z = sector[sprite[OW].sectnum].floorz - ll;

							hittype[j].bposx = sprite[j].x;
							hittype[j].bposy = sprite[j].y;
							hittype[j].bposz = sprite[j].z;

							changespritesect(j, sprite[OW].sectnum);

							break;

						case 160:
							if (!isRRRA()) break;
							sprite[j].x += (sprite[OW].x - sprite[i].x);
							sprite[j].y += (sprite[OW].y - sprite[i].y);
							sprite[j].z = sector[sprite[OW].sectnum].ceilingz + ll2;

							hittype[j].bposx = sprite[j].x;
							hittype[j].bposy = sprite[j].y;
							hittype[j].bposz = sprite[j].z;

							changespritesect(j, sprite[OW].sectnum);

							movesprite(j, (sprite[j].xvel * sintable[(sprite[j].ang + 512) & 2047]) >> 14,
								(sprite[j].xvel * sintable[sprite[j].ang & 2047]) >> 14, 0, CLIPMASK1);

							break;
						case 161:
							if (!isRRRA()) break;
							sprite[j].x += (sprite[OW].x - sprite[i].x);
							sprite[j].y += (sprite[OW].y - sprite[i].y);
							sprite[j].z = sector[sprite[OW].sectnum].floorz - ll2;

							hittype[j].bposx = sprite[j].x;
							hittype[j].bposy = sprite[j].y;
							hittype[j].bposz = sprite[j].z;

							changespritesect(j, sprite[OW].sectnum);

							movesprite(j, (sprite[j].xvel * sintable[(sprite[j].ang + 512) & 2047]) >> 14,
								(sprite[j].xvel * sintable[sprite[j].ang & 2047]) >> 14, 0, CLIPMASK1);

							break;
						}

						break;
					}
				}
				break;

			}
		JBOLT:
			j = nextj;
		}
		i = nexti;
	}
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

static void rrra_specialstats()
{
	int i, j, nexti;

	i = headspritestat[117];
	while (i >= 0)
	{
		nexti = nextspritestat[i];
		if (sprite[i].hitag > 2)
			sprite[i].hitag = 0;
		if ((sprite[i].picnum == RRTILE8488 || sprite[i].picnum == RRTILE8490) && sprite[i].hitag != 2)
		{
			sprite[i].hitag = 2;
			sprite[i].extra = -100;
		}
		if (sprite[i].hitag == 0)
		{
			sprite[i].extra++;
			if (sprite[i].extra >= 30)
				sprite[i].hitag = 1;
		}
		else if (sprite[i].hitag == 1)
		{
			sprite[i].extra--;
			if (sprite[i].extra <= -30)
				sprite[i].hitag = 0;
		}
		else if (sprite[i].hitag == 2)
		{
			sprite[i].extra--;
			if (sprite[i].extra <= -104)
			{
				spawn(i, sprite[i].lotag);
				deletesprite(i);
			}
		}
		j = movesprite(i, 0, 0, sprite[i].extra * 2, CLIPMASK0);
		i = nexti;
	}

	i = headspritestat[118];
	while (i >= 0)
	{
		nexti = nextspritestat[i];
		if (sprite[i].hitag > 1)
			sprite[i].hitag = 0;
		if (sprite[i].hitag == 0)
		{
			sprite[i].extra++;
			if (sprite[i].extra >= 20)
				sprite[i].hitag = 1;
		}
		else if (sprite[i].hitag == 1)
		{
			sprite[i].extra--;
			if (sprite[i].extra <= -20)
				sprite[i].hitag = 0;
		}
		j = movesprite(i, 0, 0, sprite[i].extra, CLIPMASK0);
		i = nexti;
	}

	if (ps[screenpeek].raat609 > 0)
	{
		ps[screenpeek].raat609--;
		if (ps[screenpeek].raat609 == 0)
		{
			ps[screenpeek].gm = MODE_EOL;
			ud.eog = 1;
			ud.level_number++;
			if (ud.level_number > 6)
				ud.level_number = 0;
			ud.m_level_number = ud.level_number;
		}
	}

	if (enemysizecheat > 0)
	{
		short ti;
		for (ti = 0; ti < MAXSPRITES; ti++)
		{
			switch (sprite[ti].picnum)
			{
				//case 4049:
				//case 4050:
			case BILLYCOCK:
			case BILLYRAY:
			case BILLYRAYSTAYPUT:
			case BRAYSNIPER:
			case DOGRUN:
			case LTH:
			case HULKJUMP:
			case HULK:
			case HULKSTAYPUT:
			case HEN:
			case DRONE:
			case PIG:
			case MINION:
			case MINIONSTAYPUT:
			case UFO1_RRRA:
			case UFO2:
			case UFO3:
			case UFO4:
			case UFO5:
			case COOT:
			case COOTSTAYPUT:
			case VIXEN:
			case BIKERB:
			case BIKERBV2:
			case BIKER:
			case MAKEOUT:
			case CHEERB:
			case CHEER:
			case CHEERSTAYPUT:
			case COOTPLAY:
			case BILLYPLAY:
			case MINIONBOAT:
			case HULKBOAT:
			case CHEERBOAT:
			case RABBIT:
			case MAMA:
				if (enemysizecheat == 3)
				{
					sprite[ti].xrepeat = sprite[ti].xrepeat << 1;
					sprite[ti].yrepeat = sprite[ti].yrepeat << 1;
					sprite[ti].clipdist = mulscale7(sprite[ti].xrepeat, tilesiz[sprite[ti].picnum].x);
				}
				else if (enemysizecheat == 2)
				{
					sprite[ti].xrepeat = sprite[ti].xrepeat >> 1;
					sprite[ti].yrepeat = sprite[ti].yrepeat >> 1;
					sprite[ti].clipdist = mulscale7(sprite[ti].xrepeat, tilesiz[sprite[ti].picnum].y);
				}
				break;
			}
		}
		enemysizecheat = 0;
	}

	i = headspritestat[121];
	while (i >= 0)
	{
		nexti = nextspritestat[i];
		sprite[i].extra++;
		if (sprite[i].extra < 100)
		{
			if (sprite[i].extra == 90)
			{
				sprite[i].picnum--;
				if (sprite[i].picnum < PIG + 7)
					sprite[i].picnum = PIG + 7;
				sprite[i].extra = 1;
			}
			movesprite(i, 0, 0, -300, CLIPMASK0);
			if (sector[sprite[i].sectnum].ceilingz + (4 << 8) > sprite[i].z)
			{
				sprite[i].picnum = 0;
				sprite[i].extra = 100;
			}
		}
		else if (sprite[i].extra == 200)
		{
			setsprite(i, sprite[i].x, sprite[i].y, sector[sprite[i].sectnum].floorz - 10);
			sprite[i].extra = 1;
			sprite[i].picnum = PIG + 11;
			spawn(i, TRANSPORTERSTAR);
		}
		i = nexti;
	}

	i = headspritestat[119];
	while (i >= 0)
	{
		nexti = nextspritestat[i];
		if (sprite[i].hitag > 0)
		{
			if (sprite[i].extra == 0)
			{
				sprite[i].hitag--;
				sprite[i].extra = 150;
				spawn(i, RABBIT);
			}
			else
				sprite[i].extra--;
		}
		i = nexti;
	}
	i = headspritestat[116];
	while (i >= 0)
	{
		nexti = nextspritestat[i];
		if (sprite[i].extra)
		{
			if (sprite[i].extra == sprite[i].lotag)
				sound(183);
			sprite[i].extra--;
			j = movesprite(i,
				(sprite[i].hitag * sintable[(sprite[i].ang + 512) & 2047]) >> 14,
				(sprite[i].hitag * sintable[sprite[i].ang & 2047]) >> 14,
				sprite[i].hitag << 1, CLIPMASK0);
			if (j > 0)
			{
				spritesound(PIPEBOMB_EXPLODE, i);
				deletesprite(i);
			}
			if (sprite[i].extra == 0)
			{
				sound(215);
				deletesprite(i);
				earthquaketime = 32;
				SetPlayerPal(&ps[myconnectindex], PalEntry(32, 32, 32, 48));
			}
		}
		i = nexti;
	}

	i = headspritestat[115];
	while (i >= 0)
	{
		nexti = nextspritestat[i];
		if (sprite[i].extra)
		{
			if (sprite[i].picnum != RRTILE8162)
				sprite[i].picnum = RRTILE8162;
			sprite[i].extra--;
			if (sprite[i].extra == 0)
			{
				int rvar;
				rvar = krand() & 127;
				if (rvar < 96)
				{
					sprite[i].picnum = RRTILE8162 + 3;
				}
				else if (rvar < 112)
				{
					if (ps[screenpeek].SlotWin & 1)
					{
						sprite[i].picnum = RRTILE8162 + 3;
					}
					else
					{
						sprite[i].picnum = RRTILE8162 + 2;
						spawn(i, BATTERYAMMO);
						ps[screenpeek].SlotWin |= 1;
						spritesound(52, i);
					}
				}
				else if (rvar < 120)
				{
					if (ps[screenpeek].SlotWin & 2)
					{
						sprite[i].picnum = RRTILE8162 + 3;
					}
					else
					{
						sprite[i].picnum = RRTILE8162 + 6;
						spawn(i, HEAVYHBOMB);
						ps[screenpeek].SlotWin |= 2;
						spritesound(52, i);
					}
				}
				else if (rvar < 126)
				{
					if (ps[screenpeek].SlotWin & 4)
					{
						sprite[i].picnum = RRTILE8162 + 3;
					}
					else
					{
						sprite[i].picnum = RRTILE8162 + 5;
						spawn(i, SIXPAK);
						ps[screenpeek].SlotWin |= 4;
						spritesound(52, i);
					}
				}
				else
				{
					if (ps[screenpeek].SlotWin & 8)
					{
						sprite[i].picnum = RRTILE8162 + 3;
					}
					else
					{
						sprite[i].picnum = RRTILE8162 + 4;
						spawn(i, ATOMICHEALTH);
						ps[screenpeek].SlotWin |= 8;
						spritesound(52, i);
					}
				}
			}
		}
		i = nexti;
	}

	i = headspritestat[122];
	while (i >= 0)
	{
		nexti = nextspritestat[i];
		if (sprite[i].extra)
		{
			if (sprite[i].picnum != RRTILE8589)
				sprite[i].picnum = RRTILE8589;
			sprite[i].extra--;
			if (sprite[i].extra == 0)
			{
				int rvar;
				rvar = krand() & 127;
				if (rvar < 96)
				{
					sprite[i].picnum = RRTILE8589 + 4;
				}
				else if (rvar < 112)
				{
					if (ps[screenpeek].SlotWin & 1)
					{
						sprite[i].picnum = RRTILE8589 + 4;
					}
					else
					{
						sprite[i].picnum = RRTILE8589 + 5;
						spawn(i, BATTERYAMMO);
						ps[screenpeek].SlotWin |= 1;
						spritesound(342, i);
					}
				}
				else if (rvar < 120)
				{
					if (ps[screenpeek].SlotWin & 2)
					{
						sprite[i].picnum = RRTILE8589 + 4;
					}
					else
					{
						sprite[i].picnum = RRTILE8589 + 6;
						spawn(i, HEAVYHBOMB);
						ps[screenpeek].SlotWin |= 2;
						spritesound(342, i);
					}
				}
				else if (rvar < 126)
				{
					if (ps[screenpeek].SlotWin & 4)
					{
						sprite[i].picnum = RRTILE8589 + 4;
					}
					else
					{
						sprite[i].picnum = RRTILE8589 + 2;
						spawn(i, SIXPAK);
						ps[screenpeek].SlotWin |= 4;
						spritesound(342, i);
					}
				}
				else
				{
					if (ps[screenpeek].SlotWin & 8)
					{
						sprite[i].picnum = RRTILE8589 + 4;
					}
					else
					{
						sprite[i].picnum = RRTILE8589 + 3;
						spawn(i, ATOMICHEALTH);
						ps[screenpeek].SlotWin |= 8;
						spritesound(342, i);
					}
				}
			}
		}
		i = nexti;
	}

	i = headspritestat[123];
	while (i >= 0)
	{
		nexti = nextspritestat[i];
		if (sprite[i].lotag == 5)
			if (!S_CheckSoundPlaying(330))
				spritesound(330, i);
		i = nexti;
	}
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void rr_specialstats()
{
	int x;
	int i, j, nexti, nextj, p, pi;
	spritetype* s;
	unsigned short pst;

	i = headspritestat[107];
	while (i >= 0)
	{
		nexti = nextspritestat[i];

		if (sprite[i].hitag == 100)
		{
			sprite[i].z += (4 << 8);
			if (sprite[i].z >= sector[sprite[i].sectnum].floorz + 15168)
				sprite[i].z = sector[sprite[i].sectnum].floorz + 15168;
		}

		if (sprite[i].picnum == LUMBERBLADE)
		{
			sprite[i].extra++;
			if (sprite[i].extra == 192)
			{
				sprite[i].hitag = 0;
				sprite[i].z = sector[sprite[i].sectnum].floorz - 15168;
				sprite[i].extra = 0;
				sprite[i].picnum = RRTILE3410;
				j = headspritestat[0];
				while (j >= 0)
				{
					nextj = nextspritestat[j];
					if (sprite[j].picnum == 128)
						if (sprite[j].hitag == 999)
							sprite[j].picnum = 127;
					j = nextj;
				}
			}
		}
		i = nexti;
	}

	if (chickenplant)
	{
		i = headspritestat[106];
		while (i >= 0)
		{
			nexti = nextspritestat[i];
			switch (sprite[i].picnum)
			{
			case RRTILE285:
				sprite[i].lotag--;
				if (sprite[i].lotag < 0)
				{
					j = spawn(i, RRTILE3190);
					sprite[j].ang = sprite[i].ang;
					sprite[i].lotag = 128;
				}
				break;
			case RRTILE286:
				sprite[i].lotag--;
				if (sprite[i].lotag < 0)
				{
					j = spawn(i, RRTILE3192);
					sprite[j].ang = sprite[i].ang;
					sprite[i].lotag = 256;
				}
				break;
			case RRTILE287:
				sprite[i].lotag--;
				if (sprite[i].lotag < 0)
				{
					lotsoffeathers_r(&sprite[i], (krand() & 3) + 4);
					sprite[i].lotag = 84;
				}
				break;
			case RRTILE288:
				sprite[i].lotag--;
				if (sprite[i].lotag < 0)
				{
					j = spawn(i, RRTILE3132);
					sprite[i].lotag = 96;
					if (!isRRRA()) spritesound(472, j);
				}
				break;
			case RRTILE289:
				sprite[i].lotag--;
				if (sprite[i].lotag < 0)
				{
					j = spawn(i, RRTILE3120);
					sprite[j].ang = sprite[i].ang;
					sprite[i].lotag = 448;
				}
				break;
			case RRTILE290:
				sprite[i].lotag--;
				if (sprite[i].lotag < 0)
				{
					j = spawn(i, RRTILE3122);
					sprite[j].ang = sprite[i].ang;
					sprite[i].lotag = 64;
				}
				break;
			case RRTILE291:
				sprite[i].lotag--;
				if (sprite[i].lotag < 0)
				{
					j = spawn(i, RRTILE3123);
					sprite[j].ang = sprite[i].ang;
					sprite[i].lotag = 512;
				}
				break;
			case RRTILE292:
				sprite[i].lotag--;
				if (sprite[i].lotag < 0)
				{
					j = spawn(i, RRTILE3124);
					sprite[j].ang = sprite[i].ang;
					sprite[i].lotag = 224;
				}
				break;
			case RRTILE293:
				sprite[i].lotag--;
				if (sprite[i].lotag < 0)
				{
					guts_r(&sprite[i], JIBS1, 1, myconnectindex);
					guts_r(&sprite[i], JIBS2, 1, myconnectindex);
					guts_r(&sprite[i], JIBS3, 1, myconnectindex);
					guts_r(&sprite[i], JIBS4, 1, myconnectindex);
					sprite[i].lotag = 256;
				}
				break;
			}
			i = nexti;
		}
	}

	i = headspritestat[105];
	while (i >= 0)
	{
		nexti = nextspritestat[i];
		if (sprite[i].picnum == RRTILE280)
			if (sprite[i].lotag == 100)
			{
				pst = pinsectorresetup(sprite[i].sectnum);
				if (pst)
				{
					sprite[i].lotag = 0;
					if (sprite[i].extra == 1)
					{
						pst = checkpins(sprite[i].sectnum);
						if (!pst)
						{
							sprite[i].extra = 2;
						}
					}
					if (sprite[i].extra == 2)
					{
						sprite[i].extra = 0;
						resetpins(sprite[i].sectnum);
					}
				}
			}
		i = nexti;
	}

	i = headspritestat[108];
	while (i >= 0)
	{
		nexti = nextspritestat[i];

		s = &sprite[i];
		if (s->picnum == RRTILE296)
		{
			p = findplayer(s, &x);
			if (x < 2047)
			{
				j = headspritestat[108];
				while (j >= 0)
				{
					nextj = nextspritestat[j];
					if (sprite[j].picnum == RRTILE297)
					{
						ps[p].setang(sprite[j].ang);
						ps[p].bobposx = ps[p].oposx = ps[p].posx = sprite[j].x;
						ps[p].bobposy = ps[p].oposy = ps[p].posy = sprite[j].y;
						ps[p].oposz = ps[p].posz = sprite[j].z - (36 << 8);
						pi = ps[p].i;
						changespritesect(pi, sprite[j].sectnum);
						ps[p].cursectnum = sprite[pi].sectnum;
						spritesound(70, j);
						deletesprite(j);
					}
					j = nextj;
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

static void heavyhbomb(int i)
{
	spritetype* s = &sprite[i];
	auto t = &hittype[i].temp_data[0];
	int sect = s->sectnum;
	int x, j, l;

	if ((s->cstat & 32768))
	{
		t[2]--;
		if (t[2] <= 0)
		{
			spritesound(TELEPORTER, i);
			spawn(i, TRANSPORTERSTAR);
			s->cstat = 257;
		}
		return;
	}

	int p = findplayer(s, &x);

	if (x < 1220) s->cstat &= ~257;
	else s->cstat |= 257;

	if (t[3] == 0)
	{
		j = ifhitbyweapon(i);
		if (j >= 0)
		{
			t[3] = 1;
			t[4] = 0;
			l = 0;
			s->xvel = 0;
			goto DETONATEB;
		}
	}

	makeitfall(i);

	if (sector[sect].lotag != 1 && (!isRRRA() || sector[sect].lotag != 160) && s->z >= hittype[i].floorz - (FOURSLEIGHT) && s->yvel < 3)
	{
		if (s->yvel > 0 || (s->yvel == 0 && hittype[i].floorz == sector[sect].floorz))
		{
			if (s->picnum != CHEERBOMB)
				spritesound(PIPEBOMB_BOUNCE, i);
			else
			{
				t[3] = 1;
				t[4] = 1;
				l = 0;
				goto DETONATEB;
			}
		}
		s->zvel = -((4 - s->yvel) << 8);
		if (sector[s->sectnum].lotag == 2)
			s->zvel >>= 2;
		s->yvel++;
	}
	if (s->picnum != CHEERBOMB && s->z < hittype[i].ceilingz + (16 << 8) && sector[sect].lotag != 2)
	{
		s->z = hittype[i].ceilingz + (16 << 8);
		s->zvel = 0;
	}

	j = movesprite(i,
		(s->xvel * (sintable[(s->ang + 512) & 2047])) >> 14,
		(s->xvel * (sintable[s->ang & 2047])) >> 14,
		s->zvel, CLIPMASK0);

	if (sector[sprite[i].sectnum].lotag == 1 && s->zvel == 0)
	{
		s->z += (32 << 8);
		if (t[5] == 0)
		{
			t[5] = 1;
			spawn(i, WATERSPLASH2);
			if (isRRRA() && s->picnum == MORTER)
				s->xvel = 0;
		}
	}
	else t[5] = 0;

	if (t[3] == 0 && s->picnum == MORTER && (j || x < 844))
	{
		t[3] = 1;
		t[4] = 0;
		l = 0;
		s->xvel = 0;
		goto DETONATEB;
	}

	if (t[3] == 0 && s->picnum == CHEERBOMB && (j || x < 844))
	{
		t[3] = 1;
		t[4] = 0;
		l = 0;
		s->xvel = 0;
		goto DETONATEB;
	}

	if (sprite[s->owner].picnum == APLAYER)
		l = sprite[s->owner].yvel;
	else l = -1;

	if (s->xvel > 0)
	{
		s->xvel -= 5;
		if (sector[sect].lotag == 2)
			s->xvel -= 10;

		if (s->xvel < 0)
			s->xvel = 0;
		if (s->xvel & 8) s->cstat ^= 4;
	}

	if ((j & 49152) == 32768)
	{
		j &= (MAXWALLS - 1);

		checkhitwall(i, j, s->x, s->y, s->z, s->picnum);

		int k = getangle(
			wall[wall[j].point2].x - wall[j].x,
			wall[wall[j].point2].y - wall[j].y);

		if (s->picnum == CHEERBOMB)
		{
			t[3] = 1;
			t[4] = 0;
			l = 0;
			s->xvel = 0;
			goto DETONATEB;
		}
		s->ang = ((k << 1) - s->ang) & 2047;
		s->xvel >>= 1;
	}

DETONATEB:

	if ((l >= 0 && ps[l].hbomb_on == 0) || t[3] == 1)
	{
		t[4]++;

		if (t[4] == 2)
		{
			x = s->extra;
			int m = 0;
			switch (s->picnum)
			{
			case TRIPBOMBSPRITE: m = powderkegblastradius; break;
			case HEAVYHBOMB: m = pipebombblastradius; break;
			case HBOMBAMMO: m = pipebombblastradius; break;
			case MORTER: m = morterblastradius; break;
			case CHEERBOMB: m = morterblastradius; break;
			}

			if (sector[s->sectnum].lotag != 800)
			{
				hitradius(i, m, x >> 2, x >> 1, x - (x >> 2), x);
				spawn(i, EXPLOSION2);
				if (s->picnum == CHEERBOMB)
					spawn(i, BURNING);
				spritesound(PIPEBOMB_EXPLODE, i);
				for (x = 0; x < 8; x++)
					RANDOMSCRAP(s, i);
			}
		}

		if (s->yrepeat)
		{
			s->yrepeat = 0;
			return;
		}

		if (t[4] > 20)
		{
			deletesprite(i);
			return;
		}
		if (s->picnum == CHEERBOMB)
		{
			spawn(i, BURNING);
			deletesprite(i);
			return;
		}
	}
	else if (s->picnum == HEAVYHBOMB && x < 788 && t[0] > 7 && s->xvel == 0)
		if (cansee(s->x, s->y, s->z - (8 << 8), s->sectnum, ps[p].posx, ps[p].posy, ps[p].posz, ps[p].cursectnum))
			if (ps[p].ammo_amount[DYNAMITE_WEAPON] < max_ammo_amount[DYNAMITE_WEAPON])
				if (s->pal == 0)
				{
					if (ud.coop >= 1)
					{
						for (j = 0; j < ps[p].weapreccnt; j++)
							if (ps[p].weaprecs[j] == i)
								return;

						if (ps[p].weapreccnt < 255)
							ps[p].weaprecs[ps[p].weapreccnt++] = i;
					}

					addammo(DYNAMITE_WEAPON, &ps[p], 1);
					addammo(CROSSBOW_WEAPON, &ps[p], 1);
					spritesound(DUKE_GET, ps[p].i);

					if (ps[p].gotweapon[DYNAMITE_WEAPON] == 0 || s->owner == ps[p].i)
						addweapon(&ps[p], DYNAMITE_WEAPON);

					if (sprite[s->owner].picnum != APLAYER)
					{
						SetPlayerPal(&ps[p], PalEntry(32, 0, 32, 0));
					}

					if (hittype[s->owner].picnum != HEAVYHBOMB || ud.respawn_items == 0 || sprite[s->owner].picnum == APLAYER)
					{
						if (s->picnum == HEAVYHBOMB &&
							sprite[s->owner].picnum != APLAYER && ud.coop)
							return;
						deletesprite(i);
						return;
					}
					else
					{
						t[2] = respawnitemtime;
						spawn(i, RESPAWNMARKERRED);
						s->cstat = (short)32768;
					}
				}

	if (t[0] < 8) t[0]++;
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

static int henstand(int i)
{
	spritetype* s = &sprite[i];
	auto t = &hittype[i].temp_data[0];
	int sect = s->sectnum;
	int j;

	if (s->picnum == HENSTAND || s->picnum == HENSTAND + 1)
	{
		s->lotag--;
		if (s->lotag == 0)
		{
			spawn(i, HEN);
			deletesprite(i);
			return 1;
		}
	}
	if (sector[s->sectnum].lotag == 900)
		s->xvel = 0;
	if (s->xvel)
	{
		makeitfall(i);
		j = movesprite(i,
			(sintable[(s->ang + 512) & 2047] * s->xvel) >> 14,
			(sintable[s->ang & 2047] * s->xvel) >> 14,
			s->zvel, CLIPMASK0);
		if (j & 49152)
		{
			if ((j & 49152) == 32768)
			{
				j &= (MAXWALLS - 1);
				int k = getangle(
					wall[wall[j].point2].x - wall[j].x,
					wall[wall[j].point2].y - wall[j].y);
				s->ang = ((k << 1) - s->ang) & 2047;
			}
			else if ((j & 49152) == 49152)
			{
				j &= (MAXSPRITES - 1);
				checkhitsprite(i, j);
				if (sprite[j].picnum == HEN)
				{
					int ns = spawn(j, HENSTAND);
					deletesprite(j);
					sprite[ns].xvel = 32;
					sprite[ns].lotag = 40;
					sprite[ns].ang = s->ang;
				}
			}
		}
		s->xvel--;
		if (s->xvel < 0) s->xvel = 0;
		s->cstat = 257;
		if (s->picnum == RRTILE3440)
		{
			s->cstat |= 4 & s->xvel;
			s->cstat |= 8 & s->xvel;
			if (krand() & 1)
				s->picnum = RRTILE3440 + 1;
		}
		else if (s->picnum == HENSTAND)
		{
			s->cstat |= 4 & s->xvel;
			s->cstat |= 8 & s->xvel;
			if (krand() & 1)
				s->picnum = HENSTAND + 1;
			if (!s->xvel)
				return 2;//deletesprite(i); still needs to run a script but should not do on a deleted object
		}
		if (s->picnum == RRTILE3440 || (s->picnum == RRTILE3440 + 1 && !s->xvel))
		{
			return 2;//deletesprite(i); still needs to run a script but should not do on a deleted object
		}
	}
	else if (sector[s->sectnum].lotag == 900)
	{
		if (s->picnum == BOWLINGBALL)
			ballreturn(i);
		deletesprite(i);
		return 1;
	}
	return 0;
}
//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void moveactors_r(void)
{
    int x, nexti;
    int j, sect, p;
    spritetype *s;
    
    dojaildoor();
    moveminecart();

	if (isRRRA())
	{
		rrra_specialstats();
	}
	rr_specialstats();

    for (int i = headspritestat[1]; i >= 0; i = nexti)
    {
        nexti = nextspritestat[i];
		bool deleteafterexecute = false;	// taking a cue here from RedNukem to not run scripts on deleted sprites.

        s = &sprite[i];

        sect = s->sectnum;

        if( s->xrepeat == 0 || sect < 0 || sect >= MAXSECTORS)
		{
			deletesprite(i);
			continue;
		}

        auto t = &hittype[i].temp_data[0];

        hittype[i].bposx = s->x;
        hittype[i].bposy = s->y;
        hittype[i].bposz = s->z;


        switch(s->picnum)
        {
            case RESPAWNMARKERRED:
            case RESPAWNMARKERYELLOW:
            case RESPAWNMARKERGREEN:
				if (!respawnmarker(i, RESPAWNMARKERYELLOW, RESPAWNMARKERGREEN)) continue;
				break;
            case RAT:
				if (!rat(i, !isRRRA())) continue;
                break;
            case RRTILE3190:
            case RRTILE3191:
            case RRTILE3192:
                if (!chickenplant) 
				{
					deletesprite(i);
					continue;
				}
                if (sector[sprite[i].sectnum].lotag == 903)
                    makeitfall(i);
                j = movesprite(i,
                    (s->xvel*sintable[(s->ang+512)&2047])>>14,
                    (s->xvel*sintable[s->ang&2047])>>14,
                    s->zvel,CLIPMASK0);
                switch (sector[sprite[i].sectnum].lotag)
                {
                    case 901:
                        sprite[i].picnum = RRTILE3191;
                        break;
                    case 902:
                        sprite[i].picnum = RRTILE3192;
                        break;
                    case 903:
                        if (sprite[i].z >= sector[sprite[i].sectnum].floorz - (8<<8)) 
						{
							deletesprite(i);
							continue;
						}
                        break;
                    case 904:
						deletesprite(i);
						continue;
                        break;
                }
				if ((j & 32768) == 32768) 
				{
					deletesprite(i);
					continue;
				}
                break;

            case RRTILE3120:
            case RRTILE3122:
            case RRTILE3123:
            case RRTILE3124:
                if (!chickenplant) 
				{
					deletesprite(i);
					continue;
				}
                makeitfall(i);
                j = movesprite(i,
                    (s->xvel*(sintable[(s->ang+512)&2047]))>>14,
                    (s->xvel*(sintable[s->ang&2047]))>>14,
                    s->zvel,CLIPMASK0);
				if ((j & 32768) == 32768) 
				{
					deletesprite(i);
					continue;
				}
                if (sector[s->sectnum].lotag == 903)
                {
                    if (sprite[i].z >= sector[sprite[i].sectnum].floorz - (4<<8))
					{
						deletesprite(i);
						continue;
					}
                }
                else if (sector[s->sectnum].lotag == 904)
				{
					deletesprite(i);
					continue;
				}
                break;

            case RRTILE3132:
                if (!chickenplant) 
				{
					deletesprite(i);
					continue;
				}
                makeitfall(i);
                j = movesprite(i,
                    (s->xvel*sintable[(s->ang+512)&2047])>>14,
                    (s->xvel*sintable[s->ang&2047])>>14,
                    s->zvel,CLIPMASK0);
                if (s->z >= sector[s->sectnum].floorz - (8<<8))
                {
                    if (sector[s->sectnum].lotag == 1)
                    {
                        j = spawn(i,WATERSPLASH2);
                        sprite[j].z = sector[sprite[j].sectnum].floorz;
                    }
					deletesprite(i);
					continue;
                }
                break;
            case BOWLINGBALL:
                if (s->xvel)
                {
                    if(!S_CheckSoundPlaying(356))
                        spritesound(356,i);
                }
                else
                {
                    spawn(i,BOWLINGBALLSPRITE);
					deletesprite(i);
					continue;
                }
                if (sector[s->sectnum].lotag == 900)
                {
                    S_StopEnvSound(356, -1);
                }
            case RRTILE3440:
            case RRTILE3440+1:
            case HENSTAND:
            case HENSTAND+1:
			{
				int todo = henstand(i);
				if (todo == 2) deleteafterexecute = true;
				if (todo == 1) continue;
                break;
			}

            case QUEBALL:
            case STRIPEBALL:
				if (!queball(i, POCKET, QUEBALL, STRIPEBALL)) continue;
                break;
            case FORCESPHERE:
				forcesphere(i, FORCESPHERE);
                continue;

            case RECON:
            case UFO1_RR:
            case UFO2:
            case UFO3:
            case UFO4:
            case UFO5:
				recon(i, EXPLOSION2, FIRELASER, -1, -1, 457, 8, [](int i) ->int
					{
						auto s = &sprite[i];
						if (isRRRA() && ufospawnsminion)
							return MINION;
						else if (s->picnum == UFO1_RR)
							return HEN;
						else if (s->picnum == UFO2)
							return COOT;
						else if (s->picnum == UFO3)
							return COW;
						else if (s->picnum == UFO4)
							return PIG;
						else if (s->picnum == UFO5)
							return BILLYRAY;
						else return -1;
					});
                continue;

            case OOZ:
				ooz(i);
                continue;

            case EMPTYBIKE:
				if (!isRRRA()) break;
                makeitfall(i);
                getglobalz(i);
                if (sector[sect].lotag == 1)
                {
                    setsprite(i,s->x,s->y,hittype[i].floorz+(16<<8));
                }
                break;

            case EMPTYBOAT:
				if (!isRRRA()) break;
				makeitfall(i);
                getglobalz(i);
                break;

            case TRIPBOMBSPRITE:
                if (!isRRRA() || (sector[sect].lotag != 1 && sector[sect].lotag != 160))
					if (s->xvel)
					{
						j = movesprite(i,
							(s->xvel*sintable[(s->ang+512)&2047])>>14,
							(s->xvel*sintable[s->ang&2047])>>14,
							s->zvel,CLIPMASK0);
						s->xvel--;
					}
                break;

			case CHEERBOMB:
				if (!isRRRA()) break;
			case MORTER:
            case HEAVYHBOMB:
				heavyhbomb(i);
                continue;
                
            case REACTORBURNT:
            case REACTOR2BURNT:
                continue;

            case REACTOR:
            case REACTOR2:
				reactor(i, REACTOR, REACTOR2, REACTOR2BURNT, REACTOR2BURNT);
                continue;

            case RR_CAMERA1:
				camera(i);
                continue;
        }


// #ifndef VOLOMEONE
        if( ud.multimode < 2 && badguy(s) )
        {
            if( actor_tog == 1)
            {
                s->cstat = (short)32768;
                continue;
            }
            else if(actor_tog == 2) s->cstat = 257;
        }
// #endif

        p = findplayer(s,&x);

        execute(i,p,x);
		if (deleteafterexecute) deletesprite(i);
    }

}


END_DUKE_NS
