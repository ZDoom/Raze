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
#include <assert.h>
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
#include "build.h"
#include "v_draw.h"
#include "v_font.h"
#include "hw_viewpointuniforms.h"
#include "hw_viewpointbuffer.h"
#include "gl_renderstate.h"
#include "hw_cvars.h"

F2DDrawer twodpsp;
static int BufferLock = 0;

CVAR(Bool, hw_use_backend, true, CVAR_ARCHIVE | CVAR_GLOBALCONFIG);


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
	:palmanager(this)
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

auto i_data = R"(
	#version 330
	// This must match the HWViewpointUniforms struct
	layout(std140) uniform ViewpointUBO {
		mat4 ProjectionMatrix;
		mat4 ViewMatrix;
		mat4 NormalViewMatrix;

		vec4 uCameraPos;
		vec4 uClipLine;

		float uGlobVis;			// uGlobVis = R_GetGlobVis(r_visibility) / 32.0
		int uPalLightLevels;	
		int uViewHeight;		// Software fuzz scaling
		float uClipHeight;
		float uClipHeightDirection;
		int uShadowmapFilter;
	};
	uniform sampler2D detailtexture;
	uniform sampler2D glowtexture;
	uniform sampler2D brighttexture;

	uniform mat4 ModelMatrix;
	uniform mat4 NormalModelMatrix;
	uniform mat4 TextureMatrix;
	uniform vec4 uDetailParms;

	uniform vec4 uTextureBlendColor;
	uniform vec4 uTextureModulateColor;
	uniform vec4 uTextureAddColor;

	uniform float uAlphaThreshold;
	uniform vec4 uLightAttr;
	#define uLightLevel uLightAttr.a
	#define uFogDensity uLightAttr.b
	#define uLightFactor uLightAttr.g
	#define uLightDist uLightAttr.r
	uniform int uFogEnabled;
	uniform vec4 uFogColor;
	uniform int uTextureMode;
	uniform vec2 uNpotEmulation;

)";

void GLInstance::LoadPolymostShader()
{
	auto fr1 = GetResource("engine/shaders/glsl/polymost.vp");
	TArray<uint8_t> Vert = fr1.Read();
	fr1 = GetResource("engine/shaders/glsl/polymost.fp");
	TArray<uint8_t> Frag = fr1.Read();
	// Zero-terminate both strings.
	Vert.Push(0);
	Frag.Push(0);
	FStringf VertS("%s\n%s", i_data, Vert.Data());
	FStringf FragS("%s\n%s", i_data, Frag.Data());
	polymostShader = new PolymostShader();
	polymostShader->Load("PolymostShader", (const char*)VertS.GetChars(), (const char*)FragS.GetChars());
	SetPolymostShader();
}

void GLInstance::InitGLState(int fogmode, int multisample)
{
	glEnable(GL_TEXTURE_2D);

    if (multisample > 0 )
    {
		//glHint(GL_MULTISAMPLE_FILTER_HINT_NV, GL_NICEST);
        glEnable(GL_MULTISAMPLE);
    }
	// This is a bad place to call this but without deconstructing the entire render loops in all front ends there is no way to have a well defined spot for this stuff.
	// Before doing that the backend needs to work in some fashion, so we have to make sure everything is set up when the first render call is performed.
	screen->BeginFrame();	
	bool useSSAO = (gl_ssao != 0);
    OpenGLRenderer::GLRenderer->mBuffers->BindSceneFB(useSSAO);
	ClearBufferState();
}

void GLInstance::Deinit()
{
	if (polymostShader) delete polymostShader;
	polymostShader = nullptr;
	activeShader = nullptr;
	palmanager.DeleteAll();
	lastPalswapIndex = -1;
}

OpenGLRenderer::FHardwareTexture* GLInstance::NewTexture(int numchannels)
{
	return new OpenGLRenderer::FHardwareTexture(numchannels);
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

	
static GLint primtypes[] ={ GL_POINTS, GL_LINES, GL_TRIANGLES, GL_TRIANGLE_FAN, GL_TRIANGLE_STRIP };
	

void GLInstance::Draw(EDrawType type, size_t start, size_t count)
{
	assert (BufferLock > 0);
	applyMapFog();
	renderState.vindex = start;
	renderState.vcount = count;
	renderState.primtype = type;
	rendercommands.Push(renderState);
	clearMapFog();
	renderState.StateFlags &= ~(STF_CLEARCOLOR | STF_CLEARDEPTH | STF_VIEWPORTSET | STF_SCISSORSET);
}

void GLInstance::DrawElement(EDrawType type, size_t start, size_t count, PolymostRenderState &renderState)
{
	if (activeShader == polymostShader)
	{
		glVertexAttrib4fv(2, renderState.Color);
		renderState.Apply(polymostShader, lastState);
	}
	if (type != DT_Lines)
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
	if (hw_use_backend)
	{
		for (auto& rs : rendercommands)
		{
			rs.Apply(*screen->RenderState(), lastState);
			screen->RenderState()->Draw(rs.primtype, rs.vindex, rs.vcount);
		}
	}
	else
	{
		for (auto& rs : rendercommands)
		{
			glVertexAttrib4fv(2, rs.Color);
			rs.Apply(polymostShader, lastState);
			glDrawArrays(primtypes[rs.primtype], rs.vindex, rs.vcount);
		}
	}
	rendercommands.Clear();
	matrixArray.Resize(1);
}


int GLInstance::SetMatrix(int num, const VSMatrix *mat)
{
	int r = renderState.matrixIndex[num];
	renderState.matrixIndex[num] = matrixArray.Size();
	matrixArray.Push(*mat);
	return r;
}

void GLInstance::SetIdentityMatrix(int num)
{
	renderState.matrixIndex[num] = 0;
}


void GLInstance::ReadPixels(int xdim, int ydim, uint8_t* buffer)
{
	glReadPixels(0, 0, xdim, ydim, GL_RGB, GL_UNSIGNED_BYTE, buffer);
}

void GLInstance::SetPolymostShader()
{
	polymostShader->Bind();
	activeShader = polymostShader;
}

void GLInstance::SetPalette(int index)
{
	palmanager.BindPalette(index);
}


void GLInstance::SetPalswap(int index)
{
	palmanager.BindPalswap(index);
	renderState.ShadeDiv = lookups.tables[index].ShadeFactor;
}

//===========================================================================
// 
//	Binds a texture to the renderer
//
//===========================================================================

void PolymostRenderState::ApplyMaterial(FMaterial* mat, int clampmode, int translation, int overrideshader, PolymostShader* shader)
{
	auto tex = mat->Source();
	clampmode = tex->GetClampMode(clampmode);

	// avoid rebinding the same texture multiple times.
	//if (mat == lastMaterial && lastClamp == clampmode && translation == lastTranslation) return;
#if 0
	lastMaterial = mat;
	lastClamp = clampmode;
	lastTranslation = translation;
#endif

	int scf = 0;
	if (Flags & RF_UsePalette)
	{
		scf |= CTF_Indexed;
		translation = -1;
	}

	int usebright = false;
	int maxbound = 0;

 	int numLayers = mat->NumLayers();
	MaterialLayerInfo* layer;
	auto base = static_cast<OpenGLRenderer::FHardwareTexture*>(mat->GetLayer(0, translation, &layer));
	scf |= layer->scaleFlags;
	if (base->BindOrCreate(layer->layerTexture, 0, layer->clampflags == -1? clampmode : layer->clampflags, translation, scf))
	{
		int LayerFlags = 0;
		for (int i = 1; i < numLayers; i++)
		{
			auto systex = static_cast<OpenGLRenderer::FHardwareTexture*>(mat->GetLayer(i, 0, &layer));
			// fixme: Upscale flags must be disabled for certain layers.
			systex->BindOrCreate(layer->layerTexture, i, layer->clampflags == -1 ? clampmode : layer->clampflags, 0, layer->scaleFlags);
			maxbound = i;
			LayerFlags |= 32768 << i;
		}
		shader->TextureMode.Set(LayerFlags);
	}

}

void PolymostRenderState::Apply(PolymostShader* shader, GLState& oldState)
{
	if (!OpenGLRenderer::GLRenderer) return;
	auto sm = OpenGLRenderer::GLRenderer->mSamplerManager;

	bool reset = false;
	if (mMaterial.mChanged)
	{
		mMaterial.mChanged = false;
		ApplyMaterial(mMaterial.mMaterial, mMaterial.mClampMode, mMaterial.mTranslation, mMaterial.mOverrideShader, shader);
		float buffer[] = { mMaterial.mMaterial->GetDetailScale().X, mMaterial.mMaterial->GetDetailScale().Y, 1.f, 0.f };
		shader->DetailParms.Set(buffer);
	}

	if (PaletteTexture != nullptr)
	{
		PaletteTexture->Bind(4, false);
		sm->Bind(4, CLAMP_NOFILTER, -1);
	}
	if (LookupTexture != nullptr)
	{
		LookupTexture->Bind(5, false);
		sm->Bind(5, CLAMP_NOFILTER, -1);
	}
	glActiveTexture(GL_TEXTURE0);


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
		if ((StateFlags ^ oldState.Flags) & (STF_STENCILTEST | STF_STENCILWRITE))
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
		if (StateFlags & (STF_CLEARCOLOR | STF_CLEARDEPTH))
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
	if (!(Flags & RF_FogDisabled) && ShadeDiv >= 1 / 1000.f)
	{
		if (!FogColor.isBlack())
		{
			//Flags &= ~RF_Brightmapping;
			shader->muFogEnabled.Set(-1);
		}
		else
		{
			shader->muFogEnabled.Set(1);
		}
	}
	else shader->muFogEnabled.Set(0);

	shader->Flags.Set(Flags);
	shader->NPOTEmulation.Set(&NPOTEmulation.X);
	shader->AlphaThreshold.Set(AlphaTest ? AlphaThreshold : -1.f);
	shader->FogColor.Set((Flags& RF_MapFog)? PalEntry(0x999999) : FogColor);
	float lightattr[] = { ShadeDiv / (numshades - 2), VisFactor, (Flags & RF_MapFog) ? -5.f : 0.f , ShadeDiv >= 1 / 1000.f? Shade : 0 };
	shader->muLightParms.Set(lightattr);

	FVector4 addcol(0, 0, 0, 0);
	FVector4 modcol(fullscreenTint.r / 255.f, fullscreenTint.g / 255.f, fullscreenTint.b / 255.f, 0);
	FVector4 blendcol(0, 0, 0, 0);
	int flags = 0;
	if (fullscreenTint != 0xffffff) flags |= 16;
	if (hictint_flags != -1)
	{
		flags |= 16;
		if (hictint_flags & TINTF_COLORIZE)
		{
			modcol.X *= hictint.r / 64.f;
			modcol.Y *= hictint.g / 64.f;
			modcol.Z *= hictint.b / 64.f;
		}
		if (hictint_flags & TINTF_GRAYSCALE)
			modcol.W = 1.f;

		if (hictint_flags & TINTF_INVERT)
			flags |= 8;

		if (hictint_flags & TINTF_BLENDMASK)
			flags |= ((hictint_flags & TINTF_BLENDMASK) >> 6) + 1;

		addcol.W = flags;
	}
	shader->muTextureAddColor.Set(&addcol[0]);
	shader->muTextureModulateColor.Set(&modcol[0]);
	shader->muTextureBlendColor.Set(&blendcol[0]);
	if (matrixIndex[Matrix_Model] != -1)
		shader->ModelMatrix.Set(matrixArray[matrixIndex[Matrix_Model]].get());

	memset(matrixIndex, -1, sizeof(matrixIndex));
}

void PolymostRenderState::Apply(FRenderState& state, GLState& oldState)
{
	if (Flags & RF_ColorOnly)
	{
		state.EnableTexture(false);
	}
	else
	{
		state.EnableTexture(true);
		state.SetMaterial(mMaterial.mMaterial, mMaterial.mClampMode, mMaterial.mTranslation, mMaterial.mOverrideShader);
	}
	/* todo: bind indexed textures */

	state.SetColor(Color[0], Color[1], Color[2], Color[3]);
	if (StateFlags != oldState.Flags)
	{
		state.EnableDepthTest(StateFlags & STF_DEPTHTEST);
		state.EnableMultisampling(StateFlags & STF_MULTISAMPLE);

		if ((StateFlags ^ oldState.Flags) & (STF_STENCILTEST | STF_STENCILWRITE))
		{
			if (StateFlags & STF_STENCILWRITE)
			{
				state.EnableStencil(true);
				state.SetEffect(EFF_STENCIL);
				state.SetStencil(0, SOP_Increment, SF_ColorMaskOff);
			}
			else if (StateFlags & STF_STENCILTEST)
			{
				state.EnableStencil(true);
				state.SetEffect(EFF_NONE);
				state.SetStencil(1, SOP_Keep, SF_DepthMaskOff);
			}
			else
			{
				state.EnableStencil(false);
				state.SetEffect(EFF_NONE);
			}
		}
		if ((StateFlags ^ oldState.Flags) & (STF_CULLCW | STF_CULLCCW))
		{
			int cull = Cull_None;
			if (StateFlags & STF_CULLCCW) cull = Cull_CCW;
			else if (StateFlags & STF_CULLCW) cull = Cull_CW;
			state.SetCulling(cull);
		}
		state.SetColorMask(StateFlags & STF_COLORMASK);
		state.SetDepthMask(StateFlags & STF_DEPTHMASK);
		if (StateFlags & (STF_CLEARCOLOR | STF_CLEARDEPTH))
		{
			int clear = 0;
			//if (StateFlags & STF_CLEARCOLOR) clear |= CT_Color;
			if (StateFlags & STF_CLEARDEPTH) clear |= CT_Depth;
			state.Clear(clear);
		}
		if (StateFlags & STF_VIEWPORTSET)
		{
			state.SetViewport(vp_x, vp_y, vp_w, vp_h);
		}
		if (StateFlags & STF_SCISSORSET)
		{
			state.SetScissor(sc_x, sc_y, sc_w, sc_h);
		}
		state.SetDepthBias(mBias.mFactor, mBias.mUnits);

		StateFlags &= ~(STF_CLEARCOLOR | STF_CLEARDEPTH | STF_VIEWPORTSET | STF_SCISSORSET);
		oldState.Flags = StateFlags;
	}
	state.SetRenderStyle(Style);
	if (DepthFunc != oldState.DepthFunc)
	{
		state.SetDepthFunc(DepthFunc);
		oldState.DepthFunc = DepthFunc;
	}
	// Disable brightmaps if non-black fog is used.
	if (!(Flags & RF_FogDisabled) && ShadeDiv >= 1 / 1000.f)
	{
		state.EnableFog(FogColor.isBlack() && !(Flags & RF_MapFog) ? 1 : -1);
	}
	else state.EnableFog(0);
	state.SetFog((Flags & RF_MapFog) ? PalEntry(0x999999) : FogColor, 21.f);	// Fixme: The real density still needs to be implemented. 21 is a reasonable default only.
	state.SetSoftLightLevel(ShadeDiv >= 1 / 1000.f ? 255 - Scale(Shade, 255, numshades) : 255);
	state.SetLightParms(VisFactor, ShadeDiv / (numshades - 2));
	state.SetTextureMode(TextureMode);

	state.SetNpotEmulation(NPOTEmulation.Y, NPOTEmulation.X);
	state.AlphaFunc(Alpha_Greater, AlphaTest ? AlphaThreshold : -1.f);

	FVector4 addcol(0, 0, 0, 0);
	FVector4 modcol(fullscreenTint.r / 255.f, fullscreenTint.g / 255.f, fullscreenTint.b / 255.f, 1);
	FVector4 blendcol(0, 0, 0, 0);
	int flags = 0;

	if (fullscreenTint != 0xffffff) flags |= 16;
	if (hictint_flags != -1)
	{
		flags |= TextureManipulation::ActiveBit;
		if (hictint_flags & TINTF_COLORIZE)
		{
			modcol.X *= hictint.r / 64.f;
			modcol.Y *= hictint.g / 64.f;
			modcol.Z *= hictint.b / 64.f;
		}
		if (hictint_flags & TINTF_GRAYSCALE)
			modcol.W = 1.f;

		if (hictint_flags & TINTF_INVERT)
			flags |= TextureManipulation::InvertBit;

		if (hictint_flags & TINTF_BLENDMASK)
		{
			blendcol = modcol;	// WTF???, but the tinting code really uses the same color for both!
			flags |= (((hictint_flags & TINTF_BLENDMASK) >> 6) + 1) & TextureManipulation::BlendMask;
		}
		addcol.W = flags;
	}
	state.SetTextureColors(&modcol.X, &addcol.X, &blendcol.X);

	if (matrixIndex[Matrix_Model] != -1)
	{
		state.EnableModelMatrix(true);
		state.mModelMatrix = matrixArray[matrixIndex[Matrix_Model]];
	}
	else state.EnableModelMatrix(false);

	memset(matrixIndex, -1, sizeof(matrixIndex));
}

void DoWriteSavePic(FileWriter* file, ESSType ssformat, uint8_t* scr, int width, int height, bool upsidedown)
{
	int pixelsize = 3;
	int pitch = width * pixelsize;
	if (upsidedown)
	{
		scr += ((height - 1) * width * pixelsize);
		pitch *= -1;
	}

	M_CreatePNG(file, scr, nullptr, ssformat, width, height, pitch, vid_gamma);
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
	auto& RenderState = *screen->RenderState();

	// we must be sure the GPU finished reading from the buffer before we fill it with new data.
	glFinish();
	screen->mVertexData->Reset();

	// Switch to render buffers dimensioned for the savepic
	screen->SetSaveBuffers(true);
	screen->ImageTransitionScene(true);

	RenderState.SetVertexBuffer(screen->mVertexData);
	screen->mVertexData->Reset();
	//screen->mLights->Clear();
	screen->mViewpoints->Clear();

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
	twod->Clear();
	twodpsp.Clear();

	int numpixels = width * height;
	uint8_t* scr = (uint8_t*)M_Malloc(numpixels * 3);
	screen->CopyScreenToBuffer(width, height, scr);

	DoWriteSavePic(file, SS_RGB, scr, width, height, screen->FlipSavePic());
	M_Free(scr);

	// Switch back the screen render buffers
	screen->SetViewportRects(nullptr);
	screen->SetSaveBuffers(false);
}


static HWViewpointUniforms vp;

void renderSetProjectionMatrix(const float* p)
{
	if (p)
	{
		vp.mProjectionMatrix.loadMatrix(p);
		GLInterface.mProjectionM5 = p[5];
	}
	else vp.mProjectionMatrix.loadIdentity();
}

void renderSetViewMatrix(const float* p)
{
	if (p) vp.mViewMatrix.loadMatrix(p);
	else vp.mViewMatrix.loadIdentity();
}

void renderSetVisibility(float vis)
{
	vp.mGlobVis = vis;
}

void renderBeginScene()
{
	if (videoGetRenderMode() < REND_POLYMOST) return;
	assert(BufferLock == 0);

	vp.mPalLightLevels = numshades | (static_cast<int>(gl_fogmode) << 8) | ((int)5 << 16);
	screen->mViewpoints->SetViewpoint(OpenGLRenderer::gl_RenderState, &vp);

	if (BufferLock++ == 0)
	{
		screen->mVertexData->Map();
	}
}

void renderFinishScene()
{
	if (videoGetRenderMode() < REND_POLYMOST) return;
	assert(BufferLock == 1);
	if (--BufferLock == 0)
	{
		screen->mVertexData->Unmap();
		GLInterface.DoDraw();
	}
}

//==========================================================================
//
// DFrameBuffer :: DrawRateStuff
//
// Draws the fps counter, dot ticker, and palette debug.
//
//==========================================================================
CVAR(Bool, vid_fps, false, 0)

void DrawRateStuff()
{
	// Draws frame time and cumulative fps
	if (vid_fps)
	{
		FString fpsbuff = gi->statFPS();

		int textScale = active_con_scale(twod);
		int rate_x = screen->GetWidth() / textScale - NewConsoleFont->StringWidth(&fpsbuff[0]);
		twod->AddColorOnlyQuad(rate_x * textScale, 0, screen->GetWidth(), NewConsoleFont->GetHeight() * textScale, MAKEARGB(255, 0, 0, 0));
		DrawText(twod, NewConsoleFont, CR_WHITE, rate_x, 0, (char*)&fpsbuff[0],
			DTA_VirtualWidth, screen->GetWidth() / textScale,
			DTA_VirtualHeight, screen->GetHeight() / textScale,
			DTA_KeepRatio, true, TAG_DONE);

	}
}

int32_t r_scenebrightness = 0;



void Draw2D(F2DDrawer* drawer, FRenderState& state);

void videoShowFrame(int32_t w)
{
	if (gl_ssao)
	{
		screen->AmbientOccludeScene(GLInterface.GetProjectionM5());
		// To do: the translucent part of the scene should be drawn here, but the render setup in the games is really too broken to do SSAO.

		//glDrawBuffers(1, buffers);
	}

	float Brightness = 8.f / (r_scenebrightness + 8.f);

	OpenGLRenderer::GLRenderer->mBuffers->BlitSceneToTexture(); // Copy the resulting scene to the current post process texture
	screen->PostProcessScene(false, 0, Brightness, []() {
		Draw2D(&twodpsp, *screen->RenderState()); // draws the weapon sprites
		});
	screen->Update();
	// After finishing the frame, reset everything for the next frame. This needs to be done better.
	screen->BeginFrame();
	bool useSSAO = (gl_ssao != 0);
	screen->SetSceneRenderTarget(useSSAO);
	twodpsp.Clear();
	twod->Clear();
	GLInterface.ResetFrame();
}


