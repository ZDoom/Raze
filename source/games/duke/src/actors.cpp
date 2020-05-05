//-------------------------------------------------------------------------
/*
Copyright (C) 1996, 2003 - 3D Realms Entertainment
Copyright (C) 2000, 2003 - Matt Saettler (EDuke Enhancements)
Copyright (C) 2017-2019 - Nuke.YKT
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

This file is a combination of code from the following sources:
- EDuke 2 by Matt Saettler
- JFDuke by Jonathon Fowler (jf@jonof.id.au),
- DukeGDX and RedneckGDX by Alexander Makarov-[M210] (m210-2007@mail.ru)
- Redneck Rampage reconstructed source by Nuke.YKT

*/
//-------------------------------------------------------------------------

#include "ns.h"
#include "global.h"
#include "names.h"

BEGIN_DUKE_NS

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

bool ceilingspace(int sectnum)
{
	if ((sector[sectnum].ceilingstat & 1) && sector[sectnum].ceilingpal == 0)
	{
		switch (sector[sectnum].ceilingpicnum)
		{
		case MOONSKY1:
		case BIGORBIT1:
			return !(g_gameType & GAMEFLAG_RRALL);

		case RR_MOONSKY1:
		case RR_BIGORBIT1:
			return !!(g_gameType & GAMEFLAG_RRALL);
		}
	}
	return 0;
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

bool floorspace(int sectnum)
{
	if ((sector[sectnum].floorstat & 1) && sector[sectnum].ceilingpal == 0)
	{
		switch (sector[sectnum].floorpicnum)
		{
		case MOONSKY1:
		case BIGORBIT1:
			return !(g_gameType & GAMEFLAG_RRALL);

		case RR_MOONSKY1:
		case RR_BIGORBIT1:
			return !!(g_gameType & GAMEFLAG_RRALL);
		}
	}
	return 0;
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void addammo(short weapon, struct player_struct* p, short amount)
{
	p->ammo_amount[weapon] += amount;

	if (p->ammo_amount[weapon] > max_ammo_amount[weapon])
		p->ammo_amount[weapon] = max_ammo_amount[weapon];
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void addweapon(struct player_struct* p, int weapon)
{
	short cw = p->curr_weapon;
	if (p->OnMotorcycle || p->OnBoat)
	{
		p->gotweapon.Set(weapon);
		if (weapon == SHRINKER_WEAPON)
		{
			p->gotweapon.Set(GROW_WEAPON);
			p->ammo_amount[GROW_WEAPON] = 1;
		}
		else if (weapon == RPG_WEAPON)
		{
			p->gotweapon.Set(RA16_WEAPON);
		}
		else if (weapon == RA15_WEAPON)
		{
			p->ammo_amount[RA15_WEAPON] = 1;
		}
		return;
	}

	if (p->gotweapon[weapon] == 0)
	{
		p->gotweapon.Set(weapon);
		if (weapon == SHRINKER_WEAPON)
		{
			p->gotweapon.Set(GROW_WEAPON);
			if (g_gameType & GAMEFLAG_RRRA) p->ammo_amount[GROW_WEAPON] = 1;
		}
		if (g_gameType & GAMEFLAG_RRRA)
		{
			if (weapon == RPG_WEAPON)
			{
				p->gotweapon.Set(RA16_WEAPON);
			}
			if (weapon == RA15_WEAPON)
			{
				p->ammo_amount[RA15_WEAPON] = 50;
			}
		}

		if (!(g_gameType & GAMEFLAG_RRALL) || weapon != HANDBOMB_WEAPON)
			cw = weapon;
	}
	else
		cw = weapon;

	if ((g_gameType & GAMEFLAG_RRALL) && weapon == HANDBOMB_WEAPON)
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

	p->kickback_pic = 0;
#ifdef EDUKE
	if (p->curr_weapon != cw)
	{
		short snum;
		snum = sprite[p->i].yvel;

		SetGameVarID(g_iWeaponVarID, cw, p->i, snum);
		if (p->curr_weapon >= 0)
		{
			SetGameVarID(g_iWorksLikeVarID, aplWeaponWorksLike[weapon][snum], p->i, snum);
		}
		else
		{
			SetGameVarID(g_iWorksLikeVarID, -1, p->i, snum);
		}
		SetGameVarID(g_iReturnVarID, 0, -1, snum);
		OnEvent(EVENT_CHANGEWEAPON, p->i, snum, -1);
		if (GetGameVarID(g_iReturnVarID, -1, snum) == 0)
		{
			p->curr_weapon = cw;
		}
	}
#else
	p->curr_weapon = cw;
#endif

	switch (weapon)
	{
	case RA15_WEAPON:
	case KNEE_WEAPON:
	case TRIPBOMB_WEAPON:
	case HANDREMOTE_WEAPON:
	case HANDBOMB_WEAPON:     
		break;
	case SHOTGUN_WEAPON:      
		spritesound(SHOTGUN_COCK, p->i); 
		break;
	case PISTOL_WEAPON:       
		spritesound(INSERT_CLIP, p->i); 
		break;
	default:      
		spritesound(SELECT_WEAPON, p->i); 
		break;
	}
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void checkavailinven(struct player_struct* p)
{

	if (p->firstaid_amount > 0)
		p->inven_icon = ICON_FIRSTAID;
	else if (p->steroids_amount > 0)
		p->inven_icon = ICON_STEROIDS;
	else if (p->holoduke_amount > 0)
		p->inven_icon = ICON_HOLODUKE;
	else if (p->jetpack_amount > 0)
		p->inven_icon = ICON_JETPACK;
	else if (p->heat_amount > 0)
		p->inven_icon = ICON_HEATS;
	else if (p->scuba_amount > 0)
		p->inven_icon = ICON_SCUBA;
	else if (p->boot_amount > 0)
		p->inven_icon = ICON_BOOTS;
	else p->inven_icon = ICON_NONE;
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void checkavailweapon(struct player_struct* p)
{
	short i, snum;
	int weap;

	if (p->wantweaponfire >= 0)
	{
		weap = p->wantweaponfire;
		p->wantweaponfire = -1;

		if (weap == p->curr_weapon) return;
		else if (p->gotweapon[weap] && p->ammo_amount[weap] > 0)
		{
			addweapon(p, weap);
			return;
		}
	}

	weap = p->curr_weapon;
	if (p->gotweapon[weap] && p->ammo_amount[weap] > 0)
		return;

	snum = sprite[p->i].yvel;

	// Note: RedNukem has this restriction, but the original source and RedneckGDX do not.
#if 1 // TRANSITIONAL
	int max = ((g_gameType & GAMEFLAG_RRALL) ? DEVISTATOR_WEAPON : FREEZE_WEAPON);
#else
	int max = FREEZE_WEAPON;
#endif
	for (i = 0; i < 10; i++)
	{
		weap = ud.wchoice[snum][i];
		if ((g_gameType & GAMEFLAG_SHAREWARE) && weap > 6) continue;

		if (weap == 0) weap = max;
		else weap--;

		if (weap == KNEE_WEAPON || (p->gotweapon[weap] && p->ammo_amount[weap] > 0))
			break;
	}

	if (i == HANDREMOTE_WEAPON) weap = KNEE_WEAPON;

	// Found the weapon

	p->last_weapon = p->curr_weapon;
	p->random_club_frame = 0;
	p->curr_weapon = weap;
	if (g_gameType & GAMEFLAG_WW2GI)
	{
		SetGameVarID(g_iWeaponVarID, p->curr_weapon, p->i, snum);
		if (p->curr_weapon >= 0)
		{
			SetGameVarID(g_iWorksLikeVarID, aplWeaponWorksLike[p->curr_weapon][snum], p->i, snum);
		}
		else
		{
			SetGameVarID(g_iWorksLikeVarID, -1, p->i, snum);
		}
		OnEvent(EVENT_CHANGEWEAPON, p->i, snum, -1);
	}

	p->kickback_pic = 0;
	if (p->holster_weapon == 1)
	{
		p->holster_weapon = 0;
		p->weapon_pos = 10;
	}
	else p->weapon_pos = -1;
}


//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

bool ifsquished(int i, int p) 
{
	if (g_gameType & GAMEFLAG_RRALL) return false;	// this function is a no-op in RR's source.

	bool squishme = false;
	if (sprite[i].picnum == TILE_APLAYER && ud.clipping)
		return false;

	auto &sc = sector[sprite[i].sectnum];
	int floorceildist = sc.floorz - sc.ceilingz;

	if (sc.lotag != ST_23_SWINGING_DOOR)
	{
		if (sprite[i].pal == 1)
			squishme = floorceildist < (32 << 8) && (sc.lotag & 32768) == 0;
		else
			squishme = floorceildist < (12 << 8);
	}

	if (squishme) 
	{
		FTA(QUOTE_SQUISHED, ps[p]);

		if (badguy(&sprite[i]))
			sprite[i].xvel = 0;

		if (sprite[i].pal == 1) 
		{
			hittype[i].picnum = SHOTSPARK1;
			hittype[i].extra = 1;
			return false;
		}

		return true;
	}
	return false;
}


END_DUKE_NS
