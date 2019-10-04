#pragma once
#include <stdlib.h>
#include <algorithm>
#include <vector>
#include <map>
#include "gl_samplers.h"
#include "gl_hwtexture.h"
#include "matrix.h"

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

enum EMatrixType
{
	Matrix_Projection,
	Matrix_ModelView,
	Matrix_Texture0,
	Matrix_Texture1,
	Matrix_Texture2,
	Matrix_Texture3,
	Matrix_Texture4,
	Matrix_Texture5,
	Matrix_Texture6,
	Matrix_Texture7,
	NUMMATRICES
};

enum ECull
{
	Cull_None,
	Cull_Front,
	Cull_Back
};

enum EDepthFunc
{
	Depth_Always,
	Depth_Less,
	Depth_Equal,
	Depth_LessEqual
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
	int maxTextureSize;
	
	VSMatrix matrices[NUMMATRICES];
	
	
public:
	FSamplerManager *mSamplers;
	
	void Init();
	void InitGLState(int fogmode, int multisample);

	void Deinit();
	
	static int GetTexDimension(int value)
	{
		//if (value > gl.max_texturesize) return gl.max_texturesize;
		return value;
	}

	std::pair<size_t, BaseVertex *> AllocVertices(size_t num);
	void Draw(EDrawType type, size_t start, size_t count);
	
	int GetTextureID();
	FHardwareTexture* NewTexture();
	void BindTexture(int texunit, FHardwareTexture *texid, int sampler = NoSampler);
	void UnbindTexture(int texunit);
	void UnbindAllTextures();
	void EnableBlend(bool on);
	void EnableAlphaTest(bool on);
	void EnableDepthTest(bool on);
	const VSMatrix &GetMatrix(int num)
	{
		return matrices[num];
	}
	void SetMatrix(int num, const VSMatrix *mat );
	void SetMatrix(int num, const float *mat)
	{
		SetMatrix(num, reinterpret_cast<const VSMatrix*>(mat));
	}
	void SetCull(int type);

	void EnableStencilWrite(int value);
	void EnableStencilTest(int value);
	void DisableStencil();
	void SetColor(float r, float g, float b, float a = 1.f);
	void SetColorub(uint8_t r, uint8_t g, uint8_t b, uint8_t a = 255)
	{
		SetColor(r * (1 / 255.f), g * (1 / 255.f), b * (1 / 255.f), a * (1 / 255.f));
	}

	void SetDepthFunc(int func);
	void SetFogLinear(float* color, float start, float end);
	void SetFogExp2(float* color, float coefficient);
};

extern GLInstance GLInterface;
