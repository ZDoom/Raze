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
#include "hw_vertexmap.h"
#include "gamefuncs.h"
#include "cmdlib.h"

#include "v_video.h"
#include "flatvertices.h"

DCoreActor* wall_to_sprite_actors[8]; // gets updated each frame. Todo: Encapsulate this better without having to permanently store actors in the wall object.

//==========================================================================
//
//
//
//==========================================================================

static walltype* IsOnWall(tspritetype* tspr, int height, DVector2& outpos)
{
	double maxorthdist = 3 * maptoworld; // maximum orthogonal distance to be considered an attached sprite.
	double maxdistsq = maxorthdist * maxorthdist;
	walltype* best = nullptr;

	auto sect = tspr->sectp;

	double tx = tspr->pos.X;
	double ty = tspr->pos.Y;

	for(auto& wal : wallsofsector(sect))
	{
		// Intentionally include two sided walls. Even on them the sprite should be projected onto the wall for better results.
		auto d = wal.delta();
		auto deltaang = absangle(VecToAngle(d), tspr->angle);
		const DAngle maxangdelta = DAngle360 / 1024;

		// angle of the sprite must either be the wall's normal or the negative wall's normal to be aligned.
		if (deltaang >= DAngle90 - maxangdelta && deltaang <= DAngle90 + maxangdelta)
		{
			if (!((tspr->angle.Buildang()) & 510))
			{
				// orthogonal lines do not check the actual position so that certain off-sector sprites get handled properly. 
				// In Wanton Destruction's airplane level there's such a sprite assigned to the wrong sector.
				if (d.X == 0)
				{
					double newdist = fabs(tx - wal.pos.X);
					if (newdist < maxorthdist)
					{
						maxorthdist = newdist;
						best = &wal;
					}
				}
				else if (d.Y == 0)
				{
					double newdist = fabs(ty - wal.pos.Y);
					if (newdist < maxorthdist)
					{
						maxorthdist = newdist;
						best = &wal;
					}
				}
			}
			else
			{
				double wdist = SquareDistToWall(tx, ty, &wal, &outpos);
				if (wdist <= maxdistsq)
				{
					return &wal;
				}
			}
		}
	}
	return best;
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
		SetLightAndFog(di, state, fade, palette, shade, visibility, alpha);
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
void HWWall::RenderMirrorSurface(HWDrawInfo *di, FRenderState &state)
{
	if (!TexMan.mirrorTexture.isValid()) return;

	state.SetDepthFunc(DF_LEqual);

	// we use texture coordinates and texture matrix to pass the normal stuff to the shader so that the default vertex buffer format can be used as is.
	state.EnableTextureMatrix(true);

	// Use sphere mapping for this
	state.SetEffect(EFF_SPHEREMAP);
	SetLightAndFog(di, state, fade, palette, min<int>(shade, numshades), visibility, alpha);
	state.SetColor(PalEntry(25, globalr >> 1, globalg >> 1, globalb >> 1));

	state.SetRenderStyle(STYLE_Add);
	state.AlphaFunc(Alpha_Greater, 0);

	auto tex = TexMan.GetGameTexture(TexMan.mirrorTexture, false);
	state.SetMaterial(tex, UF_None, 0, CLAMP_NONE, 0, -1); // do not upscale the mirror texture.

	RenderWall(di, state, HWWall::RWF_BLANK);

	state.EnableTextureMatrix(false);
	state.SetEffect(EFF_NONE);
	state.AlphaFunc(Alpha_GEqual, 0.5f);

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
	SetLightAndFog(di, state, fade, palette, shade, visibility, alpha);

	state.SetMaterial(texture, UF_Texture, 0, (flags & (HWF_CLAMPX | HWF_CLAMPY)), TRANSLATION(Translation_Remap + curbasepal, palette), -1);

	if (Sprite == nullptr)
	{
		if (shade > numshades && (GlobalMapFog || (fade & 0xffffff)))
		{
			state.SetObjectColor(fade | 0xff000000);
			state.EnableTexture(false);
		}

		int h = (int)texture->GetDisplayHeight();
		int h2 = 1 << sizeToBits(h);
		if (h2 < h) h2 *= 2;
		if (h != h2)
		{
			float xOffset = 1.f / texture->GetDisplayWidth();
			state.SetNpotEmulation(float(h2) / h, xOffset);
		}
		RenderWall(di, state, rflags);
	}
	else if (!(rflags & RWF_TRANS))
	{
		auto oldbias = state.GetDepthBias();
		if (walldist) state.SetDepthBias(-1, -129);
		else state.ClearDepthBias();
		RenderWall(di, state, rflags);
		state.SetDepthBias(oldbias);

	}
	else
		RenderWall(di, state, rflags);

	state.SetNpotEmulation(0.f, 0.f);
	state.SetObjectColor(0xffffffff);
	state.EnableTexture(true);
	/* none of these functions is in use.
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
	if (RenderStyle.BlendOp != STYLEOP_Add)
	{
		state.EnableBrightmap(false);
	}

	state.SetRenderStyle(RenderStyle);
	state.SetTextureMode(RenderStyle);
	if (texture && !checkTranslucentReplacement(texture->GetID(), palette)) state.AlphaFunc(Alpha_GEqual, texture->alphaThreshold);
	else state.AlphaFunc(Alpha_GEqual, 0.f);
	RenderTexturedWall(di, state, HWWall::RWF_TEXTURED);
	state.SetRenderStyle(STYLE_Translucent);
	state.EnableBrightmap(true);
}

//==========================================================================
//
// 
//
//==========================================================================
void HWWall::DrawWall(HWDrawInfo *di, FRenderState &state, bool translucent)
{
	rendered_lines++;
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
		ViewDistance = (di->Viewpoint.Pos.XY() - DVector2((glseg.x1 + glseg.x2) * 0.5f, (glseg.y1 + glseg.y2) * 0.5f)).LengthSquared();
	}

	if (texture->isHardwareCanvas())
	{
		tcs[UPLFT].v = 1.f - tcs[UPLFT].v;
		tcs[LOLFT].v = 1.f - tcs[LOLFT].v;
		tcs[UPRGT].v = 1.f - tcs[UPRGT].v;
		tcs[LORGT].v = 1.f - tcs[LORGT].v;
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
	flags &= ~(HWF_TRANSLUCENT|HWF_CLAMPX|HWF_CLAMPY);
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
		assert(seg->portalnum >= 0 && seg->portalnum < 8);
		portal = new HWLineToSpritePortal(&portalState, seg, wall_to_sprite_actors[seg->portalnum]);
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
	}
	if (tcs[UPLFT].u >= 0.f && tcs[UPRGT].u >= 0.f && tcs[LOLFT].u >= 0.f && tcs[LORGT].u >= 0.f &&
		tcs[UPLFT].u <= 1.f && tcs[UPRGT].u <= 1.f && tcs[LOLFT].u <= 1.f && tcs[LORGT].u <= 1.f)
	{
		flags |= HWF_CLAMPX;
	}

	if (tcs[UPLFT].v >= 0.f && tcs[UPRGT].v >= 0.f && tcs[LOLFT].v >= 0.f && tcs[LORGT].v >= 0.f &&
		tcs[UPLFT].v <= 1.f && tcs[UPRGT].v <= 1.f && tcs[LOLFT].v <= 1.f && tcs[LORGT].v <= 1.f)
	{
		flags |= HWF_CLAMPY;
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

	float tw = texture->GetDisplayWidth();
	float th = texture->GetDisplayHeight();
	int pow2size = 1 << sizeToBits(th);
	if (pow2size < th) pow2size *= 2;
	float ypanning = refwall->ypan_ ? pow2size * refwall->ypan_ / (256.0f * th) : 0;

	tcs[LOLFT].u = tcs[UPLFT].u = ((leftdist * 8.f * wal->xrepeat) + refwall->xpan_) / tw;
	tcs[LORGT].u = tcs[UPRGT].u = ((rightdist * 8.f * wal->xrepeat) + refwall->xpan_) / tw;

	auto setv = [=](float hl, float hr, float frac) -> float
	{
		float h = hl + (hr - hl) * frac;
		h = (-(float)((refheight + h) * 256) / ((th * 2048.0f) / (float)(wal->yrepeat))) + ypanning;
		if (refwall->cstat & CSTAT_WALL_YFLIP) h = -h;
		return h;
	};

	tcs[UPLFT].v = setv(topleft, topright, glseg.fracleft);
	tcs[LOLFT].v = setv(bottomleft, bottomright, glseg.fracleft);
	tcs[UPRGT].v = setv(topleft, topright, glseg.fracright);
	tcs[LORGT].v = setv(bottomleft, bottomright, glseg.fracright);
	if (th == pow2size) CheckTexturePosition(); // for NPOT textures this adjustment can break things.
	bool trans = type == RENDERWALL_M2S && maskWallHasTranslucency(wal);
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
	float refheight;

	if ((wal->cstat & CSTAT_WALL_1WAY) && backsector)
	{
		if ((!(wal->cstat & CSTAT_WALL_BOTTOM_SWAP) && (wal->cstat & CSTAT_WALL_1WAY)) ||
			((wal->cstat & CSTAT_WALL_BOTTOM_SWAP) && (wal->nextWall()->cstat & CSTAT_WALL_ALIGN_BOTTOM)))
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
	float refheight = (wal->cstat & CSTAT_WALL_ALIGN_BOTTOM) ? frontsector->ceilingz : backsector->ceilingz;

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
	auto refwall = (wal->cstat & CSTAT_WALL_BOTTOM_SWAP) ? wal->nextWall() : wal;
	float refheight = (refwall->cstat & CSTAT_WALL_ALIGN_BOTTOM) ? frontsector->ceilingz : backsector->floorz;

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
	float refheight;

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
	if ((bch1 - fch1) * (bch2 - fch2) >= 0)
	{
		topleft = min(bch1, fch1);
		topright = min(bch2, fch2);
	}
	else
	{
		// Front ceiling slope obstructs part of the wall
		topleft = bch1;
		topright = bch2;
	}
	if ((bfh1 - ffh1) * (bfh2 - ffh2) >= 0)
	{
		bottomleft = max(bfh1, ffh1);
		bottomright = max(bfh2, ffh2);
	}
	else
	{
		// Front floor slope obstructs part of the wall
		bottomleft = bfh1;
		bottomright = bfh2;
	}
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
	auto backwall = wal->twoSided()? wal->nextWall() : nullptr;
	auto p2wall = wal->point2Wall();

	float fch1;
	float ffh1;
	float fch2;
	float ffh2;

	FVector2 v1(WallStartX(wal), WallStartY(wal));
	FVector2 v2(WallEndX(wal), WallEndY(wal));

	PlanesAtPoint(frontsector, wal->pos.X, wal->pos.Y, &fch1, &ffh1);
	PlanesAtPoint(frontsector, p2wall->pos.X, p2wall->pos.Y, &fch2, &ffh2);


#ifdef _DEBUG
	if (wallnum(wal) == 34)
	{
		int a = 0;
	}
#endif

	this->seg = wal;
	this->frontsector = frontsector;
	this->backsector = backsector;
	Sprite = nullptr;
	vertindex = 0;
	vertcount = 0;

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
	fade = lookups.getFade(wal->pal);
	visibility = sectorVisibility(frontsector);

	alpha = 1.0f;
	RenderStyle = STYLE_Translucent;
	texture = NULL;

	if (gl_seamless)
	{
		auto v = &vertices[vertexMap[wallnum(wal)]];
		if (v->dirty) v->RecalcVertexHeights();
		v = &vertices[vertexMap[wal->point2]];
		if (v->dirty) v->RecalcVertexHeights();
	}

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

		int tilenum = ((wal->cstat & CSTAT_WALL_1WAY) && wal->nextwall != -1) ? wal->overpicnum : wal->picnum;
		gotpic.Set(tilenum);
		tileUpdatePicnum(&tilenum, (wal->cstat & CSTAT_WALL_ROTATE_90));
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
		PlanesAtPoint(backsector, wal->pos.X, wal->pos.Y, &bch1, &bfh1);
		PlanesAtPoint(backsector, p2wall->pos.X, p2wall->pos.Y, &bch2, &bfh2);

		SkyTop(di, wal, frontsector, backsector, v1, v2, fch1, fch2);
		SkyBottom(di, wal, frontsector, backsector, v1, v2, ffh1, ffh2);

		// upper texture
		if (!(frontsector->ceilingstat & backsector->ceilingstat & CSTAT_SECTOR_SKY))
		{
			float bch1a = bch1;
			float bch2a = bch2;
			if (ffh1 > bch1 || ffh2 > bch2)
			{
				// the back sector's floor obstructs part of this wall. Todo: Handle the portal case better.
				if ((ffh1 > bch1 && ffh2 > bch2) || frontsector->portalflags == PORTAL_SECTOR_FLOOR)
				{
					bch2a = ffh2;
					bch1a = ffh1;
				}
			}

			if (bch1a < fch1 || bch2a < fch2)
			{
				int tilenum = wal->picnum;
				gotpic.Set(tilenum);
				tileUpdatePicnum(&tilenum, (wal->cstat & CSTAT_WALL_ROTATE_90));
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
			gotpic.Set(tilenum);
			tileUpdatePicnum(&tilenum, (wal->cstat & CSTAT_WALL_ROTATE_90));
			texture = tileGetTexture(tilenum);
			if (texture && texture->isValid())
			{
				DoMidTexture(di, wal, frontsector, backsector, fch1, fch2, ffh1, ffh2, bch1, bch2, bfh1, bfh2);
			}
		}

		// lower texture
		if (!(frontsector->floorstat & backsector->floorstat & CSTAT_SECTOR_SKY))
		{
			if (fch1 < bfh1 || fch2 < bfh2)
			{
				// the back sector's ceiling obstructs part of this wall. Todo: Handle the portal case better.
				if ((fch1 < bfh1 && fch2 < bfh2) || frontsector->portalflags == PORTAL_SECTOR_CEILING)
				{
					bfh1 = fch1;
					bfh2 = fch2;
				}
			}

			if (bfh1 > ffh1 || bfh2 > ffh2)
			{
				auto w = (wal->cstat & CSTAT_WALL_BOTTOM_SWAP) ? backwall : wal;
				int tilenum = w->picnum;
				gotpic.Set(tilenum);
				tileUpdatePicnum(&tilenum, (w->cstat & CSTAT_WALL_ROTATE_90));
				texture = tileGetTexture(tilenum);
				if (texture && texture->isValid())
				{
					DoLowerTexture(di, wal, frontsector, backsector, bfh1, bfh2, ffh1, ffh2);
				}
			}
		}
	}
}

//==========================================================================
//
// checks if the wall sprite has changed since the last call.
// Returns:
//  0 if identical or only render style changed
//  1 if vertical orientation changed
//  2 if horizontal orientation changed
//  3 if positioning changed
//
//==========================================================================

int HWWall::CheckWallSprite(tspritetype* spr, tspritetype* last)
{
	// If the position changed we need to recalculate everything.
	if (spr->pos.XY() != last->pos.XY() || spr->sectp != last->sectp || spr->int_ang() != last->int_ang()) return 3;
	
	// if the horizontal orientation changes we need to recalculate the walls this attaches to, but not the positioning.
	if (spr->xrepeat != last->xrepeat || spr->xoffset != last->xoffset || spr->picnum != last->picnum || ((spr->cstat ^ last->cstat) & CSTAT_SPRITE_XFLIP)) return 2;
	
	// only y-positioning changed - we need to re-check the wall tiers this sprite attaches to
	if(spr->yrepeat != last->yrepeat || spr->yoffset != last->yoffset || ((spr->cstat ^ last->cstat) & (CSTAT_SPRITE_YFLIP|CSTAT_SPRITE_YCENTER))) return 1;

	// all remaining properties only affect the render style which is not relevant for positioning a wall sprite
	return 0;
}

//==========================================================================
//
//
//
//==========================================================================

void HWWall::ProcessWallSprite(HWDrawInfo* di, tspritetype* spr, sectortype* sector)
{
	auto tex = tileGetTexture(spr->picnum);
	if (!tex || !tex->isValid()) return;

	seg = nullptr;
	Sprite = spr;
	DVector2 pos[2];

	GetWallSpritePosition(spr, spr->pos.XY(), pos, true);
	glseg.x1 = pos[0].X;
	glseg.y1 = -pos[0].Y;
	glseg.x2 = pos[1].X;
	glseg.y2 = -pos[1].Y;

	if (spr->cstat & CSTAT_SPRITE_ONE_SIDE)
	{
		if (PointOnLineSide(di->Viewpoint.Pos.X, di->Viewpoint.Pos.Y, glseg.x1, glseg.y1, glseg.x2 - glseg.x1, glseg.y2 - glseg.y1) <= 0)
		{
			return;
		}
	}

	vertindex = 0;
	vertcount = 0;
	type = RENDERWALL_M2S;
	frontsector = sector;
	backsector = sector;
	texture = tex;

	flags = HWF_CLAMPX|HWF_CLAMPY;
	dynlightindex = -1;
	shade = spr->shade;
	palette = spr->pal;
	fade = lookups.getFade(sector->floorpal);	// fog is per sector.
	visibility = sectorVisibility(sector);

	SetSpriteTranslucency(Sprite, alpha, RenderStyle);

	int height, topofs;
	if (hw_hightile && TileFiles.tiledata[spr->picnum].hiofs.xsize)
	{
		height = TileFiles.tiledata[spr->picnum].hiofs.ysize;
		topofs = (TileFiles.tiledata[spr->picnum].hiofs.yoffs + spr->yoffset);
	}
	else
	{
		height = (int)tex->GetDisplayHeight();
		topofs = ((int)tex->GetDisplayTopOffset() + spr->yoffset);
	}

	DVector2 vec{};
	walldist = IsOnWall(spr, height, vec);
	if (walldist)
	{
		// project the sprite right onto the wall.
		auto v1 = NearestPointLine(glseg.x1, -glseg.y1, walldist);
		auto v2 = NearestPointLine(glseg.x2, -glseg.y2, walldist);
		glseg.x1 = v1.X;
		glseg.y1 = -v1.Y;
		glseg.x2 = v2.X;
		glseg.y2 = -v2.Y;

	}

	if (spr->cstat & CSTAT_SPRITE_YFLIP)
		topofs = -topofs;
	
	float yrepeat = spr->yrepeat * (1.f / 64.f);
	float sprz = spr->pos.Z - topofs * yrepeat;

	if (spr->cstat & CSTAT_SPRITE_YCENTER)
	{
		sprz += height * yrepeat * 0.5f;
		if (height & 1) sprz += yrepeat * 0.5f;  // Odd yspans
	}

	glseg.fracleft = 0;
	glseg.fracright = 1;
	tcs[LOLFT].u = tcs[UPLFT].u = (spr->cstat & CSTAT_SPRITE_XFLIP) ? 1.f : 0.f;
	tcs[LORGT].u = tcs[UPRGT].u = (spr->cstat & CSTAT_SPRITE_XFLIP) ? 0.f : 1.f;
	tcs[UPLFT].v = tcs[UPRGT].v = (spr->cstat & CSTAT_SPRITE_YFLIP) ? 1.f : 0.f;
	tcs[LOLFT].v = tcs[LORGT].v = (spr->cstat & CSTAT_SPRITE_YFLIP) ? 0.f : 1.f;

	zbottom[0] = zbottom[1] = -sprz;
	ztop[0] = ztop[1] = -sprz + height * yrepeat;
	if (zbottom[0] > ztop[0])
	{
		// reorder coordinates to make the clipping code below behave.
		auto zz = zbottom[0];
		zbottom[0] = zbottom[1] = ztop[0];
		ztop[0] = ztop[1] = zz;
		tcs[UPLFT].v = tcs[UPRGT].v = 1.f - tcs[UPLFT].v;
		tcs[LOLFT].v = tcs[LORGT].v = 1.f - tcs[LOLFT].v;
	}
	
	// Clip sprites to ceilings/floors
	if (!(sector->ceilingstat & CSTAT_SECTOR_SKY))
	{
		float polyh = (ztop[0] - zbottom[0]);
		float ceilingz = -sector->ceilingz;
		if (ceilingz < ztop[0] && ceilingz >= zbottom[0])
		{
			float newv = (ceilingz - zbottom[0]) / polyh;
			tcs[UPLFT].v = tcs[UPRGT].v = tcs[LOLFT].v + newv * (tcs[UPLFT].v - tcs[LOLFT].v);
			ztop[0] = ztop[1] = ceilingz;
		}
	}
	if (!(sector->floorstat & CSTAT_SECTOR_SKY))
	{
		float polyh = (ztop[0] - zbottom[0]);
		float floorz = -sector->floorz;
		if (floorz <= ztop[0] && floorz > zbottom[0])
		{
			float newv = (floorz - zbottom[0]) / polyh;
			tcs[LOLFT].v = tcs[LORGT].v = tcs[LOLFT].v + newv * (tcs[UPLFT].v - tcs[LOLFT].v);
			zbottom[0] = zbottom[1] = floorz;
		}
	}
	if (zbottom[0] >= ztop[0])
		return; // nothing left to render.

	// If the sprite is backward, flip it around so that we have guaranteed orientation when this is about to be sorted.
	if (PointOnLineSide(di->Viewpoint.Pos.XY(), DVector2(glseg.x1, glseg.y1), DVector2(glseg.x2, glseg.y2)) < 0)
	{
		std::swap(glseg.x1, glseg.x2);
		std::swap(glseg.y1, glseg.y2);
		// z is always the same on both sides.
		std::swap(tcs[LOLFT], tcs[LORGT]);
		std::swap(tcs[UPLFT], tcs[UPRGT]);
	}

	PutWall(di, spriteHasTranslucency(Sprite));
}
