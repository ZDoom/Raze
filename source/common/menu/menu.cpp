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
#include "c_console.h"
#include "c_bind.h"
#include "c_cvars.h"
#include "d_event.h"
//#include "i_input.h"
#include "gameconfigfile.h"
#include "gstrings.h"
#include "menu.h"
#include "textures.h"
#include "c_buttons.h"
#include "v_2ddrawer.h"
#include "printf.h"
#include "v_draw.h"
#include "gamecontrol.h"
#include "fx_man.h"
#include "pragmas.h"
#include "build.h"

void RegisterDukeMenus();
extern bool rotatesprite_2doverride;
bool help_disabled, credits_disabled;
int g_currentMenu;	// accessible by CON scripts - contains the current menu's script ID if defined or INT_MAX if none given.
int DrawBackground;

//
// Todo: Move these elsewhere
//
CVAR (Float, mouse_sensitivity, 1.f, CVAR_ARCHIVE|CVAR_GLOBALCONFIG)
EXTERN_CVAR (Bool, show_messages)


CVAR (Float, snd_menuvolume, 0.6f, CVAR_ARCHIVE)
CVAR(Int, m_use_mouse, 1, CVAR_ARCHIVE|CVAR_GLOBALCONFIG)
CVAR(Int, m_show_backbutton, 0, CVAR_ARCHIVE|CVAR_GLOBALCONFIG)

TArray<MenuClassDescriptor*> menuClasses(TArray<MenuClassDescriptor*>::ENoInit(0));

DMenu *DMenu::CurrentMenu;
int DMenu::MenuTime;

FGameStartup GameStartupInfo;
EMenuState		menuactive;
bool			M_DemoNoPlay;
FButtonStatus	MenuButtons[NUM_MKEYS];
int				MenuButtonTickers[NUM_MKEYS];
bool			MenuButtonOrigin[NUM_MKEYS];
int				BackbuttonTime;         
float			BackbuttonAlpha;
static bool		MenuEnabled = true;


#define KEY_REPEAT_DELAY	(MENU_TICRATE*5/12)
#define KEY_REPEAT_RATE		(3)

enum MenuTransitionType
{ // Note: This enum is for logical categories, not visual types.
    MA_None,
    MA_Return,
    MA_Advance,
} ;

struct MenuTransition
{
    DMenu *previous;
    DMenu *current;

    int32_t start;
    int32_t length;
	int32_t dir;
};

static MenuTransition transition;

bool M_StartTransition(DMenu *from, DMenu *to, MenuTransitionType animtype, MenuTransition &transition)
{
	if (!from->canAnimate || !to->canAnimate || animtype == MA_None)
	{
		return false;
	}
	else
	{
		transition.start  = (int32_t) totalclock;
		transition.length = 30;
		transition.dir = animtype == MA_Advance? 1 : -1;
		transition.previous = from;
		transition.current  = to;
		return true;
	}
}

bool M_DrawTransition(MenuTransition &transition)
{
	if (totalclock < transition.start + transition.length)
	{
		double factor = 120 * xdim / ydim;
		double phase = ((int32_t) totalclock - transition.start) / double(transition.length) * M_PI + M_PI/2;
		
		transition.previous->origin.X = factor * transition.dir * (sin(phase) - 1.);
		transition.current->origin.X = factor * transition.dir * (sin(phase) + 1.);
		Printf("prev.X = %2.5f, next.X = %2.5f\n", transition.previous->origin.X, transition.current->origin.X);
		transition.previous->Drawer();
		transition.current->Drawer();
		return true;
	}
	return false;
}

//============================================================================
//
// DMenu base class
//
//============================================================================


DMenu::DMenu(DMenu *parent) 
{
	mParentMenu = parent;
	mMouseCapture = false;
	mBackbuttonSelected = false;
}
	
bool DMenu::Responder (event_t *ev) 
{ 
	bool res = false;
	if (ev->type == EV_GUI_Event)
	{
		if (ev->subtype == EV_GUI_LButtonDown)
		{
			res = MouseEventBack(MOUSE_Click, ev->data1, ev->data2);
			// make the menu's mouse handler believe that the current coordinate is outside the valid range
			if (res) ev->data2 = -1;	
			res |= MouseEvent(MOUSE_Click, ev->data1, ev->data2);
			if (res)
			{
				SetCapture();
			}
			
		}
		else if (ev->subtype == EV_GUI_MouseMove)
		{
			BackbuttonTime = BACKBUTTON_TIME;
			if (mMouseCapture || m_use_mouse == 1)
			{
				res = MouseEventBack(MOUSE_Move, ev->data1, ev->data2);
				if (res) ev->data2 = -1;	
				res |= MouseEvent(MOUSE_Move, ev->data1, ev->data2);
			}
		}
		else if (ev->subtype == EV_GUI_LButtonUp)
		{
			if (mMouseCapture)
			{
				ReleaseCapture();
				res = MouseEventBack(MOUSE_Release, ev->data1, ev->data2);
				if (res) ev->data2 = -1;	
				res |= MouseEvent(MOUSE_Release, ev->data1, ev->data2);
			}
		}
	}
	return false; 
}

//=============================================================================
//
//
//
//=============================================================================

bool DMenu::MenuEvent (int mkey, bool fromcontroller)
{
	switch (mkey)
	{
	case MKEY_Back:
	{
		if (scriptID != 0)
		{
			Close();
			//S_Sound (CHAN_VOICE | CHAN_UI, 	DMenu::CurrentMenu != NULL? "menu/backup" : "menu/clear", snd_menuvolume, ATTN_NONE);
			return true;
		}
	}
	}
	return false;
}

//=============================================================================
//
//
//
//=============================================================================

void DMenu::Close ()
{
	assert(DMenu::CurrentMenu == this);
	
	DMenu::CurrentMenu = mParentMenu;
	if (mParentMenu && M_StartTransition(this, mParentMenu, MA_Return, transition))
	{
		g_currentMenu = DMenu::CurrentMenu->scriptID;
	}
	else
	{
		Destroy();
		delete this;
		if (DMenu::CurrentMenu == NULL)
		{
			M_ClearMenus();
		}
		else
		{
			g_currentMenu = DMenu::CurrentMenu->scriptID;
		}
	}
}

//=============================================================================
//
//
//
//=============================================================================

bool DMenu::MouseEvent(int type, int x, int y)
{
	return true;
}

//=============================================================================
//
//
//
//=============================================================================

bool DMenu::MouseEventBack(int type, int x, int y)
{
	if (m_show_backbutton >= 0)
	{
#if 0
		FTexture *tex = TexMan(gameinfo.mBackButton);
		if (tex != NULL)
		{
			if (m_show_backbutton&1) x -= screen->GetWidth() - tex->GetScaledWidth() * CleanXfac;
			if (m_show_backbutton&2) y -= screen->GetHeight() - tex->GetScaledHeight() * CleanYfac;
			mBackbuttonSelected = ( x >= 0 && x < tex->GetScaledWidth() * CleanXfac && 
									y >= 0 && y < tex->GetScaledHeight() * CleanYfac);
			if (mBackbuttonSelected && type == MOUSE_Release)
			{
				if (m_use_mouse == 2) mBackbuttonSelected = false;
				MenuEvent(MKEY_Back, true);
			}
			return mBackbuttonSelected;
		}
#endif
	}
	return false;
}

//=============================================================================
//
//
//
//=============================================================================

void DMenu::SetCapture()
{
	if (!mMouseCapture)
	{
		mMouseCapture = true;
		I_SetMouseCapture();
	}
}

void DMenu::ReleaseCapture()
{
	if (mMouseCapture)
	{
		mMouseCapture = false;
		I_ReleaseMouseCapture();
	}
}

//=============================================================================
//
//
//
//=============================================================================

void DMenu::Ticker () 
{
}

void DMenu::Drawer () 
{
#if 0
	if (this == DMenu::CurrentMenu && BackbuttonAlpha > 0 && m_show_backbutton >= 0 && m_use_mouse)
	{
		FTexture *tex = TexMan(gameinfo.mBackButton);
		int w = tex->GetScaledWidth() * CleanXfac;
		int h = tex->GetScaledHeight() * CleanYfac;
		int x = (!(m_show_backbutton&1))? 0:screen->GetWidth() - w;
		int y = (!(m_show_backbutton&2))? 0:screen->GetHeight() - h;
		if (mBackbuttonSelected && (mMouseCapture || m_use_mouse == 1))
		{
			screen->DrawTexture(tex, x, y, DTA_CleanNoMove, true, DTA_ColorOverlay, MAKEARGB(40, 255,255,255), TAG_DONE);
		}
		else
		{
			screen->DrawTexture(tex, x, y, DTA_CleanNoMove, true, DTA_Alpha, BackbuttonAlpha, TAG_DONE);
		}
	}
#endif
}

bool DMenu::DimAllowed()
{
	return true;
}

bool DMenu::TranslateKeyboardEvents()
{
	return true;
}

//=============================================================================
//
//
//
//=============================================================================

void M_StartControlPanel (bool makeSound)
{
	static bool created = false;
	// intro might call this repeatedly
	if (DMenu::CurrentMenu != NULL)
		return;

	if (!created) // Cannot do this earlier.
	{
		created = true;
		M_CreateMenus();
	}

	buttonMap.ResetButtonStates ();
	for (int i = 0; i < NUM_MKEYS; ++i)
	{
		MenuButtons[i].ReleaseKey(0);
	}

	C_HideConsole ();				// [RH] Make sure console goes bye bye.
	GUICapture |= 1;
	menuactive = MENU_On;
	// Pause sound effects before we play the menu switch sound.
	// That way, it won't be paused.
	//P_CheckTickerPaused ();

	BackbuttonTime = 0;
	BackbuttonAlpha = 0;
	DrawBackground = -1;
	DMenu::MenuTime = -1;	
	M_Ticker();	// This needs to be called once here to make sure that the menu actually has ticked before it gets drawn for the first time.
}

void Menu_Open(int playerid)
{
	M_StartControlPanel(DMenu::CurrentMenu == nullptr);
}

//=============================================================================
//
//
//
//=============================================================================

void M_ActivateMenu(DMenu *menu)
{
	g_currentMenu = menu->scriptID;
	if (menuactive == MENU_Off) menuactive = MENU_On;
	if (DMenu::CurrentMenu != NULL)
	{
		DMenu::CurrentMenu->ReleaseCapture();
		M_StartTransition(DMenu::CurrentMenu, menu, MA_Advance, transition);
	}
	DMenu::CurrentMenu = menu;
}

//=============================================================================
//
//
//
//=============================================================================

bool M_SetMenu(FName menu, int param, FName caller)
{
	if (DrawBackground == -1)
	{
		if (menu == NAME_MainMenu) DrawBackground = 1;
		else DrawBackground = 0;
	}
	// some menus need some special treatment (needs to be adjusted for the various frontends.
	switch (caller)
	{
	case NAME_EpisodeMenu:
		// sent from the episode menu
		GameStartupInfo.Episode = param;
		GameStartupInfo.CustomLevel1 = GameStartupInfo.CustomLevel2 = -1;
		GameStartupInfo.Skill = gDefaultSkill;
		break;

	case NAME_CustomGameMenu:
		GameStartupInfo.CustomLevel1 = param;
		GameStartupInfo.Episode = GameStartupInfo.CustomLevel2 = -1;
		GameStartupInfo.Skill = gDefaultSkill;
		// gi->CustomMenuSelection(-1, param);
		break;

	case NAME_CustomSubMenu1:
		GameStartupInfo.CustomLevel2 = param;
		// gi->CustomMenuSelection(GameStartupInfo.CustomLevel1, param);
		menu = FName(ENamedName(menu + param));
		break;

	case NAME_SkillMenu:
		GameStartupInfo.Skill = param;
		break;
	}

	switch (menu)
	{
	case NAME_StartGame:
		// gi->StartGame(&GameStartupInfo);
		return false;

#if 0
	case NAME_StartgameConfirm:
	{
		// sent from the skill menu for a skill that needs to be confirmed
		GameStartupInfo.Skill = param;

		const char *msg = AllSkills[param].MustConfirmText;
		if (*msg==0) msg = GStrings("NIGHTMARE");
		M_StartMessage (msg, 0, NAME_StartgameConfirmed);
		return;
	}

	case NAME_Savegamemenu:
		if (!usergame || (players[consoleplayer].health <= 0 && !multiplayer) || gamestate != GS_LEVEL)
		{
			// cannot save outside the game.
			M_StartMessage (GStrings("SAVEDEAD"), 1);
			return;
		}
#endif
	}

	// End of special checks

	FMenuDescriptor **desc = MenuDescriptors.CheckKey(menu);
	if (desc != NULL)
	{
		/*
		if ((*desc)->mNetgameMessage.IsNotEmpty() && netgame && !demoplayback)
		{
			M_StartMessage((*desc)->mNetgameMessage, 1);
			return;
		}
		*/

		if ((*desc)->mType == MDESC_ListMenu)
		{
			FListMenuDescriptor *ld = static_cast<FListMenuDescriptor*>(*desc);
			if (ld->mAutoselect >= 0 && ld->mAutoselect < (int)ld->mItems.Size())
			{
				// recursively activate the autoselected item without ever creating this menu.
				ld->mItems[ld->mAutoselect]->Activate(ld->mMenuName);
			}
			else
			{
				DListMenu* newmenu;
				if (ld->mClass != NAME_None)
				{
					auto ndx = menuClasses.FindEx([=](const auto p) { return p->mName == ld->mClass; });
					if (ndx == menuClasses.Size())
					{
						I_Error("Bad menu class %s\n", ld->mClass.GetChars());
					}
					else
					{
						newmenu = (DListMenu*)menuClasses[ndx]->CreateNew();
					}
				}
				else
				{
					newmenu = new DListMenu;
				}
				newmenu->Init(DMenu::CurrentMenu, ld);
				M_ActivateMenu(newmenu);
				return true;
			}
		}
		else if ((*desc)->mType == MDESC_OptionsMenu)
		{
			FOptionMenuDescriptor *ld = static_cast<FOptionMenuDescriptor*>(*desc);
			//const PClass *cls = ld->mClass == NULL? RUNTIME_CLASS(DOptionMenu) : ld->mClass;

			DOptionMenu *newmenu = new DOptionMenu;
			newmenu->Init(DMenu::CurrentMenu, ld);
			M_ActivateMenu(newmenu);
		}
		return true;
	}
	else
	{
		/*
		const PClass *menuclass = PClass::FindClass(menu);
		if (menuclass != NULL)
		{
			if (menuclass->IsDescendantOf(RUNTIME_CLASS(DMenu)))
			{
				DMenu *newmenu = (DMenu*)menuclass->CreateNew();
				newmenu->mParentMenu = DMenu::CurrentMenu;
				M_ActivateMenu(newmenu);
				return true;
			}
		}
		*/
	}
	Printf("Attempting to open menu of unknown type '%s'\n", menu.GetChars());
	return false;
}

//=============================================================================
//
//
//
//=============================================================================

bool M_Responder (event_t *ev) 
{ 
	int ch = 0;
	bool keyup = false;
	int mkey = NUM_MKEYS;
	bool fromcontroller = true;

	/*
	if (chatmodeon)
	{
		return false;
	}
	*/

	if (DMenu::CurrentMenu != NULL && menuactive != MENU_Off) 
	{
		// There are a few input sources we are interested in:
		//
		// EV_KeyDown / EV_KeyUp : joysticks/gamepads/controllers
		// EV_GUI_KeyDown / EV_GUI_KeyUp : the keyboard
		// EV_GUI_Char : printable characters, which we want in string input mode
		//
		// This code previously listened for EV_GUI_KeyRepeat to handle repeating
		// in the menus, but that doesn't work with gamepads, so now we combine
		// the multiple inputs into buttons and handle the repetition manually.
		if (ev->type == EV_GUI_Event)
		{
			fromcontroller = false;
			if (ev->subtype == EV_GUI_KeyRepeat)
			{
				// We do our own key repeat handling but still want to eat the
				// OS's repeated keys.
				return true;
			}
			else if (ev->subtype == EV_GUI_BackButtonDown || ev->subtype == EV_GUI_BackButtonUp)
			{
				mkey = MKEY_Back;
				keyup = ev->subtype == EV_GUI_BackButtonUp;
			}
			else if (ev->subtype != EV_GUI_KeyDown && ev->subtype != EV_GUI_KeyUp)
			{
				// do we want mouse input?
				if (ev->subtype >= EV_GUI_FirstMouseEvent && ev->subtype <= EV_GUI_LastMouseEvent)
				{
						if (!m_use_mouse)
							return true;
				}

				// pass everything else on to the current menu
				return DMenu::CurrentMenu->Responder(ev);
			}
			else if (DMenu::CurrentMenu->TranslateKeyboardEvents())
			{
				ch = ev->data1;
				keyup = ev->subtype == EV_GUI_KeyUp;
				switch (ch)
				{
				case GK_BACK:			mkey = MKEY_Back;		break;
				case GK_ESCAPE:			mkey = MKEY_Back;		break;
				case GK_RETURN:			mkey = MKEY_Enter;		break;
				case GK_UP:				mkey = MKEY_Up;			break;
				case GK_DOWN:			mkey = MKEY_Down;		break;
				case GK_LEFT:			mkey = MKEY_Left;		break;
				case GK_RIGHT:			mkey = MKEY_Right;		break;
				case GK_BACKSPACE:		mkey = MKEY_Clear;		break;
				case GK_PGUP:			mkey = MKEY_PageUp;		break;
				case GK_PGDN:			mkey = MKEY_PageDown;	break;
				default:
					if (!keyup)
					{
						return DMenu::CurrentMenu->Responder(ev);
					}
					break;
				}
			}
		}
		else if (menuactive != MENU_WaitKey && (ev->type == EV_KeyDown || ev->type == EV_KeyUp))
		{
			keyup = ev->type == EV_KeyUp;

			ch = ev->data1;
			switch (ch)
			{
			case KEY_JOY1:
			case KEY_PAD_A:
				mkey = MKEY_Enter;
				break;

			case KEY_JOY2:
			case KEY_PAD_B:
				mkey = MKEY_Back;
				break;

			case KEY_JOY3:
			case KEY_PAD_X:
				mkey = MKEY_Clear;
				break;

			case KEY_JOY5:
			case KEY_PAD_LSHOULDER:
				mkey = MKEY_PageUp;
				break;

			case KEY_JOY6:
			case KEY_PAD_RSHOULDER:
				mkey = MKEY_PageDown;
				break;

			case KEY_PAD_DPAD_UP:
			case KEY_PAD_LTHUMB_UP:
			case KEY_JOYAXIS1MINUS:
			case KEY_JOYPOV1_UP:
				mkey = MKEY_Up;
				break;

			case KEY_PAD_DPAD_DOWN:
			case KEY_PAD_LTHUMB_DOWN:
			case KEY_JOYAXIS1PLUS:
			case KEY_JOYPOV1_DOWN:
				mkey = MKEY_Down;
				break;

			case KEY_PAD_DPAD_LEFT:
			case KEY_PAD_LTHUMB_LEFT:
			case KEY_JOYAXIS2MINUS:
			case KEY_JOYPOV1_LEFT:
				mkey = MKEY_Left;
				break;

			case KEY_PAD_DPAD_RIGHT:
			case KEY_PAD_LTHUMB_RIGHT:
			case KEY_JOYAXIS2PLUS:
			case KEY_JOYPOV1_RIGHT:
				mkey = MKEY_Right;
				break;
			}
		}

		if (mkey != NUM_MKEYS)
		{
			if (keyup)
			{
				MenuButtons[mkey].ReleaseKey(ch);
				return false;
			}
			else
			{
				MenuButtons[mkey].PressKey(ch);
				MenuButtonOrigin[mkey] = fromcontroller;
				if (mkey <= MKEY_PageDown)
				{
					MenuButtonTickers[mkey] = KEY_REPEAT_DELAY;
				}
				DMenu::CurrentMenu->MenuEvent(mkey, fromcontroller);
				return true;
			}
		}
		return DMenu::CurrentMenu->Responder(ev) || !keyup;
	}
	else if (MenuEnabled)
	{
		if (ev->type == EV_KeyDown)
		{
			// Pop-up menu?
			if (ev->data1 == KEY_ESCAPE)
			{
				M_StartControlPanel(true);
				M_SetMenu(NAME_IngameMenu, -1);
				return true;
			}
			return false;
		}
		else if (ev->type == EV_GUI_Event && ev->subtype == EV_GUI_LButtonDown && 
				 ConsoleState != c_down && m_use_mouse)
		{
			M_StartControlPanel(true);
			M_SetMenu(NAME_MainMenu, -1);
			return true;
		}
	}
	return false;
}

//=============================================================================
//
//
//
//=============================================================================

void M_Ticker (void) 
{
	DMenu::MenuTime++;
	if (DMenu::MenuTime & 3) return;
	if (DMenu::CurrentMenu != NULL && menuactive != MENU_Off) 
	{
		if (transition.previous) transition.previous->Ticker();
		DMenu::CurrentMenu->Ticker();

		for (int i = 0; i < NUM_MKEYS; ++i)
		{
			if (MenuButtons[i].bDown)
			{
				if (MenuButtonTickers[i] > 0 &&	--MenuButtonTickers[i] <= 0)
				{
					MenuButtonTickers[i] = KEY_REPEAT_RATE;
					DMenu::CurrentMenu->MenuEvent(i, MenuButtonOrigin[i]);
				}
			}
		}
		if (BackbuttonTime > 0)
		{
			if (BackbuttonAlpha < 1.0f) BackbuttonAlpha += 0.1f;
			BackbuttonTime--;
		}
		else
		{
			if (BackbuttonAlpha > 0) BackbuttonAlpha -= 0.1f;
			if (BackbuttonAlpha < 0) BackbuttonAlpha = 0;
		}
	}
}

//=============================================================================
//
//
//
//=============================================================================

void M_Drawer (void) 
{
	rotatesprite_2doverride = true;
	PalEntry fade = 0x70000000;

	if (DMenu::CurrentMenu != NULL && menuactive != MENU_Off)
	{
		if (DMenu::CurrentMenu->DimAllowed() && fade && !DrawBackground) twod.AddColorOnlyQuad(0, 0, screen->GetWidth(), screen->GetHeight(), fade);

		bool going = false;
		if (transition.previous)
		{
			going = M_DrawTransition(transition);
			if (!going)
			{
				if (transition.dir == -1) delete transition.previous;
				transition.previous = nullptr;
				transition.current = nullptr;
			}
		}
		if (!going)
		{
			DMenu::CurrentMenu->origin = { 0,0 };
			// else if (DrawBackground) Menu_DrawBackground(origin);
			DMenu::CurrentMenu->Drawer();
		}
	}
	rotatesprite_2doverride = false;
}

//=============================================================================
//
//
//
//=============================================================================

void M_ClearMenus ()
{
	M_DemoNoPlay = false;
	if (DMenu::CurrentMenu != NULL)
	{
		DMenu::CurrentMenu->Destroy();
		delete DMenu::CurrentMenu;
		DMenu::CurrentMenu = NULL;
	}
	menuactive = MENU_Off;
	GUICapture &= ~1;
}

void Menu_Close(int playerid)
{
	M_ClearMenus();
}
//=============================================================================
//
//
//
//=============================================================================

void M_Init (void) 
{
	RegisterDukeMenus();
	timerSetCallback(M_Ticker);
	M_ParseMenuDefs();
}


//=============================================================================
//
//
//
//=============================================================================

void M_EnableMenu (bool on) 
{
	MenuEnabled = on;
}


//=============================================================================
//
// [RH] Most menus can now be accessed directly
// through console commands.
//
//=============================================================================


CCMD (openmenu)
{
	if (argv.argc() < 2)
	{
		Printf("Usage: openmenu \"menu_name\"");
		return;
	}
	M_StartControlPanel (true);
	M_SetMenu(argv[1], -1);
}

CCMD (closemenu)
{
	M_ClearMenus();
}

//
//		Toggle messages on/off
//
CCMD (togglemessages)
{
	if (show_messages)
	{
		Printf (128, "%s\n", GStrings("MSGOFF"));
		show_messages = false;
	}
	else
	{
		Printf (128, "%s\n", GStrings("MSGON"));
		show_messages = true;
	}
}

EXTERN_CVAR (Int, screenblocks)

CCMD(menuconsole)
{
	M_ClearMenus();
	C_ToggleConsole();
}

CCMD(reset2defaults)
{
	C_SetDefaultBindings ();
	C_SetCVarsToDefaults ();
}

CCMD(reset2saved)
{
	GameConfig->DoGlobalSetup ();
	GameConfig->DoGameSetup (currentGame);
}

CCMD(openmainmenu)
{
	FX_StopAllSounds();
	//gi->ClearSoundLocks();
	//gi->MenuSound();
	M_StartControlPanel(false);
	M_SetMenu(NAME_IngameMenu);
}

CCMD(openhelpmenu)
{
	if (!help_disabled)
	{
		gi->MenuOpened();
		M_StartControlPanel(false);
		M_SetMenu(NAME_HelpMenu);
	}
}

CCMD(opensavemenu)
{
	if (gi->CanSave()) 
	{
		gi->MenuOpened();
		M_StartControlPanel(false);
		M_SetMenu(NAME_SaveMenu);
	}
}

CCMD(openloadmenu)
{
	gi->MenuOpened();
	M_StartControlPanel(false);
	M_SetMenu(NAME_LoadMenu);
}
