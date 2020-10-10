/*
** c_commandbuffer.cpp
**
**---------------------------------------------------------------------------
** Copyright 1998-2006 Randy Heit
** Copyright 2010-2020 Christoph Oelckers
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

// for inclusion in c_console.cpp.

struct FCommandBuffer
{
private:
	std::u32string Text;
	unsigned CursorPos = 0;
	unsigned StartPos = 0;	// First character to display
	unsigned CursorPosCells = 0;
	unsigned StartPosCells = 0;

	std::u32string YankBuffer;	// Deleted text buffer

public:
	bool AppendToYankBuffer = false;	// Append consecutive deletes to buffer
	int ConCols;

	FCommandBuffer() = default;

	FCommandBuffer(const FCommandBuffer &o)
	{
		Text = o.Text;
		CursorPos = o.CursorPos;
		StartPos = o.StartPos;
	}

	FString GetText() const
	{
		FString build;
		for (auto chr : Text) build.AppendCharacter(chr);
		return build;
	}

	size_t TextLength() const
	{
		return Text.length();
	}

	void Draw(int x, int y, int scale, bool cursor)
	{
		if (scale == 1)
		{
			DrawChar(twod, CurrentConsoleFont, CR_ORANGE, x, y, '\x1c', TAG_DONE);
			DrawText(twod, CurrentConsoleFont, CR_ORANGE, x + CurrentConsoleFont->GetCharWidth(0x1c), y,
				&Text[StartPos], TAG_DONE);

			if (cursor)
			{
				DrawChar(twod, CurrentConsoleFont, CR_YELLOW,
					x + CurrentConsoleFont->GetCharWidth(0x1c) + (CursorPosCells - StartPosCells) * CurrentConsoleFont->GetCharWidth(0xb),
					y, '\xb', TAG_DONE);
			}
		}
		else
		{
			DrawChar(twod, CurrentConsoleFont, CR_ORANGE, x, y, '\x1c',
				DTA_VirtualWidth, twod->GetWidth() / scale,
				DTA_VirtualHeight, twod->GetHeight() / scale,
				DTA_KeepRatio, true, TAG_DONE);

			DrawText(twod, CurrentConsoleFont, CR_ORANGE, x + CurrentConsoleFont->GetCharWidth(0x1c), y,
				&Text[StartPos],
				DTA_VirtualWidth, twod->GetWidth() / scale,
				DTA_VirtualHeight, twod->GetHeight() / scale,
				DTA_KeepRatio, true, TAG_DONE);

			if (cursor)
			{
				DrawChar(twod, CurrentConsoleFont, CR_YELLOW,
					x + CurrentConsoleFont->GetCharWidth(0x1c) + (CursorPosCells - StartPosCells) * CurrentConsoleFont->GetCharWidth(0xb),
					y, '\xb',
					DTA_VirtualWidth, twod->GetWidth() / scale,
					DTA_VirtualHeight, twod->GetHeight() / scale,
					DTA_KeepRatio, true, TAG_DONE);
			}
		}
	}

	unsigned CalcCellSize(unsigned length)
	{
		unsigned cellcount = 0;
		for (unsigned i = 0; i < length; i++)
		{
			int w;
			NewConsoleFont->GetChar(Text[i], CR_UNTRANSLATED, &w);
			cellcount += w / 9;
		}
		return cellcount;

	}

	unsigned CharsForCells(unsigned cellin, bool *overflow)
	{
		unsigned chars = 0;
		int cells = cellin;
		while (cells > 0)
		{
			int w;
			NewConsoleFont->GetChar(Text[chars++], CR_UNTRANSLATED, &w);
			cells -= w / 9;
		}
		*overflow = (cells < 0);
		return chars;
	}


	void MakeStartPosGood()
	{
		// Make sure both values point to something valid.
		if (CursorPos > Text.length()) CursorPos = (unsigned)Text.length();
		if (StartPos > Text.length()) StartPos = (unsigned)Text.length();

		CursorPosCells = CalcCellSize(CursorPos);
		StartPosCells = CalcCellSize(StartPos);
		unsigned LengthCells = CalcCellSize((unsigned)Text.length());

		int n = StartPosCells;
		unsigned cols = ConCols / active_con_scale(twod);

		if (StartPosCells >= LengthCells)
		{ // Start of visible line is beyond end of line
			n = CursorPosCells - cols + 2;
		}
		if ((CursorPosCells - StartPosCells) >= cols - 2)
		{ // The cursor is beyond the visible part of the line
			n = CursorPosCells - cols + 2;
		}
		if (StartPosCells > CursorPosCells)
		{ // The cursor is in front of the visible part of the line
			n = CursorPosCells;
		}
		StartPosCells = std::max(0, n);
		bool overflow;
		StartPos = CharsForCells(StartPosCells, &overflow);
		if (overflow)
		{
			// We ended up in the middle of a double cell character, so set the start to the following character.
			StartPosCells++;
			StartPos = CharsForCells(StartPosCells, &overflow);
		}
	}

	void CursorStart()
	{
		CursorPos = 0;
		StartPos = 0;
		CursorPosCells = 0;
		StartPosCells = 0;
	}

	void CursorEnd()
	{
		CursorPos = (unsigned)Text.length();
		MakeStartPosGood();
	}

private:
	void MoveCursorLeft()
	{
		CursorPos--;
	}

	void MoveCursorRight()
	{
		CursorPos++;
	}

public:
	void CursorLeft()
	{
		if (CursorPos > 0)
		{
			MoveCursorLeft();
			MakeStartPosGood();
		}
	}

	void CursorRight()
	{
		if (CursorPos < Text.length())
		{
			MoveCursorRight();
			MakeStartPosGood();
		}
	}

	void CursorWordLeft()
	{
		if (CursorPos > 0)
		{
			do MoveCursorLeft();
			while (CursorPos > 0 && Text[CursorPos - 1] != ' ');
			MakeStartPosGood();
		}
	}

	void CursorWordRight()
	{
		if (CursorPos < Text.length())
		{
			do MoveCursorRight();
			while (CursorPos < Text.length() && Text[CursorPos] != ' ');
			MakeStartPosGood();
		}
	}

	void DeleteLeft()
	{
		if (CursorPos > 0)
		{
			MoveCursorLeft();
			Text.erase(CursorPos, 1);
			MakeStartPosGood();
		}
	}

	void DeleteRight()
	{
		if (CursorPos < Text.length())
		{
			Text.erase(CursorPos, 1);
			MakeStartPosGood();
		}
	}

	void DeleteWordLeft()
	{
		if (CursorPos > 0)
		{
			auto now = CursorPos;

			CursorWordLeft();

			if (AppendToYankBuffer) {
				YankBuffer = Text.substr(CursorPos, now - CursorPos) + YankBuffer;
			} else {
				YankBuffer = Text.substr(CursorPos, now - CursorPos);
			}
			Text.erase(CursorPos, now - CursorPos);
			MakeStartPosGood();
		}
	}

	void DeleteLineLeft()
	{
		if (CursorPos > 0)
		{
			if (AppendToYankBuffer) {
				YankBuffer = Text.substr(0, CursorPos) + YankBuffer;
			} else {
				YankBuffer = Text.substr(0, CursorPos);
			}
			Text.erase(0, CursorPos);
			CursorStart();
		}
	}

	void DeleteLineRight()
	{
		if (CursorPos < Text.length())
		{
			if (AppendToYankBuffer) {
				YankBuffer += Text.substr(CursorPos, Text.length() - CursorPos);
			} else {
				YankBuffer = Text.substr(CursorPos, Text.length() - CursorPos);
			}
			Text.resize(CursorPos);
			CursorEnd();
		}
	}

	void AddChar(int character)
	{
		if (Text.length() == 0)
		{
			Text += character;
		}
		else
		{
			Text.insert(CursorPos, 1, character);
		}
		CursorPos++;
		MakeStartPosGood();
	}

	void AddString(FString clip)
	{
		if (clip.IsNotEmpty())
		{
			// Only paste the first line.
			long brk = clip.IndexOfAny("\r\n\b");
			std::u32string build;
			if (brk >= 0)
			{
				clip.Truncate(brk);
			}
			auto strp = (const uint8_t*)clip.GetChars();
			while (auto chr = GetCharFromString(strp)) build += chr;
			
			if (Text.length() == 0)
			{
				Text = build;
			}
			else
			{
				Text.insert(CursorPos, build);
			}
			CursorPos += (unsigned)build.length();
			MakeStartPosGood();
		}
	}

	void SetString(const FString &str)
	{
		Text.clear();
		auto strp = (const uint8_t*)str.GetChars();
		while (auto chr = GetCharFromString(strp)) Text += chr;

		CursorEnd();
		MakeStartPosGood();
	}

	void AddYankBuffer()
	{
		if (YankBuffer.length() > 0)
		{
			if (Text.length() == 0)
			{
				Text = YankBuffer;
			}
			else
			{
				Text.insert(CursorPos, YankBuffer);
			}
			CursorPos += (unsigned)YankBuffer.length();
			MakeStartPosGood();
		}
	}
};
