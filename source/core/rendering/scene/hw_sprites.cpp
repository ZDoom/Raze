// 
//---------------------------------------------------------------------------
//
// Copyright(C) 2002-2016 Christoph Oelckers
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
** gl_sprite.cpp
** Sprite/Particle rendering
**
*/

#include "matrix.h"
//#include "models.h"
#include "vectors.h"
#include "texturemanager.h"
#include "basics.h"

//#include "hw_models.h"
#include "hw_drawstructs.h"
#include "hw_drawinfo.h"
#include "hw_portal.h"
#include "flatvertices.h"
#include "hw_cvars.h"
#include "hw_clock.h"
#include "hw_material.h"
#include "hw_dynlightdata.h"
#include "hw_lightbuffer.h"
#include "hw_renderstate.h"
#include "hw_models.h"
#include "hw_viewpointbuffer.h"
#include "hw_voxels.h"

//==========================================================================
//
// 
//
//==========================================================================

void HWSprite::DrawSprite(HWDrawInfo* di, FRenderState& state, bool translucent)
{
	bool additivefog = false;
	bool foglayer = false;

	if (translucent)
	{
		// The translucent pass requires special setup for the various modes.

		// for special render styles brightmaps would not look good - especially for subtractive.
		if (RenderStyle.BlendOp != STYLEOP_Add)
		{
			state.EnableBrightmap(false);
		}

		state.SetRenderStyle(RenderStyle);
		state.SetTextureMode(RenderStyle);
		state.AlphaFunc(Alpha_Greater, alphaThreshold);

		if (RenderStyle.BlendOp == STYLEOP_Add && RenderStyle.DestAlpha == STYLEALPHA_One)
		{
			additivefog = true;
		}
	}

#if 0
	if (dynlightindex == -1)	// only set if we got no light buffer index. This covers all cases where sprite lighting is used.
	{
		float out[3] = {};
		//di->GetDynSpriteLight(gl_light_sprites ? actor : nullptr, gl_light_particles ? particle : nullptr, out);
		//state.SetDynLight(out[0], out[1], out[2]);
	}
#endif


	if (RenderStyle.Flags & STYLEF_FadeToBlack)
	{
		fade = 0;
		additivefog = true;
	}

	if (RenderStyle.BlendOp == STYLEOP_RevSub || RenderStyle.BlendOp == STYLEOP_Sub)
	{
		if (!modelframe)
		{
			// non-black fog with subtractive style needs special treatment
			if (fade != 0)
			{
				foglayer = true;
				// Due to the two-layer approach we need to force an alpha test that lets everything pass
				state.AlphaFunc(Alpha_Greater, 0);
			}
		}
		else RenderStyle.BlendOp = STYLEOP_Fuzz;	// subtractive with models is not going to work.
	}

	SetLightAndFog(di, state, fade, palette, shade, visibility, alpha);


	if (modelframe == 0)
	{
		state.SetMaterial(texture, UF_Texture, 0, CLAMP_XY, TRANSLATION(Translation_Remap + curbasepal, palette), -1);
		state.SetNormal(0, 0, 0);


		if (screen->BuffersArePersistent())
		{
			CreateVertices(di);
		}
		state.SetLightIndex(-1);
		state.Draw(DT_TriangleStrip, vertexindex, 4);

		if (foglayer)
		{
			bool foggy = (GlobalMapFog || (fade & 0xffffff));
			auto ShadeDiv = lookups.tables[palette].ShadeFactor;
			if (ShadeDiv >= 1 / 1000.f && foggy)
			{
				// If we get here we know that we have colored fog and no fixed colormap.
				float density = GlobalMapFog ? GlobalFogDensity : 350.f - Scale(numshades - shade, 150, numshades);
				state.SetFog((GlobalMapFog) ? GlobalMapFog : fade, density * hw_density);
				state.SetTextureMode(TM_FOGLAYER);
				state.SetRenderStyle(STYLE_Translucent);
				state.Draw(DT_TriangleStrip, vertexindex, 4);
				state.SetTextureMode(TM_NORMAL);
			}
		}
	}
	else
	{
		FHWModelRenderer mr(state, dynlightindex);
		if (modelframe < 0)
		{
			state.mModelMatrix = rotmat;

			auto model = voxel->model;
			state.SetDepthFunc(DF_LEqual);
			state.EnableTexture(true);
			model->BuildVertexBuffer(&mr);
			bool mirrored = ((Sprite->cstat & CSTAT_SPRITE_XFLIP) != 0) ^ ((Sprite->cstat & CSTAT_SPRITE_YFLIP) != 0) ^ portalState.isMirrored();
			mr.BeginDrawModel(RenderStyle, nullptr, rotmat, mirrored);
			mr.SetupFrame(model, 0, 0, 0);
			model->RenderFrame(&mr, TexMan.GetGameTexture(model->GetPaletteTexture()), 0, 0, 0.f, TRANSLATION(Translation_Remap + curbasepal, palette), nullptr);
			mr.EndDrawModel(RenderStyle, nullptr);
			state.SetDepthFunc(DF_Less);
			state.SetVertexBuffer(screen->mVertexData);

		}
		else
		{
			//RenderModel(&renderer, x, y, z, modelframe, actor, di->Viewpoint.TicFrac);
		}
		state.SetObjectColor(0xffffffff);
		state.SetVertexBuffer(screen->mVertexData);
	}

	if (translucent)
	{
		state.EnableBrightmap(true);
		state.SetRenderStyle(STYLE_Translucent);
		state.SetTextureMode(TM_NORMAL);
	}

	state.SetObjectColor(0xffffffff);
	state.SetAddColor(0);
	state.EnableTexture(true);
	state.SetDynLight(0, 0, 0);
}


//==========================================================================
//
// 
//
//==========================================================================

void HWSprite::CalculateVertices(HWDrawInfo* di, FVector3* v, DVector3* vp)
{
	const auto& HWAngles = di->Viewpoint.HWAngles;

#if 0 // maybe later, it's a bit more tricky with Build.
	// [BB] Billboard stuff
	const bool drawWithXYBillboard = false;
	const bool drawBillboardFacingCamera = false;


	// [Nash] check for special sprite drawing modes
	if (drawWithXYBillboard || drawBillboardFacingCamera)
	{
		// Compute center of sprite
		float xcenter = (x1 + x2) * 0.5;
		float ycenter = (y1 + y2) * 0.5;
		float zcenter = (z1 + z2) * 0.5;
		float xx = -xcenter + x;
		float zz = -zcenter + z;
		float yy = -ycenter + y;
		Matrix3x4 mat;
		mat.MakeIdentity();
		mat.Translate(xcenter, zcenter, ycenter); // move to sprite center

												  // Order of rotations matters. Perform yaw rotation (Y, face camera) before pitch (X, tilt up/down).
		if (drawBillboardFacingCamera)
		{
			// [CMB] Rotate relative to camera XY position, not just camera direction,
			// which is nicer in VR
			float xrel = xcenter - vp->X;
			float yrel = ycenter - vp->Y;
			float absAngleDeg = RAD2DEG(atan2(-yrel, xrel));
			float counterRotationDeg = 270. - HWAngles.Yaw.Degrees; // counteracts existing sprite rotation
			float relAngleDeg = counterRotationDeg + absAngleDeg;

			mat.Rotate(0, 1, 0, relAngleDeg);
		}

		// [fgsfds] calculate yaw vectors
		float yawvecX = 0, yawvecY = 0, rollDegrees = 0;
		float angleRad = (270. - HWAngles.Yaw).Radians();

		if (drawWithXYBillboard)
		{
			// Rotate the sprite about the vector starting at the center of the sprite
			// triangle strip and with direction orthogonal to where the player is looking
			// in the x/y plane.
			mat.Rotate(-sin(angleRad), 0, cos(angleRad), -HWAngles.Pitch.Degrees);
		}

		mat.Translate(-xcenter, -zcenter, -ycenter); // retreat from sprite center
		v[0] = mat * FVector3(x1, z1, y1);
		v[1] = mat * FVector3(x2, z1, y2);
		v[2] = mat * FVector3(x1, z2, y1);
		v[3] = mat * FVector3(x2, z2, y2);
	}
	else // traditional "Y" billboard mode
#endif
	{
		v[0] = FVector3(x1, z1, y1);
		v[1] = FVector3(x2, z1, y2);
		v[2] = FVector3(x1, z2, y1);
		v[3] = FVector3(x2, z2, y2);
	}
}

//==========================================================================
//
// 
//
//==========================================================================

void HWSprite::CreateVertices(HWDrawInfo* di)
{
	if (modelframe == 0)
	{
		FVector3 v[4];
		CalculateVertices(di, v, &di->Viewpoint.Pos);
		auto vert = screen->mVertexData->AllocVertices(4);
		auto vp = vert.first;
		vertexindex = vert.second;

		vp[0].Set(v[0][0], v[0][1], v[0][2], ul, vt);
		vp[1].Set(v[1][0], v[1][1], v[1][2], ur, vt);
		vp[2].Set(v[2][0], v[2][1], v[2][2], ul, vb);
		vp[3].Set(v[3][0], v[3][1], v[3][2], ur, vb);
	}
}

//==========================================================================
//
// 
//
//==========================================================================

inline void HWSprite::PutSprite(HWDrawInfo* di, bool translucent)
{
	if (translucent && texture && (hw_int_useindexedcolortextures || !checkTranslucentReplacement(texture->GetID(), palette))) alphaThreshold = texture->alphaThreshold;
	else alphaThreshold = 0;

	/*
	if (modelframe == 1 && gl_light_sprites)
	{
		hw_GetDynModelLight(actor, lightdata);
		dynlightindex = screen->mLights->UploadLights(lightdata);
	}
	else*/
		dynlightindex = -1;

	rendered_sprites++;
	vertexindex = -1;
	if (!screen->BuffersArePersistent())
	{
		CreateVertices(di);
	}
	di->AddSprite(this, translucent);
}

//==========================================================================
//
// 
//
//==========================================================================

void HWSprite::Process(HWDrawInfo* di, tspritetype* spr, sectortype* sector, int thruportal)
{
	if (spr == nullptr)
		return;

	auto tex = tileGetTexture(spr->picnum);
	if (!tex || !tex->isValid()) return;

	texture = tex;
	Sprite = spr;

	modelframe = 0;
	dynlightindex = -1;
	shade = spr->shade;
	palette = spr->pal;
	fade = lookups.getFade(sector->floorpal);	// fog is per sector.
	visibility = sectorVisibility(sector);

	SetSpriteTranslucency(spr, alpha, RenderStyle);

	x = spr->int_pos().X * (1 / 16.f);
	z = spr->int_pos().Z * (1 / -256.f);
	y = spr->int_pos().Y * (1 / -16.f);
	auto vp = di->Viewpoint;

	if ((vp.Pos.XY() - DVector2(x, y)).LengthSquared() < 0.125) return;

	if (modelframe == 0)
	{
		int flags = spr->cstat;
		int tilenum = spr->picnum;

		int xsize, ysize, tilexoff, tileyoff;
		if (hw_hightile && TileFiles.tiledata[tilenum].hiofs.xsize)
		{
			xsize = TileFiles.tiledata[tilenum].hiofs.xsize;
			ysize = TileFiles.tiledata[tilenum].hiofs.ysize;
			tilexoff = TileFiles.tiledata[tilenum].hiofs.xoffs;
			tileyoff = TileFiles.tiledata[tilenum].hiofs.yoffs;
		}
		else
		{
			xsize = (int)tex->GetDisplayWidth();
			ysize = (int)tex->GetDisplayHeight();
			tilexoff = (int)tex->GetDisplayLeftOffset();
			tileyoff = (int)tex->GetDisplayTopOffset();

		}

		int heinum = 0; // tspriteGetSlope(tspr); // todo: slope sprites

		if (heinum == 0)
		{
			tilexoff += spr->xoffset;
			tileyoff += spr->yoffset;
		}

		// convert to render space.
		float width = (xsize * spr->xrepeat) * (0.2f / 16.f); // weird Build fuckery. Face sprites are rendered at 80% width only.
		float height = (ysize * spr->yrepeat) * (0.25f / 16.f);
		float xoff = (tilexoff * spr->xrepeat) * (0.2f / 16.f);
		float yoff = (tileyoff * spr->yrepeat) * (0.25f / 16.f);

		if (xsize & 1) xoff -= spr->xrepeat * (0.1f / 16.f);  // Odd xspans (taken from polymost as-is)

		if (spr->cstat & CSTAT_SPRITE_YCENTER)
		{
			yoff -= height * 0.5;
			if (ysize & 1) yoff -= spr->yrepeat * (0.125f / 16.f);  // Odd yspans (taken from polymost as-is)
		}

		if (flags & CSTAT_SPRITE_XFLIP) xoff = -xoff;
		//if (flags & CSTAT_SPRITE_YFLIP) yoff = -yoff; According to Polymost this must not be done.

		ul = (spr->cstat & CSTAT_SPRITE_XFLIP) ? 0.f : 1.f;
		ur = (spr->cstat & CSTAT_SPRITE_XFLIP) ? 1.f : 0.f;
		vt = (spr->cstat & CSTAT_SPRITE_YFLIP) ? 0.f : 1.f;
		vb = (spr->cstat & CSTAT_SPRITE_YFLIP) ? 1.f : 0.f;
		if (tex->isHardwareCanvas()) std::swap(vt, vb);

		float viewvecX = vp.ViewVector.X;
		float viewvecY = vp.ViewVector.Y;

		x = spr->int_pos().X * (1 / 16.f);
		y = spr->int_pos().Y * (1 / -16.f);
		z = spr->int_pos().Z * (1 / -256.f);

		x1 = x - viewvecY * (xoff - (width * 0.5f));
		x2 = x - viewvecY * (xoff + (width * 0.5f));
		y1 = y + viewvecX * (xoff - (width * 0.5f));
		y2 = y + viewvecX * (xoff + (width * 0.5f));

		z1 = z + yoff;
		z2 = z + height + yoff;
		if (z1 < z2)
		{
			// Make sure that z1 is the higher one. Some utilities expect it to be oriented this way.
			std::swap(z1, z2);
			std::swap(vt, vb);
		}
	}
	else
	{
		x1 = x2 = x;
		y1 = y2 = y;
		z1 = z2 = z;
		texture = nullptr;
	}

	depth = (float)((x - vp.Pos.X) * vp.TanCos + (y - vp.Pos.Y) * vp.TanSin);

#if 0 // huh?
	if ((texture && texture->GetTranslucency()) || (RenderStyle.Flags & STYLEF_RedIsAlpha) || (modelframe && spr->RenderStyle != DefaultRenderStyle()))
	{
		if (hw_styleflags == STYLEHW_Solid)
		{
			RenderStyle.SrcAlpha = STYLEALPHA_Src;
			RenderStyle.DestAlpha = STYLEALPHA_InvSrc;
		}
		hw_styleflags = STYLEHW_NoAlphaTest;
	}
#endif

	PutSprite(di, true);
}


//==========================================================================
//
// 
//
//==========================================================================

bool HWSprite::ProcessVoxel(HWDrawInfo* di, voxmodel_t* vox, tspritetype* spr, sectortype* sector, bool rotate)
{
	Sprite = spr;
	auto ownerActor = spr->ownerActor;

	texture = nullptr;
	modelframe = -1;
	dynlightindex = -1;
	shade = spr->shade;
	palette = spr->pal;
	fade = lookups.getFade(sector->floorpal);	// fog is per sector.
	visibility = sectorVisibility(sector);
	voxel = vox;

	auto ang = spr->ang + ownerActor->sprext.angoff;
	if ((spr->clipdist & TSPR_MDLROTATE) || rotate)
	{
		int myclock = (PlayClock << 3) + MulScale(4 << 3, (int)di->Viewpoint.TicFrac, 16);
		ang = (ang + myclock) & 2047;
	}


	if (!vox || (spr->cstat & CSTAT_SPRITE_ALIGNMENT_MASK) == CSTAT_SPRITE_ALIGNMENT_FLOOR) return false;

	SetSpriteTranslucency(spr, alpha, RenderStyle);


	FVector3 scalevec = { voxel->scale, voxel->scale, voxel->scale };
	FVector3 translatevec = { 0, 0, voxel->zadd * voxel->scale };

	float basescale = voxel->bscale / 64.f;
	float sprxscale = (float)spr->xrepeat * (256.f / 320.f) * basescale;
	if ((spr->ownerActor->spr.cstat & CSTAT_SPRITE_ALIGNMENT_MASK) == CSTAT_SPRITE_ALIGNMENT_WALL)
	{
		sprxscale *= 1.25f;
		translatevec.Y -= spr->xoffset * bcosf(ownerActor->sprext.angoff, -20);
		translatevec.X += spr->xoffset * bsinf(ownerActor->sprext.angoff, -20);
	}

	if (spr->cstat & CSTAT_SPRITE_YFLIP) 
	{ 
		scalevec.Z = -scalevec.Z; 
		translatevec.Z = -translatevec.Z; 
	}
	if (spr->cstat & CSTAT_SPRITE_XFLIP)
	{
		scalevec.X = -scalevec.X;
		translatevec.X = -translatevec.X;
		translatevec.Y = -translatevec.Y;
	}

	scalevec.X *= sprxscale; 
	translatevec.X *= sprxscale; 
	scalevec.Y *= sprxscale; 
	translatevec.Y *= sprxscale;
	float sprzscale = (float)spr->yrepeat * basescale;
	scalevec.Z *= sprzscale; 
	translatevec.Z *= sprzscale;

	float zpos = (float)(spr->int_pos().Z + ownerActor->sprext.position_offset.Z);
	float zscale = ((spr->cstat & CSTAT_SPRITE_YFLIP) && (spr->ownerActor->spr.cstat & CSTAT_SPRITE_ALIGNMENT_MASK) != 0) ? -4.f : 4.f;
	zpos -= (spr->yoffset * spr->yrepeat) * zscale * voxel->bscale;

	x = (spr->int_pos().X + ownerActor->sprext.position_offset.X) * (1 / 16.f);
	z = zpos * (1 / -256.f);
	y = (spr->int_pos().Y + ownerActor->sprext.position_offset.Y) * (1 / -16.f);

	float zoff = voxel->siz.Z * .5f;
	if (!(spr->cstat & CSTAT_SPRITE_YCENTER))
		zoff += voxel->piv.Z;
	else if ((spr->cstat & CSTAT_SPRITE_ALIGNMENT_MASK) != CSTAT_SPRITE_ALIGNMENT_SLAB)
	{
		zoff += voxel->piv.Z;
		zoff -= voxel->siz.Z * .5f;
	}
	if (spr->cstat & CSTAT_SPRITE_YFLIP) zoff = voxel->siz.Z - zoff;

	rotmat.loadIdentity();
	rotmat.translate(x + translatevec.X, z - translatevec.Z, y - translatevec.Y);
	rotmat.rotate(buildang(ang).asdeg() - 90.f, 0, 1, 0);
	rotmat.scale(scalevec.X, scalevec.Z, scalevec.Y);
	// Apply pivot last
	rotmat.translate(-voxel->piv.X, zoff, voxel->piv.Y);



	auto vp = di->Viewpoint;
	depth = (float)((x - vp.Pos.X) * vp.TanCos + (y - vp.Pos.Y) * vp.TanSin);
	PutSprite(di, spriteHasTranslucency(Sprite));
	return true;
}
