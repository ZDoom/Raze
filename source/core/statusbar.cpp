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
#include "menu.h"
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
CVAR(Bool, log_vgafont, false, CVAR_ARCHIVE)

DBaseStatusBar *StatusBar;

extern int setblocks;

FGameTexture *CrosshairImage;
static int CrosshairNum;


CVAR (Bool, crosshairon, true, CVAR_ARCHIVE);
CVAR (Int, crosshair, 0, CVAR_ARCHIVE)
CVAR (Bool, crosshairforce, false, CVAR_ARCHIVE)
CVAR (Color, crosshaircolor, 0xff0000, CVAR_ARCHIVE);
CVAR (Int, crosshairhealth, 1, CVAR_ARCHIVE);
CVAR (Float, crosshairscale, 1.0, CVAR_ARCHIVE);
CVAR (Bool, crosshairgrow, false, CVAR_ARCHIVE);

CVAR (Bool, idmypos, false, 0);


//---------------------------------------------------------------------------
//
// ST_Clear
//
//---------------------------------------------------------------------------

void ST_Clear()
{
	if (StatusBar != NULL)
	{
		delete StatusBar;
		StatusBar = NULL;
	}
	CrosshairImage = NULL;
	CrosshairNum = 0;
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
	CrosshairSize = 1.;
	Displacement = 0;
	ShowLog = false;
	SetSize(0);
}

static void ValidateResolution(int &hres, int &vres)
{
	if (hres == 0)
	{
		static const int HORIZONTAL_RESOLUTION_DEFAULT = 320;
		hres = HORIZONTAL_RESOLUTION_DEFAULT;
	}

	if (vres == 0)
	{
		static const int VERTICAL_RESOLUTION_DEFAULT = 200;
		vres = VERTICAL_RESOLUTION_DEFAULT;
	}
}

void DBaseStatusBar::SetSize(int reltop, int hres, int vres, int hhres, int hvres)
{
	ValidateResolution(hres, vres);

	BaseRelTop = reltop;
	BaseSBarHorizontalResolution = hres;
	BaseSBarVerticalResolution = vres;
	BaseHUDHorizontalResolution = hhres < 0? hres : hhres;
	BaseHUDVerticalResolution = hvres < 0? vres : hvres;
	SetDrawSize(reltop, hres, vres);
}

void DBaseStatusBar::SetDrawSize(int reltop, int hres, int vres)
{
	ValidateResolution(hres, vres);

	RelTop = reltop;
	HorizontalResolution = hres;
	VerticalResolution = vres;
	SetScale();	// recalculate positioning info.
}

//---------------------------------------------------------------------------
//
// PROC SetScaled
//
//---------------------------------------------------------------------------

void DBaseStatusBar::SetScale ()
{
	ValidateResolution(HorizontalResolution, VerticalResolution);

	double w = SCREENWIDTH;
	double h = SCREENHEIGHT;
	double refw, refh;

	int horz = HorizontalResolution;
	int vert = VerticalResolution;
	double refaspect = horz / double(vert);
	double screenaspect = w / double(h);

	if ((horz == 320 && vert == 200) || (horz == 640 && vert == 400))
	{
		refaspect = 1.333;
	}
	
	if (screenaspect < refaspect)
	{
		refw = w;
		refh = w / refaspect;
	}
	else
	{
		refh = h;
		refw = h * refaspect;
	}
	refw *= hud_scale;
	refh *= hud_scale;

	int sby = VerticalResolution - RelTop;
	// Use full pixels for destination size.

	ST_X = xs_CRoundToInt((w - refw) / 2);
	ST_Y = xs_CRoundToInt(h - refh);
	SBarTop = Scale(sby, h, VerticalResolution);
	SBarScale.X = refw / horz;
	SBarScale.Y = refh / vert;
}

//---------------------------------------------------------------------------
//
// PROC GetHUDScale
//
//---------------------------------------------------------------------------

DVector2 DBaseStatusBar::GetHUDScale() const
{
	return SBarScale;
}

//---------------------------------------------------------------------------
//
//  
//
//---------------------------------------------------------------------------

void DBaseStatusBar::BeginStatusBar(int resW, int resH, int relTop)
{
	SetDrawSize(relTop < 0? BaseRelTop : relTop, resW < 0? BaseSBarHorizontalResolution : resW, resH < 0? BaseSBarVerticalResolution : resH);
	fullscreenOffsets = false;
}

//---------------------------------------------------------------------------
//
//  
//
//---------------------------------------------------------------------------

void DBaseStatusBar::BeginHUD(int resW, int resH, double Alpha)
{
	SetDrawSize(RelTop, resW < 0? BaseHUDHorizontalResolution : resW, resH < 0? BaseHUDVerticalResolution : resH);	
	this->Alpha = Alpha;
	CompleteBorder = false;
	fullscreenOffsets = true;
}

//---------------------------------------------------------------------------
//
// PROC Tick
//
//---------------------------------------------------------------------------

void DBaseStatusBar::Tick ()
{
}

//============================================================================
//
// draw stuff
//
//============================================================================

void DBaseStatusBar::StatusbarToRealCoords(double &x, double &y, double &w, double &h) const
{
	x = ST_X + x * SBarScale.X;
	y = ST_Y + y * SBarScale.Y;
	w *= SBarScale.X;
	h *= SBarScale.Y;
}

//============================================================================
//
// draw stuff
//
//============================================================================

void DBaseStatusBar::DrawGraphic(FTextureID texture, double x, double y, int flags, double Alpha, double boxwidth, double boxheight, double scaleX, double scaleY, PalEntry color, int translation, double rotate, ERenderStyle style)
{
	if (!texture.isValid())
		return;

	FGameTexture* tex = TexMan.GetGameTexture(texture, !(flags & DI_DONTANIMATE));
	DrawGraphic(tex, x, y, flags, Alpha, boxwidth, boxheight, scaleX, scaleY, color, translation, rotate, style);
}

void DBaseStatusBar::DrawGraphic(FGameTexture* tex, double x, double y, int flags, double Alpha, double boxwidth, double boxheight, double scaleX, double scaleY, PalEntry color, int translation, double rotate, ERenderStyle style)
{
	double texwidth = tex->GetDisplayWidth() * scaleX;
	double texheight = tex->GetDisplayHeight() * scaleY;

	if (boxwidth > 0 || boxheight > 0)
	{
		if (!(flags & DI_FORCEFILL))
		{
			double scale1 = 1., scale2 = 1.;

			if (boxwidth > 0 && (boxwidth < texwidth || (flags & DI_FORCESCALE)))
			{
				scale1 = boxwidth / texwidth;
			}
			if (boxheight != -1 && (boxheight < texheight || (flags & DI_FORCESCALE)))
			{
				scale2 = boxheight / texheight;
			}

			if (flags & DI_FORCESCALE)
			{
				if (boxwidth <= 0 || (boxheight > 0 && scale2 < scale1))
					scale1 = scale2;
			}
			else scale1 = MIN(scale1, scale2);

			boxwidth = texwidth * scale1;
			boxheight = texheight * scale1;
		}
	}
	else
	{
		boxwidth = texwidth;
		boxheight = texheight;
	}

	// resolve auto-alignment before making any adjustments to the position values.
	if (!(flags & DI_SCREEN_MANUAL_ALIGN))
	{
		if (x < 0) flags |= DI_SCREEN_RIGHT;
		else flags |= DI_SCREEN_LEFT;
		if (y < 0) flags |= DI_SCREEN_BOTTOM;
		else flags |= DI_SCREEN_TOP;
	}

	Alpha *= this->Alpha;
	if (Alpha <= 0) return;
	x += drawOffset.X;
	y += drawOffset.Y;

	double xo = 0, yo = 0;
	if (flags & DI_ITEM_RELCENTER)
	{
		xo = tex->GetDisplayWidth() / 2 + tex->GetDisplayLeftOffset();
		yo = tex->GetDisplayHeight() / 2 + tex->GetDisplayTopOffset();
	}
	else
	{
		switch (flags & DI_ITEM_HMASK)
		{
		case DI_ITEM_HCENTER:	xo = tex->GetDisplayWidth() / 2; break;
		case DI_ITEM_RIGHT:		xo = tex->GetDisplayWidth(); break;
		case DI_ITEM_HOFFSET:	xo = tex->GetDisplayLeftOffset(); break;
		}

		switch (flags & DI_ITEM_VMASK)
		{
		case DI_ITEM_VCENTER: yo = tex->GetDisplayHeight() / 2; break;
		case DI_ITEM_BOTTOM:  yo = tex->GetDisplayHeight(); break;
		case DI_ITEM_VOFFSET: yo = tex->GetDisplayTopOffset(); break;
		}
	}
	//xo *= scaleX;
	//yo *= scaleY;

	if (!fullscreenOffsets)
	{
		StatusbarToRealCoords(x, y, boxwidth, boxheight);
	}
	else
	{
		double orgx, orgy;

		switch (flags & DI_SCREEN_HMASK)
		{
		default: orgx = 0; break;
		case DI_SCREEN_HCENTER: orgx = screen->GetWidth() / 2; break;
		case DI_SCREEN_RIGHT:   orgx = screen->GetWidth(); break;
		}

		switch (flags & DI_SCREEN_VMASK)
		{
		default: orgy = 0; break;
		case DI_SCREEN_VCENTER: orgy = screen->GetHeight() / 2; break;
		case DI_SCREEN_BOTTOM: orgy = screen->GetHeight(); break;
		}

		// move stuff in the top right corner a bit down if the fps counter is on.
		if ((flags & (DI_SCREEN_HMASK|DI_SCREEN_VMASK)) == DI_SCREEN_RIGHT_TOP && vid_fps) y += 10;

		DVector2 Scale = GetHUDScale();

		x *= Scale.X;
		y *= Scale.Y;
		boxwidth *= Scale.X;
		boxheight *= Scale.Y;
		x += orgx;
		y += orgy;
	}
	// Now reapply the texture offsets. We will need them
	DrawTexture(twod, tex, x, y, 
		DTA_TopOffsetF, yo,
		DTA_LeftOffsetF, xo,
		DTA_DestWidthF, boxwidth,
		DTA_DestHeightF, boxheight,
		DTA_Color, color,
		DTA_TranslationIndex, translation, // (flags & DI_TRANSLATABLE) ? GetTranslation() : 0,
		DTA_ColorOverlay, (flags & DI_DIM) ? MAKEARGB(170, 0, 0, 0) : 0,
		DTA_Alpha, Alpha,
		DTA_AlphaChannel, !!(flags & DI_ALPHAMAPPED),
		DTA_FillColor, (flags & DI_ALPHAMAPPED) ? 0 : -1,
		DTA_FlipX, !!(flags & DI_MIRROR),
		DTA_FlipY, !!(flags& DI_MIRRORY),
		DTA_Rotate, rotate,
		DTA_FlipOffsets, true,
		DTA_LegacyRenderStyle, style,
		TAG_DONE);
}


//============================================================================
//
// draw a string
//
//============================================================================

void DBaseStatusBar::DrawString(FFont *font, const FString &cstring, double x, double y, int flags, double Alpha, int translation, int spacing, EMonospacing monospacing, int shadowX, int shadowY, double scaleX, double scaleY)
{
	bool monospaced = monospacing != EMonospacing::Off;
	double dx = 0;

	switch (flags & DI_TEXT_ALIGN)
	{
	default:
		break;
	case DI_TEXT_ALIGN_RIGHT:
		dx = monospaced
			? static_cast<int> ((spacing)*cstring.CharacterCount()) //monospaced, so just multiply the character size
			: static_cast<int> (font->StringWidth(cstring) + (spacing * cstring.CharacterCount()));
		break;
	case DI_TEXT_ALIGN_CENTER:
		dx = monospaced
			? static_cast<int> ((spacing)*cstring.CharacterCount()) / 2 //monospaced, so just multiply the character size
			: static_cast<int> (font->StringWidth(cstring) + (spacing * cstring.CharacterCount())) / 2;
		break;
	}

	// Take text scale into account
	x -= dx * scaleX;

	const uint8_t* str = (const uint8_t*)cstring.GetChars();
	const EColorRange boldTranslation = EColorRange(translation ? translation - 1 : NumTextColors - 1);
	int fontcolor = translation;
	double orgx = 0, orgy = 0;
	DVector2 Scale;

	if (fullscreenOffsets)
	{
		Scale = GetHUDScale();
		shadowX *= (int)Scale.X;
		shadowY *= (int)Scale.Y;

		switch (flags & DI_SCREEN_HMASK)
		{
		default: orgx = 0; break;
		case DI_SCREEN_HCENTER: orgx = screen->GetWidth() / 2; break;
		case DI_SCREEN_RIGHT:   orgx = screen->GetWidth(); break;
		}

		switch (flags & DI_SCREEN_VMASK)
		{
		default: orgy = 0; break;
		case DI_SCREEN_VCENTER: orgy = screen->GetHeight() / 2; break;
		case DI_SCREEN_BOTTOM: orgy = screen->GetHeight(); break;
		}

		// move stuff in the top right corner a bit down if the fps counter is on.
		if ((flags & (DI_SCREEN_HMASK | DI_SCREEN_VMASK)) == DI_SCREEN_RIGHT_TOP && vid_fps) y += 10;
	}
	else
	{
		Scale = { 1.,1. };
	}
	int ch;
	while (ch = GetCharFromString(str), ch != '\0')
	{
		if (ch == ' ')
		{
			x += monospaced ? spacing : font->GetSpaceWidth() + spacing;
			continue;
		}
		else if (ch == TEXTCOLOR_ESCAPE)
		{
			EColorRange newColor = V_ParseFontColor(str, translation, boldTranslation);
			if (newColor != CR_UNDEFINED)
				fontcolor = newColor;
			continue;
		}

		int width;
		FGameTexture* c = font->GetChar(ch, fontcolor, &width);
		if (c == NULL) //missing character.
		{
			continue;
		}
		width += font->GetDefaultKerning();

		if (!monospaced) //If we are monospaced lets use the offset
			x += (c->GetDisplayLeftOffset() + 1); //ignore x offsets since we adapt to character size

		double rx, ry, rw, rh;
		rx = x + drawOffset.X;
		ry = y + drawOffset.Y;
		rw = c->GetDisplayWidth();
		rh = c->GetDisplayHeight();

		if (monospacing == EMonospacing::CellCenter)
			rx += (spacing - rw) / 2;
		else if (monospacing == EMonospacing::CellRight)
			rx += (spacing - rw);

		if (!fullscreenOffsets)
		{
			StatusbarToRealCoords(rx, ry, rw, rh);
		}
		else
		{
			rx *= Scale.X;
			ry *= Scale.Y;
			rw *= Scale.X;
			rh *= Scale.Y;

			rx += orgx;
			ry += orgy;
		}

		// Apply text scale
		rw *= scaleX;
		rh *= scaleY;

		// This is not really such a great way to draw shadows because they can overlap with previously drawn characters.
		// This may have to be changed to draw the shadow text up front separately.
		if ((shadowX != 0 || shadowY != 0) && !(flags & DI_NOSHADOW))
		{
#if 0
			// This doesn't work with the limited backend the engine currently uses.
			DrawChar(twod, font, CR_UNTRANSLATED, rx + shadowX, ry + shadowY, ch,
				DTA_DestWidthF, rw,
				DTA_DestHeightF, rh,
				DTA_Alpha, (Alpha * 0.33),
				DTA_FillColor, 0,
				TAG_DONE);
#endif
		}
		DrawChar(twod, font, fontcolor, rx, ry, ch,
			DTA_DestWidthF, rw,
			DTA_DestHeightF, rh,
			DTA_Alpha, Alpha,
			TAG_DONE);

		dx = monospaced 
			? spacing
			: width + spacing - (c->GetDisplayLeftOffset() + 1);

		// Take text scale into account
		x += dx * scaleX;
	}
}

void SBar_DrawString(DBaseStatusBar *self, DHUDFont *font, const FString &string, double x, double y, int flags, int trans, double alpha, int wrapwidth, int linespacing, double scaleX, double scaleY)
{
	//if (font == nullptr) ThrowAbortException(X_READ_NIL, nullptr);
	//if (!screen->HasBegun2D()) ThrowAbortException(X_OTHER, "Attempt to draw to screen outside a draw function");

	// resolve auto-alignment before making any adjustments to the position values.
	if (!(flags & DI_SCREEN_MANUAL_ALIGN))
	{
		if (x < 0) flags |= DI_SCREEN_RIGHT;
		else flags |= DI_SCREEN_LEFT;
		if (y < 0) flags |= DI_SCREEN_BOTTOM;
		else flags |= DI_SCREEN_TOP;
	}

	if (wrapwidth > 0)
	{
		auto brk = V_BreakLines(font->mFont, int(wrapwidth * scaleX), string, true);
		for (auto &line : brk)
		{
			self->DrawString(font->mFont, line.Text, x, y, flags, alpha, trans, font->mSpacing, font->mMonospacing, font->mShadowX, font->mShadowY, scaleX, scaleY);
			y += (font->mFont->GetHeight() + linespacing) * scaleY;
		}
	}
	else
	{
		self->DrawString(font->mFont, string, x, y, flags, alpha, trans, font->mSpacing, font->mMonospacing, font->mShadowX, font->mShadowY, scaleX, scaleY);
	}
}


//============================================================================
//
// draw stuff
//
//============================================================================

void DBaseStatusBar::TransformRect(double &x, double &y, double &w, double &h, int flags)
{
	// resolve auto-alignment before making any adjustments to the position values.
	if (!(flags & DI_SCREEN_MANUAL_ALIGN))
	{
		if (x < 0) flags |= DI_SCREEN_RIGHT;
		else flags |= DI_SCREEN_LEFT;
		if (y < 0) flags |= DI_SCREEN_BOTTOM;
		else flags |= DI_SCREEN_TOP;
	}

	x += drawOffset.X;
	y += drawOffset.Y;

	if (!fullscreenOffsets)
	{
		StatusbarToRealCoords(x, y, w, h);
	}
	else
	{
		double orgx, orgy;

		switch (flags & DI_SCREEN_HMASK)
		{
		default: orgx = 0; break;
		case DI_SCREEN_HCENTER: orgx = screen->GetWidth() / 2; break;
		case DI_SCREEN_RIGHT:   orgx = screen->GetWidth(); break;
		}

		switch (flags & DI_SCREEN_VMASK)
		{
		default: orgy = 0; break;
		case DI_SCREEN_VCENTER: orgy = screen->GetHeight() / 2; break;
		case DI_SCREEN_BOTTOM: orgy = screen->GetHeight(); break;
		}

		// move stuff in the top right corner a bit down if the fps counter is on.
		if ((flags & (DI_SCREEN_HMASK | DI_SCREEN_VMASK)) == DI_SCREEN_RIGHT_TOP && vid_fps) y += 10;

		DVector2 Scale = GetHUDScale();

		x *= Scale.X;
		y *= Scale.Y;
		w *= Scale.X;
		h *= Scale.Y;
		x += orgx;
		y += orgy;
	}
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

enum ENumFlags
{
	FNF_WHENNOTZERO = 0x1,
	FNF_FILLZEROS = 0x2,
};

void FormatNumber(int number, int minsize, int maxsize, int flags, const FString &prefix, FString *result)
{
	static int maxvals[] = { 1, 9, 99, 999, 9999, 99999, 999999, 9999999, 99999999, 999999999 };

	if (number == 0 && (flags & FNF_WHENNOTZERO))
	{
		*result = "";
		return;
	}
	if (maxsize > 0 && maxsize < 10)
	{
		number = clamp(number, -maxvals[maxsize - 1], maxvals[maxsize]);
	}
	FString &fmt = *result;
	if (minsize <= 1) fmt.Format("%s%d", prefix.GetChars(), number);
	else if (flags & FNF_FILLZEROS) fmt.Format("%s%0*d", prefix.GetChars(), minsize, number);
	else fmt.Format("%s%*d", prefix.GetChars(), minsize, number);
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
		y = 200 - RelTop - spacing;
	}
	else
	{
		y = 200 - stats.screenbottomspace - spacing;
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
		text.Format(TEXTCOLOR_ESCAPESTR "%cS: " TEXTCOLOR_ESCAPESTR "%c%d/%d",
			stats.letterColor + 'A', stats.secrets == stats.maxsecrets ? stats.completeColor + 'A' : stats.standardColor + 'A', stats.secrets, stats.maxsecrets);

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

void DBaseStatusBar::PrintAutomapInfo(FLevelStats& stats)
{
	FString mapname;
	if (am_showlabel) 
		mapname.Format(TEXTCOLOR_ESCAPESTR "%c%s: " TEXTCOLOR_ESCAPESTR "%c%s", stats.letterColor+'A', currentLevel->LabelName(), stats.standardColor+'A', currentLevel->DisplayName());
	else 
		mapname = currentLevel->DisplayName();

	double y;
	double scale = stats.fontscale * (am_textfont? *hud_statscale : 1);	// the tiny default font used by all games here cannot be scaled for readability purposes.
	if (stats.spacing <= 0) stats.spacing = stats.font->GetHeight() * stats.fontscale;
	double spacing = stats.spacing * (am_textfont ? *hud_statscale : 1);
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
	const auto &volname = gVolumeNames[volfromlevelnum(currentLevel->levelNumber)];
	if (volname.IsEmpty() && am_nameontop) y = 1;

	DrawText(twod, stats.font, stats.standardColor, 2 * hud_statscale, y, mapname, DTA_FullscreenScale, FSMode_ScaleToHeight, DTA_VirtualWidth, 320, DTA_VirtualHeight, 200,
		DTA_ScaleX, scale, DTA_ScaleY, scale, DTA_KeepRatio, true, TAG_DONE);
	y -= spacing;
	if (!(currentLevel->flags & MI_USERMAP) && !(g_gameType & GAMEFLAG_PSEXHUMED) && volname.IsNotEmpty())
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
	return ammo_remaining == 0 || reloading && clip_amount == 0 ? 0 : clip_current;
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
	reserved.top = xs_CRoundToInt((reserved.top * hud_scale * ydim) / 200);
	reserved.statusbar = xs_CRoundToInt((reserved.statusbar * hud_scale * ydim) / 200);

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


