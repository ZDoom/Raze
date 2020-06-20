/*
** listmenu.cpp
** A simple menu consisting of a list of items
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
#include "menu.h"
#include "v_draw.h"
#include "gamecontrol.h"
#include "build.h"
#include "v_video.h"

//=============================================================================
//
//
//
//=============================================================================

DListMenu::DListMenu(DMenu *parent, FListMenuDescriptor *desc)
: DMenu(parent)
{
	mDesc = NULL;
	if (desc != NULL) Init(parent, desc);
}

//=============================================================================
//
//
//
//=============================================================================

void DListMenu::Init(DMenu *parent, FListMenuDescriptor *desc)
{
	mParentMenu = parent;
	mDesc = desc;
	canAnimate = !!(mDesc->mFlags & LMF_Animate);
	if (mDesc->mScriptId >= 0) scriptID = mDesc->mScriptId;
#if 0
	if (desc->mCenter)
	{
		int center = 160;
		for(unsigned i=0;i<mDesc->mItems.Size(); i++)
		{
			int xpos = mDesc->mItems[i]->GetX();
			int width = mDesc->mItems[i]->GetWidth();
			int curx = mDesc->mSelectOfsX;

			if (width > 0 && mDesc->mItems[i]->Selectable())
			{
				int left = 160 - (width - curx) / 2 - curx;
				if (left < center) center = left;
			}
		}
		for(unsigned i=0;i<mDesc->mItems.Size(); i++)
		{
			int width = mDesc->mItems[i]->GetWidth();

			if (width > 0)
			{
				mDesc->mItems[i]->SetX(center);
			}
		}
	}
#endif
}

//=============================================================================
//
//
//
//=============================================================================

FListMenuItem *DListMenu::GetItem(FName name)
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

bool DListMenu::Responder (event_t *ev)
{
	if (ev->type == EV_GUI_Event)
	{
		if (ev->subtype == EV_GUI_KeyDown)
		{
			int ch = tolower (ev->data1);

			for(unsigned i = mDesc->mSelectedItem + 1; i < mDesc->mItems.Size(); i++)
			{
				if (mDesc->mItems[i]->CheckHotkey(ch))
				{
					mDesc->mSelectedItem = i;
					SelectionChanged();
					M_MenuSound(CursorSound);
					return true;
				}
			}
			for(int i = 0; i < mDesc->mSelectedItem; i++)
			{
				if (mDesc->mItems[i]->CheckHotkey(ch))
				{
					mDesc->mSelectedItem = i;
					SelectionChanged();
					M_MenuSound(CursorSound);
					return true;
				}
			}
		}
	}
	return Super::Responder(ev);
}

//=============================================================================
//
//
//
//=============================================================================

bool DListMenu::MenuEvent (int mkey, bool fromcontroller)
{
	int startedAt = mDesc->mSelectedItem;

	switch (mkey)
	{
	case MKEY_Up:
		do
		{
			if (--mDesc->mSelectedItem < 0) mDesc->mSelectedItem = mDesc->mItems.Size()-1;
		}
		while (!mDesc->mItems[mDesc->mSelectedItem]->Selectable() && mDesc->mSelectedItem != startedAt);
		SelectionChanged();
		M_MenuSound(CursorSound);
		return true;

	case MKEY_Down:
		do
		{
			if (++mDesc->mSelectedItem >= (int)mDesc->mItems.Size()) mDesc->mSelectedItem = 0;
		}
		while (!mDesc->mItems[mDesc->mSelectedItem]->Selectable() && mDesc->mSelectedItem != startedAt);
		SelectionChanged();
		M_MenuSound(CursorSound);
		return true;

	case MKEY_Enter:
		if (mDesc->mSelectedItem >= 0 && mDesc->mItems[mDesc->mSelectedItem]->Activate(mDesc->mMenuName))
		{
			M_MenuSound(AdvanceSound);
		}
		return true;

	default:
		return Super::MenuEvent(mkey, fromcontroller);
	}
}

//=============================================================================
//
//
//
//=============================================================================

bool DListMenu::MouseEvent(int type, int xx, int yy)
{
	int sel = -1;

	// convert x/y from screen to virtual coordinates, according to CleanX/Yfac use in DrawTexture
	//x = ((x - (screen->GetWidth() / 2)) / CleanXfac) + 160;
	//y = ((y - (screen->GetHeight() / 2)) / CleanYfac) + 100;


	int width43 = (screen->GetHeight() * 4 / 3);
	int x = (xx - (screen->GetWidth() - width43) / 2) * 320 / width43;
	int y = yy * 200 / screen->GetHeight();

	if (mFocusControl != NULL)
	{
		mFocusControl->MouseEvent(type, x, y);
		return true;
	}
	else
	{
		if ((mDesc->mWLeft <= 0 || x > mDesc->mWLeft) &&
			(mDesc->mWRight <= 0 || x < mDesc->mWRight))
		{
			for(unsigned i=0;i<mDesc->mItems.Size(); i++)
			{
				if (mDesc->mItems[i]->CheckCoordinate(x, y))
				{
					if ((int)i != mDesc->mSelectedItem)
					{
						// no sound. This is too noisy.
					}
					mDesc->mSelectedItem = i;
					SelectionChanged();
					mDesc->mItems[i]->MouseEvent(type, x, y);
					return true;
				}
			}
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

void DListMenu::Ticker ()
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

void DListMenu::PreDraw()
{
	if (mDesc->mCaption.IsNotEmpty())
	{
		gi->DrawMenuCaption(origin, GStrings.localize(mDesc->mCaption));
	}
}

void DListMenu::Drawer ()
{
	PreDraw();
	for(unsigned i=0;i<mDesc->mItems.Size(); i++)
	{
		mDesc->mItems[i]->Drawer(this, origin, mDesc->mSelectedItem == (int)i);
	}
	if (mDesc->mSelectedItem >= 0 && mDesc->mSelectedItem < (int)mDesc->mItems.Size())
		mDesc->mItems[mDesc->mSelectedItem]->DrawSelector(mDesc->mSelectOfsX, mDesc->mSelectOfsY, mDesc->mSelector);
	PostDraw();
	Super::Drawer();
}

//=============================================================================
//
// base class for menu items
//
//=============================================================================

FListMenuItem::~FListMenuItem()
{
}

bool FListMenuItem::CheckCoordinate(int x, int y)
{
	return false;
}

void FListMenuItem::Ticker()
{
}

void FListMenuItem::Drawer(DListMenu* menu, const DVector2& origin, bool selected)
{
}

bool FListMenuItem::Selectable()
{
	return false;
}

void FListMenuItem::DrawSelector(int xofs, int yofs, FGameTexture *tex)
{
	if (!tex)
	{
		if ((DMenu::MenuTime%8) < 6)
		{
			DrawText(twod, ConFont, OptionSettings.mFontColorSelection,
				(mXpos + xofs - 160) * CleanXfac + screen->GetWidth() / 2,
				(mYpos + yofs - 100) * CleanYfac + screen->GetHeight() / 2,
				"\xd",
				DTA_CellX, 8 * CleanXfac,
				DTA_CellY, 8 * CleanYfac,
				TAG_DONE);
		}
	}
	else
	{
		DrawTexture (twod, tex, mXpos + xofs, mYpos + yofs, DTA_Clean, true, TAG_DONE);
	}
}

bool FListMenuItem::Activate(FName)
{
	return false;	// cannot be activated
}

FName FListMenuItem::GetAction(int *pparam)
{
	return mAction;
}

bool FListMenuItem::SetString(int i, const char *s)
{
	return false;
}

bool FListMenuItem::GetString(int i, char *s, int len)
{
	return false;
}

bool FListMenuItem::SetValue(int i, int value)
{
	return false;
}

bool FListMenuItem::GetValue(int i, int *pvalue)
{
	return false;
}

void FListMenuItem::Enable(bool on)
{
	mEnabled = on;
}

bool FListMenuItem::MenuEvent(int mkey, bool fromcontroller)
{
	return false;
}

bool FListMenuItem::MouseEvent(int type, int x, int y)
{
	return false;
}

bool FListMenuItem::CheckHotkey(int c) 
{ 
	return false; 
}

int FListMenuItem::GetWidth() 
{ 
	return 0; 
}


//=============================================================================
//
// static patch
//
//=============================================================================

FListMenuItemStaticPatch::FListMenuItemStaticPatch(int x, int y, FGameTexture *patch, bool centered)
: FListMenuItem(x, y)
{
	mTexture = patch;
	mCentered = centered;
}
	
void FListMenuItemStaticPatch::Drawer(DListMenu* menu, const DVector2& origin, bool selected)
{
	if (!mTexture)
	{
		return;
	}

	int x = mXpos;
	FGameTexture *tex = mTexture;
	if (mYpos >= 0)
	{
		if (mCentered) x -= tex->GetDisplayWidth()/2;
		DrawTexture (twod, tex, x, mYpos, DTA_Clean, true, TAG_DONE);
	}
	else
	{
		int x = (mXpos - 160) * CleanXfac + (screen->GetWidth()>>1);
		if (mCentered) x -= (tex->GetDisplayWidth()*CleanXfac)/2;
		DrawTexture (twod, tex, x, -mYpos*CleanYfac, DTA_CleanNoMove, true, TAG_DONE);
	}
}

//=============================================================================
//
// static text
//
//=============================================================================

FListMenuItemStaticText::FListMenuItemStaticText(int x, int y, const char *text, FFont *font, EColorRange color, bool centered)
: FListMenuItem(x, y)
{
	mText = text;
	mFont = font;
	mColor = color;
	mCentered = centered;
}
	
void FListMenuItemStaticText::Drawer(DListMenu* menu, const DVector2& origin, bool selected)
{
	const char *text = mText;
	if (text != NULL)
	{
		if (mYpos >= 0)
		{
			int x = mXpos;
			if (mCentered) x -= mFont->StringWidth(text)/2;
			DrawText(twod, mFont, mColor, x, mYpos, text, DTA_Clean, true, TAG_DONE);
		}
		else
		{
			int x = (mXpos - 160) * CleanXfac + (screen->GetWidth()>>1);
			if (mCentered) x -= (mFont->StringWidth(text)*CleanXfac)/2;
			DrawText (twod, mFont, mColor, x, -mYpos*CleanYfac, text, DTA_CleanNoMove, true, TAG_DONE);
		}
	}
}

FListMenuItemStaticText::~FListMenuItemStaticText()
{
	if (mText != NULL) delete [] mText;
}

//=============================================================================
//
// native static text item
//
//=============================================================================

FListMenuItemNativeStaticText::FListMenuItemNativeStaticText(int x, int y, const FString& text, int fontnum, int palnum, bool centered)
	: FListMenuItem(x, y)
{
	mText = text;
	mFontnum = fontnum;
	mPalnum = palnum;
	mCentered = centered;
}

void FListMenuItemNativeStaticText::Drawer(DListMenu* menu, const DVector2& origin, bool selected)
{
	const char* text = mText;
	if (mText.Len() && !mHidden)
	{
		gi->DrawNativeMenuText(mFontnum, mPalnum, origin.X + mXpos, origin.Y + mYpos, 1.f, GStrings.localize(text), menu->Descriptor()->mFlags);
	}
}

//=============================================================================
//
// base class for selectable items
//
//=============================================================================

FListMenuItemSelectable::FListMenuItemSelectable(int x, int y, int height, FName action, int param)
: FListMenuItem(x, y, action)
{
	mHeight = height;
	mParam = param;
	mHotkey = 0;
}

bool FListMenuItemSelectable::CheckCoordinate(int x, int y)
{
	return mEnabled && y >= mYpos && y < mYpos + mHeight;	// no x check here
}

bool FListMenuItemSelectable::Selectable()
{
	return mEnabled && !mHidden;
}

bool FListMenuItemSelectable::Activate(FName caller)
{
	return M_SetMenu(mAction, mParam, caller);
}

FName FListMenuItemSelectable::GetAction(int *pparam)
{
	if (pparam != NULL) *pparam = mParam;
	return mAction;
}

bool FListMenuItemSelectable::CheckHotkey(int c) 
{ 
	return c == tolower(mHotkey); 
}

bool FListMenuItemSelectable::MouseEvent(int type, int x, int y)
{
	if (type == DMenu::MOUSE_Release)
	{
		if (NULL != CurrentMenu && CurrentMenu->MenuEvent(MKEY_Enter, true))
		{
			return true;
		}
	}
	return false;
}

//=============================================================================
//
// text item
//
//=============================================================================

FListMenuItemText::FListMenuItemText(int x, int y, int height, int hotkey, const FString &text, FFont *font, EColorRange color, EColorRange color2, FName child, int param)
: FListMenuItemSelectable(x, y, height, child, param)
{
	mText = text;
	/*
	mFont = font;
	mColor = color;
	mColorSelected = color2;
	*/
	mFont = NewSmallFont;
	mColor = CR_RED;
	mColorSelected = CR_GOLD;
	mHotkey = hotkey;
}

FListMenuItemText::~FListMenuItemText()
{
}

void FListMenuItemText::Drawer(DListMenu* menu, const DVector2& origin, bool selected)
{
	const char *text = mText;
	if (mText.Len())
	{
		DrawText(twod, mFont, selected ? mColorSelected : mColor, mXpos, mYpos, text, DTA_Clean, true, TAG_DONE);
	}
}

int FListMenuItemText::GetWidth() 
{ 
	const char *text = mText;
	if (mText.Len())
	{
		return mFont->StringWidth(GStrings.localize(text)); 
	}
	return 1;
}


//=============================================================================
//
// native text item
//
//=============================================================================

FListMenuItemNativeText::FListMenuItemNativeText(int x, int y, int height, int hotkey, const FString& text, int fontnum, int palnum, float fontscale, FName child, int param)
	: FListMenuItemSelectable(x, y, height, child, param)
{
	mText = text;
	mFontnum = NIT_BigFont;
	mPalnum = NIT_ActiveColor;
	mFontscale = fontscale;
	mHotkey = hotkey;
}

FListMenuItemNativeText::~FListMenuItemNativeText()
{
}

void FListMenuItemNativeText::Drawer(DListMenu* menu, const DVector2& origin, bool selected)
{
	const char* text = mText;
	if (mText.Len() && !mHidden)
	{
		auto state = selected ? NIT_SelectedState : mEnabled ? NIT_ActiveState : NIT_InactiveState;
		gi->DrawNativeMenuText(mFontnum, state, origin.X + mXpos, origin.Y + mYpos, 1.f, GStrings.localize(text), menu->Descriptor()->mFlags);
	}
} 

int FListMenuItemNativeText::GetWidth()
{
	return 1;
}


//=============================================================================
//
// patch item
//
//=============================================================================

FListMenuItemPatch::FListMenuItemPatch(int x, int y, int height, int hotkey, FGameTexture *patch, FName child, int param)
: FListMenuItemSelectable(x, y, height, child, param)
{
	mHotkey = hotkey;
	mTexture = patch;
}

void FListMenuItemPatch::Drawer(DListMenu* menu, const DVector2& origin, bool selected)
{
	DrawTexture (twod, mTexture, mXpos, mYpos, DTA_Clean, true, TAG_DONE);
}

int FListMenuItemPatch::GetWidth() 
{
	return mTexture
		?  mTexture->GetDisplayWidth() 
		: 0;
}

