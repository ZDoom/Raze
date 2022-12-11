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
#include "sounds.h"
#include "names_d.h"
#include "mapinfo.h"
#include "dukeactor.h"
#include "secrets.h"

// PRIMITIVE
BEGIN_DUKE_NS

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void animatewalls_d(void)
{
	int t;

	for (int p = 0; p < numanimwalls; p++)
	{
		auto wal = animwall[p].wall;
		int j = wal->wallpicnum;

		switch (j)
		{
		case DTILE_SCREENBREAK1:
		case DTILE_SCREENBREAK2:
		case DTILE_SCREENBREAK3:
		case DTILE_SCREENBREAK4:
		case DTILE_SCREENBREAK5:

		case DTILE_SCREENBREAK9:
		case DTILE_SCREENBREAK10:
		case DTILE_SCREENBREAK11:
		case DTILE_SCREENBREAK12:
		case DTILE_SCREENBREAK13:
		case DTILE_SCREENBREAK14:
		case DTILE_SCREENBREAK15:
		case DTILE_SCREENBREAK16:
		case DTILE_SCREENBREAK17:
		case DTILE_SCREENBREAK18:
		case DTILE_SCREENBREAK19:

			if ((krand() & 255) < 16)
			{
				animwall[p].tag = wal->wallpicnum;
				wal->wallpicnum = DTILE_SCREENBREAK6;
			}

			continue;

		case DTILE_SCREENBREAK6:
		case DTILE_SCREENBREAK7:
		case DTILE_SCREENBREAK8:

			if (animwall[p].tag >= 0 && wal->extra != DTILE_FEMPIC2 && wal->extra != DTILE_FEMPIC3)
				wal->wallpicnum = animwall[p].tag;
			else
			{
				wal->wallpicnum++;
				if (wal->wallpicnum == (DTILE_SCREENBREAK6 + 3))
					wal->wallpicnum = DTILE_SCREENBREAK6;
			}
			continue;

		}

		if (wal->cstat & CSTAT_WALL_MASKED)
			switch (wal->overpicnum)
			{
			case DTILE_W_FORCEFIELD:
			case DTILE_W_FORCEFIELD2:
			case DTILE_W_FORCEFIELD3:

				t = animwall[p].tag;

				if (wal->cstat & CSTAT_WALL_ANY_EXCEPT_BLOCK)
				{
					wal->addxpan(-t / 4096.f);
					wal->addypan(-t / 4096.f);

					if (wal->extra == 1)
					{
						wal->extra = 0;
						animwall[p].tag = 0;
					}
					else
						animwall[p].tag += 128;

					if (animwall[p].tag < (128 << 4))
					{
						if (animwall[p].tag & 128)
							wal->overpicnum = DTILE_W_FORCEFIELD;
						else wal->overpicnum = DTILE_W_FORCEFIELD2;
					}
					else
					{
						if ((krand() & 255) < 32)
							animwall[p].tag = 128 << (krand() & 3);
						else wal->overpicnum = DTILE_W_FORCEFIELD2;
					}
				}

				break;
			}
	}
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void operateforcefields_d(DDukeActor* act, int low)
{
	operateforcefields_common(act, low, { DTILE_W_FORCEFIELD, DTILE_W_FORCEFIELD2, DTILE_W_FORCEFIELD3, DTILE_BIGFORCE });
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

bool checkaccessswitch_d(int snum, int switchpal, DDukeActor* act, walltype* wwal)
{
	if (ps[snum].access_incs == 0)
	{
		if (switchpal == 0)
		{
			if ((ps[snum].got_access & 1))
				ps[snum].access_incs = 1;
			else FTA(70, &ps[snum]);
		}

		else if (switchpal == 21)
		{
			if (ps[snum].got_access & 2)
				ps[snum].access_incs = 1;
			else FTA(71, &ps[snum]);
		}

		else if (switchpal == 23)
		{
			if (ps[snum].got_access & 4)
				ps[snum].access_incs = 1;
			else FTA(72, &ps[snum]);
		}

		if (ps[snum].access_incs == 1)
		{
			if (!act)
				ps[snum].access_wall = wwal;
			else
				ps[snum].access_spritenum = act;
		}

		return 1;
	}
	return 0;
}

//---------------------------------------------------------------------------
//
// how NOT to implement switch animations...
//
//---------------------------------------------------------------------------
void togglespriteswitches(DDukeActor* act, const TexExtInfo& ext, int lotag, int& correctdips, int& numdips);

bool checkhitswitch_d(int snum, walltype* wwal, DDukeActor *act)
{
	uint8_t switchpal;
	int lotag, hitag, picnum, correctdips, numdips;
	DVector2 spos;
	FTextureID texid;

	if (wwal == nullptr && act == nullptr) return 0;
	correctdips = 1;
	numdips = 0;

	if (act)
	{
		lotag = act->spr.lotag;
		if (lotag == 0) return 0;
		hitag = act->spr.hitag;
		spos = act->spr.pos.XY();
		picnum = act->spr.picnum;
		switchpal = act->spr.pal;

		// custom switches that maintain themselves can immediately abort.
		if (CallTriggerSwitch(act, &ps[snum])) return true;
	}
	else
	{
		lotag = wwal->lotag;
		if (lotag == 0) return 0;
		hitag = wwal->hitag;
		spos = wwal->pos;
		picnum = wwal->wallpicnum;
		switchpal = wwal->pal;
	}
	texid = tileGetTextureID(picnum);
	auto& ext = GetExtInfo(texid);
	auto& swdef = switches[ext.switchindex];

	switch (swdef.type)
	{
	case SwitchDef::Combo:
		break;

	case SwitchDef::Access:
		if (checkaccessswitch_d(snum, switchpal, act, wwal))
			return 0;
		[[fallthrough]];

	case SwitchDef::Regular:
	case SwitchDef::Multi:
		if (check_activator_motion(lotag)) return 0;
		break;

	default:
		if (isadoorwall(texid) == 0) return 0;
		break;
	}

	togglespriteswitches(act, ext, lotag, correctdips, numdips);

	for (auto& wal : wall)
	{
		if (lotag == wal.lotag)
			switch (wal.wallpicnum)
			{
			case DTILE_DIPSWITCH:
			case DTILE_TECHSWITCH:
			case DTILE_ALIENSWITCH:
				if (!act && &wal == wwal) wal.wallpicnum++;
				else if (wal.hitag == 0) correctdips++;
				numdips++;
				break;
			case DTILE_DIPSWITCHON:
			case DTILE_TECHSWITCHON:
			case DTILE_ALIENSWITCHON:
				if (!act && &wal == wwal) wal.wallpicnum--;
				else if (wal.hitag == 1) correctdips++;
				numdips++;
				break;
			case DTILE_MULTISWITCH:
			case DTILE_MULTISWITCH_2:
			case DTILE_MULTISWITCH_3:
			case DTILE_MULTISWITCH_4:
				wal.wallpicnum++;
				if (wal.wallpicnum > (DTILE_MULTISWITCH_4))
					wal.wallpicnum = DTILE_MULTISWITCH;
				break;
			case DTILE_ACCESSSWITCH:
			case DTILE_ACCESSSWITCH2:
			case DTILE_SLOTDOOR:
			case DTILE_LIGHTSWITCH:
			case DTILE_SPACELIGHTSWITCH:
			case DTILE_SPACEDOORSWITCH:
			case DTILE_LIGHTSWITCH2:
			case DTILE_POWERSWITCH1:
			case DTILE_LOCKSWITCH1:
			case DTILE_POWERSWITCH2:
			case DTILE_PULLSWITCH:
			case DTILE_HANDSWITCH:
			case DTILE_DIPSWITCH2:
			case DTILE_DIPSWITCH3:
				wal.wallpicnum++;
				break;
			case DTILE_HANDSWITCHON:
			case DTILE_PULLSWITCHON:
			case DTILE_LIGHTSWITCH2ON:
			case DTILE_POWERSWITCH1ON:
			case DTILE_LOCKSWITCH1ON:
			case DTILE_POWERSWITCH2ON:
			case DTILE_SLOTDOORON:
			case DTILE_LIGHTSWITCHON:
			case DTILE_SPACELIGHTSWITCHON:
			case DTILE_SPACEDOORSWITCHON:
			case DTILE_DIPSWITCH2ON:
			case DTILE_DIPSWITCH3ON:
				wal.wallpicnum--;
				break;
			}
	}

	if (lotag == -1)
	{
		setnextmap(false);
		return 1;
	}

	DVector3 v(spos, ps[snum].GetActor()->getOffsetZ());
	switch (picnum)
	{
	default:
		if (isadoorwall(texid) == 0) break;
		[[fallthrough]];
	case DTILE_DIPSWITCH:
	case DTILE_DIPSWITCHON:
	case DTILE_TECHSWITCH:
	case DTILE_TECHSWITCHON:
	case DTILE_ALIENSWITCH:
	case DTILE_ALIENSWITCHON:
		if (picnum == DTILE_DIPSWITCH || picnum == DTILE_DIPSWITCHON ||
			picnum == DTILE_ALIENSWITCH || picnum == DTILE_ALIENSWITCHON ||
			picnum == DTILE_TECHSWITCH || picnum == DTILE_TECHSWITCHON)
		{
			if (picnum == DTILE_ALIENSWITCH || picnum == DTILE_ALIENSWITCHON)
			{
				if (act)
					S_PlaySound3D(ALIEN_SWITCH1, act, v);
				else S_PlaySound3D(ALIEN_SWITCH1, ps[snum].GetActor(), v);
			}
			else
			{
				if (act)
					S_PlaySound3D(SWITCH_ON, act, v);
				else S_PlaySound3D(SWITCH_ON, ps[snum].GetActor(), v);
			}
			if (numdips != correctdips) break;
			S_PlaySound3D(END_OF_LEVEL_WARN, ps[snum].GetActor(), v);
		}
		[[fallthrough]];
	case DTILE_DIPSWITCH2:
	case DTILE_DIPSWITCH2ON:
	case DTILE_DIPSWITCH3:
	case DTILE_DIPSWITCH3ON:
	case DTILE_MULTISWITCH:
	case DTILE_MULTISWITCH_2:
	case DTILE_MULTISWITCH_3:
	case DTILE_MULTISWITCH_4:
	case DTILE_ACCESSSWITCH:
	case DTILE_ACCESSSWITCH2:
	case DTILE_SLOTDOOR:
	case DTILE_SLOTDOORON:
	case DTILE_LIGHTSWITCH:
	case DTILE_LIGHTSWITCHON:
	case DTILE_SPACELIGHTSWITCH:
	case DTILE_SPACELIGHTSWITCHON:
	case DTILE_SPACEDOORSWITCH:
	case DTILE_SPACEDOORSWITCHON:
	case DTILE_FRANKENSTINESWITCH:
	case DTILE_FRANKENSTINESWITCHON:
	case DTILE_LIGHTSWITCH2:
	case DTILE_LIGHTSWITCH2ON:
	case DTILE_POWERSWITCH1:
	case DTILE_POWERSWITCH1ON:
	case DTILE_LOCKSWITCH1:
	case DTILE_LOCKSWITCH1ON:
	case DTILE_POWERSWITCH2:
	case DTILE_POWERSWITCH2ON:
	case DTILE_HANDSWITCH:
	case DTILE_HANDSWITCHON:
	case DTILE_PULLSWITCH:
	case DTILE_PULLSWITCHON:

		if (picnum == DTILE_MULTISWITCH || picnum == (DTILE_MULTISWITCH_2) ||
			picnum == (DTILE_MULTISWITCH_3) || picnum == (DTILE_MULTISWITCH_4))
			lotag += picnum - DTILE_MULTISWITCH;

		DukeStatIterator itr(STAT_EFFECTOR);
		while (auto other = itr.Next())
		{
			if (other->spr.hitag == lotag)
			{
				switch (other->spr.lotag)
				{
				case SE_12_LIGHT_SWITCH:
					other->sector()->floorpal = 0;
					other->temp_data[0]++;
					if (other->temp_data[0] == 2)
						other->temp_data[0]++;

					break;
				case SE_24_CONVEYOR:
				case SE_34:
				case SE_25_PISTON:
					other->temp_data[4] = !other->temp_data[4];
					if (other->temp_data[4])
						FTA(15, &ps[snum]);
					else FTA(2, &ps[snum]);
					break;
				case SE_21_DROP_FLOOR:
					FTA(2, &ps[screenpeek]);
					break;
				}
			}
		}

		operateactivators(lotag, &ps[snum]);
		fi.operateforcefields(ps[snum].GetActor(), lotag);
		operatemasterswitches(lotag);

		if (picnum == DTILE_DIPSWITCH || picnum == DTILE_DIPSWITCHON ||
			picnum == DTILE_ALIENSWITCH || picnum == DTILE_ALIENSWITCHON ||
			picnum == DTILE_TECHSWITCH || picnum == DTILE_TECHSWITCHON) return 1;

		if (hitag == 0 && isadoorwall(texid) == 0)
		{
			if (act)
				S_PlaySound3D(SWITCH_ON, act, v);
			else S_PlaySound3D(SWITCH_ON, ps[snum].GetActor(), v);
		}
		else if (hitag != 0)
		{
			auto flags = S_GetUserFlags(hitag);

			if (act && (flags & SF_TALK) == 0)
				S_PlaySound3D(hitag, act, v);
			else
				S_PlayActorSound(hitag, ps[snum].GetActor());
		}

		return 1;
	}
	return 0;
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void activatebysector_d(sectortype* sect, DDukeActor* activator)
{
	int didit = 0;

	DukeSectIterator it(sect);
	while (auto act = it.Next())
	{
		if (isactivator(act))
		{
			operateactivators(act->spr.lotag, nullptr);
			didit = 1;
			//			return;
		}
	}

	if (didit == 0)
		operatesectors(sect, activator);
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void checkplayerhurt_d(player_struct* p, const Collision& coll)
{
	if (coll.type == kHitSprite)
	{
		CallOnHurt(coll.actor(), p);
		return;
	}

	if (coll.type != kHitWall) return;
	auto wal = coll.hitWall;

	if (p->hurt_delay > 0) p->hurt_delay--;
	else if (wal->cstat & (CSTAT_WALL_BLOCK | CSTAT_WALL_ALIGN_BOTTOM | CSTAT_WALL_MASKED | CSTAT_WALL_BLOCK_HITSCAN)) switch (wal->overpicnum)
	{
	case DTILE_W_FORCEFIELD:
	case DTILE_W_FORCEFIELD + 1:
	case DTILE_W_FORCEFIELD + 2:
		p->GetActor()->spr.extra -= 5;

		p->hurt_delay = 16;
		SetPlayerPal(p, PalEntry(32, 32, 0, 0));

		p->vel.XY() = -p->GetActor()->spr.Angles.Yaw.ToVector() * 16;
		S_PlayActorSound(DUKE_LONGTERM_PAIN, p->GetActor());

		checkhitwall(p->GetActor(), wal, p->GetActor()->getPosWithOffsetZ() + p->GetActor()->spr.Angles.Yaw.ToVector() * 2);
		break;

	case DTILE_BIGFORCE:
		p->hurt_delay = 26;
		checkhitwall(p->GetActor(), wal, p->GetActor()->getPosWithOffsetZ() + p->GetActor()->spr.Angles.Yaw.ToVector() * 2);
		break;

	}
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void checkhitdefault_d(DDukeActor* targ, DDukeActor* proj)
{
	if ((targ->spr.cstat & CSTAT_SPRITE_ALIGNMENT_WALL) && targ->spr.hitag == 0 && targ->spr.lotag == 0 && targ->spr.statnum == STAT_DEFAULT)
		return;

	if ((proj->spr.picnum == DTILE_FREEZEBLAST || proj->GetOwner() != targ) && targ->spr.statnum != STAT_PROJECTILE)
	{
		if (badguy(targ) == 1)
		{
			if (isWorldTour() && targ->spr.picnum == DTILE_FIREFLY && targ->spr.scale.X < 0.75)
				return;

			if (proj->spr.picnum == DTILE_RPG) proj->spr.extra <<= 1;

			if ((targ->spr.picnum != DTILE_DRONE) && (targ->spr.picnum != DTILE_ROTATEGUN) && (targ->spr.picnum != DTILE_COMMANDER) && targ->spr.picnum != DTILE_GREENSLIME)
				if (proj->spr.picnum != DTILE_FREEZEBLAST)
					//if (actortype[targ->spr.picnum] == 0) //TRANSITIONAL.
				{
					auto spawned = spawn(proj, DTILE_JIBS6);
					if (spawned)
					{
						if (proj->spr.pal == 6)
							spawned->spr.pal = 6;
						spawned->spr.pos.Z += 4;
						spawned->vel.X = 1;
						spawned->spr.scale = DVector2(0.375, 0.375);
						spawned->spr.Angles.Yaw = DAngle22_5 / 4 - randomAngle(22.5 / 2);
					}
				}

			auto Owner = proj->GetOwner();

			if (Owner && Owner->isPlayer() && targ->spr.picnum != DTILE_ROTATEGUN && targ->spr.picnum != DTILE_DRONE)
				if (ps[Owner->PlayerIndex()].curr_weapon == SHOTGUN_WEAPON)
				{
					fi.shoot(targ, -1, PClass::FindActor("DukeBloodSplat3"));
					fi.shoot(targ, -1, PClass::FindActor("DukeBloodSplat1"));
					fi.shoot(targ, -1, PClass::FindActor("DukeBloodSplat2"));
					fi.shoot(targ, -1, PClass::FindActor("DukeBloodSplat4"));
				}

			if (!actorflag(targ, SFLAG2_NODAMAGEPUSH) && !bossguy(targ)) // RR does not have this.
			{
				if ((targ->spr.cstat & CSTAT_SPRITE_ALIGNMENT_MASK) == 0)
					targ->spr.Angles.Yaw = proj->spr.Angles.Yaw + DAngle180;

				targ->vel.X = -proj->spr.extra * 0.25;
				auto sp = targ->sector();
				pushmove(targ->spr.pos, &sp, 8, 4, 4, CLIPMASK0);
				if (sp != targ->sector() && sp != nullptr)
					ChangeActorSect(targ, sp);
			}

			if (targ->spr.statnum == STAT_ZOMBIEACTOR)
			{
				ChangeActorStat(targ, STAT_ACTOR);
				targ->timetosleep = SLEEPTIME;
			}
			if ((targ->spr.scale.X < 0.375 || targ->spr.picnum == DTILE_SHARK) && proj->spr.picnum == DTILE_SHRINKSPARK) return;
		}

		if (targ->spr.statnum != STAT_ZOMBIEACTOR)
		{
			if (proj->spr.picnum == DTILE_FREEZEBLAST && ((targ->isPlayer() && targ->spr.pal == 1) || (gs.freezerhurtowner == 0 && proj->GetOwner() == targ)))
				return;

			int hitpic = proj->spr.picnum;
			auto Owner = proj->GetOwner();
			if (Owner && Owner->isPlayer())
			{
				if (targ->isPlayer() && ud.coop != 0 && ud.ffire == 0)
					return;

				auto tOwner = targ->GetOwner();
				if (isWorldTour() && hitpic == DTILE_FIREBALL && tOwner && tOwner->spr.picnum != DTILE_FIREBALL)
					hitpic = DTILE_FLAMETHROWERFLAME;
			}

			targ->attackertype = hitpic;
			targ->hitextra += proj->spr.extra;
			targ->hitang = proj->spr.Angles.Yaw;
			targ->SetHitOwner(Owner);
		}

		if (targ->spr.statnum == STAT_PLAYER)
		{
			auto p = targ->spr.yint;
			if (ps[p].newOwner != nullptr)
			{
				ps[p].newOwner = nullptr;
				ps[p].GetActor()->restoreloc();

				updatesector(ps[p].GetActor()->getPosWithOffsetZ(), &ps[p].cursector);

				DukeStatIterator it(STAT_ACTOR);
				while (auto itActor = it.Next())
				{
					if (actorflag(itActor, SFLAG2_CAMERA)) itActor->spr.yint = 0;
				}
			}

			if (targ->spr.scale.X < 0.375 && proj->spr.picnum == DTILE_SHRINKSPARK)
				return;

			auto hitowner = targ->GetHitOwner();
			if (!hitowner || !hitowner->isPlayer())
				if (ud.player_skill >= 3)
					proj->spr.extra += (proj->spr.extra >> 1);
		}

	}
}

void checkhitsprite_d(DDukeActor* targ, DDukeActor* proj)
{
	if (targ->GetClass() != RUNTIME_CLASS(DDukeActor))
	{
		CallOnHit(targ, proj);
		return;
	}


	if (targ->spr.picnum == DTILE_PLAYERONWATER)
	{
		targ = targ->GetOwner();
		if (!targ) return;
	}
	checkhitdefault_d(targ, proj);
}

//---------------------------------------------------------------------------
//
// taken out of checksectors to eliminate some gotos.
//
//---------------------------------------------------------------------------

void clearcameras(player_struct* p)
{
	p->GetActor()->restorepos();
	p->newOwner = nullptr;

	updatesector(p->GetActor()->getPosWithOffsetZ(), &p->cursector);

	DukeStatIterator it(STAT_ACTOR);
	while (auto act = it.Next())
	{
		if (actorflag(act, SFLAG2_CAMERA)) act->spr.yint = 0;
	}
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void checksectors_d(int snum)
{
	int i = -1;
	player_struct* p;
	walltype* hitscanwall;
	HitInfo near;

	p = &ps[snum];
	auto pact = p->GetActor();

	if (!p->insector()) return;

	switch (p->cursector->lotag)
	{

	case 32767:
		p->cursector->lotag = 0;
		FTA(9, p);
		p->secret_rooms++;
		SECRET_Trigger(sectindex(p->cursector));
		return;
	case -1:
		p->cursector->lotag = 0;
		setnextmap(false);
		return;
	case -2:
		p->cursector->lotag = 0;
		p->timebeforeexit = 26 * 8;
		p->customexitsound = p->cursector->hitag;
		return;
	default:
		if (p->cursector->lotag >= 10000 && p->cursector->lotag < 16383)
		{
			if (snum == screenpeek || ud.coop == 1)
				S_PlayActorSound(p->cursector->lotag - 10000, pact);
			p->cursector->lotag = 0;
		}
		break;

	}

	//After this point the the player effects the map with space

	if (chatmodeon || p->GetActor()->spr.extra <= 0) return;

	if (ud.cashman && PlayerInput(snum, SB_OPEN))
		fi.lotsofmoney(p->GetActor(), 2);

	if (p->newOwner != nullptr)
	{
		if (abs(PlayerInputSideVel(snum)) > 0.75 || abs(PlayerInputForwardVel(snum)) > 0.75)
		{
			clearcameras(p);
			return;
		}
		else if (PlayerInput(snum, SB_ESCAPE))
		{
			clearcameras(p);
			return;
		}
	}

	if (!(PlayerInput(snum, SB_OPEN)))
		p->toggle_key_flag = 0;

	else if (!p->toggle_key_flag)
	{
		near.hitActor = nullptr;
		p->toggle_key_flag = 1;
		hitscanwall = nullptr;

		double dist = hitawall(p, &hitscanwall);

		if (hitscanwall != nullptr)
		{
			if (dist < 80 && hitscanwall->overtexture() == mirrortex)
				if (hitscanwall->lotag > 0 && S_CheckSoundPlaying(hitscanwall->lotag) == 0 && snum == screenpeek)
				{
					S_PlayActorSound(hitscanwall->lotag, pact);
					return;
				}

			if (hitscanwall != nullptr && (hitscanwall->cstat & CSTAT_WALL_MASKED))
				if (hitscanwall->lotag)
					return;
		}
		if (p->newOwner != nullptr)
			neartag(p->GetActor()->getPrevPosWithOffsetZ(), p->GetActor()->sector(), p->GetActor()->PrevAngles.Yaw, near, 80., NT_Lotag);
		else
		{
			neartag(p->GetActor()->getPosWithOffsetZ(), p->GetActor()->sector(), p->GetActor()->PrevAngles.Yaw, near, 80., NT_Lotag);
			if (near.actor() == nullptr && near.hitWall == nullptr && near.hitSector == nullptr)
				neartag(p->GetActor()->getPosWithOffsetZ().plusZ(8), p->GetActor()->sector(), p->GetActor()->PrevAngles.Yaw, near, 80., NT_Lotag);
			if (near.actor() == nullptr && near.hitWall == nullptr && near.hitSector == nullptr)
				neartag(p->GetActor()->getPosWithOffsetZ().plusZ(16), p->GetActor()->sector(), p->GetActor()->PrevAngles.Yaw, near, 80., NT_Lotag);
			if (near.actor() == nullptr && near.hitWall == nullptr && near.hitSector == nullptr)
			{
				neartag(p->GetActor()->getPosWithOffsetZ().plusZ(16), p->GetActor()->sector(), p->GetActor()->PrevAngles.Yaw, near, 80., NT_Lotag | NT_Hitag);
				if (near.actor() != nullptr)
				{
					if (actorflag(near.actor(), SFLAG2_TRIGGERRESPAWN))
						return;
				}

				near.clearObj();
			}
		}

		if (p->newOwner == nullptr && near.actor() == nullptr && near.hitWall == nullptr && near.hitSector == nullptr)
			if (isanunderoperator(p->GetActor()->sector()->lotag))
				near.hitSector = p->GetActor()->sector();

		if (near.hitSector && (near.hitSector->lotag & 16384))
			return;

		if (near.actor() == nullptr && near.hitWall == nullptr)
			if (p->cursector->lotag == 2)
			{
				DDukeActor* hit;
				dist = hitasprite(p->GetActor(), &hit);
				if (hit) near.hitActor = hit;
				if (dist > 80) near.hitActor = nullptr;

			}

		auto const neartagsprite = near.actor();
		if (neartagsprite != nullptr)
		{
			if (fi.checkhitswitch(snum, nullptr, neartagsprite)) return;

			if (CallOnUse(neartagsprite, p))
				return;
		}

		if (!PlayerInput(snum, SB_OPEN)) return;
		else if (p->newOwner != nullptr)
		{
			clearcameras(p);
			return;
		}

		if (near.hitWall == nullptr && near.hitSector == nullptr && near.actor() == nullptr)
			if (hits(p->GetActor()) < 32)
			{
				if ((krand() & 255) < 16)
					S_PlayActorSound(DUKE_SEARCH2, pact);
				else S_PlayActorSound(DUKE_SEARCH, pact);
				return;
			}

		if (near.hitWall)
		{
			if (near.hitWall->lotag > 0 && isadoorwall(near.hitWall->walltexture()))
			{
				if (hitscanwall == near.hitWall || hitscanwall == nullptr)
					fi.checkhitswitch(snum, near.hitWall, nullptr);
				return;
			}
			else if (p->newOwner != nullptr)
			{
				clearcameras(p);
				return;
			}
		}

		if (near.hitSector && (near.hitSector->lotag & 16384) == 0 && isanearoperator(near.hitSector->lotag))
		{
			DukeSectIterator it(near.hitSector);
			while (auto act = it.Next())
			{
				if (isactivator(act) || ismasterswitch(act))
					return;
			}
			operatesectors(near.hitSector, p->GetActor());
		}
		else if ((p->GetActor()->sector()->lotag & 16384) == 0)
		{
			if (isanunderoperator(p->GetActor()->sector()->lotag))
			{
				DukeSectIterator it(p->GetActor()->sector());
				while (auto act = it.Next())
				{
					if (isactivator(act) || ismasterswitch(act)) return;
				}
				operatesectors(p->GetActor()->sector(), p->GetActor());
			}
			else fi.checkhitswitch(snum, near.hitWall, nullptr);
		}
	}
}





END_DUKE_NS
