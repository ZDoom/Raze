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

void Draw2D(F2DDrawer *drawer, FRenderState &state)
{
	VSMatrix mat(0);
	GLInterface.SetMatrix(Matrix_View, mat.get());
	GLInterface.SetMatrix(Matrix_ModelView, mat.get());
	GLInterface.SetMatrix(Matrix_Detail, mat.get());
	mat.ortho(0, xdim, ydim, 0, -1, 1);
	GLInterface.SetMatrix(Matrix_Projection, mat.get());
	GLInterface.SetViewport(0, 0, xdim, ydim);
	GLInterface.EnableDepthTest(false);
	GLInterface.EnableMultisampling(false);

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
	GLInterface.SetVertexBuffer(vb.GetBufferObjects().first, 0, 0);
	GLInterface.SetIndexBuffer(vb.GetBufferObjects().second);
	GLInterface.SetFadeDisable(true);

	for(auto &cmd : commands)
	{

		int gltrans = -1;
		//state.SetRenderStyle(cmd.mRenderStyle);
		//state.EnableBrightmap(!(cmd.mRenderStyle.Flags & STYLEF_ColorIsFixed));
		//state.SetTextureMode(cmd.mDrawMode);

		int sciX, sciY, sciW, sciH;
		if (cmd.mFlags & F2DDrawer::DTF_Scissor)
		{
			// scissor test doesn't use the current viewport for the coordinates, so use real screen coordinates
			// Note that the origin here is the lower left corner!
			sciX = /*screen->ScreenToWindowX*/(cmd.mScissor[0]);
			sciY = /*screen->ScreenToWindowY*/(cmd.mScissor[3]);
			sciW = /*screen->ScreenToWindowX*/(cmd.mScissor[2]) - sciX;
			sciH = /*screen->ScreenToWindowY*/(cmd.mScissor[1]) - sciY;
		}
		else
		{
			sciX = sciY = sciW = sciH = -1;
		}
		//GLInterface.SetScissor(sciX, sciY, sciW, sciH);

		//state.SetFog(cmd.mColor1, 0);
		GLInterface.SetColor(1, 1, 1);
		//state.SetColor(1, 1, 1, 1, cmd.mDesaturate); 

		GLInterface.SetAlphaThreshold(0.0f);

		if (cmd.mTexture != nullptr)
		{
			auto tex = cmd.mTexture;
			GLInterface.SetNamedTexture(cmd.mTexture, cmd.mRemapIndex, cmd.mFlags & F2DDrawer::DTF_Wrap ? SamplerRepeat : SamplerClampXY);
			GLInterface.UseColorOnly(false);
		}
		else
		{
			GLInterface.UseColorOnly(true);
		}

		switch (cmd.mType)
		{
		case F2DDrawer::DrawTypeTriangles:
			GLInterface.Draw(DT_TRIANGLES, cmd.mIndexIndex, cmd.mIndexCount);
			break;

		case F2DDrawer::DrawTypeLines:
			GLInterface.Draw(DT_LINES, cmd.mVertIndex, cmd.mVertCount);
			break;

		case F2DDrawer::DrawTypePoints:
			//GLInterface.Draw(DT_POINTS, cmd.mVertIndex, cmd.mVertCount);
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
	GLInterface.SetVertexBuffer(nullptr, 0, 0);
	GLInterface.SetIndexBuffer(nullptr);
	GLInterface.UseColorOnly(false);
	//state.EnableBrightmap(true);
	//state.SetTextureMode(TM_NORMAL);
	GLInterface.SetFadeDisable(false);
	GLInterface.SetColor(1, 1, 1);
	//drawer->mIsFirstPass = false;
}
