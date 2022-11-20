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

void dojaildoor();
void moveminecart();

void ballreturn(DDukeActor* spr);
void pinsectorresetdown(sectortype* sect);
int pinsectorresetup(sectortype* sect);
int checkpins(sectortype* sect);
void resetpins(sectortype* sect);
void resetlanepics(void);


//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

bool ceilingspace_r(sectortype* sectp)
{
	if (sectp && (sectp->ceilingstat & CSTAT_SECTOR_SKY) && sectp->ceilingpal == 0)
	{
		switch(sectp->ceilingpicnum)
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

bool floorspace_r(sectortype* sectp)
{
	if (sectp && (sectp->floorstat & CSTAT_SECTOR_SKY) && sectp->ceilingpal == 0)
	{
		switch(sectp->floorpicnum)
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

void check_fta_sounds_r(DDukeActor* actor)
{
	if (actor->spr.extra > 0) switch (actor->spr.picnum)
	{
	case COOT: // LIZTROOP
		if (!isRRRA() && (krand() & 3) == 2)
			S_PlayActorSound(PRED_RECOG, actor);
		break;
	case BILLYCOCK:
	case BILLYRAY:
	case BRAYSNIPER: // PIGCOP
		S_PlayActorSound(PIG_RECOG, actor);
		break;
	}
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void addweapon_r(player_struct* p, int weapon, bool wswitch)
{
	int cw = p->curr_weapon;
	if (p->OnMotorcycle || p->OnBoat)
	{
		p->gotweapon[weapon] = true;;
		if (weapon == THROWSAW_WEAPON)
		{
			p->gotweapon[BUZZSAW_WEAPON] = true;
			p->ammo_amount[BUZZSAW_WEAPON] = 1;
		}
		else if (weapon == CROSSBOW_WEAPON)
		{
			p->gotweapon[CHICKEN_WEAPON] = true;
			p->gotweapon[DYNAMITE_WEAPON] = true;
		}
		else if (weapon == SLINGBLADE_WEAPON)
		{
			p->ammo_amount[SLINGBLADE_WEAPON] = 1;
		}
		return;
	}

	if (p->gotweapon[weapon] == 0)
	{
		p->gotweapon[weapon] = true;;
		if (weapon == THROWSAW_WEAPON)
		{
			p->gotweapon[BUZZSAW_WEAPON] = true;
			p->ammo_amount[BUZZSAW_WEAPON] = 1;
		}
		if (isRRRA())
		{
			if (weapon == CROSSBOW_WEAPON)
			{
				p->gotweapon[CHICKEN_WEAPON] = true;
			}
			if (weapon == SLINGBLADE_WEAPON)
			{
				p->ammo_amount[SLINGBLADE_WEAPON] = 50;
			}
		}
		if (weapon == CROSSBOW_WEAPON)
		{
			p->gotweapon[DYNAMITE_WEAPON] = true;
		}

		if (weapon != DYNAMITE_WEAPON)
			cw = weapon;
	}
	else
		cw = weapon;

	if (!wswitch) return;

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

	p->okickback_pic = p->kickback_pic = 0;
	p->curr_weapon = cw;
	p->wantweaponfire = -1;

	switch (weapon)
	{
	case SLINGBLADE_WEAPON:
		if (!isRRRA()) break;
	case KNEE_WEAPON:
	case DYNAMITE_WEAPON:	 
	case TRIPBOMB_WEAPON:
	case THROWINGDYNAMITE_WEAPON:
		break;
	case SHOTGUN_WEAPON:	  
		S_PlayActorSound(SHOTGUN_COCK, p->GetActor()); 
		break;
	case PISTOL_WEAPON:	   
		S_PlayActorSound(INSERT_CLIP, p->GetActor());
		break;
	default:	  
		S_PlayActorSound(EJECT_CLIP, p->GetActor());
		break;
	}
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void hitradius_r(DDukeActor* actor, int  r, int  hp1, int  hp2, int  hp3, int  hp4)
{
	double radius = r * inttoworld;
	static const uint8_t statlist[] = { STAT_DEFAULT, STAT_ACTOR, STAT_STANDABLE, STAT_PLAYER, STAT_FALLER, STAT_ZOMBIEACTOR, STAT_MISC };

	if (actor->spr.scale.X >= 0.17675 || !(actor->spr.picnum == RPG || ((isRRRA()) && actor->spr.picnum == RPG2)))
	{
		BFSSectorSearch search(actor->sector());

		while (auto dasectp = search.GetNext())
		{
			if ((dasectp->ceilingz- actor->spr.pos.Z) < radius * 16) // what value range is this supposed to be? The check that was here did not multiply correctly
			{
				auto wal = dasectp->walls.Data();
				double d = (wal->pos - actor->spr.pos.XY()).Sum();
				if (d < radius)
					fi.checkhitceiling(dasectp);
				else
				{
					auto thirdpoint = wal->point2Wall()->point2Wall();
					d = (thirdpoint->pos - actor->spr.pos.XY()).Sum();
					if (d < radius)
						fi.checkhitceiling(dasectp);
				}
			}

			for (auto& wal : dasectp->walls)
			{
				if ((wal.pos - actor->spr.pos.XY()).Sum() < radius)
				{
					if (wal.twoSided())
					{
						search.Add(wal.nextSector());
					}
					DVector3 w1(((wal.pos + wal.point2Wall()->pos) * 0.5 + actor->spr.pos) * 0.5, actor->spr.pos.Z); // half way between the actor and the wall's center.
					sectortype* sect = wal.sectorp();
					updatesector(w1, &sect);

					if (sect && cansee(w1, sect, actor->spr.pos, actor->sector()))
						fi.checkhitwall(actor, &wal, DVector3(wal.pos, actor->spr.pos.Z), actor->spr.picnum);
				}
			}
		}
	}

	double q = zrand(32) - 24;

	auto Owner = actor->GetOwner();
	for (int x = 0; x < 7; x++)
	{
		DukeStatIterator it1(statlist[x]);
		while (auto act2 = it1.Next())
		{
			if (x == 0 || x >= 5 || actorflag(act2, SFLAG_HITRADIUS_FLAG1))
			{
				if (act2->spr.cstat & CSTAT_SPRITE_BLOCK_ALL)
					if ((actor->spr.pos - act2->spr.pos).Length() < radius)
					{
						if (badguy(act2) && !cansee(act2->spr.pos.plusZ(q), act2->sector(), actor->spr.pos.plusZ(q), actor->sector()))
							continue;

						fi.checkhitsprite(act2, actor);
					}
			}
			else if (act2->spr.extra >= 0 && act2 != actor && (actorflag(act2, SFLAG_HITRADIUS_FLAG2) || badguy(act2) || (act2->spr.cstat & CSTAT_SPRITE_BLOCK_ALL)))
			{
				if (actor->spr.picnum == MORTER && act2 == Owner)
				{
					continue;
				}
				if ((isRRRA()) && actor->spr.picnum == CHEERBOMB && act2 == Owner)
				{
					continue;
				}

				if (act2->spr.picnum == APLAYER) act2->spr.pos.Z -= gs.playerheight;
				double dist = (actor->spr.pos - act2->spr.pos).Length();
				if (act2->spr.picnum == APLAYER) act2->spr.pos.Z += gs.playerheight;

				if (dist < radius && cansee(act2->spr.pos.plusZ(-8), act2->sector(), actor->spr.pos.plusZ(-12), actor->sector()))
				{
					if ((isRRRA()) && act2->spr.picnum == MINION && act2->spr.pal == 19)
					{
						continue;
					}

					act2->hitang = (act2->spr.pos - actor->spr.pos).Angle();

					if (actor->spr.picnum == RPG && act2->spr.extra > 0)
						act2->attackertype = RPG;
					else if ((isRRRA()) && actor->spr.picnum == RPG2 && act2->spr.extra > 0)
						act2->attackertype = RPG;
					else
						act2->attackertype = RADIUSEXPLOSION;

					if (dist < radius / 3)
					{
						if (hp4 == hp3) hp4++;
						act2->hitextra = hp3 + (krand() % (hp4 - hp3));
					}
					else if (dist < 2 * radius / 3)
					{
						if (hp3 == hp2) hp3++;
						act2->hitextra = hp2 + (krand() % (hp3 - hp2));
					}
					else if (dist < radius)
					{
						if (hp2 == hp1) hp2++;
						act2->hitextra = hp1 + (krand() % (hp2 - hp1));
					}

					int pic = act2->spr.picnum;
					if ((isRRRA())? 
						(pic != HULK && pic != MAMA && pic != BILLYPLAY && pic != COOTPLAY && pic != MAMACLOUD) :
						(pic != HULK && pic != SBMOVE))
					{
						if (act2->vel.X < 0) act2->vel.X = 0;
						act2->vel.X += ((actor->spr.extra / 4.));
					}

					if (actorflag(act2, SFLAG_HITRADIUSCHECK))
						fi.checkhitsprite(act2, actor);

					if (act2->spr.picnum != RADIUSEXPLOSION &&
						Owner && Owner->spr.statnum < MAXSTATUS)
					{
						if (act2->spr.picnum == APLAYER)
						{
							int p = act2->PlayerIndex();
							if (ps[p].newOwner != nullptr)
							{
								clearcamera(&ps[p]);
							}
						}
						act2->SetHitOwner(actor->GetOwner());
					}
				}
			}
		}
	}
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

int movesprite_ex_r(DDukeActor* actor, const DVector3& change, unsigned int cliptype, Collision &result)
{
	int bg = badguy(actor);

	if (actor->spr.statnum == 5 || (bg && actor->spr.scale.X < 0.0625))
	{
		actor->spr.pos += change;
		if (bg)
			SetActor(actor, actor->spr.pos);
		return result.setNone();
	}

	auto dasectp = actor->sector();

	auto ppos = actor->spr.pos;
	ppos.Z -= (tileHeight(actor->spr.picnum) * actor->spr.scale.Y) * 0.5;

	if (bg)
	{
		if (actor->spr.scale.X > 0.9375 )
			clipmove(ppos, &dasectp, change * 0.5, 64., 4., 4., cliptype, result);
		else
		{
			clipmove(ppos, &dasectp, change * 0.5, 12., 4., 4., cliptype, result);
		}

		if (dasectp == nullptr || (dasectp != nullptr && actor->actorstayput != nullptr && actor->actorstayput != dasectp))
		{
			if (dasectp && dasectp->lotag == ST_1_ABOVE_WATER)
				actor->spr.angle = randomAngle();
			else if ((actor->temp_data[0] & 3) == 1)
				actor->spr.angle = randomAngle();
			SetActor(actor, actor->spr.pos);
			if (dasectp == nullptr) dasectp = &sector[0];
			return result.setSector(dasectp);
		}
		if ((result.type == kHitWall || result.type == kHitSprite) && (actor->cgg == 0)) actor->spr.angle += DAngle45 + DAngle90;
	}
	else
	{
		if (actor->spr.statnum == STAT_PROJECTILE)
			clipmove(ppos, &dasectp, change * 0.5, 0.5, 4., 4., cliptype, result);
		else
			clipmove(ppos, &dasectp, change * 0.5, actor->clipdist, 4., 4., cliptype, result);
	}
	actor->spr.pos.XY() = ppos.XY();

	if (dasectp)
		if ((dasectp != actor->sector()))
			ChangeActorSect(actor, dasectp);
	double daz = actor->spr.pos.Z + change.Z * 0.5;
	if (daz > actor->ceilingz && daz <= actor->floorz)
		actor->spr.pos.Z = daz;
	else if (result.type == kHitNone)
		return result.setSector(dasectp);

	return result.type;
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void lotsoffeathers_r(DDukeActor *actor, int n)
{
	lotsofstuff(actor, n, MONEY);
}


//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void guts_r(DDukeActor* actor, int gtype, int n, int p)
{
	int j;
	double scale;
	uint8_t pal;

	if (badguy(actor) && actor->spr.scale.X < 0.25)
		scale = 0.0625;
	else scale = 0.25;

	double gutz = actor->spr.pos.Z - 8;
	double floorz = getflorzofslopeptr(actor->sector(), actor->spr.pos);

	if (gutz > floorz - 8)
		gutz = floorz - 8;

	gutz += gs.actorinfo[actor->spr.picnum].gutsoffset;

	if (badguy(actor) && actor->spr.pal == 6)
		pal = 6;
	else
	{
		pal = 0;
		if (isRRRA())
		{
			if (actor->spr.picnum == MINION && (actor->spr.pal == 8 || actor->spr.pal == 19)) pal = actor->spr.pal;
		}
	}

	for (j = 0; j < n; j++)
	{
		// RANDCORRECT version from RR.
		DAngle a = randomAngle();
		double zvel = -2 - krandf(8);
		double vel = 3 + krandf(2);
		DVector3 offs;
		offs.Z = gutz - krandf(16);
		offs.Y = krandf(16) - 8;
		offs.X = krandf(16) - 8;
		// TRANSITIONAL: owned by a player???
		auto spawned = CreateActor(actor->sector(), offs + actor->spr.pos.XY(), gtype, -32, DVector2(scale, scale), a, vel, zvel, ps[p].GetActor(), 5);
		if (spawned && pal != 0)
			spawned->spr.pal = pal;
	}
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

int ifhitbyweapon_r(DDukeActor *actor)
{
	int p;
	auto hitowner = actor->GetHitOwner();

	if (actor->hitextra >= 0)
	{
		if (actor->spr.extra >= 0)
		{
			if (actor->spr.picnum == APLAYER)
			{
				if (ud.god) return -1;

				p = actor->PlayerIndex();

				if (hitowner &&
					hitowner->spr.picnum == APLAYER &&
					ud.coop == 1 &&
					ud.ffire == 0)
					return -1;

				actor->spr.extra -= actor->hitextra;

				if (hitowner)
				{
					if (actor->spr.extra <= 0 && actor->attackertype != FREEZEBLAST)
					{
						actor->spr.extra = 0;

						ps[p].wackedbyactor = hitowner;

						if (hitowner->spr.picnum == APLAYER && p != hitowner->PlayerIndex())
						{
							ps[p].frag_ps = hitowner->PlayerIndex();
						}
						actor->SetHitOwner(ps[p].GetActor());
					}
				}

				if (attackerflag(actor, SFLAG2_DOUBLEDMGTHRUST))
				{
					ps[p].vel.XY() += actor->hitang.ToVector() * actor->hitextra * 0.25;
				}
				else
				{
					ps[p].vel.XY() += actor->hitang.ToVector() * actor->hitextra * 0.125;
				}
			}
			else
			{
				if (actor->hitextra == 0)
					if (actor->spr.scale.X < 0.375)
						return -1;

				actor->spr.extra -= actor->hitextra;
				if (actor->spr.picnum != RECON && actor->GetOwner() && actor->GetOwner()->spr.statnum < MAXSTATUS)
					actor->SetOwner(hitowner);
			}

			actor->hitextra = -1;
			return actor->attackertype;
		}
	}

	actor->hitextra = -1;
	return -1;
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void movefallers_r(void)
{
	DukeStatIterator it(STAT_FALLER);
	while (auto act = it.Next())
	{
		auto sectp = act->sector();

		if (act->temp_data[0] == 0)
		{
			act->spr.pos.Z -= 16;
			DAngle saved_angle = act->spr.angle;
			int x = act->spr.extra;
			int j = fi.ifhitbyweapon(act);
			if (j >= 0)
			{
				if (gs.actorinfo[j].flags2 & SFLAG2_EXPLOSIVE)
				{
					if (act->spr.extra <= 0)
					{
						act->temp_data[0] = 1;
						DukeStatIterator itr(STAT_FALLER);
						while (auto ac2 = itr.Next())
						{
							if (ac2->spr.hitag == act->spr.hitag)
							{
								ac2->temp_data[0] = 1;
								ac2->spr.cstat &= ~CSTAT_SPRITE_ONE_SIDE;
								if (ac2->spr.picnum == CEILINGSTEAM || ac2->spr.picnum == STEAM)
									ac2->spr.cstat |= CSTAT_SPRITE_INVISIBLE;
							}
						}
					}
				}
				else
				{
					act->hitextra = 0;
					act->spr.extra = x;
				}
			}
			act->spr.angle = saved_angle;
			act->spr.pos.Z += 16;
		}
		else if (act->temp_data[0] == 1)
		{
			if (act->spr.lotag > 0)
			{
				act->spr.lotag -= 3;
				act->vel.X = 4 + krandf(8);
				act->vel.Z = -4 + krandf(4);
			}
			else
			{
				if (act->vel.X > 0)
				{
					act->vel.X -= 1/8.;
					ssp(act, CLIPMASK0);
				}

				double grav;
				if (fi.floorspace(act->sector())) grav = 0;
				else
				{
					if (fi.ceilingspace(act->sector()))
						grav = gs.gravity / 6;
					else
						grav = gs.gravity;
				}

				if (act->spr.pos.Z < sectp->floorz - 1)
				{
					act->vel.Z += grav;
					if (act->vel.Z > 24)
						act->vel.Z = 24;
					act->spr.pos.Z += act->vel.Z;
				}
				if ((sectp->floorz - act->spr.pos.Z) < 16)
				{
					int j = 1 + (krand() & 7);
					for (int x = 0; x < j; x++) RANDOMSCRAP(act);
					act->Destroy();
				}
			}
		}
	}
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void movestandables_r(void)
{
	DukeStatIterator it(STAT_STANDABLE);
	while (auto act = it.Next())
	{
		int picnum = act->spr.picnum;

		if (!act->insector() || actorflag(act, SFLAG2_DIENOW))
		{
			act->Destroy();
			continue;
		}

		if (act->GetClass() != RUNTIME_CLASS(DDukeActor))
		{
			CallTick(act);
			continue;
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
			double x;
			int p = findplayer(act, &x);
			execute(act, p, x);
		}
	}
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

static void chickenarrow(DDukeActor* actor)
{
	actor->spr.hitag++;
	if (actor->attackertype != BOSS2 && actor->spr.scale.X >= 0.15625 && actor->sector()->lotag != 2)
	{
		auto spawned = spawn(actor, SMALLSMOKE);
		if (spawned) spawned->spr.pos.Z += 1;
		if ((krand() & 15) == 2)
		{
			spawn(actor, MONEY);
		}
	}
	DDukeActor* ts = actor->seek_actor;
	if (!ts) return;

	if (ts->spr.extra <= 0)
		actor->seek_actor = nullptr;

	if (actor->seek_actor && actor->spr.hitag > 5)
	{
		DAngle ang, ang2;
		ang = (ts->spr.pos - actor->spr.pos).Angle();
		ang2 = deltaangle(ang, actor->spr.angle);
		// this was quite broken in the original code. Fixed so that it seeks properly
		if (abs(ang2) < DAngle1 * 17.5)
		{
			actor->spr.angle = ang;
		}
		else if (ang2 > nullAngle)
		{
			actor->spr.angle -= DAngle1 * 9;
		}
		else
			actor->spr.angle += DAngle1 * 9;

		if (actor->spr.hitag > 180)
			if (actor->vel.Z <= 0)
				actor->vel.Z += 200 / 256;
	}
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

static bool weaponhitsprite(DDukeActor *proj, DDukeActor *targ, const DVector3 &oldpos)
{
	if (isRRRA())
	{
		if (targ->spr.picnum == MINION
			&& (proj->spr.picnum == RPG || proj->spr.picnum == RPG2)
			&& targ->spr.pal == 19)
		{
			S_PlayActorSound(RPG_EXPLODE, proj);
			auto spawned = spawn(proj, EXPLOSION2);
			if (spawned)
				spawned->spr.pos = oldpos;
			return true;
		}
	}
	else if (proj->spr.picnum == FREEZEBLAST && targ->spr.pal == 1)
		if (badguy(targ) || targ->spr.picnum == APLAYER)
		{
			auto star = spawn(proj, TRANSPORTERSTAR);
			if (star)
			{
				star->spr.pal = 1;
				star->spr.scale = DVector2(0.5, 0.5);
			}

			proj->Destroy();
			return true;
		}

	fi.checkhitsprite(targ, proj);

	if (targ->spr.picnum == APLAYER)
	{
		int p = targ->PlayerIndex();
		S_PlayActorSound(PISTOL_BODYHIT, targ);

		if (proj->spr.picnum == SPIT)
		{
			if (isRRRA() && proj->GetOwner() && proj->GetOwner()->spr.picnum == MAMA)
			{
				guts_r(proj, RABBITJIBA, 2, myconnectindex);
				guts_r(proj, RABBITJIBB, 2, myconnectindex);
				guts_r(proj, RABBITJIBC, 2, myconnectindex);
			}

			ps[p].horizon.addadjustment(DAngle::fromDeg(-14.04));
			ps[p].sync.actions |= SB_CENTERVIEW;

			if (ps[p].loogcnt == 0)
			{
				if (!S_CheckActorSoundPlaying(ps[p].GetActor(), DUKE_LONGTERM_PAIN))
					S_PlayActorSound(DUKE_LONGTERM_PAIN, ps[p].GetActor());

				int j = 3 + (krand() & 3);
				ps[p].numloogs = j;
				ps[p].loogcnt = 24 * 4;
				for (int x = 0; x < j; x++)
				{
					ps[p].loogie[x].X = krand() % 320;
					ps[p].loogie[x].Y = krand() % 200;
				}
			}
		}
	}
	return false;
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

static bool weaponhitwall(DDukeActor *proj, walltype* wal, const DVector3& oldpos)
{
	if (isRRRA() && proj->GetOwner() && proj->GetOwner()->spr.picnum == MAMA)
	{
		guts_r(proj, RABBITJIBA, 2, myconnectindex);
		guts_r(proj, RABBITJIBB, 2, myconnectindex);
		guts_r(proj, RABBITJIBC, 2, myconnectindex);
	}

	if (proj->spr.picnum != RPG && (!isRRRA() || proj->spr.picnum != RPG2) && proj->spr.picnum != FREEZEBLAST && proj->spr.picnum != SPIT && proj->spr.picnum != SHRINKSPARK && (wal->overpicnum == MIRROR || wal->picnum == MIRROR))
	{
		DAngle walang = wal->delta().Angle();
		proj->spr.angle = walang * 2 - proj->spr.angle;
		proj->SetOwner(proj);
		spawn(proj, TRANSPORTERSTAR);
		return true;
	}
	else
	{
		SetActor(proj, oldpos);
		fi.checkhitwall(proj, wal, proj->spr.pos, proj->spr.picnum);

		if (!isRRRA() && proj->spr.picnum == FREEZEBLAST)
		{
			if (wal->overpicnum != MIRROR && wal->picnum != MIRROR)
			{
				proj->spr.extra >>= 1;
				if (proj->spr.scale.X > 0.125 )
					proj->spr.scale.X += (-0.03125);
				if (proj->spr.scale.Y > 0.125 )
					proj->spr.scale.Y += (-0.03125);
				proj->spr.yint--;
			}

			DAngle walang = wal->delta().Angle();
			proj->spr.angle = walang * 2 - proj->spr.angle;
			return true;
		}
		if (proj->spr.picnum == SHRINKSPARK)
		{
			if (wal->picnum >= RRTILE3643 && wal->picnum < RRTILE3643 + 3)
			{
				proj->Destroy();
			}
			if (proj->spr.extra <= 0)
			{
				proj->spr.pos += proj->spr.angle.ToVector() * 8;
				auto Owner = proj->GetOwner();
				if (!isRRRA() || !Owner || (Owner->spr.picnum != CHEER && Owner->spr.picnum != CHEERSTAYPUT))
				{
					auto j = spawn(proj, CIRCLESTUCK);
					if (j)
					{
						j->spr.scale = DVector2(0.125, 0.125);
						j->spr.cstat = CSTAT_SPRITE_ALIGNMENT_WALL;
						j->spr.angle += DAngle90;
						j->clipdist = proj->spr.scale.X * tileWidth(proj->spr.picnum) * 0.125;
					}
				}
				proj->Destroy();
				return true;
			}
			if (wal->overpicnum != MIRROR && wal->picnum != MIRROR)
			{
				proj->spr.extra -= 20;
				proj->spr.yint--;
			}

			DAngle k = wal->delta().Angle();
			proj->spr.angle = k * 2 - proj->spr.angle;
			return true;
		}
	}
	return false;
}


//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

bool weaponhitsector(DDukeActor *proj, const DVector3& oldpos)
{
	SetActor(proj, oldpos);

	if (isRRRA() && proj->GetOwner() && proj->GetOwner()->spr.picnum == MAMA)
	{
		guts_r(proj, RABBITJIBA, 2, myconnectindex);
		guts_r(proj, RABBITJIBB, 2, myconnectindex);
		guts_r(proj, RABBITJIBC, 2, myconnectindex);
	}

	if (proj->vel.Z < 0)
	{
		if (proj->sector()->ceilingstat & CSTAT_SECTOR_SKY)
			if (proj->sector()->ceilingpal == 0)
			{
				proj->Destroy();
				return true;
			}

		fi.checkhitceiling(proj->sector());
	}

	if (!isRRRA() && proj->spr.picnum == FREEZEBLAST)
	{
		bounce(proj);
		ssp(proj, CLIPMASK1);
		proj->spr.extra >>= 1;
		if (proj->spr.scale.X > 0.125 )
			proj->spr.scale.X += (-0.03125);
		if (proj->spr.scale.Y > 0.125 )
			proj->spr.scale.Y += (-0.03125);
		proj->spr.yint--;
		return true;
	}
	return false;
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

static void weaponcommon_r(DDukeActor *proj)
{
	double vel = proj->vel.X;
	double velz = proj->vel.Z;

	int p = -1;

	if (proj->spr.picnum == RPG && proj->sector()->lotag == 2)
	{
		vel *= 0.5;
		velz *= 0.5;
	}
	else if (isRRRA() && proj->spr.picnum == RPG2 && proj->sector()->lotag == 2)
	{
		vel *= 0.5;
		velz *= 0.5;
	}


	auto oldpos = proj->spr.pos;

	getglobalz(proj);

	switch (proj->spr.picnum)
	{
	case RPG:
		if (proj->attackertype != BOSS2 && proj->spr.scale.X >= 0.15625 && proj->sector()->lotag != 2)
		{
			auto spawned = spawn(proj, SMALLSMOKE);
			if (spawned) spawned->spr.pos.Z += 1;
		}
		break;
	case RPG2:
		if (!isRRRA()) break;
		chickenarrow(proj);
		break;

	case RRTILE1790:
		if (!isRRRA()) break;
		if (proj->spr.extra)
		{
			proj->vel.Z = -(proj->spr.extra * 250/256.); // 250 looks like a typo...
			proj->spr.extra--;
		}
		else
			makeitfall(proj);
		if (proj->spr.scale.X >= 0.15625 && proj->sector()->lotag != 2)
		{
			auto spawned = spawn(proj, SMALLSMOKE);
			if (spawned) spawned->spr.pos.Z += 1;
		}
		break;
	}

	Collision coll;
	movesprite_ex(proj, DVector3(proj->spr.angle.ToVector() * vel, velz), CLIPMASK1, coll);

	if ((proj->spr.picnum == RPG || (isRRRA() && isIn(proj->spr.picnum, RPG2, RRTILE1790))) && proj->temp_actor != nullptr)
		if ((proj->spr.pos.XY() - proj->temp_actor->spr.pos.XY()).Length() < 16)
			coll.setSprite(proj->temp_actor);

	if (!proj->insector()) // || (isRR() && proj->sector()->filler == 800))
	{
		proj->Destroy();
		return;
	}

	if (coll.type != kHitSprite && proj->spr.picnum != FREEZEBLAST)
	{
		if (proj->spr.pos.Z < proj->ceilingz)
		{
			coll.setSector(proj->sector());
			proj->vel.Z -= 1/256.;
		}
		else
			if (proj->spr.pos.Z > proj->floorz)
			{
				coll.setSector(proj->sector());
				if (proj->sector()->lotag != 1)
					proj->vel.Z += 1/256.;
			}
	}

	if (proj->spr.picnum == FIRELASER)
	{
		for (int k = -3; k < 2; k++)
		{
			double zAdd = k * proj->vel.Z / 24;
			auto x = CreateActor(proj->sector(), proj->spr.pos.plusZ(zAdd) + proj->spr.angle.ToVector() * k * 2.,
				FIRELASER, -40 + (k << 2),
				proj->spr.scale, nullAngle, 0., 0., proj->GetOwner(), 5);

			if (x)
			{
				x->spr.cstat = CSTAT_SPRITE_YCENTER;
				x->spr.pal = proj->spr.pal;
			}
		}
	}
	else if (proj->spr.picnum == SPIT) if (proj->vel.Z < 24)
		proj->vel.Z += gs.gravity - 112/256.;

	if (coll.type != 0)
	{
		if (coll.type == kHitSprite)
		{
			if (weaponhitsprite(proj, coll.actor(), oldpos)) return;
		}
		else if (coll.type == kHitWall)
		{
			if (weaponhitwall(proj, coll.hitWall, oldpos)) return;
		}
		else if (coll.type == kHitSector)
		{
			if (weaponhitsector(proj, oldpos)) return;
		}

		if (proj->spr.picnum != SPIT)
		{
			if (proj->spr.picnum == RPG) rpgexplode(proj, coll.type, oldpos, EXPLOSION2, -1, -1, RPG_EXPLODE);
			else if (isRRRA() && proj->spr.picnum == RPG2) rpgexplode(proj, coll.type, oldpos, EXPLOSION2, -1,  150, 247);
			else if (isRRRA() && proj->spr.picnum == RRTILE1790) rpgexplode(proj, coll.type, oldpos, EXPLOSION2, -1,  160, RPG_EXPLODE);
			else if (proj->spr.picnum != FREEZEBLAST && proj->spr.picnum != FIRELASER && proj->spr.picnum != SHRINKSPARK)
			{
				auto spawned = spawn(proj, 1441);
				if (spawned)
				{
					auto scale = proj->spr.scale.X * 0.5;
					spawned->spr.scale = DVector2(scale, scale);
					if (coll.type == kHitSector)
					{
						if (proj->vel.Z < 0)
						{
							spawned->spr.cstat |= CSTAT_SPRITE_YFLIP;
							spawned->spr.pos.Z += 72;
						}
					}
				}
			}
		}
		proj->Destroy();
		return;
	}
	if ((proj->spr.picnum == RPG || (isRRRA() && proj->spr.picnum == RPG2)) && proj->sector()->lotag == 2 && proj->spr.scale.X >= 0.15625 && rnd(184))
		spawn(proj, WATERBUBBLE);

}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void moveweapons_r(void)
{
	DukeStatIterator it(STAT_PROJECTILE);
	while (auto proj = it.Next())
	{
		if (!proj->insector() || actorflag(proj, SFLAG2_DIENOW))
		{
			proj->Destroy();
			continue;
		}

		switch (proj->spr.picnum)
		{
		case TONGUE:
			movetongue(proj, TONGUE, INNERJAW);
			continue;

		case FREEZEBLAST:
			if (proj->spr.yint < 1 || proj->spr.extra < 2 || (proj->vel.X == 0 && proj->vel.Z == 0))
			{
				auto star = spawn(proj, TRANSPORTERSTAR);
				if (star)
				{
					star->spr.pal = 1;
					star->spr.scale = DVector2(0.5, 0.5);
				}
				proj->Destroy();
				continue;
			}
			[[fallthrough]];
		case RPG2:
		case RRTILE1790:
			if (!isRRRA()) continue;
			[[fallthrough]];
		case SHRINKSPARK:
		case RPG:
		case FIRELASER:
		case SPIT:
		case COOLEXPLOSION1:
		case OWHIP:
		case UWHIP:
			weaponcommon_r(proj);
			continue;


		case SHOTSPARK1:
		{
			double x;
			int p = findplayer(proj, &x);
			execute(proj, p, x);
			continue;
		}
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
	uint8_t warpdir = 0, warpspriteto;
	int k, p, sectlotag;
	int onfloorz;
	double ll, ll2 = 0;
	Collision coll;

	 //Transporters

	DukeStatIterator iti(STAT_TRANSPORT);
	while (auto act = iti.Next())
	{
		auto sectp = act->sector();
		sectlotag = sectp->lotag;

		auto Owner = act->GetOwner();
		if (Owner == act || Owner == nullptr)
		{
			continue;
		}

		onfloorz = act->temp_data[4];

		if (act->temp_data[0] > 0) act->temp_data[0]--;

		DukeSectIterator itj(act->sector());
		while (auto act2 = itj.Next())
		{
			switch (act2->spr.statnum)
			{
			case STAT_PLAYER:	// Player

				if (act2->GetOwner())
				{
					p = act2->PlayerIndex();

					ps[p].on_warping_sector = 1;

					if (ps[p].transporter_hold == 0 && ps[p].jumping_counter == 0)
					{
						if (ps[p].on_ground && sectlotag == 0 && onfloorz && ps[p].jetpack_on == 0)
						{
							spawn(act,  TRANSPORTERBEAM);
							S_PlayActorSound(TELEPORTER, act);

							for (k = connecthead; k >= 0; k = connectpoint2[k])
								if (ps[k].cursector == Owner->sector())
								{
									ps[k].frag_ps = p;
									ps[k].GetActor()->spr.extra = 0;
								}

							ps[p].angle.ang = Owner->spr.angle;

							if (Owner->GetOwner() != Owner)
							{
								act->temp_data[0] = 13;
								Owner->temp_data[0] = 13;
								ps[p].transporter_hold = 13;
							}

							ps[p].pos = Owner->spr.pos.plusZ(-gs.playerheight + 4);
							ps[p].backupxyz();
							ps[p].setbobpos();

							ChangeActorSect(act2, Owner->sector());
							ps[p].setCursector(act2->sector());

							auto beam = spawn(Owner, TRANSPORTERBEAM);
							if (beam) S_PlayActorSound(TELEPORTER, beam);

							break;
						}
					}
					else break;

					if (onfloorz == 0 && fabs(act->spr.pos.Z - ps[p].pos.Z) < 24)
						if ((ps[p].jetpack_on == 0) || (ps[p].jetpack_on && PlayerInput(p, SB_JUMP)) ||
							(ps[p].jetpack_on && PlayerInput(p, SB_CROUCH)))
						{
							ps[p].pos.X += Owner->spr.pos.X - act->spr.pos.X;
							ps[p].pos.Y += Owner->spr.pos.Y - act->spr.pos.Y;
							ps[p].backupxy();

							if (ps[p].jetpack_on && (PlayerInput(p, SB_JUMP) || ps[p].jetpack_on < 11))
								ps[p].pos.Z = Owner->spr.pos.Z - 24;
							else ps[p].pos.Z = Owner->spr.pos.Z + 24;
							ps[p].backupz();

							ChangeActorSect(act2, Owner->sector());
							ps[p].setCursector(Owner->sector());

							break;
						}

					k = 0;

					if (isRRRA())
					{
						if (onfloorz && sectlotag == 160 && ps[p].pos.Z > sectp->floorz - 48)
						{
							k = 2;
							ps[p].pos.Z = Owner->sector()->ceilingz + 7;
							ps[p].backupz();
						}

						if (onfloorz && sectlotag == 161 && ps[p].pos.Z < sectp->ceilingz + 6)
						{
							k = 2;
							if (ps[p].GetActor()->spr.extra <= 0) break;
							ps[p].pos.Z = Owner->sector()->floorz - 49;
							ps[p].backupz();
						}
					}

					if ((onfloorz && sectlotag == ST_1_ABOVE_WATER && ps[p].pos.Z > sectp->floorz - 6) ||
						(onfloorz && sectlotag == ST_1_ABOVE_WATER && ps[p].OnMotorcycle))
					{
						if (ps[p].OnBoat) break;
						k = 1;
						if (screenpeek == p)
						{
							FX_StopAllSounds();
						}
						S_PlayActorSound(DUKE_UNDERWATER, ps[p].GetActor());
						ps[p].pos.Z = Owner->sector()->ceilingz + 7;
						ps[p].backupz();
						if (ps[p].OnMotorcycle)
							ps[p].moto_underwater = 1;
					}

					if (onfloorz && sectlotag == ST_2_UNDERWATER && ps[p].pos.Z < sectp->ceilingz + 6)
					{
						k = 1;
						if (ps[p].GetActor()->spr.extra <= 0) break;
						if (screenpeek == p)
						{
							FX_StopAllSounds();
						}
						S_PlayActorSound(DUKE_GASP, ps[p].GetActor());

						ps[p].pos.Z = Owner->sector()->floorz - 7;
						ps[p].backupz();
					}

					if (k == 1)
					{
						ps[p].pos.X += Owner->spr.pos.X - act->spr.pos.X;
						ps[p].pos.Y += Owner->spr.pos.Y - act->spr.pos.Y;
						ps[p].backupxy();

						if (Owner->GetOwner() != Owner)
							ps[p].transporter_hold = -2;
						ps[p].setCursector(Owner->sector());

						ChangeActorSect(act2, Owner->sector());

						if ((krand() & 255) < 32)
							spawn(ps[p].GetActor(), WATERSPLASH2);
					}
					else if (isRRRA() && k == 2)
					{
						ps[p].opos.X = ps[p].pos.X += Owner->spr.pos.X - act->spr.pos.X;
						ps[p].opos.Y = ps[p].pos.Y += Owner->spr.pos.Y - act->spr.pos.Y;
						ps[p].backupxy();

						if (Owner->GetOwner() != Owner)
							ps[p].transporter_hold = -2;
						ps[p].setCursector(Owner->sector());

						ChangeActorSect(act2, Owner->sector());
					}
				}
				break;

			case STAT_ACTOR:
				if (act->spr.picnum == SHARK ||
					(isRRRA() && (act->spr.picnum == CHEERBOAT || act->spr.picnum == HULKBOAT || act->spr.picnum == MINIONBOAT || act->spr.picnum == UFO1_RRRA)) ||
					(!isRRRA() && (act->spr.picnum == UFO1_RR || act->spr.picnum == UFO2 || act->spr.picnum == UFO3 || act->spr.picnum == UFO4 || act->spr.picnum == UFO5))) continue;
				[[fallthrough]];
			case STAT_PROJECTILE:
			case STAT_MISC:
			case STAT_DUMMYPLAYER:

				ll = abs(act2->vel.Z);
				if (isRRRA())
				{
					if (act2->vel.Z >= 0)
						warpdir = 2;
					else
						warpdir = 1;
				}

				{
					warpspriteto = 0;
					if (ll && sectlotag == ST_2_UNDERWATER && act2->spr.pos.Z < (sectp->ceilingz + ll))
						warpspriteto = 1;

					if (ll && sectlotag == ST_1_ABOVE_WATER && act2->spr.pos.Z > (sectp->floorz - ll))
						if (!isRRRA() || (act2->spr.picnum != CHEERBOAT && act2->spr.picnum != HULKBOAT && act2->spr.picnum != MINIONBOAT))
							warpspriteto = 1;

					if (isRRRA())
					{
						if (ll && sectlotag == 161 && act2->spr.pos.Z < (sectp->ceilingz + ll) && warpdir == 1)
						{
							warpspriteto = 1;
							ll2 = ll - abs(act2->spr.pos.Z - sectp->ceilingz);
						}
						else if (sectlotag == 161 && act2->spr.pos.Z < (sectp->ceilingz + 3.90625) && warpdir == 1)
						{
							warpspriteto = 1;
							ll2 = zmaptoworld;
						}
						if (ll && sectlotag == 160 && act2->spr.pos.Z > (sectp->floorz - ll) && warpdir == 2)
						{
							warpspriteto = 1;
							ll2 = ll - abs(sectp->floorz - act2->spr.pos.Z);
						}
						else if (sectlotag == 160 && act2->spr.pos.Z > (sectp->floorz - 3.90625) && warpdir == 2)
						{
							warpspriteto = 1;
							ll2 = zmaptoworld;
						}
					}

					if (sectlotag == 0 && (onfloorz || abs(act2->spr.pos.Z - act->spr.pos.Z) < 16))
					{
						if (Owner->GetOwner() != Owner && onfloorz && act->temp_data[0] > 0 && act2->spr.statnum != 5)
						{
							act->temp_data[0]++;
							continue;
						}
						warpspriteto = 1;
					}

					if (warpspriteto)
					{
						if (actorflag(act2, SFLAG_NOTELEPORT)) continue;
						switch (act2->spr.picnum)
						{
						case PLAYERONWATER:
							if (sectlotag == ST_2_UNDERWATER)
							{
								act2->spr.cstat &= ~CSTAT_SPRITE_INVISIBLE;
								break;
							}
							[[fallthrough]];
						default:
							if (act2->spr.statnum == 5 && !(sectlotag == ST_1_ABOVE_WATER || sectlotag == ST_2_UNDERWATER || (isRRRA() && (sectlotag == 160 || sectlotag == 161))))
								break;
							[[fallthrough]];

						case WATERBUBBLE:
							if (rnd(192) && act2->spr.picnum == WATERBUBBLE)
								break;

							if (sectlotag > 0)
							{
								auto spawned = spawn(act2, WATERSPLASH2);
								if (spawned && sectlotag == 1 && act2->spr.statnum == 4)
								{
									spawned->vel.X = act2->vel.X * 0.5;
									spawned->spr.angle = act2->spr.angle;
									ssp(spawned, CLIPMASK0);
								}
							}

							switch (sectlotag)
							{
							case ST_0_NO_EFFECT:
								if (onfloorz)
								{
									if (checkcursectnums(act->sector()) == -1 && checkcursectnums(Owner->sector()) == -1)
									{
										act2->spr.pos += (Owner->spr.pos - act->spr.pos.XY()).plusZ(-Owner->sector()->floorz);
										act2->spr.angle = Owner->spr.angle;

										act2->backupang();

										auto beam = spawn(act, TRANSPORTERBEAM);
										if (beam) S_PlayActorSound(TELEPORTER, beam);

										beam = spawn(Owner, TRANSPORTERBEAM);
										if (beam) S_PlayActorSound(TELEPORTER, beam);

										if (Owner->GetOwner() != Owner)
										{
											act->temp_data[0] = 13;
											Owner->temp_data[0] = 13;
										}

										ChangeActorSect(act2, Owner->sector());
									}
								}
								else
								{
									act2->spr.pos.XY() += Owner->spr.pos.XY() - act->spr.pos.XY();
									act2->spr.pos.Z = Owner->spr.pos.Z + 16;
									act2->backupz();

									ChangeActorSect(act2, Owner->sector());
								}
								break;
							case ST_1_ABOVE_WATER:
								act2->spr.pos.XY() += Owner->spr.pos.XY() - act->spr.pos.XY();
								act2->spr.pos.Z = Owner->sector()->ceilingz + ll;
								act2->backupz();

								ChangeActorSect(act2, Owner->sector());

								break;
							case ST_2_UNDERWATER:
								act2->spr.pos.XY() += Owner->spr.pos.XY() - act->spr.pos.XY();
								act2->spr.pos.Z = Owner->sector()->ceilingz - ll;
								act2->backupz();

								ChangeActorSect(act2, Owner->sector());

								break;

							case 160:
								if (!isRRRA()) break;
								act2->spr.pos.XY() += Owner->spr.pos.XY() - act->spr.pos.XY();
								act2->spr.pos.Z = Owner->sector()->ceilingz + ll2;
								act2->backupz();

								ChangeActorSect(act2, Owner->sector());

								movesprite_ex(act2, DVector3(act2->spr.angle.ToVector() * act2->vel.X, 0), CLIPMASK1, coll);

								break;
							case 161:
								if (!isRRRA()) break;
								act2->spr.pos += Owner->spr.pos.XY() - act->spr.pos.XY();
								act2->spr.pos.Z = Owner->sector()->floorz - ll;
								act2->backupz();

								ChangeActorSect(act2, Owner->sector());

								movesprite_ex(act2, DVector3(act2->spr.angle.ToVector() * act2->vel.X, 0), CLIPMASK1, coll);

								break;
							}

							break;
						}
					}
				}
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

static void rrra_specialstats()
{
	Collision coll;
	DukeStatIterator it(117);
	while (auto act = it.Next())
	{
		if (act->spr.hitag > 2)
			act->spr.hitag = 0;
		if ((act->spr.picnum == RRTILE8488 || act->spr.picnum == RRTILE8490) && act->spr.hitag != 2)
		{
			act->spr.hitag = 2;
			act->spr.extra = -100;
		}
		if (act->spr.hitag == 0)
		{
			act->spr.extra++;
			if (act->spr.extra >= 30)
				act->spr.hitag = 1;
		}
		else if (act->spr.hitag == 1)
		{
			act->spr.extra--;
			if (act->spr.extra <= -30)
				act->spr.hitag = 0;
		}
		else if (act->spr.hitag == 2)
		{
			act->spr.extra--;
			if (act->spr.extra <= -104)
			{
				spawn(act, act->spr.lotag);
				act->Destroy();
			}
		}
		movesprite_ex(act, DVector3(0, 0, act->spr.extra / 128.), CLIPMASK0, coll);
	}

	it.Reset(118);
	while (auto act = it.Next())
	{
		if (act->spr.hitag > 1)
			act->spr.hitag = 0;
		if (act->spr.hitag == 0)
		{
			act->spr.extra++;
			if (act->spr.extra >= 20)
				act->spr.hitag = 1;
		}
		else if (act->spr.hitag == 1)
		{
			act->spr.extra--;
			if (act->spr.extra <= -20)
				act->spr.hitag = 0;
		}
		movesprite_ex(act, DVector3(0, 0, act->spr.extra / 256.), CLIPMASK0, coll);
	}

	if (ps[screenpeek].MamaEnd > 0)
	{
		ps[screenpeek].MamaEnd--;
		if (ps[screenpeek].MamaEnd == 0)
		{
			CompleteLevel(nullptr);
		}
	}

	if (enemysizecheat > 0)
	{
		DukeSpriteIterator itr;
		while (auto act = itr.Next())
		{
			switch (act->spr.picnum)
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
					act->spr.scale *= 2;
					act->setClipDistFromTile();
				}
				else if (enemysizecheat == 2)
				{
					act->spr.scale *= 0.5;
					act->clipdist = act->spr.scale.X, tileHeight(act->spr.picnum) * 0.125;
				}
				break;
			}

		}
		enemysizecheat = 0;
	}

	it.Reset(121);
	while (auto act = it.Next())
	{
		act->spr.extra++;
		if (act->spr.extra < 100)
		{
			if (act->spr.extra == 90)
			{
				act->spr.picnum--;
				if (act->spr.picnum < PIG + 7)
					act->spr.picnum = PIG + 7;
				act->spr.extra = 1;
			}
			movesprite_ex(act, DVector3(0, 0, -300/256.), CLIPMASK0, coll);
			if (act->sector()->ceilingz+ 4 > act->spr.pos.Z)
			{
				act->spr.picnum = 0;
				act->spr.extra = 100;
			}
		}
		else if (act->spr.extra == 200)
		{
			// This was really 10 and not (10 << 8)!
			SetActor(act, DVector3(act->spr.pos.X, act->spr.pos.Y, act->sector()->floorz - 10 * zmaptoworld));
			act->spr.extra = 1;
			act->spr.picnum = PIG + 11;
			spawn(act, TRANSPORTERSTAR);
		}
	}

	it.Reset(STAT_RABBITSPAWN);
	while (auto act = it.Next())
	{
		CallTick(act);
	}
	it.Reset(116);
	while (auto act = it.Next())
	{
		if (act->spr.extra)
		{
			if (act->spr.extra == act->spr.lotag)
				S_PlaySound(183);
			act->spr.extra--;
			int j = movesprite_ex(act, DVector3(act->spr.angle.ToVector() * act->spr.hitag / 16., act->spr.hitag / 128.), CLIPMASK0, coll);
			if (j > 0)
			{
				S_PlayActorSound(PIPEBOMB_EXPLODE, act);
				act->Destroy();
			}
			if (act->spr.extra == 0)
			{
				S_PlaySound(215);
				act->Destroy();
				earthquaketime = 32;
				SetPlayerPal(&ps[myconnectindex], PalEntry(32, 32, 32, 48));
			}
		}
	}

	it.Reset(115);
	while (auto act = it.Next())
	{
		if (act->spr.extra)
		{
			if (act->spr.picnum != RRTILE8162)
				act->spr.picnum = RRTILE8162;
			act->spr.extra--;
			if (act->spr.extra == 0)
			{
				int rvar;
				rvar = krand() & 127;
				if (rvar < 96)
				{
					act->spr.picnum = RRTILE8162 + 3;
				}
				else if (rvar < 112)
				{
					if (ps[screenpeek].SlotWin & 1)
					{
						act->spr.picnum = RRTILE8162 + 3;
					}
					else
					{
						act->spr.picnum = RRTILE8162 + 2;
						spawn(act, BATTERYAMMO);
						ps[screenpeek].SlotWin |= 1;
						S_PlayActorSound(52, act);
					}
				}
				else if (rvar < 120)
				{
					if (ps[screenpeek].SlotWin & 2)
					{
						act->spr.picnum = RRTILE8162 + 3;
					}
					else
					{
						act->spr.picnum = RRTILE8162 + 6;
						spawn(act, DYNAMITE);
						ps[screenpeek].SlotWin |= 2;
						S_PlayActorSound(52, act);
					}
				}
				else if (rvar < 126)
				{
					if (ps[screenpeek].SlotWin & 4)
					{
						act->spr.picnum = RRTILE8162 + 3;
					}
					else
					{
						act->spr.picnum = RRTILE8162 + 5;
						spawn(act, SIXPAK);
						ps[screenpeek].SlotWin |= 4;
						S_PlayActorSound(52, act);
					}
				}
				else
				{
					if (ps[screenpeek].SlotWin & 8)
					{
						act->spr.picnum = RRTILE8162 + 3;
					}
					else
					{
						act->spr.picnum = RRTILE8162 + 4;
						spawn(act, ATOMICHEALTH);
						ps[screenpeek].SlotWin |= 8;
						S_PlayActorSound(52, act);
					}
				}
			}
		}
	}

	it.Reset(122);
	while (auto act = it.Next())
	{
		if (act->spr.extra)
		{
			if (act->spr.picnum != RRTILE8589)
				act->spr.picnum = RRTILE8589;
			act->spr.extra--;
			if (act->spr.extra == 0)
			{
				int rvar;
				rvar = krand() & 127;
				if (rvar < 96)
				{
					act->spr.picnum = RRTILE8589 + 4;
				}
				else if (rvar < 112)
				{
					if (ps[screenpeek].SlotWin & 1)
					{
						act->spr.picnum = RRTILE8589 + 4;
					}
					else
					{
						act->spr.picnum = RRTILE8589 + 5;
						spawn(act, BATTERYAMMO);
						ps[screenpeek].SlotWin |= 1;
						S_PlayActorSound(342, act);
					}
				}
				else if (rvar < 120)
				{
					if (ps[screenpeek].SlotWin & 2)
					{
						act->spr.picnum = RRTILE8589 + 4;
					}
					else
					{
						act->spr.picnum = RRTILE8589 + 6;
						spawn(act, DYNAMITE);
						ps[screenpeek].SlotWin |= 2;
						S_PlayActorSound(342, act);
					}
				}
				else if (rvar < 126)
				{
					if (ps[screenpeek].SlotWin & 4)
					{
						act->spr.picnum = RRTILE8589 + 4;
					}
					else
					{
						act->spr.picnum = RRTILE8589 + 2;
						spawn(act, SIXPAK);
						ps[screenpeek].SlotWin |= 4;
						S_PlayActorSound(342, act);
					}
				}
				else
				{
					if (ps[screenpeek].SlotWin & 8)
					{
						act->spr.picnum = RRTILE8589 + 4;
					}
					else
					{
						act->spr.picnum = RRTILE8589 + 3;
						spawn(act, ATOMICHEALTH);
						ps[screenpeek].SlotWin |= 8;
						S_PlayActorSound(342, act);
					}
				}
			}
		}
	}

	it.Reset(123);
	while (auto act = it.Next())
	{
		if (act->spr.lotag == 5)
			if (!S_CheckSoundPlaying(330))
				S_PlayActorSound(330, act);
	}
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void rr_specialstats()
{
	DukeStatIterator it(107);
	while (auto act = it.Next())
	{
		if (act->spr.hitag == 100)
		{
			act->spr.pos.Z += 4;
			if (act->spr.pos.Z >= act->sector()->floorz + 59.25)
				act->spr.pos.Z = act->sector()->floorz + 59.25;
		}

		if (act->spr.picnum == LUMBERBLADE)
		{
			act->spr.extra++;
			if (act->spr.extra == 192)
			{
				act->spr.hitag = 0;
				act->spr.pos.Z = act->sector()->floorz - 59.25;
				act->spr.extra = 0;
				act->spr.picnum = RRTILE3410;
				DukeStatIterator it2(STAT_DEFAULT);
				while (auto act2 = it2.Next())
				{
					if (act2->spr.picnum == 128)
						if (act2->spr.hitag == 999)
							act2->spr.picnum = 127;
				}
			}
		}
	}

	if (chickenplant)
	{
		it.Reset(106);
		while (auto act = it.Next())
		{
			switch (act->spr.picnum)
			{
			case RRTILE285:
				act->spr.lotag--;
				if (act->spr.lotag < 0)
				{
					spawn(act, RRTILE3190)->spr.angle = act->spr.angle;
					act->spr.lotag = 128;
				}
				break;
			case RRTILE286:
				act->spr.lotag--;
				if (act->spr.lotag < 0)
				{
					spawn(act, RRTILE3192)->spr.angle = act->spr.angle;
					act->spr.lotag = 256;
				}
				break;
			case RRTILE287:
				act->spr.lotag--;
				if (act->spr.lotag < 0)
				{
					lotsoffeathers_r(act, (krand() & 3) + 4);
					act->spr.lotag = 84;
				}
				break;
			case RRTILE288:
				act->spr.lotag--;
				if (act->spr.lotag < 0)
				{
					auto j = spawn(act, RRTILE3132);
					act->spr.lotag = 96;
					if (j && !isRRRA()) S_PlayActorSound(472, j);
				}
				break;
			case RRTILE289:
				act->spr.lotag--;
				if (act->spr.lotag < 0)
				{
					spawn(act, RRTILE3120)->spr.angle = act->spr.angle;
					act->spr.lotag = 448;
				}
				break;
			case RRTILE290:
				act->spr.lotag--;
				if (act->spr.lotag < 0)
				{
					spawn(act, RRTILE3122)->spr.angle = act->spr.angle;
					act->spr.lotag = 64;
				}
				break;
			case RRTILE291:
				act->spr.lotag--;
				if (act->spr.lotag < 0)
				{
					spawn(act, RRTILE3123)->spr.angle = act->spr.angle;
					act->spr.lotag = 512;
				}
				break;
			case RRTILE292:
				act->spr.lotag--;
				if (act->spr.lotag < 0)
				{
					spawn(act, RRTILE3124)->spr.angle = act->spr.angle;
					act->spr.lotag = 224;
				}
				break;
			case RRTILE293:
				act->spr.lotag--;
				if (act->spr.lotag < 0)
				{
					fi.guts(act, JIBS1, 1, myconnectindex);
					fi.guts(act, JIBS2, 1, myconnectindex);
					fi.guts(act, JIBS3, 1, myconnectindex);
					fi.guts(act, JIBS4, 1, myconnectindex);
					act->spr.lotag = 256;
				}
				break;
			}
		}
	}

	it.Reset(STAT_BOWLING);
	while (auto act = it.Next())
	{
		if (act->spr.picnum == BOWLINGPINSPOT)
			if (act->spr.lotag == 100)
			{
				auto pst = pinsectorresetup(act->sector());
				if (pst)
				{
					act->spr.lotag = 0;
					if (act->spr.extra == 1)
					{
						pst = checkpins(act->sector());
						if (!pst)
						{
							act->spr.extra = 2;
						}
					}
					if (act->spr.extra == 2)
					{
						act->spr.extra = 0;
						resetpins(act->sector());
					}
				}
			}
	}

	it.Reset(108);
	while (auto act = it.Next())
	{
		if (act->spr.picnum == RRTILE296)
		{
			double xx;
			int p = findplayer(act, &xx);
			if (xx < 128)
			{
				DukeStatIterator it2(108);
				while (auto act2 = it2.Next())
				{
					if (act2->spr.picnum == RRTILE297)
					{
						ps[p].angle.ang = act2->spr.angle;
						ps[p].pos = act2->spr.pos.plusZ(-36);
						ps[p].backupxyz();
						ps[p].setbobpos();
						auto pact = ps[p].GetActor();
						ChangeActorSect(pact, act2->sector());
						ps[p].setCursector(pact->sector());
						S_PlayActorSound(70, act2);
						act2->Destroy();
					}
				}
			}
		}
	}
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

static void heavyhbomb(DDukeActor *actor)
{
	auto sectp = actor->sector();
	int l;
	double xx;
	auto Owner = actor->GetOwner();

	if ((actor->spr.cstat & CSTAT_SPRITE_INVISIBLE))
	{
		actor->temp_data[2]--;
		if (actor->temp_data[2] <= 0)
		{
			S_PlayActorSound(TELEPORTER, actor);
			spawn(actor, TRANSPORTERSTAR);
			actor->spr.cstat = CSTAT_SPRITE_BLOCK_ALL;
		}
		return;
	}

	int p = findplayer(actor, &xx);

	if (xx < 1220 / 16.) actor->spr.cstat &= ~CSTAT_SPRITE_BLOCK_ALL;
	else actor->spr.cstat |= CSTAT_SPRITE_BLOCK_ALL;

	if (actor->temp_data[3] == 0)
	{
		int j = fi.ifhitbyweapon(actor);
		if (j >= 0)
		{
			actor->temp_data[3] = 1;
			actor->temp_data[4] = 0;
			l = 0;
			actor->vel.X = 0;
			goto DETONATEB;
		}
	}

	makeitfall(actor);

	if (sectp->lotag != 1 && (!isRRRA() || sectp->lotag != 160) && actor->spr.pos.Z >= actor->floorz - FOURSLEIGHT_F && actor->spr.yint < 3)
	{
		if (actor->spr.yint > 0 || (actor->spr.yint == 0 && actor->floorz == sectp->floorz))
		{
			if (actor->spr.picnum != CHEERBOMB)
				S_PlayActorSound(PIPEBOMB_BOUNCE, actor);
			else
			{
				actor->temp_data[3] = 1;
				actor->temp_data[4] = 1;
				l = 0;
				goto DETONATEB;
			}
		}
		actor->vel.Z = -(4 - actor->spr.yint);
		if (actor->sector()->lotag == 2)
			actor->vel.Z *= 0.25;
		actor->spr.yint++;
	}
	if (actor->spr.picnum != CHEERBOMB && actor->spr.pos.Z < actor->ceilingz + 16 && sectp->lotag != 2)
	{
		actor->spr.pos.Z = actor->ceilingz + 16;
		actor->vel.Z = 0;
	}

	Collision coll;
	movesprite_ex(actor, DVector3(actor->spr.angle.ToVector() * actor->vel.X, actor->vel.Z), CLIPMASK0, coll);

	if (actor->sector()->lotag == 1 && actor->vel.Z == 0)
	{
		actor->spr.pos.Z += 32;
		if (actor->temp_data[5] == 0)
		{
			actor->temp_data[5] = 1;
			spawn(actor, WATERSPLASH2);
			if (isRRRA() && actor->spr.picnum == MORTER)
				actor->vel.X = 0;
		}
	}
	else actor->temp_data[5] = 0;

	if (actor->temp_data[3] == 0 && actor->spr.picnum == MORTER && (coll.type || xx < 844 / 16.))
	{
		actor->temp_data[3] = 1;
		actor->temp_data[4] = 0;
		l = 0;
		actor->vel.X = 0;
		goto DETONATEB;
	}

	if (actor->temp_data[3] == 0 && actor->spr.picnum == CHEERBOMB && (coll.type || xx < 844 / 16.))
	{
		actor->temp_data[3] = 1;
		actor->temp_data[4] = 0;
		l = 0;
		actor->vel.X = 0;
		goto DETONATEB;
	}

	if (Owner && Owner->spr.picnum == APLAYER)
		l = Owner->PlayerIndex();
	else l = -1;

	if(actor->vel.X > 0)
	{
		actor->vel.X -= 5. / 16;
		if (sectp->lotag == 2)
			actor->vel.X -= 10. / 16;

		if(actor->vel.X < 0)
			actor->vel.X = 0;
		if (int(actor->vel.X * 16) & 8) actor->spr.cstat ^= CSTAT_SPRITE_XFLIP;
	}

	if (coll.type == kHitWall)
	{
		auto wal = coll.hitWall;
		fi.checkhitwall(actor, wal, actor->spr.pos, actor->spr.picnum);

		if (actor->spr.picnum == CHEERBOMB)
		{
			actor->temp_data[3] = 1;
			actor->temp_data[4] = 0;
			l = 0;
			actor->vel.X = 0;
			goto DETONATEB;
		}
		DAngle k = wal->delta().Angle();
		actor->spr.angle = k * 2 - actor->spr.angle;
		actor->vel.X *= 0.5;
	}

DETONATEB:

	if ((l >= 0 && ps[l].hbomb_on == 0) || actor->temp_data[3] == 1)
	{
		actor->temp_data[4]++;

		if (actor->temp_data[4] == 2)
		{
			int x = actor->spr.extra;
			int m = 0;
			switch (actor->spr.picnum)
			{
			case POWDERKEG: m = gs.tripbombblastradius; break;	// powder keg
			case DYNAMITE: m = gs.pipebombblastradius; break;
			case HBOMBAMMO: m = gs.pipebombblastradius; break;
			case MORTER: m = gs.morterblastradius; break;
			case CHEERBOMB: m = gs.morterblastradius; break;
			}

			if (actor->sector()->lotag != 800)
			{
				fi.hitradius(actor, m, x >> 2, x >> 1, x - (x >> 2), x);
				spawn(actor, EXPLOSION2);
				if (actor->spr.picnum == CHEERBOMB)
					spawn(actor, BURNING);
				S_PlayActorSound(PIPEBOMB_EXPLODE, actor);
				for (x = 0; x < 8; x++)
					RANDOMSCRAP(actor);
			}
		}

		if (actor->spr.scale.Y)
		{
			actor->spr.scale.Y = (0);
			return;
		}

		if (actor->temp_data[4] > 20)
		{
			actor->Destroy();
			return;
		}
		if (actor->spr.picnum == CHEERBOMB)
		{
			spawn(actor, BURNING);
			actor->Destroy();
			return;
		}
	}
	else if (actor->spr.picnum == DYNAMITE && xx < 788 / 16. && actor->temp_data[0] > 7 && actor->vel.X == 0)
		if (cansee(actor->spr.pos.plusZ(-8), actor->sector(), ps[p].pos, ps[p].cursector))
			if (ps[p].ammo_amount[DYNAMITE_WEAPON] < gs.max_ammo_amount[DYNAMITE_WEAPON])
				if (actor->spr.pal == 0)
				{
					if (ud.coop >= 1)
					{
						for (int j = 0; j < ps[p].weapreccnt; j++)
							if (ps[p].weaprecs[j] == actor->spr.picnum)
								return;

						if (ps[p].weapreccnt < 255)
							ps[p].weaprecs[ps[p].weapreccnt++] = actor->spr.picnum;
					}

					addammo(DYNAMITE_WEAPON, &ps[p], 1);
					addammo(CROSSBOW_WEAPON, &ps[p], 1);
					S_PlayActorSound(DUKE_GET, ps[p].GetActor());

					if (ps[p].gotweapon[DYNAMITE_WEAPON] == 0 || Owner == ps[p].GetActor())
						fi.addweapon(&ps[p], DYNAMITE_WEAPON, true);

					if (!Owner || Owner->spr.picnum != APLAYER)
					{
						SetPlayerPal(&ps[p], PalEntry(32, 0, 32, 0));
					}

					if (Owner && (Owner->attackertype != DYNAMITE || ud.respawn_items == 0 || Owner->spr.picnum == APLAYER))
					{
						if (actor->spr.picnum == DYNAMITE && Owner->spr.picnum != APLAYER && ud.coop)
							return;
						actor->Destroy();
						return;
					}
					else
					{
						actor->temp_data[2] = gs.respawnitemtime;
						spawn(actor, RESPAWNMARKERRED);
						actor->spr.cstat = CSTAT_SPRITE_INVISIBLE;
					}
				}

	if (actor->temp_data[0] < 8) actor->temp_data[0]++;
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

static int henstand(DDukeActor *actor)
{
	if (actor->spr.picnum == HENSTAND || actor->spr.picnum == HENSTAND + 1)
	{
		actor->spr.lotag--;
		if (actor->spr.lotag == 0)
		{
			spawn(actor, HEN);
			actor->Destroy();
			return 1;
		}
	}
	if (actor->sector()->lotag == 900)
		actor->vel.X = 0;
	if(actor->vel.X != 0)
	{
		makeitfall(actor);
		Collision coll;
		movesprite_ex(actor, DVector3(actor->spr.angle.ToVector() * actor->vel.X, actor->vel.Z), CLIPMASK0, coll);
		if (coll.type)
		{
			if (coll.type == kHitWall)
			{
				DAngle k = coll.hitWall->delta().Angle();
				actor->spr.angle = k * 2 - actor->spr.angle;
			}
			else if (coll.type == kHitSprite)
			{
				auto hitact = coll.actor();
				fi.checkhitsprite(actor, hitact);
				if (hitact->spr.picnum == HEN)
				{
					auto ns = spawn(hitact, HENSTAND);
					hitact->Destroy();
					if (ns)
					{
						ns->vel.X = 2;
						ns->spr.lotag = 40;
						ns->spr.angle = actor->spr.angle;
					}
				}
			}
		}
		actor->vel.X -= 1/16.;
		if(actor->vel.X < 0) actor->vel.X = 0;
		actor->spr.cstat = CSTAT_SPRITE_BLOCK_ALL;
		if (actor->spr.picnum == BOWLINGPIN)
		{
			actor->spr.cstat |= CSTAT_SPRITE_XFLIP & ESpriteFlags::FromInt(int(actor->vel.X * 16));
			actor->spr.cstat |= CSTAT_SPRITE_YFLIP & ESpriteFlags::FromInt(int(actor->vel.X * 16));
			if (krand() & 1)
				actor->spr.picnum = BOWLINGPIN + 1;
		}
		else if (actor->spr.picnum == HENSTAND)
		{
			actor->spr.cstat |= CSTAT_SPRITE_XFLIP & ESpriteFlags::FromInt(int(actor->vel.X * 16));
			actor->spr.cstat |= CSTAT_SPRITE_YFLIP & ESpriteFlags::FromInt(int(actor->vel.X * 16));
			if (krand() & 1)
				actor->spr.picnum = HENSTAND + 1;
			if (actor->vel.X == 0)
				return 2;//actor->Destroy(); still needs to run a script but should not do on a deleted object
		}
		if (actor->spr.picnum == BOWLINGPIN || (actor->spr.picnum == BOWLINGPIN + 1 && actor->vel.X == 0))
		{
			return 2;//actor->Destroy(); still needs to run a script but should not do on a deleted object
		}
	}
	else if (actor->sector()->lotag == 900)
	{
		if (actor->spr.picnum == BOWLINGBALL)
			ballreturn(actor);
		actor->Destroy();
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
	double xx;
	int p;
	Collision coll;

	dojaildoor();
	moveminecart();

	if (isRRRA())
	{
		rrra_specialstats();
	}
	rr_specialstats();

	DukeStatIterator it(STAT_ACTOR);
	while (auto act = it.Next())
	{
		bool deleteafterexecute = false;	// taking a cue here from RedNukem to not run scripts on deleted sprites.

		if( act->spr.scale.X == 0 || !act->insector() || actorflag(act, SFLAG2_DIENOW))
		{
			act->Destroy();
			continue;
		}

		auto sectp = act->sector();

		if (act->GetClass() != RUNTIME_CLASS(DDukeActor))
		{
			CallTick(act);
		}
		else switch(act->spr.picnum)
		{
			case RESPAWNMARKERRED:
			case RESPAWNMARKERYELLOW:
			case RESPAWNMARKERGREEN:
				if (!respawnmarker(act, RESPAWNMARKERYELLOW, RESPAWNMARKERGREEN)) continue;
				break;
			case RAT:
				if (!rat(act, !isRRRA())) continue;
				break;
			case RRTILE3190:
			case RRTILE3191:
			case RRTILE3192:
				if (!chickenplant)
				{
					act->Destroy();
					continue;
				}
				if (sectp->lotag == 903)
					makeitfall(act);
				movesprite_ex(act, DVector3(act->spr.angle.ToVector() * act->vel.X, act->vel.Z), CLIPMASK0, coll);
				switch (sectp->lotag)
				{
					case 901:
						act->spr.picnum = RRTILE3191;
						break;
					case 902:
						act->spr.picnum = RRTILE3192;
						break;
					case 903:
						if (act->spr.pos.Z >= sectp->floorz - 8)
						{
							act->Destroy();
							continue;
						}
						break;
					case 904:
						act->Destroy();
						continue;
						break;
				}
				if (coll.type > kHitSector)
				{
					act->Destroy();
					continue;
				}
				break;

			case RRTILE3120:
			case RRTILE3122:
			case RRTILE3123:
			case RRTILE3124:
				if (!chickenplant)
				{
					act->Destroy();
					continue;
				}
				makeitfall(act);
				movesprite_ex(act, DVector3(act->spr.angle.ToVector() * act->vel.X, act->vel.Z), CLIPMASK0, coll);
				if (coll.type > kHitSector)
				{
					act->Destroy();
					continue;
				}
				if (sectp->lotag == 903)
				{
					if (act->spr.pos.Z >= sectp->floorz - 4)
					{
						act->Destroy();
						continue;
					}
				}
				else if (sectp->lotag == 904)
				{
					act->Destroy();
					continue;
				}
				break;

			case RRTILE3132:
				if (!chickenplant)
				{
					act->Destroy();
					continue;
				}
				makeitfall(act);
				movesprite_ex(act, DVector3(act->spr.angle.ToVector() * act->vel.X, act->vel.Z), CLIPMASK0, coll);
				if (act->spr.pos.Z >= sectp->floorz - 8)
				{
					if (sectp->lotag == 1)
					{
						auto j = spawn(act, WATERSPLASH2);
						if (j) j->spr.pos.Z = j->sector()->floorz;
					}
					act->Destroy();
					continue;
				}
				break;
			case BOWLINGBALL:
				if (act->vel.X != 0)
				{
					if(!S_CheckSoundPlaying(356))
						S_PlayActorSound(356,act);
				}
				else
				{
					spawn(act,BOWLINGBALLSPRITE);
					act->Destroy();
					continue;
				}
				if (act->sector()->lotag == 900)
				{
					S_StopSound(356, nullptr);
				}
				[[fallthrough]];
			case BOWLINGPIN:
			case BOWLINGPIN+1:
			case HENSTAND:
			case HENSTAND+1:
			{
				int todo = henstand(act);
				if (todo == 2) deleteafterexecute = true;
				if (todo == 1) continue;
				break;
			}

			case QUEBALL:
			case STRIPEBALL:
				if (!queball(act, POCKET, QUEBALL, STRIPEBALL)) continue;
				break;
			case FORCESPHERE:
				forcesphere(act, FORCESPHERE);
				continue;

			case RECON:
			case UFO1_RR:
			case UFO2:
			case UFO3:
			case UFO4:
			case UFO5:
				recon(act, EXPLOSION2, FIRELASER, -1, -1, 457, 1, [](DDukeActor* act) ->int
					{
						if (isRRRA() && ufospawnsminion)
							return MINION;
						else if (act->spr.picnum == UFO1_RR)
							return HEN;
						else if (act->spr.picnum == UFO2)
							return COOT;
						else if (act->spr.picnum == UFO3)
							return COW;
						else if (act->spr.picnum == UFO4)
							return PIG;
						else if (act->spr.picnum == UFO5)
							return BILLYRAY;
						else return -1;
					});
				continue;

			case OOZ:
				ooz(act);
				continue;

			case EMPTYBIKE:
				if (!isRRRA()) break;
				makeitfall(act);
				getglobalz(act);
				if (sectp->lotag == 1)
				{
					SetActor(act, DVector3(act->spr.pos.X, act->spr.pos.Y, act->floorz + 16));
				}
				break;

			case EMPTYBOAT:
				if (!isRRRA()) break;
				makeitfall(act);
				getglobalz(act);
				break;

			case POWDERKEG:
				if (!isRRRA() || (sectp->lotag != 1 && sectp->lotag != 160))
					if (act->vel.X != 0)
					{
						movesprite_ex(act, DVector3(act->spr.angle.ToVector()* act->vel.X, act->vel.Z), CLIPMASK0, coll);
						act->vel.X -= 1. / 16.;
					}
				break;

			case CHEERBOMB:
				if (!isRRRA()) break;
				[[fallthrough]];
			case MORTER:
			case DYNAMITE:
				heavyhbomb(act);
				continue;

			case REACTORBURNT:
			case REACTOR2BURNT:
				continue;

			case REACTOR:
			case REACTOR2:
				reactor(act, REACTOR, REACTOR2, REACTORBURNT, REACTOR2BURNT, REACTORSPARK, REACTOR2SPARK);
				continue;
		}


// #ifndef VOLOMEONE
		if( ud.multimode < 2 && badguy(act) )
		{
			if( actor_tog == 1)
			{
				act->spr.cstat = CSTAT_SPRITE_INVISIBLE;
				continue;
			}
			else if(actor_tog == 2) act->spr.cstat = CSTAT_SPRITE_BLOCK_ALL;
		}
// #endif

		p = findplayer(act, &xx);

		execute(act,p,xx);
		if (deleteafterexecute) act->Destroy();
	}

}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void moveexplosions_r(void)  // STATNUM 5
{
	int p;
	int * t;
	double xx;


	DukeStatIterator it(STAT_MISC);
	while (auto act = it.Next())
	{
		t = &act->temp_data[0];
		auto sectp = act->sector();

		if (!act->insector() || act->spr.scale.X == 0) 
		{
			act->Destroy();
			continue;
		}

		if (act->GetClass() != RUNTIME_CLASS(DDukeActor))
		{
			CallTick(act);
			continue;
		}

		switch (act->spr.picnum)
		{
		case SHOTGUNSPRITE:
			if (act->sector()->lotag == 800)
				if (act->spr.pos.Z >= act->sector()->floorz - 8)
				{
					act->Destroy();
					continue;
				}
			break;
		case NEON1:
		case NEON2:
		case NEON3:
		case NEON4:
		case NEON5:
		case NEON6:

			if ((global_random / (act->spr.lotag + 1) & 31) > 4) act->spr.shade = -127;
			else act->spr.shade = 127;
			continue;

		case BLOODSPLAT1:
		case BLOODSPLAT2:
		case BLOODSPLAT3:
		case BLOODSPLAT4:

			bloodsplats(act);
			continue;


		case FORCESPHERE:
			forcesphereexplode(act);
			continue;

		case MUD:

			act->temp_data[0]++;
			if (act->temp_data[0] == 1)
			{
				if (sectp->floorpicnum != 3073)
				{
					act->Destroy();
					continue;
				}
				if (S_CheckSoundPlaying(22))
					S_PlayActorSound(22, act);
			}
			if (act->temp_data[0] == 3)
			{
				act->temp_data[0] = 0;
				act->temp_data[1]++;
			}
			if (act->temp_data[1] == 5)
				act->Destroy();
			continue;

		case WATERSPLASH2:
			watersplash2(act);
			continue;

		case FRAMEEFFECT1:
			frameeffect1(act);
			continue;
		case INNERJAW:
		case INNERJAW + 1:

			p = findplayer(act, &xx);
			if (xx < 32)
			{
				SetPlayerPal(&ps[p], PalEntry(32, 32, 0, 0));
				ps[p].GetActor()->spr.extra -= 4;
			}
			[[fallthrough]];

		case COOLEXPLOSION1:
		case FIRELASER:
		case OWHIP:
		case UWHIP:
			if (act->spr.extra != 999)
				act->spr.extra = 999;
			else
			{
				act->Destroy();
				continue;
			}
			break;
		case TONGUE:
			act->Destroy();
			continue;
		case FEATHER + 1: // feather
			act->spr.pos.Z = act->floorz = getflorzofslopeptr(act->sector(), act->spr.pos.X, act->spr.pos.Y);
			if (act->sector()->lotag == 800)
			{
				act->Destroy();
				continue;
			}
			break;
		case FEATHER:
			if (!money(act, BLOODPOOL)) continue;

			if (act->sector()->lotag == 800)
				if (act->spr.pos.Z >= act->sector()->floorz - 8)
				{
					act->Destroy();
					continue;
				}

			break;

		case RRTILE2460:
		case RRTILE2465:
		case BIKEJIBA:
		case BIKEJIBB:
		case BIKEJIBC:
		case BIKERJIBA:
		case BIKERJIBB:
		case BIKERJIBC:
		case BIKERJIBD:
		case CHEERJIBA:
		case CHEERJIBB:
		case CHEERJIBC:
		case CHEERJIBD:
		case FBOATJIBA:
		case FBOATJIBB:
		case RABBITJIBA:
		case RABBITJIBB:
		case RABBITJIBC:
		case MAMAJIBA:
		case MAMAJIBB:
			if (!isRRRA()) break;
			[[fallthrough]];

		case BILLYJIBA:
		case BILLYJIBB:
		case HULKJIBA:
		case HULKJIBB:
		case HULKJIBC:
		case MINJIBA:
		case MINJIBB:
		case MINJIBC:
		case COOTJIBA:
		case COOTJIBB:
		case COOTJIBC:
		case JIBS1:
		case JIBS2:
		case JIBS3:
		case JIBS4:
		case JIBS5:
		case JIBS6:
		case DUKETORSO:
		case DUKEGUN:
		case DUKELEG:
			if (!jibs(act, JIBS6, false, true, true, act->spr.picnum == DUKELEG || act->spr.picnum == DUKETORSO || act->spr.picnum == DUKEGUN,
				isRRRA() && (act->spr.picnum == RRTILE2465 || act->spr.picnum == RRTILE2560))) continue;

			if (act->sector()->lotag == 800)
				if (act->spr.pos.Z >= act->sector()->floorz - 8)
				{
					act->Destroy();
					continue;
				}

			continue;

		case BLOODPOOL:
			if (!bloodpool(act, false)) continue;

			if (act->sector()->lotag == 800)
				if (act->spr.pos.Z >= act->sector()->floorz - 8)
				{
					act->Destroy();
				}
			continue;

		case BURNING:
		case WATERBUBBLE:
		case SMALLSMOKE:
		case EXPLOSION2:
		case EXPLOSION3:
		case BLOOD:
		case FORCERIPPLE:
		case TRANSPORTERSTAR:
		case TRANSPORTERBEAM:
			p = findplayer(act, &xx);
			execute(act, p, xx);
			continue;

		case SHELL:
		case SHOTGUNSHELL:
			shell(act, false);
			continue;

		case GLASSPIECES:
		case GLASSPIECES + 1:
		case GLASSPIECES + 2:
		case POPCORN:
			glasspieces(act);
			continue;
		}
	}
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void handle_se06_r(DDukeActor *actor)
{
	auto sc = actor->sector();
	int sh = actor->spr.hitag;

	int k = sc->extra;

	if (actor->temp_data[4] > 0)
	{
		actor->temp_data[4]--;
		if (actor->temp_data[4] >= (k - (k >> 3)))
			actor->vel.X -= (k >> 5) / 16.;
		if (actor->temp_data[4] > ((k >> 1) - 1) && actor->temp_data[4] < (k - (k >> 3)))
			actor->vel.X = 0;
		if (actor->temp_data[4] < (k >> 1))
			actor->vel.X += (k >> 5) / 16.;
		if (actor->temp_data[4] < ((k >> 1) - (k >> 3)))
		{
			actor->temp_data[4] = 0;
			actor->vel.X = k / 16.;
			if ((!isRRRA() || lastlevel) && hulkspawn)
			{
				hulkspawn--;
				auto ns = spawn(actor, HULK);
				if (ns)
				{
					ns->spr.pos.Z = ns->sector()->ceilingz;
					ns->spr.pal = 33;
				}
				if (!hulkspawn)
				{
					ns = CreateActor(actor->sector(), DVector3(actor->spr.pos.XY(), actor->sector()->ceilingz + 466.5), 3677, -8, DVector2(0.25, 0.25), nullAngle, 0., 0., actor, 5);
					if (ns)
					{
						ns->spr.cstat = CSTAT_SPRITE_TRANS_FLIP | CSTAT_SPRITE_TRANSLUCENT;
						ns->spr.pal = 7;
						ns->spr.scale = DVector2(1.25, 3.984375);
					}
					ns = spawn(actor, 296);
					if (ns)
					{
						ns->spr.cstat = 0;
						ns->spr.cstat |= CSTAT_SPRITE_INVISIBLE;
						ns->spr.pos.Z = actor->sector()->floorz - 24;
					}
					actor->Destroy();
					return;
				}
			}
		}
	}
	else
	{
		actor->vel.X = k / 16.;
		DukeSectIterator it(actor->sector());
		while (auto a2 = it.Next())
		{
			if (a2->spr.picnum == UFOBEAM && ufospawn && ++ufocnt == 64)
			{
				int pn;
				ufocnt = 0;
				ufospawn--;
				if (!isRRRA())
				{
					switch (krand() & 3)
					{
					default:
					case 0:
						pn = UFO1_RR;
						break;
					case 1:
						pn = UFO2;
						break;
					case 2:
						pn = UFO3;
						break;
					case 3:
						pn = UFO4;
						break;
					}
				}
				else pn = UFO1_RRRA;
				auto ns = spawn(actor, pn);
				if (ns) ns->spr.pos.Z = ns->sector()->ceilingz;
			}
		}
	}

	DukeStatIterator it(STAT_EFFECTOR);
	while (auto act2 = it.Next())
	{
		if ((act2->spr.lotag == SE_14_SUBWAY_CAR) && (sh == act2->spr.hitag) && (act2->temp_data[0] == actor->temp_data[0]))
		{
			act2->vel.X = actor->vel.X;
			//if( actor->temp_data[4] == 1 )
			{
				if (act2->temp_pos.X == 0)
					act2->temp_pos.X = (act2->spr.pos - actor->spr.pos).LengthSquared();
				int x = Sgn((act2->spr.pos - actor->spr.pos).LengthSquared() - act2->temp_pos.X);
				if (act2->spr.extra) x = -x;
				actor->vel.X += x / 16.;
			}
			act2->temp_data[4] = actor->temp_data[4];
		}
	}
	handle_se14(actor, false, RPG, JIBS6);
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void moveeffectors_r(void)   //STATNUM 3
{
	clearfriction();

	DukeStatIterator it(STAT_EFFECTOR);
	while (auto act = it.Next())
	{
		auto sc = act->sector();
		int st = act->spr.lotag;

		switch (st)
		{
		case SE_0_ROTATING_SECTOR:
			handle_se00(act);
			break;

		case SE_1_PIVOT: //Nothing for now used as the pivot
			handle_se01(act);
			break;

		case SE_6_SUBWAY:
			handle_se06_r(act);
			break;

		case SE_14_SUBWAY_CAR:
			handle_se14(act, false, RPG, JIBS6);
			break;

		case SE_30_TWO_WAY_TRAIN:
			handle_se30(act, JIBS6);
			break;


		case SE_2_EARTHQUAKE:
			handle_se02(act);
			break;

			//Flashing sector lights after reactor EXPLOSION2
		case SE_3_RANDOM_LIGHTS_AFTER_SHOT_OUT:
			handle_se03(act);
			break;

		case SE_4_RANDOM_LIGHTS:
			handle_se04(act);
			break;

			//BOSS
		case SE_5_BOSS:
			handle_se05(act, FIRELASER);
			break;

		case SE_8_UP_OPEN_DOOR_LIGHTS:
		case SE_9_DOWN_OPEN_DOOR_LIGHTS:
			handle_se08(act, true);
			break;

		case SE_10_DOOR_AUTO_CLOSE:
			handle_se10(act, nullptr);
			break;

		case SE_11_SWINGING_DOOR:
			handle_se11(act);
			break;

		case SE_12_LIGHT_SWITCH:
			handle_se12(act);
			break;

		case SE_47_LIGHT_SWITCH:
			if (isRRRA()) handle_se12(act, 1);
			break;

		case SE_48_LIGHT_SWITCH:
			if (isRRRA()) handle_se12(act, 2);
			break;


		case SE_13_EXPLOSIVE:
			handle_se13(act);
			break;

		case SE_15_SLIDING_DOOR:
			handle_se15(act);
			break;

		case SE_16_REACTOR:
			handle_se16(act, REACTOR, REACTOR2);
			break;

		case SE_17_WARP_ELEVATOR:
			handle_se17(act);
			break;

		case SE_18_INCREMENTAL_SECTOR_RISE_FALL:
			handle_se18(act, true);
			break;

		case SE_19_EXPLOSION_LOWERS_CEILING:
			handle_se19(act, BIGFORCE);
			break;

		case SE_20_STRETCH_BRIDGE:
			handle_se20(act);
			break;

		case SE_21_DROP_FLOOR:
			handle_se21(act);
			break;

		case SE_22_TEETH_DOOR:
			handle_se22(act);
			break;

		case SE_156_CONVEYOR_NOSCROLL:
			if (!isRRRA()) break;
			[[fallthrough]];
		case SE_24_CONVEYOR:
		case SE_34:
		{
			handle_se24(act, st != SE_156_CONVEYOR_NOSCROLL, 0.5);
			break;
		}

		case SE_35:
			handle_se35(act, SMALLSMOKE, EXPLOSION2);
			break;

		case SE_25_PISTON: //PISTONS
			if (act->temp_data[4] == 0) break;
			handle_se25(act, isRRRA() ? 371 : -1, isRRRA() ? 167 : -1);
			break;

		case SE_26:
			handle_se26(act);
			break;

		case SE_27_DEMO_CAM:
			handle_se27(act);
			break;

		case SE_29_WAVES:
			handle_se29(act);
			break;

		case SE_31_FLOOR_RISE_FALL: // True Drop Floor
			handle_se31(act, false);
			break;

		case SE_32_CEILING_RISE_FALL: // True Drop Ceiling
			handle_se32(act);
			break;

		case SE_33_QUAKE_DEBRIS:
			if (earthquaketime > 0 && (krand() & 7) == 0)
				RANDOMSCRAP(act);
			break;
		case SE_36_PROJ_SHOOTER:

			if (act->temp_data[0])
			{
				if (act->temp_data[0] == 1)
					fi.shoot(act, sc->extra);
				else if (act->temp_data[0] == 26 * 5)
					act->temp_data[0] = 0;
				act->temp_data[0]++;
			}
			break;

		case SE_128_GLASS_BREAKING:
			handle_se128(act);
			break;

		case SE_130:
			handle_se130(act, 80, EXPLOSION2);
			break;
		case SE_131:
			handle_se130(act, 40, EXPLOSION2);
			break;
		}
	}

	//Sloped sin-wave floors!
	it.Reset(STAT_EFFECTOR);
	while (auto act = it.Next())
	{
		if (act->spr.lotag != SE_29_WAVES) continue;
		auto sc = act->sector();
		if (sc->walls.Size() != 4) continue;
		auto wal = &sc->walls[2];
		if (wal->nextSector()) alignflorslope(act->sector(), DVector3(wal->pos, wal->nextSector()->floorz));
	}
}


//---------------------------------------------------------------------------
//
// game specific part of makeitfall.
//
//---------------------------------------------------------------------------

double adjustfall(DDukeActor *actor, double c)
{
	if ((actor->spr.picnum == BIKERB || actor->spr.picnum == CHEERB) && c == gs.gravity)
		c = gs.gravity * 0.25;
	else if (actor->spr.picnum == BIKERBV2 && c == gs.gravity)
		c = gs.gravity * 0.125;
	return c;
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void move_r(DDukeActor *actor, int pnum, int xvel)
{
	DAngle goalang, angdif;
	double daxvel;

	int a = actor->spr.hitag;

	if (a == -1) a = 0;

	actor->temp_data[0]++;

	if (a & face_player)
	{
		if (ps[pnum].newOwner != nullptr)
			goalang = (ps[pnum].opos.XY() - actor->spr.pos.XY()).Angle();
		else goalang = (ps[pnum].pos.XY() - actor->spr.pos.XY()).Angle();
		angdif = deltaangle(actor->spr.angle, goalang) * 0.25;
		if (angdif > -DAngle22_5 / 8 && angdif < nullAngle) angdif = nullAngle;
		actor->spr.angle += angdif;
	}

	if (a & spin)
		actor->spr.angle += DAngle45 * BobVal(actor->temp_data[0] << 3);

	if (a & face_player_slow)
	{
		if (ps[pnum].newOwner != nullptr)
			goalang = (ps[pnum].opos.XY() - actor->spr.pos.XY()).Angle();
		else goalang = (ps[pnum].pos.XY() - actor->spr.pos.XY()).Angle();
		angdif = DAngle22_5 * 0.25 * Sgn(deltaangle(actor->spr.angle, goalang).Degrees()); // this looks very wrong...
		actor->spr.angle += angdif;
	}

	if (isRRRA())
	{
		if (a & antifaceplayerslow)
		{
			if (ps[pnum].newOwner != nullptr)
				goalang = ((ps[pnum].opos.XY() - actor->spr.pos.XY()).Angle() + DAngle180);
			else goalang = ((ps[pnum].pos.XY() - actor->spr.pos.XY()).Angle() + DAngle180);
			angdif = DAngle22_5 * 0.25 * Sgn(deltaangle(actor->spr.angle, goalang).Degrees()); // this looks very wrong...
			actor->spr.angle += angdif;
		}

		if ((a & jumptoplayer) == jumptoplayer)
		{
			if (actor->spr.picnum == CHEER)
			{
				if (actor->temp_data[0] < 16)
					actor->vel.Z -= BobVal(512 + (actor->temp_data[0] << 4)) * 1.6;
			}
			else
			{
				if (actor->temp_data[0] < 16)
					actor->vel.Z -= BobVal(512 + (actor->temp_data[0] << 4)) * 2;
			}
		}
		if (a & justjump1)
		{
			if (actor->spr.picnum == RABBIT)
			{
				if (actor->temp_data[0] < 8)
					actor->vel.Z -= BobVal(512 + (actor->temp_data[0] << 4)) * 2.133;
			}
			else if (actor->spr.picnum == MAMA)
			{
				if (actor->temp_data[0] < 8)
					actor->vel.Z -= BobVal(512 + (actor->temp_data[0] << 4)) * 1.83;
			}
		}
		if (a & justjump2)
		{
			if (actor->spr.picnum == RABBIT)
			{
				if (actor->temp_data[0] < 8)
					actor->vel.Z -= BobVal(512 + (actor->temp_data[0] << 4)) * 2.667;
			}
			else if (actor->spr.picnum == MAMA)
			{
				if (actor->temp_data[0] < 8)
					actor->vel.Z -= BobVal(512 + (actor->temp_data[0] << 4)) * 2.286;
			}
		}
		if (a & windang)
		{
			if (actor->temp_data[0] < 8)
				actor->vel.Z -= BobVal(512 + (actor->temp_data[0] << 4) * 2.667);
		}
	}
	else if ((a & jumptoplayer) == jumptoplayer)
	{
		if (actor->temp_data[0] < 16)
			actor->vel.Z -= BobVal(512 + (actor->temp_data[0] << 4)) * 2;
	}


	if (a & face_player_smart)
	{
		DVector2 newpos = ps[pnum].pos.XY() + (ps[pnum].vel.XY() * (4. / 3.));
		goalang = (newpos - actor->spr.pos.XY()).Angle();
		angdif = deltaangle(actor->spr.angle, goalang) * 0.25;
		if (angdif > -DAngle22_5 / 16 && angdif < nullAngle) angdif = nullAngle;
		actor->spr.angle += angdif;
	}

	if (actor->temp_data[1] == 0 || a == 0)
	{
		if ((badguy(actor) && actor->spr.extra <= 0) || (actor->opos.X != actor->spr.pos.X) || (actor->opos.Y != actor->spr.pos.Y))
		{
			actor->backupvec2();
			SetActor(actor, actor->spr.pos);
		}
		if (badguy(actor) && actor->spr.extra <= 0)
		{
			if (actor->sector()->ceilingstat & CSTAT_SECTOR_SKY)
			{
				if (actor->sector()->shadedsector == 1)
				{
					actor->spr.shade += (16 - actor->spr.shade) >> 1;
				}
				else
				{
					actor->spr.shade += (actor->sector()->ceilingshade - actor->spr.shade) >> 1;
				}
			}
			else
			{
				actor->spr.shade += (actor->sector()->floorshade - actor->spr.shade) >> 1;
			}
		}
		return;
	}

	auto moveptr = &ScriptCode[actor->temp_data[1]];

	if (a & geth) actor->vel.X += (moveptr[0] / 16. - actor->vel.X) * 0.5;
	if (a & getv) actor->vel.Z += (moveptr[1] / 16. - actor->vel.Z) * 0.5;

	if (a & dodgebullet)
		dodge(actor);

	if (actor->spr.picnum != APLAYER)
		alterang(a, actor, pnum);

	if (abs(actor->vel.X) < 6 / 16.) actor->vel.X = 0;

	a = badguy(actor);

	if (actor->vel.X != 0 || actor->vel.Z != 0)
	{
		if (a)
		{
			if (actor->spr.picnum == DRONE && actor->spr.extra > 0)
			{
				if (actor->vel.Z > 0)
				{
					double dist = isRRRA() ? 28 : 30;
					double f = getflorzofslopeptr(actor->sector(), actor->spr.pos.X, actor->spr.pos.Y);
					actor->floorz = f;
					if (actor->spr.pos.Z > f - dist)
						actor->spr.pos.Z = f - dist;
				}
				else
				{
					double c = getceilzofslopeptr(actor->sector(), actor->spr.pos.X, actor->spr.pos.Y);
					actor->ceilingz = c;
					if (actor->spr.pos.Z < c + 50)
					{
						actor->spr.pos.Z = c + 50;
						actor->vel.Z = 0;
					}
				}
			}
			if (actor->vel.Z > 0 && actor->floorz < actor->spr.pos.Z)
				actor->spr.pos.Z = actor->floorz;
			if (actor->vel.Z < 0)
			{
				double c = getceilzofslopeptr(actor->sector(), actor->spr.pos.X, actor->spr.pos.Y);
				if (actor->spr.pos.Z < c + 66)
				{
					actor->spr.pos.Z = c + 66;
					actor->vel.Z *= 0.5;
				}
			}
		}
		else if (actor->spr.picnum == APLAYER)
			if ((actor->spr.pos.Z - actor->ceilingz) < 32)
				actor->spr.pos.Z = actor->ceilingz + 32;

		daxvel = actor->vel.X;
		angdif = actor->spr.angle;

		if (a)
		{
			if (xvel < 960 && actor->spr.scale.X > 0.25 )
			{

				daxvel = -(1024 - xvel) * maptoworld;
				angdif = (ps[pnum].pos.XY() - actor->spr.pos.XY()).Angle();

				if (xvel < 512)
				{
					ps[pnum].vel.X = 0;
					ps[pnum].vel.Y = 0;
				}
				else
				{
					ps[pnum].vel.XY() *= gs.playerfriction - 0.125;
				}
			}
			else if ((isRRRA() && actor->spr.picnum != DRONE && actor->spr.picnum != SHARK && actor->spr.picnum != UFO1_RRRA) ||
					(!isRRRA() && actor->spr.picnum != DRONE && actor->spr.picnum != SHARK && actor->spr.picnum != UFO1_RR
							&& actor->spr.picnum != UFO2 && actor->spr.picnum != UFO3 && actor->spr.picnum != UFO4 && actor->spr.picnum != UFO5))
			{
				if (!*(moveptr + 1))
				{
					if (actor->opos.Z != actor->spr.pos.Z || (ud.multimode < 2 && ud.player_skill < 2))
					{
						if ((actor->temp_data[0] & 1) || ps[pnum].actorsqu == actor) return;
						else daxvel *= 2;
					}
					else
					{
						if ((actor->temp_data[0] & 3) || ps[pnum].actorsqu == actor) return;
						else daxvel *= 4;
					}
				}
			}
		}
		if (isRRRA())
		{
			if (actor->sector()->lotag != 1)
			{
				switch (actor->spr.picnum)
				{
				case MINIONBOAT:
				case HULKBOAT:
				case CHEERBOAT:
					daxvel *= 0.5;
					break;
				}
			}
			else if (actor->sector()->lotag == 1)
			{
				switch (actor->spr.picnum)
				{
				case BIKERB:
				case BIKERBV2:
				case CHEERB:
					daxvel *= 0.5;
					break;
				}
			}
		}

		Collision coll;
		actor->movflag = movesprite_ex(actor, DVector3(angdif.ToVector() * daxvel, actor->vel.Z), CLIPMASK0, coll);
	}

	if (a)
	{
		if (actor->sector()->ceilingstat & CSTAT_SECTOR_SKY)
		{
			if (actor->sector()->shadedsector == 1)
			{
				actor->spr.shade += (16 - actor->spr.shade) >> 1;
			}
			else
			{
				actor->spr.shade += (actor->sector()->ceilingshade - actor->spr.shade) >> 1;
			}
		}
		else actor->spr.shade += (actor->sector()->floorshade - actor->spr.shade) >> 1;

		if (actor->sector()->floorpicnum == MIRROR)
			actor->Destroy();
	}
}

void fakebubbaspawn(DDukeActor *actor, int g_p)
{
	fakebubba_spawn++;
	switch (fakebubba_spawn)
	{
	default:
		break;
	case 1:
		spawn(actor, PIG);
		break;
	case 2:
		spawn(actor, MINION);
		break;
	case 3:
		spawn(actor, CHEER);
		break;
	case 4:
		spawn(actor, VIXEN);
		operateactivators(666, &ps[g_p]);
		break;
	}
}

//---------------------------------------------------------------------------
//
// special checks in fall that only apply to RR.
//
//---------------------------------------------------------------------------

static int fallspecial(DDukeActor *actor, int playernum)
{
	int sphit = 0;
	if (isRRRA())
	{
		if (actor->sector()->lotag == 801)
		{
			if (actor->spr.picnum == ROCK)
			{
				spawn(actor, ROCK2);
				spawn(actor, ROCK2);
				addspritetodelete();
			}
			return 0;
		}
		else if (actor->sector()->lotag == 802)
		{
			if (actor->spr.picnum != APLAYER && badguy(actor) && actor->spr.pos.Z == actor->floorz - FOURSLEIGHT_F)
			{
				fi.guts(actor, JIBS6, 5, playernum);
				S_PlayActorSound(SQUISHED, actor);
				addspritetodelete();
			}
			return 0;
		}
		else if (actor->sector()->lotag == 803)
		{
			if (actor->spr.picnum == ROCK2)
				addspritetodelete();
			return 0;
		}
	}
	if (actor->sector()->lotag == 800)
	{
		if (actor->spr.picnum == 40)
		{
			addspritetodelete();
			return 0;
		}
		if (actor->spr.picnum != APLAYER && (badguy(actor) || actor->spr.picnum == HEN || actor->spr.picnum == COW || actor->spr.picnum == PIG || actor->spr.picnum == DOGRUN || actor->spr.picnum == RABBIT) && (!isRRRA() || actor->spriteextra < 128))
		{
			actor->spr.pos.Z = actor->floorz - FOURSLEIGHT_F;
			actor->vel.Z = 8000 / 256.;
			actor->spr.extra = 0;
			actor->spriteextra++;
			sphit = 1;
		}
		else if (actor->spr.picnum != APLAYER)
		{
			if (!actor->spriteextra)
				addspritetodelete();
			return 0;
		}
		actor->attackertype = SHOTSPARK1;
		actor->hitextra = 1;
	}
	else if (isRRRA() && (actor->sector()->floorpicnum == RRTILE7820 || actor->sector()->floorpicnum == RRTILE7768))
	{
		if (actor->spr.picnum != MINION && actor->spr.pal != 19)
		{
			if ((krand() & 3) == 1)
			{
				actor->attackertype = SHOTSPARK1;
				actor->hitextra = 5;
			}
		}
	}	
	return sphit;
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void fall_r(DDukeActor* ac, int g_p)
{
	fall_common(ac, g_p, JIBS6, DRONE, BLOODPOOL, SHOTSPARK1, 69, 158, fallspecial);
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void destroyit(DDukeActor *actor)
{
	int lotag = 0, hitag = 0;
	DDukeActor* spr = nullptr;

	DukeSectIterator it1(actor->sector());
	while (auto a2 = it1.Next())
	{
		if (a2->spr.picnum == RRTILE63)
		{
			lotag = a2->spr.lotag;
			spr = a2;
			if (a2->spr.hitag)
				hitag = a2->spr.hitag;
		}
	}
	DukeStatIterator it(STAT_DESTRUCT);
	while (auto a2 = it.Next())
	{
		auto it_sect = a2->sector();
		if (hitag && hitag == a2->spr.hitag)
		{
			DukeSectIterator its(it_sect);
			while (auto a3 = its.Next())
			{
				if (a3->spr.picnum == DESTRUCTO)
				{
					a3->attackertype = SHOTSPARK1;
					a3->hitextra = 1;
				}
			}
		}
		if (spr && spr->sector() != it_sect)
			if (lotag == a2->spr.lotag)
			{
				auto sect = spr->sector();

				auto destsect = spr->sector();
				auto srcsect = it_sect;

				auto destwal = destsect->walls.Data();
				auto srcwal = srcsect->walls.Data();
				for (unsigned i = 0; i < destsect->walls.Size(); i++, srcwal++, destwal++)
				{
					destwal->picnum = srcwal->picnum;
					destwal->overpicnum = srcwal->overpicnum;
					destwal->shade = srcwal->shade;
					destwal->xrepeat = srcwal->xrepeat;
					destwal->yrepeat = srcwal->yrepeat;
					destwal->xpan_ = srcwal->xpan_;
					destwal->ypan_ = srcwal->ypan_;
					if (isRRRA() && destwal->twoSided())
					{
						destwal->cstat = 0;
						destwal->nextWall()->cstat = 0;
					}
				}
				destsect->setfloorz(srcsect->floorz);
				destsect->setceilingz(srcsect->ceilingz);
				destsect->ceilingstat = srcsect->ceilingstat;
				destsect->floorstat = srcsect->floorstat;
				destsect->ceilingpicnum = srcsect->ceilingpicnum;
				destsect->ceilingheinum = srcsect->ceilingheinum;
				destsect->ceilingshade = srcsect->ceilingshade;
				destsect->ceilingpal = srcsect->ceilingpal;
				destsect->ceilingxpan_ = srcsect->ceilingxpan_;
				destsect->ceilingypan_ = srcsect->ceilingypan_;
				destsect->floorpicnum = srcsect->floorpicnum;
				destsect->floorheinum = srcsect->floorheinum;
				destsect->floorshade = srcsect->floorshade;
				destsect->floorpal = srcsect->floorpal;
				destsect->floorxpan_ = srcsect->floorxpan_;
				destsect->floorypan_ = srcsect->floorypan_;
				destsect->visibility = srcsect->visibility;
				destsect->keyinfo = srcsect->keyinfo;
				destsect->lotag = srcsect->lotag;
				destsect->hitag = srcsect->hitag;
				destsect->extra = srcsect->extra;
				destsect->dirty = EDirty::AllDirty;
			}
	}
	it1.Reset(actor->sector());
	while (auto a2 = it1.Next())
	{
		switch (a2->spr.picnum)
		{
		case DESTRUCTO:
		case RRTILE63:
		case TORNADO:
		case APLAYER:
		case COOT:
			break;
		default:
			a2->Destroy();
			break;
		}
	}
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void mamaspawn(DDukeActor *actor)
{
	if (mamaspawn_count)
	{
		mamaspawn_count--;
		spawn(actor, RABBIT);
	}
}

bool spawnweapondebris_r(int picnum)
{
	return true;
}

void respawnhitag_r(DDukeActor *actor)
{
	switch (actor->spr.picnum)
	{
	case FEM10:
	case NAKED1:
	case STATUE:
		if (actor->spr.yint) operaterespawns(actor->spr.yint);
		break;
	default:
		if (actor->spr.hitag >= 0) operaterespawns(actor->spr.hitag);
		break;
	}
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void thunder(void);

void think_r(void)
{
	thinktime.Reset();
	thinktime.Clock();
	recordoldspritepos();

	movefta();			//ST 2
	moveweapons_r();		//ST 4
	movetransports_r();		//ST 9
	moveplayers();			//ST 10
	movefallers_r();		//ST 12
	moveexplosions_r();		//ST 5

	actortime.Reset();
	actortime.Clock();
	moveactors_r();			//ST 1
	actortime.Unclock();

	moveeffectors_r();		//ST 3
	movestandables_r();		//ST 6
	doanimations();
	movefx();				//ST 11

	if (numplayers < 2 && thunderon)
		thunder();

	thinktime.Unclock();
}


END_DUKE_NS
