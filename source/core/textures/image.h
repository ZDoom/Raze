#pragma once

#include <stdint.h>
#include "zstring.h"
#include "tarray.h"
#include "textures.h"
#include "bitmap.h"
#include "memarena.h"

class FImageSource;
using PrecacheInfo = TMap<int, std::pair<int, int>>;

struct PalettedPixels
{
	friend class FImageSource;
	TArrayView<uint8_t> Pixels;
private:
	TArray<uint8_t> PixelStore;

	bool ownsPixels() const
	{
		return Pixels.Data() == PixelStore.Data();
	}
};

// This represents a naked image. It has no high level logic attached to it.
// All it can do is provide raw image data to its users.
class FImageSource
{
protected:

	static int NextID;

	int SourceLump;
	int Width = 0, Height = 0;
	int LeftOffset = 0, TopOffset = 0;			// Offsets stored in the image.
	bool bUseGamePalette = false;				// true if this is an image without its own color set.
	int ImageID = -1;
	FString Name;

	// Internal image creation functions. All external access should go through the cache interface,
	// so that all code can benefit from future improvements to that.

public:

	virtual ~FImageSource() = default;
	void CopySize(FImageSource &other)
	{
		Width = other.Width;
		Height = other.Height;
		LeftOffset = other.LeftOffset;
		TopOffset = other.TopOffset;
		SourceLump = other.SourceLump;
	}

	bool bMasked = true;						// Image (might) have holes (Assume true unless proven otherwise!)
	int8_t bTranslucent = -1;					// Image has pixels with a non-0/1 value. (-1 means the user needs to do a real check)

	int GetId() const { return ImageID; }
	
	// 'noremap0' will only be looked at by FPatchTexture and forwarded by FMultipatchTexture.
	static FImageSource * GetImage(const char *name);

	virtual void CreatePalettedPixels(uint8_t *destbuffer) = 0;
	virtual int CopyPixels(FBitmap* bmp, int conversion) = 0;			// This will always ignore 'luminance'.


	// Conversion option
	enum EType
	{
		normal = 0,
		luminance = 1,
		noremap0 = 2
	};
	
	FImageSource(int sourcelump = -1) : SourceLump(sourcelump) { ImageID = ++NextID; }
	
	int GetWidth() const
	{
		return Width;
	}
	
	int GetHeight() const
	{
		return Height;
	}
	
	std::pair<int, int> GetSize() const
	{
		return std::make_pair(Width, Height);
	}
	
	std::pair<int, int> GetOffsets() const
	{
		return std::make_pair(LeftOffset, TopOffset);
	}

	void SetOffsets(int x, int y)
	{
		LeftOffset = x;
		TopOffset = y;
	}
	
	int LumpNum() const
	{
		return SourceLump;
	}
	
	bool UseGamePalette() const
	{
		return bUseGamePalette;
	}
};

//==========================================================================
//
// a TGA texture
//
//==========================================================================

class FImageTexture : public FTexture
{
	FImageSource *mImage;
public:
	FImageTexture (FImageSource *image, const char *name = nullptr);
	~FImageTexture()
	{
		if (mImage) delete mImage;
	}
	void Create8BitPixels(uint8_t* buffer) override;

	FImageSource *GetImage() const override { return mImage; }
	FBitmap GetBgraBitmap(const PalEntry *p, int *trans) override;

};

