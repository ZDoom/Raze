// 
//---------------------------------------------------------------------------
//
// Copyright(C) 2002-2016 Christoph Oelckers
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
FGameTexture* GetSkyTexture(int basetile, int lognumtiles, const int16_t* tilemap);

//==========================================================================
//
//  Set up the skyinfo struct
//
//==========================================================================

void initSkyInfo(HWDrawInfo *di, HWSkyInfo* sky, sectortype* sector, int plane, PalEntry FadeColor)
{
	int picnum = plane == plane_ceiling ? sector->ceilingpicnum : sector->floorpicnum;
	float xpanning = plane == plane_ceiling ? sector->ceilingxpan_ : sector->floorxpan_;
	float ypanning = plane == plane_ceiling ? sector->ceilingypan_ : sector->floorypan_;

	int32_t dapyscale = 0, dapskybits = 0, dapyoffs = 0, daptileyscale = 0;
	FGameTexture* skytex = nullptr;
	int realskybits = 0;
	// todo: check for skybox replacement.
	if (!skytex)
	{
		int16_t const* dapskyoff = getpsky(picnum, &dapyscale, &dapskybits, &dapyoffs, &daptileyscale);
		skytex = GetSkyTexture(picnum, dapskybits, dapskyoff);
		realskybits = dapskybits;
		if (skytex) dapskybits = 0;
		else skytex = tileGetTexture(picnum);
	}

	float t = (float)((1 << (sizeToBits(tileWidth(picnum)))) << dapskybits);
	int ti = (1 << (sizeToBits(tileHeight(picnum)))); if (ti != tileHeight(picnum)) ti += ti;

	// dapyscale is not relvant for a sky dome.
	sky->y_scale = FixedToFloat(daptileyscale);
	sky->y_offset = FixedToFloat(dapyoffs) + ypanning * (float)ti * (1.f / 256.f);
	sky->x_offset = xpanning / (1 << (realskybits - dapskybits));	
	sky->fadecolor = FadeColor;
	sky->shade = plane == plane_ceiling ? sector->ceilingshade : sector->floorshade;
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

	if (sector->portalflags == PORTAL_SECTOR_CEILING || sector->portalflags == PORTAL_SECTOR_FLOOR)
	{
		if (screen->instack[1 - plane]) return;
		portal = &allPortals[sector->portalnum];
		PutPortal(di, PORTALTYPE_SECTORSTACK, plane);
	}
	else if (sector->portalflags == PORTAL_SECTOR_CEILING_REFLECT || sector->portalflags == PORTAL_SECTOR_FLOOR_REFLECT)
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

void HWWall::SkyNormal(HWDrawInfo* di, sectortype* fs, FVector2& v1, FVector2& v2)
{
	ztop[0] = ztop[1] = 32768.0f;
	zbottom[0] = zceil[0];
	zbottom[1] = zceil[1];
	SkyPlane(di, fs, plane_ceiling, true);

	ztop[0] = zfloor[0];
	ztop[1] = zfloor[1];
	zbottom[0] = zbottom[1] = -32768.0f;
	SkyPlane(di, fs, plane_floor, true);
}

//==========================================================================
//
//  Upper Skies on two sided walls
//
//==========================================================================

void HWWall::SkyTop(HWDrawInfo *di, walltype * seg,sectortype * fs,sectortype * bs, FVector2& v1, FVector2& v2)
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
		// stacked sectors
		ztop[0] = ztop[1] = 32768.0f;
		PlanesAtPoint(fs, v1.X*16.f, v1.Y*-16.f, &zbottom[0], nullptr);
		PlanesAtPoint(fs, v2.X * 16.f, v2.Y * -16.f, &zbottom[1], nullptr);
	}
	else if (fs->ceilingstat & CSTAT_SECTOR_SKY)
	{
		float c1, c2, f1, f2;
		PlanesAtPoint(bs, v1.X * 16.f, v1.Y * -16.f, &c1, &f1);
		PlanesAtPoint(bs, v2.X * 16.f, v2.Y * -16.f, &c2, &f2);

		if (bs->ceilingstat & CSTAT_SECTOR_SKY)
		{
			// if the back sector is closed the sky must be drawn!
			if (c1 > f1 || c2 > f2) return;
		}

		ztop[0]=ztop[1]=32768.0f;

		zbottom[0] = c1;
		zbottom[1] = c2;
		flags|=HWF_SKYHACK;	// mid textures on such lines need special treatment!
	}

	SkyPlane(di, fs, plane_ceiling, true);
}


//==========================================================================
//
//  Lower Skies on two sided walls
//
//==========================================================================

void HWWall::SkyBottom(HWDrawInfo *di, walltype * seg,sectortype * fs,sectortype * bs, FVector2& v1, FVector2& v2)
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
		zbottom[0] = zbottom[1] = -32768.0f;
		PlanesAtPoint(fs, v1.X * 16.f, v1.Y * -16.f, nullptr, &ztop[0]);
		PlanesAtPoint(fs, v2.X * 16.f, v2.Y * -16.f, nullptr, &ztop[0]);
	}
	else if (fs->ceilingstat & CSTAT_SECTOR_SKY)
	{
		float c1, c2, f1, f2;
		PlanesAtPoint(bs, v1.X * 16.f, v1.Y * -16.f, &c1, &f1);
		PlanesAtPoint(bs, v2.X * 16.f, v2.Y * -16.f, &c2, &f2);

		if (bs->ceilingstat & CSTAT_SECTOR_SKY)
		{
			// if the back sector is closed the sky must be drawn!
			if (c1 > f1 || c2 > f2) return;
		}

		zbottom[0] = zbottom[1] = -32768.0f;

		zbottom[0] = f1;
		zbottom[1] = f2;
		flags |= HWF_SKYHACK;	// mid textures on such lines need special treatment!
	}

	SkyPlane(di, fs, plane_floor, true);
}

