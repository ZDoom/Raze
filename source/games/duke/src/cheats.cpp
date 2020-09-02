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
EXTERN_CVAR(Bool, sv_cheats)

BEGIN_DUKE_NS

const char *GameInterface::CheckCheatMode()
{
	if (sv_cheats && (ud.player_skill == 4 || (isRR() && ud.player_skill > 3) || (isRRRA() && ps[myconnectindex].nocheat)))
	{
		return quoteMgr.GetQuote(QUOTE_CHEATS_DISABLED);
	}
	return nullptr;
}


const char *cheatGod(int myconnectindex, int state)
{
	if (state == -1) state = 1 - ud.god;
	ud.god = state;

	sprite[ps[myconnectindex].i].extra = max_player_health;
	hittype[ps[myconnectindex].i].extra = 0;
	if (ud.god)
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

		return quoteMgr.GetQuote(QUOTE_CHEAT_GODMODE_ON);
	}
	else
	{
		ud.god = 0;
		sprite[ps[myconnectindex].i].extra = max_player_health;
		hittype[ps[myconnectindex].i].extra = -1;
		ps[myconnectindex].last_extra = max_player_health;
		return quoteMgr.GetQuote(QUOTE_CHEAT_GODMODE_OFF);
	}
}


const char* GameInterface::GenericCheat(int player, int cheat)
{
	switch (cheat)
	{
	case CHT_GOD:
		return cheatGod(player, -1);

	case CHT_GODOFF:
		return cheatGod(player, 0);

	case CHT_GODON:
		return cheatGod(player, 1);

	default:
		return nullptr;
	}
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
			cameraclock = INT_MIN;
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
	{ "dncornholio",  "god" },
	{ "dnstuff",      nullptr,          cheatStuff },
	{ "dnscotty###",  nullptr,          cheatLevel },
	{ "dncoords",     nullptr,          cheatCoord, 1 },
	{ "dnview",       nullptr,          cheatView, 1 },
	{ "dntime",       nullptr,          cheatTime, 1 },
	{ "dnunlock",     nullptr,          cheatUnlock },
	{ "dncashman",    nullptr,          cheatCashman },
	{ "dnitems",      nullptr,          cheatItems },
	{ "dnrate",       nullptr,          cheatRate, 1 },
	{ "dnskill#",     nullptr,          cheatSkill },
	{ "dnbeta",       nullptr,          cheatBeta },
	{ "dnhyper",      nullptr,          cheatHyper },
	{ "dnmonsters",   nullptr,          cheatMonsters },
	{ "dntodd",       nullptr,          cheatTodd },
	{ "dnshowmap",    nullptr,          cheatMap },
	{ "dnkroz",       "god" },
	{ "dnallen",      nullptr,          cheatAllen },
	{ "dnclip",       nullptr,          cheatClip },
	{ "dnweapons",    nullptr,          cheatWeapons },
	{ "dninventory",  nullptr,          cheatInventory },
	{ "dnkeys",       nullptr,          cheatKeys },
	{ "dndebug",      nullptr,          cheatDebug, 1 },
	{ "dncgs",        nullptr,          cheatKill },
};

static cheatseq_t ww2cheats[] = 
{
	// Use the same code prefix as EDuke because 'ww' is not usable here. Since the cheat parser eats input after the second key, this could easily cause interference for WASD users.
	{ "gi2god",       "god" },
	{ "gi2blood",     nullptr,          cheatStuff },
	{ "gi2level###",  nullptr,          cheatLevel },
	{ "gi2coords",    nullptr,          cheatCoord, 1 },
	{ "gi2view",      nullptr,          cheatView, 1 },
	{ "gi2time",      nullptr,          cheatTime, 1 },
	{ "gi2rate",      nullptr,          cheatRate, 1 },
	{ "gi2skill",     nullptr,          cheatSkill },
	{ "gi2enemies",   nullptr,          cheatMonsters },
	{ "gi2matt",      nullptr,          cheatTodd },
	{ "gi2showmap",   nullptr,          cheatMap },
	{ "gi2ryan",      "god" },
	{ "gi2clip",      nullptr,          cheatClip },
	{ "gi2weapons",   nullptr,          cheatWeapons },
	{ "gi2inventory", nullptr,          cheatInventory },
	{ "gi2debug",     nullptr,          cheatDebug, 1 },
	{ "gi2cgs",       nullptr,          cheatKill },
};

static cheatseq_t namcheats[] = {
	{ "nvacaleb",    "god" },
	{ "nvablood",    nullptr,           cheatStuff },
	{ "nvalevel###", nullptr,           cheatLevel },
	{ "nvacoords",   nullptr,           cheatCoord, 1 },
	{ "nvaview",     nullptr,           cheatView, 1 },
	{ "nvatime",     nullptr,           cheatTime, 1 },
	{ "nvarate",     nullptr,           cheatRate, 1 },
	{ "nvaskill",    nullptr,           cheatSkill },
	{ "nvahyper",    nullptr,           cheatHyper },
	{ "nvaenemies",  nullptr,           cheatMonsters },
	{ "nvamatt",     nullptr,           cheatTodd },
	{ "nvashowmap",  nullptr,           cheatMap },
	{ "nvagod",      "god" },
	{ "nvaclip",     nullptr,           cheatClip },
	{ "nvaweapons",  nullptr,           cheatWeapons }, 
	{ "nvainventory",nullptr,           cheatInventory },
	{ "nvadebug",    nullptr,           cheatDebug, 1 },
	{ "nvacgs",      nullptr,           cheatKill },
};

static cheatseq_t rrcheats[] = {
	{ "rdhounddog",  "god" },
	{ "rdall",       nullptr,           cheatStuff },
	{ "rdmeadow###", nullptr,           cheatLevel },
	{ "rdyerat",     nullptr,           cheatCoord, 1 },
	{ "rdview",      nullptr,           cheatView, 1 },
	{ "rdtime",      nullptr,           cheatTime, 1 },
	{ "rdunlock",    nullptr,           cheatUnlock },
	{ "rdcluck",     nullptr,           cheatCashman },
	{ "rditems",     nullptr,           cheatItems },
	{ "rdrate",      nullptr,           cheatRate, 1 },
	{ "rdskill#",    nullptr,           cheatSkill },
	{ "rdteachers",  nullptr,           cheatBeta },
	{ "rdmoonshine", nullptr,           cheatHyper },
	{ "rdcritters",  nullptr,           cheatMonsters },
	{ "rdrafael",    nullptr,           cheatTodd },
	{ "rdshowmap",   nullptr,           cheatMap },
	{ "rdelvis",     "god" },
	{ "rdclip",      nullptr,           cheatClip },
	{ "rdguns",      nullptr,           cheatWeapons }, 
	{ "rdinventory", nullptr,           cheatInventory },
	{ "rdkeys",      nullptr,           cheatKeys },
	{ "rddebug",     nullptr,           cheatDebug, 1 },
	{ "rdcgs",       nullptr,           cheatKill }, // 23 for RR
	// RRRA only!
	{ "rdjoseph",    nullptr,           cheatMotorcycle },
	{ "rdmrbill",    nullptr,           cheatKill },
	{ "rdtony",      nullptr,           cheatTony },
	{ "rdgary",      nullptr,           cheatGary },
	{ "rdrhett",     nullptr,           cheatRhett },
	{ "rdaaron",     nullptr,           cheatAaron },
	{ "rdnocheat",   nullptr,           cheatNocheat },
	{ "rdwoleslagle",nullptr,           cheatDrink },
	{ "rdmikael",    nullptr,           cheatStuff },
	{ "rdgreg",      nullptr,           cheatSeasick },
	//"rdnoah",      nullptr,           // no-op
	{ "rdarijit",    nullptr,           cheatBoat },
	{ "rddonut",     nullptr,           cheatBoat },
	{ "rdkfc",       nullptr,           cheatKfc },
	{ "rdvan",       nullptr,           cheatVan },
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
