// 
//---------------------------------------------------------------------------
//
// Copyright(C) 2021 Christoph Oelckers
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
** hw_voxels.cpp
**
** voxel handling.
**
**/ 

#include "build.h"
#include "voxels.h"
#include "hw_voxels.h"
#include "gamecontrol.h"

int16_t tiletovox[MAXTILES];
static int voxlumps[MAXVOXELS];
float voxscale[MAXVOXELS];
voxmodel_t* voxmodels[MAXVOXELS];
FixedBitArray<MAXVOXELS> voxrotate;


void voxInit()
{
	for (auto& v : tiletovox) v = -1;
	for (auto& v : voxscale) v = 1.f;
	voxrotate.Zero();
}

void voxClear()
{
	for (auto& vox : voxmodels)
	{
		if (vox)
		{
			delete vox->model;
			delete vox;
		}
		vox = nullptr;
	}
}

int voxDefine(int voxindex, const char* filename)
{
	if ((unsigned)voxindex >= MAXVOXELS)
		return -1;

	int i = fileSystem.FindFile(filename);
	voxlumps[voxindex] = i;
	return i < 0 ? -1 : 0;
}

static voxmodel_t* voxload(int lumpnum)
{
	FVoxel* voxel = R_LoadKVX(lumpnum);
	if (voxel != nullptr)
	{
		voxmodel_t* vm = new voxmodel_t;
		*vm = {};
		auto pivot = voxel->Mips[0].Pivot;
		vm->mdnum = 1; //VOXel model id
		vm->scale = vm->bscale = 1.f;
		vm->piv.X = float(pivot.X);
		vm->piv.Y = float(pivot.Y);
		vm->piv.Z = float(pivot.Z);
		vm->siz.X = voxel->Mips[0].SizeX;
		vm->siz.Y = voxel->Mips[0].SizeY;
		vm->siz.Z = voxel->Mips[0].SizeZ;
		vm->is8bit = true;
		voxel->Mips[0].Pivot.Zero();  // Needs to be taken out of the voxel data because it gets baked into the vertex buffer which we cannot use here.
		vm->model = new FVoxelModel(voxel, true);
		return vm;
	}
	return nullptr;
}

void LoadVoxelModels()
{
	for (int i = 0; i < MAXVOXELS; i++)
	{
		int lumpnum = voxlumps[i];
		if (lumpnum > 0)
		{
			voxmodels[i] = voxload(lumpnum);
			if (voxmodels[i])
				voxmodels[i]->scale = voxscale[i];
			else
				Printf("Unable to load voxel from %s\n", fileSystem.GetFileFullPath(lumpnum).GetChars());
		}
		else 
		{
			auto index = fileSystem.FindResource(i, "KVX");
			if (index >= 0)
			{
				voxmodels[i] = voxload(index);
			}
		}
	}
}

