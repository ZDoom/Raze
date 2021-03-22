// 
//---------------------------------------------------------------------------
//
// Copyright(C) 2003-2016 Christoph Oelckers
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

#include "filesystem.h"
#include "hw_skydome.h"
#include "hw_portal.h"
#include "hw_renderstate.h"
#include "skyboxtexture.h"

//-----------------------------------------------------------------------------
//
//
//
//-----------------------------------------------------------------------------
void HWSkyPortal::DrawContents(HWDrawInfo *di, FRenderState &state)
{
	bool drawBoth = false;
	auto &vp = di->Viewpoint;

	if (di->isSoftwareLighting())
	{
		//di->SetFallbackLightMode();
		state.SetNoSoftLightLevel();
	}

	state.ResetColor();
	state.EnableFog(false);
	state.AlphaFunc(Alpha_GEqual, 0.f);
	state.SetRenderStyle(STYLE_Translucent);
	bool oldClamp = state.SetDepthClamp(true);

	di->SetupView(state, 0, 0, 0, !!(mState->MirrorFlag & 1), !!(mState->PlaneMirrorFlag & 1));

	state.SetVertexBuffer(vertexBuffer);
	auto skybox = origin->texture ? dynamic_cast<FSkyBox*>(origin->texture->GetTexture()) : nullptr;
	if (skybox)
	{
		vertexBuffer->RenderBox(state, skybox, origin->x_offset, false, /*di->Level->info->pixelstretch*/1, { 0, 0, 1 }, { 0, 0, 1 });
	}
	else
	{
		state.SetTextureMode(TM_OPAQUE);
		vertexBuffer->RenderDome(state, origin->texture, origin->x_offset, origin->y_offset, false, FSkyVertexBuffer::SKYMODE_MAINLAYER, false);
		state.SetTextureMode(TM_NORMAL);
	}
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
}

const char *HWSkyPortal::GetName() { return "Sky"; }
