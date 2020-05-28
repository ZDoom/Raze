#pragma once
class FBitmap;
class FTexture;

#include "tarray.h"
#include "hw_ihwtexture.h"

class FHardwareTexture : public IHardwareTexture
{
public:
	enum
	{
		Indexed,
		TrueColor,
		HighColor,		// 16 bit - Can be used to save space in memory constrained scenarios at the cost of visual accuracy.
		Brightmap,		// This can be stored as RGBA2 to save space, it also doesn't really need a mipmap.
	};

private:

	unsigned int glTexID = 0;
	unsigned int glDepthID = 0;	// only used by camera textures
	unsigned int glBufferID = 0;
	int internalType = TrueColor;
	bool mipmapped = true;
	int mWidth = 0, mHeight = 0;
	int colorId = 0;
	uint32_t allocated = 0;

	int GetDepthBuffer(int w, int h);

public:

	~FHardwareTexture();

	// Satisfy the virtual interface - currently this isn't being used.
	virtual void AllocateBuffer(int w, int h, int texelsize) {}
	virtual uint8_t* MapBuffer() { return nullptr; }
	virtual unsigned int CreateTexture(unsigned char* buffer, int w, int h, int texunit, bool mipmap, const char* name) { return 0; }

	//bool BindOrCreate(FTexture *tex, int texunit, int clampmode, int translation, int flags);

	unsigned int CreateTexture(int w, int h, bool type, bool mipmapped) = delete;
	unsigned int CreateTexture(int w, int h, int type, bool mipmapped);
	unsigned int LoadTexture(const unsigned char * buffer);
	unsigned int LoadTexturePart(const unsigned char* buffer, int x, int y, int w, int h);
	unsigned int LoadTexture(FBitmap &bmp);
	unsigned int GetTextureHandle();
	bool isIndexed() const { return internalType == Indexed; }
	void BindToFrameBuffer(int w, int h);

	friend class FGameTexture;
};


