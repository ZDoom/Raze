
#ifndef __anims_h__
#define __anims_h__

#include "typedefs.h"

struct Anim
{
    short nSeq;
    short field_2;
    short field_4;
    short nSprite;
};

extern Anim AnimList[];
extern uint8_t AnimFlags[];

void InitAnims();
void DestroyAnim(int nAnim);
int BuildAnim(int nSprite, int val, int val2, int x, int y, int z, int nSector, int nRepeat, int nFlag);
short GetAnimSprite(short nAnim);

void FuncAnim(int, int, int);
void BuildExplosion(short nSprite);
int BuildSplash(int nSprite, int nSector);

#endif
