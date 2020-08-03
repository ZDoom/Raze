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

class player_t;
struct FRemapTable;

enum EHudState
{
	HUD_StatusBar,
	HUD_Fullscreen,
	HUD_None,

	HUD_AltHud // Used for passing through popups to the alt hud
};

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
	int secrets, maxsecrets;	// set maxsecrets to -1 to ignore
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

class DHUDFont //: public DObject
{
	// this blocks CreateNew on this class which is the intent here.
	//DECLARE_ABSTRACT_CLASS(DHUDFont, DObject);

public:
	FFont *mFont;
	int mSpacing;
	EMonospacing mMonospacing;
	int mShadowX;
	int mShadowY;

	DHUDFont() = default;
	DHUDFont(FFont *f, int sp, EMonospacing ms, int sx, int sy)
		: mFont(f), mSpacing(sp), mMonospacing(ms), mShadowX(sx), mShadowY(sy)
	{}
};


class DBaseStatusBar //: public DObject
{
	//DECLARE_CLASS (DBaseStatusBar, DObject)
	//HAS_OBJECT_POINTERS
public:
	// Popup screens for Strife's status bar
	enum
	{
		POP_NoChange = -1,
		POP_None,
		POP_Log,
		POP_Keys,
		POP_Status
	};

	// Status face stuff
	enum
	{
		ST_NUMPAINFACES		= 5,
		ST_NUMSTRAIGHTFACES	= 3,
		ST_NUMTURNFACES		= 2,
		ST_NUMSPECIALFACES	= 3,
		ST_NUMEXTRAFACES	= 2,
		ST_FACESTRIDE		= ST_NUMSTRAIGHTFACES+ST_NUMTURNFACES+ST_NUMSPECIALFACES,
		ST_NUMFACES			= ST_FACESTRIDE*ST_NUMPAINFACES+ST_NUMEXTRAFACES,

		ST_TURNOFFSET		= ST_NUMSTRAIGHTFACES,
		ST_OUCHOFFSET		= ST_TURNOFFSET + ST_NUMTURNFACES,
		ST_EVILGRINOFFSET	= ST_OUCHOFFSET + 1,
		ST_RAMPAGEOFFSET	= ST_EVILGRINOFFSET + 1,
		ST_GODFACE			= ST_NUMPAINFACES*ST_FACESTRIDE,
		ST_DEADFACE			= ST_GODFACE + 1
	};


	enum EAlign
	{
		TOP = 0,
		VCENTER = 1,
		BOTTOM = 2,
		VOFFSET = 3,
		VMASK = 3,

		LEFT = 0,
		HCENTER = 4,
		RIGHT = 8,
		HOFFSET = 12,
		HMASK = 12,

		CENTER = VCENTER | HCENTER,
		CENTER_BOTTOM = BOTTOM | HCENTER
	};

	DBaseStatusBar ();
	virtual ~DBaseStatusBar() = default;
	void SetSize(int reltop = 32, int hres = 320, int vres = 200, int hhres = -1, int hvres = -1);

	void ShowPlayerName ();
	double GetDisplacement() { return Displacement; }
	int GetPlayer ();

	static void AddBlend (float r, float g, float b, float a, float v_blend[4]);

	// do not make this a DObject Serialize function because it's not used like one!
	void SerializeMessages(FSerializer &arc);

	void SetScale();
	virtual void Tick ();
	void AttachToPlayer(player_t *player);
	DVector2 GetHUDScale() const;
	void NewGame ();

	void DrawGraphic(FGameTexture *texture, double x, double y, int flags, double Alpha, double boxwidth, double boxheight, double scaleX, double scaleY, PalEntry color = 0xffffffff, int translation = 0, double rotate = 0, ERenderStyle style = STYLE_Translucent);
	void DrawGraphic(FTextureID texture, double x, double y, int flags, double Alpha, double boxwidth, double boxheight, double scaleX, double scaleY, PalEntry color = 0xffffffff, int translation = 0, double rotate = 0, ERenderStyle style = STYLE_Translucent);
	void DrawString(FFont *font, const FString &cstring, double x, double y, int flags, double Alpha, int translation, int spacing, EMonospacing monospacing, int shadowX, int shadowY, double scaleX, double scaleY);
	void TransformRect(double &x, double &y, double &w, double &h, int flags = 0);
	void Fill(PalEntry color, double x, double y, double w, double h, int flags = 0);

	void BeginStatusBar(int resW, int resH, int relTop);
	void BeginHUD(int resW, int resH, double Alpha);
	void StatusbarToRealCoords(double &x, double &y, double &w, double &h) const;
	void PrintLevelStats(FLevelStats& stats);
	int GetTopOfStatusbar() const
	{
		return SBarTop;
	}
	void DoDrawAutomapHUD(int crdefault, int highlight);

//protected:
	void DrawPowerups ();

	
	void RefreshBackground () const;
	void RefreshViewBorder ();

private:
	DObject *AltHud = nullptr;

public:

	void DrawCrosshair ();

	// Sizing info for ths status bar.
	int ST_X;
	int ST_Y;
	int SBarTop;
	DVector2 SBarScale;
	int RelTop;
	int HorizontalResolution, VerticalResolution;
	bool Scaled;							// This needs to go away.

	bool Centering;
	bool FixedOrigin;
	bool CompleteBorder;
	double CrosshairSize;
	double Displacement;
	bool ShowLog;

	double Alpha = 1.;
	DVector2 drawOffset = { 0,0 };			// can be set by subclasses to offset drawing operations
	double drawClip[4] = { 0,0,0,0 };		// defines a clipping rectangle (not used yet)
	bool fullscreenOffsets = false;			// current screen is displayed with fullscreen behavior.

private:
	void SetDrawSize(int reltop, int hres, int vres);

	int BaseRelTop;
	int BaseSBarHorizontalResolution;
	int BaseSBarVerticalResolution;
	int BaseHUDHorizontalResolution;
	int BaseHUDVerticalResolution;

};

extern DBaseStatusBar *StatusBar;

// Status bar factories -----------------------------------------------------

DBaseStatusBar *CreateCustomStatusBar(int script=0);

// Crosshair stuff ----------------------------------------------------------

void ST_LoadCrosshair(bool alwaysload=false);
void ST_Clear();
void ST_CreateStatusBar(bool bTitleLevel);
extern FGameTexture *CrosshairImage;

int GetInventoryIcon(AActor *item, uint32_t flags, int *applyscale = nullptr);


enum DI_Flags
{
	DI_SKIPICON = 0x1,
	DI_SKIPALTICON = 0x2,
	DI_SKIPSPAWN = 0x4,
	DI_SKIPREADY = 0x8,
	DI_ALTICONFIRST = 0x10,
	DI_TRANSLATABLE = 0x20,
	DI_FORCESCALE = 0x40,
	DI_DIM = 0x80,
	DI_DRAWCURSORFIRST = 0x100,	// only for DrawInventoryBar.
	DI_ALWAYSSHOWCOUNT = 0x200,	// only for DrawInventoryBar.
	DI_DIMDEPLETED = 0x400,
	DI_DONTANIMATE = 0x800,		// do not animate the texture
	DI_MIRROR = 0x1000,		// flip the texture horizontally, like a mirror
	DI_ITEM_RELCENTER = 0x2000,
		
	DI_SCREEN_AUTO = 0,					// decide based on given offsets.
	DI_SCREEN_MANUAL_ALIGN = 0x4000,	// If this is on, the following flags will have an effect
		
	DI_SCREEN_TOP = DI_SCREEN_MANUAL_ALIGN,
	DI_SCREEN_VCENTER = 0x8000 | DI_SCREEN_MANUAL_ALIGN,
	DI_SCREEN_BOTTOM = 0x10000 | DI_SCREEN_MANUAL_ALIGN,
	DI_SCREEN_VOFFSET = 0x18000 | DI_SCREEN_MANUAL_ALIGN,
	DI_SCREEN_VMASK = 0x18000 | DI_SCREEN_MANUAL_ALIGN,
		
	DI_SCREEN_LEFT = DI_SCREEN_MANUAL_ALIGN,
	DI_SCREEN_HCENTER = 0x20000 | DI_SCREEN_MANUAL_ALIGN,
	DI_SCREEN_RIGHT = 0x40000 | DI_SCREEN_MANUAL_ALIGN,
	DI_SCREEN_HOFFSET = 0x60000 | DI_SCREEN_MANUAL_ALIGN,
	DI_SCREEN_HMASK = 0x60000 | DI_SCREEN_MANUAL_ALIGN,
		
	DI_SCREEN_LEFT_TOP = DI_SCREEN_TOP|DI_SCREEN_LEFT,
	DI_SCREEN_RIGHT_TOP = DI_SCREEN_TOP|DI_SCREEN_RIGHT,
	DI_SCREEN_LEFT_BOTTOM = DI_SCREEN_BOTTOM|DI_SCREEN_LEFT,
	DI_SCREEN_LEFT_CENTER = DI_SCREEN_VCENTER | DI_SCREEN_LEFT,
	DI_SCREEN_RIGHT_BOTTOM = DI_SCREEN_BOTTOM|DI_SCREEN_RIGHT,
	DI_SCREEN_RIGHT_CENTER = DI_SCREEN_VCENTER | DI_SCREEN_RIGHT,
	DI_SCREEN_CENTER = DI_SCREEN_VCENTER|DI_SCREEN_HCENTER,
	DI_SCREEN_CENTER_TOP = DI_SCREEN_TOP | DI_SCREEN_HCENTER,
	DI_SCREEN_CENTER_BOTTOM = DI_SCREEN_BOTTOM|DI_SCREEN_HCENTER,
	DI_SCREEN_OFFSETS = DI_SCREEN_HOFFSET|DI_SCREEN_VOFFSET,
		
	DI_ITEM_AUTO = 0,		// equivalent with bottom center, which is the default alignment.
		
	DI_ITEM_TOP = 0x80000,
	DI_ITEM_VCENTER = 0x100000,
	DI_ITEM_BOTTOM = 0,		// this is the default vertical alignment
	DI_ITEM_VOFFSET = 0x180000,
	DI_ITEM_VMASK = 0x180000,
		
	DI_ITEM_LEFT = 0x200000,
	DI_ITEM_HCENTER = 0,	// this is the deafault horizontal alignment
	DI_ITEM_RIGHT = 0x400000,
	DI_ITEM_HOFFSET = 0x600000,
	DI_ITEM_HMASK = 0x600000,
		
	DI_ITEM_LEFT_TOP = DI_ITEM_TOP|DI_ITEM_LEFT,
	DI_ITEM_RIGHT_TOP = DI_ITEM_TOP|DI_ITEM_RIGHT,
	DI_ITEM_LEFT_BOTTOM = DI_ITEM_BOTTOM|DI_ITEM_LEFT,
	DI_ITEM_RIGHT_BOTTOM = DI_ITEM_BOTTOM|DI_ITEM_RIGHT,
	DI_ITEM_CENTER = DI_ITEM_VCENTER|DI_ITEM_HCENTER,
	DI_ITEM_CENTER_BOTTOM = DI_ITEM_BOTTOM|DI_ITEM_HCENTER,
	DI_ITEM_OFFSETS = DI_ITEM_HOFFSET|DI_ITEM_VOFFSET,
		
	DI_TEXT_ALIGN_LEFT = 0,
	DI_TEXT_ALIGN_RIGHT = 0x800000,
	DI_TEXT_ALIGN_CENTER = 0x1000000,
	DI_TEXT_ALIGN = 0x1800000,

	DI_ALPHAMAPPED = 0x2000000,
	DI_NOSHADOW = 0x4000000,
	DI_ALWAYSSHOWCOUNTERS = 0x8000000,
	DI_ARTIFLASH = 0x10000000,
	DI_FORCEFILL = 0x20000000,

	// These 2 flags are only used by SBARINFO so these duplicate other flags not used by SBARINFO
	DI_DRAWINBOX = DI_TEXT_ALIGN_RIGHT,
	DI_ALTERNATEONFAIL = DI_TEXT_ALIGN_CENTER,

};

void SBar_DrawString(DBaseStatusBar* self, DHUDFont* font, const FString& string, double x, double y, int flags, int trans, double alpha, int wrapwidth, int linespacing, double scaleX, double scaleY);

#endif /* __SBAR_H__ */
