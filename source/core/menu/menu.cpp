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
#include "pragmas.h"
#include "build.h"
#include "statistics.h"
#include "m_joy.h"
#include "raze_sound.h"
#include "texturemanager.h"
#include "v_video.h"
#include "gamestate.h"

void RegisterDuke3dMenus();
void RegisterBloodMenus();
void RegisterSWMenus();
void RegisterPSMenus();
void RegisterLoadsaveMenus();
void RegisterOptionMenus();
void RegisterJoystickMenus();
void UpdateJoystickMenu(IJoystickConfig* joy);
bool help_disabled, credits_disabled;
int g_currentMenu;	// accessible by CON scripts - contains the current menu's script ID if defined or INT_MAX if none given.
TArray<DMenu*> toDelete;

//
// Todo: Move these elsewhere
//
CVAR (Float, mouse_sensitivity, 1.f, CVAR_ARCHIVE|CVAR_GLOBALCONFIG)
EXTERN_CVAR (Bool, show_messages)


CVAR(Bool, menu_sounds, true, CVAR_ARCHIVE) // added mainly because RR's sounds are so supremely annoying.
//CVAR (Float, snd_menuvolume, 0.6f, CVAR_ARCHIVE) the current sound engine cannot deal with this.
CVAR(Int, m_use_mouse, 2, CVAR_ARCHIVE|CVAR_GLOBALCONFIG)
CVAR(Int, m_show_backbutton, 0, CVAR_ARCHIVE|CVAR_GLOBALCONFIG)

TArray<MenuClassDescriptor*> menuClasses(TArray<MenuClassDescriptor*>::ENoInit(0));

DMenu *CurrentMenu;
bool DMenu::InMenu;

FNewGameStartup NewGameStartupInfo;
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


static MenuTransition transition;

bool MenuTransition::StartTransition(DMenu *from, DMenu *to, MenuTransitionType animtype)
{
	if (!from->canAnimate || !to->canAnimate || animtype == MA_None)
	{
		return false;
	}
	else
	{
		start  = I_GetTimeNS() * (120. / 1'000'000'000.);
		length = 30;
		dir = animtype == MA_Advance? 1 : -1;
		previous = from;
		current  = to;
		return true;
	}
}

bool MenuTransition::Draw()
{
	double now = I_GetTimeNS() * (120. / 1'000'000'000);
	if (now < start + length)
	{
		double factor = 120 * xdim / ydim;
		double phase = (now - start) / double(length) * M_PI + M_PI/2;
		
		previous->origin.X = factor * dir * (sin(phase) - 1.);
		current->origin.X = factor * dir * (sin(phase) + 1.);
		previous->Drawer();
		current->Drawer();
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
		//if (scriptID != 0)
		{
			M_MenuSound(CurrentMenu->mParentMenu? BackSound : CloseSound);
			Close();
			if (!CurrentMenu && gamestate == GS_MENUSCREEN) C_FullConsole();
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
	assert(CurrentMenu == this);
	
	CurrentMenu = mParentMenu;
	if (mParentMenu && transition.StartTransition(this, mParentMenu, MA_Return))
	{
		g_currentMenu = CurrentMenu->scriptID;
	}
	else
	{
		Destroy();
		toDelete.Push(this);
		if (CurrentMenu == NULL)
		{
			M_ClearMenus();
		}
		else
		{
			g_currentMenu = CurrentMenu->scriptID;
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
		auto texid = TexMan.CheckForTexture("engine/graphics/m_back.png", ETextureType::Any);
		if (texid.isValid())
		{
			auto tex = TexMan.GetGameTexture(texid);
			if (m_show_backbutton&1) x -= screen->GetWidth() - tex->GetDisplayWidth() * CleanXfac;
			if (m_show_backbutton&2) y -= screen->GetHeight() - tex->GetDisplayHeight() * CleanYfac;
			mBackbuttonSelected = ( x >= 0 && x < tex->GetDisplayWidth() * CleanXfac &&
									y >= 0 && y < tex->GetDisplayHeight() * CleanYfac);
			if (mBackbuttonSelected && type == MOUSE_Release)
			{
				if (m_use_mouse == 2) mBackbuttonSelected = false;
				MenuEvent(MKEY_Back, true);
			}
			return mBackbuttonSelected;
		}
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
	if (this == CurrentMenu && BackbuttonAlpha > 0 && m_show_backbutton >= 0 && m_use_mouse)
	{
		auto texid = TexMan.CheckForTexture("engine/graphics/m_back.png", ETextureType::Any);
		if (texid.isValid())
		{
			auto tex = TexMan.GetGameTexture(texid);
			int w = tex->GetDisplayWidth() * CleanXfac;
			int h = tex->GetDisplayHeight() * CleanYfac;
			int x = (!(m_show_backbutton & 1)) ? 0 : screen->GetWidth() - w;
			int y = (!(m_show_backbutton & 2)) ? 0 : screen->GetHeight() - h;
			if (mBackbuttonSelected && (mMouseCapture || m_use_mouse == 1))
			{
				DrawTexture(twod, tex, x, y, DTA_CleanNoMove, true, DTA_ColorOverlay, MAKEARGB(40, 255, 255, 255), TAG_DONE);
			}
			else
			{
				DrawTexture(twod, tex, x, y, DTA_CleanNoMove, true, DTA_Alpha, BackbuttonAlpha, TAG_DONE);
			}
		}
	}
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
	if (CurrentMenu != NULL)
		return;

	if (!created) // Cannot do this earlier.
	{
		created = true;
		M_CreateMenus();
	}
	GSnd->SetSfxPaused(true, PAUSESFX_MENU);
	gi->MenuOpened();
	if (makeSound) M_MenuSound(ActivateSound);

	inputState.ClearAllInput();
	for (int i = 0; i < NUM_MKEYS; ++i)
	{
		MenuButtons[i].ReleaseKey(0);
	}

	C_HideConsole ();				// [RH] Make sure console goes bye bye.
	menuactive = MENU_On;
	// Pause sound effects before we play the menu switch sound.
	// That way, it won't be paused.
	//P_CheckTickerPaused ();

	BackbuttonTime = 0;
	BackbuttonAlpha = 0;
}

void Menu_Open(int playerid)
{
	M_StartControlPanel(CurrentMenu == nullptr);
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
	if (CurrentMenu != NULL)
	{
		CurrentMenu->ReleaseCapture();
		transition.StartTransition(CurrentMenu, menu, MA_Advance);
	}
	CurrentMenu = menu;
}

//=============================================================================
//
//
//
//=============================================================================

bool M_SetMenu(FName menu, int param, FName caller)
{
#if 0
	// skip the menu and go right into the first level. 
	// For tracking memory leaks that normally require operating the menu to start the game so that they always get the same allocation number.
	NewGameStartupInfo.Episode = NewGameStartupInfo.Skill = 0;
	menu = NAME_Startgame;
#endif
	// some menus need some special treatment (needs to be adjusted for the various frontends.
	switch (caller.GetIndex())
	{
	case NAME_Episodemenu:
	case NAME_HuntMenu:
	case NAME_TargetMenu:
		// sent from the episode menu
		NewGameStartupInfo.Episode = param;
		NewGameStartupInfo.Level = 0;
		NewGameStartupInfo.CustomLevel1 = NewGameStartupInfo.CustomLevel2 = -1;
		NewGameStartupInfo.Skill = gDefaultSkill;
		break;

	case NAME_WeaponMenu:
		NewGameStartupInfo.Skill = param;
		break;

	case NAME_CustomGameMenu:
		NewGameStartupInfo.CustomLevel1 = param;
		NewGameStartupInfo.CustomLevel2 = -1;
		NewGameStartupInfo.Episode = 0;	// Set start to E1L1 so that even if the script fails to set the starting level it is set to something valid.
		NewGameStartupInfo.Level = 0;
		NewGameStartupInfo.Skill = gDefaultSkill;
		gi->CustomMenuSelection(param, -1);
		break;

	case NAME_CustomSubMenu1:
	case NAME_CustomSubMenu2:
	case NAME_CustomSubMenu3:
	case NAME_CustomSubMenu4:
	case NAME_CustomSubMenu5:
	case NAME_CustomSubMenu6:
	case NAME_CustomSubMenu7:
		NewGameStartupInfo.CustomLevel2 = param;
		gi->CustomMenuSelection(NewGameStartupInfo.CustomLevel1, param);
		break;

	case NAME_Skillmenu:
		NewGameStartupInfo.Skill = param;
		break;

	case NAME_EngineCredits:
	case NAME_EngineCredits2:
	{
		auto m = CurrentMenu;
		CurrentMenu = m->mParentMenu;
		m->mParentMenu = nullptr;
		toDelete.Push(m);
		break;
	}
	}

	switch (menu.GetIndex())
	{
	case NAME_Startgame:
		M_ClearMenus();	// must be done before starting the level.
		if (caller == NAME_Mainmenu || caller == NAME_IngameMenu) NewGameStartupInfo.Episode = param;
		STAT_StartNewGame(gVolumeNames[NewGameStartupInfo.Episode], NewGameStartupInfo.Skill);
		inputState.ClearAllInput();

		gi->StartGame(NewGameStartupInfo);
		return false;

	case NAME_CustomSubMenu1:
		menu = ENamedName(menu.GetIndex() + param);
		break;

#if 0
	case NAME_StartgameConfirm:
	{
		// sent from the skill menu for a skill that needs to be confirmed
		NewGameStartupInfo.Skill = param;

		const char *msg = AllSkills[param].MustConfirmText;
		if (*msg==0) msg = GStrings("NIGHTMARE");
		M_StartMessage (msg, 0, -1, NAME_StartgameConfirmed);
		return;
	}
#endif

	case NAME_Savegamemenu:
		if (!gi->CanSave())
		{
			// cannot save outside the game.
			M_StartMessage (GStrings("SAVEDEAD"), 1, -1);
			return true;
		}
		break;
			
	case NAME_Quitmenu:
		// This is no separate class
		C_DoCommand("menu_quit");
		return true;

	case NAME_EndGameMenu:
		// This is no separate class
		C_DoCommand("menu_endgame");
		return true;
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
						return true;
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
				newmenu->Init(CurrentMenu, ld);
				M_ActivateMenu(newmenu);
			}
		}
		else if ((*desc)->mType == MDESC_OptionsMenu)
		{
			FOptionMenuDescriptor *ld = static_cast<FOptionMenuDescriptor*>(*desc);
			DOptionMenu* newmenu = nullptr;
			if (ld->mClass != NAME_None)
			{
				auto ndx = menuClasses.FindEx([=](const auto p) { return p->mName == ld->mClass; });
				if (ndx == menuClasses.Size())
				{
					I_Error("Bad menu class %s\n", ld->mClass.GetChars());
				}
				else
				{
					newmenu = (DOptionMenu*)menuClasses[ndx]->CreateNew();
				}
			}
			else
			{
				newmenu = new DOptionMenu;
			}
			newmenu->Init(CurrentMenu, ld);
			M_ActivateMenu(newmenu);
		}
		else if ((*desc)->mType == MDESC_ImageScroller)
		{
			FImageScrollerDescriptor* ld = static_cast<FImageScrollerDescriptor*>(*desc);
			DImageScrollerMenu* newmenu;
			if (ld->mClass != NAME_None)
			{
				auto ndx = menuClasses.FindEx([=](const auto p) { return p->mName == ld->mClass; });
				if (ndx == menuClasses.Size())
				{
					I_Error("Bad menu class %s\n", ld->mClass.GetChars());
					return true;
				}
				else
				{
					newmenu = (DImageScrollerMenu*)menuClasses[ndx]->CreateNew();
				}
			}
			else
			{
				newmenu = new DImageScrollerMenu;
			}
			newmenu->Init(CurrentMenu, ld);
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
				newmenu->mParentMenu = CurrentMenu;
				M_ActivateMenu(newmenu);
				return true;
			}
		}
		*/
	}
	Printf("Attempting to open menu of unknown type '%s'\n", menu.GetChars());
	M_ClearMenus();
	return false;
}

//=============================================================================
//
//
//
//=============================================================================

bool M_DoResponder (event_t *ev) 
{ 
	int ch = 0;
	bool keyup = false;
	int mkey = NUM_MKEYS;
	bool fromcontroller = true;

	if (chatmodeon)
	{
		return false;
	}

	if (CurrentMenu != NULL && menuactive != MENU_Off) 
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
				return CurrentMenu->Responder(ev);
			}
			else if (CurrentMenu->TranslateKeyboardEvents())
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
						return CurrentMenu->Responder(ev);
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
				CurrentMenu->MenuEvent(mkey, fromcontroller);
				return true;
			}
		}
		return CurrentMenu->Responder(ev) || !keyup;
	}
	else if (MenuEnabled)
	{
		if (ev->type == EV_KeyDown)
		{
			// Pop-up menu?
			if (ev->data1 == KEY_ESCAPE) // Should we let the games handle Escape for special actions, like backing out of cameras?
			{
				if (gamestate != GS_STARTUP && gamestate != GS_INTRO)
				{
					M_StartControlPanel(true);
					M_SetMenu(gi->CanSave()? NAME_IngameMenu : NAME_Mainmenu, -1);
					if (gamestate == GS_FULLCONSOLE) gamestate = GS_MENUSCREEN;
				}
				return true;
			}
			return false;
		}
		else if (ev->type == EV_GUI_Event && ev->subtype == EV_GUI_LButtonDown && 
				 ConsoleState != c_down && m_use_mouse)
		{
			M_StartControlPanel(true);
			M_SetMenu(NAME_Mainmenu, -1);
			return true;
		}
	}
	return false;
}

bool M_Responder(event_t* ev)
{
	bool res = false;
	// delayed deletion, so that self-deleting menus don't crash if they are getting accessed after being closed.
	try
	{
		res = M_DoResponder(ev);
	}
	catch (...)
	{
		for (auto p : toDelete) delete p;
		toDelete.Clear();
		throw;
	}
	for (auto p : toDelete) delete p;
	toDelete.Clear();
	return res;
}

//=============================================================================
//
//
//
//=============================================================================

void M_Ticker (void) 
{
	if (CurrentMenu != NULL && menuactive != MENU_Off) 
	{
		if (transition.previous) transition.previous->Ticker();
		if (CurrentMenu == nullptr) return; // In case one of the sub-screens has closed the menu.
		CurrentMenu->Ticker();

		for (int i = 0; i < NUM_MKEYS; ++i)
		{
			if (MenuButtons[i].bDown)
			{
				if (MenuButtonTickers[i] > 0 &&	--MenuButtonTickers[i] <= 0)
				{
					MenuButtonTickers[i] = KEY_REPEAT_RATE;
					CurrentMenu->MenuEvent(i, MenuButtonOrigin[i]);
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
	PalEntry fade = 0x70000000;

	if (CurrentMenu != NULL && menuactive != MENU_Off)
	{
		if (CurrentMenu->DimAllowed() && fade && gamestate != GS_MENUSCREEN) twod->AddColorOnlyQuad(0, 0, screen->GetWidth(), screen->GetHeight(), fade);

		bool going = false;
		if (transition.previous)
		{
			going = transition.Draw();
			if (!going)
			{
				if (transition.dir == -1) delete transition.previous;
				transition.previous = nullptr;
				transition.current = nullptr;
			}
		}
		if (!going)
		{
			assert(CurrentMenu);
			CurrentMenu->origin = { 0,0 };
			CurrentMenu->Drawer();
		}
	}
}

//=============================================================================
//
//
//
//=============================================================================

void M_UnpauseSound()
{
	GSnd->SetSfxPaused(false, PAUSESFX_MENU);
}

void M_ClearMenus (bool final)
{
	if (menuactive == MENU_Off) return;
	M_DemoNoPlay = false;
	transition.previous = transition.current = nullptr;
	transition.dir = 0;
	auto menu = CurrentMenu;
	while (menu != nullptr)
	{
		auto nextm = menu->mParentMenu;
		menu->Destroy();
		delete menu;
		menu = nextm;
	}
	CurrentMenu = nullptr;
	menuactive = MENU_Off;
	M_UnpauseSound();
	inputState.ClearAllInput();
	if (!final)
	{
		gi->MenuClosed();
	}
}

bool M_Active()
{
	return CurrentMenu != nullptr || ConsoleState == c_down || ConsoleState == c_falling;
}

//=============================================================================
//
//
//
//=============================================================================

void M_MenuSound(EMenuSounds snd)
{
	if (menu_sounds) gi->MenuSound(snd);
}

//=============================================================================
//
//
//
//=============================================================================

void M_PreviousMenu()
{
	if (CurrentMenu != nullptr)
	{
		DMenu* parent = CurrentMenu->mParentMenu;
		CurrentMenu->Destroy();
		CurrentMenu = parent;
	}
}

//=============================================================================
//
//
//
//=============================================================================

void M_Init (void) 
{
	RegisterDuke3dMenus();
	RegisterBloodMenus();
	RegisterSWMenus();
	RegisterPSMenus();
	RegisterLoadsaveMenus();
	RegisterOptionMenus();
	RegisterJoystickMenus();
	M_ParseMenuDefs();
	UpdateJoystickMenu(nullptr);
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


bool M_IsAnimated()
{
	if (ConsoleState == c_down) return false;
	if (!CurrentMenu) return false;
	if (CurrentMenu->IsAnimated()) return true;
	if(transition.previous) return true;
	return false;
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

CCMD(menu_main)
{
	M_StartControlPanel(true);
	M_SetMenu(gi->CanSave() ? NAME_IngameMenu : NAME_Mainmenu, -1);
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

