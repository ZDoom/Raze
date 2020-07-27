//-------------------------------------------------------------------------
/*
Copyright (C) 1996, 2003 - 3D Realms Entertainment
Copyright (C) 2000, 2003 - Matt Saettler (EDuke Enhancements)
Copyright (C) 2017-2019 Nuke.YKT
Copyright (C) 2020 Christoph Oelckers

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
#include "duke3d.h"
#include "c_cvars.h"
#include "mapinfo.h"
#include "cheathandler.h"
#include "c_dispatch.h"

EXTERN_CVAR(Int, developer)

BEGIN_DUKE_NS

bool GameInterface::CheatAllowed(bool printmsg)
{
	if (ud.player_skill == 4 || (isRR() && ud.player_skill > 3) || (isRRRA() && ps[myconnectindex].nocheat))
	{
		if (printmsg) FTA(22, &ps[myconnectindex]);
		return false;
	}
	return true;
}

bool cheatWeapons(cheatseq_t *s)
{
	int weaponLimit = (VOLUMEONE) ? SHRINKER_WEAPON : MAX_WEAPONS;

	for (int weapon = PISTOL_WEAPON; weapon < weaponLimit; weapon++ )
	{
		addammo( weapon, &ps[myconnectindex], max_ammo_amount[weapon] );
		ps[myconnectindex].gotweapon.Set(weapon);
	}
	if (isRRRA())
		ps[myconnectindex].ammo_amount[SLINGBLADE_WEAPON] = 1;

	if (s) FTA(119,&ps[myconnectindex]);
	return true;
}

bool cheatInventory(cheatseq_t *s)
{
	auto invGet = [](int defvalue, int evtype, int16_t &dest)
	{
		SetGameVarID(g_iReturnVarID, defvalue, -1, myconnectindex);
		OnEvent(evtype, -1, myconnectindex, -1);
		if (GetGameVarID(g_iReturnVarID, -1, myconnectindex) >= 0)
		{
			dest = GetGameVarID(g_iReturnVarID, -1, myconnectindex);
		}
	};

	invGet(400, EVENT_CHEATGETSTEROIDS, ps[myconnectindex].steroids_amount);
	if (!isRR()) invGet(1200, EVENT_CHEATGETHEAT, ps[myconnectindex].heat_amount);
	invGet(isRR() ? 2000 : 200, EVENT_CHEATGETBOOT, ps[myconnectindex].boot_amount);
	invGet(100, EVENT_CHEATGETSHIELD, ps[myconnectindex].shield_amount);
	invGet(6400, EVENT_CHEATGETSCUBA, ps[myconnectindex].scuba_amount);
	invGet(2400, EVENT_CHEATGETHOLODUKE, ps[myconnectindex].holoduke_amount);
	invGet(isRR() ? 600 : 1600, EVENT_CHEATGETJETPACK, ps[myconnectindex].jetpack_amount);
	invGet(max_player_health, EVENT_CHEATGETFIRSTAID, ps[myconnectindex].firstaid_amount);
	if (s) FTA(120,&ps[myconnectindex]);
	return true;
}

bool cheatKeys(cheatseq_t *s)
{
	ps[myconnectindex].got_access = 7;
	if (isRR()) for (int ikey = 0; ikey < 5; ikey++)
		ps[myconnectindex].keys[ikey] = 1;
	if (s) FTA(121,&ps[myconnectindex]);
	return true;
}

static bool cheatDebug(cheatseq_t *)
{
	// Let's do something useful with this.
	if (developer == 0) developer = 3;
	else developer = 0;
	return true;
}

bool cheatClip(cheatseq_t *)
{
	ud.clipping = 1-ud.clipping;
	FTA(112+ud.clipping,&ps[myconnectindex]);
	return true;
}

static bool cheatAllen(cheatseq_t *)
{
	FTA(79,&ps[myconnectindex]);
	return true;
}

bool cheatGod(cheatseq_t *)
{
	ud.god = 1-ud.god;

	if(ud.god)
	{
		if (isRRRA()) S_PlaySound(218, CHAN_AUTO, CHANF_UI);
		sprite[ps[myconnectindex].i].cstat = 257;

		hittype[ps[myconnectindex].i].temp_data[0] = 0;
		hittype[ps[myconnectindex].i].temp_data[1] = 0;
		hittype[ps[myconnectindex].i].temp_data[2] = 0;
		hittype[ps[myconnectindex].i].temp_data[3] = 0;
		hittype[ps[myconnectindex].i].temp_data[4] = 0;
		hittype[ps[myconnectindex].i].temp_data[5] = 0;

		sprite[ps[myconnectindex].i].hitag = 0;
		sprite[ps[myconnectindex].i].lotag = 0;
		sprite[ps[myconnectindex].i].pal =
			ps[myconnectindex].palookup;

		FTA(17,&ps[myconnectindex]);
	}
	else
	{
		ud.god = 0;
		sprite[ps[myconnectindex].i].extra = max_player_health;
		hittype[ps[myconnectindex].i].extra = -1;
		ps[myconnectindex].last_extra = max_player_health;
		FTA(18,&ps[myconnectindex]);
	}

	sprite[ps[myconnectindex].i].extra = max_player_health;
	hittype[ps[myconnectindex].i].extra = 0;
	return true;
}	

bool cheatStuff(cheatseq_t *)
{
	cheatWeapons(nullptr);
	cheatInventory(nullptr);
	if (!isNamWW2GI()) cheatKeys(nullptr);
	FTA(5,&ps[myconnectindex]);
	return true;

}

static bool cheatLevel(cheatseq_t *s)
{
	// Fixme: This should be broadcast as a net event once things are up again.
	lastlevel = 0;
	int volnume,levnume;
	volnume = s->Args[0] - '0' - 1;
	levnume = (s->Args[1] - '0')*10+(s->Args[2]-'0') - 1;
	
	// Instead of hard coded range checks on volume and level, let's just check if the level is defined.
	auto map = FindMapByLevelNum(levelnum(volnume, levnume));
	if (map)
	{
		ud.nextLevel = map;
		FX_StopAllSounds();
		FX_SetReverb(0);
		ps[myconnectindex].gm |= MODE_RESTART;
	}
	return true;
}

static bool cheatCoord(cheatseq_t *)
{
	C_DoCommand("stat coord");
	return true;
}

static bool cheatTime(cheatseq_t *)
{
	C_DoCommand("stat fps");
	return true;
}

static bool cheatRate(cheatseq_t *)
{
	C_DoCommand("toggle vid_fps");
	return true;
}

static bool cheatItems(cheatseq_t *)
{
	cheatInventory(nullptr);
	if (!isNamWW2GI()) cheatKeys(nullptr);
	FTA(5,&ps[myconnectindex]);
	return true;
}


static bool cheatView(cheatseq_t *)
{
	if (ps[myconnectindex].OnMotorcycle == 0 && ps[myconnectindex].OnBoat == 0)
	{
		if( ps[myconnectindex].over_shoulder_on )
			ps[myconnectindex].over_shoulder_on = 0;
		else
		{
			ps[myconnectindex].over_shoulder_on = 1;
			cameradist = 0;
			cameraclock = (int)totalclock;
		}
		//FTA(22,&ps[myconnectindex]); this message makes no sense.
	}
	return true;
}

static bool cheatUnlock(cheatseq_t *)
{
	if (VOLUMEONE) return false;
	for(int i=numsectors-1;i>=0;i--) //Unlock
	{
		int j = sector[i].lotag;
		if(j == -1 || j == 32767) continue;
		if( (j & 0x7fff) > 2 )
		{
			if( j&(0xffff-16384) )
				sector[i].lotag &= (0xffff-16384);
			operatesectors(i,ps[myconnectindex].i);
		}
	}
	fi.operateforcefields(ps[myconnectindex].i,-1);
	FTA(100,&ps[myconnectindex]);
	return true;
}

static bool cheatCashman(cheatseq_t *)
{
	ud.cashman = 1-ud.cashman;
	return true;
}

static bool cheatSkill(cheatseq_t *s)
{
	lastlevel = 0;
	ud.m_player_skill = ud.player_skill = s->Args[0] - '1';
	ps[myconnectindex].gm |= MODE_RESTART;
	FX_StopAllSounds();
	FX_SetReverb(0);
	return true;
}

static bool cheatBeta(cheatseq_t *)
{
	FTA(105,&ps[myconnectindex]);
	return true;
}

static bool cheatTodd(cheatseq_t *)
{
	FTA(99,&ps[myconnectindex]);
	return true;
}

static bool cheatHyper(cheatseq_t *)
{
	ps[myconnectindex].steroids_amount = 399;
	FTA(37,&ps[myconnectindex]);
	return true;
}

static bool cheatMonsters(cheatseq_t *)
{
	if(++actor_tog == 3) actor_tog = 0;
	static const char *s [] = { "OPTVAL_ON", "OPTVAL_OFF", "TXT_ON2" };
	Printf(PRINT_NOTIFY, "%s: %s", GStrings("NETMNU_MONSTERS"), GStrings(s[actor_tog]));
	return true;
}

static bool cheatMap(cheatseq_t *)
{
	gFullMap = !gFullMap;
	FTA(gFullMap? 111 : 1, &ps[myconnectindex]);
	return true;
}

static bool cheatKill(cheatseq_t *)
{
	quickkill(&ps[myconnectindex]);
	FTA(127,&ps[myconnectindex]);
	return true;
}

// RRRA only cheats

static bool cheatMotorcycle(cheatseq_t *)
{
	OnMotorcycle(&ps[myconnectindex],0);
	ps[myconnectindex].ammo_amount[MOTORCYCLE_WEAPON] = max_ammo_amount[MOTORCYCLE_WEAPON];
	FTA(126,&ps[myconnectindex]);
	return true;
}

static bool cheatBoat(cheatseq_t *)
{
	OnBoat(&ps[myconnectindex],0);
	ps[myconnectindex].ammo_amount[BOAT_WEAPON] = max_ammo_amount[BOAT_WEAPON];
	FTA(136,&ps[myconnectindex]);
	return true;
}

static bool cheatTony(cheatseq_t *)
{
	enemysizecheat = 2;
	return true;
}

static bool cheatVan(cheatseq_t *)
{
	enemysizecheat = 3;
	return true;
}

static bool cheatGary(cheatseq_t *)
{
	S_PlayRRMusic(10);
	return true;
}

static bool cheatRhett(cheatseq_t *)
{
	ud.god = 0;
	ps[myconnectindex].gotweapon.Zero();
	ps[myconnectindex].curr_weapon = KNEE_WEAPON;
	ps[myconnectindex].nocheat = 1;
	sprite[ps[myconnectindex].i].extra = 1;
	FTA(128,&ps[myconnectindex]);
	return true;
}

static bool cheatAaron(cheatseq_t *)
{
	if (ps[myconnectindex].DrugMode)
		ps[myconnectindex].DrugMode = 0;
	else
		ps[myconnectindex].DrugMode = 5;
	return true;
}

static bool cheatNocheat(cheatseq_t *)
{
	ps[myconnectindex].nocheat = 1;
	FTA(130,&ps[myconnectindex]);
	return true;
}

static bool cheatDrink(cheatseq_t *)
{
	if (ps[myconnectindex].drink_amt)
	{
		ps[myconnectindex].drink_amt = 0;
		FTA(132,&ps[myconnectindex]);
	}
	else
	{
		ps[myconnectindex].drink_amt = 90;
		FTA(131,&ps[myconnectindex]);
	}
	return true;
}

static bool cheatSeasick(cheatseq_t *)
{
	if (ps[myconnectindex].sea_sick_stat)
	{
		ps[myconnectindex].sea_sick_stat = 0;
		FTA(129,&ps[myconnectindex]);
	}
	else
	{
		ps[myconnectindex].sea_sick_stat = 1;
		FTA(137, &ps[myconnectindex]);
	}
	return true;
}

static bool cheatKfc(cheatseq_t *)
{
	for (int i = 0; i < 7; i++)
	{
		int spr = fi.spawn(ps[screenpeek].i,TILE_HEN);
		sprite[spr].pal = 1;
		sprite[spr].xrepeat = sprite[spr].xrepeat<<2;
		sprite[spr].yrepeat = sprite[spr].yrepeat<<2;
	}
	FTA(139,&ps[myconnectindex]);
	return true;
}

static cheatseq_t dukecheats[] = {
	{ "dncornholio", cheatGod },
	{ "dnstuff", cheatStuff },
	{ "dnscotty###", cheatLevel },
	{ "dncoords", cheatCoord, 1 },
	{ "dnview", cheatView, 1 },
	{ "dntime", cheatTime, 1 },
	{ "dnunlock", cheatUnlock },
	{ "dncashman", cheatCashman },
	{ "dnitems", cheatItems },
	{ "dnrate", cheatRate, 1 },
	{ "dnskill#", cheatSkill },
	{ "dnbeta", cheatBeta },
	{ "dnhyper", cheatHyper },
	{ "dnmonsters", cheatMonsters },
	{ "dntodd", cheatTodd },
	{ "dnshowmap", cheatMap },
	{ "dnkroz", cheatGod },
	{ "dnallen", cheatAllen },
	{ "dnclip", cheatClip },
	{ "dnweapons", cheatWeapons },
	{ "dninventory", cheatInventory },
	{ "dnkeys", cheatKeys },
	{ "dndebug", cheatDebug, 1 },
	{ "dncgs", cheatKill },
};

static cheatseq_t ww2cheats[] = 
{
	// Use the same code prefix as EDuke because 'ww' is not usable here. Since the cheat parser eats input after the second key, this could easily cause interference for WASD users.
	{ "gi2god", cheatGod },
	{ "gi2blood", cheatStuff },
	{ "gi2level###", cheatLevel },
	{ "gi2coords", cheatCoord, 1 },
	{ "gi2view", cheatView, 1 },
	{ "gi2time", cheatTime, 1 },
	{ "gi2rate", cheatRate, 1 },
	{ "gi2skill", cheatSkill },
	{ "gi2enemies", cheatMonsters },
	{ "gi2matt", cheatTodd },
	{ "gi2showmap", cheatMap },
	{ "gi2ryan", cheatGod },
	{ "gi2clip", cheatClip },
	{ "gi2weapons", cheatWeapons },
	{ "gi2inventory", cheatInventory },
	{ "gi2debug", cheatDebug, 1 },
	{ "gi2cgs", cheatKill },
};

static cheatseq_t namcheats[] = {
	{ "nvacaleb", cheatGod },
	{ "nvablood", cheatStuff },
	{ "nvalevel###", cheatLevel },
	{ "nvacoords", cheatCoord, 1 },
	{ "nvaview", cheatView, 1 },
	{ "nvatime", cheatTime, 1 },
	{ "nvarate", cheatRate, 1 },
	{ "nvaskill", cheatSkill },
	{ "nvahyper", cheatHyper },
	{ "nvaenemies", cheatMonsters },
	{ "nvamatt", cheatTodd },
	{ "nvashowmap", cheatMap },
	{ "nvagod", cheatGod },
	{ "nvaclip", cheatClip },
	{ "nvaweapons", cheatWeapons }, 
	{ "nvainventory", cheatInventory },
	{ "nvadebug", cheatDebug, 1 },
	{ "nvacgs", cheatKill },
};

static cheatseq_t rrcheats[] = {
	{ "rdhounddog", cheatGod },
	{ "rdall", cheatStuff },
	{ "rdmeadow###", cheatLevel },
	{ "rdyerat", cheatCoord, 1 },
	{ "rdview", cheatView, 1 },
	{ "rdtime", cheatTime, 1 },
	{ "rdunlock", cheatUnlock },
	{ "rdcluck", cheatCashman },
	{ "rditems", cheatItems },
	{ "rdrate", cheatRate, 1 },
	{ "rdskill#", cheatSkill },
	{ "rdteachers", cheatBeta },
	{ "rdmoonshine", cheatHyper },
	{ "rdcritters", cheatMonsters },
	{ "rdrafael", cheatTodd },
	{ "rdshowmap", cheatMap },
	{ "rdelvis", cheatGod },
	{ "rdclip", cheatClip },
	{ "rdguns", cheatWeapons }, 
	{ "rdinventory", cheatInventory },
	{ "rdkeys", cheatKeys },
	{ "rddebug", cheatDebug, 1 },
	{ "rdcgs", cheatKill }, // 23 for RR
	// RRRA only!
	{ "rdjoseph", cheatMotorcycle },
	{ "rdmrbill", cheatKill },
	{ "rdtony", cheatTony },
	{ "rdgary", cheatGary },
	{ "rdrhett", cheatRhett },
	{ "rdaaron", cheatAaron },
	{ "rdnocheat", cheatNocheat },
	{ "rdwoleslagle", cheatDrink },
	{ "rdmikael", cheatStuff },
	{ "rdgreg", cheatSeasick },
	//"rdnoah",       // no-op
	{ "rdarijit", cheatBoat },
	{ "rddonut", cheatBoat },
	{ "rdkfc", cheatKfc },
	{ "rdvan", cheatVan },
};

void InitCheats()
{
	if (isRRRA()) SetCheats(rrcheats, countof(rrcheats));
	else if (isRR()) SetCheats(rrcheats, 23);
	else if (isWW2GI()) SetCheats(ww2cheats, countof(ww2cheats));
	else if (isNam()) SetCheats(namcheats, countof(namcheats));
	else SetCheats(dukecheats, countof(dukecheats));
}

END_DUKE_NS
