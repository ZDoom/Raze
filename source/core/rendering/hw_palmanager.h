#pragma once

#include "gl_hwtexture.h" 

struct palette_t;

class PaletteManager
{
	IHardwareTexture* palettetextures[256] = {};
	IHardwareTexture* lookuptextures[256] = {};

	unsigned FindPalswap(const uint8_t* paldata, palette_t& fadecolor);

public:
	~PaletteManager();
	void DeleteAll();
	IHardwareTexture *GetPalette(int index);
	IHardwareTexture* GetLookup(int index);
};


IHardwareTexture* setpalettelayer(int layer, int translation);
void ClearPalManager();

