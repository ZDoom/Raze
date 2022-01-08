// 
//---------------------------------------------------------------------------
//
// Copyright(C) 2003-2016 Christoph Oelckers
// All rights reserved.
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 2 of the License, or
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

#include "filesystem.h"
#include "hw_skydome.h"
#include "hw_portal.h"
#include "hw_renderstate.h"
#include "skyboxtexture.h"

CVAR(Float, skyoffsettest, 0, 0)

//-----------------------------------------------------------------------------
//
//
//
//-----------------------------------------------------------------------------
void HWSkyPortal::DrawContents(HWDrawInfo *di, FRenderState &state)
{
	int indexed = hw_int_useindexedcolortextures;
	hw_int_useindexedcolortextures = false; // this code does not work with indexed textures.

	state.SetNoSoftLightLevel();

	state.ResetColor();
	state.EnableFog(false);
	state.AlphaFunc(Alpha_GEqual, 0.f);
	state.SetRenderStyle(STYLE_Translucent);
	bool oldClamp = state.SetDepthClamp(true);

	di->SetupView(state, 0, 0, 0, !!(mState->MirrorFlag & 1), !!(mState->PlaneMirrorFlag & 1));

	state.SetVertexBuffer(vertexBuffer);
	state.SetTextureMode(TM_OPAQUE);
	auto skybox = origin->texture ? dynamic_cast<FSkyBox*>(origin->texture->GetTexture()) : nullptr;
	if (skybox)
	{
		vertexBuffer->RenderBox(state, skybox, origin->x_offset, false, /*di->Level->info->pixelstretch*/1, { 0, 0, 1 }, { 0, 0, 1 });
	}
	else if (!origin->cloudy)
	{
		auto tex = origin->texture;
		float texh = tex->GetDisplayHeight();
		auto& modelMatrix = state.mModelMatrix;
		auto& textureMatrix = state.mTextureMatrix;
		auto texskyoffset = tex->GetSkyOffset() + skyoffsettest;
		if (!(g_gameType & GAMEFLAG_PSEXHUMED)) texskyoffset += origin->y_offset;

		float repeat_fac = 1;
		if (texh <= 256)
		{
			repeat_fac = 336.f / texh;
			texh = 336;
		}
		modelMatrix.loadIdentity();
		modelMatrix.rotate(-180.0f + origin->x_offset, 0.f, 1.f, 0.f);
		modelMatrix.translate(0.f, -40 + texskyoffset * skyoffsetfactor, 0.f);
		modelMatrix.scale(1.f, texh / 400.f, 1.f);
		textureMatrix.loadIdentity();
		state.EnableTextureMatrix(true);
		textureMatrix.scale(1.f, repeat_fac, 1.f);
		vertexBuffer->RenderDome(state, origin->texture, FSkyVertexBuffer::SKYMODE_MAINLAYER, true);
		state.EnableTextureMatrix(false);
	}
	else
	{
		vertexBuffer->RenderDome(state, origin->texture, -origin->x_offset, origin->y_offset, false, FSkyVertexBuffer::SKYMODE_MAINLAYER, true);
	}
	state.SetTextureMode(TM_NORMAL);
	if (origin->fadecolor & 0xffffff)
	{
		PalEntry FadeColor = origin->fadecolor;
		state.EnableTexture(false);
		state.SetObjectColor(FadeColor);
		state.Draw(DT_Triangles, 0, 12);
		state.EnableTexture(true);
		state.SetObjectColor(0xffffffff);
	}

	//di->lightmode = oldlightmode;
	state.SetDepthClamp(oldClamp);
	hw_int_useindexedcolortextures = indexed;
 }

const char *HWSkyPortal::GetName() { return "Sky"; }
