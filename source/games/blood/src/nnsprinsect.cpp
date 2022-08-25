//-------------------------------------------------------------------------
/*
Copyright (C) 2010-2019 EDuke32 developers and contributors
Copyright (C) 2019 Nuke.YKT
Copyright (C) NoOne

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

#include "serializer.h"
#include "maptypes.h"

#ifdef NOONE_EXTENSIONS

BEGIN_BLD_NS

TArray<DBloodActor*> getSpritesNearWalls(sectortype* pSrcSect, int nDist);

// - CLASSES ------------------------------------------------------------------

// SPRITES_NEAR_SECTORS
// Intended for move sprites that is close to the outside walls with
// TranslateSector and/or zTranslateSector similar to Powerslave(Exhumed) way
// --------------------------------------------------------------------------
class SPRINSECT
{
public:
    static const int kMaxSprNear = 256;
    static const int kWallDist = 16;

    struct SPRITES
    {
        unsigned int nSector;
        TArray<TObjPtr<DBloodActor*>> pActors;  // this is a weak reference!
    };
private:
    TArray<SPRITES> db;
public:
    void Free() { db.Clear(); }
    void Init(int nDist = kWallDist); // used in trInit to collect the sprites before translation
    void Serialize(FSerializer& pSave);
    TArray<TObjPtr<DBloodActor*>>* GetSprPtr(int nSector);

};


// SPRITES_NEAR_SECTORS
// Intended for move sprites that is close to the outside walls with
// TranslateSector and/or zTranslateSector similar to Powerslave(Exhumed) way
// --------------------------------------------------------------------------
SPRINSECT gSprNSect;

void SPRINSECT::Init(int nDist)
{
    Free();

    int j;
    for(auto&sect : sector)
    {
        if (!isMovableSector(sect.type))
            continue;

        switch (sect.type) {
        case kSectorZMotionSprite:
        case kSectorSlideMarked:
        case kSectorRotateMarked:
            continue;
            // only allow non-marked sectors
        default:
            break;
        }

        auto collected = getSpritesNearWalls(&sect, nDist);

        // exclude sprites that is not allowed
        for (j = collected.Size() - 1; j >= 0; j--)
        {
            auto pActor = collected[j];
            if ((pActor->spr.cstat & CSTAT_SPRITE_MOVE_MASK) && pActor->insector())
            {
                // if *next* sector is movable, exclude to avoid fighting
                if (!isMovableSector(pActor->sector()->type))
                {
                    switch (pActor->spr.statnum) 
                    {
                    default:
                        continue;
                    case kStatMarker:
                    case kStatPathMarker:
                        if (pActor->spr.flags & 0x1) continue;
                        [[fallthrough]];
                    case kStatDude:
                        break;
                    }
                }
            }

            collected.Delete(j);
        }

        if (collected.Size())
        {
            db.Resize(db.Size() + 1);

            SPRITES& pEntry = db.Last();
            pEntry.pActors.Resize(collected.Size());
            for (unsigned ii = 0; ii < collected.Size(); ii++)
                pEntry.pActors[ii] = collected[ii];
            pEntry.nSector = sectnum(&sect);
        }
    }
}

TArray<TObjPtr<DBloodActor*>>* SPRINSECT::GetSprPtr(int nSector)
{
    for (auto &spre : db)
    {
        if (spre.nSector == (unsigned int)nSector && spre.pActors.Size() > 0)
            return &spre.pActors;
    }
    return nullptr;
}


FSerializer& Serialize(FSerializer& arc, const char* key, SPRINSECT::SPRITES& obj, SPRINSECT::SPRITES* defval)
{
    if (arc.BeginObject(key))
    {
        arc("sector", obj.nSector)
            ("actors", obj.pActors)
            .EndObject();
    }
    return arc;
}

void SPRINSECT::Serialize(FSerializer& arc)
{
    arc("db", db);
}



bool isMovableSector(int nType)
{
    return (nType && nType != kSectorDamage && nType != kSectorTeleport && nType != kSectorCounter);
}

bool isMovableSector(sectortype* pSect)
{
    if (isMovableSector(pSect->type) && pSect->hasX())
    {
        return (pSect->xs().busy && !pSect->xs().unused1);
    }
    return false;
}

TArray<DBloodActor*> getSpritesNearWalls(sectortype* pSrcSect, int nDist)
{
    int i, c = 0, nWall, nSect, swal, ewal;
    int xi, yi, wx, wy, lx, ly, sx, sy, qx, qy, num, den;
    TArray<DBloodActor*> skip;
    TArray<DBloodActor*> out;

    swal = pSrcSect->wallptr;
    ewal = swal + pSrcSect->wallnum - 1;

    for (i = swal; i <= ewal; i++)
    {
        nSect = wall[i].nextsector;
        if (nSect < 0)
            continue;

        nWall = i;
        wx = wall[nWall].wall_int_pos().X;	
        wy = wall[nWall].wall_int_pos().Y;
        lx = wall[wall[nWall].point2].wall_int_pos().X - wx;
        ly = wall[wall[nWall].point2].wall_int_pos().Y - wy;

        BloodSectIterator it(nSect);
        while(auto ac = it.Next())
        {
            if (skip.Find(ac))
                continue;

            sx = ac->int_pos().X;	qx = sx - wx;
            sy = ac->int_pos().Y;	qy = sy - wy;
            num = DMulScale(qx, lx, qy, ly, 4);
            den = DMulScale(lx, lx, ly, ly, 4);

            if (num > 0 && num < den)
            {
                xi = wx + Scale(lx, num, den);
                yi = wy + Scale(ly, num, den);
                if (approxDist(xi - sx, yi - sy) <= nDist)
                {
                    skip.Push(ac);
                    out.Push(ac);
                }
            }
        }
    }
    return out;
}

END_BLD_NS

#endif