
#ifndef __GLTEXTURE_H
#define __GLTEXTURE_H


class FHardwareTexture //: public IHardwareTexture
{
public:

private:

	unsigned int glTexID = 0;
	int glTextureBytes = 4;
	bool mipmapped = true;
	int mWidth = 0, mHeight = 0;

public:

	~FHardwareTexture();

	unsigned int Bind(int texunit, bool needmipmap);
	//bool BindOrCreate(FTexture *tex, int texunit, int clampmode, int translation, int flags);

	unsigned int CreateTexture(int w, int h, bool eightbit, bool mipmapped);
	unsigned int LoadTexture(unsigned char * buffer);
	unsigned int GetTextureHandle();
};

#endif
