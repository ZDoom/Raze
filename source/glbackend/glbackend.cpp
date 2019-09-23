#include "glbackend.h"
#include "glad/glad.h"
#include "gl_samplers.h"


GLInstance GLInterface;

void GLInstance::Init()
{
	if (!mSamplers)
	{
		mSamplers = new FSamplerManager;
		memset(LastBoundTextures, 0, sizeof(LastBoundTextures));
	}
}

void GLInstance::Deinit()
{
	if (mSamplers) delete mSamplers;
	mSamplers = nullptr;
}
	
std::pair<size_t, BaseVertex *> GLInstance::AllocVertices(size_t num)
{
	Buffer.resize(num);
	return std::make_pair((size_t)0, Buffer.data());
}

static GLint primtypes[] = 
{
	GL_TRIANGLES,
	GL_TRIANGLE_STRIP,
	GL_TRIANGLE_FAN,
	GL_QUADS,
	GL_LINES
};
	
void GLInstance::Draw(EDrawType type, size_t start, size_t count)
{
	glBegin(primtypes[type]);
	auto p = &Buffer[start];
	for (size_t i = 0; i < count; i++, p++)
	{
		glTexCoord2f(p->u, p->v);
		glVertex3f(p->x, p->y, p->z);
	}
	glEnd();
}

int GLInstance::GetTextureID()
{
	// Generating large numbers of texture IDs piece by piece does not work well on modern NVidia drivers.

	if (currentindex == THCACHESIZE)
	{
		currentindex = 0;
		glGenTextures(THCACHESIZE, TextureHandleCache);
	}
	else currentindex++;
	return TextureHandleCache[currentindex];
}

FHardwareTexture* GLInstance::NewTexture()
{
	return new FHardwareTexture;
}

void GLInstance::BindTexture(int texunit, FHardwareTexture *tex, int sampler)
{
	if (!tex) return;
	if (texunit != 0) glActiveTexture(GL_TEXTURE0 + texunit);
	glBindTexture(GL_TEXTURE_2D, tex->GetTextureHandle());
	mSamplers->Bind(texunit, sampler == NoSampler? tex->GetSampler() : sampler, 0);
	if (texunit != 0) glActiveTexture(GL_TEXTURE0);
	LastBoundTextures[texunit] = tex->GetTextureHandle();
}

void GLInstance::UnbindTexture(int texunit)
{
	if (LastBoundTextures[texunit] != 0)
	{
		if (texunit != 0) glActiveTexture(GL_TEXTURE0+texunit);
		glBindTexture(GL_TEXTURE_2D, 0);
		if (texunit != 0) glActiveTexture(GL_TEXTURE0);
		LastBoundTextures[texunit] = 0;
	}
}

void GLInstance::UnbindAllTextures()
{
	for(int texunit = 0; texunit < MAX_TEXTURES; texunit++)
	{
		UnbindTexture(texunit);
	}
}
