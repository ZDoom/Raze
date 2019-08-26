
#ifndef __bullet_h__
#define __bullet_h__

// 32 bytes
struct bulletInfo
{
	short nDamage; // 0
	short field_2; // 2
	int field_4;   // 4
	short field_8; // 8
	short nSeq; // 10
	short field_C; // 12
	short nFlags;
	short field_10; // damage radius?
	short xyRepeat;
	char pad[12];
};

extern bulletInfo BulletInfo[];

extern short nRadialBullet;
extern short lasthitsect;
extern int lasthitz;
extern int lasthitx;
extern int lasthity;

void InitBullets();
short GrabBullet();
void DestroyBullet(short nRun);
int MoveBullet(short nBullet);
void SetBulletEnemy(short nBullet, short nEnemy);
int BuildBullet(short nSprite, int nType, int ebx, int ecx, int val1, int nAngle, int val2, int val3);
void IgniteSprite(int nSprite);
void FuncBullet(int, int, int);
void BackUpBullet(int *x, int *y, short nAngle);

#endif
