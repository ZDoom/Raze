/*
** v_text.cpp
** Draws text to a canvas. Also has a text line-breaker thingy.
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

#include <stdlib.h>
#include <stdarg.h>
#include <ctype.h>
#include <wctype.h>

#include "utf8.h"
#include "v_text.h"

#include "drawparms.h"
#include "v_draw.h"
#include "image.h"
#include "v_2ddrawer.h"
#include "gstrings.h"
#include "v_font.h"

class FFont;

//==========================================================================
//
// Internal texture drawing function
//
//==========================================================================

void DrawTexture(F2DDrawer *drawer, FTexture* img, double x, double y, int tags_first, ...)
{
	Va_List tags;
	va_start(tags.list, tags_first);
	DrawParms parms;

	bool res = ParseDrawTextureTags(img, x, y, tags_first, tags, &parms, false);
	va_end(tags.list);
	if (!res)
	{
		return;
	}
	drawer->AddTexture(img, parms);
}

//==========================================================================
//
// DrawChar
//
// Write a single character using the given font
//
//==========================================================================

void DrawChar (F2DDrawer* drawer, FFont *font, int normalcolor, double x, double y, int character, int tag_first, ...)
{
	if (font == NULL)
		return;

	if (normalcolor >= NumTextColors)
		normalcolor = CR_UNTRANSLATED;

	FTexture *pic;
	int dummy;
	bool redirected;

	if (NULL != (pic = font->GetChar (character, normalcolor, &dummy, &redirected)))
	{
		DrawParms parms;
		Va_List tags;
		va_start(tags.list, tag_first);
		bool res = ParseDrawTextureTags(pic, x, y, tag_first, tags, &parms, false);
		va_end(tags.list);
		if (!res)
		{
			return;
		}
		PalEntry color = 0xffffffff;
		parms.TranslationId = redirected? -1 : font->GetColorTranslation((EColorRange)normalcolor, &color);
		parms.color = PalEntry((color.a * parms.color.a) / 255, (color.r * parms.color.r) / 255, (color.g * parms.color.g) / 255, (color.b * parms.color.b) / 255);
		drawer->AddTexture(pic, parms);
	}
}

//==========================================================================
//
// DrawText
//
// Write a string using the given font
//
//==========================================================================

// This is only needed as a dummy. The code using wide strings does not need color control.
EColorRange V_ParseFontColor(const char32_t *&color_value, int normalcolor, int boldcolor) { return CR_UNTRANSLATED; } 

template<class chartype>
void DrawTextCommon(F2DDrawer* drawer, FFont *font, int normalcolor, double x, double y, const chartype *string, DrawParms &parms)
{
	int 		w;
	const chartype *ch;
	int 		c;
	double 		cx;
	double 		cy;
	int			boldcolor;
	int			kerning;
	FTexture *pic;

	if (parms.celly == 0) parms.celly = font->GetHeight() + 1;
	parms.celly *= parms.scaley;

	if (normalcolor >= NumTextColors)
		normalcolor = CR_UNTRANSLATED;
	boldcolor = normalcolor ? normalcolor - 1 : NumTextColors - 1;

	PalEntry colorparm = parms.color;
	PalEntry color = 0xffffffff;
	parms.TranslationId = font->GetColorTranslation((EColorRange)normalcolor, &color);
	parms.color = PalEntry(colorparm.a, (color.r * colorparm.r) / 255, (color.g * colorparm.g) / 255, (color.b * colorparm.b) / 255);

	kerning = font->GetDefaultKerning();

	ch = string;
	cx = x;
	cy = y;

	if (parms.monospace == EMonospacing::CellCenter)
		cx += parms.spacing / 2;
	else if (parms.monospace == EMonospacing::CellRight)
		cx += parms.spacing;


	auto currentcolor = normalcolor;
	while (ch - string < parms.maxstrlen)
	{
		c = GetCharFromString(ch);
		if (!c)
			break;

		if (c == TEXTCOLOR_ESCAPE)
		{
			EColorRange newcolor = V_ParseFontColor(ch, normalcolor, boldcolor);
			if (newcolor != CR_UNDEFINED)
			{
				parms.TranslationId = font->GetColorTranslation(newcolor, &color);
				parms.color = PalEntry(colorparm.a, (color.r * colorparm.r) / 255, (color.g * colorparm.g) / 255, (color.b * colorparm.b) / 255);
				currentcolor = newcolor;
			}
			continue;
		}

		if (c == '\n')
		{
			cx = x;
			cy += parms.celly;
			continue;
		}

		bool redirected = false;
		if (NULL != (pic = font->GetChar(c, currentcolor, &w, &redirected)))
		{
			SetTextureParms(&parms, pic, cx, cy);
			if (parms.cellx)
			{
				w = parms.cellx;
				parms.destwidth = parms.cellx;
				parms.destheight = parms.celly;
			}
			if (parms.monospace == EMonospacing::CellLeft)
				parms.left = 0;
			else if (parms.monospace == EMonospacing::CellCenter)
				parms.left = w / 2.;
			else if (parms.monospace == EMonospacing::CellRight)
				parms.left = w;

			drawer->AddTexture(pic, parms);
		}
		if (parms.monospace == EMonospacing::Off)
		{
			cx += (w + kerning + parms.spacing) * parms.scalex;
		}
		else
		{
			cx += (parms.spacing) * parms.scalex;
		}
	}
}

void DrawText(F2DDrawer* drawer, FFont *font, int normalcolor, double x, double y, const char *string, int tag_first, ...)
{
	Va_List tags;
	DrawParms parms;

	if (font == NULL || string == NULL)
		return;

	va_start(tags.list, tag_first);
	bool res = ParseDrawTextureTags(nullptr, 0, 0, tag_first, tags, &parms, true);
	va_end(tags.list);
	if (!res)
	{
		return;
	}
	DrawTextCommon(drawer, font, normalcolor, x, y, (const uint8_t*)GStrings.localize(string), parms);
}

void DrawText(F2DDrawer* drawer, FFont *font, int normalcolor, double x, double y, const char32_t *string, int tag_first, ...)
{
	Va_List tags;
	DrawParms parms;

	if (font == NULL || string == NULL)
		return;

	va_start(tags.list, tag_first);
	bool res = ParseDrawTextureTags(nullptr, 0, 0, tag_first, tags, &parms, true);
	va_end(tags.list);
	if (!res)
	{
		return;
	}
	DrawTextCommon(drawer, font, normalcolor, x, y, string, parms);
}

//==========================================================================
//
// V_DrawFrame
//
// Draw a frame around the specified area using the view border
// frame graphics. The border is drawn outside the area, not in it.
//
//==========================================================================

void DrawFrame(F2DDrawer* twod, PalEntry color, int left, int top, int width, int height, int thickness)
{
	// Sanity check for incomplete gameinfo
	int offset = thickness == -1 ? screen->GetHeight() / 400 : thickness;
	int right = left + width;
	int bottom = top + height;

	// Draw top and bottom sides.
	twod->AddColorOnlyQuad(left, top - offset, width, offset, color);
	twod->AddColorOnlyQuad(left - offset, top - offset, offset, height + 2 * offset, color);
	twod->AddColorOnlyQuad(left, bottom, width, offset, color);
	twod->AddColorOnlyQuad(right, top - offset, offset, height + 2 * offset, color);
}
