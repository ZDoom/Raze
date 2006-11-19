// "Build Engine & Tools" Copyright (c) 1993-1997 Ken Silverman
// Ken Silverman's official web site: "http://www.advsys.net/ken"
// See the included license file "BUILDLIC.TXT" for license info.

#ifndef __editor_h__
#define __editor_h__

#ifdef __cplusplus
extern "C" {
#endif

#define NUMBUILDKEYS 20

extern long qsetmode;
extern short searchsector, searchwall, searchstat;
extern long zmode, kensplayerheight;
extern short defaultspritecstat;

extern short temppicnum, tempcstat, templotag, temphitag, tempextra;
extern unsigned char tempshade, temppal, tempxrepeat, tempyrepeat;
extern unsigned char somethingintab;

extern unsigned char buildkeys[NUMBUILDKEYS];

extern long ydim16, xdimgame, ydimgame, bppgame, xdim2d, ydim2d, forcesetup;


extern int ExtInit(void);
extern void ExtUnInit(void);
extern void ExtPreCheckKeys(void);
#ifdef SUPERBUILD
extern void ExtAnalyzeSprites(void);
#endif
extern void ExtCheckKeys(void);
extern void ExtPreLoadMap(void);
extern void ExtLoadMap(const char *mapname);
extern void ExtPreSaveMap(void);
extern void ExtSaveMap(const char *mapname);
extern const char *ExtGetSectorCaption(short sectnum);
extern const char *ExtGetWallCaption(short wallnum);
extern const char *ExtGetSpriteCaption(short spritenum);
extern void ExtShowSectorData(short sectnum);
extern void ExtShowWallData(short wallnum);
extern void ExtShowSpriteData(short spritenum);
extern void ExtEditSectorData(short sectnum);
extern void ExtEditWallData(short wallnum);
extern void ExtEditSpriteData(short spritenum);


int loadsetup(const char *fn);	// from config.c
int writesetup(const char *fn);	// from config.c

void editinput(void);
void clearmidstatbar16(void);

long getnumber256(char namestart[80], long num, long maxnumber, char sign);
long getnumber16(char namestart[80], long num, long maxnumber, char sign);
void printmessage256(char name[82]);
void printmessage16(char name[82]);

void getpoint(long searchxe, long searchye, long *x, long *y);
long getpointhighlight(long xplc, long yplc);

#ifdef __cplusplus
}
#endif

#endif
