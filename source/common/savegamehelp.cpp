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
#include "sjson.h"
#include "baselayer.h"
#include "gstrings.h"
#include "i_specialpaths.h"
#include "cmdlib.h"
#include "filesystem/filesystem.h"
#include "statistics.h"
#include "secrets.h"
#include "s_music.h"

static CompositeSavegameWriter savewriter;
static FResourceFile *savereader;

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


void OpenSaveGameForWrite(const char *name)
{
	savewriter.Clear();
	savewriter.SetFileName(name);
}

bool OpenSaveGameForRead(const char *name)
{
	if (savereader) delete savereader;
	savereader = FResourceFile::OpenResourceFile(name, true, true);

	if (savereader != nullptr)
	{
		// Load system-side data from savegames.
		ReadStatistics();
		SECRET_Load();
		MUS_Restore();
	}

	return savereader != nullptr;
}

FileWriter *WriteSavegameChunk(const char *name)
{
	return &savewriter.NewElement(name);
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

//=============================================================================
//
// Writes the header which is used to display the savegame in the menu.
//
//=============================================================================

void G_WriteSaveHeader(const char *name, const char*mapname, const char *maptitle)
{
	sjson_context* ctx = sjson_create_context(0, 0, NULL);
	if (!ctx)
	{
		return;
	}
	sjson_node* root = sjson_mkobject(ctx);
	auto savesig = gi->GetSaveSig();
	sjson_put_int(ctx, root, "Save Version", savesig.currentsavever);
	sjson_put_string(ctx, root, "Engine", savesig.savesig);
	sjson_put_string(ctx, root, "Game Resource", fileSystem.GetResourceFileName(1));
	sjson_put_string(ctx, root, "map", mapname);
	sjson_put_string(ctx, root, "Title", maptitle);
	if (*mapname == '/') mapname++;
	sjson_put_string(ctx, root, "Map Resource", mapname);

	char* encoded = sjson_stringify(ctx, root, "  ");

	FileWriter* fil = WriteSavegameChunk("info.json");
	if (!fil)
	{
		sjson_destroy_context(ctx);
		return;
	}

	fil->Write(encoded, strlen(encoded));

	sjson_free_string(ctx, encoded);
	sjson_destroy_context(ctx);

	// Handle system-side modules that need to persist data in savegames here, in a central place.
	SaveStatistics();
	SECRET_Save();
	MUS_Save();
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

bool G_CheckSaveGameWads (sjson_node* root, bool printwarn)
{
	bool printRequires = false;
	auto text = sjson_get_string(root, "Game Resource", "");
	CheckSingleFile (text, printRequires, printwarn);
	text = sjson_get_string(root, "MAP Resource", "");
	CheckSingleFile (text, printRequires, printwarn);

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

int G_ValidateSavegame(FileReader &fr, FString *savetitle)
{
	auto data = fr.ReadPadded(1);
	
	sjson_context* ctx = sjson_create_context(0, 0, NULL);
	if (ctx)
	{
		sjson_node* root = sjson_decode(ctx, (const char*)data.Data());
		
		
		int savever = sjson_get_int(root, "Save Version", -1);
		FString engine = sjson_get_string(root, "Engine", "");
		FString gamegrp = sjson_get_string(root, "Game Resource", "");
		FString title = sjson_get_string(root, "Title", "");
		auto savesig = gi->GetSaveSig();
		
		sjson_destroy_context(ctx);
		
		if (savetitle) *savetitle = title;
		if (engine.Compare(savesig.savesig) != 0 || savever > savesig.currentsavever)
		{
			// different engine or newer version:
			// not our business. Leave it alone.
			return 0;
		}
		
		if (savever < savesig.minsavever)
		{
			// old, incompatible savegame. List as not usable.
			return -1;
		}
		else if (gamegrp.CompareNoCase(fileSystem.GetResourceFileName(1)) == 0)
		{
			return G_CheckSaveGameWads(root, false)? 0 : -2;
		}
		else
		{
			// different game. Skip this.
			return 0;
		}
	}
	return 1;
}

//=============================================================================
//
//
//
//=============================================================================

FString G_BuildSaveName (const char *prefix)
{
	FString name = M_GetSavegamesPath();
	size_t len = name.Len();
	if (name[0] != '\0' && name[len-1] != '\\' && name[len-1] != '/')
	{
		name << "/";
	}
	name << prefix;
	if (!strchr(prefix, '.')) name << SAVEGAME_EXT; // only add an extension if the prefix doesn't have one already.
	name = NicePath(name);
	name.Substitute("\\", "/");
	CreatePath(name);
	return name;
}

