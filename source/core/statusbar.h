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

extern DStatusBarCore *StatusBar;

// Status bar factories -----------------------------------------------------

// Crosshair stuff ----------------------------------------------------------

void ST_Clear();
extern FGameTexture *CrosshairImage;


void setViewport(int viewSize);
struct MapRecord;
void setLevelStarted(MapRecord *);
void drawMapTitle();
class FSerializer;
void SerializeHud(FSerializer &arc);
extern int levelTextTime;
struct SummaryInfo;
void UpdateStatusBar(SummaryInfo* info);
void TickStatusBar();
void ResetStatusBar();
void InitStatusBar();

#endif /* __SBAR_H__ */
