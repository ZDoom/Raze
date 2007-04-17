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

extern void fixspritesectors(void);
 #define KEY_PRESSED(sc) KB_KeyPressed((sc))

// REM : This file should be included in file BUILD.C _ONLY_

#include "names.h"



// This table defines the various zoom levels, the numbers being the pixel width
//   and height of the sprite when plotted on the screen. Probably zooms in too far
//   for some, but I'm a blind old git :-(

#define FUCKING_GOOD_EYESIGHT 16

static const long ZoomToThumbSize[] =
{
	FUCKING_GOOD_EYESIGHT, 32, 64, 128, 192, 256, 384, 512
} ;

#define NUM_ZOOMS (sizeof(ZoomToThumbSize)/sizeof(ZoomToThumbSize[0]))

#define INITIAL_ZOOM 2


// If I wasn't a lazy sod I'd have made these loadable from external config files.

	///////////////////
	// ACTOR SPRITES //
	///////////////////

static const long s_idActors[] =
{
	LIZTROOP, LIZTROOPRUNNING, LIZTROOPSTAYPUT, LIZTROOPSHOOT, LIZTROOPJETPACK,
	LIZTROOPONTOILET, LIZTROOPJUSTSIT, LIZTROOPDUCKING,
	PIGCOP, PIGCOPSTAYPUT, PIGCOPDIVE,
	LIZMAN, LIZMANSTAYPUT, LIZMANSPITTING, LIZMANFEEDING, LIZMANJUMP,
	COMMANDER, COMMANDERSTAYPUT,
	OCTABRAIN, OCTABRAINSTAYPUT,
	ORGANTIC,
	NEWBEAST, NEWBEASTSTAYPUT, NEWBEASTJUMP,
	EGG, GREENSLIME, ROTATEGUN, RECON, TANK, BOUNCEMINE,
	FLOORFLAME,
	// FEMS
	FEM1, FEM2, FEM3, FEM4, FEM5, FEM6, FEM7, FEM8, FEM9, FEM10, NAKED1,
	// Lil' critters
	SHARK,
	// BIG critters
	BOSS1, BOSS1STAYPUT, BOSS1SHOOT, BOSS1LOB,
	BOSS2,
	BOSS3,
	BOSS4, BOSS4STAYPUT
} ;

#define NUM_ACTORS (sizeof(s_idActors)/sizeof(s_idActors[0]))

	/////////////////
	// DOOR TILES ///
	/////////////////

// Not all of these are tuely door tiles but are useable as such

static const long s_idDoors[] =
{
	DOORTILE1, DOORTILE2, DOORTILE3, DOORTILE4, DOORTILE5,
	DOORTILE6, DOORTILE7, DOORTILE8, DOORTILE9, DOORTILE10,
	312, 313, 314, 345,
	DOORTILE22, DOORTILE18, DOORTILE19, DOORTILE20,
	450, 455, 457, 458, 459, 469, 470, 477, 
	DOORTILE14,
	719, 735, 771,
	DOORTILE16,
	843, 858, 883,
	DOORTILE15, DOORTILE21,
	1173,
	DOORTILE11,DOORTILE12,
	353, 355,
	// Related items
	DOORSHOCK, ACCESSCARD
};

#define NUM_DOORS (sizeof(s_idDoors)/sizeof(s_idDoors[0]))

	//////////////////
	// SWITCH TILES //
	//////////////////

static const long s_idSwitches[] =
{
	ACCESSSWITCH,ACCESSSWITCH2, SLOTDOOR, LIGHTSWITCH, SPACEDOORSWITCH, SPACELIGHTSWITCH,
	FRANKENSTINESWITCH, MULTISWITCH,
	DIPSWITCH, DIPSWITCH2, DIPSWITCH3, TECHSWITCH,
	LIGHTSWITCH2, LIGHTSWITCH2+1,
	POWERSWITCH1, LOCKSWITCH1, POWERSWITCH2, HANDSWITCH, PULLSWITCH,
	ALIENSWITCH, HANDPRINTSWITCH, NUKEBUTTON,
	TARGET,
	4083, 4954 // Busted switches (Atomic)
} ;

#define NUM_SWITCHES (sizeof(s_idSwitches)/sizeof(s_idSwitches[0]))

	///////////////////
	// GOODIES TILES //
	///////////////////

static const long s_idGoodies[] =
{
	// Ammo
	AMMO,SHOTGUNAMMO, BATTERYAMMO, RPGAMMO, HEAVYHBOMB, FREEZEAMMO, GROWAMMO, CRYSTALAMMO,
	DEVISTATORAMMO, HBOMBAMMO,
	
	// Items (health,etc)
	COLA, SIXPAK, FIRSTAID, SHIELD, STEROIDS, AIRTANK, JETPACK, HEATSENSOR, ACCESSCARD,
	BOOTS, ATOMICHEALTH,
	
	// Weapons
	FIRSTGUNSPRITE, CHAINGUNSPRITE, RPGSPRITE, FREEZESPRITE, SHRINKERSPRITE, 
	TRIPBOMBSPRITE, SHOTGUNSPRITE, DEVISTATORSPRITE

} ;

#define NUM_GOODIES (sizeof(s_idGoodies)/sizeof(s_idGoodies[0]))

	////////////////////////
	// ITEMS THAT RESPAWN //
	////////////////////////

// Thought there were more than just these :-(

static const long s_idRespawns[] =
{
	CANWITHSOMETHING, CANWITHSOMETHING2, CANWITHSOMETHING3, CANWITHSOMETHING4,
	// FEMS
	FEM1, FEM2, FEM3, FEM4, FEM5, FEM6, FEM7, FEM8, FEM9, FEM10, NAKED1,
} ;

#define NUM_RESPAWNS (sizeof(s_idRespawns)/sizeof(s_idRespawns[0]))

	////////////////////////
	// EXPLOSION AND FIRE //
	////////////////////////

static const long s_idExplosions[] =
{
	CRACK1, CRACK2, CRACK3, CRACK4,
	FIREEXT, SEENINE, OOZFILTER,
	EXPLODINGBARREL, EXPLODINGBARREL2, FIREBARREL, GUNPOWDERBARREL,
	REACTOR2SPARK, BOLT1, SIDEBOLT1,
	CEILINGSTEAM,
	FIREVASE, 2066, BURNING, FIRE, BURNING2, FIRE2
} ;

#define NUM_EXPLOSIONS (sizeof(s_idExplosions)/sizeof(s_idExplosions[0]))

	////////////////////////////////////////////////////////////////
	// BUILD STRUCTURE USED IN SELECTING ONE OF ABOVE LTILE LISTS //
	////////////////////////////////////////////////////////////////

const struct
{
	const long *pIds ;	// ptr to list of tile Ids
	long  nIds ;		// num of tile ids
	char  key1 ;		// key1 and key2 are two alternative keypresses used to
	char  key2 ;		//    select tile set. Bodge to do eary upper/lower case handling
	char *szText ;		// description to present to user.
} s_TileGroups[] =
{
	{ s_idActors,     NUM_ACTORS,     'A', 'a', "(A) Actor group" },
	{ s_idDoors,      NUM_DOORS,      'D', 'd', "(D) Door group" },
	{ s_idGoodies,    NUM_GOODIES,    'G', 'g', "(G) Goodies (ammo, weapons, etc)" },
	{ s_idRespawns,   NUM_RESPAWNS,   'R', 'r', "(R) Respawning Items" },
	{ s_idSwitches,   NUM_SWITCHES,   'S', 's', "(S) Switch group" },
	{ s_idExplosions, NUM_EXPLOSIONS, 'X', 'x', "(X) eXplosions, fire, sparks, etc" }
} ;

#define NUM_TILE_GROUPS (sizeof(s_TileGroups)/sizeof(s_TileGroups[0]))


#define FIRST_USER_ART_TILE 3584
// Some atomic tiles are sprinkled in the V1.3d's area but
//   this is where the main atomic tiles start :-
#define FIRST_ATOMIC_TILE   4096

extern short localartfreq[MAXTILES];
extern short localartlookup[MAXTILES], localartlookupnum;
extern long lockclock;
extern void clearkeys(void);
