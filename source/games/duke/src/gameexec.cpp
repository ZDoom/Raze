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

*/
//-------------------------------------------------------------------------
#include "ns.h"
#include "concmd.h"
#include "duke3d.h"
#include "gamevar.h"
#include "mapinfo.h"
#include "gamestate.h"
#include "conlabel.h"
#include "automap.h"
#include "dukeactor.h"

BEGIN_DUKE_NS

// Player Actions - used by ifp instruction.
enum playeraction_t {
	pstanding    = 0x00000001,
	pwalking     = 0x00000002,
	prunning     = 0x00000004,
	pducking     = 0x00000008,
	pfalling     = 0x00000010,
	pjumping     = 0x00000020,
	phigher      = 0x00000040,
	pwalkingback = 0x00000080,
	prunningback = 0x00000100,
	pkicking     = 0x00000200,
	pshrunk      = 0x00000400,
	pjetpack     = 0x00000800,
	ponsteroids  = 0x00001000,
	ponground    = 0x00002000,
	palive       = 0x00004000,
	pdead        = 0x00008000,
	pfacing      = 0x00010000
};



struct ParseState
{
	int killit_flag;
	int g_p;
	int g_x;
	DDukeActor *g_ac;
	int* insptr;
	Collision coll;

	int parse(void);
	void parseifelse(int condition);
};

int furthestcanseepoint(DDukeActor* i, DDukeActor* ts, DVector2& pos);

sectortype* toSect(int index)
{
	return validSectorIndex(index) ? &sector[index] : nullptr;
}

int fromSect(sectortype* sect)
{
	return sect ? sectindex(sect) : -1;
}

walltype* toWall(int index)
{
	return validWallIndex(index) ? &wall[index] : nullptr;
}

int fromWall(walltype* sect)
{
	return sect ? wallindex(sect) : -1;
}

static void DoUserDef(bool bSet, int lVar1, int lLabelID, int lVar2, DDukeActor* sActor, int sPlayer, int lParm2)
{
	auto vValue = GetGameVarID(lVar2, sActor, sPlayer);
	auto lValue = vValue.safeValue();

	// most settings have been removed because they are either pointless, no longer existent or simply too dangerous to access.
	// Others have been made read-only.
	switch (lLabelID)
	{
	case USERDEFS_GOD: // redid this so that the script won't be able to disable user-set god mode.
		if (bSet) ud.god = (ud.god & ~2) | (lValue? 2:0);
		else SetGameVarID(lVar2, !!ud.god, sActor, sPlayer);
		break;

	case USERDEFS_CASHMAN:
		if (bSet) ud.cashman = lValue;
		else SetGameVarID(lVar2, ud.cashman, sActor, sPlayer);
		break;

	case USERDEFS_EOG:
		if (bSet) ud.eog = lValue;
		else SetGameVarID(lVar2, ud.eog, sActor, sPlayer);
		break;

	case USERDEFS_SHOWALLMAP:
		if (bSet) gFullMap = lValue;
		else SetGameVarID(lVar2, gFullMap, sActor, sPlayer);
		break;

	case USERDEFS_SHOWWEAPONS:
		// Read-only user state.
		if (!bSet) SetGameVarID(lVar2, cl_showweapon, sActor, sPlayer);
		break;

	case USERDEFS_CAMERASPRITE:
		if (bSet) ud.cameraactor = vValue.safeActor();
		else SetGameVarID(lVar2, ud.cameraactor, sActor, sPlayer);
		break;

	case USERDEFS_LAST_CAMSPRITE:
		if (!bSet) SetGameVarID(lVar2, -1, sActor, sPlayer);
		break;

	case USERDEFS_LAST_LEVEL:
		if (bSet) ud.last_level = lValue;
		else SetGameVarID(lVar2, ud.last_level, sActor, sPlayer);
		break;

	case USERDEFS_SECRETLEVEL:
		if (bSet) ud.secretlevel = lValue;
		else SetGameVarID(lVar2, ud.secretlevel, sActor, sPlayer);
		break;

	case USERDEFS_CONST_VISIBILITY:
		if (bSet) ud.const_visibility = lValue;
		else SetGameVarID(lVar2, ud.const_visibility, sActor, sPlayer);
		break;

	case USERDEFS_SHADOWS:
		if (bSet) ud.shadows = lValue;
		else SetGameVarID(lVar2, ud.shadows, sActor, sPlayer);
		break;
	case USERDEFS_M_COOP:
		if (!bSet) SetGameVarID(lVar2, ud.m_coop, sActor, sPlayer);
		break;

	case USERDEFS_COOP:
		if (!bSet) SetGameVarID(lVar2, ud.coop, sActor, sPlayer);
		break;

	case USERDEFS_RESPAWN_MONSTERS:
		if (bSet) ud.respawn_monsters = lValue;
		else SetGameVarID(lVar2, ud.respawn_monsters, sActor, sPlayer);
		break;

	case USERDEFS_RESPAWN_ITEMS:
		if (bSet) ud.respawn_items = lValue;
		else SetGameVarID(lVar2, ud.respawn_items, sActor, sPlayer);
		break;

	case USERDEFS_RESPAWN_INVENTORY:
		if (bSet) ud.respawn_inventory = lValue;
		else SetGameVarID(lVar2, ud.respawn_inventory, sActor, sPlayer);
		break;

	case USERDEFS_RECSTAT:
		if (!bSet) SetGameVarID(lVar2, ud.recstat, sActor, sPlayer);
		break;

	case USERDEFS_MONSTERS_OFF:
		if (bSet) ud.monsters_off = lValue;
		else SetGameVarID(lVar2, ud.monsters_off, sActor, sPlayer);
		break;

	case USERDEFS_BRIGHTNESS:
		if (bSet) ud.brightness = lValue;
		else SetGameVarID(lVar2, ud.brightness, sActor, sPlayer);
		break;

	case USERDEFS_M_RESPAWN_ITEMS:
		if (bSet) ud.m_respawn_items = lValue;
		else SetGameVarID(lVar2, ud.m_respawn_items, sActor, sPlayer);
		break;

	case USERDEFS_M_RESPAWN_MONSTERS:
		if (bSet) ud.m_respawn_monsters = lValue;
		else SetGameVarID(lVar2, ud.m_respawn_monsters, sActor, sPlayer);
		break;

	case USERDEFS_M_RESPAWN_INVENTORY:
		if (bSet) ud.m_respawn_inventory = lValue;
		else SetGameVarID(lVar2, ud.m_respawn_inventory, sActor, sPlayer);
		break;

	case USERDEFS_M_MONSTERS_OFF:
		if (bSet) ud.m_monsters_off = lValue;
		else SetGameVarID(lVar2, ud.m_monsters_off, sActor, sPlayer);
		break;

	case USERDEFS_M_FFIRE:
		if (bSet) ud.m_ffire = lValue;
		else SetGameVarID(lVar2, ud.m_ffire, sActor, sPlayer);
		break;

	case USERDEFS_FFIRE:
		if (bSet) ud.ffire = lValue;
		else SetGameVarID(lVar2, ud.ffire, sActor, sPlayer);
		break;

	case USERDEFS_MULTIMODE:
		if (!bSet) SetGameVarID(lVar2, ud.multimode, sActor, sPlayer);
		break;

	case USERDEFS_PLAYER_SKILL:
		if (bSet) ud.player_skill = lValue;
		else SetGameVarID(lVar2, ud.player_skill, sActor, sPlayer);
		break;

	case USERDEFS_MARKER:
		if (bSet) ud.marker = lValue;
		else SetGameVarID(lVar2, ud.marker, sActor, sPlayer);
		break;

	default:
		// This will also catch all deleted cases.
		// make sure that the return value is always defined.
		if (!bSet) SetGameVarID(lVar2, 0, sActor, sPlayer);
		break;
	}
	return;
}

///////////////////////////////////////////
void DoPlayer(bool bSet, int lVar1, int lLabelID, int lVar2, DDukeActor* sActor, int sPlayer, int lParm2)
{
	auto vValue = GetGameVarID(lVar2, sActor, sPlayer);
	auto lValue = vValue.safeValue();

	int iPlayer;
	int lTemp;

	if (lVar1 == g_iThisActorID)
	{
		// if they've asked for 'this', then use 'this player'...
		iPlayer = sPlayer;
	}
	else
	{
		iPlayer = GetGameVarID(lVar1, sActor, sPlayer).safeValue();
	}

	if (iPlayer < 0 || iPlayer >= MAXPLAYERS)
		return;

	const auto p = getPlayer(iPlayer);
	const auto pact = p->GetActor();

	switch (lLabelID)
	{
	case PLAYER_ZOOM:
		SetGameVarID(lVar2, 768, sActor, sPlayer);	//return default for AM zoom.
		break;

	case PLAYER_EXITX:
		if (bSet) p->Exit.X = lValue * maptoworld;
		else SetGameVarID(lVar2, int(p->Exit.X / maptoworld), sActor, sPlayer);
		break;

	case PLAYER_EXITY:
		if (bSet) p->Exit.Y = lValue * maptoworld;
		else SetGameVarID(lVar2, int(p->Exit.Y / maptoworld), sActor, sPlayer);
		break;

	case PLAYER_LOOGIEX:
		if (bSet) p->loogie[lParm2].X = lValue;
		else SetGameVarID(lVar2, (int)p->loogie[lParm2].X, sActor, sPlayer);
		break;

	case PLAYER_LOOGIEY:
		if (bSet) p->loogie[lParm2].Y = lValue;
		else SetGameVarID(lVar2, (int)p->loogie[lParm2].Y, sActor, sPlayer);
		break;

	case PLAYER_NUMLOOGS:
		if (bSet) p->numloogs = lValue;
		else SetGameVarID(lVar2, p->numloogs, sActor, sPlayer);
		break;

	case PLAYER_LOOGCNT:
		if (bSet) p->oloogcnt = p->loogcnt = lValue;
		else SetGameVarID(lVar2, p->loogcnt, sActor, sPlayer);
		break;

	case PLAYER_POSX: // oh, my... :( Writing to these has been disabled until I know how to do it without the engine shitting all over itself.
		if (!bSet) SetGameVarID(lVar2, int(pact->spr.pos.X * (1/maptoworld)), sActor, sPlayer);
		break;

	case PLAYER_POSY:
		if (!bSet) SetGameVarID(lVar2, int(pact->spr.pos.Y * (1 / maptoworld)), sActor, sPlayer);
		break;

	case PLAYER_POSZ:
		if (!bSet) SetGameVarID(lVar2, int(pact->getOffsetZ() * (1 / zmaptoworld)), sActor, sPlayer);
		break;

	case PLAYER_HORIZ:
		if (bSet)
		{	
			if (p->cmd.ucmd.actions & SB_CENTERVIEW)
			{
				p->cmd.ucmd.actions &= ~SB_CENTERVIEW;
			}
			pact->spr.Angles.Pitch = maphoriz(-lValue);
		}
		else SetGameVarID(lVar2, int(pact->spr.Angles.Pitch.Tan() * -128.), sActor, sPlayer);
		break;

	case PLAYER_OHORIZ:
		if (!bSet) SetGameVarID(lVar2, int(pact->PrevAngles.Pitch.Tan() * -128.), sActor, sPlayer);
		break;

	case PLAYER_HORIZOFF:
		if (bSet) p->Angles.ViewAngles.Pitch = maphoriz(-lValue);
		else SetGameVarID(lVar2, int(p->Angles.ViewAngles.Pitch.Tan() * -128.), sActor, sPlayer);
		break;

	case PLAYER_OHORIZOFF:
		if (!bSet) SetGameVarID(lVar2, int(p->Angles.PrevViewAngles.Pitch.Tan() * -128.), sActor, sPlayer);
		break;

	case PLAYER_INVDISPTIME:
		if (bSet) p->invdisptime = lValue;
		else SetGameVarID(lVar2, p->invdisptime, sActor, sPlayer);
		break;

	case PLAYER_BOBPOSX:
		if (bSet) p->bobpos.X = lValue * maptoworld;
		else SetGameVarID(lVar2, int(p->bobpos.X * (1/maptoworld)), sActor, sPlayer);
		break;

	case PLAYER_BOBPOSY:
		if (bSet) p->bobpos.Y = lValue * maptoworld;
		else SetGameVarID(lVar2, int(p->bobpos.Y * (1/maptoworld)), sActor, sPlayer);
		break;

	case PLAYER_OPOSX:
		if (bSet) pact->opos.X = lValue * maptoworld;
		else SetGameVarID(lVar2, int(pact->opos.X * (1/maptoworld)), sActor, sPlayer);
		break;

	case PLAYER_OPOSY:
		if (bSet) pact->opos.Y = lValue * maptoworld;
		else SetGameVarID(lVar2, int(pact->opos.Y * (1 / maptoworld)), sActor, sPlayer);
		break;

	case PLAYER_OPOSZ:
		if (bSet) pact->opos.Z = (lValue * zmaptoworld) + gs.playerheight;
		else SetGameVarID(lVar2, int(pact->getPrevOffsetZ() * (1 / zmaptoworld)), sActor, sPlayer);
		break;

	case PLAYER_PYOFF:
		if (bSet) p->pyoff = lValue * zmaptoworld;
		else SetGameVarID(lVar2, int(p->pyoff / zmaptoworld), sActor, sPlayer);
		break;

	case PLAYER_OPYOFF:
		if (bSet) p->opyoff = lValue * zmaptoworld;
		else SetGameVarID(lVar2, int(p->opyoff / zmaptoworld), sActor, sPlayer);
		break;

	case PLAYER_POSXV:
		if (bSet) p->vel.X = FixedToFloat<18>(lValue);
		else SetGameVarID(lVar2, FloatToFixed<18>(p->vel.X), sActor, sPlayer);
		break;

	case PLAYER_POSYV:
		if (bSet) p->vel.Y = FixedToFloat<18>(lValue);
		else SetGameVarID(lVar2, FloatToFixed<18>(p->vel.Y), sActor, sPlayer);
		break;

	case PLAYER_POSZV:
		if (bSet) p->vel.Z = lValue * zmaptoworld;
		else SetGameVarID(lVar2, int(p->vel.Z / zmaptoworld), sActor, sPlayer);
		break;

	case PLAYER_LAST_PISSED_TIME:
		if (bSet) p->last_pissed_time = lValue;
		else SetGameVarID(lVar2, p->last_pissed_time, sActor, sPlayer);
		break;

	case PLAYER_TRUEFZ:
		if (bSet) p->truefz = lValue * zmaptoworld;
		else SetGameVarID(lVar2, int(p->truefz * (1/zmaptoworld)), sActor, sPlayer);
		break;

	case PLAYER_TRUECZ:
		if (bSet) p->truecz = lValue * zmaptoworld;
		else SetGameVarID(lVar2, int(p->truecz * (1 / zmaptoworld)), sActor, sPlayer);
		break;

	case PLAYER_PLAYER_PAR:
		if (bSet) p->player_par = lValue;
		else SetGameVarID(lVar2, p->player_par, sActor, sPlayer);
		break;

	case PLAYER_VISIBILITY:
		if (bSet) p->visibility = lValue;
		else SetGameVarID(lVar2, p->visibility, sActor, sPlayer);
		break;

	case PLAYER_BOBCOUNTER:
		if (bSet) p->bobcounter = lValue;
		else SetGameVarID(lVar2, p->bobcounter, sActor, sPlayer);
		break;

	case PLAYER_WEAPON_SWAY:
		if (bSet) p->oweapon_sway = p->weapon_sway = lValue;
		else SetGameVarID(lVar2, p->weapon_sway, sActor, sPlayer);
		break;

	case PLAYER_PALS_TIME:
		if (bSet) p->pals.a = lValue;
		else SetGameVarID(lVar2, p->pals.a, sActor, sPlayer);
		break;

	case PLAYER_RANDOMFLAMEX:
		if (bSet) p->randomflamex = lValue;
		else SetGameVarID(lVar2, p->randomflamex, sActor, sPlayer);
		break;

	case PLAYER_CRACK_TIME:
		if (bSet) p->crack_time = lValue;
		else SetGameVarID(lVar2, p->crack_time, sActor, sPlayer);
		break;

	case PLAYER_AIM_MODE: // game has no business enforcing this in any way.
		if (!bSet) SetGameVarID(lVar2, p->aim_mode, sActor, sPlayer);
		break;

	case PLAYER_ANG:
		if (bSet) pact->spr.Angles.Yaw = mapangle(lValue);
		else SetGameVarID(lVar2, pact->spr.Angles.Yaw.Buildang(), sActor, sPlayer);
		break;

	case PLAYER_OANG:
		if (!bSet) SetGameVarID(lVar2, pact->PrevAngles.Yaw.Buildang(), sActor, sPlayer);
		break;

	case PLAYER_ANGVEL: // This no longer exists.
		if (!bSet) SetGameVarID(lVar2, 0, sActor, sPlayer);
		break;

	case PLAYER_CURSECTNUM:
		if (bSet) p->cursector = toSect(lValue);
		else SetGameVarID(lVar2, fromSect(p->cursector), sActor, sPlayer);
		break;

	case PLAYER_LOOK_ANG:
		if (bSet) p->Angles.ViewAngles.Yaw = mapangle(lValue);
		else SetGameVarID(lVar2, p->Angles.ViewAngles.Yaw.Buildang(), sActor, sPlayer);
		break;

	case PLAYER_LAST_EXTRA:
		if (bSet) p->last_extra = lValue;
		else SetGameVarID(lVar2, p->last_extra, sActor, sPlayer);
		break;

	case PLAYER_SUBWEAPON:
		if (bSet) p->subweapon = lValue;
		else SetGameVarID(lVar2, p->subweapon, sActor, sPlayer);
		break;

	case PLAYER_AMMO_AMOUNT:
		lTemp = GetGameVarID(lParm2, sActor, sPlayer).safeValue();
		if (bSet) p->ammo_amount[lTemp] = lValue;
		else SetGameVarID(lVar2, p->ammo_amount[lTemp], sActor, sPlayer);
		break;

	case PLAYER_WACKEDBYACTOR:
		if (bSet) p->wackedbyactor = vValue.safeActor();
		else SetGameVarID(lVar2, p->wackedbyactor, sActor, sPlayer);
		break;

	case PLAYER_FRAG:
		if (bSet) p->frag = lValue;
		else SetGameVarID(lVar2, p->frag, sActor, sPlayer);
		break;

	case PLAYER_FRAGGEDSELF:
		if (bSet) p->fraggedself = lValue;
		else SetGameVarID(lVar2, p->fraggedself, sActor, sPlayer);
		break;

	case PLAYER_CURR_WEAPON:
		if (bSet) p->curr_weapon = lValue;
		else SetGameVarID(lVar2, p->curr_weapon, sActor, sPlayer);
		break;

	case PLAYER_LAST_WEAPON:
		if (bSet) p->last_weapon = lValue;
		else SetGameVarID(lVar2, p->last_weapon, sActor, sPlayer);
		break;

	case PLAYER_TIPINCS:
		if (bSet) p->otipincs = p->tipincs = lValue;
		else SetGameVarID(lVar2, p->tipincs, sActor, sPlayer);
		break;

	case PLAYER_WANTWEAPONFIRE:
		if (bSet) p->wantweaponfire = lValue;
		else SetGameVarID(lVar2, p->wantweaponfire, sActor, sPlayer);
		break;

	case PLAYER_HOLODUKE_AMOUNT:
		if (bSet) p->holoduke_amount = lValue;
		else SetGameVarID(lVar2, p->holoduke_amount, sActor, sPlayer);
		break;

	case PLAYER_NEWOWNER:
		if (bSet && (p->newOwner = vValue.safeActor())) setForcedSyncInput(iPlayer);
		else SetGameVarID(lVar2, p->newOwner, sActor, sPlayer);
		break;

	case PLAYER_HURT_DELAY:
		if (bSet) p->hurt_delay = lValue;
		else SetGameVarID(lVar2, p->hurt_delay, sActor, sPlayer);
		break;

	case PLAYER_HBOMB_HOLD_DELAY:
		if (bSet) p->hbomb_hold_delay = lValue;
		else SetGameVarID(lVar2, p->hbomb_hold_delay, sActor, sPlayer);
		break;

	case PLAYER_JUMPING_COUNTER:
		if (bSet) p->jumping_counter = lValue;
		else SetGameVarID(lVar2, p->jumping_counter, sActor, sPlayer);
		break;

	case PLAYER_AIRLEFT:
		if (bSet) p->airleft = lValue;
		else SetGameVarID(lVar2, p->airleft, sActor, sPlayer);
		break;

	case PLAYER_KNEE_INCS:
		if (bSet) p->oknee_incs = p->knee_incs = lValue;
		else SetGameVarID(lVar2, p->knee_incs, sActor, sPlayer);
		break;

	case PLAYER_ACCESS_INCS:
		if (bSet) p->oaccess_incs = p->access_incs = lValue;
		else SetGameVarID(lVar2, p->access_incs, sActor, sPlayer);
		break;

	case PLAYER_ACCESS_WALLNUM:
		if (bSet) p->access_wall = toWall(lValue);
		else SetGameVarID(lVar2, fromWall(p->access_wall), sActor, sPlayer);
		break;

	case PLAYER_ACCESS_SPRITENUM:
		if (bSet) p->access_spritenum = vValue.safeActor();
		else SetGameVarID(lVar2, p->access_spritenum, sActor, sPlayer);
		break;

	case PLAYER_KICKBACK_PIC:
		if (bSet) p->okickback_pic = p->kickback_pic = lValue;
		else SetGameVarID(lVar2, p->kickback_pic, sActor, sPlayer);
		break;

	case PLAYER_GOT_ACCESS:
		if (bSet) p->got_access = lValue;
		else SetGameVarID(lVar2, p->got_access, sActor, sPlayer);
		break;

	case PLAYER_WEAPON_ANG:
		if (bSet) p->weapon_ang = lValue;
		else SetGameVarID(lVar2, p->weapon_ang, sActor, sPlayer);
		break;

	case PLAYER_FIRSTAID_AMOUNT:
		if (bSet) p->firstaid_amount = lValue;
		else SetGameVarID(lVar2, p->firstaid_amount, sActor, sPlayer);
		break;

	case PLAYER_SOMETHINGONPLAYER:
		if (bSet) p->somethingonplayer = vValue.safeActor();
		else SetGameVarID(lVar2, (p->somethingonplayer), sActor, sPlayer);
		break;

	case PLAYER_ON_CRANE:
		if (bSet && (p->on_crane = vValue.safeActor())) setForcedSyncInput(iPlayer);
		else SetGameVarID(lVar2, (p->on_crane), sActor, sPlayer);
		break;

	case PLAYER_I:	// Read only, because this is very dangerous.
		if (!bSet) SetGameVarID(lVar2, pact, sActor, sPlayer);
		break;

	case PLAYER_ONE_PARALLAX_SECTNUM:
		if (bSet) p->one_parallax_sectnum = toSect(lValue);
		else SetGameVarID(lVar2, fromSect(p->one_parallax_sectnum), sActor, sPlayer);
		break;

	case PLAYER_OVER_SHOULDER_ON:
		if (bSet) p->over_shoulder_on = lValue;
		else SetGameVarID(lVar2, p->over_shoulder_on, sActor, sPlayer);
		break;

	case PLAYER_RANDOM_CLUB_FRAME:
		if (bSet) p->orandom_club_frame = p->random_club_frame = lValue;
		else SetGameVarID(lVar2, p->random_club_frame, sActor, sPlayer);
		break;

	case PLAYER_FIST_INCS:
		if (bSet) p->ofist_incs = p->fist_incs = lValue;
		else SetGameVarID(lVar2, p->fist_incs, sActor, sPlayer);
		break;

	case PLAYER_ONE_EIGHTY_COUNT:
		if (bSet) p->Angles.YawSpin = mapangle(lValue);
		else SetGameVarID(lVar2, p->Angles.YawSpin.Buildang(), sActor, sPlayer);
		break;

	case PLAYER_CHEAT_PHASE:
		if (bSet) p->cheat_phase = lValue;
		else SetGameVarID(lVar2, p->cheat_phase, sActor, sPlayer);
		break;

	case PLAYER_DUMMYPLAYERSPRITE:
		if (bSet) p->dummyplayersprite = vValue.safeActor();
		else SetGameVarID(lVar2, (p->dummyplayersprite), sActor, sPlayer);
		break;

	case PLAYER_EXTRA_EXTRA8:
		if (bSet) p->extra_extra8 = lValue;
		else SetGameVarID(lVar2, p->extra_extra8, sActor, sPlayer);
		break;

	case PLAYER_QUICK_KICK:
		if (bSet) p->quick_kick = lValue;
		else SetGameVarID(lVar2, p->quick_kick, sActor, sPlayer);
		break;

	case PLAYER_HEAT_AMOUNT:
		if (bSet) p->heat_amount = lValue;
		else SetGameVarID(lVar2, p->heat_amount, sActor, sPlayer);
		break;

	case PLAYER_ACTORSQU:
		if (bSet) p->actorsqu = vValue.safeActor();
		else SetGameVarID(lVar2, (p->actorsqu), sActor, sPlayer);
		break;

	case PLAYER_TIMEBEFOREEXIT:
		if (bSet) p->timebeforeexit = lValue;
		else SetGameVarID(lVar2, p->timebeforeexit, sActor, sPlayer);
		break;

	case PLAYER_CUSTOMEXITSOUND:
		if (bSet) p->customexitsound = lValue;
		else SetGameVarID(lVar2, p->customexitsound, sActor, sPlayer);
		break;

	case PLAYER_INTERFACE_TOGGLE_FLAG:
		if (bSet) p->interface_toggle_flag = lValue;
		else SetGameVarID(lVar2, p->interface_toggle_flag, sActor, sPlayer);
		break;

	case PLAYER_ROTSCRNANG:
		if (bSet) p->Angles.PrevViewAngles.Roll = p->Angles.ViewAngles.Roll = -mapangle(lValue);
		else SetGameVarID(lVar2, -p->Angles.ViewAngles.Roll.Buildang(), sActor, sPlayer);
		break;

	case PLAYER_DEAD_FLAG:
		if (bSet) p->dead_flag = lValue;
		else SetGameVarID(lVar2, p->dead_flag, sActor, sPlayer);
		break;

	case PLAYER_SHOW_EMPTY_WEAPON:
		if (bSet) p->show_empty_weapon = lValue;
		else SetGameVarID(lVar2, p->show_empty_weapon, sActor, sPlayer);
		break;

	case PLAYER_SCUBA_AMOUNT:
		if (bSet) p->scuba_amount = lValue;
		else SetGameVarID(lVar2, p->scuba_amount, sActor, sPlayer);
		break;

	case PLAYER_JETPACK_AMOUNT:
		if (bSet) p->jetpack_amount = lValue;
		else SetGameVarID(lVar2, p->jetpack_amount, sActor, sPlayer);
		break;

	case PLAYER_STEROIDS_AMOUNT:
		if (bSet) p->steroids_amount = lValue;
		else SetGameVarID(lVar2, p->steroids_amount, sActor, sPlayer);
		break;

	case PLAYER_SHIELD_AMOUNT:
		if (bSet) p->shield_amount = lValue;
		else SetGameVarID(lVar2, p->shield_amount, sActor, sPlayer);
		break;

	case PLAYER_HOLODUKE_ON:
		if (bSet) p->holoduke_on = vValue.safeActor();
		else SetGameVarID(lVar2, (p->holoduke_on), sActor, sPlayer);
		break;

	case PLAYER_PYCOUNT:
		if (bSet) p->pycount = lValue;
		else SetGameVarID(lVar2, p->pycount, sActor, sPlayer);
		break;

	case PLAYER_WEAPON_POS:
		if (bSet) p->oweapon_pos = p->weapon_pos = lValue;
		else SetGameVarID(lVar2, p->weapon_pos, sActor, sPlayer);
		break;

	case PLAYER_FRAG_PS:
		if (bSet) 	p->frag_ps = lValue;
		else SetGameVarID(lVar2, p->frag_ps, sActor, sPlayer);
		break;

	case PLAYER_TRANSPORTER_HOLD:
		if (bSet) p->transporter_hold = lValue;
		else SetGameVarID(lVar2, p->transporter_hold, sActor, sPlayer);
		break;

	case PLAYER_LAST_FULL_WEAPON:
		if (bSet) p->last_full_weapon = lValue;
		else SetGameVarID(lVar2, p->last_full_weapon, sActor, sPlayer);
		break;

	case PLAYER_FOOTPRINTSHADE:
		if (bSet) p->footprintshade = lValue;
		else SetGameVarID(lVar2, p->footprintshade, sActor, sPlayer);
		break;

	case PLAYER_BOOT_AMOUNT:
		if (bSet) p->boot_amount = lValue;
		else SetGameVarID(lVar2, p->boot_amount, sActor, sPlayer);
		break;

	case PLAYER_GM:
		if (!bSet) SetGameVarID(lVar2, MODE_GAME, sActor, sPlayer);
		break;

	case PLAYER_ON_WARPING_SECTOR:
		if (bSet) p->on_warping_sector = lValue;
		else SetGameVarID(lVar2, p->on_warping_sector, sActor, sPlayer);
		break;

	case PLAYER_FOOTPRINTCOUNT:
		if (bSet) p->footprintcount = lValue;
		else SetGameVarID(lVar2, p->footprintcount, sActor, sPlayer);
		break;

	case PLAYER_HBOMB_ON:
		if (bSet) p->hbomb_on = lValue;
		else SetGameVarID(lVar2, p->hbomb_on, sActor, sPlayer);
		break;

	case PLAYER_JUMPING_TOGGLE:
		if (bSet) p->jumping_toggle = lValue;
		else SetGameVarID(lVar2, p->jumping_toggle, sActor, sPlayer);
		break;

	case PLAYER_RAPID_FIRE_HOLD:
		if (bSet) p->rapid_fire_hold = lValue;
		else SetGameVarID(lVar2, p->rapid_fire_hold, sActor, sPlayer);
		break;

	case PLAYER_ON_GROUND:
		if (bSet) p->on_ground = lValue;
		else SetGameVarID(lVar2, p->on_ground, sActor, sPlayer);
		break;

	case PLAYER_INVEN_ICON:
		if (bSet) p->inven_icon = lValue;
		else SetGameVarID(lVar2, p->inven_icon, sActor, sPlayer);
		break;

	case PLAYER_BUTTONPALETTE:
		if (bSet) p->buttonpalette = lValue;
		else SetGameVarID(lVar2, p->buttonpalette, sActor, sPlayer);
		break;

	case PLAYER_JETPACK_ON:
		if (bSet) p->jetpack_on = lValue;
		else SetGameVarID(lVar2, p->jetpack_on, sActor, sPlayer);
		break;

	case PLAYER_SPRITEBRIDGE:
		if (bSet) p->spritebridge = lValue;
		else SetGameVarID(lVar2, p->spritebridge, sActor, sPlayer);
		break;

	case PLAYER_LASTRANDOMSPOT:
		if (bSet) p->lastrandomspot = lValue;
		else SetGameVarID(lVar2, p->lastrandomspot, sActor, sPlayer);
		break;

	case PLAYER_SCUBA_ON:
		if (bSet) p->scuba_on = lValue;
		else SetGameVarID(lVar2, p->scuba_on, sActor, sPlayer);
		break;

	case PLAYER_FOOTPRINTPAL:
		if (bSet) p->footprintpal = lValue;
		else SetGameVarID(lVar2, p->footprintpal, sActor, sPlayer);
		break;

	case PLAYER_HEAT_ON:
		if (bSet) p->heat_on = lValue;
		else SetGameVarID(lVar2, p->heat_on, sActor, sPlayer);
		break;

	case PLAYER_HOLSTER_WEAPON:
		if (bSet) p->holster_weapon = lValue;
		else SetGameVarID(lVar2, p->holster_weapon, sActor, sPlayer);
		break;

	case PLAYER_FALLING_COUNTER:
		if (bSet) p->falling_counter = lValue;
		else SetGameVarID(lVar2, p->falling_counter, sActor, sPlayer);
		break;

	case PLAYER_GOTWEAPON:
		if (bSet) p->gotweapon[lParm2] = !!lValue;
		else SetGameVarID(lVar2, p->gotweapon[lParm2], sActor, sPlayer);
		break;

	case PLAYER_REFRESH_INVENTORY:
		if (bSet) p->refresh_inventory = lValue;
		else SetGameVarID(lVar2, p->refresh_inventory, sActor, sPlayer);
		break;

	case PLAYER_TOGGLE_KEY_FLAG:
		if (bSet) p->toggle_key_flag = lValue;
		else SetGameVarID(lVar2, p->toggle_key_flag, sActor, sPlayer);
		break;

	case PLAYER_KNUCKLE_INCS:
		if (bSet) p->knuckle_incs = lValue;
		else SetGameVarID(lVar2, p->knuckle_incs, sActor, sPlayer);
		break;

	case PLAYER_WALKING_SND_TOGGLE:
		if (bSet) p->walking_snd_toggle = lValue;
		else SetGameVarID(lVar2, p->walking_snd_toggle, sActor, sPlayer);
		break;

	case PLAYER_PALOOKUP:
		if (bSet) p->palookup = lValue;
		else SetGameVarID(lVar2, p->palookup, sActor, sPlayer);
		break;

	case PLAYER_HARD_LANDING:
		if (bSet) p->ohard_landing = p->hard_landing = lValue;
		else SetGameVarID(lVar2, p->hard_landing, sActor, sPlayer);
		break;

	case PLAYER_RETURN_TO_CENTER:
		if (bSet) p->cmd.ucmd.actions |= SB_CENTERVIEW;
		else SetGameVarID(lVar2, p->cmd.ucmd.actions & SB_CENTERVIEW ? int(abs((pact->spr.Angles.Pitch * (DAngle::fromDeg(9.) / GetMaxPitch())).Degrees())) : 0, sActor, sPlayer);
		break;

	default:
		if (!bSet) SetGameVarID(lVar2, 0, sActor, sPlayer);
		break;
	}
	return;
}

////////////////////
void DoWall(bool bSet, int lVar1, int lLabelID, int lVar2, DDukeActor* sActor, int sPlayer, int lParm2)
{
	auto lValue = GetGameVarID(lVar2, sActor, sPlayer).safeValue();
	auto vWall = GetGameVarID(lVar1, sActor, sPlayer);
	auto iWall = vWall.safeValue();

	if (iWall < 0 || iWall >= (int)wall.Size() || vWall.isActor())
	{
		if (!bSet) SetGameVarID(lVar2, 0, sActor, sPlayer);
		return;
	}
	auto wallp = &wall[iWall];

	// All fields affecting map geometry have been made read-only!
	switch (lLabelID)
	{
	case WALL_X:
		if (!bSet) SetGameVarID(lVar2, int(wallp->pos.X / maptoworld), sActor, sPlayer);
		break;
	case WALL_Y:
		if (!bSet) SetGameVarID(lVar2, int(wallp->pos.Y / maptoworld), sActor, sPlayer);
		break;
	case WALL_POINT2:
		if (!bSet) SetGameVarID(lVar2, wallp->point2, sActor, sPlayer);
		break;
	case WALL_NEXTWALL:
		if (!bSet) SetGameVarID(lVar2, wallp->nextwall, sActor, sPlayer);
		break;
	case WALL_NEXTSECTOR:
		if (!bSet) SetGameVarID(lVar2, wallp->nextsector, sActor, sPlayer);
		break;
	case WALL_CSTAT:
		if (bSet) wallp->cstat = EWallFlags::FromInt(lValue);
		else SetGameVarID(lVar2, wallp->cstat, sActor, sPlayer);
		break;
	case WALL_PICNUM:
		if (bSet) wallp->setwalltexture(tileGetTextureID(lValue));
		else SetGameVarID(lVar2, legacyTileNum(wallp->walltexture), sActor, sPlayer);
		break;
	case WALL_OVERPICNUM:
		if (bSet) wallp->setovertexture(tileGetTextureID(lValue));
		else SetGameVarID(lVar2, legacyTileNum(wallp->overtexture), sActor, sPlayer);
		break;
	case WALL_SHADE:
		if (bSet) wallp->shade = lValue;
		else SetGameVarID(lVar2, wallp->shade, sActor, sPlayer);
		break;
	case WALL_PAL:
		if (bSet) wallp->pal = lValue;
		else SetGameVarID(lVar2, wallp->pal, sActor, sPlayer);
		break;
	case WALL_XREPEAT:
		if (bSet) wallp->xrepeat = lValue;
		else SetGameVarID(lVar2, wallp->xrepeat, sActor, sPlayer);
		break;
	case WALL_YREPEAT:
		if (bSet) wallp->yrepeat = lValue;
		else SetGameVarID(lVar2, wallp->yrepeat, sActor, sPlayer);
		break;
	case WALL_XPANNING:
		if (bSet) wallp->xpan_ = (float)(lValue & 255);
		else SetGameVarID(lVar2, wallp->xpan(), sActor, sPlayer);
		break;
	case WALL_YPANNING:
		if (bSet) wallp->ypan_ = (float)(lValue & 255);
		else SetGameVarID(lVar2, wallp->ypan(), sActor, sPlayer);
		break;
	case WALL_LOTAG:
		if (bSet) wallp->lotag = lValue;
		else SetGameVarID(lVar2, wallp->lotag, sActor, sPlayer);
		break;
	case WALL_HITAG:
		if (bSet) wallp->hitag = lValue;
		else SetGameVarID(lVar2, wallp->hitag, sActor, sPlayer);
		break;
	case WALL_EXTRA:
		if (bSet) wallp->extra = lValue;
		else SetGameVarID(lVar2, wallp->extra, sActor, sPlayer);
		break;
	default:
		break;
	}
	return;
}

void DoSector(bool bSet, int lVar1, int lLabelID, int lVar2, DDukeActor* sActor, int sPlayer, int lParm2)
{
	int iSector;
	int lValue;
	bool no = false;

	if (lVar1 == g_iThisActorID)
	{
		// if they've asked for 'this', then use 'this'...
		iSector = sActor->sectno();
	}
	else
	{
		auto vv = GetGameVarID(lVar1, sActor, sPlayer);
		no = vv.isActor();
		iSector = vv.safeValue();
	}

	if (iSector < 0 || iSector >= (int)sector.Size() || no)
	{
		if (!bSet) SetGameVarID(lVar2, 0, sActor, sPlayer);
		return;
	}

	lValue = GetGameVarID(lVar2, sActor, sPlayer).safeValue();
	auto sectp = &sector[iSector];

	// All fields affecting map geometry have been made read-only!
	switch (lLabelID)
	{
	case SECTOR_WALLPTR:
		if (!bSet) SetGameVarID(lVar2, wallindex(sectp->walls.Data()), sActor, sPlayer);
		break;
	case SECTOR_WALLNUM:
		if (!bSet) SetGameVarID(lVar2, sectp->walls.Size(), sActor, sPlayer);
		break;
	case SECTOR_CEILINGZ:
		if (bSet) sectp->setceilingz(lValue * zmaptoworld);
		else SetGameVarID(lVar2, (int)(sectp->ceilingz / zmaptoworld), sActor, sPlayer);
		break;
	case SECTOR_FLOORZ:
		if (bSet) sectp->setfloorz(lValue * zmaptoworld);
		else SetGameVarID(lVar2, (int)(sectp->floorz / zmaptoworld), sActor, sPlayer);
		break;
	case SECTOR_CEILINGSTAT:
		if (bSet) sectp->ceilingstat = ESectorFlags::FromInt(lValue);
		else SetGameVarID(lVar2, sectp->ceilingstat, sActor, sPlayer);
		break;
	case SECTOR_FLOORSTAT:
		if (bSet) sectp->floorstat = ESectorFlags::FromInt(lValue);
		else SetGameVarID(lVar2, sectp->floorstat, sActor, sPlayer);
		break;
	case SECTOR_CEILINGPICNUM:
		if (bSet) sectp->setceilingtexture(tileGetTextureID(lValue));
		else SetGameVarID(lVar2, legacyTileNum(sectp->ceilingtexture), sActor, sPlayer);
		break;
	case SECTOR_CEILINGSLOPE:
		if (bSet) sectp->setceilingslope(lValue);
		else SetGameVarID(lVar2, sectp->ceilingheinum, sActor, sPlayer);
		break;
	case SECTOR_CEILINGSHADE:
		if (bSet) sectp->ceilingshade = lValue;
		else SetGameVarID(lVar2, sectp->ceilingshade, sActor, sPlayer);
		break;
	case SECTOR_CEILINGPAL:
		if (bSet) sectp->ceilingpal = lValue;
		else SetGameVarID(lVar2, sectp->ceilingpal, sActor, sPlayer);
		break;
	case SECTOR_CEILINGXPANNING:
		if (bSet) sectp->ceilingxpan_ = (float)(lValue & 255);
		else SetGameVarID(lVar2, sectp->ceilingxpan(), sActor, sPlayer);
		break;
	case SECTOR_CEILINGYPANNING:
		if (bSet) sectp->ceilingypan_ = (float)(lValue & 255);
		else SetGameVarID(lVar2, sectp->ceilingypan(), sActor, sPlayer);
		break;
	case SECTOR_FLOORPICNUM:
		if (bSet) sectp->setfloortexture(tileGetTextureID(lValue));
		else SetGameVarID(lVar2, legacyTileNum(sectp->floortexture), sActor, sPlayer);
		break;
	case SECTOR_FLOORSLOPE:
		if (bSet) sectp->setfloorslope(lValue);
		else SetGameVarID(lVar2, sectp->floorheinum, sActor, sPlayer);
		break;
	case SECTOR_FLOORSHADE:
		if (bSet) sectp->floorshade = lValue;
		else SetGameVarID(lVar2, sectp->floorshade, sActor, sPlayer);
		break;
	case SECTOR_FLOORPAL:
		if (bSet) sectp->floorpal = lValue;
		else SetGameVarID(lVar2, sectp->floorpal, sActor, sPlayer);
		break;
	case SECTOR_FLOORXPANNING:
		if (bSet) sectp->floorxpan_ = (float)(lValue & 255);
		else SetGameVarID(lVar2, sectp->floorxpan(), sActor, sPlayer);
		break;
	case SECTOR_FLOORYPANNING:
		if (bSet) sectp->floorypan_ = (float)(lValue & 255);
		else SetGameVarID(lVar2, sectp->floorypan(), sActor, sPlayer);
		break;
	case SECTOR_VISIBILITY:
		if (bSet) sectp->visibility = lValue;
		else SetGameVarID(lVar2, sectp->visibility, sActor, sPlayer);
		break;
	case SECTOR_LOTAG:
		if (bSet) sectp->lotag = lValue;
		else SetGameVarID(lVar2, sectp->lotag, sActor, sPlayer);
		break;
	case SECTOR_HITAG:
		if (bSet) sectp->hitag = lValue;
		else SetGameVarID(lVar2, sectp->hitag, sActor, sPlayer);
		break;
	case SECTOR_EXTRA:
		if (bSet) sectp->extra = lValue;
		else SetGameVarID(lVar2, sectp->extra, sActor, sPlayer);
		break;
	default:
		break;

	}
	return;
}

void DoActor(bool bSet, int lVar1, int lLabelID, int lVar2, DDukeActor* sActor, int sPlayer, int lParm2)
{
	auto vValue = GetGameVarID(lVar2, sActor, sPlayer);
	auto lValue = vValue.safeValue();

	DDukeActor* act;
	if (lVar1 == g_iThisActorID)
	{
		// if they've asked for 'this', then use 'this'...
		act = sActor;
	}
	else
	{
		act = GetGameVarID(lVar1, sActor, sPlayer).safeActor();
	}
	if (!act)
	{
		if (!bSet) SetGameVarID(lVar2, 0, sActor, sPlayer);
		return;
	}

	switch (lLabelID)
	{
	case ACTOR_X:
		if (!bSet) SetGameVarID(lVar2, int(act->spr.pos.X / maptoworld), sActor, sPlayer);
		break;
	case ACTOR_Y:
		if (!bSet) SetGameVarID(lVar2, int(act->spr.pos.Y / maptoworld), sActor, sPlayer);
		break;
	case ACTOR_Z:
		if (!bSet) SetGameVarID(lVar2, int(act->spr.pos.Z / zmaptoworld), sActor, sPlayer);
		break;
	case ACTOR_CSTAT:
		if (bSet) act->spr.cstat = ESpriteFlags::FromInt(lValue);
		else SetGameVarID(lVar2, act->spr.cstat, sActor, sPlayer);
		break;
	case ACTOR_SHADE:
		if (bSet) act->spr.shade = lValue;
		else SetGameVarID(lVar2, act->spr.shade, sActor, sPlayer);
		break;
	case ACTOR_PAL:
		if (bSet) act->spr.pal = lValue;
		else SetGameVarID(lVar2, act->spr.pal, sActor, sPlayer);
		break;
	case ACTOR_CLIPDIST:
		if (bSet) act->clipdist = lValue * 0.25;
		else SetGameVarID(lVar2, int(act->clipdist * 4), sActor, sPlayer);
		break;
	case ACTOR_DETAIL:
		if (bSet) act->spriteextra = lValue;
		else SetGameVarID(lVar2, act->spriteextra, sActor, sPlayer);
		break;
	case ACTOR_XREPEAT:
		if (bSet) act->spr.scale.X = (lValue * REPEAT_SCALE);
		else SetGameVarID(lVar2, int(act->spr.scale.X * INV_REPEAT_SCALE), sActor, sPlayer);
		break;
	case ACTOR_YREPEAT:
		if (bSet) act->spr.scale.Y = (lValue * REPEAT_SCALE);
		else SetGameVarID(lVar2, int(act->spr.scale.Y * INV_REPEAT_SCALE), sActor, sPlayer);
		break;
	case ACTOR_XOFFSET:
		if (bSet) act->spr.xoffset = lValue;
		else SetGameVarID(lVar2, act->spr.xoffset, sActor, sPlayer);
		break;
	case ACTOR_YOFFSET:
		if (bSet) act->spr.yoffset = lValue;
		else SetGameVarID(lVar2, act->spr.yoffset, sActor, sPlayer);
		break;
	case ACTOR_SECTNUM: // made read only because this is not safe.
		if (!bSet) /*changespritesect(iActor, lValue);
		else*/ SetGameVarID(lVar2, act->sectno(), sActor, sPlayer);
		break;
	case ACTOR_STATNUM: 
		if (!bSet) /*changespritestat(iActor, lValue);
		else*/ SetGameVarID(lVar2, act->spr.statnum, sActor, sPlayer);
		break;
	case ACTOR_ANG:
		if (bSet) act->spr.Angles.Yaw = DAngle::fromBuild(lValue);
		else SetGameVarID(lVar2, act->spr.Angles.Yaw.Buildang(), sActor, sPlayer);
		break;
	case ACTOR_OWNER:
		// there is no way to handle this well because we do not know whether this is an actor or not. Pity.
		if (bSet) act->spr.intowner = lValue;
		else SetGameVarID(lVar2, act->spr.intowner, sActor, sPlayer);
		break;
	case ACTOR_XVEL:
		if (bSet) act->vel.X = lValue * maptoworld;
		else SetGameVarID(lVar2, int(act->vel.X / maptoworld), sActor, sPlayer);
		break;
	case ACTOR_YVEL:
		if (bSet) act->spr.yint = lValue;
		else SetGameVarID(lVar2, act->spr.yint, sActor, sPlayer);
		break;
	case ACTOR_ZVEL:
		if (bSet) act->vel.Z = lValue * zmaptoworld;
		else SetGameVarID(lVar2, int(act->vel.Z / zmaptoworld), sActor, sPlayer);
		break;
	case ACTOR_LOTAG:
		if (bSet) act->spr.lotag = lValue;
		else SetGameVarID(lVar2, act->spr.lotag, sActor, sPlayer);
		break;
	case ACTOR_HITAG:
		if (bSet) act->spr.hitag = lValue;
		else SetGameVarID(lVar2, act->spr.hitag, sActor, sPlayer);
		break;
	case ACTOR_EXTRA:
		if (bSet) act->spr.extra = lValue;
		else SetGameVarID(lVar2, act->spr.extra, sActor, sPlayer);
		break;

	case ACTOR_HTCGG:
		if (bSet) act->cgg = lValue;
		else SetGameVarID(lVar2, act->cgg, sActor, sPlayer);
		break;
	case ACTOR_HTANG:
		if (bSet) act->hitang = mapangle(lValue);
		else SetGameVarID(lVar2, act->hitang.Buildang(), sActor, sPlayer);
		break;
	case ACTOR_HTEXTRA:
		if (bSet) act->hitextra = lValue;
		else SetGameVarID(lVar2, act->hitextra, sActor, sPlayer);
		break;
	case ACTOR_HTOWNER:
		if (bSet) act->hitOwnerActor = vValue.safeActor();
		else SetGameVarID(lVar2, act->hitOwnerActor, sActor, sPlayer);
		break;
	case ACTOR_HTMOVFLAG:
		if (bSet) act->movflag = lValue;
		else SetGameVarID(lVar2, act->movflag, sActor, sPlayer);
		break;
	case ACTOR_HTTEMPANG:
		if (bSet) act->tempval = lValue;
		else SetGameVarID(lVar2, act->tempval, sActor, sPlayer);
		break;
	case ACTOR_HTACTORSTAYPUT:
		if (bSet) act->actorstayput = toSect(lValue);
		else SetGameVarID(lVar2, fromSect(act->actorstayput), sActor, sPlayer);
		break;
	case ACTOR_HTTIMETOSLEEP:
		if (bSet) act->timetosleep = lValue;
		else SetGameVarID(lVar2, act->timetosleep, sActor, sPlayer);
		break;
	case ACTOR_HTFLOORZ:
		if (bSet) act->floorz = lValue * zmaptoworld;
		else SetGameVarID(lVar2, int(act->floorz * (1/zmaptoworld)), sActor, sPlayer);
		break;
	case ACTOR_HTCEILINGZ:
		if (bSet) act->ceilingz = lValue * zmaptoworld;
		else SetGameVarID(lVar2, int(act->ceilingz * (1/zmaptoworld)), sActor, sPlayer);
		break;
	case ACTOR_HTLASTVX:
		if (bSet) act->ovel.X = lValue * maptoworld;
		else SetGameVarID(lVar2, int(act->ovel.X / maptoworld), sActor, sPlayer);
		break;
	case ACTOR_HTLASTVY:
		if (bSet) act->ovel.Y = lValue * maptoworld;
		else SetGameVarID(lVar2, int(act->ovel.Y / maptoworld), sActor, sPlayer);
		break;
	case ACTOR_HTG_T0:
		if (bSet) act->counter = lValue;
		else SetGameVarID(lVar2, act->counter, sActor, sPlayer);
		break;
	case ACTOR_HTG_T1:
		if (bSet) act->temp_data[1] = lValue;
		else SetGameVarID(lVar2, act->temp_data[1], sActor, sPlayer);
		break;
	case ACTOR_HTG_T2:
		if (bSet) act->temp_data[2] = lValue;
		else SetGameVarID(lVar2, act->temp_data[2], sActor, sPlayer);
		break;
	case ACTOR_HTG_T3:
		if (bSet) act->temp_data[3] = lValue;
		else SetGameVarID(lVar2, act->temp_data[3], sActor, sPlayer);
		break;
	case ACTOR_HTG_T4:
		if (bSet) act->temp_data[4] = lValue;
		else SetGameVarID(lVar2, act->temp_data[4], sActor, sPlayer);
		break;
	case ACTOR_HTG_T5:
		if (bSet) act->temp_data[0] = lValue;
		else SetGameVarID(lVar2, act->temp_data[0], sActor, sPlayer);
		break;

	default:
		break;
	}
	return;
}

//---------------------------------------------------------------------------
//
// what a lousy hack job... :(
//
//---------------------------------------------------------------------------

int CheckWeapRec(DDukePlayer* p, DDukeActor* g_ac, int testonly)
{
	int j;
	for (j = 0; j < p->weapreccnt; j++)
		if (p->weaprecs[j] == g_ac->GetClass())
			break;

	if (testonly)
	{
		return (j < p->weapreccnt && g_ac->GetOwner() == g_ac);
	}
	else if (p->weapreccnt < 32)
	{
		p->weaprecs[p->weapreccnt++] = g_ac->GetClass();
		return (g_ac->GetOwner() == g_ac);
	}
	return false;
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void ParseState::parseifelse(int condition)
{
	if( condition )
	{
		// skip 'else' pointer.. and...
		insptr+=2;
		parse();
	}
	else
	{
		insptr = &ScriptCode[*(insptr+1)];
		if(*insptr == 10)
		{
			// else...

			// skip 'else' and...
			insptr+=2;
			parse();
		}
	}
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

static int ifcanshoottarget(DDukeActor *actor, int g_p, int g_x)
{
	if (g_x > 1024)
	{
		double sclip;
		DAngle angdif;

		if (badguy(actor) && actor->spr.scale.X > 0.875)
		{
			sclip = 3084 / 16.;
			angdif = DAngle22_5 * 3 / 8;
		}
		else
		{
			sclip = 48;
			angdif = DAngle22_5 / 8;
		}

		DDukeActor* hit;
		double hs = hitasprite(actor, &hit);
		if (hs == INT_MAX)
		{
			return 1;
		}
		if (hs > sclip)
		{
			if (hit != nullptr && hit->GetClass() == actor->GetClass())
				return 0;
			else
			{
				actor->spr.Angles.Yaw += angdif;
				hs = hitasprite(actor, &hit);
				actor->spr.Angles.Yaw -= angdif;
				if (hs > sclip)
				{
					if (hit != nullptr && hit->GetClass() == actor->GetClass())
						return 0;
					else
					{
						actor->spr.Angles.Yaw += angdif;
						hs = hitasprite(actor, &hit);
						actor->spr.Angles.Yaw -= angdif;
						if (hs > 48)
						{
							if (hit != nullptr && hit->GetClass() == actor->GetClass())
								return 0;
							return 1;
						}
						else return 0;
					}
				}
				else return 0;
			}
		}
		return 0;
	}
	return 1;
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

int ifcansee(DDukeActor* actor, DDukePlayer* p)
{
	int j;
	DDukeActor* tosee;

	// select sprite for monster to target
	// if holoduke is on, let them target holoduke first.
	// 
	if (p->holoduke_on != nullptr && !isRR())
	{
		tosee = p->holoduke_on;
		j = cansee(actor->spr.pos.plusZ(-zrand(32)), actor->sector(), tosee->spr.pos, tosee->sector());

		if (j == 0)
		{
			// they can't see player's holoduke
			// check for player..
			tosee = p->GetActor();
		}
	}
	else tosee = p->GetActor();	// holoduke not on. look for player

	// can they see player, (or player's holoduke)
	j = cansee(actor->spr.pos.plusZ(-zrand(48)), actor->sector(), tosee->spr.pos.plusZ(isRR()? -28 : -24), tosee->sector());

	if (j == 0)
	{
		// search around for target player
		// also modifies 'target' x&y if found.
		j = furthestcanseepoint(actor, tosee, actor->ovel);
	}
	else
	{
		// else, they did see it.
		// save where we were looking..
		actor->ovel = tosee->spr.pos.XY();
	}

	if (j == 1 && (actor->spr.statnum == STAT_ACTOR || actor->spr.statnum == STAT_STANDABLE))
		actor->timetosleep = SLEEPTIME;

	return j == 1;
}


// int *it = 0x00589a04;

int ParseState::parse(void)
{
	int j, l;

	if(killit_flag) return 1;

	const auto p = getPlayer(g_p);
	const auto pact = p->GetActor();

	switch (*insptr)
	{
	case concmd_ifrnd:
	{
		insptr++;
		// HACK ALERT! The fire animation uses a broken ifrnd setup to delay its start because original CON has no variables
		// But the chosen random value of 16/255 is too low and can cause delays of a second or more. (identical in Duke and RR.)
		if (g_ac->IsKindOf(RedneckFireClass) && g_ac->curAction->name == NAME_None && *insptr == 16)
		{
			parseifelse(rnd(64));
			break;
		}
		parseifelse(rnd(*insptr));
		break;
	}
	case concmd_ifcanshoottarget:
		parseifelse(ifcanshoottarget(g_ac, g_p, g_x));
		break;
	case concmd_ifcanseetarget:
		j = cansee(g_ac->spr.pos.plusZ(krand() & 41), g_ac->sector(), pact->getPosWithOffsetZ(), pact->sector());
		parseifelse(j);
		if (j) g_ac->timetosleep = SLEEPTIME;
		break;
	case concmd_ifnocover:
		j = cansee(g_ac->spr.pos, g_ac->sector(), pact->getPosWithOffsetZ(), pact->sector());
		parseifelse(j);
		if (j) g_ac->timetosleep = SLEEPTIME;
		break;

	case concmd_ifactornotstayput:
		parseifelse(g_ac->actorstayput == nullptr);
		break;
	case concmd_ifcansee:
		parseifelse(ifcansee(g_ac, p));
		break;

	case concmd_ifhitweapon:
		parseifelse(fi.ifhitbyweapon(g_ac) >= 0);
		break;
	case concmd_ifsquished:
		parseifelse(ifsquished(g_ac, g_p) == 1);
		break;
	case concmd_ifdead:
	{
		j = g_ac->spr.extra;
		if (g_ac->isPlayer())
			j--;
		parseifelse(j < 0);
	}
	break;
	case concmd_ai:
	{
		insptr++;
		auto ai = &ais[*insptr];
		g_ac->curAI = ai->name;
		g_ac->curAction = &actions[ai->action];
		g_ac->curMove = &moves[ai->move];
		g_ac->spr.hitag = ai->moveflags;
		g_ac->actioncounter = g_ac->curframe = 0;
		g_ac->counter = 0;
		if (g_ac->spr.hitag & random_angle)
			g_ac->spr.Angles.Yaw = randomAngle();
		insptr++;
		break;
	}
	case concmd_action:
		insptr++;
		g_ac->actioncounter = g_ac->curframe = 0;
		g_ac->curAction = &actions[*insptr];
		insptr++;
		break;

	case concmd_ifpdistl:
		insptr++;
		parseifelse(g_x < *insptr);
		if (g_x > MAXSLEEPDIST && g_ac->timetosleep == 0)
			g_ac->timetosleep = SLEEPTIME;
		break;
	case concmd_ifpdistg:
		insptr++;
		parseifelse(g_x > * insptr);
		if (g_x > MAXSLEEPDIST && g_ac->timetosleep == 0)
			g_ac->timetosleep = SLEEPTIME;
		break;
	case concmd_else:
		insptr = &ScriptCode[*(insptr + 1)];
		break;
	case concmd_addstrength:
		insptr++;
		g_ac->spr.extra += *insptr;
		insptr++;
		break;
	case concmd_strength:
		insptr++;
		g_ac->spr.extra = *insptr;
		insptr++;
		break;
	case concmd_smacksprite:
		switch (krand() & 1)
		{
		case 0:
			g_ac->spr.Angles.Yaw += DAngle90 + randomAngle(90);
			break;
		case 1:
			g_ac->spr.Angles.Yaw -= DAngle90 + randomAngle(90);
			break;
		}
		insptr++;
		break;
	case concmd_fakebubba:
		insptr++;
		fakebubbaspawn(g_ac, p);
		break;

	case concmd_rndmove:
		g_ac->spr.Angles.Yaw = randomAngle();
		g_ac->vel.X = 25/16.;
		insptr++;
		break;
	case concmd_mamatrigger:
		operateactivators(667, p);
		insptr++;
		break;
	case concmd_mamaspawn:
		mamaspawn(g_ac);
		insptr++;
		break;
	case concmd_mamaquake:
		if (g_ac->spr.pal == 31)
			ud.earthquaketime = 4;
		else if (g_ac->spr.pal == 32)
			ud.earthquaketime = 6;
		insptr++;
		break;
	case concmd_garybanjo:
		garybanjo(g_ac);
		insptr++;
		break;
	case concmd_motoloopsnd:
		if (!S_CheckActorSoundPlaying(g_ac, 411))
			S_PlayActorSound(411, g_ac, CHAN_VOICE);
		insptr++;
		break;
	case concmd_ifgotweaponce:
		insptr++;

		if (ud.coop >= 1 && ud.multimode > 1)
		{
			parseifelse(CheckWeapRec(p, g_ac, !*insptr));
		}
		else parseifelse(0);
		break;
	case concmd_getlastpal:
		insptr++;
		if (g_ac->isPlayer())
			g_ac->spr.pal = getPlayer(g_ac->PlayerIndex())->palookup;
		else
		{
			g_ac->spr.pal = (uint8_t)g_ac->tempval;
		}
		g_ac->tempval = 0;
		break;
	case concmd_tossweapon:
		insptr++;
		fi.checkweapons(getPlayer(g_ac->PlayerIndex()));
		break;
	case concmd_nullop:
		insptr++;
		break;
	case concmd_mikesnd:
		insptr++;
		if (!S_CheckActorSoundPlaying(g_ac, g_ac->spr.yint))
			S_PlayActorSound(g_ac->spr.yint, g_ac, CHAN_VOICE);
		break;
	case concmd_pkick:
		insptr++;
		playerkick(p, g_ac);
		break;
	case concmd_sizeto:
		insptr++;
		actorsizeto(g_ac, *insptr * REPEAT_SCALE, *(insptr + 1) * REPEAT_SCALE);
		insptr += 2;
		break;
	case concmd_sizeat:
		insptr++;
		g_ac->spr.scale.X = ((uint8_t)*insptr * REPEAT_SCALE);
		insptr++;
		g_ac->spr.scale.Y = ((uint8_t)*insptr * REPEAT_SCALE);
		insptr++;
		break;
	case concmd_shoot:
	{
		insptr++;
		int type = *insptr;
		// hackery to separate RRRA's merged projectiles. We'd prefer not to keep this mess in the core implementation.
		// Separating it here is easiest as long as the ZScript reimplementation explicitly takes care of these differences.
		if (isRRRA())
		{
			//if (g_ac->GetClass.TypeName == NAME_RedneckCheerleader && type == 3400) typecls = RedneckCheerBatonClass);
			//if (g_ac->GetClass.TypeName == NAME_RedneckMama && type == 3390) typecls = RedneckRabbitBallClass);
			//if (g_ac->GetClass.TypeName == NAME_RedneckMinion && type == 3390 && g_ac->spr.pal == 8) typecls = RedneckFrogBallClass);
			//if (g_ac->GetClass.TypeName == NAME_RedneckMinion && type == 3390 && g_ac->spr.pal == 19)) typecls = RedneckShitBurnClass);
		}
		shoot(g_ac, GetSpawnType(*insptr));
		insptr++;
		break;
	}
	case concmd_ifsoundid:
		insptr++;
		parseifelse((short)*insptr == ambienttags.SafeGet(g_ac->spr.detail, {}).lo);
		break;
	case concmd_ifsounddist:
		insptr++;
		if (*insptr == 0)
			parseifelse(ambienttags.SafeGet(g_ac->spr.detail, {}).hi > g_x);
		else if (*insptr == 1)
			parseifelse(ambienttags.SafeGet(g_ac->spr.detail, {}).hi < g_x);
		break;
	case concmd_soundtag:
		insptr++;
		S_PlayActorSound(ambienttags.SafeGet(g_ac->spr.detail, {}).lo, g_ac);
		break;
	case concmd_soundtagonce:
		insptr++;
		if (!S_CheckActorSoundPlaying(g_ac, ambienttags.SafeGet(g_ac->spr.detail, {}).lo))
			S_PlayActorSound(ambienttags.SafeGet(g_ac->spr.detail, {}).lo, g_ac);
		break;
	case concmd_soundonce:
		insptr++;
		if (!S_CheckSoundPlaying(*insptr++))
			S_PlayActorSound(*(insptr - 1), g_ac);
		break;
	case concmd_stopsound:
		insptr++;
		if (S_CheckSoundPlaying(*insptr))
			S_StopSound(*insptr);
		insptr++;
		break;
	case concmd_globalsound:
		insptr++;
		if (g_p == screenpeek || ud.coop == 1)
			S_PlayActorSound(*insptr, getPlayer(screenpeek)->GetActor());
		insptr++;
		break;
	case concmd_smackbubba:
		insptr++;
		if (!isRRRA() || g_ac->spr.pal != 105)
		{
			setnextmap(false);
		}
		break;
	case concmd_mamaend:
		insptr++;
		getPlayer(myconnectindex)->MamaEnd = 150;
		break;

	case concmd_ifactorhealthg:
		insptr++;
		parseifelse(g_ac->spr.extra > (short)*insptr);
		break;
	case concmd_ifactorhealthl:
		insptr++;
		parseifelse(g_ac->spr.extra < (short)*insptr);
		break;
	case concmd_sound:
		insptr++;
		S_PlayActorSound((short) *insptr,g_ac);
		insptr++;
		break;
	case concmd_tip:
		insptr++;
		p->tipincs = 26;
		break;
	case concmd_iftipcow:
	case concmd_ifhittruck: // both have the same code. Note that 'iftipcpw' ONLY works when used on the cow!
		if (g_ac->spriteextra == 1) // 
		{
			j = 1;
			g_ac->spriteextra++;
		}
		else
			j = 0;
		parseifelse(j > 0);
		break;
	case concmd_tearitup:
		insptr++;
		tearitup(g_ac->sector());
		break;
	case concmd_fall:
		insptr++;
		g_ac->spr.xoffset = 0;
		g_ac->spr.yoffset = 0;
		fall(g_ac, g_p);
		break;
	case concmd_enda:
	case concmd_break:
	case concmd_ends:
	case concmd_endevent:
		return 1;
	case concmd_rightbrace:
		insptr++;
		return 1;
	case concmd_addammo:
		insptr++;
		if (!playeraddammo(p, *insptr, *(insptr + 1))) killit_flag = 2;
		insptr += 2;
		break;
	case concmd_money:
		insptr++;
		lotsofstuff(g_ac,*insptr, DukeMoneyClass);
		insptr++;
		break;
	case concmd_mail:
		insptr++;
		lotsofstuff(g_ac,*insptr, DukeMailClass);
		insptr++;
		break;
	case concmd_sleeptime:
		insptr++;
		g_ac->timetosleep = (short)*insptr;
		insptr++;
		break;
	case concmd_paper:
		insptr++;
		lotsofstuff(g_ac,*insptr, DukePaperClass);
		insptr++;
		break;
	case concmd_addkills:
		insptr++;
		dokill(p, g_ac, *insptr++);
		break;
	case concmd_lotsofglass:
		insptr++;
		spriteglass(g_ac, *insptr);
		insptr++;
		break;
	case concmd_killit:
		insptr++;
		killit_flag = 1;
		break;
	case concmd_addweapon:
		insptr++;
		if (!playeraddweapon(p, *insptr, *(insptr + 1))) killit_flag = 2;
		insptr+=2;
		break;
	case concmd_debug:
		insptr++;
		Printf("%d\n",*insptr);
		insptr++;
		break;
	case concmd_endofgame:
		insptr++;
		p->timebeforeexit = *insptr;
		p->customexitsound = -1;
		ud.eog = true;
		insptr++;
		break;

	case concmd_isdrunk: // todo: move out to player_r.
		insptr++;
		playerdrink(p, *insptr++);
		break;
	case concmd_strafeleft:
		insptr++;
		movesprite_ex(g_ac, DVector3(-g_ac->spr.Angles.Yaw.Sin(), g_ac->spr.Angles.Yaw.Cos(), g_ac->vel.Z), CLIPMASK0, coll);
		break;
	case concmd_straferight:
		insptr++;
		movesprite_ex(g_ac, DVector3(g_ac->spr.Angles.Yaw.Sin(), -g_ac->spr.Angles.Yaw.Cos(), g_ac->vel.Z), CLIPMASK0, coll);
		break;
	case concmd_larrybird:
		insptr++;
		pact->spr.pos.Z = pact->sector()->ceilingz;
		break;
	case concmd_destroyit:
		insptr++;
		destroyit(g_ac);
		break;
	case concmd_iseat:
		insptr++;
		playereat(p, *insptr++, !!(g_ac->flags3 & SFLAG3_BIGHEALTH));
		break;
	case concmd_addphealth:
		insptr++;
		addphealth(p, *insptr++, !!(g_ac->flags3 & SFLAG3_BIGHEALTH));
		break;

	case concmd_state:
		{
			auto tempscrptr = insptr + 2;
			insptr = &ScriptCode[*(insptr + 1)];
			while (1) if (parse()) break;
			insptr = tempscrptr;
		}
		break;
	case concmd_leftbrace:
		insptr++;
		while (1) if (parse()) break;
		break;
	case concmd_move:
		g_ac->counter=0;
		insptr++;
		g_ac->curMove = &moves[*insptr];
		insptr++;
		g_ac->spr.hitag = *insptr;
		insptr++;
		if(g_ac->spr.hitag&random_angle)
			g_ac->spr.Angles.Yaw = randomAngle();
		break;
	case concmd_spawn:
		insptr++;
		if(g_ac->insector())
			spawnsprite(g_ac, *insptr);
		insptr++;
		break;
	case concmd_ifwasweapon:
	case concmd_ifspawnedby:	// these two are the same
		insptr++;
		parseifelse( g_ac->attackertype == GetSpawnType(*insptr));
		break;
	case concmd_ifai:
		insptr++;
		parseifelse(g_ac->curAI == ais[*insptr].name);
		break;
	case concmd_ifaction:
		insptr++;
		parseifelse((g_ac->curAction - actions.Data()) == *insptr);
		break;
	case concmd_ifactioncount:
		insptr++;
		parseifelse(g_ac->actioncounter >= *insptr);
		break;
	case concmd_resetactioncount:
		insptr++;
		g_ac->actioncounter = 0;
		break;
	case concmd_debris:
		insptr++;
		spawndebris(g_ac, *insptr - (isRR() ? SCRAP_RR : SCRAP_DUKE), *(insptr + 1)); // must subtract the index of the first scrap tile.
		insptr += 2;
		break;
	case concmd_count:
		insptr++;
		g_ac->counter = (short) *insptr;
		insptr++;
		break;
	case concmd_cstator:
		insptr++;
		g_ac->spr.cstat |= ESpriteFlags::FromInt(*insptr);
		insptr++;
		break;
	case concmd_clipdist:
		insptr++;
		g_ac->clipdist = ((uint8_t) *insptr) * 0.25;
		insptr++;
		break;
	case concmd_cstat:
		insptr++;
		g_ac->spr.cstat = ESpriteFlags::FromInt(*insptr);
		insptr++;
		break;
	case concmd_ifmove:
		insptr++;
		parseifelse((g_ac->curMove - moves.Data()) == *insptr);
		break;
	case concmd_resetplayer:
		insptr++;
		playerreset(p, g_ac);
		break;
	case concmd_ifcoop:
		parseifelse(ud.coop || numplayers > 2);
		break;
	case concmd_ifonmud:
		parseifelse(abs(g_ac->spr.pos.Z - g_ac->sector()->floorz) < 32 && (tilesurface(g_ac->sector()->floortexture) == TSURF_MUDDY) != 0);
		break;
	case concmd_ifonwater:
		parseifelse( abs(g_ac->spr.pos.Z-g_ac->sector()->floorz) < 32 && g_ac->sector()->lotag == ST_1_ABOVE_WATER);
		break;
	case concmd_ifmotofast:
		parseifelse(p->MotoSpeed > 60);
		break;
	case concmd_ifonmoto:
		parseifelse(p->OnMotorcycle == 1);
		break;
	case concmd_ifonboat:
		parseifelse(p->OnBoat == 1);
		break;
	case concmd_ifsizedown:
		g_ac->spr.scale.X -= REPEAT_SCALE;
		g_ac->spr.scale.Y -= REPEAT_SCALE;
		parseifelse(g_ac->spr.scale.X <= 5 * REPEAT_SCALE);
		break;
	case concmd_ifwind:
		parseifelse(WindTime > 0);
		break;

	case concmd_ifinwater:
		parseifelse( g_ac->sector()->lotag == ST_2_UNDERWATER);
		break;
	case concmd_ifcount:
		insptr++;
		parseifelse(g_ac->counter >= *insptr);
		break;
	case concmd_ifactor:
		insptr++;
		parseifelse(g_ac->GetClass() == GetSpawnType(*insptr));
		break;
	case concmd_resetcount:
		insptr++;
		g_ac->counter = 0;
		break;
	case concmd_addinventory:
		insptr++;
		playeraddinventory(p, g_ac, *insptr, *(insptr+1));
		insptr += 2;
		break;
	case concmd_hitradius:
		hitradius(g_ac, *(insptr + 1), *(insptr + 2), *(insptr + 3), *(insptr + 4), *(insptr + 5));
		insptr+=6;
		break;
	case concmd_ifp:
	{
			insptr++;

			l = *insptr;
			j = 0;

			double vel = g_ac->vel.X;

			// sigh.. this was yet another place where number literals were used as bit masks for every single value, making the code totally unreadable.
			if( (l& pducking) && p->on_ground && !!(p->cmd.ucmd.actions & SB_CROUCH))
					j = 1;
			else if( (l& pfalling) && p->jumping_counter == 0 && !p->on_ground &&	p->vel.Z > 8 )
					j = 1;
			else if( (l& pjumping) && p->jumping_counter > 348 )
					j = 1;
			else if( (l& pstanding) && vel >= 0 && vel < 0.5)
					j = 1;
			else if( (l& pwalking) && vel >= 0.5 && !(!!(p->cmd.ucmd.actions & SB_RUN)) )
					j = 1;
			else if( (l& prunning) && vel >= 0.5 && !!(p->cmd.ucmd.actions & SB_RUN) )
					j = 1;
			else if( (l& phigher) && pact->getOffsetZ() < g_ac->spr.pos.Z - 48)
					j = 1;
			else if( (l& pwalkingback) && vel <= -0.5 && !(!!(p->cmd.ucmd.actions & SB_RUN)) )
					j = 1;
			else if( (l& prunningback) && vel <= -0.5 && (!!(p->cmd.ucmd.actions & SB_RUN)) )
					j = 1;
			else if( (l& pkicking) && ( p->quick_kick > 0 || ( p->curr_weapon == KNEE_WEAPON && p->kickback_pic > 0 ) ) )
					j = 1;
			else if( (l& pshrunk) && pact->spr.scale.X < (isRR() ? 0.125 : 0.5))
					j = 1;
			else if( (l& pjetpack) && p->jetpack_on )
					j = 1;
			else if( (l& ponsteroids) && p->steroids_amount > 0 && p->steroids_amount < 400 )
					j = 1;
			else if( (l& ponground) && p->on_ground)
					j = 1;
			else if( (l& palive) && pact->spr.scale.X > (isRR() ? 0.125 : 0.5) && pact->spr.extra > 0 && p->timebeforeexit == 0)
					j = 1;
			else if( (l& pdead) && pact->spr.extra <= 0)
					j = 1;
			else if( (l& pfacing) )
			{
				DAngle ang;
				if (g_ac->isPlayer() && ud.multimode > 1)
				{
					const auto pact2 = getPlayer(otherp)->GetActor();
					ang = absangle(pact2->spr.Angles.Yaw, (pact->spr.pos.XY() - pact2->spr.pos.XY()).Angle());
				}
				else
				{
					ang = absangle(pact->spr.Angles.Yaw, (g_ac->spr.pos.XY() - pact->spr.pos.XY()).Angle());
				}

				j = ang < DAngle22_5;
			}

			parseifelse( j);

		}
		break;
	case concmd_ifstrength:
		insptr++;
		parseifelse(g_ac->spr.extra <= *insptr);
		break;
	case concmd_guts:
	{
		insptr += 2;
		auto clstype = GetSpawnType(*(insptr - 1));
		if (clstype) spawnguts(g_ac, clstype, *insptr);
		insptr++;
		break;
	}
	case concmd_slapplayer:
		insptr++;
		forceplayerangle(p);
		p->vel.XY() -= pact->spr.Angles.Yaw.ToVector() * 8;
		return 0;
	case concmd_wackplayer:
		insptr++;
		wackplayer(p);
		return 0;
	case concmd_ifgapzl:
		insptr++;
		parseifelse(int(g_ac->floorz - g_ac->ceilingz) < *insptr);	// Note: int cast here is needed to use the same truncation behavior as the old fixed point code.
		break;
	case concmd_ifhitspace:
		parseifelse(!!(p->cmd.ucmd.actions & SB_OPEN));
		break;
	case concmd_ifoutside:
		parseifelse(g_ac->sector()->ceilingstat & CSTAT_SECTOR_SKY);
		break;
	case concmd_ifmultiplayer:
		parseifelse(ud.multimode > 1);
		break;
	case concmd_operate:
		insptr++;
		actoroperate(g_ac);
		break;
	case concmd_ifinspace:
		parseifelse(ceilingspace(g_ac->sector()));
		break;

	case concmd_spritepal:
		insptr++;
		if(!g_ac->isPlayer())
			g_ac->tempval = g_ac->spr.pal;
		g_ac->spr.pal = *insptr;
		insptr++;
		break;

	case concmd_newpic:
	case concmd_cactor:
	{
		insptr++;
		auto info = spawnMap.CheckKey(*insptr);
		if (info != nullptr)
		{
			g_ac->ChangeType(info->cls);
			g_ac->spr.setspritetexture(tileGetTextureID(*insptr));
		}
		insptr++;
		break;
	}
	case concmd_ifbulletnear:
		parseifelse( dodge(g_ac) == 1);
		break;
	case concmd_ifrespawn:
		if( badguy(g_ac) )
			parseifelse( ud.respawn_monsters );
		else if( inventory(g_ac) )
			parseifelse( ud.respawn_inventory );
		else
			parseifelse( ud.respawn_items );
		break;
	case concmd_iffloordistl:
		insptr++;
		parseifelse(g_ac->floorz - g_ac->spr.pos.Z <= *insptr);
		break;
	case concmd_ifceilingdistl:
		insptr++;
		parseifelse(g_ac->spr.pos.Z - g_ac->ceilingz <= *insptr);
		break;
	case concmd_palfrom:
		insptr++;
		SetPlayerPal(p, PalEntry(insptr[0], insptr[1], insptr[2], insptr[3]));
		insptr += 4;
		break;

/*		  case 74:
		insptr++;
		getglobalz(g_ac);
		parseifelse( (( g_ac->floorz - g_ac->ceilingz ) >> 8 ) >= *insptr);
		break;
*/
	case concmd_addlog:
	{	int instr;
		int lFile;
		insptr++;
		lFile=*(insptr++);	// file
		instr=*(insptr++);	// line
		// this was only printing file name and line number as debug output.
		break;
	}
	case concmd_addlogvar:
	{	int instr;
		int lFile;
		insptr++;
		lFile=*(insptr++);	// file
		instr=*(insptr++);	// l=Line number, *instpr=varID
		if( (*insptr >= iGameVarCount)
			|| *insptr < 0
			)
		{
			// invalid varID
			insptr++;
			break;	// out of switch
		}
		DPrintf(DMSG_NOTIFY, "ADDLOGVAR: ");

		if( aGameVars[*insptr].dwFlags & GAMEVAR_FLAG_READONLY)
		{
			DPrintf(DMSG_NOTIFY, " (read-only)");
		}
		if( aGameVars[*insptr].dwFlags & GAMEVAR_FLAG_PERPLAYER)
		{
			DPrintf(DMSG_NOTIFY, " (Per Player. Player=%d)",g_p);
		}
		else if( aGameVars[*insptr].dwFlags & GAMEVAR_FLAG_PERACTOR)
		{
			DPrintf(DMSG_NOTIFY, " (Per Actor. Actor=%p)",g_ac);
		}
		else
		{
			DPrintf(DMSG_NOTIFY, " (Global)");
		}
		DPrintf(DMSG_NOTIFY, " =%d",	GetGameVarID(*insptr, g_ac, g_p).safeValue());
		insptr++;
		break;
	}
	case concmd_setvar:
	{	int i;
		insptr++;
		i=*(insptr++);	// ID of def
		SetGameVarID(i, *insptr, g_ac, g_p );
		insptr++;
		break;
	}
	case concmd_setvarvar:
	{	int i;
		insptr++;
		i=*(insptr++);	// ID of def
		SetGameVarID(i, GetGameVarID(*insptr, g_ac, g_p), g_ac, g_p );
//			aGameVars[i].lValue = aGameVars[*insptr].lValue;
		insptr++;
		break;
	}
	case concmd_addvar:
	{	int i;		
		insptr++;
		i=*(insptr++);	// ID of def
		SetGameVarID(i, GetGameVarID(i, g_ac, g_p).safeValue() + *insptr, g_ac, g_p );
		insptr++;
		break;
	}

	case concmd_addvarvar:
	{	int i;
		insptr++;
		i=*(insptr++);	// ID of def
		SetGameVarID(i, GetGameVarID(i, g_ac, g_p).safeValue() + GetGameVarID(*insptr, g_ac, g_p).safeValue(), g_ac, g_p );
		insptr++;
		break;
	}
	case concmd_ifvarvare:
	{
		int i;
		insptr++;
		i=*(insptr++);	// ID of def
		j=0;
		if(GetGameVarID(i, g_ac, g_p).safeValue() == GetGameVarID(*(insptr), g_ac, g_p).safeValue())
		{
			j=1;
		}
		parseifelse( j );
		break;
	}
	case concmd_ifvarvarg:
	{
		int i;
		insptr++;
		i=*(insptr++);	// ID of def
		j=0;
		if(GetGameVarID(i, g_ac, g_p).safeValue() > GetGameVarID(*(insptr), g_ac, g_p).safeValue())
		{
			j=1;
		}
		parseifelse( j );
		break;
	}
	case concmd_ifvarvarl:
	{
		int i;
		insptr++;
		i=*(insptr++);	// ID of def
		j=0;
		if(GetGameVarID(i, g_ac, g_p).safeValue() < GetGameVarID(*(insptr), g_ac, g_p).safeValue())
		{
			j=1;
		}
		parseifelse( j );
		break;
	}
	case concmd_ifvare:
	{
		int i;
		insptr++;
		i=*(insptr++);	// ID of def
		j=0;
		if(GetGameVarID(i, g_ac, g_p).safeValue() == *insptr)
		{
			j=1;
		}
		parseifelse( j );
		break;
	}
	case concmd_ifvarg:
	{
		int i;
		insptr++;
		i=*(insptr++);	// ID of def
		j=0;
		if(GetGameVarID(i, g_ac, g_p).safeValue() > *insptr)
		{
			j=1;
		}
		parseifelse( j );
		break;
	}
	case concmd_ifvarl:
	{
		int i;
		insptr++;
		i=*(insptr++);	// ID of def
		j=0;
		if(GetGameVarID(i, g_ac, g_p).safeValue() < *insptr)
		{
			j=1;
		}
		parseifelse( j );
		break;
	}
	case concmd_ifphealthl:
		insptr++;
		parseifelse( pact->spr.extra < *insptr);
		break;

	case concmd_ifpinventory:
	{
		insptr++;
		j = playercheckinventory(p, g_ac, *insptr, *(insptr + 1));
		insptr ++;
		parseifelse(j);
		break;
		}
	case concmd_pstomp:
		insptr++;
		if( p->knee_incs == 0 && pact->spr.scale.X >= (isRR()? 0.140625 : 0.625) )
			if (cansee(g_ac->spr.pos.plusZ(-4), g_ac->sector(), pact->getPosWithOffsetZ().plusZ(16), pact->sector()))
		{
			p->knee_incs = 1;
			if(p->weapon_pos == 0)
				p->weapon_pos = -1;
			p->actorsqu = g_ac;
		}
		break;
	case concmd_ifawayfromwall:
	{
		auto s1 = g_ac->sector();

		j = isAwayFromWall(g_ac, 6.75);
		parseifelse(j);
		break;
	}

	case concmd_quote:
		insptr++;
		FTA(*insptr,p);
		insptr++;
		break;
	case concmd_ifinouterspace:
		parseifelse( floorspace(g_ac->sector()));
		break;
	case concmd_ifnotmoving:
		parseifelse( g_ac->movflag > kHitSector );
		break;
	case concmd_respawnhitag:
		insptr++;
		respawnhitag(g_ac);
		break;
	case concmd_ifspritepal:
		insptr++;
		parseifelse( g_ac->spr.pal == *insptr);
		break;

	case concmd_ifangdiffl:
		{
		insptr++;
		auto ang = absangle(pact->spr.Angles.Yaw, g_ac->spr.Angles.Yaw);
		parseifelse( ang <= mapangle(*insptr));
		break;
		}

	case concmd_ifnosounds:
		parseifelse(!S_CheckAnyActorSoundPlaying(g_ac));
		break;

	case concmd_ifplaybackon: //Twentieth Anniversary World Tour
		parseifelse(false);
		break;

	case concmd_espawnvar:
	{
		DDukeActor* lReturn = nullptr;
		int lIn;
		insptr++;

		lIn = *insptr++;
		lIn = GetGameVarID(lIn, g_ac, g_p).safeValue();
		if(g_ac->insector())
			lReturn = spawn(g_ac, GetSpawnType(lIn));

		SetGameVarID(g_iReturnVarID, (lReturn), g_ac, g_p);
		break;
	}
	case concmd_espawn:
	{
		DDukeActor* lReturn = nullptr;
		insptr++;
		if(g_ac->insector())
			lReturn = spawn(g_ac, GetSpawnType(*insptr));
		insptr++;
		SetGameVarID(g_iReturnVarID, (lReturn), g_ac, g_p);
		break;
	}
	case concmd_setsector:
	case concmd_getsector:
	{
		// syntax [gs]etsector[<var>].x <VAR>
		// <varid> <xxxid> <varid>
		int lLabelID;
		int lVar1, lVar2;
		int lWhat;
		int lParm2;

		lWhat = *(insptr++);
		lVar1 = *(insptr++);
		lLabelID = *(insptr++);
		lParm2 = *(insptr++);
		lVar2 = *(insptr++);
		DoSector(lWhat == concmd_setsector, lVar1, lLabelID, lVar2, g_ac, g_p, lParm2);
		break;
	}
	case concmd_sqrt:
	{
		// syntax sqrt <invar> <outvar>

		int lInVarID;
		int lOutVarID;
		int lIn;

		insptr++;
		lInVarID = *(insptr++);
		lOutVarID = *(insptr++);
		lIn = GetGameVarID(lInVarID, g_ac, g_p).safeValue();
		SetGameVarID(lOutVarID, ksqrt(lIn), g_ac, g_p);
		break;
	}
	case concmd_findnearactor:
	{
		// syntax findnearactorvar <type> <maxdist> <getvar>
		// gets the sprite ID of the nearest actor within max dist
		// that is of <type> into <getvar>
		// -1 for none found
		// <type> <maxdist> <varid>
		int lType;
		int lMaxDist;
		int lVarID;
		double lTemp;
		double lDist;

		insptr++;

		lType = *(insptr++);
		lMaxDist = *(insptr++);
		lVarID = *(insptr++);

		DDukeActor* lFound = nullptr;
		lDist = 1000000;	// big number

		DukeStatIterator it(STAT_ACTOR);
		while (auto actj = it.Next())
		{
			if (actj->GetClass() == GetSpawnType(lType))
			{
				lTemp = (g_ac->spr.pos.XY() - actj->spr.pos.XY()).Length();
				if (lTemp < lMaxDist)
				{
					if (lTemp < lDist)
					{
						lFound = actj;
					}
				}

			}
		}
		SetGameVarID(lVarID, (lFound), g_ac, g_p);

		break;
	}
	case concmd_findnearactorvar:
	{
		// syntax findnearactorvar <type> <maxdistvar> <getvar>
		// gets the sprite ID of the nearest actor within max dist
		// that is of <type> into <getvar>
		// -1 for none found
		// <type> <maxdistvarid> <varid>
		int lType;
		int lMaxDistVar;
		int lMaxDist;
		int lVarID;
		double lTemp;
		double lDist;

		insptr++;

		lType = *(insptr++);
		lMaxDistVar = *(insptr++);
		lVarID = *(insptr++);
		lMaxDist = GetGameVarID(lMaxDistVar, g_ac, g_p).safeValue();
		DDukeActor* lFound = nullptr;
		lDist = 1000000;	// big number

		DukeStatIterator it(STAT_ACTOR);
		while (auto actj = it.Next())
		{
			if (actj->GetClass() == GetSpawnType(lType))
			{
				lTemp = (g_ac->spr.pos.XY() - actj->spr.pos.XY()).Length();
				if (lTemp < lMaxDist)
				{
					if (lTemp < lDist)
					{
						lFound = actj;
					}
				}

			}
		}
		SetGameVarID(lVarID, (lFound), g_ac, g_p);

		break;
	}
	case concmd_setplayer:
	case concmd_getplayer:
	{
		// syntax [gs]etplayer[<var>].x <VAR>
		// <varid> <xxxid> <varid>
		int lLabelID;
		int lVar1, lVar2;
		int lWhat;
		int lParm2;

		lWhat = *(insptr++);
		lVar1 = *(insptr++);
		lLabelID = *(insptr++);
		lParm2 = *(insptr++);
		lVar2 = *(insptr++);

		DoPlayer(lWhat == concmd_setplayer, lVar1, lLabelID, lVar2, g_ac, g_p, lParm2);
		break;
	}
	case concmd_getuserdef:
	case concmd_setuserdef:
	{
		// syntax [gs]etuserdef.xxx <VAR>
		//  <xxxid> <varid>
		int lLabelID;
		int lVar1, lVar2;
		int lWhat;
		int lParm2;

		lWhat = *(insptr++);
		lVar1 = -1;
		lLabelID = *(insptr++);
		lParm2 = *(insptr++);
		lVar2 = *(insptr++);

		DoUserDef(lWhat == concmd_setuserdef, lVar1, lLabelID, lVar2, g_ac, g_p, lParm2);
		break;
	}
	case concmd_setwall:
	case concmd_getwall:
	{
		// syntax [gs]etwall[<var>].x <VAR>
		// <varid> <xxxid> <varid>
		int lLabelID;
		int lVar1, lVar2;
		int lWhat;
		int lParm2;

		lWhat = *(insptr++);
		lVar1 = *(insptr++);
		lLabelID = *(insptr++);
		lParm2 = *(insptr++);
		lVar2 = *(insptr++);

		DoWall(lWhat == concmd_setwall, lVar1, lLabelID, lVar2, g_ac, g_p, lParm2);
		break;
	}
	case concmd_setactorvar:
	{
		// syntax [gs]etactorvar[<var>].<varx> <VAR>
		// gets the value of the per-actor variable varx into VAR
		// <var> <varx> <VAR>
		int lVar1, lVar2, lVar3;

		insptr++;

		lVar1 = *(insptr++);
		lVar2 = *(insptr++);
		lVar3 = *(insptr++);

		auto lSprite = GetGameVarID(lVar1, g_ac, g_p);
		if (lSprite.isActor())
		{
			auto lTemp = GetGameVarID(lVar3, g_ac, g_p);
			SetGameVarID(lVar2, lTemp, lSprite.actor(), g_p);
		}

		break;
	}
	case concmd_getactorvar:
	{
		// syntax [gs]etactorvar[<var>].<varx> <VAR>
		// gets the value of the per-actor variable varx into VAR
		// <var> <varx> <VAR>
		int lVar1, lVar2, lVar3;

		insptr++;

		lVar1 = *(insptr++);
		lVar2 = *(insptr++);
		lVar3 = *(insptr++);

		auto lSprite = GetGameVarID(lVar1, g_ac, g_p);
		if (lSprite.isActor())
		{
			auto lTemp = GetGameVarID(lVar2, lSprite.actor(), g_p);
			SetGameVarID(lVar3, lTemp, g_ac, g_p);
		}

		break;
	}
	case concmd_setactor:
	case concmd_getactor:
	{
		// syntax [gs]etactor[<var>].x <VAR>
		// <varid> <xxxid> <varid>
		int lLabelID;
		int lVar1, lVar2;
		int lWhat;
		int lParm2;

		lWhat = *(insptr++);
		lVar1 = *(insptr++);
		lLabelID = *(insptr++);
		lParm2 = *(insptr++);
		lVar2 = *(insptr++);

		DoActor(lWhat == concmd_setactor, lVar1, lLabelID, lVar2, g_ac, g_p, lParm2);
		break;
	}
	case concmd_getangletotarget:
	{
		insptr++;
		int i = *(insptr++);	// ID of def

		// g_ac->lastvx and lastvy are last known location of target.
		int ang = (g_ac->ovel - g_ac->spr.pos.XY()).Angle().Buildang();
		SetGameVarID(i, ang, g_ac, g_p);
		break;
	}
	case concmd_lockplayer:
	{
		int i;
		insptr++;
		i = *(insptr++);	// ID of def
		p->transporter_hold = GetGameVarID(i, g_ac, g_p).safeValue();
		break;
	}
	case concmd_getplayerangle:
	{
		int i;
		insptr++;
		i = *(insptr++);	// ID of def
		SetGameVarID(i, pact->spr.Angles.Yaw.Buildang(), g_ac, g_p);
		break;
	}
	case concmd_setplayerangle:
	{
		int i;
		insptr++;
		i = *(insptr++);	// ID of def
		pact->spr.Angles.Yaw = mapangle(GetGameVarID(i, g_ac, g_p).safeValue() & 2047);
		break;
	}
	case concmd_getactorangle:
	{
		int i;
		insptr++;
		i = *(insptr++);	// ID of def
		SetGameVarID(i, g_ac->spr.Angles.Yaw.Buildang(), g_ac, g_p);
		break;
	}
	case concmd_setactorangle:
	{
		int i;
		insptr++;
		i = *(insptr++);	// ID of def
		g_ac->spr.Angles.Yaw = DAngle::fromBuild(GetGameVarID(i, g_ac, g_p).safeValue() & 2047);
		break;
	}
	case concmd_randvar:
	{
		int i;
		insptr++;
		i = *(insptr++);	// ID of def
		SetGameVarID(i, MulScale(rand(), *insptr, 15), g_ac, g_p);
		insptr++;
		break;
	}
	case concmd_mulvar:
	{
		int i;
		insptr++;
		i = *(insptr++);	// ID of def
		SetGameVarID(i, GetGameVarID(i, g_ac, g_p).safeValue()* (*insptr), g_ac, g_p);
		insptr++;
		break;
	}
	case concmd_divvar:
	{
		int i;
		insptr++;
		i = *(insptr++);	// ID of def
		if ((*insptr) == 0)
		{
			I_Error("Divide by Zero in CON.");
		}
		SetGameVarID(i, GetGameVarID(i, g_ac, g_p).safeValue() / (*insptr), g_ac, g_p);
		insptr++;
		break;
	}
	case concmd_modvar:
	{
		int i;
		int instr;
		int lResult;
		insptr++;
		i = *(insptr++);	// ID of def
		instr = (*insptr);
		if (instr == 0)
		{
			I_Error("Divide by Zero in CON");
		}
		lResult = GetGameVarID(i, g_ac, g_p).safeValue() % instr;
		SetGameVarID(i, lResult, g_ac, g_p);
		insptr++;
		break;
	}
	case concmd_andvar:
	{
		int i;
		int instr;
		int lResult;
		insptr++;
		i = *(insptr++);	// ID of def
		instr = (*insptr);
		lResult = GetGameVarID(i, g_ac, g_p).safeValue() & instr;
		SetGameVarID(i, lResult, g_ac, g_p);
		insptr++;
		break;
	}
	case concmd_xorvar:
	{
		int i;
		int instr;
		int lResult;
		insptr++;
		i = *(insptr++);	// ID of def
		instr = (*insptr);
		lResult = GetGameVarID(i, g_ac, g_p).safeValue() ^ instr;
		SetGameVarID(i, lResult, g_ac, g_p);
		insptr++;
		break;
	}
	case concmd_orvar:
	{
		int i;
		int instr;
		int lResult;
		insptr++;
		i = *(insptr++);	// ID of def
		instr = (*insptr);
		lResult = GetGameVarID(i, g_ac, g_p).safeValue() | instr;
		SetGameVarID(i, lResult, g_ac, g_p);
		insptr++;
		break;
	}
	case concmd_randvarvar:
	{
		int i;
		int l1, l2;
		int lResult;
		insptr++;
		i = *(insptr++);	// ID of def
		l1 = GetGameVarID(i, g_ac, g_p).safeValue(); // not used for this command
		l2 = GetGameVarID(*insptr, g_ac, g_p).safeValue();
		lResult = MulScale(rand(), l2, 15);
		SetGameVarID(i, lResult, g_ac, g_p);
		insptr++;
		break;
	}
	case concmd_gmaxammo:
	{
		int i;
		int l1, l2;
		int lResult;
		insptr++;
		i = *(insptr++);	// ID of def
		l1 = GetGameVarID(i, g_ac, g_p).safeValue();
		l2 = GetGameVarID(*insptr, g_ac, g_p).safeValue(); // l2 not used in this one
		lResult = gs.max_ammo_amount[l1];
		SetGameVarID(*insptr, lResult, g_ac, g_p);
		insptr++;
		break;
	}
	case concmd_smaxammo:
	{
		int i;
		int l1, l2;
		insptr++;
		i = *(insptr++);	// ID of def
		l1 = GetGameVarID(i, g_ac, g_p).safeValue();
		l2 = GetGameVarID(*insptr, g_ac, g_p).safeValue();
		gs.max_ammo_amount[l1] = l2;

		insptr++;
		break;
	}
	case concmd_mulvarvar:
	{
		int i;
		int l1, l2;
		int lResult;
		insptr++;
		i = *(insptr++);	// ID of def
		l1 = GetGameVarID(i, g_ac, g_p).safeValue();
		l2 = GetGameVarID(*insptr, g_ac, g_p).safeValue();
		lResult = l1 * l2;
		SetGameVarID(i, lResult, g_ac, g_p);
		insptr++;
		break;
	}
	case concmd_divvarvar:
	{
		int i;
		int l1, l2;
		int lResult;
		insptr++;
		i = *(insptr++);	// ID of def
		l1 = GetGameVarID(i, g_ac, g_p).safeValue();
		l2 = GetGameVarID(*insptr, g_ac, g_p).safeValue();
		if (l2 == 0)
		{
			I_Error("Divide by Zero in CON");
		}
		lResult = l1 / l2;
		SetGameVarID(i, lResult, g_ac, g_p);
		insptr++;
		break;
	}
	case concmd_modvarvar:
	{
		int i;
		int l1, l2;
		int lResult;
		insptr++;
		i = *(insptr++);	// ID of def
		l1 = GetGameVarID(i, g_ac, g_p).safeValue();
		l2 = GetGameVarID(*insptr, g_ac, g_p).safeValue();
		if (l2 == 0)
		{
			I_Error("Mod by Zero in CON");
		}
		lResult = l1 % l2;
		SetGameVarID(i, lResult, g_ac, g_p);
		insptr++;
		break;
	}
	case concmd_andvarvar:
	{
		int i;
		int l1, l2;
		int lResult;
		insptr++;
		i = *(insptr++);	// ID of def
		l1 = GetGameVarID(i, g_ac, g_p).safeValue();
		l2 = GetGameVarID(*insptr, g_ac, g_p).safeValue();
		lResult = l1 & l2;
		SetGameVarID(i, lResult, g_ac, g_p);
		insptr++;
		break;
	}
	case concmd_xorvarvar:
	{
		int i;
		int l1, l2;
		int lResult;
		insptr++;
		i = *(insptr++);	// ID of def
		l1 = GetGameVarID(i, g_ac, g_p).safeValue();
		l2 = GetGameVarID(*insptr, g_ac, g_p).safeValue();
		lResult = l1 ^ l2;
		SetGameVarID(i, lResult, g_ac, g_p);
		insptr++;
		break;
	}
	case concmd_orvarvar:
	{
		int i;
		int l1, l2;
		int lResult;
		insptr++;
		i = *(insptr++);	// ID of def
		l1 = GetGameVarID(i, g_ac, g_p).safeValue();
		l2 = GetGameVarID(*insptr, g_ac, g_p).safeValue();
		lResult = l1 | l2;
		SetGameVarID(i, lResult, g_ac, g_p);
		insptr++;
		break;
	}
	case concmd_subvar:
	{
		int i;
		insptr++;
		i = *(insptr++);	// ID of def
		SetGameVarID(i, GetGameVarID(i, g_ac, g_p).safeValue() - *insptr, g_ac, g_p);
		insptr++;
		break;
	}
	case concmd_subvarvar:
	{
		int i;
		insptr++;
		i = *(insptr++);	// ID of def
		SetGameVarID(i, GetGameVarID(i, g_ac, g_p).safeValue() - GetGameVarID(*insptr, g_ac, g_p).safeValue(), g_ac, g_p);
		insptr++;
		break;
	}
	case concmd_sin:
	{
		int i;
		int lValue;
		insptr++;
		i = *(insptr++);	// ID of def
		lValue = int(16384 * BobVal(GetGameVarID(*insptr, g_ac, g_p).safeValue()));
		SetGameVarID(i, lValue, g_ac, g_p);
		insptr++;
		break;
	}

	case concmd_spgetlotag:
	{
		insptr++;
		SetGameVarID(g_iLoTagID, g_ac->spr.lotag, g_ac, g_p);
		break;
	}
	case concmd_spgethitag:
	{
		insptr++;
		SetGameVarID(g_iHiTagID, g_ac->spr.hitag, g_ac, g_p);
		break;
	}
	case concmd_sectgetlotag:
	{
		insptr++;
		SetGameVarID(g_iLoTagID, g_ac->sector()->lotag, g_ac, g_p);
		break;
	}
	case concmd_sectgethitag:
	{
		insptr++;
		SetGameVarID(g_iHiTagID, g_ac->sector()->hitag, g_ac, g_p);
		break;
	}
	case concmd_gettexturefloor:
	{
		insptr++;
		SetGameVarID(g_iTextureID, legacyTileNum(g_ac->sector()->floortexture), g_ac, g_p);
		break;
	}

	case concmd_startlevel:
	{
		// from 'level' cheat in game.c (about line 6250)
		int volnume;
		int levnume;

		insptr++; // skip command
		volnume = GetGameVarID(*insptr++, g_ac, g_p).safeValue();
		levnume = GetGameVarID(*insptr++, g_ac, g_p).safeValue();
		auto level = FindMapByIndex(volnume, levnume);
		if (level != nullptr)
			ChangeLevel(level, g_nextskill);
		break;
	}
	case concmd_myosx:
	case concmd_myospalx:

	case concmd_myos:
	case concmd_myospal:
	{
		int x, y;
		int tilenum;
		int shade;
		int orientation;
		int pal;
		int tw = *insptr++;
		x = GetGameVarID(*insptr++, g_ac, g_p).safeValue();
		y = GetGameVarID(*insptr++, g_ac, g_p).safeValue();
		tilenum = GetGameVarID(*insptr++, g_ac, g_p).safeValue();
		shade = GetGameVarID(*insptr++, g_ac, g_p).safeValue();
		orientation = GetGameVarID(*insptr++, g_ac, g_p).safeValue();
		if (tw == concmd_myospal)
		{
			pal = GetGameVarID(*insptr++, g_ac, g_p).safeValue();
			//myospal(x, y, tilenum, shade, orientation, pal);
		}
		else if (tw == concmd_myos)
		{
			//myos(x, y, tilenum, shade, orientation);
		}
		else if (tw == concmd_myosx)
		{
			//myos640(x, y, tilenum, shade, orientation);
		}
		else if (tw == concmd_myospalx)
		{
			pal = GetGameVarID(*insptr++, g_ac, g_p).safeValue();
			//myospal640(x, y, tilenum, shade, orientation, pal);
		}
		break;
	}

	case concmd_displayrand:
	{
		int i;
		insptr++;

		i = *(insptr++);	// ID of def
		SetGameVarID(i, rand(), g_ac, g_p);
		break;
	}
	case concmd_switch:
	{
		int lVarID;
		int lValue;
		int* lpDefault;
		int* lpCases;
		int lCases;
		int lEnd;
		int lCheckCase;
		bool bMatched;
		int* lTempInsPtr;

		// command format:
		// variable ID to check
		// script offset to 'end'
		// count of case statements
		// script offset to default case (null if none)
		// For each case: value, ptr to code
		insptr++; // p-code
		lVarID = *insptr++;
		lValue = GetGameVarID(lVarID, g_ac, g_p).safeValue();
		lEnd = *insptr++;
		lCases = *insptr++;
		lpDefault = insptr++;
		lpCases = insptr;
		insptr += lCases * 2;
		bMatched = false;
		lTempInsPtr = insptr;
		for (lCheckCase = 0; lCheckCase < lCases && !bMatched; lCheckCase++)
		{
			if (lpCases[lCheckCase * 2] == lValue)
			{
				insptr = &ScriptCode[lpCases[lCheckCase * 2 + 1]];
				while (1)
				{
					if (parse())
						break;
				}
				bMatched = true;
			}
		}
		if (!bMatched)
		{
			if (*lpDefault)
			{
				insptr = &ScriptCode[*lpDefault];
				while (1) if (parse()) break;
			}
			else
			{
				//AddLog("No Matching Case: No Default to use");
			}
		}
		insptr = &ScriptCode[lEnd];
		break;
	}
	case concmd_endswitch:
		insptr++;
		return 1;
		break;

	case concmd_starttrack:
	{
		insptr++;
		int music_select = *insptr++;
 		auto level = FindMapByIndex(currentLevel->cluster, music_select+1); // this was 0-based in EDuke 2.0...
		if (level) S_PlayLevelMusic(level);
		break;
	}
	case concmd_gettextureceiling:
	{
		insptr++;
		SetGameVarID(g_iTextureID, legacyTileNum(g_ac->sector()->ceilingtexture), g_ac, g_p);
		break;
	}
	case concmd_ifvarvarand:
	{
		int i;
		insptr++;
		i = *(insptr++);	// ID of def
		j = 0;
		if (GetGameVarID(i, g_ac, g_p).safeValue() & GetGameVarID(*(insptr), g_ac, g_p).safeValue())
		{
			j = 1;
		}
		parseifelse(j);
		break;
	}
	case concmd_ifvarvarn:
	{
		int i;
		insptr++;
		i = *(insptr++);	// ID of def
		j = 0;
		if (GetGameVarID(i, g_ac, g_p).safeValue() != GetGameVarID(*(insptr), g_ac, g_p).safeValue())
		{
			j = 1;
		}
		parseifelse(j);
		break;
	}
	case concmd_ifvarn:
	{
		int i;
		insptr++;
		i = *(insptr++);	// ID of def
		j = 0;
		if (GetGameVarID(i, g_ac, g_p).safeValue() != *insptr)
		{
			j = 1;
		}
		parseifelse(j);
		break;
	}
	case concmd_ifvarand:
	{
		int i;
		insptr++;
		i = *(insptr++);	// ID of def
		j = 0;
		if (GetGameVarID(i, g_ac, g_p).safeValue() & *insptr)
		{
			j = 1;
		}
		parseifelse(j);
		break;
	}

	default:
		Printf(TEXTCOLOR_RED "Unrecognized PCode of %d  in parse.  Killing current sprite.\n",*insptr);
		Printf(TEXTCOLOR_RED "Offset=%0X\n",int(insptr-ScriptCode.Data()));
		killit_flag = 1;
		break;
	}
	return 0;
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

CVAR(Bool, overridecon, false, 0)	// This is for debugging the CON replacement code only.

void LoadActor(DDukeActor *actor, int p, int x)
{
	int done;

	ParseState s;
	s.g_p = p;	// Player ID
	s.g_x = x;	// ??
	s.g_ac = actor;
	s.killit_flag = 0;

	if ((actor->flags4 & SFLAG4_CONOVERRIDE) && overridecon) return;
	auto coninf = actor->conInfo();
	if (coninf == nullptr) return;
	auto addr = coninf->loadeventscriptptr;
	if (addr == 0) return;


	if(!actor->insector())
	{
		actor->Destroy();
		return;
	}
	do
		done = s.parse();
	while (done == 0);

	if (s.killit_flag == 1)
	{
		actor->Destroy();
	}
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

bool execute(DDukeActor *actor,int p,double pdist, int* killit_flag)
{
	if (killit_flag) *killit_flag = 0;
	if ((actor->flags4 & SFLAG4_CONOVERRIDE) && overridecon) return false;

	auto coninf = actor->conInfo();
	if (coninf == nullptr) 
		return false;

	ParseState s;
	s.g_p = p;	// Player ID
	s.g_x = int(pdist / maptoworld);	// ??
	s.g_ac = actor;
	s.killit_flag = 0;
	s.insptr = coninf? &ScriptCode[4 + coninf->scriptaddress] : nullptr;

	int done;
	do
		done = s.parse();
	while( done == 0 );
	if (killit_flag) *killit_flag = s.killit_flag;


	return true;
}


//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void OnEvent(int iEventID, int p, DDukeActor *actor, int x)
{
	int done;

	if (iEventID >= MAXGAMEEVENTS)
	{
		Printf("Invalid Event ID\n");
		return;
	}
	if (apScriptGameEvent[iEventID] == 0)
	{
		return;
	}

	ParseState s;
	s.g_p = p;	/// current player ID
	s.g_x = x;	// ?
	s.g_ac = actor;

	s.insptr = &ScriptCode[apScriptGameEvent[iEventID]];

	s.killit_flag = 0;
	do
		done = s.parse();
	while (done == 0);
}

END_DUKE_NS
