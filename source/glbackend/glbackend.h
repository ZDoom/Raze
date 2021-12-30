#pragma once
#include <limits.h>
#include <stdlib.h>
#include <algorithm>
#include <vector>
#include <map>
#include "c_cvars.h"
#include "gl_samplers.h"
#include "gl_hwtexture.h"
#include "matrix.h"
#include "palentry.h"
#include "renderstyle.h"
#include "hw_material.h"
#include "hw_renderstate.h"
#include "pm_renderstate.h"


class FShader;
class FGameTexture;
class GLInstance;
class F2DDrawer;
struct palette_t;
extern int xdim, ydim;

struct glinfo_t {
	float maxanisotropy;
};

enum ECullSide
{
	Cull_Front,
	Cull_Back
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
	int Flags;
	int DepthFunc;
};

class GLInstance
{
	friend IHardwareTexture* setpalettelayer(int layer, int translation);

public:
			TArray<PolymostRenderState> rendercommands;
	FGameTexture* currentTexture = nullptr;
	int MatrixChange = 0;

	PolymostRenderState renderState;


public:
	float mProjectionM5 = 1.0f; // needed by ssao
	glinfo_t glinfo;

	void Init(int y);

	static int GetTexDimension(int value)
	{
		//if (value > gl.max_texturesize) return gl.max_texturesize;
		return value;
	}

	GLInstance();
	void Draw(EDrawType type, size_t start, size_t count);
	void DoDraw();

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

	void SetTextureMode(int m)
	{
		renderState.TextureMode = m;
	}

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
	void SetFade(int palette);

	void SetShade(int32_t shade, int numshades);

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
		ClearScreen(pe, true);
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

	void SetMaterial(FGameTexture* tex, EUpscaleFlags upscalemask, int scaleflags, int clampmode, int translation, int overrideshader)
	{
		assert(tex);
		renderState.mMaterial.mTexture = tex;
		renderState.mMaterial.uFlags = upscalemask;
		renderState.mMaterial.mScaleFlags = scaleflags;
		renderState.mMaterial.mClampMode = clampmode;
		renderState.mMaterial.mTranslation = translation;
		renderState.mMaterial.mOverrideShader = overrideshader;
		renderState.mMaterial.mChanged = true;
	}

	void UseColorOnly(bool yes)
	{
		if (yes) renderState.Flags |= RF_ColorOnly;
		else renderState.Flags &= ~RF_ColorOnly;
	}

	void SetNpotEmulation(float factor, float xOffset)
	{
		renderState.NPOTEmulation.Y = factor;
		renderState.NPOTEmulation.X = xOffset;
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

	void EnableAlphaTest(bool on)
	{
		renderState.AlphaTest = on;
	}

	void SetAlphaThreshold(float al)
	{
		renderState.AlphaThreshold = al;
	}

	bool SetTexture(FGameTexture* tex, int palette, int sampleroverride, bool notindexed = false);

	void SetModel(FModel* model, int frame1, int frame2, float factor)
	{
		renderState.model = model;
		renderState.mframes[0] = frame1;
		renderState.mframes[1] = frame2;
		renderState.mfactor = factor;
	}
};

extern GLInstance GLInterface;

void renderSetProjectionMatrix(const float* p);
void renderSetViewMatrix(const float* p);
void renderSetVisibility(float v);
void renderSetViewpoint(float x, float y, float z);
void renderBeginScene();
void renderFinishScene();
void videoShowFrame(int32_t);
