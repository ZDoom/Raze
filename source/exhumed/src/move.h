
#ifndef __move_h__
#define __move_h__

// 16 bytes
struct BlockInfo
{
	int x;
	int y;
	int field_8;
	short nSprite;
};
extern BlockInfo sBlockInfo[];

extern long hihit;
extern short nChunkSprite[];

signed int lsqrt(int a1);
void MoveThings();
void ResetMoveFifo();
void InitChunks();
void InitPushBlocks();
void Gravity(short nSprite);
short UpdateEnemy(short *nEnemy);
int MoveCreature(short nSprite);
int MoveCreatureWithCaution(int nSprite);
void WheresMyMouth(int nPlayer, int *x, int *y, int *z, short *sectnum);

int GetSpriteHeight(int nSprite);

int GrabBody();

int GrabBodyGunSprite();
void CreatePushBlock(int nSector);

void FuncCreatureChunk(int a, int, int nRun);

int FindPlayer(int nSprite, int nVal);

int BuildCreatureChunk(int nVal, int nPic);

void BuildNear(int x, int y, int walldist, int nSector);
int BelowNear(short nSprite);

int PlotCourseToSprite(int nSprite1, int nSprite2);

void CheckSectorFloor(short nSector, int z, long *a, long *b);

int GetAngleToSprite(int nSprite1, int nSprite2);

int GetWallNormal(short nWall);

int GetUpAngle(short nSprite1, int nVal, short nSprite2, int ecx);
void MoveSector(short nSector, int nAngle, int *nXVel, int *nYVel);

int AngleChase(int nSprite, int nSprite2, int ebx, int ecx, int push1);

void SetQuake(short nSprite, int nVal);

#endif
