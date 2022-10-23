//-------------------------------------------------------------------------
/*
Copyright (C) 1996, 2003 - 3D Realms Entertainment

This file is part of Duke Nukem 3D version 1.5 - Atomic Edition

Duke Nukem 3D is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

Original Source: 1996 - Todd Replogle
Prepared for public release: 03/21/2003 - Charlie Wiederhold, 3D Realms
Modifications for JonoF's port by Jonathon Fowler (jf@jonof.id.au)
*/
//-------------------------------------------------------------------------

#include "ns.h"
#include "duke3d.h"

#include "v_video.h"
#include "prediction.h"
#include "automap.h"
#include "dukeactor.h"
#include "interpolate.h"
#include "render.h"

// temporary hack to pass along RRRA's global fog. Needs to be done better later.
extern PalEntry GlobalMapFog;
extern float GlobalFogDensity;

BEGIN_DUKE_NS

//---------------------------------------------------------------------------
//
// Floor Over Floor

// If standing in sector with SE42
// then draw viewing to SE41 and raise all =hi SE43 cielings.

// If standing in sector with SE43
// then draw viewing to SE40 and lower all =hi SE42 floors.

// If standing in sector with SE44
// then draw viewing to SE40.

// If standing in sector with SE45
// then draw viewing to SE41.
//
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void GameInterface::UpdateCameras(double smoothratio)
{
	const int VIEWSCREEN_ACTIVE_DISTANCE = 1024;

	if (camsprite == nullptr)
		return;

	auto p = getPlayer(screenpeek);
	if (p->newOwner != nullptr) camsprite->SetOwner(p->newOwner);

	if (camsprite->GetOwner() && (p->GetActor()->spr.pos - camsprite->spr.pos).Length() < VIEWSCREEN_ACTIVE_DISTANCE)
	{
		auto tex = TexMan.FindGameTexture("VIEWSCR", ETextureType::Any);
		if (!tex || !tex->GetTexture()->isCanvas()) return;

		auto canvas = static_cast<FCanvasTexture*>(tex->GetTexture());

		screen->RenderTextureView(canvas, [=](IntRect& rect)
			{
				auto camera = camsprite->GetOwner();
				display_mirror = 2; // should really be 'display external view'.
				auto cstat = camera->spr.cstat;
				camera->spr.cstat = CSTAT_SPRITE_INVISIBLE;
				render_camtex(camera, camera->spr.pos, camera->sector(), DRotator(maphoriz(-camera->spr.shade), camera->interpolatedyaw(smoothratio), nullAngle), tex, rect, smoothratio);
				camera->spr.cstat = cstat;
				display_mirror = 0;
			});
	}
}

void GameInterface::EnterPortal(DCoreActor* viewer, int type)
{
	if (type == PORTAL_WALL_MIRROR) display_mirror++;
}

void GameInterface::LeavePortal(DCoreActor* viewer, int type) 
{
	if (type == PORTAL_WALL_MIRROR) display_mirror--;
}

bool GameInterface::GetGeoEffect(GeoEffect* eff, sectortype* viewsector)
{
	if (isRR() && viewsector->lotag == 848)
	{
		eff->geocnt = geocnt;
		eff->geosector = geosector;
		eff->geosectorwarp = geosectorwarp;
		eff->geosectorwarp2 = geosectorwarp2;
		eff->geox = geox;
		eff->geoy = geoy;
		eff->geox2 = geox2;
		eff->geoy2 = geoy2;
		return true;
	}
	return false;
}

//---------------------------------------------------------------------------
//
// RRRA's drug distortion effect
//
//---------------------------------------------------------------------------
int DrugTimer;

static int getdrugmode(DDukePlayer *p, int oyrepeat)
{
	int now = I_GetBuildTime() >> 1;	// this function works on a 60 fps setup.
	if (playrunning() && p->DrugMode > 0)
	{
		if (now - DrugTimer > 4 || now - DrugTimer < 0) DrugTimer = now - 1;
		while (DrugTimer < now)
		{
			DrugTimer++;
			int var_8c;
			if (p->drug_stat[0] == 0)
			{
				p->drug_stat[1]++;
				var_8c = oyrepeat + p->drug_stat[1] * 5000;
				if (oyrepeat * 3 < var_8c)
				{
					p->drug_aspect = oyrepeat * 3;
					p->drug_stat[0] = 2;
				}
				else
				{
					p->drug_aspect = var_8c;
				}
			}
			else if (p->drug_stat[0] == 3)
			{
				p->drug_stat[1]--;
				var_8c = oyrepeat + p->drug_stat[1] * 5000;
				if (var_8c < oyrepeat)
				{
					p->DrugMode = 0;
					p->drug_stat[0] = 0;
					p->drug_stat[2] = 0;
					p->drug_stat[1] = 0;
					p->drug_aspect = oyrepeat;
				}
				else
				{
					p->drug_aspect = var_8c;
				}
			}
			else if (p->drug_stat[0] == 2)
			{
				if (p->drug_stat[2] > 30)
				{
					p->drug_stat[0] = 1;
				}
				else
				{
					p->drug_stat[2]++;
					p->drug_aspect = oyrepeat * 3 + p->drug_stat[2] * 500;
				}
			}
			else
			{
				if (p->drug_stat[2] < 1)
				{
					p->drug_stat[0] = 2;
					p->DrugMode--;
					if (p->DrugMode == 1)
						p->drug_stat[0] = 3;
				}
				else
				{
					p->drug_stat[2]--;
					p->drug_aspect = oyrepeat * 3 + p->drug_stat[2] * 500;
				}
			}
		}
		return p->drug_aspect;
	}
	else
	{
		DrugTimer = now;
		return oyrepeat;
	}
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void displayrooms(int snum, double interpfrac, bool sceneonly)
{
	DVector3 cpos;
	DRotator cangles;

	DDukePlayer* p = getPlayer(snum);

	// update render angles.
	p->Angles.updateCameraAngles(interpfrac);

	if (automapMode == am_full || !p->insector())
		return;

	// Do not light up the fog in RRRA's E2L1. Ideally this should apply to all foggy levels but all others use lookup table hacks for their fog.
	if (ud.fogactive)
	{
		p->visibility = ud.const_visibility;
	}
	g_visibility = ud.const_visibility;
	g_relvisibility = p->visibility - ud.const_visibility;
	GlobalMapFog = ud.fogactive ? 0x999999 : 0;
	GlobalFogDensity = ud.fogactive ? 350.f : 0.f;

	DoInterpolations(interpfrac);

	setgamepalette(BASEPAL);

	float fov = (float)r_fov;
	auto sect = p->cursector;

	DDukeActor* viewer;
	bool camview = false;

	if (ud.cameraactor)
	{
		viewer = ud.cameraactor;
		camview = true;

		if (viewer->spr.yint < 0) viewer->spr.yint = -100;
		else if (viewer->spr.yint > 199) viewer->spr.yint = 300;

		cpos = viewer->spr.pos.plusZ(-4);
		cangles = DRotator(maphoriz(-viewer->spr.yint), viewer->interpolatedyaw(interpfrac), nullAngle);
		sect = viewer->sector();
	}
	else
	{
		if (isRRRA() && p->DrugMode)
		{
			double fovdelta = atan(getdrugmode(p, 65536) * (1. / 65536.)) * (360. / pi::pi()) - 90.;
			fov = (float)clamp<double>(r_fov + fovdelta * 0.6, r_fov, 150.);
		}

		// The camera texture must be rendered with the base palette, so this is the only place where the current global palette can be set.
		// The setting here will be carried over to the rendering of the weapon sprites, but other 2D content will always default to the main palette.
		setgamepalette(setpal(p));

		// use player's actor initially.
		viewer = p->GetActor();

		if ((snum == myconnectindex) && (numplayers > 1))
		{
			cpos = interpolatedvalue(omypos, mypos, interpfrac);
			cangles = DRotator(interpolatedvalue(omyhoriz + omyhorizoff, myhoriz + myhorizoff, interpfrac), interpolatedvalue(omyang, myang, interpfrac), nullAngle);
		}
		else
		{
			cpos = viewer->getRenderPos(interpfrac);
			cangles = p->Angles.getRenderAngles(interpfrac);
		}

		if (p->newOwner != nullptr)
		{
			viewer = p->newOwner;
			cpos = viewer->spr.pos;
			cangles = DRotator(maphoriz(-viewer->spr.shade), viewer->interpolatedyaw(interpfrac), nullAngle);
			sect = viewer->sector();
			interpfrac = 1.;
			camview = true;
		}
		else if (p->over_shoulder_on == 0)
		{
			if (cl_viewbob) cpos.Z += interpolatedvalue(p->opyoff, p->pyoff, interpfrac);
		}
		else
		{
			auto adjustment = isRR() ? 15 : 12;
			cpos.Z -= adjustment;

			if (!calcChaseCamPos(cpos, viewer, &sect, cangles, interpfrac, 64.))
			{
				cpos.Z += adjustment;
				calcChaseCamPos(cpos, viewer, &sect, cangles, interpfrac, 64.);
			}
		}

		double cz = p->GetActor()->ceilingz;
		double fz = p->GetActor()->floorz;

		if (ud.earthquaketime > 0 && p->on_ground == 1)
		{
			cpos.Z += 1 - (((ud.earthquaketime) & 1) * 2.);
			cangles.Yaw += DAngle::fromBuild((2 - ((ud.earthquaketime) & 2)) << 2);
		}

		if (p->GetActor()->spr.pal == 1) cpos.Z -= 18;

		else if (p->spritebridge == 0 && p->newOwner == nullptr)
		{
			cpos.Z = min(max(cpos.Z, p->truecz + 4), p->truefz - 4);
		}

		if (sect)
		{
			calcSlope(sect, cpos, &cz, &fz);
			cpos.Z = min(max(cpos.Z, cz + 4), fz - 4);
		}
	}

	auto cstat = viewer->spr.cstat;
	if (camview) viewer->spr.cstat = CSTAT_SPRITE_INVISIBLE;
	if (!sceneonly) drawweapon(interpfrac);
	render_drawrooms(viewer, cpos, sect, cangles, interpfrac, fov);
	viewer->spr.cstat = cstat;

	//GLInterface.SetMapFog(false);
	RestoreInterpolations();

	if (!ud.fogactive)
	{
		if (PlayClock < lastvisinc)
		{
			if (abs(p->visibility - ud.const_visibility) > 8)
				p->visibility += (ud.const_visibility - p->visibility) >> 2;
		}
		else p->visibility = ud.const_visibility;
	}
}

bool GameInterface::GenerateSavePic()
{
	displayrooms(myconnectindex, 1., true);
	return true;
}

void GameInterface::processSprites(tspriteArray& tsprites, const DVector3& view, DAngle viewang, double interpfrac)
{
	fi.animatesprites(tsprites, view.XY(), viewang, interpfrac);
}


END_DUKE_NS
