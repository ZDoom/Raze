/*
** usermap browser
** Adapted from the loadsavemenu code.
**
**---------------------------------------------------------------------------
** Copyright 2001-2010 Randy Heit
** Copyright 2010-2021 Christoph Oelckers
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


struct UsermapEntry native
{
	native readonly String displayname;
	native readonly String container;
	native readonly String filename;
	native readonly String info;
	native readonly int size;
}

struct UsermapDirectory native
{
	native readonly String dirname;
	native readonly UsermapDirectory parent;

	native static UsermapDirectory ReadData();
	native int GetNumEntries();
	native int GetNumDirectories();
	native UsermapEntry GetEntry(int num);
	native UsermapDirectory GetDirectory(int num);
	String GetInfo(int Selected)
	{
		if (parent) Selected--;
		Selected -= GetNumDirectories();
		let entry = GetEntry(Selected);
		if (!entry) return "";
		if (entry.info.Length() > 0) return String.Format("Map %s: %s\n%s", entry.filename, StringTable.Localize(entry.info), entry.container);
		return String.Format("Map %s\n%s", entry.filename, entry.container);
	}
	void Select(int Selected)
	{
	}
}


class UsermapMenu : ListMenu
{
	UsermapDirectory currentDir;
	int TopItem;
	int Selected;
	int NumTotalEntries;

	int previewLeft;
	int previewTop;
	int previewWidth;
	int previewHeight;
	int rowHeight;
	int listboxLeft;
	int listboxTop;
	int listboxWidth;

	int listboxRows;
	int listboxHeight;
	int listboxRight;

	int commentLeft;
	int commentTop;
	int commentWidth;
	int commentHeight;
	int commentRows;

	bool mEntering;
	double FontScale;

	int numparent, numdirs, numentries;

	BrokenLines BrokenComment;
	Array<int> selects;

	// private to this menu to prevent exploits.
	private native static void StartMap(UsermapEntry entry);
	private native static void DrawPreview(UsermapEntry entry, int left, int top, int width, int height);

	//=============================================================================
	//
	// 
	//
	//=============================================================================

	override void Init(Menu parent, ListMenuDescriptor desc)
	{
		Super.Init(parent, desc);
		currentDir = UsermapDirectory.ReadData();
		SetSize();
		SetWindows();
	}

	private void SetWindows()
	{
		bool aspect43 = true;
		int Width43 = screen.GetHeight() * 4 / 3;
		int Left43 = (screen.GetWidth() - Width43) / 2;

		double wScale = Width43 / 640.;

		previewLeft = Left43 + int(20 * wScale);
		previewTop = int(mDesc.mYpos * screen.GetHeight() / 200);
		previewWidth = int(260 * wScale);
		previewHeight = int(260 * wScale);

		FontScale = max(screen.GetHeight() / 480, 1);
		rowHeight = int(max((NewConsoleFont.GetHeight() + 1) * FontScale, 1));

		listboxLeft = previewLeft + previewWidth + int(20*wScale);
		listboxTop = previewTop;
		listboxWidth = Width43 + Left43 - listboxLeft - int(30 * wScale);
		int listboxHeight1 = screen.GetHeight() - listboxTop - int(20*wScale);
		listboxRows = (listboxHeight1 - 1) / rowHeight;
		listboxHeight = listboxRows * rowHeight + 1;
		listboxRight = listboxLeft + listboxWidth;

		commentLeft = previewLeft;
		commentTop = previewTop + previewHeight + int(16 * wScale);
		commentWidth = previewWidth;
		commentHeight = listboxHeight - previewHeight - int(16 * wScale);
		commentRows = commentHeight / rowHeight;
	}


	//=============================================================================
	//
	//
	//
	//=============================================================================

	virtual void DrawFrame(int left, int top, int width, int height)
	{
		let framecolor = Color(255, 80, 80, 80);
		Screen.DrawLineFrame(framecolor, left, top, width, height, screen.GetHeight() / 240);
	}

	void SetSize()
	{
		numparent = (currentDir.parent != null);
		numdirs = currentDir.GetNumDirectories();
		numentries = currentDir.GetNumEntries();
		NumTotalEntries = numparent + numdirs + numentries;
	}

	override void Drawer ()
	{
		Super.Drawer();

		int i;
		int j;

		SetWindows();
		DrawFrame(previewLeft, previewTop, previewWidth, previewHeight);
		screen.Dim(0, 0.6, previewLeft, previewTop, previewWidth, previewHeight);

		if (Selected >= numparent + numdirs)
		{
			let entry = currentDir.GetEntry(Selected - numparent - numdirs);
			DrawPreview(entry, previewLeft, previewTop, previewWidth, previewHeight);
		}


		// Draw comment area
		DrawFrame (commentLeft, commentTop, commentWidth, commentHeight);
		screen.Dim(0, 0.6, commentLeft, commentTop, commentWidth, commentHeight);

		int numlinestoprint = min(commentRows, BrokenComment? BrokenComment.Count() : 0);
		for(int i = 0; i < numlinestoprint; i++)
		{
			screen.DrawText(NewConsoleFont, Font.CR_ORANGE, commentLeft / FontScale, (commentTop + rowHeight * i) / FontScale, BrokenComment.StringAt(i),
				DTA_VirtualWidthF, screen.GetWidth() / FontScale, DTA_VirtualHeightF, screen.GetHeight() / FontScale, DTA_KeepRatio, true);
		}


		// Draw file area
		DrawFrame (listboxLeft, listboxTop, listboxWidth, listboxHeight);
		screen.Dim(0, 0.6, listboxLeft, listboxTop, listboxWidth, listboxHeight);

		/*
		if (NumTotalEntries == 0)
		{
			String text = Stringtable.Localize("$MNU_NOFILES");
			int textlen = int(NewConsoleFont.StringWidth(text) * FontScale);

			screen.DrawText (NewConsoleFont, Font.CR_GOLD, (listboxLeft+(listboxWidth-textlen)/2) / FontScale, (listboxTop+(listboxHeight-rowHeight)/2) / FontScale, text, 
				DTA_VirtualWidthF, screen.GetWidth() / FontScale, DTA_VirtualHeightF, screen.GetHeight() / FontScale, DTA_KeepRatio, true);
			return;
		}
		*/

		j = TopItem;
		for (i = 0; i < listboxRows && j < NumTotalEntries; i++)
		{
			int colr;
			String texttoprint;

			if (j < numparent)
			{
				colr = Font.CR_YELLOW;
				texttoprint = "[..]";
			}
			else if (j < numparent + numdirs)
			{
				colr = Font.CR_YELLOW;
				let dir = currentDir.GetDirectory(j - numparent);
				if (!dir) texttoprint = String.Format("Error reading directory %d", j - numparent);
				else texttoprint = String.Format("[%s]", dir.dirName);
			}
			else
			{
				if (j == Selected)
				{
					colr = Font.CR_WHITE;
				}
				else
				{
					colr = Font.CR_TAN;
				}
				let dir = currentDir.GetEntry(j - numparent - numdirs);
				if (!dir) texttoprint = String.Format("Error reading entry %d", j - numparent - numdirs);
				else texttoprint = String.Format("%s", dir.displayName);
			}

			screen.SetClipRect(listboxLeft, listboxTop+rowHeight*i, listboxRight, listboxTop+rowHeight*(i+1));

			if (j == Selected)
			{
				screen.Clear (listboxLeft, listboxTop+rowHeight*i, listboxRight, listboxTop+rowHeight*(i+1), Color(255,0,0,255));
				screen.DrawText (NewConsoleFont, colr, (listboxLeft+1) / FontScale, (listboxTop+rowHeight*i + FontScale) / FontScale, texttoprint, 
					DTA_VirtualWidthF, screen.GetWidth() / FontScale, DTA_VirtualHeightF, screen.GetHeight() / FontScale, DTA_KeepRatio, true);
			}
			else
			{
				screen.DrawText (NewConsoleFont, colr, (listboxLeft+1) / FontScale, (listboxTop+rowHeight*i + FontScale) / FontScale, texttoprint,
					DTA_VirtualWidthF, screen.GetWidth() / FontScale, DTA_VirtualHeightF, screen.GetHeight() / FontScale, DTA_KeepRatio, true);
			}
			screen.ClearClipRect();
			j++;
		}
	} 

	void UpdateComment()
	{
		BrokenComment = NewConsoleFont.BreakLines(currentDir.GetInfo(Selected), int(commentWidth / FontScale));
	}

	void GoToParent()
	{
		if (selects.Size() > 0)
		{
			Selected = selects[selects.Size()-1];
			selects.Pop();
			TopItem = selects[selects.Size()-1];
			selects.Pop();
			currentDir = currentDir.parent;
			currentDir.Select(Selected);
			SetSize();
			UpdateComment();
		}
	}

	//=============================================================================
	//
	//
	//
	//=============================================================================

	override bool MenuEvent (int mkey, bool fromcontroller)
	{
		switch (mkey)
		{
		case MKEY_Up:
			if (NumTotalEntries > 1)
			{
				if (Selected == -1) Selected = TopItem;
				else
				{
					if (--Selected < 0) Selected = NumTotalEntries-1;
					if (Selected < TopItem) TopItem = Selected;
					else if (Selected >= TopItem + listboxRows) TopItem = MAX(0, Selected - listboxRows + 1);
				}
				currentDir.Select(Selected);
				UpdateComment();
			}
			return true;

		case MKEY_Down:
			if (NumTotalEntries > 1)
			{
				if (Selected == -1) Selected = TopItem;
				else
				{
					if (++Selected >= NumTotalEntries) Selected = 0;
					if (Selected < TopItem) TopItem = Selected;
					else if (Selected >= TopItem + listboxRows) TopItem = MAX(0, Selected - listboxRows + 1);
				}
				currentDir.Select(Selected);
				UpdateComment();
			}
			return true;

		case MKEY_PageDown:
			if (NumTotalEntries > 1)
			{
				if (TopItem >= NumTotalEntries - listboxRows)
				{
					TopItem = 0;
					if (Selected != -1) Selected = 0;
				}
				else
				{
					TopItem = MIN(TopItem + listboxRows, NumTotalEntries - listboxRows);
					if (TopItem > Selected && Selected != -1) Selected = TopItem;
				}
				currentDir.Select(Selected);
				UpdateComment();
			}
			return true;

		case MKEY_PageUp:
			if (NumTotalEntries > 1)
			{
				if (TopItem == 0)
				{
					TopItem = MAX(0, NumTotalEntries - listboxRows);
					if (Selected != -1) Selected = TopItem;
				}
				else
				{
					TopItem = MAX(TopItem - listboxRows, 0);
					if (Selected >= TopItem + listboxRows) Selected = TopItem;
				}
				currentDir.Select(Selected);
				UpdateComment();
			}
			return true;

		case MKEY_Enter:
			if (Selected < numparent)
			{
				GoToParent();
			}
			else if (Selected < numparent + numdirs)
			{
				currentDir = currentDir.GetDirectory(Selected - numparent);
				selects.Push(TopItem);
				selects.Push(Selected);
				Selected = 1;
				currentDir.Select(Selected);
				SetSize();
				UpdateComment();
			}
			else
			{
				let entry = currentDir.GetEntry(Selected - numparent - numdirs);
				StartMap(entry);
			}
			return true;

		case MKEY_Back:
			if (selects.Size() > 0)
			{
				GoToParent();
				return true;
			}

		default:
			return Super.MenuEvent(mkey, fromcontroller);
		}
	}

	//=============================================================================
	//
	//
	//
	//=============================================================================

	override bool MouseEvent(int type, int x, int y)
	{
		if (x >= listboxLeft && x < listboxLeft + listboxWidth && 
			y >= listboxTop && y < listboxTop + listboxHeight)
		{
			int lineno = (y - listboxTop) / rowHeight;

			if (TopItem + lineno < NumTotalEntries)
			{
				Selected = TopItem + lineno;
				currentDir.Select(Selected);
				UpdateComment();
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

		return Super.MouseEvent(type, x, y);
	}


	//=============================================================================
	//
	//
	//
	//=============================================================================

	override bool OnUIEvent(UIEvent ev)
	{
		if (ev.Type == UIEvent.Type_KeyDown)
		{
			switch (ev.KeyChar)
			{
			case UIEvent.Key_Backspace:
				GoToParent();
				return true;
			}
		}
		else if (ev.Type == UIEvent.Type_WheelUp)
		{
			if (TopItem > 0) TopItem--;
			return true;
		}
		else if (ev.Type == UIEvent.Type_WheelDown)
		{
			if (TopItem < NumTotalEntries - listboxRows) TopItem++;
			return true;
		}
		return Super.OnUIEvent(ev);
	}


}

