#pragma once

/*
 ** imagehelpers.h
 ** Utilities for image conversion - mostly 8 bit paletted baggage
 **
 **---------------------------------------------------------------------------
 ** Copyright 2004-2007 Randy Heit
 ** Copyright 2006-2018 Christoph Oelckers
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
 **
 */


#include <stdint.h>
#include "tarray.h"
#include "palentry.h"
#include "bitmap.h"

// we do not want to pull in the entirety of build.h here.
extern uint8_t palette[768];

namespace ImageHelpers
{
	union ColorTable256k
	{
		uint8_t RGB[64][64][64];
		uint8_t All[64 * 64 * 64];
	};

	extern uint8_t GrayMap[256];
	extern int WhiteIndex, BlackIndex;
	extern ColorTable256k RGB256k;
	extern int alphaThreshold;

	// Todo: This should not pick fullbright colors.
	int BestColor(int r, int g, int b, int first = 0, int num = 255);
	int PTM_BestColor(int r, int g, int b, bool reverselookup, float powtable_val, int first, int num);
	void SetPalette(const PalEntry* colors);


	// Helpers for creating paletted images.
	inline uint8_t *GetGraymap()
	{
		return GrayMap;
	}
	
	inline uint8_t RGBToPalettePrecise(bool wantluminance, int r, int g, int b, int a = 255)
	{
		return BestColor(r, g, b);
	}
	
	inline uint8_t RGBToPalette(bool wantluminance, int r, int g, int b, int a = 255)
	{
		return a < alphaThreshold? 255 : RGB256k.RGB[r >> 2][g >> 2][b >> 2];
	}
	
	inline uint8_t RGBToPalette(bool wantluminance, PalEntry pe, bool hasalpha = true)
	{
		return RGBToPalette(wantluminance, pe.r, pe.g, pe.b, hasalpha? pe.a : 255);
	}
	
	//==========================================================================
	//
	// Converts a texture between row-major and column-major format
	// by flipping it about the X=Y axis.
	//
	//==========================================================================
	
	template<class T>
	void FlipSquareBlock (T *block, int x)
	{
		for (int i = 0; i < x; ++i)
		{
			T *corner = block + x*i + i;
			int count = x - i;
			for (int j = 0; j < count; j++)
			{
				std::swap(corner[j], corner[j*x]);
			}
		}
	}
	
	inline void FlipSquareBlockRemap (uint8_t *block, int x, const uint8_t *remap)
	{
		for (int i = 0; i < x; ++i)
		{
			uint8_t *corner = block + x*i + i;
			int count = x - i;
			for (int j = 0; j < count; j++)
			{
				auto t = remap[corner[j]];
				corner[j] = remap[corner[j*x]];
				corner[j*x] = t;
			}
		}
	}
	
	template<class T>
	void FlipNonSquareBlock (T *dst, const T *src, int x, int y, int srcpitch)
	{
		for (int i = 0; i < x; ++i)
		{
			for (int j = 0; j < y; ++j)
			{
				dst[i*y+j] = src[i+j*srcpitch];
			}
		}
	}
	
	inline void FlipNonSquareBlockRemap (uint8_t *dst, const uint8_t *src, int x, int y, int srcpitch, const uint8_t *remap)
	{
		for (int i = 0; i < x; ++i)
		{
			for (int j = 0; j < y; ++j)
			{
				dst[i*y+j] = remap[src[i+j*srcpitch]];
			}
		}
	}
	
	struct _BasePalette
	{
		PalEntry operator[](int index)
		{
			return PalEntry(
				palette[index * 3 + 0],
				palette[index * 3 + 1],
				palette[index * 3 + 2]
			);
		}
	};
	
	extern _BasePalette BasePalette;
}
