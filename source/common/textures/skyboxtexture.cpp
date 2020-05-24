/*
** skyboxtexture.cpp
**
**---------------------------------------------------------------------------
** Copyright 2004-2019 Christoph Oelckers
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
*/


#include "filesystem.h"
#include "textures.h"
#include "skyboxtexture.h"
#include "bitmap.h"
#include "texturemanager.h"



//-----------------------------------------------------------------------------
//
//
//
//-----------------------------------------------------------------------------

FSkyBox::FSkyBox(const char *name)
: FTexture(name)
{
	FTextureID texid = TexMan.CheckForTexture(name, ETextureType::Wall);
	previous = nullptr;
	if (texid.isValid())
	{
		previous = TexMan.GetTexture(texid);
		CopySize(previous);
	}
	faces[0]=faces[1]=faces[2]=faces[3]=faces[4]=faces[5] = nullptr;
	UseType = ETextureType::Override;
	bSkybox = true;
	fliptop = false;
}

//-----------------------------------------------------------------------------
//
//
//
//-----------------------------------------------------------------------------

TArray<uint8_t> FSkyBox::Get8BitPixels(bool alphatex)
{
	return previous->Get8BitPixels(alphatex);
}

//-----------------------------------------------------------------------------
//
//
//
//-----------------------------------------------------------------------------

FBitmap FSkyBox::GetBgraBitmap(const PalEntry *p, int *trans)
{
	return previous->GetBgraBitmap(p, trans);
}

//-----------------------------------------------------------------------------
//
//
//
//-----------------------------------------------------------------------------

FImageSource *FSkyBox::GetImage() const
{
	return previous->GetImage();
}
