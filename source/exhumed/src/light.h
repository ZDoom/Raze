
#ifndef __light_h__
#define __light_h__

#include "compat.h"

void MyLoadPalette();
int LoadPaletteLookups();
void WaitVBL();
void SetGreenPal();
void RestoreGreenPal();
void FixPalette();
void FadeToWhite();
int HavePLURemap();
uint8_t RemapPLU(uint8_t pal);

extern void DoOverscanSet(short someval);
void SetOverscan(int id);

//extern unsigned char kenpal[];
extern short overscanindex;

extern char *origpalookup[];

extern short nPalDiff;

#endif
