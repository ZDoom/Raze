#pragma once

#include "palentry.h"
#include "gl_buffers.h"
#include "renderstyle.h"
struct GLState;
class FMaterial;

enum EMatrixType
{
	Matrix_Model,
	// These are the only ones being used.
	NUMMATRICES
};

enum PRSFlags
{
	RF_ColorOnly = 1,
	RF_ShadeInterpolate = 64,
	RF_FogDisabled = 128,
	RF_MapFog = 256,	// RRRA E2L1.

	RF_TINT_Grayscale = 0x10000,
	RF_TINT_Invert = 0x20000,
	RF_TINT_Colorize = 0x40000,
	RF_TINT_BLEND_Screen = 0x80000,
	RF_TINT_BLEND_Overlay = 0x100000,
	RF_TINT_BLEND_Hardlight = 0x200000,
	RF_TINT_BLENDMASK = RF_TINT_BLEND_Screen | RF_TINT_BLEND_Overlay | RF_TINT_BLEND_Hardlight,
	RF_TINT_MASK = 0x3f0000,

	STF_BLEND = 1,
	STF_COLORMASK = 2,
	STF_DEPTHMASK = 4,
	STF_DEPTHTEST = 8,
	STF_STENCILWRITE = 32,
	STF_STENCILTEST = 64,
	STF_CULLCW = 128,
	STF_CULLCCW = 256,
	STF_CLEARCOLOR = 1024,
	STF_CLEARDEPTH = 2048,
	STF_VIEWPORTSET = 4096,
	STF_SCISSORSET = 8192,
};

struct PolymostRenderState
{
	int vindex, vcount, primtype;
    float Shade;
	float ShadeDiv = 62.f;
	float VisFactor = 128.f;
	int Flags = 0;
	int TextureMode = TM_NORMAL;
	FVector2 NPOTEmulation = { 0.f, 0.f };
    float AlphaThreshold = 0.5f;
	bool AlphaTest = true;
	float Color[4] = { 1,1,1,1 };
	short matrixIndex[NUMMATRICES] = { -1 };
	PalEntry fullscreenTint = 0xffffff, hictint = 0xffffff, hictint_overlay = 0xffffff;
	int hictint_flags = -1;
	FDepthBiasState mBias{ };
	FMaterialState mMaterial;

	int StateFlags = STF_COLORMASK|STF_DEPTHMASK;
	FRenderStyle Style{};
	int DepthFunc = 1;
	PalEntry ClearColor = 0;
	short vp_x, vp_y, vp_w, vp_h;
	short sc_x = SHRT_MIN, sc_y, sc_w, sc_h;

	PalEntry FogColor;

	void Apply(FRenderState & state, GLState& oldState);
};
