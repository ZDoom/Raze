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
#include "dukeactor.h"

BEGIN_DUKE_NS 


//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void incur_damage_r(struct player_struct* p)
{
	int  damage = 0, shield_damage = 0;
	int gut = 0;

	p->GetActor()->spr.extra -= p->extra_extra8 >> 8;

	damage = p->GetActor()->spr.extra - p->last_extra;
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
		case 1:
			damage = damage * 3 / 4;
			break;
		case 2:
			damage /= 4;
			break;
		}

		p->GetActor()->spr.extra = p->last_extra + damage;
	}
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

static void shootmelee(DDukeActor *actor, int p, int sx, int sy, int sz, int sa, int atwith)
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
		auto pspr = ps[findplayer(actor, &x)].GetActor();
		zvel = ((pspr->spr.pos.Z - sz) << 8) / (x + 1);
		sa = getangle(pspr->spr.pos.X - sx, pspr->spr.pos.Y - sy);
	}

	hitscan({ sx, sy, sz }, sectp, { bcos(sa), bsin(sa), zvel << 6 }, hit, CLIPMASK1);

	if (isRRRA() && hit.hitSector != nullptr && ((hit.hitSector->lotag == 160 && zvel > 0) || (hit.hitSector->lotag == 161 && zvel < 0))
		&& hit.actor() == nullptr && hit.hitWall == nullptr)
	{
		DukeStatIterator its(STAT_EFFECTOR);
		while (auto effector = its.Next())
		{
			if (effector->sector() == hit.hitSector && effector->spr.picnum == SECTOREFFECTOR && effector->GetOwner()
				&& effector->spr.lotag == SE_7_TELEPORT)
			{
				int nx, ny, nz;
				nx = hit.hitpos.X + (effector->GetOwner()->spr.pos.X - effector->spr.pos.X);
				ny = hit.hitpos.Y + (effector->GetOwner()->spr.pos.Y - effector->spr.pos.Y);
				if (hit.hitSector->lotag == 161)
				{
					nz = effector->GetOwner()->sector()->floorz;
				}
				else
				{
					nz = effector->GetOwner()->sector()->ceilingz;
				}
				hitscan({ nx, ny, nz }, effector->GetOwner()->sector(), { bcos(sa), bsin(sa), zvel << 6 }, hit, CLIPMASK1);
				break;
			}
		}
	}

	if (hit.hitSector == nullptr) return;

	if ((abs(sx - hit.hitpos.X) + abs(sy - hit.hitpos.Y)) < 1024)
	{
		if (hit.hitWall != nullptr || hit.actor())
		{
			DDukeActor* wpn;
			if (isRRRA() && atwith == SLINGBLADE)
			{
				wpn = EGS(hit.hitSector, hit.hitpos.X, hit.hitpos.Y, hit.hitpos.Z, SLINGBLADE, -15, 0, 0, sa, 32, 0, actor, 4);
				if (!wpn) return;
				wpn->spr.extra += 50;
			}
			else
			{
				wpn = EGS(hit.hitSector, hit.hitpos.X, hit.hitpos.Y, hit.hitpos.Z, KNEE, -15, 0, 0, sa, 32, 0, actor, 4);
				if (!wpn) return;
				wpn->spr.extra += (krand() & 7);
			}
			if (p >= 0)
			{
				auto k = spawn(wpn, SMALLSMOKE);
				if (k) k->spr.pos.Z -= (8 << 8);
				if (atwith == KNEE) S_PlayActorSound(KICK_HIT, wpn);
				else if (isRRRA() && atwith == SLINGBLADE)	S_PlayActorSound(260, wpn);
			}

			if (p >= 0 && ps[p].steroids_amount > 0 && ps[p].steroids_amount < 400)
				wpn->spr.extra += (gs.max_player_health >> 2);

			if (hit.actor() && hit.actor()->spr.picnum != ACCESSSWITCH && hit.actor()->spr.picnum != ACCESSSWITCH2)
			{
				fi.checkhitsprite(hit.actor(), wpn);
				if (p >= 0) fi.checkhitswitch(p, nullptr, hit.actor());
			}
			else if (hit.hitWall)
			{
				if (hit.hitWall->cstat & CSTAT_WALL_BOTTOM_SWAP)
					if (hit.hitWall->twoSided())
						if (hit.hitpos.Z >= (hit.hitWall->nextSector()->floorz))
							hit.hitWall = hit.hitWall->nextWall();

				if (hit.hitWall->picnum != ACCESSSWITCH && hit.hitWall->picnum != ACCESSSWITCH2)
				{
					fi.checkhitwall(wpn, hit.hitWall, hit.hitpos.X, hit.hitpos.Y, hit.hitpos.Z, atwith);
					if (p >= 0) fi.checkhitswitch(p, hit.hitWall, nullptr);
				}
			}
		}
		else if (p >= 0 && zvel > 0 && hit.hitSector->lotag == 1)
		{
			auto splash = spawn(ps[p].GetActor(), WATERSPLASH2);
			if (splash)
			{
				splash->spr.pos.X = hit.hitpos.X;
				splash->spr.pos.Y = hit.hitpos.Y;
				splash->spr.ang = ps[p].angle.ang.asbuild(); // Total tweek
				splash->spr.xvel = 32;
				ssp(actor, 0);
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

static void shootweapon(DDukeActor* actor, int p, int sx, int sy, int sz, int sa, int atwith)
{
	auto sectp = actor->sector();
	int zvel = 0;
	HitInfo hit{};

	if (actor->spr.extra >= 0) actor->spr.shade = -96;

	if (p >= 0)
	{
		auto aimed = aim(actor, AUTO_AIM_ANGLE);
		if (aimed)
		{
			int dal = ((aimed->spr.xrepeat * tileHeight(aimed->spr.picnum)) << 1) + (5 << 8);
			zvel = ((aimed->spr.pos.Z - sz - dal) << 8) / ldist(ps[p].GetActor(), aimed);
			sa = getangle(aimed->spr.pos.X - sx, aimed->spr.pos.Y - sy);
		}

		if (atwith == SHOTSPARK1)
		{
			if (aimed == nullptr)
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
			if (aimed == nullptr) zvel = -ps[p].horizon.sum().asq16() >> 11;
			zvel += 128 - (krand() & 255);
		}
		sz -= (2 << 8);
	}
	else
	{
		int x;
		int j = findplayer(actor, &x);
		sz -= (4 << 8);
		zvel = ((ps[j].pos.Z - sz) << 8) / (ldist(ps[j].GetActor(), actor));
		if (actor->spr.picnum != BOSS1)
		{
			zvel += 128 - (krand() & 255);
			sa += 32 - (krand() & 63);
		}
		else
		{
			zvel += 128 - (krand() & 255);
			sa = getangle(ps[j].pos.X - sx, ps[j].pos.Y - sy) + 64 - (krand() & 127);
		}
	}

	actor->spr.cstat &= ~CSTAT_SPRITE_BLOCK_ALL;
	hitscan({ sx, sy, sz }, sectp, { bcos(sa), bsin(sa),zvel << 6 }, hit, CLIPMASK1);

	if (isRRRA() && hit.hitSector != nullptr && (((hit.hitSector->lotag == 160 && zvel > 0) || (hit.hitSector->lotag == 161 && zvel < 0))
		&& hit.actor() == nullptr && hit.hitWall == nullptr))
	{
		DukeStatIterator its(STAT_EFFECTOR);
		while (auto effector = its.Next())
		{
			if (effector->sector() == hit.hitSector && effector->spr.picnum == SECTOREFFECTOR && effector->GetOwner()
				&& effector->spr.lotag == SE_7_TELEPORT)
			{
				int nx, ny, nz;
				nx = hit.hitpos.X + (effector->GetOwner()->spr.pos.X - effector->spr.pos.X);
				ny = hit.hitpos.Y + (effector->GetOwner()->spr.pos.Y - effector->spr.pos.Y);
				if (hit.hitSector->lotag == 161)
				{
					nz = effector->GetOwner()->sector()->floorz;
				}
				else
				{
					nz = effector->GetOwner()->sector()->ceilingz;
				}
				hitscan({ nx, ny, nz }, effector->GetOwner()->sector(), { bcos(sa), bsin(sa), zvel << 6 }, hit, CLIPMASK1);
				break;
			}
		}
	}

	actor->spr.cstat |= CSTAT_SPRITE_BLOCK_ALL;

	if (hit.hitSector == nullptr) return;

	if (atwith == SHOTGUN)
		if (hit.hitSector->lotag == 1)
			if (krand() & 1)
				return;

	if ((krand() & 15) == 0 && hit.hitSector->lotag == 2)
		tracers(hit.hitpos.X, hit.hitpos.Y, hit.hitpos.Z, sx, sy, sz, 8 - (ud.multimode >> 1));

	DDukeActor* spark;
	if (p >= 0)
	{
		spark = EGS(hit.hitSector, hit.hitpos.X, hit.hitpos.Y, hit.hitpos.Z, SHOTSPARK1, -15, 10, 10, sa, 0, 0, actor, 4);
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
			if (hit.hitSector->lotag != 1)
				spawn(spark, SMALLSMOKE);
		}

		if (hit.actor())
		{
			if (hit.actor()->spr.picnum == 1930)
				return;
			fi.checkhitsprite(hit.actor(), spark);
			if (hit.actor()->isPlayer() && (ud.coop != 1 || ud.ffire == 1))
			{
				auto l = spawn(spark, JIBS6);
				spark->spr.xrepeat = spark->spr.yrepeat = 0;
				if (l)
				{
					l->spr.pos.Z += (4 << 8);
					l->spr.xvel = 16;
					l->spr.xrepeat = l->spr.yrepeat = 24;
					l->spr.ang += 64 - (krand() & 127);
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
				(isRRRA() && hit.actor()->spr.picnum == RRTILE8660) ||
				hit.actor()->spr.picnum == HANDSWITCH ||
				hit.actor()->spr.picnum == HANDSWITCH + 1))
			{
				fi.checkhitswitch(p, nullptr, hit.actor());
				return;
			}
		}
		else if (hit.hitWall != nullptr)
		{
			spawn(spark, SMALLSMOKE);

			if (fi.isadoorwall(hit.hitWall->picnum) == 1)
				goto SKIPBULLETHOLE;
			if (isablockdoor(hit.hitWall->picnum) == 1)
				goto SKIPBULLETHOLE;
			if (p >= 0 && (
				hit.hitWall->picnum == DIPSWITCH ||
				hit.hitWall->picnum == DIPSWITCH + 1 ||
				hit.hitWall->picnum == DIPSWITCH2 ||
				hit.hitWall->picnum == DIPSWITCH2 + 1 ||
				hit.hitWall->picnum == DIPSWITCH3 ||
				hit.hitWall->picnum == DIPSWITCH3 + 1 ||
				(isRRRA() && hit.hitWall->picnum == RRTILE8660) ||
				hit.hitWall->picnum == HANDSWITCH ||
				hit.hitWall->picnum == HANDSWITCH + 1))
			{
				fi.checkhitswitch(p, hit.hitWall, nullptr);
				return;
			}

			if (hit.hitWall->hitag != 0 || (hit.hitWall->twoSided() && hit.hitWall->nextWall()->hitag != 0))
				goto SKIPBULLETHOLE;

			if (hit.hitSector != nullptr && hit.hitSector->lotag == 0)
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
								hole->spr.ang = getangle(-delta.X, -delta.Y) + 512;
								ssp(hole, CLIPMASK0);
								hole->spr.cstat2 |= CSTAT2_SPRITE_DECAL;
							}
						}

		SKIPBULLETHOLE:

			if (hit.hitWall->cstat & CSTAT_WALL_BOTTOM_SWAP)
				if (hit.hitWall->twoSided())
					if (hit.hitpos.Z >= (hit.hitWall->nextSector()->floorz))
						hit.hitWall = hit.hitWall->nextWall();

			fi.checkhitwall(spark, hit.hitWall, hit.hitpos.X, hit.hitpos.Y, hit.hitpos.Z, SHOTSPARK1);
		}
	}
	else
	{
		spark = EGS(hit.hitSector, hit.hitpos.X, hit.hitpos.Y, hit.hitpos.Z, SHOTSPARK1, -15, 24, 24, sa, 0, 0, actor, 4);
		if (!spark) return;
		spark->spr.extra = ScriptCode[gs.actorinfo[atwith].scriptaddress];

		if (hit.actor())
		{
			fi.checkhitsprite(hit.actor(), spark);
			if (!hit.actor()->isPlayer())
				spawn(spark, SMALLSMOKE);
			else spark->spr.xrepeat = spark->spr.yrepeat = 0;
		}
		else if (hit.hitWall != nullptr)
			fi.checkhitwall(spark, hit.hitWall, hit.hitpos.X, hit.hitpos.Y, hit.hitpos.Z, SHOTSPARK1);
	}

	if ((krand() & 255) < 10)
	{
		vec3_t v{ hit.hitpos.X, hit.hitpos.Y, hit.hitpos.Z };
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
	auto sect = actor->sector();
	int vel = 0, zvel;
	int scount;

	if (isRRRA())
	{
		if (atwith != SPIT && actor->spr.extra >= 0) actor->spr.shade = -96;

		scount = 1;
		if (atwith == SPIT)
		{
			if (actor->spr.picnum == 8705)
				vel = 600;
			else
				vel = 400;
		}
	}
	else
	{
		if (actor->spr.extra >= 0) actor->spr.shade = -96;

		scount = 1;
		if (atwith == SPIT) vel = 400;
	}
	if (atwith != SPIT)
	{
		vel = 840;
		sz -= (4 << 7);
		if (actor->spr.picnum == 4649)
		{
			sx += bcos(actor->spr.ang + 256, -6);
			sy += bsin(actor->spr.ang + 256, -6);
			sz += (12 << 8);
		}
		if (actor->spr.picnum == VIXEN)
		{
			sz -= (12 << 8);
		}
	}

	if (p >= 0)
	{
		auto aimed = aim(actor, AUTO_AIM_ANGLE);

		sx += bcos(actor->spr.ang + 160, -7);
		sy += bsin(actor->spr.ang + 160, -7);

		if (aimed)
		{
			int dal = ((aimed->spr.xrepeat * tileHeight(aimed->spr.picnum)) << 1) - (12 << 8);
			zvel = ((aimed->spr.pos.Z - sz - dal) * vel) / ldist(ps[p].GetActor(), aimed);
			sa = getangle(aimed->spr.pos.X - sx, aimed->spr.pos.Y - sy);
		}
		else
		{
			zvel = -MulScale(ps[p].horizon.sum().asq16(), 98, 16);
		}
	}
	else
	{
		int x;
		int j = findplayer(actor, &x);
		// sa = getangle(ps[j].oposx-sx,ps[j].oposy-sy);
		if (actor->spr.picnum == HULK)
			sa -= (krand() & 31);
		else if (actor->spr.picnum == VIXEN)
			sa -= (krand() & 16);
		else if (actor->spr.picnum != UFOBEAM)
			sa += 16 - (krand() & 31);

		zvel = (((ps[j].opos.Z - sz + (3 << 8))) * vel) / ldist(ps[j].GetActor(), actor);
	}

	int oldzvel = zvel;
	int sizx, sizy;

	if (atwith == SPIT)
	{
		sizx = 18; sizy = 18;
		if (!isRRRA() || actor->spr.picnum != MAMA) sz -= (10 << 8); else sz -= (20 << 8);
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
		auto j = EGS(sect, sx, sy, sz, atwith, -127, sizx, sizy, sa, vel, zvel, actor, 4);
		if (!j) return;
		j->spr.extra += (krand() & 7);
		j->spr.cstat = CSTAT_SPRITE_YCENTER;
		j->spr.clipdist = 4;

		sa = actor->spr.ang + 32 - (krand() & 63);
		zvel = oldzvel + 512 - (krand() & 1023);

		if (atwith == FIRELASER)
		{
			j->spr.xrepeat = 8;
			j->spr.yrepeat = 8;
		}

		scount--;
	}
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

static void shootrpg(DDukeActor* actor, int p, int sx, int sy, int sz, int sa, int atwith)
{
	auto sect = actor->sector();
	int vel, zvel;
	int l, scount;

	DDukeActor* act90 = nullptr;
	if (actor->spr.extra >= 0) actor->spr.shade = -96;

	scount = 1;
	vel = 644;

	DDukeActor* aimed = nullptr;

	if (p >= 0)
	{
		aimed = aim(actor, 48);
		if (aimed)
		{
			if (isRRRA() && atwith == RPG2)
			{
				if (aimed->spr.picnum == HEN || aimed->spr.picnum == HENSTAYPUT)
					act90 = ps[screenpeek].GetActor();
				else
					act90 = aimed;
			}
			int dal = ((aimed->spr.xrepeat * tileHeight(aimed->spr.picnum)) << 1) + (8 << 8);
			zvel = ((aimed->spr.pos.Z - sz - dal) * vel) / ldist(ps[p].GetActor(), aimed);
			if (aimed->spr.picnum != RECON)
				sa = getangle(aimed->spr.pos.X - sx, aimed->spr.pos.Y - sy);
		}
		else zvel = -MulScale(ps[p].horizon.sum().asq16(), 81, 16);
		if (atwith == RPG)
			S_PlayActorSound(RPG_SHOOT, actor);
		else if (isRRRA())
		{
			if (atwith == RPG2)
				S_PlayActorSound(244, actor);
			else if (atwith == RRTILE1790)
				S_PlayActorSound(94, actor);
		}

	}
	else
	{
		int x;
		int j = findplayer(actor, &x);
		sa = getangle(ps[j].opos.X - sx, ps[j].opos.Y - sy);
		if (actor->spr.picnum == BOSS3)
			sz -= (32 << 8);
		else if (actor->spr.picnum == BOSS2)
		{
			vel += 128;
			sz += 24 << 8;
		}

		l = ldist(ps[j].GetActor(), actor);
		zvel = ((ps[j].opos.Z - sz) * vel) / l;

		if (badguy(actor) && (actor->spr.hitag & face_player_smart))
			sa = actor->spr.ang + (krand() & 31) - 16;
	}

	if (p < 0) aimed = nullptr;

	if (isRRRA() && atwith == RRTILE1790)
	{
		zvel = -(10 << 8);
		vel <<= 1;
	}

	auto spawned = EGS(sect,
		sx + (bcos(sa + 348) / 448),
		sy + (bsin(sa + 348) / 448),
		sz - (1 << 8), atwith, 0, 14, 14, sa, vel, zvel, actor, 4);

	if (!spawned) return;
	if (isRRRA())
	{
		if (atwith == RRTILE1790)
		{
			spawned->spr.extra = 10;
			spawned->spr.zvel = -(10 << 8);
		}
		else if (atwith == RPG2)
		{
			spawned->seek_actor = act90;
			spawned->spr.hitag = 0;
			fi.lotsofmoney(spawned, (krand() & 3) + 1);
		}
	}

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
		if (actor->spr.picnum == HULK)
		{
			spawned->spr.xrepeat = 8;
			spawned->spr.yrepeat = 8;
		}
		else if (atwith != FREEZEBLAST)
		{
			spawned->spr.xrepeat = 30;
			spawned->spr.yrepeat = 30;
			spawned->spr.extra >>= 2;
		}
	}
	else if (ps[p].curr_weapon == TIT_WEAPON)
	{
		spawned->spr.extra >>= 2;
		spawned->spr.ang += 16 - (krand() & 31);
		spawned->spr.zvel += 256 - (krand() & 511);

		if (ps[p].hbomb_hold_delay)
		{
			spawned->spr.pos.X -= bsin(sa) / 644;
			spawned->spr.pos.Y += bcos(sa) / 644;
		}
		else
		{
			spawned->spr.pos.X += bsin(sa, -8);
			spawned->spr.pos.Y -= bcos(sa, -8);
		}
		spawned->spr.xrepeat >>= 1;
		spawned->spr.yrepeat >>= 1;
	}

	spawned->spr.cstat = CSTAT_SPRITE_YCENTER;
	if (atwith == RPG || (atwith == RPG2 && isRRRA()))
		spawned->spr.clipdist = 4;
	else
		spawned->spr.clipdist = 40;


}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

static void shootwhip(DDukeActor* actor, int p, int sx, int sy, int sz, int sa, int atwith)
{
	auto sect = actor->sector();
	int vel = 0, zvel;
	int scount;

	if (actor->spr.extra >= 0) actor->spr.shade = -96;

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
		auto aimed = aim(actor, AUTO_AIM_ANGLE);

		if (aimed)
		{
			int dal = ((aimed->spr.xrepeat * tileHeight(aimed->spr.picnum)) << 1) - (12 << 8);
			zvel = ((aimed->spr.pos.Z - sz - dal) * vel) / ldist(ps[p].GetActor(), aimed);
			sa = getangle(aimed->spr.pos.X - sx, aimed->spr.pos.Y - sy);
		}
		else
			zvel = -MulScale(ps[p].horizon.sum().asq16(), 98, 16);
	}
	else
	{
		int x;
		int j = findplayer(actor, &x);
		//                sa = getangle(ps[j].oposx-sx,ps[j].oposy-sy);
		if (actor->spr.picnum == VIXEN)
			sa -= (krand() & 16);
		else
			sa += 16 - (krand() & 31);
		zvel = (((ps[j].opos.Z - sz + (3 << 8))) * vel) / ldist(ps[j].GetActor(), actor);
	}

	int oldzvel = zvel;
	int sizx = 18; 
	int sizy = 18;

	if (p >= 0) sizx = 7, sizy = 7;
	else sizx = 8, sizy = 8;

	while (scount > 0)
	{
		auto j = EGS(sect, sx, sy, sz, atwith, -127, sizx, sizy, sa, vel, zvel, actor, 4);
		if (!j) return;
		j->spr.extra += (krand() & 7);
		j->spr.cstat = CSTAT_SPRITE_YCENTER;
		j->spr.clipdist = 4;

		sa = actor->spr.ang + 32 - (krand() & 63);
		zvel = oldzvel + 512 - (krand() & 1023);

		scount--;
	}
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void shoot_r(DDukeActor* actor, int atwith)
{
	int sa, p;
	int sx, sy, sz, vel, zvel, x;


	auto const sect = actor->sector();
	zvel = 0;

	if (actor->isPlayer())
	{
		p = actor->spr.yvel;

		sx = ps[p].pos.X;
		sy = ps[p].pos.Y;
		sz = ps[p].pos.Z + ps[p].pyoff + (4 << 8);
		sa = ps[p].angle.ang.asbuild();

		if (isRRRA()) ps[p].crack_time = CRACK_TIME;
	}
	else
	{
		p = -1;
		sa = actor->spr.ang;
		sx = actor->spr.pos.X;
		sy = actor->spr.pos.Y;
		sz = actor->spr.pos.Z - ((actor->spr.yrepeat * tileHeight(actor->spr.picnum)) << 1) + (4 << 8);
		sz -= (7 << 8);
		if (badguy(actor))
		{
			sx -= bsin(sa, -7);
			sy += bcos(sa, -7);
		}
	}

	SetGameVarID(g_iAtWithVarID, atwith, actor, p);
	SetGameVarID(g_iReturnVarID, 0, actor, p);
	OnEvent(EVENT_SHOOT, p, ps[p].GetActor(), -1);
	if (GetGameVarID(g_iReturnVarID, actor, p).safeValue() != 0)
	{
		return;
	}

	switch (atwith)
	{
	case BLOODSPLAT1:
	case BLOODSPLAT2:
	case BLOODSPLAT3:
	case BLOODSPLAT4:
		shootbloodsplat(actor, p, sx, sy, sz, sa, atwith, BIGFORCE, OOZFILTER, -1);
		return;

	case SLINGBLADE:
		if (!isRRRA()) break;
		[[fallthrough]];
	case KNEE:
	case GROWSPARK:
		shootmelee(actor, p, sx, sy, sz, sa, atwith);
		return;

	case SHOTSPARK1:
	case SHOTGUN:
	case CHAINGUN:
		shootweapon(actor, p, sx, sy, sz, sa, atwith);
		return;

	case POWDERKEG:
	{
		auto j = spawn(actor, atwith);
		if (j)
		{
			j->spr.xvel = 32;
			j->spr.ang = actor->spr.ang;
			j->spr.pos.Z -= (5 << 8);
		}
		break;
	}
	case BOWLINGBALL:
	{
		auto j = spawn(actor, atwith);
		if (j)
		{
			j->spr.xvel = 250;
			j->spr.ang = actor->spr.ang;
			j->spr.pos.Z -= (15 << 8);
		}
		break;
	}
	case OWHIP:
	case UWHIP:
		shootwhip(actor, p, sx, sy, sz, sa, atwith);
		return;

	case FIRELASER:
	case SPIT:
	case COOLEXPLOSION1:
		shootstuff(actor, p, sx, sy, sz, sa, atwith);
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
		shootrpg(actor, p, sx, sy, sz, sa, atwith);
		break;

	case CHEERBOMB:
		if (!isRRRA()) break;
	case MORTER:
	{
		if (actor->spr.extra >= 0) actor->spr.shade = -96;

		auto j = ps[findplayer(actor, &x)].GetActor();
		x = ldist(j, actor);

		zvel = -x >> 1;

		if (zvel < -4096)
			zvel = -2048;
		vel = x >> 4;

		if (atwith == CHEERBOMB)
			EGS(sect,
				sx - bsin(sa + 512, -8),
				sy + bcos(sa + 512, -8),
				sz + (6 << 8), atwith, -64, 16, 16, sa, vel, zvel, actor, 1);
		else
			EGS(sect,
				sx - bsin(sa + 512, -8),
				sy + bcos(sa + 512, -8),
				sz + (6 << 8), atwith, -64, 32, 32, sa, vel, zvel, actor, 1);
		break;
	}
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
	if (p->last_pissed_time <= (26 * 218) && p->show_empty_weapon == 0 && p->kickback_pic == 0 && p->quick_kick == 0 && p->GetActor()->spr.xrepeat > 8 && p->access_incs == 0 && p->knee_incs == 0)
	{
		if ((p->weapon_pos == 0 || (p->holster_weapon && p->weapon_pos == -9)))
		{
			if (weap == WeaponSel_Alt)
			{
				j = p->curr_weapon;
				switch (p->curr_weapon)
				{
					case THROWSAW_WEAPON:
						if (p->ammo_amount[BUZZSAW_WEAPON] > 0)
						{
							j = BUZZSAW_WEAPON;
							p->subweapon = 1 << BUZZSAW_WEAPON;
						}
						break;
					case BUZZSAW_WEAPON:
						if (p->ammo_amount[THROWSAW_WEAPON] > 0)
						{
							j = THROWSAW_WEAPON;
							p->subweapon = 0;
						}
						break;
					case POWDERKEG_WEAPON:
						if (p->ammo_amount[BOWLING_WEAPON] > 0)
						{
							j = BOWLING_WEAPON;
							p->subweapon = 1 << BOWLING_WEAPON;
						}
						break;
					case BOWLING_WEAPON:
						if (p->ammo_amount[POWDERKEG_WEAPON] > 0)
						{
							j = POWDERKEG_WEAPON;
							p->subweapon = 0;
						}
						break;
					case KNEE_WEAPON:
						if (isRRRA())
						{
							j = SLINGBLADE_WEAPON;
							p->subweapon = 2;
						}
						break;
					case SLINGBLADE_WEAPON:
						j = KNEE_WEAPON;
						p->subweapon = 0;
						break;
					case CROSSBOW_WEAPON:
						if (p->ammo_amount[CHICKEN_WEAPON] > 0 && isRRRA())
						{
							j = CHICKEN_WEAPON;
							p->subweapon = 4;
						}
						break;
					case CHICKEN_WEAPON:
						if (p->ammo_amount[CROSSBOW_WEAPON] > 0)
						{
							j = CROSSBOW_WEAPON;
							p->subweapon = 0;
						}
						break;
					default:
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
				DukeStatIterator it(STAT_ACTOR);
				while (auto act = it.Next())
				{
					if (act->spr.picnum == HEAVYHBOMB && act->GetOwner() == p->GetActor())
					{
						p->gotweapon[DYNAMITE_WEAPON] = true;
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
					S_PlayActorSound(496, ps[screenpeek].GetActor());
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
				[[fallthrough]];
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
	auto pact = p->GetActor();

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
			if (BellTime == 0 && BellSprite)
				BellSprite->spr.picnum++;
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

	snum = p->GetActor()->spr.yvel;

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
		if (!S_CheckActorSoundPlaying(pact, 420))
			S_PlayActorSound(420, pact);
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
			S_PlayActorSound(404, pact);
			break;
		case 1:
			S_PlayActorSound(422, pact);
			break;
		case 2:
			S_PlayActorSound(423, pact);
			break;
		case 3:
			S_PlayActorSound(424, pact);
			break;
		}
		if (numplayers < 2)
		{
			p->noise_radius = 16384;
			madenoise(screenpeek);
			p->vel.X += p->angle.ang.bcos(4);
			p->vel.Y += p->angle.ang.bsin(4);
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
				S_PlayActorSound(434, pact);
			else if (p->last_pissed_time == 5567)
				S_PlayActorSound(434, pact);
			else if (p->last_pissed_time == 5472)
				S_PlayActorSound(433, pact);
			else if (p->last_pissed_time == 5072)
				S_PlayActorSound(435, pact);
			else if (p->last_pissed_time == 5014)
				S_PlayActorSound(434, pact);
			else if (p->last_pissed_time == 4919)
				S_PlayActorSound(433, pact);
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
				S_PlayActorSound(DUKE_TAKEPILLS, pact);
	}

	if (p->access_incs && p->GetActor()->spr.pal != 1)
	{
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
				case 0:p->keys[1] = 1; break;
				case 21:p->keys[2] = 1; break;
				case 23:p->keys[3] = 1; break;
				}
				p->access_spritenum = nullptr;
			}
			else
			{
				fi.checkhitswitch(snum, p->access_wall, nullptr);
				switch (p->access_wall->pal)
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
		if (p->knuckle_incs == 10)
		{
			if (!wupass)
			{
				int snd = currentLevel->rr_startsound ? currentLevel->rr_startsound : 391;
				wupass = 1;
				S_PlayActorSound(snd, pact);
			}
			else if (PlayClock > 1024)
				if (snum == screenpeek || ud.coop == 1)
				{
					if (rand() & 1)
						S_PlayActorSound(DUKE_CRACK, pact);
					else S_PlayActorSound(DUKE_CRACK2, pact);
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
	static const uint16_t weapon_sprites[MAX_WEAPONS] = { KNEE, FIRSTGUNSPRITE, SHOTGUNSPRITE,
			CHAINGUNSPRITE, RPGSPRITE, HEAVYHBOMB, SHRINKERSPRITE, DEVISTATORSPRITE,
			POWDERKEG, BOWLINGBALLSPRITE, FREEZEBLAST, HEAVYHBOMB };

	if (isRRRA())
	{
		if (p->OnMotorcycle && numplayers > 1)
		{
			auto j = spawn(p->GetActor(), 7220);
			if (j)
			{
				j->spr.ang = p->angle.ang.asbuild();
				j->saved_ammo = p->ammo_amount[MOTORCYCLE_WEAPON];
			}
			p->OnMotorcycle = 0;
			p->gotweapon[MOTORCYCLE_WEAPON] = false;
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
			auto j = spawn(p->GetActor(), 7233);
			if (j)
			{
				j->spr.ang = p->angle.ang.asbuild();
				j->saved_ammo = p->ammo_amount[BOAT_WEAPON];
			}
			p->OnBoat = 0;
			p->gotweapon[BOAT_WEAPON] = false;
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
			spawn(p->GetActor(), weapon_sprites[p->curr_weapon]);
		else switch (p->curr_weapon)
		{
		case CHICKEN_WEAPON:
			if (!isRRRA()) break;
			[[fallthrough]];
		case DYNAMITE_WEAPON:
		case CROSSBOW_WEAPON:
			spawn(p->GetActor(), EXPLOSION2);
			break;
		}
	}

	for (int i = 0; i < 5; i++)
	{
		if (p->keys[i] == 1)
		{
			auto j = spawn(p->GetActor(), ACCESSCARD);
			if (j) switch (i)
			{
			case 1:
				j->spr.lotag = 100;
				break;
			case 2:
				j->spr.lotag = 101;
				break;
			case 3:
				j->spr.lotag = 102;
				break;
			case 4:
				j->spr.lotag = 103;
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
	auto pact = p->GetActor();

	int rng;

	if (p->MotoSpeed < 0)
		p->MotoSpeed = 0;

	if (p->vehForwardScale != 0)
	{
		if (p->on_ground)
		{
			if (p->MotoSpeed == 0 && p->vehBraking)
			{
				if (!S_CheckActorSoundPlaying(pact, 187))
					S_PlayActorSound(187, pact);
			}
			else if (p->MotoSpeed == 0 && !S_CheckActorSoundPlaying(pact, 214))
			{
				if (S_CheckActorSoundPlaying(pact, 187))
					S_StopSound(187, pact);
				S_PlayActorSound(214, pact);
			}
			else if (p->MotoSpeed >= 50 && !S_CheckActorSoundPlaying(pact, 188))
			{
				S_PlayActorSound(188, pact);
			}
			else if (!S_CheckActorSoundPlaying(pact, 188) && !S_CheckActorSoundPlaying(pact, 214))
			{
				S_PlayActorSound(188, pact);
			}
		}
	}
	else
	{
		if (S_CheckActorSoundPlaying(pact, 214))
		{
			S_StopSound(214, pact);
			if (!S_CheckActorSoundPlaying(pact, 189))
				S_PlayActorSound(189, pact);
		}
		if (S_CheckActorSoundPlaying(pact, 188))
		{
			S_StopSound(188, pact);
			if (!S_CheckActorSoundPlaying(pact, 189))
				S_PlayActorSound(189, pact);
		}
		if (!S_CheckActorSoundPlaying(pact, 189) && !S_CheckActorSoundPlaying(pact, 187))
			S_PlayActorSound(187, pact);
	}

	if (p->drink_amt > 88 && p->moto_drink == 0)
	{
		rng = krand() & 63;
		if (rng == 1)
			p->moto_drink = -10;
		else if (rng == 2)
			p->moto_drink = 10;
	}
	else if (p->drink_amt > 99 && p->moto_drink == 0)
	{
		rng = krand() & 31;
		if (rng == 1)
			p->moto_drink = -20;
		else if (rng == 2)
			p->moto_drink = 20;
	}

	if (p->on_ground == 1)
	{
		if (p->vehBraking && p->MotoSpeed > 0)
		{
			p->MotoSpeed -= p->moto_on_oil ? 2 : 4;
			if (p->MotoSpeed < 0)
				p->MotoSpeed = 0;
			p->VBumpTarget = -30;
			p->moto_do_bump = 1;
		}
		else if (p->vehForwardScale != 0 && !p->vehBraking)
		{
			if (p->MotoSpeed < 40)
			{
				p->VBumpTarget = 70;
				p->moto_bump_fast = 1;
			}

			p->MotoSpeed += 2 * p->vehForwardScale;
			p->vehForwardScale = 0;

			if (p->MotoSpeed > 120)
				p->MotoSpeed = 120;

			if (!p->NotOnWater && p->MotoSpeed > 80)
				p->MotoSpeed = 80;
		}
		else if (p->MotoSpeed > 0)
			p->MotoSpeed--;

		if (p->moto_do_bump && (!p->vehBraking || p->MotoSpeed == 0))
		{
			p->VBumpTarget = 0;
			p->moto_do_bump = 0;
		}

		if (p->vehReverseScale != 0 && p->MotoSpeed <= 0 && !p->vehBraking)
		{
			bool temp = p->vehTurnRight;
			p->vehTurnRight = p->vehTurnLeft;
			p->vehTurnLeft = temp;
			p->MotoSpeed = -15 * p->vehReverseScale;
			p->vehReverseScale = 0;
		}
	}
	if (p->MotoSpeed != 0 && p->on_ground == 1)
	{
		if (!p->VBumpNow && (krand() & 3) == 2)
			p->VBumpTarget = p->MotoSpeed * (1. / 16.) * ((krand() & 7) - 4);

		if (p->vehTurnLeft || p->moto_drink < 0)
		{
			if (p->moto_drink < 0)
				p->moto_drink++;
		}
		else if (p->vehTurnRight || p->moto_drink > 0)
		{
			if (p->moto_drink > 0)
				p->moto_drink--;
		}
	}

	double horiz = FRACUNIT;
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
		p->VBumpNow += p->moto_bump_fast ? 6 : 1;
		if (p->VBumpTarget < p->VBumpNow)
			p->VBumpNow = p->VBumpTarget;
		horiz = p->VBumpNow * (1. / 3.);
	}
	else if (p->VBumpTarget < p->VBumpNow)
	{
		p->VBumpNow -= p->moto_bump_fast ? 6 : 1;
		if (p->VBumpTarget > p->VBumpNow)
			p->VBumpNow = p->VBumpTarget;
		horiz = p->VBumpNow * (1. / 3.);
	}
	else
	{
		p->VBumpTarget = 0;
		p->moto_bump_fast = 0;
	}
	if (horiz != FRACUNIT)
	{
		p->horizon.addadjustment(buildfhoriz(horiz) - p->horizon.horiz);
	}

	int currSpeed = int(p->MotoSpeed);
	int velAdjustment;
	if (p->MotoSpeed >= 20 && p->on_ground == 1 && (p->vehTurnLeft || p->vehTurnRight))
	{
		velAdjustment = p->vehTurnLeft ? -10 : 10;
		auto angAdjustment = (velAdjustment < 0 ? 350 : -350) << BAMBITS;

		if (p->moto_on_mud || p->moto_on_oil || !p->NotOnWater)
		{
			currSpeed <<= p->moto_on_oil ? 3 : 2;

			if (p->moto_do_bump)
			{
				currSpeed >>= 5;
				angAdjustment >>= 2;
			}
			else
			{
				currSpeed >>= 7;
				angAdjustment >>= 6;
			}

			p->moto_on_mud = 0;
			p->moto_on_oil = 0;
		}
		else
		{
			if (p->moto_do_bump)
			{
				currSpeed >>= 5;
				angAdjustment >>= 4;
				if (!S_CheckActorSoundPlaying(pact, 220))
					S_PlayActorSound(220, pact);
			}
			else
			{
				currSpeed >>= 7;
				angAdjustment >>= 7;
			}
		}

		p->vel.X += currSpeed * bcos(velAdjustment * -51 + p->angle.ang.asbuild(), 4);
		p->vel.Y += currSpeed * bsin(velAdjustment * -51 + p->angle.ang.asbuild(), 4);
		p->angle.addadjustment(getincanglebam(p->angle.ang, p->angle.ang - bamang(angAdjustment)));
	}
	else if (p->MotoSpeed >= 20 && p->on_ground == 1 && (p->moto_on_mud || p->moto_on_oil))
	{
		rng = krand() & 1;
		velAdjustment = rng == 0 ? -10 : 10;
		currSpeed = MulScale(currSpeed, p->moto_on_oil ? 10 : 5, 7);
		p->vel.X += currSpeed * bcos(velAdjustment * -51 + p->angle.ang.asbuild(), 4);
		p->vel.Y += currSpeed * bsin(velAdjustment * -51 + p->angle.ang.asbuild(), 4);
	}

	p->moto_on_mud = p->moto_on_oil = 0;
	p->vehTurnLeft = p->vehTurnRight = p->vehBraking = false;
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

static void onBoat(int snum, ESyncBits &actions)
{
	auto p = &ps[snum];
	auto pact = p->GetActor();

	bool heeltoe;
	int rng;

	if (p->NotOnWater)
	{
		if (p->MotoSpeed > 0)
		{
			if (!S_CheckActorSoundPlaying(pact, 88))
				S_PlayActorSound(88, pact);
		}
		else
		{
			if (!S_CheckActorSoundPlaying(pact, 87))
				S_PlayActorSound(87, pact);
		}
	}

	if (p->MotoSpeed < 0)
		p->MotoSpeed = 0;

	if (p->vehBraking && (p->vehForwardScale != 0))
	{
		heeltoe = true;
		p->vehBraking = false;
		p->vehForwardScale = 0;
	}
	else
		heeltoe = false;

	if (p->vehForwardScale != 0)
	{
		if (p->MotoSpeed == 0 && !S_CheckActorSoundPlaying(pact, 89))
		{
			if (S_CheckActorSoundPlaying(pact, 87))
				S_StopSound(87, pact);
			S_PlayActorSound(89, pact);
		}
		else if (p->MotoSpeed >= 50 && !S_CheckActorSoundPlaying(pact, 88))
			S_PlayActorSound(88, pact);
		else if (!S_CheckActorSoundPlaying(pact, 88) && !S_CheckActorSoundPlaying(pact, 89))
			S_PlayActorSound(88, pact);
	}
	else
	{
		if (S_CheckActorSoundPlaying(pact, 89))
		{
			S_StopSound(89, pact);
			if (!S_CheckActorSoundPlaying(pact, 90))
				S_PlayActorSound(90, pact);
		}
		if (S_CheckActorSoundPlaying(pact, 88))
		{
			S_StopSound(88, pact);
			if (!S_CheckActorSoundPlaying(pact, 90))
				S_PlayActorSound(90, pact);
		}
		if (!S_CheckActorSoundPlaying(pact, 90) && !S_CheckActorSoundPlaying(pact, 87))
			S_PlayActorSound(87, pact);
	}

	if (p->vehTurnLeft && !S_CheckActorSoundPlaying(pact, 91) && p->MotoSpeed > 30 && !p->NotOnWater)
		S_PlayActorSound(91, pact);

	if (p->vehTurnRight && !S_CheckActorSoundPlaying(pact, 91) && p->MotoSpeed > 30 && !p->NotOnWater)
		S_PlayActorSound(91, pact);

	if (!p->NotOnWater)
	{
		if (p->drink_amt > 88 && p->moto_drink == 0)
		{
			rng = krand() & 63;
			if (rng == 1)
				p->moto_drink = -10;
			else if (rng == 2)
				p->moto_drink = 10;
		}
		else if (p->drink_amt > 99 && p->moto_drink == 0)
		{
			rng = krand() & 31;
			if (rng == 1)
				p->moto_drink = -20;
			else if (rng == 2)
				p->moto_drink = 20;
		}
	}

	if (p->on_ground == 1)
	{
		if (heeltoe)
		{
			if (p->MotoSpeed <= 25)
			{
				p->MotoSpeed++;
				if (!S_CheckActorSoundPlaying(pact, 182))
					S_PlayActorSound(182, pact);
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
		else if (p->vehBraking && p->MotoSpeed > 0)
		{
			p->MotoSpeed -= 2;
			if (p->MotoSpeed < 0)
				p->MotoSpeed = 0;
			p->VBumpTarget = 30;
			p->moto_do_bump = 1;
		}
		else if (p->vehForwardScale != 0)
		{
			if (p->MotoSpeed < 40 && !p->NotOnWater)
			{
				p->VBumpTarget = -30;
				p->moto_bump_fast = 1;
			}
			p->MotoSpeed += 1 * p->vehForwardScale;
			p->vehForwardScale = 0;
			if (p->MotoSpeed > 120)
				p->MotoSpeed = 120;
		}
		else if (p->MotoSpeed > 0)
			p->MotoSpeed--;

		if (p->moto_do_bump && (!p->vehBraking || p->MotoSpeed == 0))
		{
			p->VBumpTarget = 0;
			p->moto_do_bump = 0;
		}

		if (p->vehReverseScale != 0 && p->MotoSpeed == 0 && !p->vehBraking)
		{
			bool temp = p->vehTurnRight;
			p->vehTurnRight = p->vehTurnLeft;
			p->vehTurnLeft = temp;
			p->MotoSpeed = -(!p->NotOnWater ? 25 : 20) * p->vehReverseScale;
			p->vehReverseScale = 0;
		}
	}
	if (p->MotoSpeed != 0 && p->on_ground == 1)
	{
		if (!p->VBumpNow && (krand() & 15) == 14)
			p->VBumpTarget = p->MotoSpeed * (1. / 16.) * ((krand() & 3) - 2);

		if (p->vehTurnLeft && p->moto_drink < 0)
		{
			p->moto_drink++;
		}
		else if (p->vehTurnRight && p->moto_drink > 0)
		{
			p->moto_drink--;
		}
	}

	double horiz = FRACUNIT;
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
		p->VBumpNow += p->moto_bump_fast ? 6 : 1;
		if (p->VBumpTarget < p->VBumpNow)
			p->VBumpNow = p->VBumpTarget;
		horiz = p->VBumpNow * (1. / 3.);
	}
	else if (p->VBumpTarget < p->VBumpNow)
	{
		p->VBumpNow -= p->moto_bump_fast ? 6 : 1;
		if (p->VBumpTarget > p->VBumpNow)
			p->VBumpNow = p->VBumpTarget;
		horiz = p->VBumpNow * (1. / 3.);
	}
	else
	{
		p->VBumpTarget = 0;
		p->moto_bump_fast = 0;
	}
	if (horiz != FRACUNIT)
	{
		p->horizon.addadjustment(buildfhoriz(horiz) - p->horizon.horiz);
	}

	if (p->MotoSpeed > 0 && p->on_ground == 1 && (p->vehTurnLeft || p->vehTurnRight))
	{
		int currSpeed = int(p->MotoSpeed * 4.);
		int velAdjustment = p->vehTurnLeft ? -10 : 10;
		auto angAdjustment = (velAdjustment < 0 ? 350 : -350) << BAMBITS;

		if (p->moto_do_bump)
		{
			currSpeed >>= 6;
			angAdjustment >>= 5;
		}
		else
		{
			currSpeed >>= 7;
			angAdjustment >>= 6;
		}

		p->vel.X += currSpeed * bcos(velAdjustment * -51 + p->angle.ang.asbuild(), 4);
		p->vel.Y += currSpeed * bsin(velAdjustment * -51 + p->angle.ang.asbuild(), 4);
		p->angle.addadjustment(getincanglebam(p->angle.ang, p->angle.ang - bamang(angAdjustment)));
	}
	if (p->NotOnWater && p->MotoSpeed > 50)
		p->MotoSpeed -= (p->MotoSpeed / 2.);

	p->vehTurnLeft = p->vehTurnRight = p->vehBraking = false;
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

static void movement(int snum, ESyncBits actions, sectortype* psect, int fz, int cz, int shrunk, int truefdist, int psectlotag)
{
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
				if (p->cursector->floorpicnum == FLOORSLIME)
				{
					p->footprintpal = 8;
					p->footprintshade = 0;
				}
				else if (isRRRA() && (p->cursector->floorpicnum == RRTILE7756 || p->cursector->floorpicnum == RRTILE7888))
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

	if (p->pos.Z < (fz - (i << 8))) //falling
	{
		if ((actions & (SB_JUMP|SB_CROUCH)) == 0 && p->on_ground && (psect->floorstat & CSTAT_SECTOR_SLOPE) && p->pos.Z >= (fz - (i << 8) - (16 << 8)))
			p->pos.Z = fz - (i << 8);
		else
		{
			p->on_ground = 0;

			if ((p->OnMotorcycle || p->OnBoat) && fz - (i << 8) * 2 > p->pos.Z)
			{
				if (p->MotoOnGround)
				{
					p->VBumpTarget = 80;
					p->moto_bump_fast = 1;
					p->vel.Z -= xs_CRoundToInt(gs.gravity * (p->MotoSpeed / 16.));
					p->MotoOnGround = 0;
					if (S_CheckActorSoundPlaying(pact, 188))
						S_StopSound(188, pact);
					S_PlayActorSound(189, pact);
				}
				else
				{
					p->vel.Z += gs.gravity - 80 + int(120 - p->MotoSpeed);
					if (!S_CheckActorSoundPlaying(pact, 189) && !S_CheckActorSoundPlaying(pact, 190))
						S_PlayActorSound(190, pact);
				}
			}
			else
				p->vel.Z += (gs.gravity + 80); // (TICSPERFRAME<<6);

			if (p->vel.Z >= (4096 + 2048)) p->vel.Z = (4096 + 2048);
			if (p->vel.Z > 2400 && p->falling_counter < 255)
			{
				p->falling_counter++;
				if (p->falling_counter == 38 && !S_CheckActorSoundPlaying(pact, DUKE_SCREAM))
					S_PlayActorSound(DUKE_SCREAM, pact);
			}

			if ((p->pos.Z + p->vel.Z) >= (fz - (i << 8))) // hit the ground
			{
				S_StopSound(DUKE_SCREAM, pact);
				if (!p->insector() || p->cursector->lotag != 1)
				{
					if (isRRRA()) p->MotoOnGround = 1;
					if (p->falling_counter > 62 || (isRRRA() && p->falling_counter > 2 && p->insector() && p->cursector->lotag == 802))
						quickkill(p);

					else if (p->falling_counter > 9)
					{
						int j = p->falling_counter;
						pact->spr.extra -= j - (krand() & 3);
						if (pact->spr.extra <= 0)
						{
							S_PlayActorSound(SQUISHED, pact);
						}
						else
						{
							S_PlayActorSound(DUKE_LAND, pact);
							S_PlayActorSound(DUKE_LAND_HURT, pact);
						}

						SetPlayerPal(p, PalEntry(32, 16, 0, 0));
					}
					else if (p->vel.Z > 2048)
					{
						if (p->OnMotorcycle)
						{
							if (S_CheckActorSoundPlaying(pact, 190))
								S_StopSound(190, pact);
							S_PlayActorSound(191, pact);
							p->TurbCount = 12;
						}
						else S_PlayActorSound(DUKE_LAND, pact);
					}
					else if (p->vel.Z > 1024 && p->OnMotorcycle)
					{
						S_PlayActorSound(DUKE_LAND, pact);
						p->TurbCount = 12;
					}
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

			int k = ((fz - (i << 8)) - p->pos.Z) >> 1;
			if (abs(k) < 256) k = 0;
			p->pos.Z += k;
			p->vel.Z -= 768;
			if (p->vel.Z < 0) p->vel.Z = 0;
		}
		else if (p->jumping_counter == 0)
		{
			p->pos.Z += ((fz - (i << 7)) - p->pos.Z) >> 1; //Smooth on the water
			if (p->on_warping_sector == 0 && p->pos.Z > fz - (16 << 8))
			{
				p->pos.Z = fz - (16 << 8);
				p->vel.Z >>= 1;
			}
		}

		p->on_warping_sector = 0;

		if (((actions & SB_CROUCH) || crouch_toggle) && !p->OnMotorcycle)	// FIXME: The crouch_toggle check here is not network safe and needs revision when multiplayer is going.
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

	p->pos.Z += p->vel.Z;

	if (p->pos.Z < (cz + (4 << 8)))
	{
		p->jumping_counter = 0;
		if (p->vel.Z < 0)
			p->vel.X = p->vel.Y = 0;
		p->vel.Z = 128;
		p->pos.Z = cz + (4 << 8);
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

	p->jumping_counter = 0;

	p->pycount += 32;
	p->pycount &= 2047;
	p->pyoff = bsin(p->pycount, -7);

	if (!S_CheckActorSoundPlaying(pact, DUKE_UNDERWATER))
		S_PlayActorSound(DUKE_UNDERWATER, pact);

	if ((actions & SB_JUMP) && !p->OnMotorcycle)
	{
		if (p->vel.Z > 0) p->vel.Z = 0;
		p->vel.Z -= 348;
		if (p->vel.Z < -(256 * 6)) p->vel.Z = -(256 * 6);
	}
	else if ((actions & SB_CROUCH) || p->OnMotorcycle)
	{
		if (p->vel.Z < 0) p->vel.Z = 0;
		p->vel.Z += 348;
		if (p->vel.Z > (256 * 6)) p->vel.Z = (256 * 6);
	}
	else
	{
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

	p->pos.Z += p->vel.Z;

	if (p->pos.Z > (fz - (15 << 8)))
		p->pos.Z += ((fz - (15 << 8)) - p->pos.Z) >> 1;

	if (p->pos.Z < (cz + (4 << 8)))
	{
		p->pos.Z = cz + (4 << 8);
		p->vel.Z = 0;
	}

	if (p->scuba_on && (krand() & 255) < 8)
	{
		auto j = spawn(pact, WATERBUBBLE);
		if (j)
		{
			j->spr.pos.X += bcos(p->angle.ang.asbuild() + 64 - (global_random & 128) + 128, -6);
			j->spr.pos.Y += bsin(p->angle.ang.asbuild() + 64 - (global_random & 128) + 128, -6);
			j->spr.xrepeat = 3;
			j->spr.yrepeat = 2;
			j->spr.pos.Z = p->pos.Z + (8 << 8);
			j->spr.cstat = CSTAT_SPRITE_TRANS_FLIP | CSTAT_SPRITE_TRANSLUCENT;
		}
	}
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void onMotorcycleMove(int snum, walltype* wal)
{
	auto p = &ps[snum];
	auto pact = p->GetActor();
	int angleDelta = abs(p->angle.ang.asbuild() - getangle(wal->delta()));
	int damageAmount;

	p->angle.addadjustment(buildfang(p->MotoSpeed / (krand() & 1 ? -2 : 2)));

	if (angleDelta >= 441 && angleDelta <= 581)
	{
		damageAmount = xs_CRoundToInt((p->MotoSpeed * p->MotoSpeed) / 256.);
		p->MotoSpeed = 0;
		if (S_CheckActorSoundPlaying(pact, 238) == 0)
			S_PlayActorSound(238, pact);
	}
	else if (angleDelta >= 311 && angleDelta <= 711)
	{
		damageAmount = xs_CRoundToInt((p->MotoSpeed * p->MotoSpeed) / 2048.);
		p->MotoSpeed -= (p->MotoSpeed / 2.) + (p->MotoSpeed / 4.);
		if (S_CheckActorSoundPlaying(pact, 238) == 0)
			S_PlayActorSound(238, pact);
	}
	else if (angleDelta >= 111 && angleDelta <= 911)
	{
		damageAmount = xs_CRoundToInt((p->MotoSpeed * p->MotoSpeed) / 16384.);
		p->MotoSpeed -= p->MotoSpeed / 2.;
		if (S_CheckActorSoundPlaying(pact, 239) == 0)
			S_PlayActorSound(239, pact);
	}
	else
	{
		damageAmount = xs_CRoundToInt((p->MotoSpeed * p->MotoSpeed) / 32768.);
		p->MotoSpeed -= p->MotoSpeed / 8.;
		if (S_CheckActorSoundPlaying(pact, 240) == 0)
			S_PlayActorSound(240, pact);
	}
	pact->spr.extra -= damageAmount;
	if (pact->spr.extra <= 0)
	{
		S_PlayActorSound(SQUISHED, pact);
		SetPlayerPal(p, PalEntry(63, 63, 0, 0));
	}
	else if (damageAmount)
		S_PlayActorSound(DUKE_LAND_HURT, pact);

}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void onBoatMove(int snum, int psectlotag, walltype* wal)
{
	auto p = &ps[snum];
	auto pact = p->GetActor();
	auto delta = wal->delta();
	int angleDelta = abs(p->angle.ang.asbuild() - getangle(wal->delta()));

	p->angle.addadjustment(buildfang(p->MotoSpeed / (krand() & 1 ? -4 : 4)));

	if (angleDelta >= 441 && angleDelta <= 581)
	{
		p->MotoSpeed = ((p->MotoSpeed / 2.) + (p->MotoSpeed / 4.)) / 4.;
		if (psectlotag == 1 && S_CheckActorSoundPlaying(pact, 178) == 0)
			S_PlayActorSound(178, pact);
	}
	else if (angleDelta >= 311 && angleDelta <= 711)
	{
		p->MotoSpeed -= ((p->MotoSpeed / 2.) + (p->MotoSpeed / 4.)) / 8.;
		if (psectlotag == 1 && S_CheckActorSoundPlaying(pact, 179) == 0)
			S_PlayActorSound(179, pact);
	}
	else if (angleDelta >= 111 && angleDelta <= 911)
	{
		p->MotoSpeed -= p->MotoSpeed / 16.;
		if (psectlotag == 1 && S_CheckActorSoundPlaying(pact, 180) == 0)
			S_PlayActorSound(180, pact);
	}
	else
	{
		p->MotoSpeed -= p->MotoSpeed / 64.;
		if (psectlotag == 1 && S_CheckActorSoundPlaying(pact, 181) == 0)
			S_PlayActorSound(181, pact);
	}

}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void onMotorcycleHit(int snum, DDukeActor* victim)
{
	auto p = &ps[snum];
	if (badguy(victim) || victim->spr.picnum == APLAYER)
	{
		if (victim->spr.picnum != APLAYER)
		{
			if (numplayers == 1)
			{
				Collision coll;
				int ang = int(p->TiltStatus * 20 + p->angle.ang.asbuild());
				movesprite_ex(victim, bcos(ang, -8), bsin(ang, -8), victim->spr.zvel, CLIPMASK0, coll);
			}
		}
		else
			victim->SetHitOwner(p->GetActor());
		victim->attackertype = MOTOHIT;
		victim->hitextra = xs_CRoundToInt(p->MotoSpeed / 2.);
		p->MotoSpeed -= p->MotoSpeed / 4.;
		p->TurbCount = 6;
	}
	else if ((victim->spr.picnum == RRTILE2431 || victim->spr.picnum == RRTILE2443 || victim->spr.picnum == RRTILE2451 || victim->spr.picnum == RRTILE2455)
		&& victim->spr.picnum != ACTIVATORLOCKED && p->MotoSpeed > 45)
	{
		S_PlayActorSound(SQUISHED, victim);
		if (victim->spr.picnum == RRTILE2431 || victim->spr.picnum == RRTILE2451)
		{
			if (victim->spr.lotag != 0)
			{
				DukeSpriteIterator it;
				while (auto act2 = it.Next())
				{
					if ((act2->spr.picnum == RRTILE2431 || act2->spr.picnum == RRTILE2451) && act2->spr.pal == 4)
					{
						if (victim->spr.lotag == act2->spr.lotag)
						{
							act2->spr.xrepeat = 0;
							act2->spr.yrepeat = 0;
						}
					}
				}
			}
			fi.guts(victim, RRTILE2460, 12, myconnectindex);
			fi.guts(victim, RRTILE2465, 3, myconnectindex);
		}
		else
			fi.guts(victim, RRTILE2465, 3, myconnectindex);
		fi.guts(victim, RRTILE2465, 3, myconnectindex);
		victim->spr.xrepeat = 0;
		victim->spr.yrepeat = 0;
	}
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void onBoatHit(int snum, DDukeActor* victim)
{
	auto p = &ps[snum];

	if (badguy(victim) || victim->spr.picnum == APLAYER)
	{
		if (victim->spr.picnum != APLAYER)
		{
			if (numplayers == 1)
			{
				Collision coll;
				int ang = int(p->TiltStatus * 20 + p->angle.ang.asbuild());
				movesprite_ex(victim, bcos(ang, -9), bsin(ang, -9), victim->spr.zvel, CLIPMASK0, coll);
			}
		}
		else
			victim->SetHitOwner(p->GetActor());
		victim->attackertype = MOTOHIT;
		victim->hitextra = xs_CRoundToInt(p->MotoSpeed / 4.);
		p->MotoSpeed -= p->MotoSpeed / 4.;
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
					S_PlayActorSound(431, p->GetActor());
				}
			}
			else if (p->ammo_amount[THROWSAW_WEAPON] > 0)
			{
				p->kickback_pic = 1;
				S_PlayActorSound(SHRINKER_FIRE, p->GetActor());
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

static void operateweapon(int snum, ESyncBits actions, sectortype* psectp)
{
	auto p = &ps[snum];
	auto pact = p->GetActor();
	int i, k;
	int psectlotag = psectp ? psectp->lotag : 857;

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
				i = -MulScale(p->horizon.sum().asq16(), 20, 16);
			}
			else
			{
				k = 140;
				i = -512 - MulScale(p->horizon.sum().asq16(), 20, 16);
			}

			auto spawned = EGS(p->cursector,
				p->pos.X + p->angle.ang.bcos(-6),
				p->pos.Y + p->angle.ang.bsin(-6),
				p->pos.Z, HEAVYHBOMB, -16, 9, 9,
				p->angle.ang.asbuild(), (k + (p->hbomb_hold_delay << 5)) * 2, i, pact, 1);

			if (spawned)
			{
				if (k == 15)
				{
					spawned->spr.yvel = 3;
					spawned->spr.pos.Z += (8 << 8);
				}

				k = hits(p->GetActor());
				if (k < 512)
				{
					spawned->spr.ang += 1024;
					spawned->spr.zvel /= 3;
					spawned->spr.xvel /= 3;
				}

				p->hbomb_on = 1;
			}
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
			fi.shoot(pact, SHOTSPARK1);
			S_PlayActorSound(PISTOL_FIRE, pact);
			p->noise_radius = 8192;
			madenoise(snum);

			lastvisinc = PlayClock + 32;
			p->visibility = 0;
			if (psectlotag != 857)
			{
				p->vel.X -= p->angle.ang.bcos(4);
				p->vel.Y -= p->angle.ang.bsin(4);
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
					S_PlayActorSound(EJECT_CLIP, pact);
					break;
				case 30:
					S_PlayActorSound(INSERT_CLIP, pact);
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
			fi.shoot(pact, SHOTGUN);
			fi.shoot(pact, SHOTGUN);
			fi.shoot(pact, SHOTGUN);
			fi.shoot(pact, SHOTGUN);
			fi.shoot(pact, SHOTGUN);
			fi.shoot(pact, SHOTGUN);
			fi.shoot(pact, SHOTGUN);
			fi.shoot(pact, SHOTGUN);
			fi.shoot(pact, SHOTGUN);
			fi.shoot(pact, SHOTGUN);

			p->ammo_amount[SHOTGUN_WEAPON]--;

			S_PlayActorSound(SHOTGUN_FIRE, pact);

			p->noise_radius = 8192;
			madenoise(snum);

			lastvisinc = PlayClock + 32;
			p->visibility = 0;
		}

		if (p->kickback_pic == 7)
		{
			if (p->shotgun_state[1])
			{
				fi.shoot(pact, SHOTGUN);
				fi.shoot(pact, SHOTGUN);
				fi.shoot(pact, SHOTGUN);
				fi.shoot(pact, SHOTGUN);
				fi.shoot(pact, SHOTGUN);
				fi.shoot(pact, SHOTGUN);
				fi.shoot(pact, SHOTGUN);
				fi.shoot(pact, SHOTGUN);
				fi.shoot(pact, SHOTGUN);
				fi.shoot(pact, SHOTGUN);

				p->ammo_amount[SHOTGUN_WEAPON]--;

				S_PlayActorSound(SHOTGUN_FIRE, pact);

				if (psectlotag != 857)
				{
					p->vel.X -= p->angle.ang.bcos(5);
					p->vel.Y -= p->angle.ang.bsin(5);
				}
			}
			else if (psectlotag != 857)
			{
				p->vel.X -= p->angle.ang.bcos(4);
				p->vel.Y -= p->angle.ang.bsin(4);
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
				S_PlayActorSound(SHOTGUN_COCK, pact);
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
				S_PlayActorSound(SHOTGUN_COCK, pact);
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
		p->horizon.addadjustment(buildhoriz(1));
		p->recoil++;

		if (p->kickback_pic <= 12)
		{
			if ((p->kickback_pic % 3) == 0)
			{
				p->ammo_amount[RIFLEGUN_WEAPON]--;

				if ((p->kickback_pic % 3) == 0)
				{
					auto j = spawn(pact, SHELL);
					if (j)
					{

						j->spr.ang += 1024;
						j->spr.ang &= 2047;
						j->spr.xvel += 32;
						j->spr.pos.Z += (3 << 8);
						ssp(j, CLIPMASK0);
					}
				}

				S_PlayActorSound(CHAINGUN_FIRE, pact);
				fi.shoot(pact, CHAINGUN);
				p->noise_radius = 8192;
				madenoise(snum);
				lastvisinc = PlayClock + 32;
				p->visibility = 0;

				if (psectlotag != 857)
				{
					p->vel.X -= p->angle.ang.bcos(4);
					p->vel.Y -= p->angle.ang.bsin(4);
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
			fi.shoot(pact, GROWSPARK);
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
			fi.shoot(pact, SHRINKSPARK);
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
			lastvisinc = PlayClock + 32;
			S_PlayActorSound(CHAINGUN_FIRE, pact);
			fi.shoot(pact, SHOTSPARK1);
			p->noise_radius = 16384;
			madenoise(snum);
			p->ammo_amount[TIT_WEAPON]--;
			checkavailweapon(p);
		}
		if (p->kickback_pic == 2)
		{
			p->angle.addadjustment(buildang(16));
		}
		else if (p->kickback_pic == 4)
		{
			p->angle.addadjustment(buildang(-16));
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
			lastvisinc = PlayClock + 32;
			S_PlayActorSound(CHAINGUN_FIRE, pact);
			fi.shoot(pact, CHAINGUN);
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
			p->angle.addadjustment(buildang(4));
		}
		else if (p->kickback_pic == 4)
		{
			p->angle.addadjustment(buildang(-4));
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
			fi.shoot(pact, RRTILE1790);
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
			fi.shoot(pact, FIRELASER);

		if (p->kickback_pic == 5)
		{
			S_PlayActorSound(CAT_FIRE, pact);
			p->noise_radius = 2048;
			madenoise(snum);
		}
		else if (p->kickback_pic == 9)
		{
			p->ammo_amount[ALIENBLASTER_WEAPON]--;
			p->visibility = 0;
			lastvisinc = PlayClock + 32;
			checkavailweapon(p);
		}
		else if (p->kickback_pic == 12)
		{
			p->vel.X -= p->angle.ang.bcos(4);
			p->vel.Y -= p->angle.ang.bsin(4);
			p->horizon.addadjustment(buildhoriz(20));
			p->recoil += 20;
		}
		if (p->kickback_pic > 20)
			p->okickback_pic = p->kickback_pic = 0;
		break;

	case POWDERKEG_WEAPON:
		if (p->kickback_pic == 3)
		{
			p->ammo_amount[POWDERKEG_WEAPON]--;
			p->gotweapon[POWDERKEG_WEAPON] = false;
			if (p->on_ground && (actions & SB_CROUCH) && !p->OnMotorcycle)
			{
				k = 15;
				i = MulScale(p->horizon.sum().asq16(), 20, 16);
			}
			else
			{
				k = 32;
				i = -512 - MulScale(p->horizon.sum().asq16(), 20, 16);
			}

			EGS(p->cursector,
				p->pos.X + p->angle.ang.bcos(-6),
				p->pos.Y + p->angle.ang.bsin(-6),
				p->pos.Z, POWDERKEG, -16, 9, 9,
				p->angle.ang.asbuild(), k * 2, i, pact, 1);
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
			S_PlayActorSound(354, pact);
			fi.shoot(pact, BOWLINGBALL);
			p->noise_radius = 1024;
			madenoise(snum);
		}
		if (p->kickback_pic < 30)
		{
			p->vel.X += p->angle.ang.bcos(4);
			p->vel.Y += p->angle.ang.bsin(4);
		}
		p->kickback_pic++;
		if (p->kickback_pic > 40)
		{
			p->okickback_pic = p->kickback_pic = 0;
			p->gotweapon[BOWLING_WEAPON] = false;
			checkavailweapon(p);
		}
		break;

	case KNEE_WEAPON:
		p->kickback_pic++;
		if (p->kickback_pic == 3)
			S_PlayActorSound(426, pact);
		if (p->kickback_pic == 12)
		{
			fi.shoot(pact, KNEE);
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
			S_PlayActorSound(252, pact);
		if (p->kickback_pic == 8)
		{
			fi.shoot(pact, SLINGBLADE);
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
			lastvisinc = PlayClock + 32;
			p->visibility = 0;
			fi.shoot(pact, RPG);
			p->noise_radius = 32768;
			madenoise(snum);
			checkavailweapon(p);
		}
		else if (p->kickback_pic == 16)
			S_PlayActorSound(450, pact);
		else if (p->kickback_pic == 34)
			p->okickback_pic = p->kickback_pic = 0;
		break;

	case CHICKEN_WEAPON:
		p->kickback_pic++;
		if (p->kickback_pic == 4)
		{
			p->ammo_amount[CHICKEN_WEAPON]--;
			lastvisinc = PlayClock + 32;
			p->visibility = 0;
			fi.shoot(pact, RPG2);
			p->noise_radius = 32768;
			madenoise(snum);
			checkavailweapon(p);
		}
		else if (p->kickback_pic == 16)
			S_PlayActorSound(450, pact);
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

static void processweapon(int snum, ESyncBits actions, sectortype* psectp)
{
	auto p = &ps[snum];
	auto pact = p->GetActor();
	int shrunk = (pact->spr.yrepeat < 8);

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
		operateweapon(snum, actions, psectp);
	}
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void processinput_r(int snum)
{
	int i, k, doubvel, fz, cz, truefdist;
	Collision chz, clz;
	bool shrunk;
	int psectlotag;

	auto p = &ps[snum];
	auto pact = p->GetActor();

	p->horizon.resetadjustment();
	p->angle.resetadjustment();

	ESyncBits& actions = p->sync.actions;

	auto sb_fvel = PlayerInputForwardVel(snum);
	auto sb_svel = PlayerInputSideVel(snum);
	auto sb_avel = PlayerInputAngVel(snum);

	auto psectp = p->cursector;
	if (p->OnMotorcycle && pact->spr.extra > 0)
	{
		onMotorcycle(snum, actions);
	}
	else if (p->OnBoat && pact->spr.extra > 0)
	{
		onBoat(snum, actions);
	}
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

	if (psectlotag == 867)
	{
		DukeSectIterator it(psectp);
		while (auto act2 = it.Next())
		{
			if (act2->spr.picnum == RRTILE380)
				if (act2->spr.pos.Z - (8 << 8) < p->pos.Z)
					psectlotag = 2;
		}
	}
	else if (psectlotag == 7777 && (currentLevel->gameflags & LEVEL_RR_HULKSPAWN))
			lastlevel = 1;

	if (psectlotag == 848 && psectp->floorpicnum == WATERTILE2)
		psectlotag = 1;

	if (psectlotag == 857)
		pact->spr.clipdist = 1;
	else
		pact->spr.clipdist = 64;

	p->spritebridge = 0;

	shrunk = (pact->spr.yrepeat < 8);
	int tempfz;
	if (pact->spr.clipdist == 64)
	{
		getzrange(p->pos, psectp, &cz, chz, &fz, clz, 163L, CLIPMASK0);
		tempfz = getflorzofslopeptr(psectp, p->pos.X, p->pos.Y);
	}
	else
	{
		getzrange(p->pos, psectp, &cz, chz, &fz, clz, 4L, CLIPMASK0);
		tempfz = getflorzofslopeptr(psectp, p->pos.X, p->pos.Y);
	}

	p->truefz = tempfz;
	p->truecz = getceilzofslopeptr(psectp, p->pos.X, p->pos.Y);

	truefdist = abs(p->pos.Z - tempfz);
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
		if (chz.actor()->spr.statnum == 1 && chz.actor()->spr.extra >= 0)
		{
			chz.setNone();
			cz = p->truecz;
		}
		else if (chz.actor()->spr.picnum == LADDER)
		{
			if (!p->stairs)
			{
				p->stairs = 10;
				if ((actions & SB_JUMP) && !p->OnMotorcycle)
				{
					chz.setNone();
					cz = p->truecz;
				}
			}
			else
				p->stairs--;
		}
	}

	if (clz.type == kHitSprite)
	{
		if ((clz.actor()->spr.cstat & (CSTAT_SPRITE_ALIGNMENT_FLOOR| CSTAT_SPRITE_BLOCK)) == (CSTAT_SPRITE_ALIGNMENT_FLOOR | CSTAT_SPRITE_BLOCK))
		{
			psectlotag = 0;
			p->footprintcount = 0;
			p->spritebridge = 1;
		}
		if (p->OnMotorcycle)
			if (badguy(clz.actor()))
			{
				clz.actor()->attackertype = MOTOHIT;
				clz.actor()->hitextra = xs_CRoundToInt(2 + (p->MotoSpeed / 2.));
				p->MotoSpeed -= p->MotoSpeed / 16.;
			}
		if (p->OnBoat)
		{
			if (badguy(clz.actor()))
			{
				clz.actor()->attackertype = MOTOHIT;
				clz.actor()->hitextra = xs_CRoundToInt(2 + (p->MotoSpeed / 2.));
				p->MotoSpeed -= p->MotoSpeed / 16.;
			}
		}
		else if (badguy(clz.actor()) && clz.actor()->spr.xrepeat > 24 && abs(pact->spr.pos.Z - clz.actor()->spr.pos.Z) < (84 << 8))
		{
			int j = getangle(clz.actor()->spr.pos.X - p->pos.X, clz.actor()->spr.pos.Y - p->pos.Y);
			p->vel.X -= bcos(j, 4);
			p->vel.Y -= bsin(j, 4);
		}
		if (clz.actor()->spr.picnum == LADDER)
		{
			if (!p->stairs)
			{
				p->stairs = 10;
				if ((actions & SB_CROUCH) && !p->OnMotorcycle)
				{
					cz = clz.actor()->spr.pos.Z;
					chz.setNone();
					fz = clz.actor()->spr.pos.Z + (4 << 8);
				}
			}
			else
				p->stairs--;
		}
		else if (clz.actor()->spr.picnum == TOILET || clz.actor()->spr.picnum == RRTILE2121)
		{
			if ((actions & SB_CROUCH) && !p->OnMotorcycle)
				//if (Sound[436].num == 0)
				{
					S_PlayActorSound(436, pact);
					p->last_pissed_time = 4000;
					p->eat = 0;
				}
		}
	}


	if (pact->spr.extra > 0) fi.incur_damage(p);
	else
	{
		pact->spr.extra = 0;
		p->shield_amount = 0;
	}

	p->last_extra = pact->spr.extra;

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

	if (pact->spr.extra <= 0 && !ud.god)
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
		p->vel.X = p->vel.Y = pact->spr.xvel = 0;

		fi.doincrements(p);

		if (p->curr_weapon == THROWINGDYNAMITE_WEAPON) processweapon(snum, actions, psectp);
		return;
	}

	doubvel = TICSPERFRAME;

	checklook(snum, actions);
	p->apply_seasick(1);

	auto oldpos = p->opos;

	if (p->on_crane != nullptr)
		goto HORIZONLY;

	p->playerweaponsway(pact->spr.xvel);

	pact->spr.xvel = clamp(ksqrt((p->pos.X - p->bobpos.X) * (p->pos.X - p->bobpos.X) + (p->pos.Y - p->bobpos.Y) * (p->pos.Y - p->bobpos.Y)), 0, 512);
	if (p->on_ground) p->bobcounter += p->GetActor()->spr.xvel >> 1;

	p->backuppos(ud.clipping == 0 && ((p->insector() && p->cursector->floorpicnum == MIRROR) || !p->insector()));

	// Shrinking code

	i = 40;

	if (psectlotag == ST_17_PLATFORM_UP || (isRRRA() && psectlotag == ST_18_ELEVATOR_DOWN))
	{
		int tmp;
		tmp = getanimationgoal(anim_floorz, p->cursector);
		if (tmp >= 0)
		{
			if (!S_CheckActorSoundPlaying(pact, 432))
				S_PlayActorSound(432, pact);
		}
		else
			S_StopSound(432);
	}
	if (isRRRA() && p->sea_sick_stat)
	{
		p->pycount += 32;
		p->pycount &= 2047;
		p->pyoff = bsin(p->pycount, -(p->SeaSick ? 2 : 7));
	}

	if (psectlotag == ST_2_UNDERWATER)
	{
		underwater(snum, actions, fz, cz);
	}
	else
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
		int j = pact->sector()->floorpicnum;
		k = 0;

		if (p->on_ground && truefdist <= gs.playerheight + (16 << 8))
		{
			int whichsound = (gs.tileinfo[j].flags & TFLAG_ELECTRIC) ? 0 : j == FLOORSLIME ? 1 : j == FLOORPLASMA ? 2 :
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

	if (p->vel.X || p->vel.Y || sb_fvel || sb_svel)
	{
		p->crack_time = CRACK_TIME;

		k = bsin(p->bobcounter, -12);

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

		if (truefdist < gs.playerheight + (8 << 8) && (k == 1 || k == 3))
		{
			if (p->spritebridge == 0 && p->walking_snd_toggle == 0 && p->on_ground)
			{
				int j;
				switch (psectlotag)
				{
				case 0:

					if (clz.type == kHitSprite)
						j = clz.actor()->spr.picnum;
					else j = psectp->floorpicnum;
					break;
				case 1:
					if ((krand() & 1) == 0)
						if  (!isRRRA() || (!p->OnBoat && !p->OnMotorcycle && p->cursector->hitag != 321))
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

		if (!isRRRA() && ((p->curr_weapon == KNEE_WEAPON && p->kickback_pic > 10 && p->on_ground) || (p->on_ground && (actions & SB_CROUCH))))
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

		if (isRRRA() && psectp->floorpicnum == RRTILE7888)
		{
			if (p->OnMotorcycle)
				if (p->on_ground)
					p->moto_on_oil = 1;
		}
		else if (isRRRA() && psectp->floorpicnum == RRTILE7889)
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
				p->vel.X = MulScale(p->vel.X, gs.playerfriction, 16);
				p->vel.Y = MulScale(p->vel.Y, gs.playerfriction, 16);
			}
		}
		else

			if (psectp->floorpicnum == RRTILE3073 || psectp->floorpicnum == RRTILE2702)
			{
				if (p->OnMotorcycle)
				{
					if (p->on_ground)
					{
						p->vel.X = MulScale(p->vel.X, gs.playerfriction - 0x1800, 16);
						p->vel.Y = MulScale(p->vel.Y, gs.playerfriction - 0x1800, 16);
					}
				}
				else
					if (p->boot_amount > 0)
						p->boot_amount--;
					else
					{
						p->vel.X = MulScale(p->vel.X, gs.playerfriction - 0x1800, 16);
						p->vel.Y = MulScale(p->vel.Y, gs.playerfriction - 0x1800, 16);
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

	if (psectlotag == 1 || p->spritebridge == 1) i = (4L << 8);
	else i = (20L << 8);

	if (p->insector() && p->cursector->lotag == 2) k = 0;
	else k = 1;

	Collision clip{};
	if (ud.clipping)
	{
		p->pos.X += p->vel.X >> 14;
		p->pos.Y += p->vel.Y >> 14;
		updatesector(p->pos.X, p->pos.Y, &p->cursector);
		ChangeActorSect(pact, p->cursector);
	}
	else
		clipmove(p->pos, &p->cursector, p->vel.X, p->vel.Y, 164, (4 << 8), i, CLIPMASK0, clip);

	if (p->jetpack_on == 0 && psectlotag != 2 && psectlotag != 1 && shrunk)
		p->pos.Z += 32 << 8;

	if (clip.type != kHitNone)
		checkplayerhurt_r(p, clip);
	else if (isRRRA() && p->hurt_delay2 > 0)
		p->hurt_delay2--;


	if (clip.type == kHitWall)
	{
		auto wal = clip.hitWall;
		if (p->OnMotorcycle)
		{
			onMotorcycleMove(snum, wal);
		}
		else if (p->OnBoat)
		{
			onBoatMove(snum, psectlotag, wal);
		}
		else
		{
			if (wal->lotag >= 40 && wal->lotag <= 44)
			{
				if (wal->lotag < 44)
				{
					dofurniture(clip.hitWall, p->cursector, snum);
					pushmove(&p->pos, &p->cursector, 172L, (4L << 8), (4L << 8), CLIPMASK0);
				}
				else
					pushmove(&p->pos, &p->cursector, 172L, (4L << 8), (4L << 8), CLIPMASK0);
			}
		}
	}

	if (clip.type == kHitSprite)
	{
		if (p->OnMotorcycle)
		{
			onMotorcycleHit(snum, clip.actor());
		}
		else if (p->OnBoat)
		{
			onBoatHit(snum, clip.actor());
		}
		else if (badguy(clip.actor()))
		{
			if (clip.actor()->spr.statnum != 1)
			{
				clip.actor()->timetosleep = 0;
				if (clip.actor()->spr.picnum == BILLYRAY)
					S_PlayActorSound(404, clip.actor());
				else
					check_fta_sounds_r(clip.actor());
				ChangeActorStat(clip.actor(), 1);
			}
		}
		else if (!isRRRA() && clip.actor()->spr.picnum == RRTILE3410)
		{
			quickkill(p);
			S_PlayActorSound(446, pact);
		}
		if (isRRRA())
		{
			if (clip.actor()->spr.picnum == RRTILE3410)
			{
				quickkill(p);
				S_PlayActorSound(446, pact);
			}
			else if (clip.actor()->spr.picnum == RRTILE2443 && clip.actor()->spr.pal == 19)
			{
				clip.actor()->spr.pal = 0;
				p->DrugMode = 5;
				ps[snum].GetActor()->spr.extra = gs.max_player_health;
			}
		}
	}

	if (p->jetpack_on == 0)
	{
		if (pact->spr.xvel > 16)
		{
			if (psectlotag != ST_1_ABOVE_WATER && psectlotag != ST_2_UNDERWATER && p->on_ground && (!isRRRA() || !p->sea_sick_stat))
			{
				p->pycount += 52;
				p->pycount &= 2047;
				p->pyoff =
					abs(pact->spr.xvel * bsin(p->pycount)) / 1596;
			}
		}
		else if (psectlotag != ST_2_UNDERWATER && psectlotag != 1 && (!isRRRA() || !p->sea_sick_stat))
			p->pyoff = 0;
	}

	// RBG***
	SetActor(pact, { p->pos.X, p->pos.Y, p->pos.Z + gs.playerheight });

	if (psectlotag == 800 && (!isRRRA() || !p->lotag800kill))
	{
		if (isRRRA()) p->lotag800kill = 1;
		quickkill(p);
		return;
	}

	if (psectlotag < 3)
	{
		psectp = pact->sector();
		if (ud.clipping == 0 && psectp->lotag == ST_31_TWO_WAY_TRAIN)
		{
			auto act = barrier_cast<DDukeActor*>(psectp->hitagactor);
			if (act && act->spr.xvel && act->temp_data[0] == 0)
			{
				quickkill(p);
				return;
			}
		}
	}

	if (truefdist < gs.playerheight && p->on_ground && psectlotag != 1 && shrunk == 0 && p->insector() && p->cursector->lotag == 1)
		if (!S_CheckActorSoundPlaying(pact, DUKE_ONWATER))
			if (!isRRRA() || (!p->OnBoat && !p->OnMotorcycle && p->cursector->hitag != 321))
				S_PlayActorSound(DUKE_ONWATER, pact);

	if (p->cursector != pact->sector())
		ChangeActorSect(pact, p->cursector);

	int retry = 0;
	while (ud.clipping == 0)
	{
		int blocked;
		if (pact->spr.clipdist == 64)
			blocked = (pushmove(&p->pos, &p->cursector, 128, (4 << 8), (4 << 8), CLIPMASK0) < 0 && furthestangle(p->GetActor(), 8) < 512);
		else
			blocked = (pushmove(&p->pos, &p->cursector, 16, (4 << 8), (4 << 8), CLIPMASK0) < 0 && furthestangle(p->GetActor(), 8) < 512);

		if (abs(pact->floorz - pact->ceilingz) < (48 << 8) || blocked)
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

	if (ud.clipping == 0 && (!p->cursector || (p->cursector && p->cursector->ceilingz > (p->cursector->floorz - (12 << 8)))))
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
		int d = p->recoil >> 1;
		if (!d)
			d = 1;
		p->recoil -= d;
		p->horizon.addadjustment(buildhoriz(-d));
	}

	if (SyncInput())
	{
		p->horizon.applyinput(GetPlayerHorizon(snum), &actions);
	}

	p->checkhardlanding();

	//Shooting code/changes

	if (p->show_empty_weapon > 0)
		p->show_empty_weapon--;

	if (p->show_empty_weapon == 1)
	{
		fi.addweapon(p, p->last_full_weapon);
		return;
	}
	dokneeattack(snum, { FEM10, NAKED1, STATUE });


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

	processweapon(snum, actions, psectp);
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void OnMotorcycle(struct player_struct *p, DDukeActor* motosprite)
{
	if (!p->OnMotorcycle && !(p->cursector->lotag == 2))
	{
		if (motosprite)
		{
			p->pos.X = motosprite->spr.pos.X;
			p->pos.Y = motosprite->spr.pos.Y;
			p->angle.ang = buildang(motosprite->spr.ang);
			p->ammo_amount[MOTORCYCLE_WEAPON] = motosprite->saved_ammo;
			deletesprite(motosprite);
		}
		p->over_shoulder_on = 0;
		p->OnMotorcycle = 1;
		p->last_full_weapon = p->curr_weapon;
		p->curr_weapon = MOTORCYCLE_WEAPON;
		p->gotweapon[MOTORCYCLE_WEAPON] = true;
		p->vel.X = 0;
		p->vel.Y = 0;
		p->horizon.horiz = q16horiz(0);
	}
	if (!S_CheckActorSoundPlaying(p->GetActor(),186))
		S_PlayActorSound(186, p->GetActor());
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void OffMotorcycle(struct player_struct *p)
{
	auto pact = p->GetActor();
	if (p->OnMotorcycle)
	{
		if (S_CheckActorSoundPlaying(pact,188))
			S_StopSound(188,pact);
		if (S_CheckActorSoundPlaying(pact,187))
			S_StopSound(187,pact);
		if (S_CheckActorSoundPlaying(pact,186))
			S_StopSound(186,pact);
		if (S_CheckActorSoundPlaying(pact,214))
			S_StopSound(214,pact);
		if (!S_CheckActorSoundPlaying(pact,42))
			S_PlayActorSound(42, pact);
		p->OnMotorcycle = 0;
		p->gotweapon[MOTORCYCLE_WEAPON] = false;
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
		p->vel.X = 0;
		p->vel.Y = 0;
		p->vel.X -= p->angle.ang.bcos(7);
		p->vel.Y -= p->angle.ang.bsin(7);
		p->moto_underwater = 0;
		auto spawned = spawn(p->GetActor(), EMPTYBIKE);
		if (spawned)
		{
			spawned->spr.ang = p->angle.ang.asbuild();
			spawned->spr.xvel += p->angle.ang.bcos(7);
			spawned->spr.yvel += p->angle.ang.bsin(7);
			spawned->saved_ammo = p->ammo_amount[MOTORCYCLE_WEAPON];
		}
	}
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void OnBoat(struct player_struct *p, DDukeActor* boat)
{
	if (!p->OnBoat)
	{
		if (boat)
		{
			p->pos.X = boat->spr.pos.X;
			p->pos.Y = boat->spr.pos.Y;
			p->angle.ang = buildang(boat->spr.ang);
			p->ammo_amount[BOAT_WEAPON] = boat->saved_ammo;
			deletesprite(boat);
		}
		p->over_shoulder_on = 0;
		p->OnBoat = 1;
		p->last_full_weapon = p->curr_weapon;
		p->curr_weapon = BOAT_WEAPON;
		p->gotweapon[BOAT_WEAPON] = true;
		p->vel.X = 0;
		p->vel.Y = 0;
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
	if (p->OnBoat)
	{
		p->OnBoat = 0;
		p->gotweapon[BOAT_WEAPON] = false;
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
		p->vel.X = 0;
		p->vel.Y = 0;
		p->vel.X -= p->angle.ang.bcos(7);
		p->vel.Y -= p->angle.ang.bsin(7);
		p->moto_underwater = 0;
		auto spawned = spawn(p->GetActor(), EMPTYBOAT);
		if (spawned)
		{
			spawned->spr.ang = p->angle.ang.asbuild();
			spawned->spr.xvel += p->angle.ang.bcos(7);
			spawned->spr.yvel += p->angle.ang.bsin(7);
			spawned->saved_ammo = p->ammo_amount[BOAT_WEAPON];
		}
	}
}


END_DUKE_NS
