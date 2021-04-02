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

extern bool serpwasseen;
extern bool sumowasseen;
extern bool zillawasseen;
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
    static savedcodesym nul;
    if (!def)
    {
        def = &nul;
        if (arc.isReading()) w = {};
    }

    if (arc.BeginObject(keyname))
    {
        arc("module", w.module, def->module)
            ("index", w.index, def->index)
            .EndObject();
    }
    return arc;
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

FSerializer& Serialize(FSerializer& arc, const char* keyname, saveddatasym& w, saveddatasym* def)
{
    static saveddatasym nul;
    if (!def)
    {
        def = &nul;
        if (arc.isReading()) w = {};
    }

    if (arc.BeginObject(keyname))
    {
        arc("module", w.module, def->module)
            ("index", w.index, def->index)
            ("offset", w.offset, def->offset)
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
	if (arc.isWriting())
	{
		unsigned siz = pspAsArray.Size();
		arc("panelcount", siz);
	}
	if (arc.BeginArray("panelsprites"))
	{
		for (auto psp : pspAsArray)
		{
			arc(nullptr, *psp);
		}
		arc.EndArray();
	}
	pspAsArray.Clear();
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
			("xfract", w.xfract)
			("x", w.x)
			("yfract", w.yfract)
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
			("filler", w.filler)
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
			("lo_sp", w.lo_sp)
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
	w = ndx == -1 ? nullptr : SectorObject;
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
			("depth_fract", w.depth_fract, def->depth_fract)
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
	FixedBitArray<MAXSECTORS> hitlist;

	if (arc.isWriting())
	{
		for (int i = 0; i < MAXSECTORS; i++)
		{
			hitlist.Set(i, !!SectUser[i].Data());
		}
	}
	else
	{
		for (int i = 0; i < MAXSECTORS; i++)
		{
			SectUser[i].Clear();
		}
	}
	arc("sectusermap", hitlist);
	arc.SparseArray("sectuser", SectUser, MAXSECTORS, hitlist);
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
			("lo_sp", w.lo_sp, def->lo_sp)
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
			("tgt_sp", w.tgt_sp, def->tgt_sp)
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

void GameInterface::SerializeGameState(FSerializer& arc)
{
    Saveable_Init();

    if (arc.BeginObject("state"))
    {
        preSerializePanelSprites(arc);
        SerializeUser(arc);
		SerializeSectUser(arc);
		arc("numplayers", numplayers)
            .Array("players", Player, numplayers)
            ;
        postSerializePanelSprites(arc);
        arc.EndObject();
    }
}



int SaveSymDataInfo(MFILE_WRITE fil, void *ptr)
{
    saveddatasym sym;

    if (Saveable_FindDataSym(ptr, &sym))
    {
        FILE *fp;

        assert(false);
        fp = fopen("savegame symbols missing.txt", "a");
        if (fp)
        {
            fprintf(fp,"data %p - reference variable xdim at %p\n",ptr, &xdim);
            fclose(fp);
        }
        return 1;
    }

    MWRITE(&sym, sizeof(sym), 1, fil);

    return 0;
}

static int SaveSymCodeInfo_raw(MFILE_WRITE fil, void *ptr)
{
    savedcodesym sym;

    if (Saveable_FindCodeSym(ptr, &sym))
    {
        FILE *fp;

        assert(false);
        fp = fopen("savegame symbols missing.txt", "a");
        if (fp)
        {
            fprintf(fp,"code %p - reference function SaveSymDataInfo at %p\n",ptr, SaveSymDataInfo);
            fclose(fp);
        }
        return 1;
    }

    MWRITE(&sym, sizeof(sym), 1, fil);

    return 0;
}
template <typename T>
static int SaveSymCodeInfo(MFILE_WRITE fil, T * ptr)
{
    return SaveSymCodeInfo_raw(fil, (void *)ptr);
}

int LoadSymDataInfo(MFILE_READ fil, void **ptr)
{
    saveddatasym sym;

    MREAD(&sym, sizeof(sym), 1, fil);

    return Saveable_RestoreDataSym(&sym, ptr);
}
int LoadSymCodeInfo(MFILE_READ fil, void **ptr)
{
    savedcodesym sym;

    MREAD(&sym, sizeof(sym), 1, fil);

    return Saveable_RestoreCodeSym(&sym, ptr);
}



bool GameInterface::SaveGame()
{
    MFILE_WRITE fil;
    int i,j;
    short ndx;
    PLAYER tp;
    PLAYERp pp;
    SECT_USERp sectu;
    USER tu;
    USERp u;
    ANIM tanim;
    ANIMp a;
    PANEL_SPRITE tpanel_sprite;
    PANEL_SPRITEp psp,cur,next;
    SECTOR_OBJECTp sop;
    int saveisshot=0;

    Saveable_Init();
	
	
    // workaround until the level info here has been transitioned.
	fil = WriteSavegameChunk("snapshot.sw");

    MWRITE(&Skill,sizeof(Skill),1,fil);

    //
    // Sector object
    //

    MWRITE(SectorObject, sizeof(SectorObject),1,fil);

    for (ndx = 0; ndx < (short)SIZ(SectorObject); ndx++)
    {
        sop = &SectorObject[ndx];

        saveisshot |= SaveSymCodeInfo(fil, sop->PreMoveAnimator);
        assert(!saveisshot);
        saveisshot |= SaveSymCodeInfo(fil, sop->PostMoveAnimator);
        assert(!saveisshot);
        saveisshot |= SaveSymCodeInfo(fil, sop->Animator);
        assert(!saveisshot);
        saveisshot |= SaveSymDataInfo(fil, sop->controller);
        assert(!saveisshot);
        saveisshot |= SaveSymDataInfo(fil, sop->sp_child);
        assert(!saveisshot);
    }


    MWRITE(SineWaveFloor, sizeof(SineWaveFloor),1,fil);
    MWRITE(SineWall, sizeof(SineWall),1,fil);
    MWRITE(SpringBoard, sizeof(SpringBoard),1,fil);


    MWRITE(Track, sizeof(Track),1,fil);
    for (i = 0; i < MAX_TRACKS; i++)
    {
        ASSERT(Track[i].TrackPoint);
        if (Track[i].NumPoints == 0)
            MWRITE(Track[i].TrackPoint, sizeof(TRACK_POINT),1,fil);
        else
            MWRITE(Track[i].TrackPoint, Track[i].NumPoints * sizeof(TRACK_POINT),1,fil);
    }

    MWRITE(&Player[myconnectindex].input,sizeof(Player[myconnectindex].input),1,fil);
    MWRITE(&screenpeek,sizeof(screenpeek),1,fil);
    MWRITE(&randomseed, sizeof(randomseed), 1, fil);

    // do all sector manipulation structures

#if ANIM_SAVE
#if 1
    MWRITE(&AnimCnt,sizeof(AnimCnt),1,fil);

    for (i = 0, a = &tanim; i < AnimCnt; i++)
    {
        intptr_t offset;
        memcpy(a,&Anim[i],sizeof(ANIM));

        // maintain compatibility with sinking boat which points to user data
        for (j=0; j<MAXSPRITES; j++)
        {
            if (User[j].Data())
            {
                uint8_t* bp = (uint8_t*)User[j].Data();

                if ((uint8_t*)a->ptr >= bp && (uint8_t*)a->ptr < bp + sizeof(USER))
                {
                    offset = (intptr_t)((uint8_t*)a->ptr - bp); // offset from user data
                    a->ptr = (int *)-2;
                    break;
                }
            }
        }

        if ((intptr_t)a->ptr != -2)
        {
            for (j=0; j<numsectors; j++)
            {
                if (SectUser[j].Data())
                {
                    uint8_t* bp = (uint8_t*)SectUser[j].Data();

                    if ((uint8_t*)a->ptr >= bp && (uint8_t*)a->ptr < bp + sizeof(SECT_USER))
                    {
                        offset = (intptr_t)((uint8_t*)a->ptr - bp); // offset from user data
                        a->ptr = (int *)-3;
                        break;
                    }
                }
            }
        }
        MWRITE(a,sizeof(ANIM),1,fil);

        if ((intptr_t)a->ptr == -2 || (intptr_t)a->ptr == -3)
        {
            MWRITE(&j, sizeof(j),1,fil);
            MWRITE(&offset, sizeof(offset),1,fil);
        }
        else
        {
            saveisshot |= SaveSymDataInfo(fil, a->ptr);
            assert(!saveisshot);
        }

        saveisshot |= SaveSymCodeInfo(fil, a->callback);
        assert(!saveisshot);
        saveisshot |= SaveSymDataInfo(fil, a->callbackdata);
        assert(!saveisshot);
    }

#else
    ndx = 0;
    for (i = AnimCnt - 1, a = &tanim; i >= 0; i--)
    {
        // write header
        MWRITE(&ndx,sizeof(ndx),1,fil);

        memcpy(a,&Anim[i],sizeof(ANIM));
        MWRITE(a,sizeof(ANIM),1,fil);

        saveisshot |= SaveSymDataInfo(fil, a->ptr);
        saveisshot |= SaveSymCodeInfo(fil, a->callback);
        saveisshot |= SaveSymDataInfo(fil, a->callbackdata);

        ndx++;
    }

    // write trailer
    ndx = -1;
    MWRITE(&ndx,sizeof(ndx),1,fil);
#endif
#endif

    MWRITE(&NormalVisibility,sizeof(NormalVisibility),1,fil);
    MWRITE(&MoveSkip2,sizeof(MoveSkip2),1,fil);
    MWRITE(&MoveSkip4,sizeof(MoveSkip4),1,fil);
    MWRITE(&MoveSkip8,sizeof(MoveSkip8),1,fil);

    // SO interpolations
	saveisshot |= so_writeinterpolations(fil);
	assert(!saveisshot);

    // mirror
    MWRITE(mirror,sizeof(mirror),1,fil);
    MWRITE(&mirrorcnt,sizeof(mirrorcnt),1,fil);
    MWRITE(&mirrorinview,sizeof(mirrorinview),1,fil);

    // queue
    MWRITE(&StarQueueHead,sizeof(StarQueueHead),1,fil);
    MWRITE(StarQueue,sizeof(StarQueue),1,fil);
    MWRITE(&HoleQueueHead,sizeof(HoleQueueHead),1,fil);
    MWRITE(HoleQueue,sizeof(HoleQueue),1,fil);
    MWRITE(&WallBloodQueueHead,sizeof(WallBloodQueueHead),1,fil);
    MWRITE(WallBloodQueue,sizeof(WallBloodQueue),1,fil);
    MWRITE(&FloorBloodQueueHead,sizeof(FloorBloodQueueHead),1,fil);
    MWRITE(FloorBloodQueue,sizeof(FloorBloodQueue),1,fil);
    MWRITE(&GenericQueueHead,sizeof(GenericQueueHead),1,fil);
    MWRITE(GenericQueue,sizeof(GenericQueue),1,fil);
    MWRITE(&LoWangsQueueHead,sizeof(LoWangsQueueHead),1,fil);
    MWRITE(LoWangsQueue,sizeof(LoWangsQueue),1,fil);

    MWRITE(&PlayClock,sizeof(PlayClock),1,fil);
    MWRITE(&TotalKillable,sizeof(TotalKillable),1,fil);

    // game settings
    MWRITE(&gNet,sizeof(gNet),1,fil);

    MWRITE(&gs,sizeof(gs),1,fil);

    MWRITE(&LevelSecrets,sizeof(LevelSecrets),1,fil);

    MWRITE(&Bunny_Count,sizeof(Bunny_Count),1,fil);

    MWRITE(&GodMode,sizeof(GodMode),1,fil);

    MWRITE(&FinishTimer,sizeof(FinishTimer),1,fil);
    MWRITE(&FinishAnim,sizeof(FinishAnim),1,fil);

    MWRITE(&serpwasseen, sizeof(serpwasseen), 1, fil);
    MWRITE(&sumowasseen, sizeof(sumowasseen), 1, fil);
    MWRITE(&zillawasseen, sizeof(zillawasseen), 1, fil);
    MWRITE(BossSpriteNum, sizeof(BossSpriteNum), 1, fil);
    //MWRITE(&Zombies, sizeof(Zombies), 1, fil);

    return !saveisshot;
}


bool GameInterface::LoadGame()
{
    MFILE_READ fil;
    int i,j,saveisshot=0;
    short ndx,SpriteNum,sectnum;
    PLAYERp pp = NULL;
    USERp u;
    SECTOR_OBJECTp sop;
    SECT_USERp sectu;
    ANIMp a;
    PANEL_SPRITEp psp,next;


    Saveable_Init();

	auto filr = ReadSavegameChunk("snapshot.sw");
	if (!filr.isOpen()) return false;
	fil = &filr;

    MREAD(&Skill,sizeof(Skill),1,fil);

    MREAD(SectorObject, sizeof(SectorObject),1,fil);

    for (ndx = 0; ndx < (short)SIZ(SectorObject); ndx++)
    {
        sop = &SectorObject[ndx];

        saveisshot |= LoadSymCodeInfo(fil, (void **)&sop->PreMoveAnimator);
        saveisshot |= LoadSymCodeInfo(fil, (void **)&sop->PostMoveAnimator);
        saveisshot |= LoadSymCodeInfo(fil, (void **)&sop->Animator);
        saveisshot |= LoadSymDataInfo(fil, (void **)&sop->controller);
        saveisshot |= LoadSymDataInfo(fil, (void **)&sop->sp_child);
        if (saveisshot) { MCLOSE_READ(fil); return false; }
    }

    MREAD(SineWaveFloor, sizeof(SineWaveFloor),1,fil);
    MREAD(SineWall, sizeof(SineWall),1,fil);
    MREAD(SpringBoard, sizeof(SpringBoard),1,fil);

    MREAD(Track, sizeof(Track),1,fil);
    for (i = 0; i < MAX_TRACKS; i++)
    {
        if (Track[i].NumPoints == 0)
        {
            Track[i].TrackPoint = (TRACK_POINTp)CallocMem(sizeof(TRACK_POINT), 1);
            MREAD(Track[i].TrackPoint, sizeof(TRACK_POINT),1,fil);
        }
        else
        {
            Track[i].TrackPoint = (TRACK_POINTp)CallocMem(Track[i].NumPoints * sizeof(TRACK_POINT), 1);
            MREAD(Track[i].TrackPoint, Track[i].NumPoints * sizeof(TRACK_POINT),1,fil);
        }
    }

    MREAD(&Player[myconnectindex].input,sizeof(Player[myconnectindex].input),1,fil);

    MREAD(&screenpeek,sizeof(screenpeek),1,fil);
    MREAD(&randomseed, sizeof(randomseed), 1, fil);

    // do all sector manipulation structures

#if ANIM_SAVE
#if 1
    MREAD(&AnimCnt,sizeof(AnimCnt),1,fil);

    for (i = 0; i < AnimCnt; i++)
    {
        a = &Anim[i];
        MREAD(a,sizeof(ANIM),1,fil);

        if ((intptr_t)a->ptr == -2)
        {
            // maintain compatibility with sinking boat which points to user data
            int offset;
            MREAD(&j, sizeof(j),1,fil);
            MREAD(&offset, sizeof(offset),1,fil);
            a->ptr = (int *)(((char *)User[j].Data()) + offset);
        }
        else if ((intptr_t)a->ptr == -3)
        {
            // maintain compatibility with sinking boat which points to user data
            int offset;
            MREAD(&j, sizeof(j),1,fil);
            MREAD(&offset, sizeof(offset),1,fil);
            a->ptr = (int *)(((char *)SectUser[j].Data()) + offset);
        }
        else
        {
            saveisshot |= LoadSymDataInfo(fil, (void **)&a->ptr);
        }

        saveisshot |= LoadSymCodeInfo(fil, (void **)&a->callback);
        saveisshot |= LoadSymDataInfo(fil, (void **)&a->callbackdata);
        if (saveisshot) { MCLOSE_READ(fil); return false; }
    }
#else
    AnimCnt = 0;
    for (i = MAXANIM - 1; i >= 0; i--)
    {
        a = &Anim[i];

        MREAD(&ndx,sizeof(ndx),1,fil);

        if (ndx == -1)
            break;

        AnimCnt++;

        MREAD(a,sizeof(ANIM),1,fil);

        saveisshot |= LoadSymDataInfo(fil, (void **)&a->ptr);
        saveisshot |= LoadSymCodeInfo(fil, (void **)&a->callback);
        saveisshot |= LoadSymDataInfo(fil, (void **)&a->callbackdata);
        if (saveisshot) { MCLOSE_READ(fil); return false; }
    }
#endif
#endif

    MREAD(&NormalVisibility,sizeof(NormalVisibility),1,fil);

    MREAD(&MoveSkip2,sizeof(MoveSkip2),1,fil);
    MREAD(&MoveSkip4,sizeof(MoveSkip4),1,fil);
    MREAD(&MoveSkip8,sizeof(MoveSkip8),1,fil);

    // SO interpolations
    saveisshot |= so_readinterpolations(fil);
    if (saveisshot) { MCLOSE_READ(fil); return false; }

    // mirror
    MREAD(mirror,sizeof(mirror),1,fil);
    MREAD(&mirrorcnt,sizeof(mirrorcnt),1,fil);
    MREAD(&mirrorinview,sizeof(mirrorinview),1,fil);

    // queue
    MREAD(&StarQueueHead,sizeof(StarQueueHead),1,fil);
    MREAD(StarQueue,sizeof(StarQueue),1,fil);
    MREAD(&HoleQueueHead,sizeof(HoleQueueHead),1,fil);
    MREAD(HoleQueue,sizeof(HoleQueue),1,fil);
    MREAD(&WallBloodQueueHead,sizeof(WallBloodQueueHead),1,fil);
    MREAD(WallBloodQueue,sizeof(WallBloodQueue),1,fil);
    MREAD(&FloorBloodQueueHead,sizeof(FloorBloodQueueHead),1,fil);
    MREAD(FloorBloodQueue,sizeof(FloorBloodQueue),1,fil);
    MREAD(&GenericQueueHead,sizeof(GenericQueueHead),1,fil);
    MREAD(GenericQueue,sizeof(GenericQueue),1,fil);
    MREAD(&LoWangsQueueHead,sizeof(LoWangsQueueHead),1,fil);
    MREAD(LoWangsQueue,sizeof(LoWangsQueue),1,fil);

    // init timing vars before PlayClock is read
    MREAD(&PlayClock,sizeof(PlayClock),1,fil);
    MREAD(&TotalKillable,sizeof(TotalKillable),1,fil);

    // game settings
    MREAD(&gNet,sizeof(gNet),1,fil);

	MREAD(&gs,sizeof(gs),1,fil);

    MREAD(&LevelSecrets,sizeof(LevelSecrets),1,fil);

    MREAD(&Bunny_Count,sizeof(Bunny_Count),1,fil);

    MREAD(&GodMode,sizeof(GodMode),1,fil);

    MREAD(&FinishTimer,sizeof(FinishTimer),1,fil);
    MREAD(&FinishAnim,sizeof(FinishAnim),1,fil);

    MREAD(&serpwasseen, sizeof(serpwasseen), 1, fil);
    MREAD(&sumowasseen, sizeof(sumowasseen), 1, fil);
    MREAD(&zillawasseen, sizeof(zillawasseen), 1, fil);
    MREAD(BossSpriteNum, sizeof(BossSpriteNum), 1, fil);
    //MREAD(&Zombies, sizeof(Zombies), 1, fil);

    MCLOSE_READ(fil);


    DoTheCache();

    // this is ok - just duplicating sector list with pointers
    for (sop = SectorObject; sop < &SectorObject[SIZ(SectorObject)]; sop++)
    {
        for (i = 0; i < sop->num_sectors; i++)
            sop->sectp[i] = &sector[sop->sector[i]];
    }

    {
        int SavePlayClock = PlayClock;
        InitTimingVars();
        PlayClock = SavePlayClock;
    }
    InitNetVars();

    screenpeek = myconnectindex;

    Mus_ResumeSaved();
    if (snd_ambience)
        StartAmbientSound();

    // this is not a new game
    ShadowWarrior::NewGame = false;


    DoPlayerDivePalette(Player+myconnectindex);
    DoPlayerNightVisionPalette(Player+myconnectindex);

    InitLevelGlobals();
    return true;
}

END_SW_NS
