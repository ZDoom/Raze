//-------------------------------------------------------------------------
/*
Copyright (C) 1996, 2003 - 3D Realms Entertainment
Copyright (C) 2017-2019 Nuke.YKT

This file is part of Duke Nukem 3D version 1.5 - Atomic Edition

Duke Nukem 3D is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

Original Source: 1996 - Todd Replogle
Prepared for public release: 03/21/2003 - Charlie Wiederhold, 3D Realms
*/
//-------------------------------------------------------------------------

// this file collects all 2D content of the game that was scattered across multiple sources originally.
// All this should transition to a more modern, preferably localization friendly, approach later.

#include "ns.h"
#include "duke3d.h"
#include "names_r.h"
#include "animtexture.h"
#include "animlib.h"
#include "raze_music.h"
#include "mapinfo.h"
#include "screenjob.h"
#include "texturemanager.h"
#include "c_dispatch.h"
#include "gamestate.h"

BEGIN_DUKE_NS


//==========================================================================
//
// Sets up the game fonts.
// This is a duplicate of the _d function but needed since the tile numbers differ.
//
//==========================================================================

void InitFonts_r()
{
	GlyphSet fontdata;

	// Small font
	for (int i = 0; i < 95; i++)
	{
		auto tile = tileGetTexture(STARTALPHANUM + i);
		if (tile && tile->isValid() && tile->GetTexelWidth() > 0 && tile->GetTexelHeight() > 0)
		{
			fontdata.Insert('!' + i, tile);
			tile->SetOffsetsNotForFont();
		}
	}
	SmallFont = new ::FFont("SmallFont", nullptr, "defsmallfont", 0, 0, 0, -1, 10, false, false, false, &fontdata);
	SmallFont->SetKerning(2);
	fontdata.Clear();

	// Big font

	// This font is VERY messy...
	fontdata.Insert('_', tileGetTexture(BIGALPHANUM - 11));
	fontdata.Insert('-', tileGetTexture(BIGALPHANUM - 11));
	for (int i = 0; i < 10; i++) fontdata.Insert('0' + i, tileGetTexture(BIGALPHANUM - 10 + i));
	for (int i = 0; i < 26; i++) fontdata.Insert('A' + i, tileGetTexture(BIGALPHANUM + i));
	fontdata.Insert('.', tileGetTexture(BIGPERIOD));
	fontdata.Insert(',', tileGetTexture(BIGCOMMA));
	fontdata.Insert('!', tileGetTexture(BIGX));
	fontdata.Insert('?', tileGetTexture(BIGQ));
	fontdata.Insert(';', tileGetTexture(BIGSEMI));
	fontdata.Insert(':', tileGetTexture(BIGCOLIN));
	fontdata.Insert('\\', tileGetTexture(BIGALPHANUM + 68));
	fontdata.Insert('/', tileGetTexture(BIGALPHANUM + 68));
	fontdata.Insert('%', tileGetTexture(BIGALPHANUM + 69));
	fontdata.Insert('`', tileGetTexture(BIGAPPOS));
	fontdata.Insert('"', tileGetTexture(BIGAPPOS));
	fontdata.Insert('\'', tileGetTexture(BIGAPPOS));
	GlyphSet::Iterator it(fontdata);
	GlyphSet::Pair* pair;
	while (it.NextPair(pair)) pair->Value->SetOffsetsNotForFont();
	BigFont = new ::FFont("BigFont", nullptr, "defbigfont", 0, 0, 0, -1, 10, false, false, false, &fontdata);
	BigFont->SetKerning(6);
	fontdata.Clear();

	// Tiny font
	for (int i = 0; i < 95; i++)
	{
		auto tile = tileGetTexture(MINIFONT + i);
		if (tile && tile->isValid() && tile->GetTexelWidth() > 0 && tile->GetTexelHeight() > 0)
			fontdata.Insert('!' + i, tile);
	}
	fontdata.Insert(1, TexMan.FindGameTexture("TINYBLAK")); // this is only here to widen the color range of the font to produce a better translation.
	SmallFont2 = new ::FFont("SmallFont2", nullptr, "defsmallfont2", 0, 0, 0, -1, 6, false, false, false, &fontdata);
	SmallFont2->SetKerning(2);
	fontdata.Clear();

	// SBAR index font
	for (int i = 0; i < 10; i++) fontdata.Insert('0' + i, tileGetTexture(THREEBYFIVE + i));
	fontdata.Insert(':', tileGetTexture(THREEBYFIVE + 10));
	fontdata.Insert('/', tileGetTexture(THREEBYFIVE + 11));
	fontdata.Insert('%', tileGetTexture(MINIFONT + '%' - '!'));
	fontdata.Insert(1, TexMan.FindGameTexture("TINYBLAK")); // this is only here to widen the color range of the font to produce a better translation.
	IndexFont = new ::FFont("IndexFont", nullptr, nullptr, 0, 0, 0, -1, -1, false, false, false, &fontdata);

	fontdata.Clear();

	// digital font
	for (int i = 0; i < 10; i++) fontdata.Insert('0' + i, tileGetTexture(DIGITALNUM + i));
	fontdata.Insert(1, TexMan.FindGameTexture("TINYBLAK")); // this is only here to widen the color range of the font to produce a better translation.
	DigiFont = new ::FFont("DigiFont", nullptr, nullptr, 0, 0, 0, -1, -1, false, false, false, &fontdata);

}

//==========================================================================
//
// wrappers around DrawText to allow easier reuse of the old code.
// The vertical displacements are to have the same positioning as with the original code.
//
//==========================================================================

static void BigText(double x, double y, const char* text, int align, double alpha = 1.)
{
	//x *= 2.2; y *= 2.64;
	if (align != -1)
		x -= BigFont->StringWidth(text) * (align == 0 ? 0.2 : 0.4);
	DrawText(twod, BigFont, CR_UNTRANSLATED, x, y - 12, text, DTA_FullscreenScale, FSMode_Fit320x200, DTA_ScaleX, 0.4, DTA_ScaleY, 0.4, DTA_Alpha, alpha, TAG_DONE);
}


//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void Logo_r(const CompletionFunc& completion)
{
#if 0
	Mus_Stop();
	FX_StopAllSounds(); // JBF 20031228

	static const AnimSound introsound[] =
	{
		{ 1, 29+1 },
		{ -1, -1 }
	};

	static const AnimSound rednecksound[] =
	{
		{ 1, 478+1 },
		{ -1, -1 }
	};

	static const AnimSound  xatrixsound[] =
	{
		{ 1, 479+1 },
		{ -1, -1 }
	};

	static const int framespeed[] = { 9, 9, 9 }; // same for all 3 anims

	TArray<DScreenJob*> jobs;

	if (userConfig.nologo)
	{
		completion(false);
		return;
	}
	else if (!isRRRA())
	{
		jobs.Push(PlayVideo("rr_intro.anm", introsound, framespeed));
		jobs.Push(PlayVideo("redneck.anm", rednecksound, framespeed));
		jobs.Push(PlayVideo("xatlogo.anm", xatrixsound, framespeed));
	}
	else
	{
		jobs.Push(PlayVideo("redint.mve"));
	}
	RunScreenJob(jobs, completion, SJ_BLOCKUI);
#endif
}

#if 0
//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

static void bonussequence_r(int num, TArray<DScreenJob*>& jobs)
{
	static const AnimSound  turdmov[] =
	{
		{ 1, 82 + 1 },
		{ -1, -1 }
	};

	static const AnimSound  rr_outro[] =
	{
		{ 1, 35 + 1 },
		{ -1, -1 }
	};

	static const int framespeed[] = { 9, 9, 9 }; // same for all 3 anims

	Mus_Stop();
	FX_StopAllSounds();

	switch (num)
	{
	case 0:
		jobs.Push(PlayVideo("turdmov.anm", turdmov, framespeed));
		jobs.Push(Create<DImageScreen>(TENSCREEN));
		break;

	case 1:
		jobs.Push(PlayVideo("rr_outro.anm", rr_outro, framespeed));
		jobs.Push(Create<DImageScreen>(TENSCREEN));
		break;

	default:
		break;
	}
}
#endif


//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void dobonus_r(int bonusonly, const CompletionFunc& completion)
{
#if 0
	TArray<DScreenJob*> jobs;

	FX_StopAllSounds();
	Mus_Stop();

	if (bonusonly < 0 && !isRRRA() && numplayers < 2 && ud.from_bonus == 0)
	{
		int vol = volfromlevelnum(currentLevel->levelNumber);
		bonussequence_r(vol, jobs);
	}

	if (playerswhenstarted > 1 && ud.coop != 1)
	{
		jobs.Push(Create<DRRMultiplayerBonusScreen>(playerswhenstarted));
	}
	else if (bonusonly <= 0 && ud.multimode <= 1)
	{
		if (isRRRA() && !(currentLevel->flags & MI_USERMAP) && currentLevel->levelNumber < 106) // fixme: The logic here is awful. Shift more control to the map records.
		{
			jobs.Push(Create<DRRLevelSummaryScreen>(true));
			int levnum = clamp((currentLevel->levelNumber / 100) * 7 + (currentLevel->levelNumber % 100), 0, 13);
			char fn[20];
			mysnprintf(fn, 20, "lvl%d.anm", levnum + 1);
			static const int framespeed[] = { 20, 20, 7200 };   // wait for one minute on the final frame so that the video doesn't stop  before the user notices.
			jobs.Push(PlayVideo(fn, nullptr, framespeed));
			if (bonusonly < 0 && currentLevel->levelNumber > 100)
			{
				jobs.Push(Create<DRRRAEndOfGame>());
			}
		}
		else jobs.Push(Create<DRRLevelSummaryScreen>(false));
	}
	if (jobs.Size())
		RunScreenJob(jobs, completion);
	else if (completion) completion(false);
#endif
}


void PrintPaused_r()
{
	BigText(160, 100, GStrings("Game Paused"), 0);
}


END_DUKE_NS
