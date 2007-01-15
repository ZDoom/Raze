// "Build Engine & Tools" Copyright (c) 1993-1997 Ken Silverman
// Ken Silverman's official web site: "http://www.advsys.net/ken"
// See the included license file "BUILDLIC.TXT" for license info.
//
// This file has been modified from Ken Silverman's original release
// by Jonathon Fowler (jonof@edgenetwk.com)


#ifndef __build_h__
#define __build_h__

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
#define MAXSPRITES MAXSPRITESV8

#define MAXTILES 15360
#define MAXVOXELS 4096
#define MAXSTATUS 1024
#define MAXPLAYERS 16
#define MAXXDIM 1600
#define MAXYDIM 1200
#define MAXPALOOKUPS 256
#define MAXPSKYTILES 256
#define MAXSPRITESONSCREEN 4096
#define MAXUNIQHUDID 256 //Extra slots so HUD models can store animation state without messing game sprites

#define RESERVEDPALS 1
#define DETAILPAL MAXPALOOKUPS - 1

#define CLIPMASK0 (((1L)<<16)+1L)
#define CLIPMASK1 (((256L)<<16)+64L)

    //Make all variables in BUILD.H defined in the ENGINE,
    //and externed in GAME
#ifdef ENGINE
#  define EXTERN
#else
#  define EXTERN extern
#endif

#ifdef __GNUC__
#define BPACK __attribute__ ((packed))
#else
#define BPACK
#endif

#ifdef _MSC_VER
#pragma pack(1)
#endif

#ifdef __WATCOMC__
#pragma pack(push,1);
#endif


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
typedef struct BPACK
{
    short wallptr, wallnum;
    long ceilingz, floorz;
    short ceilingstat, floorstat;
    short ceilingpicnum, ceilingheinum;
    signed char ceilingshade;
    char ceilingpal, ceilingxpanning, ceilingypanning;
    short floorpicnum, floorheinum;
    signed char floorshade;
    char floorpal, floorxpanning, floorypanning;
    char visibility, filler;
    short lotag, hitag, extra;
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
typedef struct BPACK
{
    long x, y;
    short point2, nextwall, nextsector, cstat;
    short picnum, overpicnum;
    signed char shade;
    char pal, xrepeat, yrepeat, xpanning, ypanning;
    short lotag, hitag, extra;
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
//   bits 10-14: reserved
//   bit 15: 1 = Invisible sprite, 0 = not invisible

    //44 bytes
typedef struct BPACK
{
    long x, y, z;
    short cstat, picnum;
    signed char shade;
    char pal, clipdist, filler;
    unsigned char xrepeat, yrepeat;
    signed char xoffset, yoffset;
    short sectnum, statnum;
    short ang, owner, xvel, yvel, zvel;
    short lotag, hitag, extra;
} spritetype;

typedef struct BPACK {
    unsigned long mdanimtims;
    short mdanimcur;
    short angoff;
    short pitch, roll;
    long xoff, yoff, zoff;
    unsigned char flags;
    char filler[3];
} spriteexttype;
#define SPREXT_NOTMD 1
#define SPREXT_NOMDANIM 2
EXTERN spriteexttype spriteext[MAXSPRITES+MAXUNIQHUDID];
EXTERN long guniqhudid;

EXTERN sectortype sector[MAXSECTORS];
EXTERN walltype wall[MAXWALLS];
EXTERN spritetype sprite[MAXSPRITES];

EXTERN long spritesortcnt;
EXTERN spritetype tsprite[MAXSPRITESONSCREEN];

EXTERN long xdim, ydim, ylookup[MAXYDIM+1], numpages;
EXTERN long yxaspect, viewingrange;

#define MAXVALIDMODES 256
EXTERN long validmodecnt;
struct validmode_t {
    long xdim,ydim;
    char bpp;
    char fs;    // bit 0 = fullscreen flag
    char filler[2];
    long extra; // internal use
};
EXTERN struct validmode_t validmode[MAXVALIDMODES];

EXTERN short numsectors, numwalls;
EXTERN /*volatile*/ long totalclock;
EXTERN long numframes, randomseed;
EXTERN short sintable[2048];
EXTERN char palette[768];
EXTERN short numpalookups;
EXTERN char *palookup[MAXPALOOKUPS];
EXTERN char parallaxtype, showinvisibility;
EXTERN long parallaxyoffs, parallaxyscale;
EXTERN long visibility, parallaxvisibility;

EXTERN long windowx1, windowy1, windowx2, windowy2;
EXTERN short startumost[MAXXDIM], startdmost[MAXXDIM];

EXTERN short pskyoff[MAXPSKYTILES], pskybits;

EXTERN short headspritesect[MAXSECTORS+1], headspritestat[MAXSTATUS+1];
EXTERN short prevspritesect[MAXSPRITES], prevspritestat[MAXSPRITES];
EXTERN short nextspritesect[MAXSPRITES], nextspritestat[MAXSPRITES];

EXTERN short tilesizx[MAXTILES], tilesizy[MAXTILES];
EXTERN char picsiz[MAXTILES];
EXTERN char walock[MAXTILES];
EXTERN long pow2long[32];
EXTERN long numtiles, picanm[MAXTILES], waloff[MAXTILES];

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
extern char vgapalette[5*256];
extern unsigned long drawlinepat;

extern void faketimerhandler(void);

extern char apptitle[256];
typedef struct {
    unsigned char r,g,b,f;
} palette_t;
extern palette_t curpalette[256], curpalettefaded[256], palfadergb;
extern char palfadedelta;

extern long dommxoverlay, novoxmips;

#ifdef SUPERBUILD
extern long tiletovox[MAXTILES];
extern long usevoxels, voxscale[MAXVOXELS];
#endif
#ifdef POLYMOST
extern long usemodels, usehightile;
#endif

extern char *engineerrstr;
extern char noclip;

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
         �������������������������������Ŀ
               Sector lists:               Status lists:               
         �������������������������������Ĵ
           Sector0:  4, 5, 8             Status0:  2, 0, 8             
           Sector1:  16, 2, 0, 7         Status1:  4, 5, 16, 7, 3, 9   
           Sector2:  3, 9                                              
         ��������������������������������
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

int    preinitengine(void);	// a partial setup of the engine used for launch windows
int    initengine(void);
void   uninitengine(void);
void   initspritelists(void);
long   loadboard(char *filename, char fromwhere, long *daposx, long *daposy, long *daposz, short *daang, short *dacursectnum);
long   loadmaphack(char *filename);
long   saveboard(char *filename, long *daposx, long *daposy, long *daposz, short *daang, short *dacursectnum);
long   loadpics(char *filename, long askedsize);
void   loadtile(short tilenume);
long   qloadkvx(long voxindex, char *filename);
long   allocatepermanenttile(short tilenume, long xsiz, long ysiz);
void   copytilepiece(long tilenume1, long sx1, long sy1, long xsiz, long ysiz, long tilenume2, long sx2, long sy2);
void   makepalookup(long palnum, char *remapbuf, signed char r, signed char g, signed char b, char dastat);
void   setvgapalette(void);
void   setbrightness(char dabrightness, char *dapal, char noapply);
void   setpalettefade(char r, char g, char b, char offset);
void   squarerotatetile(short tilenume);

long   setgamemode(char davidoption, long daxdim, long daydim, long dabpp);
void   nextpage(void);
void   setview(long x1, long y1, long x2, long y2);
void   setaspect(long daxrange, long daaspect);
void   flushperms(void);

void   plotpixel(long x, long y, char col);
char   getpixel(long x, long y);
void   setviewtotile(short tilenume, long xsiz, long ysiz);
void   setviewback(void);
void   preparemirror(long dax, long day, long daz, short daang, long dahoriz, short dawall, short dasector, long *tposx, long *tposy, short *tang);
void   completemirror(void);

void   drawrooms(long daposx, long daposy, long daposz, short daang, long dahoriz, short dacursectnum);
void   drawmasks(void);
void   clearview(long dacol);
void   clearallviews(long dacol);
void   drawmapview(long dax, long day, long zoome, short ang);
void   rotatesprite(long sx, long sy, long z, short a, short picnum, signed char dashade, char dapalnum, char dastat, long cx1, long cy1, long cx2, long cy2);
void   drawline256(long x1, long y1, long x2, long y2, char col);
void   printext16(long xpos, long ypos, short col, short backcol, char *name, char fontsize);
void   printext256(long xpos, long ypos, short col, short backcol, char *name, char fontsize);

long   clipmove(long *x, long *y, long *z, short *sectnum, long xvect, long yvect, long walldist, long ceildist, long flordist, unsigned long cliptype);
long   clipinsidebox(long x, long y, short wallnum, long walldist);
long   clipinsideboxline(long x, long y, long x1, long y1, long x2, long y2, long walldist);
long   pushmove(long *x, long *y, long *z, short *sectnum, long walldist, long ceildist, long flordist, unsigned long cliptype);
void   getzrange(long x, long y, long z, short sectnum, long *ceilz, long *ceilhit, long *florz, long *florhit, long walldist, unsigned long cliptype);
long   hitscan(long xs, long ys, long zs, short sectnum, long vx, long vy, long vz, short *hitsect, short *hitwall, short *hitsprite, long *hitx, long *hity, long *hitz, unsigned long cliptype);
long   neartag(long xs, long ys, long zs, short sectnum, short ange, short *neartagsector, short *neartagwall, short *neartagsprite, long *neartaghitdist, long neartagrange, char tagsearch);
long   cansee(long x1, long y1, long z1, short sect1, long x2, long y2, long z2, short sect2);
void   updatesector(long x, long y, short *sectnum);
void   updatesectorz(long x, long y, long z, short *sectnum);
long   inside(long x, long y, short sectnum);
void   dragpoint(short pointhighlight, long dax, long day);
void   setfirstwall(short sectnum, short newfirstwall);

void   getmousevalues(long *mousx, long *mousy, long *bstatus);
long    krand(void);
long   ksqrt(long num);
long   getangle(long xvect, long yvect);
void   rotatepoint(long xpivot, long ypivot, long x, long y, short daang, long *x2, long *y2);
long   lastwall(short point);
long   nextsectorneighborz(short sectnum, long thez, short topbottom, short direction);
long   getceilzofslope(short sectnum, long dax, long day);
long   getflorzofslope(short sectnum, long dax, long day);
void   getzsofslope(short sectnum, long dax, long day, long *ceilz, long *florz);
void   alignceilslope(short dasect, long x, long y, long z);
void   alignflorslope(short dasect, long x, long y, long z);
long   sectorofwall(short theline);
long   loopnumofsector(short sectnum, short wallnum);

long   insertsprite(short sectnum, short statnum);
long   deletesprite(short spritenum);
long   changespritesect(short spritenum, short newsectnum);
long   changespritestat(short spritenum, short newstatnum);
long   setsprite(short spritenum, long newx, long newy, long newz);

long   screencapture(char *filename, char inverseit);

// PLAG: line utility functions
typedef struct  s_equation {
    float       a, b, c;
}               _equation;
typedef struct  s_point2d {
    float       x, y;
}               _point2d;
_equation       equation(long x1, long y1, long x2, long y2);
int             sameside(_equation* eq, _point2d* p1, _point2d* p2);

#define STATUS2DSIZ 144
void   qsetmode640350(void);
void   qsetmode640480(void);
void   qsetmodeany(long,long);
void   clear2dscreen(void);
void   draw2dgrid(long posxe, long posye, short ange, long zoome, short gride);
void   draw2dscreen(long posxe, long posye, short ange, long zoome, short gride);
void   drawline16(long x1, long y1, long x2, long y2, char col);
void   drawcircle16(long x1, long y1, long r, char col);

int   setrendermode(int renderer);
int   getrendermode(void);

void    setrollangle(long rolla);

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
void invalidatetile(short tilenume, long pal, long how);

long animateoffs(short tilenum, short fakevar);

void setpolymost2dview(void);   // sets up GL for 2D drawing

long polymost_drawtilescreen(long tilex, long tiley, long wallnum, long dimen);
void polymost_glreset(void);
void polymost_precache(long dapicnum, long dapalnum, long datype);

#if defined(POLYMOST) && defined(USE_OPENGL)
extern long glanisotropy;
extern long glusetexcompr;
extern long gltexfiltermode;
extern long glredbluemode;
extern long glusetexcache, glusetexcachecompression;
extern long glmultisample, glnvmultisamplehint;
extern long glwidescreen, glprojectionhacks;
void gltexapplyprops (void);

extern long r_depthpeeling, r_peelscount;
extern long r_detailmapping;
#endif

void hicinit(void);
// effect bitset: 1 = greyscale, 2 = invert
void hicsetpalettetint(long palnum, unsigned char r, unsigned char g, unsigned char b, unsigned char effect);
// flags bitset: 1 = don't compress
int hicsetsubsttex(long picnum, long palnum, char *filen, float alphacut, char flags);
int hicsetskybox(long picnum, long palnum, char *faces[6]);
int hicclearsubst(long picnum, long palnum);

int md_loadmodel(const char *fn);
int md_setmisc(int modelid, float scale, int shadeoff, float zadd);
int md_tilehasmodel(int tilenume);
int md_defineframe(int modelid, const char *framename, int tilenume, int skinnum);
int md_defineanimation(int modelid, const char *framestart, const char *frameend, int fps, int flags);
int md_defineskin(int modelid, const char *skinfn, int palnum, int skinnum, int surfnum);
int md_definehud (int modelid, int tilex, double xadd, double yadd, double zadd, double angadd, int flags);
int md_undefinetile(int tile);
int md_undefinemodel(int modelid);

int loaddefinitionsfile(char *fn);

extern long mapversion;	// if loadboard() fails with -2 return, try loadoldboard(). if it fails with -2, board is dodgy
long loadoldboard(char *filename, char fromwhere, long *daposx, long *daposy, long *daposz, short *daang, short *dacursectnum);

#ifdef _MSC_VER
#pragma pack()
#endif

#ifdef __WATCOMC__
#pragma pack(pop)
#endif

#undef BPACK

#ifdef __cplusplus
}
#endif

#endif // __build_h__
