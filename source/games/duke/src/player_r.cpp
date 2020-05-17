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
#include "game.h"
#include "names_rr.h"

BEGIN_DUKE_NS 

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void incur_damage_r(struct player_struct* p)
{
	long  damage = 0L, unk = 0L, shield_damage = 0L;
	short gut = 0;

	sprite[p->i].extra -= p->extra_extra8 >> 8;

	damage = sprite[p->i].extra - p->last_extra;
	if (damage < 0)
	{
		p->extra_extra8 = 0;

		if (p->steroids_amount > 0 && p->steroids_amount < 400)
		{
			shield_damage = damage * (20 + (krand() % 30)) / 100;
			damage -= shield_damage;
		}
		if (p->drink_amt > 31 && p->drink_amt < 65)
			gut++;
		if (p->eat > 31 && p->eat < 65)
			gut++;

		switch (gut)
		{
			double ddamage;
		case 1:
			ddamage = damage;
			ddamage *= 0.75;
			damage = ddamage;
			break;
		case 2:
			ddamage = damage;
			ddamage *= 0.25;
			damage = ddamage;
			break;
		}

		sprite[p->i].extra = p->last_extra + damage;
	}
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void shoot_r(int i, int atwith)
{
	short sect, hitsect, hitspr, hitwall, l, sa, p, j, k, scount;
	int sx, sy, sz, vel, zvel, hitx, hity, hitz, x, oldzvel, dal;
	uint8_t sizx, sizy;
	spritetype* s;

	s = &sprite[i];
	sect = s->sectnum;
	zvel = 0;

	if (s->picnum == TILE_APLAYER)
	{
		p = s->yvel;

		sx = ps[p].posx;
		sy = ps[p].posy;
		sz = ps[p].posz + ps[p].pyoff + (4 << 8);
		sa = ps[p].getang();

		if (isRRRA()) ps[p].crack_time = 777;
	}
	else
	{
		p = -1;
		sa = s->ang;
		sx = s->x;
		sy = s->y;
		sz = s->z - ((s->yrepeat * tilesiz[s->picnum].y) << 1) + (4 << 8);
		sz -= (7 << 8);
		if (badguy(s))
		{
			sx += (sintable[(sa + 1024 + 96) & 2047] >> 7);
			sy += (sintable[(sa + 512 + 96) & 2047] >> 7);
		}
	}

	switch (atwith)
	{
	case SLINGBLADE:
		if (!isRRRA()) break;
		goto rrra_slingblade;

	case BLOODSPLAT1:
	case BLOODSPLAT2:
	case BLOODSPLAT3:
	case BLOODSPLAT4:

		if (p >= 0)
			sa += 64 - (krand() & 127);
		else sa += 1024 + 64 - (krand() & 127);
		zvel = 1024 - (krand() & 2047);
	case KNEE:
	case GROWSPARK:
	rrra_slingblade:
		if (atwith == KNEE || atwith == GROWSPARK || atwith == SLINGBLADE)
		{
			if (p >= 0)
			{
				zvel = (100 - ps[p].gethorizdiff()) << 5;
				sz += (6 << 8);
				sa += 15;
			}
			else
			{
				j = ps[findplayer(s, &x)].i;
				zvel = ((sprite[j].z - sz) << 8) / (x + 1);
				sa = getangle(sprite[j].x - sx, sprite[j].y - sy);
			}
		}

		hitscan(sx, sy, sz, sect,
			sintable[(sa + 512) & 2047],
			sintable[sa & 2047], zvel << 6,
			&hitsect, &hitwall, &hitspr, &hitx, &hity, &hitz, CLIPMASK1);

		if (isRRRA() && ((sector[hitsect].lotag == 160 && zvel > 0) || (sector[hitsect].lotag == 161 && zvel < 0))
			&& hitspr == -1 && hitwall == -1)
		{
			short ii;
			for (ii = 0; ii < MAXSPRITES; ii++)
			{
				if (sprite[ii].sectnum == hitsect && sprite[ii].picnum == SECTOREFFECTOR
					&& sprite[ii].lotag == 7)
				{
					int nx, ny, nz;
					nx = hitx + (sprite[sprite[ii].owner].x - sprite[ii].x);
					ny = hity + (sprite[sprite[ii].owner].y - sprite[ii].y);
					if (sector[hitsect].lotag == 161)
					{
						nz = sector[sprite[sprite[ii].owner].sectnum].floorz;
					}
					else
					{
						nz = sector[sprite[sprite[ii].owner].sectnum].ceilingz;
					}
					hitscan(nx, ny, nz, sprite[sprite[ii].owner].sectnum,
						sintable[(sa + 512) & 2047],
						sintable[sa & 2047], zvel << 6,
						&hitsect, &hitwall, &hitspr, &hitx, &hity, &hitz, CLIPMASK1);
					break;
				}
			}
		}

		if (atwith == BLOODSPLAT1 ||
			atwith == BLOODSPLAT2 ||
			atwith == BLOODSPLAT3 ||
			atwith == BLOODSPLAT4)
		{
			if (FindDistance2D(sx - hitx, sy - hity) < 1024)
				if (hitwall >= 0 && wall[hitwall].overpicnum != BIGFORCE)
					if ((wall[hitwall].nextsector >= 0 && hitsect >= 0 &&
						sector[wall[hitwall].nextsector].lotag == 0 &&
						sector[hitsect].lotag == 0 &&
						sector[wall[hitwall].nextsector].lotag == 0 &&
						(sector[hitsect].floorz - sector[wall[hitwall].nextsector].floorz) > (16 << 8)) ||
						(wall[hitwall].nextsector == -1 && sector[hitsect].lotag == 0))
						if ((wall[hitwall].cstat & 16) == 0)
						{
							if (wall[hitwall].nextsector >= 0)
							{
								k = headspritesect[wall[hitwall].nextsector];
								while (k >= 0)
								{
									if (sprite[k].statnum == 3 && sprite[k].lotag == 13)
										return;
									k = nextspritesect[k];
								}
							}

							if (wall[hitwall].nextwall >= 0 &&
								wall[wall[hitwall].nextwall].hitag != 0)
								return;

							if (wall[hitwall].hitag == 0)
							{
								k = fi.spawn(i, atwith);
								sprite[k].xvel = -12;
								sprite[k].ang = getangle(wall[hitwall].x - wall[wall[hitwall].point2].x,
									wall[hitwall].y - wall[wall[hitwall].point2].y) + 512;
								sprite[k].x = hitx;
								sprite[k].y = hity;
								sprite[k].z = hitz;
								sprite[k].cstat |= (krand() & 4);
								ssp(k, CLIPMASK0);
								setsprite(k, sprite[k].x, sprite[k].y, sprite[k].z);
								if (s->picnum == OOZFILTER)
									sprite[k].pal = 6;
							}
						}
			return;
		}

		if (hitsect < 0) break;

		if ((abs(sx - hitx) + abs(sy - hity)) < 1024)
		{
			if (hitwall >= 0 || hitspr >= 0)
			{
				if (isRRRA() && atwith == SLINGBLADE)
				{
					j = EGS(hitsect, hitx, hity, hitz, SLINGBLADE, -15, 0, 0, sa, 32, 0, i, 4);
					sprite[j].extra += 50;
				}
				else
				{
					j = EGS(hitsect, hitx, hity, hitz, KNEE, -15, 0, 0, sa, 32, 0, i, 4);
					sprite[j].extra += (krand() & 7);
				}
				if (p >= 0)
				{
					k = fi.spawn(j, SMALLSMOKE);
					sprite[k].z -= (8 << 8);
					if (atwith == KNEE)
						spritesound(KICK_HIT, j);
					else if (isRRRA() && atwith == SLINGBLADE)
						spritesound(260, j);
				}

				if (p >= 0 && ps[p].steroids_amount > 0 && ps[p].steroids_amount < 400)
					sprite[j].extra += (max_player_health >> 2);

				if (hitspr >= 0 && sprite[hitspr].picnum != ACCESSSWITCH && sprite[hitspr].picnum != ACCESSSWITCH2)
				{
					fi.checkhitsprite(hitspr, j);
					if (p >= 0) fi.checkhitswitch(p, hitspr, 1);
				}
				else if (hitwall >= 0)
				{
					if (wall[hitwall].cstat & 2)
						if (wall[hitwall].nextsector >= 0)
							if (hitz >= (sector[wall[hitwall].nextsector].floorz))
								hitwall = wall[hitwall].nextwall;

					if (hitwall >= 0 && wall[hitwall].picnum != ACCESSSWITCH && wall[hitwall].picnum != ACCESSSWITCH2)
					{
						fi.checkhitwall(j, hitwall, hitx, hity, hitz, atwith);
						if (p >= 0) fi.checkhitswitch(p, hitwall, 0);
					}
				}
			}
			else if (p >= 0 && zvel > 0 && sector[hitsect].lotag == 1)
			{
				j = fi.spawn(ps[p].i, WATERSPLASH2);
				sprite[j].x = hitx;
				sprite[j].y = hity;
				sprite[j].ang = ps[p].getang(); // Total tweek
				sprite[j].xvel = 32;
				ssp(i, 0);
				sprite[j].xvel = 0;

			}
		}

		break;

	case SHOTSPARK1:
	case SHOTGUN:
	case CHAINGUN:

		if (s->extra >= 0) s->shade = -96;

		if (p >= 0)
		{
			j = aim(s, AUTO_AIM_ANGLE);
			if (j >= 0)
			{
				dal = ((sprite[j].xrepeat * tilesiz[sprite[j].picnum].y) << 1) + (5 << 8);
				zvel = ((sprite[j].z - sz - dal) << 8) / ldist(&sprite[ps[p].i], &sprite[j]);
				sa = getangle(sprite[j].x - sx, sprite[j].y - sy);
			}

			if (atwith == SHOTSPARK1)
			{
				if (j == -1)
				{
					sa += 16 - (krand() & 31);
					zvel = (100 - ps[p].gethorizdiff()) << 5;
					zvel += 128 - (krand() & 255);
				}
			}
			else
			{
				if (atwith == SHOTGUN)
					sa += 64 - (krand() & 127);
				else
					sa += 16 - (krand() & 31);
				if (j == -1) zvel = (100 - ps[p].gethorizdiff()) << 5;
				zvel += 128 - (krand() & 255);
			}
			sz -= (2 << 8);
		}
		else
		{
			j = findplayer(s, &x);
			sz -= (4 << 8);
			zvel = ((ps[j].posz - sz) << 8) / (ldist(&sprite[ps[j].i], s));
			if (s->picnum != BOSS1)
			{
				zvel += 128 - (krand() & 255);
				sa += 32 - (krand() & 63);
			}
			else
			{
				zvel += 128 - (krand() & 255);
				sa = getangle(ps[j].posx - sx, ps[j].posy - sy) + 64 - (krand() & 127);
			}
		}

		s->cstat &= ~257;
		hitscan(sx, sy, sz, sect,
			sintable[(sa + 512) & 2047],
			sintable[sa & 2047],
			zvel << 6, &hitsect, &hitwall, &hitspr, &hitx, &hity, &hitz, CLIPMASK1);

		if (isRRRA() && (((sector[hitsect].lotag == 160 && zvel > 0) || (sector[hitsect].lotag == 161 && zvel < 0))
			&& hitspr == -1 && hitwall == -1))
		{
			short ii;
			for (ii = 0; ii < MAXSPRITES; ii++)
			{
				if (sprite[ii].sectnum == hitsect && sprite[ii].picnum == SECTOREFFECTOR
					&& sprite[ii].lotag == 7)
				{
					int nx, ny, nz;
					nx = hitx + (sprite[sprite[ii].owner].x - sprite[ii].x);
					ny = hity + (sprite[sprite[ii].owner].y - sprite[ii].y);
					if (sector[hitsect].lotag == 161)
					{
						nz = sector[sprite[sprite[ii].owner].sectnum].floorz;
					}
					else
					{
						nz = sector[sprite[sprite[ii].owner].sectnum].ceilingz;
					}
					hitscan(nx, ny, nz, sprite[sprite[ii].owner].sectnum,
						sintable[(sa + 512) & 2047],
						sintable[sa & 2047], zvel << 6,
						&hitsect, &hitwall, &hitspr, &hitx, &hity, &hitz, CLIPMASK1);
					break;
				}
			}
		}

		s->cstat |= 257;

		if (hitsect < 0) return;

		if (atwith == SHOTGUN)
			if (sector[hitsect].lotag == 1)
				if (krand() & 1)
					return;

		if ((krand() & 15) == 0 && sector[hitsect].lotag == 2)
			tracers(hitx, hity, hitz, sx, sy, sz, 8 - (ud.multimode >> 1));

		if (p >= 0)
		{
			k = EGS(hitsect, hitx, hity, hitz, SHOTSPARK1, -15, 10, 10, sa, 0, 0, i, 4);
			sprite[k].extra = ScriptCode[actorinfo[atwith].scriptaddress];
			sprite[k].extra += (krand() % 6);

			if (hitwall == -1 && hitspr == -1)
			{
				if (zvel < 0)
				{
					if (sector[hitsect].ceilingstat & 1)
					{
						sprite[k].xrepeat = 0;
						sprite[k].yrepeat = 0;
						return;
					}
					else
						fi.checkhitceiling(hitsect);
				}
				if (sector[hitsect].lotag != 1)
					fi.spawn(k, SMALLSMOKE);
			}

			if (hitspr >= 0)
			{
				if (sprite[hitspr].picnum == 1930)
					return;
				fi.checkhitsprite(hitspr, k);
				if (sprite[hitspr].picnum == TILE_APLAYER && (ud.coop != 1 || ud.ffire == 1))
				{
					l = fi.spawn(k, JIBS6);
					sprite[k].xrepeat = sprite[k].yrepeat = 0;
					sprite[l].z += (4 << 8);
					sprite[l].xvel = 16;
					sprite[l].xrepeat = sprite[l].yrepeat = 24;
					sprite[l].ang += 64 - (krand() & 127);
				}
				else fi.spawn(k, SMALLSMOKE);

				if (p >= 0 && (
					sprite[hitspr].picnum == DIPSWITCH ||
					sprite[hitspr].picnum == DIPSWITCH + 1 ||
					sprite[hitspr].picnum == DIPSWITCH2 ||
					sprite[hitspr].picnum == DIPSWITCH2 + 1 ||
					sprite[hitspr].picnum == DIPSWITCH3 ||
					sprite[hitspr].picnum == DIPSWITCH3 + 1 ||
					(isRRRA() && sprite[hitspr].picnum == RRTILE8660) ||
					sprite[hitspr].picnum == HANDSWITCH ||
					sprite[hitspr].picnum == HANDSWITCH + 1))
				{
					fi.checkhitswitch(p, hitspr, 1);
					return;
				}
			}
			else if (hitwall >= 0)
			{
				fi.spawn(k, SMALLSMOKE);

				if (fi.isadoorwall(wall[hitwall].picnum) == 1)
					goto SKIPBULLETHOLE;
				if (isablockdoor(wall[hitwall].picnum) == 1)
					goto SKIPBULLETHOLE;
				if (p >= 0 && (
					wall[hitwall].picnum == DIPSWITCH ||
					wall[hitwall].picnum == DIPSWITCH + 1 ||
					wall[hitwall].picnum == DIPSWITCH2 ||
					wall[hitwall].picnum == DIPSWITCH2 + 1 ||
					wall[hitwall].picnum == DIPSWITCH3 ||
					wall[hitwall].picnum == DIPSWITCH3 + 1 ||
					(isRRRA() && wall[hitwall].picnum == RRTILE8660) ||
					wall[hitwall].picnum == HANDSWITCH ||
					wall[hitwall].picnum == HANDSWITCH + 1))
				{
					fi.checkhitswitch(p, hitwall, 0);
					return;
				}

				if (wall[hitwall].hitag != 0 || (wall[hitwall].nextwall >= 0 && wall[wall[hitwall].nextwall].hitag != 0))
					goto SKIPBULLETHOLE;

				if (hitsect >= 0 && sector[hitsect].lotag == 0)
					if (wall[hitwall].overpicnum != BIGFORCE)
						if ((wall[hitwall].nextsector >= 0 && sector[wall[hitwall].nextsector].lotag == 0) ||
							(wall[hitwall].nextsector == -1 && sector[hitsect].lotag == 0))
							if ((wall[hitwall].cstat & 16) == 0)
							{
								if (wall[hitwall].nextsector >= 0)
								{
									l = headspritesect[wall[hitwall].nextsector];
									while (l >= 0)
									{
										if (sprite[l].statnum == 3 && sprite[l].lotag == 13)
											goto SKIPBULLETHOLE;
										l = nextspritesect[l];
									}
								}

								l = headspritestat[5];
								while (l >= 0)
								{
									if (sprite[l].picnum == BULLETHOLE)
										if (dist(&sprite[l], &sprite[k]) < (12 + (krand() & 7)))
											goto SKIPBULLETHOLE;
									l = nextspritestat[l];
								}
								l = fi.spawn(k, BULLETHOLE);
								sprite[l].xvel = -1;
								sprite[l].ang = getangle(wall[hitwall].x - wall[wall[hitwall].point2].x,
									wall[hitwall].y - wall[wall[hitwall].point2].y) + 512;
								ssp(l, CLIPMASK0);
							}

			SKIPBULLETHOLE:

				if (wall[hitwall].cstat & 2)
					if (wall[hitwall].nextsector >= 0)
						if (hitz >= (sector[wall[hitwall].nextsector].floorz))
							hitwall = wall[hitwall].nextwall;

				fi.checkhitwall(k, hitwall, hitx, hity, hitz, SHOTSPARK1);
			}
		}
		else
		{
			k = EGS(hitsect, hitx, hity, hitz, SHOTSPARK1, -15, 24, 24, sa, 0, 0, i, 4);
			sprite[k].extra = ScriptCode[actorinfo[atwith].scriptaddress];

			if (hitspr >= 0)
			{
				fi.checkhitsprite(hitspr, k);
				if (sprite[hitspr].picnum != TILE_APLAYER)
					fi.spawn(k, SMALLSMOKE);
				else sprite[k].xrepeat = sprite[k].yrepeat = 0;
			}
			else if (hitwall >= 0)
				fi.checkhitwall(k, hitwall, hitx, hity, hitz, SHOTSPARK1);
		}

		if ((krand() & 255) < 10)
		{
			vec3_t v{ hitx, hity, hitz };
			S_PlaySound3D(PISTOL_RICOCHET, k, &v);
		}

		return;

	case TRIPBOMBSPRITE:
		j = fi.spawn(i, atwith);
		sprite[j].xvel = 32;
		sprite[j].ang = sprite[i].ang;
		sprite[j].z -= (5 << 8);
		break;

	case BOWLINGBALL:
		j = fi.spawn(i, atwith);
		sprite[j].xvel = 250;
		sprite[j].ang = sprite[i].ang;
		sprite[j].z -= (15 << 8);
		break;

	case OWHIP:
	case UWHIP:

		if (s->extra >= 0) s->shade = -96;

		scount = 1;
		if (atwith == 3471)
		{
			vel = 300;
			sz -= (15 << 8);
			scount = 1;
		}
		else if (atwith == 3475)
		{
			vel = 300;
			sz += (4 << 8);
			scount = 1;
		}

		if (p >= 0)
		{
			j = aim(s, AUTO_AIM_ANGLE);

			if (j >= 0)
			{
				dal = ((sprite[j].xrepeat * tilesiz[sprite[j].picnum].y) << 1) - (12 << 8);
				zvel = ((sprite[j].z - sz - dal) * vel) / ldist(&sprite[ps[p].i], &sprite[j]);
				sa = getangle(sprite[j].x - sx, sprite[j].y - sy);
			}
			else
				zvel = (100 - ps[p].gethorizdiff()) * 98;
		}
		else
		{
			j = findplayer(s, &x);
			//                sa = getangle(ps[j].oposx-sx,ps[j].oposy-sy);
			if (s->picnum == VIXEN)
				sa -= (krand() & 16);
			else
				sa += 16 - (krand() & 31);
			zvel = (((ps[j].oposz - sz + (3 << 8))) * vel) / ldist(&sprite[ps[j].i], s);
		}

		oldzvel = zvel;
		sizx = 18; sizy = 18;

		if (p >= 0) sizx = 7, sizy = 7;
		else sizx = 8, sizy = 8;

		while (scount > 0)
		{
			j = EGS(sect, sx, sy, sz, atwith, -127, sizx, sizy, sa, vel, zvel, i, 4);
			sprite[j].extra += (krand() & 7);

			sprite[j].cstat = 128;
			sprite[j].clipdist = 4;

			sa = s->ang + 32 - (krand() & 63);
			zvel = oldzvel + 512 - (krand() & 1023);

			scount--;
		}

		return;

	case FIRELASER:
	case SPIT:
	case COOLEXPLOSION1:

		if (isRRRA())
		{
			if (atwith != SPIT && s->extra >= 0) s->shade = -96;

			scount = 1;
			if (atwith == SPIT)
			{
				if (s->picnum == 8705)
					vel = 600;
				else
					vel = 400;
			}
		}
		else
		{
			if (s->extra >= 0) s->shade = -96;

			scount = 1;
			if (atwith == SPIT) vel = 400;
		}
		if (atwith != SPIT)
		{
			vel = 840;
			sz -= (4 << 7);
			if (s->picnum == 4649)
			{
				sx += sintable[(s->ang + 512 + 256) & 2047] >> 6;
				sy += sintable[(s->ang + 256) & 2047] >> 6;
				sz += (12 << 8);
			}
			if (s->picnum == VIXEN)
			{
				sz -= (12 << 8);
			}
		}

		if (p >= 0)
		{
			j = aim(s, AUTO_AIM_ANGLE);

			if (j >= 0)
			{
				sx += sintable[(s->ang + 512 + 160) & 2047] >> 7;
				sy += sintable[(s->ang + 160) & 2047] >> 7;
				dal = ((sprite[j].xrepeat * tilesiz[sprite[j].picnum].y) << 1) - (12 << 8);
				zvel = ((sprite[j].z - sz - dal) * vel) / ldist(&sprite[ps[p].i], &sprite[j]);
				sa = getangle(sprite[j].x - sx, sprite[j].y - sy);
			}
			else
			{
				sx += sintable[(s->ang + 512 + 160) & 2047] >> 7;
				sy += sintable[(s->ang + 160) & 2047] >> 7;
				zvel = (100 - ps[p].gethorizdiff()) * 98;
			}
		}
		else
		{
			j = findplayer(s, &x);
			//                sa = getangle(ps[j].oposx-sx,ps[j].oposy-sy);
			if (s->picnum == HULK)
				sa -= (krand() & 31);
			else if (s->picnum == VIXEN)
				sa -= (krand() & 16);
			else if (s->picnum != UFOBEAM)
				sa += 16 - (krand() & 31);

			zvel = (((ps[j].oposz - sz + (3 << 8))) * vel) / ldist(&sprite[ps[j].i], s);
		}

		oldzvel = zvel;

		if (atwith == SPIT) 
		{
			sizx = 18; sizy = 18;
			if (!isRRRA() || s->picnum != MAMA) sz -= (10 << 8); else sz -= (20 << 8);
		}
		else
		{
			if (atwith == COOLEXPLOSION1)
			{
				sizx = 8;
				sizy = 8;
			}
			else if (atwith == FIRELASER)
			{
				if (p >= 0)
				{

					sizx = 34;
					sizy = 34;
				}
				else
				{
					sizx = 18;
					sizy = 18;
				}
			}
			else
			{
				sizx = 18;
				sizy = 18;
			}
		}

		if (p >= 0) sizx = 7, sizy = 7;

		while (scount > 0)
		{
			j = EGS(sect, sx, sy, sz, atwith, -127, sizx, sizy, sa, vel, zvel, i, 4);
			sprite[j].extra += (krand() & 7);
			sprite[j].cstat = 128;
			sprite[j].clipdist = 4;

			sa = s->ang + 32 - (krand() & 63);
			zvel = oldzvel + 512 - (krand() & 1023);

			if (atwith == FIRELASER)
			{
				sprite[j].xrepeat = 8;
				sprite[j].yrepeat = 8;
			}

			scount--;
		}

		return;

	case RPG2:
	case RRTILE1790:
		if (isRRRA()) goto rrra_rpg2;
		else break;

	case FREEZEBLAST:
		sz += (3 << 8);
	case RPG:
	case SHRINKSPARK:
	rrra_rpg2:
	{
		short var90 = 0;
		if (s->extra >= 0) s->shade = -96;

		scount = 1;
		vel = 644;

		j = -1;

		if (p >= 0)
		{
			j = aim(s, 48);
			if (j >= 0)
			{
				if (isRRRA() && atwith == RPG2)
				{
					if (sprite[j].picnum == HEN || sprite[j].picnum == HENSTAYPUT)
						var90 = ps[screenpeek].i;
					else
						var90 = j;
				}
				dal = ((sprite[j].xrepeat * tilesiz[sprite[j].picnum].y) << 1) + (8 << 8);
				zvel = ((sprite[j].z - sz - dal) * vel) / ldist(&sprite[ps[p].i], &sprite[j]);
				if (sprite[j].picnum != RECON)
					sa = getangle(sprite[j].x - sx, sprite[j].y - sy);
			}
			else zvel = (100 - ps[p].gethorizdiff()) * 81;
			if (atwith == RPG)
				spritesound(RPG_SHOOT, i);
			else if (isRRRA())
			{
				if (atwith == RPG2)
					spritesound(244, i);
				else if (atwith == RRTILE1790)
					spritesound(94, i);
			}

		}
		else
		{
			j = findplayer(s, &x);
			sa = getangle(ps[j].oposx - sx, ps[j].oposy - sy);
			if (s->picnum == BOSS3)
				sz -= (32 << 8);
			else if (s->picnum == BOSS2)
			{
				vel += 128;
				sz += 24 << 8;
			}

			l = ldist(&sprite[ps[j].i], s);
			zvel = ((ps[j].oposz - sz) * vel) / l;

			if (badguy(s) && (s->hitag & face_player_smart))
				sa = s->ang + (krand() & 31) - 16;
		}

		if (p >= 0 && j >= 0)
			l = j;
		else l = -1;

		if (isRRRA() && atwith == RRTILE1790)
		{
			zvel = -(10 << 8);
			vel <<= 1;
		}

		j = EGS(sect,
			sx + (sintable[(348 + sa + 512) & 2047] / 448),
			sy + (sintable[(sa + 348) & 2047] / 448),
			sz - (1 << 8), atwith, 0, 14, 14, sa, vel, zvel, i, 4);

		if (isRRRA())
		{
			if (atwith == RRTILE1790)
			{
				sprite[j].extra = 10;
				sprite[j].zvel = -(10 << 8);
			}
			else if (atwith == RPG2)
			{
				sprite[j].lotag = var90;
				sprite[j].hitag = 0;
				fi.lotsofmoney(&sprite[j], (krand() & 3) + 1);
			}
		}

		sprite[j].extra += (krand() & 7);
		if (atwith != FREEZEBLAST)
			sprite[j].yvel = l;
		else
		{
			sprite[j].yvel = numfreezebounces;
			sprite[j].xrepeat >>= 1;
			sprite[j].yrepeat >>= 1;
			sprite[j].zvel -= (2 << 4);
		}

		if (p == -1)
		{
			if (s->picnum == HULK)
			{
				sprite[j].xrepeat = 8;
				sprite[j].yrepeat = 8;
			}
			else if (atwith != FREEZEBLAST)
			{
				sprite[j].xrepeat = 30;
				sprite[j].yrepeat = 30;
				sprite[j].extra >>= 2;
			}
		}
		else if (ps[p].curr_weapon == TIT_WEAPON)
		{
			sprite[j].extra >>= 2;
			sprite[j].ang += 16 - (krand() & 31);
			sprite[j].zvel += 256 - (krand() & 511);

			if (ps[p].hbomb_hold_delay)
			{
				sprite[j].x -= sintable[sa & 2047] / 644;
				sprite[j].y -= sintable[(sa + 1024 + 512) & 2047] / 644;
			}
			else
			{
				sprite[j].x += sintable[sa & 2047] >> 8;
				sprite[j].y += sintable[(sa + 1024 + 512) & 2047] >> 8;
			}
			sprite[j].xrepeat >>= 1;
			sprite[j].yrepeat >>= 1;
		}

		sprite[j].cstat = 128;
		if (atwith == RPG || (atwith == RPG2 && isRRRA()))
			sprite[j].clipdist = 4;
		else
			sprite[j].clipdist = 40;

	}
	break;

	case CHEERBOMB:
		if (!isRRRA()) break;
	case MORTER:

		if (s->extra >= 0) s->shade = -96;

		j = ps[findplayer(s, &x)].i;
		x = ldist(&sprite[j], s);

		zvel = -x >> 1;

		if (zvel < -4096)
			zvel = -2048;
		vel = x >> 4;

		if (atwith == CHEERBOMB)
			EGS(sect,
				sx + (sintable[(512 + sa + 512) & 2047] >> 8),
				sy + (sintable[(sa + 512) & 2047] >> 8),
				sz + (6 << 8), atwith, -64, 16, 16, sa, vel, zvel, i, 1);
		else
			EGS(sect,
				sx + (sintable[(512 + sa + 512) & 2047] >> 8),
				sy + (sintable[(sa + 512) & 2047] >> 8),
				sz + (6 << 8), atwith, -64, 32, 32, sa, vel, zvel, i, 1);
		break;
	}
	return;
}

//---------------------------------------------------------------------------
//
// this is one lousy hack job...
//
//---------------------------------------------------------------------------

void selectweapon_r(int snum, int j)
{
	int i, k;
	auto p = &ps[snum];
	if (p->last_pissed_time <= (26 * 218) && p->show_empty_weapon == 0 && p->kickback_pic == 0 && p->quick_kick == 0 && sprite[p->i].xrepeat > 8 && p->access_incs == 0 && p->knee_incs == 0)
	{
		if ((p->weapon_pos == 0 || (p->holster_weapon && p->weapon_pos == -9)))
		{
			if (j == 10 || j == 11)
			{
				k = p->curr_weapon;
				if (isRRRA())
				{
					if (k == CHICKEN_WEAPON) k = CROSSBOW_WEAPON;
					else if (k == BUZZSAW_WEAPON) k = THROWSAW_WEAPON;
					else if (k == SLINGBLADE_WEAPON) k = KNEE_WEAPON;
				}
				j = (j == 10 ? -1 : 1);
				i = 0;

				while (k >= 0 && k < 10)
				{
					k += j;
					if (k == -1) k = 9;
					else if (k == 10) k = 0;

					if (p->gotweapon[k] && p->ammo_amount[k] > 0)
					{
						j = k;
						break;
					}

					i++;
					if (i == 10)
					{
						fi.addweapon(p, KNEE_WEAPON);
						break;
					}
				}
			}

			k = -1;


			if (j == DYNAMITE_WEAPON && p->ammo_amount[DYNAMITE_WEAPON] == 0)
			{
				k = headspritestat[1];
				while (k >= 0)
				{
					if (sprite[k].picnum == HEAVYHBOMB && sprite[k].owner == p->i)
					{
						p->gotweapon.Set(DYNAMITE_WEAPON);
						j = HANDREMOTE_WEAPON;
						break;
					}
					k = nextspritestat[k];
				}
			}
			else if (j == KNEE_WEAPON && isRRRA())
			{
				if (p->curr_weapon == KNEE_WEAPON)
				{
					p->subweapon = 2;
					j = SLINGBLADE_WEAPON;
				}
				else if (p->subweapon & 2)
				{
					p->subweapon = 0;
					j = KNEE_WEAPON;
				}
			}
			else if (j == CROSSBOW_WEAPON && isRRRA())
			{
				if (screenpeek == snum) pus = NUMPAGES;

				if (p->curr_weapon == CROSSBOW_WEAPON || p->ammo_amount[CROSSBOW_WEAPON] == 0)
				{
					if (p->ammo_amount[CHICKEN_WEAPON] == 0)
						return;
					p->subweapon = 4;
					j = CHICKEN_WEAPON;
				}
				else if ((p->subweapon & 4) || p->ammo_amount[CHICKEN_WEAPON] == 0)
				{
					p->subweapon = 0;
					j = CROSSBOW_WEAPON;
				}
			}
			else if (j == THROWSAW_WEAPON)
			{
				if (screenpeek == snum) pus = NUMPAGES;

				if (p->curr_weapon == THROWSAW_WEAPON || p->ammo_amount[THROWSAW_WEAPON] == 0)
				{
					p->subweapon = (1 << BUZZSAW_WEAPON);
					j = BUZZSAW_WEAPON;
				}
				else if ((p->subweapon & (1 << BUZZSAW_WEAPON)) || p->ammo_amount[BUZZSAW_WEAPON] == 0)
				{
					p->subweapon = 0;
					j = THROWSAW_WEAPON;
				}
			}
			else if (j == POWDERKEG_WEAPON)
			{
				if (screenpeek == snum) pus = NUMPAGES;

				if (p->curr_weapon == POWDERKEG_WEAPON || p->ammo_amount[POWDERKEG_WEAPON] == 0)
				{
					p->subweapon = (1 << BOWLING_WEAPON);
					j = BOWLING_WEAPON;
				}
				else if ((p->subweapon & (1 << BOWLING_WEAPON)) || p->ammo_amount[BOWLING_WEAPON] == 0)
				{
					p->subweapon = 0;
					j = POWDERKEG_WEAPON;
				}
			}


			if (p->holster_weapon)
			{
				PlayerSetInput(snum, SK_HOLSTER);
				p->weapon_pos = -9;
			}
			else if (j >= MIN_WEAPON && p->gotweapon[j] && p->curr_weapon != j) switch (j)
			{
			case KNEE_WEAPON:
				fi.addweapon(p, j);
				break;
			case SLINGBLADE_WEAPON:
				if (isRRRA())
				{
					spritesound(496, ps[screenpeek].i);
					fi.addweapon(p, j);
				}
				break;

			case PISTOL_WEAPON:
				if (p->ammo_amount[PISTOL_WEAPON] == 0)
					if (p->show_empty_weapon == 0)
					{
						p->last_full_weapon = p->curr_weapon;
						p->show_empty_weapon = 32;
					}
				fi.addweapon(p, PISTOL_WEAPON);
				break;

			case CHICKEN_WEAPON:
				if (!isRRRA()) break;
			case SHOTGUN_WEAPON:
			case RIFLEGUN_WEAPON:
			case CROSSBOW_WEAPON:
			case TIT_WEAPON:
			case ALIENBLASTER_WEAPON:
			case THROWSAW_WEAPON:
			case BUZZSAW_WEAPON:
			case POWDERKEG_WEAPON:
			case BOWLING_WEAPON:
				if (p->ammo_amount[j] == 0 && p->show_empty_weapon == 0)
				{
					p->last_full_weapon = p->curr_weapon;
					p->show_empty_weapon = 32;
				}
				fi.addweapon(p, j);
				break;

			case MOTORCYCLE_WEAPON:
			case BOAT_WEAPON:
				if (isRRRA())
				{
					if (p->ammo_amount[j] == 0 && p->show_empty_weapon == 0)
					{
						p->show_empty_weapon = 32;
					}
					fi.addweapon(p, j);
				}
				break;

			case HANDREMOTE_WEAPON:	// what's up with this? RR doesn't define this weapon.
				if (k >= 0) // Found in list of [1]'s
				{
					p->curr_weapon = HANDREMOTE_WEAPON;
					p->last_weapon = -1;
					p->weapon_pos = 10;
				}
				break;
			case DYNAMITE_WEAPON:
				if (p->ammo_amount[DYNAMITE_WEAPON] > 0 && p->gotweapon[DYNAMITE_WEAPON])
					fi.addweapon(p, DYNAMITE_WEAPON);
				break;
			}
		}
	}
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

int doincrements_r(struct player_struct* p)
{
	int snum;

	if (isRRRA())
	{
		if (WindTime > 0)
			WindTime--;
		else if ((krand() & 127) == 8)
		{
			WindTime = 120 + ((krand() & 63) << 2);
			WindDir = krand() & 2047;
		}

		if (BellTime > 0)
		{
			BellTime--;
			if (BellTime == 0)
				sprite[word_119BE0].picnum++;
		}
		if (chickenphase > 0)
			chickenphase--;
		if (p->SeaSick)
		{
			p->SeaSick--;
			if (p->SeaSick == 0)
				p->sea_sick_stat = 0;
		}
	}

	snum = sprite[p->i].yvel;
	//    j = sync[snum].avel;
	//    p->weapon_ang = -(j/5);

	p->player_par++;
	if (p->yehaa_timer)
		p->yehaa_timer--;


	if (p->detonate_count > 0)
	{
		p->detonate_count++;
		p->detonate_time--;
	}
	p->drink_timer--;
	if (p->drink_timer <= 0)
	{
		p->drink_timer = 1024;
		if (p->drink_amt)
		{
			p->drink_amt--;
		}
	}
	p->eat_timer--;
	if (p->eat_timer <= 0)
	{
		p->eat_timer = 1024;
		if (p->eat)
			p->eat--;
	}
	if (p->drink_amt >= 100)
	{
		if (!A_CheckSoundPlaying(p->i, 420))
			spritesound(420, p->i);
		p->drink_amt -= 9;
		p->eat >>= 1;
	}
	p->eatang = (1647 + p->eat * 8) & 2047;

	if (p->eat >= 100)
		p->eat = 100;

	if (p->eat >= 31 && krand() < p->eat)
	{
		switch (krand() & 3)
		{
		case 0:
			spritesound(404, p->i);
			break;
		case 1:
			spritesound(422, p->i);
			break;
		case 2:
			spritesound(423, p->i);
			break;
		case 3:
			spritesound(424, p->i);
			break;
		}
		if (numplayers < 2)
		{
			p->noise_radius = 16384;
			madenoise(screenpeek);
			p->posxv += sintable[(p->getang() + 512) & 2047] << 4;
			p->posyv += sintable[p->getang() & 2047] << 4;
		}
		p->eat -= 4;
		if (p->eat < 0)
			p->eat = 0;
	}

	if (p->invdisptime > 0)
		p->invdisptime--;

	if (p->tipincs > 0) p->tipincs--;

	if (p->last_pissed_time > 0)
	{
		p->last_pissed_time--;

		if (p->drink_amt > 66 && (p->last_pissed_time % 26) == 0)
			p->drink_amt--;

		if (ud.lockout == 0)
		{
			if (p->last_pissed_time == 5662)
				spritesound(434, p->i);
			else if (p->last_pissed_time == 5567)
				spritesound(434, p->i);
			else if (p->last_pissed_time == 5472)
				spritesound(433, p->i);
			else if (p->last_pissed_time == 5072)
				spritesound(435, p->i);
			else if (p->last_pissed_time == 5014)
				spritesound(434, p->i);
			else if (p->last_pissed_time == 4919)
				spritesound(433, p->i);
		}

		if (p->last_pissed_time == 5668)
		{
			p->holster_weapon = 0;
			p->weapon_pos = 10;
		}
	}

	if (p->crack_time > 0)
	{
		p->crack_time--;
		if (p->crack_time == 0)
		{
			p->knuckle_incs = 1;
			p->crack_time = 777;
		}
	}

	if (p->steroids_amount > 0 && p->steroids_amount < 400)
	{
		p->steroids_amount--;
		if (p->steroids_amount == 0)
		{
			checkavailinven(p);
			p->eat = p->drink_amt = 0;
			p->eatang = p->drunkang = 1647;
		}
		if (!(p->steroids_amount & 14))
			if (snum == screenpeek || ud.coop == 1)
				spritesound(DUKE_TAKEPILLS, p->i);
	}

	if (p->access_incs && sprite[p->i].pal != 1)
	{
		p->access_incs++;
		if (sprite[p->i].extra <= 0)
			p->access_incs = 12;
		if (p->access_incs == 12)
		{
			if (p->access_spritenum >= 0)
			{
				fi.checkhitswitch(snum, p->access_spritenum, 1);
				switch (sprite[p->access_spritenum].pal)
				{
				case 0:p->keys[1] = 1; break;
				case 21:p->keys[2] = 1; break;
				case 23:p->keys[3] = 1; break;
				}
				p->access_spritenum = -1;
			}
			else
			{
				fi.checkhitswitch(snum, p->access_wallnum, 0);
				switch (wall[p->access_wallnum].pal)
				{
				case 0:p->keys[1] = 1; break;
				case 21:p->keys[2] = 1; break;
				case 23:p->keys[3] = 1; break;
				}
			}
		}

		if (p->access_incs > 20)
		{
			p->access_incs = 0;
			p->weapon_pos = 10;
			p->kickback_pic = 0;
		}
	}

	if (p->scuba_on == 0 && sector[p->cursectnum].lotag == 2)
	{
		if (p->scuba_amount > 0)
		{
			p->scuba_on = 1;
			p->inven_icon = 6;
			FTA(76, p);
		}
		else
		{
			if (p->airleft > 0)
				p->airleft--;
			else
			{
				p->extra_extra8 += 32;
				if (p->last_extra < (max_player_health >> 1) && (p->last_extra & 3) == 0)
					spritesound(DUKE_LONGTERM_PAIN, p->i);
			}
		}
	}
	else if (p->scuba_amount > 0 && p->scuba_on)
	{
		p->scuba_amount--;
		if (p->scuba_amount == 0)
		{
			p->scuba_on = 0;
			checkavailinven(p);
		}
	}

	if (p->knuckle_incs)
	{
		p->knuckle_incs++;
		if (p->knuckle_incs == 10)
		{
			if (!wupass)
			{
				short snd = -1;
				wupass = 1;
				if (lastlevel)
				{
					snd = 391;
				}
				else switch (ud.volume_number)
				{
				case 0:
					switch (ud.level_number)
					{
					case 0:
						snd = isRRRA()? 63 : 391;
						break;
					case 1:
						snd = 64;
						break;
					case 2:
						snd = 77;
						break;
					case 3:
						snd = 80;
						break;
					case 4:
						snd = 102;
						break;
					case 5:
						snd = 103;
						break;
					case 6:
						snd = 104;
						break;
					}
					break;
				case 1:
					switch (ud.level_number)
					{
					case 0:
						snd = 105;
						break;
					case 1:
						snd = 176;
						break;
					case 2:
						snd = 177;
						break;
					case 3:
						snd = 198;
						break;
					case 4:
						snd = 230;
						break;
					case 5:
						snd = 255;
						break;
					case 6:
						snd = 283;
						break;
					}
					break;
				}
				if (snd == -1)
					snd = 391;
				spritesound(snd, p->i);
			}
			else if (totalclock > 1024)
				if (snum == screenpeek || ud.coop == 1)
				{
					if (rand() & 1)
						spritesound(DUKE_CRACK, p->i);
					else spritesound(DUKE_CRACK2, p->i);
				}
		}
		else if (p->knuckle_incs == 22 || PlayerInput(snum, SK_FIRE))
			p->knuckle_incs = 0;

		return 1;
	}
	return 0;
}


END_DUKE_NS
