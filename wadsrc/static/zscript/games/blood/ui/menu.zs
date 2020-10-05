
class MenuCustomizerBlood : MenuCustomize
{
	override int DrawCaption(String title, Font fnt, int y, bool drawit)
	{
		let font = generic_ui? NewConsoleFont : BigFont;	// this ignores the passed font intentionally.
		let texid = tileFiles.GetTexture(2038, true);
		let texsize = TexMan.GetScaledSize(texid);
		let fonth = fnt.GetGlyphHeight("A");
		if (drawit)
		{
			int width = font.StringWidth(title);
			if (texid.isValid())
			{
				double scalex = 1.; // Expand the box if the text is longer
				if (texsize.X - 10 < width) scalex = texsize.X / (boxwidth - 10);
				screen.DrawTexture(texid, false, 160, 20, DTA_FullscreenScale, FSMode_Fit320x200Top, DTA_CenterOffsetRel, true, DTA_ScaleX, scalex);
			}
			screen.DrawText(fnt, Font.CR_UNDEFINED, 160 - width / 2, 20 - fonth / 2, text, DTA_FullscreenScale, FSMode_Fit320x200Top);
		}
		int h = texid.isValid()? texsize.Y : fonth;
		return y + h * screen.GetHeight() * CleanYfac_1 / 200;	// option menus use Clean?fac_1 so we have to convert to that screen space.
	}
}
