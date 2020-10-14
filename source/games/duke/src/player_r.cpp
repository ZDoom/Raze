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

BEGIN_DUKE_NS 


//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void incur_damage_r(struct player_struct* p)
{
	int  damage = 0, unk = 0, shield_damage = 0;
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
		sa = ps[p].angle.ang.asbuild();

		if (isRRRA()) ps[p].crack_time = CRACK_TIME;
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

	SetGameVarID(g_iAtWithVarID, 0, p, atwith);
	SetGameVarID(g_iReturnVarID, 0, p, i);
	OnEvent(EVENT_SHOOT, i, p, -1);
	if (GetGameVarID(g_iReturnVarID, p, i) != 0)
	{
		return;
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
				zvel = -ps[p].horizon.sum().asq16() >> 11;
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
								SectIterator it(wall[hitwall].nextsector);
								while ((k = it.NextIndex()) >= 0)
								{
									if (sprite[k].statnum == 3 && sprite[k].lotag == 13)
										return;
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
						S_PlayActorSound(KICK_HIT, j);
					else if (isRRRA() && atwith == SLINGBLADE)
						S_PlayActorSound(260, j);
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
				sprite[j].ang = ps[p].angle.ang.asbuild(); // Total tweek
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
					zvel = -ps[p].horizon.sum().asq16() >> 11;
					zvel += 128 - (krand() & 255);
				}
			}
			else
			{
				if (atwith == SHOTGUN)
					sa += 64 - (krand() & 127);
				else
					sa += 16 - (krand() & 31);
				if (j == -1) zvel = -ps[p].horizon.sum().asq16() >> 11;
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
									SectIterator it(wall[hitwall].nextsector);
									while ((l = it.NextIndex()) >= 0)
									{
										if (sprite[l].statnum == 3 && sprite[l].lotag == 13)
											goto SKIPBULLETHOLE;
									}
								}

								StatIterator it(STAT_MISC);
								while ((l = it.NextIndex()) >= 0)
								{
									if (sprite[l].picnum == BULLETHOLE)
										if (dist(&sprite[l], &sprite[k]) < (12 + (krand() & 7)))
											goto SKIPBULLETHOLE;
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
				zvel = -mulscale16(ps[p].horizon.sum().asq16(), 98);
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
				zvel = -mulscale16(ps[p].horizon.sum().asq16(), 98);
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
			else zvel = -mulscale16(ps[p].horizon.sum().asq16(), 81);
			if (atwith == RPG)
				S_PlayActorSound(RPG_SHOOT, i);
			else if (isRRRA())
			{
				if (atwith == RPG2)
					S_PlayActorSound(244, i);
				else if (atwith == RRTILE1790)
					S_PlayActorSound(94, i);
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

void selectweapon_r(int snum, int weap)
{
	int i, j, k;
	auto p = &ps[snum];
	if (p->last_pissed_time <= (26 * 218) && p->show_empty_weapon == 0 && p->kickback_pic == 0 && p->quick_kick == 0 && sprite[p->i].xrepeat > 8 && p->access_incs == 0 && p->knee_incs == 0)
	{
		if ((p->weapon_pos == 0 || (p->holster_weapon && p->weapon_pos == -9)))
		{
			if (weap == WeaponSel_Alt)
			{
				switch (p->curr_weapon)
				{
					case THROWSAW_WEAPON:
						j = BUZZSAW_WEAPON;
						break;
					case BUZZSAW_WEAPON:
						j = THROWSAW_WEAPON;
						break;
					case POWDERKEG_WEAPON:
						j = BOWLING_WEAPON;
						break;
					case BOWLING_WEAPON:
						j = POWDERKEG_WEAPON;
						break;
					case KNEE_WEAPON:
						j = isRRRA() ? SLINGBLADE_WEAPON : p->curr_weapon;
						break;
					case SLINGBLADE_WEAPON:
						j = KNEE_WEAPON;
						break;
					case DYNAMITE_WEAPON:
						j = isRRRA() ? CHICKEN_WEAPON : p->curr_weapon;
						break;
					case CHICKEN_WEAPON:
						j = DYNAMITE_WEAPON;
						break;
					default:
						j = p->curr_weapon;
						break;
				}
			}
			else if (weap == WeaponSel_Next || weap == WeaponSel_Prev)
			{
				k = p->curr_weapon;
				if (isRRRA())
				{
					if (k == CHICKEN_WEAPON) k = CROSSBOW_WEAPON;
					else if (k == BUZZSAW_WEAPON) k = THROWSAW_WEAPON;
					else if (k == SLINGBLADE_WEAPON) k = KNEE_WEAPON;
				}
				j = (weap == WeaponSel_Prev ? -1 : 1);	// JBF: prev (-1) or next (1) weapon choice
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
			else j = weap - 1;

			k = -1;

			if (j == DYNAMITE_WEAPON && p->ammo_amount[DYNAMITE_WEAPON] == 0)
			{
				StatIterator it(STAT_ACTOR);
				while ((k = it.NextIndex()) >= 0)
				{
					if (sprite[k].picnum == HEAVYHBOMB && sprite[k].owner == p->i)
					{
						p->gotweapon.Set(DYNAMITE_WEAPON);
						j = THROWINGDYNAMITE_WEAPON;
						break;
					}
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
				PlayerSetInput(snum, SB_HOLSTER);
				p->oweapon_pos = p->weapon_pos = -9;
			}
			else if (j >= MIN_WEAPON && p->gotweapon[j] && p->curr_weapon != j) switch (j)
			{
			case KNEE_WEAPON:
				fi.addweapon(p, j);
				break;
			case SLINGBLADE_WEAPON:
				if (isRRRA())
				{
					S_PlayActorSound(496, ps[screenpeek].i);
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

			case THROWINGDYNAMITE_WEAPON:
				if (k >= 0) // Found in list of [1]'s
				{
					p->curr_weapon = THROWINGDYNAMITE_WEAPON;
					p->last_weapon = -1;
					p->oweapon_pos = p->weapon_pos = 10;
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
				sprite[BellSprite].picnum++;
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
		if (!S_CheckActorSoundPlaying(p->i, 420))
			S_PlayActorSound(420, p->i);
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
			S_PlayActorSound(404, p->i);
			break;
		case 1:
			S_PlayActorSound(422, p->i);
			break;
		case 2:
			S_PlayActorSound(423, p->i);
			break;
		case 3:
			S_PlayActorSound(424, p->i);
			break;
		}
		if (numplayers < 2)
		{
			p->noise_radius = 16384;
			madenoise(screenpeek);
			p->posxv += sintable[(p->angle.ang.asbuild() + 512) & 2047] << 4;
			p->posyv += sintable[p->angle.ang.asbuild() & 2047] << 4;
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

		{
			if (p->last_pissed_time == 5662)
				S_PlayActorSound(434, p->i);
			else if (p->last_pissed_time == 5567)
				S_PlayActorSound(434, p->i);
			else if (p->last_pissed_time == 5472)
				S_PlayActorSound(433, p->i);
			else if (p->last_pissed_time == 5072)
				S_PlayActorSound(435, p->i);
			else if (p->last_pissed_time == 5014)
				S_PlayActorSound(434, p->i);
			else if (p->last_pissed_time == 4919)
				S_PlayActorSound(433, p->i);
		}

		if (p->last_pissed_time == 5668)
		{
			p->holster_weapon = 0;
			p->oweapon_pos = p->weapon_pos = 10;
		}
	}

	if (p->crack_time > 0)
	{
		p->crack_time--;
		if (p->crack_time == 0)
		{
			p->knuckle_incs = 1;
			p->crack_time = CRACK_TIME;
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
				S_PlayActorSound(DUKE_TAKEPILLS, p->i);
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
			p->oweapon_pos = p->weapon_pos = 10;
			p->okickback_pic = p->kickback_pic = 0;
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
					S_PlayActorSound(DUKE_LONGTERM_PAIN, p->i);
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
				short snd;
				wupass = 1;
				switch (currentLevel->levelNumber)
				{
				default: snd = 391; break;
				case levelnum(0, 0): snd = isRRRA() ? 63 : 391; break;
				case levelnum(0, 1): snd = 64; break;
				case levelnum(0, 2): snd = 77; break;
				case levelnum(0, 3): snd = 80; break;
				case levelnum(0, 4): snd = 102; break;
				case levelnum(0, 5): snd = 103; break;
				case levelnum(0, 6): snd = 104; break;
				case levelnum(1, 0): snd = 105; break;
				case levelnum(1, 1): snd = 176; break;
				case levelnum(1, 2): snd = 177; break;
				case levelnum(1, 3): snd = 198; break;
				case levelnum(1, 4): snd = 230; break;
				case levelnum(1, 5): snd = 255; break;
				case levelnum(1, 6): snd = 283; break;
				}
				S_PlayActorSound(snd, p->i);
			}
			else if (ud.levelclock > 1024)
				if (snum == screenpeek || ud.coop == 1)
				{
					if (rand() & 1)
						S_PlayActorSound(DUKE_CRACK, p->i);
					else S_PlayActorSound(DUKE_CRACK2, p->i);
				}
		}
		else if (p->knuckle_incs == 22 || PlayerInput(snum, SB_FIRE))
			p->knuckle_incs = 0;

		return 1;
	}
	return 0;
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void checkweapons_r(struct player_struct* p)
{
	static const short weapon_sprites[MAX_WEAPONS] = { KNEE, FIRSTGUNSPRITE, SHOTGUNSPRITE,
			CHAINGUNSPRITE, RPGSPRITE, HEAVYHBOMB, SHRINKERSPRITE, DEVISTATORSPRITE,
			TRIPBOMBSPRITE, BOWLINGBALLSPRITE, FREEZEBLAST, HEAVYHBOMB };
	short i, j;

	if (isRRRA())
	{
		if (p->OnMotorcycle && numplayers > 1)
		{
			j = fi.spawn(p->i, 7220);
			sprite[j].ang = p->angle.ang.asbuild();
			sprite[j].owner = p->ammo_amount[MOTORCYCLE_WEAPON];
			p->OnMotorcycle = 0;
			p->gotweapon.Clear(MOTORCYCLE_WEAPON);
			p->horizon.horiz = q16horiz(0);
			p->moto_do_bump = 0;
			p->MotoSpeed = 0;
			p->TiltStatus = 0;
			p->moto_drink = 0;
			p->VBumpTarget = 0;
			p->VBumpNow = 0;
			p->TurbCount = 0;
		}
		else if (p->OnBoat && numplayers > 1)
		{
			j = fi.spawn(p->i, 7233);
			sprite[j].ang = p->angle.ang.asbuild();
			sprite[j].owner = p->ammo_amount[BOAT_WEAPON];
			p->OnBoat = 0;
			p->gotweapon.Clear(BOAT_WEAPON);
			p->horizon.horiz = q16horiz(0);
			p->moto_do_bump = 0;
			p->MotoSpeed = 0;
			p->TiltStatus = 0;
			p->moto_drink = 0;
			p->VBumpTarget = 0;
			p->VBumpNow = 0;
			p->TurbCount = 0;
		}
	}

	if (p->curr_weapon > 0)
	{
		if (krand() & 1)
			fi.spawn(p->i, weapon_sprites[p->curr_weapon]);
		else switch (p->curr_weapon)
		{
		case CHICKEN_WEAPON:
			if (!isRRRA()) break;
		case DYNAMITE_WEAPON:
		case CROSSBOW_WEAPON:
			fi.spawn(p->i, EXPLOSION2);
			break;
		}
	}

	for (i = 0; i < 5; i++)
	{
		if (p->keys[i] == 1)
		{
			j = fi.spawn(p->i, ACCESSCARD);
			switch (i)
			{
			case 1:
				sprite[j].lotag = 100;
				break;
			case 2:
				sprite[j].lotag = 101;
				break;
			case 3:
				sprite[j].lotag = 102;
				break;
			case 4:
				sprite[j].lotag = 103;
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

static void onMotorcycle(int snum, ESyncBits &actions)
{
	auto p = &ps[snum];
	auto pi = p->i;
	auto s = &sprite[pi];

	int var64, var68, var6c, var70, var74, var78, var7c, var80;
	short var84;
	if (p->MotoSpeed < 0)
		p->MotoSpeed = 0;
	if (actions & SB_CROUCH)
	{
		var64 = 1;
		actions &= ~SB_CROUCH;
	}
	else
		var64 = 0;

	if (actions & SB_JUMP)
	{
		var68 = 1;
		actions &= ~SB_JUMP;
		if (p->on_ground)
		{
			if (p->MotoSpeed == 0 && var64)
			{
				if (!S_CheckActorSoundPlaying(pi, 187))
					S_PlayActorSound(187, pi);
			}
			else if (p->MotoSpeed == 0 && !S_CheckActorSoundPlaying(pi, 214))
			{
				if (S_CheckActorSoundPlaying(pi, 187))
					S_StopSound(187, pi);
				S_PlayActorSound(214, pi);
			}
			else if (p->MotoSpeed >= 50 && !S_CheckActorSoundPlaying(pi, 188))
			{
				S_PlayActorSound(188, pi);
			}
			else if (!S_CheckActorSoundPlaying(pi, 188) && !S_CheckActorSoundPlaying(pi, 214))
			{
				S_PlayActorSound(188, pi);
			}
		}
	}
	else
	{
		var68 = 0;
		if (S_CheckActorSoundPlaying(pi, 214))
		{
			S_StopSound(214, pi);
			if (!S_CheckActorSoundPlaying(pi, 189))
				S_PlayActorSound(189, pi);
		}
		if (S_CheckActorSoundPlaying(pi, 188))
		{
			S_StopSound(188, pi);
			if (!S_CheckActorSoundPlaying(pi, 189))
				S_PlayActorSound(189, pi);
		}
		if (!S_CheckActorSoundPlaying(pi, 189) && !S_CheckActorSoundPlaying(pi, 187))
			S_PlayActorSound(187, pi);
	}
	if (p->vehicle_backwards)
	{
		var6c = 1;
		p->vehicle_backwards = false;
	}
	else
		var6c = 0;
	if (p->vehicle_turnl)
	{
		var70 = 1;
		var74 = 1;
		p->vehicle_turnl = false;
	}
	else
	{
		var70 = 0;
		var74 = 0;
	}
	if (p->vehicle_turnr)
	{
		var78 = 1;
		var7c = 1;
		p->vehicle_turnr = false;
	}
	else
	{
		var78 = 0;
		var7c = 0;
	}
	var80 = 0;
	if (p->drink_amt > 88 && p->moto_drink == 0)
	{
		var84 = krand() & 63;
		if (var84 == 1)
			p->moto_drink = -10;
		else if (var84 == 2)
			p->moto_drink = 10;
	}
	else if (p->drink_amt > 99 && p->moto_drink == 0)
	{
		var84 = krand() & 31;
		if (var84 == 1)
			p->moto_drink = -20;
		else if (var84 == 2)
			p->moto_drink = 20;
	}
	if (p->on_ground == 1)
	{
		if (var64 && p->MotoSpeed > 0)
		{
			if (p->moto_on_oil)
				p->MotoSpeed -= 2;
			else
				p->MotoSpeed -= 4;
			if (p->MotoSpeed < 0)
				p->MotoSpeed = 0;
			p->VBumpTarget = -30;
			p->moto_do_bump = 1;
		}
		else if (var68 && !var64)
		{
			if (p->MotoSpeed < 40)
			{
				p->VBumpTarget = 70;
				p->moto_bump_fast = 1;
			}
			p->MotoSpeed += 2;
			if (p->MotoSpeed > 120)
				p->MotoSpeed = 120;
			if (!p->NotOnWater)
				if (p->MotoSpeed > 80)
					p->MotoSpeed = 80;
		}
		else if (p->MotoSpeed > 0)
			p->MotoSpeed--;
		if (p->moto_do_bump && (!var64 || p->MotoSpeed == 0))
		{
			p->VBumpTarget = 0;
			p->moto_do_bump = 0;
		}
		if (var6c && p->MotoSpeed <= 0 && !var64)
		{
			int var88;
			p->MotoSpeed = -15;
			var88 = var7c;
			var7c = var74;
			var74 = var88;
			var80 = 1;
		}
	}
	if (p->MotoSpeed != 0 && p->on_ground == 1)
	{
		if (!p->VBumpNow)
			if ((krand() & 3) == 2)
				p->VBumpTarget = (p->MotoSpeed >> 4) * ((krand() & 7) - 4);
		if (var74 || p->moto_drink < 0)
		{
			if (p->moto_drink < 0)
				p->moto_drink++;
		}
		else if (var7c || p->moto_drink > 0)
		{
			if (p->moto_drink > 0)
				p->moto_drink--;
		}
	}

	double horiz = 0;
	if (p->TurbCount)
	{
		if (p->TurbCount <= 1)
		{
			horiz = 0;
			p->TurbCount = 0;
			p->VBumpTarget = 0;
			p->VBumpNow = 0;
		}
		else
		{
			horiz = ((krand() & 15) - 7);
			p->TurbCount--;
			p->moto_drink = (krand() & 3) - 2;
		}
	}
	else if (p->VBumpTarget > p->VBumpNow)
	{
		if (p->moto_bump_fast)
			p->VBumpNow += 6;
		else
			p->VBumpNow++;
		if (p->VBumpTarget < p->VBumpNow)
			p->VBumpNow = p->VBumpTarget;
		horiz = p->VBumpNow / 3.;
	}
	else if (p->VBumpTarget < p->VBumpNow)
	{
		if (p->moto_bump_fast)
			p->VBumpNow -= 6;
		else
			p->VBumpNow--;
		if (p->VBumpTarget > p->VBumpNow)
			p->VBumpNow = p->VBumpTarget;
		horiz = p->VBumpNow / 3.;
	}
	else
	{
		p->VBumpTarget = 0;
		p->moto_bump_fast = 0;
	}
	if (horiz != 0)
	{
		p->horizon.addadjustment(horiz - FixedToFloat(p->horizon.horiz.asq16()));
	}

	if (p->MotoSpeed >= 20 && p->on_ground == 1 && (var74 || var7c))
	{
		short var8c, var90, var94, var98;
		var8c = p->MotoSpeed;
		var90 = p->angle.ang.asbuild();
		if (var74)
			var94 = -10;
		else
			var94 = 10;
		if (var94 < 0)
			var98 = 350;
		else
			var98 = -350;
		int ang;
		if (p->moto_on_mud || p->moto_on_oil || !p->NotOnWater)
		{
			if (p->moto_on_oil)
				var8c <<= 3;
			else
				var8c <<= 2;
			if (p->moto_do_bump)
			{
				p->posxv += (var8c >> 5) * (sintable[(var94 * -51 + var90 + 512) & 2047] << 4);
				p->posyv += (var8c >> 5) * (sintable[(var94 * -51 + var90) & 2047] << 4);
				ang = var98 >> 2;
			}
			else
			{
				p->posxv += (var8c >> 7) * (sintable[(var94 * -51 + var90 + 512) & 2047] << 4);
				p->posyv += (var8c >> 7) * (sintable[(var94 * -51 + var90) & 2047] << 4);
				ang = var98 >> 6;
			}
			p->moto_on_mud = 0;
			p->moto_on_oil = 0;
		}
		else
		{
			if (p->moto_do_bump)
			{
				p->posxv += (var8c >> 5) * (sintable[(var94 * -51 + var90 + 512) & 2047] << 4);
				p->posyv += (var8c >> 5) * (sintable[(var94 * -51 + var90) & 2047] << 4);
				ang = var98 >> 4;
				if (!S_CheckActorSoundPlaying(pi, 220))
					S_PlayActorSound(220, pi);
			}
			else
			{
				p->posxv += (var8c >> 7) * (sintable[(var94 * -51 + var90 + 512) & 2047] << 4);
				p->posyv += (var8c >> 7) * (sintable[(var94 * -51 + var90) & 2047] << 4);
				ang = var98 >> 7;
			}
		}
		p->angle.addadjustment(FixedToFloat(getincangleq16(p->angle.ang.asq16(), IntToFixed(var90 - ang))));
	}
	else if (p->MotoSpeed >= 20 && p->on_ground == 1 && (p->moto_on_mud || p->moto_on_oil))
	{
		short var9c, vara0, vara4=0;
		var9c = p->MotoSpeed;
		vara0 = p->angle.ang.asbuild();
		var84 = krand() & 1;
		if (var84 == 0)
			vara4 = -10;
		else if (var84 == 1)
			vara4 = 10;
		if (p->moto_on_oil)
			var9c *= 10;
		else
			var9c *= 5;
		p->posxv += (var9c >> 7) * (sintable[(vara4 * -51 + vara0 + 512) & 2047] << 4);
		p->posyv += (var9c >> 7) * (sintable[(vara4 * -51 + vara0) & 2047] << 4);
	}
	p->moto_on_mud = 0;
	p->moto_on_oil = 0;
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

static void onBoat(int snum, ESyncBits &actions)
{
	auto p = &ps[snum];
	auto pi = p->i;
	auto s = &sprite[pi];

	int vara8, varac, varb0, varb4, varb8, varbc, varc0, varc4, varc8;
	short varcc;
	if (p->NotOnWater)
	{
		if (p->MotoSpeed > 0)
		{
			if (!S_CheckActorSoundPlaying(pi, 88))
				S_PlayActorSound(88, pi);
		}
		else
		{
			if (!S_CheckActorSoundPlaying(pi, 87))
				S_PlayActorSound(87, pi);
		}
	}
	if (p->MotoSpeed < 0)
		p->MotoSpeed = 0;
	if ((actions & SB_CROUCH) && (actions & SB_JUMP))
	{
		vara8 = 1;
		varac = 0;
		varb0 = 0;
		actions &= ~(SB_JUMP|SB_CROUCH);
	}
	else
		vara8 = 0;
	if (actions & SB_JUMP)
	{
		varac = 1;
		actions &= ~SB_JUMP;
		if (p->MotoSpeed == 0 && !S_CheckActorSoundPlaying(pi, 89))
		{
			if (S_CheckActorSoundPlaying(pi, 87))
				S_StopSound(pi, 87);
			S_PlayActorSound(89, pi);
		}
		else if (p->MotoSpeed >= 50 && !S_CheckActorSoundPlaying(pi, 88))
			S_PlayActorSound(88, pi);
		else if (!S_CheckActorSoundPlaying(pi, 88) && !S_CheckActorSoundPlaying(pi, 89))
			S_PlayActorSound(88, pi);
	}
	else
	{
		varac = 0;
		if (S_CheckActorSoundPlaying(pi, 89))
		{
			S_StopSound(pi, 89);
			if (!S_CheckActorSoundPlaying(pi, 90))
				S_PlayActorSound(90, pi);
		}
		if (S_CheckActorSoundPlaying(pi, 88))
		{
			S_StopSound(pi, 88);
			if (!S_CheckActorSoundPlaying(pi, 90))
				S_PlayActorSound(90, pi);
		}
		if (!S_CheckActorSoundPlaying(pi, 90) && !S_CheckActorSoundPlaying(pi, 87))
			S_PlayActorSound(87, pi);
	}

	if (actions & SB_CROUCH)
	{
		varb0 = 1;
		actions &= ~SB_CROUCH;
	}
	else
		varb0 = 0;
	if (p->vehicle_backwards)
	{
		varb4 = 1;
		p->vehicle_backwards = false;
	}
	else varb4 = 0;
	if (p->vehicle_turnl)
	{
		varb8 = 1;
		varbc = 1;
		p->vehicle_turnl = false;
		if (!S_CheckActorSoundPlaying(pi, 91) && p->MotoSpeed > 30 && !p->NotOnWater)
			S_PlayActorSound(91, pi);
	}
	else
	{
		varb8 = 0;
		varbc = 0;
	}
	if (p->vehicle_turnr)
	{
		varc0 = 1;
		varc4 = 1;
		p->vehicle_turnr = false;
		if (!S_CheckActorSoundPlaying(pi, 91) && p->MotoSpeed > 30 && !p->NotOnWater)
			S_PlayActorSound(91, pi);
	}
	else
	{
		varc0 = 0;
		varc4 = 0;
	}
	varc8 = 0;
	if (!p->NotOnWater)
	{
		if (p->drink_amt > 88 && p->moto_drink == 0)
		{
			varcc = krand() & 63;
			if (varcc == 1)
				p->moto_drink = -10;
			else if (varcc == 2)
				p->moto_drink = 10;
		}
		else if (p->drink_amt > 99 && p->moto_drink == 0)
		{
			varcc = krand() & 31;
			if (varcc == 1)
				p->moto_drink = -20;
			else if (varcc == 2)
				p->moto_drink = 20;
		}
	}
	if (p->on_ground == 1)
	{
		if (vara8)
		{
			if (p->MotoSpeed <= 25)
			{
				p->MotoSpeed++;
				if (!S_CheckActorSoundPlaying(pi, 182))
					S_PlayActorSound(182, pi);
			}
			else
			{
				p->MotoSpeed -= 2;
				if (p->MotoSpeed < 0)
					p->MotoSpeed = 0;
				p->VBumpTarget = 30;
				p->moto_do_bump = 1;
			}
		}
		else if (varb0 && p->MotoSpeed > 0)
		{
			p->MotoSpeed -= 2;
			if (p->MotoSpeed < 0)
				p->MotoSpeed = 0;
			p->VBumpTarget = 30;
			p->moto_do_bump = 1;
		}
		else if (varac)
		{
			if (p->MotoSpeed < 40)
				if (!p->NotOnWater)
				{
					p->VBumpTarget = -30;
					p->moto_bump_fast = 1;
				}
			p->MotoSpeed++;
			if (p->MotoSpeed > 120)
				p->MotoSpeed = 120;
		}
		else if (p->MotoSpeed > 0)
			p->MotoSpeed--;
		if (p->moto_do_bump && (!varb0 || p->MotoSpeed == 0))
		{
			p->VBumpTarget = 0;
			p->moto_do_bump = 0;
		}
		if (varb4 && p->MotoSpeed == 0 && !varb0)
		{
			int vard0;
			if (!p->NotOnWater)
				p->MotoSpeed = -25;
			else
				p->MotoSpeed = -20;
			vard0 = varc4;
			varc4 = varbc;
			varbc = vard0;
			varc8 = 1;
		}
	}
	if (p->MotoSpeed != 0 && p->on_ground == 1)
	{
		if (!p->VBumpNow)
			if ((krand() & 15) == 14)
				p->VBumpTarget = (p->MotoSpeed >> 4) * ((krand() & 3) - 2);
		if (varbc || p->moto_drink < 0)
		{
			if (p->moto_drink < 0)
				p->moto_drink++;
		}
		else if (varc4 || p->moto_drink > 0)
		{
			if (p->moto_drink > 0)
				p->moto_drink--;
		}
	}

	double horiz = 0;
	if (p->TurbCount)
	{
		if (p->TurbCount <= 1)
		{
			horiz = 0;
			p->TurbCount = 0;
			p->VBumpTarget = 0;
			p->VBumpNow = 0;
		}
		else
		{
			horiz = ((krand() & 15) - 7);
			p->TurbCount--;
			p->moto_drink = (krand() & 3) - 2;
		}
	}
	else if (p->VBumpTarget > p->VBumpNow)
	{
		if (p->moto_bump_fast)
			p->VBumpNow += 6;
		else
			p->VBumpNow++;
		if (p->VBumpTarget < p->VBumpNow)
			p->VBumpNow = p->VBumpTarget;
		horiz = p->VBumpNow / 3.;
	}
	else if (p->VBumpTarget < p->VBumpNow)
	{
		if (p->moto_bump_fast)
			p->VBumpNow -= 6;
		else
			p->VBumpNow--;
		if (p->VBumpTarget > p->VBumpNow)
			p->VBumpNow = p->VBumpTarget;
		horiz = p->VBumpNow / 3.;
	}
	else
	{
		p->VBumpTarget = 0;
		p->moto_bump_fast = 0;
	}
	if (horiz != 0)
	{
		p->horizon.addadjustment(horiz - FixedToFloat(p->horizon.horiz.asq16()));
	}

	if (p->MotoSpeed > 0 && p->on_ground == 1 && (varbc || varc4))
	{
		short vard4, vard8, vardc, vare0;
		vard4 = p->MotoSpeed;
		vard8 = p->angle.ang.asbuild();
		if (varbc)
			vardc = -10;
		else
			vardc = 10;
		if (vardc < 0)
			vare0 = 350;
		else
			vare0 = -350;
		vard4 <<= 2;
		int ang;
		if (p->moto_do_bump)
		{
			p->posxv += (vard4 >> 6) * (sintable[(vardc * -51 + vard8 + 512) & 2047] << 4);
			p->posyv += (vard4 >> 6) * (sintable[(vardc * -51 + vard8) & 2047] << 4);
			ang = vare0 >> 5;
		}
		else
		{
			p->posxv += (vard4 >> 7) * (sintable[(vardc * -51 + vard8 + 512) & 2047] << 4);
			p->posyv += (vard4 >> 7) * (sintable[(vardc * -51 + vard8) & 2047] << 4);
			ang = vare0 >> 6;
		}
		p->angle.addadjustment(FixedToFloat(getincangleq16(p->angle.ang.asq16(), IntToFixed(vard8 - ang))));
	}
	if (p->NotOnWater)
		if (p->MotoSpeed > 50)
			p->MotoSpeed -= (p->MotoSpeed >> 1);

}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

static void movement(int snum, ESyncBits actions, int psect, int fz, int cz, int shrunk, int truefdist, int psectlotag)
{
	auto p = &ps[snum];
	auto pi = p->i;
	auto s = &sprite[pi];

	if (p->airleft != 15 * 26)
		p->airleft = 15 * 26; //Aprox twenty seconds.

	if (p->scuba_on == 1)
		p->scuba_on = 0;

	int i = 40;
	if (psectlotag == ST_1_ABOVE_WATER && p->spritebridge == 0)
	{
		if (shrunk == 0)
		{
			i = 34;
			p->pycount += 32;
			p->pycount &= 2047;
			p->pyoff = sintable[p->pycount] >> 6;
		}
		else i = 12;

		if (shrunk == 0 && truefdist <= PHEIGHT)
		{
			if (p->on_ground == 1)
			{
				if (p->dummyplayersprite == -1)
					p->dummyplayersprite =
					fi.spawn(pi, PLAYERONWATER);

				p->footprintcount = 6;
				if (sector[p->cursectnum].floorpicnum == FLOORSLIME)
				{
					p->footprintpal = 8;
					p->footprintshade = 0;
				}
				else if (isRRRA() && (sector[p->cursectnum].floorpicnum == RRTILE7756 || sector[p->cursectnum].floorpicnum == RRTILE7888))
				{
					p->footprintpal = 0;
					p->footprintshade = 40;
				}
				else
				{
					p->footprintpal = 0;
					p->footprintshade = 0;
				}
			}
		}
	}
	else if (!p->OnMotorcycle)
	{
		footprints(snum);
	}

	if (p->posz < (fz - (i << 8))) //falling
	{
		if ((actions & (SB_JUMP|SB_CROUCH)) == 0 && p->on_ground && (sector[psect].floorstat & 2) && p->posz >= (fz - (i << 8) - (16 << 8)))
			p->posz = fz - (i << 8);
		else
		{
			p->on_ground = 0;

			if ((p->OnMotorcycle || p->OnBoat) && fz - (i << 8) * 2 > p->posz)
			{
				if (p->MotoOnGround)
				{
					p->VBumpTarget = 80;
					p->moto_bump_fast = 1;
					p->poszv -= gc * (p->MotoSpeed >> 4);
					p->MotoOnGround = 0;
					if (S_CheckActorSoundPlaying(pi, 188))
						S_StopSound(188, pi);
					S_PlayActorSound(189, pi);
				}
				else
				{
					p->poszv += gc - 80 + (120 - p->MotoSpeed);
					if (!S_CheckActorSoundPlaying(pi, 189) && !S_CheckActorSoundPlaying(pi, 190))
						S_PlayActorSound(190, pi);
				}
			}
			else
				p->poszv += (gc + 80); // (TICSPERFRAME<<6);

			if (p->poszv >= (4096 + 2048)) p->poszv = (4096 + 2048);
			if (p->poszv > 2400 && p->falling_counter < 255)
			{
				p->falling_counter++;
				if (p->falling_counter == 38 && !S_CheckActorSoundPlaying(pi, DUKE_SCREAM))
					S_PlayActorSound(DUKE_SCREAM, pi);
			}

			if ((p->posz + p->poszv) >= (fz - (i << 8))) // hit the ground
			{
				S_StopSound(DUKE_SCREAM, pi);
				if (sector[p->cursectnum].lotag != 1)
				{
					if (isRRRA()) p->MotoOnGround = 1;
					if (p->falling_counter > 62 || (isRRRA() && p->falling_counter > 2 && sector[p->cursectnum].lotag == 802))
						quickkill(p);

					else if (p->falling_counter > 9)
					{
						int j = p->falling_counter;
						s->extra -= j - (krand() & 3);
						if (s->extra <= 0)
						{
							S_PlayActorSound(SQUISHED, pi);
						}
						else
						{
							S_PlayActorSound(DUKE_LAND, pi);
							S_PlayActorSound(DUKE_LAND_HURT, pi);
						}

						SetPlayerPal(p, PalEntry(32, 16, 0, 0));
					}
					else if (p->poszv > 2048)
					{
						if (p->OnMotorcycle)
						{
							if (S_CheckActorSoundPlaying(pi, 190))
								S_StopSound(pi, 190);
							S_PlayActorSound(191, pi);
							p->TurbCount = 12;
						}
						else S_PlayActorSound(DUKE_LAND, pi);
					}
					else if (p->poszv > 1024 && p->OnMotorcycle)
					{
						S_PlayActorSound(DUKE_LAND, pi);
						p->TurbCount = 12;
					}
				}
			}
		}
	}

	else
	{
		p->falling_counter = 0;
		S_StopSound(-1, pi, CHAN_VOICE);

		if (psectlotag != ST_1_ABOVE_WATER && psectlotag != ST_2_UNDERWATER && p->on_ground == 0 && p->poszv > (6144 >> 1))
			p->hard_landing = p->poszv >> 10;

		p->on_ground = 1;

		if (i == 40)
		{
			//Smooth on the ground

			int k = ((fz - (i << 8)) - p->posz) >> 1;
			if (abs(k) < 256) k = 0;
			p->posz += k;
			p->poszv -= 768;
			if (p->poszv < 0) p->poszv = 0;
		}
		else if (p->jumping_counter == 0)
		{
			p->posz += ((fz - (i << 7)) - p->posz) >> 1; //Smooth on the water
			if (p->on_warping_sector == 0 && p->posz > fz - (16 << 8))
			{
				p->posz = fz - (16 << 8);
				p->poszv >>= 1;
			}
		}

		p->on_warping_sector = 0;

		if ((actions & SB_CROUCH) && !p->OnMotorcycle)
		{
			playerCrouch(snum);
		}

		if ((actions & SB_JUMP) == 0 && !p->OnMotorcycle && p->jumping_toggle == 1)
			p->jumping_toggle = 0;

		else if ((actions & SB_JUMP) && !p->OnMotorcycle && p->jumping_toggle == 0)
		{
			playerJump(snum, fz, cz);
		}
	}

	if (p->jumping_counter)
	{
		if ((actions & SB_JUMP) == 0 && !p->OnMotorcycle && p->jumping_toggle == 1)
			p->jumping_toggle = 0;

		if (p->jumping_counter < 768)
		{
			if (psectlotag == ST_1_ABOVE_WATER && p->jumping_counter > 768)
			{
				p->jumping_counter = 0;
				p->poszv = -512;
			}
			else
			{
				p->poszv -= (sintable[(2048 - 128 + p->jumping_counter) & 2047]) / 12;
				p->jumping_counter += 180;
				p->on_ground = 0;
			}
		}
		else
		{
			p->jumping_counter = 0;
			p->poszv = 0;
		}
	}

	p->posz += p->poszv;

	if (p->posz < (cz + (4 << 8)))
	{
		p->jumping_counter = 0;
		if (p->poszv < 0)
			p->posxv = p->posyv = 0;
		p->poszv = 128;
		p->posz = cz + (4 << 8);
	}
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

static void underwater(int snum, ESyncBits actions, int psect, int fz, int cz)
{
	int j;
	auto p = &ps[snum];
	int pi = p->i;
	int psectlotag = sector[psect].lotag;

	p->jumping_counter = 0;

	p->pycount += 32;
	p->pycount &= 2047;
	p->pyoff = sintable[p->pycount] >> 7;

	if (!S_CheckActorSoundPlaying(pi, DUKE_UNDERWATER))
		S_PlayActorSound(DUKE_UNDERWATER, pi);

	if ((actions & SB_JUMP) && !p->OnMotorcycle)
	{
		if (p->poszv > 0) p->poszv = 0;
		p->poszv -= 348;
		if (p->poszv < -(256 * 6)) p->poszv = -(256 * 6);
	}
	else if ((actions & SB_CROUCH) || p->OnMotorcycle)
	{
		if (p->poszv < 0) p->poszv = 0;
		p->poszv += 348;
		if (p->poszv > (256 * 6)) p->poszv = (256 * 6);
	}
	else
	{
		if (p->poszv < 0)
		{
			p->poszv += 256;
			if (p->poszv > 0)
				p->poszv = 0;
		}
		if (p->poszv > 0)
		{
			p->poszv -= 256;
			if (p->poszv < 0)
				p->poszv = 0;
		}
	}

	if (p->poszv > 2048)
		p->poszv >>= 1;

	p->posz += p->poszv;

	if (p->posz > (fz - (15 << 8)))
		p->posz += ((fz - (15 << 8)) - p->posz) >> 1;

	if (p->posz < (cz + (4 << 8)))
	{
		p->posz = cz + (4 << 8);
		p->poszv = 0;
	}

	if (p->scuba_on && (krand() & 255) < 8)
	{
		j = fi.spawn(pi, WATERBUBBLE);
		sprite[j].x +=
			sintable[(p->angle.ang.asbuild() + 512 + 64 - (global_random & 128) + 128) & 2047] >> 6;
		sprite[j].y +=
			sintable[(p->angle.ang.asbuild() + 64 - (global_random & 128) + 128) & 2047] >> 6;
		sprite[j].xrepeat = 3;
		sprite[j].yrepeat = 2;
		sprite[j].z = p->posz + (8 << 8);
		sprite[j].cstat = 514;
	}
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void onMotorcycleMove(int snum, int psect, int j)
{
	auto p = &ps[snum];
	int pi = p->i;
	auto s = &sprite[pi];
	int psectlotag = sector[psect].lotag;

	short var104, var108, var10c;
	var104 = 0;
	j &= (MAXWALLS - 1);
	var108 = getangle(wall[wall[j].point2].x - wall[j].x, wall[wall[j].point2].y - wall[j].y);
	var10c = abs(p->angle.ang.asbuild() - var108);
	int ang;
	switch (krand() & 1)
	{
	case 0:
		ang = p->MotoSpeed >> 1;
		break;
	case 1:
		ang = -(p->MotoSpeed >> 1);
		break;
	}
	p->angle.addadjustment(ang);
	if (var10c >= 441 && var10c <= 581)
	{
		var104 = (p->MotoSpeed * p->MotoSpeed) >> 8;
		p->MotoSpeed = 0;
		if (S_CheckActorSoundPlaying(pi, 238) == 0)
			S_PlayActorSound(238, pi);
	}
	else if (var10c >= 311 && var10c <= 711)
	{
		var104 = (p->MotoSpeed * p->MotoSpeed) >> 11;
		p->MotoSpeed -= (p->MotoSpeed >> 1) + (p->MotoSpeed >> 2);
		if (S_CheckActorSoundPlaying(pi, 238) == 0)
			S_PlayActorSound(238, pi);
	}
	else if (var10c >= 111 && var10c <= 911)
	{
		var104 = (p->MotoSpeed * p->MotoSpeed) >> 14;
		p->MotoSpeed -= (p->MotoSpeed >> 1);
		if (S_CheckActorSoundPlaying(pi, 239) == 0)
			S_PlayActorSound(239, pi);
	}
	else
	{
		var104 = (p->MotoSpeed * p->MotoSpeed) >> 15;
		p->MotoSpeed -= (p->MotoSpeed >> 3);
		if (S_CheckActorSoundPlaying(pi, 240) == 0)
			S_PlayActorSound(240, pi);
	}
	s->extra -= var104;
	if (s->extra <= 0)
	{
		S_PlayActorSound(SQUISHED, pi);
		SetPlayerPal(p, PalEntry(63, 63, 0, 0));
	}
	else if (var104)
		S_PlayActorSound(DUKE_LAND_HURT, pi);

}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void onBoatMove(int snum, int psect, int j)
{
	auto p = &ps[snum];
	int pi = p->i;
	auto s = &sprite[pi];
	int psectlotag = sector[psect].lotag;

	short var114, var118;
	j &= (MAXWALLS - 1);
	var114 = getangle(wall[wall[j].point2].x - wall[j].x, wall[wall[j].point2].y - wall[j].y);
	var118 = abs(p->angle.ang.asbuild() - var114);
	int ang;
	switch (krand() & 1)
	{
	case 0:
		ang = p->MotoSpeed >> 2;
		break;
	case 1:
		ang = -(p->MotoSpeed >> 2);
		break;
	}
	p->angle.addadjustment(ang);
	if (var118 >= 441 && var118 <= 581)
	{
		p->MotoSpeed = ((p->MotoSpeed >> 1) + (p->MotoSpeed >> 2)) >> 2;
		if (psectlotag == 1)
			if (S_CheckActorSoundPlaying(pi, 178) == 0)
				S_PlayActorSound(178, pi);
	}
	else if (var118 >= 311 && var118 <= 711)
	{
		p->MotoSpeed -= ((p->MotoSpeed >> 1) + (p->MotoSpeed >> 2)) >> 3;
		if (psectlotag == 1)
			if (S_CheckActorSoundPlaying(pi, 179) == 0)
				S_PlayActorSound(179, pi);
	}
	else if (var118 >= 111 && var118 <= 911)
	{
		p->MotoSpeed -= (p->MotoSpeed >> 4);
		if (psectlotag == 1)
			if (S_CheckActorSoundPlaying(pi, 180) == 0)
				S_PlayActorSound(180, pi);
	}
	else
	{
		p->MotoSpeed -= (p->MotoSpeed >> 6);
		if (psectlotag == 1)
			if (S_CheckActorSoundPlaying(pi, 181) == 0)
				S_PlayActorSound(181, pi);
	}

}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void onMotorcycleHit(int snum, int var60)
{
	auto p = &ps[snum];
	auto s = &sprite[var60];
	auto ht = &hittype[var60];
	if (badguy(s) || s->picnum == APLAYER)
	{
		if (s->picnum != APLAYER)
		{
			if (numplayers == 1)
			{
				fi.movesprite(var60, sintable[int(p->TiltStatus * 20 + p->angle.ang.asbuild() + 512) & 2047] >> 8,
					sintable[int(p->TiltStatus * 20 + p->angle.ang.asbuild()) & 2047] >> 8, s->zvel, CLIPMASK0);
			}
		}
		else
			ht->owner = p->i;
		ht->picnum = MOTOHIT;
		ht->extra = p->MotoSpeed >> 1;
		p->MotoSpeed -= p->MotoSpeed >> 2;
		p->TurbCount = 6;
	}
	else if ((s->picnum == RRTILE2431 || s->picnum == RRTILE2443 || s->picnum == RRTILE2451 || s->picnum == RRTILE2455)
		&& s->picnum != ACTIVATORLOCKED && p->MotoSpeed > 45)
	{
		S_PlayActorSound(SQUISHED, var60);
		if (s->picnum == RRTILE2431 || s->picnum == RRTILE2451)
		{
			if (s->lotag != 0)
			{
				for (int j = 0; j < MAXSPRITES; j++)
				{
					auto sprj = &sprite[j];
					if ((sprj->picnum == RRTILE2431 || sprj->picnum == RRTILE2451) && sprj->pal == 4)
					{
						if (s->lotag == sprj->lotag)
						{
							sprj->xrepeat = 0;
							sprj->yrepeat = 0;
						}
					}
				}
			}
			fi.guts(&sprite[var60], RRTILE2460, 12, myconnectindex);
			fi.guts(&sprite[var60], RRTILE2465, 3, myconnectindex);
		}
		else
			fi.guts(&sprite[var60], RRTILE2465, 3, myconnectindex);
		fi.guts(&sprite[var60], RRTILE2465, 3, myconnectindex);
		s->xrepeat = 0;
		s->yrepeat = 0;
	}
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void onBoatHit(int snum, int var60)
{
	auto p = &ps[snum];
	auto s = &sprite[var60];
	auto ht = &hittype[var60];

	if (badguy(s) || s->picnum == APLAYER)
	{
		if (s->picnum != APLAYER)
		{
			if (numplayers == 1)
			{
				fi.movesprite(var60, sintable[int(p->TiltStatus * 20 + p->angle.ang.asbuild() + 512) & 2047] >> 9,
					sintable[int(p->TiltStatus * 20 + p->angle.ang.asbuild()) & 2047] >> 9, s->zvel, CLIPMASK0);
			}
		}
		else
			ht->owner = p->i;
		ht->picnum = MOTOHIT;
		ht->extra = p->MotoSpeed >> 2;
		p->MotoSpeed -= p->MotoSpeed >> 2;
		p->TurbCount = 6;
	}
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

static void fireweapon(int snum)
{
	auto p = &ps[snum];
	int pi = p->i;

	p->crack_time = CRACK_TIME;

	if (p->holster_weapon == 1)
	{
		if (p->last_pissed_time <= (26 * 218) && p->weapon_pos == -9)
		{
			p->holster_weapon = 0;
			p->oweapon_pos = p->weapon_pos = 10;
			FTA(74, p);
		}
	}
	else
	{
		if (!isRRRA() && p->curr_weapon >= MOTORCYCLE_WEAPON) return;
		switch (p->curr_weapon)
		{
		case DYNAMITE_WEAPON:
			p->hbomb_hold_delay = 0;
			if (p->ammo_amount[DYNAMITE_WEAPON] > 0)
				p->kickback_pic = 1;
			break;
		case THROWINGDYNAMITE_WEAPON:
			p->hbomb_hold_delay = 0;
			p->kickback_pic = 1;
			break;

		case PISTOL_WEAPON:
			if (p->ammo_amount[PISTOL_WEAPON] > 0)
			{
				p->ammo_amount[PISTOL_WEAPON]--;
				p->kickback_pic = 1;
			}
			break;

		case RIFLEGUN_WEAPON:
			if (p->ammo_amount[RIFLEGUN_WEAPON] > 0) // && p->random_club_frame == 0)
				p->kickback_pic = 1;
			break;

		case SHOTGUN_WEAPON:
			if (p->ammo_amount[SHOTGUN_WEAPON] > 0 && p->random_club_frame == 0)
				p->kickback_pic = 1;
			break;

		case BOWLING_WEAPON:
			if (p->ammo_amount[BOWLING_WEAPON] > 0)
				p->kickback_pic = 1;
			break;
		case POWDERKEG_WEAPON:
			if (p->ammo_amount[POWDERKEG_WEAPON] > 0)
				p->kickback_pic = 1;
			break;

		case BUZZSAW_WEAPON:
		case THROWSAW_WEAPON:
			if (p->curr_weapon == BUZZSAW_WEAPON)
			{
				if (p->ammo_amount[BUZZSAW_WEAPON] > 0)
				{
					p->kickback_pic = 1;
					S_PlayActorSound(431, pi);
				}
			}
			else if (p->ammo_amount[THROWSAW_WEAPON] > 0)
			{
				p->kickback_pic = 1;
				S_PlayActorSound(SHRINKER_FIRE, pi);
			}
			break;

		case ALIENBLASTER_WEAPON:
			if (p->ammo_amount[ALIENBLASTER_WEAPON] > 0)
				p->kickback_pic = 1;
			break;

		case TIT_WEAPON:
			if (p->ammo_amount[TIT_WEAPON] > 0)
			{
				p->kickback_pic = 1;
				p->hbomb_hold_delay = !p->hbomb_hold_delay;
			}
			break;

		case MOTORCYCLE_WEAPON:
			if (p->ammo_amount[MOTORCYCLE_WEAPON] > 0)
			{
				p->kickback_pic = 1;
				p->hbomb_hold_delay = !p->hbomb_hold_delay;
			}
			break;

		case BOAT_WEAPON:
			if (p->ammo_amount[BOAT_WEAPON] > 0)
				p->kickback_pic = 1;
			break;

		case CROSSBOW_WEAPON:
			if (p->ammo_amount[CROSSBOW_WEAPON] > 0)
				p->kickback_pic = 1;
			break;

		case CHICKEN_WEAPON:
			if (p->ammo_amount[CHICKEN_WEAPON] > 0)
				p->kickback_pic = 1;
			break;

		case KNEE_WEAPON:
		case SLINGBLADE_WEAPON:
			if (p->curr_weapon == SLINGBLADE_WEAPON)
			{
				if (p->ammo_amount[SLINGBLADE_WEAPON] > 0)
					if (p->quick_kick == 0)
						p->kickback_pic = 1;
			}
			else if (!isRRRA() || p->ammo_amount[KNEE_WEAPON] > 0)
				if (p->quick_kick == 0)
					p->kickback_pic = 1;
			break;
		}
	}
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

static void operateweapon(int snum, ESyncBits actions, int psect)
{
	auto p = &ps[snum];
	int pi = p->i;
	int i, j, k;
	int psectlotag = sector[psect].lotag;

	if (!isRRRA() && p->curr_weapon >= MOTORCYCLE_WEAPON) return;
	switch (p->curr_weapon)
	{
	case DYNAMITE_WEAPON:

		if (p->kickback_pic == 1)
			S_PlaySound(401);
		if (p->kickback_pic == 6 && (actions & SB_FIRE))
			p->rapid_fire_hold = 1;
		p->kickback_pic++;
		if (p->kickback_pic > 19)
		{
			p->okickback_pic = p->kickback_pic = 0;
			p->curr_weapon = THROWINGDYNAMITE_WEAPON;
			p->last_weapon = -1;
			p->oweapon_pos = p->weapon_pos = 10;
			p->detonate_time = 45;
			p->detonate_count = 1;
			S_PlaySound(402);
		}

		break;


	case THROWINGDYNAMITE_WEAPON:

		p->kickback_pic++;

		if (p->detonate_time < 0)
		{
			p->hbomb_on = 0;
		}

		if (p->kickback_pic == 39)
		{
			p->hbomb_on = 0;
			p->noise_radius = 8192;
			madenoise(snum);
		}
		if (p->kickback_pic == 12)
		{
			p->ammo_amount[DYNAMITE_WEAPON]--;
			if (p->ammo_amount[CROSSBOW_WEAPON])
				p->ammo_amount[CROSSBOW_WEAPON]--;
			if (p->on_ground && (actions & SB_CROUCH) && !p->OnMotorcycle)
			{
				k = 15;
				i = -mulscale16(p->horizon.sum().asq16(), 20);
			}
			else
			{
				k = 140;
				i = -512 - -mulscale16(p->horizon.sum().asq16(), 20);
			}

			j = EGS(p->cursectnum,
				p->posx + (sintable[(p->angle.ang.asbuild() + 512) & 2047] >> 6),
				p->posy + (sintable[p->angle.ang.asbuild() & 2047] >> 6),
				p->posz, HEAVYHBOMB, -16, 9, 9,
				p->angle.ang.asbuild(), (k + (p->hbomb_hold_delay << 5)) * 2, i, pi, 1);

			if (k == 15)
			{
				sprite[j].yvel = 3;
				sprite[j].z += (8 << 8);
			}

			k = hits(pi);
			if (k < 512)
			{
				sprite[j].ang += 1024;
				sprite[j].zvel /= 3;
				sprite[j].xvel /= 3;
			}

			p->hbomb_on = 1;
		}
		else if (p->kickback_pic < 12 && (actions & SB_FIRE))
			p->hbomb_hold_delay++;

		if (p->kickback_pic == 40)
		{
			p->okickback_pic = p->kickback_pic = 0;
			p->curr_weapon = DYNAMITE_WEAPON;
			p->last_weapon = -1;
			p->detonate_count = 0;
			p->detonate_time = 45;
			if (p->ammo_amount[DYNAMITE_WEAPON] > 0)
			{
				fi.addweapon(p, DYNAMITE_WEAPON);
				p->oweapon_pos = p->weapon_pos = -9;
			}
			else checkavailweapon(p);
		}
		break;

	case PISTOL_WEAPON:
		if (p->kickback_pic == 1)
		{
			fi.shoot(pi, SHOTSPARK1);
			S_PlayActorSound(PISTOL_FIRE, pi);
			p->noise_radius = 8192;
			madenoise(snum);

			lastvisinc = ud.levelclock + 32;
			p->visibility = 0;
			if (psectlotag != 857)
			{
				p->posxv -= sintable[(p->angle.ang.asbuild() + 512) & 2047] << 4;
				p->posyv -= sintable[p->angle.ang.asbuild() & 2047] << 4;
			}
		}
		else if (p->kickback_pic == 2)
			if (p->ammo_amount[PISTOL_WEAPON] <= 0)
			{
				p->okickback_pic = p->kickback_pic = 0;
				checkavailweapon(p);
			}

		p->kickback_pic++;

		if (p->kickback_pic >= 22)
		{
			if (p->ammo_amount[PISTOL_WEAPON] <= 0)
			{
				p->okickback_pic = p->kickback_pic = 0;
				checkavailweapon(p);
				break;
			}
			else if ((p->ammo_amount[PISTOL_WEAPON] % 6) == 0)
			{
				switch (p->kickback_pic)
				{
				case 24:
					S_PlayActorSound(EJECT_CLIP, pi);
					break;
				case 30:
					S_PlayActorSound(INSERT_CLIP, pi);
					break;
				}
			}
			else
				p->kickback_pic = 38;
		}

		if (p->kickback_pic == 38)
		{
			p->okickback_pic = p->kickback_pic = 0;
			checkavailweapon(p);
		}

		break;

	case SHOTGUN_WEAPON:

		p->kickback_pic++;

		if (p->kickback_pic == 6)
			if (p->shotgun_state[0] == 0)
				if (p->ammo_amount[SHOTGUN_WEAPON] > 1)
					if (actions & SB_FIRE)
						p->shotgun_state[1] = 1;

		if (p->kickback_pic == 4)
		{
			fi.shoot(pi, SHOTGUN);
			fi.shoot(pi, SHOTGUN);
			fi.shoot(pi, SHOTGUN);
			fi.shoot(pi, SHOTGUN);
			fi.shoot(pi, SHOTGUN);
			fi.shoot(pi, SHOTGUN);
			fi.shoot(pi, SHOTGUN);
			fi.shoot(pi, SHOTGUN);
			fi.shoot(pi, SHOTGUN);
			fi.shoot(pi, SHOTGUN);

			p->ammo_amount[SHOTGUN_WEAPON]--;

			S_PlayActorSound(SHOTGUN_FIRE, pi);

			p->noise_radius = 8192;
			madenoise(snum);

			lastvisinc = ud.levelclock + 32;
			p->visibility = 0;
		}

		if (p->kickback_pic == 7)
		{
			if (p->shotgun_state[1])
			{
				fi.shoot(pi, SHOTGUN);
				fi.shoot(pi, SHOTGUN);
				fi.shoot(pi, SHOTGUN);
				fi.shoot(pi, SHOTGUN);
				fi.shoot(pi, SHOTGUN);
				fi.shoot(pi, SHOTGUN);
				fi.shoot(pi, SHOTGUN);
				fi.shoot(pi, SHOTGUN);
				fi.shoot(pi, SHOTGUN);
				fi.shoot(pi, SHOTGUN);

				p->ammo_amount[SHOTGUN_WEAPON]--;

				S_PlayActorSound(SHOTGUN_FIRE, pi);

				if (psectlotag != 857)
				{
					p->posxv -= sintable[(p->angle.ang.asbuild() + 512) & 2047] << 5;
					p->posyv -= sintable[p->angle.ang.asbuild() & 2047] << 5;
				}
			}
			else if (psectlotag != 857)
			{
				p->posxv -= sintable[(p->angle.ang.asbuild() + 512) & 2047] << 4;
				p->posyv -= sintable[p->angle.ang.asbuild() & 2047] << 4;
			}
		}

		if (p->shotgun_state[0])
		{
			switch (p->kickback_pic)
			{
			case 16:
				checkavailweapon(p);
				break;
			case 17:
				S_PlayActorSound(SHOTGUN_COCK, pi);
				break;
			case 28:
				p->okickback_pic = p->kickback_pic = 0;
				p->shotgun_state[0] = 0;
				p->shotgun_state[1] = 0;
				return;
			}
		}
		else if (p->shotgun_state[1])
		{
			switch (p->kickback_pic)
			{
			case 26:
				checkavailweapon(p);
				break;
			case 27:
				S_PlayActorSound(SHOTGUN_COCK, pi);
				break;
			case 38:
				p->okickback_pic = p->kickback_pic = 0;
				p->shotgun_state[0] = 0;
				p->shotgun_state[1] = 0;
				return;
			}
		}
		else
		{
			switch (p->kickback_pic)
			{
			case 16:
				checkavailweapon(p);
				p->okickback_pic = p->kickback_pic = 0;
				p->shotgun_state[0] = 1;
				p->shotgun_state[1] = 0;
				return;
			}
		}
		break;

	case RIFLEGUN_WEAPON:

		p->kickback_pic++;
		p->horizon.addadjustment(1);
		p->recoil++;

		if (p->kickback_pic <= 12)
		{
			if ((p->kickback_pic % 3) == 0)
			{
				p->ammo_amount[RIFLEGUN_WEAPON]--;

				if ((p->kickback_pic % 3) == 0)
				{
					j = fi.spawn(pi, SHELL);

					sprite[j].ang += 1024;
					sprite[j].ang &= 2047;
					sprite[j].xvel += 32;
					sprite[j].z += (3 << 8);
					ssp(j, CLIPMASK0);
				}

				S_PlayActorSound(CHAINGUN_FIRE, pi);
				fi.shoot(pi, CHAINGUN);
				p->noise_radius = 8192;
				madenoise(snum);
				lastvisinc = ud.levelclock + 32;
				p->visibility = 0;

				if (psectlotag != 857)
				{
					p->posxv -= sintable[(p->angle.ang.asbuild() + 512) & 2047] << 4;
					p->posyv -= sintable[p->angle.ang.asbuild() & 2047] << 4;
				}
				checkavailweapon(p);

				if ((actions & SB_FIRE) == 0)
				{
					p->okickback_pic = p->kickback_pic = 0;
					break;
				}
			}
		}
		else if (p->kickback_pic > 10)
		{
			if (actions & SB_FIRE) p->kickback_pic = 1;
			else p->okickback_pic = p->kickback_pic = 0;
		}

		break;

	case BUZZSAW_WEAPON:

		if (p->kickback_pic > 3)
		{
			p->okickback_pic = p->kickback_pic = 0;
			fi.shoot(pi, GROWSPARK);
			p->noise_radius = 1024;
			madenoise(snum);
			checkavailweapon(p);
		}
		else p->kickback_pic++;
		break;

	case THROWSAW_WEAPON:

		if (p->kickback_pic == 1)
		{
			p->ammo_amount[THROWSAW_WEAPON]--;
			fi.shoot(pi, SHRINKSPARK);
			checkavailweapon(p);
		}
		p->kickback_pic++;
		if (p->kickback_pic > 20)
			p->okickback_pic = p->kickback_pic = 0;
		break;

	case TIT_WEAPON:
		p->kickback_pic++;
		if (p->kickback_pic == 2 || p->kickback_pic == 4)
		{
			p->visibility = 0;
			lastvisinc = ud.levelclock + 32;
			S_PlayActorSound(CHAINGUN_FIRE, pi);
			fi.shoot(pi, SHOTSPARK1);
			p->noise_radius = 16384;
			madenoise(snum);
			p->ammo_amount[TIT_WEAPON]--;
			checkavailweapon(p);
		}
		if (p->kickback_pic == 2)
		{
			p->angle.addadjustment(16);
		}
		else if (p->kickback_pic == 4)
		{
			p->angle.addadjustment(-16);
		}
		if (p->kickback_pic > 4)
			p->kickback_pic = 1;
		if (!(actions & SB_FIRE))
			p->okickback_pic = p->kickback_pic = 0;
		break;

	case MOTORCYCLE_WEAPON:
		p->kickback_pic++;
		if (p->kickback_pic == 2 || p->kickback_pic == 4)
		{
			p->visibility = 0;
			lastvisinc = ud.levelclock + 32;
			S_PlayActorSound(CHAINGUN_FIRE, pi);
			fi.shoot(pi, CHAINGUN);
			p->noise_radius = 16384;
			madenoise(snum);
			p->ammo_amount[MOTORCYCLE_WEAPON]--;
			if (p->ammo_amount[MOTORCYCLE_WEAPON] <= 0)
				p->okickback_pic = p->kickback_pic = 0;
			else
				checkavailweapon(p);
		}
		if (p->kickback_pic == 2)
		{
			p->angle.addadjustment(4);
		}
		else if (p->kickback_pic == 4)
		{
			p->angle.addadjustment(-4);
		}
		if (p->kickback_pic > 4)
			p->kickback_pic = 1;
		if (!(actions & SB_FIRE))
			p->okickback_pic = p->kickback_pic = 0;
		break;
	case BOAT_WEAPON:
		if (p->kickback_pic == 3)
		{
			p->MotoSpeed -= 20;
			p->ammo_amount[BOAT_WEAPON]--;
			fi.shoot(pi, RRTILE1790);
		}
		p->kickback_pic++;
		if (p->kickback_pic > 20)
		{
			p->okickback_pic = p->kickback_pic = 0;
			checkavailweapon(p);
		}
		if (p->ammo_amount[BOAT_WEAPON] <= 0)
			p->okickback_pic = p->kickback_pic = 0;
		else
			checkavailweapon(p);
		break;

	case ALIENBLASTER_WEAPON:
		p->kickback_pic++;
		if (p->kickback_pic >= 7 && p->kickback_pic <= 11)
			fi.shoot(pi, FIRELASER);

		if (p->kickback_pic == 5)
		{
			S_PlayActorSound(CAT_FIRE, pi);
			p->noise_radius = 2048;
			madenoise(snum);
		}
		else if (p->kickback_pic == 9)
		{
			p->ammo_amount[ALIENBLASTER_WEAPON]--;
			p->visibility = 0;
			lastvisinc = ud.levelclock + 32;
			checkavailweapon(p);
		}
		else if (p->kickback_pic == 12)
		{
			p->posxv -= sintable[(p->angle.ang.asbuild() + 512) & 2047] << 4;
			p->posyv -= sintable[p->angle.ang.asbuild() & 2047] << 4;
			p->horizon.addadjustment(20);
			p->recoil += 20;
		}
		if (p->kickback_pic > 20)
			p->okickback_pic = p->kickback_pic = 0;
		break;

	case POWDERKEG_WEAPON:
		if (p->kickback_pic == 3)
		{
			p->ammo_amount[POWDERKEG_WEAPON]--;
			p->gotweapon.Clear(POWDERKEG_WEAPON);
			if (p->on_ground && (actions & SB_CROUCH) && !p->OnMotorcycle)
			{
				k = 15;
				i = mulscale16(p->horizon.sum().asq16(), 20);
			}
			else
			{
				k = 32;
				i = -512 - mulscale16(p->horizon.sum().asq16(), 20);
			}

			j = EGS(p->cursectnum,
				p->posx + (sintable[(p->angle.ang.asbuild() + 512) & 2047] >> 6),
				p->posy + (sintable[p->angle.ang.asbuild() & 2047] >> 6),
				p->posz, TRIPBOMBSPRITE, -16, 9, 9,
				p->angle.ang.asbuild(), k * 2, i, pi, 1);
		}
		p->kickback_pic++;
		if (p->kickback_pic > 20)
		{
			p->okickback_pic = p->kickback_pic = 0;
			checkavailweapon(p);
		}
		break;

	case BOWLING_WEAPON:
		if (p->kickback_pic == 30)
		{
			p->ammo_amount[BOWLING_WEAPON]--;
			S_PlayActorSound(354, pi);
			fi.shoot(pi, BOWLINGBALL);
			p->noise_radius = 1024;
			madenoise(snum);
		}
		if (p->kickback_pic < 30)
		{
			p->posxv += sintable[(p->angle.ang.asbuild() + 512) & 2047] << 4;
			p->posyv += sintable[p->angle.ang.asbuild() & 2047] << 4;
		}
		p->kickback_pic++;
		if (p->kickback_pic > 40)
		{
			p->okickback_pic = p->kickback_pic = 0;
			p->gotweapon.Clear(BOWLING_WEAPON);
			checkavailweapon(p);
		}
		break;

	case KNEE_WEAPON:
		p->kickback_pic++;
		if (p->kickback_pic == 3)
			S_PlayActorSound(426, pi);
		if (p->kickback_pic == 12)
		{
			fi.shoot(pi, KNEE);
			p->noise_radius = 1024;
			madenoise(snum);
		}
		else if (p->kickback_pic == 16)
			p->okickback_pic = p->kickback_pic = 0;

		if (p->wantweaponfire >= 0)
			checkavailweapon(p);
		break;


	case SLINGBLADE_WEAPON:
		p->kickback_pic++;
		if (p->kickback_pic == 3)
			S_PlayActorSound(252, pi);
		if (p->kickback_pic == 8)
		{
			fi.shoot(pi, SLINGBLADE);
			p->noise_radius = 1024;
			madenoise(snum);
		}
		else if (p->kickback_pic == 16)
			p->okickback_pic = p->kickback_pic = 0;

		if (p->wantweaponfire >= 0)
			checkavailweapon(p);
		break;

	case CROSSBOW_WEAPON:
		p->kickback_pic++;
		if (p->kickback_pic == 4)
		{
			p->ammo_amount[CROSSBOW_WEAPON]--;
			if (p->ammo_amount[DYNAMITE_WEAPON])
				p->ammo_amount[DYNAMITE_WEAPON]--;
			lastvisinc = ud.levelclock + 32;
			p->visibility = 0;
			fi.shoot(pi, RPG);
			p->noise_radius = 32768;
			madenoise(snum);
			checkavailweapon(p);
		}
		else if (p->kickback_pic == 16)
			S_PlayActorSound(450, pi);
		else if (p->kickback_pic == 34)
			p->okickback_pic = p->kickback_pic = 0;
		break;

	case CHICKEN_WEAPON:
		p->kickback_pic++;
		if (p->kickback_pic == 4)
		{
			p->ammo_amount[CHICKEN_WEAPON]--;
			lastvisinc = ud.levelclock + 32;
			p->visibility = 0;
			fi.shoot(pi, RPG2);
			p->noise_radius = 32768;
			madenoise(snum);
			checkavailweapon(p);
		}
		else if (p->kickback_pic == 16)
			S_PlayActorSound(450, pi);
		else if (p->kickback_pic == 34)
			p->okickback_pic = p->kickback_pic = 0;
		break;

	}

}

//---------------------------------------------------------------------------
//
// this function exists because gotos suck. :P
//
//---------------------------------------------------------------------------

static void processweapon(int snum, ESyncBits actions, int psect)
{
	auto p = &ps[snum];
	int pi = p->i;
	auto s = &sprite[pi];
	int shrunk = (s->yrepeat < 8);

	if (actions & SB_FIRE)
	{
		int a = 0;
	}

	if (p->detonate_count > 0)
	{
		if (ud.god)
		{
			p->detonate_time = 45;
			p->detonate_count = 0;
		}
		else if (p->detonate_time <= 0 && p->kickback_pic < 5)
		{
			S_PlaySound(14);
			quickkill(p);
		}
	}


	if (isRRRA() && (p->curr_weapon == KNEE_WEAPON || p->curr_weapon == SLINGBLADE_WEAPON))
		p->random_club_frame += 64;

	if (p->curr_weapon == THROWSAW_WEAPON || p->curr_weapon == BUZZSAW_WEAPON)
		p->random_club_frame += 64; // Glowing

	if (p->curr_weapon == TRIPBOMB_WEAPON || p->curr_weapon == BOWLING_WEAPON)
		p->random_club_frame += 64;

	if (p->rapid_fire_hold == 1)
	{
		if (actions & SB_FIRE) return;
		p->rapid_fire_hold = 0;
	}

	if (shrunk || p->tipincs || p->access_incs)
		actions &= ~SB_FIRE;
	else if (shrunk == 0 && (actions & SB_FIRE) && p->kickback_pic == 0 && p->fist_incs == 0 &&
		p->last_weapon == -1 && (p->weapon_pos == 0 || p->holster_weapon == 1))
	{
		fireweapon(snum);
	}
	else if (p->kickback_pic)
	{
		operateweapon(snum, actions, psect);
	}
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void processinput_r(int snum)
{
	int j, i, k, doubvel, fz, cz, hz, lz, truefdist, var60;
	char shrunk;
	ESyncBits actions;
	short psect, psectlotag, pi;
	struct player_struct* p;
	spritetype* s;

	p = &ps[snum];
	pi = p->i;
	s = &sprite[pi];

	p->horizon.resetadjustment();
	p->angle.resetadjustment();

	actions = PlayerInputBits(snum, SB_ALL);

	auto sb_fvel = PlayerInputForwardVel(snum);
	auto sb_svel = PlayerInputSideVel(snum);
	auto sb_avel = PlayerInputAngVel(snum);

	psect = p->cursectnum;
	if (p->OnMotorcycle && s->extra > 0)
	{
		onMotorcycle(snum, actions);
	}
	else if (p->OnBoat && s->extra > 0)
	{
		onBoat(snum, actions);
	}
	if (psect == -1)
	{
		if (s->extra > 0 && ud.clipping == 0)
		{
			quickkill(p);
			S_PlayActorSound(SQUISHED, pi);
		}
		psect = 0;
	}

	psectlotag = sector[psect].lotag;

	if (psectlotag == 867)
	{
		SectIterator it(psect);
		while ((j = it.NextIndex()) >= 0)
		{
			if (sprite[j].picnum == RRTILE380)
				if (sprite[j].z - (8 << 8) < p->posz)
					psectlotag = 2;
		}
	}
	else if (psectlotag == 7777)
		if (currentLevel->levelNumber == levelnum(1, 6))
			lastlevel = 1;

	if (psectlotag == 848 && sector[psect].floorpicnum == WATERTILE2)
		psectlotag = 1;

	if (psectlotag == 857)
		s->clipdist = 1;
	else
		s->clipdist = 64;

	p->spritebridge = 0;

	shrunk = (s->yrepeat < 8);
	if (s->clipdist == 64)
	{
		getzrange(p->posx, p->posy, p->posz, psect, &cz, &hz, &fz, &lz, 163L, CLIPMASK0);
		j = getflorzofslope(psect, p->posx, p->posy);
	}
	else
	{
		getzrange(p->posx, p->posy, p->posz, psect, &cz, &hz, &fz, &lz, 4L, CLIPMASK0);
		j = getflorzofslope(psect, p->posx, p->posy);
	}

	p->truefz = j;
	p->truecz = getceilzofslope(psect, p->posx, p->posy);

	truefdist = abs(p->posz - j);
	if ((lz & 49152) == 16384 && psectlotag == 1 && truefdist > PHEIGHT + (16 << 8))
		psectlotag = 0;

	hittype[pi].floorz = fz;
	hittype[pi].ceilingz = cz;

	if (cl_syncinput)
	{
		p->horizon.backup();
		calcviewpitch(p, 1);
	}

	if (hz >= 0 && (hz & 49152) == 49152)
	{
		hz &= (MAXSPRITES - 1);

		if (sprite[hz].statnum == 1 && sprite[hz].extra >= 0)
		{
			hz = 0;
			cz = p->truecz;
		}
		if (sprite[hz].picnum == RRTILE3587)
		{
			if (!p->stairs)
			{
				p->stairs = 10;
				if ((actions & SB_JUMP) && !p->OnMotorcycle)
				{
					hz = 0;
					cz = p->truecz;
				}
			}
			else
				p->stairs--;
		}
	}

	if (lz >= 0 && (lz & 49152) == 49152)
	{
		j = lz & (MAXSPRITES - 1);

		if (isRRRA()) var60 = j & (MAXSPRITES - 1);

		if ((sprite[j].cstat & 33) == 33)
		{
			psectlotag = 0;
			p->footprintcount = 0;
			p->spritebridge = 1;
		}
		if (p->OnMotorcycle)
			if (badguy(&sprite[var60]))
			{
				hittype[var60].picnum = MOTOHIT;
				hittype[var60].extra = 2 + (p->MotoSpeed >> 1);
				p->MotoSpeed -= p->MotoSpeed >> 4;
			}
		if (p->OnBoat)
		{
			if (badguy(&sprite[var60]))
			{
				hittype[var60].picnum = MOTOHIT;
				hittype[var60].extra = 2 + (p->MotoSpeed >> 1);
				p->MotoSpeed -= p->MotoSpeed >> 4;
			}
		}
		else if (badguy(&sprite[j]) && sprite[j].xrepeat > 24 && abs(s->z - sprite[j].z) < (84 << 8))
		{
			j = getangle(sprite[j].x - p->posx, sprite[j].y - p->posy);
			p->posxv -= sintable[(j + 512) & 2047] << 4;
			p->posyv -= sintable[j & 2047] << 4;
		}
		if (sprite[j].picnum == RRTILE3587)
		{
			if (!p->stairs)
			{
				p->stairs = 10;
				if ((actions & SB_CROUCH) && !p->OnMotorcycle)
				{
					cz = sprite[j].z;
					hz = 0;
					fz = sprite[j].z + (4 << 8);
				}
			}
			else
				p->stairs--;
		}
		else if (sprite[j].picnum == TOILET || sprite[j].picnum == RRTILE2121)
		{
			if ((actions & SB_CROUCH) && !p->OnMotorcycle)
				//if (Sound[436].num == 0)
				{
					S_PlayActorSound(436, p->i);
					p->last_pissed_time = 4000;
					p->eat = 0;
				}
		}
	}


	if (s->extra > 0) fi.incur_damage(p);
	else
	{
		s->extra = 0;
		p->shield_amount = 0;
	}

	p->last_extra = s->extra;

	if (p->loogcnt > 0) p->loogcnt--;
	else p->loogcnt = 0;

	if (p->fist_incs)
	{
		if (endoflevel(snum)) return;
	}

	if (p->timebeforeexit > 1 && p->last_extra > 0)
	{
		if (timedexit(snum))
			return;
	}

	if (s->extra <= 0 && !ud.god)
	{
		playerisdead(snum, psectlotag, fz, cz);
		return;
	}

	if (p->transporter_hold > 0)
	{
		p->transporter_hold--;
		if (p->transporter_hold == 0 && p->on_warping_sector)
			p->transporter_hold = 2;
	}
	if (p->transporter_hold < 0)
		p->transporter_hold++;

	if (p->newowner >= 0)
	{
		i = p->newowner;
		p->posxv = p->posyv = s->xvel = 0;

		fi.doincrements(p);

		if (p->curr_weapon == THROWINGDYNAMITE_WEAPON) processweapon(snum, actions, psect);
		return;
	}

	doubvel = TICSPERFRAME;

	checklook(snum, actions);

	if (p->on_crane >= 0)
		goto HORIZONLY;

	playerweaponsway(p, s);

	s->xvel = clamp(ksqrt((p->posx - p->bobposx) * (p->posx - p->bobposx) + (p->posy - p->bobposy) * (p->posy - p->bobposy)), 0, 512);
	if (p->on_ground) p->bobcounter += sprite[p->i].xvel >> 1;

	backuppos(p, ud.clipping == 0 && (sector[p->cursectnum].floorpicnum == MIRROR || p->cursectnum < 0 || p->cursectnum >= MAXSECTORS));

	// Shrinking code

	i = 40;

	if (psectlotag == ST_17_PLATFORM_UP || (isRRRA() && psectlotag == ST_18_ELEVATOR_DOWN))
	{
		int tmp;
		tmp = getanimationgoal(anim_floorz, p->cursectnum);
		if (tmp >= 0)
		{
			if (!S_CheckSoundPlaying(p->i, 432))
				S_PlayActorSound(432, pi);
		}
		else
			S_StopSound(432);
	}
	if (isRRRA() && p->sea_sick_stat)
	{
		p->pycount += 32;
		p->pycount &= 2047;
		if (p->SeaSick)
			p->pyoff = sintable[p->pycount] >> 2;
		else
			p->pyoff = sintable[p->pycount] >> 7;
	}

	if (psectlotag == ST_2_UNDERWATER)
	{
		underwater(snum, actions, psect, fz, cz);
	}
	else
	{
		movement(snum, actions, psect, fz, cz, shrunk, truefdist, psectlotag);
	}

	p->psectlotag = psectlotag;

	//Do the quick lefts and rights

	if (movementBlocked(snum))
	{
		doubvel = 0;
		p->posxv = 0;
		p->posyv = 0;
	}
	else if (cl_syncinput)
	{
		//p->ang += syncangvel * constant
		//ENGINE calculates angvel for you
		// may still be needed later for demo recording

		processavel(p, &sb_avel);
		applylook(&p->angle, sb_avel, &p->sync.actions, 1, p->crouch_toggle || actions & SB_CROUCH);
		apply_seasick(p, 1);
	}

	if (p->spritebridge == 0)
	{
		j = sector[s->sectnum].floorpicnum;
		k = 0;

		if (p->on_ground && truefdist <= PHEIGHT + (16 << 8))
		{
			int whichsound = j == HURTRAIL ? 0 : j == FLOORSLIME ? 1 : j == FLOORPLASMA ? 2 :
				(isRRRA() && (j == RRTILE7768 || j == RRTILE7820) ? 3 : -1);
			if (j >= 0) k = makepainsounds(snum, whichsound);
		}

		if (k)
		{
			FTA(75, p);
			p->boot_amount -= 2;
			if (p->boot_amount <= 0)
				checkavailinven(p);
		}
	}

	if (p->posxv || p->posyv || sb_fvel || sb_svel)
	{
		p->crack_time = CRACK_TIME;

		k = sintable[p->bobcounter & 2047] >> 12;

		if (isRRRA() && p->spritebridge == 0 && p->on_ground)
		{
			if (psectlotag == 1)
				p->NotOnWater = 0;
			else if (p->OnBoat)
			{
				if (psectlotag == 1234)
					p->NotOnWater = 0;
				else
					p->NotOnWater = 1;
			}
			else
				p->NotOnWater = 1;
		}

		if (truefdist < PHEIGHT + (8 << 8) && (k == 1 || k == 3))
		{
			if (p->spritebridge == 0 && p->walking_snd_toggle == 0 && p->on_ground)
			{
				switch (psectlotag)
				{
				case 0:

					if (lz >= 0 && (lz & (MAXSPRITES - 1)) == 49152)
						j = sprite[lz & (MAXSPRITES - 1)].picnum;
					else j = sector[psect].floorpicnum;
					break;
				case 1:
					if ((krand() & 1) == 0)
						if  (!isRRRA() || (!p->OnBoat && !p->OnMotorcycle && sector[p->cursectnum].hitag != 321))
							S_PlayActorSound(DUKE_ONWATER, pi);
					p->walking_snd_toggle = 1;
					break;
				}
			}
		}
		else if (p->walking_snd_toggle > 0)
			p->walking_snd_toggle--;

		if (p->jetpack_on == 0 && p->steroids_amount > 0 && p->steroids_amount < 400)
			doubvel <<= 1;

		p->posxv += ((sb_fvel * doubvel) << 6);
		p->posyv += ((sb_svel * doubvel) << 6);

		if (!isRRRA() && ((p->curr_weapon == KNEE_WEAPON && p->kickback_pic > 10 && p->on_ground) || (p->on_ground && (actions & SB_CROUCH))))
		{
			p->posxv = mulscale(p->posxv, dukefriction - 0x2000, 16);
			p->posyv = mulscale(p->posyv, dukefriction - 0x2000, 16);
		}
		else
		{
			if (psectlotag == 2)
			{
				p->posxv = mulscale(p->posxv, dukefriction - 0x1400, 16);
				p->posyv = mulscale(p->posyv, dukefriction - 0x1400, 16);
			}
			else
			{
				p->posxv = mulscale(p->posxv, dukefriction, 16);
				p->posyv = mulscale(p->posyv, dukefriction, 16);
			}
		}

		if (isRRRA() && sector[psect].floorpicnum == RRTILE7888)
		{
			if (p->OnMotorcycle)
				if (p->on_ground)
					p->moto_on_oil = 1;
		}
		else if (isRRRA() && sector[psect].floorpicnum == RRTILE7889)
		{
			if (p->OnMotorcycle)
			{
				if (p->on_ground)
					p->moto_on_mud = 1;
			}
			else if (p->boot_amount > 0)
				p->boot_amount--;
			else
			{
				p->posxv = mulscale(p->posxv, dukefriction, 16);
				p->posyv = mulscale(p->posyv, dukefriction, 16);
			}
		}
		else

			if (sector[psect].floorpicnum == RRTILE3073 || sector[psect].floorpicnum == RRTILE2702)
			{
				if (p->OnMotorcycle)
				{
					if (p->on_ground)
					{
						p->posxv = mulscale(p->posxv, dukefriction - 0x1800, 16);
						p->posyv = mulscale(p->posyv, dukefriction - 0x1800, 16);
					}
				}
				else
					if (p->boot_amount > 0)
						p->boot_amount--;
					else
					{
						p->posxv = mulscale(p->posxv, dukefriction - 0x1800, 16);
						p->posyv = mulscale(p->posyv, dukefriction - 0x1800, 16);
					}
			}

		if (abs(p->posxv) < 2048 && abs(p->posyv) < 2048)
			p->posxv = p->posyv = 0;

		if (shrunk)
		{
			p->posxv =
				mulscale16(p->posxv, dukefriction - (dukefriction >> 1) + (dukefriction >> 2));
			p->posyv =
				mulscale16(p->posyv, dukefriction - (dukefriction >> 1) + (dukefriction >> 2));
		}
	}

HORIZONLY:

	if (psectlotag == 1 || p->spritebridge == 1) i = (4L << 8);
	else i = (20L << 8);

	if (sector[p->cursectnum].lotag == 2) k = 0;
	else k = 1;

	if (ud.clipping)
	{
		j = 0;
		p->posx += p->posxv >> 14;
		p->posy += p->posyv >> 14;
		updatesector(p->posx, p->posy, &p->cursectnum);
		changespritesect(pi, p->cursectnum);
	}
	else
		j = clipmove(&p->posx, &p->posy,
			&p->posz, &p->cursectnum,
			p->posxv, p->posyv, 164L, (4L << 8), i, CLIPMASK0);

	if (p->jetpack_on == 0 && psectlotag != 2 && psectlotag != 1 && shrunk)
		p->posz += 32 << 8;

	if (j)
		fi.checkplayerhurt(p, j);
	else if (isRRRA() && p->hurt_delay2 > 0)
		p->hurt_delay2--;

	var60 = j & (MAXWALLS - 1);
	var60 = wall[j & (MAXWALLS - 1)].lotag;

	if ((j & 49152) == 32768)
	{
		if (p->OnMotorcycle)
		{
			onMotorcycleMove(snum, psect, j);
		}
		else if (p->OnBoat)
		{
			onBoatMove(snum, psect, j);
		}
		else
		{
			if (wall[j & (MAXWALLS - 1)].lotag >= 40 && wall[j & (MAXWALLS - 1)].lotag <= 44)
			{
				if (wall[j & (MAXWALLS - 1)].lotag < 44)
				{
					dofurniture(j & (MAXWALLS - 1), p->cursectnum, snum);
					pushmove(&p->posx, &p->posy, &p->posz, &p->cursectnum, 172L, (4L << 8), (4L << 8), CLIPMASK0);
				}
				else
					pushmove(&p->posx, &p->posy, &p->posz, &p->cursectnum, 172L, (4L << 8), (4L << 8), CLIPMASK0);
			}
		}
	}

	if ((j & 49152) == 49152)
	{
		var60 = j & (MAXSPRITES - 1);
		if (p->OnMotorcycle)
		{
			onMotorcycleHit(snum, var60);
		}
		else if (p->OnBoat)
		{
			onBoatHit(snum, var60);
		}
		else
			if (badguy(&sprite[var60]))
			{
				if (sprite[var60].statnum != 1)
				{
					hittype[var60].timetosleep = 0;
					if (sprite[var60].picnum == BILLYRAY)
						S_PlayActorSound(404, var60);
					else
						fi.check_fta_sounds(var60);
					changespritestat(var60, 1);
				}
			}
			else
				if (sprite[var60].picnum == RRTILE3410)
				{
					quickkill(p);
					S_PlayActorSound(446, pi);
				}
				else if (isRRRA() && sprite[var60].picnum == RRTILE2443 && sprite[var60].pal == 19)
				{
					sprite[var60].pal = 0;
					p->DrugMode = 5;
					sprite[ps[snum].i].extra = max_player_health;
				}
	}


	if (p->jetpack_on == 0)
	{
		if (s->xvel > 16)
		{
			if (psectlotag != ST_1_ABOVE_WATER && psectlotag != ST_2_UNDERWATER && p->on_ground && (!isRRRA() || !p->sea_sick_stat))
			{
				p->pycount += 52;
				p->pycount &= 2047;
				p->pyoff =
					abs(s->xvel * sintable[p->pycount]) / 1596;
			}
		}
		else if (psectlotag != ST_2_UNDERWATER && psectlotag != 1 && (!isRRRA() || !p->sea_sick_stat))
			p->pyoff = 0;
	}

	// RBG***
	setsprite(pi, p->posx, p->posy, p->posz + PHEIGHT);

	if (psectlotag == 800 && (!isRRRA() || !p->lotag800kill))
	{
		if (isRRRA()) p->lotag800kill = 1;
		quickkill(p);
		return;
	}

	if (psectlotag < 3)
	{
		psect = s->sectnum;
		if (ud.clipping == 0 && sector[psect].lotag == 31)
		{
			if (sprite[sector[psect].hitag].xvel && hittype[sector[psect].hitag].temp_data[0] == 0)
			{
				quickkill(p);
				return;
			}
		}
	}

	if (truefdist < PHEIGHT && p->on_ground && psectlotag != 1 && shrunk == 0 && sector[p->cursectnum].lotag == 1)
		if (!S_CheckActorSoundPlaying(pi, DUKE_ONWATER))
			if (!isRRRA() || (!p->OnBoat && !p->OnMotorcycle && sector[p->cursectnum].hitag != 321))
				S_PlayActorSound(DUKE_ONWATER, pi);

	if (p->cursectnum != s->sectnum)
		changespritesect(pi, p->cursectnum);

	if (ud.clipping == 0)
	{
		if (s->clipdist == 64)
			j = (pushmove(&p->posx, &p->posy, &p->posz, &p->cursectnum, 128L, (4L << 8), (4L << 8), CLIPMASK0) < 0 && furthestangle(pi, 8) < 512);
		else
			j = (pushmove(&p->posx, &p->posy, &p->posz, &p->cursectnum, 16L, (4L << 8), (4L << 8), CLIPMASK0) < 0 && furthestangle(pi, 8) < 512);
	}
	else j = 0;

	if (ud.clipping == 0)
	{
		if (abs(hittype[pi].floorz - hittype[pi].ceilingz) < (48 << 8) || j)
		{
			if (!(sector[s->sectnum].lotag & 0x8000) && (isanunderoperator(sector[s->sectnum].lotag) ||
				isanearoperator(sector[s->sectnum].lotag)))
				fi.activatebysector(s->sectnum, pi);
			if (j)
			{
				quickkill(p);
				return;
			}
		}
		else if (abs(fz - cz) < (32 << 8) && isanunderoperator(sector[psect].lotag))
			fi.activatebysector(psect, pi);
	}

	if (ud.clipping == 0 && sector[p->cursectnum].ceilingz > (sector[p->cursectnum].floorz - (12 << 8)))
	{
		quickkill(p);
		return;
	}

	if (actions & SB_CENTERVIEW || p->hard_landing)
	{
		playerCenterView(snum);
	}
	else if (actions & SB_LOOK_UP)
	{
		playerLookUp(snum, actions);
	}
	else if (actions & SB_LOOK_DOWN)
	{
		playerLookDown(snum, actions);
	}
	else if ((actions & SB_AIM_UP) && !p->OnMotorcycle)
	{
		playerAimUp(snum, actions);
	}
	else if ((actions & SB_AIM_DOWN) && !p->OnMotorcycle)
	{
		playerAimDown(snum, actions);
	}
	if (p->recoil && p->kickback_pic == 0)
	{
		short d = p->recoil >> 1;
		if (!d)
			d = 1;
		p->recoil -= d;
		p->horizon.addadjustment(-d);
	}

	if (cl_syncinput)
	{
		sethorizon(&p->horizon.horiz, PlayerHorizon(snum), &p->sync.actions, 1);
	}

	checkhardlanding(p);

	//Shooting code/changes

	if (p->show_empty_weapon > 0)
		p->show_empty_weapon--;

	if (p->show_empty_weapon == 1)
	{
		fi.addweapon(p, p->last_full_weapon);
		return;
	}
	dokneeattack(snum, pi, { FEM10, NAKED1, STATUE });


	if (fi.doincrements(p)) return;

	if (p->weapon_pos != 0)
	{
		if (p->weapon_pos == -9)
		{
			if (p->last_weapon >= 0)
			{
				p->oweapon_pos = p->weapon_pos = 10;
				//                if(p->curr_weapon == KNEE_WEAPON) p->kickback_pic = 1;
				p->last_weapon = -1;
			}
			else if (p->holster_weapon == 0)
				p->oweapon_pos = p->weapon_pos = 10;
		}
		else p->weapon_pos--;
	}

	processweapon(snum, actions, psect);
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void processmove_r(int snum, ESyncBits actions, int psect, int fz, int cz, int shrunk, int truefdist)
{
	int psectlotag = sector[psect].lotag;
	auto p = &ps[snum];
	if (psectlotag == ST_2_UNDERWATER)
	{
		underwater(snum, actions, psect, fz, cz);
	}
	else
	{
		movement(snum, actions, psect, fz, cz, shrunk, truefdist, psectlotag);
	}
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void OnMotorcycle(struct player_struct *p, int motosprite)
{
	if (!p->OnMotorcycle && !(sector[p->cursectnum].lotag == 2))
	{
		if (motosprite)
		{
			p->posx = sprite[motosprite].x;
			p->posy = sprite[motosprite].y;
			p->angle.ang = buildang(sprite[motosprite].ang);
			p->ammo_amount[MOTORCYCLE_WEAPON] = sprite[motosprite].owner;
			deletesprite(motosprite);
		}
		p->over_shoulder_on = 0;
		p->OnMotorcycle = 1;
		p->last_full_weapon = p->curr_weapon;
		p->curr_weapon = MOTORCYCLE_WEAPON;
		p->gotweapon.Set(MOTORCYCLE_WEAPON);
		p->posxv = 0;
		p->posyv = 0;
		p->horizon.horiz = q16horiz(0);
	}
	if (!S_CheckActorSoundPlaying(p->i,186))
		S_PlayActorSound(186, p->i);
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void OffMotorcycle(struct player_struct *p)
{
	short j;
	if (p->OnMotorcycle)
	{
		if (S_CheckActorSoundPlaying(p->i,188))
			S_StopSound(188,p->i);
		if (S_CheckActorSoundPlaying(p->i,187))
			S_StopSound(187,p->i);
		if (S_CheckActorSoundPlaying(p->i,186))
			S_StopSound(186,p->i);
		if (S_CheckActorSoundPlaying(p->i,214))
			S_StopSound(214,p->i);
		if (!S_CheckActorSoundPlaying(p->i,42))
			S_PlayActorSound(42, p->i);
		p->OnMotorcycle = 0;
		p->gotweapon.Clear(MOTORCYCLE_WEAPON);
		p->curr_weapon = p->last_full_weapon;
		checkavailweapon(p);
		p->horizon.horiz = q16horiz(0);
		p->moto_do_bump = 0;
		p->MotoSpeed = 0;
		p->TiltStatus = 0;
		p->moto_drink = 0;
		p->VBumpTarget = 0;
		p->VBumpNow = 0;
		p->TurbCount = 0;
		p->posxv = 0;
		p->posyv = 0;
		p->posxv -= sintable[(p->angle.ang.asbuild()+512)&2047]<<7;
		p->posyv -= sintable[p->angle.ang.asbuild()&2047]<<7;
		p->moto_underwater = 0;
		j = fi.spawn(p->i, EMPTYBIKE);
		sprite[j].ang = p->angle.ang.asbuild();
		sprite[j].xvel += sintable[(p->angle.ang.asbuild()+512)&2047]<<7;
		sprite[j].yvel += sintable[p->angle.ang.asbuild()&2047]<<7;
		sprite[j].owner = p->ammo_amount[MOTORCYCLE_WEAPON];
	}
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void OnBoat(struct player_struct *p, int boatsprite)
{
	if (!p->OnBoat)
	{
		if (boatsprite)
		{
			p->posx = sprite[boatsprite].x;
			p->posy = sprite[boatsprite].y;
			p->angle.ang = buildang(sprite[boatsprite].ang);
			p->ammo_amount[BOAT_WEAPON] = sprite[boatsprite].owner;
			deletesprite(boatsprite);
		}
		p->over_shoulder_on = 0;
		p->OnBoat = 1;
		p->last_full_weapon = p->curr_weapon;
		p->curr_weapon = BOAT_WEAPON;
		p->gotweapon.Set(BOAT_WEAPON);
		p->posxv = 0;
		p->posyv = 0;
		p->horizon.horiz = q16horiz(0);
	}
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void OffBoat(struct player_struct *p)
{
	short j;
	if (p->OnBoat)
	{
		p->OnBoat = 0;
		p->gotweapon.Clear(BOAT_WEAPON);
		p->curr_weapon = p->last_full_weapon;
		checkavailweapon(p);
		p->horizon.horiz = q16horiz(0);
		p->moto_do_bump = 0;
		p->MotoSpeed = 0;
		p->TiltStatus = 0;
		p->moto_drink = 0;
		p->VBumpTarget = 0;
		p->VBumpNow = 0;
		p->TurbCount = 0;
		p->posxv = 0;
		p->posyv = 0;
		p->posxv -= sintable[(p->angle.ang.asbuild()+512)&2047]<<7;
		p->posyv -= sintable[p->angle.ang.asbuild()&2047]<<7;
		p->moto_underwater = 0;
		j = fi.spawn(p->i, EMPTYBOAT);
		sprite[j].ang = p->angle.ang.asbuild();
		sprite[j].xvel += sintable[(p->angle.ang.asbuild()+512)&2047]<<7;
		sprite[j].yvel += sintable[p->angle.ang.asbuild()&2047]<<7;
		sprite[j].owner = p->ammo_amount[BOAT_WEAPON];
	}
}


END_DUKE_NS
