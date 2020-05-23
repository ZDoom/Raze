/*
** v_draw.cpp
** Draw patches and blocks to a canvas
**
**---------------------------------------------------------------------------
** Copyright 1998-2008 Randy Heit
** Copyright 2005-2019 Christoph Oelckers
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

#include <stdio.h>
#include <stdarg.h>
#include "c_cvars.h"
#include "drawparms.h"
#include "templates.h"
#include "v_draw.h"
#include "v_video.h"

CUSTOM_CVAR(Int, uiscale, 0, CVAR_ARCHIVE | CVAR_NOINITCALL)
{
	if (self < 0)
	{
		self = 0;
		return;
	}
	//setsizeneeded = true;
}

int GetUIScale(int altval)
{
	int scaleval;
	if (altval > 0) scaleval = altval;
	else if (uiscale == 0)
	{
		// Default should try to scale to 640x400
		int vscale = screen->GetHeight() / 400;
		int hscale = screen->GetWidth() / 640;
		scaleval = clamp(vscale, 1, hscale);
	}
	else scaleval = uiscale;

	// block scales that result in something larger than the current screen.
	int vmax = screen->GetHeight() / 200;
	int hmax = screen->GetWidth() / 320;
	int max = std::max(vmax, hmax);
	return std::max(1,std::min(scaleval, max));
}

// The new console font is twice as high, so the scaling calculation must factor that in.
int GetConScale(int altval)
{
	int scaleval;
	if (altval > 0) scaleval = (altval+1) / 2;
	else if (uiscale == 0)
	{
		// Default should try to scale to 640x400
		int vscale = screen->GetHeight() / 800;
		int hscale = screen->GetWidth() / 1280;
		scaleval = clamp(vscale, 1, hscale);
	}
	else scaleval = (uiscale+1) / 2;

	// block scales that result in something larger than the current screen.
	int vmax = screen->GetHeight() / 400;
	int hmax = screen->GetWidth() / 640;
	int max = std::max(vmax, hmax);
	return std::max(1, std::min(scaleval, max));
}


// [RH] Stretch values to make a 320x200 image best fit the screen
// without using fractional steppings
int CleanXfac, CleanYfac;

// [RH] Effective screen sizes that the above scale values give you
int CleanWidth, CleanHeight;

// Above minus 1 (or 1, if they are already 1)
int CleanXfac_1, CleanYfac_1, CleanWidth_1, CleanHeight_1;


//==========================================================================
//
// Draw parameter parsing
//
//==========================================================================

bool SetTextureParms(DrawParms *parms, FTexture *img, double xx, double yy)
{
	if (img != NULL)
	{
		parms->x = xx;
		parms->y = yy;
		parms->texwidth = img->GetDisplayWidth();
		parms->texheight = img->GetDisplayHeight();
		if (parms->top == INT_MAX || parms->fortext)
		{
			parms->top = img->GetTopOffset();
		}
		if (parms->left == INT_MAX || parms->fortext)
		{
			parms->left = img->GetLeftOffset();
		}
		if (parms->destwidth == INT_MAX || parms->fortext)
		{
			parms->destwidth = img->GetDisplayWidth();
		}
		if (parms->destheight == INT_MAX || parms->fortext)
		{
			parms->destheight = img->GetDisplayHeight();
		}

		switch (parms->cleanmode)
		{
		default:
			break;

		case DTA_Clean:
			parms->x = (parms->x - 160.0) * CleanXfac + (screen->GetWidth() * 0.5);
			parms->y = (parms->y - 100.0) * CleanYfac + (screen->GetHeight() * 0.5);
			parms->destwidth = parms->texwidth * CleanXfac;
			parms->destheight = parms->texheight * CleanYfac;
			break;

		case DTA_CleanNoMove:
			parms->destwidth = parms->texwidth * CleanXfac;
			parms->destheight = parms->texheight * CleanYfac;
			break;

		case DTA_CleanNoMove_1:
			parms->destwidth = parms->texwidth * CleanXfac_1;
			parms->destheight = parms->texheight * CleanYfac_1;
			break;

		case DTA_Fullscreen:
			parms->x = parms->y = 0;
			break;

		}
		if (parms->virtWidth != screen->GetWidth() || parms->virtHeight != screen->GetHeight())
		{
			VirtualToRealCoords(parms->x, parms->y, parms->destwidth, parms->destheight,
				parms->virtWidth, parms->virtHeight, parms->virtBottom, !parms->keepratio);
		}
	}

	return false;
}

//==========================================================================
//
// template helpers
//
//==========================================================================

static void ListEnd(Va_List &tags)
{
	va_end(tags.list);
}

static int ListGetInt(Va_List &tags)
{
	return va_arg(tags.list, int);
}

static inline double ListGetDouble(Va_List &tags)
{
	return va_arg(tags.list, double);
}


//==========================================================================
//
// Main taglist parsing
//
//==========================================================================

bool ParseDrawTextureTags(FTexture *img, double x, double y, uint32_t tag, Va_List& tags, DrawParms *parms, bool fortext)
{
	int boolval;
	int intval;
	bool translationset = false;
	bool fillcolorset = false;

	if (!fortext)
	{
		if (img == NULL)
		{
			ListEnd(tags);
			return false;
		}
	}

	// Do some sanity checks on the coordinates.
	if (x < -16383 || x > 16383 || y < -16383 || y > 16383)
	{
		ListEnd(tags);
		return false;
	}

	parms->fortext = fortext;
	parms->windowleft = 0;
	parms->windowright = INT_MAX;
	parms->dclip = screen->GetHeight();
	parms->uclip = 0;
	parms->lclip = 0;
	parms->rclip = screen->GetWidth();
	parms->left = INT_MAX;
	parms->top = INT_MAX;
	parms->destwidth = INT_MAX;
	parms->destheight = INT_MAX;
	parms->Alpha = 1.f;
	parms->fillcolor = -1;
	parms->colorOverlay = 0;
	parms->alphaChannel = false;
	parms->flipX = false;
	parms->flipY = false;
	parms->color = 0xffffffff;
	//parms->shadowAlpha = 0;
	parms->shadowColor = 0;
	parms->virtWidth = screen->GetWidth();
	parms->virtHeight = screen->GetHeight();
	parms->keepratio = false;
	parms->style.BlendOp = 255;		// Dummy "not set" value
	parms->masked = true;
	parms->bilinear = false;
	parms->desaturate = 0;
	parms->cleanmode = DTA_Base;
	parms->scalex = parms->scaley = 1;
	parms->cellx = parms->celly = 0;
	parms->maxstrlen = INT_MAX;
	parms->virtBottom = false;
	parms->srcx = 0.;
	parms->srcy = 0.;
	parms->srcwidth = 1.;
	parms->srcheight = 1.;
	parms->burn = false;
	parms->monospace = EMonospacing::Off;
	parms->spacing = 0;
	parms->remap = -1;

	// Parse the tag list for attributes. (For floating point attributes,
	// consider that the C ABI dictates that all floats be promoted to
	// doubles when passed as function arguments.)
	while (tag != TAG_DONE)
	{
		switch (tag)
		{
		default:
			ListGetInt(tags);
			break;

		case DTA_DestWidth:
			assert(fortext == false);
			if (fortext) return false;
			parms->cleanmode = DTA_Base;
			parms->destwidth = ListGetInt(tags);
			break;

		case DTA_DestWidthF:
			assert(fortext == false);
			if (fortext) return false;
			parms->cleanmode = DTA_Base;
			parms->destwidth = ListGetDouble(tags);
			break;

		case DTA_DestHeight:
			assert(fortext == false);
			if (fortext) return false;
			parms->cleanmode = DTA_Base;
			parms->destheight = ListGetInt(tags);
			break;

		case DTA_DestHeightF:
			assert(fortext == false);
			if (fortext) return false;
			parms->cleanmode = DTA_Base;
			parms->destheight = ListGetDouble(tags);
			break;

		case DTA_Clean:
			boolval = ListGetInt(tags);
			if (boolval)
			{
				parms->scalex = 1;
				parms->scaley = 1;
				parms->cleanmode = tag;
			}
			break;

		case DTA_CleanNoMove:
			boolval = ListGetInt(tags);
			if (boolval)
			{
				parms->scalex = CleanXfac;
				parms->scaley = CleanYfac;
				parms->cleanmode = tag;
			}
			break;

		case DTA_CleanNoMove_1:
			boolval = ListGetInt(tags);
			if (boolval)
			{
				parms->scalex = CleanXfac_1;
				parms->scaley = CleanYfac_1;
				parms->cleanmode = tag;
			}
			break;

		case DTA_320x200:
			boolval = ListGetInt(tags);
			if (boolval)
			{
				parms->cleanmode = DTA_Base;
				parms->scalex = 1;
				parms->scaley = 1;
				parms->virtWidth = 320;
				parms->virtHeight = 200;
			}
			break;

		case DTA_Bottom320x200:
			boolval = ListGetInt(tags);
			if (boolval)
			{
				parms->cleanmode = DTA_Base;
				parms->scalex = 1;
				parms->scaley = 1;
				parms->virtWidth = 320;
				parms->virtHeight = 200;
			}
			parms->virtBottom = true;
			break;

		case DTA_HUDRules:
			intval = ListGetInt(tags);
			parms->cleanmode = intval == HUD_HorizCenter ? DTA_HUDRulesC : DTA_HUDRules;
			break;

		case DTA_VirtualWidth:
			parms->cleanmode = DTA_Base;
			parms->virtWidth = ListGetInt(tags);
			break;

		case DTA_VirtualWidthF:
			parms->cleanmode = DTA_Base;
			parms->virtWidth = ListGetDouble(tags);
			break;

		case DTA_VirtualHeight:
			parms->cleanmode = DTA_Base;
			parms->virtHeight = ListGetInt(tags);
			break;

		case DTA_VirtualHeightF:
			parms->cleanmode = DTA_Base;
			parms->virtHeight = ListGetDouble(tags);
			break;

		case DTA_Fullscreen:
			boolval = ListGetInt(tags);
			if (boolval)
			{
				assert(fortext == false);
				if (img == NULL) return false;
				parms->cleanmode = DTA_Fullscreen;
				parms->virtWidth = img->GetDisplayWidth();
				parms->virtHeight = img->GetDisplayHeight();
			}
			break;

		case DTA_Alpha:
			parms->Alpha = (float)(std::min<double>(1., ListGetDouble(tags)));
			break;

		case DTA_AlphaChannel:
			parms->alphaChannel = ListGetInt(tags);
			break;

		case DTA_FillColor:
			parms->fillcolor = ListGetInt(tags);
			if (parms->fillcolor != ~0u)
			{
				fillcolorset = true;
			}
			break;

		case DTA_TranslationIndex:
			parms->remap = ListGetInt(tags);
			break;

		case DTA_ColorOverlay:
			parms->colorOverlay = ListGetInt(tags);
			break;

		case DTA_Color:
			parms->color = ListGetInt(tags);
			break;

		case DTA_FlipX:
			parms->flipX = ListGetInt(tags);
			break;

		case DTA_FlipY:
			parms->flipY = ListGetInt(tags);
			break;

		case DTA_SrcX:
			parms->srcx = ListGetDouble(tags) / img->GetDisplayWidth();
			break;

		case DTA_SrcY:
			parms->srcy = ListGetDouble(tags) / img->GetDisplayHeight();
			break;

		case DTA_SrcWidth:
			parms->srcwidth = ListGetDouble(tags) / img->GetDisplayWidth();
			break;

		case DTA_SrcHeight:
			parms->srcheight = ListGetDouble(tags) / img->GetDisplayHeight();
			break;

		case DTA_TopOffset:
			assert(fortext == false);
			if (fortext) return false;
			parms->top = ListGetInt(tags);
			break;

		case DTA_TopOffsetF:
			assert(fortext == false);
			if (fortext) return false;
			parms->top = ListGetDouble(tags);
			break;

		case DTA_LeftOffset:
			assert(fortext == false);
			if (fortext) return false;
			parms->left = ListGetInt(tags);
			break;

		case DTA_LeftOffsetF:
			assert(fortext == false);
			if (fortext) return false;
			parms->left = ListGetDouble(tags);
			break;

		case DTA_CenterOffset:
			assert(fortext == false);
			if (fortext) return false;
			if (ListGetInt(tags))
			{
				parms->left = img->GetDisplayWidth() * 0.5;
				parms->top = img->GetDisplayHeight() * 0.5;
			}
			break;

		case DTA_CenterBottomOffset:
			assert(fortext == false);
			if (fortext) return false;
			if (ListGetInt(tags))
			{
				parms->left = img->GetDisplayWidth() * 0.5;
				parms->top = img->GetDisplayHeight();
			}
			break;

		case DTA_WindowLeft:
			assert(fortext == false);
			if (fortext) return false;
			parms->windowleft = ListGetInt(tags);
			break;

		case DTA_WindowLeftF:
			assert(fortext == false);
			if (fortext) return false;
			parms->windowleft = ListGetDouble(tags);
			break;

		case DTA_WindowRight:
			assert(fortext == false);
			if (fortext) return false;
			parms->windowright = ListGetInt(tags);
			break;

		case DTA_WindowRightF:
			assert(fortext == false);
			if (fortext) return false;
			parms->windowright = ListGetDouble(tags);
			break;

		case DTA_ClipTop:
			parms->uclip = ListGetInt(tags);
			if (parms->uclip < 0)
			{
				parms->uclip = 0;
			}
			break;

		case DTA_ClipBottom:
			parms->dclip = ListGetInt(tags);
			if (parms->dclip > screen->GetHeight())
			{
				parms->dclip = screen->GetHeight();
			}
			break;

		case DTA_ClipLeft:
			parms->lclip = ListGetInt(tags);
			if (parms->lclip < 0)
			{
				parms->lclip = 0;
			}
			break;

		case DTA_ClipRight:
			parms->rclip = ListGetInt(tags);
			if (parms->rclip > screen->GetWidth())
			{
				parms->rclip = screen->GetWidth();
			}
			break;

		case DTA_ShadowAlpha:
			//parms->shadowAlpha = (float)std::min(1., ListGetDouble(tags));
			break;

		case DTA_ShadowColor:
			parms->shadowColor = ListGetInt(tags);
			break;

		case DTA_Shadow:
			boolval = ListGetInt(tags);
			if (boolval)
			{
				//parms->shadowAlpha = 0.5;
				parms->shadowColor = 0;
			}
			else
			{
				//parms->shadowAlpha = 0;
			}
			break;

		case DTA_Masked:
			parms->masked = ListGetInt(tags);
			break;

		case DTA_BilinearFilter:
			parms->bilinear = ListGetInt(tags);
			break;

		case DTA_KeepRatio:
			// I think this is a terribly misleading name, since it actually turns
			// *off* aspect ratio correction.
			parms->keepratio = ListGetInt(tags);
			break;

		case DTA_RenderStyle:
			parms->style.AsDWORD = ListGetInt(tags);
			break;

		case DTA_LegacyRenderStyle:	// mainly for ZScript which does not handle FRenderStyle that well.
			parms->style = (ERenderStyle)ListGetInt(tags);
			break;

		case DTA_Desaturate:
			parms->desaturate = ListGetInt(tags);
			break;

		case DTA_TextLen:
			parms->maxstrlen = ListGetInt(tags);
			break;

		case DTA_CellX:
			parms->cellx = ListGetInt(tags);
			break;

		case DTA_CellY:
			parms->celly = ListGetInt(tags);
			break;

		case DTA_Monospace:
			parms->monospace = ListGetInt(tags);
			break;

		case DTA_Spacing:
			parms->spacing = ListGetInt(tags);
			break;

		case DTA_Burn:
			parms->burn = true;
			break;

		}
		tag = ListGetInt(tags);
	}
	ListEnd(tags);

	if (parms->uclip >= parms->dclip || parms->lclip >= parms->rclip)
	{
		return false;
	}

	if (img != NULL)
	{
		SetTextureParms(parms, img, x, y);

		if (parms->destwidth <= 0 || parms->destheight <= 0)
		{
			return false;
		}
	}

	if (parms->style.BlendOp == 255)
	{
		if (fillcolorset)
		{
			if (parms->alphaChannel)
			{
				parms->style = STYLE_Shaded;
			}
			else if (parms->Alpha < 1.f)
			{
				parms->style = STYLE_TranslucentStencil;
			}
			else
			{
				parms->style = STYLE_Stencil;
			}
		}
		else //if (parms->Alpha < 1.f)
		{
			parms->style = STYLE_Translucent;
		}
		/*
		else
		{
			parms->style = STYLE_Normal;
		}
		*/
	}
	return true;
}

//==========================================================================
//
// Coordinate conversion
//
//==========================================================================

void VirtualToRealCoords(double &x, double &y, double &w, double &h,
	double vwidth, double vheight, bool vbottom, bool handleaspect) 
{
	float myratio = handleaspect ? ActiveRatio (screen->GetWidth(), screen->GetHeight()) : (4.0f / 3.0f);

    // if 21:9 AR, map to 16:9 for all callers.
    // this allows for black bars and stops the stretching of fullscreen images
    if (myratio > 1.7f) {
        myratio = 16.0f / 9.0f;
    }

	double right = x + w;
	double bottom = y + h;

	if (myratio > 1.334f)
	{ // The target surface is either 16:9 or 16:10, so expand the
	  // specified virtual size to avoid undesired stretching of the
	  // image. Does not handle non-4:3 virtual sizes. I'll worry about
	  // those if somebody expresses a desire to use them.
		x = (x - vwidth * 0.5) * screen->GetWidth() * 960 / (vwidth * AspectBaseWidth(myratio)) + screen->GetWidth() * 0.5;
		w = (right - vwidth * 0.5) * screen->GetWidth() * 960 / (vwidth * AspectBaseWidth(myratio)) + screen->GetWidth() * 0.5 - x;
	}
	else
	{
		x = x * screen->GetWidth() / vwidth;
		w = right * screen->GetWidth() / vwidth - x;
	}
	if (AspectTallerThanWide(myratio))
	{ // The target surface is 5:4
		y = (y - vheight * 0.5) * screen->GetHeight() * 600 / (vheight * AspectBaseHeight(myratio)) + screen->GetHeight() * 0.5;
		h = (bottom - vheight * 0.5) * screen->GetHeight() * 600 / (vheight * AspectBaseHeight(myratio)) + screen->GetHeight() * 0.5 - y;
		if (vbottom)
		{
			y += (screen->GetHeight() - screen->GetHeight() * AspectMultiplier(myratio) / 48.0) * 0.5;
		}
	}
	else
	{
		y = y * screen->GetHeight() / vheight;
		h = bottom * screen->GetHeight() / vheight - y;
	}
}

