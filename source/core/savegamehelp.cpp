/*
** savegame.cpp
**
** common savegame utilities for all front ends.
** 
**---------------------------------------------------------------------------
** Copyright 2019 Christoph Oelckers
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
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OFf
** THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
**---------------------------------------------------------------------------
**
*/

#include "compositesaveame.h"
#include "savegamehelp.h"
#include "gstrings.h"
#include "i_specialpaths.h"
#include "cmdlib.h"
#include "filesystem.h"
#include "statistics.h"
#include "secrets.h"
#include "quotemgr.h"
#include "mapinfo.h"
#include "v_video.h"
#include "gamecontrol.h"
#include "m_argv.h"
#include "serializer.h"
#include "version.h"
#include "raze_music.h"
#include "raze_sound.h"
#include "gamestruct.h"
#include "automap.h"
#include "statusbar.h"
#include "gamestate.h"
#include "razemenu.h"

static CompositeSavegameWriter savewriter;
static FResourceFile *savereader;
void LoadEngineState();
void SaveEngineState();
void WriteSavePic(FileWriter* file, int width, int height);

CVAR(String, cl_savedir, "", CVAR_ARCHIVE|CVAR_GLOBALCONFIG)

//=============================================================================
//
//
//
//=============================================================================

static void SerializeSession(FSerializer& arc)
{
	SerializeStatistics(arc);
	SECRET_Serialize(arc);
	Mus_Serialize(arc);
	quoteMgr.Serialize(arc);
	S_SerializeSounds(arc);
	SerializeAutomap(arc);
	SerializeHud(arc);
}

//=============================================================================
//
// This is for keeping my sanity while working with the horrible mess
// that is the savegame code in Duke Nukem.
// Without handling this in global variables it is a losing proposition
// to save custom data along with the regular snapshot. :(
// With this the savegame code can mostly pretend to load from and write
// to files while really using a composite archive.
//
// All global non-game dependent state is also saved right here for convenience.
//
//=============================================================================


bool OpenSaveGameForRead(const char *name)
{
	if (savereader) delete savereader;
	savereader = FResourceFile::OpenResourceFile(name, true, true);

	if (savereader != nullptr)
	{
		auto file = ReadSavegameChunk("info.json");
		if (!file.isOpen())
		{
			FinishSavegameRead();
			delete savereader;
			return false;
		}
		if (G_ValidateSavegame(file, nullptr, false) <= 0)
		{
			FinishSavegameRead();
			delete savereader;
			return false;
		}

		FResourceLump* info = savereader->FindLump("session.json");
		if (info == nullptr)
		{
			return false;
		}

		void* data = info->Lock();
		FSerializer arc;
		if (!arc.OpenReader((const char*)data, info->LumpSize))
		{
			info->Unlock();
			return false;
		}
		info->Unlock();

		// Load system-side data from savegames.
		LoadEngineState();
		SerializeSession(arc); // must be AFTER LoadEngineState because it needs info from it.
		gi->SerializeGameState(arc);
	}
	return savereader != nullptr;
}

FileWriter *WriteSavegameChunk(const char *name)
{
	return &savewriter.NewElement(name);
}

void AddCompressedSavegameChunk(const char* name, FCompressedBuffer& buffer)
{
	savewriter.AddCompressedElement(name, buffer);
}

FileReader ReadSavegameChunk(const char *name)
{
	if (!savereader) return FileReader();
	auto lump = savereader->FindLump(name);
	if (!lump) return FileReader();
	return lump->NewReader();
}

bool FinishSavegameWrite()
{
	return savewriter.WriteToFile();
}

void FinishSavegameRead()
{
	delete savereader;
	savereader = nullptr;
}

CVAR(Bool, save_formatted, false, 0)	// should be set to false once the conversion is done

//=============================================================================
//
// Creates the savegame and writes all cross-game content.
//
//=============================================================================

bool OpenSaveGameForWrite(const char* filename, const char *name)
{
	savewriter.Clear();
	savewriter.SetFileName(filename);

	FSerializer savegameinfo;		// this is for displayable info about the savegame.
	FSerializer savegamesession;	// saved game session settings.
	FSerializer savegameengine;		// saved play state.

	savegameinfo.OpenWriter(true);
	savegameengine.OpenWriter(save_formatted);

	char buf[100];
	mysnprintf(buf, countof(buf), GAMENAME " %s", GetVersionString());
	auto savesig = gi->GetSaveSig();
	auto gs = gi->getStats();
	FStringf timeStr("%02d:%02d", gs.timesecnd / 60, gs.timesecnd % 60);
	auto lev = currentLevel;

	savegameinfo.AddString("Software", buf)
		("Save Version", savesig.currentsavever)
		.AddString("Engine", savesig.savesig)
		.AddString("Game Resource", fileSystem.GetResourceFileName(1))
		.AddString("Map Name", lev->DisplayName())
		.AddString("Creation Time", myasctime())
		.AddString("Title", name)
		.AddString("Map File", lev->fileName)
		.AddString("Map Label", lev->labelName)
		.AddString("Map Time", timeStr);

	const char *fn = currentLevel->fileName;
	if (*fn == '/') fn++;
	if (strncmp(fn, "file://", 7) != 0) // this only has meaning for non-usermaps
	{
		auto fileno = fileSystem.FindFile(fn);
		auto mapfile = fileSystem.GetFileContainer(fileno);
		auto mapcname = fileSystem.GetResourceFileName(mapfile);
		if (mapcname) savegameinfo.AddString("Map Resource", mapcname);
		else
		{
			savewriter.Clear();
			return false; // this should never happen. Saving on a map that isn't present is impossible.
		}
	}

	auto buff = savegameinfo.GetCompressedOutput();
	AddCompressedSavegameChunk("info.json", buff);


	// Handle system-side modules that need to persist data in savegames here, in a central place.
	savegamesession.OpenWriter(save_formatted);
	SerializeSession(savegamesession);
	SaveEngineState();
	gi->SerializeGameState(savegamesession);
	buff = savegamesession.GetCompressedOutput();
	AddCompressedSavegameChunk("session.json", buff);

	auto picfile = WriteSavegameChunk("savepic.png");
	WriteSavePic(picfile, 240, 180);
	mysnprintf(buf, countof(buf), GAMENAME " %s", GetVersionString());
	// put some basic info into the PNG so that this isn't lost when the image gets extracted.
	M_AppendPNGText(picfile, "Software", buf);
	M_AppendPNGText(picfile, "Title", name);
	M_AppendPNGText(picfile, "Current Map", lev->labelName);
	M_FinishPNG(picfile);

	return true;
}

//=============================================================================
//
//
//
//=============================================================================

static bool CheckSingleFile (const char *name, bool &printRequires, bool printwarn)
{
	if (name == NULL)
	{
		return true;
	}
	if (strncmp(name, "file://", 7) == 0)
	{
		return FileExists(name + 7);	// User maps  must be present to be validated.
	}
	if (fileSystem.CheckIfResourceFileLoaded(name) < 0)
	{
		if (printwarn)
		{
			if (!printRequires)
			{
				Printf ("%s:\n%s", GStrings("TXT_SAVEGAMENEEDS"), name);
			}
			else
			{
				Printf (", %s", name);
			}
		}
		printRequires = true;
		return false;
	}
	return true;
}

//=============================================================================
//
// Return false if not all the needed wads have been loaded.
//
//=============================================================================

static bool G_CheckSaveGameWads (const char *gamegrp, const char *mapgrp, bool printwarn)
{
	bool printRequires = false;
	CheckSingleFile (gamegrp, printRequires, printwarn);
	CheckSingleFile (mapgrp, printRequires, printwarn);

	if (printRequires)
	{
		if (printwarn)
		{
			Printf ("\n");
		}
		return false;
	}

	return true;
}

//=============================================================================
//
// Checks if the savegame is valid. Gets a reader to the included info.json
// Returns 1 if valid, 0 if invalid and -1 if old and -2 if content missing
//
//=============================================================================

int G_ValidateSavegame(FileReader &fr, FString *savetitle, bool formenu)
{
	auto data = fr.Read();
	FSerializer arc;
	if (!arc.OpenReader((const char*)data.Data(), data.Size()))
	{
		return -2;
	}

	int savever;
	FString engine, gamegrp, mapgrp, title, filename, label;

	arc("Save Version", savever)
		("Engine", engine)
		("Game Resource", gamegrp)
		("Map Resource", mapgrp)
		("Title", title)
		("Map Label", label)
		("Map File", filename);

	auto savesig = gi->GetSaveSig();

	if (savetitle) *savetitle = title;
	if (engine.Compare(savesig.savesig) != 0 || savever > savesig.currentsavever)
	{
		// different engine or newer version:
		// not our business. Leave it alone.
		return 0;
	}

	MapRecord *curLevel = FindMapByName(label);

	// If the map does not exist, check if it's a user map.
	if (!curLevel)
	{
		curLevel = AllocateMap();
		if (!formenu)
		{
			curLevel->name = "";
			curLevel->SetFileName(filename);
		}
	}
	if (!curLevel) return 0;
	if (!formenu) currentLevel = curLevel;
		

	if (savever < savesig.minsavever)
	{
		// old, incompatible savegame. List as not usable.
		return -1;
	}
	else
	{
		auto ggfn = ExtractFileBase(fileSystem.GetResourceFileName(1), true);
		if (gamegrp.CompareNoCase(ggfn) == 0)
		{
			return G_CheckSaveGameWads(gamegrp, mapgrp, false) ? 1 : -2;
		}
		else
		{
			// different game. Skip this.
			return 0;
		}
	}
	return 0;
}

//=============================================================================
//
//
//
//=============================================================================

FString G_BuildSaveName (const char *prefix)
{
	FString name;
	bool usefilter;

	if (const char *const dir = Args->CheckValue("-savedir"))
	{
		name = dir;
		usefilter = false;
	}
	else
	{
		name = **cl_savedir ? cl_savedir : M_GetSavegamesPath();
		usefilter = true;
	}

	const size_t len = name.Len();
	if (len > 0)
	{
		name.Substitute("\\", "/");
		if (name[len - 1] != '/')
			name << '/';
	}

	if (usefilter)
		name << LumpFilter << '/';

	CreatePath(name);

	name << prefix;
	if (!strchr(prefix, '.')) name << SAVEGAME_EXT; // only add an extension if the prefix doesn't have one already.
	name = NicePath(name);
	name.Substitute("\\", "/");
	return name;
}

#include "build.h"
#include "mmulti.h"

static void sv_prespriteextsave()
{
	for (int i = 0; i < MAXSPRITES; i++)
		if (spriteext[i].mdanimtims)
		{
			spriteext[i].mdanimtims -= mdtims;
			if (spriteext[i].mdanimtims == 0)
				spriteext[i].mdanimtims++;
		}
}
static void sv_postspriteext()
{
	for (int i = 0; i < MAXSPRITES; i++)
		if (spriteext[i].mdanimtims)
			spriteext[i].mdanimtims += mdtims;
}


static const int magic = 0xbeefcafe;
void WriteMagic(FileWriter *fw)
{
	fw->Write(&magic, 4);
}

void CheckMagic(FileReader& fr)
{
	int m = 0;
	fr.Read(&m, 4);
	assert(m == magic);
#ifndef _DEBUG
	if (m != magic)  I_Error("Savegame corrupt");
#endif
}

void SaveEngineState()
{
	auto fw = WriteSavegameChunk("engine.bin");
	fw->Write(&numsectors, sizeof(numsectors));
	fw->Write(sector, sizeof(sectortype) * numsectors);
	WriteMagic(fw);
	fw->Write(&numwalls, sizeof(numwalls));
	fw->Write(wall, sizeof(walltype) * numwalls);
	WriteMagic(fw);
	fw->Write(sprite, sizeof(spritetype) * MAXSPRITES);
	WriteMagic(fw);
	fw->Write(headspritesect, sizeof(headspritesect));
	fw->Write(prevspritesect, sizeof(prevspritesect));
	fw->Write(nextspritesect, sizeof(nextspritesect));
	fw->Write(headspritestat, sizeof(headspritestat));
	fw->Write(prevspritestat, sizeof(prevspritestat));
	fw->Write(nextspritestat, sizeof(nextspritestat));
	WriteMagic(fw);
	for (int i = 0; i < MAXTILES; i++)
	{
		fw->Write(&picanm[i], sizeof(picanm[i]));
	}
	WriteMagic(fw);


	fw->Write(&tailspritefree, sizeof(tailspritefree));
	fw->Write(&myconnectindex, sizeof(myconnectindex));
	fw->Write(&connecthead, sizeof(connecthead));
	fw->Write(connectpoint2, sizeof(connectpoint2));
	fw->Write(&randomseed, sizeof(randomseed));
	fw->Write(&numshades, sizeof(numshades));
	fw->Write(&showinvisibility, sizeof(showinvisibility));
	WriteMagic(fw);

	fw->Write(&g_visibility, sizeof(g_visibility));
	fw->Write(&parallaxtype, sizeof(parallaxtype));
	fw->Write(&parallaxvisibility, sizeof(parallaxvisibility));
	fw->Write(&parallaxyoffs_override, sizeof(parallaxyoffs_override));
	fw->Write(&parallaxyscale_override, sizeof(parallaxyscale_override));
	fw->Write(&pskybits_override, sizeof(pskybits_override));
	WriteMagic(fw);

	fw->Write(&Numsprites, sizeof(Numsprites));
	sv_prespriteextsave();
	fw->Write(spriteext, sizeof(spriteext_t) * MAXSPRITES);
	fw->Write(wallext, sizeof(wallext_t) * MAXWALLS);
	fw->Write(&randomseed, sizeof(randomseed));
	sv_postspriteext();
	WriteMagic(fw);

}

void LoadEngineState()
{
	auto fr = ReadSavegameChunk("engine.bin");
	if (fr.isOpen())
	{
		memset(sector, 0, sizeof(sector[0]) * MAXSECTORS);
		memset(wall, 0, sizeof(wall[0]) * MAXWALLS);
		memset(sprite, 0, sizeof(sprite[0]) * MAXSPRITES);

		fr.Read(&numsectors, sizeof(numsectors));
		fr.Read(sector, sizeof(sectortype) * numsectors);
		CheckMagic(fr);
		fr.Read(&numwalls, sizeof(numwalls));
		fr.Read(wall, sizeof(walltype) * numwalls);
		CheckMagic(fr);
		fr.Read(sprite, sizeof(spritetype) * MAXSPRITES);
		CheckMagic(fr);
		fr.Read(headspritesect, sizeof(headspritesect));
		fr.Read(prevspritesect, sizeof(prevspritesect));
		fr.Read(nextspritesect, sizeof(nextspritesect));
		fr.Read(headspritestat, sizeof(headspritestat));
		fr.Read(prevspritestat, sizeof(prevspritestat));
		fr.Read(nextspritestat, sizeof(nextspritestat));
		CheckMagic(fr);
		for (int i = 0; i < MAXTILES; i++)
		{
			fr.Read(&picanm[i], sizeof(picanm[i]));
		}
		CheckMagic(fr);

		fr.Read(&tailspritefree, sizeof(tailspritefree));
		fr.Read(&myconnectindex, sizeof(myconnectindex));
		fr.Read(&connecthead, sizeof(connecthead));
		fr.Read(connectpoint2, sizeof(connectpoint2));
		fr.Read(&randomseed, sizeof(randomseed));
		fr.Read(&numshades, sizeof(numshades));
		fr.Read(&showinvisibility, sizeof(showinvisibility));
		CheckMagic(fr);

		fr.Read(&g_visibility, sizeof(g_visibility));
		fr.Read(&parallaxtype, sizeof(parallaxtype));
		fr.Read(&parallaxvisibility, sizeof(parallaxvisibility));
		fr.Read(&parallaxyoffs_override, sizeof(parallaxyoffs_override));
		fr.Read(&parallaxyscale_override, sizeof(parallaxyscale_override));
		fr.Read(&pskybits_override, sizeof(pskybits_override));
		CheckMagic(fr);

		fr.Read(&Numsprites, sizeof(Numsprites));
		fr.Read(spriteext, sizeof(spriteext_t) * MAXSPRITES);
		fr.Read(wallext, sizeof(wallext_t) * MAXWALLS);
		fr.Read(&randomseed, sizeof(randomseed));
		sv_postspriteext();
	CheckMagic(fr);

		fr.Close();
	}
}

//=============================================================================
//
//
//
//=============================================================================

CVAR(Bool, saveloadconfirmation, true, CVAR_ARCHIVE | CVAR_GLOBALCONFIG)

CVAR(Int, autosavenum, 0, CVAR_NOSET | CVAR_ARCHIVE | CVAR_GLOBALCONFIG)
static int nextautosave = -1;
CVAR(Int, disableautosave, 0, CVAR_ARCHIVE | CVAR_GLOBALCONFIG)
CUSTOM_CVAR(Int, autosavecount, 4, CVAR_ARCHIVE | CVAR_GLOBALCONFIG)
{
	if (self < 1)
		self = 1;
}

CVAR(Int, quicksavenum, 0, CVAR_NOSET | CVAR_ARCHIVE | CVAR_GLOBALCONFIG)
static int nextquicksave = -1;
 CUSTOM_CVAR(Int, quicksavecount, 4, CVAR_ARCHIVE | CVAR_GLOBALCONFIG)
{
	if (self < 1)
		self = 1;
}

void M_Autosave()
{
	if (disableautosave) return;
	if (!gi->CanSave()) return;
	FString description;
	FString file;
	// Keep a rotating sets of autosaves
	UCVarValue num;
	const char* readableTime;
	int count = autosavecount != 0 ? autosavecount : 1;

	if (nextautosave == -1)
	{
		nextautosave = (autosavenum + 1) % count;
	}

	num.Int = nextautosave;
	autosavenum.ForceSet(num, CVAR_Int);

	FSaveGameNode sg;
	sg.Filename = G_BuildSaveName(FStringf("auto%04d", nextautosave));
	readableTime = myasctime();
	sg.SaveTitle.Format("Autosave %s", readableTime);
	nextautosave = (nextautosave + 1) % count;
	//savegameManager.SaveGame(&sg, false, false);
}

CCMD(autosave)
{
	gameaction = ga_autosave;
}

CCMD(rotatingquicksave)
{
	if (!gi->CanSave()) return;
	FString description;
	FString file;
	// Keep a rotating sets of quicksaves
	UCVarValue num;
	const char* readableTime;
	int count = quicksavecount != 0 ? quicksavecount : 1;

	if (nextquicksave == -1)
	{
		nextquicksave = (quicksavenum + 1) % count;
	}

	num.Int = nextquicksave;
	quicksavenum.ForceSet(num, CVAR_Int);

	FSaveGameNode sg;
	sg.Filename = G_BuildSaveName(FStringf("quick%04d", nextquicksave));
	readableTime = myasctime();
	sg.SaveTitle.Format("Quicksave %s", readableTime);
	nextquicksave = (nextquicksave + 1) % count;
	//savegameManager.SaveGame(&sg, false, false);
}


