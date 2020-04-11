
/*
 ** imagehelpers.cpp
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
 
 #include "imagehelpers.h"

namespace ImageHelpers
{
	uint8_t GrayMap[256];
	int WhiteIndex, BlackIndex;
	int alphaThreshold;
	ColorTable256k RGB256k;
	_BasePalette BasePalette;

	int BestColor(int r, int g, int b, int first, int num)
	{
		int bestcolor = first;
		int bestdist = 257 * 257 + 257 * 257 + 257 * 257;

		for (int color = first; color < num; color++)
		{
			int x = r - palette[color * 3 + 0];
			int y = g - palette[color * 3 + 1];
			int z = b - palette[color * 3 + 2];
			int dist = x * x + y * y + z * z;
			if (dist < bestdist)
			{
				if (dist == 0)
					return color;

				bestdist = dist;
				bestcolor = color;
			}
		}
		return bestcolor;
	}

	// [SP] Re-implemented BestColor for more precision rather than speed. This function is only ever called once until the game palette is changed.

	int PTM_BestColor(int r, int g, int b, bool reverselookup, float powtable_val, int first, int num)
	{
		static double powtable[256];
		static bool firstTime = true;
		static float trackpowtable = 0.;

		double fbestdist = DBL_MAX, fdist;
		int bestcolor = 0;

		if (firstTime || trackpowtable != powtable_val)
		{
			auto pt = powtable_val;
			trackpowtable = pt;
			firstTime = false;
			for (int x = 0; x < 256; x++) powtable[x] = pow((double)x / 255, (double)pt);
		}

		for (int color = first; color < num; color++)
		{
			double x = powtable[abs(r - palette[color * 3 + 0])];
			double y = powtable[abs(g - palette[color * 3 + 1])];
			double z = powtable[abs(b - palette[color * 3 + 2])];
			fdist = x + y + z;
			if (color == first || (reverselookup ? (fdist <= fbestdist) : (fdist < fbestdist)))
			{
				if (fdist == 0 && !reverselookup)
					return color;

				fbestdist = fdist;
				bestcolor = color;
			}
		}
		return bestcolor;
	}
	
	void SetPalette(const PalEntry* colors)
	{
		// Find white and black from the original palette so that they can be
		// used to make an educated guess of the translucency % for a
		// translucency map.
		WhiteIndex = BestColor(255, 255, 255);
		BlackIndex = BestColor(0, 0, 0);

		// create the RGB666 lookup table
		for (int r = 0; r < 64; r++)
			for (int g = 0; g < 64; g++)
				for (int b = 0; b < 64; b++)
					RGB256k.RGB[r][g][b] = BestColor((r<<2)|(r>>4), (g<<2)|(g>>4), (b<<2)|(b>>4));
	}
	
	
}