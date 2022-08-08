// 
//---------------------------------------------------------------------------
//
// Copyright(C) 2004-2018 Christoph Oelckers
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
** hw_portal.cpp
**   portal maintenance classes for skyboxes, horizons etc. (API independent parts)
**
*/

#include "c_dispatch.h"
#include "hw_portal.h"
#include "hw_clipper.h"
#include "hw_renderstate.h"
#include "flatvertices.h"
#include "hw_clock.h"
#include "texturemanager.h"
#include "gamestruct.h"

EXTERN_CVAR(Int, r_mirror_recursions)
EXTERN_CVAR(Bool, gl_portals)

//-----------------------------------------------------------------------------
//
// StartFrame
//
//-----------------------------------------------------------------------------

void FPortalSceneState::StartFrame()
{
	if (renderdepth == 0)
	{
		inskybox = false;
		screen->instack[plane_floor] = screen->instack[plane_ceiling] = 0;
	}
	renderdepth++;
}

//-----------------------------------------------------------------------------
//
// printing portal info
//
//-----------------------------------------------------------------------------

static bool gl_portalinfo;

CCMD(gl_portalinfo)
{
	gl_portalinfo = true;
}

static FString indent;
FPortalSceneState portalState;

//-----------------------------------------------------------------------------
//
// EndFrame
//
//-----------------------------------------------------------------------------

void FPortalSceneState::EndFrame(HWDrawInfo *di, FRenderState &state)
{
	HWPortal * p;

	if (gl_portalinfo)
	{
		Printf("%s%d portals, depth = %d\n%s{\n", indent.GetChars(), di->Portals.Size(), renderdepth, indent.GetChars());
		indent += "  ";
	}

	while (di->Portals.Pop(p) && p)
	{
		if (gl_portalinfo) 
		{
			Printf("%sProcessing %s, depth = %d\n", indent.GetChars(), p->GetName(), renderdepth);
		}
		if (p->lines.Size() > 0)
		{
			RenderPortal(p, state, true, di);
		}
		delete p;
	}
	renderdepth--;

	if (gl_portalinfo)
	{
		indent.Truncate(long(indent.Len()-2));
		Printf("%s}\n", indent.GetChars());
		if (indent.Len() == 0) gl_portalinfo = false;
	}
}


//-----------------------------------------------------------------------------
//
// Renders one sky portal without a stencil. Only useful if this is the only portal in view.
//
//-----------------------------------------------------------------------------
bool FPortalSceneState::RenderFirstSkyPortal(int recursion, HWDrawInfo *outer_di, FRenderState &state)
{
	HWPortal* best = nullptr;
	unsigned bestindex = 0;

	// Find the one with the highest amount of lines.
	// Normally this is also the one that saves the largest amount
	// of time by drawing it before the scene itself.
	auto& portals = outer_di->Portals;
	for (int i = portals.Size() - 1; i >= 0; --i)
	{
		auto p = portals[i];
		if (p->lines.Size() > 0 && p->IsSky())
		{
			// Cannot clear the depth buffer inside a portal recursion
			if (recursion && p->NeedDepthBuffer()) continue;

			if (!best || p->lines.Size() > best->lines.Size())
			{
				best = p;
				bestindex = i;
			}

			// If the portal area contains the current camera viewpoint, let's always use it because it's likely to give the largest area.
			if (p->boundingBox.contains(outer_di->Viewpoint.Pos))
			{
				best = p;
				bestindex = i;
				break;
			}
		}
	}

	if (best)
	{
		portals.Delete(bestindex);
		RenderPortal(best, state, false, outer_di);
		delete best;
		return true;
	}
	return false;
}


void FPortalSceneState::RenderPortal(HWPortal *p, FRenderState &state, bool usestencil, HWDrawInfo *outer_di)
{
	if (gl_portals) outer_di->RenderPortal(p, state, usestencil);
}


//-----------------------------------------------------------------------------
//
// DrawPortalStencil
//
//-----------------------------------------------------------------------------

void HWPortal::DrawPortalStencil(FRenderState &state, int pass)
{
	if (mPrimIndices.Size() == 0)
	{
		mPrimIndices.Resize(2 * lines.Size());

		for (unsigned int i = 0; i < lines.Size(); i++)
		{
			mPrimIndices[i * 2] = lines[i].vertindex;
			mPrimIndices[i * 2 + 1] = lines[i].vertcount;
		}

		if (NeedCap() && lines.Size() > 1 && planesused != 0)
		{
			screen->mVertexData->Map();
			if (planesused & (1 << plane_floor))
			{
				auto verts = screen->mVertexData->AllocVertices(4);
				auto ptr = verts.first;
				ptr[0].Set((float)boundingBox.left, -32767.f, (float)boundingBox.top, 0, 0);
				ptr[1].Set((float)boundingBox.right, -32767.f, (float)boundingBox.top, 0, 0);
				ptr[2].Set((float)boundingBox.left, -32767.f, (float)boundingBox.bottom, 0, 0);
				ptr[3].Set((float)boundingBox.right, -32767.f, (float)boundingBox.bottom, 0, 0);
				mBottomCap = verts.second;
			}
			if (planesused & (1 << plane_ceiling))
			{
				auto verts = screen->mVertexData->AllocVertices(4);
				auto ptr = verts.first;
				ptr[0].Set((float)boundingBox.left, 32767.f, (float)boundingBox.top, 0, 0);
				ptr[1].Set((float)boundingBox.right, 32767.f, (float)boundingBox.top, 0, 0);
				ptr[2].Set((float)boundingBox.left, 32767.f, (float)boundingBox.bottom, 0, 0);
				ptr[3].Set((float)boundingBox.right, 32767.f, (float)boundingBox.bottom, 0, 0);
				mTopCap = verts.second;
			}
			screen->mVertexData->Unmap();
		}

	}

	for (unsigned int i = 0; i < mPrimIndices.Size(); i += 2)
	{
		state.Draw(DT_TriangleFan, mPrimIndices[i], mPrimIndices[i + 1], i == 0);
	}
	if (NeedCap() && lines.Size() > 1)
	{
		// The cap's depth handling needs special treatment so that it won't block further portal caps.
		if (pass == STP_DepthRestore) state.SetDepthRange(1, 1);

		if (mBottomCap != ~0u)
		{
			state.Draw(DT_TriangleStrip, mBottomCap, 4, false);
		}
		if (mTopCap != ~0u)
		{
			state.Draw(DT_TriangleStrip, mTopCap, 4, false);
		}

		if (pass == STP_DepthRestore) state.SetDepthRange(0, 1);
	}
}


//-----------------------------------------------------------------------------
//
// Start
//
//-----------------------------------------------------------------------------

void HWPortal::SetupStencil(HWDrawInfo *di, FRenderState &state, bool usestencil)
{
	Clocker c(PortalAll);

	rendered_portals++;

	if (usestencil)
	{
		// Create stencil
		state.SetStencil(0, SOP_Increment);	// create stencil, increment stencil of valid pixels
		state.SetColorMask(false);
		state.SetEffect(EFF_STENCIL);
		state.EnableTexture(false);
		state.ResetColor();
		state.SetDepthFunc(DF_Less);

		if (NeedDepthBuffer())
		{
			state.SetDepthMask(false);							// don't write to Z-buffer!

			DrawPortalStencil(state, STP_Stencil);

			// Clear Z-buffer
			state.SetStencil(1, SOP_Keep); // draw sky into stencil. This stage doesn't modify the stencil.
			state.SetDepthMask(true);							// enable z-buffer again
			state.SetDepthRange(1, 1);
			state.SetDepthFunc(DF_Always);
			DrawPortalStencil(state, STP_DepthClear);

			// set normal drawing mode
			state.EnableTexture(true);
			state.SetDepthRange(0, 1);
			state.SetDepthFunc(DF_Less);
			state.SetColorMask(true);
			state.SetEffect(EFF_NONE);
		}
		else
		{
			// No z-buffer is needed therefore we can skip all the complicated stuff that is involved
			// Note: We must draw the stencil with z-write enabled here because there is no second pass!

			state.SetDepthMask(true);
			DrawPortalStencil(state, STP_AllInOne);
			state.SetStencil(1, SOP_Keep); // draw sky into stencil. This stage doesn't modify the stencil.
			state.EnableTexture(true);
			state.SetColorMask(true);
			state.SetEffect(EFF_NONE);
			state.EnableDepthTest(false);
			state.SetDepthMask(false);							// don't write to Z-buffer!
		}
		screen->stencilValue++;


	}
	else
	{
		if (!NeedDepthBuffer())
		{
			state.SetDepthMask(false);
			state.EnableDepthTest(false);
		}
	}

	// save viewpoint
	//savedvisibility = di->Viewpoint. 
}


//-----------------------------------------------------------------------------
//
// End
//
//-----------------------------------------------------------------------------
void HWPortal::RemoveStencil(HWDrawInfo *di, FRenderState &state, bool usestencil)
{
	Clocker c(PortalAll);
	bool needdepth = NeedDepthBuffer();

	// Restore the old view

	if (usestencil)
	{

		state.SetColorMask(false);						// no graphics
		state.SetEffect(EFF_NONE);
		state.ResetColor();
		state.EnableTexture(false);

		if (needdepth)
		{
			// first step: reset the depth buffer to max. depth
			state.SetDepthRange(1, 1);							// always
			state.SetDepthFunc(DF_Always);						// write the farthest depth value
			DrawPortalStencil(state, STP_DepthClear);
		}
		else
		{
			state.EnableDepthTest(true);
		}

		// second step: restore the depth buffer to the previous values and reset the stencil
		state.SetDepthFunc(DF_LEqual);
		state.SetDepthRange(0, 1);
		state.SetStencil(0, SOP_Decrement);
		DrawPortalStencil(state, STP_DepthRestore);
		state.SetDepthFunc(DF_Less);


		state.EnableTexture(true);
		state.SetEffect(EFF_NONE);
		state.SetColorMask(true);
		screen->stencilValue--;

		// restore old stencil op.
		state.SetStencil(0, SOP_Keep);
	}
	else
	{
		if (needdepth)
		{
			state.Clear(CT_Depth);
		}
		else
		{
			state.EnableDepthTest(true);
			state.SetDepthMask(true);
		}

		// This draws a valid z-buffer into the stencil's contents to ensure it
		// doesn't get overwritten by the level's geometry.

		state.ResetColor();
		state.SetDepthFunc(DF_LEqual);
		state.SetDepthRange(0, 1);
		state.SetColorMask(0, 0, 0, 1); // mark portal in alpha channel but don't touch color
		state.SetEffect(EFF_STENCIL);
		state.EnableTexture(false);
		state.SetRenderStyle(STYLE_Source);
		DrawPortalStencil(state, STP_DepthRestore);
		state.SetEffect(EFF_NONE);
		state.EnableTexture(true);
		state.SetColorMask(true);
		state.SetDepthFunc(DF_Less);
	}
}


//-----------------------------------------------------------------------------
//
// 
//
//-----------------------------------------------------------------------------

void HWScenePortalBase::DrawContents(HWDrawInfo* di, FRenderState& state)
{
	if (Setup(di, state, di->mClipper))
	{
		auto type = GetType();
		gi->EnterPortal(di->Viewpoint.CameraActor, type);
		di->DrawScene(DM_PORTAL, type == PORTAL_SECTOR_CEILING);
		gi->LeavePortal(di->Viewpoint.CameraActor, type);
		Shutdown(di, state);
	}
	else state.ClearScreen();
}

//-----------------------------------------------------------------------------
//
// 
//
//-----------------------------------------------------------------------------

void HWScenePortalBase::ClearClipper(HWDrawInfo *di, Clipper *clipper)
{
	clipper->SetVisibleRange(di->Viewpoint.RotAngle, di->FrustumAngle());
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//
//
// Common code for line to line and mirror portals
//
//
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

inline int P_GetLineSide(const DVector2& pos, const walltype* line)
{
	auto delta = WallDelta(line);
	double v = (pos.Y - WallStartY(line) * delta.X + WallStartX(line) - pos.X) * delta.Y;
	return v < -1. / 65536. ? -1 : v > 1. / 65536 ? 1 : 0;
}

bool P_ClipLineToPortal(walltype* line, walltype* portal, DVector2 view)
{
	int behind1 = P_GetLineSide(WallStart(line), portal);
	int behind2 = P_GetLineSide(WallEnd(line), portal);

	if (behind1 == 0 && behind2 == 0)
	{
		// The line is parallel to the portal and cannot possibly be visible.
		return true;
	}
	// If one point lies on the same straight line than the portal, the other vertex will determine sidedness alone.
	else if (behind2 == 0) behind2 = behind1;
	else if (behind1 == 0) behind1 = behind2;

	if (behind1 > 0 && behind2 > 0)
	{
		// The line is behind the portal, i.e. between viewer and portal line, and must be rejected
		return true;
	}
	else if (behind1 < 0 && behind2 < 0)
	{
		// The line is in front of the portal, i.e. the portal is between viewer and line. This line must not be rejected
		return false;
	}
	else
	{
		// The line intersects with the portal straight, so we need to do another check to see how both ends of the portal lie in relation to the viewer.
		int viewside = P_GetLineSide(view, line);
		int p1side = P_GetLineSide(WallStart(portal), line);
		int p2side = P_GetLineSide(WallEnd(portal), line);
		// Do the same handling of points on the portal straight as above.
		if (p1side == 0) p1side = p2side;
		else if (p2side == 0) p2side = p1side;
		// If the portal is on the other side of the line than the viewpoint, there is no possibility to see this line inside the portal.
		return (p1side == p2side && viewside != p1side);
	}
}

int HWLinePortal::ClipSeg(walltype *seg, const DVector3 &viewpos)
{
	return P_ClipLineToPortal(seg, line, viewpos) ? PClip_InFront : PClip_Inside;
}

int HWLinePortal::ClipSector(sectortype *sub)
{
	// this seg is completely behind the mirror
	for (int i = 0; i<sub->wallnum; i++)
	{
		if (PointOnLineSide(WallStart(sub->firstWall()), line) == 0) return PClip_Inside;
	}
	return PClip_InFront;
}

int HWLinePortal::ClipPoint(const DVector2 &pos)
{
	if (PointOnLineSide(pos, line))
	{
		return PClip_InFront;
	}
	return PClip_Inside;
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//
//
// Mirror Portal
//
//
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
//
//
//
//-----------------------------------------------------------------------------

bool HWMirrorPortal::Setup(HWDrawInfo *di, FRenderState &rstate, Clipper *clipper)
{
	auto state = mState;
	if (state->renderdepth > r_mirror_recursions)
	{
		return false;
	}

	auto &vp = di->Viewpoint;

	di->mClipPortal = this;

	double x = line->pos.X;
	double y = line->pos.Y;
	double dx = line->point2Wall()->pos.X - x;
	double dy = line->point2Wall()->pos.Y - y;

	const double j = dx * dx + dy * dy;
	if (j == 0)
		return false;

	DVector2 view = { vp.Pos.X, -vp.Pos.Y };

	double i = ((view.X - x) * dx + (view.Y - y) * dy) * 2;

	double newx = x * 2 + dx * i / j - view.X;
	double newy = y * 2 + dy * i / j - view.Y;

	auto myan = bvectangbam(dx, dy);
	auto newan = myan + myan - bamang(vp.RotAngle);

	vp.RotAngle = newan.asbam();
	vp.SectNums = nullptr;
	vp.SectCount = line->sector;

	vp.Pos.X = newx;
	vp.Pos.Y = -newy;
	vp.HWAngles.Yaw = FAngle::fromDeg( - 90.f + newan.asdeg());

	double FocalTangent = tan(vp.FieldOfView.Radians() / 2);
	DAngle an = DAngle::fromDeg(270. - vp.HWAngles.Yaw.Degrees());
	vp.TanSin = FocalTangent * an.Sin();
	vp.TanCos = FocalTangent * an.Cos();
	vp.ViewVector = an.ToVector();

	state->MirrorFlag++;
	di->SetClipLine(line);
	di->SetupView(rstate, vp.Pos.X, vp.Pos.Y, vp.Pos.Z, !!(state->MirrorFlag & 1), !!(state->PlaneMirrorFlag & 1));
	  

	ClearClipper(di, clipper);

	auto startan = bvectangbam(line->pos.X - newx, line->pos.Y - newy);
	auto endan = bvectangbam(line->point2Wall()->pos.X - newx, line->point2Wall()->pos.Y - newy);
	clipper->RestrictVisibleRange(endan, startan);  // we check the line from the backside so angles are reversed.
	return true;
}

void HWMirrorPortal::Shutdown(HWDrawInfo *di, FRenderState &rstate)
{
	mState->MirrorFlag--;
}

const char *HWMirrorPortal::GetName() { return "Mirror"; }

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//
//
// Line to line Portal
//
//
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
//
// only used by Blood - they can not change the angle.
//
//-----------------------------------------------------------------------------
bool HWLineToLinePortal::Setup(HWDrawInfo *di, FRenderState &rstate, Clipper *clipper)
{
	// TODO: Handle recursion more intelligently
	auto &state = mState;
	if (state->renderdepth>r_mirror_recursions)
	{
		return false;
	}
	auto &vp = di->Viewpoint;
	di->mClipPortal = this;

	auto srccenter = (WallStart(origin) + WallEnd(origin)) / 2;
	auto destcenter = (WallStart(line) + WallEnd(line)) / 2;
	DVector2 npos = vp.Pos - srccenter + destcenter;

#if 0 // Blood does not rotate these. Needs map checking to make sure it can be added.
	int dx = origin->point2Wall()->x - origin->x;
	int dy = origin->point2Wall()->y - origin->y;
	int dx2 = line->point2Wall()->x - line->x;
	int dy2 = line->point2Wall()->y - line->y;

	auto srcang = bvectangbam(dx, dy);
	auto destang = bvectangbam(-dx, -dy);

	vp.RotAngle += (destang - srcang).asbam();
#endif

	// Nothing in the entire setup mandates that both lines have the same length.
	// So we need to calculate the clip range from the origin line, not the destination, because that is what determines the visible part of the scene.
	int origx = vp.Pos.X * 16;
	int origy = vp.Pos.Y * -16;

	vp.SectNums = nullptr;
	vp.SectCount = line->sector;
	vp.Pos.X = npos.X;
	vp.Pos.Y = npos.Y;

	di->SetClipLine(line);
	di->SetupView(rstate, vp.Pos.X, vp.Pos.Y, vp.Pos.Z, !!(state->MirrorFlag & 1), !!(state->PlaneMirrorFlag & 1));

	ClearClipper(di, clipper);

	auto startan = bvectangbam(origin->wall_int_pos().X - origx, origin->wall_int_pos().Y - origy);
	auto endan = bvectangbam(origin->point2Wall()->wall_int_pos().X - origx, origin->point2Wall()->wall_int_pos().Y - origy);
	clipper->RestrictVisibleRange(startan, endan);
	return true;
}

const char *HWLineToLinePortal::GetName() { return "LineToLine"; }


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//
//
// Line to sprite Portal
//
//
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
//
// only used by SW.
//
//-----------------------------------------------------------------------------
bool HWLineToSpritePortal::Setup(HWDrawInfo* di, FRenderState& rstate, Clipper* clipper)
{
	// TODO: Handle recursion more intelligently
	auto& state = mState;
	if (state->renderdepth > r_mirror_recursions)
	{
		return false;
	}
	auto& vp = di->Viewpoint;
	di->mClipPortal = this;

	auto srccenter = (WallStart(origin) + WallEnd(origin)) / 2;
	DVector2 destcenter = camera->render_pos().XY();
	DVector2 npos = vp.Pos - srccenter + destcenter;

	double origx = vp.Pos.X;
	double origy = vp.Pos.Y;

	vp.SectNums = nullptr;
	vp.SectCount = camera->sectno();
	vp.Pos.X = npos.X;
	vp.Pos.Y = npos.Y;

	di->SetClipLine(line);
	di->SetupView(rstate, vp.Pos.X, vp.Pos.Y, vp.Pos.Z, !!(state->MirrorFlag & 1), !!(state->PlaneMirrorFlag & 1));

	ClearClipper(di, clipper);

	auto startan = bvectangbam(origin->pos.X - origx, origin->pos.Y - origy);
	auto endan = bvectangbam(origin->point2Wall()->pos.X - origx, origin->point2Wall()->pos.Y - origy);
	clipper->RestrictVisibleRange(startan, endan);
	return true;
}

const char* HWLineToSpritePortal::GetName() { return "LineToSprite"; }


#if 0 // currently none of the games has any support for this. Maybe later.
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//
//
// Skybox Portal
//
//
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
//
// GLSkyboxPortal::DrawContents
//
//-----------------------------------------------------------------------------

bool HWSkyboxPortal::Setup(HWDrawInfo *di, FRenderState &rstate, Clipper *clipper)
{
	auto state = mState;
	old_pm = state->PlaneMirrorMode;

	if (mState->skyboxrecursion >= 3)
	{
		return false;
	}
	auto &vp = di->Viewpoint;

	state->skyboxrecursion++;
	state->PlaneMirrorMode = 0;
	state->inskybox = true;

	AActor *origin = portal->mSkybox;
	portal->mFlags |= PORTSF_INSKYBOX;
	vp.extralight = 0;

	oldclamp = rstate.SetDepthClamp(false);
	vp.Pos = origin->InterpolatedPosition(vp.TicFrac);
	vp.ActorPos = origin->Pos();
	vp.Angles.Yaw += (origin->PrevAngles.Yaw + deltaangle(origin->PrevAngles.Yaw, origin->Angles.Yaw) * vp.TicFrac);

	// Don't let the viewpoint be too close to a floor or ceiling
	double floorh = origin->Sector->floorplane.ZatPoint(origin->Pos());
	double ceilh = origin->Sector->ceilingplane.ZatPoint(origin->Pos());
	if (vp.Pos.Z < floorh + 4) vp.Pos.Z = floorh + 4;
	if (vp.Pos.Z > ceilh - 4) vp.Pos.Z = ceilh - 4;

	vp.ViewActor = origin;

	di->SetupView(rstate, vp.Pos.X, vp.Pos.Y, vp.Pos.Z, !!(state->MirrorFlag & 1), !!(state->PlaneMirrorFlag & 1));
	di->SetViewArea();
	ClearClipper(di, clipper);
	di->UpdateCurrentMapSection();
	return true;
}


void HWSkyboxPortal::Shutdown(HWDrawInfo *di, FRenderState &rstate)
{
	rstate.SetDepthClamp(oldclamp);

	auto state = mState;
	portal->mFlags &= ~PORTSF_INSKYBOX;
	state->inskybox = false;
	state->skyboxrecursion--;
	state->PlaneMirrorMode = old_pm;
}

const char *HWSkyboxPortal::GetName() { return "Skybox"; }
bool HWSkyboxPortal::AllowSSAO() { return false; }	// [MK] sector skyboxes don't allow SSAO by default
#endif

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//
//
// Sector stack Portal
//
//
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
//
// GLSectorStackPortal::DrawContents
//
//-----------------------------------------------------------------------------
bool HWSectorStackPortal::Setup(HWDrawInfo *di, FRenderState &rstate, Clipper *clipper)
{
	// TODO: Handle recursion more intelligently
	auto& state = mState;
	if (state->renderdepth > r_mirror_recursions)
	{
		return false;
	}
	auto portal = origin;
	auto &vp = di->Viewpoint;

	vp.Pos += DVector3(portal->dx / 16., portal->dy / -16., portal->dz / -256.);
	vp.SectNums = portal->targets.Data();
	vp.SectCount = portal->targets.Size();
	type = origin->type;
	state->insectorportal = true;

	// avoid recursions!
	screen->instack[type == PORTAL_SECTOR_CEILING ? 1 : 0]++;

	di->SetupView(rstate, vp.Pos.X, vp.Pos.Y, vp.Pos.Z, !!(state->MirrorFlag & 1), !!(state->PlaneMirrorFlag & 1));
	//SetupCoverage(di);
	ClearClipper(di, clipper);

	return true;
}


void HWSectorStackPortal::Shutdown(HWDrawInfo *di, FRenderState &rstate)
{
	screen->instack[type == PORTAL_SECTOR_CEILING ? 1 : 0]--;
	mState->insectorportal = false;
}

const char *HWSectorStackPortal::GetName() { return "Sectorstack"; }

#if 0
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//
//
// Plane Mirror Portal (currently not needed, Witchaven 2 is the only Build game using such a feature)
//
//
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
//
// GLPlaneMirrorPortal::DrawContents
//
//-----------------------------------------------------------------------------

bool HWPlaneMirrorPortal::Setup(HWDrawInfo *di, FRenderState &rstate, Clipper *clipper)
{
	auto state = mState;
	if (state->renderdepth > r_mirror_recursions)
	{
		return false;
	}
	// A plane mirror needs to flip the portal exclusion logic because inside the mirror, up is down and down is up.
	std::swap(screen->instack[plane_floor], screen->instack[plane_ceiling]);

	auto &vp = di->Viewpoint;
	old_pm = state->PlaneMirrorMode;

	// the player is always visible in a mirror.
	vp.showviewer = true;

	double planez = origin->ZatPoint(vp.Pos);
	vp.Pos.Z = 2 * planez - vp.Pos.Z;
	vp.ViewActor = nullptr;
	state->PlaneMirrorMode = origin->fC() < 0 ? -1 : 1;

	state->PlaneMirrorFlag++;
	di->SetClipHeight(planez, state->PlaneMirrorMode < 0 ? -1.f : 1.f);
	di->SetupView(rstate, vp.Pos.X, vp.Pos.Y, vp.Pos.Z, !!(state->MirrorFlag & 1), !!(state->PlaneMirrorFlag & 1));
	ClearClipper(di, clipper);

	di->UpdateCurrentMapSection();
	return true;
}

void HWPlaneMirrorPortal::Shutdown(HWDrawInfo *di, FRenderState &rstate)
{
	auto state = mState;
	state->PlaneMirrorFlag--;
	state->PlaneMirrorMode = old_pm;
	std::swap(screen->instack[plane_floor], screen->instack[plane_ceiling]);
}

const char *HWPlaneMirrorPortal::GetName() { return origin->fC() < 0? "Planemirror ceiling" : "Planemirror floor"; }
#endif

#if 0
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//
//
// Horizon Portal
//
//
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

HWHorizonPortal::HWHorizonPortal(FPortalSceneState *s, HWHorizonInfo * pt, FRenderViewpoint &vp, bool local)
: HWPortal(s, local)
{
	origin = pt;

	// create the vertex data for this horizon portal.
	HWSectorPlane * sp = &origin->plane;
	const float vx = vp.Pos.X;
	const float vy = vp.Pos.Y;
	const float vz = vp.Pos.Z;
	const float z = sp->Texheight;
	const float tz = (z - vz);

	// Draw to some far away boundary
	// This is not drawn as larger strips because it causes visual glitches.
	auto verts = screen->mVertexData->AllocVertices(1024 + 10);
	auto ptr = verts.first;
	for (int xx = -32768; xx < 32768; xx += 4096)
	{
		float x = xx + vx;
		for (int yy = -32768; yy < 32768; yy += 4096)
		{
			float y = yy + vy;
			ptr->Set(x, z, y, x / 64, -y / 64);
			ptr++;
			ptr->Set(x + 4096, z, y, x / 64 + 64, -y / 64);
			ptr++;
			ptr->Set(x, z, y + 4096, x / 64, -y / 64 - 64);
			ptr++;
			ptr->Set(x + 4096, z, y + 4096, x / 64 + 64, -y / 64 - 64);
			ptr++;
		}
	}

	// fill the gap between the polygon and the true horizon
	// Since I can't draw into infinity there can always be a
	// small gap
	ptr->Set(-32768 + vx, z, -32768 + vy, 512.f, 0);
	ptr++;
	ptr->Set(-32768 + vx, vz, -32768 + vy, 512.f, tz);
	ptr++;
	ptr->Set(-32768 + vx, z, 32768 + vy, -512.f, 0);
	ptr++;
	ptr->Set(-32768 + vx, vz, 32768 + vy, -512.f, tz);
	ptr++;
	ptr->Set(32768 + vx, z, 32768 + vy, 512.f, 0);
	ptr++;
	ptr->Set(32768 + vx, vz, 32768 + vy, 512.f, tz);
	ptr++;
	ptr->Set(32768 + vx, z, -32768 + vy, -512.f, 0);
	ptr++;
	ptr->Set(32768 + vx, vz, -32768 + vy, -512.f, tz);
	ptr++;
	ptr->Set(-32768 + vx, z, -32768 + vy, 512.f, 0);
	ptr++;
	ptr->Set(-32768 + vx, vz, -32768 + vy, 512.f, tz);
	ptr++;

	voffset = verts.second;
	vcount = 1024;

}

//-----------------------------------------------------------------------------
//
// HWHorizonPortal::DrawContents
//
//-----------------------------------------------------------------------------
void HWHorizonPortal::DrawContents(HWDrawInfo *di, FRenderState &state)
{
	Clocker c(PortalAll);

	HWSectorPlane * sp = &origin->plane;
	auto &vp = di->Viewpoint;

	auto texture = TexMan.GetGameTexture(sp->texture, true);

	if (!texture || !texture->isValid())
	{
		state.ClearScreen();
		return;
	}
	di->SetCameraPos(vp.Pos);


	if (texture->isFullbright())
	{
		// glowing textures are always drawn full bright without color
		di->SetColor(state, 255, 0, false, origin->colormap, 1.f);
		di->SetFog(state, 255, 0, false, &origin->colormap, false);
	}
	else
	{
		int rel = getExtraLight();
		di->SetColor(state, origin->lightlevel, rel, di->isFullbrightScene(), origin->colormap, 1.0f);
		di->SetFog(state, origin->lightlevel, rel, di->isFullbrightScene(), &origin->colormap, false);
	}


	state.SetMaterial(texture, UF_Texture, 0, CLAMP_NONE, 0, -1);
	state.SetObjectColor(origin->specialcolor);

	SetPlaneTextureRotation(state, sp, texture);
	state.AlphaFunc(Alpha_GEqual, 0.f);
	state.SetRenderStyle(STYLE_Source);

	for (unsigned i = 0; i < vcount; i += 4)
	{
		state.Draw(DT_TriangleStrip, voffset + i, 4, true);// i == 0);
	}
	state.Draw(DT_TriangleStrip, voffset + vcount, 10, false);

	state.EnableTextureMatrix(false);
}
#endif
