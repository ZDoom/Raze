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
/*
** gl_flat.cpp
** Flat processing
**
*/

#include "matrix.h"
#include "hw_dynlightdata.h"
#include "hw_cvars.h"
#include "hw_clock.h"
#include "hw_material.h"
#include "hw_drawinfo.h"
#include "flatvertices.h"
#include "hw_lightbuffer.h"
#include "hw_drawstructs.h"
#include "hw_renderstate.h"
#include "sectorgeometry.h"

#ifdef _DEBUG
CVAR(Int, gl_breaksec, -1, 0)
#endif

//==========================================================================
//
//
//
//==========================================================================
#if 0
void HWFlat::SetupLights(HWDrawInfo *di, FLightNode * node, FDynLightData &lightdata, int portalgroup)
{
	Plane p;

	lightdata.Clear();
	if (renderstyle == STYLE_Add && !di->Level->lightadditivesurfaces)
	{
		dynlightindex = -1;
		return;	// no lights on additively blended surfaces.
	}
	while (node)
	{
		FDynamicLight * light = node->lightsource;

		if (!light->IsActive())
		{
			node = node->nextLight;
			continue;
		}
		iter_dlightf++;

		// we must do the side check here because gl_GetLight needs the correct plane orientation
		// which we don't have for Legacy-style 3D-floors
		double planeh = plane.plane.ZatPoint(light->Pos);
		if ((planeh<light->Z() && ceiling) || (planeh>light->Z() && !ceiling))
		{
			node = node->nextLight;
			continue;
		}

		p.Set(plane.plane.Normal(), plane.plane.fD());
		draw_dlightf += GetLight(lightdata, portalgroup, p, light, false);
		node = node->nextLight;
	}

	dynlightindex = screen->mLights->UploadLights(lightdata);
}
#endif

//==========================================================================
//
//
//
//==========================================================================

void HWFlat::MakeVertices()
{
	if (vertcount > 0) return;
	bool canvas = texture->isHardwareCanvas();
	if (sprite == nullptr)
	{
		auto mesh = sectorGeometry.get(sec - sector, plane, geoofs);
		if (!mesh) return;
		auto ret = screen->mVertexData->AllocVertices(mesh->vertices.Size());
		auto vp = ret.first;
		float base = (plane == 0 ? sec->floorz : sec->ceilingz) * (1/-256.f);
		for (unsigned i = 0; i < mesh->vertices.Size(); i++)
		{
			auto& pt = mesh->vertices[i];
			auto& uv = mesh->texcoords[i];
			vp->SetVertex(pt.X, base + pt.Z, pt.Y);
			vp->SetTexCoord(uv.X, canvas? 1.f - uv.Y : uv.Y);
			vp++;
		}
		vertindex = ret.second;
		vertcount = mesh->vertices.Size();
	}
	else
	{
		vec2_t pos[4];
		GetFlatSpritePosition(sprite, sprite->pos.vec2, pos, true);

		auto ret = screen->mVertexData->AllocVertices(6);
		auto vp = ret.first;
		float x = !(sprite->cstat & CSTAT_SPRITE_XFLIP) ? 0.f : 1.f;
		float y = !(sprite->cstat & CSTAT_SPRITE_YFLIP) ? 0.f : 1.f;
		for (unsigned i = 0; i < 6; i++)
		{
			const static unsigned indices[] = { 0, 1, 2, 0, 2, 3 };
			int j = indices[i];
			vp->SetVertex(pos[j].x * (1 / 16.f), z, pos[j].y * (1 / -16.f));
			if (!canvas) vp->SetTexCoord(j == 1 || j == 2 ? 1.f - x : x, j == 2 || j == 3 ? 1.f - y : y);
			else vp->SetTexCoord(j == 1 || j == 2 ? 1.f - x : x, j == 2 || j == 3 ? y : 1.f - y);
			vp++;
		}
		vertindex = ret.second;
		vertcount = 6;
	}
}

//==========================================================================
//
//
//
//==========================================================================
void HWFlat::DrawFlat(HWDrawInfo *di, FRenderState &state, bool translucent)
{
	if (screen->BuffersArePersistent() && !sprite)
	{
		MakeVertices();
	}

#ifdef _DEBUG
	if (sec - sector == gl_breaksec)
	{
		int a = 0;
	}
#endif

	if (!sprite)
	{
		auto mesh = sectorGeometry.get(sec - sector, plane, geoofs);
		state.SetNormal(mesh->normal);
	}
	else
	{
		if (z < di->Viewpoint.Pos.Z) state.SetNormal({ 0,1,0 });
		else state.SetNormal({ 0, -1, 0 });
	}

	SetLightAndFog(state, fade, palette, shade, visibility, alpha);

	if (translucent)
	{
		if (RenderStyle.BlendOp != STYLEOP_Add)
		{
			state.EnableBrightmap(false);
		}

		state.SetRenderStyle(RenderStyle);
		state.SetTextureMode(RenderStyle);

		if (!texture || !checkTranslucentReplacement(texture->GetID(), palette)) state.AlphaFunc(Alpha_GEqual, texture->alphaThreshold);
		else state.AlphaFunc(Alpha_GEqual, 0.f);
	}
	state.SetMaterial(texture, UF_Texture, 0, sprite == nullptr? CLAMP_NONE : CLAMP_XY, TRANSLATION(Translation_Remap + curbasepal, palette), -1);

	state.SetLightIndex(dynlightindex);
	state.Draw(DT_Triangles, vertindex, vertcount);
	vertexcount += vertcount;

	if (translucent) state.SetRenderStyle(LegacyRenderStyles[STYLE_Translucent]);
	state.EnableBrightmap(true);

	//state.SetObjectColor(0xffffffff);
	//state.SetAddColor(0);
	//state.ApplyTextureManipulation(nullptr);
}

//==========================================================================
//
// HWFlat::PutFlat
//
// submit to the renderer
//
//==========================================================================

void HWFlat::PutFlat(HWDrawInfo *di, int whichplane)
{
	vertcount = 0;
	plane = whichplane;
	if (!screen->BuffersArePersistent() || sprite || di->ingeo)	// should be made static buffer content later (when the logic is working)
	{
#if 0
		if (di->Level->HasDynamicLights && texture != nullptr && !di->isFullbrightScene() && !(hacktype & (SSRF_PLANEHACK | SSRF_FLOODHACK)))
		{
			SetupLights(di, section->lighthead, lightdata, sector->PortalGroup);
		}
#endif
		MakeVertices();
	}
	di->AddFlat(this);
	rendered_flats++;
}

//==========================================================================
//
// Process a sector's flats for rendering
// This function is only called once per sector.
// Subsequent subsectors are just quickly added to the ss_renderflags array
//
//==========================================================================

void HWFlat::ProcessSector(HWDrawInfo *di, sectortype * frontsector, int which)
{
#ifdef _DEBUG
	if (frontsector - sector == gl_breaksec)
	{
		int a = 0;
	}
#endif

	dynlightindex = -1;

    const auto &vp = di->Viewpoint;

	float florz, ceilz;
	PlanesAtPoint(frontsector, vp.Pos.X * 16.f, vp.Pos.Y * -16.f, &ceilz, &florz);

	fade = lookups.getFade(frontsector->floorpal);	// fog is per sector.
	visibility = sectorVisibility(frontsector);
	sec = frontsector;
	sprite = nullptr;
	geoofs = di->geoofs;

	//
	//
	//
	// do floors
	//
	//
	//
	if ((which & SSRF_RENDERFLOOR) && !(frontsector->floorstat & CSTAT_SECTOR_SKY) && florz <= vp.Pos.Z)
	{
		// process the original floor first.

		shade = clamp(frontsector->floorshade, 0, numshades-1);
		palette = frontsector->floorpal;
		stack = frontsector->portalflags == PORTAL_SECTOR_FLOOR || frontsector->portalflags == PORTAL_SECTOR_FLOOR_REFLECT;

		if (stack && (frontsector->floorstat & CSTAT_SECTOR_METHOD))
		{
			RenderStyle = GetRenderStyle(0, !!(frontsector->floorstat & CSTAT_SECTOR_TRANS_INVERT));
			alpha = GetAlphaFromBlend((frontsector->floorstat & CSTAT_SECTOR_TRANS_INVERT) ? DAMETH_TRANS2 : DAMETH_TRANS1, 0);
		}
		else
		{
			RenderStyle = STYLE_Translucent;
			alpha = 1.f;
		}

		if (alpha != 0.f)
		{
			int tilenum = frontsector->floorpicnum;
			setgotpic(tilenum);
			tileUpdatePicnum(&tilenum, tilenum, 0);
			texture = tileGetTexture(tilenum);
			if (texture && texture->isValid())
			{
				//iboindex = frontsector->iboindex[sector_t::floor];
				PutFlat(di, 0);
			}
		}
	}

	//
	//
	//
	// do ceilings
	//
	// 
	//
	if ((which & SSRF_RENDERCEILING) && !(frontsector->ceilingstat & CSTAT_SECTOR_SKY) && ceilz >= vp.Pos.Z)
	{
		// process the original ceiling first.

		shade = clamp(frontsector->ceilingshade, 0, numshades-1);
		palette = frontsector->ceilingpal;
		stack = frontsector->portalflags == PORTAL_SECTOR_CEILING || frontsector->portalflags == PORTAL_SECTOR_CEILING_REFLECT;

		if (stack && (frontsector->ceilingstat & CSTAT_SECTOR_METHOD))
		{
			RenderStyle = GetRenderStyle(0, !!(frontsector->ceilingstat & CSTAT_SECTOR_TRANS_INVERT));
			alpha = GetAlphaFromBlend((frontsector->ceilingstat & CSTAT_SECTOR_TRANS_INVERT) ? DAMETH_TRANS2 : DAMETH_TRANS1, 0);
		}
		else
		{
			RenderStyle = STYLE_Translucent;
			alpha = 1.f;
		}


		if (alpha != 0.f)
		{
			//iboindex = frontsector->iboindex[sector_t::ceiling];

			int tilenum = frontsector->ceilingpicnum;
			setgotpic(tilenum);
			tileUpdatePicnum(&tilenum, tilenum, 0);
			texture = tileGetTexture(tilenum);
			if (texture && texture->isValid())
			{
				//iboindex = frontsector->iboindex[sector_t::floor];
				PutFlat(di,  1);
			}
		}
	}
}

void HWFlat::ProcessFlatSprite(HWDrawInfo* di, spritetype* sprite, sectortype* sector)
{
	int tilenum = sprite->picnum;
	texture = tileGetTexture(tilenum);
	z = sprite->z * (1 / -256.f);
	if (z == di->Viewpoint.Pos.Z) return; // looking right at the edge.

	visibility = sectorVisibility(&sector[sprite->sectnum]) *(4.f / 5.f); // The factor comes directly from Polymost. What is it with Build and these magic factors?

	// Weird Build logic that really makes no sense.
	if ((sprite->cstat & CSTAT_SPRITE_ONE_SIDED) != 0 && (di->Viewpoint.Pos.Z < z) == ((sprite->cstat & CSTAT_SPRITE_YFLIP) == 0))
		return;

	if (texture && texture->isValid())
	{
		this->sprite = sprite;
		sec = sector;
		shade = clamp(sprite->shade, 0, numshades - 1);
		palette = sprite->pal;
		fade = lookups.getFade(sector[sprite->sectnum].floorpal);	// fog is per sector.

		SetSpriteTranslucency(sprite, alpha, RenderStyle);

		PutFlat(di, 0);
	}
}
