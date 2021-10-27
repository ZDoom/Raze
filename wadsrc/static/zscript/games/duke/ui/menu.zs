//-------------------------------------------------------------------------
/*
Copyright (C) 1996, 2003 - 3D Realms Entertainment
Copyright (C) 2020-2021 Christoph Oelckers

This file is part of Raze.

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
( not much left of the original code, though... ;) )
*/
//------------------------------------------------------------------------- 

class DukeMenuDelegate : RazeMenuDelegate
{
	override int DrawCaption(String title, Font fnt, int y, bool drawit)
	{
		let font = Raze.PickBigFont();	// this ignores the passed font intentionally.
		let texid = TexMan.CheckForTexture("MENUBAR");
		let texsize = TexMan.GetScaledSize(texid);
		let fonth = font.GetGlyphHeight("A");
		if (drawit)
		{
			double scalex = (gameinfo.gameType & GAMEFLAG_RRALL)? 0.4 : 1.; 
			double width = font.StringWidth(title);
			double scaley = scalex;
			if (texid.isValid())
			{
				screen.DrawTexture(texid, false, 160, 19, DTA_FullscreenScale, FSMode_Fit320x200Top, DTA_CenterOffsetRel, true, DTA_Color, 0xff808080);
				if (texsize.X - 30 < width * scalex) scalex = (texsize.X - 30) / (width); // Squash the text if it doesn't fit.
			}			
			screen.DrawText(font, Font.CR_UNTRANSLATED, 160 - width / 2 * scalex, 19 - fonth / 2 * scaley, title, DTA_ScaleX, scalex, DTA_ScaleY, scaley, DTA_FullscreenScale, FSMode_Fit320x200Top);
		}
		double fx, fy, fw, fh;
		[fx, fy, fw, fh] = Screen.GetFullscreenRect(320, 200, FSMode_ScaleToFit43Top);
		int h = texid.isValid() && texsize.Y < 40? texsize.Y : fonth;
		return int((y+h) * fh / 200); // This must be the covered height of the header in true pixels.
	}
	
	//----------------------------------------------------------------------------
	//
	//
	//
	//----------------------------------------------------------------------------

	void DrawCursor(double x, double y, double scale, bool right)
	{
		uint mclock = MSTime() * 120 / 1000;
		uint frames = (gameinfo.gametype & GAMEFLAG_RRALL) ? 16 : 7;
		String picname;
		if (!right) picname= String.Format("SPINNINGNUKEICON%d", ((mclock >> 3) % frames));
		else picname = String.Format("SPINNINGNUKEICON%d", frames - 1 - ((frames - 1 + (mclock >> 3)) % frames));
		int light = 231 + ((Raze.bsin(mclock<<5) * 3) >> 11);
		let pe = color(255, light, light, light);
		Screen.DrawTexture(TexMan.CheckForTexture(picname), false, x, y, DTA_FullscreenScale, FSMode_Fit320x200, DTA_ScaleX, scale, DTA_ScaleY, scale, DTA_Color, pe, DTA_CenterOffsetRel, true);
	}
	
	override bool DrawSelector(ListMenuDescriptor desc)
	{
		int cursorOffset = 110;
		double cursorScale = (gameinfo.gametype & GAMEFLAG_RRALL) ? 0.2 : 1.0;
		double ymid = desc.mItems[desc.mSelectedItem].GetY() + 7;	// half height must be hardcoded or layouts will break.
		DrawCursor(160 + cursorOffset, ymid, cursorScale, false);
		DrawCursor(160 - cursorOffset, ymid, cursorScale, true);
		return true;
	}
	
	//----------------------------------------------------------------------------
	//
	// not used for any localized content.
	//
	//----------------------------------------------------------------------------

	static void shadowminitext(int xx, int yy, String t, int p)
	{
		double x = xx / 65536.;
		double y = yy / 65536.;

		Screen.DrawText(SmallFont2, Font.CR_UNTRANSLATED, x + 1, y + 1, t, DTA_FullscreenScale, FSMode_Fit320x200, DTA_Color, 0xff000000, DTA_Alpha, 0.5);
		Screen.DrawText(SmallFont2, Font.CR_NATIVEPAL, x, y, t, DTA_FullscreenScale, FSMode_Fit320x200, DTA_TranslationIndex, TRANSLATION.MakeID(Translation_Remap, p));
	}

	static void mgametextcenter(int xx, int yy, String t)
	{
		double x = xx / 65536. + 160. - SmallFont.StringWidth(t) * 0.5;
		double y = yy / 65536.;

		Screen.DrawText(SmallFont, Font.CR_UNTRANSLATED, x, y + 2, t, DTA_FullscreenScale, FSMode_Fit320x200);
	}

	
}

//----------------------------------------------------------------------------
//
// Logo
//
//----------------------------------------------------------------------------

class ListMenuItemDukeLogo : ListMenuItem
{
	const x = 160;
	
	void Init(ListMenuDescriptor desc)
	{
		Super.Init(0, 0);
	}
	
	override void Draw(bool selected, ListMenuDescriptor desc)
	{
		if (gameinfo.gametype & GAMEFLAG_RRRA)
		{
			Screen.DrawTexture(TexMan.CheckForTexture("THREEDEE"), false, x-5, 57, DTA_FullscreenScale, FSMode_Fit320x200Top, DTA_ScaleX, 0.253, DTA_ScaleY, 0.253, DTA_CenterOffsetRel, true);
		}
		else if (gameinfo.gametype & GAMEFLAG_RR)
		{
			Screen.DrawTexture(TexMan.CheckForTexture("INGAMEDUKETHREEDEE"), false, x+5, 24, DTA_FullscreenScale, FSMode_Fit320x200Top, DTA_ScaleX, 0.36, DTA_ScaleY, 0.36, DTA_CenterOffsetRel, true);
		}
		else
		{
			Screen.DrawTexture(TexMan.CheckForTexture("INGAMEDUKETHREEDEE"), false, x, 29, DTA_FullscreenScale, FSMode_Fit320x200Top, DTA_CenterOffsetRel, true);
			if (gameinfo.gametype & GAMEFLAG_PLUTOPAK)
			{
				int mclock = MSTime() * 120 / 1000;
				int light = 223 + (Raze.bsin(mclock<<4) >> 9);
				let pe = Color(255, light, light, light);
				Screen.DrawTexture(TexMan.CheckForTexture("MENUPLUTOPAKSPRITE"), false, x + 100, 36, DTA_FullscreenScale, FSMode_Fit320x200Top, DTA_Color, pe, DTA_CenterOffsetRel, true);
			}
		}
		
	}
}

//----------------------------------------------------------------------------
//
// text item
//
//----------------------------------------------------------------------------

class ListMenuItemDukeTextItem : ListMenuItemTextItem
{
	void Init(ListMenuDescriptor desc, String text, String hotkey, Name child, int param = 0)
	{
		Super.Init(desc, text, hotkey, child, param);
		mColorSelected = 0;
	}
	
	void InitDirect(double x, double y, int height, String hotkey, String text, Font font, int color, int color2, Name child, int param = 0)
	{
		Super.InitDirect(x, y, height, hotkey, text, font, color, color2, child, param);
	}
	
	override void Draw(bool selected, ListMenuDescriptor desc)
	{
		let font = Raze.PickBigFont();
		int trans = mColorSelected? Translation.MakeID(Translation_Remap, 1) : 0;
		Color pe;
		double scale = (gameinfo.gametype & GAMEFLAG_RRALL) ? 0.4 : 1.;
		let xpos = 160 - font.StringWidth(mText) * scale * 0.5;

		if (selected)
		{
			int mclock = MSTime() * 120 / 1000;
			int light = 231 + (Raze.bsin(mclock<<5) >> 9);
			pe = Color(255, light, light, light);
		}
		else
		{
			pe = Color(255, 160, 160, 160);
		}

		// Palette 0 may not use NATIVEPAL so that substitution remaps work.
		Screen.DrawText(font, trans? Font.CR_NATIVEPAL : Font.CR_UNTRANSLATED, xpos, mYpos, mText, DTA_FullscreenScale, FSMode_Fit320x200, 
			DTA_ScaleX, scale, DTA_ScaleY, scale, DTA_Color, pe, DTA_TranslationIndex, trans);
	}
	
}

//----------------------------------------------------------------------------
//
// Credits for Duke 1.3
//
//----------------------------------------------------------------------------

class ImageScrollerPageDukeCredits1 : ImageScrollerPage
{
	
	void Init(ImageScrollerDescriptor desc)
	{
		Super.Init();
	} 
	
	override void Drawer(bool selected)
	{
		int m, l;
		Screen.DrawTexture(TexMan.CheckForTexture("MENUSCREEN"), false, 160, 100, DTA_FullscreenScale, FSMode_Fit320x200, DTA_Color, 0xff808080, DTA_CenterOffset, true);
		m = (20 << 16);
		l = (33 << 16);

		DukeMenuDelegate.shadowminitext(m, l, "Original Concept", 12); l += 7 << 16;
		DukeMenuDelegate.shadowminitext(m, l, "Todd Replogle and Allen H. Blum III", 12); l += 7 << 16;
		l += 3 << 16;
		DukeMenuDelegate.shadowminitext(m, l, "Produced & Directed By", 12); l += 7 << 16;
		DukeMenuDelegate.shadowminitext(m, l, "Greg Malone", 12); l += 7 << 16;
		l += 3 << 16;
		DukeMenuDelegate.shadowminitext(m, l, "Executive Producer", 12); l += 7 << 16;
		DukeMenuDelegate.shadowminitext(m, l, "George Broussard", 12); l += 7 << 16;
		l += 3 << 16;
		DukeMenuDelegate.shadowminitext(m, l, "BUILD Engine", 12); l += 7 << 16;
		DukeMenuDelegate.shadowminitext(m, l, "Ken Silverman", 12); l += 7 << 16;
		l += 3 << 16;
		DukeMenuDelegate.shadowminitext(m, l, "Game Programming", 12); l += 7 << 16;
		DukeMenuDelegate.shadowminitext(m, l, "Todd Replogle", 12); l += 7 << 16;
		l += 3 << 16;
		DukeMenuDelegate.shadowminitext(m, l, "3D Engine/Tools/Net", 12); l += 7 << 16;
		DukeMenuDelegate.shadowminitext(m, l, "Ken Silverman", 12); l += 7 << 16;
		l += 3 << 16;
		DukeMenuDelegate.shadowminitext(m, l, "Network Layer/Setup Program", 12); l += 7 << 16;
		DukeMenuDelegate.shadowminitext(m, l, "Mark Dochtermann", 12); l += 7 << 16;
		l += 3 << 16;
		DukeMenuDelegate.shadowminitext(m, l, "Map Design", 12); l += 7 << 16;
		DukeMenuDelegate.shadowminitext(m, l, "Allen H. Blum III", 12); l += 7 << 16;
		DukeMenuDelegate.shadowminitext(m, l, "Richard Gray", 12); l += 7 << 16;

		m = (180 << 16);
		l = (33 << 16);

		DukeMenuDelegate.shadowminitext(m, l, "3D Modeling", 12); l += 7 << 16;
		DukeMenuDelegate.shadowminitext(m, l, "Chuck Jones", 12); l += 7 << 16;
		DukeMenuDelegate.shadowminitext(m, l, "Sapphire Corporation", 12); l += 7 << 16;
		l += 3 << 16;
		DukeMenuDelegate.shadowminitext(m, l, "Artwork", 12); l += 7 << 16;
		DukeMenuDelegate.shadowminitext(m, l, "Dirk Jones, Stephen Hornback", 12); l += 7 << 16;
		DukeMenuDelegate.shadowminitext(m, l, "James Storey, David Demaret", 12); l += 7 << 16;
		DukeMenuDelegate.shadowminitext(m, l, "Douglas R. Wood", 12); l += 7 << 16;
		l += 3 << 16;
		DukeMenuDelegate.shadowminitext(m, l, "Sound Engine", 12); l += 7 << 16;
		DukeMenuDelegate.shadowminitext(m, l, "Jim Dose", 12); l += 7 << 16;
		l += 3 << 16;
		DukeMenuDelegate.shadowminitext(m, l, "Sound & Music Development", 12); l += 7 << 16;
		DukeMenuDelegate.shadowminitext(m, l, "Robert Prince", 12); l += 7 << 16;
		DukeMenuDelegate.shadowminitext(m, l, "Lee Jackson", 12); l += 7 << 16;
		l += 3 << 16;
		DukeMenuDelegate.shadowminitext(m, l, "Voice Talent", 12); l += 7 << 16;
		DukeMenuDelegate.shadowminitext(m, l, "Lani Minella - Voice Producer", 12); l += 7 << 16;
		DukeMenuDelegate.shadowminitext(m, l, "Jon St. John as \"Duke Nukem\"", 12); l += 7 << 16;
		l += 3 << 16;
		DukeMenuDelegate.shadowminitext(m, l, "Graphic Design", 12); l += 7 << 16;
		DukeMenuDelegate.shadowminitext(m, l, "Packaging, Manual, Ads", 12); l += 7 << 16;
		DukeMenuDelegate.shadowminitext(m, l, "Robert M. Atkins", 12); l += 7 << 16;
		DukeMenuDelegate.shadowminitext(m, l, "Michael Hadwin", 12); l += 7 << 16;
	}
}

class ImageScrollerPageDukeCredits2 : ImageScrollerPage
{
	
	void Init(ImageScrollerDescriptor desc)
	{
		Super.Init();
	} 
	
	override void Drawer(bool selected)
	{
		int m, l;
		Screen.DrawTexture(TexMan.CheckForTexture("MENUSCREEN"), false, 160, 100, DTA_FullscreenScale, FSMode_Fit320x200, DTA_Color, 0xff808080, DTA_CenterOffset, true);

		m = (20 << 16);
		l = (33 << 16);

		DukeMenuDelegate.shadowminitext(m, l, "Special Thanks To", 12); l += 7 << 16;
		DukeMenuDelegate.shadowminitext(m, l, "Steven Blackburn, Tom Hall", 12); l += 7 << 16;
		DukeMenuDelegate.shadowminitext(m, l, "Scott Miller, Joe Siegler", 12); l += 7 << 16;
		DukeMenuDelegate.shadowminitext(m, l, "Terry Nagy, Colleen Compton", 12); l += 7 << 16;
		DukeMenuDelegate.shadowminitext(m, l, "HASH, Inc., FormGen, Inc.", 12); l += 7 << 16;
		l += 3 << 16;
		DukeMenuDelegate.shadowminitext(m, l, "The 3D Realms Beta Testers", 12); l += 7 << 16;
		l += 3 << 16;
		DukeMenuDelegate.shadowminitext(m, l, "Nathan Anderson, Wayne Benner", 12); l += 7 << 16;
		DukeMenuDelegate.shadowminitext(m, l, "Glenn Brensinger, Rob Brown", 12); l += 7 << 16;
		DukeMenuDelegate.shadowminitext(m, l, "Erik Harris, Ken Heckbert", 12); l += 7 << 16;
		DukeMenuDelegate.shadowminitext(m, l, "Terry Herrin, Greg Hively", 12); l += 7 << 16;
		DukeMenuDelegate.shadowminitext(m, l, "Hank Leukart, Eric Baker", 12); l += 7 << 16;
		DukeMenuDelegate.shadowminitext(m, l, "Jeff Rausch, Kelly Rogers", 12); l += 7 << 16;
		DukeMenuDelegate.shadowminitext(m, l, "Mike Duncan, Doug Howell", 12); l += 7 << 16;
		DukeMenuDelegate.shadowminitext(m, l, "Bill Blair", 12); l += 7 << 16;

		m = (160 << 16);
		l = (33 << 16);

		DukeMenuDelegate.shadowminitext(m, l, "Company Product Support", 12); l += 7 << 16;
		l += 3 << 16;
		DukeMenuDelegate.shadowminitext(m, l, "The following companies were cool", 12); l += 7 << 16;
		DukeMenuDelegate.shadowminitext(m, l, "enough to give us lots of stuff", 12); l += 7 << 16;
		DukeMenuDelegate.shadowminitext(m, l, "during the making of Duke Nukem 3D.", 12); l += 7 << 16;
		l += 3 << 16;
		DukeMenuDelegate.shadowminitext(m, l, "Altec Lansing Multimedia", 12); l += 7 << 16;
		DukeMenuDelegate.shadowminitext(m, l, "for tons of speakers and the", 12); l += 7 << 16;
		DukeMenuDelegate.shadowminitext(m, l, "THX-licensed sound system.", 12); l += 7 << 16;
		DukeMenuDelegate.shadowminitext(m, l, "For info call 1-800-548-0620", 12); l += 7 << 16;
		l += 3 << 16;
		DukeMenuDelegate.shadowminitext(m, l, "Creative Labs, Inc.", 12); l += 7 << 16;
		l += 3 << 16;
		DukeMenuDelegate.shadowminitext(m, l, "Thanks for the hardware, guys.", 12); l += 7 << 16;
	}
}

class ImageScrollerPageDukeCredits3 : ImageScrollerPage
{
	
	void Init(ImageScrollerDescriptor desc)
	{
		Super.Init();
	} 
	
	override void Drawer(bool selected)
	{
		let VOLUMEONE = gameinfo.gametype & GAMEFLAG_SHAREWARE;
		int m, l;
		Screen.DrawTexture(TexMan.CheckForTexture("MENUSCREEN"), false, 160, 100, DTA_FullscreenScale, FSMode_Fit320x200, DTA_Color, 0xff808080, DTA_CenterOffset, true);

		DukeMenuDelegate.mgametextcenter(0, (50 << 16), "Duke Nukem 3D is a trademark of");
		DukeMenuDelegate.mgametextcenter(0, (59 << 16), "3D Realms Entertainment");
		DukeMenuDelegate.mgametextcenter(0, (77 << 16), "Duke Nukem 3D");
		DukeMenuDelegate.mgametextcenter(0, (86 << 16), "(C) 1996 3D Realms Entertainment");

		if (VOLUMEONE)
		{
			DukeMenuDelegate.mgametextcenter(0, (106 << 16), "Please read LICENSE.DOC for shareware");
			DukeMenuDelegate.mgametextcenter(0, (115 << 16), "distribution grants and restrictions.");
		}
		DukeMenuDelegate.mgametextcenter(0, ((VOLUMEONE ? 134 : 115) << 16), "Made in Dallas, Texas USA");
	}
}

class PlayerMenu : OptionMenu
{
	override void Drawer()
	{
		// Hack: The team item is #3. This part doesn't work properly yet.
		DrawPlayerSprite((mDesc.mSelectedItem == 3));
		Super.Drawer();
	}
	
	native static void DrawPlayerSprite(int sel);
}
