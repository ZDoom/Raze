
class WH2MenuDelegate : RazeMenuDelegate
{
	override int DrawCaption(String title, Font fnt, int y, bool drawit)
	{
		let font = generic_ui? NewConsoleFont : BigFont;	// this ignores the passed font intentionally.
		let fonth = font.GetGlyphHeight("A");
		let fontscale = 1.0;
		double squash = 1.0;
		if (drawit)
		{
			int width = font.StringWidth(title) * fontscale;
			if (width > 315) 
			{
				squash = 315. / width;
				width = 315;
			}
			screen.DrawText(font, Font.CR_UNTRANSLATED, 160 - width / 2, 17 - fonth / 2, title, DTA_FullscreenScale, FSMode_Fit320x200Top, DTA_ScaleX, fontscale * squash, DTA_ScaleY, fontscale);
		}
		double fx, fy, fw, fh;
		[fx, fy, fw, fh] = Screen.GetFullscreenRect(320, 200, FSMode_ScaleToFit43Top);
		int h = fonth * 0.8;
		return int((y+h) * fh / 200); // This must be the covered height of the header in true pixels.
		return y;
	}
	
	override bool DrawSelector(ListMenuDescriptor desc)
	{
		return true;
	}
}


//=============================================================================
//
// text item
//
//=============================================================================

class ListMenuItemWH2TextItem : ListMenuItemTextItem
{
	void Init(ListMenuDescriptor desc, String text, String hotkey, Name child, int param = 0)
	{
		Super.Init(desc, text, hotkey, child, param);
		if (child == 'none') mEnabled = -1;
	}
	
	void InitDirect(double x, double y, int height, String hotkey, String text, Font font, int color, int color2, Name child, int param = 0)
	{
		Super.InitDirect(x, y, height, hotkey, text, font, color, color2, child, param);
	}
	
	override void Draw(bool selected, ListMenuDescriptor desc)
	{
		let gamefont = generic_ui ? NewSmallFont : BigFont;
		int cr = generic_ui? Font.CR_Fire : Font.CR_UNDEFINED;
		double scalex = 1.;
		int trans = 0;
		Color colr = Color(255, 255, 255, 255);
		let fontscale = 1.0;

		double length = gamefont.StringWidth(mText) * fontscale;
		double xpos = mXpos - length / 2;
		if (xpos + length > 315)
		{
			xpos = 315 - length;
		}
		if (!selectable()) cr = Font.CR_BLACK;
		else if (selected) 
		{
			int mclock = MSTime() * 120 / 1000;
			int light = 223 + (Raze.calcSinTableValue(mclock<<4) / 512.);
			colr = Color(255, light, light, light); 
			cr = Font.CR_GREEN;
		}
		Screen.DrawText(BigFont, cr, xpos, mYpos, mText, DTA_FullscreenScale, FSMode_Fit320x200, DTA_Color, colr, DTA_ScaleX, scalex * fontscale, DTA_ScaleY, fontscale);
	}
}

//=============================================================================
//
// text item
//
//=============================================================================

class ListMenuItemWH2SkillItem : ListMenuItemTextItem
{
	TextureID skull;
	void Init(ListMenuDescriptor desc, String icon, String text, String hotkey, Name child, int param = 0)
	{
		Super.Init(desc, text, hotkey, child, param);
		skull = TexMan.CheckForTexture(icon);
	}
	
	override void Draw(bool selected, ListMenuDescriptor desc)
	{
		let gamefont = generic_ui ? NewSmallFont : BigFont;
		int cr = generic_ui? Font.CR_Fire : Font.CR_UNDEFINED;
		double scalex = 1.;
		int trans = 0;
		Color colr = Color(255, 255, 255, 255);
		let fontscale = 1;

		if (!selectable()) cr = Font.CR_BLACK;
		else if (selected) 
		{
			int mclock = MSTime() * 120 / 1000;
			int light = 223 + (Raze.calcSinTableValue(mclock<<4) / 512.);
			colr = Color(255, light, light, light); 
			cr = Font.CR_GREEN;
		}
		screen.DrawTexture(skull, false, mXpos - 15, mYpos + 10, DTA_FullscreenScale, FSMode_Fit320x200Top, DTA_CenterOffsetRel, true, DTA_ScaleX, 0.6, DTA_ScaleY, 0.6, DTA_Alpha, selected? 1.0 : 0.5);
		Screen.DrawText(BigFont, cr, mXpos, mYpos, mText, DTA_FullscreenScale, FSMode_Fit320x200, DTA_Color, colr, DTA_TranslationIndex, trans, DTA_ScaleX, scalex * fontscale, DTA_ScaleY, fontscale);
	}
}

