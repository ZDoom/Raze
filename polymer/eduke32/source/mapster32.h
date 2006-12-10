//-------------------------------------------------------------------------
/*
Copyright (C) 2005 - Richard Gobeille

This file is part of Mapster32

Mapster32 is free software; you can redistribute it and/or
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

#define TICSPERFRAME 3

// #define VULGARITY

char *defsfilename = "duke3d.def";

extern char keystatus[];
extern short defaultspritecstat;
extern long posx, posy, posz, horiz, qsetmode;
extern short ang, cursectnum;
extern short ceilingheinum, floorheinum;
extern char names[MAXTILES][25];

extern long ydim16, xdimgame, ydimgame, bppgame, xdim2d, ydim2d;

extern long zmode, kensplayerheight, zlock;

extern short editstatus, searchit;
extern long searchx, searchy, osearchx, osearchy;      //search input
extern short searchsector, searchwall, searchstat;     //search output

extern short temppicnum, tempcstat, templotag, temphitag, tempextra;
extern unsigned char tempshade, temppal, tempvis, tempxrepeat, tempyrepeat, somethingintab;

static long ototalclock = 0, clockval[16], clockcnt = 0;

#define NUMOPTIONS 9
#define NUMKEYS 19

char option[NUMOPTIONS] = {0,0,0,0,0,0,1,0,0};
unsigned char keys[NUMBUILDKEYS] =
    {
        0xc8,0xd0,0xcb,0xcd,0x2a,0x9d,0x1d,0x39,
        0x1e,0x2c,0xd1,0xc9,0x47,0x49,
	0x9c,0x1c,0xd,0xc,0xf,0x29
    };

int nextvoxid = 0;

extern long whitecol;
extern char vgapal16[4*256];
extern void AutoAlignWalls(long nWall0, long ply);
extern char changechar(char dachar, long dadir, char smooshyalign, char boundcheck);
extern void _printmessage16(char name[82]);
extern void updatenumsprites(void);

extern long lastpm16time, synctics;
extern long halfxdim16, midydim16, zoom;
extern void fixrepeats(short i);

char autospritehelp=0,autosecthelp=0;
short MinRate=24, MinD=3;
long xoldtimerhandler, lastmessagetime;

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
long mousxplc, mousyplc, ppointhighlight;
int counter=0;
unsigned char nosprites=0,purpleon=0,skill=4;
unsigned char framerateon=1,tabgraphic=2,shadepreview=0,autosave=1,sidemode=0;
extern long vel, svel, hvel, angvel;
long xvel, yvel, timoff;

static void SearchSectorsForward();
static void SearchSectorsBackward();
static inline void SpriteName(short spritenum, char *lo2);
static int ActorMem(long i);
static void PrintStatus(char *string,int num,char x,char y,char color);
static void SetBOSS1Palette();
static void SetSLIMEPalette();
static void SetWATERPalette();
static void SetGAMEPalette();
static void kensetpalette(char *vgapal);

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
long getmessagetimeoff, charsperline;
extern long startposx, startposy, startposz;
extern short startang, startsectnum;

long autosavetimer;
extern long numsprites;
extern char spritecol2d[MAXTILES][2];
extern char custom2dcolors;
extern char mlook;

int intro=0;
extern long frameplace, ydim16, halfxdim16, midydim16, zoom;
extern char pow2char[8];

static int acurpalette=0;

extern void showsectordata(short sectnum);
extern void showwalldata(short wallnum);
extern void showspritedata(short spritenum);
extern long checksectorpointer(short i, short sectnum);

extern double msens;

void ContextHelp(short spritenum);
void ResetKeys();

extern void clearfilenames(void), fixspritesectors(void);
 #define KEY_PRESSED(sc) KB_KeyPressed((sc))
