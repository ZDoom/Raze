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
#include "gamevar.h"
#include "names_d.h"

BEGIN_DUKE_NS 

void fireweapon_ww(int snum);
void operateweapon_ww(int snum, ESyncBits actions, int psect);

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

	SetGameVarID(g_iAtWithVarID, 0, p, atwith);
	SetGameVarID(g_iReturnVarID, 0, p, i);
	OnEvent(EVENT_SHOOT, i, p, -1);
	if (GetGameVarID(g_iReturnVarID, p, i) != 0)
	{
		return;
	}


	sect = s->sectnum;
	zvel = 0;

	if (s->picnum == TILE_APLAYER)
	{
		sx = ps[p].posx;
		sy = ps[p].posy;
		sz = ps[p].posz + ps[p].pyoff + (4 << 8);
		sa = ps[p].angle.ang.asbuild();

		ps[p].crack_time = CRACK_TIME;

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
				zvel = -mulscale16(ps[p].horizon.sum().asq16(), 98);
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
				zvel = -mulscale16(ps[p].horizon.sum().asq16(), 81);
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
								SectIterator it(wall[hitwall].nextsector);
								while ((k = it.NextIndex()) >= 0)
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
					S_PlayActorSound(KICK_HIT, j);
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
			SetGameVarID(g_iAimAngleVarID, AUTO_AIM_ANGLE, i, p);
			OnEvent(EVENT_GETAUTOAIMANGLE, i, p, -1);
			j = -1;
			if (GetGameVarID(g_iAimAngleVarID, i, p) > 0)
			{
				j = aim(s, GetGameVarID(g_iAimAngleVarID, i, p));
			}

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
					zvel = -ps[p].horizon.sum().asq16() >> 11;
				}
				zvel += (zRange / 2) - (krand() & (zRange - 1));
			}
			else if (j == -1)
			{
				sa += 16 - (krand() & 31);
				zvel = -ps[p].horizon.sum().asq16() >> 11;
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
									SectIterator it(wall[hitwall].nextsector);
									while ((l = it.NextIndex()) >= 0)
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
				zvel = -mulscale16(ps[p].horizon.sum().asq16(), 98);
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
			else zvel = -mulscale16(ps[p].horizon.sum().asq16(), 81);
			if (atwith == RPG)
				S_PlayActorSound(RPG_SHOOT, i);

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
				int xoffs = sintable[sa & 2047] / 56;
				int yoffs = sintable[(sa + 1024 + 512) & 2047] / 56;
				int aoffs = 8 + (krand() & 255) - 128;

				if (isWorldTour()) { // Twentieth Anniversary World Tour
					int siz = sprite[i].yrepeat;
					xoffs = Scale(xoffs, siz, 80);
					yoffs = Scale(yoffs, siz, 80);
					aoffs = Scale(aoffs, siz, 80);
				}

				sprite[j].x -= xoffs;
				sprite[j].y -= yoffs;
				sprite[j].ang -= aoffs;

				sprite[j].x -= sintable[sa & 2047] / 56;
				sprite[j].y -= sintable[(sa + 1024 + 512) & 2047] / 56;
				sprite[j].ang -=  8 + (krand() & 255) - 128;
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
			zvel = -ps[p].horizon.sum().asq16() >> 11;
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
			S_PlayActorSound(LASERTRIP_ONWALL, k);
			sprite[k].xvel = -20;
			ssp(k, CLIPMASK0);
			sprite[k].cstat = 16;
			hittype[k].temp_data[5] = sprite[k].ang = getangle(wall[hitwall].x - wall[wall[hitwall].point2].x, wall[hitwall].y - wall[wall[hitwall].point2].y) - 512;

			if (p >= 0)
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
				zvel = -ps[p].horizon.sum().asq16() >> 11;
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
			else zvel = -mulscale16(ps[p].horizon.sum().asq16(), 98);
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

void selectweapon_d(int snum, int weap) // playernum, weaponnum
{
	int i, j, k;
	auto p = &ps[snum];
	if (p->last_pissed_time <= (26 * 218) && p->show_empty_weapon == 0 && p->kickback_pic == 0 && p->quick_kick == 0 && sprite[p->i].xrepeat > 32 && p->access_incs == 0 && p->knee_incs == 0)
	{
		if ((p->weapon_pos == 0 || (p->holster_weapon && p->weapon_pos == -9)))
		{
			if (weap == WeaponSel_Alt)
			{
				switch (p->curr_weapon)
				{
					case SHRINKER_WEAPON:
						j = isPlutoPak() ? GROW_WEAPON : p->curr_weapon;
						break;
					case GROW_WEAPON:
						j = SHRINKER_WEAPON;
						break;
					case FREEZE_WEAPON:
						j = isWorldTour() ? FLAMETHROWER_WEAPON : p->curr_weapon;
						break;
					case FLAMETHROWER_WEAPON:
						j = FREEZE_WEAPON;
						break;
					default:
						j = p->curr_weapon;
						break;
				}
			}
			else if (weap == WeaponSel_Next || weap == WeaponSel_Prev)
			{
				k = p->curr_weapon;
				j = (weap == WeaponSel_Prev ? -1 : 1);	// JBF: prev (-1) or next (1) weapon choice
				i = 0;

				while ((k >= 0 && k < 10) || (isPlutoPak() && k == GROW_WEAPON && (p->subweapon & (1 << GROW_WEAPON)) != 0)
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
						if (isPlutoPak() && k == SHRINKER_WEAPON && (p->subweapon & (1 << GROW_WEAPON)))	// JBF: activates grower
							k = GROW_WEAPON;							// if enabled
						if (isWorldTour() && k == FREEZE_WEAPON && (p->subweapon & (1 << FLAMETHROWER_WEAPON)) != 0)
							k = FLAMETHROWER_WEAPON;
					}

					if (k == -1) k = 9;
					else if (k == 10) k = 0;

					if (p->gotweapon[k] && p->ammo_amount[k] > 0)
					{
						if (isPlutoPak())	// JBF 20040116: so we don't select grower with v1.3d
							if (k == SHRINKER_WEAPON && (p->subweapon & (1 << GROW_WEAPON)))
								k = GROW_WEAPON;
							if (isWorldTour() && k == FREEZE_WEAPON && (p->subweapon & (1 << FLAMETHROWER_WEAPON)) != 0)
								k = FLAMETHROWER_WEAPON;

						j = k;
						break;
					}
					else if (isPlutoPak() && k == GROW_WEAPON && p->ammo_amount[GROW_WEAPON] == 0 && p->gotweapon[SHRINKER_WEAPON] && p->ammo_amount[SHRINKER_WEAPON] > 0)	// JBF 20040116: added isPlutoPak() so we don't select grower with v1.3d
					{
						j = SHRINKER_WEAPON;
						p->subweapon &= ~(1 << GROW_WEAPON);
						break;
					}
					else if (isPlutoPak() && k == SHRINKER_WEAPON && p->ammo_amount[SHRINKER_WEAPON] == 0 && p->gotweapon[SHRINKER_WEAPON] && p->ammo_amount[GROW_WEAPON] > 0)	// JBF 20040116: added isPlutoPak() so we don't select grower with v1.3d
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
			else j = weap - 1;

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

			if (j == SHRINKER_WEAPON && isPlutoPak())	// JBF 20040116: so we don't select the grower with v1.3d
			{
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
				PlayerSetInput(snum, SB_HOLSTER);
				p->oweapon_pos = p->weapon_pos = -9;
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
					p->oweapon_pos = p->weapon_pos = 10;
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

	p->player_par++;

	if (p->invdisptime > 0)
		p->invdisptime--;

	if (p->tipincs > 0) p->tipincs--;

	if (p->last_pissed_time > 0)
	{
		p->last_pissed_time--;

		if (p->last_pissed_time == (26 * 219))
		{
			S_PlayActorSound(FLUSH_TOILET, p->i);
			if (snum == screenpeek || ud.coop == 1)
				S_PlayActorSound(DUKE_PISSRELIEF, p->i);
		}

		if (p->last_pissed_time == (26 * 218))
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
			checkavailinven(p);
		if (!(p->steroids_amount & 7))
			if (snum == screenpeek || ud.coop == 1)
				S_PlayActorSound(DUKE_HARTBEAT, p->i);
	}

	if (p->heat_on && p->heat_amount > 0)
	{
		p->heat_amount--;
		if (p->heat_amount == 0)
		{
			p->heat_on = 0;
			checkavailinven(p);
			S_PlayActorSound(NITEVISION_ONOFF, p->i);
			setpal(p);
		}
	}

	if (p->holoduke_on >= 0)
	{
		p->holoduke_amount--;
		if (p->holoduke_amount <= 0)
		{
			S_PlayActorSound(TELEPORTER, p->i);
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
			S_PlayActorSound(DUKE_JETPACK_OFF, p->i);
			S_StopSound(DUKE_JETPACK_IDLE, p->i);
			S_StopSound(DUKE_JETPACK_ON, p->i);
		}
	}

	if (p->quick_kick > 0 && sprite[p->i].pal != 1)
	{
		p->last_quick_kick = p->quick_kick + 1;
		p->quick_kick--;
		if (p->quick_kick == 8)
			fi.shoot(p->i, KNEE);
	}
	else if (p->last_quick_kick > 0)
		p->last_quick_kick--;

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
		if (p->knuckle_incs == 10 && !isWW2GI())
		{
			if (ud.levelclock > 1024)
				if (snum == screenpeek || ud.coop == 1)
				{
					if (rand() & 1)
						S_PlayActorSound(DUKE_CRACK, p->i);
					else S_PlayActorSound(DUKE_CRACK2, p->i);
				}
			S_PlayActorSound(DUKE_CRACK_FIRST, p->i);
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

void checkweapons_d(struct player_struct* p)
{
	static const short weapon_sprites[MAX_WEAPONS] = { KNEE, FIRSTGUNSPRITE, SHOTGUNSPRITE,
			CHAINGUNSPRITE, RPGSPRITE, HEAVYHBOMB, SHRINKERSPRITE, DEVISTATORSPRITE,
			TRIPBOMBSPRITE, FREEZESPRITE, HEAVYHBOMB, SHRINKERSPRITE };

	int cw;

	if (isWW2GI())
	{
		int snum = sprite[p->i].yvel;
		cw = aplWeaponWorksLike[p->curr_weapon][snum];
	}
	else 
		cw = p->curr_weapon;


	if (cw < 1 || cw >= MAX_WEAPONS) return;

	if (cw)
	{
		if (krand() & 1)
			fi.spawn(p->i, weapon_sprites[cw]);
		else switch (cw)
		{
		case RPG_WEAPON:
		case HANDBOMB_WEAPON:
			fi.spawn(p->i, EXPLOSION2);
			break;
		}
	}
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

static void operateJetpack(int snum, ESyncBits actions, int psectlotag, int fz, int cz, int shrunk)
{
	int j;
	auto p = &ps[snum];
	int pi = p->i;
	p->on_ground = 0;
	p->jumping_counter = 0;
	p->hard_landing = 0;
	p->falling_counter = 0;

	p->pycount += 32;
	p->pycount &= 2047;
	p->pyoff = sintable[p->pycount] >> 7;

	if (p->jetpack_on && S_CheckActorSoundPlaying(pi, DUKE_SCREAM))
	{
		S_StopSound(DUKE_SCREAM, pi);
	}

	if (p->jetpack_on < 11)
	{
		p->jetpack_on++;
		p->posz -= (p->jetpack_on << 7); //Goin up
	}
	else if (p->jetpack_on == 11 && !S_CheckActorSoundPlaying(pi, DUKE_JETPACK_IDLE))
		S_PlayActorSound(DUKE_JETPACK_IDLE, pi);

	if (shrunk) j = 512;
	else j = 2048;

	if (actions & SB_JUMP)                            //A (soar high)
	{
		// jump
		SetGameVarID(g_iReturnVarID, 0, pi, snum);
		OnEvent(EVENT_SOARUP, pi, snum, -1);
		if (GetGameVarID(g_iReturnVarID, pi, snum) == 0)
		{
			p->posz -= j;
			p->crack_time = CRACK_TIME;
		}
	}

	if (actions & SB_CROUCH)                            //Z (soar low)
	{
		// crouch
		SetGameVarID(g_iReturnVarID, 0, pi, snum);
		OnEvent(EVENT_SOARDOWN, pi, snum, -1);
		if (GetGameVarID(g_iReturnVarID, pi, snum) == 0)
		{
			p->posz += j;
			p->crack_time = CRACK_TIME;
		}
	}

	int k;
	if (shrunk == 0 && (psectlotag == 0 || psectlotag == 2)) k = 32;
	else k = 16;

	if (psectlotag != 2 && p->scuba_on == 1)
		p->scuba_on = 0;

	if (p->posz > (fz - (k << 8)))
		p->posz += ((fz - (k << 8)) - p->posz) >> 1;
	if (p->posz < (hittype[pi].ceilingz + (18 << 8)))
		p->posz = hittype[pi].ceilingz + (18 << 8);

}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

static void movement(int snum, ESyncBits actions, int psect, int fz, int cz, int shrunk, int truefdist, int psectlotag)
{
	int j;
	auto p = &ps[snum];
	int pi = p->i;

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
					p->footprintpal = 8;
				else p->footprintpal = 0;
				p->footprintshade = 0;
			}
		}
	}
	else
	{
		footprints(snum);
	}

	if (p->posz < (fz - (i << 8))) //falling
	{

		// not jumping or crouching
		if ((actions & (SB_JUMP|SB_CROUCH)) == 0 && p->on_ground && (sector[psect].floorstat & 2) && p->posz >= (fz - (i << 8) - (16 << 8)))
			p->posz = fz - (i << 8);
		else
		{
			p->on_ground = 0;
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
					if (p->falling_counter > 62) quickkill(p);

					else if (p->falling_counter > 9)
					{
						j = p->falling_counter;
						sprite[pi].extra -= j - (krand() & 3);
						if (sprite[pi].extra <= 0)
						{
							S_PlayActorSound(SQUISHED, pi);
							SetPlayerPal(p, PalEntry(63, 63, 0, 0));
						}
						else
						{
							S_PlayActorSound(DUKE_LAND, pi);
							S_PlayActorSound(DUKE_LAND_HURT, pi);
						}

						SetPlayerPal(p, PalEntry(32, 16, 0, 0));
					}
					else if (p->poszv > 2048) S_PlayActorSound(DUKE_LAND, pi);
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

		if (actions & SB_CROUCH)
		{
			playerCrouch(snum);
		}

		// jumping
		if ((actions & SB_JUMP) == 0 && p->jumping_toggle == 1)
			p->jumping_toggle = 0;

		else if ((actions & SB_JUMP))
		{
			playerJump(snum, fz, cz);
		}

		if (p->jumping_counter && (actions & SB_JUMP) == 0)
			p->jumping_toggle = 0;
	}

	if (p->jumping_counter)
	{
		if ((actions & SB_JUMP) == 0 && p->jumping_toggle == 1)
			p->jumping_toggle = 0;

		if (p->jumping_counter < (1024 + 256))
		{
			if (psectlotag == 1 && p->jumping_counter > 768)
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

	// under water
	p->jumping_counter = 0;

	p->pycount += 32;
	p->pycount &= 2047;
	p->pyoff = sintable[p->pycount] >> 7;

	if (!S_CheckActorSoundPlaying(pi, DUKE_UNDERWATER))
		S_PlayActorSound(DUKE_UNDERWATER, pi);

	if (actions & SB_JUMP)
	{
		// jump
		if (p->poszv > 0) p->poszv = 0;
		p->poszv -= 348;
		if (p->poszv < -(256 * 6)) p->poszv = -(256 * 6);
	}
	else if (actions & SB_CROUCH)
	{
		// crouch
		if (p->poszv < 0) p->poszv = 0;
		p->poszv += 348;
		if (p->poszv > (256 * 6)) p->poszv = (256 * 6);
	}
	else
	{
		// normal view
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
			sintable[(p->angle.ang.asbuild() + 512 + 64 - (global_random & 128)) & 2047] >> 6;
		sprite[j].y +=
			sintable[(p->angle.ang.asbuild() + 64 - (global_random & 128)) & 2047] >> 6;
		sprite[j].xrepeat = 3;
		sprite[j].yrepeat = 2;
		sprite[j].z = p->posz + (8 << 8);
	}
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

int operateTripbomb(int snum)
{
	auto p = &ps[snum];
	int pi = p->i;

	int sx, sy, sz;
	short sect, hw, hitsp;

	hitscan(p->posx, p->posy, p->posz,
		p->cursectnum, sintable[(p->angle.ang.asbuild() + 512) & 2047],
		sintable[p->angle.ang.asbuild() & 2047], -p->horizon.sum().asq16() >> 11,
		&sect, &hw, &hitsp, &sx, &sy, &sz, CLIPMASK1);

	if (sect < 0 || hitsp >= 0)
		return 0;

	if (hw >= 0 && sector[sect].lotag > 2)
		return 0;

	if (hw >= 0 && wall[hw].overpicnum >= 0)
		if (wall[hw].overpicnum == BIGFORCE)
			return 0;

	int j;
	SectIterator it(sect);
	while ((j = it.NextIndex()) >= 0)
	{
		auto sj = &sprite[j];
		if (sj->picnum == TRIPBOMB &&
			abs(sj->z - sz) < (12 << 8) && ((sj->x - sx) * (sj->x - sx) + (sj->y - sy) * (sj->y - sy)) < (290 * 290))
			return 0;
	}

	if (j == -1 && hw >= 0 && (wall[hw].cstat & 16) == 0)
		if ((wall[hw].nextsector >= 0 && sector[wall[hw].nextsector].lotag <= 2) || (wall[hw].nextsector == -1 && sector[sect].lotag <= 2))
			if (((sx - p->posx) * (sx - p->posx) + (sy - p->posy) * (sy - p->posy)) < (290 * 290))
			{
				p->posz = p->oposz;
				p->poszv = 0;
				return 1;
			}

	return 0;
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
	else switch (p->curr_weapon)
	{
	case HANDBOMB_WEAPON:
		p->hbomb_hold_delay = 0;
		if (p->ammo_amount[HANDBOMB_WEAPON] > 0)
			p->kickback_pic = 1;
		break;
	case HANDREMOTE_WEAPON:
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


	case CHAINGUN_WEAPON:
		if (p->ammo_amount[CHAINGUN_WEAPON] > 0) // && p->random_club_frame == 0)
			p->kickback_pic = 1;
		break;

	case SHOTGUN_WEAPON:
		if (p->ammo_amount[SHOTGUN_WEAPON] > 0 && p->random_club_frame == 0)
			p->kickback_pic = 1;
		break;
	case TRIPBOMB_WEAPON:
		if (p->ammo_amount[TRIPBOMB_WEAPON] > 0)
		{
			if (operateTripbomb(snum))
				p->kickback_pic = 1;
		}
		break;

	case SHRINKER_WEAPON:
	case GROW_WEAPON:
		if (p->curr_weapon == GROW_WEAPON)
		{
			if (p->ammo_amount[GROW_WEAPON] > 0)
			{
				p->kickback_pic = 1;
				S_PlayActorSound(EXPANDERSHOOT, pi);
			}
		}
		else if (p->ammo_amount[SHRINKER_WEAPON] > 0)
		{
			p->kickback_pic = 1;
			S_PlayActorSound(SHRINKER_FIRE, pi);
		}
		break;

	case FREEZE_WEAPON:
		if (p->ammo_amount[FREEZE_WEAPON] > 0)
		{
			p->kickback_pic = 1;
			S_PlayActorSound(CAT_FIRE, pi);
		}
		break;
	case DEVISTATOR_WEAPON:
		if (p->ammo_amount[DEVISTATOR_WEAPON] > 0)
		{
			p->kickback_pic = 1;
			p->hbomb_hold_delay = !p->hbomb_hold_delay;
			S_PlayActorSound(CAT_FIRE, pi);
		}
		break;

	case RPG_WEAPON:
		if (p->ammo_amount[RPG_WEAPON] > 0)
		{
			p->kickback_pic = 1;
		}
		break;

	case FLAMETHROWER_WEAPON: // Twentieth Anniversary World Tour
		if (isWorldTour() && p->ammo_amount[FLAMETHROWER_WEAPON] > 0) 
		{
			p->kickback_pic = 1;
			if (sector[p->cursectnum].lotag != 2)
				S_PlayActorSound(FLAMETHROWER_INTRO, pi);
		}
		break;

	case KNEE_WEAPON:
		if (p->quick_kick == 0)
		{
			p->kickback_pic = 1;
		}
		break;
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

	// already firing...

	switch (p->curr_weapon)
	{
	case HANDBOMB_WEAPON:	// grenade in NAM
		if (p->kickback_pic == 6 && (actions & SB_FIRE))
		{
			p->rapid_fire_hold = 1;
			break;
		}
		p->kickback_pic++;
		if (p->kickback_pic == 12)
		{
			p->ammo_amount[HANDBOMB_WEAPON]--;

			if (p->on_ground && (actions & SB_CROUCH))
			{
				k = 15;
				i = mulscale16(p->horizon.sum().asq16(), 20);
			}
			else
			{
				k = 140;
				i = -512 - mulscale16(p->horizon.sum().asq16(), 20);
			}

			j = EGS(p->cursectnum,
				p->posx + (sintable[(p->angle.ang.asbuild() + 512) & 2047] >> 6),
				p->posy + (sintable[p->angle.ang.asbuild() & 2047] >> 6),
				p->posz, HEAVYHBOMB, -16, 9, 9,
				p->angle.ang.asbuild(), (k + (p->hbomb_hold_delay << 5)), i, pi, 1);

			if (isNam())
			{
				sprite[j].extra = mulscale(krand(), NAM_GRENADE_LIFETIME_VAR, 14);
			}

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
		else if (p->kickback_pic > 19)
		{
			p->okickback_pic = p->kickback_pic = 0;
			// don't change to remote when in NAM: grenades are timed
			if (isNam()) checkavailweapon(p);
			else
			{
				p->curr_weapon = HANDREMOTE_WEAPON;
				p->last_weapon = -1;
				p->oweapon_pos = p->weapon_pos = 10;
			}
		}

		break;


	case HANDREMOTE_WEAPON:	// knife in NAM

		p->kickback_pic++;

		if (p->kickback_pic == 2)
		{
			p->hbomb_on = 0;
		}

		if (p->kickback_pic == 10)
		{
			p->okickback_pic = p->kickback_pic = 0;
			int weapon = isNam() ? TRIPBOMB_WEAPON : HANDBOMB_WEAPON;

			if (p->ammo_amount[weapon] > 0)
				fi.addweapon(p, weapon);
			else
				checkavailweapon(p);
		}
		break;

	case PISTOL_WEAPON:	// m-16 in NAM
		if (p->kickback_pic == 1)
		{
			fi.shoot(pi, SHOTSPARK1);
			S_PlayActorSound(PISTOL_FIRE, pi);
			lastvisinc = ud.levelclock + 32;
			p->visibility = 0;
		}

		else if (p->kickback_pic == 2)
			fi.spawn(pi, SHELL);

		p->kickback_pic++;

		if (p->kickback_pic >= 5)
		{
			if (p->ammo_amount[PISTOL_WEAPON] <= 0 || (p->ammo_amount[PISTOL_WEAPON] % (isNam() ? 20 : 12)))
			{
				p->okickback_pic = p->kickback_pic = 0;
				checkavailweapon(p);
			}
			else
			{
				switch (p->kickback_pic)
				{
				case 5:
					S_PlayActorSound(EJECT_CLIP, pi);
					break;
					//#ifdef NAM								
					//                            case WEAPON2_RELOAD_TIME - 15:
					//#else
				case 8:
					//#endif
					S_PlayActorSound(INSERT_CLIP, pi);
					break;
				}
			}
		}

		// 3 second re-load time
		if (p->kickback_pic == (isNam() ? 50 : 27))
		{
			p->okickback_pic = p->kickback_pic = 0;
			checkavailweapon(p);
		}
		break;

	case SHOTGUN_WEAPON:

		p->kickback_pic++;

		if (p->kickback_pic == 4)
		{
			fi.shoot(pi, SHOTGUN);
			fi.shoot(pi, SHOTGUN);
			fi.shoot(pi, SHOTGUN);
			fi.shoot(pi, SHOTGUN);
			fi.shoot(pi, SHOTGUN);
			fi.shoot(pi, SHOTGUN);
			fi.shoot(pi, SHOTGUN);
			p->ammo_amount[SHOTGUN_WEAPON]--;

			S_PlayActorSound(SHOTGUN_FIRE, pi);

			lastvisinc = ud.levelclock + 32;
			p->visibility = 0;

		}

		switch(p->kickback_pic)
		{
		case 13:
			checkavailweapon(p);
			break;
		case 15:
			S_PlayActorSound(SHOTGUN_COCK, pi);
			break;
		case 17:
		case 20:
			p->kickback_pic++;
			break;
		case 24:
			j = fi.spawn(pi, SHOTGUNSHELL);
			sprite[j].ang += 1024;
			ssp(j, CLIPMASK0);
			sprite[j].ang += 1024;
			p->kickback_pic++;
			break;
		case 31:
			p->okickback_pic = p->kickback_pic = 0;
			return;
		}
		break;

	case CHAINGUN_WEAPON:	// m-60 in NAM 

		p->kickback_pic++;

		if (p->kickback_pic <= 12)
		{
			if (((p->kickback_pic) % 3) == 0)
			{
				p->ammo_amount[CHAINGUN_WEAPON]--;

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
				lastvisinc = ud.levelclock + 32;
				p->visibility = 0;
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
			if (actions & SB_FIRE) p->okickback_pic = p->kickback_pic = 1;
			else p->okickback_pic = p->kickback_pic = 0;
		}

		break;

	case GROW_WEAPON:		// m-14 with scope (sniper rifle)

		bool check;
		if (isNam())
		{
			p->kickback_pic++;
			check = (p->kickback_pic == 3);
		}
		else
		{
			check = (p->kickback_pic > 3);
		}
		if (check)
		{
			// fire now, but don't reload right away...
			if (isNam())
			{
				p->kickback_pic++;
				if (p->ammo_amount[p->curr_weapon] <= 1)
					p->okickback_pic = p->kickback_pic = 0;
			}
			else
				p->okickback_pic = p->kickback_pic = 0;
			p->ammo_amount[p->curr_weapon]--;
			fi.shoot(pi, GROWSPARK);

			//#ifdef NAM
			//#else
			if (!(aplWeaponFlags[p->curr_weapon][snum] & WEAPON_FLAG_NOVISIBLE))
			{
				// make them visible if not set...
				p->visibility = 0;
				lastvisinc = ud.levelclock + 32;
			}
			checkavailweapon(p);
			//#endif
		}
		else if (!isNam()) p->kickback_pic++;
		if (isNam() && p->kickback_pic > aplWeaponReload[p->curr_weapon][snum])	// 30)
		{
			// reload now...
			p->okickback_pic = p->kickback_pic = 0;
			if (!(aplWeaponFlags[p->curr_weapon][snum] & WEAPON_FLAG_NOVISIBLE))
			{
				// make them visible if not set...
				p->visibility = 0;
				lastvisinc = ud.levelclock + 32;
			}
			checkavailweapon(p);
		}
		break;

	case SHRINKER_WEAPON:	// m-79 in NAM (Grenade launcher)
		if ((!isNam() && p->kickback_pic > 10) || (isNam() && p->kickback_pic == 10))
		{
			if (isNam()) p->kickback_pic++; // fire now, but wait for reload...
			else p->okickback_pic = p->kickback_pic = 0;

			p->ammo_amount[SHRINKER_WEAPON]--;
			fi.shoot(pi, SHRINKER);

			if (!isNam())
			{
				p->visibility = 0;
				//flashColor = 176 + (252 << 8) + (120 << 16);
				lastvisinc = ud.levelclock + 32;
				checkavailweapon(p);
			}
		}
		else if (isNam() && p->kickback_pic > 30)
		{
			p->okickback_pic = p->kickback_pic = 0;
			p->visibility = 0;
			lastvisinc = ud.levelclock + 32;
			checkavailweapon(p);
		}
		else p->kickback_pic++;
		break;

	case DEVISTATOR_WEAPON:
		if (p->kickback_pic)
		{
			p->kickback_pic++;

			if (isNam())
			{
				if ((p->kickback_pic >= 2) &&
					p->kickback_pic < 5 &&
					(p->kickback_pic & 1))
				{
					p->visibility = 0;
					lastvisinc = ud.levelclock + 32;
					fi.shoot(pi, RPG);
					p->ammo_amount[DEVISTATOR_WEAPON]--;
					checkavailweapon(p);
				}
				if (p->kickback_pic > 5) p->okickback_pic = p->kickback_pic = 0;
			}
			else if (p->kickback_pic & 1)
			{
				p->visibility = 0;
				lastvisinc = ud.levelclock + 32;
				fi.shoot(pi, RPG);
				p->ammo_amount[DEVISTATOR_WEAPON]--;
				checkavailweapon(p);
				if (p->ammo_amount[DEVISTATOR_WEAPON] <= 0) p->okickback_pic = p->kickback_pic = 0;
			}
			if (p->kickback_pic > 5) p->okickback_pic = p->kickback_pic = 0;
		}
		break;
	case FREEZE_WEAPON:
		// flame thrower in NAM

		if (p->kickback_pic < 4)
		{
			p->kickback_pic++;
			if (p->kickback_pic == 3)
			{
				p->ammo_amount[p->curr_weapon]--;

				p->visibility = 0;
				lastvisinc = ud.levelclock + 32;
				fi.shoot(pi, FREEZEBLAST);
				checkavailweapon(p);
			}
			if (sprite[p->i].xrepeat < 32)
			{
				p->okickback_pic = p->kickback_pic = 0; break;
			}
		}
		else
		{
			if (actions & SB_FIRE)
			{
				p->okickback_pic = p->kickback_pic = 1;
				S_PlayActorSound(CAT_FIRE, pi);
			}
			else p->okickback_pic = p->kickback_pic = 0;
		}
		break;

	case FLAMETHROWER_WEAPON:
		if (!isWorldTour()) // Twentieth Anniversary World Tour
			break;
		p->kickback_pic++;
		if (p->kickback_pic == 2) 
		{
			if (sector[p->cursectnum].lotag != 2) 
			{
				p->ammo_amount[FLAMETHROWER_WEAPON]--;
				if (snum == screenpeek)
					g_visibility = 0;
				fi.shoot(pi, FIREBALL);
			}
			checkavailweapon(p);
		}
		else if (p->kickback_pic == 16) 
		{
			if ((actions & SB_FIRE) != 0)
			{
				p->okickback_pic = p->kickback_pic = 1;
				S_PlayActorSound(FLAMETHROWER_INTRO, pi);
			}
			else
				p->okickback_pic = p->kickback_pic = 0;
		}
		break;

	case TRIPBOMB_WEAPON:	// Claymore in NAM
		if (p->kickback_pic < 4)
		{
			p->posz = p->oposz;
			p->poszv = 0;
			if (p->kickback_pic == 3)
				fi.shoot(pi, HANDHOLDINGLASER);
		}
		if (p->kickback_pic == 16)
		{
			p->okickback_pic = p->kickback_pic = 0;
			checkavailweapon(p);
			p->oweapon_pos = p->weapon_pos = -9;
		}
		else p->kickback_pic++;
		break;
	case KNEE_WEAPON:
		p->kickback_pic++;

		if (p->kickback_pic == 7) fi.shoot(pi, KNEE);
		else if (p->kickback_pic == 14)
		{
			if (actions & SB_FIRE)
				p->okickback_pic = p->kickback_pic = 1 + (krand() & 3);
			else p->okickback_pic = p->kickback_pic = 0;
		}

		if (p->wantweaponfire >= 0)
			checkavailweapon(p);
		break;

	case RPG_WEAPON:	// m-72 in NAM (LAW)
		p->kickback_pic++;
		if (p->kickback_pic == 4)
		{
			p->ammo_amount[RPG_WEAPON]--;
			lastvisinc = ud.levelclock + 32;
			p->visibility = 0;
			fi.shoot(pi, RPG);
			checkavailweapon(p);
		}
		else if (p->kickback_pic == (cl_dukefixrpgrecoil ? 13 : 20))
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
	int shrunk = (s->yrepeat < 32);

	// Set maximum for pistol slightly higher if playing with `cl_showmagamount 1`.
	if (!cl_showmagamt)
	{
		if (p->ammo_amount[PISTOL_WEAPON] > PISTOL_MAXDEFAULT)
			p->ammo_amount[PISTOL_WEAPON] = PISTOL_MAXDEFAULT;

		if (max_ammo_amount[PISTOL_WEAPON] != PISTOL_MAXDEFAULT)
			max_ammo_amount[PISTOL_WEAPON] = PISTOL_MAXDEFAULT;
	}
	else
	{
		short pistolAddition = 4;
		short pistolNewMaximum = PISTOL_MAXDEFAULT + pistolAddition;

		if (p->ammo_amount[PISTOL_WEAPON] == PISTOL_MAXDEFAULT && max_ammo_amount[PISTOL_WEAPON] == PISTOL_MAXDEFAULT)
			p->ammo_amount[PISTOL_WEAPON] += pistolAddition;

		if (max_ammo_amount[PISTOL_WEAPON] != pistolNewMaximum)
			max_ammo_amount[PISTOL_WEAPON] = pistolNewMaximum;
	}

	if (isNamWW2GI() && (actions & SB_HOLSTER)) // 'Holster Weapon
	{
		if (isWW2GI())
		{
			SetGameVarID(g_iReturnVarID, 0, pi, snum);
			SetGameVarID(g_iWeaponVarID, p->curr_weapon, pi, snum);
			SetGameVarID(g_iWorksLikeVarID, aplWeaponWorksLike[p->curr_weapon][snum], pi, snum);
			OnEvent(EVENT_HOLSTER, pi, snum, -1);
			if (GetGameVarID(g_iReturnVarID, pi, snum) == 0)
			{
				// now it uses the game definitions...
				if (aplWeaponFlags[p->curr_weapon][snum] & WEAPON_FLAG_HOLSTER_CLEARS_CLIP)
				{
					if (p->ammo_amount[p->curr_weapon] > aplWeaponClip[p->curr_weapon][snum]
						&& (p->ammo_amount[p->curr_weapon] % aplWeaponClip[p->curr_weapon][snum]) != 0)
					{
						// throw away the remaining clip
						p->ammo_amount[p->curr_weapon] -=
							p->ammo_amount[p->curr_weapon] % aplWeaponClip[p->curr_weapon][snum];
						//				p->kickback_pic = aplWeaponFireDelay[p->curr_weapon][snum]+1;	// animate, but don't shoot...
						p->kickback_pic = aplWeaponTotalTime[p->curr_weapon][snum] + 1;	// animate, but don't shoot...
						actions &= ~SB_FIRE; // not firing...
					}
					return;
				}
			}
		}
		else if (p->curr_weapon == PISTOL_WEAPON)
		{
			if (p->ammo_amount[PISTOL_WEAPON] > 20)
			{
				// throw away the remaining clip
				p->ammo_amount[PISTOL_WEAPON] -= p->ammo_amount[PISTOL_WEAPON] % 20;
				p->kickback_pic = 3;	// animate, but don't shoot...
				actions &= ~SB_FIRE; // not firing...
			}
			return;
		}
	}


	if (isWW2GI() && (aplWeaponFlags[p->curr_weapon][snum] & WEAPON_FLAG_GLOWS))
		p->random_club_frame += 64; // Glowing

	if (!isWW2GI() && (p->curr_weapon == SHRINKER_WEAPON || p->curr_weapon == GROW_WEAPON))
		p->random_club_frame += 64; // Glowing

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
		if (!isWW2GI()) fireweapon(snum);
		else fireweapon_ww(snum);
	}
	else if (p->kickback_pic)
	{
		if (!isWW2GI()) operateweapon(snum, actions, psect);
		else operateweapon_ww(snum, actions, psect);
	}
}
//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void processinput_d(int snum)
{
	int j, i, k, doubvel, fz, cz, hz, lz, truefdist;
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
	p->spritebridge = 0;

	shrunk = (s->yrepeat < 32);
	getzrange(p->posx, p->posy, p->posz, psect, &cz, &hz, &fz, &lz, 163L, CLIPMASK0);

	j = getflorzofslope(psect, p->posx, p->posy);

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
	}

	if (lz >= 0 && (lz & 49152) == 49152)
	{
		j = lz & (MAXSPRITES - 1);

		if ((sprite[j].cstat & 33) == 33)
		{
			psectlotag = 0;
			p->footprintcount = 0;
			p->spritebridge = 1;
		}
		else if (badguy(&sprite[j]) && sprite[j].xrepeat > 24 && abs(s->z - sprite[j].z) < (84 << 8))
		{
			j = getangle(sprite[j].x - p->posx, sprite[j].y - p->posy);
			p->posxv -= sintable[(j + 512) & 2047] << 4;
			p->posyv -= sintable[j & 2047] << 4;
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

		if (isWW2GI() && aplWeaponWorksLike[p->curr_weapon][snum] == HANDREMOTE_WEAPON) processweapon(snum, actions, psect);
		if (!isWW2GI() && p->curr_weapon == HANDREMOTE_WEAPON) processweapon(snum, actions, psect);
		return;
	}

	doubvel = TICSPERFRAME;

	checklook(snum,actions);

	if (p->on_crane >= 0)
		goto HORIZONLY;

	playerweaponsway(p, s);

	s->xvel = clamp(ksqrt((p->posx - p->bobposx) * (p->posx - p->bobposx) + (p->posy - p->bobposy) * (p->posy - p->bobposy)), 0, 512);
	if (p->on_ground) p->bobcounter += sprite[p->i].xvel >> 1;

	backuppos(p, ud.clipping == 0 && (sector[p->cursectnum].floorpicnum == MIRROR || p->cursectnum < 0 || p->cursectnum >= MAXSECTORS));

	// Shrinking code

	i = 40;

	if (psectlotag == ST_2_UNDERWATER)
	{
		underwater(snum, actions, psect, fz, cz);
	}

	else if (p->jetpack_on)
	{
		operateJetpack(snum, actions, psectlotag, fz, cz, shrunk);
	}
	else if (psectlotag != ST_2_UNDERWATER)
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
	}

	if (p->spritebridge == 0)
	{
		j = sector[s->sectnum].floorpicnum;

		if (j == PURPLELAVA || sector[s->sectnum].ceilingpicnum == PURPLELAVA)
		{
			if (p->boot_amount > 0)
			{
				p->boot_amount--;
				p->inven_icon = 7;
				if (p->boot_amount <= 0)
					checkavailinven(p);
			}
			else
			{
				if (!S_CheckActorSoundPlaying(p->i, DUKE_LONGTERM_PAIN))
					S_PlayActorSound(DUKE_LONGTERM_PAIN, p->i);
				SetPlayerPal(p, PalEntry(32, 0, 8, 0));
				s->extra--;
			}
		}

		k = 0;

		if (p->on_ground && truefdist <= PHEIGHT + (16 << 8))
		{
			int whichsound = j == HURTRAIL ? 0 : j == FLOORSLIME ? 1 : j == FLOORPLASMA ? 2 : -1;
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

					switch (j)
					{
					case PANNEL1:
					case PANNEL2:
						S_PlayActorSound(DUKE_WALKINDUCTS, pi);
						p->walking_snd_toggle = 1;
						break;
					}
					break;
				case 1:
					if ((krand() & 1) == 0)
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

		bool check;

		if (!isWW2GI()) check = ((p->curr_weapon == KNEE_WEAPON && p->kickback_pic > 10 && p->on_ground) || (p->on_ground && (actions & SB_CROUCH)));
		else check = ((aplWeaponWorksLike[p->curr_weapon][snum] == KNEE_WEAPON && p->kickback_pic > 10 && p->on_ground) || (p->on_ground && (actions & SB_CROUCH)));
		if (check)
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

	if (p->jetpack_on == 0)
	{
		if (s->xvel > 16)
		{
			if (psectlotag != 1 && psectlotag != 2 && p->on_ground)
			{
				p->pycount += 52;
				p->pycount &= 2047;
				p->pyoff =
					abs(s->xvel * sintable[p->pycount]) / 1596;
			}
		}
		else if (psectlotag != 2 && psectlotag != 1)
			p->pyoff = 0;
	}

	// RBG***
	setsprite(pi, p->posx, p->posy, p->posz + PHEIGHT);

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
			S_PlayActorSound(DUKE_ONWATER, pi);

	if (p->cursectnum != s->sectnum)
		changespritesect(pi, p->cursectnum);

	if (ud.clipping == 0)
		j = (pushmove(&p->posx, &p->posy, &p->posz, &p->cursectnum, 164L, (4L << 8), (4L << 8), CLIPMASK0) < 0 && furthestangle(pi, 8) < 512);
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

	// center_view
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
	else if (actions & SB_AIM_UP)
	{
		playerAimUp(snum, actions);
	}
	else if (actions & SB_AIM_DOWN)
	{	// aim_down
		playerAimDown(snum, actions);
	}

	if (cl_syncinput)
	{
		sethorizon(&p->horizon.horiz, PlayerHorizon(snum), &p->sync.actions, 1);
	}

	checkhardlanding(p);

	//Shooting code/changes

	if (p->show_empty_weapon > 0)
	{
		p->show_empty_weapon--;
		if (p->show_empty_weapon == 0)
		{
			if (p->last_full_weapon == GROW_WEAPON)
				p->subweapon |= (1 << GROW_WEAPON);
			else if (p->last_full_weapon == SHRINKER_WEAPON)
				p->subweapon &= ~(1 << GROW_WEAPON);
			fi.addweapon(p, p->last_full_weapon);
			return;
		}
	}

	dokneeattack(snum, pi, { FEM1, FEM2, FEM3, FEM4, FEM5, FEM6, FEM7, FEM8, FEM9, FEM10, PODFEM1, NAKED1, STATUE });

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

	// HACKS
	processweapon(snum, actions, psect);
}

void processmove_d(int snum, ESyncBits actions, int psect, int fz, int cz, int shrunk, int truefdist)
{
	int psectlotag = sector[psect].lotag;
	auto p = &ps[snum];
	if (psectlotag == 2)
	{
		underwater(snum, actions, psect, fz, cz);
	}

	else if (p->jetpack_on)
	{
		operateJetpack(snum, actions, psectlotag, fz, cz, shrunk);
	}
	else if (psectlotag != 2)
	{
		movement(snum, actions, psect, fz, cz, shrunk, truefdist, psectlotag);
	}
}
END_DUKE_NS
