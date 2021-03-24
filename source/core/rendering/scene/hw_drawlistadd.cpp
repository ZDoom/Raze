// 
//---------------------------------------------------------------------------
//
// Copyright(C) 2000-2016 Christoph Oelckers
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

#include "hw_dynlightdata.h"
#include "hw_cvars.h"
#include "hw_lightbuffer.h"
#include "hw_drawstructs.h"
#include "hw_drawinfo.h"
#include "hw_material.h"
#include "build.h"
#include "polymost.h"
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
		bool masked = wall->type != RENDERWALL_M2S ? false : (wall->texture && wall->texture->isMasked());
		int list;

		if (wall->flags & HWWall::HWF_SKYHACK && wall->type == RENDERWALL_M2S)
		{
			list = GLDL_MASKEDWALLSOFS;
		}
		else
		{
			list = masked ? GLDL_MASKEDWALLS : GLDL_PLAINWALLS;
		}
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

#if 0
	bool hasDecals = newwall->seg->sidedef && newwall->seg->sidedef->AttachedDecals;
	if (hasDecals && Level->HasDynamicLights && !isFullbrightScene())
	{
		newwall->SetupLights(this, lightdata);
	}
	newwall->ProcessDecals(this);
#endif
	newwall->dynlightindex = -1; // the environment map should not be affected by lights - only the decals.
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
	int list = GLDL_PLAINFLATS;

#if 0
	if (flat->renderstyle != STYLE_Translucent || flat->alpha < 1.f - FLT_EPSILON)
	{
		// translucent portals go into the translucent border list.
		list = GLDL_TRANSLUCENTBORDER;
	}
	else if (flat->texture->GetTranslucency())
	{
		/*
		if (flat->stack)
		{
			list = GLDL_TRANSLUCENTBORDER;
		}
		else
		*/
		{
			list = GLDL_PLAINFLATS;
		}
	}
	else //if (flat->hacktype != SSRF_FLOODHACK) // The flood hack may later need different treatment but with the current setup can go into the existing render list.
	{
		bool masked = flat->texture->isMasked() && flat->stack;
		list = masked ? GLDL_MASKEDFLATS : GLDL_PLAINFLATS;
	}
#endif
	auto newflat = drawlists[list].NewFlat();
	*newflat = *flat;
}


//==========================================================================
//
// 
//
//==========================================================================
void HWDrawInfo::AddSprite(HWSprite *sprite, bool translucent)
{
#if 0
	int list;
	// [BB] Allow models to be drawn in the GLDL_TRANSLUCENT pass.
	if (translucent || ( (!spriteIsModelOrVoxel(sprite->actor) && !(sprite->actor->cstat & CSTAT_SPRITE_ALIGNMENT_MASK))))
	{
		list = GLDL_TRANSLUCENT;
	}
	else
	{
		list = GLDL_MODELS;
	}

	auto newsprt = drawlists[list].NewSprite();
	*newsprt = *sprite;
#endif
}

