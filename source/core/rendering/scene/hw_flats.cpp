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
#include "hw_sections.h"

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

void HWFlat::MakeVertices(HWDrawInfo* di)
{
	if (vertcount > 0) return;
	bool canvas = texture->isHardwareCanvas();
	if (Sprite == nullptr)
	{
		TArray<int>* pIndices;
		auto mesh = sectionGeometry.get(&sections[section], plane, geoofs, &pIndices);

		auto ret = screen->mVertexData->AllocVertices(pIndices->Size());
		auto vp = ret.first;
		float base = (plane == 0 ? sec->floorz : sec->ceilingz) * (1 / -256.f);
		for (unsigned i = 0; i < pIndices->Size(); i++)
		{
			auto ii = (*pIndices)[i];
			auto& pt = mesh->vertices[ii];
			auto& uv = mesh->texcoords[ii];
			vp->SetVertex(pt.X, base + pt.Z, pt.Y);
			vp->SetTexCoord(uv.X, canvas ? 1.f - uv.Y : uv.Y);
			vp++;
		}
		vertindex = ret.second;
		vertcount = pIndices->Size();
		normal = mesh->normal;
	}
	else
	{
		vec2_t pos[4];
		int ofsz[4];
		auto cstat = Sprite->cstat;
		if (tspriteGetSlope(Sprite)) cstat &= ~CSTAT_SPRITE_YFLIP;	// NBlood doesn't y-flip slope sprites.
		GetFlatSpritePosition(Sprite, Sprite->pos.vec2, pos, ofsz, true);
		Sprite->cstat = cstat;

		auto ret = screen->mVertexData->AllocVertices(6);
		auto vp = ret.first;
		float x = !(Sprite->cstat & CSTAT_SPRITE_XFLIP) ? 0.f : 1.f;
		float y = !(Sprite->cstat & CSTAT_SPRITE_YFLIP) ? 0.f : 1.f;
		if (Sprite->clipdist & TSPR_SLOPESPRITE)
		{
			int posx = int(di->Viewpoint.Pos.X * 16.f);
			int posy = int(di->Viewpoint.Pos.Y * -16.f);
			int posz = int(di->Viewpoint.Pos.Z * -256.f);

			// Make adjustments for poorly aligned slope sprites on floors or ceilings
			constexpr float ONPLANE_THRESHOLD = 3.f;
			if (tspriteGetZOfSlope(Sprite, posx, posy) < posz)
			{
				float maxofs = -FLT_MAX, minofs = FLT_MAX;
				for (int i = 0; i < 4; i++)
				{
					float vz = getceilzofslopeptr(Sprite->sectp, pos[i].X, pos[i].Y) * (1 / -256.f);
					float sz = z + ofsz[i] * (1 / -256.f);
					int diff = vz - sz;
					if (diff > maxofs) maxofs = diff;
					if (diff < minofs) minofs = diff;
				}
				if (maxofs > 0 && minofs >= -ONPLANE_THRESHOLD && maxofs <= ONPLANE_THRESHOLD) z -= maxofs;
			}
			else
			{
				float maxofs = -FLT_MAX, minofs = FLT_MAX;
				for (int i = 0; i < 4; i++)
				{
					float vz = getflorzofslopeptr(Sprite->sectp, pos[i].X, pos[i].Y) * (1 / -256.f);
					float sz = z + ofsz[i] * (1 / -256.f);
					int diff = vz - sz;
					if (diff > maxofs) maxofs = diff;
					if (diff < minofs) minofs = diff;
				}
				if (minofs < 0 && maxofs <= -ONPLANE_THRESHOLD && minofs >= ONPLANE_THRESHOLD) z -= minofs;
			}
		}
		else
		{
			if (z < di->Viewpoint.Pos.Z) normal = { 0,1,0 };
			else normal = { 0, -1, 0 };
		}


		unsigned svi = di->SlopeSpriteVertices.Reserve(4);
		auto svp = &di->SlopeSpriteVertices[svi];

		auto& vpt = di->Viewpoint;
		depth = (float)((Sprite->pos.X * (1/16.f) - vpt.Pos.X) * vpt.TanCos + (Sprite->pos.Y * (1 / -16.f) - vpt.Pos.Y) * vpt.TanSin);

		for (unsigned j = 0; j < 4; j++)
		{
			svp->SetVertex(pos[j].X * (1 / 16.f), z + ofsz[j] * (1 / -256.f), pos[j].Y * (1 / -16.f));
			if (!canvas) svp->SetTexCoord(j == 1 || j == 2 ? 1.f - x : x, j == 2 || j == 3 ? 1.f - y : y);
			else svp->SetTexCoord(j == 1 || j == 2 ? 1.f - x : x, j == 2 || j == 3 ? y : 1.f - y);
			svp++;
		}
		if (Sprite->clipdist & TSPR_SLOPESPRITE)
		{
			FVector3 v1 = {
				di->SlopeSpriteVertices[svi + 1].x - di->SlopeSpriteVertices[svi].x,
				di->SlopeSpriteVertices[svi + 1].y - di->SlopeSpriteVertices[svi].y,
				di->SlopeSpriteVertices[svi + 1].z - di->SlopeSpriteVertices[svi].z };
			FVector3 v2 = {
				di->SlopeSpriteVertices[svi + 2].x - di->SlopeSpriteVertices[svi].x,
				di->SlopeSpriteVertices[svi + 2].y - di->SlopeSpriteVertices[svi].y,
				di->SlopeSpriteVertices[svi + 2].z - di->SlopeSpriteVertices[svi].z };

			normal = (v1 ^ v2).Unit();
		}
		for (unsigned i = 0; i < 6; i++)
		{
			const static unsigned indices[] = { 0, 1, 2, 0, 2, 3 };
			*vp++ = di->SlopeSpriteVertices[svi + indices[i]];
		}
		slopeindex = svi;
		slopecount = 4;
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
	rendered_flats++;
	if (screen->BuffersArePersistent() && !Sprite)
	{
		MakeVertices(di);
	}

#ifdef _DEBUG
	if (sectnum(sec) == gl_breaksec)
	{
		int a = 0;
	}
#endif

	state.SetNormal(normal);
	SetLightAndFog(di, state, fade, palette, shade, visibility, alpha);

	if (translucent)
	{
		if (RenderStyle.BlendOp != STYLEOP_Add)
		{
			state.EnableBrightmap(false);
		}

		state.SetRenderStyle(RenderStyle);
		state.SetTextureMode(RenderStyle);

		if (texture && !checkTranslucentReplacement(texture->GetID(), palette)) state.AlphaFunc(Alpha_GEqual, texture->alphaThreshold);
		else state.AlphaFunc(Alpha_GEqual, 0.f);
	}
	else if (shade > numshades && (GlobalMapFog || (fade & 0xffffff)))
	{
		state.SetObjectColor(fade | 0xff000000);
		state.EnableTexture(false);
	}


	state.SetMaterial(texture, UF_Texture, 0, Sprite == nullptr? CLAMP_NONE : CLAMP_XY, TRANSLATION(Translation_Remap + curbasepal, palette), -1);

	state.SetLightIndex(dynlightindex);
	state.Draw(DT_Triangles, vertindex, vertcount);
	vertexcount += vertcount;

	if (translucent) state.SetRenderStyle(LegacyRenderStyles[STYLE_Translucent]);
	state.EnableBrightmap(true);

	state.SetObjectColor(0xffffffff);
	state.EnableTexture(true);
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
	if (!screen->BuffersArePersistent() || Sprite || di->ingeo)	// should be made static buffer content later (when the logic is working)
	{
#if 0
		if (di->Level->HasDynamicLights && texture != nullptr && !di->isFullbrightScene() && !(hacktype & (SSRF_PLANEHACK | SSRF_FLOODHACK)))
		{
			SetupLights(di, section->lighthead, lightdata, sector->PortalGroup);
		}
#endif
		MakeVertices(di);
	}
	di->AddFlat(this);
}

//==========================================================================
//
// Process a sector's flats for rendering
// This function is only called once per sector.
// Subsequent subsectors are just quickly added to the ss_renderflags array
//
//==========================================================================

void HWFlat::ProcessSector(HWDrawInfo *di, sectortype * frontsector, int section_, int which)
{
#ifdef _DEBUG
	if (sectnum(sec) == gl_breaksec)
	{
		int a = 0;
	}
#endif

	dynlightindex = -1;

	const auto &vp = di->Viewpoint;

	float florz, ceilz;
	PlanesAtPoint(frontsector, float(vp.Pos.X) * 16.f, float(vp.Pos.Y) * -16.f, &ceilz, &florz);

	visibility = sectorVisibility(frontsector);
	sec = frontsector;
	section = section_;
	Sprite = nullptr;
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

		fade = lookups.getFade(frontsector->floorpal);
		shade = frontsector->floorshade;
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
			gotpic.Set(tilenum);
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

		fade = lookups.getFade(frontsector->ceilingpal);
		shade = frontsector->ceilingshade;
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
			gotpic.Set(tilenum);
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

//==========================================================================
//
// Process a floor sprite
//
//==========================================================================

void HWFlat::ProcessFlatSprite(HWDrawInfo* di, tspritetype* sprite, sectortype* sector)
{
	int tilenum = sprite->picnum;
	texture = tileGetTexture(tilenum);
	bool belowfloor = false;
	if (sprite->pos.Z > sprite->sectp->floorz)
	{
		belowfloor = true;
		sprite->pos.Z = sprite->sectp->floorz;
	}
	z = sprite->pos.Z * (1 / -256.f);
	if (z == di->Viewpoint.Pos.Z) return; // looking right at the edge.
	dynlightindex = -1;

	visibility = sectorVisibility(sector) *(4.f / 5.f); // The factor comes directly from Polymost. What is it with Build and these magic factors?

	// Weird Build logic that really makes no sense.
	if ((sprite->cstat & CSTAT_SPRITE_ONE_SIDE) != 0)
	{
		double myz = !(sprite->clipdist & TSPR_SLOPESPRITE) ? z :
			tspriteGetZOfSlope(sprite, int(di->Viewpoint.Pos.X * 16), int(di->Viewpoint.Pos.Y * -16)) * -(1. / 256.);
		if ((di->Viewpoint.Pos.Z < myz) == ((sprite->cstat & CSTAT_SPRITE_YFLIP) == 0))
			return;
	}

	if (texture && texture->isValid())
	{
		this->Sprite = sprite;
		sec = sector;
		shade = sprite->shade;
		palette = sprite->pal;
		fade = lookups.getFade(sector->floorpal);	// fog is per sector.

		SetSpriteTranslucency(sprite, alpha, RenderStyle);
		if (belowfloor) alpha *= 0.33f;

		PutFlat(di, z > di->Viewpoint.Pos.Z);
	}
}
