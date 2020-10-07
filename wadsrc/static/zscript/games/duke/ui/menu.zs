
class DukeMenuDelegate : RazeMenuDelegate
{
	override int DrawCaption(String title, Font fnt, int y, bool drawit)
	{
		let font = generic_ui? NewConsoleFont : BigFont;	// this ignores the passed font intentionally.
		let texid = TexMan.CheckForTexture("MENUBAR");
		let texsize = TexMan.GetScaledSize(texid);
		let fonth = font.GetGlyphHeight("A");
		if (drawit)
		{
			int width = font.StringWidth(title);
			double scalex = (gameinfo.gameType & GAMEFLAG_RRALL)? 0.4 : 1.; 
			double scaley = scalex;
			if (texid.isValid())
			{
				screen.DrawTexture(texid, false, 160, 19, DTA_FullscreenScale, FSMode_Fit320x200Top, DTA_CenterOffsetRel, true, DTA_Color, 0xff808080);
				if (texsize.X - 30 < width) scalex = (texsize.X - 30) / width; // Squash the text if it doesn't fit.
			}			
			screen.DrawText(font, Font.CR_UNTRANSLATED, 160 - width / 2 * scalex, 18 - fonth / 2 * scaley, title, DTA_ScaleX, scalex, DTA_ScaleY, scaley, DTA_FullscreenScale, FSMode_Fit320x200Top);
		}
		double fx, fy, fw, fh;
		[fx, fy, fw, fh] = Screen.GetFullscreenRect(320, 200, FSMode_ScaleToFit43Top);
		int h = texid.isValid()? texsize.Y : fonth;
		return int((y+h) * fh / 200); // This must be the covered height of the header in true pixels.
	}
	
	//----------------------------------------------------------------------------
	//
	//
	//
	//----------------------------------------------------------------------------

	void DrawCursor(double x, double y, double scale, bool right)
	{
		int mclock = MSTime() * 120 / 1000;
		int frames = (gameinfo.gametype & GAMEFLAG_RR) ? 16 : 7;
		String picname;
		if (!right) picname= String.Format("SPINNINGNUKEICON%d", ((mclock >> 3) % frames));
		else picname = String.Format("SPINNINGNUKEICON%d", frames - 1 - ((frames - 1 + (mclock >> 3)) % frames));
		int light = 231 + (Build.calcSinTableValue(mclock<<5) / 768.);
		let pe = color(255, light, light, light);
		Screen.DrawTexture(TexMan.CheckForTexture(picname), false, x, y, DTA_FullscreenScale, FSMode_Fit320x200, DTA_ScaleX, scale, DTA_ScaleY, scale, DTA_Color, pe, DTA_CenterOffsetRel, true);
	}
	
	override bool DrawSelector(ListMenuDescriptor desc)
	{
		int cursorOffset = 110;
		double cursorScale = (gameinfo.gametype & GAMEFLAG_RR) ? 0.2 : 1.0;
		double ymid = desc.mItems[desc.mSelectedItem].GetY() + 7;	// half height must be hardcoded or layouts will break.
		DrawCursor(160 + cursorOffset, ymid, cursorScale, false);
		DrawCursor(169 - cursorOffset, ymid, cursorScale, true);
		return true;
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
				int light = 223 + (Build.calcSinTableValue(mclock<<4) / 512.);
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
		int trans = mColorSelected? Translation.MakeID(Translation_Remap, 1) : 0; 
		Color pe;
		double scale = (gameinfo.gametype & GAMEFLAG_RR) ? 0.4 : 1.;
		let xpos = mXpos - BigFont.StringWidth(mText) * scale * 0.5;

		if (selected)
		{
			int mclock = MSTime() * 120 / 1000;
			int light = 231 + (Build.calcSinTableValue(mclock<<5) / 512.);
			pe = Color(255, light, light, light);
		}
		else
		{
			pe = Color(255, 160, 160, 160);
		}

		Screen.DrawText(BigFont, Font.CR_UNDEFINED, xpos, mYpos, mText, DTA_FullscreenScale, FSMode_Fit320x200, DTA_ScaleX, scale, DTA_ScaleY, scale, DTA_Color, pe, DTA_TranslationIndex, trans);
	}
	
}
