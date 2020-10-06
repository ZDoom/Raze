
//-------------------------------------------------------------------------------------------
//
// Caption drawer
//
//-------------------------------------------------------------------------------------------

class MenuCustomizerBlood : MenuCustomize
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
			if (texid.isValid())
			{
				double scalex = 1.; // Expand the box if the text is longer
				if (texsize.X - 10 < width) scalex = width / (texsize.X - 10);
				screen.DrawTexture(texid, false, 160, 20, DTA_FullscreenScale, FSMode_Fit320x200Top, DTA_CenterOffsetRel, true, DTA_ScaleX, scalex);
			}
			screen.DrawText(font, Font.CR_UNDEFINED, 160 - width / 2, 20 - fonth / 2, title, DTA_FullscreenScale, FSMode_Fit320x200Top);
		}
		double fx, fy, fw, fh;
		[fx, fy, fw, fh] = Screen.GetFullscreenRect(320, 200, FSMode_ScaleToFit43Top);
		int h = texid.isValid()? texsize.Y : fonth;
		return int((y+h) * fh / 200); // This must be the covered height of the header in true pixels.
	}
	
	override bool DrawSelector(ListMenuDescriptor desc)
	{
		return true;	// do not draw any selector.
	}
	
}

//-------------------------------------------------------------------------------------------
//
// The dripping blood - partially native.
//
//-------------------------------------------------------------------------------------------

class ListMenuItemBloodDripDrawer : ListMenuItem
{
	void Init(ListMenuDescriptor desc)
	{
		Super.Init(0, 0);
	}

	native override void Draw(bool selected, ListMenuDescriptor desc);
}