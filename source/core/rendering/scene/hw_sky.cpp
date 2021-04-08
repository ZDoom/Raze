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

#include "texturemanager.h"
#include "hw_drawinfo.h"
#include "hw_drawstructs.h"
#include "hw_portal.h"
//#include "hw_lighting.h"
#include "hw_material.h"
#include "gamefuncs.h"
#include "build.h"
#include "cmdlib.h"

CVAR(Bool,gl_noskyboxes, false, 0)
FGameTexture* GetSkyTexture(int basetile, int lognumtiles, const int16_t* tilemap, int remap);

//==========================================================================
//
//  Set up the skyinfo struct
//
//==========================================================================

void initSkyInfo(HWDrawInfo *di, HWSkyInfo* sky, sectortype* sector, int plane, PalEntry FadeColor)
{
	int picnum = plane == plane_ceiling ? sector->ceilingpicnum : sector->floorpicnum;

	int32_t dapyscale = 0, dapskybits = 0, dapyoffs = 0, daptileyscale = 0;
	FGameTexture* skytex = nullptr;
	int realskybits = 0;
	// todo: check for skybox replacement.
	if (!skytex)
	{
		int palette = plane == plane_ceiling ? sector->ceilingpal : sector->floorpal;
		int remap = TRANSLATION(Translation_Remap + curbasepal, palette);

		int16_t const* dapskyoff = getpsky(picnum, &dapyscale, &dapskybits, &dapyoffs, &daptileyscale);
		int tw = tileWidth(picnum);
		if ((1 << sizeToBits(tw)) < tw) dapskybits--; // Build math is weird.

		skytex = GetSkyTexture(picnum, dapskybits, dapskyoff, remap);
		realskybits = dapskybits;
		if (skytex) dapskybits = 0;
		else skytex = tileGetTexture(picnum);
	}

	float xpanning = plane == plane_ceiling ? sector->ceilingxpan_ : sector->floorxpan_;
	float ypanning = plane == plane_ceiling ? sector->ceilingypan_ : sector->floorypan_;

	// dapyscale is not relvant for a sky dome.
	sky->y_scale = FixedToFloat(daptileyscale);
	sky->cloudy = !!(sector->exflags & SECTOREX_CLOUDSCROLL);
	if (!sky->cloudy)
	{
		sky->y_offset = dapyoffs * 1.5;
		sky->x_offset = xpanning / (1 << (realskybits - dapskybits));
	}
	else
	{
		sky->y_offset = ypanning;
		sky->x_offset = 2 * xpanning / (1 << (realskybits - dapskybits));
	}
	sky->fadecolor = FadeColor;
	sky->shade = 0;// clamp(plane == plane_ceiling ? sector->ceilingshade : sector->floorshade, 0, numshades - 1);
	sky->texture = skytex;
}


//==========================================================================
//
//  Calculate sky texture for ceiling or floor
//
//==========================================================================

void HWWall::SkyPlane(HWDrawInfo *di, sectortype *sector, int plane, bool allowreflect)
{
	int ptype;

	if ((sector->portalflags == PORTAL_SECTOR_CEILING && plane == plane_ceiling) || (sector->portalflags == PORTAL_SECTOR_FLOOR && plane == plane_floor))
	{
		if (screen->instack[1 - plane] || sector->portalnum >= (int)allPortals.Size()) return;
		portal = &allPortals[sector->portalnum];
		PutPortal(di, PORTALTYPE_SECTORSTACK, plane);
	}
	else if ((sector->portalflags == PORTAL_SECTOR_CEILING_REFLECT && plane == plane_ceiling) || (sector->portalflags == PORTAL_SECTOR_FLOOR_REFLECT && plane == plane_floor))
	{
		ptype = PORTALTYPE_PLANEMIRROR;
		if (plane == plane_ceiling && (sector->ceilingstat & CSTAT_SECTOR_SLOPE)) return;
		if (plane == plane_floor && (sector->floorstat & CSTAT_SECTOR_SLOPE)) return;
		planemirror = plane == plane_floor ? &sector->floorz : &sector->ceilingz;
		PutPortal(di, PORTALTYPE_PLANEMIRROR, plane);
	}
	else
	{
		ptype = PORTALTYPE_SKY;
		HWSkyInfo skyinfo;
		initSkyInfo(di, &skyinfo, sector, plane, fade);
		ptype = PORTALTYPE_SKY;
		sky = &skyinfo;
		PutPortal(di, ptype, plane);
	}
}


//==========================================================================
//
//  Skies on one sided walls
//
//==========================================================================

void HWWall::SkyNormal(HWDrawInfo* di, sectortype* fs, FVector2& v1, FVector2& v2, float fch1, float fch2, float ffh1, float ffh2)
{
	if ((fs->ceilingstat & CSTAT_SECTOR_SKY) || fs->portalflags == PORTAL_SECTOR_CEILING || fs->portalflags == PORTAL_SECTOR_CEILING_REFLECT)
	{
		ztop[0] = ztop[1] = 32768.0f;
		zbottom[0] = fch1;
		zbottom[1] = fch2;
		SkyPlane(di, fs, plane_ceiling, true);
	}

	if ((fs->floorstat & CSTAT_SECTOR_SKY) || fs->portalflags == PORTAL_SECTOR_FLOOR || fs->portalflags == PORTAL_SECTOR_FLOOR_REFLECT)
	{
		ztop[0] = ffh1;
		ztop[1] = ffh2;
		zbottom[0] = zbottom[1] = -32768.0f;
		SkyPlane(di, fs, plane_floor, true);
	}
}

//==========================================================================
//
//  Upper Skies on two sided walls
//
//==========================================================================

void HWWall::SkyTop(HWDrawInfo *di, walltype * seg,sectortype * fs,sectortype * bs, FVector2& v1, FVector2& v2, float fch1, float fch2)
{
	if (fs->portalflags == PORTAL_SECTOR_CEILING || fs->portalflags == PORTAL_SECTOR_CEILING_REFLECT)
	{
		if (fs->portalflags == PORTAL_SECTOR_CEILING_REFLECT)
		{

			/*
				float backreflect = bs->GetReflect(sectortype::ceiling);
				if (backreflect > 0 && bs->ceilingplane.fD() == fs->ceilingplane.fD() && !bs->isClosed())
				{
					// Don't add intra-portal line to the portal.
					return;
				}
			*/
		}
		else
		{
			if (bs->portalflags == PORTAL_SECTOR_CEILING && bs->portalnum == fs->portalnum)
			{
				return;
			}
		}
	}
	else if (fs->ceilingstat & CSTAT_SECTOR_SKY)
	{
		if (bs->ceilingstat & CSTAT_SECTOR_SKY)
		{
			return;
		}

		flags |= HWF_SKYHACK;	// mid textures on such lines need special treatment!
	}
	else return;

	ztop[0] = ztop[1] = 32768.0f;
	zbottom[0] = fch1;
	zbottom[1] = fch2;
	SkyPlane(di, fs, plane_ceiling, true);
}


//==========================================================================
//
//  Lower Skies on two sided walls
//
//==========================================================================

void HWWall::SkyBottom(HWDrawInfo *di, walltype * seg,sectortype * fs,sectortype * bs, FVector2& v1, FVector2& v2, float ffh1, float ffh2)
{
	if (fs->portalflags == PORTAL_SECTOR_FLOOR || fs->portalflags == PORTAL_SECTOR_FLOOR_REFLECT)
	{
		if (fs->portalflags == PORTAL_SECTOR_FLOOR_REFLECT)
		{

			/*
				float backreflect = bs->GetReflect(sectortype::floor);
				if (backreflect > 0 && bs->ceilingplane.fD() == fs->ceilingplane.fD() && !bs->isClosed())
				{
					// Don't add intra-portal line to the portal.
					return;
				}
			*/
		}
		else
		{
			if (bs->portalflags == PORTAL_SECTOR_FLOOR && bs->portalnum == fs->portalnum)
			{
				return;
			}
		}
		// stacked sectors
	}
	else if (fs->floorstat & CSTAT_SECTOR_SKY)
	{

		if (bs->floorstat & CSTAT_SECTOR_SKY)
		{
			return;
		}
		flags |= HWF_SKYHACK;	// mid textures on such lines need special treatment!
	}
	else return;

	zbottom[0] = zbottom[1] = -32768.0f;
	ztop[0] = ffh1;
	ztop[1] = ffh2;
	SkyPlane(di, fs, plane_floor, true);
}

