#pragma once

class PolymostShader;

struct PolymostRenderState
{
	int PalSwapIndex;
	float PalswapPos[2];
    float PalswapSize[2];
    float Clamp[2];
    float Shade;
    float NumShades = 64.f;
    float VisFactor = 128.f;
    float FogEnabled = 1.f;
    float UseColorOnly;
    float UsePalette = 1.f;
    float UseDetailMapping;
    float UseGlowMapping;
    float NPOTEmulation;
    float NPOTEmulationFactor = 1.f;
    float NPOTEmulationXOffset;
    float Brightness = 1.f;
	float ShadeInterpolate = 1.f;
	float Fog[4];
	float FogColor[4];
 	
	void Apply(PolymostShader *shader);
};
