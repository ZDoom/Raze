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

void GLInstance::InitGLState(int fogmode, int multisample)
{
	glShadeModel(GL_SMOOTH);  // GL_FLAT
	glClearColor(0, 0, 0, 1.0);  // Black Background
	glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);
	glDisable(GL_DITHER);
	glEnable(GL_TEXTURE_2D);
    glHint(GL_FOG_HINT, GL_NICEST);
    glFogi(GL_FOG_MODE, (fogmode < 2) ? GL_EXP2 : GL_LINEAR);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glPixelStorei(GL_PACK_ALIGNMENT, 1);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

	glEnable(GL_DEPTH_CLAMP);

    if (multisample > 0 )
    {
		glHint(GL_MULTISAMPLE_FILTER_HINT_NV, GL_NICEST);
        glEnable(GL_MULTISAMPLE);
    }
	glGetIntegerv(GL_MAX_TEXTURE_SIZE, &maxTextureSize);
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

void GLInstance::EnableBlend(bool on)
{
	if (on) glEnable (GL_BLEND);
	else glDisable (GL_BLEND);
}

void GLInstance::EnableAlphaTest(bool on)
{
	if (on) glEnable (GL_ALPHA_TEST);
	else glDisable (GL_ALPHA_TEST);
}

void GLInstance::EnableDepthTest(bool on)
{
	if (on) glEnable (GL_DEPTH_TEST);
	else glDisable (GL_DEPTH_TEST);
}

void GLInstance::SetMatrix(int num, const VSMatrix *mat)
{
	matrices[num] = *mat;
	switch(num)
	{
		case Matrix_Projection:
			glMatrixMode(GL_PROJECTION);
			break;
			
		case Matrix_ModelView:
			glMatrixMode(GL_MODELVIEW);
			break;
			
		default:
			glActiveTexture(GL_TEXTURE0 + num - Matrix_Texture0);
			glMatrixMode(GL_TEXTURE);
			break;
	}
	glLoadMatrixf(mat->get());
	glMatrixMode(GL_MODELVIEW);
	if (num > Matrix_Texture0) glActiveTexture(GL_TEXTURE0);
}

void GLInstance::EnableStencilWrite(int value)
{
    glEnable(GL_STENCIL_TEST);
    glClear(GL_STENCIL_BUFFER_BIT);
    glStencilOp(GL_REPLACE, GL_REPLACE, GL_REPLACE);
    glStencilFunc(GL_ALWAYS, value, 0xFF);
}

void GLInstance::EnableStencilTest(int value)
{
    glEnable(GL_STENCIL_TEST);
    glStencilFunc(GL_EQUAL, value, 0xFF);
    glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);
}

void GLInstance::DisableStencil()
{
	glDisable(GL_STENCIL_TEST);
}

void GLInstance::SetCull(int type)
{
	if (type == Cull_None)
	{
		glDisable(GL_CULL_FACE);
	}
	else if (type == Cull_Front)
	{
		glEnable(GL_CULL_FACE);
		glCullFace(GL_FRONT);
	}
	else if (type == Cull_Back)
	{
		glEnable(GL_CULL_FACE);
		glCullFace(GL_BACK);
	}
}

void GLInstance::SetColor(float r, float g, float b, float a)
{
	glColor4f(r, g, b, a);
}