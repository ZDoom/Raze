/*
** menudef.cpp
** MENUDEF parser amd menu generation code
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
#include <float.h>

#include "menu.h"
#include "c_dispatch.h"
#include "filesystem.h"
#include "sc_man.h"
#include "v_font.h"
#include "c_bind.h"
#include "d_event.h"
#include "d_gui.h"
#include "printf.h"
#include "gamecontrol.h"
#include "cmdlib.h"
#include "c_cvars.h"
#include "optionmenuitems.h"
#include "i_soundfont.h"
#include "zstring.h"
#include "texturemanager.h"
#include "v_video.h"
#include <zmusic.h>

// Menu-relevant content that gets filled in by scripts. This will get processed after the game has loaded.
FString gSkillNames[MAXSKILLS];
FString gVolumeNames[MAXVOLUMES];
FString gVolumeSubtitles[MAXVOLUMES];
int32_t gVolumeFlags[MAXVOLUMES];
int gDefaultVolume = 0, gDefaultSkill = 1;
MenuGameplayStemEntry g_MenuGameplayEntries[MAXMENUGAMEPLAYENTRIES];

MenuDescriptorList MenuDescriptors;
static FListMenuDescriptor DefaultListMenuSettings;	// contains common settings for all list menus
static FOptionMenuDescriptor DefaultOptionMenuSettings;	// contains common settings for all Option menus
FOptionMenuSettings OptionSettings;
FOptionMap OptionValues;

void I_BuildMIDIMenuList(FOptionValues* opt);
void I_BuildALDeviceList(FOptionValues *opt);
void I_BuildALResamplersList(FOptionValues* opt);

bool IsOpenALPresent();

void M_DeinitMenus()
{
	{
		MenuDescriptorList::Iterator it(MenuDescriptors);

		MenuDescriptorList::Pair *pair;

		while (it.NextPair(pair))
		{
			delete pair->Value;
			pair->Value = NULL;
		}
	}

	{
		FOptionMap::Iterator it(OptionValues);

		FOptionMap::Pair *pair;

		while (it.NextPair(pair))
		{
			delete pair->Value;
			pair->Value = NULL;
		}
	}
	MenuDescriptors.Clear();
	OptionValues.Clear();

	CurrentMenu = NULL;
	DefaultListMenuSettings.mItems.Clear();
}

static FGameTexture* GetMenuTexture(const char* const name)
{
	auto texid = TexMan.CheckForTexture(name, ETextureType::Any);
	if (!texid.isValid())
	{
		Printf("Missing menu texture: \"%s\"\n", name);
	}

	return TexMan.GetGameTexture(texid);
}

//=============================================================================
//
//
//
//=============================================================================

static void SkipSubBlock(FScanner &sc)
{
	sc.MustGetStringName("{");
	int depth = 1;
	while (depth > 0)
	{
		sc.MustGetString();
		if (sc.Compare("{")) depth++;
		if (sc.Compare("}")) depth--;
	}
}

//=============================================================================
//
//
//
//=============================================================================

struct gamefilter
{
	const char* gamename;
	int gameflag;
};

static const gamefilter games[] = {
	{ "Duke", GAMEFLAG_DUKE},
	{ "Nam", GAMEFLAG_NAM|GAMEFLAG_NAPALM},
	{ "NamOnly", GAMEFLAG_NAM},	// for cases where the difference matters.
	{ "Napalm", GAMEFLAG_NAPALM},
	{ "WW2GI", GAMEFLAG_WW2GI},
	{ "Fury", GAMEFLAG_FURY},
	{ "Redneck", GAMEFLAG_RR},
	{ "RedneckRides", GAMEFLAG_RRRA},
	{ "Deer", GAMEFLAG_DEER},
	{ "Blood", GAMEFLAG_BLOOD},
	{ "ShadowWarrior", GAMEFLAG_SW},
	{ "Exhumed", GAMEFLAG_POWERSLAVE|GAMEFLAG_EXHUMED},
};

// for other parts that need to filter by game name.
bool validFilter(const char *str)
{
	for (auto &gf : games)
	{
		if (g_gameType & gf.gameflag)
		{
			if (!stricmp(str, gf.gamename)) return true;
		}
	}
	return false;
}


static bool CheckSkipGameBlock(FScanner &sc)
{
	int filter = 0;
	sc.MustGetStringName("(");
	do
	{
		sc.MustGetString();
		int gi = sc.MustMatchString(&games[0].gamename, sizeof(games[0]));

		filter |= games[gi].gameflag;
	}
	while (sc.CheckString(","));
	sc.MustGetStringName(")");
	if (!(filter & g_gameType))
	{
		SkipSubBlock(sc);
		return !sc.CheckString("else");
	}
	return false;
}

//=============================================================================
//
//
//
//=============================================================================

static bool CheckSkipOptionBlock(FScanner &sc)
{
	bool filter = false;
	sc.MustGetStringName("(");
	do
	{
		sc.MustGetString();
		if (sc.Compare("Windows"))
		{
			#ifdef _WIN32
				filter = true;
			#endif
		}
		else if (sc.Compare("unix"))
		{
			#ifdef __unix__
				filter = true;
			#endif
		}
		else if (sc.Compare("Mac"))
		{
			#ifdef __APPLE__
				filter = true;
			#endif
		}

		if (sc.Compare("openal"))
		{
			if (IsOpenALPresent())
				filter = true;
		}
	}
	while (sc.CheckString(","));
	sc.MustGetStringName(")");
	if (!filter)
	{
		SkipSubBlock(sc);
		return !sc.CheckString("else");
	}
	return false;
}

//=============================================================================
//
//
//
//=============================================================================

static bool CheckSkipNoSwBlock(FScanner& sc)
{
	sc.MustGetStringName("(");
	sc.MustGetString();
	bool res = sc.Compare("true");
	sc.MustGetStringName(")");
	if ((!(g_gameType & GAMEFLAG_SHAREWARE)) == res)
	{
		SkipSubBlock(sc);
		return !sc.CheckString("else");
	}
	return false;
}

//=============================================================================
//
//
//
//=============================================================================

static void ParseListMenuBody(FScanner &sc, FListMenuDescriptor *desc)
{
	sc.MustGetStringName("{");
	while (!sc.CheckString("}"))
	{
		sc.MustGetString();
		if (sc.Compare("else"))
		{
			SkipSubBlock(sc);
		}
		else if (sc.Compare("ifgame"))
		{
			if (!CheckSkipGameBlock(sc))
			{
				// recursively parse sub-block
				ParseListMenuBody(sc, desc);
			}
		}
		else if (sc.Compare("ifoption"))
		{
			if (!CheckSkipOptionBlock(sc))
			{
				// recursively parse sub-block
				ParseListMenuBody(sc, desc);
			}
		}
		else if (sc.Compare("ifshareware"))
		{
			if (!CheckSkipNoSwBlock(sc))
			{
				// recursively parse sub-block
				ParseListMenuBody(sc, desc);
			}
		}
		else if (sc.Compare("Class"))
		{
			sc.MustGetString();
			FString s = sc.String;
			s.Substitute("$", gi->Name());
			desc->mClass = s;
		}
		else if (sc.Compare("Selector"))
		{
			sc.MustGetString();
			desc->mSelector = GetMenuTexture(sc.String);
			sc.MustGetStringName(",");
			sc.MustGetNumber();
			desc->mSelectOfsX = sc.Number;
			sc.MustGetStringName(",");
			sc.MustGetNumber();
			desc->mSelectOfsY = sc.Number;
		}
		else if (sc.Compare("Linespacing"))
		{
			sc.MustGetNumber();
			desc->mLinespacing = sc.Number;
		}
		else if (sc.Compare("Position"))
		{
			sc.MustGetNumber();
			desc->mXpos = sc.Number;
			sc.MustGetStringName(",");
			sc.MustGetNumber();
			desc->mYpos = sc.Number;
			if (sc.CheckString(","))
			{
				sc.MustGetNumber();
				desc->mYbotton = sc.Number;
			}
		}
		else if (sc.Compare("Centermenu"))
		{
			desc->mFlags |= LMF_Centered;
		}
		else if (sc.Compare("animatedtransition"))
		{
			desc->mFlags |= LMF_Animate;
		}
		else if (sc.Compare("Fixedspacing"))
		{
			sc.MustGetNumber();
			desc->mSpacing = sc.Number;
		}
		else if (sc.Compare("MouseWindow"))
		{
			sc.MustGetNumber();
			desc->mWLeft = sc.Number;
			sc.MustGetStringName(",");
			sc.MustGetNumber();
			desc->mWRight = sc.Number;
		}
		else if (sc.Compare("StaticPatch") || sc.Compare("StaticPatchCentered"))
		{
			bool centered = sc.Compare("StaticPatchCentered");
			sc.MustGetNumber();
			int x = sc.Number;
			sc.MustGetStringName(",");
			sc.MustGetNumber();
			int y = sc.Number;
			sc.MustGetStringName(",");
			sc.MustGetString();
			auto tex = GetMenuTexture(sc.String);

			FListMenuItem *it = new FListMenuItemStaticPatch(x, y, tex, centered);
			desc->mItems.Push(it);
		}
		else if (sc.Compare("ScriptId"))
		{
			sc.MustGetNumber();
			desc->mScriptId = sc.Number;
		}
		else if (sc.Compare("Caption"))
		{
			sc.MustGetString();
			desc->mCaption = sc.String;
		}
		else if (sc.Compare("StaticText") || sc.Compare("StaticTextCentered"))
		{
			bool centered = sc.Compare("StaticTextCentered");
			sc.MustGetNumber();
			int x = sc.Number;
			sc.MustGetStringName(",");
			sc.MustGetNumber();
			int y = sc.Number;
			sc.MustGetStringName(",");
			sc.MustGetString();
			FListMenuItem *it = new FListMenuItemStaticText(x, y, sc.String, desc->mFont, desc->mFontColor, centered);
			desc->mItems.Push(it);
		}
		else if (sc.Compare("PatchItem"))
		{
			sc.MustGetString();
			auto tex = GetMenuTexture(sc.String);
			sc.MustGetStringName(",");
			sc.MustGetString();
			int hotkey = sc.String[0];
			sc.MustGetStringName(",");
			sc.MustGetString();
			FName action = sc.String;
			int param = 0;
			if (sc.CheckString(","))
			{
				sc.MustGetNumber();
				param = sc.Number;
			}

			FListMenuItem *it = new FListMenuItemPatch(desc->mXpos, desc->mYpos, desc->mLinespacing, hotkey, tex, action, param);
			desc->mItems.Push(it);
			desc->mYpos += desc->mLinespacing;
			if (desc->mSelectedItem == -1) desc->mSelectedItem = desc->mItems.Size()-1;
		}
		else if (sc.Compare("TextItem"))
		{
			sc.MustGetString();
			FString text = sc.String;
			sc.MustGetStringName(",");
			sc.MustGetString();
			int hotkey = sc.String[0];
			sc.MustGetStringName(",");
			sc.MustGetString();
			FName action = sc.String;
			int param = 0;
			if (sc.CheckString(","))
			{
				sc.MustGetNumber();
				param = sc.Number;
			}

			FListMenuItem *it = new FListMenuItemText(desc->mXpos, desc->mYpos, desc->mLinespacing, hotkey, text, desc->mFont, desc->mFontColor, desc->mFontColor2, action, param);
			desc->mItems.Push(it);
			desc->mYpos += desc->mLinespacing;
			if (desc->mSelectedItem == -1) desc->mSelectedItem = desc->mItems.Size()-1;

		}
		else if (sc.Compare("NativeTextItem"))
		{
		sc.MustGetString();
		FString text = sc.String;
		sc.MustGetStringName(",");
		sc.MustGetString();
		int hotkey = sc.String[0];
		sc.MustGetStringName(",");
		sc.MustGetString();
		FName action = sc.String;
		int param = 0;
		if (sc.CheckString(","))
		{
			sc.MustGetNumber();
			param = sc.Number;
		}

		auto it = new FListMenuItemNativeText(desc->mXpos, desc->mYpos, desc->mLinespacing, hotkey, text, desc->mNativeFontNum, desc->mNativePalNum, desc->mNativeFontScale, action, param);
		desc->mItems.Push(it);
		desc->mYpos += desc->mLinespacing;
		if (desc->mSelectedItem == -1) desc->mSelectedItem = desc->mItems.Size() - 1;

		}
		else if (sc.Compare("Font"))
		{
		sc.MustGetString();
		FFont* newfont = V_GetFont(sc.String);
		if (newfont != NULL) desc->mFont = newfont;
		if (sc.CheckString(","))
		{
			sc.MustGetString();
			desc->mFontColor2 = desc->mFontColor = V_FindFontColor((FName)sc.String);
			if (sc.CheckString(","))
			{
				sc.MustGetString();
				desc->mFontColor2 = V_FindFontColor((FName)sc.String);
				}
			}
		else
		{
			desc->mFontColor = OptionSettings.mFontColor;
			desc->mFontColor2 = OptionSettings.mFontColorValue;
		}
		}
		else if (sc.Compare("NetgameMessage"))
		{
			sc.MustGetString();
			desc->mNetgameMessage = sc.String;
		}
		else
		{
			sc.ScriptError("Unknown keyword '%s'", sc.String);
		}
	}
}

//=============================================================================
//
//
//
//=============================================================================

static bool CheckCompatible(FMenuDescriptor *newd, FMenuDescriptor *oldd)
{
	/*if (oldd->mClass == NULL)*/ return true;
	//return oldd->mClass == newd->mClass;
}

static bool ReplaceMenu(FScanner &sc, FMenuDescriptor *desc)
{
	FMenuDescriptor **pOld = MenuDescriptors.CheckKey(desc->mMenuName);
	if (pOld != NULL && *pOld != NULL) 
	{
		if (CheckCompatible(desc, *pOld))
		{
			delete *pOld;
		}
		else
		{
			sc.ScriptMessage("Tried to replace menu '%s' with a menu of different type", desc->mMenuName.GetChars());
			return true;
		}
	}
	MenuDescriptors[desc->mMenuName] = desc;
	return false;
}

//=============================================================================
//
//
//
//=============================================================================

static void ParseListMenu(FScanner &sc)
{
	sc.MustGetString();

	FListMenuDescriptor *desc = new FListMenuDescriptor;
	desc->mType = MDESC_ListMenu;
	desc->mMenuName = sc.String;
	desc->mSelectedItem = -1;
	desc->mAutoselect = -1;
	desc->mSelectOfsX = DefaultListMenuSettings.mSelectOfsX;
	desc->mSelectOfsY = DefaultListMenuSettings.mSelectOfsY;
	desc->mSelector = DefaultListMenuSettings.mSelector;
	desc->mDisplayTop = DefaultListMenuSettings.mDisplayTop;
	desc->mXpos = DefaultListMenuSettings.mXpos;
	desc->mYpos = DefaultListMenuSettings.mYpos;
	desc->mLinespacing = DefaultListMenuSettings.mLinespacing;
	desc->mNetgameMessage = DefaultListMenuSettings.mNetgameMessage;
	desc->mFont = DefaultListMenuSettings.mFont;
	desc->mFontColor = DefaultListMenuSettings.mFontColor;
	desc->mFontColor2 = DefaultListMenuSettings.mFontColor2;
	desc->mClass = NULL;
	desc->mRedirect = NULL;
	desc->mWLeft = 0;
	desc->mWRight = 0;

	ParseListMenuBody(sc, desc);
	bool scratch = ReplaceMenu(sc, desc);
	if (scratch) delete desc;
}

//=============================================================================
//
//
//
//=============================================================================

static void ParseImageScrollerBody(FScanner &sc, FImageScrollerDescriptor *desc)
{
	sc.MustGetStringName("{");
	while (!sc.CheckString("}"))
	{
		sc.MustGetString();
		if (sc.Compare("else"))
		{
			SkipSubBlock(sc);
		}
		else if (sc.Compare("ifgame"))
		{
			if (!CheckSkipGameBlock(sc))
			{
				// recursively parse sub-block
				ParseImageScrollerBody(sc, desc);
			}
		}
		else if (sc.Compare("ifshareware"))
		{
			if (!CheckSkipNoSwBlock(sc))
			{
				// recursively parse sub-block
				ParseImageScrollerBody(sc, desc);
			}
		}
		else if (sc.Compare("ifoption"))
		{
			if (!CheckSkipOptionBlock(sc))
			{
				// recursively parse sub-block
				ParseImageScrollerBody(sc, desc);
			}
		}
		else if (sc.Compare("Class"))
		{
			sc.MustGetString();
			FString s = sc.String;
			s.Substitute("$", gi->Name());
			desc->mClass = s;
		}
		else if (sc.Compare("TextItem") || sc.Compare("ImageItem"))
		{
			FImageScrollerDescriptor::ScrollerItem item;
			int type = sc.Compare("TextItem");
			sc.MustGetString();
			item.text = strbin1(sc.String);
			if (type)
			{
				sc.MustGetStringName(",");
				sc.MustGetNumber();
				item.type = sc.Number; // y-coordinate
			}
			else item.type = 0;
			item.scriptID = INT_MAX;
			if (sc.CheckString(","))
			{
				sc.MustGetNumber();
				item.scriptID = sc.Number;
			}
			desc->mItems.Push(item);
		}
		else if (sc.Compare("qavanimationitem"))
		{
			if (!(g_gameType & GAMEFLAG_BLOOD))
			{
				I_Error("QAV animations not available!"); // these (currently) only exist in Blood.
			}
			FImageScrollerDescriptor::ScrollerItem item;
			sc.GetString();
			item.text = sc.String;
			item.type = -1;
			item.scriptID = INT_MAX;
			desc->mItems.Push(item);
		}
		else if (sc.Compare("animatedtransition"))
		{
			desc->mFlags |= LMF_Animate;
		}
		else
		{
			sc.ScriptError("Unknown keyword '%s'", sc.String);
		}
	}
}


//=============================================================================
//
//
//
//=============================================================================

static void ParseImageScroller(FScanner &sc)
{
	sc.MustGetString();

	FImageScrollerDescriptor *desc = new FImageScrollerDescriptor;
	desc->mType = MDESC_ImageScroller;
	desc->mMenuName = sc.String;
	desc->mClass = NAME_None;

	ParseImageScrollerBody(sc, desc);
	bool scratch = ReplaceMenu(sc, desc);
	if (scratch) delete desc;
}

//=============================================================================
//
//
//
//=============================================================================

static void ParseOptionValue(FScanner &sc)
{
	FName optname;

	FOptionValues *val = new FOptionValues;
	sc.MustGetString();
	optname = sc.String;
	sc.MustGetStringName("{");
	while (!sc.CheckString("}"))
	{
		FOptionValues::Pair &pair = val->mValues[val->mValues.Reserve(1)];
		sc.MustGetFloat();
		pair.Value = sc.Float;
		sc.MustGetStringName(",");
		sc.MustGetString();
		pair.Text = strbin1(sc.String);
	}
	FOptionValues **pOld = OptionValues.CheckKey(optname);
	if (pOld != NULL && *pOld != NULL) 
	{
		delete *pOld;
	}
	OptionValues[optname] = val;
}


//=============================================================================
//
//
//
//=============================================================================

static void ParseOptionString(FScanner &sc)
{
	FName optname;

	FOptionValues *val = new FOptionValues;
	sc.MustGetString();
	optname = sc.String;
	sc.MustGetStringName("{");
	while (!sc.CheckString("}"))
	{
		FOptionValues::Pair &pair = val->mValues[val->mValues.Reserve(1)];
		sc.MustGetString();
		pair.Value = DBL_MAX;
		pair.TextValue = sc.String;
		sc.MustGetStringName(",");
		sc.MustGetString();
		pair.Text = strbin1(sc.String);
	}
	FOptionValues **pOld = OptionValues.CheckKey(optname);
	if (pOld != NULL && *pOld != NULL) 
	{
		delete *pOld;
	}
	OptionValues[optname] = val;
}


//=============================================================================
//
//
//
//=============================================================================

static void ParseOptionSettings(FScanner &sc)
{
	sc.MustGetStringName("{");
	while (!sc.CheckString("}"))
	{
		sc.MustGetString();
		if (sc.Compare("else"))
		{
			SkipSubBlock(sc);
		}
		else if (sc.Compare("ifgame"))
		{
			if (!CheckSkipGameBlock(sc))
			{
				// recursively parse sub-block
				ParseOptionSettings(sc);
			}
		}
		else if (sc.Compare("Linespacing"))
		{
			sc.MustGetNumber();
			OptionSettings.mLinespacing = sc.Number;
		}
		else if (sc.Compare("LabelOffset"))
		{
			sc.MustGetNumber();
			// ignored
		}
		else
		{
			sc.ScriptError("Unknown keyword '%s'", sc.String);
		}
	}
}

//=============================================================================
//
//
//
//=============================================================================

static void ParseOptionMenuBody(FScanner &sc, FOptionMenuDescriptor *desc)
{
	sc.MustGetStringName("{");
	while (!sc.CheckString("}"))
	{
		sc.MustGetString();
		if (sc.Compare("else"))
		{
			SkipSubBlock(sc);
		}
		else if (sc.Compare("ifgame"))
		{
			if (!CheckSkipGameBlock(sc))
			{
				// recursively parse sub-block
				ParseOptionMenuBody(sc, desc);
			}
		}
		else if (sc.Compare("ifshareware"))
		{
			if (!CheckSkipNoSwBlock(sc))
			{
				// recursively parse sub-block
				ParseOptionMenuBody(sc, desc);
			}
		}
		else if (sc.Compare("ifoption"))
		{
			if (!CheckSkipOptionBlock(sc))
			{
				// recursively parse sub-block
				ParseOptionMenuBody(sc, desc);
			}
		}
		else if (sc.Compare("Class"))
		{
			sc.MustGetString();
			FString s = sc.String;
			s.Substitute("$", gi->Name());
			desc->mClass = s;
		}
		else if (sc.Compare("Title") || sc.Compare("Caption"))
		{
			sc.MustGetString();
			desc->mTitle = sc.String;
		}
		else if (sc.Compare("Position"))
		{
			sc.MustGetNumber();
			desc->mPosition = sc.Number;
		}
		else if (sc.Compare("DefaultSelection"))
		{
			sc.MustGetNumber();
			desc->mSelectedItem = sc.Number;
		}
		else if (sc.Compare("ScrollTop"))
		{
			sc.MustGetNumber();
			desc->mScrollTop = sc.Number;
		}
		else if (sc.Compare("Indent"))
		{
			sc.MustGetNumber();
			desc->mIndent = sc.Number;
		}
		else if (sc.Compare("Submenu"))
		{
			sc.MustGetString();
			FString label = sc.String;
			sc.MustGetStringName(",");
			sc.MustGetString();
			FOptionMenuItem *it = new FOptionMenuItemSubmenu(label, sc.String);
			desc->mItems.Push(it);
		}
		else if (sc.Compare("LabeledSubmenu"))
		{
			sc.MustGetString();
			FString label = sc.String;
			sc.MustGetStringName(",");
			sc.MustGetString();
			auto cvar = FindCVar(sc.String, nullptr);
			sc.MustGetStringName(",");
			sc.MustGetString();
			FOptionMenuItem* it = new FOptionMenuItemLabeledSubmenu(label, cvar, sc.String);
			desc->mItems.Push(it);
		}
		else if (sc.Compare("Option"))
		{
			sc.MustGetString();
			FString label = sc.String;
			sc.MustGetStringName(",");
			sc.MustGetString();
			FString cvar = sc.String;
			sc.MustGetStringName(",");
			sc.MustGetString();
			FString values = sc.String;
			FString check;
			int center = 0;
			if (sc.CheckString(","))
			{
				sc.MustGetString();
				if (*sc.String != 0) check = sc.String;
				if (sc.CheckString(","))
				{
					sc.MustGetNumber();
					center = sc.Number;
				}
			}
			FOptionMenuItem *it = new FOptionMenuItemOption(label, cvar, values, check, center);
			desc->mItems.Push(it);
		}
		else if (sc.Compare("Command"))
		{
			sc.MustGetString();
			FString label = sc.String;
			sc.MustGetStringName(",");
			sc.MustGetString();
			FOptionMenuItem *it = new FOptionMenuItemCommand(label, sc.String);
			desc->mItems.Push(it);
		}
		else if (sc.Compare("SafeCommand"))
		{
			sc.MustGetString();
			FString label = sc.String;
			sc.MustGetStringName(",");
			sc.MustGetString();
			FOptionMenuItem *it = new FOptionMenuItemSafeCommand(label, sc.String);
			desc->mItems.Push(it);
		}
		else if (sc.Compare("Control") || sc.Compare("MapControl"))
		{
			bool map = sc.Compare("MapControl");
			sc.MustGetString();
			FString label = sc.String;
			sc.MustGetStringName(",");
			sc.MustGetString();
			FOptionMenuItem *it = new FOptionMenuItemControl(label, sc.String, map? &AutomapBindings : &Bindings);
			desc->mItems.Push(it);
		}
		else if (sc.Compare("StaticText"))
		{
			sc.MustGetString();
			FString label = sc.String;
			EColorRange cr = OptionSettings.mFontColorHeader;
			if (sc.CheckString(","))
			{
				sc.MustGetNumber();
				cr = sc.Number? OptionSettings.mFontColorHeader : OptionSettings.mFontColor;	// fixme!
			}
			FOptionMenuItem *it = new FOptionMenuItemStaticText(label, cr);
			desc->mItems.Push(it);
		}
		else if (sc.Compare("StaticTextSwitchable"))
		{
			sc.MustGetString();
			FString label = sc.String;
			sc.MustGetStringName(",");
			sc.MustGetString();
			FString label2 = sc.String;
			sc.MustGetStringName(",");
			sc.MustGetString();
			FName action = sc.String;
			EColorRange cr = OptionSettings.mFontColorHeader;
			if (sc.CheckString(","))
			{
				sc.MustGetNumber();
				cr = sc.Number ? OptionSettings.mFontColorHeader : OptionSettings.mFontColor;	// fixme!
			}
			FOptionMenuItem *it = new FOptionMenuItemStaticTextSwitchable(label, label2, action, cr);
			desc->mItems.Push(it);
		}
		else if (sc.Compare("Slider"))
		{
			sc.MustGetString();
			FString text = sc.String;
			sc.MustGetStringName(",");
			sc.MustGetString();
			FString action = sc.String;
			sc.MustGetStringName(",");
			sc.MustGetFloat();
			double min = sc.Float;
			sc.MustGetStringName(",");
			sc.MustGetFloat();
			double max = sc.Float;
			sc.MustGetStringName(",");
			sc.MustGetFloat();
			double step = sc.Float;
			int showvalue = 1;
			if (sc.CheckString(","))
			{
				sc.MustGetNumber();
				showvalue = sc.Number;
			}
			FOptionMenuItem *it = new FOptionMenuSliderCVar(text, action, min, max, step, showvalue);
			desc->mItems.Push(it);
		}
		// [TP] -- Text input widget
		else if ( sc.Compare( "TextField" ))
		{
			sc.MustGetString();
			FString label = sc.String;
			sc.MustGetStringName( "," );
			sc.MustGetString();
			FString cvar = sc.String;
			FString check;
			
			if ( sc.CheckString( "," ))
			{
				sc.MustGetString();
				check = sc.String;
			}

			FOptionMenuItem* it = new FOptionMenuTextField( label, cvar, check );
			desc->mItems.Push( it );
		}
		// [TP] -- Number input widget
		else if ( sc.Compare( "NumberField" ))
		{
			sc.MustGetString();
			FString label = sc.String;
			sc.MustGetStringName( "," );
			sc.MustGetString();
			FString cvar = sc.String;
			float minimum = 0.0f;
			float maximum = 100.0f;
			float step = 1.0f;
			FString check;

			if ( sc.CheckString( "," ))
			{
				sc.MustGetFloat();
				minimum = (float) sc.Float;
				sc.MustGetStringName( "," );
				sc.MustGetFloat();
				maximum = (float) sc.Float;

				if ( sc.CheckString( "," ))
				{
					sc.MustGetFloat();
					step = (float) sc.Float;

					if ( sc.CheckString( "," ))
					{
						sc.MustGetString();
						check = sc.String;
					}
				}
			}

			FOptionMenuItem* it = new FOptionMenuNumberField( label, cvar,
				minimum, maximum, step, check );
			desc->mItems.Push( it );
		}
		else
		{
			sc.ScriptError("Unknown keyword '%s'", sc.String);
		}
	}
}

//=============================================================================
//
//
//
//=============================================================================

static void ParseOptionMenu(FScanner &sc)
{
	sc.MustGetString();

	FOptionMenuDescriptor *desc = new FOptionMenuDescriptor;
	desc->mType = MDESC_OptionsMenu;
	desc->mMenuName = sc.String;
	desc->mSelectedItem = -1;
	desc->mScrollPos = 0;
	desc->mClass = NULL;
	desc->mPosition = DefaultOptionMenuSettings.mPosition;
	desc->mScrollTop = DefaultOptionMenuSettings.mScrollTop;
	desc->mIndent =  DefaultOptionMenuSettings.mIndent;
	desc->mDontDim =  DefaultOptionMenuSettings.mDontDim;

	ParseOptionMenuBody(sc, desc);
	bool scratch = ReplaceMenu(sc, desc);
	if (scratch) delete desc;
}


//=============================================================================
//
//
//
//=============================================================================

void M_ParseMenuDefs()
{
	int lump, lastlump = 0;

	//OptionSettings.mTitleColor = CR_RED;// V_FindFontColor(gameinfo.mTitleColor);
	OptionSettings.mFontColor = CR_RED;
	OptionSettings.mFontColorValue = CR_GRAY;
	OptionSettings.mFontColorMore = CR_GRAY;
	OptionSettings.mFontColorHeader = CR_GOLD;
	OptionSettings.mFontColorHighlight = CR_YELLOW;
	OptionSettings.mFontColorSelection = CR_BRICK;

	if (g_gameType & (GAMEFLAG_NAM | GAMEFLAG_NAPALM | GAMEFLAG_WW2GI))
	{
		OptionSettings.mFontColor = CR_DARKGREEN;
		OptionSettings.mFontColorHeader = CR_DARKGRAY;
		OptionSettings.mFontColorHighlight = CR_WHITE;
		OptionSettings.mFontColorSelection = CR_DARKGREEN;
	}
	else if (g_gameType & GAMEFLAG_BLOOD)
	{
		OptionSettings.mFontColorHeader = CR_DARKGRAY;
		OptionSettings.mFontColorHighlight = CR_WHITE;
		OptionSettings.mFontColorSelection = CR_DARKRED;
	}
	else if (g_gameType & GAMEFLAG_FURY)
	{
		OptionSettings.mFontColor = CR_TEAL;
		OptionSettings.mFontColorHeader = CR_LIGHTBLUE;
		OptionSettings.mFontColorHighlight = CR_ORANGE;
		OptionSettings.mFontColorSelection = CR_GOLD;
	}
	else if (g_gameType & GAMEFLAG_RRALL)
	{
		OptionSettings.mFontColor = CR_BROWN;
		OptionSettings.mFontColorHeader = CR_DARKBROWN;
		OptionSettings.mFontColorHighlight = CR_ORANGE;
		OptionSettings.mFontColorSelection = CR_TAN;
	}
	else if (g_gameType & GAMEFLAG_SW)
	{
		OptionSettings.mFontColorHeader = CR_DARKRED;
		OptionSettings.mFontColorHighlight = CR_WHITE;
	}

	DefaultListMenuSettings.Reset();
	DefaultOptionMenuSettings.Reset();

	M_DeinitMenus();
	while ((lump = fileSystem.FindLumpFullName("engine/menudef.txt", &lastlump)) != -1)
	{
		FScanner sc(lump);

		sc.SetCMode(true);
		while (sc.GetString())
		{
			if (sc.Compare("LISTMENU"))
			{
				ParseListMenu(sc);
			}
			else if (sc.Compare("ImageScroller"))
			{
				ParseImageScroller(sc);
			}
			else if (sc.Compare("DEFAULTLISTMENU"))
			{
				ParseListMenuBody(sc, &DefaultListMenuSettings);
				if (DefaultListMenuSettings.mItems.Size() > 0)
				{
					I_Error("You cannot add menu items to the menu default settings.");
				}
			}
			else if (sc.Compare("OPTIONVALUE"))
			{
				ParseOptionValue(sc);
			}
			else if (sc.Compare("OPTIONSTRING"))
			{
				ParseOptionString(sc);
			}
			else if (sc.Compare("OPTIONMENUSETTINGS"))
			{
				ParseOptionSettings(sc);
			}
			else if (sc.Compare("OPTIONMENU"))
			{
				ParseOptionMenu(sc);
			}
			else if (sc.Compare("DEFAULTOPTIONMENU"))
			{
				ParseOptionMenuBody(sc, &DefaultOptionMenuSettings);
				if (DefaultOptionMenuSettings.mItems.Size() > 0)
				{
					I_Error("You cannot add menu items to the menu default settings.");
				}
			}
			else
			{
				sc.ScriptError("Unknown keyword '%s'", sc.String);
			}
		}
	}
}


//=============================================================================
//
// Unlocks a custom menu from a script
//
//=============================================================================

void M_UnhideCustomMenu(int menu, int iSet)
{
	FName menuname = FName(ENamedName(NAME_CustomSubMenu1 + menu));
	auto desc = MenuDescriptors.CheckKey(menuname);
	if (desc != NULL && (*desc)->mType == MDESC_ListMenu)
	{
		FListMenuDescriptor* ld = static_cast<FListMenuDescriptor*>(*desc);
		for (unsigned int b = 0; b < MAXMENUGAMEPLAYENTRIES; ++b)
		{
			if (b >= ld->mItems.Size()) return;
			if (iSet & (1u << b))
			{
				ld->mItems[b]->mHidden = false;
				ld->mItems[b]->mEnabled = true;
			}
		}
	}
}

//=============================================================================
//
// Creates the episode menu
//
//=============================================================================

static void BuildEpisodeMenu()
{
	// Build episode menu
	int addedVolumes = 0;
	FMenuDescriptor **desc = MenuDescriptors.CheckKey(NAME_Episodemenu);
	if (desc != NULL && (*desc)->mType == MDESC_ListMenu)
	{
		FListMenuDescriptor *ld = static_cast<FListMenuDescriptor*>(*desc);
		ld->mSelectedItem = gDefaultVolume;
		int y = ld->mYpos;

		for (int i = 0; i < MAXVOLUMES; i++)
		{
			if (gVolumeNames[i].IsNotEmpty() && !(gVolumeFlags[i] & EF_HIDEFROMSP))

			{
				auto it = new FListMenuItemNativeText(ld->mXpos, y, ld->mLinespacing, gVolumeNames[i][0], gVolumeNames[i], NIT_BigFont, NIT_ActiveState, 1, NAME_Skillmenu, i);
				if ((g_gameType & GAMEFLAG_DUKE) && (g_gameType & GAMEFLAG_SHAREWARE) && i > 0)
				{
					it->mEnabled = false;
				}
				y += ld->mLinespacing;
				ld->mItems.Push(it);
				addedVolumes++;
				if (gVolumeSubtitles[i].IsNotEmpty())
				{
					auto it = new FListMenuItemNativeStaticText(ld->mXpos, y, gVolumeSubtitles[i], NIT_SmallFont, NIT_ActiveState, false);
					y += ld->mLinespacing * 6 / 10;
					ld->mItems.Push(it);
				}
			}
		}
#if 0	// this needs to be backed by a working selection menu, until that gets done it must be disabled.
		if (!(g_gameType & GAMEFLAG_SHAREWARE))
		{
			//auto it = new FListMenuItemNativeStaticText(ld->mXpos, "", NIT_SmallFont);	// empty entry as spacer.
			//ld->mItems.Push(it);

			y += ld->mLinespacing / 3;
			auto it = new FListMenuItemNativeText(ld->mXpos, y, ld->mLineSpacing, 0, "$MNU_USERMAP", NIT_BigFont, NIT_ActiveState, 1, NAME_UsermapMenu);
			ld->mItems.Push(it);
			addedVolumes++;
		}
#endif
		if (addedVolumes == 1)
		{
			ld->mAutoselect = 0;
		}
	}

	// Build skill menu
	int addedSkills = 0;
	desc = MenuDescriptors.CheckKey(NAME_Skillmenu);
	if (desc != NULL && (*desc)->mType == MDESC_ListMenu)
	{
		FListMenuDescriptor* ld = static_cast<FListMenuDescriptor*>(*desc);
		ld->mSelectedItem = gDefaultSkill;
		int y = ld->mYpos;

		for (int i = 0; i < MAXSKILLS; i++)
		{
			if (gSkillNames[i].IsNotEmpty())
			{
				auto it = new FListMenuItemNativeText(ld->mXpos, y, ld->mLinespacing, gSkillNames[i][0], gSkillNames[i], NIT_BigFont, NIT_ActiveState, 1, NAME_Startgame, i);
				y += ld->mLinespacing;
				ld->mItems.Push(it);
				addedSkills++;
			}
		}
		if (addedSkills == 0)
		{
			// Need to add one item with the default skill so that the menu does not break.
			auto it = new FListMenuItemNativeText(ld->mXpos, 0, ld->mLinespacing, 0, "", NIT_BigFont, NIT_ActiveState, 1, NAME_Startgame, gDefaultSkill);
			ld->mItems.Push(it);
		}
		if (addedSkills == 1)
		{
			ld->mAutoselect = 0;
		}
	}

	if (g_MenuGameplayEntries[0].entry.isValid())
	{
		int e = 0;
		FMenuDescriptor** desc = MenuDescriptors.CheckKey("CustomGameMenu");
		if (desc != NULL && (*desc)->mType == MDESC_ListMenu)
		{
			FListMenuDescriptor* ldo = static_cast<FListMenuDescriptor*>(*desc);

			for (MenuGameplayStemEntry const& stem : g_MenuGameplayEntries)
			{
				MenuGameplayEntry const& entry = stem.entry;
				if (!entry.isValid())
					break;

				int s = 0;
				FMenuDescriptor** sdesc = MenuDescriptors.CheckKey(FName(FStringf("CustomSubMenu%d", e+1)));
				if (sdesc != NULL && (*sdesc)->mType == MDESC_ListMenu)
				{
					FListMenuDescriptor* ld = static_cast<FListMenuDescriptor*>(*sdesc);
					ld->mCaption = entry.name;

					for (MenuGameplayEntry const& subentry : stem.subentries)
					{
						if (!subentry.isValid())
							break;

						auto li = new FListMenuItemNativeText(ld->mXpos, 0, ld->mLinespacing, 0, subentry.name, NIT_BigFont, NIT_ActiveColor, 1.f, subentry.flags & MGE_UserContent ? NAME_UsermapMenu : NAME_Skillmenu);

						if (subentry.flags & MGE_Locked) li->mEnabled = false;
						if (subentry.flags & MGE_Hidden) li->mHidden = true;
						ld->mItems.Push(li);
						++s;
					}
				}
				FName link = entry.flags & MGE_UserContent ? NAME_UsermapMenu : s == 0 ? NAME_Skillmenu : NAME_CustomSubMenu1;

				auto li = new FListMenuItemNativeText(ldo->mXpos, 0, ldo->mLinespacing, 0, entry.name, NIT_BigFont, NIT_ActiveColor, 1.f, link, e);
				if (entry.flags & MGE_Locked) li->mEnabled = false;
				if (entry.flags & MGE_Hidden) li->mHidden = true;
				ldo->mItems.Push(li);
				e++;
			}
		}
		if (e > 0)
		{
			for (auto name : { NAME_Mainmenu, NAME_IngameMenu })
			{
				FMenuDescriptor** desc = MenuDescriptors.CheckKey(name);
				if (desc != NULL && (*desc)->mType == MDESC_ListMenu)
				{
					FListMenuDescriptor* ld = static_cast<FListMenuDescriptor*>(*desc);
					auto li = ld->mItems[0];
					if (li->GetAction(nullptr) == NAME_Episodemenu)
					{
						li->SetAction(NAME_CustomGameMenu);
					}
				}
			}
		}
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
#if 0
	int lastlump, lump;

	lastlump = 0;

	FOptionValues **opt = OptionValues.CheckKey(NAME_Crosshairs);
	if (opt == NULL) 
	{
		return;	// no crosshair value list present. No need to go on.
	}

	FOptionValues::Pair *pair = &(*opt)->mValues[(*opt)->mValues.Reserve(1)];
	pair->Value = 0;
	pair->Text = "None";

	while ((lump = Wads.FindLump("XHAIRS", &lastlump)) != -1)
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
#endif
}

//=============================================================================
//
// Initialize the music configuration submenus
//
//=============================================================================
extern "C"
{
	extern int adl_getBanksCount();
	extern const char* const* adl_getBankNames();
}

static void InitMusicMenus()
{
	FMenuDescriptor** advmenu = MenuDescriptors.CheckKey(NAME_AdvSoundOptions);
	auto soundfonts = sfmanager.GetList();
	std::tuple<const char*, int, const char*> sfmenus[] = { 
																std::make_tuple("FluidPatchsetMenu", SF_SF2, "fluid_patchset") };

	for (auto& p : sfmenus)
	{
		FMenuDescriptor** menu = MenuDescriptors.CheckKey(std::get<0>(p));

		if (menu != nullptr)
		{
			if (soundfonts.Size() > 0)
			{
				for (auto& entry : soundfonts)
				{
					if (entry.type & std::get<1>(p))
					{
						FString display = entry.mName;
						display.ReplaceChars("_", ' ');
						auto it = new FOptionMenuItemCommand (display, FStringf("%s \"%s\"", std::get<2>(p), entry.mName.GetChars())/*, true*/);
						static_cast<FOptionMenuDescriptor*>(*menu)->mItems.Push(it);
					}
				}
			}
			else if (advmenu != nullptr)
			{
				// Remove the item for this submenu
				auto d = static_cast<FOptionMenuDescriptor*>(*advmenu);
				auto it = d->GetItem(std::get<0>(p));
				if (it != nullptr) d->mItems.Delete(d->mItems.Find(it));
			}
		}
	}
}

//=============================================================================
//
// Special menus will be created once all engine data is loaded
//
//=============================================================================

void M_CreateMenus()
{
	BuildEpisodeMenu();
	InitCrosshairsList();
	InitMusicMenus();

	FOptionValues** opt = OptionValues.CheckKey(NAME_Mididevices);
	if (opt != nullptr)
	{
		I_BuildMIDIMenuList(*opt);
	}
#ifndef NO_OPENAL
	opt = OptionValues.CheckKey(NAME_Aldevices);
	if (opt != nullptr)
	{
		I_BuildALDeviceList(*opt);
	}
	opt = OptionValues.CheckKey(NAME_Alresamplers);
	if (opt != nullptr)
	{
		I_BuildALResamplersList(*opt);
	}
#endif // !NO_OPENAL
}

//=============================================================================
//
// Returns the default skill level.
//
//=============================================================================

int M_GetDefaultSkill()
{
	return gDefaultSkill;
}

void FListMenuDescriptor::Reset()
{
	// Reset the default settings (ignore all other values in the struct)
	mSelectOfsX = 0;
	mSelectOfsY = 0;
	mSelector = nullptr;
	mDisplayTop = 0;
	mXpos = 0;
	mYpos = 0;
	mLinespacing = 0;
	mNetgameMessage = "";
	mFont = NULL;
	mFontColor = CR_UNTRANSLATED;
	mFontColor2 = CR_UNTRANSLATED;
	mScriptId = -1;
	mSecondaryId = 0;
	mNativeFontNum = NIT_BigFont;
	mNativePalNum = NIT_ActiveColor;
	mNativeFontScale = 1.f;
	mFlags = 0;
	mSpacing = 0;
}
