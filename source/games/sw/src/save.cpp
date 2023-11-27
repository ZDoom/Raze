//-------------------------------------------------------------------------
/*
Copyright (C) 1997, 2005 - 3D Realms Entertainment

This file is part of Shadow Warrior version 1.2

Shadow Warrior is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.

Original Source: 1997 - Frank Maddin and Jim Norwood
Prepared for public release: 03/28/2005 - Charlie Wiederhold, 3D Realms
*/
//-------------------------------------------------------------------------

#include "ns.h"

#define MAIN
#define QUIET
#include "build.h"

#include "names2.h"
#include "panel.h"
#include "game.h"
#include "tags.h"
#include "lists.h"
#include "interpolate.h"
#include "interpso.h"

#include "network.h"
#include "jsector.h"
#include "parent.h"

//#define FILE_TYPE 1

#include "weapon.h"
#include "misc.h"
#include "player.h"
#include "i_specialpaths.h"
#include "savegamehelp.h"
#include "raze_music.h"
#include "mapinfo.h"
#include "serializer_raze.h"

//void TimerFunc(task * Task);
BEGIN_SW_NS

// This cannot have a namespace declaration
#include "saveable.h"

extern int parallaxyscale_override, pskybits_override;

/*
//////////////////////////////////////////////////////////////////////////////
TO DO


//////////////////////////////////////////////////////////////////////////////
*/

void InitLevelGlobals(void);

extern int lastUpdate;
extern bool NewGame;
extern int GodMode;
extern int FinishTimer;
extern int FinishAnim;
extern int GameVersion;
//extern short Zombies;

extern bool bosswasseen[3];

#define ANIM_SAVE 1




//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

FSerializer& Serialize(FSerializer& arc, const char* keyname, savedcodesym& w, savedcodesym* def)
{
	if (arc.isWriting() && w.name.IsEmpty()) return arc;
	arc(keyname, w.name);
    return arc;
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

FSerializer& Serialize(FSerializer& arc, const char* keyname, saveddatasym& w, saveddatasym* def)
{
	if (arc.isWriting() && w.name.IsEmpty()) return arc;
	if (arc.BeginObject(keyname))
    {
        arc("name", w.name)
            ("offset", w.offset)
            .EndObject();
    }
    return arc;
}

//---------------------------------------------------------------------------
//
// todo: make sure all saveables are arrays so we can store indices instead of offsets
//
//---------------------------------------------------------------------------

FSerializer& SerializeDataPtr(FSerializer& arc, const char* keyname, void*& w, size_t sizeOf)
{
    saveddatasym sym;
    if (arc.isWriting())
    {
        Saveable_FindDataSym(w, &sym);
        arc(keyname, sym);
    }
    else
    {
        arc(keyname, sym);
        Saveable_RestoreDataSym(&sym, &w);
    }
    return arc;
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

FSerializer& SerializeCodePtr(FSerializer& arc, const char* keyname, void** w)
{
    savedcodesym sym;
    if (arc.isWriting())
    {
        Saveable_FindCodeSym(*w, &sym);
        arc(keyname, sym);
    }
    else
    {
        arc(keyname, sym);
        Saveable_RestoreCodeSym(&sym, w);
    }
    return arc;
}

//---------------------------------------------------------------------------
//
// Unfortunately this cannot be simplified with templates.
// This requires an explicit function for each pointer type.
//
//---------------------------------------------------------------------------

FSerializer& Serialize(FSerializer& arc, const char* keyname, PANEL_STATE*& w, PANEL_STATE** def)
{
	return SerializeDataPtr(arc, keyname, *(void**)&w, sizeof(PANEL_STATE));
}

FSerializer& _Serialize(FSerializer& arc, const char* keyname, FState*& w, FState** def)
{
	return SerializeDataPtr(arc, keyname, *(void**)&w, sizeof(FState));
}

FSerializer& _Serialize(FSerializer& arc, const char* keyname, FState**& w, FState*** def)
{
	return SerializeDataPtr(arc, keyname, *(void**)&w, sizeof(FState*));
}

FSerializer& Serialize(FSerializer& arc, const char* keyname, ACTOR_ACTION_SET*& w, ACTOR_ACTION_SET** def)
{
	return SerializeDataPtr(arc, keyname, *(void**)&w, sizeof(ACTOR_ACTION_SET));
}

FSerializer& Serialize(FSerializer& arc, const char* keyname, PERSONALITY*& w, PERSONALITY** def)
{
	return SerializeDataPtr(arc, keyname, *(void**)&w, sizeof(PERSONALITY));
}

FSerializer& Serialize(FSerializer& arc, const char* keyname, ATTRIBUTE*& w, ATTRIBUTE** def)
{
	return SerializeDataPtr(arc, keyname, *(void**)&w, sizeof(ATTRIBUTE));
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

FSerializer& Serialize(FSerializer& arc, const char* keyname, PANEL_SPRITE_OVERLAY& w, PANEL_SPRITE_OVERLAY* def)
{
	if (arc.BeginObject(keyname))
	{
		arc("state", w.State)
			("flags", w.flags)
			("tics", w.tics)
			("pic", w.pic)
			("xoff", w.xoff)
			("yoff", w.yoff)
			.EndObject();
	}
	return arc;
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void DPanelSprite::Serialize(FSerializer& arc)
{
	Super::Serialize(arc);
	arc("Next", Next)
		("Prev", Prev)
		("sibling", sibling)
		("State", State)
		("RetractState", RetractState)
		("PresentState", PresentState)
		("ActionState", ActionState)
		("RestState", RestState)
		("pos", pos)
		.Array("over", over, countof(over))
		("id", ID)
		.Sprite("sprite", Sprite, nullptr)
		("picnum", texnum)
		("vel", vel)
		("vel_adj", vel_adj)
		("orig", bobpos)
		("flags", flags)
		("priority", priority)
		("scale", scale)
		("jump_speed", jump_speed)
		("jump_grav", jump_grav)
		("xspeed", xspeed)
		("tics", tics)
		("delay", delay)
		("ang", ang)
		("rotate_ang", rotate_ang)
		("sin_ndx", sin_ndx)
		("sin_amt", sin_amt)
		("sin_arc_speed", sin_arc_speed)
		("bob_height_divider", bob_height_divider)
		("shade", shade)
		("pal", pal)
		("kill_tics", kill_tics)
		("WeaponType", WeaponType)
		("playerp", PlayerP);

	SerializeCodePtr(arc, "PanelSpriteFunc", (void**)&PanelSpriteFunc);
	if (arc.isReading())
	{
		opos = pos;
	}
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

FSerializer& Serialize(FSerializer& arc, const char* keyname, REMOTE_CONTROL& w, REMOTE_CONTROL* def)
{
	static REMOTE_CONTROL nul;
	if (!def)
	{
		def = &nul;
		if (arc.isReading()) w = {};
	}

	if (arc.BeginObject(keyname))
	{
		arc("cursectnum", w.cursectp)
			("lastcursectnum", w.lastcursectp)
			("pang", w.pang)
			("vect", w.vect)
			("slide_vect", w.slide_vect)
			("pos", w.pos)
			("sop_control", w.sop_control)
			.EndObject();
	}
	if (arc.isReading())
	{
		w.ovect = w.vect;
	}
	return arc;
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void DSWPlayer::Serialize(FSerializer& arc)
{
	Super::Serialize(arc);
	arc("lv_sectnum", lv_sector)
		("lv", lv)
		("remote_sprite", remoteActor)
		("remote", remote)
		("sop_remote", sop_remote)
		("sop", sop)
		("jump_count", jump_count)
		("jump_speed", jump_speed)
		("z_speed", z_speed)
		("climb_ndx", climb_ndx)
		("hiz", hiz)
		("loz", loz)
		("ceiling_dist", p_ceiling_dist)
		("floor_dist", p_floor_dist)
		("hi_sectp", hi_sectp)
		("lo_sectp", lo_sectp)
		("hi_sp", highActor)
		("lo_sp", lowActor)
		("last_camera_sp", last_camera_act)
		("circle_camera_dist", circle_camera_dist)
		("vect", vect)
		("friction", friction)
		("slide_vect", slide_vect)
		("slide_ang", slide_ang)
		("slide_dec", slide_dec)
		("circle_camera_ang", circle_camera_ang)
		("camera_check_time_delay", camera_check_time_delay)
		("cursectnum", cursector)
		("lastcursectnum", lastcursector)
		("recoil_amt", recoil_amt)
		("recoil_speed", recoil_speed)
		("recoil_ndx", recoil_ndx)
		("recoil_horizoff", recoil_horizoff)
		("recoil_ohorizoff", recoil_ohorizoff)
		("revolve", Revolve)
		("RevolveDeltaAng", RevolveDeltaAng)
		("RevolveAng", RevolveAng)
		("PlayerUnderSprite", PlayerUnderActor)
		("LadderSector", LadderSector)
		("ladderpos", LadderPosition)
		("JumpDuration", JumpDuration)
		("WadeDepth", WadeDepth)
		("bob_amt", pbob_amt)
		("bob_ndx", bob_ndx)
		("bcnt", bcnt)
		("bob_z", bob_z)
		("playerreadyflag", playerreadyflag)
		("Flags", Flags)
		("Flags2", Flags2)
		("sop_control", sop_control)
		("sop_riding", sop_riding)
		.Array("HasKey", HasKey, countof(HasKey))
		("SwordAng", SwordAng)
		("WpnGotOnceFlags", WpnGotOnceFlags)
		("WpnFlags", WpnFlags)
		.Array("WpnAmmo", WpnAmmo, countof(WpnAmmo))
		("WpnNum", WpnNum)
		("curwpn", CurWpn)
		.Array("wpn", Wpn, countof(Wpn))
		("WpnRocketType", WpnRocketType)
		("WpnRocketHeat", WpnRocketHeat)
		("WpnRocketNuke", WpnRocketNuke)
		("WpnFlameType", WpnFlameType)
		("WpnFirstType", WpnFirstType)
		("WeaponType", WeaponType)
		("FirePause", FirePause)
		("InventoryNum", InventoryNum)
		("InventoryBarTics", InventoryBarTics)
		.Array("InventoryTics", InventoryTics, countof(InventoryTics))
		.Array("InventoryPercent", InventoryPercent, countof(InventoryPercent))
		.Array("InventoryAmount", InventoryAmount, countof(InventoryAmount))
		.Array("InventoryActive", InventoryActive, countof(InventoryActive))
		("DiveTics", DiveTics)
		("DiveDamageTics", DiveDamageTics)
		("DeathType", DeathType)
		("Killer", KillerActor)
		.Array("KilledPlayer", KilledPlayer, countof(KilledPlayer))
		("Armor", Armor)
		("MaxHealth", MaxHealth)
		("UziShellLeftAlt", UziShellLeftAlt)
		("UziShellRightAlt", UziShellRightAlt)
		("TeamColor", TeamColor)
		("FadeTics", FadeTics)
		("FadeAmt", FadeAmt)
		("NightVision", NightVision)
		("IsAI", IsAI)
		("NumFootPrints", NumFootPrints)
		("WpnUziType", WpnUziType)
		("WpnShotgunType", WpnShotgunType)
		("WpnShotgunAuto", WpnShotgunAuto)
		("WpnShotgunLastShell", WpnShotgunLastShell)
		("WpnRailType", WpnRailType)
		("Bloody", Bloody)
		("InitingNuke", InitingNuke)
		("TestNukeInit", TestNukeInit)
		("NukeInitialized", NukeInitialized)
		("FistAng", FistAng)
		("WpnKungFuMove", WpnKungFuMove)
		("HitBy", HitBy)
		("Reverb", Reverb)
		("Heads", Heads)
		("PlayerVersion", PlayerVersion)
		("cookieTime", cookieTime)
		("WpnReloadState", WpnReloadState)
		("keypressbits", KeyPressBits)
		("PanelSpriteList", PanelSpriteList)
		("chops", Chops);

	SerializeCodePtr(arc, "DoPlayerAction", (void**)&DoPlayerAction);
	if (arc.isReading())
	{
		ovect = vect;
		obob_z = bob_z;
        memset(cookieQuote, 0, sizeof(cookieQuote)); // no need to remember this.
        StartColor = 0;
	}
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

FSerializer& Serialize(FSerializer& arc, const char* keyname, SECTOR_OBJECT*& w, SECTOR_OBJECT** def)
{
	int ndx = w ? int(w - SectorObject) : -1;
	arc(keyname, ndx);
	if (arc.isReading() )w = ndx == -1 ? nullptr : SectorObject + ndx;
	return arc;
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

FSerializer& Serialize(FSerializer& arc, const char* keyname, SECTOR_OBJECT& w, SECTOR_OBJECT* def)
{
	static SECTOR_OBJECT nul;
	if (!def)
	{
		def = &nul;
		if (arc.isReading()) w = {};
	}
	if (arc.BeginObject(keyname))
	{
		arc("num_sectors", w.num_sectors, def->num_sectors)
			("num_walls", w.num_walls, def->num_walls)
			("clipbox_num", w.clipbox_num, def->clipbox_num)
			.Array("sectp", w.sectp, def->sectp, w.num_sectors)
			.Array("zorig_floor", w.zorig_floor, def->zorig_floor, w.num_sectors)
			.Array("sp_num", w.so_actors, def->so_actors, countof(w.so_actors))
			.Array("orig", w.orig, def->orig, w.num_walls)
			("controller", w.controller, def->controller)
			("child", w.sp_child, def->sp_child)
			("mid", w.pmid, def->pmid)
			("vel", w.vel, def->vel)
			("vel_tgt", w.vel_tgt, def->vel_tgt)
			("zdelta", w.zdelta, def->zdelta)

			("z_tgt", w.z_tgt, def->z_tgt)
			("z_rate", w.z_rate, def->z_rate)
			("update", w.update, def->update)
			("bob_diff", w.bob_diff, def->bob_diff)
			("target_dist", w.target_dist, def->target_dist)
			("floor_loz", w.floor_loz, def->floor_loz)
			("floor_hiz", w.floor_hiz, def->floor_hiz)
			("morph_z", w.morph_z, def->morph_z)
			("morph_z_min", w.morph_z_min, def->morph_z_min)
			("morph_z_max", w.morph_z_max, def->morph_z_max)
			("bob_amt", w.bob_amt, def->bob_amt)
			("drive_angspeed", w.drive_angspeed, def->drive_angspeed)
			("drive_angslide", w.drive_angslide, def->drive_angslide)
			("drive_speed", w.drive_speed, def->drive_speed)
			("drive_slide", w.drive_slide, def->drive_slide)
			("crush_z", w.crush_z, def->crush_z)
			("flags", w.flags, def->flags)
			("mid_sector", w.mid_sector, def->mid_sector)
			("max_damage", w.max_damage, def->max_damage)
			("ram_damage", w.ram_damage, def->ram_damage)
			("wait_tics", w.wait_tics, def->wait_tics)
			("track", w.track, def->track)
			("point", w.point, def->point)
			("vel_rate", w.vel_rate, def->vel_rate)
			("dir", w.dir, def->dir)
			("ang", w.ang, def->ang)
			("ang_moving", w.ang_moving, def->ang_moving)
			("clipdist", w.clipdist, def->clipdist)
			("ang_tgt", w.ang_tgt, def->ang_tgt)
			("ang_orig", w.ang_orig, def->ang_orig)
			("last_ang", w.last_ang, def->last_ang)
			("spin_speed", w.spin_speed, def->spin_speed)
			("spin_ang", w.spin_ang, def->spin_ang)
			("turn_speed", w.turn_speed, def->turn_speed)
			("bob_sine_ndx", w.bob_sine_ndx, def->bob_sine_ndx)
			("bob_speed", w.bob_speed, def->bob_speed)
			("op_main_sector", w.op_main_sector, def->op_main_sector)
			("save_vel", w.save_vel, def->save_vel)
			("save_spin_speed", w.save_spin_speed, def->save_spin_speed)
			("match_event", w.match_event, def->match_event)
			("match_event_sprite", w.match_event_actor, def->match_event_actor)
			("scale_type", w.scale_type, def->scale_type)
			("scale_active_type", w.scale_active_type, def->scale_active_type)
			("scale_dist", w.scale_dist, def->scale_dist)
			("scale_speed", w.scale_speed, def->scale_speed)
			("scale_dist_min", w.scale_dist_min, def->scale_dist_min)
			("scale_dist_max", w.scale_dist_max, def->scale_dist_max)
			("scale_rand_freq", w.scale_rand_freq, def->scale_rand_freq)
			.Array("clipbox_dist", w.clipbox_dist, def->clipbox_dist, w.clipbox_num)
			.Array("clipbox_ang", w.clipbox_ang, def->clipbox_ang, w.clipbox_num)
			.Array("clipbox_vdist", w.clipbox_vdist, def->clipbox_vdist, w.clipbox_num)
			.Array("scale_point_dist", w.scale_point_dist, def->scale_point_dist, MAX_SO_POINTS)
			.Array("scale_point_speed", w.scale_point_speed, def->scale_point_speed, MAX_SO_POINTS)
			("scale_point_base_speed", w.scale_point_base_speed, def->scale_point_base_speed)
			("scale_point_dist_min", w.scale_point_dist_min, def->scale_point_dist_min)
			("scale_point_dist_max", w.scale_point_dist_max, def->scale_point_dist_max)
			("scale_point_rand_freq", w.scale_point_rand_freq, def->scale_point_rand_freq)
			("scale_x_mult", w.scale_x_mult, def->scale_x_mult)
			("scale_y_mult", w.scale_y_mult, def->scale_y_mult)
			("morph_wall_point", w.morph_wall_point, def->morph_wall_point)
			("morph_ang", w.morph_ang, def->morph_ang)
			("morph_speed", w.morph_speed, def->morph_speed)
			("morph_dist_max", w.morph_dist_max, def->morph_dist_max)
			("morph_rand_freq", w.morph_rand_freq, def->morph_rand_freq)
			("morph_dist", w.morph_dist, def->morph_dist)
			("morph_z_speed", w.morph_z_speed, def->morph_z_speed)
			("morph_off", w.morph_off, def->morph_off)
			("limit_ang_center", w.limit_ang_center, def->limit_ang_center)
			("limit_ang_delta", w.limit_ang_delta, def->limit_ang_delta)
			("premovescale", w.PreMoveScale, def->PreMoveScale)
			("animtype", w.AnimType, def->AnimType);

		arc.EndObject();
	}
	return arc;
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

FSerializer& Serialize(FSerializer& arc, const char* keyname, ROTATOR& w, ROTATOR* def)
{
	if (arc.BeginObject(keyname))
	{
		arc("pos", w.pos)
			("open_dest", w.open_dest)
			("tgt", w.tgt)
			("speed", w.speed)
			("orig_speed", w.orig_speed)
			("vel", w.vel)
			("orig", w.orig)
			.EndObject();
	}
	return arc;
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------
static USER nuluser; // must be outside the function to evade C++'s retarded initialization rules for static function variables.

FSerializer& Serialize(FSerializer& arc, const char* keyname, USER& w, USER* def)
{
	if (!def)
	{
		def = &nuluser;
		if (arc.isReading()) w.Clear();
	}
	if (arc.BeginObject(keyname))
	{
		// The default serializer cannot handle the statically defined states so call these functons explicitly.
		_Serialize(arc, "State", w.State, &def->State);
		_Serialize(arc, "Rot", w.__legacyState.Rot, &def->__legacyState.Rot);
		_Serialize(arc, "StateStart", w.__legacyState.StateStart, &def->__legacyState.StateStart);
		_Serialize(arc, "StateEnd", w.__legacyState.StateEnd, &def->__legacyState.StateEnd);
		_Serialize(arc, "StateFallOverride", w.__legacyState.StateFallOverride, &def->__legacyState.StateFallOverride);
		/*
			("State", w.State, def->State)
			("Rot", w.__legacyState.Rot, def->__legacyState.Rot)
			("StateStart", w.__legacyState.StateStart, def->__legacyState.StateStart)
			("StateEnd", w.__legacyState.StateEnd, def->__legacyState.StateEnd)
			("StateFallOverride", w.__legacyState.StateFallOverride, def->__legacyState.StateFallOverride)
			*/
		arc
			("WallP", w.WallP, def->WallP)
			("ActorActionSet", w.__legacyState.ActorActionSet, def->__legacyState.ActorActionSet)
			("Personality", w.Personality, def->Personality)
			("Attrib", w.__legacyState.Attrib, def->__legacyState.Attrib)
			("sop_parent", w.sop_parent, def->sop_parent)
			("flags", w.Flags, def->Flags)
			("flags2", w.Flags2, def->Flags2)
			("Tics", w.Tics, def->Tics)
			("ID", w.ID, def->ID)
			("Health", w.Health, def->Health)
			("MaxHealth", w.MaxHealth, def->MaxHealth)
			("LastDamage", w.LastDamage, def->LastDamage)
			("PainThreshold", w.PainThreshold, def->PainThreshold)
			("jump_speed", w.jump_speed, def->jump_speed)
			("jump_grav", w.jump_grav, def->jump_grav)
			("ceiling_dist", w.ceiling_dist, def->ceiling_dist)
			("floor_dist", w.floor_dist, def->floor_dist)
			("lo_step", w.lo_step, def->lo_step)
			("hiz", w.hiz, def->hiz)
			("loz", w.loz, def->loz)
			("zclip", w.zclip, def->zclip)
			("hi_sectp", w.hi_sectp, def->hi_sectp)
			("lo_sectp", w.lo_sectp, def->lo_sectp)
			("hi_sp", w.highActor, def->highActor)
			("lo_sp", w.lowActor, def->lowActor)
			("active_range", w.active_range, def->active_range)
			("Attach", w.attachActor, def->attachActor);
		arc("PlayerP", w.PlayerP, def->PlayerP);
			arc("Sibling", w.Sibling, def->Sibling)
			("change", w.change, def->change)
			("z_tgt", w.z_tgt, def->z_tgt)
			("vel_tgt", w.vel_tgt, def->vel_tgt)
			("vel_rate", w.vel_rate, def->vel_rate)
			("speed", w.speed, def->speed)
			("Counter", w.Counter, def->Counter)
			("Counter2", w.Counter2, def->Counter2)
			("Counter3", w.Counter3, def->Counter3)
			("DamageTics", w.DamageTics, def->DamageTics)
			("BladeDamageTics", w.BladeDamageTics, def->BladeDamageTics)
			("WpnGoal", w.WpnGoalActor, def->WpnGoalActor)
			("Radius", w.Radius, def->Radius)
			("OverlapZ", w.OverlapZ, def->OverlapZ)
			("flame", w.flameActor, def->flameActor)
			("tgt_sp", w.targetActor, def->targetActor)
			("scale_speed", w.scale_speed, def->scale_speed)
			("scale_value", w.scale_value, def->scale_value)
			("scale_tgt", w.scale_tgt, def->scale_tgt)
			("DistCheck", w.DistCheck, def->DistCheck)
			("Dist", w.Dist, def->Dist)
			("TargetDist", w.TargetDist, def->TargetDist)
			("WaitTics", w.WaitTics, def->WaitTics)
			("track", w.track, def->track)
			("point", w.point, def->point)
			("track_dir", w.track_dir, def->track_dir)
			("track_vel", w.track_vel, def->track_vel)
			("slide_ang", w.slide_ang, def->slide_ang)
			("slide_vel", w.slide_vel, def->slide_vel)
			("slide_dec", w.slide_dec, def->slide_dec)
			("motion_blur_dist", w.motion_blur_dist, def->motion_blur_dist)
			("motion_blur_num", w.motion_blur_num, def->motion_blur_num)
			("wait_active_check", w.wait_active_check, def->wait_active_check)
			("inactive_time", w.inactive_time, def->inactive_time)
			("pos", w.pos, def->pos)
			("sang", w.sang, def->sang)
			("spal", w.spal, def->spal)
			//("ret", w.coll, def->coll) // is this needed?
			("Flag1", w.Flag1, def->Flag1)
			("LastWeaponNum", w.LastWeaponNum, def->LastWeaponNum)
			("WeaponNum", w.WeaponNum, def->WeaponNum)
			("bounce", w.bounce, def->bounce)
			("ShellNum", w.ShellNum, def->ShellNum)
			("FlagOwner", w.FlagOwner, def->FlagOwner)
			("FlagOwnerActor", w.flagOwnerActor, def->flagOwnerActor)
			("Vis", w.Vis, def->Vis)
			("DidAlert", w.DidAlert, def->DidAlert)
			("filler", w.filler, def->filler)
			("wallshade", w.WallShade)
			("rotator", w.rotator)
			("actoractionfunc", w.ActorActionFunc)
			("oz", w.oz, def->oz);

		arc.EndObject();

		if (arc.isReading())
		{
			w.oangdiff = nullAngle;
		}
	}
	return arc;
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

FSerializer& Serialize(FSerializer& arc, const char* keyname, SINE_WAVE_FLOOR& w, SINE_WAVE_FLOOR* def)
{
	static SINE_WAVE_FLOOR nul = { nullptr,-1,-1,-1,-1,-1,255 };
	if (!def)
	{
		def = &nul;
		if (arc.isReading()) w = nul;
	}

	if (arc.BeginObject(keyname))
	{
		arc("floor_origz", w.floorOrigz, def->floorOrigz)
			("ceiling_origz", w.ceilingOrigz, def->ceilingOrigz)
			("range", w.Range, def->Range)
			("sector", w.sectp, def->sectp)
			("sintable_ndx", w.sintable_ndx, def->sintable_ndx)
			("speed_shift", w.speed_shift, def->speed_shift)
			("flags", w.flags, def->flags)
			.EndObject();
	}
	return arc;
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

FSerializer& Serialize(FSerializer& arc, const char* keyname, SINE_WALL& w, SINE_WALL* def)
{
	static SINE_WALL nul = { nullptr,-1,-1,-1,-1,-1 };
	if (!def)
	{
		def = &nul;
		if (arc.isReading()) w = nul;
	}

	if (arc.BeginObject(keyname))
	{
		arc("orig_xy", w.origXY, def->origXY)
			("range", w.Range, def->Range)
			("sector", w.wallp, def->wallp)
			("sintable_ndx", w.sintable_ndx, def->sintable_ndx)
			("speed_shift", w.speed_shift, def->speed_shift)
			("flags", w.type, def->type)
			.EndObject();
	}
	return arc;
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

FSerializer& Serialize(FSerializer& arc, const char* keyname, SPRING_BOARD& w, SPRING_BOARD* def)
{
	static SPRING_BOARD nul = { nullptr,-1 };
	if (!def)
	{
		def = &nul;
		if (arc.isReading()) w = nul;
	}

	if (arc.BeginObject(keyname))
	{
		arc("sector", w.sectp, def->sectp)
			("timeout", w.TimeOut, def->TimeOut)
			.EndObject();
	}
	return arc;
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

FSerializer& Serialize(FSerializer& arc, const char* keyname, MIRRORTYPE& w, MIRRORTYPE* def)
{
	static MIRRORTYPE nul;
	if (!def)
	{
		def = &nul;
		if (arc.isReading()) w = {};
	}
	if (arc.BeginObject(keyname))
	{
		arc("mirrorwall", w.mirrorWall, def->mirrorWall)
			("mirrorsector", w.mirrorSector, def->mirrorSector)
			("camera", w.cameraActor, def->cameraActor)
			("camsprite", w.camspriteActor, def->camspriteActor)
			("campic", w.campic, def->campic)
			("numspawnspots", w.numspawnspots, def->numspawnspots)
			.Array("spawnspots", w.spawnspots, def->spawnspots, w.numspawnspots)
			("ismagic", w.ismagic, def->ismagic)
			("mstate", w.mstate, def->mstate)
			("maxtics", w.maxtics, def->maxtics)
			("tics", w.tics, def->tics)
			.EndObject();
	}
	return arc;
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

FSerializer& Serialize(FSerializer& arc, const char* keyname, gNET& w, gNET* def)
{
	static gNET nul;
	if (!def)
	{
		def = &nul;
		if (arc.isReading()) w = {};
	}
	if (arc.BeginObject(keyname))
	{
		arc("KillLimit", w.KillLimit, def->KillLimit)
			("TimeLimit", w.TimeLimit, def->TimeLimit)
			("TimeLimitClock", w.TimeLimitClock, def->TimeLimitClock)
			("MultiGameType", w.MultiGameType, def->MultiGameType)
			("TeamPlay", w.TeamPlay, def->TeamPlay)
			("HurtTeammate", w.HurtTeammate, def->HurtTeammate)
			("SpawnMarkers", w.SpawnMarkers, def->SpawnMarkers)
			("AutoAim", w.AutoAim, def->AutoAim)
			("NoRespawn", w.NoRespawn, def->NoRespawn)
			("Nuke", w.Nuke, def->Nuke)
			.EndObject();
	}
	return arc;
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

FSerializer& Serialize(FSerializer& arc, const char* keyname, GAME_SET& w, GAME_SET* def)
{
	static GAME_SET nul;
	if (!def)
	{
		def = &nul;
		if (arc.isReading()) w = {};
	}
	if (arc.BeginObject(keyname))
	{
		arc("NetGameType", w.NetGameType, def->NetGameType)
			("NetMonsters", w.NetMonsters, def->NetMonsters)
			("NetHurtTeammate", w.NetHurtTeammate, def->NetHurtTeammate)
			("NetSpawnMarkers", w.NetSpawnMarkers, def->NetSpawnMarkers)
			("NetTeamPlay", w.NetTeamPlay, def->NetTeamPlay)
			("NetKillLimit", w.NetKillLimit, def->NetKillLimit)
			("NetTimeLimit", w.NetTimeLimit, def->NetTimeLimit)
			("NetColor", w.NetColor, def->NetColor)
			("Nuke", w.NetNuke, def->NetNuke)
			.EndObject();
	}
	return arc;
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

FSerializer& Serialize(FSerializer& arc, const char* keyname, TRACK_POINT& w, TRACK_POINT* def)
{
	static TRACK_POINT nul;
	if (!def)
	{
		def = &nul;
		if (arc.isReading()) w = {};
	}
	if (arc.BeginObject(keyname))
	{
		arc("pos", w.pos, def->pos)
			("angle", w.angle, def->angle)
			("tag_low", w.tag_low, def->tag_low)
			("tag_high", w.tag_high, def->tag_high)
			.EndObject();
	}
	return arc;
}


//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

FSerializer& Serialize(FSerializer& arc, const char* keyname, TRACK& w, TRACK* def)
{
	static int nul;
	if (!def)
	{
		if (arc.isReading()) w.flags = w.ttflags = 0;
	}
	if (arc.BeginObject(keyname))
	{
		arc("numpoints", w.NumPoints, nul)
			("flags", w.flags, nul)
			("ttflag", w.ttflags, nul);

		if (arc.isReading())
		{
			if (w.TrackPoint) FreeMem(w.TrackPoint);
			int size = w.NumPoints ? w.NumPoints : 1;
			w.TrackPoint = (TRACK_POINT*)CallocMem(sizeof(TRACK_POINT), size);
		}
		if (w.NumPoints > 0) arc.Array("points", w.TrackPoint, w.NumPoints);
		arc.EndObject();
	}
	return arc;
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void DSWActor::Serialize(FSerializer& arc)
{
	Super::Serialize(arc);
	arc("hasuser", hasUser)
		("tempwall", tempwall)
		("owner", ownerActor)
		("texparam", texparam)
		("texparam2", texparam2);
	if (hasUser) arc("user", user); // only write if defined.
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void GameInterface::SerializeGameState(FSerializer& arc)
{
    Saveable_Init();

	if (arc.BeginObject("state"))
	{
		so_serializeinterpolations(arc);
		arc("numplayers", numplayers)
			("skill", Skill)
			("screenpeek", screenpeek)
			.Array("sop", SectorObject, countof(SectorObject))
			.Array("swf", &SineWaveFloor[0][0], 6 * 21)
			.Array("sinewall", &SineWall[0][0], 10 * 64)
			.Array("springboard", SpringBoard, countof(SpringBoard))
			("NormalVisibility", NormalVisibility)
			("MoveSkip2", MoveSkip2)
			("MoveSkip4", MoveSkip4)
			("MoveSkip8", MoveSkip8)
			("mirrorcnt", mirrorcnt)
			.Array("mirror", mirror, mirrorcnt)
			("mirrorinview", mirrorinview)
			("StarQueueHead", StarQueueHead)
			.Array("StarQueue", StarQueue, countof(StarQueue))
			("HoleQueueHead", HoleQueueHead)
			.Array("HoleQueue", HoleQueue, countof(HoleQueue))
			("WallBloodQueueHead", WallBloodQueueHead)
			.Array("WallBloodQueue", WallBloodQueue, countof(WallBloodQueue))
			("FloorBloodQueueHead", FloorBloodQueueHead)
			.Array("FloorBloodQueue", FloorBloodQueue, countof(FloorBloodQueue))
			("GenericQueueHead", GenericQueueHead)
			.Array("GenericQueue", GenericQueue, countof(GenericQueue))
			("LoWangsQueueHead", LoWangsQueueHead)
			.Array("LoWangsQueue", LoWangsQueue, countof(LoWangsQueue))
			("PlayClock", PlayClock)
			("net", gNet)
			("gs", gs)
			("Bunny_Count", Bunny_Count)
			("GodMode", GodMode)
			("FinishTimer", FinishTimer)
			("FinishAnim", FinishAnim)
			.Array("bosswasseen", bosswasseen, 3)
			.Array("BossSpriteNum", BossSpriteNum, 3)
			.Array("tracks", Track, countof(Track))
			("minenemyskill", MinEnemySkill)
			("parallaxys", parallaxyscale_override)
			("pskybits", pskybits_override)
			;
		arc.EndObject();
	}

	if (arc.isReading())
	{
		DoTheCache();

		int SavePlayClock = PlayClock;
		InitTimingVars();
		PlayClock = SavePlayClock;
		defineSky(nullptr, pskybits_override, nullptr, 0, parallaxyscale_override / 8192.f);

		screenpeek = myconnectindex;

		Mus_ResumeSaved();
		if (snd_ambience)
			StartAmbientSound();

		// this is not a new game
		ShadowWarrior::NewGame = false;

		DoPlayerDivePalette(getPlayer(myconnectindex));
		DoPlayerNightVisionPalette(getPlayer(myconnectindex));
		InitLevelGlobals();
	}
}

END_SW_NS
