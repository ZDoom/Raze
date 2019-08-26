
#ifndef __mummy_h__
#define __mummy_h__

#include "runlist.h"

#define kMaxMummies	150

void InitMummy();
int BuildMummy(int val, int x, int y, int z, int nSector, int angle);

void FuncMummy(int nSector, int edx, int nRun);

#endif
