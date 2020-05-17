//-------------------------------------------------------------------------
/*
Copyright (C) 1996, 2003 - 3D Realms Entertainment
Copyright (C) 2000, 2003 - Matt Saettler (EDuke Enhancements)

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
#include "game.h"
#include "gamevar.h"
#include "player.h"
#include "names.h"
#include "macros.h"

BEGIN_DUKE_NS 

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void incur_damage_d(struct player_struct* p)
{
	int  damage = 0L, shield_damage = 0L;

	sprite[p->i].extra -= p->extra_extra8 >> 8;

	damage = sprite[p->i].extra - p->last_extra;

	if (damage < 0)
	{
		p->extra_extra8 = 0;

		if (p->shield_amount > 0)
		{
			shield_damage = damage * (20 + (rand() % 30)) / 100;
			damage -= shield_damage;

			p->shield_amount += shield_damage;

			if (p->shield_amount < 0)
			{
				damage += p->shield_amount;
				p->shield_amount = 0;
			}
		}

		sprite[p->i].extra = p->last_extra + damage;
	}
}


//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void shoot_d(int i, int atwith)
{
	short sect, hitsect, hitspr, hitwall, l, sa, p, j, k, scount;
	int sx, sy, sz, vel, zvel, hitx, hity, hitz, x, oldzvel, dal;
	unsigned char sizx, sizy;
	spritetype* const s = &sprite[i];
	if (s->picnum == TILE_APLAYER)
	{
		p = s->yvel;
	}
	else
	{
		p = -1;
	}

	
	sect = s->sectnum;
	zvel = 0;

	if (s->picnum == TILE_APLAYER)
	{
		sx = ps[p].posx;
		sy = ps[p].posy;
		sz = ps[p].posz + ps[p].pyoff + (4 << 8);
		sa = ps[p].getang();

		ps[p].crack_time = 777;

	}
	else
	{
		sa = s->ang;
		sx = s->x;
		sy = s->y;
		sz = s->z - ((s->yrepeat * tilesiz[s->picnum].y) << 1) + (4 << 8);
		if (s->picnum != ROTATEGUN)
		{
			sz -= (7 << 8);
			if (badguy(s) && s->picnum != COMMANDER)
			{
				sx += (sintable[(sa + 1024 + 96) & 2047] >> 7);
				sy += (sintable[(sa + 512 + 96) & 2047] >> 7);
			}
		}
	}

	if (isWorldTour()) 
	{ // Twentieth Anniversary World Tour
		switch (atwith) 
		{
		case FIREBALL:
		{
			if (s->extra >= 0)
				s->shade = -96;

			sz -= (4 << 7);
			if (sprite[i].picnum != BOSS5)
				vel = 840;
			else {
				vel = 968;
				sz += 6144;
			}

			if (p < 0)
			{
				sa += 16 - (krand() & 31);
				int scratch;
				j = findplayer(s, &scratch);
				zvel = (((ps[j].oposz - sz + (3 << 8))) * vel) / ldist(&sprite[ps[j].i], s);
			}
			else
			{
				zvel = 98 * (100 + ps[p].gethorizdiff());
				sx += sintable[(sa + 860) & 0x7FF] / 448;
				sy += sintable[(sa + 348) & 0x7FF] / 448;
				sz += (3 << 8);
			}

			sizx = 18;
			sizy = 18;
			if (p >= 0)
			{
				sizx = 7;
				sizy = 7;
			}

			j = EGS(sect, sx, sy, sz, atwith, -127, sizx, sizy, sa, vel, zvel, i, (short)4);
			auto spr = &sprite[j];
			spr->extra += (krand() & 7);
			if (sprite[i].picnum == BOSS5 || p >= 0)
			{
				spr->xrepeat = 40;
				spr->yrepeat = 40;
			}
			spr->yvel = p;
			spr->cstat = 128;
			spr->clipdist = 4;
			return;
		}
		case FLAMETHROWERFLAME:
			if (s->extra >= 0)
				s->shade = -96;
			vel = 400;

			k = -1;
			if (p < 0) 
			{
				j = findplayer(s, &x);
				sa = getangle(ps[j].oposx - sx, ps[j].oposy - sy);

				if (sprite[i].picnum == BOSS5) 
				{
					vel = 528;
					sz += 6144;
				}
				else if (sprite[i].picnum == BOSS3)
					sz -= 8192;

				l = ldist(&sprite[ps[j].i], s);
				if (l != 0)
					zvel = ((ps[j].oposz - sz) * vel) / l;

				if (badguy(s) && (s->hitag & face_player_smart) != 0)
					sa = (short)(s->ang + (krand() & 31) - 16);

				if (sector[s->sectnum].lotag == 2 && (krand() % 5) == 0)
					k = fi.spawn(i, WATERBUBBLE);
			}
			else
			{
				zvel = (int)(100 - ps[p].gethorizdiff()) * 81;
				if (sprite[ps[p].i].xvel != 0)
					vel = (int)((((512 - (1024
						- abs(abs(getangle(sx - ps[p].oposx, sy - ps[p].oposy) - sa) - 1024)))
						* 0.001953125f) * sprite[ps[p].i].xvel) + 400);
				if (sector[s->sectnum].lotag == 2 && (krand() % 5) == 0)
					k = fi.spawn(i, WATERBUBBLE);
			}

			if (k == -1) 
			{
				k = fi.spawn(i, atwith);
				sprite[k].xvel = (short)vel;
				sprite[k].zvel = (short)zvel;
			}

			sprite[k].x = sx + sintable[(sa + 630) & 0x7FF] / 448;
			sprite[k].y = sy + sintable[(sa + 112) & 0x7FF] / 448;
			sprite[k].z = sz - 256;
			sprite[k].sectnum = sect;
			sprite[k].cstat = 0x80;
			sprite[k].ang = sa;
			sprite[k].xrepeat = 2;
			sprite[k].yrepeat = 2;
			sprite[k].clipdist = 40;
			sprite[k].yvel = p;
			sprite[k].owner = (short)i;

			if (p == -1) 
			{
				if (sprite[i].picnum == BOSS5) 
				{
					sprite[k].x -= sintable[sa & 2047] / 56;
					sprite[k].y -= sintable[(sa + 1024 + 512) & 2047] / 56;
					sprite[k].xrepeat = 10;
					sprite[k].yrepeat = 10;
				}
			}
			return;
		case FIREFLY: // BOSS5 shot
			k = fi.spawn(i, atwith);
			sprite[k].sectnum = sect;
			sprite[k].x = sx;
			sprite[k].y = sy;
			sprite[k].z = sz;
			sprite[k].ang = sa;
			sprite[k].xvel = 500;
			sprite[k].zvel = 0;
			return;
		}
	}

	switch (atwith)
	{
	case BLOODSPLAT1:
	case BLOODSPLAT2:
	case BLOODSPLAT3:
	case BLOODSPLAT4:

		if (p >= 0)
			sa += 64 - (krand() & 127);
		else sa += 1024 + 64 - (krand() & 127);
		zvel = 1024 - (krand() & 2047);
	case KNEE:
		if (atwith == KNEE)
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

		//            writestring(sx,sy,sz,sect,sintable[(sa+512)&2047],sintable[sa&2047],zvel<<6);

		hitscan(sx, sy, sz, sect,
			sintable[(sa + 512) & 2047],
			sintable[sa & 2047], zvel << 6,
			&hitsect, &hitwall, &hitspr, &hitx, &hity, &hitz, CLIPMASK1);

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
								if (s->picnum == OOZFILTER || s->picnum == NEWBEAST)
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
				j = EGS(hitsect, hitx, hity, hitz, KNEE, -15, 0, 0, sa, 32, 0, i, 4);
				sprite[j].extra += (krand() & 7);
				if (p >= 0)
				{
					k = fi.spawn(j, SMALLSMOKE);
					sprite[k].z -= (8 << 8);
					spritesound(KICK_HIT, j);
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
				ssp(i, CLIPMASK0);
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
			if (isWW2GI())
			{
				SetGameVarID(g_iAimAngleVarID, AUTO_AIM_ANGLE, i, p);
				OnEvent(EVENT_GETAUTOAIMANGLE, i, p, -1);
				j = -1;
				if (GetGameVarID(g_iAimAngleVarID, i, p) > 0)
				{
					j = aim(s, GetGameVarID(g_iAimAngleVarID, i, p));
				}
			}
			else
				j = aim(s, AUTO_AIM_ANGLE);
			if (j >= 0)
			{
				dal = ((sprite[j].xrepeat * tilesiz[sprite[j].picnum].y) << 1) + (5 << 8);
				switch (sprite[j].picnum)
				{
				case GREENSLIME:
				case GREENSLIME + 1:
				case GREENSLIME + 2:
				case GREENSLIME + 3:
				case GREENSLIME + 4:
				case GREENSLIME + 5:
				case GREENSLIME + 6:
				case GREENSLIME + 7:
				case ROTATEGUN:
					dal -= (8 << 8);
					break;
				}
				zvel = ((sprite[j].z - sz - dal) << 8) / ldist(&sprite[ps[p].i], &sprite[j]);
				sa = getangle(sprite[j].x - sx, sprite[j].y - sy);
			}

			if (isWW2GI())
			{
				int angRange = 32;
				int zRange = 256;
				SetGameVarID(g_iAngRangeVarID, 32, i, p);
				SetGameVarID(g_iZRangeVarID, 256, i, p);
				OnEvent(EVENT_GETSHOTRANGE, i, p, -1);
				angRange = GetGameVarID(g_iAngRangeVarID, i, p);
				zRange = GetGameVarID(g_iZRangeVarID, i, p);

				sa += (angRange / 2) - (krand() & (angRange - 1));
				if (j == -1)
				{
					// no target
					zvel = (100 - ps[p].gethorizdiff()) << 5;
				}
				zvel += (zRange / 2) - (krand() & (zRange - 1));
			}
			else if (j == -1 || atwith != SHOTSPARK1)
			{
				sa += 16 - (krand() & 31);
				zvel = (100 - ps[p].gethorizdiff()) << 5;
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
		s->cstat |= 257;

		if (hitsect < 0) return;

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
				fi.spawn(k, SMALLSMOKE);
			}

			if (hitspr >= 0)
			{
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
				if (p >= 0 && (
					wall[hitwall].picnum == DIPSWITCH ||
					wall[hitwall].picnum == DIPSWITCH + 1 ||
					wall[hitwall].picnum == DIPSWITCH2 ||
					wall[hitwall].picnum == DIPSWITCH2 + 1 ||
					wall[hitwall].picnum == DIPSWITCH3 ||
					wall[hitwall].picnum == DIPSWITCH3 + 1 ||
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
			else if (hitwall >= 0) {

				fi.checkhitwall(k, hitwall, hitx, hity, hitz, SHOTSPARK1);
			}
		}

		if ((krand() & 255) < 4)
		{
			vec3_t v{ hitx, hity, hitz };
			S_PlaySound3D(PISTOL_RICOCHET, k, &v);
		}

		return;

	case FIRELASER:
	case SPIT:
	case COOLEXPLOSION1:

		if (s->extra >= 0) s->shade = -96;

		scount = 1;
		if (atwith == SPIT) vel = 292;
		else
		{
			if (atwith == COOLEXPLOSION1)
			{
				if (s->picnum == BOSS2) vel = 644;
				else vel = 348;
				sz -= (4 << 7);
			}
			else
			{
				vel = 840;
				sz -= (4 << 7);
			}
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
			sa += 16 - (krand() & 31);
			zvel = (((ps[j].oposz - sz + (3 << 8))) * vel) / ldist(&sprite[ps[j].i], s);
		}

		oldzvel = zvel;

		if (atwith == SPIT) { sizx = 18; sizy = 18, sz -= (10 << 8); }
		else
		{
			if (atwith == FIRELASER)
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

			if (atwith == COOLEXPLOSION1)
			{
				sprite[j].shade = 0;
				if (s->picnum == BOSS2)
				{
					l = sprite[j].xvel;
					sprite[j].xvel = 1024;
					ssp(j, CLIPMASK0);
					sprite[j].xvel = l;
					sprite[j].ang += 128 - (krand() & 255);
				}
			}

			sprite[j].cstat = 128;
			sprite[j].clipdist = 4;

			sa = s->ang + 32 - (krand() & 63);
			zvel = oldzvel + 512 - (krand() & 1023);

			scount--;
		}

		return;

	case FREEZEBLAST:
		sz += (3 << 8);
	case RPG:

		if (s->extra >= 0) s->shade = -96;

		scount = 1;
		vel = 644;

		j = -1;

		if (p >= 0)
		{
			j = aim(s, 48);
			if (j >= 0)
			{
				dal = ((sprite[j].xrepeat * tilesiz[sprite[j].picnum].y) << 1) + (8 << 8);
				zvel = ((sprite[j].z - sz - dal) * vel) / ldist(&sprite[ps[p].i], &sprite[j]);
				if (sprite[j].picnum != RECON)
					sa = getangle(sprite[j].x - sx, sprite[j].y - sy);
			}
			else zvel = (100 - ps[p].gethorizdiff()) * 81;
			if (atwith == RPG)
				spritesound(RPG_SHOOT, i);

		}
		else
		{
			j = findplayer(s, &x);
			sa = getangle(ps[j].oposx - sx, ps[j].oposy - sy);
			if (s->picnum == BOSS3)
			{
				int zoffs = (32 << 8);
				if (isWorldTour()) // Twentieth Anniversary World Tour
					zoffs = (int)((sprite[i].yrepeat / 80.0f) * zoffs);
				sz -= zoffs;
			}
			else if (s->picnum == BOSS2)
			{
				vel += 128;
				int zoffs = 24 << 8;
				if (isWorldTour()) // Twentieth Anniversary World Tour
					zoffs = (int)((sprite[i].yrepeat / 80.0f) * zoffs);
				sz += zoffs;
			}

			l = ldist(&sprite[ps[j].i], s);
			zvel = ((ps[j].oposz - sz) * vel) / l;

			if (badguy(s) && (s->hitag & face_player_smart))
				sa = s->ang + (krand() & 31) - 16;
		}

		if (p >= 0 && j >= 0)
			l = j;
		else l = -1;

		j = EGS(sect,
			sx + (sintable[(348 + sa + 512) & 2047] / 448),
			sy + (sintable[(sa + 348) & 2047] / 448),
			sz - (1 << 8), atwith, 0, 14, 14, sa, vel, zvel, i, 4);

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
			if (s->picnum == BOSS3)
			{
				int xoffs = sintable[sa & 2047] >> 6;
				int yoffs = sintable[(sa + 1024 + 512) & 2047] >> 6;
				int aoffs = 4;

				if ((krand() & 1) != 0) 
				{
					xoffs = -xoffs;
					yoffs = -yoffs;
					aoffs = -8;
				}

				if (isWorldTour()) // Twentieth Anniversary World Tour
				{
					float siz = sprite[i].yrepeat / 80.0f;
					xoffs *= siz;
					yoffs *= siz;
					aoffs *= siz;
				}

				sprite[j].x += xoffs;
				sprite[j].y += yoffs;
				sprite[j].ang += aoffs;

				sprite[j].xrepeat = 42;
				sprite[j].yrepeat = 42;
			}
			else if (s->picnum == BOSS2)
			{
				sprite[j].x -= sintable[sa & 2047] / 56;
				sprite[j].y -= sintable[(sa + 1024 + 512) & 2047] / 56;
				sprite[j].ang -= 8 + (krand() & 255) - 128;
				sprite[j].xrepeat = 24;
				sprite[j].yrepeat = 24;
			}
			else if (atwith != FREEZEBLAST)
			{
				sprite[j].xrepeat = 30;
				sprite[j].yrepeat = 30;
				sprite[j].extra >>= 2;
			}
		}
		else if ((isWW2GI() && aplWeaponWorksLike[ps[p].curr_weapon][p] == DEVISTATOR_WEAPON) || (!isWW2GI() && ps[p].curr_weapon == DEVISTATOR_WEAPON))
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
		if (atwith == RPG)
			sprite[j].clipdist = 4;
		else
			sprite[j].clipdist = 40;

		break;

	case HANDHOLDINGLASER:

		if (p >= 0)
			zvel = (100 - ps[p].gethorizdiff()) * 32;
		else zvel = 0;

		hitscan(sx, sy, sz - ps[p].pyoff, sect,
			sintable[(sa + 512) & 2047],
			sintable[sa & 2047],
			zvel << 6, &hitsect, &hitwall, &hitspr, &hitx, &hity, &hitz, CLIPMASK1);

		j = 0;
		if (hitspr >= 0) break;

		if (hitwall >= 0 && hitsect >= 0)
			if (((hitx - sx) * (hitx - sx) + (hity - sy) * (hity - sy)) < (290 * 290))
			{
				if (wall[hitwall].nextsector >= 0)
				{
					if (sector[wall[hitwall].nextsector].lotag <= 2 && sector[hitsect].lotag <= 2)
						j = 1;
				}
				else if (sector[hitsect].lotag <= 2)
					j = 1;
			}

		if (j == 1)
		{
			k = EGS(hitsect, hitx, hity, hitz, TRIPBOMB, -16, 4, 5, sa, 0, 0, i, 6);
			if (isWW2GI())
			{
				int lTripBombControl = GetGameVar("TRIPBOMB_CONTROL", TRIPBOMB_TRIPWIRE, -1, -1);
				if (lTripBombControl & TRIPBOMB_TIMER)
				{
					int lLifetime = GetGameVar("STICKYBOMB_LIFETIME", NAM_GRENADE_LIFETIME, -1, p);
					int lLifetimeVar = GetGameVar("STICKYBOMB_LIFETIME_VAR", NAM_GRENADE_LIFETIME_VAR, -1, p);
					// set timer.  blows up when at zero....
					sprite[k].extra = lLifetime
						+ mulscale(krand(), lLifetimeVar, 14)
						- lLifetimeVar;
				}
			}

			sprite[k].hitag = k;
			spritesound(LASERTRIP_ONWALL, k);
			sprite[k].xvel = -20;
			ssp(k, CLIPMASK0);
			sprite[k].cstat = 16;
			hittype[k].temp_data[5] = sprite[k].ang = getangle(wall[hitwall].x - wall[wall[hitwall].point2].x, wall[hitwall].y - wall[wall[hitwall].point2].y) - 512;

			if (isWW2GI() && p >= 0)
				ps[p].ammo_amount[TRIPBOMB_WEAPON]--;

		}
		return;

	case BOUNCEMINE:
	case MORTER:

		if (s->extra >= 0) s->shade = -96;

		j = ps[findplayer(s, &x)].i;
		x = ldist(&sprite[j], s);

		zvel = -x >> 1;

		if (zvel < -4096)
			zvel = -2048;
		vel = x >> 4;

		EGS(sect,
			sx + (sintable[(512 + sa + 512) & 2047] >> 8),
			sy + (sintable[(sa + 512) & 2047] >> 8),
			sz + (6 << 8), atwith, -64, 32, 32, sa, vel, zvel, i, 1);
		break;

	case GROWSPARK:

		if (p >= 0)
		{
			j = aim(s, AUTO_AIM_ANGLE);
			if (j >= 0)
			{
				dal = ((sprite[j].xrepeat * tilesiz[sprite[j].picnum].y) << 1) + (5 << 8);
				switch (sprite[j].picnum)
				{
				case GREENSLIME:
				case GREENSLIME + 1:
				case GREENSLIME + 2:
				case GREENSLIME + 3:
				case GREENSLIME + 4:
				case GREENSLIME + 5:
				case GREENSLIME + 6:
				case GREENSLIME + 7:
				case ROTATEGUN:
					dal -= (8 << 8);
					break;
				}
				zvel = ((sprite[j].z - sz - dal) << 8) / (ldist(&sprite[ps[p].i], &sprite[j]));
				sa = getangle(sprite[j].x - sx, sprite[j].y - sy);
			}
			else
			{
				sa += 16 - (krand() & 31);
				zvel = (100 - ps[p].gethorizdiff()) << 5;
				zvel += 128 - (krand() & 255);
			}

			sz -= (2 << 8);
		}
		else
		{
			j = findplayer(s, &x);
			sz -= (4 << 8);
			zvel = ((ps[j].posz - sz) << 8) / (ldist(&sprite[ps[j].i], s));
			zvel += 128 - (krand() & 255);
			sa += 32 - (krand() & 63);
		}

		k = 0;

		//            RESHOOTGROW:

		s->cstat &= ~257;
		hitscan(sx, sy, sz, sect,
			sintable[(sa + 512) & 2047],
			sintable[sa & 2047],
			zvel << 6, &hitsect, &hitwall, &hitspr, &hitx, &hity, &hitz, CLIPMASK1);

		s->cstat |= 257;

		j = EGS(sect, hitx, hity, hitz, GROWSPARK, -16, 28, 28, sa, 0, 0, i, 1);

		sprite[j].pal = 2;
		sprite[j].cstat |= 130;
		sprite[j].xrepeat = sprite[j].yrepeat = 1;

		if (hitwall == -1 && hitspr == -1 && hitsect >= 0)
		{
			if (zvel < 0 && (sector[hitsect].ceilingstat & 1) == 0)
				fi.checkhitceiling(hitsect);
		}
		else if (hitspr >= 0) fi.checkhitsprite(hitspr, j);
		else if (hitwall >= 0 && wall[hitwall].picnum != ACCESSSWITCH && wall[hitwall].picnum != ACCESSSWITCH2)
		{
			/*    if(wall[hitwall].overpicnum == MIRROR && k == 0)
				{
					l = getangle(
						wall[wall[hitwall].point2].x-wall[hitwall].x,
						wall[wall[hitwall].point2].y-wall[hitwall].y);

					sx = hitx;
					sy = hity;
					sz = hitz;
					sect = hitsect;
					sa = ((l<<1) - sa)&2047;
					sx += sintable[(sa+512)&2047]>>12;
					sy += sintable[sa&2047]>>12;

					k++;
					goto RESHOOTGROW;
				}
				else */
			fi.checkhitwall(j, hitwall, hitx, hity, hitz, atwith);
		}

		break;

	case SHRINKER:
		if (s->extra >= 0) s->shade = -96;
		if (p >= 0)
		{
			j = isNamWW2GI()? -1 : aim(s, AUTO_AIM_ANGLE);
			if (j >= 0)
			{
				dal = ((sprite[j].xrepeat * tilesiz[sprite[j].picnum].y) << 1);
				zvel = ((sprite[j].z - sz - dal - (4 << 8)) * 768) / (ldist(&sprite[ps[p].i], &sprite[j]));
				sa = getangle(sprite[j].x - sx, sprite[j].y - sy);
			}
			else zvel = (100 - ps[p].gethorizdiff()) * 98;
		}
		else if (s->statnum != 3)
		{
			j = findplayer(s, &x);
			l = ldist(&sprite[ps[j].i], s);
			zvel = ((ps[j].oposz - sz) * 512) / l;
		}
		else zvel = 0;

		j = EGS(sect,
			sx + (sintable[(512 + sa + 512) & 2047] >> 12),
			sy + (sintable[(sa + 512) & 2047] >> 12),
			sz + (2 << 8), SHRINKSPARK, -16, 28, 28, sa, 768, zvel, i, 4);

		sprite[j].cstat = 128;
		sprite[j].clipdist = 32;


		return;
	}
	return;
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void selectweapon_d(int snum, int j) // playernum, weaponnum
{
	int i, k;
	auto p = &ps[snum];
	if (p->last_pissed_time <= (26 * 218) && p->show_empty_weapon == 0 && p->kickback_pic == 0 && p->quick_kick == 0 && sprite[p->i].xrepeat > 32 && p->access_incs == 0 && p->knee_incs == 0)
	{
		if ((p->weapon_pos == 0 || (p->holster_weapon && p->weapon_pos == -9)))
		{
			if (j == 10 || j == 11)
			{
				k = p->curr_weapon;
				j = (j == 10 ? -1 : 1);	// JBF: prev (-1) or next (1) weapon choice
				i = 0;

				while ((k >= 0 && k < 10) || (PLUTOPAK && k == GROW_WEAPON && (p->subweapon & (1 << GROW_WEAPON)) != 0)
					|| (isWorldTour() && k == FLAMETHROWER_WEAPON && (p->subweapon & (1 << FLAMETHROWER_WEAPON)) != 0))
				{
					if (k == FLAMETHROWER_WEAPON) //Twentieth Anniversary World Tour
					{
						if (j == -1) k = TRIPBOMB_WEAPON;
						else k = PISTOL_WEAPON;
					}
					else if (k == GROW_WEAPON)	// JBF: this is handling next/previous with the grower selected
					{
						if (j == (unsigned int)-1)
							k = 5;
						else k = 7;

					}
					else
					{
						k += j;
						// JBF 20040116: so we don't select grower with v1.3d
						if (PLUTOPAK && k == SHRINKER_WEAPON && (p->subweapon & (1 << GROW_WEAPON)))	// JBF: activates grower
							k = GROW_WEAPON;							// if enabled
						if (isWorldTour() && k == FREEZE_WEAPON && (p->subweapon & (1 << FLAMETHROWER_WEAPON)) != 0)
							k = FLAMETHROWER_WEAPON;
					}

					if (k == -1) k = 9;
					else if (k == 10) k = 0;

					if (p->gotweapon[k] && p->ammo_amount[k] > 0)
					{
						if (PLUTOPAK)	// JBF 20040116: so we don't select grower with v1.3d
							if (k == SHRINKER_WEAPON && (p->subweapon & (1 << GROW_WEAPON)))
								k = GROW_WEAPON;
							if (isWorldTour() && k == FREEZE_WEAPON && (p->subweapon & (1 << FLAMETHROWER_WEAPON)) != 0)
								k = FLAMETHROWER_WEAPON;

						j = k;
						break;
					}
					else if (PLUTOPAK && k == GROW_WEAPON && p->ammo_amount[GROW_WEAPON] == 0 && p->gotweapon[SHRINKER_WEAPON] && p->ammo_amount[SHRINKER_WEAPON] > 0)	// JBF 20040116: added PLUTOPAK so we don't select grower with v1.3d
					{
						j = SHRINKER_WEAPON;
						p->subweapon &= ~(1 << GROW_WEAPON);
						break;
					}
					else if (PLUTOPAK && k == SHRINKER_WEAPON && p->ammo_amount[SHRINKER_WEAPON] == 0 && p->gotweapon[SHRINKER_WEAPON] && p->ammo_amount[GROW_WEAPON] > 0)	// JBF 20040116: added PLUTOPAK so we don't select grower with v1.3d
					{
						j = GROW_WEAPON;
						p->subweapon |= (1 << GROW_WEAPON);
						break;
					}
					//Twentieth Anniversary World Tour
					else if (isWorldTour() && k == FLAMETHROWER_WEAPON && p->ammo_amount[FLAMETHROWER_WEAPON] == 0 && p->gotweapon[FREEZE_WEAPON] && p->ammo_amount[FREEZE_WEAPON] > 0)
					{
						j = FREEZE_WEAPON;
						p->subweapon &= ~(1 << FLAMETHROWER_WEAPON);
						break;
					}
					else if (isWorldTour() && k == FREEZE_WEAPON && p->ammo_amount[FREEZE_WEAPON] == 0 && p->gotweapon[FLAMETHROWER_WEAPON] && p->ammo_amount[FLAMETHROWER_WEAPON] > 0)
					{
						j = FLAMETHROWER_WEAPON;
						p->subweapon |= (1 << FLAMETHROWER_WEAPON);
						break;
					}

					i++;	// absolutely no weapons, so use foot
					if (i == 10)
					{
						fi.addweapon(p, KNEE_WEAPON);
						break;
					}
				}
			}

			k = -1;


			if (j == HANDBOMB_WEAPON && p->ammo_amount[HANDBOMB_WEAPON] == 0)
			{
				k = headspritestat[1];
				while (k >= 0)
				{
					if (sprite[k].picnum == HEAVYHBOMB && sprite[k].owner == p->i)
					{
						p->gotweapon.Set(HANDBOMB_WEAPON);
						j = HANDREMOTE_WEAPON;
						break;
					}
					k = nextspritestat[k];
				}
			}

			//Twentieth Anniversary World Tour
			if (j == FREEZE_WEAPON && isWorldTour())
			{
				if (p->curr_weapon != FLAMETHROWER_WEAPON && p->curr_weapon != FREEZE_WEAPON)
				{
					if (p->ammo_amount[FLAMETHROWER_WEAPON] > 0)
					{
						if ((p->subweapon & (1 << FLAMETHROWER_WEAPON)) == (1 << FLAMETHROWER_WEAPON))
							j = FLAMETHROWER_WEAPON;
						else if (p->ammo_amount[FREEZE_WEAPON] == 0)
						{
							j = FLAMETHROWER_WEAPON;
							p->subweapon |= (1 << FLAMETHROWER_WEAPON);
						}
					}
					else if (p->ammo_amount[FREEZE_WEAPON] > 0)
						p->subweapon &= ~(1 << FLAMETHROWER_WEAPON);
				}
				else if (p->curr_weapon == FREEZE_WEAPON)
				{
					p->subweapon |= (1 << FLAMETHROWER_WEAPON);
					j = FLAMETHROWER_WEAPON;
				}
				else
					p->subweapon &= ~(1 << FLAMETHROWER_WEAPON);
			}

			if (j == SHRINKER_WEAPON && PLUTOPAK)	// JBF 20040116: so we don't select the grower with v1.3d
			{
				if (screenpeek == snum) pus = NUMPAGES;

				if (p->curr_weapon != GROW_WEAPON && p->curr_weapon != SHRINKER_WEAPON)
				{
					if (p->ammo_amount[GROW_WEAPON] > 0)
					{
						if ((p->subweapon & (1 << GROW_WEAPON)) == (1 << GROW_WEAPON))
							j = GROW_WEAPON;
						else if (p->ammo_amount[SHRINKER_WEAPON] == 0)
						{
							j = GROW_WEAPON;
							p->subweapon |= (1 << GROW_WEAPON);
						}
					}
					else if (p->ammo_amount[SHRINKER_WEAPON] > 0)
						p->subweapon &= ~(1 << GROW_WEAPON);
				}
				else if (p->curr_weapon == SHRINKER_WEAPON)
				{
					p->subweapon |= (1 << GROW_WEAPON);
					j = GROW_WEAPON;
				}
				else
					p->subweapon &= ~(1 << GROW_WEAPON);
			}

			if (p->holster_weapon)
			{
				PlayerSetInput(snum, SK_HOLSTER);
				p->weapon_pos = -9;
			}
			else if (j >= MIN_WEAPON && p->gotweapon[j] && (unsigned int)p->curr_weapon != j) switch (j)
			{
			case KNEE_WEAPON:
				fi.addweapon(p, KNEE_WEAPON);
				break;
			case PISTOL_WEAPON:
			case SHOTGUN_WEAPON:
			case CHAINGUN_WEAPON:
			case RPG_WEAPON:
			case DEVISTATOR_WEAPON:
			case FREEZE_WEAPON:
			case FLAMETHROWER_WEAPON:
			case GROW_WEAPON:
			case SHRINKER_WEAPON:

				if (p->ammo_amount[j] == 0 && p->show_empty_weapon == 0)
				{
					p->show_empty_weapon = 32;
					p->last_full_weapon = p->curr_weapon;
				}

				fi.addweapon(p, j);
				break;
			case HANDREMOTE_WEAPON:
				if (k >= 0) // Found in list of [1]'s
				{
					p->curr_weapon = HANDREMOTE_WEAPON;
					p->last_weapon = -1;
					p->weapon_pos = 10;
				}
				break;
			case HANDBOMB_WEAPON:
				if (p->ammo_amount[HANDBOMB_WEAPON] > 0 && p->gotweapon[HANDBOMB_WEAPON])
					fi.addweapon(p, HANDBOMB_WEAPON);
				break;
			case TRIPBOMB_WEAPON:
				if (p->ammo_amount[TRIPBOMB_WEAPON] > 0 && p->gotweapon[TRIPBOMB_WEAPON])
					fi.addweapon(p, TRIPBOMB_WEAPON);
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

int doincrements_d(struct player_struct* p)
{
	int snum;

	snum = sprite[p->i].yvel;
	//    j = sync[snum].avel;
	//    p->weapon_ang = -(j/5);

	p->player_par++;

	if (p->invdisptime > 0)
		p->invdisptime--;

	if (p->tipincs > 0) p->tipincs--;

	if (p->last_pissed_time > 0)
	{
		p->last_pissed_time--;

		if (p->last_pissed_time == (26 * 219))
		{
			spritesound(FLUSH_TOILET, p->i);
			if (snum == screenpeek || ud.coop == 1)
				spritesound(DUKE_PISSRELIEF, p->i);
		}

		if (p->last_pissed_time == (26 * 218))
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
			checkavailinven(p);
		if (!(p->steroids_amount & 7))
			if (snum == screenpeek || ud.coop == 1)
				spritesound(DUKE_HARTBEAT, p->i);
	}

	if (p->heat_on && p->heat_amount > 0)
	{
		p->heat_amount--;
		if (p->heat_amount == 0)
		{
			p->heat_on = 0;
			checkavailinven(p);
			spritesound(NITEVISION_ONOFF, p->i);
			setpal(p);
		}
	}

	if (p->holoduke_on >= 0)
	{
		p->holoduke_amount--;
		if (p->holoduke_amount <= 0)
		{
			spritesound(TELEPORTER, p->i);
			p->holoduke_on = -1;
			checkavailinven(p);
		}
	}

	if (p->jetpack_on && p->jetpack_amount > 0)
	{
		p->jetpack_amount--;
		if (p->jetpack_amount <= 0)
		{
			p->jetpack_on = 0;
			checkavailinven(p);
			spritesound(DUKE_JETPACK_OFF, p->i);
			stopsound(DUKE_JETPACK_IDLE);
			stopsound(DUKE_JETPACK_ON);
		}
	}

	if (p->quick_kick > 0 && sprite[p->i].pal != 1)
	{
		p->quick_kick--;
		if (p->quick_kick == 8)
			fi.shoot(p->i, KNEE);
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
				case 0:p->got_access &= (0xffff - 0x1); break;
				case 21:p->got_access &= (0xffff - 0x2); break;
				case 23:p->got_access &= (0xffff - 0x4); break;
				}
				p->access_spritenum = -1;
			}
			else
			{
				fi.checkhitswitch(snum, p->access_wallnum, 0);
				switch (wall[p->access_wallnum].pal)
				{
				case 0:p->got_access &= (0xffff - 0x1); break;
				case 21:p->got_access &= (0xffff - 0x2); break;
				case 23:p->got_access &= (0xffff - 0x4); break;
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
		if (p->knuckle_incs == 10 && !isWW2GI())
		{
			if (totalclock > 1024)
				if (snum == screenpeek || ud.coop == 1)
				{
					if (rand() & 1)
						spritesound(DUKE_CRACK, p->i);
					else spritesound(DUKE_CRACK2, p->i);
				}
			spritesound(DUKE_CRACK_FIRST, p->i);
		}
		else if (p->knuckle_incs == 22 || PlayerInput(snum, SK_FIRE))
			p->knuckle_incs = 0;

		return 1;
	}
	return 0;
}


END_DUKE_NS
