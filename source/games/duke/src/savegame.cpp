//-------------------------------------------------------------------------
/*
Copyright (C) 1996, 2003 - 3D Realms Entertainment
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
#include "serializer.h"
#include "mapinfo.h"
#include "duke3d.h"
#include "gamestate.h"
#include "dukeactor.h"
#include "savegamehelp.h"
#include "gamevar.h"

//==========================================================================
//
//
//
//==========================================================================


BEGIN_DUKE_NS

FSerializer& Serialize(FSerializer& arc, const char* keyname, GameVarValue& w, GameVarValue* def);
void lava_serialize(FSerializer& arc);
void SerializeGameVars(FSerializer &arc);

template<class T>
FSerializer& NamedSerialize(FSerializer& arc, const char* keyname, T*& w, TArray<T>& store)
{
	if (arc.isWriting())
	{
		auto ww = w ? w : &store[0];
		if (keyname == nullptr || ww->qualifiedName != NAME_None) Serialize(arc, keyname, ww->qualifiedName, nullptr);
	}
	else
	{
		FName n = NAME_None;
		Serialize(arc, keyname, n, nullptr);
		auto index = store.FindEx([=](const auto& el) { return el.qualifiedName == n; });
		if (index >= store.Size()) index = 0;
		w = &store[index];
	}
	return arc;
}

FSerializer& Serialize(FSerializer& arc, const char* keyname, ActorMove*& w, ActorMove** def)
{
	return NamedSerialize(arc, keyname, w, moves);
}

FSerializer& Serialize(FSerializer& arc, const char* keyname, ActorAction*& w, ActorAction** def)
{
	return NamedSerialize(arc, keyname, w, actions);
}

FSerializer& Serialize(FSerializer& arc, const char* keyname, animwalltype& w, animwalltype* def)
{
	if (arc.BeginObject(keyname))
	{
	  arc("wall", w.wall)
		("tag", w.tag)
		("texid", w.origtex)
		("overpic", w.overpic)
		  .EndObject();
	}
	return arc;
}

FSerializer& Serialize(FSerializer& arc, const char* keyname, player_orig& w, player_orig* def)
{
	if (arc.BeginObject(keyname))
	{
	  arc("opos", w.opos)
		("oa", w.oa)
		("os", w.os)
		.EndObject();
	}
	return arc;
}

void DDukePlayer::Serialize(FSerializer& arc)
{
	Super::Serialize(arc);
	arc.Array("gotweapon", gotweapon, MAX_WEAPONS)
		("pals", pals)
		("fric", fric)
		("exit", Exit)
		("numloogs", numloogs)
		("loogcnt", loogcnt)
		.Array("loogie", loogie, numloogs)
		("bobpos", bobpos)
		("pyoff", pyoff)
		("posv", vel)
		("last_pissed_time", last_pissed_time)
		("truefz", truefz)
		("truecz", truecz)
		("player_par", player_par)
		("visibility", visibility)
		("bobcounter", bobcounter)
		("weapon_sway", weapon_sway)
		("randomflamex", randomflamex)
		("crack_time", crack_time)
		("aim.mode", aim_mode)
		("psectlotag", psectlotag)
		("cursectnum", cursector)
		("last_extra", last_extra)
		("subweapon", subweapon)
		.Array("ammo_count", ammo_amount, MAX_WEAPONS)
		("wackedbyactor", wackedbyactor)
		("frag", frag)
		("fraggedself", fraggedself)
		("curr_weapon", curr_weapon)
		("last_weapon", last_weapon)
		("tipincs", tipincs)
		("wantweaponfire", wantweaponfire)
		("holoduke_amount", holoduke_amount)
		("newowner", newOwner)
		("hurt_delay", hurt_delay)
		("hbomb_hold_delay", hbomb_hold_delay)
		("jumping_counter", jumping_counter)
		("airleft", airleft)
		("knee_incs", knee_incs)
		("access_incs", access_incs)
		("ftq", ftq)
		("access_wallnum", access_wall)
		("access_spritenum", access_spritenum)
		("kickback_pic", kickback_pic)
		("got_access", got_access)
		("weapon_ang", weapon_ang)
		("firstaid_amount", firstaid_amount)
		("somethingonplayer", somethingonplayer)
		("on_crane", on_crane)
		("one_parallax_sectnum", one_parallax_sectnum)
		("over_shoulder_on", over_shoulder_on)
		("random_club_frame", random_club_frame)
		("fist_incs", fist_incs)
		("dummyplayersprite", dummyplayersprite)
		("extra_extra8", extra_extra8)
		("quick_kick", quick_kick)
		("last_quick_kick", last_quick_kick)
		("heat_amount", heat_amount)
		("actorsqu", actorsqu)
		("timebeforeexit", timebeforeexit)
		("customexitsound", customexitsound)
		("weapreccnt", weapreccnt)
		.Array("weaprecs", weaprecs, weapreccnt)
		("interface_toggle_flag", interface_toggle_flag)
		("dead_flag", dead_flag)
		("show_empty_weapon", show_empty_weapon)
		("scuba_amount", scuba_amount)
		("jetpack_amount", jetpack_amount)
		("steroids_amount", steroids_amount)
		("shield_amount", shield_amount)
		("holoduke_on", holoduke_on)
		("pycount", pycount)
		("weapon_pos", weapon_pos)
		("frag_ps", frag_ps)
		("transporter_hold", transporter_hold)
		("last_full_weapon", last_full_weapon)
		("footprintshade", footprintshade)
		("boot_amount", boot_amount)
		("on_warping_sector", on_warping_sector)
		("footprintcount", footprintcount)
		("hbomb_on", hbomb_on)
		("jumping_toggle", jumping_toggle)
		("rapid_fire_hold", rapid_fire_hold)
		("on_ground", on_ground)
		.Array("name", name, 32)
		("inven_icon", inven_icon)
		("buttonpalette", buttonpalette)
		("jetpack_on", jetpack_on)
		("spritebridge", spritebridge)
		("lastrandomspot", lastrandomspot)
		("scuba_on", scuba_on)
		("footprintpal", footprintpal)
		("heat_on", heat_on)
		("holster_weapon", holster_weapon)
		("falling_counter", falling_counter)
		("refresh_inventory", refresh_inventory)
		("toggle_key_flag", toggle_key_flag)
		("knuckle_incs", knuckle_incs)
		("walking_snd_toggle", walking_snd_toggle)
		("palookup", palookup)
		("hard_landing", hard_landing)
		// RR from here on
		("stairs", stairs)
		("detonate_count", detonate_count)
		("noise", noise)
		("noise_radius", noise_radius)
		("drink_timer", drink_timer)
		("eat_timer", eat_timer)
		("slotwin", SlotWin)
		("recoil", recoil)
		("detonate_time", detonate_time)
		("yehaa_timer", yehaa_timer)
		("drink_amt", drink_amt)
		("eat", eat)
		("drunkang", drunkang)
		("eatang", eatang)
		.Array("shotgun_state", shotgun_state, 2)
		("donoise", donoise)
		.Array("keys", keys, 5)
		// RRRA from here on
		("drug_aspect", drug_aspect)
		("drug_timer", drug_timer)
		("seasick", SeaSick)
		("mamaend", MamaEnd)
		("motospeed", MotoSpeed)
		("moto_drink", moto_drink)
		("tiltstatus", TiltStatus)
		("vbumpnow", VBumpNow)
		("vbumptarget", VBumpTarget)
		("turbcount", TurbCount)
		.Array("drug_stat", drug_stat, 3)
		("drugmode", DrugMode)
		("lotag800kill", lotag800kill)
		("sea_sick_stat", sea_sick_stat)
		("hurt_delay2", hurt_delay2)
		("nocheat", nocheat)
		("onmotorcycle", OnMotorcycle)
		("onboat", OnBoat)
		("moto_underwater", moto_underwater)
		("notonwater", NotOnWater)
		("motoonground", MotoOnGround)
		("moto_do_bump", moto_do_bump)
		("moto_bump_fast", moto_bump_fast)
		("moto_on_oil", moto_on_oil)
		("moto_on_mud", moto_on_mud)
		// new stuff
		.Array("frags", frags, MAXPLAYERS)
		("uservars", uservars)
		("fistsign", fistsign);

	if (arc.isReading())
	{
		invdisptime = 0;
		opyoff = pyoff;
		backupweapon();
	}
}


void DDukeActor::Serialize(FSerializer& arc)
{
	//AActor* def = GetDefault();

	Super::Serialize(arc);

	arc("cgg", cgg)
		("spriteextra", spriteextra)
		("attackertype", attackertype)
		("ang", hitang)
		("extra", hitextra)
		("owneractor", ownerActor)
		("owner", hitOwnerActor)
		("movflag", movflag)
		("tempang", tempval)
		("actorstayput", actorstayput)
		("basepicnum", basepicnum)
		("timetosleep", timetosleep)
		("mapspawned", mapSpawned)
		("floorz", floorz)
		("ceilingz", ceilingz)
		("lastv", ovel)
		("saved_ammo", saved_ammo)
		("temp_actor", temp_actor)
		("seek_actor", seek_actor)
		.Array("temp_data", temp_data, 5)
		.Array("temp_wall", temp_walls, 2)
		("temp_angle", temp_angle)
		("temp_pos", temp_pos)
		("temp_pos2", temp_pos2)
		("temp_sect", temp_sect)
		("uservars", uservars)
		("flags1", flags1)
		("flags2", flags2)
		("flags3", flags3)
		("flags4", flags4)
		("curmove", curMove)
		("curaction", curAction)
		("curai", curAI)
		("curframe", curframe)
		("counter", counter)
		("actioncounter", actioncounter);
}


FSerializer& Serialize(FSerializer& arc, const char* keyname, Cycler& w, Cycler* def)
{
	static Cycler nul;
	if (!def)
	{
		def = &nul;
		if (arc.isReading()) w = {};
	}
	if (arc.BeginObject(keyname))
	{
		arc("sector", w.sector, def->sector)
			("lotag", w.lotag, def->lotag)
			("hitag", w.hitag, def->hitag)
			("shade1", w.shade1, def->shade1)
			("shade2", w.shade2, def->shade2)
			("state", w.state, def->state)
			.EndObject();
	}
	return arc;
}

FSerializer& Serialize(FSerializer& arc, const char* keyname, AmbientTags& w, AmbientTags* def)
{
	static AmbientTags nul;
	if (!def)
	{
		def = &nul;
		if (arc.isReading()) w = {};
	}
	if (arc.BeginObject(keyname))
	{
		arc("lotag", w.lo, def->lo)
			("hitag", w.hi, def->hi)
			.EndObject();
	}
	return arc;
}

FSerializer& Serialize(FSerializer& arc, const char* keyname, animate& w, animate* def)
{
	static animate nul;
	if (!def)
	{
		def = &nul;
		if (arc.isReading()) w = {};
	}
	if (arc.BeginObject(keyname))
	{
		arc("sector", w.sect, def->sect)
			("type", w.type, def->type)
			("target", w.target, def->target)
			("goal", w.goal, def->goal)
			("vel", w.vel, def->vel)
			.EndObject();
	}
	return arc;
}

void GameInterface::SerializeGameState(FSerializer& arc)
{
	if (arc.isReading())
	{
		memset(geosectorwarp, -1, sizeof(geosectorwarp));
		memset(geosectorwarp2, -1, sizeof(geosectorwarp2));
	}
	if (arc.BeginObject("duke.gamestate"))
	{
		arc("multimode", ud.multimode);

		arc("skill", ud.player_skill)
			("from_bonus", ud.from_bonus)
			("secretlevel", ud.secretlevel)
			("respawn_monsters", ud.respawn_monsters)
			("respawn_items", ud.respawn_items)
			("respawn_inventory", ud.respawn_inventory)
			("const_visibility", ud.const_visibility)
			("god", ud.god)
			("eog", ud.eog)
			("monsters_off", ud.monsters_off)
			("last_level", ud.last_level)
			("coop", ud.coop)
			("marker", ud.marker)
			("ffire", ud.ffire)
			("levelclock", PlayClock)
			("bomb_tag", ud.bomb_tag)

			("rtsplaying", rtsplaying)
			//("tempwallptr", tempwallptr)
			("joe9000", ud.joe9000)
			("spriteqamount", spriteqamount)
			("lastvisinc", lastvisinc)
			("numanimwalls", numanimwalls)
			.Array("animwall", animwall, numanimwalls)
			("camsprite", camsprite)
			("earthquaketime", ud.earthquaketime)
			("gs.freezerhurtowner", gs.freezerhurtowner)
			("global_random", global_random)
			("gs.impact_damage", gs.impact_damage)
			("numplayersprites", numplayersprites)
			("spriteqloc", spriteqloc)
			("animates", animates)
			("chickenplant", ud.chickenplant)
			("ufospawnsminion", ud.ufospawnsminion)

			("numclouds", numclouds)
			("cloudx", cloudx)
			("cloudy", cloudy)
			("cloudclock", cloudclock)
			.Array("clouds", clouds, numclouds)

			.Array("spriteq", spriteq, 1024)
			("cycler", cyclers)
			("mirrorcnt", mirrorcnt)
			.Array("mirrorsector", mirrorsector, mirrorcnt)
			.Array("mirrorwall", mirrorwall, mirrorcnt)
			("wupass", wupass)
			("thunderon", thunderon)
			("ufospawn", ufospawn)
			("ufocnt", ufocnt)
			("hulkspawn", hulkspawn)
			("lastlevel", lastlevel)
			("geocnt", geocnt)
			.Array("geosectorwarp", geosectorwarp, geocnt)
			.Array("geosectorwarp2", geosectorwarp2, geocnt)
			.Array("geosector", geosector, geocnt)
			.Array("geox", geox, geocnt)
			.Array("geoy", geoy, geocnt)
			.Array("geox2", geox2, geocnt)
			.Array("geoy2", geoy2, geocnt)
			("ambienttags", ambienttags)
			("mspos", mspos)
			("windtime", WindTime)
			("winddir", WindDir)
			("fakebubba_spawn", fakebubba_spawn)
			("mamaspawn_count", mamaspawn_count)
			("banjosound", banjosound)
			("enemysizecheat", enemysizecheat)
			("pistonsound", ud.pistonsound)
			("chickenphase", chickenphase)
			("RRRA_ExitedLevel", RRRA_ExitedLevel)
			("fogactive", ud.fogactive)
			("thunder_brightness", thunder_brightness)
			.Array("po", po, ud.multimode)
			("rrcdtrack", g_cdTrack)
			.EndObject();

		lava_serialize(arc);
		SerializeGameVars(arc);

		if (arc.isReading())
		{
			screenpeek = myconnectindex;
			ud.recstat = 0;

			ud.m_respawn_monsters = ud.respawn_monsters;
			ud.m_respawn_items = ud.respawn_items;
			ud.m_respawn_inventory = ud.respawn_inventory;
			ud.m_monsters_off = ud.monsters_off;
			ud.m_coop = ud.coop;
			ud.m_ffire = ud.ffire;
			if (getPlayer(myconnectindex)->over_shoulder_on != 0)
			{
				cameradist = 0;
				cameraclock = 0;
				getPlayer(myconnectindex)->over_shoulder_on = 1;
			}

			cacheit();

			Mus_ResumeSaved();
			Mus_SetPaused(false);

			S_SetReverb(0);
			show_shareware = 0;

			S_SetReverb(0);
			resetlanepics();

		}
	}
}

END_DUKE_NS
