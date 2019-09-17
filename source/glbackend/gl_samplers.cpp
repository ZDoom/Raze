// 
//---------------------------------------------------------------------------
//
// Copyright(C) 2014-2016 Christoph Oelckers
// All rights reserved.
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public License
// along with this program.  If not, see http://www.gnu.org/licenses/
//
//--------------------------------------------------------------------------
//

#include "glad/glad.h"
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
	//{GL_LINEAR_MIPMAP_LINEAR,		GL_NEAREST,		true},
};
 

FSamplerManager::FSamplerManager()
{
	glGenSamplers(NumSamplers, mSamplers);
	SetTextureFilterMode(0);
	glSamplerParameteri(mSamplers[5], GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glSamplerParameteri(mSamplers[5], GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glSamplerParameterf(mSamplers[5], GL_TEXTURE_MAX_ANISOTROPY_EXT, 1.f);
	glSamplerParameterf(mSamplers[4], GL_TEXTURE_MAX_ANISOTROPY_EXT, 1.f);
	glSamplerParameterf(mSamplers[6], GL_TEXTURE_MAX_ANISOTROPY_EXT, 1.f);
	glSamplerParameteri(mSamplers[6], GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glSamplerParameteri(mSamplers[6], GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glSamplerParameterf(mSamplers[7], GL_TEXTURE_MAX_ANISOTROPY_EXT, 1.f);
	glSamplerParameteri(mSamplers[7], GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glSamplerParameteri(mSamplers[7], GL_TEXTURE_MAG_FILTER, GL_NEAREST);

	glSamplerParameteri(mSamplers[1], GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glSamplerParameteri(mSamplers[2], GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glSamplerParameteri(mSamplers[3], GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glSamplerParameteri(mSamplers[3], GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glSamplerParameteri(mSamplers[4], GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glSamplerParameteri(mSamplers[4], GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glSamplerParameteri(mSamplers[6], GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glSamplerParameteri(mSamplers[6], GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glSamplerParameteri(mSamplers[7], GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glSamplerParameteri(mSamplers[7], GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
}

FSamplerManager::~FSamplerManager()
{
	UnbindAll();
	glDeleteSamplers(NumSamplers, mSamplers);
}

void FSamplerManager::UnbindAll()
{
	for (int i = 0; i < 8 /* fixme */; i++)
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

	for (int i = 0; i < 4; i++)
	{
		glSamplerParameteri(mSamplers[i], GL_TEXTURE_MIN_FILTER, TexFilter[filter].minfilter);
		glSamplerParameteri(mSamplers[i], GL_TEXTURE_MAG_FILTER, TexFilter[filter].magfilter);
		glSamplerParameterf(mSamplers[i], GL_TEXTURE_MAX_ANISOTROPY_EXT, anisotropy);
	}
	glSamplerParameteri(mSamplers[4], GL_TEXTURE_MIN_FILTER, TexFilter[filter].magfilter);
	glSamplerParameteri(mSamplers[4], GL_TEXTURE_MAG_FILTER, TexFilter[filter].magfilter);
}
