/*
** g_level.cpp
** Parses MAPINFO
**
**---------------------------------------------------------------------------
** Copyright 1998-2016 Randy Heit
** Copyright 2009-2021 Christoph Oelckers
** All rights reserved.
**
** Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions
** are met:
**
** 1. Redistributions of source code must retain the above copyright
**    notice, this list of conditions and the following disclaimer.
** 2. Redistributions in binary form must reproduce the above copyright
**    notice, this list of conditions and the following disclaimer in the
**    documentation and/or other materials provided with the distribution.
** 3. The name of the author may not be used to endorse or promote products
**    derived from this software without specific prior written permission.
**
** THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
** IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
** OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
** IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
** INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
** NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
** THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
**---------------------------------------------------------------------------
**
*/

#include <assert.h>
#include "mapinfo.h"
#include "g_mapinfo.h"
#include "templates.h"
#include "filesystem.h"
#include "cmdlib.h"
#include "v_video.h"
#include "gi.h"
#include "gstrings.h"
#include "autosegs.h"
#include "i_system.h"
#include "gamecontrol.h"
#include "autosegs.h"

extern TArray<ClusterDef> clusters;
extern TArray<VolumeRecord> volumes;
extern TArray<TPointer<MapRecord>> mapList;	// must be allocated as pointers because it can whack the currentlLevel pointer if this was a flat array.

static MapRecord TheDefaultLevelInfo;
static ClusterDef TheDefaultClusterInfo;

TArray<int> ParsedLumps(8);

//==========================================================================
//
//
//==========================================================================

void FMapInfoParser::ParseOpenBrace()
{
	sc.MustGetStringName("{");
	sc.SetCMode(true);
}

//==========================================================================
//
//
//==========================================================================

bool FMapInfoParser::ParseCloseBrace()
{
	return sc.Compare("}");
}

//==========================================================================
//
//
//==========================================================================

bool FMapInfoParser::CheckAssign()
{
	return sc.CheckString("=");
}

//==========================================================================
//
//
//==========================================================================

void FMapInfoParser::ParseAssign()
{
	sc.MustGetStringName("=");
}

//==========================================================================
//
//
//==========================================================================

void FMapInfoParser::MustParseAssign()
{
	sc.MustGetStringName("=");
}

//==========================================================================
//
//
//==========================================================================

void FMapInfoParser::ParseComma()
{
	sc.MustGetStringName(",");
}

//==========================================================================
//
//
//==========================================================================

bool FMapInfoParser::CheckNumber()
{
	if (sc.CheckString(","))
	{
		sc.MustGetNumber();
		return true;
	}
	return false;
}

//==========================================================================
//
//
//==========================================================================

bool FMapInfoParser::CheckFloat()
{
	if (sc.CheckString(","))
	{
		sc.MustGetFloat();
		return true;
	}
	return false;
}

//==========================================================================
//
// skips an entire parameter list that's separated by commas
//
//==========================================================================

void FMapInfoParser::SkipToNext()
{
	if (sc.CheckString("="))
	{
		do
		{
			sc.MustGetString();
		}
		while (sc.CheckString(","));
	}
}


//==========================================================================
//
// checks if the current block was properly terminated
//
//==========================================================================

void FMapInfoParser::CheckEndOfFile(const char *block)
{
	if (sc.End)
	{
		sc.ScriptError("Unexpected end of file in %s definition", block);
	}
}

//==========================================================================
//
// ParseLookupname
//
//==========================================================================

bool FMapInfoParser::ParseLookupName(FString &dest)
{
	sc.MustGetString();
	dest = sc.String;
	return true;
}

//==========================================================================
//
//
//==========================================================================

void FMapInfoParser::ParseLumpOrTextureName(FString &name)
{
	sc.MustGetString();
	name = sc.String;
}


//==========================================================================
//
//
//==========================================================================

void FMapInfoParser::ParseMusic(FString &name, int &order)
{
	sc.MustGetString();
	name = sc.String;
	if (CheckNumber())
	{
		order = sc.Number;
	}
}

//==========================================================================
//
//
//
//==========================================================================

void FMapInfoParser::ParseCutscene(CutsceneDef& cdef)
{
	FString sound;
	sc.MustGetStringName("{");
	while (!sc.CheckString("}"))
	{
		sc.MustGetString();
		if (sc.Compare("video")) { ParseAssign(); sc.MustGetString(); cdef.video = sc.String; cdef.function = ""; }
		else if (sc.Compare("function")) { ParseAssign(); sc.SetCMode(false); sc.MustGetString(); sc.SetCMode(true); cdef.function = sc.String; cdef.video = ""; }
		else if (sc.Compare("sound")) { ParseAssign(); sc.MustGetString(); cdef.soundName = sc.String; }
		else if (sc.Compare("soundid")) { ParseAssign(); sc.MustGetNumber(); cdef.soundID = sc.Number; }
		else if (sc.Compare("fps")) { ParseAssign();  sc.MustGetNumber();  cdef.framespersec = sc.Number; }
		else if (sc.Compare("transitiononly")) cdef.transitiononly = true;
		else if (sc.Compare("delete")) { cdef.function = "none"; cdef.video = ""; } // this means 'play nothing', not 'not defined'.
		else if (sc.Compare("clear")) cdef = {};
	}
}

//==========================================================================
//
//
//
//==========================================================================

void FMapInfoParser::ParseCluster()
{
	sc.MustGetNumber ();
	auto clusterinfo = MustFindCluster(sc.Number);

	ParseOpenBrace();
	while (sc.GetString())
	{
		if (sc.Compare("clear"))
		{
			*clusterinfo = {};
		}
		else if (sc.Compare("name"))
		{
			ParseAssign();
			ParseLookupName(clusterinfo->name);
		}
		else if (sc.Compare("intro"))
		{
			ParseCutscene(clusterinfo->intro);
		}
		else if (sc.Compare("outro"))
		{
			ParseCutscene(clusterinfo->outro);
		}
		else if (sc.Compare("gameover"))
		{
			ParseCutscene(clusterinfo->gameover);
		}
		else if (sc.Compare("interbackground"))
		{
			ParseAssign();
			ParseLookupName(clusterinfo->InterBackground);
		}
		else if (!ParseCloseBrace())
		{
			// Unknown
			sc.ScriptMessage("Unknown property '%s' found in cluster definition\n", sc.String);
			SkipToNext();
		}
		else
		{
			break;
		}
	}
	CheckEndOfFile("cluster");
}

//==========================================================================
//
// allow modification of maps defined through legacy means.
//
//==========================================================================

bool FMapInfoParser::CheckLegacyMapDefinition(FString& mapname)
{
	if (Internal && (g_gameType & GAMEFLAG_BLOOD | GAMEFLAG_DUKECOMPAT | GAMEFLAG_SW) && sc.CheckString("{"))
	{
		sc.MustGetNumber();
		int vol = sc.Number;
		if (!isSWALL())
		{
			// Blood and Duke use volume/level pairs
			sc.MustGetStringName(",");
			sc.MustGetNumber();
			int indx = sc.Number;
			auto map = FindMapByIndexOnly(vol, indx);
			if (!map) mapname = "";
			else mapname = map->labelName;
		}
		else
		{
			// SW only uses the level number
			auto map = FindMapByLevelNum(vol);
			if (!map) mapname = "";
			else mapname = map->labelName;
		}
		sc.MustGetStringName("}");
		return true;
	}
	return false;
}

//==========================================================================
//
// ParseNextMap
// Parses a next map field
//
//==========================================================================

void FMapInfoParser::ParseMapName(FString &mapname)
{
	if (!CheckLegacyMapDefinition(mapname))
	{
		sc.MustGetString();
		mapname = ExtractFileBase(sc.String);
	}
}

//==========================================================================
//
// Map options
//
//==========================================================================

DEFINE_MAP_OPTION(clear, true)
{
	// Save the names, reset and restore the names
	FString fn = info->fileName;
	FString dn = info->name;
	FString ln = info->labelName;
	*info = *parse.defaultinfoptr;
	info->fileName = fn;
	info->name = dn;
	info->labelName = ln;
}


DEFINE_MAP_OPTION(levelnum, true)
{
	parse.ParseAssign();
	parse.sc.MustGetNumber();
	info->levelNumber = parse.sc.Number;
}

DEFINE_MAP_OPTION(next, true)
{
	parse.ParseAssign();
	parse.ParseMapName(info->NextMap);
}

DEFINE_MAP_OPTION(author, true)
{
	parse.ParseAssign();
	parse.sc.MustGetString();
	info->Author = parse.sc.String;
}

DEFINE_MAP_OPTION(secretnext, true)
{
	parse.ParseAssign();
	parse.ParseMapName(info->NextSecret);
}

DEFINE_MAP_OPTION(cluster, true)
{
	parse.ParseAssign();
	parse.sc.MustGetNumber();
	info->cluster = parse.sc.Number;

	// If this cluster hasn't been defined yet, add it.
	MustFindCluster(info->cluster);
}

DEFINE_MAP_OPTION(fade, true)
{
	parse.ParseAssign();
	parse.sc.MustGetString();
	info->fadeto = V_GetColor(nullptr, parse.sc);
}

DEFINE_MAP_OPTION(partime, true)
{
	parse.ParseAssign();
	parse.sc.MustGetNumber();
	info->parTime = parse.sc.Number;
}

DEFINE_MAP_OPTION(designertime, true)
{
	parse.ParseAssign();
	parse.sc.MustGetNumber();
	info->designerTime = parse.sc.Number;
}

DEFINE_MAP_OPTION(music, true)
{
	parse.ParseAssign();
	parse.ParseMusic(info->music, info->musicorder);
}

DEFINE_MAP_OPTION(cdtrack, true)
{
	parse.ParseAssign();
	parse.sc.MustGetNumber();
	info->cdSongId = parse.sc.Number;
}

DEFINE_MAP_OPTION(intro, true)
{
	parse.ParseCutscene(info->intro);
}

DEFINE_MAP_OPTION(outro, true)
{
	parse.ParseCutscene(info->outro);
}

DEFINE_MAP_OPTION(interbackground, true)
{
	parse.ParseAssign();
	parse.sc.MustGetString();
	info->InterBackground = parse.sc.String;
}


/* currently all sounds are precached. This requires significant work on sound management and info collection.
DEFINE_MAP_OPTION(PrecacheSounds, true)
{
	parse.ParseAssign();

	do
	{
		parse.sc.MustGetString();
		FSoundID snd = parse.sc.String;
		if (snd == 0)
		{
			parse.sc.ScriptMessage("Unknown sound \"%s\"", parse.sc.String);
		}
		else
		{
			info->PrecacheSounds.Push(snd);
		}
	} while (parse.sc.CheckString(","));
}
*/

DEFINE_MAP_OPTION(PrecacheTextures, true)
{
	parse.ParseAssign();
	do
	{
		parse.sc.MustGetString();
		//the texture manager is not initialized here so all we can do is store the texture's name.
		info->PrecacheTextures.Push(parse.sc.String);
	} while (parse.sc.CheckString(","));
}

DEFINE_MAP_OPTION(bordertexture, true)
{
	parse.ParseAssign();
	parse.ParseLumpOrTextureName(info->BorderTexture);
}

DEFINE_MAP_OPTION(fogdensity, false)
{
	parse.ParseAssign();
	parse.sc.MustGetNumber();
	info->fogdensity = clamp(parse.sc.Number, 0, 512) >> 1;
}

DEFINE_MAP_OPTION(skyfog, false)
{
	parse.ParseAssign();
	parse.sc.MustGetNumber();
	info->skyfog = parse.sc.Number;
}

DEFINE_MAP_OPTION(message, false)
{
	parse.ParseAssign();
	parse.sc.MustGetNumber();
	if (parse.sc.Number < 1 || parse.sc.Number > MAX_MESSAGES) parse.sc.ScriptError("Invalid message ID %d - must be 1..32", parse.sc.Number);
	int num = parse.sc.Number;
	parse.ParseComma();
	parse.sc.MustGetString();
	info->messages[num] = parse.sc.String;
}

/* stuff for later when the new renderer is done.
DEFINE_MAP_OPTION(lightmode, false)
{
	parse.ParseAssign();
	parse.sc.MustGetNumber();

	if ((parse.sc.Number >= 0 && parse.sc.Number <= 4) || parse.sc.Number == 8 || parse.sc.Number == 16)
	{
		info->lightmode = ELightMode(parse.sc.Number);
	}
	else
	{
		parse.sc.ScriptMessage("Invalid light mode %d", parse.sc.Number);
	}
}
*/

DEFINE_MAP_OPTION(skyrotate, false)
{
	parse.ParseAssign();
	parse.sc.MustGetFloat();
	info->skyrotatevector.X = (float)parse.sc.Float;
	parse.sc.MustGetStringName(",");
	parse.sc.MustGetFloat();
	info->skyrotatevector.Y = (float)parse.sc.Float;
	parse.sc.MustGetStringName(",");
	parse.sc.MustGetFloat();
	info->skyrotatevector.Z = (float)parse.sc.Float;
	info->skyrotatevector.W = 0;
	info->skyrotatevector.MakeUnit();
	parse.sc.MustGetStringName(",");
	parse.sc.MustGetFloat();
	info->skyrotatevector.W = (float)parse.sc.Float; // W is the rotation speed. This must not be normalized
}

DEFINE_MAP_OPTION(rr_startsound, false)
{
	parse.ParseAssign();
	parse.sc.MustGetNumber();
	info->rr_startsound = parse.sc.Number;
}

DEFINE_MAP_OPTION(rr_mamaspawn, false)
{
	parse.ParseAssign();
	parse.sc.MustGetNumber();
	info->rr_mamaspawn = parse.sc.Number;
}

DEFINE_MAP_OPTION(ex_ramses_horiz, false)
{
	parse.ParseAssign();
	parse.sc.MustGetNumber();
	info->ex_ramses_horiz = parse.sc.Number;
}

DEFINE_MAP_OPTION(ex_ramses_cdtrack, false)
{
	parse.ParseAssign();
	parse.sc.MustGetNumber();
	info->ex_ramses_cdtrack = parse.sc.Number;
}

DEFINE_MAP_OPTION(ex_ramses_pup, false)
{
	parse.ParseAssign();
	parse.sc.MustGetString();
	info->ex_ramses_pup = parse.sc.String;
}

DEFINE_MAP_OPTION(ex_ramses_text, false)
{
	parse.ParseAssign();
	parse.sc.MustGetString();
	info->ex_ramses_text = parse.sc.String;
}

int ex_ramses_horiz = 11;
int ex_ramses_cdtrack = -1; // this is not music, it is the actual dialogue!
FString ex_ramses_pup;
FString ex_ramses_text;

//==========================================================================
//
// All flag based map options 
//
//==========================================================================

enum EMIType
{
	MITYPE_IGNORE,
	MITYPE_EATNEXT,
	MITYPE_SETFLAG,
	MITYPE_CLRFLAG,
	MITYPE_SCFLAGS,
	MITYPE_SETFLAGG,
	MITYPE_CLRFLAGG,
	MITYPE_SCFLAGSG,
	MITYPE_COMPATFLAG,
};

struct MapInfoFlagHandler
{
	const char *name;
	EMIType type;
	uint32_t data1, data2;
	int gameflagmask;
}
MapFlagHandlers[] =
{
	{ "nointermission",					MITYPE_SETFLAG,	LEVEL_NOINTERMISSION, 0, -1 },
	{ "secretexitoverride",				MITYPE_SETFLAG,	LEVEL_SECRETEXITOVERRIDE, 0, -1 },
	{ "clearinventory",					MITYPE_SETFLAG, LEVEL_CLEARINVENTORY, 0, -1 },
	{ "clearweapons",					MITYPE_SETFLAG, LEVEL_CLEARWEAPONS, 0, -1 },
	{ "forcenoeog",						MITYPE_SETFLAG, LEVEL_FORCENOEOG, 0, -1 },
	{ "wt_bossspawn",					MITYPE_SETFLAG, LEVEL_WT_BOSSSPAWN, 0, -1 },
	{ "rrra_hulkspawn",					MITYPE_SETFLAGG,LEVEL_RR_HULKSPAWN, 0, GAMEFLAG_RRRA },
	{ "rr_clearmoonshine",				MITYPE_SETFLAGG,LEVEL_RR_CLEARMOONSHINE, 0, GAMEFLAG_RR },
	{ "ex_training",					MITYPE_SETFLAGG,LEVEL_EX_TRAINING, 0, GAMEFLAG_PSEXHUMED },
	{ "ex_altsound",					MITYPE_SETFLAGG,LEVEL_EX_ALTSOUND, 0, GAMEFLAG_PSEXHUMED },
	{ "ex_countdown",					MITYPE_SETFLAGG,LEVEL_EX_COUNTDOWN, 0, GAMEFLAG_PSEXHUMED },
	{ "ex_multi",						MITYPE_SETFLAGG,LEVEL_EX_MULTI, 0, GAMEFLAG_PSEXHUMED },
	{ "sw_bossmeter_serpent",			MITYPE_SETFLAGG,LEVEL_SW_BOSSMETER_SERPENT, 0, GAMEFLAG_SW },
	{ "sw_bossmeter_sumo",				MITYPE_SETFLAGG,LEVEL_SW_BOSSMETER_SUMO, 0, GAMEFLAG_SW },
	{ "sw_bossmeter_zilla",				MITYPE_SETFLAGG,LEVEL_SW_BOSSMETER_ZILLA, 0, GAMEFLAG_SW },
	{ "sw_deathexit_serpent",			MITYPE_SETFLAGG,LEVEL_SW_DEATHEXIT_SERPENT, 0, GAMEFLAG_SW },
	{ "sw_deathexit_serpent_next",		MITYPE_SETFLAGG,LEVEL_SW_DEATHEXIT_SERPENT | LEVEL_SW_DEATHEXIT_SERPENT_NEXT, 0, GAMEFLAG_SW },
	{ "sw_deathexit_sumo",				MITYPE_SETFLAGG,LEVEL_SW_DEATHEXIT_SUMO, 0, GAMEFLAG_SW },
	{ "sw_deathexit_zilla",				MITYPE_SETFLAGG,LEVEL_SW_DEATHEXIT_ZILLA, 0, GAMEFLAG_SW },
	{ "sw_spawnmines",					MITYPE_SETFLAGG,LEVEL_SW_SPAWNMINES, 0, GAMEFLAG_SW },

	{ NULL, MITYPE_IGNORE, 0, 0}
};

void PrintCutscene(const char* name, CutsceneDef& cut)
{
	if (cut.function.IsEmpty() && cut.video.IsEmpty()) return;
	Printf("\t%s\n\t{\n", name);
	if (cut.function.IsNotEmpty())
	{
		Printf("\t\tfunction = %s\n", cut.function.GetChars());
	}
	if (cut.video.IsNotEmpty())
	{
		Printf("\t\tvideo = \"%s\"\n", cut.video.GetChars());
	}
	if (cut.soundName.IsNotEmpty())
	{
		Printf("\t\tsound = \"%s\"\n", cut.soundName.GetChars());
	}
	Printf("\t}\n");
}

CCMD(mapinfo)
{
	for (auto& vol : volumes)
	{
		Printf("episode %s\n{\n", vol.startmap.GetChars());
		if (vol.name.IsNotEmpty()) Printf("\tname = \"%s\"\n", vol.name.GetChars());
		if (vol.subtitle.IsNotEmpty()) Printf("\tsubtitle = \"%s\"\n{\n", vol.subtitle.GetChars());
		Printf("}\n");
	}
	for (auto& clust : clusters)
	{
		Printf("cluster %d\n{\n", clust.index);
		if (clust.name.IsNotEmpty()) Printf("\tname = \"%s\"\n", clust.name.GetChars());
		if (clust.InterBackground.IsNotEmpty()) Printf("\tInterBackground = %s\n", clust.InterBackground.GetChars());
		PrintCutscene("intro", clust.intro);
		PrintCutscene("outro", clust.outro);
		PrintCutscene("gameover", clust.gameover);
		Printf("}\n");
	}
	for (auto& map : mapList)
	{
		int lump = fileSystem.FindFile(map->fileName);
		if (lump >= 0)
		{
			int rfnum = fileSystem.GetFileContainer(lump);
			Printf("map %s \"%s\"\n{\n", map->labelName.GetChars(), map->DisplayName());
			Printf("\tlevelnum = %d\n\tCluster = %d\n", map->levelNumber, map->cluster);
			if (map->Author.IsNotEmpty())
			{
				FString auth = map->Author;
				auth.Substitute("\"", "\\\"");
				Printf("\tAuthor = \"%s\"\n", auth.GetChars());
			}
			if (map->NextMap.IsNotEmpty()) Printf("\tNext = %s\n", map->NextMap.GetChars());
			if (map->NextSecret.IsNotEmpty()) Printf("\tSecretNext = %s\n", map->NextSecret.GetChars());
			if (map->InterBackground.IsNotEmpty()) Printf("\tInterBackground = %s\n", map->InterBackground.GetChars());
			if (map->music.IsNotEmpty()) Printf("\tMusic = \"%s\"\n", map->music.GetChars());
			if (map->musicorder > 0) Printf("\tMusicorder = %d\n", map->musicorder);
			if (map->cdSongId > 0) Printf("\tCDtrack = %d\n", map->cdSongId);
			if (map->parTime) Printf("\tParTime = %d\n", map->parTime);
			if (map->designerTime) Printf("\tDesignerTime = %d\n", map->designerTime);
			for (int i = 0; i < MAX_MESSAGES; i++)
			{
				if (map->messages[i].IsNotEmpty()) Printf("\tMessage = %d, \"%s\"\n", i + 1, map->messages[i].GetChars());
			}

			for (auto& flagh : MapFlagHandlers)
			{
				if (flagh.type == MITYPE_SETFLAG)
				{
					if (map->flags & flagh.data1) Printf("\t%s\n", flagh.name);
				}
				if (flagh.type == MITYPE_SETFLAGG)
				{
					if (map->gameflags & flagh.data1) Printf("\t%s\n", flagh.name);
				}
			}
			PrintCutscene("intro", map->intro);
			PrintCutscene("outro", map->outro);
			Printf("}\n");
		}
		else
		{
			//Printf("%s - %s (defined but does not exist)\n", map->fileName.GetChars(), map->DisplayName());
		}
	}
}


//==========================================================================
//
// ParseMapDefinition
// Parses the body of a map definition, including defaultmap etc.
//
//==========================================================================

void FMapInfoParser::ParseMapDefinition(MapRecord &info)
{
	int index;

	ParseOpenBrace();

	while (sc.GetString())
	{
		if ((index = sc.MatchString(&MapFlagHandlers->name, sizeof(*MapFlagHandlers))) >= 0)
		{
			MapInfoFlagHandler *handler = &MapFlagHandlers[index];
			switch (handler->type)
			{
			case MITYPE_EATNEXT:
				ParseAssign();
				sc.MustGetString();
				break;

			case MITYPE_IGNORE:
				break;

			case MITYPE_SETFLAG:
				if (!CheckAssign())
				{
					info.flags |= handler->data1;
				}
				else
				{
					sc.MustGetNumber();
					if (sc.Number) info.flags |= handler->data1;
					else info.flags &= ~handler->data1;
				}
				info.flags |= handler->data2;
				break;

			case MITYPE_CLRFLAG:
				info.flags &= ~handler->data1;
				info.flags |= handler->data2;
				break;

			case MITYPE_SCFLAGS:
				info.flags = (info.flags & handler->data2) | handler->data1;
				break;

			case MITYPE_SETFLAGG:
				if (!CheckAssign())
				{
					info.gameflags |= handler->data1;
				}
				else
				{
					sc.MustGetNumber();
					if (sc.Number) info.gameflags |= handler->data1;
					else info.gameflags &= ~handler->data1;
				}
				info.gameflags |= handler->data2;
				break;

			case MITYPE_CLRFLAGG:
				info.gameflags &= ~handler->data1;
				info.gameflags |= handler->data2;
				break;
			case MITYPE_SCFLAGSG:
				info.gameflags = (info.gameflags & handler->data2) | handler->data1;
				break;

			default:
				// should never happen
				assert(false);
				break;
			}
		}
		else
		{
			bool success = false;

			AutoSegs::MapInfoOptions.ForEach([this, &success, &info](FMapOptInfo* option)
			{
				if (sc.Compare(option->name))
				{
					option->handler(*this, &info);
					success = true;
					return false;  // break
				}
				
				return true;  // continue
			});
			
			if (!success)
			{
				if (!ParseCloseBrace())
				{
					// Unknown
					sc.ScriptMessage("Unknown property '%s' found in map definition\n", sc.String);
					SkipToNext();
				}
				else
				{
					break;
				}
			}
		}
	}
	CheckEndOfFile("map");
}


//==========================================================================
//
// GetDefaultLevelNum
// Gets a default level num from a map name.
//
//==========================================================================

static int GetDefaultLevelNum(const char *mapname)
{
	if ((!strnicmp (mapname, "MAP", 3) || !strnicmp(mapname, "LEV", 3)) && strlen(mapname) <= 5)
	{
		int mapnum = atoi (mapname + 3);

		if (mapnum >= 1 && mapnum <= 99)
			return mapnum;
	}
	else if (mapname[0] == 'E' &&
			mapname[1] >= '0' && mapname[1] <= '9' &&
			(mapname[2] == 'M' || mapname[2] == 'L') &&
			mapname[3] >= '0' && mapname[3] <= '9')
	{
		int epinum = mapname[1] - '0';
		int mapnum = mapname[3] - '0';
		return makelevelnum(epinum, mapnum);
	}
	return 0;
}

//==========================================================================
//
// ParseMapHeader
// Parses the header of a map definition ('map mapxx mapname')
//
//==========================================================================
static MapRecord sink;

MapRecord *FMapInfoParser::ParseMapHeader(MapRecord &defaultinfo)
{
	FString mapname;
	MapRecord* map;

	if (!CheckLegacyMapDefinition(mapname))
	{
		ParseLookupName(mapname);
	}

	if (mapname.IsEmpty())
	{
		map = &sink; // parse over the entire definition but discard the result.
	}
	else
	{
		map = FindMapByName(mapname);
		if (!map)
		{
			map = AllocateMap();
			*map = defaultinfo;
			DefaultExtension(mapname, ".map");
			map->SetFileName(mapname);
		}
	}

	if (!sc.CheckString("{"))
	{
		sc.MustGetString();
		map->name = sc.String;
	}
	else
	{
		if (map != &sink && map->name.IsEmpty()) sc.ScriptError("Missing level name");
		sc.UnGet();
	}
	if (map->levelNumber <= 0) map->levelNumber = GetDefaultLevelNum(map->labelName);
	return map;
}


//==========================================================================
//
// Episode definitions start with the header "episode <start-map>"
// and then can be followed by any of the following:
//
// name "Episode name as text"
// picname "Picture to display the episode name"
// key "Shortcut key for the menu"
// noskillmenu
// remove
//
//==========================================================================

void FMapInfoParser::ParseEpisodeInfo ()
{
	unsigned int i;
	FString map;
	FString pic;
	FString name;
	bool remove = false;
	char key = 0;
	int flags = 0;

	// Get map name
	sc.MustGetString ();
	map = sc.String;

	ParseOpenBrace();

	while (sc.GetString())
	{
		if (sc.Compare ("optional"))
		{
			flags |= VF_OPTIONAL;
		}
		else if (sc.Compare("sharewarelock"))
		{
			flags |= VF_SHAREWARELOCK;
		}
		else if (sc.Compare ("name"))
		{
			ParseAssign();
			sc.MustGetString ();
			name = sc.String;
		}
		else if (sc.Compare ("remove"))
		{
			remove = true;
		}
		else if (sc.Compare ("key"))
		{
			ParseAssign();
			sc.MustGetString ();
			key = sc.String[0];
		}
		else if (sc.Compare("noskillmenu"))
		{
			flags |= VF_NOSKILL;
		}
		else if (!ParseCloseBrace())
		{
			// Unknown
			sc.ScriptMessage("Unknown property '%s' found in episode definition\n", sc.String);
			SkipToNext();
		}
		else
		{
			break;
		}
	}
	CheckEndOfFile("episode");


	for (i = 0; i < volumes.Size(); i++)
	{
		if (volumes[i].startmap.CompareNoCase(map) == 0)
		{
			break;
		}
	}

	if (remove)
	{
		// If the remove property is given for an episode, remove it.
		volumes.Delete(i);
	}
	else
	{
		// Only allocate a new entry if this doesn't replace an existing episode.
		if (i >= volumes.Size())
		{
			i = volumes.Reserve(1);
		}

		auto epi = &volumes[i];

		epi->startmap = map;
		epi->name = name;
		epi->shortcut = tolower(key);
		epi->flags = flags;
		epi->index = i;
	}
}


//==========================================================================
//
//
//
//==========================================================================

void FMapInfoParser::ParseCutsceneInfo()
{
	FString map;
	FString pic;
	FString name;
	bool remove = false;
	char key = 0;
	int flags = 0;

	ParseOpenBrace();

	while (sc.GetString())
	{
		if (sc.Compare("intro"))
		{
			ParseCutscene(globalCutscenes.Intro);
		}
		else if (sc.Compare("defaultmapintro"))
		{
			ParseCutscene(globalCutscenes.DefaultMapIntro);
		}
		else if (sc.Compare("defaultmapoutro"))
		{
			ParseCutscene(globalCutscenes.DefaultMapOutro);
		}
		else if (sc.Compare("defaultgameover"))
		{
			ParseCutscene(globalCutscenes.DefaultGameover);
		}
		else if (sc.Compare("sharewareend"))
		{
			ParseCutscene(globalCutscenes.SharewareEnd);
		}
		else if (sc.Compare("loadscreen"))
		{
			ParseCutscene(globalCutscenes.LoadingScreen);
		}
		else if (!ParseCloseBrace())
		{
			// Unknown
			sc.ScriptMessage("Unknown property '%s' found in cutscene definition\n", sc.String);
			SkipToNext();
		}
		else
		{
			break;
		}
	}
	CheckEndOfFile("cutscenes");
}


//==========================================================================
//
//
//
//==========================================================================

void FMapInfoParser::ParseGameInfo()
{
	FString map;
	FString pic;
	FString name;
	bool remove = false;
	char key = 0;
	int flags = 0;

	ParseOpenBrace();

	while (sc.GetString())
	{
		if (sc.Compare("summaryscreen"))
		{
			ParseAssign();
			sc.SetCMode(false);
			sc.MustGetString();
			sc.SetCMode(false);
			globalCutscenes.SummaryScreen = sc.String;
		}
		else if (sc.Compare("mpsummaryscreen"))
		{
			ParseAssign();
			sc.SetCMode(false);
			sc.MustGetString();
			sc.SetCMode(false);
			globalCutscenes.MPSummaryScreen = sc.String;
		}
		else if (sc.Compare("statusbarclass"))
		{
			ParseAssign();
			sc.SetCMode(false);
			sc.MustGetString();
			sc.SetCMode(false);
			globalCutscenes.StatusBarClass = sc.String;
		}
		else if (!ParseCloseBrace())
		{
			// Unknown
			sc.ScriptMessage("Unknown property '%s' found in gameinfo definition\n", sc.String);
			SkipToNext();
		}
		else
		{
			break;
		}
	}
	CheckEndOfFile("cutscenes");
}


//==========================================================================
//
// SetLevelNum
// Avoid duplicate levelnums. The level being set always has precedence.
//
//==========================================================================

void SetLevelNum (MapRecord *info, int num)
{
	for (auto& map : mapList)
	{
		
		if (map->levelNumber == num)
			map->levelNumber = 0;
	}
	info->levelNumber = num;
}

//==========================================================================
//
// G_DoParseMapInfo
// Parses a single MAPINFO lump
// data for wadlevelinfos and wadclusterinfos.
//
//==========================================================================

void FMapInfoParser::ParseMapInfo (int lump, MapRecord &gamedefaults, MapRecord &defaultinfo)
{
	sc.OpenLumpNum(lump);
	Internal = (fileSystem.GetFileContainer(lump) == 0);

	defaultinfo = gamedefaults;
	defaultinfoptr = &defaultinfo;

	if (ParsedLumps.Find(lump) != ParsedLumps.Size())
	{
		sc.ScriptMessage("MAPINFO file is processed more than once\n");
	}
	else
	{
		ParsedLumps.Push(lump);
	}

	while (sc.GetString ())
	{
		if (sc.Compare("include"))
		{
			sc.MustGetString();
			int inclump = fileSystem.CheckNumForFullName(sc.String, true);
			if (inclump < 0)
			{
				sc.ScriptError("include file '%s' not found", sc.String);
			}
			if (fileSystem.GetFileContainer(sc.LumpNum) != fileSystem.GetFileContainer(inclump))
			{
				// Do not allow overriding includes from the default MAPINFO
				if (fileSystem.GetFileContainer(sc.LumpNum) == 0)
				{
					I_FatalError("File %s is overriding core lump %s.",
						fileSystem.GetResourceFileFullName(fileSystem.GetFileContainer(inclump)), sc.String);
				}
			}
			FScanner saved_sc = sc;
			ParseMapInfo(inclump, gamedefaults, defaultinfo);
			sc = saved_sc;
		}
		else if (sc.Compare("gamedefaults"))
		{
			gamedefaults = {};
			ParseMapDefinition(gamedefaults);
			defaultinfo = gamedefaults;
		}
		else if (sc.Compare("defaultmap"))
		{
			defaultinfo = gamedefaults;
			ParseMapDefinition(defaultinfo);
		}
		else if (sc.Compare("adddefaultmap"))
		{
			// Same as above but adds to the existing definitions instead of replacing them completely
			ParseMapDefinition(defaultinfo);
		}
		else if (sc.Compare("map"))
		{
			auto levelinfo = ParseMapHeader(defaultinfo);

			ParseMapDefinition(*levelinfo);
			SetLevelNum (levelinfo, levelinfo->levelNumber);	// Wipe out matching levelnums from other maps.
		}
		// clusterdef is the old keyword but the new format has enough 
		// structuring that 'cluster' can be handled, too. The old format does not.
		else if (sc.Compare("cluster"))
		{
			ParseCluster();
		}
		else if (sc.Compare("episode"))
		{
			ParseEpisodeInfo();
		}
		else if (sc.Compare("clearepisodes"))
		{
			volumes.Clear();
		}
		else if (sc.Compare("clearall"))
		{
			// Wipe out all legacy content to start a fresh definition.
			volumes.Clear();
			mapList.Clear();
			clusters.Clear();
		}
		else if (sc.Compare("cutscenes"))
		{
			ParseCutsceneInfo();
		}
		else if (sc.Compare("gameinfo"))
		{
			ParseGameInfo();
		}
		else if (sc.Compare("clearall"))
		{
			// clears all map and progression related data, so that a mod can start with a clean slate.
			mapList.Clear();
			volumes.Clear();
			clusters.Clear();
			globalCutscenes.DefaultMapIntro = {};
			globalCutscenes.DefaultMapOutro = {};
			globalCutscenes.DefaultGameover = {};
		}
		else
		{
			sc.ScriptError("%s: Unknown top level keyword", sc.String);
		}
	}
}

//==========================================================================
//
// G_ParseMapInfo
// Parses the MAPINFO lumps of all loaded WADs and generates
// data for wadlevelinfos and wadclusterinfos.
//
//==========================================================================

void G_ParseMapInfo ()
{
	int lump, lastlump = 0;
	MapRecord gamedefaults;

	// first parse the internal one which sets up the needed basics and patches the legacy definitions of each game.
	FMapInfoParser parse;
	MapRecord defaultinfo;

	// Parse internal RMAPINFOs.
	while ((lump = fileSystem.FindLumpFullName("engine/rmapinfo.txt", &lastlump, false)) != -1)
	{
		if (fileSystem.GetFileContainer(lump) > 0) break; // only load from raze.pk3

		FMapInfoParser parse;
		MapRecord defaultinfo;
		parse.ParseMapInfo(lump, gamedefaults, defaultinfo);
	}

	// Parse any extra RMAPINFOs.
	lastlump = 0;
	while ((lump = fileSystem.FindLump ("RMAPINFO", &lastlump, false)) != -1)
	{
		FMapInfoParser parse;
		MapRecord defaultinfo;
		parse.ParseMapInfo(lump, gamedefaults, defaultinfo);
	}

	if (volumes.Size() == 0)
	{
		I_FatalError ("No volumes defined.");
	}
}
