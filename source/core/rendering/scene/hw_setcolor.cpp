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
** hw_renderstate.cpp
** hardware independent part of render state.
**
*/

#include "hw_renderstate.h"
#include "hw_drawstructs.h"
#include "hw_portal.h"
#include "hw_cvars.h"

CUSTOM_CVAR(Int, hw_lightmode, 0, CVAR_ARCHIVE) // this is intentionally per game.
{
	if (self < 0 || self > 2 || (isExhumed() && self != 0)) self = 0;
}

//==========================================================================
//
// set current light color
//
//==========================================================================
void HWDrawInfo::SetColor(FRenderState &state, int sectorlightlevel, int rellight, bool fullbright, const FColormap &cm, float alpha, bool weapon)
{
	if (fullbright)
	{
		state.SetColorAlpha(0xffffff, alpha, 0);
		state.SetNoSoftLightLevel();
	}
	else
	{
		int hwlightlevel = CalcLightLevel(sectorlightlevel, rellight, weapon, cm.BlendFactor);
		PalEntry pe = CalcLightColor(hwlightlevel, cm.LightColor, cm.BlendFactor);
		state.SetColorAlpha(pe, alpha, cm.Desaturation);
		state.SetNoSoftLightLevel();
	}
}

//==========================================================================
//
// Lighting stuff 
//
//==========================================================================

void HWDrawInfo::SetShaderLight(FRenderState &state, float level, float olight)
{
	const float MAXDIST = 256.f;
	const float THRESHOLD = 96.f;
	const float FACTOR = 0.75f;

	if (level > 0)
	{
		float lightdist, lightfactor;

		if (olight < THRESHOLD)
		{
			lightdist = (MAXDIST / 2) + (olight * MAXDIST / THRESHOLD / 2);
			olight = THRESHOLD;
		}
		else lightdist = MAXDIST;

		lightfactor = 1.f + ((olight / level) - 1.f) * FACTOR;
		if (lightfactor == 1.f) lightdist = 0.f;	// save some code in the shader
		state.SetLightParms(lightfactor, lightdist);
	}
	else
	{
		state.SetLightParms(1.f, 0.f);
	}
}


//==========================================================================
//
// Sets the fog for the current polygon
//
//==========================================================================

void HWDrawInfo::SetFog(FRenderState &state, int lightlevel, float visibility, bool fullbright, const FColormap *cmap, bool isadditive)
{
	PalEntry fogcolor;
	float fogdensity;

	if (cmap != nullptr && !fullbright && visibility > 0)
	{
		fogcolor = cmap->FadeColor;
		fogdensity = GetFogDensity(lightlevel, fogcolor, cmap->FogDensity, cmap->BlendFactor) * visibility;
		fogcolor.a = 0;
	}
	else
	{
		fogcolor = 0;
		fogdensity = 0;
	}

	// Make fog a little denser when inside a skybox
	if (portalState.inskybox) fogdensity += fogdensity / 2;


	// no fog in enhanced vision modes!
	if (fogdensity == 0 || gl_fogmode == 0)
	{
		state.EnableFog(false);
		state.SetFog(0, 0);
	}
	else
	{
#if 0
		if (lightmode == ELightMode::Doom)
		{
			float light = (float)CalcLightLevel(lightlevel, rellight, false, cmap->BlendFactor);
			SetShaderLight(state, light, lightlevel);
		}
		else
#endif
		{
			state.SetLightParms(1.f, 0.f);
		}

		// For additive rendering using the regular fog color here would mean applying it twice
		// so always use black
		if (isadditive)
		{
			fogcolor = 0;
		}

		state.EnableFog(true);
		state.SetFog(fogcolor, fogdensity);
	}
}

//==========================================================================
//
// 
//
//==========================================================================

void SetLightAndFog(HWDrawInfo* di, FRenderState& state, PalEntry fade, int palette, int shade, float visibility, float alpha)
{
	bool foggy = (GlobalMapFog || (fade & 0xffffff));
	auto ShadeDiv = lookups.tables[palette].ShadeFactor;
	if (shade == 127) state.SetObjectColor(0xff000000);	// 127 is generally used for shadow objects that must be black, even in foggy areas.

	if (!di->isBuildSoftwareLighting() && !foggy)
	{
		bool fullbright = ShadeDiv < 1 / 1000.f || shade < -numshades;
		float inverselight = shade * 255.f / numshades;
		if (!fullbright) inverselight /= ShadeDiv;
		int lightlevel = !fullbright ? clamp(int(255 - inverselight), 0, 255) : 255;

		FColormap cm = { 0xffffffff };
		di->SetColor(state, lightlevel, di->rellight, fullbright, cm, alpha);
		di->SetFog(state, 255 - inverselight, visibility * di->visibility, fullbright, &cm, false);
		return;
	}
	// Fog must be done before the texture so that the texture selector can override it.

	// Disable brightmaps if non-black fog is used.
	if (ShadeDiv >= 1 / 1000.f && foggy)
	{
		state.EnableFog(1);
		float density = GlobalMapFog ? GlobalFogDensity : 350.f - Scale(numshades - shade, 150, numshades);
		state.SetFog((GlobalMapFog) ? GlobalMapFog : fade, density * hw_density);
		state.SetSoftLightLevel(255);
		state.SetLightParms(128.f, 1 / 1000.f);
	}
	else 
	{
		state.EnableFog(0);
		state.SetFog(0, 0);
		state.SetSoftLightLevel(gl_fogmode != 0 && ShadeDiv >= 1 / 1000.f ? max(0, 255 - Scale(shade, 255, numshades)) : 255);
		state.SetLightParms(visibility, ShadeDiv / (numshades - 2));
	}

	// The shade rgb from the tint is ignored here.
	state.SetColor(globalr * (1 / 255.f), globalg * (1 / 255.f), globalb * (1 / 255.f), alpha);
}

