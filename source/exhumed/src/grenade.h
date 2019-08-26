
#ifndef __grenade_h__
#define __grenade_h__

#define kMaxGrenades	50

void InitGrenades();
int BuildGrenade(int nPlayer);
void DestroyGrenade(short nGrenade);
int ThrowGrenade(short nPlayer, int edx, int ebx, int ecx, int push1);
void FuncGrenade(int, int, int);

#endif
