//-------------------------------------------------------------------------
/*
Copyright (C) 1996, 2003 - 3D Realms Entertainment
Copyright (C) 2020 - Christoph Oelckers

This file is part of Duke Nukem 3D version 1.5 - Atomic Edition

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
Modifications for JonoF's port by Jonathon Fowler (jf@jonof.id.au)
*/
//------------------------------------------------------------------------- 

#include "automap.h"
#include "c_dispatch.h"
#include "c_cvars.h"
#include "gstrings.h"
#include "printf.h"
#include "serializer.h"
#include "v_2ddrawer.h"
#include "earcut.hpp"
#include "buildtiles.h"
#include "d_event.h"
#include "c_bind.h"
#include "gamestate.h"
#include "gamecontrol.h"
#include "quotemgr.h"
#include "v_video.h"
#include "gamestruct.h"
#include "v_draw.h"
#include "sectorgeometry.h"
#include "gamefuncs.h"
#include "hw_sections.h"

CVAR(Bool, am_followplayer, true, CVAR_ARCHIVE)
CVAR(Bool, am_rotate, true, CVAR_ARCHIVE)
CVAR(Float, am_linealpha, 1.0f, CVAR_ARCHIVE)
CVAR(Int, am_linethickness, 1, CVAR_ARCHIVE)
CVAR(Bool, am_textfont, false, CVAR_ARCHIVE)
CVAR(Bool, am_showlabel, false, CVAR_ARCHIVE)
CVAR(Bool, am_nameontop, false, CVAR_ARCHIVE)

int automapMode;
static float am_zoomdir;
int follow_x = INT_MAX, follow_y = INT_MAX, follow_a = INT_MAX;
static int gZoom = 768;
bool automapping;
bool gFullMap;
FixedBitArray<MAXSECTORS> show2dsector;
FixedBitArray<MAXWALLS> show2dwall;
static int x_min_bound = INT_MAX, y_min_bound, x_max_bound, y_max_bound;

CVAR(Color, am_twosidedcolor, 0xaaaaaa, CVAR_ARCHIVE)
CVAR(Color, am_onesidedcolor, 0xaaaaaa, CVAR_ARCHIVE)
CVAR(Color, am_playercolor, 0xaaaaaa, CVAR_ARCHIVE)
CVAR(Color, am_ovtwosidedcolor, 0xaaaaaa, CVAR_ARCHIVE)
CVAR(Color, am_ovonesidedcolor, 0xaaaaaa, CVAR_ARCHIVE)
CVAR(Color, am_ovplayercolor, 0xaaaaaa, CVAR_ARCHIVE)

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

CCMD(allmap)
{
	if (!CheckCheatmode(true, false))
	{
		gFullMap = !gFullMap;
		Printf("%s\n", GStrings(gFullMap ? "SHOW MAP: ON" : "SHOW MAP: OFF"));
	}
}

CCMD(togglemap)
{
	if (gamestate == GS_LEVEL)
	{
		automapMode++;
		if (automapMode == am_count) automapMode = am_off;
		if (isBlood() && automapMode == am_overlay) automapMode = am_full; // todo: investigate if this can be re-enabled
	}
}

CCMD(togglefollow)
{
	am_followplayer = !am_followplayer;
	auto msg = quoteMgr.GetQuote(am_followplayer ? 84 : 83);
	if (!msg || !*msg) msg = am_followplayer ? GStrings("FOLLOW MODE ON") : GStrings("FOLLOW MODE Off");
	Printf(PRINT_NOTIFY, "%s\n", msg);
	if (am_followplayer) follow_x = INT_MAX;
}

CCMD(togglerotate)
{
	am_rotate = !am_rotate;
	auto msg = am_rotate ? GStrings("TXT_ROTATE_ON") : GStrings("TXT_ROTATE_OFF");
	Printf(PRINT_NOTIFY, "%s\n", msg);
}


CCMD(am_zoom)
{
	if (argv.argc() >= 2)
	{
		am_zoomdir = (float)atof(argv[1]);
	}
}

//==========================================================================
//
// AM_Responder
// Handle automap exclusive bindings.
//
//==========================================================================

bool AM_Responder(event_t* ev, bool last)
{
	if (ev->type == EV_KeyDown || ev->type == EV_KeyUp)
	{
		if (am_followplayer)
		{
			// check for am_pan* and ignore in follow mode
			const char* defbind = AutomapBindings.GetBind(ev->data1);
			if (defbind && !strnicmp(defbind, "+am_pan", 7)) return false;
		}

		bool res = C_DoKey(ev, &AutomapBindings, nullptr);
		if (res && ev->type == EV_KeyUp && !last)
		{
			// If this is a release event we also need to check if it released a button in the main Bindings
			// so that that button does not get stuck.
			const char* defbind = Bindings.GetBind(ev->data1);
			return (!defbind || defbind[0] != '+'); // Let G_Responder handle button releases
		}
		return res;
	}
	return false;
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

static void CalcMapBounds()
{
	x_min_bound = INT_MAX;
	y_min_bound = INT_MAX;
	x_max_bound = INT_MIN;
	y_max_bound = INT_MIN;


	for (int i = 0; i < numwalls; i++)
	{
		// get map min and max coordinates
		if (wall[i].x < x_min_bound) x_min_bound = wall[i].x;
		if (wall[i].y < y_min_bound) y_min_bound = wall[i].y;
		if (wall[i].x > x_max_bound) x_max_bound = wall[i].x;
		if (wall[i].y > y_max_bound) y_max_bound = wall[i].y;
	}
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void AutomapControl()
{
	static int nonsharedtimer;
	int ms = (int)screen->FrameTime;
	int interval;
	int panvert = 0, panhorz = 0;

	if (nonsharedtimer > 0 || ms < nonsharedtimer)
	{
		interval = ms - nonsharedtimer;
	}
	else
	{
		interval = 0;
	}
	nonsharedtimer = ms;

	if (System_WantGuiCapture())
		return;

	if (automapMode != am_off)
	{
		const int keymove = 4;
		if (am_zoomdir > 0)
		{
			gZoom = xs_CRoundToInt(gZoom * am_zoomdir);
		}
		else if (am_zoomdir < 0)
		{
			gZoom = xs_CRoundToInt(gZoom / -am_zoomdir);
		}
		am_zoomdir = 0;

		double j = interval * 35. / gZoom;

		if (buttonMap.ButtonDown(gamefunc_Enlarge_Screen))
			gZoom += (int)MulScaleF(j, max(gZoom, 256), 6);
		if (buttonMap.ButtonDown(gamefunc_Shrink_Screen))
			gZoom -= (int)MulScaleF(j, max(gZoom, 256), 6);

		gZoom = clamp(gZoom, 48, 2048);

		if (!am_followplayer)
		{
			if (buttonMap.ButtonDown(gamefunc_AM_PanLeft))
				panhorz += keymove;

			if (buttonMap.ButtonDown(gamefunc_AM_PanRight))
				panhorz -= keymove;

			if (buttonMap.ButtonDown(gamefunc_AM_PanUp))
				panvert += keymove;

			if (buttonMap.ButtonDown(gamefunc_AM_PanDown))
				panvert -= keymove;

			int momx = MulScale(panvert, bcos(follow_a), 9);
			int momy = MulScale(panvert, bsin(follow_a), 9);

			momx += MulScale(panhorz, bsin(follow_a), 9);
			momy += MulScale(panhorz, -bcos(follow_a), 9);

			follow_x += int(momx * j);
			follow_y += int(momy * j);

			if (x_min_bound == INT_MAX) CalcMapBounds();
			follow_x = clamp(follow_x, x_min_bound, x_max_bound);
			follow_y = clamp(follow_y, y_min_bound, y_max_bound);
		}
	}
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void SerializeAutomap(FSerializer& arc)
{
	if (arc.BeginObject("automap"))
	{
		arc("automapping", automapping)
			("fullmap", gFullMap)
			// Only store what's needed. Unfortunately for sprites it is not that easy
			.SerializeMemory("mappedsectors", show2dsector.Storage(), (numsectors + 7) / 8)
			.SerializeMemory("mappedwalls", show2dwall.Storage(), (numwalls + 7) / 8)
			.EndObject();
	}
}


//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void ClearAutomap()
{
	show2dsector.Zero();
	show2dwall.Zero();
	x_min_bound = INT_MAX;
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void MarkSectorSeen(int i)
{
	if (i >= 0) 
	{
		show2dsector.Set(i);
		auto wal = &wall[sector[i].wallptr];
		for (int j = sector[i].wallnum; j > 0; j--, wal++)
		{
			i = wal->nextsector;
			if (i < 0) continue;
			if (wal->cstat & 0x0071) continue;
			if (wall[wal->nextwall].cstat & 0x0071) continue;
			if (sector[i].lotag == 32767) continue;
			if (sector[i].ceilingz >= sector[i].floorz) continue;
			show2dsector.Set(i);
		}
	}
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void drawlinergb(int32_t x1, int32_t y1, int32_t x2, int32_t y2, PalEntry p)
{
	if (am_linethickness >= 2) {
		twod->AddThickLine(x1 / 4096, y1 / 4096, x2 / 4096, y2 / 4096, am_linethickness, p, uint8_t(am_linealpha * 255));
	} else {
		// Use more efficient thin line drawing routine.
		twod->AddLine(x1 / 4096.f, y1 / 4096.f, x2 / 4096.f, y2 / 4096.f, windowxy1.x, windowxy1.y, windowxy2.x, windowxy2.y, p, uint8_t(am_linealpha * 255));
	}
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

PalEntry RedLineColor()
{
	// todo:
	// Blood uses palette index 12 (99,99,99)
	// Exhumed uses palette index 111 (roughly 170,170,170) but darkens the line in overlay mode the farther it is away from the player in vertical direction.
	// Shadow Warrior uses palette index 152 in overlay mode and index 12 in full map mode. (152: 84, 88, 40)
	return automapMode == am_overlay? *am_ovtwosidedcolor : *am_twosidedcolor;
}

PalEntry WhiteLineColor()
{

	// todo:
	// Blood uses palette index 24
	// Exhumed uses palette index 111 (roughly 170,170,170) but darkens the line in overlay mode the farther it is away from the player in vertical direction.
	// Shadow Warrior uses palette index 24 (60,60,60)
	return automapMode == am_overlay ? *am_ovonesidedcolor : *am_onesidedcolor;
}

PalEntry PlayerLineColor()
{
	return automapMode == am_overlay ? *am_ovplayercolor : *am_playercolor;
}


CCMD(printpalcol)
{
	if (argv.argc() < 2) return;

	int i = atoi(argv[1]);
	Printf("%d, %d, %d\n", GPalette.BaseColors[i].r, GPalette.BaseColors[i].g, GPalette.BaseColors[i].b);
}
//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

bool ShowRedLine(int j, int i)
{
	auto wal = &wall[j];
	if (!isSWALL())
	{
		return !gFullMap && !show2dsector[wal->nextsector];
	}
	else
	{
		if (!gFullMap)
		{
			if (!show2dwall[j]) return false;
			int k = wal->nextwall;
			if (k > j && !show2dwall[k]) return false; //???
		}
		if (automapMode == am_full)
		{
			if (sector[i].floorz != sector[i].ceilingz)
				if (sector[wal->nextsector].floorz != sector[wal->nextsector].ceilingz)
					if (((wal->cstat | wall[wal->nextwall].cstat) & (16 + 32)) == 0)
						if (sector[i].floorz == sector[wal->nextsector].floorz)
							return false;
			if (sector[i].floorpicnum != sector[wal->nextsector].floorpicnum)
				return false;
			if (sector[i].floorshade != sector[wal->nextsector].floorshade)
				return false;
		}
		return true;
	}
}

//---------------------------------------------------------------------------
//
// two sided lines
//
//---------------------------------------------------------------------------

void drawredlines(int cposx, int cposy, int czoom, int cang)
{
	int xvect = -bsin(cang) * czoom;
	int yvect = -bcos(cang) * czoom;
	int width = screen->GetWidth();
	int height = screen->GetHeight();

	for (int i = 0; i < numsectors; i++)
	{
		if (!gFullMap && !show2dsector[i]) continue;

		int startwall = sector[i].wallptr;
		int endwall = sector[i].wallptr + sector[i].wallnum;

		int z1 = sector[i].ceilingz;
		int z2 = sector[i].floorz;
		walltype* wal;
		int j;

		for (j = startwall, wal = &wall[startwall]; j < endwall; j++, wal++)
		{
			int k = wal->nextwall;
			if (k < 0 || k >= numwalls) continue;

			int s = wal->nextsector;
			if (s < 0 || s >= numsectors) continue;

			if (sector[s].ceilingz == z1 && sector[s].floorz == z2)
				if (((wal->cstat | wall[wal->nextwall].cstat) & (16 + 32)) == 0) continue;

			if (ShowRedLine(j, i))
			{
				int ox = wal->x - cposx;
				int oy = wal->y - cposy;
				int x1 = DMulScale(ox, xvect, -oy, yvect, 16) + (width << 11);
				int y1 = DMulScale(oy, xvect, ox, yvect, 16) + (height << 11);

				auto wal2 = &wall[wal->point2];
				ox = wal2->x - cposx;
				oy = wal2->y - cposy;
				int x2 = DMulScale(ox, xvect, -oy, yvect, 16) + (width << 11);
				int y2 = DMulScale(oy, xvect, ox, yvect, 16) + (height << 11);

				drawlinergb(x1, y1, x2, y2, RedLineColor());
			}
		}
	}
}

//---------------------------------------------------------------------------
//
// one sided lines
//
//---------------------------------------------------------------------------

static void drawwhitelines(int cposx, int cposy, int czoom, int cang)
{
	int xvect = -bsin(cang) * czoom;
	int yvect = -bcos(cang) * czoom;
	int width = screen->GetWidth();
	int height = screen->GetHeight();

	for (int i = numsectors - 1; i >= 0; i--)
	{
		if (!gFullMap && !show2dsector[i] && !isSWALL()) continue;

		int startwall = sector[i].wallptr;
		int endwall = sector[i].wallptr + sector[i].wallnum;

		walltype* wal;
		int j;

		for (j = startwall, wal = &wall[startwall]; j < endwall; j++, wal++)
		{
			if (wal->nextwall >= 0) continue;
			if (!gFullMap && !tileGetTexture(wal->picnum)->isValid()) continue;

			if (isSWALL() && !gFullMap && !show2dwall[j])
				continue;

			int ox = wal->x - cposx;
			int oy = wal->y - cposy;
			int x1 = DMulScale(ox, xvect, -oy, yvect, 16) + (width << 11);
			int y1 = DMulScale(oy, xvect, ox, yvect, 16) + (height << 11);

			int k = wal->point2;
			auto wal2 = &wall[k];
			ox = wal2->x - cposx;
			oy = wal2->y - cposy;
			int x2 = DMulScale(ox, xvect, -oy, yvect, 16) + (width << 11);
			int y2 = DMulScale(oy, xvect, ox, yvect, 16) + (height << 11);

			drawlinergb(x1, y1, x2, y2, WhiteLineColor());
		}
	}
}

//---------------------------------------------------------------------------
//
// player sprite fallback
//
//---------------------------------------------------------------------------

void DrawPlayerArrow(int cposx, int cposy, int cang, int pl_x, int pl_y, int zoom, int pl_angle)
{
	int arrow[] =
	{
		0, 65536, 0, -65536,
		0, 65536, -32768, 32878,
		0, 65536, 32768, 32878,
	};

	int xvect = -bsin(cang) * zoom;
	int yvect = -bcos(cang) * zoom;

	int pxvect = -bsin(pl_angle);
	int pyvect = -bcos(pl_angle);

	int width = screen->GetWidth();
	int height = screen->GetHeight();

	for (int i = 0; i < 12; i += 4)
	{

		int px1 = DMulScale(arrow[i], pxvect, -arrow[i+1], pyvect, 16);
		int py1 = DMulScale(arrow[i+1], pxvect, arrow[i], pyvect, 16) + (height << 11);
		int px2 = DMulScale(arrow[i+2], pxvect, -arrow[i + 3], pyvect, 16);
		int py2 = DMulScale(arrow[i + 3], pxvect, arrow[i+2], pyvect, 16) + (height << 11);

		int ox1 = px1 - cposx;
		int oy1 = py1 - cposy;
		int ox2 = px2 - cposx;
		int oy2 = py2 - cposy;

		int sx1 = DMulScale(ox1, xvect, -oy1, yvect, 16) + (width << 11);
		int sy1 = DMulScale(oy1, xvect, ox1, yvect, 16) + (height << 11);
		int sx2 = DMulScale(ox2, xvect, -oy2, yvect, 16) + (width << 11);
		int sy2 = DMulScale(oy2, xvect, ox2, yvect, 16) + (height << 11);

		drawlinergb(sx1, sy1, sx2, sy2, WhiteLineColor());
	}
}


//---------------------------------------------------------------------------
//
// floor textures
//
//---------------------------------------------------------------------------

void renderDrawMapView(int cposx, int cposy, int czoom, int cang)
{
	int xvect = -bsin(cang) * czoom;
	int yvect = -bcos(cang) * czoom;
	int width = screen->GetWidth();
	int height = screen->GetHeight();
	TArray<FVector4> vertices;
	TArray<int> floorsprites;


	for (int i = numsectors - 1; i >= 0; i--)
	{
		if (!gFullMap && !show2dsector[i]) continue;

		//Collect floor sprites to draw
		SectIterator it(i);
		int s;
		while ((s = it.NextIndex()) >= 0)
		{
			if (sprite[s].cstat & CSTAT_SPRITE_INVISIBLE)
				continue;

			if ((sprite[s].cstat & CSTAT_SPRITE_ALIGNMENT_MASK) == CSTAT_SPRITE_ALIGNMENT_FLOOR)
			{
				if ((sprite[s].cstat & (CSTAT_SPRITE_ONE_SIDED | CSTAT_SPRITE_YFLIP)) == (CSTAT_SPRITE_ONE_SIDED | CSTAT_SPRITE_YFLIP))
					continue; // upside down
				floorsprites.Push(s);
			}
		}

		if (sector[i].floorstat & CSTAT_SECTOR_SKY) continue;

		int picnum = sector[i].floorpicnum;
		if ((unsigned)picnum >= (unsigned)MAXTILES) continue;

		for (auto ii : sectionspersector[i])
		{
			auto mesh = sectorGeometry.get(ii, 0, { 0.f,0.f });
			vertices.Resize(mesh->vertices.Size());
			for (unsigned j = 0; j < mesh->vertices.Size(); j++)
			{
				int ox = int(mesh->vertices[j].X * 16.f) - cposx;
				int oy = int(mesh->vertices[j].Y * -16.f) - cposy;
				int x1 = DMulScale(ox, xvect, -oy, yvect, 16) + (width << 11);
				int y1 = DMulScale(oy, xvect, ox, yvect, 16) + (height << 11);
				vertices[j] = { x1 / 4096.f, y1 / 4096.f, mesh->texcoords[j].X, mesh->texcoords[j].Y };
			}
		}

		int translation = TRANSLATION(Translation_Remap + curbasepal, sector[i].floorpal);
		setgotpic(picnum);
		twod->AddPoly(tileGetTexture(picnum, true), vertices.Data(), vertices.Size(), nullptr, 0, translation, shadeToLight(sector[i].floorshade), 
			LegacyRenderStyles[STYLE_Translucent], windowxy1.x, windowxy1.y, windowxy2.x + 1, windowxy2.y + 1);


	}
	qsort(floorsprites.Data(), floorsprites.Size(), sizeof(int), [](const void* a, const void* b)
		{
			int A = *(int*)a;
			int B = *(int*)b;
			if (sprite[A].z != sprite[B].z) return sprite[B].z - sprite[A].z;
			return A - B; // ensures stable sort.
		});

	vertices.Resize(4);
	for (auto sn : floorsprites)
	{
		if (!gFullMap && !(sprite[sn].cstat2 & CSTAT2_SPRITE_MAPPED)) continue;
		auto spr = &sprite[sn];
		vec2_t pp[4];
		GetFlatSpritePosition(spr, spr->pos.vec2, pp, true);

		for (unsigned j = 0; j < 4; j++)
		{
			int ox = pp[j].x - cposx;
			int oy = pp[j].y - cposy;
			int x1 = DMulScale(ox, xvect, -oy, yvect, 16) + (width << 11);
			int y1 = DMulScale(oy, xvect, ox, yvect, 16) + (height << 11);
			vertices[j] = { x1 / 4096.f, y1 / 4096.f, j == 1 || j == 2 ? 1.f : 0.f, j == 2 || j == 3 ? 1.f : 0.f };
		}
		int shade;
		if ((sector[spr->sectnum].ceilingstat & CSTAT_SECTOR_SKY)) shade = sector[spr->sectnum].ceilingshade;
		else shade = sector[spr->sectnum].floorshade;
		shade += spr->shade;
		PalEntry color = shadeToLight(shade);
		FRenderStyle rs = LegacyRenderStyles[STYLE_Translucent];
		float alpha = 1;
		if (spr->cstat & CSTAT_SPRITE_TRANSLUCENT)
		{
			rs = GetRenderStyle(0, !!(spr->cstat & CSTAT_SPRITE_TRANSLUCENT_INVERT));
			alpha = GetAlphaFromBlend((spr->cstat & CSTAT_SPRITE_TRANSLUCENT_INVERT) ? DAMETH_TRANS2 : DAMETH_TRANS1, 0);
			color.a = uint8_t(alpha * 255);
		}

		int translation = TRANSLATION(Translation_Remap + curbasepal, spr->pal);
		int picnum = spr->picnum;
		setgotpic(picnum);
		const static unsigned indices[] = { 0, 1, 2, 0, 2, 3 };
		twod->AddPoly(tileGetTexture(picnum, true), vertices.Data(), vertices.Size(), indices, 6, translation, color, rs,
			windowxy1.x, windowxy1.y, windowxy2.x + 1, windowxy2.y + 1);
	}
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void DrawOverheadMap(int pl_x, int pl_y, int pl_angle, double const smoothratio)
{
	if (am_followplayer || follow_x == INT_MAX)
	{
		follow_x = pl_x;
		follow_y = pl_y;
	}
	int x = follow_x;
	int y = follow_y;
	follow_a = am_rotate ? pl_angle : 0;
	AutomapControl();

	if (automapMode == am_full)
	{
		twod->ClearScreen();
		renderDrawMapView(x, y, gZoom, follow_a);
	}
	drawredlines(x, y, gZoom, follow_a);
	drawwhitelines(x, y, gZoom, follow_a);
	if (!gi->DrawAutomapPlayer(pl_x, pl_y, x, y, gZoom, follow_a, smoothratio))
		DrawPlayerArrow(x, y, follow_a, pl_x, pl_y, gZoom, -pl_angle);

}

