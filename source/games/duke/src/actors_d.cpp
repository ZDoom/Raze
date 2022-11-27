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

This file contains parts of DukeGDX by Alexander Makarov-[M210] (m210-2007@mail.ru)

*/
//-------------------------------------------------------------------------

#include "ns.h"
#include "global.h"
#include "names_d.h"
#include "serializer.h"
#include "dukeactor.h"

BEGIN_DUKE_NS


//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void check_fta_sounds_d(DDukeActor* actor)
{
	if (actor->spr.extra > 0) switch (actor->spr.picnum)
	{
	case LIZTROOPONTOILET:
	case LIZTROOPJUSTSIT:
	case LIZTROOPSHOOT:
	case LIZTROOPJETPACK:
	case LIZTROOPDUCKING:
	case LIZTROOPRUNNING:
	case LIZTROOP:
		S_PlayActorSound(PRED_RECOG, actor);
		break;
	case LIZMAN:
	case LIZMANSPITTING:
	case LIZMANFEEDING:
	case LIZMANJUMP:
		S_PlayActorSound(CAPT_RECOG, actor);
		break;
	case PIGCOP:
	case PIGCOPDIVE:
		S_PlayActorSound(PIG_RECOG, actor);
		break;
	case RECON:
		S_PlayActorSound(RECO_RECOG, actor);
		break;
	case DRONE:
		S_PlayActorSound(DRON_RECOG, actor);
		break;
	case COMMANDER:
	case COMMANDERSTAYPUT:
		S_PlayActorSound(COMM_RECOG, actor);
		break;
	case ORGANTIC:
		S_PlayActorSound(TURR_RECOG, actor);
		break;
	case OCTABRAIN:
	case OCTABRAINSTAYPUT:
		S_PlayActorSound(OCTA_RECOG, actor);
		break;
	case BOSS1:
		S_PlaySound(BOS1_RECOG);
		break;
	case BOSS2:
		if (actor->spr.pal == 1)
			S_PlaySound(BOS2_RECOG);
		else S_PlaySound(WHIPYOURASS);
		break;
	case BOSS3:
		if (actor->spr.pal == 1)
			S_PlaySound(BOS3_RECOG);
		else S_PlaySound(RIPHEADNECK);
		break;
	case BOSS4:
	case BOSS4STAYPUT:
		if (actor->spr.pal == 1)
			S_PlaySound(BOS4_RECOG);
		S_PlaySound(BOSS4_FIRSTSEE);
		break;
	case GREENSLIME:
		S_PlayActorSound(SLIM_RECOG, actor);
		break;
	}
}


//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void addweapon_d(player_struct *p, int weapon, bool wswitch)
{
	if ( p->gotweapon[weapon] == 0 )
	{
		p->gotweapon[weapon] = true;
		if (weapon == SHRINKER_WEAPON)
			p->gotweapon[GROW_WEAPON] = true;
	}
	if (!wswitch) return;

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
	p->curr_weapon = weapon;
	p->wantweaponfire = -1;

	switch (weapon)
	{
	case KNEE_WEAPON:
	case TRIPBOMB_WEAPON:
	case HANDREMOTE_WEAPON:
	case HANDBOMB_WEAPON:	 
		break;
	case SHOTGUN_WEAPON:	  
		S_PlayActorSound(SHOTGUN_COCK, p->GetActor()); 
		break;
	case PISTOL_WEAPON:	   
		S_PlayActorSound(INSERT_CLIP, p->GetActor());
		break;
	default:	  
		S_PlayActorSound(SELECT_WEAPON, p->GetActor());
		break;
	}
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

bool ifsquished(DDukeActor* actor, int p)
{
	if (isRR()) return false;	// this function is a no-op in RR's source.

	bool squishme = false;
	if (actor->isPlayer() && ud.clipping)
		return false;

	auto sectp = actor->sector();
	double floorceildist = sectp->floorz - sectp->ceilingz;

	if (sectp->lotag != ST_23_SWINGING_DOOR)
	{
		if (actor->spr.pal == 1)
			squishme = floorceildist < 32 && (sectp->lotag & 32768) == 0;
		else
			squishme = floorceildist < 12;
	}

	if (squishme)
	{
		FTA(QUOTE_SQUISHED, &ps[p]);

		if (badguy(actor))
			actor->vel.X = 0;

		if (actor->spr.pal == 1)
		{
			actor->attackertype = SHOTSPARK1;
			actor->hitextra = 1;
			return false;
		}

		return true;
	}
	return false;
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void hitradius_d(DDukeActor* actor, int  r, int  hp1, int  hp2, int  hp3, int  hp4)
{
	double radius = r * inttoworld;
	static const uint8_t statlist[] = { STAT_DEFAULT, STAT_ACTOR, STAT_STANDABLE, STAT_PLAYER, STAT_FALLER, STAT_ZOMBIEACTOR, STAT_MISC };

	if(actor->spr.picnum != SHRINKSPARK && !(actor->spr.picnum == RPG && actor->spr.scale.X < 0.171875))
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

	double q = zrand(32) - 16;

	auto Owner = actor->GetOwner();
	for (int x = 0; x < 7; x++)
	{
		DukeStatIterator itj(statlist[x]);
		while (auto act2 = itj.Next())
		{
			if (isWorldTour() && Owner)
			{
				if (Owner->isPlayer() && act2->isPlayer() && ud.coop != 0 && ud.ffire == 0 && Owner != act2)
				{
					continue;
				}

				if (actor->spr.picnum == FLAMETHROWERFLAME && ((Owner->spr.picnum == FIREFLY && act2->spr.picnum == FIREFLY) || (Owner->spr.picnum == BOSS5 && act2->spr.picnum == BOSS5)))
				{
					continue;
				}
			}

			if (x == 0 || x >= 5 || actorflag(act2, SFLAG_HITRADIUS_FLAG1))
			{
				if (actor->spr.picnum != SHRINKSPARK || (act2->spr.cstat & CSTAT_SPRITE_BLOCK_ALL))
					if ((actor->spr.pos - act2->spr.pos).Length() < radius)
					{
						if (badguy(act2) && !cansee(act2->spr.pos.plusZ(q), act2->sector(), actor->spr.pos.plusZ(q), actor->sector()))
							continue;
						fi.checkhitsprite(act2, actor);
					}
			}
			else if (act2->spr.extra >= 0 && act2 != actor && (actorflag(act2, SFLAG_HITRADIUS_FLAG2) || badguy(act2) || (act2->spr.cstat & CSTAT_SPRITE_BLOCK_ALL)))
			{
				if (actor->spr.picnum == SHRINKSPARK && act2->spr.picnum != SHARK && (act2 == Owner || act2->spr.scale.X < 0.375))
				{
					continue;
				}
				if (actor->spr.picnum == MORTER && act2 == Owner)
				{
					continue;
				}

				double dist = (actor->getPosWithOffsetZ() - act2->spr.pos).Length();

				if (dist < radius && cansee(act2->spr.pos.plusZ(-8), act2->sector(), actor->spr.pos.plusZ(-12), actor->sector()))
				{
					act2->hitang = (act2->spr.pos - actor->spr.pos).Angle();

					if (actor->spr.picnum == RPG && act2->spr.extra > 0)
						act2->attackertype = RPG;
					else if (!isWorldTour())
					{
						if (actor->spr.picnum == SHRINKSPARK)
							act2->attackertype = SHRINKSPARK;
						else act2->attackertype = RADIUSEXPLOSION;
					}
					else
					{
						if (actor->spr.picnum == SHRINKSPARK || actor->spr.picnum == FLAMETHROWERFLAME)
							act2->	attackertype = actor->spr.picnum;
						else if (actor->spr.picnum != FIREBALL || !Owner || !Owner->isPlayer())
						{
							if (actor->spr.picnum == LAVAPOOL)
								act2->attackertype = FLAMETHROWERFLAME;
							else
								act2->attackertype = RADIUSEXPLOSION;
						}
						else
							act2->attackertype = FLAMETHROWERFLAME;
					}

					if (actor->spr.picnum != SHRINKSPARK && (!isWorldTour() || actor->spr.picnum != LAVAPOOL))
					{
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

						if (!actorflag(act2, SFLAG2_NORADIUSPUSH) && !bossguy(act2))
						{
							if (act2->vel.X < 0) act2->vel.X = 0;
							act2->vel.X += ( (actor->spr.extra / 4.));
						}

						if (actorflag(act2, SFLAG_HITRADIUSCHECK))
							fi.checkhitsprite(act2, actor);
					}
					else if (actor->spr.extra == 0) act2->hitextra = 0;

					if (act2->spr.picnum != RADIUSEXPLOSION && Owner && Owner->spr.statnum < MAXSTATUS)
					{
						if (act2->isPlayer())
						{
							int p = act2->spr.yint;

							if (isWorldTour() && act2->attackertype == FLAMETHROWERFLAME && Owner->isPlayer())
							{
								ps[p].numloogs = -1 - actor->spr.yint;
							}

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


int movesprite_ex_d(DDukeActor* actor, const DVector3& change, unsigned int cliptype, Collision &result)
{
	int bg = badguy(actor);

	if (actor->spr.statnum == STAT_MISC || (bg && actor->spr.scale.X < 0.0625))
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
			double clipdist;
			if (actor->spr.picnum == LIZMAN)
				clipdist = 18.25;
			else if (actorflag(actor, SFLAG_BADGUY))
				clipdist = actor->clipdist;
			else
				clipdist = 12;

			clipmove(ppos, &dasectp, change * 0.5, clipdist, 4., 4., cliptype, result);
		}

		// conditional code from hell...
		if (dasectp == nullptr || (dasectp != nullptr &&
			((actor->actorstayput != nullptr && actor->actorstayput != dasectp) ||
			 ((actor->spr.picnum == BOSS2) && actor->spr.pal == 0 && dasectp->lotag != 3) ||
			 ((actor->spr.picnum == BOSS1 || actor->spr.picnum == BOSS2) && dasectp->lotag == ST_1_ABOVE_WATER) ||
			 (dasectp->lotag == ST_1_ABOVE_WATER && (actor->spr.picnum == LIZMAN || (actor->spr.picnum == LIZTROOP && actor->vel.Z == 0)))
			))
		 )
		{
			if (dasectp && dasectp->lotag == ST_1_ABOVE_WATER && actor->spr.picnum == LIZMAN)
				actor->spr.Angles.Yaw = randomAngle();
			else if ((actor->temp_data[0]&3) == 1 && actor->spr.picnum != COMMANDER)
				actor->spr.Angles.Yaw = randomAngle();
			SetActor(actor,actor->spr.pos);
			if (dasectp == nullptr) dasectp = &sector[0];
			return result.setSector(dasectp);
		}
		if ((result.type == kHitWall || result.type == kHitSprite) && (actor->cgg == 0)) actor->spr.Angles.Yaw += DAngle90 + DAngle45;
	}
	else
	{
		if (actor->spr.statnum == STAT_PROJECTILE)
			clipmove(ppos, &dasectp, change * 0.5, 0.5, 4., 4., cliptype, result);
		else
			clipmove(ppos, &dasectp, change * 0.5, actor->clipdist, 4., 4., cliptype, result);
	}
	actor->spr.pos.XY() = ppos.XY();

	if (dasectp != nullptr && dasectp != actor->sector())
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

void lotsofmoney_d(DDukeActor *actor, int n)
{
	lotsofstuff(actor, n, MONEY);
}

void lotsofmail_d(DDukeActor *actor, int n)
{
	lotsofstuff(actor, n, MAIL);
}

void lotsofpaper_d(DDukeActor *actor, int n)
{
	lotsofstuff(actor, n, PAPER);
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

int ifhitbyweapon_d(DDukeActor *actor)
{
	int p;
	auto hitowner = actor->GetHitOwner();

	if (actor->hitextra >= 0)
	{
		if (actor->spr.extra >= 0)
		{
			if (actor->isPlayer())
			{
				if (ud.god && actor->attackertype != SHRINKSPARK) return -1;

				p = actor->PlayerIndex();

				if (hitowner &&
					hitowner->isPlayer() &&
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

						if (hitowner->isPlayer() && p != hitowner->PlayerIndex())
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
					if (actor->attackertype == SHRINKSPARK && actor->spr.scale.X < 0.375)
						return -1;

				if (isWorldTour() && actor->attackertype == FIREFLY && actor->spr.scale.X < 0.75)
				{
					if (actor->attackertype != RADIUSEXPLOSION && actor->attackertype != RPG)
						return -1;
				}

				actor->spr.extra -= actor->hitextra;
				auto Owner = actor->GetOwner();
				if (actor->spr.picnum != RECON && Owner && Owner->spr.statnum < MAXSTATUS)
					actor->SetOwner(hitowner);
			}

			actor->hitextra = -1;
			return actor->attackertype;
		}
	}


	if (ud.multimode < 2 || !isWorldTour()
		|| actor->attackertype != FLAMETHROWERFLAME
		|| actor->hitextra >= 0
		|| actor->spr.extra > 0
		|| !actor->isPlayer()
		|| ps[actor->PlayerIndex()].numloogs > 0
		|| hitowner == nullptr)
	{
		actor->hitextra = -1;
		return -1;
	}
	else
	{
		p = actor->PlayerIndex();
		actor->spr.extra = 0;
		ps[p].wackedbyactor = hitowner;

		if (hitowner->isPlayer() && hitowner != ps[p].GetActor())
			ps[p].frag_ps = hitowner->PlayerIndex(); // set the proper player index here - this previously set the sprite index...

		actor->SetHitOwner(ps[p].GetActor());
		actor->hitextra = -1;

		return FLAMETHROWERFLAME;
	}
}


//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void movefallers_d(void)
{
	int j;

	DukeStatIterator iti(STAT_FALLER);
	while (auto act = iti.Next())
	{
		auto sectp = act->sector();

		if (act->temp_data[0] == 0)
		{
			act->spr.pos.Z -= 16;
			DAngle saved_angle = act->spr.Angles.Yaw;
			int x = act->spr.extra;
			j = fi.ifhitbyweapon(act);
			if (j >= 0)
			{
				if (gs.actorinfo[j].flags2 & SFLAG2_EXPLOSIVE)
				{
					if (act->spr.extra <= 0)
					{
						act->temp_data[0] = 1;
						DukeStatIterator itj(STAT_FALLER);
						while (auto a2 = itj.Next())
						{
							if (a2->spr.hitag == act->spr.hitag)
							{
								a2->temp_data[0] = 1;
								a2->spr.cstat &= ~CSTAT_SPRITE_ONE_SIDE;
								if (a2->spr.picnum == CEILINGSTEAM || a2->spr.picnum == STEAM)
									a2->spr.cstat |= CSTAT_SPRITE_INVISIBLE;
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
			act->spr.Angles.Yaw = saved_angle;
			act->spr.pos.Z += 16;
		}
		else if (act->temp_data[0] == 1)
		{
			if (act->spr.lotag > 0)
			{
				act->spr.lotag-=3;
				if (act->spr.lotag <= 0)
				{
					act->vel.X = 2 + krandf(4);
					act->vel.Z = -4 + krandf(4);
				}
			}
			else
			{
				if (act->vel.X > 0)
				{
					act->vel.X -= 0.5;
					ssp(act, CLIPMASK0);
				}

				double grav;
				if (floorspace(act->sector())) grav = 0;
				else
				{
					if (ceilingspace(act->sector()))
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
					j = 1 + (krand() & 7);
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

void movestandables_d(void)
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
				FLOORFLAME,
				FIREBARREL,
				FIREVASE,
				NUKEBARREL,
				NUKEBARRELDENTED,
				NUKEBARRELLEAKED,
				TOILETWATER,
				RUBBERCAN,
				STEAM,
				CEILINGSTEAM,
				WATERBUBBLEMAKER))
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

static bool movefireball(DDukeActor* actor)
{
	auto Owner = actor->GetOwner();

	if (actor->sector()->lotag == 2)
	{
		actor->Destroy();
		return true;
	}

	if (!Owner || Owner->spr.picnum != FIREBALL)
	{
		if (actor->temp_data[0] >= 1 && actor->temp_data[0] < 6)
		{
			float siz = 1.0f - (actor->temp_data[0] * 0.2f);
			DDukeActor* trail = actor->temp_actor;
			auto ball = spawn(actor, FIREBALL);
			if (ball)
			{
				actor->temp_actor = ball;

				ball->vel.X = actor->vel.X;
				ball->vel.Z = actor->vel.Z;
				if (actor->temp_data[0] > 1)
				{
					if (trail)
					{
						ball->spr.pos = trail->temp_pos;
						ball->vel = trail->temp_pos2;
					}
				}
				double scale = actor->spr.scale.X * siz;
				ball->spr.scale = DVector2(scale, scale);
				ball->spr.cstat = actor->spr.cstat;
				ball->spr.extra = 0;

				ball->temp_pos = ball->spr.pos;
				ball->temp_pos2 = ball->vel;

				ChangeActorStat(ball, STAT_PROJECTILE);
			}
		}
		actor->temp_data[0]++;
	}
	if (actor->vel.Z < 15000. / 256.)
		actor->vel.Z += 200 / 256.;
	return false;
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

static bool weaponhitsprite(DDukeActor* proj, DDukeActor *targ, bool fireball)
{
	if (proj->spr.picnum == FREEZEBLAST && targ->spr.pal == 1)
		if (badguy(targ) || targ->isPlayer())
		{
			auto spawned = spawn(targ, TRANSPORTERSTAR);
			if (spawned)
			{
				spawned->spr.pal = 1;
				spawned->spr.scale = DVector2(0.5, 0.5);
			}

			proj->Destroy();
			return true;
		}

	if (!isWorldTour() || proj->spr.picnum != FIREBALL || fireball)
		fi.checkhitsprite(targ, proj);

	if (targ->isPlayer())
	{
		int p = targ->spr.yint;
		auto Owner = proj->GetOwner();

		if (ud.multimode >= 2 && fireball && Owner && Owner->isPlayer())
		{
			ps[p].numloogs = -1 - proj->spr.yint;
		}

		S_PlayActorSound(PISTOL_BODYHIT, targ);

		if (proj->spr.picnum == SPIT)
		{
			ps[p].Angles.addPitch(DAngle::fromDeg(-14.04));
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

static bool weaponhitwall(DDukeActor *proj, walltype* wal, const DVector3 &oldpos)
{
	if (proj->spr.picnum != RPG && proj->spr.picnum != FREEZEBLAST && proj->spr.picnum != SPIT &&
		(!isWorldTour() || proj->spr.picnum != FIREBALL) &&
		(wal->overpicnum == MIRROR || wal->picnum == MIRROR))
	{
		DAngle k = wal->delta().Angle();
		proj->spr.Angles.Yaw = k * 2 - proj->spr.Angles.Yaw;
		proj->SetOwner(proj);
		spawn(proj, TRANSPORTERSTAR);
		return true;
	}
	else
	{
		SetActor(proj, oldpos);
		fi.checkhitwall(proj, wal, proj->spr.pos, proj->spr.picnum);

		if (proj->spr.picnum == FREEZEBLAST)
		{
			if (wal->overpicnum != MIRROR && wal->picnum != MIRROR)
			{
				proj->spr.extra >>= 1;
				proj->spr.yint--;
			}

			DAngle k = wal->delta().Angle();
			proj->spr.Angles.Yaw = k * 2 - proj->spr.Angles.Yaw;
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

static bool weaponhitsector(DDukeActor* proj, const DVector3& oldpos, bool fireball)
{
	SetActor(proj, oldpos);

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
	else if (fireball)
	{
		auto spawned = spawn(proj, LAVAPOOL);
		if (spawned)
		{
			spawned->SetOwner(proj);
			spawned->SetHitOwner(proj);
			spawned->spr.yint = proj->spr.yint;
		}
		proj->Destroy();
		return true;
	}

	if (proj->spr.picnum == FREEZEBLAST)
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

static void weaponcommon_d(DDukeActor* proj)
{
	if (proj->spr.picnum == COOLEXPLOSION1)
		if (!S_CheckActorSoundPlaying(proj, WIERDSHOT_FLY))
			S_PlayActorSound(WIERDSHOT_FLY, proj);

	double vel = proj->vel.X;
	double velz = proj->vel.Z;

	int p = -1;

	if (proj->spr.picnum == RPG && proj->sector()->lotag == 2)
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

	case FIREBALL:
		if (movefireball(proj)) return;
		break;
	}

	Collision coll;
	movesprite_ex(proj, DVector3(proj->spr.Angles.Yaw.ToVector() * vel, velz), CLIPMASK1, coll);


	if (proj->spr.picnum == RPG && proj->temp_actor != nullptr)
		if ((proj->spr.pos.XY() - proj->temp_actor->spr.pos.XY()).Length() < 16)
			coll.setSprite(proj->temp_actor);

	if (!proj->insector())
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
			if ((proj->spr.pos.Z > proj->floorz && proj->sector()->lotag != 1) ||
				(proj->spr.pos.Z > proj->floorz + 16 && proj->sector()->lotag == 1))
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
			auto spawned = CreateActor(proj->sector(), proj->spr.pos.plusZ(zAdd) + proj->spr.Angles.Yaw.ToVector() * k * 2.,
				FIRELASER, -40 + (k << 2),
				proj->spr.scale, nullAngle, 0., 0., proj->GetOwner(), 5);

			if (spawned)
			{
				spawned->opos = proj->opos - proj->spr.pos + spawned->spr.pos;
				spawned->spr.cstat = CSTAT_SPRITE_YCENTER;
				spawned->spr.pal = proj->spr.pal;
			}
		}
	}
	else if (proj->spr.picnum == SPIT) if (proj->vel.Z < 24)
		proj->vel.Z += gs.gravity - 112 / 256.;

	if (coll.type != 0)
	{
		if (proj->spr.picnum == COOLEXPLOSION1)
		{
			if (coll.type == kHitSprite && !coll.actor()->isPlayer())
			{
				return;
			}
			proj->vel.X = 0;
			proj->vel.Z = 0;
		}

		bool fireball = (isWorldTour() && proj->spr.picnum == FIREBALL && (!proj->GetOwner() || proj->GetOwner()->spr.picnum != FIREBALL));

		if (coll.type == kHitSprite)
		{
			if (weaponhitsprite(proj, coll.actor(), fireball)) return;
		}
		else if (coll.type == kHitWall)
		{
			if (weaponhitwall(proj, coll.hitWall, oldpos)) return;
		}
		else if (coll.type == kHitSector)
		{
			if (weaponhitsector(proj, oldpos, fireball)) return;
		}

		if (proj->spr.picnum != SPIT)
		{
			if (proj->spr.picnum == RPG)
			{
				// j is only needed for the hit type mask.
				rpgexplode(proj, coll.type, oldpos, EXPLOSION2, EXPLOSION2BOT, -1, RPG_EXPLODE);
			}
			else if (proj->spr.picnum == SHRINKSPARK)
			{
				spawn(proj, SHRINKEREXPLOSION);
				S_PlayActorSound(SHRINKER_HIT, proj);
				fi.hitradius(proj, gs.shrinkerblastradius, 0, 0, 0, 0);
			}
			else if (proj->spr.picnum != COOLEXPLOSION1 && proj->spr.picnum != FREEZEBLAST && proj->spr.picnum != FIRELASER && (!isWorldTour() || proj->spr.picnum != FIREBALL))
			{
				auto spawned = spawn(proj, EXPLOSION2);
				if (spawned)
				{
					auto scale = proj->spr.scale.X * 0.5;
					spawned->spr.scale = DVector2(scale,scale);
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
			if (fireball)
			{
				auto spawned = spawn(proj, EXPLOSION2);
				if (spawned)
				{
					auto scale = proj->spr.scale.X * 0.5;
					spawned->spr.scale = DVector2(scale,scale);
				}
			}
		}
		if (proj->spr.picnum != COOLEXPLOSION1)
		{
			proj->Destroy();
			return;
		}
	}
	if (proj->spr.picnum == COOLEXPLOSION1)
	{
		proj->spr.shade++;
		if (proj->spr.shade >= 40)
		{
			proj->Destroy();
			return;
		}
	}
	else if (proj->spr.picnum == RPG && proj->sector()->lotag == 2 && proj->spr.scale.X >= 0.15625 && rnd(140))
		spawn(proj, WATERBUBBLE);

}
//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void moveweapons_d(void)
{
	DukeStatIterator it(STAT_PROJECTILE);
	while (auto act = it.Next())
	{
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

		switch(act->spr.picnum)
		{
		case FREEZEBLAST:
			if (act->spr.yint < 1 || act->spr.extra < 2 || (act->vel.X == 0 && act->vel.Z == 0))
			{
				auto spawned = spawn(act,TRANSPORTERSTAR);
				if (spawned)
				{
					spawned->spr.pal = 1;
					spawned->spr.scale = DVector2(0.5, 0.5);
				}
				act->Destroy();
				continue;
			}
			[[fallthrough]];
		case FIREBALL:
			// Twentieth Anniversary World Tour
			if (act->spr.picnum == FIREBALL && !isWorldTour()) break;
			[[fallthrough]];
		case SHRINKSPARK:
		case RPG:
		case FIRELASER:
		case SPIT:
		case COOLEXPLOSION1:
			weaponcommon_d(act);
			break;

		case SHOTSPARK1:
		{
			double x;
			int p = findplayer(act, &x);
			execute(act, p, x);
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

void movetransports_d(void)
{
	int warpspriteto;
	double ll;

	DukeStatIterator iti(STAT_TRANSPORT);
	while (auto act = iti.Next())
	{
		auto Owner = act->GetOwner();

		if (Owner == act)
		{
			continue;
		}

		auto sectp = act->sector();
		int sectlotag = sectp->lotag;
		int onfloorz = act->temp_data[4];

		if (act->temp_data[0] > 0) act->temp_data[0]--;

		DukeSectIterator itj(act->sector());
		while (auto act2 = itj.Next()) 
		{
			switch (act2->spr.statnum)
			{
			case STAT_PLAYER:

				if (act2->GetOwner())
				{
					int p = act2->PlayerIndex();

					ps[p].on_warping_sector = 1;

					if (ps[p].transporter_hold == 0 && ps[p].jumping_counter == 0)
					{
						if (ps[p].on_ground && sectlotag == 0 && onfloorz && ps[p].jetpack_on == 0)
						{
							if (act->spr.pal == 0)
							{
								spawn(act, TRANSPORTERBEAM);
								S_PlayActorSound(TELEPORTER, act);
							}

							for (int k = connecthead; k >= 0; k = connectpoint2[k])
							if (ps[k].cursector == Owner->sector())
							{
								ps[k].frag_ps = p;
								ps[k].GetActor()->spr.extra = 0;
							}

							ps[p].Angles.setYaw(Owner->spr.Angles.Yaw, true);

							if (Owner->GetOwner() != Owner)
							{
								act->temp_data[0] = 13;
								Owner->temp_data[0] = 13;
								ps[p].transporter_hold = 13;
							}

							ps[p].GetActor()->spr.pos = Owner->spr.pos;
							ps[p].GetActor()->backuppos();
							ps[p].setbobpos();

							ChangeActorSect(act2, Owner->sector());
							ps[p].setCursector(act2->sector());

							if (act->spr.pal == 0)
							{
								auto k = spawn(Owner, TRANSPORTERBEAM);
								if (k) S_PlayActorSound(TELEPORTER, k);
							}

							break;
						}
					}
					else if (!(sectlotag == 1 && ps[p].on_ground == 1)) break;

					if (onfloorz == 0 && abs(act->spr.pos.Z - ps[p].GetActor()->getOffsetZ()) < 24)
						if ((ps[p].jetpack_on == 0) || (ps[p].jetpack_on && (PlayerInput(p, SB_JUMP))) ||
							(ps[p].jetpack_on && PlayerInput(p, SB_CROUCH)))
						{
							ps[p].GetActor()->spr.pos.XY() += Owner->spr.pos.XY() - act->spr.pos.XY();
							ps[p].GetActor()->backupvec2();

							if (ps[p].jetpack_on && (PlayerInput(p, SB_JUMP) || ps[p].jetpack_on < 11))
								ps[p].GetActor()->spr.pos.Z = Owner->spr.pos.Z - 24 + gs.playerheight;
							else ps[p].GetActor()->spr.pos.Z = Owner->spr.pos.Z + 24 + gs.playerheight;
							ps[p].GetActor()->backupz();

							auto pa = ps[p].GetActor();
							pa->opos = ps[p].GetActor()->getPosWithOffsetZ();

							ChangeActorSect(act2, Owner->sector());
							ps[p].setCursector(Owner->sector());

							break;
						}

					int k = 0;

					if (onfloorz && sectlotag == ST_1_ABOVE_WATER && ps[p].on_ground && ps[p].GetActor()->getOffsetZ() > (sectp->floorz - 16) && (PlayerInput(p, SB_CROUCH) || ps[p].vel.Z > 8))
						// if( onfloorz && sectlotag == 1 && ps[p].pos.z > (sectp->floorz-(6<<8)) )
					{
						k = 1;
						if (screenpeek == p)
						{
							FX_StopAllSounds();
						}
						if (ps[p].GetActor()->spr.extra > 0)
							S_PlayActorSound(DUKE_UNDERWATER, act2);
						ps[p].GetActor()->spr.pos.Z = Owner->sector()->ceilingz + 7 + gs.playerheight;
						ps[p].GetActor()->backupz();

						// this is actually below the precision óf the original Build coordinate system...
						ps[p].vel.X = ((krand() & 8192) ? 1 / 64. : -1 / 64.);
						ps[p].vel.Y = ((krand() & 8192) ? 1 / 64. : -1 / 64.);

					}

					if (onfloorz && sectlotag == ST_2_UNDERWATER && ps[p].GetActor()->getOffsetZ() < (sectp->ceilingz + 6))
					{
						k = 1;
						//     if( act2->spr.extra <= 0) break;
						if (screenpeek == p)
						{
							FX_StopAllSounds();
						}
						S_PlayActorSound(DUKE_GASP, act2);

						ps[p].GetActor()->spr.pos.Z = Owner->sector()->floorz - 7 + gs.playerheight;
						ps[p].GetActor()->backupz();

						ps[p].jumping_toggle = 1;
						ps[p].jumping_counter = 0;
					}

					if (k == 1)
					{
						ps[p].GetActor()->spr.pos.XY() += Owner->spr.pos.XY() - act->spr.pos.XY();
						ps[p].GetActor()->backupvec2();

						if (!Owner || Owner->GetOwner() != Owner)
							ps[p].transporter_hold = -2;
						ps[p].setCursector(Owner->sector());

						ChangeActorSect(act2, Owner->sector());
						SetActor(act2, act2->spr.pos);

						if ((krand() & 255) < 32)
							spawn(act2, WATERSPLASH2);

						if (sectlotag == 1)
							for (int l = 0; l < 9; l++)
						{
							auto q = spawn(ps[p].GetActor(), WATERBUBBLE);
							if (q) q->spr.pos.Z += krandf(64);
						}
					}
				}
				break;

			case STAT_ACTOR:
				switch (act2->spr.picnum)
				{
				case SHARK:
				case COMMANDER:
				case OCTABRAIN:
				case GREENSLIME:
				case GREENSLIME + 1:
				case GREENSLIME + 2:
				case GREENSLIME + 3:
				case GREENSLIME + 4:
				case GREENSLIME + 5:
				case GREENSLIME + 6:
				case GREENSLIME + 7:
					if (act2->spr.extra > 0)
						continue;
				}
				[[fallthrough]];
			case STAT_PROJECTILE:
			case STAT_MISC:
			case STAT_FALLER:
			case STAT_DUMMYPLAYER:

				ll = abs(act2->vel.Z);

				{
					warpspriteto = 0;
					if (ll && sectlotag == 2 && act2->spr.pos.Z < (sectp->ceilingz + ll) && act2->vel.Z < 0)
						warpspriteto = 1;

					if (ll && sectlotag == 1 && act2->spr.pos.Z > (sectp->floorz - ll) && act2->vel.Z > 0)
						warpspriteto = 1;

					if (sectlotag == 0 && (onfloorz || abs(act2->spr.pos.Z - act->spr.pos.Z) < 16))
					{
						if ((!Owner || Owner->GetOwner() != Owner) && onfloorz && act->temp_data[0] > 0 && act2->spr.statnum != STAT_MISC)
						{
							act->temp_data[0]++;
							goto BOLT;
						}
						warpspriteto = 1;
					}

					if (warpspriteto)
					{
						if (actorflag(act2, SFLAG_NOTELEPORT)) continue;
						switch (act2->spr.picnum)
						{
						case PLAYERONWATER:
							if (sectlotag == 2)
							{
								act2->spr.cstat &= ~CSTAT_SPRITE_INVISIBLE;
								break;
							}
							[[fallthrough]];
						default:
							if (act2->spr.statnum == 5 && !(sectlotag == 1 || sectlotag == 2))
								break;
							[[fallthrough]];

						case WATERBUBBLE:
							//if( rnd(192) && a2->s.picnum == WATERBUBBLE)
							// break;

							if (sectlotag > 0)
							{
								auto k = spawn(act2, WATERSPLASH2);
								if (k && sectlotag == 1 && act2->spr.statnum == 4)
								{
									k->vel.X = act2->vel.X * 0.5;
									k->spr.Angles.Yaw = act2->spr.Angles.Yaw;
									ssp(k, CLIPMASK0);
								}
							}

							switch (sectlotag)
							{
							case ST_0_NO_EFFECT:
								if (onfloorz)
								{
									if (act2->spr.statnum == STAT_PROJECTILE || (checkcursectnums(act->sector()) == -1 && checkcursectnums(Owner->sector()) == -1))
									{
										act2->spr.pos += (Owner->spr.pos - act->spr.pos.XY()).plusZ(-Owner->sector()->floorz);
										act2->spr.Angles.Yaw = Owner->spr.Angles.Yaw;

										act2->backupang();

										if (act->spr.pal == 0)
										{
											auto k = spawn(act, TRANSPORTERBEAM);
											if (k) S_PlayActorSound(TELEPORTER, k);

											k = spawn(Owner, TRANSPORTERBEAM);
											if (k) S_PlayActorSound(TELEPORTER, k);
										}

										if (Owner && Owner->GetOwner() == Owner)
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
							}

							break;
						}
					}
				}
				break;

			}
		}
	BOLT:;
	}
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

static void greenslime(DDukeActor *actor)
{
	auto sectp = actor->sector();
	int j;

	// #ifndef isShareware()
	if (ud.multimode < 2)
	{
		if (actor_tog == 1)
		{
			actor->spr.cstat = CSTAT_SPRITE_INVISIBLE;
			return;
		}
		else if (actor_tog == 2) actor->spr.cstat = CSTAT_SPRITE_BLOCK_ALL;
	}
	// #endif

	actor->temp_data[1] += 128;

	if (sectp->floorstat & CSTAT_SECTOR_SKY)
	{
		actor->Destroy();
		return;
	}

	double xx;
	int p = findplayer(actor, &xx);

	if (xx > 1280)
	{
		actor->timetosleep++;
		if (actor->timetosleep > SLEEPTIME)
		{
			actor->timetosleep = 0;
			ChangeActorStat(actor, STAT_ZOMBIEACTOR);
			return;
		}
	}

	if (actor->temp_data[0] == -5) // FROZEN
	{
		actor->temp_data[3]++;
		if (actor->temp_data[3] > 280)
		{
			actor->spr.pal = 0;
			actor->temp_data[0] = 0;
			return;
		}
		makeitfall(actor);
		actor->spr.cstat = CSTAT_SPRITE_BLOCK_ALL;
		actor->spr.picnum = GREENSLIME + 2;
		actor->spr.extra = 1;
		actor->spr.pal = 1;
		j = fi.ifhitbyweapon(actor);
		if (j >= 0)
		{
			if (j == FREEZEBLAST)
				return;
			for (j = 16; j >= 0; j--)
			{
				auto a = randomAngle();
				auto vel = krandf(2) + 2;
				auto zvel = 4 - krandf(4);

				auto k = CreateActor(actor->sector(), actor->spr.pos, GLASSPIECES + (j % 3), -32, DVector2(0.5625, 0.5625), a, vel, zvel, actor, 5);
				k->spr.pal = 1;
			}
			ps[p].actors_killed++;
			S_PlayActorSound(GLASS_BREAKING, actor);
			actor->Destroy();
		}
		else if (xx < 64 && ps[p].quick_kick == 0)
		{
			auto ang = absangle(ps[p].GetActor()->spr.Angles.Yaw, (actor->spr.pos.XY() - ps[p].GetActor()->spr.pos.XY()).Angle());
			if (ang < DAngle22_5)
				ps[p].quick_kick = 14;
		}

		return;
	}

	if (xx < 99.75)
		actor->spr.cstat = 0;
	else actor->spr.cstat = CSTAT_SPRITE_BLOCK_ALL;

	if (actor->temp_data[0] == -4) //On the player
	{
		if (ps[p].GetActor()->spr.extra < 1)
		{
			actor->temp_data[0] = 0;
			return;
		}

		SetActor(actor, actor->spr.pos);

		actor->spr.Angles.Yaw = ps[p].GetActor()->spr.Angles.Yaw;

		if ((PlayerInput(p, SB_FIRE) || (ps[p].quick_kick > 0)) && ps[p].GetActor()->spr.extra > 0)
			if (ps[p].quick_kick > 0 || (ps[p].curr_weapon != HANDREMOTE_WEAPON && ps[p].curr_weapon != HANDBOMB_WEAPON && ps[p].curr_weapon != TRIPBOMB_WEAPON && ps[p].ammo_amount[ps[p].curr_weapon] >= 0))
			{
				for (int x = 0; x < 8; x++)
				{
					auto a = randomAngle();
					auto vel = krandf(4) + 4;
					auto zvel = -krandf(16) - actor->vel.Z * 0.25;

					auto spawned = CreateActor(actor->sector(), actor->spr.pos.plusZ(-8), PClass::FindActor("DukeScrap"), -8, DVector2(0.75, 0.75), a, vel, zvel, actor, STAT_MISC);
					if (spawned)
					{
						if (spawned) spawned->spriteextra = Scrap3 + (krand() & 3);
						spawned->spr.pal = 6;
					}
				}

				S_PlayActorSound(SLIM_DYING, actor);
				S_PlayActorSound(SQUISHED, actor);
				if ((krand() & 255) < 32)
				{
					auto spawned = spawn(actor, BLOODPOOL);
					if (spawned) spawned->spr.pal = 0;
				}
				ps[p].actors_killed++;
				actor->temp_data[0] = -3;
				if (ps[p].somethingonplayer == actor)
					ps[p].somethingonplayer = nullptr;
				actor->Destroy();
				return;
			}

		actor->spr.pos.Z = ps[p].GetActor()->getOffsetZ() + 8 + ps[p].pyoff - (actor->temp_data[2] + (ps[p].GetActor()->spr.Angles.Pitch.Tan() * 2048.)) * zinttoworld;

		if (actor->temp_data[2] > 512)
			actor->temp_data[2] -= 128;

		if (actor->temp_data[2] < 348)
			actor->temp_data[2] += 128;

		if (ps[p].newOwner != nullptr)
		{
			ps[p].newOwner = nullptr;
			ps[p].GetActor()->restoreloc();

			updatesector(ps[p].GetActor()->getPosWithOffsetZ(), &ps[p].cursector);

			DukeStatIterator it(STAT_ACTOR);
			while (auto ac = it.Next())
			{
				if (actorflag(ac, SFLAG2_CAMERA)) ac->spr.yint = 0;
			}
		}

		if (actor->temp_data[3] > 0)
		{
			static const uint8_t frames[] = { 5,5,6,6,7,7,6,5 };

			actor->spr.picnum = GREENSLIME + frames[actor->temp_data[3]];

			if (actor->temp_data[3] == 5)
			{
				auto psp = ps[p].GetActor();
				psp->spr.extra += -(5 + (krand() & 3));
				S_PlayActorSound(SLIM_ATTACK, actor);
			}

			if (actor->temp_data[3] < 7) actor->temp_data[3]++;
			else actor->temp_data[3] = 0;

		}
		else
		{
			actor->spr.picnum = GREENSLIME + 5;
			if (rnd(32))
				actor->temp_data[3] = 1;
		}

		double add = (BobVal(actor->temp_data[1]) * 2) * REPEAT_SCALE;
		actor->spr.scale = DVector2(0.3125 + add, 0.234375 + add);
		actor->spr.pos.XY() = ps[p].GetActor()->spr.pos.XY() + ps[p].GetActor()->spr.Angles.Yaw.ToVector() * 8;
		return;
	}

	else if (actor->vel.X < 4 && xx < 48)
	{
		if (ps[p].somethingonplayer == nullptr)
		{
			ps[p].somethingonplayer = actor;
			if (actor->temp_data[0] == 3 || actor->temp_data[0] == 2) //Falling downward
				actor->temp_data[2] = (12 << 8);
			else actor->temp_data[2] = -(13 << 8); //Climbing up duke
			actor->temp_data[0] = -4;
		}
	}

	j = fi.ifhitbyweapon(actor);
	if (j >= 0)
	{
		S_PlayActorSound(SLIM_DYING, actor);

		if (ps[p].somethingonplayer == actor)
			ps[p].somethingonplayer = nullptr;

		if (j == FREEZEBLAST)
		{
			S_PlayActorSound(SOMETHINGFROZE, actor); 
			actor->temp_data[0] = -5; actor->temp_data[3] = 0;
			return;
		}
		ps[p].actors_killed++;

		if ((krand() & 255) < 32)
		{
			auto spawned = spawn(actor, BLOODPOOL);
			if (spawned) spawned->spr.pal = 0;
		}

		for (int x = 0; x < 8; x++)
		{
			auto a = randomAngle();
			auto vel = krandf(4) + 4;
			auto zvel = -krandf(16) - actor->vel.Z * 0.25;

			auto spawned = CreateActor(actor->sector(), actor->spr.pos.plusZ(-8), PClass::FindActor("DukeScrap"), -8, DVector2(0.75, 0.75), a, vel, zvel, actor, STAT_MISC);
			if (spawned)
			{
			if (spawned) spawned->spriteextra = Scrap3 + (krand() & 3);
				spawned->spr.pal = 6;
			}
		}
		actor->temp_data[0] = -3;
		actor->Destroy();
		return;
	}
	// All weap
	if (actor->temp_data[0] == -1) //Shrinking down
	{
		makeitfall(actor);

		actor->spr.cstat &= ~CSTAT_SPRITE_YFLIP;
		actor->spr.picnum = GREENSLIME + 4;

		if (actor->spr.scale.X > 0.5 ) actor->spr.scale.X += (-(krand() & 7) * REPEAT_SCALE);
		if (actor->spr.scale.Y > 0.25 ) actor->spr.scale.Y += (-(krand() & 7) * REPEAT_SCALE);
		else
		{
			actor->spr.scale = DVector2(0.625, 0.25);
			actor->temp_actor = nullptr;
			actor->temp_data[0] = 0;
		}

		return;
	}
	else if (actor->temp_data[0] != -2) getglobalz(actor);

	if (actor->temp_data[0] == -2) //On top of somebody (an enemy)
	{
		DDukeActor* s5 = actor->temp_actor;
		makeitfall(actor);
		if (s5)
		{
			s5->vel.X = 0;
			
			actor->spr.pos = s5->spr.pos + s5->spr.Angles.Yaw.ToVector() * 0.5;
			actor->spr.picnum = GREENSLIME + 2 + (global_random & 1);

			if (actor->spr.scale.Y < 1) actor->spr.scale.Y += (0.03125);
			else
			{
				if (actor->spr.scale.X < 0.5) actor->spr.scale.X += (0.0625);
				else
				{
					actor->temp_data[0] = -1;
					double dist = (actor->spr.pos.XY() - s5->spr.pos.XY()).LengthSquared();
					if (dist < 48*48) {
						s5->spr.scale.X = 0;
					}
				}
			}
		}
		return;
	}

	//Check randomly to see of there is an actor near
	if (rnd(32))
	{
		DukeSectIterator it(actor->sector());
		while (auto a2 = it.Next())
		{
			if (actorflag(a2, SFLAG_GREENSLIMEFOOD))
			{
				double dist = (actor->spr.pos.XY() - a2->spr.pos.XY()).LengthSquared();
				if (dist < 48*48 && (abs(actor->spr.pos.Z - a2->spr.pos.Z) < 16)) //Gulp them
				{
					actor->temp_actor = a2;
					actor->temp_data[0] = -2;
					actor->temp_data[1] = 0;
					return;
				}
			}
		}
	}

	//Moving on the ground or ceiling

	if (actor->temp_data[0] == 0 || actor->temp_data[0] == 2)
	{
		actor->spr.picnum = GREENSLIME;

		if ((krand() & 511) == 0)
			S_PlayActorSound(SLIM_ROAM, actor);

		if (actor->temp_data[0] == 2)
		{
			actor->vel.Z = 0;
			actor->spr.cstat &= ~CSTAT_SPRITE_YFLIP;

			if ((sectp->ceilingstat & CSTAT_SECTOR_SKY) || (actor->ceilingz + 24) < actor->spr.pos.Z)
			{
				actor->spr.pos.Z += 8;
				actor->temp_data[0] = 3;
				return;
			}
		}
		else
		{
			actor->spr.cstat |= CSTAT_SPRITE_YFLIP;
			makeitfall(actor);
		}

		if (everyothertime & 1) ssp(actor, CLIPMASK0);

		if (actor->vel.X > 6)
		{
			actor->vel.X -= 1/8.;
			return;
		}
		else
		{
			if (actor->vel.X < 2) actor->vel.X += 0.25;
			actor->vel.X = 4 - BobVal(512 + actor->temp_data[1]) * 2;
			actor->spr.Angles.Yaw += deltaangle(actor->spr.Angles.Yaw, (ps[p].GetActor()->spr.pos.XY() - actor->spr.pos.XY()).Angle()) * 0.125;
			// TJR
		}

		actor->spr.scale.X = (0.5625 + BobVal(512 + actor->temp_data[1]) * 0.125);
		actor->spr.scale.Y = (0.25 + BobVal(actor->temp_data[1]) * 0.03125);

		if (rnd(4) && (sectp->ceilingstat & CSTAT_SECTOR_SKY) == 0 &&
			abs(actor->floorz - actor->ceilingz) < 192)
		{
			actor->vel.Z = 0;
			actor->temp_data[0]++;
		}

	}

	if (actor->temp_data[0] == 1)
	{
		actor->spr.picnum = GREENSLIME;
		if (actor->spr.scale.Y < 0.625) actor->spr.scale.Y += (0.125);
		if (actor->spr.scale.X > 0.125 ) actor->spr.scale.X += (-0.0625);
		if (actor->vel.Z > -12)
			actor->vel.Z -= 348 / 256.;
		actor->spr.pos.Z += actor->vel.Z;
		if (actor->spr.pos.Z < actor->ceilingz + 16)
		{
			actor->spr.pos.Z = actor->ceilingz + 16;
			actor->vel.X = 0;
			actor->temp_data[0] = 2;
		}
	}

	if (actor->temp_data[0] == 3)
	{
		actor->spr.picnum = GREENSLIME + 1;

		makeitfall(actor);

		if (actor->spr.pos.Z > actor->floorz - 8)
		{
			actor->spr.scale.Y += (-0.0625);
			actor->spr.scale.X += (0.03125);
		}
		else
		{
			if (actor->spr.scale.Y < 0.5625) actor->spr.scale.Y += (0.125);
			if (actor->spr.scale.X > 0.125 ) actor->spr.scale.X += (-0.0625);
		}

		if (actor->spr.pos.Z > actor->floorz - 8)
		{
			actor->spr.pos.Z = actor->floorz - 8;
			actor->temp_data[0] = 0;
			actor->vel.X = 0;
		}
	}
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

static void flamethrowerflame(DDukeActor *actor)
{
	auto sectp = actor->sector();
	double xx;
	int p = findplayer(actor, &xx);
	execute(actor, p, xx);
	actor->temp_data[0]++;
	if (sectp->lotag == 2)
	{
		spawn(actor, EXPLOSION2)->spr.shade = 127;
		actor->Destroy();
		return;
	}

	auto dapos = actor->spr.pos;

	getglobalz(actor);

	int ds = actor->temp_data[0] / 6;
	if (actor->spr.scale.X < 0.1250)
	{
		actor->spr.scale.X += (ds * REPEAT_SCALE);
		actor->spr.scale.Y = (actor->spr.scale.X);
	}
	actor->clipdist += ds * 0.25;
	if (actor->temp_data[0] <= 2)
		actor->temp_data[3] = krand() % 10;
	if (actor->temp_data[0] > 30) 
	{
		spawn(actor, EXPLOSION2)->spr.shade = 127;
		actor->Destroy();
		return;
	}

	Collision coll;
	movesprite_ex(actor, DVector3(actor->spr.Angles.Yaw.ToVector() * actor->vel.X, actor->vel.Z), CLIPMASK1, coll);

	if (!actor->insector())
	{
		actor->Destroy();
		return;
	}

	if (coll.type != kHitSprite)
	{
		if (actor->spr.pos.Z < actor->ceilingz)
		{
			coll.setSector(actor->sector());
			actor->vel.Z -= 1/256.;
		}
		else if ((actor->spr.pos.Z > actor->floorz && actor->sector()->lotag != 1)
			|| (actor->spr.pos.Z > actor->floorz + 16 && actor->sector()->lotag == 1))
		{
			coll.setSector(actor->sector());
			if (actor->sector()->lotag != 1)
				actor->vel.Z += 1/256.;
		}
	}

	if (coll.type != 0) {
		actor->vel.XY().Zero();
		actor->vel.Z = 0;
		if (coll.type == kHitSprite)
		{
			fi.checkhitsprite(coll.actor(), actor);
			if (coll.actor()->isPlayer())
				S_PlayActorSound(PISTOL_BODYHIT, coll.actor());
		}
		else if (coll.type == kHitWall)
		{
			SetActor(actor, dapos);
			fi.checkhitwall(actor, coll.hitWall, actor->spr.pos, actor->spr.picnum);
		}
		else if (coll.type == kHitSector)
		{
			SetActor(actor, dapos);
			if (actor->vel.Z < 0)
				fi.checkhitceiling(actor->sector());
		}

		if (actor->spr.scale.X >= 0.15625)
		{
			int x = actor->spr.extra;
			fi.hitradius(actor, gs.rpgblastradius, x >> 2, x >> 1, x - (x >> 2), x);
		}
		else
		{
			int x = actor->spr.extra + (global_random & 3);
			fi.hitradius(actor, (gs.rpgblastradius >> 1), x >> 2, x >> 1, x - (x >> 2), x);
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
	auto Owner = actor->GetOwner();
	auto sectp = actor->sector();
	int l;
	double xx;

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

	if (actor->spr.picnum != BOUNCEMINE)
	{
		makeitfall(actor);

		if (sectp->lotag != 1 && actor->spr.pos.Z >= actor->floorz - FOURSLEIGHT_F && actor->spr.yint < 3)
		{
			if (actor->spr.yint > 0 || (actor->spr.yint == 0 && actor->floorz == sectp->floorz))
				S_PlayActorSound(PIPEBOMB_BOUNCE, actor);
			actor->vel.Z = -(4 - actor->spr.yint);
			if (actor->sector()->lotag == 2)
				actor->vel.Z *= 0.25;
			actor->spr.yint++;
		}
		if (actor->spr.pos.Z < actor->ceilingz) // && sectp->lotag != 2 )
		{
			actor->spr.pos.Z = actor->ceilingz + 3;
			actor->vel.Z = 0;
		}
	}

	Collision coll;
	movesprite_ex(actor, DVector3(actor->spr.Angles.Yaw.ToVector() * actor->vel.X, actor->vel.Z), CLIPMASK0, coll);

	if (actor->sector()->lotag == 1 && actor->vel.Z == 0)
	{
		actor->spr.pos.Z += 32;
		if (actor->temp_data[5] == 0)
		{
			actor->temp_data[5] = 1;
			spawn(actor, WATERSPLASH2);
		}
	}
	else actor->temp_data[5] = 0;

	if (actor->temp_data[3] == 0 && (actor->spr.picnum == BOUNCEMINE || actor->spr.picnum == MORTER) && (coll.type || xx < 844/16.))
	{
		actor->temp_data[3] = 1;
		actor->temp_data[4] = 0;
		l = 0;
		actor->vel.X = 0;
		goto DETONATEB;
	}

	if ( Owner && Owner->isPlayer())
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

	if (coll.type== kHitWall)
	{
		auto wal = coll.hitWall;
		fi.checkhitwall(actor, wal, actor->spr.pos, actor->spr.picnum);

		DAngle k = wal->delta().Angle();
		actor->spr.Angles.Yaw = k * 2 - actor->spr.Angles.Yaw;
		actor->vel.X *= 0.5;
	}

DETONATEB:

	bool bBoom = false;
	if ((l >= 0 && ps[l].hbomb_on == 0) || actor->temp_data[3] == 1)
		bBoom = true;
	if (isNamWW2GI() && actor->spr.picnum == HEAVYHBOMB)
	{
		actor->spr.extra--;
		if (actor->spr.extra <= 0)
			bBoom = true;
	}
	if (bBoom)
	{
		actor->temp_data[4]++;

		if (actor->temp_data[4] == 2)
		{
			int x = actor->spr.extra;
			int m = 0;
			switch (actor->spr.picnum)
			{
			case HEAVYHBOMB: m = gs.pipebombblastradius; break;
			case MORTER: m = gs.morterblastradius; break;
			case BOUNCEMINE: m = gs.bouncemineblastradius; break;
			}

			fi.hitradius(actor, m, x >> 2, x >> 1, x - (x >> 2), x);
			spawn(actor, EXPLOSION2);
			if (actor->vel.Z == 0)	spawn(actor, EXPLOSION2BOT);
			S_PlayActorSound(PIPEBOMB_EXPLODE, actor);
			for (x = 0; x < 8; x++)
				RANDOMSCRAP(actor);
		}

		if (actor->spr.scale.Y)
		{
			actor->spr.scale.Y = (0);
			return;
		}

		if (actor->temp_data[4] > 20)
		{
			if (Owner != actor || ud.respawn_items == 0)
			{
				actor->Destroy();
				return;
			}
			else
			{
				actor->temp_data[2] = gs.respawnitemtime;
				spawn(actor, PClass::FindActor("DukeRespawnMarker"));
				actor->spr.cstat = CSTAT_SPRITE_INVISIBLE;
				actor->spr.scale.Y = (0.140625);
				return;
			}
		}
	}
	else if (actor->spr.picnum == HEAVYHBOMB && xx < 788 / 16. && actor->temp_data[0] > 7 && actor->vel.X == 0)
		if (cansee(actor->spr.pos.plusZ(-8), actor->sector(), ps[p].GetActor()->getPosWithOffsetZ(), ps[p].cursector))
			if (ps[p].ammo_amount[HANDBOMB_WEAPON] < gs.max_ammo_amount[HANDBOMB_WEAPON])
			{
				if (ud.coop >= 1 && Owner == actor)
				{
					for (int j = 0; j < ps[p].weapreccnt; j++)
						if (ps[p].weaprecs[j] == actor->spr.picnum)
							continue;

					if (ps[p].weapreccnt < 255) // DukeGDX has 16 here.
						ps[p].weaprecs[ps[p].weapreccnt++] = actor->spr.picnum;
				}

				addammo(HANDBOMB_WEAPON, &ps[p], 1);
				S_PlayActorSound(DUKE_GET, ps[p].GetActor());

				if (ps[p].gotweapon[HANDBOMB_WEAPON] == 0 || Owner == ps[p].GetActor())
					fi.addweapon(&ps[p], HANDBOMB_WEAPON, true);

				if (!Owner || !Owner->isPlayer())
				{
					SetPlayerPal(&ps[p], PalEntry(32, 0, 32, 0));
				}

				if (Owner != actor || ud.respawn_items == 0)
				{
					if (Owner == actor && ud.coop >= 1)
						return;

					actor->Destroy();
					return;
				}
				else
				{
					actor->temp_data[2] = gs.respawnitemtime;
					spawn(actor, PClass::FindActor("DukeRespawnMarker"));
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

void moveactors_d(void)
{
	int p;
	unsigned int k;

	DukeStatIterator it(STAT_ACTOR);
	while (auto act = it.Next())
	{
		if (act->spr.picnum == QUEBALL)
		{
			int a = 0;
		}
		auto sectp = act->sector();

		if (act->spr.scale.X == 0 || sectp == nullptr || actorflag(act, SFLAG2_DIENOW))
		{ 
			act->Destroy();
			continue;
		}

		int *t = &act->temp_data[0];

		if (act->GetClass() != RUNTIME_CLASS(DDukeActor))
		{
			CallTick(act);
		}
		else switch (act->spr.picnum)
		{
		case FLAMETHROWERFLAME:
			if (isWorldTour()) flamethrowerflame(act);
			continue;

		case DUCK:
		case TARGET:
			if (act->spr.cstat & CSTAT_SPRITE_ALIGNMENT_FLOOR)
			{
				act->temp_data[0]++;
				if (act->temp_data[0] > 60)
				{
					act->temp_data[0] = 0;
					act->spr.cstat = CSTAT_SPRITE_YCENTER | CSTAT_SPRITE_BLOCK_ALL | CSTAT_SPRITE_ALIGNMENT_WALL;
					act->spr.extra = 1;
				}
			}
			else
			{
				int j = fi.ifhitbyweapon(act);
				if (j >= 0)
				{
					act->spr.cstat = CSTAT_SPRITE_ALIGNMENT_FLOOR | CSTAT_SPRITE_YCENTER;
					k = 1;

					DukeStatIterator itr(STAT_ACTOR);
					while (auto act2 = itr.Next())
					{
						if (act2->spr.lotag == act->spr.lotag &&
							act2->spr.picnum == act->spr.picnum)
						{
							if ((act2->spr.hitag && !(act2->spr.cstat & CSTAT_SPRITE_ALIGNMENT_FLOOR)) ||
								(!act2->spr.hitag && (act2->spr.cstat & CSTAT_SPRITE_ALIGNMENT_FLOOR))
								)
							{
								k = 0;
								break;
							}
						}
					}

					if (k == 1)
					{
						operateactivators(act->spr.lotag, nullptr);
						fi.operateforcefields(act, act->spr.lotag);
						operatemasterswitches(act->spr.lotag);
					}
				}
			}
			continue;

		case HELECOPT:
		case DUKECAR:

			act->spr.pos.Z += act->vel.Z;
			act->temp_data[0]++;

			if (act->temp_data[0] == 4) S_PlayActorSound(WAR_AMBIENCE2, act);

			if (act->temp_data[0] > (26 * 8))
			{
				S_PlaySound(RPG_EXPLODE);
				for (int j = 0; j < 32; j++) 
						RANDOMSCRAP(act);
				earthquaketime = 16;
				act->Destroy();
				continue;
			} 
			else if ((act->temp_data[0] & 3) == 0)
				spawn(act, EXPLOSION2);
			ssp(act, CLIPMASK0);
			break;
		case FORCESPHERE:
			forcesphere(act, FORCESPHERE);
			continue;

		case RECON:
			recon(act, EXPLOSION2, FIRELASER, RECO_ATTACK, RECO_PAIN, RECO_ROAM, 4, [](DDukeActor* i)->int { return PIGCOP; });
			continue;

		case OOZ:
		case OOZ2:
			ooz(act);
			continue;

		case GREENSLIME:
		case GREENSLIME + 1:
		case GREENSLIME + 2:
		case GREENSLIME + 3:
		case GREENSLIME + 4:
		case GREENSLIME + 5:
		case GREENSLIME + 6:
		case GREENSLIME + 7:
			greenslime(act);
			continue;

		case BOUNCEMINE:
		case MORTER:
			spawn(act, FRAMEEFFECT1)->temp_data[0] = 3;
			[[fallthrough]];

		case HEAVYHBOMB:
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
		if (ud.multimode < 2 && badguy(act))
		{
			if (actor_tog == 1)
			{
				act->spr.cstat = CSTAT_SPRITE_INVISIBLE;
				continue;
			}
			else if (actor_tog == 2) act->spr.cstat = CSTAT_SPRITE_BLOCK_ALL;
		}
		// #endif

		double xx;
		p = findplayer(act, &xx);

		execute(act, p, xx);
	}

}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

static void fireflyflyingeffect(DDukeActor *actor)
{
	double xx;
	int p = findplayer(actor, &xx);
	execute(actor, p, xx);

	auto Owner = actor->GetOwner();
	if (!Owner || Owner->spr.picnum != FIREFLY) 
	{
		actor->Destroy();
		return;
	}

	if (Owner->spr.scale.X >= 0.375 || Owner->spr.pal == 1)
		actor->spr.cstat |= CSTAT_SPRITE_INVISIBLE;
	else
		actor->spr.cstat &= ~CSTAT_SPRITE_INVISIBLE;

	auto dvec = Owner->spr.pos.XY() - ps[p].GetActor()->spr.pos.XY();
	double dist = dvec.Length();

	if (dist != 0.0) dvec /= dist;
	actor->spr.pos = Owner->spr.pos + DVector3(dvec.X * -0.625, dvec.Y * -0.625, 8);

	if (Owner->spr.extra <= 0) 
	{
		actor->Destroy();
	}

}
//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void moveexplosions_d(void)  // STATNUM 5
{
	int p;
	double xx;

	DukeStatIterator it(STAT_MISC);
	while (auto act = it.Next())
	{
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


		auto sectp = act->sector();

		switch (act->spr.picnum)
		{
		case FIREFLYFLYINGEFFECT:
			if (isWorldTour()) fireflyflyingeffect(act);
			continue;

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

		case NUKEBUTTON:
		case NUKEBUTTON + 1:
		case NUKEBUTTON + 2:
		case NUKEBUTTON + 3:

			if (act->temp_data[0])
			{
				act->temp_data[0]++;
				auto Owner = act->GetOwner();
				if (act->temp_data[0] == 8) act->spr.picnum = NUKEBUTTON + 1;
				else if (act->temp_data[0] == 16 && Owner)
				{
					act->spr.picnum = NUKEBUTTON + 2;
					ps[Owner->PlayerIndex()].fist_incs = 1;
				}
				if (Owner && ps[Owner->PlayerIndex()].fist_incs == 26)
					act->spr.picnum = NUKEBUTTON + 3;
			}
			continue;

		case FORCESPHERE:
			forcesphereexplode(act);
			continue;
		case WATERSPLASH2:
			watersplash2(act);
			continue;

		case FRAMEEFFECT1:
			frameeffect1(act);
			continue;
		case FIRELASER:
			if (act->spr.extra != 999)
				act->spr.extra = 999;
			else
			{
				act->Destroy();
				continue;
			}
			break;
		case MONEY + 1:
		case MAIL + 1:
		case PAPER + 1:
			act->vel.Z = act->floorz = getflorzofslopeptr(act->sector(), act->spr.pos.X, act->spr.pos.Y);
			break;
		case MONEY:
		case MAIL:
		case PAPER:
			money(act, BLOODPOOL);
			break;

		case BLOODPOOL:
		case PUKE:
			bloodpool(act, act->spr.picnum == PUKE);

			continue;

		case LAVAPOOL:
		case ONFIRE:
		case ONFIRESMOKE:
		case BURNEDCORPSE:
		case LAVAPOOLBUBBLE:
		case WHISPYSMOKE:
			if (!isWorldTour())
				continue;
			[[fallthrough]];

		case BURNING:
		case BURNING2:
		case FECES:
		case WATERBUBBLE:
		case SMALLSMOKE:
		case EXPLOSION2:
		case SHRINKEREXPLOSION:
		case EXPLOSION2BOT:
		case BLOOD:
		case LASERSITE:
		case FORCERIPPLE:
		case TRANSPORTERSTAR:
		case TRANSPORTERBEAM:
			p = findplayer(act, &xx);
			execute(act, p, xx);
			continue;

		case SHELL:
		case SHOTGUNSHELL:
			shell(act, sectp->floorz + 24 < act->spr.pos.Z);
			continue;

		case GLASSPIECES:
		case GLASSPIECES + 1:
		case GLASSPIECES + 2:
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

void handle_se06_d(DDukeActor* actor)
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
		}
	}
	else actor->vel.X = k / 16.;

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
				if (act2->spr.extra)
					x = -x;
				actor->vel.X += x / 16.;
			}
			act2->temp_data[4] = actor->temp_data[4];
		}
	}
	handle_se14(actor, true, RPG, JIBS6);
}


//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

static void handle_se28(DDukeActor* actor)
{
	if (actor->temp_data[5] > 0)
	{
		actor->temp_data[5]--;
		return;
	}

	if (actor->temp_data[0] == 0)
	{
		double x;
		findplayer(actor, &x);
		if (x > 15500 / 16.)
			return;
		actor->temp_data[0] = 1;
		actor->temp_data[1] = 64 + (krand() & 511);
		actor->temp_data[2] = 0;
	}
	else
	{
		actor->temp_data[2]++;
		if (actor->temp_data[2] > actor->temp_data[1])
		{
			actor->temp_data[0] = 0;
			ps[screenpeek].visibility = ud.const_visibility;
			return;
		}
		else if (actor->temp_data[2] == (actor->temp_data[1] >> 1))
			S_PlayActorSound(THUNDER, actor);
		else if (actor->temp_data[2] == (actor->temp_data[1] >> 3))
			S_PlayActorSound(LIGHTNING_SLAP, actor);
		else if (actor->temp_data[2] == (actor->temp_data[1] >> 2))
		{
			DukeStatIterator it(STAT_DEFAULT);
			while (auto act2 = it.Next())
			{
				if (act2->spr.picnum == NATURALLIGHTNING && act2->spr.hitag == actor->spr.hitag)
					act2->spr.cstat |= CSTAT_SPRITE_INVISIBLE;
			}
		}
		else if (actor->temp_data[2] > (actor->temp_data[1] >> 3) && actor->temp_data[2] < (actor->temp_data[1] >> 2))
		{
			int j = !!cansee(actor->spr.pos, actor->sector(), ps[screenpeek].GetActor()->getPosWithOffsetZ(), ps[screenpeek].cursector);

			if (rnd(192) && (actor->temp_data[2] & 1))
			{
				if (j) ps[screenpeek].visibility = 0;
			}
			else if (j)	ps[screenpeek].visibility = ud.const_visibility;

			DukeStatIterator it(STAT_DEFAULT);
			while (auto act2 = it.Next())
			{
				if (act2->spr.picnum == NATURALLIGHTNING && act2->spr.hitag == actor->spr.hitag)
				{
					if (rnd(32) && (actor->temp_data[2] & 1))
					{
						act2->spr.cstat &= ~CSTAT_SPRITE_INVISIBLE;
						spawn(act2, SMALLSMOKE);

						double x;
						int p = findplayer(actor, &x);
						auto psa = ps[p].GetActor();
						double dist = (psa->spr.pos.XY() - act2->spr.pos.XY()).LengthSquared();
						if (dist < 49*48)
						{
							if (S_CheckActorSoundPlaying(psa, DUKE_LONGTERM_PAIN) < 1)
								S_PlayActorSound(DUKE_LONGTERM_PAIN, psa);
							S_PlayActorSound(SHORT_CIRCUIT, psa);
							psa->spr.extra -= 8 + (krand() & 7);
							SetPlayerPal(&ps[p], PalEntry(32, 16, 0, 0));
						}
						return;
					}
					else act2->spr.cstat |= CSTAT_SPRITE_INVISIBLE;
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

void moveeffectors_d(void)   //STATNUM 3
{
	clearfriction();

	DukeStatIterator it(STAT_EFFECTOR);
	while (auto act = it.Next())
	{
		auto sc = act->sector();
		switch (act->spr.lotag)
		{
		case SE_0_ROTATING_SECTOR:
			handle_se00(act);
			break;

		case SE_1_PIVOT: //Nothing for now used as the pivot
			handle_se01(act);
			break;

		case SE_6_SUBWAY:
			handle_se06_d(act);
			break;

		case SE_14_SUBWAY_CAR:
			handle_se14(act, true, RPG, JIBS6);
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
			handle_se08(act, false);
			break;

		case SE_10_DOOR_AUTO_CLOSE:
		{
			static const int tags[] = { 20, 21, 22, 26, 0};
			handle_se10(act, tags);
			break;
		}
		case SE_11_SWINGING_DOOR:
			handle_se11(act);
			break;

		case SE_12_LIGHT_SWITCH:
			handle_se12(act);
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

		case SE_24_CONVEYOR:
		case SE_34:
		{
			handle_se24(act, true, 0.25);
			break;
		}
		case SE_35:
			handle_se35(act, SMALLSMOKE, EXPLOSION2);
			break;

		case SE_25_PISTON: //PISTONS
			if (act->temp_data[4] == 0) break;
			handle_se25(act, -1, -1);
			break;

		case SE_26:
			handle_se26(act);
			break;

		case SE_27_DEMO_CAM:
			handle_se27(act);
			break;
		case SE_28_LIGHTNING:
			handle_se28(act);
			break;

		case SE_29_WAVES:
			handle_se29(act);
			break;

		case SE_31_FLOOR_RISE_FALL: // True Drop Floor
			handle_se31(act, true);
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

		case 130:
			handle_se130(act, 80, EXPLOSION2);
			break;
		case 131:
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
//
//
//---------------------------------------------------------------------------

void move_d(DDukeActor *actor, int playernum, int xvel)
{
	DAngle goalang, angdif;
	double daxvel;

	int a = actor->spr.hitag;

	if (a == -1) a = 0;

	actor->temp_data[0]++;

	if (a & face_player)
	{
		if (ps[playernum].newOwner != nullptr)
			goalang = (ps[playernum].GetActor()->opos.XY() - actor->spr.pos.XY()).Angle();
		else goalang = (ps[playernum].GetActor()->spr.pos.XY() - actor->spr.pos.XY()).Angle();
		angdif = deltaangle(actor->spr.Angles.Yaw, goalang) * 0.25;
		if (angdif > -DAngle22_5 / 16 && angdif < nullAngle) angdif = nullAngle;
		actor->spr.Angles.Yaw += angdif;
	}

	if (a & spin)
		actor->spr.Angles.Yaw += DAngle45 * BobVal(actor->temp_data[0] << 3);

	if (a & face_player_slow)
	{
		if (ps[playernum].newOwner != nullptr)
			goalang = (ps[playernum].GetActor()->opos.XY() - actor->spr.pos.XY()).Angle();
		else goalang = (ps[playernum].GetActor()->spr.pos.XY() - actor->spr.pos.XY()).Angle();
		angdif = DAngle22_5 * 0.25 * Sgn(deltaangle(actor->spr.Angles.Yaw, goalang).Degrees()); // this looks very wrong...
		actor->spr.Angles.Yaw += angdif;
	}


	if ((a & jumptoplayer) == jumptoplayer)
	{
		if (actor->temp_data[0] < 16)
			actor->vel.Z -= BobVal(512 + (actor->temp_data[0] << 4)) * 2;
	}

	if (a & face_player_smart)
	{
		DVector2 newpos = ps[playernum].GetActor()->spr.pos.XY() + (ps[playernum].vel.XY() * (4. / 3.));
		goalang = (newpos - actor->spr.pos.XY()).Angle();
		angdif = deltaangle(actor->spr.Angles.Yaw, goalang) * 0.25;
		if (angdif > -DAngle22_5/16 && angdif < nullAngle) angdif = nullAngle;
		actor->spr.Angles.Yaw += angdif;
	}

	if (actor->temp_data[1] == 0 || a == 0)
	{
		if ((badguy(actor) && actor->spr.extra <= 0) || (actor->opos.X != actor->spr.pos.X) || (actor->opos.Y != actor->spr.pos.Y))
		{
			if (!actor->isPlayer()) actor->backupvec2();
			SetActor(actor, actor->spr.pos);
		}
		return;
	}

	if (actor->spr.picnum == WATERBUBBLE)
	{
		int a = 0;
	}

	auto moveptr = &ScriptCode[actor->temp_data[1]];

	if (a & geth) actor->vel.X += (moveptr[0] / 16. - actor->vel.X) * 0.5;
	if (a & getv) actor->vel.Z += (moveptr[1] / 16. - actor->vel.Z) * 0.5;

	if (a & dodgebullet)
		dodge(actor);

	if (!actor->isPlayer())
		alterang(a, actor, playernum);

	if (abs(actor->vel.X) < 6 / 16.) actor->vel.X = 0;

	a = badguy(actor);

	if (actor->vel.X != 0 || actor->vel.Z != 0)
	{
		if (a && actor->spr.picnum != ROTATEGUN)
		{
			if ((actor->spr.picnum == DRONE || actor->spr.picnum == COMMANDER) && actor->spr.extra > 0)
			{
				if (actor->spr.picnum == COMMANDER)
				{
					double c, f;
					calcSlope(actor->sector(), actor->spr.pos.X, actor->spr.pos.Y, &c, &f);
					actor->floorz = f;
					actor->ceilingz = c;

					if (actor->spr.pos.Z > f - 8)
					{
						actor->spr.pos.Z = f - 8;
						actor->vel.Z = 0;
					}

					if (actor->spr.pos.Z < c + 80)
					{
						actor->spr.pos.Z = c + 80;
						actor->vel.Z = 0;
					}
				}
				else
				{
					if (actor->vel.Z > 0)
					{
						double f = getflorzofslopeptr(actor->sector(), actor->spr.pos.X, actor->spr.pos.Y);
						actor->floorz = f;
						if (actor->spr.pos.Z > f - 30)
							actor->spr.pos.Z = f - 30;
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
			}
			else if (actor->spr.picnum != ORGANTIC)
			{
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
		}

		daxvel = actor->vel.X;
		angdif = actor->spr.Angles.Yaw;

		if (a && actor->spr.picnum != ROTATEGUN)
		{
			if (xvel < 960 && actor->spr.scale.X > 0.25 )
			{

				daxvel = -(1024 - xvel) * maptoworld;
				angdif = (ps[playernum].GetActor()->spr.pos.XY() - actor->spr.pos.XY()).Angle();

				if (xvel < 512)
				{
					ps[playernum].vel.X = 0;
					ps[playernum].vel.Y = 0;
				}
				else
				{
					ps[playernum].vel.XY() *= gs.playerfriction - 0.125;
				}
			}
			else if (actor->spr.picnum != DRONE && actor->spr.picnum != SHARK && actor->spr.picnum != COMMANDER)
			{
				if (!*(moveptr + 1))
				{
					if (actor->opos.Z != actor->spr.pos.Z || (ud.multimode < 2 && ud.player_skill < 2))
					{
						if ((actor->temp_data[0] & 1) || ps[playernum].actorsqu == actor) return;
						else daxvel *= 2;
					}
					else
					{
						if ((actor->temp_data[0] & 3) || ps[playernum].actorsqu == actor) return;
						else daxvel *= 4;
					}
				}
			}
		}

		Collision coll;
		actor->movflag = movesprite_ex(actor, DVector3(angdif.ToVector() * daxvel, actor->vel.Z), CLIPMASK0, coll);
	}

	if (a)
	{
		if (actor->sector()->ceilingstat & CSTAT_SECTOR_SKY)
			actor->spr.shade += (actor->sector()->ceilingshade - actor->spr.shade) >> 1;
		else actor->spr.shade += (actor->sector()->floorshade - actor->spr.shade) >> 1;

		if (actor->sector()->floorpicnum == MIRROR)
			actor->Destroy();
	}
}


//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void fall_d(DDukeActor *actor, int g_p)
{
	fall_common(actor, g_p, JIBS6, DRONE, BLOODPOOL, SHOTSPARK1, SQUISHED, THUD, nullptr);
}

bool spawnweapondebris_d(int picnum)
{
	return picnum == BLIMP;
}

void respawnhitag_d(DDukeActor* actor)
{
	switch (actor->spr.picnum)
	{
	case FEM1:
	case FEM2:
	case FEM3:
	case FEM4:
	case FEM5:
	case FEM6:
	case FEM7:
	case FEM8:
	case FEM9:
	case FEM10:
	case PODFEM1:
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

void think_d(void)
{
	thinktime.Reset();
	thinktime.Clock();

	movefta();			//ST 2
	moveweapons_d();		//ST 4
	moveplayers();			//ST 10
	movefallers_d();		//ST 12
	moveexplosions_d();		//ST 5

	actortime.Reset();
	actortime.Clock();
	moveactors_d();			//ST 1
	actortime.Unclock();

	moveeffectors_d();		//ST 3
	movestandables_d();		//ST 6
	doanimations();
	tickstat(STAT_FX);				//ST 11

	thinktime.Unclock();
}


END_DUKE_NS
