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
#include "mapinfo.h"
#include "dukeactor.h"

BEGIN_DUKE_NS

void dojaildoor();
void moveminecart();

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

void lotsoffeathers_r(DDukeActor *actor, int n)
{
	lotsofstuff(actor, n, RedneckFeatherClass);
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
		auto adef = actor->attackerDefaults();
		if (actor->spr.extra >= 0)
		{
			if (actor->isPlayer())
			{
				if (ud.god) return -1;

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

	actor->hitextra = -1;
	return -1;
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void movetransports_r(void)
{
	uint8_t warpdir = 0;

	 //Transporters

	DukeStatIterator iti(STAT_TRANSPORT);
	while (auto act = iti.Next())
	{
		auto Owner = act->GetOwner();
		if (Owner == act || Owner == nullptr)
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
						if (ps[p].on_ground && sectlotag == ST_0_NO_EFFECT && onfloorz && ps[p].jetpack_on == 0)
						{
							spawn(act, DukeTransporterBeamClass);
							S_PlayActorSound(TELEPORTER, act);

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

							ps[p].GetActor()->spr.pos = Owner->spr.pos.plusZ(4);
							ps[p].GetActor()->backuppos();
							ps[p].setbobpos();

							ChangeActorSect(act2, Owner->sector());
							ps[p].setCursector(act2->sector());

							auto beam = spawn(Owner, DukeTransporterBeamClass);
							if (beam) S_PlayActorSound(TELEPORTER, beam);

							break;
						}
					}
					else break;

					if (onfloorz == 0 && fabs(act->spr.pos.Z - ps[p].GetActor()->getOffsetZ()) < 24)
						if ((ps[p].jetpack_on == 0) || (ps[p].jetpack_on && (PlayerInput(p, SB_JUMP) || ps[p].sync.uvel > 0)) ||
							(ps[p].jetpack_on && (PlayerInput(p, SB_CROUCH) || ps[p].sync.uvel < 0)))
						{
							ps[p].GetActor()->spr.pos.XY() += Owner->spr.pos.XY() - act->spr.pos.XY();
							ps[p].GetActor()->backupvec2();

							if (ps[p].jetpack_on && (PlayerInput(p, SB_JUMP) || ps[p].jetpack_on < 11))
								ps[p].GetActor()->spr.pos.Z = Owner->spr.pos.Z - 24 + gs.playerheight;
							else ps[p].GetActor()->spr.pos.Z = Owner->spr.pos.Z + 24 + gs.playerheight;
							ps[p].GetActor()->backupz();

							ChangeActorSect(act2, Owner->sector());
							ps[p].setCursector(Owner->sector());

							break;
						}

					int k = 0;

					if (ud.mapflags & MFLAG_ALLSECTORTYPES)
					{
						if (onfloorz && sectlotag == ST_160_FLOOR_TELEPORT && ps[p].GetActor()->getOffsetZ() > sectp->floorz - 48)
						{
							k = 2;
							ps[p].GetActor()->spr.pos.Z = Owner->sector()->ceilingz + 7 + gs.playerheight;
							ps[p].GetActor()->backupz();
						}

						if (onfloorz && sectlotag == ST_161_CEILING_TELEPORT && ps[p].GetActor()->getOffsetZ() < sectp->ceilingz + 6)
						{
							k = 2;
							if (ps[p].GetActor()->spr.extra <= 0) break;
							ps[p].GetActor()->spr.pos.Z = Owner->sector()->floorz - 49 + gs.playerheight;
							ps[p].GetActor()->backupz();
						}
					}

					if ((onfloorz && sectlotag == ST_1_ABOVE_WATER && ps[p].GetActor()->getOffsetZ() > sectp->floorz - 6) ||
						(onfloorz && sectlotag == ST_1_ABOVE_WATER && ps[p].OnMotorcycle))
					{
						if (ps[p].OnBoat) break;
						k = 1;
						if (screenpeek == p)
						{
							FX_StopAllSounds();
						}
						S_PlayActorSound(DUKE_UNDERWATER, ps[p].GetActor());
						ps[p].GetActor()->spr.pos.Z = Owner->sector()->ceilingz + 7 + gs.playerheight;
						ps[p].GetActor()->backupz();
						if (ps[p].OnMotorcycle)
							ps[p].moto_underwater = 1;
					}

					if (onfloorz && sectlotag == ST_2_UNDERWATER && ps[p].GetActor()->getOffsetZ() < sectp->ceilingz + 6)
					{
						k = 1;
						if (ps[p].GetActor()->spr.extra <= 0) break;
						if (screenpeek == p)
						{
							FX_StopAllSounds();
						}
						S_PlayActorSound(DUKE_GASP, act2);

						ps[p].GetActor()->spr.pos.Z = Owner->sector()->floorz - 7 + gs.playerheight;
						ps[p].GetActor()->backupz();
					}

					if (k == 1)
					{
						ps[p].GetActor()->spr.pos.XY() += Owner->spr.pos.XY() - act->spr.pos.XY();
						ps[p].GetActor()->backupvec2();

						if (!Owner || Owner->GetOwner() != Owner)
							ps[p].transporter_hold = -2;
						ps[p].setCursector(Owner->sector());

						ChangeActorSect(act2, Owner->sector());

						if ((krand() & 255) < 32)
							spawn(act2, DukeWaterSplashClass);
					}
					else if (k == 2)
					{
						ps[p].GetActor()->spr.pos.XY() += Owner->spr.pos.XY() - act->spr.pos.XY();
						ps[p].GetActor()->backupvec2();

						if (Owner->GetOwner() != Owner)
							ps[p].transporter_hold = -2;
						ps[p].setCursector(Owner->sector());

						ChangeActorSect(act2, Owner->sector());
					}
				}
				break;

			case STAT_ACTOR:
				if ((act2->flags3 & SFLAG3_DONTDIVEALIVE) && act2->spr.extra > 0) continue;
				[[fallthrough]];
			case STAT_PROJECTILE:
			case STAT_MISC:
			case STAT_DUMMYPLAYER:
				if ((act2->flags2 & SFLAG2_DONTDIVE)) continue;
				checkdive(act, act2);
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
	DukeStatIterator it(STAT_BOBBING);
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
			if (badguy(act))
			{
				if (enemysizecheat == 3)
				{
					act->spr.scale *= 2;
					act->setClipDistFromTile();
				}
				else if (enemysizecheat == 2)
				{
					act->spr.scale *= 0.5;
					auto tex = TexMan.GetGameTexture(act->spr.spritetexture());
					act->setClipDistFromTile();
				}
				break;
			}
		}
		enemysizecheat = 0;
	}

	tickstat(STAT_RABBITSPAWN);
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void moveactors_r(void)
{
	dojaildoor();
	moveminecart();

	if (isRRRA())
	{
		rrra_specialstats();
	}
	tickstat(STAT_LUMBERMILL);
	if (ud.chickenplant) tickstat(STAT_CHICKENPLANT);
	tickstat(STAT_BOWLING);
	tickstat(STAT_TELEPORT);
	tickstat(STAT_ACTOR, true);
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
				auto ns = spawn(actor, RedneckHulkClass);
				if (ns)
				{
					ns->spr.pos.Z = ns->sector()->ceilingz;
					ns->spr.pal = 33;
				}
				if (!hulkspawn)
				{
					ns = CreateActor(actor->sector(), DVector3(actor->spr.pos.XY(), actor->sector()->ceilingz + 466.5), RedneckUfoLightClass, -8, DVector2(0.25, 0.25), nullAngle, 0., 0., actor, 5);
					if (ns)
					{
						ns->spr.cstat = CSTAT_SPRITE_TRANS_FLIP | CSTAT_SPRITE_TRANSLUCENT;
						ns->spr.pal = 7;
						ns->spr.scale = DVector2(1.25, 3.984375);
					}
					ns = spawn(actor, RedneckTeleportClass);
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
			if (a2->GetClass() == RedneckUfoBeamClass && ufospawn && ++ufocnt == 64)
			{
				ufocnt = 0;
				ufospawn--;
				PClassActor* pn;
				if (!isRRRA())
				{
					switch (krand() & 3)
					{
					default:
					case 0:
						pn = RedneckUfo1Class;
						break;
					case 1:
						pn = RedneckUfo2Class;
						break;
					case 2:
						pn = RedneckUfo3Class;
						break;
					case 3:
						pn = RedneckUfo4Class;
						break;
					}
				}
				else pn = RedneckUfoRRRAClass;
				auto ns = spawn(actor, pn);
				if (ns) ns->spr.pos.Z = ns->sector()->ceilingz;
			}
		}
	}

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
				if (act2->spr.extra) x = -x;
				actor->vel.X += x / 16.;
			}
			act2->temp_data[4] = actor->temp_data[4];
		}
	}
	handle_se14(actor, false, RedneckDynamiteArrowClass);
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
			handle_se14(act, false, RedneckDynamiteArrowClass);
			break;

		case SE_30_TWO_WAY_TRAIN:
			handle_se30(act);
			break;


		case SE_2_EARTHQUAKE:
			handle_se02(act);
			break;

			//Flashing sector lights after reactor explosion
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
			handle_se35(act);
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
			if (ud.earthquaketime > 0 && (krand() & 7) == 0)
				RANDOMSCRAP(act);
			break;
		case SE_36_PROJ_SHOOTER:
			handle_se36(act);
			break;

		case SE_128_GLASS_BREAKING:
			handle_se128(act);
			break;

		case SE_130:
			handle_se130(act, 80);
			break;
		case SE_131:
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

void fakebubbaspawn(DDukeActor *actor, player_struct* p)
{
	fakebubba_spawn++;
	switch (fakebubba_spawn)
	{
	default:
		break;
	case 1:
		spawn(actor, RedneckPigClass);
		break;
	case 2:
		spawn(actor, RedneckMinionClass);
		break;
	case 3:
		spawn(actor, RedneckCheerleaderClass);
		break;
	case 4:
		spawn(actor, RedneckVixenClass);
		operateactivators(666, p);
		break;
	}
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
		if (a2->GetClass() == RedneckDestroyTagsClass)
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
				if (a3->GetClass() == RedneckDestructoClass)
				{
					a3->attackertype = DukeShotSparkClass;
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
					destwal->setwalltexture(srcwal->walltexture);
					destwal->setovertexture(srcwal->overtexture);
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
				destsect->setceilingtexture(srcsect->ceilingtexture);
				destsect->ceilingheinum = srcsect->ceilingheinum;
				destsect->ceilingshade = srcsect->ceilingshade;
				destsect->ceilingpal = srcsect->ceilingpal;
				destsect->ceilingxpan_ = srcsect->ceilingxpan_;
				destsect->ceilingypan_ = srcsect->ceilingypan_;
				destsect->setfloortexture(srcsect->floortexture);
				destsect->floorheinum = srcsect->floorheinum;
				destsect->floorshade = srcsect->floorshade;
				destsect->floorpal = srcsect->floorpal;
				destsect->floorxpan_ = srcsect->floorxpan_;
				destsect->floorypan_ = srcsect->floorypan_;
				destsect->visibility = srcsect->visibility;
				destsect->lockinfo = srcsect->lockinfo;
				destsect->lotag = srcsect->lotag;
				destsect->hitag = srcsect->hitag;
				destsect->extra = srcsect->extra;
				destsect->dirty = EDirty::AllDirty;
			}
	}
	it1.Reset(actor->sector());
	while (auto a2 = it1.Next())
	{
		if (!(a2->flags3 & SFLAG3_DESTRUCTOIMMUNE))
			a2->Destroy();
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
		spawn(actor, RedneckRabbitClass);
	}
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void think_r(void)
{
	thinktime.Reset();
	thinktime.Clock();

	movefta();			//ST 2
	tickstat(STAT_PROJECTILE);
	moveplayers();			//ST 10
	movefallers();		//ST 12
	tickstat(STAT_MISC, true);

	actortime.Reset();
	actortime.Clock();
	moveactors_r();			//ST 1
	actortime.Unclock();

	moveeffectors_r();		//ST 3
	tickstat(STAT_STANDABLE);
	doanimations();
	tickstat(STAT_FX);				//ST 11

	if (numplayers < 2 && thunderon)
		thunder();

	thinktime.Unclock();
}


END_DUKE_NS
