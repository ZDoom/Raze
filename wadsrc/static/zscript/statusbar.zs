

struct StatsPrintInfo
{
	int screenbottomspace;
	int spacing; // uses fontheight if 0 or less.
	int altspacing;	// in case a larger replacement font is needed.
	String letterColor, standardColor, completeColor;
	double fontscale;
	Font statfont;
};


//============================================================================
//
//
//
//============================================================================

class RazeStatusBar : StatusBarCore
{
	virtual void Init()
	{
	}

	virtual void Tick() {}
	virtual void Reset() {}
	virtual void UpdateStatusBar(SummaryInfo info) {}

	void drawStatText(Font statFont, double x, double y, String text, double scale)
	{
		Screen.DrawText(statfont, Font.CR_UNTRANSLATED, x + scale, y + scale, text, DTA_FullscreenScale, FSMode_ScaleToHeight, DTA_VirtualWidth, 320, DTA_VirtualHeight, 200,
			DTA_KeepRatio, true, DTA_ScaleX, scale, DTA_ScaleY, scale, DTA_LegacyRenderStyle, STYLE_TranslucentStencil, DTA_Color, 0x80000000);
		Screen.DrawText(statfont, Font.CR_UNTRANSLATED, x, y, text, DTA_FullscreenScale, FSMode_ScaleToHeight, DTA_VirtualWidth, 320, DTA_VirtualHeight, 200,
			DTA_KeepRatio, true, DTA_ScaleX, scale, DTA_ScaleY, scale);
	}

	//============================================================================
	//
	// Prints the current level statistics
	// hud_statscale is the desired display scale for the stat display
	// hud_scalefactor is the desired display scale for the actual status bar / HUD
	//
	//============================================================================

	void PrintLevelStats(StatsPrintInfo info, SummaryInfo stats, double y = -1)
	{
		double scale = info.fontscale * hud_statscale;
		if (info.spacing <= 0) info.spacing = info.statfont.GetHeight() * info.fontscale;
		double spacing = info.spacing * hud_statscale;
		
		if (y < 0)
		{
			if (hud_size == Hud_Nothing)
			{
				y = 198 - spacing;
			}
			else if (info.screenbottomspace < 0)
			{
				y = 200 - (RelTop - info.screenbottomspace) * hud_scalefactor - spacing;
			}
			else
			{
				y = 200 - info.screenbottomspace * hud_scalefactor - spacing;
			}
		}

		double y1, y2, y3;

		if (stats.maxsecrets > 0)	// don't bother if there are no secrets.
		{
			y1 = y;
			y -= spacing;
		}
		if (stats.maxkills != -1)
		{
			y2 = y;
			y -= spacing;
		}
		y3 = y;

		String text;

		text = String.Format("%sT: %s%d:%02d", info.letterColor, info.standardColor, stats.time / 60000, (stats.time % 60000) / 1000);
		drawStatText(info.statFont, 2 * hud_statscale, y3, text, scale);

		if (stats.maxkills != -1)
		{
			if (stats.maxkills == -3) text.Format("%sF: %s%d", info.letterColor, info.standardColor, stats.kills);
			else if (stats.maxkills == -2) text.Format("%sK: %s%d", info.letterColor, info.standardColor, stats.kills);
			else text = String.Format("%sK: %s%d/%d", info.letterColor, 
				stats.kills == stats.maxkills ? info.completeColor : info.standardColor, stats.kills, stats.maxkills);

			drawStatText(info.statFont, 2 * hud_statscale, y2, text, scale);
		}

		if (stats.maxsecrets > 0)	// don't bother if there are no secrets.
		{
			text = String.Format("%sS: %s%d/%d", info.letterColor, stats.secrets >= stats.maxsecrets ? info.completeColor : info.standardColor, stats.secrets, stats.maxsecrets);
			if (stats.supersecrets > 0) text.AppendFormat("+%d", stats.supersecrets);

			drawStatText(info.statFont, 2 * hud_statscale, y1, text, scale);
		}
	}

	//============================================================================
	//
	// Prints the automap label
	// hud_statscale is the desired display scale for the stat display
	// hud_scalefactor is the desired display scale for the actual status bar / HUD
	//
	//============================================================================

	int PrintAutomapInfo(StatsPrintInfo info, SummaryInfo stats, bool forcetextfont = false)
	{
		let TEXTCOLOR_ESCAPESTR = "\034";
		let lev = currentLevel;
		let levname = lev.DisplayName();

		let cluster = lev.GetCluster();
		String volname;
		if (cluster) volname = cluster.name;

		let allname = levname .. volname;

		double scale, spacing;
		String tcol = info.standardColor;
		Font myfont;
		if (!forcetextfont && !am_textfont && info.statfont.CanPrint(allname))
		{
			scale = info.fontscale;
			spacing = info.spacing;
			myfont = info.statfont;
		}
		else
		{
			scale = info.fontscale * hud_statscale;
			spacing = info.altspacing * hud_statscale;
			myfont = Raze.isNamWW2GI()? ConFont : Raze.PickSmallFont(allname);
		}

		String mapname;
		if (am_showlabel) mapname = String.Format("%s%s: %s%s", info.letterColor, lev.GetLabelName(), tcol, levname);
		else mapname = String.Format("%s%s", tcol, levname);




		double y;
		double st_y = -1;
		if (am_nameontop)
		{
			y = spacing + 1;
			if (info.screenbottomspace < 0)
			{
				st_y = (200 - RelTop) * hud_scalefactor - spacing;
			}
			else
			{
				st_y = 200 - info.screenbottomspace * hud_scalefactor - spacing;
			}
			
		}
		else if (info.screenbottomspace < 0)
		{
			y = (200 - RelTop) * hud_scalefactor - spacing;
		}
		else
		{
			y = 200 - info.screenbottomspace * hud_scalefactor - spacing;
		}
		if (volname.length() == 0 && am_nameontop) y = 1;

		Screen.DrawText(myfont, Font.CR_UNTRANSLATED, 2 * hud_statscale, y, mapname, DTA_FullscreenScale, FSMode_ScaleToHeight, DTA_VirtualWidth, 320, DTA_VirtualHeight, 200,
			DTA_ScaleX, scale, DTA_ScaleY, scale, DTA_KeepRatio, true);
		y -= spacing;
		if (volname.length() > 0)
		{
			Screen.DrawText(myfont, Font.CR_UNTRANSLATED, 2 * hud_statscale, y, volname, DTA_FullscreenScale, FSMode_ScaleToHeight, DTA_VirtualWidth, 320, DTA_VirtualHeight, 200,
				DTA_ScaleX, scale, DTA_ScaleY, scale, DTA_KeepRatio, true);
			y -= spacing;
		}
		if (!am_nameontop)
		{
			st_y = y;
		}
		return st_y;
	}

	//============================================================================
	//
	//
	//
	//============================================================================

	int CalcMagazineAmount(int ammo_remaining, int clip_capacity, bool reloading)
	{
		// Determine amount in clip.
		int clip_amount = ammo_remaining % clip_capacity;

		// Set current clip value to clip capacity if wrapped around to zero, otherwise use determined value.
		int clip_current = ammo_remaining != 0 && clip_amount == 0 ? clip_capacity : clip_amount;

		// Return current clip value if weapon has rounds or is not on a reload cycle.
		return ammo_remaining == 0 || (reloading && clip_amount == 0) ? 0 : clip_current;
	}

	//============================================================================
	//
	//
	//
	//============================================================================

	void Set43ClipRect()
	{
		let screenratio = screen.GetAspectRatio();
		if (screenratio < 1.34) return;

		int width = int(screen.GetWidth() * 1.333 / screenratio);
		int left = (screen.GetWidth() - width) / 2;
		screen.SetClipRect(left, 0, width, screen.GetHeight());
	}
}
