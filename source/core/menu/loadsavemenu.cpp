/*
** loadsavemenu.cpp
** The load game and save game menus
**
**---------------------------------------------------------------------------
** Copyright 2001-2010 Randy Heit
** Copyright 2010-2020 Christoph Oelckers
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

#include "razemenu.h"
#include "version.h"
#include "m_png.h"
#include "filesystem.h"
#include "v_text.h"
#include "gstrings.h"
#include "serializer.h"
#include "vm.h"
#include "i_system.h"
#include "v_video.h"
#include "findfile.h"
#include "v_draw.h"
#include "fs_findfile.h"
#include "savegamehelp.h"

using namespace FileSys;
//=============================================================================
//
// M_ReadSaveStrings
//
// Find savegames and read their titles
//
//=============================================================================

void FSavegameManager::ReadSaveStrings()
{
	if (SaveGames.Size() == 0)
	{
		LastSaved = LastAccessed = -1;
		quickSaveSlot = nullptr;
		FileList list;
		if (ScanDirectory(list, G_GetSavegamesFolder().GetChars(), "*." SAVEGAME_EXT, true))
		{
			for (auto& entry : list)
			{
				FResourceFile *savegame = FResourceFile::OpenResourceFile(entry.FilePath.c_str(), true);
				if (savegame != nullptr)
				{
					auto info = savegame->FindEntry("info.json");
					if (info < 0)
					{
						// savegame info not found. This is not a savegame so leave it alone.
						delete savegame;
						continue;
					}
					auto fr = savegame->GetEntryReader(info, true);
					FString title;
					int check = G_ValidateSavegame(fr, &title, true);
					fr.Close();
					delete savegame;
					if (check != 0)
					{
						FSaveGameNode *node = new FSaveGameNode;
						node->Filename = entry.FilePath.c_str();
						node->bOldVersion = check == -1;
						node->bMissingWads = check == -2;
						node->SaveTitle = title;
						InsertSaveNode(node);
					}
				}
			} 
		}
	}
}


//=============================================================================
//
// 
//
//=============================================================================

void FSavegameManager::PerformLoadGame(const char *f, bool s)
{
	G_LoadGame(f, s);
}

void FSavegameManager::PerformSaveGame(const char *f, const char *s)
{
	G_SaveGame(f, s);
}

FString FSavegameManager::BuildSaveName(const char* fn, int slot)
{
	return G_BuildSaveName(FStringf("%s%04d", fn, slot).GetChars());
}

//=============================================================================
//
//
//
//=============================================================================

FString FSavegameManager::ExtractSaveComment(FSerializer& arc)
{
	FString comment, fcomment, ncomment, mtime;

	arc("Creation Time", comment)
		("Map Label", fcomment)
		("Map Name", ncomment)
		("Map Time", mtime);

	comment.AppendFormat("\n%s - %s\n%s", fcomment.GetChars(), ncomment.GetChars(), mtime.GetChars());
	return comment;
}

FSavegameManager savegameManager;

DEFINE_ACTION_FUNCTION(FSavegameManager, GetManager)
{
	PARAM_PROLOGUE;
	ACTION_RETURN_POINTER(&savegameManager);
}

