#include "glbackend.h"
#include "glad/glad.h"
#include <vector>

GLInstance GLInterface;

static std::vector<BaseVertex> Buffer;	// cheap-ass implementation. The primary purpose is to get the GL accesses out of polymost.cpp, not writing something performant right away.
	
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
