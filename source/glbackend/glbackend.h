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

class FSamplerManager;
class FShader;
class PolymostShader;
class SurfaceShader;
class FTexture;
class GLInstance;

struct PaletteData
{
	int32_t crc32;
	PalEntry colors[256];
	float shades[512];	// two values (addshade and mulshade for each palswap.)
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

	// Keep the short lived movie palettes out of the palette list for ease of maintenance.
	// Since it is transient this doesn't need to be preserved if it changes, unlike the other palettes which need to be preserved as references for the texture management.
	PaletteData transientpalette = { -1 };

	// All data is being stored in contiguous blocks that can be used as uniform buffers as-is.
	TArray<PaletteData> palettes;
	TArray<PalswapData> palswaps;
	TMap<int, int> swappedpalmap;
	FHardwareTexture* palswapTexture = nullptr;
	GLInstance* const inst;

	//OpenGLRenderer::GLDataBuffer* palswapBuffer = nullptr;

	unsigned FindPalette(const uint8_t* paldata);
	unsigned FindPalswap(const uint8_t* paldata);

public:
	PaletteManager(GLInstance *inst_) : inst(inst_)
	{}
	~PaletteManager();
	void DeleteAll();
	void SetPalette(int index, const uint8_t *data, bool transient);
	void SetPalswapData(int index, const uint8_t* data, int numshades);

	void BindPalette(int index);
	void BindPalswap(int index);
	int ActivePalswap() const { return lastsindex; }
	int LookupPalette(int palette, int palswap, bool brightmap);
	const PalEntry *GetPaletteData(int palid) const { return palettes[palid].colors; }
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
	Matrix_Glow,
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

enum ERenderAlpha
{
	STYLEALPHA_Zero,		// Blend factor is 0.0
	STYLEALPHA_One,			// Blend factor is 1.0
	STYLEALPHA_Src,			// Blend factor is alpha
	STYLEALPHA_InvSrc,		// Blend factor is 1.0 - alpha
	STYLEALPHA_SrcCol,		// Blend factor is color (HWR only)
	STYLEALPHA_InvSrcCol,	// Blend factor is 1.0 - color (HWR only)
	STYLEALPHA_DstCol,		// Blend factor is dest. color (HWR only)
	STYLEALPHA_InvDstCol,	// Blend factor is 1.0 - dest. color (HWR only)
	STYLEALPHA_Dst,			// Blend factor is dest. alpha
	STYLEALPHA_InvDst,		// Blend factor is 1.0 - dest. alpha
	STYLEALPHA_MAX
};

enum ERenderOp
{
	STYLEOP_Add,			// Add source to destination
	STYLEOP_Sub,			// Subtract source from destination
	STYLEOP_RevSub,			// Subtract destination from source
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


	VSMatrix matrices[NUMMATRICES];
	PolymostRenderState renderState;
	FShader* activeShader;
	PolymostShader* polymostShader;
	SurfaceShader* surfaceShader;
	FShader* vpxShader;
	
	
public:
	glinfo_t glinfo;
	FSamplerManager *mSamplers;
	
	void Init();
	void InitGLState(int fogmode, int multisample);
	void LoadPolymostShader();
	void LoadSurfaceShader();
	void LoadVPXShader();

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
	FGameTexture* NewTexture(const char *name, bool hightile);
	void BindTexture(int texunit, FHardwareTexture *texid, int sampler = NoSampler);
	void BindTexture(int texunit, FGameTexture* texid, int sampler = NoSampler);
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
	void SetFogLinear(float* color, float start, float end);
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

	void SetPaletteData(int index, const uint8_t* data, bool transient)
	{
		palmanager.SetPalette(index, data, transient);
	}

	void SetPalswapData(int index, const uint8_t* data, int numshades)
	{
		palmanager.SetPalswapData(index, data, numshades);
	}

	void SetPalswap(int index);

	int GetClamp()
	{
		return int(renderState.Clamp[0] + 2*renderState.Clamp[1]);
	}

	void SetClamp(int clamp)
	{
		renderState.Clamp[0] = clamp & 1;
		renderState.Clamp[1] = !!(clamp & 2);
	}

	void SetShade(int32_t shade, int numshades)
	{
		renderState.Shade = shade;
		renderState.NumShades = numshades;
	}

	void SetVisibility(float visibility, float fviewingrange)
	{
		renderState.VisFactor = visibility * fviewingrange * (1.f / (64.f * 65536.f));
	}

	void SetFogEnabled(bool fogEnabled)
	{
		renderState.FogEnabled = fogEnabled;
	}

	void UseColorOnly(bool useColorOnly)
	{
		renderState.UseColorOnly = useColorOnly;
	}

	void UseDetailMapping(bool useDetailMapping)
	{
		renderState.UseDetailMapping = useDetailMapping;
	}

	void UseGlowMapping(bool useGlowMapping)
	{
		renderState.UseGlowMapping = useGlowMapping;
	}

	void SetNpotEmulation(bool npotEmulation, float factor, float xOffset)
	{
		renderState.NPOTEmulation = npotEmulation;
		renderState.NPOTEmulationFactor = factor;
		renderState.NPOTEmulationXOffset = xOffset;
	}

	void SetShadeInterpolate(int32_t shadeInterpolate)
	{
		renderState.ShadeInterpolate = shadeInterpolate;
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
	
	FHardwareTexture* CreateIndexedTexture(FTexture* tex);
	FHardwareTexture* CreateTrueColorTexture(FTexture* tex, int palid, bool checkfulltransparency = false);
	FHardwareTexture *LoadTexture(FTexture* tex, int texturetype, int palid);
	bool SetTexture(FTexture* tex, int palette, int method, int sampleroverride = -1);
};

extern GLInstance GLInterface;
