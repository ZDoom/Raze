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
static float distfogtable[2][256];	// light to fog conversion table for black fog

CVAR(Int, hw_weaponlight, 8, CVAR_ARCHIVE);

//==========================================================================
//
// Sets up the fog tables
//
//==========================================================================

CUSTOM_CVAR(Int, gl_distfog, 70, CVAR_ARCHIVE | CVAR_GLOBALCONFIG)
{
	for (int i = 0; i < 256; i++)
	{

		if (i < 164)
		{
			distfogtable[0][i] = (float)((gl_distfog >> 1) + (gl_distfog)*(164 - i) / 164);
		}
		else if (i < 230)
		{
			distfogtable[0][i] = (float)((gl_distfog >> 1) - (gl_distfog >> 1)*(i - 164) / (230 - 164));
		}
		else distfogtable[0][i] = 0;

		if (i < 128)
		{
			distfogtable[1][i] = 6.f + (float)((gl_distfog >> 1) + (gl_distfog)*(128 - i) / 48);
		}
		else if (i < 216)
		{
			distfogtable[1][i] = (216.f - i) / ((216.f - 128.f)) * gl_distfog / 10;
		}
		else distfogtable[1][i] = 0;
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

	if (lightlevel <= 0) return 0;

	bool darklightmode = isDarkLightMode();

	if (darklightmode && lightlevel < 192 && !weapon) 
	{
		if (lightlevel > 100)
		{
			light = xs_CRoundToInt(192.f - (192 - lightlevel)* 1.87f);
			if (light + rellight < 20)
			{
				light = 20 + (light + rellight - 20) / 5;
			}
			else
			{
				light += rellight;
			}
		}
		else
		{
			light = (lightlevel + rellight) / 5;
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
		density = distfogtable[hw_lightmode != 1][hw_ClampLight(lightlevel)];
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

