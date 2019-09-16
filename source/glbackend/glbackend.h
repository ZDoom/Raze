#pragma once
#include <stdlib.h>
#include <algorithm>
#include <vector>
#include "gl_samplers.h"

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
	std::vector<BaseVertex> Buffer;	// cheap-ass implementation. The primary purpose is to get the GL accesses out of polymost.cpp, not writing something performant right away.
	
public:
	FSamplerManager *mSamplers;
	
	void Init();
	void Deinit();

	std::pair<size_t, BaseVertex *> AllocVertices(size_t num);
	void Draw(EDrawType type, size_t start, size_t count);
	
	void BindTexture(int texunit, int texid, int sampler = NoSampler);
	
};

extern GLInstance GLInterface;