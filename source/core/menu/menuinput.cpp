/*
** menuinput.cpp
** The string input code
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
#include "c_cvars.h"
#include "d_event.h"
#include "d_gui.h"
#include "v_font.h"
#include "v_text.h"
#include "v_draw.h"
#include "v_video.h"

#define INPUTGRID_WIDTH		13
#define INPUTGRID_HEIGHT	5

// Heretic and Hexen do not, by default, come with glyphs for all of these
// characters. Oh well. Doom and Strife do.
static const char InputGridChars[INPUTGRID_WIDTH * INPUTGRID_HEIGHT] =
	"ABCDEFGHIJKLM"
	"NOPQRSTUVWXYZ"
	"0123456789+-="
	".,!?@'\":;[]()"
	"<>^#$%&*/_ \b";


CVAR(Bool, m_showinputgrid, false, CVAR_ARCHIVE|CVAR_GLOBALCONFIG)

//=============================================================================
//
//
//
//=============================================================================

// [TP] Added allowcolors
DTextEnterMenu::DTextEnterMenu(DMenu *parent, FFont *dpf, FString textbuffer, int maxlen, bool showgrid, bool allowcolors)
: DMenu(parent)
{
	mEnterString = textbuffer;
	mEnterSize = maxlen;
	mInputGridOkay = (showgrid && (m_showinputgrid == 0)) || (m_showinputgrid >= 1);
	if (mEnterString.Len() > 0)
	{
		InputGridX = INPUTGRID_WIDTH - 1;
		InputGridY = INPUTGRID_HEIGHT - 1;
	}
	else
	{
		// If we are naming a new save, don't start the cursor on "end".
		InputGridX = 0;
		InputGridY = 0;
	}
	AllowColors = allowcolors; // [TP]
	displayFont = dpf;
	CursorSize = displayFont->StringWidth(displayFont->GetCursor());
}

//=============================================================================
//
//
//
//=============================================================================

bool DTextEnterMenu::TranslateKeyboardEvents()
{
	return mInputGridOkay; 
}

//=============================================================================
//
//
//
//=============================================================================

bool DTextEnterMenu::Responder(event_t *ev)
{
	if (ev->type == EV_GUI_Event)
	{
		// Save game and player name string input
		if (ev->subtype == EV_GUI_Char)
		{
			mInputGridOkay = false;
			AppendChar(ev->data1);
			return true;
		}
		char ch = (char)ev->data1;
		if ((ev->subtype == EV_GUI_KeyDown || ev->subtype == EV_GUI_KeyRepeat) && ch == '\b')
		{
			if (mEnterString.Len() > 0)
			{
				mEnterString.DeleteLastCharacter();
			}
		}
		else if (ev->subtype == EV_GUI_KeyDown)
		{
			if (ch == GK_ESCAPE)
			{
				DMenu *parent = mParentMenu;
				parent->MenuEvent(MKEY_Abort, false);
				Close();
				return true;
			}
			else if (ch == '\r')
			{
				if (mEnterString.Len() > 0)
				{
					// [TP] If we allow color codes, colorize the string now.
					//if (AllowColors)
						//mEnterString = mEnterString.Filter();

					DMenu *parent = mParentMenu;
					parent->MenuEvent(MKEY_Input, false);
					Close();
					return true;
				}
			}
		}
		if (ev->subtype == EV_GUI_KeyDown || ev->subtype == EV_GUI_KeyRepeat)
		{
			return true;
		}
	}
	return Super::Responder(ev);
}

//=============================================================================
//
//
//
//=============================================================================

bool DTextEnterMenu::MouseEvent(int type, int x, int y)
{
	const int cell_width = 18 * CleanXfac_1;
	const int cell_height = 16 * CleanYfac_1;
	const int screen_y = screen->GetHeight() - INPUTGRID_HEIGHT * cell_height;
	const int screen_x = (screen->GetWidth() - INPUTGRID_WIDTH * cell_width) / 2;

	if (x >= screen_x && x < screen_x + INPUTGRID_WIDTH * cell_width && y >= screen_y)
	{
		InputGridX = (x - screen_x) / cell_width;
		InputGridY = (y - screen_y) / cell_height;
		if (type == DMenu::MOUSE_Release)
		{
			if (MenuEvent(MKEY_Enter, true))
			{
				//M_MenuSound(CursorSound);
				if (m_use_mouse == 2) InputGridX = InputGridY = -1;
			}
		}
		return true;
	}
	else
	{
		InputGridX = InputGridY = -1;
	}
	return Super::MouseEvent(type, x, y);
}

//=============================================================================
//
//
//
//=============================================================================

void DTextEnterMenu::AppendChar(int ch)
{
	FStringf newstring("%s%c%c", mEnterString.GetChars(), ch, displayFont->GetCursor());
	if (mEnterSize < 0 || displayFont->StringWidth(newstring) < mEnterSize)
	{
		mEnterString.AppendCharacter(ch);
	}
}

//=============================================================================
//
//
//
//=============================================================================

bool DTextEnterMenu::MenuEvent (int key, bool fromcontroller)
{
	if (key == MKEY_Back)
	{
		mParentMenu->MenuEvent(MKEY_Abort, false);
		return Super::MenuEvent(key, fromcontroller);
	}
	if (fromcontroller)
	{
		mInputGridOkay = true;
	}

	if (mInputGridOkay)
	{
		int ch;

		if (InputGridX == -1 || InputGridY == -1)
		{
			InputGridX = InputGridY = 0;
		}
		switch (key)
		{
		case MKEY_Down:
			InputGridY = (InputGridY + 1) % INPUTGRID_HEIGHT;
			return true;

		case MKEY_Up:
			InputGridY = (InputGridY + INPUTGRID_HEIGHT - 1) % INPUTGRID_HEIGHT;
			return true;

		case MKEY_Right:
			InputGridX = (InputGridX + 1) % INPUTGRID_WIDTH;
			return true;

		case MKEY_Left:
			InputGridX = (InputGridX + INPUTGRID_WIDTH - 1) % INPUTGRID_WIDTH;
			return true;

		case MKEY_Clear:
				if (mEnterString.Len() > 0)
				{
					mEnterString.DeleteLastCharacter();
			}
			return true;

		case MKEY_Enter:
			assert(unsigned(InputGridX) < INPUTGRID_WIDTH && unsigned(InputGridY) < INPUTGRID_HEIGHT);
			if (mInputGridOkay)
			{
				ch = InputGridChars[InputGridX + InputGridY * INPUTGRID_WIDTH];
				if (ch == 0)			// end
				{
						if (mEnterString.Len() > 0)
					{
						DMenu *parent = mParentMenu;
						parent->MenuEvent(MKEY_Input, false);
						Close();
						return true;
					}
				}
				else if (ch == '\b')	// bs
				{
					if (mEnterString.Len() > 0)
					{
						mEnterString.DeleteLastCharacter();
					}
				}
				else
				{
					AppendChar(ch);
				}
			}
			return true;

		default:
			break;	// Keep GCC quiet
		}
	}
	return false;
}

//=============================================================================
//
//
//
//=============================================================================

void DTextEnterMenu::Drawer ()
{
	mParentMenu->Drawer();
	if (mInputGridOkay)
	{
		const int cell_width = 18 * CleanXfac;
		const int cell_height = 12 * CleanYfac;
		const int top_padding = cell_height / 2 - displayFont->GetHeight() * CleanYfac / 2;

		// Darken the background behind the character grid.
		// Unless we frame it with a border, I think it looks better to extend the
		// background across the full width of the screen.
		twod->AddColorOnlyQuad(0 /*screen->GetWidth()/2 - 13 * cell_width / 2*/,
			screen->GetHeight() - INPUTGRID_HEIGHT * cell_height,
			screen->GetWidth() /*13 * cell_width*/,
			INPUTGRID_HEIGHT * cell_height, 0xc8000000);

		if (InputGridX >= 0 && InputGridY >= 0)
		{
			// Highlight the background behind the selected character.
			twod->AddColorOnlyQuad(
				InputGridX * cell_width - INPUTGRID_WIDTH * cell_width / 2 + screen->GetWidth() / 2,
				InputGridY * cell_height - INPUTGRID_HEIGHT * cell_height + screen->GetHeight(),
				cell_width, cell_height, PalEntry(255, 255, 248, 220));
		}

		for (int y = 0; y < INPUTGRID_HEIGHT; ++y)
		{
			const int yy = y * cell_height - INPUTGRID_HEIGHT * cell_height + screen->GetHeight();
			for (int x = 0; x < INPUTGRID_WIDTH; ++x)
			{
				int width;
				const int xx = x * cell_width - INPUTGRID_WIDTH * cell_width / 2 + screen->GetWidth() / 2;
				const int ch = InputGridChars[y * INPUTGRID_WIDTH + x];
				auto pic = displayFont->GetChar(ch, CR_DARKGRAY, &width);
				EColorRange color;
				int remap;

				// The highlighted character is yellow; the rest are dark gray.
				color = (x == InputGridX && y == InputGridY) ? CR_YELLOW : CR_DARKGRAY;
				remap = displayFont->GetColorTranslation(color);

				if (pic != NULL)
				{
					// Draw a normal character.
					DrawTexture(twod, pic, xx + cell_width/2 - width*CleanXfac_1/2, yy + top_padding,
						DTA_TranslationIndex, remap,
						DTA_CleanNoMove_1, true,
						TAG_DONE);
				}
				else if (ch == ' ')
				{
					// Draw the space as a box outline. We also draw it 50% wider than it really is.
					const int x1 = xx + cell_width/2 - width * CleanXfac_1 * 3 / 4;
					const int x2 = x1 + width * 3 * CleanXfac_1 / 2;
					const int y1 = yy + top_padding;
					const int y2 = y1 + displayFont->GetHeight() * CleanYfac_1;
					auto palcolor = PalEntry(255, 160, 160, 160);
					twod->AddColorOnlyQuad(x1, y1, x2 - x1, CleanYfac_1, palcolor);	// top
					twod->AddColorOnlyQuad(x1, y2, x2 - x1, CleanYfac_1, palcolor);	// bottom
					twod->AddColorOnlyQuad(x1, y1+CleanYfac_1, CleanXfac_1, y2 - y1, palcolor);	// left
					twod->AddColorOnlyQuad(x2-CleanXfac_1, y1+CleanYfac_1, CleanXfac_1, CleanYfac_1, palcolor);	// right
				}
				else if (ch == '\b' || ch == 0)
				{
					// Draw the backspace and end "characters".
					const char *const str = ch == '\b' ? "BS" : "ED";
					DrawText(twod, NewSmallFont, color,
						xx + cell_width/2 - displayFont->StringWidth(str)*CleanXfac_1/2,
						yy + top_padding, str, DTA_CleanNoMove_1, true, TAG_DONE);
				}
			}
		}
	}
	Super::Drawer();
}