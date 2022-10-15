//-------------------------------------------------------------------------
/*
Copyright (C) 1996, 2003 - 3D Realms Entertainment
Copyright (C) 2000, 2003 - Matt Saettler (EDuke Enhancements)
Copyright (C) 2020-2021 - Christoph Oelckers

This file is part of Enhanced Duke Nukem 3D version 1.5 - Atomic Edition

Duke Nukem 3D is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

Original Source: 1996 - Todd Replogle
Prepared for public release: 03/21/2003 - Charlie Wiederhold, 3D Realms

EDuke enhancements integrated: 04/13/2003 - Matt Saettler

Note: EDuke source was in transition.  Changes are in-progress in the
source as it is released.

*/
//------------------------------------------------------------------------- 


class DukeCommonStatusBar : RazeStatusBar
{

	HUDFont numberFont;
	HUDFont miniFont;
	HUDFont digiFont;
	double scale;
	Array<String> ammo_sprites;
	Array<String> item_icons;

	int tileHeight(String tex)
	{
		let img = TexMan.CheckForTexture(tex, TexMan.TYPE_Any);
		let siz = TexMan.GetScaledSize(img);
		return int(siz.Y);
	}

	//==========================================================================
	//
	// Frag bar - todo
	//
	//==========================================================================
	/*
	void displayfragbar(void)
	{
		short i, j;

		j = 0;

		for (i = connecthead; i >= 0; i = connectpoint2[i])
			if (i > j) j = i;

		auto tex = tileGetTexture(TILE_FRAGBAR);
		for (int y = 0; y < 32; y += 8)
			DrawTexture(twod, tex, 0, 0, DTA_FullscreenScale, FSMode_Fit320x200, DTA_ScaleX, 1.001, DTA_ScaleY, 1.001, TAG_Done);

		for (i = connecthead; i >= 0; i = connectpoint2[i])
		{
			m initext(21 + (73 * (i & 3)), 2 + ((i & 28) << 1), &ud.user_name[i][0], ps[i].GetActor().s.pal, 2 + 8 + 16 + 128);
			sprintf(tempbuf, "%d", ps[i].frag - ps[i].fraggedself);
			m initext(17 + 50 + (73 * (i & 3)), 2 + ((i & 28) << 1), tempbuf, ps[i].GetActor().s.pal, 2 + 8 + 16 + 128);
		}
	}
	*/
	//==========================================================================
	//
	// Common inventory icon code for all styles
	//
	//==========================================================================

	String, int ontext(DukePlayer p)
	{
		String first = "";
		int second = 6;

		int onstate = 0x80000000;
		switch (p.inven_icon)
		{
		case Duke.ICON_HOLODUKE:
			onstate = p.holoduke_on != null;
			break;
		case Duke.ICON_JETPACK:
			onstate = p.jetpack_on;
			break;
		case Duke.ICON_HEATS:
			onstate = p.heat_on;
			break;
		}

		// Texts are intentionally not translated because the font is too small for making localization work and the translated words are too long.
		if (onstate != 0x80000000 && !(gameinfo.gameType & (GAMEFLAG_WW2GI|GAMEFLAG_RRALL)))
		{
			second = onstate > 0 ? 0 : 2;
			first = onstate > 0 ? "ON" : "OFF";
		}
		if (p.inven_icon >= Duke.ICON_SCUBA)
		{
			second = 2;
			first = "AUTO";
		}
		return first, second;
	}

	//==========================================================================
	//
	// draws the inventory selector
	//
	//==========================================================================

	void DrawInventory(DukePlayer p, double x, double y, int align)
	{
		if (p.invdisptime <= 0)return;

		int n = 0, j = 0;
		if (p.firstaid_amount > 0) { n |= 1; j++; }
		if (p.steroids_amount > 0) { n |= 2; j++; }
		if (p.holoduke_amount > 0) { n |= 4; j++; }
		if (p.jetpack_amount > 0) { n |= 8; j++; }
		if (p.heat_amount > 0) { n |= 16; j++; }
		if (p.scuba_amount > 0) { n |= 32; j++; }
		if (p.boot_amount > 0) { n |= 64; j++; }

		x -= (j * 11);
		y -= 6;

		align |= DI_ITEM_CENTER;
		for(int bit = 0; bit < 7; bit++)
		{
			int i = 1 << bit;
			if (n & i)
			{
				int select = 1 << (p.inven_icon - 1);
				double alpha = select == i ? 1.0 : 0.7;
				DrawImage(item_icons[bit+1], (x, y), align, alpha, scale:(scale, scale));
				if (select == i) DrawImage("ARROW", (Raze.isWW2GI()? x + 7.5 : x, Raze.isWW2GI()? y + 0.5 : y), align, alpha, scale:(scale, scale));
				x += 22;
			}
		}
	}

	//==========================================================================
	//
	// Statistics output
	//
	//==========================================================================

	void DoLevelStats(int bottomy, SummaryInfo info)
	{
		StatsPrintInfo stats;
		stats.fontscale = Raze.isRR() ? 0.5 : 1.;
		stats.screenbottomspace = bottomy;

		int y = -1;
		int mask = 1;
		if (automapMode == am_full)
		{
			stats.statfont = SmallFont2;
			stats.spacing = 6;
			if (Raze.isNamWW2GI()) stats.altspacing = 10;
			else if (!Raze.isRR()) stats.altspacing = 11;
			else stats.altspacing = 14;

			stats.standardColor = Font.TEXTCOLOR_UNTRANSLATED;
			stats.letterColor = Font.TEXTCOLOR_GOLD;
			y = PrintAutomapInfo(stats, info, false);
			mask = 2;
		}
		if (hud_stats & mask)
		{
			stats.statfont = SmallFont;
			stats.letterColor = Font.TEXTCOLOR_ORANGE;
			if (Raze.isNamWW2GI())
			{
				stats.statfont = ConFont;
				stats.spacing = 8;
				stats.standardColor = Font.TEXTCOLOR_YELLOW;
				stats.completeColor = Font.TEXTCOLOR_FIRE;
			}
			else if (!Raze.isRR())
			{
				stats.spacing = 7;
				stats.standardColor = Font.TEXTCOLOR_CREAM;
				stats.completeColor = Font.TEXTCOLOR_FIRE;
			}
			else
			{
				stats.spacing = 10;
				stats.standardColor =
					stats.completeColor = Font.TEXTCOLOR_UNTRANSLATED;
			}
			PrintLevelStats(stats, info, y);
		}
	}

}