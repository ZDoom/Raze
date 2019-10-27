
#ifndef __lighting_h__
#define __lighting_h__

extern short nFlashDepth;

void InitLights();
void AddFlash(short nSector, int x, int y, int z, int val);
void SetTorch(int nPlayer, int bTorchOnOff);
void UndoFlashes();
void DoLights();
void AddFlow(int nSprite, int a, int b);
void BuildFlash(short nPlayer, short nSector, int nVal);
void AddGlow(short nSector, int nVal);
void AddFlicker(short nSector, int nVal);

extern short bTorch;

#endif