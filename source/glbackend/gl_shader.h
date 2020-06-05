#pragma once

#include "gl_uniform.h"

class FShader
{
	friend class FShaderManager;
	friend class FRenderState;

	unsigned int hVertProg = 0;
	unsigned int hFragProg = 0;

protected:
	unsigned int hShader = 0;


public:
	FShader() = default;
	virtual ~FShader();

	virtual bool Load(const char * name, const char * vert_prog_lump, const char * fragprog); //, const char * fragprog2, const char *defines);
	bool Bind();
	unsigned int GetHandle() const { return hShader; }
};

class PolymostShader : public FShader
{
public:
	FBufferedUniform1i Flags;
    FBufferedUniform1f NPOTEmulationFactor;
    FBufferedUniform1f NPOTEmulationXOffset;
    FBufferedUniform1f Brightness;
	FBufferedUniformPalEntry FogColor;

	FBufferedUniform4f DetailParms;
	FBufferedUniform1f AlphaThreshold;
	FBufferedUniform1i muFogEnabled;
	FBufferedUniform4f muLightParms;
	FUniformMatrix4f   ModelMatrix;
	FUniformMatrix4f   TextureMatrix;
	FBufferedUniform4f muTextureBlendColor;
	FBufferedUniform4f muTextureModulateColor;
	FBufferedUniform4f muTextureAddColor;

public:

	PolymostShader() = default;
	virtual bool Load(const char * name, const char * vert_prog_lump, const char * fragprog); //, const char * fragprog2, const char *defines);
};

