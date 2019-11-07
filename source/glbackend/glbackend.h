#pragma once
#include <stdlib.h>
#include <algorithm>
#include <vector>
#include <map>
#include "gl_samplers.h"
#include "gl_hwtexture.h"
#include "gl_renderstate.h"
#include "matrix.h"
#include "palentry.h"
#include "renderstyle.h"

class FSamplerManager;
class FShader;
class PolymostShader;
class SurfaceShader;
class FTexture;
class GLInstance;
class F2DDrawer;

struct PaletteData
{
	int32_t crc32;
	PalEntry colors[256];
	bool shadesdone;
	int whiteindex, blackindex;
	FHardwareTexture* paltexture;
};

struct PalShade
{
	int palindex;
	float mulshade, addshade;
};

struct PalswapData
{
	int32_t crc32;
	const uint8_t *lookup;	// points to the original data. This is static so no need to copy
	FHardwareTexture* swaptexture;
	PalEntry fadeColor;
};

enum
{
	PALSWAP_TEXTURE_SIZE = 2048
};

class PaletteManager
{
	// The current engine limit is 256 palettes and 256 palswaps.
	uint32_t palettemap[256] = {};
	uint32_t palswapmap[256] = {};
	float addshade[256] = {};
	float mulshade[256] = {};
	uint32_t lastindex = ~0u;
	uint32_t lastsindex = ~0u;
	int numshades = 1;

	// All data is being stored in contiguous blocks that can be used as uniform buffers as-is.
	TArray<PaletteData> palettes;
	TArray<PalswapData> palswaps;
	TMap<int, int> swappedpalmap;
	FHardwareTexture* palswapTexture = nullptr;
	GLInstance* const inst;

	//OpenGLRenderer::GLDataBuffer* palswapBuffer = nullptr;

	unsigned FindPalswap(const uint8_t* paldata);

public:
	PaletteManager(GLInstance *inst_) : inst(inst_)
	{}
	~PaletteManager();
	void DeleteAll();
	void DeleteAllTextures();
	void SetPalette(int index, const uint8_t *data);
	void SetPalswapData(int index, const uint8_t* data, int numshades);

	void BindPalette(int index);
	void BindPalswap(int index);
	int ActivePalswap() const { return lastsindex; }
	int LookupPalette(int palette, int palswap, bool brightmap);
	const PalEntry *GetPaletteData(int palid) const { return palettes[palid].colors; }
	unsigned FindPalette(const uint8_t* paldata);

};


struct glinfo_t {
	const char* vendor;
	const char* renderer;
	const char* version;
	const char* extensions;

	float maxanisotropy;
	char bufferstorage;
	char dumped;
};

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
	Matrix_View,
	Matrix_Projection,
	Matrix_ModelView,
	Matrix_Detail,
	Matrix_Texture,
	// These are the only ones being used.
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


enum EWinding
{
	Winding_CCW,
	Winding_CW
};

enum ETexType
{
	TT_INDEXED,
	TT_TRUECOLOR,
	TT_HICREPLACE,
	TT_BRIGHTMAP
};

struct ImDrawData;

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
	PaletteManager palmanager;
	int lastPalswapIndex = -1;
	FHardwareTexture* texv;
	FTexture* currentTexture = nullptr;
	int TextureType;
	int MatrixChange = 0;

	IVertexBuffer* LastVertexBuffer = nullptr;
	int LastVB_Offset[2] = {};
	IIndexBuffer* LastIndexBuffer = nullptr;


	VSMatrix matrices[NUMMATRICES];
	PolymostRenderState renderState;
	FShader* activeShader;
	PolymostShader* polymostShader;
	SurfaceShader* surfaceShader;
	FShader* vpxShader;
	
	
public:
	glinfo_t glinfo;
	FSamplerManager *mSamplers;
	
	void Init(int y);
	void InitGLState(int fogmode, int multisample);
	void LoadPolymostShader();
	void LoadSurfaceShader();
	void LoadVPXShader();
	void Draw2D(F2DDrawer* drawer);
	void DrawImGui(ImDrawData*);

	void Deinit();
	
	static int GetTexDimension(int value)
	{
		//if (value > gl.max_texturesize) return gl.max_texturesize;
		return value;
	}

	GLInstance();
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
	void EnableMultisampling(bool on);
	void SetVertexBuffer(IVertexBuffer* vb, int offset1, int offset2)
	{
		renderState.VertexBuffer = vb;
		renderState.VB_Offset[0] = offset1;
		renderState.VB_Offset[1] = offset2;
	}
	void SetIndexBuffer(IIndexBuffer* vb)
	{
		renderState.IndexBuffer = vb;
	}
	const VSMatrix &GetMatrix(int num)
	{
		return matrices[num];
	}
	void SetMatrix(int num, const VSMatrix *mat );
	void SetMatrix(int num, const float *mat)
	{
		SetMatrix(num, reinterpret_cast<const VSMatrix*>(mat));
	}
	void SetCull(int type, int winding = Winding_CCW);

	void EnableStencilWrite(int value);
	void EnableStencilTest(int value);
	void DisableStencil();
	void SetColor(float r, float g, float b, float a = 1.f);
	void SetColorub(uint8_t r, uint8_t g, uint8_t b, uint8_t a = 255)
	{
		SetColor(r * (1 / 255.f), g * (1 / 255.f), b * (1 / 255.f), a * (1 / 255.f));
	}

	void SetDepthFunc(int func);
	void SetColorMask(bool on);
	void SetDepthMask(bool on);
	void SetBlendFunc(int src, int dst);
	void SetBlendOp(int op);
	void ClearScreen(float r, float g, float b, bool depth);
	void ClearDepth();
	void SetViewport(int x, int y, int w, int h);
	void SetAlphaThreshold(float al);
	void SetWireframe(bool on);
	void SetPolymostShader();
	void SetSurfaceShader();
	void SetVPXShader();
	void SetPalette(int palette);
	bool ApplyTextureProps(FTexture *tex, int pal);
	void RestoreTextureProps();

	void ReadPixels(int w, int h, uint8_t* buffer);

	void SetPaletteData(int index, const uint8_t* data)
	{
		palmanager.SetPalette(index, data);
	}

	void SetPalswapData(int index, const uint8_t* data, int numshades)
	{
		palmanager.SetPalswapData(index, data, numshades);
	}

	void SetPalswap(int index);

	int GetClamp()
	{
		return 0;// int(renderState.Clamp[0] + 2 * renderState.Clamp[1]);
	}

	void SetClamp(int clamp)
	{
		// This option is totally pointless and should be removed.
		//renderState.Clamp[0] = clamp & 1;
		//renderState.Clamp[1] = !!(clamp & 2);
	}

	void SetShade(int32_t shade, int numshades)
	{
		renderState.Shade = shade;
		renderState.NumShades = numshades;
	}

	void SetShadeDiv(int value)
	{
		renderState.ShadeDiv = 1.f / value;	// There's 3 values here: Blood uses 62 with numShades = 64, Ion Fury uses 30 with NumShades = 32, the other games use 26 with NumShades = 32.
	}

	void SetVisibility(float visibility, float fviewingrange)
	{
		renderState.VisFactor = visibility* fviewingrange* (1.f / (64.f * 65536.f));
	}

	void UseColorOnly(bool yes)
	{
		if (yes) renderState.Flags |= RF_ColorOnly;
		else renderState.Flags &= ~RF_ColorOnly;
	}

	void UseDetailMapping(bool yes)
	{
		if (yes) renderState.Flags |= RF_DetailMapping;
		else renderState.Flags &= ~RF_DetailMapping;
	}

	void UseGlowMapping(bool yes)
	{
		if (yes) renderState.Flags |= RF_GlowMapping;
		else renderState.Flags &= ~RF_GlowMapping;
	}

	void UseBrightmaps(bool yes)
	{
		if (yes) renderState.Flags |= RF_Brightmapping;
		else renderState.Flags &= ~RF_Brightmapping;
	}

	void SetNpotEmulation(bool yes, float factor, float xOffset)
	{
		if (yes)
		{
			renderState.Flags |= RF_NPOTEmulation;
			renderState.NPOTEmulationFactor = factor;
			renderState.NPOTEmulationXOffset = xOffset;
		}
		else renderState.Flags &= ~RF_NPOTEmulation;
	}

	void SetShadeInterpolate(int32_t yes)
	{
		if (yes) renderState.Flags |= RF_ShadeInterpolate;
		else renderState.Flags &= ~RF_ShadeInterpolate;
	}

	void SetFadeColor(PalEntry color)
	{
		renderState.FogColor = color;
	};

	void SetFadeDisable(bool yes)
	{
		if (yes) renderState.Flags |= RF_FogDisabled;
		else renderState.Flags &= ~RF_FogDisabled;
	}

	void SetBrightness(int brightness) 
	{
		renderState.Brightness = 8.f / (brightness + 8.f);
	}

	void SetTinting(int flags, PalEntry color, PalEntry modulateColor)
	{
		// not yet implemented.
	}

	void SetBasepalTint(PalEntry color)
	{
		// not yet implemented - only relevant for hires replacements.
	}

	int GetPaletteIndex(PalEntry* palette)
	{
		return palmanager.FindPalette((uint8_t*)palette);
	}
	
	FHardwareTexture* CreateIndexedTexture(FTexture* tex);
	FHardwareTexture* CreateTrueColorTexture(FTexture* tex, int palid, bool checkfulltransparency = false, bool rgb8bit = false);
	FHardwareTexture *LoadTexture(FTexture* tex, int texturetype, int palid);
	bool SetTextureInternal(int globalpicnum, FTexture* tex, int palette, int method, int sampleroverride, float xpanning, float ypanning, FTexture *det, float detscale, FTexture *glow);

	bool SetNamedTexture(FTexture* tex, int palette, int sampleroverride);

	bool SetTexture(int globalpicnum, FTexture* tex, int palette, int method, int sampleroverride)
	{
		return SetTextureInternal(globalpicnum, tex, palette, method, sampleroverride, 0, 0, nullptr, 1, nullptr);
	}

	bool SetModelTexture(FTexture *tex, int palette, float xpanning, float ypanning, FTexture *det, float detscale, FTexture *glow)
	{
		return SetTextureInternal(-1, tex, palette, 8/*DAMETH_MODEL*/, -1, xpanning, ypanning, det, detscale, glow);
	}
};

extern GLInstance GLInterface;
