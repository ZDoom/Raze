
#ifndef __scorp_h__
#define __scorp_h__

#include "compat.h"

/* 
    Selkis Boss AI code
*/

#define kMaxScorpions	5

struct Scorpion
{
    short nHealth;
    short nFrame;
    short nAction;
    short nSprite;
    short nTarget;
    short f;
    short g;
    int8_t h;
    int8_t i;
};

void InitScorp();
int BuildScorp(short nSprite, int x, int y, int z, short nSector, short nAngle, int nChannel);
void FuncScorp(int, int, int);

#endif
