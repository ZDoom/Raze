
#ifndef __lavadude_h__
#define __lavadude_h__

void InitLava();
int BuildLava(short nSprite, int x, int y, int z, short nSector, short nAngle, int lastArg);
int BuildLavaLimb(int nSprite, int edx, int ebx);
void FuncLavaLimb(int, int, int);
void FuncLava(int, int, int);

#endif
