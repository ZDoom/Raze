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
#include "baselayer.h"
#include "gstrings.h"
#include "i_specialpaths.h"
#include "cmdlib.h"
#include "filesystem/filesystem.h"
#include "statistics.h"
#include "secrets.h"
#include "quotemgr.h"
#include "mapinfo.h"
#include "v_video.h"
#include "gamecontrol.h"
#include "m_argv.h"
#include "serializer.h"
#include "version.h"
#include "z_music.h"
#include "s_soundinternal.h"

static CompositeSavegameWriter savewriter;
static FResourceFile *savereader;
void LoadEngineState();
void SaveEngineState();

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
		FResourceLump* info = savereader->FindLump("session.json");
		if (info == nullptr)
		{
			return false;
		}

		FSerializer arc;
		void* data = info->Get();
		if (!arc.OpenReader((const char*)data, info->LumpSize))
		{
			return false;
		}

		// Load system-side data from savegames.
		SerializeSession(arc);
		LoadEngineState();

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

CVAR(Bool, save_formatted, true, 0)	// should be set to false once the conversion is done

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

	savegameinfo.AddString("Software", buf)
		("Save Version", savesig.currentsavever)
		.AddString("Engine", savesig.savesig)
		.AddString("Game Resource", fileSystem.GetResourceFileName(1))
		.AddString("Map Name", currentLevel->DisplayName())
		.AddString("Creation Time", myasctime())
		.AddString("Title", name)
		.AddString("Map File", currentLevel->fileName)
		.AddString("Map Label", currentLevel->labelName)
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
	buff = savegamesession.GetCompressedOutput();
	AddCompressedSavegameChunk("session.json", buff);

	SaveEngineState();
	auto picfile = WriteSavegameChunk("savepic.png");
	screen->WriteSavePic(picfile, 240, 180);
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
	FString engine, gamegrp, mapgrp, title, filename;

	arc("Save Version", savever)
		("Engine", engine)
		("Game Resource", gamegrp)
		("Map Resource", mapgrp)
		("Title", title)
		("Map File", filename);

	auto savesig = gi->GetSaveSig();

	if (savetitle) *savetitle = title;
	if (engine.Compare(savesig.savesig) != 0 || savever > savesig.currentsavever)
	{
		// different engine or newer version:
		// not our business. Leave it alone.
		return 0;
	}

	MapRecord *curLevel = nullptr;

	if (strncmp(filename, "file://", 7) != 0)
	{
		for (auto& mr : mapList)
		{
			if (mr.fileName.Compare(filename) == 0)
			{
				curLevel = &mr;
			}
		}
	}
	else
	{
		curLevel = &userMapRecord;
		if (!formenu)
		{
			userMapRecord.name = "";
			userMapRecord.SetFileName(filename);
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

	fw->Write(&tailspritefree, sizeof(tailspritefree));
	fw->Write(&myconnectindex, sizeof(myconnectindex));
	fw->Write(&connecthead, sizeof(connecthead));
	fw->Write(connectpoint2, sizeof(connectpoint2));
	fw->Write(&numframes, sizeof(numframes));
	fw->Write(&randomseed, sizeof(randomseed));
	fw->Write(&numshades, sizeof(numshades));
	fw->Write(&automapping, sizeof(automapping));
	fw->Write(&showinvisibility, sizeof(showinvisibility));
	WriteMagic(fw);

	fw->Write(&g_visibility, sizeof(g_visibility));
	fw->Write(&parallaxtype, sizeof(parallaxtype));
	fw->Write(&parallaxvisibility, sizeof(parallaxvisibility));
	fw->Write(&parallaxyoffs_override, sizeof(parallaxyoffs_override));
	fw->Write(&parallaxyscale_override, sizeof(parallaxyscale_override));
	fw->Write(&pskybits_override, sizeof(pskybits_override));
	WriteMagic(fw);

	fw->Write(show2dwall, sizeof(show2dwall));
	fw->Write(show2dsprite, sizeof(show2dsprite));
	fw->Write(show2dsector, sizeof(show2dsector));
	WriteMagic(fw);

	fw->Write(&numyaxbunches, sizeof(numyaxbunches));
	fw->Write(yax_bunchnum, sizeof(yax_bunchnum));
	fw->Write(yax_nextwall, sizeof(yax_nextwall));
	WriteMagic(fw);

	fw->Write(&Numsprites, sizeof(Numsprites));
	sv_prespriteextsave();
	fw->Write(spriteext, sizeof(spriteext_t) * MAXSPRITES);
	fw->Write(wallext, sizeof(wallext_t) * MAXWALLS);
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

		fr.Read(&tailspritefree, sizeof(tailspritefree));
		fr.Read(&myconnectindex, sizeof(myconnectindex));
		fr.Read(&connecthead, sizeof(connecthead));
		fr.Read(connectpoint2, sizeof(connectpoint2));
		fr.Read(&numframes, sizeof(numframes));
		fr.Read(&randomseed, sizeof(randomseed));
		fr.Read(&numshades, sizeof(numshades));
		fr.Read(&automapping, sizeof(automapping));
		fr.Read(&showinvisibility, sizeof(showinvisibility));
		CheckMagic(fr);

		fr.Read(&g_visibility, sizeof(g_visibility));
		fr.Read(&parallaxtype, sizeof(parallaxtype));
		fr.Read(&parallaxvisibility, sizeof(parallaxvisibility));
		fr.Read(&parallaxyoffs_override, sizeof(parallaxyoffs_override));
		fr.Read(&parallaxyscale_override, sizeof(parallaxyscale_override));
		fr.Read(&pskybits_override, sizeof(pskybits_override));
		CheckMagic(fr);

		fr.Read(show2dwall, sizeof(show2dwall));
		fr.Read(show2dsprite, sizeof(show2dsprite));
		fr.Read(show2dsector, sizeof(show2dsector));
		CheckMagic(fr);

		fr.Read(&numyaxbunches, sizeof(numyaxbunches));
		fr.Read(yax_bunchnum, sizeof(yax_bunchnum));
		fr.Read(yax_nextwall, sizeof(yax_nextwall));
		yax_update(numyaxbunches > 0 ? 2 : 1);
		CheckMagic(fr);

		fr.Read(&Numsprites, sizeof(Numsprites));
		fr.Read(spriteext, sizeof(spriteext_t) * MAXSPRITES);
		fr.Read(wallext, sizeof(wallext_t) * MAXWALLS);
		sv_postspriteext();
	CheckMagic(fr);

		fr.Close();
	}
}
