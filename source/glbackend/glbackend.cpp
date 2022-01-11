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
#include "glbackend.h"
#include "textures.h"
#include "palette.h"
#include "gamecontrol.h"
#include "v_2ddrawer.h"
#include "v_video.h"
#include "flatvertices.h"
#include "build.h"
#include "v_draw.h"
#include "v_font.h"
#include "hw_viewpointuniforms.h"
#include "hw_viewpointbuffer.h"
#include "hw_renderstate.h"
#include "hw_cvars.h"
#include "gamestruct.h"
#include "hw_models.h"
#include "gamefuncs.h"
#include "gamehud.h"

CVARD(Bool, hw_hightile, true, CVAR_ARCHIVE | CVAR_GLOBALCONFIG, "enable/disable hightile texture rendering")
bool hw_int_useindexedcolortextures;
CUSTOM_CVARD(Bool, hw_useindexedcolortextures, false, CVAR_ARCHIVE | CVAR_GLOBALCONFIG, "enable/disable indexed color texture rendering")
{
	if (screen) screen->SetTextureFilterMode();
}

EXTERN_CVAR(Bool, gl_texture)

static int BufferLock = 0;

TArray<VSMatrix> matrixArray;

GLInstance GLInterface;

GLInstance::GLInstance()
{
	VSMatrix mat(0);
	matrixArray.Push(mat);
}

void GLInstance::Init(int ydim)
{
	new(&renderState) PolymostRenderState;	// reset to defaults.
}

void GLInstance::Draw(EDrawType type, size_t start, size_t count)
{
	assert (BufferLock > 0);
	applyMapFog();
	renderState.vindex = (int)start;
	renderState.vcount = (int)count;
	renderState.primtype = type;
	rendercommands.Push(renderState);
	clearMapFog();
	renderState.StateFlags &= ~(STF_CLEARCOLOR | STF_CLEARDEPTH | STF_VIEWPORTSET | STF_SCISSORSET);
}

void GLInstance::DoDraw()
{
	GLState lastState;

	if (rendercommands.Size() > 0)
	{
		if (!useMapFog) hw_int_useindexedcolortextures = hw_useindexedcolortextures;

		lastState.Flags = ~rendercommands[0].StateFlags;	// Force ALL flags to be considered 'changed'.
		lastState.DepthFunc = INT_MIN;						// Something totally invalid.
		screen->RenderState()->EnableMultisampling(true);
		auto& state = *screen->RenderState();

		for (auto& rs : rendercommands)
		{
			if (rs.Apply(state, lastState))
			{
				if (!rs.model)
				{
					state.Draw(rs.primtype, rs.vindex, rs.vcount);
				}
				else
				{
					FHWModelRenderer mr(*screen->RenderState(), -1);
					state.SetDepthFunc(DF_LEqual);
					state.EnableTexture(true);
					rs.model->BuildVertexBuffer(&mr);
					mr.SetupFrame(rs.model, rs.mframes[0], rs.mframes[1], 0);
					rs.model->RenderFrame(&mr, rs.mMaterial.mTexture, rs.mframes[0], rs.mframes[1], 0.f, rs.mMaterial.mTranslation);
					state.SetDepthFunc(DF_Less);
					state.SetVertexBuffer(screen->mVertexData);
				}
			}
		}
		state.SetNpotEmulation(0, 0);	// make sure we do not leave this in an undefined state.
		renderState.Apply(*screen->RenderState(), lastState);	// apply any pending change before returning.
		rendercommands.Clear();
		hw_int_useindexedcolortextures = false;
	}
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
	renderState.matrixIndex[num] = -1;
}

void GLInstance::SetPalswap(int index)
{
	renderState.ShadeDiv = lookups.tables[index].ShadeFactor;
}

void GLInstance::SetFade(int index)
{
	renderState.FogColor = lookups.getFade(index);
}

extern int globalpal;
void GLInstance::SetShade(int32_t shade, int numshades)
{
	// Ugh... This particular palette does not fade to black. Should be handled better. 
	// It's really too bad that everything runs through here without being able to identify it anymore.
	renderState.drawblack = (!(g_gameType & GAMEFLAG_PSEXHUMED) || globalpal != 4) ? shade > numshades : false;
	renderState.Shade = min(shade, numshades - 1);
}

bool PolymostRenderState::Apply(FRenderState& state, GLState& oldState)
{
	// Fog must be done before the texture so that the texture selector can override it.
	bool foggy = (GLInterface.useMapFog || (FogColor & 0xffffff));
	// Disable brightmaps if non-black fog is used.
	if (!(Flags & RF_FogDisabled) && ShadeDiv >= 1 / 1000.f && foggy)
	{
		state.EnableFog(1);
		float density = GLInterface.useMapFog ? 350.f : 350.f - Scale(numshades - Shade, 150, numshades);
		state.SetFog((GLInterface.useMapFog) ? PalEntry(0x999999) : FogColor, density);
		state.SetSoftLightLevel(255);
		state.SetLightParms(128.f, 1 / 1000.f);
	}
	else
	{
		state.EnableFog(0);
		state.SetFog(0, 0);
		state.SetSoftLightLevel(ShadeDiv >= 1 / 1000.f ? 255 - Scale(Shade, 255, numshades) : 255);
		state.SetLightParms(VisFactor, ShadeDiv / (numshades - 2));
	}

	if (Flags & RF_ColorOnly)
	{
		state.EnableTexture(false);
	}
	else
	{
		if (!mMaterial.mTexture) return false;	// Oh no! Something passed an invalid tile!
		state.EnableTexture(gl_texture);
		state.SetMaterial(mMaterial.mTexture, mMaterial.uFlags, mMaterial.mScaleFlags, mMaterial.mClampMode, mMaterial.mTranslation, mMaterial.mOverrideShader);
	}

	if (!drawblack) state.SetColor(Color[0], Color[1], Color[2], Color[3]);
	else state.SetColor(0, 0, 0, Color[3]);
	if (StateFlags != oldState.Flags)
	{
		state.EnableDepthTest(StateFlags & STF_DEPTHTEST);

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
			if (StateFlags & STF_CLEARCOLOR) clear |= CT_Color;
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

	state.SetTextureMode(TextureMode);

	state.SetNpotEmulation(NPOTEmulation.Y, NPOTEmulation.X);
	state.AlphaFunc(Alpha_Greater, AlphaTest ? AlphaThreshold : -1.f);

	if (matrixIndex[Matrix_Model] != -1)
	{
		state.EnableModelMatrix(true);
		state.mModelMatrix = matrixArray[matrixIndex[Matrix_Model]];
	}
	else state.EnableModelMatrix(false);

	memset(matrixIndex, -1, sizeof(matrixIndex));
	return true;
}

static void PM_DoWriteSavePic(FileWriter* file, ESSType ssformat, uint8_t* scr, int width, int height, bool upsidedown)
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

void PM_WriteSavePic(FileWriter* file, int width, int height)
{
	IntRect bounds;
	bounds.left = 0;
	bounds.top = 0;
	bounds.width = width;
	bounds.height = height;
	auto& RenderState = *screen->RenderState();

	// we must be sure the GPU finished reading from the buffer before we fill it with new data.
	screen->WaitForCommands(false);
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
	screen->SetSceneRenderTarget(false);
	RenderState.SetPassType(NORMAL_PASS);
	RenderState.EnableDrawBuffers(1, true);

	screen->SetViewportRects(&bounds);
	twodpsp.Clear();
	/*bool didit =*/ gi->GenerateSavePic();

	float Brightness = 8.f / (r_scenebrightness + 8.f);
	screen->PostProcessScene(false, 0, Brightness, []() {
		Draw2D(&twodpsp, *screen->RenderState()); // draws the weapon sprites
		});

	xdim = oldx;
	ydim = oldy;
	videoSetViewableArea(oldwindowxy1.X, oldwindowxy1.Y, oldwindowxy2.X, oldwindowxy2.Y);

	// The 2D drawers can contain some garbage from the dirty render setup. Get rid of that first.
	twod->Clear();
	twodpsp.Clear();

	int numpixels = width * height;
	uint8_t* scr = (uint8_t*)M_Malloc(numpixels * 3);
	screen->CopyScreenToBuffer(width, height, scr);

	PM_DoWriteSavePic(file, SS_RGB, scr, width, height, screen->FlipSavePic());
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

void renderSetViewpoint(float x, float y, float z)
{
	vp.mCameraPos = {x, z, y, 0};
}

void renderBeginScene()
{
	assert(BufferLock == 0);

	vp.mPalLightLevels = numshades | (static_cast<int>(gl_fogmode) << 8) | ((int)5 << 16);
	screen->mViewpoints->SetViewpoint(*screen->RenderState(), &vp);

	if (BufferLock++ == 0)
	{
		screen->mVertexData->Map();
	}
}

void renderFinishScene()
{
	assert(BufferLock == 1);
	if (--BufferLock == 0)
	{
		screen->mVertexData->Unmap();
		GLInterface.DoDraw();
	}
}


int32_t r_scenebrightness = 0;



void videoShowFrame(int32_t w)
{
	int oldssao = gl_ssao;

	// These two features do not really work with Polymost because the rendered scene does not provide it
	gl_ssao = 0;
	float Brightness = 8.f / (r_scenebrightness + 8.f);

	screen->PostProcessScene(false, 0, Brightness, []() {
		Draw2D(&twodpsp, *screen->RenderState()); // draws the weapon sprites
		});
	screen->Update();
	screen->mVertexData->Reset();
	screen->mViewpoints->Clear();

	gl_ssao = oldssao;
}
