/*
** gl_renderer.cpp
** Renderer interface
**
**---------------------------------------------------------------------------
** Copyright 2005-2020 Christoph Oelckers
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
#include "files.h"
#include "v_video.h"
#include "m_png.h"
#include "filesystem.h"
#include "i_time.h"
#include "cmdlib.h"
#include "m_png.h"
//#include "swrenderer/r_swscene.h"
//#include "hwrenderer/utility/hw_clock.h"

#include "gl_load/gl_interface.h"
#include "gl/system/gl_framebuffer.h"
#include "gamecvars.h"
#include "gl/system/gl_debug.h"
#include "gl/renderer/gl_renderer.h"
//#include "gl/renderer/gl_renderstate.h"
#include "gl/renderer/gl_renderbuffers.h"
#include "gl/shaders/gl_shaderprogram.h"
//#include "hwrenderer/data/flatvertices.h"
//#include "gl/textures/gl_samplers.h"
//#include "hwrenderer/dynlights/hw_lightbuffer.h"
//#include "hwrenderer/data/hw_viewpointbuffer.h"
#include "r_videoscale.h"
//#include "r_data/models/models.h"
#include "gl/renderer/gl_postprocessstate.h"
#include "gl/system/gl_buffers.h"
#include "../glbackend/gl_hwtexture.h"
#include "build.h"

EXTERN_CVAR(Int, screenblocks)
EXTERN_CVAR(Bool, cl_capfps)

extern bool NoInterpolateView;

namespace OpenGLRenderer
{

//===========================================================================
// 
// Renderer interface
//
//===========================================================================

//-----------------------------------------------------------------------------
//
// Initialize
//
//-----------------------------------------------------------------------------

FGLRenderer::FGLRenderer(OpenGLFrameBuffer *fb) 
{
	framebuffer = fb;
}

void FGLRenderer::Initialize(int width, int height)
{
	mScreenBuffers = new FGLRenderBuffers(gl_multisample);
	mSaveBuffers = new FGLRenderBuffers(0);
	mBuffers = mScreenBuffers;
	mPresentShader = new FPresentShader();
	mPresent3dCheckerShader = new FPresent3DCheckerShader();
	mPresent3dColumnShader = new FPresent3DColumnShader();
	mPresent3dRowShader = new FPresent3DRowShader();

	//glGenQueries(1, &PortalQueryObject);

	// needed for the core profile, because someone decided it was a good idea to remove the default VAO.
	glGenVertexArrays(1, &mVAOID);
	glBindVertexArray(mVAOID);
	FGLDebug::LabelObject(GL_VERTEX_ARRAY, mVAOID, "FGLRenderer.mVAOID");

	mFBID = 0;
	mOldFBID = 0;

	//mShaderManager = new FShaderManager;
	//mSamplerManager = new FSamplerManager;
}

FGLRenderer::~FGLRenderer() 
{
	//FlushModels();
	//TexMan.FlushAll();
	//if (mShaderManager != nullptr) delete mShaderManager;
	//if (mSamplerManager != nullptr) delete mSamplerManager;
	if (mFBID != 0) glDeleteFramebuffers(1, &mFBID);
	if (mVAOID != 0)
	{
		glBindVertexArray(0);
		glDeleteVertexArrays(1, &mVAOID);
	}
	//if (PortalQueryObject != 0) glDeleteQueries(1, &PortalQueryObject);

	//if (swdrawer) delete swdrawer;
	if (mBuffers) delete mBuffers;
	if (mSaveBuffers) delete mSaveBuffers;
	if (mPresentShader) delete mPresentShader;
	if (mPresent3dCheckerShader) delete mPresent3dCheckerShader;
	if (mPresent3dColumnShader) delete mPresent3dColumnShader;
	if (mPresent3dRowShader) delete mPresent3dRowShader;
}

//===========================================================================
// 
//
//
//===========================================================================

bool FGLRenderer::StartOffscreen()
{
	bool firstBind = (mFBID == 0);
	if (mFBID == 0)
		glGenFramebuffers(1, &mFBID);
	glGetIntegerv(GL_FRAMEBUFFER_BINDING, &mOldFBID);
	glBindFramebuffer(GL_FRAMEBUFFER, mFBID);
	if (firstBind)
		FGLDebug::LabelObject(GL_FRAMEBUFFER, mFBID, "OffscreenFB");
	return true;
}

//===========================================================================
// 
//
//
//===========================================================================

void FGLRenderer::EndOffscreen()
{
	glBindFramebuffer(GL_FRAMEBUFFER, mOldFBID); 
}

//===========================================================================
//
//
//
//===========================================================================

void FGLRenderer::BindToFrameBuffer(FTexture *mat)
{
	auto pBaseLayer = mat->SystemTextures.GetHardwareTexture(0, false);
	auto BaseLayer = pBaseLayer ? (::FHardwareTexture*)pBaseLayer : nullptr;

	if (BaseLayer == nullptr)
	{
		// must create the hardware texture first
		BaseLayer = new ::FHardwareTexture;
		BaseLayer->CreateTexture(mat->GetTexelWidth()*4, mat->GetTexelHeight()*4, ::FHardwareTexture::TrueColor, false);
		mat->SystemTextures.AddHardwareTexture(0, false, BaseLayer);
	}
	BaseLayer->BindToFrameBuffer(mat->GetTexelWidth()*4, mat->GetTexelHeight()*4);
}

//===========================================================================
//
// Render the view to a savegame picture
//
//===========================================================================

void FGLRenderer::WriteSavePic ( FileWriter *file, int width, int height)
{
    IntRect bounds;
    bounds.left = 0;
    bounds.top = 0;
    bounds.width = width;
    bounds.height = height;
    
    // we must be sure the GPU finished reading from the buffer before we fill it with new data.
    glFinish();
    
    // Switch to render buffers dimensioned for the savepic
    mBuffers = mSaveBuffers;
	mBuffers->BindSceneFB(false);
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
	CopyToBackbuffer(&bounds, false);
    
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
    mBuffers = mScreenBuffers;
	bool useSSAO = (gl_ssao != 0);
	mBuffers->BindSceneFB(useSSAO);
}


//===========================================================================
//
//
//
//===========================================================================

void FGLRenderer::BeginFrame()
{
	mScreenBuffers->Setup(screen->mScreenViewport.width, screen->mScreenViewport.height, screen->mSceneViewport.width, screen->mSceneViewport.height);
	mSaveBuffers->Setup(240, 180, 240, 180);
}

}
