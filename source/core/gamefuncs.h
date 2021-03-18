#pragma once

#include "gamecontrol.h"
#include "buildtypes.h"
#include "binaryangle.h"

extern int cameradist, cameraclock;

bool calcChaseCamPos(int* px, int* py, int* pz, spritetype* pspr, short *psectnum, binangle ang, fixedhoriz horiz, double const smoothratio);
bool spriteIsModelOrVoxel(const spritetype* tspr);
