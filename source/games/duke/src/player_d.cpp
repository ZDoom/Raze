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
#include "dukeactor.h"

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

	p->GetActor()->s->extra -= p->extra_extra8 >> 8;

	damage = p->GetActor()->s->extra - p->last_extra;

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

		p->GetActor()->s->extra = p->last_extra + damage;
	}
}


//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

static void shootfireball(DDukeActor *actor, int p, int sx, int sy, int sz, int sa)
{
	auto s = actor->s;
	int vel, zvel;

	if (s->extra >= 0)
		s->shade = -96;

	sz -= (4 << 7);
	if (s->picnum != BOSS5)
		vel = 840;
	else {
		vel = 968;
		sz += 6144;
	}

	if (p < 0)
	{
		sa += 16 - (krand() & 31);
		int scratch;
		int j = findplayer(actor, &scratch);
		zvel = (((ps[j].oposz - sz + (3 << 8))) * vel) / ldist(ps[j].GetActor(), actor);
	}
	else
	{
		zvel = -MulScale(ps[p].horizon.sum().asq16(), 98, 16);
		sx += bcos(sa + 348) / 448;
		sy += bsin(sa + 348) / 448;
		sz += (3 << 8);
	}

	int sizx = 18;
	int sizy = 18;
	if (p >= 0)
	{
		sizx = 7;
		sizy = 7;
	}

	auto spawned = EGS(s->sectnum, sx, sy, sz, FIREBALL, -127, sizx, sizy, sa, vel, zvel, actor, (short)4);
	auto spr = spawned->s;
	spr->extra += (krand() & 7);
	if (s->picnum == BOSS5 || p >= 0)
	{
		spr->xrepeat = 40;
		spr->yrepeat = 40;
	}
	spr->yvel = p;
	spr->cstat = 128;
	spr->clipdist = 4;
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

static void shootflamethrowerflame(DDukeActor* actor, int p, int sx, int sy, int sz, int sa)
{
	auto s = actor->s;
	int vel, zvel;

	if (s->extra >= 0)
		s->shade = -96;
	vel = 400;

	DDukeActor* spawned = nullptr;
	if (p < 0)
	{
		int x;
		int j = findplayer(actor, &x);
		sa = getangle(ps[j].oposx - sx, ps[j].oposy - sy);

		if (s->picnum == BOSS5)
		{
			vel = 528;
			sz += 6144;
		}
		else if (s->picnum == BOSS3)
			sz -= 8192;

		int l = ldist(ps[j].GetActor(), actor);
		if (l != 0)
			zvel = ((ps[j].oposz - sz) * vel) / l;

		if (badguy(actor) && (s->hitag & face_player_smart) != 0)
			sa = (short)(s->ang + (krand() & 31) - 16);

		if (s->sector()->lotag == 2 && (krand() % 5) == 0)
			spawned = spawn(actor, WATERBUBBLE);
	}
	else
	{
		zvel = -MulScale(ps[p].horizon.sum().asq16(), 81, 16);
		if (ps[p].GetActor()->s->xvel != 0)
			vel = (int)((((512 - (1024
				- abs(abs(getangle(sx - ps[p].oposx, sy - ps[p].oposy) - sa) - 1024)))
				* 0.001953125f) * ps[p].GetActor()->s->xvel) + 400);
		if (s->sector()->lotag == 2 && (krand() % 5) == 0)
			spawned = spawn(actor, WATERBUBBLE);
	}

	if (spawned == nullptr)
	{
		spawned = spawn(actor, FLAMETHROWERFLAME);
		spawned->s->xvel = (short)vel;
		spawned->s->zvel = (short)zvel;
	}

	spawned->s->x = sx + bsin(sa + 630) / 448;
	spawned->s->y = sy + bsin(sa + 112) / 448;
	spawned->s->z = sz - 256;
	spawned->s->sectnum = s->sectnum;
	spawned->s->cstat = 0x80;
	spawned->s->ang = sa;
	spawned->s->xrepeat = 2;
	spawned->s->yrepeat = 2;
	spawned->s->clipdist = 40;
	spawned->s->yvel = p;
	spawned->SetOwner(actor);

	if (p == -1)
	{
		if (s->picnum == BOSS5)
		{
			spawned->s->x -= bsin(sa) / 56;
			spawned->s->y += bcos(sa) / 56;
			spawned->s->xrepeat = 10;
			spawned->s->yrepeat = 10;
		}
	}
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

static void shootknee(DDukeActor* actor, int p, int sx, int sy, int sz, int sa)
{
	auto s = actor->s;
	int sect = s->sectnum;
	int zvel;
	int hitsect, hitwall;
	int hitx, hity, hitz;
	DDukeActor* hitsprt;

	if (p >= 0)
	{
		zvel = -ps[p].horizon.sum().asq16() >> 11;
		sz += (6 << 8);
		sa += 15;
	}
	else
	{
		int x;
		auto pactor = ps[findplayer(actor, &x)].GetActor();
		zvel = ((pactor->s->z - sz) << 8) / (x + 1);
		sa = getangle(pactor->s->x - sx, pactor->s->y - sy);
	}

	hitscan(sx, sy, sz, sect,
		bcos(sa),
		bsin(sa), zvel << 6,
		&hitsect, &hitwall, &hitsprt, &hitx, &hity, &hitz, CLIPMASK1);


	if (hitsect < 0) return;

	if ((abs(sx - hitx) + abs(sy - hity)) < 1024)
	{
		if (hitwall >= 0 || hitsprt)
		{
			auto knee = EGS(hitsect, hitx, hity, hitz, KNEE, -15, 0, 0, sa, 32, 0, actor, 4);
			knee->s->extra += (krand() & 7);
			if (p >= 0)
			{
				auto k = spawn(knee, SMALLSMOKE);
				k->s->z -= (8 << 8);
				S_PlayActorSound(KICK_HIT, knee);
			}

			if (p >= 0 && ps[p].steroids_amount > 0 && ps[p].steroids_amount < 400)
				knee->s->extra += (gs.max_player_health >> 2);

			if (hitsprt && hitsprt->s->picnum != ACCESSSWITCH && hitsprt->s->picnum != ACCESSSWITCH2)
			{
				fi.checkhitsprite(hitsprt, knee);
				if (p >= 0) fi.checkhitswitch(p, -1, hitsprt);
			}

			else if (hitwall >= 0)
			{
				if (wall[hitwall].cstat & 2)
					if (wall[hitwall].nextsector >= 0)
						if (hitz >= (sector[wall[hitwall].nextsector].floorz))
							hitwall = wall[hitwall].nextwall;

				if (hitwall >= 0 && wall[hitwall].picnum != ACCESSSWITCH && wall[hitwall].picnum != ACCESSSWITCH2)
				{
					fi.checkhitwall(knee, hitwall, hitx, hity, hitz, KNEE);
					if (p >= 0) fi.checkhitswitch(p, hitwall, nullptr);
				}
			}
		}
		else if (p >= 0 && zvel > 0 && sector[hitsect].lotag == 1)
		{
			auto splash = spawn(ps[p].GetActor(), WATERSPLASH2);
			splash->s->x = hitx;
			splash->s->y = hity;
			splash->s->ang = ps[p].angle.ang.asbuild(); // Total tweek
			splash->s->xvel = 32;
			ssp(actor, CLIPMASK0);
			splash->s->xvel = 0;

		}
	}
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

static void shootweapon(DDukeActor *actor, int p, int sx, int sy, int sz, int sa, int atwith)
{
	auto s = actor->s;
	int sect = s->sectnum;
	int zvel;
	int hitsect, hitwall;
	int hitx, hity, hitz;
	DDukeActor* hitact;

	if (s->extra >= 0) s->shade = -96;

	if (p >= 0)
	{
		SetGameVarID(g_iAimAngleVarID, AUTO_AIM_ANGLE, actor, p);
		OnEvent(EVENT_GETAUTOAIMANGLE, p, ps[p].GetActor(), -1);
		int varval = GetGameVarID(g_iAimAngleVarID, actor, p);
		DDukeActor* aimed = nullptr;
		if (varval > 0)
		{
			aimed = aim(actor, varval);
		}

		if (aimed)
		{
			int dal = ((aimed->s->xrepeat * tileHeight(aimed->s->picnum)) << 1) + (5 << 8);
			switch (aimed->s->picnum)
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
			zvel = ((aimed->s->z - sz - dal) << 8) / ldist(ps[p].GetActor(), aimed);
			sa = getangle(aimed->s->x - sx, aimed->s->y - sy);
		}

		if (isWW2GI())
		{
			int angRange = 32;
			int zRange = 256;
			SetGameVarID(g_iAngRangeVarID, 32, actor, p);
			SetGameVarID(g_iZRangeVarID, 256, actor, p);
			OnEvent(EVENT_GETSHOTRANGE, p, ps[p].GetActor(), -1);
			angRange = GetGameVarID(g_iAngRangeVarID, actor, p);
			zRange = GetGameVarID(g_iZRangeVarID, actor, p);

			sa += (angRange / 2) - (krand() & (angRange - 1));
			if (aimed == nullptr)
			{
				// no target
				zvel = -ps[p].horizon.sum().asq16() >> 11;
			}
			zvel += (zRange / 2) - (krand() & (zRange - 1));
		}
		else if (aimed == nullptr)
		{
			sa += 16 - (krand() & 31);
			zvel = -ps[p].horizon.sum().asq16() >> 11;
			zvel += 128 - (krand() & 255);
		}

		sz -= (2 << 8);
	}
	else
	{
		int x;
		int j = findplayer(actor, &x);
		sz -= (4 << 8);
		zvel = ((ps[j].pos.z - sz) << 8) / (ldist(ps[j].GetActor(), actor));
		if (s->picnum != BOSS1)
		{
			zvel += 128 - (krand() & 255);
			sa += 32 - (krand() & 63);
		}
		else
		{
			zvel += 128 - (krand() & 255);
			sa = getangle(ps[j].pos.x - sx, ps[j].pos.y - sy) + 64 - (krand() & 127);
		}
	}

	s->cstat &= ~257;
	hitscan(sx, sy, sz, sect,
		bcos(sa),
		bsin(sa),
		zvel << 6, &hitsect, &hitwall, &hitact, &hitx, &hity, &hitz, CLIPMASK1);
	s->cstat |= 257;


	if (hitsect < 0) return;

	if ((krand() & 15) == 0 && sector[hitsect].lotag == 2)
		tracers(hitx, hity, hitz, sx, sy, sz, 8 - (ud.multimode >> 1));

	DDukeActor* spark;
	if (p >= 0)
	{
		spark = EGS(hitsect, hitx, hity, hitz, SHOTSPARK1, -15, 10, 10, sa, 0, 0, actor, 4);
		spark->s->extra = ScriptCode[gs.actorinfo[atwith].scriptaddress];
		spark->s->extra += (krand() % 6);

		if (hitwall == -1 && hitact == nullptr)
		{
			if (zvel < 0)
			{
				if (sector[hitsect].ceilingstat & 1)
				{
					spark->s->xrepeat = 0;
					spark->s->yrepeat = 0;
					return;
				}
				else
					fi.checkhitceiling(hitsect);
			}
			spawn(spark, SMALLSMOKE);
		}

		if (hitact)
		{
			fi.checkhitsprite(hitact, spark);
			if (hitact->s->picnum == TILE_APLAYER && (ud.coop != 1 || ud.ffire == 1))
			{
				auto jib = spawn(spark, JIBS6);
				spark->s->xrepeat = spark->s->yrepeat = 0;
				jib->s->z += (4 << 8);
				jib->s->xvel = 16;
				jib->s->xrepeat = jib->s->yrepeat = 24;
				jib->s->ang += 64 - (krand() & 127);
			}
			else spawn(spark, SMALLSMOKE);

			if (p >= 0 && (
				hitact->s->picnum == DIPSWITCH ||
				hitact->s->picnum == DIPSWITCH + 1 ||
				hitact->s->picnum == DIPSWITCH2 ||
				hitact->s->picnum == DIPSWITCH2 + 1 ||
				hitact->s->picnum == DIPSWITCH3 ||
				hitact->s->picnum == DIPSWITCH3 + 1 ||
				hitact->s->picnum == HANDSWITCH ||
				hitact->s->picnum == HANDSWITCH + 1))
			{
				fi.checkhitswitch(p, -1, hitact);
				return;
			}
		}
		else if (hitwall >= 0)
		{
			spawn(spark, SMALLSMOKE);
			auto wal = &wall[hitwall];

			if (fi.isadoorwall(wal->picnum) == 1)
				goto SKIPBULLETHOLE;
			if (p >= 0 && (
				wal->picnum == DIPSWITCH ||
				wal->picnum == DIPSWITCH + 1 ||
				wal->picnum == DIPSWITCH2 ||
				wal->picnum == DIPSWITCH2 + 1 ||
				wal->picnum == DIPSWITCH3 ||
				wal->picnum == DIPSWITCH3 + 1 ||
				wal->picnum == HANDSWITCH ||
				wal->picnum == HANDSWITCH + 1))
			{
				fi.checkhitswitch(p, hitwall, nullptr);
				return;
			}

			if (wal->hitag != 0 || (wal->nextwall >= 0 && wal->nextWall()->hitag != 0))
				goto SKIPBULLETHOLE;

			if (hitsect >= 0 && sector[hitsect].lotag == 0)
				if (wal->overpicnum != BIGFORCE)
					if ((wal->nextsector >= 0 && wal->nextSector()->lotag == 0) ||
						(wal->nextsector == -1 && sector[hitsect].lotag == 0))
						if ((wal->cstat & 16) == 0)
						{
							if (wal->nextsector >= 0)
							{
								DukeSectIterator it(wal->nextsector);
								while (auto l = it.Next())
								{
									if (l->s->statnum == 3 && l->s->lotag == 13)
										goto SKIPBULLETHOLE;
								}
							}

							DukeStatIterator it(STAT_MISC);
							while (auto l = it.Next())
							{
								if (l->s->picnum == BULLETHOLE)
									if (dist(l, spark) < (12 + (krand() & 7)))
										goto SKIPBULLETHOLE;
							}
							auto hole = spawn(spark, BULLETHOLE);
							hole->s->xvel = -1;
							hole->s->ang = getangle(wal->x - wall[wal->point2].x,
								wal->y - wall[wal->point2].y) + 512;
							ssp(hole, CLIPMASK0);
						}

		SKIPBULLETHOLE:

			if (wal->cstat & 2)
				if (wal->nextsector >= 0)
					if (hitz >= (wal->nextSector()->floorz))
						hitwall = wal->nextwall;

			fi.checkhitwall(spark, hitwall, hitx, hity, hitz, SHOTSPARK1);
		}
	}
	else
	{
		spark = EGS(hitsect, hitx, hity, hitz, SHOTSPARK1, -15, 24, 24, sa, 0, 0, actor, 4);
		spark->s->extra = ScriptCode[gs.actorinfo[atwith].scriptaddress];

		if (hitact)
		{
			fi.checkhitsprite(hitact, spark);
			if (hitact->s->picnum != TILE_APLAYER)
				spawn(spark, SMALLSMOKE);
			else spark->s->xrepeat = spark->s->yrepeat = 0;
		}
		else if (hitwall >= 0)
			fi.checkhitwall(spark, hitwall, hitx, hity, hitz, SHOTSPARK1);
	}

	if ((krand() & 255) < 4)
	{
		vec3_t v{ hitx, hity, hitz };
		S_PlaySound3D(PISTOL_RICOCHET, spark, &v);
	}
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

static void shootstuff(DDukeActor* actor, int p, int sx, int sy, int sz, int sa, int atwith)
{
	spritetype* const s = actor->s;
	int sect = s->sectnum;
	int vel, zvel;
	int l, scount;

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
		auto aimed = aim(actor, AUTO_AIM_ANGLE);

		if (aimed)
		{
			int dal = ((aimed->s->xrepeat * tileHeight(aimed->s->picnum)) << 1) - (12 << 8);
			zvel = ((aimed->s->z - sz - dal) * vel) / ldist(ps[p].GetActor(), aimed);
			sa = getangle(aimed->s->x - sx, aimed->s->y - sy);
		}
		else
			zvel = -MulScale(ps[p].horizon.sum().asq16(), 98, 16);
	}
	else
	{
		int x;
		int j = findplayer(actor, &x);
		// sa = getangle(ps[j].oposx-sx,ps[j].oposy-sy);
		sa += 16 - (krand() & 31);
		zvel = (((ps[j].oposz - sz + (3 << 8))) * vel) / ldist(ps[j].GetActor(), actor);
	}

	int oldzvel = zvel;
	int sizx, sizy;

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
		auto spawned = EGS(sect, sx, sy, sz, atwith, -127, sizx, sizy, sa, vel, zvel, actor, 4);
		spawned->s->extra += (krand() & 7);

		if (atwith == COOLEXPLOSION1)
		{
			spawned->s->shade = 0;
			if (s->picnum == BOSS2)
			{
				l = spawned->s->xvel;
				spawned->s->xvel = 1024;
				ssp(spawned, CLIPMASK0);
				spawned->s->xvel = l;
				spawned->s->ang += 128 - (krand() & 255);
			}
		}

		spawned->s->cstat = 128;
		spawned->s->clipdist = 4;

		sa = s->ang + 32 - (krand() & 63);
		zvel = oldzvel + 512 - (krand() & 1023);

		scount--;
	}
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

static void shootrpg(DDukeActor *actor, int p, int sx, int sy, int sz, int sa, int atwith)
{
	auto s = actor->s;
	int sect = s->sectnum;
	int vel, zvel;
	int l, scount;

	if (s->extra >= 0) s->shade = -96;

	scount = 1;
	vel = 644;

	DDukeActor* aimed = nullptr;

	if (p >= 0)
	{
		aimed = aim(actor, 48);
		if (aimed)
		{
			int dal = ((aimed->s->xrepeat * tileHeight(aimed->s->picnum)) << 1) + (8 << 8);
			zvel = ((aimed->s->z - sz - dal) * vel) / ldist(ps[p].GetActor(), aimed);
			if (aimed->s->picnum != RECON)
				sa = getangle(aimed->s->x - sx, aimed->s->y - sy);
		}
		else zvel = -MulScale(ps[p].horizon.sum().asq16(), 81, 16);
		if (atwith == RPG)
			S_PlayActorSound(RPG_SHOOT, actor);

	}
	else
	{
		int x;
		int j = findplayer(actor, &x);
		sa = getangle(ps[j].oposx - sx, ps[j].oposy - sy);
		if (s->picnum == BOSS3)
		{
			int zoffs = (32 << 8);
			if (isWorldTour()) // Twentieth Anniversary World Tour
				zoffs = (int)((actor->s->yrepeat / 80.0f) * zoffs);
			sz -= zoffs;
		}
		else if (s->picnum == BOSS2)
		{
			vel += 128;
			int zoffs = 24 << 8;
			if (isWorldTour()) // Twentieth Anniversary World Tour
				zoffs = (int)((actor->s->yrepeat / 80.0f) * zoffs);
			sz += zoffs;
		}

		l = ldist(ps[j].GetActor(), actor);
		zvel = ((ps[j].oposz - sz) * vel) / l;

		if (badguy(actor) && (s->hitag & face_player_smart))
			sa = s->ang + (krand() & 31) - 16;
	}
	if (p < 0) aimed = nullptr;

	auto spawned = EGS(sect,
		sx + (bcos(sa + 348) / 448),
		sy + (bsin(sa + 348) / 448),
		sz - (1 << 8), atwith, 0, 14, 14, sa, vel, zvel, actor, 4);

	auto spj = spawned->s;
	spj->extra += (krand() & 7);
	if (atwith != FREEZEBLAST)
		spawned->temp_actor = aimed;
	else
	{
		spj->yvel = gs.numfreezebounces;
		spj->xrepeat >>= 1;
		spj->yrepeat >>= 1;
		spj->zvel -= (2 << 4);
	}

	if (p == -1)
	{
		if (s->picnum == BOSS3)
		{
			int xoffs = bsin(sa, -6);
			int yoffs = -bcos(sa, -6);
			int aoffs = 4;

			if ((krand() & 1) != 0)
			{
				xoffs = -xoffs;
				yoffs = -yoffs;
				aoffs = -8;
			}

			if (isWorldTour()) // Twentieth Anniversary World Tour
			{
				float siz = actor->s->yrepeat / 80.0f;
				xoffs = int(xoffs * siz);
				yoffs = int(yoffs * siz);
				aoffs = int(aoffs * siz);
			}

			spj->x += xoffs;
			spj->y += yoffs;
			spj->ang += aoffs;

			spj->xrepeat = 42;
			spj->yrepeat = 42;
		}
		else if (s->picnum == BOSS2)
		{
			int xoffs = bsin(sa) / 56;
			int yoffs = -bcos(sa) / 56;
			int aoffs = 8 + (krand() & 255) - 128;

			if (isWorldTour()) { // Twentieth Anniversary World Tour
				int siz = actor->s->yrepeat;
				xoffs = Scale(xoffs, siz, 80);
				yoffs = Scale(yoffs, siz, 80);
				aoffs = Scale(aoffs, siz, 80);
			}

			spj->x -= xoffs;
			spj->y -= yoffs;
			spj->ang -= aoffs;

			spj->xrepeat = 24;
			spj->yrepeat = 24;
		}
		else if (atwith != FREEZEBLAST)
		{
			spj->xrepeat = 30;
			spj->yrepeat = 30;
			spj->extra >>= 2;
		}
	}
	else if ((isWW2GI() && aplWeaponWorksLike[ps[p].curr_weapon][p] == DEVISTATOR_WEAPON) || (!isWW2GI() && ps[p].curr_weapon == DEVISTATOR_WEAPON))
	{
		spj->extra >>= 2;
		spj->ang += 16 - (krand() & 31);
		spj->zvel += 256 - (krand() & 511);

		if (ps[p].hbomb_hold_delay)
		{
			spj->x -= bsin(sa) / 644;
			spj->y += bcos(sa) / 644;
		}
		else
		{
			spj->x += bsin(sa, -8);
			spj->y -= bcos(sa, -8);
		}
		spj->xrepeat >>= 1;
		spj->yrepeat >>= 1;
	}

	spj->cstat = 128;
	if (atwith == RPG)
		spj->clipdist = 4;
	else
		spj->clipdist = 40;

}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

static void shootlaser(DDukeActor* actor, int p, int sx, int sy, int sz, int sa)
{
	spritetype* const s = actor->s;
	int sect = s->sectnum;
	int zvel;
	int hitsect, hitwall, j;
	int hitx, hity, hitz;
	DDukeActor* hitsprt;

	if (p >= 0)
		zvel = -ps[p].horizon.sum().asq16() >> 11;
	else zvel = 0;

	hitscan(sx, sy, sz - ps[p].pyoff, sect,
		bcos(sa),
		bsin(sa),
		zvel << 6, &hitsect, &hitwall, &hitsprt, &hitx, &hity, &hitz, CLIPMASK1);

	j = 0;
	if (hitsprt) return;

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
		auto bomb = EGS(hitsect, hitx, hity, hitz, TRIPBOMB, -16, 4, 5, sa, 0, 0, actor, 6);
		if (isWW2GI())
		{
			int lTripBombControl = GetGameVar("TRIPBOMB_CONTROL", TRIPBOMB_TRIPWIRE, nullptr, -1);
			if (lTripBombControl & TRIPBOMB_TIMER)
			{
				int lLifetime = GetGameVar("STICKYBOMB_LIFETIME", NAM_GRENADE_LIFETIME, nullptr, p);
				int lLifetimeVar = GetGameVar("STICKYBOMB_LIFETIME_VAR", NAM_GRENADE_LIFETIME_VAR, nullptr, p);
				// set timer.  blows up when at zero....
				bomb->s->extra = lLifetime
					+ MulScale(krand(), lLifetimeVar, 14)
					- lLifetimeVar;
			}
		}

		// this originally used the sprite index as tag to link the laser segments.
		// This value is never used again to reference an actor by index. Decouple this for robustness.
		ud.bomb_tag = (ud.bomb_tag + 1) & 32767;
		bomb->s->hitag = ud.bomb_tag;
		S_PlayActorSound(LASERTRIP_ONWALL, bomb);
		bomb->s->xvel = -20;
		ssp(bomb, CLIPMASK0);
		bomb->s->cstat = 16;
		bomb->temp_data[5] = bomb->s->ang = getangle(wall[hitwall].x - wall[wall[hitwall].point2].x, wall[hitwall].y - wall[wall[hitwall].point2].y) - 512;

		if (p >= 0)
			ps[p].ammo_amount[TRIPBOMB_WEAPON]--;
	}
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

static void shootgrowspark(DDukeActor* actor, int p, int sx, int sy, int sz, int sa)
{
	auto s = actor->s;
	int sect = s->sectnum;
	int zvel;
	int hitsect, hitwall, k;
	int hitx, hity, hitz;
	DDukeActor* hitsprt;

	if (p >= 0)
	{
		auto aimed = aim(actor, AUTO_AIM_ANGLE);
		if (aimed)
		{
			int dal = ((aimed->s->xrepeat * tileHeight(aimed->s->picnum)) << 1) + (5 << 8);
			switch (aimed->s->picnum)
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
			zvel = ((aimed->s->z - sz - dal) << 8) / (ldist(ps[p].GetActor(), aimed));
			sa = getangle(aimed->s->x - sx, aimed->s->y - sy);
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
		int x;
		int j = findplayer(actor, &x);
		sz -= (4 << 8);
		zvel = ((ps[j].pos.z - sz) << 8) / (ldist(ps[j].GetActor(), actor));
		zvel += 128 - (krand() & 255);
		sa += 32 - (krand() & 63);
	}

	k = 0;

	//RESHOOTGROW:

	s->cstat &= ~257;
	hitscan(sx, sy, sz, sect, bcos(sa), bsin(sa),
		zvel << 6, &hitsect, &hitwall, &hitsprt, &hitx, &hity, &hitz, CLIPMASK1);

	s->cstat |= 257;

	auto spark = EGS(sect, hitx, hity, hitz, GROWSPARK, -16, 28, 28, sa, 0, 0, actor, 1);

	spark->s->pal = 2;
	spark->s->cstat |= 130;
	spark->s->xrepeat = spark->s->yrepeat = 1;

	if (hitwall == -1 && hitsprt == nullptr && hitsect >= 0)
	{
		if (zvel < 0 && (sector[hitsect].ceilingstat & 1) == 0)
			fi.checkhitceiling(hitsect);
	}
	else if (hitsprt != nullptr) fi.checkhitsprite(hitsprt, spark);
	else if (hitwall >= 0 && wall[hitwall].picnum != ACCESSSWITCH && wall[hitwall].picnum != ACCESSSWITCH2)
	{
		fi.checkhitwall(spark, hitwall, hitx, hity, hitz, GROWSPARK);
	}
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void shoot_d(DDukeActor* actor, int atwith)
{
	spritetype* const s = actor->s;

	int sect, l, j;
	int sx, sy, sz, sa, p, vel, zvel, x, dal;
	if (s->picnum == TILE_APLAYER)
	{
		p = s->yvel;
	}
	else
	{
		p = -1;
	}

	SetGameVarID(g_iAtWithVarID, atwith, actor, p);
	SetGameVarID(g_iReturnVarID, 0, actor, p);
	OnEvent(EVENT_SHOOT, p, ps[p].GetActor(), -1);
	if (GetGameVarID(g_iReturnVarID, actor, p) != 0)
	{
		return;
	}


	sect = s->sectnum;
	zvel = 0;

	if (s->picnum == TILE_APLAYER)
	{
		sx = ps[p].pos.x;
		sy = ps[p].pos.y;
		sz = ps[p].pos.z + ps[p].pyoff + (4 << 8);
		sa = ps[p].angle.ang.asbuild();

		ps[p].crack_time = CRACK_TIME;

	}
	else
	{
		sa = s->ang;
		sx = s->x;
		sy = s->y;
		sz = s->z - (s->yrepeat * tileHeight(s->picnum) << 1) + (4 << 8);
		if (s->picnum != ROTATEGUN)
		{
			sz -= (7 << 8);
			if (badguy(s) && s->picnum != COMMANDER)
			{
				sx -= bsin(sa + 96, -7);
				sy += bcos(sa + 96, -7);
			}
		}
	}

	if (isWorldTour()) 
	{ // Twentieth Anniversary World Tour
		switch (atwith) 
		{
		case FIREBALL:
			shootfireball(actor, p, sx, sy, sz, sa);
			return;

		case FLAMETHROWERFLAME:
			shootflamethrowerflame(actor, p, sx, sy, sz, sa);
			return;

		case FIREFLY: // BOSS5 shot
		{
			auto k = spawn(actor, atwith);
			k->s->sectnum = sect;
			k->s->x = sx;
			k->s->y = sy;
			k->s->z = sz;
			k->s->ang = sa;
			k->s->xvel = 500;
			k->s->zvel = 0;
			return;
		}
		}
	}

	switch (atwith)
	{
	case BLOODSPLAT1:
	case BLOODSPLAT2:
	case BLOODSPLAT3:
	case BLOODSPLAT4:
		shootbloodsplat(actor, p, sx, sy, sz, sa, atwith, BIGFORCE, OOZFILTER, NEWBEAST);
		break;

	case KNEE:
		shootknee(actor, p, sx, sy, sz, sa);
		break;

	case SHOTSPARK1:
	case SHOTGUN:
	case CHAINGUN:
		shootweapon(actor, p, sx, sy, sz, sa, atwith);
		return;

	case FIRELASER:
	case SPIT:
	case COOLEXPLOSION1:
		shootstuff(actor, p, sx, sy, sz, sa, atwith);
		return;

	case FREEZEBLAST:
		sz += (3 << 8);
	case RPG:

		shootrpg(actor, p, sx, sy, sz, sa, atwith);
		break;

	case HANDHOLDINGLASER:
		shootlaser(actor, p, sx, sy, sz, sa);
		return;

	case BOUNCEMINE:
	case MORTER:
	{
		if (s->extra >= 0) s->shade = -96;

		auto j = ps[findplayer(actor, &x)].GetActor();
		x = ldist(j, actor);

		zvel = -x >> 1;

		if (zvel < -4096)
			zvel = -2048;
		vel = x >> 4;

		EGS(sect,
			sx - bsin(sa, -8),
			sy + bcos(sa, -8),
			sz + (6 << 8), atwith, -64, 32, 32, sa, vel, zvel, actor, 1);
		break;
	}
	case GROWSPARK:
		shootgrowspark(actor, p, sx, sy, sz, sa);
		break;

	case SHRINKER:
	{
		if (s->extra >= 0) s->shade = -96;
		if (p >= 0)
		{
			auto aimed = isNamWW2GI() ? nullptr : aim(actor, AUTO_AIM_ANGLE);
			if (aimed)
			{
				dal = ((aimed->s->xrepeat * tileHeight(aimed->s->picnum)) << 1);
				zvel = ((aimed->s->z - sz - dal - (4 << 8)) * 768) / (ldist(ps[p].GetActor(), aimed));
				sa = getangle(aimed->s->x - sx, aimed->s->y - sy);
			}
			else zvel = -MulScale(ps[p].horizon.sum().asq16(), 98, 16);
		}
		else if (s->statnum != 3)
		{
			j = findplayer(actor, &x);
			l = ldist(ps[j].GetActor(), actor);
			zvel = ((ps[j].oposz - sz) * 512) / l;
		}
		else zvel = 0;

		auto j = EGS(sect,
			sx - bsin(sa, -12),
			sy + bcos(sa, -12),
			sz + (2 << 8), SHRINKSPARK, -16, 28, 28, sa, 768, zvel, actor, 4);

		j->s->cstat = 128;
		j->s->clipdist = 32;


		return;
	}
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
	if (p->last_pissed_time <= (26 * 218) && p->show_empty_weapon == 0 && p->kickback_pic == 0 && p->quick_kick == 0 && p->GetActor()->s->xrepeat > 32 && p->access_incs == 0 && p->knee_incs == 0)
	{
		if ((p->weapon_pos == 0 || (p->holster_weapon && p->weapon_pos == -9)))
		{
			if (weap == WeaponSel_Alt)
			{
				j = p->curr_weapon;
				switch (p->curr_weapon)
				{
					case SHRINKER_WEAPON:
						if (p->ammo_amount[GROW_WEAPON] > 0 && p->gotweapon[GROW_WEAPON] && isPlutoPak())
						{
							j = GROW_WEAPON;
							p->subweapon |= (1 << GROW_WEAPON);
						}
						break;
					case GROW_WEAPON:
						if (p->ammo_amount[SHRINKER_WEAPON] > 0 && p->gotweapon[SHRINKER_WEAPON])
						{
							j = SHRINKER_WEAPON;
							p->subweapon &= ~(1 << GROW_WEAPON);
						}
						break;
					case FREEZE_WEAPON:
						if (p->ammo_amount[FLAMETHROWER_WEAPON] > 0 && p->gotweapon[FLAMETHROWER_WEAPON] && isWorldTour())
						{
							j = FLAMETHROWER_WEAPON;
							p->subweapon |= (1 << FLAMETHROWER_WEAPON);
						}
						break;
					case FLAMETHROWER_WEAPON:
						if (p->ammo_amount[FREEZE_WEAPON] > 0 && p->gotweapon[FREEZE_WEAPON])
						{
							j = FREEZE_WEAPON;
							p->subweapon &= ~(1 << FLAMETHROWER_WEAPON);
						}
						break;
					default:
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
						if (j == -1)
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
				DukeStatIterator it(STAT_ACTOR);
				while (auto act = it.Next())
				{
					if (act->s->picnum == HEAVYHBOMB && act->GetOwner() == p->GetActor())
					{
						p->gotweapon[HANDBOMB_WEAPON] = true;
						j = HANDREMOTE_WEAPON;
						break;
					}
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
			else if (j >= MIN_WEAPON && p->gotweapon[j] && p->curr_weapon != j) switch (j)
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

	auto pact = p->GetActor();
	snum = pact->s->yvel;

	p->player_par++;

	if (p->invdisptime > 0)
		p->invdisptime--;

	if (p->tipincs > 0) p->tipincs--;

	if (p->last_pissed_time > 0)
	{
		p->last_pissed_time--;

		if (p->last_pissed_time == (26 * 219))
		{
			S_PlayActorSound(FLUSH_TOILET, pact);
			if (snum == screenpeek || ud.coop == 1)
				S_PlayActorSound(DUKE_PISSRELIEF, pact);
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
				S_PlayActorSound(DUKE_HARTBEAT, pact);
	}

	if (p->heat_on && p->heat_amount > 0)
	{
		p->heat_amount--;
		if (p->heat_amount == 0)
		{
			p->heat_on = 0;
			checkavailinven(p);
			S_PlayActorSound(NITEVISION_ONOFF, pact);
		}
	}

	if (p->holoduke_on != nullptr)
	{
		p->holoduke_amount--;
		if (p->holoduke_amount <= 0)
		{
			S_PlayActorSound(TELEPORTER, pact);
			p->holoduke_on = nullptr;
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
			S_PlayActorSound(DUKE_JETPACK_OFF, pact);
			S_StopSound(DUKE_JETPACK_IDLE, pact);
			S_StopSound(DUKE_JETPACK_ON, pact);
		}
	}

	if (p->quick_kick > 0 && p->GetActor()->s->pal != 1)
	{
		p->last_quick_kick = p->quick_kick + 1;
		p->quick_kick--;
		if (p->quick_kick == 8)
			fi.shoot(p->GetActor(), KNEE);
	}
	else if (p->last_quick_kick > 0)
		p->last_quick_kick--;

	if (p->access_incs && p->GetActor()->s->pal != 1)
	{
		p->access_incs++;
		if (p->GetActor()->s->extra <= 0)
			p->access_incs = 12;
		if (p->access_incs == 12)
		{
			if (p->access_spritenum != nullptr)
			{
				fi.checkhitswitch(snum, -1, p->access_spritenum);
				switch (p->access_spritenum->s->pal)
				{
				case 0:p->got_access &= (0xffff - 0x1); break;
				case 21:p->got_access &= (0xffff - 0x2); break;
				case 23:p->got_access &= (0xffff - 0x4); break;
				}
				p->access_spritenum = nullptr;
			}
			else
			{
				fi.checkhitswitch(snum, p->access_wallnum, nullptr);
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

	if (p->scuba_on == 0 && p->cursector()->lotag == 2)
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
				if (p->last_extra < (gs.max_player_health >> 1) && (p->last_extra & 3) == 0)
					S_PlayActorSound(DUKE_LONGTERM_PAIN, pact);
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
			if (PlayClock > 1024)
				if (snum == screenpeek || ud.coop == 1)
				{
					if (rand() & 1)
						S_PlayActorSound(DUKE_CRACK, pact);
					else S_PlayActorSound(DUKE_CRACK2, pact);
				}
			S_PlayActorSound(DUKE_CRACK_FIRST, pact);
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
	static const uint16_t weapon_sprites[MAX_WEAPONS] = { KNEE, FIRSTGUNSPRITE, SHOTGUNSPRITE,
			CHAINGUNSPRITE, RPGSPRITE, HEAVYHBOMB, SHRINKERSPRITE, DEVISTATORSPRITE,
			TRIPBOMBSPRITE, FREEZESPRITE, HEAVYHBOMB, SHRINKERSPRITE };

	int cw;

	if (isWW2GI())
	{
		int snum = p->GetActor()->s->yvel;
		cw = aplWeaponWorksLike[p->curr_weapon][snum];
	}
	else 
		cw = p->curr_weapon;


	if (cw < 1 || cw >= MAX_WEAPONS) return;

	if (cw)
	{
		if (krand() & 1)
			spawn(p->GetActor(), weapon_sprites[cw]);
		else switch (cw)
		{
		case RPG_WEAPON:
		case HANDBOMB_WEAPON:
			spawn(p->GetActor(), EXPLOSION2);
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
	auto pact = p->GetActor();
	p->on_ground = 0;
	p->jumping_counter = 0;
	p->hard_landing = 0;
	p->falling_counter = 0;

	p->pycount += 32;
	p->pycount &= 2047;
	p->pyoff = bsin(p->pycount, -7);

	if (p->jetpack_on && S_CheckActorSoundPlaying(pact, DUKE_SCREAM))
	{
		S_StopSound(DUKE_SCREAM, pact);
	}

	if (p->jetpack_on < 11)
	{
		p->jetpack_on++;
		p->pos.z -= (p->jetpack_on << 7); //Goin up
	}
	else if (p->jetpack_on == 11 && !S_CheckActorSoundPlaying(pact, DUKE_JETPACK_IDLE))
		S_PlayActorSound(DUKE_JETPACK_IDLE, pact);

	if (shrunk) j = 512;
	else j = 2048;

	if (actions & SB_JUMP)                            //A (soar high)
	{
		// jump
		SetGameVarID(g_iReturnVarID, 0, p->GetActor(), snum);
		OnEvent(EVENT_SOARUP, snum, p->GetActor(), -1);
		if (GetGameVarID(g_iReturnVarID, p->GetActor(), snum) == 0)
		{
			p->pos.z -= j;
			p->crack_time = CRACK_TIME;
		}
	}

	if (actions & SB_CROUCH)                            //Z (soar low)
	{
		// crouch
		SetGameVarID(g_iReturnVarID, 0, p->GetActor(), snum);
		OnEvent(EVENT_SOARDOWN, snum, p->GetActor(), -1);
		if (GetGameVarID(g_iReturnVarID, p->GetActor(), snum) == 0)
		{
			p->pos.z += j;
			p->crack_time = CRACK_TIME;
		}
	}

	int k;
	if (shrunk == 0 && (psectlotag == 0 || psectlotag == 2)) k = 32;
	else k = 16;

	if (psectlotag != 2 && p->scuba_on == 1)
		p->scuba_on = 0;

	if (p->pos.z > (fz - (k << 8)))
		p->pos.z += ((fz - (k << 8)) - p->pos.z) >> 1;
	if (p->pos.z < (pact->ceilingz + (18 << 8)))
		p->pos.z = pact->ceilingz + (18 << 8);

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
	auto pact = p->GetActor();

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
			p->pyoff = bsin(p->pycount, -6);
		}
		else i = 12;

		if (shrunk == 0 && truefdist <= gs.playerheight)
		{
			if (p->on_ground == 1)
			{
				if (p->dummyplayersprite == nullptr)
					p->dummyplayersprite = spawn(pact, PLAYERONWATER);

				p->footprintcount = 6;
				if (p->cursector()->floorpicnum == FLOORSLIME)
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

	if (p->pos.z < (fz - (i << 8))) //falling
	{

		// not jumping or crouching
		if ((actions & (SB_JUMP|SB_CROUCH)) == 0 && p->on_ground && (sector[psect].floorstat & 2) && p->pos.z >= (fz - (i << 8) - (16 << 8)))
			p->pos.z = fz - (i << 8);
		else
		{
			p->on_ground = 0;
			p->poszv += (gs.gravity + 80); // (TICSPERFRAME<<6);
			if (p->poszv >= (4096 + 2048)) p->poszv = (4096 + 2048);
			if (p->poszv > 2400 && p->falling_counter < 255)
			{
				p->falling_counter++;
				if (p->falling_counter == 38 && !S_CheckActorSoundPlaying(pact, DUKE_SCREAM))
					S_PlayActorSound(DUKE_SCREAM, pact);
			}

			if ((p->pos.z + p->poszv) >= (fz - (i << 8))) // hit the ground
			{
				S_StopSound(DUKE_SCREAM, pact);
				if (p->cursector()->lotag != 1)
				{
					if (p->falling_counter > 62) quickkill(p);

					else if (p->falling_counter > 9)
					{
						j = p->falling_counter;
						pact->s->extra -= j - (krand() & 3);
						if (pact->s->extra <= 0)
						{
							S_PlayActorSound(SQUISHED, pact);
							SetPlayerPal(p, PalEntry(63, 63, 0, 0));
						}
						else
						{
							S_PlayActorSound(DUKE_LAND, pact);
							S_PlayActorSound(DUKE_LAND_HURT, pact);
						}

						SetPlayerPal(p, PalEntry(32, 16, 0, 0));
					}
					else if (p->poszv > 2048) S_PlayActorSound(DUKE_LAND, pact);
				}
			}
		}
	}

	else
	{
		p->falling_counter = 0;
		S_StopSound(-1, pact, CHAN_VOICE);

		if (psectlotag != ST_1_ABOVE_WATER && psectlotag != ST_2_UNDERWATER && p->on_ground == 0 && p->poszv > (6144 >> 1))
			p->hard_landing = p->poszv >> 10;

		p->on_ground = 1;

		if (i == 40)
		{
			//Smooth on the ground

			int k = ((fz - (i << 8)) - p->pos.z) >> 1;
			if (abs(k) < 256) k = 0;
			p->pos.z += k;
			p->poszv -= 768;
			if (p->poszv < 0) p->poszv = 0;
		}
		else if (p->jumping_counter == 0)
		{
			p->pos.z += ((fz - (i << 7)) - p->pos.z) >> 1; //Smooth on the water
			if (p->on_warping_sector == 0 && p->pos.z > fz - (16 << 8))
			{
				p->pos.z = fz - (16 << 8);
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
				p->poszv -= bsin(2048 - 128 + p->jumping_counter) / 12;
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

	p->pos.z += p->poszv;

	if (p->pos.z < (cz + (4 << 8)))
	{
		p->jumping_counter = 0;
		if (p->poszv < 0)
			p->posxv = p->posyv = 0;
		p->poszv = 128;
		p->pos.z = cz + (4 << 8);
	}
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

static void underwater(int snum, ESyncBits actions, int psect, int fz, int cz)
{
	auto p = &ps[snum];
	auto pact = p->GetActor();
	int psectlotag = sector[psect].lotag;

	// under water
	p->jumping_counter = 0;

	p->pycount += 32;
	p->pycount &= 2047;
	p->pyoff = bsin(p->pycount, -7);

	if (!S_CheckActorSoundPlaying(pact, DUKE_UNDERWATER))
		S_PlayActorSound(DUKE_UNDERWATER, pact);

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

	p->pos.z += p->poszv;

	if (p->pos.z > (fz - (15 << 8)))
		p->pos.z += ((fz - (15 << 8)) - p->pos.z) >> 1;

	if (p->pos.z < (cz + (4 << 8)))
	{
		p->pos.z = cz + (4 << 8);
		p->poszv = 0;
	}

	if (p->scuba_on && (krand() & 255) < 8)
	{
		auto j = spawn(pact, WATERBUBBLE);
		j->s->x += bcos(p->angle.ang.asbuild() + 64 - (global_random & 128), -6);
		j->s->y += bsin(p->angle.ang.asbuild() + 64 - (global_random & 128), -6);
		j->s->xrepeat = 3;
		j->s->yrepeat = 2;
		j->s->z = p->pos.z + (8 << 8);
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

	int sx, sy, sz;
	int sect, hw;
	DDukeActor* hitsprt;

	hitscan(p->pos.x, p->pos.y, p->pos.z,
		p->cursectnum, p->angle.ang.bcos(),
		p->angle.ang.bsin(), -p->horizon.sum().asq16() >> 11,
		&sect, &hw, &hitsprt, &sx, &sy, &sz, CLIPMASK1);

	if (sect < 0 || hitsprt)
		return 0;

	if (hw >= 0 && sector[sect].lotag > 2)
		return 0;

	if (hw >= 0 && wall[hw].overpicnum >= 0)
		if (wall[hw].overpicnum == BIGFORCE)
			return 0;

	DDukeActor* j;
	DukeSectIterator it(sect);
	while ((j = it.Next()))
	{
		auto sj = j->s;
		if (sj->picnum == TRIPBOMB &&
			abs(sj->z - sz) < (12 << 8) && ((sj->x - sx) * (sj->x - sx) + (sj->y - sy) * (sj->y - sy)) < (290 * 290))
			return 0;
	}

	if (j == nullptr && hw >= 0 && (wall[hw].cstat & 16) == 0)
		if ((wall[hw].nextsector >= 0 && sector[wall[hw].nextsector].lotag <= 2) || (wall[hw].nextsector == -1 && sector[sect].lotag <= 2))
			if (((sx - p->pos.x) * (sx - p->pos.x) + (sy - p->pos.y) * (sy - p->pos.y)) < (290 * 290))
			{
				p->pos.z = p->oposz;
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
	auto pact = p->GetActor();

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
				S_PlayActorSound(EXPANDERSHOOT, pact);
			}
		}
		else if (p->ammo_amount[SHRINKER_WEAPON] > 0)
		{
			p->kickback_pic = 1;
			S_PlayActorSound(SHRINKER_FIRE, pact);
		}
		break;

	case FREEZE_WEAPON:
		if (p->ammo_amount[FREEZE_WEAPON] > 0)
		{
			p->kickback_pic = 1;
			S_PlayActorSound(CAT_FIRE, pact);
		}
		break;
	case DEVISTATOR_WEAPON:
		if (p->ammo_amount[DEVISTATOR_WEAPON] > 0)
		{
			p->kickback_pic = 1;
			p->hbomb_hold_delay = !p->hbomb_hold_delay;
			S_PlayActorSound(CAT_FIRE, pact);
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
			if (p->cursector()->lotag != 2)
				S_PlayActorSound(FLAMETHROWER_INTRO, pact);
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
	auto pact = p->GetActor();
	int i, k;

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
				i = MulScale(p->horizon.sum().asq16(), 20, 16);
			}
			else
			{
				k = 140;
				i = -512 - MulScale(p->horizon.sum().asq16(), 20, 16);
			}

			auto spawned = EGS(p->cursectnum,
				p->pos.x + p->angle.ang.bcos(-6),
				p->pos.y + p->angle.ang.bsin(-6),
				p->pos.z, HEAVYHBOMB, -16, 9, 9,
				p->angle.ang.asbuild(), (k + (p->hbomb_hold_delay << 5)), i, pact, 1);

			if (isNam())
			{
				spawned->s->extra = MulScale(krand(), NAM_GRENADE_LIFETIME_VAR, 14);
			}

			if (k == 15)
			{
				spawned->s->yvel = 3;
				spawned->s->z += (8 << 8);
			}

			k = hits(pact);
			if (k < 512)
			{
				spawned->s->ang += 1024;
				spawned->s->zvel /= 3;
				spawned->s->xvel /= 3;
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
			fi.shoot(pact, SHOTSPARK1);
			S_PlayActorSound(PISTOL_FIRE, pact);
			lastvisinc = PlayClock + 32;
			p->visibility = 0;
		}

		else if (p->kickback_pic == 2)
			spawn(pact, SHELL);

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
					S_PlayActorSound(EJECT_CLIP, pact);
					break;
					//#ifdef NAM								
					//     case WEAPON2_RELOAD_TIME - 15:
					//#else
				case 8:
					//#endif
					S_PlayActorSound(INSERT_CLIP, pact);
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
			for(int i = 0; i < 7; i++)
				fi.shoot(pact, SHOTGUN);
			p->ammo_amount[SHOTGUN_WEAPON]--;

			S_PlayActorSound(SHOTGUN_FIRE, pact);

			lastvisinc = PlayClock + 32;
			p->visibility = 0;

		}

		switch(p->kickback_pic)
		{
		case 13:
			checkavailweapon(p);
			break;
		case 15:
			S_PlayActorSound(SHOTGUN_COCK, pact);
			break;
		case 17:
		case 20:
			p->kickback_pic++;
			break;
		case 24:
		{
			auto j = spawn(pact, SHOTGUNSHELL);
			j->s->ang += 1024;
			ssp(j, CLIPMASK0);
			j->s->ang += 1024;
			p->kickback_pic++;
			break;
		}
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
					auto j = spawn(pact, SHELL);

					j->s->ang += 1024;
					j->s->ang &= 2047;
					j->s->xvel += 32;
					j->s->z += (3 << 8);
					ssp(j, CLIPMASK0);
				}

				S_PlayActorSound(CHAINGUN_FIRE, pact);
				fi.shoot(pact, CHAINGUN);
				lastvisinc = PlayClock + 32;
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
			fi.shoot(pact, GROWSPARK);

			//#ifdef NAM
			//#else
			if (!(aplWeaponFlags[p->curr_weapon][snum] & WEAPON_FLAG_NOVISIBLE))
			{
				// make them visible if not set...
				p->visibility = 0;
				lastvisinc = PlayClock + 32;
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
				lastvisinc = PlayClock + 32;
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
			fi.shoot(pact, SHRINKER);

			if (!isNam())
			{
				p->visibility = 0;
				//flashColor = 176 + (252 << 8) + (120 << 16);
				lastvisinc = PlayClock + 32;
				checkavailweapon(p);
			}
		}
		else if (isNam() && p->kickback_pic > 30)
		{
			p->okickback_pic = p->kickback_pic = 0;
			p->visibility = 0;
			lastvisinc = PlayClock + 32;
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
					lastvisinc = PlayClock + 32;
					fi.shoot(pact, RPG);
					p->ammo_amount[DEVISTATOR_WEAPON]--;
					checkavailweapon(p);
				}
				if (p->kickback_pic > 5) p->okickback_pic = p->kickback_pic = 0;
			}
			else if (p->kickback_pic & 1)
			{
				p->visibility = 0;
				lastvisinc = PlayClock + 32;
				fi.shoot(pact, RPG);
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
				lastvisinc = PlayClock + 32;
				fi.shoot(pact, FREEZEBLAST);
				checkavailweapon(p);
			}
			if (pact->s->xrepeat < 32)
			{
				p->okickback_pic = p->kickback_pic = 0; break;
			}
		}
		else
		{
			if (actions & SB_FIRE)
			{
				p->okickback_pic = p->kickback_pic = 1;
				S_PlayActorSound(CAT_FIRE, pact);
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
			if (p->cursector()->lotag != 2) 
			{
				p->ammo_amount[FLAMETHROWER_WEAPON]--;
				if (snum == screenpeek)
					g_visibility = 0;
				fi.shoot(pact, FIREBALL);
			}
			checkavailweapon(p);
		}
		else if (p->kickback_pic == 16) 
		{
			if ((actions & SB_FIRE) != 0)
			{
				p->okickback_pic = p->kickback_pic = 1;
				S_PlayActorSound(FLAMETHROWER_INTRO, pact);
			}
			else
				p->okickback_pic = p->kickback_pic = 0;
		}
		break;

	case TRIPBOMB_WEAPON:	// Claymore in NAM
		if (p->kickback_pic < 4)
		{
			p->pos.z = p->oposz;
			p->poszv = 0;
			if (p->kickback_pic == 3)
				fi.shoot(pact, HANDHOLDINGLASER);
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

		if (p->kickback_pic == 7) fi.shoot(pact, KNEE);
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
			lastvisinc = PlayClock + 32;
			p->visibility = 0;
			fi.shoot(pact, RPG);
			checkavailweapon(p);
		}
		else if (p->kickback_pic == 20)
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
	auto pact = p->GetActor();
	auto s = pact->s;
	int shrunk = (s->yrepeat < 32);

	if (isNamWW2GI() && (actions & SB_HOLSTER)) // 'Holster Weapon
	{
		if (isWW2GI())
		{
			SetGameVarID(g_iReturnVarID, 0, p->GetActor(), snum);
			SetGameVarID(g_iWeaponVarID, p->curr_weapon, p->GetActor(), snum);
			SetGameVarID(g_iWorksLikeVarID, aplWeaponWorksLike[p->curr_weapon][snum], p->GetActor(), snum);
			OnEvent(EVENT_HOLSTER, snum, p->GetActor(), -1);
			if (GetGameVarID(g_iReturnVarID, p->GetActor(), snum) == 0)
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
	int j, k, doubvel, fz, cz, truefdist;
	Collision chz, clz;
	bool shrunk;
	int psect, psectlotag;
	struct player_struct* p;
	spritetype* s;

	p = &ps[snum];
	auto pact = p->GetActor();
	s = pact->s;

	p->horizon.resetadjustment();
	p->angle.resetadjustment();

	ESyncBits& actions = p->sync.actions;

	auto sb_fvel = PlayerInputForwardVel(snum);
	auto sb_svel = PlayerInputSideVel(snum);
	auto sb_avel = PlayerInputAngVel(snum);

	psect = p->cursectnum;
	if (psect == -1)
	{
		if (s->extra > 0 && ud.clipping == 0)
		{
			quickkill(p);
			S_PlayActorSound(SQUISHED, pact);
		}
		psect = 0;
	}

	psectlotag = sector[psect].lotag;
	p->spritebridge = 0;

	shrunk = (s->yrepeat < 32);
	getzrange_ex(p->pos.x, p->pos.y, p->pos.z, psect, &cz, chz, &fz, clz, 163L, CLIPMASK0);

	j = getflorzofslope(psect, p->pos.x, p->pos.y);

	p->truefz = j;
	p->truecz = getceilzofslope(psect, p->pos.x, p->pos.y);

	truefdist = abs(p->pos.z - j);
	if (clz.type == kHitSector && psectlotag == 1 && truefdist > gs.playerheight + (16 << 8))
		psectlotag = 0;

	pact->floorz = fz;
	pact->ceilingz = cz;

	if (SyncInput())
	{
		p->horizon.backup();
		doslopetilting(p);
	}

	if (chz.type == kHitSprite)
	{
		if (chz.actor->s->statnum == 1 && chz.actor->s->extra >= 0)
		{
			chz.type = kHitNone;
			chz.actor = nullptr;
			cz = p->truecz;
		}
	}

	if (clz.type == kHitSprite)
	{
		if ((clz.actor->s->cstat & 33) == 33)
		{
			psectlotag = 0;
			p->footprintcount = 0;
			p->spritebridge = 1;
		}
		else if (badguy(clz.actor) && clz.actor->s->xrepeat > 24 && abs(s->z - clz.actor->s->z) < (84 << 8))
		{
			j = getangle(clz.actor->s->x - p->pos.x, clz.actor->s->y - p->pos.y);
			p->posxv -= bcos(j, 4);
			p->posyv -= bsin(j, 4);
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

	if (p->newOwner != nullptr)
	{
		p->posxv = p->posyv = s->xvel = 0;

		fi.doincrements(p);

		if (isWW2GI() && aplWeaponWorksLike[p->curr_weapon][snum] == HANDREMOTE_WEAPON) processweapon(snum, actions, psect);
		if (!isWW2GI() && p->curr_weapon == HANDREMOTE_WEAPON) processweapon(snum, actions, psect);
		return;
	}

	doubvel = TICSPERFRAME;

	checklook(snum,actions);
	int ii = 40;

	if (p->on_crane != nullptr)
		goto HORIZONLY;

	p->playerweaponsway(s->xvel);

	s->xvel = clamp(ksqrt((p->pos.x - p->bobposx) * (p->pos.x - p->bobposx) + (p->pos.y - p->bobposy) * (p->pos.y - p->bobposy)), 0, 512);
	if (p->on_ground) p->bobcounter += p->GetActor()->s->xvel >> 1;

	p->backuppos(ud.clipping == 0 && (p->cursector()->floorpicnum == MIRROR || p->cursectnum < 0 || p->cursectnum >= MAXSECTORS));

	// Shrinking code

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

	if (movementBlocked(p))
	{
		doubvel = 0;
		p->posxv = 0;
		p->posyv = 0;
	}
	else if (SyncInput())
	{
		//p->ang += syncangvel * constant
		//ENGINE calculates angvel for you
		// may still be needed later for demo recording

		sb_avel = p->adjustavel(sb_avel);
		p->angle.applyinput(sb_avel, &actions);
	}

	if (p->spritebridge == 0)
	{
		j = s->sector()->floorpicnum;

		if (j == PURPLELAVA || s->sector()->ceilingpicnum == PURPLELAVA)
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
				if (!S_CheckActorSoundPlaying(pact, DUKE_LONGTERM_PAIN))
					S_PlayActorSound(DUKE_LONGTERM_PAIN, pact);
				SetPlayerPal(p, PalEntry(32, 0, 8, 0));
				s->extra--;
			}
		}

		k = 0;

		if (p->on_ground && truefdist <= gs.playerheight + (16 << 8))
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

		k = bsin(p->bobcounter, -12);

		if (truefdist < gs.playerheight + (8 << 8) && (k == 1 || k == 3))
		{
			if (p->spritebridge == 0 && p->walking_snd_toggle == 0 && p->on_ground)
			{
				switch (psectlotag)
				{
				case 0:

					if (clz.type == kHitSprite)
						j = clz.actor->s->picnum;
					else
						j = sector[psect].floorpicnum;

					switch (j)
					{
					case PANNEL1:
					case PANNEL2:
						S_PlayActorSound(DUKE_WALKINDUCTS, pact);
						p->walking_snd_toggle = 1;
						break;
					}
					break;
				case 1:
					if ((krand() & 1) == 0)
						S_PlayActorSound(DUKE_ONWATER, pact);
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
			p->posxv = MulScale(p->posxv, gs.playerfriction - 0x2000, 16);
			p->posyv = MulScale(p->posyv, gs.playerfriction - 0x2000, 16);
		}
		else
		{
			if (psectlotag == 2)
			{
				p->posxv = MulScale(p->posxv, gs.playerfriction - 0x1400, 16);
				p->posyv = MulScale(p->posyv, gs.playerfriction - 0x1400, 16);
			}
			else
			{
				p->posxv = MulScale(p->posxv, gs.playerfriction, 16);
				p->posyv = MulScale(p->posyv, gs.playerfriction, 16);
			}
		}

		if (abs(p->posxv) < 2048 && abs(p->posyv) < 2048)
			p->posxv = p->posyv = 0;

		if (shrunk)
		{
			p->posxv =
				MulScale(p->posxv, gs.playerfriction - (gs.playerfriction >> 1) + (gs.playerfriction >> 2), 16);
			p->posyv =
				MulScale(p->posyv, gs.playerfriction - (gs.playerfriction >> 1) + (gs.playerfriction >> 2), 16);
		}
	}

HORIZONLY:

	if (psectlotag == 1 || p->spritebridge == 1) ii = (4L << 8);
	else ii = (20L << 8);

	if (p->cursector()->lotag == 2) k = 0;
	else k = 1;

	Collision clip{};
	if (ud.clipping)
	{
		p->pos.x += p->posxv >> 14;
		p->pos.y += p->posyv >> 14;
		updatesector(p->pos.x, p->pos.y, &p->cursectnum);
		changeactorsect(pact, p->cursectnum);
	}
	else
		clipmove_ex(&p->pos, &p->cursectnum, p->posxv, p->posyv, 164, (4 << 8), ii, CLIPMASK0, clip);

	if (p->jetpack_on == 0 && psectlotag != 2 && psectlotag != 1 && shrunk)
		p->pos.z += 32 << 8;

	if (clip.type != kHitNone)
		checkplayerhurt_d(p, clip);

	if (p->jetpack_on == 0)
	{
		if (s->xvel > 16)
		{
			if (psectlotag != 1 && psectlotag != 2 && p->on_ground)
			{
				p->pycount += 52;
				p->pycount &= 2047;
				p->pyoff = abs(s->xvel * bsin(p->pycount)) / 1596;
			}
		}
		else if (psectlotag != 2 && psectlotag != 1)
			p->pyoff = 0;
	}

	// RBG***
	setsprite(pact, p->pos.x, p->pos.y, p->pos.z + gs.playerheight);

	if (psectlotag < 3)
	{
		psect = s->sectnum;
		if (ud.clipping == 0 && sector[psect].lotag == 31)
		{
			auto secact = ScriptIndexToActor(sector[psect].hitag);
			if (secact && secact->s->xvel && secact->temp_data[0] == 0)
			{
				quickkill(p);
				return;
			}
		}
	}

	if (truefdist < gs.playerheight && p->on_ground && psectlotag != 1 && shrunk == 0 && p->cursector()->lotag == 1)
		if (!S_CheckActorSoundPlaying(pact, DUKE_ONWATER))
			S_PlayActorSound(DUKE_ONWATER, pact);

	if (p->cursectnum != s->sectnum)
		changeactorsect(pact, p->cursectnum);

	if (ud.clipping == 0)
		j = (pushmove(&p->pos, &p->cursectnum, 164L, (4L << 8), (4L << 8), CLIPMASK0) < 0 && furthestangle(p->GetActor(), 8) < 512);
	else j = 0;

	if (ud.clipping == 0)
	{
		if (abs(pact->floorz - pact->ceilingz) < (48 << 8) || j)
		{
			if (!(s->sector()->lotag & 0x8000) && (isanunderoperator(s->sector()->lotag) ||
				isanearoperator(s->sector()->lotag)))
				fi.activatebysector(s->sectnum, pact);
			if (j)
			{
				quickkill(p);
				return;
			}
		}
		else if (abs(fz - cz) < (32 << 8) && isanunderoperator(sector[psect].lotag))
			fi.activatebysector(psect, pact);
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

	if (SyncInput())
	{
		p->horizon.applyinput(GetPlayerHorizon(snum), &actions);
	}

	p->checkhardlanding();

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

	dokneeattack(snum, { FEM1, FEM2, FEM3, FEM4, FEM5, FEM6, FEM7, FEM8, FEM9, FEM10, PODFEM1, NAKED1, STATUE });

	if (fi.doincrements(p)) return;

	if (p->weapon_pos != 0)
	{
		if (p->weapon_pos == -9)
		{
			if (p->last_weapon >= 0)
			{
				p->oweapon_pos = p->weapon_pos = 10;
				// if(p->curr_weapon == KNEE_WEAPON) p->kickback_pic = 1;
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
