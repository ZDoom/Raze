// 
//---------------------------------------------------------------------------
//
// Copyright(C) 2018 Christoph Oelckers
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
** 2d drawer
** Renderer interface
**
*/

#include "cmdlib.h"
#include "gl_buffers.h"
#include "v_2ddrawer.h"
#include "c_cvars.h"
#include "glbackend.h"
#include "v_draw.h"
#include "palette.h"

extern int16_t numshades;
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
	SetMatrix(Matrix_View, mat.get());
	SetMatrix(Matrix_ModelView, mat.get());
	SetMatrix(Matrix_Detail, mat.get());
	mat.ortho(0, xdim, ydim, 0, -1, 1);
	SetMatrix(Matrix_Projection, mat.get());
	SetViewport(0, 0, xdim, ydim);
	EnableDepthTest(false);
	EnableMultisampling(false);
	EnableBlend(true);
	EnableAlphaTest(true);
	SetBlendFunc(STYLEALPHA_Src, STYLEALPHA_InvSrc);

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

		SetBlendFunc(cmd.mRenderStyle.SrcAlpha, cmd.mRenderStyle.DestAlpha);
		//state.SetRenderStyle(cmd.mRenderStyle);
		//state.EnableBrightmap(!(cmd.mRenderStyle.Flags & STYLEF_ColorIsFixed));
		//state.SetTextureMode(cmd.mDrawMode);

		int sciX, sciY, sciW, sciH;
		if (cmd.mFlags & F2DDrawer::DTF_Scissor)
		{
			// scissor test doesn't use the current viewport for the coordinates, so use real screen coordinates
			// Note that the origin here is the lower left corner!
			sciX = screen->ScreenToWindowX(cmd.mScissor[0]);
			sciY = screen->ScreenToWindowY(cmd.mScissor[3]);
			sciW = screen->ScreenToWindowX(cmd.mScissor[2]) - sciX;
			sciH = screen->ScreenToWindowY(cmd.mScissor[1]) - sciY;
			SetScissor(sciX, sciY, sciW, sciH);
		}
		else
		{
			sciX = sciY = sciW = sciH = -1;
			DisableScissor();
		}

		//state.SetFog(cmd.mColor1, 0);
		SetColor(1, 1, 1);
		//state.SetColor(1, 1, 1, 1, cmd.mDesaturate); 

		if (cmd.mTexture != nullptr)
		{
			auto tex = cmd.mTexture;
			if (cmd.mType == F2DDrawer::DrawTypeRotateSprite)
			{
				// todo: Set up hictinting. (broken as the feature is...)
				SetShade(cmd.mRemapIndex >> 16, numshades);
				SetFadeDisable(false);
				SetTexture(0, tex, cmd.mRemapIndex & 0xffff, 4/*DAMETH_CLAMPED*/, cmd.mFlags & F2DDrawer::DTF_Wrap ? SamplerRepeat : SamplerClampXY);
				EnableBlend(!(cmd.mRenderStyle.Flags & STYLEF_Alpha1));
			}
			else
			{
				SetFadeDisable(true);
				SetShade(0, numshades);
				SetNamedTexture(cmd.mTexture, cmd.mRemapIndex, cmd.mFlags & F2DDrawer::DTF_Wrap ? SamplerRepeat : SamplerClampXY);
				EnableBlend(true);
			}
			UseColorOnly(false);
		}
		else
		{
			UseColorOnly(true);
		}

		switch (cmd.mType)
		{
		case F2DDrawer::DrawTypeTriangles:
		case F2DDrawer::DrawTypeRotateSprite:
			Draw(DT_TRIANGLES, cmd.mIndexIndex, cmd.mIndexCount);
			break;

		case F2DDrawer::DrawTypeLines:
			//Draw(DT_LINES, cmd.mVertIndex, cmd.mVertCount);
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
	//state.SetScissor(-1, -1, -1, -1);

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
}


void fullscreen_tint_gl(PalEntry pe)
{
	// Todo: reroute to the 2D drawer
	auto oldproj = GLInterface.GetMatrix(Matrix_Projection);
	auto oldmv = GLInterface.GetMatrix(Matrix_ModelView);
	VSMatrix identity(0);
	GLInterface.SetMatrix(Matrix_Projection, &identity);
	GLInterface.SetMatrix(Matrix_ModelView, &identity);

	GLInterface.EnableDepthTest(false);
	GLInterface.EnableAlphaTest(false);

	GLInterface.SetBlendFunc(STYLEALPHA_Src, STYLEALPHA_InvSrc);
	GLInterface.EnableBlend(true);
	GLInterface.SetColorub (pe.r, pe.g, pe.b, pe.a);

	GLInterface.UseColorOnly(true);

	auto data = GLInterface.AllocVertices(3);
	auto vt = data.second;
	vt[0].Set(-2.5f, 1.f);
	vt[1].Set(2.5f, 1.f);
	vt[2].Set(.0f, -2.5f);
	GLInterface.Draw(DT_TRIANGLES, data.first, 3);
	GLInterface.UseColorOnly(false);

	GLInterface.SetMatrix(Matrix_Projection, &oldproj);
	GLInterface.SetMatrix(Matrix_ModelView, &oldmv);
}

void fullscreen_tint_gl_blood(int tint_blood_r, int tint_blood_g, int tint_blood_b)
{
	if (!(tint_blood_r | tint_blood_g | tint_blood_b))
		return;
	auto oldproj = GLInterface.GetMatrix(Matrix_Projection);
	auto oldmv = GLInterface.GetMatrix(Matrix_ModelView);
	VSMatrix identity(0);
	GLInterface.SetMatrix(Matrix_Projection, &identity);
	GLInterface.SetMatrix(Matrix_ModelView, &identity);


	GLInterface.EnableDepthTest(false);
	GLInterface.EnableAlphaTest(false);

	GLInterface.SetBlendFunc(STYLEALPHA_One, STYLEALPHA_One);
	GLInterface.EnableBlend(true);

	GLInterface.UseColorOnly(true);
	GLInterface.SetColorub(max(tint_blood_r, 0), max(tint_blood_g, 0), max(tint_blood_b, 0), 255);
	auto data = GLInterface.AllocVertices(3);
	auto vt = data.second;
	vt[0].Set(-2.5f, 1.f);
	vt[1].Set(2.5f, 1.f);
	vt[2].Set(.0f, -2.5f);
	GLInterface.Draw(DT_TRIANGLES, data.first, 3);
	GLInterface.SetBlendOp(STYLEOP_RevSub);
	GLInterface.SetColorub(max(-tint_blood_r, 0), max(-tint_blood_g, 0), max(-tint_blood_b, 0), 255);
	data = GLInterface.AllocVertices(3);
	vt = data.second;
	vt[0].Set(-2.5f, 1.f);
	vt[1].Set(2.5f, 1.f);
	vt[2].Set(.0f, -2.5f);
	GLInterface.Draw(DT_TRIANGLES, data.first, 3);
	GLInterface.SetBlendOp(STYLEOP_Add);
	GLInterface.SetColorub(255, 255, 255, 255);
	GLInterface.SetBlendFunc(STYLEALPHA_Src, STYLEALPHA_InvSrc);
	GLInterface.UseColorOnly(false);

	GLInterface.SetMatrix(Matrix_Projection, &oldproj);
	GLInterface.SetMatrix(Matrix_ModelView, &oldmv);

}

static int32_t tint_blood_r = 0, tint_blood_g = 0, tint_blood_b = 0;
extern palette_t palfadergb;
extern char palfadedelta ;

void DrawFullscreenBlends()
{
	if (palfadedelta)	fullscreen_tint_gl(PalEntry(palfadedelta, palfadergb.r, palfadergb.g, palfadergb.b));
	fullscreen_tint_gl_blood(tint_blood_r, tint_blood_g, tint_blood_b);
}


void videoTintBlood(int32_t r, int32_t g, int32_t b)
{
	tint_blood_r = r;
	tint_blood_g = g;
	tint_blood_b = b;
}
