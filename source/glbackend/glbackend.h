#pragma once
#include <stdlib.h>
#include <algorithm>
#include <vector>
#include <map>
#include "gl_samplers.h"
#include "gl_hwtexture.h"
#include "matrix.h"
#include "palentry.h"
#include "renderstyle.h"
#include "hw_material.h"
#include "hw_renderstate.h"
#include "pm_renderstate.h"

class FShader;
class PolymostShader;
class FGameTexture;
class GLInstance;
class F2DDrawer;
struct palette_t;
extern int xdim, ydim;

class PaletteManager
{
	OpenGLRenderer::FHardwareTexture* palettetextures[256] = {};
	OpenGLRenderer::FHardwareTexture* palswaptextures[256] = {};

	uint32_t lastindex = ~0u;
	uint32_t lastsindex = ~0u;

	GLInstance* const inst;

	//OpenGLRenderer::GLDataBuffer* palswapBuffer = nullptr;

	unsigned FindPalswap(const uint8_t* paldata, palette_t& fadecolor);

public:
	PaletteManager(GLInstance *inst_) : inst(inst_)
	{}
	~PaletteManager();
	void DeleteAll();
	void BindPalette(int index);
	void BindPalswap(int index);
};


struct glinfo_t {
	float maxanisotropy;
};

enum ECullSide
{
	Cull_Front,
	Cull_Back
};

enum EDepthFunct
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

struct ImDrawData;
struct palette_t;

enum
{
	MAX_TEXTURES = 4, /*15*/	// slot 15 is used internally and not available. - The renderer uses only 5, though.
};

struct GLState
{
	int Flags = STF_COLORMASK | STF_DEPTHMASK;
	FRenderStyle Style{};
	int DepthFunc = -1;
	int TexId[MAX_TEXTURES] = {}, SamplerId[MAX_TEXTURES] = {};
};

class GLInstance
{
public:
	TArray<PolymostRenderState> rendercommands;
	int maxTextureSize;
	PaletteManager palmanager;
	int lastPalswapIndex = -1;
	OpenGLRenderer::FHardwareTexture* texv;
	FGameTexture* currentTexture = nullptr;
	int MatrixChange = 0;

	// Cached GL state.
	GLState lastState;

	PolymostRenderState renderState;
	FShader* activeShader;
	
	
public:
	float mProjectionM5 = 1.0f; // needed by ssao
	PolymostShader* polymostShader;
	glinfo_t glinfo;
	
	void Init(int y);
	void InitGLState(int fogmode, int multisample);
	void LoadPolymostShader();
	void Draw2D(F2DDrawer* drawer);
	void DrawImGui(ImDrawData*);
	void ResetFrame();

	void Deinit();
	
	static int GetTexDimension(int value)
	{
		//if (value > gl.max_texturesize) return gl.max_texturesize;
		return value;
	}

	GLInstance();
	void Draw(EDrawType type, size_t start, size_t count);
	void DoDraw();
	void DrawElement(EDrawType type, size_t start, size_t count, PolymostRenderState& renderState);

	OpenGLRenderer::FHardwareTexture* NewTexture(int numchannels = 4);

	void SetVertexBuffer(IVertexBuffer* vb, int offset1, int offset2);
	void SetIndexBuffer(IIndexBuffer* vb);
	void ClearBufferState();

	float GetProjectionM5() { return mProjectionM5; }
	int SetMatrix(int num, const VSMatrix *mat );
	int SetMatrix(int num, const float *mat)
	{
		return SetMatrix(num, reinterpret_cast<const VSMatrix*>(mat));
	}
	void SetIdentityMatrix(int num);
	void RestoreMatrix(int num, int index)
	{
		renderState.matrixIndex[num] = index;
	}

	void SetPolymostShader();
	void SetPalette(int palette);
	
	void ReadPixels(int w, int h, uint8_t* buffer);

	void SetDepthBias(float a, float b)
	{
		renderState.mBias.mFactor = a;
		renderState.mBias.mUnits = b;
		renderState.mBias.mChanged = true;
	}

	void ClearDepthBias()
	{
		renderState.mBias.mFactor = 0;
		renderState.mBias.mUnits = 0;
		renderState.mBias.mChanged = true;
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
	}

	void SetVisibility(float visibility)
	{
		renderState.VisFactor = visibility;
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
		renderState.sc_x = SHRT_MIN;
		renderState.StateFlags |= STF_SCISSORSET;
	}

	void SetDepthFunc(int func)
	{
		renderState.DepthFunc = func;
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

	void SetColor(float r, float g, float b, float a = 1.f)
	{
		renderState.Color[0] = r;
		renderState.Color[1] = g;
		renderState.Color[2] = b;
		renderState.Color[3] = a;
	}
	void SetColorub(uint8_t r, uint8_t g, uint8_t b, uint8_t a = 255)
	{
		SetColor(r * (1 / 255.f), g * (1 / 255.f), b * (1 / 255.f), a * (1 / 255.f));
	}

	void SetMaterial(FMaterial* mat, int clampmode, int translation, int overrideshader)
	{
		assert(mat);
		renderState.mMaterial.mMaterial = mat;
		renderState.mMaterial.mClampMode = clampmode;
		renderState.mMaterial.mTranslation = translation;
		renderState.mMaterial.mOverrideShader = overrideshader;
		renderState.mMaterial.mChanged = true;
		//mTextureModeFlags = mat->GetLayerFlags();
	}

	void SetMaterial(FGameTexture* tex, EUpscaleFlags upscalemask, int scaleflags, int clampmode, int translation, int overrideshader)
	{
		assert(tex);
		if (shouldUpscale(tex, upscalemask)) scaleflags |= CTF_Upscale;
		SetMaterial(FMaterial::ValidateTexture(tex, scaleflags), clampmode, translation, overrideshader);
	}
#if 0
	void BindTexture(int texunit, OpenGLRenderer::FHardwareTexture* tex, int sampler)
	{
		if (!tex) return;
		if (texunit == 0)
		{
			if (tex->numChannels() == 1) 
				renderState.Flags |= RF_UsePalette;
			else 
				renderState.Flags &= ~RF_UsePalette;
		}
		renderState.texIds[texunit] = tex->GetTextureHandle();
		renderState.samplerIds[texunit] = sampler;
	}

	void UnbindTexture(int texunit)
	{
		renderState.texIds[texunit] = 0;
		renderState.samplerIds[texunit] = 0;
	}

	void UnbindAllTextures()
	{
		for (int texunit = 0; texunit < MAX_TEXTURES; texunit++)
		{
			UnbindTexture(texunit);
		}
	}
#endif

	void UseColorOnly(bool yes)
	{
		if (yes) renderState.Flags |= RF_ColorOnly;
		else renderState.Flags &= ~RF_ColorOnly;
	}

	void UseDetailMapping(bool yes)
	{
		if (yes) renderState.LayerFlags |= TEXF_Detailmap;
		else renderState.LayerFlags &= ~TEXF_Detailmap;
	}

	void UseGlowMapping(bool yes)
	{
		if (yes) renderState.LayerFlags |= TEXF_Glowmap;
		else renderState.LayerFlags &= ~TEXF_Glowmap;
	}

	void UseBrightmaps(bool yes)
	{
		if (yes) renderState.LayerFlags |= TEXF_Brightmap;
		else renderState.LayerFlags &= ~TEXF_Brightmap;
	}

	void SetNpotEmulation(float factor, float xOffset)
	{
		renderState.NPOTEmulationFactor = factor;
		renderState.NPOTEmulationXOffset = xOffset;
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

	// Hack...
	bool useMapFog = false;

	void SetMapFog(bool yes)
	{
		useMapFog = yes;
	}

	void applyMapFog()
	{
		if (useMapFog) renderState.Flags |= RF_MapFog;
		else renderState.Flags &= ~RF_MapFog;
	}

	void clearMapFog()
	{
		renderState.Flags &= ~RF_MapFog;
	}

	void SetTinting(int flags, PalEntry color, PalEntry overlayColor)
	{
		renderState.hictint = color;
		renderState.hictint_overlay = overlayColor;
		renderState.hictint_flags = flags;
	}

	void SetBasepalTint(PalEntry color)
	{
		renderState.fullscreenTint = color;
	}

	void EnableAlphaTest(bool on)
	{
		renderState.AlphaTest = on;
	}

	void SetAlphaThreshold(float al)
	{
		renderState.AlphaThreshold = al;
	}

	OpenGLRenderer::FHardwareTexture *LoadTexture(FTexture* tex, int texturetype, int palid);

	void SetPaletteTexture(OpenGLRenderer::FHardwareTexture* tex)
	{
		renderState.PaletteTexture = tex;
	}

	void SetLookupTexture(OpenGLRenderer::FHardwareTexture* tex)
	{
		renderState.LookupTexture = tex;
	}

	bool SetTexture(int globalpicnum, FGameTexture* tex, int palette, int sampleroverride);
};

extern GLInstance GLInterface;

void renderSetProjectionMatrix(const float* p);
void renderSetViewMatrix(const float* p);
void renderSetVisibility(float v);
void renderBeginScene();
void renderFinishScene();
void DrawRateStuff();
