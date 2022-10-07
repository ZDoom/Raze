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

static void shootfireball(DDukeActor *actor, int p, DVector3 pos, DAngle ang)
{
	// World Tour's values for angles and velocities are quite arbitrary...
	double vel, zvel;

	if (actor->spr.extra >= 0)
		actor->spr.shade = -96;

	pos.Z -= 2;
	if (actor->spr.picnum != BOSS5)
		vel = 840/16.;
	else {
		vel = 968/16.;
		pos.Z += 24;
	}

	if (p < 0)
	{
		ang += DAngle22_5 / 8 - randomAngle(22.5 / 4);
		double scratch;
		int j = findplayer(actor, &scratch);
		double dist = (ps[j].GetActor()->spr.pos.XY() - actor->spr.pos.XY()).Length();
		zvel = ((ps[j].opos.Z - pos.Z + 3) * vel) / dist;
	}
	else
	{
		zvel = ps[p].horizon.sum().Tan() * 49.;
		pos += (ang + DAngle1 * 61).ToVector() * (1024 / 448.);
		pos.Z += 3;
	}

	double scale = p >= 0? 0.109375 : 0.28125;

	auto spawned = CreateActor(actor->sector(), pos, FIREBALL, -127, DVector2(scale, scale), ang, vel, zvel, actor, (short)4);
	if (spawned)
	{
		spawned->spr.extra += (krand() & 7);
		if (actor->spr.picnum == BOSS5 || p >= 0)
		{
			spawned->spr.SetScale(0.625, 0.625);
		}
		spawned->spr.yint = p;
		spawned->spr.cstat = CSTAT_SPRITE_YCENTER;
		spawned->clipdist = 1;
	}
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

static void shootflamethrowerflame(DDukeActor* actor, int p, DVector3 spos, DAngle sang)
{
	double vel, zvel = 0;

	if (actor->spr.extra >= 0)
		actor->spr.shade = -96;
	vel = 25;

	DDukeActor* spawned = nullptr;
	if (p < 0)
	{
		double x;
		int j = findplayer(actor, &x);
		sang = (ps[j].opos.XY() - spos.XY()).Angle();

		if (actor->spr.picnum == BOSS5)
		{
			vel = 33;
			spos.Z += 24;
		}
		else if (actor->spr.picnum == BOSS3)
			spos.Z -= 32;

		double dist = (ps[j].GetActor()->spr.pos.XY() - actor->spr.pos.XY()).Length();
		if (dist != 0)
			zvel = (((ps[j].opos.Z - spos.Z) * vel) / dist);

		if (badguy(actor) && (actor->spr.hitag & face_player_smart) != 0)
			sang = actor->spr.angle + mapangle((krand() & 31) - 16);

		if (actor->sector()->lotag == 2 && (krand() % 5) == 0)
			spawned = spawn(actor, WATERBUBBLE);
	}
	else
	{
		zvel = ps[p].horizon.sum().Tan() * 40.5;
		
		// WTF???
		DAngle myang = DAngle90 - (DAngle180 - abs(abs((spos.XY() - ps[p].pos.XY()).Angle() - sang) - DAngle180));
		if (ps[p].GetActor()->vel.X != 0)
			vel = ((myang / DAngle90) * ps[p].GetActor()->vel.X) + 25;
		if (actor->sector()->lotag == 2 && (krand() % 5) == 0)
			spawned = spawn(actor, WATERBUBBLE);
	}

	if (spawned == nullptr)
	{
		spawned = spawn(actor, FLAMETHROWERFLAME);
		if (!spawned) return;
		spawned->vel.X = vel;
		spawned->vel.Z = zvel;
	}


	DVector3 offset;
	offset.X = (sang + DAngle::fromBuild(118)).Cos() * (1024 / 448.); // Yes, these angles are really different!
	offset.Y = (sang + DAngle::fromBuild(112)).Sin() * (1024 / 448.);
	offset.Z = -1;

	spawned->spr.pos = spos + offset;
	spawned->spr.pos.Z--;
	spawned->setsector(actor->sector());
	spawned->spr.cstat = CSTAT_SPRITE_YCENTER;
	spawned->spr.angle = sang;
	spawned->spr.SetScale(0.03125, 0.03125);
	spawned->clipdist = 10;
	spawned->spr.yint = p;
	spawned->SetOwner(actor);

	if (p == -1)
	{
		if (actor->spr.picnum == BOSS5)
		{
			spawned->spr.pos += sang.ToVector() * (128. / 7);
			spawned->spr.SetScale(0.15625, 0.15625);
		}
	}
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

static void shootknee(DDukeActor* actor, int p, DVector3 pos, DAngle ang)
{
	auto sectp = actor->sector();
	double zvel;
	HitInfo hit{};

	if (p >= 0)
	{
		zvel = ps[p].horizon.sum().Tan() * 16.;
		pos.Z += 6;
		ang += DAngle1 * 2.64;
	}
	else
	{
		double x;
		auto pactor = ps[findplayer(actor, &x)].GetActor();
		zvel = ((pactor->spr.pos.Z - pos.Z) * 16) / (x + 1/16.);
		ang = (pactor->spr.pos.XY() - pos.XY()).Angle();
	}

	hitscan(pos, sectp, DVector3(ang.ToVector() * 1024, zvel * 64), hit, CLIPMASK1);


	if (hit.hitSector == nullptr) return;

	if ((pos.XY() - hit.hitpos.XY()).Sum() < 64)
	{
		if (hit.hitWall || hit.actor())
		{
			auto knee = CreateActor(hit.hitSector, hit.hitpos, KNEE, -15, DVector2(0, 0), ang, 2., 0., actor, 4);
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
					fi.checkhitwall(knee, hit.hitWall, hit.hitpos, KNEE);
					if (p >= 0) fi.checkhitswitch(p, hit.hitWall, nullptr);
				}
			}
		}
		else if (p >= 0 && zvel > 0 && hit.hitSector->lotag == 1)
		{
			auto splash = spawn(ps[p].GetActor(), WATERSPLASH2);
			if (splash)
			{
				splash->spr.pos.XY() = hit.hitpos.XY();
				splash->spr.angle = ps[p].angle.ang; // Total tweek
				splash->vel.X = 2;
				ssp(actor, CLIPMASK0);
				splash->vel.X = 0;
			}

		}
	}
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

static void shootweapon(DDukeActor *actor, int p, DVector3 pos, DAngle ang, int atwith)
{
	auto sectp = actor->sector();
	double zvel = 0;
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
			double dal = ((aimed->spr.scale.X * tileHeight(aimed->spr.picnum)) * 0.5) + 5;
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
				dal -= 8;
				break;
			}
			double dist = (ps[p].GetActor()->spr.pos.XY() - aimed->spr.pos.XY()).Length();
			zvel = ((aimed->spr.pos.Z - pos.Z - dal) * 16) / dist;
			ang = (aimed->spr.pos - pos).Angle();
		}

		if (isWW2GI())
		{
			int angRange = 32;
			double zRange = 1;
			SetGameVarID(g_iAngRangeVarID, 32, actor, p);
			SetGameVarID(g_iZRangeVarID, 256, actor, p);
			OnEvent(EVENT_GETSHOTRANGE, p, ps[p].GetActor(), -1);
			angRange = GetGameVarID(g_iAngRangeVarID, actor, p).value();
			zRange = GetGameVarID(g_iZRangeVarID, actor, p).value() / 256.;

			ang += DAngle::fromBuild((angRange / 2) - (krand() & (angRange - 1)));
			if (aimed == nullptr)
			{
				// no target
				zvel = ps[p].horizon.sum().Tan() * 16.;
			}
			zvel += (zRange / 2) - krandf(zRange);
		}
		else if (aimed == nullptr)
		{
			ang += DAngle22_5 / 8 - randomAngle(22.5 / 4);
			zvel = ps[p].horizon.sum().Tan() * 16.;
			zvel += 0.5 - krandf(1);
		}

		pos.Z -= 2;
	}
	else
	{
		double x;
		int j = findplayer(actor, &x);
		pos.Z -= 4;
		double dist = (ps[j].GetActor()->spr.pos.XY() - actor->spr.pos.XY()).Length();
		zvel = ((ps[j].pos.Z - pos.Z) * 16) / dist;
		zvel += 0.5 - krandf(1);
		if (actor->spr.picnum != BOSS1)
		{
			ang += DAngle22_5 / 8 - randomAngle(22.5 / 4);
		}
		else
		{
			ang = (ps[j].pos - pos).Angle() + DAngle22_5 / 2 - randomAngle(22.5);
		}
	}

	actor->spr.cstat &= ~CSTAT_SPRITE_BLOCK_ALL;
	hitscan(pos, sectp, DVector3(ang.ToVector() * 1024, zvel * 64), hit, CLIPMASK1);
	actor->spr.cstat |= CSTAT_SPRITE_BLOCK_ALL;


	if (hit.hitSector == nullptr) return;

	if ((krand() & 15) == 0 && hit.hitSector->lotag == 2)
		tracers(hit.hitpos, pos, 8 - (ud.multimode >> 1));

	DDukeActor* spark;
	if (p >= 0)
	{
		spark = CreateActor(hit.hitSector, hit.hitpos, SHOTSPARK1, -15, DVector2(0.15625, 0.15625), ang, 0., 0., actor, 4);
		if (!spark) return;

		spark->spr.extra = ScriptCode[gs.actorinfo[atwith].scriptaddress];
		spark->spr.extra += (krand() % 6);

		if (hit.hitWall == nullptr && hit.actor() == nullptr)
		{
			if (zvel < 0)
			{
				if (hit.hitSector->ceilingstat & CSTAT_SECTOR_SKY)
				{
					spark->spr.SetScale(0, 0);
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
				spark->spr.SetScale(0, 0);
				auto jib = spawn(spark, JIBS6);
				if (jib)
				{
					jib->spr.pos.Z += 4;
					jib->vel.X = 1;
					jib->spr.SetScale(0.375, 0.375);
					jib->spr.angle += DAngle22_5 / 2 - randomAngle(22.5);
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
									if ((l->spr.pos - spark->spr.pos).Length() < 0.75 + krandf(0.5))
										goto SKIPBULLETHOLE;
							}
							auto hole = spawn(spark, BULLETHOLE);
							if (hole)
							{
								hole->vel.X = -1 / 16.;
								hole->spr.angle = hit.hitWall->delta().Angle() - DAngle90;
								ssp(hole, CLIPMASK0);
								hole->spr.cstat2 |= CSTAT2_SPRITE_DECAL;
							}
						}

		SKIPBULLETHOLE:

			if (hit.hitWall->cstat & CSTAT_WALL_BOTTOM_SWAP)
				if (hit.hitWall->twoSided())
					if (hit.hitpos.Z >= hit.hitWall->nextSector()->floorz)
						hit.hitWall = hit.hitWall->nextWall();

			fi.checkhitwall(spark, hit.hitWall, hit.hitpos, SHOTSPARK1);
		}
	}
	else
	{
		spark = CreateActor(hit.hitSector, hit.hitpos, SHOTSPARK1, -15, DVector2(0.375, 0.375), ang, 0., 0., actor, 4);
		if (spark)
		{
			spark->spr.extra = ScriptCode[gs.actorinfo[atwith].scriptaddress];

			if (hit.actor())
			{
				fi.checkhitsprite(hit.actor(), spark);
				if (!hit.actor()->isPlayer())
					spawn(spark, SMALLSMOKE);
				else spark->spr.SetScale(0, 0);
			}
			else if (hit.hitWall)
				fi.checkhitwall(spark, hit.hitWall, hit.hitpos, SHOTSPARK1);
		}
	}

	if ((krand() & 255) < 4)
	{
		S_PlaySound3D(PISTOL_RICOCHET, spark, hit.hitpos);
	}
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

static void shootstuff(DDukeActor* actor, int p, DVector3 pos, DAngle ang, int atwith)
{
	sectortype* sect = actor->sector();
	double vel, zvel;
	int scount;

	if (actor->spr.extra >= 0) actor->spr.shade = -96;

	scount = 1;
	if (atwith == SPIT) vel = 292;
	else
	{
		if (atwith == COOLEXPLOSION1)
		{
			if (actor->spr.picnum == BOSS2) vel = 644 / 16.;
			else vel = 348 / 16.;
			pos.Z -= 2;
		}
		else
		{
			vel = 840 / 16.;
			pos.Z -= 2;
		}
	}

	if (p >= 0)
	{
		auto aimed = aim(actor, AUTO_AIM_ANGLE);

		if (aimed)
		{
			double dal = ((aimed->spr.scale.X * tileHeight(aimed->spr.picnum)) * 0.5) - 12;
			double dist = (ps[p].GetActor()->spr.pos.XY() - aimed->spr.pos.XY()).Length();

			zvel = ((aimed->spr.pos.Z - pos.Z - dal) * vel) / dist;
			ang = (aimed->spr.pos.XY() - pos.XY()).Angle();
		}
		else
			zvel = ps[p].horizon.sum().Tan() * 49.;
	}
	else
	{
		double x;
		int j = findplayer(actor, &x);
		ang += DAngle22_5 / 8 - randomAngle(22.5 / 4);
#if 1
		double dist = (ps[j].GetActor()->spr.pos.XY() - actor->spr.pos.XY()).Length();
		zvel = ((ps[j].opos.Z - pos.Z + 3) * vel) / dist;
#else
		// this is for pitch corrected velocity
		auto dist = (ps[j].GetActor()->spr.pos - actor->spr.pos).Resized(vel);
		vel = dist.XY().Length();
		zvel = dist.Z;
#endif
	}

	double oldzvel = zvel;
	double scale = p >= 0? 0.109375 : 0.28125;
	if (atwith == SPIT)
	{
		pos.Z -= 10;
	}
	// Whatever else was here always got overridden by the final 'p>=0' check.


	while (scount > 0)
	{
		auto spawned = CreateActor(sect, pos, atwith, -127, DVector2(scale, scale), ang, vel, zvel, actor, 4);
		if (!spawned) return;
		spawned->spr.extra += (krand() & 7);

		if (atwith == COOLEXPLOSION1)
		{
			spawned->spr.shade = 0;
			if (actor->spr.picnum == BOSS2)
			{
				auto ovel = spawned->vel.X;
				spawned->vel.X = 64;
				ssp(spawned, CLIPMASK0);
				spawned->vel.X = ovel;
				spawned->spr.angle += DAngle22_5 - randomAngle(45);
			}
		}

		spawned->spr.cstat = CSTAT_SPRITE_YCENTER;
		spawned->clipdist = 1;

		ang = actor->spr.angle + DAngle22_5 / 4 - randomAngle(22.5 / 2);
		zvel = oldzvel + 2 - krandf(4);

		scount--;
	}
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

static void shootrpg(DDukeActor *actor, int p, DVector3 pos, DAngle ang, int atwith)
{
	auto sect = actor->sector();
	double vel, zvel;
	int scount;

	if (actor->spr.extra >= 0) actor->spr.shade = -96;

	scount = 1;
	vel = 644 / 16.;

	DDukeActor* aimed = nullptr;

	if (p >= 0)
	{
		aimed = aim(actor, AUTO_AIM_ANGLE);
		if (aimed)
		{
			double dal = ((aimed->spr.scale.X * tileHeight(aimed->spr.picnum)) * 0.5) + 8;
			double dist = (ps[p].GetActor()->spr.pos.XY() - aimed->spr.pos.XY()).Length();
			zvel = ((aimed->spr.pos.Z - pos.Z - dal) * vel) / dist;
			if (aimed->spr.picnum != RECON)
				ang = (aimed->spr.pos.XY() - pos.XY()).Angle();
		}
		else 
			zvel = ps[p].horizon.sum().Tan() * 40.5;

		if (atwith == RPG)
			S_PlayActorSound(RPG_SHOOT, actor);

	}
	else
	{
		double x;
		int j = findplayer(actor, &x);
		ang = (ps[j].opos.XY() - pos.XY()).Angle();
		if (actor->spr.picnum == BOSS3)
		{
			double zoffs = 32;
			if (isWorldTour()) // Twentieth Anniversary World Tour
				zoffs *= (actor->spr.scale.Y * 0.8);
			pos.Z += zoffs;
		}
		else if (actor->spr.picnum == BOSS2)
		{
			vel += 8;
			double zoffs = 24;
			if (isWorldTour()) // Twentieth Anniversary World Tour
				zoffs *= (actor->spr.scale.Y * 0.8);
			pos.Z -= zoffs;
		}

		double dist = (ps[j].GetActor()->spr.pos.XY() - actor->spr.pos.XY()).Length();

		zvel = ((ps[j].opos.Z - pos.Z) * vel) / dist;

		if (badguy(actor) && (actor->spr.hitag & face_player_smart))
			ang = actor->spr.angle + randomAngle(DAngle22_5 / 4) - DAngle22_5 / 8;
	}
	if (p < 0) aimed = nullptr;

	auto offset = (ang + DAngle1 * 61.171875).ToVector() * (1024. / 448.);
	auto spawned = CreateActor(sect, pos.plusZ(-1) + offset, atwith, 0, DVector2(0.21875, 0.21875), ang, vel, zvel, actor, 4);

	if (!spawned) return;

	spawned->spr.extra += (krand() & 7);
	if (atwith != FREEZEBLAST)
		spawned->temp_actor = aimed;
	else
	{
		spawned->spr.yint = gs.numfreezebounces;
		spawned->spr.scale *= 0.5;
		spawned->vel.Z -= 0.25;
	}

	if (p == -1)
	{
		if (actor->spr.picnum == BOSS3)
		{
			DVector2 spawnofs(ang.Sin() * 4, ang.Cos() * -4);
			DAngle aoffs = DAngle22_5 / 32.;

			if ((krand() & 1) != 0)
			{
				spawnofs = -spawnofs;
				aoffs = -aoffs;
			}

			if (isWorldTour()) // Twentieth Anniversary World Tour
			{
				float siz = actor->spr.scale.Y * 0.8;
				spawnofs *= siz;
				aoffs *= siz;
			}

			spawned->spr.pos += spawnofs;
			spawned->spr.angle += aoffs;

			spawned->spr.SetScale(0.65625, 0.65625);
		}
		else if (actor->spr.picnum == BOSS2)
		{
			DVector2 spawnofs(ang.Sin() * (1024. / 56.), ang.Cos() * -(1024. / 56.));
			DAngle aoffs = DAngle22_5 / 16. - DAngle45 + randomAngle(90);

			if (isWorldTour()) { // Twentieth Anniversary World Tour
				double siz = actor->spr.scale.Y * 0.9143;
				spawnofs *= siz;
				aoffs *= siz;
			}

			spawned->spr.pos += spawnofs;
			spawned->spr.angle += aoffs;

			spawned->spr.SetScale(0.375, 0.375);
		}
		else if (atwith != FREEZEBLAST)
		{
			spawned->spr.SetScale(0.46875, 0.46875);
			spawned->spr.extra >>= 2;
		}
	}
	else if ((isWW2GI() && aplWeaponWorksLike(ps[p].curr_weapon, p) == DEVISTATOR_WEAPON) || (!isWW2GI() && ps[p].curr_weapon == DEVISTATOR_WEAPON))
	{
		spawned->spr.extra >>= 2;
		spawned->spr.angle += DAngle22_5 / 8 - randomAngle(22.5 / 4);
		spawned->vel.Z += 1 - krandf(2);

		if (ps[p].hbomb_hold_delay)
		{
			DVector2 spawnofs(-ang.Sin()* (1024. / 644.), ang.Cos() * (1024. / 644.));
			spawned->spr.pos += spawnofs;
		}
		else
		{
			DVector2 spawnofs(ang.Sin()* 4, ang.Cos() * -4);
			spawned->spr.pos += spawnofs;
		}
		spawned->spr.scale *= 0.5;
	}

	spawned->spr.cstat = CSTAT_SPRITE_YCENTER;
	if (atwith == RPG)
		spawned->clipdist = 1;
	else
		spawned->clipdist = 10;

}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

static void shootlaser(DDukeActor* actor, int p, DVector3 pos, DAngle ang)
{
	auto sectp = actor->sector();
	double zvel;
	int j;
	HitInfo hit{};

	if (p >= 0)
		zvel = ps[p].horizon.sum().Tan() * 16.;
	else zvel = 0;

	hitscan(pos, sectp, DVector3(ang.ToVector() * 1024, zvel * 64), hit, CLIPMASK1);

	j = 0;
	if (hit.actor()) return;

	if (hit.hitWall && hit.hitSector)
	{
		if ((hit.hitpos.XY() - pos.XY()).LengthSquared() < 18.125 * 18.125)
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
			auto bomb = CreateActor(hit.hitSector, hit.hitpos, TRIPBOMB, -16, DVector2(0.0625, 0.078125), ang, 0., 0., actor, STAT_STANDABLE);
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
			bomb->vel.X = -1.25;
			ssp(bomb, CLIPMASK0);
			bomb->spr.cstat = CSTAT_SPRITE_ALIGNMENT_WALL;
			auto delta = -hit.hitWall->delta();
			bomb->spr.angle = delta.Angle() - DAngle90;
			bomb->temp_angle = bomb->spr.angle;

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

static void shootgrowspark(DDukeActor* actor, int p, DVector3 pos, DAngle ang)
{
	auto sect = actor->sector();
	double zvel;
	int k;
	HitInfo hit{};

	if (p >= 0)
	{
		auto aimed = aim(actor, AUTO_AIM_ANGLE);
		if (aimed)
		{
			double dal = ((aimed->spr.scale.X * tileHeight(aimed->spr.picnum)) * 0.5) + 5;
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
				dal -= 8;
				break;
			}
			double dist = (ps[p].GetActor()->spr.pos.XY() - aimed->spr.pos.XY()).Length();
			zvel = ((aimed->spr.pos.Z - pos.Z - dal) * 16) / dist;
			ang = (aimed->spr.pos.XY() - pos.XY()).Angle();
		}
		else
		{
			ang += DAngle22_5 / 8 - randomAngle(22.5 / 4);
			zvel = ps[p].horizon.sum().Tan() * 16.;
			zvel += 0.5 - krandf(1);
		}

		pos.Z -= 2;
	}
	else
	{
		double x;
		int j = findplayer(actor, &x);
		pos.Z -= 4;
		double dist = (ps[j].GetActor()->spr.pos.XY() - actor->spr.pos.XY()).Length();
		zvel = ((ps[j].pos.Z - pos.Z) * 16) / dist;
		zvel += 0.5 - krandf(1);
		ang += DAngle22_5 / 4 - randomAngle(22.5 / 2);
	}

	k = 0;

	//RESHOOTGROW:

	actor->spr.cstat &= ~CSTAT_SPRITE_BLOCK_ALL;
	hitscan(pos, sect, DVector3(ang.ToVector() * 1024, zvel * 64), hit, CLIPMASK1);

	actor->spr.cstat |= CSTAT_SPRITE_BLOCK_ALL;

	auto spark = CreateActor(sect, hit.hitpos, GROWSPARK, -16, DVector2(0.4375, 0.4375), ang, 0., 0., actor, 1);
	if (!spark) return;

	spark->spr.pal = 2;
	spark->spr.cstat |= CSTAT_SPRITE_YCENTER | CSTAT_SPRITE_TRANSLUCENT;
	spark->spr.SetScale(REPEAT_SCALE, REPEAT_SCALE);

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
			fi.checkhitwall(spark, hit.hitWall, hit.hitpos, GROWSPARK);
		}
	}
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

static void shootmortar(DDukeActor* actor, int p, const DVector3& pos, DAngle ang, int atwith)
{
	auto sect = actor->sector();
	if (actor->spr.extra >= 0) actor->spr.shade = -96;

	double x;
	auto plActor = ps[findplayer(actor, &x)].GetActor();
	x = (plActor->spr.pos.XY() - actor->spr.pos.XY()).Length();

	double zvel = -x * 0.5;

	if (zvel < -8)
		zvel = -4;
	double vel = x / 16.;

	CreateActor(sect, pos.plusZ(-6) + ang.ToVector() * 4, atwith, -64, DVector2(0.5, 0.5), ang, vel, zvel, actor, 1);
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

static void shootshrinker(DDukeActor* actor, int p, const DVector3& pos, DAngle ang, int atwith)
{
	double zvel;
	if (actor->spr.extra >= 0) actor->spr.shade = -96;
	if (p >= 0)
	{
		auto aimed = isNamWW2GI() ? nullptr : aim(actor, AUTO_AIM_ANGLE);
		if (aimed)
		{
			double dal = ((aimed->spr.scale.X * tileHeight(aimed->spr.picnum)) * 0.5);
			double dist = (ps[p].GetActor()->spr.pos.XY() - aimed->spr.pos.XY()).Length();
			zvel = ((aimed->spr.pos.Z - pos.Z - dal - 4) * 48) / dist;
			ang = (aimed->spr.pos.XY() - pos.XY()).Angle();
		}
		else
			zvel = ps[p].horizon.sum().Tan() * 49.;
	}
	else if (actor->spr.statnum != 3)
	{
		double x;
		int j = findplayer(actor, &x);
		double dist = (ps[j].GetActor()->spr.pos.XY() - actor->spr.pos.XY()).Length();
		zvel = ((ps[j].pos.Z - pos.Z) * 32) / dist;
	}
	else zvel = 0;

	auto spawned = CreateActor(actor->sector(),
		pos.plusZ(2) + ang.ToVector() * 0.25, SHRINKSPARK, -16, DVector2(0.4375, 0.4375), ang, 48., zvel, actor, 4);

	if (spawned)
	{
		spawned->spr.cstat = CSTAT_SPRITE_YCENTER;
		spawned->clipdist = 8;
	}
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void shoot_d(DDukeActor* actor, int atwith)
{
	int p;
	DVector3 spos;
	DAngle sang;

	auto const sect = actor->sector();

	if (actor->isPlayer())
	{
		p = actor->PlayerIndex();
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


	if (actor->isPlayer())
	{
		spos = ps[p].pos.plusZ(ps[p].pyoff + 4);
		sang = ps[p].angle.ang;

		ps[p].crack_time = CRACK_TIME;

	}
	else
	{
		sang = actor->spr.angle;
		spos = actor->spr.pos.plusZ(-(actor->spr.scale.Y * tileHeight(actor->spr.picnum) * 0.5) + 4);

		if (actor->spr.picnum != ROTATEGUN)
		{
			spos.Z -= 7;
			if (badguy(actor) && actor->spr.picnum != COMMANDER)
			{
				spos.X -= (sang + DAngle22_5 * 0.75).Sin() * 8;
				spos.Y += (sang + DAngle22_5 * 0.75).Cos() * 8;
			}
		}
	}

	if (isWorldTour()) 
	{ // Twentieth Anniversary World Tour
		switch (atwith) 
		{
		case FIREBALL:
			shootfireball(actor, p, spos, sang);
			return;

		case FLAMETHROWERFLAME:
			shootflamethrowerflame(actor, p, spos, sang);
			return;

		case FIREFLY: // BOSS5 shot
		{
			auto k = spawn(actor, atwith);
			if (k)
			{
				k->setsector(sect);
				k->spr.pos = spos;
				k->spr.angle = sang;
				k->vel.X = 500 / 16.;
				k->vel.Z = 0;
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
		shootbloodsplat(actor, p, spos, sang, atwith, BIGFORCE, OOZFILTER, NEWBEAST);
		break;

	case KNEE:
		shootknee(actor, p, spos, sang);
		break;

	case SHOTSPARK1:
	case SHOTGUN:
	case CHAINGUN:
		shootweapon(actor, p, spos, sang, atwith);
		return;

	case FIRELASER:
	case SPIT:
	case COOLEXPLOSION1:
		shootstuff(actor, p, spos, sang, atwith);
		return;

	case FREEZEBLAST:
		spos.Z += 3;
		[[fallthrough]];

	case RPG:
		shootrpg(actor, p, spos, sang, atwith);
		break;

	case HANDHOLDINGLASER:
		shootlaser(actor, p, spos, sang);
		return;

	case BOUNCEMINE:
	case MORTER:
		shootmortar(actor, p, spos, sang, atwith);
		return;

	case GROWSPARK:
		shootgrowspark(actor, p, spos, sang);
		break;

	case SHRINKER:
		shootshrinker(actor, p, spos, sang, atwith);
		break;
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
	if (p->last_pissed_time <= (26 * 218) && p->show_empty_weapon == 0 && p->kickback_pic == 0 && p->quick_kick == 0 && p->GetActor()->spr.scale.X > 0.5  && p->access_incs == 0 && p->knee_incs == 0)
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
	snum = pact->PlayerIndex();

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
		int snum = p->GetActor()->PlayerIndex();
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

static void operateJetpack(int snum, ESyncBits actions, int psectlotag, double floorz, double ceilingz, int shrunk)
{
	auto p = &ps[snum];
	auto pact = p->GetActor();
	p->on_ground = 0;
	p->jumping_counter = 0;
	p->hard_landing = 0;
	p->falling_counter = 0;

	p->pycount += 32;
	p->pycount &= 2047;
	p->pyoff = BobVal(p->pycount);

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

	if (p->pos.Z > floorz - k)
		p->pos.Z += ((floorz - k) - p->pos.Z) * 0.5;
	if (p->pos.Z < pact->ceilingz + 18)
		p->pos.Z = pact->ceilingz + 18;

}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

static void movement(int snum, ESyncBits actions, sectortype* psect, double floorz, double ceilingz, int shrunk, double truefdist, int psectlotag)
{
	int j;
	auto p = &ps[snum];
	auto pact = p->GetActor();

	if (p->airleft != 15 * 26)
		p->airleft = 15 * 26; //Aprox twenty seconds.

	if (p->scuba_on == 1)
		p->scuba_on = 0;

	double i = 40;
	if (psectlotag == ST_1_ABOVE_WATER && p->spritebridge == 0)
	{
		if (shrunk == 0)
		{
			i = 34;
			p->pycount += 32;
			p->pycount &= 2047;
			p->pyoff = BobVal(p->pycount) * 2;
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

	if (p->pos.Z < floorz - i) //falling
	{

		// not jumping or crouching
		if ((actions & (SB_JUMP|SB_CROUCH)) == 0 && p->on_ground && (psect->floorstat & CSTAT_SECTOR_SLOPE) && p->pos.Z >= (floorz - i - 16))
			p->pos.Z = floorz - i;
		else
		{
			p->on_ground = 0;
			p->vel.Z += (gs.gravity + 5/16.); // (TICSPERFRAME<<6);
			if (p->vel.Z >= (16 + 8)) p->vel.Z = (16 + 8);
			if (p->vel.Z > 2400 / 256 && p->falling_counter < 255)
			{
				p->falling_counter++;
				if (p->falling_counter == 38 && !S_CheckActorSoundPlaying(pact, DUKE_SCREAM))
					S_PlayActorSound(DUKE_SCREAM, pact);
			}

			if (p->pos.Z + p->vel.Z  >= floorz - i) // hit the ground
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
					else if (p->vel.Z > 8) S_PlayActorSound(DUKE_LAND, pact);
				}
			}
		}
	}

	else
	{
		p->falling_counter = 0;
		S_StopSound(-1, pact, CHAN_VOICE);

		if (psectlotag != ST_1_ABOVE_WATER && psectlotag != ST_2_UNDERWATER && p->on_ground == 0 && p->vel.Z > 12)
			p->hard_landing = uint8_t(p->vel.Z / 4. );

		p->on_ground = 1;

		if (i == 40)
		{
			//Smooth on the ground

			double k = (floorz - i - p->pos.Z) * 0.5;
			if (abs(k) < 1) k = 0;
			p->pos.Z += k;
			p->vel.Z -= 3;
			if (p->vel.Z < 0) p->vel.Z = 0;
		}
		else if (p->jumping_counter == 0)
		{
			p->pos.Z += ((floorz - i * 0.5) - p->pos.Z) * 0.5; //Smooth on the water
			if (p->on_warping_sector == 0 && p->pos.Z > floorz - 16)
			{
				p->pos.Z = floorz - 16;
				p->vel.Z *= 0.5;
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
			playerJump(snum, floorz, ceilingz);
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
				p->vel.Z = -2;
			}
			else
			{
				p->vel.Z -= BobVal(2048 - 128 + p->jumping_counter) * (64. / 12);
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

	p->pos.Z += p->vel.Z ;

	if (p->pos.Z < ceilingz + 4)
	{
		p->jumping_counter = 0;
		if (p->vel.Z < 0)
			p->vel.X = p->vel.Y = 0;
		p->vel.Z = 0.5;
		p->pos.Z = ceilingz + 4;
	}
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

static void underwater(int snum, ESyncBits actions, double floorz, double ceilingz)
{
	auto p = &ps[snum];
	auto pact = p->GetActor();

	// under water
	p->jumping_counter = 0;

	p->pycount += 32;
	p->pycount &= 2047;
	p->pyoff = BobVal(p->pycount);

	if (!S_CheckActorSoundPlaying(pact, DUKE_UNDERWATER))
		S_PlayActorSound(DUKE_UNDERWATER, pact);

	if (actions & SB_JUMP)
	{
		// jump
		if (p->vel.Z > 0) p->vel.Z = 0;
		p->vel.Z -= (348 / 256.);
		if (p->vel.Z < -6) p->vel.Z = -6;
	}
	else if (actions & SB_CROUCH)
	{
		// crouch
		if (p->vel.Z < 0) p->vel.Z = 0;
		p->vel.Z += (348 / 256.);
		if (p->vel.Z > 6) p->vel.Z = 6;
	}
	else
	{
		// normal view
		if (p->vel.Z < 0)
		{
			p->vel.Z += 1;
			if (p->vel.Z > 0)
				p->vel.Z = 0;
		}
		if (p->vel.Z > 0)
		{
			p->vel.Z -= 1;
			if (p->vel.Z < 0)
				p->vel.Z = 0;
		}
	}

	if (p->vel.Z > 8)
		p->vel.Z *= 0.5;

	p->pos.Z += p->vel.Z ;

	if (p->pos.Z > floorz - 15)
		p->pos.Z += (((floorz - 15) - p->pos.Z) * 0.5);

	if (p->pos.Z < ceilingz + 4)
	{
		p->pos.Z = ceilingz + 4;
		p->vel.Z = 0;
	}

	if (p->scuba_on && (krand() & 255) < 8)
	{
		auto j = spawn(pact, WATERBUBBLE);
		if (j)
		{
			j->spr.pos += (p->angle.ang.ToVector() + DVector2(4 - (global_random & 8), 4 - (global_random & 8))) * 16;
			j->spr.SetScale(0.046875, 0.3125);
			j->spr.pos.Z = p->pos.Z + 8;
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

	hitscan(p->pos, p->cursector, DVector3(p->angle.ang.ToVector() * 1024, p->horizon.sum().Tan() * 16.), hit, CLIPMASK1);

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
		if (!actorflag(act, SFLAG_BLOCK_TRIPBOMB))
		{
			auto delta = act->spr.pos - hit.hitpos;
			if (abs(delta.Z) < 12 && delta.XY().LengthSquared() < (18.125 * 18.125))
				return 0;
		}
	}

	if (act == nullptr && hit.hitWall != nullptr && (hit.hitWall->cstat & CSTAT_WALL_MASKED) == 0)
		if ((hit.hitWall->twoSided() && hit.hitWall->nextSector()->lotag <= 2) || (!hit.hitWall->twoSided() && hit.hitSector->lotag <= 2))
		{
			auto delta = hit.hitpos.XY() - p->pos.XY();
			if (delta.LengthSquared() < (18.125 * 18.125))
			{
				p->pos.Z = p->opos.Z;
				p->vel.Z = 0;
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
			double zvel, vel;

			p->ammo_amount[HANDBOMB_WEAPON]--;

			if (p->on_ground && (actions & SB_CROUCH))
			{
				vel = 15/16.;
				zvel = p->horizon.sum().Tan() * 10.;
			}
			else
			{
				vel = 140/16.;
				zvel = -4 + p->horizon.sum().Tan() * 10.;
			}

			auto spawned = CreateActor(p->cursector, p->pos + p->angle.ang.ToVector() * 16, HEAVYHBOMB, -16, DVector2(0.140625, 0.140625),
				p->angle.ang, vel + p->hbomb_hold_delay * 2, zvel, pact, 1);

			if (isNam())
			{
				spawned->spr.extra = MulScale(krand(), NAM_GRENADE_LIFETIME_VAR, 14);
			}

			if (vel == 15 / 16.)
			{
				spawned->spr.yint = 3;
				spawned->spr.pos.Z += 8;
			}

			double hd = hits(pact);
			if (hd < 32)
			{
				spawned->spr.angle += DAngle180;
				spawned->vel *= 1./3.;
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
				j->spr.angle += DAngle180;
				ssp(j, CLIPMASK0);
				j->spr.angle -= DAngle180;
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
						j->spr.angle += DAngle180;
						j->vel.X += 2.;
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
			if (pact->spr.scale.X < 0.5)
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
	int shrunk = (pact->spr.scale.Y < 0.5);

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
	int k, doubvel;
	double floorz, ceilingz, truefdist;
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

	shrunk = (pact->spr.scale.Y < 0.5);
	getzrange(p->pos, psectp, &ceilingz, chz, &floorz, clz, 10.1875, CLIPMASK0);

	p->truefz = getflorzofslopeptr(psectp, p->pos);
	p->truecz = getceilzofslopeptr(psectp, p->pos);

	truefdist = abs(p->pos.Z - p->truefz);
	if (clz.type == kHitSector && psectlotag == 1 && truefdist > gs.playerheight + 16)
		psectlotag = 0;

	pact->floorz = floorz;
	pact->ceilingz = ceilingz;

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
			ceilingz = p->truecz;
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
		else if (badguy(clz.actor()) && clz.actor()->spr.scale.X > 0.375 && abs(pact->spr.pos.Z - clz.actor()->spr.pos.Z) < 84)
		{
			auto ang = (clz.actor()->spr.pos - p->pos).Angle();
			p->vel.XY() -= ang.ToVector();
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
		playerisdead(snum, psectlotag, floorz, ceilingz);
		return;
	}

	if (p->GetActor()->spr.scale.X < 0.625 && p->jetpack_on == 0)
	{
		p->ofistsign = p->fistsign;
		p->fistsign += p->GetActor()->vel.X * 16;
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
		p->vel.X = p->vel.Y = 0;
		pact->vel.X = 0;

		fi.doincrements(p);

		if (isWW2GI() && aplWeaponWorksLike(p->curr_weapon, snum) == HANDREMOTE_WEAPON) processweapon(snum, actions);
		if (!isWW2GI() && p->curr_weapon == HANDREMOTE_WEAPON) processweapon(snum, actions);
		return;
	}

	doubvel = TICSPERFRAME;

	checklook(snum,actions);
	double iif = 2.5;
	auto oldpos = p->opos;

	if (p->on_crane != nullptr)
		goto HORIZONLY;

	p->playerweaponsway(pact->vel.X);

	pact->vel.X = clamp((p->pos.XY() - p->bobpos).Length(), 0., 32.);
	if (p->on_ground) p->bobcounter += int(p->GetActor()->vel.X * 8);

	p->backuppos(ud.clipping == 0 && ((p->insector() && p->cursector->floorpicnum == MIRROR) || !p->insector()));

	// Shrinking code

	if (psectlotag == ST_2_UNDERWATER)
	{
		underwater(snum, actions, floorz, ceilingz);
	}

	else if (p->jetpack_on)
	{
		operateJetpack(snum, actions, psectlotag, floorz, ceilingz, shrunk);
	}
	else if (psectlotag != ST_2_UNDERWATER)
	{
		movement(snum, actions, psectp, floorz, ceilingz, shrunk, truefdist, psectlotag);
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

		if (p->on_ground && truefdist <= gs.playerheight + 16)
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

		k = int(BobVal(p->bobcounter) * 4);

		if (truefdist < gs.playerheight + 8 && (k == 1 || k == 3))
		{
			if (p->spritebridge == 0 && p->walking_snd_toggle == 0 && p->on_ground)
			{
				int j;
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

		p->vel.X += sb_fvel * doubvel * (5. / 16.);
		p->vel.Y += sb_svel * doubvel * (5. / 16.);

		bool check;

		if (!isWW2GI()) check = ((p->curr_weapon == KNEE_WEAPON && p->kickback_pic > 10 && p->on_ground) || (p->on_ground && (actions & SB_CROUCH)));
		else check = ((aplWeaponWorksLike(p->curr_weapon, snum) == KNEE_WEAPON && p->kickback_pic > 10 && p->on_ground) || (p->on_ground && (actions & SB_CROUCH)));
		if (check)
		{
			p->vel.XY() *= gs.playerfriction - 0.125;
		}
		else
		{
			if (psectlotag == 2)
			{
				p->vel.XY() *= gs.playerfriction - FixedToFloat(0x1400);
			}
			else
			{
				p->vel.XY() *= gs.playerfriction;
			}
		}

		if (abs(p->vel.X) < 1/128. && abs(p->vel.Y) < 1 / 128.)
			p->vel.X = p->vel.Y = 0;

		if (shrunk)
		{
			p->vel.XY() *= gs.playerfriction * 0.75;
		}
	}

HORIZONLY:

	if (psectlotag == 1 || p->spritebridge == 1) iif = 4;
	else iif = 20;

	if (p->insector() && p->cursector->lotag == 2) k = 0;
	else k = 1;

	Collision clip{};
	if (ud.clipping)
	{
		p->pos.XY() += p->vel.XY() ;
		updatesector(p->pos, &p->cursector);
		ChangeActorSect(pact, p->cursector);
	}
	else
		clipmove(p->pos, &p->cursector, p->vel, 10.25, 4., iif, CLIPMASK0, clip);

	if (p->jetpack_on == 0 && psectlotag != 2 && psectlotag != 1 && shrunk)
		p->pos.Z += 32;

	if (clip.type != kHitNone)
		checkplayerhurt_d(p, clip);

	if (p->jetpack_on == 0)
	{
		if (pact->vel.X > 1)
		{
			if (psectlotag != 1 && psectlotag != 2 && p->on_ground)
			{
				p->pycount += 52;
				p->pycount &= 2047;
				const double factor = 1024. / 1596; // What is 1596?
				p->pyoff = abs(pact->vel.X * BobVal(p->pycount)) * factor;
			}
		}
		else if (psectlotag != 2 && psectlotag != 1)
			p->pyoff = 0;
	}

	// RBG***
	SetActor(pact, p->pos.plusZ(gs.playerheight));

	if (psectlotag < 3)
	{
		psectp = pact->sector();
		if (ud.clipping == 0 && psectp->lotag == 31)
		{
			auto secact = barrier_cast<DDukeActor*>(psectp->hitagactor);
			if (secact && secact->vel.X != 0 && secact->temp_data[0] == 0)
			{
				quickkill(p);
				return;
			}
		}
	}

	if (truefdist < gs.playerheight && p->on_ground && psectlotag != 1 && shrunk == 0 && p->insector() && p->cursector->lotag == 1)
		if (!S_CheckActorSoundPlaying(pact, DUKE_ONWATER))
			S_PlayActorSound(DUKE_ONWATER, pact);

	if (p->cursector != pact->sector())
		ChangeActorSect(pact, p->cursector);

	int retry = 0;
	while (ud.clipping == 0)
	{
		int blocked;
		blocked = (pushmove(p->pos, &p->cursector, 10.25, 4, 4, CLIPMASK0) < 0 && furthestangle(p->GetActor(), 8) < DAngle90);

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
		else if (abs(floorz - ceilingz) < 32 && isanunderoperator(psectp->lotag))
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
