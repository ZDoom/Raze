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

static void BigText(double x, double y, const char* text, double alpha = 1.)
{
	auto width = BigFont->StringWidth(text);
	DrawText(twod, BigFont, CR_UNTRANSLATED, x - width / 2, y - 12, text, DTA_FullscreenScale, FSMode_Fit320x200, DTA_Alpha, alpha, TAG_DONE);
}

static void GameText(double x, double y, const char* t, int shade, int align = -1, int trans = 0)
{
	if (align != -1)
		x -= SmallFont->StringWidth(t) * (align == 0 ? 0.5 : 1);
	DrawText(twod, SmallFont, CR_UNDEFINED, x, y + 2, t, DTA_FullscreenScale, FSMode_Fit320x200, DTA_TranslationIndex, TRANSLATION(Translation_Remap, trans), DTA_Color, shadeToLight(shade), TAG_DONE);
}

static void MiniText(double x, double y, const char* t, int shade, int align = -1, int trans = 0)
{
	if (align != -1)
		x -= SmallFont2->StringWidth(t) * (align == 0 ? 0.5 : 1);
	DrawText(twod, SmallFont2, CR_UNDEFINED, x, y, t, DTA_FullscreenScale, FSMode_Fit320x200, DTA_TranslationIndex, TRANSLATION(Translation_Remap, trans), DTA_Color, shadeToLight(shade), TAG_DONE);
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

class DDRealmsScreen : public DSkippableScreenJob
{
public:
	DDRealmsScreen() : DSkippableScreenJob(fadein | fadeout)	{}

	void OnTick() override
	{
		if (ticks >= 7 * GameTicRate) state = finished;
	}

	void Draw(double smoothratio) override
	{
		const auto tex = tileGetTexture(DREALMS, true);
		int translation = tex->GetTexture()->GetImage()->UseGamePalette() ? TRANSLATION(Translation_BasePalettes, DREALMSPAL) : 0;

		twod->ClearScreen();
		DrawTexture(twod, tex, 0, 0, DTA_FullscreenEx, FSMode_ScaleToFit43, DTA_TranslationIndex, translation, DTA_LegacyRenderStyle, STYLE_Normal, TAG_DONE);
	}
};

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

class DTitleScreen : public DSkippableScreenJob
{
	int soundanm = 0;

public:
	DTitleScreen() : DSkippableScreenJob(fadein | fadeout) 
	{
	}

	void OnTick() override
	{
		int clock = ticks * 120 / GameTicRate;
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
			if (isPlutoPak()) S_PlaySound(FLY_BY, CHAN_AUTO, CHANF_UI);
		}
		else if (soundanm == 3 && clock >= 395)
		{
			soundanm = 4;
			if (isPlutoPak()) S_PlaySound(PIPEBOMB_EXPLODE, CHAN_AUTO, CHANF_UI);
		}

		if (clock > (860 + 120))
		{
			state = finished;
		}
	}

	void Draw(double smoothratio) override
	{
		twod->ClearScreen();
		int clock = (ticks + smoothratio) * 120 / GameTicRate;

		twod->ClearScreen();

		// Only translate if the image depends on the global palette.
		auto tex = tileGetTexture(BETASCREEN, true);
		int translation = tex->GetTexture()->GetImage()->UseGamePalette() ? TRANSLATION(Translation_BasePalettes, TITLEPAL) : 0;
		DrawTexture(twod, tex, 0, 0, DTA_FullscreenEx, FSMode_ScaleToFit43, DTA_TranslationIndex, translation, DTA_LegacyRenderStyle, STYLE_Normal, TAG_DONE);


		double scale = clamp(clock - 120, 0, 60) / 64.;
		if (scale > 0.)
		{
			tex = tileGetTexture(DUKENUKEM, true);
			translation = tex->GetTexture()->GetImage()->UseGamePalette() ? TRANSLATION(Translation_BasePalettes, TITLEPAL) : 0;

			DrawTexture(twod, tileGetTexture(DUKENUKEM, true), 160, 104, DTA_FullscreenScale, FSMode_Fit320x200,
				DTA_CenterOffsetRel, true, DTA_TranslationIndex, translation, DTA_ScaleX, scale, DTA_ScaleY, scale, TAG_DONE);
		}

		scale = clamp(clock - 220, 0, 30) / 32.;
		if (scale > 0.)
		{
			tex = tileGetTexture(THREEDEE, true);
			translation = tex->GetTexture()->GetImage()->UseGamePalette() ? TRANSLATION(Translation_BasePalettes, TITLEPAL) : 0;

			DrawTexture(twod, tileGetTexture(THREEDEE, true), 160, 129, DTA_FullscreenScale, FSMode_Fit320x200,
				DTA_CenterOffsetRel, true, DTA_TranslationIndex, translation, DTA_ScaleX, scale, DTA_ScaleY, scale, TAG_DONE);
		}

		if (isPlutoPak()) 
		{
			scale = (410 - clamp(clock, 280, 395)) / 16.;
			if (scale > 0. && clock > 280)
			{
				tex = tileGetTexture(PLUTOPAKSPRITE + 1, true);
				translation = tex->GetTexture()->GetImage()->UseGamePalette() ? TRANSLATION(Translation_BasePalettes, TITLEPAL) : 0;

				DrawTexture(twod, tileGetTexture(PLUTOPAKSPRITE + 1, true), 160, 151, DTA_FullscreenScale, FSMode_Fit320x200,
					DTA_CenterOffsetRel, true, DTA_TranslationIndex, translation, DTA_ScaleX, scale, DTA_ScaleY, scale, TAG_DONE);
			}
		}
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
	if (!userConfig.nologo)
	{
		if (!isShareware()) jobs[job++] = { PlayVideo("logo.anm", logosound, logoframetimes), []() { S_PlaySpecialMusic(MUS_INTRO); } };
		else jobs[job++] = { Create<DBlackScreen>(1), []() { S_PlaySpecialMusic(MUS_INTRO); } };
		if (!isNam()) jobs[job++] = { Create<DDRealmsScreen>(), nullptr };
	}
	else S_PlaySpecialMusic(MUS_INTRO);
	jobs[job++] = { Create<DTitleScreen>(), []() { S_PlaySound(NITEVISION_ONOFF, CHAN_AUTO, CHANF_UI); } };
	RunScreenJob(jobs, job, completion, true, true);
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

class DEpisode1End1 : public DSkippableScreenJob
{
	int bonuscnt = 0;
	int bossani = -1;
	int breatheani = -1;
	bool breathebg = false;

	static inline const int breathe[] =
	{
		 0,  30,VICTORY1 + 1,176,59,
		30,  60,VICTORY1 + 2,176,59,
		60,  90,VICTORY1 + 1,176,59,
		90, 120,0         ,176,59
	};

	static inline const int bossmove[] =
	{
		 0, 120,VICTORY1 + 3,86,59,
	   220, 260,VICTORY1 + 4,86,59,
	   260, 290,VICTORY1 + 5,86,59,
	   290, 320,VICTORY1 + 6,86,59,
	   320, 350,VICTORY1 + 7,86,59,
	   350, 380,VICTORY1 + 8,86,59,
	   350, 380,VICTORY1 + 8,86,59,
	};

public:
	DEpisode1End1() : DSkippableScreenJob(fadein | fadeout) {}

	void OnTick()
	{
		int currentclock = ticks * 120 / GameTicRate;

		bossani = -1;
		breathebg = false;
		breatheani = -1;

		// boss
		if (currentclock > 390 && currentclock < 780)
		{
			for (int t = 0; t < 35; t += 5) if (bossmove[t + 2] && (currentclock % 390) > bossmove[t] && (currentclock % 390) <= bossmove[t + 1])
			{
				if (t == 10 && bonuscnt == 1)
				{
					S_PlaySound(SHOTGUN_FIRE, CHAN_AUTO, CHANF_UI);
					S_PlaySound(SQUISHED, CHAN_AUTO, CHANF_UI);
					bonuscnt++;
				}
				bossani = t;
			}
		}

		// Breathe
		if (currentclock < 450 || currentclock >= 750)
		{
			if (currentclock >= 750)
			{
				breathebg = true;
				if (currentclock >= 750 && bonuscnt == 2)
				{
					S_PlaySound(DUKETALKTOBOSS, CHAN_AUTO, CHANF_UI);
					bonuscnt++;
				}
			}
			for (int t = 0; t < 20; t += 5)
				if (breathe[t + 2] && (currentclock % 120) > breathe[t] && (currentclock % 120) <= breathe[t + 1])
				{
					if (t == 5 && bonuscnt == 0)
					{
						S_PlaySound(BOSSTALKTODUKE, CHAN_AUTO, CHANF_UI);
						bonuscnt++;
					}
					breatheani = t;
				}
		}

	}

	void Draw(double) override
	{
		auto translation = TRANSLATION(Translation_BasePalettes, ENDINGPAL);

		twod->ClearScreen();
		DrawTexture(twod, tileGetTexture(VICTORY1, true), 0, 50, DTA_FullscreenScale, FSMode_Fit320x200, 
			DTA_TranslationIndex, translation, DTA_LegacyRenderStyle, STYLE_Normal, DTA_TopLeft, true, TAG_DONE);

		if (bossani != -1)
		{
			DrawTexture(twod, tileGetTexture(bossmove[bossani + 2], true), bossmove[bossani + 3], bossmove[bossani + 4], DTA_FullscreenScale, FSMode_Fit320x200,
				DTA_TranslationIndex, translation, DTA_TopLeft, true, TAG_DONE);
		}

		if (breathebg)
		{
			DrawTexture(twod, tileGetTexture(VICTORY1 + 8, true), 86, 59, DTA_FullscreenScale, FSMode_Fit320x200,
				DTA_TranslationIndex, translation, DTA_TopLeft, true, TAG_DONE);
		}

		if (breatheani != -1)
		{
			DrawTexture(twod, tileGetTexture(breathe[breatheani + 2], true), breathe[breatheani + 3], breathe[breatheani + 4], DTA_FullscreenScale, FSMode_Fit320x200,
				DTA_TranslationIndex, translation, DTA_TopLeft, true, TAG_DONE);
		}
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
		auto texid = TexMan.CheckForTexture("radlogo.anm", ETextureType::Any, FTextureManager::TEXMAN_TryAny | FTextureManager::TEXMAN_ForceLookup);
		if (texid.isValid()) return TexMan.GetGameTexture(texid);
		else return TexMan.GameByIndex(0);
	}

public:
	DEpisode3End() : DImageScreen(getTexture(), fadein|fadeout, 0x7fffffff)
	{
	}

	bool OnEvent(event_t* evt) override
	{
		if (evt->type == EV_GUI_KeyDown)
		{
			state = skipped;
			FX_StopAllSounds();
		}
		return true;
	}

	void OnTick() override
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
			if (!S_CheckSoundPlaying(ENDSEQVOL3SND9))
			{
				sound++;
				waittime = ticks + GameTicRate * (SoundEnabled() ? 1 : 5);	// if sound is off this wouldn't wait without a longer delay here.
			}
			break;

		case 6:
			if (isPlutoPak())
			{
				if (ticks > waittime) state = finished;
			}
			break;

		default:
			break;
		}
		if (state != running) FX_StopAllSounds();
	}
};

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

class DEpisode4Text : public DSkippableScreenJob
{
public:
	DEpisode4Text() : DSkippableScreenJob(fadein | fadeout) {}

	void Draw(double)
	{
		twod->ClearScreen();
		BigText(160, 60, GStrings("Thanks to all our"));
		BigText(160, 60 + 16, GStrings("fans for giving"));
		BigText(160, 60 + 16 + 16, GStrings("us big heads."));
		BigText(160, 70 + 16 + 16 + 16, GStrings("Look for a Duke Nukem 3D"));
		BigText(160, 70 + 16 + 16 + 16 + 16, GStrings("sequel soon."));
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

	void OnTick() override
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
		jobs[job++] = { Create<DEpisode1End1>(), nullptr };
		jobs[job++] = { Create<DImageScreen>(E1ENDSCREEN, DScreenJob::fadein|DScreenJob::fadeout, 0x7fffffff), []() { Mus_Stop(); } };
		break;

	case 1:
		Mus_Stop();
		jobs[job++] = { PlayVideo("cineov2.anm", cineov2sound, framespeed_18), []() { S_PlaySound(PIPEBOMB_EXPLODE, CHAN_AUTO, CHANF_UI); } };
		jobs[job++] = { Create<DImageScreen>(E2ENDSCREEN, DScreenJob::fadein | DScreenJob::fadeout, 0x7fffffff), []() { FX_StopAllSounds(); } };
		break;

	case 2:
		Mus_Stop();
		if (g_gameType & GAMEFLAG_DUKEDC)
		{
			jobs[job++] = { PlayVideo("radlogo.anm", dukedcsound, framespeed_10), nullptr };
		}
		else
		{
			jobs[job++] = { PlayVideo("cineov3.anm", cineov3sound, framespeed_10), nullptr };
			jobs[job++] = { Create<DBlackScreen>(200), []() { FX_StopAllSounds(); } };
			jobs[job++] = { Create<DEpisode3End>(), []() { if (!isPlutoPak()) S_PlaySound(ENDSEQVOL3SND4, CHAN_AUTO, CHANF_UI); } };
			if (!isPlutoPak()) jobs[job++] = { Create<DImageScreen>(TexMan.GetGameTextureByName("DUKETEAM.ANM", false, FTextureManager::TEXMAN_ForceLookup),
				DScreenJob::fadein | DScreenJob::fadeout, 0x7fffffff), []() { FX_StopAllSounds(); } };
		}
		break;

	case 3:
		Mus_Stop();
		jobs[job++] = { PlayVideo("vol4e1.anm", vol4e1, framespeed_10), nullptr };
		jobs[job++] = { PlayVideo("vol4e2.anm", vol4e2, framespeed_10), nullptr };
		jobs[job++] = { PlayVideo("vol4e3.anm", vol4e3, framespeed_10), []() { S_PlaySound(ENDSEQVOL3SND4, CHAN_AUTO, CHANF_UI); } };
		jobs[job++] = { Create<DEpisode4Text>(), nullptr };
		jobs[job++] = { Create<DImageScreen>(TexMan.GetGameTextureByName("DUKETEAM.ANM", false, FTextureManager::TEXMAN_ForceLookup), 
			DScreenJob::fadein | DScreenJob::fadeout, 0x7fffffff),  []() { FX_StopAllSounds(); } };
		break;

	case 4:
		Mus_Stop();
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

class DDukeMultiplayerBonusScreen : public DSkippableScreenJob
{
	int playerswhenstarted;

public:
	DDukeMultiplayerBonusScreen(int pws) : DSkippableScreenJob(fadein|fadeout)
	{
		playerswhenstarted = pws;
	}

	void OnTick() override
	{
		if (ticks == 1) S_PlayBonusMusic();
	}

	void Draw(double smoothratio) override
	{
		char tempbuf[32];
		int currentclock = int((ticks + smoothratio) * 120 / GameTicRate);
		twod->ClearScreen();
		DrawTexture(twod, tileGetTexture(MENUSCREEN), 0, 0, DTA_FullscreenEx, FSMode_ScaleToFit43, DTA_Color, 0xff808080, DTA_LegacyRenderStyle, STYLE_Normal, TAG_DONE);
		DrawTexture(twod, tileGetTexture(INGAMEDUKETHREEDEE, true), 160, 34, DTA_FullscreenScale, FSMode_Fit320x200, DTA_CenterOffsetRel, true, TAG_DONE);
		if (isPlutoPak())
			DrawTexture(twod, tileGetTexture(PLUTOPAKSPRITE+2, true), 260, 36, DTA_FullscreenScale, FSMode_Fit320x200, DTA_CenterOffsetRel, true, TAG_DONE);

		GameText(160, 58 + 2, GStrings("Multiplayer Totals"), 0, 0);
		GameText(160, 58 + 10, currentLevel->DisplayName(), 0, 0);
		GameText(160, 165, GStrings("Presskey"), 8 - int(sin(currentclock / 10.) * 8), 0);

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
			MiniText(38, 90 + t, PlayerName(i), 0, -1, ps[i].palookup);

			for (int y = 0; y < playerswhenstarted; y++)
			{
				int frag = ps[i].frags[y];
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
				int frag = ps[i].frags[y];
				yfragtotal += frag;
			}
			mysnprintf(tempbuf, 32, "%-4d", yfragtotal);
			MiniText(92 + (y * 23), 96 + (8 * 7), tempbuf, 0, -1, 2);
		}

		MiniText(45, 96 + (8 * 7), GStrings("Deaths"), 0, -1, 8);
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
	int speech = -1;
	int displaystate = 0;
	int dukeAnimStart;

	enum
	{
		printTimeText = 1,
		printTimeVal = 2,
		printKillsText = 4,
		printKillsVal = 8,
		printSecretsText = 16,
		printSecretsVal = 32,
		printStatsAll = 63,
		dukeAnim = 64,
		dukeWait = 128,

	};

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
	}

	void FormatTime(int time, char* tempbuf)
	{
		mysnprintf(tempbuf, 32, "%02d:%02d", (time / (26 * 60)) % 60, (time / 26) % 60);
	}

	bool OnEvent(event_t* ev) override
	{
		if (ev->type == EV_KeyDown)
		{
			if ((displaystate & printStatsAll) != printStatsAll)
			{
				S_PlaySound(PIPEBOMB_EXPLODE, CHAN_AUTO, CHANF_UI);
				displaystate = printStatsAll;
			}
			else if (!(displaystate & dukeAnim))
			{
				displaystate |= dukeAnim;
				dukeAnimStart = ticks;
				S_PlaySound(SHOTGUN_COCK, CHAN_AUTO, CHANF_UI);
				static const uint16_t speeches[] = { BONUS_SPEECH1, BONUS_SPEECH2, BONUS_SPEECH3, BONUS_SPEECH4 };
				speech = speeches[(rand() & 3)];
				S_PlaySound(speech, CHAN_AUTO, CHANF_UI, 1);
			}
			return true;
		}
		return false;
	}

	void OnTick() override
	{
		if (ticks == 1) S_PlayBonusMusic();
		if ((displaystate & printStatsAll) != printStatsAll)
		{
			if (ticks == 15 * 3)
			{
				displaystate |= printTimeText;
			}
			else if (ticks == 15 * 4)
			{
				displaystate |= printTimeVal;
				S_PlaySound(PIPEBOMB_EXPLODE, CHAN_AUTO, CHANF_UI);
			}
			else if (ticks == 15 * 6)
			{
				displaystate |= printKillsText;
				S_PlaySound(FLY_BY, CHAN_AUTO, CHANF_UI);
			}
			else if (ticks == 15 * 7)
			{
				displaystate |= printKillsVal;
				S_PlaySound(PIPEBOMB_EXPLODE, CHAN_AUTO, CHANF_UI);
			}
			else if (ticks == 15 * 9)
			{
				displaystate |= printSecretsText;
			}
			else if (ticks == 15 * 10)
			{
				displaystate |= printSecretsVal;
				S_PlaySound(PIPEBOMB_EXPLODE, CHAN_AUTO, CHANF_UI);
			}
		}
		if (displaystate & dukeAnim)
		{
			if (ticks >= dukeAnimStart + 60)
			{
				displaystate ^= dukeAnim | dukeWait;
			}
		}
		if (displaystate & dukeWait)
		{
			if (speech <= 0 || !soundEngine->GetSoundPlayingInfo(SOURCE_None, nullptr, speech)) 
				state = finished;
		}
	}

	void PrintTime()
	{
		char tempbuf[32];
		GameText(10, 59 + 9, GStrings("TXT_YourTime"), 0);
		GameText(10, 69 + 9, GStrings("TXT_ParTime"), 0);
		if (!isNamWW2GI())
			GameText(10, 79 + 9, GStrings("TXT_3DRTIME"), 0);

		if (displaystate & printTimeVal)
		{
			FormatTime(ps[myconnectindex].player_par, tempbuf);
			GameText((320 >> 2) + 71, 59 + 9, tempbuf, 0);

			FormatTime(currentLevel->parTime, tempbuf);
			GameText((320 >> 2) + 71, 69 + 9, tempbuf, 0);

			if (!isNamWW2GI())
			{
				FormatTime(currentLevel->designerTime, tempbuf);
				GameText((320 >> 2) + 71, 79 + 9, tempbuf, 0);
			}
		}
	}

	void PrintKills()
	{
		char tempbuf[32];
		GameText(10, 94 + 9, GStrings("TXT_EnemiesKilled"), 0);
		GameText(10, 104 + 9, GStrings("TXT_EnemiesLeft"), 0);

		if (displaystate & printKillsVal)
		{
			mysnprintf(tempbuf, 32, "%-3d", ps[myconnectindex].actors_killed);
			GameText((320 >> 2) + 70, 94 + 9, tempbuf, 0);

			if (ud.player_skill > 3)
			{
				mysnprintf(tempbuf, 32, "%s", GStrings("TXT_N_A"));
			}
			else
			{
				if ((ps[myconnectindex].max_actors_killed - ps[myconnectindex].actors_killed) < 0)
					mysnprintf(tempbuf, 32, "%-3d", 0);
				else mysnprintf(tempbuf, 32, "%-3d", ps[myconnectindex].max_actors_killed - ps[myconnectindex].actors_killed);
				
			}
			GameText((320 >> 2) + 70, 104 + 9, tempbuf, 0);
		}
	}

	void PrintSecrets()
	{
		char tempbuf[32];
		GameText(10, 119 + 9, GStrings("TXT_SECFND"), 0);
		GameText(10, 129 + 9, GStrings("TXT_SECMISS"), 0);

		if (displaystate & printSecretsVal)
		{
			mysnprintf(tempbuf, 32, "%-3d", ps[myconnectindex].secret_rooms);
			GameText((320 >> 2) + 70, 119 + 9, tempbuf, 0);
			if (ps[myconnectindex].secret_rooms > 0)
				sprintf(tempbuf, "%-3d", (100 * ps[myconnectindex].secret_rooms / ps[myconnectindex].max_secret_rooms));
			mysnprintf(tempbuf, 32, "%-3d", ps[myconnectindex].max_secret_rooms - ps[myconnectindex].secret_rooms);
			GameText((320 >> 2) + 70, 129 + 9, tempbuf, 0);
		}
	}

	void Draw(double) override
	{
		twod->ClearScreen();
		DrawTexture(twod, tileGetTexture(gfx_offset, true), 0, 0, DTA_FullscreenEx, FSMode_ScaleToFit43, DTA_LegacyRenderStyle, STYLE_Normal, TAG_DONE);

		GameText(160, 190, GStrings("PRESSKEY"), 8 - int(sin(ticks * 12 / GameTicRate) * 8), 0);

		if (displaystate & printTimeText)
		{
			PrintTime();
		}
		if (displaystate & printKillsText)
		{
			PrintKills();
		}
		if (displaystate & printSecretsText)
		{
			PrintSecrets();
		}

		if (displaystate & dukeAnim)
		{
			switch (((ticks - dukeAnimStart) >> 2) % 15)
			{
				case 0:
				case 1:
				case 4:
				case 5:
					DrawTexture(twod, tileGetTexture(gfx_offset + 3), 199, 31, DTA_FullscreenScale, FSMode_Fit320x200, DTA_TopLeft, true, TAG_DONE);
					break;
				case 2:
				case 3:
					DrawTexture(twod, tileGetTexture(gfx_offset + 4), 199, 31, DTA_FullscreenScale, FSMode_Fit320x200, DTA_TopLeft, true, TAG_DONE);
					break;
			}
		}
		else if (!(displaystate & dukeWait))
		{
			switch((ticks >> 3) & 3)
			{
				case 1:
				case 3:
					DrawTexture(twod, tileGetTexture(gfx_offset + 1), 199, 31, DTA_FullscreenScale, FSMode_Fit320x200, DTA_TopLeft, true, TAG_DONE);
					break;
				case 2:
					DrawTexture(twod, tileGetTexture(gfx_offset + 2), 199, 31, DTA_FullscreenScale, FSMode_Fit320x200, DTA_TopLeft, true, TAG_DONE);
					break;
			}
		}

		if (lastmapname) BigText(160, 20 - 6, lastmapname);
		BigText(160, 36 - 6, GStrings("Completed"));
	}

};

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void dobonus_d(int bonusonly, const CompletionFunc& completion)
{
	JobDesc jobs[20];
	int job = 0;

	FX_StopAllSounds();

	if (bonusonly < 0 && numplayers < 2 && ud.from_bonus == 0)
	{
		bonussequence_d(volfromlevelnum(currentLevel->levelNumber), jobs, job);
	}
	else
		Mus_Stop();

	if (playerswhenstarted > 1 && ud.coop != 1)
	{
		jobs[job++] = { Create<DDukeMultiplayerBonusScreen>(playerswhenstarted) };
	}
	else if (bonusonly <= 0 && ud.multimode <= 1)
	{
		jobs[job++] = { Create<DDukeLevelSummaryScreen>() };
	}
	if (job)
	{
		RunScreenJob(jobs, job, completion);
	}
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
	MapRecord* rec;
	
public:
	DDukeLoadScreen(MapRecord *maprec) : DScreenJob(0), rec(maprec) {}

	void Draw(double) override
	{
		twod->ClearScreen();
		DrawTexture(twod, tileGetTexture(LOADSCREEN), 0, 0, DTA_FullscreenEx, FSMode_ScaleToFit43, DTA_LegacyRenderStyle, STYLE_Normal, TAG_DONE);
		
		BigText(160, 90, (rec->flags & MI_USERMAP)? GStrings("TXT_LOADUM") :  GStrings("TXT_LOADING"));
		BigText(160, 114, rec->DisplayName());
	}
};

void loadscreen_d(MapRecord *rec, CompletionFunc func)
{
	JobDesc job = { Create<DDukeLoadScreen>(rec) };
	RunScreenJob(&job, 1, func);
}

void PrintPaused_d()
{
	BigText(160, 100, GStrings("Game Paused"));
}


END_DUKE_NS
