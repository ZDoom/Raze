//-------------------------------------------------------------------------
/*
Copyright (C) 2020 Christoph Oelckers

This is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.

*/
//------------------------------------------------------------------------- 

#include "build.h"
#include "interpolate.h"
#include "xs_Float.h"
#include "serializer.h"
#include "gamecvars.h"


struct Interpolation
{
	double old, bak;
	int index;
	int type;
	DCoreActor* actor;
};

static TArray<Interpolation> interpolations;

double Get(int index, DCoreActor* actor, int type)
{
	switch(type)
	{
	case Interp_Sect_Floorz:			return sector[index].floorz;
	case Interp_Sect_Ceilingz:			return sector[index].ceilingz;
	case Interp_Sect_Floorheinum:		return sector[index].floorheinum;
	case Interp_Sect_Ceilingheinum:		return sector[index].ceilingheinum;
	case Interp_Sect_FloorPanX:			return sector[index].floorxpan_;
	case Interp_Sect_FloorPanY:			return sector[index].floorypan_;
	case Interp_Sect_CeilingPanX:		return sector[index].ceilingxpan_;
	case Interp_Sect_CeilingPanY:		return sector[index].ceilingypan_;

	case Interp_Wall_X:					return wall[index].pos.X;
	case Interp_Wall_Y:					return wall[index].pos.Y;
	case Interp_Wall_PanX:				return wall[index].xpan_;
	case Interp_Wall_PanY:				return wall[index].ypan_;

	case Interp_Sprite_Z:				return !actor? 0 : actor->spr.pos.Z;
	default: return 0;
	}
}

void Set(int index, DCoreActor* actor, int type, double val)
{
	int old;
	switch(type)
	{
	case Interp_Sect_Floorz:			sector[index].setfloorz(xs_CRoundToInt(val)); break;
	case Interp_Sect_Ceilingz:          sector[index].setceilingz(xs_CRoundToInt(val)); break;
	case Interp_Sect_Floorheinum:       sector[index].floorheinum = (short)xs_CRoundToInt(val); break;
	case Interp_Sect_Ceilingheinum:     sector[index].ceilingheinum = (short)xs_CRoundToInt(val); break;
	case Interp_Sect_FloorPanX:         sector[index].floorxpan_ = float(val); break;
	case Interp_Sect_FloorPanY:	        sector[index].floorypan_ = float(val); break;
	case Interp_Sect_CeilingPanX:       sector[index].ceilingxpan_ = float(val); break;
	case Interp_Sect_CeilingPanY:       sector[index].ceilingypan_ = float(val); break;

	case Interp_Wall_X:                 old = wall[index].pos.X; wall[index].pos.X = val; if (wall[index].pos.X != old) wall[index].moved(); break;
	case Interp_Wall_Y:                 old = wall[index].pos.Y; wall[index].pos.Y = val; if (wall[index].pos.Y != old) wall[index].moved(); break;
	case Interp_Wall_PanX:              wall[index].xpan_ = float(val);  break;
	case Interp_Wall_PanY:              wall[index].ypan_ = float(val);  break;
                                        
	case Interp_Sprite_Z:               if (actor) actor->spr.pos.Z = xs_CRoundToInt(val); break;
	}
}

void StartInterpolation(int index, int type)
{
    for (unsigned i = 0; i < interpolations.Size(); i++)
    {
        if (interpolations[i].index == index && interpolations[i].type == type)
            return;
    }
	int n = interpolations.Reserve(1);

    interpolations[n].index = index;
	interpolations[n].actor = nullptr;
    interpolations[n].type = type;
    interpolations[n].old = Get(index, nullptr, type);
}

void StopInterpolation(int index, int type)
{
    for (unsigned i = 0; i < interpolations.Size(); i++)
    {
        if (interpolations[i].index == index && interpolations[i].type == type)
		{
            interpolations[i] = interpolations.Last();
			interpolations.Pop();
			return;
		}
    }
}

void StartInterpolation(DCoreActor* actor, int type)
{
	assert(type = Interp_Sprite_Z);
	for (unsigned i = 0; i < interpolations.Size(); i++)
	{
		if (interpolations[i].actor == actor && interpolations[i].type == type)
			return;
	}
	int n = interpolations.Reserve(1);

	interpolations[n].index = -1;
	interpolations[n].actor = actor;
	interpolations[n].type = type;
	interpolations[n].old = Get(-1, actor, type);
}

void StopInterpolation(DCoreActor* actor, int type)
{
	assert(type = Interp_Sprite_Z);
	for (unsigned i = 0; i < interpolations.Size(); i++)
	{
		if (interpolations[i].actor == actor && interpolations[i].type == type)
		{
			interpolations[i] = interpolations.Last();
			interpolations.Pop();
			return;
		}
	}
}

void UpdateInterpolations()
{
    for (unsigned i = 0; i < interpolations.Size(); i++)
    {
		interpolations[i].old = Get(interpolations[i].index, interpolations[i].actor, interpolations[i].type);
	}		
}

void DoInterpolations(double smoothratio)
{
	if (!cl_interpolate) return;
    for (unsigned i = 0; i < interpolations.Size(); i++)
    {
		double bak;
		interpolations[i].bak = bak = Get(interpolations[i].index, interpolations[i].actor, interpolations[i].type);
		double old = interpolations[i].old;
		if (interpolations[i].type < Interp_Pan_First || fabs(bak-old) < 128.)
		{
			Set(interpolations[i].index, interpolations[i].actor, interpolations[i].type, old + (bak - old) * smoothratio);
		}
		else
		{
			// with the panning types we need to check for potential wraparound.
			if (bak < old) bak += 256.;
			else old += 256;
			double cur = old + (bak - old) * smoothratio;
			if (cur >= 256.) cur -= 256.;
			Set(interpolations[i].index, interpolations[i].actor, interpolations[i].type, cur);
		}
	}
}

void RestoreInterpolations()
{
	if (!cl_interpolate) return;
	for (unsigned i = 0; i < interpolations.Size(); i++)
    {
		Set(interpolations[i].index, interpolations[i].actor, interpolations[i].type, interpolations[i].bak);
	}		
}

void ClearInterpolations()
{
	interpolations.Clear();
}

void ClearMovementInterpolations()
{
	// This clears all movement interpolations. Needed for Blood which destroys its interpolations each frame.
	for (unsigned i = 0; i < interpolations.Size();)
	{
		switch (interpolations[i].type)
		{
		case Interp_Sect_Floorz:
		case Interp_Sect_Ceilingz:
		case Interp_Sect_Floorheinum:
		case Interp_Sect_Ceilingheinum:
		case Interp_Wall_X:
		case Interp_Wall_Y:
			interpolations[i] = interpolations.Last();
			interpolations.Pop();
			break;
		default:
			i++;
			break;
		}
	}
}

void setsectinterpolate(sectortype* sect)
{
	for (auto&wal : wallsofsector(sect))
	{
		StartInterpolation(&wal, Interp_Wall_X);
		StartInterpolation(&wal, Interp_Wall_Y);
		if (wal.twoSided())
		{
			auto nwal = wal.nextWall();
			StartInterpolation(nwal, Interp_Wall_X);
			StartInterpolation(nwal, Interp_Wall_Y);
			nwal = nwal->point2Wall();
			StartInterpolation(nwal, Interp_Wall_X);
			StartInterpolation(nwal, Interp_Wall_Y);
		}
	}
}

void clearsectinterpolate(sectortype* sect)
{
	for (auto& wal : wallsofsector(sect))
	{
		StopInterpolation(&wal, Interp_Wall_X);
		StopInterpolation(&wal, Interp_Wall_Y);
		if (wal.twoSided())
		{
			StopInterpolation(wal.nextWall(), Interp_Wall_X);
			StopInterpolation(wal.nextWall(), Interp_Wall_Y);
		}
	}
}

FSerializer& Serialize(FSerializer& arc, const char* keyname, Interpolation& w, Interpolation* def)
{
    if (arc.BeginObject(keyname))
    {
        arc ("index", w.index)
			("actor", w.actor)
            ("type", w.type)
            .EndObject();
    }
	if (arc.isReading())
	{
		w.old = Get(w.index, w.actor, w.type);
	}
    return arc;
}

void SerializeInterpolations(FSerializer& arc)
{
	arc("interpolations", interpolations);
}
