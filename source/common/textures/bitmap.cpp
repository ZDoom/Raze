/*
** bitmap.cpp
**
**---------------------------------------------------------------------------
** Copyright 2008-2019 Christoph Oelckers
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

#include <assert.h>
#include "bitmap.h"


//===========================================================================
// 
// multi-format pixel copy with colormap application
// requires the previously defined conversion classes to work
//
//===========================================================================
template<class TSrc, class TDest, class TBlend>
void iCopyColors(uint8_t *pout, const uint8_t *pin, int count, int step,
	uint8_t tr, uint8_t tg, uint8_t tb)
{
	int i;
	int a;

	for(i=0;i<count;i++)
	{
		a = TSrc::A(pin, tr, tg, tb);
		if (TBlend::ProcessAlpha0() || a)
		{
			TBlend::OpC(pout[TDest::RED], TSrc::R(pin), a);
			TBlend::OpC(pout[TDest::GREEN], TSrc::G(pin), a);
			TBlend::OpC(pout[TDest::BLUE], TSrc::B(pin), a);
			TBlend::OpA(pout[TDest::ALPHA], a);
		}
		pout+=4;
		pin+=step;
	}
}

typedef void (*CopyFunc)(uint8_t *pout, const uint8_t *pin, int count, int step, uint8_t r, uint8_t g, uint8_t b);

static const CopyFunc copyfuncs[]=
{
	iCopyColors<cRGB, cBGRA, bCopy>,
	iCopyColors<cRGBT, cBGRA, bCopy>,
	iCopyColors<cRGBA, cBGRA, bCopy>,
	iCopyColors<cIA, cBGRA, bCopy>,
	iCopyColors<cCMYK, cBGRA, bCopy>,
	iCopyColors<cYCbCr, cBGRA, bCopy>,
	iCopyColors<cBGR, cBGRA, bCopy>,
	iCopyColors<cBGRA, cBGRA, bCopy>,
	iCopyColors<cI16, cBGRA, bCopy>,
	iCopyColors<cRGB555, cBGRA, bCopy>,
	iCopyColors<cPalEntry, cBGRA, bCopy>
};

//===========================================================================
//
// True Color texture copy function
//
//===========================================================================
void FBitmap::CopyPixelDataRGB(int originx, int originy, const uint8_t *patch, int srcwidth, 
							   int srcheight, int step_x, int step_y, int rotate, int ct,
							   int r, int g, int b)
{
	uint8_t *buffer = data + 4 * originx + Pitch * originy;
	for (int y=0;y<srcheight;y++)
	{
		copyfuncs[ct](&buffer[y*Pitch], &patch[y*step_y], srcwidth, step_x, r, g, b);
	}
}


template<class TDest, class TBlend> 
void iCopyPaletted(uint8_t *buffer, const uint8_t * patch, int srcwidth, int srcheight, int Pitch,
					int step_x, int step_y, int rotate, const PalEntry * palette)
{
	int x,y,pos;

	for (y=0;y<srcheight;y++)
	{
		pos = y*Pitch;
		for (x=0;x<srcwidth;x++,pos+=4)
		{
			int v=(unsigned char)patch[y*step_y+x*step_x];
			int a = palette[v].a;

			if (TBlend::ProcessAlpha0() || a)
			{
				TBlend::OpC(buffer[pos + TDest::RED], palette[v].r, a);
				TBlend::OpC(buffer[pos + TDest::GREEN], palette[v].g, a);
				TBlend::OpC(buffer[pos + TDest::BLUE], palette[v].b, a);
				TBlend::OpA(buffer[pos + TDest::ALPHA], a);
			}
		}
	}
}

typedef void (*CopyPalettedFunc)(uint8_t *buffer, const uint8_t * patch, int srcwidth, int srcheight, int Pitch,
					int step_x, int step_y, int rotate, PalEntry * palette);


//===========================================================================
//
// Paletted to True Color texture copy function
//
//===========================================================================
void FBitmap::CopyPixelData(int originx, int originy, const uint8_t * patch, int srcwidth, int srcheight, 
										int step_x, int step_y, int rotate, const PalEntry * palette)
{
	uint8_t *buffer = data + 4*originx + Pitch*originy;
	iCopyPaletted<cBGRA, bCopy>(buffer, patch, srcwidth, srcheight, Pitch,
													step_x, step_y, rotate, palette);
}

