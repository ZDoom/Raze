/*
** optionmenu.cpp
** Handler class for the option menus and associated items
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

#include "v_font.h"
#include "cmdlib.h"
#include "gstrings.h"
#include "d_gui.h"
#include "d_event.h"
#include "c_dispatch.h"
#include "c_console.h"
#include "c_cvars.h"
#include "c_bind.h"
#include "gameconfigfile.h"
#include "menu/menu.h"
#include "v_draw.h"
#include "v_2ddrawer.h"
#include "v_video.h"

//=============================================================================
//
// Draws a string in the console font, scaled to the 8x8 cells
// used by the default console font.
//
//=============================================================================

FFont *OptionFont()
{
	return NewSmallFont;
}

int OptionHeight()
{
	return OptionFont()->GetHeight();
}

int OptionWidth(const char * s)
{
	return OptionFont()->StringWidth(s);
}

void DrawOptionText(int x, int y, int color, const char *text, bool grayed)
{
	PalEntry overlay = grayed? PalEntry(96,48,0,0) : PalEntry(0,0,0);
	DrawText (twod, OptionFont(), color, x, y, text, DTA_CleanNoMove_1, true, DTA_ColorOverlay, overlay, TAG_END);
}

int DOptionMenu::GetPosition()
{
	return mDesc->mPosition * screen->GetHeight() * 2 / CleanYfac_1 / 1080;	// y position uses a 1920x1080 screen as reference but has to adjust to scaled 320x200 content.
}

//=============================================================================
//
//
//
//=============================================================================

DOptionMenu::DOptionMenu(DMenu *parent, FOptionMenuDescriptor *desc)
: DMenu(parent)
{
	CanScrollUp = false;
	CanScrollDown = false;
	VisBottom = 0;
	mFocusControl = NULL;
	Init(parent, desc);
}

//=============================================================================
//
//
//
//=============================================================================

void DOptionMenu::Init(DMenu *parent, FOptionMenuDescriptor *desc)
{
	mParentMenu = parent;
	mDesc = desc;
	if (mDesc != NULL && mDesc->mSelectedItem == -1) mDesc->mSelectedItem = FirstSelectable();

}

//=============================================================================
//
//
//
//=============================================================================

int DOptionMenu::FirstSelectable()
{
	if (mDesc != NULL)
	{
		// Go down to the first selectable item
		int i = -1;
		do
		{
			i++;
		}
		while (i < (int)mDesc->mItems.Size() && !mDesc->mItems[i]->Selectable());
		if (i>=0 && i < (int)mDesc->mItems.Size()) return i;
	}
	return -1;
}

//=============================================================================
//
//
//
//=============================================================================

FOptionMenuItem *DOptionMenu::GetItem(FName name)
{
	for(unsigned i=0;i<mDesc->mItems.Size(); i++)
	{
		FName nm = mDesc->mItems[i]->GetAction(NULL);
		if (nm == name) return mDesc->mItems[i];
	}
	return NULL;
}

//=============================================================================
//
//
//
//=============================================================================

bool DOptionMenu::Responder (event_t *ev)
{
	if (ev->type == EV_GUI_Event)
	{
		if (ev->subtype == EV_GUI_WheelUp)
		{
			int scrollamt = std::min(2, mDesc->mScrollPos);
			mDesc->mScrollPos -= scrollamt;
			return true;
		}
		else if (ev->subtype == EV_GUI_WheelDown)
		{
			if (CanScrollDown)
			{
				if (VisBottom < (int)(mDesc->mItems.Size()-2))
				{
					mDesc->mScrollPos += 2;
					VisBottom += 2;
				}
				else
				{
					mDesc->mScrollPos++;
					VisBottom++;
				}
			}
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

bool DOptionMenu::MenuEvent (int mkey, bool fromcontroller)
{
	int startedAt = mDesc->mSelectedItem;

	switch (mkey)
	{
	case MKEY_Up:
		if (mDesc->mSelectedItem == -1)
		{
			mDesc->mSelectedItem = FirstSelectable();
			break;
		}
		do
		{
			--mDesc->mSelectedItem;

			if (mDesc->mScrollPos > 0 &&
				mDesc->mSelectedItem <= mDesc->mScrollTop + mDesc->mScrollPos)
			{
				mDesc->mScrollPos = std::max(mDesc->mSelectedItem - mDesc->mScrollTop - 1, 0);
			}

			if (mDesc->mSelectedItem < 0) 
			{
				// Figure out how many lines of text fit on the menu
				int y = GetPosition();

				y *= CleanYfac_1;
				int	rowheight = OptionSettings.mLinespacing * CleanYfac_1;
				int maxitems = (screen->GetHeight() - rowheight - y) / rowheight + 1;

				mDesc->mScrollPos = std::max(0, (int)mDesc->mItems.Size() - maxitems + mDesc->mScrollTop);
				mDesc->mSelectedItem = mDesc->mItems.Size()-1;
			}
		}
		while (!mDesc->mItems[mDesc->mSelectedItem]->Selectable() && mDesc->mSelectedItem != startedAt);
		break;

	case MKEY_Down:
		if (mDesc->mSelectedItem == -1)
		{
			mDesc->mSelectedItem = FirstSelectable();
			break;
		}
		do
		{
			++mDesc->mSelectedItem;
			
			if (CanScrollDown && mDesc->mSelectedItem == VisBottom)
			{
				mDesc->mScrollPos++;
				VisBottom++;
			}
			if (mDesc->mSelectedItem >= (int)mDesc->mItems.Size()) 
			{
				if (startedAt == -1)
				{
					mDesc->mSelectedItem = -1;
					mDesc->mScrollPos = -1;
					break;
				}
				else
				{
					mDesc->mSelectedItem = 0;
					mDesc->mScrollPos = 0;
				}
			}
		}
		while (!mDesc->mItems[mDesc->mSelectedItem]->Selectable() && mDesc->mSelectedItem != startedAt);
		break;

	case MKEY_PageUp:
		if (mDesc->mScrollPos > 0)
		{
			mDesc->mScrollPos -= VisBottom - mDesc->mScrollPos - mDesc->mScrollTop;
			if (mDesc->mScrollPos < 0)
			{
				mDesc->mScrollPos = 0;
			}
			if (mDesc->mSelectedItem != -1)
			{
				mDesc->mSelectedItem = mDesc->mScrollTop + mDesc->mScrollPos + 1;
				while (!mDesc->mItems[mDesc->mSelectedItem]->Selectable())
				{
					if (++mDesc->mSelectedItem >= (int)mDesc->mItems.Size())
					{
						mDesc->mSelectedItem = 0;
					}
				}
				if (mDesc->mScrollPos > mDesc->mSelectedItem)
				{
					mDesc->mScrollPos = mDesc->mSelectedItem;
				}
			}
		}
		break;

	case MKEY_PageDown:
		if (CanScrollDown)
		{
			int pagesize = VisBottom - mDesc->mScrollPos - mDesc->mScrollTop;
			mDesc->mScrollPos += pagesize;
			if (mDesc->mScrollPos + mDesc->mScrollTop + pagesize > (int)mDesc->mItems.Size())
			{
				mDesc->mScrollPos = mDesc->mItems.Size() - mDesc->mScrollTop - pagesize;
			}
			if (mDesc->mSelectedItem != -1)
			{
				mDesc->mSelectedItem = mDesc->mScrollTop + mDesc->mScrollPos;
				while (!mDesc->mItems[mDesc->mSelectedItem]->Selectable())
				{
					if (++mDesc->mSelectedItem >= (int)mDesc->mItems.Size())
					{
						mDesc->mSelectedItem = 0;
					}
				}
				if (mDesc->mScrollPos > mDesc->mSelectedItem)
				{
					mDesc->mScrollPos = mDesc->mSelectedItem;
				}
			}
		}
		break;

	case MKEY_Enter:
		if (mDesc->mSelectedItem >= 0 && mDesc->mItems[mDesc->mSelectedItem]->Activate(mDesc->mMenuName)) 
		{
			return true;
		}
		// fall through to default
	default:
		if (mDesc->mSelectedItem >= 0 && 
			mDesc->mItems[mDesc->mSelectedItem]->MenuEvent(mkey, fromcontroller)) return true;
		return Super::MenuEvent(mkey, fromcontroller);
	}

	if (mDesc->mSelectedItem != startedAt)
	{
		M_MenuSound(CursorSound);
	}
	return true;
}

//=============================================================================
//
//
//
//=============================================================================

bool DOptionMenu::MouseEvent(int type, int x, int y)
{
	y = (y / CleanYfac_1) - mDesc->mDrawTop;

	if (mFocusControl)
	{
		mFocusControl->MouseEvent(type, x, y);
		return true;
	}
	else
	{
		int yline = (y / OptionSettings.mLinespacing);
		if (yline >= mDesc->mScrollTop)
		{
			yline += mDesc->mScrollPos;
		}
		if ((unsigned)yline < mDesc->mItems.Size() && mDesc->mItems[yline]->Selectable())
		{
			if (yline != mDesc->mSelectedItem)
			{
				mDesc->mSelectedItem = yline;
				//M_MenuSound(CursorSound); too noisy

			}
			mDesc->mItems[yline]->MouseEvent(type, x, y);
			return true;
		}
	}
	mDesc->mSelectedItem = -1;
	return Super::MouseEvent(type, x, y);
}

//=============================================================================
//
//
//
//=============================================================================

void DOptionMenu::Ticker ()
{
	Super::Ticker();
	for(unsigned i=0;i<mDesc->mItems.Size(); i++)
	{
		mDesc->mItems[i]->Ticker();
	}
}

//=============================================================================
//
//
//
//=============================================================================
int DOptionMenu::GetIndent()
{
	int indent = std::max(0, (mDesc->mIndent + 40) - CleanWidth_1 / 2);
	return screen->GetWidth() / 2 + indent * CleanXfac_1;
}

void DOptionMenu::Drawer ()
{
	int y = GetPosition();

	if (mDesc->mTitle.IsNotEmpty())
	{
		gi->DrawMenuCaption(origin, GStrings.localize(mDesc->mTitle));
	}
	mDesc->mDrawTop = y;
	int fontheight = OptionSettings.mLinespacing * CleanYfac_1;
	y *= CleanYfac_1;

	int indent = GetIndent();

	int ytop = y + mDesc->mScrollTop * 8 * CleanYfac_1;
	int lastrow = screen->GetHeight() - OptionFont()->GetHeight() * CleanYfac_1;

	unsigned i;
	for (i = 0; i < mDesc->mItems.Size() && y <= lastrow; i++, y += fontheight)
	{
		// Don't scroll the uppermost items
		if ((int)i == mDesc->mScrollTop)
		{
			i += mDesc->mScrollPos;
			if (i >= mDesc->mItems.Size()) break;	// skipped beyond end of menu 
		}
		bool isSelected = mDesc->mSelectedItem == (int)i;
		int cur_indent = mDesc->mItems[i]->Draw(mDesc, y, indent, isSelected);
		if (cur_indent >= 0 && isSelected && mDesc->mItems[i]->Selectable())
		{
			if ((((DMenu::MenuTime>>2)%8) < 6) || DMenu::CurrentMenu != this)
			{
				DrawOptionText(cur_indent + 3 * CleanXfac_1, y, OptionSettings.mFontColorSelection, "◄");
				//M_DrawConText(OptionSettings.mFontColorSelection, cur_indent + 3 * CleanXfac_1, y+fontheight-9*CleanYfac_1, "\xd");
			}
		}
	}

	CanScrollUp = (mDesc->mScrollPos > 0);
	CanScrollDown = (i < mDesc->mItems.Size());
	VisBottom = i - 1;

	if (CanScrollUp)
	{
		DrawOptionText(screen->GetWidth() - 11 * CleanXfac_1, ytop, OptionSettings.mFontColorSelection, "▲");
		//M_DrawConText(CR_ORANGE, 3 * CleanXfac_1, ytop, "\x1a");
	}
	if (CanScrollDown)
	{
		DrawOptionText(screen->GetWidth() - 11 * CleanXfac_1 , y - 8*CleanYfac_1, OptionSettings.mFontColorSelection, "▼");
		//M_DrawConText(CR_ORANGE, 3 * CleanXfac_1, y - 8*CleanYfac_1, "\x1b");
	}
	Super::Drawer();
}


//=============================================================================
//
// base class for menu items
//
//=============================================================================

FOptionMenuItem::~FOptionMenuItem()
{
}

int FOptionMenuItem::Draw(FOptionMenuDescriptor *desc, int y, int indent, bool selected)
{
	return indent;
}

bool FOptionMenuItem::Selectable()
{
	return true;
}

bool FOptionMenuItem::MouseEvent(int type, int x, int y)
{
	if (Selectable() && type == DMenu::MOUSE_Release)
	{
		return DMenu::CurrentMenu->MenuEvent(MKEY_Enter, true);
	}
	return false;
}


int  FOptionMenuItem::GetIndent()
{
	if (mCentered) return 0;
	if (screen->GetWidth() < 640) return screen->GetWidth() / 2;
	return OptionWidth(GStrings.localize(mLabel));
}

void FOptionMenuItem::drawText(int x, int y, int color, const char * text, bool grayed)
{
	DrawOptionText(x, y, color, text, grayed);
}

int FOptionMenuItem::drawLabel(int indent, int y, EColorRange color, bool grayed)
{
	const char *label = GStrings.localize(mLabel);
	int x;
	int w = OptionWidth(label) * CleanXfac_1;
	if (!mCentered) x = indent - w;
	else x = (screen->GetWidth() - w) / 2;
	DrawOptionText(x, y, color, label, grayed);
	return x;
}

void FOptionMenuItem::drawValue(int indent, int y, int color, const char *text, bool grayed)
{
	DrawOptionText(indent + CursorSpace(), y, color, text, grayed);
}

int FOptionMenuItem::CursorSpace()
{
	return (14 * CleanXfac_1);
}

void FOptionMenuDescriptor::CalcIndent()
{
	// calculate the menu indent
	int widest = 0, thiswidth;

	for (unsigned i = 0; i < mItems.Size(); i++)
	{
		thiswidth = mItems[i]->GetIndent();
		if (thiswidth > widest) widest = thiswidth;
	}
	mIndent =  widest + 4;
}

//=============================================================================
//
//
//
//=============================================================================

FOptionMenuItem *FOptionMenuDescriptor::GetItem(FName name)
{
	for(unsigned i=0;i<mItems.Size(); i++)
	{
		FName nm = mItems[i]->GetAction(NULL);
		if (nm == name) return mItems[i];
	}
	return NULL;
}

class PlayerMenu : public DOptionMenu
{
	using Super = DOptionMenu;

public:
	void Drawer()
	{
		// Hack: The team item is #3. This part doesn't work properly yet.
		gi->DrawPlayerSprite(origin, (mDesc->mSelectedItem == 3));
		Super::Drawer();
	}
};

static TMenuClassDescriptor<PlayerMenu> _ppm("NewPlayerMenu");

void RegisterOptionMenus()
{
	menuClasses.Push(&_ppm);
}
