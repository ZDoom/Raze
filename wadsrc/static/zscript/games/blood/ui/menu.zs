
//-------------------------------------------------------------------------------------------
//
// Caption drawer
//
//-------------------------------------------------------------------------------------------

class BloodMenuDelegate : RazeMenuDelegate
{
	override int DrawCaption(String title, Font fnt, int y, bool drawit)
	{
		return BloodScreen.DrawCaption(title, y, drawit);	// this ignores the passed font intentionally.
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



//=============================================================================
//
// text item
//
//=============================================================================

class ListMenuItemBloodTextItem : ListMenuItemTextItem
{
	void Init(ListMenuDescriptor desc, String text, String hotkey, Name child, int param = 0)
	{
		Super.Init(desc, text, hotkey, child, param);
	}

	void InitDirect(double x, double y, int height, String hotkey, String text, Font font, int color, int color2, Name child, int param = 0)
	{
		Super.InitDirect(x, y, height, hotkey, text, font, color, color2, child, param);
	}

	override void Draw(bool selected, ListMenuDescriptor desc)
	{
		int shade = Selectable()? 32: 48;
		int pal = 5;
		let gamefont = Raze.PickBigFont();
		int xpos = mXpos - gamefont.StringWidth(mText) / 2;
		int cr = generic_ui? Font.CR_GRAY : Font.CR_NATIVEPAL;
		int trans = generic_ui? 0 : Translation.MakeID(Translation_Remap, pal);

		if (selected) shade = 32 - ((MSTime() * 120 / 1000) & 63);

		Screen.DrawText(gamefont, Font.CR_UNTRANSLATED, xpos+1, mYpos+1, mText, DTA_Color, 0xff000000, DTA_FullscreenScale, FSMode_Fit320x200);
		Screen.DrawText(gamefont, Font.CR_NATIVEPAL, xpos, mYpos, mText, DTA_TranslationIndex, trans, DTA_Color, Raze.shadeToLight(shade), DTA_FullscreenScale, FSMode_Fit320x200);
	}

}


class ImageScrollerPageQavDrawer : ImageScrollerPage
{
	String qavn;
	voidptr qav;

	void Init(ImageScrollerDescriptor desc, String qavname)
	{
		Super.Init();
		qavn = qavname;
		qav = null;
	} 

	override void OnDestroy()
	{
		if (qav) DestroyQav(qav);
		Super.OnDestroy();
	}

	override void Drawer(bool selected)
	{
		if (qav) DrawQav(qav);
	}

	override void OnStartPage()
	{
		qav = LoadQav(qavn);
	}

	override void OnEndPage()
	{
		if (qav) DestroyQav(qav);
		qav = null;
	}

	native static voidptr LoadQav(string s);
	native static void DestroyQav(voidptr s);
	native static void DrawQav(voidptr s);
} 
