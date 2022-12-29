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
#include "texturemanager.h"

BEGIN_DUKE_NS


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

int ifsquished(DDukeActor* actor, int p)
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
			actor->attackertype = DukeShotSparkClass;
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

int ifhitbyweapon_d(DDukeActor *actor)
{
	int p;
	auto hitowner = actor->GetHitOwner();

	if (actor->hitextra >= 0)
	{
		auto adef = actor->attackerDefaults();
		if (actor->spr.extra >= 0)
		{
			if (actor->isPlayer())
			{
				if (ud.god && !(adef->flags3 & SFLAG3_LIGHTDAMAGE)) return -1;

				p = actor->PlayerIndex();

				if (hitowner &&
					hitowner->isPlayer() &&
					ud.coop == 1 &&
					ud.ffire == 0)
					return -1;

				actor->spr.extra -= actor->hitextra;

				if (hitowner)
				{
					if (actor->spr.extra <= 0 && !(adef->flags2 & SFLAG2_FREEZEDAMAGE))
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

				if (adef->flags2 & SFLAG2_DOUBLEDMGTHRUST)
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
					if (actor->attackertype == DukeShrinkSparkClass && actor->spr.scale.X < 0.375)
						return -1;

				if (actor->attackertype == DukeFireflyClass && actor->spr.scale.X < 0.75)
				{
					return -1;
				}

				actor->spr.extra -= actor->hitextra;
				auto Owner = actor->GetOwner();
				if (!(actor->flags2 & SFLAG2_IGNOREHITOWNER) && Owner && Owner->spr.statnum < MAXSTATUS)
					actor->SetOwner(hitowner);
			}

			actor->hitextra = -1;
			// makeshift damage type reporting. Needs improvement and generalization later.
			int res = 0;
			if (adef->flags2 & SFLAG2_FREEZEDAMAGE) res |= 1;
			if (adef->flags2 & SFLAG2_EXPLOSIVE) res |= 2;
			return res;
		}
	}


	if (ud.multimode < 2
		|| actor->attackertype == nullptr
		|| actor->attackertype != DukeFlamethrowerFlameClass
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

		return 0;
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

		if (act->counter > 0) act->counter--;

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
								spawn(act, DukeTransporterBeamClass);
								S_PlayActorSound(TELEPORTER, act);
							}

							for (int k = connecthead; k >= 0; k = connectpoint2[k])
							if (ps[k].cursector == Owner->sector())
							{
								ps[k].frag_ps = p;
								ps[k].GetActor()->spr.extra = 0;
							}

							ps[p].GetActor()->PrevAngles.Yaw = ps[p].GetActor()->spr.Angles.Yaw = Owner->spr.Angles.Yaw;

							if (Owner->GetOwner() != Owner)
							{
								act->counter = 13;
								Owner->counter = 13;
								ps[p].transporter_hold = 13;
							}

							ps[p].GetActor()->spr.pos = Owner->spr.pos;
							ps[p].GetActor()->backuppos();
							ps[p].setbobpos();

							ChangeActorSect(act2, Owner->sector());
							ps[p].setCursector(act2->sector());

							if (act->spr.pal == 0)
							{
								auto k = spawn(Owner, DukeTransporterBeamClass);
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
							spawn(act2, DukeWaterSplashClass);

						if (sectlotag == 1)
							for (int l = 0; l < 9; l++)
						{
							auto q = spawn(ps[p].GetActor(), DukeWaterBubbleClass);
							if (q) q->spr.pos.Z += krandf(64);
						}
					}
				}
				break;

			case STAT_ACTOR:
				if ((act->flags3 & SFLAG3_DONTDIVEALIVE) && act2->spr.extra > 0) continue;
				[[fallthrough]];
			case STAT_PROJECTILE:
			case STAT_MISC:
			case STAT_FALLER:
			case STAT_DUMMYPLAYER:
				if ((act->flags2 & SFLAG2_DONTDIVE)) continue;

				ll = abs(act2->vel.Z);

				{
					warpspriteto = 0;
					if (ll && sectlotag == 2 && act2->spr.pos.Z < (sectp->ceilingz + ll) && act2->vel.Z < 0)
						warpspriteto = 1;

					if (ll && sectlotag == 1 && act2->spr.pos.Z > (sectp->floorz - ll) && act2->vel.Z > 0)
						warpspriteto = 1;

					if (sectlotag == 0 && (onfloorz || abs(act2->spr.pos.Z - act->spr.pos.Z) < 16))
					{
						if ((!Owner || Owner->GetOwner() != Owner) && onfloorz && act->counter > 0 && act2->spr.statnum != STAT_MISC)
						{
							act->counter++;
							goto BOLT;
						}
						warpspriteto = 1;
					}

					if (warpspriteto)
					{
						if ((act2->flags1 & SFLAG_NOTELEPORT)) continue;
						switch (act2->spr.picnum)
						{
						case DTILE_PLAYERONWATER:
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

						case DTILE_WATERBUBBLE:

							if (sectlotag > 0)
							{
								auto k = spawn(act2, DukeWaterSplashClass);
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
											auto k = spawn(act, DukeTransporterBeamClass);
											if (k) S_PlayActorSound(TELEPORTER, k);

											k = spawn(Owner, DukeTransporterBeamClass);
											if (k) S_PlayActorSound(TELEPORTER, k);
										}

										if (Owner && Owner->GetOwner() == Owner)
										{
											act->counter = 13;
											Owner->counter = 13;
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
		if ((act2->spr.lotag == SE_14_SUBWAY_CAR) && (sh == act2->spr.hitag) && (act2->counter == actor->counter))
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
	handle_se14(actor, true, DukeRPGClass);
}


//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

static void handle_se28(DDukeActor* actor)
{
	if (actor->temp_data[0] > 0)
	{
		actor->temp_data[0]--;
		return;
	}

	if (actor->counter == 0)
	{
		double x;
		findplayer(actor, &x);
		if (x > 15500 / 16.)
			return;
		actor->counter = 1;
		actor->temp_data[1] = 64 + (krand() & 511);
		actor->temp_data[2] = 0;
	}
	else
	{
		actor->temp_data[2]++;
		if (actor->temp_data[2] > actor->temp_data[1])
		{
			actor->counter = 0;
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
				if (act2->GetClass() == DukeNaturalLightningClass && act2->spr.hitag == actor->spr.hitag)
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
				if (act2->GetClass() == DukeNaturalLightningClass && act2->spr.hitag == actor->spr.hitag)
				{
					if (rnd(32) && (actor->temp_data[2] & 1))
					{
						act2->spr.cstat &= ~CSTAT_SPRITE_INVISIBLE;
						spawn(act2, DukeSmallSmokeClass);

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
			handle_se14(act, true, DukeRPGClass);
			break;

		case SE_30_TWO_WAY_TRAIN:
			handle_se30(act);
			break;

		case SE_2_EARTHQUAKE:
			handle_se02(act);
			break;

			//Flashing sector lights after reactor DTILE_EXPLOSION2
		case SE_3_RANDOM_LIGHTS_AFTER_SHOT_OUT:
			handle_se03(act);
			break;

		case SE_4_RANDOM_LIGHTS:
			handle_se04(act);
			break;

			//BOSS
		case SE_5_BOSS:
			handle_se05(act);
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
			handle_se16(act);
			break;

		case SE_17_WARP_ELEVATOR:
			handle_se17(act);
			break;

		case SE_18_INCREMENTAL_SECTOR_RISE_FALL:
			handle_se18(act, true);
			break;

		case SE_19_EXPLOSION_LOWERS_CEILING:
			handle_se19(act);
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
			handle_se35(act);
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
			if (ud.earthquaketime > 0 && (krand() & 7) == 0)
				RANDOMSCRAP(act);
			break;
		case SE_36_PROJ_SHOOTER:
			handle_se36(act);
			break;

		case SE_128_GLASS_BREAKING:
			handle_se128(act);
			break;

		case 130:
			handle_se130(act, 80);
			break;
		case 131:
			handle_se130(act, 40);
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

	actor->counter++;

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
		actor->spr.Angles.Yaw += DAngle45 * BobVal(actor->counter << 3);

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
		if (actor->counter < 16)
			actor->vel.Z -= BobVal(512 + (actor->counter << 4)) * 2;
	}

	if (a & face_player_smart)
	{
		DVector2 newpos = ps[playernum].GetActor()->spr.pos.XY() + (ps[playernum].vel.XY() * (4. / 3.));
		goalang = (newpos - actor->spr.pos.XY()).Angle();
		angdif = deltaangle(actor->spr.Angles.Yaw, goalang) * 0.25;
		if (angdif > -DAngle22_5/16 && angdif < nullAngle) angdif = nullAngle;
		actor->spr.Angles.Yaw += angdif;
	}

	if (actor->curMove->name == NAME_None || a == 0)
	{
		if ((badguy(actor) && actor->spr.extra <= 0) || (actor->opos.X != actor->spr.pos.X) || (actor->opos.Y != actor->spr.pos.Y))
		{
			if (!actor->isPlayer()) actor->backupvec2();
			SetActor(actor, actor->spr.pos);
		}
		return;
	}

	if (a & geth) actor->vel.X += (actor->curMove->movex - actor->vel.X) * 0.5;
	if (a & getv) actor->vel.Z += (actor->curMove->movez - actor->vel.Z) * 0.5;

	if (a & dodgebullet)
		dodge(actor);

	if (!actor->isPlayer())
		alterang(a, actor, playernum);

	if (abs(actor->vel.X) < 6 / 16.) actor->vel.X = 0;

	a = badguy(actor);

	if (actor->vel.X != 0 || actor->vel.Z != 0)
	{
		if (a && actor->spr.picnum != DTILE_ROTATEGUN)
		{
			if ((actor->spr.picnum == DTILE_DRONE || actor->spr.picnum == DTILE_COMMANDER) && actor->spr.extra > 0)
			{
				if (actor->spr.picnum == DTILE_COMMANDER)
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
			else if (actor->spr.picnum != DTILE_ORGANTIC)
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

		if (a && actor->spr.picnum != DTILE_ROTATEGUN)
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
			else if (!(actor->flags2 & SFLAG2_FLOATING))
			{
				if (actor->curMove->movez == 0)
				{
					if (actor->opos.Z != actor->spr.pos.Z || (ud.multimode < 2 && ud.player_skill < 2))
					{
						if ((actor->counter & 1) || ps[playernum].actorsqu == actor) return;
						else daxvel *= 2;
					}
					else
					{
						if ((actor->counter & 3) || ps[playernum].actorsqu == actor) return;
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

		if (actor->sector()->floortexture == mirrortex)
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
	fall_common(actor, g_p, nullptr);
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
	tickstat(STAT_PROJECTILE);		//ST 4
	moveplayers();			//ST 10
	movefallers();		//ST 12
	tickstat(STAT_MISC, true);		//ST 5

	actortime.Reset();
	actortime.Clock();
	tickstat(STAT_ACTOR, true);			//ST 1
	actortime.Unclock();

	moveeffectors_d();		//ST 3
	tickstat(STAT_STANDABLE);		//ST 6
	doanimations();
	tickstat(STAT_FX);				//ST 11

	if (numplayers < 2 && thunderon)
		thunder();

	thinktime.Unclock();
}


END_DUKE_NS
