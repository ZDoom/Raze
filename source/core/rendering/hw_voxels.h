#pragma once

#include <stdint.h>
#include "mdsprite.h"

// We still need the relation to mdmodel_t as long as the model code hasn't been redone.
struct voxmodel_t : public mdmodel_t
{
    FVoxelModel* model = nullptr;
    vec3_t siz;
    FVector3 piv;
    int32_t is8bit;
};



extern int16_t tiletovox[];
extern float voxscale[];
extern voxmodel_t* voxmodels[MAXVOXELS];
extern FixedBitArray<MAXVOXELS> voxrotate;

void voxInit();
void voxClear();
int voxDefine(int voxindex, const char* filename);
