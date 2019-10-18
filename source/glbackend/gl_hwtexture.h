
#ifndef __GLTEXTURE_H
#define __GLTEXTURE_H

class FBitmap;
class FTexture;

#include "tarray.h"

class FHardwareTexture //: public IHardwareTexture
{
public:

private:

	int mSampler = 0;
	unsigned int glTexID = 0;
	int glTextureBytes = 4;
	bool mipmapped = true;
	int mWidth = 0, mHeight = 0;
	int colorId = 0;

public:

	~FHardwareTexture();

	//bool BindOrCreate(FTexture *tex, int texunit, int clampmode, int translation, int flags);

	unsigned int CreateTexture(int w, int h, bool eightbit, bool mipmapped);
	unsigned int LoadTexture(const unsigned char * buffer, bool brga = false);
	unsigned int LoadTexturePart(const unsigned char* buffer, int x, int y, int w, int h, bool brga = false);
	unsigned int LoadTexture(FBitmap &bmp, bool brga = false);
	unsigned int GetTextureHandle();
	int GetSampler() { return mSampler; }
	void SetSampler(int sampler) { mSampler = sampler;  }
	bool isIndexed() const { return glTextureBytes == 1; }

	friend class FGameTexture;
};

// This class identifies a single source image to the game.
// Since hightile palette variations are identified by file name, they will create separate game textures.
class FGameTexture
{
	int Width, Height;
	bool isHightile;

	// Source image for this texture.
	FTexture* sourceData = nullptr;
	// indexed or the sole image for hightiles. 
	FHardwareTexture* hwBase = nullptr;	
	// If the number was large a TMap would be better - 
	// but in most cases the maximum number of palettes for a single tile is less than 10 where a linear search is much faster than a TMap.
	TArray<FHardwareTexture*> hwTextures;	
public:
	FGameTexture(bool hightile, int width, int height);
	virtual ~FGameTexture();

	// For dynamic subtypes.
	virtual void SizeChanged(int width, int height);
	static constexpr int MakeId(int palette, int palswap)
	{
		return palette + 256 * palswap;
	}

	FHardwareTexture* GetBaseTexture();
	FHardwareTexture* CreateHardwareTexture(int palette, int palswap);

	FHardwareTexture* GetHardwareTexture(int palette, int palswap)
	{
		if (isHightile) return GetBaseTexture();
		auto id = MakeId(palette, palswap);
		auto found = hwTextures.FindEx([=](FHardwareTexture* tex)
			{
				return tex->colorId == id;
			});
		if (!found) return CreateHardwareTexture(palette, palswap);
	}
};

#endif
