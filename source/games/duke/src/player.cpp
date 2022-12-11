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
#include "mapinfo.h"
#include "dukeactor.h"
#include "vm.h"

BEGIN_DUKE_NS 

//---------------------------------------------------------------------------
//
// callback for playercolor CVAR
//
//---------------------------------------------------------------------------

int playercolor2lookup(int color)
{
	static int8_t player_pals[] = { 0, 9, 10, 11, 12, 13, 14, 15, 16, 21, 23, };
	if (color >= 0 && color < 10) return player_pals[color];
	return 0;
}

void PlayerColorChanged(void)
{
	if (ud.recstat != 0)
		return;

	auto& pp = ps[myconnectindex];
	if (ud.multimode > 1)
	{
		//Net_SendClientInfo();
	}
	else
	{
		pp.palookup = ud.user_pals[myconnectindex] = playercolor2lookup(playercolor);
	}
	if (pp.GetActor()->isPlayer() && pp.GetActor()->spr.pal != 1)
		pp.GetActor()->spr.pal = ud.user_pals[myconnectindex];
}

//---------------------------------------------------------------------------
//
// why is this such a mess?
//
//---------------------------------------------------------------------------

int setpal(player_struct* p)
{
	int palette;
	if (p->DrugMode) palette = DRUGPAL;
	else if (p->heat_on) palette = SLIMEPAL;
	else if (!p->insector()) palette = BASEPAL; // don't crash if out of range.
	else if (tileflags(p->cursector->ceilingpicnum) & TFLAG_SLIME) palette = SLIMEPAL;
	else if (p->cursector->lotag == ST_2_UNDERWATER) palette = WATERPAL;
	else palette = BASEPAL;
	return palette;
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void quickkill(player_struct* p)
{
	SetPlayerPal(p, PalEntry(48, 48, 48, 48));

	auto pa = p->GetActor();
	pa->spr.extra = 0;
	pa->spr.cstat |= CSTAT_SPRITE_INVISIBLE;
	if (ud.god == 0) spawnguts(pa, PClass::FindActor("DukeJibs6"), 8);
	return;
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void forceplayerangle(int snum)
{
	player_struct* p = &ps[snum];
	const auto ang = (DAngle22_5 - randomAngle(45)) / 2.;

	p->GetActor()->spr.Angles.Pitch -= DAngle::fromDeg(26.566);
	p->sync.actions |= SB_CENTERVIEW;
	p->Angles.ViewAngles.Yaw = ang;
	p->Angles.ViewAngles.Roll = -ang;
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void tracers(const DVector3& start, const DVector3& dest, int n)
{
	sectortype* sect = nullptr;

	auto direction = dest - start;

	if (direction.XY().Sum() < 192.75)
		return;

	auto pos = start;
	auto add = direction / (n + 1);
	for (int i = n; i > 0; i--)
	{
		pos += add;
		updatesector(pos, &sect);
		if (sect)
		{
			if (sect->lotag == 2)
			{
				DVector2 scale(0.0625 + (krand() & 3) * REPEAT_SCALE, 0.0625 + (krand() & 3) * REPEAT_SCALE);
				CreateActor(sect, pos, TILE_WATERBUBBLE, -32, scale, randomAngle(), 0., 0., ps[0].GetActor(), 5);
			}
			else
				CreateActor(sect, pos, TILE_SMALLSMOKE, -32, DVector2(0.21875, 0.21875), nullAngle, 0., 0., ps[0].GetActor(), 5);
		}
	}
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

double hits(DDukeActor* actor)
{
	double zoff;
	HitInfo hit{};

	if (actor->isPlayer()) zoff = gs.playerheight;
	else zoff = 0;

	auto pos = actor->spr.pos;
	hitscan(pos.plusZ(-zoff), actor->sector(), DVector3(actor->spr.Angles.Yaw.ToVector() * 1024, 0), hit, CLIPMASK1);
	return (hit.hitpos.XY() - actor->spr.pos.XY()).Length();
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

double hitasprite(DDukeActor* actor, DDukeActor** hitsp)
{
	double zoff;
	HitInfo hit{};

	if (badguy(actor))
		zoff = 42;
	else if (actor->isPlayer()) zoff = gs.playerheight;
	else zoff = 0;

	auto pos = actor->spr.pos;
	hitscan(pos.plusZ(-zoff), actor->sector(), DVector3(actor->spr.Angles.Yaw.ToVector() * 1024, 0), hit, CLIPMASK1);
	if (hitsp) *hitsp = hit.actor();

	if (hit.hitWall != nullptr && (hit.hitWall->cstat & CSTAT_WALL_MASKED) && badguy(actor))
		return INT_MAX;

	return (hit.hitpos.XY() - actor->spr.pos.XY()).Length();
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

double hitawall(player_struct* p, walltype** hitw)
{
	HitInfo hit{};

	hitscan(p->GetActor()->getPosWithOffsetZ(), p->cursector, DVector3(p->GetActor()->spr.Angles.Yaw.ToVector() * 1024, 0), hit, CLIPMASK0);
	if (hitw) *hitw = hit.hitWall;

	return (hit.hitpos.XY() - p->GetActor()->spr.pos.XY()).Length();
}


//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

DDukeActor* aim(DDukeActor* actor, int abase)
{
	DAngle aang = DAngle90 * (AUTO_AIM_ANGLE / 512.);

	bool gotshrinker, gotfreezer;
	static const int aimstats[] = { STAT_PLAYER, STAT_DUMMYPLAYER, STAT_ACTOR, STAT_ZOMBIEACTOR };

	DAngle a = actor->spr.Angles.Yaw;

	// Autoaim from DukeGDX.
	if (actor->isPlayer())
	{
		auto* plr = &ps[actor->PlayerIndex()];
		int autoaim = Autoaim(actor->PlayerIndex());
		if (!autoaim)
		{
			// Some fudging to avoid aim randomization when autoaim is off.
			// This is a reimplementation of how it was solved in RedNukem.
			if (plr->curr_weapon == PISTOL_WEAPON && !isWW2GI())
			{
				double vel = 1024, zvel = 0;
				setFreeAimVelocity(vel, zvel, plr->Angles.getPitchWithView(), 16.);

				HitInfo hit{};
				hitscan(plr->GetActor()->getPosWithOffsetZ().plusZ(4), actor->sector(), DVector3(actor->spr.Angles.Yaw.ToVector() * vel, zvel), hit, CLIPMASK1);

				if (hit.actor() != nullptr)
				{
					if (isIn(hit.actor()->spr.statnum, { STAT_PLAYER, STAT_DUMMYPLAYER, STAT_ACTOR, STAT_ZOMBIEACTOR }))
						return hit.actor();
				}
			}
			// The chickens in RRRA are homing and must always autoaim.
			if (!isRRRA() || plr->curr_weapon != CHICKEN_WEAPON)
				return nullptr;
		}
		else if (autoaim == 2)
		{
			int weap;
			if (!isWW2GI())
			{
				weap = plr->curr_weapon;
			}
			else
			{
				weap = aplWeaponWorksLike(plr->curr_weapon, actor->PlayerIndex());
			}
			// The chickens in RRRA are homing and must always autoaim.
			if (!isRRRA() || plr->curr_weapon != CHICKEN_WEAPON)
			{
				if (weap > CHAINGUN_WEAPON || weap == KNEE_WEAPON)
				{
					return nullptr;
				}
			}

		}
	}
	DDukeActor* aimed = nullptr;
	//	  if(actor->isPlayer() && ps[actor->PlayerIndex()].aim_mode) return -1;

	if (isRR())
	{
		gotshrinker = false;
		gotfreezer = false;
	}
	else if (isWW2GI())
	{
		gotshrinker = actor->isPlayer() && aplWeaponWorksLike(ps[actor->PlayerIndex()].curr_weapon, actor->PlayerIndex()) == SHRINKER_WEAPON;
		gotfreezer = actor->isPlayer() && aplWeaponWorksLike(ps[actor->PlayerIndex()].curr_weapon, actor->PlayerIndex()) == FREEZE_WEAPON;
	}
	else
	{
		gotshrinker = actor->isPlayer() && ps[actor->PlayerIndex()].curr_weapon == SHRINKER_WEAPON;
		gotfreezer = actor->isPlayer() && ps[actor->PlayerIndex()].curr_weapon == FREEZE_WEAPON;
	}

	double smax = 0x7fffffff;

	auto dv1 = (a - aang).ToVector();
	auto dv2 = (a + aang).ToVector();
	auto dv3 = a.ToVector();

	for (int k = 0; k < 4; k++)
	{
		if (aimed)
			break;

		DukeStatIterator it(aimstats[k]);
		while (auto act = it.Next())
		{
			if (act->spr.scale.X > 0 && act->spr.extra >= 0 && (act->spr.cstat & (CSTAT_SPRITE_BLOCK_ALL | CSTAT_SPRITE_INVISIBLE)) == CSTAT_SPRITE_BLOCK_ALL)
				if (badguy(act) || k < 2)
				{
					if (badguy(act) || act->isPlayer())
					{
						if (act->isPlayer() &&
							(isRR() && ud.ffire == 0) &&
							ud.coop == 1 &&
							actor->isPlayer() &&
							actor != act)
							continue;

						if (gotshrinker && act->spr.scale.X < 0.46875 && !actorflag(act, SFLAG_SHRINKAUTOAIM)) continue;
						if (gotfreezer && act->spr.pal == 1) continue;
					}

					DVector2 vv = act->spr.pos.XY() - actor->spr.pos.XY();

					if ((dv1.Y * vv.X) <= (dv1.X * vv.Y))
						if ((dv2.Y * vv.X) >= (dv2.X * vv.Y))
						{
							double sdist = dv3.dot(vv);
							if (sdist > 32 && sdist < smax)
							{
								int check;
								if (actor->isPlayer())
								{
									double checkval = (act->spr.pos.Z - actor->spr.pos.Z) * 1.25 / sdist;
									double horiz = ps[actor->PlayerIndex()].Angles.getPitchWithView().Tan();
									check = abs(checkval - horiz) < 0.78125;
								}
								else check = 1;

								int cans = cansee(act->spr.pos.plusZ(-32 + gs.actorinfo[act->spr.picnum].aimoffset), act->sector(), actor->spr.pos.plusZ(-32), actor->sector());

								if (check && cans)
								{
									smax = sdist;
									aimed = act;
								}
							}
						}
				}
		}
	}

	return aimed;
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void dokneeattack(int snum)
{
	auto p = &ps[snum];
	auto pact = p->GetActor();

	if (p->knee_incs > 0)
	{
		p->oknee_incs = p->knee_incs;
		p->knee_incs++;
		pact->spr.Angles.Pitch = (pact->getPosWithOffsetZ() - p->actorsqu->spr.pos.plusZ(-4)).Pitch();
		p->sync.actions |= SB_CENTERVIEW;
		if (p->knee_incs > 15)
		{
			p->oknee_incs = p->knee_incs = 0;
			p->holster_weapon = 0;
			if (p->weapon_pos < 0)
				p->weapon_pos = -p->weapon_pos;
			if (p->actorsqu != nullptr && (pact->spr.pos - p->actorsqu->spr.pos).Length() < 1400/16.)
			{
				spawnguts(p->actorsqu, PClass::FindActor("DukeJibs6"), 7);
				spawn(p->actorsqu, TILE_BLOODPOOL);
				S_PlayActorSound(SQUISHED, p->actorsqu);
				if (actorflag(p->actorsqu, SFLAG2_TRIGGERRESPAWN))
				{
					if (p->actorsqu->spr.yint)
						operaterespawns(p->actorsqu->spr.yint);
				}

				if (p->actorsqu->isPlayer())
				{
					quickkill(&ps[p->actorsqu->PlayerIndex()]);
					ps[p->actorsqu->PlayerIndex()].frag_ps = snum;
				}
				else if (badguy(p->actorsqu))
				{
					p->actorsqu->Destroy();
					p->actors_killed++;
				}
				else p->actorsqu->Destroy();
			}
			p->actorsqu = nullptr;
		}
	}

}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

int makepainsounds(int snum, int type)
{
	auto p = &ps[snum];
	auto actor = p->GetActor();
	int k = 0;

	switch (type)
	{
	case 0:
		if (rnd(32))
		{
			if (p->boot_amount > 0)
				k = 1;
			else
			{
				if (!S_CheckActorSoundPlaying(actor, DUKE_LONGTERM_PAIN))
					S_PlayActorSound(DUKE_LONGTERM_PAIN, actor);
				SetPlayerPal(p, PalEntry(32, 64, 64, 64));
				actor->spr.extra -= 1 + (krand() & 3);
				if (!S_CheckActorSoundPlaying(actor, SHORT_CIRCUIT))
					S_PlayActorSound(SHORT_CIRCUIT, actor);
			}
		}
		break;
	case 1:
		if (rnd(16))
		{
			if (p->boot_amount > 0)
				k = 1;
			else
			{
				if (!S_CheckActorSoundPlaying(actor, DUKE_LONGTERM_PAIN))
					S_PlayActorSound(DUKE_LONGTERM_PAIN, actor);
				SetPlayerPal(p, PalEntry(32, 0, 8, 0));
				actor->spr.extra -= 1 + (krand() & 3);
			}
		}
		break;
	case 2:
		if (rnd(32))
		{
			if (p->boot_amount > 0)
				k = 1;
			else
			{
				if (!S_CheckActorSoundPlaying(actor, DUKE_LONGTERM_PAIN))
					S_PlayActorSound(DUKE_LONGTERM_PAIN, actor);
				SetPlayerPal(p, PalEntry(32, 8, 0, 0));
				actor->spr.extra -= 1 + (krand() & 3);
			}
		}
		break;
	case 3:
		if ((krand() & 3) == 1)
			if (p->on_ground)
			{
				if (p->OnMotorcycle)
					actor->spr.extra -= 2;
				else
					actor->spr.extra -= 4;
				S_PlayActorSound(DUKE_LONGTERM_PAIN, actor);
			}
		break;
	}
	return k;
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void footprints(int snum)
{
	auto p = &ps[snum];
	auto actor = p->GetActor();

	if (p->footprintcount > 0 && p->on_ground)
		if (p->insector() && (p->cursector->floorstat & CSTAT_SECTOR_SLOPE) != 2)
		{
			int j = -1;
			DukeSectIterator it(actor->sector());
			while (auto act = it.Next())
			{
				if (act->IsKindOf(NAME_DukeFootprints))
					if (abs(act->spr.pos.X - p->GetActor()->spr.pos.X) < 24)
						if (abs(act->spr.pos.Y - p->GetActor()->spr.pos.Y) < 24)
						{
							j = 1;
							break;
						}
			}
			if (j < 0)
			{
				p->footprintcount--;
				if (p->cursector->lotag == 0 && p->cursector->hitag == 0)
				{
					DDukeActor* fprint = spawn(actor, PClass::FindActor(NAME_DukeFootprints));
					if (fprint)
					{
						fprint->spr.Angles.Yaw = p->actor->spr.Angles.Yaw;
						fprint->spr.pal = p->footprintpal;
						fprint->spr.shade = (int8_t)p->footprintshade;
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

void playerisdead(int snum, int psectlotag, double floorz, double ceilingz)
{
	auto p = &ps[snum];
	auto actor = p->GetActor();

	if (p->dead_flag == 0)
	{
		if (actor->spr.pal != 1)
		{
			SetPlayerPal(p, PalEntry(63, 63, 0, 0));
			actor->spr.pos.Z -= 16;
		}
#if 0
		if (ud.recstat == 1 && ud.multimode < 2)
			closedemowrite();
#endif

		if (actor->spr.pal != 1)
			p->dead_flag = (512 - ((krand() & 1) << 10) + (krand() & 255) - 512) & 2047;

		p->jetpack_on = 0;
		p->holoduke_on = nullptr;

		if (!isRR())S_StopSound(DUKE_JETPACK_IDLE, actor);
		S_StopSound(-1, actor, CHAN_VOICE);


		if (actor->spr.pal != 1 && (actor->spr.cstat & CSTAT_SPRITE_INVISIBLE) == 0) actor->spr.cstat = 0;

		if (ud.multimode > 1 && (actor->spr.pal != 1 || (actor->spr.cstat & CSTAT_SPRITE_INVISIBLE)))
		{
			if (p->frag_ps != snum)
			{
				ps[p->frag_ps].frag++;
				ps[p->frag_ps].frags[snum]++;

				auto pname = PlayerName(p->frag_ps);
				if (snum == screenpeek)
				{
					Printf(PRINT_NOTIFY, "Killed by %s", pname);
				}
				else
				{
					Printf(PRINT_NOTIFY, "Killed %s", pname);
				}

			}
			else p->fraggedself++;

			p->frag_ps = snum;
		}
	}

	if (psectlotag == ST_2_UNDERWATER)
	{
		if (p->on_warping_sector == 0)
		{
			if (abs(p->GetActor()->getOffsetZ() - floorz) > (gs.playerheight * 0.5))
				p->GetActor()->spr.pos.Z += 348/ 256.;
		}
		else
		{
			actor->spr.pos.Z -= 2;
			actor->vel.Z = -348 / 256.;
		}

		Collision coll;
		clipmove(p->GetActor()->spr.pos.XY(), p->GetActor()->getOffsetZ(), &p->cursector, DVector2( 0, 0), 10.25, 4., 4., CLIPMASK0, coll);
	}

	actor->backuploc();

	p->Angles.ViewAngles.Pitch = p->GetActor()->spr.Angles.Pitch = nullAngle;

	updatesector(p->GetActor()->getPosWithOffsetZ(), &p->cursector);

	pushmove(p->GetActor()->spr.pos.XY(), p->GetActor()->getOffsetZ(), &p->cursector, 8, 4, 20, CLIPMASK0);
	
	if (floorz > ceilingz + 16 && actor->spr.pal != 1)
		p->Angles.ViewAngles.Roll = DAngle::fromBuild(-(p->dead_flag + ((floorz + p->GetActor()->getOffsetZ()) * 2)));

	p->on_warping_sector = 0;

}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

int endoflevel(int snum)
{
	auto p = &ps[snum];

	// the fist puching the end-of-level thing...
	p->ofist_incs = p->fist_incs;
	p->fist_incs++;
	if (p->fist_incs == 28)
	{
#if 0
		if (ud.recstat == 1) closedemowrite();
#endif
		S_PlaySound(PIPEBOMB_EXPLODE);
		SetPlayerPal(p, PalEntry(48, 64, 64, 64));
	}
	if (p->fist_incs > 42)
	{
		setnextmap(!!p->buttonpalette);
		return 1;
	}
	return 0;
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

int timedexit(int snum)
{
	auto p = &ps[snum];
	p->timebeforeexit--;
	if (p->timebeforeexit == 26 * 5)
	{
		FX_StopAllSounds();
		if (p->customexitsound >= 0)
		{
			S_PlaySound(p->customexitsound);
			FTA(102, p);
		}
	}
	else if (p->timebeforeexit == 1)
	{
		setnextmap(false);
		return true;
	}
	return false;
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void playerCrouch(int snum)
{
	auto p = &ps[snum];
	// crouching
	SetGameVarID(g_iReturnVarID, 0, p->GetActor(), snum);
	OnEvent(EVENT_CROUCH, snum, p->GetActor(), -1);
	if (GetGameVarID(g_iReturnVarID, p->GetActor(), snum).value() == 0)
	{
		p->GetActor()->spr.pos.Z += 8 + 3;
		p->crack_time = CRACK_TIME;
	}
}

void playerJump(int snum, double floorz, double ceilingz)
{
	auto p = &ps[snum];
	if (p->jumping_toggle == 0 && p->jumping_counter == 0)
	{
		if ((floorz - ceilingz) > 56)
		{
			SetGameVarID(g_iReturnVarID, 0, p->GetActor(), snum);
			OnEvent(EVENT_JUMP, snum, p->GetActor(), -1);
			if (GetGameVarID(g_iReturnVarID, p->GetActor(), snum).value() == 0)
			{
				p->jumping_counter = 1;
				p->jumping_toggle = 1;
			}
		}
	}
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void player_struct::apply_seasick()
{
	if (isRRRA() && SeaSick && (dead_flag == 0 || (dead_flag && resurrected)))
	{
		if (SeaSick < 250)
		{
			static constexpr DAngle adjustment = DAngle::fromDeg(4.21875);

			if (SeaSick >= 180)
				Angles.ViewAngles.Roll -= adjustment;
			else if (SeaSick >= 130)
				Angles.ViewAngles.Roll += adjustment;
			else if (SeaSick >= 70)
				Angles.ViewAngles.Roll -= adjustment;
			else if (SeaSick >= 20)
				Angles.ViewAngles.Roll += adjustment;
		}
		if (SeaSick < 250)
			Angles.ViewAngles.Yaw = DAngle::fromDeg(krandf(45) - 22.5);
	}
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void player_struct::backuppos(bool noclipping)
{
	if (!noclipping)
	{
		GetActor()->backupvec2();
	}
	else
	{
		GetActor()->restorevec2();
	}

	GetActor()->backupz();
	bobpos = GetActor()->spr.pos.XY();
	opyoff = pyoff;
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void player_struct::backupweapon()
{
	oweapon_sway = weapon_sway;
	oweapon_pos = weapon_pos;
	okickback_pic = kickback_pic;
	orandom_club_frame = random_club_frame;
	ohard_landing = hard_landing;
	ofistsign = fistsign;
	otipincs = tipincs;
	oknee_incs = knee_incs;
	oaccess_incs = access_incs;
	ofist_incs = fist_incs;
	oloogcnt = loogcnt;
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void player_struct::checkhardlanding()
{
	if (hard_landing > 0)
	{
		GetActor()->spr.Angles.Pitch += maphoriz(hard_landing << 4);
		hard_landing--;
	}
}

void player_struct::playerweaponsway(double xvel)
{
	if (cl_weaponsway)
	{
		if (xvel < 2 || on_ground == 0 || bobcounter == 1024)
		{
			if ((weapon_sway & 2047) > (1024 + 96))
				weapon_sway -= 96;
			else if ((weapon_sway & 2047) < (1024 - 96))
				weapon_sway += 96;
			else oweapon_sway = weapon_sway = 1024;
		}
		else
		{
			weapon_sway = bobcounter;

			if ((bobcounter - oweapon_sway) > 256)
			{
				oweapon_sway = weapon_sway;
			}
		}
	}
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void checklook(int snum, ESyncBits actions)
{
	auto p = &ps[snum];

	if ((actions & SB_LOOK_LEFT) && !p->OnMotorcycle)
	{
		SetGameVarID(g_iReturnVarID, 0, p->GetActor(), snum);
		OnEvent(EVENT_LOOKLEFT, snum, p->GetActor(), -1);
		if (GetGameVarID(g_iReturnVarID, p->GetActor(), snum).value() != 0)
		{
			actions &= ~SB_LOOK_LEFT;
		}
	}

	if ((actions & SB_LOOK_RIGHT) && !p->OnMotorcycle)
	{
		SetGameVarID(g_iReturnVarID, 0, p->GetActor(), snum);
		OnEvent(EVENT_LOOKRIGHT, snum, p->GetActor(), -1);
		if (GetGameVarID(g_iReturnVarID, p->GetActor(), snum).value() != 0)
		{
			actions &= ~SB_LOOK_RIGHT;
		}
	}
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void playerCenterView(int snum)
{
	auto p = &ps[snum];
	SetGameVarID(g_iReturnVarID, 0, p->GetActor(), snum);
	OnEvent(EVENT_RETURNTOCENTER, snum, p->GetActor(), -1);
	if (GetGameVarID(g_iReturnVarID, p->GetActor(), snum).value() == 0)
	{
		p->sync.actions |= SB_CENTERVIEW;
	}
	else
	{
		p->sync.actions &= ~SB_CENTERVIEW;
	}
}

void playerLookUp(int snum, ESyncBits actions)
{
	auto p = &ps[snum];
	SetGameVarID(g_iReturnVarID, 0, p->GetActor(), snum);
	OnEvent(EVENT_LOOKUP, snum, p->GetActor(), -1);
	if (GetGameVarID(g_iReturnVarID, p->GetActor(), snum).value() == 0)
	{
		p->sync.actions |= SB_CENTERVIEW;
	}
	else
	{
		p->sync.actions &= ~SB_LOOK_UP;
	}
}

void playerLookDown(int snum, ESyncBits actions)
{
	auto p = &ps[snum];
	SetGameVarID(g_iReturnVarID, 0, p->GetActor(), snum);
	OnEvent(EVENT_LOOKDOWN, snum, p->GetActor(), -1);
	if (GetGameVarID(g_iReturnVarID, p->GetActor(), snum).value() == 0)
	{
		p->sync.actions |= SB_CENTERVIEW;
	}
	else
	{
		p->sync.actions &= ~SB_LOOK_DOWN;
	}
}

void playerAimUp(int snum, ESyncBits actions)
{
	auto p = &ps[snum];
	SetGameVarID(g_iReturnVarID, 0, p->GetActor(), snum);
	OnEvent(EVENT_AIMUP, snum, p->GetActor(), -1);
	if (GetGameVarID(g_iReturnVarID, p->GetActor(), snum).value() != 0)
	{
		p->sync.actions &= ~SB_AIM_UP;
	}
}

void playerAimDown(int snum, ESyncBits actions)
{
	auto p = &ps[snum];
	SetGameVarID(g_iReturnVarID, 0, p->GetActor(), snum);
	OnEvent(EVENT_AIMDOWN, snum, p->GetActor(), -1);	// due to a typo in WW2GI's CON files this is the same as EVENT_AIMUP.
	if (GetGameVarID(g_iReturnVarID, p->GetActor(), snum).value() != 0)
	{
		p->sync.actions &= ~SB_AIM_DOWN;
	}
}

//---------------------------------------------------------------------------
//
// split out so that the weapon check can be done right.
//
//---------------------------------------------------------------------------

bool movementBlocked(player_struct *p)
{
	auto blockingweapon = [=]()
	{
		if (isRR()) return false;
		if (isWW2GI()) return aplWeaponWorksLike(p->curr_weapon, p->GetPlayerNum()) == TRIPBOMB_WEAPON;
		else return p->curr_weapon == TRIPBOMB_WEAPON;
	};

	auto weapondelay = [=]()
	{
		if (isWW2GI()) return aplWeaponFireDelay(p->curr_weapon, p->GetPlayerNum());
		else return 4;
	};

	return (p->fist_incs ||
		p->transporter_hold > 2 ||
		p->hard_landing ||
		p->access_incs > 0 ||
		p->knee_incs > 0 ||
		(blockingweapon() && p->kickback_pic > 1 && p->kickback_pic < weapondelay()));
}


//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

int haskey(sectortype* sectp, int snum)
{
	auto p = &ps[snum];
	if (!sectp)
		return 0;
	if (!sectp->keyinfo)
		return 1;
	if (sectp->keyinfo > 6)
		return 1;
	int wk = sectp->keyinfo;
	if (wk > 3)
		wk -= 3;

	if (p->keys[wk] == 1)
	{
		sectp->keyinfo = 0;
		return 1;
	}

	return 0;
}


END_DUKE_NS
