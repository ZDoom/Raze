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
#include "baselayer.h"
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

F2DDrawer twodpsp;
static int BufferLock = 0;

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
	new(&renderState) PolymostRenderState;	// reset to defaults.
}

void GLInstance::InitGLState(int fogmode, int multisample)
{
	// This is a bad place to call this but without deconstructing the entire render loops in all front ends there is no way to have a well defined spot for this stuff.
	// Before doing that the backend needs to work in some fashion, so we have to make sure everything is set up when the first render call is performed.
	screen->BeginFrame();	
}

void GLInstance::Deinit()
{
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

void GLInstance::DoDraw()
{
	for (auto& rs : rendercommands)
	{
		rs.Apply(*screen->RenderState(), lastState);
		screen->RenderState()->Draw(rs.primtype, rs.vindex, rs.vcount);
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


void GLInstance::SetPalette(int index)
{
	palmanager.BindPalette(index);
}


void GLInstance::SetPalswap(int index)
{
	palmanager.BindPalswap(index);
	renderState.ShadeDiv = lookups.tables[index].ShadeFactor;
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
	screen->mViewpoints->SetViewpoint(*screen->RenderState(), &vp);

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


