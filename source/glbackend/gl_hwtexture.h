
#ifndef __GLTEXTURE_H
#define __GLTEXTURE_H


class FHardwareTexture //: public IHardwareTexture
{
public:

private:

	int mSampler = 0;
	unsigned int glTexID = 0;
	int glTextureBytes = 4;
	bool mipmapped = true;
	int mWidth = 0, mHeight = 0;

public:

	~FHardwareTexture();

	//bool BindOrCreate(FTexture *tex, int texunit, int clampmode, int translation, int flags);

	unsigned int CreateTexture(int w, int h, bool eightbit, bool mipmapped);
	unsigned int LoadTexture(unsigned char * buffer);
	unsigned int GetTextureHandle();
	int GetSampler() { return mSampler; }
	void SetSampler(int sampler) { mSampler = sampler;  }
};

#endif
