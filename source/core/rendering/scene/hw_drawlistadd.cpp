// 
//---------------------------------------------------------------------------
//
// Copyright(C) 2000-2016 Christoph Oelckers
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

#include "hw_dynlightdata.h"
#include "hw_cvars.h"
#include "hw_lightbuffer.h"
#include "hw_drawstructs.h"
#include "hw_drawinfo.h"
#include "hw_material.h"
#include "build.h"
#include "gamefuncs.h"

EXTERN_CVAR(Bool, gl_seamless)

//==========================================================================
//
// 
//
//==========================================================================

void HWDrawInfo::AddWall(HWWall *wall)
{
	if (wall->flags & HWWall::HWF_TRANSLUCENT)
	{
		auto newwall = drawlists[GLDL_TRANSLUCENT].NewWall();
		*newwall = *wall;
	}
	else
	{
		int list;

		if (wall->type != RENDERWALL_M2S) list = GLDL_PLAINWALLS;
		else if (wall->Sprite == nullptr) list = GLDL_MASKEDWALLS;
		else if (wall->glseg.x1 == wall->glseg.x2) list = GLDL_MASKEDWALLSV;
		else if (wall->glseg.y1 == wall->glseg.y2) list = GLDL_MASKEDWALLSH;
		else list = wall->walldist? GLDL_MASKEDWALLSD : GLDL_MASKEDWALLSS;
		auto newwall = drawlists[list].NewWall();
		*newwall = *wall;
	}
}

//==========================================================================
//
// 
//
//==========================================================================

void HWDrawInfo::AddMirrorSurface(HWWall *w)
{
	w->type = RENDERWALL_MIRRORSURFACE;
	auto newwall = drawlists[GLDL_TRANSLUCENTBORDER].NewWall();
	*newwall = *w;

	// Invalidate vertices to allow setting of texture coordinates
	newwall->vertcount = 0;

	FVector3 v = newwall->glseg.Normal();
	auto tcs = newwall->tcs;
	tcs[HWWall::LOLFT].u = tcs[HWWall::LORGT].u = tcs[HWWall::UPLFT].u = tcs[HWWall::UPRGT].u = v.X;
	tcs[HWWall::LOLFT].v = tcs[HWWall::LORGT].v = tcs[HWWall::UPLFT].v = tcs[HWWall::UPRGT].v = v.Z;
	newwall->MakeVertices(this, false);
	newwall->dynlightindex = -1; // the environment map should not be affected by lights.
}

//==========================================================================
//
// FDrawInfo::AddFlat
//
// Checks texture, lighting and translucency settings and puts this
// plane in the appropriate render list.
//
//==========================================================================

void HWDrawInfo::AddFlat(HWFlat *flat)
{
	int list;;
	bool slopespr = false;

	if (flat->RenderStyle != LegacyRenderStyles[STYLE_Translucent] || flat->alpha < 1.f - FLT_EPSILON || checkTranslucentReplacement(flat->texture->GetID(), flat->palette))
	{
		// translucent portals go into the translucent border list.
		list = flat->Sprite? GLDL_TRANSLUCENT : GLDL_TRANSLUCENTBORDER;
		slopespr = !!(flat->Sprite);//&& flat->Sprite->clipdist& TSPR_SLOPESPRITE);
	}
	else
	{
		list = flat->Sprite ? ((flat->Sprite->clipdist & TSPR_SLOPESPRITE)? GLDL_MASKEDSLOPEFLATS : GLDL_MASKEDFLATS) : GLDL_PLAINFLATS;
	}
	auto newflat = drawlists[list].NewFlat(slopespr);
	*newflat = *flat;
}


//==========================================================================
//
// 
//
//==========================================================================
void HWDrawInfo::AddSprite(HWSprite *sprite, bool translucent)
{
	int list;
	if (translucent || sprite->modelframe == 0) list = GLDL_TRANSLUCENT;
	else list = GLDL_MODELS;

	auto newsprt = drawlists[list].NewSprite();
	*newsprt = *sprite;
}

