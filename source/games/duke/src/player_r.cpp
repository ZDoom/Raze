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
		else if (ps[p].curr_weapon == DEVISTATOR_WEAPON)
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


END_DUKE_NS
