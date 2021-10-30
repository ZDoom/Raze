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

//void TimerFunc(task * Task);
BEGIN_SW_NS

// This cannot have a namespace declaration
#include "saveable.h"

/*
//////////////////////////////////////////////////////////////////////////////
TO DO


//////////////////////////////////////////////////////////////////////////////
*/

void InitLevelGlobals(void);

extern int lastUpdate;
extern char SaveGameDescr[10][80];
extern short Bunny_Count;
extern bool NewGame;
extern int GodMode;
extern int FinishTimer;
extern int FinishAnim;
extern int GameVersion;
//extern short Zombies;

extern bool bosswasseen[3];
extern short BossSpriteNum[3];

#define ANIM_SAVE 1

extern STATE s_NotRestored[];




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

FSerializer& Serialize(FSerializer& arc, const char* keyname, PANEL_STATEp& w, PANEL_STATEp* def)
{
	return SerializeDataPtr(arc, keyname, *(void**)&w, sizeof(PANEL_STATE));
}

FSerializer& Serialize(FSerializer& arc, const char* keyname, STATEp& w, STATEp* def)
{
	return SerializeDataPtr(arc, keyname, *(void**)&w, sizeof(STATE));
}

FSerializer& Serialize(FSerializer& arc, const char* keyname, STATEp*& w, STATEp** def)
{
	return SerializeDataPtr(arc, keyname, *(void**)&w, sizeof(STATEp));
}

FSerializer& Serialize(FSerializer& arc, const char* keyname, ACTOR_ACTION_SETp& w, ACTOR_ACTION_SETp* def)
{
	return SerializeDataPtr(arc, keyname, *(void**)&w, sizeof(ACTOR_ACTION_SET));
}

FSerializer& Serialize(FSerializer& arc, const char* keyname, PERSONALITYp& w, PERSONALITYp* def)
{
	return SerializeDataPtr(arc, keyname, *(void**)&w, sizeof(PERSONALITY));
}

FSerializer& Serialize(FSerializer& arc, const char* keyname, ATTRIBUTEp& w, ATTRIBUTEp* def)
{
	return SerializeDataPtr(arc, keyname, *(void**)&w, sizeof(ATTRIBUTE));
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

// Temporary array to serialize the panel sprites.
static TArray<PANEL_SPRITEp> pspAsArray;

FSerializer& Serialize(FSerializer& arc, const char* keyname, PANEL_SPRITEp& w, PANEL_SPRITEp* def)
{
	unsigned idx = ~0u;
	if (arc.isWriting())
	{
		if (w != nullptr) 
		{
			idx = pspAsArray.Find(w);
			if ((unsigned)idx >= pspAsArray.Size())
			{
				for (unsigned i = 0; i < MAX_SW_PLAYERS_REG; i++)
				{
					// special case for pointing to the list head
					if ((LIST)w == (LIST)&Player[i].PanelSpriteList)
					{
						idx = 1000'0000 + i;
						break;
					}
				}
				if (idx >= pspAsArray.Size() && idx < 1000'0000)
					idx = pspAsArray.Push(w);
			}
		}
		arc(keyname, idx);
	}
	else
	{
		unsigned int ndx;
		arc(keyname, ndx);

		if (ndx == ~0u) w = nullptr;
		else if (ndx >= 1000'0000) w = (PANEL_SPRITEp)&Player[ndx - 1000'0000].PanelSpriteList;
		else if ((unsigned)ndx >= pspAsArray.Size())
			I_Error("Bad panel sprite index in savegame");
		else w = pspAsArray[ndx];
	}
	return arc;
}

//---------------------------------------------------------------------------
//
// we need to allocate the needed panel sprites before loading anything else
//
//---------------------------------------------------------------------------

void preSerializePanelSprites(FSerializer& arc)
{
	if (arc.isReading())
	{
		unsigned siz;
		arc("panelcount", siz);
		pspAsArray.Resize(siz);
		for (unsigned i = 0; i < siz; i++)
		{
			pspAsArray[i] = (PANEL_SPRITEp)CallocMem(sizeof(PANEL_SPRITE), 1);
		}
	}
}

void postSerializePanelSprites(FSerializer& arc)
{
	if (arc.BeginArray("panelsprites"))
	{
		for(unsigned i = 0; i < pspAsArray.Size(); i++)
		{
			arc(nullptr, *pspAsArray[i]);
		}
		arc.EndArray();
	}
	if (arc.isWriting())
	{
		unsigned siz = pspAsArray.Size();
		arc("panelcount", siz);
	}
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

FSerializer& Serialize(FSerializer& arc, const char* keyname, PANEL_SPRITEstruct& w, PANEL_SPRITEstruct* def)
{
	static PANEL_SPRITEstruct nul;
	if (!def)
	{
		def = &nul;
		if (arc.isReading()) w = {};
	}

	if (arc.BeginObject(keyname))
	{
		arc("Next", w.Next)
			("Prev", w.Prev)
			("sibling", w.sibling)
			("State", w.State)
			("RetractState", w.RetractState)
			("PresentState", w.PresentState)
			("ActionState", w.ActionState)
			("RestState", w.RestState)
			("ox", w.ox)
			("oy", w.oy)
			("x", w.x)
			("y", w.y)
			.Array("over", w.over, countof(w.over))
			("id", w.ID)
			("picndx", w.picndx)
			("picnum", w.picnum)
			("vel", w.vel)
			("vel_adj", w.vel_adj)
			("xorig", w.xorig)
			("yorig", w.yorig)
			("flags", w.flags)
			("priority", w.priority)
			("scale", w.scale)
			("jump_speed", w.jump_speed)
			("jump_grav", w.jump_grav)
			("xspeed", w.xspeed)
			("tics", w.tics)
			("delay", w.delay)
			("ang", w.ang)
			("rotate_ang", w.rotate_ang)
			("sin_ndx", w.sin_ndx)
			("sin_amt", w.sin_amt)
			("sin_arc_speed", w.sin_arc_speed)
			("bob_height_divider", w.bob_height_divider)
			("shade", w.shade)
			("pal", w.pal)
			("kill_tics", w.kill_tics)
			("WeaponType", w.WeaponType)
			("playerp", w.PlayerP);

		SerializeCodePtr(arc, "PanelSpriteFunc", (void**)&w.PanelSpriteFunc);

		arc.EndObject();
	}
	if (arc.isReading())
	{
		w.ox = w.x;
		w.oy = w.y;
	}
	return arc;
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
		arc("cursectnum", w.cursectnum)
			("lastcursectnum", w.lastcursectnum)
			("pang", w.pang)
			("xvect", w.xvect)
			("yvect", w.yvect)
			("slide_xvect", w.slide_xvect)
			("slide_yvect", w.slide_yvect)
			("x", w.posx)
			("y", w.posy)
			("z", w.posz)
			("sop_control", w.sop_control)
			.EndObject();
	}
	if (arc.isReading())
	{
		w.oxvect = w.xvect;
		w.oyvect = w.yvect;
	}
	return arc;
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

FSerializer& Serialize(FSerializer& arc, const char* keyname, PLAYERp& w, PLAYERp* def)
{
	int ndx = w ? int(w - Player) : -1;
	arc(keyname, ndx);
	w = ndx == -1 ? nullptr : Player + ndx;
	return arc;
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

FSerializer& Serialize(FSerializer& arc, const char* keyname, PLAYERstruct& w, PLAYERstruct* def)
{
	if (arc.BeginObject(keyname))
	{
		arc("x", w.posx)
			("y", w.posy)
			("z", w.posz)
			("lv_sectnum", w.lv_sectnum)
			("lv_x", w.lv_x)
			("lv_y", w.lv_y)
			("lv_z", w.lv_z)
			("remote_sprite", w.remote_sprite)
			("remote", w.remote)
			("sop_remote", w.sop_remote)
			("sop", w.sop)
			("jump_count", w.jump_count)
			("jump_speed", w.jump_speed)
			("down_speed", w.down_speed)
			("up_speed", w.up_speed)
			("z_speed", w.z_speed)
			("climb_ndx", w.climb_ndx)
			("hiz", w.hiz)
			("loz", w.loz)
			("ceiling_dist", w.ceiling_dist)
			("floor_dist", w.floor_dist)
			("hi_sectp", w.hi_sectp)
			("lo_sectp", w.lo_sectp)
			("hi_sp", w.hi_sp)
			("lo_sp", w.lowActor)
			("last_camera_sp", w.last_camera_sp)
			("circle_camera_dist", w.circle_camera_dist)
			("six", w.six)
			("siy", w.siy)
			("siz", w.siz)
			("siang", w.siang)
			("xvect", w.xvect)
			("yvect", w.yvect)
			("friction", w.friction)
			("slide_xvect", w.slide_xvect)
			("slide_yvect", w.slide_yvect)
			("slide_ang", w.slide_ang)
			("slide_dec", w.slide_dec)
			("drive_avel", w.drive_avel)
			("view_outside_dang", w.view_outside_dang)
			("circle_camera_ang", w.circle_camera_ang)
			("camera_check_time_delay", w.camera_check_time_delay)
			("cursectnum", w.cursectnum)
			("lastcursectnum", w.lastcursectnum)
			("turn180_target", w.turn180_target)
			("hvel", w.hvel)
			("tilt", w.tilt)
			("tilt_dest", w.tilt_dest)
			("horizon", w.horizon)
			("angle", w.angle)
			("recoil_amt", w.recoil_amt)
			("recoil_speed", w.recoil_speed)
			("recoil_ndx", w.recoil_ndx)
			("recoil_horizoff", w.recoil_horizoff)
			("recoil_ohorizoff", w.recoil_ohorizoff)
			("oldposx", w.oldposx)
			("oldposy", w.oldposy)
			("oldposz", w.oldposz)
			("revolvex", w.RevolveX)
			("revolvey", w.RevolveY)
			("RevolveDeltaAng", w.RevolveDeltaAng)
			("RevolveAng", w.RevolveAng)
			("PlayerSprite", w.PlayerSprite)
			("PlayerUnderSprite", w.PlayerUnderSprite)
			("SpriteP", w.SpriteP)
			("UnderSpriteP", w.UnderSpriteP)
			("pnum", w.pnum)
			("LadderSector", w.LadderSector)
			("lx", w.lx)
			("ly", w.ly)
			("JumpDuration", w.JumpDuration)
			("WadeDepth", w.WadeDepth)
			("bob_amt", w.bob_amt)
			("bob_ndx", w.bob_ndx)
			("bcnt", w.bcnt)
			("bob_z", w.bob_z)
			("playerreadyflag", w.playerreadyflag)
			("Flags", w.Flags)
			("Flags2", w.Flags2)
			("sop_control", w.sop_control)
			("sop_riding", w.sop_riding)
			.Array("HasKey", w.HasKey, countof(w.HasKey))
			("SwordAng", w.SwordAng)
			("WpnGotOnceFlags", w.WpnGotOnceFlags)
			("WpnFlags", w.WpnFlags)
			.Array("WpnAmmo", w.WpnAmmo, countof(w.WpnAmmo))
			("WpnNum", w.WpnNum)
			("pnum", w.pnum)
			("panelnext", w.PanelSpriteList.Next)
			("panelprev", w.PanelSpriteList.Prev)
			("curwpn", w.CurWpn)
			.Array("wpn", w.Wpn, countof(w.Wpn))
			("WpnRocketType", w.WpnRocketType)
			("WpnRocketHeat", w.WpnRocketHeat)
			("WpnRocketNuke", w.WpnRocketNuke)
			("WpnFlameType", w.WpnFlameType)
			("WpnFirstType", w.WpnFirstType)
			("WeaponType", w.WeaponType)
			("FirePause", w.FirePause)
			("InventoryNum", w.InventoryNum)
			("InventoryBarTics", w.InventoryBarTics)
			.Array("InventoryTics", w.InventoryTics, countof(w.InventoryTics))
			.Array("InventoryPercent", w.InventoryPercent, countof(w.InventoryPercent))
			.Array("InventoryAmount", w.InventoryAmount, countof(w.InventoryAmount))
			.Array("InventoryActive", w.InventoryActive, countof(w.InventoryActive))
			("DiveTics", w.DiveTics)
			("DiveDamageTics", w.DiveDamageTics)
			("DeathType", w.DeathType)
			("Kills", w.Kills)
			("Killer", w.Killer)
			.Array("KilledPlayer", w.KilledPlayer, countof(w.KilledPlayer))
			("SecretsFound", w.SecretsFound)
			("Armor", w.Armor)
			("MaxHealth", w.MaxHealth)
			("UziShellLeftAlt", w.UziShellLeftAlt)
			("UziShellRightAlt", w.UziShellRightAlt)
			("TeamColor", w.TeamColor)
			("FadeTics", w.FadeTics)
			("FadeAmt", w.FadeAmt)
			("NightVision", w.NightVision)
			("IsAI", w.IsAI)
			("NumFootPrints", w.NumFootPrints)
			("WpnUziType", w.WpnUziType)
			("WpnShotgunType", w.WpnShotgunType)
			("WpnShotgunAuto", w.WpnShotgunAuto)
			("WpnShotgunLastShell", w.WpnShotgunLastShell)
			("WpnRailType", w.WpnRailType)
			("Bloody", w.Bloody)
			("InitingNuke", w.InitingNuke)
			("TestNukeInit", w.TestNukeInit)
			("NukeInitialized", w.NukeInitialized)
			("FistAng", w.FistAng)
			("WpnKungFuMove", w.WpnKungFuMove)
			("HitBy", w.HitBy)
			("Reverb", w.Reverb)
			("Heads", w.Heads)
			("PlayerVersion", w.PlayerVersion)
			("cookieTime", w.cookieTime)
			("WpnReloadState", w.WpnReloadState)
			("keypressbits", w.KeyPressBits)
			("chops", w.Chops);

		SerializeCodePtr(arc, "DoPlayerAction", (void**)&w.DoPlayerAction);
		arc.EndObject();
	}
	if (arc.isReading())
	{
		w.oposx = w.posx;
		w.oposy = w.posx;
		w.oposz = w.posx;
		w.oz_speed = w.z_speed;
		w.oxvect = w.xvect;
		w.oyvect = w.yvect;
		w.obob_z = w.bob_z;
		w.input = {};
		w.lastinput = {};
        memset(w.cookieQuote, 0, sizeof(w.cookieQuote)); // no need to remember this.
        w.StartColor = 0;

	}
	return arc;
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

FSerializer& Serialize(FSerializer& arc, const char* keyname, SECTOR_OBJECTp& w, SECTOR_OBJECTp* def)
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

FSerializer& Serialize(FSerializer& arc, const char* keyname, SECTOR_OBJECTstruct& w, SECTOR_OBJECTstruct* def)
{
	static SECTOR_OBJECTstruct nul;
	if (!def)
	{
		def = &nul;
		if (arc.isReading()) w = {};
	}
	if (arc.BeginObject(keyname))
	{
		int sp_cnt;
		for (sp_cnt = 0; w.sp_num[sp_cnt] != -1 && sp_cnt < (int)countof(w.sp_num); sp_cnt++) {}

		arc("num_sectors", w.num_sectors, def->num_sectors)
			("num_walls", w.num_walls, def->num_walls)
			("num_sp", sp_cnt)
			("clipbox_num", w.clipbox_num, def->clipbox_num)
			.Array("sectp", w.sectp, def->sectp, w.num_sectors)
			.Array("sector", w.sector, def->sector, w.num_sectors)  // is this really different from sectp?
			.Array("zorig_floor", w.zorig_floor, def->zorig_floor, w.num_sectors)
			.Array("zorig_ceiling", w.zorig_ceiling, def->zorig_ceiling, w.num_sectors)
			.Array("sp_num", w.sp_num, def->sp_num, countof(w.sp_num))
			.Array("xorig", w.xorig, def->xorig, w.num_walls)
			.Array("yorig", w.yorig, def->yorig, w.num_walls)
			("controller", w.controller, def->controller)
			("child", w.sp_child, def->sp_child)
			("xmid", w.xmid, def->xmid)
			("ymid", w.ymid, def->ymid)
			("zmid", w.zmid, def->zmid)
			("vel", w.vel, def->vel)
			("vel_tgt", w.vel_tgt, def->vel_tgt)
			("player_xoff", w.player_xoff, def->player_xoff)
			("player_yoff", w.player_yoff, def->player_yoff)
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
			("sectnum", w.sectnum, def->sectnum)
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
			("old_ang", w.old_ang, def->old_ang)
			("spin_speed", w.spin_speed, def->spin_speed)
			("spin_ang", w.spin_ang, def->spin_ang)
			("turn_speed", w.turn_speed, def->turn_speed)
			("bob_sine_ndx", w.bob_sine_ndx, def->bob_sine_ndx)
			("bob_speed", w.bob_speed, def->bob_speed)
			("op_main_sector", w.op_main_sector, def->op_main_sector)
			("save_vel", w.save_vel, def->save_vel)
			("save_spin_speed", w.save_spin_speed, def->save_spin_speed)
			("match_event", w.match_event, def->match_event)
			("match_event_sprite", w.match_event_sprite, def->match_event_sprite)
			("scale_type", w.scale_type, def->scale_type)
			("scale_active_type", w.scale_active_type, def->scale_active_type)
			("scale_dist", w.scale_dist, def->scale_dist)
			("scale_speed", w.scale_speed, def->scale_speed)
			("scale_dist_min", w.scale_dist_min, def->scale_dist_min)
			("scale_dist_max", w.scale_dist_max, def->scale_dist_max)
			("scale_rand_freq", w.scale_rand_freq, def->scale_rand_freq)
			.Array("clipbox_dist", w.clipbox_dist, def->clipbox_dist, w.clipbox_num)
			.Array("clipbox_xoff", w.clipbox_xoff, def->clipbox_xoff, w.clipbox_num)
			.Array("clipbox_yoff", w.clipbox_yoff, def->clipbox_yoff, w.clipbox_num)
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
			("morph_xoff", w.morph_xoff, def->morph_xoff)
			("morph_yoff", w.morph_yoff, def->morph_yoff)
			("limit_ang_center", w.limit_ang_center, def->limit_ang_center)
			("limit_ang_delta", w.limit_ang_delta, def->limit_ang_delta);

		SerializeCodePtr(arc, "preanimator", (void**)&w.PreMoveAnimator);
		SerializeCodePtr(arc, "postanimator", (void**)&w.PostMoveAnimator);
		SerializeCodePtr(arc, "animator", (void**)&w.Animator);

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
			("origx", w.origX)
			("origy", w.origY)
			.EndObject();
	}
	return arc;
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

FSerializer& Serialize(FSerializer& arc, const char* keyname, SECT_USER& w, SECT_USER* def)
{
	static SECT_USER nul;
	if (!def)
	{
		def = &nul;
		if (arc.isReading()) w = {};
	}
	if (arc.BeginObject(keyname))
	{
		arc("dist", w.dist, def->dist)
			("flags", w.flags, def->flags)
			("depth", w.depth_fixed, def->depth_fixed)
			("stag", w.stag, def->stag)
			("ang", w.ang, def->ang)
			("height", w.height, def->height)
			("speed", w.speed, def->speed)
			("damage", w.damage, def->damage)
			("number", w.number, def->number)
			("flags2", w.flags2, def->flags2)
			.EndObject();
	}
	return arc;
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void SerializeSectUser(FSerializer& arc)
{
	BitArray hitlist(numsectors);

	if (arc.isWriting())
	{
		for (int i = 0; i < numsectors; i++)
		{
			hitlist.Set(i, !!SectUser[i].Data());
		}
	}
	else
	{
		for (int i = 0; i < numsectors; i++)
		{
			SectUser[i].Clear();
		}
	}
	arc.SerializeMemory("sectusermap", hitlist.Storage().Data(), hitlist.Storage().Size());
	arc.SparseArray("sectuser", SectUser, numsectors, hitlist);
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
		arc("WallP", w.WallP, def->WallP)
			("State", w.State, def->State)
			("Rot", w.Rot, def->Rot)
			("StateStart", w.StateStart, def->StateStart)
			("StateEnd", w.StateEnd, def->StateEnd)
			("StateFallOverride", w.StateFallOverride, def->StateFallOverride)
			("ActorActionSet", w.ActorActionSet, def->ActorActionSet)
			("Personality", w.Personality, def->Personality)
			("Attrib", w.Attrib, def->Attrib)
			("sop_parent", w.sop_parent, def->sop_parent)
			("flags", w.Flags, def->Flags)
			("flags2", w.Flags2, def->Flags2)
			("Tics", w.Tics, def->Tics)
			("RotNum", w.RotNum, def->RotNum)
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
			("hi_sp", w.hi_sp, def->hi_sp)
			("lo_sp", w.lowActor, def->lowActor)
			("active_range", w.active_range, def->active_range)
			("SpriteNum", w.SpriteNum, def->SpriteNum)
			("Attach", w.Attach, def->Attach)
			("SpriteP", w.SpriteP, def->SpriteP)
			("PlayerP", w.PlayerP, def->PlayerP)
			("Sibling", w.Sibling, def->Sibling)
			("xchange", w.xchange, def->xchange)
			("ychange", w.ychange, def->ychange)
			("zchange", w.zchange, def->zchange)
			("z_tgt", w.z_tgt, def->z_tgt)
			("vel_tgt", w.vel_tgt, def->vel_tgt)
			("vel_rate", w.vel_rate, def->vel_rate)
			("speed", w.speed, def->speed)
			("Counter", w.Counter, def->Counter)
			("Counter2", w.Counter2, def->Counter2)
			("Counter3", w.Counter3, def->Counter3)
			("DamageTics", w.DamageTics, def->DamageTics)
			("BladeDamageTics", w.BladeDamageTics, def->BladeDamageTics)
			("WpnGoal", w.WpnGoal, def->WpnGoal)
			("Radius", w.Radius, def->Radius)
			("OverlapZ", w.OverlapZ, def->OverlapZ)
			("flame", w.flame, def->flame)
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
			("sx", w.sx, def->sx)
			("sy", w.sy, def->sy)
			("sz", w.sz, def->sz)
			("sang", w.sang, def->sang)
			("spal", w.spal, def->spal)
			("ret", w.ret, def->ret)
			("Flag1", w.Flag1, def->Flag1)
			("LastWeaponNum", w.LastWeaponNum, def->LastWeaponNum)
			("WeaponNum", w.WeaponNum, def->WeaponNum)
			("bounce", w.bounce, def->bounce)
			("ShellNum", w.ShellNum, def->ShellNum)
			("FlagOwner", w.FlagOwner, def->FlagOwner)
			("Vis", w.Vis, def->Vis)
			("DidAlert", w.DidAlert, def->DidAlert)
			("filler", w.filler, def->filler)
			("wallshade", w.WallShade)
			("rotator", w.rotator)
			("oz", w.oz, def->oz);

		SerializeCodePtr(arc, "ActorActionFunc", (void**)&w.ActorActionFunc);
		arc.EndObject();

		if (arc.isReading())
		{
			w.oangdiff = 0;
		}
	}
	return arc;
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void SerializeUser(FSerializer& arc)
{
	FixedBitArray<MAXSPRITES> hitlist;

	if (arc.isWriting())
	{
		for (int i = 0; i < MAXSPRITES; i++)
		{
			hitlist.Set(i, !!User[i].Data());
		}
	}
	else
	{
		for (int i = 0; i < MAXSPRITES; i++)
		{
			User[i].Clear();
		}
	}
	arc("usermap", hitlist);
	arc.SparseArray("user", User, MAXSPRITES, hitlist);
}


//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

FSerializer& Serialize(FSerializer& arc, const char* keyname, SINE_WAVE_FLOOR& w, SINE_WAVE_FLOOR* def)
{
	static SINE_WAVE_FLOOR nul = { -1,-1,-1,-1,-1,-1,255 };
	if (!def)
	{
		def = &nul;
		if (arc.isReading()) w = nul;
	}

	if (arc.BeginObject(keyname))
	{
		arc("floor_origz", w.floor_origz, def->floor_origz)
			("ceiling_origz", w.ceiling_origz, def->ceiling_origz)
			("range", w.range, def->range)
			("sector", w.sector, def->sector)
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
	static SINE_WALL nul = { -1,-1,-1,-1,-1,-1 };
	if (!def)
	{
		def = &nul;
		if (arc.isReading()) w = nul;
	}

	if (arc.BeginObject(keyname))
	{
		arc("orig_xy", w.orig_xy, def->orig_xy)
			("range", w.range, def->range)
			("sector", w.wall, def->wall)
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
	static SPRING_BOARD nul = { -1,-1 };
	if (!def)
	{
		def = &nul;
		if (arc.isReading()) w = nul;
	}

	if (arc.BeginObject(keyname))
	{
		arc("sector", w.Sector, def->Sector)
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
		arc("mirrorwall", w.mirrorwall, def->mirrorwall)
			("mirrorsector", w.mirrorsector, def->mirrorsector)
			("camera", w.camera, def->camera)
			("camsprite", w.camsprite, def->camsprite)
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
		arc("x", w.x, def->x)
			("y", w.y, def->y)
			("z", w.z, def->z)
			("ang", w.ang, def->ang)
			("tag_low", w.tag_low, def->tag_low)
			("tag_high", w.tag_high, def->tag_high)
			("filler", w.filler, def->filler)
			.EndObject();
	}
	return arc;
}

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

void GameInterface::SerializeGameState(FSerializer& arc)
{
	pspAsArray.Clear();
    Saveable_Init();

    if (arc.BeginObject("state"))
    {
        preSerializePanelSprites(arc);
        SerializeUser(arc);
		SerializeSectUser(arc);
		so_serializeinterpolations(arc);
		arc("numplayers", numplayers)
			.Array("players", Player, numplayers)
			("skill", Skill)
			("screenpeek", screenpeek)
			("randomseed", randomseed)
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
			("TotalKillable", TotalKillable)
			("net", gNet)
			("gs", gs)
			("LevelSecrets", LevelSecrets)
			("Bunny_Count", Bunny_Count)
			("GodMode", GodMode)
			("FinishTimer", FinishTimer)
			("FinishAnim", FinishAnim)
			.Array("bosswasseen", bosswasseen, 3)
			.Array("BossSpriteNum", BossSpriteNum, 3);
			arc.Array("tracks", Track, countof(Track))
			("minenemyskill", MinEnemySkill)
            ;
        postSerializePanelSprites(arc);
        arc.EndObject();
    }

	if (arc.isReading())
    {
    DoTheCache();

        int SavePlayClock = PlayClock;
        InitTimingVars();
        PlayClock = SavePlayClock;
    InitNetVars();

    screenpeek = myconnectindex;

    Mus_ResumeSaved();
    if (snd_ambience)
        StartAmbientSound();

    // this is not a new game
    ShadowWarrior::NewGame = false;

		DoPlayerDivePalette(Player + myconnectindex);
		DoPlayerNightVisionPalette(Player + myconnectindex);
    InitLevelGlobals();
	}
}

END_SW_NS
