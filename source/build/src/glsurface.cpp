/*
 * glsurface.cpp
 *  A 32-bit rendering surface that can quickly blit 8-bit paletted buffers implemented in OpenGL.
 *
 * Copyright © 2018, Alex Dawson. All rights reserved.
 */

#include "glsurface.h"
#include "glad/glad.h"

#include "baselayer.h"
#include "build.h"
#include "tarray.h"
#include "../../glbackend/glbackend.h"

static TArray<uint8_t> buffer;
static FHardwareTexture* bufferTexture;
static vec2_t bufferRes;

static FHardwareTexture* paletteTexture;

bool glsurface_initialize(vec2_t bufferResolution)
{
    if (buffer.Size())
        glsurface_destroy();

    bufferRes = bufferResolution;
	buffer.Resize(bufferRes.x * bufferRes.y);

	bufferTexture = GLInterface.NewTexture();
	bufferTexture->CreateTexture(bufferRes.x, bufferRes.y, true, false);

    glsurface_setPalette(curpalettefaded);
	GLInterface.SetSurfaceShader();
    return true;
}

void glsurface_destroy()
{
	if (bufferTexture) delete bufferTexture;
	bufferTexture = nullptr;
	if (paletteTexture) delete paletteTexture;
	paletteTexture = nullptr;
}

void glsurface_setPalette(void* pPalette)
{
    if (!buffer.Size())
        return;
    if (!pPalette)
        return;

	if (!paletteTexture)
	{
		paletteTexture = GLInterface.NewTexture();
		paletteTexture->CreateTexture(256, 1, false, false);
	}
	paletteTexture->LoadTexture(palette);
	GLInterface.BindTexture(1, paletteTexture, Sampler2DNoFilter);
}

void* glsurface_getBuffer()
{
    return buffer.Data();
}

vec2_t glsurface_getBufferResolution()
{
    return bufferRes;
}

void glsurface_blitBuffer()
{
	if (!buffer.Size())
		return;

	bufferTexture->LoadTexture(buffer.Data());
	GLInterface.BindTexture(0, bufferTexture, Sampler2DNoFilter);

	auto data = GLInterface.AllocVertices(4);
	auto vt = data.second;

	vt[0].Set(-1.0f, 1.0f, 0.0f, 0.0f, 0.0f); //top-left
	vt[1].Set(-1.0f, -1.0f, 0.0f, 0.0f, 1.0f); //bottom-left
	vt[2].Set(1.0f, 1.0f, 0.0f, 1.0f, 0.0f); //top-right
	vt[3].Set(1.0f, -1.0f, 0.0f, 1.0f, 1.0f);  //bottom-right
	GLInterface.Draw(DT_TRIANGLE_STRIP, data.first, 4);
}
