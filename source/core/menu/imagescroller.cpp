/*
** imagescroller.cpp
** Scrolls through multiple fullscreen image pages,
**
**---------------------------------------------------------------------------
** Copyright 2019 Christoph Oelckers
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
#include "baselayer.h"
#include "gamecontrol.h"
#include "build.h"
#include "zstring.h"

//=============================================================================
//
// Fullscreen image drawer (move to its own source file!)
//
//=============================================================================

void ImageScreen::Drawer()
{
	if (mDesc == nullptr)
	{
		// don't let bogus definitions crash this.
	}
	else if (mDesc->type == 0)
	{
		auto tileindexp = NameToTileIndex.CheckKey(FName(mDesc->text, true));
		int tileindex = 0;
		if (tileindexp == nullptr)
		{
			// If this isn't a name, try a literal tile index;
			auto c = mDesc->text.GetChars();
			if (*c == '#') tileindex = (int)strtoll(c + 1, nullptr, 0);
			// Error out if the screen cannot be found, this is always a definition error that needs to be reported.
			else I_Error("Invalid menu screen '%s'", mDesc->text.GetChars());
		}
		else tileindex = *tileindexp;
		if (!gi->DrawSpecialScreen(origin, tileindex)) // allows the front end to do custom handling for a given image.
		{
			DrawTexture(twod, tileGetTexture(tileindex), origin.X, origin.Y, DTA_FullscreenScale, 3, DTA_VirtualWidth, 320, DTA_VirtualHeight, 200,
						DTA_TopLeft, true, DTA_LegacyRenderStyle, STYLE_Normal, TAG_DONE);
		}
	}
	else if (mDesc->type > 0)
	{
		gi->DrawCenteredTextScreen(origin, mDesc->text, mDesc->type);
	}
	// QAVs are handled in the Blood frontend. Maybe they should be moved out? Stuff for later, but this is a feature where it is feasible.
}

ImageScreen* DImageScrollerMenu::newImageScreen(FImageScrollerDescriptor::ScrollerItem* desc)
{
	return new ImageScreen(desc);
}

void DImageScrollerMenu::Init(DMenu* parent, FImageScrollerDescriptor* desc)
{
	mParentMenu = parent;
	index = 0;
	mDesc = desc;
	canAnimate = !!(mDesc->mFlags & LMF_Animate);

	mCurrent = newImageScreen(&mDesc->mItems[0]);
	mCurrent->canAnimate = canAnimate;
}

bool DImageScrollerMenu::MenuEvent(int mkey, bool fromcontroller)
{
	if (mDesc->mItems.Size() <= 1)
	{
		if (mkey == MKEY_Enter) mkey = MKEY_Back;
		else if (mkey == MKEY_Right || mkey == MKEY_Left) return true;
	}
	switch (mkey)
	{
	case MKEY_Back:
		// Before going back the currently running transition must be terminated.
		pageTransition.previous = nullptr;
		if (pageTransition.current) pageTransition.current->origin = { 0,0 };
		return DMenu::MenuEvent(mkey, fromcontroller);


	case MKEY_Left:
		if (pageTransition.previous == nullptr)
		{
			if (--index < 0) index = mDesc->mItems.Size() - 1;
			auto next = newImageScreen(&mDesc->mItems[index]);
			next->canAnimate = canAnimate;
			if (!pageTransition.StartTransition(mCurrent, next, MA_Return))
			{
				delete mCurrent;
			}
			mCurrent = next;
			gi->MenuSound(ChooseSound);
		}
		return true;

	case MKEY_Right:
	case MKEY_Enter:
		if (pageTransition.previous == nullptr)
		{
			int oldindex = index;
			if (++index >= (int)mDesc->mItems.Size()) index = 0;

			auto next = newImageScreen(&mDesc->mItems[index]);
			next->canAnimate = canAnimate;
			if (!pageTransition.StartTransition(mCurrent, next, MA_Advance))
			{
				delete mCurrent;
			}
			mCurrent = next;
			gi->MenuSound(ChooseSound);
		}
		return true;

	default:
		return DMenu::MenuEvent(mkey, fromcontroller);
	}
}

bool DImageScrollerMenu::MouseEvent(int type, int x, int y)
{
	// Todo: Implement some form of drag event to switch between pages.
	if (type == MOUSE_Release)
	{
		return MenuEvent(MKEY_Enter, false);
	}

	return DMenu::MouseEvent(type, x, y);
}

void DImageScrollerMenu::Ticker()
{
}

void DImageScrollerMenu::Drawer()
{
	if (pageTransition.previous != nullptr)
	{
		auto res = pageTransition.Draw();
		if (res) return;
		delete pageTransition.previous;
		pageTransition.previous = nullptr;
	}
	mCurrent->origin = origin;
	mCurrent->Drawer();
	mCurrent->origin = {};
}

