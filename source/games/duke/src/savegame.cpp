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

//==========================================================================
//
//
//
//==========================================================================

template<> FSerializer& Serialize(FSerializer& arc, const char* key, Duke3d::DDukeActor*& ht, Duke3d::DDukeActor** def)
{
	int index = ht? int(ht - Duke3d::hittype) : -1;
	assert(index >= -1 && index < MAXSPRITES);
	Serialize(arc, key, index, nullptr);
	ht = index < 0? nullptr : &Duke3d::hittype[index];
	return arc;
}

BEGIN_DUKE_NS

void SerializeActorGlobals(FSerializer& arc);
void lava_serialize(FSerializer& arc);
void SerializeGameVars(FSerializer &arc);


FSerializer& Serialize(FSerializer& arc, const char* keyname, CraneDef& w, CraneDef* def)
{
	if (arc.BeginObject(keyname))
	{
		arc("x", w.x)
			("y", w.y)
			("z", w.z)
			("polex", w.polex)
			("poley", w.poley)
			("pole", w.poleactor)
			.EndObject();
	}
	return arc;
}

FSerializer& Serialize(FSerializer& arc, const char* keyname, animwalltype& w, animwalltype* def)
{
	if (arc.BeginObject(keyname))
	{
	  arc("wallnum", w.wall)
		("tag", w.tag)
		.EndObject();
	}
	return arc;
}

FSerializer& Serialize(FSerializer& arc, const char* keyname, player_orig& w, player_orig* def)
{
	if (arc.BeginObject(keyname))
	{
	  arc("ox", w.ox)
		("oy", w.oy)
		("oz", w.oz)
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
		arc("posx", w.pos.x)
			("posy", w.pos.y)
			("posz", w.pos.z)
			("angle", w.angle)
			("horizon", w.horizon)
			.Array("gotweapon", w.gotweapon, MAX_WEAPONS)
			("pals", w.pals)
			("fricx", w.fric.x)
			("fricy", w.fric.y)
			("exitx", w.exitx)
			("exity", w.exity)
			("numloogs", w.numloogs)
			("loogcnt", w.loogcnt)
			.Array("loogiex", w.loogiex, w.numloogs)
			.Array("loogiey", w.loogiey, w.numloogs)
			("bobposx", w.bobposx)
			("bobposy", w.bobposy)
			("pyoff", w.pyoff)
			("posxv", w.posxv)
			("posyv", w.posyv)
			("poszv", w.poszv)
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
			("cursectnum", w.cursectnum)
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
			("i", w.i)
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
			("resurrected", w.resurrected)
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
			("noise_x", w.noise_x)
			("noise_y", w.noise_y)
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
			.EndObject();

		w.invdisptime = 0;
		w.oposx = w.pos.x;
		w.oposy = w.pos.y;
		w.oposz = w.pos.z;
		w.opyoff = w.pyoff;
		w.oweapon_sway = w.weapon_sway;
		w.oweapon_pos = w.weapon_pos;
		w.okickback_pic = w.kickback_pic;
		w.orandom_club_frame = w.random_club_frame;
		w.ohard_landing = w.hard_landing;
		w.sync.actions &= SB_CENTERVIEW|SB_CROUCH; // these are the only bits we need to preserve.
	}
	return arc;
}


FSerializer& Serialize(FSerializer& arc, const char* keyname, DDukeActor& w, DDukeActor* def)
{
	if (!def) def = &hittype[MAXSPRITES];
	if (arc.BeginObject(keyname))
	{
		arc("cgg", w.cgg, def->cgg)
			("spriteextra", w.spriteextra, def->spriteextra)
			("picnum", w.picnum, def->picnum)
			("ang", w.ang, def->ang)
			("extra", w.extra, def->extra)
			("owner", w.owner, def->owner)
			("movflag", w.movflag, def->movflag)
			("tempang", w.tempang, def->tempang)
			("actorstayput", w.actorstayput, def->actorstayput)
			("dispicnum", w.dispicnum, def->dispicnum)
			("timetosleep", w.timetosleep, def->timetosleep)
			("floorz", w.floorz, def->floorz)
			("ceilingz", w.ceilingz, def->ceilingz)
			("lastvx", w.lastvx, def->lastvx)
			("lastvy", w.lastvy, def->lastvy)
			("aflags", w.aflags, def->aflags)
			("saved_ammo", w.saved_ammo, def->saved_ammo)
			("temp_actor", w.temp_actor, def->temp_actor)
			("seek_actor", w.seek_actor, def->seek_actor)
			.Array("temp_data", w.temp_data, def->temp_data, 6)
			.Array("temo_wall", w.temp_walls, def->temp_walls,2)
			("temp_sect", w.temp_sect, def->temp_sect)
			.EndObject();

#ifdef  OLD_SAVEGAME
		// compat handling
		if (SaveVersion < 12 && arc.isReading())
		{
			if (w.s->picnum == SECTOREFFECTOR)
			{
				if (w.s->lotag == SE_20_STRETCH_BRIDGE)
				{
					for (int i : {0, 1}) w.temp_walls[i] = &wall[w.temp_data[i+1]];
				}
				if (w.s->lotag == SE_128_GLASS_BREAKING)
				{
					w.temp_walls[0] = &wall[w.temp_data[2]];
				}
			}
		}
#endif
	}
	return arc;
}

FSerializer& Serialize(FSerializer& arc, const char* keyname, Cycler& w, Cycler* def)
{
	static Cycler nul;
	if (!def) def = &nul;
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


void GameInterface::SerializeGameState(FSerializer& arc)
{
	if (arc.isReading())
	{
		for (auto& h : hittype) h.clear();
		memset(sectorextra, 0, sizeof(sectorextra));
		memset(shadedsector, 0, sizeof(shadedsector));
		memset(geosectorwarp, -1, sizeof(geosectorwarp));
		memset(geosectorwarp2, -1, sizeof(geosectorwarp2));
		memset(ambienthitag, -1, sizeof(ambienthitag));
		memset(ambientlotag, -1, sizeof(ambientlotag));
	}
	if (arc.BeginObject("duke.gamestate"))
	{
		arc("multimode", ud.multimode);

		arc.SparseArray("actors", hittype, MAXSPRITES, activeSprites)
			("skill", ud.player_skill)
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

			.Array("sectorextra", sectorextra, numsectors)
			("rtsplaying", rtsplaying)
			("tempwallptr", tempwallptr)
			("cranes", cranes)
			("sound445done", sound445done)
			.Array("players", ps, ud.multimode)
			("spriteqamount", spriteqamount)
			.Array("shadedsector", shadedsector, numsectors)
			("lastvisinc", lastvisinc)
			("numanimwalls", numanimwalls)
			.Array("animwall", animwall, numanimwalls)
			("camsprite", camsprite)
			("earthquaketime", earthquaketime)
			("gs.freezerhurtowner", gs.freezerhurtowner)
			("global_random", global_random)
			("gs.impact_damage", gs.impact_damage)
			("numplayersprites", numplayersprites)
			("spriteqloc", spriteqloc)
			("animatecnt", animatecnt)

			.Array("animatesect", animatesect, animatecnt)
			.Array("animatetype", animatetype, animatecnt)
			.Array("animatetarget", animatetarget, animatecnt)
			.Array("animategoal", animategoal, animatecnt)
			.Array("animatevel", animatevel, animatecnt)

			("numclouds", numclouds)
			("cloudx", cloudx)
			("cloudy", cloudy)
			("cloudclock", cloudclock)
			.Array("clouds", clouds, numclouds)

			.Array("spriteq", spriteq, 1024)
			("numcyclers", numcyclers)
			.Array("cycler", cyclers, numcyclers)
			("mirrorcnt", mirrorcnt)
			.Array("mirrorsector", mirrorsector, mirrorcnt)
			.Array("mirrorwall", mirrorwall, mirrorcnt)
			("wupass", wupass)
			("chickenplant", chickenplant)
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
			("ambientfx", ambientfx)
			.Array("ambientlotag", ambientlotag, ambientfx)
			.Array("ambienthitag", ambienthitag, ambientfx)
			.Array("msx", msx, MAXANIMPOINTS)
			.Array("msy", msy, MAXANIMPOINTS)
			("windtime", WindTime)
			("winddir", WindDir)
			("fakebubba_spawn", fakebubba_spawn)
			("mamaspawn_count", mamaspawn_count)
			("banjosound", banjosound)
			("belltime", BellTime)
			("bellsprite", BellSprite)
			("enemysizecheat", enemysizecheat)
			("ufospawnsminion", ufospawnsminion)
			("pistonsound", pistonsound)
			("chickenphase", chickenphase)
			("RRRA_ExitedLevel", RRRA_ExitedLevel)
			("fogactive", fogactive)
			("thunder_brightness", thunder_brightness)
			.Array("po", po, ud.multimode)
			.EndObject();

		SerializeActorGlobals(arc);
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

			memset(gotpic, 0, sizeof(gotpic));
			if (isRR()) cacheit_r(); else cacheit_d();

			Mus_ResumeSaved();
			Mus_SetPaused(false);

			FX_SetReverb(0);
			show_shareware = 0;
			everyothertime = 0;

			FX_SetReverb(0);

		}
	}
}

END_DUKE_NS
