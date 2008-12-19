//-------------------------------------------------------------------------
/*
Copyright (C) 1996, 2003 - 3D Realms Entertainment
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
#include "build.h"
#include "editor.h"
#include "pragmas.h"
#include "baselayer.h"
#include "names.h"
#include "osd.h"
#include "osdfuncs.h"
#include "cache1d.h"

#include "mapster32.h"
#include "keys.h"
#include "types.h"
#include "keyboard.h"
#include "scriptfile.h"
#include "crc32.h"

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <shellapi.h>
#endif

#define BUILDDATE " 20081215"
#define VERSION " 1.2.0devel"

static int floor_over_floor;

// static char *startwin_labeltext = "Starting Mapster32...";
static char setupfilename[BMAX_PATH]= "mapster32.cfg";
static char defaultduke3dgrp[BMAX_PATH] = "duke3d.grp";
static char *duke3dgrp = defaultduke3dgrp;
static int fixmapbeforesaving = 1;
static int lastsave = -180*60;
static int NoAutoLoad = 0;
int spnoclip=1;

#if !defined(_WIN32)
static int usecwd = 0;
#endif

static struct strllist
{
    struct strllist *next;
    char *str;
}
*CommandPaths = NULL, *CommandGrps = NULL;

#define MAXHELP2D (signed int)(sizeof(Help2d)/sizeof(Help2d[0]))

#define eitherALT   (keystatus[KEYSC_LALT]|  keystatus[KEYSC_RALT])
#define eitherCTRL  (keystatus[KEYSC_LCTRL]| keystatus[KEYSC_RCTRL])
#define eitherSHIFT (keystatus[KEYSC_LSHIFT]|keystatus[KEYSC_RSHIFT])

static char *Help2d[]=
{
    " 'A = Autosave toggle",
    " 'J = Jump to location",
    " 'L = Adjust sprite/wall coords",
    " 'S = Sprite size",
    " '3 = Caption mode",
    " '7 = Swap tags",
    " 'F = Special functions",
    " X  = Horiz. flip selected sects",
    " Y  = Vert. flip selected sects",
    " F5 = Item count",
    " F6 = Actor count/SE help",
    " F7 = Edit sector",
    " F8 = Edit wall/sprite",
    " F9 = Sector tag help",
    " Ctrl-S = Quick save",
    " Alt-F7 = Search sector lotag",
    " Alt-F8 = Search wall/sprite tags",
    " [      = Search forward",
    " ]      = Search backward",
};

static char *SpriteMode[]=
{
    "NONE",
    "SECTORS",
    "WALLS",
    "SPRITES",
    "ALL",
    "ITEMS ONLY",
    "CURRENT SPRITE ONLY",
    "ONLY SECTOREFFECTORS AND SECTORS",
    "NO SECTOREFFECTORS OR SECTORS"
};

#define MAXSKILL 5
static char *SKILLMODE[MAXSKILL]=
{
    "Actor skill display: PIECE OF CAKE",
    "Actor skill display: LET'S ROCK",
    "Actor skill display: COME GET SOME",
    "Actor skill display: DAMN I'M GOOD",
    "Actor skill display: ALL SKILL LEVELS"
};

#define MAXNOSPRITES 4
static char *SPRDSPMODE[MAXNOSPRITES]=
{
    "Sprite display: DISPLAY ALL SPRITES",
    "Sprite display: NO EFFECTORS",
    "Sprite display: NO ACTORS",
    "Sprite display: NO EFFECTORS OR ACTORS"
};

#define MAXHELP3D (signed int)(sizeof(Help3d)/sizeof(Help3d[0]))
static char *Help3d[]=
{
    "Mapster32 3D mode help",
    " ",
    " F1 = TOGGLE THIS HELP DISPLAY",
    " F2 = TOGGLE CLIPBOARD",
    " F3 = MOUSELOOK",
    " F6 = AUTOMATIC SECTOREFFECTOR HELP",
    " F7 = AUTOMATIC SECTOR TAG HELP",
    "",
    " ' A = TOGGLE AUTOSAVE",
    " ' D = CYCLE SPRITE SKILL DISPLAY",
    " ' R = TOGGLE FRAMERATE DISPLAY",
    " ' W = TOGGLE SPRITE DISPLAY",
    " ' X = MAP SHADE PREVIEW",
    " ' Y = TOGGLE PURPLE BACKGROUND",
    "",
    " ' T = CHANGE LOTAG",
    " ' H = CHANGE HITAG",
    " ' S = CHANGE SHADE",
    " ' M = CHANGE EXTRA",
    " ' V = CHANGE VISIBILITY",
    " ' L = CHANGE OBJECT COORDINATES",
    " ' C = CHANGE GLOBAL SHADE",
    "",
    " ' ENTER = PASTE GRAPHIC ONLY",
    " ' P & ; P = PASTE PALETTE TO ALL SELECTED SECTORS",
    " ; V = SET VISIBILITY ON ALL SELECTED SECTORS",
    " ' DEL = CSTAT=0",
    " CTRL-S = SAVE BOARD",
    " HOME = PGUP/PGDN MODIFIER (256 UNITS)",
    " END = PGUP/PGDN MODIFIER (512 UNITS)",
};
static char *type2str[]={"Wall","Sector","Sector","Sprite","Wall"};

static CACHE1D_FIND_REC *finddirs=NULL, *findfiles=NULL, *finddirshigh=NULL, *findfileshigh=NULL;
static int numdirs=0, numfiles=0;
static int currentlist=0;
static int mouseaction=0, mouseax=0, mouseay=0;
static int repeatcountx, repeatcounty;
static int infobox=3; // bit0: current window, bit1: mouse pointer, the variable should be renamed

extern char mskip;
extern short capturecount;
extern int editorgridextent;	// in engine.c
extern char game_executable[BMAX_PATH];

extern int fillsector(short sectnum, char fillcolor);

static void clearfilenames(void)
{
    klistfree(finddirs);
    klistfree(findfiles);
    finddirs = findfiles = NULL;
    numfiles = numdirs = 0;
}

static int getfilenames(const char *path, char kind[])
{
    CACHE1D_FIND_REC *r;

    clearfilenames();
    finddirs = klistpath(path,"*",CACHE1D_FIND_DIR);
    findfiles = klistpath(path,kind,CACHE1D_FIND_FILE);
    for (r = finddirs; r; r=r->next) numdirs++;
    for (r = findfiles; r; r=r->next) numfiles++;

    finddirshigh = finddirs;
    findfileshigh = findfiles;
    currentlist = 0;
    if (findfileshigh) currentlist = 1;

    return(0);
}

void ExtLoadMap(const char *mapname)
{
    int i;
    int sky=0;
    int j;

    getmessageleng = 0;
    getmessagetimeoff = 0;

    // PreCache Wall Tiles
    /*
        for(j=0;j<numwalls;j++)
            if(waloff[wall[j].picnum] == 0)
            {
                loadtile(wall[j].picnum);
                if (bpp != 8)
                    polymost_precache(wall[j].picnum,wall[j].pal,0);
            }

        for(j=0;j<numsectors;j++)
            if(waloff[sector[j].floorpicnum] == 0 || waloff[sector[j].ceilingpicnum] == 0)
            {
                loadtile(sector[j].floorpicnum);
                loadtile(sector[j].ceilingpicnum);
                if (bpp != 8)
                {
                    polymost_precache(sector[j].floorpicnum,sector[j].floorpal,0);
                    polymost_precache(sector[j].floorpicnum,sector[j].floorpal,0);
                }
            }

        for(j=0;j<numsprites;j++)
            if(waloff[sprite[j].picnum] == 0)
            {
                loadtile(sprite[j].picnum);
                if (bpp != 8)
                    polymost_precache(sprite[j].picnum,sprite[j].pal,1);
            }
    */
    // Presize Sprites
    for (j=numsprites;j>=0;j--)
    {
        /*        if (tilesizx[sprite[j].picnum]==0 || tilesizy[sprite[j].picnum]==0)
                    sprite[j].picnum=0; */

        if (sprite[j].picnum>=20 && sprite[j].picnum<=59)
        {
            if (sprite[j].picnum==26)
            {
                sprite[j].xrepeat = 8;
                sprite[j].yrepeat = 8;
            }
            else
            {
                sprite[j].xrepeat = 32;
                sprite[j].yrepeat = 32;
            }
        }

    }

    Bstrcpy(levelname,mapname);
    pskyoff[0]=0;
    for (i=0;i<8;i++) pskyoff[i]=0;

    for (i=0;i<numsectors;i++)
    {
        switch (sector[i].ceilingpicnum)
        {
        case MOONSKY1 :
        case BIGORBIT1 : // orbit
        case LA : // la city
            sky=sector[i].ceilingpicnum;
            break;
        }
    }

    switch (sky)
    {
    case MOONSKY1 :
        //        earth          mountian   mountain         sun
        pskyoff[6]=1;
        pskyoff[1]=2;
        pskyoff[4]=2;
        pskyoff[2]=3;
        break;

    case BIGORBIT1 : // orbit
        //       earth1         2           3           moon/sun
        pskyoff[5]=1;
        pskyoff[6]=2;
        pskyoff[7]=3;
        pskyoff[2]=4;
        break;

    case LA : // la city
        //       earth1         2           3           moon/sun
        pskyoff[0]=1;
        pskyoff[1]=2;
        pskyoff[2]=1;
        pskyoff[3]=3;
        pskyoff[4]=4;
        pskyoff[5]=0;
        pskyoff[6]=2;
        pskyoff[7]=3;
        break;
    }

    pskybits=3;
    parallaxtype=0;
    Bsprintf(tempbuf, "Mapster32 - %s",mapname);
    wm_setapptitle(tempbuf);
}

void ExtSaveMap(const char *mapname)
{
    UNREFERENCED_PARAMETER(mapname);
    saveboard("backup.map",&posx,&posy,&posz,&ang,&cursectnum);
}

int getTileGroup(const char *groupName)
{
    int temp;
    for (temp = 0; temp < MAX_TILE_GROUPS; temp++)
    {
        if (s_TileGroups[temp].szText == NULL)
        {
            return -1;
        }
        if (!strcmp(s_TileGroups[temp].szText, groupName))
        {
            return temp;
        }
    }
    return -1;
}

int tileInGroup(int group, int tilenum)
{
    // @todo Make a bitmap instead of doing this slow search..
    int temp;
    if (group < 0 || group >= MAX_TILE_GROUPS || s_TileGroups[group].szText == NULL)
    {
        // group isn't valid.
        return 0;
    }
    for (temp = 0; temp < s_TileGroups[group].nIds; temp++)
    {
        if (tilenum == s_TileGroups[group].pIds[temp])
        {
            return 1;
        }
    }
    return 0;
}

const char *ExtGetSectorType(int lotag)
{
    static char tempbuf[64];
    Bmemset(tempbuf,0,sizeof(tempbuf));
    switch (lotag)
    {
    case 1:
        Bsprintf(tempbuf,"WATER (SE 7)");
        break;
    case 2:
        Bsprintf(tempbuf,"UNDERWATER (SE 7)");
        break;
    case 9:
        Bsprintf(tempbuf,"STAR TREK DOORS");
        break;
    case 15:
        Bsprintf(tempbuf,"ELEVATOR TRANSPORT (SE 17)");
        break;
    case 16:
        Bsprintf(tempbuf,"ELEVATOR PLATFORM DOWN");
        break;
    case 17:
        Bsprintf(tempbuf,"ELEVATOR PLATFORM UP");
        break;
    case 18:
        Bsprintf(tempbuf,"ELEVATOR DOWN");
        break;
    case 19:
        Bsprintf(tempbuf,"ELEVATOR UP");
        break;
    case 20:
        Bsprintf(tempbuf,"CEILING DOOR");
        break;
    case 21:
        Bsprintf(tempbuf,"FLOOR DOOR");
        break;
    case 22:
        Bsprintf(tempbuf,"SPLIT DOOR");
        break;
    case 23:
        Bsprintf(tempbuf,"SWING DOOR (SE 11)");
        break;
    case 25:
        Bsprintf(tempbuf,"SLIDE DOOR (SE 15)");
        break;
    case 26:
        Bsprintf(tempbuf,"SPLIT STAR TREK DOOR");
        break;
    case 27:
        Bsprintf(tempbuf,"BRIDGE (SE 20)");
        break;
    case 28:
        Bsprintf(tempbuf,"DROP FLOOR (SE 21)");
        break;
    case 29:
        Bsprintf(tempbuf,"TEETH DOOR (SE 22)");
        break;
    case 30:
        Bsprintf(tempbuf,"ROTATE RISE BRIDGE");
        break;
    case 31:
        Bsprintf(tempbuf,"2 WAY TRAIN (SE=30)");
        break;
    case 32767:
        Bsprintf(tempbuf,"SECRET ROOM");
        break;
    case -1:
        Bsprintf(tempbuf,"END OF LEVEL");
        break;
    default :
        if (lotag > 10000 && lotag < 32767)
            Bsprintf(tempbuf,"1 TIME SOUND");
//        else  Bsprintf(tempbuf,"%hu",lotag);
        break;
    }
    return(tempbuf);
}

const char *ExtGetSectorCaption(short sectnum)
{
    static char tempbuf[64];
    Bmemset(tempbuf,0,sizeof(tempbuf));
    if (qsetmode != 200 && (!(onnames==1 || onnames==4 || onnames==7) || (onnames==8)))
    {
        tempbuf[0] = 0;
        return(tempbuf);
    }

    if (qsetmode != 200 && (sector[sectnum].lotag|sector[sectnum].hitag) == 0)
    {
        tempbuf[0] = 0;
    }
    else
    {
        Bstrcpy(lo,ExtGetSectorType(sector[sectnum].lotag));
        if (qsetmode != 200)
            Bsprintf(tempbuf,"%hu,%hu %s",sector[sectnum].hitag,sector[sectnum].lotag,lo);
        else Bsprintf(tempbuf,"%hu %s",sector[sectnum].lotag,lo);
    }
    return(tempbuf);
}

const char *ExtGetWallCaption(short wallnum)
{
    static char tempbuf[64];

    Bmemset(tempbuf,0,sizeof(tempbuf));

    if (wall[wallnum].cstat & (1<<14))
    {
        int dax = wall[wallnum].x-wall[wall[wallnum].point2].x;
        int day = wall[wallnum].y-wall[wall[wallnum].point2].y;
        int dist = ksqrt(dax*dax+day*day);
        Bsprintf(tempbuf,"%d",dist);
        wall[wallnum].cstat &= ~(1<<14);
        return(tempbuf);
    }


    if (!(onnames==2 || onnames==4))
    {
        tempbuf[0] = 0;
        return(tempbuf);
    }

    // HERE

    if ((wall[wallnum].lotag|wall[wallnum].hitag) == 0)
    {
        tempbuf[0] = 0;
    }
    else
    {
        Bsprintf(tempbuf,"%hu,%hu",wall[wallnum].hitag,wall[wallnum].lotag);
    }
    return(tempbuf);
} //end

const char *SectorEffectorTagText(int lotag)
{
    static char tempbuf[64];
    Bmemset(tempbuf,0,sizeof(tempbuf));
    switch (lotag)
    {
    case 0:
        Bsprintf(tempbuf,"%d: ROTATED SECTOR",lotag);
        break;
    case 1:
        Bsprintf(tempbuf,"%d: PIVOT SPRITE FOR SE 0",lotag);
        break;
    case 2:
        Bsprintf(tempbuf,"%d: EARTHQUAKE",lotag);
        break;
    case 3:
        Bsprintf(tempbuf,"%d: RANDOM LIGHTS AFTER SHOT OUT",lotag);
        break;
    case 4:
        Bsprintf(tempbuf,"%d: RANDOM LIGHTS",lotag);
        break;
    case 6:
        Bsprintf(tempbuf,"%d: SUBWAY",lotag);
        break;
    case 7:
        Bsprintf(tempbuf,"%d: TRANSPORT",lotag);
        break;
    case 8:
        Bsprintf(tempbuf,"%d: UP OPEN DOOR LIGHTS",lotag);
        break;
    case 9:
        Bsprintf(tempbuf,"%d: DOWN OPEN DOOR LIGHTS",lotag);
        break;
    case 10:
        Bsprintf(tempbuf,"%d: DOOR AUTO CLOSE (H=DELAY)",lotag);
        break;
    case 11:
        Bsprintf(tempbuf,"%d: ROTATE SECTOR DOOR",lotag);
        break;
    case 12:
        Bsprintf(tempbuf,"%d: LIGHT SWITCH",lotag);
        break;
    case 13:
        Bsprintf(tempbuf,"%d: EXPLOSIVE",lotag);
        break;
    case 14:
        Bsprintf(tempbuf,"%d: SUBWAY CAR",lotag);
        break;
    case 15:
        Bsprintf(tempbuf,"%d: SLIDE DOOR (ST 25)",lotag);
        break;
    case 16:
        Bsprintf(tempbuf,"%d: ROTATE REACTOR SECTOR",lotag);
        break;
    case 17:
        Bsprintf(tempbuf,"%d: ELEVATOR TRANSPORT (ST 15)",lotag);
        break;
    case 18:
        Bsprintf(tempbuf,"%d: INCREMENTAL SECTOR RISE/FALL",lotag);
        break;
    case 19:
        Bsprintf(tempbuf,"%d: CEILING FALL ON EXPLOSION",lotag);
        break;
    case 20:
        Bsprintf(tempbuf,"%d: BRIDGE (ST 27)",lotag);
        break;
    case 21:
        Bsprintf(tempbuf,"%d: DROP FLOOR (ST 28)",lotag);
        break;
    case 22:
        Bsprintf(tempbuf,"%d: TEETH DOOR (ST 29)",lotag);
        break;
    case 23:
        Bsprintf(tempbuf,"%d: 1-WAY SE7 DESTINATION (H=SE 7)",lotag);
        break;
    case 24:
        Bsprintf(tempbuf,"%d: CONVAYER BELT",lotag);
        break;
    case 25:
        Bsprintf(tempbuf,"%d: ENGINE",lotag);
        break;
    case 28:
        Bsprintf(tempbuf,"%d: LIGHTNING (H= TILE#4890)",lotag);
        break;
    case 27:
        Bsprintf(tempbuf,"%d: CAMERA FOR PLAYBACK",lotag);
        break;
    case 29:
        Bsprintf(tempbuf,"%d: FLOAT",lotag);
        break;
    case 30:
        Bsprintf(tempbuf,"%d: 2 WAY TRAIN (ST=31)",lotag);
        break;
    case 31:
        Bsprintf(tempbuf,"%d: FLOOR RISE",lotag);
        break;
    case 32:
        Bsprintf(tempbuf,"%d: CEILING FALL",lotag);
        break;
    case 33:
        Bsprintf(tempbuf,"%d: SPAWN JIB W/QUAKE",lotag);
        break;
    case 36:
        Bsprintf(tempbuf,"%d: SKRINK RAY SHOOTER",lotag);
        break;
    default:
        Bsprintf(tempbuf,"%d: (UNKNOWN)",lotag);
        break;
    }
    return (tempbuf);
}

const char *SectorEffectorText(int spritenum)
{
    static char tempbuf[64];
    Bmemset(tempbuf,0,sizeof(tempbuf));
    Bmemset(lo,0,sizeof(lo));
    Bstrcpy(lo,SectorEffectorTagText(sprite[spritenum].lotag));
    if (!lo[5]) // tags are 5 chars or less
        SpriteName(spritenum,tempbuf);
    else Bsprintf(tempbuf,"SE %s",lo);
    return (tempbuf);
}

const char *ExtGetSpriteCaption(short spritenum)
{
    static char tempbuf[1024];

    Bmemset(tempbuf,0,sizeof(tempbuf));
    if ((onnames!=5 && onnames!=6 &&(!(onnames==3 || onnames==4 || onnames==7 || onnames==8))) || (onnames==7 && sprite[spritenum].picnum!=1))
    {
        tempbuf[0] = 0;
        return(tempbuf);
    }

    if (onnames==5)
    {
        if (!tileInGroup(tilegroupItems, sprite[spritenum].picnum))
        {
            tempbuf[0] = 0;
            return(tempbuf);
        }
    }

    if (onnames==6 && sprite[spritenum].picnum != sprite[cursprite].picnum)
    {
        tempbuf[0] = 0;
        return(tempbuf);
    }

    tempbuf[0] = 0;
    if ((sprite[spritenum].lotag|sprite[spritenum].hitag) == 0)
    {
        SpriteName(spritenum,lo);
        if (lo[0]!=0)
        {
            if (sprite[spritenum].pal==1) Bsprintf(tempbuf,"%s (MULTIPLAYER)",lo);
            else Bsprintf(tempbuf,"%s",lo);
        }
    }
    else
        if (sprite[spritenum].picnum==SECTOREFFECTOR)
        {
            if (onnames==8)
                tempbuf[0] = 0;
            else
            {
                Bsprintf(lo,"%s",SectorEffectorText(spritenum));
                Bsprintf(tempbuf,"%s, %hu",lo,sprite[spritenum].hitag);
            }
        }
        else
        {
            SpriteName(spritenum,lo);
            if (sprite[spritenum].extra != -1)
                Bsprintf(tempbuf,"%hu,%hu,%d %s",sprite[spritenum].hitag,sprite[spritenum].lotag,sprite[spritenum].extra,lo);
            else
                Bsprintf(tempbuf,"%hu,%hu %s",sprite[spritenum].hitag,sprite[spritenum].lotag,lo);
        }
    return(tempbuf);

} //end

//printext16 parameters:
//printext16(int xpos, int ypos, short col, short backcol,
//           char name[82], char fontsize)
//  xpos 0-639   (top left)
//  ypos 0-479   (top left)
//  col 0-15
//  backcol 0-15, -1 is transparent background
//  name
//  fontsize 0=8*8, 1=3*5

//drawline16 parameters:
// drawline16(int x1, int y1, int x2, int y2, char col)
//  x1, x2  0-639
//  y1, y2  0-143  (status bar is 144 high, origin is top-left of STATUS BAR)
//  col     0-15

void ExtShowSectorData(short sectnum)   //F5
{
    short statnum=0;
    int x,x2,y;
    int i;
    int secrets=0;
    int totalactors1=0,totalactors2=0,totalactors3=0,totalactors4=0;
    int totalrespawn=0;

    UNREFERENCED_PARAMETER(sectnum);
    if (qsetmode==200)
        return;

    for (i=0;i<numsectors;i++)
    {
        if (sector[i].lotag==32767) secrets++;
    }

    statnum=0;
    i = headspritestat[statnum];
    while (i != -1)
    {
        // Count all non-player actors.
        if (tileInGroup(tilegroupActors, sprite[i].picnum))
        {
            if (sprite[i].lotag<=1) totalactors1++;
            if (sprite[i].lotag<=2) totalactors2++;
            if (sprite[i].lotag<=3) totalactors3++;
            if (sprite[i].lotag<=4) totalactors4++;
        }
        if (sprite[i].picnum == RESPAWN) totalrespawn++;

        i = nextspritestat[i];
    }

    for (i=0;i<MAXSPRITES;i++) numsprite[i]=0;
    for (i=0;i<MAXSPRITES;i++) multisprite[i]=0;
    for (i=0;i<MAXSPRITES;i++)
    {
        if (sprite[i].statnum==0)
        {
            if (sprite[i].pal!=0) multisprite[sprite[i].picnum]++;
            else numsprite[sprite[i].picnum]++;
        }
    }

    clearmidstatbar16();             //Clear middle of status bar
    Bsprintf(tempbuf,"Level %s",levelname);
    printmessage16(tempbuf);

    x=1;
    x2=14;
    y=4;
    begindrawing();
    printext16(x*8,ydim16+y*8,11,-1,"Item Count",0);
    enddrawing();
    PrintStatus("10%health=",numsprite[COLA],x,y+2,11);
    PrintStatus("",multisprite[COLA],x2,y+2,9);
    PrintStatus("30%health=",numsprite[SIXPAK],x,y+3,11);
    PrintStatus("",multisprite[SIXPAK],x2,y+3,9);
    PrintStatus("Med-Kit  =",numsprite[FIRSTAID],x,y+4,11);
    PrintStatus("",multisprite[FIRSTAID],x2,y+4,9);
    PrintStatus("Atom     =",numsprite[ATOMICHEALTH],x,y+5,11);
    PrintStatus("",multisprite[ATOMICHEALTH],x2,y+5,9);
    PrintStatus("Shields  =",numsprite[SHIELD],x,y+6,11);
    PrintStatus("",multisprite[SHIELD],x2,y+6,9);

    x=17;
    x2=30;
    y=4;
    begindrawing();
    printext16(x*8,ydim16+y*8,11,-1,"Inventory",0);
    enddrawing();
    PrintStatus("Steroids =",numsprite[STEROIDS],x,y+2,11);
    PrintStatus("",multisprite[STEROIDS],x2,y+2,9);
    PrintStatus("Airtank  =",numsprite[AIRTANK],x,y+3,11);
    PrintStatus("",multisprite[AIRTANK],x2,y+3,9);
    PrintStatus("Jetpack  =",numsprite[JETPACK],x,y+4,11);
    PrintStatus("",multisprite[JETPACK],x2,y+4,9);
    PrintStatus("Goggles  =",numsprite[HEATSENSOR],x,y+5,11);
    PrintStatus("",multisprite[HEATSENSOR],x2,y+5,9);
    PrintStatus("Boots    =",numsprite[BOOTS],x,y+6,11);
    PrintStatus("",multisprite[BOOTS],x2,y+6,9);
    PrintStatus("HoloDuke =",numsprite[HOLODUKE],x,y+7,11);
    PrintStatus("",multisprite[HOLODUKE],x2,y+7,9);
    PrintStatus("Multi D  =",numsprite[APLAYER],x,y+8,11);

    x=33;
    x2=46;
    y=4;
    begindrawing();
    printext16(x*8,ydim16+y*8,11,-1,"Weapon Count",0);
    enddrawing();
    PrintStatus("Pistol   =",numsprite[FIRSTGUNSPRITE],x,y+2,11);
    PrintStatus("",multisprite[FIRSTGUNSPRITE],x2,y+2,9);
    PrintStatus("Shotgun  =",numsprite[SHOTGUNSPRITE],x,y+3,11);
    PrintStatus("",multisprite[SHOTGUNSPRITE],x2,y+3,9);
    PrintStatus("Chaingun =",numsprite[CHAINGUNSPRITE],x,y+4,11);
    PrintStatus("",multisprite[CHAINGUNSPRITE],x2,y+4,9);
    PrintStatus("RPG      =",numsprite[RPGSPRITE],x,y+5,11);
    PrintStatus("",multisprite[RPGSPRITE],x2,y+5,9);
    PrintStatus("Pipe Bomb=",numsprite[HEAVYHBOMB],x,y+6,11);
    PrintStatus("",multisprite[HEAVYHBOMB],x2,y+6,9);
    PrintStatus("Shrinker =",numsprite[SHRINKERSPRITE],x,y+7,11);
    PrintStatus("",multisprite[SHRINKERSPRITE],x2,y+7,9);
    PrintStatus("Devastatr=",numsprite[DEVISTATORSPRITE],x,y+8,11);
    PrintStatus("",multisprite[DEVISTATORSPRITE],x2,y+8,9);
    PrintStatus("Trip mine=",numsprite[TRIPBOMBSPRITE],x,y+9,11);
    PrintStatus("",multisprite[TRIPBOMBSPRITE],x2,y+9,9);
    PrintStatus("Freezeray=",numsprite[FREEZESPRITE],x,y+10,11);
    PrintStatus("",multisprite[FREEZESPRITE],x2,y+10,9);

    x=49;
    x2=62;
    y=4;
    begindrawing();
    printext16(x*8,ydim16+y*8,11,-1,"Ammo Count",0);
    enddrawing();
    PrintStatus("Pistol   =",numsprite[AMMO],x,y+2,11);
    PrintStatus("",multisprite[AMMO],x2,y+2,9);
    PrintStatus("Shot     =",numsprite[SHOTGUNAMMO],x,y+3,11);
    PrintStatus("",multisprite[SHOTGUNAMMO],x2,y+3,9);
    PrintStatus("Chain    =",numsprite[BATTERYAMMO],x,y+4,11);
    PrintStatus("",multisprite[BATTERYAMMO],x2,y+4,9);
    PrintStatus("RPG Box  =",numsprite[RPGAMMO],x,y+5,11);
    PrintStatus("",multisprite[RPGAMMO],x2,y+5,9);
    PrintStatus("Pipe Bomb=",numsprite[HBOMBAMMO],x,y+6,11);
    PrintStatus("",multisprite[HBOMBAMMO],x2,y+6,9);
    PrintStatus("Shrinker =",numsprite[CRYSTALAMMO],x,y+7,11);
    PrintStatus("",multisprite[CRYSTALAMMO],x2,y+7,9);
    PrintStatus("Devastatr=",numsprite[DEVISTATORAMMO],x,y+8,11);
    PrintStatus("",multisprite[DEVISTATORAMMO],x2,y+8,9);
    PrintStatus("Expander =",numsprite[GROWAMMO],x,y+9,11);
    PrintStatus("",multisprite[GROWAMMO],x2,y+9,9);
    PrintStatus("Freezeray=",numsprite[FREEZEAMMO],x,y+10,11);
    PrintStatus("",multisprite[FREEZEAMMO],x2,y+10,9);

    begindrawing();
    printext16(65*8,ydim16+4*8,11,-1,"MISC",0);
    enddrawing();
    PrintStatus("Secrets =",secrets,65,6,11);
    begindrawing();
    printext16(65*8,ydim16+8*8,11,-1,"ACTORS",0);
    enddrawing();
    PrintStatus("Skill 1 =",totalactors1,65,10,11);
    PrintStatus("Skill 2 =",totalactors2,65,11,11);
    PrintStatus("Skill 3 =",totalactors3,65,12,11);
    PrintStatus("Skill 4 =",totalactors4,65,13,11);
    PrintStatus("Respawn =",totalrespawn,65,14,11);

}// end ExtShowSectorData

void ExtShowWallData(short wallnum)       //F6
{
    int i,nextfreetag=0,total=0;
    char x,y;

    UNREFERENCED_PARAMETER(wallnum);

    if (qsetmode==200)
        return;

    for (i=0;i<MAXSPRITES;i++)
    {
        if (sprite[i].statnum==0)
            switch (sprite[i].picnum)
            {
                //LOTAG
            case ACTIVATOR:
            case ACTIVATORLOCKED:
            case TOUCHPLATE:
            case MASTERSWITCH:
            case RESPAWN:
            case ACCESSSWITCH:
            case SLOTDOOR:
            case LIGHTSWITCH:
            case SPACEDOORSWITCH:
            case SPACELIGHTSWITCH:
            case FRANKENSTINESWITCH:
            case MULTISWITCH:
            case DIPSWITCH:
            case DIPSWITCH2:
            case TECHSWITCH:
            case DIPSWITCH3:
            case ACCESSSWITCH2:
            case POWERSWITCH1:
            case LOCKSWITCH1:
            case POWERSWITCH2:
            case PULLSWITCH:
            case ALIENSWITCH:
                if (sprite[i].lotag>nextfreetag) nextfreetag=1+sprite[i].lotag;
                break;

                //HITAG
            case SEENINE:
            case OOZFILTER:
            case SECTOREFFECTOR:
                if (sprite[i].lotag==10 || sprite[i].lotag==27 || sprite[i].lotag==28 || sprite[i].lotag==29)
                    break;
                else
                    if (sprite[i].hitag>nextfreetag) nextfreetag=1+sprite[i].hitag;
                break;
            default:
                break;

            }

    } // end sprite loop

    //Count Normal Actors
    for (i=0;i<MAXSPRITES;i++) numsprite[i]=0;
    for (i=0;i<MAXSPRITES;i++) multisprite[i]=0;
    for (i=0;i<MAXSPRITES;i++)
    {
        if (sprite[i].statnum==0)
        {
            if (sprite[i].pal!=0)
                switch (sprite[i].picnum)
                {
                case LIZTROOP :
                case LIZTROOPRUNNING :
                case LIZTROOPSTAYPUT :
                case LIZTROOPSHOOT :
                case LIZTROOPJETPACK :
                case LIZTROOPONTOILET :
                case LIZTROOPDUCKING :
                    numsprite[LIZTROOP]++;
                    break;
                case BOSS1:
                case BOSS1STAYPUT:
                case BOSS1SHOOT:
                case BOSS1LOB:
                case BOSSTOP:
                    multisprite[BOSS1]++;
                    break;
                case BOSS2:
                    multisprite[BOSS2]++;
                    break;
                case BOSS3:
                    multisprite[BOSS3]++;
                    break;

                default:
                    break;
                }
            else
                switch (sprite[i].picnum)
                {
                case LIZTROOP :
                case LIZTROOPRUNNING :
                case LIZTROOPSTAYPUT :
                case LIZTROOPSHOOT :
                case LIZTROOPJETPACK :
                case LIZTROOPONTOILET :
                case LIZTROOPDUCKING :
                    numsprite[LIZTROOP]++;
                    break;
                case PIGCOP:
                case PIGCOPSTAYPUT:
                case PIGCOPDIVE:
                    numsprite[PIGCOP]++;
                    break;
                case LIZMAN:
                case LIZMANSTAYPUT:
                case LIZMANSPITTING:
                case LIZMANFEEDING:
                case LIZMANJUMP:
                    numsprite[LIZMAN]++;
                    break;
                case BOSS1:
                case BOSS1STAYPUT:
                case BOSS1SHOOT:
                case BOSS1LOB:
                case BOSSTOP:
                    numsprite[BOSS1]++;
                    break;
                case COMMANDER:
                case COMMANDERSTAYPUT:
                    numsprite[COMMANDER]++;
                    break;
                case OCTABRAIN:
                case OCTABRAINSTAYPUT:
                    numsprite[OCTABRAIN]++;
                    break;
                case RECON:
                case DRONE:
                case ROTATEGUN:
                case EGG:
                case ORGANTIC:
                case GREENSLIME:
                case BOSS2:
                case BOSS3:
                case TANK:
                case NEWBEAST:
                case BOSS4:

                    numsprite[sprite[i].picnum]++;
                default:
                    break;

                }// end switch
        }// end if
    }//end for
    total=0;
    for (i=0;i<MAXSPRITES;i++) if (numsprite[i]!=0) total+=numsprite[i];
    for (i=0;i<MAXSPRITES;i++) if (multisprite[i]!=0) total+=multisprite[i];

    clearmidstatbar16();

    Bsprintf(tempbuf,"Level %s next tag %d",levelname,nextfreetag);
    printmessage16(tempbuf);

    x=2;
    y=4;
    PrintStatus("Normal Actors =",total,x,y,11);

    PrintStatus(" Liztroop  =",numsprite[LIZTROOP],x,y+1,11);
    PrintStatus(" Lizman    =",numsprite[LIZMAN],x,y+2,11);
    PrintStatus(" Commander =",numsprite[COMMANDER],x,y+3,11);
    PrintStatus(" Octabrain =",numsprite[OCTABRAIN],x,y+4,11);
    PrintStatus(" PigCop    =",numsprite[PIGCOP],x,y+5,11);
    PrintStatus(" Recon Car =",numsprite[RECON],x,y+6,11);
    PrintStatus(" Drone     =",numsprite[DRONE],x,y+7,11);
    PrintStatus(" Turret    =",numsprite[ROTATEGUN],x,y+8,11);
    PrintStatus(" Egg       =",numsprite[EGG],x,y+9,11);
    x+=17;
    PrintStatus("Slimer    =",numsprite[GREENSLIME],x,y+1,11);
    PrintStatus("Boss1     =",numsprite[BOSS1],x,y+2,11);
    PrintStatus("MiniBoss1 =",multisprite[BOSS1],x,y+3,11);
    PrintStatus("Boss2     =",numsprite[BOSS2],x,y+4,11);
    PrintStatus("Boss3     =",numsprite[BOSS3],x,y+5,11);
    PrintStatus("Riot Tank =",numsprite[TANK],x,y+6,11);
    PrintStatus("Newbeast  =",numsprite[NEWBEAST],x,y+7,11);
    PrintStatus("Boss4     =",numsprite[BOSS4],x,y+8,11);

    //Count Respawn Actors
    for (i=0;i<MAXSPRITES;i++) numsprite[i]=0;
    for (i=0;i<MAXSPRITES;i++) multisprite[i]=0;
    for (i=0;i<MAXSPRITES;i++)
    {
        if (sprite[i].statnum==0 && sprite[i].picnum==RESPAWN)
        {
            switch (sprite[i].hitag)
            {
            case LIZTROOP :
            case LIZTROOPRUNNING :
            case LIZTROOPSTAYPUT :
            case LIZTROOPSHOOT :
            case LIZTROOPJETPACK :
            case LIZTROOPONTOILET :
            case LIZTROOPDUCKING :
                numsprite[LIZTROOP]++;
                break;
            case PIGCOP:
            case PIGCOPSTAYPUT:
            case PIGCOPDIVE:
                numsprite[PIGCOP]++;
                break;
            case LIZMAN:
            case LIZMANSTAYPUT:
            case LIZMANSPITTING:
            case LIZMANFEEDING:
            case LIZMANJUMP:
                numsprite[LIZMAN]++;
                break;
            case BOSS1:
            case BOSS1STAYPUT:
            case BOSS1SHOOT:
            case BOSS1LOB:
            case BOSSTOP:
                if (sprite[i].pal!=0) multisprite[BOSS1]++;
                else numsprite[BOSS1]++;
                break;
            case COMMANDER:
            case COMMANDERSTAYPUT:
                numsprite[COMMANDER]++;
                break;
            case OCTABRAIN:
            case OCTABRAINSTAYPUT:
                numsprite[OCTABRAIN]++;
                break;
            case RECON:
            case DRONE:
            case ROTATEGUN:
            case EGG:
            case ORGANTIC:
            case GREENSLIME:
            case BOSS2:
            case BOSS3:
            case TANK:
            case NEWBEAST:
            case BOSS4:
                numsprite[sprite[i].hitag]++;
            default:
                break;
            }//end switch
        }// end if
    }// end for
    total=0;
    for (i=0;i<MAXSPRITES;i++) if (numsprite[i]!=0) total+=numsprite[i];
    for (i=0;i<MAXSPRITES;i++) if (multisprite[i]!=0) total+=multisprite[i];


    x=36;
    y=4;
    PrintStatus("Respawn",total,x,y,11);

    PrintStatus(" Liztroop  =",numsprite[LIZTROOP],x,y+1,11);
    PrintStatus(" Lizman    =",numsprite[LIZMAN],x,y+2,11);
    PrintStatus(" Commander =",numsprite[COMMANDER],x,y+3,11);
    PrintStatus(" Octabrain =",numsprite[OCTABRAIN],x,y+4,11);
    PrintStatus(" PigCop    =",numsprite[PIGCOP],x,y+5,11);
    PrintStatus(" Recon Car =",numsprite[RECON],x,y+6,11);
    PrintStatus(" Drone     =",numsprite[DRONE],x,y+7,11);
    PrintStatus(" Turret    =",numsprite[ROTATEGUN],x,y+8,11);
    PrintStatus(" Egg       =",numsprite[EGG],x,y+9,11);
    x+=17;
    PrintStatus("Slimer    =",numsprite[GREENSLIME],x,y+1,11);
    PrintStatus("Boss1     =",numsprite[BOSS1],x,y+2,11);
    PrintStatus("MiniBoss1 =",multisprite[BOSS1],x,y+3,11);
    PrintStatus("Boss2     =",numsprite[BOSS2],x,y+4,11);
    PrintStatus("Boss3     =",numsprite[BOSS3],x,y+5,11);
    PrintStatus("Riot Tank =",numsprite[TANK],x,y+6,11);
    PrintStatus("Newbeast  =",numsprite[NEWBEAST],x,y+7,11);
    PrintStatus("Boss4     =",numsprite[BOSS4],x,y+8,11);
}// end ExtShowWallData

static void Show2dText(char *name)
{
    int fp,t;
    unsigned char x=0,y=4,xmax=0,xx=0,col=0;
    clearmidstatbar16();
    if ((fp=kopen4load(name,0)) == -1)
    {
        begindrawing();
        printext16(1*4,ydim16+4*8,11,-1,"ERROR: file not found.",0);
        enddrawing();
        return;
    }

    t=65;
    begindrawing();
    while (t!=EOF && col<5)
    {
        t = 0;
        if (kread(fp,&t,1)<=0)
            t = EOF;
        while (t!=EOF && t!='\n' && x<250)
        {
            tempbuf[x]=t;
            t = 0;
            if (kread(fp,&t,1)<=0) t = EOF;
            x++;
            if (x>xmax) xmax=x;
        }
        tempbuf[x]=0;
        printext16(xx*4,ydim16+(y*6)+2,11,-1,tempbuf,1);
        x=0;
        y++;
        if (y>18)
        {
            col++;
            y=6;
            xx+=xmax;
            xmax=0;
        }
    }
    enddrawing();

    kclose(fp);

}// end Show2dText

// PK_ vvvv
typedef struct helppage_
{
    int numlines;
    char line[][80];  // C99 flexible array member
} helppage_t;

helppage_t **helppage=NULL;
int numhelppages=0;

static int emptyline(const char *start)
{
    int i;
    for (i=0; i<80; i++)
    {
        if (start[i]=='\n' || !start[i]) break;
        if (start[i]!=' ' && start[i]!='\t' && start[i]!='\r')
            return 0;
    }
    return 1;
}

static int newpage(const char *start)
{
    int i;
    for (i=80-1; i>=0; i--)
    {
        if (start[i] == '^' && start[i+1] == 'P')
            return 1;
    }
    return 0;
}

#define IHELP_INITPAGES 32
#define IHELP_INITLINES 16

static void ReadHelpFile(const char *name)
{
    BFILE *fp;
    int i, j, k, numallocpages;
    long int pos, charsread=0;
    helppage_t *hp;
    char skip=0;

    initprintf("Loading \"%s\"\n",name);

    if ((fp=fopenfrompath(name,"rb")) == NULL)
    {
        initprintf("Error initializing integrated help: file \"%s\" not found.\n", name);
        return;
    }

    helppage=malloc(IHELP_INITPAGES * sizeof(helppage_t *));
    numallocpages=IHELP_INITPAGES;
    if (!helppage) goto HELPFILE_ERROR;

    i=0;
    while (!Bfeof(fp) && !ferror(fp))
    {
        while (!Bfeof(fp))    // skip empty lines
        {
            pos = ftell(fp);
            if (Bfgets(tempbuf, 80, fp) == NULL) break;
            charsread = ftell(fp)-pos;
            if (!newpage(tempbuf))
            {
                break;
            }
        }

        if (Bfeof(fp) || charsread<=0) break;

        hp=Bcalloc(1,sizeof(helppage_t) + IHELP_INITLINES*80);
        if (!hp) goto HELPFILE_ERROR;
        hp->numlines = IHELP_INITLINES;

        if (charsread == 79 && tempbuf[78]!='\n') skip=1;
        j=0;

        do
        {
            if (j >= hp->numlines)
            {
                hp=realloc(hp, sizeof(helppage_t) + 2*hp->numlines*80);
                if (!hp) goto HELPFILE_ERROR;
                hp->numlines *= 2;
            }

            // limit the line length to 78 chars and probably get rid of the CR
            if (charsread>0)
            {
                tempbuf[charsread-1]=0;
                if (tempbuf[charsread-2]==0x0d) tempbuf[charsread-2]=0;
            }

            memcpy(hp->line[j], tempbuf, 80);

            for (k=charsread; k<80; k++) hp->line[j][k]=0;

            if (skip)
            {
                while (fgetc(fp)!='\n' && !Bfeof(fp)) /*skip rest of line*/;
                skip=0;
            }

            pos = ftell(fp);
            if (Bfgets(tempbuf, 80, fp) == NULL) break;
            charsread = ftell(fp)-pos;
            if (charsread == 79 && tempbuf[78]!='\n') skip=1;

            j++;

        }
        while (!newpage(tempbuf) && !Bfeof(fp) && charsread>0);

        hp=realloc(hp, sizeof(helppage_t) + j*80);
        if (!hp) goto HELPFILE_ERROR;
        hp->numlines=j;

        if (i >= numallocpages)
        {
            helppage = realloc(helppage, 2*numallocpages*sizeof(helppage_t *));
            numallocpages *= 2;
            if (!helppage) goto HELPFILE_ERROR;
        }
        helppage[i] = hp;
        i++;
    }

    helppage = realloc(helppage, i*sizeof(helppage_t *));
    if (!helppage) goto HELPFILE_ERROR;
    numhelppages = i;

    Bfclose(fp);
    return;

HELPFILE_ERROR:

    Bfclose(fp);
    initprintf("ReadHelpFile(): ERROR allocating memory.\n");
    return;
}

// why can't MSVC allocate an array of variable size?!
#define IHELP_NUMDISPLINES 42 // ((overridepm16y>>4)+(overridepm16y>>5)+(overridepm16y>>7)-2)
#define IHELP_PATLEN 45
extern int overridepm16y;  // influences printmessage16() and clearmidstatbar16()

static void IntegratedHelp()
{
    if (!helppage) return;

    overridepm16y = 3*STATUS2DSIZ;

    {
        int i, j;
        static int curhp=0, curline=0;
        int highlighthp=-1, highlightline=-1, lasthighlighttime=0;
        char disptext[IHELP_NUMDISPLINES][80];
        char oldpattern[IHELP_PATLEN+1];

        begindrawing();
        clearbuf((char *)(frameplace + (ydim-overridepm16y)*bytesperline), (bytesperline*(overridepm16y-25)) >> 2, 0L);

        drawline16(0,ydim-overridepm16y,xdim-1,ydim-overridepm16y,1);
        Bsprintf(tempbuf, "Mapster32 Help Mode");
        printext16(9L,ydim2d-overridepm16y+9L,4,-1,tempbuf,0);
        printext16(8L,ydim2d-overridepm16y+8L,12,-1,tempbuf,0);
        enddrawing();

        memset(oldpattern, 0, sizeof(char));
//    clearmidstatbar16();

        while (keystatus[KEYSC_ESC]==0 && keystatus[KEYSC_Q]==0 && keystatus[KEYSC_F1]==0)
        {
            if (handleevents())
            {
                if (quitevent) quitevent = 0;
            }
            idle();
//        printmessage16("Help mode, press <Esc> to exit");

            if (keystatus[KEYSC_T])    // goto table of contents
            {
                keystatus[KEYSC_T]=0;
                curhp=0;
                curline=0;
            }
            else if (keystatus[KEYSC_G])    // goto arbitrary page
            {
                keystatus[KEYSC_G]=0;
                Bsprintf(tempbuf, "Goto page: ");
                curhp=getnumber16(tempbuf, 0, numhelppages-1, 0);
                curline=0;
            }
            else if (keystatus[KEYSC_UP])    // scroll up
            {
                keystatus[KEYSC_UP]=0;
                if (curline>0) curline--;
            }
            else if (keystatus[KEYSC_DOWN])    // scroll down
            {
                keystatus[KEYSC_DOWN]=0;
                if (curline+IHELP_NUMDISPLINES < helppage[curhp]->numlines) curline++;
            }
            else if (keystatus[KEYSC_PGUP])    // scroll one page up
            {
                keystatus[KEYSC_PGUP]=0;
                i=IHELP_NUMDISPLINES;
                while (i>0 && curline>0) i--, curline--;
            }
            else if (keystatus[KEYSC_PGDN])    // scroll one page down
            {
                keystatus[KEYSC_PGDN]=0;
                i=IHELP_NUMDISPLINES;
                while (i>0 && curline+IHELP_NUMDISPLINES < helppage[curhp]->numlines) i--, curline++;
            }
            else if (keystatus[KEYSC_SPACE])   // goto next paragraph
            {
                keystatus[KEYSC_SPACE]=0;
                for (i=curline, j=0; i < helppage[curhp]->numlines; i++)
                {
                    if (emptyline(helppage[curhp]->line[i])) { j=1; continue; }
                    if (j==1 && !emptyline(helppage[curhp]->line[i])) { j=2; break; }
                }
                if (j==2)
                {
                    if (i+IHELP_NUMDISPLINES < helppage[curhp]->numlines)
                        curline=i;
                    else if (helppage[curhp]->numlines-IHELP_NUMDISPLINES > curline)
                        curline = helppage[curhp]->numlines-IHELP_NUMDISPLINES;
                }
            }
            else if (keystatus[KEYSC_BS])   // goto prev paragraph
            {
                keystatus[KEYSC_BS]=0;
                for (i=curline-1, j=0; i>=0; i--)
                {
                    if (!emptyline(helppage[curhp]->line[i])) { j=1; continue; }
                    if (j==1 && emptyline(helppage[curhp]->line[i])) { j=2; break; }
                }
                if (j==2 || i==-1) curline=i+1;
            }
            else if (keystatus[KEYSC_HOME])    // goto beginning of page
            {
                keystatus[KEYSC_HOME]=0;
                curline=0;
            }
            else if (keystatus[KEYSC_END])    // goto end of page
            {
                keystatus[KEYSC_END]=0;
                if ((curline=helppage[curhp]->numlines-IHELP_NUMDISPLINES) >= 0) /**/;
                else curline=0;
            }
            else if (keystatus[KEYSC_LEFT] || keystatus[KEYSC_LBRACK])    // prev page
            {
                keystatus[KEYSC_LEFT] = keystatus[KEYSC_LBRACK] = 0;
                if (curhp>0)
                {
                    curhp--;
                    curline=0;
                }
            }
            else if (keystatus[KEYSC_RIGHT] || keystatus[KEYSC_RBRACK])    // next page
            {
                keystatus[KEYSC_RIGHT] = keystatus[KEYSC_RBRACK] = 0;
                if (curhp<numhelppages-1)
                {
                    curhp++;
                    curline=0;
                }
            }

            // based on 'save as' dialog in overheadeditor()
            else if (keystatus[KEYSC_S])    // text search
            {

                char ch, bad=0, pattern[IHELP_PATLEN+1];

                for (i=0; i<IHELP_PATLEN+1; i++) pattern[i]=0;

                i=0;
                bflushchars();
                while (bad == 0)
                {
                    Bsprintf(tempbuf,"Search: %s_", pattern);
                    printmessage16(tempbuf);
                    showframe(1);

                    if (handleevents())
                    {
                        if (quitevent) quitevent = 0;
                    }
                    idle();

                    ch = bgetchar();

                    if (keystatus[1]) bad = 1;
                    else if (ch == 13) bad = 2;
                    else if (ch > 0)
                    {
                        if (i > 0 && (ch == 8 || ch == 127))
                        {
                            i--;
                            pattern[i] = 0;
                        }
                        else if (i < IHELP_PATLEN && ch >= 32 && ch < 128)
                        {
                            pattern[i++] = ch;
                            pattern[i] = 0;
                        }
                    }
                }

                if (bad==1)
                {
                    keystatus[KEYSC_ESC] = keystatus[KEYSC_Q] = keystatus[KEYSC_F1] = 0;
                }

                if (bad==2)
                {
                    keystatus[KEYSC_ENTER] = 0;

                    for (i=curhp; i<numhelppages; i++)
                    {
                        for (j = (i==curhp)?(curline+1):0; j<helppage[i]->numlines; j++)
                        {
                            // entering an empty pattern will search with the last used pattern
                            if (strstr(helppage[i]->line[j], pattern[0]?pattern:oldpattern))
                            {
                                curhp = i;

                                if ((curline=j) <= helppage[i]->numlines-IHELP_NUMDISPLINES) /**/;
                                else if ((curline=helppage[i]->numlines-IHELP_NUMDISPLINES) >= 0) /**/;
                                else curline=0;

                                highlighthp = i;
                                highlightline = j;
                                lasthighlighttime = totalclock;
                                goto ENDFOR1;
                            }
                        }
                    }
ENDFOR1:
                    if (pattern[0])
                        memcpy(oldpattern, pattern, IHELP_PATLEN+1);
                }
            }
            else    // '1'-'0' on the upper row
            {
                for (i=2; i<=11; i++)
                    if (keystatus[i]) break;
                if (i--<12 && i<numhelppages)
                {
                    curhp=i;
                    curline=0;
                }
            }

            clearmidstatbar16();



            if (curhp < helppage[0]->numlines)
            {
                printmessage16(helppage[0]->line[curhp]);
            }
            else
            {
                for (i=Bsprintf(tempbuf, "%d. (Untitled page)", curhp); i<80; i++)
                    tempbuf[i]=0;
                printmessage16(tempbuf);
            }

            for (i=0; j=(curhp==0)?(i+curline+1):(i+curline),
                    i<IHELP_NUMDISPLINES && j<helppage[curhp]->numlines; i++)
            {
                Bmemcpy(disptext[i], helppage[curhp]->line[j], 80);
                printext16(8,ydim-overridepm16y+28+i*9,10,
                           (j==highlightline && curhp==highlighthp
                            && totalclock-lasthighlighttime<120*5)?1:-1,
                           disptext[i],0);
            }

            showframe(1);
        }

        clearmidstatbar16();
        overridepm16y = -1;
        i=ydim16;
        ydim16=ydim;
        drawline16(0,ydim-STATUS2DSIZ,xdim-1,ydim-STATUS2DSIZ,1);
        ydim16=i;
        printmessage16("");
        showframe(1);

        keystatus[KEYSC_ESC] = keystatus[KEYSC_Q] = keystatus[KEYSC_F1] = 0;
    }
}
// PK_ ^^^^

static void Show3dText(char *name)
{
    int fp,t;
    unsigned char x=0,y=4,xmax=0,xx=0,col=0;

    if ((fp=kopen4load(name,0)) == -1)
    {
        begindrawing();
        printext256(1*4,4*8,whitecol,-1,"ERROR: file not found.",0);
        enddrawing();
        return;
    }
    t=65;
    begindrawing();
    while (t!=EOF && col<5)
    {
        t = 0;
        if (kread(fp,&t,1)<=0)
            t = EOF;
        while (t!=EOF && t!='\n' && x<250)
        {
            tempbuf[x]=t;
            t = 0;
            if (kread(fp,&t,1)<=0) t = EOF;
            x++;
            if (x>xmax) xmax=x;
        }
        tempbuf[x]=0;
        printext256(xx*4,(y*6)+2,whitecol,-1,tempbuf,1);
        x=0;
        y++;
        if (y>18)
        {
            col++;
            y=6;
            xx+=xmax;
            xmax=0;
        }
    }
    enddrawing();

    kclose(fp);
}// end Show3dText

#if 0
static void ShowHelpText(char *name)
{
    BFILE *fp;
    char x=0,y=4;
    UNREFERENCED_PARAMETER(name);
    if ((fp=fopenfrompath("helpdoc.txt","rb")) == NULL)
    {
        begindrawing();
        printext256(1*4,4*8,whitecol,-1,"ERROR: file not found.",0);
        enddrawing();
        return;
    }
    /*
        Bfgets(tempbuf,80,fp);
        while(!Bfeof(fp) && Bstrcmp(tempbuf,"SectorEffector"))
        {
            Bfgets(tempbuf,80,fp);
        }
    */
    y=2;
    Bfgets(tempbuf,80,fp);
    Bstrcat(tempbuf,"\n");
    begindrawing();
    while (!Bfeof(fp) && !(Bstrcmp(tempbuf,"SectorEffector")==0))
    {
        Bfgets(tempbuf,80,fp);
        Bstrcat(tempbuf,"\n");
        printext256(x*4,(y*6)+2,whitecol,-1,tempbuf,1);
        y++;
    }
    enddrawing();

    Bfclose(fp);
}// end ShowHelpText
#endif
void ExtShowSpriteData(short spritenum)   //F6
{
    UNREFERENCED_PARAMETER(spritenum);
    if (qsetmode != 200)
        Show2dText("sehelp.hlp");
    /*    if (qsetmode == 200)                // In 3D mode
            return;

        while (KEY_PRESSED(KEYSC_F6));
        ResetKeys();
        ContextHelp(spritenum);             // Get context sensitive help */
}// end ExtShowSpriteData

// Floor Over Floor (duke3d)

// If standing in sector with SE42 or SE44
// then draw viewing to SE41 and raise all =hi SE43 cielings.

// If standing in sector with SE43 or SE45
// then draw viewing to SE40 and lower all =hi SE42 floors.

int fofsizex = -1;
int fofsizey = -1;
#if 0
static void ResetFOFSize()
{
    if (fofsizex != -1) tilesizx[FOF] = fofsizex;
    if (fofsizey != -1) tilesizy[FOF] = fofsizey;
}
#endif
static void ExtSE40Draw(int spnum,int x,int y,int z,short a,short h)
{
    static int tempsectorz[MAXSECTORS];
    static int tempsectorpicnum[MAXSECTORS];

    int i=0,j=0,k=0;
    int floor1=0,floor2=0,ok=0,fofmode=0,draw_both=0;
    int offx,offy,offz;

    if (sprite[spnum].ang!=512) return;

    // Things are a little different now, as we allow for masked transparent
    // floors and ceilings. So the FOF textures is no longer required
    //	if (!(gotpic[FOF>>3]&(1<<(FOF&7))))
    //		return;
    //	gotpic[FOF>>3] &= ~(1<<(FOF&7));

    if (tilesizx[562])
    {
        fofsizex = tilesizx[562];
        tilesizx[562] = 0;
    }
    if (tilesizy[562])
    {
        fofsizey = tilesizy[562];
        tilesizy[562] = 0;
    }

    floor1=spnum;

    if (sprite[spnum].lotag==42) fofmode=40;
    if (sprite[spnum].lotag==43) fofmode=41;
    if (sprite[spnum].lotag==44) fofmode=40;
    if (sprite[spnum].lotag==45) fofmode=41;

    // fofmode=sprite[spnum].lotag-2;

    // sectnum=sprite[j].sectnum;
    // sectnum=cursectnum;
    ok++;

    /*  recursive?
    for(j=0;j<MAXSPRITES;j++)
    {
    if(
    sprite[j].sectnum==sectnum &&
    sprite[j].picnum==1 &&
    sprite[j].lotag==110
       ) { DrawFloorOverFloor(j); break;}
    }
    */

    // if(ok==0) { Message("no fof",RED); return; }

    for (j=0;j<MAXSPRITES;j++)
    {
        if (sprite[j].picnum==1 && sprite[j].lotag==fofmode && sprite[j].hitag==sprite[floor1].hitag)
        {
            floor1=j;
            fofmode=sprite[j].lotag;
            ok++;
            break;
        }
    }
    // if(ok==1) { Message("no floor1",RED); return; }

    if (fofmode==40) k=41;
    else k=40;

    for (j=0;j<MAXSPRITES;j++)
    {
        if (sprite[j].picnum==1 && sprite[j].lotag==k && sprite[j].hitag==sprite[floor1].hitag)
        {
            floor2=j;
            ok++;
            break;
        }
    }

    i=floor1;
    offx=sprite[floor2].x-sprite[floor1].x;
    offy=sprite[floor2].y-sprite[floor1].y;
    offz=0;

    if (sprite[floor2].ang >= 1024)
        offz = sprite[floor2].z;
    else if (fofmode==41)
        offz = sector[sprite[floor2].sectnum].floorz;
    else
        offz = sector[sprite[floor2].sectnum].ceilingz;

    if (sprite[floor1].ang >= 1024)
        offz -= sprite[floor1].z;
    else if (fofmode==40)
        offz -= sector[sprite[floor1].sectnum].floorz;
    else
        offz -= sector[sprite[floor1].sectnum].ceilingz;

    // if(ok==2) { Message("no floor2",RED); return; }

    for (j=0;j<MAXSPRITES;j++) // raise ceiling or floor
    {
        if (sprite[j].picnum==1 && sprite[j].lotag==k+2 && sprite[j].hitag==sprite[floor1].hitag)
        {
            if (k==40)
            {
                tempsectorz[sprite[j].sectnum]=sector[sprite[j].sectnum].floorz;
                sector[sprite[j].sectnum].floorz+=(((z-sector[sprite[j].sectnum].floorz)/32768)+1)*32768;
                tempsectorpicnum[sprite[j].sectnum]=sector[sprite[j].sectnum].floorpicnum;
                sector[sprite[j].sectnum].floorpicnum=562;
            }
            else
            {
                tempsectorz[sprite[j].sectnum]=sector[sprite[j].sectnum].ceilingz;
                sector[sprite[j].sectnum].ceilingz+=(((z-sector[sprite[j].sectnum].ceilingz)/32768)-1)*32768;
                tempsectorpicnum[sprite[j].sectnum]=sector[sprite[j].sectnum].ceilingpicnum;
                sector[sprite[j].sectnum].ceilingpicnum=562;
            }
            draw_both = 1;
        }
    }

    drawrooms(x+offx,y+offy,z+offz,a,h,sprite[floor2].sectnum);
    ExtAnalyzeSprites();
    drawmasks();

    if (draw_both)
    {
        for (j=0;j<MAXSPRITES;j++) // restore ceiling or floor for the draw both sectors
        {
            if (sprite[j].picnum==1 &&
                    sprite[j].lotag==k+2 &&
                    sprite[j].hitag==sprite[floor1].hitag)
            {
                if (k==40)
                {
                    sector[sprite[j].sectnum].floorz=tempsectorz[sprite[j].sectnum];
                    sector[sprite[j].sectnum].floorpicnum=tempsectorpicnum[sprite[j].sectnum];
                }
                else
                {
                    sector[sprite[j].sectnum].ceilingz=tempsectorz[sprite[j].sectnum];
                    sector[sprite[j].sectnum].ceilingpicnum=tempsectorpicnum[sprite[j].sectnum];
                }
            }// end if
        }// end for

        // Now re-draw
        drawrooms(x+offx,y+offy,z+offz,a,h,sprite[floor2].sectnum);
        ExtAnalyzeSprites();
        drawmasks();
    }

} // end SE40

static void SE40Code(int x,int y,int z,int a,int h)
{
    int i;

    i = 0;
    while (i<MAXSPRITES)
    {
        int t = sprite[i].lotag;
        switch (t)
        {
            //            case 40:
            //            case 41:
            //                ExtSE40Draw(i,x,y,z,a,h);
            //                break;
        case 42:
        case 43:
        case 44:
        case 45:
            if (cursectnum == sprite[i].sectnum)
                ExtSE40Draw(i,x,y,z,a,h);
            break;
        }
        i++;
    }
}

void ExtEditSectorData(short sectnum)    //F7
{
    //    if (qsetmode != 200) Show2dText("sthelp.hlp");
    if (qsetmode == 200)
        return;
    if (eitherALT)  //ALT
    {
        keystatus[KEYSC_F7] = 0;
        wallsprite=0;
        curwall = 0;
        curwallnum = 0;
        cursearchspritenum = 0;
        cursectornum=0;
        cursector_lotag = sector[sectnum].lotag;
        cursector_lotag=getnumber16("Enter search sector lotag : ", cursector_lotag, 65536L,0);
        Bsprintf(tempbuf,"Search sector lotag %d",cursector_lotag);
        printmessage16(tempbuf);
    }
    else EditSectorData(sectnum);
}// end ExtEditSectorData

void ExtEditWallData(short wallnum)       //F8
{
    if (qsetmode==200)
        return;
    if (eitherALT)  //ALT
    {
        wallsprite=1;
        curwall = wallnum;
        curwallnum = 0;
        cursearchspritenum = 0;
        cursectornum = 0;
        search_lotag = wall[curwall].lotag;
        search_hitag = wall[curwall].hitag;
        search_lotag=getnumber16("Enter wall search lotag : ", search_lotag, 65536L,0);
        search_hitag=getnumber16("Enter wall search hitag : ", search_hitag, 65536L,0);
        //    Bsprintf(tempbuf,"Current wall %d lo=%d hi=%d",
        //             curwall,wall[curwall].lotag,wall[curwall].hitag);
        Bsprintf(tempbuf,"Search wall lo=%d hi=%d",search_lotag,search_hitag);
        printmessage16(tempbuf);
    }
    else EditWallData(wallnum);
}

static void GenSearchSprite(void);

void ExtEditSpriteData(short spritenum)   //F8
{
    if (qsetmode==200)
        return;
    if (eitherALT)  //ALT
        GenSearchSprite();
#if 0
    {
        wallsprite=2;
        cursearchsprite = spritenum;
        curwallnum = 0;
        cursearchspritenum = 0;
        cursectornum = 0;
        search_lotag = sprite[cursearchsprite].lotag;
        search_hitag = sprite[cursearchsprite].hitag;
        search_lotag=getnumber16("Enter sprite search lotag : ", search_lotag, 65536L,0);
        search_hitag=getnumber16("Enter sprite search hitag : ", search_hitag, 65536L,0);
        Bsprintf(tempbuf,"Search sprite lo=%d hi=%d",search_lotag,search_hitag);
        printmessage16(tempbuf);
    }
#endif
    else EditSpriteData(spritenum);
}

static void PrintStatus(char *string,int num,char x,char y,char color)
{
    Bsprintf(tempbuf,"%s %d",string,num);
    begindrawing();
    printext16(x*8,ydim16+y*8,color,-1,tempbuf,0);
    enddrawing();
}

static inline void SpriteName(short spritenum, char *lo2)
{
    Bstrcpy(lo2,names[sprite[spritenum].picnum]);
}// end SpriteName

static void ReadPaletteTable()
{
    int i,j,fp;
    char lookup_num;
    if ((fp=kopen4load("lookup.dat",0)) == -1)
    {
        if ((fp=kopen4load("lookup.dat",1)) == -1)
        {
            initprintf("LOOKUP.DAT not found, creating dummy palette lookups\n");
            for (i=0;i<256;i++)
                tempbuf[i] = ((i+32)&255);  //remap colors for screwy palette sectors
            makepalookup(MAXPALOOKUPS,tempbuf,0,0,0,1);
            return;
        }
    }
//    initprintf("Loading palette lookups... ");
    kread(fp,&num_tables,1);
    for (j=0;j<num_tables;j++)
    {
        kread(fp,&lookup_num,1);
        kread(fp,tempbuf,256);
        makepalookup(lookup_num,tempbuf,0,0,0,1);
    }
    for (j = 0; j < 256; j++)
        tempbuf[j] = j;
    num_tables++;
    makepalookup(num_tables, tempbuf, 15, 15, 15, 1);
    makepalookup(num_tables + 1, tempbuf, 15, 0, 0, 1);
    makepalookup(num_tables + 2, tempbuf, 0, 15, 0, 1);
    makepalookup(num_tables + 3, tempbuf, 0, 0, 15, 1);

    kread(fp,WATERpalette,768);
    kread(fp,SLIMEpalette,768);
    kread(fp,TITLEpalette,768);
    kread(fp,REALMSpalette,768);
    kread(fp,BOSS1palette,768);
    kclose(fp);
//    initprintf("success.\n");
}// end ReadPaletteTable

static void ReadGamePalette()
{
    int i,fp;
    if ((fp=kopen4load("palette.dat",0)) == -1)
        if ((fp=kopen4load("palette.dat",1)) == -1)
        {
            initprintf("!!! PALETTE.DAT NOT FOUND !!!\n");
            Bstrcpy(tempbuf, "Mapster32"VERSION BUILDDATE);
            wm_msgbox(tempbuf,"palette.dat not found");
            exit(0);
        }
//    initprintf("Loading game palette... ");
    kread(fp,GAMEpalette,768);
    for (i=0;i<768;++i) GAMEpalette[i]=GAMEpalette[i];
    kclose(fp);
//    initprintf("success.\n");
    ReadPaletteTable();
}

void message(const char *fmt, ...)
{
    char tmpstr[256];
    va_list va;

    va_start(va, fmt);
    Bvsnprintf(tmpstr, 256, fmt, va);
    va_end(va);

    Bstrcpy(getmessage,tmpstr);
    getmessageleng = strlen(getmessage);
    getmessagetimeoff = totalclock+120*3;
    lastmessagetime = totalclock;
    if (!mouseaction)
    {
        Bstrcat(tmpstr,"\n");
        OSD_Printf(tmpstr);
    }
}

static char lockbyte4094;

static int lastupdate, mousecol, mouseadd = 1, bstatus;

static void m32_showmouse(void)
{
    int i, col;

    if (totalclock > lastupdate)
    {
        mousecol += mouseadd;
        if (mousecol >= 30 || mousecol <= 0)
        {
            mouseadd = -mouseadd;
            mousecol += mouseadd;
        }
        lastupdate = totalclock + 3;
    }

    switch (whitecol)
    {
    case 1:  // Shadow Warrior
        col = whitecol+mousecol;
        break;
    case 31: // Duke Nukem 3D
        col = whitecol-mousecol;
        break;
    default:
        col = whitecol;
        break;
    }

    if (col != whitecol)
    {
        for (i=((xdim > 640)?3:2);i<=((xdim > 640)?7:3);i++)
        {
            plotpixel(searchx+i,searchy,col);
            plotpixel(searchx-i,searchy,col);
            plotpixel(searchx,searchy-i,col);
            plotpixel(searchx,searchy+i,col);
        }
        for (i=1;i<=((xdim > 640)?2:1);i++)
        {
            plotpixel(searchx+i,searchy,whitecol);
            plotpixel(searchx-i,searchy,whitecol);
            plotpixel(searchx,searchy-i,whitecol);
            plotpixel(searchx,searchy+i,whitecol);
        }
        i=((xdim > 640)?8:4);
        plotpixel(searchx+i,searchy,0);
        plotpixel(searchx-i,searchy,0);
        plotpixel(searchx,searchy-i,0);
        plotpixel(searchx,searchy+i,0);
    }

    if (xdim > 640)
    {
        for (i=1;i<=4;i++)
        {
            plotpixel(searchx+i,searchy,whitecol);
            plotpixel(searchx-i,searchy,whitecol);
            plotpixel(searchx,searchy-i,whitecol);
            plotpixel(searchx,searchy+i,whitecol);
        }
    }
}

static int AskIfSure(char *text)
{
    int retval=1;

    if (qsetmode == 200)
    {
        begindrawing(); //{{{
        printext256(0,0,whitecol,0,text?text:"Are you sure you want to proceed?",0);
        enddrawing();   //}}}
    }
    else
    {
        printmessage16(text?text:"Are you sure you want to proceed?");
    }

    showframe(1);

    while ((keystatus[KEYSC_ESC]|keystatus[KEYSC_ENTER]|keystatus[KEYSC_SPACE]|keystatus[KEYSC_N]) == 0)
    {
        if (handleevents())
        {
            if (quitevent)
            {
                retval = 1;
                break;
            }
        }
        idle();
        if (keystatus[KEYSC_Y]||keystatus[KEYSC_ENTER])
        {
            keystatus[KEYSC_Y] = 0;
            keystatus[KEYSC_ENTER] = 0;
            retval = 0;
            break;
        }
    }
    while (keystatus[KEYSC_ESC])
    {
        keystatus[KEYSC_ESC] = 0;
        retval = 1;
        break;
    }
    return(retval);
}

static int IsValidTile(const int idTile)
{
    int bValid = 0;

    if ((idTile >= 0) && (idTile < MAXTILES))
    {
        if ((tilesizx[idTile] != 0) && (tilesizy[idTile] != 0))
        {
            bValid = 1;
        }
    }

    return bValid;
}

static int SelectAllTiles(int iCurrentTile)
{
    int i;

    if (iCurrentTile < localartlookupnum)
    {
        iCurrentTile = localartlookup[iCurrentTile];
    }
    else
    {
        iCurrentTile = 0;
    }

    localartlookupnum = MAXTILES;

    for (i = 0; i < MAXTILES; i++)
    {
        localartlookup[i] = i;
        localartfreq[i] = 0;
    }

    return iCurrentTile;
}

static int OnGotoTile(int iTile);
static int OnSelectTile(int iTile);
static int s_Zoom = INITIAL_ZOOM;
static int s_TileZoom = 1;

static int DrawTiles(int iTopLeft, int iSelected, int nXTiles, int nYTiles, int TileDim, int offset);


static int m32gettile(int idInitialTile)
{
    int gap, temp;
    int nXTiles, nYTiles, nDisplayedTiles;
    int i;
    int iTile, iTopLeftTile;
    int idSelectedTile;
    int scrollmode;
    int mousedx, mousedy, mtile, omousex=searchx, omousey=searchy, moffset=0;

// Enable following line for testing. I couldn't work out how to change vidmode on the fly
// s_Zoom = NUM_ZOOMS - 1;

    if (idInitialTile < 0)
    {
        idInitialTile = 0;
    }
    else if (idInitialTile >= MAXTILES)
    {
        idInitialTile = MAXTILES - 1;
    }

    // Ensure zoom not to big (which can happen if display size
    //   changes whilst Mapster is running)
    do
    {
        nXTiles = xdim / ZoomToThumbSize[s_Zoom];
        nYTiles = ydim / ZoomToThumbSize[s_Zoom];
        // Refuse to draw less than half of a row.
        if (ZoomToThumbSize[s_Zoom]/2 < 12) nYTiles--;
        nDisplayedTiles  = nXTiles * nYTiles;

        if (!nDisplayedTiles)
        {
            // Eh-up, resolution changed since we were last displaying tiles.
            s_Zoom--;
        }
    }
    while (!nDisplayedTiles);

    keystatus[KEYSC_V] = 0;

    for (i = 0; i < MAXTILES; i++)
    {
        localartfreq[i] = 0;

        localartlookup[i] = i;
    }

    iTile = idSelectedTile = idInitialTile;

    switch (searchstat)
    {
    case 0 :
        for (i = 0; i < numwalls; i++)
        {
            localartfreq[ wall[i].picnum ]++;
        }
        break;

    case 1 :
    case 2 :
        for (i = 0; i < numsectors; i++)
        {
            localartfreq[ sector[i].ceilingpicnum ]++;
            localartfreq[ sector[i].floorpicnum ]++;
        }
        break;

    case 3 :
        for (i=0;i<MAXSPRITES;i++)
        {
            if (sprite[i].statnum < MAXSTATUS)
            {
                localartfreq[ sprite[i].picnum ]++;
            }
        }
        break;

    case 4 :
        for (i = 0; i < numwalls; i++)
        {
            localartfreq[ wall[i].overpicnum ]++;
        }
        break;

    default :
        break;
    }


    //
    //	Sort tiles into frequency order
    //

    gap = MAXTILES / 2;

    do
    {
        for (i = 0; i < MAXTILES-gap; i++)
        {
            temp = i;

            while ((localartfreq[temp] < localartfreq[temp+gap]) && (temp >= 0))
            {
                int tempint;

                tempint = localartfreq[temp];
                localartfreq[temp] = localartfreq[temp+gap];
                localartfreq[temp+gap] = tempint;

                tempint = localartlookup[temp];
                localartlookup[temp] = localartlookup[temp+gap];
                localartlookup[temp+gap] = tempint;

                if (iTile == temp)
                {
                    iTile = temp + gap;
                }
                else if (iTile == temp + gap)
                {
                    iTile = temp;
                }

                temp -= gap;
            }
        }
        gap >>= 1;
    }
    while (gap > 0);

    //
    // Set up count of number of used tiles
    //

    localartlookupnum = 0;
    while (localartfreq[localartlookupnum] > 0)
    {
        localartlookupnum++;
    }

    //
    // Check : If no tiles used at all then switch to displaying all tiles
    //

    if (!localartfreq[0])
    {
        localartlookupnum = MAXTILES;

        for (i = 0; i < MAXTILES; i++)
        {
            localartlookup[i] = i;
            localartfreq[i] = 0; // Terrible bodge : zero tilefreq's not displayed in tile view. Still, when in Rome ... :-)
        }

        iTile = idInitialTile;
    }

    //
    //
    //

    iTopLeftTile = iTile - (iTile % nXTiles);

    if (iTopLeftTile < 0)
    {
        iTopLeftTile = 0;
    }

    if (iTopLeftTile > MAXTILES-nDisplayedTiles)
    {
        iTopLeftTile = MAXTILES-nDisplayedTiles;
    }

    searchx=((iTile-iTopLeftTile)%nXTiles)*ZoomToThumbSize[s_Zoom]+ZoomToThumbSize[s_Zoom]/2;
    searchy=((iTile-iTopLeftTile)/nXTiles)*ZoomToThumbSize[s_Zoom]+ZoomToThumbSize[s_Zoom]/2;

    ////////////////////////////////
    // Start of key handling code //
    ////////////////////////////////

    while ((keystatus[KEYSC_ENTER]|keystatus[KEYSC_ESC]|(bstatus&1)) == 0) // <- Presumably one of these is escape key ???
    {
        DrawTiles(iTopLeftTile, (iTile >= localartlookupnum)?localartlookupnum-1:iTile, nXTiles, nYTiles, ZoomToThumbSize[s_Zoom],moffset);

        getmousevalues(&mousedx,&mousedy,&bstatus);
        searchx += mousedx;
        searchy += mousedy;
        if (bstatus&2)
        {
            moffset+=mousedy*2;
            searchy += mousedy;
            searchx -= mousedx;
            if ((moffset < 0 && iTopLeftTile > localartlookupnum-nDisplayedTiles-1)
                    || (moffset > 0 && iTopLeftTile==0))
            {
                moffset=0;
                searchy -= mousedy*2;
            }
            while (moffset>ZoomToThumbSize[s_Zoom])
            {
                iTopLeftTile-=nXTiles;
                moffset-=ZoomToThumbSize[s_Zoom];
            }
            while (moffset<-ZoomToThumbSize[s_Zoom])
            {
                iTopLeftTile+=nXTiles;
                moffset+=ZoomToThumbSize[s_Zoom];
            }
        }

        // Keep the pointer visible at all times.
        temp = min((ZoomToThumbSize[s_Zoom] / 2), 12);
        if (searchx < temp) searchx = temp;
        if (searchy < temp) searchy = temp;
        if (searchx > xdim - temp) searchx = xdim - temp;
        if (searchy > ydim - temp) searchy = ydim - temp;

        scrollmode=!(eitherCTRL^revertCTRL);
        if (bstatus&16 && scrollmode && iTopLeftTile > 0)
        {
            mouseb &= ~16;
            iTopLeftTile -= (nXTiles*scrollamount);
        }
        if (bstatus&32 && scrollmode && iTopLeftTile < localartlookupnum-nDisplayedTiles-1)
        {
            mouseb &= ~32;
            iTopLeftTile += (nXTiles*scrollamount);
        }
        mtile=iTile=(searchx/ZoomToThumbSize[s_Zoom])+((searchy-moffset)/ZoomToThumbSize[s_Zoom])*nXTiles+iTopLeftTile;
        while (iTile >= iTopLeftTile + nDisplayedTiles)
        {
            iTile-=nXTiles;
            mtile=iTile;
        }

        if (handleevents())
        {
            if (quitevent) quitevent = 0;
        }
        idle();

        // These two lines are so obvious I don't need to comment them ...;-)
        synctics = totalclock-lockclock;
        lockclock += synctics;

        // Zoom in / out using numeric key pad's / and * keys
        if (((keystatus[KEYSC_gSLASH] || (!scrollmode && bstatus&16)) && s_Zoom<(signed)(NUM_ZOOMS-1))
                || ((keystatus[KEYSC_gSTAR]  || (!scrollmode && bstatus&32)) && s_Zoom>0))
        {
            if (keystatus[KEYSC_gSLASH] || (!scrollmode && bstatus&16))
            {
                keystatus[KEYSC_gSLASH] = 0;
                mouseb &= ~16;
                bstatus &= ~16;

                // Watch out : If editor window is small, then the next zoom level
                //  might get so large that even one tile might not fit !
                if ((ZoomToThumbSize[s_Zoom+1] <= xdim)
                        && (ZoomToThumbSize[s_Zoom+1] <= ydim))
                {
                    // Phew, plenty of room.
                    s_Zoom++;
                }
            }
            else
            {
                keystatus[KEYSC_gSTAR] = 0;
                mouseb &= ~32;
                bstatus &= ~32;
                s_Zoom--;
            }

            if (iTile >= localartlookupnum)iTile = localartlookupnum-1;

            // Calculate new num of tiles to display
            nXTiles = xdim / ZoomToThumbSize[s_Zoom];
            nYTiles = ydim / ZoomToThumbSize[s_Zoom];
            // Refuse to draw less than half of a row.
            if (ZoomToThumbSize[s_Zoom]/2 < 12) nYTiles--;
            nDisplayedTiles  = nXTiles * nYTiles;

            // Determine if the top-left displayed tile needs to
            //   alter in order to display selected tile
            iTopLeftTile = iTile - (iTile % nXTiles);

            if (iTopLeftTile < 0)
            {
                iTopLeftTile = 0;
            }
            else if (iTopLeftTile > MAXTILES - nDisplayedTiles)
            {
                iTopLeftTile = MAXTILES - nDisplayedTiles;
            }
            // scroll window so mouse points the same tile as it was before zooming
            iTopLeftTile-=(searchx/ZoomToThumbSize[s_Zoom])+((searchy-moffset)/ZoomToThumbSize[s_Zoom])*nXTiles+iTopLeftTile-iTile;
        }

        if (keystatus[KEYSC_LEFT])
        {
            iTile -= (iTile > 0);
            keystatus[KEYSC_LEFT] = 0;
        }

        if (keystatus[KEYSC_RIGHT])
        {
            iTile += (iTile < MAXTILES);
            keystatus[KEYSC_RIGHT] = 0;
        }

        if (keystatus[KEYSC_UP])
        {
            iTile -= nXTiles;
            keystatus[KEYSC_UP] = 0;
        }

        if (keystatus[KEYSC_DOWN])
        {
            iTile += nXTiles;
            keystatus[KEYSC_DOWN] = 0;
        }

        if (keystatus[KEYSC_PGUP])
        {
            iTile -= nDisplayedTiles;
            keystatus[KEYSC_PGUP] = 0;
        }

        if (keystatus[KEYSC_PGDN])
        {
            iTile += nDisplayedTiles;
            keystatus[KEYSC_PGDN] = 0;
        }

        //
        // Ensure tilenum is within valid range
        //

        if (iTile < 0)
        {
            iTile = 0;
        }

        if (iTile >= MAXTILES)	// shouldn't this be the count of num tiles ???
        {
            iTile = MAXTILES-1;
        }

        // 'V'  KEYPRESS
        if (keystatus[KEYSC_V])
        {
            keystatus[KEYSC_V] = 0;

            iTile = SelectAllTiles(iTile);
        }

        // 'G'  KEYPRESS - Goto frame
        if (keystatus[KEYSC_G])
        {
            keystatus[KEYSC_G] = 0;

            iTile = OnGotoTile(iTile);
        }

        // 'U'  KEYPRESS : go straight to user defined art
        if (keystatus[KEYSC_U])
        {
            SelectAllTiles(iTile);

            iTile = FIRST_USER_ART_TILE;
            keystatus[KEYSC_U] = 0;
        }

        // 'A'  KEYPRESS : Go straight to start of Atomic edition's art
        if (keystatus[KEYSC_A])
        {
            SelectAllTiles(iTile);

            iTile = FIRST_ATOMIC_TILE;
            keystatus[KEYSC_A] = 0;
        }

        // 'T' KEYPRESS = Select from pre-defined tileset
        if (keystatus[KEYSC_T])
        {
            keystatus[KEYSC_T] = 0;

            iTile = OnSelectTile(iTile);
        }

        // 'E'  KEYPRESS : Go straight to start of extended art
        if (keystatus[KEYSC_E])
        {
            SelectAllTiles(iTile);

            if (iTile == FIRST_EXTENDED_TILE)
                iTile = SECOND_EXTENDED_TILE;
            else iTile = FIRST_EXTENDED_TILE;

            keystatus[KEYSC_E] = 0;
        }

        if (keystatus[KEYSC_Z])
        {
            s_TileZoom = !s_TileZoom;
            keystatus[KEYSC_Z] = 0;
        }

        //
        //	Adjust top-left to ensure tilenum is within displayed range of tiles
        //

        while (iTile < iTopLeftTile - (moffset<0)?nXTiles:0)
        {
            iTopLeftTile -= nXTiles;
        }

        while (iTile >= iTopLeftTile + nDisplayedTiles)
        {
            iTopLeftTile += nXTiles;
        }

        if (iTopLeftTile < 0)
        {
            iTopLeftTile = 0;
        }

        if (iTopLeftTile > MAXTILES - nDisplayedTiles)
        {
            iTopLeftTile = MAXTILES - nDisplayedTiles;
        }

        if ((keystatus[KEYSC_ENTER] || (bstatus&1)) == 0)   // uh ? Not escape key ?
        {
            idSelectedTile = idInitialTile;
        }
        else
        {
            if (iTile < localartlookupnum)
            {
                // Convert tile num from index to actual tile num
                idSelectedTile = localartlookup[iTile];

                // Check : if invalid tile selected, return original tile num
                if (!IsValidTile(idSelectedTile))
                {
                    idSelectedTile = idInitialTile;
                }
            }
            else
            {
                idSelectedTile = idInitialTile;
            }
        }
        if (mtile!=iTile) // if changed by keyboard, update mouse cursor
        {
            searchx=((iTile-iTopLeftTile)%nXTiles)*ZoomToThumbSize[s_Zoom]+ZoomToThumbSize[s_Zoom]/2;
            searchy=((iTile-iTopLeftTile)/nXTiles)*ZoomToThumbSize[s_Zoom]+ZoomToThumbSize[s_Zoom]/2+moffset;
        }
    }
    searchx=omousex;searchy=omousey;

    keystatus[KEYSC_ESC] = 0;
    keystatus[KEYSC_ENTER] = 0;

    return(idSelectedTile);

}

// Dir = 0 (zoom out) or 1 (zoom in)
//void OnZoomInOut( int *pZoom, int Dir /*0*/ )
//{
//}

static int OnGotoTile(int iTile)
{
    int iTemp, iNewTile;
    char ch;
    char szTemp[128];

    //Automatically press 'V'
    iTile = SelectAllTiles(iTile);

    bflushchars();

    iNewTile = iTemp = 0; //iTile; //PK

    while (keystatus[KEYSC_ESC] == 0)
    {
        if (handleevents())
        {
            if (quitevent) quitevent = 0;
        }
        idle();

        ch = bgetchar();

        Bsprintf(szTemp, "Goto tile: %d_ ", iNewTile);
        printext256(0, 0, whitecol, 0, szTemp, 0);
        showframe(1);

        if (ch >= '0' && ch <= '9')
        {
            iTemp = (iNewTile*10) + (ch-'0');
            if (iTemp < MAXTILES)
            {
                iNewTile = iTemp;
            }
        }
        else if (ch == 8)
        {
            iNewTile /= 10;
        }
        else if (ch == 13)
        {
            iTile = iNewTile;
            break;
        }
    }

    clearkeys();

    return iTile;
}

static int LoadTileSet(const int idCurrentTile, const int *pIds, const int nIds)
{
    int iNewTile = 0;
    int i;

    localartlookupnum = nIds;

    for (i = 0; i < localartlookupnum; i++)
    {
        localartlookup[i] = pIds[i];
        // REM : Could we still utilise localartfreq[] to mark
        //  which tiles are currently used in the map ? Set to 0xFFFF perhaps ?
        localartfreq[i] = 0;

        if (idCurrentTile == pIds[i])
        {
            iNewTile = i;
        }
    }

    return iNewTile;

}

static int OnSelectTile(int iTile)
{
    int bDone = 0;
    int i;
    char ch;

    for (i = 0; (unsigned)i < tile_groups; i++)
    {
        if (s_TileGroups[i].pIds != NULL)
            break;
    }

    if ((unsigned)i == tile_groups) // no tile groups
        return (iTile);

    SelectAllTiles(iTile);

    bflushchars();

    begindrawing();
    setpolymost2dview();
    clearview(0);

    //
    //	Await appropriate selection keypress.
    //

    bDone = 0;

    while (keystatus[KEYSC_ESC] == 0 && (!bDone))
    {
        if (handleevents())
        {
            if (quitevent) quitevent = 0;
        }
        idle();

        //
        // Display the description strings for each available tile group
        //
        for (i = 0; (unsigned)i < tile_groups; i++)
        {
            if (s_TileGroups[i].szText != NULL)
            {
                if ((i+2)*16 > ydimgame) break;
                Bsprintf(tempbuf,"(%c) %s",s_TileGroups[i].key1,s_TileGroups[i].szText);
                printext256(10L, (i+1)*16, whitecol, -1, tempbuf, 0);
            }
        }
        showframe(1);


        ch = bgetchar();

        for (i = 0; (unsigned)i < tile_groups; i++)
        {
            if (s_TileGroups[i].pIds != NULL && s_TileGroups[i].key1)
                if ((ch == s_TileGroups[i].key1) || (ch == s_TileGroups[i].key2))
                {
                    iTile = LoadTileSet(iTile, s_TileGroups[i].pIds, s_TileGroups[i].nIds);
                    bDone = 1;
                }
        }
    }

    enddrawing();
    showframe(1);

    clearkeys();

    return iTile;
}

const char * GetTilePixels(const int idTile)
{
    char *pPixelData = 0;

    if ((idTile >= 0) && (idTile < MAXTILES))
    {
        if (!waloff[idTile])
        {
            loadtile(idTile);
        }

        if (IsValidTile(idTile))
        {
            pPixelData = (char *)waloff[idTile];
        }
    }

    return pPixelData;
}

static int DrawTiles(int iTopLeft, int iSelected, int nXTiles, int nYTiles, int TileDim, int offset)
{
    int XTile, YTile;
    int iTile, idTile;
    int XBox, YBox;
    int XPos, YPos;
    int XOffset, YOffset;
    int i;
    const char * pRawPixels;
    int TileSizeX, TileSizeY;
    int DivInc,MulInc;
    char *pScreen;
    char szT[128];

    begindrawing();

    setpolymost2dview();

    clearview(0);

    for (YTile = 0-(offset>0); YTile < nYTiles+(offset<0)+1; YTile++)
    {
        for (XTile = 0; XTile < nXTiles; XTile++)
        {
            iTile = iTopLeft + XTile + (YTile * nXTiles);

            if (iTile>=0 && iTile < localartlookupnum)
            {
                idTile = localartlookup[ iTile ];

                // Get pointer to tile's raw pixel data
                pRawPixels = GetTilePixels(idTile);

                if (pRawPixels != NULL)
                {
                    XPos = XTile * TileDim;
                    YPos = YTile * TileDim+offset;

                    if (polymost_drawtilescreen(XPos, YPos, idTile, TileDim, s_TileZoom))
                    {
                        TileSizeX = tilesizx[ idTile ];
                        TileSizeY = tilesizy[ idTile ];

                        DivInc = 1;
                        MulInc = 1;

                        while ((TileSizeX/DivInc > TileDim)
                                || (TileSizeY/DivInc) > TileDim)
                        {
                            DivInc++;
                        }

                        if (DivInc == 1 && s_TileZoom)
                        {
                            while (((TileSizeX*(MulInc+1)) <= TileDim)
                                    && ((TileSizeY*(MulInc+1)) <= TileDim))
                            {
                                MulInc++;
                            }
                        }

                        TileSizeX = (TileSizeX / DivInc) * MulInc;
                        TileSizeY = (TileSizeY / DivInc) * MulInc;

                        for (YOffset = 0; YOffset < TileSizeY; YOffset++)
                        {
                            int y=YPos+YOffset;
                            if (y>=0 && y<ydim)
                            {
                                pScreen = (char *)ylookup[y]+XPos+frameplace;
                                for (XOffset = 0; XOffset < TileSizeX; XOffset++)
                                {
                                    pScreen[XOffset] = pRawPixels[((YOffset * DivInc) / MulInc) + (((XOffset * DivInc) / MulInc) * tilesizy[idTile])];
                                }
                            }
                        }
                    }
                    if (localartfreq[iTile] != 0 && YPos >= 0 && YPos <= ydim-20)
                    {
                        Bsprintf(szT, "%d", localartfreq[iTile]);
                        printext256(XPos, YPos, whitecol, -1, szT, 1);
                    }
                }
            }
        }
    }

    //
    // Draw white box around currently selected tile
    //

    XBox = ((iSelected-iTopLeft) % nXTiles) * TileDim;
    YBox = ((iSelected - ((iSelected-iTopLeft) % nXTiles) - iTopLeft) / nXTiles) * TileDim+offset;

    if (iSelected-iTopLeft>0)
        for (i = 0; i < TileDim; i++)
        {
            if (YBox>=0 && YBox<ydim)
                plotpixel(XBox+i,         YBox,           whitecol);
            if (YBox+TileDim>=0 && YBox+TileDim<ydim)
                plotpixel(XBox+i,         YBox + TileDim, whitecol);
            if (YBox+i>=0 && YBox+i<ydim)
            {
                plotpixel(XBox,           YBox + i,       whitecol);
                plotpixel(XBox + TileDim, YBox + i,       whitecol);
            }
        }

    idTile = localartlookup[ iSelected ];

    // Draw info bar at bottom.

    // Clear out behind the text for improved visibility.
    //drawline256(0, (ydim-12)<<12, xdim<<12, (ydim-12)<<12, whitecol);
    for (i=ydim-12; i<ydim; i++)
    {
        drawline256(0, i<<12, xdim<<12, i<<12, (ydim-i));
    }

    // Tile number on left.
    Bsprintf(szT, "%d" , idTile);
    printext256(1, ydim-10, whitecol, -1, szT, 0);

    // Tile name on right.
    printext256(xdim-(Bstrlen(names[idTile])<<3)-1,ydim-10,whitecol,-1,names[idTile],0);

    // Tile dimensions.
    Bsprintf(szT,"%dx%d",tilesizx[idTile],tilesizy[idTile]);
    printext256(xdim>>2,ydim-10,whitecol,-1,szT,0);

    // EditArt animation flags.
    Bsprintf(szT,"%d, %d",(picanm[idTile]>>8)&0xFF,(picanm[idTile]>>16)&0xFF);
    printext256((xdim>>2)+100,ydim-10,whitecol,-1,szT,0);

    m32_showmouse();

    enddrawing();
    showframe(1);

    return(0);

}

int spriteonceilingz(int searchwall)
{
    int z=sprite[searchwall].z;
    z = getceilzofslope(searchsector,sprite[searchwall].x,sprite[searchwall].y);
    if (sprite[searchwall].cstat&128) z -= ((tilesizy[sprite[searchwall].picnum]*sprite[searchwall].yrepeat)<<1);
    if ((sprite[searchwall].cstat&48) != 32)
        z += ((tilesizy[sprite[searchwall].picnum]*sprite[searchwall].yrepeat)<<2);
    return z;
}

int spriteongroundz(int searchwall)
{
    int z=sprite[searchwall].z;
    z = getflorzofslope(searchsector,sprite[searchwall].x,sprite[searchwall].y);
    if (sprite[searchwall].cstat&128) z -= ((tilesizy[sprite[searchwall].picnum]*sprite[searchwall].yrepeat)<<1);
    return z;
}

#define WIND1X   3
#define WIND1Y 150
void drawtileinfo(char *title,int x,int y,int picnum,int shade,int pal,int cstat,int lotag,int hitag,int extra)
{
    char buf[64];
    int i,j;
    int scale=65536;
    int x1;

    j = xdimgame>640?0:1;
    i = ydimgame>>6;

    x1=x+80;
    if (j)x1/=2;
    x1=(int)(x1*(320./xdimgame));
    scale=(int)(scale/(max(tilesizx[picnum],tilesizy[picnum])/24.));
    rotatesprite((x1+13)<<16,(y+11)<<16,scale,0,picnum,shade,pal,2,0L,0L,xdim-1L,ydim-1L);

    x=(int)(x*(xdimgame/320.));
    y=(int)(y*(ydimgame/200.));
    begindrawing();
    printext256(x+2,y+2,0,-1,title,j);
    printext256(x,y,255-13,-1,title,j);

    Bsprintf(buf,"Pic:%4d",picnum);
    printext256(x+2,y+2+i*1,0,-1,buf,j);
    printext256(x,y+i*1,whitecol,-1,buf,j);
    Bsprintf(buf,"Shd:%4d",shade);
    printext256(x+2,y+2+i*2,0,-1,buf,j);
    printext256(x,y+i*2,whitecol,-1,buf,j);
    Bsprintf(buf,"Pal:%4d",pal);
    printext256(x+2,y+2+i*3,0,-1,buf,j);
    printext256(x,y+i*3,whitecol,-1,buf,j);
    Bsprintf(buf,"Cst:%4d",cstat);
    printext256(x+2,y+2+i*4,0,-1,buf,j);
    printext256(x,y+i*4,whitecol,-1,buf,j);
    Bsprintf(buf,"Lot:%4d",lotag);
    printext256(x+2,y+2+i*5,0,-1,buf,j);
    printext256(x,y+i*5,whitecol,-1,buf,j);
    Bsprintf(buf,"Hit:%4d",hitag);
    printext256(x+2,y+2+i*6,0,-1,buf,j);
    printext256(x,y+i*6,whitecol,-1,buf,j);
    Bsprintf(buf,"Ext:%4d",extra);
    printext256(x+2,y+2+i*7,0,-1,buf,j);
    printext256(x,y+i*7,whitecol,-1,buf,j);
    enddrawing();
}
int snap=0;
int saveval1,saveval2,saveval3;

static inline void getnumber_dochar(char *ptr, int num)
{
    *ptr = (char) num;
}

static inline void getnumber_doshort(short *ptr, int num)
{
    *ptr = (short) num;
}

static inline void getnumber_doint32(int32 *ptr, int num)
{
    *ptr = (int32) num;
}

static inline void getnumber_doint64(int64 *ptr, int num)
{
    *ptr = (int64) num;
}

void getnumberptr256(char *namestart, void *num, int bytes, int maxnumber, char sign, void *(func)(int))
{
    char buffer[80], ch;
    int n, danum = 0, oldnum;

    switch (bytes)
    {
    case 1:
        danum = *(char *)num;
        break;
    case 2:
        danum = *(short *)num;
        break;
    case 4:
        danum = *(int32 *)num;
        break;
    case 8:
        danum = *(int64 *)num;
        break;
    }

    oldnum = danum;
    bflushchars();
    while (keystatus[0x1] == 0)
    {
        if (handleevents())
        {
            if (quitevent) quitevent = 0;
        }

        drawrooms(posx,posy,posz,ang,horiz,cursectnum);
#ifdef SUPERBUILD
        ExtAnalyzeSprites();
#endif
        drawmasks();

        ch = bgetchar();

        if (keystatus[0x1]) break;

        clearkeys();

        mouseb = 0;
        searchx = osearchx;
        searchy = osearchy;

        ExtCheckKeys();

        Bsprintf(buffer,"%s%d",namestart,danum);
        if (totalclock & 32) Bstrcat(buffer,"_ ");
        printmessage256(0, 0, buffer);
        if (func != NULL)
        {
            Bsprintf(buffer,"%s",(char *)func((int)danum));
            printmessage256(0, 9, buffer);
        }
        showframe(1);

        if (ch >= '0' && ch <= '9')
        {
            if (danum >= 0)
            {
                n = (danum*10)+(ch-'0');
                if (n <= maxnumber) danum = n;
            }
            else if (sign)
            {
                n = (danum*10)-(ch-'0');
                if (n >= -maxnumber) danum = n;
            }
        }
        else if (ch == 8 || ch == 127)  	// backspace
        {
            danum /= 10;
        }
        else if (ch == 13)
        {
            oldnum = danum;
            asksave = 1;
            break;
        }
        else if (ch == '-' && sign)  	// negate
        {
            danum = -danum;
        }
        switch (bytes)
        {
        case 1:
            getnumber_dochar(num, danum);
            break;
        case 2:
            getnumber_doshort(num, danum);
            break;
        case 4:
            getnumber_doint32(num, danum);
            break;
        case 8:
            getnumber_doint64(num, danum);
            break;
        }
    }
    clearkeys();

    lockclock = totalclock;  //Reset timing

    switch (bytes)
    {
    case 1:
        getnumber_dochar(num, oldnum);
        break;
    case 2:
        getnumber_doshort(num, oldnum);
        break;
    case 4:
        getnumber_doint32(num, oldnum);
        break;
    case 8:
        getnumber_doint64(num, oldnum);
        break;
    }
}

static void DoSpriteOrnament(int i)
{
    int j;
    int hitx, hity, hitz;
    short hitsect, hitwall, hitsprite;

    hitscan(sprite[i].x,sprite[i].y,sprite[i].z,sprite[i].sectnum,
            sintable[(sprite[i].ang+2560+1024)&2047],
            sintable[(sprite[i].ang+2048+1024)&2047],
            0,
            &hitsect,&hitwall,&hitsprite,&hitx,&hity,&hitz,CLIPMASK1);

    sprite[i].x = hitx;
    sprite[i].y = hity;
    sprite[i].z = hitz;
    changespritesect(i,hitsect);
    if (hitwall >= 0)
        sprite[i].ang = ((getangle(wall[wall[hitwall].point2].x-wall[hitwall].x,wall[wall[hitwall].point2].y-wall[hitwall].y)+512)&2047);

    //Make sure sprite's in right sector
    if (inside(sprite[i].x,sprite[i].y,sprite[i].sectnum) == 0)
    {
        j = wall[hitwall].point2;
        sprite[i].x -= ksgn(wall[j].y-wall[hitwall].y);
        sprite[i].y += ksgn(wall[j].x-wall[hitwall].x);
    }
}

int64 ldistsqr(spritetype *s1,spritetype *s2)
{
    return (((int64)(s2->x - s1->x))*((int64)(s2->x - s1->x)) +
            ((int64)(s2->y - s1->y))*((int64)(s2->y - s1->y)));
}

void rendertext(short startspr)
{
    char ch, buffer[80], doingspace=0;
    short daang = 0, t, alphidx, basetile, linebegspr, curspr, cursor;
    int i, j, k, dax = 0, day = 0;
    static unsigned char hgap=0, vgap=4;
    static unsigned char spcgap[MAX_ALPHABETS], firstrun=1;
    spritetype *sp;

    short *spritenums;
    int stackallocsize=32, numletters=0;

    if (firstrun)
    {
        firstrun=0;
        for (i=0; i<numalphabets; i++)
            spcgap[i] = 0;
    }

    if (startspr<0 || startspr>=MAXSPRITES ||
            sprite[startspr].statnum == MAXSTATUS)
        return;

    if (numalphabets == 0)
    {
        message("Alphabet configuration not read.");
        return;
    }

    if ((sprite[startspr].cstat&16) == 0)
    {
        message("Must point at a wall-aligned text sprite.");
        return;
    }

    t = sprite[startspr].picnum;
    alphidx = -1;
    for (i=0; i<numalphabets; i++)
    {
        for (j=0; j<NUMPRINTABLES; j++)
            if (alphabets[i].pic[j] == t)
            {
                alphidx = i;
                basetile = t;
                if (spcgap[i] == 0)
                    spcgap[i] = 3*tilesizx[t]/2;
                goto ENDFOR1;
            }
    }
ENDFOR1:
    if (alphidx==-1)
    {
        message("Must point at a text sprite.");
        return;
    }

    curspr = linebegspr = startspr;

    t = sprite[startspr].picnum;
    sprite[startspr].xoffset = -(((picanm[t])>>8)&255);
    sprite[startspr].yoffset = -(((picanm[t])>>16)&255);

    spritenums = Bmalloc(stackallocsize * sizeof(short));
    if (!spritenums) goto ERROR_NOMEMORY;

    cursor = insertsprite(sprite[startspr].sectnum,0);
    if (cursor < 0) goto ERROR_TOOMANYSPRITES;

    updatenumsprites();

    sp = &sprite[cursor];
    Bmemcpy(sp, &sprite[startspr], sizeof(spritetype));
    sp->yoffset = 0;
    sp->picnum = SMALLFNTCURSOR;
    sp->xrepeat = min(max(sp->xrepeat/tilesizx[sp->picnum],2),255);
    sp->yrepeat = min(max((sp->yrepeat*tilesizy[sprite[startspr].picnum])/tilesizy[sp->picnum],4),255);
    sp->pal = 0;
    sp->cstat = 18;

    bflushchars();
    while (keystatus[0x1] == 0)
    {
        if (handleevents())
        {
            if (quitevent) quitevent = 0;
        }

        if (keystatus[KEYSC_UP])  // vertical gap in pixels (32 x-units)
        {
            keystatus[KEYSC_UP]=0;
            if (vgap<255) vgap++;
        }
        if (keystatus[KEYSC_DOWN])
        {
            keystatus[KEYSC_DOWN]=0;
            if (vgap>0) vgap--;
        }

        if (keystatus[KEYSC_RIGHT])  // horizontal gap in half pixels
        {
            keystatus[KEYSC_RIGHT]=0;
            if (hgap<255) hgap++;
        }
        if (keystatus[KEYSC_LEFT])
        {
            keystatus[KEYSC_LEFT]=0;
            if (hgap>0) hgap--;
        }

        if (keystatus[KEYSC_INSERT])  // space gap in half pixels
        {
            keystatus[KEYSC_INSERT]=0;
            if (spcgap[alphidx]<255) spcgap[alphidx]++;
        }
        if (keystatus[KEYSC_DELETE])
        {
            keystatus[KEYSC_DELETE]=0;
            if (spcgap[alphidx]>1) spcgap[alphidx]--;
        }

        if (keystatus[KEYSC_HOME])  // shade
        {
            keystatus[KEYSC_HOME]=0;
            if (sprite[linebegspr].shade<127) sprite[linebegspr].shade++;
        }
        if (keystatus[KEYSC_END])
        {
            keystatus[KEYSC_END]=0;
            if (sprite[linebegspr].shade>-128) sprite[linebegspr].shade--;
        }

        if (keystatus[KEYSC_PGUP])  // pal
        {
            keystatus[KEYSC_PGUP]=0;
            if (sprite[linebegspr].pal<255) sprite[linebegspr].pal++;
        }
        if (keystatus[KEYSC_PGDN])
        {
            keystatus[KEYSC_PGDN]=0;
            if (sprite[linebegspr].pal>0) sprite[linebegspr].pal--;
        }

        drawrooms(posx,posy,posz,ang,horiz,cursectnum);
#ifdef SUPERBUILD
        ExtAnalyzeSprites();
#endif
        drawmasks();

        ch = bgetchar();

        if (keystatus[0x1]) break;

        clearkeys();

        mouseb = 0;
        searchx = osearchx;
        searchy = osearchy;

        ExtCheckKeys();

        printmessage256(0,0,"^251Text entry mode.^31 Navigation keys change vars.");
        Bsprintf(buffer, "Hgap=%d, Vgap=%d, SPCgap=%d, Shd=%d, Pal=%d",
                 hgap, vgap, spcgap[alphidx], sprite[linebegspr].shade, sprite[linebegspr].pal);
        printmessage256(0, 9, buffer);
        showframe(1);

// ---
        sp = &sprite[curspr];
        if (!doingspace)
        {
            dax = sp->x; day = sp->y;
            daang = sp->ang;
        }

        j = sp->xrepeat*(hgap+tilesizx[sp->picnum]+2);
        setsprite(cursor,
                  dax + ((j*sintable[daang])>>17),
                  day - ((j*sintable[(daang+512)&2047])>>17),
                  sp->z);

        if (ch>=33 && ch<=126 && alphabets[alphidx].pic[ch-33] >= 0)
        {
            short sect;

            // mapping char->tilenum
            t = alphabets[alphidx].pic[ch-33];
            j = sp->xrepeat*(hgap+tilesizx[sp->picnum]+tilesizx[t]);

            dax += (j*sintable[daang])>>17;
            day -= (j*sintable[(daang+512)&2047])>>17;
            dax += (j*sintable[(sprite[curspr].ang+2560)&2047])>>17;
            day += (j*sintable[(sprite[curspr].ang+2048)&2047])>>17;

            sect = sprite[curspr].sectnum;
            updatesector(dax,day,&sect);
            if (numsprites < MAXSPRITES && sect >= 0)
            {
                i = insertsprite(sect,0);
                Bmemcpy(&sprite[i], &sprite[linebegspr], sizeof(spritetype));
                sprite[i].x = dax, sprite[i].y = day;
                sprite[i].picnum = t;
                sprite[i].ang = daang;

                sprite[i].xoffset = -(((picanm[sprite[i].picnum])>>8)&255);
                sprite[i].yoffset = -(((picanm[sprite[i].picnum])>>16)&255);
                sprite[i].xoffset += alphabets[alphidx].xofs[(int)ch-33];
                sprite[i].yoffset += alphabets[alphidx].yofs[(int)ch-33];

                DoSpriteOrnament(i);

                for (k=0;k<MAXTILES;k++)
                    localartfreq[k] = 0;
                for (k=0;k<MAXSPRITES;k++)
                    if (sprite[k].statnum < MAXSTATUS)
                        localartfreq[sprite[k].picnum]++;

                curspr = i;
                doingspace = 0;

                updatenumsprites();
                asksave = 1;

                if (numletters >= stackallocsize)
                {
                    stackallocsize *= 2;
                    spritenums = Brealloc(spritenums, stackallocsize*sizeof(short));
                    if (!spritenums) goto ERROR_NOMEMORY;
                }
                spritenums[numletters++] = i;
            }
        }
        else if (ch == 32)
        {
            dax += ((sp->xrepeat*spcgap[alphidx]*sintable[daang])>>17);
            day -= ((sp->xrepeat*spcgap[alphidx]*sintable[(daang+512)&2047])>>17);
            doingspace = 1;
        }
        else if (ch == 8)  	// backspace
        {
            if (doingspace)
                doingspace = 0;
            else if (numletters > 0)
            {
                short last = spritenums[numletters-1];

                if (sprite[last].z != sprite[linebegspr].z)  // only "delete" line break
                {
                    sprite[linebegspr].z = sprite[last].z;
                    curspr = last;
                }
                else if (numletters > 1)
                {
                    short sectolast = spritenums[numletters-2];

                    if (sprite[last].z == sprite[sectolast].z)
                        curspr = sectolast;
                    else  // if we delete the first letter on the line
                        curspr = linebegspr;

                    numletters--;
                    deletesprite(last);

                    updatenumsprites();
                    asksave = 1;
                }
                else
                {
                    numletters--;
                    deletesprite(last);
                    curspr = linebegspr;
                    updatenumsprites();
                    asksave = 1;
                }
            }
            else
            {
                sprite[linebegspr].z -= ((sprite[linebegspr].yrepeat*(vgap+tilesizy[basetile]))<<2);
                asksave = 1;
            }
        }
        else if (ch == 13)  // enter
        {
            sprite[linebegspr].z += ((sprite[linebegspr].yrepeat*(vgap+tilesizy[basetile]))<<2);
            curspr = linebegspr;
            doingspace = 0;
            asksave = 1;
        }
    }

ERROR_TOOMANYSPRITES:
    if (cursor < 0) message("Too many sprites in map!");
    else deletesprite(cursor);
    updatenumsprites();

ERROR_NOMEMORY:
    if (spritenums) Bfree(spritenums);
    else message("Out of memory!");

    clearkeys();

    lockclock = totalclock;  //Reset timing
}

static void Keys3d(void)
{
    int i,count,nexti,changedir;
    int j, k, tempint = 0, hiz, loz;
    int hihit, lohit;
    char smooshyalign=0, repeatpanalign=0, buffer[80];
    short startwall, endwall, dasector, statnum=0;
    char tempbuf[128];

    /* start Mapster32 */

    /*
    if (sidemode != 0)
    {
        setviewback();
        rotatesprite(320<<15,200<<15,65536,(horiz-100)<<2,4094,0,0,2+4,0,0,0,0);
        lockbyte4094 = 0;
        searchx = ydim-1-searchx;
        searchx ^= searchy;
        searchy ^= searchx;
        searchx ^= searchy;

        //      overwritesprite(160L,170L,1153,0,1+2,0);
        rotatesprite(160<<16,170<<16,65536,(100-horiz+1024)<<3,1153,0,0,2,0,0,0,0);

    }
    */

    if (usedcount && !helpon)
    {
        if (searchstat!=3)
        {
            count=0;
            for (i=0;i<numwalls;i++)
            {
                if (wall[i].picnum == temppicnum) count++;
                if (wall[i].overpicnum == temppicnum) count++;
            }
            for (i=0;i<numsectors;i++)
            {
                if (sector[i].ceilingpicnum == temppicnum) count++;
                if (sector[i].floorpicnum == temppicnum) count++;
            }
        }

        if (searchstat==3)
        {
            count=0;
            statnum=0;
            i = headspritestat[statnum];
            while (i != -1)
            {
                nexti = nextspritestat[i];
                if (sprite[i].picnum == temppicnum) count++;
                i = nexti;
            }
        }

        drawtileinfo("Clipboard",3,124,temppicnum,tempshade,temppal,tempcstat,templotag,temphitag,tempextra);
    }// end if usedcount

//    if (infobox&1)
    {
        char lines[8][64];
        int dax, day, dist, height1=0,height2=0,height3=0, num=0;
        int x,y;

        if (infobox&1)
        {
            height2=sector[searchsector].floorz-sector[searchsector].ceilingz;
            switch (searchstat)
            {
            case 0:
            case 4:
                drawtileinfo("Current",WIND1X,WIND1Y,wall[searchwall].picnum,wall[searchwall].shade,
                             wall[searchwall].pal,wall[searchwall].cstat,wall[searchwall].lotag,
                             wall[searchwall].hitag,wall[searchwall].extra);

                dax = wall[searchwall].x-wall[wall[searchwall].point2].x;
                day = wall[searchwall].y-wall[wall[searchwall].point2].y;
                dist = ksqrt(dax*dax+day*day);
                if (wall[searchwall].nextsector!=-1)
                {
                    int nextsect=wall[searchwall].nextsector;
                    height1=sector[searchsector].floorz-sector[nextsect].floorz;
                    height2=sector[nextsect].floorz-sector[nextsect].ceilingz;
                    height3=sector[nextsect].ceilingz-sector[searchsector].ceilingz;
                }
                Bsprintf(lines[num++],"Panning: %d, %d",wall[searchwall].xpanning,wall[searchwall].ypanning);
                Bsprintf(lines[num++],"Repeat:  %d, %d",wall[searchwall].xrepeat,wall[searchwall].yrepeat);
                Bsprintf(lines[num++],"Overpic: %d",wall[searchwall].overpicnum);
                lines[num++][0]=0;
                if (getmessageleng)
                    break;
                Bsprintf(lines[num++],"^251Wall %d^31",searchwall);
                if (wall[searchwall].nextsector!=-1)
                    Bsprintf(lines[num++],"LoHeight:%d, HiHeight:%d, Length:%d",height1,height3,dist);
                else
                    Bsprintf(lines[num++],"Height:%d, Length:%d",height2,dist);
                break;
            case 1:
                drawtileinfo("Current",WIND1X,WIND1Y,sector[searchsector].ceilingpicnum,sector[searchsector].ceilingshade,
                             sector[searchsector].ceilingpal,sector[searchsector].ceilingstat,
                             sector[searchsector].lotag,sector[searchsector].hitag,sector[searchsector].extra);

                Bsprintf(lines[num++],"Panning:  %d, %d",sector[searchsector].ceilingxpanning,sector[searchsector].ceilingypanning);
                Bsprintf(lines[num++],"CeilingZ: %d",sector[searchsector].ceilingz);
                Bsprintf(lines[num++],"Slope:    %d",sector[searchsector].ceilingheinum);
                lines[num++][0]=0;
                if (getmessageleng)
                    break;
                Bsprintf(lines[num++],"^251Sector %d^31 ceiling, Lotag:%s",searchsector,ExtGetSectorCaption(searchsector));
                Bsprintf(lines[num++],"Height: %d, Visibility:%d",height2,sector[searchsector].visibility);
                break;
            case 2:
                drawtileinfo("Current",WIND1X,WIND1Y,sector[searchsector].floorpicnum,sector[searchsector].floorshade,
                             sector[searchsector].floorpal,sector[searchsector].floorstat,
                             sector[searchsector].lotag,sector[searchsector].hitag,sector[searchsector].extra);

                Bsprintf(lines[num++],"Panning: %d,%d",sector[searchsector].floorxpanning,sector[searchsector].floorypanning);
                Bsprintf(lines[num++],"FloorZ:  %d",sector[searchsector].floorz);
                Bsprintf(lines[num++],"Slope:   %d",sector[searchsector].floorheinum);
                lines[num++][0]=0;
                if (getmessageleng)
                    break;
                Bsprintf(lines[num++],"^251Sector %d^31 floor, Lotag:%s",searchsector,ExtGetSectorCaption(searchsector));
                Bsprintf(lines[num++],"Height:%d, Visibility:%d",height2,sector[searchsector].visibility);
                break;
            case 3:
                drawtileinfo("Current",WIND1X,WIND1Y,sprite[searchwall].picnum,sprite[searchwall].shade,
                             sprite[searchwall].pal,sprite[searchwall].cstat,sprite[searchwall].lotag,
                             sprite[searchwall].hitag,sprite[searchwall].extra);

                Bsprintf(lines[num++],"Repeat:  %d,%d",sprite[searchwall].xrepeat,sprite[searchwall].yrepeat);
                Bsprintf(lines[num++],"PosXY:   %d,%d",sprite[searchwall].x,sprite[searchwall].y);
                Bsprintf(lines[num++],"PosZ: ""   %d",sprite[searchwall].z);// prevents tab character
                lines[num++][0]=0;

                if (getmessageleng)
                    break;
                if (strlen(names[sprite[searchwall].picnum]) > 0)
                {
                    if (sprite[searchwall].picnum==SECTOREFFECTOR)
                        Bsprintf(lines[num++],"^251Sprite %d^31 %s",searchwall,SectorEffectorText(searchwall));
                    else Bsprintf(lines[num++],"^251Sprite %d^31 %s",searchwall,names[sprite[searchwall].picnum]);
                }
                else Bsprintf(lines[num++],"^251Sprite %d^31, picnum %d",searchwall,sprite[searchwall].picnum);
                Bsprintf(lines[num++],"Elevation:%d",getflorzofslope(searchsector,sprite[searchwall].x,sprite[searchwall].y)-sprite[searchwall].z);
                break;
            }
        }
        x=WIND1X;y=WIND1Y;
        x=(int)(x*(xdimgame/320.));
        y=(int)(y*(ydimgame/200.));
        y+=(ydimgame>>6)*8;
        if (getmessageleng)
        {
            while (num < 4)
                lines[num++][0] = 0;
            Bsprintf(lines[num++],"^251%s",getmessage);
        }
        begindrawing();
        for (i=0;i<num;i++)
        {
            printext256(x+2,y+2,0,-1,lines[i],xdimgame>640?0:1);
            printext256(x,y,whitecol,-1,lines[i],xdimgame>640?0:1);
            y+=ydimgame>>6;
        }
        enddrawing();
    }

    if (keystatus[KEYSC_QUOTE] && keystatus[KEYSC_V]) // ' V
    {
        keystatus[KEYSC_V] = 0;
        switch (searchstat)
        {
        case 1:
        case 2:
            getnumberptr256("Sector visibility: ",&sector[searchsector].visibility,sizeof(sector[searchsector].visibility),256L,0,NULL);
            break;
        }
    }

    if (keystatus[KEYSC_SEMI] && keystatus[KEYSC_V])   // ; V
    {
        short currsector;
        unsigned char visval;

        keystatus[KEYSC_V] = 0;

        if (highlightsectorcnt == -1)
        {
            message("You didn't select any sectors!");
            return;
        }
        visval = (unsigned char)getnumber256("Visibility of selected sectors: ",sector[searchsector].visibility,256L,0);
        if (AskIfSure(0)) return;

        for (i = 0; i < highlightsectorcnt; i++)
        {
            currsector = highlightsector[i];
            sector[currsector].visibility = visval;
        }
        message("Visibility changed on all selected sectors");
    }

    if (keystatus[KEYSC_V])  //V
    {
        int oldtile;
        if (searchstat == 0) tempint = wall[searchwall].picnum;
        if (searchstat == 1) tempint = sector[searchsector].ceilingpicnum;
        if (searchstat == 2) tempint = sector[searchsector].floorpicnum;
        if (searchstat == 3) tempint = sprite[searchwall].picnum;
        if (searchstat == 4) tempint = wall[searchwall].overpicnum;
        oldtile = tempint;
        tempint = m32gettile(tempint);
        if (searchstat == 0) wall[searchwall].picnum = tempint;
        if (searchstat == 1) sector[searchsector].ceilingpicnum = tempint;
        if (searchstat == 2) sector[searchsector].floorpicnum = tempint;
        if (searchstat == 3) sprite[searchwall].picnum = tempint;
        if (searchstat == 4)
        {
            wall[searchwall].overpicnum = tempint;
            if (wall[searchwall].nextwall >= 0)
                wall[wall[searchwall].nextwall].overpicnum = tempint;
        }
        if (oldtile!=tempint)asksave = 1;
        keystatus[KEYSC_V] = 0;
    }

    if (keystatus[KEYSC_3])  /* 3 (toggle floor-over-floor (cduke3d only) */
    {
        floor_over_floor = !floor_over_floor;
        //        if (!floor_over_floor) ResetFOFSize();
        message("Floor-over-floor display %s",floor_over_floor?"enabled":"disabled");
        keystatus[KEYSC_3] = 0;
    }

    if (keystatus[KEYSC_F3])
    {
        mlook = 1-mlook;
        message("Mouselook: %s",mlook?"enabled":"disabled");
        keystatus[KEYSC_F3] = 0;
    }

// PK
    if (keystatus[KEYSC_F5])
    {
        unrealedlook = 1-unrealedlook;
        message("UnrealEd mouse navigation: %s",unrealedlook?"enabled":"disabled");
        keystatus[KEYSC_F5] = 0;
    }

    if (keystatus[KEYSC_QUOTE] && keystatus[KEYSC_BS]) // ' del
    {
        keystatus[KEYSC_BS] = 0;
        switch (searchstat)
        {
        case 0:
        case 4:
            wall[searchwall].cstat = 0;
            message("Wall %d cstat = 0",searchwall);
            break;
            //            case 1: case 2: sector[searchsector].cstat = 0; break;
        case 3:
            sprite[searchwall].cstat = 0;
            message("Sprite %d cstat = 0",searchwall);
            break;
        }
    }

    // 'P - Will copy palette to all sectors highlighted with R-Alt key
    if (keystatus[KEYSC_QUOTE] && keystatus[KEYSC_P])   // ' P
    {
        short w, start_wall, end_wall, currsector;
        signed char pal[4];

        keystatus[KEYSC_P] = 0;

        if (highlightsectorcnt == -1)
        {
            message("You didn't select any sectors!");
            return;
        }

        pal[0] = getnumber256("Ceiling palette: ",-1,MAXPALOOKUPS,1);
        pal[1] = getnumber256("Floor palette: ",-1,MAXPALOOKUPS,1);
        pal[2] = getnumber256("Wall palette: ",-1,MAXPALOOKUPS,1);
        pal[3] = getnumber256("Sprite palette: ",-1,MAXPALOOKUPS,1);
        if (AskIfSure(0)) return;

        for (i = 0; i < highlightsectorcnt; i++)
        {
            currsector = highlightsector[i];
            if (pal[0] > -1)
                sector[currsector].ceilingpal = pal[0];
            if (pal[1] > -1)
                sector[currsector].floorpal = pal[1];
            // Do all the walls in the sector
            start_wall = sector[currsector].wallptr;
            end_wall = start_wall + sector[currsector].wallnum;

            if (pal[2] > -1)
                for (w = start_wall; w < end_wall; w++)
                {
                    wall[w].pal = pal[2];
                }
            if (pal[3] > -1)
                for (k=0;k<highlightsectorcnt;k++)
                {
                    w = headspritesect[highlightsector[k]];
                    while (w >= 0)
                    {
                        j = nextspritesect[w];
                        sprite[w].pal = pal[3];
                        w = j;
                    }
                }
        }
        message("Palettes changed");
    }

    // ;P - Will copy palette to all sectors & sprites within highlighted with R-Alt key
    if (keystatus[KEYSC_SEMI] && keystatus[KEYSC_P])   // ; P
    {
        short w, start_wall, end_wall, currsector;
        unsigned char pal;

        keystatus[KEYSC_P] = 0;

        if (highlightsectorcnt == -1)
        {
            message("You didn't select any sectors!");
            return;
        }

        pal = (unsigned char)getnumber256("Global palette: ",0,MAXPALOOKUPS,0);
        if (AskIfSure(0)) return;

        for (i = 0; i < highlightsectorcnt; i++)
        {
            currsector = highlightsector[i];
            sector[currsector].ceilingpal = pal;
            sector[currsector].floorpal = pal;
            // Do all the walls in the sector
            start_wall = sector[currsector].wallptr;
            end_wall = start_wall + sector[currsector].wallnum;
            for (w = start_wall; w < end_wall; w++)
            {
                wall[w].pal = pal;
            }
            for (k=0;k<highlightsectorcnt;k++)
            {
                w = headspritesect[highlightsector[k]];
                while (w >= 0)
                {
                    j = nextspritesect[w];
                    sprite[w].pal = pal;
                    w = j;
                }
            }
        }
        message("Palettes changed");
    }

    if (keystatus[KEYSC_DELETE])
    {
        if (searchstat == 3)
        {
            deletesprite(searchwall);
            updatenumsprites();
            message("Sprite %d deleted",searchwall);
            asksave = 1;
        }
        keystatus[KEYSC_DELETE] = 0;
    }

    if (keystatus[KEYSC_F6])  //F6
    {
        keystatus[KEYSC_F6] = 0;
        autospritehelp=!autospritehelp;
        message("Automatic SECTOREFFECTOR help %s",autospritehelp?"enabled":"disabled");
    }
    if (keystatus[KEYSC_F7])  //F7
    {
        keystatus[KEYSC_F7] = 0;
        autosecthelp=!autosecthelp;
        message("Automatic sector tag help %s",autosecthelp?"enabled":"disabled");
    }

    if ((searchstat == 3) && (sprite[searchwall].picnum==SECTOREFFECTOR))
        if (autospritehelp && helpon==0) Show3dText("sehelp.hlp");

    if (searchstat == 1 || searchstat == 2)
        if (autosecthelp && helpon==0) Show3dText("sthelp.hlp");



    if (keystatus[KEYSC_COMMA]) // , Search & fix panning to the left (3D)
    {
        if (searchstat == 3)
        {
            i = searchwall;
            if (eitherSHIFT)
                sprite[i].ang = ((sprite[i].ang+2048-1)&2047);
            else
            {
                sprite[i].ang = ((sprite[i].ang+2048-128)&2047);
                keystatus[KEYSC_COMMA] = 0;
            }
            message("Sprite %d angle: %d",i,sprite[i].ang);
        }
    }
    if (keystatus[KEYSC_PERIOD]) // . Search & fix panning to the right (3D)
    {
        if ((searchstat == 0) || (searchstat == 4))
        {
            AutoAlignWalls((int)searchwall,0L);
            message("Wall %d autoalign",searchwall);
            keystatus[KEYSC_PERIOD] = 0;
        }
        if (searchstat == 3)
        {
            i = searchwall;
            if (eitherSHIFT)
                sprite[i].ang = ((sprite[i].ang+2048+1)&2047);
            else
            {
                sprite[i].ang = ((sprite[i].ang+2048+128)&2047);
                keystatus[KEYSC_PERIOD] = 0;
            }
            message("Sprite %d angle: %d",i,sprite[i].ang);
        }
    }

    if (keystatus[KEYSC_QUOTE] && keystatus[KEYSC_L]) // ' L
    {
        i = noclip;
        noclip = 1;
        switch (searchstat)
        {
        case 1:
            getnumberptr256("Sector ceilingz: ",&sector[searchsector].ceilingz,sizeof(sector[searchsector].ceilingz),8388608,1,NULL);
            if (!(sector[searchsector].ceilingstat&2))
            {
                sector[searchsector].ceilingstat |= 2;
                sector[searchsector].ceilingheinum = 0;
            }
            getnumberptr256("Sector ceiling slope: ",&sector[searchsector].ceilingheinum,sizeof(sector[searchsector].ceilingheinum),65536,1,NULL);
            break;
        case 2:
            getnumberptr256("Sector floorz: ",&sector[searchsector].floorz,sizeof(sector[searchsector].floorz),8388608,1,NULL);
            if (!(sector[searchsector].floorstat&2))
            {
                sector[searchsector].floorheinum = 0;
                sector[searchsector].floorstat |= 2;
            }
            getnumberptr256("Sector floor slope: ",&sector[searchsector].floorheinum,sizeof(sector[searchsector].floorheinum),65536,1,NULL);
            break;
        case 3:
            getnumberptr256("Sprite x: ",&sprite[searchwall].x,sizeof(sprite[searchwall].x),131072,1,NULL);
            getnumberptr256("Sprite y: ",&sprite[searchwall].y,sizeof(sprite[searchwall].y),131072,1,NULL);
            getnumberptr256("Sprite z: ",&sprite[searchwall].z,sizeof(sprite[searchwall].z),8388608,1,NULL);
            getnumberptr256("Sprite angle: ",&sprite[searchwall].ang,sizeof(sprite[searchwall].ang),2048L,0,NULL);
            break;
        }
        if (sector[searchsector].ceilingheinum == 0)
            sector[searchsector].ceilingstat &= ~2;
        else
            sector[searchsector].ceilingstat |= 2;

        if (sector[searchsector].floorheinum == 0)
            sector[searchsector].floorstat &= ~2;
        else
            sector[searchsector].floorstat |= 2;
        asksave = 1;
        keystatus[KEYSC_L] = 0;
        noclip = i;
    }

    getzrange(posx,posy,posz,cursectnum,&hiz,&hihit,&loz,&lohit,128L,CLIPMASK0);

    if (keystatus[KEYSC_CAPS] || (keystatus[KEYSC_QUOTE] && keystatus[KEYSC_Z]))
    {
        zmode++;
        if (zmode == 3) zmode = 0;
        else if (zmode == 1) zlock = (loz-posz)&0xfffffc00;
        switch (zmode)
        {
        case 0: message("Zmode = Gravity");break;
        case 1: message("Zmode = Locked/Sector");break;
        case 2: message("Zmode = Locked/Free");break;
        }
        keystatus[KEYSC_CAPS] = keystatus[KEYSC_Z] = 0;
    }

    if (keystatus[KEYSC_QUOTE] && keystatus[KEYSC_M])  // M
    {
        switch (searchstat)
        {
        case 0:
        case 4:
            strcpy(buffer,"Wall extra: ");
            wall[searchwall].extra = getnumber256(buffer,(int)wall[searchwall].extra,65536L,1);
            break;
        case 1:
        case 2:
            strcpy(buffer,"Sector extra: ");
            sector[searchsector].extra = getnumber256(buffer,(int)sector[searchsector].extra,65536L,1);
            break;
        case 3:
            strcpy(buffer,"Sprite extra: ");
            sprite[searchwall].extra = getnumber256(buffer,(int)sprite[searchwall].extra,65536L,1);
            break;
        }
        asksave = 1;
        keystatus[KEYSC_M] = 0;
    }

    if (keystatus[KEYSC_1])  // 1 (make 1-way wall)
    {
        if (searchstat != 3)
        {
            wall[searchwall].cstat ^= 32;
            sprintf(getmessage,"Wall %d one side masking bit %s",searchwall,wall[searchwall].cstat&32?"ON":"OFF");
            message(getmessage);
            asksave = 1;
        }
        else
        {
            sprite[searchwall].cstat ^= 64;
            i = sprite[searchwall].cstat;
            if ((i&48) == 32)
            {
                sprite[searchwall].cstat &= ~8;
                if ((i&64) > 0)
                    if (posz > sprite[searchwall].z)
                        sprite[searchwall].cstat |= 8;
            }
            asksave = 1;
            sprintf(getmessage,"Sprite %d one sided bit %s",searchwall,sprite[searchwall].cstat&64?"ON":"OFF");
            message(getmessage);

        }
        keystatus[KEYSC_1] = 0;
    }
    if (keystatus[KEYSC_2])  // 2 (bottom wall swapping)
    {
        if (searchstat != 3)
        {
            wall[searchwall].cstat ^= 2;
            sprintf(getmessage,"Wall %d bottom texture swap bit %s",searchwall,wall[searchwall].cstat&2?"ON":"OFF");
            message(getmessage);
            asksave = 1;
        }
        keystatus[KEYSC_2] = 0;
    }
    if (keystatus[KEYSC_O])  // O (top/bottom orientation - for doors)
    {
        if ((searchstat == 0) || (searchstat == 4))
        {
            wall[searchwall].cstat ^= 4;
            Bsprintf(getmessage,"Wall %d %s orientation",searchwall,wall[searchwall].cstat&4?"bottom":"top");
            message(getmessage);
            asksave = 1;
        }
        if (searchstat == 3)   // O (ornament onto wall) (2D)
        {
            asksave = 1;
            i = searchwall;

            DoSpriteOrnament(i);
            Bsprintf(getmessage,"Sprite %d ornament onto wall",i);
            message(getmessage);
        }
        keystatus[KEYSC_O] = 0;
    }
    if (keystatus[KEYSC_M])  // M (masking walls)
    {
        if (searchstat != 3)
        {
            i = wall[searchwall].nextwall;
            tempint = eitherSHIFT;
            if (i >= 0)
            {
                wall[searchwall].cstat ^= 16;
                sprintf(getmessage,"Wall %d masking bit %s",searchwall,wall[searchwall].cstat&16?"ON":"OFF");
                message(getmessage);
                if ((wall[searchwall].cstat&16) > 0)
                {
                    wall[searchwall].cstat &= ~8;
                    if (tempint == 0)
                    {
                        wall[i].cstat |= 8;           //auto other-side flip
                        wall[i].cstat |= 16;
                        wall[i].overpicnum = wall[searchwall].overpicnum;
                    }
                }
                else
                {
                    wall[searchwall].cstat &= ~8;
                    if (tempint == 0)
                    {
                        wall[i].cstat &= ~8;         //auto other-side unflip
                        wall[i].cstat &= ~16;
                    }
                }
                wall[searchwall].cstat &= ~32;
                if (tempint == 0) wall[i].cstat &= ~32;
                asksave = 1;
            }
        }
        keystatus[KEYSC_M] = 0;
    }

    if (keystatus[KEYSC_H])  // H (hitscan sensitivity)
    {
        if ((keystatus[KEYSC_QUOTE]))
        {
            switch (searchstat)
            {
            case 0:
            case 4:
                strcpy(buffer,"Wall hitag: ");
                wall[searchwall].hitag = getnumber256(buffer,(int)wall[searchwall].hitag,65536L,0);
                break;
            case 1:
                strcpy(buffer,"Sector hitag: ");
                sector[searchsector].hitag = getnumber256(buffer,(int)sector[searchsector].hitag,65536L,0);
                break;
            case 2:
                strcpy(buffer,"Sector hitag: ");
                sector[searchsector].hitag = getnumber256(buffer,(int)sector[searchsector].hitag,65536L,0);
                break;
            case 3:
                strcpy(buffer,"Sprite hitag: ");
                sprite[searchwall].hitag = getnumber256(buffer,(int)sprite[searchwall].hitag,65536L,0);
                break;
            }
        }

        else
        {

            if (searchstat == 3)
            {
                sprite[searchwall].cstat ^= 256;
                sprintf(getmessage,"Sprite %d hitscan sensitivity bit %s",searchwall,sprite[searchwall].cstat&256?"ON":"OFF");
                message(getmessage);
                asksave = 1;
            }
            else
            {
                wall[searchwall].cstat ^= 64;

                if ((wall[searchwall].nextwall >= 0) && (eitherSHIFT == 0))
                {
                    wall[wall[searchwall].nextwall].cstat &= ~64;
                    wall[wall[searchwall].nextwall].cstat |= (wall[searchwall].cstat&64);
                }
                sprintf(getmessage,"Wall %d hitscan sensitivity bit %s",searchwall,wall[searchwall].cstat&64?"ON":"OFF");
                message(getmessage);

                asksave = 1;
            }
        }
        keystatus[KEYSC_H] = 0;
    }

    smooshyalign = keystatus[KEYSC_gKP5];
    repeatpanalign = eitherSHIFT || (bstatus&2);

    if (mlook == 2)
        mlook = 0;

    if (!unrealedlook && (bstatus&4)) mlook = 2;

//    if (bstatus&4)
    if ((bstatus&(16|32) && !(bstatus&(1|2|4))) || keystatus[KEYSC_gMINUS] || keystatus[KEYSC_gPLUS])  // PK: no btn: wheel changes shade
    {
//        if (bstatus&1)
//        {
//            mlook = 2;
//        }
        if (bstatus&32 || keystatus[KEYSC_gMINUS])  // -
        {
            keystatus[KEYSC_gMINUS]=0;
            mouseb &= ~32;
            bstatus &= ~32;
            if (eitherALT)  //ALT
            {
                if (eitherCTRL)  //CTRL
                {
                    if (visibility < 16384) visibility += visibility;
                    Bsprintf(getmessage,"Global visibility %d",visibility);
                    message(getmessage);
                }
                else
                {
                    k=eitherSHIFT?1:16;

                    if (highlightsectorcnt >= 0)
                        for (i=0;i<highlightsectorcnt;i++)
                            if (highlightsector[i] == searchsector)
                            {
                                while (k > 0)
                                {
                                    for (i=0;i<highlightsectorcnt;i++)
                                    {
                                        sector[highlightsector[i]].visibility++;
                                        if (sector[highlightsector[i]].visibility == 240)
                                            sector[highlightsector[i]].visibility = 239;
                                    }
                                    k--;
                                }
                                break;
                            }
                    while (k > 0)
                    {
                        sector[searchsector].visibility++;
                        if (sector[searchsector].visibility == 240)
                            sector[searchsector].visibility = 239;
                        k--;
                    }
                    Bsprintf(getmessage,"Sector %d visibility %d",searchsector,sector[searchsector].visibility);
                    message(getmessage);
                    asksave = 1;
                }
            }
            else
            {
                k = 0;
                if (highlightsectorcnt >= 0)
                {
                    for (i=0;i<highlightsectorcnt;i++)
                        if (highlightsector[i] == searchsector)
                        {
                            k = 1;
                            break;
                        }
                }

                if (k == 0)
                {
                    int shade=-1, i=-1;
                    if (searchstat == 0) shade=++wall[i=searchwall].shade;
                    if (searchstat == 1) shade=++sector[i=searchsector].ceilingshade;
                    if (searchstat == 2) shade=++sector[i=searchsector].floorshade;
                    if (searchstat == 3) shade=++sprite[i=searchwall].shade;
                    if (searchstat == 4) shade=++wall[i=searchwall].shade;
                    if (i!=-1)
                    {
                        Bsprintf(getmessage,"%s %d shade %d",type2str[searchstat],i,shade);
                        message(getmessage);
                    }
                }
                else
                {
                    for (i=0;i<highlightsectorcnt;i++)
                    {
                        dasector = highlightsector[i];

                        sector[dasector].ceilingshade++;        //sector shade
                        sector[dasector].floorshade++;

                        startwall = sector[dasector].wallptr;   //wall shade
                        endwall = startwall + sector[dasector].wallnum - 1;
                        for (j=startwall;j<=endwall;j++)
                            wall[j].shade++;

                        j = headspritesect[dasector];           //sprite shade
                        while (j != -1)
                        {
                            sprite[j].shade++;
                            j = nextspritesect[j];
                        }
                    }
                }
                asksave = 1;
            }
        }
        if (bstatus&16 || keystatus[KEYSC_gPLUS])  // +
        {
            keystatus[KEYSC_gPLUS]=0;
            mouseb &= ~16;
            bstatus &= ~16;
            if (eitherALT)  //ALT
            {
                if (eitherCTRL)  //CTRL
                {
                    if (visibility > 32) visibility >>= 1;
                    Bsprintf(getmessage,"Global visibility %d",visibility);
                    message(getmessage);
                }
                else
                {
                    k=eitherSHIFT?1:16;

                    if (highlightsectorcnt >= 0)
                        for (i=0;i<highlightsectorcnt;i++)
                            if (highlightsector[i] == searchsector)
                            {
                                while (k > 0)
                                {
                                    for (i=0;i<highlightsectorcnt;i++)
                                    {
                                        sector[highlightsector[i]].visibility--;
                                        if (sector[highlightsector[i]].visibility == 239)
                                            sector[highlightsector[i]].visibility = 240;
                                    }
                                    k--;
                                }
                                break;
                            }
                    while (k > 0)
                    {
                        sector[searchsector].visibility--;
                        if (sector[searchsector].visibility == 239)
                            sector[searchsector].visibility = 240;
                        k--;
                    }
                    Bsprintf(getmessage,"Sector %d visibility %d",searchsector,sector[searchsector].visibility);
                    message(getmessage);
                    asksave = 1;
                }
            }
            else
            {
                k = 0;
                if (highlightsectorcnt >= 0)
                {
                    for (i=0;i<highlightsectorcnt;i++)
                        if (highlightsector[i] == searchsector)
                        {
                            k = 1;
                            break;
                        }
                }

                if (k == 0)
                {
                    int shade=-1, i=-1;
                    if (searchstat == 0) shade=--wall[i=searchwall].shade;
                    if (searchstat == 1) shade=--sector[i=searchsector].ceilingshade;
                    if (searchstat == 2) shade=--sector[i=searchsector].floorshade;
                    if (searchstat == 3) shade=--sprite[i=searchwall].shade;
                    if (searchstat == 4) shade=--wall[i=searchwall].shade;
                    if (i!=-1)
                    {
                        Bsprintf(getmessage,"%s %d shade %d",type2str[searchstat],i,shade);
                        message(getmessage);
                    }
                }
                else
                {
                    for (i=0;i<highlightsectorcnt;i++)
                    {
                        dasector = highlightsector[i];

                        sector[dasector].ceilingshade--;        //sector shade
                        sector[dasector].floorshade--;

                        startwall = sector[dasector].wallptr;   //wall shade
                        endwall = startwall + sector[dasector].wallnum - 1;
                        for (j=startwall;j<=endwall;j++)
                            wall[j].shade--;

                        j = headspritesect[dasector];           //sprite shade
                        while (j != -1)
                        {
                            sprite[j].shade--;
                            j = nextspritesect[j];
                        }
                    }
                }
                asksave = 1;
            }
        }
    }

//    if ((keystatus[KEYSC_DASH]|keystatus[KEYSC_EQUAL]|((bstatus&(16|32)) && !(bstatus&2))) > 0) // mousewheel, -, and +, cycle picnum
    if (keystatus[KEYSC_DASH] | keystatus[KEYSC_EQUAL] | (bstatus&(16|32) && (bstatus&1) && !(bstatus&2)))  // PK: lmb only & mousewheel, -, and +, cycle picnum

    {
        j = i = (keystatus[KEYSC_EQUAL] || (bstatus&16))?1:-1;
        switch (searchstat)
        {
        case 0:
            while (!tilesizx[wall[searchwall].picnum]||!tilesizy[wall[searchwall].picnum]||j)
            {
                if (wall[searchwall].picnum+i >= MAXTILES) wall[searchwall].picnum = 0;
                else if (wall[searchwall].picnum+i < 0) wall[searchwall].picnum = MAXTILES-1;
                else wall[searchwall].picnum += i;
                j = 0;
            }
            break;
        case 1:
            while (!tilesizx[sector[searchsector].ceilingpicnum]||!tilesizy[sector[searchsector].ceilingpicnum]||j)
            {
                if (sector[searchsector].ceilingpicnum+i >= MAXTILES) sector[searchsector].ceilingpicnum = 0;
                else if (sector[searchsector].ceilingpicnum+i < 0) sector[searchsector].ceilingpicnum = MAXTILES-1;
                else sector[searchsector].ceilingpicnum += i;
                j = 0;
            }
            break;
        case 2:
            while (!tilesizx[sector[searchsector].floorpicnum]||!tilesizy[sector[searchsector].floorpicnum]||j)
            {
                if (sector[searchsector].floorpicnum+i >= MAXTILES) sector[searchsector].floorpicnum = 0;
                else if (sector[searchsector].floorpicnum+i < 0) sector[searchsector].floorpicnum = MAXTILES-1;
                else sector[searchsector].floorpicnum += i;
                j = 0;
            }
            break;
        case 3:
            while (!tilesizx[sprite[searchwall].picnum]||!tilesizy[sprite[searchwall].picnum]||j)
            {
                if (sprite[searchwall].picnum+i >= MAXTILES) sprite[searchwall].picnum = 0;
                else if (sprite[searchwall].picnum+i < 0) sprite[searchwall].picnum = MAXTILES-1;
                else sprite[searchwall].picnum += i;
                j = 0;
            }
            break;
        case 4:
            while (!tilesizx[wall[searchwall].overpicnum]||!tilesizy[wall[searchwall].overpicnum]||j)
            {
                if (wall[searchwall].overpicnum+i >= MAXTILES) wall[searchwall].overpicnum = 0;
                else if (wall[searchwall].overpicnum+i < 0) wall[searchwall].overpicnum = MAXTILES-1;
                else wall[searchwall].overpicnum += i;
                j = 0;
            }
            break;
        }
        asksave = 1;
        keystatus[KEYSC_DASH] = keystatus[KEYSC_EQUAL] = 0;
        mouseb &= ~(16|32);
    }

    if (keystatus[KEYSC_E])  // E (expand)
    {
        if (searchstat == 1)
        {
            sector[searchsector].ceilingstat ^= 8;
            sprintf(getmessage,"Sector %d ceiling texture expansion bit %s",searchsector,sector[searchsector].ceilingstat&8?"ON":"OFF");
            message(getmessage);
            asksave = 1;
        }
        if (searchstat == 2)
        {
            sector[searchsector].floorstat ^= 8;
            sprintf(getmessage,"Sector %d floor texture expansion bit %s",searchsector,sector[searchsector].floorstat&8?"ON":"OFF");
            message(getmessage);
            asksave = 1;
        }
        keystatus[KEYSC_E] = 0;
    }
    if (keystatus[KEYSC_R])  // R (relative alignment, rotation)
    {

        if (keystatus[KEYSC_QUOTE]) // FRAMERATE TOGGLE
        {

            framerateon = !framerateon;
            if (framerateon) message("Show framerate ON");
            else message("Show framerate OFF");

        }

        else

        {
            if (searchstat == 1)
            {
                sector[searchsector].ceilingstat ^= 64;
                sprintf(getmessage,"Sector %d ceiling texture relativity bit %s",searchsector,sector[searchsector].ceilingstat&64?"ON":"OFF");
                message(getmessage);
                asksave = 1;
            }
            if (searchstat == 2)
            {
                sector[searchsector].floorstat ^= 64;
                sprintf(getmessage,"Sector %d floor texture relativity bit %s",searchsector,sector[searchsector].floorstat&64?"ON":"OFF"); //PK (was ceiling in string)
                message(getmessage);
                asksave = 1;
            }
            if (searchstat == 3)
            {
                i = sprite[searchwall].cstat;
                if ((i&48) < 32) i += 16;

                else i &= ~48;
                sprite[searchwall].cstat = i;

                if (sprite[searchwall].cstat&16)
                    sprintf(getmessage,"Sprite %d now wall aligned",searchwall);
                else if (sprite[searchwall].cstat&32)
                    sprintf(getmessage,"Sprite %d now floor aligned",searchwall);
                else
                    sprintf(getmessage,"Sprite %d now view aligned",searchwall);
                message(getmessage);
                asksave = 1;
            }
        }
        keystatus[KEYSC_R] = 0;
    }
    if (keystatus[KEYSC_F])  //F (Flip)
    {
        keystatus[KEYSC_F] = 0;
        if (eitherALT)  //ALT-F (relative alignmment flip)
        {
            if (searchstat != 3)
            {
                setfirstwall(searchsector,searchwall);
                Bsprintf(getmessage,"Sector %d first wall",searchsector);
                message(getmessage);
                asksave = 1;
            }
        }
        else
        {
            if ((searchstat == 0) || (searchstat == 4))
            {
                i = wall[searchwall].cstat;
                i = ((i>>3)&1)+((i>>7)&2);    //3-x,8-y
                switch (i)
                {
                case 0:
                    i = 1;
                    break;
                case 1:
                    i = 3;
                    break;
                case 2:
                    i = 0;
                    break;
                case 3:
                    i = 2;
                    break;
                }
                Bsprintf(getmessage,"Wall %d flip %d",searchwall,i);
                message(getmessage);
                i = ((i&1)<<3)+((i&2)<<7);
                wall[searchwall].cstat &= ~0x0108;
                wall[searchwall].cstat |= i;
                asksave = 1;
            }
            if (searchstat == 1)         //8-way ceiling flipping (bits 2,4,5)
            {
                i = sector[searchsector].ceilingstat;
                i = (i&0x4)+((i>>4)&3);
                switch (i)
                {
                case 0:
                    i = 6;
                    break;
                case 6:
                    i = 3;
                    break;
                case 3:
                    i = 5;
                    break;
                case 5:
                    i = 1;
                    break;
                case 1:
                    i = 7;
                    break;
                case 7:
                    i = 2;
                    break;
                case 2:
                    i = 4;
                    break;
                case 4:
                    i = 0;
                    break;
                }
                Bsprintf(getmessage,"Sector %d flip %d",searchsector,i);
                message(getmessage);
                i = (i&0x4)+((i&3)<<4);
                sector[searchsector].ceilingstat &= ~0x34;
                sector[searchsector].ceilingstat |= i;
                asksave = 1;
            }
            if (searchstat == 2)         //8-way floor flipping (bits 2,4,5)
            {
                i = sector[searchsector].floorstat;
                i = (i&0x4)+((i>>4)&3);
                switch (i)
                {
                case 0:
                    i = 6;
                    break;
                case 6:
                    i = 3;
                    break;
                case 3:
                    i = 5;
                    break;
                case 5:
                    i = 1;
                    break;
                case 1:
                    i = 7;
                    break;
                case 7:
                    i = 2;
                    break;
                case 2:
                    i = 4;
                    break;
                case 4:
                    i = 0;
                    break;
                }
                Bsprintf(getmessage,"Sector %d flip %d",searchsector,i);
                message(getmessage);
                i = (i&0x4)+((i&3)<<4);
                sector[searchsector].floorstat &= ~0x34;
                sector[searchsector].floorstat |= i;
                asksave = 1;
            }
            if (searchstat == 3)
            {
                i = sprite[searchwall].cstat;
                if (((i&48) == 32) && ((i&64) == 0))
                {
                    sprite[searchwall].cstat &= ~0xc;
                    sprite[searchwall].cstat |= ((i&4)^4);
                    Bsprintf(getmessage,"Sprite %d flip bit %s",searchwall,sprite[searchwall].cstat&4?"ON":"OFF");
                    message(getmessage);
                }
                else
                {
                    i = ((i>>2)&3);
                    switch (i)
                    {
                    case 0:
                        i = 1;
                        break;
                    case 1:
                        i = 3;
                        break;
                    case 2:
                        i = 0;
                        break;
                    case 3:
                        i = 2;
                        break;
                    }
                    Bsprintf(getmessage,"Sprite %d flip %d",searchwall,i);
                    message(getmessage);
                    i <<= 2;
                    sprite[searchwall].cstat &= ~0xc;
                    sprite[searchwall].cstat |= i;
                }
                asksave = 1;
            }
        }
    }

    if (keystatus[KEYSC_HOME])
        updownunits = 256;
    else if (keystatus[KEYSC_END])
        updownunits = 512;
    else
        updownunits = 1024;
    mouseaction=0;
    if (eitherALT && bstatus&1)
    {
        mousex=0;mskip=1;
        if (mousey<0)
        {
            updownunits=klabs(mousey*128);
            mouseaction=1;
        }
    }
    if (keystatus[KEYSC_PGUP] || mouseaction || ((bstatus&2) && (bstatus&16 && !(bstatus&1)))) // PK: PGUP, rmb only & mwheel
    {
        k = 0;
        if (highlightsectorcnt >= 0)
        {
            for (i=0;i<highlightsectorcnt;i++)
                if (highlightsector[i] == searchsector)
                {
                    k = 1;
                    break;
                }
        }

        if ((searchstat == 0) || (searchstat == 1))
        {
            if (k == 0)
            {
                i = headspritesect[searchsector];
                while (i != -1)
                {
                    tempint = getceilzofslope(searchsector,sprite[i].x,sprite[i].y);
                    tempint += ((tilesizy[sprite[i].picnum]*sprite[i].yrepeat)<<2);
                    if (sprite[i].cstat&128) tempint += ((tilesizy[sprite[i].picnum]*sprite[i].yrepeat)<<1);
                    if (sprite[i].z == tempint)
                        sprite[i].z -= updownunits << (eitherCTRL<<1);   // JBF 20031128
                    i = nextspritesect[i];
                }
                sector[searchsector].ceilingz -= updownunits << (eitherCTRL<<1); // JBF 20031128
                sprintf(getmessage,"Sector %d ceilingz = %d",searchsector,sector[searchsector].ceilingz);
                message(getmessage);

            }
            else
            {
                for (j=0;j<highlightsectorcnt;j++)
                {
                    i = headspritesect[highlightsector[j]];
                    while (i != -1)
                    {
                        tempint = getceilzofslope(highlightsector[j],sprite[i].x,sprite[i].y);
                        tempint += ((tilesizy[sprite[i].picnum]*sprite[i].yrepeat)<<2);
                        if (sprite[i].cstat&128) tempint += ((tilesizy[sprite[i].picnum]*sprite[i].yrepeat)<<1);
                        if (sprite[i].z == tempint)
                            sprite[i].z -= updownunits << (eitherCTRL<<1);   // JBF 20031128
                        i = nextspritesect[i];
                    }
                    sector[highlightsector[j]].ceilingz -= updownunits << (eitherCTRL<<1);   // JBF 20031128
                    sprintf(getmessage,"Sector %d ceilingz = %d",*highlightsector,sector[highlightsector[j]].ceilingz);
                    message(getmessage);

                }
            }
        }
        if (searchstat == 2)
        {
            if (k == 0)
            {
                i = headspritesect[searchsector];
                while (i != -1)
                {
                    tempint = getflorzofslope(searchsector,sprite[i].x,sprite[i].y);
                    if (sprite[i].cstat&128) tempint += ((tilesizy[sprite[i].picnum]*sprite[i].yrepeat)<<1);
                    if (sprite[i].z == tempint)
                        sprite[i].z -= updownunits << (eitherCTRL<<1);   // JBF 20031128
                    i = nextspritesect[i];
                }
                sector[searchsector].floorz -= updownunits << (eitherCTRL<<1);   // JBF 20031128
                sprintf(getmessage,"Sector %d floorz = %d",searchsector,sector[searchsector].floorz);
                message(getmessage);

            }
            else
            {
                for (j=0;j<highlightsectorcnt;j++)
                {
                    i = headspritesect[highlightsector[j]];
                    while (i != -1)
                    {
                        tempint = getflorzofslope(highlightsector[j],sprite[i].x,sprite[i].y);
                        if (sprite[i].cstat&128) tempint += ((tilesizy[sprite[i].picnum]*sprite[i].yrepeat)<<1);
                        if (sprite[i].z == tempint)
                            sprite[i].z -= updownunits << (eitherCTRL<<1);   // JBF 20031128
                        i = nextspritesect[i];
                    }
                    sector[highlightsector[j]].floorz -= updownunits << (eitherCTRL<<1); // JBF 20031128
                    sprintf(getmessage,"Sector %d floorz = %d",*highlightsector,sector[highlightsector[j]].floorz);
                    message(getmessage);

                }
            }

        }
        if (sector[searchsector].floorz < sector[searchsector].ceilingz)
            sector[searchsector].floorz = sector[searchsector].ceilingz;
        if (searchstat == 3)
        {
            if (eitherCTRL)  //CTRL - put sprite on ceiling
            {
                sprite[searchwall].z = spriteonceilingz(searchwall);
            }
            else
            {
                k = 0;
                if (highlightcnt >= 0)
                    for (i=0;i<highlightcnt;i++)
                        if (highlight[i] == searchwall+16384)
                        {
                            k = 1;
                            break;
                        }

                if (k == 0)
                {
                    sprite[searchwall].z -= updownunits;
                    if (!spnoclip)sprite[searchwall].z = max(sprite[searchwall].z,spriteonceilingz(searchwall));
                    sprintf(getmessage,"Sprite %d z = %d",searchwall,sprite[searchwall].z);
                    message(getmessage);

                }
                else
                {
                    for (i=0;i<highlightcnt;i++)
                        if ((highlight[i]&0xc000) == 16384)
                        {
                            sprite[highlight[i]&16383].z -= updownunits;
                            if (!spnoclip)sprite[highlight[i]&16383].z = max(sprite[highlight[i]&16383].z,spriteonceilingz(highlight[i]&16383));
                        }
                    sprintf(getmessage,"Sprite %d z = %d",highlight[i]&16383,sprite[highlight[i]&16383].z);
                    message(getmessage);

                }
            }
        }
        asksave = 1;
        keystatus[KEYSC_PGUP] = 0;
        mouseb &= ~16;
    }

    mouseaction=0;
    if (eitherALT && bstatus&1)
    {
        mousex=0;mskip=1;
        if (mousey>0)
        {
            updownunits=klabs(mousey*128);
            mouseaction=1;
        }
    }
    if (keystatus[KEYSC_PGDN] || mouseaction || ((bstatus&2) && (bstatus&32) && !(bstatus&1))) // PK: PGDN, rmb only & mwheel
    {
        k = 0;
        if (highlightsectorcnt >= 0)
        {
            for (i=0;i<highlightsectorcnt;i++)
                if (highlightsector[i] == searchsector)
                {
                    k = 1;
                    break;
                }
        }

        if ((searchstat == 0) || (searchstat == 1))
        {
            if (k == 0)
            {
                i = headspritesect[searchsector];
                while (i != -1)
                {
                    tempint = getceilzofslope(searchsector,sprite[i].x,sprite[i].y);
                    if (sprite[i].cstat&128) tempint += ((tilesizy[sprite[i].picnum]*sprite[i].yrepeat)<<1);
                    tempint += ((tilesizy[sprite[i].picnum]*sprite[i].yrepeat)<<2);
                    if (sprite[i].z == tempint)
                        sprite[i].z += updownunits << (eitherCTRL<<1);   // JBF 20031128
                    i = nextspritesect[i];
                }
                sector[searchsector].ceilingz += updownunits << (eitherCTRL<<1); // JBF 20031128
                sprintf(getmessage,"Sector %d ceilingz = %d",searchsector,sector[searchsector].ceilingz);
                message(getmessage);

            }
            else
            {
                for (j=0;j<highlightsectorcnt;j++)
                {
                    i = headspritesect[highlightsector[j]];
                    while (i != -1)
                    {
                        tempint = getceilzofslope(highlightsector[j],sprite[i].x,sprite[i].y);
                        if (sprite[i].cstat&128) tempint += ((tilesizy[sprite[i].picnum]*sprite[i].yrepeat)<<1);
                        tempint += ((tilesizy[sprite[i].picnum]*sprite[i].yrepeat)<<2);
                        if (sprite[i].z == tempint)
                            sprite[i].z += updownunits << (eitherCTRL<<1);   // JBF 20031128
                        i = nextspritesect[i];
                    }
                    sector[highlightsector[j]].ceilingz += updownunits << (eitherCTRL<<1);   // JBF 20031128
                    sprintf(getmessage,"Sector %d ceilingz = %d",*highlightsector,sector[highlightsector[j]].ceilingz);
                    message(getmessage);

                }
            }
        }
        if (searchstat == 2)
        {
            if (k == 0)
            {
                i = headspritesect[searchsector];
                while (i != -1)
                {
                    tempint = getflorzofslope(searchsector,sprite[i].x,sprite[i].y);
                    if (sprite[i].cstat&128) tempint += ((tilesizy[sprite[i].picnum]*sprite[i].yrepeat)<<1);
                    if (sprite[i].z == tempint)
                        sprite[i].z += updownunits << (eitherCTRL<<1);   // JBF 20031128
                    i = nextspritesect[i];
                }
                sector[searchsector].floorz += updownunits << (eitherCTRL<<1);   // JBF 20031128
                sprintf(getmessage,"Sector %d floorz = %d",searchsector,sector[searchsector].floorz);
                message(getmessage);

            }
            else
            {
                for (j=0;j<highlightsectorcnt;j++)
                {
                    i = headspritesect[highlightsector[j]];
                    while (i != -1)
                    {
                        tempint = getflorzofslope(highlightsector[j],sprite[i].x,sprite[i].y);
                        if (sprite[i].cstat&128) tempint += ((tilesizy[sprite[i].picnum]*sprite[i].yrepeat)<<1);
                        if (sprite[i].z == tempint)
                            sprite[i].z += updownunits << (eitherCTRL<<1);   // JBF 20031128
                        i = nextspritesect[i];
                    }
                    sector[highlightsector[j]].floorz += updownunits << (eitherCTRL<<1); // JBF 20031128
                    sprintf(getmessage,"Sector %d floorz = %d",*highlightsector,sector[highlightsector[j]].floorz);
                    message(getmessage);

                }
            }
        }
        if (sector[searchsector].ceilingz > sector[searchsector].floorz)
            sector[searchsector].ceilingz = sector[searchsector].floorz;
        if (searchstat == 3)
        {
            if (eitherCTRL)  //CTRL - put sprite on ground
            {
                sprite[searchwall].z = spriteongroundz(searchwall);
            }
            else
            {
                k = 0;
                if (highlightcnt >= 0)
                    for (i=0;i<highlightcnt;i++)
                        if (highlight[i] == searchwall+16384)
                        {
                            k = 1;
                            break;
                        }

                if (k == 0)
                {
                    sprite[searchwall].z += updownunits;
                    if (!spnoclip)sprite[searchwall].z = min(sprite[searchwall].z,spriteongroundz(searchwall));
                    sprintf(getmessage,"Sprite %d z = %d",searchwall,sprite[searchwall].z);
                    message(getmessage);

                }
                else
                {
                    for (i=0;i<highlightcnt;i++)
                        if ((highlight[i]&0xc000) == 16384)
                        {
                            sprite[highlight[i]&16383].z += updownunits;
                            if (!spnoclip)sprite[highlight[i]&16383].z = min(sprite[highlight[i]&16383].z,spriteongroundz(highlight[i]&16383));
                        }
                    sprintf(getmessage,"Sprite %d z = %d",highlight[i]&16383,sprite[highlight[i]&16383].z);
                    message(getmessage);

                }
            }
        }
        asksave = 1;
        keystatus[KEYSC_PGDN] = 0;
        mouseb &= ~32;
    }

    /* end Mapster32 */

    //  DoWater(horiz);

    if (framerateon)
    {
        static int FrameCount = 0;
        static int LastCount = 0;
        static int LastSec = 0;
        static int LastMS = 0;
        int ms = getticks();
        int howlong = ms - LastMS;
        if (howlong >= 0)
        {
            int thisSec = ms/1000;
            int x = (xdim <= 640);
            int chars = Bsprintf(tempbuf, "%2u ms (%3u fps)", howlong, LastCount);

            if (!x)
            {
                printext256(windowx2-(chars<<3)+1,windowy1+2,0,-1,tempbuf,x);
                printext256(windowx2-(chars<<3),windowy1+1,COLOR_WHITE,-1,tempbuf,x);
            }
            else
            {
                printext256(windowx2-(chars<<2)+1,windowy1+2,0,-1,tempbuf,x);
                printext256(windowx2-(chars<<2),windowy1+1,COLOR_WHITE,-1,tempbuf,x);
            }

            if (LastSec < thisSec)
            {
                LastCount = FrameCount / (thisSec - LastSec);
                LastSec = thisSec;
                FrameCount = 0;
            }
            FrameCount++;
        }
        LastMS = ms;
    }

    tempbuf[0] = 0;
    if (bstatus&4 && !(bstatus&(1|2)) && !unrealedlook)  //PK
    {
        Bsprintf(tempbuf,"VIEW");
//        else Bsprintf(tempbuf,"SHADE");
    }
    else if (bstatus&2 && !(bstatus&1))
        Bsprintf(tempbuf,"Z");
    else if (bstatus&1 && !(bstatus&2))
        Bsprintf(tempbuf,"LOCK");

    if (bstatus&1)
    {
        Bsprintf(tempbuf,"LOCK");
        switch (searchstat)
        {
        case 0:
        case 4:
            if (eitherSHIFT) Bsprintf(tempbuf,"PAN");
            if (eitherCTRL)  Bsprintf(tempbuf,"SCALE");
            if (eitherALT)   Bsprintf(tempbuf,"Z");
            break;
        case 1:
        case 2:
            if (eitherSHIFT) Bsprintf(tempbuf,"PAN");
            if (eitherCTRL)  Bsprintf(tempbuf,"SLOPE");
            if (eitherALT)   Bsprintf(tempbuf,"Z");
            break;
        case 3:
            if (eitherSHIFT) Bsprintf(tempbuf,"MOVE XY");
            if (eitherCTRL)  Bsprintf(tempbuf,"SIZE");
            if (eitherALT)   Bsprintf(tempbuf,"MOVE Z");
            break;
        }
    }

    if (tempbuf[0] != 0)
    {
        i = (Bstrlen(tempbuf)<<3)+6;
        if ((searchx+i) < (xdim-1))
            i = 0;
        else i = (searchx+i)-(xdim-1);
        if ((searchy+16) < (ydim-1))
            j = 0;
        else j = (searchy+16)-(ydim-1);
        //            printext16(searchx+6-i,searchy+6-j,11,-1,tempbuf,0);
        printext256(searchx+4+2-i,searchy+4+2-j,0,-1,tempbuf,!(xdimgame > 640));
        printext256(searchx+4-i,searchy+4-j,whitecol,-1,tempbuf,!(xdimgame > 640));

        //        printext256(searchx+4+2,searchy+4+2,0,-1,tempbuf,!(xdimgame > 640));
        //        printext256(searchx+4,searchy+4,whitecol,-1,tempbuf,!(xdimgame > 640));
    }
    if (helpon==1)
    {
        for (i=0;i<MAXHELP3D;i++)
        {
            begindrawing();
            printext256(0*8+2,8+(i*(8+(xdimgame > 640)))+2,0,-1,Help3d[i],!(xdimgame > 640));
            printext256(0*8,8+(i*(8+(xdimgame > 640))),whitecol,-1,Help3d[i],!(xdimgame > 640));
            enddrawing();
            switch (i)
            {
            case 8:
                Bsprintf(tempbuf,"%d",autosave);
                break;
            case 9:
                Bsprintf(tempbuf,"%s",SKILLMODE[skill]);
                break;
            case 10:
                Bsprintf(tempbuf,"%d",framerateon);
                break;
            case 11:
                Bsprintf(tempbuf,"%s",SPRDSPMODE[nosprites]);
                break;
            case 12:
                Bsprintf(tempbuf,"%d",shadepreview);
                break;
            case 13:
                Bsprintf(tempbuf,"%d",purpleon);
                break;
            default :
                sprintf(tempbuf," ");
                break;
            }
            begindrawing();
            if (!strcmp(tempbuf,"0"))
                Bsprintf(tempbuf,"OFF");
            else if (!strcmp(tempbuf,"1"))
                Bsprintf(tempbuf,"ON");
            else if (!strcmp(tempbuf,"2"))
                Bsprintf(tempbuf,"ON (2)");

            printext256((20+((xdimgame > 640) * 20))*8+2,8+(i*(8+(xdimgame > 640)))+2,0,-1,tempbuf,!(xdimgame > 640));
            printext256((20+((xdimgame > 640) * 20))*8,8+(i*(8+(xdimgame > 640))),whitecol,-1,tempbuf,!(xdimgame > 640));
            enddrawing();
        }
    }

    /* if(purpleon) {
                begindrawing();
    //          printext256(1*4,1*8,whitecol,-1,"Purple ON",0);
                    sprintf(getmessage,"Purple ON");
                    message(getmessage);
                enddrawing();
                }
    */
    if (sector[cursectnum].lotag==2)
    {
        if (sector[cursectnum].ceilingpicnum==FLOORSLIME) SetSLIMEPalette();
        else SetWATERPalette();
    }
    else SetGAMEPalette();

    if (keystatus[buildkeys[BK_MODE2D_3D]])  // Enter
        SetGAMEPalette();

    //Stick this in 3D part of ExtCheckKeys
    //Also choose your own key scan codes



    if (keystatus[KEYSC_QUOTE] && keystatus[KEYSC_D]) // ' d
        /*
            {
                ShowHelpText("SectorEffector");
            } */

    {
        keystatus[KEYSC_D] = 0;
        skill++;
        if (skill>MAXSKILL-1) skill=0;
        message("%s",SKILLMODE[skill]);
        //        printext256(1*4,1*8,11,-1,tempbuf,0);
    }

    /*    if (keystatus[KEYSC_QUOTE] && keystatus[KEYSC_G]) // ' g <Unused>
        {
            keystatus[KEYSC_G] = 0;
            tabgraphic++;
            if (tabgraphic > 2) tabgraphic = 0;
            if (tabgraphic) message("Graphics ON");
            else message("Graphics OFF");
    	}*/

    if (keystatus[KEYSC_QUOTE] && keystatus[KEYSC_X]) // ' x
    {
        keystatus[KEYSC_X] = 0;
        shadepreview=!shadepreview;
        if (shadepreview) message("Map shade preview ON");
        else message("Map shade preview OFF");
    }


    /*    if (keystatus[KEYSC_QUOTE] && keystatus[KEYSC_R]) // ' r <Handled already>
        {
            keystatus[KEYSC_R] = 0;
            framerateon=!framerateon;
            if (framerateon) message("Framerate ON");
            else message("Framerate OFF");
    	}*/

    if (keystatus[KEYSC_QUOTE] && keystatus[KEYSC_W]) // ' w
    {
        keystatus[KEYSC_W] = 0;
        nosprites++;
        if (nosprites>3) nosprites=0;
        message("%s",SPRDSPMODE[nosprites]);
        //        printext256(1*4,1*8,whitecol,-1,tempbuf,0);
    }

    if (keystatus[KEYSC_QUOTE] && keystatus[KEYSC_Y]) // ' y
    {
        keystatus[KEYSC_Y] = 0;
        purpleon=!purpleon;
        if (nosprites>3) nosprites=0;
        if (purpleon) message("Purple ON");
        else message("Purple OFF");
    }

    if (keystatus[KEYSC_QUOTE] && keystatus[KEYSC_C]) // ' C
    {
        keystatus[KEYSC_C] = 0;
        switch (searchstat)
        {
        case 0:
        case 4:
            for (i=0;i<MAXWALLS;i++)
            {
                if (wall[i].picnum==temppicnum)
                    wall[i].shade=tempshade;
            }
            message("Walls with picnum %d have shade of %d",temppicnum,tempshade);
            asksave=1;
            break;
        case 1:
        case 2:
            for (i=0;i<MAXSECTORS;i++)
            {
                if (searchstat==1)
                    if (sector[i].ceilingpicnum==temppicnum)
                    {
                        sector[i].ceilingshade=tempshade;
                    }
                if (searchstat==2)
                    if (sector[i].floorpicnum==temppicnum)
                    {
                        sector[i].floorshade=tempshade;
                    }
            }
            message("Sectors with picnum %d have shade of %d",temppicnum,tempshade);
            asksave=1;
            break;
        case 3:
            for (i=0;i<MAXSPRITES;i++)
            {
                if (sprite[i].picnum==temppicnum)
                {
                    sprite[i].shade=tempshade;
                }
            }
            message("Sprites with picnum %d have shade of %d",temppicnum,tempshade);
            asksave=1;
            break;
        }
    }

    if (keystatus[KEYSC_QUOTE] && keystatus[KEYSC_T]) // ' T
    {
        keystatus[KEYSC_T] = 0;
        switch (searchstat)
        {
        case 0:
        case 4:
            wall[searchwall].lotag =
                getnumber256("Wall lotag: ",wall[searchwall].lotag,65536L,0);
            break;
        case 1:
        case 2:
            sector[searchsector].lotag =
                _getnumber256("Sector lotag: ",sector[searchsector].lotag,65536L,0,(void *)ExtGetSectorType);
            break;
        case 3:
            if (sprite[searchwall].picnum == SECTOREFFECTOR)
                sprite[searchwall].lotag =
                    _getnumber256("Sprite lotag: ",sprite[searchwall].lotag,65536L,0,(void *)SectorEffectorTagText);
            else
                sprite[searchwall].lotag =
                    getnumber256("Sprite lotag: ",sprite[searchwall].lotag,65536L,0);
            break;
        }
    }

    if (keystatus[KEYSC_QUOTE] && keystatus[KEYSC_H]) // ' H
    {
        keystatus[KEYSC_H] = 0;
        switch (searchstat)
        {
        case 0:
        case 4:
            wall[searchwall].hitag =
                getnumber256("Wall hitag: ",wall[searchwall].hitag,65536L,0);
            break;
        case 1:
        case 2:
            sector[searchsector].hitag =
                getnumber256("Sector hitag: ",sector[searchsector].hitag,65536L,0);
            break;
        case 3:
            sprite[searchwall].hitag =
                getnumber256("Sprite hitag: ",sprite[searchwall].hitag,65536L,0);
            break;
        }
    }

    if (keystatus[KEYSC_QUOTE] && keystatus[KEYSC_S]) // ' S
    {
        keystatus[KEYSC_S] = 0;
        switch (searchstat)
        {
        case 0:
        case 4:
            getnumberptr256("Wall shade: ",&wall[searchwall].shade,sizeof(wall[searchwall].shade),128L,1,NULL);
            break;
        case 1:
            getnumberptr256("Ceiling shade: ",&sector[searchsector].ceilingshade,sizeof(sector[searchsector].ceilingshade),128L,1,NULL);
            break;
        case 2:
            getnumberptr256("Floor shade: ",&sector[searchsector].floorshade,sizeof(sector[searchsector].floorshade),128L,1,NULL);
            break;
        case 3:
            getnumberptr256("Sprite shade: ",&sprite[searchwall].shade,sizeof(sprite[searchwall].shade),128L,1,NULL);
            break;
        }
    }

    if (keystatus[KEYSC_F2]) // F2
    {
        if (eitherSHIFT)
            infobox^=1;
        else
            if (eitherCTRL)
                infobox^=2;
            else usedcount=!usedcount;
        keystatus[KEYSC_F2] = 0;
    }
    if (keystatus[KEYSC_TAB]) // TAB : USED
    {
        //        usedcount=!usedcount;

        count=0;
        for (i=0;i<numwalls;i++)
        {
            if (wall[i].picnum == temppicnum) count++;
            if (wall[i].overpicnum == temppicnum) count++;
        }
        for (i=0;i<numsectors;i++)  // JBF 20040307: was numwalls, thanks Semicharm
        {
            if (sector[i].ceilingpicnum == temppicnum) count++;
            if (sector[i].floorpicnum == temppicnum) count++;
        }
        statnum = 0;        //status 1
        i = headspritestat[statnum];
        while (i != -1)
        {
            nexti = nextspritestat[i];

            //your code goes here
            //ex: printf("Sprite %d has a status of 1 (active)\n",i,statnum);

            if (sprite[i].picnum == temppicnum) count++;
            i = nexti;
        }

    }


    if (keystatus[KEYSC_F1]) // F1
    {
        helpon=!helpon;
        keystatus[KEYSC_H]=0;  // delete this line?
        keystatus[KEYSC_F1]=0;
    }

    if (keystatus[KEYSC_G]) // G
    {
        switch (searchstat)
        {
        case 0:
            getnumberptr256("Wall picnum: ",&wall[searchwall].picnum,sizeof(wall[searchwall].picnum),MAXTILES-1,0,NULL);
            break;
        case 1:
            getnumberptr256("Sector ceiling picnum: ",&sector[searchsector].ceilingpicnum,sizeof(sector[searchsector].ceilingpicnum),MAXTILES-1,0,NULL);
            break;
        case 2:
            getnumberptr256("Sector floor picnum: ",&sector[searchsector].floorpicnum,sizeof(sector[searchsector].floorpicnum),MAXTILES-1,0,NULL);
            break;
        case 3:
            getnumberptr256("Sprite picnum: ",&sprite[searchwall].picnum,sizeof(sprite[searchwall].picnum),MAXTILES-1,0,NULL);
            break;
        case 4:
            getnumberptr256("Masked wall picnum: ",&wall[searchwall].overpicnum,sizeof(wall[searchwall].overpicnum),MAXTILES-1,0,NULL);
            break;
        }
        asksave = 1;
        keystatus[KEYSC_G] = 0;
    }

    if (keystatus[KEYSC_B])  // B (clip Blocking xor) (3D)
    {
        if (searchstat == 3)
        {
            sprite[searchwall].cstat ^= 1;
            //                                sprite[searchwall].cstat &= ~256;
            //                                sprite[searchwall].cstat |= ((sprite[searchwall].cstat&1)<<8);
            sprintf(getmessage,"Sprite %d blocking bit %s",searchwall,sprite[searchwall].cstat&1?"ON":"OFF");
            message(getmessage);
            asksave = 1;
        }
        else
        {
            wall[searchwall].cstat ^= 1;
            //                                wall[searchwall].cstat &= ~64;
            if ((wall[searchwall].nextwall >= 0) && (eitherSHIFT == 0))
            {
                wall[wall[searchwall].nextwall].cstat &= ~(1+64);
                wall[wall[searchwall].nextwall].cstat |= (wall[searchwall].cstat&1);
            }
            sprintf(getmessage,"Wall %d blocking bit %s",searchwall,wall[searchwall].cstat&1?"ON":"OFF");
            message(getmessage);
            asksave = 1;
        }
        keystatus[KEYSC_B] = 0;
    }

    if (keystatus[KEYSC_T])  // T (transluscence for sprites/masked walls)
    {
        if (searchstat == 1)   //Set masked/transluscent ceilings/floors
        {
            i = (sector[searchsector].ceilingstat&(128+256));
            sector[searchsector].ceilingstat &= ~(128+256);
            switch (i)
            {
            case 0:
                sector[searchsector].ceilingstat |= 128;
                break;
            case 128:
                sector[searchsector].ceilingstat |= 256;
                break;
            case 256:
                sector[searchsector].ceilingstat |= 384;
                break;
            case 384:
                sector[searchsector].ceilingstat |= 0;
                break;
            }
            asksave = 1;
        }
        if (searchstat == 2)
        {
            i = (sector[searchsector].floorstat&(128+256));
            sector[searchsector].floorstat &= ~(128+256);
            switch (i)
            {
            case 0:
                sector[searchsector].floorstat |= 128;
                break;
            case 128:
                sector[searchsector].floorstat |= 256;
                break;
            case 256:
                sector[searchsector].floorstat |= 384;
                break;
            case 384:
                sector[searchsector].floorstat |= 0;
                break;
            }
            asksave = 1;
        }

        if (keystatus[KEYSC_QUOTE])
        {
            switch (searchstat)
            {
            case 0:
            case 4:
                strcpy(buffer,"Wall lotag: ");
                wall[searchwall].lotag = getnumber256(buffer,(int)wall[searchwall].lotag,65536L,0);
                break;
            case 1:
                strcpy(buffer,"Sector lotag: ");
                sector[searchsector].lotag = getnumber256(buffer,(int)sector[searchsector].lotag,65536L,0);
                break;
            case 2:
                strcpy(buffer,"Sector lotag: ");
                sector[searchsector].lotag = getnumber256(buffer,(int)sector[searchsector].lotag,65536L,0);
                break;
            case 3:
                strcpy(buffer,"Sprite lotag: ");
                sprite[searchwall].lotag = getnumber256(buffer,(int)sprite[searchwall].lotag,65536L,0);
                break;
            }
        }
        else if (eitherCTRL)
        {
            if (searchstat == 3)
                rendertext(searchwall);
        }
        else
        {
            if (searchstat == 3)
            {
                if ((sprite[searchwall].cstat&2) == 0)
                    sprite[searchwall].cstat |= 2;
                else if ((sprite[searchwall].cstat&512) == 0)
                    sprite[searchwall].cstat |= 512;
                else
                    sprite[searchwall].cstat &= ~(2+512);
                asksave = 1;
            }
            if (searchstat == 4)
            {
                if ((wall[searchwall].cstat&128) == 0)
                    wall[searchwall].cstat |= 128;
                else if ((wall[searchwall].cstat&512) == 0)
                    wall[searchwall].cstat |= 512;
                else
                    wall[searchwall].cstat &= ~(128+512);

                if (wall[searchwall].nextwall >= 0)
                {
                    wall[wall[searchwall].nextwall].cstat &= ~(128+512);
                    wall[wall[searchwall].nextwall].cstat |= (wall[searchwall].cstat&(128+512));
                }
                asksave = 1;
            }
        }
        keystatus[KEYSC_T] = 0;
    }

    if (keystatus[KEYSC_QUOTE] && keystatus[KEYSC_ENTER]) // ' ENTER
    {
        message("Pasted picnum only");
        switch (searchstat)
        {
        case 0 :
            wall[searchwall].picnum = temppicnum;
            break;
        case 1 :
            sector[searchsector].ceilingpicnum = temppicnum;
            break;
        case 2 :
            sector[searchsector].floorpicnum = temppicnum;
            break;
        case 3 :
            sprite[searchwall].picnum = temppicnum;
            break;
        case 4 :
            wall[searchwall].overpicnum = temppicnum;
            break;
        }
        keystatus[KEYSC_ENTER]=0;
    }

    i = 512;
    if (keystatus[KEYSC_RSHIFT]) i = 8;
    if (keystatus[KEYSC_LSHIFT]) i = 1;
    mouseaction=0;
    if (eitherCTRL && bstatus&1 && (searchstat == 1 || searchstat == 2))
    {
        mousex=0;mskip=1;
        if (mousey<0)
        {
            i=klabs(mousey*2);
            mouseaction=1;
        }
    }

    if (keystatus[KEYSC_LBRACK] || mouseaction)  // [
    {
        keystatus[KEYSC_LBRACK] = 0;
        if (eitherALT)
        {
            i = wall[searchwall].nextsector;
            if (i >= 0)
                switch (searchstat)
                {
                case 0:
                case 1:
                case 4:
                    alignceilslope(searchsector,wall[searchwall].x,wall[searchwall].y,getceilzofslope(i,wall[searchwall].x,wall[searchwall].y));
                    message("Sector %d align ceiling to wall %d",searchsector,searchwall);
                    break;
                case 2:
                    alignflorslope(searchsector,wall[searchwall].x,wall[searchwall].y,getflorzofslope(i,wall[searchwall].x,wall[searchwall].y));
                    message("Sector %d align floor to wall %d",searchsector,searchwall);
                    break;
                }
        }
        else
        {
            if (searchstat == 1)
            {
                if (!(sector[searchsector].ceilingstat&2))
                    sector[searchsector].ceilingheinum = 0;
                sector[searchsector].ceilingheinum = max(sector[searchsector].ceilingheinum-i,-32768);
                message("Sector %d ceiling slope = %d",searchsector,sector[searchsector].ceilingheinum);
            }
            if (searchstat == 2)
            {
                if (!(sector[searchsector].floorstat&2))
                    sector[searchsector].floorheinum = 0;
                sector[searchsector].floorheinum = max(sector[searchsector].floorheinum-i,-32768);
                message("Sector %d floor slope = %d",searchsector,sector[searchsector].floorheinum);
            }
        }

        if (sector[searchsector].ceilingheinum == 0)
            sector[searchsector].ceilingstat &= ~2;
        else
            sector[searchsector].ceilingstat |= 2;

        if (sector[searchsector].floorheinum == 0)
            sector[searchsector].floorstat &= ~2;
        else
            sector[searchsector].floorstat |= 2;
        asksave = 1;
    }

    i = 512;
    if (keystatus[KEYSC_RSHIFT]) i = 8;
    if (keystatus[KEYSC_LSHIFT]) i = 1;
    mouseaction=0;
    if (eitherCTRL && bstatus&1 && (searchstat == 1 || searchstat == 2))
    {
        mousex=0;mskip=1;
        if (mousey>0)
        {
            i=klabs(mousey*2);
            mouseaction=1;
        }
    }
    if (keystatus[KEYSC_RBRACK] || mouseaction)  // ]
    {
        keystatus[KEYSC_RBRACK] = 0;
        if (eitherALT)
        {
            i = wall[searchwall].nextsector;
            if (i >= 0)
                switch (searchstat)
                {
                case 1:
                    alignceilslope(searchsector,wall[searchwall].x,wall[searchwall].y,getceilzofslope(i,wall[searchwall].x,wall[searchwall].y));
                    message("Sector %d align ceiling to wall %d",searchsector,searchwall);
                    break;
                case 0:
                case 2:
                case 4:
                    alignflorslope(searchsector,wall[searchwall].x,wall[searchwall].y,getflorzofslope(i,wall[searchwall].x,wall[searchwall].y));
                    message("Sector %d align floor to wall %d",searchsector,searchwall);
                    break;
                }
        }
        else
        {
            if (searchstat == 1)
            {
                if (!(sector[searchsector].ceilingstat&2))
                    sector[searchsector].ceilingheinum = 0;
                sector[searchsector].ceilingheinum = min(sector[searchsector].ceilingheinum+i,32767);
                message("Sector %d ceiling slope = %d",searchsector,sector[searchsector].ceilingheinum);
            }
            if (searchstat == 2)
            {
                if (!(sector[searchsector].floorstat&2))
                    sector[searchsector].floorheinum = 0;
                sector[searchsector].floorheinum = min(sector[searchsector].floorheinum+i,32767);
                message("Sector %d floor slope = %d",searchsector,sector[searchsector].floorheinum);
            }
        }

        if (sector[searchsector].ceilingheinum == 0)
            sector[searchsector].ceilingstat &= ~2;
        else
            sector[searchsector].ceilingstat |= 2;

        if (sector[searchsector].floorheinum == 0)
            sector[searchsector].floorstat &= ~2;
        else
            sector[searchsector].floorstat |= 2;

        asksave = 1;
    }

    if (bstatus&1 && eitherSHIFT) mskip=1;
    if (bstatus&1 && eitherSHIFT && (searchstat == 1 || searchstat == 2) && (mousex|mousey))
    {
        int fw,x1,y1,x2,y2,stat,ma,a=0;

        stat=(searchstat==2)?sector[searchsector].floorstat:sector[searchsector].ceilingstat;
        if (stat&64) // align to first wall
        {
            fw=sector[searchsector].wallptr;
            x1=wall[fw].x,y1=wall[fw].y;
            x2=wall[wall[fw].point2].x,y2=wall[wall[fw].point2].y;
            a=getangle(x1-x2,y1-y2);
        }
        mouseax+=mousex;mouseay+=mousey;
        ma=getangle(mouseax,mouseay);
        ma+=ang-a;

        i = stat;
        i = (i&0x4)+((i>>4)&3);
        if (stat&64) // align to first wall
            switch (i)
            {
            case 0:break;
            case 1:ma=-ma;break;
            case 2:ma=1024-ma;break;
            case 3:ma+=1024;break;
            case 4:ma=-512-ma;break;
            case 5:ma+=512;break;
            case 6:ma-=512;break;
            case 7:ma=512-ma;break;
            }
        else
            switch (i)
            {
            case 0:ma=-ma;break;
            case 1:break;
            case 2:ma+=1024;break;
            case 3:ma=1024-ma;break;
            case 4:ma-=512;break;
            case 5:ma=512-ma;break;
            case 6:ma=-512-ma;break;
            case 7:ma+=512;break;
            }

        a=ksqrt(mouseax*mouseax+mouseay*mouseay);
        if (a)
        {
            int mult=(stat&8)?8192:8192*2;
            x1=-a*sintable[(ma+2048)&2047]/mult;
            y1=-a*sintable[(ma+1536)&2047]/mult;
            if (x1||y1)
            {
                mouseax=0;mouseay=0;
                if (searchstat==1)
                {
                    changedir=1;if (x1<0) {changedir=-1;x1*=-1;}
                    while (x1--)sector[searchsector].ceilingxpanning = changechar(sector[searchsector].ceilingxpanning,changedir,0,0);
                    changedir=1;if (y1<0) {changedir=-1;y1*=-1;}
                    while (y1--)sector[searchsector].ceilingypanning = changechar(sector[searchsector].ceilingypanning,changedir,0,0);
                    message("Sector %d ceiling panning: %d, %d",searchsector,sector[searchsector].ceilingxpanning,sector[searchsector].ceilingypanning);
                }
                else
                {
                    changedir=1;if (x1<0) {changedir=-1;x1*=-1;}
                    while (x1--)sector[searchsector].floorxpanning = changechar(sector[searchsector].floorxpanning,changedir,0,0);
                    changedir=1;if (y1<0) {changedir=-1;y1*=-1;}
                    while (y1--)sector[searchsector].floorypanning = changechar(sector[searchsector].floorypanning,changedir,0,0);
                    message("Sector %d floor panning: %d, %d",searchsector,sector[searchsector].floorxpanning,sector[searchsector].floorypanning);
                }
                asksave=1;
            }
        }
        mousex=0;mousey=0;
    }
    if (!mouseb) {mouseax=0;mouseay=0;}

    smooshyalign = keystatus[KEYSC_gKP5];
    repeatpanalign = eitherSHIFT;

    updownunits=1;
    mouseaction=0;

    if (bstatus&1 && searchstat != 1 && searchstat != 2)
    {
        if (eitherSHIFT)
        {
            mskip=1;
            if (mousex!=0)
            {
                mouseaction=1;
                mouseax+=mousex;
                updownunits=klabs((int)(mouseax/2.));
                if (updownunits) {mouseax=0;}
            }
        }
        else
            if (eitherCTRL)
            {
                mskip=1;
                if (mousex!=0)
                {
                    mouseaction=2;
                    repeatpanalign=0;
                    if (searchstat==3)
                    {
                        updownunits=klabs(mouseax+=mousex)/4;
                        if (updownunits)mouseax=0;
                    }
                    else
                    {
                        updownunits=klabs(mouseax+=mousex)/16;
                        if (updownunits)mouseax=0;
                    }
                }
            }
    }

    if (keystatus[KEYSC_gLEFT] || keystatus[KEYSC_gRIGHT] || mouseaction) // 4 & 6 (keypad)
    {
        if ((repeatcountx == 0) || (repeatcountx > 32) || mouseaction)
        {
            changedir = 0;
            if (keystatus[KEYSC_gLEFT]  || mousex>0) changedir = -1;
            if (keystatus[KEYSC_gRIGHT] || mousex<0) changedir = 1;

            if ((searchstat == 0) || (searchstat == 4))
            {
                if (repeatpanalign == 0)
                {
                    while (updownunits--)wall[searchwall].xrepeat = changechar(wall[searchwall].xrepeat,changedir,smooshyalign,1);
                    message("Wall %d repeat: %d, %d",searchwall,wall[searchwall].xrepeat,wall[searchwall].yrepeat);
                }
                else
                {
                    if (mouseaction)
                    {
                        i=wall[searchwall].cstat;
                        i=((i>>3)&1)+((i>>7)&2);
                        if (i==1||i==3)changedir*=-1;
                    }
                    while (updownunits--)wall[searchwall].xpanning = changechar(wall[searchwall].xpanning,changedir,smooshyalign,0);
                    message("Wall %d panning: %d, %d",searchwall,wall[searchwall].xpanning,wall[searchwall].ypanning);
                }
            }
            if ((searchstat == 1) || (searchstat == 2))
            {
                if (searchstat == 1)
                {
                    while (updownunits--)sector[searchsector].ceilingxpanning = changechar(sector[searchsector].ceilingxpanning,changedir,smooshyalign,0);
                    message("Sector %d ceiling panning: %d, %d",searchsector,sector[searchsector].ceilingxpanning,sector[searchsector].ceilingypanning);
                }
                else
                {
                    while (updownunits--)sector[searchsector].floorxpanning = changechar(sector[searchsector].floorxpanning,changedir,smooshyalign,0);
                    message("Sector %d floor panning: %d, %d",searchsector,sector[searchsector].floorxpanning,sector[searchsector].floorypanning);
                }
            }
            if (searchstat == 3)
            {
                if (mouseaction==1)
                {
                    int xvect,yvect;
                    short cursectnum=sprite[searchwall].sectnum;
                    xvect = -((mousex*(int)sintable[(ang+2048)&2047])<<3);
                    yvect = -((mousex*(int)sintable[(ang+1536)&2047])<<3);
                    clipmove(&sprite[searchwall].x,&sprite[searchwall].y,&sprite[searchwall].z,
                             &cursectnum,xvect,yvect,128L,4L<<8,4L<<8,spnoclip?1:CLIPMASK0);
                    setsprite(searchwall,sprite[searchwall].x,sprite[searchwall].y,sprite[searchwall].z);
                }
                else
                {
                    if (mouseaction==2)changedir*=-1;
                    while (updownunits--)sprite[searchwall].xrepeat = changechar(sprite[searchwall].xrepeat,changedir,smooshyalign,1);
                    if (sprite[searchwall].xrepeat < 4)
                        sprite[searchwall].xrepeat = 4;
                    message("Sprite %d repeat: %d, %d",searchwall,sprite[searchwall].xrepeat,sprite[searchwall].yrepeat);
                }
            }
            asksave = 1;
            repeatcountx = max(1,repeatcountx-2);
        }
        repeatcountx += synctics;
    }
    else
        repeatcountx = 0;

    updownunits=1;
    mouseaction=0;
    if (bstatus&1 && searchstat != 1 && searchstat != 2)
    {
        if (eitherSHIFT)
        {
            mskip=1;
            if (mousey!=0)
            {
                mouseaction=1;
                updownunits=klabs(mousey);
                if (searchstat != 3)
                {
                    updownunits=klabs((int)(mousey*128./tilesizy[wall[searchwall].picnum]));
                }
            }
        }
        else
            if (eitherCTRL)
            {
                mskip=1;
                if (mousey!=0)
                {
                    mouseaction=2;
                    repeatpanalign=0;
                    if (searchstat==3)
                    {
                        updownunits=klabs(mouseay+=mousey)/4;
                        if (updownunits)mouseay=0;
                    }
                    else
                    {
                        updownunits=klabs(mouseay+=mousey)/32;
                        if (updownunits)mouseay=0;
                    }
                }
            }
    }
    if (!mouseb) {mouseax=0;mouseay=0;}
    if (keystatus[KEYSC_gUP] || keystatus[KEYSC_gDOWN] || mouseaction)  // 2 & 8 (keypad)
    {
        if ((repeatcounty == 0) || (repeatcounty > 32) || mouseaction)
        {
            changedir = 0;
            if (keystatus[KEYSC_gUP]   || mousey>0) changedir = -1;
            if (keystatus[KEYSC_gDOWN] || mousey<0) changedir = 1;

            if ((searchstat == 0) || (searchstat == 4))
            {
                if (repeatpanalign == 0)
                {
                    while (updownunits--)wall[searchwall].yrepeat = changechar(wall[searchwall].yrepeat,changedir,smooshyalign,1);
                    message("Wall %d repeat: %d, %d",searchwall,wall[searchwall].xrepeat,wall[searchwall].yrepeat);
                }
                else
                {
                    while (updownunits--)wall[searchwall].ypanning = changechar(wall[searchwall].ypanning,changedir,smooshyalign,0);
                    message("Wall %d panning: %d, %d",searchwall,wall[searchwall].xpanning,wall[searchwall].ypanning);
                }
            }
            if ((searchstat == 1) || (searchstat == 2))
            {
                if (searchstat == 1)
                {
                    while (updownunits--)sector[searchsector].ceilingypanning = changechar(sector[searchsector].ceilingypanning,changedir,smooshyalign,0);
                    message("Sector %d ceiling panning: %d, %d",searchsector,sector[searchsector].ceilingxpanning,sector[searchsector].ceilingypanning);
                }
                else
                {
                    while (updownunits--)sector[searchsector].floorypanning = changechar(sector[searchsector].floorypanning,changedir,smooshyalign,0);
                    message("Sector %d floor panning: %d, %d",searchsector,sector[searchsector].floorxpanning,sector[searchsector].floorypanning);
                }
            }
            if (searchstat == 3)
            {
                if (mouseaction==1)
                {
                    int xvect,yvect;
                    short cursectnum=sprite[searchwall].sectnum;
                    xvect = -((mousey*(int)sintable[(ang+2560)&2047])<<3);
                    yvect = -((mousey*(int)sintable[(ang+2048)&2047])<<3);
                    clipmove(&sprite[searchwall].x,&sprite[searchwall].y,&sprite[searchwall].z,
                             &cursectnum,xvect,yvect,128L,4L<<8,4L<<8,spnoclip?1:CLIPMASK0);
                    setsprite(searchwall,sprite[searchwall].x,sprite[searchwall].y,sprite[searchwall].z);
                }
                else
                {
                    while (updownunits--)sprite[searchwall].yrepeat = changechar(sprite[searchwall].yrepeat,changedir,smooshyalign,1);
                    if (sprite[searchwall].yrepeat < 4)
                        sprite[searchwall].yrepeat = 4;
                    message("Sprite %d repeat: %d, %d",searchwall,sprite[searchwall].xrepeat,sprite[searchwall].yrepeat);
                }
            }
            asksave = 1;
            repeatcounty = max(1,repeatcounty-2);
        }
        repeatcounty += synctics;
    }
    else
        repeatcounty = 0;

    if (keystatus[KEYSC_F11])  //F11 - brightness
    {
        extern short brightness;

        keystatus[KEYSC_F11] = 0;

        if (eitherSHIFT)
        {
            brightness--;
            if (brightness < 0) brightness = 15;
        }
        else
        {
            brightness++;
            if (brightness >= 16) brightness = 0;
        }

        vid_gamma = 1.0 + ((float)brightness / 10.0);
        setbrightness(brightness,palette,0);
        message("Brightness: %d/16",brightness+1);
    }

    if (keystatus[KEYSC_TAB])  //TAB
    {
        if (searchstat == 0)
        {
            temppicnum = wall[searchwall].picnum;
            tempshade = wall[searchwall].shade;
            temppal = wall[searchwall].pal;
            tempxrepeat = wall[searchwall].xrepeat;
            tempyrepeat = wall[searchwall].yrepeat;
            tempcstat = wall[searchwall].cstat;
            templotag = wall[searchwall].lotag;
            temphitag = wall[searchwall].hitag;
            tempextra = wall[searchwall].extra;
        }
        if (searchstat == 1)
        {
            temppicnum = sector[searchsector].ceilingpicnum;
            tempshade = sector[searchsector].ceilingshade;
            temppal = sector[searchsector].ceilingpal;
            tempvis = sector[searchsector].visibility;
            tempxrepeat = sector[searchsector].ceilingxpanning;
            tempyrepeat = sector[searchsector].ceilingypanning;
            tempcstat = sector[searchsector].ceilingstat;
            templotag = sector[searchsector].lotag;
            temphitag = sector[searchsector].hitag;
            tempextra = sector[searchsector].extra;
        }
        if (searchstat == 2)
        {
            temppicnum = sector[searchsector].floorpicnum;
            tempshade = sector[searchsector].floorshade;
            temppal = sector[searchsector].floorpal;
            tempvis = sector[searchsector].visibility;
            tempxrepeat = sector[searchsector].floorxpanning;
            tempyrepeat = sector[searchsector].floorypanning;
            tempcstat = sector[searchsector].floorstat;
            templotag = sector[searchsector].lotag;
            temphitag = sector[searchsector].hitag;
            tempextra = sector[searchsector].extra;
        }
        if (searchstat == 3)
        {
            temppicnum = sprite[searchwall].picnum;
            tempshade = sprite[searchwall].shade;
            temppal = sprite[searchwall].pal;
            tempxrepeat = sprite[searchwall].xrepeat;
            tempyrepeat = sprite[searchwall].yrepeat;
            tempcstat = sprite[searchwall].cstat;
            templotag = sprite[searchwall].lotag;
            temphitag = sprite[searchwall].hitag;
            tempextra = sprite[searchwall].extra;
            tempxvel = sprite[searchwall].xvel;
            tempyvel = sprite[searchwall].xvel;
            tempzvel = sprite[searchwall].xvel;
        }
        if (searchstat == 4)
        {
            temppicnum = wall[searchwall].overpicnum;
            tempshade = wall[searchwall].shade;
            temppal = wall[searchwall].pal;
            tempxrepeat = wall[searchwall].xrepeat;
            tempyrepeat = wall[searchwall].yrepeat;
            tempcstat = wall[searchwall].cstat;
            templotag = wall[searchwall].lotag;
            temphitag = wall[searchwall].hitag;
            tempextra = wall[searchwall].extra;
        }
        somethingintab = searchstat;
        keystatus[KEYSC_TAB] = 0;
    }

    if (keystatus[KEYSC_ENTER])
    {
        extern char pskysearch[MAXSECTORS];
        short daang;
        int dashade[2];
        if (eitherSHIFT)
        {
            if (((searchstat == 0) || (searchstat == 4)) && eitherCTRL)  //Ctrl-shift Enter (auto-shade)
            {
                dashade[0] = 127;
                dashade[1] = -128;
                i = searchwall;
                do
                {
                    if ((int)wall[i].shade < dashade[0]) dashade[0] = wall[i].shade;
                    if ((int)wall[i].shade > dashade[1]) dashade[1] = wall[i].shade;

                    i = wall[i].point2;
                }
                while (i != searchwall);

                daang = getangle(wall[wall[searchwall].point2].x-wall[searchwall].x,wall[wall[searchwall].point2].y-wall[searchwall].y);
                i = searchwall;
                do
                {
                    j = getangle(wall[wall[i].point2].x-wall[i].x,wall[wall[i].point2].y-wall[i].y);
                    k = ((j+2048-daang)&2047);
                    if (k > 1024)
                        k = 2048-k;
                    wall[i].shade = dashade[0]+mulscale10(k,dashade[1]-dashade[0]);

                    i = wall[i].point2;
                }
                while (i != searchwall);
                message("Wall %d auto-shaded",searchwall);
            }
            else if (somethingintab < 255)
            {
                if (searchstat == 0) wall[searchwall].shade = tempshade, wall[searchwall].pal = temppal;
                if (searchstat == 1)
                {
                    sector[searchsector].ceilingshade = tempshade, sector[searchsector].ceilingpal = temppal;
                    if ((somethingintab == 1) || (somethingintab == 2))
                        sector[searchsector].visibility = tempvis;
                }
                if (searchstat == 2)
                {
                    sector[searchsector].floorshade = tempshade, sector[searchsector].floorpal = temppal;
                    if ((somethingintab == 1) || (somethingintab == 2))
                        sector[searchsector].visibility = tempvis;
                }
                if (searchstat == 3) sprite[searchwall].shade = tempshade, sprite[searchwall].pal = temppal;
                if (searchstat == 4) wall[searchwall].shade = tempshade, wall[searchwall].pal = temppal;
                message("Pasted shading+pal");
            }
        }
        else if (((searchstat == 0) || (searchstat == 4)) && eitherCTRL && (somethingintab < 255))  //Either ctrl key
        {
            i = searchwall;
            do
            {
                wall[i].picnum = temppicnum;
                wall[i].shade = tempshade;
                wall[i].pal = temppal;
                if ((somethingintab == 0) || (somethingintab == 4))
                {
                    wall[i].xrepeat = tempxrepeat;
                    wall[i].yrepeat = tempyrepeat;
                    wall[i].cstat = tempcstat;
                }
                fixrepeats((short)i);
                i = wall[i].point2;
            }
            while (i != searchwall);
            message("Pasted picnum+shading+pal");
        }
        else if (((searchstat == 1) || (searchstat == 2)) && eitherCTRL && (somethingintab < 255))  //Either ctrl key
        {
            clearbuf(&pskysearch[0],(int)((numsectors+3)>>2),0L);
            if (searchstat == 1)
            {
                i = searchsector;
                if ((sector[i].ceilingstat&1) > 0)
                    pskysearch[i] = 1;

                while (pskysearch[i] == 1)
                {
                    sector[i].ceilingpicnum = temppicnum;
                    sector[i].ceilingshade = tempshade;
                    sector[i].ceilingpal = temppal;
                    if ((somethingintab == 1) || (somethingintab == 2))
                    {
                        sector[i].ceilingxpanning = tempxrepeat;
                        sector[i].ceilingypanning = tempyrepeat;
                        sector[i].ceilingstat = tempcstat;
                    }
                    pskysearch[i] = 2;

                    startwall = sector[i].wallptr;
                    endwall = startwall + sector[i].wallnum - 1;
                    for (j=startwall;j<=endwall;j++)
                    {
                        k = wall[j].nextsector;
                        if (k >= 0)
                            if ((sector[k].ceilingstat&1) > 0)
                                if (pskysearch[k] == 0)
                                    pskysearch[k] = 1;
                    }

                    for (j=0;j<numsectors;j++)
                        if (pskysearch[j] == 1)
                            i = j;
                }
            }
            if (searchstat == 2)
            {
                i = searchsector;
                if ((sector[i].floorstat&1) > 0)
                    pskysearch[i] = 1;

                while (pskysearch[i] == 1)
                {
                    sector[i].floorpicnum = temppicnum;
                    sector[i].floorshade = tempshade;
                    sector[i].floorpal = temppal;
                    if ((somethingintab == 1) || (somethingintab == 2))
                    {
                        sector[i].floorxpanning = tempxrepeat;
                        sector[i].floorypanning = tempyrepeat;
                        sector[i].floorstat = tempcstat;
                    }
                    pskysearch[i] = 2;

                    startwall = sector[i].wallptr;
                    endwall = startwall + sector[i].wallnum - 1;
                    for (j=startwall;j<=endwall;j++)
                    {
                        k = wall[j].nextsector;
                        if (k >= 0)
                            if ((sector[k].floorstat&1) > 0)
                                if (pskysearch[k] == 0)
                                    pskysearch[k] = 1;
                    }

                    for (j=0;j<numsectors;j++)
                        if (pskysearch[j] == 1)
                            i = j;
                }
            }
            message("Pasted picnum+shading+pal");
        }
        else if (somethingintab < 255)
        {
            if (searchstat == 0)
            {
                wall[searchwall].picnum = temppicnum;
                wall[searchwall].shade = tempshade;
                wall[searchwall].pal = temppal;
                if (somethingintab == 0)
                {
                    wall[searchwall].xrepeat = tempxrepeat;
                    wall[searchwall].yrepeat = tempyrepeat;
                    wall[searchwall].cstat = tempcstat;
                    wall[searchwall].lotag = templotag;
                    wall[searchwall].hitag = temphitag;
                    wall[searchwall].extra = tempextra;
                }
                fixrepeats(searchwall);
            }
            if (searchstat == 1)
            {
                sector[searchsector].ceilingpicnum = temppicnum;
                sector[searchsector].ceilingshade = tempshade;
                sector[searchsector].ceilingpal = temppal;
                if ((somethingintab == 1) || (somethingintab == 2))
                {
                    sector[searchsector].ceilingxpanning = tempxrepeat;
                    sector[searchsector].ceilingypanning = tempyrepeat;
                    sector[searchsector].ceilingstat = tempcstat;
                    sector[searchsector].visibility = tempvis;
                    sector[searchsector].lotag = templotag;
                    sector[searchsector].hitag = temphitag;
                    sector[searchsector].extra = tempextra;
                }
            }
            if (searchstat == 2)
            {
                sector[searchsector].floorpicnum = temppicnum;
                sector[searchsector].floorshade = tempshade;
                sector[searchsector].floorpal = temppal;
                if ((somethingintab == 1) || (somethingintab == 2))
                {
                    sector[searchsector].floorxpanning= tempxrepeat;
                    sector[searchsector].floorypanning= tempyrepeat;
                    sector[searchsector].floorstat = tempcstat;
                    sector[searchsector].visibility = tempvis;
                    sector[searchsector].lotag = templotag;
                    sector[searchsector].hitag = temphitag;
                    sector[searchsector].extra = tempextra;
                }
            }
            if (searchstat == 3)
            {
                sprite[searchwall].picnum = temppicnum;
                if ((tilesizx[temppicnum] <= 0) || (tilesizy[temppicnum] <= 0))
                {
                    j = 0;
                    for (k=0;k<MAXTILES;k++)
                        if ((tilesizx[k] > 0) && (tilesizy[k] > 0))
                        {
                            j = k;
                            break;
                        }
                    sprite[searchwall].picnum = j;
                }
                sprite[searchwall].shade = tempshade;
                sprite[searchwall].pal = temppal;
                if (somethingintab == 3)
                {
                    sprite[searchwall].xrepeat = tempxrepeat;
                    sprite[searchwall].yrepeat = tempyrepeat;
                    if (sprite[searchwall].xrepeat < 1) sprite[searchwall].xrepeat = 1;
                    if (sprite[searchwall].yrepeat < 1) sprite[searchwall].yrepeat = 1;
                    sprite[searchwall].cstat = tempcstat;
                    sprite[searchwall].lotag = templotag;
                    sprite[searchwall].hitag = temphitag;
                    sprite[searchwall].extra = tempextra;
                    sprite[searchwall].xvel = tempxvel;
                    sprite[searchwall].yvel = tempyvel;
                    sprite[searchwall].zvel = tempzvel;
                }
            }
            if (searchstat == 4)
            {
                wall[searchwall].overpicnum = temppicnum;
                if (wall[searchwall].nextwall >= 0)
                    wall[wall[searchwall].nextwall].overpicnum = temppicnum;
                wall[searchwall].shade = tempshade;
                wall[searchwall].pal = temppal;
                if (somethingintab == 4)
                {
                    wall[searchwall].xrepeat = tempxrepeat;
                    wall[searchwall].yrepeat = tempyrepeat;
                    wall[searchwall].cstat = tempcstat;
                    wall[searchwall].lotag = templotag;
                    wall[searchwall].hitag = temphitag;
                    wall[searchwall].extra = tempextra;
                }
                fixrepeats(searchwall);
            }
            message("Pasted clipboard");
        }
        asksave = 1;
        keystatus[KEYSC_ENTER] = 0;
    }

    if (keystatus[KEYSC_C])
    {
        keystatus[KEYSC_C] = 0;
        if (eitherALT)
        {
            if (somethingintab < 255)
            {
                switch (searchstat)
                {
                case 0:
                    j = wall[searchwall].picnum;
                    for (i=0;i<numwalls;i++)
                        if (wall[i].picnum == j) wall[i].picnum = temppicnum;
                    break;
                case 1:
                    j = sector[searchsector].ceilingpicnum;
                    for (i=0;i<numsectors;i++)
                        if (sector[i].ceilingpicnum == j) sector[i].ceilingpicnum = temppicnum;
                    break;
                case 2:
                    j = sector[searchsector].floorpicnum;
                    for (i=0;i<numsectors;i++)
                        if (sector[i].floorpicnum == j) sector[i].floorpicnum = temppicnum;
                    break;
                case 3:
                    j = sprite[searchwall].picnum;
                    for (i=0;i<MAXSPRITES;i++)
                        if (sprite[i].statnum < MAXSTATUS)
                            if (sprite[i].picnum == j) sprite[i].picnum = temppicnum;
                    break;
                case 4:
                    j = wall[searchwall].overpicnum;
                    for (i=0;i<numwalls;i++)
                        if (wall[i].overpicnum == j) wall[i].overpicnum = temppicnum;
                    break;
                }
                message("Picnums replaced");
                asksave = 1;
            }
        }
        else	//C
        {
            if (searchstat == 3)
            {
                sprite[searchwall].cstat ^= 128;
                message("Sprite %d center bit %s",searchwall,(sprite[searchwall].cstat&128)?"ON":"OFF");
                asksave = 1;
            }
        }
    }

    if (keystatus[KEYSC_SLASH])  // /?     Reset panning&repeat to 0
    {
        if ((searchstat == 0) || (searchstat == 4))
        {
            wall[searchwall].xpanning = 0;
            wall[searchwall].ypanning = 0;
            wall[searchwall].xrepeat = 8;
            wall[searchwall].yrepeat = 8;
            wall[searchwall].cstat = 0;
            fixrepeats((short)searchwall);
        }
        if (searchstat == 1)
        {
            sector[searchsector].ceilingxpanning = 0;
            sector[searchsector].ceilingypanning = 0;
            sector[searchsector].ceilingstat &= ~2;
            sector[searchsector].ceilingheinum = 0;
        }
        if (searchstat == 2)
        {
            sector[searchsector].floorxpanning = 0;
            sector[searchsector].floorypanning = 0;
            sector[searchsector].floorstat &= ~2;
            sector[searchsector].floorheinum = 0;
        }
        if (searchstat == 3)
        {
            if (eitherSHIFT)
            {
                sprite[searchwall].xrepeat = sprite[searchwall].yrepeat;
            }
            else
            {
                sprite[searchwall].xrepeat = 64;
                sprite[searchwall].yrepeat = 64;
            }
        }
        message("%s's size and panning reset",type2str[searchstat]);
        keystatus[KEYSC_SLASH] = 0;
        asksave = 1;
    }

    if (keystatus[KEYSC_P])  // P (parallaxing sky)
    {
        if (eitherCTRL)
        {
            parallaxtype++;
            if (parallaxtype == 3)
                parallaxtype = 0;
            sector[searchsector].ceilingstat ^= 1;
            message("Parallax type %d",parallaxtype);
        }
        else if (eitherALT)
        {
            switch (searchstat)
            {
            case 0:
            case 4:
                Bstrcpy(buffer,"Wall pal: ");
                getnumberptr256(buffer,&wall[searchwall].pal,sizeof(wall[searchwall].pal),256L,0,NULL);
                break;
            case 1:
                Bstrcpy(buffer,"Ceiling pal: ");
                getnumberptr256(buffer,&sector[searchsector].ceilingpal,sizeof(sector[searchsector].ceilingpal),256L,0,NULL);
                break;
            case 2:
                Bstrcpy(buffer,"Floor pal: ");
                getnumberptr256(buffer,&sector[searchsector].floorpal,sizeof(sector[searchsector].floorpal),256L,0,NULL);
                break;
            case 3:
                Bstrcpy(buffer,"Sprite pal: ");
                getnumberptr256(buffer,&sprite[searchwall].pal,sizeof(sprite[searchwall].pal),256L,0,NULL);
                break;
            }
        }
        else
        {
            if ((searchstat == 0) || (searchstat == 1) || (searchstat == 4))
            {
                sector[searchsector].ceilingstat ^= 1;
                message("Sector %d ceiling parallax bit %s",searchsector,sector[searchsector].ceilingstat&1?"ON":"OFF");
                asksave = 1;
            }
            else if (searchstat == 2)
            {
                sector[searchsector].floorstat ^= 1;
                message("Sector %d floor parallax bit %s",searchsector,sector[searchsector].floorstat&1?"ON":"OFF");
                asksave = 1;
            }
        }
        keystatus[KEYSC_P] = 0;
    }

    if (keystatus[KEYSC_D])   //Alt-D  (adjust sprite[].clipdist)
    {
        keystatus[KEYSC_D] = 0;
        if (eitherALT)
        {
            if (searchstat == 3)
            {
                Bstrcpy(buffer,"Sprite clipdist: ");
                sprite[searchwall].clipdist = getnumber256(buffer,sprite[searchwall].clipdist,256L,0);
            }
        }
    }
}// end 3d

static void DoSpriteSearch(int dir)  // <0: backwards, >=0: forwards
{
    char did_wrap = 0;
    int i, j, k = 0;

    for (dir<0 ? gs_cursprite-- : gs_cursprite++;; dir<0 ? gs_cursprite-- : gs_cursprite++)
    {
        if (gs_cursprite < 0)
        {
            if (!did_wrap)
            {
                did_wrap = 1;
                gs_cursprite = MAXSPRITES-1;
            }
            else break;
        }
        if (gs_cursprite >= MAXSPRITES)
        {
            if (!did_wrap)
            {
                did_wrap = 1;
                gs_cursprite = 0;
            }
            else break;
        }

        if (sprite[gs_cursprite].statnum == MAXSTATUS) continue;

        for (i=0; i<3; i++)
            for (j=0; i==1 ? j<6 : j<7; j++)
            {
                if (!gs_spritewhat[i][j]) continue;

                if (i==0)
                {
                    switch (j)
                    {
                    case 0: k = sprite[gs_cursprite].x; break;
                    case 1: k = sprite[gs_cursprite].y; break;
                    case 2: k = sprite[gs_cursprite].z; break;
                    case 3: k = sprite[gs_cursprite].sectnum; break;
                    case 4: k = sprite[gs_cursprite].statnum; break;
                    case 5: k = sprite[gs_cursprite].hitag; break;
                    case 6: k = sprite[gs_cursprite].lotag; break;
                    }
                }
                if (i==1)
                {
                    switch (j)
                    {
                    case 0:
                        k = sprite[gs_cursprite].cstat;
                        k &= gs_sprite[1][0];
                        break;
                    case 1: k = sprite[gs_cursprite].shade; break;
                    case 2: k = sprite[gs_cursprite].pal; break;
                    case 3:
                        k = gs_sprite[1][3];
                        if (k != sprite[gs_cursprite].xrepeat &&
                                k != sprite[gs_cursprite].yrepeat)
                            goto NEXTSPRITE;
                        break;
                    case 4:
                        k = gs_sprite[1][4];
                        if (k != sprite[gs_cursprite].xoffset &&
                                k != sprite[gs_cursprite].yoffset)
                            goto NEXTSPRITE;
                        break;
                    case 5: k = sprite[gs_cursprite].picnum; break;
                    }
                }
                if (i==2)
                {
                    switch (j)
                    {
                    case 0: k = sprite[gs_cursprite].ang; break;
                    case 1: k = sprite[gs_cursprite].xvel; break;
                    case 2: k = sprite[gs_cursprite].yvel; break;
                    case 3: k = sprite[gs_cursprite].zvel; break;
                    case 4: k = sprite[gs_cursprite].owner; break;
                    case 5: k = sprite[gs_cursprite].clipdist; break;
                    case 6: k = sprite[gs_cursprite].extra; break;
                    }
                }

                if (k != gs_sprite[i][j]) goto NEXTSPRITE;
            }

        // found matching sprite
        posx = sprite[gs_cursprite].x;
        posy = sprite[gs_cursprite].y;
        ang = sprite[gs_cursprite].ang;

        printmessage16("%s Sprite seach%s: found sprite %d", dir<0 ? "<" : ">",
                       did_wrap ? " (wrap)" : "", gs_cursprite);
        did_wrap = 0;
        return;

NEXTSPRITE:
        ;
    }
    printmessage16("%s Sprite search: none found", dir<0 ? "<" : ">");
}

static void Keys2d(void)
{
    short temp=0;
    int i=0, j,k;
    int repeatcountx=0,repeatcounty=0,smooshyalign,changedir;
    static int opointhighlight=-1, olinehighlight=-1, ocursectornum=-1;
    /*
       for(i=0;i<0x50;i++)
       {if(keystatus[i]==1) {Bsprintf(tempbuf,"key %d",i); printmessage16(tempbuf);
    }}
    */

    Bsprintf(tempbuf, "Mapster32" VERSION);
    printext16(9L,ydim2d-STATUS2DSIZ+9L,4,-1,tempbuf,0);
    printext16(8L,ydim2d-STATUS2DSIZ+8L,12,-1,tempbuf,0);

    updatesector(mousxplc,mousyplc,&cursectornum);
    searchsector=cursectornum;
    if ((totalclock > getmessagetimeoff) && (totalclock > (lastpm16time + 120*3)))
    {
        if (pointhighlight >= 16384)
        {
            char tmpbuf[2048];
            i = pointhighlight-16384;
            if (strlen(names[sprite[i].picnum]) > 0)
            {
                if (sprite[i].picnum==SECTOREFFECTOR)
                    Bsprintf(tmpbuf,"Sprite %d %s, hi:%d ex:%d",i,SectorEffectorText(i),sprite[i].hitag,sprite[i].extra);
                else Bsprintf(tmpbuf,"Sprite %d %s: lo:%d hi:%d ex:%d",i,names[sprite[i].picnum],sprite[i].lotag,sprite[i].hitag,sprite[i].extra);
            }
            else Bsprintf(tmpbuf,"Sprite %d picnum %d: lo:%d hi:%d ex:%d",i,sprite[i].picnum,sprite[i].lotag,sprite[i].hitag,sprite[i].extra);
            _printmessage16(tmpbuf);
        }
        else if ((linehighlight >= 0) && (sectorofwall(linehighlight) == cursectornum))
        {
            int dax, day, dist;
            dax = wall[linehighlight].x-wall[wall[linehighlight].point2].x;
            day = wall[linehighlight].y-wall[wall[linehighlight].point2].y;
            dist = ksqrt(dax*dax+day*day);
            Bsprintf(tempbuf,"Wall %d: length:%d lo:%d hi:%d ex:%d",linehighlight,dist,wall[linehighlight].lotag,wall[linehighlight].hitag,wall[linehighlight].extra);
            _printmessage16(tempbuf);
        }
        else if (cursectornum >= 0)
        {
            Bsprintf(tempbuf,"Sector %d: lo:%d hi:%d ex:%d",cursectornum,sector[cursectornum].lotag,sector[cursectornum].hitag,sector[cursectornum].extra);
            _printmessage16(tempbuf);
        }
        else _printmessage16("");
    }

    if (bstatus&1 || opointhighlight != pointhighlight || olinehighlight != linehighlight || ocursectornum != cursectornum)
    {
        if (pointhighlight >= 16384)
        {
            i = pointhighlight-16384;
            clearmidstatbar16();
            showspritedata((short)i);
        }
        else if ((linehighlight >= 0) && (bstatus&1 || sectorofwall(linehighlight) == cursectornum))
        {
            clearmidstatbar16();
            showwalldata((short)linehighlight);
        }
        else if (cursectornum >= 0)
        {
            clearmidstatbar16();
            showsectordata((short)cursectornum);
        }
        else clearmidstatbar16();
        opointhighlight = pointhighlight;
        olinehighlight = linehighlight;
        ocursectornum = cursectornum;
    }

    if (keystatus[KEYSC_QUOTE] && keystatus[KEYSC_Z]) // ' z
    {
        keystatus[KEYSC_Z] = 0;

        editorzrange[0]=getnumber16("Upper Z range: ",editorzrange[0],INT32_MAX,1);
        editorzrange[1]=getnumber16("Lower Z range: ",editorzrange[1],INT32_MAX,1);
        printmessage16("");
    }

    if (keystatus[0x14])  // T (tag)
    {
        char buffer[80];

        keystatus[0x14] = 0;
        if (keystatus[0x1d]|keystatus[0x9d])  //Ctrl-T
        {
            extern short showtags;

            showtags ^= 1;
            if (showtags == 0)
                printmessage16("Show tags OFF");
            else
                printmessage16("Show tags ON");
        }
        else if (keystatus[0x38]|keystatus[0xb8])  //ALT
        {
            if (pointhighlight >= 16384)
            {
                i = pointhighlight-16384;
                Bsprintf(buffer,"Sprite (%d) Lo-tag: ",i);
                if (sprite[i].picnum == SECTOREFFECTOR)
                    sprite[i].lotag = _getnumber16(buffer,sprite[i].lotag,65536L,0,(void *)SectorEffectorTagText);
                else sprite[i].lotag = getnumber16(buffer,sprite[i].lotag,65536L,0);
                clearmidstatbar16();
                showspritedata((short)i);
            }
            else if (linehighlight >= 0)
            {
                i = linehighlight;
                Bsprintf(buffer,"Wall (%d) Lo-tag: ",i);
                wall[i].lotag = getnumber16(buffer,wall[i].lotag,65536L,0);
                clearmidstatbar16();
                showwalldata((short)i);
            }
            printmessage16("");
        }
        else
        {
            for (i=0;i<numsectors;i++)
                if (inside(mousxplc,mousyplc,i) == 1)
                {
                    Bsprintf(buffer,"Sector (%d) Lo-tag: ",i);
                    j = qsetmode;
                    qsetmode = 200;
                    sector[i].lotag = _getnumber16(buffer,sector[i].lotag,65536L,0,(void *)ExtGetSectorType);
                    qsetmode = j;
                    clearmidstatbar16();
                    showsectordata((short)i);
                    break;
                }
            printmessage16("");
        }
    }

    if (keystatus[KEYSC_F1] || (keystatus[KEYSC_QUOTE] && keystatus[KEYSC_TILDE])) //F1 or ' ~
    {
        keystatus[KEYSC_F1]=0;
        clearmidstatbar16();

// PK_
        if (numhelppages>0) IntegratedHelp();
        else
        {
            begindrawing();
            for (i=0;i<MAXHELP2D;i++)
            {
                k = 0;
                j = 0;
                if (i > 9)
                {
                    j = 256;
                    k = 90;
                }
                printext16(j,ydim16+32+(i*9)-k,11,-1,Help2d[i],0);
            }
            enddrawing();
        }
    }

    getpoint(searchx,searchy,&mousxplc,&mousyplc);
    ppointhighlight = getpointhighlight(mousxplc,mousyplc, ppointhighlight);

    if ((ppointhighlight&0xc000) == 16384)
    {
        //   sprite[ppointhighlight&16383].cstat ^= 1;
        cursprite=(ppointhighlight&16383);
    }

    if (keystatus[KEYSC_F9]) // F9 f1=3b
    {
        keystatus[KEYSC_F9] = 0;
        Show2dText("sthelp.hlp");
    }

    /* start Mapster32 */

    if (keystatus[KEYSC_F4])
    {
        showfirstwall = !showfirstwall;
        message("Sector firstwall highlight %s",showfirstwall?"enabled":"disabled");
        keystatus[KEYSC_F4] = 0;
    }

    if (keystatus[KEYSC_M])  // M (tag)
    {
        keystatus[KEYSC_M] = 0;
        if (eitherALT)  //ALT
        {
            if (pointhighlight >= 16384)
            {
                i = pointhighlight-16384;
                Bsprintf(tempbuf,"Sprite %d Extra: ",i);
                sprite[i].extra = getnumber16(tempbuf,sprite[i].extra,65536L,1);
                clearmidstatbar16();
                showspritedata((short)i);
            }
            else if (linehighlight >= 0)
            {
                i = linehighlight;
                Bsprintf(tempbuf,"Wall %d Extra: ",i);
                wall[i].extra = getnumber16(tempbuf,wall[i].extra,65536L,1);
                clearmidstatbar16();
                showwalldata((short)i);
            }
            printmessage16("");
        }
        else
        {
            for (i=0;i<numsectors;i++)
                if (inside(mousxplc,mousyplc,i) == 1)
                {
                    Bsprintf(tempbuf,"Sector %d Extra: ",i);
                    sector[i].extra = getnumber16(tempbuf,sector[i].extra,65536L,1);
                    clearmidstatbar16();
                    showsectordata((short)i);
                    break;
                }
            printmessage16("");
        }
    }

    if (keystatus[KEYSC_SLASH])  // /?     Reset panning&repeat to 0
    {
        if ((ppointhighlight&0xc000) == 16384)
        {
            if (eitherSHIFT)
            {
                sprite[cursprite].xrepeat = sprite[cursprite].yrepeat;
            }
            else
            {
                sprite[cursprite].xrepeat = 64;
                sprite[cursprite].yrepeat = 64;
            }
        }
        keystatus[KEYSC_SLASH] = 0;
        asksave = 1;
    }

    if (keystatus[KEYSC_gLEFT] || keystatus[KEYSC_gRIGHT])  // 4 & 6 (keypad)
    {
        smooshyalign = keystatus[KEYSC_gKP5];
        if ((repeatcountx == 0) || (repeatcountx > 16))
        {
            changedir = 0;
            if (keystatus[KEYSC_gLEFT])  changedir = -1;
            if (keystatus[KEYSC_gRIGHT]) changedir = 1;

            if ((ppointhighlight&0xc000) == 16384)
            {
                sprite[cursprite].xrepeat = changechar(sprite[cursprite].xrepeat,changedir,smooshyalign,1);
                if (sprite[cursprite].xrepeat < 4)
                    sprite[cursprite].xrepeat = 4;
            }
            asksave = 1;
            repeatcountx = max(1,repeatcountx);
        }
        repeatcountx += synctics;
    }
    else
        repeatcountx = 0;

    if (keystatus[KEYSC_gUP] || keystatus[KEYSC_gDOWN])  // 2 & 8 (keypad)
    {
        smooshyalign = keystatus[KEYSC_gKP5];
        if ((repeatcounty == 0) || (repeatcounty > 16))
        {
            changedir = 0;
            if (keystatus[KEYSC_gUP])   changedir = -1;
            if (keystatus[KEYSC_gDOWN]) changedir = 1;

            if ((ppointhighlight&0xc000) == 16384)
            {
                sprite[cursprite].yrepeat = changechar(sprite[cursprite].yrepeat,changedir,smooshyalign,1);
                if (sprite[cursprite].yrepeat < 4)
                    sprite[cursprite].yrepeat = 4;
            }
            asksave = 1;
            repeatcounty = max(1,repeatcounty);
        }
        repeatcounty += synctics;
        //}
    }
    else
        repeatcounty = 0;

    if (keystatus[KEYSC_R])  // R (relative alignment, rotation)
    {
        if (pointhighlight >= 16384)
        {
            i = sprite[cursprite].cstat;
            if ((i&48) < 32) i += 16;

            else i &= ~48;
            sprite[cursprite].cstat = i;

            if (sprite[cursprite].cstat&16)
                sprintf(getmessage,"Sprite %d now wall aligned",cursprite);
            else if (sprite[cursprite].cstat&32)
                sprintf(getmessage,"Sprite %d now floor aligned",cursprite);
            else
                sprintf(getmessage,"Sprite %d now view aligned",cursprite);
            message(getmessage);
            asksave = 1;

        }
        keystatus[KEYSC_R] = 0;
    }


    if (keystatus[KEYSC_QUOTE] && keystatus[KEYSC_S]) // ' S
    {
        if (pointhighlight >= 16384)
        {
            keystatus[KEYSC_S] = 0;
            Bsprintf(tempbuf,"Sprite %d xrepeat: ",cursprite);
            sprite[cursprite].xrepeat=getnumber16(tempbuf, sprite[cursprite].xrepeat, 256,0);
            Bsprintf(tempbuf,"Sprite %d yrepeat: ",cursprite);
            sprite[cursprite].yrepeat=getnumber16(tempbuf, sprite[cursprite].yrepeat, 256,0);
            Bsprintf(tempbuf,"Sprite %d updated",i);
            printmessage16(tempbuf);
        }
    }

    if (keystatus[KEYSC_QUOTE] && keystatus[KEYSC_F]) // ' F
    {
        keystatus[KEYSC_F] = 0;
        FuncMenu();
    }

    if (keystatus[KEYSC_LBRACK]) // [     search backward
    {
        keystatus[KEYSC_LBRACK]=0;
        if (wallsprite==0)
        {
            SearchSectorsBackward();
        }
        else

            if (wallsprite==1)
            {
                if (curwallnum>0) curwallnum--;
                for (i=curwallnum;i>=0;i--)
                {
                    if (
                        (wall[i].picnum==wall[curwall].picnum)
                        &&((search_lotag==0)||
                           (search_lotag!=0 && search_lotag==wall[i].lotag))
                        &&((search_hitag==0)||
                           (search_hitag!=0 && search_hitag==wall[i].hitag))
                    )
                    {
                        posx=(wall[i].x)-(((wall[i].x)-(wall[wall[i].point2].x))/2);
                        posy=(wall[i].y)-(((wall[i].y)-(wall[wall[i].point2].y))/2);
                        printmessage16("< Wall search: found");
                        //                    curwallnum--;
                        keystatus[KEYSC_LBRACK]=0;
                        return;
                    }
                    curwallnum--;
                }
                printmessage16("< Wall search: none found");
            }
            else

                if (wallsprite==2)
                    DoSpriteSearch(-1);
#if 0
        {
            if (cursearchspritenum>0) cursearchspritenum--;
            for (i=cursearchspritenum;i>=0;i--)
            {

                if (
                    (sprite[i].picnum==sprite[cursearchsprite].picnum &&
                     sprite[i].statnum==0)
                    &&((search_lotag==0)||
                       (search_lotag!=0 && search_lotag==sprite[i].lotag))
                    &&((search_hitag==0)||
                       (search_hitag!=0 && search_hitag==sprite[i].hitag))
                )
                {
                    posx=sprite[i].x;
                    posy=sprite[i].y;
                    ang= sprite[i].ang;
                    printmessage16("< Sprite search: found");
                    //                    curspritenum--;
                    keystatus[KEYSC_LBRACK]=0;
                    return;
                }
                cursearchspritenum--;
            }
            printmessage16("< Sprite search: none found");
        }
#endif
    }


    if (keystatus[KEYSC_RBRACK]) // ]     search forward
    {
        keystatus[KEYSC_RBRACK]=0;
        if (wallsprite==0)
        {
            SearchSectorsForward();
        }
        else if (wallsprite==1)
        {
            if (curwallnum<MAXWALLS) curwallnum++;
            for (i=curwallnum;i<=MAXWALLS;i++)
            {
                if (
                    (wall[i].picnum==wall[curwall].picnum)
                    &&((search_lotag==0)||
                       (search_lotag!=0 && search_lotag==wall[i].lotag))
                    &&((search_hitag==0)||
                       (search_hitag!=0 && search_hitag==wall[i].hitag))
                )
                {
                    posx=(wall[i].x)-(((wall[i].x)-(wall[wall[i].point2].x))/2);
                    posy=(wall[i].y)-(((wall[i].y)-(wall[wall[i].point2].y))/2);
                    printmessage16("> Wall search: found");
                    //                    curwallnum++;
                    keystatus[KEYSC_RBRACK]=0;
                    return;
                }
                curwallnum++;
            }
            printmessage16("> Wall search: none found");
        }
        else if (wallsprite==2)
            DoSpriteSearch(1);
#if 0
        {
            if (cursearchspritenum<MAXSPRITES) cursearchspritenum++;
            for (i=cursearchspritenum;i<=MAXSPRITES;i++)
            {
                if (
                    (sprite[i].picnum==sprite[cursearchsprite].picnum &&
                     sprite[i].statnum==0)
                    &&((search_lotag==0)||
                       (search_lotag!=0 && search_lotag==sprite[i].lotag))
                    &&((search_hitag==0)||
                       (search_hitag!=0 && search_hitag==sprite[i].hitag))
                )
                {
                    posx=sprite[i].x;
                    posy=sprite[i].y;
                    ang= sprite[i].ang;
                    printmessage16("> Sprite search: found");
                    //                    curspritenum++;
                    keystatus[KEYSC_RBRACK]=0;
                    return;
                }
                cursearchspritenum++;
            }
            printmessage16("> Sprite search: none found");
        }
#endif
    }

    {
        static int autogrid = 0;

        if (keystatus[KEYSC_G])  // G (grid on/off)
        {
            if (autogrid)
            {
                grid = 0;
                if (eitherSHIFT)
                    grid = 8;
                autogrid = 0;
            }
            else
            {
                grid += eitherSHIFT?-1:1;
                if (grid == -1 || grid == 9)
                {
                    switch (grid)
                    {
                    case -1:
                        autogrid = 1;
                        grid = 8;
                        break;
                    case 9:
                        autogrid = 1;
                        grid = 0;
                        break;
                    }
                }
            }
            if (autogrid) sprintf(tempbuf,"Grid size: 9 (autosize)");
            else if (!grid) sprintf(tempbuf,"Grid off");
            else sprintf(tempbuf,"Grid size: %d (%d units)",grid,2048>>grid);
            printmessage16(tempbuf);
            keystatus[KEYSC_G] = 0;
        }
        if (autogrid)
        {
            grid = zoom+512;
            if (grid > 16384) grid = 16384;
            grid = scale(grid,6,6144);
            if (grid > 7) grid = 7;
            if (grid < 0) grid = 0;
        }
    }

    if (keystatus[KEYSC_QUOTE] && keystatus[KEYSC_L]) // ' L
    {
        if (pointhighlight >= 16384)
        {
            i = pointhighlight - 16384;
            Bsprintf(tempbuf,"Sprite %d x: ",i);
            sprite[i].x = getnumber16(tempbuf,sprite[i].x,131072,1);
            Bsprintf(tempbuf,"Sprite %d y: ",i);
            sprite[i].y = getnumber16(tempbuf,sprite[i].y,131072,1);
            Bsprintf(tempbuf,"Sprite %d z: ",i);
            sprite[i].z = getnumber16(tempbuf,sprite[i].z,8388608,1);
            Bsprintf(tempbuf,"Sprite %d angle: ",i);
            sprite[i].ang = getnumber16(tempbuf,sprite[i].ang,2048L,0);
            Bsprintf(tempbuf,"Sprite %d updated",i);
            printmessage16(tempbuf);
        }

        else if (pointhighlight >= 0 /*<= 16383*/)
        {
            i = linehighlight;
            j = wall[i].x;
            k = wall[i].y;
            Bsprintf(tempbuf,"Wall %d x: ",i);
            j = getnumber16(tempbuf,j,editorgridextent,1);
            Bsprintf(tempbuf,"Wall %d y: ",i);
            k = getnumber16(tempbuf,k,editorgridextent,1);
            dragpoint(i, j, k);
            Bsprintf(tempbuf,"Wall %d updated",i);
            printmessage16(tempbuf);
        }

        keystatus[KEYSC_L] = 0;
    }


    if (keystatus[KEYSC_QUOTE] && keystatus[KEYSC_3]) // ' 3
    {
        onnames++;
        if (onnames>8) onnames=0;
        keystatus[KEYSC_3]=0;
        Bsprintf(tempbuf,"Mode %d %s",onnames,SpriteMode[onnames]);
        printmessage16(tempbuf);
        //   clearmidstatbar16();
        //   for(i=0;i<MAXMODE32D;i++) {printext16(0*8,ydim16+32+(i*8),15,-1,SpriteMode[i],0);
    }
    //   Ver();

    /*
     if(keystatus[KEYSC_QUOTE] && keystatus[KEYSC_5]) // ' 5
     {
    	keystatus[KEYSC_5]=0;
       sprintf(tempbuf,"Power-Up Ammo now equals Normal");
       printmessage16(tempbuf);
        for(i=0;i<MAXSPRITES;i++)
            {
         if(sprite[i].picnum>=20 && sprite[i].picnum<=59)
         {
            sprite[i].xrepeat = 32;
            sprite[i].yrepeat = 32;
         }
        }

     }
    */

    //  What the fuck is this supposed to do?

    /* Motorcycle ha ha ha
    if(keystatus[KEYSC_QUOTE] && keystatus[KEYSC_5]) // ' 5
    {
    	keystatus[KEYSC_5]=0;
            sidemode++; if (sidemode > 2) sidemode = 0;
            if (sidemode == 1)
            {
                    editstatus = 0;
                    zmode = 2;
                    posz = ((sector[cursectnum].ceilingz+sector[cursectnum].floorz)>>1);
            }
            else
            {
                    editstatus = 1;
                    zmode = 1;
            }
    }
    */

    if (keystatus[KEYSC_QUOTE] && keystatus[KEYSC_7]) // ' 7 : swap hilo
    {
        keystatus[KEYSC_7]=0;

        if (pointhighlight >= 16384)
        {
            temp=sprite[cursprite].lotag;
            sprite[cursprite].lotag=sprite[cursprite].hitag;
            sprite[cursprite].hitag=temp;
            Bsprintf(tempbuf,"Sprite %d tags swapped",cursprite);
            printmessage16(tempbuf);
        }
        else if (linehighlight >= 0)
        {
            temp=wall[linehighlight].lotag;
            wall[linehighlight].lotag=wall[linehighlight].hitag;
            wall[linehighlight].hitag=temp;
            Bsprintf(tempbuf,"Wall %d tags swapped",linehighlight);
            printmessage16(tempbuf);
        }
    }

    if (keystatus[KEYSC_QUOTE] && keystatus[KEYSC_J]) // ' J
    {
        posx=getnumber16("X-coordinate:    ",posx,131072L,1);
        posy=getnumber16("Y-coordinate:    ",posy,131072L,1);
        Bsprintf(tempbuf,"Current pos now (%d, %d)",posx,posy);
        printmessage16(tempbuf);
        keystatus[KEYSC_J]=0;
    }

}// end key2d

static void InitCustomColors(void)
{
    /* blue */
    /*    vgapal16[9*4+0] = 63;
        vgapal16[9*4+1] = 31;
        vgapal16[9*4+2] = 7; */

    /* orange */
    vgapal16[31*4+0] = 20; // blue
    vgapal16[31*4+1] = 45; // green
    vgapal16[31*4+2] = 60; // red

    vgapal16[39*4+0] = 36;
    vgapal16[39*4+1] = 53;
    vgapal16[39*4+2] = 63;


    /* light yellow */
    vgapal16[22*4+0] = 51;
    vgapal16[22*4+1] = 63;
    vgapal16[22*4+2] = 63;

    /* grey */
    vgapal16[23*4+0] = 45;
    vgapal16[23*4+1] = 45;
    vgapal16[23*4+2] = 45;

    /* blue */
    vgapal16[24*4+0] = 51;
    vgapal16[24*4+1] = 41;
    vgapal16[24*4+2] = 12;

    vgapal16[32*4+0] = 60;
    vgapal16[32*4+1] = 50;
    vgapal16[32*4+2] = 21;

    // grid color
    vgapal16[25*4+0] = 19;
    vgapal16[25*4+1] = 17;
    vgapal16[25*4+2] = 17;

    vgapal16[26*4+0] = 24;
    vgapal16[26*4+1] = 24;
    vgapal16[26*4+2] = 24;

    vgapal16[33*4+0] = 15; // blue
    vgapal16[33*4+1] = 30; // green
    vgapal16[33*4+2] = 45; // red

    vgapal16[41*4+0] = 24;
    vgapal16[41*4+1] = 40;
    vgapal16[41*4+2] = 48;


}

void ExtPreSaveMap(void)
{
    if (fixmapbeforesaving)
    {
        int i, startwall, j, endwall;

        for (i=0;i<numsectors;i++)
        {
            startwall = sector[i].wallptr;
            for (j=startwall;j<numwalls;j++)
                if (wall[j].point2 < startwall) startwall = wall[j].point2;
            sector[i].wallptr = startwall;
        }
        for (i=numsectors-2;i>=0;i--)
            sector[i].wallnum = sector[i+1].wallptr-sector[i].wallptr;
        sector[numsectors-1].wallnum = numwalls-sector[numsectors-1].wallptr;

        for (i=0;i<numwalls;i++)
        {
            wall[i].nextsector = -1;
            wall[i].nextwall = -1;
        }
        for (i=0;i<numsectors;i++)
        {
            startwall = sector[i].wallptr;
            endwall = startwall + sector[i].wallnum;
            for (j=startwall;j<endwall;j++)
                checksectorpointer((short)j,(short)i);
        }
    }
}

void ExtPreLoadMap(void)
{}

static void G_ShowParameterHelp(void)
{
    char *s = "Usage: mapster32 [OPTIONS] [FILE]\n\n"
              "-gFILE, -grp FILE\tUse extra group file FILE\n"
              "-hFILE\t\tUse definitions file FILE\n"
              "-jDIR, -game_dir DIR\n\t\tAdds DIR to the file path stack\n"
              "-nocheck\t\tDisables map pointer checking when saving\n"
#if defined RENDERTYPEWIN || (defined RENDERTYPESDL && !defined __APPLE__ && defined HAVE_GTK2)
              "-setup\t\tDisplays the configuration dialog\n"
#endif
#if !defined(_WIN32)
              "-usecwd\t\tRead game data and configuration file from working directory\n"
#endif
              "\n-?, -help, --help\tDisplay this help message and exit"
              ;
    wm_msgbox("Mapster32"VERSION BUILDDATE,s);
}

static void AddGamePath(const char *buffer)
{
    struct strllist *s;
    s = (struct strllist *)Bcalloc(1,sizeof(struct strllist));
    s->str = strdup(buffer);

    if (CommandPaths)
    {
        struct strllist *t;
        for (t = CommandPaths; t->next; t=t->next) ;
        t->next = s;
        return;
    }
    CommandPaths = s;
}

static void G_AddGroup(const char *buffer)
{
    struct strllist *s;
    s = (struct strllist *)Bcalloc(1,sizeof(struct strllist));
    s->str = Bstrdup(buffer);
    if (Bstrchr(s->str,'.') == 0)
        Bstrcat(s->str,".grp");

    if (CommandGrps)
    {
        struct strllist *t;
        for (t = CommandGrps; t->next; t=t->next) ;
        t->next = s;
        return;
    }
    CommandGrps = s;
}

#define COPYARG(i) \
    Bmemcpy(&testplay_addparam[j], argv[i], lengths[i]); \
    j += lengths[i]; \
    testplay_addparam[j++] = ' ';

#if defined(RENDERTYPEWIN) && defined(USE_OPENGL)
extern char forcegl;
#endif

static void G_CheckCommandLine(int argc, const char **argv)
{
    int i = 1, j, maxlen=0, *lengths;
    char *c, *k;

    mapster32_fullpath = (char*)argv[0];

    if (argc > 1)
    {
        lengths = Bmalloc(argc*sizeof(int));
        for (j=1; j<argc; j++) maxlen += (lengths[j] = Bstrlen(argv[j]));

        testplay_addparam = Bmalloc(maxlen+argc);
        testplay_addparam[0] = 0;

        j = 0;

        while (i < argc)
        {
            c = (char *)argv[i];
            if (((*c == '/') || (*c == '-')))
            {
                if (!Bstrcasecmp(c+1,"?") || !Bstrcasecmp(c+1,"help") || !Bstrcasecmp(c+1,"-help"))
                {
                    G_ShowParameterHelp();
                    exit(0);
                }

                if (!Bstrcasecmp(c+1, "g") || !Bstrcasecmp(c+1,  "grp"))
                {
                    if (argc > i+1)
                    {
                        G_AddGroup(argv[i+1]);
                        COPYARG(i);
                        COPYARG(i+1);
                        i++;
                    }
                    i++;
                    continue;
                }

                if (!Bstrcasecmp(c+1,"game_dir"))
                {
                    if (argc > i+1)
                    {
#if defined(POLYMOST) && defined(USE_OPENGL)
                        extern char TEXCACHEFILE[BMAX_PATH];

                        Bsprintf(tempbuf,"%s/%s",argv[i+1],TEXCACHEFILE);
                        Bstrcpy(TEXCACHEFILE,tempbuf);
#endif
                        AddGamePath(argv[i+1]);

                        COPYARG(i);
                        COPYARG(i+1);
                        i++;
                    }
                    i++;
                    continue;
                }
                if (!Bstrcasecmp(c+1,"cfg"))
                {
                    if (argc > i+1)
                    {
                        Bstrcpy(setupfilename,argv[i+1]);
                        i++;
                    }
                    i++;
                    continue;
                }
                if (!Bstrcasecmp(c+1,"gamegrp"))
                {
                    if (argc > i+1)
                    {
                        Bstrcpy(defaultduke3dgrp,argv[i+1]);
                        COPYARG(i);
                        COPYARG(i+1);
                        i++;
                    }
                    i++;
                    continue;
                }
                if (!Bstrcasecmp(c+1,"nam"))
                {
                    strcpy(duke3dgrp, "nam.grp");
                    COPYARG(i);
                    i++;
                    continue;
                }
                if (!Bstrcasecmp(c+1,"ww2gi"))
                {
                    strcpy(duke3dgrp, "ww2gi.grp");
                    COPYARG(i);
                    i++;
                    continue;
                }
                if (!Bstrcasecmp(c+1,"nocheck"))
                {
                    initprintf("Map pointer checking disabled\n");
                    fixmapbeforesaving = 0;
                    i++;
                    continue;
                }
                if (!Bstrcasecmp(c+1,"noautoload"))
                {
                    initprintf("Autoload disabled\n");
                    NoAutoLoad = 1;
                    COPYARG(i);
                    i++;
                    continue;
                }
#if !defined(_WIN32)
                if (!Bstrcasecmp(c+1,"usecwd"))
                {
                    usecwd = 1;
                    COPYARG(i);
                    i++;
                    continue;
                }
#endif
#if defined(RENDERTYPEWIN) && defined(USE_OPENGL)
                if (!Bstrcasecmp(c+1,"forcegl"))
                {
                    forcegl = 1;
                    i++;
                    continue;
                }
#endif
            }

            if ((*c == '/') || (*c == '-'))
            {
                c++;
                switch (*c)
                {
                case 'h':
                case 'H':
                    c++;
                    if (*c)
                    {
                        defsfilename = c;
                        COPYARG(i);
                        initprintf("Using DEF file: %s.\n",defsfilename);
                    }
                    break;
                case 'j':
                case 'J':
                    c++;
                    if (!*c) break;
                    AddGamePath(c);
                    COPYARG(i);
                    break;
                case 'g':
                case 'G':
                    c++;
                    if (!*c) break;
                    G_AddGroup(c);
                    COPYARG(i);
                    break;
                }
            }
            else
            {
                k = Bstrchr(c,'.');
                if (k)
                {
                    if (!Bstrcasecmp(k,".grp") || !Bstrcasecmp(k,".zip"))
                    {
                        G_AddGroup(argv[i++]);
                        COPYARG(i);
                        continue;
                    }
                    if (!Bstrcasecmp(k,".def"))
                    {
                        defsfilename = (char *)argv[i++];
                        COPYARG(i);
                        initprintf("Using DEF file: %s.\n",defsfilename);
                        continue;
                    }
                }
            }
            i++;
        }

        Bfree(lengths);

        if (j > 0)
        {
            testplay_addparam[j-1] = 0;
            testplay_addparam = Brealloc(testplay_addparam, j*sizeof(char));
        }
        else
        {
            Bfree(testplay_addparam);
            testplay_addparam = NULL;
        }
    }
}
#undef COPYARG

int ExtPreInit(int argc,const char **argv)
{
    wm_setapptitle("Mapster32"VERSION BUILDDATE);

#ifdef _WIN32
    tempbuf[GetModuleFileName(NULL,tempbuf,BMAX_PATH)] = 0;
    Bcorrectfilename(tempbuf,1);
    chdir(tempbuf);
#endif

    OSD_SetLogFile("mapster32.log");
    OSD_SetVersionString("Mapster32"VERSION,0,2);
    initprintf("Mapster32"VERSION BUILDDATE"\n");
//    initprintf("Copyright (c) 2008 EDuke32 team\n");

    G_CheckCommandLine(argc,argv);

    return 0;
}

static int osdcmd_quit(const osdfuncparm_t *parm)
{
    UNREFERENCED_PARAMETER(parm);
    clearfilenames();
    ExtUnInit();
    uninitengine();

    exit(0);

    return OSDCMD_OK;
}

static int osdcmd_editorgridextent(const osdfuncparm_t *parm)
{
    int i;
    extern int editorgridextent;

    if (parm->numparms == 0)
    {
        OSD_Printf("\"editorgridextent\" is \"%d\"\n", editorgridextent);
        return OSDCMD_SHOWHELP;
    }
    else if (parm->numparms != 1) return OSDCMD_SHOWHELP;

    i = Batol(parm->parms[0]);

    if (i >= 65536 && i <= 524288)
    {
        editorgridextent = i;
        OSD_Printf("editorgridextent %d\n", editorgridextent);
    }
    else OSD_Printf("editorgridextent: value out of range\n");
    return OSDCMD_OK;
}

static int osdcmd_addpath(const osdfuncparm_t *parm)
{
    char pathname[BMAX_PATH];

    if (parm->numparms != 1) return OSDCMD_SHOWHELP;

    strcpy(pathname,parm->parms[0]);
    addsearchpath(pathname);
    return OSDCMD_OK;
}

static int osdcmd_initgroupfile(const osdfuncparm_t *parm)
{
    char file[BMAX_PATH];

    if (parm->numparms != 1) return OSDCMD_SHOWHELP;

    strcpy(file,parm->parms[0]);
    initgroupfile(file);
    return OSDCMD_OK;
}

static int osdcmd_echo(const osdfuncparm_t *parm)
{
    int i;
    for (i = 0; i < parm->numparms; i++)
    {
        if (i > 0) OSD_Printf(" ");
        OSD_Printf("%s", parm->parms[i]);
    }
    OSD_Printf("\n");

    return OSDCMD_OK;
}

static int osdcmd_fileinfo(const osdfuncparm_t *parm)
{
    unsigned int crc, length;
    int i,j;
    char buf[256];

    if (parm->numparms != 1) return OSDCMD_SHOWHELP;

    if ((i = kopen4load((char *)parm->parms[0],0)) < 0)
    {
        OSD_Printf("fileinfo: File \"%s\" not found.\n", parm->parms[0]);
        return OSDCMD_OK;
    }

    length = kfilelength(i);

    crc32init(&crc);
    do
    {
        j = kread(i,buf,256);
        crc32block(&crc,(unsigned char *)buf,j);
    }
    while (j == 256);
    crc32finish(&crc);

    kclose(i);

    OSD_Printf("fileinfo: %s\n"
               "  File size: %d\n"
               "  CRC-32:    %08X\n",
               parm->parms[0], length, crc);

    return OSDCMD_OK;
}

static int osdcmd_sensitivity(const osdfuncparm_t *parm)
{
    if (parm->numparms != 1)
    {
        OSD_Printf("\"sensitivity\" is \"%.2f\"\n",msens);
        return OSDCMD_SHOWHELP;
    }
    msens = atof(parm->parms[0]);
    OSD_Printf("sensitivity %.2f\n",msens);
    return OSDCMD_OK;
}

static int osdcmd_gamma(const osdfuncparm_t *parm)
{
    extern short brightness;

    if (parm->numparms != 1)
    {
        OSD_Printf("\"gamma\" \"%d\"\n",brightness);
        return OSDCMD_SHOWHELP;
    }
    brightness = atoi(parm->parms[0]);
    setbrightness(brightness,palette,0);
    OSD_Printf("gamma %d\n",brightness);
    return OSDCMD_OK;
}

static int osdcmd_noclip(const osdfuncparm_t *parm)
{
    UNREFERENCED_PARAMETER(parm);
    noclip = !noclip;

    return OSDCMD_OK;
}

static int osdcmd_testplay_addparam(const osdfuncparm_t *parm)
{
    int slen;

    if (parm->numparms != 1)
    {
        OSD_Printf("additional parameters for test playing: %s%s%s\n",
                   testplay_addparam ? "\"" : "",
                   testplay_addparam ? testplay_addparam : "<empty>",
                   testplay_addparam ? "\"" : "");
        return OSDCMD_OK;
    }

    slen = Bstrlen(parm->parms[0]);

    if (slen > 0)
    {
        if (!testplay_addparam)
            testplay_addparam = Bmalloc(slen+1);
        else
            testplay_addparam = Brealloc(testplay_addparam, slen+1);

        Bmemcpy(testplay_addparam, parm->parms[0], slen);
        testplay_addparam[slen] = 0;
    }
    else
    {
        if (testplay_addparam)
        {
            Bfree(testplay_addparam);
            testplay_addparam = NULL;
        }
    }

    return OSDCMD_OK;
}

static int osdcmd_showheightindicators(const osdfuncparm_t *parm)
{
    extern int showheightindicators;

    if (parm->numparms == 1)
        showheightindicators = min(max(atoi(parm->parms[0]),0),2);

    OSD_Printf("height indicators: %s\n",
               showheightindicators==0 ? "none" :
               (showheightindicators==1 ? "two-sided walls only" : "all"));

    return OSDCMD_OK;
}

//PK vvv ------------
static int osdcmd_vars_pk(const osdfuncparm_t *parm)
{
    int showval = (parm->numparms < 1);

    // this is something of a misnomer, since it's actually accel+decel
    if (!Bstrcasecmp(parm->name, "pk_turnaccel"))
    {
        if (showval) { OSD_Printf("Turning acceleration+declaration is %d\n", pk_turnaccel); }
        else
        {
            pk_turnaccel = atoi(parm->parms[0]);
            pk_turnaccel = pk_turnaccel<=pk_turndecel ? (pk_turndecel+1):pk_turnaccel;
            pk_turnaccel = pk_turnaccel>256 ? 256:pk_turnaccel;
        }
    }
    else if (!Bstrcasecmp(parm->name, "pk_turndecel"))
    {
        if (showval) { OSD_Printf("Turning deceleration is %d\n", pk_turndecel); }
        else
        {
            pk_turndecel = atoi(parm->parms[0]);
            pk_turndecel = pk_turndecel<=0 ? 1:pk_turndecel;
            pk_turndecel = pk_turndecel>=pk_turnaccel ? (pk_turnaccel-1):pk_turndecel;
            pk_turndecel = pk_turndecel>128 ? 128:pk_turndecel;
        }
    }
    else if (!Bstrcasecmp(parm->name, "pk_quickmapcycling"))
    {
        OSD_Printf("Quick map cycling ((LShift-)Ctrl-X): %s\n",
                   (quickmapcycling = !quickmapcycling) ? "enabled":"disabled");
    }
    else if (!Bstrcasecmp(parm->name, "pk_uedaccel"))
    {
        if (showval) { OSD_Printf("UnrealEd mouse navigation acceleration is %d\n", pk_uedaccel); }
        else
        {
            pk_uedaccel = atoi(parm->parms[0]);
            pk_uedaccel = pk_uedaccel<0 ? 0:pk_uedaccel;
            pk_uedaccel = pk_uedaccel>5 ? 5:pk_uedaccel;
        }
    }
    return OSDCMD_OK;
}

static int registerosdcommands(void)
{
    OSD_RegisterFunction("addpath","addpath <path>: adds path to game filesystem", osdcmd_addpath);

    OSD_RegisterFunction("echo","echo [text]: echoes text to the console", osdcmd_echo);
    OSD_RegisterFunction("editorgridextent","editorgridextent: sets the size of the 2D mode editing grid",osdcmd_editorgridextent);

    OSD_RegisterFunction("fileinfo","fileinfo <file>: gets a file's information", osdcmd_fileinfo);

    OSD_RegisterFunction("gamma","gamma <value>: changes brightness", osdcmd_gamma);

    OSD_RegisterFunction("initgroupfile","initgroupfile <path>: adds a grp file into the game filesystem", osdcmd_initgroupfile);

    OSD_RegisterFunction("noclip","noclip: toggles clipping mode", osdcmd_noclip);

    OSD_RegisterFunction("quit","quit: exits the game immediately", osdcmd_quit);
    OSD_RegisterFunction("exit","exit: exits the game immediately", osdcmd_quit);

    OSD_RegisterFunction("sensitivity","sensitivity <value>: changes the mouse sensitivity", osdcmd_sensitivity);

    //PK
    OSD_RegisterFunction("pk_turnaccel", "pk_turnaccel: sets turning acceleration", osdcmd_vars_pk);
    OSD_RegisterFunction("pk_turndecel", "pk_turndecel: sets turning deceleration", osdcmd_vars_pk);
    OSD_RegisterFunction("pk_uedaccel", "pk_uedaccel: sets UnrealEd movement speed factor (0-5, exponentially)", osdcmd_vars_pk);
    OSD_RegisterFunction("pk_quickmapcycling", "pk_quickmapcycling: allows cycling of maps with (Shift-)Ctrl-X", osdcmd_vars_pk);
    OSD_RegisterFunction("testplay_addparam", "testplay_addparam \"string\": set additional parameters for test playing", osdcmd_testplay_addparam);
    OSD_RegisterFunction("showheightindicators", "showheightindicators [012]: toggles height indicators in 2D mode", osdcmd_showheightindicators);
    return 0;
}
#define DUKEOSD
#ifdef DUKEOSD
void GAME_drawosdchar(int x, int y, char ch, int shade, int pal)
{
    int ac;

    if (ch == 32) return;
    ac = ch-'!'+STARTALPHANUM;
    if (ac < STARTALPHANUM || ac > ENDALPHANUM) return;

    rotatesprite(((x<<3)+x)<<16, (y<<3)<<16, 65536l, 0, ac, shade, pal, 8|16, 0, 0, xdim-1, ydim-1);
}

void GAME_drawosdstr(int x, int y, char *ch, int len, int shade, int pal)
{
    int ac;

    for (x = (x<<3)+x; len>0; len--, ch++, x++)
    {
        if (*ch == 32)
        {
            x+=5;
            continue;
        }
        ac = *ch-'!'+STARTALPHANUM;
        if (ac < STARTALPHANUM || ac > ENDALPHANUM) return;

        rotatesprite(x<<16, (y<<3)<<16, 65536l, 0, ac, shade, pal, 8|16, 0, 0, xdim-1, ydim-1);
        if (*ch >= '0' && *ch <= '9') x+=8;
        else x += tilesizx[ac];
    }
}

static int GetTime(void)
{
    return totalclock;
}

void GAME_drawosdcursor(int x, int y, int type, int lastkeypress)
{
    int ac;

    if (type) ac = SMALLFNTCURSOR;
    else ac = '_'-'!'+STARTALPHANUM;

    if (!((GetTime()-lastkeypress) & 0x40l))
        rotatesprite(((x<<3)+x)<<16, ((y<<3)+(type?-1:2))<<16, 65536l, 0, ac, 0, 8, 8|16, 0, 0, xdim-1, ydim-1);
}

int GAME_getcolumnwidth(int w)
{
    return w/9;
}

int GAME_getrowheight(int w)
{
    return w>>3;
}

//#define BGTILE 311
//#define BGTILE 1156
#define BGTILE 1141	// BIGHOLE
#define BORDTILE 3250	// VIEWBORDER
#define BITSTH 1+32+8+16	// high translucency
#define BITSTL 1+8+16	// low translucency
#define BITS 8+16+64		// solid
#define SHADE 16
#define PALETTE 4
void GAME_clearbackground(int c, int r)
{
    int x, y, xsiz, ysiz, tx2, ty2;
    int daydim, bits;

    UNREFERENCED_PARAMETER(c);
    /*
    #ifdef _WIN32
        if (qsetmode != 200)
        {
            OSD_SetFunctions(
                NULL,
                NULL,
                NULL,
                NULL,
                NULL,
                NULL,
                (int(*)(void))GetTime,
                NULL
            );
            return;
        }
    #endif
    */
    if (getrendermode() < 3) bits = BITS;
    else bits = BITSTL;

    daydim = r<<3;

    xsiz = tilesizx[BGTILE];
    tx2 = xdim/xsiz;
    ysiz = tilesizy[BGTILE];
    ty2 = daydim/ysiz;

    for (x=0;x<=tx2;x++)
        for (y=0;y<=ty2;y++)
            rotatesprite(x*xsiz<<16,y*ysiz<<16,65536L,0,BGTILE,SHADE,PALETTE,bits,0,0,xdim,daydim);

    xsiz = tilesizy[BORDTILE];
    tx2 = xdim/xsiz;
    ysiz = tilesizx[BORDTILE];

    for (x=0;x<=tx2;x++)
        rotatesprite(x*xsiz<<16,(daydim+ysiz+1)<<16,65536L,1536,BORDTILE,SHADE-12,PALETTE,BITS,0,0,xdim,daydim+ysiz+1);
}
#endif

enum
{
    T_EOF = -2,
    T_ERROR = -1,
    T_INCLUDE = 0,
    T_DEFINE = 1,
    T_LOADGRP,
    T_TILEGROUP,
    T_TILE,
    T_TILERANGE,
    T_HOTKEY,
    T_TILES,
    T_NOAUTOLOAD,
    T_COLORS,

    T_ALPHABET,
    T_MAP,
    T_MAPA,
    T_MAPRANGE,
    T_MAPRANGEA,
    T_OFFSET,
    T_OFFSETA,
};

typedef struct
{
    char *text;
    int tokenid;
}
tokenlist;

static int getatoken(scriptfile *sf, tokenlist *tl, int ntokens)
{
    char *tok;
    int i;

    if (!sf) return T_ERROR;
    tok = scriptfile_gettoken(sf);
    if (!tok) return T_EOF;

    for (i=0;i<ntokens;i++)
    {
        if (!Bstrcasecmp(tok, tl[i].text))
            return tl[i].tokenid;
    }
    return T_ERROR;
}

static void DoAutoload(const char *fn)
{
    Bsprintf(tempbuf,"autoload/%s",fn);
    getfilenames(tempbuf,"*.grp");
    while (findfiles) { Bsprintf(tempbuf,"autoload/%s/%s",fn,findfiles->name); initprintf("Using group file '%s'.\n",tempbuf); initgroupfile(tempbuf); findfiles = findfiles->next; }
    Bsprintf(tempbuf,"autoload/%s",fn);
    getfilenames(tempbuf,"*.zip");
    while (findfiles) { Bsprintf(tempbuf,"autoload/%s/%s",fn,findfiles->name); initprintf("Using group file '%s'.\n",tempbuf); initgroupfile(tempbuf); findfiles = findfiles->next; }
    Bsprintf(tempbuf,"autoload/%s",fn);
    getfilenames(tempbuf,"*.pk3");
    while (findfiles) { Bsprintf(tempbuf,"autoload/%s/%s",fn,findfiles->name); initprintf("Using group file '%s'.\n",tempbuf); initgroupfile(tempbuf); findfiles = findfiles->next; }
}

int parsegroupfiles(scriptfile *script)
{
    int tokn;
    char *cmdtokptr;

    tokenlist grptokens[] =
    {
        { "include",         T_INCLUDE },
        { "#include",        T_INCLUDE },
        { "loadgrp",         T_LOADGRP },
        { "noautoload",      T_NOAUTOLOAD }
    };

    while (1)
    {
        tokn = getatoken(script,grptokens,sizeof(grptokens)/sizeof(tokenlist));
        cmdtokptr = script->ltextptr;
        switch (tokn)
        {
        case T_LOADGRP:
        {
            char *fn;

            pathsearchmode = 1;
            if (!scriptfile_getstring(script,&fn))
            {
                int j = initgroupfile(fn);

                if (j == -1)
                    initprintf("Could not find group file '%s'.\n",fn);
                else
                {
                    initprintf("Using group file '%s'.\n",fn);
                    if (!NoAutoLoad)
                        DoAutoload(fn);
                }

            }
            pathsearchmode = 0;
        }
        break;
        case T_INCLUDE:
        {
            char *fn;
            if (!scriptfile_getstring(script,&fn))
            {
                scriptfile *included;

                included = scriptfile_fromfile(fn);
                if (!included)
                {
                    initprintf("Warning: Failed including %s on line %s:%d\n",
                               fn, script->filename,scriptfile_getlinum(script,cmdtokptr));
                }
                else
                {
                    parsegroupfiles(included);
                    scriptfile_close(included);
                }
            }
            break;
        }
        break;
        case T_NOAUTOLOAD:
            NoAutoLoad = 1;
            break;
        case T_EOF:
            return(0);
        default:
            break;
        }
    }
    return 0;
}

int loadgroupfiles(char *fn)
{
    scriptfile *script;

    script = scriptfile_fromfile(fn);
    if (!script) return -1;

    parsegroupfiles(script);

    scriptfile_close(script);
    scriptfile_clearsymbols();

    return 0;
}

int parsetilegroups(scriptfile *script)
{
    int tokn;
    char *cmdtokptr;

    tokenlist tgtokens[] =
    {
        { "include",         T_INCLUDE          },
        { "#include",        T_INCLUDE          },
        { "define",          T_DEFINE           },
        { "#define",         T_DEFINE           },
        { "tilegroup",       T_TILEGROUP        },
        { "spritehotkey",    T_HOTKEY           },
        { "alphabet",        T_ALPHABET         },
    };

    while (1)
    {
        tokn = getatoken(script,tgtokens,sizeof(tgtokens)/sizeof(tokenlist));
        cmdtokptr = script->ltextptr;
        switch (tokn)
        {
        case T_HOTKEY:
        {
            int i, j;
            if (scriptfile_getsymbol(script,&i)) break;
            if (scriptfile_getsymbol(script,&j)) break;
            if (i < 0 || i > 9 || j < 0 || j >= MAXTILES) break;
            prefixtiles[i] = j;
            break;
        }
        case T_INCLUDE:
        {
            char *fn;
            if (!scriptfile_getstring(script,&fn))
            {
                scriptfile *included;

                included = scriptfile_fromfile(fn);
                if (!included)
                {
                    initprintf("Warning: Failed including %s on line %s:%d\n",
                               fn, script->filename,scriptfile_getlinum(script,cmdtokptr));
                }
                else
                {
                    parsetilegroups(included);
                    scriptfile_close(included);
                }
            }
            break;
        }
        case T_DEFINE:
        {
            char *name;
            int number;

            if (scriptfile_getstring(script,&name)) break;
            if (scriptfile_getsymbol(script,&number)) break;
            if (scriptfile_addsymbolvalue(name,number) < 0)
                initprintf("Warning: Symbol %s was NOT redefined to %d on line %s:%d\n",
                           name,number,script->filename,scriptfile_getlinum(script,cmdtokptr));
            break;
        }
        case T_TILEGROUP:
        {
            char *end, *name;
            int i;

            if (tile_groups >= MAX_TILE_GROUPS) break;
            if (scriptfile_getstring(script,&name)) break;
            if (scriptfile_getbraces(script,&end)) break;

            s_TileGroups[tile_groups].pIds = Bcalloc(MAX_TILE_GROUP_ENTRIES,sizeof(int));
            s_TileGroups[tile_groups].szText = Bstrdup(name);

            while (script->textptr < end)
            {
                tokenlist tgtokens2[] =
                {
                    { "tilegroup",  T_TILEGROUP   },
                    { "tile",       T_TILE        },
                    { "tilerange",  T_TILERANGE   },
                    { "hotkey",     T_HOTKEY      },
                    { "tiles",      T_TILES       },
                    { "colors",     T_COLORS      },
                };

                int token = getatoken(script,tgtokens2,sizeof(tgtokens2)/sizeof(tokenlist));
                switch (token)
                {
                case T_TILE:
                {
                    if (scriptfile_getsymbol(script,&i)) break;
                    if (i >= 0 && i < MAXTILES && s_TileGroups[tile_groups].nIds < MAX_TILE_GROUP_ENTRIES)
                        s_TileGroups[tile_groups].pIds[s_TileGroups[tile_groups].nIds++] = i;
//                    OSD_Printf("added tile %d to group %d\n",i,g);
                    break;
                }
                case T_TILERANGE:
                {
                    int j;
                    if (scriptfile_getsymbol(script,&i)) break;
                    if (scriptfile_getsymbol(script,&j)) break;
                    if (i < 0 || i >= MAXTILES || j < 0 || j >= MAXTILES) break;
                    while (s_TileGroups[tile_groups].nIds < MAX_TILE_GROUP_ENTRIES && i <= j)
                    {
                        s_TileGroups[tile_groups].pIds[s_TileGroups[tile_groups].nIds++] = i++;
//                        OSD_Printf("added tile %d to group %d\n",i,g);
                    }
                    break;
                }
                case T_COLORS:
                {
                    int j;
                    if (scriptfile_getsymbol(script, &i)) break;
                    if (scriptfile_getsymbol(script, &j)) break;
                    if (i < 0 || i >= 256 || j < 0 || j >= 256) break;
                    s_TileGroups[tile_groups].color1 = i;
                    s_TileGroups[tile_groups].color2 = j;
                    break;
                }
                case T_HOTKEY:
                {
                    char *c;
                    if (scriptfile_getstring(script,&c)) break;
                    s_TileGroups[tile_groups].key1 = Btoupper(c[0]);
                    s_TileGroups[tile_groups].key2 = Btolower(c[0]);
                    break;
                }
                case T_TILES:
                {
                    char *end2;
                    if (scriptfile_getbraces(script,&end2)) break;
                    while (script->textptr < end2-1)
                    {
                        if (!scriptfile_getsymbol(script,&i))
                        {
                            if (i >= 0 && i < MAXTILES && s_TileGroups[tile_groups].nIds < MAX_TILE_GROUP_ENTRIES)
                                s_TileGroups[tile_groups].pIds[s_TileGroups[tile_groups].nIds++] = i;
                            //                    OSD_Printf("added tile %d to group %d\n",i,g);
                        }
                    }
                    break;
                }
                }
            }
            s_TileGroups[tile_groups].pIds = Brealloc(s_TileGroups[tile_groups].pIds,s_TileGroups[tile_groups].nIds*sizeof(int));
            tile_groups++;
            break;
        }
        case T_ALPHABET:
        {
            char *end;
            int i, j, k;

            if (numalphabets >= MAX_ALPHABETS) break;
            if (scriptfile_getbraces(script,&end)) break;

            for (i=0; i<NUMPRINTABLES; i++)
            {
                alphabets[numalphabets].pic[i] = -1;
                alphabets[numalphabets].xofs[i] = 0;
                alphabets[numalphabets].yofs[i] = 0;
            }

            while (script->textptr < end)
            {
                tokenlist alphtokens2[] =
                {
                    { "map",        T_MAP         },
                    { "mapa",       T_MAPA        },
                    { "maprange",   T_MAPRANGE    },
                    { "maprangea",  T_MAPRANGEA   },
                    { "offset",     T_OFFSET      },
                    { "offseta",    T_OFFSETA     },
                };

                int token = getatoken(script,alphtokens2,sizeof(alphtokens2)/sizeof(tokenlist));
                switch (token)
                {
                case T_MAP:  // map <ascii num> <start tilenum>, e.g. map 46 3002
                {
                    if (scriptfile_getnumber(script,&i)) break;
                    if (scriptfile_getsymbol(script,&j)) break;

                    if (i>=33 && i<=126 && j>= 0 && j<MAXTILES)
                        alphabets[numalphabets].pic[i-33] = j;

                    break;
                }
                case T_MAPA:  // mapa <ascii string> <start tilenum>, e.g. map ".,!?" 3002
                {
                    char *s;
                    if (scriptfile_getstring(script,&s)) break;
                    if (scriptfile_getsymbol(script,&i)) break;

                    for (;*s; s++, i++)
                    {
                        if (*s>=33 && *s<=126 && i>= 0 && i<MAXTILES)
                            alphabets[numalphabets].pic[(*s)-33] = i;
                    }
                    break;
                }
                // maprange <start ascii num> <end ascii num> <start tilenum>, e.g. map 33 126 STARTALPHANUM
                // maprangea <start char> <end char> <start tilenum>, e.g. map "!" "~" STARTALPHANUM
                case T_MAPRANGE:
                case T_MAPRANGEA:
                {
                    if (token==T_MAPRANGE)
                    {
                        if (scriptfile_getnumber(script,&i)) break;
                        if (scriptfile_getnumber(script,&j)) break;
                    }
                    else
                    {
                        char *c1, *c2;
                        if (scriptfile_getstring(script,&c1)) break;
                        if (scriptfile_getstring(script,&c2)) break;
                        i=*c1;
                        j=*c2;
                    }
                    if (scriptfile_getsymbol(script,&k)) break;

                    if (i>126 || j<33) break;
                    for (; i<=j && k<MAXTILES; i++, k++)
                    {
                        if (i>=33 && i<=126)
                            alphabets[numalphabets].pic[i-33] = k;
                    }
                    break;
                }
                case T_OFFSET:  // offset <ascii num> <xoffset> <yoffset>
                {
                    if (scriptfile_getnumber(script, &i)) break;
                    if (scriptfile_getnumber(script, &j)) break;
                    if (scriptfile_getnumber(script, &k)) break;

                    if (i >= 33 && i <= 126)
                    {
                        alphabets[numalphabets].xofs[i-33] = j;
                        alphabets[numalphabets].yofs[i-33] = k;
                    }
                    break;
                }
                case T_OFFSETA:  // offseta <ascii string> <xoffset> <yoffset>
                {
                    char *s;
                    if (scriptfile_getstring(script, &s)) break;
                    if (scriptfile_getnumber(script, &i)) break;
                    if (scriptfile_getnumber(script, &j)) break;

                    for (;*s; s++)
                        if (*s >= 33 && *s <= 126)
                        {
                            alphabets[numalphabets].xofs[(*s)-33] = i;
                            alphabets[numalphabets].yofs[(*s)-33] = j;
                        }
                    break;
                }
                }
            }
            numalphabets++;
            break;
        }
        case T_EOF:
            return(0);
        default:
            break;
        }
    }
    return 0;
}

int loadtilegroups(char *fn)
{
    int i, j;
    scriptfile *script;
    TileGroup blank = { NULL,     0,  NULL,     0, 0, 0, 0};

    script = scriptfile_fromfile(fn);
    if (!script) return -1;

    for (i = MAX_TILE_GROUPS-1; i >= 0; i--)
    {
        Bmemcpy(&s_TileGroups[i],&blank,sizeof(blank));
    }

    parsetilegroups(script);

    scriptfile_close(script);
    scriptfile_clearsymbols();

    tilegroupItems = getTileGroup("Items");
    tilegroupActors = getTileGroup("Actors");

    // Apply 2d sprite colors as specified in tiles.cfg.
    for (i = 0; i < MAX_TILE_GROUPS; i++)
    {
        if (s_TileGroups[i].szText == NULL) break;
        // If the colors were specified...
        if (s_TileGroups[i].color1 && s_TileGroups[i].color2)
        {
            for (j = s_TileGroups[i].nIds-1; j >= 0 ; j--)
            {
                // Apply the colors to all tiles in the group.
                spritecol2d[s_TileGroups[i].pIds[j]][0] = s_TileGroups[i].color1;
                spritecol2d[s_TileGroups[i].pIds[j]][1] = s_TileGroups[i].color2;
            }
        }
    }


    return 0;
}

int ExtInit(void)
{
    int rv = 0;
    int i;
    char cwd[BMAX_PATH];

#if defined(__linux__) || defined(__FreeBSD__) || defined(__NetBSD__) || defined(__OpenBSD__)
    addsearchpath("/usr/share/games/jfduke3d");
    addsearchpath("/usr/local/share/games/jfduke3d");
    addsearchpath("/usr/share/games/eduke32");
    addsearchpath("/usr/local/share/games/eduke32");
#elif defined(__APPLE__)
    addsearchpath("/Library/Application Support/JFDuke3D");
    addsearchpath("/Library/Application Support/EDuke32");
#endif

    if (getcwd(cwd,BMAX_PATH))
    {
#if defined(__APPLE__)
        /* Dirty hack on OS X to also look for gamedata inside the application bundle - rhoenie 08/08 */
        char seekinappcontainer[BMAX_PATH];
        Bsnprintf(seekinappcontainer,sizeof(seekinappcontainer),"%s/EDuke32.app/", cwd);
        addsearchpath(seekinappcontainer);
#endif
        addsearchpath(cwd);
        Bstrcpy(program_origcwd, cwd);
    }
    else
        program_origcwd[0] = '\0';

    if (CommandPaths)
    {
        struct strllist *s;
        while (CommandPaths)
        {
            s = CommandPaths->next;
            addsearchpath(CommandPaths->str);

            free(CommandPaths->str);
            free(CommandPaths);
            CommandPaths = s;
        }
    }

#if defined(_WIN32)
    if (!access("user_profiles_enabled", F_OK))
#else
    if (usecwd == 0 && access("user_profiles_disabled", F_OK))
#endif
    {
        char *homedir;
        int asperr;

        if ((homedir = Bgethomedir()))
        {
            Bsnprintf(cwd,sizeof(cwd),"%s/"
#if defined(_WIN32)
                      "EDuke32 Settings"
#elif defined(__APPLE__)
                      "Library/Application Support/EDuke32"
#else
                      ".eduke32"
#endif
                      ,homedir);
            asperr = addsearchpath(cwd);
            if (asperr == -2)
            {
                if (Bmkdir(cwd,S_IRWXU) == 0) asperr = addsearchpath(cwd);
                else asperr = -1;
            }
            if (asperr == 0)
                chdir(cwd);
            Bfree(homedir);
        }
    }

    // JBF 20031220: Because it's annoying renaming GRP files whenever I want to test different game data
    if (getenv("DUKE3DGRP"))
    {
        duke3dgrp = getenv("DUKE3DGRP");
        initprintf("Using %s as main GRP file\n", duke3dgrp);
    }

    i = initgroupfile(duke3dgrp);

    if (!NoAutoLoad)
    {
        getfilenames("autoload","*.grp");
        while (findfiles) { Bsprintf(tempbuf,"autoload/%s",findfiles->name); initprintf("Using group file '%s'.\n",tempbuf); initgroupfile(tempbuf); findfiles = findfiles->next; }
        getfilenames("autoload","*.zip");
        while (findfiles) { Bsprintf(tempbuf,"autoload/%s",findfiles->name); initprintf("Using group file '%s'.\n",tempbuf); initgroupfile(tempbuf); findfiles = findfiles->next; }
        getfilenames("autoload","*.pk3");
        while (findfiles) { Bsprintf(tempbuf,"autoload/%s",findfiles->name); initprintf("Using group file '%s'.\n",tempbuf); initgroupfile(tempbuf); findfiles = findfiles->next; }

        if (i != -1)
            DoAutoload(duke3dgrp);
    }

    if (getenv("DUKE3DDEF"))
    {
        defsfilename = getenv("DUKE3DDEF");
        initprintf("Using '%s' as definitions file\n", defsfilename);
    }
    loadgroupfiles(defsfilename);

    {
        struct strllist *s;
        int j;

        pathsearchmode = 1;
        while (CommandGrps)
        {
            s = CommandGrps->next;
            j = initgroupfile(CommandGrps->str);
            if (j == -1) initprintf("Could not find group file '%s'.\n",CommandGrps->str);
            else
            {
                initprintf("Using group file '%s'.\n",CommandGrps->str);
                if (!NoAutoLoad)
                    DoAutoload(CommandGrps->str);
            }

            free(CommandGrps->str);
            free(CommandGrps);
            CommandGrps = s;
        }
        pathsearchmode = 0;
    }

    bpp = 32;

#if defined(POLYMOST) && defined(USE_OPENGL)
    glusetexcache = glusetexcachecompression = -1;

    initprintf("Using config file '%s'.\n",setupfilename);
    if (loadsetup(setupfilename) < 0) initprintf("Configuration file not found, using defaults.\n"), rv = 1;

    if (glusetexcache == -1 || glusetexcachecompression == -1)
    {
        int i;
#if 1
        i=wm_ynbox("Texture Caching",
                   "Would you like to enable the on-disk texture cache?\n\n"
                   "You will generally want to say 'yes' here, especially if using the HRP.");
#else
        i = 1;
#endif
        if (i)
            glusetexcompr = glusetexcache = glusetexcachecompression = 1;
        else glusetexcache = glusetexcachecompression = 0;
    }
#endif

    Bmemcpy((void *)buildkeys,(void *)keys,NUMBUILDKEYS);   //Trick to make build use setup.dat keys

    if (initengine())
    {
        initprintf("There was a problem initializing the engine.\n");
        return -1;
    }

    kensplayerheight = 40; //32
    zmode = 2;
    zlock = kensplayerheight<<8;
    defaultspritecstat = 0;

    ReadGamePalette();
    //  InitWater();

    InitCustomColors();

    getmessageleng = 0;
    getmessagetimeoff = 0;

    Bstrcpy(apptitle, "Mapster32"VERSION BUILDDATE);
    autosavetimer = totalclock+120*autosave;

#if defined(_WIN32) && defined(DUKEOSD)
    OSD_SetFunctions(
        /*  	  GAME_drawosdchar,
                GAME_drawosdstr,
                GAME_drawosdcursor,
                GAME_getcolumnwidth,
        		GAME_getrowheight,*/
        0,0,0,0,0,
        GAME_clearbackground,
        (int(*)(void))GetTime,
        NULL
    );
#endif

    OSD_SetParameters(0,2, 0,0, 4,0);
    registerosdcommands();

    loadtilegroups("tiles.cfg");

    ReadHelpFile("m32help.hlp");

    return rv;
}

#ifdef RENDERTYPEWIN
void app_crashhandler(void)
{
    if (levelname[0])
    {
        char *f;
        fixspritesectors();   //Do this before saving!
        updatesector(startposx,startposy,&startsectnum);
        if (pathsearchmode)
            f = levelname;
        else
        {
            // virtual filesystem mode can't save to directories so drop the file into
            // the current directory
            f = strrchr(levelname, '/');
            if (!f) f = levelname; else f++;
        }
        f=strstr(levelname,".map");
        if (f)
            Bstrcpy(f,"_crash.map");
        else Bstrcat(f,"_crash.map");
        ExtPreSaveMap();
        saveboard(levelname,&startposx,&startposy,&startposz,&startang,&startsectnum);
        ExtSaveMap(levelname);
    }
}
#endif

void ExtUnInit(void)
{
    int i;
    // setvmode(0x03);
    uninitgroupfile();
    writesetup(setupfilename);

    for (i = MAX_TILE_GROUPS-1; i >= 0; i--)
    {
        if (s_TileGroups[i].pIds != NULL)
            Bfree(s_TileGroups[i].pIds);
        if (s_TileGroups[i].szText != NULL)
            Bfree(s_TileGroups[i].szText);
    }
    for (i = numhelppages-1; i >= 0;i--) free(helppage[i]);
    if (helppage) free(helppage);
}

static char wallshades[MAXWALLS];
static char sectorshades[MAXSECTORS][2];
static char spriteshades[MAXSPRITES];
static char wallpals[MAXWALLS];
static char sectorpals[MAXSECTORS][2];
static char spritepals[MAXSPRITES];
static char wallflag[MAXWALLS];

void ExtPreCheckKeys(void) // just before drawrooms
{
    int i = 0;
    int radius, xp1, yp1;
    char col;

    if (qsetmode == 200)    //In 3D mode
    {
        if (shadepreview)
        {
            int i = 0;
            for (i=numsprites-1;i>=0;i--)
                if (sprite[i].picnum == SECTOREFFECTOR && (sprite[i].lotag == 12 || sprite[i].lotag == 3))
                {
                    int w;
                    int start_wall = sector[sprite[i].sectnum].wallptr;
                    int end_wall = start_wall + sector[sprite[i].sectnum].wallnum;

                    for (w = start_wall; w < end_wall; w++)
                    {
                        if (!wallflag[w])
                        {
                            wallshades[w] = wall[w].shade;
                            wall[w].shade = sprite[i].shade;
                            wallpals[w] = wall[w].pal;
                            wall[w].pal = sprite[i].pal;
                            wallflag[w] = 1;
                        }
                        /*                        if (wall[w].nextwall >= 0)
                                                {
                                                    if (!wallflag[wall[w].nextwall])
                                                    {
                                                        wallshades[wall[w].nextwall] = wall[wall[w].nextwall].shade;
                                                        wall[wall[w].nextwall].shade = sprite[i].shade;
                                                        wallpals[wall[w].nextwall] = wall[wall[w].nextwall].pal;
                                                        wall[wall[w].nextwall].pal = sprite[i].pal;
                                                        wallflag[wall[w].nextwall] = 1;
                                                    }
                                                } */
                    }
                    sectorshades[sprite[i].sectnum][0] = sector[sprite[i].sectnum].floorshade;
                    sectorshades[sprite[i].sectnum][1] = sector[sprite[i].sectnum].ceilingshade;
                    sector[sprite[i].sectnum].floorshade = sprite[i].shade;
                    sector[sprite[i].sectnum].ceilingshade = sprite[i].shade;
                    sectorpals[sprite[i].sectnum][0] = sector[sprite[i].sectnum].floorpal;
                    sectorpals[sprite[i].sectnum][1] = sector[sprite[i].sectnum].ceilingpal;
                    sector[sprite[i].sectnum].floorpal = sprite[i].pal;
                    sector[sprite[i].sectnum].ceilingpal = sprite[i].pal;
                    w = headspritesect[sprite[i].sectnum];
                    while (w >= 0)
                    {
                        if (w == i) { w = nextspritesect[w]; continue; }
                        spriteshades[w] = sprite[w].shade;
                        sprite[w].shade = sprite[i].shade;
                        spritepals[w] = sprite[w].pal;
                        sprite[w].pal = sprite[i].pal;
                        w = nextspritesect[w];
                    }
                }
        }
        if (floor_over_floor) SE40Code(posx,posy,posz,ang,horiz);
        if (purpleon) clearview(255);
        if (sidemode != 0)
        {
            lockbyte4094 = 1;
            if (waloff[4094] == 0)
                allocache(&waloff[4094],320L*200L,&lockbyte4094);
            setviewtotile(4094,320L,200L);
            searchx ^= searchy;
            searchy ^= searchx;
            searchx ^= searchy;
            searchx = ydim-1-searchx;
        }
        return;
    }
    begindrawing();

//    if (cursectornum >= 0)
//        fillsector(cursectornum, 31);

    for (i=0;i<numsprites;i++) // Game specific 2D sprite stuff goes here.  Drawn below everything else.
    {
        xp1 = mulscale14(sprite[i].x-posx,zoom);
        yp1 = mulscale14(sprite[i].y-posy,zoom);

        if (sprite[i].picnum == 5 /*&& zoom >= 256*/ && sprite[i].sectnum != MAXSECTORS)
        {
            radius = mulscale14(sprite[i].hitag,zoom);
            col = 6;
            if (i+16384 == pointhighlight)
                if (totalclock & 32) col += (2<<2);
            drawlinepat = 0xf0f0f0f0;
            drawcircle16(halfxdim16+xp1, midydim16+yp1, radius, col);
            drawlinepat = 0xffffffff;
//            radius = mulscale15(sprite[i].hitag,zoom);
            //          drawcircle16(halfxdim16+xp1, midydim16+yp1, radius, col);
        }
    }

    enddrawing();
}

void ExtAnalyzeSprites(void)
{
    int i, k;
    spritetype *tspr;
    int frames=0, l;

    for (i=0,tspr=&tsprite[0];i<spritesortcnt;i++,tspr++)
    {
        frames=0;

        if ((nosprites==1||nosprites==3)&&tspr->picnum<11) tspr->xrepeat=0;

        if (nosprites==1||nosprites==3)
            switch (tspr->picnum)
            {
            case SEENINE :
                tspr->xrepeat=0;
            }

        if (shadepreview && !(tspr->cstat & 16))
        {
            if (sector[tspr->sectnum].ceilingstat&1)
                l = sector[tspr->sectnum].ceilingshade;
            else
            {
                l = sector[tspr->sectnum].floorshade;
                if (sector[tspr->sectnum].floorpal != 0 && sector[tspr->sectnum].floorpal < num_tables)
                    tspr->pal=sector[tspr->sectnum].floorpal;
            }
            if (l < -127) l = -127;
            if (l > 126) l =  127;

            tspr->shade = l;

        }

        switch (tspr->picnum)
        {
            // 5-frame walk
        case 1550 :             // Shark
            frames=5;
            // 2-frame walk
        case 1445 :             // duke kick
        case LIZTROOPDUCKING :
        case 2030 :            // pig shot
        case OCTABRAIN :
        case PIGCOPDIVE :
        case 2190 :            // liz capt shot
        case BOSS1SHOOT :
        case BOSS1LOB :
        case LIZTROOPSHOOT :
            if (frames==0) frames=2;

            // 4-frame walk
        case 1491 :             // duke crawl
        case LIZTROOP :
        case LIZTROOPRUNNING :
        case PIGCOP :
        case LIZMAN :
        case BOSS1 :
        case BOSS2 :
        case BOSS3 :
        case BOSS4 :
        case NEWBEAST:
            if (frames==0) frames=4;
        case LIZTROOPJETPACK :
        case DRONE :
        case COMMANDER :
        case TANK :
        case RECON :
            if (frames==0) frames = 10;
        case CAMERA1:
        case APLAYER :
            if (frames==0) frames=1;
        case GREENSLIME :
        case EGG :
        case PIGCOPSTAYPUT :
        case LIZMANSTAYPUT:
        case LIZTROOPSTAYPUT :
        case LIZMANSPITTING :
        case LIZMANFEEDING :
        case LIZMANJUMP :
            if (skill!=4)
            {
                if (tspr->lotag>skill+1)
                {
                    tspr->xrepeat=0;
                    tspr->cstat=32768;
                    break;
                }
            }
            if (nosprites==2||nosprites==3)
            {
                tspr->xrepeat=0;
                tspr->cstat=32768;
            }
            //                else tspr->cstat&=32767;

#if defined(USE_OPENGL) && defined(POLYMOST)
            if (!usemodels || md_tilehasmodel(tspr->picnum,tspr->pal) < 0)
#endif
            {
                if (frames!=0)
                {
                    if (frames==10) frames=0;
                    k = getangle(tspr->x-posx,tspr->y-posy);
                    k = (((tspr->ang+3072+128-k)&2047)>>8)&7;
                    //This guy has only 5 pictures for 8 angles (3 are x-flipped)
                    if (k <= 4)
                    {
                        tspr->picnum += k;
                        tspr->cstat &= ~4;   //clear x-flipping bit
                    }
                    else
                    {
                        tspr->picnum += 8-k;
                        tspr->cstat |= 4;    //set x-flipping bit
                    }
                }

                if (frames==2) tspr->picnum+=((((4-(totalclock>>5)))&1)*5);
                if (frames==4) tspr->picnum+=((((4-(totalclock>>5)))&3)*5);
                if (frames==5) tspr->picnum+=(((totalclock>>5)%5))*5;

                if (tilesizx[tspr->picnum] == 0)
                    tspr->picnum -= 5;       //Hack, for actors
            }
            break;
        default:
            break;

        }
    }
}

#define MESSAGEX 3 // (xdimgame>>1)
#define MESSAGEY 3 // ((i/charsperline)<<3)+(ydimgame-(ydimgame>>3))-(((getmessageleng-1)/charsperline)<<3)

static void Keys2d3d(void)
{
    int i;

    if (keystatus[KEYSC_F12])   //F12
    {
        screencapture("captxxxx.tga",keystatus[KEYSC_LSHIFT]|keystatus[KEYSC_RSHIFT]);
        message("Saved screenshot %04d",capturecount-1);
        keystatus[KEYSC_F12] = 0;
    }

    if (keystatus[KEYSC_QUOTE] && keystatus[KEYSC_A]) // ' a
    {
        keystatus[KEYSC_A] = 0;

        if (qsetmode == 200) autosave=autosave?0:getnumber256("Autosave interval, in seconds: ",180,3600,0);
        else autosave=autosave?0:getnumber16("Autosave interval, in seconds: ",180,3600,0);

        if (autosave) message("Autosave enabled, interval: %d seconds",autosave);
        else message("Autosave disabled");
    }

    if (keystatus[KEYSC_QUOTE] && keystatus[KEYSC_N]) // ' n
    {
        keystatus[KEYSC_N] = 0;
        noclip=!noclip;
        if (noclip) message("Clipping disabled");
        else message("Clipping enabled");
    }

    if (eitherCTRL && keystatus[KEYSC_N]) // CTRL+N
    {
        keystatus[KEYSC_N] = 0;
        spnoclip=!spnoclip;
        if (spnoclip) message("Sprite clipping disabled");
        else message("Sprite clipping enabled");
    }

    if ((totalclock > autosavetimer) && (autosave))
    {
        if (asksave == 1)
        {
            fixspritesectors();   //Do this before saving!
            //             updatesector(startposx,startposy,&startsectnum);
            ExtPreSaveMap();
            saveboard("autosave.map",&startposx,&startposy,&startposz,&startang,&startsectnum);
            ExtSaveMap("autosave.map");
            message("Board autosaved to AUTOSAVE.MAP");
            asksave = 2;
        }
        autosavetimer = totalclock+120*autosave;
    }

    if (eitherCTRL)  //CTRL
    {
        char *f;
        if (pathsearchmode) f = levelname;
        else
        {
            // virtual filesystem mode can't save to directories so drop the file into
            // the current directory
            f = strrchr(levelname, '/');
            if (!f) f = levelname; else f++;
        }

        if (keystatus[KEYSC_P]) // Ctrl-P: Map playtesting
        {
            keystatus[KEYSC_P] = 0;

            if (!eitherALT)
                updatesector(posx, posy, &cursectnum);
            else
                updatesector(startposx, startposy, &startsectnum);

            if ((!eitherALT && cursectnum >= 0) || (eitherALT && startsectnum >= 0))
            {
                char *param = " -map autosave.map -noinstancechecking";
                char *fullparam;
                char current_cwd[BMAX_PATH];
                int slen = 0;
                BFILE *fp;

                if ((program_origcwd[0] == '\0') || !getcwd(current_cwd, BMAX_PATH))
                    current_cwd[0] = '\0';
                else // Before we check if file exists, for the case there's no absolute path.
                    chdir(program_origcwd);

                fp = fopen(game_executable, "rb"); // File exists?
                if (fp != NULL)
                    fclose(fp);
                else
                {
#ifdef _WIN32
                    fullparam = Bstrrchr(mapster32_fullpath, '\\');
#else
                    fullparam = Bstrrchr(mapster32_fullpath, '/');
#endif
                    if (fullparam)
                    {
                        slen = fullparam-mapster32_fullpath+1;
                        Bstrncpy(game_executable, mapster32_fullpath, slen);
                        // game_executable is now expected to not be NULL-terminated!
                        Bstrcpy(game_executable+slen, DEFAULT_GAME_EXEC);
                    }
                    else
                        Bstrcpy(game_executable, DEFAULT_GAME_LOCAL_EXEC);
                }

                if (current_cwd[0] != '\0') // Temporarily changing back,
                    chdir(current_cwd);       // after checking if file exists.

                if (testplay_addparam)
                    slen = Bstrlen(testplay_addparam);

                // Considering the NULL character, quatation marks
                // and a possible extra space not in testplay_addparam,
                // the length should be Bstrlen(game_executable)+Bstrlen(param)+(slen+1)+2+1.

                fullparam = Bmalloc(Bstrlen(game_executable)+Bstrlen(param)+slen+4);
                Bsprintf(fullparam,"\"%s\"",game_executable);

                if (testplay_addparam)
                {
                    Bstrcat(fullparam, " ");
                    Bstrcat(fullparam, testplay_addparam);
                }
                Bstrcat(fullparam, param);

                fixspritesectors();   //Do this before saving!
                ExtPreSaveMap();
                if (eitherALT)
                    saveboard("autosave.map",&startposx,&startposy,&startposz,&startang,&startsectnum);
                else
                    saveboard("autosave.map",&posx,&posy,&posz,&ang,&cursectnum);
                message("Board saved to AUTOSAVE.MAP. Starting the game...");

                uninitmouse();
#ifdef _WIN32
                {
                    STARTUPINFO si;
                    PROCESS_INFORMATION pi;

                    ZeroMemory(&si,sizeof(si));
                    ZeroMemory(&pi,sizeof(pi));
                    si.cb = sizeof(si);

                    if (!CreateProcess(NULL,fullparam,NULL,NULL,0,0,NULL,NULL,&si,&pi))
                        message("Error launching the game!");
                    else WaitForSingleObject(pi.hProcess,INFINITE);
                }
#else
                if (current_cwd[0] != '\0')
                {
                    chdir(program_origcwd);
                    system(fullparam);
                    //  message("Error launching the game!");
                    chdir(current_cwd);
                }
                else system(fullparam);
#endif
                //  message("Error launching the game!");
                message("Game process exited");
                initmouse();

                Bfree(fullparam);
            }
            else
                message("Position must be in valid player space to test map!");
        }

        if (keystatus[KEYSC_S]) // S
        {
            if (levelname[0])
            {
                fixspritesectors();   //Do this before saving!
                updatesector(startposx,startposy,&startsectnum);
                ExtPreSaveMap();
                saveboard(f,&startposx,&startposy,&startposz,&startang,&startsectnum);
                ExtSaveMap(f);
                message("Board saved");
                asksave = 0;
                keystatus[KEYSC_S] = 0;
                lastsave=totalclock;
            }
        }
        if (keystatus[KEYSC_L]) // L
        {
            extern int grponlymode;
            extern void loadmhk();

            if (totalclock < (lastsave + 120*10) || !AskIfSure("Are you sure you want to load the last saved map?"))
            {
                int sposx=posx,sposy=posy,sposz=posz,sang=ang;

                lastsave=totalclock;
                highlightcnt = -1;
//  			  sectorhighlightstat = -1;
//  			  newnumwalls = -1;
//  			  joinsector[0] = -1;
//  			  circlewall = -1;
//  			  circlepoints = 7;

                for (i=MAXSECTORS-1;i>=0;i--) sector[i].extra = -1;
                for (i=MAXWALLS-1;i>=0;i--) wall[i].extra = -1;
                for (i=MAXSPRITES-1;i>=0;i--) sprite[i].extra = -1;

                ExtPreLoadMap();
                i = loadboard(f,(!pathsearchmode&&grponlymode?2:0),&posx,&posy,&posz,&ang,&cursectnum);
                loadmhk();
                if (i == -2) i = loadoldboard(f,(!pathsearchmode&&grponlymode?2:0),&posx,&posy,&posz,&ang,&cursectnum);
                if (i < 0) printmessage16("Invalid map format.");
                else
                {
                    ExtLoadMap(f);
                    if (mapversion < 7) message("Map %s loaded successfully and autoconverted to V7!",f);
                    else message("Map %s loaded successfully",f);
                }
                updatenumsprites();
                startposx = posx;
                startposy = posy;
                startposz = posz;
                startang = ang;
                startsectnum = cursectnum;
                posx=sposx;posy=sposy;posz=sposz;ang=sang;
                keystatus[KEYSC_L]=0;
            }
        }
    }

    if (keystatus[buildkeys[BK_MODE2D_3D]])  // Enter
    {
        getmessageleng = 0;
        getmessagetimeoff = 0;
#if defined(_WIN32) && defined(DUKEOSD)
        if (qsetmode == 200)
        {
            OSD_SetFunctions(
                NULL,
                NULL,
                NULL,
                NULL,
                NULL,
                NULL,
                (int(*)(void))GetTime,
                NULL
            );
        }
        else
        {
            OSD_SetFunctions(
                /*  			  GAME_drawosdchar,
                                GAME_drawosdstr,
                                GAME_drawosdcursor,
                                GAME_getcolumnwidth,
                				GAME_getrowheight,*/
                0,0,0,0,0,
                GAME_clearbackground,
                (int(*)(void))GetTime,
                NULL
            );
        }
#endif
    }

    if (getmessageleng > 0)
    {
//        charsperline = 64;
        //if (dimensionmode[snum] == 2) charsperline = 80;
        /*        if (qsetmode == 200)
                {
                    for (i=0;i<=getmessageleng;i+=charsperline)
                    {
                        for (j=0;j<charsperline;j++)
                            tempbuf[j] = getmessage[i+j];
                        if (getmessageleng < i+charsperline)
                            tempbuf[(getmessageleng-i)] = 0;
                        else
                            tempbuf[charsperline] = 0;
                        begindrawing();
                        if (tempbuf[charsperline] != 0)
                        {
                            printext256((MESSAGEX*(xdimgame/320.))+2,(MESSAGEY*(ydimgame/200.))+2,0,-1,tempbuf,xdimgame>640?0:1);
                            printext256(MESSAGEX*(xdimgame/320.),MESSAGEY*(ydimgame/200.),
                                        (totalclock > (lastmessagetime + 120*5))?whitecol:256-5,-1,tempbuf,xdimgame>640?0:1);
                        }
                        enddrawing();
                    }
                }
                else */
        if (qsetmode != 200)
            printmessage16(getmessage);
        if (totalclock > getmessagetimeoff)
            getmessageleng = 0;
    }

}
#undef EDUKE32_EXEC
#undef EDUKE32_LOCALEXEC

void ExtCheckKeys(void)
{
    if (qsetmode == 200)
    {
        if (shadepreview)
        {
            int i = 0;
            for (i=numsprites-1;i>=0;i--)
                if (sprite[i].picnum == SECTOREFFECTOR && (sprite[i].lotag == 12 || sprite[i].lotag == 3))
                {
                    int w;
                    int start_wall = sector[sprite[i].sectnum].wallptr;
                    int end_wall = start_wall + sector[sprite[i].sectnum].wallnum;

                    for (w = start_wall; w < end_wall; w++)
                    {
                        if (wallflag[w])
                        {
                            wall[w].shade = wallshades[w];
                            wall[w].pal = wallpals[w];
                            wallflag[w] = 0;
                        }
                        /*                        if (wall[w].nextwall >= 0)
                                                {
                                                    if (wallflag[wall[w].nextwall])
                                                    {
                                                        wall[wall[w].nextwall].shade = wallshades[wall[w].nextwall];
                                                        wall[wall[w].nextwall].pal = wallpals[wall[w].nextwall];
                                                        wallflag[wall[w].nextwall] = 0;
                                                    }
                                                } */
                    }
                    sector[sprite[i].sectnum].floorshade = sectorshades[sprite[i].sectnum][0];
                    sector[sprite[i].sectnum].ceilingshade = sectorshades[sprite[i].sectnum][1];
                    sector[sprite[i].sectnum].floorpal = sectorpals[sprite[i].sectnum][0];
                    sector[sprite[i].sectnum].ceilingpal = sectorpals[sprite[i].sectnum][1];

                    w = headspritesect[sprite[i].sectnum];
                    while (w >= 0)
                    {
                        if (w == i) { w = nextspritesect[w]; continue; }
                        sprite[w].shade = spriteshades[w];
                        sprite[w].pal = spritepals[w];
                        w = nextspritesect[w];
                    }
                }
        }
    }
    readmousebstatus(&bstatus);
    Keys2d3d();
    if (qsetmode == 200)    //In 3D mode
    {
        Keys3d();
        if (sidemode != 1)
        {
            editinput();
            if (infobox&2)m32_showmouse();
        }
        return;
    }
    Keys2d();
}

void faketimerhandler(void)
{
    int i, dist;
    int hiz, hihit, loz, lohit, oposx, oposy;
    short hitwall, daang;

    counter++;
    if (counter>=5) counter=0;

    sampletimer();
    if (totalclock < ototalclock+TICSPERFRAME || qsetmode != 200 || sidemode != 1)
        return;
    ototalclock = totalclock;

    oposx = posx;
    oposy = posy;
    hitwall = clipmove(&posx,&posy,&posz,&cursectnum,xvel,yvel,128L,4L<<8,4L<<8,0);
    xvel = ((posx-oposx)<<14);
    yvel = ((posy-oposy)<<14);

    yvel += 80000;
    if ((hitwall&0xc000) == 32768)
    {
        hitwall &= (MAXWALLS-1);
        i = wall[hitwall].point2;
        daang = getangle(wall[i].x-wall[hitwall].x,wall[i].y-wall[hitwall].y);

        xvel -= (xvel>>4);
        if (xvel < 0) xvel++;
        if (xvel > 0) xvel--;

        yvel -= (yvel>>4);
        if (yvel < 0) yvel++;
        if (yvel > 0) yvel--;

        i = 4-keystatus[buildkeys[BK_RUN]];
        xvel += mulscale(vel,(int)sintable[(ang+512)&2047],i);
        yvel += mulscale(vel,(int)sintable[ang&2047],i);

        if (((daang-ang)&2047) < 1024)
            ang = ((ang+((((daang-ang)&2047)+24)>>4))&2047);
        else
            ang = ((ang-((((ang-daang)&2047)+24)>>4))&2047);

        timoff = ototalclock;
    }
    else
    {
        if (ototalclock > timoff+32)
            ang = ((ang+((timoff+32-ototalclock)>>4))&2047);
    }

    getzrange(posx,posy,posz,cursectnum,&hiz,&hihit,&loz,&lohit,128L,0);

    oposx -= posx;
    oposy -= posy;

    dist = ksqrt(oposx*oposx+oposy*oposy);
    if (ototalclock > timoff+32) dist = 0;

    daang = mulscale(dist,angvel,9);
    posz += (daang<<6);
    if (posz > loz-(4<<8)) posz = loz-(4<<8), hvel = 0;
    if (posz < hiz+(4<<8)) posz = hiz+(4<<8), hvel = 0;

    horiz = ((horiz*7+(100-(daang>>1)))>>3);
    if (horiz < 100) horiz++;
    if (horiz > 100) horiz--;
    /*
        if (keystatus[KEYSC_QUOTE] && keystatus[KEYSC_5]) // ' 5
        {
            keystatus[KEYSC_5]=0;
            editstatus = 1;
            sidemode = 2;
        }
        */
}

extern short brightness;

static inline void SetBOSS1Palette()
{
    if (acurpalette==3) return;
    acurpalette=3;
    setbrightness(brightness,BOSS1palette,0);
}

static inline void SetSLIMEPalette()
{
    if (acurpalette==2) return;
    acurpalette=2;
    setbrightness(brightness,SLIMEpalette,0);
}

static inline void SetWATERPalette()
{
    if (acurpalette==1) return;
    acurpalette=1;
    setbrightness(brightness,WATERpalette,0);
}

static inline void SetGAMEPalette()
{
    if (acurpalette==0) return;
    acurpalette=0;
    setbrightness(brightness,GAMEpalette,0);
}

static void SearchSectorsForward()
{
    int ii=0;
    if (cursector_lotag!=0)
    {
        if (cursectornum<MAXSECTORS) cursectornum++;
        for (ii=cursectornum;ii<=MAXSECTORS;ii++)
        {
            if (sector[ii].lotag==cursector_lotag)
            {
                posx=wall[sector[ii].wallptr].x;
                posy=wall[sector[ii].wallptr].y;
                printmessage16("> Sector search: found");
                //            cursectornum++;
                keystatus[KEYSC_RBRACK]=0; // ]
                return;
            }
            cursectornum++;
        }
    }
    printmessage16("> Sector search: none found");
}

static void SearchSectorsBackward()
{
    int ii=0;
    if (cursector_lotag!=0)
    {
        if (cursectornum>0) cursectornum--;
        for (ii=cursectornum;ii>=0;ii--)
        {
            if (sector[ii].lotag==cursector_lotag)
            {
                posx=wall[sector[ii].wallptr].x;
                posy=wall[sector[ii].wallptr].y;
                printmessage16("< Sector search: found");
                //            cursectornum--;
                keystatus[KEYSC_LBRACK]=0; // [
                return;
            }
            cursectornum--;
        }
    }
    printmessage16("< Sector search: none found");
}

// Build edit originally by Ed Coolidge <semicharm@earthlink.net>
static void EditSectorData(short sectnum)
{
    char disptext[80];
    char edittext[80];
    int col=1, row=0, rowmax = 6, dispwidth = 24, editval = 0, i = -1;
    int xpos = 200, ypos = ydim-STATUS2DSIZ+48;

    disptext[dispwidth] = 0;
    clearmidstatbar16();
    showsectordata(sectnum);

    begindrawing();
    while (keystatus[KEYSC_ESC] == 0)
    {
        if (handleevents())
        {
            if (quitevent) quitevent = 0;
        }
        idle();
        printmessage16("Edit mode, press <Esc> to exit");
        if (keystatus[KEYSC_DOWN])
        {
            if (row < rowmax)
            {
                printext16(xpos,ypos+row*8,11,0,disptext,0);
                row++;
            }
            keystatus[KEYSC_DOWN] = 0;
        }
        if (keystatus[KEYSC_UP])
        {
            if (row > 0)
            {
                printext16(xpos,ypos+row*8,11,0,disptext,0);
                row--;
            }
            keystatus[KEYSC_UP] = 0;
        }
        if (keystatus[KEYSC_LEFT])
        {
            if (col == 2)
            {
                printext16(xpos,ypos+row*8,11,0,disptext,0);
                col = 1;
                xpos = 200;
                rowmax = 6;
                dispwidth = 24;
                disptext[dispwidth] = 0;
                if (row > rowmax) row = rowmax;
            }
            keystatus[KEYSC_LEFT] = 0;
        }
        if (keystatus[KEYSC_RIGHT])
        {
            if (col == 1)
            {
                printext16(xpos,ypos+row*8,11,0,disptext,0);
                col = 2;
                xpos = 400;
                rowmax = 6;
                dispwidth = 24;
                disptext[dispwidth] = 0;
                if (row > rowmax) row = rowmax;
            }
            keystatus[KEYSC_RIGHT] = 0;
        }
        if (keystatus[KEYSC_ENTER])
        {
            keystatus[KEYSC_ENTER] = 0;
            editval = 1;
        }

        if (col == 1)
        {
            switch (row)
            {
            case 0:
                for (i=Bsprintf(disptext,"Flags (hex): %x",sector[sectnum].ceilingstat); i < dispwidth; i++) disptext[i] = ' ';
                Bsprintf(edittext,"Sector %d Ceiling Flags: ",sectnum);
                if (editval)
                {
                    printmessage16(edittext);
                    sector[sectnum].ceilingstat = (short)getnumber16(edittext,(int)sector[sectnum].ceilingstat,65536L,0);
                }
                break;
            case 1:
                for (i=Bsprintf(disptext,"(X,Y)pan: %d, %d",sector[sectnum].ceilingxpanning,sector[sectnum].ceilingypanning); i < dispwidth; i++) disptext[i] = ' ';
                if (editval)
                {
                    Bsprintf(edittext,"Sector %d Ceiling X Pan: ",sectnum);
                    printmessage16(edittext);
                    sector[sectnum].ceilingxpanning = (char)getnumber16(edittext,(int)sector[sectnum].ceilingxpanning,256L,0);
                    Bsprintf(edittext,"Sector %d Ceiling Y Pan: ",sectnum);
                    printmessage16(edittext);
                    sector[sectnum].ceilingypanning = (char)getnumber16(edittext,(int)sector[sectnum].ceilingypanning,256L,0);
                }
                break;
            case 2:
                for (i=Bsprintf(disptext,"Shade byte: %d",sector[sectnum].ceilingshade); i < dispwidth; i++) disptext[i] = ' ';
                Bsprintf(edittext,"Sector %d Ceiling Shade: ",sectnum);
                if (editval)
                {
                    printmessage16(edittext);
                    sector[sectnum].ceilingshade = (char)getnumber16(edittext,(int)sector[sectnum].ceilingshade,128L,1);
                }
                break;

            case 3:
                for (i=Bsprintf(disptext,"Z-coordinate: %d",sector[sectnum].ceilingz); i < dispwidth; i++) disptext[i] = ' ';
                Bsprintf(edittext,"Sector %d Ceiling Z-coordinate: ",sectnum);
                if (editval)
                {
                    printmessage16(edittext);
                    sector[sectnum].ceilingz = getnumber16(edittext,sector[sectnum].ceilingz,8388608,1); //2147483647L,-2147483648L
                }
                break;

            case 4:
                for (i=Bsprintf(disptext,"Tile number: %d",sector[sectnum].ceilingpicnum); i < dispwidth; i++) disptext[i] = ' ';
                Bsprintf(edittext,"Sector %d Ceiling Tile Number: ",sectnum);
                if (editval)
                {
                    printmessage16(edittext);
                    sector[sectnum].ceilingpicnum = (short)getnumber16(edittext,(int)sector[sectnum].ceilingpicnum,MAXTILES,0);
                }
                break;

            case 5:
                for (i=Bsprintf(disptext,"Ceiling heinum: %d",sector[sectnum].ceilingheinum); i < dispwidth; i++) disptext[i] = ' ';
                Bsprintf(edittext,"Sector %d Ceiling Heinum: ",sectnum);
                if (editval)
                {
                    printmessage16(edittext);
                    sector[sectnum].ceilingheinum = (short)getnumber16(edittext,(int)sector[sectnum].ceilingheinum,65536L,1);
                }
                break;

            case 6:
                for (i=Bsprintf(disptext,"Palookup number: %d",sector[sectnum].ceilingpal); i < dispwidth; i++) disptext[i] = ' ';
                Bsprintf(edittext,"Sector %d Ceiling Palookup Number: ",sectnum);
                if (editval)
                {
                    printmessage16(edittext);
                    sector[sectnum].ceilingpal = (char)getnumber16(edittext,(int)sector[sectnum].ceilingpal,MAXPALOOKUPS,0);
                }
                break;
            }
        }
        if (col == 2)
        {
            switch (row)
            {
            case 0:
                for (i=Bsprintf(disptext,"Flags (hex): %x",sector[sectnum].floorstat); i < dispwidth; i++) disptext[i] = ' ';
                Bsprintf(edittext,"Sector %d Floor Flags: ",sectnum);
                if (editval)
                {
                    printmessage16(edittext);
                    sector[sectnum].floorstat = (short)getnumber16(edittext,(int)sector[sectnum].floorstat,65536L,0);
                }
                break;

            case 1:
                for (i=Bsprintf(disptext,"(X,Y)pan: %d, %d",sector[sectnum].floorxpanning,sector[sectnum].floorypanning); i < dispwidth; i++) disptext[i] = ' ';
                if (editval)
                {
                    Bsprintf(edittext,"Sector %d Floor X Pan: ",sectnum);
                    printmessage16(edittext);
                    sector[sectnum].floorxpanning = (char)getnumber16(edittext,(int)sector[sectnum].floorxpanning,256L,0);
                    Bsprintf(edittext,"Sector %d Floor Y Pan: ",sectnum);
                    printmessage16(edittext);
                    sector[sectnum].floorypanning = (char)getnumber16(edittext,(int)sector[sectnum].floorypanning,256L,0);
                }
                break;

            case 2:
                for (i=Bsprintf(disptext,"Shade byte: %d",sector[sectnum].floorshade); i < dispwidth; i++) disptext[i] = ' ';
                Bsprintf(edittext,"Sector %d Floor Shade: ",sectnum);
                if (editval)
                {
                    printmessage16(edittext);
                    sector[sectnum].floorshade = (char)getnumber16(edittext,(int)sector[sectnum].floorshade,128L,1L);
                }
                break;

            case 3:
                for (i=Bsprintf(disptext,"Z-coordinate: %d",sector[sectnum].floorz); i < dispwidth; i++) disptext[i] = ' ';
                Bsprintf(edittext,"Sector %d Floor Z-coordinate: ",sectnum);
                if (editval)
                {
                    printmessage16(edittext);
                    sector[sectnum].floorz = getnumber16(edittext,sector[sectnum].floorz,8388608L,1); //2147483647L,-2147483648L
                }
                break;

            case 4:
                for (i=Bsprintf(disptext,"Tile number: %d",sector[sectnum].floorpicnum); i < dispwidth; i++) disptext[i] = ' ';
                Bsprintf(edittext,"Sector %d Floor Tile Number: ",sectnum);
                if (editval)
                {
                    printmessage16(edittext);
                    sector[sectnum].floorpicnum = (short)getnumber16(edittext,(int)sector[sectnum].floorpicnum,MAXTILES,0);
                }
                break;
            case 5:
                for (i=Bsprintf(disptext,"Floor heinum: %d",sector[sectnum].floorheinum); i < dispwidth; i++) disptext[i] = ' ';
                Bsprintf(edittext,"Sector %d Flooring Heinum: ",sectnum);
                if (editval)
                {
                    printmessage16(edittext);
                    sector[sectnum].floorheinum = (short)getnumber16(edittext,(int)sector[sectnum].floorheinum,65536L,1);
                }
                break;
            case 6:
                for (i=Bsprintf(disptext,"Palookup number: %d",sector[sectnum].floorpal); i < dispwidth; i++) disptext[i] = ' ';
                Bsprintf(edittext,"Sector %d Floor Palookup Number: ",sectnum);
                if (editval)
                {
                    printmessage16(edittext);
                    sector[sectnum].floorpal = (char)getnumber16(edittext,(int)sector[sectnum].floorpal,MAXPALOOKUPS,0);
                }
                break;
            }
        }
        printext16(xpos,ypos+row*8,11,1,disptext,0);
        if (editval)
        {
            editval = 0;
        }
        showframe(1);
    }
    printext16(xpos,ypos+row*8,11,0,disptext,0);
    printmessage16("");
    enddrawing();
    showframe(1);
    keystatus[KEYSC_ESC] = 0;
}

static void EditWallData(short wallnum)
{
    char disptext[80];
    char edittext[80];
    int row=0, dispwidth = 24, editval = 0, i = -1;
    int xpos = 200, ypos = ydim-STATUS2DSIZ+48;

    disptext[dispwidth] = 0;
    clearmidstatbar16();
    showwalldata(wallnum);
    begindrawing();
    while (keystatus[KEYSC_ESC] == 0)
    {
        if (handleevents())
        {
            if (quitevent) quitevent = 0;
        }
        idle();
        printmessage16("Edit mode, press <Esc> to exit");
        if (keystatus[KEYSC_DOWN])
        {
            if (row < 6)
            {
                printext16(xpos,ypos+row*8,11,0,disptext,0);
                row++;
            }
            keystatus[KEYSC_DOWN] = 0;
        }
        if (keystatus[KEYSC_UP])
        {
            if (row > 0)
            {
                printext16(xpos,ypos+row*8,11,0,disptext,0);
                row--;
            }
            keystatus[KEYSC_UP] = 0;
        }
        if (keystatus[KEYSC_ENTER])
        {
            keystatus[KEYSC_ENTER] = 0;
            editval = 1;
        }
        switch (row)
        {
        case 0:
            for (i=Bsprintf(disptext,"Flags (hex): %x",wall[wallnum].cstat); i < dispwidth; i++) disptext[i] = ' ';
            Bsprintf(edittext,"Wall %d Flags: ",wallnum);
            if (editval)
            {
                printmessage16(edittext);
                wall[wallnum].cstat = (short)getnumber16(edittext,(int)wall[wallnum].cstat,65536L,0);
            }
            break;
        case 1:
            for (i=Bsprintf(disptext,"Shade: %d",wall[wallnum].shade); i < dispwidth; i++) disptext[i] = ' ';
            Bsprintf(edittext,"Wall %d Shade: ",wallnum);
            if (editval)
            {
                printmessage16(edittext);
                wall[wallnum].shade = (char)getnumber16(edittext,(int)wall[wallnum].shade,128,1);
            }
            break;
        case 2:
            for (i=Bsprintf(disptext,"Pal: %d",wall[wallnum].pal); i < dispwidth; i++) disptext[i] = ' ';
            Bsprintf(edittext,"Wall %d Pal: ",wallnum);
            if (editval)
            {
                printmessage16(edittext);
                wall[wallnum].pal = (char)getnumber16(edittext,(int)wall[wallnum].pal,MAXPALOOKUPS,0);
            }
            break;
        case 3:
            for (i=Bsprintf(disptext,"(X,Y)repeat: %d, %d",wall[wallnum].xrepeat,wall[wallnum].yrepeat); i < dispwidth; i++) disptext[i] = ' ';
            if (editval)
            {
                Bsprintf(edittext,"Wall %d X Repeat: ",wallnum);
                printmessage16(edittext);
                wall[wallnum].xrepeat = (char)getnumber16(edittext,(int)wall[wallnum].xrepeat,256L,0);
                Bsprintf(edittext,"Wall %d Y Repeat: ",wallnum);
                printmessage16(edittext);
                wall[wallnum].yrepeat = (char)getnumber16(edittext,(int)wall[wallnum].yrepeat,256L,0);
            }
            break;
        case 4:
            for (i=Bsprintf(disptext,"(X,Y)pan: %d, %d",wall[wallnum].xpanning,wall[wallnum].ypanning); i < dispwidth; i++) disptext[i] = ' ';
            if (editval)
            {
                Bsprintf(edittext,"Wall %d X Pan: ",wallnum);
                printmessage16(edittext);
                wall[wallnum].xpanning = (char)getnumber16(edittext,(int)wall[wallnum].xpanning,256L,0);
                Bsprintf(edittext,"Wall %d Y Pan: ",wallnum);
                printmessage16(edittext);
                wall[wallnum].ypanning = (char)getnumber16(edittext,(int)wall[wallnum].ypanning,256L,0);
            }
            break;
        case 5:
            for (i=Bsprintf(disptext,"Tile number: %d",wall[wallnum].picnum); i < dispwidth; i++) disptext[i] = ' ';
            Bsprintf(edittext,"Wall %d Tile number: ",wallnum);
            if (editval)
            {
                printmessage16(edittext);
                wall[wallnum].picnum = (short)getnumber16(edittext,(int)wall[wallnum].picnum,MAXTILES,0);
            }
            break;

        case 6:
            for (i=Bsprintf(disptext,"OverTile number: %d",wall[wallnum].overpicnum); i < dispwidth; i++) disptext[i] = ' ';
            Bsprintf(edittext,"Wall %d OverTile number: ",wallnum);
            if (editval)
            {
                printmessage16(edittext);
                wall[wallnum].overpicnum = (short)getnumber16(edittext,(int)wall[wallnum].overpicnum,MAXTILES,0);
            }
            break;
        }
        printext16(xpos,ypos+row*8,11,1,disptext,0);
        if (editval)
        {
            editval = 0;
            //showwalldata(wallnum);
            //printmessage16("");
        }
        //enddrawing();
        showframe(1);
    }
    //begindrawing();
    printext16(xpos,ypos+row*8,11,0,disptext,0);
    printmessage16("");
    enddrawing();
    showframe(1);
    keystatus[KEYSC_ESC] = 0;
}

static void EditSpriteData(short spritenum)
{
    char disptext[80];
    char edittext[80];
    int col=0, row=0, rowmax=4, dispwidth = 24, editval = 0, i = -1;
    int xpos = 8, ypos = ydim-STATUS2DSIZ+48;

    disptext[dispwidth] = 0;
    clearmidstatbar16();
    showspritedata(spritenum);

    while (keystatus[KEYSC_ESC] == 0)
    {
        begindrawing();
        if (handleevents())
        {
            if (quitevent) quitevent = 0;
        }
        idle();
        printmessage16("Edit mode, press <Esc> to exit");
        if (keystatus[KEYSC_DOWN])
        {
            if (row < rowmax)
            {
                printext16(xpos,ypos+row*8,11,0,disptext,0);
                row++;
            }
            keystatus[KEYSC_DOWN] = 0;
        }
        if (keystatus[KEYSC_UP])
        {
            if (row > 0)
            {
                printext16(xpos,ypos+row*8,11,0,disptext,0);
                row--;
            }
            keystatus[KEYSC_UP] = 0;
        }
        if (keystatus[KEYSC_LEFT])
        {
            switch (col)
            {
            case 1:
            {
                printext16(xpos,ypos+row*8,11,0,disptext,0);
                col = 0;
                xpos = 8;
                rowmax = 4;
                dispwidth = 23;
                disptext[dispwidth] = 0;
                if (row > rowmax) row = rowmax;
            }
            break;
            case 2:
            {
                printext16(xpos,ypos+row*8,11,0,disptext,0);
                col = 1;
                xpos = 200;
                rowmax = 5;
                dispwidth = 24;
                disptext[dispwidth] = 0;
                if (row > rowmax) row = rowmax;
            }
            break;
            }
            keystatus[KEYSC_LEFT] = 0;
        }
        if (keystatus[KEYSC_RIGHT])
        {
            switch (col)
            {
            case 0:
            {
                printext16(xpos,ypos+row*8,11,0,disptext,0);
                col = 1;
                xpos = 200;
                rowmax = 5;
                dispwidth = 24;
                disptext[dispwidth] = 0;
                if (row > rowmax) row = rowmax;
            }
            break;
            case 1:
            {
                printext16(xpos,ypos+row*8,11,0,disptext,0);
                col = 2;
                xpos = 400;
                rowmax = 6;
                dispwidth = 26;
                disptext[dispwidth] = 0;
                if (row > rowmax) row = rowmax;
            }
            break;
            }
            keystatus[KEYSC_RIGHT] = 0;
        }
        if (keystatus[KEYSC_ENTER])
        {
            keystatus[KEYSC_ENTER] = 0;
            editval = 1;
        }
        switch (col)
        {
        case 0:
        {
            switch (row)
            {
            case 0:
            {
                for (i=Bsprintf(disptext,"X-coordinate: %d",sprite[spritenum].x); i < dispwidth; i++) disptext[i] = ' ';
                Bsprintf(edittext,"Sprite %d X-coordinate: ",spritenum);
                if (editval)
                {
                    printmessage16(edittext);
                    sprite[spritenum].x = getnumber16(edittext,sprite[spritenum].x,131072,1);
                }
            }
            break;
            case 1:
            {
                for (i=Bsprintf(disptext,"Y-coordinate: %d",sprite[spritenum].y); i < dispwidth; i++) disptext[i] = ' ';
                Bsprintf(edittext,"Sprite %d Y-coordinate: ",spritenum);
                if (editval)
                {
                    printmessage16(edittext);
                    sprite[spritenum].y = getnumber16(edittext,sprite[spritenum].y,131072,1);
                }
            }
            break;
            case 2:
            {
                for (i=Bsprintf(disptext,"Z-coordinate: %d",sprite[spritenum].z); i < dispwidth; i++) disptext[i] = ' ';
                Bsprintf(edittext,"Sprite %d Z-coordinate: ",spritenum);
                if (editval)
                {
                    printmessage16(edittext);
                    sprite[spritenum].z = getnumber16(edittext,sprite[spritenum].z,8388608,1); //2147483647L,-2147483648L
                }
            }
            break;
            case 3:
            {
                for (i=Bsprintf(disptext,"Sectnum: %d",sprite[spritenum].sectnum); i < dispwidth; i++) disptext[i] = ' ';
                Bsprintf(edittext,"Sprite %d Sectnum: ",spritenum);
                if (editval)
                {
                    printmessage16(edittext);
                    i = getnumber16(edittext,sprite[spritenum].sectnum,MAXSECTORS-1,0);
                    if (i != sprite[spritenum].sectnum)
                        changespritesect(spritenum,i);
                }
            }
            break;
            case 4:
            {
                for (i=Bsprintf(disptext,"Statnum: %d",sprite[spritenum].statnum); i < dispwidth; i++) disptext[i] = ' ';
                Bsprintf(edittext,"Sprite %d Statnum: ",spritenum);
                if (editval)
                {
                    printmessage16(edittext);
                    i = getnumber16(edittext,sprite[spritenum].statnum,MAXSTATUS-1,0);
                    if (i != sprite[spritenum].statnum)
                        changespritestat(spritenum,i);
                }
            }
            break;
            }
        }
        break;
        case 1:
        {
            switch (row)
            {
            case 0:
            {
                for (i=Bsprintf(disptext,"Flags (hex): %x",sprite[spritenum].cstat); i < dispwidth; i++) disptext[i] = ' ';
                Bsprintf(edittext,"Sprite %d Flags: ",spritenum);
                if (editval)
                {
                    printmessage16(edittext);
                    sprite[spritenum].cstat = (short)getnumber16(edittext,(int)sprite[spritenum].cstat,65536L,0);
                }
            }
            break;
            case 1:
            {
                for (i=Bsprintf(disptext,"Shade: %d",sprite[spritenum].shade); i < dispwidth; i++) disptext[i] = ' ';
                Bsprintf(edittext,"Sprite %d Shade: ",spritenum);
                if (editval)
                {
                    printmessage16(edittext);
                    sprite[spritenum].shade = (char)getnumber16(edittext,(int)sprite[spritenum].shade,128,1);
                }
            }
            break;
            case 2:
            {
                for (i=Bsprintf(disptext,"Pal: %d",sprite[spritenum].pal); i < dispwidth; i++) disptext[i] = ' ';
                Bsprintf(edittext,"Sprite %d Pal: ",spritenum);
                if (editval)
                {
                    printmessage16(edittext);
                    sprite[spritenum].pal = (char)getnumber16(edittext,(int)sprite[spritenum].pal,MAXPALOOKUPS,0);
                }
            }
            break;
            case 3:
            {
                for (i=Bsprintf(disptext,"(X,Y)repeat: %d, %d",sprite[spritenum].xrepeat,sprite[spritenum].yrepeat); i < dispwidth; i++) disptext[i] = ' ';
                if (editval)
                {
                    Bsprintf(edittext,"Sprite %d X Repeat: ",spritenum);
                    printmessage16(edittext);
                    sprite[spritenum].xrepeat = (char)getnumber16(edittext,(int)sprite[spritenum].xrepeat,256L,0);
                    Bsprintf(edittext,"Sprite %d Y Repeat: ",spritenum);
                    printmessage16(edittext);
                    sprite[spritenum].yrepeat = (char)getnumber16(edittext,(int)sprite[spritenum].yrepeat,256L,0);
                }
            }
            break;
            case 4:
            {
                for (i=Bsprintf(disptext,"(X,Y)offset: %d, %d",sprite[spritenum].xoffset,sprite[spritenum].yoffset); i < dispwidth; i++) disptext[i] = ' ';
                if (editval)
                {
                    Bsprintf(edittext,"Sprite %d X Offset: ",spritenum);
                    printmessage16(edittext);
                    sprite[spritenum].xoffset = (char)getnumber16(edittext,(int)sprite[spritenum].xoffset,128L,1);
                    Bsprintf(edittext,"Sprite %d Y Offset: ",spritenum);
                    printmessage16(edittext);
                    sprite[spritenum].yoffset = (char)getnumber16(edittext,(int)sprite[spritenum].yoffset,128L,1);
                }
            }
            break;
            case 5:
            {
                for (i=Bsprintf(disptext,"Tile number: %d",sprite[spritenum].picnum); i < dispwidth; i++) disptext[i] = ' ';
                Bsprintf(edittext,"Sprite %d Tile number: ",spritenum);
                if (editval)
                {
                    printmessage16(edittext);
                    sprite[spritenum].picnum = (short)getnumber16(edittext,(int)sprite[spritenum].picnum,MAXTILES,0);
                }
            }
            break;
            }
        }
        break;
        case 2:
        {
            switch (row)
            {
            case 0:
            {
                for (i=Bsprintf(disptext,"Angle (2048 degrees): %d",sprite[spritenum].ang); i < dispwidth; i++) disptext[i] = ' ';
                Bsprintf(edittext,"Sprite %d Angle: ",spritenum);
                if (editval)
                {
                    printmessage16(edittext);
                    sprite[spritenum].ang = (short)getnumber16(edittext,(int)sprite[spritenum].ang,2048L,0);
                }
            }
            break;
            case 1:
            {
                for (i=Bsprintf(disptext,"X-Velocity: %d",sprite[spritenum].xvel); i < dispwidth; i++) disptext[i] = ' ';
                Bsprintf(edittext,"Sprite %d X-Velocity: ",spritenum);
                if (editval)
                {
                    printmessage16(edittext);
                    sprite[spritenum].xvel = getnumber16(edittext,(int)sprite[spritenum].xvel,65536,1);
                }
            }
            break;
            case 2:
            {
                for (i=Bsprintf(disptext,"Y-Velocity: %d",sprite[spritenum].yvel); i < dispwidth; i++) disptext[i] = ' ';
                Bsprintf(edittext,"Sprite %d Y-Velocity: ",spritenum);
                if (editval)
                {
                    printmessage16(edittext);
                    sprite[spritenum].yvel = getnumber16(edittext,(int)sprite[spritenum].yvel,65536,1);
                }
            }
            break;
            case 3:
            {
                for (i=Bsprintf(disptext,"Z-Velocity: %d",sprite[spritenum].zvel); i < dispwidth; i++) disptext[i] = ' ';
                Bsprintf(edittext,"Sprite %d Z-Velocity: ",spritenum);
                if (editval)
                {
                    printmessage16(edittext);
                    sprite[spritenum].zvel = getnumber16(edittext,(int)sprite[spritenum].zvel,65536,1);
                }
            }
            break;
            case 4:
            {
                for (i=Bsprintf(disptext,"Owner: %d",sprite[spritenum].owner); i < dispwidth; i++) disptext[i] = ' ';
                Bsprintf(edittext,"Sprite %d Owner: ",spritenum);
                if (editval)
                {
                    printmessage16(edittext);
                    sprite[spritenum].owner = getnumber16(edittext,(int)sprite[spritenum].owner,MAXSPRITES,0);
                }
            }
            break;
            case 5:
            {
                for (i=Bsprintf(disptext,"Clipdist: %d",sprite[spritenum].clipdist); i < dispwidth; i++) disptext[i] = ' ';
                Bsprintf(edittext,"Sprite %d Clipdist: ",spritenum);
                if (editval)
                {
                    printmessage16(edittext);
                    sprite[spritenum].clipdist = (char)getnumber16(edittext,(int)sprite[spritenum].clipdist,256,0);
                }
            }
            break;
            case 6:
            {
                for (i=Bsprintf(disptext,"Extra: %d",sprite[spritenum].extra); i < dispwidth; i++) disptext[i] = ' ';
                Bsprintf(edittext,"Sprite %d Extra: ",spritenum);
                if (editval)
                {
                    printmessage16(edittext);
                    sprite[spritenum].extra = getnumber16(edittext,(int)sprite[spritenum].extra,65536,1);
                }
            }
            break;
            }
        }
        break;
        }

        printext16(xpos,ypos+row*8,11,1,disptext,0);
        if (editval)
        {
            editval = 0;
        }
        enddrawing();
        showframe(1);
    }
    begindrawing();
    printext16(xpos,ypos+row*8,11,0,disptext,0);
    printmessage16("");
    enddrawing();
    showframe(1);
    keystatus[KEYSC_ESC] = 0;
}

static void GenSearchSprite()
{
    char disptext[80];
    char edittext[80];
    static int col=0, row=0;
    int i, j, k;
    int rowmax[3]={6,5,6}, dispwidth[3] = {24,24,28};
    int xpos[3] = {8,200,400}, ypos = ydim-STATUS2DSIZ+48;

    static char *labels[7][3] =
    {
        {"X-coordinate", "Flags (hex)", "Angle (2048 degrees)"},
        {"Y-coordinate", "Shade",       "X-Velocity"},
        {"Z-coordinate", "Pal",         "Y-Velocity"},
        {"Sectnum",      "(X/Y)repeat", "Z-Velocity"},
        {"Statnum",      "(X/Y)offset", "Owner"},
        {"Hitag",        "Tile number", "Clipdist"},
        {"Lotag",        "",            "Extra"}
    };

    static int maxval[7][3] =
    {
        { 524288      , 65536       , 2048 },
        { 524288      , 128         , 65536 },
        { 8388608     , MAXPALOOKUPS, 65536 },
        { MAXSECTORS-1, 128         , 65536 },
        { MAXSTATUS-1 , 128         , MAXSPRITES-1 },
        { 65536       , MAXTILES    , 256 },
        { 65536       , 0           , 65536 }
    };

    static char sign[7][3] =
    {
        {1,0,1}, {1,1,1}, {1,0,1}, {0,0,1}, {0,1,0}, {0,0,0}, {0,0,1}
    };

    static char firstrun=1;

    if (firstrun)
    {
        firstrun = 0;
        Bmemset(&gs_spritewhat, 0, sizeof(gs_spritewhat));
    }

    clearmidstatbar16();

    printext16(xpos[0], ypos-2*8, 10, 0, "Sprite search", 0);

    for (i=0; i<3; i++)
        for (j=0; j<=rowmax[i]; j++)
        {
            if (gs_spritewhat[i][j])
                k=Bsprintf(disptext, "%s: ^O%d", labels[j][i], gs_sprite[i][j]);
            else
                k=Bsprintf(disptext, "%s: ^7any", labels[j][i]);
            for (;k<dispwidth[i];k++) disptext[k] = 0;

            printext16(xpos[i], ypos+j*8, 11, 0, disptext, 0);
        }
    for (k=0; k<80; k++) disptext[k] = 0;

//    disptext[dispwidth[col]] = 0;
//    showspritedata(spritenum);
    wallsprite = 2;

    while (keystatus[KEYSC_ESC] == 0)
    {
        begindrawing();
        if (handleevents())
        {
            if (quitevent) quitevent = 0;
        }
        idle();
        printmessage16("Sprite search, press <Esc> to exit");

        if (keystatus[KEYSC_DOWN])
        {
            keystatus[KEYSC_DOWN] = 0;
            if (row < rowmax[col])
            {
                printext16(xpos[col],ypos+row*8,11,0,disptext,0);
                row++;
            }
        }
        if (keystatus[KEYSC_UP])
        {
            keystatus[KEYSC_UP] = 0;
            if (row > 0)
            {
                printext16(xpos[col],ypos+row*8,11,0,disptext,0);
                row--;
            }
        }
        if (keystatus[KEYSC_LEFT])
        {
            keystatus[KEYSC_LEFT] = 0;
            if (col > 0)
            {
                printext16(xpos[col],ypos+row*8,11,0,disptext,0);
                col--;
                disptext[dispwidth[col]] = 0;
                if (row > rowmax[col]) row = rowmax[col];
            }
        }
        if (keystatus[KEYSC_RIGHT])
        {
            keystatus[KEYSC_RIGHT] = 0;
            if (col < 2)
            {
                printext16(xpos[col],ypos+row*8,11,0,disptext,0);
                col++;
                disptext[dispwidth[col]] = 0;
                if (row > rowmax[col]) row = rowmax[col];
            }
        }
        if (keystatus[KEYSC_ENTER])
        {
            keystatus[KEYSC_ENTER] = 0;
            Bsprintf(edittext, "%s: ", labels[row][col]);
            enddrawing();
            printmessage16(edittext);
            i = getnumber16(edittext, gs_spritewhat[col][row] ? gs_sprite[col][row] : 0,
                            maxval[row][col], sign[row][col]);
            if (col == 2 && row == 0) i = (i+2048)&2047;  // angle
            gs_sprite[col][row] = i;
            begindrawing();
            gs_spritewhat[col][row] = 1;

            if (col == 1 && row == 5)  // picnum
                printext16(xpos[1], ypos-2*8, 14, 0, names[i], 0);
        }
        if (keystatus[KEYSC_BS] || keystatus[KEYSC_DELETE])
        {
            keystatus[KEYSC_BS] = keystatus[KEYSC_DELETE] = 0;
            gs_spritewhat[col][row] = 0;

            if (col == 1 && row == 5)  // picnum
                printext16(xpos[1], ypos-2*8, 14, 0, "                         ", 0);
        }

        if (gs_spritewhat[col][row])
        {
            if (col == 1 && row == 0)  // flags
                k = Bsprintf(disptext, "%s: ^O%x", labels[row][col], gs_sprite[col][row]);
            else
                k = Bsprintf(disptext, "%s: ^O%d", labels[row][col], gs_sprite[col][row]);
        }
        else
            k = Bsprintf(disptext, "%s: ^7any", labels[row][col]);
        for (;k<dispwidth[col];k++) disptext[k] = ' ';
        disptext[k] = 0;

        printext16(xpos[col],ypos+row*8,11,1,disptext,0);

        enddrawing();
        showframe(1);
    }
//    begindrawing();
    printext16(xpos[col],ypos+row*8,11,0,disptext,0);
    printmessage16("Search sprite");
//    enddrawing();
    showframe(1);
    keystatus[KEYSC_ESC] = 0;
}

// Build edit

static char *FuncMenuStrings[] =
{
    "Replace invalid tiles",
    "Delete all spr of tile #",
    "Set map sky shade",
    "Set map sky height",
    "Global Z coord shift",
    "Resize selection",
    "Global shade divide",
    "Global visibility divide"
};

#define MENU_Y_SPACING 8
#define MENU_BASE_Y ydim-STATUS2DSIZ+32


static void FuncMenuOpts(void)
{
    int x = 8;
    int y = MENU_BASE_Y+16;
    int i = 0;
    //  int x2 = 0;
//    static int x2_max = 0;

    int numopts = (sizeof(FuncMenuStrings)/sizeof(FuncMenuStrings[0]));

    do
    {
//        x2 =
        printext16(x,y,11,0,FuncMenuStrings[i],0);
        //    if (x2 > x2_max) x2_max = x2;
        y += MENU_Y_SPACING;
    }
    while (++i < numopts);
//    drawline16(x-1,y,x2_max+1,y,1);
    //  drawline16(x-1,MENU_BASE_Y-4,x-1,y,1);

//    x2 =
    printext16(x,MENU_BASE_Y,11,0,"Special functions",0);
//    drawline16(x-1,MENU_BASE_Y-4,x2+1,MENU_BASE_Y-4,1);
    //  drawline16(x2_max+1,MENU_BASE_Y+16-4,x2_max+1,y-1,1);
    //drawline16(x2+1,MENU_BASE_Y+16-1,x2_max+1,MENU_BASE_Y+16-1,1);
}

static void FuncMenu(void)
{
    char disptext[80];
    int col=0, row=0, rowmax=7, dispwidth = 24, editval = 0, i = -1, j;
    int xpos = 8, ypos = MENU_BASE_Y+16;

    disptext[dispwidth] = 0;
    clearmidstatbar16();

    FuncMenuOpts();

    while (!editval && keystatus[KEYSC_ESC] == 0)
    {
        begindrawing();
        if (handleevents())
        {
            if (quitevent) quitevent = 0;
        }
        idle();
        printmessage16("Select an option, press <Esc> to exit");
        if (keystatus[KEYSC_DOWN])
        {
            if (row < rowmax)
            {
                printext16(xpos,ypos+row*MENU_Y_SPACING,11,0,disptext,0);
                row++;
            }
            keystatus[KEYSC_DOWN] = 0;
        }
        if (keystatus[KEYSC_UP])
        {
            if (row > 0)
            {
                printext16(xpos,ypos+row*MENU_Y_SPACING,11,0,disptext,0);
                row--;
            }
            keystatus[KEYSC_UP] = 0;
        }
#if 0
        if (keystatus[KEYSC_LEFT])
        {
            /*            if (col == 2)
                        {
                            printext16(xpos,ypos+row*8,11,0,disptext,0);
                            col = 1;
                            xpos = 200;
                            rowmax = 6;
                            dispwidth = 24;
                            disptext[dispwidth] = 0;
                            if (row > rowmax) row = rowmax;
                        }
                        else */
            if (col == 1)
            {
                printext16(xpos,ypos+row*8,11,0,disptext,0);
                col = 0;
                xpos = 8;
                rowmax = 7;
                dispwidth = 24;
                disptext[dispwidth] = 0;
                if (row > rowmax) row = rowmax;
            }
            keystatus[KEYSC_LEFT] = 0;
        }

        if (keystatus[KEYSC_RIGHT])
        {
            if (col == 0)
            {
                printext16(xpos,ypos+row*8,11,0,disptext,0);
                col = 1;
                xpos = 200;
                rowmax = 0;
                dispwidth = 24;
                disptext[dispwidth] = 0;
                if (row > rowmax) row = rowmax;
            }
            /*            else if (col == 1)
                        {
                            printext16(xpos,ypos+row*8,11,0,disptext,0);
                            col = 2;
                            xpos = 400;
                            rowmax = 6;
                            dispwidth = 26;
                            disptext[dispwidth] = 0;
                            if (row > rowmax) row = rowmax;
                        } */
            keystatus[KEYSC_RIGHT] = 0;
        }
#endif
        if (keystatus[KEYSC_ENTER])
        {
            keystatus[KEYSC_ENTER] = 0;
            editval = 1;
        }
        switch (col)
        {
        case 0:
            switch (row)
            {
            case 0:
            {
                for (i=Bsprintf(disptext,"%s",FuncMenuStrings[row]); i < dispwidth; i++) disptext[i] = ' ';
                if (editval)
                {
                    j = 0;
                    for (i=0;i<MAXSECTORS;i++)
                    {
                        if (tilesizx[sector[i].ceilingpicnum] <= 0)
                            sector[i].ceilingpicnum = 0,j++;
                        if (tilesizx[sector[i].floorpicnum] <= 0)
                            sector[i].floorpicnum = 0,j++;
                    }
                    for (i=0;i<MAXWALLS;i++)
                    {
                        if (tilesizx[wall[i].picnum] <= 0)
                            wall[i].picnum = 0,j++;
                        if (tilesizx[wall[i].overpicnum] <= 0)
                            wall[i].overpicnum = 0,j++;
                    }
                    for (i=0;i<MAXSPRITES;i++)
                    {
                        if (tilesizx[sprite[i].picnum] <= 0)
                            sprite[i].picnum = 0,j++;
                    }
                    Bsprintf(tempbuf,"Replaced %d invalid tiles",j);
                    printmessage16(tempbuf);
                }
            }
            break;
            case 1:
            {
                for (i=Bsprintf(disptext,"%s",FuncMenuStrings[row]); i < dispwidth; i++) disptext[i] = ' ';
                if (editval)
                {
                    Bsprintf(tempbuf,"Delete all sprites of tile #: ");
                    i = getnumber16(tempbuf,-1,MAXSPRITES-1,1);
                    if (i >= 0)
                    {
                        int k = 0;
                        for (j=0;j<MAXSPRITES-1;j++)
                            if (sprite[j].picnum == i)
                                deletesprite(j), k++;
                        Bsprintf(tempbuf,"%d sprite(s) deleted",k);
                        printmessage16(tempbuf);
                    }
                    else printmessage16("Aborted");
                }
            }
            break;
            case 2:
            {
                for (i=Bsprintf(disptext,"%s",FuncMenuStrings[row]); i < dispwidth; i++) disptext[i] = ' ';
                if (editval)
                {
                    j=getnumber16("Set map sky shade:    ",0,128,1);

                    for (i=0;i<numsectors;i++)
                    {
                        if (sector[i].ceilingstat&1)
                            sector[i].ceilingshade = j;
                    }
                    printmessage16("All parallax skies adjusted");
                }
            }
            break;
            case 3:
            {
                for (i=Bsprintf(disptext,"%s",FuncMenuStrings[row]); i < dispwidth; i++) disptext[i] = ' ';
                if (editval)
                {
                    j=getnumber16("Set map sky height:    ",0,16777216,1);
                    if (j != 0)
                    {
                        for (i=0;i<numsectors;i++)
                        {
                            if (sector[i].ceilingstat&1)
                                sector[i].ceilingz = j;
                        }
                        printmessage16("All parallax skies adjusted");
                    }
                    else printmessage16("Aborted");
                }
            }
            break;
            case 4:
            {
                for (i=Bsprintf(disptext,"%s",FuncMenuStrings[row]); i < dispwidth; i++) disptext[i] = ' ';
                if (editval)
                {
                    j=getnumber16("Z offset:    ",0,16777216,1);
                    if (j!=0)
                    {
                        for (i=0;i<numsectors;i++)
                        {
                            sector[i].ceilingz += j;
                            sector[i].floorz += j;
                        }
                        for (i=0;i<MAXSPRITES;i++)
                            sprite[i].z += j;
                        printmessage16("Map adjusted");
                    }
                    else printmessage16("Aborted");
                }
            }
            break;
            case 5:
            {
                for (i=Bsprintf(disptext,"%s",FuncMenuStrings[row]); i < dispwidth; i++) disptext[i] = ' ';
                if (editval)
                {
                    j=getnumber16("Percentage of original:    ",100,1000,0);
                    if (j!=100)
                    {
                        int w, currsector, start_wall, end_wall;
                        double size = (j/100.f);
                        for (i = 0; i < highlightsectorcnt; i++)
                        {
                            currsector = highlightsector[i];
                            sector[currsector].ceilingz = (int)(sector[currsector].ceilingz*size);
                            sector[currsector].floorz = (int)(sector[currsector].floorz*size);
                            // Do all the walls in the sector
                            start_wall = sector[currsector].wallptr;
                            end_wall = start_wall + sector[currsector].wallnum;
                            for (w = start_wall; w < end_wall; w++)
                            {
                                wall[w].x = (int)(wall[w].x*size);
                                wall[w].y = (int)(wall[w].y*size);
                                wall[w].yrepeat = min((int)(wall[w].yrepeat/size),255);
                            }
                            w = headspritesect[highlightsector[i]];
                            while (w >= 0)
                            {
                                sprite[w].x = (int)(sprite[w].x*size);
                                sprite[w].y = (int)(sprite[w].y*size);
                                sprite[w].z = (int)(sprite[w].z*size);
                                sprite[w].xrepeat = min(max((int)(sprite[w].xrepeat*size),1),255);
                                sprite[w].yrepeat = min(max((int)(sprite[w].yrepeat*size),1),255);
                                w = nextspritesect[w];
                            }
                        }
                        printmessage16("Map scaled");
                    }
                    else printmessage16("Aborted");
                }
            }
            break;
            case 6:
            {
                for (i=Bsprintf(disptext,"%s",FuncMenuStrings[row]); i < dispwidth; i++) disptext[i] = ' ';
                if (editval)
                {
                    j=getnumber16("Shade divisor:    ",1,128,1);
                    if (j!=1)
                    {
                        for (i=0;i<numsectors;i++)
                        {
                            sector[i].ceilingshade /= j;
                            sector[i].floorshade /= j;
                        }
                        for (i=0;i<numwalls;i++)
                            wall[i].shade /= j;
                        for (i=0;i<MAXSPRITES;i++)
                            sprite[i].shade /= j;
                        printmessage16("Shades adjusted");
                    }
                    else printmessage16("Aborted");
                }
            }
            break;
            case 7:
            {
                for (i=Bsprintf(disptext,"%s",FuncMenuStrings[row]); i < dispwidth; i++) disptext[i] = ' ';
                if (editval)
                {
                    j=getnumber16("Visibility divisor:    ",1,128,0);
                    if (j!=1)
                    {
                        for (i=0;i<numsectors;i++)
                        {
                            if (sector[i].visibility < 240)
                                sector[i].visibility /= j;
                            else sector[i].visibility = 240 + (sector[i].visibility>>4)/j;
                        }
                        printmessage16("Visibility adjusted");
                    }
                    else printmessage16("Aborted");
                }
            }
            break;
            }
            break;
        }
        printext16(xpos,ypos+row*MENU_Y_SPACING,11,1,disptext,0);
        enddrawing();
        showframe(1);
    }
    begindrawing();
    printext16(xpos,ypos+row*MENU_Y_SPACING,11,0,disptext,0);
    enddrawing();
    clearmidstatbar16();
    showframe(1);
    keystatus[KEYSC_ESC] = 0;
}
/*
#define UNDODEPTH 96

typedef struct _mapundo
{
    int numsectors;
    int numwalls;
    int numsprites;

    sectortype *sectors;
    walltype *walls;
    spritetype *sprites;

    short *headspritesect;
    short *prevspritesect;
    short *nextspritesect;

    short *headspritestat;
    short *prevspritestat;
    short *nextspritestat;

    int revision;

    struct _mapundo *next; // 'redo' loads this
    struct _mapundo *prev; // 'undo' loads this
} mapundo_t;

mapundo_t *undopos = NULL;
mapundo_t undoredo[UNDODEPTH];

int map_revision = 0;
*/
