// "Build Engine & Tools" Copyright (c) 1993-1997 Ken Silverman
// Ken Silverman's official web site: "http://www.advsys.net/ken"
// See the included license file "BUILDLIC.TXT" for license info.

#ifndef __editor_h__
#define __editor_h__

#ifdef __cplusplus
extern "C" {
#endif

#define VERSION " 1.3.0devel"

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

extern int32_t qsetmode;
extern int16_t searchsector, searchwall, searchstat;
extern int32_t zmode, kensplayerheight;
extern int16_t defaultspritecstat;

extern int32_t temppicnum, tempcstat, templotag, temphitag, tempextra;
extern uint32_t temppal, tempvis, tempxrepeat, tempyrepeat;
extern int32_t tempshade, tempxvel, tempyvel, tempzvel;
extern char somethingintab;

extern uint8_t buildkeys[NUMBUILDKEYS];

extern int32_t ydim16, xdimgame, ydimgame, bppgame, xdim2d, ydim2d, forcesetup;
extern int32_t unrealedlook, quickmapcycling;
extern int32_t pk_turnaccel,pk_turndecel,pk_uedaccel;
extern int32_t revertCTRL,scrollamount;
extern int32_t autosave;
extern int32_t mlook;
extern int16_t prefixtiles[16];

extern char program_origcwd[BMAX_PATH];
extern char *mapster32_fullpath;
extern char *testplay_addparam;

extern int32_t ExtInit(void);
extern int32_t ExtPreInit(int32_t argc,const char **argv);
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
extern const char *ExtGetSectorCaption(int16_t sectnum);
extern const char *ExtGetWallCaption(int16_t wallnum);
extern const char *ExtGetSpriteCaption(int16_t spritenum);
extern void ExtShowSectorData(int16_t sectnum);
extern void ExtShowWallData(int16_t wallnum);
extern void ExtShowSpriteData(int16_t spritenum);
extern void ExtEditSectorData(int16_t sectnum);
extern void ExtEditWallData(int16_t wallnum);
extern void ExtEditSpriteData(int16_t spritenum);
extern const char *ExtGetSectorType(int32_t lotag);

extern int32_t circlewall;

int32_t loadsetup(const char *fn);	// from config.c
int32_t writesetup(const char *fn);	// from config.c

void editinput(void);
void clearmidstatbar16(void);

int32_t _getnumber256(char *namestart, int32_t num, int32_t maxnumber, char sign, void *(func)(int32_t));
#define getnumber256(namestart, num, maxnumber, sign) _getnumber256(namestart, num, maxnumber, sign, NULL)
int32_t _getnumber16(char *namestart, int32_t num, int32_t maxnumber, char sign, void *(func)(int32_t));
#define getnumber16(namestart, num, maxnumber, sign) _getnumber16(namestart, num, maxnumber, sign, NULL)
void printmessage256(int32_t x, int32_t y, char *name);
void _printmessage16(const char *fmt, ...);

#define printmessage16(fmt, ...) lastpm16time = totalclock, _printmessage16(fmt, ## __VA_ARGS__)

extern char lastpm16buf[156];

void getpoint(int32_t searchxe, int32_t searchye, int32_t *x, int32_t *y);
int32_t getpointhighlight(int32_t xplc, int32_t yplc, int32_t point);

#ifdef _WIN32
#define DEFAULT_GAME_EXEC "eduke32.exe"
#define DEFAULT_GAME_LOCAL_EXEC "eduke32.exe"
#else
#define DEFAULT_GAME_EXEC "eduke32"
#define DEFAULT_GAME_LOCAL_EXEC "./eduke32"
#endif

void test_map(int32_t mode);

#ifdef __cplusplus
}
#endif

#endif
