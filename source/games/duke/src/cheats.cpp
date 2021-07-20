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
#include "gamestate.h"
#include "automap.h"
#include "dukeactor.h"

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


static const char *cheatGod(int myconnectindex, int state)
{
	if (state == -1) state = !ud.god;
	ud.god = state;

	auto* p = &ps[myconnectindex];
	auto act = p->GetActor();

	p->resurrected = true;
	act->s->extra = gs.max_player_health;
	act->extra = 0;
	if (ud.god)
	{
		if (isRRRA()) S_PlaySound(218, CHAN_AUTO, CHANF_UI);
		act->s->cstat = 257;

		act->temp_data[0] = 0;
		act->temp_data[1] = 0;
		act->temp_data[2] = 0;
		act->temp_data[3] = 0;
		act->temp_data[4] = 0;
		act->temp_data[5] = 0;

		act->s->hitag = 0;
		act->s->lotag = 0;
		act->s->pal =
			ps[myconnectindex].palookup;

		return quoteMgr.GetQuote(QUOTE_CHEAT_GODMODE_ON);
	}
	else
	{
		ud.god = 0;
		act->s->extra = gs.max_player_health;
		act->extra = -1;
		ps[myconnectindex].last_extra = gs.max_player_health;
		return quoteMgr.GetQuote(QUOTE_CHEAT_GODMODE_OFF);
	}
}

static const char* cheatUnlock()
{
	if (isShareware()) return nullptr;
	for (int i = numsectors - 1; i >= 0; i--) //Unlock
	{
		int j = sector[i].lotag;
		if (j == -1 || j == 32767) continue;
		if ((j & 0x7fff) > 2)
		{
			if (j & (0xffff - 16384))
				sector[i].lotag &= (0xffff - 16384);
			operatesectors(i, ps[myconnectindex].GetActor());
		}
	}
	fi.operateforcefields(ps[myconnectindex].GetActor(), -1);
	return quoteMgr.GetQuote(QUOTE_CHEAT_UNLOCK);
}

static const char *cheatKfc(int player)
{
	for (int i = 0; i < 7; i++)
	{
		auto spr = spawn(ps[player].GetActor(), TILE_HEN);
		spr->s->pal = 1;
		spr->s->xrepeat = spr->s->xrepeat << 2;
		spr->s->yrepeat = spr->s->yrepeat << 2;
	}
	return quoteMgr.GetQuote(QUOTE_CHEAT_KFC);
}

static const char * cheatMonsters()
{
	static char textbuf[64];
	if (++actor_tog == 3) actor_tog = 0;
	static const char* s[] = { "OPTVAL_ON", "OPTVAL_OFF", "TXT_ON2" };
	mysnprintf(textbuf, 64, "%s: %s", GStrings("NETMNU_MONSTERS"), GStrings(s[actor_tog]));
	return textbuf;
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

	case CHT_NOCLIP:
		ud.clipping = 1 - ud.clipping;
		return quoteMgr.GetQuote(ud.clipping ? QUOTE_CHEAT_NOCLIP : QUOTE_CHEAT_CLIP);

	case CHT_UNLOCK:
		return cheatUnlock();

	case CHT_CASHMAN:
		ud.cashman = 1 - ud.cashman;
		return nullptr;

	case CHT_HYPER:
		ps[player].steroids_amount = 399;
		return quoteMgr.GetQuote(QUOTE_CHEAT_STEROIDS);

	case CHT_KILL:
		quickkill(&ps[player]);
		return quoteMgr.GetQuote(QUOTE_CHEAT_KILL);

	case CHT_MONSTERS:
		return cheatMonsters();

	case CHT_BIKE:
		OnMotorcycle(&ps[player], nullptr);
		ps[player].ammo_amount[MOTORCYCLE_WEAPON] = gs.max_ammo_amount[MOTORCYCLE_WEAPON];
		return quoteMgr.GetQuote(QUOTE_ON_BIKE);

	case CHT_BOAT:
		OnBoat(&ps[player], 0);
		ps[player].ammo_amount[BOAT_WEAPON] = gs.max_ammo_amount[BOAT_WEAPON];
		return quoteMgr.GetQuote(QUOTE_ON_BOAT);

	case CHT_TONY:
		enemysizecheat = 2;
		return nullptr;

	case CHT_VAN:
		enemysizecheat = 3;
		return nullptr;

	case CHT_RHETT:
		ud.god = 0;
		memset(ps[player].gotweapon, 0, MAX_WEAPONS);
		ps[player].curr_weapon = KNEE_WEAPON;
		ps[player].nocheat = 1;
		ps[player].GetActor()->s->extra = 1;
		return quoteMgr.GetQuote(QUOTE_YERFUCKED);

	case CHT_AARON:
		ps[player].DrugMode = !ps[player].DrugMode;
		return nullptr;

	case CHT_NOCHEAT:
		ps[player].nocheat = 1;
		return quoteMgr.GetQuote(QUOTE_NOCHEATS);

	case CHT_DRINK:
		ps[player].drink_amt = ps[player].drink_amt ? 0 : 90;
		return quoteMgr.GetQuote(ps[player].drink_amt ? QUOTE_INSTADRUNK : QUOTE_INSTASOBER);

	case CHT_SEASICK:
		ps[player].sea_sick_stat = !ps[player].sea_sick_stat;
		return quoteMgr.GetQuote(ps[player].sea_sick_stat ? QUOTE_BOATMODEON : QUOTE_BOATMODEOFF);

	case CHT_KFC:
		return cheatKfc(player);

	default:
		return nullptr;
	}
}

static bool cheatWeapons(int player)
{
	int weaponLimit = (isShareware()) ? SHRINKER_WEAPON : MAX_WEAPONS;

	for (int weapon = PISTOL_WEAPON; weapon < weaponLimit; weapon++ )
	{
		addammo( weapon, &ps[player], gs.max_ammo_amount[weapon] );
		ps[player].gotweapon[weapon] = true;;
	}
	if (isRRRA())
		ps[player].ammo_amount[SLINGBLADE_WEAPON] = 1;

	return true;
}

static bool cheatInventory(int player)
{
	auto invGet = [=](int defvalue, int evtype, int16_t &dest)
	{
		SetGameVarID(g_iReturnVarID, defvalue, nullptr, player);
		OnEvent(evtype, player, nullptr, -1);
		if (GetGameVarID(g_iReturnVarID, nullptr, player) >= 0)
		{
			dest = GetGameVarID(g_iReturnVarID, nullptr, player);
		}
	};

	invGet(400, EVENT_CHEATGETSTEROIDS, ps[player].steroids_amount);
	if (!isRR()) invGet(1200, EVENT_CHEATGETHEAT, ps[player].heat_amount);
	invGet(isRR() ? 2000 : 200, EVENT_CHEATGETBOOT, ps[player].boot_amount);
	invGet(100, EVENT_CHEATGETSHIELD, ps[player].shield_amount);
	invGet(6400, EVENT_CHEATGETSCUBA, ps[player].scuba_amount);
	invGet(2400, EVENT_CHEATGETHOLODUKE, ps[player].holoduke_amount);
	invGet(isRR() ? 600 : 1600, EVENT_CHEATGETJETPACK, ps[player].jetpack_amount);
	invGet(gs.max_player_health, EVENT_CHEATGETFIRSTAID, ps[player].firstaid_amount);
	return true;
}

static bool cheatKeys(int player)
{
	ps[player].got_access = 7;
	if (isRR()) for (int ikey = 0; ikey < 5; ikey++)
		ps[player].keys[ikey] = 1;
	return true;
}

static bool cheatStuff(int player)
{
	cheatWeapons(player);
	cheatInventory(player);
	if (!isNamWW2GI()) cheatKeys(player);
	return true;

}

static bool cheatItems(int player)
{
	cheatInventory(player);
	if (!isNamWW2GI()) cheatKeys(player);
	return true;
}




static bool cheatLevel(cheatseq_t *s)
{
	int volnume,levnume;
	volnume = s->Args[0] - '0';
	levnume = (s->Args[1] - '0')*10+(s->Args[2]-'0');
	
	// Instead of hard coded range checks on volume and level, let's just check if the level is defined.
	auto map = FindMapByIndex(volnume, levnume);
	if (map)
	{
		ChangeLevel(map, g_nextskill);
	}
	return true;
}

static bool cheatSkill(cheatseq_t *s)
{
	ChangeLevel(currentLevel, s->Args[0] - '1');
	//FX_StopAllSounds();
	//FX_SetReverb(0);
	return true;
}

// The remaining cheats are client side only.

static bool cheatDebug(cheatseq_t*)
{
	// Let's do something useful with this.
	if (developer == 0) developer = 3;
	else developer = 0;
	return true;
}

static bool cheatAllen(cheatseq_t*)
{
	FTA(79, &ps[myconnectindex]);
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

static bool cheatMap(cheatseq_t *)
{
	gFullMap = !gFullMap;
	FTA(gFullMap? 111 : 1, &ps[myconnectindex]);
	return true;
}

// RRRA only cheats

static bool cheatGary(cheatseq_t *)
{
	S_PlayRRMusic(10);
	return true;
}

static cheatseq_t dukecheats[] = {
	{ "dncornholio",  nullptr,			SendGenericCheat, 0, CHT_GOD },
	{ "dnstuff",      "give all", },
	{ "dnscotty###",  nullptr,          cheatLevel }, // -> levelwarp
	{ "dncoords",     "stat coord",		nullptr, 1 },
	{ "dnview",       "third_person_view",nullptr, 1 },
	{ "dntime",       "stat fps",       nullptr, 1 },
	{ "dnunlock",     nullptr,          SendGenericCheat, 0, CHT_UNLOCK },
	{ "dncashman",    nullptr,          SendGenericCheat, 0, CHT_CASHMAN },
	{ "dnitems",      "give items", },
	{ "dnrate",       "toggle vid_fps", nullptr, 1 },
	{ "dnskill#",     nullptr,          cheatSkill }, // restartmap <skill>
	{ "dnbeta",       nullptr,          cheatBeta },
	{ "dnhyper",      nullptr,          SendGenericCheat, 0, CHT_HYPER },
	{ "dnmonsters",   nullptr,          SendGenericCheat, 0, CHT_MONSTERS },
	{ "dntodd",       nullptr,          cheatTodd },
	{ "dnshowmap",    nullptr,          cheatMap },
	{ "dnkroz",       nullptr,			SendGenericCheat, 0, CHT_GOD },
	{ "dnallen",      nullptr,          cheatAllen },
	{ "dnclip",       nullptr,			SendGenericCheat, 0, CHT_NOCLIP },
	{ "dnweapons",    "give weapons" },
	{ "dninventory",  "give inventory" },
	{ "dnkeys",       "give keys" },
	{ "dndebug",      nullptr,          cheatDebug, 1 },
	{ "dncgs",        nullptr,          SendGenericCheat, 0, CHT_KILL },
};

static cheatseq_t ww2cheats[] = 
{
	// Use the same code prefix as EDuke because 'ww' is not usable here. Since the cheat parser eats input after the second key, this could easily cause interference for WASD users.
	{ "gi2god",       nullptr,			SendGenericCheat, 0, CHT_GOD },
	{ "gi2blood",     "give all", },
	{ "gi2level###",  nullptr,          cheatLevel },
	{ "gi2coords",    "stat coord",		nullptr, 1 },
	{ "gi2view",      "third_person_view",nullptr, 1 },
	{ "gi2time",      "stat fps",       nullptr, 1 },
	{ "gi2rate",      "toggle vid_fps", nullptr, 1 },
	{ "gi2skill",     nullptr,          cheatSkill },
	{ "gi2enemies",   nullptr,          SendGenericCheat, 0, CHT_MONSTERS },
	{ "gi2matt",      nullptr,          cheatTodd },
	{ "gi2showmap",   nullptr,          cheatMap },
	{ "gi2ryan",      nullptr,			SendGenericCheat, 0, CHT_GOD },
	{ "gi2clip",      nullptr,			SendGenericCheat, 0, CHT_NOCLIP },
	{ "gi2weapons",   "give weapons" },
	{ "gi2inventory", "give inventory" },
	{ "gi2debug",     nullptr,          cheatDebug, 1 },
	{ "gi2cgs",       nullptr,          SendGenericCheat, 0, CHT_KILL },
};

static cheatseq_t namcheats[] = {
	{ "nvacaleb",    nullptr,			SendGenericCheat, 0, CHT_GOD },
	{ "nvablood",    "give all", },
	{ "nvalevel###", nullptr,           cheatLevel },
	{ "nvacoords",   "stat coord",		nullptr, 1 },
	{ "nvaview",     "third_person_view",nullptr, 1 },
	{ "nvatime",     "stat fps",       nullptr, 1 },
	{ "nvarate",     "toggle vid_fps", nullptr, 1 },
	{ "nvaskill",    nullptr,           cheatSkill },
	{ "nvahyper",    nullptr,          SendGenericCheat, 0, CHT_HYPER },
	{ "nvaenemies",  nullptr,          SendGenericCheat, 0, CHT_MONSTERS },
	{ "nvamatt",     nullptr,           cheatTodd },
	{ "nvashowmap",  nullptr,           cheatMap },
	{ "nvagod",      nullptr,			SendGenericCheat, 0, CHT_GOD },
	{ "nvaclip",     nullptr,			SendGenericCheat, 0, CHT_NOCLIP },
	{ "nvaweapons",  "give weapons" },
	{ "nvainventory","give inventory" },
	{ "nvadebug",    nullptr,           cheatDebug, 1 },
	{ "nvacgs",      nullptr,           SendGenericCheat, 0, CHT_KILL },
};

static cheatseq_t rrcheats[] = {
	{ "rdhounddog",  "god" },
	{ "rdall",       "give all", },
	{ "rdmeadow###", nullptr,           cheatLevel },
	{ "rdyerat",     "stat coord",		nullptr, 1 },
	{ "rdview",      "third_person_view",nullptr, 1 },
	{ "rdtime",      "stat fps",       nullptr, 1 },
	{ "rdunlock",    nullptr,          SendGenericCheat, 0, CHT_UNLOCK },
	{ "rdcluck",     nullptr,          SendGenericCheat, 0, CHT_CASHMAN },
	{ "rditems",     "give items", },
	{ "rdrate",      "toggle vid_fps", nullptr, 1 },
	{ "rdskill#",    nullptr,           cheatSkill },
	{ "rdteachers",  nullptr,           cheatBeta },
	{ "rdmoonshine", nullptr,          SendGenericCheat, 0, CHT_HYPER },
	{ "rdcritters",  nullptr,          SendGenericCheat, 0, CHT_MONSTERS },
	{ "rdrafael",    nullptr,           cheatTodd },
	{ "rdshowmap",   nullptr,           cheatMap },
	{ "rdelvis",     "god" },
	{ "rdclip",      "noclip" },
	{ "rdguns",      "give weapons" },
	{ "rdinventory", "give inventory" },
	{ "rdkeys",      "give keys" },
	{ "rddebug",     nullptr,           cheatDebug, 1 },
	{ "rdcgs",       nullptr,           SendGenericCheat, 0, CHT_KILL },
	// RRRA only!
	{ "rdjoseph",    nullptr,           SendGenericCheat, 0, CHT_BIKE },
	{ "rdmrbill",    nullptr,           SendGenericCheat, 0, CHT_KILL },
	{ "rdtony",      nullptr,           SendGenericCheat, 0, CHT_TONY },
	{ "rdgary",      nullptr,           cheatGary },
	{ "rdrhett",     nullptr,           SendGenericCheat, 0, CHT_RHETT },
	{ "rdaaron",     nullptr,           SendGenericCheat, 0, CHT_AARON },
	{ "rdnocheat",   nullptr,           SendGenericCheat, 0, CHT_NOCHEAT },
	{ "rdwoleslagle",nullptr,           SendGenericCheat, 0, CHT_DRINK },
	{ "rdmikael",    "give all", },
	{ "rdgreg",      nullptr,           SendGenericCheat, 0, CHT_SEASICK },
	//"rdnoah",      nullptr,           // no-op
	{ "rdarijit",    nullptr,           SendGenericCheat, 0, CHT_BOAT },
	{ "rddonut",     nullptr,           SendGenericCheat, 0, CHT_BOAT },
	{ "rdkfc",       nullptr,           SendGenericCheat, 0, CHT_KFC },
	{ "rdvan",       nullptr,           SendGenericCheat, 0, CHT_VAN },
};


static void cmd_Give(int player, uint8_t** stream, bool skip)
{
	int type = ReadByte(stream);
	if (skip) return;

	if (numplayers != 1 || gamestate != GS_LEVEL || ps[player].GetActor()->s->extra <= 0)
	{
		Printf("give: Cannot give while dead or not in a single-player game.\n");
		return;
	}

	switch (type)
	{
	case GIVE_ALL:
		cheatStuff(player);
		FTA(5, &ps[player]);
		break;

	case GIVE_HEALTH:
		ps[player].GetActor()->s->extra = gs.max_player_health << 1;
		break;

	case GIVE_WEAPONS:
		cheatWeapons(player);
		FTA(119, &ps[player]);
		break;

	case GIVE_AMMO:
	{
		int maxw = isShareware() ? SHRINKER_WEAPON : MAX_WEAPONS;
		for (int i = maxw; i >= PISTOL_WEAPON; i--)
			addammo(i, &ps[player], gs.max_ammo_amount[i]);
		break;
	}

	case GIVE_ARMOR:
		ps[player].shield_amount = 100;
		break;

	case GIVE_KEYS:
		cheatKeys(player);
		FTA(121, &ps[player]);
		break;

	case GIVE_INVENTORY:
		cheatInventory(player);
		FTA(120, &ps[player]);
		break;

	case GIVE_ITEMS:
		cheatItems(player);
		FTA(5, &ps[player]);
		break;
	}
}


void InitCheats()
{
	if (isRRRA()) SetCheats(rrcheats, countof(rrcheats));
	else if (isRR()) SetCheats(rrcheats, 23);
	else if (isWW2GI()) SetCheats(ww2cheats, countof(ww2cheats));
	else if (isNam()) SetCheats(namcheats, countof(namcheats));
	else SetCheats(dukecheats, countof(dukecheats));
	Net_SetCommandHandler(DEM_GIVE, cmd_Give);
}

END_DUKE_NS
