
class SWMenuDelegate : RazeMenuDelegate
{
	override int DrawCaption(String title, Font fnt, int y, bool drawit)
	{
		let font = generic_ui? NewConsoleFont : BigFont;	// this ignores the passed font intentionally.
		let texid = TexMan.CheckForTexture("MENUBAR");
		let texsize = TexMan.GetScaledSize(texid);
		let fonth = font.GetGlyphHeight("A");
		let fontscale = 1.;
		if (drawit)
		{
			int width = font.StringWidth(title) * fontscale;
			double scalex = 1.; 
			if (texid.isValid())
			{
				if (texsize.X - 60 < width) 
				{
					// First start squashing the font down to 0.7x the original width.
					fontscale = (texsize.X - 60) / width;
					if (fontscale < 0.7)
					{
						// If that is not enough, extend the box.
						fontscale = 0.7;
						width *= 0.7;
						scalex = width / (texsize.X - 60);
					}
					else width *= fontscale;
				}
				screen.DrawTexture(texid, false, 160, 15, DTA_FullscreenScale, FSMode_Fit320x200Top, DTA_CenterOffsetRel, true, DTA_ScaleX, scalex);
			}
			screen.DrawText(font, Font.CR_UNTRANSLATED, 160 - width / 2, 17 - fonth / 2, title, DTA_FullscreenScale, FSMode_Fit320x200Top, DTA_ScaleX, fontscale);
		}
		double fx, fy, fw, fh;
		[fx, fy, fw, fh] = Screen.GetFullscreenRect(320, 200, FSMode_ScaleToFit43Top);
		int h = texid.isValid()? texsize.Y : fonth;
		return int((y+h) * fh / 200); // This must be the covered height of the header in true pixels.
	}
	
	override bool DrawSelector(ListMenuDescriptor desc)
	{
		let item = desc.mItems[desc.mSelectedItem];
		let x = item.GetX();
		let y = item.GetY();
		

		let tex = TexMan.CheckForTexture("YINYANG");
		
		x -= TexMan.GetSize(tex) / 4 + 2;
		y += 4;

		Screen.DrawTexture(tex, true, x, y, DTA_FullscreenScale, FSMode_Fit320x200,
			DTA_CenterOffset, true, DTA_Color, 0xfff0f0f0, DTA_ScaleX, 0.5, DTA_ScaleY, 0.5);
		return true;
	}

	//native static void PlayOrderSound();
}

//=============================================================================
//
// logo
//
//=============================================================================

class ListMenuItemSWLogo : ListMenuItem
{
	void Init(ListMenuDescriptor desc)
	{
		Super.Init(0, 0);
	}

	override void Draw(bool selected, ListMenuDescriptor desc)
	{
		screen.DrawTexture(TexMan.CheckForTexture("shadow_warrior"), false, 160, 15, DTA_FullscreenScale, FSMode_Fit320x200, DTA_CenterOffsetRel, true, DTA_Color, 0xfff0f0f0);
	}
}

//=============================================================================
//
// SW order sound
//
//=============================================================================
/*
class ListMenuItemOrderSound : ListMenuItem
{
	void Init(ListMenuDescriptor desc)
	{
		Super.Init(0, 0);
	}

	override void Drawer()
	{
		if (mEnabled) SWMenuDelegate.PlayOrderSound();
		mEnabled = 0;
	}
}
*/

//=============================================================================
//
// text item
//
//=============================================================================

class ListMenuItemSWTextItem : ListMenuItemTextItem
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
		int cr = generic_ui? Font.CR_RED : Font.CR_UNDEFINED;
		double scalex = 1.;


		// The font here is very bulky and may cause problems with localized content. Account for that by squashing the text if needed.
		int length = gamefont.StringWidth(mText);
		if (mXpos + length > 320)
		{
			scalex = (315. - mXpos) / length;
		}

		Screen.DrawText(BigFont, cr, mXpos, mYpos, mText, DTA_FullscreenScale, FSMode_Fit320x200, DTA_Color, !Selectable()? 0xff505050 : 0xffffffff, DTA_ScaleX, scalex);
	}
}

