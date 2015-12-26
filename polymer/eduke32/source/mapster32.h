//-------------------------------------------------------------------------
/*
Copyright (C) 2010 EDuke32 developers and contributors

This file is part of EDuke32.

EDuke32 is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License version 2
as published by the Free Software Foundation.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/
//-------------------------------------------------------------------------

#include "compat.h"

#define SETUPFILENAME "mapster32.cfg"

#define TICSPERFRAME 3

// #define VULGARITY

//#define NUMOPTIONS 9
//char option[NUMOPTIONS] = {0,0,0,0,0,0,1,0,0};

char default_buildkeys[NUMBUILDKEYS] =
{
	0xc8,0xd0,0xcb,0xcd,0x2a,0x9d,0x1d,0x39,
	0x1e,0x2c,0xd1,0xc9,0x47,0x49,
	0x9c,0x1c,0xd,0xc,0xf,0x29
};

extern char changechar(char dachar, int32_t dadir, char smooshyalign, char boundcheck);

static char autospritehelp=0, autosecthelp=0;
//static int32_t lastmessagetime=-1;

static char tempbuf[1024];
static int32_t numsprite[MAXTILES], multisprite[MAXTILES];
static char lo[64];
static int16_t curwall=0, curwallnum=0;
static int16_t /*cursearchsprite=0,*/ cursearchspritenum=0, cursector_lotag=0, cursectornum=0;
static int16_t search_lotag=0,search_hitag=0;
static char wallsprite=0;
static char helpon=0;
//static char onwater=0;
static uint8_t onnames=4, usedcount=1;
static int16_t cursprite;
static int32_t ppointhighlight;
//static int32_t counter=0;
static uint8_t nosprites=0,purpleon=0,skill=4;
static uint8_t framerateon=1,shadepreview=0;

static int32_t autosavetimer;

static void SearchSectors(int32_t dir);
static inline void SpriteName(int16_t spritenum, char *lo2);
//static void PrintStatus(const char *string,int32_t num,char x,char y,char color);

static void EditSpriteData(int16_t spritenum);
static void EditWallData(int16_t wallnum);
static void EditSectorData(int16_t sectnum);


static int32_t updownunits=1024;


//int32_t intro=0;

//void ContextHelp(int16_t spritenum);
//void ResetKeys();

//#define KEY_PRESSED(sc) KB_KeyPressed((sc))

// This table defines the various zoom levels, the numbers being the pixel width
//   and height of the sprite when plotted on the screen. Probably zooms in too far
//   for some, but I'm a blind old git :-(

#define FUCKING_GOOD_EYESIGHT 16
static const int32_t ZoomToThumbSize[] =
{
	FUCKING_GOOD_EYESIGHT, 32, 64, 128, 192, 256, 384, 512
};

#define NUM_ZOOMS ARRAY_SIZE(ZoomToThumbSize)
#define INITIAL_ZOOM 2

typedef struct
{
	int32_t *pIds;  // ptr to list of tile Ids
	int32_t  nIds;  // num of tile ids
	char *szText;   // description to present to user.
	char  key1;     // key1 and key2 are two alternative keypresses used to
	char  key2;     //    select tile set. Bodge to do eary upper/lower case handling
    char  color1;   // 2d sprite color 1
    char  color2;   // 2d sprite color 2
} TileGroup;

#define MAX_TILE_GROUPS 32
#define MAX_TILE_GROUP_ENTRIES MAXUSERTILES

static TileGroup s_TileGroups[MAX_TILE_GROUPS];
static int32_t tilegroupItems;
static int32_t tilegroupActors;
static int32_t tile_groups = 0;


#define NUMPRINTABLES 94
#define MAX_ALPHABETS 32
// alphabet definitions struct for the sprite text renderer
typedef struct
{
    int16_t pic[NUMPRINTABLES];  // maps printable ASCII chars to tilenums
    int8_t xofs[NUMPRINTABLES];  // additional offset
    int8_t yofs[NUMPRINTABLES];
} alphabet_t;

static alphabet_t alphabets[MAX_ALPHABETS];
static int32_t numalphabets = 0;

#define FIRST_USER_ART_TILE 3584
// Some atomic tiles are sprinkled in the V1.3d's area but
//   this is where the main atomic tiles start :-
#define FIRST_ATOMIC_TILE   4096
#define FIRST_EXTENDED_TILE 6144
#define SECOND_EXTENDED_TILE 9216

extern int16_t localartfreq[MAXTILES];
extern int16_t localartlookup[MAXTILES], localartlookupnum;
extern int32_t lockclock;

static int32_t gs_spriteTagValue[3][7];
static char gs_spriteTagInterested[3][7];
static int16_t gs_cursprite=-1;

//int32_t g_musicSize=0;
