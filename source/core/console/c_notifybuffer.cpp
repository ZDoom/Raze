/*
** c_notifybuffer.cpp
** Implements the buffer for the notification message
**
**---------------------------------------------------------------------------
** Copyright 1998-2006 Randy Heit
** Copyright 2005-2020 Christoph Oelckers
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

#include "c_console.h"
#include "vm.h"
#include "gamestate.h"
#include "c_cvars.h"
#include "v_video.h"
#include "i_time.h"
#include "printf.h"
#include "c_console.h"
#include "c_notifybufferbase.h"
#include "v_video.h"
#include "v_font.h"
#include "v_draw.h"
#include "gamecontrol.h"
#include "gstrings.h"

struct FNotifyBuffer : public FNotifyBufferBase
{
	void DrawNative();
public:
	void AddString(int printlevel, FString source) override;
	void Draw() override;

};

static FNotifyBuffer NotifyStrings;
EXTERN_CVAR(Bool, hud_messages)
extern bool generic_ui;
CVAR(Float, con_notifytime, 3.f, CVAR_ARCHIVE)
CVAR(Bool, con_centernotify, false, CVAR_ARCHIVE)
CVAR(Bool, con_pulsetext, false, CVAR_ARCHIVE)
CVAR(Bool, con_notify_advanced, false, CVAR_ARCHIVE)

const int NOTIFYFADETIME = 6;

CUSTOM_CVAR(Int, con_notifylines, 4, CVAR_GLOBALCONFIG | CVAR_ARCHIVE)
{
	NotifyStrings.Shift(self);
}

CUSTOM_CVAR(Float, con_notifyscale, 1, CVAR_ARCHIVE)
{
	if (self < 0.36f) self = 0.36f;
	if (self > 1) self = 1;
}


static FFont* GetNotifyFont()
{
	return generic_ui ? NewSmallFont : isWW2GI()? ConFont : SmallFont ? SmallFont : AlternativeSmallFont;
}

void FNotifyBuffer::AddString(int printlevel, FString source)
{
	if (!(printlevel & PRINT_NOTIFY) && !con_notify_advanced) return;

	if (hud_messages == 0 ||
		screen == nullptr ||
		source.IsEmpty() ||
		gamestate == GS_FULLCONSOLE ||
		gamestate == GS_MENUSCREEN ||
		con_notifylines == 0)
		return;

	auto screenratio = ActiveRatio(screen->GetWidth(), screen->GetHeight());

	FFont* font = GetNotifyFont();
	if (font == nullptr) return;	// Without an initialized font we cannot handle the message (this is for those which come here before the font system is ready.)
	double fontscale = (generic_ui? 0.7 : NotifyFontScale) * con_notifyscale;

	int width = int(320 * (screenratio / 1.333) / fontscale);
	FNotifyBufferBase::AddString(printlevel & PRINT_TYPES, font, source, width, con_notifytime, con_notifylines);
}


void FNotifyBuffer::DrawNative()
{
	// Native display is:
	// * centered at the top and pulsing for Duke
	// * centered shifted down and not pulsing for  Shadow Warrior
	// * top left for Exhumed
	// * 4 lines with the tiny font for Blood. (same mechanic as the regular one, just a different font and scale.)

	bool center = g_gameType & (GAMEFLAG_DUKE | GAMEFLAG_NAM | GAMEFLAG_WW2GI | GAMEFLAG_RRALL | GAMEFLAG_SW);
	bool pulse = g_gameType & (GAMEFLAG_DUKE | GAMEFLAG_NAM | GAMEFLAG_WW2GI | GAMEFLAG_RRALL);
	unsigned topline = isBlood() ? 0 : Text.Size() - 1;

	FFont* font = isBlood() ? SmallFont2 : SmallFont;

	double line = isBlood() ? Top : isSWALL() ? 40 : font->GetDisplacement();
	bool canskip = isBlood();
	double scale = 1 / (NotifyFontScale * con_notifyscale);
	double lineadv = font->GetHeight() / NotifyFontScale;

	for (unsigned i = topline; i < Text.Size(); ++i)
	{
		FNotifyText& notify = Text[i];

		if (notify.TimeOut == 0)
			continue;

		int j = notify.TimeOut - notify.Ticker;
		if (j > 0)
		{
			double alpha = isBlood() ? ((j < NOTIFYFADETIME) ? 1. * j / NOTIFYFADETIME : 1) : 1;
			if (pulse)
			{
				alpha *= 0.7 + 0.3 * sin(I_msTime() / 100.);
			}

			if (!center)
			{
				DrawText(twod, font, CR_UNTRANSLATED, 0, line, notify.Text,
					DTA_FullscreenScale, FSMode_ScaleToHeight,
					DTA_VirtualWidthF, 320 * scale, DTA_VirtualHeightF, 200 * scale, DTA_KeepRatio, true,
					DTA_Alpha, alpha, TAG_DONE);
			}
			else
			{
				DrawText(twod, font, CR_UNTRANSLATED, 160 * scale - font->StringWidth(notify.Text) / 2, line, notify.Text,
					DTA_FullscreenScale, FSMode_ScaleToHeight,
					DTA_VirtualWidthF, 320 * scale, DTA_VirtualHeightF, 200 * scale,
					DTA_Alpha, alpha, TAG_DONE);
			}
			line += lineadv;
			canskip = false;
		}
		else
		{
			notify.TimeOut = 0;
		}
	}
	if (canskip)
	{
		Top = TopGoal;
	}
}

static bool printNative()
{
	// Blood originally uses its tiny font for the notify display which does not play along well with localization because it is too small, so for non-English switch to the text font.
	if (con_notify_advanced) return false;
	if (!isBlood()) return true;
	auto p = GStrings["REQUIRED_CHARACTERS"];
	if (p && *p) return false;
	return true;
}

void FNotifyBuffer::Draw()
{
	if (printNative())
	{
		DrawNative();
		return;
	}

	bool center = (con_centernotify != 0.f);
	int color;
	bool canskip = true;


	FFont* font = GetNotifyFont();
	double nfscale = (generic_ui? 0.7 : NotifyFontScale);
	double scale = 1 / (nfscale * con_notifyscale);

	double line = Top + font->GetDisplacement() / nfscale;
	double lineadv = font->GetHeight () / nfscale;

	for (unsigned i = 0; i < Text.Size(); ++ i)
	{
		FNotifyText &notify = Text[i];

		if (notify.TimeOut == 0)
			continue;

		int j = notify.TimeOut - notify.Ticker;
		if (j > 0)
		{
			double alpha = (j < NOTIFYFADETIME) ? 1. * j / NOTIFYFADETIME : 1;
			if (con_pulsetext)
			{
				alpha *= 0.7 + 0.3 * sin(I_msTime() / 100.);
			}

			if (notify.PrintLevel >= PRINTLEVELS)
				color = CR_UNTRANSLATED;
			else
				color = PrintColors[notify.PrintLevel];

			if (!center)
				DrawText(twod, font, color, 0, line * NotifyFontScale, notify.Text,
					DTA_FullscreenScale, FSMode_ScaleToHeight,
					DTA_VirtualWidthF, 320. * scale,
					DTA_VirtualHeightF, 200. * scale,
					DTA_KeepRatio, true,
					DTA_Alpha, alpha, TAG_DONE);
			else
				DrawText(twod, font, color, 160 * scale - font->StringWidth (notify.Text) / 2.,
					line, notify.Text,
					DTA_FullscreenScale, FSMode_ScaleToHeight,
					DTA_VirtualWidthF, 320. * scale,
					DTA_VirtualHeightF, 200. * scale,
					DTA_Alpha, alpha, TAG_DONE);
			line += lineadv;
			canskip = false;
		}
		else
		{
			notify.TimeOut = 0;
		}
	}
	if (canskip)
	{
		Top = TopGoal;
	}
}

void SetConsoleNotifyBuffer()
{
	C_SetNotifyBuffer(&NotifyStrings);
}
