
#ifndef __runlist_h__
#define __runlist_h__

#include "typedefs.h"

#define kMaxRuns		25600
#define kMaxChannels	4096

struct RunStruct
{
	union
	{
		int nMoves;
		struct
		{
			short nVal;
			short nRef;
		};
	};

	short _4;
	short _6;
};

struct RunChannel
{
	short a;
	short b;
	short c;
	short d;
};

typedef void(*AiFunc)(int, int, int nRun);

extern RunStruct RunData[kMaxRuns];
extern RunChannel sRunChannels[kMaxChannels];
extern short NewRun;
extern int nRadialOwner;
extern short nRadialSpr;

void runlist_InitRun();

int runlist_GrabRun();
int runlist_FreeRun(int nRun);
int runlist_AddRunRec(int a, int b);
int runlist_HeadRun();
void runlist_InitChan();
void runlist_ChangeChannel(int eax, short dx);
void runlist_ReadyChannel(short eax);
void runlist_ProcessSectorTag(int nSector, int lotag, int hitag);
int runlist_AllocChannel(int a);
void runlist_DoSubRunRec(int RunPtr);
void runlist_SubRunRec(int RunPtr);
void runlist_ProcessWallTag(int nWall, short lotag, short hitag);
int runlist_CheckRadialDamage(short nSprite);
void runlist_RadialDamageEnemy(short nSprite, short nDamage, short nRadius);
void runlist_DamageEnemy(short nSprite, short nSprite2, short nDamage);
void runlist_SignalRun(int NxtPtr, int edx);

void runlist_CleanRunRecs();
void runlist_ExecObjects();

#endif
