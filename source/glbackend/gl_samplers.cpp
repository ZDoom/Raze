/*
** gl_samplers.cpp
**
** Texture sampler handling
**
**---------------------------------------------------------------------------
** Copyright 2015-2019 Christoph Oelckers
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

#include "gl_load.h"
#include "glbackend.h"

struct TexFilter_s
{
	int minfilter;
	int magfilter;
	bool mipmapping;
} ;

TexFilter_s TexFilter[]={
	{GL_NEAREST,					GL_NEAREST,		false},
	{GL_LINEAR,						GL_LINEAR,		false},
	{GL_NEAREST_MIPMAP_NEAREST,		GL_NEAREST,		true},
	{GL_LINEAR_MIPMAP_NEAREST,		GL_LINEAR,		true},
	{GL_NEAREST_MIPMAP_LINEAR,		GL_NEAREST,		true},
	{GL_LINEAR_MIPMAP_LINEAR,		GL_LINEAR,		true},
	{GL_LINEAR_MIPMAP_LINEAR,		GL_NEAREST,		true},
};
 

FSamplerManager::FSamplerManager()
{
	glGenSamplers(NumSamplers, mSamplers);

	for (int i = SamplerNoFilterRepeat; i <= SamplerNoFilterClampXY; i++)
	{
		glSamplerParameteri(mSamplers[i], GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glSamplerParameteri(mSamplers[i], GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glSamplerParameterf(mSamplers[i], GL_TEXTURE_MAX_ANISOTROPY_EXT, 1.f);
	}

	glSamplerParameteri(mSamplers[SamplerClampX], GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glSamplerParameteri(mSamplers[SamplerClampY], GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glSamplerParameteri(mSamplers[SamplerClampXY], GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glSamplerParameteri(mSamplers[SamplerClampXY], GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	glSamplerParameteri(mSamplers[SamplerNoFilterClampX], GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glSamplerParameteri(mSamplers[SamplerNoFilterClampY], GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glSamplerParameteri(mSamplers[SamplerNoFilterClampXY], GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glSamplerParameteri(mSamplers[SamplerNoFilterClampXY], GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	glSamplerParameteri(mSamplers[Sampler2DFiltered], GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glSamplerParameteri(mSamplers[Sampler2DFiltered], GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

}

FSamplerManager::~FSamplerManager()
{
	UnbindAll();
	glDeleteSamplers(NumSamplers, mSamplers);
}

void FSamplerManager::UnbindAll()
{
	for (int i = 0; i < 16 /* fixme */; i++)
	{
		glBindSampler(i, 0);
	}
}
	
uint8_t FSamplerManager::Bind(int texunit, int num, int lastval)
{
	unsigned int samp = mSamplers[num];
	glBindSampler(texunit, samp);
	return 255;
}

	
void FSamplerManager::SetTextureFilterMode(int filter, int anisotropy)
{
	UnbindAll();

	for (int i = SamplerRepeat; i <= SamplerClampXY; i++)
	{
		glSamplerParameteri(mSamplers[i], GL_TEXTURE_MIN_FILTER, TexFilter[filter].minfilter);
		glSamplerParameteri(mSamplers[i], GL_TEXTURE_MAG_FILTER, TexFilter[filter].magfilter);
		glSamplerParameterf(mSamplers[i], GL_TEXTURE_MAX_ANISOTROPY_EXT, anisotropy);
	}
	glSamplerParameteri(mSamplers[Sampler2DFiltered], GL_TEXTURE_MIN_FILTER, TexFilter[filter].magfilter);
	glSamplerParameteri(mSamplers[Sampler2DFiltered], GL_TEXTURE_MAG_FILTER, TexFilter[filter].magfilter);
}
