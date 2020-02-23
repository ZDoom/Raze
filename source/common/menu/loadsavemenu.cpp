/*
** loadsavemenu.cpp
** The load game and save game menus
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

#include "menu/menu.h"
#include "version.h"
#include "m_png.h"
#include "filesystem.h"
#include "v_text.h"
#include "d_event.h"
#include "gstrings.h"
#include "d_gui.h"
#include "v_draw.h"
#include "files.h"
#include "resourcefile.h"
#include "savegamehelp.h"
#include "i_specialpaths.h"
#include "../../platform/win32/i_findfile.h"	// This is a temporary direct path. Needs to be fixed when stuff gets cleaned up.



class DLoadSaveMenu : public DListMenu
{
	using Super = DListMenu;

protected:

	int Selected = 0;
	int TopItem = 0;


	int savepicLeft;
	int savepicTop;
	int savepicWidth;
	int savepicHeight;

	int rowHeight;
	int listboxLeft;
	int listboxTop;
	int listboxWidth;
	
	int listboxRows;
	int listboxHeight;
	int listboxRight;
	int listboxBottom;

	int commentLeft;
	int commentTop;
	int commentWidth;
	int commentHeight;
	int commentRight;
	int commentBottom;
	int commentRows;

	double FontScale;
	DTextEnterMenu *mInput = nullptr;
	TArray<FBrokenLines> BrokenSaveComment;
	bool mEntering = false;


	//=============================================================================
	//
	// End of static savegame maintenance code
	//
	//=============================================================================

	DLoadSaveMenu()
	{
		savegameManager.ReadSaveStrings();
	}

	void Init(DMenu* parent, FListMenuDescriptor* desc) override
	{
		Super::Init(parent, desc);
		int Width43 = screen->GetHeight() * 4 / 3;
		int Left43 = (screen->GetWidth() - Width43) / 2;
		float wScale = Width43 / 640.;
		savepicLeft = Left43 + int(20 * wScale);
		savepicTop = mDesc->mYpos * screen->GetHeight() / 200 ;
		savepicWidth = int(240 * wScale);
		savepicHeight = int(180 * wScale);


		FontScale = max(screen->GetHeight() / 480, 1);
		rowHeight = std::max(int((NewConsoleFont->GetHeight() + 1) * FontScale), 1);
		listboxLeft = savepicLeft + savepicWidth + int(20 * wScale);
		listboxTop = savepicTop;
		listboxWidth = Width43 + Left43 - listboxLeft - int(30 * wScale);
		int listboxHeight1 = screen->GetHeight() - listboxTop - int(20*wScale);
		listboxRows = (listboxHeight1 - 1) / rowHeight;
		listboxHeight = listboxRows * rowHeight + 1;
		listboxRight = listboxLeft + listboxWidth;
		listboxBottom = listboxTop + listboxHeight;

		commentLeft = savepicLeft;
		commentTop = savepicTop + savepicHeight + int(16 * wScale);
		commentWidth = savepicWidth;
		commentHeight = listboxHeight - savepicHeight - (16 * wScale);
		commentRight = commentLeft + commentWidth;
		commentBottom = commentTop + commentHeight;
		commentRows = commentHeight / rowHeight;
		UpdateSaveComment();
	}

	//=============================================================================
	//
	//
	//
	//=============================================================================

	void Drawer() override
	{
		Super::Drawer();

		int i;
		unsigned j;
		bool didSeeSelected = false;

		// Draw picture area
		/*
		if (gameaction == ga_loadgame || gameaction == ga_loadgamehidecon || gameaction == ga_savegame)
		{
			return;
		}
		*/

		PalEntry frameColor(255, 80, 80, 80);	// todo: pick a proper color per game.
		PalEntry fillColor(160, 0, 0, 0);
		DrawFrame(twod, frameColor, savepicLeft, savepicTop, savepicWidth, savepicHeight, -1);
		if (!savegameManager.DrawSavePic(savepicLeft, savepicTop, savepicWidth, savepicHeight))
		{
			twod->AddColorOnlyQuad(savepicLeft, savepicTop, savepicWidth, savepicHeight, fillColor);

			if (savegameManager.SavegameCount() > 0)
			{
				if (Selected >= savegameManager.SavegameCount()) Selected = 0;
				FString text = (Selected == -1 || !savegameManager.GetSavegame(Selected)->bOldVersion) ? GStrings("MNU_NOPICTURE") : GStrings("MNU_DIFFVERSION");
				int textlen = NewSmallFont->StringWidth(text) * CleanXfac;

				DrawText(twod, NewSmallFont, CR_GOLD, savepicLeft + (savepicWidth - textlen) / 2,
					savepicTop + (savepicHeight - rowHeight) / 2, text, DTA_CleanNoMove, true, TAG_DONE);
			}
		}

		// Draw comment area
		DrawFrame(twod, frameColor, commentLeft, commentTop, commentWidth, commentHeight, -1);
		twod->AddColorOnlyQuad(commentLeft, commentTop, commentWidth, commentHeight, fillColor);

		int numlinestoprint = std::min(commentRows, (int)BrokenSaveComment.Size());
		for (int i = 0; i < numlinestoprint; i++)
		{
			DrawText(twod, NewConsoleFont, CR_ORANGE, commentLeft / FontScale, (commentTop + rowHeight * i) / FontScale, BrokenSaveComment[i].Text,
				DTA_VirtualWidthF, screen->GetWidth() / FontScale, DTA_VirtualHeightF, screen->GetHeight() / FontScale, DTA_KeepRatio, true, TAG_DONE);
		}


		// Draw file area
		DrawFrame(twod, frameColor, listboxLeft, listboxTop, listboxWidth, listboxHeight, -1);
		twod->AddColorOnlyQuad(listboxLeft, listboxTop, listboxWidth, listboxHeight, fillColor);

		if (savegameManager.SavegameCount() == 0)
		{
			FString text = GStrings("MNU_NOFILES");
			int textlen = int(NewConsoleFont->StringWidth(text) * FontScale);

			DrawText(twod, NewConsoleFont, CR_GOLD, (listboxLeft + (listboxWidth - textlen) / 2) / FontScale, (listboxTop + (listboxHeight - rowHeight) / 2) / FontScale, text,
				DTA_VirtualWidthF, screen->GetWidth() / FontScale, DTA_VirtualHeightF, screen->GetHeight() / FontScale, DTA_KeepRatio, true, TAG_DONE);
			return;
		}

		j = TopItem;
		for (i = 0; i < listboxRows && j < savegameManager.SavegameCount(); i++)
		{
			int colr;
			auto& node = *savegameManager.GetSavegame(j);
			if (node.bOldVersion)
			{
				colr = CR_RED;
			}
			else if (node.bMissingWads)
			{
				colr = CR_YELLOW;
			}
			else if (j == Selected)
			{
				colr = CR_WHITE;
			}
			else
			{
				colr = CR_TAN;
			}

			//screen->SetClipRect(listboxLeft, listboxTop+rowHeight*i, listboxRight, listboxTop+rowHeight*(i+1));

			if ((int)j == Selected)
			{
				twod->AddColorOnlyQuad(listboxLeft, listboxTop + rowHeight * i, listboxWidth, rowHeight, mEntering ? PalEntry(255, 255, 0, 0) : PalEntry(255, 0, 0, 255));
				didSeeSelected = true;
				if (!mEntering)
				{
					DrawText(twod, NewConsoleFont, colr, (listboxLeft + 1) / FontScale, (listboxTop + rowHeight * i + FontScale) / FontScale, node.SaveTitle,
						DTA_VirtualWidthF, screen->GetWidth() / FontScale, DTA_VirtualHeightF, screen->GetHeight() / FontScale, DTA_KeepRatio, true, TAG_DONE);
				}
				else
				{
					FStringf s("%s%c", mInput->GetText(), NewConsoleFont->GetCursor());
					int length = int(NewConsoleFont->StringWidth(s) * FontScale);
					int displacement = std::min(0, listboxWidth - 2 - length);
					DrawText(twod, NewConsoleFont, CR_WHITE, (listboxLeft + 1 + displacement) / FontScale, (listboxTop + rowHeight * i + FontScale) / FontScale, s,
						DTA_VirtualWidthF, screen->GetWidth() / FontScale, DTA_VirtualHeightF, screen->GetHeight() / FontScale, DTA_KeepRatio, true, TAG_DONE);
				}
			}
			else
			{
				DrawText(twod, NewConsoleFont, colr, (listboxLeft + 1) / FontScale, (listboxTop + rowHeight * i + FontScale) / FontScale, node.SaveTitle,
					DTA_VirtualWidthF, screen->GetWidth() / FontScale, DTA_VirtualHeightF, screen->GetHeight() / FontScale, DTA_KeepRatio, true, TAG_DONE);
			}
			//screen->ClearClipRect();
			j++;
		}
	}

	void UpdateSaveComment()
	{
		BrokenSaveComment = V_BreakLines(NewConsoleFont, int(commentWidth / FontScale), savegameManager.SaveCommentString);
	}

	//=============================================================================
	//
	//
	//
	//=============================================================================

	bool MenuEvent(int mkey, bool fromcontroller) override
	{
		auto& manager = savegameManager;
		switch (mkey)
		{
		case MKEY_Up:
			if (manager.SavegameCount() > 1)
			{
				if (Selected == -1) Selected = TopItem;
				else
				{
					if (--Selected < 0) Selected = manager.SavegameCount() - 1;
					if (Selected < TopItem) TopItem = Selected;
					else if (Selected >= TopItem + listboxRows) TopItem = std::max(0, Selected - listboxRows + 1);
				}
				manager.UnloadSaveData();
				manager.ExtractSaveData(Selected);
				UpdateSaveComment();
			}
			return true;

		case MKEY_Down:
			if (manager.SavegameCount() > 1)
			{
				if (Selected == -1) Selected = TopItem;
				else
				{
					if (++Selected >= manager.SavegameCount()) Selected = 0;
					if (Selected < TopItem) TopItem = Selected;
					else if (Selected >= TopItem + listboxRows) TopItem = std::max(0, Selected - listboxRows + 1);
				}
				manager.UnloadSaveData();
				manager.ExtractSaveData(Selected);
				UpdateSaveComment();
			}
			return true;

		case MKEY_PageDown:
			if (manager.SavegameCount() > 1)
			{
				if (TopItem >= manager.SavegameCount() - listboxRows)
				{
					TopItem = 0;
					if (Selected != -1) Selected = 0;
				}
				else
				{
					TopItem = std::min(TopItem + listboxRows, int(manager.SavegameCount()) - listboxRows);
					if (TopItem > Selected&& Selected != -1) Selected = TopItem;
				}
				manager.UnloadSaveData();
				manager.ExtractSaveData(Selected);
				UpdateSaveComment();
			}
			return true;

		case MKEY_PageUp:
			if (manager.SavegameCount() > 1)
			{
				if (TopItem == 0)
				{
					TopItem = std::max(0, int(manager.SavegameCount()) - listboxRows);
					if (Selected != -1) Selected = TopItem;
				}
				else
				{
					TopItem = std::max(int(TopItem - listboxRows), 0);
					if (Selected >= TopItem + listboxRows) Selected = TopItem;
				}
				manager.UnloadSaveData();
				manager.ExtractSaveData(Selected);
				UpdateSaveComment();
			}
			return true;

		case MKEY_Enter:
			return false;	// This event will be handled by the subclasses

		case MKEY_MBYes:
		{
			if (Selected < manager.SavegameCount())
			{
				Selected = manager.RemoveSaveSlot(Selected);
				UpdateSaveComment();
			}
			return true;
		}

		default:
			return Super::MenuEvent(mkey, fromcontroller);
		}
	}

	//=============================================================================
	//
	//
	//
	//=============================================================================

	bool MouseEvent(int type, int x, int y) override
	{
		auto& manager = savegameManager;
		if (x >= listboxLeft && x < listboxLeft + listboxWidth &&
			y >= listboxTop && y < listboxTop + listboxHeight)
		{
			int lineno = (y - listboxTop) / rowHeight;

			if (TopItem + lineno < manager.SavegameCount())
			{
				Selected = TopItem + lineno;
				manager.UnloadSaveData();
				manager.ExtractSaveData(Selected);
				UpdateSaveComment();
				if (type == MOUSE_Release)
				{
					if (MenuEvent(MKEY_Enter, true))
					{
						return true;
					}
				}
			}
			else Selected = -1;
		}
		else Selected = -1;

		return Super::MouseEvent(type, x, y);
	}

	//=============================================================================
	//
	//
	//
	//=============================================================================

	bool Responder(event_t * ev) override
	{
		auto& manager = savegameManager;
		if (ev->type == EV_GUI_Event)
		{
			if (ev->subtype == EV_GUI_KeyDown)
			{
				if ((unsigned)Selected < manager.SavegameCount())
				{
					switch (ev->data1)
					{
					case GK_F1:
						manager.SetFileInfo(Selected);
						UpdateSaveComment();
						return true;

					case GK_DEL:
					case '\b':
					{
						FString EndString;
						EndString.Format("%s" TEXTCOLOR_WHITE "%s" TEXTCOLOR_NORMAL "?\n\n%s",
							GStrings("MNU_DELETESG"), manager.GetSavegame(Selected)->SaveTitle.GetChars(), GStrings("PRESSYN"));
						M_StartMessage(EndString, 0, -1);
					}
					return true;
					}
				}
			}
			else if (ev->subtype == EV_GUI_WheelUp)
			{
				if (TopItem > 0) TopItem--;
				return true;
			}
			else if (ev->subtype == EV_GUI_WheelDown)
			{
				if (TopItem < manager.SavegameCount() - listboxRows) TopItem++;
				return true;
			}
		}
		return Super::Responder(ev);
	}
};

//=============================================================================
//
//
//
//=============================================================================

class DSaveMenu : public DLoadSaveMenu
{
	using Super = DLoadSaveMenu;
	FString mSaveName;
public:


	//=============================================================================
	//
	//
	//
	//=============================================================================

	DSaveMenu()
	{
		savegameManager.InsertNewSaveNode();
		TopItem = 0;
		Selected = savegameManager.ExtractSaveData (-1);
		UpdateSaveComment();
	}

	//=============================================================================
	//
	//
	//
	//=============================================================================

	void Destroy() override
	{
		if (savegameManager.RemoveNewSaveNode())
		{
			Selected--;
		}
		Super::Destroy();
	}
	//=============================================================================
	//
	//
	//
	//=============================================================================

	bool MenuEvent (int mkey, bool fromcontroller) override
	{
		if (Super::MenuEvent(mkey, fromcontroller)) 
		{
			return true;
		}
		if (Selected == -1)
		{
			return false;
		}

		if (mkey == MKEY_Enter)
		{
			FString SavegameString = (Selected != 0)? savegameManager.GetSavegame(Selected)->SaveTitle : FString();
			mInput = new DTextEnterMenu(this, NewConsoleFont, SavegameString, listboxWidth, false, false);
			M_ActivateMenu(mInput);
			mEntering = true;
		}
		else if (mkey == MKEY_Input)
		{
			mEntering = false;
			mSaveName = mInput->GetText();
			mInput = nullptr;
		}
		else if (mkey == MKEY_Abort)
		{
			mEntering = false;
			mInput = nullptr;
		}
		return false;
	}

		//=============================================================================
		//
		//
		//
		//=============================================================================

		bool MouseEvent(int type, int x, int y) override
		{
			if (mSaveName.Len() > 0)
			{
				// Do not process events when saving is in progress to avoid update of the current index,
				// i.e. Selected member variable must remain unchanged
				return true;
			}

			return Super::MouseEvent(type, x, y);
		}

	//=============================================================================
	//
	//
	//
	//=============================================================================

	bool Responder (event_t *ev) override
	{
		if (ev->subtype == EV_GUI_KeyDown)
		{
			if (Selected != -1)
			{
				switch (ev->data1)
				{
				case GK_DEL:
				case '\b':
					// cannot delete 'new save game' item
					if (Selected == 0) return true;
					break;

				case 'N':
					Selected = TopItem = 0;
					savegameManager.UnloadSaveData();
					return true;
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

	void Ticker() override
	{
		if (mSaveName.Len() > 0)
		{
			savegameManager.DoSave(Selected, mSaveName);
		}
	}
	
};

//=============================================================================
//
//
//
//=============================================================================

class DLoadMenu : public DLoadSaveMenu
{
	using Super = DLoadSaveMenu;

public:

	//=============================================================================
	//
	//
	//
	//=============================================================================

	DLoadMenu()
	{
		TopItem = 0;
		Selected = savegameManager.ExtractSaveData(-1);
		UpdateSaveComment();
	}

	//=============================================================================
	//
	//
	//
	//=============================================================================

	bool MenuEvent(int mkey, bool fromcontroller) override
	{
		if (Super::MenuEvent(mkey, fromcontroller))
		{
			return true;
		}
		if (Selected == -1 || savegameManager.SavegameCount() == 0)
		{
			return false;
		}

		if (mkey == MKEY_Enter)
		{
			savegameManager.LoadSavegame(Selected);
			return true;
		}
		return false;
	}
};

static TMenuClassDescriptor<DLoadMenu> _lm("LoadMenu");
static TMenuClassDescriptor<DSaveMenu> _sm("SaveMenu");

void RegisterLoadsaveMenus()
{
	menuClasses.Push(&_sm);
	menuClasses.Push(&_lm);
}
