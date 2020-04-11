#pragma once

#include "palentry.h"
#include "gl_buffers.h"
#include "renderstyle.h"
class PolymostShader;
struct GLState;

enum EMatrixType
{
	Matrix_View,
	Matrix_Projection,
	Matrix_Model,
	Matrix_Detail,
	Matrix_Texture,
	// These are the only ones being used.
	NUMMATRICES
};

enum PRSFlags
{
	RF_ColorOnly = 1,
	RF_UsePalette = 2,
	RF_DetailMapping = 4,
	RF_GlowMapping = 8,
	RF_Brightmapping = 16,
	RF_NPOTEmulation = 32,
	RF_ShadeInterpolate = 64,
	RF_FogDisabled = 128,
	RF_MapFog = 256,	// RRRA E2L1.

	RF_HICTINT_Grayscale = 0x10000,
	RF_HICTINT_Invert = 0x20000,
	RF_HICTINT_Colorize = 0x40000,
	RF_HICTINT_BLEND_Screen = 0x80000,
	RF_HICTINT_BLEND_Overlay = 0x100000,
	RF_HICTINT_BLEND_Hardlight = 0x200000,
	RF_HICTINT_BLENDMASK = RF_HICTINT_BLEND_Screen | RF_HICTINT_BLEND_Overlay | RF_HICTINT_BLEND_Hardlight,
	RF_HICTINT_MASK = 0x3f0000,

	STF_BLEND = 1,
	STF_COLORMASK = 2,
	STF_DEPTHMASK = 4,
	STF_DEPTHTEST = 8,
	STF_MULTISAMPLE = 16,
	STF_STENCILWRITE = 32,
	STF_STENCILTEST = 64,
	STF_CULLCW = 128,
	STF_CULLCCW = 256,
	STF_WIREFRAME = 512,
	STF_CLEARCOLOR = 1024,
	STF_CLEARDEPTH = 2048,
	STF_VIEWPORTSET = 4096,
	STF_SCISSORSET = 8192,


};

struct FDepthBiasState
{
	float mFactor;
	float mUnits;
	bool mChanged;

	void Reset()
	{
		mFactor = 0;
		mUnits = 0;
		mChanged = false;
	}
};


struct PolymostRenderState
{
	int vindex, vcount, primtype;
    float Shade;
    float NumShades = 64.f;
	float ShadeDiv = 62.f;
	float VisFactor = 128.f;
	int Flags = 0;
    float NPOTEmulationFactor = 1.f;
    float NPOTEmulationXOffset;
    float Brightness = 1.f;
	float AlphaThreshold = 0.5f;
	bool AlphaTest = true;
	float Color[4] = { 1,1,1,1 };
	short matrixIndex[NUMMATRICES] = { 0,0,0,0,0 };
	PalEntry fullscreenTint = 0xffffff, hictint = 0xffffff, hictint_overlay = 0xffffff;
	int hictint_flags = -1;
	FDepthBiasState mBias{ };

	int StateFlags = STF_COLORMASK|STF_DEPTHMASK;
	FRenderStyle Style{};
	int DepthFunc = 1;
	PalEntry ClearColor = 0;
	short vp_x, vp_y, vp_w, vp_h;
	short sc_x = SHRT_MIN, sc_y, sc_w, sc_h;
	int texIds[6], samplerIds[6];

	PalEntry FogColor;

	void Apply(PolymostShader *shader, GLState &oldstate);
};
