/*
** sbar.h
** Base status bar definition
**
**---------------------------------------------------------------------------
** Copyright 1998-2006 Randy Heit
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

#ifndef __SBAR_H__
#define __SBAR_H__

#include "dobject.h"
#include "v_text.h"
#include "renderstyle.h"
#include "base_sbar.h"

struct FRemapTable;

#if 0
enum EHudState
{
	HUD_StatusBar,
	HUD_Fullscreen,
	HUD_None,

	HUD_AltHud // Used for passing through popups to the alt hud
};
#endif

enum EMonospacing : int;


// Base Status Bar ----------------------------------------------------------

class FGameTexture;

enum
{
	HUDMSGLayer_OverHUD,
	HUDMSGLayer_UnderHUD,
	HUDMSGLayer_OverMap,

	NUM_HUDMSGLAYERS,
	HUDMSGLayer_Default = HUDMSGLayer_OverHUD,
};

struct FLevelStats
{
	int screenbottomspace;
	int time; // in milliseconds
	int frags;
	int kills, maxkills;	// set maxkills to -1 to ignore, or to -2 to only print kills
	int secrets, maxsecrets, supersecrets;	// set maxsecrets to -1 to ignore
	int spacing; // uses fontheight if 0 or less.
	EColorRange letterColor, standardColor, completeColor;
	double fontscale;
	FFont* font;
};

//============================================================================
//
// encapsulates all settings a HUD font may need
//
//============================================================================

class DBaseStatusBar : public DStatusBarCore
{
	DECLARE_ABSTRACT_CLASS (DBaseStatusBar, DStatusBarCore)

public:
	DBaseStatusBar ();
	virtual ~DBaseStatusBar() = default;

	// do not make this a DObject Serialize function because it's not used like one!
	//void SerializeMessages(FSerializer &arc);

	virtual void Tick ();

	void PrintLevelStats(FLevelStats& stats);
	void PrintAutomapInfo(FLevelStats& stats, bool forcetextfont = false);
	int GetTopOfStatusbar() const
	{
		return SBarTop;
	}
	short CalcMagazineAmount(short ammo_remaining, short clip_capacity, bool reloading);
	void Set43ClipRect();
	virtual void UpdateStatusBar() = 0;
	

private:
	DObject *AltHud = nullptr;

public:


	bool Centering;
	bool FixedOrigin;

private:

};

extern DBaseStatusBar *StatusBar;

// Status bar factories -----------------------------------------------------

// Crosshair stuff ----------------------------------------------------------

void ST_Clear();
extern FGameTexture *CrosshairImage;


void SBar_DrawString(DStatusBarCore* self, DHUDFont* font, const FString& string, double x, double y, int flags, int trans, double alpha, int wrapwidth, int linespacing, double scaleX, double scaleY, int pt = 0, int style = STYLE_Translucent);
void setViewport(int viewSize);
struct MapRecord;
void setLevelStarted(MapRecord *);
void drawMapTitle();
class FSerializer;
void SerializeHud(FSerializer &arc);
extern int levelTextTime;

#endif /* __SBAR_H__ */
