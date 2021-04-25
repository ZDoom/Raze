//-------------------------------------------------------------------------
/*
Copyright (C) 1996, 2003 - 3D Realms Entertainment

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
aint with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

Original Source: 1996 - Todd Replogle
Prepared for public release: 03/21/2003 - Charlie Wiederhold, 3D Realms
Modifications for JonoF's port by Jonathon Fowler (jf@jonof.id.au)
*/
//-------------------------------------------------------------------------

// this file collects all 2D content of the game that was scattered across multiple sources originally.
// All this should transition to a more modern, preferably localization friendly, approach later.

#include "ns.h"
#include "duke3d.h"
#include "names_d.h"
#include "animtexture.h"
#include "animlib.h"
#include "raze_music.h"
#include "mapinfo.h"
#include "screenjob.h"
#include "texturemanager.h"
#include "buildtiles.h"
#include "mapinfo.h"
#include "c_dispatch.h"
#include "gamestate.h"

BEGIN_DUKE_NS

//==========================================================================
//
// Sets up the game fonts.
//
//==========================================================================

void InitFonts_d()
{
	GlyphSet fontdata;

	// Small font
	for (int i = 0; i < 95; i++)
	{
		auto tile = tileGetTexture(STARTALPHANUM + i);
		if (tile && tile->isValid() && tile->GetTexelWidth() > 0 && tile->GetTexelHeight() > 0)
		{
			if (i >= 'a' && i <= 'z' && tileEqualTo(i, i - 32)) continue;
			fontdata.Insert('!' + i, tile);
			tile->SetOffsetsNotForFont();
		}
	}
	SmallFont = new ::FFont("SmallFont", nullptr, "defsmallfont", 0, 0, 0, -1, 5, false, false, false, &fontdata);
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
	// The texture offsets in this font are useless for font printing. This should only apply to these glyphs, not for international extensions, though.
	GlyphSet::Iterator it(fontdata);
	GlyphSet::Pair* pair;
	while (it.NextPair(pair)) pair->Value->SetOffsetsNotForFont();
	BigFont = new ::FFont("BigFont", nullptr, "defbigfont", 0, 0, 0, -1, 5, false, false, false, &fontdata);
	fontdata.Clear();

	// Tiny font
	for (int i = 0; i < 95; i++)
	{
		auto tile = tileGetTexture(MINIFONT + i);
		if (tile && tile->isValid() && tile->GetTexelWidth() > 0 && tile->GetTexelHeight() > 0)
		{
			if (i >= 'a' && i <= 'z' && tileEqualTo(i, i - 32)) continue;
			fontdata.Insert('!' + i, tile);
			tile->SetOffsetsNotForFont();
		}
	}
	fontdata.Insert(1, TexMan.FindGameTexture("TINYBLAK")); // this is only here to widen the color range of the font to produce a better translation.
	SmallFont2 = new ::FFont("SmallFont2", nullptr, "defsmallfont2", 0, 0, 0, -1, 3, false, false, false, &fontdata);
	SmallFont2->SetKerning(1);
	fontdata.Clear();

	// SBAR index font
	for (int i = 0; i < 10; i++) fontdata.Insert('0' + i, tileGetTexture(THREEBYFIVE + i));
	fontdata.Insert(':', tileGetTexture(THREEBYFIVE + 10));
	fontdata.Insert('/', tileGetTexture(THREEBYFIVE + 11));
	fontdata.Insert('%', tileGetTexture(MINIFONT + '%' - '!'));
	fontdata.Insert(1, TexMan.FindGameTexture("TINYBLAK")); // this is only here to widen the color range of the font to produce a better translation.
	GlyphSet::Iterator iti(fontdata);
	while (iti.NextPair(pair)) pair->Value->SetOffsetsNotForFont();
	IndexFont = new ::FFont("IndexFont", nullptr, nullptr, 0, 0, 0, -1, -1, false, false, false, &fontdata);

	fontdata.Clear();

	// digital font
	for (int i = 0; i < 10; i++) fontdata.Insert('0' + i, tileGetTexture(DIGITALNUM + i));
	fontdata.Insert(1, TexMan.FindGameTexture("TINYBLAK")); // this is only here to widen the color range of the font to produce a better translation.
	GlyphSet::Iterator itd(fontdata);
	while (itd.NextPair(pair)) pair->Value->SetOffsetsNotForFont();
	DigiFont = new ::FFont("DigiFont", nullptr, nullptr, 0, 0, 0, -1, -1, false, false, false, &fontdata);

}

//==========================================================================
//
// wrappers around DrawText to allow easier reuse of the old code.
// The vertical displacements are to have the same positioning as with the original code.
//
//==========================================================================

static void BigText(double x, double y, const char* text, int align = -1, double alpha = 1.)
{
	if (align != -1)
		x -= BigFont->StringWidth(text) * (align == 0 ? 0.5 : 1);
	DrawText(twod, BigFont, CR_UNTRANSLATED, x, y - 12, text, DTA_FullscreenScale, FSMode_Fit320x200, DTA_Alpha, alpha, TAG_DONE);
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void Logo_d(const CompletionFunc &completion)
{
#if 0
	Mus_Stop();
	FX_StopAllSounds(); // JBF 20031228

	static const AnimSound logosound[] =
	{
		{ 1, FLY_BY+1 },
		{ 19, PIPEBOMB_EXPLODE+1 },
		{ -1, -1 }
	};
	static const int logoframetimes[] = { 9, 9, 9 };

	TArray<DScreenJob*> jobs;
	int job = 0;
	if (!userConfig.nologo)
	{
		if (!isShareware()) jobs.Push(PlayVideo("logo.anm", logosound, logoframetimes));
		if (!isNam()) jobs.Push(Create<DDRealmsScreen>());
	}
	jobs.Push(Create<DTitleScreen>());
	RunScreenJob(jobs, completion, SJ_BLOCKUI);
#endif
}


#if 0
//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

static void bonussequence_d(int num, TArray<DScreenJob*>& jobs)
{
	static const AnimSound  cineov2sound[] =
	{
		{ 1, WIND_AMBIENCE+1 },
		{ 26, ENDSEQVOL2SND1+1 },
		{ 36, ENDSEQVOL2SND2+1 },
		{ 54, THUD+1 },
		{ 62, ENDSEQVOL2SND3+1 },
		{ 75, ENDSEQVOL2SND4 + 1 },
		{ 81, ENDSEQVOL2SND5 + 1 },
		{ 115, ENDSEQVOL2SND6 + 1 },
		{ 124, ENDSEQVOL2SND7 + 1 },
		{ -1, -1 }
	};

	static const AnimSound cineov3sound[] =
	{
		{ 1, WIND_REPEAT + 1 },
		{ 98, DUKE_GRUNT + 1 },
		{ 102, THUD + 1 },
		{ 102, SQUISHED + 1 },
		{ 124, ENDSEQVOL3SND3 + 1 },
		{ 134, ENDSEQVOL3SND2 + 1 },
		{ 158, PIPEBOMB_EXPLODE + 1 },
		{ -1,-1 }
	};

	static const AnimSound dukedcsound[] =
	{
		{ 144, ENDSEQVOL3SND3 + 1 },
		{ -1,-1 }
	};

	static const AnimSound vol4e1[] =
	{
		{ 3, DUKE_UNDERWATER+1 },
		{ 35, VOL4ENDSND1+1 },
		{ -1,-1 }
	};

	static const AnimSound vol4e2[] =
	{
		{ 11, DUKE_UNDERWATER+1 },
		{ 20, VOL4ENDSND1+1 },
		{ 39, VOL4ENDSND2+1 },
		{ 50, -1 },
		{ -1,-1 }
	};

	static const AnimSound vol4e3[] =
	{
		{ 1, BOSS4_DEADSPEECH+1 },
		{ 40, VOL4ENDSND1+1 },
		{ 40, DUKE_UNDERWATER+1 },
		{ 50, BIGBANG+1 },
		{ -1,-1 }
	};


	static const int framespeed_10[] = { 10, 10, 10 };
	static const int framespeed_14[] = { 14, 14, 14 };
	static const int framespeed_18[] = { 18, 18, 18 };

	switch (num)
	{
	case 0:
		jobs.Push(Create<DEpisode1End1>());
		jobs.Push(Create<DImageScreen>(E1ENDSCREEN, DScreenJob::fadein|DScreenJob::fadeout|DScreenJob::stopmusic, 0x7fffffff));
		break;

	case 1:
		Mus_Stop();
		jobs.Push(PlayVideo("cineov2.anm", cineov2sound, framespeed_18));
		jobs.Push(Create<DE2EndScreen>());
		break;

	case 2:
		Mus_Stop();
		if (g_gameType & GAMEFLAG_DUKEDC)
		{
			jobs.Push(PlayVideo("radlogo.anm", dukedcsound, framespeed_10));
		}
		else
		{
			jobs.Push(PlayVideo("cineov3.anm", cineov3sound, framespeed_10));
			jobs.Push(Create<DBlackScreen>(200, DScreenJob::stopsound));
			jobs.Push(Create<DEpisode3End>());
			if (!isPlutoPak()) jobs.Push(Create<DImageScreen>(TexMan.GetGameTextureByName("DUKETEAM.ANM", false, FTextureManager::TEXMAN_ForceLookup),
				DScreenJob::fadein | DScreenJob::fadeout | DScreenJob::stopsound, 0x7fffffff));
		}
		break;

	case 3:
		Mus_Stop();
		jobs.Push(PlayVideo("vol4e1.anm", vol4e1, framespeed_10));
		jobs.Push(PlayVideo("vol4e2.anm", vol4e2, framespeed_10));
		jobs.Push(PlayVideo("vol4e3.anm", vol4e3, framespeed_10));
		jobs.Push(Create<DEpisode4Text>());
		jobs.Push(Create<DImageScreen>(TexMan.GetGameTextureByName("DUKETEAM.ANM", false, FTextureManager::TEXMAN_ForceLookup), 
			DScreenJob::fadein | DScreenJob::fadeout | DScreenJob::stopsound, 0x7fffffff));
		break;

	case 4:
		Mus_Stop();
		jobs.Push(Create<DEpisode5End>());
		break;
	}
}
#endif

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void showtwoscreens(const CompletionFunc& completion)
{
#if 0
	TArray<DScreenJob*> jobs;

	jobs.Push(Create<DImageScreen>(3291));
	jobs.Push(Create<DImageScreen>(3290));
	RunScreenJob(jobs, completion);
#endif
}

void doorders(const CompletionFunc& completion)
{
#if 0
	TArray<DScreenJob*> jobs;

	for (int i = 0; i < 4; i++)
		jobs.Push(Create<DImageScreen>(ORDERING + i));
	RunScreenJob(jobs, completion);
#endif
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void dobonus_d(int bonusonly, const CompletionFunc& completion)
{
#if 0
	TArray<DScreenJob*> jobs;

	FX_StopAllSounds();

	if (bonusonly < 0 && numplayers < 2 && ud.from_bonus == 0)
	{
		bonussequence_d(volfromlevelnum(currentLevel->levelNumber), jobs);
	}
	else
		Mus_Stop();

	if (playerswhenstarted > 1 && ud.coop != 1)
	{
		jobs.Push(Create<DDukeMultiplayerBonusScreen>(playerswhenstarted));
	}
	else if (bonusonly <= 0 && ud.multimode <= 1)
	{
		jobs.Push(Create<DDukeLevelSummaryScreen>());
	}
	if (jobs.Size())
	{
		RunScreenJob(jobs, completion);
	}
	else if (completion) completion(false);
#endif
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void e4intro(const CompletionFunc& completion)
{
#if 0
	TArray<DScreenJob*> jobs;

	static const AnimSound vol42a[] =
	{
		{ 1, INTRO4_B + 1 },
		{ 12, SHORT_CIRCUIT + 1 },
		{ 18, INTRO4_5 + 1 },
		{ 34, SHORT_CIRCUIT + 1 },
		{ -1,-1 }
	};

	static const AnimSound vol41a[] =
	{
		{ 1, INTRO4_1 + 1 },
		{ 7, INTRO4_3 + 1 },
		{ 12, INTRO4_2 + 1 },
		{ 26, INTRO4_4 + 1 },
		{ -1,-1 }
	};

	static const AnimSound vol43a[] =
	{
		{ 10, INTRO4_6 + 1 },
		{ -1,-1 }
	};

	static const int framespeed_10[] = { 10, 10, 10 };
	static const int framespeed_14[] = { 14, 14, 14 };

	S_PlaySpecialMusic(MUS_BRIEFING);
	jobs.Push(PlayVideo("vol41a.anm", vol41a, framespeed_10));
	jobs.Push(PlayVideo("vol42a.anm", vol42a, framespeed_14));
	jobs.Push(PlayVideo("vol43a.anm", vol43a, framespeed_10));
	RunScreenJob(jobs, completion, SJ_SKIPALL);
#endif
}

#if 0

void loadscreen_d(MapRecord *rec, CompletionFunc func)
{
	TArray<DScreenJob*> jobs(1, true);

	jobs[0] = Create<DDukeLoadScreen>(rec);
	RunScreenJob(jobs, func);
}
#endif

void PrintPaused_d()
{
	BigText(160, 100, GStrings("Game Paused"));
}


END_DUKE_NS
