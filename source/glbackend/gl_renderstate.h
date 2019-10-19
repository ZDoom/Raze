#pragma once

class PolymostShader;

struct PolymostRenderState
{
    float Clamp[2];
    float Shade;
    float NumShades = 64.f;
    float VisFactor = 128.f;
    float UseColorOnly;
    float UsePalette = 1.f;
    float UseDetailMapping;
    float UseGlowMapping;
    float NPOTEmulation;
    float NPOTEmulationFactor = 1.f;
    float NPOTEmulationXOffset;
    float Brightness = 1.f;
	float ShadeInterpolate = 1.f;
	float FogColor[4];
 	
	void Apply(PolymostShader *shader);
};
