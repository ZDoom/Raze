#pragma once
#include <stdlib.h>
#include <algorithm>
#include <vector>
#include <map>
#include "gl_samplers.h"
#include "gl_hwtexture.h"

class FSamplerManager;

struct BaseVertex
{
	float x, y, z;
	float u, v;
	
	void SetVertex(float _x, float _y, float _z = 0)
	{
		x = _x;
		y = _y;
		z = _z;
	}
	
	void SetTexCoord(float _u = 0, float _v = 0)
	{
		u = _u;
		v = _v;
	}

	void Set(float _x, float _y, float _z = 0, float _u = 0, float _v = 0)
	{
		x = _x;
		y = _y;
		z = _z;
		u = _u;
		v = _v;
	}
};

enum EDrawType
{
	DT_TRIANGLES,
	DT_TRIANGLE_STRIP,
	DT_TRIANGLE_FAN,
	DT_QUADS,
	DT_LINES
};

class GLInstance
{
	enum
	{
		MAX_TEXTURES = 15,	// slot 15 is used internally and not available.
		THCACHESIZE = 200,
	};
	std::vector<BaseVertex> Buffer;	// cheap-ass implementation. The primary purpose is to get the GL accesses out of polymost.cpp, not writing something performant right away.
	unsigned int LastBoundTextures[MAX_TEXTURES];
	unsigned TextureHandleCache[THCACHESIZE];
	int currentindex = THCACHESIZE;
	
	
public:
	FSamplerManager *mSamplers;
	
	void Init();
	void Deinit();
	
	static int GetTexDimension(int value)
	{
		//if (value > gl.max_texturesize) return gl.max_texturesize;
		return value;
	}

	std::pair<size_t, BaseVertex *> AllocVertices(size_t num);
	void Draw(EDrawType type, size_t start, size_t count);
	
	int GetTextureID();
	void BindTexture(int texunit, int texid, int sampler = NoSampler);
	void UnbindTexture(int texunit);
	void UnbindAllTextures();
	
};

extern GLInstance GLInterface;