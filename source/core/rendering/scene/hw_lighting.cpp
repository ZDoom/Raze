// 
//---------------------------------------------------------------------------
//
// Copyright(C) 2002-2018 Christoph Oelckers
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
** gl_light.cpp
** Light level / fog management / dynamic lights
**
**/

#include "c_cvars.h"
#include "hw_drawinfo.h"

// externally settable lighting properties
static float distfogtable[256];	// light to fog conversion table for black fog

CVAR(Int, hw_weaponlight, 8, CVAR_ARCHIVE);

//==========================================================================
//
// Sets up the fog tables
//
//==========================================================================

void BuildFogTable()
{
	const int gl_distfog = 70;
	for (int i = 0; i < 256; i++)
	{
		int l = i >> 1;
		if (i < 64) l = -32 + i;

		if (numshades == 64) distfogtable[i] = (float)((gl_distfog >> 1) + (gl_distfog)*(164 - l) / 164);
		else distfogtable[i] = 5.f + (float)((gl_distfog >> 1) + (float)((gl_distfog)*(128 - (i >> 1)) / 70));
	}
}

//==========================================================================
//
// Get current light level
//
//==========================================================================

int HWDrawInfo::CalcLightLevel(int lightlevel, int rellight, bool weapon, int blendfactor)
{
	int light;

	if (numshades == 64)
	{
		//lightlevel = clamp(lightlevel * 55 / 50, 0, 255);
	}

	if (lightlevel <= 0) return 0;
	rellight = Scale(rellight, lightlevel, 255);

	bool darklightmode = isDarkLightMode();

	if (darklightmode && lightlevel < 192 && !weapon) 
	{
		if (lightlevel > 90)
		{
			light = int(192.f - (192 - lightlevel)* 1.5f) + rellight;
		}
		else
		{
			light = int((lightlevel + rellight) * 0.444);
		}

	}
	else
	{
		light=lightlevel+rellight;
	}

	// Fake contrast should never turn a positive value into 0.
	return clamp(light, 1, 255);
}

//==========================================================================
//
// Get current light color
//
//==========================================================================

PalEntry HWDrawInfo::CalcLightColor(int light, PalEntry pe, int blendfactor)
{
	int r,g,b;

	if (blendfactor == 0)
	{
		r = pe.r * light / 255;
		g = pe.g * light / 255;
		b = pe.b * light / 255;
	}
	else
	{
		// This is what Legacy does with colored light in 3D volumes. No, it doesn't really make sense...
		// It also doesn't translate well to software style lighting.
		int mixlight = light * (255 - blendfactor);

		r = (mixlight + pe.r * blendfactor) / 255;
		g = (mixlight + pe.g * blendfactor) / 255;
		b = (mixlight + pe.b * blendfactor) / 255;
	}
	return PalEntry(255, uint8_t(r), uint8_t(g), uint8_t(b));
}

//==========================================================================
//
// calculates the current fog density
//
//==========================================================================

float HWDrawInfo::GetFogDensity(int lightlevel, PalEntry fogcolor, int sectorfogdensity, int blendfactor)
{
	float density;

	if (sectorfogdensity != 0)
	{
		// case 1: Sector has an explicit fog density set.
		density = (float)sectorfogdensity;
	}
	else if ((fogcolor.d & 0xffffff) == 0)
	{
		// case 2: black fog
		if (lightlevel > 255)
			density = max(0.f, distfogtable[255] * (1.f - (lightlevel - 255.f) / 192.f));
		else
			density = distfogtable[hw_ClampLight(lightlevel)];
	}
#if 0
	else if (Level->outsidefogdensity != 0 && APART(Level->info->outsidefog) != 0xff && (fogcolor.d & 0xffffff) == (Level->info->outsidefog & 0xffffff))
	{
		// case 3. outsidefogdensity has already been set as needed
		density = (float)Level->outsidefogdensity;
	}
	else  if (Level->fogdensity != 0)
	{
		// case 4: level has fog density set
		density = (float)Level->fogdensity;
	}
#endif
	else if (lightlevel < 248)
	{
		// case 5: use light level
		density = (float)clamp<int>(255 - lightlevel, 30, 255);
	}
	else
	{
		density = 0.f;
	}
	return density;
}

void HWDrawInfo::SetVisibility()
{
	rellight = 0;
	if (g_relvisibility)
	{
		rellight = (sizeToBits(g_visibility) - sizeToBits(g_visibility + g_relvisibility)) * (32 * 32) / numshades;
	}
	// g_visibility == 512 is the baseline for 32 shades - this value is size dependent, so with twice the shades the visibility must be twice as high to get the same effect.
	visibility = g_visibility * (32.f / 512.f) / numshades;
}