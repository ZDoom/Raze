#pragma once

#include "textures.h"
//-----------------------------------------------------------------------------
//
// This is not a real texture but will be added to the texture manager
// so that it can be handled like any other sky.
//
//-----------------------------------------------------------------------------

class FSkyBox : public FTexture
{
public:

	FTexture* faces[6] = {};
	bool fliptop;

	FSkyBox(const char *name);
	FBitmap GetBgraBitmap(const PalEntry *, int *trans) override;
	FImageSource *GetImage() const override;


	void SetSize()
	{
		CopySize(faces[0]);
	}

	bool Is3Face() const
	{
		return faces[5] == nullptr;
	}

	bool IsFlipped() const
	{
		return fliptop;
	}
};
