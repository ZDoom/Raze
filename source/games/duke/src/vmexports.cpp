BEGIN_DUKE_NS

//---------------------------------------------------------------------------
//
// global exports
//
//---------------------------------------------------------------------------

DEFINE_ACTION_FUNCTION(_Duke, GetViewPlayer)
{
	ACTION_RETURN_POINTER(&ps[screenpeek]);
}

DEFINE_ACTION_FUNCTION(_Duke, MaxPlayerHealth)
{
	ACTION_RETURN_INT(gs.max_player_health);
}

DEFINE_ACTION_FUNCTION(_Duke, MaxAmmoAmount)
{
	PARAM_PROLOGUE;
	PARAM_INT(weap);
	int max = weap < 0 || weap >= MAX_WEAPONS ? 0 : gs.max_ammo_amount[weap];
	ACTION_RETURN_INT(max);
}

void S_PlaySpecialMusic(unsigned int m);

DEFINE_ACTION_FUNCTION_NATIVE(_Duke, PlaySpecialMusic, S_PlaySpecialMusic)
{
	PARAM_PROLOGUE;
	PARAM_INT(song);
	S_PlaySpecialMusic(song);
	return 0;
}

static int PlaySound(int num, int chan, int flags, double vol)
{
	return S_PlaySound(num, chan, EChanFlags::FromInt(flags), float(vol));
}

DEFINE_ACTION_FUNCTION_NATIVE(_Duke, PlaySound, PlaySound)
{
	PARAM_PROLOGUE;
	PARAM_INT(snd);
	PARAM_INT(chan);
	PARAM_INT(flags);
	PARAM_FLOAT(vol);
	ACTION_RETURN_INT(PlaySound(snd, chan, flags, vol));
}
static void StopSound(int num)
{
	S_StopSound(num);
}


DEFINE_ACTION_FUNCTION_NATIVE(_Duke, StopSound, StopSound)
{
	PARAM_PROLOGUE;
	PARAM_INT(snd);
	StopSound(snd);
	return 0;
}

DEFINE_ACTION_FUNCTION_NATIVE(_Duke, CheckSoundPlaying, S_CheckSoundPlaying)
{
	PARAM_PROLOGUE;
	PARAM_INT(snd);
	ACTION_RETURN_INT(S_CheckSoundPlaying(snd));
}

DEFINE_GLOBAL_UNSIZED(dlevel)

//---------------------------------------------------------------------------
//
// DukeActor
//
//---------------------------------------------------------------------------

DEFINE_FIELD(DDukeActor, ownerActor)
DEFINE_FIELD(DDukeActor, hitOwnerActor)
DEFINE_FIELD(DDukeActor, cgg)
DEFINE_FIELD(DDukeActor, spriteextra)
DEFINE_FIELD(DDukeActor, hitang)
DEFINE_FIELD(DDukeActor, hitextra)
DEFINE_FIELD(DDukeActor, movflag)
DEFINE_FIELD(DDukeActor, tempang)
DEFINE_FIELD(DDukeActor, timetosleep)
DEFINE_FIELD(DDukeActor, floorz)
DEFINE_FIELD(DDukeActor, ceilingz)
DEFINE_FIELD(DDukeActor, saved_ammo)
DEFINE_FIELD(DDukeActor, palvals)
DEFINE_FIELD(DDukeActor, temp_data)
DEFINE_FIELD(DDukeActor, temp_actor)
DEFINE_FIELD(DDukeActor, seek_actor)
DEFINE_FIELD(DDukeActor, flags1)
DEFINE_FIELD(DDukeActor, flags2)
DEFINE_FIELD(DDukeActor, spritesetindex)

static void setSpritesetImage(DDukeActor* self, unsigned int index)
{
	auto& spriteset = static_cast<PClassActor*>(self->GetClass())->ActorInfo()->SpriteSet;

	if (index >= spriteset.Size())
	{
		ThrowAbortException(X_ARRAY_OUT_OF_BOUNDS, "Bad sprite set index %d (max. allowed is %d", index, spriteset.Size() - 1);
	}
	self->spritesetindex = index;
	self->spr.picnum = spriteset[index];
}

DEFINE_ACTION_FUNCTION_NATIVE(DDukeActor, SetSpritesetImage, setSpritesetImage)
{
	PARAM_SELF_PROLOGUE(DDukeActor);
	PARAM_UINT(index);
	setSpritesetImage(self, index);
	return 0;
}

//---------------------------------------------------------------------------
//
// DukePlayer
//
//---------------------------------------------------------------------------

DEFINE_FIELD_X(DukePlayer, player_struct, gotweapon)
DEFINE_FIELD_X(DukePlayer, player_struct, pals)
DEFINE_FIELD_X(DukePlayer, player_struct, weapon_sway)
DEFINE_FIELD_X(DukePlayer, player_struct, oweapon_sway)
DEFINE_FIELD_X(DukePlayer, player_struct, weapon_pos)
DEFINE_FIELD_X(DukePlayer, player_struct, kickback_pic)
DEFINE_FIELD_X(DukePlayer, player_struct, random_club_frame)
DEFINE_FIELD_X(DukePlayer, player_struct, oweapon_pos)
DEFINE_FIELD_X(DukePlayer, player_struct, okickback_pic)
DEFINE_FIELD_X(DukePlayer, player_struct, orandom_club_frame)
DEFINE_FIELD_X(DukePlayer, player_struct, hard_landing)
DEFINE_FIELD_X(DukePlayer, player_struct, ohard_landing)
DEFINE_FIELD_X(DukePlayer, player_struct, psectlotag)
//DEFINE_FIELD_X(DukePlayer, player_struct, exitx)
//DEFINE_FIELD_X(DukePlayer, player_struct, exity)
//DEFINE_FIELD_X(DukePlayer, player_struct, loogiex)
//DEFINE_FIELD_X(DukePlayer, player_struct, loogiey)
DEFINE_FIELD_X(DukePlayer, player_struct, numloogs)
DEFINE_FIELD_X(DukePlayer, player_struct, loogcnt)
DEFINE_FIELD_X(DukePlayer, player_struct, invdisptime)
//DEFINE_FIELD_X(DukePlayer, player_struct, bobposx)
//DEFINE_FIELD_X(DukePlayer, player_struct, bobposy)
//DEFINE_FIELD_X(DukePlayer, player_struct, oposx)
//DEFINE_FIELD_X(DukePlayer, player_struct, oposy)
//DEFINE_FIELD_X(DukePlayer, player_struct, oposz)
DEFINE_FIELD_X(DukePlayer, player_struct, pyoff)
DEFINE_FIELD_X(DukePlayer, player_struct, opyoff)
//DEFINE_FIELD_X(DukePlayer, player_struct, posxv)
//DEFINE_FIELD_X(DukePlayer, player_struct, posyv)
//DEFINE_FIELD_X(DukePlayer, player_struct, poszv)
DEFINE_FIELD_X(DukePlayer, player_struct, last_pissed_time)
DEFINE_FIELD_X(DukePlayer, player_struct, truefz)
DEFINE_FIELD_X(DukePlayer, player_struct, truecz)
DEFINE_FIELD_X(DukePlayer, player_struct, player_par)
DEFINE_FIELD_X(DukePlayer, player_struct, visibility)
DEFINE_FIELD_X(DukePlayer, player_struct, bobcounter)
DEFINE_FIELD_X(DukePlayer, player_struct, randomflamex)
DEFINE_FIELD_X(DukePlayer, player_struct, crack_time)
DEFINE_FIELD_X(DukePlayer, player_struct, aim_mode)
DEFINE_FIELD_X(DukePlayer, player_struct, ftt)
DEFINE_FIELD_X(DukePlayer, player_struct, cursector)
DEFINE_FIELD_X(DukePlayer, player_struct, last_extra)
DEFINE_FIELD_X(DukePlayer, player_struct, subweapon)
DEFINE_FIELD_X(DukePlayer, player_struct, ammo_amount)
DEFINE_FIELD_X(DukePlayer, player_struct, frag)
DEFINE_FIELD_X(DukePlayer, player_struct, fraggedself)
DEFINE_FIELD_X(DukePlayer, player_struct, curr_weapon)
DEFINE_FIELD_X(DukePlayer, player_struct, last_weapon)
DEFINE_FIELD_X(DukePlayer, player_struct, tipincs)
DEFINE_FIELD_X(DukePlayer, player_struct, wantweaponfire)
DEFINE_FIELD_X(DukePlayer, player_struct, holoduke_amount)
DEFINE_FIELD_X(DukePlayer, player_struct, hurt_delay)
DEFINE_FIELD_X(DukePlayer, player_struct, hbomb_hold_delay)
DEFINE_FIELD_X(DukePlayer, player_struct, jumping_counter)
DEFINE_FIELD_X(DukePlayer, player_struct, airleft)
DEFINE_FIELD_X(DukePlayer, player_struct, knee_incs)
DEFINE_FIELD_X(DukePlayer, player_struct, access_incs)
DEFINE_FIELD_X(DukePlayer, player_struct, ftq)
DEFINE_FIELD_X(DukePlayer, player_struct, access_wall)
DEFINE_FIELD_X(DukePlayer, player_struct, got_access)
DEFINE_FIELD_X(DukePlayer, player_struct, weapon_ang)
DEFINE_FIELD_X(DukePlayer, player_struct, firstaid_amount)
DEFINE_FIELD_X(DukePlayer, player_struct, actor)
DEFINE_FIELD_X(DukePlayer, player_struct, one_parallax_sectnum)
DEFINE_FIELD_X(DukePlayer, player_struct, over_shoulder_on)
DEFINE_FIELD_X(DukePlayer, player_struct, fist_incs)
DEFINE_FIELD_X(DukePlayer, player_struct, cheat_phase)
DEFINE_FIELD_X(DukePlayer, player_struct, extra_extra8)
DEFINE_FIELD_X(DukePlayer, player_struct, quick_kick)
DEFINE_FIELD_X(DukePlayer, player_struct, last_quick_kick)
DEFINE_FIELD_X(DukePlayer, player_struct, heat_amount)
DEFINE_FIELD_X(DukePlayer, player_struct, timebeforeexit)
DEFINE_FIELD_X(DukePlayer, player_struct, customexitsound)
DEFINE_FIELD_X(DukePlayer, player_struct, weaprecs)
DEFINE_FIELD_X(DukePlayer, player_struct, weapreccnt)
DEFINE_FIELD_X(DukePlayer, player_struct, interface_toggle_flag)
DEFINE_FIELD_X(DukePlayer, player_struct, dead_flag)
DEFINE_FIELD_X(DukePlayer, player_struct, show_empty_weapon)
DEFINE_FIELD_X(DukePlayer, player_struct, scuba_amount)
DEFINE_FIELD_X(DukePlayer, player_struct, jetpack_amount)
DEFINE_FIELD_X(DukePlayer, player_struct, steroids_amount)
DEFINE_FIELD_X(DukePlayer, player_struct, shield_amount)
DEFINE_FIELD_X(DukePlayer, player_struct, pycount)
DEFINE_FIELD_X(DukePlayer, player_struct, frag_ps)
DEFINE_FIELD_X(DukePlayer, player_struct, transporter_hold)
DEFINE_FIELD_X(DukePlayer, player_struct, last_full_weapon)
DEFINE_FIELD_X(DukePlayer, player_struct, footprintshade)
DEFINE_FIELD_X(DukePlayer, player_struct, boot_amount)
DEFINE_FIELD_X(DukePlayer, player_struct, on_warping_sector)
DEFINE_FIELD_X(DukePlayer, player_struct, footprintcount)
DEFINE_FIELD_X(DukePlayer, player_struct, hbomb_on)
DEFINE_FIELD_X(DukePlayer, player_struct, jumping_toggle)
DEFINE_FIELD_X(DukePlayer, player_struct, rapid_fire_hold)
DEFINE_FIELD_X(DukePlayer, player_struct, on_ground)
DEFINE_FIELD_X(DukePlayer, player_struct, inven_icon)
DEFINE_FIELD_X(DukePlayer, player_struct, buttonpalette)
DEFINE_FIELD_X(DukePlayer, player_struct, jetpack_on)
DEFINE_FIELD_X(DukePlayer, player_struct, spritebridge)
DEFINE_FIELD_X(DukePlayer, player_struct, lastrandomspot)
DEFINE_FIELD_X(DukePlayer, player_struct, scuba_on)
DEFINE_FIELD_X(DukePlayer, player_struct, footprintpal)
DEFINE_FIELD_X(DukePlayer, player_struct, heat_on)
DEFINE_FIELD_X(DukePlayer, player_struct, holster_weapon)
DEFINE_FIELD_X(DukePlayer, player_struct, falling_counter)
DEFINE_FIELD_X(DukePlayer, player_struct, refresh_inventory)
DEFINE_FIELD_X(DukePlayer, player_struct, toggle_key_flag)
DEFINE_FIELD_X(DukePlayer, player_struct, knuckle_incs)
DEFINE_FIELD_X(DukePlayer, player_struct, walking_snd_toggle)
DEFINE_FIELD_X(DukePlayer, player_struct, palookup)
DEFINE_FIELD_X(DukePlayer, player_struct, quick_kick_msg)
DEFINE_FIELD_X(DukePlayer, player_struct, max_secret_rooms)
DEFINE_FIELD_X(DukePlayer, player_struct, secret_rooms)
DEFINE_FIELD_X(DukePlayer, player_struct, max_actors_killed)
DEFINE_FIELD_X(DukePlayer, player_struct, actors_killed)
DEFINE_FIELD_X(DukePlayer, player_struct, resurrected)
DEFINE_FIELD_X(DukePlayer, player_struct, stairs)
DEFINE_FIELD_X(DukePlayer, player_struct, detonate_count)
//DEFINE_FIELD_X(DukePlayer, player_struct, noise.X)
//DEFINE_FIELD_X(DukePlayer, player_struct, noise.Y)
DEFINE_FIELD_X(DukePlayer, player_struct, noise_radius)
DEFINE_FIELD_X(DukePlayer, player_struct, drink_timer)
DEFINE_FIELD_X(DukePlayer, player_struct, eat_timer)
DEFINE_FIELD_X(DukePlayer, player_struct, SlotWin)
DEFINE_FIELD_X(DukePlayer, player_struct, recoil)
DEFINE_FIELD_X(DukePlayer, player_struct, detonate_time)
DEFINE_FIELD_X(DukePlayer, player_struct, yehaa_timer)
DEFINE_FIELD_X(DukePlayer, player_struct, drink_amt)
DEFINE_FIELD_X(DukePlayer, player_struct, eat)
DEFINE_FIELD_X(DukePlayer, player_struct, drunkang)
DEFINE_FIELD_X(DukePlayer, player_struct, eatang)
DEFINE_FIELD_X(DukePlayer, player_struct, shotgun_state)
DEFINE_FIELD_X(DukePlayer, player_struct, donoise)
DEFINE_FIELD_X(DukePlayer, player_struct, keys)
DEFINE_FIELD_X(DukePlayer, player_struct, drug_aspect)
DEFINE_FIELD_X(DukePlayer, player_struct, drug_timer)
DEFINE_FIELD_X(DukePlayer, player_struct, SeaSick)
DEFINE_FIELD_X(DukePlayer, player_struct, MamaEnd)
DEFINE_FIELD_X(DukePlayer, player_struct, moto_drink)
DEFINE_FIELD_X(DukePlayer, player_struct, TiltStatus)
DEFINE_FIELD_X(DukePlayer, player_struct, oTiltStatus)
DEFINE_FIELD_X(DukePlayer, player_struct, VBumpNow)
DEFINE_FIELD_X(DukePlayer, player_struct, VBumpTarget)
DEFINE_FIELD_X(DukePlayer, player_struct, TurbCount)
DEFINE_FIELD_X(DukePlayer, player_struct, drug_stat)
DEFINE_FIELD_X(DukePlayer, player_struct, DrugMode)
DEFINE_FIELD_X(DukePlayer, player_struct, lotag800kill)
DEFINE_FIELD_X(DukePlayer, player_struct, sea_sick_stat)
DEFINE_FIELD_X(DukePlayer, player_struct, hurt_delay2)
DEFINE_FIELD_X(DukePlayer, player_struct, nocheat)
DEFINE_FIELD_X(DukePlayer, player_struct, OnMotorcycle)
DEFINE_FIELD_X(DukePlayer, player_struct, OnBoat)
DEFINE_FIELD_X(DukePlayer, player_struct, moto_underwater)
DEFINE_FIELD_X(DukePlayer, player_struct, NotOnWater)
DEFINE_FIELD_X(DukePlayer, player_struct, MotoOnGround)
DEFINE_FIELD_X(DukePlayer, player_struct, moto_do_bump)
DEFINE_FIELD_X(DukePlayer, player_struct, moto_bump_fast)
DEFINE_FIELD_X(DukePlayer, player_struct, moto_on_oil)
DEFINE_FIELD_X(DukePlayer, player_struct, moto_on_mud)
DEFINE_FIELD_X(DukePlayer, player_struct, vehForwardScale)
DEFINE_FIELD_X(DukePlayer, player_struct, vehReverseScale)
DEFINE_FIELD_X(DukePlayer, player_struct, MotoSpeed)
DEFINE_FIELD_X(DukePlayer, player_struct, vehTurnLeft)
DEFINE_FIELD_X(DukePlayer, player_struct, vehTurnRight)
DEFINE_FIELD_X(DukePlayer, player_struct, vehBraking)
DEFINE_FIELD_X(DukePlayer, player_struct, holoduke_on)

DEFINE_ACTION_FUNCTION(_DukePlayer, IsFrozen)
{
	PARAM_SELF_STRUCT_PROLOGUE(player_struct);
	ACTION_RETURN_BOOL(self->GetActor()->spr.pal == 1 && self->last_extra < 2);
}

DEFINE_ACTION_FUNCTION(_DukePlayer, GetGameVar)
{
	PARAM_SELF_STRUCT_PROLOGUE(player_struct);
	PARAM_STRING(name);
	PARAM_INT(def);
	ACTION_RETURN_INT(GetGameVar(name, def, self->GetActor(), self->GetPlayerNum()).safeValue());
}


//---------------------------------------------------------------------------
//
// Class properties
//
//---------------------------------------------------------------------------

//==========================================================================
//
//==========================================================================
DEFINE_PROPERTY(spriteset, Ssssssssssssssssssss, DukeActor)
{
	for (int i = 0; i < PROP_PARM_COUNT; ++i)
	{
		PROP_STRING_PARM(n, i);
		bag.Info->ActorInfo()->SpriteSet.Push(TileFiles.tileForName(n));
	}
}


END_DUKE_NS