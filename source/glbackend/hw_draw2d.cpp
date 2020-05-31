/*
** hw_draw2d.cpp
** 2d drawer Renderer interface
**
**---------------------------------------------------------------------------
** Copyright 2018-2019 Christoph Oelckers
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


#include "cmdlib.h"
#include "gl_buffers.h"
#include "v_2ddrawer.h"
#include "c_cvars.h"
#include "glbackend.h"
#include "v_draw.h"
#include "palette.h"
#include "flatvertices.h"
#include "build.h"
#include "v_video.h"
#include "hw_renderstate.h"

extern int16_t numshades;
extern TArray<VSMatrix> matrixArray;

//===========================================================================
// 
// Vertex buffer for 2D drawer
//
//===========================================================================

class F2DVertexBuffer
{
	IVertexBuffer *mVertexBuffer;
	IIndexBuffer *mIndexBuffer;


public:

	F2DVertexBuffer()
	{
		mVertexBuffer = new OpenGLRenderer::GLVertexBuffer();
		mIndexBuffer = new OpenGLRenderer::GLIndexBuffer();

		static const FVertexBufferAttribute format[] = {
			{ 0, VATTR_VERTEX, VFmt_Float3, (int)myoffsetof(F2DDrawer::TwoDVertex, x) },
			{ 0, VATTR_TEXCOORD, VFmt_Float2, (int)myoffsetof(F2DDrawer::TwoDVertex, u) },
			{ 0, VATTR_COLOR, VFmt_Byte4, (int)myoffsetof(F2DDrawer::TwoDVertex, color0) }
		};
		mVertexBuffer->SetFormat(1, 3, sizeof(F2DDrawer::TwoDVertex), format);
	}
	~F2DVertexBuffer()
	{
		delete mIndexBuffer;
		delete mVertexBuffer;
	}

	void UploadData(F2DDrawer::TwoDVertex *vertices, int vertcount, int *indices, int indexcount)
	{
		mVertexBuffer->SetData(vertcount * sizeof(*vertices), vertices, false);
		mIndexBuffer->SetData(indexcount * sizeof(unsigned int), indices, false);
	}

	std::pair<IVertexBuffer *, IIndexBuffer *> GetBufferObjects() const
	{
		return std::make_pair(mVertexBuffer, mIndexBuffer);
	}
};

//===========================================================================
// 
// Draws the 2D stuff. This is the version for OpenGL 3 and later.
//
//===========================================================================
void polymost_dorotatesprite(int32_t sx, int32_t sy, int32_t z, int16_t a, int16_t picnum, int8_t dashade, uint8_t dapalnum, int32_t dastat, uint8_t daalpha, uint8_t dablend, int32_t cx1, int32_t cy1, int32_t cx2, int32_t cy2, int32_t uniqid);

void GLInstance::Draw2D(F2DDrawer *drawer)
{
	VSMatrix mat(0);
	SetIdentityMatrix(Matrix_View);
	SetIdentityMatrix(Matrix_Model);
	SetIdentityMatrix(Matrix_Detail);
	mat.ortho(0, xdim, ydim, 0, -1, 1);
	SetMatrix(Matrix_Projection, mat.get());
	SetViewport(0, 0, xdim, ydim);
	EnableDepthTest(false);
	EnableMultisampling(false);
	EnableBlend(true);
	EnableAlphaTest(true);
	SetRenderStyle(LegacyRenderStyles[STYLE_Translucent]);

	auto &vertices = drawer->mVertices;
	auto &indices = drawer->mIndices;
	auto &commands = drawer->mData;

	if (commands.Size() == 0)
	{
		return;
	}

	if (drawer->mIsFirstPass)
	{
		for (auto &v : vertices)
		{
			// Change from BGRA to RGBA
			std::swap(v.color0.r, v.color0.b);
		}
	}
	F2DVertexBuffer vb;
	vb.UploadData(&vertices[0], vertices.Size(), &indices[0], indices.Size());
	assert(vb.GetBufferObjects().first && vb.GetBufferObjects().second);
	SetVertexBuffer(vb.GetBufferObjects().first, 0, 0);
	SetIndexBuffer(vb.GetBufferObjects().second);
	SetFadeDisable(true);

	for(auto &cmd : commands)
	{


		int gltrans = -1;

		SetRenderStyle(cmd.mRenderStyle);
		//state.EnableBrightmap(!(cmd.mRenderStyle.Flags & STYLEF_ColorIsFixed));
		//state.SetTextureMode(cmd.mDrawMode);

		if (cmd.mFlags & F2DDrawer::DTF_Scissor)
		{
			// scissor test doesn't use the current viewport for the coordinates, so use real screen coordinates
			// Note that the origin here is the lower left corner!
			int sciX = screen->ScreenToWindowX(cmd.mScissor[0]);
			int sciY = screen->ScreenToWindowY(cmd.mScissor[3]);
			int sciW = screen->ScreenToWindowX(cmd.mScissor[2]) - sciX;
			int sciH = screen->ScreenToWindowY(cmd.mScissor[1]) - sciY;
			SetScissor(sciX, sciY, sciW, sciH);
		}
		else
		{
			DisableScissor();
		}

		auto tex = cmd.mTexture;
		if (tex != nullptr && tex->isValid())
		{
			auto tex = cmd.mTexture;

			SetFadeDisable(true);
			SetShade(0, numshades);

			SetTexture(TileFiles.GetTileIndex(tex), tex, cmd.mTranslationId, 0, cmd.mFlags & F2DDrawer::DTF_Wrap ? CLAMP_NONE : CLAMP_XY);
			EnableBlend(!(cmd.mRenderStyle.Flags & STYLEF_Alpha1));
			UseColorOnly(false);
		}
		else
		{
			UseColorOnly(true);
		}

		switch (cmd.mType)
		{
		case F2DDrawer::DrawTypeTriangles:
			DrawElement(DT_Triangles, cmd.mIndexIndex, cmd.mIndexCount, renderState);
			break;

		case F2DDrawer::DrawTypeLines:
			DrawElement(DT_Lines, cmd.mVertIndex, cmd.mVertCount, renderState);
			break;

		case F2DDrawer::DrawTypePoints:
			//Draw(DT_POINTS, cmd.mVertIndex, cmd.mVertCount);
			break;

		}
		/*
		state.SetObjectColor(0xffffffff);
		state.SetObjectColor2(0);
		state.SetAddColor(0);
		state.EnableTextureMatrix(false);
		state.SetEffect(EFF_NONE);
		*/

	}

	//state.SetRenderStyle(STYLE_Translucent);
	ClearBufferState();
	UseColorOnly(false);
	//state.EnableBrightmap(true);
	//state.SetTextureMode(TM_NORMAL);
	SetShade(0, numshades);
	SetFadeDisable(false);
	SetColor(1, 1, 1);
	DisableScissor();
	//drawer->mIsFirstPass = false;
	EnableBlend(true);
	EnableMultisampling(true);
	SetIdentityMatrix(Matrix_Projection);
	matrixArray.Resize(1);
	renderState.Apply(polymostShader, lastState);	// actually set the desired state before returning.
}


static int32_t tint_blood_r = 0, tint_blood_g = 0, tint_blood_b = 0;
extern PalEntry palfadergb;

void DrawFullscreenBlends()
{
	GLInterface.SetIdentityMatrix(Matrix_Projection);
	GLInterface.SetIdentityMatrix(Matrix_Model);
	GLInterface.SetIdentityMatrix(Matrix_View);

	GLInterface.EnableDepthTest(false);
	GLInterface.EnableAlphaTest(false);
	GLInterface.EnableBlend(true);
	GLInterface.UseColorOnly(true);

	if (palfadergb.a > 0)
	{
		// Todo: reroute to the 2D drawer
		GLInterface.SetRenderStyle(LegacyRenderStyles[STYLE_Translucent]);
		GLInterface.SetColorub(palfadergb.r, palfadergb.g, palfadergb.b, palfadergb.a);
		GLInterface.Draw(DT_TriangleStrip, FFlatVertexBuffer::PRESENT_INDEX, 4);
	}
	if (tint_blood_r | tint_blood_g | tint_blood_b)
	{
		GLInterface.SetRenderStyle(LegacyRenderStyles[STYLE_Add]);

		GLInterface.SetColorub(max(tint_blood_r, 0), max(tint_blood_g, 0), max(tint_blood_b, 0), 255);
		GLInterface.Draw(DT_TriangleStrip, FFlatVertexBuffer::PRESENT_INDEX, 4);

		GLInterface.SetRenderStyle(LegacyRenderStyles[STYLE_Subtract]);
		GLInterface.SetColorub(max(-tint_blood_r, 0), max(-tint_blood_g, 0), max(-tint_blood_b, 0), 255);
		GLInterface.Draw(DT_TriangleStrip, FFlatVertexBuffer::PRESENT_INDEX, 4);

		GLInterface.SetColorub(255, 255, 255, 255);
		GLInterface.SetRenderStyle(LegacyRenderStyles[STYLE_Translucent]);
	}
	GLInterface.DoDraw();
	GLInterface.UseColorOnly(false);

}

void DrawRateStuff();

void Draw2D(F2DDrawer* drawer, FRenderState& state)
{
	::DrawFullscreenBlends();
	DrawRateStuff();
	GLInterface.Draw2D(twod);
}

void videoTintBlood(int32_t r, int32_t g, int32_t b)
{
	tint_blood_r = r;
	tint_blood_g = g;
	tint_blood_b = b;
}
