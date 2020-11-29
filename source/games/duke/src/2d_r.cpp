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
	auto width = BigFont->StringWidth(text);
	DrawText(twod, BigFont, CR_UNTRANSLATED, x, y - 12, text, DTA_FullscreenScale, FSMode_Fit320x200, DTA_ScaleX, 0.4, DTA_ScaleY, 0.4, DTA_Alpha, alpha, TAG_DONE);
}

static void GameText(double x, double y, const char* t, int shade, int align = -1, int trans = 0)
{
	x *= 2; y *= 2;
	if (align != -1)
		x -= SmallFont->StringWidth(t) * (align == 0 ? 0.5 : 1);
	DrawText(twod, SmallFont, CR_UNDEFINED, x, y + 2, t, DTA_FullscreenScale, FSMode_Fit640x400, DTA_TranslationIndex, TRANSLATION(Translation_Remap, trans), DTA_Color, shadeToLight(shade), TAG_DONE);
}

static void MiniText(double x, double y, const char* t, int shade, int align = -1, int trans = 0)
{
	x *= 2; y *= 2;
	if (align != -1)
		x -= SmallFont2->StringWidth(t) * (align == 0 ? 0.5 : 1);
	DrawText(twod, SmallFont2, CR_UNDEFINED, x, y, t, DTA_FullscreenScale, FSMode_Fit640x400, DTA_TranslationIndex, TRANSLATION(Translation_Remap, trans), DTA_Color, shadeToLight(shade), TAG_DONE);
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void Logo_r(const CompletionFunc& completion)
{
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

	JobDesc jobs[3];
	int job = 0;

	if (userConfig.nologo)
	{
		completion(false);
		return;
	}
	else if (!isRRRA())
	{
		jobs[job++] = { PlayVideo("rr_intro.anm", introsound, framespeed), nullptr };
		jobs[job++] = { PlayVideo("redneck.anm", rednecksound, framespeed), nullptr };
		jobs[job++] = { PlayVideo("xatlogo.anm", xatrixsound, framespeed), nullptr };
	}
	else
	{
		jobs[job++] = { PlayVideo("redint.mve"), nullptr };
	}
	RunScreenJob(jobs, job, completion, true, true);
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

static void bonussequence_r(int num, JobDesc* jobs, int& job)
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
		jobs[job++] = { PlayVideo("turdmov.anm", turdmov, framespeed), nullptr };
		jobs[job++] = { Create<DImageScreen>(TENSCREEN), nullptr };
		break;

	case 1:
		jobs[job++] = { PlayVideo("rr_outro.anm", rr_outro, framespeed), nullptr };
		jobs[job++] = { Create<DImageScreen>(TENSCREEN), nullptr };
		break;

	default:
		break;
	}
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

class DRRMultiplayerBonusScreen : public DScreenJob
{
	int playerswhenstarted;

public:
	DRRMultiplayerBonusScreen(int pws) : DScreenJob(fadein | fadeout)
	{
		playerswhenstarted = pws;
	}

	int Frame(uint64_t clock, bool skiprequest)
	{
		if (clock == 0) S_PlayBonusMusic();
		char tempbuf[32];
		twod->ClearScreen();
		DrawTexture(twod, tileGetTexture(MENUSCREEN), 0, 0, DTA_FullscreenEx, FSMode_ScaleToFit43, DTA_Color, 0xff808080, DTA_LegacyRenderStyle, STYLE_Normal, TAG_DONE);
		double scale = 0.36;
		DrawTexture(twod, tileGetTexture(INGAMEDUKETHREEDEE, true), 160, 34, DTA_FullscreenScale, FSMode_Fit320x200, 
			DTA_CenterOffsetRel, true, DTA_ScaleX, scale, DTA_ScaleY, 0.36, TAG_DONE);

		GameText(160, 58, GStrings("Multiplayer Totals"), 0, 0);
		GameText(160, 58 + 10, currentLevel->DisplayName(), 0, 0);
		GameText(160, 165, GStrings("Presskey"), 0, 0);

		int t = 0;

		MiniText(38, 80, GStrings("Name"), 0);
		MiniText(269 + 20, 80, GStrings("Kills"), 0, 1);

		for (int i = 0; i < playerswhenstarted; i++)
		{
			mysnprintf(tempbuf, 32, "%-4d", i + 1);
			MiniText(92 + (i * 23), 80, tempbuf, 0);
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
					MiniText(92 + (y * 23), 90 + t, tempbuf, 0);
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
			MiniText(101 + (8 * 23), 90 + t, tempbuf, 0);

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
			MiniText(92 + (y * 23), 96 + (8 * 7), tempbuf, 0);
		}

		MiniText(45, 96 + (8 * 7), GStrings("Deaths"), 0);
		return skiprequest ? -1 : 1;
	}
};

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

class DRRLevelSummaryScreen : public DScreenJob
{
	const char* lastmapname;
	int gfx_offset;
	int bonuscnt = 0;
	int speech = -1;

	void SetTotalClock(int tc)
	{
		SetClock(tc * (uint64_t)1'000'000'000 / 120);
	}

public:
	DRRLevelSummaryScreen(bool dofadeout = true) : DScreenJob(dofadeout? (fadein | fadeout) : fadein)
	{
		if (currentLevel->flags & MI_USERMAP)
			gfx_offset = BONUSPIC01;
		else if (!isRRRA())
			gfx_offset = BONUSPIC01 + clamp((currentLevel->levelNumber / 100) * 7 + (currentLevel->levelNumber % 100), 0, 13);
		else
			gfx_offset = LEVELMAP01 + clamp((currentLevel->levelNumber / 100) * 7 + (currentLevel->levelNumber % 100), 0, 13);
		

		lastmapname = currentLevel->DisplayName();
	}

	void FormatTime(int time, char* tempbuf)
	{
		mysnprintf(tempbuf, 32, "%02d:%02d", (time / (26 * 60)) % 60, (time / 26) % 60);
	}

	void PrintTime(int currentclock)
	{
		char tempbuf[32];
		BigText(30, 48, GStrings("TXT_YerTime"), -1);
		BigText(30, 64, GStrings("TXT_ParTime"), -1);
		BigText(30, 80, GStrings("TXT_XTRTIME"), -1);

		if (bonuscnt == 0)
			bonuscnt++;

		if (currentclock > (60 * 4))
		{
			if (bonuscnt == 1)
			{
				bonuscnt++;
				S_PlaySound(404, CHAN_AUTO, CHANF_UI);
			}
			FormatTime(ps[myconnectindex].player_par, tempbuf);
			BigText(191, 48, tempbuf, -1);

			FormatTime(currentLevel->parTime, tempbuf);
			BigText(191, 64, tempbuf, -1);

			if (!isNamWW2GI())
			{
				FormatTime(currentLevel->designerTime, tempbuf);
				BigText(191, 80, tempbuf, -1);
			}
		}
	}

	void PrintKills(int currentclock)
	{
		char tempbuf[32];
		BigText(30, 112, GStrings("TXT_VarmintsKilled"), -1);
		BigText(30, 128, GStrings("TXT_VarmintsLeft"), -1);

		if (bonuscnt == 2)
			bonuscnt++;

		if (currentclock > (60 * 7))
		{
			if (bonuscnt == 3)
			{
				bonuscnt++;
				S_PlaySound(442, CHAN_AUTO, CHANF_UI);
			}
			mysnprintf(tempbuf, 32, "%-3d", ps[myconnectindex].actors_killed);
			BigText(231, 112, tempbuf, -1);
			if (ud.player_skill > 3)
			{
				mysnprintf(tempbuf, 32, "%s", GStrings("TXT_N_A"));
				BigText(231, 128, tempbuf, -1);
			}
			else
			{
				if ((ps[myconnectindex].max_actors_killed - ps[myconnectindex].actors_killed) < 0)
					mysnprintf(tempbuf, 32, "%-3d", 0);
				else mysnprintf(tempbuf, 32, "%-3d", ps[myconnectindex].max_actors_killed - ps[myconnectindex].actors_killed);
				BigText(231, 128, tempbuf, -1);
			}
		}
	}

	void PrintSecrets(int currentclock)
	{
		char tempbuf[32];
		BigText(30, 144, GStrings("TXT_SECFND"), -1);
		BigText(30, 160, GStrings("TXT_SECMISS"), -1);
		if (bonuscnt == 4) bonuscnt++;

		if (currentclock > (60 * 10))
		{
			if (bonuscnt == 5)
			{
				bonuscnt++;
				S_PlaySound(404, CHAN_AUTO, CHANF_UI);
			}
			mysnprintf(tempbuf, 32, "%-3d", ps[myconnectindex].secret_rooms);
			BigText(231, 144, tempbuf, -1);
			if (ps[myconnectindex].secret_rooms > 0)
				sprintf(tempbuf, "%-3d", (100 * ps[myconnectindex].secret_rooms / ps[myconnectindex].max_secret_rooms));
			mysnprintf(tempbuf, 32, "%-3d", ps[myconnectindex].max_secret_rooms - ps[myconnectindex].secret_rooms);
			BigText(231, 160, tempbuf, -1);
		}
	}

	int Frame(uint64_t clock, bool skiprequest)
	{
		if (clock == 0) S_PlayBonusMusic();
		twod->ClearScreen();
		int currentclock = int(clock * 120 / 1'000'000'000);
		DrawTexture(twod, tileGetTexture(gfx_offset, true), 0, 0, DTA_FullscreenEx, FSMode_ScaleToFit43, DTA_LegacyRenderStyle, STYLE_Normal, TAG_DONE);

		if (lastmapname) BigText(80, 16, lastmapname, -1);
		BigText(15, 192, GStrings("PRESSKEY"), -1);

		if (currentclock > (60 * 3))
		{
			PrintTime(currentclock);
		}
		if (currentclock > (60 * 6))
		{
			PrintKills(currentclock);
		}
		if (currentclock > (60 * 9))
		{
			PrintSecrets(currentclock);
		}

		if (currentclock > (1000000000L) && currentclock < (1000000320L))
		{
			int val = (currentclock >> 4) % 15;
			if (val == 0)
			{
				if (bonuscnt == 6)
				{
					bonuscnt++;
					S_PlaySound(425, CHAN_AUTO, CHANF_UI);
					speech = BONUS_SPEECH1 + (rand() & 3);
					S_PlaySound(speech, CHAN_AUTO, CHANF_UI);
				}
			}
		}
		else if (currentclock > (10240 + 120L))
		{
			if (speech > 0 && !skiprequest && soundEngine->GetSoundPlayingInfo(SOURCE_None, nullptr, speech)) return 1;
			return 0;
		}

		if (currentclock > 10240 && currentclock < 10240 + 10240)
			SetTotalClock(1024);

		if (skiprequest && currentclock > (60 * 2))
		{
			skiprequest = false;
			if (currentclock < (60 * 13))
			{
				SetTotalClock(60 * 13);
			}
			else if (currentclock < (1000000000))
				SetTotalClock(1000000000);
		}

		return 1;
	}

};


class DRRRAEndOfGame : public DScreenJob
{
public:
	DRRRAEndOfGame() : DScreenJob(fadein|fadeout)
	{
		S_PlaySound(35, CHAN_AUTO, CHANF_UI);
	}
	int Frame(uint64_t clock, bool skiprequest)
	{
		int currentclock = int(clock * 120 / 1'000'000'000);
		auto tex = tileGetTexture(ENDGAME + ((currentclock >> 4) & 1));
		DrawTexture(twod, tex, 0, 0, DTA_FullscreenEx, FSMode_ScaleToFit43, TAG_DONE);
		if (!S_CheckSoundPlaying(-1, 35) && currentclock > 15*120) return 0; // make sure it stays, even if sound is off.
		if (skiprequest)
		{
			S_StopSound(35);
			return -1;
		}
		return 1;
	}
};

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void dobonus_r(int bonusonly, const CompletionFunc& completion)
{
	JobDesc jobs[20];
	int job = 0;

	FX_StopAllSounds();
	Mus_Stop();

	if (bonusonly < 0 && !isRRRA() && numplayers < 2 && ud.from_bonus == 0)
	{
		int vol = volfromlevelnum(currentLevel->levelNumber);
		bonussequence_r(vol, jobs, job);
	}

	if (playerswhenstarted > 1 && ud.coop != 1)
	{
		jobs[job++] = { Create<DRRMultiplayerBonusScreen>(playerswhenstarted) };
	}
	else if (bonusonly <= 0 && ud.multimode <= 1)
	{
		if (isRRRA() && !(currentLevel->flags & MI_USERMAP) && currentLevel->levelNumber < 106) // fixme: The logic here is awful. Shift more control to the map records.
		{
			jobs[job++] = { Create<DRRLevelSummaryScreen>(true) };
			int levnum = clamp((currentLevel->levelNumber / 100) * 7 + (currentLevel->levelNumber % 100), 0, 13);
			char fn[20];
			mysnprintf(fn, 20, "lvl%d.anm", levnum + 1);
			static const int framespeed[] = { 20, 20, 7200 };   // wait for one minute on the final frame so that the video doesn't stop  before the user notices.
			jobs[job++] = { PlayVideo(fn, nullptr, framespeed) };
			if (bonusonly < 0 && currentLevel->levelNumber > 100)
			{
				jobs[job++] = { Create<DRRRAEndOfGame>() };
			}
		}
		else jobs[job++] = { Create<DRRLevelSummaryScreen>(false) };
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

class DRRLoadScreen : public DScreenJob
{
	MapRecord* rec;

public:
	DRRLoadScreen(MapRecord* maprec) : DScreenJob(0), rec(maprec) {}

	int Frame(uint64_t clock, bool skiprequest)
	{
		DrawTexture(twod, tileGetTexture(LOADSCREEN), 0, 0, DTA_FullscreenEx, FSMode_ScaleToFit43, DTA_LegacyRenderStyle, STYLE_Normal, TAG_DONE);
		
		int y = isRRRA()? 140 : 90;
		BigText(160, y, (rec->flags & MI_USERMAP) ? GStrings("TXT_ENTRUM") : GStrings("TXT_ENTERIN"), 0);
		BigText(160, y+24, rec->DisplayName(), 0);
		return 0;
	}
};

void loadscreen_r(MapRecord* rec, CompletionFunc func)
{
	JobDesc job = { Create<DRRLoadScreen>(rec) };
	RunScreenJob(&job, 1, func);
}

void PrintPaused_r()
{
	BigText(160, 100, GStrings("Game Paused"), 0);
}


END_DUKE_NS
