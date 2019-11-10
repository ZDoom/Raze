#pragma once

#include "PalEntry.h"
#include "gl_buffers.h"
class PolymostShader;

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

	RF_HICTINT_Grayscale = 0x10000,
	RF_HICTINT_Invert = 0x20000,
	RF_HICTINT_Colorize = 0x40000,
	RF_HICTINT_BLEND_Screen = 0x80000,
	RF_HICTINT_BLEND_Overlay = 0x100000,
	RF_HICTINT_BLEND_Hardlight = 0x200000,
	RF_HICTINT_BLENDMASK = RF_HICTINT_BLEND_Screen | RF_HICTINT_BLEND_Overlay | RF_HICTINT_BLEND_Hardlight,
};

struct PolymostRenderState
{
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

	PalEntry FogColor;

	IVertexBuffer* VertexBuffer = nullptr;
	int VB_Offset[2] = {};
	IIndexBuffer* IndexBuffer = nullptr;
 	
	void Apply(PolymostShader *shader);
};
