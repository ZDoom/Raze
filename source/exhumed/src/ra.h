
#ifndef __ra_h__
#define __ra_h__

struct RA
{
    short field_0;
    short field_2;
    short field_4;
    short nSprite;
    short nTarget;
    short field_A;
    short field_C;
    short field_E;
};

extern RA Ra[];

void FreeRa(short nPlayer);
int BuildRa(short nPlayer);
void InitRa();
void MoveRaToEnemy(short nPlayer);
void FuncRa(int, int, int);

#endif
