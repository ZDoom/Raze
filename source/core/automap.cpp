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
#include "coreactor.h"
CVAR(Bool, am_followplayer, true, CVAR_ARCHIVE)
CVAR(Bool, am_rotate, true, CVAR_ARCHIVE)
CVAR(Float, am_linealpha, 1.0f, CVAR_ARCHIVE)
CVAR(Int, am_linethickness, 1, CVAR_ARCHIVE)
CVAR(Bool, am_textfont, false, CVAR_ARCHIVE)
CVAR(Bool, am_showlabel, false, CVAR_ARCHIVE)
CVAR(Bool, am_nameontop, false, CVAR_ARCHIVE)

int automapMode;
static float am_zoomdir;
double follow_x = INT_MAX, follow_y = INT_MAX;
DAngle follow_a = DAngle::fromDeg(INT_MAX);
static double gZoom = 768;
bool automapping;
bool gFullMap;
BitArray show2dsector;
BitArray show2dwall;
static double x_min_bound = INT_MAX, y_min_bound, x_max_bound, y_max_bound;

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


	for(auto& wal : wall)
	{
		// get map min and max coordinates
		if (wal.pos.X < x_min_bound) x_min_bound = wal.pos.X;
		if (wal.pos.Y < y_min_bound) y_min_bound = wal.pos.Y;
		if (wal.pos.X > x_max_bound) x_max_bound = wal.pos.X;
		if (wal.pos.Y > y_max_bound) y_max_bound = wal.pos.Y;
	}
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void AutomapControl()
{
	static double nonsharedtimer;
	double ms = screen->FrameTime;
	double interval;
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
			gZoom = gZoom * am_zoomdir;
		}
		else if (am_zoomdir < 0)
		{
			gZoom = gZoom / -am_zoomdir;
		}
		am_zoomdir = 0;

		double j = interval * 35. / gZoom;

		if (buttonMap.ButtonDown(gamefunc_Enlarge_Screen))
			gZoom += MulScaleF(j, max(gZoom, 256.), 6);
		if (buttonMap.ButtonDown(gamefunc_Shrink_Screen))
			gZoom -= MulScaleF(j, max(gZoom, 256.), 6);

		gZoom = clamp(gZoom, 48., 2048.);

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

			auto fcos = follow_a.Cos();
			auto fsin = follow_a.Sin();
			auto momx = (panvert * fcos * 8) + (panhorz * fsin * 8);
			auto momy = (panvert * fsin * 8) - (panhorz * fcos * 8);

			follow_x += momx * j;
			follow_y += momy * j;

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
			("mappedsectors", show2dsector)
			("mappedwalls", show2dwall)
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

void MarkSectorSeen(sectortype* sec)
{
	if (sec) 
	{
		show2dsector.Set(sectnum(sec));
		for (auto& wal : wallsofsector(sec))
		{
			if (!wal.twoSided()) continue;
			const auto bits = (CSTAT_WALL_BLOCK | CSTAT_WALL_MASKED | CSTAT_WALL_1WAY | CSTAT_WALL_BLOCK_HITSCAN);
			if (wal.cstat & bits) continue;
			if (wal.nextWall()->cstat & bits) continue;
			auto osec = wal.nextSector();
			if (osec->lotag == 32767) continue;
			if (osec->ceilingz >= osec->floorz) continue;
			show2dsector.Set(sectnum(osec));
		}
	}
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void drawlinergb(const double x1, const double y1, const double x2, const double y2, PalEntry p)
{
	if (am_linethickness >= 2) {
		twod->AddThickLine(x1, y1, x2, y2, am_linethickness, p, uint8_t(am_linealpha * 255));
	} else {
		// Use more efficient thin line drawing routine.
		twod->AddLine(x1, y1, x2, y2, &viewport3d, p, uint8_t(am_linealpha * 255));
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
				if (wal->nextSector()->floorz != wal->nextSector()->ceilingz)
					if (((wal->cstat | wal->nextWall()->cstat) & (CSTAT_WALL_MASKED | CSTAT_WALL_1WAY)) == 0)
						if (sector[i].floorz == wal->nextSector()->floorz)
							return false;
			if (sector[i].floorpicnum != wal->nextSector()->floorpicnum)
				return false;
			if (sector[i].floorshade != wal->nextSector()->floorshade)
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

static void drawredlines(const DVector2& cpos, const double czoom, const DAngle cang)
{
	double xvect = -cang.Sin() * czoom * (1. / 1024.);
	double yvect = -cang.Cos() * czoom * (1. / 1024.);
	int width = screen->GetWidth();
	int height = screen->GetHeight();

	for (unsigned i = 0; i < sector.Size(); i++)
	{
		if (!gFullMap && !show2dsector[i]) continue;

		double z1 = sector[i].ceilingz;
		double z2 = sector[i].floorz;

		for (auto& wal : wallsofsector(i))
		{
			if (!wal.twoSided()) continue;

			auto osec = wal.nextSector();

			if (osec->ceilingz == z1 && osec->floorz == z2)
				if (((wal.cstat | wal.nextWall()->cstat) & (CSTAT_WALL_MASKED | CSTAT_WALL_1WAY)) == 0) continue;

			if (ShowRedLine(wallnum(&wal), i))
			{
				auto oxy1 = wal.pos - cpos;
				double x1 = (oxy1.X * xvect) - (oxy1.Y * yvect) + (width * 0.5);
				double y1 = (oxy1.Y * xvect) + (oxy1.X * yvect) + (height * 0.5);

				auto wal2 = wal.point2Wall();
				auto oxy2 = wal2->pos - cpos;
				double x2 = (oxy2.X * xvect) - (oxy2.Y * yvect) + (width * 0.5);
				double y2 = (oxy2.Y * xvect) + (oxy2.X * yvect) + (height * 0.5);

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

static void drawwhitelines(const DVector2& cpos, const double czoom, const DAngle cang)
{
	double xvect = -cang.Sin() * czoom * (1. / 1024);
	double yvect = -cang.Cos() * czoom * (1. / 1024);
	int width = screen->GetWidth();
	int height = screen->GetHeight();

	for (int i = (int)sector.Size() - 1; i >= 0; i--)
	{
		if (!gFullMap && !show2dsector[i] && !isSWALL()) continue;

		for (auto& wal : wallsofsector(i))
		{
			if (wal.nextwall >= 0) continue;
			if (!gFullMap && !tileGetTexture(wal.picnum)->isValid()) continue;

			if (isSWALL() && !gFullMap && !show2dwall[wallnum(&wal)])
				continue;

			auto oxy1 = wal.pos - cpos;
			double x1 = (oxy1.X * xvect) - (oxy1.Y * yvect) + (width * 0.5);
			double y1 = (oxy1.Y * xvect) + (oxy1.X * yvect) + (height * 0.5);

			auto wal2 = wal.point2Wall();
			auto oxy2 = wal2->pos - cpos;
			double x2 = (oxy2.X * xvect) - (oxy2.Y * yvect) + (width * 0.5);
			double y2 = (oxy2.Y * xvect) + (oxy2.X * yvect) + (height * 0.5);

			drawlinergb(x1, y1, x2, y2, WhiteLineColor());
		}
	}
}

//---------------------------------------------------------------------------
//
// player sprite fallback
//
//---------------------------------------------------------------------------

static void DrawPlayerArrow(const DVector2& cpos, const DAngle cang, const double czoom, const DAngle pl_angle)
{
	static constexpr int arrow[] =
	{
		0, 65536, 0, -65536,
		0, 65536, -32768, 32878,
		0, 65536, 32768, 32878,
	};

	double xvect = -cang.Sin() * czoom * (1. / 1024);
	double yvect = -cang.Cos() * czoom * (1. / 1024);

	double pxvect = -pl_angle.Sin();
	double pyvect = -pl_angle.Cos();

	int width = screen->GetWidth();
	int height = screen->GetHeight();

	for (int i = 0; i < 12; i += 4)
	{
		double px1 = (arrow[i] * pxvect) - (arrow[i+1] * pyvect);
		double py1 = (arrow[i+1] * pxvect) + (arrow[i] * pyvect) + (height * 0.5);
		double px2 = (arrow[i+2] * pxvect) - (arrow[i+3] * pyvect);
		double py2 = (arrow[i+3] * pxvect) + (arrow[i+2] * pyvect) + (height * 0.5);

		auto oxy1 = DVector2(px1, py1) - cpos;
		auto oxy2 = DVector2(px2, py2) - cpos;

		double sx1 = (oxy1.X * xvect) - (oxy1.Y * yvect) + (width * 0.5);
		double sy1 = (oxy1.Y * xvect) + (oxy1.X * yvect) + (height * 0.5);
		double sx2 = (oxy2.X * xvect) - (oxy2.Y * yvect) + (width * 0.5);
		double sy2 = (oxy2.Y * xvect) + (oxy2.X * yvect) + (height * 0.5);

		drawlinergb(sx1, sy1, sx2, sy2, WhiteLineColor());
	}
}


//---------------------------------------------------------------------------
//
// floor textures
//
//---------------------------------------------------------------------------

static void renderDrawMapView(const DVector2& cpos, const double czoom, const DAngle cang)
{
	double xvect = -cang.Sin() * czoom * (1. / 1024.);
	double yvect = -cang.Cos() * czoom * (1. / 1024.);
	int width = screen->GetWidth();
	int height = screen->GetHeight();
	TArray<FVector4> vertices;
	TArray<DCoreActor*> floorsprites;

	for (int i = (int)sector.Size() - 1; i >= 0; i--)
	{
		auto sect = &sector[i];
		if (!gFullMap && !show2dsector[i]) continue;

		//Collect floor sprites to draw
		TSectIterator<DCoreActor> it(sect);
		while (auto act = it.Next())
		{
			if (act->spr.cstat & CSTAT_SPRITE_INVISIBLE)
				continue;

			if (act->spr.cstat & CSTAT_SPRITE_ALIGNMENT_FLOOR)	// floor and slope sprites
			{
				if ((act->spr.cstat & (CSTAT_SPRITE_ONE_SIDE | CSTAT_SPRITE_YFLIP)) == (CSTAT_SPRITE_ONE_SIDE | CSTAT_SPRITE_YFLIP))
					continue; // upside down
				floorsprites.Push(act);
			}
		}

		if (sect->floorstat & CSTAT_SECTOR_SKY) continue;

		int picnum = sect->floorpicnum;
		if ((unsigned)picnum >= (unsigned)MAXTILES) continue;

		int translation = TRANSLATION(Translation_Remap + curbasepal, sector[i].floorpal);
		PalEntry light = shadeToLight(sector[i].floorshade);
		gotpic.Set(picnum);

		for (auto section : sectionsPerSector[i])
		{
			TArray<int>* indices;
			auto mesh = sectionGeometry.get(&sections[section], 0, { 0.f, 0.f }, &indices);
			vertices.Resize(mesh->vertices.Size());
			for (unsigned j = 0; j < mesh->vertices.Size(); j++)
			{
				auto oxy = DVector2(mesh->vertices[j].X - cpos.X, -mesh->vertices[j].Y - cpos.Y);
				float x1 = (oxy.X * xvect) - (oxy.Y * yvect) + (width * 0.5);
				float y1 = (oxy.Y * xvect) + (oxy.X * yvect) + (height * 0.5);
				vertices[j] = { x1, y1, mesh->texcoords[j].X, mesh->texcoords[j].Y };
			}

			twod->AddPoly(tileGetTexture(picnum, true), vertices.Data(), vertices.Size(), (unsigned*)indices->Data(), indices->Size(), translation, light,
				LegacyRenderStyles[STYLE_Translucent], &viewport3d);
		}
	}
	qsort(floorsprites.Data(), floorsprites.Size(), sizeof(DCoreActor*), [](const void* a, const void* b)
		{
			auto A = *(DCoreActor**)a;
			auto B = *(DCoreActor**)b;
			if (A->spr.pos.Z < B->spr.pos.Z) return 1;
			if (A->spr.pos.Z > B->spr.pos.Z) return -1;
			return A->time - B->time; // ensures stable sort.
		});

	vertices.Resize(4);
	for (auto actor : floorsprites)
	{
		if (!gFullMap && !(actor->spr.cstat2 & CSTAT2_SPRITE_MAPPED)) continue;
		DVector2 pp[4];
		GetFlatSpritePosition(actor, actor->spr.pos.XY(), pp, true);

		for (unsigned j = 0; j < 4; j++)
		{
			auto oxy = pp[j] - cpos;
			float x1 = (oxy.X * xvect) - (oxy.Y * yvect) + (width * 0.5);
			float y1 = (oxy.Y * xvect) + (oxy.X * yvect) + (height * 0.5);
			vertices[j] = { x1, y1, j == 1 || j == 2 ? 1.f : 0.f, j == 2 || j == 3 ? 1.f : 0.f };
		}
		int shade;
		if ((actor->sector()->ceilingstat & CSTAT_SECTOR_SKY)) shade = actor->sector()->ceilingshade;
		else shade = actor->sector()->floorshade;
		shade += actor->spr.shade;
		PalEntry color = shadeToLight(shade);
		FRenderStyle rs = LegacyRenderStyles[STYLE_Translucent];
		float alpha = 1;
		if (actor->spr.cstat & CSTAT_SPRITE_TRANSLUCENT)
		{
			rs = GetRenderStyle(0, !!(actor->spr.cstat & CSTAT_SPRITE_TRANS_FLIP));
			alpha = GetAlphaFromBlend((actor->spr.cstat & CSTAT_SPRITE_TRANS_FLIP) ? DAMETH_TRANS2 : DAMETH_TRANS1, 0);
			color.a = uint8_t(alpha * 255);
		}

		int translation = TRANSLATION(Translation_Remap + curbasepal, actor->spr.pal);
		int picnum = actor->spr.picnum;
		gotpic.Set(picnum);
		const static unsigned indices[] = { 0, 1, 2, 0, 2, 3 };
		twod->AddPoly(tileGetTexture(picnum, true), vertices.Data(), vertices.Size(), indices, 6, translation, color, rs, &viewport3d);
	}
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void DrawOverheadMap(int pl_x, int pl_y, const DAngle pl_angle, double const smoothratio)
{
	if (am_followplayer || follow_x == INT_MAX)
	{
		follow_x = pl_x * inttoworld;
		follow_y = pl_y * inttoworld;
	}
	int x = follow_x * worldtoint;
	int y = follow_y * worldtoint;
	const DVector2 follow(follow_x, follow_y);
	follow_a = am_rotate ? pl_angle : DAngle::fromBuild(1536);
	AutomapControl();

	if (automapMode == am_full)
	{
		twod->ClearScreen();
		renderDrawMapView(follow, gZoom, follow_a);
	}
	drawredlines(follow, gZoom, follow_a);
	drawwhitelines(follow, gZoom, follow_a);
	if (!gi->DrawAutomapPlayer(pl_x, pl_y, x, y, gZoom, follow_a, smoothratio))
		DrawPlayerArrow(follow, follow_a, gZoom, -pl_angle);

}

