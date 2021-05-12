

struct StatsPrintInfo
{
	int screenbottomspace;
	int spacing; // uses fontheight if 0 or less.
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
	void Init()
	{
		SetSize(0, 320, 200);
	}
	
	virtual void Tick() {}
	virtual void UpdateStatusBar(SummaryInfo info) {}

	void drawStatText(Font statFont, int x, int y, String text, double scale)
	{
		Screen.DrawText(statfont, Font.CR_UNTRANSLATED, x + scale, y + scale, text, DTA_FullscreenScale, FSMode_ScaleToHeight, DTA_VirtualWidth, 320, DTA_VirtualHeight, 200,
			DTA_KeepRatio, true, DTA_ScaleX, scale, DTA_ScaleY, scale, DTA_LegacyRenderStyle, STYLE_TranslucentStencil, DTA_Color, 0);
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

	void PrintLevelStats(StatsPrintInfo info, SummaryInfo stats)
	{

		double y;
		double scale = info.fontscale * hud_statscale;
		if (info.spacing <= 0) info.spacing = info.statfont.GetHeight() * info.fontscale;
		double spacing = info.spacing * hud_statscale;
		if (info.screenbottomspace < 0)
		{
			y = 200 - (RelTop - info.screenbottomspace) * hud_scalefactor - spacing;
		}
		else
		{
			y = 200 - info.screenbottomspace * hud_scalefactor - spacing;
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

	void PrintAutomapInfo(StatsPrintInfo info, bool forcetextfont = false)
	{
		let TEXTCOLOR_ESCAPESTR= "\034";
		let lev = currentLevel;
		String mapname;
		if (am_showlabel) 
			mapname.Format("%s%s: %s%s", info.letterColor, lev.GetLabelName(), info.standardColor, lev.DisplayName());
		else 
			mapname.Format("%s%s", info.standardColor, lev.DisplayName());

		forcetextfont |= am_textfont;
		double y;
		double scale = info.fontScale * (forcetextfont ? hud_statscale : 1);	// the tiny default font used by all games here cannot be scaled for readability purposes.
		if (info.spacing <= 0) info.spacing = info.statfont.GetHeight() * info.fontScale;
		double spacing = info.spacing * (forcetextfont ? hud_statscale : 1);
		if (am_nameontop)
		{
			y = spacing + 1;
		}
		else if (info.screenbottomspace < 0)
		{
			y = 200 - RelTop - spacing;
		}
		else
		{
			y = 200 - info.screenbottomspace - spacing;
		}
		let cluster = lev.GetCluster();
		String volname;
		if (cluster) volname = cluster.name;
		if (volname.length() == 0 && am_nameontop) y = 1;

		Screen.DrawText(info.statfont, Font.CR_UNTRANSLATED, 2 * hud_statscale, y, mapname, DTA_FullscreenScale, FSMode_ScaleToHeight, DTA_VirtualWidth, 320, DTA_VirtualHeight, 200,
			DTA_ScaleX, scale, DTA_ScaleY, scale, DTA_KeepRatio, true);
		y -= spacing;
		if (volname.length() > 0)
			Screen.DrawText(info.statfont, Font.CR_UNTRANSLATED, 2 * hud_statscale, y, volname, DTA_FullscreenScale, FSMode_ScaleToHeight, DTA_VirtualWidth, 320, DTA_VirtualHeight, 200,
				DTA_ScaleX, scale, DTA_ScaleY, scale, DTA_KeepRatio, true);
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
