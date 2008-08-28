//-------------------------------------------------------------------------
/*
Copyright (C) 2004, 2007 - EDuke32 developers

This file is part of EDuke32

EDuke32 is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License version 2
as published by the Free Software Foundation.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
*/
//-------------------------------------------------------------------------

#include "compat.h"

#define TICSPERFRAME 3

// #define VULGARITY

char *defsfilename = "duke3d.def";

extern char keystatus[];
extern short defaultspritecstat;
extern int posx, posy, posz, horiz, qsetmode;
extern short ang, cursectnum;
extern short ceilingheinum, floorheinum;
extern char names[MAXTILES][25];

extern int ydim16, xdimgame, ydimgame, bppgame, xdim2d, ydim2d;

extern int zmode, kensplayerheight, zlock;

extern short editstatus, searchit;
extern int searchx, searchy, osearchx, osearchy;      //search input
extern short searchsector, searchwall, searchstat;     //search output

#define AVERAGEFRAMES 128
#define COLOR_RED 248
#define COLOR_WHITE 31

static int ototalclock = 0, clockval[AVERAGEFRAMES], clockcnt = 0;

#define NUMOPTIONS 9

char option[NUMOPTIONS] = {0,0,0,0,0,0,1,0,0};
unsigned char keys[NUMBUILDKEYS] =
    {
        0xc8,0xd0,0xcb,0xcd,0x2a,0x9d,0x1d,0x39,
        0x1e,0x2c,0xd1,0xc9,0x47,0x49,
	0x9c,0x1c,0xd,0xc,0xf,0x29
    };

int nextvoxid = 0;

extern int whitecol;
extern char vgapal16[4*256];
extern void AutoAlignWalls(int nWall0, int ply);
extern char changechar(char dachar, int dadir, char smooshyalign, char boundcheck);
extern void updatenumsprites(void);

extern int lastpm16time, synctics;
extern int halfxdim16, midydim16, zoom;
extern void fixrepeats(short i);

char autospritehelp=0,autosecthelp=0;
short MinRate=24, MinD=3;
int xoldtimerhandler, lastmessagetime=-1;

char tempbuf[1024]; //1024
int numsprite[MAXSPRITES];
int multisprite[MAXSPRITES];
char lo[32];
char levelname[255];
short curwall=0,curwallnum=0;
short cursearchsprite=0,cursearchspritenum=0,cursector_lotag=0,cursectornum=0;
short search_lotag=0,search_hitag=0;
char wallsprite=0;
char helpon=0;
//static char onwater=0;
unsigned char onnames=4, usedcount=1;
short cursprite;
int mousxplc, mousyplc, ppointhighlight;
int counter=0;
unsigned char nosprites=0,purpleon=0,skill=4;
unsigned char framerateon=1,shadepreview=0,sidemode=0;
int autosave=180;
extern int vel, svel, hvel, angvel;
int xvel, yvel, timoff;

static void SearchSectorsForward();
static void SearchSectorsBackward();
static inline void SpriteName(short spritenum, char *lo2);
static void PrintStatus(char *string,int num,char x,char y,char color);
static inline void SetBOSS1Palette();
static inline void SetSLIMEPalette();
static inline void SetWATERPalette();
static inline void SetGAMEPalette();

extern short grid;

static void EditSpriteData(short spritenum);
static void EditWallData(short wallnum);
static void EditSectorData(short sectnum);
static void FuncMenu(void);

char GAMEpalette[768];
char WATERpalette[768];
char SLIMEpalette[768];
char TITLEpalette[768];
char REALMSpalette[768];
char BOSS1palette[768];

char num_tables;

int updownunits=1024;
extern short highlightsector[MAXSECTORS], highlightsectorcnt;
extern short highlight[MAXWALLS];
extern short pointhighlight, linehighlight, highlightcnt;
extern short asksave;

char getmessage[162], getmessageleng;
int getmessagetimeoff, charsperline;
extern int startposx, startposy, startposz;
extern short startang, startsectnum;

int autosavetimer;
extern int numsprites;
extern int showfirstwall;
extern char spritecol2d[MAXTILES][2];
extern char custom2dcolors;

int intro=0;
extern int ydim16, halfxdim16, midydim16, zoom;
extern intptr_t frameplace;
extern char pow2char[8];

static int acurpalette=0;

extern void showsectordata(short sectnum);
extern void showwalldata(short wallnum);
extern void showspritedata(short spritenum);
extern int checksectorpointer(short i, short sectnum);

extern double msens;

void ContextHelp(short spritenum);
void ResetKeys();

extern void fixspritesectors(void);
 #define KEY_PRESSED(sc) KB_KeyPressed((sc))

// This table defines the various zoom levels, the numbers being the pixel width
//   and height of the sprite when plotted on the screen. Probably zooms in too far
//   for some, but I'm a blind old git :-(

#define FUCKING_GOOD_EYESIGHT 16

static const int ZoomToThumbSize[] =
{
	FUCKING_GOOD_EYESIGHT, 32, 64, 128, 192, 256, 384, 512
} ;

#define NUM_ZOOMS (sizeof(ZoomToThumbSize)/sizeof(ZoomToThumbSize[0]))

#define INITIAL_ZOOM 2

typedef struct
{
	int *pIds ;	// ptr to list of tile Ids
	int  nIds ;		// num of tile ids
	char *szText ;		// description to present to user.
	char  key1 ;		// key1 and key2 are two alternative keypresses used to
	char  key2 ;		//    select tile set. Bodge to do eary upper/lower case handling
    char  color1;       // 2d sprite color 1
    char  color2;       // 2d sprite color 2
} TileGroup;

#define MAX_TILE_GROUPS 32
#define MAX_TILE_GROUP_ENTRIES 1024

TileGroup s_TileGroups[MAX_TILE_GROUPS];

int tilegroupItems;
int tilegroupActors;

static unsigned int tile_groups = 0;

#define FIRST_USER_ART_TILE 3584
// Some atomic tiles are sprinkled in the V1.3d's area but
//   this is where the main atomic tiles start :-
#define FIRST_ATOMIC_TILE   4096
#define FIRST_EXTENDED_TILE 6144
#define SECOND_EXTENDED_TILE 9216

extern short localartfreq[MAXTILES];
extern short localartlookup[MAXTILES], localartlookupnum;
extern int lockclock;
extern void clearkeys(void);
