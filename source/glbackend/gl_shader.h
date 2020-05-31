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

#if 0
	FName mName;
	int projectionmatrix_index;
	int viewmatrix_index;
	int modelmatrix_index;
	int texturematrix_index;
	bool currentTextureMatrixState = false;
	bool currentModelMatrixState = false;
#endif

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
    FBufferedUniform1f Shade;
    FBufferedUniform1f NumShades;
	FBufferedUniform1f ShadeDiv;
	FBufferedUniform1f VisFactor;
    FBufferedUniform1f NPOTEmulationFactor;
    FBufferedUniform1f NPOTEmulationXOffset;
    FBufferedUniform1f Brightness;
	FBufferedUniform1f AlphaThreshold;
	FBufferedUniformPalEntry FogColor;
	FBufferedUniformPalEntry FullscreenTint;
	FBufferedUniformPalEntry TintModulate;
	FBufferedUniformPalEntry TintOverlay;
	FBufferedUniform1i TintFlags;
	FBufferedUniform2f DetailParms;


	FUniformMatrix4f   RotMatrix;
	FUniformMatrix4f   ModelMatrix;
	FUniformMatrix4f   ProjectionMatrix;
	FUniformMatrix4f   TextureMatrix;

public:

	PolymostShader() = default;
	virtual bool Load(const char * name, const char * vert_prog_lump, const char * fragprog); //, const char * fragprog2, const char *defines);
};

