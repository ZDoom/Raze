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
#include "razefont.h"

#include "../version.h"

EXTERN_CVAR (Bool, am_showmonsters)
EXTERN_CVAR (Bool, am_showsecrets)
EXTERN_CVAR (Bool, am_showtime)
EXTERN_CVAR (Bool, am_showtotaltime)
EXTERN_CVAR (Bool, noisedebug)
EXTERN_CVAR(Bool, vid_fps)
EXTERN_CVAR(Bool, inter_subtitles)

extern int setblocks;

//---------------------------------------------------------------------------
//
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
		double scale = isRR()? 0.4 : isSWALL()? 0.7 : 1.0;
		auto text = currentLevel->DisplayName();
		auto myfont = PickBigFont(text);
		double x = 160 - myfont->StringWidth(text) * scale / 2.;
		double y = isBlood() ? 50 : 100 - myfont->GetHeight()/2.;
		bool shadow = true;

		if (shadow)
		{
			DrawText(twod, myfont, CR_UNTRANSLATED, x+1, y+1, text, DTA_FullscreenScale, FSMode_Fit320x200, DTA_Color, 0xff000000, DTA_Alpha, alpha / 2., DTA_ScaleX, scale, DTA_ScaleY, scale, TAG_DONE);
		}
		DrawText(twod, myfont, CR_UNTRANSLATED, x, y, text, DTA_FullscreenScale, FSMode_Fit320x200, DTA_Alpha, alpha, DTA_ScaleX, scale, DTA_ScaleY, scale, TAG_DONE);
	}
}

void UpdateStatusBar(SummaryInfo* info)
{
	IFVIRTUALPTRNAME(StatusBar, NAME_RazeStatusBar, UpdateStatusBar)
	{
		VMValue params[] = { StatusBar, info };
		VMCall(func, params, 2, nullptr, 0);
	}
}

void TickStatusBar()
{
	IFVIRTUALPTRNAME(StatusBar, NAME_RazeStatusBar, Tick)
	{
		VMValue params[] = { StatusBar };
		VMCall(func, params, 1, nullptr, 0);
	}
}

void ResetStatusBar()
{
	IFVIRTUALPTRNAME(StatusBar, NAME_RazeStatusBar, Reset)
	{
		VMValue params[] = { StatusBar };
		VMCall(func, params, 1, nullptr, 0);
	}
}

void InitStatusBar()
{
	IFVIRTUALPTRNAME(StatusBar, NAME_RazeStatusBar, Init)
	{
		VMValue params[] = { StatusBar };
		VMCall(func, params, 1, nullptr, 0);
	}
}

