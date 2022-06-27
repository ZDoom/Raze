//-----------------------------------------------------------------------------
//
// Copyright 1993-1996 id Software
// Copyright 1994-1996 Raven Software
// Copyright 1999-2016 Randy Heit
// Copyright 2002-2016 Christoph Oelckers
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see http://www.gnu.org/licenses/
//
//-----------------------------------------------------------------------------
//
//
// DESCRIPTION:  the automap code
//
//-----------------------------------------------------------------------------

#include <stdio.h>
#include <array>

#include "filesystem.h"
#include "d_event.h"
#include "gi.h"
#include "c_bind.h"
#include "serializer.h"
#include "v_video.h"
#include "v_draw.h"
#include "c_dispatch.h"
#include "statusbar.h"
#include "i_net.h"
#include "fcolormap.h"
#include "gamestate.h"

// Data.
#include "gstrings.h"

#include "am_map.h"
#include "c_buttons.h"
#include "texturemanager.h"
#include "v_font.h"
#include "printf.h"
#include "coreactor.h"
#include "gamefuncs.h"
#include "sectorgeometry.h"
#include "hw_sections.h"

#include "gamecontrol.h"

//=============================================================================
//
// Global state
//
//=============================================================================

enum
{
	AM_NUMMARKPOINTS = 10,
};

// C++ cannot do static const floats in a class, so these need to be global...
static const double PLAYERRADIUS = 16.;	// player radius for automap checking
static const double M_ZOOMIN = 2; // how much zoom-in per second
static const double M_ZOOMOUT = 0.2; // how much zoom-out per second
static const double M_OLDZOOMIN = (1.02); // for am_zoom
static const double M_OLDZOOMOUT = (1 / 1.02);

static FTextureID marknums[AM_NUMMARKPOINTS]; // numbers used for marking by the automap
bool automapactive = false;

//=============================================================================
//
// Types
//
//=============================================================================

struct  fpoint_t
{
	int x, y;
};

struct fline_t
{
	fpoint_t a, b;
};

struct mpoint_t
{
	double x, y;
};

struct mline_t
{
	mpoint_t a, b;
};

struct islope_t
{
	double slp, islp;
};

//=============================================================================
//
// CVARs
//
//=============================================================================

CVAR(Bool, am_textured, false, CVAR_ARCHIVE)
CVAR(Float, am_linealpha, 1.0f, CVAR_ARCHIVE)
CVAR(Int, am_linethickness, 1, CVAR_ARCHIVE)
CVAR(Bool, am_thingrenderstyles, true, CVAR_ARCHIVE)


EXTERN_CVAR(Bool, sv_cheats)
CUSTOM_CVAR(Int, am_cheat, 0, 0)
{
	// No automap cheat in net games when cheats are disabled!
	if (netgame && !sv_cheats && self != 0)
	{
		self = 0;
	}
}

CVAR(Int, am_rotate, 0, CVAR_ARCHIVE);
CVAR(Int, am_overlay, 0, CVAR_ARCHIVE);
CVAR(Bool, am_showsecrets, true, CVAR_ARCHIVE);
CVAR(Bool, am_showmonsters, true, CVAR_ARCHIVE);
CVAR(Bool, am_showtime, true, CVAR_ARCHIVE);
CVAR(Bool, am_showtotaltime, false, CVAR_ARCHIVE);
CVAR(Int, am_colorset, 0, CVAR_ARCHIVE);
CVAR(Bool, am_customcolors, true, CVAR_ARCHIVE);
CVAR(Int, am_map_secrets, 1, CVAR_ARCHIVE);
CVAR(Int, am_drawmapback, 1, CVAR_ARCHIVE);
CVAR(Bool, am_showkeys, true, CVAR_ARCHIVE);
CVAR(Int, am_showthingsprites, 0, CVAR_ARCHIVE);
CVAR (Bool, am_showkeys_always, false, CVAR_ARCHIVE);

DAutomapBase* automap;

CUSTOM_CVAR(Int, am_emptyspacemargin, 0, CVAR_ARCHIVE)
{
	if (self < 0)
	{
		self = 0;
	}
	else if (self > 90)
	{
		self = 90;
	}

	if (automap)
	{
		automap->NewResolution();
	}
}

//=============================================================================
//
// map functions
//
//=============================================================================

CVAR(Bool, am_followplayer, true, CVAR_ARCHIVE)
CVAR(Bool, am_portaloverlay, true, CVAR_ARCHIVE)
CVAR(Bool, am_showgrid, false, CVAR_ARCHIVE)
CVAR(Float, am_zoomdir, 0, CVAR_ARCHIVE)

static const char *const DEFAULT_FONT_NAME = "AMMNUMx";
CVAR(String, am_markfont, DEFAULT_FONT_NAME, CVAR_ARCHIVE)
CVAR(Int, am_markcolor, CR_GREY, CVAR_ARCHIVE)

CCMD(am_togglefollow)
{
	am_followplayer = !am_followplayer;
	if (automap)
		automap->ResetFollowLocation();
	Printf("%s\n", GStrings(am_followplayer ? "AMSTR_FOLLOWON" : "AMSTR_FOLLOWOFF"));
}

CCMD(am_togglegrid)
{
	am_showgrid = !am_showgrid;
	Printf("%s\n", GStrings(am_showgrid ? "AMSTR_GRIDON" : "AMSTR_GRIDOFF"));
}

CCMD(am_toggletexture)
{
	am_textured = !am_textured;
	Printf("%s\n", GStrings(am_textured ? "AMSTR_TEXON" : "AMSTR_TEXOFF"));
}

CCMD(am_setmark)
{
	if (automap)
	{
		int m = automap->addMark();
		if (m >= 0)
		{
			Printf("%s %d\n", GStrings("AMSTR_MARKEDSPOT"), m);
		}
	}
}

CCMD(am_clearmarks)
{
	if (automap && automap->clearMarks())
	{
		Printf("%s\n", GStrings("AMSTR_MARKSCLEARED"));
	}
}

CCMD(am_gobig)
{
	if (automap)
		automap->GoBig();
}

CCMD(togglemap)
{
	if (gameaction == ga_nothing)
	{
		gameaction = ga_togglemap;
	}
}

CCMD(am_zoom)
{
	if (argv.argc() >= 2)
	{
		am_zoomdir = (float)atof(argv[1]);
	}
}


//=============================================================================
//
// Automap colors
//
//=============================================================================

CVAR (Color, am_backcolor,			0x6c5440,	CVAR_ARCHIVE);
CVAR (Color, am_yourcolor,			0xfce8d8,	CVAR_ARCHIVE);
CVAR (Color, am_wallcolor,			0x2c1808,	CVAR_ARCHIVE);
CVAR (Color, am_secretwallcolor,	0x000000,	CVAR_ARCHIVE);
CVAR (Color, am_specialwallcolor,	0xffffff,	CVAR_ARCHIVE);
CVAR (Color, am_tswallcolor,		0x888888,	CVAR_ARCHIVE);
CVAR (Color, am_fdwallcolor,		0x887058,	CVAR_ARCHIVE);
CVAR (Color, am_cdwallcolor,		0x4c3820,	CVAR_ARCHIVE);
CVAR (Color, am_efwallcolor,		0x665555,	CVAR_ARCHIVE);
CVAR (Color, am_thingcolor,			0xfcfcfc,	CVAR_ARCHIVE);
CVAR (Color, am_gridcolor,			0x8b5a2b,	CVAR_ARCHIVE);
CVAR (Color, am_xhaircolor,			0x808080,	CVAR_ARCHIVE);
CVAR (Color, am_notseencolor,		0x6c6c6c,	CVAR_ARCHIVE);
CVAR (Color, am_lockedcolor,		0x007800,	CVAR_ARCHIVE);
CVAR (Color, am_intralevelcolor,	0x0000ff,	CVAR_ARCHIVE);
CVAR (Color, am_interlevelcolor,	0xff0000,	CVAR_ARCHIVE);
CVAR (Color, am_secretsectorcolor,	0xff00ff,	CVAR_ARCHIVE);
CVAR (Color, am_unexploredsecretcolor,	0xff00ff,	CVAR_ARCHIVE);
CVAR (Color, am_thingcolor_friend,	0xfcfcfc,	CVAR_ARCHIVE);
CVAR (Color, am_thingcolor_monster,	0xfcfcfc,	CVAR_ARCHIVE);
CVAR (Color, am_thingcolor_ncmonster,	0xfcfcfc,	CVAR_ARCHIVE);
CVAR (Color, am_thingcolor_item,	0xfcfcfc,	CVAR_ARCHIVE);
CVAR (Color, am_thingcolor_citem,	0xfcfcfc,	CVAR_ARCHIVE);
CVAR (Color, am_portalcolor,		0x404040,	CVAR_ARCHIVE);

CVAR (Color, am_ovyourcolor,		0xfce8d8,	CVAR_ARCHIVE);
CVAR (Color, am_ovwallcolor,		0x00ff00,	CVAR_ARCHIVE);
CVAR (Color, am_ovsecretwallcolor,	0x008844,	CVAR_ARCHIVE);
CVAR (Color, am_ovspecialwallcolor,	0xffffff,	CVAR_ARCHIVE);
CVAR (Color, am_ovotherwallscolor,	0x008844,	CVAR_ARCHIVE);
CVAR (Color, am_ovlockedcolor,		0x008844,	CVAR_ARCHIVE);
CVAR (Color, am_ovefwallcolor,		0x008844,	CVAR_ARCHIVE);
CVAR (Color, am_ovfdwallcolor,		0x008844,	CVAR_ARCHIVE);
CVAR (Color, am_ovcdwallcolor,		0x008844,	CVAR_ARCHIVE);
CVAR (Color, am_ovunseencolor,		0x00226e,	CVAR_ARCHIVE);
CVAR (Color, am_ovtelecolor,		0xffff00,	CVAR_ARCHIVE);
CVAR (Color, am_ovinterlevelcolor,	0xffff00,	CVAR_ARCHIVE);
CVAR (Color, am_ovsecretsectorcolor,0x00ffff,	CVAR_ARCHIVE);
CVAR (Color, am_ovunexploredsecretcolor,0x00ffff,	CVAR_ARCHIVE);
CVAR (Color, am_ovthingcolor,		0xe88800,	CVAR_ARCHIVE);
CVAR (Color, am_ovthingcolor_friend,	0xe88800,	CVAR_ARCHIVE);
CVAR (Color, am_ovthingcolor_monster,	0xe88800,	CVAR_ARCHIVE);
CVAR (Color, am_ovthingcolor_ncmonster,	0xe88800,	CVAR_ARCHIVE);
CVAR (Color, am_ovthingcolor_item,		0xe88800,	CVAR_ARCHIVE);
CVAR (Color, am_ovthingcolor_citem,		0xe88800,	CVAR_ARCHIVE);
CVAR (Color, am_ovportalcolor,			0x004022,	CVAR_ARCHIVE);

//=============================================================================
//
// internal representation of a single color
//
//=============================================================================

struct AMColor
{
	uint32_t RGB;

	void FromCVar(FColorCVar & cv)
	{
		RGB = uint32_t(cv) | MAKEARGB(255, 0, 0, 0);
	}

	void FromRGB(int r,int g, int b)
	{
		RGB = MAKEARGB(255, r, g, b);
	}

	void setInvalid()
	{
		RGB = 0;
	}

	bool isValid() const
	{
		return RGB != 0;
	}
};

//=============================================================================
//
// a complete color set
//
//=============================================================================

static const char *ColorNames[] = {
		"Background", 
		"YourColor", 
		"WallColor", 
		"TwoSidedWallColor",
		"FloorDiffWallColor", 
		"CeilingDiffWallColor", 
		"ExtraFloorWallColor", 
		"ThingColor",
		"ThingColor_Item", 
		"ThingColor_CountItem", 
		"ThingColor_Monster", 
		"ThingColor_NocountMonster", 
		"ThingColor_Friend",
		"SpecialWallColor", 
		"SecretWallColor", 
		"GridColor", 
		"XHairColor",
		"NotSeenColor",
		"LockedColor",
		"IntraTeleportColor", 
		"InterTeleportColor",
		"SecretSectorColor",
		"UnexploredSecretColor",
		"PortalColor",
		"AlmostBackgroundColor",
		nullptr
};

struct AMColorset
{
	enum
	{
		Background, 
		YourColor, 
		WallColor, 
		TSWallColor,
		FDWallColor, 
		CDWallColor, 
		EFWallColor, 
		ThingColor,
		ThingColor_Item, 
		ThingColor_CountItem, 
		ThingColor_Monster, 
		ThingColor_NocountMonster, 
		ThingColor_Friend,
		SpecialWallColor, 
		SecretWallColor, 
		GridColor, 
		XHairColor,
		NotSeenColor,
		LockedColor,
		IntraTeleportColor, 
		InterTeleportColor,
		SecretSectorColor,
		UnexploredSecretColor,
		PortalColor,
		AlmostBackgroundColor,
		AM_NUM_COLORS
	};

	AMColor c[AM_NUM_COLORS];
	bool displayLocks;
	bool forcebackground;
	bool defined;	// only for mod specific colorsets: must be true to be usable

	void initFromCVars(FColorCVar **values)
	{
		for(int i=0;i<AlmostBackgroundColor; i++)
		{
			c[i].FromCVar(*values[i]);
		}

		uint32_t ba = *(values[0]);

		int r = RPART(ba) - 16;
		int g = GPART(ba) - 16;
		int b = BPART(ba) - 16;

		if (r < 0)
			r += 32;
		if (g < 0)
			g += 32;
		if (b < 0)
			b += 32;

		c[AlmostBackgroundColor].FromRGB(r, g, b);
		displayLocks = true;
		forcebackground = false;
	}

	void initFromColors(const unsigned char *colors, bool showlocks)
	{
		for(int i=0, j=0; i<AM_NUM_COLORS; i++, j+=3)
		{
			if (colors[j] == 1 && colors[j+1] == 0 && colors[j+2] == 0)
			{
				c[i].setInvalid();
			}
			else
			{
				c[i].FromRGB(colors[j], colors[j+1], colors[j+2]);
			}
		}
		displayLocks = showlocks;
		forcebackground = false;
	}

	void setWhite()
	{
		c[0].FromRGB(0,0,0);
		for(int i=1; i<AM_NUM_COLORS; i++)
		{
			c[i].FromRGB(255,255,255);
		}
	}

	const AMColor &operator[](int index) const
	{
		return c[index];
	}

	bool isValid(int index) const
	{
		return c[index].isValid();
	}
};

//=============================================================================
//
// predefined colorsets
//
//=============================================================================

static FColorCVar *cv_standard[] = {
	&am_backcolor,
	&am_yourcolor,
	&am_wallcolor,
	&am_tswallcolor,
	&am_fdwallcolor,
	&am_cdwallcolor,
	&am_efwallcolor,
	&am_thingcolor,
	&am_thingcolor_item,
	&am_thingcolor_citem,
	&am_thingcolor_monster,
	&am_thingcolor_ncmonster,
	&am_thingcolor_friend,
	&am_specialwallcolor,
	&am_secretwallcolor,
	&am_gridcolor,
	&am_xhaircolor,
	&am_notseencolor,
	&am_lockedcolor,
	&am_intralevelcolor,
	&am_interlevelcolor,
	&am_secretsectorcolor,
	&am_unexploredsecretcolor,
	&am_portalcolor
};

static FColorCVar *cv_overlay[] = {
	&am_backcolor,	// this will not be used in overlay mode
	&am_ovyourcolor,
	&am_ovwallcolor,
	&am_ovotherwallscolor,
	&am_ovfdwallcolor,
	&am_ovcdwallcolor,
	&am_ovefwallcolor,
	&am_ovthingcolor,
	&am_ovthingcolor_item,
	&am_ovthingcolor_citem,
	&am_ovthingcolor_monster,
	&am_ovthingcolor_ncmonster,
	&am_ovthingcolor_friend,
	&am_ovspecialwallcolor,
	&am_ovsecretwallcolor,
	&am_gridcolor,	// this will not be used in overlay mode
	&am_xhaircolor,	// this will not be used in overlay mode
	&am_ovunseencolor,
	&am_ovlockedcolor,
	&am_ovtelecolor,
	&am_ovinterlevelcolor,
	&am_ovsecretsectorcolor,
	&am_ovunexploredsecretcolor,
	&am_ovportalcolor
};

CCMD(am_restorecolors)
{
	for (unsigned i = 0; i < countof(cv_standard); i++)
	{
		cv_standard[i]->ResetToDefault();
	}
	for (unsigned i = 0; i < countof(cv_overlay); i++)
	{
		cv_overlay[i]->ResetToDefault();
	}
}



#define NOT_USED 1,0,0	// use almost black as indicator for an unused color

static unsigned char DoomColors[]= {
	0x00,0x00,0x00, // background
	0xff,0xff,0xff, // yourcolor
	0xfc,0x00,0x00, // wallcolor
	0x80,0x80,0x80, // tswallcolor
	0xbc,0x78,0x48,	// fdwallcolor
	0xfc,0xfc,0x00, // cdwallcolor
	0xbc,0x78,0x48,	// efwallcolor
	0x74,0xfc,0x6c, // thingcolor
	0x74,0xfc,0x6c, // thingcolor_item
	0x74,0xfc,0x6c, // thingcolor_citem
	0x74,0xfc,0x6c, // thingcolor_monster
	0x74,0xfc,0x6c, // thingcolor_ncmonster
	0x74,0xfc,0x6c, // thingcolor_friend
	NOT_USED,		// specialwallcolor
	NOT_USED,		// secretwallcolor
	0x4c,0x4c,0x4c,	// gridcolor
	0x80,0x80,0x80, // xhaircolor
	0x6c,0x6c,0x6c,	// notseencolor
	0xfc,0xfc,0x00, // lockedcolor
	NOT_USED,		// intrateleport
	NOT_USED,		// interteleport
	NOT_USED,		// secretsector
	NOT_USED,		// unexploredsecretsector
	0x10,0x10,0x10,	// almostbackground
	0x40,0x40,0x40	// portal
};

static unsigned char StrifeColors[]= {
	0x00,0x00,0x00, // background
	239, 239,   0,	// yourcolor
	199, 195, 195,	// wallcolor
	119, 115, 115,	// tswallcolor
	 55,  59,  91,	// fdwallcolor
	119, 115, 115,	// cdwallcolor
	 55,  59,  91,	// efwallcolor
	187,  59,   0,	// thingcolor
	219, 171,   0,	// thingcolor_item
	219, 171,   0,	// thingcolor_citem
	0xfc,0x00,0x00,	// thingcolor_monster
	0xfc,0x00,0x00,	// thingcolor_ncmonster
	0xfc,0x00,0x00, // thingcolor_friend
	NOT_USED,		// specialwallcolor
	NOT_USED,		// secretwallcolor
	0x4c,0x4c,0x4c,	// gridcolor
	0x80,0x80,0x80, // xhaircolor
	0x6c,0x6c,0x6c,	// notseencolor
	119, 115, 115,	// lockedcolor
	NOT_USED,		// intrateleport
	NOT_USED,		// interteleport
	NOT_USED,		// secretsector
	NOT_USED,		// unexploredsecretsector
	0x10,0x10,0x10,	// almostbackground
	0x40,0x40,0x40	// portal
};

static unsigned char RavenColors[]= {
	0x6c,0x54,0x40, // background
	0xff,0xff,0xff, // yourcolor
	 75,  50,  16,	// wallcolor
	 88,  93,  86,	// tswallcolor
	208, 176, 133,  // fdwallcolor
	103,  59,  31,	// cdwallcolor
	208, 176, 133,  // efwallcolor
	236, 236, 236,	// thingcolor
	236, 236, 236,	// thingcolor_item
	236, 236, 236,	// thingcolor_citem
	236, 236, 236,	// thingcolor_monster
	236, 236, 236,	// thingcolor_ncmonster
	236, 236, 236,	// thingcolor_friend
	NOT_USED,		// specialwallcolor
	NOT_USED,		// secretwallcolor
	 75,  50,  16,	// gridcolor
	0x00,0x00,0x00, // xhaircolor
	0x00,0x00,0x00,	// notseencolor
	103,  59,  31,	// lockedcolor
	NOT_USED,		// intrateleport
	NOT_USED,		// interteleport
	NOT_USED,		// secretsector
	NOT_USED,		// unexploredsecretsector
	0x10,0x10,0x10,	// almostbackground
	0x50,0x50,0x50	// portal
};

#undef NOT_USED

static AMColorset AMColors;
static AMColorset AMMod;
static AMColorset AMModOverlay;


void AM_ClearColorsets()
{
	AMModOverlay.defined = false;
	AMMod.defined = false;
}

//=============================================================================
//
//
//
//=============================================================================

static void AM_initColors(bool overlayed)
{
	if (overlayed)
	{
		if (am_customcolors && AMModOverlay.defined)
		{
			AMColors = AMModOverlay;
		}
		else
		{
			AMColors.initFromCVars(cv_overlay);
		}
	}
	else if (am_customcolors && AMMod.defined)
	{
		AMColors = AMMod;
	}
	else switch (am_colorset)
	{
	default:
		/* Use the custom colors in the am_* cvars */
		AMColors.initFromCVars(cv_standard);
		break;

	case 1:	// Doom
		// Use colors corresponding to the original Doom's
		AMColors.initFromColors(DoomColors, false);
		break;

	case 2:	// Strife
		// Use colors corresponding to the original Strife's
		AMColors.initFromColors(StrifeColors, false);
		break;

	case 3:	// Raven
		// Use colors corresponding to the original Raven's
		AMColors.initFromColors(RavenColors, true);
		break;

	}
}

#if 0
//=============================================================================
//
// custom color parser
//
//=============================================================================

void FMapInfoParser::ParseAMColors(bool overlay)
{
	bool colorset = false;

	AMColorset &cset = overlay? AMModOverlay : AMMod;

	cset.setWhite();
	cset.defined = true;
	sc.MustGetToken('{');
	while(sc.GetToken())
	{
		if (sc.TokenType == '}') return;

		sc.TokenMustBe(TK_Identifier);
		FString nextKey = sc.String;
		sc.MustGetToken('=');

		if (nextKey.CompareNoCase("base") == 0)
		{
			if (colorset) sc.ScriptError("'base' must be specified before the first color");
			sc.MustGetToken(TK_StringConst);
			if (sc.Compare("doom"))
			{
				cset.initFromColors(DoomColors, false);
			}
			else if (sc.Compare("raven"))
			{
				cset.initFromColors(RavenColors, true);
			}
			else if (sc.Compare("strife"))
			{
				cset.initFromColors(StrifeColors, false);
			}
			else
			{
				sc.ScriptError("Unknown value for 'base'. Must be 'Doom', 'Strife' or 'Raven'.");
			}
		}
		else if (nextKey.CompareNoCase("showlocks") == 0)
		{
			if(sc.CheckToken(TK_False)) 
				cset.displayLocks = false; 
			else 
			{ 
				sc.MustGetToken(TK_True); 
				cset.displayLocks = true; 
			} 
		}
		else
		{
			int i;
			for (i = 0; ColorNames[i] != nullptr; i++)
			{
				if (nextKey.CompareNoCase(ColorNames[i]) == 0)
				{
					sc.MustGetToken(TK_StringConst);
					FString color = sc.String;
					FString colorName = V_GetColorStringByName(color);
					if(!colorName.IsEmpty()) color = colorName;
					int colorval = V_GetColorFromString(color);
					cset.c[i].FromRGB(RPART(colorval), GPART(colorval), BPART(colorval)); 
					colorset = true;
					break;
				}
			}
			if (ColorNames[i]== nullptr)
			{
				sc.ScriptError("Unknown key '%s'", nextKey.GetChars());
			}
		}
	}
}
#endif

//=============================================================================
//
// The vector graphics for the automap.
//  A line drawing of the player pointing right,
//   starting from the middle.
//
//=============================================================================

static TArray<mline_t> MapArrow;
static TArray<mline_t> CheatMapArrow;
static TArray<mline_t> CheatKey;
static TArray<mline_t> EasyKey;

static std::array<mline_t, 3> thintriangle_guy = { {
	{{-.5,-.7}, {1,0}},
	{{1,0}, {-.5,.7}},
	{{-.5,.7}, {-.5,-.7}}
} };

//=============================================================================
//
// vector graphics
//
//=============================================================================

static void AM_ParseArrow(TArray<mline_t> &Arrow, const char *lumpname)
{
	const int R = int((8 * PLAYERRADIUS) / 7);
	FScanner sc;
	int lump = fileSystem.CheckNumForFullName(lumpname, true);
	if (lump >= 0)
	{
		sc.OpenLumpNum(lump);
		sc.SetCMode(true);
		while (sc.GetToken())
		{
			mline_t line;
			sc.TokenMustBe('(');
			sc.MustGetFloat();
			line.a.x = sc.Float*R;
			sc.MustGetToken(',');
			sc.MustGetFloat();
			line.a.y = sc.Float*R;
			sc.MustGetToken(')');
			sc.MustGetToken(',');
			sc.MustGetToken('(');
			sc.MustGetFloat();
			line.b.x = sc.Float*R;
			sc.MustGetToken(',');
			sc.MustGetFloat();
			line.b.y = sc.Float*R;
			sc.MustGetToken(')');
			Arrow.Push(line);
		}
	}
}

void AM_StaticInit()
{
	MapArrow.Clear();
	CheatMapArrow.Clear();
	CheatKey.Clear();
	EasyKey.Clear();

	if (gameinfo.mMapArrow.IsNotEmpty()) AM_ParseArrow(MapArrow, gameinfo.mMapArrow);
	if (gameinfo.mCheatMapArrow.IsNotEmpty()) AM_ParseArrow(CheatMapArrow, gameinfo.mCheatMapArrow);
	AM_ParseArrow(CheatKey, gameinfo.mCheatKey);
	AM_ParseArrow(EasyKey, gameinfo.mEasyKey);
	if (MapArrow.Size() == 0) I_FatalError("No automap arrow defined");

	char namebuf[9];

	for (int i = 0; i < AM_NUMMARKPOINTS; i++)
	{
		mysnprintf(namebuf, countof(namebuf), "AMMNUM%d", i);
		marknums[i] = TexMan.CheckForTexture(namebuf, ETextureType::MiscPatch);
	}
}


//=============================================================================
//
// the actual automap class definition
//
//=============================================================================

IMPLEMENT_CLASS(DAutomapBase, true, false);

class DAutomap :public DAutomapBase
{
	DECLARE_CLASS(DAutomap, DAutomapBase)

	double F_PANINC;

	//FLevelLocals *Level;
	// scale on entry
	// used by MTOF to scale from map-to-frame-buffer coords
	double scale_mtof = .2;
	// used by FTOM to scale from frame-buffer-to-map coords (=1/scale_mtof)
	double scale_ftom;

	int bigstate;
	int MapPortalGroup;

	// Disable the ML_DONTDRAW line flag if x% of all lines in a map are flagged with it
	// (To counter annoying mappers who think they are smart by making the automap unusable)
	bool am_showallenabled;

	// location of window on screen
	int	f_x;
	int	f_y;

	// size of window on screen
	int	f_w;
	int	f_h;

	int	amclock;

	mpoint_t	m_paninc;		// how far the window pans each tic (map coords)
	double	mtof_zoommul;	// how far the window zooms in each tic (map coords)

	double	m_x, m_y;		// LL x,y where the window is on the map (map coords)
	double	m_x2, m_y2;		// UR x,y where the window is on the map (map coords)

	//
	// width/height of window on map (map coords)
	//
	double	m_w;
	double	m_h;

	// based on level size
	double	min_x, min_y, max_x, max_y;

	double	max_w; // max_x-min_x,
	double	max_h; // max_y-min_y

	// based on player size
	double	min_w;
	double	min_h;


	double	min_scale_mtof; // used to tell when to stop zooming out
	double	max_scale_mtof; // used to tell when to stop zooming in

	// old stuff for recovery later
	double old_m_w, old_m_h;
	double old_m_x, old_m_y;

	// old location used by the Follower routine
	mpoint_t f_oldloc;

	mpoint_t markpoints[AM_NUMMARKPOINTS]; // where the points are
	int markpointnum = 0; // next point to be assigned

	FTextureID mapback;	// the automap background
	double mapystart = 0; // y-value for the start of the map bitmap...used in the parallax stuff.
	double mapxstart = 0; //x-value for the bitmap.

	TArray<FVector2> points;

	// translates between frame-buffer and map distances
	double FTOM(double x)
	{
		return x * scale_ftom;
	}

	double MTOF(double x)
	{
		return x * scale_mtof;
	}

	// translates between frame-buffer and map coordinates
	int CXMTOF(double x)
	{
		return int(MTOF((x)-m_x)/* - f_x*/);
	}

	int CYMTOF(double y) 
	{
		return int(f_h - MTOF((y)-m_y)/* + f_y*/);
	}

	void calcMinMaxMtoF();

	void DrawMarker(FGameTexture *tex, double x, double y, int yadjust,
		INTBOOL flip, double xscale, double yscale, int translation, double alpha, uint32_t fillcolor, FRenderStyle renderstyle);

	void rotatePoint(double *x, double *y);
	void rotate(double *x, double *y, DAngle an);
	void doFollowPlayer();
	void saveScaleAndLoc();
	void restoreScaleAndLoc();
	void minOutWindowScale();
	void activateNewScale();
	void findMinMaxBoundaries();
	void ClipRotatedExtents(double pivotx, double pivoty);
	void ScrollParchment(double dmapx, double dmapy);
	void changeWindowLoc();
	void maxOutWindowScale();
	void changeWindowScale(double delta);
	void clearFB(const AMColor &color);
	bool clipMline(mline_t *ml, fline_t *fl);
	void drawMline(mline_t *ml, const AMColor &color);
	void drawMline(mline_t *ml, int colorindex);
	void drawGrid(int color);
	void drawSubsectors();
	void drawWalls(bool allmap);
	void drawLineCharacter(const mline_t *lineguy, size_t lineguylines, double scale, DAngle angle, const AMColor &color, double x, double y);
	void drawPlayers();
	void drawKeys();
	void drawThings();
	void drawMarks();
	void drawAuthorMarkers();
	void drawCrosshair(const AMColor &color);


public:
	DAutomap() { F_PANINC = 140 / GameTicRate; }
	bool Responder(event_t* ev, bool last) override;
	void Ticker(void) override;
	void Drawer(int bottom) override;
	void NewResolution() override;
	void LevelInit() override;
	void UpdateShowAllLines() override;
	void Serialize(FSerializer &arc) override;
	void GoBig() override;
	void ResetFollowLocation() override;
	int addMark() override;
	bool clearMarks() override;
	DVector2 GetPosition() override;
	void startDisplay() override;

};

IMPLEMENT_CLASS(DAutomap, false, false)


//=============================================================================
//
//
//
//=============================================================================




//=============================================================================
//
// called by the coordinate drawer
//
//=============================================================================

DVector2 DAutomap::GetPosition()
{
	return DVector2((m_x + m_w / 2), (m_y + m_h / 2));
}

//=============================================================================
//
//
//
//=============================================================================

void DAutomap::activateNewScale ()
{
	m_x += m_w/2;
	m_y += m_h/2;
	m_w = FTOM(f_w);
	m_h = FTOM(f_h);
	m_x -= m_w/2;
	m_y -= m_h/2;
	m_x2 = m_x + m_w;
	m_y2 = m_y + m_h;
}

//=============================================================================
//
//
//
//=============================================================================

void DAutomap::saveScaleAndLoc ()
{
	old_m_x = m_x;
	old_m_y = m_y;
	old_m_w = m_w;
	old_m_h = m_h;
}

//=============================================================================
//
//
//
//=============================================================================

void DAutomap::restoreScaleAndLoc ()
{
	m_w = old_m_w;
	m_h = old_m_h;
	if (!am_followplayer)
	{
		m_x = old_m_x;
		m_y = old_m_y;
    }
	else
	{
		m_x = playerPos.X - m_w/2;
		m_y = playerPos.Y - m_h/2;
    }
	m_x2 = m_x + m_w;
	m_y2 = m_y + m_h;

	// Change the scaling multipliers
	scale_mtof = f_w / m_w;
	scale_ftom = 1. / scale_mtof;
}

//=============================================================================
//
// adds a marker at the current location
//
//=============================================================================

int DAutomap::addMark ()
{
	// Add a mark when default font is selected and its textures (AMMNUM?)
	// are loaded. Mark is always added when custom font is selected
	if (stricmp(*am_markfont, DEFAULT_FONT_NAME) != 0 || marknums[0].isValid())
	{
		auto m = markpointnum;
		markpoints[markpointnum].x = m_x + m_w/2;
		markpoints[markpointnum].y = m_y + m_h/2;
		markpointnum = (markpointnum + 1) % AM_NUMMARKPOINTS;
		return m;
	}
	return -1;
}

//=============================================================================
//
// Determines bounding box of all vertices,
// sets global variables controlling zoom range.
//
//=============================================================================

void DAutomap::findMinMaxBoundaries ()
{
	min_x = min_y = FLT_MAX;
	max_x = max_y = -FLT_MAX;
  
	for (auto &vert : wall)
	{walltype a;
		if (vert.pos.X < min_x)
			min_x = vert.pos.X;
		else if (vert.pos.X > max_x)
			max_x = vert.pos.X;
    
		if (vert.pos.Y < min_y)
			min_y = vert.pos.Y;
		else if (vert.pos.Y > max_y)
			max_y = vert.pos.Y;
	}
  
	max_w = max_x - min_x;
	max_h = max_y - min_y;

	min_w = 2*PLAYERRADIUS; // const? never changed?
	min_h = 2*PLAYERRADIUS;

	calcMinMaxMtoF();
}

//=============================================================================
//
//
//
//=============================================================================

void DAutomap::calcMinMaxMtoF()
{
	const double safe_frame = 1.0 - am_emptyspacemargin / 100.0;
	double a = safe_frame * (twod->GetWidth() / max_w);
	double b = safe_frame * (StatusBar->GetTopOfStatusbar() / max_h);

	min_scale_mtof = a < b ? a : b;
	max_scale_mtof = twod->GetHeight() / (2*PLAYERRADIUS);
}

//=============================================================================
//
//
//
//=============================================================================

void DAutomap::ClipRotatedExtents (double pivotx, double pivoty)
{
	if (am_rotate == 0 || (am_rotate == 2 && !viewactive))
	{
		if (m_x + m_w/2 > max_x)
			m_x = max_x - m_w/2;
		else if (m_x + m_w/2 < min_x)
			m_x = min_x - m_w/2;
	  
		if (m_y + m_h/2 > max_y)
			m_y = max_y - m_h/2;
		else if (m_y + m_h/2 < min_y)
			m_y = min_y - m_h/2;
	}

	m_x2 = m_x + m_w;
	m_y2 = m_y + m_h;
}

//=============================================================================
//
//
//
//=============================================================================

void DAutomap::ScrollParchment (double dmapx, double dmapy)
{
	mapxstart = mapxstart - dmapx * scale_mtof;
	mapystart = mapystart - dmapy * scale_mtof;

	mapxstart = clamp(mapxstart, -40000., 40000.);
	mapystart = clamp(mapystart, -40000., 40000.);

	if (mapback.isValid())
	{
		auto backtex = TexMan.GetGameTexture(mapback);

		if (backtex != nullptr)
		{
			int pwidth = int(backtex->GetDisplayWidth() * CleanXfac);
			int pheight = int(backtex->GetDisplayHeight() * CleanYfac);

			while(mapxstart > 0)
				mapxstart -= pwidth;
			while(mapxstart <= -pwidth)
				mapxstart += pwidth;
			while(mapystart > 0)
				mapystart -= pheight;
			while(mapystart <= -pheight)
				mapystart += pheight;
		}
	}
}

//=============================================================================
//
//
//
//=============================================================================

void DAutomap::changeWindowLoc ()
{
	if (m_paninc.x || m_paninc.y)
	{
		am_followplayer = false;
		f_oldloc.x = FLT_MAX;
	}

	double oldmx = m_x, oldmy = m_y;
	double incx, incy, oincx, oincy;
	
	incx = m_paninc.x;
	incy = m_paninc.y;

	oincx = incx = m_paninc.x * twod->GetWidth() / 320;
	oincy = incy = m_paninc.y * twod->GetHeight() / 200;
	if (am_rotate == 1 || (am_rotate == 2 && viewactive))
	{
		rotate(&incx, &incy, playerAngle - 90.);
	}

	m_x += incx;
	m_y += incy;

	ClipRotatedExtents (oldmx + m_w/2, oldmy + m_h/2);
	ScrollParchment (m_x != oldmx ? oincx : 0, m_y != oldmy ? -oincy : 0);
}


//=============================================================================
//
//
//
//=============================================================================

void DAutomap::startDisplay()
{
	int pnum;

	f_oldloc.x = FLT_MAX;
	amclock = 0;

	m_paninc.x = m_paninc.y = 0;
	mtof_zoommul = 1.;

	m_w = FTOM(twod->GetWidth());
	m_h = FTOM(twod->GetHeight());

	m_x = playerPos.X - m_w/2;
	m_y = playerPos.Y - m_h/2;
	changeWindowLoc();

	// for saving & restoring
	old_m_x = m_x;
	old_m_y = m_y;
	old_m_w = m_w;
	old_m_h = m_h;
}

//=============================================================================
//
//
//
//=============================================================================

bool DAutomap::clearMarks ()
{
	for (int i = AM_NUMMARKPOINTS-1; i >= 0; i--)
		markpoints[i].x = -1; // means empty
	markpointnum = 0;
	return marknums[0].isValid();
}

//=============================================================================
//
// called right after the level has been loaded
//
//=============================================================================

void DAutomap::LevelInit ()
{
	//if (Level->info->MapBackground.Len() == 0)
	{
		mapback = TexMan.CheckForTexture("AUTOPAGE", ETextureType::MiscPatch);
	}
	/*
	else
	{
		mapback = TexMan.CheckForTexture(Level->info->MapBackground, ETextureType::MiscPatch);
	}
	 */

	clearMarks();

	findMinMaxBoundaries();
	scale_mtof = min_scale_mtof / 0.7;
	if (scale_mtof > max_scale_mtof)
		scale_mtof = min_scale_mtof;
	scale_ftom = 1 / scale_mtof;

	UpdateShowAllLines();
}

//=============================================================================
//
// set the window scale to the maximum size
//
//=============================================================================

void DAutomap::minOutWindowScale ()
{
	scale_mtof = min_scale_mtof;
	scale_ftom = 1/ scale_mtof;
}

//=============================================================================
//
// set the window scale to the minimum size
//
//=============================================================================

void DAutomap::maxOutWindowScale ()
{
	scale_mtof = max_scale_mtof;
	scale_ftom = 1 / scale_mtof;
}

//=============================================================================
//
// Called right after the resolution has changed
//
//=============================================================================

void DAutomap::NewResolution()
{
	double oldmin = min_scale_mtof;
	
	if ( oldmin == 0 ) 
	{
		return; // [SP] Not in a game, exit!
	}	
	calcMinMaxMtoF();
	scale_mtof = scale_mtof * min_scale_mtof / oldmin;
	scale_ftom = 1 / scale_mtof;
	if (scale_mtof < min_scale_mtof)
		minOutWindowScale();
	else if (scale_mtof > max_scale_mtof)
		maxOutWindowScale();
	f_w = twod->GetWidth();
	f_h = StatusBar->GetTopOfStatusbar();
	activateNewScale();
}


//=============================================================================
//
// Handle events (user inputs) in automap mode
//
//=============================================================================

bool DAutomap::Responder (event_t *ev, bool last)
{
	if (automapactive && (ev->type == EV_KeyDown || ev->type == EV_KeyUp))
	{
		if (am_followplayer)
		{
			// check for am_pan* and ignore in follow mode
			const char *defbind = AutomapBindings.GetBind(ev->data1);
			if (defbind && !strnicmp(defbind, "+am_pan", 7)) return false;
		}

		bool res = C_DoKey(ev, &AutomapBindings, nullptr);
		if (res && ev->type == EV_KeyUp && !last)
		{
			// If this is a release event we also need to check if it released a button in the main Bindings
			// so that that button does not get stuck.
			const char *defbind = Bindings.GetBind(ev->data1);
			return (!defbind || defbind[0] != '+'); // Let G_Responder handle button releases
		}
		return res;
	}
	return false;
}


//=============================================================================
//
// Zooming
//
//=============================================================================

void DAutomap::changeWindowScale (double delta)
{

	double mtof_zoommul;

	if (am_zoomdir > 0)
	{
		mtof_zoommul = M_OLDZOOMIN * am_zoomdir;
	}
	else if (am_zoomdir < 0)
	{
		mtof_zoommul = M_OLDZOOMOUT / -am_zoomdir;
	}
	else if (buttonMap.ButtonDown(gamefunc_Enlarge_Screen))
	{
		mtof_zoommul = (1 + (M_ZOOMIN - 1) * delta);
	}
	else if (buttonMap.ButtonDown(gamefunc_Shrink_Screen))
	{
		mtof_zoommul = (1 + (M_ZOOMOUT - 1) * delta);
	}
	else
	{
		mtof_zoommul = 1;
	}
	am_zoomdir = 0;

	// Change the scaling multipliers
	scale_mtof = scale_mtof * mtof_zoommul;
	scale_ftom = 1 / scale_mtof;

	if (scale_mtof < min_scale_mtof)
		minOutWindowScale();
	else if (scale_mtof > max_scale_mtof)
		maxOutWindowScale();
}

//=============================================================================
//
//
//
//=============================================================================

void DAutomap::doFollowPlayer ()
{
	double sx, sy;
	
	DVector2 ampos = playerPos;
	
	if (f_oldloc.x != ampos.X || f_oldloc.y != ampos.Y)
	{
		m_x = ampos.X - m_w / 2;
		m_y = ampos.Y - m_h / 2;
		m_x2 = m_x + m_w;
		m_y2 = m_y + m_h;
		
		// do the parallax parchment scrolling.
		sx = (ampos.X - f_oldloc.x);
		sy = (f_oldloc.y - ampos.Y);
		if (am_rotate == 1 || (am_rotate == 2 && viewactive))
		{
			rotate(&sx, &sy, playerAngle - 90);
		}
		ScrollParchment(sx, sy);
		
		f_oldloc.x = ampos.X;
		f_oldloc.y = ampos.Y;
	}
}

//=============================================================================
//
// Updates on Game Tick
//
//=============================================================================

void DAutomap::Ticker ()
{
	if (!automapactive)
		return;

	amclock++;
}


//=============================================================================
//
// Clear automap frame buffer.
//
//=============================================================================

void DAutomap::clearFB (const AMColor &color)
{
	bool drawback = mapback.isValid() && am_drawmapback != 0;
	if (am_drawmapback == 2)
	{
		// only draw background when using a mod defined custom color set or Raven colors, if am_drawmapback is 2.
		if (!am_customcolors || !AMMod.defined)
		{
			drawback &= (am_colorset == 3);
		}
	}

	if (!drawback)
	{
		ClearRect(twod, 0, 0, f_w, f_h, -1, color.RGB);
	}
	else
	{
		auto backtex = TexMan.GetGameTexture(mapback);
		if (backtex != nullptr)
		{
			int pwidth = int(backtex->GetDisplayWidth() * CleanXfac);
			int pheight = int(backtex->GetDisplayHeight() * CleanYfac);
			int x, y;

			//blit the automap background to the screen.
			for (y = int(mapystart); y < f_h; y += pheight)
			{
				for (x = int(mapxstart); x < f_w; x += pwidth)
				{
					DrawTexture(twod, backtex, x, y, DTA_ClipBottom, f_h, DTA_TopOffset, 0, DTA_LeftOffset, 0, DTA_DestWidth, pwidth, DTA_DestHeight, pheight, TAG_DONE);
				}
			}
		}
	}
}


//=============================================================================
//
// Automap clipping of lines.
//
// Based on Cohen-Sutherland clipping algorithm but with a slightly
// faster reject and precalculated slopes.  If the speed is needed,
// use a hash algorithm to handle the common cases.
//
//=============================================================================

bool DAutomap::clipMline (mline_t *ml, fline_t *fl)
{
	enum {
		LEFT	=1,
		RIGHT	=2,
		BOTTOM	=4,
		TOP		=8
	};

	int outcode1 = 0;
	int outcode2 = 0;
	int outside;

	fpoint_t tmp = { 0, 0 };
	int dx;
	int dy;

	auto DOOUTCODE = [this](int &oc, double mx, double my)
	{
		oc = 0;
		if (my < 0) oc |= TOP;
		else if (my >= f_h) oc |= BOTTOM;
		if (mx < 0) oc |= LEFT;
		else if (mx >= f_w) oc |= RIGHT;
	};

	// do trivial rejects and outcodes
	if (ml->a.y > m_y2)
		outcode1 = TOP;
	else if (ml->a.y < m_y)
		outcode1 = BOTTOM;

	if (ml->b.y > m_y2)
		outcode2 = TOP;
	else if (ml->b.y < m_y)
		outcode2 = BOTTOM;

	if (outcode1 & outcode2)
		return false; // trivially outside

	if (ml->a.x < m_x)
		outcode1 |= LEFT;
	else if (ml->a.x > m_x2)
		outcode1 |= RIGHT;

	if (ml->b.x < m_x)
		outcode2 |= LEFT;
	else if (ml->b.x > m_x2)
		outcode2 |= RIGHT;

	if (outcode1 & outcode2)
		return false; // trivially outside

	// transform to frame-buffer coordinates.
	fl->a.x = CXMTOF(ml->a.x);
	fl->a.y = CYMTOF(ml->a.y);
	fl->b.x = CXMTOF(ml->b.x);
	fl->b.y = CYMTOF(ml->b.y);

	DOOUTCODE(outcode1, fl->a.x, fl->a.y);
	DOOUTCODE(outcode2, fl->b.x, fl->b.y);

	if (outcode1 & outcode2)
		return false;

	while (outcode1 | outcode2) {
		// may be partially inside box
		// find an outside point
		if (outcode1)
			outside = outcode1;
		else
			outside = outcode2;
	
		// clip to each side
		if (outside & TOP)
		{
			dy = fl->a.y - fl->b.y;
			dx = fl->b.x - fl->a.x;
			tmp.x = fl->a.x + Scale(dx, fl->a.y, dy);
			tmp.y = 0;
		}
		else if (outside & BOTTOM)
		{
			dy = fl->a.y - fl->b.y;
			dx = fl->b.x - fl->a.x;
			tmp.x = fl->a.x + Scale(dx, fl->a.y - f_h, dy);
			tmp.y = f_h-1;
		}
		else if (outside & RIGHT)
		{
			dy = fl->b.y - fl->a.y;
			dx = fl->b.x - fl->a.x;
			tmp.y = fl->a.y + Scale(dy, f_w-1 - fl->a.x, dx);
			tmp.x = f_w-1;
		}
		else if (outside & LEFT)
		{
			dy = fl->b.y - fl->a.y;
			dx = fl->b.x - fl->a.x;
			tmp.y = fl->a.y + Scale(dy, -fl->a.x, dx);
			tmp.x = 0;
		}

		if (outside == outcode1)
		{
			fl->a = tmp;
			DOOUTCODE(outcode1, fl->a.x, fl->a.y);
		}
		else
		{
			fl->b = tmp;
			DOOUTCODE(outcode2, fl->b.x, fl->b.y);
		}
	
		if (outcode1 & outcode2)
			return false; // trivially outside
	}

	return true;
}

//=============================================================================
//
// Clip lines, draw visible parts of lines.
//
//=============================================================================

void DAutomap::drawMline (mline_t *ml, const AMColor &color)
{
	fline_t fl;

	if (clipMline (ml, &fl))
	{
		const int x1 = f_x + fl.a.x;
		const int y1 = f_y + fl.a.y;
		const int x2 = f_x + fl.b.x;
		const int y2 = f_y + fl.b.y;
		if (am_linethickness >= 2) {
			twod->AddThickLine(x1, y1, x2, y2, am_linethickness, color.RGB, uint8_t(am_linealpha * 255));
		} else {
			// Use more efficient thin line drawing routine.
			twod->AddLine(x1, y1, x2, y2, -1, -1, INT_MAX, INT_MAX, color.RGB, uint8_t(am_linealpha * 255));
		}
	}
}

void DAutomap::drawMline (mline_t *ml, int colorindex)
{
	drawMline(ml, AMColors[colorindex]);
}

//=============================================================================
//
// Draws flat (floor/ceiling tile) aligned grid lines.
//
//=============================================================================

void DAutomap::drawGrid (int color)
{
	double x, y;
	double start, end;
	mline_t ml;
	double minlen, extx, exty;
	double minx, miny;

	// [RH] Calculate a minimum for how long the grid lines should be so that
	// they cover the screen at any rotation.
	minlen = sqrt (m_w*m_w + m_h*m_h);
	extx = (minlen - m_w) / 2;
	exty = (minlen - m_h) / 2;

	minx = m_x;
	miny = m_y;

	// Figure out start of vertical gridlines
	start = int(minx - extx) & ~127;

	end = minx + minlen - extx;

	// draw vertical gridlines
	for (x = start; x < end; x += 128)
	{
		ml.a.x = x;
		ml.b.x = x;
		ml.a.y = miny - exty;
		ml.b.y = ml.a.y + minlen;
		if (am_rotate == 1 || (am_rotate == 2 && viewactive))
		{
			rotatePoint (&ml.a.x, &ml.a.y);
			rotatePoint (&ml.b.x, &ml.b.y);
		}
		drawMline(&ml, color);
	}

	// Figure out start of horizontal gridlines
	start = miny - exty;
	end = int(miny + minlen - exty) & ~127;

	// draw horizontal gridlines
	for (y=start; y<end; y+=128)
	{
		ml.a.x = minx - extx;
		ml.b.x = ml.a.x + minlen;
		ml.a.y = y;
		ml.b.y = y;
		if (am_rotate == 1 || (am_rotate == 2 && viewactive))
		{
			rotatePoint (&ml.a.x, &ml.a.y);
			rotatePoint (&ml.b.x, &ml.b.y);
		}
		drawMline (&ml, color);
	}
}

//=============================================================================
//
// AM_drawSubsectors
//
//=============================================================================


void DAutomap::drawSubsectors()
{
	int width = screen->GetWidth();
	int height = screen->GetHeight();
	TArray<FVector4> vertices;
	TArray<DCoreActor*> floorsprites;
	TArray<FVector2> points;
	
	auto transformPt = [=](const FVector2& pos) -> FVector2
	{
		mpoint_t pt = { pos.X, pos.Y };
		if (am_rotate == 1 || (am_rotate == 2 && viewactive))
		{
			rotatePoint(&pt.x, &pt.y);
		}
		return { float(f_x + ((pt.x - m_x) * scale_mtof)), float(f_y + (f_h - (pt.y - m_y) * scale_mtof)) };
	};
	

	for (int i = (int)sector.Size() - 1; i >= 0; i--)
	{
		auto sect = &sector[i];
		if (am_cheat == 0 && !show2dsector[i]) continue;

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
				auto p = transformPt(mesh->vertices[j]);
				vertices[j] = { p.X, p.Y, mesh->texcoords[j].X, mesh->texcoords[j].Y };
			}

			twod->AddPoly(tileGetTexture(picnum, true), vertices.Data(), vertices.Size(), (unsigned*)indices->Data(), indices->Size(), translation, light,
				LegacyRenderStyles[STYLE_Translucent], windowxy1.X, windowxy1.Y, windowxy2.X + 1, windowxy2.Y + 1);
		}
	}
	qsort(floorsprites.Data(), floorsprites.Size(), sizeof(DCoreActor*), [](const void* a, const void* b)
		{
			auto A = *(DCoreActor**)a;
			auto B = *(DCoreActor**)b;
			if (A->spr.pos.Z != B->spr.pos.Z) return B->spr.pos.Z - A->spr.pos.Z;
			return A->time - B->time; // ensures stable sort.
		});

	vertices.Resize(4);

	for (auto actor : floorsprites)
	{
		if (am_cheat == 0 && !(actor->spr.cstat2 & CSTAT2_SPRITE_MAPPED)) continue;
		vec2_t pp[4];
		GetFlatSpritePosition(actor, actor->spr.pos.vec2, pp, true);

		for (unsigned j = 0; j < 4; j++)
		{
			auto p = transformPt(FVector2(pp[j].X * maptoworld, pp[j].Y * maptoworld));
			vertices[j] = { p.X, p.Y, j == 1 || j == 2 ? 1.f : 0.f, j == 2 || j == 3 ? 1.f : 0.f };
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
		twod->AddPoly(tileGetTexture(picnum, true), vertices.Data(), vertices.Size(), indices, 6, translation, color, rs,
			windowxy1.X, windowxy1.Y, windowxy2.X + 1, windowxy2.Y + 1);
	}
}

//=============================================================================
//
//
//
//=============================================================================

static int AM_CheckSecret(walltype *line)
{
	if (AMColors.isValid(AMColors.SecretSectorColor))
	{
		// todo
	}
	return 0;
}


//=============================================================================
//
// Determines visible lines, draws them.
// This is LineDef based, not LineSeg based.
//
//=============================================================================

void DAutomap::drawWalls (bool allmap)
{
	static mline_t l;
	int lock, color;

	for(unsigned i = 0; i < wall.Size(); i++)
	{
		auto& line = wall[i];
		if ((unsigned)line.nextwall < i) continue;

		DVector2 offset = { 0, 0 };
		
		l.a.x = (line.pos.X);
		l.a.y = (line.pos.Y);
		l.b.x = (line.point2Wall()->pos.X);
		l.b.y = (line.point2Wall()-> pos.Y);
		
		if (am_rotate == 1 || (am_rotate == 2 && viewactive))
		{
			rotatePoint(&l.a.x, &l.a.y);
			rotatePoint(&l.b.x, &l.b.y);
		}
		
		if (am_cheat != 0 || show2dwall[i])
		{
			if (AM_CheckSecret(&line) == 1)
			{
				// map secret sectors like Boom
				drawMline(&l, AMColors.SecretSectorColor);
			}
			else if (AM_CheckSecret(&line) == 2)
			{
				drawMline(&l, AMColors.UnexploredSecretColor);
			}
			else if (line.nextwall == -1)
			{
				drawMline(&l, AMColors.WallColor);	// one-sided wall
			}
			else if (line.nextSector()->floorz != line.sectorp()->floorz)
			{
				drawMline(&l, AMColors.FDWallColor); // floor level change
			}
			else if (line.nextSector()->ceilingz != line.sectorp()->ceilingz)
			{
				drawMline(&l, AMColors.CDWallColor); // ceiling level change
			}
			else if (am_cheat > 0 && am_cheat < 4)
			{
				drawMline(&l, AMColors.TSWallColor);
			}
		}
		else if (allmap || show2dwall[i])
		{
			drawMline(&l, AMColors.NotSeenColor);
		}
	}
}


//=============================================================================
//
// Rotation in 2D.
// Used to rotate player arrow line character.
//
//=============================================================================

void DAutomap::rotate(double *xp, double *yp, DAngle a)
{
	static DAngle angle_saved = 0.;
	static double sinrot = 0;
	static double cosrot = 1;

	if (angle_saved != a)
	{
		angle_saved = a;
		sinrot = sin(a.Radians());
		cosrot = cos(a.Radians());
	}

	double x = *xp;
	double y = *yp;
	double tmpx = (x * cosrot) - (y * sinrot);
	y = (x * sinrot) + (y * cosrot);
	x = tmpx;
	*xp = x;
	*yp = y;
}

//=============================================================================
//
//
//
//=============================================================================

void DAutomap::rotatePoint (double *x, double *y)
{
	double pivotx = m_x + m_w/2;
	double pivoty = m_y + m_h/2;
	*x -= pivotx;
	*y -= pivoty;
	rotate (x, y, playerAngle);
	*x += pivotx;
	*y += pivoty;
}

//=============================================================================
//
//
//
//=============================================================================

void DAutomap::drawLineCharacter(const mline_t *lineguy, size_t lineguylines, double scale, DAngle angle, const AMColor &color, double x, double y)
{
	mline_t	l;

	for (size_t i=0;i<lineguylines;i++)
	{
		l.a.x = lineguy[i].a.x;
		l.a.y = lineguy[i].a.y;

		if (scale)
		{
			l.a.x *= scale;
			l.a.y *= scale;
		}

		if (angle != 0)
			rotate(&l.a.x, &l.a.y, angle);

		l.a.x += x;
		l.a.y += y;

		l.b.x = lineguy[i].b.x;
		l.b.y = lineguy[i].b.y;

		if (scale)
		{
			l.b.x *= scale;
			l.b.y *= scale;
		}

		if (angle != 0)
			rotate(&l.b.x, &l.b.y, angle);

		l.b.x += x;
		l.b.y += y;

		drawMline(&l, color);
	}
}

//=============================================================================
//
//
//
//=============================================================================

void DAutomap::drawPlayers ()
{
#if 0
	if (am_cheat >= 2 && am_cheat != 4 && am_showthingsprites > 0)
	{
		// Player sprites are drawn with the others
		return;
	}

	mpoint_t pt;
	DAngle angle;
	int i;

	if (!multiplayer)
	{
		mline_t *arrow;
		int numarrowlines;

		double vh = players[consoleplayer].viewheight;
		DVector2 pos = players[consoleplayer].camera->InterpolatedPosition(r_viewpoint.TicFrac);
		pt.x = pos.X;
		pt.y = pos.Y;
		if (am_rotate == 1 || (am_rotate == 2 && viewactive))
		{
			angle = 90.;
			rotatePoint (&pt.x, &pt.y);
		}
		else
		{
			angle = players[consoleplayer].camera->InterpolatedAngles(r_viewpoint.TicFrac).Yaw;
		}
		
		if (am_cheat != 0 && CheatMapArrow.Size() > 0)
		{
			arrow = &CheatMapArrow[0];
			numarrowlines = CheatMapArrow.Size();
		}
		else
		{
			arrow = &MapArrow[0];
			numarrowlines = MapArrow.Size();
		}
		drawLineCharacter(arrow, numarrowlines, 0, angle, AMColors[AMColors.YourColor], pt.x, pt.y);
		return;
	}

	for (i = 0; i < MAXPLAYERS; i++)
	{
		player_t *p = &players[i];
		AMColor color;

		if (!playeringame[i] || p->mo == nullptr)
		{
			continue;
		}

		// We don't always want to show allies on the automap.
		if (dmflags2 & DF2_NO_AUTOMAP_ALLIES && i != consoleplayer)
			continue;
		
		if (deathmatch && !demoplayback &&
			!p->mo->IsTeammate (players[consoleplayer].mo) &&
			p != players[consoleplayer].camera->player)
		{
			continue;
		}

		if (p->mo->Alpha < 1.)
		{
			color = AMColors[AMColors.AlmostBackgroundColor];
		}
		else
		{
			float h, s, v, r, g, b;

			D_GetPlayerColor (i, &h, &s, &v, nullptr);
			HSVtoRGB (&r, &g, &b, h, s, v);

			color.FromRGB(clamp (int(r*255.f),0,255), clamp (int(g*255.f),0,255), clamp (int(b*255.f),0,255));
		}

		if (p->mo != nullptr)
		{
			DVector3 pos = p->mo->PosRelative(MapPortalGroup);
			pt.x = pos.X;
			pt.y = pos.Y;

			angle = p->mo->InterpolatedAngles(r_viewpoint.TicFrac).Yaw;

			if (am_rotate == 1 || (am_rotate == 2 && viewactive))
			{
				rotatePoint (&pt.x, &pt.y);
				angle -= players[consoleplayer].camera->InterpolatedAngles(r_viewpoint.TicFrac).Yaw - 90.;
			}

			drawLineCharacter(&MapArrow[0], MapArrow.Size(), 0, angle, color, pt.x, pt.y);
		}
    }
#endif
}

//=============================================================================
//
//
//
//=============================================================================

void DAutomap::drawKeys ()
{
	AMColor color;
	mpoint_t p;
	DAngle	 angle;

	TSpriteIterator<DCoreActor> it;
	DCoreActor *key;

	while ((key = it.Next()) != nullptr)
	{
		DVector3 pos = key->float_pos();
		p.x = pos.X;
		p.y = pos.Y;

		angle = buildang(key->spr.ang).asdeg();

		if (am_rotate == 1 || (am_rotate == 2 && viewactive))
		{
			rotatePoint (&p.x, &p.y);
			angle += playerAngle;
		}

		// Find the key's own color.
		// Only works correctly if single-key locks have lower numbers than any-key locks.
		// That is the case for all default keys, however.
		int c = 0;// P_GetMapColorForKey(key);
		
		if (c >= 0)
		{
			color.FromRGB(RPART(c), GPART(c), BPART(c));
			drawLineCharacter(&EasyKey[0], EasyKey.Size(), 0, 0., color, p.x, p.y);
		}
	}
}

//=============================================================================
//
//
//
//=============================================================================
void DAutomap::drawThings ()
{
#if 0
	AMColor color;
	AActor*	 t;
	mpoint_t p;
	DAngle	 angle;

	for (auto &sec : Level->sectors)
	{
		t = sec.thinglist;
		while (t)
		{
			if (am_cheat > 0 || !(t->flags6 & MF6_NOTONAUTOMAP)
				|| (am_thingrenderstyles && !(t->renderflags & RF_INVISIBLE) && !(t->flags6 & MF6_NOTONAUTOMAP)))
			{
				DVector3 pos = t->InterpolatedPosition(r_viewpoint.TicFrac) + t->Level->Displacements.getOffset(sec.PortalGroup, MapPortalGroup);
				p.x = pos.X;
				p.y = pos.Y;

				if (am_showthingsprites > 0 && t->sprite > 0)
				{
					FGameTexture *texture = nullptr;
					spriteframe_t *frame;
					int rotation = 0;

					// try all modes backwards until a valid texture has been found.	
					for(int show = am_showthingsprites; show > 0 && texture == nullptr; show--)
					{
						const spritedef_t& sprite = sprites[t->sprite];
						const size_t spriteIndex = sprite.spriteframes + (show > 1 ? t->frame : 0);

						frame = &SpriteFrames[spriteIndex];
						DAngle angle = 270. + 22.5 - t->InterpolatedAngles(r_viewpoint.TicFrac).Yaw;
						if (frame->Texture[0] != frame->Texture[1]) angle += 180. / 16;
						if (am_rotate == 1 || (am_rotate == 2 && viewactive))
						{
							angle += players[consoleplayer].camera->InterpolatedAngles(r_viewpoint.TicFrac).Yaw - 90.;
						}
						rotation = int((angle.Normalized360() * (16. / 360.)).Degrees);

						const FTextureID textureID = frame->Texture[show > 2 ? rotation : 0];
						texture = TexMan.GetGameTexture(textureID, true);
					}

					if (texture == nullptr) goto drawTriangle;	// fall back to standard display if no sprite can be found.

					const double spriteXScale = (t->Scale.X * (10. / 16.) * scale_mtof);
					const double spriteYScale = (t->Scale.Y * (10. / 16.) * scale_mtof);

					if (am_thingrenderstyles) DrawMarker(texture, p.x, p.y, 0, !!(frame->Flip & (1 << rotation)),
						spriteXScale, spriteYScale, t->Translation, t->Alpha, t->fillcolor, t->RenderStyle);
					else DrawMarker(texture, p.x, p.y, 0, !!(frame->Flip & (1 << rotation)),
						spriteXScale, spriteYScale, t->Translation, 1., 0, LegacyRenderStyles[STYLE_Normal]);
				}
				else
				{
			drawTriangle:
					angle = t->InterpolatedAngles(r_viewpoint.TicFrac).Yaw;

					if (am_rotate == 1 || (am_rotate == 2 && viewactive))
					{
						rotatePoint (&p.x, &p.y);
						angle += -players[consoleplayer].camera->InterpolatedAngles(r_viewpoint.TicFrac).Yaw + 90.;
					}

					color = AMColors[AMColors.ThingColor];

					// use separate colors for special thing types
					if (t->flags3&MF3_ISMONSTER && !(t->flags&MF_CORPSE))
					{
						if (t->flags & MF_FRIENDLY) color = AMColors[AMColors.ThingColor_Friend];
						else if (!(t->flags & MF_COUNTKILL)) color = AMColors[AMColors.ThingColor_NocountMonster];
						else color = AMColors[AMColors.ThingColor_Monster];
					}
					else if (t->flags&MF_SPECIAL)
					{
						// Find the key's own color.
						// Only works correctly if single-key locks have lower numbers than any-key locks.
						// That is the case for all default keys, however.
						if (t->IsKindOf(NAME_Key))
						{
							if (G_SkillProperty(SKILLP_EasyKey) || am_showkeys_always)
							{
								// Already drawn by AM_drawKeys(), so don't draw again
								color.RGB = 0;
							}
							else if (am_showkeys)
							{
								int c = P_GetMapColorForKey(t);

								if (c >= 0)	color.FromRGB(RPART(c), GPART(c), BPART(c));
								else color = AMColors[AMColors.ThingColor_CountItem];
								drawLineCharacter(&CheatKey[0], CheatKey.Size(), 0, 0., color, p.x, p.y);
								color.RGB = 0;
							}
							else
							{
								color = AMColors[AMColors.ThingColor_Item];
							}
						}
						else if (t->flags&MF_COUNTITEM)
							color = AMColors[AMColors.ThingColor_CountItem];
						else
							color = AMColors[AMColors.ThingColor_Item];
					}

					if (color.isValid())
					{
						drawLineCharacter(thintriangle_guy.data(), thintriangle_guy.size(), 16, angle, color, p.x, p.y);
					}

					if (am_cheat == 3 || am_cheat == 6)
					{
						static const mline_t box[4] =
						{
							{ { -1, -1 }, {  1, -1 } },
							{ {  1, -1 }, {  1,  1 } },
							{ {  1,  1 }, { -1,  1 } },
							{ { -1,  1 }, { -1, -1 } },
						};

						drawLineCharacter (box, 4, t->radius, angle - t->InterpolatedAngles(r_viewpoint.TicFrac).Yaw, color, p.x, p.y);
					}
				}
			}
			t = t->snext;
		}
	}
#endif
}

//=============================================================================
//
//
//
//=============================================================================

void DAutomap::DrawMarker (FGameTexture *tex, double x, double y, int yadjust,
	INTBOOL flip, double xscale, double yscale, int translation, double alpha, uint32_t fillcolor, FRenderStyle renderstyle)
{
	if (tex == nullptr || !tex->isValid())
	{
		return;
	}
	if (xscale < 0)
	{
		flip = !flip;
		xscale = -xscale;
	}
	if (am_rotate == 1 || (am_rotate == 2 && viewactive))
	{
		rotatePoint (&x, &y);
	}
	DrawTexture(twod, tex, CXMTOF(x) + f_x, CYMTOF(y) + yadjust + f_y,
		DTA_DestWidthF, tex->GetDisplayWidth() * CleanXfac * xscale,
		DTA_DestHeightF, tex->GetDisplayHeight() * CleanYfac * yscale,
		DTA_ClipTop, f_y,
		DTA_ClipBottom, f_y + f_h,
		DTA_ClipLeft, f_x,
		DTA_ClipRight, f_x + f_w,
		DTA_FlipX, flip,
		DTA_TranslationIndex, translation,
		DTA_Alpha, alpha,
		DTA_FillColor, fillcolor,
		DTA_RenderStyle, renderstyle.AsDWORD,
		TAG_DONE);
}

//=============================================================================
//
//
//
//=============================================================================

void DAutomap::drawMarks ()
{
	FFont* font;
	bool fontloaded = false;

	for (int i = 0; i < AM_NUMMARKPOINTS; i++)
	{
		if (markpoints[i].x != -1)
		{
			if (!fontloaded)
			{
				font = stricmp(*am_markfont, DEFAULT_FONT_NAME) == 0 ? nullptr : V_GetFont(am_markfont);
				fontloaded = true;
			}

			if (font == nullptr)
			{
				DrawMarker(TexMan.GetGameTexture(marknums[i], true), markpoints[i].x, markpoints[i].y, -3, 0,
					1, 1, 0, 1, 0, LegacyRenderStyles[STYLE_Normal]);
			}
			else
			{
				char numstr[2] = { char('0' + i), 0 };
				double x = markpoints[i].x;
				double y = markpoints[i].y;

				if (am_rotate == 1 || (am_rotate == 2 && viewactive))
				{
					rotatePoint (&x, &y);
				}

				DrawText(twod, font, am_markcolor, CXMTOF(x), CYMTOF(y), numstr, TAG_DONE);
			}
		}
	}
}

//=============================================================================
//
//
//
//=============================================================================

void DAutomap::drawAuthorMarkers ()
{
#if 0
	// [RH] Draw any actors derived from AMapMarker on the automap.
	// If args[0] is 0, then the actor's sprite is drawn at its own location.
	// Otherwise, its sprite is drawn at the location of any actors whose TIDs match args[0].
	auto it = Level->GetThinkerIterator<AActor>(NAME_MapMarker, STAT_MAPMARKER);
	AActor *mark;

	while ((mark = it.Next()) != nullptr)
	{
		if (mark->flags2 & MF2_DORMANT)
		{
			continue;
		}

		FTextureID picnum;
		FGameTexture *tex;
		uint16_t flip = 0;

		if (mark->picnum.isValid())
		{
			tex = TexMan.GetGameTexture(mark->picnum, true);
			if (tex->GetRotations() != 0xFFFF)
			{
				spriteframe_t *sprframe = &SpriteFrames[tex->GetRotations()];
				picnum = sprframe->Texture[0];
				flip = sprframe->Flip & 1;
				tex = TexMan.GetGameTexture(picnum);
			}
		}
		else
		{
			spritedef_t *sprdef = &sprites[mark->sprite];
			if (mark->frame >= sprdef->numframes)
			{
				continue;
			}
			else
			{
				spriteframe_t *sprframe = &SpriteFrames[sprdef->spriteframes + mark->frame];
				picnum = sprframe->Texture[0];
				flip = sprframe->Flip & 1;
				tex = TexMan.GetGameTexture(picnum);
			}
		}
		auto it = Level->GetActorIterator(mark->args[0]);
		AActor *marked = mark->args[0] == 0 ? mark : it.Next();

		double xscale = mark->Scale.X;
		double yscale = mark->Scale.Y;
		// [MK] scale with automap zoom if args[2] is 1, otherwise keep a constant scale
		if (mark->args[2] == 1)
		{
			xscale = MTOF(xscale);
			yscale = MTOF(yscale);
		}

		while (marked != nullptr)
		{
			if (mark->args[1] == 0 || (mark->args[1] == 1 && (marked->subsector->flags & SSECMF_DRAWN)))
			{
				DrawMarker (tex, marked->X(), marked->Y(), 0, flip, xscale, yscale, mark->Translation,
					mark->Alpha, mark->fillcolor, mark->RenderStyle);
			}
			marked = mark->args[0] != 0 ? it.Next() : nullptr;
		}
	}
#endif
}

//=============================================================================
//
//
//
//=============================================================================

void DAutomap::drawCrosshair (const AMColor &color)
{
	twod->AddPixel(f_w/2, (f_h+1)/2, color.RGB);
}

//=============================================================================
//
//
//
//=============================================================================

void DAutomap::Drawer (int bottom)
{
	static uint64_t LastMS = 0;
	// Use a delta to zoom/pan at a constant speed regardless of current FPS
	uint64_t ms = screen->FrameTime;
	double delta = (ms - LastMS) * 0.001;

	if (!automapactive)
		return;

	if (am_followplayer)
	{
		doFollowPlayer();
	}
	else
	{
		m_paninc.x = m_paninc.y = 0;
		if (buttonMap.ButtonDown(gamefunc_AM_PanLeft))
			m_paninc.x -= FTOM(F_PANINC) * delta * GameTicRate;
		if (buttonMap.ButtonDown(gamefunc_AM_PanRight))
			m_paninc.x += FTOM(F_PANINC) * delta * GameTicRate;
		if (buttonMap.ButtonDown(gamefunc_AM_PanUp))
			m_paninc.y += FTOM(F_PANINC) * delta * GameTicRate;
		if (buttonMap.ButtonDown(gamefunc_AM_PanDown))
			m_paninc.y -= FTOM(F_PANINC) * delta * GameTicRate;
	}

	// Change the zoom if necessary
	if (buttonMap.ButtonDown(gamefunc_Enlarge_Screen) || buttonMap.ButtonDown(gamefunc_Shrink_Screen) || am_zoomdir != 0)
		changeWindowScale(delta);

	// Change x,y location
	changeWindowLoc();

	bool allmap = false;
	bool allthings = false;

	AM_initColors (viewactive);

	if (!viewactive)
	{
		// [RH] Set f_? here now to handle automap overlaying
		// and view size adjustments.
		f_x = f_y = 0;
		f_w = twod->GetWidth ();
		f_h = bottom;

		clearFB(AMColors[AMColors.Background]);
	}
	else 
	{
		f_x = windowxy1.X;
		f_y = windowxy1.Y;
		f_w = windowxy2.X - f_x;
		f_h = windowxy2.Y - f_y;
	}
	activateNewScale();

	if (am_textured && !viewactive)
		drawSubsectors();

	if (am_showgrid)	
		drawGrid(AMColors.GridColor);

	drawWalls(allmap);
	drawPlayers();
	if (am_showkeys_always)
		drawKeys();
	if ((am_cheat >= 2 && am_cheat != 4) || allthings)
		drawThings();

	drawAuthorMarkers();

	if (!viewactive)
		drawCrosshair(AMColors[AMColors.XHairColor]);

	drawMarks();

	LastMS = ms;
}

//=============================================================================
//
//
//
//=============================================================================

void DAutomap::Serialize(FSerializer &arc)
{
	Super::Serialize(arc);
	// This only stores those variables which do not get set each time the automap is either activated or drawn.
	// Especially the screen coordinates can not be brought over because the display settings may have changed.
	arc("markpointnum", markpointnum)
		.Array("markpoints", &markpoints[0].x, AM_NUMMARKPOINTS * 2)	// write as a double array.
		("scale_mtof", scale_mtof)
		("scale_ftom", scale_ftom)
		("bigstate", bigstate)
		("min_x", min_x)
		("min_y", min_y)
		("max_x", max_x)
		("max_y", max_y)
		("min_w", min_w)
		("min_h", min_h)
		("max_w", max_w)
		("max_h", max_h)
		("min_scale_mtof", min_scale_mtof)
		("max_scale_mtof", max_scale_mtof)
		("mapback", mapback);


}


//=============================================================================
//
//
//
//=============================================================================

void DAutomap::UpdateShowAllLines()
{
}

void DAutomap::GoBig()
{
	bigstate = !bigstate;
	if (bigstate)
	{
		saveScaleAndLoc();
		minOutWindowScale();
	}
	else
		restoreScaleAndLoc();
}

void DAutomap::ResetFollowLocation()
{
	f_oldloc.x = FLT_MAX;
}


//=============================================================================
//
//
//
//=============================================================================

void AM_Stop()
{
	automapactive = false;
	automap->viewactive = true;
}

//=============================================================================
//
//
//
//=============================================================================

void AM_ToggleMap()
{
	if (gamestate != GS_LEVEL)
		return;

	if (!automapactive)
	{
		// Reset AM buttons
		buttonMap.ClearButton(gamefunc_AM_PanLeft);
		buttonMap.ClearButton(gamefunc_AM_PanRight);
		buttonMap.ClearButton(gamefunc_AM_PanUp);
		buttonMap.ClearButton(gamefunc_AM_PanDown);
		buttonMap.ClearButton(gamefunc_Shrink_Screen);
		buttonMap.ClearButton(gamefunc_Enlarge_Screen);

		automap->startDisplay();
		automapactive = true;
		automap->viewactive = (am_overlay != 0.f);
	}
	else
	{
		if (am_overlay == 1 && automap->viewactive)
		{
			automap->viewactive = false;
		}
		else
		{
			AM_Stop();
		}
	}
}

//=============================================================================
//
//
//
//=============================================================================

DAutomapBase *AM_Create(FLevelLocals *Level)
{
	auto am = Create<DAutomap>();
	return am;
}
