/*
** menu.cpp
** Menu base class and global interface
**
**---------------------------------------------------------------------------
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

#include "c_dispatch.h"
#include "d_gui.h"
#include "c_buttons.h"
#include "c_console.h"
#include "c_bind.h"
#include "d_eventbase.h"
#include "g_input.h"
#include "configfile.h"
#include "gstrings.h"
#include "menu.h"
#include "vm.h"
#include "v_video.h"
#include "i_system.h"
#include "types.h"
#include "texturemanager.h"
#include "v_draw.h"
#include "vm.h"
#include "gamestate.h"
#include "i_interface.h" 
#include "d_event.h"
#include "st_start.h"
#include "i_system.h"
#include "gameconfigfile.h"
#include "gamecontrol.h"
#include "raze_sound.h"
#include "gamestruct.h"
#include "razemenu.h"
#include "mapinfo.h"
#include "statistics.h"
#include "i_net.h"
#include "savegamehelp.h"
#include "gi.h"
#include "raze_music.h"
#include "razefont.h"

EXTERN_CVAR(Int, cl_gfxlocalization)
EXTERN_CVAR(Bool, m_quickexit)
EXTERN_CVAR(Bool, saveloadconfirmation) // [mxd]
EXTERN_CVAR(Bool, quicksaverotation)
EXTERN_CVAR(Bool, show_messages)
CVAR(Bool, menu_sounds, true, CVAR_ARCHIVE) // added mainly because RR's sounds are so supremely annoying.

typedef void(*hfunc)();
DMenu* CreateMessageBoxMenu(DMenu* parent, const char* message, int messagemode, bool playsound, FName action = NAME_None, hfunc handler = nullptr);
bool OkForLocalization(FTextureID texnum, const char* substitute);
void D_ToggleHud();
void I_WaitVBL(int count);

extern bool hud_toggled;
bool help_disabled;
FNewGameStartup NewGameStartupInfo;



//FNewGameStartup NewGameStartupInfo;

static bool DoStartGame(FNewGameStartup& gs)
{
	auto vol = FindVolume(gs.Episode);
	if (!vol) return false;

	if (isShareware() && (vol->flags & VF_SHAREWARELOCK))
	{
		M_StartMessage(GStrings("SHAREWARELOCK"), 1, NAME_None);
		return false;
	}

	auto map = FindMapByName(vol->startmap);
	if (!map) return false;
	soundEngine->StopAllChannels();

	gi->StartGame(gs);	// play game specific effects (like Duke/RR/SW's voice lines when starting a game.)

	DeferredStartGame(map, gs.Skill);
	return true;
}



bool M_SetSpecialMenu(FName& menu, int param)
{
	// Engine credits need a different approach to work with the option search
#if 0
	// Transitions between the engine credits pages need to pop off the last slide
	if (!strnicmp(menu.GetChars(), "EngineCredits", 13) && CurrentMenu && !strnicmp(CurrentMenu->GetClass()->TypeName.GetChars(), "EngineCredits", 13))
	{
		auto m = CurrentMenu;
		CurrentMenu = m->mParentMenu;
		m->mParentMenu = nullptr;
		m->Destroy();
	}
#endif

	switch (menu.GetIndex())
	{
	case NAME_Mainmenu:
		if (gi->CanSave()) menu = NAME_IngameMenu;
		break;

	case NAME_Skillmenu:
		// sent from the episode menu
		NewGameStartupInfo.Episode = param;
		NewGameStartupInfo.Level = 0;
		NewGameStartupInfo.Skill = gDefaultSkill;
		return true;

	case NAME_Startgame:
	case NAME_StartgameNoSkill:
		NewGameStartupInfo.Skill = param;
		if (menu == NAME_StartgameNoSkill)
		{
			menu = NAME_Startgame;
			NewGameStartupInfo.Episode = param;
			NewGameStartupInfo.Skill = 1;
		}
		if (DoStartGame(NewGameStartupInfo))
		{
			M_ClearMenus();
			int ep = NewGameStartupInfo.Episode;
			auto vol = FindVolume(ep);
			if (vol) STAT_StartNewGame(vol->name, NewGameStartupInfo.Skill);
			inputState.ClearAllInput();
		}
		return false;

	case NAME_CustomSubMenu1:
		menu = ENamedName(menu.GetIndex() + param);
		break;

	case NAME_Savegamemenu:
		if (!gi->CanSave())
		{
			// cannot save outside the game.
			M_StartMessage(GStrings("SAVEDEAD"), 1, NAME_None);
			return true;
		}
		break;

	case NAME_Quitmenu:
		// This is no separate class
		C_DoCommand("menu_quit");
		return false;

	case NAME_EndGameMenu:
		// This is no separate class
		C_DoCommand("menu_endgame");
		return false;
}

	// End of special checks
	return true;
}

//=============================================================================
//
//
//
//=============================================================================

void M_StartControlPanel(bool makeSound, bool)
{
	// intro might call this repeatedly
	if (CurrentMenu != NULL)
		return;

	GSnd->SetSfxPaused(true, PAUSESFX_MENU);
	gi->MenuOpened();
	if (makeSound && menu_sounds) gi->MenuSound(ActivateSound);
	M_DoStartControlPanel(false);
}


void System_MenuClosed()
{
	GSnd->SetSfxPaused(false, PAUSESFX_MENU);
	inputState.ClearAllInput();
	gi->MenuClosed();
}

//==========================================================================
//
//
//
//==========================================================================

void System_MenuDim()
{
	if (gamestate != GS_MENUSCREEN)	// With GS_MENUSCREEN we can assume that the background has been tuned for proper menu display already.
	{
		Dim(twod, 0, 0.5f, 0, 0, screen->GetWidth(), screen->GetHeight());
	}
}

//=============================================================================
//
//
//
//=============================================================================

CCMD(menu_quit)
{	// F10

	M_StartControlPanel(true);

	FString EndString;
	EndString << GStrings("CONFIRM_QUITMSG") << "\n\n" << GStrings("PRESSYN");

	DMenu* newmenu = CreateMessageBoxMenu(CurrentMenu, EndString, 0, false, NAME_None, []()
		{
            M_ClearMenus();
			gi->ExitFromMenu();
		});

	M_ActivateMenu(newmenu);
}

//=============================================================================
//
//
//
//=============================================================================

CCMD(menu_endgame)
{	// F7
	if (!gi->CanSave())
	{
		return;
	}

	M_StartControlPanel(true);
	FString tempstring;
	tempstring << GStrings("ENDGAME") << "\n\n" << GStrings("PRESSYN");
	DMenu* newmenu = CreateMessageBoxMenu(CurrentMenu, tempstring, 0, false, NAME_None, []()
		{
			STAT_Cancel();
			M_ClearMenus();
			Mus_Stop();
			gameaction = ga_mainmenu;
		});

	M_ActivateMenu(newmenu);
}

//=============================================================================
//
//
//
//=============================================================================


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
		G_SaveGame(savegameManager.quickSaveSlot->Filename, savegameManager.quickSaveSlot->SaveTitle);
		return;
	}

	FString tempstring = GStrings("QSPROMPT");
	tempstring.Substitute("%s", slot->SaveTitle.GetChars());
	M_StartControlPanel(true);

	DMenu* newmenu = CreateMessageBoxMenu(CurrentMenu, tempstring, 0, false, NAME_None, []()
		{
			M_ClearMenus();
			G_SaveGame(savegameManager.quickSaveSlot->Filename, savegameManager.quickSaveSlot->SaveTitle);
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
	if (netgame)
	{
		M_StartControlPanel(true);
		M_StartMessage(GStrings("QLOADNET"), 1);
		return;
	}

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
		G_LoadGame(savegameManager.quickSaveSlot->Filename);
		return;
	}
	FString tempstring = GStrings("QLPROMPT");
	tempstring.Substitute("%s", savegameManager.quickSaveSlot->SaveTitle.GetChars());

	M_StartControlPanel(true);

	DMenu* newmenu = CreateMessageBoxMenu(CurrentMenu, tempstring, 0, false, NAME_None, []()
		{
			M_ClearMenus();
			G_LoadGame(savegameManager.quickSaveSlot->Filename);
	});
	M_ActivateMenu(newmenu);
}

//=============================================================================
//
// Creation wrapper
//
//=============================================================================

static DMenuItemBase* CreateCustomListMenuItemText(double x, double y, int height, int hotkey, const char* text, FFont* font, PalEntry color1, PalEntry color2, FName command, int param)
{
	const char* classname = 
		isBlood() ? "ListMenuItemBloodTextItem" :
		isSWALL() ? "ListMenuItemSWTextItem" :
		(g_gameType & GAMEFLAG_PSEXHUMED) ? "ListMenuItemExhumedTextItem" : "ListMenuItemDukeTextItem";
	auto c = PClass::FindClass(classname);
	auto p = c->CreateNew();
	FString keystr = FString(char(hotkey));
	FString textstr = text;
	VMValue params[] = { p, x, y, height, &keystr, &textstr, font, int(color1.d), int(color2.d), command.GetIndex(), param };
	auto f = dyn_cast<PFunction>(c->FindSymbol("InitDirect", false));
	VMCall(f->Variants[0].Implementation, params, countof(params), nullptr, 0);
	return (DMenuItemBase*)p;
}

//=============================================================================
//
// Creates the episode menu
//
//=============================================================================
extern TArray<VolumeRecord> volumes;

static void BuildEpisodeMenu()
{
	// Build episode menu
	int addedVolumes = 0;
	bool textadded = false;
	DMenuDescriptor** desc = MenuDescriptors.CheckKey(NAME_Episodemenu);
	if (desc != nullptr && (*desc)->IsKindOf(RUNTIME_CLASS(DListMenuDescriptor)))
	{
		DListMenuDescriptor* ld = static_cast<DListMenuDescriptor*>(*desc);

		DMenuItemBase* popped = nullptr;
		if (ld->mItems.Size() && ld->mItems.Last()->IsKindOf(NAME_ListMenuItemBloodDripDrawer))
		{
			ld->mItems.Pop(popped);
		}

		ld->mSelectedItem = gDefaultVolume + ld->mItems.Size(); // account for pre-added items
		double y = ld->mYpos;

		// Volume definitions should be sorted by intended menu order.
		auto font = PickSmallFont();
		for (auto &vol : volumes)
		{
			if (vol.name.IsNotEmpty() && !(vol.flags & VF_HIDEFROMSP))
			{
				int isShareware = ((g_gameType & GAMEFLAG_DUKE) && (g_gameType & GAMEFLAG_SHAREWARE) && (vol.flags & VF_SHAREWARELOCK));
				auto it = CreateCustomListMenuItemText(ld->mXpos, y, ld->mLinespacing, vol.name[0],
					vol.name, ld->mFont, CR_UNTRANSLATED, isShareware, NAME_Skillmenu, vol.index); // font colors are not used, so hijack one for the shareware flag.

				y += ld->mLinespacing;
				ld->mItems.Push(it);
				addedVolumes++;
				if (vol.subtitle.IsNotEmpty())
				{
					auto it = CreateCustomListMenuItemText(ld->mXpos, y, ld->mLinespacing * 6 / 10, 1,
						vol.subtitle, SmallFont, CR_GRAY, false, NAME_None, vol.index);
					y += ld->mLinespacing * 6 / 10;
					ld->mItems.Push(it);
					textadded = true;
				}
			}
		}
#if 0	// this needs to be backed by a working selection menu, until that gets done it must be disabled.
		if (!(g_gameType & GAMEFLAG_SHAREWARE))
		{
			//auto it = new FListMenuItemNativeStaticText(ld->mXpos, "", NIT_SmallFont);	// empty entry as spacer.
			//ld->mItems.Push(it);

			y += ld->mLinespacing / 3;
			auto it = CreateCustomListMenuItemText(ld->mXpos, y, ld->mLinespacing, 'U', "$MNU_USERMAP", ld->mFont, 0, 0, NAME_UsermapMenu);
			ld->mItems.Push(it);
			addedVolumes++;
		}
#endif
		if (addedVolumes == 1)
		{
			ld->mAutoselect = ld->mItems.Size() - (textadded ? 2 : 1);
		}
		if (popped) ld->mItems.Push(popped);
	}

	// Build skill menu
	int addedSkills = 0;
	desc = MenuDescriptors.CheckKey(NAME_Skillmenu);
	if (desc != nullptr && (*desc)->IsKindOf(RUNTIME_CLASS(DListMenuDescriptor)))
	{
		DListMenuDescriptor* ld = static_cast<DListMenuDescriptor*>(*desc);
		DMenuItemBase* popped = nullptr;
		if (ld->mItems.Size() && ld->mItems.Last()->IsKindOf(NAME_ListMenuItemBloodDripDrawer))
		{
			ld->mItems.Pop(popped);
		}
		if (isBlood() || isSWALL()) gDefaultSkill = 2;
		ld->mSelectedItem = gDefaultSkill + ld->mItems.Size(); // account for pre-added items
		double y = ld->mYpos;

		for (int i = 0; i < MAXSKILLS; i++)
		{
			if (gSkillNames[i].IsNotEmpty())
			{
				auto it = CreateCustomListMenuItemText(ld->mXpos, y, ld->mLinespacing, gSkillNames[i][0], gSkillNames[i], ld->mFont, CR_UNTRANSLATED, 0, NAME_Startgame, i);
				y += ld->mLinespacing;
				ld->mItems.Push(it);
				addedSkills++;
			}
		}
		if (addedSkills == 0)
		{
			// Need to add one item with the default skill so that the menu does not break.
			auto it = CreateCustomListMenuItemText(ld->mXpos, y, ld->mLinespacing, 0, "", ld->mFont, 0, 0, NAME_Startgame, gDefaultSkill);
			ld->mItems.Push(it);
		}
		if (addedSkills == 1)
		{
			ld->mAutoselect = ld->mItems.Size() - 1;
		}
		if (popped) ld->mItems.Push(popped);
	}
}

//=============================================================================
//
// Reads any XHAIRS lumps for the names of crosshairs and
// adds them to the display options menu.
//
//=============================================================================

static void InitCrosshairsList()
{
	int lastlump, lump;

	lastlump = 0;

	FOptionValues **opt = OptionValues.CheckKey(NAME_Crosshairs);
	if (opt == nullptr) 
	{
		return;	// no crosshair value list present. No need to go on.
	}

	FOptionValues::Pair *pair = &(*opt)->mValues[(*opt)->mValues.Reserve(1)];
	pair->Value = 0;
	pair->Text = "None";

	while ((lump = fileSystem.FindLump("XHAIRS", &lastlump)) != -1)
	{
		FScanner sc(lump);
		while (sc.GetNumber())
		{
			FOptionValues::Pair value;
			value.Value = sc.Number;
			sc.MustGetString();
			value.Text = sc.String;
			if (value.Value != 0)
			{ // Check if it already exists. If not, add it.
				unsigned int i;

				for (i = 1; i < (*opt)->mValues.Size(); ++i)
				{
					if ((*opt)->mValues[i].Value == value.Value)
					{
						break;
					}
				}
				if (i < (*opt)->mValues.Size())
				{
					(*opt)->mValues[i].Text = value.Text;
				}
				else
				{
					(*opt)->mValues.Push(value);
				}
			}
		}
	}
}

//==========================================================================
//
// Defines how graphics substitution is handled.
// 0: Never replace a text-containing graphic with a font-based text.
// 1: Always replace, regardless of any missing information. Useful for testing the substitution without providing full data.
// 2: Only replace for non-default texts, i.e. if some language redefines the string's content, use it instead of the graphic. Never replace a localized graphic.
// 3: Only replace if the string is not the default and the graphic comes from the IWAD. Never replace a localized graphic.
// 4: Like 1, but lets localized graphics pass.
//
// The default is 3, which only replaces known content with non-default texts.
//
//==========================================================================

bool CheckSkipGameOptionBlock(FScanner& sc) { return false; } // not applicable

#if 0
CUSTOM_CVAR(Int, cl_gfxlocalization, 3, CVAR_ARCHIVE)
{
	if (self < 0 || self > 4) self = 0;
}

bool OkForLocalization(FTextureID texnum, const char* substitute)
{
	if (!texnum.isValid()) return false;

	// First the unconditional settings, 0='never' and 1='always'.
	if (cl_gfxlocalization == 1 || gameinfo.forcetextinmenus) return false;
	if (cl_gfxlocalization == 0 || gameinfo.forcenogfxsubstitution) return true;
	return TexMan.OkForLocalization(texnum, substitute, cl_gfxlocalization);
}

#endif
void SetDefaultMenuColors()
{
	PClass* cls = nullptr;
	//OptionSettings.mTitleColor = CR_RED;// V_FindFontColor(gameinfo.mTitleColor);
	OptionSettings.mFontColor = CR_RED;
	OptionSettings.mFontColorValue = CR_GRAY;
	OptionSettings.mFontColorMore = CR_GRAY;
	OptionSettings.mFontColorHeader = CR_GOLD;
	OptionSettings.mFontColorHighlight = CR_YELLOW;
	OptionSettings.mFontColorSelection = CR_BRICK;
	gameinfo.mSliderColor = "Orange";
	gameinfo.mSliderBackColor = "White";

	if (isBlood())
	{
		OptionSettings.mFontColorHeader = CR_DARKGRAY;
		OptionSettings.mFontColorHighlight = CR_WHITE;
		OptionSettings.mFontColorSelection = CR_DARKRED;
		gameinfo.mSliderColor = "Red";
		cls = PClass::FindClass("BloodMenuDelegate");
	}
	else if (isSWALL())
	{
		OptionSettings.mFontColorHeader = CR_DARKRED;
		OptionSettings.mFontColorHighlight = CR_WHITE;
		gameinfo.mSliderColor = "Red";
		cls = PClass::FindClass("SWMenuDelegate");
	}
	else if (g_gameType & GAMEFLAG_PSEXHUMED)
	{
		OptionSettings.mFontColorHeader = CR_LIGHTBLUE;
		OptionSettings.mFontColorHighlight = CR_SAPPHIRE;
		OptionSettings.mFontColorSelection = CR_ORANGE;
		OptionSettings.mFontColor = CR_FIRE;
		gameinfo.mSliderColor = "Yellow";
		cls = PClass::FindClass("ExhumedMenuDelegate");
	}
	else
	{
		if (isNamWW2GI())
		{
			OptionSettings.mFontColor = CR_DARKGREEN;
			OptionSettings.mFontColorHeader = CR_DARKGRAY;
			OptionSettings.mFontColorHighlight = CR_WHITE;
			OptionSettings.mFontColorSelection = CR_DARKGREEN;
			gameinfo.mSliderColor = "Green";
		}
		else if (isRR())
		{
			OptionSettings.mFontColor = CR_BROWN;
			OptionSettings.mFontColorHeader = CR_DARKBROWN;
			OptionSettings.mFontColorHighlight = CR_ORANGE;
			OptionSettings.mFontColorSelection = CR_TAN;
			gameinfo.mSliderColor = "Tan";
		}
		cls = PClass::FindClass("DukeMenuDelegate");
	}
	if (!cls) cls = PClass::FindClass("RazeMenuDelegate");
	if (cls) menuDelegate = cls->CreateNew();
}

void BuildGameMenus()
{
	BuildEpisodeMenu();
	InitCrosshairsList();
	UpdateJoystickMenu(nullptr);
}


//=============================================================================
//
// [RH] Most menus can now be accessed directly
// through console commands.
//
//=============================================================================

EXTERN_CVAR(Int, screenblocks)

CCMD(reset2defaults)
{
	C_SetDefaultBindings();
	C_SetCVarsToDefaults();
}

CCMD(reset2saved)
{
	GameConfig->DoGlobalSetup();
	GameConfig->DoGameSetup(currentGame);
}

CCMD(menu_main)
{
	if (gamestate == GS_FULLCONSOLE) gamestate = GS_MENUSCREEN;
	M_StartControlPanel(true);
	M_SetMenu(NAME_Mainmenu, -1);
}

CCMD(openhelpmenu)
{
	if (!help_disabled)
	{
		M_StartControlPanel(true);
		M_SetMenu(NAME_HelpMenu);
	}
}

CCMD(opensavemenu)
{
	if (gi->CanSave())
	{
		M_StartControlPanel(true);
		M_SetMenu(NAME_Savegamemenu);
	}
}

CCMD(openloadmenu)
{
	M_StartControlPanel(true);
	M_SetMenu(NAME_Loadgamemenu);
}


// The sound system is not yet capable of resolving this properly.
DEFINE_ACTION_FUNCTION(_RazeMenuDelegate, PlaySound)
{
	PARAM_SELF_STRUCT_PROLOGUE(void);
	PARAM_NAME(name);
	EMenuSounds soundindex;

	switch (name.GetIndex())
	{
	case NAME_menu_cursor:
		soundindex = CursorSound;
		break;

	case NAME_menu_choose:
		soundindex = ChooseSound;
		break;

	case NAME_menu_backup:
		soundindex = BackSound;
		break;

	case NAME_menu_clear:
	case NAME_menu_dismiss:
		soundindex = CloseSound;
		break;

	case NAME_menu_change:
		soundindex = ChangeSound;
		break;

	case NAME_menu_advance:
		soundindex = AdvanceSound;
		break;

	default:
		return 0;
	}
	gi->MenuSound(soundindex);
	return 0;
}

// C_ToggleConsole cannot be exported for security reasons as it can be used to make the engine unresponsive.
DEFINE_ACTION_FUNCTION(_RazeMenuDelegate, MenuDismissed)
{
	if (CurrentMenu == nullptr && gamestate == GS_MENUSCREEN) C_ToggleConsole();
	return 0;
}

DEFINE_ACTION_FUNCTION(_PlayerMenu, DrawPlayerSprite)
{
	PARAM_PROLOGUE;
	PARAM_INT(selected);
	gi->DrawPlayerSprite(DVector2(0.,0.), selected);
	return 0;
}

#ifdef _WIN32
EXTERN_CVAR(Bool, vr_enable_quadbuffered)
#endif

void UpdateVRModes(bool considerQuadBuffered)
{
	FOptionValues** pVRModes = OptionValues.CheckKey("VRMode");
	if (pVRModes == nullptr) return;

	TArray<FOptionValues::Pair>& vals = (*pVRModes)->mValues;
	TArray<FOptionValues::Pair> filteredValues;
	int cnt = vals.Size();
	for (int i = 0; i < cnt; ++i) {
		auto const& mode = vals[i];
		if (mode.Value == 7) {  // Quad-buffered stereo
#ifdef _WIN32
			if (!vr_enable_quadbuffered) continue;
#else
			continue;  // Remove quad-buffered option on Mac and Linux
#endif
			if (!considerQuadBuffered) continue;  // Probably no compatible screen mode was found
		}
		filteredValues.Push(mode);
	}
	vals = filteredValues;
}
