//-------------------------------------------------------------------------
/*
Copyright (C) 2010-2019 EDuke32 developers and contributors
Copyright (C) 2019 Nuke.YKT

This file is part of NBlood.

NBlood is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License version 2
as published by the Free Software Foundation.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/
//-------------------------------------------------------------------------
#include "ns.h"	// Must come before everything else!

#include <stdio.h>
#include <stdlib.h>
#include <string.h>


#include "blood.h"
#include "hw_voxels.h"
#include "tilesetbuilder.h"

BEGIN_BLD_NS


int nTileFiles = 0;


#define x(a, b) registerName(#a, b);
static void SetTileNames(TilesetBuildInfo& info)
{
    auto registerName = [&](const char* name, int index)
    {
        info.addName(name, index);
    };
#include "namelist.h"
    // Oh Joy! Plasma Pak changes the tile number of the title screen, but we preferably want mods that use the original one to display it, e.g. Cryptic Passage
    // So let's make this remapping depend on the CRC.
    const int OTITLE = 2046, PTITLE = 2518;
    auto& orgtitle = info.tile[OTITLE];
    auto& pptile = info.tile[PTITLE];

    if (tileGetCRC32(pptile.tileimage) == 1170870757 && (tileGetCRC32(orgtitle.tileimage) != 290208654 || pptile.tileimage->GetWidth() == 0)) registerName("titlescreen", 2046);
    else registerName("titlescreen", 2518);
}
#undef x

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void GameInterface::LoadTextureInfo(TilesetBuildInfo& info)
{
    auto hFile = fileSystem.OpenFileReader("SURFACE.DAT");
    if (hFile.isOpen())
    {
        int count = (int)hFile.GetLength();
        for (int i = 0; i < count; i++)
        {
            info.tile[i].extinfo.surftype = hFile.ReadInt8();
        }
    }

    hFile = fileSystem.OpenFileReader("VOXEL.DAT");
    if (hFile.isOpen())
    {
        int count = (int)hFile.GetLength() / 2;

        for (int i = 0; i < count; i++)
        {
            int voxindex = hFile.ReadInt16();

            // only insert into the table if they are flagged to be processed in viewProcessSprites, i.e. the type value is 6 or 7,
            if (voxindex > -1 && (info.tile[i].extinfo.picanm.extra & 7) >= 6)
            {
                info.tile[i].extinfo.tiletovox = voxindex;
                info.tile[i].extinfo.voxoffs = (uint8_t)info.tile[i].orgimage->GetOffsets().second;
            }
        }
    }

    hFile = fileSystem.OpenFileReader("SHADE.DAT");
    if (hFile.isOpen())
    {
        int count = (int)hFile.GetLength();
        for (int i = 0; i < count; i++)
        {
            info.tile[i].extinfo.tileshade = hFile.ReadInt8();
        }
    }
}

void GameInterface::SetupSpecialTextures(TilesetBuildInfo& info)
{
    SetTileNames(info);
    // set up all special tiles here, before we fully hook up with the texture manager.
    info.Delete(504);
    info.MakeWritable(2342);

}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

int tileGetSurfType(CollisionBase& hit)
{
    switch (hit.type)
    {
    default:
        return 0;
    case kHitSector:
        return GetExtInfo(hit.hitSector->floortexture).surftype;
    case kHitWall:
        return GetExtInfo(hit.hitWall->walltexture).surftype;
    case kHitSprite:
        return GetExtInfo(hit.hitActor->spr.spritetexture()).surftype;
    }
}

END_BLD_NS
