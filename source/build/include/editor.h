// "Build Engine & Tools" Copyright (c) 1993-1997 Ken Silverman
// Ken Silverman's official web site: "http://www.advsys.net/ken"
// See the included license file "BUILDLIC.TXT" for license info.

#ifndef editor_h_
#define editor_h_

#include "compat.h"
#include "baselayer.h"

#ifdef __cplusplus
extern "C" {
#endif

extern const char *defaultsetupfilename;
extern char setupfilename[BMAX_PATH];

extern char levelname[BMAX_PATH];

// Build keys
#define BK_MOVEFORWARD   0
#define BK_MOVEBACKWARD  1
#define BK_TURNLEFT 	 2
#define BK_TURNRIGHT	 3
#define BK_RUN  		 4
#define BK_STRAFE   	 5
//#define BK_SHOOT		 6
//#define BK_OPEN 		 7
#define BK_MOVEUP   	 8
#define BK_MOVEDOWN 	 9
//#define BK_LOOKUP   	10
//#define BK_LOOKDOWN 	11
//#define BK_STRAFELEFT   12
//#define BK_STRAFERIGHT  13
#define BK_MODE2D_3D	14
//#define BK_PLAYERVIEW   15
//#define BK_ZOOMIN   	16
//#define BK_ZOOMOUT  	17
//#define BK_MESSAGE  	18
#define BK_CONSOLE  	19
#define NUMBUILDKEYS 20

extern int32_t horiz;
extern vec3_t pos;
extern int16_t ang, cursectnum;
extern int32_t g_doHardcodedMovement;

extern int8_t m32_clipping;  // 0: none, 1: only white walls, 2: like game

extern int16_t editstatus, searchit;
extern int32_t searchx, searchy, osearchx, osearchy;      //search input

extern int16_t searchsector, searchwall, searchstat;  //search output
extern int16_t searchbottomwall, searchisbottom;
extern int32_t zmode, kensplayerheight, zlock;

extern int16_t highlightsector[MAXSECTORS], highlightsectorcnt;
extern int16_t highlight[MAXWALLS+MAXSPRITES];
extern int16_t asksave;

extern int16_t pointhighlight, linehighlight, highlightcnt;

#define DEFAULT_SPRITE_CSTAT 0
//extern int16_t defaultspritecstat;

extern int32_t tempsectornum;
extern int32_t temppicnum, tempcstat, templotag, temphitag, tempextra;
extern uint32_t temppal, tempvis, tempxrepeat, tempyrepeat, tempxpanning, tempypanning;
extern int32_t tempshade, tempxvel, tempyvel, tempzvel;
extern int32_t tempstatnum, tempblend;
extern char somethingintab;

extern char names[MAXTILES][25];
extern uint8_t buildkeys[NUMBUILDKEYS];

extern float vid_gamma_3d, vid_contrast_3d, vid_brightness_3d;
extern double msens;

extern vec3_t startpos;
extern int16_t startang, startsectnum;

extern int32_t lastpm16time, synctics;
extern int32_t halfxdim16, midydim16, zoom;
extern int32_t ydim16, xdimgame, ydimgame, bppgame, xdim2d, ydim2d, forcesetup;
extern int32_t unrealedlook, quickmapcycling;
extern int32_t pk_turnaccel,pk_turndecel,pk_uedaccel;
extern int32_t revertCTRL,scrollamount;
extern int32_t autosave;
extern int32_t mlook;
extern int16_t prefixtiles[10];

extern int32_t showfirstwall;

extern int32_t graphicsmode;

extern int32_t grid, autogrid;
extern int32_t editorgridextent;	// in engine.c

extern char game_executable[BMAX_PATH];
extern const char* DefaultGameExec;
extern const char* DefaultGameLocalExec;

extern char program_origcwd[BMAX_PATH];
extern const char *mapster32_fullpath;
extern char *testplay_addparam;
extern const char *g_namesFileName;

extern int32_t gridlock;

extern int32_t g_maxCacheSize;

extern int32_t g_lazy_tileselector;
extern int32_t m32_osd_tryscript;
extern int32_t showheightindicators;
extern int32_t showambiencesounds;

extern int32_t numgraysects;
extern uint8_t graysectbitmap[MAXSECTORS>>3];
extern uint8_t graywallbitmap[MAXWALLS>>3];
extern int32_t autogray, showinnergray;

extern void drawgradient(void);

#ifdef YAX_ENABLE
int32_t yax_is121(int16_t bunchnum, int16_t getfloor);
#endif

extern void M32_DrawRoomsAndMasks(void);

extern void yax_tweakpicnums(int32_t bunchnum, int32_t cf, int32_t restore);  // editor-only
extern void M32_ResetFakeRORTiles(void);

// set to 1 to enable:
#define M32_UNDO 1
extern int32_t map_revision;
extern int32_t map_undoredo(int32_t dir);
extern void map_undoredo_free(void);
extern void create_map_snapshot(void);

typedef struct mapundo_
{
    int32_t revision;
    int32_t num[3];  // numsectors, numwalls, numsprites

    // These exist temporarily as sector/wall/sprite data, but are compressed
    // most of the time.  +4 bytes refcount at the beginning.
    char *sws[3];  // sector, wall, sprite

    uintptr_t crc[3];

    struct mapundo_ *next;  // 'redo' loads this
    struct mapundo_ *prev;  // 'undo' loads this
} mapundo_t;
extern mapundo_t *mapstate;

extern void FuncMenu(void);
#ifdef LUNATIC
extern void LuaFuncMenu(void);
#endif

// editor side view
extern int32_t m32_sideview;
extern int32_t m32_sideelev;
extern int16_t m32_sideang;
extern int32_t m32_sidecos, m32_sidesin;
extern int32_t m32_swcnt;
extern int32_t m32_wallscreenxy[MAXWALLS][2];
extern int16_t m32_wallsprite[MAXWALLS+MAXSPRITES];
extern int8_t sideview_reversehrot;
extern int32_t scalescreeny(int32_t sy);
extern void screencoords(int32_t *xres, int32_t *yres, int32_t x, int32_t y, int32_t zoome) ATTRIBUTE((nonnull));
//extern void invscreencoords(int32_t *dx, int32_t *dy, int32_t sx, int32_t sy, int32_t zoome);
extern int32_t getinvdisplacement(int32_t *dx, int32_t *dy, int32_t dz) ATTRIBUTE((nonnull));
extern int32_t getscreenvdisp(int32_t bz, int32_t zoome);
extern void setup_sideview_sincos(void);

extern int8_t keeptexturestretch;
extern int16_t pointhighlightdist, linehighlightdist;

extern int32_t wallength(int16_t i);
extern void fixrepeats(int16_t i);
extern uint32_t getlenbyrep(int32_t len, int32_t repeat);
extern void fixxrepeat(int16_t wallnum, uint32_t lenrepquot);
extern void AlignWallPoint2(int32_t w0);
extern int32_t AutoAlignWalls(int32_t w0, uint32_t flags, int32_t nrecurs);

extern void SetFirstWall(int32_t sectnum, int32_t wallnum, int32_t alsoynw);
extern int32_t fixspritesectors(void);
extern void clearkeys(void);

extern const char *ExtGetVer(void);
extern int32_t ExtInit(void);
extern int32_t ExtPreInit(int32_t argc,char const * const * argv);
extern int32_t ExtPostStartupWindow(void);
extern void ExtPostInit(void);
extern void ExtUnInit(void);
extern void ExtPreCheckKeys(void);
extern void ExtAnalyzeSprites(int32_t, int32_t, int32_t, int32_t);
extern void ExtCheckKeys(void);
extern void ExtPreLoadMap(void);
extern void ExtSetupMapFilename(const char *mapname);
extern void ExtLoadMap(const char *mapname);
extern int32_t ExtPreSaveMap(void);
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

extern void B_SetBoardFileName(const char *fn);
extern int32_t LoadBoard(const char *filename, uint32_t flags);
extern const char *SaveBoard(const char *fn, uint32_t flags);
extern const char *GetSaveBoardFilename(const char *fn);

extern int32_t clockdir(int32_t wallstart);
extern int32_t loopinside(int32_t x, int32_t y, int16_t startwall);

enum {
    // NOTE: These must not be changed, see e.g. loopinside().
    CLOCKDIR_CW = 0,  // outer loop
    CLOCKDIR_CCW = 1,  // inner loop
};

// <loopstart> has to be the starting (i.e. least index) wall of a loop!
//
// The returned value will be either
//  - the starting wall of the next loop of this sector,
//  - the first wall of the next sector, or
//  - out of bounds (== MAXWALLS).
// Thus, it MUST be checked for a proper bound!
static inline int32_t get_nextloopstart(int32_t loopstart)
{
    int32_t i = loopstart;

    while (++i)  // sic
        if (wall[i].point2 == loopstart)
            return i + 1;

    return MAXWALLS;
}

#define CORRUPT_SECTOR (1<<17)
#define CORRUPT_WALL (1<<18)
#define CORRUPT_SPRITE (1<<19)
#define CORRUPT_MASK (CORRUPT_SECTOR|CORRUPT_WALL|CORRUPT_SPRITE)
#define MAXCORRUPTTHINGS 64
extern int32_t corruptlevel, numcorruptthings, corruptthings[MAXCORRUPTTHINGS];
extern int32_t autocorruptcheck;
extern int32_t corruptcheck_noalreadyrefd, corruptcheck_heinum;
extern int32_t corruptcheck_game_duke3d;
extern int32_t corrupt_tryfix_alt;
extern int32_t CheckMapCorruption(int32_t printfromlev, uint64_t tryfixing);

extern int32_t fixmaponsave_sprites;
extern int32_t m32_script_expertmode;  // if true, make read-only vars writable

extern void showsectordata(int16_t sectnum, int16_t small);
extern void showwalldata(int16_t wallnum, int16_t small);
extern void showspritedata(int16_t spritenum, int16_t small);

extern void drawsmallabel(const char *text, char col, char backcol, char border, int32_t dax, int32_t day, int32_t daz);

extern int32_t circlewall;
extern int32_t searchlock;

int32_t loadsetup(const char *fn);	// from config.c
int32_t writesetup(const char *fn);	// from config.c

void editinput(void);
void clearmidstatbar16(void);
void fade_editor_screen(int32_t keepcol);

// internal getnumber* helpers:
extern int32_t getnumber_internal1(char ch, int32_t *danumptr, int32_t maxnumber, char sign);
extern int32_t getnumber_autocomplete(const char *namestart, char ch, int32_t *danum, int32_t flags);

// always CRLF for us
#ifdef _WIN32
# define OURNEWL "\n"
#else
# define OURNEWL "\r\n"
#endif

#ifdef YAX_ENABLE
extern const char *yupdownwall[2];
extern const char *YUPDOWNWALL[2];
#endif

int32_t _getnumber256(const char *namestart, int32_t num, int32_t maxnumber, char sign, const char *(func)(int32_t));
#define getnumber256(namestart, num, maxnumber, sign) _getnumber256(namestart, num, maxnumber, sign, NULL)
int32_t _getnumber16(const char *namestart, int32_t num, int32_t maxnumber, char sign, const char *(func)(int32_t));
#define getnumber16(namestart, num, maxnumber, sign) _getnumber16(namestart, num, maxnumber, sign, NULL)
void printmessage256(int32_t x, int32_t y, const char *name);
void message(const char *fmt, ...) ATTRIBUTE((format(printf,1,2)));
void silentmessage(const char *fmt, ...) ATTRIBUTE((format(printf,1,2)));
extern int32_t AskIfSure(const char *text);

extern char getmessage[162], getmessageleng;
extern int32_t getmessagetimeoff;

extern int32_t mouseaction;

const char* getstring_simple(const char *querystr, const char *defaultstr, int32_t maxlen, int32_t completion);

// like snprintf, but pads the output buffer with 'fill' at the end
//int32_t snfillprintf(char *outbuf, size_t bufsiz, int32_t fill, const char *fmt, ...);
void _printmessage16(const char *fmt, ...) ATTRIBUTE((format(printf,1,2)));

#define printmessage16(fmt, ...) lastpm16time = totalclock, _printmessage16(fmt, ## __VA_ARGS__)

extern char lastpm16buf[156];

int32_t fillsector_maybetrans(int16_t sectnum, int32_t fillcolor, uint8_t dotrans);  // fillcolor == -1: default (pulsating)
int32_t ask_if_sure(const char *query, uint32_t flags);
int32_t editor_ask_function(const char *question, const char *dachars, int32_t numchars);

static inline int32_t fillsector(int16_t sectnum, int32_t fillcolor)
{
    return fillsector_maybetrans(sectnum, fillcolor, 1);
}

static inline int32_t fillsector_notrans(int16_t sectnum, int32_t fillcolor)
{
    return fillsector_maybetrans(sectnum, fillcolor, 0);
}

void spriteoncfz(int32_t i, int32_t *czptr, int32_t *fzptr) ATTRIBUTE((nonnull(2,3)));
void DoSpriteOrnament(int32_t i);

void getpoint(int32_t searchxe, int32_t searchye, int32_t *x, int32_t *y);
int32_t getpointhighlight(int32_t xplc, int32_t yplc, int32_t point);
void update_highlight(void);
void update_highlightsector(void);
void reset_highlightsector(void);
void reset_highlight(void);
void handlesecthighlight1(int32_t i, int32_t sub, int32_t nograycheck);
void ovh_whiteoutgrab(int32_t restoreredwalls);

int32_t inside_editor_curpos(int16_t sectnum);
void correct_sprite_yoffset(int32_t i);

void inflineintersect(int32_t x1, int32_t y1, int32_t x2, int32_t y2,
                      int32_t x3, int32_t y3, int32_t x4, int32_t y4,
                      int32_t *intx, int32_t *inty, int32_t *sign12, int32_t *sign34);

void ovhscrcoords(int32_t x, int32_t y, int32_t *scrx, int32_t *scry);

extern uint8_t hlsectorbitmap[MAXSECTORS>>3];

void test_map(int32_t mode);

extern void M32RunScript(const char *s);

////////// tag labeling system //////////
// max (strlen+1), i.e. array length to allocate for a tag label:
#define TAGLAB_MAX 40

extern int32_t taglab_load(const char *filename, int32_t flags);
extern int32_t taglab_save(const char *mapname);

extern void taglab_init();
extern int32_t taglab_add(const char *label, int16_t tag);
extern const char *taglab_getlabel(int16_t tag);
extern int32_t taglab_gettag(const char *label);

extern int32_t taglab_linktags(int32_t spritep, int32_t num);
extern int32_t taglab_getnextfreetag(int32_t *duetoptr);

extern int32_t showtags;

int32_t select_sprite_tag(int32_t spritenum);

extern int32_t m32_2d3dmode, m32_2d3dsize;
extern vec2_t m32_2d3d;

#define XSIZE_2D3D (xdim2d / m32_2d3dsize)
#define YSIZE_2D3D (ydim2d / m32_2d3dsize)

static inline int32_t m32_2d3d_resolutions_match()
{
    return (xdimgame == xdim2d && ydimgame == ydim2d);
}

static inline int32_t m32_is2d3dmode(void)
{
    return !in3dmode() && m32_2d3dmode && (unsigned)cursectnum < MAXSECTORS &&
        m32_2d3d_resolutions_match() &&
        searchx > m32_2d3d.x && searchx < (m32_2d3d.x + XSIZE_2D3D) &&
        searchy > m32_2d3d.y && searchy < (m32_2d3d.y + YSIZE_2D3D);
}

extern int32_t getspritecol(int32_t spr);

#define NEXTWALL(i) (wall[wall[i].nextwall])
#define POINT2(i) (wall[wall[i].point2])
#define SPRITESEC(j) (sector[sprite[j].sectnum])

#define SCRIPTHISTSIZ 32  // should be the same as OSD_HISTORYDEPTH for maximum win, should be a power of two
extern char *scripthist[SCRIPTHISTSIZ];
extern int32_t scripthistend;

//////////////////// Aiming ////////////////////
#define SEARCH_WALL 0
#define SEARCH_CEILING 1
#define SEARCH_FLOOR 2
#define SEARCH_SPRITE 3
#define SEARCH_MASKWALL 4

#define ASSERT_AIMING ((unsigned)searchstat<=4)

#define AIMING_AT_WALL (searchstat==0)
#define AIMING_AT_CEILING (searchstat==1)
#define AIMING_AT_FLOOR (searchstat==2)
#define AIMING_AT_SPRITE (searchstat==3)
#define AIMING_AT_MASKWALL (searchstat==4)

#define AIMING_AT_WALL_OR_MASK (AIMING_AT_WALL || AIMING_AT_MASKWALL)
#define AIMING_AT_CEILING_OR_FLOOR (AIMING_AT_CEILING || AIMING_AT_FLOOR)

// SaveBoard flags.
enum SaveBoardFlags
{
    M32_SB_NOEXT = 1,  // no ExtSaveMap (backup.map) and no taglabels saving
    M32_SB_ASKOV = 2,  // for NEW_MAP_FORMAT build, ask whether write map if going from V7/8 to VX
};

// showdebug is now used as a general informational on-screen display
#define M32_SHOWDEBUG

// this should be only used if there's no dependency on endwall after the loop.
// of course, users of this macro should know that it modifies startwall and endwall.
#define WALLS_OF_SECTOR(Sect, Itervar) \
    startwall=sector[(Sect)].wallptr, endwall=startwall+sector[(Sect)].wallnum, Itervar=startwall; \
    Itervar < endwall; \
    Itervar++

#define SET_PROTECT_BITS(Var, Newval, Bitstoprotect) do {  \
    (Var) &= (Bitstoprotect);  \
    (Var) |= (Newval) & ~(Bitstoprotect); } while (0)

#define BTAG_MAX 65535
#define BZ_MAX 8388608
#define BHEINUM_MAX 32767

#define M32_MAXPALOOKUPS (MAXPALOOKUPS-RESERVEDPALS-1)

static FORCE_INLINE int32_t atoi_safe(const char *str) { return (int32_t)Bstrtol(str, NULL, 10); }

static FORCE_INLINE void inpclamp(int32_t *x, int32_t mi, int32_t ma)
{
    if (*x > ma) *x = ma;
    if (*x < mi) *x = mi;
}

#define drawtranspixel(p, col) drawpixel(p, blendtable[0][(readpixel(p) * 256) + col])

// Timed offset for Mapster32 color index cycling.
// Range: 0 .. 16
#define M32_THROB klabs(sintable[((totalclock << 4) & 2047)] >> 10)

#ifdef __cplusplus
}
#endif

#endif
