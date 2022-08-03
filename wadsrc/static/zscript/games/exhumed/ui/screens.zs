//-------------------------------------------------------------------------
/*
Copyright (C) 2010-2019 EDuke32 developers and contributors
Copyright (C) 2019 sirlemonhead, Nuke.YKT
Copyright (C) 2020-2021 Christoph Oelckers
This file is part of Raze.
PCExhumed is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License version 2
as published by the Free Software Foundation.
This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
See the GNU General Public License for more details.
You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/
//-------------------------------------------------------------------------

struct LMFDecoder native
{
	static native bool Identify(String fn);
	static native LMFDecoder Create(String fn);
	native bool Frame(double clock);
	native TextureID GetTexture();
	native void Close();
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

class LmfPlayer : SkippableScreenJob
{
	LMFDecoder decoder;
	double nextclock;
	String fn;

	ScreenJob Init(String filename)
	{
		fn = filename;
		return self;
	}

	override void Start()
	{
		decoder = LMFDecoder.Create(fn);
	}

	//---------------------------------------------------------------------------
	//
	// 
	//
	//---------------------------------------------------------------------------

	override void Draw(double smoothratio)
	{
		double clock = (ticks + smoothratio) * 1000000000. / GameTicRate;
		if (clock >= nextclock)
		{
			if (decoder.Frame(clock))
			{
				jobstate = finished;
				return;
			}
		}

		double duration = clock * (120. / 8000000000.);
		double z = 2048 * duration;
		if (z > 65536) z = 65536;

		double angle = 1536. + 16. * duration;
		if (angle >= 2048.) angle = 0.;

		Screen.DrawTexture(decoder.getTexture(), false, 160, 100, DTA_FullscreenScale, FSMode_Fit320x200,
				DTA_CenterOffset, true, DTA_FlipY, true, DTA_ScaleX, z / 65536., DTA_ScaleY, z / 65536., DTA_Rotate, (-angle - 512) * (360. / 2048.));
	}

	override void OnDestroy()
	{
		decoder.Close();
	}
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

class LobotomyScreen : ImageScreen
{
	ScreenJob Init(String texname, int fade)
	{
		Super.InitNamed(texname, fade);
		return self;
	}

	override void OnSkip()
	{
		Exhumed.StopLocalSound();
	}

	override void Start()
	{
		Exhumed.PlayLocalSound(ExhumedSnd.kSoundJonLaugh2, 7000, false, CHANF_UI);
	}

	override void OnTick()
	{
		Super.OnTick();
		if (jobstate == finished) Exhumed.StopLocalSound();
	}
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------


class MainTitle : SkippableScreenJob
{
	String a, b;
	int mystate;
	int duration;
	int var_4;
	int esi;
	int nCount;
	int starttime;
	static const short skullDurations[] = { 6, 25, 43, 50, 68, 78, 101, 111, 134, 158, 173, 230, 600 };

	ScreenJob Init()
	{
		Super.Init(fadein);
		a = StringTable.Localize("$TXT_EX_COPYRIGHT1");
		b = StringTable.Localize("$TXT_EX_COPYRIGHT2");
		duration = skullDurations[0];
		esi = 130;
		return self;
	}

	override void Start()
	{
		Exhumed.PlayLocalSound(59, 0, true, CHANF_UI);
		Exhumed.playCDtrack(19, true);
	}

	override void OnTick()
	{
		int ticker = ticks * 120 / GameTicRate;
		if (ticks > 1 && mystate == 0 && !Exhumed.LocalSoundPlaying())
		{
			if (random(0, 15))
				Exhumed.PlayLocalSound(ExhumedSnd.kSoundJonLaugh2, 0, false, CHANF_UI);
			else
				Exhumed.PlayLocalSound(61, 0, false, CHANF_UI);
			mystate = 1;
			starttime = ticker;
		}
		if (mystate == 1)
		{
			if (ticker > duration)
			{
				nCount++;
				if (nCount > 12)
				{
					jobstate = finished;
					return;
				}
				duration = starttime + skullDurations[nCount];
				var_4 = var_4 == 0;
			}
		}
	}

	override void Draw(double sr)
	{
		Exhumed.DrawPlasma();
		Exhumed.DrawRel("SkullHead", 160, 100);
		if (mystate == 0)
		{
			Exhumed.DrawRel("SkullJaw", 161, 130);
		}
		else
		{
			int nStringWidth = SmallFont.StringWidth(a);
			Screen.DrawText(SmallFont, Font.CR_UNTRANSLATED, 160 - nStringWidth / 2, 200 - 24, a, DTA_FullscreenScale, FSMode_Fit320x200);
			nStringWidth = SmallFont.StringWidth(b);
			Screen.DrawText(SmallFont, Font.CR_UNTRANSLATED, 160 - nStringWidth / 2, 200 - 16, b, DTA_FullscreenScale, FSMode_Fit320x200);


			String nTile = "SkullJaw";

			if (var_4)
			{
				if (esi >= 135) nTile = "SkullJaw2";
				else esi += 5;
			}
			else if (esi <= 130) esi = 130;
			else esi -= 2;

			int y;

			if (nTile == "SkullJaw2")
			{
				y = 131;
			}
			else
			{
				y = esi;
				if (y > 135) y = 135;
			}

			Exhumed.DrawRel(nTile, 161, y);
		}
	}
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

class MapScreen : ScreenJob
{
	static const int MapLevelOffsets[] = { 0, 50, 10, 20, 0, 45, -20, 20, 5, 0, -10, 10, 30, -20, 0, 20, 0, 0, 0, 0 };
	static const int MapPlaqueX[] = { 100, 230, 180, 10,  210, 10, 10, 140, 30, 200, 145, 80, 15, 220, 190, 20, 220, 20, 200, 20 };
	static const int MapPlaqueY[] = { 170, 10, 125, 95, 160, 110, 50, 0, 20, 150, 170, 80, 0, 35, 40, 130, 160, 10, 10, 10 };
	static const int MapPlaqueTextX[] = { 18, 18, 18, 18, 18, 18, 18, 18, 18, 20, 18, 18, 18, 18, 18, 19, 18, 18, 18, 19 };
	static const int MapPlaqueTextY[] = { 6, 6, 6, 6, 6, 6, 6, 6, 6, 4, 6, 6, 5, 6, 6, 6, 6, 6, 5, 4 };

	static const int FireTilesX[] = { 0, 0, 0, 0, 1, 1, 0, 1, 1, 1, 1, 1 };
	static const int FireTilesY[] = { 3, 0, 3, 0, 0, 0, 1, 1, 2, 0, 2, 0 };

	static const int MapLevelFires[] = {
		3,   0, 107, 95 ,  1, 58,  140 ,  2, 28,   38  ,
		3,   2, 240,  0 ,  0, 237,  32 ,  1, 200,  30  ,
		2,   2, 250, 57 ,  0, 250,  43 ,  2, 200,  70  ,
		2,   1, 82,  59 ,  2, 84,   16 ,  0, 10,   95  ,
		2,   2, 237, 50 ,  1, 215,  42 ,  1, 210,  50  ,
		3,   0, 40,   7 ,  1, 75,    6 ,  2, 100,  10  ,
		3,   0, 58,  61 ,  1, 85,   80 ,  2, 111,  63  ,
		3,   0, 260, 65 ,  1, 228,   0 ,  2, 259,  15  ,
		2,   0, 81,  38 ,  2, 58,   38 ,  2, 30,   20  ,
		3,   0, 259, 49 ,  1, 248,  76 ,  2, 290,  65  ,
		3,   2, 227, 66 ,  0, 224,  98 ,  1, 277,  30  ,
		2,   0, 100, 10 ,  2, 48,   76 ,  2, 80,   80  ,
		3,   0, 17,   2 ,  1, 29,   49 ,  2, 53,   28  ,
		3,   0, 266, 42 ,  1, 283,  99 ,  2, 243, 108  ,
		2,   0, 238, 19 ,  2, 240,  92 ,  2, 190,  40  ,
		2,   0, 27,   0 ,  1, 70,   40 ,  0, 20,  130  ,
		3,   0, 275, 65 ,  1, 235,   8 ,  2, 274,   6  ,
		3,   0, 75,  45 ,  1, 152, 105 ,  2, 24,   68  ,
		3,   0, 290, 25 ,  1, 225,  63 ,  2, 260, 110  ,
		0,   1, 20,  10 ,  1, 20,   10 ,  1, 20,   10  
	};

	const FIRE_SIZE = 10;
	const FIRE_TYPE = 1;
	const FIRE_XOFS = 2;
	const FIRE_YOFS = 3;
	const FIRE_ELEMENT_SIZE = 3;

	int x;
	int delta;
	int nIdleSeconds;

	int curYPos, destYPos;
	int nLevel, nLevelNew, nLevelBest;

	native static void SetNextLevel(int num);

	//---------------------------------------------------------------------------
	//
	//
	//
	//---------------------------------------------------------------------------

	ScreenJob Init(int oldlevel, int newlevel, int maxlevel)
	{
		Super.Init(fadein|fadeout);
		nLevel = oldlevel - 1;
		nLevelNew = newlevel - 1;
		nLevelBest = min(maxlevel, 19) - 1;
		curYPos = MapLevelOffsets[nLevel] + (200 * (nLevel / 2));
		destYPos = MapLevelOffsets[nLevelNew] + (200 * (nLevelNew / 2));
		if (curYPos < destYPos) delta = 2;
		else if (curYPos > destYPos) delta = -2;
		// Trim smoke in widescreen
		/*
		vec2_t mapwinxy1 = windowxy1, mapwinxy2 = windowxy2;
		int32_t width = mapwinxy2.x - mapwinxy1.x + 1, height = mapwinxy2.y - mapwinxy1.y + 1;
		if (3 * width > 4 * height)
		{
			mapwinxy1.x += (width - 4 * height / 3) / 2;
			mapwinxy2.x -= (width - 4 * height / 3) / 2;
		}
		*/
	return self;
	}

	//---------------------------------------------------------------------------
	//
	//
	//
	//---------------------------------------------------------------------------

	override bool OnEvent(InputEvent ev)
	{
		if (ev.type == InputEvent.Type_KeyDown)
		{
			int key = ev.KeyScan;
			let binding = Bindings.GetBinding(key);
			if (key == InputEvent.KEY_UPARROW || key == InputEvent.KEY_PAD_DPAD_UP || key == InputEvent.Key_kpad_8 || binding ~== "+move_forward")
			{
				if (curYPos == destYPos && nLevelNew <= nLevelBest)
				{
					nLevelNew++;
					SetNextLevel(nLevelNew + 1);
					destYPos = MapLevelOffsets[nLevelNew] + (200 * (nLevelNew / 2));

					if (curYPos <= destYPos) delta = 2;
					else delta = -2;
					nIdleSeconds = 0;
				}
				return true;
			}

			if (key == InputEvent.KEY_DOWNARROW || key == InputEvent.KEY_PAD_DPAD_DOWN || key == InputEvent.Key_kpad_2 || binding ~== "+move_backward")
			{
				if (curYPos == destYPos && nLevelNew > 0)
				{
					nLevelNew--;
					SetNextLevel(nLevelNew + 1);
					destYPos = MapLevelOffsets[nLevelNew] + (200 * (nLevelNew / 2));

					if (curYPos <= destYPos) delta = 2;
					else delta = -2;
					nIdleSeconds = 0;
				}
				return true;
			}
			if (!System.specialKeyEvent(ev)) jobstate = skipped;
			return true;
		}
		return false;
	}

	//---------------------------------------------------------------------------
	//
	//
	//
	//---------------------------------------------------------------------------

	override void OnTick() 
	{
		if (curYPos != destYPos)
		{
			// scroll the map every couple of ms
			curYPos += delta;

			if ((curYPos > destYPos && delta > 0) || (curYPos < destYPos && delta < 0))
				curYPos = destYPos;

			nIdleSeconds = 0;
		}
		else nIdleSeconds++;
		if (nIdleSeconds > 300) jobstate = finished;
	}

	//---------------------------------------------------------------------------
	//
	//
	//
	//---------------------------------------------------------------------------

	override void Draw(double smoothratio)
	{
		int currentclock = int((ticks + smoothratio) * 120 / GameTicRate);
		int myCurYPos = curYPos == destYPos? curYPos : curYPos - int((1. - smoothratio) * delta);
		int tileY = myCurYPos;

		// Draw the background screens
		for (int i = 0; i < 10; i++)
		{
			let tex = String.Format("MapBG%02d", i+1);
			Exhumed.DrawAbs(tex, x, tileY);
			tileY -= 200;
		}

		// for each level - drawing the 'level completed' on-fire smoke markers
		for (int i = 0; i < 20; i++)
		{
			int screenY = (i >> 1) * -200;

			if (nLevelBest >= i) // check if the player has finished this level
			{
				for (int j = 0; j < MapLevelFires[i * FIRE_SIZE]; j++)
				{
					int nFireFrame = ((currentclock >> 4) & 3);
					int elem = i * FIRE_SIZE + FIRE_ELEMENT_SIZE * j;
					int nFireType = MapLevelFires[elem + FIRE_TYPE];
					int x = MapLevelFires[elem + FIRE_XOFS];
					int y = MapLevelFires[elem + FIRE_YOFS];

					String nTile = String.Format("MAPFIRE_%d%d", nFireType+1, nFireFrame+1);
					int smokeX = x + FireTilesX[nFireType*3 + nFireFrame];
					int smokeY = y + FireTilesY[nFireType*3 + nFireFrame] + myCurYPos + screenY;

					// Use rotatesprite to trim smoke in widescreen
					Exhumed.DrawAbs(nTile, smokeX, smokeY);
					// Todo: mask out the sides of the screen if the background is not widescreen.
				}
			}

			int t = (((currentclock & 16) >> 4));

			String nTile = String.Format("MapPlaque%d_%02d", t+1, i+1);

			int nameX = mapPlaqueX[i];
			int nameY = mapPlaqueY[i] + myCurYPos + screenY;

			// Draw level name plaque
			Exhumed.DrawAbs(nTile, nameX, nameY);

			int shade = 96;

			if (nLevelNew == i)
			{
				shade = (Raze.bsin(16 * currentclock) + 31) >> 8;
			}
			else if (nLevelBest >= i)
			{
				shade = 31;
			}

			int textY = nameY + MapPlaqueTextY[i];
			int textX = nameX + MapPlaqueTextX[i];
			nTile = String.Format("MapPlaqueText_%02d", i+1);

			// draw the text, alternating between red and black
			Exhumed.DrawAbs(nTile, textX, textY, shade);
		}
	}

}

//---------------------------------------------------------------------------
//
// cinema (this has been stripped off all game logic that was still in here)
//
//---------------------------------------------------------------------------

class Cinema : SkippableScreenJob
{
	TextOverlay textov;
	TextureID cinematile;
	int currentCinemaPalette;
	int cdtrack;
	int palette;
	bool done;

	ScreenJob Init(String bgTexture, String text, int pal, int cdtrk)
	{
		Super.Init(fadein|fadeout);
		cinematile = TexMan.CheckForTexture(bgTexture, TexMan.Type_Any);
		textov = new("TextOverlay");
		palette =  Translation.MakeID(Translation_BasePalette, pal);
		textov.Init(text, Font.CR_NATIVEPAL, palette);
		cdtrack = cdtrk;
		return self;
	}

	override void Start()
	{
		System.StopAllSounds();
		if (cdtrack != -1)
		{
			Exhumed.playCDtrack(cdtrack+2, false);
		}
	}

	override void OnTick()
	{
		if (done) jobstate = finished;
	}

	override void Draw(double smoothratio)
	{
		Screen.DrawTexture(cinematile, false, 0, 0, DTA_FullscreenEx, FSMode_ScaleToFit43, DTA_TranslationIndex, palette);
		textov.DisplayText();
		done = textov.ScrollText((ticks + smoothratio) * (120. / GameTicRate));
	}
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

class LastLevelCinema : ScreenJob
{
	int var_24;
	int var_28;

	int ebp;
	int phase;
	int nextclock;
	uint nStringTypeOn, nCharTypeOn;
	int screencnt;
	bool skiprequest;

	BrokenLines screentext;
	Font printFont;
	TextureID tex;

	ScreenJob Init()
	{
		Super.Init(fadein | fadeout);
		var_24 = 16;
		var_28 = 12;
		nextclock = 4;
		let p = StringTable.Localize("REQUIRED_CHARACTERS", false);
		if (p == "REQUIRED_CHARACTERS") printFont = SmallFont2;
		else printFont = ConFont;
		return self;
	}

	native static TextureID DoStatic(int a, int b);
	native static TextureID UndoStatic();

	void Phase1()
	{
		if (var_24 >= 116)
		{
			if (var_28 < 192)
				var_28 += 20;
		}
		else
		{
			var_24 += 20;
		}

		tex = DoStatic(var_28, var_24);
	}

	bool InitPhase2()
	{
		let label = StringTable.Localize(String.Format("$TXT_EX_LASTLEVEL%d", screencnt + 1));
		screentext = printFont.BreakLines(label, 320);
		if (screentext.Count() == 0) return false;

		nStringTypeOn = 0;
		nCharTypeOn = 0;

		ebp = screentext.Count() * 4;    // half height of the entire text
		ebp = 81 - ebp;                 // offset from the screen's center.
		tex = UndoStatic();
		return true;
	}

	bool Phase3()
	{
		tex = DoStatic(var_28, var_24);

		if (var_28 > 20) 
		{
			var_28 -= 20;
			return true;
		}

		if (var_24 > 20) 
		{
			var_24 -= 20;
			return true;
		}
		return false;
	}

	void DisplayPhase2()
	{
		int yy = ebp;

		// for international content, use the generic 8x8 font. The original one is too small for expansion.
		if (printFont == ConFont)
		{
			yy *= 2;
			for (uint i = 0; i < nStringTypeOn; i++, yy += 10) Screen.DrawText(ConFont, Font.CR_GREEN, 140, yy, screentext.StringAt(i), DTA_FullscreenScale, FSMode_Fit640x400);
			Screen.DrawText(ConFont, Font.CR_GREEN, 140, yy, screentext.StringAt(nStringTypeOn), DTA_FullscreenScale, FSMode_Fit640x400, DTA_TextLen, nCharTypeOn);
		}
		else
		{
			for (uint i = 0; i < nStringTypeOn; i++, yy += 8) Screen.DrawText(SmallFont2, Font.CR_UNTRANSLATED, 70, yy, screentext.StringAt(i), DTA_FullscreenScale, FSMode_Fit320x200);
			Screen.DrawText(SmallFont2, Font.CR_UNTRANSLATED, 70, yy, screentext.StringAt(nStringTypeOn), DTA_FullscreenScale, FSMode_Fit320x200, DTA_TextLen, nCharTypeOn);
		}
	}

	override bool OnEvent(InputEvent ev)
	{
		if (ev.type == InputEvent.Type_KeyDown && !System.specialKeyEvent(ev)) skiprequest = true;
		return true;
	}

	override void Start()
	{
		Exhumed.PlayLocalSound(ExhumedSnd.kSound75, 0, false, CHANF_UI);
		phase = 1;
	}

	override void OnTick()
	{
		switch (phase)
		{
		case 1:
			Phase1();
			if (skiprequest || ticks >= nextclock)
			{
				InitPhase2();
				phase = 2;
				skiprequest = false;
			}
			break;

		case 2:
		{
			let text = screenText.StringAt(nStringTypeOn);
			int chr;
			[chr,nCharTypeOn] = text.GetNextCodePoint(nCharTypeOn);

			if (chr == 0)
			{
				nCharTypeOn = 0;
				nStringTypeOn++;
				if (nStringTypeOn >= uint(screentext.Count()))
				{
					nextclock = (GameTicRate * (screentext.Count() + 2)) + ticks;
					phase = 3;
				}

			}
			else 
			{
				nCharTypeOn++;
				if (chr != 32) Exhumed.PlayLocalSound(ExhumedSnd.kSound71, 0, false, CHANF_UI);
			}

			if (skiprequest)
			{
				nextclock = (GameTicRate * (screentext.Count() + 2)) + ticks;
				phase = 4;
			}
			break;
		}
		case 3:
			if (ticks >= nextclock || skiprequest)
			{
				Exhumed.PlayLocalSound(ExhumedSnd.kSound75, 0, false, CHANF_UI);
				phase = 4;
				nextclock = ticks + 60;
				skiprequest = false;
			}

		case 4:
			if (ticks >= nextclock)
			{
				skiprequest |= !Phase3();
			}
			if (skiprequest)
			{
				// Go to the next text page.
				if (screencnt != 2)
				{
					screencnt++;
					nextclock = ticks + 60;
					skiprequest = 0;
					phase = 1;
				}
				else jobstate = finished;
			}

			if (skiprequest)
			{
				jobstate = finished;
			}
		}
	}

	override void Draw(double sm)
	{
		Screen.DrawTexture(tex, false, 0, 0, DTA_FullscreenEx, FSMode_ScaleToFit43);
		if (phase == 2 || phase == 3) DisplayPhase2();
	}

}

//---------------------------------------------------------------------------
//
// Credits roll
//
//---------------------------------------------------------------------------

class ExCredits : ScreenJob
{
	Array<String> credits;
	Array<String> pagelines;
	int page;
	int pagetime;
	bool skiprequest;

	ScreenJob Init()
	{
		Super.Init();
		String text;
		int lump = Wads.CheckNumForFullName("credits.txt");
		if (lump > -1) text = Wads.ReadLump(lump);
		text.Substitute("\r", "");
		text.Split(credits, "\n\n");
		return self;
	}

	override bool OnEvent(InputEvent ev)
	{
		if (ev.type == InputEvent.Type_KeyDown && !System.specialKeyEvent(ev)) skiprequest = true;
		return true;
	}

	override void Start()
	{
		if (credits.Size() == 0)
		{
			jobstate = finished;
			return;
		}
		Exhumed.playCDtrack(19, false);
		pagetime = 0;
		page = -1;
	}

	override void OnTick()
	{
		if (ticks >= pagetime || skiprequest)
		{
			page++;
			pagelines.Clear();
			if (page < credits.Size())
				credits[page].Split(pagelines, "\n");
			else
			{
				if (skiprequest || !musplaying.handle)
				{
					jobstate = finished;
					return;
				}
			}
			pagetime = ticks + 90; // 
		}
	}

	override void Draw(double smoothratio)
	{
		int y = 100 - ((10 * (pagelines.Size() - 1)) / 2);

		int ptime = clamp((pagetime - ticks - smoothratio) * 1000 / GameTicRate, 0, 2000); // in milliseconds
		int light;

		if (ptime < 255) light = ptime;
		else if (ptime > 2000 - 255) light = 2000 - ptime;
		else light = 255;

		let colr = Color(255, light, light, light);

		for (int i = 0; i < pagelines.Size(); i++)
		{
			int nStringWidth = SmallFont.StringWidth(pagelines[i]);
			Screen.DrawText(SmallFont, Font.CR_UNTRANSLATED, 160 - nStringWidth / 2, y, pagelines[i], DTA_FullscreenScale, FSMode_Fit320x200, DTA_Color, colr);
			y += 10;
		}
	}
}

class ExhumedCutscenes ui
{
	//---------------------------------------------------------------------------
	//
	//
	//
	//---------------------------------------------------------------------------

	static void BuildIntro(ScreenJobRunner runner)
	{
		let logo = (gameinfo.gameType & GAMEFLAG_EXHUMED) ? "TileBMGLogo" : "TilePIELogo";
		runner.Append(ImageScreen.CreateNamed(logo, ScreenJob.fadein | ScreenJob.fadeout));
		runner.Append(new("LobotomyScreen").Init("LobotomyLogo", ScreenJob.fadein | ScreenJob.fadeout));
		if (LMFDecoder.Identify("book.mov")) runner.Append(new("LMFPlayer").Init("book.mov"));
		else runner.Append(MoviePlayerJob.Create("book.mov", 0));
		runner.Append(new("MainTitle").Init());
	}

	//---------------------------------------------------------------------------
	//
	//
	//
	//---------------------------------------------------------------------------

	static void BuildMap(ScreenJobRunner runner, MapRecord frommap, SummaryInfo info, MapRecord tomap)
	{
		// This is only defined for the regular levels.
		int frommapnum = frommap == null? 1 : frommap.levelNumber;
		if (fromMapnum < 1 || fromMapNum > 20 || tomap == null || tomap.levelNumber < 1 || tomap.levelNumber > 20) return;

		// hijack the super secret info in the summary info to convey the max. map because we won't need that field for its real purpose.
		runner.Append(new("MapScreen").Init(fromMapNum, toMap.levelNumber, info.supersecrets));
	}

	//---------------------------------------------------------------------------
	//
	// This removes all the insanity the original setup had with these.
	// Simplicity rules!
	//
	//---------------------------------------------------------------------------

	static void BuildCinemaBefore5(ScreenJobRunner runner)
	{
		runner.Append(new("Cinema").Init("TileCinema5", "$TXT_EX_CINEMA2", 3, 2));
	}

	static void BuildCinemaAfter10(ScreenJobRunner runner)
	{
		runner.Append(new("Cinema").Init("TileCinema10", "$TXT_EX_CINEMA4", 5, 3));
	}

	static void BuildCinemaBefore11(ScreenJobRunner runner)
	{
		runner.Append(new("Cinema").Init("TileCinema11", "$TXT_EX_CINEMA3", 1, 4));
	}

	static void BuildCinemaAfter15(ScreenJobRunner runner)
	{
		runner.Append(new("Cinema").Init("TileCinema15", "$TXT_EX_CINEMA6", 7, 6));
	}

	static void BuildCinemaBefore20(ScreenJobRunner runner)
	{
		runner.Append(new("LastLevelCinema").Init());
	}

	static void BuildCinemaAfter20(ScreenJobRunner runner)
	{
		runner.Append(new("Cinema").Init("TileCinema20", "$TXT_EX_CINEMA8", 6, 8));
		runner.Append(new("ExCredits").Init());
	}

	static void BuildCinemaLose(ScreenJobRunner runner)
	{
		runner.Append(new("Cinema").Init("TileCinemaLose", "$TXT_EX_CINEMA7", 4, 7));
	}


	//---------------------------------------------------------------------------
	//
	// player died
	//
	//---------------------------------------------------------------------------

	static void BuildGameOverScene(ScreenJobRunner runner)
	{
		System.StopMusic();
		Exhumed.PlayLocalSound(ExhumedSnd.kSoundJonLaugh2, 0, false, CHANF_UI);
		runner.Append(ImageScreen.CreateNamed("Gameover", ScreenJob.fadein | ScreenJob.fadeout, 0x7fffffff, Translation.MakeID(Translation_BasePalette, 16)));
	}

}
