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
#include "names.h"
#include "animtexture.h"
#include "animlib.h"
#include "raze_music.h"
#include "mapinfo.h"
#include "screenjob.h"
#include "texturemanager.h"
#include "buildtiles.h"
//#include "zz_text.h"

#undef gametext
//#undef menutext

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
        if (tile && tile->GetTexelWidth() > 0 && tile->GetTexelHeight() > 0)
            fontdata.Insert('!' + i, tile);
    }
    SmallFont = new ::FFont("SmallFont", nullptr, "defsmallfont", 0, 0, 0, -1, -1, false, false, false, &fontdata);
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
        if (tile && tile->GetTexelWidth() > 0 && tile->GetTexelHeight() > 0)
            fontdata.Insert('!' + i, tile);
    }
    fontdata.Insert(1, TexMan.FindGameTexture("TINYBLAK")); // this is only here to widen the color range of the font to produce a better translation.
    SmallFont2 = new ::FFont("SmallFont2", nullptr, "defsmallfont2", 0, 0, 0, -1, -1, false, false, false, &fontdata);
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


// Text output - needs to transition to the actual font routines once everything is set up.
#if 1
static int gametext(int x,int y,const char *t,char s,short dabits)
{
	short ac,newx;
	char centre;
	const char *oldt;

	centre = ( x == (320>>1) );
	newx = 0;
	oldt = t;

	if(centre)
	{
		while(*t)
		{
			if(*t == 32) {newx+=5;t++;continue;}
			else ac = *t - '!' + STARTALPHANUM;

			if( ac < STARTALPHANUM || ac > ENDALPHANUM ) break;

			if(*t >= '0' && *t <= '9')
				newx += 8;
			else newx += tilesiz[ac].x;
			t++;
		}

		t = oldt;
		x = (320>>1)-(newx>>1);
	}

	while(*t)
	{
		if(*t == 32) {x+=5;t++;continue;}
		else ac = *t - '!' + STARTALPHANUM;

		if( ac < STARTALPHANUM || ac > ENDALPHANUM )
			break;

		rotatesprite(x<<16,y<<16,65536L,0,ac,s,0,dabits,0,0,xdim-1,ydim-1);

		if(*t >= '0' && *t <= '9')
			x += 8;
		else x += tilesiz[ac].x;

		t++;
	}

	return (x);
}

static int gametextpal(int x,int y,const char *t,char s,unsigned char p)
{
	short ac,newx;
	char centre;
	const char *oldt;

	centre = ( x == (320>>1) );
	newx = 0;
	oldt = t;

	if(centre)
	{
		while(*t)
		{
			if(*t == 32) {newx+=5;t++;continue;}
			else ac = *t - '!' + STARTALPHANUM;

			if( ac < STARTALPHANUM || ac > ENDALPHANUM ) break;

			if(*t >= '0' && *t <= '9')
				newx += 8;
			else newx += tilesiz[ac].x;
			t++;
		}

		t = oldt;
		x = (320>>1)-(newx>>1);
	}

	while(*t)
	{
		if(*t == 32) {x+=5;t++;continue;}
		else ac = *t - '!' + STARTALPHANUM;

		if( ac < STARTALPHANUM || ac > ENDALPHANUM )
			break;

		rotatesprite(x<<16,y<<16,65536L,0,ac,s,p,2+8+16,0,0,xdim-1,ydim-1);
		if(*t >= '0' && *t <= '9')
			x += 8;
		else x += tilesiz[ac].x;

		t++;
	}

	return (x);
}

static int gametextpart(int x,int y,const char *t,char s,short p)
{
	short ac,newx, cnt;
	char centre;
	const char *oldt;

	centre = ( x == (320>>1) );
	newx = 0;
	oldt = t;
	cnt = 0;

	if(centre)
	{
		while(*t)
		{
			if(cnt == p) break;

			if(*t == 32) {newx+=5;t++;continue;}
			else ac = *t - '!' + STARTALPHANUM;

			if( ac < STARTALPHANUM || ac > ENDALPHANUM ) break;

			newx += tilesiz[ac].x;
			t++;
			cnt++;

		}

		t = oldt;
		x = (320>>1)-(newx>>1);
	}

	cnt = 0;
	while(*t)
	{
		if(*t == 32) {x+=5;t++;continue;}
		else ac = *t - '!' + STARTALPHANUM;

		if( ac < STARTALPHANUM || ac > ENDALPHANUM ) break;

		if(cnt == p)
		{
			rotatesprite(x<<16,y<<16,65536L,0,ac,s,1,2+8+16,0,0,xdim-1,ydim-1);
			break;
		}
		else
			rotatesprite(x<<16,y<<16,65536L,0,ac,s,0,2+8+16,0,0,xdim-1,ydim-1);

		x += tilesiz[ac].x;

		t++;
		cnt++;
	}

	return (x);
}

static void gamenumber(int x,int y,int n,char s)
{
	char b[10];
	//ltoa(n,b,10);
	mysnprintf(b,10,"%d",n);
	gametext(x,y,b,s,2+8+16);
}
#endif

void intro4animsounds(int fr)
{
	switch(fr)
	{
		case 1:
			sound(INTRO4_B);
			break;
		case 12:
		case 34:
			sound(SHORT_CIRCUIT);
			break;
		case 18:
			sound(INTRO4_5);
			break;
	}
}

void first4animsounds(int fr)
{
	switch(fr)
	{
		case 1:
			sound(INTRO4_1);
			break;
		case 12:
			sound(INTRO4_2);
			break;
		case 7:
			sound(INTRO4_3);
			break;
		case 26:
			sound(INTRO4_4);
			break;
	}
}

void intro42animsounds(int fr)
{
	switch(fr)
	{
		case 10:
			sound(INTRO4_6);
			break;
	}
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

class DDRealmsScreen : public DScreenJob
{
	int Frame(uint64_t clock, bool skiprequest) override
	{
		const int duration = 7500;
		const auto tex = tileGetTexture(DREALMS, true);
		const int translation = TRANSLATION(Translation_BasePalettes, DREALMSPAL);

		int span = int(clock / 1'000'000);
		int light = 255;
		if (span < 255) light = span;
		else if (span > duration - 255) light = duration - span;
		light = clamp(light, 0, 255);
		PalEntry pe(255, light, light, light);
		twod->ClearScreen();
		DrawTexture(twod, tex, 0, 0, DTA_FullscreenEx, 3, DTA_TranslationIndex, translation, DTA_Color, pe, DTA_LegacyRenderStyle, STYLE_Normal, TAG_DONE);
		return skiprequest ? -1 : span < duration ? 1 : 0;
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

	int Frame(uint64_t nsclock, bool skiprequest) override
	{
		twod->ClearScreen();
		int clock = nsclock * 120 / 1'000'000'000;
		// Make this draw an empty frame before it ends.
		if (clock > (860 + 120))
		{
			return 0;
		}

		auto translation = TRANSLATION(Translation_BasePalettes, TITLEPAL);
		uint64_t span = nsclock / 1'000'000;
		int light = 255;
		if (span < 255) light = span;
		//else if (span > duration - 255) light = duration - span;
		light = clamp(light, 0, 255);
		PalEntry pe(255, light, light, light);

		twod->ClearScreen();
		DrawTexture(twod, tileGetTexture(BETASCREEN, true), 0, 0, DTA_FullscreenEx, 3, DTA_TranslationIndex, translation, DTA_Color, pe, DTA_LegacyRenderStyle, STYLE_Normal, TAG_DONE);

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
				DTA_CenterOffset, true,  DTA_TranslationIndex, translation, DTA_Color, pe, DTA_ScaleX, scale, DTA_ScaleY, scale, TAG_DONE);

		scale = clamp(clock - 220, 0, 30) / 32.;
		if (scale > 0.)
			DrawTexture(twod, tileGetTexture(THREEDEE, true), 160, 129, DTA_FullscreenScale, 3, DTA_VirtualWidth, 320, DTA_VirtualHeight, 200,
				DTA_CenterOffset, true, DTA_TranslationIndex, translation, DTA_Color, pe, DTA_ScaleX, scale, DTA_ScaleY, scale, TAG_DONE);

		if (PLUTOPAK) 
		{
			scale = (410 - clamp(clock, 280, 395)) / 16.;
			if (scale > 0. && clock > 280)
				DrawTexture(twod, tileGetTexture(PLUTOPAKSPRITE+1, true), 160, 151, DTA_FullscreenScale, 3, DTA_VirtualWidth, 320, DTA_VirtualHeight, 200,
					DTA_CenterOffset, true, DTA_TranslationIndex, translation, DTA_Color, pe, DTA_ScaleX, scale, DTA_ScaleY, scale, TAG_DONE);
		}
		return skiprequest ? -1 : 1;
	}
};

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void Logo_d(CompletionFunc completion)
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
	if (VOLUMEALL && !inputState.CheckAllInput()) jobs[job++] = { PlayVideo("logo.anm", logosound, logoframetimes), []() { S_PlaySpecialMusic(MUS_INTRO); } };
	if (!isNam()) jobs[job++] = { Create<DDRealmsScreen>(), nullptr };
	jobs[job++] = { Create<DTitleScreen>(), []() { S_PlaySound(NITEVISION_ONOFF, CHAN_AUTO, CHANF_UI); } };
	RunScreenJob(jobs, job, completion);
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

class DEpisode1End1 : public DScreenJob
{
	int bonuscnt = 0;
	int fadeoutstart = -1;

public:
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
		if (skiprequest) fadeoutstart = totalclock;

		uint64_t span = nsclock / 1'000'000;
		int light = 255;
		if (span < 255) light = span;
		else if (fadeoutstart > 0 && span > fadeoutstart - 255) light = fadeoutstart - span;
		light = clamp(light, 0, 255);
		PalEntry pe(255, light, light, light);

		twod->ClearScreen();
		DrawTexture(twod, tileGetTexture(VICTORY1, true), 0, 50, DTA_FullscreenScale, 3, DTA_VirtualWidth, 320, DTA_VirtualHeight, 200, 
			DTA_TranslationIndex, translation, DTA_Color, pe, DTA_LegacyRenderStyle, STYLE_Normal, TAG_DONE);


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
					DTA_TranslationIndex, translation, DTA_Color, pe, TAG_DONE);
			}

		// Breathe
		if (totalclock < 450 || totalclock >= 750)
		{
			if (totalclock >= 750)
			{
				DrawTexture(twod, tileGetTexture(VICTORY1 + 8, true), 86, 59, DTA_FullscreenScale, 3, DTA_VirtualWidth, 320, DTA_VirtualHeight, 200,
					DTA_TranslationIndex, translation, DTA_Color, pe, TAG_DONE);
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
						DTA_TranslationIndex, translation, DTA_Color, pe, TAG_DONE);
				}
		}
		// Only end after having faded out.
		return fadeoutstart > 0 && light == 0 ? -1 : 1;
	}
};

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

class DBlackScreen : public DScreenJob
{
	int wait;

public:
	DBlackScreen(int w) : wait(w) {}
	int Frame(uint64_t clock, bool skiprequest)
	{
		int span = int(clock / 1'000'000);
		twod->ClearScreen();
		return span < wait ? 1 : -1;
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

	FGameTexture* getTexture()
	{
		// Here we must provide a real texture, even if invalid, so that the sounds play.
		auto texid = TexMan.CheckForTexture("radlogo.anm", ETextureType::Any, FTextureManager::TEXMAN_TryAny | FTextureManager::TEXMAN_ForceLookup);
		if (texid.isValid()) return TexMan.GetGameTexture(texid);
		else return TexMan.GameByIndex(0);
	}

public:
	DEpisode3End() : DImageScreen(getTexture())
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
				waittime = clock + 1'000'000'000;
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

	void centertext(double y, const char* text)
	{
		text = GStrings(text);
		auto width = BigFont->StringWidth(text);
		DrawText(twod, BigFont, CR_RED, 160. - width/2, y - 12, text, DTA_FullscreenScale, 3, DTA_VirtualWidth, 320, DTA_VirtualHeight, 200, TAG_DONE);
	}

	int Frame(uint64_t clock, bool skiprequest)
	{
		twod->ClearScreen();
		centertext(60, "Thanks to all our");
		centertext(60 + 16, "fans for giving");
		centertext(60 + 16 + 16, "us big heads.");
		centertext(70 + 16 + 16 + 16, "Look for a Duke Nukem 3D");
		centertext(70 + 16 + 16 + 16 + 16, "sequel soon.");
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
	int64_t waittime = 0;

public:
	DEpisode5End() : DImageScreen(FIREFLYGROWEFFECT)
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

void bonussequence_d(int num, CompletionFunc completion)
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

	static const AnimSound vol42a[] =
	{
		{ 1, INTRO4_B +1 },
		{ 12, SHORT_CIRCUIT + 1 },
		{ 18, INTRO4_5 + 1 },
		{ 34, SHORT_CIRCUIT+1 },
		{ -1,-1 }
	};

	static const AnimSound vol41a[] =
	{
		{ 1, INTRO4_1+1 },
		{ 7, INTRO4_3+1 },
		{ 12, INTRO4_2+1 },
		{ 26, INTRO4_4+1 },
		{ -1,-1 }
	};

	static const AnimSound vol43a[] =
	{
		{ 10, INTRO4_6+1 },
		{ -1,-1 }
	};


	static const int framespeed_10[] = { 10, 10, 10 };
	static const int framespeed_14[] = { 14, 14, 14 };
	static const int framespeed_18[] = { 18, 18, 18 };

	Mus_Stop();
	FX_StopAllSounds();

	JobDesc jobs[10];
	int job = 0;

	switch (num)
	{
	case 0:
		jobs[job++] = { Create<DEpisode1End1>(), nullptr };
		jobs[job++] = { Create<DImageScreen>(E1ENDSCREEN), nullptr };
		break;

	case 1:
		jobs[job++] = { PlayVideo("cineov2.anm", cineov2sound, framespeed_18), []() { S_PlaySound(PIPEBOMB_EXPLODE, CHAN_AUTO, CHANF_UI); } };
		jobs[job++] = { Create<DImageScreen>(E2ENDSCREEN), nullptr };
		break;

	case 2:
		jobs[job++] = { PlayVideo("cineov3.anm", cineov3sound, framespeed_10), nullptr };
		jobs[job++] = { Create<DBlackScreen>(200), []() { FX_StopAllSounds(); } };
		jobs[job++] = { Create<DEpisode3End>(), []() { if (!PLUTOPAK) S_PlaySound(ENDSEQVOL3SND4, CHAN_AUTO, CHANF_UI); } };
		if (!PLUTOPAK) jobs[job++] = { Create<DImageScreen>(TexMan.GetGameTextureByName("DUKETEAM.ANM", false, FTextureManager::TEXMAN_ForceLookup)) };
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

	case 5:	// Episode 4 start
		S_PlaySpecialMusic(MUS_BRIEFING);
		jobs[job++] = { PlayVideo("vol41a.anm", vol41a, framespeed_10), nullptr };
		jobs[job++] = { PlayVideo("vol42a.anm", vol42a, framespeed_14), nullptr };
		jobs[job++] = { PlayVideo("vol43a.anm", vol43a, framespeed_10), nullptr };
		break;
	}
	RunScreenJob(jobs, job, completion); 
}

#if 0
CCMD(testbonus)
{
	if (argv.argc() > 1)
	{
		bonussequence_d(strtol(argv[1], nullptr, 0), nullptr);
	}
}
#endif


void dobonus(char bonusonly)
{
	short tinc,gfx_offset;
	//int i, y, xfragtotal, yfragtotal;
	short bonuscnt;

	bonuscnt = 0;

	FX_StopAllSounds();
	FX_SetReverb(0);

	if(bonusonly) goto FRAGBONUS;

FRAGBONUS:
	;
#if 0

	inputState.ClearAllInput();
	totalclock = 0; tinc = 0;
	bonuscnt = 0;

	Mus_Stop();
	FX_StopAllSounds();

	if(playerswhenstarted > 1 && ud.coop != 1 )
	{
		if(!(MusicToggle == 0 || MusicDevice == NumSoundCards))
			sound(BONUSMUSIC);

		rotatesprite(0,0,65536L,0,MENUSCREEN,16,0,2+8+16+64,0,0,xdim-1,ydim-1);
		rotatesprite(160<<16,34<<16,65536L,0,INGAMEDUKETHREEDEE,0,0,10,0,0,xdim-1,ydim-1);

#ifndef UK
		rotatesprite((260)<<16,36<<16,65536L,0,PLUTOPAKSPRITE+2,0,0,2+8,0,0,xdim-1,ydim-1);
#endif

		gametext(160,58+2,"MULTIPLAYER TOTALS",0,2+8+16);
		gametext(160,58+10,level_names[(ud.volume_number*11)+ud.last_level-1],0,2+8+16);

		gametext(160,165,"PRESS ANY KEY TO CONTINUE",0,2+8+16);


		t = 0;
		minitext(23,80,"   NAME                                           KILLS",8,2+8+16+128);
		for(i=0;i<playerswhenstarted;i++)
		{
			sprintf(tempbuf,"%-4ld",i+1);
			minitext(92+(i*23),80,tempbuf,3,2+8+16+128);
		}

		for(i=0;i<playerswhenstarted;i++)
		{
			xfragtotal = 0;
			sprintf(tempbuf,"%ld",i+1);

			minitext(30,90+t,tempbuf,0,2+8+16+128);
			minitext(38,90+t,ud.user_name[i],ps[i].palookup,2+8+16+128);

			for(y=0;y<playerswhenstarted;y++)
			{
				if(i == y)
				{
					sprintf(tempbuf,"%-4ld",ps[y].fraggedself);
					minitext(92+(y*23),90+t,tempbuf,2,2+8+16+128);
					xfragtotal -= ps[y].fraggedself;
				}
				else
				{
					sprintf(tempbuf,"%-4ld",frags[i][y]);
					minitext(92+(y*23),90+t,tempbuf,0,2+8+16+128);
					xfragtotal += frags[i][y];
				}

				if(myconnectindex == connecthead)
				{
					sprintf(tempbuf,"stats %ld killed %ld %ld\n",i+1,y+1,frags[i][y]);
					sendscore(tempbuf);
				}
			}

			sprintf(tempbuf,"%-4ld",xfragtotal);
			minitext(101+(8*23),90+t,tempbuf,2,2+8+16+128);

			t += 7;
		}

		for(y=0;y<playerswhenstarted;y++)
		{
			yfragtotal = 0;
			for(i=0;i<playerswhenstarted;i++)
			{
				if(i == y)
					yfragtotal += ps[i].fraggedself;
				yfragtotal += frags[i][y];
			}
			sprintf(tempbuf,"%-4ld",yfragtotal);
			minitext(92+(y*23),96+(8*7),tempbuf,2,2+8+16+128);
		}

		minitext(45,96+(8*7),"DEATHS",8,2+8+16+128);
		videoNextPage();

		for(t=0;t<64;t+=7)
			palto(0,0,0,63-t);

		inputState.ClearAllInput();
		while(inputState.CheckAllInput()==0) //getpackets();

		if(bonusonly || ud.multimode > 1) return;

		for(t=0;t<64;t+=7) palto(0,0,0,t);
	}
#endif

	if(bonusonly || ud.multimode > 1) return;

	switch(ud.volume_number)
	{
		case 1:
			gfx_offset = 5;
			break;
		default:
			gfx_offset = 0;
			break;
	}

	const char* lastmapname;

	if (ud.volume_number == 0 && ud.last_level == 8 && boardfilename[0])
	{
		lastmapname = strrchr(boardfilename, '\\');
		if (!lastmapname) lastmapname = strrchr(boardfilename, '/');
		if (!lastmapname) lastmapname = boardfilename;
	}
	else
	{
		lastmapname = currentLevel->name;
		if (!lastmapname || !*lastmapname) // this isn't right but it's better than no name at all
			lastmapname = currentLevel->fileName;
	}



	rotatesprite(0,0,65536L,0,BONUSSCREEN+gfx_offset,0,0,2+8+16+64+128,0,0,xdim-1,ydim-1);

	menutext_center(20-6,lastmapname);
	menutext_center(36-6,"COMPLETED");

	gametext(160,192,"PRESS ANY KEY TO CONTINUE",16,2+8+16);

	if (MusicEnabled() && mus_enabled)
		S_PlaySound(BONUSMUSIC, CHAN_AUTO, CHANF_UI);

	videoNextPage();
	inputState.ClearAllInput();
	//for(t=0;t<64;t++) palto(0,0,0,63-t);
	bonuscnt = 0;
	totalclock = 0; tinc = 0;

	while( 1 )
	{
		if(ps[myconnectindex].gm&MODE_EOL)
		{
			rotatesprite(0,0,65536L,0,BONUSSCREEN+gfx_offset,0,0,2+8+16+64+128,0,0,xdim-1,ydim-1);

			if( totalclock > (1000000000L) && totalclock < (1000000320L) )
			{
				switch( ((int)totalclock>>4)%15 )
				{
					case 0:
						if(bonuscnt == 6)
						{
							bonuscnt++;
							sound(SHOTGUN_COCK);
							switch(rand()&3)
							{
								case 0:
									sound(BONUS_SPEECH1);
									break;
								case 1:
									sound(BONUS_SPEECH2);
									break;
								case 2:
									sound(BONUS_SPEECH3);
									break;
								case 3:
									sound(BONUS_SPEECH4);
									break;
							}
						}
					case 1:
					case 4:
					case 5:
						rotatesprite(199<<16,31<<16,65536L,0,BONUSSCREEN+3+gfx_offset,0,0,2+8+16+64+128,0,0,xdim-1,ydim-1);
						break;
					case 2:
					case 3:
					   rotatesprite(199<<16,31<<16,65536L,0,BONUSSCREEN+4+gfx_offset,0,0,2+8+16+64+128,0,0,xdim-1,ydim-1);
					   break;
				}
			}
			else if( totalclock > (10240+120L) ) break;
			else
			{
				switch( ((int)totalclock>>5)&3 )
				{
					case 1:
					case 3:
						rotatesprite(199<<16,31<<16,65536L,0,BONUSSCREEN+1+gfx_offset,0,0,2+8+16+64+128,0,0,xdim-1,ydim-1);
						break;
					case 2:
						rotatesprite(199<<16,31<<16,65536L,0,BONUSSCREEN+2+gfx_offset,0,0,2+8+16+64+128,0,0,xdim-1,ydim-1);
						break;
				}
			}

			menutext_center(20-6,lastmapname);
			menutext_center(36-6,"COMPLETED");

			gametext(160,192,"PRESS ANY KEY TO CONTINUE",16,2+8+16);

			if( totalclock > (60*3) )
			{
				gametext(10,59+9,"Your Time:",0,2+8+16);
				gametext(10,69+9,"Par time:",0,2+8+16);
				if (!isNamWW2GI())
					gametext(10,78+9,"3D Realms' Time:",0,2+8+16);

				if(bonuscnt == 0)
					bonuscnt++;

				if( totalclock > (60*4) )
				{
					if(bonuscnt == 1)
					{
						bonuscnt++;
						sound(PIPEBOMB_EXPLODE);
					}
					sprintf(tempbuf,"%02ld:%02ld",
						(ps[myconnectindex].player_par/(26*60))%60,
						(ps[myconnectindex].player_par/26)%60);
					gametext((320>>2)+71,60+9,tempbuf,0,2+8+16);

					sprintf(tempbuf,"%02ld:%02ld",
						(currentLevel->parTime / (26*60))%60,
						(currentLevel->parTime / 26)%60);
					gametext((320>>2)+71,69+9,tempbuf,0,2+8+16);

					if (!isNamWW2GI())
					{
						sprintf(tempbuf, "%02ld:%02ld",
							(currentLevel->designerTime / (26 * 60)) % 60,
							(currentLevel->designerTime / 26) % 60);
							gametext((320 >> 2) + 71, 78 + 9, tempbuf, 0, 2 + 8 + 16);
					}

				}
			}
			if( totalclock > (60*6) )
			{
				gametext(10,94+9,"Enemies Killed:",0,2+8+16);
				gametext(10,99+4+9,"Enemies Left:",0,2+8+16);

				if(bonuscnt == 2)
				{
					bonuscnt++;
					sound(FLY_BY);
				}

				if( totalclock > (60*7) )
				{
					if(bonuscnt == 3)
					{
						bonuscnt++;
						sound(PIPEBOMB_EXPLODE);
					}
					sprintf(tempbuf,"%-3ld",ps[myconnectindex].actors_killed);
					gametext((320>>2)+70,93+9,tempbuf,0,2+8+16);
					if(ud.player_skill > 3 )
					{
						sprintf(tempbuf,"N/A");
						gametext((320>>2)+70,99+4+9,tempbuf,0,2+8+16);
					}
					else
					{
						if( (ps[myconnectindex].max_actors_killed-ps[myconnectindex].actors_killed) < 0 )
							sprintf(tempbuf,"%-3ld",0);
						else sprintf(tempbuf,"%-3ld",ps[myconnectindex].max_actors_killed-ps[myconnectindex].actors_killed);
						gametext((320>>2)+70,99+4+9,tempbuf,0,2+8+16);
					}
				}
			}
			if( totalclock > (60*9) )
			{
				gametext(10,120+9,"Secrets Found:",0,2+8+16);
				gametext(10,130+9,"Secrets Missed:",0,2+8+16);
				if(bonuscnt == 4) bonuscnt++;

				if( totalclock > (60*10) )
				{
					if(bonuscnt == 5)
					{
						bonuscnt++;
						sound(PIPEBOMB_EXPLODE);
					}
					sprintf(tempbuf,"%-3d",ps[myconnectindex].secret_rooms);
					gametext((320>>2)+70,120+9,tempbuf,0,2+8+16);
					if( ps[myconnectindex].secret_rooms > 0 )
						sprintf(tempbuf,"%-3d",(100*ps[myconnectindex].secret_rooms/ps[myconnectindex].max_secret_rooms));
					sprintf(tempbuf,"%-3d",ps[myconnectindex].max_secret_rooms-ps[myconnectindex].secret_rooms);
					gametext((320>>2)+70,130+9,tempbuf,0,2+8+16);
				}
			}

			if(totalclock > 10240 && totalclock < 10240+10240)
				totalclock = 1024;

			if( inputState.CheckAllInput() && totalclock > (60*2) )
			{
				if( totalclock < (60*13) )
				{
					inputState.ClearAllInput();
					totalclock = (60*13);
				}
				else if( totalclock < (1000000000L))
				   totalclock = (1000000000L);
			}
		}
		else break;
		videoNextPage();
	}
}


END_DUKE_NS
