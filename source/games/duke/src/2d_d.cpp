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
			fontdata.Insert('!' + i, tile);
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

static void BigText(double x, double y, const char* text, double alpha = 1.)
{
	auto width = BigFont->StringWidth(text);
	DrawText(twod, BigFont, CR_UNTRANSLATED, x - width / 2, y - 12, text, DTA_FullscreenScale, 3, DTA_VirtualWidth, 320, DTA_VirtualHeight, 200, DTA_Alpha, alpha, TAG_DONE);
}

static void GameText(double x, double y, const char* t, int shade, int align = -1, int trans = 0)
{
	if (align != -1)
		x -= SmallFont->StringWidth(t) * (align == 0 ? 0.5 : 1);
	int light = Scale(numshades - shade, 255, numshades);
	PalEntry pe(255, light, light, light);
	DrawText(twod, SmallFont, CR_UNDEFINED, x, y + 2, t, DTA_FullscreenScale, 3, DTA_VirtualWidth, 320, DTA_VirtualHeight, 200, DTA_TranslationIndex, TRANSLATION(Translation_Remap, trans), DTA_Color, pe, TAG_DONE);
}

static void MiniText(double x, double y, const char* t, int shade, int align = -1, int trans = 0)
{
	if (align != -1)
		x -= SmallFont2->StringWidth(t) * (align == 0 ? 0.5 : 1);
	int light = Scale(numshades - shade, 255, numshades);
	PalEntry pe(255, light, light, light);
	DrawText(twod, SmallFont2, CR_UNDEFINED, x, y, t, DTA_FullscreenScale, 3, DTA_VirtualWidth, 320, DTA_VirtualHeight, 200, DTA_TranslationIndex, TRANSLATION(Translation_Remap, trans), DTA_Color, pe, TAG_DONE);
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

class DDRealmsScreen : public DScreenJob
{
public:
	DDRealmsScreen() : DScreenJob(fadein | fadeout)	{}

	int Frame(uint64_t clock, bool skiprequest) override
	{
		const uint64_t duration = 7'000'000'000;
		const auto tex = tileGetTexture(DREALMS, true);
		const int translation = TRANSLATION(Translation_BasePalettes, DREALMSPAL);

		twod->ClearScreen();
		DrawTexture(twod, tex, 0, 0, DTA_FullscreenEx, 3, DTA_TranslationIndex, translation, DTA_LegacyRenderStyle, STYLE_Normal, TAG_DONE);
		return skiprequest ? -1 : clock < duration ? 1 : 0;
	}
};

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

class DTitleScreen : public DScreenJob
{
	int soundanm = 0;

public:
	DTitleScreen() : DScreenJob(fadein | fadeout) {}

	int Frame(uint64_t nsclock, bool skiprequest) override
	{
		twod->ClearScreen();
		int clock = nsclock * 120 / 1'000'000'000;

		auto translation = TRANSLATION(Translation_BasePalettes, TITLEPAL);

		twod->ClearScreen();
		DrawTexture(twod, tileGetTexture(BETASCREEN, true), 0, 0, DTA_FullscreenEx, 3, DTA_TranslationIndex, translation, DTA_LegacyRenderStyle, STYLE_Normal, TAG_DONE);

		if (soundanm == 0 && clock >= 120 && clock < 120 + 60)
		{
			soundanm = 1;
			S_PlaySound(PIPEBOMB_EXPLODE, CHAN_AUTO, CHANF_UI);
		}
		if (soundanm == 1 && clock > 220 && clock < (220 + 30))
		{
			soundanm = 2;
			S_PlaySound(PIPEBOMB_EXPLODE, CHAN_AUTO, CHANF_UI);
		}
		if (soundanm == 2 && clock >= 280 && clock < 395)
		{
			soundanm = 3;
			S_PlaySound(FLY_BY, CHAN_AUTO, CHANF_UI);
		}
		else if (soundanm == 3 && clock >= 395)
		{
			soundanm = 4;
			S_PlaySound(PIPEBOMB_EXPLODE, CHAN_AUTO, CHANF_UI);
		}

		double scale = clamp(clock - 120, 0, 60) / 64.;
		if (scale > 0.)
			DrawTexture(twod, tileGetTexture(DUKENUKEM, true), 160, 104, DTA_FullscreenScale, 3, DTA_VirtualWidth, 320, DTA_VirtualHeight, 200,
				DTA_CenterOffsetRel, true,  DTA_TranslationIndex, translation, DTA_ScaleX, scale, DTA_ScaleY, scale, TAG_DONE);

		scale = clamp(clock - 220, 0, 30) / 32.;
		if (scale > 0.)
			DrawTexture(twod, tileGetTexture(THREEDEE, true), 160, 129, DTA_FullscreenScale, 3, DTA_VirtualWidth, 320, DTA_VirtualHeight, 200,
				DTA_CenterOffsetRel, true, DTA_TranslationIndex, translation, DTA_ScaleX, scale, DTA_ScaleY, scale, TAG_DONE);

		if (PLUTOPAK) 
		{
			scale = (410 - clamp(clock, 280, 395)) / 16.;
			if (scale > 0. && clock > 280)
				DrawTexture(twod, tileGetTexture(PLUTOPAKSPRITE+1, true), 160, 151, DTA_FullscreenScale, 3, DTA_VirtualWidth, 320, DTA_VirtualHeight, 200,
					DTA_CenterOffsetRel, true, DTA_TranslationIndex, translation, DTA_ScaleX, scale, DTA_ScaleY, scale, TAG_DONE);
		}

		if (clock > (860 + 120))
		{
			return 0;
		}

		return skiprequest ? -1 : 1;
	}
};

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void Logo_d(const CompletionFunc &completion)
{
	Mus_Stop();
	FX_StopAllSounds(); // JBF 20031228

	static const AnimSound logosound[] =
	{
		{ 1, FLY_BY+1 },
		{ 19, PIPEBOMB_EXPLODE+1 },
		{ -1, -1 }
	};
	static const int logoframetimes[] = { 9, 9, 9 };

	JobDesc jobs[3];
	int job = 0;
	if (VOLUMEALL) jobs[job++] = { PlayVideo("logo.anm", logosound, logoframetimes), []() { S_PlaySpecialMusic(MUS_INTRO); } };
	if (!isNam()) jobs[job++] = { Create<DDRealmsScreen>(), nullptr };
	jobs[job++] = { Create<DTitleScreen>(), []() { S_PlaySound(NITEVISION_ONOFF, CHAN_AUTO, CHANF_UI); } };
	RunScreenJob(jobs, job, completion, true, true);
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

class DEpisode1End1 : public DScreenJob
{
	int bonuscnt = 0;

public:
	DEpisode1End1() : DScreenJob(fadein | fadeout) {}

	int Frame(uint64_t nsclock, bool skiprequest) override
	{
		static const int breathe[] =
		{
			 0,  30,VICTORY1 + 1,176,59,
			30,  60,VICTORY1 + 2,176,59,
			60,  90,VICTORY1 + 1,176,59,
			90, 120,0         ,176,59
		};

		static const int bossmove[] =
		{
			 0, 120,VICTORY1 + 3,86,59,
		   220, 260,VICTORY1 + 4,86,59,
		   260, 290,VICTORY1 + 5,86,59,
		   290, 320,VICTORY1 + 6,86,59,
		   320, 350,VICTORY1 + 7,86,59,
		   350, 380,VICTORY1 + 8,86,59
		};

		auto translation = TRANSLATION(Translation_BasePalettes, ENDINGPAL);

		int totalclock = nsclock * 120 / 1'000'000'000;

		uint64_t span = nsclock / 1'000'000;

		twod->ClearScreen();
		DrawTexture(twod, tileGetTexture(VICTORY1, true), 0, 50, DTA_FullscreenScale, 3, DTA_VirtualWidth, 320, DTA_VirtualHeight, 200, 
			DTA_TranslationIndex, translation, DTA_LegacyRenderStyle, STYLE_Normal, DTA_TopLeft, true, TAG_DONE);


		// boss
		if (totalclock > 390 && totalclock < 780)
			for (int t = 0; t < 35; t += 5) if (bossmove[t + 2] && (totalclock % 390) > bossmove[t] && (totalclock % 390) <= bossmove[t + 1])
			{
				if (t == 10 && bonuscnt == 1) 
				{ 
					S_PlaySound(SHOTGUN_FIRE, CHAN_AUTO, CHANF_UI);
					S_PlaySound(SQUISHED, CHAN_AUTO, CHANF_UI);
					bonuscnt++; 
				}
				DrawTexture(twod, tileGetTexture(bossmove[t + 2], true), bossmove[t + 3], bossmove[t + 4], DTA_FullscreenScale, 3, DTA_VirtualWidth, 320, DTA_VirtualHeight, 200,
					DTA_TranslationIndex, translation, DTA_TopLeft, true, TAG_DONE);
			}

		// Breathe
		if (totalclock < 450 || totalclock >= 750)
		{
			if (totalclock >= 750)
			{
				DrawTexture(twod, tileGetTexture(VICTORY1 + 8, true), 86, 59, DTA_FullscreenScale, 3, DTA_VirtualWidth, 320, DTA_VirtualHeight, 200,
					DTA_TranslationIndex, translation, DTA_TopLeft, true, TAG_DONE);
				if (totalclock >= 750 && bonuscnt == 2) 
				{ 
					S_PlaySound(DUKETALKTOBOSS, CHAN_AUTO, CHANF_UI);
					bonuscnt++; 
				}
			}
			for (int t = 0; t < 20; t += 5)
				if (breathe[t + 2] && (totalclock % 120) > breathe[t] && (totalclock % 120) <= breathe[t + 1])
				{
					if (t == 5 && bonuscnt == 0)
					{
						S_PlaySound(BOSSTALKTODUKE, CHAN_AUTO, CHANF_UI);
						bonuscnt++;
					}
					DrawTexture(twod, tileGetTexture(breathe[t + 2], true), breathe[t + 3], breathe[t + 4], DTA_FullscreenScale, 3, DTA_VirtualWidth, 320, DTA_VirtualHeight, 200,
						DTA_TranslationIndex, translation, DTA_TopLeft, true, TAG_DONE);
				}
		}
		// Only end after having faded out.
		return skiprequest ? -1 : 1;
	}
};

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

class DEpisode3End : public DImageScreen
{
	int sound = 0;
	int64_t waittime = 0;

public:

	FGameTexture* getTexture()
	{
		// Here we must provide a real texture, even if invalid, so that the sounds play.
		auto texid = TexMan.CheckForTexture("radlogo.anm", ETextureType::Any, FTextureManager::TEXMAN_TryAny | FTextureManager::TEXMAN_ForceLookup);
		if (texid.isValid()) return TexMan.GetGameTexture(texid);
		else return TexMan.GameByIndex(0);
	}

public:
	DEpisode3End() : DImageScreen(getTexture(), fadein|fadeout)
	{
	}

	int Frame(uint64_t clock, bool skiprequest)
	{
		switch (sound)
		{
		case 0:
			S_PlaySound(ENDSEQVOL3SND5, CHAN_AUTO, CHANF_UI);
			sound++;
			break;

		case 1:
			if (!S_CheckSoundPlaying(ENDSEQVOL3SND5))
			{
				S_PlaySound(ENDSEQVOL3SND6, CHAN_AUTO, CHANF_UI);
				sound++;
			}
			break;

		case 2:
			if (!S_CheckSoundPlaying(ENDSEQVOL3SND6))
			{
				S_PlaySound(ENDSEQVOL3SND7, CHAN_AUTO, CHANF_UI);
				sound++;
			}
			break;

		case 3:
			if (!S_CheckSoundPlaying(ENDSEQVOL3SND7))
			{
				S_PlaySound(ENDSEQVOL3SND8, CHAN_AUTO, CHANF_UI);
				sound++;
			}
			break;

		case 4:
			if (!S_CheckSoundPlaying(ENDSEQVOL3SND8))
			{
				S_PlaySound(ENDSEQVOL3SND9, CHAN_AUTO, CHANF_UI);
				sound++;
			}
			break;

		case 5:
			if (!S_CheckSoundPlaying(ENDSEQVOL3SND8))
			{
				sound++;
				waittime = clock + (SoundEnabled()? 1'000'000'000 : 5'000'000'000);	// if sound is off this wouldn't wait without a longer delay here.
			}
			break;

		case 6:
			if (PLUTOPAK)
			{
				if (clock > waittime) skiprequest = true;
			}
			break;

		default:
			break;
		}
		int ret = DImageScreen::Frame(clock, skiprequest);
		if (ret != 1) FX_StopAllSounds();
		return ret;
	}
};

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

class DEpisode4Text : public DScreenJob
{
public:
	DEpisode4Text() : DScreenJob(fadein | fadeout) {}

	int Frame(uint64_t clock, bool skiprequest)
	{
		twod->ClearScreen();
		BigText(160, 60, GStrings("Thanks to all our"));
		BigText(160, 60 + 16, GStrings("fans for giving"));
		BigText(160, 60 + 16 + 16, GStrings("us big heads."));
		BigText(160, 70 + 16 + 16 + 16, GStrings("Look for a Duke Nukem 3D"));
		BigText(160, 70 + 16 + 16 + 16 + 16, GStrings("sequel soon."));
		return skiprequest ? -1 : 1;
	}
};

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

class DEpisode5End : public DImageScreen
{
	int sound = 0;

public:
	DEpisode5End() : DImageScreen(FIREFLYGROWEFFECT, fadein|fadeout)
	{
	}

	int Frame(uint64_t clock, bool skiprequest)
	{
		switch (sound)
		{
		case 0:
			sound++;
			break;

		case 1:
			S_PlaySound(E5L7_DUKE_QUIT_YOU, CHAN_AUTO, CHANF_UI);
			sound++;
			break;

		default:
			break;
		}
		int ret = DImageScreen::Frame(clock, skiprequest);
		if (ret != 1) FX_StopAllSounds();
		return ret;
	}
};

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

static void bonussequence_d(int num, JobDesc *jobs, int &job)
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
		jobs[job++] = { Create<DEpisode1End1>(), nullptr };
		jobs[job++] = { Create<DImageScreen>(E1ENDSCREEN), nullptr };
		break;

	case 1:
		jobs[job++] = { PlayVideo("cineov2.anm", cineov2sound, framespeed_18), []() { S_PlaySound(PIPEBOMB_EXPLODE, CHAN_AUTO, CHANF_UI); } };
		jobs[job++] = { Create<DImageScreen>(E2ENDSCREEN), []() { FX_StopAllSounds(); } };
		break;

	case 2:
		jobs[job++] = { PlayVideo("cineov3.anm", cineov3sound, framespeed_10), nullptr };
		jobs[job++] = { Create<DBlackScreen>(200), []() { FX_StopAllSounds(); } };
		jobs[job++] = { Create<DEpisode3End>(), []() { if (!PLUTOPAK) S_PlaySound(ENDSEQVOL3SND4, CHAN_AUTO, CHANF_UI); } };
		if (!PLUTOPAK) jobs[job++] = { Create<DImageScreen>(TexMan.GetGameTextureByName("DUKETEAM.ANM", false, FTextureManager::TEXMAN_ForceLookup)), []() { FX_StopAllSounds(); } };
		break;

	case 3:
		jobs[job++] = { PlayVideo("vol4e1.anm", vol4e1, framespeed_10), nullptr };
		jobs[job++] = { PlayVideo("vol4e2.anm", vol4e2, framespeed_10), nullptr };
		jobs[job++] = { PlayVideo("vol4e3.anm", vol4e3, framespeed_10), []() { S_PlaySound(ENDSEQVOL3SND4, CHAN_AUTO, CHANF_UI); } };
		jobs[job++] = { Create<DEpisode4Text>(), nullptr };
		jobs[job++] = { Create<DImageScreen>(TexMan.GetGameTextureByName("DUKETEAM.ANM", false, FTextureManager::TEXMAN_ForceLookup)),  []() { FX_StopAllSounds(); } };
		break;

	case 4:
		jobs[job++] = { Create<DEpisode5End>(),  []() { FX_StopAllSounds(); } };
		break;
	}
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void showtwoscreens(const CompletionFunc& completion)
{
	JobDesc jobs[2];
	int job = 0;

	jobs[job++] = { Create<DImageScreen>(3291), nullptr };
	jobs[job++] = { Create<DImageScreen>(3290), nullptr };
	RunScreenJob(jobs, job, completion);
}

void doorders(const CompletionFunc& completion)
{
	JobDesc jobs[4];
	int job = 0;

	for (int i = 0; i < 4; i++)
		jobs[job++] = { Create<DImageScreen>(ORDERING + i), nullptr };
	RunScreenJob(jobs, job, completion);
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

class DDukeMultiplayerBonusScreen : public DScreenJob
{
	int playerswhenstarted;

public:
	DDukeMultiplayerBonusScreen(int pws) : DScreenJob(fadein|fadeout)
	{
		playerswhenstarted = pws;
		S_PlayBonusMusic();
	}

	int Frame(uint64_t clock, bool skiprequest)
	{
		char tempbuf[32];
		int totalclock = int(clock * 120 / 1'000'000'000);
		twod->ClearScreen();
		DrawTexture(twod, tileGetTexture(MENUSCREEN), 0, 0, DTA_FullscreenEx, 3, DTA_Color, 0xff808080, DTA_LegacyRenderStyle, STYLE_Normal, TAG_DONE);
		DrawTexture(twod, tileGetTexture(INGAMEDUKETHREEDEE, true), 160, 34, DTA_FullscreenScale, 3, DTA_VirtualWidth, 320, DTA_VirtualHeight, 200, DTA_CenterOffsetRel, true, TAG_DONE);
		if (PLUTOPAK)
			DrawTexture(twod, tileGetTexture(PLUTOPAKSPRITE+2, true), 260, 36, DTA_FullscreenScale, 3, DTA_VirtualWidth, 320, DTA_VirtualHeight, 200, DTA_CenterOffsetRel, true, TAG_DONE);

		GameText(160, 58 + 2, GStrings("Multiplayer Totals"), 0, 0);
		GameText(160, 58 + 10, currentLevel->DisplayName(), 0, 0);
		GameText(160, 165, GStrings("Presskey"), 8 - int(sin(totalclock / 10.) * 8), 0);

		int t = 0;

		MiniText(38, 80, GStrings("Name"), 0, -1, 8);
		MiniText(269+20, 80, GStrings("Kills"), 0, 1, 8);

		for (int i = 0; i < playerswhenstarted; i++)
		{
			mysnprintf(tempbuf, 32, "%-4d", i + 1);
			MiniText(92 + (i * 23), 80, tempbuf, 0, -1, 3);
		}

		for (int i = 0; i < playerswhenstarted; i++)
		{
			int xfragtotal = 0;
			mysnprintf(tempbuf, 32, "%d", i + 1);

			MiniText(30, 90 + t, tempbuf, 0);
			MiniText(38, 90 + t, ud.user_name[i], 0, -1, ps[i].palookup);

			for (int y = 0; y < playerswhenstarted; y++)
			{
				int frag = frags[i][y];
				if (i == y)
				{
					mysnprintf(tempbuf, 32, "%-4d", ps[y].fraggedself);
					MiniText(92 + (y * 23), 90 + t, tempbuf, 0, -1, 2);
					xfragtotal -= ps[y].fraggedself;
				}
				else
				{
					mysnprintf(tempbuf, 32, "%-4d", frag);
					MiniText(92 + (y * 23), 90 + t, tempbuf, 0);
					xfragtotal += frag;
				}
				/*
				if (myconnectindex == connecthead)
				{
					mysnprintf(tempbuf, 32, "stats %ld killed %ld %ld\n", i + 1, y + 1, frag);
					sendscore(tempbuf);
				}
				*/
			}

			mysnprintf(tempbuf, 32, "%-4d", xfragtotal);
			MiniText(101 + (8 * 23), 90 + t, tempbuf, 0, -1, 2);

			t += 7;
		}

		for (int y = 0; y < playerswhenstarted; y++)
		{
			int yfragtotal = 0;
			for (int i = 0; i < playerswhenstarted; i++)
			{
				if (i == y)
					yfragtotal += ps[i].fraggedself;
				int frag = frags[i][y];
				yfragtotal += frag;
			}
			mysnprintf(tempbuf, 32, "%-4d", yfragtotal);
			MiniText(92 + (y * 23), 96 + (8 * 7), tempbuf, 0, -1, 2);
		}

		MiniText(45, 96 + (8 * 7), GStrings("Deaths"), 0, -1, 8);
		return skiprequest ? -1 : 1;
	}
};

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

class DDukeLevelSummaryScreen : public DScreenJob
{
	const char* lastmapname;
	int gfx_offset;
	int bonuscnt = 0;

	void SetTotalClock(int tc)
	{
		SetClock(tc * (uint64_t)1'000'000'000 / 120);
	}

public:
	DDukeLevelSummaryScreen() : DScreenJob(fadein | fadeout)
	{
		int vol = volfromlevelnum(currentLevel->levelNumber);
		gfx_offset = BONUSSCREEN + ((vol == 1) ? 5 : 0);
		lastmapname = currentLevel->DisplayName();
		S_PlayBonusMusic();
	}

	void FormatTime(int time, char* tempbuf)
	{
		mysnprintf(tempbuf, 32, "%02d:%02d", (time / (26 * 60)) % 60, (time / 26) % 60);
	}

	void PrintTime(int totalclock)
	{
		char tempbuf[32];
		GameText(10, 59 + 9, GStrings("TXT_YourTime"), 0);
		GameText(10, 69 + 9, GStrings("TXT_ParTime"), 0);
		if (!isNamWW2GI())
			GameText(10, 78 + 9, GStrings("TXT_3DRTIME"), 0);

		if (bonuscnt == 0)
			bonuscnt++;

		if (totalclock > (60 * 4))
		{
			if (bonuscnt == 1)
			{
				bonuscnt++;
				S_PlaySound(PIPEBOMB_EXPLODE, CHAN_AUTO, CHANF_UI);
			}
			FormatTime(ps[myconnectindex].player_par, tempbuf);
			GameText((320 >> 2) + 71, 60 + 9, tempbuf, 0);

			FormatTime(currentLevel->parTime, tempbuf);
			GameText((320 >> 2) + 71, 69 + 9, tempbuf, 0);

			if (!isNamWW2GI())
			{
				FormatTime(currentLevel->designerTime, tempbuf);
				GameText((320 >> 2) + 71, 78 + 9, tempbuf, 0);
			}
		}
	}

	void PrintKills(int totalclock)
	{
		char tempbuf[32];
		GameText(10, 94 + 9, GStrings("TXT_EnemiesKilled"), 0);
		GameText(10, 99 + 4 + 9, GStrings("TXT_EnemiesLeft"), 0);

		if (bonuscnt == 2)
		{
			bonuscnt++;
			S_PlaySound(FLY_BY, CHAN_AUTO, CHANF_UI);
		}

		if (totalclock > (60 * 7))
		{
			if (bonuscnt == 3)
			{
				bonuscnt++;
				S_PlaySound(PIPEBOMB_EXPLODE, CHAN_AUTO, CHANF_UI);
			}
			mysnprintf(tempbuf, 32, "%-3d", ps[myconnectindex].actors_killed);
			GameText((320 >> 2) + 70, 93 + 9, tempbuf, 0);
			if (ud.player_skill > 3)
			{
				mysnprintf(tempbuf, 32, "%s", GStrings("TXT_N_A"));
				GameText((320 >> 2) + 70, 99 + 4 + 9, tempbuf, 0);
			}
			else
			{
				if ((ps[myconnectindex].max_actors_killed - ps[myconnectindex].actors_killed) < 0)
					mysnprintf(tempbuf, 32, "%-3d", 0);
				else mysnprintf(tempbuf, 32, "%-3d", ps[myconnectindex].max_actors_killed - ps[myconnectindex].actors_killed);
				GameText((320 >> 2) + 70, 99 + 4 + 9, tempbuf, 0);
			}
		}
	}

	void PrintSecrets(int totalclock)
	{
		char tempbuf[32];
		GameText(10, 120 + 9, GStrings("TXT_SECFND"), 0);
		GameText(10, 130 + 9, GStrings("TXT_SECMISS"), 0);
		if (bonuscnt == 4) bonuscnt++;

		if (totalclock > (60 * 10))
		{
			if (bonuscnt == 5)
			{
				bonuscnt++;
				S_PlaySound(PIPEBOMB_EXPLODE, CHAN_AUTO, CHANF_UI);
			}
			mysnprintf(tempbuf, 32, "%-3d", ps[myconnectindex].secret_rooms);
			GameText((320 >> 2) + 70, 120 + 9, tempbuf, 0);
			if (ps[myconnectindex].secret_rooms > 0)
				sprintf(tempbuf, "%-3d", (100 * ps[myconnectindex].secret_rooms / ps[myconnectindex].max_secret_rooms));
			mysnprintf(tempbuf, 32, "%-3d", ps[myconnectindex].max_secret_rooms - ps[myconnectindex].secret_rooms);
			GameText((320 >> 2) + 70, 130 + 9, tempbuf, 0);
		}
	}

	int Frame(uint64_t clock, bool skiprequest)
	{
		twod->ClearScreen();
		int totalclock = int(clock * 120 / 1'000'000'000);
		DrawTexture(twod, tileGetTexture(gfx_offset, true), 0, 0, DTA_FullscreenEx, 3, DTA_LegacyRenderStyle, STYLE_Normal, TAG_DONE);

		if (lastmapname) BigText(160, 20 - 6, lastmapname);
		BigText(160, 36 - 6, GStrings("Completed"));
		GameText(160, 190, GStrings("PRESSKEY"), 8 - int(sin(totalclock / 10.) * 8), 0);

		if (totalclock > (60 * 3))
		{
			PrintTime(totalclock);
		}
		if (totalclock > (60 * 6))
		{
			PrintKills(totalclock);
		}
		if (totalclock > (60 * 9))
		{
			PrintSecrets(totalclock);
		}

		if (totalclock >= (1000000000L) && totalclock < (1000000320L))
		{
			switch ((totalclock >> 4) % 15)
			{
				case 0:
					if (bonuscnt == 6)
					{
						bonuscnt++;
						S_PlaySound(SHOTGUN_COCK, CHAN_AUTO, CHANF_UI);
						static const uint16_t speeches[] = { BONUS_SPEECH1, BONUS_SPEECH2, BONUS_SPEECH3, BONUS_SPEECH4};
						S_PlaySound(speeches[(rand() & 3)], CHAN_AUTO, CHANF_UI);
					}
				case 1:
				case 4:
				case 5:
					DrawTexture(twod, tileGetTexture(gfx_offset + 3), 199, 31, DTA_FullscreenScale, 3, DTA_VirtualWidth, 320, DTA_VirtualHeight, 200, DTA_TopLeft, true, TAG_DONE);
					break;
				case 2:
				case 3:
					DrawTexture(twod, tileGetTexture(gfx_offset + 4), 199, 31, DTA_FullscreenScale, 3, DTA_VirtualWidth, 320, DTA_VirtualHeight, 200, DTA_TopLeft, true, TAG_DONE);
					break;
			}
		}
		else if (totalclock > (10240 + 120L)) return 0;
		else
		{
			switch((totalclock >> 5) & 3)
			{
				case 1:
				case 3:
					DrawTexture(twod, tileGetTexture(gfx_offset + 1), 199, 31, DTA_FullscreenScale, 3, DTA_VirtualWidth, 320, DTA_VirtualHeight, 200, DTA_TopLeft, true, TAG_DONE);
					break;
				case 2:
					DrawTexture(twod, tileGetTexture(gfx_offset + 2), 199, 31, DTA_FullscreenScale, 3, DTA_VirtualWidth, 320, DTA_VirtualHeight, 200, DTA_TopLeft, true, TAG_DONE);
					break;
			}
		}

		if (totalclock > 10240 && totalclock < 10240 + 10240)
			SetTotalClock(1024);

		if (skiprequest && totalclock > (60 * 2))
		{
			skiprequest = false;
			if (totalclock < (60 * 13))
			{
				SetTotalClock(60 * 13);
			}
			else if (totalclock < (1000000000))
			{
				// force-set bonuscnt here so that it won't desync with the rest of the logic and Duke's voice can be heard.
				if (bonuscnt < 6) bonuscnt = 6;
				SetTotalClock(1000000000);
			}
		}

		return 1;
	}

};

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void dobonus_d(bool bonusonly, const CompletionFunc& completion)
{
	JobDesc jobs[20];
	int job = 0;

	FX_StopAllSounds();
	Mus_Stop();

	if (!bonusonly && numplayers < 2 && ud.eog && ud.from_bonus == 0)
	{
		bonussequence_d(volfromlevelnum(currentLevel->levelNumber), jobs, job);
	}

	if (playerswhenstarted > 1 && ud.coop != 1)
	{
		jobs[job++] = { Create<DDukeMultiplayerBonusScreen>(playerswhenstarted) };
	}
	else if (!bonusonly && ud.multimode <= 1)
	{
		jobs[job++] = { Create<DDukeLevelSummaryScreen>() };
	}
	if (job)
		RunScreenJob(jobs, job, completion);
	else if (completion) completion(false);
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void e4intro(const CompletionFunc& completion)
{
	JobDesc jobs[5];
	int job = 0;

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
	jobs[job++] = { PlayVideo("vol41a.anm", vol41a, framespeed_10), nullptr };
	jobs[job++] = { PlayVideo("vol42a.anm", vol42a, framespeed_14), nullptr, true };
	jobs[job++] = { PlayVideo("vol43a.anm", vol43a, framespeed_10), nullptr, true };
	RunScreenJob(jobs, job, completion);
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

class DDukeLoadScreen : public DScreenJob
{
	std::function<int(void)> callback;
	MapRecord* rec;
	
public:
	DDukeLoadScreen(MapRecord *maprec, std::function<int(void)> callback_) : DScreenJob(fadein|fadeout),  callback(callback_), rec(maprec) {}

	int Frame(uint64_t clock, bool skiprequest)
	{
		DrawTexture(twod, tileGetTexture(LOADSCREEN), 0, 0, DTA_FullscreenEx, 3, DTA_LegacyRenderStyle, STYLE_Normal, TAG_DONE);
		
		BigText(160, 90, (rec->flags & MI_USERMAP)? GStrings("TXT_LOADUM") :  GStrings("TXT_LOADING"));
		BigText(160, 114, rec->DisplayName());

		// Initiate the level load once the page has been faded in completely.
		if (callback && GetFadeState() == visible)
		{
			callback();
			callback = nullptr;
		}
		if (clock > 5'000'000'000) return 0;	// make sure the screen stays long enough to be seen.
		return skiprequest? -1 : 1;
	}
};

void PrintPaused_d()
{
	BigText(160, 100, GStrings("Game Paused"));
}

void PrintLevelName_d(double alpha)
{
	BigText(160, 114, currentLevel->DisplayName(), alpha);
}

// Utility for testing the above screens
CCMD(testscreen)
{
	JobDesc jobs[10];
	int job = 0;
	C_HideConsole();
	FX_StopAllSounds();
	Mus_Stop();

	auto gs = gamestate;
	auto completion = [=](bool)
	{
		if (gs == GS_LEVEL || gs == GS_DEMOSCREEN) gamestate = gs;
		else gamestate = GS_STARTUP;
	};


	if (argv.argc() > 1)
	{
		int screen = strtol(argv[1], nullptr, 0);
		switch (screen)
		{
		case 0:
		case 1:
		case 2:
		case 3:
		case 4:
			bonussequence_d(screen, jobs, job);
			RunScreenJob(jobs, job, completion);
			break;

		case 5:
			e4intro(nullptr);
			break;

		case 6:
			jobs[job++] = { Create<DDukeMultiplayerBonusScreen>(6) };
			RunScreenJob(jobs, job, completion);
			break;

		case 7:
			showtwoscreens(nullptr);
			break;

		case 8:
			doorders(nullptr);
			break;

		case 9:
			jobs[job++] = { Create<DDukeLevelSummaryScreen>() };
			RunScreenJob(jobs, job, completion);
			break;

		case 10:
			ud.eog = true;
			jobs[job++] = { Create<DDukeLevelSummaryScreen>() };
			RunScreenJob(jobs, job, completion);
			ud.eog = false;
			break;

		}
	}
}

END_DUKE_NS
