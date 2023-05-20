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
	  arc("ox", w.opos.X)
		("oy", w.opos.Y)
		("oz", w.opos.Z)
		("oa", w.oa)
		("os", w.os)
		.EndObject();
	}
	return arc;
}

FSerializer& Serialize(FSerializer& arc, const char* keyname, player_struct& w, player_struct* def)
{
	if (arc.BeginObject(keyname))
	{
		arc("angles", w.Angles)
			.Array("gotweapon", w.gotweapon, MAX_WEAPONS)
			("pals", w.pals)
			("fricx", w.fric.X)
			("fricy", w.fric.Y)
			("exitx", w.Exit.X)
			("exity", w.Exit.Y)
			("numloogs", w.numloogs)
			("loogcnt", w.loogcnt)
			.Array("loogie", w.loogie, w.numloogs)
			("bobposx", w.bobpos.X)
			("bobposy", w.bobpos.Y)
			("pyoff", w.pyoff)
			("posxv", w.vel.X)
			("posyv", w.vel.Y)
			("poszv", w.vel.Z)
			("last_pissed_time", w.last_pissed_time)
			("truefz", w.truefz)
			("truecz", w.truecz)
			("player_par", w.player_par)
			("visibility", w.visibility)
			("bobcounter", w.bobcounter)
			("weapon_sway", w.weapon_sway)
			("randomflamex", w.randomflamex)
			("crack_time", w.crack_time)
			("aim.mode", w.aim_mode)
			("psectlotag", w.psectlotag)
			("cursectnum", w.cursector)
			("last_extra", w.last_extra)
			("subweapon", w.subweapon)
			.Array("ammo_count", w.ammo_amount, MAX_WEAPONS)
			("wackedbyactor", w.wackedbyactor)
			("frag", w.frag)
			("fraggedself", w.fraggedself)
			("curr_weapon", w.curr_weapon)
			("last_weapon", w.last_weapon)
			("tipincs", w.tipincs)
			("wantweaponfire", w.wantweaponfire)
			("holoduke_amount", w.holoduke_amount)
			("newowner", w.newOwner)
			("hurt_delay", w.hurt_delay)
			("hbomb_hold_delay", w.hbomb_hold_delay)
			("jumping_counter", w.jumping_counter)
			("airleft", w.airleft)
			("knee_incs", w.knee_incs)
			("access_incs", w.access_incs)
			("ftq", w.ftq)
			("access_wallnum", w.access_wall)
			("access_spritenum", w.access_spritenum)
			("kickback_pic", w.kickback_pic)
			("got_access", w.got_access)
			("weapon_ang", w.weapon_ang)
			("firstaid_amount", w.firstaid_amount)
			("somethingonplayer", w.somethingonplayer)
			("on_crane", w.on_crane)
			("i", w.actor)
			("one_parallax_sectnum", w.one_parallax_sectnum)
			("over_shoulder_on", w.over_shoulder_on)
			("random_club_frame", w.random_club_frame)
			("fist_incs", w.fist_incs)
			("dummyplayersprite", w.dummyplayersprite)
			("extra_extra8", w.extra_extra8)
			("quick_kick", w.quick_kick)
			("last_quick_kick", w.last_quick_kick)
			("heat_amount", w.heat_amount)
			("actorsqu", w.actorsqu)
			("timebeforeexit", w.timebeforeexit)
			("customexitsound", w.customexitsound)
			("weapreccnt", w.weapreccnt)
			.Array("weaprecs", w.weaprecs, w.weapreccnt)
			("interface_toggle_flag", w.interface_toggle_flag)
			("dead_flag", w.dead_flag)
			("show_empty_weapon", w.show_empty_weapon)
			("scuba_amount", w.scuba_amount)
			("jetpack_amount", w.jetpack_amount)
			("steroids_amount", w.steroids_amount)
			("shield_amount", w.shield_amount)
			("holoduke_on", w.holoduke_on)
			("pycount", w.pycount)
			("weapon_pos", w.weapon_pos)
			("frag_ps", w.frag_ps)
			("transporter_hold", w.transporter_hold)
			("last_full_weapon", w.last_full_weapon)
			("footprintshade", w.footprintshade)
			("boot_amount", w.boot_amount)
			("on_warping_sector", w.on_warping_sector)
			("footprintcount", w.footprintcount)
			("hbomb_on", w.hbomb_on)
			("jumping_toggle", w.jumping_toggle)
			("rapid_fire_hold", w.rapid_fire_hold)
			("on_ground", w.on_ground)
			.Array("name", w.name, 32)
			("inven_icon", w.inven_icon)
			("buttonpalette", w.buttonpalette)
			("jetpack_on", w.jetpack_on)
			("spritebridge", w.spritebridge)
			("lastrandomspot", w.lastrandomspot)
			("scuba_on", w.scuba_on)
			("footprintpal", w.footprintpal)
			("heat_on", w.heat_on)
			("holster_weapon", w.holster_weapon)
			("falling_counter", w.falling_counter)
			("refresh_inventory", w.refresh_inventory)
			("toggle_key_flag", w.toggle_key_flag)
			("knuckle_incs", w.knuckle_incs)
			("walking_snd_toggle", w.walking_snd_toggle)
			("palookup", w.palookup)
			("hard_landing", w.hard_landing)
			("max_secret_rooms", w.max_secret_rooms)
			("secret_rooms", w.secret_rooms)
			("max_actors_killed", w.max_actors_killed)
			("actors_killed", w.actors_killed)
			// RR from here on
			("stairs", w.stairs)
			("detonate_count", w.detonate_count)
			("noise.X", w.noise.X)
			("noise.Y", w.noise.Y)
			("noise_radius", w.noise_radius)
			("drink_timer", w.drink_timer)
			("eat_timer", w.eat_timer)
			("slotwin", w.SlotWin)
			("recoil", w.recoil)
			("detonate_time", w.detonate_time)
			("yehaa_timer", w.yehaa_timer)
			("drink_amt", w.drink_amt)
			("eat", w.eat)
			("drunkang", w.drunkang)
			("eatang", w.eatang)
			.Array("shotgun_state", w.shotgun_state, 2)
			("donoise", w.donoise)
			.Array("keys", w.keys, 5)
			// RRRA from here on
			("drug_aspect", w.drug_aspect)
			("drug_timer", w.drug_timer)
			("seasick", w.SeaSick)
			("mamaend", w.MamaEnd)
			("motospeed", w.MotoSpeed)
			("moto_drink", w.moto_drink)
			("tiltstatus", w.TiltStatus)
			("vbumpnow", w.VBumpNow)
			("vbumptarget", w.VBumpTarget)
			("turbcount", w.TurbCount)
			.Array("drug_stat", w.drug_stat, 3)
			("drugmode", w.DrugMode)
			("lotag800kill", w.lotag800kill)
			("sea_sick_stat", w.sea_sick_stat)
			("hurt_delay2", w.hurt_delay2)
			("nocheat", w.nocheat)
			("onmotorcycle", w.OnMotorcycle)
			("onboat", w.OnBoat)
			("moto_underwater", w.moto_underwater)
			("notonwater", w.NotOnWater)
			("motoonground", w.MotoOnGround)
			("moto_do_bump", w.moto_do_bump)
			("moto_bump_fast", w.moto_bump_fast)
			("moto_on_oil", w.moto_on_oil)
			("moto_on_mud", w.moto_on_mud)
			// new stuff
			("actions", w.sync.actions)
			.Array("frags", w.frags, MAXPLAYERS)
			("uservars", w.uservars)
			("fistsign", w.fistsign)
			("crouch_toggle", w.crouch_toggle)
			.EndObject();

		w.invdisptime = 0;
		w.GetActor()->backuploc();
		w.opyoff = w.pyoff;
		w.backupweapon();
		w.sync.actions &= SB_CENTERVIEW|SB_CROUCH; // these are the only bits we need to preserve.
	}
	return arc;
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
		("lastvx", ovel.X)
		("lastvy", ovel.Y)
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
			.Array("players", ps, ud.multimode)
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
			if (ps[myconnectindex].over_shoulder_on != 0)
			{
				cameradist = 0;
				cameraclock = 0;
				ps[myconnectindex].over_shoulder_on = 1;
			}

			cacheit();

			Mus_ResumeSaved();
			Mus_SetPaused(false);

			FX_SetReverb(0);
			show_shareware = 0;

			FX_SetReverb(0);
			resetlanepics();

		}
	}
}

END_DUKE_NS
