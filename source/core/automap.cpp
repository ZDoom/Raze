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

static DVector2 min_bounds = { INT_MAX, 0 };;
static DVector2 max_bounds = { 0, 0 };
static DVector2 follow = { INT_MAX, INT_MAX };
static DAngle follow_a = DAngle::fromDeg(INT_MAX);
static double gZoom = 0.75;
static float am_zoomdir;
int automapMode;
bool automapping;
bool gFullMap;
BitArray show2dsector;
BitArray show2dwall;

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
	if (am_followplayer) follow.X = INT_MAX;
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
	min_bounds = { INT_MAX, INT_MAX };
	max_bounds = { INT_MIN, INT_MIN };

	for(auto& wal : wall)
	{
		// get map min and max coordinates
		if (wal.pos.X < min_bounds.X) min_bounds.X = wal.pos.X;
		if (wal.pos.Y < min_bounds.Y) min_bounds.Y = wal.pos.Y;
		if (wal.pos.X > max_bounds.X) max_bounds.X = wal.pos.X;
		if (wal.pos.Y > max_bounds.Y) max_bounds.Y = wal.pos.Y;
	}
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

static void AutomapControl(const DVector2& cangvect)
{
	static double nonsharedtimer;
	double ms = screen->FrameTime;
	double interval;

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
		if (am_zoomdir > 0)
		{
			gZoom = gZoom * am_zoomdir;
		}
		else if (am_zoomdir < 0)
		{
			gZoom = gZoom / -am_zoomdir;
		}
		am_zoomdir = 0;

		double j = interval * (35. / 65536.) / gZoom;
		gZoom += (buttonMap.ButtonDown(gamefunc_Enlarge_Screen) - buttonMap.ButtonDown(gamefunc_Shrink_Screen)) * j * max(gZoom, 0.25);
		gZoom = clamp(gZoom, 0.05, 2.);

		if (!am_followplayer)
		{
			const double zoomspeed = j * 512.;
			const auto panhorz = buttonMap.ButtonDown(gamefunc_AM_PanRight) - buttonMap.ButtonDown(gamefunc_AM_PanLeft);
			const auto panvert = buttonMap.ButtonDown(gamefunc_AM_PanUp) - buttonMap.ButtonDown(gamefunc_AM_PanDown);

			if (min_bounds.X == INT_MAX) CalcMapBounds();

			follow = clamp(follow + DVector2(panvert, panhorz).Rotated(cangvect.X, cangvect.Y) * zoomspeed, min_bounds, max_bounds);
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
	min_bounds.X = INT_MAX;
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

void drawlinergb(const DVector2& v1, const DVector2& v2, PalEntry p)
{
	if (am_linethickness <= 1) 
	{
		twod->AddLine(v1, v2, &viewport3d, p, uint8_t(am_linealpha * 255));
	}
	else
	{
		twod->AddThickLine(v1, v2, am_linethickness, p, uint8_t(am_linealpha * 255));
	}
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

static inline PalEntry RedLineColor()
{
	// todo:
	// Blood uses palette index 12 (99,99,99)
	// Exhumed uses palette index 111 (roughly 170,170,170) but darkens the line in overlay mode the farther it is away from the player in vertical direction.
	// Shadow Warrior uses palette index 152 in overlay mode and index 12 in full map mode. (152: 84, 88, 40)
	return automapMode == am_overlay? *am_ovtwosidedcolor : *am_twosidedcolor;
}

static inline PalEntry WhiteLineColor()
{

	// todo:
	// Blood uses palette index 24
	// Exhumed uses palette index 111 (roughly 170,170,170) but darkens the line in overlay mode the farther it is away from the player in vertical direction.
	// Shadow Warrior uses palette index 24 (60,60,60)
	return automapMode == am_overlay ? *am_ovonesidedcolor : *am_onesidedcolor;
}

static inline PalEntry PlayerLineColor()
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

static void drawredlines(const DVector2& cpos, const DVector2& cangvect, const DVector2& xydim)
{
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
				auto v1 = OutAutomapVector(wal.pos - cpos, cangvect, gZoom, xydim);
				auto v2 = OutAutomapVector(wal.point2Wall()->pos - cpos, cangvect, gZoom, xydim);
				drawlinergb(v1, v2, RedLineColor());
			}
		}
	}
}

//---------------------------------------------------------------------------
//
// one sided lines
//
//---------------------------------------------------------------------------

static void drawwhitelines(const DVector2& cpos, const DVector2& cangvect, const DVector2& xydim)
{
	for (int i = (int)sector.Size() - 1; i >= 0; i--)
	{
		if (!gFullMap && !show2dsector[i] && !isSWALL()) continue;

		for (auto& wal : wallsofsector(i))
		{
			if (wal.nextwall >= 0) continue;
			if (!gFullMap && !tileGetTexture(wal.picnum)->isValid()) continue;

			if (isSWALL() && !gFullMap && !show2dwall[wallnum(&wal)])
				continue;

			auto v1 = OutAutomapVector(wal.pos - cpos, cangvect, gZoom, xydim);
			auto v2 = OutAutomapVector(wal.point2Wall()->pos - cpos, cangvect, gZoom, xydim);
			drawlinergb(v1, v2, WhiteLineColor());
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
#if 0
	static constexpr int arrow[] =
	{
		0, 65536, 0, -65536,
		0, 65536, -32768, 32878,
		0, 65536, 32768, 32878,
	};

	double xvect = -cang.Sin() * czoom;
	double yvect = -cang.Cos() * czoom;

	double pxvect = -pl_angle.Sin();
	double pyvect = -pl_angle.Cos();

	int width = screen->GetWidth();
	int height = screen->GetHeight();

	for (int i = 0; i < 12; i += 4)
	{
		// FIXME: This has been broken since before the floatification refactor.
		// Needs repair and changing out to backended vector function.
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
#endif
}


//---------------------------------------------------------------------------
//
// floor textures
//
//---------------------------------------------------------------------------

static void renderDrawMapView(const DVector2& cpos, const DVector2& cangvect, const DVector2& xydim)
{
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
				auto v = OutAutomapVector(DVector2(mesh->vertices[j].X - cpos.X, -mesh->vertices[j].Y - cpos.Y), cangvect, gZoom, xydim);
				vertices[j] = { float(v.X), float(v.Y), mesh->texcoords[j].X, mesh->texcoords[j].Y };
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
			auto v = OutAutomapVector(pp[j] - cpos, cangvect, gZoom, xydim);
			vertices[j] = { float(v.X), float(v.Y), j == 1 || j == 2 ? 1.f : 0.f, j == 2 || j == 3 ? 1.f : 0.f };
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

void DrawOverheadMap(const DVector2& plxy, const DAngle pl_angle, double const interpfrac)
{
	if (am_followplayer || follow.X == INT_MAX)
	{
		follow = plxy;
	}

	follow_a = am_rotate ? pl_angle : DAngle270;
	const DVector2 xydim = DVector2(screen->GetWidth(), screen->GetHeight()) * 0.5;
	const DVector2 avect = follow_a.ToVector();

	AutomapControl(avect);

	if (automapMode == am_full)
	{
		twod->ClearScreen();
		renderDrawMapView(follow, avect, xydim);
	}

	drawredlines(follow, avect, xydim);
	drawwhitelines(follow, avect, xydim);
	if (!gi->DrawAutomapPlayer(plxy, follow, follow_a, xydim, gZoom, interpfrac))
		DrawPlayerArrow(follow, follow_a, gZoom, pl_angle);

}

//---------------------------------------------------------------------------
//
// Draws lines for alls in Duke/SW when cstat is CSTAT_SPRITE_ALIGNMENT_FACING.
//
//---------------------------------------------------------------------------

void DrawAutomapAlignmentFacing(const spritetype& spr, const DVector2& bpos, const DVector2& cangvect, const double czoom, const DVector2& xydim, const PalEntry& col)
{
	auto v1 = OutAutomapVector(bpos, cangvect, czoom, xydim);
	auto v2 = OutAutomapVector(spr.angle.ToVector() * 8., cangvect, czoom);
	auto v3 = v2.Rotated90CW();
	auto v4 = v1 + v2;

	drawlinergb(v1 - v2, v4, col);
	drawlinergb(v1 - v3, v4, col);
	drawlinergb(v1 + v3, v4, col);
}

//---------------------------------------------------------------------------
//
// Draws lines for alls in Duke/SW when cstat is CSTAT_SPRITE_ALIGNMENT_WALL.
//
//---------------------------------------------------------------------------

void DrawAutomapAlignmentWall(const spritetype& spr, const DVector2& bpos, const DVector2& cangvect, const double czoom, const DVector2& xydim, const PalEntry& col)
{
	auto xrep = spr.xrepeat * REPEAT_SCALE;
	auto xspan = tileWidth(spr.picnum);
	auto xoff = tileLeftOffset(spr.picnum) + spr.xoffset;

	if ((spr.cstat & CSTAT_SPRITE_XFLIP) > 0) xoff = -xoff;

	auto sprvec = spr.angle.ToVector().Rotated90CW() * xrep;
	
	auto b1 = bpos - sprvec * ((xspan * 0.5) + xoff);
	auto b2 = b1 + sprvec * xspan;

	auto v1 = OutAutomapVector(b1, cangvect, czoom, xydim);
	auto v2 = OutAutomapVector(b2, cangvect, czoom, xydim);

	drawlinergb(v1, v2, col);
}


//---------------------------------------------------------------------------
//
// Draws lines for alls in Duke/SW when cstat is CSTAT_SPRITE_ALIGNMENT_FLOOR.
//
//---------------------------------------------------------------------------

void DrawAutomapAlignmentFloor(const spritetype& spr, const DVector2& bpos, const DVector2& cangvect, const double czoom, const DVector2& xydim, const PalEntry& col)
{
	auto xrep = spr.xrepeat * REPEAT_SCALE;
	auto yrep = spr.yrepeat * REPEAT_SCALE;
	auto xspan = tileWidth(spr.picnum);
	auto yspan = tileHeight(spr.picnum);
	auto xoff = tileLeftOffset(spr.picnum);
	auto yoff = tileTopOffset(spr.picnum);

	if (isSWALL() || (spr.cstat & CSTAT_SPRITE_ALIGNMENT_MASK) != CSTAT_SPRITE_ALIGNMENT_SLOPE)
	{
		xoff += spr.xoffset;
		yoff += spr.yoffset;
	}

	if ((spr.cstat & CSTAT_SPRITE_XFLIP) > 0) xoff = -xoff;
	if ((spr.cstat & CSTAT_SPRITE_YFLIP) > 0) yoff = -yoff;

	auto sprvec = spr.angle.ToVector();
	auto xscale = sprvec.Rotated90CW() * xspan * xrep;
	auto yscale = sprvec * yspan * yrep;
	auto xybase = DVector2(((xspan * 0.5) + xoff) * xrep, ((yspan * 0.5) + yoff) * yrep);

	auto b1 = bpos + (xybase * sprvec.Y) + (xybase.Rotated90CW() * sprvec.X);
	auto b2 = b1 - xscale;
	auto b3 = b2 - yscale;
	auto b4 = b1 - yscale;

	auto v1 = OutAutomapVector(b1, cangvect, czoom, xydim);
	auto v2 = OutAutomapVector(b2, cangvect, czoom, xydim);
	auto v3 = OutAutomapVector(b3, cangvect, czoom, xydim);
	auto v4 = OutAutomapVector(b4, cangvect, czoom, xydim);

	drawlinergb(v1, v2, col);
	drawlinergb(v2, v3, col);
	drawlinergb(v3, v4, col);
	drawlinergb(v4, v1, col);
}
