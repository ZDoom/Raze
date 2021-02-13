/*
** shared_sbar.cpp
** Base status bar implementation
**
**---------------------------------------------------------------------------
** Copyright 1998-2006 Randy Heit
** Copyright 2017 Christoph Oelckers
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

#include <assert.h>

#include "build.h"
#include "templates.h"
#include "statusbar.h"
#include "c_cvars.h"
#include "c_dispatch.h"
#include "c_console.h"
#include "v_video.h"
#include "filesystem.h"
#include "s_soundinternal.h"
#include "serializer.h"
#include "serialize_obj.h"
#include "cmdlib.h"
#include "vm.h"
#include "gstrings.h"
#include "utf8.h"
#include "texturemanager.h"
#include "cmdlib.h"
#include "v_draw.h"
#include "v_font.h"
#include "v_draw.h"
#include "gamecvars.h"
#include "m_fixed.h"
#include "gamecontrol.h"
#include "gamestruct.h"
#include "razemenu.h"
#include "mapinfo.h"

#include "../version.h"

#define XHAIRSHRINKSIZE		(1./18)
#define XHAIRPICKUPSIZE		(2+XHAIRSHRINKSIZE)
#define POWERUPICONSIZE		32

//IMPLEMENT_CLASS(DHUDFont, true, false);

EXTERN_CVAR (Bool, am_showmonsters)
EXTERN_CVAR (Bool, am_showsecrets)
EXTERN_CVAR (Bool, am_showtime)
EXTERN_CVAR (Bool, am_showtotaltime)
EXTERN_CVAR (Bool, noisedebug)
EXTERN_CVAR(Bool, vid_fps)
EXTERN_CVAR(Bool, inter_subtitles)

//extern DBaseStatusBar *StatusBar;

extern int setblocks;

IMPLEMENT_CLASS(DBaseStatusBar, true, false)
//---------------------------------------------------------------------------
// ST_Clear
//
//---------------------------------------------------------------------------

void ST_Clear()
{
	/*
	if (StatusBar != NULL)
	{
		delete StatusBar;
		StatusBar = NULL;
	}
	*/
}

//---------------------------------------------------------------------------
//
// Constructor
//
//---------------------------------------------------------------------------
DBaseStatusBar::DBaseStatusBar ()
{
	CompleteBorder = false;
	Centering = false;
	FixedOrigin = false;
	SetSize(0);
}

//---------------------------------------------------------------------------
//
// PROC Tick
//
//---------------------------------------------------------------------------

void DBaseStatusBar::Tick ()
{
}


static DObject *InitObject(PClass *type, int paramnum, VM_ARGS)
{
	auto obj =  type->CreateNew();
	// Todo: init
	return obj;
}

//============================================================================
//
//
//
//============================================================================

void DBaseStatusBar::PrintLevelStats(FLevelStats &stats)
{
	double y;
	double scale = stats.fontscale * hud_statscale;
	if (stats.spacing <= 0) stats.spacing = stats.font->GetHeight() * stats.fontscale;
	double spacing = stats.spacing * hud_statscale;
	if (stats.screenbottomspace < 0)
	{
		y = 200 - (RelTop - stats.screenbottomspace) * hud_scalefactor - spacing;
	}
	else
	{
		y = 200 - stats.screenbottomspace * hud_scalefactor - spacing;
	}

	double y1, y2, y3;

	if (stats.maxsecrets > 0)	// don't bother if there are no secrets.
	{
		y1 = y;
		y -= spacing;
	}
	if (stats.frags >= 0 || stats.maxkills != -1)
	{
		y2 = y;
		y -= spacing;
	}
	y3 = y;


	FString text;
	int black = 0x80000000;

	text.Format(TEXTCOLOR_ESCAPESTR "%cT: " TEXTCOLOR_ESCAPESTR "%c%d:%02d", stats.letterColor + 'A', stats.standardColor + 'A', stats.time / 60000, (stats.time % 60000) / 1000);
	DrawText(twod, stats.font, CR_UNTRANSLATED, 2 * hud_statscale + scale, y3 + scale, text, DTA_FullscreenScale, FSMode_ScaleToHeight, DTA_VirtualWidth, 320, DTA_VirtualHeight, 200,
		DTA_KeepRatio, true, DTA_ScaleX, scale, DTA_ScaleY, scale, DTA_LegacyRenderStyle, STYLE_TranslucentStencil, DTA_Color, black, TAG_DONE);
	DrawText(twod, stats.font, CR_UNTRANSLATED, 2 * hud_statscale, y3, text, DTA_FullscreenScale, FSMode_ScaleToHeight, DTA_VirtualWidth, 320, DTA_VirtualHeight, 200,
		DTA_KeepRatio, true, DTA_ScaleX, scale, DTA_ScaleY, scale, TAG_DONE);

	text = "";
	if (stats.frags > -1) text.Format(TEXTCOLOR_ESCAPESTR "%cF: " TEXTCOLOR_ESCAPESTR "%c%d", stats.letterColor + 'A', stats.standardColor + 'A', stats.frags);
	else if (stats.maxkills == -2) text.Format(TEXTCOLOR_ESCAPESTR "%cK: " TEXTCOLOR_ESCAPESTR "%c%d", stats.letterColor + 'A', stats.standardColor + 'A', stats.kills);
	else if (stats.maxkills != -1) text.Format(TEXTCOLOR_ESCAPESTR "%cK: " TEXTCOLOR_ESCAPESTR "%c%d/%d",
		stats.letterColor + 'A', stats.kills == stats.maxkills ? stats.completeColor + 'A' : stats.standardColor + 'A', stats.kills, stats.maxkills);

	if (text.IsNotEmpty())
	{
		DrawText(twod, stats.font, CR_UNTRANSLATED, 2 * hud_statscale+scale, y2+scale, text, DTA_FullscreenScale, FSMode_ScaleToHeight, DTA_VirtualWidth, 320, DTA_VirtualHeight, 200,
			DTA_KeepRatio, true, DTA_ScaleX, scale, DTA_ScaleY, scale, DTA_LegacyRenderStyle, STYLE_TranslucentStencil, DTA_Color, black, TAG_DONE);

		DrawText(twod, stats.font, CR_UNTRANSLATED, 2 * hud_statscale, y2, text, DTA_FullscreenScale, FSMode_ScaleToHeight, DTA_VirtualWidth, 320, DTA_VirtualHeight, 200,
			DTA_KeepRatio, true, DTA_ScaleX, scale, DTA_ScaleY, scale, TAG_DONE);
	}

	if (stats.maxsecrets > 0)	// don't bother if there are no secrets.
	{
		if (stats.supersecrets <= 0)
			text.Format(TEXTCOLOR_ESCAPESTR "%cS: " TEXTCOLOR_ESCAPESTR "%c%d/%d",
				stats.letterColor + 'A', stats.secrets >= stats.maxsecrets ? stats.completeColor + 'A' : stats.standardColor + 'A', stats.secrets, stats.maxsecrets);
		else
			text.Format(TEXTCOLOR_ESCAPESTR "%cS: " TEXTCOLOR_ESCAPESTR "%c%d/%d+%d",
				stats.letterColor + 'A', stats.secrets >= stats.maxsecrets ? stats.completeColor + 'A' : stats.standardColor + 'A', stats.secrets, stats.maxsecrets, stats.supersecrets);


		DrawText(twod, stats.font, CR_UNTRANSLATED, 2 * hud_statscale + scale, y1 + scale, text, DTA_FullscreenScale, FSMode_ScaleToHeight, DTA_VirtualWidth, 320, DTA_VirtualHeight, 200,
			DTA_KeepRatio, true, DTA_ScaleX, scale, DTA_ScaleY, scale, DTA_LegacyRenderStyle, STYLE_TranslucentStencil, DTA_Color, black, TAG_DONE);

		DrawText(twod, stats.font, CR_UNTRANSLATED, 2 * hud_statscale, y1, text, DTA_FullscreenScale, FSMode_ScaleToHeight, DTA_VirtualWidth, 320, DTA_VirtualHeight, 200,
			DTA_KeepRatio, true, DTA_ScaleX, scale, DTA_ScaleY, scale, TAG_DONE);
	}
}

//============================================================================
//
//
//
//============================================================================

void DBaseStatusBar::PrintAutomapInfo(FLevelStats& stats, bool forcetextfont)
{
	auto lev = currentLevel;
	FString mapname;
	if (am_showlabel) 
		mapname.Format(TEXTCOLOR_ESCAPESTR "%c%s: " TEXTCOLOR_ESCAPESTR "%c%s", stats.letterColor+'A', lev->LabelName(), stats.standardColor+'A', lev->DisplayName());
	else 
		mapname = lev->DisplayName();

	forcetextfont |= am_textfont;
	double y;
	double scale = stats.fontscale * (forcetextfont ? *hud_statscale : 1);	// the tiny default font used by all games here cannot be scaled for readability purposes.
	if (stats.spacing <= 0) stats.spacing = stats.font->GetHeight() * stats.fontscale;
	double spacing = stats.spacing * (forcetextfont ? *hud_statscale : 1);
	if (am_nameontop)
	{
		y = spacing + 1;
	}
	else if (stats.screenbottomspace < 0)
	{
		y = 200 - RelTop - spacing;
	}
	else
	{
		y = 200 - stats.screenbottomspace - spacing;
	}
	const auto &volname = gVolumeNames[volfromlevelnum(lev->levelNumber)];
	if (volname.IsEmpty() && am_nameontop) y = 1;

	DrawText(twod, stats.font, stats.standardColor, 2 * hud_statscale, y, mapname, DTA_FullscreenScale, FSMode_ScaleToHeight, DTA_VirtualWidth, 320, DTA_VirtualHeight, 200,
		DTA_ScaleX, scale, DTA_ScaleY, scale, DTA_KeepRatio, true, TAG_DONE);
	y -= spacing;
	if (!(lev->flags & MI_USERMAP) && !(g_gameType & GAMEFLAG_PSEXHUMED) && volname.IsNotEmpty())
		DrawText(twod, stats.font, stats.standardColor, 2 * hud_statscale, y, GStrings.localize(volname),
			DTA_FullscreenScale, FSMode_ScaleToHeight, DTA_VirtualWidth, 320, DTA_VirtualHeight, 200,
			DTA_ScaleX, scale, DTA_ScaleY, scale, DTA_KeepRatio, true, TAG_DONE);
}

//============================================================================
//
// 
//
//============================================================================

short DBaseStatusBar::CalcMagazineAmount(short ammo_remaining, short clip_capacity, bool reloading)
{
	// Determine amount in clip.
	short clip_amount = ammo_remaining % clip_capacity;

	// Set current clip value to clip capacity if wrapped around to zero, otherwise use determined value.
	short clip_current = ammo_remaining != 0 && clip_amount == 0 ? clip_capacity : clip_amount;

	// Return current clip value if weapon has rounds or is not on a reload cycle.
	return ammo_remaining == 0 || (reloading && clip_amount == 0) ? 0 : clip_current;
}

//============================================================================
//
// 
//
//============================================================================

void DBaseStatusBar::Set43ClipRect()
{
	auto GetWidth = [=]() { return twod->GetWidth(); };
	auto GetHeight = [=]() {return twod->GetHeight(); };

	auto screenratio = ActiveRatio(GetWidth(), GetHeight());
	if (screenratio < 1.34) return;

	int width = xs_CRoundToInt(GetWidth() * 1.333 / screenratio);
	int left = (GetWidth() - width) / 2;
	twod->SetClipRect(left, 0, width, GetHeight());
}

//============================================================================
//
// 
//
//============================================================================

void setViewport(int viewSize)
{
	int x0, y0, x1, y1;
	if (screen == nullptr) return;
	int xdim = screen->GetWidth();
	int ydim = screen->GetHeight();
	if (xdim == 0 || ydim == 0) return;
	auto reserved = gi->GetReservedScreenSpace(viewSize);
	reserved.top = xs_CRoundToInt((reserved.top * hud_scalefactor * ydim) / 200);
	reserved.statusbar = xs_CRoundToInt((reserved.statusbar * hud_scalefactor * ydim) / 200);

	int xdimcorrect = std::min(Scale(ydim, 4, 3), xdim);
	if (viewSize > Hud_Stbar)
	{
		x0 = 0;
		x1 = xdim - 1;
		y0 = reserved.top;
		y1 = ydim - 1;
	}
	else
	{
		x0 = 0;
		y0 = reserved.top;
		x1 = xdim - 1;
		y1 = ydim - 1 - reserved.statusbar;

		int height = y1 - y0;
		int frameheight = (height * (5 - viewSize) / 20);
		int framewidth = Scale(frameheight, xdim, y1+1);
		x0 += framewidth;
		x1 -= framewidth;
		y0 += frameheight;
		y1 -= frameheight;
	}
	videoSetViewableArea(x0, y0, x1, y1);
}

//============================================================================
//
//
//
//============================================================================

int levelTextTime;

void SerializeHud(FSerializer &arc)
{
	if (arc.BeginObject("hud"))
	{
		arc("texttimer", levelTextTime)
		.EndObject();
	}
}

void setLevelStarted(MapRecord *mi)
{
	levelTextTime = 85;
	Printf(PRINT_NONOTIFY, TEXTCOLOR_GOLD "%s: %s\n", mi->LabelName(), mi->DisplayName());
}

void drawMapTitle()
{
    if (!hud_showmapname || levelTextTime <= 0 || M_Active())
        return;
	
	double alpha = levelTextTime > 16? 1.0 : levelTextTime / 16.;
    if (alpha > 0)
    {
		double scale = (g_gameType & GAMEFLAG_RRALL)? 0.4 : (g_gameType & GAMEFLAG_SW)? 0.7 : 1.0;
		auto text = currentLevel->DisplayName();
		double x = 160 - BigFont->StringWidth(text) * scale / 2.;
		double y = isBlood() ? 50 : 100 - BigFont->GetHeight()/2.;
		bool shadow = true;

		if (shadow)
		{
			DrawText(twod, BigFont, CR_UNDEFINED, x+1, y+1, text, DTA_FullscreenScale, FSMode_Fit320x200, DTA_Color, 0xff000000, DTA_Alpha, alpha / 2., DTA_ScaleX, scale, DTA_ScaleY, scale, TAG_DONE);
		}
		DrawText(twod, BigFont, CR_UNDEFINED, x, y, text, DTA_FullscreenScale, FSMode_Fit320x200, DTA_Alpha, alpha, DTA_ScaleX, scale, DTA_ScaleY, scale, TAG_DONE);
    }
}



