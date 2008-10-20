// "Build Engine & Tools" Copyright (c) 1993-1997 Ken Silverman
// Ken Silverman's official web site: "http://www.advsys.net/ken"
// See the included license file "BUILDLIC.TXT" for license info.

#ifndef __editor_h__
#define __editor_h__

#ifdef __cplusplus
extern "C" {
#endif

// Build keys
#define BK_MOVEFORWARD   0
#define BK_MOVEBACKWARD  1
#define BK_TURNLEFT 	 2
#define BK_TURNRIGHT	 3
#define BK_RUN  		 4
#define BK_STRAFE   	 5
#define BK_SHOOT		 6
#define BK_OPEN 		 7
#define BK_MOVEUP   	 8
#define BK_MOVEDOWN 	 9
#define BK_LOOKUP   	10
#define BK_LOOKDOWN 	11
#define BK_STRAFELEFT   12
#define BK_STRAFERIGHT  13
#define BK_MODE2D_3D	14
#define BK_PLAYERVIEW   15
#define BK_ZOOMIN   	16
#define BK_ZOOMOUT  	17
#define BK_MESSAGE  	18
#define BK_CONSOLE  	19
#define NUMBUILDKEYS 20

extern int qsetmode;
extern short searchsector, searchwall, searchstat;
extern int zmode, kensplayerheight;
extern short defaultspritecstat;

extern int temppicnum, tempcstat, templotag, temphitag, tempextra;
extern unsigned int temppal, tempvis, tempxrepeat, tempyrepeat;
extern int tempshade, tempxvel, tempyvel, tempzvel;
extern unsigned char somethingintab;

extern unsigned char buildkeys[NUMBUILDKEYS];

extern int ydim16, xdimgame, ydimgame, bppgame, xdim2d, ydim2d, forcesetup;
extern int unrealedlook, quickmapcycling;
extern int pk_turnaccel,pk_turndecel,pk_uedaccel;
extern int revertCTRL,scrollamount;
extern int autosave;
extern int mlook;
extern short prefixtiles[16];

extern int ExtInit(void);
extern int ExtPreInit(int argc,const char **argv);
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
extern const char *ExtGetSectorType(int lotag);

extern int circlewall;

int loadsetup(const char *fn);	// from config.c
int writesetup(const char *fn);	// from config.c

void editinput(void);
void clearmidstatbar16(void);

int _getnumber256(char *namestart, int num, int maxnumber, char sign, void *(func)(int));
#define getnumber256(namestart, num, maxnumber, sign) _getnumber256(namestart, num, maxnumber, sign, NULL)
int _getnumber16(char *namestart, int num, int maxnumber, char sign, void *(func)(int));
#define getnumber16(namestart, num, maxnumber, sign) _getnumber16(namestart, num, maxnumber, sign, NULL)
void printmessage256(int x, int y, char *name);
void _printmessage16(const char *fmt, ...);

#define printmessage16(fmt, ...) lastpm16time = totalclock, _printmessage16(fmt, ## __VA_ARGS__)

void getpoint(int searchxe, int searchye, int *x, int *y);
int getpointhighlight(int xplc, int yplc, int point);

#ifdef _WIN32
#define DEFAULT_GAME_EXEC "eduke32.exe"
#define DEFAULT_GAME_LOCAL_EXEC "eduke32.exe"
#else
#define DEFAULT_GAME_EXEC "eduke32"
#define DEFAULT_GAME_LOCAL_EXEC "./eduke32"
#endif

#ifdef __cplusplus
}
#endif

#endif
