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
struct palette_t;
extern int xdim, ydim;

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
	bool isbright;
	const uint8_t *lookup;	// points to the original data. This is static so no need to copy
	FHardwareTexture* swaptexture;
	PalEntry fadeColor;
	uint8_t brightcolors[255];
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

	unsigned FindPalswap(const uint8_t* paldata, palette_t& fadecolor);

public:
	PaletteManager(GLInstance *inst_) : inst(inst_)
	{}
	~PaletteManager();
	void DeleteAll();
	void DeleteAllTextures();
	void SetPalette(int index, const uint8_t *data);
	void SetPalswapData(int index, const uint8_t* data, int numshades, palette_t &fadecolor);

	void BindPalette(int index);
	void BindPalswap(int index);
	int ActivePalswap() const { return lastsindex; }
	int LookupPalette(int palette, int palswap, bool brightmap, bool nontransparent255 = false);
	const PalEntry *GetPaletteData(int palid) const { return palettes[palid].colors; }
	unsigned FindPalette(const uint8_t* paldata);

};


struct glinfo_t {
	float maxanisotropy;
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
struct palette_t;
extern float shadediv[256];

struct GLState
{
	int Flags = STF_COLORMASK | STF_DEPTHMASK;
	FRenderStyle Style{};
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
	bool istrans = false;
	bool g_nontransparent255 = false;	// Ugh... This is for movie playback and needs to be maintained as global state.

	// Cached GL state.
	GLState lastState;

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
	
	FHardwareTexture* NewTexture();
	void BindTexture(int texunit, FHardwareTexture *texid, int sampler = NoSampler);
	void UnbindTexture(int texunit);
	void UnbindAllTextures();
	void EnableNonTransparent255(bool on)
	{
		g_nontransparent255 = on;
	}

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
	void ClearBufferState()
	{
		SetVertexBuffer(nullptr, 0, 0);
		SetIndexBuffer(nullptr);
		// Invalidate the pointers as well to make sure that if another buffer with the same address is used it actually gets bound.
		LastVertexBuffer = (IVertexBuffer*)~intptr_t(0);
		LastIndexBuffer = (IIndexBuffer*)~intptr_t(0);
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
	void SetColor(float r, float g, float b, float a = 1.f);
	void SetColorub(uint8_t r, uint8_t g, uint8_t b, uint8_t a = 255)
	{
		SetColor(r * (1 / 255.f), g * (1 / 255.f), b * (1 / 255.f), a * (1 / 255.f));
	}

	void SetDepthFunc(int func);
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

	void SetPalswapData(int index, const uint8_t* data, int numshades, palette_t& fadecolor)
	{
		palmanager.SetPalswapData(index, data, numshades, fadecolor);
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

	void SetVisibility(float visibility, float fviewingrange)
	{
		renderState.VisFactor = visibility* fviewingrange* (1.f / (64.f * 65536.f));
	}

	void EnableBlend(bool on)
	{
		if (on) renderState.StateFlags |= STF_BLEND;
		else renderState.StateFlags &= ~STF_BLEND;
	}

	void EnableDepthTest(bool on)
	{
		if (on) renderState.StateFlags |= STF_DEPTHTEST;
		else renderState.StateFlags &= ~STF_DEPTHTEST;
	}

	void EnableMultisampling(bool on)
	{
		if (on) renderState.StateFlags |= STF_MULTISAMPLE;
		else renderState.StateFlags &= ~STF_MULTISAMPLE;
	}

	void EnableStencilWrite(int value)
	{
		renderState.StateFlags |= STF_STENCILWRITE;
		renderState.StateFlags &= ~STF_STENCILTEST;
	}

	void EnableStencilTest(int value)
	{
		renderState.StateFlags &= ~STF_STENCILWRITE;
		renderState.StateFlags |= STF_STENCILTEST;
	}

	void DisableStencil()
	{
		renderState.StateFlags &= ~(STF_STENCILWRITE | STF_STENCILTEST);
	}

	void SetCull(int type, int winding = Winding_CW)
	{
		renderState.StateFlags &= ~(STF_CULLCCW | STF_CULLCW);
		if (type != Cull_None)
		{
			if (winding == Winding_CW) renderState.StateFlags |= STF_CULLCW;
			else renderState.StateFlags |= STF_CULLCCW;
		}
	}

	void SetColorMask(bool on)
	{
		if (on) renderState.StateFlags |= STF_COLORMASK;
		else renderState.StateFlags &= ~STF_COLORMASK;
	}

	void SetDepthMask(bool on)
	{
		if (on) renderState.StateFlags |= STF_DEPTHMASK;
		else renderState.StateFlags &= ~STF_DEPTHMASK;
	}

	void SetWireframe(bool on)
	{
		if (on) renderState.StateFlags |= STF_WIREFRAME;
		else renderState.StateFlags &= ~STF_WIREFRAME;
	}

	void ClearScreen(PalEntry pe, bool depth)
	{
		renderState.ClearColor = pe;
		renderState.StateFlags |= STF_CLEARCOLOR;
		if (depth) renderState.StateFlags |= STF_CLEARDEPTH;
	}

	void SetViewport(int x, int y, int w, int h)
	{
		renderState.vp_x = (short)x;
		renderState.vp_y = (short)y;
		renderState.vp_w = (short)w;
		renderState.vp_h = (short)h;
		renderState.StateFlags |= STF_VIEWPORTSET;
	}

	void SetScissor(int x1, int y1, int x2, int y2)
	{
		renderState.sc_x = (short)x1;
		renderState.sc_y = (short)y1;
		renderState.sc_w = (short)x2;
		renderState.sc_h = (short)y2;
		renderState.StateFlags |= STF_SCISSORSET;
	}

	void DisableScissor()
	{
		renderState.sc_x = -1;
		renderState.StateFlags |= STF_SCISSORSET;
	}


	void ClearScreen(PalEntry pe)
	{
		//twod->Clear();
		SetViewport(0, 0, xdim, ydim);
		ClearScreen(pe, false);
	}

	void ClearDepth()
	{
		renderState.StateFlags |= STF_CLEARDEPTH;
	}

	void SetRenderStyle(FRenderStyle style)
	{
		renderState.Style = style;
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

	void EnableAlphaTest(bool on)
	{
		renderState.AlphaTest = on;
	}

	void SetAlphaThreshold(float al)
	{
		renderState.AlphaThreshold = al;
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
