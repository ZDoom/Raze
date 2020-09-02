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


BEGIN_DUKE_NS

void SerializeActorGlobals(FSerializer& arc);
void lava_serialize(FSerializer& arc);
void SerializeGameVars(FSerializer &arc);


static void recreateinterpolations()
{
	numinterpolations = 0;

	int k = headspritestat[STAT_EFFECTOR];
	while (k >= 0)
	{
		switch (sprite[k].lotag)
		{
		case SE_31_FLOOR_RISE_FALL:
			setinterpolation(&sector[sprite[k].sectnum].floorz);
			break;
		case SE_32_CEILING_RISE_FALL:
			setinterpolation(&sector[sprite[k].sectnum].ceilingz);
			break;
		case SE_17_WARP_ELEVATOR:
		case SE_25_PISTON:
			setinterpolation(&sector[sprite[k].sectnum].floorz);
			setinterpolation(&sector[sprite[k].sectnum].ceilingz);
			break;
		case SE_0_ROTATING_SECTOR:
		case SE_5_BOSS:
		case SE_6_SUBWAY:
		case SE_11_SWINGING_DOOR:
		case SE_14_SUBWAY_CAR:
		case SE_15_SLIDING_DOOR:
		case SE_16_REACTOR:
		case SE_26:
		case SE_30_TWO_WAY_TRAIN:
			setsectinterpolate(k);
			break;
		}

		k = nextspritestat[k];
	}

	for (int i = numinterpolations - 1; i >= 0; i--) bakipos[i] = *curipos[i];
	for (int i = animatecnt - 1; i >= 0; i--)
		setinterpolation(animateptr(i));
}


FSerializer& Serialize(FSerializer& arc, const char* keyname, animwalltype& w, animwalltype* def)
{
	if (arc.BeginObject(keyname))
	{
	  arc("wallnum", w.wallnum)
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
		arc("posx", w.posx)
			("posy", w.posy)
			("posz", w.posz)
			("q16ang", w.q16ang)
			("q16horiz", w.q16horiz)
			("q16horizoff", w.q16horizoff)
			("q16rotscrnang", w.q16rotscrnang)
			("q16look_ang", w.q16look_ang)
			("one_eighty_count", w.one_eighty_count)
			("gotweapon", w.gotweapon)
			("palette", w.palette)
			("pals", w.pals)
			("fricx", w.fric.x)
			("fricy", w.fric.y)
			("zoom", w.zoom)
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
			("auto_aim", w.auto_aim)
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
			("newowner", w.newowner)
			("hurt_delay", w.hurt_delay)
			("hbomb_hold_delay", w.hbomb_hold_delay)
			("jumping_counter", w.jumping_counter)
			("airleft", w.airleft)
			("knee_incs", w.knee_incs)
			("access_incs", w.access_incs)
			("ftq", w.ftq)
			("access_wallnum", w.access_wallnum)
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
			("gm", w.gm)
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
			("return_to_center", w.return_to_center)
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
			("crouch_toggle", w.crouch_toggle)
			.EndObject();

		w.invdisptime = 0;
		w.oq16ang = w.q16ang;
		w.oq16horiz = w.q16horiz;
		w.oq16horizoff = w.q16horizoff;
		w.oq16rotscrnang = w.q16rotscrnang;
		w.oposx = w.posx;
		w.oposy = w.posy;
		w.oposz = w.posz;
		w.opyoff = w.pyoff;
		w.oweapon_sway = w.weapon_sway;
		w.oweapon_pos = w.weapon_pos;
		w.okickback_pic = w.kickback_pic;
		w.orandom_club_frame = w.random_club_frame;
		w.ohard_landing = w.hard_landing;
		w.horizAdjust = 0;
		w.angAdjust = 0;
		w.pitchAdjust = 0;
		w.lookLeft = false;
		w.lookRight = false;
	}
	return arc;
}

FSerializer& Serialize(FSerializer& arc, const char* keyname, weaponhit& w, weaponhit* def)
{
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
			("bposx", w.bposx, def->bposx)
			("bposy", w.bposy, def->bposy)
			("bposz", w.bposz, def->bposz)
			("aflags", w.aflags, def->aflags)
			.Array("temp_data", w.temp_data, def->temp_data, 6)
			.EndObject();
	}
	return arc;
}


void GameInterface::SerializeGameState(FSerializer& arc)
{
	if (arc.isReading())
	{
		memset(hittype, 0, sizeof(hittype));
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
		if (ud.multimode > 1) arc.Array("frags", &frags[0][0], MAXPLAYERS * MAXPLAYERS);

		// Here we must only save the used entries, otherwise the savegame would get too large.
		weaponhit def = {};
		if (arc.isWriting())
		{
			if (arc.BeginArray("weaponhit"))
			{
				// Save this in a way that's easy to read out again. RapidJSON sucks at iterating over objects. :(
				for (int i = 0; i < MAXSPRITES; i++)
				{
					if (sprite[i].statnum != MAXSTATUS)
					{
						arc(nullptr, i);
						arc(nullptr, hittype[i], def);
					}
				}
			}
			arc.EndArray();
		}
		else
		{
			if (arc.BeginArray("weaponhit"))
			{
				auto s = arc.ArraySize()/2;
				for (unsigned i = 0; i < s; i++)
				{
					int ii;
					arc(nullptr, ii);
					arc(nullptr, hittype[ii], def);
				}
				arc.EndArray();
			}
		}


		arc("skill", ud.player_skill)

			("from_bonus", ud.from_bonus)
			("secretlevel", ud.secretlevel)
			("respawn_monsters", ud.respawn_monsters)
			("respawn_items", ud.respawn_items)
			("respawn_inventory", ud.respawn_inventory)
			("god", ud.god)
			//("auto_run", ud.auto_run)
			("monsters_off", ud.monsters_off)
			("last_level", ud.last_level)
			("eog", ud.eog)
			("coop", ud.coop)
			("marker", ud.marker)
			("ffire", ud.ffire)
			("levelclock", ud.levelclock)

			.Array("sectorextra", sectorextra, numsectors)
			("rtsplaying", rtsplaying)
			("tempwallptr", tempwallptr)
			("sound445done", sound445done)
			("leveltexttime", levelTextTime)
			.Array("players", ps, ud.multimode)
			("spriteqamount", spriteqamount)
			.Array("shadedsector", shadedsector, numsectors)
			("lastvisinc", lastvisinc)
			("numanimwalls", numanimwalls)
			.Array("animwall", animwall, numanimwalls)
			("camsprite", camsprite)
			("earthquaketime", earthquaketime)
			("freezerhurtowner", freezerhurtowner)
			("global_random", global_random)
			("impact_damage", impact_damage)
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
			.Array("cyclers", &cyclers[0][0], 6 * numcyclers)
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
			ps[myconnectindex].gm = MODE_GAME;
			gamestate = GS_LEVEL;
			ud.recstat = 0;

			ud.m_player_skill = ud.player_skill;
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
			setpal(&ps[myconnectindex]);

			memset(gotpic, 0, sizeof(gotpic));
			if (isRR()) cacheit_r(); else cacheit_d();

			Mus_ResumeSaved();
			Mus_SetPaused(false);

			FX_SetReverb(0);
			recreateinterpolations();
			show_shareware = 0;
			everyothertime = 0;

			// should be unnecessary with the sounds getting serialized as well.
			#if 0
			if (ps[myconnectindex].jetpack_on)
				spritesound(DUKE_JETPACK_IDLE, ps[myconnectindex].i);

				// Update sound state in SFX sprites.
			for (int i = headspritestat[STAT_FX]; i >= 0; i = nextspritestat[i])
				if (sprite[i].picnum == MUSICANDSFX)
				{
					hittype[i].temp_data[1] = SoundEnabled();
					hittype[i].temp_data[0] = 0;
				}

			#endif
			FX_SetReverb(0);

		}
	}
}

END_DUKE_NS
