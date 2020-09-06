#pragma once

#include "tarray.h"
#include "build.h"

extern bool automapping;
extern bool gFullMap;
extern FixedBitArray<MAXSECTORS> show2dsector;
extern FixedBitArray<MAXWALLS> show2dwall;
extern FixedBitArray<MAXSPRITES> show2dsprite;


void ClearAutomap();
void MarkSectorSeen(int sect);
void DrawOverheadMap(int x, int y, int ang);

