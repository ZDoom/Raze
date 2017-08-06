// Ken Silverman's official web site: "http://www.advsys.net/ken"
// See the included license file "BUILDLIC.TXT" for license info.
//
// This file has been modified from Ken Silverman's original release
// by Jonathon Fowler (jf@jonof.id.au)

#include "compat.h"
#include "build.h"
#include "names.h"
#include "pragmas.h"
#include "cache1d.h"
#include "game.h"
#include "osd.h"
#include "mmulti.h"
#include "common.h"

#include "renderlayer.h"

#include "common_game.h"

const char *AppProperName = "KenBuild";
const char *AppTechnicalName = "testgame";

#define SETUPFILENAME "testgame.cfg"
char setupfilename[BMAX_PATH] = SETUPFILENAME;

#define TIMERINTSPERSECOND 140 //280
#define MOVESPERSECOND 40
#define TICSPERFRAME 3
#define MOVEFIFOSIZ 256
#define EYEHEIGHT (32<<8)   //Normally (32<<8), (51<<8) to make mirrors happy

#define TILE_TILT           (MAXTILES-2)


static int32_t setsprite_eyeheight(int16_t spritenum, const vec3_t *pos) ATTRIBUTE((nonnull(2)));
static int32_t setsprite_eyeheight(int16_t spritenum, const vec3_t *pos)
{
    vec3_t eyepos = *pos;
    eyepos.z += EYEHEIGHT;
    return setsprite(spritenum, &eyepos);
}


// declared in sound.c
void initsb(char,char,int,char,char,char,char);
void uninitsb(void);
void setears(int,int,int,int);
void wsayfollow(char const *,int,int,int *,int *,char);
void wsay(char const *,int,int,int);
void loadwaves(void);
void loadsong(char const *);
void musicon(void);
void musicoff(void);
void refreshaudio(void);

// declared in config.c
int Ken_loadsetup(const char *);
int Ken_writesetup(const char *);

/***************************************************************************
    KEN'S TAG DEFINITIONS:      (Please define your own tags for your games)

 sector[?].lotag = 0   Normal sector
 sector[?].lotag = 1   If you are on a sector with this tag, then all sectors
                       with same hi tag as this are operated.  Once.
 sector[?].lotag = 2   Same as sector[?].tag = 1 but this is retriggable.
 sector[?].lotag = 3   A really stupid sector that really does nothing now.
 sector[?].lotag = 4   A sector where you are put closer to the floor
                       (such as the slime in DOOM1.DAT)
 sector[?].lotag = 5   A really stupid sector that really does nothing now.
 sector[?].lotag = 6   A normal door - instead of pressing D, you tag the
                       sector with a 6.  The reason I make you edit doors
                       this way is so that can program the doors
                       yourself.
 sector[?].lotag = 7   A door the goes down to open.
 sector[?].lotag = 8   A door that opens horizontally in the middle.
 sector[?].lotag = 9   A sliding door that opens vertically in the middle.
                       -Example of the advantages of not using BSP tree.
 sector[?].lotag = 10  A warping sector with floor and walls that shade.
 sector[?].lotag = 11  A sector with all walls that do X-panning.
 sector[?].lotag = 12  A sector with walls using the dragging function.
 sector[?].lotag = 13  A sector with some swinging doors in it.
 sector[?].lotag = 14  A revolving door sector.
 sector[?].lotag = 15  A subway track.
 sector[?].lotag = 16  A true double-sliding door.

    wall[?].lotag = 0   Normal wall
    wall[?].lotag = 1   Y-panning wall
    wall[?].lotag = 2   Switch - If you flip it, then all sectors with same hi
                        tag as this are operated.
    wall[?].lotag = 3   Marked wall to detemine starting dir. (sector tag 12)
    wall[?].lotag = 4   Mark on the shorter wall closest to the pivot point
                        of a swinging door. (sector tag 13)
    wall[?].lotag = 5   Mark where a subway should stop. (sector tag 15)
    wall[?].lotag = 6   Mark for true double-sliding doors (sector tag 16)
    wall[?].lotag = 7   Water fountain
    wall[?].lotag = 8   Bouncy wall!

 sprite[?].lotag = 0   Normal sprite
 sprite[?].lotag = 1   If you press space bar on an AL, and the AL is tagged
                       with a 1, he will turn evil.
 sprite[?].lotag = 2   When this sprite is operated, a bomb is shot at its
                       position.
 sprite[?].lotag = 3   Rotating sprite.
 sprite[?].lotag = 4   Sprite switch.
 sprite[?].lotag = 5   Basketball hoop score.

    KEN'S STATUS DEFINITIONS:  (Please define your own statuses for your games)
 status = 0            Inactive sprite
 status = 1            Active monster sprite
 status = 2            Monster that becomes active only when it sees you
 status = 3            Smoke on the wall for chainguns
 status = 4            Splashing sprites (When you shoot slime)
 status = 5            Explosion!
 status = 6            Travelling bullet
 status = 7            Bomb sprial-out explosion
 status = 8            Player!
 status = 9            EVILALGRAVE shrinking list
 status = 10           EVILAL list
 status = 11           Sprite respawning list
 status = 12           Sprite which does not respawn (Andy's addition)
 status = MAXSTATUS    Non-existent sprite (this will be true for your
                       code also)
**************************************************************************/

typedef struct
{
    signed char fvel, svel, avel;
    short bits;
} input;

static int screentilt = 0, oscreentilt = 0;


static int fvel, svel, avel;
static int fvel2, svel2, avel2;

#define NUMOPTIONS 8
#define NUMGAMEKEYS 19
char option[NUMOPTIONS] = {0,0,0,0,0,0,1,0};
unsigned char keys[NUMGAMEKEYS] =
{
    0xc8,0xd0,0xcb,0xcd,0x2a,0x9d,0x1d,0x39,
    0x1e,0x2c,0xd1,0xc9,0x33,0x34,
    0x9c,0x1c,0xd,0xc,0xf
};
int xdimgame = 320, ydimgame = 200, bppgame = 8, xdim2d = 640, ydim2d = 480;    // JBF 20050318: config.c expects to find these
int forcesetup = 1;

static int digihz[8] = {6000,8000,11025,16000,22050,32000,44100,48000};

static char frame2draw[MAXPLAYERS];
static int frameskipcnt[MAXPLAYERS];

#define LAVASIZ 128
#define LAVALOGSIZ 7
#define LAVAMAXDROPS 32
static char lavabakpic[(LAVASIZ+4)*(LAVASIZ+4)], lavainc[LAVASIZ];
static int lavanumdrops, lavanumframes;
static int lavadropx[LAVAMAXDROPS], lavadropy[LAVAMAXDROPS];
static int lavadropsiz[LAVAMAXDROPS], lavadropsizlookup[LAVAMAXDROPS];
static int lavaradx[24][96], lavarady[24][96], lavaradcnt[32];

//Shared player variables
static vec3_t pos[MAXPLAYERS];
static int horiz[MAXPLAYERS], zoom[MAXPLAYERS], hvel[MAXPLAYERS];
static short ang[MAXPLAYERS], cursectnum[MAXPLAYERS], ocursectnum[MAXPLAYERS];
static short playersprite[MAXPLAYERS], deaths[MAXPLAYERS];
static int lastchaingun[MAXPLAYERS];
static int health[MAXPLAYERS], flytime[MAXPLAYERS];
static short oflags[MAXPLAYERS];
static short numbombs[MAXPLAYERS];
static short numgrabbers[MAXPLAYERS];   // Andy did this
static short nummissiles[MAXPLAYERS];   // Andy did this
static char dimensionmode[MAXPLAYERS];
static char revolvedoorstat[MAXPLAYERS];
static short revolvedoorang[MAXPLAYERS], revolvedoorrotang[MAXPLAYERS];
static int revolvedoorx[MAXPLAYERS], revolvedoory[MAXPLAYERS];

static int nummoves;
// Bug: NUMSTATS used to be equal to the greatest tag number,
// so that the last statrate[] entry was random memory junk
// because stats 0-NUMSTATS required NUMSTATS+1 bytes.   -Andy
#define NUMSTATS 13
static signed char statrate[NUMSTATS] = {-1,0,-1,0,0,0,1,3,0,3,15,-1,-1};

//Input structures
static char networkmode;     //0 is 2(n-1) mode, 1 is n(n-1) mode
static int locselectedgun, locselectedgun2;
static input loc, oloc, loc2;
static input ffsync[MAXPLAYERS], osync[MAXPLAYERS], ssync[MAXPLAYERS];
//Input faketimerhandler -> movethings fifo
static int movefifoplc, movefifoend[MAXPLAYERS];
static input baksync[MOVEFIFOSIZ][MAXPLAYERS];
//Game recording variables
static int reccnt, recstat = 1;
static input recsync[16384][2];

//static int myminlag[MAXPLAYERS], mymaxlag, otherminlag, bufferjitter = 1;
static signed char otherlag[MAXPLAYERS] = {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1};
static int averagelag[MAXPLAYERS] = {512,512,512,512,512,512,512,512,512,512,512,512,512,512,512,512};

static int fakemovefifoplc;
static vec3_t my, omy;
static int myzvel;
static int myhoriz, omyhoriz;
static short myang, omyang, mycursectnum;
static vec3_t mybak[MOVEFIFOSIZ];
static int myhorizbak[MOVEFIFOSIZ];
static short myangbak[MOVEFIFOSIZ];

//GAME.C sync state variables
static char syncstat, syncval[MOVEFIFOSIZ], othersyncval[MOVEFIFOSIZ];
static int syncvaltottail, syncvalhead, othersyncvalhead, syncvaltail;

static char detailmode = 0, ready2send = 0;
static int ototalclock = 0, gotlastpacketclock = 0, smoothratio;
static vec3_t opos[MAXPLAYERS];
static int ohoriz[MAXPLAYERS], ozoom[MAXPLAYERS];
static short oang[MAXPLAYERS];

static vec3_t osprite[MAXSPRITES];

#define MAXINTERPOLATIONS 1024
static int numinterpolations = 0, startofdynamicinterpolations = 0;
static int oldipos[MAXINTERPOLATIONS];
static int bakipos[MAXINTERPOLATIONS];
static int *curipos[MAXINTERPOLATIONS];

// extern int cachecount;

static char playerreadyflag[MAXPLAYERS];

//Miscellaneous variables
static unsigned char packbuf[MAXXDIM];
static char tempbuf[MAXXDIM];
static char boardfilename[BMAX_PATH];
static short tempshort[MAXSECTORS];
static short screenpeek = 0, oldmousebstatus = 0;
short brightness = 0;
static short screensize, screensizeflag = 0;
static short neartagsector, neartagwall, neartagsprite;
static int lockclock, neartagdist, neartaghitdist;
extern int pageoffset, ydim16;
static int globhiz, globloz, globhihit, globlohit;

//Over the shoulder mode variables
static int cameradist = -1, cameraang = 0, cameraclock = 0;

//Board animation variables
#define MAXMIRRORS 64
static short mirrorwall[MAXMIRRORS], mirrorsector[MAXMIRRORS], mirrorcnt;
static short floormirrorsector[64], floormirrorcnt;
static short turnspritelist[16], turnspritecnt;
static short warpsectorlist[64], warpsectorcnt;
static short xpanningsectorlist[16], xpanningsectorcnt;
static short ypanningwalllist[64], ypanningwallcnt;
static short floorpanninglist[64], floorpanningcnt;
static short dragsectorlist[16], dragxdir[16], dragydir[16], dragsectorcnt;
static int dragx1[16], dragy1[16], dragx2[16], dragy2[16], dragfloorz[16];
static short swingcnt, swingwall[32][5], swingsector[32];
static short swingangopen[32], swingangclosed[32], swingangopendir[32];
static short swingang[32], swinganginc[32];
static int swingx[32][8], swingy[32][8];
static short revolvesector[4], revolveang[4], revolvecnt;
static int revolvex[4][16], revolvey[4][16];
static int revolvepivotx[4], revolvepivoty[4];
static short subwaytracksector[4][128], subwaynumsectors[4], subwaytrackcnt;
static int subwaystop[4][8], subwaystopcnt[4];
static int subwaytrackx1[4], subwaytracky1[4];
static int subwaytrackx2[4], subwaytracky2[4];
static int subwayx[4], subwaygoalstop[4], subwayvel[4], subwaypausetime[4];
static short waterfountainwall[MAXPLAYERS], waterfountaincnt[MAXPLAYERS];
static short slimesoundcnt[MAXPLAYERS];

//Variables that let you type messages to other player
static char getmessage[162], getmessageleng;
static int getmessagetimeoff;
static char typemessage[162], typemessageleng = 0, typemode = 0;
#if 0
static char scantoasc[128] =
{
    0,0,'1','2','3','4','5','6','7','8','9','0','-','=',0,0,
    'q','w','e','r','t','y','u','i','o','p','[',']',0,0,'a','s',
    'd','f','g','h','j','k','l',';',39,'`',0,92,'z','x','c','v',
    'b','n','m',',','.','/',0,'*',0,32,0,0,0,0,0,0,
    0,0,0,0,0,0,0,'7','8','9','-','4','5','6','+','1',
    '2','3','0','.',0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
};
static char scantoascwithshift[128] =
{
    0,0,'!','@','#','$','%','^','&','*','(',')','_','+',0,0,
    'Q','W','E','R','T','Y','U','I','O','P','{','}',0,0,'A','S',
    'D','F','G','H','J','K','L',':',34,'~',0,'|','Z','X','C','V',
    'B','N','M','<','>','?',0,'*',0,32,0,0,0,0,0,0,
    0,0,0,0,0,0,0,'7','8','9','-','4','5','6','+','1',
    '2','3','0','.',0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
};
#endif

//These variables are for animating x, y, or z-coordinates of sectors,
//walls, or sprites (They are NOT to be used for changing the [].picnum's)
//See the setanimation(), and getanimategoal() functions for more details.
#define MAXANIMATES 512
static int *animateptr[MAXANIMATES], animategoal[MAXANIMATES];
static int animatevel[MAXANIMATES], animateacc[MAXANIMATES], animatecnt = 0;

#if defined USE_OPENGL
//These parameters are in exact order of sprite structure in BUILD.H
#define spawnsprite(newspriteindex2,x2,y2,z2,cstat2,shade2,pal2,        \
                    clipdist2,xrepeat2,yrepeat2,xoffset2,yoffset2,picnum2,ang2,     \
                    xvel2,yvel2,zvel2,owner2,sectnum2,statnum2,lotag2,hitag2,extra2) \
    {                                                                       \
        spritetype *spr2;                                                   \
        newspriteindex2 = insertsprite(sectnum2,statnum2);                  \
        spr2 = &sprite[newspriteindex2];                                    \
        spr2->x = x2; spr2->y = y2; spr2->z = z2;                           \
        spr2->cstat = cstat2; spr2->shade = shade2;                         \
        spr2->pal = pal2; spr2->clipdist = clipdist2;                       \
        spr2->xrepeat = xrepeat2; spr2->yrepeat = yrepeat2;                 \
        spr2->xoffset = xoffset2; spr2->yoffset = yoffset2;                 \
        spr2->picnum = picnum2; spr2->ang = ang2;                           \
        spr2->xvel = xvel2; spr2->yvel = yvel2; spr2->zvel = zvel2;         \
        spr2->owner = owner2;                                               \
        spr2->lotag = lotag2; spr2->hitag = hitag2; spr2->extra = extra2;   \
        copybuf(&spr2->x,&osprite[newspriteindex2].x,3);                    \
        show2dsprite[newspriteindex2>>3] &= ~(1<<(newspriteindex2&7));      \
        if (show2dsector[sectnum2>>3]&(1<<(sectnum2&7)))                    \
            show2dsprite[newspriteindex2>>3] |= (1<<(newspriteindex2&7));   \
        clearbufbyte(&spriteext[newspriteindex2], sizeof(spriteext_t), 0);  \
    }
#else
#define spawnsprite(newspriteindex2,x2,y2,z2,cstat2,shade2,pal2,        \
                    clipdist2,xrepeat2,yrepeat2,xoffset2,yoffset2,picnum2,ang2,     \
                    xvel2,yvel2,zvel2,owner2,sectnum2,statnum2,lotag2,hitag2,extra2) \
    {                                                                       \
        spritetype *spr2;                                                   \
        newspriteindex2 = insertsprite(sectnum2,statnum2);                  \
        spr2 = &sprite[newspriteindex2];                                    \
        spr2->x = x2; spr2->y = y2; spr2->z = z2;                           \
        spr2->cstat = cstat2; spr2->shade = shade2;                         \
        spr2->pal = pal2; spr2->clipdist = clipdist2;                       \
        spr2->xrepeat = xrepeat2; spr2->yrepeat = yrepeat2;                 \
        spr2->xoffset = xoffset2; spr2->yoffset = yoffset2;                 \
        spr2->picnum = picnum2; spr2->ang = ang2;                           \
        spr2->xvel = xvel2; spr2->yvel = yvel2; spr2->zvel = zvel2;         \
        spr2->owner = owner2;                                               \
        spr2->lotag = lotag2; spr2->hitag = hitag2; spr2->extra = extra2;   \
        copybuf(&spr2->x,&osprite[newspriteindex2].x,3);                    \
        show2dsprite[newspriteindex2>>3] &= ~(1<<(newspriteindex2&7));      \
        if (show2dsector[sectnum2>>3]&(1<<(sectnum2&7)))                    \
            show2dsprite[newspriteindex2>>3] |= (1<<(newspriteindex2&7));   \
    }
#endif

int nextvoxid = 0;

int osdcmd_restartvid(const osdfuncparm_t *parm)
{
    UNREFERENCED_PARAMETER(parm);

    resetvideomode();
    if (setgamemode(fullscreen,xdim,ydim,bpp))
        buildputs("restartvid: Reset failed...\n");

    return OSDCMD_OK;
}

static int osdcmd_vidmode(const osdfuncparm_t *parm)
{
    int newx = xdim, newy = ydim, newbpp = bpp, newfullscreen = fullscreen;

    if (parm->numparms < 1 || parm->numparms > 4) return OSDCMD_SHOWHELP;

    switch (parm->numparms)
    {
    case 1:   // bpp switch
        newbpp = Batol(parm->parms[0]);
        break;
    case 2: // res switch
        newx = Batol(parm->parms[0]);
        newy = Batol(parm->parms[1]);
        break;
    case 3:   // res & bpp switch
    case 4:
        newx = Batol(parm->parms[0]);
        newy = Batol(parm->parms[1]);
        newbpp = Batol(parm->parms[2]);
        if (parm->numparms == 4)
            newfullscreen = (Batol(parm->parms[3]) != 0);
        break;
    }

    if (setgamemode(newfullscreen,newx,newy,newbpp))
        buildputs("vidmode: Mode change failed!\n");
    screensize = xdim+1;
    return OSDCMD_OK;
}

static int osdcmd_map(const osdfuncparm_t *parm)
{
    int i;
    char *dot, namebuf[BMAX_PATH+1];

    if (parm->numparms != 1) return OSDCMD_SHOWHELP;

    strncpy(namebuf, parm->parms[0], BMAX_PATH);
    namebuf[BMAX_PATH] = 0;
    dot = strrchr(namebuf, '.');
    if ((!dot || Bstrcasecmp(dot, ".map")) && strlen(namebuf) <= BMAX_PATH-4)
    {
        strcat(namebuf, ".map");
    }

    prepareboard(namebuf);

    screenpeek = myconnectindex;
    reccnt = 0;
    for (i=connecthead; i>=0; i=connectpoint2[i]) initplayersprite((short)i);

    waitforeverybody();
    totalclock = ototalclock = 0; gotlastpacketclock = 0; nummoves = 0;

    ready2send = 1;
    drawscreen(screenpeek,65536L);

    return OSDCMD_OK;
}

static void Ken_UninitAll(void)
{
    sendlogoff();         //Signing off
    musicoff();
    uninitmultiplayers();
    uninittimer();
    uninitinput();
    uninitengine();
    uninitsb();
    uninitgroupfile();
}

static void Ken_FatalEngineError(void)
{
    buildprintf("There was a problem initialising the engine: %s.\n", engineerrstr);
}

int32_t app_main(int32_t argc, char const * const * argv)
{
#if defined STARTUP_SETUP_WINDOW
    int cmdsetup = 0;
#endif
    int i, j, k /*, l, fil, waitplayers, x1, y1, x2, y2*/;
    int /*other, packleng,*/ netparm;

    OSD_SetFunctions(
        NULL, NULL, NULL, NULL, NULL,
        COMMON_clearbackground,
        BGetTime,
        NULL
        );

    OSD_SetParameters(0,2, 0,0, 4,0, 0, 0, 0); // TODO: Add error and red palookup IDs.

    OSD_SetLogFile("testgame.log");
    initprintf("%s %s\n", AppProperName, s_buildRev);
    PrintBuildInfo();

#ifdef USE_OPENGL
    OSD_RegisterFunction("restartvid","restartvid: reinitialise the video mode",osdcmd_restartvid);
    OSD_RegisterFunction("vidmode","vidmode [xdim ydim] [bpp] [fullscreen]: immediately change the video mode",osdcmd_vidmode);
    OSD_RegisterFunction("map", "map [filename]: load a map", osdcmd_map);
#endif

    wm_setapptitle(AppProperName);

    Bstrcpy(boardfilename, "nukeland.map");
    j = 0; netparm = argc;
    for (i=1; i<argc; i++)
    {
        if ((!Bstrcasecmp("-net",argv[i])) || (!Bstrcasecmp("/net",argv[i]))) { j = 1; netparm = i; continue; }
        if (j)
        {
            if (argv[i][0] == '-' || argv[i][0] == '/')
            {
                if (((argv[i][1] == 'n') || (argv[i][1] == 'N')) && (argv[i][2] == '0')) { networkmode = 0; continue; }
                if (((argv[i][1] == 'n') || (argv[i][1] == 'N')) && (argv[i][2] == '1')) { networkmode = 1; continue; }
            }
            if (isvalidipaddress(argv[i])) continue;
        }
        else
        {
            if (!Bstrcasecmp(argv[i], "-setup"))
            {
#if defined STARTUP_SETUP_WINDOW
                cmdsetup = 1;
#endif
            }
            else
            {
                Bstrcpy(boardfilename, argv[i]);
                if (!Bstrrchr(boardfilename,'.')) Bstrcat(boardfilename,".map");
            }
        }
    }

    if ((i = Ken_loadsetup(setupfilename)) < 0)
        buildputs("Configuration file not found, using defaults.\n");

#if defined STARTUP_SETUP_WINDOW
    if (i || forcesetup || cmdsetup)
    {
        if (quitevent || !startwin_run()) return -1;
    }
#endif
    Ken_writesetup(setupfilename);

    initgroupfile(G_GrpFile());
    if (initengine())
    {
        Ken_FatalEngineError();
        return -1;
    }

    Ken_InitMultiPsky();

    initinput();
    if (option[3] != 0) initmouse();
    inittimer(TIMERINTSPERSECOND);

    //initmultiplayers(argc-netparm,&argv[netparm],option[4],option[5],0);
    if (initmultiplayersparms(argc-netparm,&argv[netparm]))
    {
        buildputs("Waiting for players...\n");
        while (initmultiplayerscycle())
        {
            handleevents();
            if (quitevent)
            {
                Ken_UninitAll();
                return 0;
            }
        }
    }
    option[4] = (numplayers >= 2);

    loadpics("tiles000.art",1048576);                      //Load artwork
    if (!qloadkvx(nextvoxid,"voxel000.kvx"))
        tiletovox[PLAYER] = nextvoxid++;
    if (!qloadkvx(nextvoxid,"voxel001.kvx"))
        tiletovox[BROWNMONSTER] = nextvoxid++;
    if (!loaddefinitionsfile(G_DefFile())) buildputs("Definitions file loaded.\n");

    if (E_PostInit())
    {
        Ken_UninitAll();
        Ken_FatalEngineError();
        return -1;
    }

    //Here's an example of TRUE ornamented walls
    //The allocatepermanenttile should be called right after loadpics
    //Since it resets the tile cache for each call.
    if (allocatepermanenttile(SLIME,128,128) == 0)    //If enough memory
    {
        buildputs("Not enough memory for slime!\n");
        exit(0);
    }
    if (allocatepermanenttile(MAXTILES-1,64,64) != 0)    //If enough memory
    {
        //My face with an explosion written over it
        copytilepiece(KENPICTURE,0,0,64,64,MAXTILES-1,0,0);
        copytilepiece(EXPLOSION,0,0,64,64,MAXTILES-1,0,0);
    }

    initlava();

    for (j=0; j<256; j++)
        tempbuf[j] = ((j+32)&255);  //remap colors for screwy palette sectors
    makepalookup(16,tempbuf,0,0,0,1);

    for (j=0; j<256; j++) tempbuf[j] = j;
    makepalookup(17,tempbuf,96,96,96,1);

    for (j=0; j<256; j++) tempbuf[j] = j; //(j&31)+32;
    makepalookup(18,tempbuf,32,32,192,1);

    fillemptylookups();

    prepareboard(boardfilename);                   //Load board

    initsb(option[1],option[2],digihz[option[7]>>4],((option[7]&4)>0)+1,((option[7]&2)>0)+1,60,option[7]&1);
    //if (Bstrcmp(boardfilename,"klab.map") == 0)
    //   loadsong("klabsong.kdm");
    //else
    loadsong("neatsong.kdm");
    musicon();

#if 0
    if (option[4] > 0)
    {
        x1 = ((xdim-screensize)>>1);
        x2 = x1+screensize-1;
        y1 = (((ydim-32)-scale(screensize,ydim-32,xdim))>>1);
        y2 = y1 + scale(screensize,ydim-32,xdim)-1;

        drawtilebackground(/*0L,0L,*/ BACKGROUND,8,x1,y1,x2,y2,0);

        sendlogon();

        if (option[4] < 5) waitplayers = 2; else waitplayers = option[4]-3;
        while (numplayers < waitplayers)
        {
            sprintf(tempbuf,"%ld of %ld players in...",numplayers,waitplayers);
            printext256(68L,84L,31,0,tempbuf,0);
            nextpage();

            if (getpacket(&other,packbuf) > 0)
                if (packbuf[0] == 255)
                    keystatus[1] = 1;

            if (handleevents())
            {
                if (quitevent)
                {
                    keystatus[1] = 1;
                    quitevent = 0;
                }
            }

            if (keystatus[1])
            {
                Ken_UninitAll();
                return 0;
            }
        }
        screenpeek = myconnectindex;

        if (numplayers <= 3)
            networkmode = 1;
        else
            networkmode = 0;

        j = 1;
        for (i=connecthead; i>=0; i=connectpoint2[i])
        {
            if (myconnectindex == i) break;
            j++;
        }
        sprintf(getmessage,"Player %ld",j);
        if (networkmode == 0)
        {
            if (j == 1) Bstrcat(getmessage," (Master)");
            else Bstrcat(getmessage," (Slave)");
        }
        else
            Bstrcat(getmessage," (Even)");
        getmessageleng = Bstrlen(getmessage);
        getmessagetimeoff = totalclock+120;
    }
#endif
    screenpeek = myconnectindex;
    reccnt = 0;
    for (i=connecthead; i>=0; i=connectpoint2[i]) initplayersprite((short)i);

    waitforeverybody();
    totalclock = ototalclock = 0; gotlastpacketclock = 0; nummoves = 0;

    ready2send = 1;
    drawscreen(screenpeek,65536L);

    while (!keystatus[1])       //Main loop starts here
    {
        if (handleevents())
        {
            if (quitevent)
            {
                keystatus[1] = 1;
                quitevent = 0;
            }
        }

        refreshaudio();
        OSD_DispatchQueued();

        // backslash (useful only with KDM)
//      if (keystatus[0x2b]) { keystatus[0x2b] = 0; preparesndbuf(); }

        if ((networkmode == 1) || (myconnectindex != connecthead))
            while (fakemovefifoplc != movefifoend[myconnectindex]) fakedomovethings();

        getpackets();

        if (typemode == 0)           //if normal game keys active
        {
            if ((keystatus[0x2a]&keystatus[0x36]&keystatus[0x13]) > 0)   //Sh.Sh.R (replay)
            {
                keystatus[0x13] = 0;
                playback();
            }

            if (keystatus[0x26]&(keystatus[0x1d]|keystatus[0x9d])) //Load game
            {
                keystatus[0x26] = 0;
                loadgame();
                drawstatusbar(screenpeek);   // Andy did this
            }

            if (keystatus[0x1f]&(keystatus[0x1d]|keystatus[0x9d])) //Save game
            {
                keystatus[0x1f] = 0;
                savegame();
            }
        }

        if ((networkmode == 0) || (option[4] == 0))
        {
            while (movefifoplc != movefifoend[0]) domovethings();
        }
        else
        {
            j = connecthead;
            if (j == myconnectindex) j = connectpoint2[j];
            averagelag[j] = ((averagelag[j]*7+(((movefifoend[myconnectindex]-movefifoend[j]+otherlag[j]+2)&255)<<8))>>3);
            j = max(averagelag[j]>>9,1);
            while (((movefifoend[myconnectindex]-movefifoplc)&(MOVEFIFOSIZ-1)) > j)
            {
                for (i=connecthead; i>=0; i=connectpoint2[i])
                    if (movefifoplc == movefifoend[i]) break;
                if (i >= 0) break;
                if (myconnectindex != connecthead)
                {
                    k = ((movefifoend[myconnectindex]-movefifoend[connecthead]-otherlag[connecthead]+128)&255);
                    if (k > 128+1) ototalclock++;
                    if (k < 128-1) ototalclock--;
                }
                domovethings();
            }
        }
        i = (totalclock-gotlastpacketclock)*(65536/(TIMERINTSPERSECOND/MOVESPERSECOND));

        drawscreen(screenpeek,i);
    }

    Ken_UninitAll();

    return 0;
}

void operatesector(short dasector)
{
    //Door code
    int i, j, /*k, s, nexti, good, cnt,*/ datag;
    int /*dax, day,*/ daz, dax2, day2, /*daz2,*/ centx, centy;
    short startwall, endwall, wallfind[2];

    datag = sector[dasector].lotag;

    startwall = sector[dasector].wallptr;
    endwall = startwall + sector[dasector].wallnum;
    centx = 0L, centy = 0L;
    for (i=startwall; i<endwall; i++)
    {
        centx += wall[i].x;
        centy += wall[i].y;
    }
    centx /= (endwall-startwall);
    centy /= (endwall-startwall);

    //Simple door that moves up  (tag 8 is a combination of tags 6 & 7)
    if ((datag == 6) || (datag == 8))    //If the sector in front is a door
    {
        i = getanimationgoal(&sector[dasector].ceilingz);
        if (i >= 0)      //If door already moving, reverse its direction
        {
            if (datag == 8)
                daz = ((sector[dasector].ceilingz+sector[dasector].floorz)>>1);
            else
                daz = sector[dasector].floorz;

            if (animategoal[i] == daz)
                animategoal[i] = sector[nextsectorneighborz(dasector,sector[dasector].floorz,-1,-1)].ceilingz;
            else
                animategoal[i] = daz;
            animatevel[i] = 0;
        }
        else      //else insert the door's ceiling on the animation list
        {
            if (sector[dasector].ceilingz == sector[dasector].floorz)
                daz = sector[nextsectorneighborz(dasector,sector[dasector].floorz,-1,-1)].ceilingz;
            else
            {
                if (datag == 8)
                    daz = ((sector[dasector].ceilingz+sector[dasector].floorz)>>1);
                else
                    daz = sector[dasector].floorz;
            }
            if ((j = setanimation(&sector[dasector].ceilingz,daz,6L,6L)) >= 0)
                wsayfollow("updowndr.wav",4096L+(krand()&255)-128,256L,&centx,&centy,0);
        }
    }
    //Simple door that moves down
    if ((datag == 7) || (datag == 8)) //If the sector in front's elevator
    {
        i = getanimationgoal(&sector[dasector].floorz);
        if (i >= 0)      //If elevator already moving, reverse its direction
        {
            if (datag == 8)
                daz = ((sector[dasector].ceilingz+sector[dasector].floorz)>>1);
            else
                daz = sector[dasector].ceilingz;

            if (animategoal[i] == daz)
                animategoal[i] = sector[nextsectorneighborz(dasector,sector[dasector].ceilingz,1,1)].floorz;
            else
                animategoal[i] = daz;
            animatevel[i] = 0;
        }
        else      //else insert the elevator's ceiling on the animation list
        {
            if (sector[dasector].floorz == sector[dasector].ceilingz)
                daz = sector[nextsectorneighborz(dasector,sector[dasector].ceilingz,1,1)].floorz;
            else
            {
                if (datag == 8)
                    daz = ((sector[dasector].ceilingz+sector[dasector].floorz)>>1);
                else
                    daz = sector[dasector].ceilingz;
            }
            if ((j = setanimation(&sector[dasector].floorz,daz,6L,6L)) >= 0)
                wsayfollow("updowndr.wav",4096L+(krand()&255)-128,256L,&centx,&centy,0);
        }
    }

    if (datag == 9)   //Smooshy-wall sideways double-door
    {
        //find any points with either same x or same y coordinate
        //  as center (centx, centy) - should be 2 points found.
        wallfind[0] = -1;
        wallfind[1] = -1;
        for (i=startwall; i<endwall; i++)
            if ((wall[i].x == centx) || (wall[i].y == centy))
            {
                if (wallfind[0] == -1)
                    wallfind[0] = i;
                else
                    wallfind[1] = i;
            }

        for (j=0; j<2; j++)
        {
            if ((wall[wallfind[j]].x == centx) && (wall[wallfind[j]].y == centy))
            {
                //find what direction door should open by averaging the
                //  2 neighboring points of wallfind[0] & wallfind[1].
                i = wallfind[j]-1; if (i < startwall) i = endwall-1;
                dax2 = ((wall[i].x+wall[wall[wallfind[j]].point2].x)>>1)-wall[wallfind[j]].x;
                day2 = ((wall[i].y+wall[wall[wallfind[j]].point2].y)>>1)-wall[wallfind[j]].y;
                if (dax2 != 0)
                {
                    dax2 = wall[wall[wall[wallfind[j]].point2].point2].x;
                    dax2 -= wall[wall[wallfind[j]].point2].x;
                    setanimation(&wall[wallfind[j]].x,wall[wallfind[j]].x+dax2,4L,0L);
                    setanimation(&wall[i].x,wall[i].x+dax2,4L,0L);
                    setanimation(&wall[wall[wallfind[j]].point2].x,wall[wall[wallfind[j]].point2].x+dax2,4L,0L);
                }
                else if (day2 != 0)
                {
                    day2 = wall[wall[wall[wallfind[j]].point2].point2].y;
                    day2 -= wall[wall[wallfind[j]].point2].y;
                    setanimation(&wall[wallfind[j]].y,wall[wallfind[j]].y+day2,4L,0L);
                    setanimation(&wall[i].y,wall[i].y+day2,4L,0L);
                    setanimation(&wall[wall[wallfind[j]].point2].y,wall[wall[wallfind[j]].point2].y+day2,4L,0L);
                }
            }
            else
            {
                i = wallfind[j]-1; if (i < startwall) i = endwall-1;
                dax2 = ((wall[i].x+wall[wall[wallfind[j]].point2].x)>>1)-wall[wallfind[j]].x;
                day2 = ((wall[i].y+wall[wall[wallfind[j]].point2].y)>>1)-wall[wallfind[j]].y;
                if (dax2 != 0)
                {
                    setanimation(&wall[wallfind[j]].x,centx,4L,0L);
                    setanimation(&wall[i].x,centx+dax2,4L,0L);
                    setanimation(&wall[wall[wallfind[j]].point2].x,centx+dax2,4L,0L);
                }
                else if (day2 != 0)
                {
                    setanimation(&wall[wallfind[j]].y,centy,4L,0L);
                    setanimation(&wall[i].y,centy+day2,4L,0L);
                    setanimation(&wall[wall[wallfind[j]].point2].y,centy+day2,4L,0L);
                }
            }
        }
        wsayfollow("updowndr.wav",4096L-256L,256L,&centx,&centy,0);
        wsayfollow("updowndr.wav",4096L+256L,256L,&centx,&centy,0);
    }

    if (datag == 13)  //Swinging door
    {
        for (i=0; i<swingcnt; i++)
        {
            if (swingsector[i] == dasector)
            {
                if (swinganginc[i] == 0)
                {
                    if (swingang[i] == swingangclosed[i])
                    {
                        swinganginc[i] = swingangopendir[i];
                        wsayfollow("opendoor.wav",4096L+(krand()&511)-256,256L,&centx,&centy,0);
                    }
                    else
                        swinganginc[i] = -swingangopendir[i];
                }
                else
                    swinganginc[i] = -swinganginc[i];

                for (j=1; j<=3; j++)
                {
                    setinterpolation(&wall[swingwall[i][j]].x);
                    setinterpolation(&wall[swingwall[i][j]].y);
                }
            }
        }
    }

    if (datag == 16)  //True sideways double-sliding door
    {
        //get 2 closest line segments to center (dax, day)
        wallfind[0] = -1;
        wallfind[1] = -1;
        for (i=startwall; i<endwall; i++)
            if (wall[i].lotag == 6)
            {
                if (wallfind[0] == -1)
                    wallfind[0] = i;
                else
                    wallfind[1] = i;
            }

        for (j=0; j<2; j++)
        {
            if ((((wall[wallfind[j]].x+wall[wall[wallfind[j]].point2].x)>>1) == centx) && (((wall[wallfind[j]].y+wall[wall[wallfind[j]].point2].y)>>1) == centy))
            {
                //door was closed
                //find what direction door should open
                i = wallfind[j]-1; if (i < startwall) i = endwall-1;
                dax2 = wall[i].x-wall[wallfind[j]].x;
                day2 = wall[i].y-wall[wallfind[j]].y;
                if (dax2 != 0)
                {
                    dax2 = wall[wall[wall[wall[wallfind[j]].point2].point2].point2].x;
                    dax2 -= wall[wall[wall[wallfind[j]].point2].point2].x;
                    setanimation(&wall[wallfind[j]].x,wall[wallfind[j]].x+dax2,4L,0L);
                    setanimation(&wall[i].x,wall[i].x+dax2,4L,0L);
                    setanimation(&wall[wall[wallfind[j]].point2].x,wall[wall[wallfind[j]].point2].x+dax2,4L,0L);
                    setanimation(&wall[wall[wall[wallfind[j]].point2].point2].x,wall[wall[wall[wallfind[j]].point2].point2].x+dax2,4L,0L);
                }
                else if (day2 != 0)
                {
                    day2 = wall[wall[wall[wall[wallfind[j]].point2].point2].point2].y;
                    day2 -= wall[wall[wall[wallfind[j]].point2].point2].y;
                    setanimation(&wall[wallfind[j]].y,wall[wallfind[j]].y+day2,4L,0L);
                    setanimation(&wall[i].y,wall[i].y+day2,4L,0L);
                    setanimation(&wall[wall[wallfind[j]].point2].y,wall[wall[wallfind[j]].point2].y+day2,4L,0L);
                    setanimation(&wall[wall[wall[wallfind[j]].point2].point2].y,wall[wall[wall[wallfind[j]].point2].point2].y+day2,4L,0L);
                }
            }
            else
            {
                //door was not closed
                i = wallfind[j]-1; if (i < startwall) i = endwall-1;
                dax2 = wall[i].x-wall[wallfind[j]].x;
                day2 = wall[i].y-wall[wallfind[j]].y;
                if (dax2 != 0)
                {
                    setanimation(&wall[wallfind[j]].x,centx,4L,0L);
                    setanimation(&wall[i].x,centx+dax2,4L,0L);
                    setanimation(&wall[wall[wallfind[j]].point2].x,centx,4L,0L);
                    setanimation(&wall[wall[wall[wallfind[j]].point2].point2].x,centx+dax2,4L,0L);
                }
                else if (day2 != 0)
                {
                    setanimation(&wall[wallfind[j]].y,centy,4L,0L);
                    setanimation(&wall[i].y,centy+day2,4L,0L);
                    setanimation(&wall[wall[wallfind[j]].point2].y,centy,4L,0L);
                    setanimation(&wall[wall[wall[wallfind[j]].point2].point2].y,centy+day2,4L,0L);
                }
            }
        }
        wsayfollow("updowndr.wav",4096L-64L,256L,&centx,&centy,0);
        wsayfollow("updowndr.wav",4096L+64L,256L,&centx,&centy,0);
    }
}

void operatesprite(short dasprite)
{
    int datag;

    datag = sprite[dasprite].lotag;

    if (datag == 2)    //A sprite that shoots a bomb
    {
        vec3_t vector = { sprite[dasprite].x,sprite[dasprite].y,sprite[dasprite].z };
        shootgun(dasprite, &vector,
                 sprite[dasprite].ang,100L,sprite[dasprite].sectnum,2);
    }
}

int changehealth(short snum, short deltahealth)
{
    // int dax, day;
    // short good, k, startwall, endwall, s;

    if (health[snum] > 0)
    {
        health[snum] += deltahealth;
        if (health[snum] > 999) health[snum] = 999;

        if (health[snum] <= 0)
        {
            health[snum] = -1;
            wsayfollow("death.wav",4096L+(krand()&127)-64,256L,&pos[snum].x,&pos[snum].y,1);
            sprite[playersprite[snum]].picnum = SKELETON;
        }

        if ((snum == screenpeek) && (screensize <= xdim))
        {
            if (health[snum] > 0)
                sprintf((char *)tempbuf,"Health:%3d",health[snum]);
            else
                sprintf((char *)tempbuf,"YOU STINK!");

            printext((xdim>>1)-(Bstrlen((char *)tempbuf)<<2),ydim-24,(char *)tempbuf,ALPHABET /*,80*/);
        }
    }
    return health[snum] <= 0;       //You were just injured
}

void changenumbombs(short snum, short deltanumbombs)     // Andy did this
{
    numbombs[snum] += deltanumbombs;
    if (numbombs[snum] > 999) numbombs[snum] = 999;
    if (numbombs[snum] <= 0)
    {
        wsayfollow("doh.wav",4096L+(krand()&127)-64,256L,&pos[snum].x,&pos[snum].y,1);
        numbombs[snum] = 0;
    }

    if ((snum == screenpeek) && (screensize <= xdim))
    {
        sprintf((char *)tempbuf,"B:%3d",numbombs[snum]);
        printext(8L,(ydim - 28L),(char *)tempbuf,ALPHABET /*,80*/);
    }
}

void changenummissiles(short snum, short deltanummissiles)     // Andy did this
{
    nummissiles[snum] += deltanummissiles;
    if (nummissiles[snum] > 999) nummissiles[snum] = 999;
    if (nummissiles[snum] <= 0)
    {
        wsayfollow("doh.wav",4096L+(krand()&127)-64,256L,&pos[snum].x,&pos[snum].y,1);
        nummissiles[snum] = 0;
    }

    if ((snum == screenpeek) && (screensize <= xdim))
    {
        sprintf((char *)tempbuf,"M:%3d",nummissiles[snum]);
        printext(8L,(ydim - 20L),(char *)tempbuf,ALPHABET /*,80*/);
    }
}

void changenumgrabbers(short snum, short deltanumgrabbers)     // Andy did this
{
    numgrabbers[snum] += deltanumgrabbers;
    if (numgrabbers[snum] > 999) numgrabbers[snum] = 999;
    if (numgrabbers[snum] <= 0)
    {
        wsayfollow("doh.wav",4096L+(krand()&127)-64,256L,&pos[snum].x,&pos[snum].y,1);
        numgrabbers[snum] = 0;
    }

    if ((snum == screenpeek) && (screensize <= xdim))
    {
        sprintf((char *)tempbuf,"G:%3d",numgrabbers[snum]);
        printext(8L,(ydim - 12L),(char *)tempbuf,ALPHABET /*,80*/);
    }
}

static int ostatusflytime = 0x80000000;
void drawstatusflytime(short snum)     // Andy did this
{
    int nstatusflytime;

    if ((snum == screenpeek) && (screensize <= xdim))
    {
        nstatusflytime = (((flytime[snum] + 119) - lockclock) / 120);
        if (nstatusflytime > 1000) nstatusflytime = 1000;
        else if (nstatusflytime < 0) nstatusflytime = 0;
        if (nstatusflytime != ostatusflytime)
        {
            if (nstatusflytime > 999) sprintf((char *)tempbuf,"FT:BIG");
            else sprintf((char *)tempbuf,"FT:%3d",nstatusflytime);
            printext((xdim - 56L),(ydim - 20L),(char *)tempbuf,ALPHABET /*,80*/);
            ostatusflytime = nstatusflytime;
        }
    }
}

void drawstatusbar(short snum)     // Andy did this
{
    int nstatusflytime;

    if ((snum == screenpeek) && (screensize <= xdim))
    {
        sprintf((char *)tempbuf,"Deaths:%d",deaths[snum]);
        printext((xdim>>1)-(strlen((char *)tempbuf)<<2),ydim-16,(char *)tempbuf,ALPHABET /*,80*/);
        sprintf((char *)tempbuf,"Health:%3d",health[snum]);
        printext((xdim>>1)-(strlen((char *)tempbuf)<<2),ydim-24,(char *)tempbuf,ALPHABET /*,80*/);

        sprintf((char *)tempbuf,"B:%3d",numbombs[snum]);
        printext(8L,(ydim - 28L),(char *)tempbuf,ALPHABET /*,80*/);
        sprintf((char *)tempbuf,"M:%3d",nummissiles[snum]);
        printext(8L,(ydim - 20L),(char *)tempbuf,ALPHABET /*,80*/);
        sprintf((char *)tempbuf,"G:%3d",numgrabbers[snum]);
        printext(8L,(ydim - 12L),(char *)tempbuf,ALPHABET /*,80*/);

        nstatusflytime = (((flytime[snum] + 119) - lockclock) / 120);
        if (nstatusflytime < 0)
        {
            sprintf((char *)tempbuf,"FT:  0");
            ostatusflytime = 0;
        }
        else if (nstatusflytime > 999)
        {
            sprintf((char *)tempbuf,"FT:BIG");
            ostatusflytime = 999;
        }
        else
        {
            sprintf((char *)tempbuf,"FT:%3d",nstatusflytime);
            ostatusflytime = nstatusflytime;
        }
        printext((xdim - 56L),(ydim - 20L),(char *)tempbuf,ALPHABET /*,80*/);
    }
}

void prepareboard(char *daboardfilename)
{
    short startwall, endwall, dasector;
    int i, j, k=0, s, dax, day, /*daz,*/ dax2, day2;

    getmessageleng = 0;
    typemessageleng = 0;

    randomseed = 17L;

    //Clear (do)animation's list
    animatecnt = 0;
    typemode = 0;
    locselectedgun = 0;
    locselectedgun2 = 0;

    if (loadboard(daboardfilename,0,&pos[0],&ang[0],&cursectnum[0]) == -1)
    {
        musicoff();
        uninitmultiplayers();
        uninittimer();
        uninitinput();
        uninitengine();
        uninitsb();
        uninitgroupfile();
        printf("Board not found\n");
        exit(0);
    }
    else
    {
        char tempfn[BMAX_PATH + 1], *fp;

        strncpy(tempfn, daboardfilename, BMAX_PATH);
        tempfn[BMAX_PATH] = 0;

        fp = strrchr(tempfn,'.');
        if (fp) *fp = 0;

        if (strlen(tempfn) <= BMAX_PATH-4)
        {
            strcat(tempfn,".mhk");
            loadmaphack(tempfn);
        }
    }

    setup3dscreen();

    for (i=0; i<MAXPLAYERS; i++)
    {
        pos[i] = pos[0];
        ang[i] = ang[0];
        cursectnum[i] = cursectnum[0];
        ocursectnum[i] = cursectnum[0];
        horiz[i] = 100;
        lastchaingun[i] = 0;
        health[i] = 100;
        dimensionmode[i] = 3;
        numbombs[i] = 0;
        numgrabbers[i] = 0;
        nummissiles[i] = 0;
        flytime[i] = 0L;
        zoom[i] = 768L;
        deaths[i] = 0L;
        playersprite[i] = -1;
        screensize = xdim;

        opos[i] = pos[0];
        ohoriz[i] = horiz[0];
        ozoom[i] = zoom[0];
        oang[i] = ang[0];
    }

    my = omy = pos[myconnectindex];
    myhoriz = omyhoriz = horiz[myconnectindex];
    myang = omyang = ang[myconnectindex];
    mycursectnum = cursectnum[myconnectindex];
    myzvel = 0;

    movefifoplc = fakemovefifoplc = 0;
    syncvalhead = 0L; othersyncvalhead = 0L;
    syncvaltottail = 0L; syncvaltail = 0L;
    numinterpolations = 0;

    clearbufbyte(&oloc,sizeof(input),0L);
    for (i=0; i<MAXPLAYERS; i++)
    {
        movefifoend[i] = 0;
        clearbufbyte(&ffsync[i],sizeof(input),0L);
        clearbufbyte(&ssync[i],sizeof(input),0L);
        clearbufbyte(&osync[i],sizeof(input),0L);
    }

    //Scan sector tags

    for (i=0; i<MAXPLAYERS; i++)
    {
        waterfountainwall[i] = -1;
        waterfountaincnt[i] = 0;
        slimesoundcnt[i] = 0;
    }
    warpsectorcnt = 0;      //Make a list of warping sectors
    xpanningsectorcnt = 0;  //Make a list of wall x-panning sectors
    floorpanningcnt = 0;    //Make a list of slime sectors
    dragsectorcnt = 0;      //Make a list of moving platforms
    swingcnt = 0;           //Make a list of swinging doors
    revolvecnt = 0;         //Make a list of revolving doors
    subwaytrackcnt = 0;     //Make a list of subways

    floormirrorcnt = 0;
    tilesiz[FLOORMIRROR].x = 0;
    tilesiz[FLOORMIRROR].y = 0;

    for (i=0; i<numsectors; i++)
    {
        switch (sector[i].lotag)
        {
        case 4:
            floorpanninglist[floorpanningcnt++] = i;
            break;
        case 10:
            warpsectorlist[warpsectorcnt++] = i;
            break;
        case 11:
            xpanningsectorlist[xpanningsectorcnt++] = i;
            break;
        case 12:
            dasector = i;
            dax = 0x7fffffff;
            day = 0x7fffffff;
            dax2 = 0x80000000;
            day2 = 0x80000000;
            startwall = sector[i].wallptr;
            endwall = startwall+sector[i].wallnum;
            for (j=startwall; j<endwall; j++)
            {
                if (wall[j].x < dax) dax = wall[j].x;
                if (wall[j].y < day) day = wall[j].y;
                if (wall[j].x > dax2) dax2 = wall[j].x;
                if (wall[j].y > day2) day2 = wall[j].y;
                if (wall[j].lotag == 3) k = j;
            }
            if (wall[k].x == dax) dragxdir[dragsectorcnt] = -16;
            if (wall[k].y == day) dragydir[dragsectorcnt] = -16;
            if (wall[k].x == dax2) dragxdir[dragsectorcnt] = 16;
            if (wall[k].y == day2) dragydir[dragsectorcnt] = 16;

            dasector = wall[startwall].nextsector;
            dragx1[dragsectorcnt] = 0x7fffffff;
            dragy1[dragsectorcnt] = 0x7fffffff;
            dragx2[dragsectorcnt] = 0x80000000;
            dragy2[dragsectorcnt] = 0x80000000;
            startwall = sector[dasector].wallptr;
            endwall = startwall+sector[dasector].wallnum;
            for (j=startwall; j<endwall; j++)
            {
                if (wall[j].x < dragx1[dragsectorcnt]) dragx1[dragsectorcnt] = wall[j].x;
                if (wall[j].y < dragy1[dragsectorcnt]) dragy1[dragsectorcnt] = wall[j].y;
                if (wall[j].x > dragx2[dragsectorcnt]) dragx2[dragsectorcnt] = wall[j].x;
                if (wall[j].y > dragy2[dragsectorcnt]) dragy2[dragsectorcnt] = wall[j].y;

                setinterpolation(&sector[dasector].floorz);
                setinterpolation(&wall[j].x);
                setinterpolation(&wall[j].y);
                setinterpolation(&wall[wall[j].nextwall].x);
                setinterpolation(&wall[wall[j].nextwall].y);
            }

            dragx1[dragsectorcnt] += (wall[sector[i].wallptr].x-dax);
            dragy1[dragsectorcnt] += (wall[sector[i].wallptr].y-day);
            dragx2[dragsectorcnt] -= (dax2-wall[sector[i].wallptr].x);
            dragy2[dragsectorcnt] -= (day2-wall[sector[i].wallptr].y);

            dragfloorz[dragsectorcnt] = sector[i].floorz;

            dragsectorlist[dragsectorcnt++] = i;
            break;
        case 13:
            startwall = sector[i].wallptr;
            endwall = startwall+sector[i].wallnum;
            for (j=startwall; j<endwall; j++)
            {
                if (wall[j].lotag == 4)
                {
                    k = wall[wall[wall[wall[j].point2].point2].point2].point2;
                    if ((wall[j].x == wall[k].x) && (wall[j].y == wall[k].y))
                    {
                        //Door opens counterclockwise
                        swingwall[swingcnt][0] = j;
                        swingwall[swingcnt][1] = wall[j].point2;
                        swingwall[swingcnt][2] = wall[wall[j].point2].point2;
                        swingwall[swingcnt][3] = wall[wall[wall[j].point2].point2].point2;
                        swingangopen[swingcnt] = 1536;
                        swingangclosed[swingcnt] = 0;
                        swingangopendir[swingcnt] = -1;
                    }
                    else
                    {
                        //Door opens clockwise
                        swingwall[swingcnt][0] = wall[j].point2;
                        swingwall[swingcnt][1] = j;
                        swingwall[swingcnt][2] = lastwall(j);
                        swingwall[swingcnt][3] = lastwall(swingwall[swingcnt][2]);
                        swingwall[swingcnt][4] = lastwall(swingwall[swingcnt][3]);
                        swingangopen[swingcnt] = 512;
                        swingangclosed[swingcnt] = 0;
                        swingangopendir[swingcnt] = 1;
                    }
                    for (k=0; k<4; k++)
                    {
                        swingx[swingcnt][k] = wall[swingwall[swingcnt][k]].x;
                        swingy[swingcnt][k] = wall[swingwall[swingcnt][k]].y;
                    }

                    swingsector[swingcnt] = i;
                    swingang[swingcnt] = swingangclosed[swingcnt];
                    swinganginc[swingcnt] = 0;
                    swingcnt++;
                }
            }
            break;
        case 14:
            startwall = sector[i].wallptr;
            endwall = startwall+sector[i].wallnum;
            dax = 0L;
            day = 0L;
            for (j=startwall; j<endwall; j++)
            {
                dax += wall[j].x;
                day += wall[j].y;
            }
            revolvepivotx[revolvecnt] = dax / (endwall-startwall);
            revolvepivoty[revolvecnt] = day / (endwall-startwall);

            k = 0;
            for (j=startwall; j<endwall; j++)
            {
                revolvex[revolvecnt][k] = wall[j].x;
                revolvey[revolvecnt][k] = wall[j].y;

                setinterpolation(&wall[j].x);
                setinterpolation(&wall[j].y);
                setinterpolation(&wall[wall[j].nextwall].x);
                setinterpolation(&wall[wall[j].nextwall].y);

                k++;
            }
            revolvesector[revolvecnt] = i;
            revolveang[revolvecnt] = 0;

            revolvecnt++;
            break;
        case 15:
            subwaytracksector[subwaytrackcnt][0] = i;

            subwaystopcnt[subwaytrackcnt] = 0;
            dax = 0x7fffffff;
            day = 0x7fffffff;
            dax2 = 0x80000000;
            day2 = 0x80000000;
            startwall = sector[i].wallptr;
            endwall = startwall+sector[i].wallnum;
            for (j=startwall; j<endwall; j++)
            {
                if (wall[j].x < dax) dax = wall[j].x;
                if (wall[j].y < day) day = wall[j].y;
                if (wall[j].x > dax2) dax2 = wall[j].x;
                if (wall[j].y > day2) day2 = wall[j].y;
            }
            for (j=startwall; j<endwall; j++)
            {
                if (wall[j].lotag == 5)
                {
                    if ((wall[j].x > dax) && (wall[j].y > day) && (wall[j].x < dax2) && (wall[j].y < day2))
                    {
                        subwayx[subwaytrackcnt] = wall[j].x;
                    }
                    else
                    {
                        subwaystop[subwaytrackcnt][subwaystopcnt[subwaytrackcnt]] = wall[j].x;
                        subwaystopcnt[subwaytrackcnt]++;
                    }
                }
            }

            for (j=1; j<subwaystopcnt[subwaytrackcnt]; j++)
                for (k=0; k<j; k++)
                    if (subwaystop[subwaytrackcnt][j] < subwaystop[subwaytrackcnt][k])
                    {
                        s = subwaystop[subwaytrackcnt][j];
                        subwaystop[subwaytrackcnt][j] = subwaystop[subwaytrackcnt][k];
                        subwaystop[subwaytrackcnt][k] = s;
                    }

            subwaygoalstop[subwaytrackcnt] = 0;
            for (j=0; j<subwaystopcnt[subwaytrackcnt]; j++)
                if (klabs(subwaystop[subwaytrackcnt][j]-subwayx[subwaytrackcnt]) < klabs(subwaystop[subwaytrackcnt][subwaygoalstop[subwaytrackcnt]]-subwayx[subwaytrackcnt]))
                    subwaygoalstop[subwaytrackcnt] = j;

            subwaytrackx1[subwaytrackcnt] = dax;
            subwaytracky1[subwaytrackcnt] = day;
            subwaytrackx2[subwaytrackcnt] = dax2;
            subwaytracky2[subwaytrackcnt] = day2;

            subwaynumsectors[subwaytrackcnt] = 1;
            for (j=0; j<numsectors; j++)
                if (j != i)
                {
                    startwall = sector[j].wallptr;
                    if (wall[startwall].x > subwaytrackx1[subwaytrackcnt])
                        if (wall[startwall].y > subwaytracky1[subwaytrackcnt])
                            if (wall[startwall].x < subwaytrackx2[subwaytrackcnt])
                                if (wall[startwall].y < subwaytracky2[subwaytrackcnt])
                                {
                                    if (sector[j].floorz != sector[i].floorz)
                                    {
                                        sector[j].ceilingstat |= 64;
                                        sector[j].floorstat |= 64;
                                    }
                                    subwaytracksector[subwaytrackcnt][subwaynumsectors[subwaytrackcnt]] = j;
                                    subwaynumsectors[subwaytrackcnt]++;
                                }
                }

            subwayvel[subwaytrackcnt] = 64;
            subwaypausetime[subwaytrackcnt] = 720;

            startwall = sector[i].wallptr;
            endwall = startwall+sector[i].wallnum;
            for (k=startwall; k<endwall; k++)
                if (wall[k].x > subwaytrackx1[subwaytrackcnt])
                    if (wall[k].y > subwaytracky1[subwaytrackcnt])
                        if (wall[k].x < subwaytrackx2[subwaytrackcnt])
                            if (wall[k].y < subwaytracky2[subwaytrackcnt])
                                setinterpolation(&wall[k].x);

            for (j=1; j<subwaynumsectors[subwaytrackcnt]; j++)
            {
                dasector = subwaytracksector[subwaytrackcnt][j];

                startwall = sector[dasector].wallptr;
                endwall = startwall+sector[dasector].wallnum;
                for (k=startwall; k<endwall; k++)
                    setinterpolation(&wall[k].x);

                for (k=headspritesect[dasector]; k>=0; k=nextspritesect[k])
                    if (statrate[sprite[k].statnum] < 0)
                        setinterpolation(&sprite[k].x);
            }


            subwaytrackcnt++;
            break;
        }
        if (sector[i].floorpicnum == FLOORMIRROR)
            floormirrorsector[mirrorcnt++] = i;
        //if (sector[i].ceilingpicnum == FLOORMIRROR) floormirrorsector[mirrorcnt++] = i; //SOS
    }

    //Scan wall tags

    mirrorcnt = 0;
    tilesiz[MIRROR].x = 0;
    tilesiz[MIRROR].y = 0;
    for (i=0; i<MAXMIRRORS; i++)
    {
        tilesiz[i+MIRRORLABEL].x = 0;
        tilesiz[i+MIRRORLABEL].y = 0;
    }

    ypanningwallcnt = 0;
    for (i=0; i<numwalls; i++)
    {
        if (wall[i].lotag == 1) ypanningwalllist[ypanningwallcnt++] = i;
        s = wall[i].nextsector;
        if ((s >= 0) && (wall[i].overpicnum == MIRROR) && (wall[i].cstat&32))
        {
            if ((sector[s].floorstat&1) == 0)
            {
                wall[i].overpicnum = MIRRORLABEL+mirrorcnt;
                sector[s].ceilingpicnum = MIRRORLABEL+mirrorcnt;
                sector[s].floorpicnum = MIRRORLABEL+mirrorcnt;
                sector[s].floorstat |= 1;
                mirrorwall[mirrorcnt] = i;
                mirrorsector[mirrorcnt] = s;
                mirrorcnt++;
            }
            else
                wall[i].overpicnum = sector[s].ceilingpicnum;
        }
    }

    //Invalidate textures in sector behind mirror
    for (i=0; i<mirrorcnt; i++)
    {
        k = mirrorsector[i];
        startwall = sector[k].wallptr;
        endwall = startwall + sector[k].wallnum;
        for (j=startwall; j<endwall; j++)
        {
            wall[j].picnum = MIRROR;
            wall[j].overpicnum = MIRROR;
        }
    }

    //Scan sprite tags&picnum's

    turnspritecnt = 0;
    for (i=0; i<MAXSPRITES; i++)
    {
        if (sprite[i].lotag == 3) turnspritelist[turnspritecnt++] = i;

        if (sprite[i].statnum < MAXSTATUS)    //That is, if sprite exists
            switch (sprite[i].picnum)
            {
            case BROWNMONSTER:              //All cases here put the sprite
                if ((sprite[i].cstat&128) == 0)
                {
                    sprite[i].z -= ((tilesiz[sprite[i].picnum].y*sprite[i].yrepeat)<<1);
                    sprite[i].cstat |= 128;
                }
                sprite[i].extra = sprite[i].ang;
                sprite[i].clipdist = mulscale7(sprite[i].xrepeat,tilesiz[sprite[i].picnum].x);
                if (sprite[i].statnum != 1) changespritestat(i,2);   //on waiting for you (list 2)
                sprite[i].lotag = mulscale5(sprite[i].xrepeat,sprite[i].yrepeat);
                sprite[i].cstat |= 0x101;    //Set the hitscan sensitivity bit
                break;
            case AL:
                sprite[i].cstat |= 0x101;    //Set the hitscan sensitivity bit
                sprite[i].lotag = 0x60;
                changespritestat(i,0);
                break;
            case EVILAL:
                sprite[i].cstat |= 0x101;    //Set the hitscan sensitivity bit
                sprite[i].lotag = 0x60;
                changespritestat(i,10);
                break;
            }
    }

    for (i=MAXSPRITES-1; i>=0; i--) copybuf(&sprite[i].x,&osprite[i].x,3);

    searchmap(cursectnum[connecthead]);

    lockclock = 0;
    ototalclock = 0;
    gotlastpacketclock = 0;

    screensize = xdim;
    dax = ((xdim-screensize)>>1);
    dax2 = dax+screensize-1;
    day = (((ydim-32)-scale(screensize,ydim-32,xdim))>>1);
    day2 = day + scale(screensize,ydim-32,xdim)-1;
    setview(dax,day,dax2,day2);

    startofdynamicinterpolations = numinterpolations;

#if 0
    for (i=connecthead; i>=0; i=connectpoint2[i]) myminlag[i] = 0;
    otherminlag = mymaxlag = 0;
#endif
}

void checktouchsprite(short snum, short sectnum)
{
    int i, nexti;

    if ((sectnum < 0) || (sectnum >= numsectors)) return;

    for (i=headspritesect[sectnum]; i>=0; i=nexti)
    {
        nexti = nextspritesect[i];
        if (sprite[i].cstat&0x8000) continue;
        if ((klabs(pos[snum].x-sprite[i].x)+klabs(pos[snum].y-sprite[i].y) < 512) && (klabs((pos[snum].z>>8)-((sprite[i].z>>8)-(tilesiz[sprite[i].picnum].y>>1))) <= 40))
        {
            switch (sprite[i].picnum)
            {
            case COIN:
                wsayfollow("getstuff.wav",4096L+(krand()&127)-64,192L,&sprite[i].x,&sprite[i].y,0);
                changehealth(snum,5);
                if (sprite[i].statnum == 12) deletesprite((short)i);
                else
                {
                    sprite[i].cstat |= 0x8000;
                    sprite[i].extra = 120*60;
                    changespritestat((short)i,11);
                }
                break;
            case DIAMONDS:
                wsayfollow("getstuff.wav",4096L+(krand()&127)-64,256L,&sprite[i].x,&sprite[i].y,0);
                changehealth(snum,15);
                if (sprite[i].statnum == 12) deletesprite((short)i);
                else
                {
                    sprite[i].cstat |= 0x8000;
                    sprite[i].extra = 120*120;
                    changespritestat((short)i,11);
                }
                break;
            case COINSTACK:
                wsayfollow("getstuff.wav",4096L+(krand()&127)-64,256L,&sprite[i].x,&sprite[i].y,0);
                changehealth(snum,25);
                if (sprite[i].statnum == 12) deletesprite((short)i);
                else
                {
                    sprite[i].cstat |= 0x8000;
                    sprite[i].extra = 120*180;
                    changespritestat((short)i,11);
                }
                break;
            case GIFTBOX:
                wsayfollow("getstuff.wav",4096L+(krand()&127)+256-mulscale4(sprite[i].xrepeat,sprite[i].yrepeat),208L,&sprite[i].x,&sprite[i].y,0);
                changehealth(snum,max(mulscale8(sprite[i].xrepeat,sprite[i].yrepeat),1));
                if (sprite[i].statnum == 12) deletesprite((short)i);
                else
                {
                    sprite[i].cstat |= 0x8000;
                    sprite[i].extra = 90*(sprite[i].xrepeat+sprite[i].yrepeat);
                    changespritestat((short)i,11);
                }
                break;
            case CANNON:
                wsayfollow("getstuff.wav",3584L+(krand()&127)-64,256L,&sprite[i].x,&sprite[i].y,0);
                if (snum == myconnectindex) keystatus[4] = 1;
                changenumbombs(snum,((sprite[i].xrepeat+sprite[i].yrepeat)>>1));
                if (sprite[i].statnum == 12) deletesprite((short)i);
                else
                {
                    sprite[i].cstat |= 0x8000;
                    sprite[i].extra = 60*(sprite[i].xrepeat+sprite[i].yrepeat);
                    changespritestat((short)i,11);
                }
                break;
            case LAUNCHER:
                wsayfollow("getstuff.wav",3584L+(krand()&127)-64,256L,&sprite[i].x,&sprite[i].y,0);
                if (snum == myconnectindex) keystatus[5] = 1;
                changenummissiles(snum,((sprite[i].xrepeat+sprite[i].yrepeat)>>1));
                if (sprite[i].statnum == 12) deletesprite((short)i);
                else
                {
                    sprite[i].cstat |= 0x8000;
                    sprite[i].extra = 90*(sprite[i].xrepeat+sprite[i].yrepeat);
                    changespritestat((short)i,11);
                }
                break;
            case GRABCANNON:
                wsayfollow("getstuff.wav",3584L+(krand()&127)-64,256L,&sprite[i].x,&sprite[i].y,0);
                if (snum == myconnectindex) keystatus[6] = 1;
                changenumgrabbers(snum,((sprite[i].xrepeat+sprite[i].yrepeat)>>1));
                if (sprite[i].statnum == 12) deletesprite((short)i);
                else
                {
                    sprite[i].cstat |= 0x8000;
                    sprite[i].extra = 120*(sprite[i].xrepeat+sprite[i].yrepeat);
                    changespritestat((short)i,11);
                }
                break;
            case AIRPLANE:
                wsayfollow("getstuff.wav",4096L+(krand()&127)-64,256L,&sprite[i].x,&sprite[i].y,0);
                if (flytime[snum] < lockclock) flytime[snum] = lockclock;
                flytime[snum] += 60*(sprite[i].xrepeat+sprite[i].yrepeat);
                drawstatusflytime(snum);
                if (sprite[i].statnum == 12) deletesprite((short)i);
                else
                {
                    sprite[i].cstat |= 0x8000;
                    sprite[i].extra = 120*(sprite[i].xrepeat+sprite[i].yrepeat);
                    changespritestat((short)i,11);
                }
                break;
            }
        }
    }
}

void checkgrabbertouchsprite(short snum, short sectnum)   // Andy did this
{
    int i, nexti;
    short onum;

    if ((sectnum < 0) || (sectnum >= numsectors)) return;
    onum = (sprite[snum].owner & (MAXSPRITES - 1));

    for (i=headspritesect[sectnum]; i>=0; i=nexti)
    {
        nexti = nextspritesect[i];
        if (sprite[i].cstat&0x8000) continue;
        if ((klabs(sprite[snum].x-sprite[i].x)+klabs(sprite[snum].y-sprite[i].y) < 512) && (klabs((sprite[snum].z>>8)-((sprite[i].z>>8)-(tilesiz[sprite[i].picnum].y>>1))) <= 40))
        {
            switch (sprite[i].picnum)
            {
            case COIN:
                wsayfollow("getstuff.wav",4096L+(krand()&127)-64,192L,&sprite[i].x,&sprite[i].y,0);
                changehealth(onum,5);
                if (sprite[i].statnum == 12) deletesprite((short)i);
                else
                {
                    sprite[i].cstat |= 0x8000;
                    sprite[i].extra = 120*60;
                    changespritestat((short)i,11);
                }
                break;
            case DIAMONDS:
                wsayfollow("getstuff.wav",4096L+(krand()&127)-64,256L,&sprite[i].x,&sprite[i].y,0);
                changehealth(onum,15);
                if (sprite[i].statnum == 12) deletesprite((short)i);
                else
                {
                    sprite[i].cstat |= 0x8000;
                    sprite[i].extra = 120*120;
                    changespritestat((short)i,11);
                }
                break;
            case COINSTACK:
                wsayfollow("getstuff.wav",4096L+(krand()&127)-64,256L,&sprite[i].x,&sprite[i].y,0);
                changehealth(onum,25);
                if (sprite[i].statnum == 12) deletesprite((short)i);
                else
                {
                    sprite[i].cstat |= 0x8000;
                    sprite[i].extra = 120*180;
                    changespritestat((short)i,11);
                }
                break;
            case GIFTBOX:
                wsayfollow("getstuff.wav",4096L+(krand()&127)+256-mulscale4(sprite[i].xrepeat,sprite[i].yrepeat),208L,&sprite[i].x,&sprite[i].y,0);
                changehealth(onum,max(mulscale8(sprite[i].xrepeat,sprite[i].yrepeat),1));
                if (sprite[i].statnum == 12) deletesprite((short)i);
                else
                {
                    sprite[i].cstat |= 0x8000;
                    sprite[i].extra = 90*(sprite[i].xrepeat+sprite[i].yrepeat);
                    changespritestat((short)i,11);
                }
                break;
            case CANNON:
                wsayfollow("getstuff.wav",3584L+(krand()&127)-64,256L,&sprite[i].x,&sprite[i].y,0);
                if (onum == myconnectindex) keystatus[4] = 1;
                changenumbombs(onum,((sprite[i].xrepeat+sprite[i].yrepeat)>>1));
                if (sprite[i].statnum == 12) deletesprite((short)i);
                else
                {
                    sprite[i].cstat |= 0x8000;
                    sprite[i].extra = 60*(sprite[i].xrepeat+sprite[i].yrepeat);
                    changespritestat((short)i,11);
                }
                break;
            case LAUNCHER:
                wsayfollow("getstuff.wav",3584L+(krand()&127)-64,256L,&sprite[i].x,&sprite[i].y,0);
                if (onum == myconnectindex) keystatus[5] = 1;
                changenummissiles(onum,((sprite[i].xrepeat+sprite[i].yrepeat)>>1));
                if (sprite[i].statnum == 12) deletesprite((short)i);
                else
                {
                    sprite[i].cstat |= 0x8000;
                    sprite[i].extra = 90*(sprite[i].xrepeat+sprite[i].yrepeat);
                    changespritestat((short)i,11);
                }
                break;
            case GRABCANNON:
                wsayfollow("getstuff.wav",3584L+(krand()&127)-64,256L,&sprite[i].x,&sprite[i].y,0);
                if (onum == myconnectindex) keystatus[6] = 1;
                changenumgrabbers(onum,((sprite[i].xrepeat+sprite[i].yrepeat)>>1));
                if (sprite[i].statnum == 12) deletesprite((short)i);
                else
                {
                    sprite[i].cstat |= 0x8000;
                    sprite[i].extra = 120*(sprite[i].xrepeat+sprite[i].yrepeat);
                    changespritestat((short)i,11);
                }
                break;
            case AIRPLANE:
                wsayfollow("getstuff.wav",4096L+(krand()&127)-64,256L,&sprite[i].x,&sprite[i].y,0);
                if (flytime[snum] < lockclock) flytime[snum] = lockclock;
                flytime[onum] += 60*(sprite[i].xrepeat+sprite[i].yrepeat);
                drawstatusflytime(onum);
                if (sprite[i].statnum == 12) deletesprite((short)i);
                else
                {
                    sprite[i].cstat |= 0x8000;
                    sprite[i].extra = 120*(sprite[i].xrepeat+sprite[i].yrepeat);
                    changespritestat((short)i,11);
                }
                break;
            }
        }
    }
}

void shootgun(short snum, const vec3_t *vector,
              short daang, int dahoriz, short dasectnum, char guntype)
{
    short daang2;
    int /*i,*/ j, daz2;
    hitdata_t hitinfo;

    switch (guntype)
    {
    case 0:    //Shoot chain gun
        daang2 = ((daang + (krand()&31)-16)&2047);
        daz2 = ((100-dahoriz)*2000) + ((krand()-32768)>>1);

        hitscan(vector,dasectnum,                   //Start position
                sintable[(daang2+512)&2047],            //X vector of 3D ang
                sintable[daang2&2047],                  //Y vector of 3D ang
                daz2,                                   //Z vector of 3D ang
                &hitinfo,CLIPMASK1);

        if (wall[hitinfo.wall].picnum == KENPICTURE)
        {
            if (waloff[MAXTILES-1] != 0) wall[hitinfo.wall].picnum = MAXTILES-1;
            wsayfollow("hello.wav",4096L+(krand()&127)-64,256L,&wall[hitinfo.wall].x,&wall[hitinfo.wall].y,0);
        }
        else if (((hitinfo.wall < 0) && (hitinfo.sprite < 0) && (hitinfo.pos.z >= vector->z) && ((sector[hitinfo.sect].floorpicnum == SLIME) || (sector[hitinfo.sect].floorpicnum == FLOORMIRROR))) || ((hitinfo.wall >= 0) && (wall[hitinfo.wall].picnum == SLIME)))
        {
            //If you shoot slime, make a splash
            wsayfollow("splash.wav",4096L+(krand()&511)-256,256L,&hitinfo.pos.x,&hitinfo.pos.y,0);
            spawnsprite(j,hitinfo.pos.x,hitinfo.pos.y,hitinfo.pos.z,2,0,0,32,64,64,0,0,SPLASH,daang,
                        0,0,0,snum+4096,hitinfo.sect,4,63,0,0); //63=time left for splash
        }
        else
        {
            wsayfollow("shoot.wav",4096L+(krand()&127)-64,256L,&hitinfo.pos.x,&hitinfo.pos.y,0);

            if ((hitinfo.sprite >= 0) && (sprite[hitinfo.sprite].statnum < MAXSTATUS))
                switch (sprite[hitinfo.sprite].picnum)
                {
                case BROWNMONSTER:
                    if (sprite[hitinfo.sprite].lotag > 0) sprite[hitinfo.sprite].lotag -= 10;
                    if (sprite[hitinfo.sprite].lotag > 0)
                    {
                        wsayfollow("hurt.wav",4096L+(krand()&511)-256,256L,&hitinfo.pos.x,&hitinfo.pos.y,0);
                        if (sprite[hitinfo.sprite].lotag <= 25)
                            sprite[hitinfo.sprite].cstat |= 2;
                    }
                    else
                    {
                        wsayfollow("mondie.wav",4096L+(krand()&127)-64,256L,&hitinfo.pos.x,&hitinfo.pos.y,0);
                        sprite[hitinfo.sprite].z += ((tilesiz[sprite[hitinfo.sprite].picnum].y*sprite[hitinfo.sprite].yrepeat)<<1);
                        sprite[hitinfo.sprite].picnum = GIFTBOX;
                        sprite[hitinfo.sprite].cstat &= ~0x83;    //Should not clip, foot-z
                        changespritestat(hitinfo.sprite,12);

                        spawnsprite(j,hitinfo.pos.x,hitinfo.pos.y,hitinfo.pos.z+(32<<8),0,-4,0,32,64,64,
                                    0,0,EXPLOSION,daang,0,0,0,snum+4096,
                                    hitinfo.sect,5,31,0,0);
                    }
                    break;
                case EVILAL:
                    wsayfollow("blowup.wav",4096L+(krand()&127)-64,256L,&hitinfo.pos.x,&hitinfo.pos.y,0);
                    sprite[hitinfo.sprite].picnum = EVILALGRAVE;
                    sprite[hitinfo.sprite].cstat = 0;
                    sprite[hitinfo.sprite].xvel = (krand()&255)-128;
                    sprite[hitinfo.sprite].yvel = (krand()&255)-128;
                    sprite[hitinfo.sprite].zvel = (krand()&4095)-3072;
                    changespritestat(hitinfo.sprite,9);

                    spawnsprite(j,hitinfo.pos.x,hitinfo.pos.y,hitinfo.pos.z+(32<<8),0,-4,0,32,64,64,0,
                                0,EXPLOSION,daang,0,0,0,snum+4096,hitinfo.sect,5,31,0,0);
                    //31=time left for explosion

                    break;
                case PLAYER:
                    for (j=connecthead; j>=0; j=connectpoint2[j])
                        if (playersprite[j] == hitinfo.sprite)
                        {
                            wsayfollow("ouch.wav",4096L+(krand()&127)-64,256L,&hitinfo.pos.x,&hitinfo.pos.y,0);
                            changehealth(j,-10);
                            break;
                        }
                    break;
                }

            spawnsprite(j,hitinfo.pos.x,hitinfo.pos.y,hitinfo.pos.z+(8<<8),2,-4,0,32,16,16,0,0,
                        EXPLOSION,daang,0,0,0,snum+4096,hitinfo.sect,3,63,0,0);

            //Sprite starts out with center exactly on wall.
            //This moves it back enough to see it at all angles.
            movesprite((short)j,-(((int)sintable[(512+daang)&2047]*TICSPERFRAME)<<4),-(((int)sintable[daang]*TICSPERFRAME)<<4),0L,4L<<8,4L<<8,CLIPMASK1);
        }
        break;
    case 1:    //Shoot silver sphere bullet
        spawnsprite(j,vector->x,vector->y,vector->z,1+128,0,0,16,64,64,0,0,BULLET,daang,
                    sintable[(daang+512)&2047]>>5,sintable[daang&2047]>>5,
                    (100-dahoriz)<<6,snum+4096,dasectnum,6,0,0,0);
        wsayfollow("shoot2.wav",4096L+(krand()&127)-64,128L,&sprite[j].x,&sprite[j].y,1);
        break;
    case 2:    //Shoot bomb
        spawnsprite(j,vector->x,vector->y,vector->z,128,0,0,12,16,16,0,0,BOMB,daang,
                    sintable[(daang+512)&2047]*5>>8,sintable[daang&2047]*5>>8,
                    (80-dahoriz)<<6,snum+4096,dasectnum,6,0,0,0);
        wsayfollow("shoot3.wav",4096L+(krand()&127)-64,192L,&sprite[j].x,&sprite[j].y,1);
        break;
    case 3:    //Shoot missile (Andy did this)
        spawnsprite(j,vector->x,vector->y,vector->z,1+128,0,0,16,32,32,0,0,MISSILE,daang,
                    sintable[(daang+512)&2047]>>4,sintable[daang&2047]>>4,
                    (100-dahoriz)<<7,snum+4096,dasectnum,6,0,0,0);
        wsayfollow("shoot3.wav",4096L+(krand()&127)-64,192L,&sprite[j].x,&sprite[j].y,1);
        break;
    case 4:    //Shoot grabber (Andy did this)
        spawnsprite(j,vector->x,vector->y,vector->z,1+128,0,0,16,64,64,0,0,GRABBER,daang,
                    sintable[(daang+512)&2047]>>5,sintable[daang&2047]>>5,
                    (100-dahoriz)<<6,snum+4096,dasectnum,6,0,0,0);
        wsayfollow("shoot4.wav",4096L+(krand()&127)-64,128L,&sprite[j].x,&sprite[j].y,1);
        break;
    }
}

#define MAXVOXMIPS 5
extern intptr_t voxoff[][MAXVOXMIPS];
void analyzesprites(int dax, int day)
{
    int i, j=0, k, *intptr;
    vec3_t *ospr;
    uspritetype *tspr;

    //This function is called between drawrooms() and drawmasks()
    //It has a list of possible sprites that may be drawn on this frame

    for (i=0,tspr=&tsprite[0]; i<spritesortcnt; i++,tspr++)
    {
        if (usevoxels && tiletovox[tspr->picnum] >= 0)
            switch (tspr->picnum)
            {
            case PLAYER:
                //   //Get which of the 8 angles of the sprite to draw (0-7)
                //   //k ranges from 0-7
                //k = getangle(tspr->x-dax,tspr->y-day);
                //k = (((tspr->ang+3072+128-k)&2047)>>8)&7;
                //   //This guy has only 5 pictures for 8 angles (3 are x-flipped)
                //if (k <= 4)
                //{
                //   tspr->picnum += (k<<2);
                //   tspr->cstat &= ~4;   //clear x-flipping bit
                //}
                //else
                //{
                //   tspr->picnum += ((8-k)<<2);
                //   tspr->cstat |= 4;    //set x-flipping bit
                //}

                if ((tspr->cstat&2) == 0)
                {
                    //tspr->cstat |= 48; tspr->picnum = tiletovox[tspr->picnum];
                    intptr = (int *)voxoff[tiletovox[PLAYER]][0];
                    tspr->xrepeat = scale(tspr->xrepeat,56,intptr[2]);
                    tspr->yrepeat = scale(tspr->yrepeat,56,intptr[2]);
                    tspr->shade -= 6;
                }
                break;
            case BROWNMONSTER:
                //tspr->cstat |= 48; tspr->picnum = tiletovox[tspr->picnum];
                break;
            }

        k = statrate[tspr->statnum];
        if (k >= 0)  //Interpolate moving sprite
        {
            ospr = &osprite[tspr->owner];
            switch (k)
            {
            case 0: j = smoothratio; break;
            case 1: j = (smoothratio>>1)+(((nummoves-tspr->owner)&1)<<15); break;
            case 3: j = (smoothratio>>2)+(((nummoves-tspr->owner)&3)<<14); break;
            case 7: j = (smoothratio>>3)+(((nummoves-tspr->owner)&7)<<13); break;
            case 15: j = (smoothratio>>4)+(((nummoves-tspr->owner)&15)<<12); break;
            }
            k = tspr->x-ospr->x; tspr->x = ospr->x;
            if (k != 0) tspr->x += mulscale16(k,j);
            k = tspr->y-ospr->y; tspr->y = ospr->y;
            if (k != 0) tspr->y += mulscale16(k,j);
            k = tspr->z-ospr->z; tspr->z = ospr->z;
            if (k != 0) tspr->z += mulscale16(k,j);
        }

        //Don't allow close explosion sprites to be transluscent
        k = tspr->statnum;
        if ((k == 3) || (k == 4) || (k == 5) || (k == 7))
            if (klabs(dax-tspr->x) < 256)
                if (klabs(day-tspr->y) < 256)
                    tspr->cstat &= ~2;

        tspr->shade += 6;
        if (sector[tspr->sectnum].ceilingstat&1)
            tspr->shade += sector[tspr->sectnum].ceilingshade;
        else
            tspr->shade += sector[tspr->sectnum].floorshade;
    }
}

void tagcode(void)
{
    int i, /*nexti,*/ j, k, l, s, /*daz, dax2, day2,*/ cnt, good;
    short startwall, endwall, dasector, p, oldang;

    for (p=connecthead; p>=0; p=connectpoint2[p])
    {
        if (sector[cursectnum[p]].lotag == 1)
        {
            activatehitag(sector[cursectnum[p]].hitag);
            sector[cursectnum[p]].lotag = 0;
            sector[cursectnum[p]].hitag = 0;
        }
        if ((sector[cursectnum[p]].lotag == 2) && (cursectnum[p] != ocursectnum[p]))
            activatehitag(sector[cursectnum[p]].hitag);
    }

    for (i=0; i<warpsectorcnt; i++)
    {
        dasector = warpsectorlist[i];
        j = ((lockclock&127)>>2);
        if (j >= 16) j = 31-j;
        {
            sector[dasector].ceilingshade = j;
            sector[dasector].floorshade = j;
            startwall = sector[dasector].wallptr;
            endwall = startwall+sector[dasector].wallnum;
            for (s=startwall; s<endwall; s++)
                wall[s].shade = j;
        }
    }

    for (p=connecthead; p>=0; p=connectpoint2[p])
        if (sector[cursectnum[p]].lotag == 10)  //warp sector
        {
            if (cursectnum[p] != ocursectnum[p])
            {
                warpsprite(playersprite[p]);
                pos[p].x = sprite[playersprite[p]].x;
                pos[p].y = sprite[playersprite[p]].y;
                pos[p].z = sprite[playersprite[p]].z;
                ang[p] = sprite[playersprite[p]].ang;
                cursectnum[p] = sprite[playersprite[p]].sectnum;

                sprite[playersprite[p]].z += EYEHEIGHT;

                //warp(&pos[p].x,&pos[p].y,&pos[p].z,&ang[p],&cursectnum[p]);
                //Update sprite representation of player
                //setsprite_eyeheight(playersprite[p],&pos[p]);
                //sprite[playersprite[p]].ang = ang[p];
            }
        }

    for (i=0; i<xpanningsectorcnt; i++) //animate wall x-panning sectors
    {
        dasector = xpanningsectorlist[i];

        startwall = sector[dasector].wallptr;
        endwall = startwall+sector[dasector].wallnum;
        for (s=startwall; s<endwall; s++)
            wall[s].xpanning = ((lockclock>>2)&255);
    }

    for (i=0; i<ypanningwallcnt; i++)
        wall[ypanningwalllist[i]].ypanning = ~(lockclock&255);

    for (i=0; i<turnspritecnt; i++)
    {
        sprite[turnspritelist[i]].ang += (TICSPERFRAME<<2);
        sprite[turnspritelist[i]].ang &= 2047;
    }

    for (i=0; i<floorpanningcnt; i++) //animate floor of slime sectors
    {
        sector[floorpanninglist[i]].floorxpanning = ((lockclock>>2)&255);
        sector[floorpanninglist[i]].floorypanning = ((lockclock>>2)&255);
    }

    for (i=0; i<dragsectorcnt; i++)
    {
        dasector = dragsectorlist[i];

        startwall = sector[dasector].wallptr;
        endwall = startwall+sector[dasector].wallnum;

        if (wall[startwall].x+dragxdir[i] < dragx1[i]) dragxdir[i] = 16;
        if (wall[startwall].y+dragydir[i] < dragy1[i]) dragydir[i] = 16;
        if (wall[startwall].x+dragxdir[i] > dragx2[i]) dragxdir[i] = -16;
        if (wall[startwall].y+dragydir[i] > dragy2[i]) dragydir[i] = -16;

        for (j=startwall; j<endwall; j++)
            dragpoint(j,wall[j].x+dragxdir[i],wall[j].y+dragydir[i],0);
        j = sector[dasector].floorz;
        sector[dasector].floorz = dragfloorz[i]+(sintable[(lockclock<<4)&2047]>>3);

        for (p=connecthead; p>=0; p=connectpoint2[p])
            if (cursectnum[p] == dasector)
            {
                pos[p].x += dragxdir[i];
                pos[p].y += dragydir[i];
                if (p == myconnectindex)
                { my.x += dragxdir[i]; my.y += dragydir[i]; }
                //pos[p].z += (sector[dasector].floorz-j);

                //Update sprite representation of player
                setsprite_eyeheight(playersprite[p],&pos[p]);
                sprite[playersprite[p]].ang = ang[p];
            }
    }

    for (i=0; i<swingcnt; i++)
    {
        if (swinganginc[i] != 0)
        {
            oldang = swingang[i];
            for (j=0; j<(TICSPERFRAME<<2); j++)
            {
                swingang[i] = ((swingang[i]+swinganginc[i])&2047);
                if (swingang[i] == swingangclosed[i])
                {
                    wsayfollow("closdoor.wav",4096L+(krand()&511)-256,256L,&swingx[i][0],&swingy[i][0],0);
                    swinganginc[i] = 0;
                }
                if (swingang[i] == swingangopen[i]) swinganginc[i] = 0;
            }
            for (k=1; k<=3; k++)
            {
                vec2_t const pivot = { swingx[i][0], swingy[i][0] };
                vec2_t const p = { swingx[i][k], swingy[i][k] };
                rotatepoint(pivot, p, swingang[i], (vec2_t *)&wall[swingwall[i][k]].x);
            }

            if (swinganginc[i] != 0)
            {
                for (p=connecthead; p>=0; p=connectpoint2[p])
                    if ((cursectnum[p] == swingsector[i]) || (testneighborsectors(cursectnum[p],swingsector[i]) == 1))
                    {
                        cnt = 256;
                        do
                        {
                            good = 1;

                            //swingangopendir is -1 if forwards, 1 is backwards
                            l = (swingangopendir[i] > 0);
                            for (k=l+3; k>=l; k--)
                                if (clipinsidebox((vec2_t *)&pos[p],swingwall[i][k],128L) != 0)
                                {
                                    good = 0;
                                    break;
                                }
                            if (good == 0)
                            {
                                if (cnt == 256)
                                {
                                    swinganginc[i] = -swinganginc[i];
                                    swingang[i] = oldang;
                                }
                                else
                                {
                                    swingang[i] = ((swingang[i]-swinganginc[i])&2047);
                                }
                                for (k=1; k<=3; k++)
                                {
                                    vec2_t const pivot = { swingx[i][0], swingy[i][0] };
                                    vec2_t const p = { swingx[i][k], swingy[i][k] };
                                    rotatepoint(pivot, p, swingang[i], (vec2_t *)&wall[swingwall[i][k]].x);
                                }
                                if (swingang[i] == swingangclosed[i])
                                {
                                    wsayfollow("closdoor.wav",4096L+(krand()&511)-256,256L,&swingx[i][0],&swingy[i][0],0);
                                    swinganginc[i] = 0;
                                    break;
                                }
                                if (swingang[i] == swingangopen[i])
                                {
                                    swinganginc[i] = 0;
                                    break;
                                }
                                cnt--;
                            }
                        }
                        while ((good == 0) && (cnt > 0));
                    }
            }
        }
        if (swinganginc[i] == 0)
            for (j=1; j<=3; j++)
            {
                stopinterpolation(&wall[swingwall[i][j]].x);
                stopinterpolation(&wall[swingwall[i][j]].y);
            }
    }

    for (i=0; i<revolvecnt; i++)
    {
        startwall = sector[revolvesector[i]].wallptr;
        endwall = startwall + sector[revolvesector[i]].wallnum;

        revolveang[i] = ((revolveang[i]-(TICSPERFRAME<<2))&2047);
        for (k=startwall; k<endwall; k++)
        {
            vec2_t const pivot = { revolvepivotx[i], revolvepivoty[i] };
            vec2_t const p = { revolvex[i][k-startwall], revolvey[i][k-startwall] };
            vec2_t daxy;
            rotatepoint(pivot, p, revolveang[i], &daxy);
            dragpoint(k,daxy.x,daxy.y,0);
        }
    }

    for (i=0; i<subwaytrackcnt; i++)
    {
        if ((subwayvel[i] < -2) || (subwayvel[i] > 2))
        {
            dasector = subwaytracksector[i][0];
            startwall = sector[dasector].wallptr;
            endwall = startwall+sector[dasector].wallnum;
            for (k=startwall; k<endwall; k++)
                if (wall[k].x > subwaytrackx1[i])
                    if (wall[k].y > subwaytracky1[i])
                        if (wall[k].x < subwaytrackx2[i])
                            if (wall[k].y < subwaytracky2[i])
                                wall[k].x += subwayvel[i];

            for (j=1; j<subwaynumsectors[i]; j++)
            {
                dasector = subwaytracksector[i][j];

                startwall = sector[dasector].wallptr;
                endwall = startwall+sector[dasector].wallnum;
                for (k=startwall; k<endwall; k++)
                    wall[k].x += subwayvel[i];

                for (s=headspritesect[dasector]; s>=0; s=nextspritesect[s])
                    sprite[s].x += subwayvel[i];
            }

            for (p=connecthead; p>=0; p=connectpoint2[p])
                if (cursectnum[p] != subwaytracksector[i][0])
                    if (sector[cursectnum[p]].floorz != sector[subwaytracksector[i][0]].floorz)
                        if (pos[p].x > subwaytrackx1[i])
                            if (pos[p].y > subwaytracky1[i])
                                if (pos[p].x < subwaytrackx2[i])
                                    if (pos[p].y < subwaytracky2[i])
                                    {
                                        pos[p].x += subwayvel[i];
                                        if (p == myconnectindex)
                                        { my.x += subwayvel[i]; }

                                        //Update sprite representation of player
                                        setsprite_eyeheight(playersprite[p],&pos[p]);
                                        sprite[playersprite[p]].ang = ang[p];
                                    }

            subwayx[i] += subwayvel[i];
        }

        j = subwayvel[i];
        k = subwaystop[i][subwaygoalstop[i]] - subwayx[i];
        if (k > 0)
        {
            if (k > 4096)
            {
                if (subwayvel[i] < 256) subwayvel[i]++;
            }
            else
                subwayvel[i] = (k>>4)+1;
        }
        else if (k < 0)
        {
            if (k < -4096)
            {
                if (subwayvel[i] > -256) subwayvel[i]--;
            }
            else
                subwayvel[i] = (k>>4)-1;
        }
        if ((j < 0) && (subwayvel[i] >= 0)) subwayvel[i] = -1;
        if ((j > 0) && (subwayvel[i] <= 0)) subwayvel[i] = 1;

        if ((subwayvel[i] <= 2) && (subwayvel[i] >= -2) && (klabs(k) < 2048))
        {
            //Open / close doors
            if ((subwaypausetime[i] == 720) || ((subwaypausetime[i] >= 120) && (subwaypausetime[i]-TICSPERFRAME < 120)))
                activatehitag(sector[subwaytracksector[i][0]].hitag);

            subwaypausetime[i] -= TICSPERFRAME;
            if (subwaypausetime[i] < 0)
            {
                subwaypausetime[i] = 720;
                if (subwayvel[i] < 0)
                {
                    subwaygoalstop[i]--;
                    if (subwaygoalstop[i] < 0)
                    {
                        subwaygoalstop[i] = 1;
                        subwayvel[i] = 1;
                    }
                }
                else if (subwayvel[i] > 0)
                {
                    subwaygoalstop[i]++;
                    if (subwaygoalstop[i] >= subwaystopcnt[i])
                    {
                        subwaygoalstop[i] = subwaystopcnt[i]-2;
                        subwayvel[i] = -1;
                    }
                }
            }
        }
    }
}

void statuslistcode(void)
{
    short p, target, hitobject, daang, osectnum, movestat;
    int i, nexti, j, nextj, k, l, dax, day, daz, dist=0, ox, oy, mindist;
    int doubvel, xvect, yvect;

    //Go through active BROWNMONSTER list
    for (i=headspritestat[1]; i>=0; i=nexti)
    {
        nexti = nextspritestat[i];

        k = krand();

        //Choose a target player
        mindist = 0x7fffffff; target = connecthead;
        for (p=connecthead; p>=0; p=connectpoint2[p])
        {
            dist = klabs(sprite[i].x-pos[p].x)+klabs(sprite[i].y-pos[p].y);
            if (dist < mindist) mindist = dist, target = p;
        }

        //brown monster decides to shoot bullet
        if ((k&63) == 23)
        {
            if (cansee(sprite[i].x,sprite[i].y,sprite[i].z-(tilesiz[sprite[i].picnum].y<<7),sprite[i].sectnum,pos[target].x,pos[target].y,pos[target].z,cursectnum[target]) == 0)
            {
                if ((k&0xf00) == 0xb00) changespritestat(i,2);
            }
            else
            {
                wsayfollow("monshoot.wav",5144L+(krand()&127)-64,256L,&sprite[i].x,&sprite[i].y,1);

                doubvel = (TICSPERFRAME<<((ssync[target].bits&256)>0));
                xvect = 0, yvect = 0;
                if (ssync[target].fvel != 0)
                {
                    xvect += ((((int)ssync[target].fvel)*doubvel*(int)sintable[(ang[target]+512)&2047])>>3);
                    yvect += ((((int)ssync[target].fvel)*doubvel*(int)sintable[ang[target]&2047])>>3);
                }
                if (ssync[target].svel != 0)
                {
                    xvect += ((((int)ssync[target].svel)*doubvel*(int)sintable[ang[target]&2047])>>3);
                    yvect += ((((int)ssync[target].svel)*doubvel*(int)sintable[(ang[target]+1536)&2047])>>3);
                }

                ox = pos[target].x; oy = pos[target].y;

                //distance is j
                j = ksqrt((ox-sprite[i].x)*(ox-sprite[i].x)+(oy-sprite[i].y)*(oy-sprite[i].y));

                switch ((sprite[i].extra>>11)&3)
                {
                case 1: j = -(j>>1); break;
                case 3: j = 0; break;
                case 0: case 2: break;
                }
                sprite[i].extra += 2048;

                //rate is (TICSPERFRAME<<19)
                xvect = scale(xvect,j,TICSPERFRAME<<19);
                yvect = scale(yvect,j,TICSPERFRAME<<19);
                clipmove_old(&ox,&oy,&pos[target].z,&cursectnum[target],xvect<<14,yvect<<14,128L,4<<8,4<<8,CLIPMASK0);
                ox -= sprite[i].x;
                oy -= sprite[i].y;

                daang = ((getangle(ox,oy)+(krand()&7)-4)&2047);

                dax = (sintable[(daang+512)&2047]>>6);
                day = (sintable[daang&2047]>>6);
                daz = 0;
                if (ox != 0)
                    daz = scale(dax,pos[target].z+(8<<8)-sprite[i].z,ox);
                else if (oy != 0)
                    daz = scale(day,pos[target].z+(8<<8)-sprite[i].z,oy);

                spawnsprite(j,sprite[i].x,sprite[i].y,sprite[i].z,128,0,0,
                            16,sprite[i].xrepeat,sprite[i].yrepeat,0,0,BULLET,daang,dax,day,daz,i,sprite[i].sectnum,6,0,0,0);

                sprite[i].extra &= (~2047);
            }
        }

        //Move brown monster
        dax = sprite[i].x;   //Back up old x&y if stepping off cliff
        day = sprite[i].y;

        doubvel = max(mulscale7(sprite[i].xrepeat,sprite[i].yrepeat),4);

        osectnum = sprite[i].sectnum;
        movestat = movesprite((short)i,(int)sintable[(sprite[i].ang+512)&2047]*doubvel,(int)sintable[sprite[i].ang]*doubvel,0L,4L<<8,4L<<8,CLIPMASK0);
        if (globloz > sprite[i].z+(48<<8))
        { sprite[i].x = dax; sprite[i].y = day; movestat = 1; }
        else
            sprite[i].z = globloz-((tilesiz[sprite[i].picnum].y*sprite[i].yrepeat)<<1);

        if ((sprite[i].sectnum != osectnum) && (sector[sprite[i].sectnum].lotag == 10))
        { warpsprite((short)i); movestat = 0; }

        if ((movestat != 0) || ((k&63) == 1))
        {
            if (sprite[i].ang == (sprite[i].extra&2047))
            {
                daang = (getangle(pos[target].x-sprite[i].x,pos[target].y-sprite[i].y)&2047);
                daang = ((daang+(krand()&1023)-512)&2047);
                sprite[i].extra = ((sprite[i].extra&(~2047))|daang);
            }
            if ((sprite[i].extra-sprite[i].ang)&1024)
            {
                sprite[i].ang = ((sprite[i].ang-32)&2047);
                if (!((sprite[i].extra-sprite[i].ang)&1024)) sprite[i].ang = (sprite[i].extra&2047);
            }
            else
            {
                sprite[i].ang = ((sprite[i].ang+32)&2047);
                if (((sprite[i].extra-sprite[i].ang)&1024)) sprite[i].ang = (sprite[i].extra&2047);
            }
        }
    }

    for (i=headspritestat[10]; i>=0; i=nexti) //EVILAL list
    {
        nexti = nextspritestat[i];

        if (sprite[i].yrepeat < 38) continue;
        if (sprite[i].yrepeat < 64)
        {
            sprite[i].xrepeat++;
            sprite[i].yrepeat++;
            continue;
        }

        if ((nummoves-i)&statrate[10]) continue;

        //Choose a target player
        mindist = 0x7fffffff; target = connecthead;
        for (p=connecthead; p>=0; p=connectpoint2[p])
        {
            dist = klabs(sprite[i].x-pos[p].x)+klabs(sprite[i].y-pos[p].y);
            if (dist < mindist) mindist = dist, target = p;
        }

        k = (krand()&255);

        if ((sprite[i].lotag&32) && (k < 48))  //Al decides to reproduce
        {
            l = 0;
            if ((sprite[i].lotag&64) && (k < 2))  //Give him a chance to reproduce without seeing you
                l = 1;
            else if (cansee(sprite[i].x,sprite[i].y,sprite[i].z-(tilesiz[sprite[i].picnum].y<<7),sprite[i].sectnum,pos[target].x,pos[target].y,pos[target].z,cursectnum[target]) == 1)
                l = 1;
            if (l != 0)
            {
                spawnsprite(j,sprite[i].x,sprite[i].y,sprite[i].z,sprite[i].cstat,sprite[i].shade,sprite[i].pal,
                            sprite[i].clipdist,38,38,sprite[i].xoffset,sprite[i].yoffset,sprite[i].picnum,krand()&2047,0,0,0,i,
                            sprite[i].sectnum,10,sprite[i].lotag,sprite[i].hitag,sprite[i].extra);
                switch (krand()&31) //Mutations!
                {
                case 0: sprite[i].cstat ^= 2; break;
                case 1: sprite[i].cstat ^= 512; break;
                case 2: sprite[i].shade++; break;
                case 3: sprite[i].shade--; break;
                case 4: sprite[i].pal ^= 16; break;
                case 5: case 6: case 7: sprite[i].lotag ^= (1<<(krand()&7)); break;
                case 8: sprite[i].lotag = (krand()&255); break;
                }
            }
        }
        if (k >= 208+((sprite[i].lotag&128)>>2))    //Al decides to shoot bullet
        {
            if (cansee(sprite[i].x,sprite[i].y,sprite[i].z-(tilesiz[sprite[i].picnum].y<<7),sprite[i].sectnum,pos[target].x,pos[target].y,pos[target].z,cursectnum[target]) == 1)
            {
                wsayfollow("zipguns.wav",5144L+(krand()&127)-64,256L,&sprite[i].x,&sprite[i].y,1);

                spawnsprite(j,sprite[i].x,sprite[i].y,
                            sector[sprite[i].sectnum].floorz-(24<<8),
                            0,0,0,16,32,32,0,0,BULLET,
                            (getangle(pos[target].x-sprite[j].x,
                                      pos[target].y-sprite[j].y)+(krand()&15)-8)&2047,
                            sintable[(sprite[j].ang+512)&2047]>>6,
                            sintable[sprite[j].ang&2047]>>6,
                            ((pos[target].z+(8<<8)-sprite[j].z)<<8) /
                            (ksqrt((pos[target].x-sprite[j].x) *
                                   (pos[target].x-sprite[j].x) +
                                   (pos[target].y-sprite[j].y) *
                                   (pos[target].y-sprite[j].y))+1),
                            i,sprite[i].sectnum,6,0,0,0);
            }
        }

        //Move Al
        l = (((sprite[i].lotag&3)+2)<<8);
        if (sprite[i].lotag&4) l = -l;
        dax = sintable[(sprite[i].ang+512)&2047]*l;
        day = sintable[sprite[i].ang]*l;

        osectnum = sprite[i].sectnum;
        movestat = movesprite((short)i,dax,day,0L,-(8L<<8),-(8L<<8),CLIPMASK0);
        sprite[i].z = globloz;
        if ((sprite[i].sectnum != osectnum) && (sector[sprite[i].sectnum].lotag == 10))
        {
            warpsprite((short)i);
            movestat = 0;
        }

        if (sprite[i].lotag&16)
        {
            if (((k&124) >= 120) && (cansee(sprite[i].x,sprite[i].y,sprite[i].z-(tilesiz[sprite[i].picnum].y<<7),sprite[i].sectnum,pos[target].x,pos[target].y,pos[target].z,cursectnum[target]) == 1))
                sprite[i].ang = getangle(pos[target].x-sprite[i].x,pos[target].y-sprite[i].y);
            else
                sprite[i].ang = (krand()&2047);
        }

        if (movestat != 0)
        {
            if ((k&2) && (cansee(sprite[i].x,sprite[i].y,sprite[i].z-(tilesiz[sprite[i].picnum].y<<7),sprite[i].sectnum,pos[target].x,pos[target].y,pos[target].z,cursectnum[target]) == 1))
                sprite[i].ang = getangle(pos[target].x-sprite[i].x,pos[target].y-sprite[i].y);
            else
                sprite[i].ang = (krand()&2047);

            if ((movestat&49152) == 49152)
                if (sprite[movestat&16383].picnum == EVILAL)
                    if ((k&31) >= 30)
                    {
                        wsayfollow("blowup.wav",5144L+(krand()&127)-64,256L,&sprite[i].x,&sprite[i].y,0);
                        sprite[i].picnum = EVILALGRAVE;
                        sprite[i].cstat = 0;
                        sprite[i].xvel = (krand()&255)-128;
                        sprite[i].yvel = (krand()&255)-128;
                        sprite[i].zvel = (krand()&4095)-3072;
                        changespritestat(i,9);
                    }

            if (sprite[i].lotag&8)
                if ((k&31) >= 30)
                {
                    wsayfollow("blowup.wav",5144L+(krand()&127)-64,256L,&sprite[i].x,&sprite[i].y,0);
                    sprite[i].picnum = EVILALGRAVE;
                    sprite[i].cstat = 0;
                    sprite[i].xvel = (krand()&255)-128;
                    sprite[i].yvel = (krand()&255)-128;
                    sprite[i].zvel = (krand()&4095)-3072;
                    changespritestat(i,9);
                }

            if (movestat == -1)
            {
                wsayfollow("blowup.wav",5144L+(krand()&127)-64,256L,&sprite[i].x,&sprite[i].y,0);
                sprite[i].picnum = EVILALGRAVE;
                sprite[i].cstat = 0;
                sprite[i].xvel = (krand()&255)-128;
                sprite[i].yvel = (krand()&255)-128;
                sprite[i].zvel = (krand()&4095)-3072;
                changespritestat(i,9);
            }
        }
    }

    //Go through travelling bullet sprites
    for (i=headspritestat[6]; i>=0; i=nexti)
    {
        nexti = nextspritestat[i];

        if ((nummoves-i)&statrate[6]) continue;

        //If the sprite is a bullet then...
        if ((sprite[i].picnum == BULLET) || (sprite[i].picnum == GRABBER) || (sprite[i].picnum == MISSILE) || (sprite[i].picnum == BOMB))
        {
            dax = ((((int)sprite[i].xvel)*TICSPERFRAME)<<12);
            day = ((((int)sprite[i].yvel)*TICSPERFRAME)<<12);
            daz = ((((int)sprite[i].zvel)*TICSPERFRAME)>>2);
            if (sprite[i].picnum == BOMB) daz = 0;

            osectnum = sprite[i].sectnum;
            hitobject = movesprite((short)i,dax,day,daz,4L<<8,4L<<8,CLIPMASK1);
            if ((sprite[i].sectnum != osectnum) && (sector[sprite[i].sectnum].lotag == 10))
            {
                warpsprite((short)i);
                hitobject = 0;
            }

            if (sprite[i].picnum == GRABBER)     // Andy did this (& Ken) !Homing!
            {
                checkgrabbertouchsprite(i,sprite[i].sectnum);
                l = 0x7fffffff;
                for (j = connecthead; j >= 0; j = connectpoint2[j])   // Players
                    if (j != (sprite[i].owner & (MAXSPRITES - 1)))
                        if (cansee(sprite[i].x,sprite[i].y,sprite[i].z,sprite[i].sectnum,pos[j].x,pos[j].y,pos[j].z,cursectnum[j]))
                        {
                            k = ksqrt(sqr(pos[j].x - sprite[i].x) + sqr(pos[j].y - sprite[i].y) + (sqr(pos[j].z - sprite[i].z) >> 8));
                            if (k < l)
                            {
                                l = k;
                                dax = (pos[j].x - sprite[i].x);
                                day = (pos[j].y - sprite[i].y);
                                daz = (pos[j].z - sprite[i].z);
                            }
                        }
                for (j = headspritestat[1]; j >= 0; j = nextj)    // Active monsters
                {
                    nextj = nextspritestat[j];
                    if (cansee(sprite[i].x,sprite[i].y,sprite[i].z,sprite[i].sectnum,sprite[j].x,sprite[j].y,sprite[j].z,sprite[j].sectnum))
                    {
                        k = ksqrt(sqr(sprite[j].x - sprite[i].x) + sqr(sprite[j].y - sprite[i].y) + (sqr(sprite[j].z - sprite[i].z) >> 8));
                        if (k < l)
                        {
                            l = k;
                            dax = (sprite[j].x - sprite[i].x);
                            day = (sprite[j].y - sprite[i].y);
                            daz = (sprite[j].z - sprite[i].z);
                        }
                    }
                }
                for (j = headspritestat[2]; j >= 0; j = nextj)    // Inactive monsters
                {
                    nextj = nextspritestat[j];
                    if (cansee(sprite[i].x,sprite[i].y,sprite[i].z,sprite[i].sectnum,sprite[j].x,sprite[j].y,sprite[j].z,sprite[j].sectnum))
                    {
                        k = ksqrt(sqr(sprite[j].x - sprite[i].x) + sqr(sprite[j].y - sprite[i].y) + (sqr(sprite[j].z - sprite[i].z) >> 8));
                        if (k < l)
                        {
                            l = k;
                            dax = (sprite[j].x - sprite[i].x);
                            day = (sprite[j].y - sprite[i].y);
                            daz = (sprite[j].z - sprite[i].z);
                        }
                    }
                }
                if (l != 0x7fffffff)
                {
                    sprite[i].xvel = (divscale7(dax,l) + sprite[i].xvel);   // 1/5 of velocity is homing, 4/5 is momentum
                    sprite[i].yvel = (divscale7(day,l) + sprite[i].yvel);   // 1/5 of velocity is homing, 4/5 is momentum
                    sprite[i].zvel = (divscale7(daz,l) + sprite[i].zvel);   // 1/5 of velocity is homing, 4/5 is momentum
                    l = ksqrt((sprite[i].xvel * sprite[i].xvel) + (sprite[i].yvel * sprite[i].yvel) + ((sprite[i].zvel * sprite[i].zvel) >> 8));
                    sprite[i].xvel = divscale9(sprite[i].xvel,l);
                    sprite[i].yvel = divscale9(sprite[i].yvel,l);
                    sprite[i].zvel = divscale9(sprite[i].zvel,l);
                    sprite[i].ang = getangle(sprite[i].xvel,sprite[i].yvel);
                }
            }

            if (sprite[i].picnum == BOMB)
            {
                j = sprite[i].sectnum;
                if ((sector[j].floorstat&2) && (sprite[i].z > globloz-(8<<8)))
                {
                    k = sector[j].wallptr;
                    daang = getangle(wall[wall[k].point2].x-wall[k].x,wall[wall[k].point2].y-wall[k].y);
                    sprite[i].xvel += mulscale22(sintable[(daang+1024)&2047],sector[j].floorheinum);
                    sprite[i].yvel += mulscale22(sintable[(daang+512)&2047],sector[j].floorheinum);
                }
            }

            if (sprite[i].picnum == BOMB)
            {
                sprite[i].z += sprite[i].zvel;
                sprite[i].zvel += (TICSPERFRAME<<7);
                if (sprite[i].z < globhiz+(tilesiz[BOMB].y<<6))
                {
                    sprite[i].z = globhiz+(tilesiz[BOMB].y<<6);
                    sprite[i].zvel = -(sprite[i].zvel>>1);
                }
                if (sprite[i].z > globloz-(tilesiz[BOMB].y<<6))
                {
                    sprite[i].z = globloz-(tilesiz[BOMB].y<<6);
                    sprite[i].zvel = -(sprite[i].zvel>>1);
                }
                dax = sprite[i].xvel; day = sprite[i].yvel;
                dist = dax*dax+day*day;
                if (dist < 512)
                {
                    bombexplode(i);
                    goto bulletisdeletedskip;
                }
                if (dist < 4096)
                {
                    sprite[i].xrepeat = ((4096+2048)*16) / (dist+2048);
                    sprite[i].yrepeat = sprite[i].xrepeat;
                    sprite[i].xoffset = (krand()&15)-8;
                    sprite[i].yoffset = (krand()&15)-8;
                }
                if (mulscale30(krand(),dist) == 0)
                {
                    sprite[i].xvel -= ksgn(sprite[i].xvel);
                    sprite[i].yvel -= ksgn(sprite[i].yvel);
                    sprite[i].zvel -= ksgn(sprite[i].zvel);
                }
            }

            //Check for bouncy objects before killing bullet
            if ((hitobject&0xc000) == 16384)  //Bullet hit a ceiling/floor
            {
                k = sector[hitobject&(MAXSECTORS-1)].wallptr; l = wall[k].point2;
                daang = getangle(wall[l].x-wall[k].x,wall[l].y-wall[k].y);

                getzsofslope(hitobject&(MAXSECTORS-1),sprite[i].x,sprite[i].y,&k,&l);
                if (sprite[i].z < ((k+l)>>1)) k = sector[hitobject&(MAXSECTORS-1)].ceilingheinum;
                else k = sector[hitobject&(MAXSECTORS-1)].floorheinum;

                dax = mulscale14(k,sintable[(daang)&2047]);
                day = mulscale14(k,sintable[(daang+1536)&2047]);
                daz = 4096;

                k = sprite[i].xvel*dax+sprite[i].yvel*day+mulscale4(sprite[i].zvel,daz);
                l = dax*dax+day*day+daz*daz;
                if ((klabs(k)>>14) < l)
                {
                    k = divscale17(k,l);
                    sprite[i].xvel -= mulscale16(dax,k);
                    sprite[i].yvel -= mulscale16(day,k);
                    sprite[i].zvel -= mulscale12(daz,k);
                }
                wsayfollow("bouncy.wav",4096L+(krand()&127)-64,255,&sprite[i].x,&sprite[i].y,1);
                hitobject = 0;
                sprite[i].owner = -1;   //Bullet turns evil!
            }
            else if ((hitobject&0xc000) == 32768)  //Bullet hit a wall
            {
                if (wall[hitobject&4095].lotag == 8)
                {
                    dax = sprite[i].xvel; day = sprite[i].yvel;
                    if ((sprite[i].picnum != BOMB) || (dax*dax+day*day >= 512))
                    {
                        k = (hitobject&4095); l = wall[k].point2;
                        j = getangle(wall[l].x-wall[k].x,wall[l].y-wall[k].y)+512;

                        //k = cos(ang) * sin(ang) * 2
                        k = mulscale13(sintable[(j+512)&2047],sintable[j&2047]);
                        //l = cos(ang * 2)
                        l = sintable[((j<<1)+512)&2047];

                        ox = sprite[i].xvel; oy = sprite[i].yvel;
                        dax = -ox; day = -oy;
                        sprite[i].xvel = dmulscale14(day,k,dax,l);
                        sprite[i].yvel = dmulscale14(dax,k,-day,l);

                        if (sprite[i].picnum == BOMB)
                        {
                            sprite[i].xvel -= (sprite[i].xvel>>3);
                            sprite[i].yvel -= (sprite[i].yvel>>3);
                            sprite[i].zvel -= (sprite[i].zvel>>3);
                        }
                        ox -= sprite[i].xvel; oy -= sprite[i].yvel;
                        dist = ((ox*ox+oy*oy)>>8);
                        wsayfollow("bouncy.wav",4096L+(krand()&127)-64,min(dist,256),&sprite[i].x,&sprite[i].y,1);
                        hitobject = 0;
                        sprite[i].owner = -1;   //Bullet turns evil!
                    }
                }
            }
            else if ((hitobject&0xc000) == 49152)  //Bullet hit a sprite
            {
                if (sprite[hitobject&4095].picnum == BOUNCYMAT)
                {
                    if ((sprite[hitobject&4095].cstat&48) == 0)
                    {
                        sprite[i].xvel = -sprite[i].xvel;
                        sprite[i].yvel = -sprite[i].yvel;
                        sprite[i].zvel = -sprite[i].zvel;
                        dist = 255;
                    }
                    else if ((sprite[hitobject&4095].cstat&48) == 16)
                    {
                        j = sprite[hitobject&4095].ang;

                        //k = cos(ang) * sin(ang) * 2
                        k = mulscale13(sintable[(j+512)&2047],sintable[j&2047]);
                        //l = cos(ang * 2)
                        l = sintable[((j<<1)+512)&2047];

                        ox = sprite[i].xvel; oy = sprite[i].yvel;
                        dax = -ox; day = -oy;
                        sprite[i].xvel = dmulscale14(day,k,dax,l);
                        sprite[i].yvel = dmulscale14(dax,k,-day,l);

                        ox -= sprite[i].xvel; oy -= sprite[i].yvel;
                        dist = ((ox*ox+oy*oy)>>8);
                    }
                    sprite[i].owner = -1;   //Bullet turns evil!
                    wsayfollow("bouncy.wav",4096L+(krand()&127)-64,min(dist,256),&sprite[i].x,&sprite[i].y,1);
                    hitobject = 0;
                }
            }

            if (hitobject != 0)
            {
                if ((sprite[i].picnum == MISSILE) || (sprite[i].picnum == BOMB))
                {
                    if ((hitobject&0xc000) == 49152)
                        if (sprite[hitobject&4095].lotag == 5)  //Basketball hoop
                        {
                            wsayfollow("niceshot.wav",3840L+(krand()&127)-64,256L,&sprite[i].x,&sprite[i].y,0);
                            deletesprite((short)i);
                            goto bulletisdeletedskip;
                        }

                    bombexplode(i);
                    goto bulletisdeletedskip;
                }

                if ((hitobject&0xc000) == 16384)  //Hits a ceiling / floor
                {
                    wsayfollow("bullseye.wav",4096L+(krand()&127)-64,256L,&sprite[i].x,&sprite[i].y,0);
                    deletesprite((short)i);
                    goto bulletisdeletedskip;
                }
                else if ((hitobject&0xc000) == 32768)  //Bullet hit a wall
                {
                    if (wall[hitobject&4095].picnum == KENPICTURE)
                    {
                        if (waloff[MAXTILES-1] != 0)
                            wall[hitobject&4095].picnum = MAXTILES-1;
                        wsayfollow("hello.wav",4096L+(krand()&127)-64,256L,&sprite[i].x,&sprite[i].y,0);   //Ken says, "Hello... how are you today!"
                    }
                    else
                        wsayfollow("bullseye.wav",4096L+(krand()&127)-64,256L,&sprite[i].x,&sprite[i].y,0);

                    deletesprite((short)i);
                    goto bulletisdeletedskip;
                }
                else if ((hitobject&0xc000) == 49152)  //Bullet hit a sprite
                {
                    if ((sprite[hitobject&4095].lotag == 5) && (sprite[i].picnum == GRABBER))    // Basketball hoop (Andy's addition)
                    {
                        wsayfollow("niceshot.wav",3840L+(krand()&127)-64,256L,&sprite[i].x,&sprite[i].y,0);
                        switch (krand() & 63)
                        {
                        case 0: case 1: case 2: case 3: case 4: case 5: case 6: case 7: case 8: case 9:
                            sprite[i].picnum = COIN; break;
                        case 10: case 11: case 12: case 13: case 14: case 15: case 16:
                            sprite[i].picnum = DIAMONDS; break;
                        case 17: case 18: case 19:
                            sprite[i].picnum = COINSTACK; break;
                        case 20: case 21: case 22: case 23:
                            sprite[i].picnum = GIFTBOX; break;
                        case 24: case 25:
                            sprite[i].picnum = GRABCANNON; break;
                        case 26: case 27:
                            sprite[i].picnum = LAUNCHER; break;
                        case 28: case 29: case 30:
                            sprite[i].picnum = CANNON; break;
                        case 31:
                            sprite[i].picnum = AIRPLANE; break;
                        default:
                            deletesprite((short)i);
                            goto bulletisdeletedskip;
                        }
                        sprite[i].xvel = sprite[i].yvel = sprite[i].zvel = 0;
                        sprite[i].cstat &= ~0x83;    //Should not clip, foot-z
                        changespritestat(i,12);
                        goto bulletisdeletedskip;
                    }

                    //Check if bullet hit a player & find which player it was...
                    if (sprite[hitobject&4095].picnum == PLAYER)
                        for (j=connecthead; j>=0; j=connectpoint2[j])
                            if (sprite[i].owner != j+4096)
                                if (playersprite[j] == (hitobject&4095))
                                {
                                    wsayfollow("ouch.wav",4096L+(krand()&127)-64,256L,&sprite[i].x,&sprite[i].y,0);
                                    if (sprite[i].picnum == GRABBER)     // Andy did this
                                    {
                                        k = ((sprite[i].xrepeat * sprite[i].yrepeat) * 3) >> 9;
                                        changehealth((sprite[i].owner - 4096),k);
                                        changehealth(j,-k);
                                    }
                                    else changehealth(j,-mulscale8(sprite[i].xrepeat,sprite[i].yrepeat));
                                    deletesprite((short)i);
                                    goto bulletisdeletedskip;
                                }

                    //Check if bullet hit any monsters...
                    j = (hitobject&4095);     //j is the spritenum that the bullet (spritenum i) hit
                    if (sprite[i].owner != j)
                    {
                        switch (sprite[j].picnum)
                        {
                        case BROWNMONSTER:
                            if (sprite[j].lotag > 0)
                            {
                                if (sprite[i].picnum == GRABBER)     // Andy did this
                                {
                                    k = ((sprite[i].xrepeat * sprite[i].yrepeat) * 3) >> 9;
                                    changehealth((sprite[i].owner - 4096),k);
                                    sprite[j].lotag -= k;
                                }
                                sprite[j].lotag -= mulscale8(sprite[i].xrepeat,sprite[i].yrepeat);
                            }
                            if (sprite[j].lotag > 0)
                            {
                                if (sprite[j].lotag <= 25) sprite[j].cstat |= 2;
                                wsayfollow("hurt.wav",4096L+(krand()&511)-256,256L,&sprite[i].x,&sprite[i].y,1);
                            }
                            else
                            {
                                wsayfollow("mondie.wav",4096L+(krand()&127)-64,256L,&sprite[i].x,&sprite[i].y,0);
                                sprite[j].z += ((tilesiz[sprite[j].picnum].y*sprite[j].yrepeat)<<1);
                                sprite[j].picnum = GIFTBOX;
                                sprite[j].cstat &= ~0x83;    //Should not clip, foot-z

                                spawnsprite(k,sprite[j].x,sprite[j].y,sprite[j].z,
                                            0,-4,0,32,64,64,0,0,EXPLOSION,sprite[j].ang,
                                            0,0,0,j,sprite[j].sectnum,5,31,0,0);
                                //31=Time left for explosion to stay

                                changespritestat(j,12);
                            }
                            deletesprite((short)i);
                            goto bulletisdeletedskip;
                        case EVILAL:
                            wsayfollow("blowup.wav",5144L+(krand()&127)-64,256L,&sprite[i].x,&sprite[i].y,0);
                            sprite[j].picnum = EVILALGRAVE;
                            sprite[j].cstat = 0;
                            sprite[j].xvel = (krand()&255)-128;
                            sprite[j].yvel = (krand()&255)-128;
                            sprite[j].zvel = (krand()&4095)-3072;
                            changespritestat(j,9);

                            deletesprite((short)i);
                            goto bulletisdeletedskip;
                        case AL:
                            wsayfollow("blowup.wav",5144L+(krand()&127)-64,256L,&sprite[i].x,&sprite[i].y,0);
                            sprite[j].xrepeat += 2;
                            sprite[j].yrepeat += 2;
                            if (sprite[j].yrepeat >= 38)
                            {
                                sprite[j].picnum = EVILAL;
                                //sprite[j].cstat |= 2;      //Make him transluscent
                                changespritestat(j,10);
                            }
                            deletesprite((short)i);
                            goto bulletisdeletedskip;
                        default:
                            wsayfollow("bullseye.wav",4096L+(krand()&127)-64,256L,&sprite[i].x,&sprite[i].y,0);
                            deletesprite((short)i);
                            goto bulletisdeletedskip;
                        }
                    }
                }
            }
        }
bulletisdeletedskip: continue;
    }

    //Go through monster waiting for you list
    for (i=headspritestat[2]; i>=0; i=nexti)
    {
        nexti = nextspritestat[i];

        if ((nummoves-i)&15) continue;

        //Use dot product to see if monster's angle is towards a player
        for (p=connecthead; p>=0; p=connectpoint2[p])
            if (sintable[(sprite[i].ang+512)&2047]*(pos[p].x-sprite[i].x) + sintable[sprite[i].ang&2047]*(pos[p].y-sprite[i].y) >= 0)
                if (cansee(sprite[i].x,sprite[i].y,sprite[i].z-(tilesiz[sprite[i].picnum].y<<7),sprite[i].sectnum,pos[p].x,pos[p].y,pos[p].z,cursectnum[p]) == 1)
                {
                    changespritestat(i,1);
                    //if (sprite[i].lotag == 100)
                    //{
                    wsayfollow("iseeyou.wav",4096L+(krand()&127)-64,256L,&sprite[i].x,&sprite[i].y,1);
                    //   sprite[i].lotag = 99;
                    //}
                }
    }

    //Go through smoke sprites
    for (i=headspritestat[3]; i>=0; i=nexti)
    {
        nexti = nextspritestat[i];

        sprite[i].z -= (TICSPERFRAME<<6);
        sprite[i].lotag -= TICSPERFRAME;
        if ((int16_t)sprite[i].lotag < 0) deletesprite(i);
    }

    //Go through splash sprites
    for (i=headspritestat[4]; i>=0; i=nexti)
    {
        nexti = nextspritestat[i];

        sprite[i].lotag -= TICSPERFRAME;
        sprite[i].picnum = SPLASH + ((63-sprite[i].lotag)>>4);
        if ((int16_t)sprite[i].lotag < 0) deletesprite(i);
    }

    //Go through explosion sprites
    for (i=headspritestat[5]; i>=0; i=nexti)
    {
        nexti = nextspritestat[i];

        sprite[i].lotag -= TICSPERFRAME;
        if ((int16_t)sprite[i].lotag < 0) deletesprite(i);
    }

    //Go through bomb spriral-explosion sprites
    for (i=headspritestat[7]; i>=0; i=nexti)
    {
        nexti = nextspritestat[i];

        sprite[i].xrepeat = (sprite[i].lotag>>2);
        sprite[i].yrepeat = (sprite[i].lotag>>2);
        sprite[i].lotag -= (TICSPERFRAME<<2);
        if ((int16_t)sprite[i].lotag < 0) { deletesprite(i); continue; }

        if ((nummoves-i)&statrate[7]) continue;

        sprite[i].x += ((sprite[i].xvel*TICSPERFRAME)>>2);
        sprite[i].y += ((sprite[i].yvel*TICSPERFRAME)>>2);
        sprite[i].z += ((sprite[i].zvel*TICSPERFRAME)>>2);

        sprite[i].zvel += (TICSPERFRAME<<9);
        if (sprite[i].z < sector[sprite[i].sectnum].ceilingz+(4<<8))
        {
            sprite[i].z = sector[sprite[i].sectnum].ceilingz+(4<<8);
            sprite[i].zvel = -(sprite[i].zvel>>1);
        }
        if (sprite[i].z > sector[sprite[i].sectnum].floorz-(4<<8))
        {
            sprite[i].z = sector[sprite[i].sectnum].floorz-(4<<8);
            sprite[i].zvel = -(sprite[i].zvel>>1);
        }
    }

    //EVILALGRAVE shrinking list
    for (i=headspritestat[9]; i>=0; i=nexti)
    {
        nexti = nextspritestat[i];

        sprite[i].xrepeat = (sprite[i].lotag>>2);
        sprite[i].yrepeat = (sprite[i].lotag>>2);
        sprite[i].lotag -= TICSPERFRAME;
        if ((int16_t)sprite[i].lotag < 0) { deletesprite(i); continue; }

        if ((nummoves-i)&statrate[9]) continue;

        sprite[i].x += (sprite[i].xvel*TICSPERFRAME);
        sprite[i].y += (sprite[i].yvel*TICSPERFRAME);
        sprite[i].z += (sprite[i].zvel*TICSPERFRAME);

        sprite[i].zvel += (TICSPERFRAME<<8);
        if (sprite[i].z < sector[sprite[i].sectnum].ceilingz)
        {
            sprite[i].z = sector[sprite[i].sectnum].ceilingz;
            sprite[i].xvel -= (sprite[i].xvel>>2);
            sprite[i].yvel -= (sprite[i].yvel>>2);
            sprite[i].zvel = -(sprite[i].zvel>>1);
        }
        if (sprite[i].z > sector[sprite[i].sectnum].floorz)
        {
            sprite[i].z = sector[sprite[i].sectnum].floorz;
            sprite[i].xvel -= (sprite[i].xvel>>2);
            sprite[i].yvel -= (sprite[i].yvel>>2);
            sprite[i].zvel = -(sprite[i].zvel>>1);
        }
    }

    //Re-spawning sprite list
    for (i=headspritestat[11]; i>=0; i=nexti)
    {
        nexti = nextspritestat[i];

        sprite[i].extra -= TICSPERFRAME;
        if (sprite[i].extra < 0)
        {
            wsayfollow("warp.wav",6144L+(krand()&127)-64,128L,&sprite[i].x,&sprite[i].y,0);
            sprite[i].cstat &= (uint16_t) ~0x8000;
            sprite[i].extra = -1;
            changespritestat((short)i,0);
        }
    }
}

void activatehitag(short dahitag)
{
    int i, nexti;

    for (i=0; i<numsectors; i++)
        if (sector[i].hitag == dahitag) operatesector(i);

    for (i=headspritestat[0]; i>=0; i=nexti)
    {
        nexti = nextspritestat[i];
        if (sprite[i].hitag == dahitag) operatesprite(i);
    }
}

void bombexplode(int i)
{
    int j, nextj, k, daang, dax, day, dist;

    spawnsprite(j,sprite[i].x,sprite[i].y,sprite[i].z,0,-4,0,
                32,64,64,0,0,EXPLOSION,sprite[i].ang,
                0,0,0,sprite[i].owner,sprite[i].sectnum,5,31,0,0);
    //31=Time left for explosion to stay

    for (k=0; k<12; k++)
    {
        spawnsprite(j,sprite[i].x,sprite[i].y,sprite[i].z+(8<<8),2,-4,0,
                    32,24,24,0,0,EXPLOSION,sprite[i].ang,
                    (krand()>>7)-256,(krand()>>7)-256,(krand()>>2)-8192,
                    sprite[i].owner,sprite[i].sectnum,7,96,0,0);
        //96=Time left for smoke to be alive
    }

    for (j=connecthead; j>=0; j=connectpoint2[j])
    {
        dist = (pos[j].x-sprite[i].x)*(pos[j].x-sprite[i].x);
        dist += (pos[j].y-sprite[i].y)*(pos[j].y-sprite[i].y);
        dist += ((pos[j].z-sprite[i].z)>>4)*((pos[j].z-sprite[i].z)>>4);
        if (dist < 4194304)
            if (cansee(sprite[i].x,sprite[i].y,sprite[i].z-(tilesiz[sprite[i].picnum].y<<7),sprite[i].sectnum,pos[j].x,pos[j].y,pos[j].z,cursectnum[j]) == 1)
            {
                k = ((32768/((dist>>16)+4))>>5);
                if (j == myconnectindex)
                {
                    daang = getangle(pos[j].x-sprite[i].x,pos[j].y-sprite[i].y);
                    dax = ((k*sintable[(daang+512)&2047])>>14);
                    day = ((k*sintable[daang&2047])>>14);
                    fvel += ((dax*sintable[(ang[j]+512)&2047]+day*sintable[ang[j]&2047])>>14);
                    svel += ((day*sintable[(ang[j]+512)&2047]-dax*sintable[ang[j]&2047])>>14);
                }
                changehealth(j,-k);    //if changehealth returns 1, you're dead
            }
    }

    for (k=1; k<=2; k++)      //Check for hurting monsters
    {
        for (j=headspritestat[k]; j>=0; j=nextj)
        {
            nextj = nextspritestat[j];

            dist = (sprite[j].x-sprite[i].x)*(sprite[j].x-sprite[i].x);
            dist += (sprite[j].y-sprite[i].y)*(sprite[j].y-sprite[i].y);
            dist += ((sprite[j].z-sprite[i].z)>>4)*((sprite[j].z-sprite[i].z)>>4);
            if (dist >= 4194304) continue;
            if (cansee(sprite[i].x,sprite[i].y,sprite[i].z-(tilesiz[sprite[i].picnum].y<<7),sprite[i].sectnum,sprite[j].x,sprite[j].y,sprite[j].z-(tilesiz[sprite[j].picnum].y<<7),sprite[j].sectnum) == 0)
                continue;
            if (sprite[j].picnum == BROWNMONSTER)
            {
                sprite[j].z += ((tilesiz[sprite[j].picnum].y*sprite[j].yrepeat)<<1);
                sprite[j].picnum = GIFTBOX;
                sprite[j].cstat &= ~0x83;    //Should not clip, foot-z
                changespritestat(j,12);
            }
        }
    }

    for (j=headspritestat[10]; j>=0; j=nextj) //Check for EVILAL's
    {
        nextj = nextspritestat[j];

        dist = (sprite[j].x-sprite[i].x)*(sprite[j].x-sprite[i].x);
        dist += (sprite[j].y-sprite[i].y)*(sprite[j].y-sprite[i].y);
        dist += ((sprite[j].z-sprite[i].z)>>4)*((sprite[j].z-sprite[i].z)>>4);
        if (dist >= 4194304) continue;
        if (cansee(sprite[i].x,sprite[i].y,sprite[i].z-(tilesiz[sprite[i].picnum].y<<7),sprite[i].sectnum,sprite[j].x,sprite[j].y,sprite[j].z-(tilesiz[sprite[j].picnum].y<<7),sprite[j].sectnum) == 0)
            continue;

        sprite[j].picnum = EVILALGRAVE;
        sprite[j].cstat = 0;
        sprite[j].xvel = (krand()&255)-128;
        sprite[j].yvel = (krand()&255)-128;
        sprite[j].zvel = (krand()&4095)-3072;
        changespritestat(j,9);
    }

    wsayfollow("blowup.wav",3840L+(krand()&127)-64,256L,&sprite[i].x,&sprite[i].y,0);
    deletesprite((short)i);
}

void processinput(short snum)
{
    // int oldposx, oldposy, nexti;
    int i, j, k, doubvel, xvect, yvect, goalz;
    int dax, day /*, dax2, day2, odax, oday, odax2, oday2*/;
    // short startwall, endwall;
    // char *ptr;

    //SHARED KEYS:
    //Movement code
    if ((ssync[snum].fvel|ssync[snum].svel) != 0)
    {
        doubvel = (TICSPERFRAME<<((ssync[snum].bits&256)>0));

        xvect = 0, yvect = 0;
        if (ssync[snum].fvel != 0)
        {
            xvect += ((((int)ssync[snum].fvel)*doubvel*(int)sintable[(ang[snum]+512)&2047])>>3);
            yvect += ((((int)ssync[snum].fvel)*doubvel*(int)sintable[ang[snum]&2047])>>3);
        }
        if (ssync[snum].svel != 0)
        {
            xvect += ((((int)ssync[snum].svel)*doubvel*(int)sintable[ang[snum]&2047])>>3);
            yvect += ((((int)ssync[snum].svel)*doubvel*(int)sintable[(ang[snum]+1536)&2047])>>3);
        }
        if (flytime[snum] > lockclock) { xvect += xvect; yvect += yvect; }   // DOuble flying speed
        clipmove(&pos[snum],&cursectnum[snum],xvect,yvect,128L,4<<8,4<<8,CLIPMASK0);
        revolvedoorstat[snum] = 1;
    }
    else
    {
        revolvedoorstat[snum] = 0;
    }

    sprite[playersprite[snum]].cstat &= ~1;
    //Push player away from walls if clipmove doesn't work
    if (pushmove(&pos[snum],&cursectnum[snum],128L,4<<8,4<<8,CLIPMASK0) < 0)
        changehealth(snum,-1000);  //If this screws up, then instant death!!!

    // Getzrange returns the highest and lowest z's for an entire box,
    // NOT just a point.  This prevents you from falling off cliffs
    // when you step only slightly over the cliff.
    getzrange(&pos[snum],cursectnum[snum],&globhiz,&globhihit,&globloz,&globlohit,128L,CLIPMASK0);
    sprite[playersprite[snum]].cstat |= 1;

    if (ssync[snum].avel != 0)          //ang += avel * constant
    {
        //ENGINE calculates avel for you
        doubvel = TICSPERFRAME;
        if ((ssync[snum].bits&256) > 0)  //Lt. shift makes turn velocity 50% faster
            doubvel += (TICSPERFRAME>>1);
        ang[snum] += ((((int)ssync[snum].avel)*doubvel)>>4);
        ang[snum] &= 2047;
    }

    if (health[snum] < 0)
    {
        health[snum] -= TICSPERFRAME;
        if (health[snum] <= -160)
        {
            hvel[snum] = 0;
            if (snum == myconnectindex)
                fvel = 0, svel = 0, avel = 0, keystatus[3] = 1;

            deaths[snum]++;
            health[snum] = 100;
            numbombs[snum] = 0;
            numgrabbers[snum] = 0;
            nummissiles[snum] = 0;
            flytime[snum] = 0;

            findrandomspot(&pos[snum].x,&pos[snum].y,&cursectnum[snum]);
            pos[snum].z = getflorzofslope(cursectnum[snum],pos[snum].x,pos[snum].y)-(1<<8);
            horiz[snum] = 100;
            ang[snum] = (krand()&2047);

            sprite[playersprite[snum]].x = pos[snum].x;
            sprite[playersprite[snum]].y = pos[snum].y;
            sprite[playersprite[snum]].z = pos[snum].z+EYEHEIGHT;
            sprite[playersprite[snum]].picnum = PLAYER;
            sprite[playersprite[snum]].ang = ang[snum];
            sprite[playersprite[snum]].xrepeat = 64;
            sprite[playersprite[snum]].yrepeat = 64;
            changespritesect(playersprite[snum],cursectnum[snum]);

            drawstatusbar(snum);   // Andy did this

            i = playersprite[snum];
            wsayfollow("zipguns.wav",4096L+(krand()&127)-64,256L,&sprite[i].x,&sprite[i].y,1);
            for (k=0; k<16; k++)
            {
                spawnsprite(j,sprite[i].x,sprite[i].y,sprite[i].z+(8<<8),2,-4,0,
                            32,24,24,0,0,EXPLOSION,sprite[i].ang,
                            (krand()&511)-256,(krand()&511)-256,(krand()&16384)-8192,
                            sprite[i].owner,sprite[i].sectnum,7,96,0,0);
                //96=Time left for smoke to be alive
            }
        }
        else
        {
            sprite[playersprite[snum]].xrepeat = max(((128+health[snum])>>1),0);
            sprite[playersprite[snum]].yrepeat = max(((128+health[snum])>>1),0);

            hvel[snum] += (TICSPERFRAME<<2);
            horiz[snum] = max(horiz[snum]-4,0);
            pos[snum].z += hvel[snum];
            if (pos[snum].z > globloz-(4<<8))
            {
                pos[snum].z = globloz-(4<<8);
                horiz[snum] = min(horiz[snum]+5,200);
                hvel[snum] = 0;
            }
        }
    }

    if (((ssync[snum].bits&8) > 0) && (horiz[snum] > 100-(200>>1))) horiz[snum] -= 4;     //-
    if (((ssync[snum].bits&4) > 0) && (horiz[snum] < 100+(200>>1))) horiz[snum] += 4;   //+

    goalz = globloz-EYEHEIGHT;
    if (sector[cursectnum[snum]].lotag == 4)   //slime sector
        if ((globlohit&0xc000) != 49152)            //You're not on a sprite
        {
            goalz = globloz-(8<<8);
            if (pos[snum].z >= goalz-(2<<8))
            {
                clipmove(&pos[snum],&cursectnum[snum],-(TICSPERFRAME<<14),-(TICSPERFRAME<<14),128L,4<<8,4<<8,CLIPMASK0);

                if (slimesoundcnt[snum] >= 0)
                {
                    slimesoundcnt[snum] -= TICSPERFRAME;
                    while (slimesoundcnt[snum] < 0)
                    {
                        slimesoundcnt[snum] += 120;
                        wsayfollow("slime.wav",4096L+(krand()&127)-64,256L,&pos[snum].x,&pos[snum].y,1);
                    }
                }
            }
        }
    if (goalz < globhiz+(16<<8))   //ceiling&floor too close
        goalz = ((globloz+globhiz)>>1);
    //goalz += mousz;
    if (health[snum] >= 0)
    {
        if ((ssync[snum].bits&1) > 0)                         //A (stand high)
        {
            if (flytime[snum] <= lockclock)
            {
                if (pos[snum].z >= globloz-(32<<8))
                {
                    goalz -= (16<<8);
                    if (ssync[snum].bits&256) goalz -= (24<<8);
                }
            }
            else
            {
                hvel[snum] -= 192;
                if (ssync[snum].bits&256) hvel[snum] -= 192;
            }
        }
        if ((ssync[snum].bits&2) > 0)                         //Z (stand low)
        {
            if (flytime[snum] <= lockclock)
            {
                goalz += (12<<8);
                if (ssync[snum].bits&256) goalz += (12<<8);
            }
            else
            {
                hvel[snum] += 192;
                if (ssync[snum].bits&256) hvel[snum] += 192;
            }
        }
    }

    if (flytime[snum] <= lockclock)
    {
        if (pos[snum].z < goalz)
            hvel[snum] += (TICSPERFRAME<<4);
        else
            hvel[snum] = (((goalz-pos[snum].z)*TICSPERFRAME)>>5);
    }
    else
    {
        hvel[snum] -= (hvel[snum]>>2);
        hvel[snum] -= ksgn(hvel[snum]);
    }

    pos[snum].z += hvel[snum];
    if (pos[snum].z > globloz-(4<<8)) pos[snum].z = globloz-(4<<8), hvel[snum] = 0;
    if (pos[snum].z < globhiz+(4<<8)) pos[snum].z = globhiz+(4<<8), hvel[snum] = 0;

    if (dimensionmode[snum] != 3)
    {
        if (((ssync[snum].bits&32) > 0) && (zoom[snum] > 48)) zoom[snum] -= (zoom[snum]>>4);
        if (((ssync[snum].bits&16) > 0) && (zoom[snum] < 4096)) zoom[snum] += (zoom[snum]>>4);
    }

    //Update sprite representation of player
    //   -should be after movement, but before shooting code
    setsprite_eyeheight(playersprite[snum],&pos[snum]);
    sprite[playersprite[snum]].ang = ang[snum];

    if (health[snum] >= 0)
    {
        if ((cursectnum[snum] < 0) || (cursectnum[snum] >= numsectors))
        {
            //How did you get in the wrong sector?
            wsayfollow("ouch.wav",4096L+(krand()&127)-64,64L,&pos[snum].x,&pos[snum].y,1);
            changehealth(snum,-TICSPERFRAME);
        }
        else if (globhiz+(8<<8) > globloz)
        {
            //Ceiling and floor are smooshing you!
            wsayfollow("ouch.wav",4096L+(krand()&127)-64,64L,&pos[snum].x,&pos[snum].y,1);
            changehealth(snum,-TICSPERFRAME);
        }
    }

    if ((waterfountainwall[snum] >= 0) && (health[snum] >= 0))
        if ((wall[neartagwall].lotag != 7) || ((ssync[snum].bits&1024) == 0))
        {
            i = waterfountainwall[snum];
            if (wall[i].overpicnum == USEWATERFOUNTAIN)
                wall[i].overpicnum = WATERFOUNTAIN;
            else if (wall[i].picnum == USEWATERFOUNTAIN)
                wall[i].picnum = WATERFOUNTAIN;

            waterfountainwall[snum] = -1;
        }

    if ((ssync[snum].bits&1024) > 0)  //Space bar
    {
        //Continuous triggers...

        neartag(pos[snum].x,pos[snum].y,pos[snum].z,cursectnum[snum],ang[snum],&neartagsector,&neartagwall,&neartagsprite,&neartaghitdist,1024L,3,NULL);
        if (neartagsector == -1)
        {
            i = cursectnum[snum];
            if ((sector[i].lotag|sector[i].hitag) != 0)
                neartagsector = i;
        }

        if (wall[neartagwall].lotag == 7)  //Water fountain
        {
            if (wall[neartagwall].overpicnum == WATERFOUNTAIN)
            {
                wsayfollow("water.wav",4096L+(krand()&127)-64,256L,&pos[snum].x,&pos[snum].y,1);
                wall[neartagwall].overpicnum = USEWATERFOUNTAIN;
                waterfountainwall[snum] = neartagwall;
            }
            else if (wall[neartagwall].picnum == WATERFOUNTAIN)
            {
                wsayfollow("water.wav",4096L+(krand()&127)-64,256L,&pos[snum].x,&pos[snum].y,1);
                wall[neartagwall].picnum = USEWATERFOUNTAIN;
                waterfountainwall[snum] = neartagwall;
            }

            if (waterfountainwall[snum] >= 0)
            {
                waterfountaincnt[snum] -= TICSPERFRAME;
                while (waterfountaincnt[snum] < 0)
                {
                    waterfountaincnt[snum] += 120;
                    wsayfollow("water.wav",4096L+(krand()&127)-64,256L,&pos[snum].x,&pos[snum].y,1);
                    changehealth(snum,2);
                }
            }
        }

        //1-time triggers...
        if ((oflags[snum]&1024) == 0)
        {
            if (neartagsector >= 0)
                if (sector[neartagsector].hitag == 0)
                    operatesector(neartagsector);

            if (neartagwall >= 0)
                if (wall[neartagwall].lotag == 2)  //Switch
                {
                    activatehitag(wall[neartagwall].hitag);

                    j = wall[neartagwall].overpicnum;
                    if (j == SWITCH1ON)                     //1-time switch
                    {
                        wall[neartagwall].overpicnum = GIFTBOX;
                        wall[neartagwall].lotag = 0;
                        wall[neartagwall].hitag = 0;
                    }
                    if (j == GIFTBOX)                       //1-time switch
                    {
                        wall[neartagwall].overpicnum = SWITCH1ON;
                        wall[neartagwall].lotag = 0;
                        wall[neartagwall].hitag = 0;
                    }
                    if (j == SWITCH2ON) wall[neartagwall].overpicnum = SWITCH2OFF;
                    if (j == SWITCH2OFF) wall[neartagwall].overpicnum = SWITCH2ON;
                    if (j == SWITCH3ON) wall[neartagwall].overpicnum = SWITCH3OFF;
                    if (j == SWITCH3OFF) wall[neartagwall].overpicnum = SWITCH3ON;

                    i = wall[neartagwall].point2;
                    dax = ((wall[neartagwall].x+wall[i].x)>>1);
                    day = ((wall[neartagwall].y+wall[i].y)>>1);
                    wsayfollow("switch.wav",4096L+(krand()&255)-128,256L,&dax,&day,0);
                }

            if (neartagsprite >= 0)
            {
                if (sprite[neartagsprite].lotag == 1)
                {
                    //if you're shoving innocent little AL around, he gets mad!
                    if (sprite[neartagsprite].picnum == AL)
                    {
                        sprite[neartagsprite].picnum = EVILAL;
                        sprite[neartagsprite].cstat |= 2;   //Make him transluscent
                        sprite[neartagsprite].xrepeat = 38;
                        sprite[neartagsprite].yrepeat = 38;
                        changespritestat(neartagsprite,10);
                    }
                }
                if (sprite[neartagsprite].lotag == 4)
                {
                    activatehitag(sprite[neartagsprite].hitag);

                    j = sprite[neartagsprite].picnum;
                    if (j == SWITCH1ON)                     //1-time switch
                    {
                        sprite[neartagsprite].picnum = GIFTBOX;
                        sprite[neartagsprite].lotag = 0;
                        sprite[neartagsprite].hitag = 0;
                    }
                    if (j == GIFTBOX)                       //1-time switch
                    {
                        sprite[neartagsprite].picnum = SWITCH1ON;
                        sprite[neartagsprite].lotag = 0;
                        sprite[neartagsprite].hitag = 0;
                    }
                    if (j == SWITCH2ON) sprite[neartagsprite].picnum = SWITCH2OFF;
                    if (j == SWITCH2OFF) sprite[neartagsprite].picnum = SWITCH2ON;
                    if (j == SWITCH3ON) sprite[neartagsprite].picnum = SWITCH3OFF;
                    if (j == SWITCH3OFF) sprite[neartagsprite].picnum = SWITCH3ON;

                    dax = sprite[neartagsprite].x;
                    day = sprite[neartagsprite].y;
                    wsayfollow("switch.wav",4096L+(krand()&255)-128,256L,&dax,&day,0);
                }
            }
        }
    }

    if ((ssync[snum].bits & 2048) > 0)     // Shoot a bullet
    {
        if ((numbombs[snum] == 0) && (((ssync[snum].bits >> 13) & 7) == 2) && (myconnectindex == snum))
            locselectedgun = 0;
        if ((nummissiles[snum] == 0) && (((ssync[snum].bits >> 13) & 7) == 3) && (myconnectindex == snum))
            locselectedgun = 1;
        if ((numgrabbers[snum] == 0) && (((ssync[snum].bits >> 13) & 7) == 4) && (myconnectindex == snum))
            locselectedgun = 1;

        if ((health[snum] >= 0) || ((krand() & 127) > -health[snum]))
            switch ((ssync[snum].bits >> 13) & 7)
            {
            case 0:
                if (lockclock > lastchaingun[snum]+8)
                {
                    lastchaingun[snum] = lockclock;
                    shootgun(snum,&pos[snum],ang[snum],horiz[snum],cursectnum[snum],0);
                }
                break;
            case 1:
                if ((oflags[snum] & 2048) == 0)
                    shootgun(snum,&pos[snum],ang[snum],horiz[snum],cursectnum[snum],1);
                break;
            case 2:
                if ((oflags[snum] & 2048) == 0)
                    if (numbombs[snum] > 0)
                    {
                        shootgun(snum,&pos[snum],ang[snum],horiz[snum],cursectnum[snum],2);
                        changenumbombs(snum,-1);
                    }
                break;
            case 3:
                if ((oflags[snum] & 2048) == 0)
                    if (nummissiles[snum] > 0)
                    {
                        shootgun(snum,&pos[snum],ang[snum],horiz[snum],cursectnum[snum],3);
                        changenummissiles(snum,-1);
                    }
                break;
            case 4:
                if ((oflags[snum] & 2048) == 0)
                    if (numgrabbers[snum] > 0)
                    {
                        shootgun(snum,&pos[snum],ang[snum],horiz[snum],cursectnum[snum],4);
                        changenumgrabbers(snum,-1);
                    }
                break;
            }
    }

    if ((ssync[snum].bits&4096) > (oflags[snum]&4096))  //Keypad enter
    {
        dimensionmode[snum]++;
        if (dimensionmode[snum] > 3) dimensionmode[snum] = 1;
    }

    oflags[snum] = ssync[snum].bits;
}

void view(short snum, vec3_t *v, short *vsectnum, short ang, int horiz)
{
    spritetype *sp;
    int i, nx, ny, nz, hx, hy /*, hz*/;
    short bakcstat, daang;
    hitdata_t hitinfo;

    nx = (sintable[(ang+1536)&2047]>>4);
    ny = (sintable[(ang+1024)&2047]>>4);
    nz = (horiz-100)*128;

    sp = &sprite[snum];

    bakcstat = sp->cstat;
    sp->cstat &= (short)~0x101;

    updatesectorz(v->x,v->y,v->z,vsectnum);
    hitscan(v,*vsectnum,nx,ny,nz,&hitinfo,CLIPMASK1);
    hx = hitinfo.pos.x-v->x; hy = hitinfo.pos.y-v->y;
    if (klabs(nx)+klabs(ny) > klabs(hx)+klabs(hy))
    {
        *vsectnum = hitinfo.sect;
        if (hitinfo.wall >= 0)
        {
            daang = getangle(wall[wall[hitinfo.wall].point2].x-wall[hitinfo.wall].x,
                             wall[wall[hitinfo.wall].point2].y-wall[hitinfo.wall].y);

            i = nx*sintable[daang]+ny*sintable[(daang+1536)&2047];
            if (klabs(nx) > klabs(ny)) hx -= mulscale28(nx,i);
            else hy -= mulscale28(ny,i);
        }
        else if (hitinfo.sprite < 0)
        {
            if (klabs(nx) > klabs(ny)) hx -= (nx>>5);
            else hy -= (ny>>5);
        }
        if (klabs(nx) > klabs(ny)) i = divscale16(hx,nx);
        else i = divscale16(hy,ny);
        if (i < cameradist) cameradist = i;
    }
    v->x = v->x+mulscale16(nx,cameradist);
    v->y = v->y+mulscale16(ny,cameradist);
    v->z = v->z+mulscale16(nz,cameradist);

    updatesectorz(v->x,v->y,v->z,vsectnum);

    sp->cstat = bakcstat;
}

void drawscreen(short snum, int dasmoothratio)
{
    int i, j, k=0, l, charsperline, tempint;
    int x1, y1, x2, y2, ox1, oy1, ox2, oy2, dist, maxdist;
    vec3_t cpos;
    int choriz, czoom, tposx, tposy;
    int tiltlock, *intptr, ovisibility, oparallaxvisibility;
    short cang, tang, csect;
    char ch, *ptr, *ptr2, *ptr3, *ptr4;
    uspritetype *tspr;

    smoothratio = max(min(dasmoothratio,65536),0);

    dointerpolations();

    if ((snum == myconnectindex) && ((networkmode == 1) || (myconnectindex != connecthead)))
    {
        cpos.x = omy.x+mulscale16(my.x-omy.x,smoothratio);
        cpos.y = omy.y+mulscale16(my.y-omy.y,smoothratio);
        cpos.z = omy.z+mulscale16(my.z-omy.z,smoothratio);
        choriz = omyhoriz+mulscale16(myhoriz-omyhoriz,smoothratio);
        cang = omyang+mulscale16((int)(((myang+1024-omyang)&2047)-1024),smoothratio);
    }
    else
    {
        cpos.x = opos[snum].x+mulscale16(pos[snum].x-opos[snum].x,smoothratio);
        cpos.y = opos[snum].y+mulscale16(pos[snum].y-opos[snum].y,smoothratio);
        cpos.z = opos[snum].z+mulscale16(pos[snum].z-opos[snum].z,smoothratio);
        choriz = ohoriz[snum]+mulscale16(horiz[snum]-ohoriz[snum],smoothratio);
        cang = oang[snum]+mulscale16(((ang[snum]+1024-oang[snum])&2047)-1024,smoothratio);
    }
    czoom = ozoom[snum]+mulscale16(zoom[snum]-ozoom[snum],smoothratio);

    setears(cpos.x,cpos.y,(int)sintable[(cang+512)&2047]<<14,(int)sintable[cang&2047]<<14);

    if (dimensionmode[myconnectindex] == 3)
    {
        tempint = screensize;

        if (((loc.bits&32) > (screensizeflag&32)) && (screensize > 64))
        {
            ox1 = ((xdim-screensize)>>1);
            ox2 = ox1+screensize-1;
            oy1 = (((ydim-32)-scale(screensize,ydim-32,xdim))>>1);
            oy2 = oy1 + scale(screensize,ydim-32,xdim)-1;
            screensize -= (screensize>>3);

            if (tempint > xdim)
            {
                screensize = xdim;

                flushperms();

                rotatesprite((xdim-320)<<15,(ydim-32)<<16,65536L,0,STATUSBAR,0,0,8+16+64+128,0L,0L,xdim-1L,ydim-1L);
                i = ((xdim-320)>>1);
                while (i >= 8) i -= 8, rotatesprite(i<<16,(ydim-32)<<16,65536L,0,STATUSBARFILL8,0,0,8+16+64+128,0L,0L,xdim-1L,ydim-1L);
                if (i >= 4) i -= 4, rotatesprite(i<<16,(ydim-32)<<16,65536L,0,STATUSBARFILL4,0,0,8+16+64+128,0L,0L,xdim-1L,ydim-1L);
                i = ((xdim-320)>>1)+320;
                while (i <= xdim-8) rotatesprite(i<<16,(ydim-32)<<16,65536L,0,STATUSBARFILL8,0,0,8+16+64+128,0L,0L,xdim-1L,ydim-1L), i += 8;
                if (i <= xdim-4) rotatesprite(i<<16,(ydim-32)<<16,65536L,0,STATUSBARFILL4,0,0,8+16+64+128,0L,0L,xdim-1L,ydim-1L), i += 4;

                drawstatusbar(screenpeek);   // Andy did this
            }

            x1 = ((xdim-screensize)>>1);
            x2 = x1+screensize-1;
            y1 = (((ydim-32)-scale(screensize,ydim-32,xdim))>>1);
            y2 = y1 + scale(screensize,ydim-32,xdim)-1;
            setview(x1,y1,x2,y2);

            // (ox1,oy1)⁄ƒƒƒƒƒƒƒƒƒƒƒƒƒƒƒƒƒø
            //          ≥  (x1,y1)        ≥
            //          ≥     ⁄ƒƒƒƒƒø     ≥
            //          ≥     ≥     ≥     ≥
            //          ≥     ¿ƒƒƒƒƒŸ     ≥
            //          ≥        (x2,y2)  ≥
            //          ¿ƒƒƒƒƒƒƒƒƒƒƒƒƒƒƒƒƒŸ(ox2,oy2)

            drawtilebackground(/*0L,0L,*/ BACKGROUND,8,ox1,oy1,x1-1,oy2,0);
            drawtilebackground(/*0L,0L,*/ BACKGROUND,8,x2+1,oy1,ox2,oy2,0);
            drawtilebackground(/*0L,0L,*/ BACKGROUND,8,x1,oy1,x2,y1-1,0);
            drawtilebackground(/*0L,0L,*/ BACKGROUND,8,x1,y2+1,x2,oy2,0);
        }
        if (((loc.bits&16) > (screensizeflag&16)) && (screensize <= xdim))
        {
            screensize += (screensize>>3);
            if ((screensize > xdim) && (tempint == xdim))
            {
                screensize = xdim+1;
                x1 = 0; y1 = 0;
                x2 = xdim-1; y2 = ydim-1;
            }
            else
            {
                if (screensize > xdim) screensize = xdim;
                x1 = ((xdim-screensize)>>1);
                x2 = x1+screensize-1;
                y1 = (((ydim-32)-scale(screensize,ydim-32,xdim))>>1);
                y2 = y1 + scale(screensize,ydim-32,xdim)-1;
            }
            setview(x1,y1,x2,y2);
        }
        screensizeflag = loc.bits;
    }

    if (dimensionmode[snum] != 2)
    {
        if ((numplayers > 1) && (option[4] == 0))
        {
            //Do not draw other views constantly if they're staying still
            //It's a shame this trick will only work in screen-buffer mode
            //At least screen-buffer mode covers all the HI hi-res modes
            //if (vidoption == 2)
            //{
            for (i=connecthead; i>=0; i=connectpoint2[i]) frame2draw[i] = 0;
            frame2draw[snum] = 1;

            //2-1,3-1,4-2
            //5-2,6-2,7-2,8-3,9-3,10-3,11-3,12-4,13-4,14-4,15-4,16-5
            x1 = pos[snum].x; y1 = pos[snum].y;
            for (j=(numplayers>>2)+1; j>0; j--)
            {
                maxdist = 0x80000000;
                for (i=connecthead; i>=0; i=connectpoint2[i])
                    if (frame2draw[i] == 0)
                    {
                        x2 = pos[i].x-x1; y2 = pos[i].y-y1;
                        dist = dmulscale12(x2,x2,y2,y2);

                        if (dist < 64) dist = 16384;
                        else if (dist > 16384) dist = 64;
                        else dist = 1048576 / dist;

                        dist *= frameskipcnt[i];

                        //Increase frame rate if screen is moving
                        if ((pos[i].x != opos[i].x) || (pos[i].y != opos[i].y) ||
                            (pos[i].z != opos[i].z) || (ang[i] != oang[i]) ||
                            (horiz[i] != ohoriz[i])) dist += dist;

                        if (dist > maxdist) maxdist = dist, k = i;
                    }

                for (i=connecthead; i>=0; i=connectpoint2[i])
                    frameskipcnt[i] += (frameskipcnt[i]>>3)+1;
                frameskipcnt[k] = 0;

                frame2draw[k] = 1;
            }
            //}
            //else
            //{
            //   for(i=connecthead;i>=0;i=connectpoint2[i]) frame2draw[i] = 1;
            //}

            for (i=connecthead,j=0; i>=0; i=connectpoint2[i],j++)
                if (frame2draw[i] != 0)
                {
                    if (numplayers <= 4)
                    {
                        switch (j)
                        {
                        case 0: setview(0,0,(xdim>>1)-1,(ydim>>1)-1); break;
                        case 1: setview((xdim>>1),0,xdim-1,(ydim>>1)-1); break;
                        case 2: setview(0,(ydim>>1),(xdim>>1)-1,ydim-1); break;
                        case 3: setview((xdim>>1),(ydim>>1),xdim-1,ydim-1); break;
                        }
                    }
                    else
                    {
                        switch (j)
                        {
                        case 0: setview(0,0,(xdim>>2)-1,(ydim>>2)-1); break;
                        case 1: setview(xdim>>2,0,(xdim>>1)-1,(ydim>>2)-1); break;
                        case 2: setview(xdim>>1,0,xdim-(xdim>>2)-1,(ydim>>2)-1); break;
                        case 3: setview(xdim-(xdim>>2),0,xdim-1,(ydim>>2)-1); break;
                        case 4: setview(0,ydim>>2,(xdim>>2)-1,(ydim>>1)-1); break;
                        case 5: setview(xdim>>2,ydim>>2,(xdim>>1)-1,(ydim>>1)-1); break;
                        case 6: setview(xdim>>1,ydim>>2,xdim-(xdim>>2)-1,(ydim>>1)-1); break;
                        case 7: setview(xdim-(xdim>>2),ydim>>2,xdim-1,(ydim>>1)-1); break;
                        case 8: setview(0,ydim>>1,(xdim>>2)-1,ydim-(ydim>>2)-1); break;
                        case 9: setview(xdim>>2,ydim>>1,(xdim>>1)-1,ydim-(ydim>>2)-1); break;
                        case 10: setview(xdim>>1,ydim>>1,xdim-(xdim>>2)-1,ydim-(ydim>>2)-1); break;
                        case 11: setview(xdim-(xdim>>2),ydim>>1,xdim-1,ydim-(ydim>>2)-1); break;
                        case 12: setview(0,ydim-(ydim>>2),(xdim>>2)-1,ydim-1); break;
                        case 13: setview(xdim>>2,ydim-(ydim>>2),(xdim>>1)-1,ydim-1); break;
                        case 14: setview(xdim>>1,ydim-(ydim>>2),xdim-(xdim>>2)-1,ydim-1); break;
                        case 15: setview(xdim-(xdim>>2),ydim-(ydim>>2),xdim-1,ydim-1); break;
                        }
                    }

                    if (i == snum)
                    {
                        sprite[playersprite[snum]].cstat |= 0x8000;
                        drawrooms(cpos.x,cpos.y,cpos.z,cang,choriz,cursectnum[i]);
                        sprite[playersprite[snum]].cstat &= (uint16_t) ~0x8000;
                        analyzesprites(cpos.x,cpos.y);
                    }
                    else
                    {
                        sprite[playersprite[i]].cstat |= 0x8000;
                        drawrooms(pos[i].x,pos[i].y,pos[i].z,ang[i],horiz[i],cursectnum[i]);
                        sprite[playersprite[i]].cstat &= (uint16_t) ~0x8000;
                        analyzesprites(pos[i].x,pos[i].y);
                    }
                    drawmasks();
                    if ((numgrabbers[i] > 0) || (nummissiles[i] > 0) || (numbombs[i] > 0))
                        rotatesprite(160<<16,184L<<16,65536,0,GUNONBOTTOM,sector[cursectnum[i]].floorshade,0,2,windowxy1.x,windowxy1.y,windowxy2.x,windowxy2.y);

                    if (lockclock < 384)
                    {
                        if (lockclock < 128)
                            rotatesprite(320<<15,200<<15,lockclock<<9,lockclock<<4,DEMOSIGN,(128-lockclock)>>2,0,1+2,windowxy1.x,windowxy1.y,windowxy2.x,windowxy2.y);
                        else if (lockclock < 256)
                            rotatesprite(320<<15,200<<15,65536,0,DEMOSIGN,0,0,2,windowxy1.x,windowxy1.y,windowxy2.x,windowxy2.y);
                        else
                            rotatesprite(320<<15,200<<15,(384-lockclock)<<9,lockclock<<4,DEMOSIGN,(lockclock-256)>>2,0,1+2,windowxy1.x,windowxy1.y,windowxy2.x,windowxy2.y);
                    }

                    if (health[i] <= 0)
                        rotatesprite(320<<15,200<<15,(-health[i])<<11,(-health[i])<<5,NO,0,0,2,windowxy1.x,windowxy1.y,windowxy2.x,windowxy2.y);
                }
        }
        else
        {
            //Init for screen rotation
            if (getrendermode() == 0)     // JBF 20031220
            {
                tiltlock = screentilt;
                if ((tiltlock) || (detailmode))
                {
                    walock[TILE_TILT] = 255;
                    if (waloff[TILE_TILT] == 0)
                        allocache(&waloff[TILE_TILT],320L*320L,&walock[TILE_TILT]);
                    if ((tiltlock&1023) == 0)
                        setviewtotile(TILE_TILT,200L>>detailmode,320L>>detailmode);
                    else
                        setviewtotile(TILE_TILT,320L>>detailmode,320L>>detailmode);
                    if ((tiltlock&1023) == 512)
                    {
                        //Block off unscreen section of 90¯ tilted screen
                        j = ((320-60)>>detailmode);
                        for (i=(60>>detailmode)-1; i>=0; i--)
                        {
                            startumost[i] = 1; startumost[i+j] = 1;
                            startdmost[i] = 0; startdmost[i+j] = 0;
                        }
                    }

                    i = (tiltlock&511); if (i > 256) i = 512-i;
                    i = sintable[i+512]*8 + sintable[i]*5L;
                    setaspect(i>>1,yxaspect);
                }
            }
            else
            {
                tiltlock = screentilt;
                // Ken loves to interpolate
                setrollangle(oscreentilt + mulscale16(((screentilt-oscreentilt+1024)&2047)-1024,smoothratio));
            }

            if ((gotpic[FLOORMIRROR>>3]&(1<<(FLOORMIRROR&7))) > 0)
            {
                dist = 0x7fffffff; i = 0;
                for (k=floormirrorcnt-1; k>=0; k--)
                {
                    j = klabs(wall[sector[floormirrorsector[k]].wallptr].x-cpos.x);
                    j += klabs(wall[sector[floormirrorsector[k]].wallptr].y-cpos.y);
                    if (j < dist) dist = j, i = k;
                }

                //if (cpos.z > sector[floormirrorsector[i]].ceilingz) i = 1-i; //SOS

                j = floormirrorsector[i];

                if (cameradist < 0) sprite[playersprite[snum]].cstat |= 0x8000;
                drawrooms(cpos.x,cpos.y,(sector[j].floorz<<1)-cpos.z,cang,201-choriz,j); //SOS
                //drawrooms(cpos.x,cpos.y,cpos.z,cang,choriz,j+MAXSECTORS); //SOS
                sprite[playersprite[snum]].cstat &= (uint16_t) ~0x8000;
                analyzesprites(cpos.x,cpos.y);
                drawmasks();

                //Temp horizon
                if (getrendermode() == 0)
                {
                    l = scale(choriz-100,windowxy2.x-windowxy1.x,320)+((windowxy1.y+windowxy2.y)>>1);
                    begindrawing();   //{{{
                    for (y1=windowxy1.y,y2=windowxy2.y; y1<y2; y1++,y2--)
                    {
                        ptr = (char *)(frameplace+ylookup[y1]);
                        ptr2 = (char *)(frameplace+ylookup[y2]);
                        ptr3 = palookup[18];
                        ptr3 += (min(klabs(y1-l)>>2,31)<<8);
                        ptr4 = palookup[18];
                        ptr4 += (min(klabs(y2-l)>>2,31)<<8);

                        j = sintable[((y2+totalclock)<<6)&2047];
                        j += sintable[((y2-totalclock)<<7)&2047];
                        j >>= 14;

                        //ptr2 += j;

                        //for(x1=windowxy1.x;x1<=windowxy2.x;x1++)
                        //	{ ch = ptr[x1]; ptr[x1] = ptr3[ptr2[x1]]; ptr2[x1] = ptr4[ch]; }

                        ox1 = windowxy1.x-min(j,0);
                        ox2 = windowxy2.x-max(j,0);

                        for (x1=windowxy1.x; x1<ox1; x1++)
                        { ch = ptr[x1]; ptr[x1] = ptr3[ptr2[x1]]; ptr2[x1] = ptr4[ch]; }
                        for (x1=ox2+1; x1<=windowxy2.x; x1++)
                        { ch = ptr[x1]; ptr[x1] = ptr3[ptr2[x1]]; ptr2[x1] = ptr4[ch]; }

                        ptr2 += j;
                        for (x1=ox1; x1<=ox2; x1++)
                        { ch = ptr[x1]; ptr[x1] = ptr3[ptr2[x1]]; ptr2[x1] = ptr4[ch]; }
                    }
                    enddrawing(); //}}}
                }
                gotpic[FLOORMIRROR>>3] &= ~(1<<(FLOORMIRROR&7));
            }


            //Over the shoulder mode
            csect = cursectnum[snum];
            if (cameradist >= 0)
            {
                cang += cameraang;
                view(playersprite[snum],&cpos,&csect,cang,choriz);
            }

            //WARNING!  Assuming (MIRRORLABEL&31) = 0 and MAXMIRRORS = 64
            intptr = (int *)&gotpic[MIRRORLABEL>>3];   // CHECK!
            if (intptr[0]|intptr[1])
                for (i=MAXMIRRORS-1; i>=0; i--)
                    if (gotpic[(i+MIRRORLABEL)>>3]&(1<<(i&7)))
                    {
                        gotpic[(i+MIRRORLABEL)>>3] &= ~(1<<(i&7));

                        //Prepare drawrooms for drawing mirror and calculate reflected
                        //position into tposx, tposy, and tang (tpos.z == cpos.z)
                        //Must call preparemirror before drawrooms and
                        //          completemirror after drawrooms
                        preparemirror(cpos.x,cpos.y,/*cpos.z,*/ cang, /*choriz,*/
                                      mirrorwall[i],/*mirrorsector[i],*/ &tposx,&tposy,&tang);

                        ovisibility = g_visibility;
                        oparallaxvisibility = parallaxvisibility;
                        g_visibility <<= 1;
                        parallaxvisibility <<= 1;
                        ptr = palookup[0]; palookup[0] = palookup[17]; palookup[17] = ptr;

                        drawrooms(tposx,tposy,cpos.z,tang,choriz,mirrorsector[i]|MAXSECTORS);
                        for (j=0,tspr=&tsprite[0]; j<spritesortcnt; j++,tspr++)
                            if ((tspr->cstat&48) == 0) tspr->cstat |= 4;
                        analyzesprites(tposx,tposy);
                        drawmasks();

                        ptr = palookup[0]; palookup[0] = palookup[17]; palookup[17] = ptr;
                        g_visibility = ovisibility;
                        parallaxvisibility = oparallaxvisibility;

                        completemirror();   //Reverse screen x-wise in this function

                        break;
                    }

            if (cameradist < 0) sprite[playersprite[snum]].cstat |= 0x8000;
            drawrooms(cpos.x,cpos.y,cpos.z,cang,choriz,csect);
            sprite[playersprite[snum]].cstat &= (uint16_t) ~0x8000;
            analyzesprites(cpos.x,cpos.y);
            drawmasks();

            //Finish for screen rotation
            if (getrendermode() == 0)        // JBF 20031220
            {
                if ((tiltlock) || (detailmode))
                {
                    setviewback();
                    i = (tiltlock&511); if (i > 256) i = 512-i;
                    i = sintable[i+512]*8 + sintable[i]*5L;
                    if (detailmode == 0) i >>= 1;
                    rotatesprite(320<<15,200<<15,i,tiltlock+512,TILE_TILT,0,0,2+4+64,windowxy1.x,windowxy1.y,windowxy2.x,windowxy2.y);
                    walock[TILE_TILT] = 1;
                }
            }

            if (((numgrabbers[screenpeek] > 0) || (nummissiles[screenpeek] > 0) || (numbombs[screenpeek] > 0)) && (cameradist < 0))
            {
                //Reset startdmost to bottom of screen
                if ((windowxy1.x == 0) && (windowxy2.x == 319) && (yxaspect == 65536) && (tiltlock == 0))
                {
                    x1 = 160L-(tilesiz[GUNONBOTTOM].x>>1); y1 = windowxy2.y+1;
                    for (i=0; i<tilesiz[GUNONBOTTOM].x; i++)
                        startdmost[i+x1] = y1;
                }
                rotatesprite(160<<16,184L<<16,65536,0,GUNONBOTTOM,sector[cursectnum[screenpeek]].floorshade,0,2,windowxy1.x,windowxy1.y,windowxy2.x,windowxy2.y);
            }

#if 0
            if (cachecount != 0)
            {
                rotatesprite((320-16)<<16,16<<16,32768,0,BUILDDISK,0,0,2+64,windowxy1.x,windowxy1.y,windowxy2.x,windowxy2.y);
                cachecount = 0;
            }
#endif

            if (lockclock < 384)
            {
                if (lockclock < 128)
                    rotatesprite(320<<15,200<<15,lockclock<<9,lockclock<<4,DEMOSIGN,(128-lockclock)>>2,0,1+2,windowxy1.x,windowxy1.y,windowxy2.x,windowxy2.y);
                else if (lockclock < 256)
                    rotatesprite(320<<15,200<<15,65536,0,DEMOSIGN,0,0,2,windowxy1.x,windowxy1.y,windowxy2.x,windowxy2.y);
                else
                    rotatesprite(320<<15,200<<15,(384-lockclock)<<9,lockclock<<4,DEMOSIGN,(lockclock-256)>>2,0,1+2,windowxy1.x,windowxy1.y,windowxy2.x,windowxy2.y);
            }

            if (health[screenpeek] <= 0)
                rotatesprite(320<<15,200<<15,(-health[screenpeek])<<11,(-health[screenpeek])<<5,NO,0,0,2,windowxy1.x,windowxy1.y,windowxy2.x,windowxy2.y);
        }
    }

    //Only animate lava if its picnum is on screen
    //gotpic is a bit array where the tile number's bit is set
    //whenever it is drawn (ceilings, walls, sprites, etc.)
    if ((gotpic[SLIME>>3]&(1<<(SLIME&7))) > 0)
    {
        gotpic[SLIME>>3] &= ~(1<<(SLIME&7));
        if (waloff[SLIME] != 0)
        {
            movelava((char *)waloff[SLIME]);
            invalidatetile(SLIME,0,1);   // JBF 20031228
        }
    }

    if ((show2dsector[cursectnum[snum]>>3]&(1<<(cursectnum[snum]&7))) == 0)
        searchmap(cursectnum[snum]);

    if (dimensionmode[snum] != 3)
    {
        //Move back pivot point
        i = scale(czoom,screensize,320);
        if (dimensionmode[snum] == 2)
        {
            clearview(0L);  //Clear screen to specified color
            drawmapview(cpos.x,cpos.y,i,cang);
        }
        drawoverheadmap(cpos.x,cpos.y,i,cang);
    }

    if (typemode != 0)
    {
        charsperline = 40;
        //if (dimensionmode[snum] == 2) charsperline = 80;

        for (i=0; i<=typemessageleng; i+=charsperline)
        {
            for (j=0; j<charsperline; j++)
                tempbuf[j] = typemessage[i+j];
            if (typemessageleng < i+charsperline)
            {
                tempbuf[(typemessageleng-i)] = '_';
                tempbuf[(typemessageleng-i)+1] = 0;
            }
            else
                tempbuf[charsperline] = 0;
            //if (dimensionmode[snum] == 3)
            printext256(0L,(i/charsperline)<<3,31 /*183*/,-1,(char *)tempbuf,0);
            //else
            //   printext16(0L,((i/charsperline)<<3)+(pageoffset/640),10,-1,(char *)tempbuf,0);
        }
    }

    if (getmessageleng > 0)
    {
        charsperline = 40;
        //if (dimensionmode[snum] == 2) charsperline = 80;

        for (i=0; i<=getmessageleng; i+=charsperline)
        {
            for (j=0; j<charsperline; j++)
                tempbuf[j] = getmessage[i+j];
            if (getmessageleng < i+charsperline)
                tempbuf[(getmessageleng-i)] = 0;
            else
                tempbuf[charsperline] = 0;

            printext256(0L,((i/charsperline)<<3)+(ydim-32-8)-(((getmessageleng-1)/charsperline)<<3),31 /*151*/,-1,(char *)tempbuf,0);
        }
        if (totalclock > getmessagetimeoff)
            getmessageleng = 0;
    }
    if ((numplayers >= 2) && (screenpeek != myconnectindex))
    {
        j = 1;
        for (i=connecthead; i>=0; i=connectpoint2[i])
        {
            if (i == screenpeek) break;
            j++;
        }
        Bsprintf((char *)tempbuf,"(Player %d's view)",j);
        printext256((xdim>>1)-(Bstrlen((char *)tempbuf)<<2),0,24,-1,(char *)tempbuf,0);
    }

    if (syncstat != 0) printext256(68L,84L,31,0,"OUT OF SYNC!",0);
    if (syncstate != 0) printext256(68L,92L,31,0,"Missed Network packet!",0);

//   //Uncomment this to test cache locks
//extern int cacnum;
//typedef struct { int *hand, leng; char *lock; } cactype;
//extern cactype cac[];
//
//   j = 0;
//   for(i=0;i<cacnum;i++)
//      if ((*cac[i].lock) >= 200)
//      {
//         Bsprintf(tempbuf,"Locked- %ld: Leng:%ld, Lock:%ld",i,cac[i].leng,*cac[i].lock);
//         printext256(0L,j,31,-1,tempbuf,1); j += 6;
//      }

    nextpage();   // send completed frame to display

    while (totalclock >= ototalclock+(TIMERINTSPERSECOND/MOVESPERSECOND))
        faketimerhandler();

    if (keystatus[0x3f])   //F5
    {
        keystatus[0x3f] = 0;
        detailmode ^= 1;
        //setrendermode(3);
    }
    if (keystatus[0x58])   //F12
    {
        keystatus[0x58] = 0;
        screencapture("captxxxx.tga",keystatus[0x2a]|keystatus[0x36]);
    }
    if (keystatus[0x3e])  //F4 - screen re-size
    {
        keystatus[0x3e] = 0;

        if (keystatus[0x2a]|keystatus[0x36])
        {
            setgamemode(!fullscreen, xdim, ydim, bpp);
        }
        else
        {

            //cycle through all modes
            j=-1;

            // work out a mask to select the mode
            for (i=0; i<validmodecnt; i++)
                if ((validmode[i].xdim == xdim) &&
                    (validmode[i].ydim == ydim) &&
                    (validmode[i].fs == fullscreen) &&
                    (validmode[i].bpp == bpp))
                { j=i; break; }

            for (k=0; k<validmodecnt; k++)
                if (validmode[k].fs == fullscreen && validmode[k].bpp == bpp) break;

            if (j==-1) j=k;
            else
            {
                j++;
                if (j==validmodecnt) j=k;
            }
            setgamemode(fullscreen,validmode[j].xdim,validmode[j].ydim,bpp);
        }
        screensize = xdim+1;

        Bsprintf(getmessage,"Video mode: %d x %d",xdim,ydim);
        getmessageleng = Bstrlen(getmessage);
        getmessagetimeoff = totalclock+120*5;
    }
    if (keystatus[0x57])  //F11 - brightness
    {
        keystatus[0x57] = 0;
        brightness++;
        if (brightness > 8) brightness = 0;
        setbrightness(brightness,0,0);
    }

    if (option[4] == 0)           //Single player only keys
    {
        if (keystatus[0xd2])   //Insert - Insert player
        {
            keystatus[0xd2] = 0;
            if (numplayers < MAXPLAYERS)
            {
                connectpoint2[numplayers-1] = numplayers;
                connectpoint2[numplayers] = -1;

                movefifoend[numplayers] = movefifoend[0];   //HACK 01/05/2000

                initplayersprite(numplayers);

                clearallviews(0L);  //Clear screen to specified color

                numplayers++;
            }
        }
        if (keystatus[0xd3])   //Delete - Delete player
        {
            keystatus[0xd3] = 0;
            if (numplayers > 1)
            {
                numplayers--;
                connectpoint2[numplayers-1] = -1;

                deletesprite(playersprite[numplayers]);
                playersprite[numplayers] = -1;

                if (myconnectindex >= numplayers) myconnectindex = 0;
                if (screenpeek >= numplayers) screenpeek = 0;

                if (numplayers < 2)
                    setup3dscreen();
                else
                    clearallviews(0L);  //Clear screen to specified color
            }
        }
        if (keystatus[0x46])   //Scroll Lock
        {
            keystatus[0x46] = 0;

            myconnectindex = connectpoint2[myconnectindex];
            if (myconnectindex < 0) myconnectindex = connecthead;
            screenpeek = myconnectindex;
        }
    }

    restoreinterpolations();
}

void movethings(void)
{
    int i;

    gotlastpacketclock = totalclock;
    for (i=connecthead; i>=0; i=connectpoint2[i])
    {
        copybufbyte(&ffsync[i],&baksync[movefifoend[i]][i],sizeof(input));
        movefifoend[i] = ((movefifoend[i]+1)&(MOVEFIFOSIZ-1));
    }
}

void fakedomovethings(void)
{
    input *syn;
    int /*i, j, k,*/ doubvel, xvect, yvect, goalz;
    short bakcstat;

    syn = (input *)&baksync[fakemovefifoplc][myconnectindex];

    omy = my;
    omyang = myang;
    omyhoriz = myhoriz;

    bakcstat = sprite[playersprite[myconnectindex]].cstat;
    sprite[playersprite[myconnectindex]].cstat &= ~0x101;

    if ((syn->fvel|syn->svel) != 0)
    {
        doubvel = (TICSPERFRAME<<((syn->bits&256)>0));

        xvect = 0, yvect = 0;
        if (syn->fvel != 0)
        {
            xvect += ((((int)syn->fvel)*doubvel*(int)sintable[(myang+512)&2047])>>3);
            yvect += ((((int)syn->fvel)*doubvel*(int)sintable[myang&2047])>>3);
        }
        if (syn->svel != 0)
        {
            xvect += ((((int)syn->svel)*doubvel*(int)sintable[myang&2047])>>3);
            yvect += ((((int)syn->svel)*doubvel*(int)sintable[(myang+1536)&2047])>>3);
        }
        if (flytime[myconnectindex] > lockclock) { xvect += xvect; yvect += yvect; }   // DOuble flying speed
        clipmove(&my,&mycursectnum,xvect,yvect,128L,4<<8,4<<8,CLIPMASK0);
    }

    pushmove(&my,&mycursectnum,128L,4<<8,4<<8,CLIPMASK0);
    getzrange(&my,mycursectnum,&globhiz,&globhihit,&globloz,&globlohit,128L,CLIPMASK0);

    if (syn->avel != 0)          //ang += avel * constant
    {
        //ENGINE calculates avel for you
        doubvel = TICSPERFRAME;
        if ((syn->bits&256) > 0)  //Lt. shift makes turn velocity 50% faster
            doubvel += (TICSPERFRAME>>1);
        myang += ((((int)syn->avel)*doubvel)>>4);
        myang &= 2047;
    }

    if (((syn->bits&8) > 0) && (myhoriz > 100-(200>>1))) myhoriz -= 4;   //-
    if (((syn->bits&4) > 0) && (myhoriz < 100+(200>>1))) myhoriz += 4;   //+

    goalz = globloz-EYEHEIGHT;
    if (sector[mycursectnum].lotag == 4)   //slime sector
        if ((globlohit&0xc000) != 49152)            //You're not on a sprite
        {
            goalz = globloz-(8<<8);
            if (my.z >= goalz-(2<<8))
                clipmove(&my,&mycursectnum,-(TICSPERFRAME<<14),-(TICSPERFRAME<<14),128L,4<<8,4<<8,CLIPMASK0);
        }
    if (goalz < globhiz+(16<<8))   //ceiling&floor too close
        goalz = ((globloz+globhiz)>>1);

    if (health[myconnectindex] >= 0)
    {
        if ((syn->bits&1) > 0)                         //A (stand high)
        {
            if (flytime[myconnectindex] <= lockclock)
            {
                if (my.z >= globloz-(32<<8))
                {
                    goalz -= (16<<8);
                    if (syn->bits&256) goalz -= (24<<8);
                }
            }
            else
            {
                myzvel -= 192;
                if (syn->bits&256) myzvel -= 192;
            }
        }
        if ((syn->bits&2) > 0)                         //Z (stand low)
        {
            if (flytime[myconnectindex] <= lockclock)
            {
                goalz += (12<<8);
                if (syn->bits&256) goalz += (12<<8);
            }
            else
            {
                myzvel += 192;
                if (syn->bits&256) myzvel += 192;
            }
        }
    }

    if (flytime[myconnectindex] <= lockclock)
    {
        if (my.z < goalz)
            myzvel += (TICSPERFRAME<<4);
        else
            myzvel = (((goalz-my.z)*TICSPERFRAME)>>5);
    }
    else
    {
        myzvel -= (myzvel>>2);
        myzvel -= ksgn(myzvel);
    }

    my.z += myzvel;
    if (my.z > globloz-(4<<8)) my.z = globloz-(4<<8), myzvel = 0;
    if (my.z < globhiz+(4<<8)) my.z = globhiz+(4<<8), myzvel = 0;

    sprite[playersprite[myconnectindex]].cstat = bakcstat;

    mybak[fakemovefifoplc] = my;
    myangbak[fakemovefifoplc] = myang;
    myhorizbak[fakemovefifoplc] = myhoriz;
    fakemovefifoplc = (fakemovefifoplc+1)&(MOVEFIFOSIZ-1);
}

//Prediction correction
void fakedomovethingscorrect(void)
{
    int i;

    if ((networkmode == 0) && (myconnectindex == connecthead)) return;

    i = ((movefifoplc-1)&(MOVEFIFOSIZ-1));

    if ((pos[myconnectindex].x == mybak[i].x) &&
        (pos[myconnectindex].y == mybak[i].y) &&
        (pos[myconnectindex].z == mybak[i].z) &&
        (horiz[myconnectindex] == myhorizbak[i]) &&
        (ang[myconnectindex] == myangbak[i]))
        return;

    //Re-start fakedomovethings back to place of error
    my = omy;
    myzvel = hvel[myconnectindex];
    myang = omyang = ang[myconnectindex];
    mycursectnum = cursectnum[myconnectindex];
    myhoriz = omyhoriz = horiz[myconnectindex];

    fakemovefifoplc = movefifoplc;
    while (fakemovefifoplc != movefifoend[myconnectindex]) fakedomovethings();
}

void domovethings(void)
{
    short i, j, startwall, endwall;
    // spritetype *spr;
    walltype *wal;
    // vec3_t *ospr;

    nummoves++;

    for (i=connecthead; i>=0; i=connectpoint2[i])
        copybufbyte(&baksync[movefifoplc][i],&ssync[i],sizeof(input));
    movefifoplc = ((movefifoplc+1)&(MOVEFIFOSIZ-1));

    if (option[4] != 0)
    {
        syncval[syncvalhead] = (char)(randomseed&255);
        syncvalhead = ((syncvalhead+1)&(MOVEFIFOSIZ-1));
    }

    for (i=connecthead; i>=0; i=connectpoint2[i])
    {
        opos[i] = pos[i];
        ohoriz[i] = horiz[i];
        ozoom[i] = zoom[i];
        oang[i] = ang[i];
    }

    for (i=NUMSTATS-1; i>=0; i--)
        if (statrate[i] >= 0)
            for (j=headspritestat[i]; j>=0; j=nextspritestat[j])
                if (((nummoves-j)&statrate[i]) == 0)
                    copybuf(&sprite[j].x,&osprite[j].x,3);

    for (i=connecthead; i>=0; i=connectpoint2[i])
        ocursectnum[i] = cursectnum[i];

    updateinterpolations();

    if ((numplayers <= 2) && (recstat == 1))
    {
        j = 0;
        for (i=connecthead; i>=0; i=connectpoint2[i])
        {
            copybufbyte(&ssync[i],&recsync[reccnt][j],sizeof(input));
            j++;
        }
        reccnt++; if (reccnt > 16383) reccnt = 16383;
    }

    lockclock += TICSPERFRAME;
    drawstatusflytime(screenpeek);   // Andy did this

    if (cameradist >= 0)
    {
        cameradist = min(cameradist+((totalclock-cameraclock)<<10),65536);
        if (keystatus[0x52])       //0
            cameraang -= ((totalclock-cameraclock)<<(2+(keystatus[0x2a]|keystatus[0x36])));
        if (keystatus[0x53])       //.
            cameraang += ((totalclock-cameraclock)<<(2+(keystatus[0x2a]|keystatus[0x36])));
        cameraclock = totalclock;
    }

    for (i=connecthead; i>=0; i=connectpoint2[i])
    {
        processinput(i);                        //Move player

        checktouchsprite(i,cursectnum[i]);      //Pick up coins
        startwall = sector[cursectnum[i]].wallptr;
        endwall = startwall + sector[cursectnum[i]].wallnum;
        for (j=startwall,wal=&wall[j]; j<endwall; j++,wal++)
            if (wal->nextsector >= 0) checktouchsprite(i,wal->nextsector);
    }

    doanimations();
    tagcode();            //Door code, moving sector code, other stuff
    statuslistcode();     //Monster / bullet code / explosions

    fakedomovethingscorrect();

    checkmasterslaveswitch();
}

void getinput(void)
{
    char ch /*, keystate, *ptr*/;
    int i, j /*, k*/;
    int mousx, mousy, bstatus;

    if (typemode == 0)           //if normal game keys active
    {
        if (keystatus[keys[15]])
        {
            keystatus[keys[15]] = 0;

            screenpeek = connectpoint2[screenpeek];
            if (screenpeek < 0) screenpeek = connecthead;
            drawstatusbar(screenpeek);   // Andy did this
        }

        for (i=7; i>=0; i--)
            if (keystatus[i+2])
            { keystatus[i+2] = 0; locselectedgun = i; break; }
    }


    //KEYTIMERSTUFF
    if (!keystatus[keys[5]])
    {
        if (keystatus[keys[2]]) avel = max(avel-16*TICSPERFRAME,-128);
        if (keystatus[keys[3]]) avel = min(avel+16*TICSPERFRAME,127);
    }
    else
    {
        if (keystatus[keys[2]]) svel = min(svel+8*TICSPERFRAME,127);
        if (keystatus[keys[3]]) svel = max(svel-8*TICSPERFRAME,-128);
    }
    if (keystatus[keys[0]]) fvel = min(fvel+8*TICSPERFRAME,127);
    if (keystatus[keys[1]]) fvel = max(fvel-8*TICSPERFRAME,-128);
    if (keystatus[keys[12]]) svel = min(svel+8*TICSPERFRAME,127);
    if (keystatus[keys[13]]) svel = max(svel-8*TICSPERFRAME,-128);

    if (avel < 0) avel = min(avel+12*TICSPERFRAME,0);
    if (avel > 0) avel = max(avel-12*TICSPERFRAME,0);
    if (svel < 0) svel = min(svel+2*TICSPERFRAME,0);
    if (svel > 0) svel = max(svel-2*TICSPERFRAME,0);
    if (fvel < 0) fvel = min(fvel+2*TICSPERFRAME,0);
    if (fvel > 0) fvel = max(fvel-2*TICSPERFRAME,0);

    if ((option[4] == 0) && (numplayers >= 2))
    {
        if (!keystatus[0x4f])
        {
            if (keystatus[0x4b]) avel2 = max(avel2-16*TICSPERFRAME,-128);
            if (keystatus[0x4d]) avel2 = min(avel2+16*TICSPERFRAME,127);
        }
        else
        {
            if (keystatus[0x4b]) svel2 = min(svel2+8*TICSPERFRAME,127);
            if (keystatus[0x4d]) svel2 = max(svel2-8*TICSPERFRAME,-128);
        }
        if (keystatus[0x48]) fvel2 = min(fvel2+8*TICSPERFRAME,127);
        if (keystatus[0x4c]) fvel2 = max(fvel2-8*TICSPERFRAME,-128);

        if (avel2 < 0) avel2 = min(avel2+12*TICSPERFRAME,0);
        if (avel2 > 0) avel2 = max(avel2-12*TICSPERFRAME,0);
        if (svel2 < 0) svel2 = min(svel2+2*TICSPERFRAME,0);
        if (svel2 > 0) svel2 = max(svel2-2*TICSPERFRAME,0);
        if (fvel2 < 0) fvel2 = min(fvel2+2*TICSPERFRAME,0);
        if (fvel2 > 0) fvel2 = max(fvel2-2*TICSPERFRAME,0);
    }

    oscreentilt = screentilt;
    if (keystatus[0x1a]) screentilt += ((4*TICSPERFRAME)<<(keystatus[0x2a]|keystatus[0x36]));
    if (keystatus[0x1b]) screentilt -= ((4*TICSPERFRAME)<<(keystatus[0x2a]|keystatus[0x36]));

    i = (TICSPERFRAME<<1);
    while ((screentilt != 0) && (i > 0))
    { screentilt = ((screentilt+ksgn(screentilt-1024))&2047); i--; }
    if (keystatus[0x28]) screentilt = 1536;


    loc.fvel = min(max(fvel,-128+8),127-8);
    loc.svel = min(max(svel,-128+8),127-8);
    loc.avel = min(max(avel,-128+16),127-16);

    getmousevalues(&mousx,&mousy,&bstatus);
    loc.avel = min(max(loc.avel+(mousx<<3),-128),127);
    loc.fvel = min(max(loc.fvel-(mousy<<3),-128),127);

    loc.bits = (locselectedgun<<13);
    if (typemode == 0)           //if normal game keys active
    {
        loc.bits |= (keystatus[0x32]<<9);                 //M (be master)
        loc.bits |= ((keystatus[keys[14]]==1)<<12);       //Map mode
    }
    loc.bits |= keystatus[keys[8]];                   //Stand high
    loc.bits |= (keystatus[keys[9]]<<1);              //Stand low
    loc.bits |= (keystatus[keys[16]]<<4);             //Zoom in
    loc.bits |= (keystatus[keys[17]]<<5);             //Zoom out
    loc.bits |= (keystatus[keys[4]]<<8);                 //Run
    loc.bits |= (keystatus[keys[10]]<<2);                //Look up
    loc.bits |= (keystatus[keys[11]]<<3);                //Look down
    loc.bits |= ((keystatus[keys[7]]==1)<<10);           //Space
    loc.bits |= ((keystatus[keys[6]]==1)<<11);           //Shoot
    loc.bits |= (((bstatus&6)>(oldmousebstatus&6))<<10); //Space
    loc.bits |= (((bstatus&1)>(oldmousebstatus&1))<<11); //Shoot

    oldmousebstatus = bstatus;
    if (((loc.bits&2048) > 0) && (locselectedgun == 0))
        oldmousebstatus &= ~1;     //Allow continous fire with mouse for chain gun

    //PRIVATE KEYS:
#if 0
    if (keystatus[0xb7])  //Printscreen
    {
        keystatus[0xb7] = 0;
        printscreeninterrupt();
    }
#endif
    if (keystatus[0x2f])       //V
    {
        keystatus[0x2f] = 0;
        if (cameradist < 0) cameradist = 0; else cameradist = -1;
        cameraang = 0;
    }

    if (typemode == 0)           //if normal game keys active
    {
        if (keystatus[0x19])  //P
        {
            keystatus[0x19] = 0;
            parallaxtype++;
            if (parallaxtype > 2) parallaxtype = 0;
        }
        if (keystatus[0x38]|keystatus[0xb8])  //ALT
        {
            if (keystatus[0x4a])  // Keypad -
                g_visibility = min(g_visibility+(g_visibility>>3),16384);
            if (keystatus[0x4e])  // Keypad +
                g_visibility = max(g_visibility-(g_visibility>>3),128);
        }

        if (keystatus[keys[18]])   //Typing mode
        {
            keystatus[keys[18]] = 0;
            typemode = 1;
            bflushchars();
            keyfifoplc = keyfifoend;      //Reset keyboard fifo
        }
    }
    else
    {
        while ((ch = bgetchar()))
        {
            if (ch == 8)   //Backspace
            {
                if (typemessageleng == 0) { typemode = 0; break; }
                typemessageleng--;
            }
            else if (ch == 9)   // tab
            {
                keystatus[0xf] = 0;
                typemode = 0;
                break;
            }
            else if (ch == 13)  //Either ENTER
            {
                keystatus[0x1c] = 0; keystatus[0x9c] = 0;
                if (typemessageleng > 0)
                {
                    packbuf[0] = 2;          //Sending text is message type 4
                    for (j=typemessageleng-1; j>=0; j--)
                        packbuf[j+1] = typemessage[j];

                    for (i=connecthead; i>=0; i=connectpoint2[i])
                        if (i != myconnectindex)
                            sendpacket(i,packbuf,typemessageleng+1);

                    typemessageleng = 0;
                }
                typemode = 0;
                break;
            }
            else if ((typemessageleng < 159) && (ch >= 32) && (ch < 128))
            {
                typemessage[typemessageleng++] = ch;
            }
        }
    }
}

void initplayersprite(short snum)
{
    int i;

    if (playersprite[snum] >= 0) return;

    spawnsprite(playersprite[snum],pos[snum].x,pos[snum].y,pos[snum].z+EYEHEIGHT,
                1+256,0,snum,32,64,64,0,0,PLAYER,ang[snum],0,0,0,snum+4096,
                cursectnum[snum],8,0,0,0);

    switch (snum)
    {
    case 1: for (i=0; i<32; i++) tempbuf[i+192] = i+128; break; //green->red
    case 2: for (i=0; i<32; i++) tempbuf[i+192] = i+32; break; //green->blue
    case 3: for (i=0; i<32; i++) tempbuf[i+192] = i+224; break; //green->pink
    case 4: for (i=0; i<32; i++) tempbuf[i+192] = i+64; break; //green->brown
    case 5: for (i=0; i<32; i++) tempbuf[i+192] = i+96; break;
    case 6: for (i=0; i<32; i++) tempbuf[i+192] = i+160; break;
    case 7: for (i=0; i<32; i++) tempbuf[i+192] = i+192; break;
    default: for (i=0; i<256; i++) tempbuf[i] = i; break;
    }
    makepalookup(snum,tempbuf,0,0,0,1);
}

void playback(void)
{
    int i, j, k;

    ready2send = 0;
    recstat = 0; i = reccnt;
    while (!keystatus[1])
    {
        if (handleevents())
        {
            if (quitevent)
            {
                keystatus[1] = 1;
                quitevent = 0;
            }
        }

        refreshaudio();

        while (totalclock >= lockclock+TICSPERFRAME)
        {
            sampletimer();
            if (i >= reccnt)
            {
                prepareboard(boardfilename);
                for (i=connecthead; i>=0; i=connectpoint2[i])
                    initplayersprite((short)i);
                totalclock = 0;
                i = 0;
            }

            k = 0;
            for (j=connecthead; j>=0; j=connectpoint2[j])
            {
                copybufbyte(&recsync[i][k],&ffsync[j],sizeof(input));
                k++;
            }
            movethings(); domovethings();
            i++;
        }
        drawscreen(screenpeek,(totalclock-gotlastpacketclock)*(65536/(TIMERINTSPERSECOND/MOVESPERSECOND)));

        if (keystatus[keys[15]])
        {
            keystatus[keys[15]] = 0;
            screenpeek = connectpoint2[screenpeek];
            if (screenpeek < 0) screenpeek = connecthead;
            drawstatusbar(screenpeek);   // Andy did this
        }
        if (keystatus[keys[14]])
        {
            keystatus[keys[14]] = 0;
            dimensionmode[screenpeek]++;
            if (dimensionmode[screenpeek] > 3) dimensionmode[screenpeek] = 1;
        }
    }

    musicoff();
    uninitmultiplayers();
    uninittimer();
    uninitinput();
    uninitengine();
    uninitsb();
    uninitgroupfile();
    exit(0);
}

void setup3dscreen(void)
{
    int i, dax, day, dax2, day2;

    i = setgamemode(fullscreen,xdimgame,ydimgame,bppgame);
    if (i < 0)
    {
        printf("Error setting video mode.\n");
        sendlogoff();
        musicoff();
        uninitmultiplayers();
        uninittimer();
        uninitinput();
        uninitengine();
        uninitsb();
        uninitgroupfile();
        exit(0);
    }

#if 0
    //Make that ugly pink into black in case it ever shows up!
    i = 0L;
    setpalette(255,1,(char *)&i);
    //outp(0x3c8,255); outp(0x3c9,0); outp(0x3c9,0); outp(0x3c9,0);
#endif

    screensize = xdim;
    if (screensize > xdim)
    {
        dax = 0; day = 0;
        dax2 = xdim-1; day2 = ydim-1;
    }
    else
    {
        dax = ((xdim-screensize)>>1);
        dax2 = dax+screensize-1;
        day = (((ydim-32)-scale(screensize,ydim-32,xdim))>>1);
        day2 = day + scale(screensize,ydim-32,xdim)-1;
        setview(dax,day,dax2,day2);
    }

    flushperms();

    if (screensize < xdim)
        drawtilebackground(/*0L,0L,*/ BACKGROUND,8,0L,0L,xdim-1L,ydim-1L,0);     //Draw background

    if (screensize <= xdim)
    {
        rotatesprite((xdim-320)<<15,(ydim-32)<<16,65536L,0,STATUSBAR,0,0,8+16+64+128,0L,0L,xdim-1L,ydim-1L);
        i = ((xdim-320)>>1);
        while (i >= 8) i -= 8, rotatesprite(i<<16,(ydim-32)<<16,65536L,0,STATUSBARFILL8,0,0,8+16+64+128,0L,0L,xdim-1L,ydim-1L);
        if (i >= 4) i -= 4, rotatesprite(i<<16,(ydim-32)<<16,65536L,0,STATUSBARFILL4,0,0,8+16+64+128,0L,0L,xdim-1L,ydim-1L);
        i = ((xdim-320)>>1)+320;
        while (i <= xdim-8) rotatesprite(i<<16,(ydim-32)<<16,65536L,0,STATUSBARFILL8,0,0,8+16+64+128,0L,0L,xdim-1L,ydim-1L), i += 8;
        if (i <= xdim-4) rotatesprite(i<<16,(ydim-32)<<16,65536L,0,STATUSBARFILL4,0,0,8+16+64+128,0L,0L,xdim-1L,ydim-1L), i += 4;

        drawstatusbar(screenpeek);   // Andy did this
    }
}

void findrandomspot(int *x, int *y, short *sectnum)
{
    short startwall, endwall, s, dasector;
    vec3_t da = { 0, 0, 0 };
    int minx, maxx, miny, maxy, cnt;

    for (cnt=256; cnt>=0; cnt--)
    {
        do
        {
            dasector = mulscale16(krand(),numsectors);
        }
        while ((sector[dasector].ceilingz+(8<<8) >= sector[dasector].floorz) || ((sector[dasector].lotag|sector[dasector].hitag) != 0) || ((sector[dasector].floorstat&1) != 0));

        startwall = sector[dasector].wallptr;
        endwall = startwall+sector[dasector].wallnum;
        if (endwall <= startwall) continue;

        minx = 0x7fffffff; maxx = 0x80000000;
        miny = 0x7fffffff; maxy = 0x80000000;

        for (s=startwall; s<endwall; s++)
        {
            da.x += wall[s].x;
            da.y += wall[s].y;
            if (wall[s].x < minx) minx = wall[s].x;
            if (wall[s].x > maxx) maxx = wall[s].x;
            if (wall[s].y < miny) miny = wall[s].y;
            if (wall[s].y > maxy) maxy = wall[s].y;
        }

        if ((maxx-minx <= 256) || (maxy-miny <= 256)) continue;

        da.x /= (endwall-startwall);
        da.y /= (endwall-startwall);

        if (inside(da.x,da.y,dasector) == 0) continue;

        da.z = sector[dasector].floorz-(32<<8);
        if (pushmove(&da,&dasector,128L,4<<8,4<<8,CLIPMASK0) < 0) continue;

        *x = da.x; *y = da.y; *sectnum = dasector;
        return;
    }
}

void warp(int *x, int *y, int *z, short *daang, short *dasector)
{
    short startwall, endwall, s;
    int i, j, dax, day, ox, oy;

    ox = *x; oy = *y;

    for (i=0; i<warpsectorcnt; i++)
        if (warpsectorlist[i] == *dasector)
        {
            j = sector[*dasector].hitag;
            do
            {
                i++;
                if (i >= warpsectorcnt) i = 0;
            }
            while (sector[warpsectorlist[i]].hitag != j);
            *dasector = warpsectorlist[i];
            break;
        }

    //Find center of sector
    startwall = sector[*dasector].wallptr;
    endwall = startwall+sector[*dasector].wallnum;
    dax = 0L, day = 0L;
    for (s=startwall; s<endwall; s++)
    {
        dax += wall[s].x, day += wall[s].y;
        if (wall[s].nextsector >= 0)
            i = s;
    }
    *x = dax / (endwall-startwall);
    *y = day / (endwall-startwall);
    *z = sector[*dasector].floorz-(32<<8);
    updatesector(*x,*y,dasector);
    dax = ((wall[i].x+wall[wall[i].point2].x)>>1);
    day = ((wall[i].y+wall[wall[i].point2].y)>>1);
    *daang = getangle(dax-*x,day-*y);

    wsayfollow("warp.wav",3072L+(krand()&127)-64,192L,&ox,&oy,0);
    wsayfollow("warp.wav",4096L+(krand()&127)-64,256L,x,y,0);
}

void warpsprite(short spritenum)
{
    short dasectnum;

    dasectnum = sprite[spritenum].sectnum;
    warp(&sprite[spritenum].x,&sprite[spritenum].y,&sprite[spritenum].z,
         &sprite[spritenum].ang,&dasectnum);

    copybuf(&sprite[spritenum].x,&osprite[spritenum].x,3);
    changespritesect(spritenum,dasectnum);

    show2dsprite[spritenum>>3] &= ~(1<<(spritenum&7));
    if (show2dsector[dasectnum>>3]&(1<<(dasectnum&7)))
        show2dsprite[spritenum>>3] |= (1<<(spritenum&7));
}

void initlava(void)
{
    int x, y, z, r;

    for (z=0; z<32; z++) lavaradcnt[z] = 0;
    for (x=-16; x<=16; x++)
        for (y=-16; y<=16; y++)
        {
            r = ksqrt(x*x + y*y);
            lavaradx[r][lavaradcnt[r]] = x;
            lavarady[r][lavaradcnt[r]] = y;
            lavaradcnt[r]++;
        }

    for (z=0; z<16; z++)
        lavadropsizlookup[z] = 8 / (ksqrt(z)+1);

    for (z=0; z<LAVASIZ; z++)
        lavainc[z] = klabs((((z^17)>>4)&7)-4)+12;

    lavanumdrops = 0;
    lavanumframes = 0;
}

#if defined(__WATCOMC__) && !defined(NOASM)
#pragma aux addlava = \
    "mov al, byte ptr [ebx-133]", \
    "mov dl, byte ptr [ebx-1]", \
    "add al, byte ptr [ebx-132]", \
    "add dl, byte ptr [ebx+131]", \
    "add al, byte ptr [ebx-131]", \
    "add dl, byte ptr [ebx+132]", \
    "add al, byte ptr [ebx+1]", \
    "add al, dl", \
    parm [ebx] \
    modify exact [eax edx]
int addlava(int);
#elif defined(_MSC_VER) && !defined(NOASM)
int addlava(void *b)
{
    _asm
    {
        mov ebx, b
        mov al, byte ptr [ebx-133]
        mov dl, byte ptr [ebx-1]
        add al, byte ptr [ebx-132]
        add dl, byte ptr [ebx+131]
        add al, byte ptr [ebx-131]
        add dl, byte ptr [ebx+132]
        add al, byte ptr [ebx+1]
        add al, dl
    }
}
#elif defined(__GNUC__) && defined(__i386__) && !defined(NOASM)
int addlava(void *b)
{
    int r;
    __asm__ __volatile__(
        "movb -133(%%ebx), %%al\n\t"
        "movb -1(%%ebx), %%dl\n\t"
        "addb -132(%%ebx), %%al\n\t"
        "addb 131(%%ebx), %%dl\n\t"
        "addb -131(%%ebx), %%al\n\t"
        "addb 132(%%ebx), %%dl\n\t"
        "addb 1(%%ebx), %%al\n\t"
        "addb %%dl, %%al"
        : "=a" (r) : "b" (b)
        : "dx"
        );
    return r;
}
#else
int addlava(void *bx)
{
    char *b = (char *)bx;
    return b[-133] + b[-132] + b[-131] + b[1] + b[-1] + b[131] + b[132];
}
#endif

void movelava(char *dapic)
{
    int i, /*j,*/ x, y, z, zz, dalavadropsiz, dadropsizlookup;
    int dalavax, dalavay, *ptr, *ptr2;
    char *pi, *pj, *py;

    for (z=min(LAVAMAXDROPS-lavanumdrops-1,3); z>=0; z--)
    {
        lavadropx[lavanumdrops] = (Brand()&(LAVASIZ-1));
        lavadropy[lavanumdrops] = (Brand()&(LAVASIZ-1));
        lavadropsiz[lavanumdrops] = 1;
        lavanumdrops++;
    }

    for (z=lavanumdrops-1; z>=0; z--)
    {
        dadropsizlookup = lavadropsizlookup[lavadropsiz[z]]*(((z&1)<<1)-1);
        dalavadropsiz = lavadropsiz[z];
        dalavax = lavadropx[z]; dalavay = lavadropy[z];
        for (zz=lavaradcnt[lavadropsiz[z]]-1; zz>=0; zz--)
        {
            i = (((lavaradx[dalavadropsiz][zz]+dalavax)&(LAVASIZ-1))<<LAVALOGSIZ);
            i += ((lavarady[dalavadropsiz][zz]+dalavay)&(LAVASIZ-1));
            dapic[i] += dadropsizlookup;
            if (dapic[i] < 192) dapic[i] = 192;
        }

        lavadropsiz[z]++;
        if (lavadropsiz[z] > 10)
        {
            lavanumdrops--;
            lavadropx[z] = lavadropx[lavanumdrops];
            lavadropy[z] = lavadropy[lavanumdrops];
            lavadropsiz[z] = lavadropsiz[lavanumdrops];
        }
    }

    //Back up dapic with 1 pixel extra on each boundary
    //(to prevent anding for wrap-around)
    ptr = (int *)dapic;
    ptr2 = (int *)((LAVASIZ+4)+1+((intptr_t)lavabakpic));
    for (x=0; x<LAVASIZ; x++)
    {
        for (y=(LAVASIZ>>2); y>0; y--) *ptr2++ = ((*ptr++)&0x1f1f1f1f);
        ptr2++;
    }
    for (y=0; y<LAVASIZ; y++)
    {
        lavabakpic[y+1] = (dapic[y+((LAVASIZ-1)<<LAVALOGSIZ)]&31);
        lavabakpic[y+1+(LAVASIZ+1)*(LAVASIZ+4)] = (dapic[y]&31);
    }
    for (x=0; x<LAVASIZ; x++)
    {
        lavabakpic[(x+1)*(LAVASIZ+4)] = (dapic[(x<<LAVALOGSIZ)+(LAVASIZ-1)]&31);
        lavabakpic[(x+1)*(LAVASIZ+4)+(LAVASIZ+1)] = (dapic[x<<LAVALOGSIZ]&31);
    }
    lavabakpic[0] = (dapic[LAVASIZ*LAVASIZ-1]&31);
    lavabakpic[LAVASIZ+1] = (dapic[LAVASIZ*(LAVASIZ-1)]&31);
    lavabakpic[(LAVASIZ+4)*(LAVASIZ+1)] = (dapic[LAVASIZ-1]&31);
    lavabakpic[(LAVASIZ+4)*(LAVASIZ+2)-1] = (dapic[0]&31);

    ptr = (int *)dapic;
    for (x=0; x<LAVASIZ; x++)
    {
        pi = &lavabakpic[(x+1)*(LAVASIZ+4)+1];
        pj = pi+LAVASIZ;
        for (py=pi; py<pj; py+=4)
        {
            *ptr++ = ((addlava(&py[0])&0xf8)>>3)+
                     ((addlava(&py[1])&0xf8)<<5)+
                     ((addlava(&py[2])&0xf8)<<13)+
                     ((addlava(&py[3])&0xf8)<<21)+
                     0xc2c2c2c2;
        }
    }

    lavanumframes++;
}

void doanimations(void)
{
    int i, j;

    for (i=animatecnt-1; i>=0; i--)
    {
        j = *animateptr[i];

        if (j < animategoal[i])
            j = min(j+animatevel[i]*TICSPERFRAME,animategoal[i]);
        else
            j = max(j-animatevel[i]*TICSPERFRAME,animategoal[i]);
        animatevel[i] += animateacc[i];

        *animateptr[i] = j;

        if (j == animategoal[i])
        {
            animatecnt--;
            if (i != animatecnt)
            {
                stopinterpolation(animateptr[i]);
                animateptr[i] = animateptr[animatecnt];
                animategoal[i] = animategoal[animatecnt];
                animatevel[i] = animatevel[animatecnt];
                animateacc[i] = animateacc[animatecnt];
            }
        }
    }
}

int getanimationgoal(int *animptr)
{
    int i;

    for (i=animatecnt-1; i>=0; i--)
        if (animptr == animateptr[i]) return i;
    return -1;
}

int setanimation(int *animptr, int thegoal, int thevel, int theacc)
{
    int i, j;

    if (animatecnt >= MAXANIMATES) return -1;

    j = animatecnt;
    for (i=animatecnt-1; i>=0; i--)
        if (animptr == animateptr[i])
        { j = i; break; }

    setinterpolation(animptr);

    animateptr[j] = animptr;
    animategoal[j] = thegoal;
    animatevel[j] = thevel;
    animateacc[j] = theacc;
    if (j == animatecnt) animatecnt++;
    return j;
}

void checkmasterslaveswitch(void)
{
    int i, j;

    if (option[4] == 0) return;

    j = 0;
    for (i=connecthead; i>=0; i=connectpoint2[i])
        if (ssync[i].bits&512) j++;
    if (j != 1) return;

    i = connecthead;
    for (j=connectpoint2[i]; j>=0; j=connectpoint2[j])
    {
        if (ssync[j].bits&512)
        {
            connectpoint2[i] = connectpoint2[j];
            connectpoint2[j] = connecthead;
            connecthead = (short)j;

            oloc.fvel = loc.fvel+1;
            oloc.svel = loc.svel+1;
            oloc.avel = loc.avel+1;
            oloc.bits = loc.bits+1;
            for (i=0; i<MAXPLAYERS; i++)
            {
                osync[i].fvel = ffsync[i].fvel+1;
                osync[i].svel = ffsync[i].svel+1;
                osync[i].avel = ffsync[i].avel+1;
                osync[i].bits = ffsync[i].bits+1;
            }

            syncvalhead = othersyncvalhead = syncvaltail = 0L;
            totalclock = ototalclock = gotlastpacketclock = lockclock;

            j = 1;
            for (i=connecthead; i>=0; i=connectpoint2[i])
            {
                if (myconnectindex == i) break;
                j++;
            }
            if (j == 1)
                Bstrcpy(getmessage,"Player 1 (Master)");
            else
                Bsprintf(getmessage,"Player %d (Slave)",j);
            getmessageleng = Bstrlen(getmessage);
            getmessagetimeoff = totalclock+120;

            return;
        }
        i = j;
    }
}


int testneighborsectors(short sect1, short sect2)
{
    short i, startwall, num1, num2;

    num1 = sector[sect1].wallnum;
    num2 = sector[sect2].wallnum;
    if (num1 < num2) //Traverse walls of sector with fewest walls (for speed)
    {
        startwall = sector[sect1].wallptr;
        for (i=num1-1; i>=0; i--)
            if (wall[i+startwall].nextsector == sect2)
                return 1;
    }
    else
    {
        startwall = sector[sect2].wallptr;
        for (i=num2-1; i>=0; i--)
            if (wall[i+startwall].nextsector == sect1)
                return 1;
    }
    return 0;
}

int loadgame(void)
{
    int dummy = 0;
    int i;
    int fil;
    int tmpanimateptr[MAXANIMATES];

    if ((fil = kopen4load("save0000.gam",0)) == -1) return -1;

    kdfread(&numplayers,4,1,fil);
    kdfread(&myconnectindex,4,1,fil);
    kdfread(&connecthead,4,1,fil);
    kdfread(connectpoint2,4,MAXPLAYERS,fil);

    //Make sure palookups get set, sprites will get overwritten later
    for (i=connecthead; i>=0; i=connectpoint2[i]) initplayersprite((short)i);

    for (i = 0; i < MAXPLAYERS; ++i)
        kdfread(&pos[i].x,4,1,fil);
    for (i = 0; i < MAXPLAYERS; ++i)
        kdfread(&pos[i].y,4,1,fil);
    for (i = 0; i < MAXPLAYERS; ++i)
        kdfread(&pos[i].z,4,1,fil);

    kdfread(horiz,4,MAXPLAYERS,fil);
    kdfread(zoom,4,MAXPLAYERS,fil);
    kdfread(hvel,4,MAXPLAYERS,fil);
    kdfread(ang,2,MAXPLAYERS,fil);
    kdfread(cursectnum,2,MAXPLAYERS,fil);
    kdfread(ocursectnum,2,MAXPLAYERS,fil);
    kdfread(playersprite,2,MAXPLAYERS,fil);
    kdfread(deaths,2,MAXPLAYERS,fil);
    kdfread(lastchaingun,4,MAXPLAYERS,fil);
    kdfread(health,4,MAXPLAYERS,fil);
    kdfread(numgrabbers,2,MAXPLAYERS,fil);
    kdfread(nummissiles,2,MAXPLAYERS,fil);
    kdfread(numbombs,2,MAXPLAYERS,fil);
    kdfread(flytime,4,MAXPLAYERS,fil);
    kdfread(oflags,2,MAXPLAYERS,fil);
    kdfread(dimensionmode,1,MAXPLAYERS,fil);
    kdfread(revolvedoorstat,1,MAXPLAYERS,fil);
    kdfread(revolvedoorang,2,MAXPLAYERS,fil);
    kdfread(revolvedoorrotang,2,MAXPLAYERS,fil);
    kdfread(revolvedoorx,4,MAXPLAYERS,fil);
    kdfread(revolvedoory,4,MAXPLAYERS,fil);

    kdfread(&numsectors,2,1,fil);
    kdfread(sector,sizeof(sectortype),numsectors,fil);
    kdfread(&numwalls,2,1,fil);
    kdfread(wall,sizeof(walltype),numwalls,fil);
    //Store all sprites (even holes) to preserve indeces
    kdfread(sprite,sizeof(spritetype),MAXSPRITES,fil);
    kdfread(headspritesect,2,MAXSECTORS+1,fil);
    kdfread(prevspritesect,2,MAXSPRITES,fil);
    kdfread(nextspritesect,2,MAXSPRITES,fil);
    kdfread(headspritestat,2,MAXSTATUS+1,fil);
    kdfread(prevspritestat,2,MAXSPRITES,fil);
    kdfread(nextspritestat,2,MAXSPRITES,fil);

    kdfread(&fvel,4,1,fil);
    kdfread(&svel,4,1,fil);
    kdfread(&avel,4,1,fil);

    kdfread(&locselectedgun,4,1,fil);
    kdfread(&loc.fvel,1,1,fil);
    kdfread(&oloc.fvel,1,1,fil);
    kdfread(&loc.svel,1,1,fil);
    kdfread(&oloc.svel,1,1,fil);
    kdfread(&loc.avel,1,1,fil);
    kdfread(&oloc.avel,1,1,fil);
    kdfread(&loc.bits,2,1,fil);
    kdfread(&oloc.bits,2,1,fil);

    kdfread(&locselectedgun2,4,1,fil);
    kdfread(&loc2.fvel,sizeof(input),1,fil);

    kdfread(ssync,sizeof(input),MAXPLAYERS,fil);
    kdfread(osync,sizeof(input),MAXPLAYERS,fil);

    kdfread(boardfilename,1,80,fil);
    kdfread(&screenpeek,2,1,fil);
    kdfread(&oldmousebstatus,2,1,fil);
    kdfread(&brightness,2,1,fil);
    kdfread(&neartagsector,2,1,fil);
    kdfread(&neartagwall,2,1,fil);
    kdfread(&neartagsprite,2,1,fil);
    kdfread(&lockclock,4,1,fil);
    kdfread(&neartagdist,4,1,fil);
    kdfread(&neartaghitdist,4,1,fil);

    kdfread(turnspritelist,2,16,fil);
    kdfread(&turnspritecnt,2,1,fil);
    kdfread(warpsectorlist,2,16,fil);
    kdfread(&warpsectorcnt,2,1,fil);
    kdfread(xpanningsectorlist,2,16,fil);
    kdfread(&xpanningsectorcnt,2,1,fil);
    kdfread(ypanningwalllist,2,64,fil);
    kdfread(&ypanningwallcnt,2,1,fil);
    kdfread(floorpanninglist,2,64,fil);
    kdfread(&floorpanningcnt,2,1,fil);
    kdfread(dragsectorlist,2,16,fil);
    kdfread(dragxdir,2,16,fil);
    kdfread(dragydir,2,16,fil);
    kdfread(&dragsectorcnt,2,1,fil);
    kdfread(dragx1,4,16,fil);
    kdfread(dragy1,4,16,fil);
    kdfread(dragx2,4,16,fil);
    kdfread(dragy2,4,16,fil);
    kdfread(dragfloorz,4,16,fil);
    kdfread(&swingcnt,2,1,fil);
    kdfread(swingwall,2,32*5,fil);
    kdfread(swingsector,2,32,fil);
    kdfread(swingangopen,2,32,fil);
    kdfread(swingangclosed,2,32,fil);
    kdfread(swingangopendir,2,32,fil);
    kdfread(swingang,2,32,fil);
    kdfread(swinganginc,2,32,fil);
    kdfread(swingx,4,32*8,fil);
    kdfread(swingy,4,32*8,fil);
    kdfread(revolvesector,2,4,fil);
    kdfread(revolveang,2,4,fil);
    kdfread(&revolvecnt,2,1,fil);
    kdfread(revolvex,4,4*16,fil);
    kdfread(revolvey,4,4*16,fil);
    kdfread(revolvepivotx,4,4,fil);
    kdfread(revolvepivoty,4,4,fil);
    kdfread(subwaytracksector,2,4*128,fil);
    kdfread(subwaynumsectors,2,4,fil);
    kdfread(&subwaytrackcnt,2,1,fil);
    kdfread(subwaystop,4,4*8,fil);
    kdfread(subwaystopcnt,4,4,fil);
    kdfread(subwaytrackx1,4,4,fil);
    kdfread(subwaytracky1,4,4,fil);
    kdfread(subwaytrackx2,4,4,fil);
    kdfread(subwaytracky2,4,4,fil);
    kdfread(subwayx,4,4,fil);
    kdfread(subwaygoalstop,4,4,fil);
    kdfread(subwayvel,4,4,fil);
    kdfread(subwaypausetime,4,4,fil);
    kdfread(waterfountainwall,2,MAXPLAYERS,fil);
    kdfread(waterfountaincnt,2,MAXPLAYERS,fil);
    kdfread(slimesoundcnt,2,MAXPLAYERS,fil);

    //Warning: only works if all pointers are in sector structures!
    kdfread(tmpanimateptr,4,MAXANIMATES,fil);
    for (i=MAXANIMATES-1; i>=0; i--)
        animateptr[i] = (int *)(tmpanimateptr[i]+(intptr_t)sector);

    kdfread(animategoal,4,MAXANIMATES,fil);
    kdfread(animatevel,4,MAXANIMATES,fil);
    kdfread(animateacc,4,MAXANIMATES,fil);
    kdfread(&animatecnt,4,1,fil);

    kdfread(&totalclock,4,1,fil);
    kdfread(&numframes,4,1,fil);
    kdfread(&randomseed,4,1,fil);
    kdfread(&numshades,2,1,fil);

    kdfread(&g_visibility,4,1,fil);
    kdfread(&parallaxvisibility,4,1,fil);
    kdfread(&parallaxtype,1,1,fil);
    kdfread(&dummy,4,1,fil);
    kdfread(&dummy,2,MAXPSKYTILES,fil);
    kdfread(&dummy,2,1,fil);

    kdfread(&mirrorcnt,2,1,fil);
    kdfread(mirrorwall,2,mirrorcnt,fil);
    kdfread(mirrorsector,2,mirrorcnt,fil);

    //I should save off interpolation list, but they're pointers :(
    numinterpolations = 0;
    startofdynamicinterpolations = 0;

    kclose(fil);

    for (i=connecthead; i>=0; i=connectpoint2[i]) initplayersprite((short)i);

    totalclock = lockclock;
    ototalclock = lockclock;

    Bstrcpy(getmessage,"Game loaded.");
    getmessageleng = Bstrlen(getmessage);
    getmessagetimeoff = totalclock+360+(getmessageleng<<4);
    return 0;
}

int savegame(void)
{
    int dummy = 0;
    int i;
    BFILE *fil;
    int tmpanimateptr[MAXANIMATES];

    if ((fil = Bfopen("save0000.gam","wb")) == 0) return -1;

    dfwrite(&numplayers,4,1,fil);
    dfwrite(&myconnectindex,4,1,fil);
    dfwrite(&connecthead,4,1,fil);
    dfwrite(connectpoint2,4,MAXPLAYERS,fil);

    for (i = 0; i < MAXPLAYERS; ++i)
        dfwrite(&pos[i].x,4,1,fil);
    for (i = 0; i < MAXPLAYERS; ++i)
        dfwrite(&pos[i].y,4,1,fil);
    for (i = 0; i < MAXPLAYERS; ++i)
        dfwrite(&pos[i].z,4,1,fil);

    dfwrite(horiz,4,MAXPLAYERS,fil);
    dfwrite(zoom,4,MAXPLAYERS,fil);
    dfwrite(hvel,4,MAXPLAYERS,fil);
    dfwrite(ang,2,MAXPLAYERS,fil);
    dfwrite(cursectnum,2,MAXPLAYERS,fil);
    dfwrite(ocursectnum,2,MAXPLAYERS,fil);
    dfwrite(playersprite,2,MAXPLAYERS,fil);
    dfwrite(deaths,2,MAXPLAYERS,fil);
    dfwrite(lastchaingun,4,MAXPLAYERS,fil);
    dfwrite(health,4,MAXPLAYERS,fil);
    dfwrite(numgrabbers,2,MAXPLAYERS,fil);
    dfwrite(nummissiles,2,MAXPLAYERS,fil);
    dfwrite(numbombs,2,MAXPLAYERS,fil);
    dfwrite(flytime,4,MAXPLAYERS,fil);
    dfwrite(oflags,2,MAXPLAYERS,fil);
    dfwrite(dimensionmode,1,MAXPLAYERS,fil);
    dfwrite(revolvedoorstat,1,MAXPLAYERS,fil);
    dfwrite(revolvedoorang,2,MAXPLAYERS,fil);
    dfwrite(revolvedoorrotang,2,MAXPLAYERS,fil);
    dfwrite(revolvedoorx,4,MAXPLAYERS,fil);
    dfwrite(revolvedoory,4,MAXPLAYERS,fil);

    dfwrite(&numsectors,2,1,fil);
    dfwrite(sector,sizeof(sectortype),numsectors,fil);
    dfwrite(&numwalls,2,1,fil);
    dfwrite(wall,sizeof(walltype),numwalls,fil);
    //Store all sprites (even holes) to preserve indeces
    dfwrite(sprite,sizeof(spritetype),MAXSPRITES,fil);
    dfwrite(headspritesect,2,MAXSECTORS+1,fil);
    dfwrite(prevspritesect,2,MAXSPRITES,fil);
    dfwrite(nextspritesect,2,MAXSPRITES,fil);
    dfwrite(headspritestat,2,MAXSTATUS+1,fil);
    dfwrite(prevspritestat,2,MAXSPRITES,fil);
    dfwrite(nextspritestat,2,MAXSPRITES,fil);

    dfwrite(&fvel,4,1,fil);
    dfwrite(&svel,4,1,fil);
    dfwrite(&avel,4,1,fil);

    dfwrite(&locselectedgun,4,1,fil);
    dfwrite(&loc.fvel,1,1,fil);
    dfwrite(&oloc.fvel,1,1,fil);
    dfwrite(&loc.svel,1,1,fil);
    dfwrite(&oloc.svel,1,1,fil);
    dfwrite(&loc.avel,1,1,fil);
    dfwrite(&oloc.avel,1,1,fil);
    dfwrite(&loc.bits,2,1,fil);
    dfwrite(&oloc.bits,2,1,fil);

    dfwrite(&locselectedgun2,4,1,fil);
    dfwrite(&loc2.fvel,sizeof(input),1,fil);

    dfwrite(ssync,sizeof(input),MAXPLAYERS,fil);
    dfwrite(osync,sizeof(input),MAXPLAYERS,fil);

    dfwrite(boardfilename,1,80,fil);
    dfwrite(&screenpeek,2,1,fil);
    dfwrite(&oldmousebstatus,2,1,fil);
    dfwrite(&brightness,2,1,fil);
    dfwrite(&neartagsector,2,1,fil);
    dfwrite(&neartagwall,2,1,fil);
    dfwrite(&neartagsprite,2,1,fil);
    dfwrite(&lockclock,4,1,fil);
    dfwrite(&neartagdist,4,1,fil);
    dfwrite(&neartaghitdist,4,1,fil);

    dfwrite(turnspritelist,2,16,fil);
    dfwrite(&turnspritecnt,2,1,fil);
    dfwrite(warpsectorlist,2,16,fil);
    dfwrite(&warpsectorcnt,2,1,fil);
    dfwrite(xpanningsectorlist,2,16,fil);
    dfwrite(&xpanningsectorcnt,2,1,fil);
    dfwrite(ypanningwalllist,2,64,fil);
    dfwrite(&ypanningwallcnt,2,1,fil);
    dfwrite(floorpanninglist,2,64,fil);
    dfwrite(&floorpanningcnt,2,1,fil);
    dfwrite(dragsectorlist,2,16,fil);
    dfwrite(dragxdir,2,16,fil);
    dfwrite(dragydir,2,16,fil);
    dfwrite(&dragsectorcnt,2,1,fil);
    dfwrite(dragx1,4,16,fil);
    dfwrite(dragy1,4,16,fil);
    dfwrite(dragx2,4,16,fil);
    dfwrite(dragy2,4,16,fil);
    dfwrite(dragfloorz,4,16,fil);
    dfwrite(&swingcnt,2,1,fil);
    dfwrite(swingwall,2,32*5,fil);
    dfwrite(swingsector,2,32,fil);
    dfwrite(swingangopen,2,32,fil);
    dfwrite(swingangclosed,2,32,fil);
    dfwrite(swingangopendir,2,32,fil);
    dfwrite(swingang,2,32,fil);
    dfwrite(swinganginc,2,32,fil);
    dfwrite(swingx,4,32*8,fil);
    dfwrite(swingy,4,32*8,fil);
    dfwrite(revolvesector,2,4,fil);
    dfwrite(revolveang,2,4,fil);
    dfwrite(&revolvecnt,2,1,fil);
    dfwrite(revolvex,4,4*16,fil);
    dfwrite(revolvey,4,4*16,fil);
    dfwrite(revolvepivotx,4,4,fil);
    dfwrite(revolvepivoty,4,4,fil);
    dfwrite(subwaytracksector,2,4*128,fil);
    dfwrite(subwaynumsectors,2,4,fil);
    dfwrite(&subwaytrackcnt,2,1,fil);
    dfwrite(subwaystop,4,4*8,fil);
    dfwrite(subwaystopcnt,4,4,fil);
    dfwrite(subwaytrackx1,4,4,fil);
    dfwrite(subwaytracky1,4,4,fil);
    dfwrite(subwaytrackx2,4,4,fil);
    dfwrite(subwaytracky2,4,4,fil);
    dfwrite(subwayx,4,4,fil);
    dfwrite(subwaygoalstop,4,4,fil);
    dfwrite(subwayvel,4,4,fil);
    dfwrite(subwaypausetime,4,4,fil);
    dfwrite(waterfountainwall,2,MAXPLAYERS,fil);
    dfwrite(waterfountaincnt,2,MAXPLAYERS,fil);
    dfwrite(slimesoundcnt,2,MAXPLAYERS,fil);

    //Warning: only works if all pointers are in sector structures!
    for (i=MAXANIMATES-1; i>=0; i--)
        tmpanimateptr[i] = (int)((intptr_t)animateptr[i]-(intptr_t)sector);
    dfwrite(tmpanimateptr,4,MAXANIMATES,fil);

    dfwrite(animategoal,4,MAXANIMATES,fil);
    dfwrite(animatevel,4,MAXANIMATES,fil);
    dfwrite(animateacc,4,MAXANIMATES,fil);
    dfwrite(&animatecnt,4,1,fil);

    dfwrite(&totalclock,4,1,fil);
    dfwrite(&numframes,4,1,fil);
    dfwrite(&randomseed,4,1,fil);
    dfwrite(&numshades,2,1,fil);

    dfwrite(&g_visibility,4,1,fil);
    dfwrite(&parallaxvisibility,4,1,fil);
    dfwrite(&parallaxtype,1,1,fil);
    dfwrite(&dummy,4,1,fil);
    dfwrite(&dummy,2,MAXPSKYTILES,fil);
    dfwrite(&dummy,2,1,fil);

    dfwrite(&mirrorcnt,2,1,fil);
    dfwrite(mirrorwall,2,mirrorcnt,fil);
    dfwrite(mirrorsector,2,mirrorcnt,fil);

    Bfclose(fil);

    Bstrcpy(getmessage,"Game saved.");
    getmessageleng = Bstrlen(getmessage);
    getmessagetimeoff = totalclock+360+(getmessageleng<<4);
    return 0;
}

void faketimerhandler(void)
{
    short other /*, packbufleng*/;
    int i, j, k, l;

    sampletimer();
    if ((totalclock < ototalclock+(TIMERINTSPERSECOND/MOVESPERSECOND)) || (ready2send == 0)) return;
    ototalclock += (TIMERINTSPERSECOND/MOVESPERSECOND);

    getpackets();
    if (getoutputcirclesize() >= 16) return;
    getinput();

#if 0
    for (i=connecthead; i>=0; i=connectpoint2[i])
        if (i != myconnectindex)
        {
            k = (movefifoend[myconnectindex]-1)-movefifoend[i];
            myminlag[i] = min(myminlag[i],k);
            mymaxlag = max(mymaxlag,k);
        }

    if (((movefifoend[myconnectindex]-1)&(TIMERUPDATESIZ-1)) == 0)
    {
        i = mymaxlag-bufferjitter; mymaxlag = 0;
        if (i > 0) bufferjitter += ((2+i)>>2);
        else if (i < 0) bufferjitter -= ((2-i)>>2);
    }
#endif

    if (networkmode == 1)
    {
        packbuf[2] = 0; j = 3;
        if (loc.fvel != oloc.fvel) packbuf[j++] = loc.fvel, packbuf[2] |= 1;
        if (loc.svel != oloc.svel) packbuf[j++] = loc.svel, packbuf[2] |= 2;
        if (loc.avel != oloc.avel) packbuf[j++] = loc.avel, packbuf[2] |= 4;
        if ((loc.bits^oloc.bits)&0x00ff) packbuf[j++] = (loc.bits&255), packbuf[2] |= 8;
        if ((loc.bits^oloc.bits)&0xff00) packbuf[j++] = ((loc.bits>>8)&255), packbuf[2] |= 16;
        copybufbyte(&loc,&oloc,sizeof(input));

        copybufbyte(&loc,&baksync[movefifoend[myconnectindex]][myconnectindex],sizeof(input));
        movefifoend[myconnectindex] = ((movefifoend[myconnectindex]+1)&(MOVEFIFOSIZ-1));

        for (i=connecthead; i>=0; i=connectpoint2[i])
            if (i != myconnectindex)
            {
                packbuf[0] = 17;
                packbuf[1] = (char)((movefifoend[myconnectindex]-movefifoend[i])&(MOVEFIFOSIZ-1));

                k = j;
                if ((myconnectindex == connecthead) || ((i == connecthead) && (myconnectindex == connectpoint2[connecthead])))
                {
                    while (syncvalhead != syncvaltail)
                    {
                        packbuf[j++] = syncval[syncvaltail];
                        syncvaltail = ((syncvaltail+1)&(MOVEFIFOSIZ-1));
                    }
                }
                sendpacket(i,packbuf,j);
                j = k;
            }

        gotlastpacketclock = totalclock;
        return;
    }

    //MASTER (or 1 player game)
    if ((myconnectindex == connecthead) || (option[4] == 0))
    {
        copybufbyte(&loc,&ffsync[myconnectindex],sizeof(input));

        if (option[4] != 0)
        {
            packbuf[0] = 0;
            j = ((numplayers+1)>>1)+1;
            for (k=1; k<j; k++) packbuf[k] = 0;
            k = (1<<3);
            for (i=connecthead; i>=0; i=connectpoint2[i])
            {
                l = 0;
                if (ffsync[i].fvel != osync[i].fvel) packbuf[j++] = ffsync[i].fvel, l |= 1;
                if (ffsync[i].svel != osync[i].svel) packbuf[j++] = ffsync[i].svel, l |= 2;
                if (ffsync[i].avel != osync[i].avel) packbuf[j++] = ffsync[i].avel, l |= 4;
                if (ffsync[i].bits != osync[i].bits)
                {
                    packbuf[j++] = (ffsync[i].bits&255);
                    packbuf[j++] = ((ffsync[i].bits>>8)&255);
                    l |= 8;
                }
                packbuf[k>>3] |= (l<<(k&7));
                k += 4;

                copybufbyte(&ffsync[i],&osync[i],sizeof(input));
            }

            while (syncvalhead != syncvaltail)
            {
                packbuf[j++] = syncval[syncvaltail];
                syncvaltail = ((syncvaltail+1)&(MOVEFIFOSIZ-1));
            }

            for (i=connectpoint2[connecthead]; i>=0; i=connectpoint2[i])
                sendpacket(i,packbuf,j);
        }
        else if (numplayers >= 2)
        {
            if (keystatus[0xb5])
            {
                keystatus[0xb5] = 0;
                locselectedgun2++; if (locselectedgun2 >= 3) locselectedgun2 = 0;
            }

            //Second player on 1 computer mode
            loc2.fvel = min(max(fvel2,-128+8),127-8);
            loc2.svel = min(max(svel2,-128+8),127-8);
            loc2.avel = min(max(avel2,-128+16),127-16);
            loc2.bits = (locselectedgun2<<13);
            loc2.bits |= keystatus[0x45];                  //Stand high
            loc2.bits |= (keystatus[0x47]<<1);             //Stand low
            loc2.bits |= (1<<8);                           //Run
            loc2.bits |= (keystatus[0x49]<<2);             //Look up
            loc2.bits |= (keystatus[0x37]<<3);             //Look down
            loc2.bits |= (keystatus[0x50]<<10);            //Space
            loc2.bits |= (keystatus[0x52]<<11);            //Shoot

            other = connectpoint2[myconnectindex];
            if (other < 0) other = connecthead;

            copybufbyte(&loc2,&ffsync[other],sizeof(input));
        }
        movethings();  //Move EVERYTHING (you too!)
    }
    else                        //I am a SLAVE
    {
        packbuf[0] = 1; packbuf[1] = 0; j = 2;
        if (loc.fvel != oloc.fvel) packbuf[j++] = loc.fvel, packbuf[1] |= 1;
        if (loc.svel != oloc.svel) packbuf[j++] = loc.svel, packbuf[1] |= 2;
        if (loc.avel != oloc.avel) packbuf[j++] = loc.avel, packbuf[1] |= 4;
        if ((loc.bits^oloc.bits)&0x00ff) packbuf[j++] = (loc.bits&255), packbuf[1] |= 8;
        if ((loc.bits^oloc.bits)&0xff00) packbuf[j++] = ((loc.bits>>8)&255), packbuf[1] |= 16;
        copybufbyte(&loc,&oloc,sizeof(input));
        sendpacket(connecthead,packbuf,j);
    }
}

void getpackets(void)
{
    int i, j, k, l;
    int other, packbufleng, movecnt;

    if (option[4] == 0) return;

    movecnt = 0;
    while ((packbufleng = getpacket(&other,packbuf)) > 0)
    {
        switch (packbuf[0])
        {
        case 0:  //[0] (receive master sync buffer)
            j = ((numplayers+1)>>1)+1; k = (1<<3);
            for (i=connecthead; i>=0; i=connectpoint2[i])
            {
                l = (packbuf[k>>3]>>(k&7));
                if (l&1) ffsync[i].fvel = packbuf[j++];
                if (l&2) ffsync[i].svel = packbuf[j++];
                if (l&4) ffsync[i].avel = packbuf[j++];
                if (l&8)
                {
                    ffsync[i].bits = ((short)packbuf[j])+(((short)packbuf[j+1])<<8);
                    j += 2;
                }
                k += 4;
            }

            while (j != packbufleng)
            {
                othersyncval[othersyncvalhead] = packbuf[j++];
                othersyncvalhead = ((othersyncvalhead+1)&(MOVEFIFOSIZ-1));
            }
            if ((syncvalhead != syncvaltottail) && (othersyncvalhead != syncvaltottail))
            {
                syncstat = 0;
                do
                {
                    syncstat |= (syncval[syncvaltottail]^othersyncval[syncvaltottail]);
                    syncvaltottail = ((syncvaltottail+1)&(MOVEFIFOSIZ-1));
                }
                while ((syncvalhead != syncvaltottail) && (othersyncvalhead != syncvaltottail));
            }

            movethings();        //Move all players and sprites
            movecnt++;
            break;
        case 1:  //[1] (receive slave sync buffer)
            j = 2; k = packbuf[1];
            if (k&1) ffsync[other].fvel = packbuf[j++];
            if (k&2) ffsync[other].svel = packbuf[j++];
            if (k&4) ffsync[other].avel = packbuf[j++];
            if (k&8) ffsync[other].bits = ((ffsync[other].bits&0xff00)|((short)packbuf[j++]));
            if (k&16) ffsync[other].bits = ((ffsync[other].bits&0x00ff)|(((short)packbuf[j++])<<8));
            break;
        case 2:
            getmessageleng = packbufleng-1;
            for (j=getmessageleng-1; j>=0; j--) getmessage[j] = packbuf[j+1];
            getmessagetimeoff = totalclock+360+(getmessageleng<<4);
            wsay("getstuff.wav",8192L,63L,63L); //Added 12/2004
            break;
        case 3:
            wsay("getstuff.wav",4096L,63L,63L);
            break;
#if 0
        case 5:
            playerreadyflag[other] = packbuf[1];
            if ((other == connecthead) && (packbuf[1] == 2))
                sendpacket(connecthead,packbuf,2);
            break;
#endif
        case 250:
            playerreadyflag[other]++;
            break;
        case 17:
            j = 3; k = packbuf[2];
            if (k&1) ffsync[other].fvel = packbuf[j++];
            if (k&2) ffsync[other].svel = packbuf[j++];
            if (k&4) ffsync[other].avel = packbuf[j++];
            if (k&8) ffsync[other].bits = ((ffsync[other].bits&0xff00)|((short)packbuf[j++]));
            if (k&16) ffsync[other].bits = ((ffsync[other].bits&0x00ff)|(((short)packbuf[j++])<<8));
            otherlag[other] = packbuf[1];

            copybufbyte(&ffsync[other],&baksync[movefifoend[other]][other],sizeof(input));
            movefifoend[other] = ((movefifoend[other]+1)&(MOVEFIFOSIZ-1));

            while (j != packbufleng)
            {
                othersyncval[othersyncvalhead] = packbuf[j++];
                othersyncvalhead = ((othersyncvalhead+1)&(MOVEFIFOSIZ-1));
            }
            if ((syncvalhead != syncvaltottail) && (othersyncvalhead != syncvaltottail))
            {
                syncstat = 0;
                do
                {
                    syncstat |= (syncval[syncvaltottail]^othersyncval[syncvaltottail]);
                    syncvaltottail = ((syncvaltottail+1)&(MOVEFIFOSIZ-1));
                }
                while ((syncvalhead != syncvaltottail) && (othersyncvalhead != syncvaltottail));
            }

            break;
        case 255:  //[255] (logout)
            keystatus[1] = 1;
            break;
        }
    }
    if ((networkmode == 0) && (myconnectindex != connecthead) && ((movecnt&1) == 0))
    {
        if (rand()&1) ototalclock += (TICSPERFRAME>>1);
        else ototalclock -= (TICSPERFRAME>>1);
    }
}

void drawoverheadmap(int cposx, int cposy, int czoom, short cang)
{
    int i, j, k, l=0, x1, y1, x2=0, y2=0, x3, y3, x4, y4, ox, oy, xoff, yoff;
    int dax, day, cosang, sinang, xspan, yspan, sprx, spry;
    int xrepeat, yrepeat, z1, z2, startwall, endwall, tilenum, daang;
    int xvect, yvect, xvect2, yvect2;
    char col;
    walltype *wal, *wal2;
    spritetype *spr;

    xvect = sintable[(-cang)&2047] * czoom;
    yvect = sintable[(1536-cang)&2047] * czoom;
    xvect2 = mulscale16(xvect,yxaspect);
    yvect2 = mulscale16(yvect,yxaspect);

    //Draw red lines
    for (i=0; i<numsectors; i++)
    {
        startwall = sector[i].wallptr;
        endwall = sector[i].wallptr + sector[i].wallnum;

        z1 = sector[i].ceilingz; z2 = sector[i].floorz;

        for (j=startwall,wal=&wall[startwall]; j<endwall; j++,wal++)
        {
            k = wal->nextwall; if (k < 0) continue;

            if ((show2dwall[j>>3]&(1<<(j&7))) == 0) continue;
            if ((k > j) && ((show2dwall[k>>3]&(1<<(k&7))) > 0)) continue;

            if (sector[wal->nextsector].ceilingz == z1)
                if (sector[wal->nextsector].floorz == z2)
                    if (((wal->cstat|wall[wal->nextwall].cstat)&(16+32)) == 0) continue;

            col = 152;

            if (dimensionmode[screenpeek] == 2)
            {
                if (sector[i].floorz != sector[i].ceilingz)
                    if (sector[wal->nextsector].floorz != sector[wal->nextsector].ceilingz)
                        if (((wal->cstat|wall[wal->nextwall].cstat)&(16+32)) == 0)
                            if (sector[i].floorz == sector[wal->nextsector].floorz) continue;
                if (sector[i].floorpicnum != sector[wal->nextsector].floorpicnum) continue;
                if (sector[i].floorshade != sector[wal->nextsector].floorshade) continue;
                col = 12;
            }

            ox = wal->x-cposx; oy = wal->y-cposy;
            x1 = dmulscale16(ox,xvect,-oy,yvect)+(xdim<<11);
            y1 = dmulscale16(oy,xvect2,ox,yvect2)+(ydim<<11);

            wal2 = &wall[wal->point2];
            ox = wal2->x-cposx; oy = wal2->y-cposy;
            x2 = dmulscale16(ox,xvect,-oy,yvect)+(xdim<<11);
            y2 = dmulscale16(oy,xvect2,ox,yvect2)+(ydim<<11);

            drawline256(x1,y1,x2,y2,col);
        }
    }

    //Draw sprites
    k = playersprite[screenpeek];
    for (i=0; i<numsectors; i++)
        for (j=headspritesect[i]; j>=0; j=nextspritesect[j])
            if ((show2dsprite[j>>3]&(1<<(j&7))) > 0)
            {
                spr = &sprite[j]; if (spr->cstat&0x8000) continue;
                col = 56;
                if (spr->cstat&1) col = 248;
                if (j == k) col = 31;

                k = statrate[spr->statnum];
                sprx = spr->x;
                spry = spr->y;
                if (k >= 0)
                {
                    switch (k)
                    {
                    case 0: l = smoothratio; break;
                    case 1: l = (smoothratio>>1)+(((nummoves-j)&1)<<15); break;
                    case 3: l = (smoothratio>>2)+(((nummoves-j)&3)<<14); break;
                    case 7: l = (smoothratio>>3)+(((nummoves-j)&7)<<13); break;
                    case 15: l = (smoothratio>>4)+(((nummoves-j)&15)<<12); break;
                    }
                    sprx = osprite[j].x+mulscale16(sprx-osprite[j].x,l);
                    spry = osprite[j].y+mulscale16(spry-osprite[j].y,l);
                }

                switch (spr->cstat&48)
                {
                case 0:
                    ox = sprx-cposx; oy = spry-cposy;
                    x1 = dmulscale16(ox,xvect,-oy,yvect);
                    y1 = dmulscale16(oy,xvect2,ox,yvect2);

                    if (dimensionmode[screenpeek] == 1)
                    {
                        ox = (sintable[(spr->ang+512)&2047]>>7);
                        oy = (sintable[(spr->ang)&2047]>>7);
                        x2 = dmulscale16(ox,xvect,-oy,yvect);
                        y2 = dmulscale16(oy,xvect,ox,yvect);

                        if (j == playersprite[screenpeek])
                        {
                            x2 = 0L;
                            y2 = -(czoom<<5);
                        }

                        x3 = mulscale16(x2,yxaspect);
                        y3 = mulscale16(y2,yxaspect);

                        drawline256(x1-x2+(xdim<<11),y1-y3+(ydim<<11),
                                    x1+x2+(xdim<<11),y1+y3+(ydim<<11),col);
                        drawline256(x1-y2+(xdim<<11),y1+x3+(ydim<<11),
                                    x1+x2+(xdim<<11),y1+y3+(ydim<<11),col);
                        drawline256(x1+y2+(xdim<<11),y1-x3+(ydim<<11),
                                    x1+x2+(xdim<<11),y1+y3+(ydim<<11),col);
                    }
                    else
                    {
                        if (((gotsector[i>>3]&(1<<(i&7))) > 0) && (czoom > 96))
                        {
                            daang = (spr->ang-cang)&2047;
                            if (j == playersprite[screenpeek]) { x1 = 0; y1 = 0; daang = 0; }
                            rotatesprite((x1<<4)+(xdim<<15),(y1<<4)+(ydim<<15),mulscale16(czoom*spr->yrepeat,yxaspect),daang,spr->picnum,spr->shade,spr->pal,(spr->cstat&2)>>1,windowxy1.x,windowxy1.y,windowxy2.x,windowxy2.y);
                        }
                    }
                    break;
                case 16:
                    x1 = sprx; y1 = spry;
                    tilenum = spr->picnum;
                    xoff = (int)picanm[tilenum].xofs+((int)spr->xoffset);
                    if ((spr->cstat&4) > 0) xoff = -xoff;
                    k = spr->ang; l = spr->xrepeat;
                    dax = sintable[k&2047]*l; day = sintable[(k+1536)&2047]*l;
                    l = tilesiz[tilenum].x; k = (l>>1)+xoff;
                    x1 -= mulscale16(dax,k); x2 = x1+mulscale16(dax,l);
                    y1 -= mulscale16(day,k); y2 = y1+mulscale16(day,l);

                    ox = x1-cposx; oy = y1-cposy;
                    x1 = dmulscale16(ox,xvect,-oy,yvect);
                    y1 = dmulscale16(oy,xvect2,ox,yvect2);

                    ox = x2-cposx; oy = y2-cposy;
                    x2 = dmulscale16(ox,xvect,-oy,yvect);
                    y2 = dmulscale16(oy,xvect2,ox,yvect2);

                    drawline256(x1+(xdim<<11),y1+(ydim<<11),
                                x2+(xdim<<11),y2+(ydim<<11),col);

                    break;
                case 32:
                    if (dimensionmode[screenpeek] == 1)
                    {
                        tilenum = spr->picnum;
                        xoff = (int)picanm[tilenum].xofs+((int)spr->xoffset);
                        yoff = (int)picanm[tilenum].yofs+((int)spr->yoffset);
                        if ((spr->cstat&4) > 0) xoff = -xoff;
                        if ((spr->cstat&8) > 0) yoff = -yoff;

                        k = spr->ang;
                        cosang = sintable[(k+512)&2047]; sinang = sintable[k];
                        xspan = tilesiz[tilenum].x; xrepeat = spr->xrepeat;
                        yspan = tilesiz[tilenum].y; yrepeat = spr->yrepeat;

                        dax = ((xspan>>1)+xoff)*xrepeat; day = ((yspan>>1)+yoff)*yrepeat;
                        x1 = sprx + dmulscale16(sinang,dax,cosang,day);
                        y1 = spry + dmulscale16(sinang,day,-cosang,dax);
                        l = xspan*xrepeat;
                        x2 = x1 - mulscale16(sinang,l);
                        y2 = y1 + mulscale16(cosang,l);
                        l = yspan*yrepeat;
                        k = -mulscale16(cosang,l); x3 = x2+k; x4 = x1+k;
                        k = -mulscale16(sinang,l); y3 = y2+k; y4 = y1+k;

                        ox = x1-cposx; oy = y1-cposy;
                        x1 = dmulscale16(ox,xvect,-oy,yvect);
                        y1 = dmulscale16(oy,xvect2,ox,yvect2);

                        ox = x2-cposx; oy = y2-cposy;
                        x2 = dmulscale16(ox,xvect,-oy,yvect);
                        y2 = dmulscale16(oy,xvect2,ox,yvect2);

                        ox = x3-cposx; oy = y3-cposy;
                        x3 = dmulscale16(ox,xvect,-oy,yvect);
                        y3 = dmulscale16(oy,xvect2,ox,yvect2);

                        ox = x4-cposx; oy = y4-cposy;
                        x4 = dmulscale16(ox,xvect,-oy,yvect);
                        y4 = dmulscale16(oy,xvect2,ox,yvect2);

                        drawline256(x1+(xdim<<11),y1+(ydim<<11),
                                    x2+(xdim<<11),y2+(ydim<<11),col);

                        drawline256(x2+(xdim<<11),y2+(ydim<<11),
                                    x3+(xdim<<11),y3+(ydim<<11),col);

                        drawline256(x3+(xdim<<11),y3+(ydim<<11),
                                    x4+(xdim<<11),y4+(ydim<<11),col);

                        drawline256(x4+(xdim<<11),y4+(ydim<<11),
                                    x1+(xdim<<11),y1+(ydim<<11),col);

                    }
                    break;
                }
            }

    //Draw white lines
    for (i=0; i<numsectors; i++)
    {
        startwall = sector[i].wallptr;
        endwall = sector[i].wallptr + sector[i].wallnum;

        k = -1;
        for (j=startwall,wal=&wall[startwall]; j<endwall; j++,wal++)
        {
            if (wal->nextwall >= 0) continue;

            if ((show2dwall[j>>3]&(1<<(j&7))) == 0) continue;

            if (tilesiz[wal->picnum].x == 0) continue;
            if (tilesiz[wal->picnum].y == 0) continue;

            if (j == k)
            { x1 = x2; y1 = y2; }
            else
            {
                ox = wal->x-cposx; oy = wal->y-cposy;
                x1 = dmulscale16(ox,xvect,-oy,yvect)+(xdim<<11);
                y1 = dmulscale16(oy,xvect2,ox,yvect2)+(ydim<<11);
            }

            k = wal->point2; wal2 = &wall[k];
            ox = wal2->x-cposx; oy = wal2->y-cposy;
            x2 = dmulscale16(ox,xvect,-oy,yvect)+(xdim<<11);
            y2 = dmulscale16(oy,xvect2,ox,yvect2)+(ydim<<11);

            drawline256(x1,y1,x2,y2,24);
        }
    }
}

//New movesprite using getzrange.  Note that I made the getzrange
//parameters global (&globhiz,&globhihit,&globloz,&globlohit) so they
//don't need to be passed everywhere.  Also this should make this
//movesprite function compatible with the older movesprite functions.
int movesprite(short spritenum, int dx, int dy, int dz, int ceildist, int flordist, int clipmask)
{
    int daz, zoffs /*, tempint*/;
    short retval, dasectnum, datempshort;
    spritetype *spr;

    spr = &sprite[spritenum];

    if ((spr->cstat&128) == 0)
        zoffs = -((tilesiz[spr->picnum].y*spr->yrepeat)<<1);
    else
        zoffs = 0;

    dasectnum = spr->sectnum;  //Can't modify sprite sectors directly becuase of linked lists
    daz = spr->z+zoffs;  //Must do this if not using the new centered centering (of course)
    retval = clipmove_old(&spr->x,&spr->y,&daz,&dasectnum,dx,dy,
                          ((int)spr->clipdist)<<2,ceildist,flordist,clipmask);

    if (dasectnum < 0) retval = -1;

    if ((dasectnum != spr->sectnum) && (dasectnum >= 0))
        changespritesect(spritenum,dasectnum);

    //Set the blocking bit to 0 temporarly so getzrange doesn't pick up
    //its own sprite
    datempshort = spr->cstat; spr->cstat &= ~1;
    getzrange_old(spr->x,spr->y,spr->z-1,spr->sectnum,
                  &globhiz,&globhihit,&globloz,&globlohit,
                  ((int)spr->clipdist)<<2,clipmask);
    spr->cstat = datempshort;

    daz = spr->z+zoffs + dz;
    if ((daz <= globhiz) || (daz > globloz))
    {
        if (retval != 0) return retval;
        return 16384+dasectnum;
    }
    spr->z = daz-zoffs;
    return retval;
}


void waitforeverybody()
{
    int i;
    if (numplayers < 2) return;
    packbuf[0] = 250;
    for (i=connecthead; i>=0; i=connectpoint2[i])
    {
        if (i != myconnectindex) sendpacket(i,packbuf,1);
        if ((!networkmode) && (myconnectindex != connecthead)) break; //slaves in M/S mode only send to master
    }
    playerreadyflag[myconnectindex]++;
    while (1)
    {
        handleevents();
        refreshaudio();

        drawrooms(pos[myconnectindex].x,pos[myconnectindex].y,pos[myconnectindex].z,ang[myconnectindex],horiz[myconnectindex],cursectnum[myconnectindex]);
        if (!networkmode) Bsprintf((char *)tempbuf,"Master/slave mode");
        else Bsprintf((char *)tempbuf,"Peer-peer mode");
        printext256((xdim>>1)-(strlen((char *)tempbuf)<<2),(ydim>>1)-24,31,0,(char *)tempbuf,0);
        Bsprintf((char *)tempbuf,"Waiting for players");
        printext256((xdim>>1)-(strlen((char *)tempbuf)<<2),(ydim>>1)-16,31,0,(char *)tempbuf,0);
        for (i=connecthead; i>=0; i=connectpoint2[i])
        {
            if (playerreadyflag[i] < playerreadyflag[myconnectindex])
            {
                //slaves in M/S mode only wait for master
                if ((!networkmode) && (myconnectindex != connecthead) && (i != connecthead))
                {
                    Bsprintf((char *)tempbuf,"Player %d",i);
                    printext256((xdim>>1)-(16<<2),(ydim>>1)+i*8,15,0,(char *)tempbuf,0);
                }
                else
                {
                    Bsprintf((char *)tempbuf,"Player %d NOT ready",i);
                    printext256((xdim>>1)-(16<<2),(ydim>>1)+i*8,127,0,(char *)tempbuf,0);
                }
            }
            else
            {
                Bsprintf((char *)tempbuf,"Player %d ready",i);
                printext256((xdim>>1)-(16<<2),(ydim>>1)+i*8,31,0,(char *)tempbuf,0);
            }
            if (i == myconnectindex)
            {
                Bsprintf((char *)tempbuf,"You->");
                printext256((xdim>>1)-(26<<2),(ydim>>1)+i*8,95,0,(char *)tempbuf,0);
            }
        }
        nextpage();


        if (quitevent || keystatus[1])
        {
            sendlogoff();         //Signing off
            musicoff();
            uninitmultiplayers();
            uninittimer();
            uninitinput();
            uninitengine();
            uninitsb();
            uninitgroupfile();
            exit(0);
        }

        getpackets();

        for (i=connecthead; i>=0; i=connectpoint2[i])
        {
            if (playerreadyflag[i] < playerreadyflag[myconnectindex]) break;
            if ((!networkmode) && (myconnectindex != connecthead)) { i = -1; break; } //slaves in M/S mode only wait for master
        }
        if (i < 0) return;
    }
}


void searchmap(short startsector)
{
    int i, j, dasect, splc, send, startwall, endwall;
    short dapic;
    walltype *wal;

    if ((startsector < 0) || (startsector >= numsectors)) return;
    for (i=0; i<(MAXSECTORS>>3); i++) show2dsector[i] = 0;
    for (i=0; i<(MAXWALLS>>3); i++) show2dwall[i] = 0;
    for (i=0; i<(MAXSPRITES>>3); i++) show2dsprite[i] = 0;

    //Search your area recursively & set all show2dsector/show2dwalls
    tempshort[0] = startsector;
    show2dsector[startsector>>3] |= (1<<(startsector&7));
    dapic = sector[startsector].ceilingpicnum;
    if (waloff[dapic] == 0) loadtile(dapic);
    dapic = sector[startsector].floorpicnum;
    if (waloff[dapic] == 0) loadtile(dapic);
    for (splc=0,send=1; splc<send; splc++)
    {
        dasect = tempshort[splc];
        startwall = sector[dasect].wallptr;
        endwall = startwall + sector[dasect].wallnum;
        for (i=startwall,wal=&wall[startwall]; i<endwall; i++,wal++)
        {
            show2dwall[i>>3] |= (1<<(i&7));
            dapic = wall[i].picnum;
            if (waloff[dapic] == 0) loadtile(dapic);
            dapic = wall[i].overpicnum;
            if (((dapic&0xfffff000) == 0) && (waloff[dapic] == 0)) loadtile(dapic);

            j = wal->nextsector;
            if ((j >= 0) && ((show2dsector[j>>3]&(1<<(j&7))) == 0))
            {
                show2dsector[j>>3] |= (1<<(j&7));

                dapic = sector[j].ceilingpicnum;
                if (waloff[dapic] == 0) loadtile(dapic);
                dapic = sector[j].floorpicnum;
                if (waloff[dapic] == 0) loadtile(dapic);

                tempshort[send++] = (short)j;
            }
        }

        for (i=headspritesect[dasect]; i>=0; i=nextspritesect[i])
        {
            show2dsprite[i>>3] |= (1<<(i&7));
            dapic = sprite[i].picnum;
            if (waloff[dapic] == 0) loadtile(dapic);
        }
    }
}

void setinterpolation(int *posptr)
{
    int i;

    if (numinterpolations >= MAXINTERPOLATIONS) return;
    for (i=numinterpolations-1; i>=0; i--)
        if (curipos[i] == posptr) return;
    curipos[numinterpolations] = posptr;
    oldipos[numinterpolations] = *posptr;
    numinterpolations++;
}

void stopinterpolation(int *posptr)
{
    int i;

    for (i=numinterpolations-1; i>=startofdynamicinterpolations; i--)
        if (curipos[i] == posptr)
        {
            numinterpolations--;
            oldipos[i] = oldipos[numinterpolations];
            bakipos[i] = bakipos[numinterpolations];
            curipos[i] = curipos[numinterpolations];
        }
}

void updateinterpolations(void)  //Stick at beginning of domovethings
{
    int i;

    for (i=numinterpolations-1; i>=0; i--) oldipos[i] = *curipos[i];
}

void dointerpolations(void)       //Stick at beginning of drawscreen
{
    int i, j, odelta, ndelta;

    ndelta = 0; j = 0;
    for (i=numinterpolations-1; i>=0; i--)
    {
        bakipos[i] = *curipos[i];
        odelta = ndelta; ndelta = (*curipos[i])-oldipos[i];
        if (odelta != ndelta) j = mulscale16(ndelta,smoothratio);
        *curipos[i] = oldipos[i]+j;
    }
}

void restoreinterpolations(void)  //Stick at end of drawscreen
{
    int i;

    for (i=numinterpolations-1; i>=0; i--) *curipos[i] = bakipos[i];
}

void printext(int x, int y, char *buffer, short tilenum /*, char invisiblecol*/)
{
    int i;
    char ch;

    for (i=0; buffer[i]!=0; i++)
    {
        ch = (char)buffer[i];
        rotatesprite((x-((8&15)<<3))<<16,(y-((8>>4)<<3))<<16,65536L,0,tilenum,0,0,8+16+64+128,x,y,x+7,y+7);
        rotatesprite((x-((ch&15)<<3))<<16,(y-((ch>>4)<<3))<<16,65536L,0,tilenum,0,0,8+16+128,x,y,x+7,y+7);
        x += 8;
    }
}

void drawtilebackground(/*int thex, int they,*/ short tilenum,
                                                signed char shade, int cx1, int cy1,
                                                int cx2, int cy2, char dapalnum)
{
    int x, y, xsiz, ysiz, tx1, ty1, tx2, ty2;

    xsiz = tilesiz[tilenum].x; tx1 = cx1/xsiz; tx2 = cx2/xsiz;
    ysiz = tilesiz[tilenum].y; ty1 = cy1/ysiz; ty2 = cy2/ysiz;

    for (x=tx1; x<=tx2; x++)
        for (y=ty1; y<=ty2; y++)
            rotatesprite(x*xsiz<<16,y*ysiz<<16,65536L,0,tilenum,shade,dapalnum,8+16+64+128,cx1,cy1,cx2,cy2);
}

void M32RunScript(const char *s) { UNREFERENCED_PARAMETER(s); }
void G_Polymer_UnInit(void) { }
void app_crashhandler(void) { }

/*
 * vim:ts=4:sw=4:
 */
