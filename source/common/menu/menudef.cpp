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

#include "optionmenuitems.h"

void ClearSaveGames();

MenuDescriptorList MenuDescriptors;
static FListMenuDescriptor DefaultListMenuSettings;	// contains common settings for all list menus
static FOptionMenuDescriptor DefaultOptionMenuSettings;	// contains common settings for all Option menus
FOptionMenuSettings OptionSettings;
FOptionMap OptionValues;

void I_BuildALDeviceList(FOptionValues *opt);

static void DeinitMenus()
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
	DMenu::CurrentMenu = NULL;
	DefaultListMenuSettings.mItems.Clear();
	ClearSaveGames();
}

static FTexture* GetMenuTexture(const char* const name)
{
	auto tex = TileFiles.GetTexture(name);

	if (!tex)
	{
		Printf("Missing menu texture: \"%s\"\n", name);
	}

	return tex;
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
	{ "WW2GI", GAMEFLAG_WW2GI},
	{ "Fury", GAMEFLAG_FURY},
	{ "Redneck", GAMEFLAG_RR},
	{ "RedneckRides", GAMEFLAG_RRRA},
	{ "Blood", GAMEFLAG_BLOOD},
	{ "ShadowWarrior", GAMEFLAG_SW},
};


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
	if (!(filter & 1))	// todo: apply correct filter.
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
		else if (sc.Compare("Class"))
		{
			sc.MustGetString();
			desc->mClass = sc.String;
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
		}
		else if (sc.Compare("Centermenu"))
		{
			desc->mCenter = true;
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
			FTexture* tex = GetMenuTexture(sc.String);

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
			FTexture* tex = GetMenuTexture(sc.String);
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
		else if (sc.Compare("NativeFont"))
		{
			desc->mNativePalNum = NIT_ActiveColor;
			desc->mNativeFontScale = 1.f;
			sc.MustGetString();
			if (sc.Compare("Big")) desc->mNativeFontNum = NIT_BigFont;
			else if (sc.Compare("Small")) desc->mNativeFontNum = NIT_SmallFont;
			else if (sc.Compare("Tiny")) desc->mNativeFontNum = NIT_TinyFont;
			else sc.ScriptError("Unknown native font type");
			if (sc.CheckString(","))
			{
				sc.MustGetString();
				if (sc.Compare("Active")) desc->mNativePalNum = NIT_ActiveColor;
				else if (sc.Compare("Inactive")) desc->mNativePalNum = NIT_InactiveColor;
				else if (sc.Compare("Selected")) desc->mNativePalNum = NIT_SelectedColor;
				else
				{
					char* ep;
					int v = (int)strtoll(sc.String, &ep, 0);
					if (*ep != 0) sc.ScriptError("Unknown native palette");
					desc->mNativePalNum = v;
				}
				if (sc.CheckString(","))
				{
					sc.MustGetFloat();
					desc->mNativeFontScale = sc.Float;
				}			
			}
		}
		else if (sc.Compare("Position"))
		{
			sc.MustGetNumber();
			sc.MustGetStringName(",");
			desc->mXpos = sc.Number;
			sc.MustGetNumber();
			desc->mYpos = sc.Number;
			if (sc.CheckString(","))
			{
				desc->mCenter = sc.Number;
			}
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
#if 0
		else if (sc.Compare("PlayerDisplay"))
		{
			bool noportrait = false;
			FName action = NAME_None;
			sc.MustGetNumber();
			int x = sc.Number;
			sc.MustGetStringName(",");
			sc.MustGetNumber();
			int y = sc.Number;
			sc.MustGetStringName(",");
			sc.MustGetString();
			PalEntry c1 = V_GetColor(NULL, sc.String);
			sc.MustGetStringName(",");
			sc.MustGetString();
			PalEntry c2 = V_GetColor(NULL, sc.String);
			if (sc.CheckString(","))
			{
				sc.MustGetNumber();
				noportrait = !!sc.Number;
				if (sc.CheckString(","))
				{
					sc.MustGetString();
					action = sc.String;
				}
			}
			FListMenuItemPlayerDisplay *it = new FListMenuItemPlayerDisplay(desc, x, y, c1, c2, noportrait, action);
			desc->mItems.Push(it);
		}
#endif
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
	desc->mCenter = false;

	ParseListMenuBody(sc, desc);
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
			desc->mClass = sc.String;
		}
		else if (sc.Compare("Title"))
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
			bool cr = false;
			if (sc.CheckString(","))
			{
				sc.MustGetNumber();
				cr = !!sc.Number;
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
			bool cr = false;
			if (sc.CheckString(","))
			{
				sc.MustGetNumber();
				cr = !!sc.Number;
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
	if (desc->mIndent == 0) desc->CalcIndent();
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

	OptionSettings.mTitleColor = CR_RED;// V_FindFontColor(gameinfo.mTitleColor);
	OptionSettings.mFontColor = CR_RED; // V_FindFontColor(gameinfo.mFontColor);
	OptionSettings.mFontColorValue = CR_GRAY;// = V_FindFontColor(gameinfo.mFontColorValue);
	OptionSettings.mFontColorMore = CR_GOLD;// = V_FindFontColor(gameinfo.mFontColorMore);
	OptionSettings.mFontColorHeader = CR_YELLOW;// = V_FindFontColor(gameinfo.mFontColorHeader);
	OptionSettings.mFontColorHighlight = CR_BRICK;// = V_FindFontColor(gameinfo.mFontColorHighlight);
	OptionSettings.mFontColorSelection = CR_DARKRED;// = V_FindFontColor(gameinfo.mFontColorSelection);
	DefaultListMenuSettings.Reset();
	DefaultOptionMenuSettings.Reset();

	atexit(DeinitMenus);
	DeinitMenus();
	while ((lump = fileSystem.Iterate("demolition/menudef.txt", &lastlump)) != -1)
	{
		FScanner sc(lump);

		sc.SetCMode(true);
		while (sc.GetString())
		{
			if (sc.Compare("LISTMENU"))
			{
				ParseListMenu(sc);
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
// Creates the episode menu
// Falls back on an option menu if there's not enough screen space to show all episodes
//
//=============================================================================

static void BuildEpisodeMenu()
{
#if 0
	// Build episode menu
	bool success = false;
	FMenuDescriptor **desc = MenuDescriptors.CheckKey(NAME_Episodemenu);
	if (desc != NULL)
	{
		if ((*desc)->mType == MDESC_ListMenu)
		{
			FListMenuDescriptor *ld = static_cast<FListMenuDescriptor*>(*desc);
			int posy = ld->mYpos;
			int topy = posy;

			// Get lowest y coordinate of any static item in the menu
			for(unsigned i = 0; i < ld->mItems.Size(); i++)
			{
				int y = ld->mItems[i]->GetY();
				if (y < topy) topy = y;
			}

			// center the menu on the screen if the top space is larger than the bottom space
			int totalheight = posy + AllEpisodes.Size() * ld->mLinespacing - topy;

			if (totalheight < 190 || AllEpisodes.Size() == 1)
			{
				int newtop = (200 - totalheight + topy) / 2;
				int topdelta = newtop - topy;
				if (topdelta < 0)
				{
					for(unsigned i = 0; i < ld->mItems.Size(); i++)
					{
						ld->mItems[i]->OffsetPositionY(topdelta);
					}
					posy -= topdelta;
				}

				ld->mSelectedItem = ld->mItems.Size();
				for(unsigned i = 0; i < AllEpisodes.Size(); i++)
				{
					FListMenuItem *it;
					if (AllEpisodes[i].mPicName.IsNotEmpty())
					{
						FTextureID tex = GetMenuTexture(AllEpisodes[i].mPicName);
						it = new FListMenuItemPatch(ld->mXpos, posy, ld->mLinespacing, AllEpisodes[i].mShortcut, 
							tex, NAME_Skillmenu, i);
					}
					else
					{
						it = new FListMenuItemText(ld->mXpos, posy, ld->mLinespacing, AllEpisodes[i].mShortcut, 
							AllEpisodes[i].mEpisodeName, ld->mFont, ld->mFontColor, ld->mFontColor2, NAME_Skillmenu, i);
					}
					ld->mItems.Push(it);
					posy += ld->mLinespacing;
				}
				if (AllEpisodes.Size() == 1)
				{
					ld->mAutoselect = ld->mSelectedItem;
				}
				success = true;
			}
		}
	}
	if (!success)
	{
		// Couldn't create the episode menu, either because there's too many episodes or some error occured
		// Create an option menu for episode selection instead.
		FOptionMenuDescriptor *od = new FOptionMenuDescriptor;
		if (desc != NULL) delete *desc;
		MenuDescriptors[NAME_Episodemenu] = od;
		od->mType = MDESC_OptionsMenu;
		od->mMenuName = NAME_Episodemenu;
		od->mTitle = "$MNU_EPISODE";
		od->mSelectedItem = 0;
		od->mScrollPos = 0;
		od->mClass = NULL;
		od->mPosition = -15;
		od->mScrollTop = 0;
		od->mIndent = 160;
		od->mDontDim = false;
		for(unsigned i = 0; i < AllEpisodes.Size(); i++)
		{
			FOptionMenuItemSubmenu *it = new FOptionMenuItemSubmenu(AllEpisodes[i].mEpisodeName, "Skillmenu", i);
			od->mItems.Push(it);
		}
	}
#endif
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
// Special menus will be created once all engine data is loaded
//
//=============================================================================

void M_CreateMenus()
{
	BuildEpisodeMenu();
	InitCrosshairsList();

#if 0
	FOptionValues **opt = OptionValues.CheckKey(NAME_Mididevices);
	if (opt != NULL) 
	{
		I_BuildMIDIMenuList(*opt);
	}
	opt = OptionValues.CheckKey(NAME_Aldevices);
	if (opt != NULL) 
	{
		I_BuildALDeviceList(*opt);
	}
#endif
}

//=============================================================================
//
// The skill menu must be refeshed each time it starts up
//
//=============================================================================
extern int restart;

void M_StartupSkillMenu(FGameStartup *gs)
{
#if 0
	static int done = -1;
	bool success = false;
	FMenuDescriptor **desc = MenuDescriptors.CheckKey(NAME_Skillmenu);
	if (desc != NULL)
	{
		if ((*desc)->mType == MDESC_ListMenu)
		{
			FListMenuDescriptor *ld = static_cast<FListMenuDescriptor*>(*desc);
			int x = ld->mXpos;
			int y = ld->mYpos;

			// Delete previous contents
			for(unsigned i=0; i<ld->mItems.Size(); i++)
			{
				FName n = ld->mItems[i]->GetAction(NULL);
				if (n == NAME_Startgame || n == NAME_StartgameConfirm) 
				{
					for(unsigned j=i; j<ld->mItems.Size(); j++)
					{
						delete ld->mItems[j];
					}
					ld->mItems.Resize(i);
					break;
				}
			}

			if (done != restart)
			{
				done = restart;
				int defskill = DefaultSkill;
				if ((unsigned int)defskill >= AllSkills.Size())
				{
					defskill = (AllSkills.Size() - 1) / 2;
				}
				ld->mSelectedItem = ld->mItems.Size() + defskill;

				int posy = y;
				int topy = posy;

				// Get lowest y coordinate of any static item in the menu
				for(unsigned i = 0; i < ld->mItems.Size(); i++)
				{
					int y = ld->mItems[i]->GetY();
					if (y < topy) topy = y;
				}

				// center the menu on the screen if the top space is larger than the bottom space
				int totalheight = posy + AllSkills.Size() * ld->mLinespacing - topy;

				if (totalheight < 190 || AllSkills.Size() == 1)
				{
					int newtop = (200 - totalheight + topy) / 2;
					int topdelta = newtop - topy;
					if (topdelta < 0)
					{
						for(unsigned i = 0; i < ld->mItems.Size(); i++)
						{
							ld->mItems[i]->OffsetPositionY(topdelta);
						}
						y = ld->mYpos = posy - topdelta;
					}
				}
				else
				{
					// too large
					delete ld;
					desc = NULL;
					done = false;
					goto fail;
				}
			}

			unsigned firstitem = ld->mItems.Size();
			for(unsigned int i = 0; i < AllSkills.Size(); i++)
			{
				FSkillInfo &skill = AllSkills[i];
				FListMenuItem *li;
				// Using a different name for skills that must be confirmed makes handling this easier.
				FName action = (skill.MustConfirm && !AllEpisodes[gs->Episode].mNoSkill) ?
					NAME_StartgameConfirm : NAME_Startgame;
				FString *pItemText = NULL;
				if (gs->PlayerClass != NULL)
				{
					pItemText = skill.MenuNamesForPlayerClass.CheckKey(gs->PlayerClass);
				}

				if (skill.PicName.Len() != 0 && pItemText == NULL)
				{
					FTextureID tex = GetMenuTexture(skill.PicName);
					li = new FListMenuItemPatch(ld->mXpos, y, ld->mLinespacing, skill.Shortcut, tex, action, i);
				}
				else
				{
					EColorRange color = (EColorRange)skill.GetTextColor();
					if (color == CR_UNTRANSLATED) color = ld->mFontColor;
					li = new FListMenuItemText(x, y, ld->mLinespacing, skill.Shortcut, 
									pItemText? *pItemText : skill.MenuName, ld->mFont, color,ld->mFontColor2, action, i);
				}
				ld->mItems.Push(li);
				y += ld->mLinespacing;
			}
			if (AllEpisodes[gs->Episode].mNoSkill || AllSkills.Size() == 1)
			{
				ld->mAutoselect = firstitem + M_GetDefaultSkill();
			}
			else
			{
				ld->mAutoselect = -1;
			}
			success = true;
		}
	}
	if (success) return;
fail:
	// Option menu fallback for overlong skill lists
	FOptionMenuDescriptor *od;
	if (desc == NULL)
	{
		od = new FOptionMenuDescriptor;
		if (desc != NULL) delete *desc;
		MenuDescriptors[NAME_Skillmenu] = od;
		od->mType = MDESC_OptionsMenu;
		od->mMenuName = NAME_Skillmenu;
		od->mTitle = "$MNU_CHOOSESKILL";
		od->mSelectedItem = 0;
		od->mScrollPos = 0;
		od->mClass = NULL;
		od->mPosition = -15;
		od->mScrollTop = 0;
		od->mIndent = 160;
		od->mDontDim = false;
	}
	else
	{
		od = static_cast<FOptionMenuDescriptor*>(*desc);
		for(unsigned i=0;i<od->mItems.Size(); i++)
		{
			delete od->mItems[i];
		}
		od->mItems.Clear();
	}
	for(unsigned int i = 0; i < AllSkills.Size(); i++)
	{
		FSkillInfo &skill = AllSkills[i];
		FOptionMenuItem *li;
		// Using a different name for skills that must be confirmed makes handling this easier.
		const char *action = (skill.MustConfirm && !AllEpisodes[gs->Episode].mNoSkill) ?
			"StartgameConfirm" : "Startgame";

		FString *pItemText = NULL;
		if (gs->PlayerClass != NULL)
		{
			pItemText = skill.MenuNamesForPlayerClass.CheckKey(gs->PlayerClass);
		}
		li = new FOptionMenuItemSubmenu(pItemText? *pItemText : skill.MenuName, action, i);
		od->mItems.Push(li);
		if (!done)
		{
			done = true;
			od->mSelectedItem = M_GetDefaultSkill();
		}
	}
#endif
}

//=============================================================================
//
// Returns the default skill level.
//
//=============================================================================

int M_GetDefaultSkill()
{
#if 0
	int defskill = DefaultSkill;
	if ((unsigned int)defskill >= AllSkills.Size())
	{
		defskill = (AllSkills.Size() - 1) / 2;
	}
	return defskill;
#else
	return 1;
#endif
}
