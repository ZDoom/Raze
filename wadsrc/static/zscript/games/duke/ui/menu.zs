
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
}
