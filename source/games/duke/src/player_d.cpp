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
void operateweapon_ww(int snum, ESyncBits actions);

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void incur_damage_d(player_struct* p)
{
	int  damage = 0L, shield_damage = 0L;

	p->GetActor()->spr.extra -= p->extra_extra8 >> 8;

	damage = p->GetActor()->spr.extra - p->last_extra;

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

		p->GetActor()->spr.extra = p->last_extra + damage;
	}
}


//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

static void shootfireball(DDukeActor *actor, int p, int sx, int sy, int sz, int sa)
{
	int vel, zvel;

	if (actor->spr.extra >= 0)
		actor->spr.shade = -96;

	sz -= (4 << 7);
	if (actor->spr.picnum != BOSS5)
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
		zvel = (((ps[j].player_int_opos().Z - sz + (3 << 8))) * vel) / ldist(ps[j].GetActor(), actor);
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

	auto spawned = EGS(actor->sector(), sx, sy, sz, FIREBALL, -127, sizx, sizy, sa, vel, zvel, actor, (short)4);
	if (spawned)
	{
		spawned->spr.extra += (krand() & 7);
		if (actor->spr.picnum == BOSS5 || p >= 0)
		{
			spawned->spr.xrepeat = 40;
			spawned->spr.yrepeat = 40;
		}
		spawned->spr.yvel = p;
		spawned->spr.cstat = CSTAT_SPRITE_YCENTER;
		spawned->spr.clipdist = 4;
	}
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

static void shootflamethrowerflame(DDukeActor* actor, int p, int sx, int sy, int sz, int sa)
{
	int vel, zvel = 0;

	if (actor->spr.extra >= 0)
		actor->spr.shade = -96;
	vel = 400;

	DDukeActor* spawned = nullptr;
	if (p < 0)
	{
		int x;
		int j = findplayer(actor, &x);
		sa = getangle(ps[j].player_int_opos().X - sx, ps[j].player_int_opos().Y - sy);

		if (actor->spr.picnum == BOSS5)
		{
			vel = 528;
			sz += 6144;
		}
		else if (actor->spr.picnum == BOSS3)
			sz -= 8192;

		int l = ldist(ps[j].GetActor(), actor);
		if (l != 0)
			zvel = ((ps[j].player_int_opos().Z - sz) * vel) / l;

		if (badguy(actor) && (actor->spr.hitag & face_player_smart) != 0)
			sa = (short)(actor->int_ang() + (krand() & 31) - 16);

		if (actor->sector()->lotag == 2 && (krand() % 5) == 0)
			spawned = spawn(actor, WATERBUBBLE);
	}
	else
	{
		zvel = -MulScale(ps[p].horizon.sum().asq16(), 81, 16);
		if (ps[p].GetActor()->spr.xvel != 0)
			vel = (int)((((512 - (1024
				- abs(abs(getangle(sx - ps[p].player_int_opos().X, sy - ps[p].player_int_opos().Y) - sa) - 1024)))
				* 0.001953125f) * ps[p].GetActor()->spr.xvel) + 400);
		if (actor->sector()->lotag == 2 && (krand() % 5) == 0)
			spawned = spawn(actor, WATERBUBBLE);
	}

	if (spawned == nullptr)
	{
		spawned = spawn(actor, FLAMETHROWERFLAME);
		if (!spawned) return;
		spawned->spr.xvel = (short)vel;
		spawned->spr.zvel = (short)zvel;
	}

	spawned->set_int_pos({ sx + bsin(sa + 630) / 448, sy + bsin(sa + 112) / 448, sz - 256 });
	spawned->setsector(actor->sector());
	spawned->spr.cstat = CSTAT_SPRITE_YCENTER;
	spawned->set_int_ang(sa);
	spawned->spr.xrepeat = 2;
	spawned->spr.yrepeat = 2;
	spawned->spr.clipdist = 40;
	spawned->spr.yvel = p;
	spawned->SetOwner(actor);

	if (p == -1)
	{
		if (actor->spr.picnum == BOSS5)
		{
			spawned->add_int_pos({ -bsin(sa) / 56, bcos(sa) / 56, 0 });
			spawned->spr.xrepeat = 10;
			spawned->spr.yrepeat = 10;
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
	auto sectp = actor->sector();
	int zvel;
	HitInfo hit{};

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
		zvel = ((pactor->int_pos().Z - sz) << 8) / (x + 1);
		sa = getangle(pactor->int_pos().X - sx, pactor->int_pos().Y - sy);
	}

	hitscan(vec3_t( sx, sy, sz ), sectp, { bcos(sa), bsin(sa), zvel << 6 }, hit, CLIPMASK1);


	if (hit.hitSector == nullptr) return;

	if ((abs(sx - hit.int_hitpos().X) + abs(sy - hit.int_hitpos().Y)) < 1024)
	{
		if (hit.hitWall || hit.actor())
		{
			auto knee = EGS(hit.hitSector, hit.int_hitpos().X, hit.int_hitpos().Y, hit.int_hitpos().Z, KNEE, -15, 0, 0, sa, 32, 0, actor, 4);
			if (knee)
			{
				knee->spr.extra += (krand() & 7);
				if (p >= 0)
				{
					auto k = spawn(knee, SMALLSMOKE);
					if (k) k->spr.pos.Z -= 8;
					S_PlayActorSound(KICK_HIT, knee);
				}

				if (p >= 0 && ps[p].steroids_amount > 0 && ps[p].steroids_amount < 400)
					knee->spr.extra += (gs.max_player_health >> 2);
			}
			if (hit.actor() && hit.actor()->spr.picnum != ACCESSSWITCH && hit.actor()->spr.picnum != ACCESSSWITCH2)
			{
				fi.checkhitsprite(hit.actor(), knee);
				if (p >= 0) fi.checkhitswitch(p, nullptr, hit.actor());
			}

			else if (hit.hitWall)
			{
				if (hit.hitWall->cstat & CSTAT_WALL_BOTTOM_SWAP)
					if (hit.hitWall->twoSided())
						if (hit.hitpos.Z >= hit.hitWall->nextSector()->floorz)
							hit.hitWall =hit.hitWall->nextWall();

				if (hit.hitWall->picnum != ACCESSSWITCH && hit.hitWall->picnum != ACCESSSWITCH2)
				{
					fi.checkhitwall(knee, hit.hitWall, hit.int_hitpos().X, hit.int_hitpos().Y, hit.int_hitpos().Z, KNEE);
					if (p >= 0) fi.checkhitswitch(p, hit.hitWall, nullptr);
				}
			}
		}
		else if (p >= 0 && zvel > 0 && hit.hitSector->lotag == 1)
		{
			auto splash = spawn(ps[p].GetActor(), WATERSPLASH2);
			if (splash)
			{
				splash->set_int_xy(hit.int_hitpos().X, hit.int_hitpos().Y);
				splash->set_int_ang(ps[p].angle.ang.Buildang()); // Total tweek
				splash->spr.xvel = 32;
				ssp(actor, CLIPMASK0);
				splash->spr.xvel = 0;
			}

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
	auto sectp = actor->sector();
	int zvel = 0;
	HitInfo hit{};

	if (actor->spr.extra >= 0) actor->spr.shade = -96;

	if (p >= 0)
	{
		SetGameVarID(g_iAimAngleVarID, AUTO_AIM_ANGLE, actor, p);
		OnEvent(EVENT_GETAUTOAIMANGLE, p, ps[p].GetActor(), -1);
		int varval = GetGameVarID(g_iAimAngleVarID, actor, p).value();
		DDukeActor* aimed = nullptr;
		if (varval > 0)
		{
			aimed = aim(actor, varval);
		}

		if (aimed)
		{
			int dal = ((aimed->spr.xrepeat * tileHeight(aimed->spr.picnum)) << 1) + (5 << 8);
			switch (aimed->spr.picnum)
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
			zvel = ((aimed->int_pos().Z - sz - dal) << 8) / ldist(ps[p].GetActor(), aimed);
			sa = getangle(aimed->int_pos().X - sx, aimed->int_pos().Y - sy);
		}

		if (isWW2GI())
		{
			int angRange = 32;
			int zRange = 256;
			SetGameVarID(g_iAngRangeVarID, 32, actor, p);
			SetGameVarID(g_iZRangeVarID, 256, actor, p);
			OnEvent(EVENT_GETSHOTRANGE, p, ps[p].GetActor(), -1);
			angRange = GetGameVarID(g_iAngRangeVarID, actor, p).value();
			zRange = GetGameVarID(g_iZRangeVarID, actor, p).value();

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
		zvel = ((ps[j].player_int_pos().Z - sz) << 8) / (ldist(ps[j].GetActor(), actor));
		if (actor->spr.picnum != BOSS1)
		{
			zvel += 128 - (krand() & 255);
			sa += 32 - (krand() & 63);
		}
		else
		{
			zvel += 128 - (krand() & 255);
			sa = getangle(ps[j].player_int_pos().X - sx, ps[j].player_int_pos().Y - sy) + 64 - (krand() & 127);
		}
	}

	actor->spr.cstat &= ~CSTAT_SPRITE_BLOCK_ALL;
	hitscan(vec3_t( sx, sy, sz ), sectp, { bcos(sa), bsin(sa), zvel << 6 }, hit, CLIPMASK1);
	actor->spr.cstat |= CSTAT_SPRITE_BLOCK_ALL;


	if (hit.hitSector == nullptr) return;

	if ((krand() & 15) == 0 && hit.hitSector->lotag == 2)
		tracers(hit.int_hitpos().X, hit.int_hitpos().Y, hit.int_hitpos().Z, sx, sy, sz, 8 - (ud.multimode >> 1));

	DDukeActor* spark;
	if (p >= 0)
	{
		spark = EGS(hit.hitSector, hit.int_hitpos().X, hit.int_hitpos().Y, hit.int_hitpos().Z, SHOTSPARK1, -15, 10, 10, sa, 0, 0, actor, 4);
		if (!spark) return;

		spark->spr.extra = ScriptCode[gs.actorinfo[atwith].scriptaddress];
		spark->spr.extra += (krand() % 6);

		if (hit.hitWall == nullptr && hit.actor() == nullptr)
		{
			if (zvel < 0)
			{
				if (hit.hitSector->ceilingstat & CSTAT_SECTOR_SKY)
				{
					spark->spr.xrepeat = 0;
					spark->spr.yrepeat = 0;
					return;
				}
				else
					fi.checkhitceiling(hit.hitSector);
			}
			spawn(spark, SMALLSMOKE);
		}

		if (hit.actor())
		{
			fi.checkhitsprite(hit.actor(), spark);
			if (hit.actor()->isPlayer() && (ud.coop != 1 || ud.ffire == 1))
			{
				spark->spr.xrepeat = spark->spr.yrepeat = 0;
				auto jib = spawn(spark, JIBS6);
				if (jib)
				{
					jib->spr.pos.Z += 4;
					jib->spr.xvel = 16;
					jib->spr.xrepeat = jib->spr.yrepeat = 24;
					jib->add_int_ang(64 - (krand() & 127));
				}
			}
			else spawn(spark, SMALLSMOKE);

			if (p >= 0 && (
				hit.actor()->spr.picnum == DIPSWITCH ||
				hit.actor()->spr.picnum == DIPSWITCH + 1 ||
				hit.actor()->spr.picnum == DIPSWITCH2 ||
				hit.actor()->spr.picnum == DIPSWITCH2 + 1 ||
				hit.actor()->spr.picnum == DIPSWITCH3 ||
				hit.actor()->spr.picnum == DIPSWITCH3 + 1 ||
				hit.actor()->spr.picnum == HANDSWITCH ||
				hit.actor()->spr.picnum == HANDSWITCH + 1))
			{
				fi.checkhitswitch(p, nullptr, hit.actor());
				return;
			}
		}
		else if (hit.hitWall)
		{
			spawn(spark, SMALLSMOKE);

			if (fi.isadoorwall(hit.hitWall->picnum) == 1)
				goto SKIPBULLETHOLE;
			if (p >= 0 && (
				hit.hitWall->picnum == DIPSWITCH ||
				hit.hitWall->picnum == DIPSWITCH + 1 ||
				hit.hitWall->picnum == DIPSWITCH2 ||
				hit.hitWall->picnum == DIPSWITCH2 + 1 ||
				hit.hitWall->picnum == DIPSWITCH3 ||
				hit.hitWall->picnum == DIPSWITCH3 + 1 ||
				hit.hitWall->picnum == HANDSWITCH ||
				hit.hitWall->picnum == HANDSWITCH + 1))
			{
				fi.checkhitswitch(p, hit.hitWall, nullptr);
				return;
			}

			if (hit.hitWall->hitag != 0 || (hit.hitWall->twoSided() && hit.hitWall->nextWall()->hitag != 0))
				goto SKIPBULLETHOLE;

			if (hit.hitSector && hit.hitSector->lotag == 0)
				if (hit.hitWall->overpicnum != BIGFORCE)
					if ((hit.hitWall->twoSided() && hit.hitWall->nextSector()->lotag == 0) ||
						(!hit.hitWall->twoSided() && hit.hitSector->lotag == 0))
						if ((hit.hitWall->cstat & CSTAT_WALL_MASKED) == 0)
						{
							if (hit.hitWall->twoSided())
							{
								DukeSectIterator it(hit.hitWall->nextSector());
								while (auto l = it.Next())
								{
									if (l->spr.statnum == STAT_EFFECTOR && l->spr.lotag == SE_13_EXPLOSIVE)
										goto SKIPBULLETHOLE;
								}
							}

							DukeStatIterator it(STAT_MISC);
							while (auto l = it.Next())
							{
								if (l->spr.picnum == BULLETHOLE)
									if (dist(l, spark) < (12 + (krand() & 7)))
										goto SKIPBULLETHOLE;
							}
							auto hole = spawn(spark, BULLETHOLE);
							if (hole)
							{
								hole->spr.xvel = -1;
								auto delta = hit.hitWall->delta();
								hole->set_int_ang(getangle(-delta.X, -delta.Y) + 512);
								ssp(hole, CLIPMASK0);
								hole->spr.cstat2 |= CSTAT2_SPRITE_DECAL;
							}
						}

		SKIPBULLETHOLE:

			if (hit.hitWall->cstat & CSTAT_WALL_BOTTOM_SWAP)
				if (hit.hitWall->twoSided())
					if (hit.hitpos.Z >= hit.hitWall->nextSector()->floorz)
						hit.hitWall = hit.hitWall->nextWall();

			fi.checkhitwall(spark, hit.hitWall, hit.int_hitpos().X, hit.int_hitpos().Y, hit.int_hitpos().Z, SHOTSPARK1);
		}
	}
	else
	{
		spark = EGS(hit.hitSector, hit.int_hitpos().X, hit.int_hitpos().Y, hit.int_hitpos().Z, SHOTSPARK1, -15, 24, 24, sa, 0, 0, actor, 4);
		if (spark)
		{
			spark->spr.extra = ScriptCode[gs.actorinfo[atwith].scriptaddress];

			if (hit.actor())
			{
				fi.checkhitsprite(hit.actor(), spark);
				if (!hit.actor()->isPlayer())
					spawn(spark, SMALLSMOKE);
				else spark->spr.xrepeat = spark->spr.yrepeat = 0;
			}
			else if (hit.hitWall)
				fi.checkhitwall(spark, hit.hitWall, hit.int_hitpos().X, hit.int_hitpos().Y, hit.int_hitpos().Z, SHOTSPARK1);
		}
	}

	if ((krand() & 255) < 4)
	{
		S_PlaySound3D(PISTOL_RICOCHET, spark, hit.int_hitpos());
	}
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

static void shootstuff(DDukeActor* actor, int p, int sx, int sy, int sz, int sa, int atwith)
{
	sectortype* sect = actor->sector();
	int vel, zvel;
	int l, scount;

	if (actor->spr.extra >= 0) actor->spr.shade = -96;

	scount = 1;
	if (atwith == SPIT) vel = 292;
	else
	{
		if (atwith == COOLEXPLOSION1)
		{
			if (actor->spr.picnum == BOSS2) vel = 644;
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
			int dal = ((aimed->spr.xrepeat * tileHeight(aimed->spr.picnum)) << 1) - (12 << 8);
			zvel = ((aimed->int_pos().Z - sz - dal) * vel) / ldist(ps[p].GetActor(), aimed);
			sa = getangle(aimed->int_pos().X - sx, aimed->int_pos().Y - sy);
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
		zvel = (((ps[j].player_int_opos().Z - sz + (3 << 8))) * vel) / ldist(ps[j].GetActor(), actor);
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
		if (!spawned) return;
		spawned->spr.extra += (krand() & 7);

		if (atwith == COOLEXPLOSION1)
		{
			spawned->spr.shade = 0;
			if (actor->spr.picnum == BOSS2)
			{
				l = spawned->spr.xvel;
				spawned->spr.xvel = 1024;
				ssp(spawned, CLIPMASK0);
				spawned->spr.xvel = l;
				spawned->add_int_ang(128 - (krand() & 255));
			}
		}

		spawned->spr.cstat = CSTAT_SPRITE_YCENTER;
		spawned->spr.clipdist = 4;

		sa = actor->int_ang() + 32 - (krand() & 63);
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
	auto sect = actor->sector();
	int vel, zvel;
	int l, scount;

	if (actor->spr.extra >= 0) actor->spr.shade = -96;

	scount = 1;
	vel = 644;

	DDukeActor* aimed = nullptr;

	if (p >= 0)
	{
		aimed = aim(actor, 48);
		if (aimed)
		{
			int dal = ((aimed->spr.xrepeat * tileHeight(aimed->spr.picnum)) << 1) + (8 << 8);
			zvel = ((aimed->int_pos().Z - sz - dal) * vel) / ldist(ps[p].GetActor(), aimed);
			if (aimed->spr.picnum != RECON)
				sa = getangle(aimed->int_pos().X - sx, aimed->int_pos().Y - sy);
		}
		else zvel = -MulScale(ps[p].horizon.sum().asq16(), 81, 16);
		if (atwith == RPG)
			S_PlayActorSound(RPG_SHOOT, actor);

	}
	else
	{
		int x;
		int j = findplayer(actor, &x);
		sa = getangle(ps[j].player_int_opos().X - sx, ps[j].player_int_opos().Y - sy);
		if (actor->spr.picnum == BOSS3)
		{
			int zoffs = (32 << 8);
			if (isWorldTour()) // Twentieth Anniversary World Tour
				zoffs = (int)((actor->spr.yrepeat / 80.0f) * zoffs);
			sz -= zoffs;
		}
		else if (actor->spr.picnum == BOSS2)
		{
			vel += 128;
			int zoffs = 24 << 8;
			if (isWorldTour()) // Twentieth Anniversary World Tour
				zoffs = (int)((actor->spr.yrepeat / 80.0f) * zoffs);
			sz += zoffs;
		}

		l = ldist(ps[j].GetActor(), actor);
		zvel = ((ps[j].player_int_opos().Z - sz) * vel) / l;

		if (badguy(actor) && (actor->spr.hitag & face_player_smart))
			sa = actor->int_ang() + (krand() & 31) - 16;
	}
	if (p < 0) aimed = nullptr;

	auto spawned = EGS(sect,
		sx + (bcos(sa + 348) / 448),
		sy + (bsin(sa + 348) / 448),
		sz - (1 << 8), atwith, 0, 14, 14, sa, vel, zvel, actor, 4);

	if (!spawned) return;

	spawned->spr.extra += (krand() & 7);
	if (atwith != FREEZEBLAST)
		spawned->temp_actor = aimed;
	else
	{
		spawned->spr.yvel = gs.numfreezebounces;
		spawned->spr.xrepeat >>= 1;
		spawned->spr.yrepeat >>= 1;
		spawned->spr.zvel -= (2 << 4);
	}

	if (p == -1)
	{
		if (actor->spr.picnum == BOSS3)
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
				float siz = actor->spr.yrepeat / 80.0f;
				xoffs = int(xoffs * siz);
				yoffs = int(yoffs * siz);
				aoffs = int(aoffs * siz);
			}

			spawned->add_int_pos({ xoffs, yoffs, 0 });
			spawned->add_int_ang(aoffs);

			spawned->spr.xrepeat = 42;
			spawned->spr.yrepeat = 42;
		}
		else if (actor->spr.picnum == BOSS2)
		{
			int xoffs = bsin(sa) / 56;
			int yoffs = -bcos(sa) / 56;
			int aoffs = 8 + (krand() & 255) - 128;

			if (isWorldTour()) { // Twentieth Anniversary World Tour
				int siz = actor->spr.yrepeat;
				xoffs = Scale(xoffs, siz, 80);
				yoffs = Scale(yoffs, siz, 80);
				aoffs = Scale(aoffs, siz, 80);
			}

			spawned->add_int_pos({ -xoffs, -yoffs, 0 });
			spawned->add_int_ang(-aoffs);

			spawned->spr.xrepeat = 24;
			spawned->spr.yrepeat = 24;
		}
		else if (atwith != FREEZEBLAST)
		{
			spawned->spr.xrepeat = 30;
			spawned->spr.yrepeat = 30;
			spawned->spr.extra >>= 2;
		}
	}
	else if ((isWW2GI() && aplWeaponWorksLike(ps[p].curr_weapon, p) == DEVISTATOR_WEAPON) || (!isWW2GI() && ps[p].curr_weapon == DEVISTATOR_WEAPON))
	{
		spawned->spr.extra >>= 2;
		spawned->add_int_ang(16 - (krand() & 31));
		spawned->spr.zvel += 256 - (krand() & 511);

		if (ps[p].hbomb_hold_delay)
		{
			spawned->add_int_pos({ -bsin(sa) / 644, bcos(sa) / 644, 0 });
		}
		else
		{
			spawned->add_int_pos({ bsin(sa, -8), -bcos(sa, -8), 0 });
		}
		spawned->spr.xrepeat >>= 1;
		spawned->spr.yrepeat >>= 1;
	}

	spawned->spr.cstat = CSTAT_SPRITE_YCENTER;
	if (atwith == RPG)
		spawned->spr.clipdist = 4;
	else
		spawned->spr.clipdist = 40;

}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

static void shootlaser(DDukeActor* actor, int p, int sx, int sy, int sz, int sa)
{
	auto sectp = actor->sector();
	int zvel;
	int j;
	HitInfo hit{};

	if (p >= 0)
		zvel = -ps[p].horizon.sum().asq16() >> 11;
	else zvel = 0;

	hitscan(vec3_t( sx, sy, sz - ps[p].pyoff ), sectp, { bcos(sa), bsin(sa), zvel << 6 }, hit, CLIPMASK1);

	j = 0;
	if (hit.actor()) return;

	if (hit.hitWall && hit.hitSector)
	{
		if (((hit.int_hitpos().X - sx) * (hit.int_hitpos().X - sx) + (hit.int_hitpos().Y - sy) * (hit.int_hitpos().Y - sy)) < (290 * 290))
		{
			if (hit.hitWall->twoSided())
			{
				if (hit.hitWall->nextSector()->lotag <= 2 && hit.hitSector->lotag <= 2)
					j = 1;
			}
			else if (hit.hitSector->lotag <= 2)
				j = 1;
		}

		if (j == 1)
		{
			auto bomb = EGS(hit.hitSector, hit.int_hitpos().X, hit.int_hitpos().Y, hit.int_hitpos().Z, TRIPBOMB, -16, 4, 5, sa, 0, 0, actor, STAT_STANDABLE);
			if (!bomb) return;
			if (isWW2GI())
			{
				int lTripBombControl = GetGameVar("TRIPBOMB_CONTROL", TRIPBOMB_TRIPWIRE, nullptr, -1).value();
				if (lTripBombControl & TRIPBOMB_TIMER)
				{
					int lLifetime = GetGameVar("STICKYBOMB_LIFETIME", NAM_GRENADE_LIFETIME, nullptr, p).value();
					int lLifetimeVar = GetGameVar("STICKYBOMB_LIFETIME_VAR", NAM_GRENADE_LIFETIME_VAR, nullptr, p).value();
					// set timer.  blows up when at zero....
					bomb->spr.extra = lLifetime
					+ MulScale(krand(), lLifetimeVar, 14)
					- lLifetimeVar;
				}
			}

			// this originally used the sprite index as tag to link the laser segments.
			// This value is never used again to reference an actor by index. Decouple this for robustness.
			ud.bomb_tag = (ud.bomb_tag + 1) & 32767;
			bomb->spr.hitag = ud.bomb_tag;
			S_PlayActorSound(LASERTRIP_ONWALL, bomb);
			bomb->spr.xvel = -20;
			ssp(bomb, CLIPMASK0);
			bomb->spr.cstat = CSTAT_SPRITE_ALIGNMENT_WALL;
			auto delta = hit.hitWall->delta();
			bomb->set_int_ang(getangle(-delta.X, -delta.Y) - 512);
			bomb->temp_data[5] = bomb->int_ang();

			if (p >= 0)
				ps[p].ammo_amount[TRIPBOMB_WEAPON]--;
		}
	}
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

static void shootgrowspark(DDukeActor* actor, int p, int sx, int sy, int sz, int sa)
{
	auto sect = actor->sector();
	int zvel;
	int k;
	HitInfo hit{};

	if (p >= 0)
	{
		auto aimed = aim(actor, AUTO_AIM_ANGLE);
		if (aimed)
		{
			int dal = ((aimed->spr.xrepeat * tileHeight(aimed->spr.picnum)) << 1) + (5 << 8);
			switch (aimed->spr.picnum)
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
			zvel = ((aimed->int_pos().Z - sz - dal) << 8) / (ldist(ps[p].GetActor(), aimed));
			sa = getangle(aimed->int_pos().X - sx, aimed->int_pos().Y - sy);
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
		zvel = ((ps[j].player_int_pos().Z - sz) << 8) / (ldist(ps[j].GetActor(), actor));
		zvel += 128 - (krand() & 255);
		sa += 32 - (krand() & 63);
	}

	k = 0;

	//RESHOOTGROW:

	actor->spr.cstat &= ~CSTAT_SPRITE_BLOCK_ALL;
	hitscan(vec3_t( sx, sy, sz ), sect, { bcos(sa), bsin(sa), zvel << 6 }, hit, CLIPMASK1);

	actor->spr.cstat |= CSTAT_SPRITE_BLOCK_ALL;

	auto spark = EGS(sect, hit.int_hitpos().X, hit.int_hitpos().Y, hit.int_hitpos().Z, GROWSPARK, -16, 28, 28, sa, 0, 0, actor, 1);
	if (!spark) return;

	spark->spr.pal = 2;
	spark->spr.cstat |= CSTAT_SPRITE_YCENTER | CSTAT_SPRITE_TRANSLUCENT;
	spark->spr.xrepeat = spark->spr.yrepeat = 1;

	if (hit.hitWall == nullptr && hit.actor() == nullptr && hit.hitSector != nullptr)
	{
		if (zvel < 0 && (hit.hitSector->ceilingstat & CSTAT_SECTOR_SKY) == 0)
			fi.checkhitceiling(hit.hitSector);
	}
	else if (hit.actor() != nullptr) fi.checkhitsprite(hit.actor(), spark);
	else if (hit.hitWall != nullptr)
	{
		if (hit.hitWall->picnum != ACCESSSWITCH && hit.hitWall->picnum != ACCESSSWITCH2)
		{
			fi.checkhitwall(spark, hit.hitWall, hit.int_hitpos().X, hit.int_hitpos().Y, hit.int_hitpos().Z, GROWSPARK);
		}
	}
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void shoot_d(DDukeActor* actor, int atwith)
{
	int l, j;
	int sx, sy, sz, sa, p, vel, zvel, x, dal;
	if (actor->isPlayer())
	{
		p = actor->spr.yvel;
	}
	else
	{
		p = -1;
	}

	SetGameVarID(g_iAtWithVarID, atwith, actor, p);
	SetGameVarID(g_iReturnVarID, 0, actor, p);
	OnEvent(EVENT_SHOOT, p, ps[p].GetActor(), -1);
	if (GetGameVarID(g_iReturnVarID, actor, p).safeValue() != 0)
	{
		return;
	}


	auto sect = actor->sector();
	zvel = 0;

	if (actor->isPlayer())
	{
		sx = ps[p].player_int_pos().X;
		sy = ps[p].player_int_pos().Y;
		sz = ps[p].player_int_pos().Z + ps[p].pyoff + (4 << 8);
		sa = ps[p].angle.ang.Buildang();

		ps[p].crack_time = CRACK_TIME;

	}
	else
	{
		sa = actor->int_ang();
		sx = actor->int_pos().X;
		sy = actor->int_pos().Y;
		sz = actor->int_pos().Z - (actor->spr.yrepeat * tileHeight(actor->spr.picnum) << 1) + (4 << 8);
		if (actor->spr.picnum != ROTATEGUN)
		{
			sz -= (7 << 8);
			if (badguy(actor) && actor->spr.picnum != COMMANDER)
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
			if (k)
			{
				k->setsector(sect);
				k->set_int_pos({ sx, sy, sz });
				k->set_int_ang(sa);
				k->spr.xvel = 500;
				k->spr.zvel = 0;
			}
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
		[[fallthrough]];
	case RPG:

		shootrpg(actor, p, sx, sy, sz, sa, atwith);
		break;

	case HANDHOLDINGLASER:
		shootlaser(actor, p, sx, sy, sz, sa);
		return;

	case BOUNCEMINE:
	case MORTER:
	{
		if (actor->spr.extra >= 0) actor->spr.shade = -96;

		auto plActor = ps[findplayer(actor, &x)].GetActor();
		x = ldist(plActor, actor);

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
		if (actor->spr.extra >= 0) actor->spr.shade = -96;
		if (p >= 0)
		{
			auto aimed = isNamWW2GI() ? nullptr : aim(actor, AUTO_AIM_ANGLE);
			if (aimed)
			{
				dal = ((aimed->spr.xrepeat * tileHeight(aimed->spr.picnum)) << 1);
				zvel = ((aimed->int_pos().Z - sz - dal - (4 << 8)) * 768) / (ldist(ps[p].GetActor(), aimed));
				sa = getangle(aimed->int_pos().X - sx, aimed->int_pos().Y - sy);
			}
			else zvel = -MulScale(ps[p].horizon.sum().asq16(), 98, 16);
		}
		else if (actor->spr.statnum != 3)
		{
			j = findplayer(actor, &x);
			l = ldist(ps[j].GetActor(), actor);
			zvel = ((ps[j].player_int_opos().Z - sz) * 512) / l;
		}
		else zvel = 0;

		auto spawned = EGS(sect,
			sx - bsin(sa, -12),
			sy + bcos(sa, -12),
			sz + (2 << 8), SHRINKSPARK, -16, 28, 28, sa, 768, zvel, actor, 4);

		if (spawned)
		{
			spawned->spr.cstat = CSTAT_SPRITE_YCENTER;
			spawned->spr.clipdist = 32;
		}


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
	if (p->last_pissed_time <= (26 * 218) && p->show_empty_weapon == 0 && p->kickback_pic == 0 && p->quick_kick == 0 && p->GetActor()->spr.xrepeat > 32 && p->access_incs == 0 && p->knee_incs == 0)
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
						{
							if (k == SHRINKER_WEAPON && (p->subweapon & (1 << GROW_WEAPON)))
								k = GROW_WEAPON;
							if (isWorldTour() && k == FREEZE_WEAPON && (p->subweapon & (1 << FLAMETHROWER_WEAPON)) != 0)
								k = FLAMETHROWER_WEAPON;
						}
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
					if (act->spr.picnum == HEAVYHBOMB && act->GetOwner() == p->GetActor())
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

int doincrements_d(player_struct* p)
{
	int snum;

	auto pact = p->GetActor();
	snum = pact->spr.yvel;

	p->player_par++;

	if (p->invdisptime > 0)
		p->invdisptime--;

	if (p->tipincs > 0)
	{
		p->otipincs = p->tipincs;
		p->tipincs--;
	}

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

	if (p->quick_kick > 0 && p->GetActor()->spr.pal != 1)
	{
		p->last_quick_kick = p->quick_kick + 1;
		p->quick_kick--;
		if (p->quick_kick == 8)
			fi.shoot(p->GetActor(), KNEE);
	}
	else if (p->last_quick_kick > 0)
		p->last_quick_kick--;

	if (p->access_incs && p->GetActor()->spr.pal != 1)
	{
		p->oaccess_incs = p->access_incs;
		p->access_incs++;
		if (p->GetActor()->spr.extra <= 0)
			p->access_incs = 12;
		if (p->access_incs == 12)
		{
			if (p->access_spritenum != nullptr)
			{
				fi.checkhitswitch(snum, nullptr, p->access_spritenum);
				switch (p->access_spritenum->spr.pal)
				{
				case 0:p->got_access &= (0xffff - 0x1); break;
				case 21:p->got_access &= (0xffff - 0x2); break;
				case 23:p->got_access &= (0xffff - 0x4); break;
				}
				p->access_spritenum = nullptr;
			}
			else
			{
				fi.checkhitswitch(snum, p->access_wall, nullptr);
				switch (p->access_wall->pal)
				{
				case 0:p->got_access &= (0xffff - 0x1); break;
				case 21:p->got_access &= (0xffff - 0x2); break;
				case 23:p->got_access &= (0xffff - 0x4); break;
				}
			}
		}

		if (p->access_incs > 20)
		{
			p->oaccess_incs = p->access_incs = 0;
			p->oweapon_pos = p->weapon_pos = 10;
			p->okickback_pic = p->kickback_pic = 0;
		}
	}

	if (p->scuba_on == 0 && p->insector() && p->cursector->lotag == 2)
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

void checkweapons_d(player_struct* p)
{
	static const uint16_t weapon_sprites[MAX_WEAPONS] = { KNEE, FIRSTGUNSPRITE, SHOTGUNSPRITE,
			CHAINGUNSPRITE, RPGSPRITE, HEAVYHBOMB, SHRINKERSPRITE, DEVISTATORSPRITE,
			TRIPBOMBSPRITE, FREEZESPRITE, HEAVYHBOMB, SHRINKERSPRITE };

	int cw;

	if (isWW2GI())
	{
		int snum = p->GetActor()->spr.yvel;
		cw = aplWeaponWorksLike(p->curr_weapon, snum);
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
		p->pos.Z -= (p->jetpack_on * 0.5); //Goin up
	}
	else if (p->jetpack_on == 11 && !S_CheckActorSoundPlaying(pact, DUKE_JETPACK_IDLE))
		S_PlayActorSound(DUKE_JETPACK_IDLE, pact);

	double dist;
	if (shrunk) dist = 2;
	else dist = 8;

	if (actions & SB_JUMP)                            //A (soar high)
	{
		// jump
		SetGameVarID(g_iReturnVarID, 0, p->GetActor(), snum);
		OnEvent(EVENT_SOARUP, snum, p->GetActor(), -1);
		if (GetGameVarID(g_iReturnVarID, p->GetActor(), snum).value() == 0)
		{
			p->pos.Z -= dist;
			p->crack_time = CRACK_TIME;
		}
	}

	if (actions & SB_CROUCH)                            //Z (soar low)
	{
		// crouch
		SetGameVarID(g_iReturnVarID, 0, p->GetActor(), snum);
		OnEvent(EVENT_SOARDOWN, snum, p->GetActor(), -1);
		if (GetGameVarID(g_iReturnVarID, p->GetActor(), snum).value() == 0)
		{
			p->pos.Z += dist;
			p->crack_time = CRACK_TIME;
		}
	}

	int k;
	if (shrunk == 0 && (psectlotag == 0 || psectlotag == 2)) k = 32;
	else k = 16;

	if (psectlotag != 2 && p->scuba_on == 1)
		p->scuba_on = 0;

	if (p->player_int_pos().Z > (fz - (k << 8)))
		p->pos.Z += (((fz - (k << 8)) - p->player_int_pos().Z) >> 1) * zinttoworld;
	if (p->pos.Z < pact->ceilingz + 18)
		p->pos.Z = pact->ceilingz + 18;

}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

static void movement(int snum, ESyncBits actions, sectortype* psect, int fz, int cz, int shrunk, int truefdist, int psectlotag)
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

		if (shrunk == 0 && truefdist <= gs.int_playerheight)
		{
			if (p->on_ground == 1)
			{
				if (p->dummyplayersprite == nullptr)
					p->dummyplayersprite = spawn(pact, PLAYERONWATER);

				p->footprintcount = 6;
				if (p->cursector->floorpicnum == FLOORSLIME)
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

	if (p->player_int_pos().Z < (fz - (i << 8))) //falling
	{

		// not jumping or crouching
		if ((actions & (SB_JUMP|SB_CROUCH)) == 0 && p->on_ground && (psect->floorstat & CSTAT_SECTOR_SLOPE) && p->player_int_pos().Z >= (fz - (i << 8) - (16 << 8)))
			p->player_set_int_z(fz - (i << 8));
		else
		{
			p->on_ground = 0;
			p->vel.Z += (gs.gravity + 80); // (TICSPERFRAME<<6);
			if (p->vel.Z >= (4096 + 2048)) p->vel.Z = (4096 + 2048);
			if (p->vel.Z > 2400 && p->falling_counter < 255)
			{
				p->falling_counter++;
				if (p->falling_counter == 38 && !S_CheckActorSoundPlaying(pact, DUKE_SCREAM))
					S_PlayActorSound(DUKE_SCREAM, pact);
			}

			if ((p->player_int_pos().Z + p->vel.Z) >= (fz - (i << 8))) // hit the ground
			{
				S_StopSound(DUKE_SCREAM, pact);
				if (!p->insector() || p->cursector->lotag != 1)
				{
					if (p->falling_counter > 62) quickkill(p);

					else if (p->falling_counter > 9)
					{
						j = p->falling_counter;
						pact->spr.extra -= j - (krand() & 3);
						if (pact->spr.extra <= 0)
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
					else if (p->vel.Z > 2048) S_PlayActorSound(DUKE_LAND, pact);
				}
			}
		}
	}

	else
	{
		p->falling_counter = 0;
		S_StopSound(-1, pact, CHAN_VOICE);

		if (psectlotag != ST_1_ABOVE_WATER && psectlotag != ST_2_UNDERWATER && p->on_ground == 0 && p->vel.Z > (6144 >> 1))
			p->hard_landing = p->vel.Z >> 10;

		p->on_ground = 1;

		if (i == 40)
		{
			//Smooth on the ground

			int k = ((fz - (i << 8)) - p->player_int_pos().Z) >> 1;
			if (abs(k) < 256) k = 0;
			p->player_add_int_z(k);
			p->vel.Z -= 768;
			if (p->vel.Z < 0) p->vel.Z = 0;
		}
		else if (p->jumping_counter == 0)
		{
			p->player_add_int_z(((fz - (i << 7)) - p->player_int_pos().Z) >> 1); //Smooth on the water
			if (p->on_warping_sector == 0 && p->player_int_pos().Z > fz - (16 << 8))
			{
				p->player_set_int_z(fz - (16 << 8));
				p->vel.Z >>= 1;
			}
		}

		p->on_warping_sector = 0;

		if ((actions & SB_CROUCH) || crouch_toggle)	// FIXME: The crouch_toggle check here is not network safe and needs revision when multiplayer is going.
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
				p->vel.Z = -512;
			}
			else
			{
				p->vel.Z -= bsin(2048 - 128 + p->jumping_counter) / 12;
				p->jumping_counter += 180;
				p->on_ground = 0;
			}
		}
		else
		{
			p->jumping_counter = 0;
			p->vel.Z = 0;
		}
	}

	p->player_add_int_z(p->vel.Z);

	if (p->player_int_pos().Z < (cz + (4 << 8)))
	{
		p->jumping_counter = 0;
		if (p->vel.Z < 0)
			p->vel.X = p->vel.Y = 0;
		p->vel.Z = 128;
		p->player_set_int_z(cz + (4 << 8));
	}
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

static void underwater(int snum, ESyncBits actions, int fz, int cz)
{
	auto p = &ps[snum];
	auto pact = p->GetActor();

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
		if (p->vel.Z > 0) p->vel.Z = 0;
		p->vel.Z -= 348;
		if (p->vel.Z < -(256 * 6)) p->vel.Z = -(256 * 6);
	}
	else if (actions & SB_CROUCH)
	{
		// crouch
		if (p->vel.Z < 0) p->vel.Z = 0;
		p->vel.Z += 348;
		if (p->vel.Z > (256 * 6)) p->vel.Z = (256 * 6);
	}
	else
	{
		// normal view
		if (p->vel.Z < 0)
		{
			p->vel.Z += 256;
			if (p->vel.Z > 0)
				p->vel.Z = 0;
		}
		if (p->vel.Z > 0)
		{
			p->vel.Z -= 256;
			if (p->vel.Z < 0)
				p->vel.Z = 0;
		}
	}

	if (p->vel.Z > 2048)
		p->vel.Z >>= 1;

	p->player_add_int_z(p->vel.Z);

	if (p->player_int_pos().Z > (fz - (15 << 8)))
		p->player_add_int_z(((fz - (15 << 8)) - p->player_int_pos().Z) >> 1);

	if (p->player_int_pos().Z < (cz + (4 << 8)))
	{
		p->player_set_int_z(cz + (4 << 8));
		p->vel.Z = 0;
	}

	if (p->scuba_on && (krand() & 255) < 8)
	{
		auto j = spawn(pact, WATERBUBBLE);
		if (j)
		{
			j->add_int_pos({ bcos(p->angle.ang.Buildang() + 64 - (global_random & 128), -6), bsin(p->angle.ang.Buildang() + 64 - (global_random & 128), -6), 0 });
			j->spr.xrepeat = 3;
			j->spr.yrepeat = 2;
			j->set_int_z(p->player_int_pos().Z + (8 << 8));
		}
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
	HitInfo hit{};

	hitscan(p->player_int_pos(), p->cursector, { int(p->angle.ang.Cos() * (1 << 14)), int(p->angle.ang.Sin() * (1 << 14)), -p->horizon.sum().asq16() >> 11 }, hit, CLIPMASK1);

	if (hit.hitSector == nullptr || hit.actor())
		return 0;

	if (hit.hitWall != nullptr && hit.hitSector->lotag > 2)
		return 0;

	if (hit.hitWall != nullptr && hit.hitWall->overpicnum >= 0)
		if (hit.hitWall->overpicnum == BIGFORCE)
			return 0;

	DDukeActor* act;
	DukeSectIterator it(hit.hitSector);
	while ((act = it.Next()))
	{
		if (!actorflag(act, SFLAG_BLOCK_TRIPBOMB) &&
			abs(act->int_pos().Z - hit.int_hitpos().Z) < (12 << 8) && ((act->int_pos().X - hit.int_hitpos().X) * (act->int_pos().X - hit.int_hitpos().X) + (act->int_pos().Y - hit.int_hitpos().Y) * (act->int_pos().Y - hit.int_hitpos().Y)) < (290 * 290))
			return 0;
	}

	if (act == nullptr && hit.hitWall != nullptr && (hit.hitWall->cstat & CSTAT_WALL_MASKED) == 0)
		if ((hit.hitWall->twoSided() && hit.hitWall->nextSector()->lotag <= 2) || (!hit.hitWall->twoSided() && hit.hitSector->lotag <= 2))
			if (((hit.int_hitpos().X - p->player_int_pos().X) * (hit.int_hitpos().X - p->player_int_pos().X) + (hit.int_hitpos().Y - p->player_int_pos().Y) * (hit.int_hitpos().Y - p->player_int_pos().Y)) < (290 * 290))
			{
				p->pos.Z = p->opos.Z;
				p->vel.Z = 0;
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
			if (p->cursector->lotag != 2)
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

static void operateweapon(int snum, ESyncBits actions)
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

			auto spawned = EGS(p->cursector,
				p->player_int_pos().X + p->angle.ang.Cos() * (1 << 8),
				p->player_int_pos().Y + p->angle.ang.Sin() * (1 << 8),
				p->player_int_pos().Z, HEAVYHBOMB, -16, 9, 9,
				p->angle.ang.Buildang(), (k + (p->hbomb_hold_delay << 5)), i, pact, 1);

			if (isNam())
			{
				spawned->spr.extra = MulScale(krand(), NAM_GRENADE_LIFETIME_VAR, 14);
			}

			if (k == 15)
			{
				spawned->spr.yvel = 3;
				spawned->spr.pos.Z += 8;
			}

			k = hits(pact);
			if (k < 512)
			{
				spawned->add_int_ang(1024);
				spawned->spr.zvel /= 3;
				spawned->spr.xvel /= 3;
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
			for(int ii = 0; ii < 7; ii++)
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
			if (j)
			{
				j->add_int_ang(1024);
				ssp(j, CLIPMASK0);
				j->add_int_ang(1024);
			}
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
					if (j)
					{
						j->set_int_ang((j->int_ang() + 1024) & 2047);
						j->spr.xvel += 32;
						j->spr.pos.Z += 3;
						ssp(j, CLIPMASK0);
					}
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
			if (!(aplWeaponFlags(p->curr_weapon, snum) & WEAPON_FLAG_NOVISIBLE))
			{
				// make them visible if not set...
				p->visibility = 0;
				lastvisinc = PlayClock + 32;
			}
			checkavailweapon(p);
			//#endif
		}
		else if (!isNam()) p->kickback_pic++;
		if (isNam() && p->kickback_pic > 30)
		{
			// reload now...
			p->okickback_pic = p->kickback_pic = 0;
			if (!(aplWeaponFlags(p->curr_weapon, snum) & WEAPON_FLAG_NOVISIBLE))
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
			if (pact->spr.xrepeat < 32)
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
			if (p->cursector->lotag != 2) 
			{
				p->ammo_amount[FLAMETHROWER_WEAPON]--;
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
			p->pos.Z = p->opos.Z;
			p->vel.Z = 0;
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

static void processweapon(int snum, ESyncBits actions)
{
	auto p = &ps[snum];
	auto pact = p->GetActor();
	int shrunk = (pact->spr.yrepeat < 32);

	if (isNamWW2GI() && (actions & SB_HOLSTER)) // 'Holster Weapon
	{
		if (isWW2GI())
		{
			SetGameVarID(g_iReturnVarID, 0, p->GetActor(), snum);
			SetGameVarID(g_iWeaponVarID, p->curr_weapon, p->GetActor(), snum);
			SetGameVarID(g_iWorksLikeVarID, aplWeaponWorksLike(p->curr_weapon, snum), p->GetActor(), snum);
			OnEvent(EVENT_HOLSTER, snum, p->GetActor(), -1);
			if (GetGameVarID(g_iReturnVarID, p->GetActor(), snum).value() == 0)
			{
				// now it uses the game definitions...
				if (aplWeaponFlags(p->curr_weapon, snum) & WEAPON_FLAG_HOLSTER_CLEARS_CLIP)
				{
					if (p->ammo_amount[p->curr_weapon] > aplWeaponClip(p->curr_weapon, snum)
						&& (p->ammo_amount[p->curr_weapon] % aplWeaponClip(p->curr_weapon, snum)) != 0)
					{
						// throw away the remaining clip
						p->ammo_amount[p->curr_weapon] -=
							p->ammo_amount[p->curr_weapon] % aplWeaponClip(p->curr_weapon, snum);
						//				p->kickback_pic = aplWeaponFireDelay(p->curr_weapon, snum)+1;	// animate, but don't shoot...
						p->kickback_pic = aplWeaponTotalTime(p->curr_weapon, snum) + 1;	// animate, but don't shoot...
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


	if (isWW2GI() && (aplWeaponFlags(p->curr_weapon, snum) & WEAPON_FLAG_GLOWS))
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
		if (!isWW2GI()) operateweapon(snum, actions);
		else operateweapon_ww(snum, actions);
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
	int psectlotag;
	player_struct* p;

	p = &ps[snum];
	auto pact = p->GetActor();

	p->horizon.resetadjustment();
	p->angle.resetadjustment();

	ESyncBits& actions = p->sync.actions;

	auto sb_fvel = PlayerInputForwardVel(snum);
	auto sb_svel = PlayerInputSideVel(snum);
	auto sb_avel = PlayerInputAngVel(snum);

	auto psectp = p->cursector;
	if (psectp == nullptr)
	{
		if (pact->spr.extra > 0 && ud.clipping == 0)
		{
			quickkill(p);
			S_PlayActorSound(SQUISHED, pact);
		}
		psectp = &sector[0];
	}

	psectlotag = psectp->lotag;
	p->spritebridge = 0;

	shrunk = (pact->spr.yrepeat < 32);
	getzrange(p->player_int_pos(), psectp, &cz, chz, &fz, clz, 163, CLIPMASK0);

	j = getflorzofslopeptr(psectp, p->player_int_pos().X, p->player_int_pos().Y);

	p->truefz = j * zinttoworld;
	p->truecz = getceilzofslopeptr(psectp, p->player_int_pos().X, p->player_int_pos().Y) * zinttoworld;

	truefdist = abs(p->player_int_pos().Z - j);
	if (clz.type == kHitSector && psectlotag == 1 && truefdist > gs.int_playerheight + (16 << 8))
		psectlotag = 0;

	pact->floorz = fz * zinttoworld;
	pact->ceilingz = cz * zinttoworld;

	if (SyncInput())
	{
		p->horizon.backup();
		doslopetilting(p);
	}

	if (chz.type == kHitSprite)
	{
		if (chz.actor()->spr.statnum == 1 && chz.actor()->spr.extra >= 0)
		{
			chz.setNone();
			cz = p->truecz * zworldtoint;
		}
	}

	if (clz.type == kHitSprite)
	{
		if ((clz.actor()->spr.cstat & (CSTAT_SPRITE_ALIGNMENT_FLOOR | CSTAT_SPRITE_BLOCK)) == (CSTAT_SPRITE_ALIGNMENT_FLOOR | CSTAT_SPRITE_BLOCK))
		{
			psectlotag = 0;
			p->footprintcount = 0;
			p->spritebridge = 1;
		}
		else if (badguy(clz.actor()) && clz.actor()->spr.xrepeat > 24 && abs(pact->int_pos().Z - clz.actor()->int_pos().Z) < (84 << 8))
		{
			j = getangle(clz.actor()->int_pos().X - p->player_int_pos().X, clz.actor()->int_pos().Y - p->player_int_pos().Y);
			p->vel.X -= bcos(j, 4);
			p->vel.Y -= bsin(j, 4);
		}
	}


	if (pact->spr.extra > 0) fi.incur_damage(p);
	else
	{
		pact->spr.extra = 0;
		p->shield_amount = 0;
	}

	p->last_extra = pact->spr.extra;

	if (p->loogcnt > 0)
	{
		p->oloogcnt = p->loogcnt;
		p->loogcnt--;
	}
	else
	{
		p->oloogcnt = p->loogcnt = 0;
	}

	if (p->fist_incs)
	{
		if (endoflevel(snum)) return;
	}

	if (p->timebeforeexit > 1 && p->last_extra > 0)
	{
		if (timedexit(snum))
			return;
	}

	if (pact->spr.extra <= 0 && !ud.god)
	{
		playerisdead(snum, psectlotag, fz, cz);
		return;
	}

	if (p->GetActor()->spr.xrepeat < 40 && p->jetpack_on == 0)
	{
		p->ofistsign = p->fistsign;
		p->fistsign += p->GetActor()->spr.xvel;
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
		p->vel.X = p->vel.Y = pact->spr.xvel = 0;

		fi.doincrements(p);

		if (isWW2GI() && aplWeaponWorksLike(p->curr_weapon, snum) == HANDREMOTE_WEAPON) processweapon(snum, actions);
		if (!isWW2GI() && p->curr_weapon == HANDREMOTE_WEAPON) processweapon(snum, actions);
		return;
	}

	doubvel = TICSPERFRAME;

	checklook(snum,actions);
	int ii = 40;
	auto oldpos = p->opos;

	if (p->on_crane != nullptr)
		goto HORIZONLY;

	p->playerweaponsway(pact->spr.xvel);

	pact->spr.xvel = int(clamp((p->pos.XY() - p->bobpos).Length(), 0., 32.) * worldtoint);
	if (p->on_ground) p->bobcounter += p->GetActor()->spr.xvel >> 1;

	p->backuppos(ud.clipping == 0 && ((p->insector() && p->cursector->floorpicnum == MIRROR) || !p->insector()));

	// Shrinking code

	if (psectlotag == ST_2_UNDERWATER)
	{
		underwater(snum, actions, fz, cz);
	}

	else if (p->jetpack_on)
	{
		operateJetpack(snum, actions, psectlotag, fz, cz, shrunk);
	}
	else if (psectlotag != ST_2_UNDERWATER)
	{
		movement(snum, actions, psectp, fz, cz, shrunk, truefdist, psectlotag);
	}

	p->psectlotag = psectlotag;

	//Do the quick lefts and rights

	if (movementBlocked(p))
	{
		doubvel = 0;
		p->vel.X = 0;
		p->vel.Y = 0;
	}
	else if (SyncInput())
	{
		//p->ang += syncangvel * constant
		//ENGINE calculates angvel for you
		// may still be needed later for demo recording

		sb_avel = p->adjustavel(sb_avel);
		p->angle.applyinput(sb_avel, &actions);
	}

	if (p->spritebridge == 0 && pact->insector())
	{
		j = pact->sector()->floorpicnum;

		if (j == PURPLELAVA || pact->sector()->ceilingpicnum == PURPLELAVA)
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
				pact->spr.extra--;
			}
		}

		k = 0;

		if (p->on_ground && truefdist <= gs.int_playerheight + (16 << 8))
		{
			int whichsound = (gs.tileinfo[j].flags & TFLAG_ELECTRIC) ? 0 : j == FLOORSLIME ? 1 : j == FLOORPLASMA ? 2 : -1;
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

	if (p->vel.X || p->vel.Y || sb_fvel || sb_svel)
	{
		p->crack_time = CRACK_TIME;

		k = bsin(p->bobcounter, -12);

		if (truefdist < gs.int_playerheight + (8 << 8) && (k == 1 || k == 3))
		{
			if (p->spritebridge == 0 && p->walking_snd_toggle == 0 && p->on_ground)
			{
				switch (psectlotag)
				{
				case 0:

					if (clz.type == kHitSprite)
						j = clz.actor()->spr.picnum;
					else
						j = psectp->floorpicnum;

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

		p->vel.X += ((sb_fvel * doubvel) << 6);
		p->vel.Y += ((sb_svel * doubvel) << 6);

		bool check;

		if (!isWW2GI()) check = ((p->curr_weapon == KNEE_WEAPON && p->kickback_pic > 10 && p->on_ground) || (p->on_ground && (actions & SB_CROUCH)));
		else check = ((aplWeaponWorksLike(p->curr_weapon, snum) == KNEE_WEAPON && p->kickback_pic > 10 && p->on_ground) || (p->on_ground && (actions & SB_CROUCH)));
		if (check)
		{
			p->vel.X = MulScale(p->vel.X, gs.playerfriction - 0x2000, 16);
			p->vel.Y = MulScale(p->vel.Y, gs.playerfriction - 0x2000, 16);
		}
		else
		{
			if (psectlotag == 2)
			{
				p->vel.X = MulScale(p->vel.X, gs.playerfriction - 0x1400, 16);
				p->vel.Y = MulScale(p->vel.Y, gs.playerfriction - 0x1400, 16);
			}
			else
			{
				p->vel.X = MulScale(p->vel.X, gs.playerfriction, 16);
				p->vel.Y = MulScale(p->vel.Y, gs.playerfriction, 16);
			}
		}

		if (abs(p->vel.X) < 2048 && abs(p->vel.Y) < 2048)
			p->vel.X = p->vel.Y = 0;

		if (shrunk)
		{
			p->vel.X =
				MulScale(p->vel.X, gs.playerfriction - (gs.playerfriction >> 1) + (gs.playerfriction >> 2), 16);
			p->vel.Y =
				MulScale(p->vel.Y, gs.playerfriction - (gs.playerfriction >> 1) + (gs.playerfriction >> 2), 16);
		}
	}

HORIZONLY:

	if (psectlotag == 1 || p->spritebridge == 1) ii = (4L << 8);
	else ii = (20L << 8);

	if (p->insector() && p->cursector->lotag == 2) k = 0;
	else k = 1;

	Collision clip{};
	if (ud.clipping)
	{
		p->player_add_int_xy({ p->vel.X >> 14, p->vel.Y >> 14 });
		updatesector(p->player_int_pos().X, p->player_int_pos().Y, &p->cursector);
		ChangeActorSect(pact, p->cursector);
	}
	else
		clipmove(p->pos, &p->cursector, p->vel.X, p->vel.Y, 164, (4 << 8), ii, CLIPMASK0, clip);

	if (p->jetpack_on == 0 && psectlotag != 2 && psectlotag != 1 && shrunk)
		p->pos.Z += 32;

	if (clip.type != kHitNone)
		checkplayerhurt_d(p, clip);

	if (p->jetpack_on == 0)
	{
		if (pact->spr.xvel > 16)
		{
			if (psectlotag != 1 && psectlotag != 2 && p->on_ground)
			{
				p->pycount += 52;
				p->pycount &= 2047;
				p->pyoff = abs(pact->spr.xvel * bsin(p->pycount)) / 1596;
			}
		}
		else if (psectlotag != 2 && psectlotag != 1)
			p->pyoff = 0;
	}

	// RBG***
	SetActor(pact, vec3_t( p->player_int_pos().X, p->player_int_pos().Y, p->player_int_pos().Z + gs.int_playerheight ));

	if (psectlotag < 3)
	{
		psectp = pact->sector();
		if (ud.clipping == 0 && psectp->lotag == 31)
		{
			auto secact = barrier_cast<DDukeActor*>(psectp->hitagactor);
			if (secact && secact->spr.xvel && secact->temp_data[0] == 0)
			{
				quickkill(p);
				return;
			}
		}
	}

	if (truefdist < gs.int_playerheight && p->on_ground && psectlotag != 1 && shrunk == 0 && p->insector() && p->cursector->lotag == 1)
		if (!S_CheckActorSoundPlaying(pact, DUKE_ONWATER))
			S_PlayActorSound(DUKE_ONWATER, pact);

	if (p->cursector != pact->sector())
		ChangeActorSect(pact, p->cursector);

	int retry = 0;
	while (ud.clipping == 0)
	{
		int blocked;
		blocked = (pushmove(p->pos, &p->cursector, 164, (4 << 8), (4 << 8), CLIPMASK0) < 0 && furthestangle(p->GetActor(), 8) < 512);

		if (fabs(pact->floorz - pact->ceilingz) < 48 || blocked)
		{
			if (!(pact->sector()->lotag & 0x8000) && (isanunderoperator(pact->sector()->lotag) ||
				isanearoperator(pact->sector()->lotag)))
				fi.activatebysector(pact->sector(), pact);
			if (blocked)
			{
				if (!retry++)
				{
					p->pos = p->opos = oldpos;
					continue;
				}
				quickkill(p);
				return;
			}
		}
		else if (abs(fz - cz) < (32 << 8) && isanunderoperator(psectp->lotag))
			fi.activatebysector(psectp, pact);
		break;
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
	processweapon(snum, actions);
}

END_DUKE_NS
