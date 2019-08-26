
#ifndef __switch_h__
#define __switch_h__

#define kMaxLinks		1024
#define kMaxSwitches	1024

void InitLink();
void InitSwitch();

void FuncSwReady(int, int, int);
void FuncSwPause(int, int, int);
void FuncSwStepOn(int, int, int);
void FuncSwNotOnPause(int, int, int);
void FuncSwPressSector(int, int, int);
void FuncSwPressWall(int, int, int);

int BuildSwPause(int nChannel, int nLink, int ebx);
int BuildSwNotOnPause(int nChannel, int nLink, int nSector, int ecx);
int BuildLink(int nCount, int argList ...);
int BuildSwPressSector(int nChannel, int nLink, int nSector, int ecx);
int BuildSwStepOn(int nChannel, int nLink, int nSector);
int BuildSwReady(int nChannel, short nLink);

int BuildSwPressWall(short nChannel, short nLink, short nWall);

#endif
