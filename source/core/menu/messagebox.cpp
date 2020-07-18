/*
** messagebox.cpp
** Confirmation, notification screns
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

#include "menu.h"
#include "d_event.h"
#include "d_gui.h"
#include "v_text.h"
#include "v_draw.h"
#include "gstrings.h"
#include "c_dispatch.h"
#include "statistics.h"
#include "v_2ddrawer.h"
#include "v_video.h"
#include "engineerrors.h"

extern FSaveGameNode *quickSaveSlot;


void GameInterface::DrawCenteredTextScreen(const DVector2& origin, const char* text, int position, bool bg)
{
	double scale = SmallFontScale();
	int formatwidth = int(320 / scale);
	auto lines = V_BreakLines(SmallFont, formatwidth, text, true);
	auto fheight = bg? 10 : SmallFont->GetHeight()* scale;	// Fixme: Get spacing for text pages from elsewhere.
	if (!bg)
	{
		auto totaltextheight = lines.Size() * fheight;
		position -= totaltextheight / 2;
	}

	double y = origin.Y + position;
	for (auto& line : lines)
	{
		double x = origin.X + 160 - line.Width * scale * 0.5;
		DrawText(twod, SmallFont, CR_UNTRANSLATED, x, y, line.Text, DTA_FullscreenScale, 3, DTA_VirtualWidth, 320, DTA_VirtualHeight, 200, DTA_ScaleX, scale, DTA_ScaleY, scale, TAG_DONE);
		y += fheight;
	}
}

class DMessageBoxMenu : public DMenu
{
	using Super = DMenu;
	FString mFullMessage;
	TArray<FBrokenLines> mMessage;
	int mMessageMode;
	int messageSelection;
	int mMouseLeft, mMouseRight, mMouseY;
	FName mAction;
	std::function<void(bool)> mActionFunc;

public:

	DMessageBoxMenu(DMenu *parent = NULL, const char *message = NULL, int messagemode = 0, bool playsound = false, FName action = NAME_None, hFunc handler = nullptr);
	void Destroy();
	void Init(DMenu *parent, const char *message, int messagemode, bool playsound = false);
	void Drawer();
	bool Responder(event_t *ev);
	bool MenuEvent(int mkey, bool fromcontroller);
	bool MouseEvent(int type, int x, int y);
	void CloseSound();
	virtual void HandleResult(bool res);
};


//=============================================================================
//
//
//
//=============================================================================

DMessageBoxMenu::DMessageBoxMenu(DMenu *parent, const char *message, int messagemode, bool playsound, FName action, hFunc handler)
: DMenu(parent)
{
	mAction = action;
	mActionFunc = handler;
	messageSelection = 0;
	mMouseLeft = 140;
	mMouseY = INT_MIN;
	int mr1 = 170 + SmallFont->StringWidth(GStrings["TXT_YES"]);
	int mr2 = 170 + SmallFont->StringWidth(GStrings["TXT_NO"]);
	mMouseRight = std::max(mr1, mr2);

	Init(parent, message, messagemode, playsound);
}

//=============================================================================
//
//
//
//=============================================================================

void DMessageBoxMenu::Init(DMenu *parent, const char *message, int messagemode, bool playsound)
{
	mParentMenu = parent;
	if (message != NULL) 
	{
		mFullMessage = message;
		mMessage = V_BreakLines(SmallFont, 300, GStrings.localize(message));
	}
	mMessageMode = messagemode;
	if (playsound)
	{
		//S_StopSound (CHAN_VOICE);
		//S_Sound (CHAN_VOICE | CHANF_UI, "menu/prompt", snd_menuvolume, ATTN_NONE);
	}
}

//=============================================================================
//
//
//
//=============================================================================

void DMessageBoxMenu::Destroy()
{
	mMessage.Reset();
}

//=============================================================================
//
//
//
//=============================================================================

void DMessageBoxMenu::CloseSound()
{
	M_MenuSound(CurrentMenu ? BackSound : ::CloseSound);
}

//=============================================================================
//
//
//
//=============================================================================

void DMessageBoxMenu::HandleResult(bool res)
{
	if (mMessageMode == 0)
	{
		if (mActionFunc)
		{
			mActionFunc(res);
			Close();
		}
		else if (mAction == NAME_None && mParentMenu)
		{
			mParentMenu->MenuEvent(res ? MKEY_MBYes : MKEY_MBNo, false);
			Close();
		}
		else
		{
			Close();
			if (res) M_SetMenu(mAction, -1);
		}
		CloseSound();
	}
}

//=============================================================================
//
//
//
//=============================================================================
CVAR(Bool, m_generic_messagebox, false, CVAR_ARCHIVE)

void DMessageBoxMenu::Drawer()
{
	int y;
	PalEntry fade = 0;

	int fontheight = SmallFont->GetHeight();
	//V_SetBorderNeedRefresh();
	//ST_SetNeedRefresh();

	y = 100;

	if (m_generic_messagebox)
	{
		if (mMessage.Size())
		{
			for (unsigned i = 0; i < mMessage.Size(); i++)
				y -= SmallFont->GetHeight() / 2;

			for (unsigned i = 0; i < mMessage.Size(); i++)
			{
				DrawText(twod, SmallFont, CR_UNTRANSLATED, 160 - mMessage[i].Width / 2, y, mMessage[i].Text,
					DTA_Clean, true, TAG_DONE);
				y += fontheight;
			}
		}

		if (mMessageMode == 0)
		{
			y += fontheight;
			mMouseY = y;
			DrawText(twod, SmallFont,
				messageSelection == 0 ? OptionSettings.mFontColorSelection : OptionSettings.mFontColor,
				160, y, GStrings["TXT_YES"], DTA_Clean, true, TAG_DONE);
			DrawText(twod, SmallFont,
				messageSelection == 1 ? OptionSettings.mFontColorSelection : OptionSettings.mFontColor,
				160, y + fontheight + 1, GStrings["TXT_NO"], DTA_Clean, true, TAG_DONE);

			if (messageSelection >= 0)
			{
				if (((DMenu::MenuTime >> 2) % 8) < 6)
				{
					DrawText(twod, SmallFont, OptionSettings.mFontColorSelection,
						(150 - 160) * CleanXfac + screen->GetWidth() / 2,
						(y + (fontheight + 1) * messageSelection - 100 + fontheight / 2 - 5) * CleanYfac + screen->GetHeight() / 2,
						"\xd",
						DTA_CellX, 8 * CleanXfac,
						DTA_CellY, 8 * CleanYfac,
						TAG_DONE);
				}
			}
		}
	}
	else
	{
		twod->ClearScreen(0xa0000000);
		gi->DrawCenteredTextScreen(origin, mFullMessage, 100, false);
	}
}

//=============================================================================
//
//
//
//=============================================================================

bool DMessageBoxMenu::Responder(event_t *ev)
{
	if (ev->type == EV_GUI_Event && ev->subtype == EV_GUI_KeyDown)
	{
		if (mMessageMode == 0)
		{
			int ch = tolower(ev->data1);
			if (ch == 'n' || ch == ' ') 
			{
				HandleResult(false);		
				return true;
			}
			else if (ch == 'y') 
			{
				HandleResult(true);
				return true;
			}
		}
		else
		{
			Close();
			return true;
		}
		return false;
	}
	else if (ev->type == EV_KeyDown)
	{
		Close();
		return true;
	}
	return Super::Responder(ev);
}

//=============================================================================
//
//
//
//=============================================================================

bool DMessageBoxMenu::MenuEvent(int mkey, bool fromcontroller)
{
	if (mMessageMode == 0)
	{
		if ((mkey == MKEY_Up || mkey == MKEY_Down) && m_generic_messagebox)
		{
			//S_Sound (CHAN_VOICE | CHANF_UI, "menu/cursor", snd_menuvolume, ATTN_NONE);
			messageSelection = !messageSelection;
			return true;
		}
		else if (mkey == MKEY_Enter)
		{
			// 0 is yes, 1 is no
			HandleResult(!messageSelection);
			return true;
		}
		else if (mkey == MKEY_Back)
		{
			HandleResult(false);
			return true;
		}
		return false;
	}
	else
	{
		Close();
		CloseSound();
		return true;
	}
}

//=============================================================================
//
//
//
//=============================================================================

bool DMessageBoxMenu::MouseEvent(int type, int x, int y)
{
	if (mMessageMode == 1 || m_generic_messagebox)
	{
		if (type == MOUSE_Click)
		{
			return MenuEvent(MKEY_Enter, true);
		}
		return false;
	}
	else
	{
		int sel = -1;
		int fh = SmallFont->GetHeight() + 1;

		// convert x/y from screen to virtual coordinates, according to CleanX/Yfac use in DrawTexture
		x = ((x - (screen->GetWidth() / 2)) / CleanXfac) + 160;
		y = ((y - (screen->GetHeight() / 2)) / CleanYfac) + 100;

		if (x >= mMouseLeft && x <= mMouseRight && y >= mMouseY && y < mMouseY + 2 * fh)
		{
			sel = y >= mMouseY + fh;
		}
		if (sel != -1 && sel != messageSelection)
		{
			gi->MenuSound(CursorSound);
		}
		messageSelection = sel;
		if (type == MOUSE_Release)
		{
			return MenuEvent(MKEY_Enter, true);
		}
		return true;
	}
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

void M_StartMessage(const char *message, int messagemode, int scriptId, FName action)
{
	if (CurrentMenu == NULL) 
	{
		// only play a sound if no menu was active before
		M_StartControlPanel(menuactive == MENU_Off);
	}
	DMenu *newmenu = new DMessageBoxMenu(CurrentMenu, message, messagemode, false, action);
	newmenu->mParentMenu = CurrentMenu;
	newmenu->scriptID = scriptId;
	M_ActivateMenu(newmenu);
}


//=============================================================================
//
//
//
//=============================================================================

DMenu* CreateMessageBoxMenu(DMenu* parent, const char* message, int messagemode, int scriptId, bool playsound, FName action, hFunc handler)
{
	auto newmenu = new DMessageBoxMenu(CurrentMenu, message, messagemode, false, action, handler);
	newmenu->scriptID = scriptId;
	return newmenu;

}


void ActivateEndGameMenu()
{
}

CCMD (menu_endgame)
{	// F7
	if (!gi->CanSave())
	{
		return;
	}
		
	M_StartControlPanel (true);
	FString tempstring;
	tempstring << GStrings("ENDGAME") << "\n\n" << GStrings("PRESSYN");
	DMenu* newmenu = CreateMessageBoxMenu(CurrentMenu, tempstring, 0, 501, false, NAME_None, [](bool res)
		{
			if (res)
			{
                STAT_Cancel();
				gi->QuitToTitle();
			}
		});

	M_ActivateMenu(newmenu);
}

//=============================================================================
//
//
//
//=============================================================================

CCMD (menu_quit)
{	// F10

	M_StartControlPanel (true);

	FString EndString;
	EndString << GStrings("CONFIRM_QUITMSG") << "\n\n" << GStrings("PRESSYN");

	DMenu *newmenu = CreateMessageBoxMenu(CurrentMenu, EndString, 0, 500, false, NAME_None, [](bool res)
	{
			if (res) gi->ExitFromMenu();
	});

	M_ActivateMenu(newmenu);
}


