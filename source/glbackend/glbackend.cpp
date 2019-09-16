#include "glbackend.h"
#include "glad/glad.h"
#include "gl_samplers.h"

GLInstance GLInterface;

void GLInstance::Init()
{
	mSamplers = new FSamplerManager;
}

void GLInstance::Deinit()
{
	if (mSamplers) delete mSamplers;
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


void GLInstance::BindTexture(int texunit, int tex, int sampler)
{
	glActiveTexture(GL_TEXTURE0 + texunit);
	glBindTexture(GL_TEXTURE_2D, tex);
	if (sampler != NoSampler) mSamplers->Bind(texunit, sampler, 0);
	else glBindSampler(texunit, 0);
	glActiveTexture(GL_TEXTURE0);
}