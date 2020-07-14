//-------------------------------------------------------------------------
/*
Copyright (C) 1996, 2003 - 3D Realms Entertainment
Copyright (C) 2000, 2003 - Matt Saettler (EDuke Enhancements)
Copyright (C) 2020 - Christoph Oelckers

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
#include "ns.h"	// Must come before everything else!

#include "v_font.h"
#include "duke3d.h"
#include "compat.h"
#include "sbar.h"
#include "v_draw.h"
#include "texturemanager.h"
BEGIN_DUKE_NS

int levelTextTime;

//==========================================================================
//
// very much a dummy to access the methods.
// The goal is to export this to a script.
//
//==========================================================================

DDukeCommonStatusBar::DDukeCommonStatusBar()
	: numberFont(BigFont, 1, Off, 1, 1),
	indexFont(IndexFont, 4, CellRight, 1, 1),
	miniFont(SmallFont2, 1, Off, 1, 1),
	digiFont(DigiFont, 1 , Off, 1, 1)
{
	drawOffset.Y = 0;
}


//==========================================================================
//
// Frag bar - todo
//
//==========================================================================
#if 0
void DDukeCommonStatusBar::displayfragbar(void)
{
	short i, j;

	j = 0;

	for (i = connecthead; i >= 0; i = connectpoint2[i])
		if (i > j) j = i;

	auto tex = tileGetTexture(TILE_FRAGBAR);
	for (int y = 0; y < 32; y += 8)
		DrawTexture(twod, tex, 0, 0, DTA_FullscreenScale, 3, DTA_ViewportWidth, 320, DTA_ViewportHeight, 200, DTA_ScaleX, 1.001, DTA_ScaleY, 1.001, TAG_Done);
		
	for (i = connecthead; i >= 0; i = connectpoint2[i])
	{
		m initext(21 + (73 * (i & 3)), 2 + ((i & 28) << 1), &ud.user_name[i][0], sprite[ps[i].i].pal, 2 + 8 + 16 + 128);
		sprintf(tempbuf, "%d", ps[i].frag - ps[i].fraggedself);
		m initext(17 + 50 + (73 * (i & 3)), 2 + ((i & 28) << 1), tempbuf, sprite[ps[i].i].pal, 2 + 8 + 16 + 128);
	}
}
#endif
//==========================================================================
//
// Common inventory icon code for all styles
//
//==========================================================================

std::pair<const char*, EColorRange> DDukeCommonStatusBar::ontext(struct player_struct *p)
{
	std::pair<const char*, EColorRange> retval(nullptr, CR_RED);

	int onstate = 0x80000000;
	switch (p->inven_icon)
	{
	case ICON_HOLODUKE:
		onstate = p->holoduke_on;
	case ICON_JETPACK:
		onstate = p->jetpack_on;
	case ICON_HEATS:
		onstate = p->heat_on;
	}

	// Texts are intentionally not translated because the font is too small for making localization work and the translated words are too long.
	if ((unsigned)onstate != 0x80000000 && !(g_gameType & (GAMEFLAG_WW2GI|GAMEFLAG_RRALL)))
	{
		retval.second = onstate > 0 ? CR_LIGHTBLUE : CR_RED;
		retval.first = onstate > 0 ? "ON" : "OFF";
	}
	if (p->inven_icon >= ICON_SCUBA)
	{
		retval.second = CR_ORANGE;
		retval.first = "AUTO";
	}
	return retval;
}

//==========================================================================
//
// draws the inventory selector
//
//==========================================================================

void DDukeCommonStatusBar::DrawInventory(const struct player_struct* p, double x, double y, int align)
{
	if (p->invdisptime <= 0)return;

	int n = 0, j = 0;
	if (p->firstaid_amount > 0) n |= 1, j++;
	if (p->steroids_amount > 0) n |= 2, j++;
	if (p->holoduke_amount > 0) n |= 4, j++;
	if (p->jetpack_amount > 0) n |= 8, j++;
	if (p->heat_amount > 0) n |= 16, j++;
	if (p->scuba_amount > 0) n |= 32, j++;
	if (p->boot_amount > 0) n |= 64, j++;

	x -= (j * 11);
	y -= 6;

	; align |= DI_ITEM_CENTER;
	for(int bit = 0; bit < 7; bit++)
	{
		int i = 1 << bit;
		if (n & i)
		{
			int select = 1 << (p->inven_icon - 1);
			double alpha = select == i ? 1.0 : 0.7;
			DrawGraphic(tileGetTexture(item_icons[bit+1]), x, y, align, alpha, 0, 0, scale, scale);
			if (select == i) DrawGraphic(tileGetTexture(TILE_ARROW), x, y, align, alpha, 0, 0, scale, scale);
			x += 22;
		}
	}
}

//==========================================================================
//
// Helper
//
//==========================================================================

PalEntry DDukeCommonStatusBar::LightForShade(int shade)
{
	int ll = clamp((numshades - shade) * 255 / numshades, 0, 255);
	return PalEntry(255, ll, ll, ll);
}


//==========================================================================
//
// Statistics output
//
//==========================================================================

void DDukeCommonStatusBar::PrintLevelStats(int bottomy)
{
	// JBF 20040124: display level stats in screen corner
	if (ud.overhead_on != 2 && hud_stats)
	{
		FLevelStats stats{};
		auto pp = &ps[myconnectindex];

		stats.fontscale = isRR() ? 0.5 : 1.;
		stats.spacing = isRR() ? 10 : 7;
		stats.screenbottomspace = bottomy;

		stats.time = Scale(pp->player_par, 1000, REALGAMETICSPERSEC);
		stats.kills = pp->actors_killed;
		stats.maxkills = !isRR() && ud.player_skill > 3 ? -2 : pp->max_actors_killed;
		stats.frags = ud.multimode > 1 && !ud.coop ? pp->frag - pp->fraggedself : -1;
		stats.secrets = pp->secret_rooms;
		stats.maxsecrets = pp->max_secret_rooms;
		stats.font = SmallFont;
		if (isNamWW2GI())
		{
			// The stock font of these games is totally unusable for this.
			stats.font = ConFont;
			stats.spacing = ConFont->GetHeight() + 2;
			stats.letterColor = CR_ORANGE;
			stats.standardColor = CR_YELLOW;
			stats.completeColor = CR_FIRE;
		}
		else if (!isRR())
		{
			stats.letterColor = CR_ORANGE;
			stats.standardColor = CR_CREAM;
			stats.completeColor = CR_FIRE;
		}
		else
		{
			stats.letterColor = CR_ORANGE;
			stats.standardColor =
				stats.completeColor = CR_UNTRANSLATED;
		}
		DBaseStatusBar::PrintLevelStats(stats);
	}
}

//==========================================================================
//
// 3D viewport size management
//
//==========================================================================

void DrawBorder()
{
	auto tex = tileGetTexture(TILE_SCREENBORDER);
	if (tex != nullptr && tex->isValid())
	{
		if (windowxy1.y > 0)
		{
			twod->AddFlatFill(0, 0, twod->GetWidth(), windowxy1.y, tex, false, 1);
		}
		if (windowxy2.y + 1 < twod->GetHeight())
		{
			twod->AddFlatFill(0, windowxy2.y + 1, twod->GetWidth(), twod->GetHeight(), tex, false, 1);
		}
		if (windowxy1.x > 0)
		{
			twod->AddFlatFill(0, windowxy1.y, windowxy1.x, windowxy2.y + 1, tex, false, 1);
		}
		if (windowxy2.x + 1 < twod->GetWidth())
		{
			twod->AddFlatFill(windowxy2.x + 1, windowxy1.y, twod->GetWidth(), windowxy2.y + 1, tex, false, 1);
		}
		auto vb = tileGetTexture(TILE_VIEWBORDER);
		auto ve = tileGetTexture(TILE_VIEWBORDER + 1);
		int x1 = windowxy1.x - 4;
		int y1 = windowxy1.y - 4;
		int x2 = windowxy2.x + 5;
		int y2 = windowxy2.y + 5;
		twod->AddFlatFill(x1, y1, x2, y1 + 4, vb, 5);
		twod->AddFlatFill(x1, y2 - 4, x2, y2, vb, 6);
		twod->AddFlatFill(x1, y1, x1 + 4, y2, vb, 1);
		twod->AddFlatFill(x2 - 4, y1, x2, y2, vb, 3);
		twod->AddFlatFill(x1, y1, x1 + 4, y1 + 4, ve, 1);
		twod->AddFlatFill(x2 - 4, y1, x2, y1 + 4, ve, 3);
		twod->AddFlatFill(x1, y2 - 4, x1 + 4, y2, ve, 2);
		twod->AddFlatFill(x2 - 4, y2 - 4, x2, y2, ve, 4);
	}
	else
	{
		// If we got no frame just clear the screen.
		twod->ClearScreen();
	}
}

//---------------------------------------------------------------------------
//
// calculate size of 3D viewport.
// Fixme: this needs to be adjusted to the new status bar code, 
// once the status bar is a persistent queriable object
// (it should also be moved out of the game code then.
//
//---------------------------------------------------------------------------

void updateviewport(void)
{
	ud.screen_size = clamp(ud.screen_size, 0, 64);
	int ss = std::max(ud.screen_size - 8, 0);

	int x1 = scale(ss, xdim, 160);
	int x2 = xdim - x1;

	int y1 = scale(ss, (200 * 100) - ((tilesiz[TILE_BOTTOMSTATUSBAR].y >> (isRR() ? 1 : 0)) * ud.statusbarscale), 200 - tilesiz[TILE_BOTTOMSTATUSBAR].y);
	int y2 = 200 * 100 - y1;

	if (isRR() && ud.screen_size <= 12)
	{
		x1 = 0;
		x2 = xdim;
		y1 = 0;
		if (ud.statusbarmode)
			y2 = 200 * 100;
	}

	int fbh = 0;
	if (ud.screen_size > 0 && ud.coop != 1 && ud.multimode > 1)
	{
		int j = 0;
		for (int i = connecthead; i >= 0; i = connectpoint2[i])
			if (i > j) j = i;

		if (j >= 1) fbh += 8;
		if (j >= 4) fbh += 8;
		if (j >= 8) fbh += 8;
		if (j >= 12) fbh += 8;
	}

	y1 += fbh * 100;
	if (ud.screen_size >= 8 && ud.statusbarmode == 0)
		y2 -= (tilesiz[TILE_BOTTOMSTATUSBAR].y >> (isRR() ? 1 : 0)) * ud.statusbarscale;
	y1 = scale(y1, ydim, 200 * 100);
	y2 = scale(y2, ydim, 200 * 100);

	videoSetViewableArea(x1, y1, x2 - 1, y2 - 1);
}

//==========================================================================
//
// view sizing game interface
//
//==========================================================================

bool GameInterface::validate_hud(int layout)
{
	return layout <= 11;
}

void GameInterface::set_hud_layout(int layout)
{
	static const uint8_t screen_size_vals[] = { 60, 54, 48, 40, 32, 24, 16, 8, 8, 4, 4, 0 };
	static const uint8_t screen_size_vals_rr[] = { 56, 48, 40, 32, 24, 16, 12, 8, 8, 4, 4, 0 };
	if (validate_hud(layout))
	{
		ud.screen_size = isRR()? screen_size_vals_rr[layout] : screen_size_vals[layout];
		ud.statusbarmode = layout >= 8;
		ud.althud = layout >= 10;
		if (xdim > 0 && ydim > 0) updateviewport();
	}
}

void GameInterface::set_hud_scale(int scale)
{
    ud.statusbarscale = clamp(scale, 36, 100);
	if (xdim > 0 && ydim > 0) updateviewport();
}

void GameInterface::UpdateScreenSize()
{
	updateviewport();
}



END_DUKE_NS
