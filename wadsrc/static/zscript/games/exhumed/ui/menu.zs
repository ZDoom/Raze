
class ExhumedMenuDelegate : RazeMenuDelegate
{
	double zoomsize;	// this is the only persistent place where it can be conveniently stored.
	
	override int DrawCaption(String title, Font fnt, int y, bool drawit)
	{
		let font = Raze.PickBigFont();
		let cr = generic_ui ? Font.CR_FIRE : Font.CR_UNTRANSLATED;	// this ignores the passed font intentionally.
		let texid = TexMan.CheckForTexture("MENUBLANK");
		let texsize = TexMan.GetScaledSize(texid);
		let fonth = font.GetGlyphHeight("A");
		if (drawit)
		{
			int width = font.StringWidth(title);
			if (texid.isValid())
			{
				double scalex = 1.; // Expand the box if the text is longer
				if (texsize.X - 18 < width) scalex = width / (texsize.X - 18);
				screen.DrawTexture(texid, false, 160, 20, DTA_FullscreenScale, FSMode_Fit320x200Top, DTA_CenterOffset, true, DTA_ScaleX, scalex);
			}
			screen.DrawText(font, cr, 160 - width / 2, 20 - fonth / 2, title, DTA_FullscreenScale, FSMode_Fit320x200Top);
		}
		double fx, fy, fw, fh;
		[fx, fy, fw, fh] = Screen.GetFullscreenRect(320, 200, FSMode_ScaleToFit43Top);
		int h = texid.isValid()? texsize.Y : fonth;
		return int((y+h) * fh / 200); // This must be the covered height of the header in true pixels.
	}

	override bool DrawSelector(ListMenuDescriptor desc)
	{
		double y = desc.mItems[desc.mSelectedItem].GetY();
		let tex = TexMan.CheckForTexture("MENUCURSORTILE");
		screen.DrawTexture(tex, false, 37, y - 12, DTA_FullscreenScale, FSMode_Fit320x200, DTA_TopLeft, true);
		screen.DrawTexture(tex, false, 232, y - 12, DTA_FullscreenScale, FSMode_Fit320x200, DTA_TopLeft, true, DTA_FlipX, true);
		return true;
	}
}

//----------------------------------------------------------------------------
//
// 
//
//----------------------------------------------------------------------------

class ListMenuItemExhumedPlasma : ListMenuItem
{
	void Init(ListMenuDescriptor desc)
	{
		Super.Init(0, 0);
	}

	override void Draw(bool selected, ListMenuDescriptor desc)
	{
		Exhumed.DrawPlasma();
	}
}

class ListMenuItemExhumedLogo : ListMenuItem
{
	void Init(ListMenuDescriptor desc)
	{
		Super.Init(0, 0);
	}

	native override void Draw(bool selected, ListMenuDescriptor desc);
}

//----------------------------------------------------------------------------
//
// Menu related game interface functions
//
//----------------------------------------------------------------------------

class ListMenuItemExhumedTextItem : ListMenuItemTextItem
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
		let myfont = Raze.PickBigFont();
		let cr = generic_ui ? Font.CR_FIRE : Font.CR_UNTRANSLATED;	// this ignores the passed myfont intentionally.
		let tex = TexMan.CheckForTexture("MENUBLANK");
		let texsize = TexMan.GetScaledSize(tex);
		let fonth = myfont.GetGlyphHeight("A");
		int width = myfont.StringWidth(mText);
		let delegate = ExhumedMenuDelegate(menuDelegate);
		let zoom = delegate ? delegate.zoomsize : 1.;


		let v = TexMan.GetScaledSize(tex);
		double y = mYpos + v.y / 2;

		int shade;
		if (selected) shade = Raze.bsin(MSTime() * 16 * 120 / 1000) >> 9;
		else if (Selectable()) shade = 0;
		else shade = 25;
		let color = Raze.shadeToLight(shade);

		double scalex = 1.; // Squash the text if it is too wide. Due to design limitations we cannot expand the box here. :(
		if (texsize.X - 18 < width)
		{
			scalex = (texsize.X - 18) / width;
			width = (texsize.X - 18);
		}

		screen.DrawTexture(tex, false, 160, y, DTA_FullscreenScale, FSMode_Fit320x200, DTA_CenterOffset, true, DTA_ScaleX, scalex, DTA_Color, color, DTA_ScaleX, zoom, DTA_ScaleY, zoom);
		screen.DrawText(myfont, cr, 160 - zoom * width / 2, y - zoom * fonth / 2, mText, DTA_FullscreenScale, FSMode_Fit320x200, DTA_Color, color, DTA_ScaleX, zoom * scalex, DTA_ScaleY, zoom);
	}
}

//----------------------------------------------------------------------------
//
// 
//
//----------------------------------------------------------------------------
class ExhumedMainMenu : ListMenu
{
	override void Ticker()
	{
		Super.Ticker();
		let delegate = ExhumedMenuDelegate(menuDelegate);
		if (!delegate) return;
		// handle the menu zoom-in. The zoom is stored in the delegate so that it can be accessed by code which does not receive a reference to the menu.
		if (delegate.zoomsize < 1.)
		{
			delegate.zoomsize += 0.0625;
			if (delegate.zoomsize >= 1.)
				delegate.zoomsize = 1.;
		}
	}

}
