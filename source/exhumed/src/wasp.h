
#ifndef __wasp_h__
#define __wasp_h__

extern short nWaspCount;

void InitWasps();
int BuildWasp(short nSprite, int x, int y, int z, short nSector, short nAngle);
void FuncWasp(int eax, int edx, int nRun);

#endif
