/*
** glbackend.cpp
**
** OpenGL API abstraction
**
**---------------------------------------------------------------------------
** Copyright 2019 Christoph Oelckers
** All rights reserved.
**
** Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions
** are met:
**
** 1. Redistributions of source code must retain the above copyright
**    notice, this list of conditions and the following disclaimer.
** 2. Redistributions in binary form must reproduce the above copyright
**    notice, this list of conditions and the following disclaimer in the
**    documentation and/or other materials provided with the distribution.
** 3. The name of the author may not be used to endorse or promote products
**    derived from this software without specific prior written permission.
**
** THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
** IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
** OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
** IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
** INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
** NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
** THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
**---------------------------------------------------------------------------
**
*/
#include <memory>
#include "gl_load.h"
#include "glbackend.h"
#include "gl_samplers.h"
#include "gl_shader.h"
#include "textures.h"
#include "palette.h"
//#include "imgui.h"
#include "gamecontrol.h"
//#include "imgui_impl_sdl.h"
//#include "imgui_impl_opengl3.h"
#include "baselayer.h"
#include "gl_interface.h"
#include "v_2ddrawer.h"
#include "v_video.h"
#include "flatvertices.h"
#include "gl_renderer.h"
#include "gl_samplers.h"
#include "build.h"

float shadediv[MAXPALOOKUPS];

static int blendstyles[] = { GL_ZERO, GL_ONE, GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_SRC_COLOR, GL_ONE_MINUS_SRC_COLOR, GL_DST_COLOR, GL_ONE_MINUS_DST_COLOR, GL_DST_ALPHA, GL_ONE_MINUS_DST_ALPHA };
static int renderops[] = { GL_FUNC_ADD, GL_FUNC_ADD, GL_FUNC_SUBTRACT, GL_FUNC_REVERSE_SUBTRACT };
int depthf[] = { GL_ALWAYS, GL_LESS, GL_EQUAL, GL_LEQUAL };

TArray<VSMatrix> matrixArray;

FileReader GetResource(const char* fn)
{
	auto fr = fileSystem.OpenFileReader(fn);
	if (!fr.isOpen())
	{
		I_Error("Fatal: '%s' not found", fn);
	}
	return fr;
}

GLInstance GLInterface;

GLInstance::GLInstance()
{
	VSMatrix mat(0);
	matrixArray.Push(mat);
}

//void ImGui_Init_Backend();
//ImGuiContext* im_ctx;
TArray<uint8_t> ttf;

void GLInstance::Init(int ydim)
{
	//glinfo.bufferstorage =  !!strstr(glinfo.extensions, "GL_ARB_buffer_storage");
	glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &glinfo.maxanisotropy);

	new(&renderState) PolymostRenderState;	// reset to defaults.
	LoadPolymostShader();
}

void GLInstance::LoadPolymostShader()
{
	auto fr1 = GetResource("engine/shaders/glsl/polymost.vp");
	TArray<uint8_t> Vert = fr1.Read();
	fr1 = GetResource("engine/shaders/glsl/polymost.fp");
	TArray<uint8_t> Frag = fr1.Read();
	// Zero-terminate both strings.
	Vert.Push(0);
	Frag.Push(0);
	polymostShader = new PolymostShader();
	polymostShader->Load("PolymostShader", (const char*)Vert.Data(), (const char*)Frag.Data());
	SetPolymostShader();
}

void GLInstance::InitGLState(int fogmode, int multisample)
{
	glShadeModel(GL_SMOOTH);  // GL_FLAT
	glEnable(GL_TEXTURE_2D);

    glPixelStorei(GL_PACK_ALIGNMENT, 1);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

    if (multisample > 0 )
    {
		//glHint(GL_MULTISAMPLE_FILTER_HINT_NV, GL_NICEST);
        glEnable(GL_MULTISAMPLE);
    }
	glGetIntegerv(GL_MAX_TEXTURE_SIZE, &maxTextureSize);

	// This is a bad place to call this but without deconstructing the entire render loops in all front ends there is no way to have a well defined spot for this stuff.
	// Before doing that the backend needs to work in some fashion, so we have to make sure everything is set up when the first render call is performed.
	screen->BeginFrame();	
	bool useSSAO = (gl_ssao != 0);
    OpenGLRenderer::GLRenderer->mBuffers->BindSceneFB(useSSAO);
	ClearBufferState();
}

void GLInstance::Deinit()
{
#if 0
	if (im_ctx)
	{
		ImGui_ImplOpenGL3_Shutdown();
		ImGui_ImplSDL2_Shutdown();
		ImGui::DestroyContext(im_ctx);
	}
#endif
	if (polymostShader) delete polymostShader;
	polymostShader = nullptr;
	if (surfaceShader) delete surfaceShader;
	surfaceShader = nullptr;
	activeShader = nullptr;
	lastPalswapIndex = -1;
}

void GLInstance::ResetFrame()
{
	GLState s;
	lastState = s; // Back to defaults.
	lastState.Style.BlendOp = -1;	// invalidate. This forces a reset for the next operation

}

void GLInstance::SetVertexBuffer(IVertexBuffer* vb, int offset1, int offset2)
{
	int o[] = { offset1, offset2 };
	static_cast<OpenGLRenderer::GLVertexBuffer*>(vb)->Bind(o);
}

void GLInstance::SetIndexBuffer(IIndexBuffer* vb)
{
	if (vb) static_cast<OpenGLRenderer::GLIndexBuffer*>(vb)->Bind();
	else glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}

void GLInstance::ClearBufferState()
{
	SetVertexBuffer(screen->mVertexData->GetBufferObjects().first, 0, 0);
	SetIndexBuffer(nullptr);
}

	
static GLint primtypes[] =
{
	GL_TRIANGLES,
	GL_TRIANGLE_STRIP,
	GL_TRIANGLE_FAN,
	GL_LINES
};
	

void GLInstance::Draw(EDrawType type, size_t start, size_t count)
{
	applyMapFog();
	renderState.vindex = start;
	renderState.vcount = count;
	renderState.primtype = type;
	rendercommands.Push(renderState);
	clearMapFog();
	SetIdentityMatrix(Matrix_Texture);
	SetIdentityMatrix(Matrix_Detail);
	renderState.StateFlags &= ~(STF_CLEARCOLOR | STF_CLEARDEPTH | STF_VIEWPORTSET | STF_SCISSORSET);
}

void GLInstance::DrawElement(EDrawType type, size_t start, size_t count, PolymostRenderState &renderState)
{
	if (activeShader == polymostShader)
	{
		glVertexAttrib4fv(2, renderState.Color);
		if (renderState.Color[3] != 1.f) renderState.Flags &= ~RF_Brightmapping;	// The way the colormaps are set up means that brightmaps cannot be used on translucent content at all.
		renderState.Apply(polymostShader, lastState);
	}
	if (type != DT_LINES)
	{
		glDrawElements(primtypes[type], count, GL_UNSIGNED_INT, (void*)(intptr_t)(start * sizeof(uint32_t)));
	}
	else
	{
		glDrawArrays(primtypes[type], start, count);
	}
}

void GLInstance::DoDraw()
{
	for (auto& rs : rendercommands)
	{
		glVertexAttrib4fv(2, rs.Color);
		if (rs.Color[3] != 1.f) rs.Flags &= ~RF_Brightmapping;	// The way the colormaps are set up means that brightmaps cannot be used on translucent content at all.
		rs.Apply(polymostShader, lastState);
		glDrawArrays(primtypes[rs.primtype], rs.vindex, rs.vcount);
	}
	rendercommands.Clear();
	matrixArray.Resize(1);
}


int GLInstance::SetMatrix(int num, const VSMatrix *mat)
{
	int r = renderState.matrixIndex[num];
	if (num == Matrix_Projection) mProjectionM5 = mat->get()[5];
	renderState.matrixIndex[num] = matrixArray.Size();
	matrixArray.Push(*mat);
	return r;
}

void GLInstance::ReadPixels(int xdim, int ydim, uint8_t* buffer)
{
	glReadPixels(0, 0, xdim, ydim, GL_RGB, GL_UNSIGNED_BYTE, buffer);
}

void GLInstance::SetPolymostShader()
{
	if (activeShader != polymostShader)
	{
		polymostShader->Bind();
		activeShader = polymostShader;
	}
}

void GLInstance::SetSurfaceShader()
{
	if (activeShader != surfaceShader)
	{
		surfaceShader->Bind();
		activeShader = surfaceShader;
	}
}

void GLInstance::SetPalette(int index)
{
#if 0
	palmanager.BindPalette(index);
#endif
}


void GLInstance::SetPalswap(int index)
{
#if 0
	palmanager.BindPalswap(index);
#endif
	renderState.ShadeDiv = shadediv[index] == 0 ? 1.f / (renderState.NumShades - 2) : shadediv[index];
}

void GLInstance::DrawImGui(ImDrawData* data)
{
#if 0
	ImGui_ImplOpenGL3_RenderDrawData(data);
#endif
}


//===========================================================================
// 
//	Binds a texture to the renderer
//
//===========================================================================

void PolymostRenderState::ApplyMaterial(FMaterial* mat, int clampmode, int translation, int overrideshader)
{

	auto tex = mat->Source();
	//mEffectState = overrideshader >= 0 ? overrideshader : mat->GetShaderIndex();
	//mShaderTimer = tex->GetShaderSpeed();
	//SetSpecular(tex->GetGlossiness(), tex->GetSpecularLevel());
	if (tex->isHardwareCanvas()) static_cast<FCanvasTexture*>(tex->GetTexture())->NeedUpdate();

	clampmode = tex->GetClampMode(clampmode);

	// avoid rebinding the same texture multiple times.
	//if (mat == lastMaterial && lastClamp == clampmode && translation == lastTranslation) return;
#if 0
	lastMaterial = mat;
	lastClamp = clampmode;
	lastTranslation = translation;
#endif

	int usebright = false;
	int maxbound = 0;

	int numLayers = mat->NumLayers();
	MaterialLayerInfo* layer;
	auto base = static_cast<FHardwareTexture*>(mat->GetLayer(0, translation, &layer));
	if (base->BindOrCreate(layer->layerTexture, 0, clampmode, translation, layer->scaleFlags))
	{
		for (int i = 1; i < numLayers; i++)
		{
			auto systex = static_cast<FHardwareTexture*>(mat->GetLayer(i, 0, &layer));
			// fixme: Upscale flags must be disabled for certain layers.
			systex->BindOrCreate(layer->layerTexture, i, clampmode, 0, layer->scaleFlags);
			maxbound = i;
		}
	}
	// unbind everything from the last texture that's still active
	for (int i = maxbound + 1; i <= 1/*maxBoundMaterial*/; i++)
	{
		FHardwareTexture::Unbind(i);
		//maxBoundMaterial = maxbound;
	}
}

void PolymostRenderState::Apply(PolymostShader* shader, GLState &oldState)
{
	bool reset = false;
	if (mMaterial.mChanged)
	{
		mMaterial.mChanged = false;
		ApplyMaterial(mMaterial.mMaterial, mMaterial.mClampMode, mMaterial.mTranslation, mMaterial.mOverrideShader);
	}
 
	if (StateFlags != oldState.Flags)
	{
		if ((StateFlags ^ oldState.Flags) & STF_DEPTHTEST)
		{
			if (StateFlags & STF_DEPTHTEST) glEnable(GL_DEPTH_TEST);
			else glDisable(GL_DEPTH_TEST);
		}
		if ((StateFlags ^ oldState.Flags) & STF_BLEND)
		{
			if (StateFlags & STF_BLEND) glEnable(GL_BLEND);
			else glDisable(GL_BLEND);
		}
		if ((StateFlags ^ oldState.Flags) & STF_MULTISAMPLE)
		{
			if (StateFlags & STF_MULTISAMPLE) glEnable(GL_MULTISAMPLE);
			else glDisable(GL_MULTISAMPLE);
		}
		if ((StateFlags ^ oldState.Flags) & (STF_STENCILTEST|STF_STENCILWRITE))
		{
			if (StateFlags & STF_STENCILWRITE)
			{
				glEnable(GL_STENCIL_TEST);
				glClear(GL_STENCIL_BUFFER_BIT);
				glStencilOp(GL_REPLACE, GL_REPLACE, GL_REPLACE);
				glStencilFunc(GL_ALWAYS, 1/*value*/, 0xFF);
			}
			else if (StateFlags & STF_STENCILTEST)
			{
				glEnable(GL_STENCIL_TEST);
				glStencilFunc(GL_EQUAL, 1/*value*/, 0xFF);
				glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);

			}
			else
			{
				glDisable(GL_STENCIL_TEST);
			}
		}
		if ((StateFlags ^ oldState.Flags) & (STF_CULLCW | STF_CULLCCW))
		{
			if (StateFlags & (STF_CULLCW | STF_CULLCCW))
			{
				glFrontFace(StateFlags & STF_CULLCW ? GL_CW : GL_CCW);
				glEnable(GL_CULL_FACE);
				glCullFace(GL_BACK); // Cull_Front is not being used.
			}
			else
			{
				glDisable(GL_CULL_FACE);
			}
		}
		if ((StateFlags ^ oldState.Flags) & STF_COLORMASK)
		{
			if (StateFlags & STF_COLORMASK) glColorMask(1, 1, 1, 1);
			else glColorMask(0, 0, 0, 0);
		}
		if ((StateFlags ^ oldState.Flags) & STF_DEPTHMASK)
		{
			if (StateFlags & STF_DEPTHMASK) glDepthMask(1);
			else glDepthMask(0);
		}
		if ((StateFlags ^ oldState.Flags) & STF_WIREFRAME)
		{
			glPolygonMode(GL_FRONT_AND_BACK, (StateFlags & STF_WIREFRAME) ? GL_LINE : GL_FILL);
		}
		if (StateFlags & (STF_CLEARCOLOR| STF_CLEARDEPTH))
		{
			glClearColor(ClearColor.r / 255.f, ClearColor.g / 255.f, ClearColor.b / 255.f, 1.f);
			int bit = 0;
			if (StateFlags & STF_CLEARCOLOR) bit |= GL_COLOR_BUFFER_BIT;
			if (StateFlags & STF_CLEARDEPTH) bit |= GL_DEPTH_BUFFER_BIT;
			glClear(bit);
		}
		if (StateFlags & STF_VIEWPORTSET)
		{
			glViewport(vp_x, vp_y, vp_w, vp_h);
		}
		if (StateFlags & STF_SCISSORSET)
		{
			if (sc_x > SHRT_MIN)
			{
				glScissor(sc_x, sc_y, sc_w, sc_h);
				glEnable(GL_SCISSOR_TEST);
			}
			else
				glDisable(GL_SCISSOR_TEST);
		}
		if (mBias.mChanged)
		{
			if (mBias.mFactor == 0 && mBias.mUnits == 0)
			{
				glDisable(GL_POLYGON_OFFSET_FILL);
			}
			else
			{
				glEnable(GL_POLYGON_OFFSET_FILL);
			}
			glPolygonOffset(mBias.mFactor, mBias.mUnits);
			mBias.mChanged = false;
		}

		StateFlags &= ~(STF_CLEARCOLOR | STF_CLEARDEPTH | STF_VIEWPORTSET | STF_SCISSORSET);
		oldState.Flags = StateFlags;
	}
	if (Style != oldState.Style)
	{
		glBlendFunc(blendstyles[Style.SrcAlpha], blendstyles[Style.DestAlpha]);
		if (Style.BlendOp != oldState.Style.BlendOp) glBlendEquation(renderops[Style.BlendOp]);
		oldState.Style = Style;
		// Flags are not being checked yet, the current shader has no implementation for them.
	}
	if (DepthFunc != oldState.DepthFunc)
	{
		glDepthFunc(depthf[DepthFunc]);
		oldState.DepthFunc = DepthFunc;
	}
	// Disable brightmaps if non-black fog is used.
	if (!(Flags & RF_FogDisabled) && !FogColor.isBlack()) Flags &= ~RF_Brightmapping;
	shader->Flags.Set(Flags);
	shader->Shade.Set(Shade);
	shader->NumShades.Set(NumShades);
	shader->ShadeDiv.Set(ShadeDiv);
	shader->VisFactor.Set(VisFactor);
	shader->NPOTEmulationFactor.Set(NPOTEmulationFactor);
	shader->NPOTEmulationXOffset.Set(NPOTEmulationXOffset);
	shader->AlphaThreshold.Set(AlphaTest ? AlphaThreshold : -1.f);
	shader->Brightness.Set(Brightness);
	shader->FogColor.Set(FogColor);
	shader->TintFlags.Set(hictint_flags);
	shader->TintModulate.Set(hictint);
	shader->TintOverlay.Set(hictint_overlay);
	shader->FullscreenTint.Set(fullscreenTint);
	if (matrixIndex[Matrix_View] != -1)
		shader->RotMatrix.Set(matrixArray[matrixIndex[Matrix_View]].get());
	if (matrixIndex[Matrix_Projection] != -1)
		shader->ProjectionMatrix.Set(matrixArray[matrixIndex[Matrix_Projection]].get());
	if (matrixIndex[Matrix_Model] != -1)
		shader->ModelMatrix.Set(matrixArray[matrixIndex[Matrix_Model]].get());
	if (matrixIndex[Matrix_Detail] != -1)
		shader->DetailMatrix.Set(matrixArray[matrixIndex[Matrix_Detail]].get());
	if (matrixIndex[Matrix_Texture] != -1)
		shader->TextureMatrix.Set(matrixArray[matrixIndex[Matrix_Texture]].get());
	memset(matrixIndex, -1, sizeof(matrixIndex));
}

//===========================================================================
//
// Render the view to a savegame picture
//
//===========================================================================

void WriteSavePic(FileWriter* file, int width, int height)
{
	IntRect bounds;
	bounds.left = 0;
	bounds.top = 0;
	bounds.width = width;
	bounds.height = height;

	// we must be sure the GPU finished reading from the buffer before we fill it with new data.
	glFinish();

	// Switch to render buffers dimensioned for the savepic
	OpenGLRenderer::GLRenderer->mBuffers = OpenGLRenderer::GLRenderer->mSaveBuffers;
	OpenGLRenderer::GLRenderer->mBuffers->BindSceneFB(false);
	screen->SetViewportRects(&bounds);


	int oldx = xdim;
	int oldy = ydim;
	auto oldwindowxy1 = windowxy1;
	auto oldwindowxy2 = windowxy2;

	xdim = width;
	ydim = height;
	videoSetViewableArea(0, 0, width - 1, height - 1);
	renderSetAspect(65536, 65536);
	bool didit = gi->GenerateSavePic();

	xdim = oldx;
	ydim = oldy;
	videoSetViewableArea(oldwindowxy1.x, oldwindowxy1.y, oldwindowxy2.x, oldwindowxy2.y);

	// The 2D drawers can contain some garbage from the dirty render setup. Get rid of that first.
	twodgen.Clear();
	twodpsp.Clear();
	OpenGLRenderer::GLRenderer->CopyToBackbuffer(&bounds, false);

	// strictly speaking not needed as the glReadPixels should block until the scene is rendered, but this is to safeguard against shitty drivers
	glFinish();

	if (didit)
	{
		int numpixels = width * height;
		uint8_t* scr = (uint8_t*)Xmalloc(numpixels * 3);
		glReadPixels(0, 0, width, height, GL_RGB, GL_UNSIGNED_BYTE, scr);
		M_CreatePNG(file, scr + ((height - 1) * width * 3), nullptr, SS_RGB, width, height, -width * 3, vid_gamma);
		M_FinishPNG(file);
		Xfree(scr);
	}

	// Switch back the screen render buffers
	screen->SetViewportRects(nullptr);
	OpenGLRenderer::GLRenderer->mBuffers = OpenGLRenderer::GLRenderer->mScreenBuffers;
	bool useSSAO = (gl_ssao != 0);
	OpenGLRenderer::GLRenderer->mBuffers->BindSceneFB(useSSAO);
}


