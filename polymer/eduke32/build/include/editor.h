// "Build Engine & Tools" Copyright (c) 1993-1997 Ken Silverman
// Ken Silverman's official web site: "http://www.advsys.net/ken"
// See the included license file "BUILDLIC.TXT" for license info.

#ifndef __editor_h__
#define __editor_h__

#include <math.h>

#ifdef __cplusplus
extern "C" {
#endif

#define VERSION " 2.0.0devel"

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
extern int16_t searchbottomwall;
extern int32_t zmode, kensplayerheight;
extern int16_t defaultspritecstat;

extern int32_t temppicnum, tempcstat, templotag, temphitag, tempextra;
extern uint32_t temppal, tempvis, tempxrepeat, tempyrepeat;
extern int32_t tempshade, tempxvel, tempyvel, tempzvel;
extern char somethingintab;

extern uint8_t buildkeys[NUMBUILDKEYS];

extern double vid_gamma_3d, vid_contrast_3d, vid_brightness_3d;

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

extern int32_t m32_osd_tryscript;
extern int32_t showheightindicators;
extern int32_t showambiencesounds;

// editor side view
extern int32_t m32_sideview;
extern int32_t m32_sideelev;
extern int16_t m32_sideang;
extern int32_t m32_sidecos, m32_sidesin;
extern int32_t m32_swcnt;
extern int16_t *m32_wallsprite;
extern int8_t sideview_reversehrot;
extern inline int32_t scalescreeny(int32_t sy);
extern void screencoords(int32_t *xres, int32_t *yres, int32_t x, int32_t y, int32_t zoome) ATTRIBUTE((nonnull));
//extern void invscreencoords(int32_t *dx, int32_t *dy, int32_t sx, int32_t sy, int32_t zoome);
extern int32_t getinvdisplacement(int32_t *dx, int32_t *dy, int32_t dz) ATTRIBUTE((nonnull));
extern inline int32_t getscreenvdisp(int32_t bz, int32_t zoome);
extern void setup_sideview_sincos(void);
extern void m32_setkeyfilter(int32_t on);

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

extern int32_t LoadBoard(const char *filename, uint32_t flags);
extern const char *SaveBoard(const char *fn, uint32_t flags);

#define CORRUPT_SECTOR (1<<17)
#define CORRUPT_WALL (1<<18)
#define CORRUPT_SPRITE (1<<19)
#define CORRUPT_MASK (CORRUPT_SECTOR|CORRUPT_WALL|CORRUPT_SPRITE)
#define MAXCORRUPTTHINGS 64
extern int32_t numcorruptthings, corruptthings[MAXCORRUPTTHINGS];
extern int32_t autocorruptcheck;
extern int32_t CheckMapCorruption(int32_t printfromlev);

extern void showsectordata(int16_t sectnum, int16_t small);
extern void showwalldata(int16_t wallnum, int16_t small);
extern void showspritedata(int16_t spritenum, int16_t small);

extern int32_t circlewall;

int32_t loadsetup(const char *fn);	// from config.c
int32_t writesetup(const char *fn);	// from config.c

void editinput(void);
void clearmidstatbar16(void);

int32_t _getnumber256(const char *namestart, int32_t num, int32_t maxnumber, char sign, void *(func)(int32_t));
#define getnumber256(namestart, num, maxnumber, sign) _getnumber256(namestart, num, maxnumber, sign, NULL)
int32_t _getnumber16(const char *namestart, int32_t num, int32_t maxnumber, char sign, void *(func)(int32_t));
#define getnumber16(namestart, num, maxnumber, sign) _getnumber16(namestart, num, maxnumber, sign, NULL)
void printmessage256(int32_t x, int32_t y, const char *name);
void message(const char *fmt, ...) ATTRIBUTE((format(printf,1,2)));

// currently only for 3d mode
const char* getstring_simple(const char *querystr, const char *defaultstr, int32_t maxlen);

// like snprintf, but pads the output buffer with 'fill' at the end
//int32_t snfillprintf(char *outbuf, size_t bufsiz, int32_t fill, const char *fmt, ...);
void _printmessage16(const char *fmt, ...) ATTRIBUTE((format(printf,1,2)));

extern int32_t lastpm16time;
#define printmessage16(fmt, ...) lastpm16time = totalclock, _printmessage16(fmt, ## __VA_ARGS__)

extern char lastpm16buf[156];

void getpoint(int32_t searchxe, int32_t searchye, int32_t *x, int32_t *y);
int32_t getpointhighlight(int32_t xplc, int32_t yplc, int32_t point);
void update_highlight();
void update_highlightsector();

#ifdef _WIN32
#define DEFAULT_GAME_EXEC "eduke32.exe"
#define DEFAULT_GAME_LOCAL_EXEC "eduke32.exe"
#else
#define DEFAULT_GAME_EXEC "eduke32"
#define DEFAULT_GAME_LOCAL_EXEC "./eduke32"
#endif

void test_map(int32_t mode);


#define NEXTWALL(i) (wall[wall[i].nextwall])
#define POINT2(i) (wall[wall[i].point2])
#define SPRITESEC(j) (sector[sprite[j].sectnum])

static inline int32_t wallength(int16_t i)
{
    int64_t dax = POINT2(i).x - wall[i].x;
    int64_t day = POINT2(i).y - wall[i].y;
#if 1 //def POLYMOST
    int64_t hypsq = dax*dax + day*day;
    if (hypsq > (int64_t)INT_MAX)
        return (int32_t)sqrt((double)hypsq);
    else
        return ksqrt((int32_t)hypsq);
#else
    return ksqrt(dax*dax + day*day);
#endif
}

#define CLEARLINES2D(Startline, Numlines, Color) clearbuf((char *)(frameplace + ((Startline)*bytesperline)), (bytesperline*(Numlines))>>2, (Color))

#define SCRIPTHISTSIZ 32  // should be the same as OSD_HISTORYDEPTH for maximum win, should be a power of two
extern const char *scripthist[SCRIPTHISTSIZ];
extern int32_t scripthistend;

//////////////////// Aiming ////////////////////
#define SEARCH_WALL 0
#define SEARCH_CEILING 1
#define SEARCH_FLOOR 2
#define SEARCH_SPRITE 3
#define SEARCH_MASKWALL 4

#define ASSERT_AIMING (searchstat>=0 && searchstat<=4)

#define AIMING_AT_WALL (searchstat==0)
#define AIMING_AT_CEILING (searchstat==1)
#define AIMING_AT_FLOOR (searchstat==2)
#define AIMING_AT_SPRITE (searchstat==3)
#define AIMING_AT_MASKWALL (searchstat==4)

#define AIMING_AT_WALL_OR_MASK (AIMING_AT_WALL || AIMING_AT_MASKWALL)
#define AIMING_AT_CEILING_OR_FLOOR (AIMING_AT_CEILING || AIMING_AT_FLOOR)

#ifdef __cplusplus
}
#endif

// showdebug is now used as a general informational on-screen display
#define M32_SHOWDEBUG

#endif
