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

#include "texturemanager.h"
#include "hw_dynlightdata.h"
#include "hw_material.h"
#include "hw_cvars.h"
#include "hw_clock.h"
//#include "hw_lighting.h"
#include "hw_drawinfo.h"
#include "hw_portal.h"
#include "hw_lightbuffer.h"
#include "hw_renderstate.h"
#include "hw_skydome.h"
#include "hw_drawstructs.h"
#include "gamefuncs.h"
#include "cmdlib.h"

#include "v_video.h"
#include "flatvertices.h"
#include "glbackend/glbackend.h"

extern PalEntry GlobalMapFog;
extern float GlobalFogDensity;

//==========================================================================
//
// Create vertices for one wall
//
//==========================================================================

int HWWall::CreateVertices(FFlatVertex*& ptr, bool split)
{
	auto oo = ptr;
	ptr->Set(glseg.x1, zbottom[0], glseg.y1, tcs[LOLFT].u, tcs[LOLFT].v);
	ptr++;
	ptr->Set(glseg.x1, ztop[0], glseg.y1, tcs[UPLFT].u, tcs[UPLFT].v);
	ptr++;
	ptr->Set(glseg.x2, ztop[1], glseg.y2, tcs[UPRGT].u, tcs[UPRGT].v);
	ptr++;
	ptr->Set(glseg.x2, zbottom[1], glseg.y2, tcs[LORGT].u, tcs[LORGT].v);
	ptr++;
	return int(ptr - oo);
}

//==========================================================================
//
// build the vertices for this wall
//
//==========================================================================

void HWWall::MakeVertices(HWDrawInfo* di, bool nosplit)
{
	if (vertcount == 0)
	{
		auto ret = screen->mVertexData->AllocVertices(4);
		vertindex = ret.second;
		vertcount = CreateVertices(ret.first, false);
	}
}

//==========================================================================
//
// General purpose wall rendering function
// everything goes through here
//
//==========================================================================

void HWWall::RenderWall(HWDrawInfo *di, FRenderState &state, int textured)
{
	assert(vertcount > 0);
	state.SetLightIndex(dynlightindex);
	state.Draw(DT_TriangleFan, vertindex, vertcount);
	vertexcount += vertcount;
}

//==========================================================================
//
// 
//
//==========================================================================

void HWWall::RenderFogBoundary(HWDrawInfo *di, FRenderState &state)
{
	if (gl_fogmode)// && !di->isFullbrightScene())
	{
		state.EnableDrawBufferAttachments(false);
		SetLightAndFog(state);
		state.SetEffect(EFF_FOGBOUNDARY);
		state.AlphaFunc(Alpha_GEqual, 0.f);
		state.SetDepthBias(-1, -128);
		RenderWall(di, state, HWWall::RWF_BLANK);
		state.ClearDepthBias();
		state.SetEffect(EFF_NONE);
		state.EnableDrawBufferAttachments(true);
	}
}

//==========================================================================
//
// 
//
//==========================================================================

void HWWall::SetLightAndFog(FRenderState& state)
{
	// Fog must be done before the texture so that the texture selector can override it.
	bool foggy = (GlobalMapFog || (fade & 0xffffff));
	auto ShadeDiv = lookups.tables[palette].ShadeFactor;
	// Disable brightmaps if non-black fog is used.
	if (ShadeDiv >= 1 / 1000.f && foggy)
	{
		state.EnableFog(1);
		float density = GlobalMapFog ? GlobalFogDensity : 350.f - Scale(numshades - shade, 150, numshades);
		state.SetFog((GlobalMapFog) ? GlobalMapFog : fade, density);
		state.SetSoftLightLevel(255);
		state.SetLightParms(128.f, 1 / 1000.f);
	}
	else
	{
		state.EnableFog(0);
		state.SetFog(0, 0);
		state.SetSoftLightLevel(ShadeDiv >= 1 / 1000.f ? 255 - Scale(shade, 255, numshades) : 255);
		state.SetLightParms(visibility, ShadeDiv / (numshades - 2));
	}

	// The shade rgb from the tint is ignored here.
	state.SetColor(PalEntry(255, globalr, globalg, globalb));
}

//==========================================================================
//
// 
//
//==========================================================================
void HWWall::RenderMirrorSurface(HWDrawInfo *di, FRenderState &state)
{
	if (!TexMan.mirrorTexture.isValid()) return;

	state.SetDepthFunc(DF_LEqual);

	// we use texture coordinates and texture matrix to pass the normal stuff to the shader so that the default vertex buffer format can be used as is.
	state.EnableTextureMatrix(true);

	// Use sphere mapping for this
	state.SetEffect(EFF_SPHEREMAP);
	SetLightAndFog(state);
	state.SetColor(PalEntry(25, globalr >> 1, globalg >> 1, globalb >> 1));

	state.SetRenderStyle(STYLE_Add);
	state.AlphaFunc(Alpha_Greater, 0);

	auto tex = TexMan.GetGameTexture(TexMan.mirrorTexture, false);
	state.SetMaterial(tex, UF_None, 0, CLAMP_NONE, 0, -1); // do not upscale the mirror texture.

	RenderWall(di, state, HWWall::RWF_BLANK);

	state.EnableTextureMatrix(false);
	state.SetEffect(EFF_NONE);
	state.AlphaFunc(Alpha_GEqual, gl_mask_sprite_threshold);

	state.SetDepthFunc(DF_Less);
	state.SetRenderStyle(STYLE_Translucent);
}

//==========================================================================
//
// 
//
//==========================================================================

void HWWall::RenderTexturedWall(HWDrawInfo *di, FRenderState &state, int rflags)
{
	//int tmode = state.GetTextureMode();

	state.SetMaterial(texture, UF_Texture, 0, 0/*flags & 3*/, TRANSLATION(Translation_Remap + curbasepal, palette), -1);

	SetLightAndFog(state);

	int h = texture->GetTexelHeight();
	int h2 = 1 << sizeToBits(h);
	if (h2 < h) h2 *= 2;
	if (h != h2)
	{
		float xOffset = 1.f / texture->GetTexelWidth();
		state.SetNpotEmulation(float(h2) / h, xOffset);
	}
	else
	{
		state.SetNpotEmulation(0.f, 0.f);
	}

	RenderWall(di, state, rflags);

	state.SetNpotEmulation(0.f, 0.f);
	/* none of these functions is in use.
	state.SetObjectColor(0xffffffff);
	state.SetObjectColor2(0);
	state.SetAddColor(0);
	state.SetTextureMode(tmode);
	state.EnableGlow(false);
	state.EnableGradient(false);
	state.ApplyTextureManipulation(nullptr);
	*/
}

//==========================================================================
//
// 
//
//==========================================================================

void HWWall::RenderTranslucentWall(HWDrawInfo *di, FRenderState &state)
{
	state.SetRenderStyle(RenderStyle);
	if (!texture->GetTranslucency()) state.AlphaFunc(Alpha_GEqual, gl_mask_threshold);
	else state.AlphaFunc(Alpha_GEqual, 0.f);
	RenderTexturedWall(di, state, HWWall::RWF_TEXTURED);
	state.SetRenderStyle(STYLE_Translucent);
}

//==========================================================================
//
// 
//
//==========================================================================
void HWWall::DrawWall(HWDrawInfo *di, FRenderState &state, bool translucent)
{
	if (screen->BuffersArePersistent())
	{
		/*
		if (di->Level->HasDynamicLights && !di->isFullbrightScene() && texture != nullptr)
		{
			SetupLights(di, lightdata);
		}
		*/
		MakeVertices(di, !!(flags & HWWall::HWF_TRANSLUCENT));
	}

	state.SetNormal(glseg.Normal());
	if (!translucent)
	{
		RenderTexturedWall(di, state, HWWall::RWF_TEXTURED);
	}
	else
	{
		switch (type)
		{
		case RENDERWALL_MIRRORSURFACE:
			RenderMirrorSurface(di, state);
			break;

		case RENDERWALL_FOGBOUNDARY:
			RenderFogBoundary(di, state);
			break;

		default:
			RenderTranslucentWall(di, state);
			break;
		}
	}
}

//==========================================================================
//
// Collect lights for shader
//
//==========================================================================
#if 0
void HWWall::SetupLights(HWDrawInfo *di, FDynLightData &lightdata)
{
	lightdata.Clear();

	if (RenderStyle == STYLE_Add && !di->Level->lightadditivesurfaces) return;	// no lights on additively blended surfaces.

	// check for wall types which cannot have dynamic lights on them (portal types never get here so they don't need to be checked.)
	switch (type)
	{
	case RENDERWALL_FOGBOUNDARY:
	case RENDERWALL_MIRRORSURFACE:
	case RENDERWALL_COLOR:
		return;
	}

	float vtx[]={glseg.x1,zbottom[0],glseg.y1, glseg.x1,ztop[0],glseg.y1, glseg.x2,ztop[1],glseg.y2, glseg.x2,zbottom[1],glseg.y2};
	Plane p;


	auto normal = glseg.Normal();
	p.Set(normal, -normal.X * glseg.x1 - normal.Z * glseg.y1);

	FLightNode *node;
	if (seg->sidedef == NULL)
	{
		node = NULL;
	}
	else if (!(seg->sidedef->Flags & WALLF_POLYOBJ))
	{
		node = seg->sidedef->lighthead;
	}
	else if (sub)
	{
		// Polobject segs cannot be checked per sidedef so use the subsector instead.
		node = sub->section->lighthead;
	}
	else node = NULL;

	// Iterate through all dynamic lights which touch this wall and render them
	while (node)
	{
		if (node->lightsource->IsActive())
		{
			iter_dlight++;

			DVector3 posrel = node->lightsource->PosRelative(seg->frontsector->PortalGroup);
			float x = posrel.X;
			float y = posrel.Y;
			float z = posrel.Z;
			float dist = fabsf(p.DistToPoint(x, z, y));
			float radius = node->lightsource->GetRadius();
			float scale = 1.0f / ((2.f * radius) - dist);
			FVector3 fn, pos;

			if (radius > 0.f && dist < radius)
			{
				FVector3 nearPt, up, right;

				pos = { x, z, y };
				fn = p.Normal();

				fn.GetRightUp(right, up);

				FVector3 tmpVec = fn * dist;
				nearPt = pos + tmpVec;

				FVector3 t1;
				int outcnt[4]={0,0,0,0};
				texcoord tcs[4];

				// do a quick check whether the light touches this polygon
				for(int i=0;i<4;i++)
				{
					t1 = FVector3(&vtx[i*3]);
					FVector3 nearToVert = t1 - nearPt;
					tcs[i].u = ((nearToVert | right) * scale) + 0.5f;
					tcs[i].v = ((nearToVert | up) * scale) + 0.5f;

					if (tcs[i].u<0) outcnt[0]++;
					if (tcs[i].u>1) outcnt[1]++;
					if (tcs[i].v<0) outcnt[2]++;
					if (tcs[i].v>1) outcnt[3]++;

				}
				if (outcnt[0]!=4 && outcnt[1]!=4 && outcnt[2]!=4 && outcnt[3]!=4) 
				{
					draw_dlight += GetLight(lightdata, seg->frontsector->PortalGroup, p, node->lightsource, true);
				}
			}
		}
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
void HWWall::PutWall(HWDrawInfo *di, bool translucent)
{
	if (translucent || (texture && texture->GetTranslucency() && type == RENDERWALL_M2S))
	{
		flags |= HWF_TRANSLUCENT;
		//ViewDistance = (di->Viewpoint.Pos - (seg->linedef->v1->fPos() + seg->linedef->Delta() / 2)).XY().LengthSquared();
	}
	
	if (!screen->BuffersArePersistent())
	{
		/*
		if (di->Level->HasDynamicLights && !di->isFullbrightScene() && texture != nullptr)
		{
			SetupLights(di, lightdata);
		}
		*/
		MakeVertices(di, translucent);
	}

	di->AddWall(this);
	// make sure that following parts of the same linedef do not get this one's vertex and lighting info.
	vertcount = 0;	
	dynlightindex = -1;
	flags &= ~HWF_TRANSLUCENT;
}

//==========================================================================
//
// will be done later.
//
//==========================================================================

void HWWall::PutPortal(HWDrawInfo *di, int ptype, int plane)
{
	HWPortal * portal = nullptr;

	MakeVertices(di, false);
	switch (ptype)
	{
#if 0
		// portals don't go into the draw list.
		// Instead they are added to the portal manager
	case PORTALTYPE_HORIZON:
		horizon = portalState.UniqueHorizons.Get(horizon);
		portal = di->FindPortal(horizon);
		if (!portal)
		{
			portal = new HWHorizonPortal(&portalState, horizon, di->Viewpoint);
			di->Portals.Push(portal);
		}
		portal->AddLine(this);
		break;

	case PORTALTYPE_SKYBOX:
		portal = di->FindPortal(secportal);
		if (!portal)
		{
			// either a regular skybox or an Eternity-style horizon
			if (secportal->mType != PORTS_SKYVIEWPOINT) portal = new HWEEHorizonPortal(&portalState, secportal);
			else
			{
				portal = new HWSkyboxPortal(&portalState, secportal);
				di->Portals.Push(portal);
			}
		}
		portal->AddLine(this);
		break;
#endif

	case PORTALTYPE_SECTORSTACK:
		portal = di->FindPortal(this->portal);
		if (!portal)
		{
			portal = new HWSectorStackPortal(&portalState, this->portal);
			di->Portals.Push(portal);
		}
		portal->AddLine(this);
		break;

#if 0
	case PORTALTYPE_PLANEMIRROR:
		if (portalState.PlaneMirrorMode * planemirror->fC() <= 0)
		{
			planemirror = portalState.UniquePlaneMirrors.Get(planemirror);
			portal = di->FindPortal(planemirror);
			if (!portal)
			{
				portal = new HWPlaneMirrorPortal(&portalState, planemirror);
				di->Portals.Push(portal);
			}
			portal->AddLine(this);
		}
		break;
#endif
	case PORTALTYPE_MIRROR:
		// These are unique. No need to look existing ones up.
		portal = new HWMirrorPortal(&portalState, seg);
		di->Portals.Push(portal);
		portal->AddLine(this);
		if (gl_mirror_envmap)
		{
			// draw a reflective layer over the mirror
			di->AddMirrorSurface(this);
		}
		break;

	case PORTALTYPE_LINETOLINE:
		// These are also unique.
		portal = new HWLineToLinePortal(&portalState, seg, &wall[seg->portalnum]);
		di->Portals.Push(portal);
		portal->AddLine(this);
		break;

	case PORTALTYPE_LINETOSPRITE:
		// These are also unique.
		portal = new HWLineToSpritePortal(&portalState, seg, &sprite[seg->portalnum]);
		di->Portals.Push(portal);
		portal->AddLine(this);
		break;

	case PORTALTYPE_SKY:
		sky = portalState.UniqueSkies.Get(sky);
		portal = di->FindPortal(sky);
		if (!portal)
		{
			portal = new HWSkyPortal(screen->mSkyData, &portalState, sky);
			di->Portals.Push(portal);
		}
		portal->AddLine(this);
		break;
	}
	vertcount = 0;

	if (plane != -1 && portal)
	{
		portal->planesused |= (1<<plane);
	}
}

//==========================================================================
//
// Build does not have horizon effects.
//
//==========================================================================
bool HWWall::DoHorizon(HWDrawInfo* di, walltype* seg, sectortype* fs, DVector2& v1, DVector2& v2)
{
#if 0
	HWHorizonInfo hi;
	lightlist_t * light;

	// ZDoom doesn't support slopes in a horizon sector so I won't either!
	ztop[1] = ztop[0] = fs->GetPlaneTexZ(sector_t::ceiling);
	zbottom[1] = zbottom[0] = fs->GetPlaneTexZ(sector_t::floor);

    auto vpz = di->Viewpoint.Pos.Z;
	if (vpz < fs->GetPlaneTexZ(sector_t::ceiling))
	{
		if (vpz > fs->GetPlaneTexZ(sector_t::floor))
			zbottom[1] = zbottom[0] = vpz;

		if (fs->GetTexture(sector_t::ceiling) == skyflatnum)
		{
			SkyPlane(di, fs, sector_t::ceiling, false);
		}
		else
		{
			hi.plane.GetFromSector(fs, sector_t::ceiling);
			hi.lightlevel = hw_ClampLight(fs->GetCeilingLight());
			hi.colormap = fs->Colormap;
			hi.specialcolor = fs->SpecialColors[sector_t::ceiling];

			if (di->isFullbrightScene()) hi.colormap.Clear();
			horizon = &hi;
			PutPortal(di, PORTALTYPE_HORIZON, -1);
		}
		ztop[1] = ztop[0] = zbottom[0];
	} 

	if (vpz > fs->GetPlaneTexZ(sector_t::floor))
	{
		zbottom[1] = zbottom[0] = fs->GetPlaneTexZ(sector_t::floor);
		if (fs->GetTexture(sector_t::floor) == skyflatnum)
		{
			SkyPlane(di, fs, sector_t::floor, false);
		}
		else
		{
			hi.plane.GetFromSector(fs, sector_t::floor);
			hi.lightlevel = hw_ClampLight(fs->GetFloorLight());
			hi.colormap = fs->Colormap;
			hi.specialcolor = fs->SpecialColors[sector_t::floor];

			horizon = &hi;
			PutPortal(di, PORTALTYPE_HORIZON, -1);
		}
	}
#endif
	return true;
}

//==========================================================================
//
// 
//
//==========================================================================

bool HWWall::SetWallCoordinates(walltype * seg, float topleft, float topright, float bottomleft, float bottomright)
{
	//
	//
	// set up coordinates for the left side of the polygon
	//
	// check left side for intersections
	if (topleft >= bottomleft)
	{
		// normal case
		ztop[0] = topleft;
		zbottom[0] = bottomleft;
	}
	else
	{
		// ceiling below floor - clip to the visible part of the wall
		float dch = topright - topleft;
		float dfh = bottomright - bottomleft;
		float inter_x = (bottomleft - topleft) / (dch - dfh);
		float inter_y = topleft + inter_x * dch;

		glseg.x1 = glseg.x1 + inter_x * (glseg.x2 - glseg.x1);
		glseg.y1 = glseg.y1 + inter_x * (glseg.y2 - glseg.y1);
		glseg.fracleft = inter_x;

		zbottom[0] = ztop[0] = inter_y;
	}

	//
	//
	// set up coordinates for the right side of the polygon
	//
	// check left side for intersections
	if (topright >= bottomright)
	{
		// normal case
		ztop[1] = topright;
		zbottom[1] = bottomright;
	}
	else
	{
		// ceiling below floor - clip to the visible part of the wall
		float dch = topright - topleft;
		float dfh = bottomright - bottomleft;
		float inter_x = (bottomleft - topleft) / (dch - dfh);

		float inter_y = topleft + inter_x * dch;

		glseg.x2 = glseg.x1 + inter_x * (glseg.x2 - glseg.x1);
		glseg.y2 = glseg.y1 + inter_x * (glseg.y2 - glseg.y1);
		glseg.fracright = inter_x;

		zbottom[1] = ztop[1] = inter_y;
	}

	return true;
}

//==========================================================================
//
// Do some tweaks with the texture coordinates to reduce visual glitches
//
//==========================================================================

void HWWall::CheckTexturePosition()
{
	float sub;

	if (texture->isHardwareCanvas()) return;

	// clamp texture coordinates to a reasonable range.
	// Extremely large values can cause visual problems
	if (tcs[UPLFT].v < tcs[LOLFT].v || tcs[UPRGT].v < tcs[LORGT].v)
	{
		if (tcs[UPLFT].v < tcs[UPRGT].v)
		{
			sub = float(xs_FloorToInt(tcs[UPLFT].v));
		}
		else
		{
			sub = float(xs_FloorToInt(tcs[UPRGT].v));
		}
		tcs[UPLFT].v -= sub;
		tcs[UPRGT].v -= sub;
		tcs[LOLFT].v -= sub;
		tcs[LORGT].v -= sub;

		if ((tcs[UPLFT].v == 0.f && tcs[UPRGT].v == 0.f && tcs[LOLFT].v <= 1.f && tcs[LORGT].v <= 1.f) ||
			(tcs[UPLFT].v >= 0.f && tcs[UPRGT].v >= 0.f && tcs[LOLFT].v == 1.f && tcs[LORGT].v == 1.f))
		{
			flags |= HWF_CLAMPY;
		}
	}
	else
	{
		if (tcs[LOLFT].v < tcs[LORGT].v)
		{
			sub = float(xs_FloorToInt(tcs[LOLFT].v));
		}
		else
		{
			sub = float(xs_FloorToInt(tcs[LORGT].v));
		}
		tcs[UPLFT].v -= sub;
		tcs[UPRGT].v -= sub;
		tcs[LOLFT].v -= sub;
		tcs[LORGT].v -= sub;

		if ((tcs[LOLFT].v == 0.f && tcs[LORGT].v == 0.f && tcs[UPLFT].v <= 1.f && tcs[UPRGT].v <= 1.f) ||
			(tcs[LOLFT].v >= 0.f && tcs[LORGT].v >= 0.f && tcs[UPLFT].v == 1.f && tcs[UPRGT].v == 1.f))
		{
			flags |= HWF_CLAMPY;
		}
	}
}


//==========================================================================
//
// Common part of wall drawers
//
//==========================================================================

void HWWall::DoTexture(HWDrawInfo* di, walltype* wal, walltype* refwall, float refheight, float topleft, float topright, float bottomleft, float bottomright)
{
	auto glsave = glseg;
	SetWallCoordinates(wal, topleft, topright, bottomleft, bottomright);

	bool xflipped = (wal->cstat & CSTAT_WALL_XFLIP);
	float leftdist = xflipped ? 1.f - glseg.fracleft : glseg.fracleft;
	float rightdist = xflipped ? 1.f - glseg.fracright : glseg.fracright;

	float tw = texture->GetTexelWidth();
	float th = texture->GetTexelHeight();
	int pow2size = 1 << sizeToBits(th);
	if (pow2size < th) pow2size *= 2;
	float ypanning = refwall->ypan_ ? pow2size * refwall->ypan_ / (256.0f * th) : 0;

	tcs[LOLFT].u = tcs[UPLFT].u = ((leftdist * 8.f * wal->xrepeat) + refwall->xpan_) / tw;
	tcs[LORGT].u = tcs[UPRGT].u = ((rightdist * 8.f * wal->xrepeat) + refwall->xpan_) / tw;
	 
	auto setv = [=](float hl, float hr, float frac) -> float
	{
		float h = hl + (hr - hl) * frac;
		h = (-(float)(refheight + (h * 256)) / ((th * 2048.0f) / (float)(wal->yrepeat))) + ypanning;
		if (wal->cstat & CSTAT_WALL_YFLIP) h = 1.f - h;
		return h;
	};

	tcs[UPLFT].v = setv(topleft, topright, glseg.fracleft);
	tcs[LOLFT].v = setv(bottomleft, bottomright, glseg.fracleft);
	tcs[UPRGT].v = setv(topleft, topright, glseg.fracright);
	tcs[LORGT].v = setv(bottomleft, bottomright, glseg.fracright);
	if (th == pow2size) CheckTexturePosition(); // for NPOT textures this adjustment can break things.
	bool trans = type == RENDERWALL_M2S && (wal->cstat & CSTAT_WALL_TRANSLUCENT);
	if (trans)
	{
		RenderStyle = GetRenderStyle(0, !!(wal->cstat & CSTAT_WALL_TRANS_FLIP));
		alpha = GetAlphaFromBlend((wal->cstat & CSTAT_WALL_TRANS_FLIP) ? DAMETH_TRANS2 : DAMETH_TRANS1, 0);
	}
	PutWall(di, trans);
	flags = 0;
	glseg = glsave;
}

//==========================================================================
//
//  Handle one sided walls
//
//==========================================================================

void HWWall::DoOneSidedTexture(HWDrawInfo* di, walltype* wal, sectortype* frontsector, sectortype* backsector,
	float topleft, float topright, float bottomleft, float bottomright)
{
	// get the alignment reference position.
	int refheight;

	if ((wal->cstat & CSTAT_WALL_1WAY) && backsector)
	{
		if ((!(wal->cstat & CSTAT_WALL_BOTTOM_SWAP) && (wal->cstat & CSTAT_WALL_1WAY)) ||
			((wal->cstat & CSTAT_WALL_BOTTOM_SWAP) && (wall[wal->nextwall].cstat & CSTAT_WALL_ALIGN_BOTTOM)))
			refheight = frontsector->ceilingz;
		else
			refheight = backsector->floorz;
	}
	else
	{
		refheight = (wal->cstat & CSTAT_WALL_ALIGN_BOTTOM) ? frontsector->floorz : frontsector->ceilingz;
	}

	type = RENDERWALL_M1S;
	DoTexture(di, wal, wal, refheight, topleft, topright, bottomleft, bottomright);
}

//==========================================================================
//
//
//
//==========================================================================

void HWWall::DoUpperTexture(HWDrawInfo* di, walltype* wal, sectortype* frontsector, sectortype* backsector,
	float topleft, float topright, float bottomleft, float bottomright)
{
	// get the alignment reference position.
	int refheight = (wal->cstat & CSTAT_WALL_ALIGN_BOTTOM) ? frontsector->ceilingz : backsector->ceilingz;

	type = RENDERWALL_TOP;
	DoTexture(di, wal, wal, refheight, topleft, topright, bottomleft, bottomright);
}

//==========================================================================
//
//
//
//==========================================================================

void HWWall::DoLowerTexture(HWDrawInfo* di, walltype* wal, sectortype* frontsector, sectortype* backsector,
	float topleft, float topright, float bottomleft, float bottomright)
{
	// get the alignment reference position.
	int refheight;
	auto refwall = (wal->cstat & CSTAT_WALL_BOTTOM_SWAP) ? &wall[wal->nextwall] : wal;
	refheight = (refwall->cstat & CSTAT_WALL_ALIGN_BOTTOM) ? frontsector->ceilingz : backsector->floorz;

	shade = refwall->shade;
	palette = refwall->pal;
	type = RENDERWALL_BOTTOM;
	DoTexture(di, wal, refwall, refheight, topleft, topright, bottomleft, bottomright);
}

//==========================================================================
//
// 
//
//==========================================================================

void HWWall::DoMidTexture(HWDrawInfo* di, walltype* wal,
	sectortype* front, sectortype* back,
	float fch1, float fch2, float ffh1, float ffh2,
	float bch1, float bch2, float bfh1, float bfh2)
{
	float topleft,bottomleft,topright,bottomright;
	int refheight;

	const int swapit = (wal->cstat & CSTAT_WALL_ALIGN_BOTTOM);

	if (wal->cstat & CSTAT_WALL_1WAY)
	{
		// 1-sided wall
		refheight = swapit ? front->ceilingz : back->ceilingz;
	}
	else
	{
		// masked wall
		if (swapit)
			refheight = min(front->floorz, back->floorz);
		else
			refheight = max(front->ceilingz, back->ceilingz);
	}

	topleft = std::min(bch1,fch1);
	topright = std::min(bch2,fch2);
	bottomleft = std::max(bfh1,ffh1);
	bottomright = std::max(bfh2,ffh2);
	if (topleft<=bottomleft && topright<=bottomright) return;
	type = seg->cstat & CSTAT_WALL_1WAY ? RENDERWALL_M1S : RENDERWALL_M2S;

	// todo: transparency.

	DoTexture(di, wal, wal, refheight, topleft, topright, bottomleft, bottomright);
	RenderStyle = STYLE_Translucent;
	alpha = 1.f;
}


//==========================================================================
//
// 
//
//==========================================================================
void HWWall::Process(HWDrawInfo* di, walltype* wal, sectortype* frontsector, sectortype* backsector)
{
	auto backwall = wal->nextwall >= 0 && wal->nextwall < numwalls ? &wall[wal->nextwall] : nullptr;
	auto p2wall = &wall[wal->point2];

	float fch1;
	float ffh1;
	float fch2;
	float ffh2;

	FVector2 v1(WallStartX(wal), WallStartY(wal));
	FVector2 v2(WallEndX(wal), WallEndY(wal));

	PlanesAtPoint(frontsector, wal->x, wal->y, &fch1, &ffh1);
	PlanesAtPoint(frontsector, p2wall->x, p2wall->y, &fch2, &ffh2);


#ifdef _DEBUG
	if (wal - wall == 843)
	{
		int a = 0;
	}
#endif

	// note: we always have a valid sidedef and linedef reference when getting here.

	this->seg = wal;
	this->frontsector = frontsector;
	this->backsector = backsector;
	vertindex = 0;
	vertcount = 0;

	//vertexes[0] = v1;
	//vertexes[1] = v2;

	glseg.x1 = v1.X;
	glseg.y1 = v1.Y;
	glseg.x2 = v2.X;
	glseg.y2 = v2.Y;
	glseg.fracleft = 0;
	glseg.fracright = 1;
	flags = 0;
	dynlightindex = -1;
	shade = wal->shade;
	palette = wal->pal;
	fade = lookups.getFade(frontsector->floorpal);	// fog is per sector.
	visibility = sectorVisibility(frontsector);

	alpha = 1.0f;
	RenderStyle = STYLE_Translucent;
	texture = NULL;

	/*
	if (wal->linedef->special == Line_Horizon)
	{
		SkyNormal(di, frontsector, v1, v2);
		DoHorizon(di, wal, frontsector, v1, v2);
		return;
	}
	*/

	bool isportal = false;// wal->linedef->isVisualPortal() && wal->sidedef == wal->linedef->sidedef[0];

	if (seg->portalflags)
	{
		int ptype = -1;
		if (seg->portalflags == PORTAL_WALL_MIRROR) ptype = PORTALTYPE_MIRROR;
		else if (seg->portalflags == PORTAL_WALL_VIEW) ptype = PORTALTYPE_LINETOLINE;
		else if (seg->portalflags == PORTAL_WALL_TO_SPRITE) ptype = PORTALTYPE_LINETOSPRITE;
		if (ptype != -1)
		{
			ztop[0] = fch1;
			ztop[1] = fch2;
			zbottom[0] = ffh1;
			zbottom[1] = ffh2;
			PutPortal(di, ptype, -1);
			return;
		}
	}


	if (!backsector || !backwall)
	{
		// sector's sky
		SkyNormal(di, frontsector, v1, v2, fch1, fch2, ffh1, ffh2);

		// normal texture

		int tilenum = (wal->cstat & CSTAT_WALL_1WAY) ? wal->overpicnum : wal->picnum;
		setgotpic(tilenum);
		tileUpdatePicnum(&tilenum, int(wal-wall) + 16384, wal->cstat);
		texture = tileGetTexture(tilenum);
		if (texture && texture->isValid())
		{
			DoOneSidedTexture(di, wal, frontsector, backsector, fch1, fch2, ffh1, ffh2);
		}
	}
	else // two sided
	{
		float bfh1;
		float bfh2;
		float bch1;
		float bch2;
		PlanesAtPoint(backsector, wal->x, wal->y, &bch1, &bfh1);
		PlanesAtPoint(backsector, p2wall->x, p2wall->y, &bch2, &bfh2);

		float zalign = 0.f;

		if (fch1 > ffh1 || fch2 > ffh2)
		{
			SkyTop(di, wal, frontsector, backsector, v1, v2, fch1, fch2);
			SkyBottom(di, wal, frontsector, backsector, v1, v2, ffh1, ffh2);
		}

		// upper texture
		if (!(frontsector->ceilingstat & backsector->ceilingstat & CSTAT_SECTOR_SKY))
		{
			float bch1a = bch1;
			float bch2a = bch2;
			if (ffh1 > bch1 && ffh2 > bch2)
			{
				// the back sector's floor obstructs part of this wall
				bch2a = ffh2;
				bch1a = ffh1;
			}

			if (bch1a < fch1 || bch2a < fch2)
			{
				int tilenum = wal->picnum;
				setgotpic(tilenum);
				tileUpdatePicnum(&tilenum, int(wal - wall) + 16384, wal->cstat);
				texture = tileGetTexture(tilenum);
				if (texture && texture->isValid())
				{
					DoUpperTexture(di, wal, frontsector, backsector, fch1, fch2, bch1a, bch2a);
				}
			}
		}

		if (wal->cstat & (CSTAT_WALL_MASKED | CSTAT_WALL_1WAY))
		{
			int tilenum = wal->overpicnum;
			setgotpic(tilenum);
			tileUpdatePicnum(&tilenum, int(wal - wall) + 16384, wal->cstat);
			texture = tileGetTexture(tilenum);
			if (texture && texture->isValid())
			{
				DoMidTexture(di, wal, frontsector, backsector, fch1, fch2, ffh1, ffh2, bch1, bch2, bfh1, bfh2);
			}
		}

		// lower texture
		if (!(frontsector->floorstat & backsector->floorstat & CSTAT_SECTOR_SKY))
		{
			if (fch1 < bfh1 && fch2 < bfh2)
			{
				// the back sector's ceiling obstructs part of this wall.
				bfh1 = fch1;
				bfh2 = fch2;
			}

			if (bfh1 > ffh1 || bfh2 > ffh2)
			{
				auto w = (wal->cstat & CSTAT_WALL_BOTTOM_SWAP) ? backwall : wal;
				int tilenum = w->picnum;
				setgotpic(tilenum);
				tileUpdatePicnum(&tilenum, int(wal - wall) + 16384, w->cstat);
				texture = tileGetTexture(tilenum);
				if (texture && texture->isValid())
				{
					DoLowerTexture(di, wal, frontsector, backsector, bfh1, bfh2, ffh1, ffh2);
				}
			}
		}
	}
	globalr = globalg = globalb = 255;
}

void HWWall::ProcessWallSprite(HWDrawInfo* di, spritetype* spr, sectortype* sector)
{
	auto tex = tileGetTexture(spr->picnum);
	if (!tex || !tex->isValid()) return;

	seg = nullptr;
	sprite = spr;
	vec2_t pos[2];
	int sprz = spr->pos.z;

	if (spr->cstat & CSTAT_SPRITE_ONE_SIDED)
	{
		DAngle sprang = buildang(spr->ang).asdeg();
		DAngle lookang = bamang(di->Viewpoint.RotAngle).asdeg();
		if ((sprang.ToVector() | lookang.ToVector()) >= 0.) return;
	}

	vertindex = 0;
	vertcount = 0;
	type = RENDERWALL_M2S;
	frontsector = sector;
	backsector = sector;
	texture = tex;

	flags = 0;
	dynlightindex = -1;
	shade = spr->shade;
	palette = spr->pal;
	fade = lookups.getFade(sector->floorpal);	// fog is per sector.
	visibility = sectorVisibility(sector);

	bool trans = (sprite->cstat & CSTAT_SPRITE_TRANSLUCENT);
	if (trans)
	{
		RenderStyle = GetRenderStyle(0, !!(sprite->cstat & CSTAT_SPRITE_TRANSLUCENT_INVERT));
		alpha = GetAlphaFromBlend((sprite->cstat & CSTAT_SPRITE_TRANSLUCENT_INVERT) ? DAMETH_TRANS2 : DAMETH_TRANS1, 0);
	}
	else
	{
		RenderStyle = LegacyRenderStyles[STYLE_Translucent];
		alpha = 1.f;
	}


	GetWallSpritePosition(spr, spr->pos.vec2, pos, true);

	int height, topofs;
	if (hw_hightile && TileFiles.tiledata[spr->picnum].h_xsize)
	{
		height = TileFiles.tiledata[spr->picnum].h_ysize;
		topofs = (TileFiles.tiledata[spr->picnum].h_yoffs + spr->yoffset);
	}
	else
	{
		height = tex->GetTexelHeight();
		topofs = (tex->GetTexelTopOffset() + spr->yoffset);
	}

	if (spr->cstat & CSTAT_SPRITE_YFLIP)
		topofs = -topofs;

	sprz -= ((topofs * spr->yrepeat) << 2);

	if (spr->cstat & CSTAT_SPRITE_YCENTER)
	{
		sprz += ((height * spr->yrepeat) << 1);
		if (height & 1) sprz += (spr->yrepeat << 1);  // Odd yspans (taken from polymost as-is)
	}

	glseg.fracleft = 0;
	glseg.fracright = 1;
	glseg.x1 = pos[0].x * (1 / 16.f);
	glseg.y1 = pos[0].y * (1 / -16.f);
	glseg.x2 = pos[1].x * (1 / 16.f);
	glseg.y2 = pos[1].y * (1 / -16.f);
	tcs[LOLFT].u = tcs[UPLFT].u = (spr->cstat & CSTAT_SPRITE_XFLIP) ? 1.f : 0.f;
	tcs[LORGT].u = tcs[UPRGT].u = (spr->cstat & CSTAT_SPRITE_XFLIP) ? 0.f : 1.f;
	tcs[UPLFT].v = tcs[UPRGT].u = (spr->cstat & CSTAT_SPRITE_YFLIP) ? 0.f : 1.f;
	tcs[LOLFT].v = tcs[LORGT].u = (spr->cstat & CSTAT_SPRITE_YFLIP) ? 0.f : 1.f;
	ztop[0] = ztop[1] = (sprz - height) * (1 / -256.);
	zbottom[0] = zbottom[1] = (sprz) * (1 / -256.);


	// Clip sprites to ceilings/floors
	float origz = ztop[0];
	float polyh = (zbottom[0] - origz);
	if (!(sector->ceilingstat & CSTAT_SECTOR_SKY))
	{
		float ceilingz = sector->ceilingz * (1 / 256.f);
		if (ceilingz < ztop[0] && ceilingz > zbottom[0])
		{
			float newv = (ceilingz - origz) / polyh;
			tcs[UPLFT].v = tcs[UPRGT].v = newv;
			ztop[0] = ztop[1] = ceilingz;
		}
	}
	if (!(sector->floorstat & CSTAT_SECTOR_SKY))
	{
		float floorz = sector->floorz * (1 / 256.f);
		if (floorz < ztop[0] && floorz > zbottom[0])
		{
			float newv = (floorz - origz) / polyh;
			tcs[LOLFT].v = tcs[LORGT].v = newv;
			zbottom[0] = zbottom[1] = floorz;
		}
	}
}