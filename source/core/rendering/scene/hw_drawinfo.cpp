// 
//---------------------------------------------------------------------------
//
// Copyright(C) 2000-2018 Christoph Oelckers
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

EXTERN_CVAR(Float, r_visibility)
CVAR(Bool, gl_bandedswlight, false, CVAR_ARCHIVE)
CVAR(Bool, gl_sort_textures, false, CVAR_ARCHIVE | CVAR_GLOBALCONFIG)
CVAR(Bool, gl_no_skyclear, false, CVAR_ARCHIVE | CVAR_GLOBALCONFIG)
CVAR(Int, gl_enhanced_nv_stealth, 3, CVAR_ARCHIVE | CVAR_GLOBALCONFIG)

CVAR(Bool, gl_texture, true, 0)
CVAR(Float, gl_mask_threshold, 0.5f, CVAR_ARCHIVE | CVAR_GLOBALCONFIG)
CVAR(Float, gl_mask_sprite_threshold, 0.5f, CVAR_ARCHIVE | CVAR_GLOBALCONFIG)

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
	staticClipper.Clear();
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
		VPUniforms.mGlobVis = (2 / 65536.f) * g_visibility / r_ambientlight;
		VPUniforms.mPalLightLevels = numshades | (static_cast<int>(gl_fogmode) << 8) | (5 << 16);

		VPUniforms.mClipLine.X = -10000000.0f;
		VPUniforms.mShadowmapFilter = gl_shadowmap_filter;
	}
	vec2_t view = { int(Viewpoint.Pos.X * 16), int(Viewpoint.Pos.Y * -16) };
	mClipper->SetViewpoint(view);

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
	float WidescreenRatio = 1.6666f;	// fixme - this is a placeholder.
	float tilt = fabs(Viewpoint.HWAngles.Pitch.Degrees);

	// If the pitch is larger than this you can look all around at a FOV of 90°
	if (tilt > 46.0f) return 0xffffffff;

	// ok, this is a gross hack that barely works...
	// but at least it doesn't overestimate too much...
	double floatangle = 2.0 + (45.0 + ((tilt / 1.9)))*Viewpoint.FieldOfView.Degrees*48.0 / AspectMultiplier(WidescreenRatio) / 90.0;
	angle_t a1 = DAngle(floatangle).BAMs();
	if (a1 >= ANGLE_180) return 0xffffffff;
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
	VPUniforms.mViewMatrix.rotate(angles.Roll.Degrees, 0.0f, 0.0f, 1.0f);
	VPUniforms.mViewMatrix.rotate(angles.Pitch.Degrees, 1.0f, 0.0f, 0.0f);
	VPUniforms.mViewMatrix.rotate(angles.Yaw.Degrees, 0.0f, mult, 0.0f);
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
	for (int i = 0; i < spritesortcnt; i++)
	{
		auto tspr = &tsprite[i];
		int tilenum = tspr->picnum;
		int spritenum = tspr->owner;

		if (spritenum < 0 || (unsigned)tilenum >= MAXTILES)
			continue;

		setgotpic(tilenum);

		while (!(spriteext[spritenum].flags & SPREXT_NOTMD))
		{
			int pt = Ptile2tile(tspr->picnum, tspr->pal);
			if (hw_models && tile2model[pt].modelid >= 0 && tile2model[pt].framenum >= 0)
			{
				//HWSprite hwsprite;
				//if (hwsprite.ProcessModel(pt, tspr)) return;
				break;
			}

			if (r_voxels)
			{
				if ((tspr->cstat & CSTAT_SPRITE_ALIGNMENT) != CSTAT_SPRITE_ALIGNMENT_SLAB && tiletovox[tspr->picnum] >= 0 && voxmodels[tiletovox[tspr->picnum]])
				{
					//HWSprite hwsprite;
					//if (hwsprite.ProcessVoxel(voxmodels[tiletovox[tspr->picnum]], tspr)) return;
					break;
				}
				else if ((tspr->cstat & CSTAT_SPRITE_ALIGNMENT) == CSTAT_SPRITE_ALIGNMENT_SLAB && tspr->picnum < MAXVOXELS && voxmodels[tspr->picnum])
				{
					//HWSprite hwsprite;
					//hwsprite.ProcessVoxel(voxmodels[tspr->picnum], tspr);
					break;
				}
			}
			break;
		}

		if (spriteext[spritenum].flags & SPREXT_AWAY1)
		{
			tspr->pos.x += bcos(tspr->ang, -13);
			tspr->pos.y += bsin(tspr->ang, -13);
		}
		else if (spriteext[spritenum].flags & SPREXT_AWAY2)
		{
			tspr->pos.x -= bcos(tspr->ang, -13);
			tspr->pos.y -= bsin(tspr->ang, -13);
		}

		tileUpdatePicnum(&tilenum, sprite->owner + 32768, 0);
		tspr->picnum = tilenum;

		switch (tspr->cstat & CSTAT_SPRITE_ALIGNMENT)
		{
		case CSTAT_SPRITE_ALIGNMENT_FACING:
		{
			HWSprite sprite;
			sprite.Process(this, tspr, &sector[tspr->sectnum], false);
			break;
		}

		case CSTAT_SPRITE_ALIGNMENT_WALL:
		{
			HWWall wall;
			wall.ProcessWallSprite(this, tspr, &sector[tspr->sectnum]);
			break;
		}

		case CSTAT_SPRITE_ALIGNMENT_FLOOR:
		{
			HWFlat flat;
			flat.ProcessFlatSprite(this, tspr, &sector[tspr->sectnum]);
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

void HWDrawInfo::CreateScene()
{
	const auto& vp = Viewpoint;

	angle_t a1 = FrustumAngle();
	mClipper->SafeAddClipRange(bamang(vp.RotAngle + a1), bamang(vp.RotAngle - a1));

	// reset the portal manager
	portalState.StartFrame();

	ProcessAll.Clock();

	// clip the scene and fill the drawlists
	screen->mVertexData->Map();
	screen->mLights->Map();

	spritesortcnt = 0;

	vec2_t view = { int(vp.Pos.X * 16), int(vp.Pos.Y * -16) };
	mDrawer.Init(this, mClipper, view);
	if (vp.SectNums)
		mDrawer.RenderScene(vp.SectNums, vp.SectCount);
	else
		mDrawer.RenderScene(&vp.SectCount, 1); 

	SetupSprite.Clock();
	gi->processSprites(view.x, view.y, vp.Pos.Z * -256, bamang(vp.RotAngle), vp.TicFrac * 65536);
	DispatchSprites();
	SetupSprite.Unclock();

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
	const auto &vp = Viewpoint;
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

	state.SetDepthBias(-1, -128);

	// these lists are only wall and floor sprites - often attached to walls and floors - so they need to be offset from the plane they may be attached to.
	drawlists[GLDL_MASKEDWALLSS].DrawWalls(this, state, false);

	// Each list must draw both its passes before the next one to ensure proper depth buffer contents.
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

	state.SetDepthBias(-1, -128);

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

void HWDrawInfo::DrawScene(int drawmode)
{
	static int recursion = 0;
	static int ssao_portals_available = 0;
	const auto& vp = Viewpoint;

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

	CreateScene();
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
	DrawScene(toscreen ? DM_MAINVIEW : DM_OFFSCREEN);
}
