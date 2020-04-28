/*
** gl_framebuffer.cpp
** Implementation of the non-hardware specific parts of the
** OpenGL frame buffer
**
**---------------------------------------------------------------------------
** Copyright 2010-2020 Christoph Oelckers
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


#include "gl_load/gl_system.h"
#include "v_video.h"
#include "m_png.h"
#include "printf.h"
#include "templates.h"
#include "palette.h"
#include "build.h"
#include "glbackend/glbackend.h"

#include "gl_load/gl_interface.h"
#include "gl/system/gl_framebuffer.h"
#include "gl/renderer/gl_renderer.h"
#include "gl_renderbuffers.h"
#include "flatvertices.h"
#include "hw_lightbuffer.h"
/*
#include "gl/textures/gl_samplers.h"
#include "hwrenderer/utility/hw_clock.h"
#include "hwrenderer/utility/hw_vrmodes.h"
#include "hwrenderer/models/hw_models.h"
#include "hwrenderer/scene/hw_skydome.h"
#include "hwrenderer/data/hw_viewpointbuffer.h"
#include "gl/shaders/gl_shaderprogram.h"
*/
#include "gl_debug.h"
#include "r_videoscale.h"
//#include "gl_buffers.h"

//#include "hwrenderer/data/flatvertices.h"

EXTERN_CVAR (Bool, vid_vsync)
EXTERN_CVAR(Bool, r_drawvoxels)
EXTERN_CVAR(Int, gl_tonemap)
EXTERN_CVAR(Bool, gl_texture_usehires)
EXTERN_CVAR(Int, gl_ssao)

void gl_LoadExtensions();
void gl_PrintStartupLog();
void DrawRateStuff();
//void Draw2D(F2DDrawer *drawer, FRenderState &state);

extern bool vid_hdr_active;

void DrawFullscreenBlends();

namespace OpenGLRenderer
{
	FGLRenderer *GLRenderer;

//==========================================================================
//
//
//
//==========================================================================

OpenGLFrameBuffer::OpenGLFrameBuffer(void *hMonitor, bool fullscreen) : 
	Super(hMonitor, fullscreen) 
{
	// SetVSync needs to be at the very top to workaround a bug in Nvidia's OpenGL driver.
	// If wglSwapIntervalEXT is called after glBindFramebuffer in a frame the setting is not changed!
	Super::SetVSync(vid_vsync);

#ifdef IMPLEMENT_IT
	// Make sure all global variables tracking OpenGL context state are reset..
	FHardwareTexture::InitGlobalState();
	gl_RenderState.Reset();
#endif

	GLRenderer = nullptr;
}

OpenGLFrameBuffer::~OpenGLFrameBuffer()
{
	PPResource::ResetAll();

	if (mVertexData != nullptr) delete mVertexData;
#ifdef IMPLEMENT_IT
	if (mSkyData != nullptr) delete mSkyData;
	if (mViewpoints != nullptr) delete mViewpoints;
#endif
	if (mLights != nullptr) delete mLights;
	mShadowMap.Reset();

	if (GLRenderer)
	{
		delete GLRenderer;
		GLRenderer = nullptr;
	}
}

//==========================================================================
//
// Initializes the GL renderer
//
//==========================================================================

void OpenGLFrameBuffer::InitializeState()
{
	static bool first=true;

	if (first)
	{
		if (ogl_LoadFunctions() == ogl_LOAD_FAILED)
		{
			I_FatalError("Failed to load OpenGL functions.");
		}
	}

	gl_LoadExtensions();

	// Move some state to the framebuffer object for easier access.
	hwcaps = gl.flags;
	glslversion = gl.glslversion;
	uniformblockalignment = gl.uniformblockalignment;
	maxuniformblock = gl.maxuniformblock;
	vendorstring = gl.vendorstring;

	if (first)
	{
		first=false;
		gl_PrintStartupLog();
	}

	glDepthFunc(GL_LESS);

	glEnable(GL_DITHER);
	glDisable(GL_CULL_FACE);
	glDisable(GL_POLYGON_OFFSET_FILL);
	glEnable(GL_POLYGON_OFFSET_LINE);
	glEnable(GL_BLEND);
	glEnable(GL_DEPTH_CLAMP);
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_LINE_SMOOTH);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
	glClearDepth(1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	SetViewportRects(nullptr);

	mVertexData = new FFlatVertexBuffer(GetWidth(), GetHeight());
#ifdef IMPLEMENT_IT
	mSkyData = new FSkyVertexBuffer;
	mViewpoints = new HWViewpointBuffer;
#endif
	mLights = new FLightBuffer();
	GLRenderer = new FGLRenderer(this);
	GLRenderer->Initialize(GetWidth(), GetHeight());
	static_cast<GLDataBuffer*>(mLights->GetBuffer())->BindBase();

	mDebug = std::make_shared<FGLDebug>();
	mDebug->Update();
}

//==========================================================================
//
// Updates the screen
//
//==========================================================================

void OpenGLFrameBuffer::Update()
{
#if 0
	twoD.Reset();
	Flush3D.Reset();

	Flush3D.Clock();
#endif
	GLRenderer->Flush();
//	Flush3D.Unclock();

	Swap();
	Super::Update();
	screen->mVertexData->Reset();
}

//===========================================================================
//
//
//
//===========================================================================


const char* OpenGLFrameBuffer::DeviceName() const 
{
	return gl.modelstring;
}

//==========================================================================
//
// Swap the buffers
//
//==========================================================================

CVAR(Bool, gl_finishbeforeswap, false, CVAR_ARCHIVE|CVAR_GLOBALCONFIG);

void OpenGLFrameBuffer::Swap()
{
	bool swapbefore = gl_finishbeforeswap && camtexcount == 0;
	//Finish.Reset();
	//Finish.Clock();
	if (swapbefore) glFinish();
	FPSLimit();
	SwapBuffers();
	if (!swapbefore) glFinish();
	//Finish.Unclock();
	camtexcount = 0;
	//FHardwareTexture::UnbindAll();
	mDebug->Update();
}

//==========================================================================
//
// Enable/disable vertical sync
//
//==========================================================================

void OpenGLFrameBuffer::SetVSync(bool vsync)
{
	// Switch to the default frame buffer because some drivers associate the vsync state with the bound FB object.
	GLint oldDrawFramebufferBinding = 0, oldReadFramebufferBinding = 0;
	glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &oldDrawFramebufferBinding);
	glGetIntegerv(GL_READ_FRAMEBUFFER_BINDING, &oldReadFramebufferBinding);
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
	glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);

	Super::SetVSync(vsync);

	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, oldDrawFramebufferBinding);
	glBindFramebuffer(GL_READ_FRAMEBUFFER, oldReadFramebufferBinding);
}

//===========================================================================
//
//
//===========================================================================

void OpenGLFrameBuffer::SetTextureFilterMode()
{
	if (GLRenderer != nullptr && GLRenderer->mSamplerManager != nullptr) GLRenderer->mSamplerManager->SetTextureFilterMode();
}

IHardwareTexture *OpenGLFrameBuffer::CreateHardwareTexture() 
{ 
	return new FHardwareTexture(true/*tex->bNoCompress*/);
}

#ifdef IMPLEMENT_IT
void OpenGLFrameBuffer::PrecacheMaterial(FMaterial *mat, int translation)
{
	auto tex = mat->tex;
	if (tex->isSWCanvas()) return;

	// Textures that are already scaled in the texture lump will not get replaced by hires textures.
	int flags = mat->isExpanded() ? CTF_Expand : (!tex->isScaled()) ? CTF_CheckHires : 0;
	int numLayers = mat->GetLayers();
	auto base = static_cast<FHardwareTexture*>(mat->GetLayer(0, translation));

	if (base->BindOrCreate(tex, 0, CLAMP_NONE, translation, flags))
	{
		for (int i = 1; i < numLayers; i++)
		{
			FTexture *layer;
			auto systex = static_cast<FHardwareTexture*>(mat->GetLayer(i, 0, &layer));
			systex->BindOrCreate(layer, i, CLAMP_NONE, 0, mat->isExpanded() ? CTF_Expand : 0);
		}
	}
	// unbind everything. 
	FHardwareTexture::UnbindAll();
}

FModelRenderer *OpenGLFrameBuffer::CreateModelRenderer(int mli)
{
	return new FHWModelRenderer(nullptr, gl_RenderState, mli);
}
#endif

IVertexBuffer *OpenGLFrameBuffer::CreateVertexBuffer()
{ 
	return new GLVertexBuffer; 
}

IIndexBuffer *OpenGLFrameBuffer::CreateIndexBuffer()
{ 
	return new GLIndexBuffer; 
}

IDataBuffer *OpenGLFrameBuffer::CreateDataBuffer(int bindingpoint, bool ssbo, bool needsresize)
{
	return new GLDataBuffer(bindingpoint, ssbo);
}

#ifdef IMPLEMENT_IT
void OpenGLFrameBuffer::TextureFilterChanged()
{
	if (GLRenderer != NULL && GLRenderer->mSamplerManager != NULL) GLRenderer->mSamplerManager->SetTextureFilterMode();
}
#endif

void OpenGLFrameBuffer::BlurScene(float amount)
{
	GLRenderer->BlurScene(amount);
}

#if 0
void OpenGLFrameBuffer::UpdatePalette()
{
	if (GLRenderer)
		GLRenderer->ClearTonemapPalette();
}
#endif

//===========================================================================
//
// 
//
//===========================================================================

void OpenGLFrameBuffer::BeginFrame()
{
	SetViewportRects(nullptr);
	if (GLRenderer != nullptr)
		GLRenderer->BeginFrame();
}

//===========================================================================
// 
//	Takes a screenshot
//
//===========================================================================

TArray<uint8_t> OpenGLFrameBuffer::GetScreenshotBuffer(int &pitch, ESSType &color_type, float &gamma)
{
	const auto &viewport = mOutputLetterbox;

	// Grab what is in the back buffer.
	// We cannot rely on SCREENWIDTH/HEIGHT here because the output may have been scaled.
	TArray<uint8_t> pixels;
	pixels.Resize(viewport.width * viewport.height * 3);
	glPixelStorei(GL_PACK_ALIGNMENT, 1);
	glReadPixels(viewport.left, viewport.top, viewport.width, viewport.height, GL_RGB, GL_UNSIGNED_BYTE, &pixels[0]);
	glPixelStorei(GL_PACK_ALIGNMENT, 4);

	// Copy to screenshot buffer:
	int w = SCREENWIDTH;
	int h = SCREENHEIGHT;

	TArray<uint8_t> ScreenshotBuffer(w * h * 3, true);

	float rcpWidth = 1.0f / w;
	float rcpHeight = 1.0f / h;
	for (int y = 0; y < h; y++)
	{
		for (int x = 0; x < w; x++)
		{
			float u = (x + 0.5f) * rcpWidth;
			float v = (y + 0.5f) * rcpHeight;
			int sx = u * viewport.width;
			int sy = v * viewport.height;
			int sindex = (sx + sy * viewport.width) * 3;
			int dindex = (x + (h - y - 1) * w) * 3;
			ScreenshotBuffer[dindex] = pixels[sindex];
			ScreenshotBuffer[dindex + 1] = pixels[sindex + 1];
			ScreenshotBuffer[dindex + 2] = pixels[sindex + 2];
		}
	}

	pitch = w * 3;
	color_type = SS_RGB;

	// Screenshot should not use gamma correction if it was already applied to rendered image
	gamma = 1;
	return ScreenshotBuffer;
}

//===========================================================================
// 
// 2D drawing
//
//===========================================================================

void OpenGLFrameBuffer::Draw2D()
{
	if (GLRenderer != nullptr)
	{
		GLRenderer->mBuffers->BindCurrentFB();
		::DrawFullscreenBlends();
        DrawRateStuff();
		GLInterface.Draw2D(twod);
	}
}

void OpenGLFrameBuffer::PostProcessScene(bool swscene, int fixedcm, const std::function<void()> &afterBloomDrawEndScene2D)
{
	GLRenderer->PostProcessScene(fixedcm, afterBloomDrawEndScene2D);
}


}

void videoShowFrame(int32_t w)
{
	static GLenum buffers[] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2 };

	if (gl_ssao)
	{
		glDrawBuffers(1, buffers);
		OpenGLRenderer::GLRenderer->AmbientOccludeScene(GLInterface.GetProjectionM5());
		glViewport(screen->mSceneViewport.left, screen->mSceneViewport.top, screen->mSceneViewport.width, screen->mSceneViewport.height);
		OpenGLRenderer::GLRenderer->mBuffers->BindSceneFB(true);
		glDrawBuffers(3, buffers);

		// To do: the translucent part of the scene should be drawn here

		glDrawBuffers(1, buffers);
	}

	OpenGLRenderer::GLRenderer->mBuffers->BlitSceneToTexture(); // Copy the resulting scene to the current post process texture
	screen->PostProcessScene(false, 0, []() {
		GLInterface.Draw2D(&twodpsp); // draws the weapon sprites
		});
	screen->Update();
	// After finishing the frame, reset everything for the next frame. This needs to be done better.
	screen->BeginFrame();
    if (gl_ssao)
	{
        OpenGLRenderer::GLRenderer->mBuffers->BindSceneFB(true);
        glDrawBuffers(3, buffers);
    }
    else
	{
        OpenGLRenderer::GLRenderer->mBuffers->BindSceneFB(false);
    }
	twodpsp.Clear();
	twod->Clear();
	GLInterface.ResetFrame();
}
