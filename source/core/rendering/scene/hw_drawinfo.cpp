// 
//---------------------------------------------------------------------------
//
// Copyright(C) 2000-2018 Christoph Oelckers
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
** gl_drawinfo.cpp
** Basic scene draw info management class
**
*/

#include "hw_portal.h"
#include "build.h"
#include "hw_renderstate.h"
#include "hw_drawinfo.h"
//#include "models.h"
#include "hw_clock.h"
#include "hw_cvars.h"
#include "hw_viewpointbuffer.h"
#include "flatvertices.h"
#include "hw_lightbuffer.h"
#include "hw_vrmodes.h"
#include "hw_clipper.h"
#include "v_draw.h"
#include "gamecvars.h"
#include "gamestruct.h"
#include "automap.h"
#include "hw_voxels.h"
#include "coreactor.h"

EXTERN_CVAR(Float, r_visibility)
CVAR(Bool, gl_no_skyclear, false, CVAR_ARCHIVE | CVAR_GLOBALCONFIG)

CVAR(Bool, gl_texture, true, 0)
CVAR(Float, gl_mask_threshold, 0.5f, CVAR_ARCHIVE | CVAR_GLOBALCONFIG)
CVAR(Float, gl_mask_sprite_threshold, 0.5f, CVAR_ARCHIVE | CVAR_GLOBALCONFIG)

BitArray gotsector;

//==========================================================================
//
//
//
//==========================================================================

class FDrawInfoList
{
public:
	TDeletingArray<HWDrawInfo *> mList;

	HWDrawInfo * GetNew();
	void Release(HWDrawInfo *);
};


FDrawInfoList di_list;

//==========================================================================
//
// Try to reuse the lists as often as possible as they contain resources that
// are expensive to create and delete.
//
// Note: If multithreading gets used, this class needs synchronization.
//
//==========================================================================

HWDrawInfo *FDrawInfoList::GetNew()
{
	if (mList.Size() > 0)
	{
		HWDrawInfo *di;
		mList.Pop(di);
		return di;
	}
	return new HWDrawInfo();
}

void FDrawInfoList::Release(HWDrawInfo * di)
{
	di->ClearBuffers();
	mList.Push(di);
}

//==========================================================================
//
// Sets up a new drawinfo struct
//
//==========================================================================

HWDrawInfo *HWDrawInfo::StartDrawInfo(HWDrawInfo *parent, FRenderViewpoint &parentvp, HWViewpointUniforms *uniforms)
{
	HWDrawInfo *di = di_list.GetNew();
	di->StartScene(parentvp, uniforms);
	return di;
}


//==========================================================================
//
//
//
//==========================================================================

static Clipper staticClipper;		// Since all scenes are processed sequentially we only need one clipper.
static HWDrawInfo * gl_drawinfo;	// This is a linked list of all active DrawInfos and needed to free the memory arena after the last one goes out of scope.

void HWDrawInfo::StartScene(FRenderViewpoint& parentvp, HWViewpointUniforms* uniforms)
{
	mClipper = &staticClipper;

	Viewpoint = parentvp;
	//lightmode = Level->lightMode;
	if (uniforms)
	{
		VPUniforms = *uniforms;
		// The clip planes will never be inherited from the parent drawinfo.
		VPUniforms.mClipLine.X = -1000001.f;
		VPUniforms.mClipHeight = 0;
	}
	else
	{
		VPUniforms.mProjectionMatrix.loadIdentity();
		VPUniforms.mViewMatrix.loadIdentity();
		VPUniforms.mNormalViewMatrix.loadIdentity();
		//VPUniforms.mViewHeight = viewheight;
		VPUniforms.mGlobVis = (2 / 65536.f) * (g_visibility + g_relvisibility) / r_ambientlight;
		VPUniforms.mPalLightLevels = numshades | (static_cast<int>(gl_fogmode) << 8);
		if (isBuildSoftwareLighting()) VPUniforms.mPalLightLevels |= (5 << 16);

		VPUniforms.mClipLine.X = -10000000.0f;
		VPUniforms.mShadowmapFilter = gl_shadowmap_filter;
	}
	vec2_t view = { int(Viewpoint.Pos.X * 16), int(Viewpoint.Pos.Y * -16) };

	ClearBuffers();

	for (int i = 0; i < GLDL_TYPES; i++) drawlists[i].Reset();
	vpIndex = 0;

	// Fullbright information needs to be propagated from the main view.
	if (outer != nullptr) FullbrightFlags = outer->FullbrightFlags;
	else FullbrightFlags = 0;

	outer = gl_drawinfo;
	gl_drawinfo = this;

}

//==========================================================================
//
//
//
//==========================================================================

HWDrawInfo *HWDrawInfo::EndDrawInfo()
{
	assert(this == gl_drawinfo);
	for (int i = 0; i < GLDL_TYPES; i++) drawlists[i].Reset();
	gl_drawinfo = outer;
	di_list.Release(this);
	if (gl_drawinfo == nullptr)
		ResetRenderDataAllocator();
	return gl_drawinfo;
}


//==========================================================================
//
//
//
//==========================================================================

void HWDrawInfo::ClearBuffers()
{
	spriteindex = 0;
	mClipPortal = nullptr;
	mCurrentPortal = nullptr;
}

//-----------------------------------------------------------------------------
//
// R_FrustumAngle
//
//-----------------------------------------------------------------------------

angle_t HWDrawInfo::FrustumAngle()
{
	float WidescreenRatio = (float)screen->GetWidth() / screen->GetHeight();
	float tilt = fabs(Viewpoint.HWAngles.Pitch.Degrees());

	// If the pitch is larger than this you can look all around at a FOV of 90°
	if (tilt > 46.0f) return 0xffffffff;

	// ok, this is a gross hack that barely works...
	// but at least it doesn't overestimate too much...
	double floatangle = 2.0 + (45.0 + ((tilt / 1.9)))*Viewpoint.FieldOfView.Degrees() * 48.0 / AspectMultiplier(WidescreenRatio) / 90.0;
	angle_t a1 = DAngle::fromDeg(floatangle).BAMs();
	if (a1 >= ANGLE_90) return 0xffffffff; // it's either below 90 or bust.
	return a1;
}

//-----------------------------------------------------------------------------
//
// Setup the modelview matrix
//
//-----------------------------------------------------------------------------

void HWDrawInfo::SetViewMatrix(const FRotator &angles, float vx, float vy, float vz, bool mirror, bool planemirror)
{
	float mult = mirror ? -1.f : 1.f;
	float planemult = planemirror ? -1 : 1;// Level->info->pixelstretch : Level->info->pixelstretch;

	VPUniforms.mViewMatrix.loadIdentity();
	VPUniforms.mViewMatrix.rotate(angles.Roll.Degrees(), 0.0f, 0.0f, 1.0f);
	VPUniforms.mViewMatrix.rotate(angles.Pitch.Degrees(), 1.0f, 0.0f, 0.0f);
	VPUniforms.mViewMatrix.rotate(angles.Yaw.Degrees(), 0.0f, mult, 0.0f);
	VPUniforms.mViewMatrix.translate(vx * mult, -vz * planemult, -vy);
	VPUniforms.mViewMatrix.scale(-mult, planemult, 1);
}


//-----------------------------------------------------------------------------
//
// SetupView
// Setup the view rotation matrix for the given viewpoint
//
//-----------------------------------------------------------------------------
void HWDrawInfo::SetupView(FRenderState &state, float vx, float vy, float vz, bool mirror, bool planemirror)
{
	auto &vp = Viewpoint;
	//vp.SetViewAngle(r_viewwindow); // todo: need to pass in.
	SetViewMatrix(vp.HWAngles, vx, vy, vz, mirror, planemirror);
	SetCameraPos(vp.Pos);
	VPUniforms.CalcDependencies();
	vpIndex = screen->mViewpoints->SetViewpoint(state, &VPUniforms);
}

//-----------------------------------------------------------------------------
//
//
//
//-----------------------------------------------------------------------------

HWPortal * HWDrawInfo::FindPortal(const void * src)
{
	int i = Portals.Size() - 1;

	while (i >= 0 && Portals[i] && Portals[i]->GetSource() != src) i--;
	return i >= 0 ? Portals[i] : nullptr;
}

//-----------------------------------------------------------------------------
//
//
//
//-----------------------------------------------------------------------------

void HWDrawInfo::DispatchSprites()
{
	for (unsigned i = 0; i < tsprites.Size(); i++)
	{
		auto tspr = tsprites.get(i);
		int tilenum = tspr->picnum;
		auto actor = tspr->ownerActor;

		if (actor == nullptr || tspr->xrepeat == 0 || tspr->yrepeat == 0 || (unsigned)tilenum >= MAXTILES)
			continue;

		actor->spr.cstat2 |= CSTAT2_SPRITE_MAPPED;

		if ((tspr->cstat & CSTAT_SPRITE_ALIGNMENT_MASK) != CSTAT_SPRITE_ALIGNMENT_SLAB)
			tileUpdatePicnum(&tilenum, false, (actor->GetIndex() & 16383));
		tspr->picnum = tilenum;
		gotpic.Set(tilenum);

		if (!(actor->sprext.renderflags & SPREXT_NOTMD))
		{
			int pt = Ptile2tile(tilenum, tspr->pal);
			if (hw_models && tile2model[pt].modelid >= 0 && tile2model[pt].framenum >= 0)
			{
				//HWSprite hwsprite;
				//if (hwsprite.ProcessModel(pt, tspr)) continue;
			}
			if (r_voxels)
			{
				if ((tspr->cstat & CSTAT_SPRITE_ALIGNMENT_MASK) != CSTAT_SPRITE_ALIGNMENT_SLAB && tiletovox[tilenum] >= 0 && voxmodels[tiletovox[tilenum]])
				{
					HWSprite hwsprite;
					int num = tiletovox[tilenum];
					if (hwsprite.ProcessVoxel(this, voxmodels[num], tspr, tspr->sectp, voxrotate[num])) 
						continue;
				}
				else if ((tspr->cstat & CSTAT_SPRITE_ALIGNMENT_MASK) == CSTAT_SPRITE_ALIGNMENT_SLAB && tspr->picnum < MAXVOXELS && voxmodels[tilenum])
				{
					HWSprite hwsprite;
					int num = tilenum;
					hwsprite.ProcessVoxel(this, voxmodels[tspr->picnum], tspr, tspr->sectp, voxrotate[num]);
					continue;
				}
			}
		}

		if (actor->sprext.renderflags & SPREXT_AWAY1)
		{
			tspr->add_int_x(bcos(tspr->ang, -13));
			tspr->add_int_y(bsin(tspr->ang, -13));
		}
		else if (actor->sprext.renderflags & SPREXT_AWAY2)
		{
			tspr->add_int_x(-bcos(tspr->ang, -13));
			tspr->add_int_y(-bsin(tspr->ang, -13));
		}

		switch (tspr->cstat & CSTAT_SPRITE_ALIGNMENT_MASK)
		{
		case CSTAT_SPRITE_ALIGNMENT_FACING:
		{
			HWSprite sprite;
			sprite.Process(this, tspr, tspr->sectp, false);
			break;
		}

		case CSTAT_SPRITE_ALIGNMENT_WALL:
		{
			HWWall wall;
			wall.ProcessWallSprite(this, tspr, tspr->sectp);
			break;
		}

		case CSTAT_SPRITE_ALIGNMENT_FLOOR:
		{
			HWFlat flat;
			flat.ProcessFlatSprite(this, tspr, tspr->sectp);
			break;
		}

		default:
			break;
		}
	}
}
//-----------------------------------------------------------------------------
//
// CreateScene
//
// creates the draw lists for the current scene
//
//-----------------------------------------------------------------------------

void HWDrawInfo::CreateScene(bool portal)
{
	const auto& vp = Viewpoint;

	angle_t a1 = FrustumAngle();

	// reset the portal manager
	portalState.StartFrame();

	ProcessAll.Clock();

	// clip the scene and fill the drawlists
	screen->mVertexData->Map();
	screen->mLights->Map();

	tsprites.clear();
	ingeo = false;
	geoofs = { 0,0 };

	vec2_t view = { int(vp.Pos.X * 16), int(vp.Pos.Y * -16) };

	if(!portal) mClipper->SetVisibleRange(vp.RotAngle, a1);

	if (a1 != 0xffffffff) mDrawer.Init(this, mClipper, view, bamang(vp.RotAngle - a1), bamang(vp.RotAngle + a1));
	else mDrawer.Init(this, mClipper, view, bamang(0), bamang(0));
	if (vp.SectNums)
		mDrawer.RenderScene(vp.SectNums, vp.SectCount, portal);
	else
		mDrawer.RenderScene(&vp.SectCount, 1, portal);

	SetupSprite.Clock();
	gi->processSprites(tsprites, view.X, view.Y, vp.Pos.Z * -256, bamang(vp.RotAngle), vp.TicFrac * 65536);
	DispatchSprites();
	SetupSprite.Unclock();

	GeoEffect eff;
	int effsect = vp.SectNums ? vp.SectNums[0] : vp.SectCount;
	auto drawsectp = &sector[effsect];
	auto orgdrawsectp = drawsectp;
	// RR geometry hack. Ugh...
	// This just adds to the existing render list, so we must offset the effect areas to the same xy-space as the main one as we cannot change the view matrix.
	if (gi->GetGeoEffect(&eff, drawsectp))
	{
		ingeo = true;
		geoofs = { (float)eff.geox[0], (float)eff.geoy[0] };
		// process the first layer.
		for (int i = 0; i < eff.geocnt; i++)
		{
			auto sect = eff.geosectorwarp[i];
			for (auto w = 0; w < sect->wallnum; w++)
			{
				auto wal = sect->firstWall() + w;
				wal->pos.X += eff.geox[i];
				wal->pos.Y += eff.geoy[i];
			}
			sect->dirty = EDirty::AllDirty;
			if (eff.geosector[i] == drawsectp) drawsectp = eff.geosectorwarp[i];
		}

		if (a1 != 0xffffffff) mDrawer.Init(this, mClipper, view, bamang(vp.RotAngle - a1), bamang(vp.RotAngle + a1));
		else mDrawer.Init(this, mClipper, view, bamang(0), bamang(0));

		int drawsect = sectnum(drawsectp);
		mDrawer.RenderScene(&drawsect, 1, false);

		for (int i = 0; i < eff.geocnt; i++)
		{
			auto sect = eff.geosectorwarp[i];
			for (auto w = 0; w < sect->wallnum; w++)
			{
				auto wal = sect->firstWall() + w;
				wal->pos.X -= eff.geox[i];
				wal->pos.Y -= eff.geoy[i];
			}
		}

		// Now the second layer. Same shit, different arrays.
		geoofs = { (float)eff.geox2[0], (float)eff.geoy2[0] };
		for (int i = 0; i < eff.geocnt; i++)
		{
			auto sect = eff.geosectorwarp2[i];
			for (auto w = 0; w < sect->wallnum; w++)
			{
				auto wal = sect->firstWall() + w;
				wal->pos.X += eff.geox2[i];
				wal->pos.Y += eff.geoy2[i];
			}
			sect->dirty = EDirty::AllDirty;
			if (eff.geosector[i] == orgdrawsectp) drawsectp = eff.geosectorwarp2[i];
		}

		if (a1 != 0xffffffff) mDrawer.Init(this, mClipper, view, bamang(vp.RotAngle - a1), bamang(vp.RotAngle + a1));
		else mDrawer.Init(this, mClipper, view, bamang(0), bamang(0));
		drawsect = sectnum(drawsectp);
		mDrawer.RenderScene(&drawsect, 1, false);

		for (int i = 0; i < eff.geocnt; i++)
		{
			auto sect = eff.geosectorwarp2[i];
			for (auto w = 0; w < sect->wallnum; w++)
			{
				auto wal = sect->firstWall() + w;
				wal->pos.X -= eff.geox2[i];
				wal->pos.Y -= eff.geoy2[i];
			}
		}
		ingeo = false;
	}


	screen->mLights->Unmap();
	screen->mVertexData->Unmap();

	ProcessAll.Unclock();

}

//-----------------------------------------------------------------------------
//
// RenderScene
//
// Draws the current draw lists for the non GLSL renderer
//
//-----------------------------------------------------------------------------

void HWDrawInfo::RenderScene(FRenderState &state)
{
	RenderAll.Clock();

	state.SetDepthMask(true);

	state.EnableFog(true);
	state.SetRenderStyle(STYLE_Source);


	// Part 1: solid geometry. This is set up so that there are no transparent parts
	state.SetDepthFunc(DF_Less);
	state.AlphaFunc(Alpha_GEqual, 0.f);
	state.ClearDepthBias();

	state.EnableTexture(gl_texture);
	state.EnableBrightmap(true);
	drawlists[GLDL_PLAINWALLS].DrawWalls(this, state, false);

	drawlists[GLDL_PLAINFLATS].DrawFlats(this, state, false);


	// Part 2: masked geometry. This is set up so that only pixels with alpha>gl_mask_threshold will show
	state.AlphaFunc(Alpha_GEqual, gl_mask_threshold);

	// This list is masked, non-translucent walls.
	drawlists[GLDL_MASKEDWALLS].DrawWalls(this, state, false);

	// These lists must be drawn in two passes for color and depth to avoid depth fighting with overlapping entries
	drawlists[GLDL_MASKEDFLATS].SortFlats(this);
	drawlists[GLDL_MASKEDWALLSV].SortWallsHorz(this);
	drawlists[GLDL_MASKEDWALLSH].SortWallsVert(this);
	drawlists[GLDL_MASKEDWALLSD].SortWallsDiag(this);


	// these lists are only wall and floor sprites - often attached to walls and floors - so they need to be offset from the plane they may be attached to.
	drawlists[GLDL_MASKEDWALLSS].DrawWalls(this, state, false);

	// Each list must draw both its passes before the next one to ensure proper depth buffer contents.
	auto& list = drawlists[GLDL_MASKEDWALLSD].drawitems;
	unsigned i = 0;
	RenderWall.Clock();
	while (i < list.Size())
	{
		unsigned j;
		auto check = drawlists[GLDL_MASKEDWALLSD].walls[list[i].index]->walldist;
		state.SetDepthMask(false);
		for (j = i; j < list.Size() && drawlists[GLDL_MASKEDWALLSD].walls[list[j].index]->walldist == check; j++)
		{
			drawlists[GLDL_MASKEDWALLSD].walls[list[j].index]->DrawWall(this, state, false);
		}
		state.SetDepthMask(true);
		for (unsigned k = i; k < j; k++)
		{
			drawlists[GLDL_MASKEDWALLSD].walls[list[k].index]->DrawWall(this, state, false);
		}
		i = j;
	}
	RenderWall.Unclock();

	state.SetDepthMask(false);
	drawlists[GLDL_MASKEDWALLSD].DrawWalls(this, state, false);
	state.SetDepthMask(true);
	state.SetColorMask(false);
	drawlists[GLDL_MASKEDWALLSD].DrawWalls(this, state, false);
	state.SetColorMask(true);

	state.SetDepthMask(false);
	drawlists[GLDL_MASKEDWALLSV].DrawWalls(this, state, false);
	state.SetDepthMask(true);
	state.SetColorMask(false);
	drawlists[GLDL_MASKEDWALLSV].DrawWalls(this, state, false);
	state.SetColorMask(true);

	state.SetDepthMask(false);
	drawlists[GLDL_MASKEDWALLSH].DrawWalls(this, state, false);
	state.SetDepthMask(true);
	state.SetColorMask(false);
	drawlists[GLDL_MASKEDWALLSH].DrawWalls(this, state, false);
	state.SetColorMask(true);

	state.SetDepthBias(-1, -128);
	drawlists[GLDL_MASKEDSLOPEFLATS].DrawFlats(this, state, false);
	state.SetDepthMask(false);
	drawlists[GLDL_MASKEDFLATS].DrawFlats(this, state, false);
	state.SetDepthMask(true);
	state.SetColorMask(false);
	drawlists[GLDL_MASKEDFLATS].DrawFlats(this, state, false);
	state.SetColorMask(true);
	state.ClearDepthBias();

	drawlists[GLDL_MODELS].Draw(this, state, false);

	state.SetRenderStyle(STYLE_Translucent);

	state.SetDepthFunc(DF_LEqual);
	RenderAll.Unclock();
}

//-----------------------------------------------------------------------------
//
// RenderTranslucent
//
//-----------------------------------------------------------------------------

void HWDrawInfo::RenderTranslucent(FRenderState &state)
{
	RenderAll.Clock();

	state.SetDepthBias(-1, -160);

	// final pass: translucent stuff
	state.AlphaFunc(Alpha_GEqual, gl_mask_sprite_threshold);
	state.SetRenderStyle(STYLE_Translucent);

	state.EnableBrightmap(true);
	drawlists[GLDL_TRANSLUCENTBORDER].Draw(this, state, true);
	state.SetDepthMask(false);

	drawlists[GLDL_TRANSLUCENT].DrawSorted(this, state);
	state.EnableBrightmap(false);

	state.ClearDepthBias();
	state.AlphaFunc(Alpha_GEqual, 0.5f);
	state.SetDepthMask(true);

	RenderAll.Unclock();
}


//-----------------------------------------------------------------------------
//
// RenderTranslucent
//
//-----------------------------------------------------------------------------

void HWDrawInfo::RenderPortal(HWPortal *p, FRenderState &state, bool usestencil)
{
	auto gp = static_cast<HWPortal *>(p);
	gp->SetupStencil(this, state, usestencil);
	auto new_di = StartDrawInfo(this, Viewpoint, &VPUniforms);
	new_di->visibility = visibility;
	new_di->rellight = rellight;
	new_di->mCurrentPortal = gp;
	state.SetLightIndex(-1);
	gp->DrawContents(new_di, state);
	new_di->EndDrawInfo();
	state.SetVertexBuffer(screen->mVertexData);
	screen->mViewpoints->Bind(state, vpIndex);
	gp->RemoveStencil(this, state, usestencil);
}

//-----------------------------------------------------------------------------
//
// Draws player sprites and color blend
//
//-----------------------------------------------------------------------------


void HWDrawInfo::EndDrawScene(FRenderState &state)
{
	state.EnableFog(false);

#if 0
	// [BB] HUD models need to be rendered here. 
	const bool renderHUDModel = IsHUDModelForPlayerAvailable(players[consoleplayer].camera->player);
	if (renderHUDModel)
	{
		// [BB] The HUD model should be drawn over everything else already drawn.
		state.Clear(CT_Depth);
		DrawPlayerSprites(true, state);
	}
#endif

	state.EnableStencil(false);
	state.SetViewport(screen->mScreenViewport.left, screen->mScreenViewport.top, screen->mScreenViewport.width, screen->mScreenViewport.height);

	// Restore standard rendering state
	state.SetRenderStyle(STYLE_Translucent);
	state.ResetColor();
	state.EnableTexture(true);
	state.SetScissor(0, 0, -1, -1);
}


//-----------------------------------------------------------------------------
//
// sets 3D viewport and initial state
//
//-----------------------------------------------------------------------------

void HWDrawInfo::Set3DViewport(FRenderState &state)
{
	// Always clear all buffers with scissor test disabled.
	// This is faster on newer hardware because it allows the GPU to skip
	// reading from slower memory where the full buffers are stored.
	state.SetScissor(0, 0, -1, -1);
	state.Clear(CT_Color | CT_Depth | CT_Stencil);

	const auto &bounds = screen->mSceneViewport;
	state.SetViewport(bounds.left, bounds.top, bounds.width, bounds.height);
	state.SetScissor(bounds.left, bounds.top, bounds.width, bounds.height);
	state.EnableMultisampling(true);
	state.EnableDepthTest(true);
	state.EnableStencil(true);
	state.SetStencil(0, SOP_Keep, SF_AllOn);
}

//-----------------------------------------------------------------------------
//
// gl_drawscene - this function renders the scene from the current
// viewpoint, including mirrors and skyboxes and other portals
// It is assumed that the HWPortal::EndFrame returns with the 
// stencil, z-buffer and the projection matrix intact!
//
//-----------------------------------------------------------------------------

void HWDrawInfo::DrawScene(int drawmode, bool portal)
{
	static int recursion = 0;
	static int ssao_portals_available = 0;

	bool applySSAO = false;
	if (drawmode == DM_MAINVIEW)
	{
		ssao_portals_available = gl_ssao_portals;
		applySSAO = true;
	}
	else if (drawmode == DM_OFFSCREEN)
	{
		ssao_portals_available = 0;
	}
	else if (drawmode == DM_PORTAL && ssao_portals_available > 0)
	{
		applySSAO = (mCurrentPortal->AllowSSAO()/* || Level->flags3&LEVEL3_SKYBOXAO*/);
		ssao_portals_available--;
	}

	CreateScene(portal);
	auto& RenderState = *screen->RenderState();

	RenderState.SetDepthMask(true);

	if (!gl_no_skyclear) portalState.RenderFirstSkyPortal(recursion, this, RenderState);

	RenderScene(RenderState);

	if (applySSAO && RenderState.GetPassType() == GBUFFER_PASS)
	{
		screen->AmbientOccludeScene(VPUniforms.mProjectionMatrix.get()[5]);
		screen->mViewpoints->Bind(RenderState, vpIndex);
	}

	// Handle all portals after rendering the opaque objects but before
	// doing all translucent stuff
	recursion++;
	portalState.EndFrame(this, RenderState);
	recursion--;
	RenderTranslucent(RenderState);
}


//-----------------------------------------------------------------------------
//
// R_RenderView - renders one view - either the screen or a camera texture
//
//-----------------------------------------------------------------------------

void HWDrawInfo::ProcessScene(bool toscreen)
{
	portalState.BeginScene();
	DrawScene(toscreen ? DM_MAINVIEW : DM_OFFSCREEN, false);
	if (toscreen && isBlood())
	{
		gotsector = mDrawer.GotSector(); // Blood needs this to implement some lighting effect hacks. Needs to be refactored to use better info.
	}
}
