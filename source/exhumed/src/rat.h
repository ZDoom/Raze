
#ifndef __rat_h__
#define __rat_h__

void InitRats();
void SetRatVel(short nSprite);
int BuildRat(short nSprite, int x, int y, int z, short nSector, int nAngle);
int FindFood(short nSprite);
void FuncRat(int a, int b, int nRun);

#endif
