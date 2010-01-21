// "Build Engine & Tools" Copyright (c) 1993-1997 Ken Silverman
// Ken Silverman's official web site: "http://www.advsys.net/ken"
// See the included license file "BUILDLIC.TXT" for license info.
//
// This file has been modified from Ken Silverman's original release
// by Jonathon Fowler (jonof@edgenetwk.com)


#ifndef __build_h__
#define __build_h__

#include "compat.h"
#include "pragmas.h"

#ifdef __cplusplus
extern "C" {
#endif

#define MAXSECTORSV8 4096
#define MAXWALLSV8 16384
#define MAXSPRITESV8 16384

#define MAXSECTORSV7 1024
#define MAXWALLSV7 8192
#define MAXSPRITESV7 4096

#define MAXSECTORS MAXSECTORSV8
#define MAXWALLS MAXWALLSV8
#define MAXWALLSB ((MAXWALLS>>2)+(MAXWALLS>>3))
#define MAXSPRITES MAXSPRITESV8

#define MAXTILES 15360
#define MAXVOXELS 4096
#define MAXSTATUS 1024
#define MAXPLAYERS 16
#define MAXXDIM 3072
#define MAXYDIM 2304
#define MAXPALOOKUPS 256
#define MAXPSKYTILES 256
#define MAXSPRITESONSCREEN 4096
#define MAXUNIQHUDID 256 //Extra slots so HUD models can store animation state without messing game sprites

#define RESERVEDPALS 4 // don't forget to increment this when adding reserved pals
#define DETAILPAL   (MAXPALOOKUPS - 1)
#define GLOWPAL     (MAXPALOOKUPS - 2)
#define SPECULARPAL (MAXPALOOKUPS - 3)
#define NORMALPAL   (MAXPALOOKUPS - 4)

#define TSPR_TEMP 99
#define TSPR_MIRROR 100

#define PR_LIGHT_PRIO_MAX       0
#define PR_LIGHT_PRIO_MAX_GAME  1
#define PR_LIGHT_PRIO_HIGH      2
#define PR_LIGHT_PRIO_HIGH_GAME 3
#define PR_LIGHT_PRIO_LOW       4
#define PR_LIGHT_PRIO_LOW_GAME  5

#define CLIPMASK0 (((1L)<<16)+1L)
#define CLIPMASK1 (((256L)<<16)+64L)

    //Make all variables in BUILD.H defined in the ENGINE,
    //and externed in GAME
#ifdef ENGINE
#  define EXTERN
#else
#  define EXTERN extern
#endif

#pragma pack(push,1)

//ceilingstat/floorstat:
//   bit 0: 1 = parallaxing, 0 = not                                 "P"
//   bit 1: 1 = groudraw, 0 = not
//   bit 2: 1 = swap x&y, 0 = not                                    "F"
//   bit 3: 1 = double smooshiness                                   "E"
//   bit 4: 1 = x-flip                                               "F"
//   bit 5: 1 = y-flip                                               "F"
//   bit 6: 1 = Align texture to first wall of sector                "R"
//   bits 7-8:                                                       "T"
//          00 = normal floors
//          01 = masked floors
//          10 = transluscent masked floors
//          11 = reverse transluscent masked floors
//   bits 9-15: reserved

    //40 bytes
typedef struct
{
    int16_t wallptr, wallnum;
    int32_t ceilingz, floorz;
    int16_t ceilingstat, floorstat;
    int16_t ceilingpicnum, ceilingheinum;
    int8_t ceilingshade;
    uint8_t ceilingpal, ceilingxpanning, ceilingypanning;
    int16_t floorpicnum, floorheinum;
    int8_t floorshade;
    uint8_t floorpal, floorxpanning, floorypanning;
    uint8_t visibility, filler;
    int16_t lotag, hitag, extra;
} sectortype;

//cstat:
//   bit 0: 1 = Blocking wall (use with clipmove, getzrange)         "B"
//   bit 1: 1 = bottoms of invisible walls swapped, 0 = not          "2"
//   bit 2: 1 = align picture on bottom (for doors), 0 = top         "O"
//   bit 3: 1 = x-flipped, 0 = normal                                "F"
//   bit 4: 1 = masking wall, 0 = not                                "M"
//   bit 5: 1 = 1-way wall, 0 = not                                  "1"
//   bit 6: 1 = Blocking wall (use with hitscan / cliptype 1)        "H"
//   bit 7: 1 = Transluscence, 0 = not                               "T"
//   bit 8: 1 = y-flipped, 0 = normal                                "F"
//   bit 9: 1 = Transluscence reversing, 0 = normal                  "T"
//   bits 10-15: reserved

    //32 bytes
typedef struct
{
    int32_t x, y;
    int16_t point2, nextwall, nextsector, cstat;
    int16_t picnum, overpicnum;
    int8_t shade;
    uint8_t pal, xrepeat, yrepeat, xpanning, ypanning;
    int16_t lotag, hitag, extra;
} walltype;

//cstat:
//   bit 0: 1 = Blocking sprite (use with clipmove, getzrange)       "B"
//   bit 1: 1 = transluscence, 0 = normal                            "T"
//   bit 2: 1 = x-flipped, 0 = normal                                "F"
//   bit 3: 1 = y-flipped, 0 = normal                                "F"
//   bits 5-4: 00 = FACE sprite (default)                            "R"
//             01 = WALL sprite (like masked walls)
//             10 = FLOOR sprite (parallel to ceilings&floors)
//   bit 6: 1 = 1-sided sprite, 0 = normal                           "1"
//   bit 7: 1 = Real centered centering, 0 = foot center             "C"
//   bit 8: 1 = Blocking sprite (use with hitscan / cliptype 1)      "H"
//   bit 9: 1 = Transluscence reversing, 0 = normal                  "T"
//   bits 10-12: reserved
//   bit 13: 1 = does not cast shadow
//   bit 14: 1 = invisible but casts shadow
//   bit 15: 1 = Invisible sprite, 0 = not invisible

    //44 bytes
typedef struct
{
    int32_t x, y, z;
    int16_t cstat, picnum;
    int8_t shade;
    uint8_t pal, clipdist, filler;
    uint8_t xrepeat, yrepeat;
    int8_t xoffset, yoffset;
    int16_t sectnum, statnum;
    int16_t ang, owner, xvel, yvel, zvel;
    int16_t lotag, hitag, extra;
} spritetype;

typedef struct {
    uint32_t mdanimtims;
    int16_t mdanimcur;
    int16_t angoff;
    int16_t pitch, roll;
    int32_t xoff, yoff, zoff;
    uint8_t flags;
    uint8_t xpanning, ypanning;
    uint8_t filler;
    float alpha;
    spritetype *tspr;
} spriteext_t;

typedef struct {
    float smoothduration;
    int16_t mdcurframe, mdoldframe;
    int16_t mdsmooth;
    uint8_t filler[2];
} spritesmooth_t;

#define SPREXT_NOTMD 1
#define SPREXT_NOMDANIM 2
#define SPREXT_AWAY1 4
#define SPREXT_AWAY2 8
#define SPREXT_TSPRACCESS 16

EXTERN spriteext_t *spriteext;
EXTERN spritesmooth_t *spritesmooth;
EXTERN int32_t guniqhudid;

EXTERN sectortype *sector;
EXTERN walltype *wall;
EXTERN int16_t maskwall[MAXWALLSB], maskwallcnt;
EXTERN int16_t thewall[MAXWALLSB];
EXTERN spritetype *sprite;
EXTERN spritetype *tspriteptr[MAXSPRITESONSCREEN];

EXTERN int32_t spritesortcnt;
EXTERN spritetype *tsprite;

EXTERN int32_t xdim, ydim, numpages;
EXTERN int32_t yxaspect, viewingrange;
EXTERN intptr_t ylookup[MAXYDIM+1];

#define MAXVALIDMODES 256
EXTERN int32_t validmodecnt;
struct validmode_t {
    int32_t xdim,ydim;
    char bpp;
    char fs;    // bit 0 = fullscreen flag
    char filler[2];
    int32_t extra; // internal use
};
EXTERN struct validmode_t validmode[MAXVALIDMODES];

EXTERN int16_t numsectors, numwalls;
EXTERN char display_mirror;
EXTERN /*volatile*/ int32_t totalclock;
EXTERN int32_t numframes, randomseed;
EXTERN int16_t sintable[2048];
EXTERN uint8_t palette[768];
EXTERN int16_t numpalookups;
EXTERN char *palookup[MAXPALOOKUPS];
EXTERN char parallaxtype, showinvisibility;
EXTERN int32_t parallaxyoffs, parallaxyscale;
EXTERN int32_t visibility, parallaxvisibility;

EXTERN int32_t windowx1, windowy1, windowx2, windowy2;
EXTERN int16_t startumost[MAXXDIM], startdmost[MAXXDIM];

EXTERN int16_t pskyoff[MAXPSKYTILES], pskybits;

EXTERN int16_t headspritesect[MAXSECTORS+1], headspritestat[MAXSTATUS+1];
EXTERN int16_t prevspritesect[MAXSPRITES], prevspritestat[MAXSPRITES];
EXTERN int16_t nextspritesect[MAXSPRITES], nextspritestat[MAXSPRITES];

EXTERN int16_t tilesizx[MAXTILES], tilesizy[MAXTILES];
EXTERN char picsiz[MAXTILES];
EXTERN char walock[MAXTILES];
EXTERN int32_t pow2long[32];
EXTERN int32_t numtiles, picanm[MAXTILES];
EXTERN intptr_t waloff[MAXTILES];  // stores pointers to cache  -- SA

EXTERN int32_t windowpos, windowx, windowy;

    //These variables are for auto-mapping with the draw2dscreen function.
    //When you load a new board, these bits are all set to 0 - since
    //you haven't mapped out anything yet.  Note that these arrays are
    //bit-mapped.
    //If you want draw2dscreen() to show sprite #54 then you say:
    //   spritenum = 54;
    //   show2dsprite[spritenum>>3] |= (1<<(spritenum&7));
    //And if you want draw2dscreen() to not show sprite #54 then you say:
    //   spritenum = 54;
    //   show2dsprite[spritenum>>3] &= ~(1<<(spritenum&7));
    //Automapping defaults to 0 (do nothing).  If you set automapping to 1,
    //   then in 3D mode, the walls and sprites that you see will show up the
    //   next time you flip to 2D mode.

EXTERN char show2dsector[(MAXSECTORS+7)>>3];
EXTERN char show2dwall[(MAXWALLS+7)>>3];
EXTERN char show2dsprite[(MAXSPRITES+7)>>3];
EXTERN char automapping;

EXTERN char gotpic[(MAXTILES+7)>>3];
EXTERN char gotsector[(MAXSECTORS+7)>>3];

EXTERN char captureformat;
EXTERN char editorcolors[256];

EXTERN int32_t faketilesiz[MAXTILES];
EXTERN char *faketiledata[MAXTILES];

EXTERN char spritecol2d[MAXTILES][2];
extern char vgapal16[4*256];

extern char vgapalette[5*256];
extern uint32_t drawlinepat;

extern void faketimerhandler(void);

extern char apptitle[256];
typedef struct {
    char r,g,b,f;
} palette_t;
extern palette_t curpalette[256], curpalettefaded[256], palfadergb;
extern char palfadedelta;

extern int32_t dommxoverlay, novoxmips;

#ifdef SUPERBUILD
extern int32_t tiletovox[MAXTILES];
extern int32_t usevoxels, voxscale[MAXVOXELS];
#endif
#ifdef POLYMOST
extern int32_t usemodels, usehightile;
extern int32_t rendmode;
#endif
EXTERN int32_t h_xsize[MAXTILES], h_ysize[MAXTILES];
EXTERN int8_t h_xoffs[MAXTILES], h_yoffs[MAXTILES];

extern char *engineerrstr;
extern char noclip;

EXTERN int32_t editorzrange[2];

EXTERN int32_t myconnectindex, numplayers;
EXTERN int32_t connecthead, connectpoint2[MAXPLAYERS];

static inline int32_t getrendermode(void)
{
#ifndef POLYMOST
    return 0;
#else
    return rendmode;
#endif
}

/*************************************************************************
POSITION VARIABLES:

        POSX is your x - position ranging from 0 to 65535
        POSY is your y - position ranging from 0 to 65535
            (the length of a side of the grid in EDITBORD would be 1024)
        POSZ is your z - position (height) ranging from 0 to 65535, 0 highest.
        ANG is your angle ranging from 0 to 2047.  Instead of 360 degrees, or
             2 * PI radians, I use 2048 different angles, so 90 degrees would
             be 512 in my system.

SPRITE VARIABLES:

    EXTERN short headspritesect[MAXSECTORS+1], headspritestat[MAXSTATUS+1];
    EXTERN short prevspritesect[MAXSPRITES], prevspritestat[MAXSPRITES];
    EXTERN short nextspritesect[MAXSPRITES], nextspritestat[MAXSPRITES];

    Example: if the linked lists look like the following:
         ????????????????
               Sector lists:               Status lists:               
         ????????????????J
           Sector0:  4, 5, 8             Status0:  2, 0, 8             
           Sector1:  16, 2, 0, 7         Status1:  4, 5, 16, 7, 3, 9   
           Sector2:  3, 9                                              
         ????????????????
    Notice that each number listed above is shown exactly once on both the
        left and right side.  This is because any sprite that exists must
        be in some sector, and must have some kind of status that you define.


Coding example #1:
    To go through all the sprites in sector 1, the code can look like this:

        sectnum = 1;
        i = headspritesect[sectnum];
        while (i != -1)
        {
            nexti = nextspritesect[i];

            //your code goes here
            //ex: printf("Sprite %d is in sector %d\n",i,sectnum);

            i = nexti;
        }

Coding example #2:
    To go through all sprites with status = 1, the code can look like this:

        statnum = 1;        //status 1
        i = headspritestat[statnum];
        while (i != -1)
        {
            nexti = nextspritestat[i];

            //your code goes here
            //ex: printf("Sprite %d has a status of 1 (active)\n",i,statnum);

            i = nexti;
        }

             insertsprite(short sectnum, short statnum);
             deletesprite(short spritenum);
             changespritesect(short spritenum, short newsectnum);
             changespritestat(short spritenum, short newstatnum);

TILE VARIABLES:
        NUMTILES - the number of tiles found TILES.DAT.
        TILESIZX[MAXTILES] - simply the x-dimension of the tile number.
        TILESIZY[MAXTILES] - simply the y-dimension of the tile number.
        WALOFF[MAXTILES] - the actual 32-bit offset pointing to the top-left
                                 corner of the tile.
        PICANM[MAXTILES] - flags for animating the tile.

TIMING VARIABLES:
        TOTALCLOCK - When the engine is initialized, TOTALCLOCK is set to zero.
            From then on, it is incremented 120 times a second by 1.  That
            means that the number of seconds elapsed is totalclock / 120.
        NUMFRAMES - The number of times the draw3dscreen function was called
            since the engine was initialized.  This helps to determine frame
            rate.  (Frame rate = numframes * 120 / totalclock.)

OTHER VARIABLES:

        STARTUMOST[320] is an array of the highest y-coordinates on each column
                that my engine is allowed to write to.  You need to set it only
                once.
        STARTDMOST[320] is an array of the lowest y-coordinates on each column
                that my engine is allowed to write to.  You need to set it only
                once.
        SINTABLE[2048] is a sin table with 2048 angles rather than the
            normal 360 angles for higher precision.  Also since SINTABLE is in
            all integers, the range is multiplied by 16383, so instead of the
            normal -1<sin(x)<1, the range of sintable is -16383<sintable[]<16383
            If you use this sintable, you can possibly speed up your code as
            well as save space in memory.  If you plan to use sintable, 2
            identities you may want to keep in mind are:
                sintable[ang&2047]       = sin(ang * (3.141592/1024)) * 16383
                sintable[(ang+512)&2047] = cos(ang * (3.141592/1024)) * 16383
        NUMSECTORS - the total number of existing sectors.  Modified every time
            you call the loadboard function.
***************************************************************************/

typedef struct {
    int32_t x, y, z;
} vec3_t;

typedef struct {
    vec3_t pos;
    int16_t hitsprite, hitwall, hitsect;
} hitdata_t;

#pragma pack(pop)

int32_t    preinitengine(void);	// a partial setup of the engine used for launch windows
int32_t    initengine(void);
void   uninitengine(void);
void   initspritelists(void);
int32_t   loadboard(char *filename, char fromwhere, int32_t *daposx, int32_t *daposy, int32_t *daposz, int16_t *daang, int16_t *dacursectnum);
int32_t   loadmaphack(char *filename);
int32_t   saveboard(char *filename, int32_t *daposx, int32_t *daposy, int32_t *daposz, int16_t *daang, int16_t *dacursectnum);
int32_t   loadpics(char *filename, int32_t askedsize);
void   loadtile(int16_t tilenume);
int32_t   qloadkvx(int32_t voxindex, char *filename);
int32_t   allocatepermanenttile(int16_t tilenume, int32_t xsiz, int32_t ysiz);
void   copytilepiece(int32_t tilenume1, int32_t sx1, int32_t sy1, int32_t xsiz, int32_t ysiz, int32_t tilenume2, int32_t sx2, int32_t sy2);
void   makepalookup(int32_t palnum, char *remapbuf, int8_t r, int8_t g, int8_t b, char dastat);
void   setvgapalette(void);
void   setbrightness(char dabrightness, uint8_t *dapal, char noapply);
void   setpalettefade(char r, char g, char b, char offset);
void   squarerotatetile(int16_t tilenume);

int32_t   setgamemode(char davidoption, int32_t daxdim, int32_t daydim, int32_t dabpp);
void   nextpage(void);
void   setview(int32_t x1, int32_t y1, int32_t x2, int32_t y2);
void   setaspect(int32_t daxrange, int32_t daaspect);
void   flushperms(void);

void   plotpixel(int32_t x, int32_t y, char col);
char   getpixel(int32_t x, int32_t y);
void   setviewtotile(int16_t tilenume, int32_t xsiz, int32_t ysiz);
void   setviewback(void);
void   preparemirror(int32_t dax, int32_t day, int32_t daz, int16_t daang, int32_t dahoriz, int16_t dawall, int16_t dasector, int32_t *tposx, int32_t *tposy, int16_t *tang);
void   completemirror(void);

void   drawrooms(int32_t daposx, int32_t daposy, int32_t daposz, int16_t daang, int32_t dahoriz, int16_t dacursectnum);
void   drawmasks(void);
void   clearview(int32_t dacol);
void   clearallviews(int32_t dacol);
void   drawmapview(int32_t dax, int32_t day, int32_t zoome, int16_t ang);
void   rotatesprite(int32_t sx, int32_t sy, int32_t z, int16_t a, int16_t picnum, int8_t dashade, char dapalnum, char dastat, int32_t cx1, int32_t cy1, int32_t cx2, int32_t cy2);
void   drawline256(int32_t x1, int32_t y1, int32_t x2, int32_t y2, char col);
int32_t    printext16(int32_t xpos, int32_t ypos, int16_t col, int16_t backcol, char *name, char fontsize);
void   printext256(int32_t xpos, int32_t ypos, int16_t col, int16_t backcol, char *name, char fontsize);

int32_t   clipmove(vec3_t *vect, int16_t *sectnum, int32_t xvect, int32_t yvect, int32_t walldist, int32_t ceildist, int32_t flordist, uint32_t cliptype);
int32_t   clipinsidebox(int32_t x, int32_t y, int16_t wallnum, int32_t walldist);
int32_t   clipinsideboxline(int32_t x, int32_t y, int32_t x1, int32_t y1, int32_t x2, int32_t y2, int32_t walldist);
int32_t   pushmove(vec3_t *vect, int16_t *sectnum, int32_t walldist, int32_t ceildist, int32_t flordist, uint32_t cliptype);
void   getzrange(const vec3_t *vect, int16_t sectnum, int32_t *ceilz, int32_t *ceilhit, int32_t *florz, int32_t *florhit, int32_t walldist, uint32_t cliptype);
int32_t   hitscan(const vec3_t *sv, int16_t sectnum, int32_t vx, int32_t vy, int32_t vz, hitdata_t *hitinfo, uint32_t cliptype);
int32_t   neartag(int32_t xs, int32_t ys, int32_t zs, int16_t sectnum, int16_t ange, int16_t *neartagsector, int16_t *neartagwall, int16_t *neartagsprite, int32_t *neartaghitdist, int32_t neartagrange, char tagsearch);
int32_t   cansee(int32_t x1, int32_t y1, int32_t z1, int16_t sect1, int32_t x2, int32_t y2, int32_t z2, int16_t sect2);
void   updatesector(int32_t x, int32_t y, int16_t *sectnum);
void   updatesectorz(int32_t x, int32_t y, int32_t z, int16_t *sectnum);
int32_t   inside(int32_t x, int32_t y, int16_t sectnum);
void   dragpoint(int16_t pointhighlight, int32_t dax, int32_t day);
void   setfirstwall(int16_t sectnum, int16_t newfirstwall);

void   getmousevalues(int32_t *mousx, int32_t *mousy, int32_t *bstatus);
int32_t    krand(void);
int32_t   ksqrt(int32_t num);
// int32_t   getangle(int32_t xvect, int32_t yvect);

//
// getangle
//

EXTERN int16_t radarang[1280];

static inline int32_t getangle(int32_t xvect, int32_t yvect)
{
    if ((xvect|yvect) == 0) return(0);
    if (xvect == 0) return(512+((yvect<0)<<10));
    if (yvect == 0) return(((xvect<0)<<10));
    if (xvect == yvect) return(256+((xvect<0)<<10));
    if (xvect == -yvect) return(768+((xvect>0)<<10));
    if (klabs(xvect) > klabs(yvect))
        return(((radarang[640+scale(160,yvect,xvect)]>>6)+((xvect<0)<<10))&2047);
    return(((radarang[640-scale(160,xvect,yvect)]>>6)+512+((yvect<0)<<10))&2047);
}

void   rotatepoint(int32_t xpivot, int32_t ypivot, int32_t x, int32_t y, int16_t daang, int32_t *x2, int32_t *y2);
int32_t   lastwall(int16_t point);
int32_t   nextsectorneighborz(int16_t sectnum, int32_t thez, int16_t topbottom, int16_t direction);
int32_t   getceilzofslope(int16_t sectnum, int32_t dax, int32_t day);
int32_t   getflorzofslope(int16_t sectnum, int32_t dax, int32_t day);
void   getzsofslope(int16_t sectnum, int32_t dax, int32_t day, int32_t *ceilz, int32_t *florz);
void   alignceilslope(int16_t dasect, int32_t x, int32_t y, int32_t z);
void   alignflorslope(int16_t dasect, int32_t x, int32_t y, int32_t z);
int32_t   sectorofwall(int16_t theline);
int32_t   loopnumofsector(int16_t sectnum, int16_t wallnum);

// int32_t   insertsprite(int16_t sectnum, int16_t statnum);
// int32_t   deletesprite(int16_t spritenum);

//
// insertsprite
//

int32_t insertspritesect(int16_t sectnum);
int32_t insertspritestat(int16_t statnum);
int32_t deletespritesect(int16_t deleteme);
int32_t deletespritestat(int16_t deleteme);

static inline int32_t insertsprite(int16_t sectnum, int16_t statnum)
{
    insertspritestat(statnum);
    return(insertspritesect(sectnum));
}

//
// deletesprite
//
static inline int32_t deletesprite(int16_t spritenum)
{
    deletespritestat(spritenum);
    return(deletespritesect(spritenum));
}

int32_t   changespritesect(int16_t spritenum, int16_t newsectnum);
int32_t   changespritestat(int16_t spritenum, int16_t newstatnum);
int32_t   setsprite(int16_t spritenum, const vec3_t *new);

int32_t   screencapture(char *filename, char inverseit);

int32_t   getclosestcol(int32_t r, int32_t g, int32_t b);

// PLAG: line utility functions
typedef struct  s_equation {
    float       a, b, c;
}               _equation;
typedef struct  s_point2d {
    float       x, y;
}               _point2d;
int32_t             wallvisible(int32_t x, int32_t y, int16_t wallnum);

#define STATUS2DSIZ 144
#define STATUS2DSIZ2 26

void   qsetmode640350(void);
void   qsetmode640480(void);
void   qsetmodeany(int32_t,int32_t);
void   clear2dscreen(void);
void   draw2dgrid(int32_t posxe, int32_t posye, int16_t ange, int32_t zoome, int16_t gride);
void   draw2dscreen(int32_t posxe, int32_t posye, int16_t ange, int32_t zoome, int16_t gride);
void   drawline16(int32_t x1, int32_t y1, int32_t x2, int32_t y2, char col);
void   drawcircle16(int32_t x1, int32_t y1, int32_t r, char col);

int32_t   setrendermode(int32_t renderer);
int32_t   getrendermode(void);

#ifdef POLYMOST
void    setrollangle(int32_t rolla);
#endif

//  pal: pass -1 to invalidate all palettes for the tile, or >=0 for a particular palette
//  how: pass -1 to invalidate all instances of the tile in texture memory, or a bitfield
//         bit 0: opaque or masked (non-translucent) texture, using repeating
//         bit 1: ignored
//         bit 2: 33% translucence, using repeating
//         bit 3: 67% translucence, using repeating
//         bit 4: opaque or masked (non-translucent) texture, using clamping
//         bit 5: ignored
//         bit 6: 33% translucence, using clamping
//         bit 7: 67% translucence, using clamping
//       clamping is for sprites, repeating is for walls
void invalidatetile(int16_t tilenume, int32_t pal, int32_t how);

int32_t animateoffs(int16_t tilenum, int16_t fakevar);

void setpolymost2dview(void);   // sets up GL for 2D drawing

int32_t polymost_drawtilescreen(int32_t tilex, int32_t tiley, int32_t wallnum, int32_t dimen, int32_t tilezoom);
void polymost_glreset(void);
void polymost_precache(int32_t dapicnum, int32_t dapalnum, int32_t datype);

#if defined(POLYMOST) && defined(USE_OPENGL)
extern int32_t glanisotropy;
extern int32_t glusetexcompr;
extern int32_t gltexfiltermode;
extern int32_t glredbluemode;
extern int32_t glusetexcache;
extern int32_t glmultisample, glnvmultisamplehint;
extern int32_t glwidescreen, glprojectionhacks;
extern int32_t gltexmaxsize;
void gltexapplyprops (void);
void invalidatecache(void);

extern int32_t r_depthpeeling, r_peelscount;
extern int32_t r_detailmapping;
extern int32_t r_glowmapping;
extern int32_t r_vertexarrays;
extern int32_t r_vbos;
extern int32_t r_vbocount;
extern int32_t r_animsmoothing;
extern int32_t r_parallaxskyclamping;
extern int32_t r_parallaxskypanning;
extern int32_t r_modelocclusionchecking;
extern int32_t r_fullbrights;
extern int32_t r_downsize;
extern int32_t r_downsizevar;
extern int32_t mdtims, omdtims;
extern int32_t glrendmode;
#endif

void hicinit(void);
// effect bitset: 1 = greyscale, 2 = invert
void hicsetpalettetint(int32_t palnum, char r, char g, char b, char effect);
// flags bitset: 1 = don't compress
int32_t hicsetsubsttex(int32_t picnum, int32_t palnum, char *filen, float alphacut, float xscale, float yscale, float specpower, float specfactor, char flags);
int32_t hicsetskybox(int32_t picnum, int32_t palnum, char *faces[6]);
int32_t hicclearsubst(int32_t picnum, int32_t palnum);

int32_t Ptile2tile(int32_t tile, int32_t pallet);
int32_t md_loadmodel(const char *fn);
int32_t md_setmisc(int32_t modelid, float scale, int32_t shadeoff, float zadd, int32_t flags);
// int32_t md_tilehasmodel(int32_t tilenume, int32_t pal);

typedef struct
{
    // maps build tiles to particular animation frames of a model
    int32_t     modelid;
    int32_t     skinnum;
    int32_t     framenum;   // calculate the number from the name when declaring
    float   smoothduration;
    int32_t     next;
    char    pal;
} tile2model_t;

#define EXTRATILES MAXTILES

EXTERN int32_t mdinited;
EXTERN tile2model_t tile2model[MAXTILES+EXTRATILES];

static inline int32_t md_tilehasmodel(int32_t tilenume,int32_t pal)
{
    if (!mdinited) return -1;
    return tile2model[Ptile2tile(tilenume,pal)].modelid;
}

int32_t md_defineframe(int32_t modelid, const char *framename, int32_t tilenume, int32_t skinnum, float smoothduration, int32_t pal);
int32_t md_defineanimation(int32_t modelid, const char *framestart, const char *frameend, int32_t fps, int32_t flags);
int32_t md_defineskin(int32_t modelid, const char *skinfn, int32_t palnum, int32_t skinnum, int32_t surfnum, float param, float specpower, float specfactor);
int32_t md_definehud (int32_t modelid, int32_t tilex, double xadd, double yadd, double zadd, double angadd, int32_t flags);
int32_t md_undefinetile(int32_t tile);
int32_t md_undefinemodel(int32_t modelid);

int32_t loaddefinitionsfile(char *fn);

extern int32_t mapversion;	// if loadboard() fails with -2 return, try loadoldboard(). if it fails with -2, board is dodgy
int32_t loadoldboard(char *filename, char fromwhere, int32_t *daposx, int32_t *daposy, int32_t *daposz, int16_t *daang, int16_t *dacursectnum);

// Hash functions

typedef struct _hashitem // size is 12/24 bits.
{
    char *string;
    int32_t key;
    struct _hashitem *next;
} hashitem_t;

typedef struct
{
    int32_t size;
    hashitem_t **items;
} hashtable_t;

void hash_init(hashtable_t *t);
void hash_free(hashtable_t *t);
int32_t  hash_findcase(hashtable_t *t, const char *s);
int32_t  hash_find(hashtable_t *t, const char *s);
void hash_replace(hashtable_t *t, const char *s, int32_t key);
void hash_add(hashtable_t *t, const char *s, int32_t key);

#ifdef POLYMER
# include "polymer.h"
#endif

#ifdef __cplusplus
}
#endif

#endif // __build_h__
