
#ifndef __init_h__
#define __init_h__

#include "typedefs.h"

#define kMap20	20

enum {
	kSectUnderwater = 0x2000,
	kSectLava = 0x4000,
};

extern int ototalclock;

extern int initx;
extern int inity;
extern int initz;
extern short inita;
extern short initsect;

extern short nCurChunkNum;
extern short nBodyGunSprite[50];
extern int movefifoend;
extern int movefifopos;
extern short nCurBodyGunNum;

void SnapSectors(short nSectorA, short nSectorB, short b);

extern short SectSound[];
extern short SectDamage[];
extern short SectSpeed[];
extern int SectBelow[];
extern short SectFlag[];
extern int SectDepth[];
extern short SectSoundSect[];
extern int SectAbove[];

BOOL LoadLevel(int nMap);
void InstallEngine();
void ResetEngine();
void RemoveEngine();
void LoadObjects();

int myloadconfig();
int mysaveconfig();


#endif
