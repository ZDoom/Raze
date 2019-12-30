// 
//---------------------------------------------------------------------------
//
// Copyright(C) 2016-2018 Christoph Oelckers
// All rights reserved.
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public License
// along with this program.  If not, see http://www.gnu.org/licenses/
//
//--------------------------------------------------------------------------
//
/*
** v_2ddrawer.h
** Device independent 2D draw list
**
**/

#include <stdarg.h>
#include "c_cvars.h"
#include "v_2ddrawer.h"
#include "renderstyle.h"
#include "drawparms.h"
#include "vectors.h"
#include "gamecvars.h"
//#include "doomtype.h"
#include "templates.h"
//#include "r_utility.h"
//#include "v_video.h"
//#include "g_levellocals.h"
//#include "vm.h"

F2DDrawer twod;

//==========================================================================
//
//
//
//==========================================================================

int F2DDrawer::AddCommand(const RenderCommand *data) 
{
	if (mData.Size() > 0 && data->isCompatible(mData.Last()))
	{
		// Merge with the last command.
		mData.Last().mIndexCount += data->mIndexCount;
		mData.Last().mVertCount += data->mVertCount;
		return mData.Size();
	}
	else
	{
		return mData.Push(*data);
	}
}

//==========================================================================
//
//
//
//==========================================================================

void F2DDrawer::AddIndices(int firstvert, int count, ...)
{
	va_list ap;
	va_start(ap, count);
	int addr = mIndices.Reserve(count);
	for (int i = 0; i < count; i++)
	{
		mIndices[addr + i] = firstvert + va_arg(ap, int);
	}
}

//==========================================================================
//
// SetStyle
//
// Patterned after R_SetPatchStyle.
//
//==========================================================================

bool F2DDrawer::SetStyle(FTexture *tex, DrawParms &parms, PalEntry &vertexcolor, RenderCommand &quad)
{
	FRenderStyle style = parms.style;
	float alpha;
	bool stencilling;

	if (style.Flags & STYLEF_Alpha1)
	{
		alpha = 1;
	}
	else
	{
		alpha = clamp(parms.Alpha, 0.f, 1.f);
	}

	style.CheckFuzz();
	if (style.BlendOp == STYLEOP_Shadow || style.BlendOp == STYLEOP_Fuzz)
	{
		style = LegacyRenderStyles[STYLE_TranslucentStencil];
		alpha = 0.3f;
		parms.fillcolor = 0;
	}
	else if (style.BlendOp == STYLEOP_FuzzOrAdd)
	{
		style.BlendOp = STYLEOP_Add;
	}
	else if (style.BlendOp == STYLEOP_FuzzOrSub)
	{
		style.BlendOp = STYLEOP_Sub;
	}
	else if (style.BlendOp == STYLEOP_FuzzOrRevSub)
	{
		style.BlendOp = STYLEOP_RevSub;
	}

	stencilling = false;

	if (style.Flags & STYLEF_InvertOverlay)
	{
		// Only the overlay color is inverted, not the overlay alpha.
		parms.colorOverlay.r = 255 - parms.colorOverlay.r;
		parms.colorOverlay.g = 255 - parms.colorOverlay.g;
		parms.colorOverlay.b = 255 - parms.colorOverlay.b;
	}

	SetColorOverlay(parms.colorOverlay, alpha, vertexcolor, quad.mColor1);

	if (style.Flags & STYLEF_ColorIsFixed)
	{
		if (style.Flags & STYLEF_InvertSource)
		{ // Since the source color is a constant, we can invert it now
		  // without spending time doing it in the shader.
			parms.fillcolor.r = 255 - parms.fillcolor.r;
			parms.fillcolor.g = 255 - parms.fillcolor.g;
			parms.fillcolor.b = 255 - parms.fillcolor.b;
			style.Flags &= ~STYLEF_InvertSource;
		}
		if (parms.desaturate > 0)
		{
			// Desaturation can also be computed here without having to do it in the shader.
			auto gray = parms.fillcolor.Luminance();
			auto notgray = 255 - gray;
			parms.fillcolor.r = uint8_t((parms.fillcolor.r * notgray + gray * 255) / 255);
			parms.fillcolor.g = uint8_t((parms.fillcolor.g * notgray + gray * 255) / 255);
			parms.fillcolor.b = uint8_t((parms.fillcolor.b * notgray + gray * 255) / 255);
			parms.desaturate = 0;
		}

		// Set up the color mod to replace the color from the image data.
		vertexcolor.r = parms.fillcolor.r;
		vertexcolor.g = parms.fillcolor.g;
		vertexcolor.b = parms.fillcolor.b;

		if (style.Flags & STYLEF_RedIsAlpha)
		{
			quad.mDrawMode = TM_ALPHATEXTURE;
		}
		else
		{
			quad.mDrawMode = TM_STENCIL;
		}
	}
	else
	{
		if (style.Flags & STYLEF_RedIsAlpha)
		{
			quad.mDrawMode = TM_ALPHATEXTURE;
		}
		else if (style.Flags & STYLEF_InvertSource)
		{
			quad.mDrawMode = TM_INVERSE;
		}
		quad.mDesaturate = parms.desaturate;
	}
	// apply the element's own color. This is being blended with anything that came before.
	vertexcolor = PalEntry((vertexcolor.a * parms.color.a) / 255, (vertexcolor.r * parms.color.r) / 255, (vertexcolor.g * parms.color.g) / 255, (vertexcolor.b * parms.color.b) / 255);

	if (!parms.masked)
	{
		// For TM_ALPHATEXTURE and TM_STENCIL the mask cannot be turned off because it would not yield a usable result.
		if (quad.mDrawMode == TM_NORMAL) quad.mDrawMode = TM_OPAQUE;
		else if (quad.mDrawMode == TM_INVERSE) quad.mDrawMode = TM_INVERTOPAQUE;
	}
	quad.mRenderStyle = parms.style;	// this  contains the blend mode and blend equation settings.
    if (parms.burn) quad.mFlags |= DTF_Burn;
	return true;
}

//==========================================================================
//
// Draws a texture
//
//==========================================================================

void F2DDrawer::SetColorOverlay(PalEntry color, float alpha, PalEntry &vertexcolor, PalEntry &overlaycolor)
{
	if (color.a != 0 && (color & 0xffffff) != 0)
	{
		// overlay color uses premultiplied alpha.
		int a = color.a * 256 / 255;
		overlaycolor.r = (color.r * a) >> 8;
		overlaycolor.g = (color.g * a) >> 8;
		overlaycolor.b = (color.b * a) >> 8;
		overlaycolor.a = 0;	// The overlay gets added on top of the texture data so to preserve the pixel's alpha this must be 0.
	}
	else
	{
		overlaycolor = 0;
	}
	// Vertex intensity is the inverse of the overlay so that the shader can do a simple addition to combine them.
	uint8_t light = 255 - color.a;
	vertexcolor = PalEntry(int(alpha * 255), light, light, light);

	// The real color gets multiplied into vertexcolor later.
}

//==========================================================================
//
// Draws a texture
//
//==========================================================================

void F2DDrawer::AddTexture(FTexture *img, DrawParms &parms)
{
	if (parms.style.BlendOp == STYLEOP_None) return;	// not supposed to be drawn.

	double xscale = parms.destwidth / parms.texwidth;
	double yscale = parms.destheight / parms.texheight;
	double x = parms.x - parms.left * xscale;
	double y = parms.y - parms.top * yscale;
	double w = parms.destwidth;
	double h = parms.destheight;
	double u1, v1, u2, v2;
	PalEntry vertexcolor;

	RenderCommand dg;

	dg.mType = DrawTypeTriangles;
	dg.mVertCount = 4;
	dg.mTexture = img;

	dg.mRemapIndex = parms.remap;
	SetStyle(img, parms, vertexcolor, dg);

	u1 = parms.srcx;
	v1 = parms.srcy;
	u2 = parms.srcx + parms.srcwidth;
	v2 = parms.srcy + parms.srcheight;

	if (parms.flipX) 
		std::swap(u1, u2);

	if (parms.flipY)
		std::swap(v1, v2);

	// This is crap. Only kept for backwards compatibility with scripts that may have used it.
	// Note that this only works for unflipped full textures.
	if (parms.windowleft > 0 || parms.windowright < parms.texwidth)
	{
		double wi = std::min(parms.windowright, parms.texwidth);
		x += parms.windowleft * xscale;
		w -= (parms.texwidth - wi + parms.windowleft) * xscale;

		u1 = float(u1 + parms.windowleft / parms.texwidth);
		u2 = float(u2 - (parms.texwidth - wi) / parms.texwidth);
	}

	if (x < (double)parms.lclip || y < (double)parms.uclip || x + w >(double)parms.rclip || y + h >(double)parms.dclip)
	{
		dg.mScissor[0] = parms.lclip;
		dg.mScissor[1] = parms.uclip;
		dg.mScissor[2] = parms.rclip;
		dg.mScissor[3] = parms.dclip;
		dg.mFlags |= DTF_Scissor;
	}
	else
	{
		memset(dg.mScissor, 0, sizeof(dg.mScissor));
	}

	dg.mVertCount = 4;
	dg.mVertIndex = (int)mVertices.Reserve(4);
	TwoDVertex *ptr = &mVertices[dg.mVertIndex];
	ptr->Set(x, y, 0, u1, v1, vertexcolor); ptr++;
	ptr->Set(x, y + h, 0, u1, v2, vertexcolor); ptr++;
	ptr->Set(x + w, y, 0, u2, v1, vertexcolor); ptr++;
	ptr->Set(x + w, y + h, 0, u2, v2, vertexcolor); ptr++;
	dg.mIndexIndex = mIndices.Size();
	dg.mIndexCount += 6;
	AddIndices(dg.mVertIndex, 6, 0, 1, 2, 1, 3, 2);
	AddCommand(&dg);
}

//==========================================================================
//
//
//
//==========================================================================
void F2DDrawer::AddPoly(FTexture *texture, FVector2 *points, int npoints,
		double originx, double originy, double scalex, double scaley,
		DAngle rotation, int colormap, PalEntry flatcolor, int lightlevel,
		uint32_t *indices, size_t indexcount)
{
	// Use an equation similar to player sprites to determine shade

	// Convert a light level into an unbounded colormap index (shade). 
	// Why the +12? I wish I knew, but experimentation indicates it
	// is necessary in order to best reproduce Doom's original lighting.
	double fadelevel;

	// The hardware renderer's light modes 0, 1 and 4 use a linear light scale which must be used here as well. Otherwise the automap gets too dark.
	fadelevel = 1. - clamp(lightlevel, 0, 255) / 255.f;

	RenderCommand poly;

	poly.mType = DrawTypeTriangles;
	poly.mTexture = texture;
	poly.mRenderStyle = DefaultRenderStyle();
	poly.mFlags |= DTF_Wrap;
	poly.mDesaturate = 0;

	PalEntry color0; 
	double invfade = 1. - fadelevel;

	color0.r = uint8_t(flatcolor.r * invfade);
	color0.g = uint8_t(flatcolor.g * invfade);
	color0.b = uint8_t(flatcolor.b * invfade);
	color0.a = 255;

	poly.mColor1 = 0;

	bool dorotate = rotation != 0;

	float cosrot = (float)cos(rotation.Radians());
	float sinrot = (float)sin(rotation.Radians());

	float uscale = float(1.f / (texture->GetWidth() * scalex));
	float vscale = float(1.f / (texture->GetHeight() * scaley));
	float ox = float(originx);
	float oy = float(originy);

	poly.mVertCount = npoints;
	poly.mVertIndex = (int)mVertices.Reserve(npoints);
	for (int i = 0; i < npoints; ++i)
	{
		float u = points[i].X - 0.5f - ox;
		float v = points[i].Y - 0.5f - oy;
		if (dorotate)
		{
			float t = u;
			u = t * cosrot - v * sinrot;
			v = v * cosrot + t * sinrot;
		}
		mVertices[poly.mVertIndex+i].Set(points[i].X, points[i].Y, 0, u*uscale, v*vscale, color0);
	}
	poly.mIndexIndex = mIndices.Size();

	if (indices == nullptr || indexcount == 0)
	{
		poly.mIndexCount += (npoints - 2) * 3;
		for (int i = 2; i < npoints; ++i)
		{
			AddIndices(poly.mVertIndex, 3, 0, i - 1, i);
		}
	}
	else
	{
		poly.mIndexCount += (int)indexcount;
		int addr = mIndices.Reserve(indexcount);
		for (size_t i = 0; i < indexcount; i++)
		{
			mIndices[addr + i] = poly.mVertIndex + indices[i];
		}
	}

	AddCommand(&poly);
}

//==========================================================================
//
//
//
//==========================================================================

void F2DDrawer::AddFlatFill(int left, int top, int right, int bottom, FTexture *src, bool local_origin)
{
	float fU1, fU2, fV1, fV2;

	RenderCommand dg;

	dg.mType = DrawTypeTriangles;
	dg.mRenderStyle = DefaultRenderStyle();
	dg.mTexture = src;
	dg.mVertCount = 4;
	dg.mTexture = src;
	dg.mFlags = DTF_Wrap;

	// scaling is not used here.
	if (!local_origin)
	{
		fU1 = float(left) / src->GetWidth();
		fV1 = float(top) / src->GetHeight();
		fU2 = float(right) / src->GetWidth();
		fV2 = float(bottom) / src->GetHeight();
	}
	else
	{
		fU1 = 0;
		fV1 = 0;
		fU2 = float(right - left) / src->GetWidth();
		fV2 = float(bottom - top) / src->GetHeight();
	}
	dg.mVertIndex = (int)mVertices.Reserve(4);
	auto ptr = &mVertices[dg.mVertIndex];

	ptr->Set(left, top, 0, fU1, fV1, 0xffffffff); ptr++;
	ptr->Set(left, bottom, 0, fU1, fV2, 0xffffffff); ptr++;
	ptr->Set(right, top, 0, fU2, fV1, 0xffffffff); ptr++;
	ptr->Set(right, bottom, 0, fU2, fV2, 0xffffffff); ptr++;
	dg.mIndexIndex = mIndices.Size();
	dg.mIndexCount += 6;
	AddIndices(dg.mVertIndex, 6, 0, 1, 2, 1, 3, 2);
	AddCommand(&dg);
}


//===========================================================================
// 
//
//
//===========================================================================

void F2DDrawer::AddColorOnlyQuad(int x1, int y1, int w, int h, PalEntry color, FRenderStyle *style)
{
	RenderCommand dg;

	dg.mType = DrawTypeTriangles;
	dg.mVertCount = 4;
	dg.mVertIndex = (int)mVertices.Reserve(4);
	dg.mRenderStyle = style? *style : LegacyRenderStyles[STYLE_Translucent];
	auto ptr = &mVertices[dg.mVertIndex];
	ptr->Set(x1, y1, 0, 0, 0, color); ptr++;
	ptr->Set(x1, y1 + h, 0, 0, 0, color); ptr++;
	ptr->Set(x1 + w, y1, 0, 0, 0, color); ptr++;
	ptr->Set(x1 + w, y1 + h, 0, 0, 0, color); ptr++;
	dg.mIndexIndex = mIndices.Size();
	dg.mIndexCount += 6;
	AddIndices(dg.mVertIndex, 6, 0, 1, 2, 1, 3, 2);
	AddCommand(&dg);
}

//==========================================================================
//
//
//
//==========================================================================

void F2DDrawer::AddLine(int x1, int y1, int x2, int y2, uint32_t color, uint8_t alpha)
{
	PalEntry p = (PalEntry)color;
	p.a = alpha;

	RenderCommand dg;

	dg.mType = DrawTypeLines;
	dg.mRenderStyle = LegacyRenderStyles[STYLE_Translucent];
	dg.mVertCount = 2;
	dg.mVertIndex = (int)mVertices.Reserve(2);
	mVertices[dg.mVertIndex].Set(x1, y1, 0, 0, 0, p);
	mVertices[dg.mVertIndex+1].Set(x2, y2, 0, 0, 0, p);
	AddCommand(&dg);
}

//==========================================================================
//
//
//
//==========================================================================

void F2DDrawer::AddThickLine(int x1, int y1, int x2, int y2, double thickness, uint32_t color, uint8_t alpha)
{
	PalEntry p = (PalEntry)color;
	p.a = alpha;

	DVector2 point0(x1, y1);
	DVector2 point1(x2, y2);

	DVector2 delta = point1 - point0;
	DVector2 perp(-delta.Y, delta.X);
	perp.MakeUnit();
	perp *= thickness / 2;

	DVector2 corner0 = point0 + perp;
	DVector2 corner1 = point0 - perp;
	DVector2 corner2 = point1 + perp;
	DVector2 corner3 = point1 - perp;

	RenderCommand dg;

	dg.mType = DrawTypeTriangles;
	dg.mVertCount = 4;
	dg.mVertIndex = (int)mVertices.Reserve(4);
	dg.mRenderStyle = LegacyRenderStyles[STYLE_Translucent];
	auto ptr = &mVertices[dg.mVertIndex];
	ptr->Set(corner0.X, corner0.Y, 0, 0, 0, p); ptr++;
	ptr->Set(corner1.X, corner1.Y, 0, 0, 0, p); ptr++;
	ptr->Set(corner2.X, corner2.Y, 0, 0, 0, p); ptr++;
	ptr->Set(corner3.X, corner3.Y, 0, 0, 0, p); ptr++;
	dg.mIndexIndex = mIndices.Size();
	dg.mIndexCount += 6;
	AddIndices(dg.mVertIndex, 6, 0, 1, 2, 1, 3, 2);
	AddCommand(&dg);
}

//==========================================================================
//
//
//
//==========================================================================

void F2DDrawer::AddPixel(int x1, int y1, uint32_t color)
{
	PalEntry p = (PalEntry)color;
	p.a = 255;

	RenderCommand dg;

	dg.mType = DrawTypePoints;
	dg.mRenderStyle = LegacyRenderStyles[STYLE_Translucent];
	dg.mVertCount = 1;
	dg.mVertIndex = (int)mVertices.Reserve(1);
	mVertices[dg.mVertIndex].Set(x1, y1, 0, 0, 0, p);
	AddCommand(&dg);
}

//==========================================================================
//
//
//
//==========================================================================

void F2DDrawer::Clear()
{
	mVertices.Clear();
	mIndices.Clear();
	mData.Clear();
	mIsFirstPass = true;
}

//==========================================================================
//
//
//
//==========================================================================


#include "build.h"
#include "../src/engine_priv.h"

//sx,sy       center of sprite; screen coords*65536
//z           zoom*65536. > is zoomed in
//a           angle (0 is default)
//dastat&1    1:translucence
//dastat&2    1:auto-scale mode (use 320*200 coordinates)
//dastat&4    1:y-flip
//dastat&8    1:don't clip to startumost/startdmost
//dastat&16   1:force point passed to be top-left corner, 0:Editart center
//dastat&32   1:reverse translucence
//dastat&64   1:non-masked, 0:masked
//dastat&128  1:draw all pages (permanent - no longer used)
//cx1,...     clip window (actual screen coords)

//==========================================================================
//
// INTERNAL helper function for classic/polymost dorotatesprite
//  sxptr, sxptr, z: in/out
//  ret_yxaspect, ret_xyaspect: out
//
//==========================================================================

static int32_t dorotspr_handle_bit2(int32_t* sxptr, int32_t* syptr, int32_t* z, int32_t dastat, int32_t cx1_plus_cx2, int32_t cy1_plus_cy2)
{
	if ((dastat & RS_AUTO) == 0)
	{
		if (!(dastat & RS_STRETCH) && 4 * ydim <= 3 * xdim)
		{
			return (10 << 16) / 12;
		}
		else
		{
			return xyaspect;
		}
	}
	else
	{
		// dastat&2: Auto window size scaling
		const int32_t oxdim = xdim;
		const int32_t oydim = ydim;
		int32_t xdim = oxdim;  // SHADOWS global
		int32_t ydim = oydim;

		int32_t zoomsc, sx = *sxptr, sy = *syptr;
		int32_t ouryxaspect = yxaspect, ourxyaspect = xyaspect;

		sy += rotatesprite_y_offset;

		if (!(dastat & RS_STRETCH) && 4 * ydim <= 3 * xdim)
		{
			if ((dastat & RS_ALIGN_MASK) && (dastat & RS_ALIGN_MASK) != RS_ALIGN_MASK)
				sx += NEGATE_ON_CONDITION(scale(120 << 16, xdim, ydim) - (160 << 16), !(dastat & RS_ALIGN_R));

			if ((dastat & RS_ALIGN_MASK) == RS_ALIGN_MASK)
				ydim = scale(xdim, 3, 4);
			else
				xdim = scale(ydim, 4, 3);

			ouryxaspect = (12 << 16) / 10;
			ourxyaspect = (10 << 16) / 12;
		}

		ouryxaspect = mulscale16(ouryxaspect, rotatesprite_yxaspect);
		ourxyaspect = divscale16(ourxyaspect, rotatesprite_yxaspect);

		// screen center to s[xy], 320<<16 coords.
		const int32_t normxofs = sx - (320 << 15), normyofs = sy - (200 << 15);

		// nasty hacks go here
		if (!(dastat & RS_NOCLIP))
		{
			const int32_t twice_midcx = cx1_plus_cx2 + 2;

			// screen x center to sx1, scaled to viewport
			const int32_t scaledxofs = scale(normxofs, scale(xdimen, xdim, oxdim), 320);

			sx = ((twice_midcx) << 15) + scaledxofs;

			zoomsc = xdimenscale;   //= scale(xdimen,yxaspect,320);
			zoomsc = mulscale16(zoomsc, rotatesprite_yxaspect);

			if ((dastat & RS_ALIGN_MASK) == RS_ALIGN_MASK)
				zoomsc = scale(zoomsc, ydim, oydim);

			sy = ((cy1_plus_cy2 + 2) << 15) + mulscale16(normyofs, zoomsc);
		}
		else
		{
			//If not clipping to startmosts, & auto-scaling on, as a
			//hard-coded bonus, scale to full screen instead

			sx = (xdim << 15) + 32768 + scale(normxofs, xdim, 320);

			zoomsc = scale(xdim, ouryxaspect, 320);
			sy = (ydim << 15) + 32768 + mulscale16(normyofs, zoomsc);

			if ((dastat & RS_ALIGN_MASK) == RS_ALIGN_MASK)
				sy += (oydim - ydim) << 15;
			else
				sx += (oxdim - xdim) << 15;

			if (dastat & RS_CENTERORIGIN)
				sx += oxdim << 15;
		}

		*sxptr = sx;
		*syptr = sy;
		*z = mulscale16(*z, zoomsc);

		return ourxyaspect;
	}
}

//==========================================================================
//
//
//
//==========================================================================

void F2DDrawer::rotatesprite(int32_t sx, int32_t sy, int32_t z, int16_t a, int16_t picnum,
	int8_t dashade, uint8_t dapalnum, int32_t dastat, uint8_t daalpha, uint8_t dablend,
	int32_t clipx1, int32_t clipy1, int32_t clipx2, int32_t clipy2)
{
	RenderCommand dg = {};
	int method = 0;

	dg.mType = DrawTypeRotateSprite;
	if (clipx1 > 0 || clipy1 > 0 || clipx2 < xdim - 1 || clipy2 < ydim - 1)
	{
		dg.mScissor[0] = clipx1;
		dg.mScissor[1] = clipy1;
		dg.mScissor[2] = clipx2 + 1;
		dg.mScissor[3] = clipy2 + 1;
		dg.mFlags |= DTF_Scissor;
	}

	if (!(dastat & RS_NOMASK))
	{
		if (dastat & RS_TRANS1)
			method |= (dastat & RS_TRANS2) ? DAMETH_TRANS2 : DAMETH_TRANS1;
		else
			method |= DAMETH_MASK;

		dg.mRenderStyle = GetBlend(dablend, (dastat & RS_TRANS2) ? 1 : 0);
	}
	else
	{
		dg.mRenderStyle = LegacyRenderStyles[STYLE_Normal];
	}

	PalEntry p = 0xffffffff;
	dg.mTexture = TileFiles.tiles[picnum];
	dg.mRemapIndex = dapalnum | (dashade << 16);
	dg.mVertCount = 4;
	dg.mVertIndex = (int)mVertices.Reserve(4);
	dg.mRenderStyle = LegacyRenderStyles[STYLE_Translucent];
	auto ptr = &mVertices[dg.mVertIndex];
	float drawpoly_alpha = daalpha * (1.0f / 255.0f);
	float alpha = float_trans(method, dablend) * (1.f - drawpoly_alpha); // Hmmm...

	vec2_16_t const siz = tilesiz[picnum];
	vec2_16_t ofs = { 0, 0 };

	if (!(dastat & RS_TOPLEFT))
	{
		ofs = { int16_t(picanm[picnum].xofs + (siz.x >> 1)),
				int16_t(picanm[picnum].yofs + (siz.y >> 1)) };
	}

	if (dastat & RS_YFLIP)
		ofs.y = siz.y - ofs.y;

	int32_t aspectcorrect = dorotspr_handle_bit2(&sx, &sy, &z, dastat, clipx1 + clipx2, clipy1 + clipy2);

	int32_t cosang = mulscale14(sintable[(a + 512) & 2047], z);
	int32_t cosang2 = cosang;
	int32_t sinang = mulscale14(sintable[a & 2047], z);
	int32_t sinang2 = sinang;

	if ((dastat & RS_AUTO) || (!(dastat & RS_NOCLIP)))  // Don't aspect unscaled perms
	{
		cosang2 = mulscale16(cosang2, aspectcorrect);
		sinang2 = mulscale16(sinang2, aspectcorrect);
	}

	int cx0 = sx - ofs.x * cosang2 + ofs.y * sinang2;
	int cy0 = sy - ofs.x * sinang - ofs.y * cosang;

	int cx1 = cx0 + siz.x * cosang2;
	int cy1 = cy0 + siz.x * sinang;

	int cx3 = cx0 - siz.y * sinang2;
	int cy3 = cy0 + siz.y * cosang;

	int cx2 = cx1 + cx3 - cx0;
	int cy2 = cy1 + cy3 - cy0;

	float y = (dastat & RS_YFLIP) ? 1.f : 0.f;

	ptr->Set(cx0 / 65536.f, cy0 / 65536.f, 0.f, 0.f, y, p); ptr++;
	ptr->Set(cx1 / 65536.f, cy1 / 65536.f, 0.f, 1.f, y, p); ptr++;
	ptr->Set(cx2 / 65536.f, cy2 / 65536.f, 0.f, 1.f, 1.f-y, p); ptr++;
	ptr->Set(cx3 / 65536.f, cy3 / 65536.f, 0.f, 0.f, 1.f-y, p); ptr++;
	dg.mIndexIndex = mIndices.Size();
	dg.mIndexCount += 6;
	AddIndices(dg.mVertIndex, 6, 0, 1, 2, 0, 2, 3);
	AddCommand(&dg);

}
