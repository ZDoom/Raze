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
Modifications for JonoF's port by Jonathon Fowler (jf@jonof.id.au)
*/
//------------------------------------------------------------------------- 

// all code related to startup, up to entering the main loop.

#include "ns.h"	// Must come before everything else!

#include "duke3d.h"
#include "m_argv.h"
#include "mapinfo.h"
#include "texturemanager.h"
#include "statusbar.h"
#include "st_start.h"
#include "i_interface.h"
#include "prediction.h"
#include "gamestate.h"
#include "razefont.h"
#include "psky.h"
#include "vm.h"
#include "thingdef.h"
#include "tilesetbuilder.h"
#include "concmd.h"

BEGIN_DUKE_NS

void SetDispatcher();
void InitCheats();
int registerosdcommands(void);

FTextureID mirrortex, foftex;

//---------------------------------------------------------------------------
//
// DObject stuff - everything GC related.
//
//---------------------------------------------------------------------------

IMPLEMENT_CLASS(DDukeActor, false, true)
IMPLEMENT_POINTERS_START(DDukeActor)
IMPLEMENT_POINTER(ownerActor)
IMPLEMENT_POINTER(hitOwnerActor)
IMPLEMENT_POINTER(temp_actor)
IMPLEMENT_POINTER(seek_actor)
IMPLEMENT_POINTERS_END

size_t DDukeActor::PropagateMark()
{
	for (auto& var : uservars)
	{
		var.Mark();
	}
	return Super::PropagateMark();
}

static void markgcroots()
{
	GC::Mark(camsprite);
	GC::MarkArray(spriteq, 1024);
	GC::Mark(currentCommentarySprite);
	GC::Mark(ud.cameraactor);
	for (auto& pl : ps)
	{
		GC::Mark(pl.actor);
		GC::Mark(pl.actorsqu);
		GC::Mark(pl.wackedbyactor);
		GC::Mark(pl.on_crane);
		GC::Mark(pl.holoduke_on);
		GC::Mark(pl.somethingonplayer);
		GC::Mark(pl.access_spritenum);
		GC::Mark(pl.dummyplayersprite);
		GC::Mark(pl.newOwner);
		for (auto& var : pl.uservars)
		{
			var.Mark();
		}
	}
}

//---------------------------------------------------------------------------
//
// game specific command line args go here. 
//
//---------------------------------------------------------------------------

static void checkcommandline()
{
#if 0
	val = Args->CheckValue("-respawn");
	if (!val) val = Args->CheckValue("-t");
	if (val)
	{
		if (*val == '1') ud.m_respawn_monsters = 1;
		else if (*val == '2') ud.m_respawn_items = 1;
		else if (*val == '3') ud.m_respawn_inventory = 1;
		else
		{
			ud.m_respawn_monsters = 1;
			ud.m_respawn_items = 1;
			ud.m_respawn_inventory = 1;
		}
		Printf("Respawn on.\n");
	}
#endif
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

static void genspriteremaps(void)
{
	int j;

	auto fr = fileSystem.OpenFileReader("lookup.dat");
	if (!fr.isOpen())
		return;

	j = lookups.loadTable(fr);

	if (j < 0)
	{
		if (j == -1)
			Printf("ERROR loading \"lookup.dat\": failed reading enough data.\n");

		return;
	}

	uint8_t paldata[768];

	for (j = 1; j <= 5; j++)
	{
		if (fr.Read(paldata, 768) != 768)
			return;

		for (int k = 0; k < 768; k++) // Build uses 6 bit VGA palettes.
			paldata[k] = (paldata[k] << 2) | (paldata[k] >> 6);

		paletteSetColorTable(j, paldata, j == DREALMSPAL || j == ENDINGPAL, j > SLIMEPAL);
	}

	for (int i = 0; i < 256; i++)
	{
		// swap red and blue channels.
		paldata[i * 3] = GPalette.BaseColors[i].b;
		paldata[i * 3 + 1] = GPalette.BaseColors[i].g;
		paldata[i * 3 + 2] = GPalette.BaseColors[i].r;
	}
	paletteSetColorTable(DRUGPAL, paldata, false, false); // todo: implement this as a shader effect (swap R and B in postprocessing.)

	if (isRR())
	{
		uint8_t table[256];
		for (j = 0; j < 256; j++)
			table[j] = j;
		for (j = 0; j < 32; j++)
			table[j] = j + 32;

		lookups.makeTable(7, table, 0, 0, 0, 0);

		for (j = 0; j < 256; j++)
			table[j] = j;
		lookups.makeTable(30, table, 0, 0, 0, 0);
		lookups.makeTable(31, table, 0, 0, 0, 0);
		lookups.makeTable(32, table, 0, 0, 0, 0);
		lookups.makeTable(33, table, 0, 0, 0, 0);
		if (isRRRA())
			lookups.makeTable(105, table, 0, 0, 0, 0);

		int unk = 63;
		for (j = 64; j < 80; j++)
		{
			unk--;
			table[j] = unk;
			table[j + 16] = j - 24;
		}
		table[80] = 80;
		table[81] = 81;
		for (j = 0; j < 32; j++)
		{
			table[j] = j + 32;
		}
		lookups.makeTable(34, table, 0, 0, 0, 0);
		for (j = 0; j < 256; j++)
			table[j] = j;
		for (j = 0; j < 16; j++)
			table[j] = j + 129;
		for (j = 16; j < 32; j++)
			table[j] = j + 192;
		lookups.makeTable(35, table, 0, 0, 0, 0);
		if (isRRRA())
		{
			lookups.makeTable(50, nullptr, 12 * 4, 12 * 4, 12 * 4, 0);
			lookups.makeTable(51, nullptr, 12 * 4, 12 * 4, 12 * 4, 0);
			lookups.makeTable(54, lookups.getTable(8), 32 * 4, 32 * 4, 32 * 4, 0);
		}
	}
}

//---------------------------------------------------------------------------
//
// Define sky layouts.
//
//---------------------------------------------------------------------------

static void setupbackdrop()
{
	static const int16_t moonoff[8] = { 0, 2, 3, 0, 2, 0, 1, 0 };
	static const int16_t orbitoff[8] = { 0, 0, 4, 0, 0, 1, 2, 3 };
	static const int16_t laoff[8] = { 1, 2, 1, 3, 4, 0, 2, 3 };
	static const int16_t defoff[8] = { 0, 1, 2, 3, 4, 5, 6, 7 };
	static const int16_t defoff1[8] = { 1, 2, 3, 4, 5, 6, 7, 0 };
	static const int16_t defoff4[8] = { 4, 5, 6, 7, 0, 1, 2, 3 };
	static const int16_t defoff7[8] = { 7, 0, 1, 2, 3, 4, 5, 6 };

	defineSky(nullptr, 3, nullptr);
	defineSky("CLOUDYOCEAN", 3, nullptr);
	defineSky("MOONSKY12", 3, moonoff);
	defineSky("BIGORBIT1", 3, orbitoff);
	defineSky("LA", 3, laoff);
	if (isWorldTour())
	{
		defineSky("SPACESKY", 3, defoff);
		defineSky("PARISSKY", 3, defoff, 80);
		defineSky("LONDONSKY", 3, defoff, 80);
		defineSky("MOSCOWSKY", 3, defoff7, 80);
		defineSky("DESERTSKY", 3, defoff, 80);
		defineSky("AMSTERDAMSKY", 3, defoff, 80);
		defineSky("HOLLYWOODSKY", 3, defoff1, 80);
		defineSky("FRISCOSKY", 3, defoff4, 80);
		defineSky("ROMESKY", 3, defoff, 80);
	}

	if (isNam())
	{
		defineSky("NAMSKY1", 3, nullptr, 0, 1, 140);
		defineSky("NAMSKY2", 3, nullptr, 0, 1, 140);
	}

	if (isWW2GI() && (g_gameType & GAMEFLAG_ADDON))
	{
		defineSky("PLATOONSKY", 3, nullptr, 0, 1, 140);
	}

	// this sky isn't actually placed wrong - it's just so poorly designed that it needs to be shifted down to hide its shortcomings as good as possible.
	if (isDuke() && (g_gameType & GAMEFLAG_DUKEDC))
	{
		defineSky("DUKEDCSKY", 3, nullptr, 0, 1, -40);
	}
}

void GameInterface::SetupSpecialTextures(TilesetBuildInfo& info)
{
	// set up all special tiles here, before we fully hook up with the texture manager.
	info.Delete(FOF);	// portal marker

	FImageSource* viewscreen;
	if (!isRR())
	{
		info.Delete(MIRROR_DUKE); // the mirror tile.
		viewscreen = info.tile[VIEWSCREEN_DUKE].tileimage;
	}
	else
	{
		info.Delete(MIRROR_RR);	// the mirror tile.
		info.Delete(0);		// RR uses this as an empty texture
		info.MakeWritable(BOWLINGLANE);	// bowling lane pin displays
		info.MakeWritable(BOWLINGLANE + 1);
		info.MakeWritable(BOWLINGLANE + 2);
		info.MakeWritable(BOWLINGLANE + 3);
		viewscreen = info.tile[VIEWSCREEN_RR].tileimage;
	}
	info.MakeCanvas(TILE_VIEWSCR, viewscreen? viewscreen->GetWidth() : 128, viewscreen? viewscreen->GetHeight() : 128);
}



void GameInterface::loadPalette()
{
	paletteLoadFromDisk();
	genspriteremaps();

	// Not exactly palette stuff, but this needs to be done before the LoadScripts call and here's a convenient place for that.
	moves.Push({});		// make sure the first entry in 'moves' is a null move.
	actions.Push({});	// make sure the first entry in 'actions' is a null action.
	ais.Push({});	// make sure the first entry in 'actions' is a null action.
}

int GameInterface::GetCurrentSkill()
{
	return ud.player_skill - 1;
}

//---------------------------------------------------------------------------
//
// IDs for non-textures that need direct checking
//
//---------------------------------------------------------------------------

void setTextureIDs()
{
	mirrortex = tileGetTextureID(isRR() ? MIRROR_RR : MIRROR_DUKE);
	foftex = tileGetTextureID(FOF);
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void initactorflags()
{
	if (!isRR())
	{
		gs.weaponsandammosprites[0] = DukeRPGSpriteClass;
		gs.weaponsandammosprites[1] = DukeChaingunSpriteClass;
		gs.weaponsandammosprites[2] = DukeDevastatorAmmoClass;
		gs.weaponsandammosprites[3] = DukeRPGAmmoClass;
		gs.weaponsandammosprites[4] = DukeRPGAmmoClass;
		gs.weaponsandammosprites[5] = DukeJetpackClass;
		gs.weaponsandammosprites[6] = DukeShieldClass;
		gs.weaponsandammosprites[7] = DukeFirstAidClass;
		gs.weaponsandammosprites[8] = DukeSteroidsClass;
		gs.weaponsandammosprites[9] = DukeRPGAmmoClass;
		gs.weaponsandammosprites[10] = DukeRPGAmmoClass;
		gs.weaponsandammosprites[11] = DukeRPGSpriteClass;
		gs.weaponsandammosprites[12] = DukeRPGAmmoClass;
		gs.weaponsandammosprites[13] = DukeFreezeSpriteClass;
		gs.weaponsandammosprites[14] = DukeFreezeAmmoClass;
	}
	else
	{
		gs.weaponsandammosprites[0] = RedneckCrossbowClass;
		gs.weaponsandammosprites[1] = RedneckRiflegunClass;
		gs.weaponsandammosprites[2] = RedneckBlasterammoClass;
		gs.weaponsandammosprites[3] = RedneckDynamiteAmmoClass;
		gs.weaponsandammosprites[4] = RedneckDynamiteAmmoClass;
		gs.weaponsandammosprites[5] = RedneckCowpieClass;
		gs.weaponsandammosprites[6] = RedneckWhiskeyClass;
		gs.weaponsandammosprites[7] = RedneckPorkRindsClass;
		gs.weaponsandammosprites[8] = RedneckMoonshineClass;
		gs.weaponsandammosprites[9] = RedneckDynamiteAmmoClass;
		gs.weaponsandammosprites[10] = RedneckDynamiteAmmoClass;
		gs.weaponsandammosprites[11] = RedneckCrossbowClass;
		gs.weaponsandammosprites[12] = RedneckDynamiteAmmoClass;
		gs.weaponsandammosprites[13] = RedneckTitgunClass;
		gs.weaponsandammosprites[14] = RedneckTitAmmoClass;

		gs.gutsscale = 0.125;
	}
}


//---------------------------------------------------------------------------
//
// set up the game module's state
//
//---------------------------------------------------------------------------

void GameInterface::app_init()
{
	RegisterClasses();
	GC::AddMarkerFunc(markgcroots);

	if (isRR()) C_SetNotifyFontScale(0.5);
	ud.god = 0;
	ud.m_respawn_items = 0;
	ud.m_respawn_monsters = 0;
	ud.m_respawn_inventory = 0;
	ud.cashman = 0;
	ud.player_skill = 2;
	ud.wchoice[0][0] = 3;
	ud.wchoice[0][1] = 4;
	ud.wchoice[0][2] = 5;
	ud.wchoice[0][3] = 7;
	ud.wchoice[0][4] = 8;
	ud.wchoice[0][5] = 6;
	ud.wchoice[0][6] = 0;
	ud.wchoice[0][7] = 2;
	ud.wchoice[0][8] = 9;
	ud.wchoice[0][9] = 1;
	ud.multimode = 1;
	ud.m_monsters_off = userConfig.nomonsters;
	ps[0].aim_mode = 1;
	ud.cameraactor = nullptr;

	if (fileSystem.FileExists("DUKESW.BIN"))
		g_gameType |= GAMEFLAG_SHAREWARE;

	numplayers = 1;
	playerswhenstarted = ud.multimode;

	connectpoint2[0] = -1;

	SetDispatcher();
	
	loadcons();
	initactorflags();
	setTextureIDs();			// sets a few texture IDs needed for map checking.
	duke_menufont->Callback(); // depends on the .CON files so it must be after loadcons

	OnEvent(EVENT_INIT);

	//Net_SendClientInfo();

	setupbackdrop();
	InitCheats();
	checkcommandline();
	registerosdcommands();

	screenpeek = myconnectindex;

	C_InitConback(TexMan.CheckForTexture("MENUSCREEN", ETextureType::Any), false, 0.75);

	if (ud.multimode > 1)
	{
		ud.m_monsters_off = 1;
		//ud.player_skill = 0;
	}

	ud.last_level = -1;
	enginecompatibility_mode = ENGINECOMPATIBILITY_19961112;//bVanilla;
	S_ParseDeveloperCommentary();
}

void GameInterface::FinalizeSetup()
{
	for (int i = 0; i < MAXTILES; i++)
	{
		auto& actinf = gs.actorinfo[i];
		if (actinf.scriptaddress != 0)
		{
			int act = ScriptCode[actinf.scriptaddress + 1];
			int cmd = ScriptCode[actinf.scriptaddress + 4];
			auto info = spawnMap.CheckKey(i);
			PClassActor* cls = nullptr;

			if (info != nullptr && !info->basetex.isValid())
			{
				cls = info->cls;
			}
			else if (info == nullptr || !info->basetex.isValid())
			{
				// No unique actor class exists here. Since we need one, create a new class here, directly derived from DDukeActor.
				auto newcls = (PClassActor*)RUNTIME_CLASS(DDukeActor)->CreateDerivedClass(FStringf("NewConActor%d", i), RUNTIME_CLASS(DDukeActor)->Size);
				newcls->InitializeDefaults();
				insertSpawnType(i, { newcls, FNullTextureID(), FNullTextureID(), NO_SOUND, int8_t(0), int8_t(0), int16_t(0x8000) });
				cls = newcls;
				GetDefaultByType(newcls)->spr.setspritetexture(tileGetTextureID(i)); // make it show the right pic.
			}
			else
			{
				// the ugly case: This tries to replace a variant of a special actor.
				// All of Duke's entries falling in this category are coded to not execute scripts at all with no possible override.
				// this means that none of these actors can ever run its scripts.

			}

			// now copy all data over so that we don't have to do double maintenance.
			if (cls)
			{
				cls->ActorInfo()->TypeNum = i;
				GetDefaultByType(cls)->IntVar(NAME_strength) = ScriptCode[actinf.scriptaddress];
				if (actinf.enemyflags &  EDukeFlags1::FromInt(1))
				{
					auto def = static_cast<DDukeActor*>(GetDefaultByType(cls));
					auto fb = (SFLAG_BADGUY | SFLAG_KILLCOUNT | SFLAG_BADGUYSTAYPUT);
					auto check = (def->flags1 & (SFLAG_BADGUY | SFLAG_KILLCOUNT));
					// do not enable KILLCOUNT if it the base is a non-counting badguy. This is needed for RR's animals.
					if (check == EDukeFlags1::FromInt(SFLAG_BADGUY)) fb &= ~SFLAG_KILLCOUNT; 
					def->flags1 = (def->flags1 & ~fb) | (actinf.enemyflags & fb);
				}

			}
		}
		ScriptCode[actinf.scriptaddress] = 0; // ignore strength values for hashing the script code.
		// todo: hash the entire script code and compare against precalculated value for the current game.
		// If identical, remove all ScriptAddresses from the class list.

	}

	// flag all actors which override RunState so we can quickly check for this in the game,
	auto VIndex = GetVirtualIndex(RUNTIME_CLASS(DDukeActor), "RunState");
	assert(VIndex != ~0u);
	auto RunState = RUNTIME_CLASS(DDukeActor)->Virtuals[VIndex];

	for (auto& cls : PClassActor::AllActorClasses)
	{
		if (cls->IsDescendantOf(RUNTIME_CLASS(DDukeActor)) && cls->Virtuals[VIndex] != RunState)
		{
			auto def = static_cast<DDukeActor*>(GetDefaultByType(cls));
			def->flags4 |= SFLAG4_CONOVERRIDE;

			auto ainf = static_cast<PClassActor*>(cls)->ActorInfo();
			for (int i = 0; i < ainf->NumAIs; i++)
			{
				auto ai = &ais[ainf->FirstAI + i];

				if (ai->move & 0x80000000)
				{
					auto nm = FName(ENamedName(ai->move & ~0x80000000));
					int newmove = LookupMove(cls, nm);
					if (newmove == 0)
					{
						Printf(TEXTCOLOR_RED "Invalid move '%s' in AI '%s' for class '%s'\n", nm.GetChars(), ai->name.GetChars(), cls->TypeName.GetChars());
					}
					ai->move = newmove;
				}
				if (ai->action & 0x80000000)
				{
					auto nm = FName(ENamedName(ai->action & ~0x80000000));
					int newaction = LookupAction(cls, nm);
					if (newaction == 0)
					{
						Printf(TEXTCOLOR_RED "Invalid action '%s' in AI '%s' for class '%s'\n", nm.GetChars(), ai->name.GetChars(), cls->TypeName.GetChars());
					}
					ai->action = newaction;
				}
			}



		}
	}

}


int LookupAction(PClass* cls, FName name)
{
	while (true)
	{
		auto ainf = static_cast<PClassActor*>(cls)->ActorInfo();
		for (int i = 0; i < ainf->NumActions; i++)
		{
			if (actions[ainf->FirstAction + i].name == name) return ainf->FirstAction + i;
		}
		if (cls == RUNTIME_CLASS(DDukeActor)) return 0;
		cls = cls->ParentClass;
	}
}

int LookupMove(PClass* cls, FName name)
{
	while (true)
	{
		auto ainf = static_cast<PClassActor*>(cls)->ActorInfo();
		for (int i = 0; i < ainf->NumMoves; i++)
		{
			if (moves[ainf->FirstMove + i].name == name) return ainf->FirstMove + i;
		}
		if (cls == RUNTIME_CLASS(DDukeActor)) return 0;
		cls = cls->ParentClass;
	}
}

int LookupAI(PClass* cls, FName name)
{
	while (true)
	{
		auto ainf = static_cast<PClassActor*>(cls)->ActorInfo();
		for (int i = 0; i < ainf->NumAIs; i++)
		{
			if (ais[ainf->FirstAI + i].name == name) return ainf->FirstAI + i;
		}
		if (cls == RUNTIME_CLASS(DDukeActor)) return 0;
		cls = cls->ParentClass;
	}
}



void CallInitialize(DDukeActor* actor)
{
	IFVIRTUALPTR(actor, DDukeActor, Initialize)
	{
		VMValue val = actor;
		VMCall(func, &val, 1, nullptr, 0);
	}
}

void CallTick(DDukeActor* actor)
{
	IFVIRTUALPTR(actor, DDukeActor, Tick)
	{
		VMValue val = actor;
		VMCall(func, &val, 1, nullptr, 0);
	}
}

void CallAction(DDukeActor* actor)
{
	IFVIRTUALPTR(actor, DDukeActor, RunState)
	{
		VMValue val = actor;
		VMCall(func, &val, 1, nullptr, 0);
	}
}

void checkhitsprite(DDukeActor* actor, DDukeActor* hitter)
{
	IFVIRTUALPTR(actor, DDukeActor, onHit)
	{
		VMValue val[2] = { actor, hitter };
		VMCall(func, val, 2, nullptr, 0);
	}
}

void CallOnHurt(DDukeActor* actor, player_struct* hitter)
{
	IFVIRTUALPTR(actor, DDukeActor, onHurt)
	{
		VMValue val[2] = { actor, hitter };
		VMCall(func, val, 2, nullptr, 0);
	}
}

void CallOnTouch(DDukeActor* actor, player_struct* hitter)
{
	IFVIRTUALPTR(actor, DDukeActor, onTouch)
	{
		VMValue val[2] = { actor, hitter };
		VMCall(func, val, 2, nullptr, 0);
	}
}


bool CallOnUse(DDukeActor* actor, player_struct* user)
{
	int nval = false;
	IFVIRTUALPTR(actor, DDukeActor, onUse)
	{
		VMValue val[2] = { actor, user };
		VMReturn ret(&nval);
		VMCall(func, val, 2, &ret, 1);
	}
	return nval;
}

void CallOnMotoSmash(DDukeActor* actor, player_struct* hitter)
{
	IFVIRTUALPTR(actor, DDukeActor, onMotoSmash)
	{
		VMValue val[2] = { actor, hitter };
		VMCall(func, val, 2, nullptr, 0);
	}
}


void CallOnRespawn(DDukeActor* actor, int low)
{
	IFVIRTUALPTR(actor, DDukeActor, onRespawn)
	{
		VMValue val[2] = { actor, low };
		VMCall(func, val, 2, nullptr, 0);
	}
}

bool CallAnimate(DDukeActor* actor, tspritetype* tspr)
{
	int nval = false;
	IFVIRTUALPTR(actor, DDukeActor, animate)
	{
		VMReturn ret(& nval);
		VMValue val[2] = { actor, tspr };
		VMCall(func, val, 2, &ret, 1);
	}
	return nval;
}

void CallStaticSetup(DDukeActor* actor)
{
	IFVIRTUALPTR(actor, DDukeActor, StaticSetup)
	{
		VMValue val = actor;
		VMCall(func, &val, 1, nullptr, 0);
	}
}

bool CallShootThis(DDukeActor* clsdef, DDukeActor* actor, int pn, const DVector3& spos, DAngle sang)
{
	int rv = 0;
	VMReturn ret(&rv);
	IFVIRTUALPTR(clsdef, DDukeActor, ShootThis)
	{
		VMValue val[] = {clsdef, actor, pn >= 0? &ps[pn] : nullptr, spos.X, spos.Y, spos.Z, sang.Degrees()};
		VMCall(func, val, 7, &ret, 1);
	}
	return rv;
}

void CallPlayFTASound(DDukeActor* actor, int mode)
{
	IFVIRTUALPTR(actor, DDukeActor, PlayFTASound)
	{
		VMValue val[] = { actor, mode };
		VMCall(func, val, 2, nullptr, 0);
	}
}

void CallStandingOn(DDukeActor* actor, player_struct* p)
{
	IFVIRTUALPTR(actor, DDukeActor, StandingOn)
	{
		VMValue val[] = { actor, p };
		VMCall(func, val, 2, nullptr, 0);
	}
}

int CallTriggerSwitch(DDukeActor* actor, player_struct* p)
{
	int nval = false;
	IFVIRTUALPTR(actor, DDukeActor, TriggerSwitch)
	{
		VMReturn ret(&nval);
		VMValue val[] = { actor, p };
		VMCall(func, val, 2, &ret, 1);
	}
	return nval;
}

PClassActor* CallGetRadiusDamageType(DDukeActor* actor, int targhealth)
{
	PClassActor* nval = nullptr;
	IFVIRTUALPTR(actor, DDukeActor, GetRadiusDamageType)
	{
		VMReturn ret;
		ret.PointerAt((void**)&nval);
		VMValue val[] = { actor, targhealth };
		VMCall(func, val, 2, &ret, 1);
	}
	return nval;
}


//==========================================================================
//
// Sets up the flag defaults which differ between RR and Duke.
// 
//==========================================================================

DEFINE_PROPERTY(setgamedefaults, 0, DukeActor)
{
	if (!isRR())
	{
		defaults->flags1 |= SFLAG_LOOKALLAROUND; // feature comes from RR, but we want the option in Duke as well, so this fake property sets the default
		defaults->FloatVar(NAME_shootzoffset) = -7;
	}
	else
	{
		defaults->flags1 |= SFLAG_MOVEFTA_WAKEUPCHECK; // Animals were not supposed to have this, but due to a coding bug the logic was unconditional for everything in the game.
		defaults->flags2 |= SFLAG2_NODAMAGEPUSH;		// RR does not have this feature, so set the flag for everything, this allows disabling it if wanted later.
		defaults->flags3 |= SFLAG3_RANDOMANGLEONWATER;	// RR does this for all badguys, Duke only for the LizMan.
	}
}

//==========================================================================
//
// The 3 major CON related properties - moves, actions, ais.
// 
//==========================================================================

DEFINE_PROPERTY(move, Sii, DukeActor)
{
	if (info->ActorInfo()->NumMoves++ == 0) info->ActorInfo()->FirstMove = moves.Size();
	auto move = &moves[moves.Reserve(1)];
	move->movex = move->movez = 0;
	PROP_STRING_PARM(n, 0);
	move->name = n;
	move->qualifiedName = FStringf("%s.%s", info->TypeName.GetChars(), n);
	if (PROP_PARM_COUNT > 1)
	{
		PROP_INT_PARM(v, 1);
		move->movex = v / 16.f;
		if (PROP_PARM_COUNT > 2)
		{
			PROP_INT_PARM(v2, 2);
			move->movez = v2 / 16.f;
		}
	}
}

DEFINE_PROPERTY(movef, Sff, DukeActor)
{
	if (info->ActorInfo()->NumMoves++ == 0) info->ActorInfo()->FirstMove = moves.Size();
	auto move = &moves[moves.Reserve(1)];
	move->movex = move->movez = 0;
	PROP_STRING_PARM(n, 0);
	move->name = n;
	move->qualifiedName = FStringf("%s.%s", info->TypeName.GetChars(), n);
	if (PROP_PARM_COUNT > 1)
	{
		PROP_FLOAT_PARM(v, 1);
		move->movex = v;
		if (PROP_PARM_COUNT > 2)
		{
			PROP_FLOAT_PARM(v2, 2);
			move->movez = v2;
		}
	}
}

DEFINE_PROPERTY(action, SZIiiii, DukeActor)
{
	if (info->ActorInfo()->NumActions++ == 0) info->ActorInfo()->FirstAction = actions.Size();
	auto action = &actions[actions.Reserve(1)];
	memset(action, 0, sizeof(*action));
	PROP_STRING_PARM(n, 0);
	action->name = n;
	action->qualifiedName = FStringf("%s.%s", info->TypeName.GetChars(), n);
	PROP_STRING_PARM(b, 1);
	action->base = b == nullptr ? FNullTextureID() : TexMan.CheckForTexture(b, ETextureType::Any, FTextureManager::TEXMAN_ReturnAll | FTextureManager::TEXMAN_TryAny | FTextureManager::TEXMAN_ForceLookup);
	PROP_INT_PARM(v2, 2);
	action->offset = v2;

	if (PROP_PARM_COUNT > 3)
	{
		PROP_INT_PARM(v3, 3);
		action->numframes = v3;
		if (PROP_PARM_COUNT > 4)
		{
			PROP_INT_PARM(v4, 4);
			action->rotationtype = v4;
			if (PROP_PARM_COUNT > 5)
			{
				PROP_INT_PARM(v5, 5);
				action->increment = v5;
				if (PROP_PARM_COUNT > 6)
				{
					PROP_INT_PARM(v6, 6);
					action->delay = v6;
				}
			}
		}
	}
}

DEFINE_PROPERTY(ai, SSSi, DukeActor)
{
	if (info->ActorInfo()->NumAIs++ == 0) info->ActorInfo()->FirstAI = ais.Size();
	auto ai = &ais[ais.Reserve(1)];
	ai->moveflags = 0;
	PROP_STRING_PARM(n, 0);
	ai->name = n;
	PROP_NAME_PARM(na, 1);
	ai->action = na == NAME_None? 0 : na.GetIndex() | 0x80000000;	// don't look it up yet if not 'none'
	PROP_NAME_PARM(nm, 2);
	ai->move = nm == NAME_None ? 0 : nm.GetIndex() | 0x80000000;	// don't look it up yet if not 'none'
	if (PROP_PARM_COUNT > 3)
	{
		PROP_INT_PARM(v3, 3);
		ai->moveflags = v3;
	}
}

DEFINE_PROPERTY(startaction, S, DukeActor)
{
	PROP_STRING_PARM(n, 0);
	info->ActorInfo()->DefaultAction = n;
}

DEFINE_PROPERTY(startmove, S, DukeActor)
{
	PROP_STRING_PARM(n, 0);
	info->ActorInfo()->DefaultMove = n;
}

DEFINE_PROPERTY(moveflags, I, DukeActor)
{
	PROP_INT_PARM(n, 0);
	info->ActorInfo()->DefaultMoveflags = n;
}

CCMD(changewalltexture)
{
	if (argv.argc() < 2) return;
	FTextureID tile = TexMan.CheckForTexture(argv[1], ETextureType::Any);
	if (!tile.isValid()) tile = tileGetTextureID((int)strtol(argv[1], nullptr, 10));
	HitInfoBase hit;
	hitscan(ps[0].actor->spr.pos, ps[0].cursector, DVector3(ps[0].actor->spr.Angles.Yaw.ToVector(), 0) * 1024, hit, CLIPMASK1);
	if (hit.hitWall)
	{
		hit.hitWall->setwalltexture(tile);
	}
}



END_DUKE_NS
