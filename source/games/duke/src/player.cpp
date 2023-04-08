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
	else if (tilesurface(p->cursector->ceilingtexture) == TSURF_SLIME) palette = SLIMEPAL;
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

void forceplayerangle(player_struct* p)
{
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
				CreateActor(sect, pos, PClass::FindActor(NAME_DukeWaterBubble), -32, scale, randomAngle(), 0., 0., ps[0].GetActor(), 5);
			}
			else
				CreateActor(sect, pos, PClass::FindActor(NAME_DukeSmallSmoke), -32, DVector2(0.21875, 0.21875), nullAngle, 0., 0., ps[0].GetActor(), 5);
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
	DAngle aang = DAngle90 * (+AUTO_AIM_ANGLE / 512.);

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
				hitscan(plr->GetActor()->getPosWithOffsetZ().plusZ(4), actor->sector(), DVector3(actor->spr.Angles.Yaw.ToVector() * vel, zvel * 64), hit, CLIPMASK1);

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
				if ((weap > CHAINGUN_WEAPON && weap != GROW_WEAPON) || weap == KNEE_WEAPON)
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
					addkill(p->actorsqu);
					p->actorsqu->Destroy();
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

	// lock input when dead.
	setForcedSyncInput(snum);

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
			if (abs(actor->getOffsetZ() - floorz) > (gs.playerheight * 0.5))
				actor->spr.pos.Z += 348/ 256.;
		}
		else
		{
			actor->spr.pos.Z -= 2;
			actor->vel.Z = -348 / 256.;
		}

		Collision coll;
		clipmove(actor->spr.pos.XY(), actor->getOffsetZ(), &p->cursector, DVector2( 0, 0), 10.25, 4., 4., CLIPMASK0, coll);
	}

	actor->backuploc();

	p->Angles.ViewAngles.Pitch = actor->spr.Angles.Pitch = nullAngle;

	updatesector(actor->getPosWithOffsetZ(), &p->cursector);

	pushmove(actor->spr.pos.XY(), actor->getOffsetZ(), &p->cursector, 8, 4, 20, CLIPMASK0);
	
	if (floorz > ceilingz + 16 && actor->spr.pal != 1)
		p->Angles.ViewAngles.Roll = DAngle::fromBuild(-(p->dead_flag + ((floorz + actor->getOffsetZ()) * 2)));

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
	if (isRRRA() && SeaSick && (dead_flag == 0))
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
		GetActor()->spr.Angles.Pitch += maphoriz(hard_landing << 4) * !!(cl_dukepitchmode & kDukePitchHardLanding);
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
		p->sync.horz = 0;
		setForcedSyncInput(snum);
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
		p->GetActor()->spr.extra <= 0 ||
		(p->dead_flag && !ud.god) ||
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

//---------------------------------------------------------------------------
//
// taken out of Duke, now that it is no longer hard coded.
//
//---------------------------------------------------------------------------

void purplelavacheck(player_struct* p)
{
	auto pact = p->actor;
	if (p->spritebridge == 0 && pact->insector())
	{
		auto sect = pact->sector();
		// one texflag for a single texture again, just to avoid one hard coded check...
		if ((tilesurface(sect->floortexture) == TSURF_PURPLELAVA) || (tilesurface(sect->ceilingtexture) == TSURF_PURPLELAVA))
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
	}
}


//---------------------------------------------------------------------------
//
// moved out of the CON interpreter.
//
//---------------------------------------------------------------------------

bool addphealth(player_struct* p, int amount, bool bigitem)
{
	if (p->newOwner != nullptr)
	{
		p->newOwner = nullptr;
		p->GetActor()->restoreloc();
		updatesector(p->GetActor()->getPosWithOffsetZ(), &p->cursector);

		DukeStatIterator it(STAT_ACTOR);
		while (auto actj = it.Next())
		{
			if (actorflag(actj, SFLAG2_CAMERA))
				actj->spr.yint = 0;
		}
	}

	int curhealth = p->GetActor()->spr.extra;

	if (!bigitem)
	{
		if (curhealth > gs.max_player_health && amount > 0)
		{
			return false;
		}
		else
		{
			if (curhealth > 0)
				curhealth += amount;
			if (curhealth > gs.max_player_health && amount > 0)
				curhealth = gs.max_player_health;
		}
	}
	else
	{
		if (curhealth > 0)
			curhealth += amount;
		if (curhealth > (gs.max_player_health << 1))
			curhealth = (gs.max_player_health << 1);
	}

	if (curhealth < 0) curhealth = 0;

	if (ud.god == 0)
	{
		if (amount > 0)
		{
			if ((curhealth - amount) < (gs.max_player_health >> 2) &&
				curhealth >= (gs.max_player_health >> 2))
				S_PlayActorSound(PLAYER_GOTHEALTHATLOW, p->GetActor());

			p->last_extra = curhealth;
		}

		p->GetActor()->spr.extra = curhealth;
	}
	return true;
}

//---------------------------------------------------------------------------
//
// moved out of the CON interpreter.
//
//---------------------------------------------------------------------------

bool playereat(player_struct* p, int amount, bool bigitem)
{
	p->eat += amount;
	if (p->eat > 100)
	{
		p->eat = 100;
	}
	p->drink_amt -= amount;
	if (p->drink_amt < 0)
		p->drink_amt = 0;
	int curhealth = p->GetActor()->spr.extra;
	if (!bigitem)
	{
		if (curhealth > gs.max_player_health && amount > 0)
		{
			return false;
		}
		else
		{
			if (curhealth > 0)
				curhealth += (amount) * 3;
			if (curhealth > gs.max_player_health && amount > 0)
				curhealth = gs.max_player_health;
		}
	}
	else
	{
		if (curhealth > 0)
			curhealth += amount;
		if (curhealth > (gs.max_player_health << 1))
			curhealth = (gs.max_player_health << 1);
	}

	if (curhealth < 0) curhealth = 0;

	if (ud.god == 0)
	{
		if (amount > 0)
		{
			if ((curhealth - amount) < (gs.max_player_health >> 2) &&
				curhealth >= (gs.max_player_health >> 2))
				S_PlayActorSound(PLAYER_GOTHEALTHATLOW, p->GetActor());

			p->last_extra = curhealth;
		}

		p->GetActor()->spr.extra = curhealth;
	}
	return true;
}

//---------------------------------------------------------------------------
//
// moved out of the CON interpreter.
//
//---------------------------------------------------------------------------

void playerdrink(player_struct* p, int amount)
{
	p->drink_amt += amount;
	int curhealth = p->GetActor()->spr.extra;
	if (curhealth > 0)
		curhealth += amount;
	if (curhealth > gs.max_player_health * 2)
		curhealth = gs.max_player_health * 2;
	if (curhealth < 0)
		curhealth = 0;

	if (ud.god == 0)
	{
		if (amount > 0)
		{
			if ((curhealth - amount) < (gs.max_player_health >> 2) &&
				curhealth >= (gs.max_player_health >> 2))
				S_PlayActorSound(PLAYER_GOTHEALTHATLOW, p->GetActor());

			p->last_extra = curhealth;
		}

		p->GetActor()->spr.extra = curhealth;
	}
	if (p->drink_amt > 100)
		p->drink_amt = 100;

	if (p->GetActor()->spr.extra >= gs.max_player_health)
	{
		p->GetActor()->spr.extra = gs.max_player_health;
		p->last_extra = gs.max_player_health;
	}
}

//---------------------------------------------------------------------------
//
// moved out of the CON interpreter.
//
//---------------------------------------------------------------------------

bool playeraddammo(player_struct* p, int weaponindex, int amount)
{
	if (p->ammo_amount[weaponindex] >= gs.max_ammo_amount[weaponindex])
	{
		return false;
	}
	addammo(weaponindex, p, amount);
	if (p->curr_weapon == KNEE_WEAPON)
		if (p->gotweapon[weaponindex] && (WeaponSwitch(p - ps) & 1))
			fi.addweapon(p, weaponindex, true);
	return true;
}

bool playeraddweapon(player_struct* p, int weaponindex, int amount)
{
	if (p->gotweapon[weaponindex] == 0) fi.addweapon(p, weaponindex, !!(WeaponSwitch(p- ps) & 1));
	else if (p->ammo_amount[weaponindex] >= gs.max_ammo_amount[weaponindex])
	{
		return false;
	}
	addammo(weaponindex, p, amount);
	if (p->curr_weapon == KNEE_WEAPON)
		if (p->gotweapon[weaponindex] && (WeaponSwitch(p - ps) & 1))
			fi.addweapon(p, weaponindex, true);

	return true;
}

//---------------------------------------------------------------------------
//
// moved out of the CON interpreter.
//
//---------------------------------------------------------------------------

void playeraddinventory(player_struct* p, DDukeActor* item, int type, int amount)
{
	switch (type)
	{
	case 0:
		p->steroids_amount = amount;
		p->inven_icon = 2;
		break;
	case 1:
		p->shield_amount += amount;// 100;
		if (p->shield_amount > gs.max_player_health)
			p->shield_amount = gs.max_player_health;
		break;
	case 2:
		p->scuba_amount = amount;// 1600;
		p->inven_icon = 6;
		break;
	case 3:
		p->holoduke_amount = amount;// 1600;
		p->inven_icon = 3;
		break;
	case 4:
		p->jetpack_amount = amount;// 1600;
		p->inven_icon = 4;
		break;
	case 6:
		if (isRR())
		{
			switch (item->spr.lotag)
			{
			case 100: p->keys[1] = 1; break;
			case 101: p->keys[2] = 1; break;
			case 102: p->keys[3] = 1; break;
			case 103: p->keys[4] = 1; break;
			}
		}
		else
		{
			switch (item->spr.pal)
			{
			case  0: p->got_access |= 1; break;
			case 21: p->got_access |= 2; break;
			case 23: p->got_access |= 4; break;
			}
		}
		break;
	case 7:
		p->heat_amount = amount;
		p->inven_icon = 5;
		break;
	case 9:
		p->inven_icon = 1;
		p->firstaid_amount = amount;
		break;
	case 10:
		p->inven_icon = 7;
		p->boot_amount = amount;
		break;
	}
}

//---------------------------------------------------------------------------
//
// moved out of the CON interpreter.
//
//---------------------------------------------------------------------------

bool checkp(DDukeActor* self, player_struct* p, int flags)
{
	bool j = 0;

	double vel = self->vel.X;
	unsigned plindex = unsigned(p - ps);

	// sigh.. this was yet another place where number literals were used as bit masks for every single value, making the code totally unreadable.
	if ((flags & pducking) && p->on_ground && PlayerInput(plindex, SB_CROUCH))
		j = 1;
	else if ((flags & pfalling) && p->jumping_counter == 0 && !p->on_ground && p->vel.Z > 8)
		j = 1;
	else if ((flags & pjumping) && p->jumping_counter > 348)
		j = 1;
	else if ((flags & pstanding) && vel >= 0 && vel < 0.5)
		j = 1;
	else if ((flags & pwalking) && vel >= 0.5 && !(PlayerInput(plindex, SB_RUN)))
		j = 1;
	else if ((flags & prunning) && vel >= 0.5 && PlayerInput(plindex, SB_RUN))
		j = 1;
	else if ((flags & phigher) && p->GetActor()->getOffsetZ() < self->spr.pos.Z - 48)
		j = 1;
	else if ((flags & pwalkingback) && vel <= -0.5 && !(PlayerInput(plindex, SB_RUN)))
		j = 1;
	else if ((flags & prunningback) && vel <= -0.5 && (PlayerInput(plindex, SB_RUN)))
		j = 1;
	else if ((flags & pkicking) && (p->quick_kick > 0 || (p->curr_weapon == KNEE_WEAPON && p->kickback_pic > 0)))
		j = 1;
	else if ((flags & pshrunk) && p->GetActor()->spr.scale.X < (isRR() ? 0.125 : 0.5))
		j = 1;
	else if ((flags & pjetpack) && p->jetpack_on)
		j = 1;
	else if ((flags & ponsteroids) && p->steroids_amount > 0 && p->steroids_amount < 400)
		j = 1;
	else if ((flags & ponground) && p->on_ground)
		j = 1;
	else if ((flags & palive) && p->GetActor()->spr.scale.X > (isRR() ? 0.125 : 0.5) && p->GetActor()->spr.extra > 0 && p->timebeforeexit == 0)
		j = 1;
	else if ((flags & pdead) && p->GetActor()->spr.extra <= 0)
		j = 1;
	else if ((flags & pfacing))
	{
		DAngle ang;
		if (self->isPlayer() && ud.multimode > 1)
			ang = absangle(ps[otherp].GetActor()->spr.Angles.Yaw, (p->GetActor()->spr.pos.XY() - ps[otherp].GetActor()->spr.pos.XY()).Angle());
		else
			ang = absangle(p->GetActor()->spr.Angles.Yaw, (self->spr.pos.XY() - p->GetActor()->spr.pos.XY()).Angle());

		j = ang < DAngle22_5;
	}
	return j;
}

//---------------------------------------------------------------------------
//
// moved out of the CON interpreter.
//
//---------------------------------------------------------------------------

bool playercheckinventory(player_struct* p, DDukeActor* item, int type, int amount)
{
	bool j = 0;
	switch (type)
	{
	case 0:
		if (p->steroids_amount != amount)
			j = 1;
		break;
	case 1:
		if (p->shield_amount != gs.max_player_health)
			j = 1;
		break;
	case 2:
		if (p->scuba_amount != amount) j = 1;
		break;
	case 3:
		if (p->holoduke_amount != amount) j = 1;
		break;
	case 4:
		if (p->jetpack_amount != amount) j = 1;
		break;
	case 6:
		if (isRR())
		{
			switch (item->spr.lotag)
			{
			case 100:
				if (p->keys[1]) j = 1;
				break;
			case 101:
				if (p->keys[2]) j = 1;
				break;
			case 102:
				if (p->keys[3]) j = 1;
				break;
			case 103:
				if (p->keys[4]) j = 1;
				break;
			}
		}
		else
		{
			switch (item->spr.pal)
			{
			case  0:
				if (p->got_access & 1) j = 1;
				break;
			case 21:
				if (p->got_access & 2) j = 1;
				break;
			case 23:
				if (p->got_access & 4) j = 1;
				break;
			}
		}
		break;
	case 7:
		if (p->heat_amount != amount) j = 1;
		break;
	case 9:
		if (p->firstaid_amount != amount) j = 1;
		break;
	case 10:
		if (p->boot_amount != amount) j = 1;
		break;
	}
	return j;
}

//---------------------------------------------------------------------------
//
// moved out of the CON interpreter.
//
//---------------------------------------------------------------------------

void playerstomp(player_struct* p, DDukeActor* stomped)
{
	if (p->knee_incs == 0 && p->GetActor()->spr.scale.X >= (isRR() ? 0.140625 : 0.625))
		if (cansee(stomped->spr.pos.plusZ(-4), stomped->sector(), p->GetActor()->getPosWithOffsetZ().plusZ(16), p->GetActor()->sector()))
		{
			p->knee_incs = 1;
			if (p->weapon_pos == 0)
				p->weapon_pos = -1;
			p->actorsqu = stomped;
		}
}

//---------------------------------------------------------------------------
//
// moved out of the CON interpreter.
//
//---------------------------------------------------------------------------

void playerreset(player_struct* p, DDukeActor* g_ac)
{
	if (ud.multimode < 2)
	{
		gameaction = ga_autoloadgame;
		g_ac->killit_flag = 2;
	}
	else
	{
		// I am not convinced this is even remotely smart to be executed from here..
		pickrandomspot(int(p - ps));
		g_ac->spr.pos = p->GetActor()->getPosWithOffsetZ();
		p->GetActor()->backuppos();
		p->setbobpos();
		g_ac->backuppos();
		updatesector(p->GetActor()->getPosWithOffsetZ(), &p->cursector);
		SetActor(p->GetActor(), p->GetActor()->spr.pos);
		g_ac->spr.cstat = CSTAT_SPRITE_BLOCK_ALL;

		g_ac->spr.shade = -12;
		g_ac->clipdist = 16;
		g_ac->spr.scale = DVector2(0.65625, 0.5625);
		g_ac->SetOwner(g_ac);
		g_ac->spr.xoffset = 0;
		g_ac->spr.pal = p->palookup;

		p->last_extra = g_ac->spr.extra = gs.max_player_health;
		p->wantweaponfire = -1;
		p->GetActor()->PrevAngles.Pitch = p->GetActor()->spr.Angles.Pitch = nullAngle;
		p->on_crane = nullptr;
		p->frag_ps = int(p - ps);
		p->Angles.PrevViewAngles.Pitch = p->Angles.ViewAngles.Pitch = nullAngle;
		p->opyoff = 0;
		p->wackedbyactor = nullptr;
		p->shield_amount = gs.max_armour_amount;
		p->dead_flag = 0;
		p->pals.a = 0;
		p->footprintcount = 0;
		p->weapreccnt = 0;
		p->ftq = 0;
		p->vel.X = p->vel.Y = 0;
		if (!isRR()) p->Angles.PrevViewAngles.Roll = p->Angles.ViewAngles.Roll = nullAngle;

		p->falling_counter = 0;

		g_ac->hitextra = -1;

		g_ac->cgg = 0;
		g_ac->movflag = 0;
		g_ac->tempval = 0;
		g_ac->actorstayput = nullptr;
		g_ac->dispicnum = 0;
		g_ac->SetHitOwner(p->GetActor());
		g_ac->temp_data[4] = 0;

		resetinventory(p);
		resetweapons(p);
	}
}

//---------------------------------------------------------------------------
//
// moved out of the CON interpreter.
//
//---------------------------------------------------------------------------

void wackplayer(player_struct* p)
{
	if (!isRR())
		forceplayerangle(p);
	else
	{
		p->vel.XY() -= p->GetActor()->spr.Angles.Yaw.ToVector() * 64;
		p->jumping_counter = 767;
		p->jumping_toggle = 1;
	}

}

//---------------------------------------------------------------------------
//
// moved out of the CON interpreter.
//
//---------------------------------------------------------------------------

void playerkick(player_struct* p, DDukeActor* g_ac)
{
	if (ud.multimode > 1 && g_ac->isPlayer())
	{
		if (ps[otherp].quick_kick == 0)
			ps[otherp].quick_kick = 14;
	}
	else if (!g_ac->isPlayer() && p->quick_kick == 0)
		p->quick_kick = 14;
}

END_DUKE_NS
