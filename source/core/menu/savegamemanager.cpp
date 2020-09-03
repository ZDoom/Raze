/*
** loadsavemenu.cpp
** The load game and save game menus
**
**---------------------------------------------------------------------------
** Copyright 2001-2010 Randy Heit
** Copyright 2010 Christoph Oelckers
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

#include "menu.h"
#include "version.h"
#include "m_png.h"
#include "filesystem.h"
#include "v_text.h"
#include "d_event.h"
#include "gstrings.h"
#include "d_gui.h"
#include "v_draw.h"
#include "files.h"
#include "resourcefile.h"
#include "cmdlib.h"
#include "files.h"
#include "savegamehelp.h"
#include "i_specialpaths.h"
#include "c_dispatch.h"
#include "build.h"
#include "serializer.h"
#include "findfile.h"
#include "inputstate.h"
#include "gamestate.h"


FSavegameManager savegameManager;

void FSavegameManager::LoadGame(FSaveGameNode* node)
{
	inputState.ClearAllInput();
	gi->FreeLevelData();
	if (OpenSaveGameForRead(node->Filename))
	{
		if (gi->LoadGame(node))
		{
			// do something here?
		}
	}
}

void FSavegameManager::SaveGame(FSaveGameNode* node, bool ok4q, bool forceq)
{
	if (OpenSaveGameForWrite(node->Filename, node->SaveTitle))
	{
		if (gi->SaveGame(node) && FinishSavegameWrite())
		{
			FString fn = node->Filename;
			FString desc = node->SaveTitle;
			NotifyNewSave(fn, desc, ok4q, forceq);
			Printf(PRINT_NOTIFY, "%s\n", GStrings("GAME SAVED"));
		}
	}
}

//=============================================================================
//
// Save data maintenance 
//
//=============================================================================

void FSavegameManager::ClearSaveGames()
{
	for (unsigned i = 0; i<SaveGames.Size(); i++)
	{
		if (!SaveGames[i]->bNoDelete)
			delete SaveGames[i];
	}
	SaveGames.Clear();
}

FSavegameManager::~FSavegameManager()
{
	ClearSaveGames();
}

//=============================================================================
//
// Save data maintenance 
//
//=============================================================================

int FSavegameManager::RemoveSaveSlot(int index)
{
	int listindex = SaveGames[0]->bNoDelete ? index - 1 : index;
	if (listindex < 0) return index;

	remove(SaveGames[index]->Filename.GetChars());
	UnloadSaveData();

	FSaveGameNode *file = SaveGames[index];

	if (quickSaveSlot == SaveGames[index])
	{
		quickSaveSlot = nullptr;
	}
	if (!file->bNoDelete) delete file;

	if (LastSaved == listindex) LastSaved = -1;
	else if (LastSaved > listindex) LastSaved--;
	if (LastAccessed == listindex) LastAccessed = -1;
	else if (LastAccessed > listindex) LastAccessed--;

	SaveGames.Delete(index);
	if ((unsigned)index >= SaveGames.Size()) index--;
	ExtractSaveData(index);
	return index;
}

//=============================================================================
//
//
//
//=============================================================================

int FSavegameManager::InsertSaveNode(FSaveGameNode *node)
{
	if (SaveGames.Size() == 0)
	{
		return SaveGames.Push(node);
	}

	if (node->bOldVersion)
	{ // Add node at bottom of list
		return SaveGames.Push(node);
	}
	else
	{	// Add node at top of list
		unsigned int i = 0;
		if (SaveGames[0] == &NewSaveNode) i++; // To not insert above the "new savegame" dummy entry.
		for (; i < SaveGames.Size(); i++)
		{
			if (SaveGames[i]->bOldVersion || node->SaveTitle.CompareNoCase(SaveGames[i]->SaveTitle) <= 0)
			{
				break;
			}
		}
		SaveGames.Insert(i, node);
		return i;
	}
}

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
		void *filefirst;
		findstate_t c_file;
		FString filter;

		LastSaved = LastAccessed = -1;
		quickSaveSlot = nullptr;
		filter = G_BuildSaveName("*");
		filefirst = I_FindFirst(filter.GetChars(), &c_file);
		if (filefirst != ((void *)(-1)))
		{
			do
			{
				// I_FindName only returns the file's name and not its full path
				FString filepath = G_BuildSaveName(I_FindName(&c_file));

				FResourceFile *savegame = FResourceFile::OpenResourceFile(filepath, true, true);
				if (savegame != nullptr)
				{
					FResourceLump *info = savegame->FindLump("info.json");
					if (info == nullptr)
					{
						// savegame info not found. This is not a savegame so leave it alone.
						delete savegame;
						continue;
					}
					auto fr = info->NewReader();
					FString title;
					int check = G_ValidateSavegame(fr, &title, true);
					fr.Close();
					delete savegame;
					if (check != 0)
					{
						FSaveGameNode *node = new FSaveGameNode;
						node->Filename = filepath;
						node->bOldVersion = check == -1;
						node->bMissingWads = check == -2;
						node->SaveTitle = title;
						InsertSaveNode(node);
					}
				}
			} while (I_FindNext (filefirst, &c_file) == 0);
			I_FindClose (filefirst);
		}
	}
}


//=============================================================================
//
//
//
//=============================================================================

void FSavegameManager::NotifyNewSave(const FString &file, const FString &title, bool okForQuicksave, bool forceQuicksave)
{
	FSaveGameNode *node;

	if (file.IsEmpty())
		return;

	ReadSaveStrings();

	// See if the file is already in our list
	for (unsigned i = 0; i<SaveGames.Size(); i++)
	{
		FSaveGameNode *node = SaveGames[i];
#ifdef __unix__
		if (node->Filename.Compare(file) == 0)
#else
		if (node->Filename.CompareNoCase(file) == 0)
#endif
		{
			node->SaveTitle = title;
			node->bOldVersion = false;
			node->bMissingWads = false;
			if (okForQuicksave)
			{
				if (quickSaveSlot == nullptr || quickSaveSlot == (FSaveGameNode*)1 || forceQuicksave) quickSaveSlot = node;
				LastAccessed = LastSaved = i - 1; // without <new save> item
			}
			return;
		}
	}

	node = new FSaveGameNode;
	node->SaveTitle = title;
	node->Filename = file;
	node->bOldVersion = false;
	node->bMissingWads = false;
	int index = InsertSaveNode(node);

	if (okForQuicksave)
	{
		if (quickSaveSlot == nullptr || quickSaveSlot == (FSaveGameNode*)1 || forceQuicksave) quickSaveSlot = node;
		LastAccessed = LastSaved = index - 1; // without <new save> item
	}
	else
	{
		LastAccessed = ++LastSaved;
	}
}

//=============================================================================
//
// Loads the savegame
//
//=============================================================================

void FSavegameManager::LoadSavegame(int Selected)
{
	auto sel = savegameManager.GetSavegame(Selected);
	if (sel && !sel->bOldVersion && !sel->bMissingWads)
	{
		savegameManager.LoadGame(SaveGames[Selected]);
		if (quickSaveSlot == (FSaveGameNode*)1)
		{
			quickSaveSlot = SaveGames[Selected];
		}
		M_ClearMenus();
		LastAccessed = Selected;
	}
}


//=============================================================================
//
// 
//
//=============================================================================

void FSavegameManager::DoSave(int Selected, const char *savegamestring)
{
	if (Selected != 0)
	{
		auto node = *SaveGames[Selected];
		node.SaveTitle = savegamestring;
		savegameManager.SaveGame(&node, true, false);
	}
	else
	{
		// Find an unused filename and save as that
		FString filename;
		int i;

		for (i = 0;; ++i)
		{
			filename = G_BuildSaveName(FStringf("save%04d", i));
			if (!FileExists(filename))
			{
				break;
			}
		}
		FSaveGameNode sg{ savegamestring, filename };
		savegameManager.SaveGame(&sg, true, false);
	}
	M_ClearMenus();
}


//=============================================================================
//
//
//
//=============================================================================

unsigned FSavegameManager::ExtractSaveData(int index)
{
	FResourceFile *resf;
	FSaveGameNode *node;

	if (index == -1)
	{
		if (SaveGames.Size() > 0 && SaveGames[0]->bNoDelete)
		{
			index = LastSaved + 1;
		}
		else
		{
			index = LastAccessed < 0? 0 : LastAccessed;
		}
	}

	UnloadSaveData();

	if ((unsigned)index < SaveGames.Size() &&
		(node = SaveGames[index]) &&
		!node->Filename.IsEmpty() &&
		!node->bOldVersion &&
		(resf = FResourceFile::OpenResourceFile(node->Filename.GetChars(), true)) != nullptr)
	{
		FResourceLump *info = resf->FindLump("info.json");
		if (info == nullptr)
		{
			// this should not happen because the file has already been verified.
			return index;
		}

		void* data = info->Lock();
		FSerializer arc;
		if (!arc.OpenReader((const char*)data, info->LumpSize))
		{
			info->Unlock();
			return index;
		}
		info->Unlock();

		FString comment, fcomment, ncomment, mtime;

		arc("Creation Time", comment)
			("Map Label", fcomment)
			("Map Name", ncomment)
			("Map Time", mtime);

		comment.AppendFormat("\n%s - %s\n%s", fcomment.GetChars(), ncomment.GetChars(), mtime.GetChars());
		SaveCommentString = comment;

		FResourceLump *pic = resf->FindLump("savepic.png");
		if (pic != nullptr)
		{
			FileReader picreader;

			picreader.OpenMemoryArray([=](TArray<uint8_t> &array)
			{
				auto cache = pic->Lock();
				array.Resize(pic->LumpSize);
				memcpy(&array[0], cache, pic->LumpSize);
				pic->Unlock();
				return true;
			});
			PNGHandle *png = M_VerifyPNG(picreader);
			if (png != nullptr)
			{
				SavePic = PNGTexture_CreateFromFile(png, node->Filename);
				delete png;
				if (SavePic && SavePic->GetDisplayWidth() == 1 && SavePic->GetDisplayHeight() == 1)
				{
					delete SavePic;
					SavePic = nullptr;
					SavePicData.Clear();
				}
			}
		}
		delete resf;
	}
	return index;
}

//=============================================================================
//
//
//
//=============================================================================

void FSavegameManager::UnloadSaveData()
{
	if (SavePic != nullptr)
	{
		delete SavePic;
	}

	SaveCommentString = "";
	SavePic = nullptr;
	SavePicData.Clear();
}

//=============================================================================
//
//
//
//=============================================================================

void FSavegameManager::ClearSaveStuff()
{
	UnloadSaveData();
	if (quickSaveSlot == (FSaveGameNode*)1)
	{
		quickSaveSlot = nullptr;
	}
}


//=============================================================================
//
//
//
//=============================================================================

bool FSavegameManager::DrawSavePic(int x, int y, int w, int h)
{
	if (SavePic == nullptr) return false;
	DrawTexture(twod, SavePic, x, y, 	DTA_DestWidth, w, DTA_DestHeight, h, DTA_Masked, false,	TAG_DONE);
	return true;
}


//=============================================================================
//
//
//
//=============================================================================

void FSavegameManager::SetFileInfo(int Selected)
{
	if (!SaveGames[Selected]->Filename.IsEmpty())
	{
		SaveCommentString.Format("File on disk:\n%s", SaveGames[Selected]->Filename.GetChars());
	}
}


//=============================================================================
//
//
//
//=============================================================================

unsigned FSavegameManager::SavegameCount()
{
	return SaveGames.Size();
}


//=============================================================================
//
//
//
//=============================================================================

FSaveGameNode *FSavegameManager::GetSavegame(int i)
{
	return SaveGames[i];
}


//=============================================================================
//
//
//
//=============================================================================

void FSavegameManager::InsertNewSaveNode()
{
	NewSaveNode.SaveTitle = GStrings("NEWSAVE");
	NewSaveNode.bNoDelete = true;
	SaveGames.Insert(0, &NewSaveNode);
}


//=============================================================================
//
//
//
//=============================================================================

bool FSavegameManager::RemoveNewSaveNode()
{
	if (SaveGames[0] == &NewSaveNode)
	{
		SaveGames.Delete(0);
		return true;
	}
	return false;
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
	savegameManager.SaveGame(&sg, false, false);
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
	savegameManager.SaveGame(&sg, false, false);
}


//=============================================================================
//
//
//
//=============================================================================

CCMD(quicksave)
{	// F6
	if (!gi->CanSave()) return;

	if (savegameManager.quickSaveSlot == NULL || savegameManager.quickSaveSlot == (FSaveGameNode*)1)
	{
		M_StartControlPanel(true);
		M_SetMenu(NAME_Savegamemenu);
		return;
	}

	auto slot = savegameManager.quickSaveSlot;

	// [mxd]. Just save the game, no questions asked.
	if (!saveloadconfirmation)
	{
		savegameManager.SaveGame(savegameManager.quickSaveSlot, true, true);
		return;
	}

	FString tempstring = GStrings("QSPROMPT");
	tempstring.Substitute("%s", slot->SaveTitle.GetChars());
	M_StartControlPanel(true);

	DMenu* newmenu = CreateMessageBoxMenu(CurrentMenu, tempstring, 0, INT_MAX, false, NAME_None, [](bool res)
		{
			if (res)
			{
				savegameManager.SaveGame(savegameManager.quickSaveSlot, true, true);
			}
			return true;
		});

	M_ActivateMenu(newmenu);
}

//=============================================================================
//
//
//
//=============================================================================

CCMD(quickload)
{	// F9
#if 0
	if (netgame)
	{
		M_StartControlPanel(true);
		M_StartMessage(GStrings("QLOADNET"), 1);
		return;
	}
#endif

	if (savegameManager.quickSaveSlot == nullptr || savegameManager.quickSaveSlot == (FSaveGameNode*)1)
	{
		M_StartControlPanel(true);
		// signal that whatever gets loaded should be the new quicksave
		savegameManager.quickSaveSlot = (FSaveGameNode*)1;
		M_SetMenu(NAME_Loadgamemenu);
		return;
	}

	// [mxd]. Just load the game, no questions asked.
	if (!saveloadconfirmation)
	{
		savegameManager.LoadGame(savegameManager.quickSaveSlot);
		return;
	}
	FString tempstring = GStrings("QLPROMPT");
	tempstring.Substitute("%s", savegameManager.quickSaveSlot->SaveTitle.GetChars());

	M_StartControlPanel(true);

	DMenu* newmenu = CreateMessageBoxMenu(CurrentMenu, tempstring, 0, INT_MAX, false, NAME_None, [](bool res)
		{
			if (res)
			{
				savegameManager.LoadGame(savegameManager.quickSaveSlot);
			}
			return true;
		});
	M_ActivateMenu(newmenu);
}
