/*
** Enhanced heads up 'overlay' for fullscreen
**
**---------------------------------------------------------------------------
** Copyright 2003-2008 Christoph Oelckers
** All rights reserved.
**
** Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions
** are met:
**
** 1. Redistributions of source code must retain the above copyright
**    notice, this list of conditions and the following disclaimer.
** 2. Redistributions in binary form must reproduce the above copyright
**    notice, this list of conditions and the following disclaimer in the
**    documentation and/or other materials provided with the distribution.
** 3. The name of the author may not be used to endorse or promote products
**    derived from this software without specific prior written permission.
**
** THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
** IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
** OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
** IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
** INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
** NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
** THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
**---------------------------------------------------------------------------
**
*/

struct HudStats
{
	String healthicon;
	int healthtranslation;
	int healthvalue;
	
	Array<String> armoricons;
	Array<int> armortranslations;
	Array<int> armorvalues;
	
	Array<String> weaponicons;
	Array<int> weapontranslations;
	int weaponselect;
	
	Array<String> ammoicons;
	Array<int> ammotranslations;
	Array<int> ammovalues;
	Array<int> ammomaxvalues;
	Array<int> ammoaltvalues;
	int ammoselect;
	
	Array<String> keyicons;
	Array<TranslationID> keytranslations;

	Array<String> inventoryicons;
	Array<int> inventoryamounts;	// negative values can be used for special states (-1: "ON", -2: "OFF", -3: "AUTO")
	int inventoryselect;
	
	StatsPrintInfo info;
	
	void Clear()
	{
		healthicon = "";
		healthvalue = 0;
		healthtranslation = 0;
		armoricons.Clear();
		armortranslations.Clear();
		armorvalues.Clear();

		ammoselect = -1;
		ammoicons.Clear();
		ammotranslations.Clear();
		ammovalues.Clear();
		ammomaxvalues.Clear();
		ammoaltvalues.Clear();

		weaponselect = -1;
		weaponicons.Clear();
		weapontranslations.Clear();
		
		keyicons.Clear();
		keytranslations.Clear();
		
		inventoryselect = -1;
		inventoryicons.Clear();
		inventoryamounts.Clear();
	}
};

	
class AltHud ui
{
	TextureID invgem_left, invgem_right;
	int hudwidth, hudheight;
	int statspace;
	Font HudFont;					// The font for the health and armor display
	Font IndexFont;					// The font for the inventory indices
	Font StatFont;
	int HudFontOffset;
	const POWERUPICONSIZE = 32;
	HudStats currentStats;			// must be filled in by the status bar.

	
	virtual void Init()
	{
		HudFont = BigFont;
		if (isBlood()) HudFont = Font.GetFont("HUDFONT_BLOOD");
		else if (isDuke()) HudFontOffset = 6;
		IndexFont = Font.GetFont("HUDINDEXFONT");
		if (IndexFont == NULL) IndexFont = ConFont;	// Emergency fallback
		if (!isNamWW2GI())
			StatFont = SmallFont;
		else
			StatFont = ConFont;

		invgem_left = TexMan.CheckForTexture("INVGEML1", TexMan.Type_MiscPatch);
		invgem_right = TexMan.CheckForTexture("INVGEMR1", TexMan.Type_MiscPatch);
		statspace = StatFont.StringWidth("Ac:");
	}
	
	//---------------------------------------------------------------------------
	//
	// Draws an image into a box with its bottom center at the bottom
	// center of the box. The image is scaled down if it doesn't fit
	//
	//---------------------------------------------------------------------------

	void DrawImageToBox(TextureID tex, int x, int y, int w, int h, double trans = 0.75, bool animate = false, TranslationID translation = 0)
	{
		double scale1, scale2;

		if (tex)
		{
			let texsize = TexMan.GetScaledSize(tex);

			if (w < texsize.X) scale1 = w / texsize.X;
			else scale1 = 1.0;
			if (h < texsize.Y) scale2 = h / texsize.Y;
			else scale2 = 1.0;
			scale1 = min(scale1, scale2);
			if (scale2 < scale1) scale1=scale2;

			x += w >> 1;
			y += h;

			w = (int)(texsize.X * scale1);
			h = (int)(texsize.Y * scale1);

			screen.DrawTexture(tex, animate, x, y,
				DTA_KeepRatio, true,
				DTA_VirtualWidth, hudwidth, DTA_VirtualHeight, hudheight, DTA_Alpha, trans,  DTA_TranslationIndex, translation,
				DTA_DestWidth, w, DTA_DestHeight, h, DTA_CenterBottomOffset, 1);

		}
	}

	
	//---------------------------------------------------------------------------
	//
	// Draws a text but uses a fixed width for all characters
	//
	//---------------------------------------------------------------------------

	void DrawHudText(Font fnt, int color, String text, int x, int y, double trans = 0.75, double fontscale = 1.0)
	{
		//if (fnt == HudFont) y += HudFontOffset;
		int zerowidth = fnt.GetCharWidth("0");
		screen.DrawText(fnt, color, x, y - (fnt.GetHeight() - fnt.Getdisplacement()) * fontscale, text, DTA_VirtualWidth, hudwidth, DTA_VirtualHeight, hudheight,
			 DTA_KeepRatio, true, DTA_Alpha, trans, DTA_Monospace, MONO_CellCenter, DTA_Spacing, zerowidth, DTA_ScaleX, fontscale, DTA_ScaleY, fontscale);
	}


	//---------------------------------------------------------------------------
	//
	// Draws a number with a fixed width for all digits
	//
	//---------------------------------------------------------------------------

	void DrawHudNumber(Font fnt, int color, int num, int x, int y, double trans = 0.75, double fontscale = 1.0)
	{
		DrawHudText(fnt, color, String.Format("%d", num), x, y, trans, fontscale);
	}

	//---------------------------------------------------------------------------
	//
	// Draws a time string as hh:mm:ss
	//
	//---------------------------------------------------------------------------

	virtual void DrawTimeString(Font fnt, int color, int seconds, int x, int y, double trans = 0.75, double fontscale = 1.0)
	{
		String s = String.Format("%02i:%02i:%02i", seconds / 3600, (seconds % 3600) / 60, seconds % 60);
		int length = 8 * fnt.GetCharWidth("0");
		DrawHudText(fnt, color, s, x-length * fontscale, y, trans, fontscale);
	}
	
	//===========================================================================
	//
	// draw the status (number of kills etc)
	//
	//===========================================================================

	virtual void DrawStatLine(int x, in out int y, String prefix, String text, double fontscale)
	{

		y -= (StatFont.GetHeight()-1) * fontscale;
		screen.DrawText(StatFont, Font.CR_UNTRANSLATED, x, y, prefix, 
			DTA_KeepRatio, true,
			DTA_VirtualWidth, hudwidth, DTA_VirtualHeight, hudheight, DTA_Alpha, 0.75, DTA_ScaleX, fontscale, DTA_ScaleY, fontscale);

		screen.DrawText(StatFont, Font.CR_UNTRANSLATED, x+statspace, y, text,
			DTA_KeepRatio, true,
			DTA_VirtualWidth, hudwidth, DTA_VirtualHeight, hudheight, DTA_Alpha, 0.75, DTA_ScaleX, fontscale, DTA_ScaleY, fontscale);
	}

	virtual void DrawStatus(SummaryInfo stats, int x, int y, double fontscale)
	{
		//if (!deathmatch)
		{
			if (hud_showsecrets && stats.maxsecrets > 0)
			{
				String prefix = String.Format("%sS:", currentStats.info.letterColor);
				String text = String.Format("%s%d/%d", stats.secrets >= stats.maxsecrets ? currentStats.info.completeColor : currentStats.info.standardColor, stats.secrets, stats.maxsecrets);
				if (stats.supersecrets > 0) text.AppendFormat("+%d", stats.supersecrets);
				
				DrawStatLine(x, y, prefix, text, fontscale);
			}
			
			if (hud_showkills && stats.maxkills != -1)
			{
				String prefix;
				String text;
				
				if (stats.maxkills == -3) prefix = String.Format("%sF:", currentStats.info.letterColor);
				else prefix = String.Format("%sK:", currentStats.info.letterColor);
				
				if (stats.maxkills == -3) text = String.Format("%s%d", currentStats.info.standardColor, stats.kills);
				else if (stats.maxkills == -2) text = String.Format("%s%d", currentStats.info.standardColor, stats.kills);
				else text = String.Format("%s%d/%d", stats.kills == stats.maxkills ? currentStats.info.completeColor : currentStats.info.standardColor, stats.kills, stats.maxkills);
				
				DrawStatLine(x, y, prefix, text, fontscale);
			}

			if (hud_showtimestat)
			{
				String prefix = String.Format("%sT:", currentStats.info.letterColor);
				String text;
				let seconds = stats.time / 1000;

				if (seconds >= 3600)
					text = String.Format("%s%02i:%02i:%02i", currentStats.info.standardColor, seconds / 3600, (seconds % 3600) / 60, seconds % 60);
				else
					text = String.Format("%s%02i:%02i", currentStats.info.standardColor, seconds / 60, seconds % 60);
				DrawStatLine(x, y, prefix, text, fontscale);
			}
		}
	}

	//===========================================================================
	//
	// draw health
	//
	//===========================================================================

	virtual void DrawHealth(int x, int y, double fontscale)
	{
		int health = currentStats.healthvalue;

		// decide on the color first
		int fontcolor =
			health < hud_health_red ? Font.CR_RED :
			health < hud_health_yellow ? Font.CR_GOLD :
			health <= hud_health_green ? Font.CR_GREEN :
			Font.CR_BLUE;

		DrawImageToBox(TexMan.CheckForTexture(currentStats.healthicon), x, y, 31, 17, 0.75, true);
		if (isSW()) y -= 4; // still need to figure out why the font is misaligned this much.
		DrawHudNumber(HudFont, fontcolor, health, x + 33, y + 17, fontscale:fontscale);
	}

	//===========================================================================
	//
	// Draw Armor.
	// very similar to drawhealth, but adapted to handle Hexen armor too
	//
	//===========================================================================

	virtual void DrawArmor(int xx, int y, double fontscale)
	{
		int x = xx;
		int spacing = HudFont.StringWidth("000") * fontscale;
		for(int i = 0; i < currentStats.armoricons.Size(); i++)
		{
			int ap = currentStats.armorvalues[i];

			// decide on color
			int fontcolor =
				ap < hud_armor_red ? Font.CR_RED :
				ap < hud_armor_yellow ? Font.CR_GOLD :
				ap <= hud_armor_green ? Font.CR_GREEN :
				Font.CR_BLUE;

			DrawImageToBox(TexMan.CheckForTexture(currentStats.armoricons[i]), x, y, 31, 17, 0.75, true);
			if (isSW()) y -= 4; // still need to figure out why the font is misaligned.
			if (ap >= 0) DrawHudNumber(HudFont, fontcolor, ap, x + 33, y + 17, fontscale:fontscale);
			x += 35 + spacing;
		}
	}

	//===========================================================================
	//
	// KEYS
	//
	//===========================================================================

	//---------------------------------------------------------------------------
	//
	// Draw one key
	//
	//---------------------------------------------------------------------------

	virtual bool DrawOneKey(int x, int y, int keyindex)
	{
		TextureID icon = TexMan.CheckForTexture(currentstats.keyicons[keyindex]);

		if (icon.isValid())
		{
			TranslationID trans = 0;
			if (currentStats.keytranslations.Size()) trans = currentStats.keytranslations[keyindex];
			DrawImageToBox(icon, x, y, 8, 10, 0.75, false, trans);
			return true;
		}
		return false;
	}

	//---------------------------------------------------------------------------
	//
	// Draw all keys
	//
	//---------------------------------------------------------------------------

	virtual int DrawKeys(int x, int y)
	{
		int yo = y;
		int xo = x;
		int i;
		int c = 0;

		// Go through the list in reverse order of definition, because we start at the right.
		for(int i = currentStats.keyicons.Size() - 1; i >= 0; i--)
		{
			if (DrawOneKey(x - 9, y, i))
			{
				x -= 9;
				if (++c >= 10)
				{
					x = xo;
					y -= 11;
					c = 0;
				}
			}
		}
		if (x == xo && y != yo) y += 11;	// undo the last wrap if the current line is empty.
		return y - 11;
	}

	//---------------------------------------------------------------------------
	//
	// Drawing Ammo helpers
	//
	//---------------------------------------------------------------------------

	static int GetDigitCount(int value)
	{
		int digits = 0;

		do
		{
			value /= 10;
			++digits;
		}
		while (0 != value);

		return digits;
	}

	int, int GetAmmoTextLengths()
	{
		int tammomax = 0, tammocur = 0;
		for(int i = 0; i < currentStats.ammovalues.Size(); i++)
		{
			int ammomax = currentstats.ammomaxvalues[i], ammocur = currentstats.ammovalues[i];
			tammocur = MAX(ammocur, tammocur);
			tammomax = MAX(ammomax, tammomax);
		}
		return GetDigitCount(tammocur), GetDigitCount(tammomax);
	}

	//---------------------------------------------------------------------------
	//
	// Drawing Ammo 
	//
	//---------------------------------------------------------------------------

	virtual int DrawAmmo(int x, int y)
	{
		int ammocurlen = 0;
		int ammomaxlen = 0;
		[ammocurlen, ammomaxlen] = GetAmmoTextLengths();

		String buf = String.Format("%0*d/%0*d", ammocurlen, 0, ammomaxlen, 0);
		int def_width = ConFont.StringWidth(buf);
		int yadd = ConFont.GetHeight();

		int xtext = x - def_width;
		int ximage = x;

		if (hud_ammo_order > 0)
		{
			xtext -= 24;
			ximage -= 20;
		}
		else
		{
			ximage -= def_width + 20;
		}

		
		// Go through the list in reverse order of definition, because we start at the right.
		for(int i = currentStats.ammoicons.Size() - 1; i >= 0; i--)
		{
			int curammo = currentStats.ammovalues[i];
			int maxammo = currentStats.ammomaxvalues[i];

			double trans=  i == currentstats.ammoselect ? 0.75 : 0.375;

			// buf = String.Format("%d/%d", ammo, maxammo);
			buf = String.Format("%*d/%*d", ammocurlen, curammo, ammomaxlen, maxammo);

			int tex_width= clamp(ConFont.StringWidth(buf) - def_width, 0, 1000);

			int fontcolor=( !maxammo ? Font.CR_GRAY :    
							 curammo < ( (maxammo * hud_ammo_red) / 100) ? Font.CR_RED :   
							 curammo < ( (maxammo * hud_ammo_yellow) / 100) ? Font.CR_GOLD : Font.CR_GREEN );

			DrawHudText(ConFont, fontcolor, buf, xtext-tex_width, y+yadd, trans);
			DrawImageToBox(TexMan.CheckForTexture(currentStats.ammoicons[i]), ximage, y, 16, 8, trans);
			y-=10;
		}
		return y;
	}

	//---------------------------------------------------------------------------
	//
	// Drawing weapons
	//
	//---------------------------------------------------------------------------

	virtual void DrawOneWeapon(int x, in out int y, int weapon)
	{
		double trans = weapon == currentstats.weaponselect? 0.85 : 0.4;

		TextureID texid = TexMan.CheckForTexture(currentstats.weaponicons[weapon]);

		if (texid.isValid())
		{
			// don't draw tall sprites too small.
			int w, h;
			[w, h] = TexMan.GetSize(texid);
			int rh;
			if (w > h) rh = 8;
			else 
			{
				rh = 16;
				y -= 8;	
			}
			DrawImageToBox(texid, x-24, y, 20, rh, trans);
			y-=10;
		}
	}


	virtual void DrawWeapons(int x, int y)
	{
		// Go through the list in reverse order of definition, because we start at the right.
		for(int i = currentStats.weaponicons.Size() - 1; i >= 0; i--)
		{
			DrawOneWeapon(x, y, i);
		}
	}

	//---------------------------------------------------------------------------
	//
	// Draw the Inventory
	//
	//---------------------------------------------------------------------------

	virtual void DrawInventory(int x,int y)
	{
		int numitems = (hudwidth - 2*x) / 32;
		int i;

		int item = 0;//StatusBar.ValidateInvFirst(numitems);
		if(item != 0)
		{
			screen.DrawTexture(invgem_left, true, x-10, y,
				DTA_KeepRatio, true,
				DTA_VirtualWidth, hudwidth, DTA_VirtualHeight, hudheight, DTA_Alpha, 0.4);
		}

		int itemcount = currentstats.inventoryicons.Size();
		for(i = 0; i < itemcount;)
		{
			int amount = currentstats.inventoryamounts[i];
			double trans = i == currentstats.inventoryselect ? 1.0 : 0.4;

			DrawImageToBox(TexMan.CheckForTexture(currentstats.inventoryicons[i]), x, y, 19, 25, trans, true);
			if (amount > 0)
			{
				String buffer = String.Format("%d", amount);

				int xx = amount >= 100? 18 : 22;
				screen.DrawText(IndexFont, Font.CR_GOLD, x+xx, y+20, buffer, 
					DTA_KeepRatio, true,
					DTA_VirtualWidth, hudwidth, DTA_VirtualHeight, hudheight, DTA_Alpha, trans);
			}
			
			x+=32;
			i++;
			if (item + i == numitems) break;
		}
		if(i < itemcount)
		{
			screen.DrawTexture(invgem_right, true, x-10, y,
				DTA_KeepRatio, true,
				DTA_VirtualWidth, hudwidth, DTA_VirtualHeight, hudheight, DTA_Alpha, 0.4);
		}
	}

	//---------------------------------------------------------------------------
	//
	// PROC DrawCoordinates
	//
	//---------------------------------------------------------------------------
	
	void DrawCoordinateEntry(int xpos, int ypos, String coordstr, double fontscale)
	{
		screen.DrawText(StatFont, hudcolor_xyco, xpos, ypos, coordstr,
						 DTA_KeepRatio, true, DTA_ScaleX, fontscale, DTA_ScaleY, fontscale,
						 DTA_VirtualWidth, hudwidth, DTA_VirtualHeight, hudheight);
	}

	virtual void DrawCoordinates(bool withmapname, double fontscale)
	{
		/* todo when everything else works.
		Vector3 pos = (0, 0, 0);
		String coordstr;
		int h = StatFont.GetHeight();
		

		int xpos = hudwidth - StatFont.StringWidth("X: -00000")-6;
		int ypos = 18;

		if (withmapname)
		{
			let font = generic_ui? NewSmallFont : StatFont.CanPrint(Level.LevelName)? StatFont : OriginalSmallFont;
			int hh = font.GetHeight();

			screen.DrawText(font, hudcolor_titl, hudwidth - 6 - font.StringWidth(Level.MapName), ypos, Level.MapName,
				DTA_KeepRatio, true, DTA_ScaleX, fontscale, DTA_ScaleY, fontscale,
				DTA_VirtualWidth, hudwidth, DTA_VirtualHeight, hudheight);

			screen.DrawText(font, hudcolor_titl, hudwidth - 6 - font.StringWidth(Level.LevelName), ypos + hh, Level.LevelName,
				DTA_KeepRatio, true, DTA_ScaleX, fontscale, DTA_ScaleY, fontscale,
				DTA_VirtualWidth, hudwidth, DTA_VirtualHeight, hudheight);
	
			ypos += 2 * hh + h;
		}
		
		DrawCoordinateEntry(xpos, ypos, String.Format("X: %.0f", pos.X), fontscale);
		ypos += h;
		DrawCoordinateEntry(xpos, ypos, String.Format("Y: %.0f", pos.Y), fontscale);
		ypos += h;
		DrawCoordinateEntry(xpos, ypos, String.Format("Z: %.0f", pos.Z), fontscale);
		ypos += h;

		if (hud_showangles)
		{
			DrawCoordinateEntry(xpos, ypos, String.Format("Y: %.0f", Actor.Normalize180(mo.Angle)), fontscale);
			ypos += h;
			DrawCoordinateEntry(xpos, ypos, String.Format("P: %.0f", Actor.Normalize180(mo.Pitch)), fontscale);
			ypos += h;
			DrawCoordinateEntry(xpos, ypos, String.Format("R: %.0f", Actor.Normalize180(mo.Roll)));
		}
		*/
	}
	
	//---------------------------------------------------------------------------
	//
	// main drawer
	//
	//---------------------------------------------------------------------------

	virtual void DrawInGame(SummaryInfo summary)
	{
		// No HUD in the title level!
		if (gamestate == GS_TITLELEVEL) return;
		let fontscale = currentStats.info.fontscale;

		DrawStatus(summary, 5, hudheight-50, fontscale);
		DrawHealth(5, hudheight - 45, fontscale);
		DrawArmor(5, hudheight-20, fontscale);
		
		int y = DrawKeys(hudwidth-4, hudheight-10);
		y = DrawAmmo(hudwidth-5, y);
		if (hud_showweapons) DrawWeapons(hudwidth - 5, y);
		DrawInventory(200, hudheight - 28);
	}

	//---------------------------------------------------------------------------
	//
	// automap drawer
	//
	//---------------------------------------------------------------------------

	virtual void DrawAutomap(SummaryInfo summary)
	{
		let lev = currentlevel;
		let amstr = String.Format("%s: \034%c%s", lev.GetLabelName(), hudcolor_titl + 65, lev.DisplayName());
		
		let cluster = lev.GetCluster();
		String volname;
		if (cluster) volname = cluster.name;

		let allname = amstr .. volname;
		let myfont = generic_ui? NewSmallFont : StatFont.CanPrint(allname)? StatFont : OriginalSmallFont;
		int bottom = hudheight - 1;
		double fontscale = generic_ui? 1. : currentStats.info.fontscale;
		double fonth = myfont.GetHeight() * fontscale + 1;
	
		if (am_showtotaltime)
		{
			let seconds = summary.totaltime / 1000;
			DrawTimeString(myfont, hudcolor_ttim, seconds, hudwidth-2, bottom, 1, fontscale);
			bottom -= fonth;
		}

		if (am_showtime)
		{
			let seconds = summary.time / 1000;
			DrawTimeString(myfont, hudcolor_ltim, seconds, hudwidth-2, bottom, 1, fontscale);
		}

		screen.DrawText(myfont, Font.CR_BRICK, 2, hudheight - fonth - 1, amstr,
			DTA_KeepRatio, true, DTA_ScaleX, fontscale, DTA_ScaleY, fontscale,
			DTA_VirtualWidth, hudwidth, DTA_VirtualHeight, hudheight);
			
		if (volname.length() > 0)
		{
			Screen.DrawText(myfont, Font.CR_ORANGE, 2, hudheight - fonth * 2 - 1, volname,
				DTA_KeepRatio, true, DTA_ScaleX, fontscale, DTA_ScaleY, fontscale,
				DTA_VirtualWidth, hudwidth, DTA_VirtualHeight, hudheight);
		}
	}

	//---------------------------------------------------------------------------
	//
	// main drawer
	//
	//---------------------------------------------------------------------------

	virtual void Draw(RazeStatusBar stbar, SummaryInfo summary, int w, int h)
	{
		hudwidth = w;
		hudheight = h;
		stbar.GetAllStats(currentStats);
		if (automapMode != am_full)
		{
			DrawInGame(summary);
		}
		else
		{
			DrawAutomap(summary);
		}
	}
	
}
