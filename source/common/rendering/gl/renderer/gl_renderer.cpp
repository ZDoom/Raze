// 
//---------------------------------------------------------------------------
//
// Copyright(C) 2005-2016 Christoph Oelckers
// All rights reserved.
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public License
// along with this program.  If not, see http://www.gnu.org/licenses/
//
//--------------------------------------------------------------------------
//
/*
** gl1_renderer.cpp
** Renderer interface
**
*/

#include "gl_load/gl_system.h"
#include "files.h"
#include "v_video.h"
#include "m_png.h"
#include "filesystem.h"
#include "i_time.h"
#include "cmdlib.h"
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

EXTERN_CVAR(Int, screenblocks)
EXTERN_CVAR(Bool, cl_capfps)

extern bool NoInterpolateView;

void DoWriteSavePic(FileWriter *file, ESSType ssformat, uint8_t *scr, int width, int height, sector_t *viewsector, bool upsidedown);

namespace OpenGLRenderer
{

	FGLRenderer* GLRenderer;
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
	mScreenBuffers = new FGLRenderBuffers();
	mSaveBuffers = new FGLRenderBuffers();
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

void FGLRenderer::BindToFrameBuffer(FMaterial *mat)
{
#if 0
	auto BaseLayer = static_cast<FHardwareTexture*>(mat->GetLayer(0, 0));

	if (BaseLayer == nullptr)
	{
		// must create the hardware texture first
		BaseLayer->BindOrCreate(mat->sourcetex, 0, 0, 0, 0);
		FHardwareTexture::Unbind(0);
		gl_RenderState.ClearLastMaterial();
	}
	BaseLayer->BindToFrameBuffer(mat->GetWidth(), mat->GetHeight());
#endif
}

//===========================================================================
//
// Render the view to a savegame picture
//
//===========================================================================

#if 0
void FGLRenderer::WriteSavePic (player_t *player, FileWriter *file, int width, int height)
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
    
	hw_ClearFakeFlat();
	gl_RenderState.SetVertexBuffer(screen->mVertexData);
	screen->mVertexData->Reset();
    screen->mLights->Clear();
	screen->mViewpoints->Clear();

    // This shouldn't overwrite the global viewpoint even for a short time.
    FRenderViewpoint savevp;
    sector_t *viewsector = RenderViewpoint(savevp, players[consoleplayer].camera, &bounds, r_viewpoint.FieldOfView.Degrees, 1.6f, 1.6f, true, false);
    glDisable(GL_STENCIL_TEST);
    gl_RenderState.SetNoSoftLightLevel();
    CopyToBackbuffer(&bounds, false);
    
    // strictly speaking not needed as the glReadPixels should block until the scene is rendered, but this is to safeguard against shitty drivers
    glFinish();
    
	int numpixels = width * height;
    uint8_t * scr = (uint8_t *)M_Malloc(numpixels * 3);
    glReadPixels(0,0,width, height,GL_RGB,GL_UNSIGNED_BYTE,scr);

	DoWriteSavePic(file, SS_RGB, scr, width, height, viewsector, true);
    M_Free(scr);
    
    // Switch back the screen render buffers
    screen->SetViewportRects(nullptr);
    mBuffers = mScreenBuffers;
}
#endif

//===========================================================================
//
//
//
//===========================================================================

void FGLRenderer::BeginFrame()
{
	mScreenBuffers->Setup(screen->mScreenViewport.width, screen->mScreenViewport.height, screen->mSceneViewport.width, screen->mSceneViewport.height);
	//mSaveBuffers->Setup(SAVEPICWIDTH, SAVEPICHEIGHT, SAVEPICWIDTH, SAVEPICHEIGHT);
}

}
