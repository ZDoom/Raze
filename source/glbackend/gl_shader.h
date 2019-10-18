#pragma once;

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
    FBufferedUniform2f Clamp;
    FBufferedUniform1f Shade;
    FBufferedUniform1f NumShades;
    FBufferedUniform1f VisFactor;
    FBufferedUniform1f FogEnabled;
    FBufferedUniform1f UseColorOnly;
    FBufferedUniform1f UsePalette;
    FBufferedUniform1f UseDetailMapping;
    FBufferedUniform1f UseGlowMapping;
    FBufferedUniform1f NPOTEmulation;
    FBufferedUniform1f NPOTEmulationFactor;
    FBufferedUniform1f NPOTEmulationXOffset;
    FBufferedUniform1f Brightness;
	FBufferedUniform4f Fog;
	FBufferedUniform4f FogColor;
	FBufferedUniform1f ShadeInterpolate;

	FUniformMatrix4f   RotMatrix;
	FUniformMatrix4f   ModelMatrix;
	FUniformMatrix4f   ProjectionMatrix;
	FUniformMatrix4f   DetailMatrix;
	FUniformMatrix4f   TextureMatrix;

public:

	PolymostShader() = default;
	virtual bool Load(const char * name, const char * vert_prog_lump, const char * fragprog); //, const char * fragprog2, const char *defines);
};

class SurfaceShader : public FShader
{
public:
	SurfaceShader() = default;
	virtual bool Load(const char* name, const char* vert_prog_lump, const char* fragprog); //, const char * fragprog2, const char *defines);

};
