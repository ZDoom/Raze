// "Build Engine & Tools" Copyright (c) 1993-1997 Ken Silverman
// Ken Silverman's official web site: "http://www.advsys.net/ken"
// See the included license file "BUILDLIC.TXT" for license info.
//
// This file has been modified from Ken Silverman's original release
// by Jonathon Fowler (jonof@edgenetwk.com)

#include "build.h"
#include "compat.h"
#include "pragmas.h"
#include "osd.h"
#include "cache1d.h"
#include "editor.h"

#include "baselayer.h"
#ifdef RENDERTYPEWIN
#include "winlayer.h"
#endif

#include "m32script.h"

#define TIMERINTSPERSECOND 120

#define updatecrc16(crc,dat) (crc = (((crc<<8)&65535)^crctable[((((uint16_t)crc)>>8)&65535)^dat]))
static int32_t crctable[256];
static char kensig[64];

extern const char *ExtGetVer(void);
extern int32_t ExtInit(void);
extern int32_t ExtPreInit(int32_t argc,const char **argv);
extern void ExtUnInit(void);
extern void ExtPreCheckKeys(void);
extern void ExtAnalyzeSprites(void);
extern void ExtCheckKeys(void);
extern void ExtLoadMap(const char *mapname);
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

extern char spritecol2d[MAXTILES][2];

char noclip=0;

int32_t vel, svel, angvel;

// 0   1     2     3      4       5      6      7
// up, down, left, right, lshift, rctrl, lctrl, space
// 8  9  10    11    12   13
// a, z, pgdn, pgup, [,], [.]
// 14       15     16 17 18   19
// kpenter, enter, =, -, tab, `
uint8_t buildkeys[NUMBUILDKEYS] =
{
    0xc8,0xd0,0xcb,0xcd,0x2a,0x9d,0x1d,0x39,
    0x1e,0x2c,0xd1,0xc9,0x33,0x34,
    0x9c,0x1c,0xd,0xc,0xf,0x29
};

vec3_t pos;
int32_t horiz = 100;
static int32_t mousexsurp = 0, mouseysurp = 0;
int16_t ang, cursectnum;
int32_t hvel;

int32_t grponlymode = 0;
extern int32_t editorgridextent;	// in engine.c
extern double msens;
int32_t graphicsmode = 0;
extern int32_t xyaspect;
extern int32_t totalclocklock;

int32_t synctics = 0, lockclock = 0;

// those ones save the respective 3d video vars while in 2d mode
// so that exiting from mapster32 in 2d mode saves the correct ones
double vid_gamma_3d=-1, vid_contrast_3d=-1, vid_brightness_3d=-1;

extern char vgacompatible;

extern char picsiz[MAXTILES];
extern int32_t startposx, startposy, startposz;
extern int16_t startang, startsectnum;
extern int32_t ydim16, halfxdim16, midydim16;
extern intptr_t frameplace;
int32_t xdim2d = 640, ydim2d = 480, xdimgame = 640, ydimgame = 480, bppgame = 8;
int32_t forcesetup = 1;

extern int32_t cachesize, artsize;

static int16_t oldmousebstatus = 0;
char game_executable[BMAX_PATH] = DEFAULT_GAME_LOCAL_EXEC;
int32_t zlock = 0x7fffffff, zmode = 0, whitecol, kensplayerheight = 32;
//int16_t defaultspritecstat = 0;

int16_t localartfreq[MAXTILES];
int16_t localartlookup[MAXTILES], localartlookupnum;

char tempbuf[4096];

char names[MAXTILES][25];

int16_t asksave = 0;
extern int16_t editstatus, searchit;
extern int32_t searchx, searchy;                          //search input
extern int16_t searchsector, searchwall, searchstat;     //search output
int32_t osearchx, osearchy;                               //old search input

extern int16_t pointhighlight, linehighlight, highlightcnt;
int32_t grid = 3, autogrid = 0, gridlock = 1, showtags = 1;
int32_t zoom = 768, gettilezoom = 1;
int32_t lastpm16time = 0;

int32_t numsprites;
extern int32_t mapversion;

int16_t highlight[MAXWALLS+MAXSPRITES];
int16_t highlightsector[MAXSECTORS], highlightsectorcnt = -1;
extern char textfont[128][8];

char pskysearch[MAXSECTORS];

int32_t temppicnum, tempcstat, templotag, temphitag, tempextra;
uint32_t temppal, tempvis, tempxrepeat, tempyrepeat;
int32_t tempshade, tempxvel, tempyvel, tempzvel;
char somethingintab = 255;

int32_t mlook = 0,mskip=0;
int32_t revertCTRL=0,scrollamount=3;
int32_t unrealedlook=1, quickmapcycling=1; //PK

char program_origcwd[BMAX_PATH];
char *mapster32_fullpath;
char *testplay_addparam = 0;

static char boardfilename[BMAX_PATH], selectedboardfilename[BMAX_PATH];

static CACHE1D_FIND_REC *finddirs=NULL, *findfiles=NULL, *finddirshigh=NULL, *findfileshigh=NULL;
static int32_t numdirs=0, numfiles=0;
static int32_t currentlist=0;

//static int32_t repeatcountx, repeatcounty;

static int32_t fillist[640];

static int32_t mousx, mousy;
int16_t prefixtiles[16] = { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10 };
uint8_t hlsectorbitmap[MAXSECTORS>>3];  // show2dsector is already taken...

static uint8_t visited[MAXWALLS>>3];  // used for AlignWalls and trace_loop

typedef struct
{
    int16_t numsectors, numwalls, numsprites;
    sectortype *sector;
    walltype *wall;
    spritetype *sprite;
} mapinfofull_t;

static int32_t backup_highlighted_map(mapinfofull_t *mapinfo);
static int32_t restore_highlighted_map(mapinfofull_t *mapinfo);

/*
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
*/

#define eitherALT   (keystatus[0x38]|keystatus[0xb8])
#define eitherCTRL  (keystatus[0x1d]|keystatus[0x9d])
#define eitherSHIFT (keystatus[0x2a]|keystatus[0x36])

#define DOWN_BK(BuildKey) (keystatus[buildkeys[BK_##BuildKey]])

int32_t pk_turnaccel=16;
int32_t pk_turndecel=12;
int32_t pk_uedaccel=3;

int8_t sideview_reversehrot = 0;

char lastpm16buf[156];

//static int32_t checksectorpointer_warn = 0;

char changechar(char dachar, int32_t dadir, char smooshyalign, char boundcheck);
static int32_t adjustmark(int32_t *xplc, int32_t *yplc, int16_t danumwalls);
static void locktogrid(int32_t *dax, int32_t *day);
static int32_t checkautoinsert(int32_t dax, int32_t day, int16_t danumwalls);
void keytimerstuff(void);
static int32_t clockdir(int16_t wallstart);
static void flipwalls(int16_t numwalls, int16_t newnumwalls);
static void insertpoint(int16_t linehighlight, int32_t dax, int32_t day);
static void deletepoint(int16_t point);
static int32_t deletesector(int16_t sucksect);
void fixrepeats(int16_t i);
static int16_t loopinside(int32_t x, int32_t y, int16_t startwall);
int32_t fillsector(int16_t sectnum, char fillcolor);
static int16_t whitelinescan(int16_t dalinehighlight);
void printcoords16(int32_t posxe, int32_t posye, int16_t ange);
static void copysector(int16_t soursector, int16_t destsector, int16_t deststartwall, char copystat, const int16_t *oldtonewsect);
int32_t drawtilescreen(int32_t pictopleft, int32_t picbox);
void overheadeditor(void);
static int32_t getlinehighlight(int32_t xplc, int32_t yplc, int32_t line);
void fixspritesectors(void);
static int32_t movewalls(int32_t start, int32_t offs);
int32_t loadnames(void);
void updatenumsprites(void);
static void getclosestpointonwall(int32_t x, int32_t y, int32_t dawall, int32_t *nx, int32_t *ny);
static void initcrc(void);
void AutoAlignWalls(int32_t nWall0, int32_t ply);
int32_t gettile(int32_t tilenum);

static int32_t menuselect(void);
static int32_t menuselect_auto(int32_t); //PK

static int32_t insert_sprite_common(int32_t sucksect, int32_t dax, int32_t day);
static void correct_ornamented_sprite(int32_t i, int32_t hitw);

static int32_t getfilenames(const char *path, const char *kind);
static void clearfilenames(void);
void loadmhk(int32_t domessage);
extern int32_t map_revision;
extern int32_t map_undoredo(int32_t dir);
extern void map_undoredo_free(void);

void clearkeys(void) { Bmemset(keystatus,0,sizeof(keystatus)); }

static inline void bclamp(int32_t *x, int32_t mi, int32_t ma)
{
    if (*x>ma) *x=ma;
    if (*x<mi) *x=mi;
}

#ifdef USE_OPENGL
static int32_t osdcmd_restartvid(const osdfuncparm_t *parm)
{
    extern int32_t qsetmode;

    UNREFERENCED_PARAMETER(parm);

    if (qsetmode != 200) return OSDCMD_OK;

    resetvideomode();
    if (setgamemode(fullscreen,xdim,ydim,bpp))
        OSD_Printf("restartvid: Reset failed...\n");

    return OSDCMD_OK;
}

static int32_t osdcmd_vidmode(const osdfuncparm_t *parm)
{
    int32_t newx = xdim, newy = ydim, newbpp = bpp, newfullscreen = fullscreen, tmp;
    extern int32_t qsetmode;

    switch (parm->numparms)
    {
    case 1:	// bpp switch
        tmp = Batol(parm->parms[0]);
        if (tmp==8 || tmp==16 || tmp==32)
            newbpp = tmp;
        break;
    case 4:	// fs, res, bpp switch
        newfullscreen = (Batol(parm->parms[3]) != 0);
    case 3:	// res & bpp switch
        tmp = Batol(parm->parms[2]);
        if (tmp==8 || tmp==16 || tmp==32)
            newbpp = tmp;
    case 2: // res switch
        newx = Batol(parm->parms[0]);
        newy = Batol(parm->parms[1]);
        break;
    default:
        return OSDCMD_SHOWHELP;
    }

    if (qsetmode != 200)
    {
        qsetmodeany(newx,newy);
        xdim2d = xdim;
        ydim2d = ydim;

        begindrawing();	//{{{

        CLEARLINES2D(0, ydim16, 0);

        ydim16 = ydim;
//        drawline16(0,ydim-STATUS2DSIZ,xdim-1,ydim-STATUS2DSIZ,editorcolors[1]);
        /*        drawline16(0,ydim-1,xdim-1,ydim-1,1);
                drawline16(0,ydim-STATUS2DSIZ,0,ydim-1,1);
                drawline16(xdim-1,ydim-STATUS2DSIZ,xdim-1,ydim-1,1);
                drawline16(0,ydim-STATUS2DSIZ+24,xdim-1,ydim-STATUS2DSIZ+24,1);
                drawline16(192-24,ydim-STATUS2DSIZ,192-24,ydim-STATUS2DSIZ+24,1);
                drawline16(0,ydim-1-20,xdim-1,ydim-1-20,1);
                drawline16(256,ydim-1-20,256,ydim-1,1); */
        ydim16 = ydim-STATUS2DSIZ2;
        enddrawing();	//}}}
        return OSDCMD_OK;
    }

    if (setgamemode(newfullscreen,newx,newy,newbpp))
        OSD_Printf("vidmode: Mode change failed!\n");

    xdimgame = newx;
    ydimgame = newy;
    bppgame = newbpp;
    fullscreen = newfullscreen;

    return OSDCMD_OK;
}
#endif

extern int32_t startwin_run(void);

extern char *defsfilename;	// set in bstub.c


#ifdef M32_SHOWDEBUG
char m32_debugstr[64][128];
int32_t m32_numdebuglines=0;

static void M32_drawdebug(void)
{
    int i;
    int x=4, y=8;

    begindrawing();
#if 0
    {
        static char tstr[128];
        Bsprintf(tstr, "searchstat=%d, searchsector=%d, searchwall=%d (%d)",
                 searchstat, searchsector, searchwall, searchbottomwall);
        printext256(x,y,whitecol,0,tstr,xdimgame>640?0:1);
    }
#endif
    for (i=0; i<m32_numdebuglines; i++)
    {
        y+=8;
        printext256(x,y,whitecol,0,m32_debugstr[i],xdimgame>640?0:1);
    }
    enddrawing();
    m32_numdebuglines=0;
}
#endif

int32_t app_main(int32_t argc, const char **argv)
{
    char quitflag, cmdsetup = 0;
    int32_t i, j, k;

    pathsearchmode = 1;		// unrestrict findfrompath so that full access to the filesystem can be had

#ifdef USE_OPENGL
    OSD_RegisterFunction("restartvid","restartvid: reinitialize the video mode",osdcmd_restartvid);
    OSD_RegisterFunction("vidmode","vidmode <xdim> <ydim> <bpp> <fullscreen>: immediately change the video mode",osdcmd_vidmode);
#endif

    wm_setapptitle("Mapster32");

    editstatus = 1;
    newaspect_enable = 1;

    if ((i = ExtPreInit(argc,argv)) < 0) return -1;

#ifdef RENDERTYPEWIN
    backgroundidle = 1;
#endif

    boardfilename[0] = 0;
    for (i=1; i<argc; i++)
    {
        if (argv[i][0] == '-')
        {
            if (!Bstrcmp(argv[i], "-setup")) cmdsetup = 1;
            else if (!Bstrcmp(argv[i], "-help") || !Bstrcmp(argv[i], "--help") || !Bstrcmp(argv[i], "-?"))
            {
                char *s =
                    "Mapster32\n"
                    "Syntax: mapster32 [options] mapname\n"
                    "Options:\n"
                    "\t-grp\tUse an extra GRP or ZIP file.\n"
                    "\t-g\tSame as above.\n"
#if defined RENDERTYPEWIN || (defined RENDERTYPESDL && !defined __APPLE__ && defined HAVE_GTK2)
                    "\t-setup\tDisplays the configuration dialogue box before entering the editor.\n"
#endif
                    ;
#if defined RENDERTYPEWIN || (defined RENDERTYPESDL && !defined __APPLE__ && defined HAVE_GTK2)
                wm_msgbox("Mapster32","%s",s);
#else
                puts(s);
#endif
                return 0;
            }
            continue;
        }
        if (!boardfilename[0])
        {
            Bstrncpy(boardfilename, argv[i], BMAX_PATH);
            boardfilename[i-BMAX_PATH] = 0;
        }
    }
    if (boardfilename[0] == 0)
    {
        Bstrcpy(boardfilename,"newboard.map");
    }
    else if (Bstrchr(boardfilename,'.') == 0)
    {
        Bstrcat(boardfilename, ".map");
    }
    //Bcanonicalisefilename(boardfilename,0);

    if ((i = ExtInit()) < 0) return -1;
#if defined RENDERTYPEWIN || (defined RENDERTYPESDL && !defined __APPLE__ && defined HAVE_GTK2)
    if (i || forcesetup || cmdsetup)
    {
        if (quitevent || !startwin_run())
        {
            uninitengine();
            exit(0);
        }
    }
#endif

    loadnames();

    if (initinput()) return -1;
    // if (option[3] != 0) moustat =
    initmouse();

    inittimer(TIMERINTSPERSECOND);
    installusertimercallback(keytimerstuff);

    loadpics("tiles000.art", 1048576*16);

    Bstrcpy(kensig,"Uses BUILD technology by Ken Silverman");
    initcrc();

    if (!loaddefinitionsfile(defsfilename))
        initprintf("Definitions file loaded.\n");

    k = 0;
    for (i=0; i<256; i++)
    {
        j = ((int32_t)palette[i*3])+((int32_t)palette[i*3+1])+((int32_t)palette[i*3+2]);
        if (j > k) { k = j; whitecol = i; }
    }

    k = clipmapinfo_load("_clipshape0.map");
    if (k>0)
        initprintf("There was an error loading the sprite clipping map (status %d).\n", k);

    if (LoadBoard(boardfilename, 1))
    {
        initspritelists();
        pos.x = 32768;
        pos.y = 32768;
        pos.z = 0;
        ang = 1536;
        numsectors = 0;
        numwalls = 0;
        cursectnum = -1;
    }

    totalclock = 0;

    updatesector(pos.x,pos.y,&cursectnum);

    if (cursectnum == -1)
    {
        vid_gamma_3d = vid_gamma;
        vid_brightness_3d = vid_brightness;
        vid_contrast_3d = vid_contrast;

        vid_gamma = vid_contrast = 1.0;
        vid_brightness = 0.0;

        setbrightness(0,0,0);
        if (setgamemode(fullscreen, xdim2d, ydim2d, 8) < 0)
        {
            ExtUnInit();
            uninitengine();
            Bprintf("%d * %d not supported in this graphics mode\n",xdim2d,ydim2d);
            exit(0);
        }

        // executed once per init, but after setgamemode so that OSD has the right width
        OSD_Exec("m32_autoexec.cfg");

        overheadeditor();
        keystatus[buildkeys[BK_MODE2D_3D]] = 0;

        vid_gamma = vid_gamma_3d;
        vid_contrast = vid_contrast_3d;
        vid_brightness = vid_brightness_3d;

        vid_gamma_3d = vid_contrast_3d = vid_brightness_3d = -1;

        setbrightness(GAMMA_CALC,0,0);
    }
    else
    {
        if (setgamemode(fullscreen, xdimgame, ydimgame, bppgame) < 0)
        {
            ExtUnInit();
            uninitengine();
            Bprintf("%d * %d not supported in this graphics mode\n",xdim,ydim);
            exit(0);
        }

        // executed once per init, but after setgamemode so that OSD has the right width
        OSD_Exec("m32_autoexec.cfg");

        setbrightness(GAMMA_CALC,0,0);
    }

CANCEL:
    quitflag = 0;
    while (quitflag == 0)
    {
        if (handleevents())
        {
            if (quitevent)
            {
                keystatus[1] = 1;
                quitevent = 0;
            }
        }

        OSD_DispatchQueued();

        ExtPreCheckKeys();

        nextpage();
        synctics = totalclock-lockclock;
        lockclock += synctics;

        drawrooms(pos.x,pos.y,pos.z,ang,horiz,cursectnum);
        ExtAnalyzeSprites();
        drawmasks();

#ifdef POLYMER
        if (rendmode == 4 && searchit == 2)
        {
            polymer_editorpick();
            drawrooms(pos.x,pos.y,pos.z,ang,horiz,cursectnum);
            ExtAnalyzeSprites();
            drawmasks();
        }
#endif

#ifdef M32_SHOWDEBUG
        if (searchstat>=0 && (searchwall<0 || searchsector<0))
        {
            Bsprintf(m32_debugstr[m32_numdebuglines++], "inconsistent search variables!");
            searchstat = -1;
        }

        M32_drawdebug();
#endif
        ExtCheckKeys();


        if (keystatus[1])
        {
            keystatus[1] = 0;

            printext256(0,0,whitecol,0,"Are you sure you want to quit?",0);

            showframe(1);
            synctics = totalclock-lockclock;
            lockclock += synctics;

            while ((keystatus[1]|keystatus[0x1c]|keystatus[0x39]|keystatus[0x31]) == 0)
            {
                idle_waitevent();
                if (handleevents())
                {
                    if (quitevent)
                    {
                        quitflag = 1;
                        break;
                    }
                }

                if (keystatus[0x15]||keystatus[0x1c]) // Y or ENTER
                {
                    keystatus[0x15] = 0;
                    keystatus[0x1c] = 0;
                    quitflag = 1; break;
                }
            }
            while (keystatus[1])
            {
                keystatus[1] = 0;
                quitevent = 0;
                goto CANCEL;
            }
        }
    }

    if (asksave)
    {
        i = CheckMapCorruption(4, 0);

        printext256(0,8,whitecol,0,i<4?"Save changes?":"Map is heavily corrupt. Save changes?",0);
        showframe(1);

        while ((keystatus[1]|keystatus[0x1c]|keystatus[0x39]|keystatus[0x31]|keystatus[0x2e]) == 0)
        {
            idle_waitevent();
            if (handleevents()) { if (quitevent) break;	} // like saying no

            if (keystatus[0x15] || keystatus[0x1c]) // Y or ENTER
            {
                keystatus[0x15] = keystatus[0x1c] = 0;

                SaveBoard(NULL, 0);

                break;
            }
        }
        while (keystatus[1]||keystatus[0x2e])
        {
            keystatus[1] = keystatus[0x2e] = 0;
            quitevent = 0;
            goto CANCEL;
        }
    }


    clearfilenames();
    ExtUnInit();
    uninitengine();

    Bprintf("Memory status: %d(%d) bytes\n",cachesize,artsize);
    Bprintf("%s\n",kensig);
    return(0);
}

/*
void showmouse(void)
{
    int32_t i;

    for (i=1;i<=4;i++)
    {
        plotpixel(searchx+i,searchy,whitecol);
        plotpixel(searchx-i,searchy,whitecol);
        plotpixel(searchx,searchy-i,whitecol);
        plotpixel(searchx,searchy+i,whitecol);
    }
}
*/

static int32_t mhk=0;
void loadmhk(int32_t domessage)
{
    char *p; char levname[BMAX_PATH];

    if (!mhk)
        return;

    Bstrcpy(levname, boardfilename);
    p = Bstrrchr(levname,'.');
    if (!p)
        Bstrcat(levname,".mhk");
    else
    {
        p[1]='m';
        p[2]='h';
        p[3]='k';
        p[4]=0;
    }

    if (!loadmaphack(levname))
    {
        if (domessage)
            message("Loaded map hack file '%s'",levname);
        else
            initprintf("Loaded map hack file '%s'\n",levname);
    }
    else
    {
        mhk=2;
        if (domessage)
            message("No maphack found for map '%s'",boardfilename);
    }
}

void editinput(void)
{
//    char smooshyalign, repeatpanalign, buffer[80];
//    short sectnum, nextsectnum, startwall, endwall, dasector, daang;
    int32_t mousz, bstatus;
    int32_t i, j, k, /*cnt,*/ tempint=0, doubvel/*, changedir, wallfind[2], daz[2]*/;
    int32_t /*dashade[2],*/ goalz, xvect, yvect,/*PK*/ zvect, hiz, loz;
    int32_t dax, day, hihit, lohit;

// 3B  3C  3D  3E   3F  40  41  42   43  44  57  58          46
// F1  F2  F3  F4   F5  F6  F7  F8   F9 F10 F11 F12        SCROLL

    /*    if (keystatus[0x57] > 0)  //F11 - brightness
        {
            keystatus[0x57] = 0;
            brightness++;
            if (brightness >= 16) brightness = 0;
            setbrightness(brightness,palette,0);
        }
        if (keystatus[88] > 0)   //F12
        {
            screencapture("captxxxx.tga",eitherSHIFT);
            keystatus[88] = 0;
    	}*/

    mousz = 0;
    getmousevalues(&mousx,&mousy,&bstatus);
    mousx = (mousx<<16)+mousexsurp;
    mousy = (mousy<<16)+mouseysurp;

    if ((unrealedlook && !mskip) &&
            (((!mlook && (bstatus&2) && !(bstatus&(1|4)))) || ((bstatus&1) && !(bstatus&(2|4)))))
        mlook = 3;

    {
        ldiv_t ld;
        if (mlook)
        {
            ld = ldiv((int32_t)mousx, (int32_t)((1<<16)/(msens*0.5f))); mousx = ld.quot; mousexsurp = ld.rem;
            ld = ldiv((int32_t)mousy, (int32_t)((1<<16)/(msens*0.25f))); mousy = ld.quot; mouseysurp = ld.rem;
        }
        else
        {
            ld = ldiv((int32_t)mousx, (int32_t)((1<<16)/msens)); mousx = ld.quot; mousexsurp = ld.rem;
            ld = ldiv((int32_t)mousy, (int32_t)((1<<16)/msens)); mousy = ld.quot; mouseysurp = ld.rem;
        }
    }

    if (mlook == 3)
        mlook = 0;

    // UnrealEd:
    // rmb: mouselook
    // lbm: x:turn y:fwd/back local x
    // lmb&rmb: x:strafe y:up/dn (move in local yz plane)
    // mmb: fwd/back in viewing vector

    if (unrealedlook && !mskip)    //PK
    {
        if ((bstatus&1) && !(bstatus&(2|4)))
        {
            ang += mousx;
            xvect = -((mousy*(int32_t)sintable[(ang+2560)&2047])<<(3+pk_uedaccel));
            yvect = -((mousy*(int32_t)sintable[(ang+2048)&2047])<<(3+pk_uedaccel));

            if (noclip)
            {
                pos.x += xvect>>14;
                pos.y += yvect>>14;
                updatesector(pos.x,pos.y,&cursectnum);
            }
            else clipmove(&pos,&cursectnum,xvect,yvect,128L,4L<<8,4L<<8,CLIPMASK0);
        }
        else if (!mlook && (bstatus&2) && !(bstatus&(1|4)))
        {
            mlook=2;
        }
        else if ((bstatus&1) && (bstatus&2) && !(bstatus&4))
        {
            zmode = 2;
            xvect = -((mousx*(int32_t)sintable[(ang+2048)&2047])<<pk_uedaccel);
            yvect = -((mousx*(int32_t)sintable[(ang+1536)&2047])<<pk_uedaccel);
            pos.z += mousy<<(4+pk_uedaccel);
            if (noclip)
            {
                pos.x += xvect>>14;
                pos.y += yvect>>14;
                updatesectorz(pos.x,pos.y,pos.z,&cursectnum);
            }
            else clipmove(&pos,&cursectnum,xvect,yvect,128L,4L<<8,4L<<8,CLIPMASK0);
        }
        else if (bstatus&4)
        {
            zmode = 2;

            // horiz-100 of 200 is viewing at 326.4 build angle units (=atan(200/128)) upward
            tempint = getangle(128, horiz-100);

            xvect = -((mousy*
                       ((int32_t)sintable[(ang+2560)&2047]>>6)*
                       ((int32_t)sintable[(tempint+512)&2047])>>6)
                      <<pk_uedaccel);
            yvect = -((mousy*
                       ((int32_t)sintable[(ang+2048)&2047]>>6)*
                       ((int32_t)sintable[(tempint+512)&2047])>>6)
                      <<pk_uedaccel);

            zvect = mousy*(((int32_t)sintable[(tempint+2048)&2047])>>(10-pk_uedaccel));

            pos.z += zvect;
            if (noclip)
            {
                pos.x += xvect>>16;
                pos.y += yvect>>16;
                updatesectorz(pos.x,pos.y,pos.z,&cursectnum);
            }
            else clipmove(&pos,&cursectnum,xvect>>2,yvect>>2,128L,4L<<8,4L<<8,CLIPMASK0);
        }
    }

    if (mskip)
        mskip=0;
    else
    {
        if (mlook && !(unrealedlook && bstatus&(1|4)))
        {
            ang += mousx;
            horiz -= mousy;

//            if (mousy && !(mousy/4))
            //              horiz--;
            //        if (mousx && !(mousx/2))
            //          ang++;

            bclamp(&horiz, -99, 299);

            if (mlook == 1)
            {
                searchx = xdim>>1;
                searchy = ydim>>1;
            }
            osearchx = searchx-mousx;
            osearchy = searchy-mousy;
        }
        else if (!(unrealedlook && (bstatus&(1|2|4))))
        {
            osearchx = searchx;
            osearchy = searchy;
            searchx += mousx;
            searchy += mousy;

            bclamp(&searchx, 12, xdim-13);
            bclamp(&searchy, 12, ydim-13);
        }
    }

//    showmouse();

//    if (keystatus[0x3b] > 0) pos.x--;
//    if (keystatus[0x3c] > 0) pos.x++;
//    if (keystatus[0x3d] > 0) pos.y--;
//    if (keystatus[0x3e] > 0) pos.y++;
//    if (keystatus[0x43] > 0) ang--;
//    if (keystatus[0x44] > 0) ang++;

    if (keystatus[0x43])  // F9
    {
        if (mhk)
        {
            Bmemset(spriteext, 0, sizeof(spriteext_t) * MAXSPRITES);
            Bmemset(spritesmooth, 0, sizeof(spritesmooth_t) * (MAXSPRITES+MAXUNIQHUDID));
            delete_maphack_lights();
            mhk = 0;
            message("Maphacks disabled");
        }
        else
        {
            mhk = 1;
            loadmhk(1);
        }

        keystatus[0x43] = 0;
    }

    if (angvel != 0)          //ang += angvel * constant
    {
        //ENGINE calculates angvel for you
        doubvel = synctics;
        if (DOWN_BK(RUN))  //Lt. shift makes turn velocity 50% faster
            doubvel += (synctics>>1);
        ang += ((angvel*doubvel)>>4);
        ang = (ang+2048)&2047;
    }
    if ((vel|svel) != 0)
    {
        //Lt. shift doubles forward velocity
        doubvel = (1+(DOWN_BK(RUN)))*synctics;

        xvect = 0;
        yvect = 0;

        if (vel != 0)
        {
            xvect += (vel*doubvel*(int32_t)sintable[(ang+2560)&2047])>>3;
            yvect += (vel*doubvel*(int32_t)sintable[(ang+2048)&2047])>>3;
        }
        if (svel != 0)
        {
            xvect += (svel*doubvel*(int32_t)sintable[(ang+2048)&2047])>>3;
            yvect += (svel*doubvel*(int32_t)sintable[(ang+1536)&2047])>>3;
        }
        if (noclip)
        {
            pos.x += xvect>>14;
            pos.y += yvect>>14;
            updatesector(pos.x,pos.y,&cursectnum);
        }
        else clipmove(&pos,&cursectnum,xvect,yvect,128L,4L<<8,4L<<8,CLIPMASK0);
    }

    getzrange(&pos,cursectnum,&hiz,&hihit,&loz,&lohit,128L,CLIPMASK0);

    /*    if (keystatus[0x3a] > 0)
        {
            zmode++;
            if (zmode == 3) zmode = 0;
            if (zmode == 1) zlock = (loz-pos.z)&0xfffffc00;
            keystatus[0x3a] = 0;
    	}*/

    if (zmode == 0)
    {
        goalz = loz-(kensplayerheight<<8);  //playerheight pixels above floor
        if (goalz < hiz+(16<<8))  //ceiling&floor too close
            goalz = (loz+hiz)>>1;
        goalz += mousz;

        if (DOWN_BK(MOVEUP))  //A (stand high)
        {
            /* if (eitherCTRL)
               horiz = max(-100,horiz-((DOWN_BK(RUN)+1)*synctics*2));
               else */
            {
                goalz -= (16<<8);
                if (DOWN_BK(RUN))  //Either shift key
                    goalz -= (24<<8);
            }
        }
        if (DOWN_BK(MOVEDOWN))  //Z (stand low)
        {
            /* if (eitherCTRL)
               horiz = min(300,horiz+((DOWN_BK(RUN)+1)*synctics*2));
               else */
            {
                goalz += (12<<8);
                if (DOWN_BK(RUN))  //Either shift key
                    goalz += (12<<8);
            }
        }

        if (goalz != pos.z)
        {
            if (pos.z < goalz) hvel += 64;
            if (pos.z > goalz) hvel = ((goalz-pos.z)>>3);

            pos.z += hvel;
            if (pos.z > loz-(4<<8)) pos.z = loz-(4<<8), hvel = 0;
            if (pos.z < hiz+(4<<8)) pos.z = hiz+(4<<8), hvel = 0;
        }
    }
    else
    {
        goalz = pos.z;
        if (DOWN_BK(MOVEUP))  //A
        {
            if (eitherCTRL)
            {
                horiz = max(-100,horiz-((DOWN_BK(RUN)+1)*synctics*2));
            }
            else
            {
                if (zmode != 1)
                    goalz -= (8<<8);
                else
                {
                    zlock += (4<<8);
                    DOWN_BK(MOVEUP) = 0;
                }
            }
        }
        if (DOWN_BK(MOVEDOWN))  //Z (stand low)
        {
            if (eitherCTRL)
            {
                horiz = min(300,horiz+((DOWN_BK(RUN)+1)*synctics*2));
            }
            else
            {
                if (zmode != 1)
                    goalz += (8<<8);
                else if (zlock > 0)
                {
                    zlock -= (4<<8);
                    DOWN_BK(MOVEDOWN) = 0;
                }
            }
        }

        if (!noclip)
            bclamp(&goalz, hiz+(4<<8), loz-(4<<8));

        if (zmode == 1) goalz = loz-zlock;
        if (!noclip && (goalz < hiz+(4<<8)))
            goalz = ((loz+hiz)>>1);  //ceiling&floor too close
        if (zmode == 1) pos.z = goalz;

        if (goalz != pos.z)
        {
            //if (pos.z < goalz) hvel += (32<<DOWN_BK(RUN));
            //if (pos.z > goalz) hvel -= (32<<DOWN_BK(RUN));
            if (pos.z < goalz) hvel = ((synctics* 192)<<DOWN_BK(RUN));
            if (pos.z > goalz) hvel = ((synctics*-192)<<DOWN_BK(RUN));

            pos.z += hvel;

            if (!noclip)
            {
                if (pos.z > loz-(4<<8)) pos.z = loz-(4<<8), hvel = 0;
                if (pos.z < hiz+(4<<8)) pos.z = hiz+(4<<8), hvel = 0;
            }
        }
        else
            hvel = 0;
    }

    updatesectorz(pos.x,pos.y,pos.z, &cursectnum);

    searchit = 2;
    if (searchstat >= 0)
    {
        if ((bstatus&(1|2|4)) > 0)
            searchit = 0;

        if (keystatus[0x1f])  //S (insert sprite) (3D)
        {
            hitdata_t hitinfo;

            dax = 16384;
            day = divscale14(searchx-(xdim>>1), xdim>>1);
            rotatepoint(0,0, dax,day, ang, &dax,&day);

            hitscan((const vec3_t *)&pos,cursectnum,              //Start position
                    dax,day,(scale(searchy,200,ydim)-horiz)*2000, //vector of 3D ang
                    &hitinfo,CLIPMASK1);

            if (hitinfo.hitsect >= 0)
            {
                dax = hitinfo.pos.x;
                day = hitinfo.pos.y;
                if (gridlock && grid > 0)
                {
                    if (searchstat == 0 || searchstat == 4)
                        hitinfo.pos.z &= 0xfffffc00;
                    else
                        locktogrid(&dax, &day);
                }

                i = insert_sprite_common(hitinfo.hitsect, dax, day);

                if (i < 0)
                    message("Couldn't insert sprite.");
                else
                {
                    int32_t cz, fz;

                    if (somethingintab == 3)
                    {
                        sprite[i].picnum = temppicnum;
                        if (tilesizx[temppicnum] <= 0 || tilesizy[temppicnum] <= 0)
                        {
                            j = 0;
                            for (k=0; k<MAXTILES; k++)
                                if (tilesizx[k] > 0 && tilesizy[k] > 0)
                                {
                                    j = k;
                                    break;
                                }
                            sprite[i].picnum = j;
                        }

                        sprite[i].shade = tempshade;
                        sprite[i].pal = temppal;
                        sprite[i].xrepeat = max(tempxrepeat, 1);
                        sprite[i].yrepeat = max(tempyrepeat, 1);
                        sprite[i].cstat = tempcstat;
                    }

                    cz = getceilzofslope(hitinfo.hitsect, hitinfo.pos.x, hitinfo.pos.y);
                    fz = getflorzofslope(hitinfo.hitsect, hitinfo.pos.x, hitinfo.pos.y);

                    j = spriteheight(i, NULL)>>1;
                    sprite[i].z = hitinfo.pos.z;
                    if ((sprite[i].cstat&48)!=32)
                    {
                        if ((sprite[i].cstat&128) == 0)
                            bclamp(&sprite[i].z, cz+(j<<1), fz);
                        else
                            bclamp(&sprite[i].z, cz+j, fz-j);
                    }

                    if (searchstat == 0 || searchstat == 4)
                    {
                        sprite[i].cstat &= ~48;
                        sprite[i].cstat |= (16+64);

                        correct_ornamented_sprite(i, hitinfo.hitwall);
                    }
                    else
                        sprite[i].cstat |= (tilesizy[sprite[i].picnum]>=32);

                    updatenumsprites();
                    asksave = 1;

                    VM_OnEvent(EVENT_INSERTSPRITE3D, i);
                }
            }

            keystatus[0x1f] = 0;
        }

#if 0
        if (keystatus[0xd3] > 0)
        {
            if (searchstat == 3)
            {
                deletesprite(searchwall);
                updatenumsprites();
                asksave = 1;
            }
            keystatus[0xd3] = 0;
        }
#endif

        if (keystatus[0x3f]||keystatus[0x40])  //F5,F6
        {
            switch (searchstat)
            {
            case 1:
            case 2:
                ExtShowSectorData(searchsector); break;
            case 0:
            case 4:
                ExtShowWallData(searchwall); break;
            case 3:
                ExtShowSpriteData(searchwall); break;
            }
            keystatus[0x3f] = 0, keystatus[0x40] = 0;
        }
        if (keystatus[0x41]||keystatus[0x42])  //F7,F8
        {
            switch (searchstat)
            {
            case 1:
            case 2:
                ExtEditSectorData(searchsector); break;
            case 0:
            case 4:
                ExtEditWallData(searchwall); break;
            case 3:
                ExtEditSpriteData(searchwall); break;
            }
            keystatus[0x41] = 0, keystatus[0x42] = 0;
        }

    }

    if (keystatus[buildkeys[BK_MODE2D_3D]])  // Enter
    {

        vid_gamma_3d = vid_gamma;
        vid_contrast_3d = vid_contrast;
        vid_brightness_3d = vid_brightness;

        vid_gamma = vid_contrast = 1.0;
        vid_brightness = 0.0;

        setbrightness(0,0,0);

        keystatus[buildkeys[BK_MODE2D_3D]] = 0;
        overheadeditor();
        keystatus[buildkeys[BK_MODE2D_3D]] = 0;

        vid_gamma = vid_gamma_3d;
        vid_contrast = vid_contrast_3d;
        vid_brightness = vid_brightness_3d;

        vid_gamma_3d = vid_contrast_3d = vid_brightness_3d = -1;

        setbrightness(GAMMA_CALC,0,0);
    }
}

char changechar(char dachar, int32_t dadir, char smooshyalign, char boundcheck)
{
    if (dadir < 0)
    {
        if ((dachar > 0) || (boundcheck == 0))
        {
            dachar--;
            if (smooshyalign > 0)
                dachar = (dachar&0xf8);
        }
    }
    else if (dadir > 0)
    {
        if ((dachar < 255) || (boundcheck == 0))
        {
            dachar++;
            if (smooshyalign > 0)
            {
                if (dachar >= 256-8) dachar = 255;
                else dachar = ((dachar+7)&0xf8);
            }
        }
    }
    return(dachar);
}


////////////////////// OVERHEADEDITOR //////////////////////

int32_t inside_editor(const vec3_t *pos, int32_t searchx, int32_t searchy, int32_t zoom,
                      int32_t x, int32_t y, int16_t sectnum)
{
    if (!m32_sideview)
        return inside(x, y, sectnum);

    // if in side-view mode, use the screen coords instead
    {
        int32_t dst = MAXSECTORS+M32_FIXME_SECTORS-1, i, oi;
        int32_t srcw=sector[sectnum].wallptr, dstw=MAXWALLS;
        int32_t ret;

        if (sector[sectnum].wallnum > M32_FIXME_WALLS)
            return -1;

        Bmemcpy(&sector[dst], &sector[sectnum], sizeof(sectortype));
        sector[dst].wallptr = dstw;

        Bmemcpy(&wall[dstw], &wall[srcw], sector[dst].wallnum*sizeof(walltype));
        for (i=dstw, oi=srcw; i<dstw+sector[dst].wallnum; i++, oi++)
        {
            wall[i].point2 += dstw-srcw;

            screencoords(&wall[i].x, &wall[i].y, wall[i].x-pos->x, wall[i].y-pos->y, zoom);
            wall[i].y += getscreenvdisp(getflorzofslope(sectnum,wall[oi].x,wall[oi].y)-pos->z, zoom);
            wall[i].x += halfxdim16;
            wall[i].y += midydim16;
        }

        i = numsectors;
        numsectors = dst+1;
        ret = inside(searchx, searchy, dst);
        numsectors = i;
        return ret;
    }
}

static inline void drawline16base(int32_t bx, int32_t by, int32_t x1, int32_t y1, int32_t x2, int32_t y2, char col)
{
    drawline16(bx+x1, by+y1, bx+x2, by+y2, col);
}

static void drawsmalllabel(const char *text, char col, char backcol, int32_t dax, int32_t day)
{
    int32_t x1, y1, x2, y2;

    x1 = halfxdim16+dax-(Bstrlen(text)<<1);
    y1 = midydim16+day-4;
    x2 = x1 + (Bstrlen(text)<<2)+2;
    y2 = y1 + 7;

    if ((x1 > 3) && (x2 < xdim) && (y1 > 1) && (y2 < ydim16))
    {
        printext16(x1,y1, col,backcol, text,1);
        drawline16(x1-1,y1-1, x2-3,y1-1, backcol);
        drawline16(x1-1,y2+1, x2-3,y2+1, backcol);

        drawline16(x1-2,y1, x1-2,y2, backcol);
        drawline16(x2-2,y1, x2-2,y2, backcol);
        drawline16(x2-3,y1, x2-3,y2, backcol);
    }
}

// backup highlighted sectors with sprites as mapinfo for later restoration
// return values:
//  -1: highlightsectorcnt<=0
//  -2: out of mem
//   0: ok
static int32_t backup_highlighted_map(mapinfofull_t *mapinfo)
{
    int32_t i, j, k, m, tmpnumwalls=0, tmpnumsprites=0;

    if (highlightsectorcnt <= 0)
        return -1;

    // count walls & sprites
    for (i=0; i<highlightsectorcnt; i++)
    {
        tmpnumwalls += sector[highlightsector[i]].wallnum;

        m = headspritesect[highlightsector[i]];
        while (m != -1)
        {
            tmpnumsprites++;
            m = nextspritesect[m];
        }
    }

    // allocate temp storage
    mapinfo->sector = Bmalloc(highlightsectorcnt * sizeof(sectortype));
    if (!mapinfo->sector) return -2;
    mapinfo->wall = Bmalloc(tmpnumwalls * sizeof(walltype));
    if (!mapinfo->wall) { Bfree(mapinfo->sector); return -2; }
    if (tmpnumsprites>0)
    {
        mapinfo->sprite = Bmalloc(tmpnumsprites * sizeof(spritetype));
        if (!mapinfo->sprite)
        {
            Bfree(mapinfo->sector);
            Bfree(mapinfo->wall);
            return -2;
        }
    }


    // copy everything over
    tmpnumwalls = 0;
    tmpnumsprites = 0;
    for (i=0; i<highlightsectorcnt; i++)
    {
        k = highlightsector[i];
        Bmemcpy(&mapinfo->sector[i], &sector[k], sizeof(sectortype));
        mapinfo->sector[i].wallptr = tmpnumwalls;

        for (j=0; j<sector[k].wallnum; j++)
        {
            Bmemcpy(&mapinfo->wall[tmpnumwalls+j], &wall[sector[k].wallptr+j], sizeof(walltype));
            mapinfo->wall[tmpnumwalls+j].point2 += (tmpnumwalls-sector[k].wallptr);
            mapinfo->wall[tmpnumwalls+j].nextsector = -1;
            mapinfo->wall[tmpnumwalls+j].nextwall = -1;
        }
        tmpnumwalls += j;

        m = headspritesect[highlightsector[i]];
        while (m != -1)
        {
            Bmemcpy(&mapinfo->sprite[tmpnumsprites], &sprite[m], sizeof(spritetype));
            mapinfo->sprite[tmpnumsprites].sectnum = i;
            m = nextspritesect[m];
            tmpnumsprites++;
        }
    }


    mapinfo->numsectors = highlightsectorcnt;
    mapinfo->numwalls = tmpnumwalls;
    mapinfo->numsprites = tmpnumsprites;

    return 0;
}

static void mapinfofull_free(mapinfofull_t *mapinfo)
{
    Bfree(mapinfo->sector);
    Bfree(mapinfo->wall);
    if (mapinfo->numsprites>0)
        Bfree(mapinfo->sprite);
}

// restore map saved with backup_highlighted_map, also
// frees mapinfo's sector, wall, (sprite) in any case.
// return values:
//  -1: limits exceeded
//   0: ok
static int32_t restore_highlighted_map(mapinfofull_t *mapinfo)
{
    int32_t i, j, sect, onumsectors=numsectors, newnumsectors, newnumwalls;

    updatenumsprites();
    if (numsectors+mapinfo->numsectors>MAXSECTORS || numwalls+mapinfo->numwalls>MAXWALLS
            || numsprites+mapinfo->numsprites>MAXSPRITES)
    {
        mapinfofull_free(mapinfo);
        return -1;
    }

    newnumsectors = numsectors + mapinfo->numsectors;
    newnumwalls = numwalls + mapinfo->numwalls;

    // copy sectors & walls
    Bmemcpy(&sector[numsectors], mapinfo->sector, mapinfo->numsectors*sizeof(sectortype));
    Bmemcpy(&wall[numwalls], mapinfo->wall, mapinfo->numwalls*sizeof(walltype));

    // tweak index members
    for (i=numsectors; i<newnumsectors; i++)
        sector[i].wallptr += numwalls;
    for (i=numwalls; i<newnumwalls; i++)
    {
        wall[i].point2 += numwalls;
        wall[i].x += BXY_MAX*2;
    }

    // reconstruct wall connections
    numsectors=newnumsectors;  // needed now for checksectorpointer
//    checksectorpointer_warn = 0;
    Bmemset(hlsectorbitmap, 0, sizeof(hlsectorbitmap));
    for (i=onumsectors; i<newnumsectors; i++)
    {
        for (j=sector[i].wallptr; j<sector[i].wallptr+sector[i].wallnum; j++)
            checksectorpointer(j, i);
        hlsectorbitmap[i>>3] |= (1<<(i&7));
    }
    for (i=numwalls; i<newnumwalls; i++)
        wall[i].x -= BXY_MAX*2;
//    checksectorpointer_warn = 1;

    // insert sprites
    for (i=0; i<mapinfo->numsprites; i++)
    {
        sect = onumsectors+mapinfo->sprite[i].sectnum;
        j = insertsprite(sect, mapinfo->sprite[i].statnum);
        Bmemcpy(&sprite[j], &mapinfo->sprite[i], sizeof(spritetype));
        sprite[j].sectnum = sect;
    }

    numwalls = newnumwalls;
    updatenumsprites();

    update_highlightsector();

    mapinfofull_free(mapinfo);
    return 0;
}


static int32_t newnumwalls=-1;

static void ovh_whiteoutgrab()
{
    int32_t i, j, k, startwall, endwall;

    //White out all bordering lines of grab that are
    //not highlighted on both sides
    for (i=highlightsectorcnt-1; i>=0; i--)
    {
        startwall = sector[highlightsector[i]].wallptr;
        endwall = startwall + sector[highlightsector[i]].wallnum;
        for (j=startwall; j<endwall; j++)
        {
            if (wall[j].nextwall >= 0)
            {
//                for (k=highlightsectorcnt-1; k>=0; k--)
//                    if (highlightsector[k] == wall[j].nextsector)
//                        break;
                k = wall[j].nextsector;
                if ((hlsectorbitmap[k>>3]&(1<<(k&7)))==0)
//                if (k < 0)
                {
                    NEXTWALL(j).nextwall = -1;
                    NEXTWALL(j).nextsector = -1;
                    wall[j].nextwall = -1;
                    wall[j].nextsector = -1;
                }
            }
        }
    }
}

static void duplicate_selected_sectors()
{
    int32_t i, j, startwall, endwall, newnumsectors, newwalls = 0;
    int32_t minx=INT_MAX, maxx=INT_MIN, miny=INT_MAX, maxy=INT_MIN, dx, dy;

    for (i=0; i<highlightsectorcnt; i++)
        newwalls += sector[highlightsector[i]].wallnum;

    if (highlightsectorcnt + numsectors <= MAXSECTORS && numwalls+newwalls <= MAXWALLS)
    {
        int16_t *oldtonewsect = Bmalloc(numsectors * sizeof(int16_t));

        if (oldtonewsect)
        {
            for (i=0; i<numsectors; i++)
                oldtonewsect[i] = -1;
            for (i=0; i<highlightsectorcnt; i++)
                oldtonewsect[highlightsector[i]] = numsectors+i;
        }
        else
            message("warning: out of memory!");

        newnumsectors = numsectors;
        newnumwalls = numwalls;
        for (i=0; i<highlightsectorcnt; i++)
        {
            copysector(highlightsector[i], newnumsectors, newnumwalls, 1, oldtonewsect);
            newnumsectors++;
            newnumwalls += sector[highlightsector[i]].wallnum;
        }

        if (oldtonewsect)
            Bfree(oldtonewsect);

        Bmemset(hlsectorbitmap, 0, sizeof(hlsectorbitmap));
        for (i=0; i<highlightsectorcnt; i++)
        {
            // first, make red lines of old selected sectors, effectively
            // restoring the original state
            for (WALLS_OF_SECTOR(highlightsector[i], j))
            {
                if (wall[j].nextwall >= 0)
                    checksectorpointer(wall[j].nextwall,wall[j].nextsector);
                checksectorpointer(j, highlightsector[i]);

                minx = min(minx, wall[j].x);
                maxx = max(maxx, wall[j].x);
                miny = min(miny, wall[j].y);
                maxy = max(maxy, wall[j].y);
            }

            // Then, highlight the ones just copied.
            // These will have all walls whited out.
            j = numsectors + i;
            hlsectorbitmap[j>>3] |= (1<<(j&7));
        }

        // displace walls of new sectors by a small amount
        dx = 512; //((maxx-minx)+255)&~255;
        dy = -256; //((maxy-miny)+255)&~255;
        if (maxx+dx >= editorgridextent) dx*=-1;
        if (minx+dx <= -editorgridextent) dx*=-1;
        if (maxy+dy >= editorgridextent) dy*=-1;
        if (miny+dy <= -editorgridextent) dy*=-1;

        for (i=numsectors; i<newnumsectors; i++)
        {
            for (j=sector[i].wallptr; j<sector[i].wallptr+sector[i].wallnum; j++)
            {
                wall[j].x += dx;
                wall[j].y += dy;
            }

            for (j=headspritesect[i]; j>=0; j=nextspritesect[j])
            {
                sprite[j].x += dx;
                sprite[j].y += dy;
            }
        }

        numsectors = newnumsectors;
        numwalls = newnumwalls;

        update_highlightsector();  // must be after numsectors = newnumsectors

        newnumwalls = -1;
        newnumsectors = -1;

        updatenumsprites();
        printmessage16("Sectors duplicated and stamped.");
        asksave = 1;
    }
    else
    {
        printmessage16("Copying sectors would exceed sector or wall limit.");
    }
}

static void duplicate_selected_sprites()
{
    int32_t i, j, k=0;

    for (i=0; i<highlightcnt; i++)
        k += ((highlight[i]&0xc000) == 16384);

    if (highlightcnt + k <= MAXSPRITES)
    {
        for (i=0; i<highlightcnt; i++)
            if ((highlight[i]&0xc000) == 16384)
            {
                //duplicate sprite
                k = (highlight[i]&16383);
                j = insertsprite(sprite[k].sectnum,sprite[k].statnum);
                Bmemcpy(&sprite[j],&sprite[k],sizeof(spritetype));
//                sprite[j].sectnum = sprite[k].sectnum;   //Don't let memcpy overwrite sector!
//                setsprite(j,(vec3_t *)&sprite[j]);
            }
        updatenumsprites();
        printmessage16("Sprites duplicated and stamped.");
        asksave = 1;
    }
    else
    {
        printmessage16("Copying sprites would exceed sprite limit.");
    }
}

static void correct_ornamented_sprite(int32_t i, int32_t hitw)
{
    int32_t j;

    if (hitw >= 0)
        sprite[i].ang = (getangle(POINT2(hitw).x-wall[hitw].x,
                                  POINT2(hitw).y-wall[hitw].y)+512)&2047;

    //Make sure sprite's in right sector
    if (inside(sprite[i].x, sprite[i].y, sprite[i].sectnum) == 0)
    {
        j = wall[hitw].point2;
        sprite[i].x -= ksgn(wall[j].y-wall[hitw].y);
        sprite[i].y += ksgn(wall[j].x-wall[hitw].x);
    }
}

void DoSpriteOrnament(int32_t i)
{
    hitdata_t hitinfo;

    hitscan((const vec3_t *)&sprite[i],sprite[i].sectnum,
            sintable[(sprite[i].ang+1536)&2047],
            sintable[(sprite[i].ang+1024)&2047],
            0,
            &hitinfo,CLIPMASK1);

    sprite[i].x = hitinfo.pos.x;
    sprite[i].y = hitinfo.pos.y;
    sprite[i].z = hitinfo.pos.z;
    changespritesect(i, hitinfo.hitsect);

    correct_ornamented_sprite(i, hitinfo.hitwall);
}

void update_highlight()
{
    int32_t i;

    highlightcnt = 0;
    for (i=0; i<numwalls; i++)
        if (show2dwall[i>>3]&(1<<(i&7)))
            highlight[highlightcnt++] = i;
    for (i=0; i<MAXSPRITES; i++)
        if (sprite[i].statnum < MAXSTATUS)
        {
            if (show2dsprite[i>>3]&(1<<(i&7)))
                highlight[highlightcnt++] = i+16384;
        }
        else
            show2dsprite[i>>3] &= ~(1<<(i&7));

    if (highlightcnt == 0)
        highlightcnt = -1;
}

void update_highlightsector()
{
    int32_t i;

    highlightsectorcnt = 0;
    for (i=0; i<numsectors; i++)
        if (hlsectorbitmap[i>>3]&(1<<(i&7)))
            highlightsector[highlightsectorcnt++] = i;

    if (highlightsectorcnt==0)
        highlightsectorcnt = -1;
}

// hook run after handleevents in side view
static void sideview_filter_keys(void)
{
    uint32_t i;

    for (i=0; i<sizeof(keystatus)/sizeof(keystatus[0]); i++)
    {
        switch (i)
        {
        case 0xd2: case 0xd3:  // ins, del
        case 0x2e: case 0x39:  // c, space
//        case 0xb8:  // ralt
            keystatus[i] = 0;
            break;
        }
    }
}

void m32_setkeyfilter(int32_t on)
{
    if (m32_sideview && on)
    {
        after_handleevents_hook = &sideview_filter_keys;
        clearkeys();
    }
    else
        after_handleevents_hook = 0;
}

// Get average point of sectors
static void get_sectors_center(const int16_t *sectors, int32_t numsecs, int32_t *cx, int32_t *cy)
{
    int32_t i, j, k=0, dax = 0, day = 0;
    int32_t startwall, endwall;

    for (i=0; i<numsecs; i++)
    {
        for (WALLS_OF_SECTOR(sectors[i], j))
        {
            dax += wall[j].x;
            day += wall[j].y;
            k++;
        }
    }

    if (k > 0)
    {
        dax /= k;
        day /= k;
    }

    *cx = dax;
    *cy = day;
}

static int32_t insert_sprite_common(int32_t sucksect, int32_t dax, int32_t day)
{
    int32_t i, j, k;

    i = insertsprite(sucksect,0);
    if (i < 0)
        return -1;

    sprite[i].x = dax, sprite[i].y = day;
    sprite[i].cstat = DEFAULT_SPRITE_CSTAT;
    sprite[i].shade = 0;
    sprite[i].pal = 0;
    sprite[i].xrepeat = 64, sprite[i].yrepeat = 64;
    sprite[i].xoffset = 0, sprite[i].yoffset = 0;
    sprite[i].ang = 1536;
    sprite[i].xvel = 0; sprite[i].yvel = 0; sprite[i].zvel = 0;
    sprite[i].owner = -1;
    sprite[i].clipdist = 32;
    sprite[i].lotag = 0;
    sprite[i].hitag = 0;
    sprite[i].extra = -1;

    Bmemset(localartfreq, 0, sizeof(localartfreq));
    for (k=0; k<MAXSPRITES; k++)
        localartfreq[sprite[k].picnum] += (sprite[k].statnum < MAXSTATUS);

    j = 0;
    for (k=0; k<MAXTILES; k++)
        if (localartfreq[k] > localartfreq[j])
            j = k;

    if (localartfreq[j] > 0)
        sprite[i].picnum = j;
    else
        sprite[i].picnum = 0;

    return i;
}


static void fade_screen()
{
    char blackcol=editorcolors[0], greycol=whitecol-25, *cp;
    int32_t i;

    begindrawing();
    cp = (char *)frameplace;
    for (i=0; i<bytesperline*(ydim-STATUS2DSIZ2); i++, cp++)
        if (*cp==greycol)
            *cp = blackcol;
        else if (*cp != blackcol)
            *cp = greycol;
    enddrawing();
    showframe(1);
}

static void copy_some_wall_members(int16_t dst, int16_t src)
{
    //                                 x  y  p2 nw ns cs p  op sh pl xr yr xp yp lo hi ex
    static const walltype nullwall = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 8, 0, 0, 0, 0, -1 };
    walltype *dstwal=&wall[dst];
    const walltype *srcwal = src >= 0 ? &wall[src] : &nullwall;

    dstwal->cstat = srcwal->cstat;
    dstwal->shade = srcwal->shade;
    dstwal->yrepeat = srcwal->yrepeat;
    fixrepeats(dst);  // xrepeat
    dstwal->picnum = srcwal->picnum;
    dstwal->overpicnum = srcwal->overpicnum;

    dstwal->nextwall = -1;
    dstwal->nextsector = -1;

    dstwal->pal = srcwal->pal;
    dstwal->xpanning = srcwal->xpanning;
    dstwal->ypanning = srcwal->ypanning;
    dstwal->lotag = 0; //srcwal->lotag;
    dstwal->hitag = 0; //srcwal->hitag;
    dstwal->extra = -1; //srcwal->extra;
}

// helpers for often needed ops:
static int32_t copyloop1(int16_t *danumwalls, int32_t *m)
{
    if (*danumwalls >= MAXWALLS + M32_FIXME_WALLS)
        return 1;

    Bmemcpy(&wall[*danumwalls], &wall[*m], sizeof(walltype));
    wall[*danumwalls].point2 = *danumwalls+1;
    (*danumwalls)++;
    *m = wall[*m].point2;

    return 0;
}

static void updatesprite1(int16_t i)
{
    setsprite(i, (vec3_t *)&sprite[i]);

    if ((sprite[i].cstat&48)!=32 && sprite[i].sectnum>=0)
    {
        int32_t tempint, cz, fz;
        tempint = spriteheight(i, NULL);
        if (sprite[i].cstat&128)
            tempint >>= 1;
        cz = getceilzofslope(sprite[i].sectnum, sprite[i].x,sprite[i].y);
        fz = getflorzofslope(sprite[i].sectnum, sprite[i].x,sprite[i].y);
        sprite[i].z = max(sprite[i].z, cz+tempint);
        sprite[i].z = min(sprite[i].z, fz);
    }
}

static int32_t ask_if_sure(const char *query, uint32_t flags);
static int32_t ask_above_or_below();

// returns:
//  0: continue
// >0: newnumwalls
// <0: error
static int32_t trace_loop(int32_t j, uint8_t *visitedwall, int16_t *ignore_ret, int16_t *refsect_ret)
{
    int16_t refsect, ignore;
    int32_t k, n, refwall;

    if (wall[j].nextwall>=0 || (visitedwall[j>>3]&(1<<(j&7))))
        return 0;

    n=2*MAXWALLS;  // simple inf loop check
    refwall = j;
    k = numwalls;

    ignore = 0;

    if (ignore_ret)
    {
        refsect = -1;
        updatesectorexclude(wall[j].x, wall[j].y, &refsect, hlsectorbitmap);
        if (refsect<0)
            return -1;
    }

    do
    {
        if (j!=refwall && visitedwall[j>>3]&(1<<(j&7)))
            ignore = 1;
        visitedwall[j>>3] |= (1<<(j&7));

        if (ignore_ret)
        {
            if (inside(wall[j].x, wall[j].y, refsect) != 1)
                ignore = 1;
        }

        if (!ignore)
        {
            if (k>=MAXWALLS)
            {
                message("Wall limits exceeded while tracing outer loop.");
                return -2;
            }

            Bmemcpy(&wall[k], &wall[j], sizeof(walltype));
            wall[k].point2 = k+1;
            wall[k].nextsector = wall[k].nextwall = wall[k].extra = -1;
            k++;
        }

        j = wall[j].point2;
        n--;

        while (wall[j].nextwall>=0 && n>0)
        {
            j = wall[wall[j].nextwall].point2;
//            if (j!=refwall && (visitedwall[j>>3]&(1<<(j&7))))
//                ignore = 1;
//            visitedwall[j>>3] |= (1<<(j&7));
            n--;
        }
    }
    while (j!=refwall && n>0);

    if (j!=refwall)
    {
        message("internal error while tracing outer loop: didn't reach refwall");
        return -3;
    }

    if (ignore_ret)
    {
        *ignore_ret = ignore;
        if (refsect_ret)
            *refsect_ret = refsect;
    }

    return k;
}

void overheadeditor(void)
{
    char buffer[80], *dabuffer, ch;
    int32_t i, j, k, m=0, mousxplc, mousyplc, firstx=0, firsty=0, oposz, col;
    int32_t tempint, tempint1, tempint2, doubvel;
    int32_t startwall=0, endwall, dax, day, x1, y1, x2, y2, x3, y3, x4, y4;
    int32_t highlightx1, highlighty1, highlightx2, highlighty2, xvect, yvect;
    int16_t pag, suckwall=0, sucksect, split=0, bad;
    int16_t splitsect=0, joinsector[2];
    int16_t splitstartwall=0, loopnum;
    int32_t mousx, mousy, bstatus;
    int32_t centerx, centery, circlerad;
    int16_t circlepoints, circleang1, circleang2, circleangdir;
    int32_t sectorhighlightx=0, sectorhighlighty=0;
    int16_t cursectorhighlight, sectorhighlightstat;
    walltype *wal;
    int32_t prefixarg = 0;
    int32_t resetsynctics = 0, lasttick=getticks(), waitdelay=totalclock, lastdraw=getticks();
    int32_t tsign;

    m32_setkeyfilter(1);

    //qsetmode640480();
    qsetmodeany(xdim2d,ydim2d);
    xdim2d = xdim;
    ydim2d = ydim;

    osearchx = searchx;
    osearchy = searchy;

    searchx = clamp(scale(searchx,xdim2d,xdimgame), 8, xdim2d-8-1);
    searchy = clamp(scale(searchy,ydim2d-STATUS2DSIZ2,ydimgame), 8, ydim2d-STATUS2DSIZ-8-1);
    oposz = pos.z;

    begindrawing(); //{{{

    CLEARLINES2D(0, ydim, 0);

    ydim16 = ydim;
#if 0
    drawline16(0,ydim-STATUS2DSIZ2,xdim-1,ydim-STATUS2DSIZ2,editorcolors[1]);
    drawline16(0,ydim-1,xdim-1,ydim-1,1);
    drawline16(0,ydim-STATUS2DSIZ,0,ydim-1,1);
    drawline16(xdim-1,ydim-STATUS2DSIZ,xdim-1,ydim-1,1);
    drawline16(0,ydim-STATUS2DSIZ+24,xdim-1,ydim-STATUS2DSIZ+24,1);
    drawline16(192-24,ydim-STATUS2DSIZ,192-24,ydim-STATUS2DSIZ+24,1);

    printmessage16("Version: "VERSION);
    drawline16(0,ydim-1-20,xdim-1,ydim-1-20,1);
    drawline16(256,ydim-1-20,256,ydim-1,1);
#endif
    ydim16 = ydim-STATUS2DSIZ2;

    enddrawing(); //}}}

    pag = 0;
    cursectorhighlight = -1;
    lastpm16time = -1;

    update_highlightsector();
    ovh_whiteoutgrab();

    highlightcnt = -1;
    Bmemset(show2dwall, 0, sizeof(show2dwall));  //Clear all highlights
    Bmemset(show2dsprite, 0, sizeof(show2dsprite));

    sectorhighlightstat = -1;
    newnumwalls = -1;
    joinsector[0] = -1;
    circlewall = -1;
    circlepoints = 7;
    bstatus = 0;

    while ((keystatus[buildkeys[BK_MODE2D_3D]]>>1) == 0)
    {
        if (!((vel|angvel|svel) //DOWN_BK(MOVEFORWARD) || DOWN_BK(MOVEBACKWARD) || DOWN_BK(TURNLEFT) || DOWN_BK(TURNRIGHT)
                || DOWN_BK(MOVEUP) || DOWN_BK(MOVEDOWN) || keystatus[0x10] || keystatus[0x11]
                || keystatus[0x48] || keystatus[0x4b] || keystatus[0x4d] || keystatus[0x50]  // keypad keys
                || bstatus || OSD_IsMoving()))
        {
            if (totalclock > waitdelay)
            {
                uint32_t ms = (highlightsectorcnt>0) ? 75 : 200;
                // wait for event, timeout after 200 ms - (last loop time)
                idle_waitevent_timeout(ms - min(getticks()-lasttick, ms));
                // have synctics reset to 0 after we've slept to avoid zooming out to the max instantly
                resetsynctics = 1;
            }
        }
        else waitdelay = totalclock + 30; // should be 250 ms

        lasttick = getticks();

        if (handleevents())
        {
            if (quitevent)
            {
                keystatus[1] = 1;
                quitevent = 0;
            }
        }

        if (resetsynctics)
        {
            resetsynctics = 0;
            lockclock = totalclock;
            synctics = 0;
        }

        OSD_DispatchQueued();

        if (totalclock < 120*3)
            printmessage16("Uses BUILD technology by Ken Silverman.");
        else if (totalclock < 120*6)
        {
            printmessage16("Press F1 for help.  This is a test release; always keep backups of your maps.");
            //        printext16(8L,ydim-STATUS2DSIZ+32L,editorcolors[9],-1,kensig,0);
        }

        oldmousebstatus = bstatus;
        getmousevalues(&mousx,&mousy,&bstatus);
        mousx = (mousx<<16)+mousexsurp;
        mousy = (mousy<<16)+mouseysurp;
        {
            ldiv_t ld;
            ld = ldiv((int32_t)(mousx), (1<<16)); mousx = ld.quot; mousexsurp = ld.rem;
            ld = ldiv((int32_t)(mousy), (1<<16)); mousy = ld.quot; mouseysurp = ld.rem;
        }
        searchx += mousx;
        searchy += mousy;

        bclamp(&searchx, 8, xdim-8-1);
        bclamp(&searchy, 8, ydim-8-1);

        /*      if (keystatus[0x3b] > 0) pos.x--, keystatus[0x3b] = 0;
        		if (keystatus[0x3c] > 0) pos.x++, keystatus[0x3c] = 0;
        		if (keystatus[0x3d] > 0) pos.y--, keystatus[0x3d] = 0;
        		if (keystatus[0x3e] > 0) pos.y++, keystatus[0x3e] = 0;
        		if (keystatus[0x43] > 0) ang--, keystatus[0x43] = 0;
                if (keystatus[0x44] > 0) ang++, keystatus[0x44] = 0; */

        if (angvel != 0)          //ang += angvel * constant
        {
            //ENGINE calculates angvel for you
            doubvel = synctics;
            if (DOWN_BK(RUN))  //Lt. shift makes turn velocity 50% faster
                doubvel += (synctics>>1);
            ang += ((angvel*doubvel)>>4);
            ang = (ang+2048)&2047;
        }
        if ((vel|svel) != 0)
        {
            doubvel = synctics;
            if (DOWN_BK(RUN))     //Lt. shift doubles forward velocity
                doubvel += synctics;
            xvect = 0, yvect = 0;
            if (vel != 0)
            {
                xvect += (vel*doubvel*(int32_t)sintable[(ang+2560)&2047])>>3;
                yvect += (vel*doubvel*(int32_t)sintable[(ang+2048)&2047])>>3;
            }
            if (svel != 0)
            {
                xvect += (svel*doubvel*(int32_t)sintable[(ang+2048)&2047])>>3;
                yvect += (svel*doubvel*(int32_t)sintable[(ang+1536)&2047])>>3;
            }
            if (noclip)
            {
                pos.x += xvect>>14;
                pos.y += yvect>>14;
                updatesector(pos.x,pos.y,&cursectnum);
            }
            else clipmove(&pos,&cursectnum,xvect,yvect,128L,4L<<8,4L<<8,CLIPMASK0);
        }

        getpoint(searchx,searchy,&mousxplc,&mousyplc);
        linehighlight = getlinehighlight(mousxplc, mousyplc, linehighlight);

        if (newnumwalls >= numwalls)
        {
            // if we're in the process of drawing a wall, set the end point's coordinates
            dax = mousxplc;
            day = mousyplc;
            adjustmark(&dax,&day,newnumwalls);
            wall[newnumwalls].x = dax;
            wall[newnumwalls].y = day;
        }

        ydim16 = ydim;// - STATUS2DSIZ2;
        midydim16 = ydim>>1;

        tempint = numwalls;
        numwalls = newnumwalls;
        if (numwalls < 0)
            numwalls = tempint;

        if ((getticks() - lastdraw) >= 5 || (vel|angvel|svel) || DOWN_BK(MOVEUP) || DOWN_BK(MOVEDOWN)
                || mousx || mousy || bstatus || keystatus[0x10] || keystatus[0x11]
                || newnumwalls>=0 || OSD_IsMoving())
        {
            lastdraw = getticks();

            clear2dscreen();

            setup_sideview_sincos();

            if (graphicsmode && !m32_sideview)
            {
                Bmemset(show2dsector, 0xff, sizeof(show2dsector));
                setview(0, 0, xdim-1, ydim16-1);

                if (graphicsmode == 2)
                    totalclocklock = totalclock;

                drawmapview(pos.x, pos.y, zoom, 1536);
            }

            draw2dgrid(pos.x,pos.y,pos.z,cursectnum,ang,zoom,grid);

            ExtPreCheckKeys();

            {
                int32_t cx, cy;

                // Draw brown arrow (start)
                screencoords(&x2, &y2, startposx-pos.x,startposy-pos.y, zoom);
                if (m32_sideview)
                    y2 += getscreenvdisp(startposz-pos.z, zoom);

                cx = halfxdim16+x2;
                cy = midydim16+y2;
                if ((cx >= 2 && cx <= xdim-3) && (cy >= 2 && cy <= ydim16-3))
                {
                    int16_t angofs = m32_sideview ? m32_sideang : 0;
                    x1 = mulscale11(sintable[(startang+angofs+2560)&2047],zoom) / 768;
                    y1 = mulscale11(sintable[(startang+angofs+2048)&2047],zoom) / 768;
                    i = scalescreeny(x1);
                    j = scalescreeny(y1);
                    begindrawing();	//{{{
                    drawline16base(cx,cy, x1,j, -x1,-j, editorcolors[2]);
                    drawline16base(cx,cy, x1,j, +y1,-i, editorcolors[2]);
                    drawline16base(cx,cy, x1,j, -y1,+i, editorcolors[2]);
                    enddrawing();	//}}}
                }
            }

            draw2dscreen(&pos,cursectnum,ang,zoom,grid);
            VM_OnEvent(EVENT_DRAW2DSCREEN, -1);

            begindrawing();	//{{{
            if (showtags == 1)
            {
                if (zoom >= 768)
                {
                    for (i=0; i<numsectors; i++)
                    {
                        int32_t vdisp = 0;
                        int16_t secshort = i;

                        dabuffer = (char *)ExtGetSectorCaption(i);
                        if (dabuffer[0] == 0)
                            continue;

                        get_sectors_center(&secshort, 1, &dax, &day);

                        if (m32_sideview)
                            vdisp = getscreenvdisp(getflorzofslope(i,dax,day)-pos.z, zoom);
                        screencoords(&dax,&day, dax-pos.x,day-pos.y, zoom);
                        if (m32_sideview)
                            day += vdisp;

                        drawsmalllabel(dabuffer, editorcolors[0], editorcolors[7], dax, day);
                    }
                }

                x3 = pos.x + divscale14(-halfxdim16,zoom);
                y3 = pos.y + divscale14(-(midydim16-4),zoom);
                x4 = pos.x + divscale14(halfxdim16,zoom);
                y4 = pos.y + divscale14(ydim16-(midydim16-4),zoom);

                if (newnumwalls >= 0)
                {
                    for (i=newnumwalls; i>=tempint; i--)
                        wall[i].cstat |= (1<<14);
                }

                i = numwalls-1;
                if (newnumwalls >= 0)
                    i = newnumwalls-1;
                for (wal=&wall[i]; i>=0; i--,wal--)
                {
                    if (zoom < 768 && !(wal->cstat & (1<<14)))
                        continue;

                    //Get average point of wall
                    dax = (wal->x+wall[wal->point2].x)>>1;
                    day = (wal->y+wall[wal->point2].y)>>1;
                    if ((dax > x3) && (dax < x4) && (day > y3) && (day < y4))
                    {
                        dabuffer = (char *)ExtGetWallCaption(i);
                        if (dabuffer[0] != 0)
                        {
                            screencoords(&dax,&day, dax-pos.x,day-pos.y, zoom);
                            if (m32_sideview)
                                day += getscreenvdisp(getflorzofslope(sectorofwall(i), dax,day)-pos.z, zoom);

                            drawsmalllabel(dabuffer, editorcolors[0], editorcolors[31], dax, day);
                        }
                    }
                }

                i = 0; j = numsprites; k=0;
                if (zoom >= 768)
                    while (j > 0 && i < MAXSPRITES && (!m32_sideview || k<m32_swcnt))
                    {
                        if (m32_sideview)
                        {
                            i = m32_wallsprite[k++];
                            if (i<MAXWALLS)
                                continue;
                            i = i-MAXWALLS;
                        }

                        if (sprite[i].statnum < MAXSTATUS)
                        {
                            dabuffer = (char *)ExtGetSpriteCaption(i);
                            if (dabuffer[0] != 0)
                            {
                                //Get average point of sprite
                                screencoords(&dax,&day, sprite[i].x-pos.x,sprite[i].y-pos.y, zoom);
                                if (m32_sideview)
                                    day += getscreenvdisp(sprite[i].z-pos.z, zoom);

                                {
                                    int32_t blocking = (sprite[i].cstat&1);

                                    col = 3 + 2*blocking;
                                    if (spritecol2d[sprite[i].picnum][blocking])
                                        col = spritecol2d[sprite[i].picnum][blocking];

                                    if ((i == pointhighlight-16384) && (totalclock & 32))
                                        col += (2<<2);

                                    drawsmalllabel(dabuffer, editorcolors[0], editorcolors[col], dax, day);
                                }
                            }
                            j--;
                        }
                        i++;
                    }
            }

            printcoords16(pos.x,pos.y,ang);

            numwalls = tempint;

            if (highlightsectorcnt >= 0)
                for (i=0; i<numsectors; i++)
                    if (hlsectorbitmap[i>>3]&(1<<(i&7)))
                        fillsector(i,2);

            if (keystatus[0x2a]) // FIXME
            {
                drawlinepat = 0x00ff00ff;
                drawline16(searchx,0, searchx,ydim2d-1, editorcolors[15]);
                drawline16(0,searchy, xdim2d-1,searchy, editorcolors[15]);
                drawlinepat = 0xffffffff;

                _printmessage16("(%d,%d)",mousxplc,mousyplc);
#if 0
                i = (Bstrlen(tempbuf)<<3)+6;
                if ((searchx+i) < (xdim2d-1))
                    i = 0;
                else i = (searchx+i)-(xdim2d-1);
                if ((searchy+16) < (ydim2d-STATUS2DSIZ2-1))
                    j = 0;
                else j = (searchy+16)-(ydim2d-STATUS2DSIZ2-1);
                printext16(searchx+6-i,searchy+6-j,editorcolors[11],-1,tempbuf,0);
#endif
            }
            drawline16(searchx,0, searchx,8, editorcolors[15]);
            drawline16(0,searchy, 8,searchy, editorcolors[15]);

            ////// draw mouse pointer
            col = editorcolors[15 - 3*gridlock];
            if (joinsector[0] >= 0)
                col = editorcolors[11];

            if (numcorruptthings>0)
            {
                static char cbuf[64];

                if ((pointhighlight&16384)==0)
                {
                    for (i=0; i<numcorruptthings; i++)
                        if ((corruptthings[i]&CORRUPT_MASK)==CORRUPT_WALL &&
                            (corruptthings[i]&(MAXWALLS-1))==pointhighlight)
                        {
                            col = editorcolors[13];
                            printext16(searchx+6,searchy-6-8,editorcolors[13],editorcolors[0],"corrupt wall",0);
                            break;
                        }
                }

                Bsprintf(cbuf, "Map corrupt (level %d): %s%d errors", corruptlevel,
                         numcorruptthings>=MAXCORRUPTTHINGS ? ">=":"", numcorruptthings);
                printext16(8,8, editorcolors[13],editorcolors[0],cbuf,0);
            }

            if (keystatus[0x36] || keystatus[0xb8])  // RSHIFT || RALT
            {
                if (keystatus[0x27] || keystatus[0x28])  // ' and ;
                {
                    col = editorcolors[14];

                    drawline16base(searchx+16, searchy-16, -4,0, +4,0, col);
                    if (keystatus[0x28])
                        drawline16base(searchx+16, searchy-16, 0,-4, 0,+4, col);
                }

                if (keystatus[0x36] && eitherCTRL)
                    printext16(searchx+6,searchy-6-8,editorcolors[12],-1,"S",0);
            }

            drawline16base(searchx,searchy, +0,-8, +0,-1, col);
            drawline16base(searchx,searchy, +1,-8, +1,-1, col);
            drawline16base(searchx,searchy, +0,+2, +0,+9, col);
            drawline16base(searchx,searchy, +1,+2, +1,+9, col);
            drawline16base(searchx,searchy, -8,+0, -1,+0, col);
            drawline16base(searchx,searchy, -8,+1, -1,+1, col);
            drawline16base(searchx,searchy, +2,+0, +9,+0, col);
            drawline16base(searchx,searchy, +2,+1, +9,+1, col);

            ////// Draw the white pixel closest to mouse cursor on linehighlight
            if (linehighlight>=0 && !m32_sideview)
            {
                getclosestpointonwall(mousxplc,mousyplc,(int32_t)linehighlight,&dax,&day);
                x2 = mulscale14(dax-pos.x,zoom);
                y2 = mulscale14(day-pos.y,zoom);

                drawline16base(halfxdim16+x2,midydim16+y2, 0,0, 0,0,
                               wall[linehighlight].nextsector >= 0 ? editorcolors[15] : editorcolors[5]);
            }

            enddrawing();	//}}}

            OSD_Draw();
        }

        VM_OnEvent(EVENT_PREKEYS2D, -1);
        ExtCheckKeys(); // TX 20050101, it makes more sense to have this here so keys can be overwritten with new functions in bstub.c

        // Flip/mirror sector Ed Coolidge
        if (keystatus[0x2d] || keystatus[0x15])  // X or Y (2D)
        {
            int32_t about_x=keystatus[0x2d];
            int32_t doMirror = eitherALT;  // mirror walls and wall/floor sprites

            if (highlightsectorcnt > 0)
            {
                keystatus[0x2d] = keystatus[0x15] = 0;

                get_sectors_center(highlightsector, highlightsectorcnt, &dax, &day);

                if (gridlock && grid > 0)
                    locktogrid(&dax, &day);

                for (i=0; i<highlightsectorcnt; i++)
                {
                    int32_t startofloop, endofloop;
                    int32_t numtoswap = -1;
                    int32_t w=0;
                    walltype tempwall;

                    startofloop = startwall = sector[highlightsector[i]].wallptr;
                    endofloop = endwall = startwall+sector[highlightsector[i]].wallnum-1;
#if 0
                    if (doMirror)
                    {
                        //mirror sector textures
                        sector[highlightsector[i]].ceilingstat ^= 0x10;
                        sector[highlightsector[i]].floorstat ^= 0x10;
                    }
#endif
                    //save position of wall at start of loop
                    x3 = wall[startofloop].x;
                    y3 = wall[startofloop].y;

                    for (j=startwall; j<=endwall; j++)
                    {
                        //fix position of walls
                        if (about_x)
                        {
                            wall[j].x = dax-POINT2(j).x+dax; //flip wall.x about dax
                            wall[j].y = POINT2(j).y;
                        }
                        else
                        {
                            wall[j].x = POINT2(j).x;
                            wall[j].y = day-POINT2(j).y+day; //flip wall.y about day
                        }

                        if (doMirror)
                            wall[j].cstat ^= 8;  //mirror walls about dax/day

                        if (wall[j].point2==startofloop) //check if j is end of loop
                        {
                            endofloop = j;
                            if (about_x)
                            {
                                wall[endofloop].x = dax-x3+dax; //flip wall.x about dax
                                wall[endofloop].y = y3;
                            }
                            else
                            {
                                wall[endofloop].x = x3;
                                wall[endofloop].y = day-y3+day; //flip wall.y about dax
                            }

                            //correct order of walls in loop to maintain player space (right-hand rule)
                            numtoswap = (endofloop-startofloop)>>1;
                            for (w=1; w<=numtoswap; w++)
                            {
                                Bmemcpy(&tempwall, &wall[startofloop+w], sizeof(walltype));
                                Bmemcpy(&wall[startofloop+w], &wall[endofloop-w+1], sizeof(walltype));
                                Bmemcpy(&wall[endofloop-w+1], &tempwall, sizeof(walltype));
                            }

                            //make point2 point to next wall in loop
                            for (w=startofloop; w<endofloop; w++)
                                wall[w].point2 = w+1;
                            wall[endofloop].point2 = startofloop;

                            startofloop = endofloop+1; //set first wall of next loop
                            //save position of wall at start of loop
                            x3 = wall[startofloop].x;
                            y3 = wall[startofloop].y;
                        }
                    }

                    j = headspritesect[highlightsector[i]];
                    while (j != -1)
                    {
                        if (about_x)
                        {
                            x3 = sprite[j].x;
                            sprite[j].x = dax-x3+dax; //flip sprite.x about dax
                            sprite[j].ang = (1024+2048-sprite[j].ang)&2047; //flip ang about 512
                        }
                        else
                        {
                            y3 = sprite[j].y;
                            sprite[j].y = day-y3+day; //flip sprite.y about day
                            sprite[j].ang = (2048-sprite[j].ang)&2047; //flip ang about 512
                        }

                        if (doMirror && (sprite[j].cstat & 0x30))
                            sprite[j].cstat ^= 4;  // mirror sprites about dax/day (don't mirror monsters)

                        j = nextspritesect[j];
                    }
                }

                printmessage16("Selected sector(s) flipped");
                asksave = 1;
            }
        }
        // end edit for sector flip

        if (keystatus[88])   //F12
        {
            keystatus[88] = 0;
//__clearscreen_beforecapture__
            screencapture("captxxxx.tga", eitherSHIFT);

            showframe(1);
        }
        if (keystatus[0x30])  // B (clip Blocking xor) (2D)
        {
            pointhighlight = getpointhighlight(mousxplc, mousyplc, pointhighlight);
            linehighlight = getlinehighlight(mousxplc, mousyplc, linehighlight);

            if ((pointhighlight&0xc000) == 16384)
            {
                sprite[pointhighlight&16383].cstat ^= 1;
                sprite[pointhighlight&16383].cstat &= ~256;
                sprite[pointhighlight&16383].cstat |= ((sprite[pointhighlight&16383].cstat&1)<<8);
                asksave = 1;
            }
            else if (linehighlight >= 0)
            {
                wall[linehighlight].cstat ^= 1;
                wall[linehighlight].cstat &= ~64;
                if ((wall[linehighlight].nextwall >= 0) && !eitherSHIFT)
                {
                    NEXTWALL(linehighlight).cstat &= ~(1+64);
                    NEXTWALL(linehighlight).cstat |= (wall[linehighlight].cstat&1);
                }
                asksave = 1;
            }
            keystatus[0x30] = 0;
        }
        if (keystatus[0x21])  //F (F alone does nothing in 2D right now)
        {
            keystatus[0x21] = 0;
            if (eitherALT)  //ALT-F (relative alignmment flip)
            {
                linehighlight = getlinehighlight(mousxplc, mousyplc, linehighlight);
                if (linehighlight >= 0)
                {
                    int32_t secti = sectorofwall(linehighlight);
                    setfirstwall(secti, linehighlight);
                    asksave = 1;
                    printmessage16("This wall now sector %d's first wall (sector[].wallptr)", secti);
                }
            }
        }

        if (keystatus[0x18])  // O (ornament onto wall) (2D)
        {
            keystatus[0x18] = 0;
            if ((pointhighlight&0xc000) == 16384)
            {
                asksave = 1;
                DoSpriteOrnament(pointhighlight&16383);
            }
        }


        tsign = 0;
        if (keystatus[0x33] || (bstatus&33)==33)  // , (2D)
            tsign = +1;
        if (keystatus[0x34] || (bstatus&17)==17)  // . (2D)
            tsign = -1;

        if (tsign)
        {
            if (highlightsectorcnt > 0)
            {
                int32_t smoothRotation = !eitherSHIFT;

                get_sectors_center(highlightsector, highlightsectorcnt, &dax, &day);

                if (smoothRotation)
                {
                    if (gridlock && grid > 0)
                        locktogrid(&dax, &day);
                }

                for (i=0; i<highlightsectorcnt; i++)
                {
                    for (WALLS_OF_SECTOR(highlightsector[i], j))
                    {
                        if (smoothRotation)
                        {
                            x3 = wall[j].x;
                            y3 = wall[j].y;
                            wall[j].x = dax + tsign*(day-y3);
                            wall[j].y = day + tsign*(x3-dax);
                        }
                        else
                            rotatepoint(dax,day, wall[j].x,wall[j].y, tsign&2047, &wall[j].x,&wall[j].y);
                    }

                    for (j=headspritesect[highlightsector[i]]; j != -1; j=nextspritesect[j])
                    {
                        if (smoothRotation)
                        {
                            x3 = sprite[j].x;
                            y3 = sprite[j].y;
                            sprite[j].x = dax + tsign*(day-y3);
                            sprite[j].y = day + tsign*(x3-dax);
                            sprite[j].ang = (sprite[j].ang+tsign*512)&2047;
                        }
                        else
                        {
                            rotatepoint(dax,day, sprite[j].x,sprite[j].y, tsign&2047, &sprite[j].x,&sprite[j].y);
                            sprite[j].ang = (sprite[j].ang+tsign)&2047;
                        }
                    }
                }

                if (smoothRotation)
                    keystatus[0x33] = keystatus[0x34] = 0;

                mouseb &= ~(16|32);
                bstatus &= ~(16|32);
                asksave = 1;
            }
            else
            {
                if (pointhighlight >= 16384)
                {
                    i = pointhighlight-16384;
                    if (eitherSHIFT)
                        sprite[i].ang = (sprite[i].ang-tsign)&2047;
                    else
                    {
                        sprite[i].ang = (sprite[i].ang-128*tsign)&2047;
                        keystatus[0x33] = keystatus[0x34] = 0;
                    }

                    mouseb &= ~(16|32);
                    bstatus &= ~(16|32);

//                    clearmidstatbar16();
//                    showspritedata((int16_t)pointhighlight-16384);
                }
            }
        }

        if (keystatus[0x46])  //Scroll lock (set starting position)
        {
            startposx = pos.x;
            startposy = pos.y;
            startposz = pos.z;
            startang = ang;
            startsectnum = cursectnum;
            keystatus[0x46] = 0;
            asksave = 1;
        }

        if (keystatus[0x3f])  //F5
        {
//            keystatus[0x3f] = 0;

            ydim16 = STATUS2DSIZ;
            ExtShowSectorData((int16_t)0);
            ydim16 = ydim-STATUS2DSIZ2;
        }
        if (keystatus[0x40])  //F6
        {
//            keystatus[0x40] = 0;

            if (pointhighlight >= 16384)
            {
                i = pointhighlight-16384;

                ydim16 = STATUS2DSIZ;
                ExtShowSpriteData((int16_t)i);
                ydim16 = ydim-STATUS2DSIZ2;
            }
            else if (linehighlight >= 0)
            {
                i = linehighlight;

                ydim16 = STATUS2DSIZ;
                ExtShowWallData((int16_t)i);
                ydim16 = ydim-STATUS2DSIZ2;
            }
            else
            {
                ydim16 = STATUS2DSIZ;
                ExtShowWallData((int16_t)0);
                ydim16 = ydim-STATUS2DSIZ2;
            }
        }
        if (keystatus[0x41])  //F7
        {
            keystatus[0x41] = 0;

            for (i=0; i<numsectors; i++)
                if (inside_editor(&pos, searchx,searchy, zoom, mousxplc,mousyplc,i) == 1)
                {
                    ydim16 = STATUS2DSIZ;
                    ExtEditSectorData((int16_t)i);
                    ydim16 = ydim-STATUS2DSIZ2;
                    break;
                }
        }
        if (keystatus[0x42])  //F8
        {
            keystatus[0x42] = 0;

            if (pointhighlight >= 16384)
            {
                i = pointhighlight-16384;

                ydim16 = STATUS2DSIZ;
                ExtEditSpriteData((int16_t)i);
                ydim16 = ydim-STATUS2DSIZ2;
            }
            else if (linehighlight >= 0)
            {
                i = linehighlight;

                ydim16 = STATUS2DSIZ;
                ExtEditWallData((int16_t)i);
                ydim16 = ydim-STATUS2DSIZ2;
            }
        }

        if (keystatus[0x23])  //H (Hi 16 bits of tag)
        {
            keystatus[0x23] = 0;
            if (eitherCTRL)  //Ctrl-H
            {
                pointhighlight = getpointhighlight(mousxplc, mousyplc, pointhighlight);
                linehighlight = getlinehighlight(mousxplc, mousyplc, linehighlight);

                if ((pointhighlight&0xc000) == 16384)
                {
                    sprite[pointhighlight&16383].cstat ^= 256;
                    asksave = 1;
                }
                else if (linehighlight >= 0)
                {
                    wall[linehighlight].cstat ^= 64;
                    if ((wall[linehighlight].nextwall >= 0) && !eitherSHIFT)
                    {
                        NEXTWALL(linehighlight).cstat &= ~64;
                        NEXTWALL(linehighlight).cstat |= (wall[linehighlight].cstat&64);
                    }
                    asksave = 1;
                }
            }
            else if (eitherALT)  //ALT
            {
                if (pointhighlight >= 16384)
                {
                    i = pointhighlight-16384;
                    Bsprintf(buffer, "Sprite (%d) Hi-tag: ", i);
                    sprite[i].hitag = getnumber16(buffer, sprite[i].hitag, BTAG_MAX, 0);
                }
                else if (linehighlight >= 0)
                {
                    i = linehighlight;
                    Bsprintf(buffer, "Wall (%d) Hi-tag: ", i);
                    wall[i].hitag = getnumber16(buffer, wall[i].hitag, BTAG_MAX, 0);
                }
            }
            else
            {
                for (i=0; i<numsectors; i++)
                    if (inside_editor(&pos, searchx,searchy, zoom, mousxplc,mousyplc,i) == 1)
                    {
                        Bsprintf(buffer, "Sector (%d) Hi-tag: ", i);
                        sector[i].hitag = getnumber16(buffer, sector[i].hitag, BTAG_MAX, 0);
                        break;
                    }
            }
            // printmessage16("");
        }
        if (keystatus[0x19])  // P (palookup #)
        {
            keystatus[0x19] = 0;

            for (i=0; i<numsectors; i++)
                if (inside_editor(&pos, searchx,searchy, zoom, mousxplc,mousyplc,i) == 1)
                {
                    Bsprintf(buffer, "Sector (%d) Ceilingpal: ", i);
                    sector[i].ceilingpal = getnumber16(buffer, sector[i].ceilingpal, MAXPALOOKUPS-1, 0);

                    Bsprintf(buffer, "Sector (%d) Floorpal: ", i);
                    sector[i].floorpal = getnumber16(buffer, sector[i].floorpal, MAXPALOOKUPS-1, 0);
                    break;
                }
        }
        if (keystatus[0x12])  // E (status list)
        {
            keystatus[0x12] = 0;

            if (!eitherCTRL)
            {
                if (pointhighlight >= 16384)
                {
                    i = pointhighlight-16384;
                    Bsprintf(buffer, "Sprite (%d) Status list: ", i);
                    changespritestat(i, getnumber16(buffer, sprite[i].statnum, MAXSTATUS-1, 0));
//                clearmidstatbar16();
//                showspritedata((int16_t)i);
                    // printmessage16("");
                }
            }
            else if (highlightsectorcnt > 0 && newnumwalls < 0&&0)
            {
                ////////// YAX //////////
                static const char *cfs[2] = {"ceiling", "floor"};

                int32_t cf, thez;

                cf = ask_above_or_below();
                if (cf==-1)
                    goto end_yax;

                thez = YAX_SECTORFLD(highlightsector[0],z, cf);
                for (i=0; i<highlightsectorcnt; i++)
                {
                    if (yax_getbunch(highlightsector[i], cf) >= 0)
                    {
                        message("Sector %d's %s is already extended", highlightsector[i], cfs[cf]);
                        goto end_yax;                        
                    }

                    if (i==0)
                        continue;

                    if (YAX_SECTORFLD(highlightsector[i],z, cf) != thez)
                    {
                        message("All sectors must have the same %s height", cfs[cf]);
                        goto end_yax;
                    }
                    if ((YAX_SECTORFLD(highlightsector[i],stat, cf)&2)!=0)
                    {
                        message("Sector %ss must not be sloped", cfs[cf]);
                        goto end_yax;
                    }
                }

                m = numwalls;
                Bmemset(visited, 0, sizeof(visited));
                // construct!
                for (i=0; i<highlightsectorcnt; i++)
                    for (WALLS_OF_SECTOR(highlightsector[i], j))
                    {
                        k = trace_loop(j, visited, NULL, NULL);
                        if (k == 0)
                            continue;
                        else if (k < 0)
                        {
                            numwalls = m;
                            goto end_yax;
                        }
//message("loop");
                        wall[k-1].point2 = numwalls;
                        numwalls = k;
                    }

                i = highlightsector[0];
                Bmemcpy(&sector[numsectors], &sector[i], sizeof(sectortype));
                sector[numsectors].wallptr = m;
                sector[numsectors].wallnum = numwalls-m;

                if (YAX_SECTORFLD(i,stat, cf)&2)
                {
                    YAX_SECTORFLD(numsectors,stat, !cf) |= 2;
                    YAX_SECTORFLD(numsectors,heinum, !cf) = YAX_SECTORFLD(i,heinum, cf);
                }

                YAX_SECTORFLD(numsectors,z, !cf) = YAX_SECTORFLD(i,z, cf);
                YAX_SECTORFLD(numsectors,z, cf) = YAX_SECTORFLD(i,z, cf) - (1-2*cf)*16384;

                k = -1;  // nextbunchnum
                for (i=0; i<numsectors; i++)
                {
                    j = yax_getbunch(i,YAX_CEILING);
                    k = max(k, j);
                    j = yax_getbunch(i,YAX_FLOOR);
                    k = max(k, j);
                }
                k++;

                newnumwalls = numwalls;
                numwalls = m;

                // restore red walls of the selected sectors
                for (i=0; i<highlightsectorcnt; i++)
                {
                    for (WALLS_OF_SECTOR(highlightsector[i], j))
                        if (wall[j].nextwall < 0)
                            checksectorpointer(j, highlightsector[i]);
                }

                // link
                yax_setbunch(numsectors, !cf, k);
                for (i=0; i<highlightsectorcnt; i++)
                    yax_setbunch(highlightsector[i], cf, k);

                numwalls = newnumwalls;
                newnumwalls = -1;

                numsectors++;

                Bmemset(hlsectorbitmap, 0, sizeof(hlsectorbitmap));
                update_highlightsector();
end_yax: ;
            }
        }

        if (highlightsectorcnt < 0)
        {
            if (keystatus[0x36])  //Right shift (point highlighting)
            {
                if (highlightcnt == 0)
                {
                    int32_t xx[] = { highlightx1, highlightx1, searchx, searchx, highlightx1 };
                    int32_t yy[] = { highlighty1, searchy, searchy, highlighty1, highlighty1 };

                    highlightx2 = searchx;
                    highlighty2 = searchy;
                    ydim16 = ydim-STATUS2DSIZ2;

                    plotlines2d(xx, yy, 5, editorcolors[5]);
                }
                else
                {
                    highlightcnt = 0;

                    highlightx1 = searchx;
                    highlighty1 = searchy;
                    highlightx2 = searchx;
                    highlighty2 = searchy;
                }
            }
            else
            {
                if (highlightcnt == 0)
                {
                    if (!m32_sideview)
                    {
                        getpoint(highlightx1,highlighty1, &highlightx1,&highlighty1);
                        getpoint(highlightx2,highlighty2, &highlightx2,&highlighty2);
                    }

                    if (highlightx1 > highlightx2)
                        swaplong(&highlightx1, &highlightx2);
                    if (highlighty1 > highlighty2)
                        swaplong(&highlighty1, &highlighty2);

                    // Ctrl+RShift: select all wall-points of highlighted wall's loop:
                    if (eitherCTRL && highlightx1==highlightx2 && highlighty1==highlighty2)
                    {
                        Bmemset(show2dwall, 0, sizeof(show2dwall));
                        Bmemset(show2dsprite, 0, sizeof(show2dsprite));

                        if (linehighlight >= 0 && linehighlight < MAXWALLS)
                        {
                            i = linehighlight;
                            do
                            {
                                show2dwall[i>>3] |= (1<<(i&7));

                                for (j=0; j<numwalls; j++)
                                    if (j!=i && wall[j].x==wall[i].x && wall[j].y==wall[i].y)
                                        show2dwall[j>>3] |= (1<<(j&7));

                                i = wall[i].point2;
                            }
                            while (i != linehighlight);
                        }

                        update_highlight();
                    }
                    else
                    {
                        int32_t add=keystatus[0x28], sub=(!add && keystatus[0x27]), setop=(add||sub);
                        int32_t tx, ty, onlySprites=eitherCTRL;

                        if (!setop)
                        {
                            Bmemset(show2dwall, 0, sizeof(show2dwall));
                            Bmemset(show2dsprite, 0, sizeof(show2dsprite));
                        }

                        for (i=0; i<numwalls; i++)
                        {
                            if (onlySprites)
                                break;

                            if (!m32_sideview)
                            {
                                tx = wall[i].x;
                                ty = wall[i].y;
                                wall[i].cstat &= ~(1<<14);
                            }
                            else
                            {
                                screencoords(&tx,&ty, wall[i].x-pos.x,wall[i].y-pos.y, zoom);
                                ty += getscreenvdisp(
                                    getflorzofslope(sectorofwall(i), wall[i].x,wall[i].y)-pos.z, zoom);
                                tx += halfxdim16;
                                ty += midydim16;
                            }

                            if (tx >= highlightx1 && tx <= highlightx2 &&
                                    ty >= highlighty1 && ty <= highlighty2)
                            {
                                if (!sub)
                                    show2dwall[i>>3] |= (1<<(i&7));
                                else if (sub)
                                    show2dwall[i>>3] &= ~(1<<(i&7));
                            }
                        }

                        if (m32_sideview && !onlySprites)
                        {
                            // also select walls that would be dragged but
                            // maybe were missed
#if 1
                            for (i=0; i<numwalls; i++)
                                if (show2dwall[i>>3]&(1<<(i&7)))
                                {
                                    //N^2...ugh
                                    for (j=0; j<numwalls; j++)
                                        if ((int64_t *)&wall[i] == (int64_t *)&wall[j])
                                            show2dwall[j>>3] |= (1<<(j&7));
                                }
#else
                            for (i=0; i<numwalls; i++)
                                if (show2dwall[i>>3]&(1<<(i&7)))
                                    dragpoint(i, wall[i].x, wall[i].y);
                            for (i=0; i<numwalls; i++)
                                if (wall[i].cstat & (1<<14))
                                {
                                    show2dwall[i>>3] |= (1<<(i&7));
                                    wall[i].cstat &= ~(1<<14);
                                }
#endif
                        }

                        for (i=0; i<MAXSPRITES; i++)
                        {
                            if (sprite[i].statnum == MAXSTATUS)
                                continue;

                            if (!m32_sideview)
                            {
                                tx = sprite[i].x;
                                ty = sprite[i].y;
                            }
                            else
                            {
                                screencoords(&tx,&ty, sprite[i].x-pos.x,sprite[i].y-pos.y, zoom);
                                ty += getscreenvdisp(sprite[i].z-pos.z, zoom);
                                tx += halfxdim16;
                                ty += midydim16;
                            }

                            if (tx >= highlightx1 && tx <= highlightx2 &&
                                    ty >= highlighty1 && ty <= highlighty2)
                            {
                                if (!sub)
                                {
                                    if (sprite[i].sectnum >= 0)  // don't allow to select sprites in null space
                                        show2dsprite[i>>3] |= (1<<(i&7));
                                }
                                else
                                    show2dsprite[i>>3] &= ~(1<<(i&7));
                            }
                        }

                        update_highlight();
                    }
                }
            }
        }

        if (highlightcnt < 0)
        {
            if (keystatus[0xb8])  //Right alt (sector highlighting)
            {
                if (highlightsectorcnt == 0)
                {
                    if (!eitherCTRL)
                    {
                        int32_t xx[] = { highlightx1, highlightx1, searchx, searchx, highlightx1 };
                        int32_t yy[] = { highlighty1, searchy, searchy, highlighty1, highlighty1 };

                        highlightx2 = searchx;
                        highlighty2 = searchy;
                        ydim16 = ydim-STATUS2DSIZ2;

                        plotlines2d(xx, yy, 5, editorcolors[10]);
                    }
                }
                else
                {
                    int32_t didmakered = (highlightsectorcnt<0), hadouterpoint=0;
                    int16_t tmprefsect;

                    for (i=0; i<highlightsectorcnt; i++)
                    {
                        for (WALLS_OF_SECTOR(highlightsector[i], j))
                        {
                            if (wall[j].nextwall >= 0)
                                checksectorpointer(wall[j].nextwall,wall[j].nextsector);
                            didmakered |= !!checksectorpointer(j, highlightsector[i]);

                            if (!didmakered)
                            {
                                updatesectorexclude(wall[j].x, wall[j].y, &tmprefsect, hlsectorbitmap);
                                if (tmprefsect<0)
                                    hadouterpoint = 1;
                            }
                        }
                    }

                    if (!didmakered && !hadouterpoint && newnumwalls<0)
                    {
                        // fade the screen to have the user's attention
                        fade_screen();

                        didmakered |= !ask_if_sure("Insert outer loop and make red walls?", 0);
                        clearkeys();
                    }

                    if (!didmakered && !hadouterpoint && newnumwalls<0)
                    {
                        int16_t ignore, refsect;
                        int32_t n;

                        Bmemset(visited, 0, sizeof(visited));

                        for (i=0; i<highlightsectorcnt; i++)
                        {
                            for (WALLS_OF_SECTOR(highlightsector[i], j))
                            {
                                k = trace_loop(j, visited, &ignore, &refsect);
                                if (k == 0)
                                    continue;
                                else if (k < 0)
                                {
                                    i = highlightsectorcnt;
                                    break;  // outer loop too
                                }

                                if (!ignore)
                                {
                                    wall[k-1].point2 = numwalls;  // close the loop
                                    newnumwalls = k;
                                    n = (newnumwalls-numwalls);  // number of walls in just constructed loop

                                    if (clockdir(numwalls)==0)
                                    {
                                        int16_t begwalltomove = sector[refsect].wallptr+sector[refsect].wallnum;

                                        flipwalls(numwalls, newnumwalls);

                                        sector[refsect].wallnum += n;
                                        if (refsect != numsectors-1)
                                        {
                                            walltype *tmpwall = Bmalloc(n * sizeof(walltype));

                                            if (!tmpwall)
                                            {
                                                message("out of memory!");
                                                ExtUnInit();
                                                uninitsystem();
                                                exit(1);
                                            }

                                            for (m=0; m<numwalls; m++)
                                            {
                                                if (wall[m].nextwall >= begwalltomove)
                                                    wall[m].nextwall += n;
                                            }
                                            for (m=refsect+1; m<numsectors; m++)
                                                sector[m].wallptr += n;
                                            for (m=begwalltomove; m<numwalls; m++)
                                                wall[m].point2 += n;
                                            for (m=numwalls; m<newnumwalls; m++)
                                                wall[m].point2 += (begwalltomove-numwalls);

                                            Bmemcpy(tmpwall, &wall[numwalls], n*sizeof(walltype));
                                            Bmemmove(&wall[begwalltomove+n], &wall[begwalltomove], (numwalls-begwalltomove)*sizeof(walltype));
                                            Bmemcpy(&wall[begwalltomove], tmpwall, n*sizeof(walltype));

                                            Bfree(tmpwall);
                                        }
                                        numwalls = newnumwalls;
                                        newnumwalls = -1;

                                        for (m=begwalltomove; m<begwalltomove+n; m++)
                                            checksectorpointer(m, refsect);

                                        message("Attached new inner loop to sector %d", refsect);
                                    }
                                }
                            }
                        }

                        newnumwalls = -1;
                    }

                    highlightx1 = searchx;
                    highlighty1 = searchy;
                    highlightx2 = searchx;
                    highlighty2 = searchy;
                    highlightsectorcnt = 0;
                }
            }
            else
            {
                if (highlightsectorcnt == 0)
                {
                    int32_t add=keystatus[0x28], sub=(!add && keystatus[0x27]), setop=(add||sub);
                    int32_t tx,ty, pointsel = eitherCTRL;

                    if (!m32_sideview)
                    {
                        getpoint(highlightx1,highlighty1, &highlightx1,&highlighty1);
                        getpoint(highlightx2,highlighty2, &highlightx2,&highlighty2);
                    }

                    if (!pointsel)
                    {
                        if (highlightx1 > highlightx2)
                            swaplong(&highlightx1, &highlightx2);
                        if (highlighty1 > highlighty2)
                            swaplong(&highlighty1, &highlighty2);
                    }

                    if (!setop)
                        Bmemset(hlsectorbitmap, 0, sizeof(hlsectorbitmap));

                    for (i=0; i<numsectors; i++)
                    {
                        if (pointsel)
                        {
                            bad = (inside_editor(&pos, searchx,searchy, zoom, highlightx2, highlighty2, i)!=1);
                        }
                        else
                        {
                            bad = 0;
                            for (WALLS_OF_SECTOR(i, j))
                            {
                                if (!m32_sideview)
                                {
                                    tx = wall[j].x;
                                    ty = wall[j].y;
                                }
                                else
                                {
                                    screencoords(&tx,&ty, wall[j].x-pos.x,wall[j].y-pos.y, zoom);
                                    ty += getscreenvdisp(getflorzofslope(i, wall[j].x,wall[j].y)-pos.z, zoom);
                                    tx += halfxdim16;
                                    ty += midydim16;
                                }

                                if (tx < highlightx1 || tx > highlightx2) bad = 1;
                                if (ty < highlighty1 || ty > highlighty2) bad = 1;
                                if (bad == 1) break;
                            }
                        }

                        if (bad == 0)
                        {
                            if (sub)
                            {
                                hlsectorbitmap[i>>3] &= ~(1<<(i&7));
                                for (j=sector[i].wallptr; j<sector[i].wallptr+sector[i].wallnum; j++)
                                {
                                    if (wall[j].nextwall >= 0)
                                        checksectorpointer(wall[j].nextwall,wall[j].nextsector);
                                    checksectorpointer(j, i);
                                }
                            }
                            else
                                hlsectorbitmap[i>>3] |= (1<<(i&7));
                        }
                    }

                    update_highlightsector();
                    ovh_whiteoutgrab();

//                    if (highlightsectorcnt>0)
//                        printmessage16("Total selected sectors: %d", highlightsectorcnt);
                }
            }
        }

        if (((bstatus&1) < (oldmousebstatus&1)) && highlightsectorcnt < 0)  //after dragging
        {
            int32_t wallsdrawn = newnumwalls-numwalls, runi;
            walltype *tmpwall = NULL;

            if (newnumwalls != -1)
            {
                tmpwall = Bmalloc(wallsdrawn * sizeof(walltype));
                if (!tmpwall)
                {
                    wallsdrawn = -1;
                    goto end_after_dragging;
                }

                newnumwalls = -1;
                Bmemcpy(tmpwall, &wall[numwalls], wallsdrawn*sizeof(walltype));
            }
            else wallsdrawn = -1;

            j = 1;
            if (highlightcnt > 0)
                for (i=0; i<highlightcnt; i++)
                    if (pointhighlight == highlight[i])
                    {
                        j = 0;
                        break;
                    }

            if (j == 0)
            {
                for (i=0; i<highlightcnt; i++)
                    if ((highlight[i]&0xc000) == 16384)
                        updatesprite1(highlight[i]&16383);
            }
            else if ((pointhighlight&0xc000) == 16384)
            {
                updatesprite1(pointhighlight&16383);
            }

            if ((pointhighlight&0xc000) == 0)
            {
                dax = wall[pointhighlight].x;
                day = wall[pointhighlight].y;
            }
            else if ((pointhighlight&0xc000) == 16384)
            {
                dax = sprite[pointhighlight&16383].x;
                day = sprite[pointhighlight&16383].y;
            }

            for (runi=0; runi<2; runi++)
                for (i=numwalls-1; i>=0; i--)  //delete points
                {
                    if (wall[i].x == POINT2(i).x && wall[i].y == POINT2(i).y
                            && sector[sectorofwall(i)].wallnum > 3
                            && sector[sectorofwall(wall[i].point2)].wallnum > 3)
                    {
                        int32_t b = (wall[i].nextwall == -1 ||
                                     (sector[sectorofwall(wall[i].nextwall)].wallnum > 3 &&
                                      sector[sectorofwall(NEXTWALL(i).point2)].wallnum > 3));
                        if (runi==0 && !b)
                        {
                            printmessage16("Invalid operation, delete or join sector instead.");
                            goto end_after_dragging;
                        }
                        else if (runi==1 && b)
                        {
                            deletepoint(i);
                            printmessage16("Point deleted.");
                            asksave = 1;
                        }
                    }
                }

            for (i=0; i<numwalls; i++)     //make new red lines?
            {
                if ((wall[i].x == dax && wall[i].y == day)
                        || (POINT2(i).x == dax && POINT2(i).y == day))
                {
                    checksectorpointer(i, sectorofwall(i));
//                    fixrepeats(i);
                    asksave = 1;
                }
            }

end_after_dragging:
            if (wallsdrawn != -1)
            {
                Bmemcpy(&wall[numwalls], tmpwall, wallsdrawn*sizeof(walltype));
                newnumwalls = numwalls + wallsdrawn;
                for (i=numwalls; i<newnumwalls; i++)
                    wall[i].point2 = i+1;

                Bfree(tmpwall);
            }
        }

        if ((bstatus&1) > 0)                //drag points
        {
            if (highlightsectorcnt > 0)
            {
                if ((bstatus&1) > (oldmousebstatus&1))
                {
                    newnumwalls = -1;
                    sectorhighlightstat = -1;

//                    updatesector(mousxplc,mousyplc,&cursectorhighlight);
                    cursectorhighlight = -1;
                    for (i=0; i<highlightsectorcnt; i++)
                        if (inside_editor(&pos, searchx,searchy, zoom, mousxplc, mousyplc, highlightsector[i])==1)
                        {
                            cursectorhighlight = highlightsector[i];
                            break;
                        }

                    if (cursectorhighlight >= 0 && cursectorhighlight < numsectors)
                    {
//                        for (i=0; i<highlightsectorcnt; i++)
//                            if (cursectorhighlight == highlightsector[i])
                        {
                            //You clicked inside one of the flashing sectors!
                            sectorhighlightstat = 1;

                            dax = mousxplc;
                            day = mousyplc;
                            if (gridlock && grid > 0)
                                locktogrid(&dax, &day);

                            sectorhighlightx = dax;
                            sectorhighlighty = day;
//                                break;
                        }
                    }
                }
                else if (sectorhighlightstat == 1)
                {
                    dax = mousxplc;
                    day = mousyplc;
                    if (gridlock && grid > 0)
                        locktogrid(&dax, &day);

                    dax -= sectorhighlightx;
                    day -= sectorhighlighty;
                    sectorhighlightx += dax;
                    sectorhighlighty += day;

                    for (i=0; i<highlightsectorcnt; i++)
                    {
                        for (WALLS_OF_SECTOR(highlightsector[i], j))
                            { wall[j].x += dax; wall[j].y += day; }

                        for (j=headspritesect[highlightsector[i]]; j>=0; j=nextspritesect[j])
                            { sprite[j].x += dax; sprite[j].y += day; }
                    }

                    //for(i=0;i<highlightsectorcnt;i++)
                    //{
                    //   startwall = sector[highlightsector[i]].wallptr;
                    //   endwall = startwall+sector[highlightsector[i]].wallnum-1;
                    //   for(j=startwall;j<=endwall;j++)
                    //   {
                    //      if (wall[j].nextwall >= 0)
                    //         checksectorpointer(wall[j].nextwall,wall[j].nextsector);
                    //     checksectorpointer((short)j,highlightsector[i]);
                    //   }
                    //}
                    asksave = 1;
                }

            }
            else  //if (highlightsectorcnt <= 0)
            {
                if ((bstatus&1) > (oldmousebstatus&1))
                    pointhighlight = getpointhighlight(mousxplc, mousyplc, pointhighlight);

                if (pointhighlight >= 0 && (!m32_sideview || m32_sideelev>=32))
                {
                    if (m32_sideview)
                    {
                        int32_t dz;
                        if (pointhighlight>=16384)
                            dz = sprite[pointhighlight&16383].z - pos.z;
                        else
                            dz = getflorzofslope(sectorofwall(pointhighlight),
                                                 wall[pointhighlight].x, wall[pointhighlight].y) - pos.z;
                        getinvdisplacement(&dax,&day, -dz);
                        dax += mousxplc;
                        day += mousyplc;
                    }
                    else
                    {
                        dax = mousxplc;
                        day = mousyplc;
                    }

                    if (gridlock && grid > 0)
                        locktogrid(&dax, &day);

                    j = 1;
                    if (highlightcnt > 0)
                        for (i=0; i<highlightcnt; i++)
                            if (pointhighlight == highlight[i])
                                { j = 0; break; }

                    if (j == 0)
                    {
                        if ((pointhighlight&0xc000) == 0)
                        {
                            dax -= wall[pointhighlight].x;
                            day -= wall[pointhighlight].y;
                        }
                        else
                        {
                            dax -= sprite[pointhighlight&16383].x;
                            day -= sprite[pointhighlight&16383].y;
                        }

                        for (i=0; i<highlightcnt; i++)
                        {
                            if ((highlight[i]&0xc000) == 0)
                            {
                                wall[highlight[i]].x += dax;
                                wall[highlight[i]].y += day;
                            }
                            else
                            {
                                sprite[highlight[i]&16383].x += dax;
                                sprite[highlight[i]&16383].y += day;
                            }
                        }
                    }
                    else
                    {
                        if ((pointhighlight&0xc000) == 0)
                        {
                            if (newnumwalls >= numwalls &&
                                    wall[pointhighlight].x==firstx && wall[pointhighlight].y==firsty)
                            {
                                printmessage16("Can't drag point where drawing started.");
                                goto end_point_dragging;
                            }

                            dragpoint(pointhighlight,dax,day);
                        }
                        else if ((pointhighlight&0xc000) == 16384)
                        {
                            int32_t daspr=pointhighlight&16383, osec=sprite[daspr].sectnum;
                            vec3_t vec, ovec;

                            Bmemcpy(&ovec, (vec3_t *)&sprite[daspr], sizeof(vec3_t));
                            vec.x = dax;
                            vec.y = day;
                            vec.z = sprite[daspr].z;
                            if (setsprite(daspr, &vec) == -1 && osec>=0)
                                Bmemcpy(&sprite[daspr], &ovec, sizeof(vec3_t));
#if 0
                            daz = spriteheight(daspr, NULL);

                            for (i=0; i<numsectors; i++)
                                if (inside(dax,day,i) == 1)
                                    if (sprite[daspr].z >= getceilzofslope(i,dax,day))
                                        if (sprite[daspr].z-daz <= getflorzofslope(i,dax,day))
                                        {
                                            sprite[daspr].x = dax;
                                            sprite[daspr].y = day;
                                            if (sprite[daspr].sectnum != i)
                                                changespritesect(daspr,(int16_t)i);
                                            break;
                                        }
#endif
                        }
                    }
                    asksave = 1;
                }
            }
        }
        else //if ((bstatus&1) == 0)
        {
            pointhighlight = getpointhighlight(mousxplc, mousyplc, pointhighlight);
            sectorhighlightstat = -1;
        }
end_point_dragging:

        if (bstatus&(2|4))  // change arrow position
        {
            if (eitherCTRL)
            {
                int16_t cursectornum;

                for (cursectornum=0; cursectornum<numsectors; cursectornum++)
                    if (inside_editor(&pos, searchx,searchy, zoom, mousxplc,mousyplc,cursectornum) == 1)
                        break;

                if (cursectornum < numsectors)
                {
                    if (pointhighlight >= 16384)
                    {
                        i = pointhighlight-16384;
                        ExtEditSpriteData((int16_t)i);
                    }
                    else if ((linehighlight >= 0) && (bstatus&1 || sectorofwall(linehighlight) == cursectornum))
                        ExtEditWallData((int16_t)linehighlight);
                    else if (cursectornum >= 0)
                        ExtEditSectorData((int16_t)cursectornum);
                }

                bstatus &= ~6;
            }
            else
            {
                if (m32_sideview && (bstatus&4))
                {
                    pos.z += divscale18(searchy-midydim16,zoom);
                    getpoint(searchx,midydim16, &pos.x, &pos.y);
                }
                else
                {
                    pos.x = mousxplc;
                    pos.y = mousyplc;
                }

                if (m32_sideview)
                {
                    int32_t opat=drawlinepat;

                    y1 = INT_MAX;

                    for (i=0; i<numsectors; i++)
                    {
                        if (inside(pos.x, pos.y, i)==1)
                        {
                            day = getscreenvdisp(getflorzofslope(i, pos.x, pos.y)-pos.z, zoom);

                            x2 = max(4, mulscale14(64,zoom));
                            y2 = scalescreeny(x2);

                            if (klabs(day) < y1)
                                y1 = day;

                            drawline16base(halfxdim16, midydim16+day, -x2,-y2, x2,y2, editorcolors[14]);
                            drawline16base(halfxdim16, midydim16+day, -x2,y2, x2,-y2, editorcolors[14]);
                        }
                    }

                    drawlinepat = 0x11111111;
                    if (y1 != INT_MAX)
                        drawline16base(halfxdim16,midydim16, 0,0, 0,y1, editorcolors[14]);
//                    else
//                        drawline16base(halfxdim16,midydim16, 0,0, 0,getscreenvdisp(-pos.z, zoom), editorcolors[14]);
                    drawlinepat = opat;
                }

                searchx = halfxdim16;
                searchy = midydim16;
            }
        }
        else if ((oldmousebstatus&6) > 0)
            updatesectorz(pos.x,pos.y,pos.z,&cursectnum);

        if (circlewall != -1 && (keystatus[0x4a] || ((bstatus&32) && !eitherCTRL)))  // -, mousewheel down
        {
            if (circlepoints > 1)
                circlepoints--;
            keystatus[0x4a] = 0;
            mouseb &= ~32;
            bstatus &= ~32;
        }
        if (circlewall != -1 && (keystatus[0x4e] || ((bstatus&16) && !eitherCTRL)))  // +, mousewheel up
        {
            if (circlepoints < 63)
                circlepoints++;
            keystatus[0x4e] = 0;
            mouseb &= ~16;
            bstatus &= ~16;
        }

        if (keystatus[0x3d])  // F3
        {
            keystatus[0x3d]=0;
            if (!m32_sideview && (newnumwalls>=0 || joinsector[0]>=0 || circlewall>=0 || (bstatus&1)))
                message("Must not be editing map while switching to side view mode.");
            else
            {
                m32_sideview = !m32_sideview;
                printmessage16("Side view %s", m32_sideview?"enabled":"disabled");

                m32_setkeyfilter(1);
            }
        }

        if (m32_sideview && (keystatus[0x10] || keystatus[0x11]))
        {
            if (eitherCTRL)
            {
                if (m32_sideang&63)
                {
                    m32_sideang += (1-2*keystatus[0x10])*(1-2*sideview_reversehrot)*32;
                    m32_sideang &= (2047&~63);
                }
                else
                {
                    m32_sideang += (1-2*keystatus[0x10])*(1-2*sideview_reversehrot)*64;
                    m32_sideang &= 2047;
                }

                keystatus[0x10] = keystatus[0x11] = 0;
            }
            else
            {
                m32_sideang += (1-2*keystatus[0x10])*(1-2*sideview_reversehrot)*synctics<<(eitherSHIFT*2);
                m32_sideang &= 2047;
            }
            _printmessage16("Sideview angle: %d", (int32_t)m32_sideang);
        }

        if (m32_sideview && (keystatus[0x2a] || (bstatus&(16|32))))  // LShift
        {
            if ((DOWN_BK(MOVEUP) || (bstatus&16)) && m32_sideelev < 512)
            {
                m32_sideelev += synctics<<(1+!!(bstatus&16));
                if (m32_sideelev > 512)
                    m32_sideelev = 512;
                _printmessage16("Sideview elevation: %d", m32_sideelev);
            }
            if ((DOWN_BK(MOVEDOWN) || (bstatus&32)) && m32_sideelev > 0)
            {
                m32_sideelev -= synctics<<(1+!!(bstatus&32));
                if (m32_sideelev < 0)
                    m32_sideelev = 0;
                _printmessage16("Sideview elevation: %d", m32_sideelev);
            }
        }
        else
        {
            int32_t didzoom=0;

            if ((DOWN_BK(MOVEUP) || (bstatus&16)) && zoom < 65536)
            {
                zoom += synctics*(zoom>>4);
                if (zoom < 24) zoom += 2;
                didzoom = 1;
            }
            if ((DOWN_BK(MOVEDOWN) || (bstatus&32)) && zoom > 8)
            {
                zoom -= synctics*(zoom>>4);
                didzoom = 1;
            }

            if (didzoom)
            {
                if (eitherALT)
                {
                    searchx = halfxdim16;
                    searchy = midydim16;
                    pos.x = mousxplc;
                    pos.y = mousyplc;
                }
                zoom = clamp(zoom, 8, 65536);
                _printmessage16("Zoom: %d",zoom);
            }
        }

        if (keystatus[0x22])  // G (grid on/off)
        {
            keystatus[0x22] = 0;
            grid++;
            if (grid == 7) grid = 0;
        }
        if (keystatus[0x26])  // L (grid lock)
        {
            keystatus[0x26] = 0;
            gridlock = !gridlock;
            printmessage16("Grid locking %s", gridlock?"on":"off");
        }

        if (keystatus[0x24])  // J (join sectors)
        {
            keystatus[0x24] = 0;

            if (newnumwalls >= 0)
            {
                printmessage16("Can't join sectors while editing.");
                goto end_join_sectors;
            }

            if (joinsector[0] < 0)
            {
                joinsector[0] = -1;
                for (i=0; i<numsectors; i++)
                    if (inside_editor(&pos, searchx,searchy, zoom, mousxplc,mousyplc,i) == 1)
                    {
                        joinsector[0] = i;
                        printmessage16("Join sector - press J again on sector to join with.");
                        break;
                    }
                goto end_join_sectors;
            }
            else
            {
                int16_t joink;

                joinsector[1] = -1;

                for (i=0; i<numsectors; i++)
                {
                    if (inside_editor(&pos, searchx,searchy, zoom, mousxplc,mousyplc,i) == 1)
                    {
                        startwall = sector[i].wallptr;
                        endwall = startwall + sector[i].wallnum;
                        for (j=startwall; j<endwall; j++)
                            if (wall[j].nextsector == joinsector[0])
                                break;

                        joinsector[1] = i;
                        // pressing J into the same sector is the same as saying 'no'
                        //                  v----------------v
                        if (j == endwall && i != joinsector[0])
                        {
                            fade_screen();

                            if (!ask_if_sure("Join non-adjacent sectors? (Y/N)", 0))
                                joinsector[1] = joinsector[0];
                        }

                        break;
                    }
                }

                if (joinsector[1] < 0 || joinsector[0] == joinsector[1])
                {
                    printmessage16("No sectors joined.");
                    joinsector[0] = -1;
                    goto end_join_sectors;
                }


                for (i=0; i<numwalls; i++)
                    wall[i].cstat &= ~(1<<14);

                newnumwalls = numwalls;

                for (k=0; k<2; k++)
                {
                    for (WALLS_OF_SECTOR(joinsector[k], j))
                    {
                        int32_t loopnum=MAXWALLS*2;

                        if (wall[j].cstat & (1<<14))
                            continue;

                        if (wall[j].nextsector == joinsector[1-k])
                        {
                            wall[j].cstat |= (1<<14);
                            continue;
                        }

                        i = j;
                        m = newnumwalls;
                        joink = k;
                        do
                        {
                            if (newnumwalls >= MAXWALLS + M32_FIXME_WALLS)
                            {
                                message("Joining sectors failed: not enough space beyond wall[]");
                                joinsector[0] = -1;
                                newnumwalls = -1;

                                for (i=0; i<numwalls; i++)
                                    wall[i].cstat &= ~(1<<14);

                                goto end_join_sectors;
                            }

                            Bmemcpy(&wall[newnumwalls], &wall[i], sizeof(walltype));
                            wall[newnumwalls].point2 = newnumwalls+1;
                            newnumwalls++;

                            wall[i].cstat |= (1<<14);

                            i = wall[i].point2;
                            if (wall[i].nextsector == joinsector[1-joink])
                            {
                                i = NEXTWALL(i).point2;
                                joink = 1 - joink;
                            }

                            loopnum--;
                        }
                        while (loopnum>0 && ((wall[i].cstat & (1<<14))==0)
                                   && (wall[i].nextsector != joinsector[1-joink]));

                        wall[newnumwalls-1].point2 = m;

                        if (loopnum==0)
                        {
                            printmessage16("internal error while joining sectors: infloop!");
                            newnumwalls = -1;
                        }
                    }
                }

                if (newnumwalls > numwalls)
                {
                    Bmemcpy(&sector[numsectors], &sector[joinsector[0]], sizeof(sectortype));
                    sector[numsectors].wallptr = numwalls;
                    sector[numsectors].wallnum = newnumwalls-numwalls;

                    //fix sprites
                    for (i=0; i<2; i++)
                    {
                        j = headspritesect[joinsector[i]];
                        while (j != -1)
                        {
                            k = nextspritesect[j];
                            changespritesect(j, numsectors);
                            j = k;
                        }
                    }

                    numsectors++;

                    for (i=numwalls; i<newnumwalls; i++)
                    {
                        if (wall[i].nextwall >= 0)
                        {
                            NEXTWALL(i).nextwall = i;
                            NEXTWALL(i).nextsector = numsectors-1;
                        }
                    }

                    numwalls = newnumwalls;
                    newnumwalls = -1;

                    for (k=0; k<2; k++)
                        for (WALLS_OF_SECTOR(joinsector[k], j))
                            wall[j].nextwall = wall[j].nextsector = -1;

                    deletesector(joinsector[0]);
                    if (joinsector[0] < joinsector[1])
                        joinsector[1]--;
                    deletesector(joinsector[1]);

                    printmessage16("Sectors joined.");
                    asksave = 1;
                }

                joinsector[0] = -1;
            }
        }
end_join_sectors:

// PK
        for (i=0x02; i<=0x0b; i++)  // keys '1' to '0' on the upper row
            if (keystatus[i])
            {
                prefixarg = prefixtiles[i-2];
                break;
            }

        if (eitherALT && keystatus[0x1f]) //ALT-S
        {
            keystatus[0x1f] = 0;

            if (linehighlight >= 0 && wall[linehighlight].nextwall == -1)
            {
                newnumwalls = whitelinescan(linehighlight);
                if (newnumwalls < numwalls)
                {
                    printmessage16("Can't make a sector out there.");
                    newnumwalls = -1;
                }
                else if (newnumwalls > MAXWALLS)
                {
                    printmessage16("Making new sector from inner loop would exceed wall limits.");
                    newnumwalls = -1;
                }
                else
                {
                    for (i=numwalls; i<newnumwalls; i++)
                    {
                        NEXTWALL(i).nextwall = i;
                        NEXTWALL(i).nextsector = numsectors;
                    }

                    numwalls = newnumwalls;
                    newnumwalls = -1;
                    numsectors++;

                    printmessage16("Inner loop made into new sector.");
                }
            }
        }
        else if (keystatus[0x1f])  //S
        {
            keystatus[0x1f] = 0;

            sucksect = -1;
            for (i=0; i<numsectors; i++)
                if (inside_editor(&pos, searchx,searchy, zoom, mousxplc,mousyplc,i) == 1)
                {
                    sucksect = i;
                    break;
                }

            if (sucksect >= 0)
            {
                dax = mousxplc;
                day = mousyplc;
                if (gridlock && grid > 0)
                    locktogrid(&dax, &day);

                i = insert_sprite_common(sucksect, dax, day);

                if (i < 0)
                    printmessage16("Couldn't insert sprite.");
                else
                {
                    sprite[i].z = getflorzofslope(sucksect,dax,day);
//                    if (sprite[i].cstat&128)
//                        sprite[i].z -= spriteheight(i, NULL)>>1;

// PK
                    if (prefixarg)
                    {
                        sprite[i].picnum = prefixarg;
                        sprite[i].xrepeat = sprite[i].yrepeat = 48;
                        prefixarg = 0;
                    }
                    else if (somethingintab == 3)
                    {
                        sprite[i].picnum = temppicnum;
                        if (tilesizx[temppicnum] <= 0 || tilesizy[temppicnum] <= 0)
                        {
                            j = 0;
                            for (k=0; k<MAXTILES; k++)
                                if (tilesizx[k] > 0 && tilesizy[k] > 0)
                                {
                                    j = k;
                                    break;
                                }
                            sprite[i].picnum = j;
                        }
                        sprite[i].shade = tempshade;
                        sprite[i].pal = temppal;
                        sprite[i].xrepeat = max(tempxrepeat, 1);
                        sprite[i].yrepeat = max(tempyrepeat, 1);
                        sprite[i].cstat = tempcstat;
                    }

                    if (tilesizy[sprite[i].picnum] >= 32)
                        sprite[i].cstat |= 1;

                    printmessage16("Sprite inserted.");
                    updatenumsprites();
                    asksave = 1;

                    VM_OnEvent(EVENT_INSERTSPRITE2D, i);
                }
            }
        }

        if (keystatus[0x2e])  // C (make circle of points)
        {
            if (highlightsectorcnt >= 0)
                duplicate_selected_sectors();
            else if (highlightcnt >= 0)
                duplicate_selected_sprites();

            else if (circlewall >= 0)
            {
                circlewall = -1;
            }
            else
            {
                if (linehighlight >= 0)
                    circlewall = linehighlight;
            }
            keystatus[0x2e] = 0;
        }

        bad = keystatus[0x39];  //Gotta do this to save lots of 3 spaces!

        if (circlewall >= 0)
        {
            x1 = wall[circlewall].x;
            y1 = wall[circlewall].y;
            x2 = POINT2(circlewall).x;
            y2 = POINT2(circlewall).y;
            x3 = mousxplc;
            y3 = mousyplc;
            adjustmark(&x3,&y3,newnumwalls);
            tempint1 = dmulscale4(x3-x2,x1-x3, y1-y3,y3-y2);
            tempint2 = dmulscale4(y1-y2,x1-x3, y1-y3,x2-x1);

            if (tempint2 != 0)
            {
                int32_t ps = 2, goodtogo;  // pointsize

                centerx = ((x1+x2) + scale(y1-y2,tempint1,tempint2))>>1;
                centery = ((y1+y2) + scale(x2-x1,tempint1,tempint2))>>1;

                dax = mulscale14(centerx-pos.x,zoom);
                day = mulscale14(centery-pos.y,zoom);
                drawline16base(halfxdim16+dax,midydim16+day, -2,-2, +2,+2, editorcolors[14]);
                drawline16base(halfxdim16+dax,midydim16+day, -2,+2, +2,-2, editorcolors[14]);

                circleang1 = getangle(x1-centerx,y1-centery);
                circleang2 = getangle(x2-centerx,y2-centery);

                circleangdir = 1;
                k = ((circleang2-circleang1)&2047);
                if (mulscale4(x3-x1,y2-y1) < mulscale4(x2-x1,y3-y1))
                {
                    circleangdir = -1;
                    k = -((circleang1-circleang2)&2047);
                }

                circlerad = ksqrt(dmulscale4(centerx-x1,centerx-x1, centery-y1,centery-y1))<<2;
                goodtogo = (numwalls+circlepoints <= MAXWALLS);

                for (i=circlepoints; i>0; i--)
                {
                    j = (circleang1 + scale(i,k,circlepoints+1))&2047;
                    dax = centerx + mulscale14(sintable[(j+512)&2047],circlerad);
                    day = centery + mulscale14(sintable[j],circlerad);

                    bclamp(&dax, -editorgridextent, editorgridextent);
                    bclamp(&day, -editorgridextent, editorgridextent);

                    if (bad > 0 && goodtogo)
                    {
                        insertpoint(circlewall,dax,day);
                        if (wall[circlewall].nextwall >= 0 && wall[circlewall].nextwall < circlewall)
                            circlewall++;
                    }

                    dax = mulscale14(dax-pos.x,zoom);
                    day = mulscale14(day-pos.y,zoom);
                    drawline16base(halfxdim16+dax,midydim16+day, -ps,-ps, +ps,-ps, editorcolors[14]);
                    drawline16base(halfxdim16+dax,midydim16+day, +ps,-ps, +ps,+ps, editorcolors[14]);
                    drawline16base(halfxdim16+dax,midydim16+day, +ps,+ps, -ps,+ps, editorcolors[14]);
                    drawline16base(halfxdim16+dax,midydim16+day, -ps,+ps, -ps,-ps, editorcolors[14]);
//                    drawcircle16(halfxdim16+dax, midydim16+day, 3, 14);
                }

                if (bad > 0)
                {
                    bad = 0;
                    keystatus[0x39] = 0;

                    if (goodtogo)
                    {
                        asksave = 1;
                        printmessage16("Circle points inserted.");
                        circlewall = -1;
                    }
                    else
                        printmessage16("Inserting circle points would exceed wall limit.");
                }
            }
        }

        if (bad > 0)   //Space bar test
        {
            keystatus[0x39] = 0;
            adjustmark(&mousxplc,&mousyplc,newnumwalls);

            if (checkautoinsert(mousxplc,mousyplc,newnumwalls) == 1)
            {
                printmessage16("You must insert a point there first.");
                bad = 0;
            }
        }

        if (bad > 0)  //Space
        {
            if (newnumwalls < numwalls)  // starting wall drawing
            {
                if (numwalls >= MAXWALLS-1)
                {
                    // whatever we do, we will need at least two new walls
                    printmessage16("Can't start sector drawing: wall limit reached.");
                    goto end_space_handling;
                }

                if (numsectors >= MAXSECTORS)
                {
                    printmessage16("Can't start sector drawing: sector limit reached.");
                    goto end_space_handling;
                }

                firstx = mousxplc;
                firsty = mousyplc;  //Make first point
                newnumwalls = numwalls;
                suckwall = -1;
                split = 0;

                Bmemset(&wall[newnumwalls], 0, sizeof(walltype));
                wall[newnumwalls].extra = -1;

                wall[newnumwalls].x = mousxplc;
                wall[newnumwalls].y = mousyplc;
                wall[newnumwalls].nextsector = -1;
                wall[newnumwalls].nextwall = -1;

                for (i=0; i<numwalls; i++)
                    if (wall[i].x == mousxplc && wall[i].y == mousyplc)
                        suckwall = i;

                wall[newnumwalls].point2 = newnumwalls+1;
                newnumwalls++;

                printmessage16("Sector drawing started.");
            }
            else  // 2nd point and up...
            {
                //if not back to first point
                if (firstx != mousxplc || firsty != mousyplc)  //nextpoint
                {
                    if (newnumwalls>=MAXWALLS)
                    {
                        printmessage16("Inserting another point would exceed wall limit.");
                        goto end_space_handling;
                    }

                    j = 0;
                    for (i=numwalls; i<newnumwalls; i++)
                        if (mousxplc == wall[i].x && mousyplc == wall[i].y)
                            j = 1;

                    if (j == 0)  // if new point is not on a position of already drawn points
                    {
                        // on the second point insertion, check if starting to split a sector
                        if (newnumwalls == numwalls+1)
                        {
                            dax = (wall[numwalls].x+mousxplc)>>1;
                            day = (wall[numwalls].y+mousyplc)>>1;

                            for (i=0; i<numsectors; i++)
                            {
                                if (inside(dax,day,i) != 1)
                                    continue;

                                //check if first point at point of sector
                                m = -1;
                                for (WALLS_OF_SECTOR(i, k))
                                    if (wall[k].x==wall[numwalls].x && wall[k].y==wall[numwalls].y)
                                    {
                                        m = k;
                                        break;
                                    }

                                // if the second insertion is not on a neighboring point of the first one...
                                if (m>=0 && (POINT2(k).x != mousxplc || POINT2(k).y != mousyplc))
                                    if (wall[lastwall(k)].x != mousxplc || wall[lastwall(k)].y != mousyplc)
                                    {
                                        split = 1;
                                        splitsect = i;
                                        splitstartwall = m;
                                        break;
                                    }
                            }
                        }

                        //make new point

                        //make sure not drawing over old red line
                        bad = 0;
                        for (i=0; i<numwalls; i++)
                        {
                            if (wall[i].nextwall >= 0)
                            {
                                int32_t lastwalx = wall[newnumwalls-1].x;
                                int32_t lastwaly = wall[newnumwalls-1].y;

                                if (wall[i].x == mousxplc && wall[i].y == mousyplc)
                                    if (POINT2(i).x == lastwalx && POINT2(i).y == lastwaly)
                                        bad = 1;
                                if (wall[i].x == lastwalx && wall[i].y == lastwaly)
                                    if (POINT2(i).x == mousxplc && POINT2(i).y == mousyplc)
                                        bad = 1;
                            }
                        }

                        if (bad == 0)
                        {
                            Bmemset(&wall[newnumwalls], 0, sizeof(walltype));
                            wall[newnumwalls].extra = -1;

                            wall[newnumwalls].x = mousxplc;
                            wall[newnumwalls].y = mousyplc;
                            wall[newnumwalls].nextsector = -1;
                            wall[newnumwalls].nextwall = -1;

                            for (i=0; i<numwalls; i++)
                                if (wall[i].x == mousxplc && wall[i].y == mousyplc)
                                    suckwall = i;

                            wall[newnumwalls].point2 = newnumwalls+1;
                            newnumwalls++;
                        }
                        else
                        {
                            printmessage16("You can't draw new lines over red lines.");
                            goto end_space_handling;
                        }
                    }
                }

                ////////// newnumwalls is at most MAXWALLS here //////////

                //if not split and back to first point
                if (!split && newnumwalls >= numwalls+3
                        && firstx==mousxplc && firsty==mousyplc)
                {
                    wall[newnumwalls-1].point2 = numwalls;

                    if (suckwall == -1)  //if no connections to other sectors
                    {
                        k = -1;
                        for (i=0; i<numsectors; i++)
                        {
                            if (inside(firstx,firsty,i) == 1)
                            {
                                // if all points inside that one sector i,
                                // will add an inner loop
                                for (j=numwalls+1; j<newnumwalls; j++)
                                {
                                    if (inside(wall[j].x, wall[j].y, i) == 0)
                                        goto check_next_sector;
                                }

                                k = i;
                                break;
                            }
check_next_sector: ;
                        }

                        if (k == -1)   //if not inside another sector either
                        {
                            //add island sector
                            if (clockdir(numwalls) == 1)
                                flipwalls(numwalls,newnumwalls);

                            Bmemset(&sector[numsectors], 0, sizeof(sectortype));
                            sector[numsectors].extra = -1;

                            sector[numsectors].wallptr = numwalls;
                            sector[numsectors].wallnum = newnumwalls-numwalls;
                            sector[numsectors].ceilingz = (-32<<8);
                            sector[numsectors].floorz = (32<<8);

                            for (i=numwalls; i<newnumwalls; i++)
                                copy_some_wall_members(i, -1);

                            headspritesect[numsectors] = -1;
                            numsectors++;

                            printmessage16("Created new sector %d", numsectors-1);
                        }
                        else       //else add loop to sector
                        {
                            if (clockdir(numwalls) == 0)
                                flipwalls(numwalls,newnumwalls);

                            j = newnumwalls-numwalls;

                            sector[k].wallnum += j;
                            for (i=k+1; i<numsectors; i++)
                                sector[i].wallptr += j;
                            suckwall = sector[k].wallptr;

                            for (i=0; i<numwalls; i++)
                            {
                                if (wall[i].nextwall >= suckwall)
                                    wall[i].nextwall += j;
                                if (wall[i].point2 >= suckwall)
                                    wall[i].point2 += j;
                            }

                            for (i=newnumwalls-1; i>=suckwall; i--)
                                Bmemcpy(&wall[i+j], &wall[i], sizeof(walltype));
                            for (i=0; i<j; i++)
                                Bmemcpy(&wall[i+suckwall], &wall[i+newnumwalls], sizeof(walltype));

                            for (i=suckwall; i<suckwall+j; i++)
                            {
                                wall[i].point2 += (suckwall-numwalls);

                                copy_some_wall_members(i, suckwall+j);
                                wall[i].cstat &= ~(1+16+32+64);
                            }

                            setfirstwall(k, suckwall+j);  // restore old first wall

                            printmessage16("Added inner loop to sector %d", k);
                        }
                    }
                    else  // if connected to at least one other sector
                    {
                        //add new sector with connections

                        if (clockdir(numwalls) == 1)
                            flipwalls(numwalls,newnumwalls);

                        sucksect = sectorofwall(suckwall);

                        if (numsectors != sucksect)
                            Bmemcpy(&sector[numsectors], &sector[sucksect], sizeof(sectortype));

                        sector[numsectors].wallptr = numwalls;
                        sector[numsectors].wallnum = newnumwalls-numwalls;

                        sector[numsectors].extra = -1;
                        sector[numsectors].lotag = sector[numsectors].hitag = 0;

                        sector[numsectors].ceilingstat &= ~2;
                        sector[numsectors].floorstat &= ~2;
                        sector[numsectors].ceilingheinum = sector[numsectors].floorheinum = 0;
                        sector[numsectors].ceilingpal = sector[numsectors].floorpal = 0;

                        for (i=numwalls; i<newnumwalls; i++)
                        {
                            copy_some_wall_members(i, suckwall);
                            checksectorpointer(i, numsectors);
                        }

                        headspritesect[numsectors] = -1;
                        numsectors++;

                        printmessage16("Created new sector %d based on sector %d", numsectors-1, sucksect);
                    }

                    numwalls = newnumwalls;
                    newnumwalls = -1;
                    asksave = 1;

                    goto end_space_handling;
                }
                ////////// split sector //////////
                else if (split == 1)
                {
                    int16_t danumwalls, splitendwall, doSectorSplit;
                    int16_t secondstartwall=-1;  // used only with splitting

                    startwall = sector[splitsect].wallptr;
                    endwall = startwall + sector[splitsect].wallnum - 1;

//                    OSD_Printf("numwalls: %d, newnumwalls: %d\n", numwalls, newnumwalls);
                    for (k=startwall; k<=endwall; k++)
                    {
                        if (wall[k].x != wall[newnumwalls-1].x || wall[k].y != wall[newnumwalls-1].y)
                            continue;

                        doSectorSplit = (loopnumofsector(splitsect,splitstartwall) == loopnumofsector(splitsect,k));

                        if (numwalls+2*(newnumwalls-numwalls-1) > MAXWALLS)
                        {
                            printmessage16("%s would exceed wall limit.", bad==0 ?
                                           "Splitting sector" : "Joining sector loops");
                            newnumwalls--;
                            break;
                        }

                        ////////// common code for splitting/loop joining //////////

                        splitendwall = k;
                        newnumwalls--;  //first fix up the new walls
                        for (i=numwalls; i<newnumwalls; i++)
                        {
                            copy_some_wall_members(i, startwall);
                            wall[i].point2 = i+1;
                        }

                        danumwalls = newnumwalls;  //where to add more walls
                        m = splitendwall;          //copy rest of loop next

                        if (doSectorSplit)
                        {
                            while (m != splitstartwall)
                                if (copyloop1(&danumwalls, &m)) goto split_not_enough_walls;
                            wall[danumwalls-1].point2 = numwalls;

                            //Add other loops for 1st sector
                            i = loopnum = loopnumofsector(splitsect,splitstartwall);

                            for (j=startwall; j<=endwall; j++)
                            {
                                k = loopnumofsector(splitsect,j);
                                if (k == i)
                                    continue;

                                if (k == loopnum)
                                    continue;

                                i = k;

                                if (loopinside(wall[j].x,wall[j].y, numwalls) != 1)
                                    continue;

                                m = j;          //copy loop
                                k = danumwalls;
                                do
                                    if (copyloop1(&danumwalls, &m)) goto split_not_enough_walls;
                                while (m != j);
                                wall[danumwalls-1].point2 = k;
                            }

                            secondstartwall = danumwalls;
                        }
                        else
                        {
                            do
                                if (copyloop1(&danumwalls, &m)) goto split_not_enough_walls;
                            while (m != splitendwall);
                        }

                        //copy split points for other sector backwards
                        for (j=newnumwalls; j>numwalls; j--)
                        {
                            Bmemcpy(&wall[danumwalls],&wall[j],sizeof(walltype));
                            wall[danumwalls].nextwall = -1;
                            wall[danumwalls].nextsector = -1;
                            wall[danumwalls].point2 = danumwalls+1;
                            danumwalls++;
                        }

                        m = splitstartwall;     //copy rest of loop next

                        if (doSectorSplit)
                        {
                            while (m != splitendwall)
                                if (copyloop1(&danumwalls, &m)) goto split_not_enough_walls;
                            wall[danumwalls-1].point2 = secondstartwall;
                        }
                        else
                        {
                            do
                                if (copyloop1(&danumwalls, &m)) goto split_not_enough_walls;
                            while (m != splitstartwall);
                            wall[danumwalls-1].point2 = numwalls;
                        }

                        //Add other loops for 2nd sector
                        i = loopnum = loopnumofsector(splitsect,splitstartwall);

                        for (j=startwall; j<=endwall; j++)
                        {
                            k = loopnumofsector(splitsect, j);
                            if (k==i)
                                continue;

                            if (doSectorSplit && k==loopnum)
                                continue;
                            if (!doSectorSplit && (k == loopnumofsector(splitsect,splitstartwall) || k == loopnumofsector(splitsect,splitendwall)))
                                continue;

                            i = k;

                            if (doSectorSplit && (loopinside(wall[j].x,wall[j].y, secondstartwall) != 1))
                                continue;

                            m = j; k = danumwalls;    //copy loop
                            do
                                if (copyloop1(&danumwalls, &m)) goto split_not_enough_walls;
                            while (m != j);
                            wall[danumwalls-1].point2 = k;
                        }

                        //fix all next pointers on old sector line
                        for (j=numwalls; j<danumwalls; j++)
                        {
                            if (wall[j].nextwall >= 0)
                            {
                                NEXTWALL(j).nextwall = j;

                                if (!doSectorSplit || j < secondstartwall)
                                    NEXTWALL(j).nextsector = numsectors;
                                else
                                    NEXTWALL(j).nextsector = numsectors+1;
                            }
                        }

                        if (doSectorSplit)
                        {
                            //set all next pointers on split
                            for (j=numwalls; j<newnumwalls; j++)
                            {
                                m = secondstartwall+(newnumwalls-1-j);
                                wall[j].nextwall = m;
                                wall[j].nextsector = numsectors+1;
                                wall[m].nextwall = j;
                                wall[m].nextsector = numsectors;
                            }

                            //copy sector attributes & fix wall pointers
                            Bmemcpy(&sector[numsectors], &sector[splitsect], sizeof(sectortype));
                            sector[numsectors].wallptr = numwalls;
                            sector[numsectors].wallnum = secondstartwall-numwalls;
                            Bmemcpy(&sector[numsectors+1], &sector[splitsect], sizeof(sectortype));
                            sector[numsectors+1].wallptr = secondstartwall;
                            sector[numsectors+1].wallnum = danumwalls-secondstartwall;
                        }
                        else
                        {
                            //copy sector attributes & fix wall pointers
                            Bmemcpy(&sector[numsectors], &sector[splitsect], sizeof(sectortype));
                            sector[numsectors].wallptr = numwalls;
                            sector[numsectors].wallnum = danumwalls-numwalls;
                        }

                        //fix sprites
                        j = headspritesect[splitsect];
                        while (j != -1)
                        {
                            k = nextspritesect[j];
                            if (!doSectorSplit || loopinside(sprite[j].x,sprite[j].y,numwalls) == 1)
                                changespritesect(j,numsectors);
                            //else if (loopinside(sprite[j].x,sprite[j].y,secondstartwall) == 1)
                            else  //Make sure no sprites get left out & deleted!
                                changespritesect(j,numsectors+1);
                            j = k;
                        }

                        numsectors += 1 + doSectorSplit;

                        k = danumwalls-numwalls;  //Back of number of walls of new sector for later
                        numwalls = danumwalls;

                        //clear out old sector's next pointers for clean deletesector
                        for (j=startwall; j<=endwall; j++)
                            wall[j].nextwall = wall[j].nextsector = -1;
                        deletesector(splitsect);

                        //Check pointers
                        for (j=numwalls-k; j<numwalls; j++)
                        {
                            if (wall[j].nextwall >= 0)
                                checksectorpointer(wall[j].nextwall, wall[j].nextsector);
                            checksectorpointer(j, sectorofwall(j));
                        }

                        //k now safe to use as temp
#if 0
                        if (doSectorSplit)
                            for (m=numsectors-2; m<numsectors; m++)
                            {
                                j = headspritesect[m];
                                while (j != -1)
                                {
                                    k = nextspritesect[j];
                                    setsprite(j, (vec3_t *)&sprite[j]);
                                    j = k;
                                }
                            }
#endif
                        printmessage16(doSectorSplit ? "Sector split." : "Loops joined.");

                        if (0)
                        {
split_not_enough_walls:
                            message("%s failed: not enough space beyond wall[]",
                                    doSectorSplit ? "Splitting sectors" : "Joining loops");
                        }

                        newnumwalls = -1;
                        break;
                    }
                }
            }
        }
end_space_handling:

        if (keystatus[0x1c]) //Left Enter
        {
            keystatus[0x1c] = 0;
            if (keystatus[0x2a] && keystatus[0x1d])  // LCtrl+LShift
            {
                printmessage16("CHECKING ALL POINTERS!");
                for (i=0; i<numsectors; i++)
                {
                    startwall = sector[i].wallptr;
                    for (j=startwall; j<numwalls; j++)
                        if (wall[j].point2 < startwall) startwall = wall[j].point2;
                    sector[i].wallptr = startwall;
                }
                for (i=numsectors-2; i>=0; i--)
                    sector[i].wallnum = sector[i+1].wallptr-sector[i].wallptr;
                sector[numsectors-1].wallnum = numwalls-sector[numsectors-1].wallptr;

                for (i=0; i<numwalls; i++)
                {
                    wall[i].nextsector = -1;
                    wall[i].nextwall = -1;
                }
                for (i=0; i<numsectors; i++)
                {
                    for (WALLS_OF_SECTOR(i, j))
                        checksectorpointer(j, i);
                }
                printmessage16("ALL POINTERS CHECKED!");
                asksave = 1;
            }
            else
            {
                if (linehighlight >= 0)
                {
                    checksectorpointer(linehighlight,sectorofwall(linehighlight));
                    printmessage16("Highlighted line pointers checked.");
                    asksave = 1;
                }
            }
        }

        {
            static int32_t backspace_last = 0;

            if (keystatus[0x0e]) //Backspace
            {
                keystatus[0x0e] = 0;

                if (newnumwalls >= numwalls)
                {
                    backspace_last = 1;

                    if (newnumwalls == numwalls+1 || keystatus[0x1d])  // LCtrl: delete all newly drawn walls
                        newnumwalls = -1;
                    else
                        newnumwalls--;
                }
                else if (backspace_last==0)
                {
                    graphicsmode += (1-2*(DOWN_BK(RUN) || keystatus[0x36]))+3;
                    graphicsmode %= 3;
                    printmessage16("2D mode textures %s",
                                   (graphicsmode == 2)?"enabled w/ animation":graphicsmode?"enabled":"disabled");
                }
            }
            else
                backspace_last = 0;
        }

        if (keystatus[0xd3] && eitherCTRL && (numwalls >= 0))  //sector delete
        {
            keystatus[0xd3] = 0;

            sucksect = -1;
            for (i=0; i<numsectors; i++)
            {
                if (inside_editor(&pos, searchx,searchy, zoom, mousxplc,mousyplc,i) != 1)
                    continue;
                
                k = 0;
                if (highlightsectorcnt >= 0)
                    for (j=0; j<highlightsectorcnt; j++)
                        if (highlightsector[j] == i)
                        {
                            for (j=highlightsectorcnt-1; j>=0; j--)
                            {
                                deletesector(highlightsector[j]);
                                for (k=j-1; k>=0; k--)
                                    if (highlightsector[k] >= highlightsector[j])
                                        highlightsector[k]--;
                            }

                            printmessage16("Highlighted sectors deleted.");
                            k = 1;
                            break;
                        }

                if (k == 0)
                {
                    deletesector(i);
                    printmessage16("Sector deleted.");
                }

                Bmemset(hlsectorbitmap, 0, sizeof(hlsectorbitmap));
                highlightsectorcnt = -1;

                Bmemset(show2dwall, 0, sizeof(show2dwall));
                update_highlight();

                newnumwalls = -1;
                asksave = 1;

                break;
            }
        }

        if (keystatus[0xd3] && (pointhighlight >= 0))
        {
            if ((pointhighlight&0xc000) == 16384)   //Sprite Delete
            {
                deletesprite(pointhighlight&16383);
                printmessage16("Sprite deleted.");
                updatenumsprites();
                update_highlight();
                asksave = 1;
            }
            keystatus[0xd3] = 0;
        }

        if (keystatus[0xd2] || keystatus[0x17])  //InsertPoint
        {
            if (highlightsectorcnt >= 0)
                duplicate_selected_sectors();
            else if (highlightcnt >= 0)
                duplicate_selected_sprites();
            else if (linehighlight >= 0)
            {
                int32_t wallsdrawn = newnumwalls-numwalls;
                int32_t wallis2sided = (wall[linehighlight].nextwall>=0);

                if (newnumwalls != -1)
                {
                    newnumwalls = -1;
                    Bmemcpy(&wall[MAXWALLS-wallsdrawn],&wall[numwalls],sizeof(walltype) * wallsdrawn);
                }
                else wallsdrawn = -1;

                if (max(numwalls,newnumwalls) >= MAXWALLS-wallis2sided)
                    printmessage16("Inserting point would exceed wall limit.");
                else
                {
                    getclosestpointonwall(mousxplc,mousyplc, linehighlight, &dax,&day);
                    adjustmark(&dax,&day, newnumwalls);
                    insertpoint(linehighlight, dax,day);
                    printmessage16("Point inserted.");

                    j = 0;
                    //Check to see if point was inserted over another point
                    for (i=numwalls-1; i>=0; i--)  //delete points
                        if (wall[i].x == POINT2(i).x && wall[i].y == POINT2(i).y)
                        {
                            deletepoint(i);
                            j++;
                        }
                    for (i=0; i<numwalls; i++)     //make new red lines?
                    {
                        if ((wall[i].x == dax && wall[i].y == day) || (POINT2(i).x == dax && POINT2(i).y == day))
                        {
                            checksectorpointer(i, sectorofwall(i));
//                            fixrepeats(i);
                        }
                    }
                    //if (j != 0)
                    //{
                    //   dax = ((wall[linehighlight].x + POINT2(linehighlight).x)>>1);
                    //   day = ((wall[linehighlight].y + POINT2(linehighlight).y)>>1);
                    //   if ((dax != wall[linehighlight].x) || (day != wall[linehighlight].y))
                    //      if ((dax != POINT2(linehighlight).x) || (day != POINT2(linehighlight).y))
                    //      {
                    //         insertpoint(linehighlight,dax,day);
                    //         printmessage16("Point inserted at midpoint.");
                    //      }
                    //}
                }

                if (wallsdrawn != -1)
                {
                    Bmemcpy(&wall[numwalls],&wall[MAXWALLS-wallsdrawn], sizeof(walltype)*wallsdrawn);
                    newnumwalls = numwalls + wallsdrawn;
                    for (i=numwalls; i<newnumwalls; i++)
                        wall[i].point2 = i+1;
                }

                asksave = 1;
            }
            keystatus[0xd2] = keystatus[0x17] = 0;
        }

        /*j = 0;
        for(i=22-1;i>=0;i--) updatecrc16(j,kensig[i]);
        if ((j&0xffff) != 0xebf)
        {
        	printf("Don't screw with my name.\n");
        	exit(0);
        }*/
        //printext16(9L,336+9L,4,-1,kensig,0);
        //printext16(8L,336+8L,12,-1,kensig,0);

        showframe(1);
        synctics = totalclock-lockclock;
        lockclock += synctics;

        if (keystatus[buildkeys[BK_MODE2D_3D]])
        {
            updatesector(pos.x,pos.y,&cursectnum);
            if (cursectnum >= 0)
                keystatus[buildkeys[BK_MODE2D_3D]] = 2;
            else
                printmessage16("Arrow must be inside a sector before entering 3D mode.");
        }

// vvv PK ------------------------------------ (LShift) Ctrl-X: (prev) next map
// this is copied from 'L' (load map), but without copying the highlighted sectors

        if (quickmapcycling && keystatus[0x2d])   //X
        {
            if (eitherCTRL)  //Ctrl
            {
nextmap:
//				bad = 0;
                i = menuselect_auto(keystatus[0x2a] ? 0:1); // Left Shift: prev map
                if (i < 0)
                {
                    if (i == -1)
                        message("No more map files.");
                    else if (i == -2)
                        message("No .MAP files found.");
                    else if (i == -3)
                        message("Load map first!");
                }
                else
                {
                    if (LoadBoard(NULL, 4))
                        goto nextmap;

                    sectorhighlightstat = -1;
                    joinsector[0] = -1;
                    circlewall = -1;
                    circlepoints = 7;
                    oposz = pos.z;
                }
                showframe(1);
                keystatus[0x1c] = 0;

                keystatus[0x2d]=keystatus[0x13]=0;

            }
        }
// ^^^ PK ------------------------------------

        if (keystatus[1] && joinsector[0] >= 0)
        {
            keystatus[1]=0;
            joinsector[0]=-1;
            printmessage16("No sectors joined.");
        }

CANCEL:
        if (keystatus[1])
        {
            m32_setkeyfilter(0);

            keystatus[1] = 0;
            _printmessage16("(N)ew, (L)oad, (S)ave, save (A)s, (T)est map, (U)ndo, (R)edo, (Q)uit");
            showframe(1);
            bflushchars();
            bad = 1;
            while (bad == 1)
            {
                if (handleevents())
                {
                    if (quitevent)
                        quitevent = 0;
                }
                idle();

                ch = bgetchar();

                if (keystatus[1])
                {
                    keystatus[1] = 0;
                    bad = 0;
                    // printmessage16("");
                }
                else if (ch == 'n' || ch == 'N')  //N
                {
                    bad = 0;

                    if (!ask_if_sure("Are you sure you want to start a new board? (Y/N)", 0))
                        break;
                    else
                    {
                        int32_t bakstat=-1;
                        mapinfofull_t bakmap;

                        if (highlightsectorcnt > 0)
                            bakstat = backup_highlighted_map(&bakmap);

//                        Bmemset(hlsectorbitmap, 0, sizeof(hlsectorbitmap));
//                        highlightsectorcnt = -1;

                        highlightcnt = -1;
                        //Clear all highlights
                        Bmemset(show2dwall, 0, sizeof(show2dwall));
                        Bmemset(show2dsprite, 0, sizeof(show2dsprite));

                        for (i=0; i<MAXSECTORS; i++) sector[i].extra = -1;
                        for (i=0; i<MAXWALLS; i++) wall[i].extra = -1;
                        for (i=0; i<MAXSPRITES; i++) sprite[i].extra = -1;

                        sectorhighlightstat = -1;
                        newnumwalls = -1;
                        joinsector[0] = -1;
                        circlewall = -1;
                        circlepoints = 7;

                        pos.x = 32768;          //new board!
                        pos.y = 32768;
                        pos.z = 0;
                        ang = 1536;
                        numsectors = 0;
                        numwalls = 0;
                        numsprites = 0;
                        cursectnum = -1;
                        initspritelists();
                        Bstrcpy(boardfilename,"newboard.map");
                        map_undoredo_free();

                        if (bakstat==0)
                        {
                            bakstat = restore_highlighted_map(&bakmap);
                            if (bakstat == -1)
                                message("Can't copy highlighted portion of old map: limits exceeded.");
                        }

                        break;
                    }

                    // printmessage16("");
                    showframe(1);
                }
                else if (ch == 'l' || ch == 'L')  //L
                {
                    bad = 0;
                    i = menuselect();
                    if (i < 0)
                    {
                        if (i == -2)
                            printmessage16("No .MAP files found.");
                    }
                    else
                    {
                        int32_t bakstat=-1;
                        mapinfofull_t bakmap;

                        if (highlightsectorcnt > 0)
                            bakstat = backup_highlighted_map(&bakmap);
// __old_mapcopy_2__
                        if (LoadBoard(NULL, 4))
                        {
                            printmessage16("Invalid map format.");
                            if (bakstat==0)
                                mapinfofull_free(&bakmap);
                        }
                        else
                        {
                            sectorhighlightstat = -1;
                            joinsector[0] = -1;
                            circlewall = -1;
                            circlepoints = 7;
                            oposz = pos.z;

                            if (bakstat==0)
                            {
                                bakstat = restore_highlighted_map(&bakmap);
                                if (bakstat == -1)
                                    message("Can't copy highlighted portion of old map: limits exceeded.");
                            }
                        }
                    }
                    showframe(1);
                    keystatus[0x1c] = 0;
                }
                else if (ch == 'a' || ch == 'A')  //A
                {
                    int32_t corrupt = CheckMapCorruption(4, 0);

                    bad = 0;

                    Bstrcpy(selectedboardfilename, boardfilename);
                    if (Bstrrchr(boardfilename, '/'))
                        Bstrcpy(boardfilename, Bstrrchr(boardfilename, '/')+1);

                    i = 0;
                    while ((boardfilename[i] != 0) && (i < 64))
                        i++;
                    if (boardfilename[i-4] == '.')
                        i -= 4;
                    boardfilename[i] = 0;

                    bflushchars();
                    while (bad == 0)
                    {
                        _printmessage16("%sSave as: ^011%s%s", corrupt>=4?"(map corrupt) ":"",
                                        boardfilename, (totalclock&32)?"_":"");
                        showframe(1);

                        if (handleevents())
                            quitevent = 0;

                        idle();

                        ch = bgetchar();

                        if (keystatus[1]) bad = 1;
                        else if (ch == 13) bad = 2;
                        else if (ch > 0)
                        {
                            if (i > 0 && (ch == 8 || ch == 127))
                            {
                                i--;
                                boardfilename[i] = 0;
                            }
                            else if (i < 40 && ch > 32 && ch < 128)
                            {
                                boardfilename[i++] = ch;
                                boardfilename[i] = 0;
                            }
                        }
                    }

                    if (bad == 1)
                    {
                        Bstrcpy(boardfilename, selectedboardfilename);
                        keystatus[1] = 0;
                        printmessage16("Operation cancelled");
                        showframe(1);
                    }
                    else if (bad == 2)
                    {
                        const char *f;
                        char *slash;

                        keystatus[0x1c] = 0;

                        boardfilename[i] = '.';
                        boardfilename[i+1] = 'm';
                        boardfilename[i+2] = 'a';
                        boardfilename[i+3] = 'p';
                        boardfilename[i+4] = 0;

                        slash = Bstrrchr(selectedboardfilename,'/');
                        Bstrcpy(slash ? slash+1 : selectedboardfilename, boardfilename);

                        _printmessage16("Saving board...");
                        showframe(1);

                        f = SaveBoard(selectedboardfilename, 0);

                        if (f)
                            printmessage16("Saved board to %s.", f);
                        else
                            printmessage16("Saving board failed.");

                        Bstrcpy(boardfilename, selectedboardfilename);
                    }
                    bad = 0;
                }
                else if (ch == 's' || ch == 'S')  //S
                {
                    const char *f;

                    bad = 0;

                    if (CheckMapCorruption(4, 0)>=4)
                        if (!ask_if_sure("Map is corrupt. Are you sure you want to save?", 0))
                            break;

                    _printmessage16("Saving board...");
                    showframe(1);

                    f = SaveBoard(NULL, 0);

                    if (f)
                        printmessage16("Saved board to %s.", f);
                    else
                        printmessage16("Saving board failed.");

                    showframe(1);
                }
                else if (ch == 't' || ch == 'T')
                {
                    test_map(0);
                }
                else if (ch == 'u' || ch == 'U')
                {
                    bad = 0;
                    if (map_undoredo(0)) printmessage16("Nothing to undo!");
                    else printmessage16("Revision %d undone",map_revision);
                }
                else if (ch == 'r' || ch == 'R')
                {
                    bad = 0;
                    if (map_undoredo(1)) printmessage16("Nothing to redo!");
                    else printmessage16("Restored revision %d",map_revision-1);
                }
                else if (ch == 'q' || ch == 'Q')  //Q
                {
                    bad = 0;

                    if (ask_if_sure("Are you sure you want to quit?", 0))
                    {
                        //QUIT!

                        int32_t corrupt = CheckMapCorruption(4, 0);

                        if (ask_if_sure(corrupt<4?"Save changes?":"Map corrupt. Save changes?", 2+(corrupt>=4)))
                            SaveBoard(NULL, 0);

                        while (keystatus[1] || keystatus[0x2e])
                        {
                            keystatus[1] = 0;
                            keystatus[0x2e] = 0;
                            quitevent = 0;
                            printmessage16("Operation cancelled");
                            showframe(1);
                            goto CANCEL;
                        }

                        ExtUnInit();
                        clearfilenames();
                        uninittimer();
                        uninitinput();
                        uninitengine();
                        exit(0);
                    }

                    // printmessage16("");
                    showframe(1);
                }
            }

            clearkeys();

            m32_setkeyfilter(1);
        }

        VM_OnEvent(EVENT_KEYS2D, -1);

        //nextpage();
    }

    for (i=0; i<highlightsectorcnt; i++)
    {
        for (WALLS_OF_SECTOR(highlightsector[i], j))
        {
            if (wall[j].nextwall >= 0)
                checksectorpointer(wall[j].nextwall, wall[j].nextsector);
            checksectorpointer(j, highlightsector[i]);
        }
    }

    fixspritesectors();

    if (setgamemode(fullscreen,xdimgame,ydimgame,bppgame) < 0)
    {
        initprintf("%d * %d not supported in this graphics mode\n",xdim,ydim);
        ExtUnInit();
        uninitinput();
        uninittimer();
        uninitsystem();
        clearfilenames();
        exit(1);
    }

    setbrightness(GAMMA_CALC,0,0);

    pos.z = oposz;

    searchx = clamp(scale(searchx,xdimgame,xdim2d), 8, xdimgame-8-1);
    searchy = clamp(scale(searchy,ydimgame,ydim2d-STATUS2DSIZ), 8, ydimgame-8-1);

    m32_setkeyfilter(0);

    VM_OnEvent(EVENT_ENTER3DMODE, -1);
}

// flags:
// 1: quit_is_yes
// 2: don't clear keys on return
static int32_t ask_if_sure(const char *query, uint32_t flags)
{
    char ch;
    int32_t ret=-1;

    if (!query)
        _printmessage16("Are you sure?");
    else
        _printmessage16("%s", query);
    showframe(1);
    bflushchars();

    while ((keystatus[1]|keystatus[0x2e]) == 0 && ret==-1)
    {
        if (handleevents())
        {
            if (quitevent)
            {
                if (flags&1)
                    return 1;
                else
                    quitevent = 0;
            }
        }
        idle();

        ch = bgetchar();

        if (ch == 'y' || ch == 'Y')
            ret = 1;
        else if (ch == 'n' || ch == 'N' || ch == 13 || ch == ' ')
            ret = 0;
    }

    if ((flags&2)==0)
        clearkeys();

    if (ret >= 0)
        return ret;

    return 0;
}

static int32_t ask_above_or_below()
{
    char ch;
    int32_t ret=-1;

    _printmessage16("Extend above (a) or below (z)?");

    showframe(1);
    bflushchars();

    while ((keystatus[1]|keystatus[0x2e]) == 0 && ret==-1)
    {
        if (handleevents())
            quitevent = 0;

        idle();
        ch = bgetchar();

        if (ch == 'a' || ch == 'A')
            ret = 0;
        else if (ch == 'z' || ch == 'Z')
            ret = 1;
    }

    clearkeys();

    return ret;
}

// flags:  1:no ExSaveMap (backup.map)
const char *SaveBoard(const char *fn, uint32_t flags)
{
    const char *f;
    int32_t ret;

    if (!fn)
        fn = boardfilename;

    if (pathsearchmode)
        f = fn;
    else
    {
        // virtual filesystem mode can't save to directories so drop the file into
        // the current directory
        f = Bstrrchr(fn, '/');
        if (!f)
            f = fn;
        else
            f++;
    }

    fixspritesectors();   //Do this before saving!
    updatesector(startposx,startposy,&startsectnum);
    ExtPreSaveMap();
    ret = saveboard(f,&startposx,&startposy,&startposz,&startang,&startsectnum);
    if ((flags&1)==0)
        ExtSaveMap(f);

    if (!ret)
        return f;
    else
        return NULL;
}

// flags:  1: for running on Mapster32 init
//         4: passed to loadboard flags (no polymer_loadboard); implies no maphack loading
int32_t LoadBoard(const char *filename, uint32_t flags)
{
    int32_t i;

    if (!filename)
        filename = selectedboardfilename;

    if (filename != boardfilename)
        Bstrcpy(boardfilename, filename);

    if ((flags&1)==0)
    {
        highlightcnt = -1;
        Bmemset(show2dwall, 0, sizeof(show2dwall));  //Clear all highlights
        Bmemset(show2dsprite, 0, sizeof(show2dsprite));

        newnumwalls = -1;
    }

///    sectorhighlightstat = -1;
///    joinsector[0] = -1;
///    circlewall = -1;
///    circlepoints = 7;

    for (i=0; i<MAXSECTORS; i++) sector[i].extra = -1;
    for (i=0; i<MAXWALLS; i++) wall[i].extra = -1;
    for (i=0; i<MAXSPRITES; i++) sprite[i].extra = -1;

    ExtPreLoadMap();
    i = loadboard(boardfilename,(flags&4)|(!pathsearchmode&&grponlymode?2:0),
                  &pos.x,&pos.y,&pos.z,&ang,&cursectnum);
    if (i == -2) i = loadoldboard(boardfilename,(!pathsearchmode&&grponlymode?2:0),
                                  &pos.x,&pos.y,&pos.z,&ang,&cursectnum);
    if (i < 0)
    {
//        printmessage16("Invalid map format.");
        return i;
    }
    else
    {
        if ((flags&4)==0)
            loadmhk(0);

        ExtLoadMap(boardfilename);

        if (mapversion < 7) message("Map %s loaded successfully and autoconverted to V7!",boardfilename);
        else
        {
            i = CheckMapCorruption(4, 0);
            message("Loaded map %s %s",boardfilename, i==0?"successfully":
                    (i<4 ? "(moderate corruption)" : "(HEAVY corruption)"));
        }
    }

    updatenumsprites();
    startposx = pos.x;      //this is same
    startposy = pos.y;
    startposz = pos.z;
    startang = ang;
    startsectnum = cursectnum;

///    oposz = pos.z;

    return 0;
}

void getpoint(int32_t searchxe, int32_t searchye, int32_t *x, int32_t *y)
{
    bclamp(&pos.x, -editorgridextent, editorgridextent);
    bclamp(&pos.y, -editorgridextent, editorgridextent);

    searchxe -= halfxdim16;
    searchye -= midydim16;

    if (m32_sideview)
    {
        if (m32_sidesin!=0)
            searchye = divscale14(searchye, m32_sidesin);
        rotatepoint(0,0, searchxe,searchye, -m32_sideang, &searchxe,&searchye);
    }

    *x = pos.x + divscale14(searchxe,zoom);
    *y = pos.y + divscale14(searchye,zoom);

    bclamp(x, -editorgridextent, editorgridextent);
    bclamp(y, -editorgridextent, editorgridextent);
}

static int32_t getlinehighlight(int32_t xplc, int32_t yplc, int32_t line)
{
    int32_t i, dst, dist, closest, x1, y1, x2, y2, nx, ny;

    if (numwalls == 0)
        return(-1);

    if (mouseb & 1)
        return line;

    if ((pointhighlight&0xc000) == 16384)
        return (-1);

    dist = 1024;
    closest = -1;
    for (i=0; i<numwalls; i++)
    {
        getclosestpointonwall(xplc,yplc, i, &nx,&ny);
        dst = klabs(xplc-nx) + klabs(yplc-ny);
        if (dst <= dist)
        {
            dist = dst;
            closest = i;
        }
    }

    if (closest>=0 && wall[closest].nextwall >= 0)
    {
        //if red line, allow highlighting of both sides
        x1 = wall[closest].x;
        y1 = wall[closest].y;
        x2 = POINT2(closest).x;
        y2 = POINT2(closest).y;
        if (dmulscale32(xplc-x1,y2-y1,-(x2-x1),yplc-y1) >= 0)
            closest = wall[closest].nextwall;
    }

    return(closest);
}

int32_t getpointhighlight(int32_t xplc, int32_t yplc, int32_t point)
{
    int32_t i, j, dst, dist = 512, closest = -1;
    int32_t dax,day;

    if (numwalls == 0)
        return(-1);

    if (mouseb & 1)
        return point;

    if (grid < 1)
        dist = 0;

    for (i=0; i<numsectors; i++)
        for (j=sector[i].wallptr; j<sector[i].wallptr+sector[i].wallnum; j++)
        {
            if (!m32_sideview)
                dst = klabs(xplc-wall[j].x) + klabs(yplc-wall[j].y);
            else
            {
                screencoords(&dax,&day, wall[j].x-pos.x,wall[j].y-pos.y, zoom);
                day += getscreenvdisp(getflorzofslope(i, wall[j].x,wall[j].y)-pos.z, zoom);

                if (halfxdim16+dax < 0 || halfxdim16+dax >= xdim || midydim16+day < 0 || midydim16+day >= ydim)
                    continue;

                dst = klabs(halfxdim16+dax-searchx) + klabs(midydim16+day-searchy);
            }

            if (dst <= dist)
            {
                // prefer white walls
                if (dst<dist || closest==-1 || (wall[j].nextwall>=0)-(wall[closest].nextwall>=0) <= 0)
                    dist = dst, closest = j;
            }
        }

    if (zoom >= 256)
        for (i=0; i<MAXSPRITES; i++)
            if (sprite[i].statnum < MAXSTATUS)
            {
                if (!m32_sideview)
                    dst = klabs(xplc-sprite[i].x) + klabs(yplc-sprite[i].y);
                else
                {
                    screencoords(&dax,&day, sprite[i].x-pos.x,sprite[i].y-pos.y, zoom);
                    day += getscreenvdisp(sprite[i].z-pos.z, zoom);

                    if (halfxdim16+dax < 0 || halfxdim16+dax >= xdim || midydim16+day < 0 || midydim16+day >= ydim)
                        continue;

                    dst = klabs(halfxdim16+dax-searchx) + klabs(midydim16+day-searchy);
                }

                // was (dst <= dist), but this way, when duplicating sprites,
                // the selected ones are dragged first
                if (dst < dist || (dst == dist && (show2dsprite[i>>3]&(1<<(i&7)))))
                    dist = dst, closest = i+16384;
            }

    return(closest);
}

static void locktogrid(int32_t *dax, int32_t *day)
{
    *dax = ((*dax+(1024>>grid))&(0xffffffff<<(11-grid)));
    *day = ((*day+(1024>>grid))&(0xffffffff<<(11-grid)));
}

static int32_t adjustmark(int32_t *xplc, int32_t *yplc, int16_t danumwalls)
{
    int32_t i, dst, dist, dax, day, pointlockdist;

    if (danumwalls < 0)
        danumwalls = numwalls;

    pointlockdist = 0;
    if (grid > 0 && gridlock)
        pointlockdist = (128>>grid);

    dist = pointlockdist;
    dax = *xplc;
    day = *yplc;

    for (i=0; i<danumwalls; i++)
    {
        dst = klabs((*xplc)-wall[i].x) + klabs((*yplc)-wall[i].y);
        if (dst < dist)
        {
            dist = dst;
            dax = wall[i].x;
            day = wall[i].y;
        }
    }
    if (dist == pointlockdist)
        if (gridlock && grid > 0)
            locktogrid(&dax, &day);

    *xplc = dax;
    *yplc = day;

    return(0);
}

static int32_t checkautoinsert(int32_t dax, int32_t day, int16_t danumwalls)
{
    int32_t i, x1, y1, x2, y2;

    if (danumwalls < 0)
        danumwalls = numwalls;

    for (i=0; i<danumwalls; i++)    // Check if a point should be inserted
    {
        x1 = wall[i].x;
        y1 = wall[i].y;
        x2 = POINT2(i).x;
        y2 = POINT2(i).y;

        if ((x1 != dax || y1 != day) && (x2 != dax || y2 != day))
            if ((x1 <= dax && dax <= x2) || (x2 <= dax && dax <= x1))
                if ((y1 <= day && day <= y2) || (y2 <= day && day <= y1))
                    if ((dax-x1)*(y2-y1) == (day-y1)*(x2-x1))
                        return(1);          //insertpoint((short)i,dax,day);
    }

    return(0);
}

static int32_t clockdir(int16_t wallstart)   //Returns: 0 is CW, 1 is CCW
{
    int16_t i, themin;
    int32_t minx, tempint, x0, x1, x2, y0, y1, y2;

    minx = 0x7fffffff;
    themin = -1;
    i = wallstart-1;
    do
    {
        i++;
        if (POINT2(i).x < minx)
        {
            minx = POINT2(i).x;
            themin = i;
        }
    }
    while ((wall[i].point2 != wallstart) && (i < MAXWALLS));

    x0 = wall[themin].x;
    y0 = wall[themin].y;
    x1 = POINT2(themin).x;
    y1 = POINT2(themin).y;
    x2 = POINT2(wall[themin].point2).x;
    y2 = POINT2(wall[themin].point2).y;

    if (y1 >= y2 && y1 <= y0) return(0);
    if (y1 >= y0 && y1 <= y2) return(1);

    tempint = (x0-x1)*(y2-y1) - (x2-x1)*(y0-y1);
    if (tempint < 0)
        return(0);
    else
        return(1);
}

static void flipwalls(int16_t numwalls, int16_t newnumwalls)
{
    int32_t i, j, nume, tempint;

    nume = newnumwalls-numwalls;

    for (i=numwalls; i<numwalls+(nume>>1); i++)
    {
        j = numwalls+newnumwalls-i-1;
        tempint = wall[i].x; wall[i].x = wall[j].x; wall[j].x = tempint;
        tempint = wall[i].y; wall[i].y = wall[j].y; wall[j].y = tempint;
    }
}

static void insertpoint(int16_t linehighlight, int32_t dax, int32_t day)
{
    int16_t sucksect;
    int32_t i, j, k;
    uint32_t templenrepquot;

    j = linehighlight;
    sucksect = sectorofwall(j);
    templenrepquot = divscale12(wallength(j), wall[j].xrepeat);
    templenrepquot = max(1, templenrepquot);

    sector[sucksect].wallnum++;
    for (i=sucksect+1; i<numsectors; i++)
        sector[i].wallptr++;

    movewalls(j+1, +1);
    Bmemcpy(&wall[j+1], &wall[j], sizeof(walltype));

    wall[j].point2 = j+1;
    wall[j+1].x = dax;
    wall[j+1].y = day;
    fixxrepeat(j, templenrepquot);
    fixxrepeat(j+1, templenrepquot);

    if (wall[j].nextwall >= 0)
    {
        k = wall[j].nextwall;

        sucksect = sectorofwall(k);

        sector[sucksect].wallnum++;
        for (i=sucksect+1; i<numsectors; i++)
            sector[i].wallptr++;

        movewalls(k+1, +1);
        Bmemcpy(&wall[k+1], &wall[k], sizeof(walltype));

        wall[k].point2 = k+1;
        wall[k+1].x = dax;
        wall[k+1].y = day;
        fixxrepeat(k, templenrepquot);
        fixxrepeat(k+1, templenrepquot);

        j = wall[k].nextwall;
        wall[j].nextwall = k+1;
        wall[j+1].nextwall = k;
        wall[k].nextwall = j+1;
        wall[k+1].nextwall = j;
    }
}

static void deletepoint(int16_t point)
{
    int32_t i, j, k, sucksect;

    sucksect = sectorofwall(point);

    sector[sucksect].wallnum--;
    for (i=sucksect+1; i<numsectors; i++)
        sector[i].wallptr--;

    j = lastwall(point);
    k = wall[point].point2;
    wall[j].point2 = k;

    if (wall[j].nextwall >= 0)
    {
        NEXTWALL(j).nextwall = -1;
        NEXTWALL(j).nextsector = -1;
    }
    if (wall[point].nextwall >= 0)
    {
        NEXTWALL(point).nextwall = -1;
        NEXTWALL(point).nextsector = -1;
    }

    movewalls(point, -1);

    checksectorpointer(j, sucksect);
}

static int32_t deletesector(int16_t sucksect)
{
    int32_t i, j, k, nextk, startwall, endwall;

    while (headspritesect[sucksect] >= 0)
        deletesprite(headspritesect[sucksect]);
    updatenumsprites();

    startwall = sector[sucksect].wallptr;
    endwall = startwall + sector[sucksect].wallnum - 1;
    j = sector[sucksect].wallnum;

    for (i=sucksect; i<numsectors-1; i++)
    {
        k = headspritesect[i+1];
        while (k != -1)
        {
            nextk = nextspritesect[k];
            changespritesect(k, i);
            k = nextk;
        }

        Bmemcpy(&sector[i], &sector[i+1], sizeof(sectortype));
        sector[i].wallptr -= j;
    }
    numsectors--;

    for (i=startwall; i<=endwall; i++)
        if (wall[i].nextwall >= 0)
        {
            NEXTWALL(i).nextwall = -1;
            NEXTWALL(i).nextsector = -1;
        }

    movewalls(startwall, -j);
    for (i=0; i<numwalls; i++)
        if (wall[i].nextwall >= startwall)
            wall[i].nextsector--;

    return(0);
}

void fixspritesectors(void)
{
    int32_t i, j, dax, day, daz;

    for (i=numsectors-1; i>=0; i--)
        if (sector[i].wallnum <= 0 || sector[i].wallptr >= numwalls)
            deletesector(i);

    for (i=0; i<MAXSPRITES; i++)
        if (sprite[i].statnum < MAXSTATUS)
        {
            dax = sprite[i].x;
            day = sprite[i].y;

            if (inside(dax,day,sprite[i].sectnum) != 1)
            {
                daz = spriteheight(i, NULL);

                for (j=0; j<numsectors; j++)
                    if (inside(dax,day, j) == 1
                            && sprite[i].z >= getceilzofslope(j,dax,day)
                            && sprite[i].z-daz <= getflorzofslope(j,dax,day))
                    {
                        changespritesect(i, j);
                        break;
                    }
            }
        }
}

static int32_t movewalls(int32_t start, int32_t offs)
{
    int32_t i;

    if (offs < 0)  //Delete
    {
        for (i=start; i<numwalls+offs; i++)
            Bmemcpy(&wall[i], &wall[i-offs], sizeof(walltype));
    }
    else if (offs > 0)  //Insert
    {
        for (i=numwalls+offs-1; i>=start+offs; i--)
            Bmemcpy(&wall[i], &wall[i-offs], sizeof(walltype));
    }

    numwalls += offs;
    for (i=0; i<numwalls; i++)
    {
        if (wall[i].nextwall >= start) wall[i].nextwall += offs;
        if (wall[i].point2 >= start) wall[i].point2 += offs;
    }

    return(0);
}

int32_t wallength(int16_t i)
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

void fixrepeats(int16_t i)
{
    int32_t dist = wallength(i);
    int32_t day = wall[i].yrepeat;

    wall[i].xrepeat = clamp(mulscale10(dist,day), 1, 255);
}

void fixxrepeat(int16_t i, uint32_t lenrepquot)  // lenrepquot: divscale12(wallength,xrepeat)
{
    if (lenrepquot != 0)
        wall[i].xrepeat = clamp(divscale12(wallength(i), lenrepquot), 1, 255);
}


int32_t overridepm16y = -1;

void clearmidstatbar16(void)
{
    int32_t y = overridepm16y<0 ? STATUS2DSIZ : overridepm16y;

    begindrawing();
//    ydim16 = ydim;

    //  clearbuf((char *)(frameplace + (bytesperline*(ydim-STATUS2DSIZ+25L))),(bytesperline*(STATUS2DSIZ-1-(25<<1))) >> 2, 0x08080808l);

    CLEARLINES2D(ydim-y+25, STATUS2DSIZ+2-(25<<1), 0);

//        drawline16(0,ydim-STATUS2DSIZ,0,ydim-1,editorcolors[7]);
//        drawline16base(xdim,ydim, -1,-STATUS2DSIZ, -1,-1, editorcolors[7]);

    ydim16 = ydim-STATUS2DSIZ2;
    enddrawing();
}

static void clearministatbar16(void)
{
    int32_t i, col = whitecol - 21;
//    static const char *tempbuf = "Mapster32" " " VERSION;
    char tempbuf[16];

    begindrawing();

    for (i=ydim-STATUS2DSIZ2; i<ydim; i++)
    {
//        drawline256(0, i<<12, xdim<<12, i<<12, col);
        CLEARLINES2D(i, 1, (col<<24)|(col<<16)|(col<<8)|col);

        col--;
        if (col <= 0) break;
    }

    CLEARLINES2D(i, ydim-i, 0);

    Bsprintf(tempbuf, "Mapster32 %s", ExtGetVer());
    printext16(xdim2d-(Bstrlen(tempbuf)<<3)-3, ydim2d-STATUS2DSIZ2+10, editorcolors[4],-1, tempbuf, 0);
    printext16(xdim2d-(Bstrlen(tempbuf)<<3)-2, ydim2d-STATUS2DSIZ2+9, editorcolors[12],-1, tempbuf, 0);

    enddrawing();
}

static int16_t loopinside(int32_t x, int32_t y, int16_t startwall)
{
    int32_t x1, y1, x2, y2;
    int16_t i, cnt;

    cnt = clockdir(startwall);
    i = startwall;
    do
    {
        x1 = wall[i].x;
        x2 = POINT2(i).x;

        if (x1 >= x || x2 >= x)
        {
            y1 = wall[i].y;
            y2 = POINT2(i).y;

            if (y1 > y2)
            {
                swaplong(&x1, &x2);
                swaplong(&y1, &y2);
            }
            if (y1 <= y && y2 > y)
                if (x1*(y-y2)+x2*(y1-y) <= x*(y1-y2))
                    cnt ^= 1;
        }
        i = wall[i].point2;
    }
    while (i != startwall);

    return(cnt);
}

#if 0
static int32_t numloopsofsector(int16_t sectnum)
{
    int32_t i, numloops, startwall, endwall;

    numloops = 0;
    startwall = sector[sectnum].wallptr;
    endwall = startwall + sector[sectnum].wallnum;
    for (i=startwall; i<endwall; i++)
        if (wall[i].point2 < i) numloops++;
    return(numloops);
}
#endif

int32_t _getnumber16(const char *namestart, int32_t num, int32_t maxnumber, char sign, void *(func)(int32_t))
{
    char buffer[80], ch;
    int32_t n, danum, oldnum;

    danum = (int32_t)num;
    oldnum = danum;
    bflushchars();
    while (keystatus[0x1] == 0)
    {
        if (handleevents())
        {
            if (quitevent) quitevent = 0;
        }
        idle();

        ch = bgetchar();

        Bsprintf(buffer,"%s^011%d",namestart,danum);
        n = Bstrlen(buffer);
        if (totalclock & 32) Bstrcat(buffer,"_ ");
        _printmessage16("%s", buffer);

        if (func != NULL)
        {
            Bsprintf(buffer,"^011%s",(char *)func((int32_t)danum));
            // printext16(200L-24, ydim-STATUS2DSIZ+20L, editorcolors[9], editorcolors[0], buffer, 0);
            printext16(n<<3, ydim-STATUS2DSIZ+128, editorcolors[9], -1, buffer,0);
        }

        showframe(1);

        if (ch >= '0' && ch <= '9')
        {
            int64_t nbig;
            if (danum >= 0)
            {
                nbig = ((int64_t)danum*10)+(ch-'0');
                if (nbig <= (int64_t)maxnumber) danum = nbig;
            }
            else if (sign) // this extra check isn't hurting anything
            {
                nbig = ((int64_t)danum*10)-(ch-'0');
                if (nbig >= (int64_t)-maxnumber) danum = nbig;
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
            printmessage16("%s", buffer);
            break;
        }
        else if (ch == '-' && sign)  	// negate
        {
            danum = -danum;
        }
    }
    clearkeys();
    return(oldnum);
}

int32_t _getnumber256(const char *namestart, int32_t num, int32_t maxnumber, char sign, void *(func)(int32_t))
{
    char buffer[80], ch;
    int32_t danum, oldnum;

    danum = num;
    oldnum = danum;
    bflushchars();
    while (keystatus[0x1] == 0)
    {
        if (handleevents())
            quitevent = 0;

        drawrooms(pos.x,pos.y,pos.z,ang,horiz,cursectnum);
        ExtAnalyzeSprites();
        drawmasks();

#ifdef POLYMER
        if (rendmode == 4 && searchit == 2)
        {
            polymer_editorpick();
            drawrooms(pos.x,pos.y,pos.z,ang,horiz,cursectnum);
            ExtAnalyzeSprites();
            drawmasks();
        }
#endif

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
            Bsprintf(buffer,"%s",(char *)func((int32_t)danum));
            printmessage256(0, 9, buffer);
        }

        showframe(1);

        if (ch >= '0' && ch <= '9')
        {
            int64_t nbig;
            if (danum >= 0)
            {
                nbig = ((int64_t)danum*10)+(ch-'0');
                if (nbig <= (int64_t)maxnumber) danum = nbig;
            }
            else if (sign)
            {
                nbig = ((int64_t)danum*10)-(ch-'0');
                if (nbig >= (int64_t)-maxnumber) danum = nbig;
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
    }
    clearkeys();

    lockclock = totalclock;  //Reset timing

    return(oldnum);
}

// querystr: e.g. "Name: ", must be !=NULL
// defaultstr: can be NULL
//  NO overflow checks are done when copying them!
// maxlen: maximum length of entry string, if ==1, enter single char
const char *getstring_simple(const char *querystr, const char *defaultstr, int32_t maxlen)
{
    static char buf[128];
    int32_t ei=0, qrylen=0;
    char ch;

    bflushchars();
    clearkeys();

    if (maxlen==0)
        maxlen = 1000;

    Bmemset(buf, 0, sizeof(buf));

    qrylen = Bstrlen(querystr);
    Bmemcpy(buf, querystr, qrylen);

    ei = qrylen;

    if (defaultstr)
    {
        int32_t deflen = Bstrlen(defaultstr);
        Bmemcpy(&buf[ei], defaultstr, deflen);
        ei += deflen;
    }

    buf[ei] = 0;

    if (maxlen==1)
    {
        ei = qrylen;
        buf[ei] = '_';
        buf[ei+1] = 0;
    }

    while (1)
    {
        printext256(0, 0, whitecol, 0, buf, 0);
        showframe(1);

        if (handleevents())
            quitevent = 0;

        idle_waitevent();

        ch = bgetchar();

        if (ch==13)
        {
            if (maxlen != 1)
                buf[ei] = 0;
            break;
        }
        else if (keystatus[1])
        {
            clearkeys();
            return defaultstr;
        }

        if (maxlen!=1)
        {
            if (ei>qrylen && (ch==8 || ch==127))
            {
                buf[ei] = ' ';
                buf[--ei] = '_';
            }
            else if ((unsigned)ei<sizeof(buf)-2 && ei-qrylen<maxlen && isprint(ch))
            {
                buf[ei++] = ch;
                buf[ei] = '_';
                buf[ei+1] = 0;
            }
        }
        else
        {
            if (isalnum(ch) || ch==' ')
                buf[ei] = Btoupper(ch);
        }
    }

    clearkeys();

    return buf+qrylen;
}

static void clearfilenames(void)
{
    klistfree(finddirs);
    klistfree(findfiles);
    finddirs = findfiles = NULL;
    numfiles = numdirs = 0;
}

static int32_t getfilenames(const char *path, const char *kind)
{
    CACHE1D_FIND_REC *r;

    clearfilenames();
    finddirs = klistpath(path,"*",CACHE1D_FIND_DIR|CACHE1D_FIND_DRIVE|(!pathsearchmode&&grponlymode?CACHE1D_OPT_NOSTACK:0));
    findfiles = klistpath(path,kind,CACHE1D_FIND_FILE|(!pathsearchmode&&grponlymode?CACHE1D_OPT_NOSTACK:0));
    for (r = finddirs; r; r=r->next) numdirs++;
    for (r = findfiles; r; r=r->next) numfiles++;

    finddirshigh = finddirs;
    findfileshigh = findfiles;
    currentlist = 0;
    if (findfileshigh) currentlist = 1;

    return(0);
}

// vvv PK ------------------------------------
// copied off menuselect

static const char *g_oldpath=NULL;
static int32_t menuselect_auto(int32_t direction) // 20080104: jump to next (direction!=0) or prev (direction==0) file
{
    const char *boardbasename;

    if (!g_oldpath)
        return -3;  // not inited
    else
        Bmemcpy(selectedboardfilename, g_oldpath, BMAX_PATH);

    if (pathsearchmode)
        Bcanonicalisefilename(selectedboardfilename, 1);  // clips off the last token and compresses relative path
    else
        Bcorrectfilename(selectedboardfilename, 1);

    getfilenames(selectedboardfilename, "*.map");
    if (numfiles==0)
        return -2;

    boardbasename = Bstrrchr(boardfilename,'/'); // PK
    if (!boardbasename)
        boardbasename=boardfilename;
    else
        boardbasename++;

    for (; findfileshigh; findfileshigh=findfileshigh->next)
        if (!Bstrcmp(findfileshigh->name,boardbasename))
            break;

    if (!findfileshigh)
        findfileshigh=findfiles;

    if (direction)
    {
        if (findfileshigh->next)
            findfileshigh=findfileshigh->next;
        else
            return -1;
    }
    else
    {
        if (findfileshigh->prev)
            findfileshigh=findfileshigh->prev;
        else
            return -1;
    }

    Bstrcat(selectedboardfilename, findfileshigh->name);

    return(0);
}
// ^^^ PK ------------------------------------

static int32_t menuselect(void)
{
    int32_t listsize;
    int32_t i;
    char ch, buffer[96], /*PK*/ *boardbasename;
    static char oldpath[BMAX_PATH];
    CACHE1D_FIND_REC *dir;
    int32_t bakpathsearchmode = pathsearchmode;

    g_oldpath=oldpath; //PK: need it in menuselect_auto

    Bstrcpy(selectedboardfilename, oldpath);
    if (pathsearchmode)
        Bcanonicalisefilename(selectedboardfilename, 1);		// clips off the last token and compresses relative path
    else
        Bcorrectfilename(selectedboardfilename, 1);

    getfilenames(selectedboardfilename, "*.map");

    // PK 20080103: start with last selected map
    boardbasename = Bstrrchr(boardfilename,'/');
    if (!boardbasename)
        boardbasename=boardfilename;
    else
        boardbasename++;

    for (; findfileshigh; findfileshigh=findfileshigh->next)
        if (!Bstrcmp(findfileshigh->name,boardbasename))
            break;

    if (!findfileshigh)
        findfileshigh=findfiles;

    _printmessage16("Select map file with arrow keys and enter.");

    ydim16 = ydim-STATUS2DSIZ2;
    listsize = (ydim16-32)/9;

    do
    {
        begindrawing(); //{{{

        CLEARLINES2D(0, ydim16, 0);

        if (pathsearchmode)
            Bstrcpy(buffer,"Local filesystem mode.  Ctrl-F: game filesystem");
        else
            Bsprintf(buffer,"Game filesystem %smode.  Ctrl-F: local filesystem, Ctrl-G: %s",
                     grponlymode?"GRP-only ":"", grponlymode?"all files":"GRP contents only");

        printext16(halfxdim16-(8*Bstrlen(buffer)/2), 4, editorcolors[12],editorcolors[0],buffer,0);

        Bsnprintf(buffer,sizeof(buffer)-1,"(%d dirs, %d files) %s",numdirs,numfiles,selectedboardfilename);
        buffer[sizeof(buffer)-1] = 0;

        printext16(8,ydim16-8-1,editorcolors[8],editorcolors[0],buffer,0);

        if (finddirshigh)
        {
            dir = finddirshigh;
            for (i=(listsize/2)-1; i>=0; i--)
            {
                if (!dir->prev) break;
                else dir=dir->prev;
            }
            for (i=0; ((i<listsize) && dir); i++, dir=dir->next)
            {
                int32_t c = (dir->type == CACHE1D_FIND_DIR ? 2 : 3); //PK
                Bmemset(buffer,0,sizeof(buffer));
                Bstrncpy(buffer,dir->name,25);
                if (Bstrlen(buffer) == 25)
                    buffer[21] = buffer[22] = buffer[23] = '.', buffer[24] = 0;
                if (dir == finddirshigh)
                {
                    if (currentlist == 0) printext16(8,16+9*i,editorcolors[c|8],editorcolors[0],"->",0);
                    printext16(32,16+9*i,editorcolors[c|8],editorcolors[0],buffer,0);
                }
                else
                {
                    printext16(32,16+9*i,editorcolors[c],editorcolors[0],buffer,0);
                }
            }
        }

        if (findfileshigh)
        {
            dir = findfileshigh;
            for (i=(listsize/2)-1; i>=0; i--)
            {
                if (!dir->prev) break;
                else dir=dir->prev;
            }
            for (i=0; ((i<listsize) && dir); i++, dir=dir->next)
            {
                if (dir == findfileshigh)
                {
                    if (currentlist == 1) printext16(240,16+9*i,editorcolors[7|8],editorcolors[0],"->",0);
                    printext16(240+24,16+9*i,editorcolors[7|8],editorcolors[0],dir->name,0);
                }
                else
                {
                    printext16(240+24,16+9*i,editorcolors[7],editorcolors[0],dir->name,0);
                }
            }
        }

        enddrawing(); //}}}
        showframe(1);

        keystatus[0xcb] = 0;
        keystatus[0xcd] = 0;
        keystatus[0xc8] = 0;
        keystatus[0xd0] = 0;
        keystatus[0x1c] = 0;	//enter
        keystatus[0xf] = 0;		//tab
        keystatus[1] = 0;		//esc
        ch = 0;                      //Interesting fakery of ch = getch()
        while (ch == 0)
        {
            if (handleevents())
                if (quitevent)
                {
                    keystatus[1] = 1;
                    quitevent = 0;
                }

            idle();

            ch = bgetchar();

            {
                // JBF 20040208: seek to first name matching pressed character
                CACHE1D_FIND_REC *seeker = currentlist ? findfiles : finddirs;
                if (keystatus[0xc7]||keystatus[0xcf]) // home/end
                {
                    while (keystatus[0xcf]?seeker->next:seeker->prev)
                        seeker = keystatus[0xcf]?seeker->next:seeker->prev;
                    if (seeker)
                    {
                        if (currentlist) findfileshigh = seeker;
                        else finddirshigh = seeker;
                    }
                    ch = keystatus[0xcf]?80:72;
                    keystatus[0xc7] = keystatus[0xcf] = 0;
                }
                else if (keystatus[0xc9]|keystatus[0xd1]) // page up/down
                {
                    seeker = currentlist?findfileshigh:finddirshigh;
                    i = (ydim2d-STATUS2DSIZ2-48)>>5/*3*/;  //PK

                    while (i>0)
                    {
                        if (keystatus[0xd1]?seeker->next:seeker->prev)
                            seeker = keystatus[0xd1]?seeker->next:seeker->prev;
                        i--;
                    }
                    if (seeker)
                    {
                        if (currentlist) findfileshigh = seeker;
                        else finddirshigh = seeker;
                    }
                    ch = keystatus[0xd1]?80:72;
                    keystatus[0xc9] = keystatus[0xd1] = 0;
                }
                else
                {
                    char ch2;
                    if (ch > 0 && ((ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z') ||
                                   (ch >= '0' && ch <= '9') || (ch=='_')))
                    {
#ifdef _WIN32
                        if (ch >= 'a') ch -= ('a'-'A');
#endif
                        while (seeker)
                        {
                            ch2 = seeker->name[0];
#ifdef _WIN32
                            if (ch2 >= 'a' && ch2 <= 'z') ch2 -= ('a'-'A');
#endif
                            if (ch2 == ch) break;
                            seeker = seeker->next;
                        }
                        if (seeker)
                        {
                            if (currentlist) findfileshigh = seeker;
                            else finddirshigh = seeker;
                            continue;
                        }
                    }
                }
            }
            if (keystatus[0xcb]) ch = 9;		// left arr
            if (keystatus[0xcd]) ch = 9;		// right arr
            if (keystatus[0xc8]) ch = 72;   	// up arr
            if (keystatus[0xd0]) ch = 80;   	// down arr
        }

        if (ch==6)  // Ctrl-F
        {
            currentlist = 0;
            pathsearchmode = 1-pathsearchmode;
            if (pathsearchmode)
            {
                Bstrcpy(selectedboardfilename, "");
                Bcanonicalisefilename(selectedboardfilename, 0);
            }
            else Bstrcpy(selectedboardfilename, "/");

            getfilenames(selectedboardfilename, "*.map");
            Bstrcpy(oldpath,selectedboardfilename);
        }
        else if (ch==7)  // Ctrl-G
        {
            if (!pathsearchmode)
            {
                grponlymode = 1-grponlymode;
                getfilenames(selectedboardfilename, "*.map");
                Bstrcpy(oldpath,selectedboardfilename);
            }
        }
        else if (ch == 9)
        {
            if ((currentlist == 0 && findfiles) || (currentlist == 1 && finddirs))
                currentlist = 1-currentlist;
        }
        else if (keystatus[0xc8] /*(ch == 75) || (ch == 72)*/)
        {
            if (currentlist == 0)
            {
                if (finddirshigh && finddirshigh->prev)
                    finddirshigh = finddirshigh->prev;
            }
            else
            {
                if (findfileshigh && findfileshigh->prev)
                    findfileshigh = findfileshigh->prev;
            }
        }
        else if (keystatus[0xd0] /*(ch == 77) || (ch == 80)*/)
        {
            if (currentlist == 0)
            {
                if (finddirshigh && finddirshigh->next)
                    finddirshigh = finddirshigh->next;
            }
            else
            {
                if (findfileshigh && findfileshigh->next)
                    findfileshigh = findfileshigh->next;
            }
        }
        else if ((ch == 13) && (currentlist == 0))
        {
            if (finddirshigh->type == CACHE1D_FIND_DRIVE)
                Bstrcpy(selectedboardfilename, finddirshigh->name);
            else
                Bstrcat(selectedboardfilename, finddirshigh->name);

            Bstrcat(selectedboardfilename, "/");

            if (pathsearchmode)
                Bcanonicalisefilename(selectedboardfilename, 1);
            else
                Bcorrectfilename(selectedboardfilename, 1);

            Bstrcpy(oldpath,selectedboardfilename);
            //printf("Changing directories to: %s\n", selectedboardfilename);

            getfilenames(selectedboardfilename, "*.map");
            ch = 0;

            begindrawing();
            CLEARLINES2D(0, ydim16, 0);
            enddrawing();
            showframe(1);
        }

        if (ch == 13 && !findfileshigh) ch = 0;
    }
    while ((ch != 13) && (ch != 27));

    if (ch == 13)
    {
        Bstrcat(selectedboardfilename, findfileshigh->name);
        //printf("Selected file: %s\n", selectedboardfilename);

        return(0);
    }

    pathsearchmode = bakpathsearchmode;

    return(-1);
}

static int32_t fillsectorxy[MAXWALLS][2];

int32_t fillsector(int16_t sectnum, char fillcolor)
{
    int32_t x1, x2, y1, y2, sy, y;
    int32_t lborder, rborder, uborder, dborder, miny, maxy, dax;
    int16_t z, zz, startwall, endwall, fillcnt;

    UNREFERENCED_PARAMETER(fillcolor);

    lborder = 0; rborder = xdim;
    uborder = 0; dborder = ydim16;

    if (sectnum == -1)
        return 0;

    miny = dborder-1;
    maxy = uborder;

    startwall = sector[sectnum].wallptr;
    endwall = startwall + sector[sectnum].wallnum - 1;
    for (z=startwall; z<=endwall; z++)
    {
        screencoords(&x1,&y1, wall[z].x-pos.x,wall[z].y-pos.y, zoom);
        if (m32_sideview)
            y1 += getscreenvdisp(getflorzofslope(sectnum,wall[z].x,wall[z].y)-pos.z, zoom);

        x1 += halfxdim16;
        y1 += midydim16;

        if (m32_sideview)
        {
            fillsectorxy[z][0] = x1;
            fillsectorxy[z][1] = y1;
        }

        miny = min(miny, y1);
        maxy = max(maxy, y1);
    }

    if (miny < uborder) miny = uborder;
    if (maxy >= dborder) maxy = dborder-1;

//+((totalclock>>2)&3)
    for (sy=miny; sy<=maxy; sy+=3)	// JBF 20040116: numframes%3 -> (totalclock>>2)&3
    {
        y = pos.y + ((sy-midydim16)<<14)/zoom;

        fillist[0] = lborder; fillcnt = 1;
        for (z=startwall; z<=endwall; z++)
        {
            if (m32_sideview)
            {
                x1 = fillsectorxy[z][0];
                y1 = fillsectorxy[z][1];
                x2 = fillsectorxy[wall[z].point2][0];
                y2 = fillsectorxy[wall[z].point2][1];

                if (y1 > y2)
                {
                    swaplong(&x1, &x2);
                    swaplong(&y1, &y2);
                }

                if (y1 <= sy && sy < y2)
                {
                    if (fillcnt == sizeof(fillist)/sizeof(fillist[0]))
                        break;

                    x1 += scale(sy-y1, x2-x1, y2-y1);
                    fillist[fillcnt++] = x1;
                }
            }
            else
            {
                x1 = wall[z].x; x2 = POINT2(z).x;
                y1 = wall[z].y; y2 = POINT2(z).y;

                if (y1 > y2)
                {
                    swaplong(&x1, &x2);
                    swaplong(&y1, &y2);
                }

                if (y1 <= y && y < y2)
                    //if (x1*(y-y2) + x2*(y1-y) <= 0)
                {
                    dax = x1 + scale(y-y1, x2-x1, y2-y1);
                    dax = halfxdim16 + (((dax-pos.x)*zoom)>>14);
                    if (dax >= lborder)
                    {
                        if (fillcnt == sizeof(fillist)/sizeof(fillist[0]))
                            break;

                        fillist[fillcnt++] = dax;
                    }
                }
            }
        }

        if (fillcnt > 0)
        {
            for (z=1; z<fillcnt; z++)
                for (zz=0; zz<z; zz++)
                    if (fillist[z] < fillist[zz])
                        swaplong(&fillist[z], &fillist[zz]);

            for (z=(fillcnt&1); z<fillcnt-1; z+=2)
            {
                if (fillist[z] > rborder)
                    break;
                if (fillist[z+1] > rborder)
                    fillist[z+1] = rborder;

                drawline16(fillist[z],sy, fillist[z+1],sy, 159  //editorcolors[fillcolor]
                           -klabs(sintable[((totalclock<<3)&2047)]>>11));
            }
        }
    }

    return(0);
}

static int16_t whitelinescan(int16_t dalinehighlight)
{
    int32_t i, j, k;
    int16_t sucksect, tnewnumwalls;

    sucksect = sectorofwall(dalinehighlight);

    if (numsectors >= MAXSECTORS)
        return MAXWALLS+1;

    Bmemcpy(&sector[numsectors], &sector[sucksect], sizeof(sectortype));
    sector[numsectors].wallptr = numwalls;
    sector[numsectors].wallnum = 0;

    i = dalinehighlight;
    tnewnumwalls = numwalls;
    do
    {
        if (tnewnumwalls >= MAXWALLS)
            return MAXWALLS+1;

        j = lastwall(i);
        if (wall[j].nextwall >= 0)
        {
            j = wall[j].point2;
            for (k=0; k<numwalls; k++)
            {
                if (POINT2(k).x == wall[j].x && POINT2(k).y == wall[j].y)
                    if (wall[k].nextwall == -1)
                    {
                        j = k;
                        break;
                    }
            }
        }

        Bmemcpy(&wall[tnewnumwalls], &wall[i], sizeof(walltype));

        wall[tnewnumwalls].nextwall = j;
        wall[tnewnumwalls].nextsector = sectorofwall(j);

        tnewnumwalls++;
        sector[numsectors].wallnum++;

        i = j;
    }
    while (i != dalinehighlight);

    for (i=numwalls; i<tnewnumwalls-1; i++)
        wall[i].point2 = i+1;
    wall[tnewnumwalls-1].point2 = numwalls;

    if (clockdir(numwalls) == 1)
        return(-1);
    else
        return(tnewnumwalls);
}

int32_t loadnames(void)
{
    char buffer[1024], *p, *name, *number, *endptr;
    int32_t num, syms=0, line=0, a, comment=0;
    BFILE *fp;

    fp = fopenfrompath("NAMES.H","r");
    if (!fp)
    {
        if ((fp = fopenfrompath("names.h","r")) == NULL)
        {
            initprintf("Failed to open NAMES.H\n");
            return -1;
        }
    }

    //clearbufbyte(names, sizeof(names), 0);
    Bmemset(names,0,sizeof(names));

    initprintf("Loading NAMES.H\n");

    while (Bfgets(buffer, 1024, fp))
    {
        a = Bstrlen(buffer);
        if (a >= 1)
        {
            if (a > 1)
                if (buffer[a-2] == '\r') buffer[a-2] = 0;
            if (buffer[a-1] == '\n') buffer[a-1] = 0;
        }

        p = buffer;
        line++;
        while (*p == 32) p++;
        if (*p == 0) continue;	// blank line

        if (*p == '#' && !comment)
        {
            p++;
            while (*p == 32) p++;
            if (*p == 0) continue;	// null directive

            if (!Bstrncmp(p, "define ", 7))
            {
                // #define_...
                p += 7;
                while (*p == 32) p++;
                if (*p == 0)
                {
                    initprintf("Error: Malformed #define at line %d\n", line-1);
                    continue;
                }

                name = p;
                while (*p != 32 && *p != 0) p++;
                if (*p == 32)
                {
                    *(p++) = 0;
                    while (*p == 32) p++;
                    if (*p == 0)  	// #define_NAME with no number
                    {
                        initprintf("Error: No number given for name \"%s\" (line %d)\n", name, line-1);
                        continue;
                    }

                    number = p;
                    while (*p != 0) p++;
                    if (*p != 0) *p = 0;

                    // add to list
                    num = Bstrtol(number, &endptr, 10);
                    if (*endptr != 0)
                    {
                        p = endptr;
                        goto badline;
                    }
                    //printf("Grokked \"%s\" -> \"%d\"\n", name, num);
                    if (num < 0 || num >= MAXTILES)
                    {
                        initprintf("Error: Constant %d for name \"%s\" out of range (line %d)\n", num, name, line-1);
                        continue;
                    }

                    if (Bstrlen(name) > 24)
                        initprintf("Warning: Name \"%s\" longer than 24 characters (line %d). Truncating.\n", name, line-1);

                    Bstrncpy(names[num], name, 24);
                    names[num][24] = 0;

                    syms++;

                    continue;

                }
                else  	// #define_NAME with no number
                {
                    initprintf("Error: No number given for name \"%s\" (line %d)\n", name, line-1);
                    continue;
                }
            }
            else goto badline;
        }
        else if (*p == '/')
        {
            if (*(p+1) == '*') {comment++; continue;}
            if (*(p+1) == '/') continue;	// comment
        }
        else if (*p == '*' && p[1] == '/')
        {
            comment--; continue;
        }
        else if (comment)continue;
badline:
        initprintf("Error: Invalid statement found at character %d on line %d\n", (int32_t)(p-buffer), line-1);
    }
    initprintf("Read %d lines, loaded %d names.\n", line, syms);

    Bfclose(fp);
    return 0;
}

void printcoords16(int32_t posxe, int32_t posye, int16_t ange)
{
    char snotbuf[80];
    int32_t i, m;
    int32_t v8 = (numsectors > MAXSECTORSV7 || numwalls > MAXWALLSV7 || numsprites > MAXSPRITESV7);

    Bsprintf(snotbuf,"x:%d y:%d ang:%d r%d",posxe,posye,ange,map_revision-1);
    i = 0;
    while ((snotbuf[i] != 0) && (i < 33))
        i++;
    while (i < 33)
    {
        snotbuf[i] = 32;
        i++;
    }
    snotbuf[33] = 0;

    clearministatbar16();

    printext16(8, ydim-STATUS2DSIZ+128, whitecol, -1, snotbuf,0);

    if (highlightcnt<=0 && highlightsectorcnt<=0)
    {
        Bsprintf(snotbuf,"%d/%d sect. %d/%d walls %d/%d spri.",
                 numsectors, v8?MAXSECTORSV8:MAXSECTORSV7,
                 numwalls, v8?MAXWALLSV8:MAXWALLSV7,
                 numsprites, v8?MAXSPRITESV8:MAXSPRITESV7);
    }
    else
    {
        if (highlightcnt>0)
        {
            m = 0;
            for (i=0; i<highlightcnt; i++)
                m += !!(highlight[i]&16384);
            Bsprintf(snotbuf, "%d sprites, %d walls selected", m, highlightcnt-m);
        }
        else if (highlightsectorcnt>0)
            Bsprintf(snotbuf, "%d sectors selected", highlightsectorcnt);
        else
            snotbuf[0] = 0;

        v8 = 1;
    }

    m = xdim/8 - 264/8;
    m = clamp(m, 1, (signed)sizeof(snotbuf)-1);

    i = 0;
    while (snotbuf[i] && i < m)
        i++;
    while (i < m)
    {
        snotbuf[i] = 32;
        i++;
    }
    snotbuf[m] = 0;

    printext16(264, ydim-STATUS2DSIZ+128, v8?editorcolors[10]:whitecol, -1, snotbuf,0);
}

void updatenumsprites(void)
{
    int32_t i;

    numsprites = 0;
    for (i=0; i<MAXSPRITES; i++)
        numsprites += (sprite[i].statnum != MAXSTATUS);
}

static void copysector(int16_t soursector, int16_t destsector, int16_t deststartwall, char copystat,
                       const int16_t *oldtonewsect)
{
    int16_t i, j, k, m, newnumwalls, startwall, endwall;

    newnumwalls = deststartwall;  //erase existing sector fragments

    //duplicate walls
    startwall = sector[soursector].wallptr;
    endwall = startwall + sector[soursector].wallnum;
    for (j=startwall; j<endwall; j++)
    {
        Bmemcpy(&wall[newnumwalls], &wall[j], sizeof(walltype));
        wall[newnumwalls].point2 += deststartwall-startwall;

        if (wall[newnumwalls].nextwall >= 0)
        {
            k = wall[newnumwalls].nextsector;
            if (oldtonewsect && oldtonewsect[k]>=0)
            {
                wall[newnumwalls].nextsector = oldtonewsect[k];
                m = 0;
                for (i=0; i<highlightsectorcnt; i++)
                {
                    if (highlightsector[i]==k)
                        break;
                    m += sector[highlightsector[i]].wallnum;
                }

                if (i==highlightsectorcnt)
                {
                    message("internal error in copysector(): i==highlightsectorcnt");
                    goto nonextsector;
                }
                else if (highlightsector[i]==soursector)
                {
                    message("internal error in copysector(): highlightsector[i]==soursector");
                    goto nonextsector;
                }
                wall[newnumwalls].nextwall = numwalls + m + (wall[j].nextwall-sector[k].wallptr);
            }
            else
            {
nonextsector:
                wall[newnumwalls].nextsector = -1;
                wall[newnumwalls].nextwall = -1;
            }
            // the below code is incorrect in the general case since, in a set of
            // selected sectors, the order may not be the same as the destination ones
//            wall[newnumwalls].nextwall += deststartwall-startwall;
//            wall[newnumwalls].nextsector += destsector-soursector;
        }
        newnumwalls++;
    }

    //for(j=deststartwall;j<newnumwalls;j++)
    //{
    //   if (wall[j].nextwall >= 0)
    //      checksectorpointer(wall[j].nextwall,wall[j].nextsector);
    //   checksectorpointer((short)j,destsector);
    //}

    if (newnumwalls > deststartwall)
    {
        //duplicate sectors
        Bmemcpy(&sector[destsector],&sector[soursector],sizeof(sectortype));
        sector[destsector].wallptr = deststartwall;
        sector[destsector].wallnum = newnumwalls-deststartwall;

        if (copystat == 1)
        {
            //duplicate sprites
            j = headspritesect[soursector];
            while (j >= 0)
            {
                m = insertsprite(destsector,sprite[j].statnum);
                if (m<0)
                {
                    message("Some sprites not duplicated because limit was reached.");
                    break;
                }

                Bmemcpy(&sprite[m],&sprite[j],sizeof(spritetype));
                sprite[m].sectnum = destsector;   //Don't let memcpy overwrite sector!

                j = nextspritesect[j];
            }
        }
    }
}

#define DOPRINT(Yofs, fmt, ...) \
    Bsprintf(snotbuf, fmt, ## __VA_ARGS__); \
    printext16(8+col*200, ydim/*-(row*96)*/-STATUS2DSIZ+Yofs, color, -1, snotbuf, 0);

void showsectordata(int16_t sectnum, int16_t small)
{
    sectortype *sec;
    char snotbuf[80];
    int32_t col=0;  //,row = 0;
    int32_t color = small ? whitecol : editorcolors[11];

    sec = &sector[sectnum];

    if (small)
    {
        _printmessage16("^10Sector %d %s ^O(F7 to edit)", sectnum, ExtGetSectorCaption(sectnum));
        return;
    }

    DOPRINT(32, "^10Sector %d", sectnum);
    DOPRINT(48, "Firstwall: %d", sec->wallptr);
    DOPRINT(56, "Numberofwalls: %d", sec->wallnum);
    DOPRINT(64, "Firstsprite: %d", headspritesect[sectnum]);
    DOPRINT(72, "Tags: %d, %d", sec->hitag, sec->lotag);
    DOPRINT(80, "     (0x%x), (0x%x)", sec->hitag, sec->lotag);
    DOPRINT(88, "Extra: %d", sec->extra);
    DOPRINT(96, "Visibility: %d", sec->visibility);
    DOPRINT(104, "Pixel height: %d", (sec->floorz-sec->ceilingz)>>8);

    col++;

    DOPRINT(32, "^10CEILING:^O");
    DOPRINT(48, "Flags (hex): %x", sec->ceilingstat);
    DOPRINT(56, "(X,Y)pan: %d, %d", sec->ceilingxpanning, sec->ceilingypanning);
    DOPRINT(64, "Shade byte: %d", sec->ceilingshade);
    DOPRINT(72, "Z-coordinate: %d", sec->ceilingz);
    DOPRINT(80, "Tile number: %d", sec->ceilingpicnum);
    DOPRINT(88, "Ceiling heinum: %d", sec->ceilingheinum);
    DOPRINT(96, "Palookup number: %d", sec->ceilingpal);

    col++;

    DOPRINT(32, "^10FLOOR:^O");
    DOPRINT(48, "Flags (hex): %x", sec->floorstat);
    DOPRINT(56, "(X,Y)pan: %d, %d", sec->floorxpanning, sec->floorypanning);
    DOPRINT(64, "Shade byte: %d", sec->floorshade);
    DOPRINT(72, "Z-coordinate: %d", sec->floorz);
    DOPRINT(80, "Tile number: %d", sec->floorpicnum);
    DOPRINT(88, "Floor heinum: %d", sec->floorheinum);
    DOPRINT(96, "Palookup number: %d", sec->floorpal);
}

void showwalldata(int16_t wallnum, int16_t small)
{
    walltype *wal;
    int32_t sec;
    char snotbuf[80];
    int32_t col=0; //, row = 0;
    int32_t color = small ? whitecol : editorcolors[11];

    wal = &wall[wallnum];

    if (small)
    {
        _printmessage16("^10Wall %d %s ^O(F8 to edit)", wallnum, ExtGetWallCaption(wallnum));
        return;
    }

    DOPRINT(32, "^10Wall %d", wallnum);
    DOPRINT(48, "X-coordinate: %d", wal->x);
    DOPRINT(56, "Y-coordinate: %d", wal->y);
    DOPRINT(64, "Point2: %d", wal->point2);
    DOPRINT(72, "Sector: ^010%d", sectorofwall(wallnum));

    DOPRINT(88, "Tags: %d,  %d", wal->hitag, wal->lotag);
    DOPRINT(96, "     (0x%x),  (0x%x)", wal->hitag, wal->lotag);

    col++;

    DOPRINT(32, "^10%s^O", (wal->picnum>=0 && wal->picnum<MAXTILES) ? names[wal->picnum] : "!INVALID!");
    DOPRINT(48, "Flags (hex): %x", wal->cstat);
    DOPRINT(56, "Shade: %d", wal->shade);
    DOPRINT(64, "Pal: %d", wal->pal);
    DOPRINT(72, "(X,Y)repeat: %d, %d", wal->xrepeat, wal->yrepeat);
    DOPRINT(80, "(X,Y)pan: %d, %d", wal->xpanning, wal->ypanning);
    DOPRINT(88, "Tile number: %d", wal->picnum);
    DOPRINT(96, "OverTile number: %d", wal->overpicnum);

    col++;

    DOPRINT(48-(small?16:0), "nextsector: %d", wal->nextsector);
    DOPRINT(56-(small?16:0), "nextwall: %d", wal->nextwall);

    DOPRINT(72-(small?16:0), "Extra: %d", wal->extra);

    // TX 20050102 I'm not sure what unit dist<<4 is supposed to be, but dist itself is correct in terms of game coordinates as one would expect
    DOPRINT(96-(small?16:0),  "Wall length: %d",  wallength(wallnum));

    sec = sectorofwall(wallnum);
    DOPRINT(104-(small?16:0), "Pixel height: %d", (sector[sec].floorz-sector[sec].ceilingz)>>8);
}

void showspritedata(int16_t spritenum, int16_t small)
{
    spritetype *spr;
    char snotbuf[80];
    int32_t col=0; //, row = 0;
    int32_t color = small ? whitecol : editorcolors[11];

    spr = &sprite[spritenum];

    if (small)
    {
        _printmessage16("^10Sprite %d %s ^O(F8 to edit)",spritenum, ExtGetSpriteCaption(spritenum));
        return;
    }

    DOPRINT(32, "^10Sprite %d", spritenum);
    DOPRINT(48, "X-coordinate: %d", spr->x);
    DOPRINT(56, "Y-coordinate: %d", spr->y);
    DOPRINT(64, "Z-coordinate: %d", spr->z);

    DOPRINT(72, "Sectnum: ^010%d", spr->sectnum);
    DOPRINT(80, "Statnum: %d", spr->statnum);

    DOPRINT(96, "Tags: %d,  %d", spr->hitag, spr->lotag);
    DOPRINT(104, "     (0x%x),  (0x%x)", spr->hitag, spr->lotag);

    col++;

    DOPRINT(32, "^10%s^O", (spr->picnum>=0 && spr->picnum<MAXTILES) ? names[spr->picnum] : "!INVALID!");
    DOPRINT(48, "Flags (hex): %x", spr->cstat);
    DOPRINT(56, "Shade: %d", spr->shade);
    DOPRINT(64, "Pal: %d", spr->pal);
    DOPRINT(72, "(X,Y)repeat: %d, %d", spr->xrepeat, spr->yrepeat);
    DOPRINT(80, "(X,Y)offset: %d, %d", spr->xoffset, spr->yoffset);
    DOPRINT(88, "Tile number: %d", spr->picnum);

    col++;

    DOPRINT(48, "Angle (2048 degrees): %d", spr->ang);
    DOPRINT(56, "X-Velocity: %d", spr->xvel);
    DOPRINT(64, "Y-Velocity: %d", spr->yvel);
    DOPRINT(72, "Z-Velocity: %d", spr->zvel);
    DOPRINT(80, "Owner: %d", spr->owner);
    DOPRINT(88, "Clipdist: %d", spr->clipdist);
    DOPRINT(96, "Extra: %d", spr->extra);
}

#undef DOPRINT

// gets called once per totalclock increment since last call
void keytimerstuff(void)
{
    if (DOWN_BK(STRAFE) == 0)
    {
        if (DOWN_BK(TURNLEFT)) angvel = max(angvel-pk_turnaccel, -128);
        if (DOWN_BK(TURNRIGHT)) angvel = min(angvel+pk_turnaccel, 127);
    }
    else
    {
        if (DOWN_BK(TURNLEFT)) svel = min(svel+16, 255); // svel and vel aren't even chars...
        if (DOWN_BK(TURNRIGHT)) svel = max(svel-16, -256);
    }
    if (DOWN_BK(MOVEFORWARD))  vel = min(vel+16, 255);
    if (DOWN_BK(MOVEBACKWARD)) vel = max(vel-16, -256);
    /*  if (DOWN_BK(STRAFELEFT))  svel = min(svel+8, 127);
    	if (DOWN_BK(STRAFERIGHT)) svel = max(svel-8, -128); */

    if (angvel < 0) angvel = min(angvel+pk_turndecel, 0);
    if (angvel > 0) angvel = max(angvel-pk_turndecel, 0);
    if (svel < 0) svel = min(svel+6, 0);
    if (svel > 0) svel = max(svel-6, 0);
    if (vel < 0) vel = min(vel+6, 0);
    if (vel > 0) vel = max(vel-6, 0);
    /*    if(mlook) pos.z -= (horiz-101)*(vel/40); */
}

#if 0
int32_t snfillprintf(char *outbuf, size_t bufsiz, int32_t fill, const char *fmt, ...)
{
    char tmpstr[256];
    int32_t nwritten, ofs;
    va_list va;

    va_start(va, fmt);
    nwritten = Bvsnprintf(tmpstr, bufsiz, fmt, va);
    va_end(va);

    ofs = min(nwritten, (signed)bufsiz-1);
    Bmemset(outbuf, fill, bufsiz-ofs);

    return ofs;
}
#endif

void _printmessage16(const char *fmt, ...)
{
    int32_t i, ybase;
    char snotbuf[156];
    char tmpstr[160];
    va_list va;

    va_start(va, fmt);
    Bvsnprintf(tmpstr, 156, fmt, va);
    va_end(va);

    i = 0;
    while (tmpstr[i] && i < 146)
    {
        snotbuf[i] = tmpstr[i];
        i++;
    }
    snotbuf[i] = 0;
    if (lastpm16time == totalclock)
        Bstrcpy(lastpm16buf, snotbuf);

    clearministatbar16();

//    ybase = (overridepm16y >= 0) ? ydim-overridepm16y : ydim-STATUS2DSIZ+128-8;
    ybase = ydim-STATUS2DSIZ+128-8;

    printext16(/*(overridepm16y >= 0) ? 200L-24 :*/ 8, ybase+8, whitecol, -1, snotbuf, 0);
}

void printmessage256(int32_t x, int32_t y, const char *name)
{
    char snotbuf[80];
    int32_t i;

    i = 0;
    while (name[i] && i < 62)
    {
        snotbuf[i] = name[i];
        i++;
    }
    while (i < 62)
    {
        snotbuf[i] = 32;
        i++;
    }
    snotbuf[62] = 0;
    printext256(x+2,y+2,0,-1,snotbuf,0);
    printext256(x,y,whitecol,-1,snotbuf,0);
}

//Find closest point (*dax, *day) on wall (dawall) to (x, y)
static void getclosestpointonwall(int32_t x, int32_t y, int32_t dawall, int32_t *nx, int32_t *ny)
{
    walltype *wal;
    int64_t i, j, dx, dy;

    wal = &wall[dawall];
    dx = wall[wal->point2].x - wal->x;
    dy = wall[wal->point2].y - wal->y;
    i = dx*(x-wal->x) + dy*(y-wal->y);
    if (i <= 0) { *nx = wal->x; *ny = wal->y; return; }
    j = dx*dx+dy*dy;
    if (i >= j) { *nx = wal->x+dx; *ny = wal->y+dy; return; }
    i=((i<<15)/j)<<15;
    *nx = wal->x + ((dx*i)>>30);
    *ny = wal->y + ((dy*i)>>30);
}

static void initcrc(void)
{
    int32_t i, j, k, a;

    for (j=0; j<256; j++)   //Calculate CRC table
    {
        k = (j<<8); a = 0;
        for (i=7; i>=0; i--)
        {
            if (((k^a)&0x8000) > 0)
                a = ((a<<1)&65535) ^ 0x1021;   //0x1021 = genpoly
            else
                a = ((a<<1)&65535);
            k = ((k<<1)&65535);
        }
        crctable[j] = (a&65535);
    }
}

static int32_t GetWallZPeg(int32_t nWall)
{
    int32_t z=0, nSector, nNextSector;

    nSector = sectorofwall((int16_t)nWall);
    nNextSector = wall[nWall].nextsector;
    if (nNextSector == -1)
    {
        //1-sided wall
        if (wall[nWall].cstat&4) z = sector[nSector].floorz;
        else z = sector[nSector].ceilingz;
    }
    else
    {
        //2-sided wall
        if (wall[nWall].cstat&4)
            z = sector[nSector].ceilingz;
        else
        {
            if (sector[nNextSector].ceilingz > sector[nSector].ceilingz)
                z = sector[nNextSector].ceilingz;   //top step
            if (sector[nNextSector].floorz < sector[nSector].floorz)
                z = sector[nNextSector].floorz;   //bottom step
        }
    }
    return(z);
}

static void AlignWalls(int32_t nWall0, int32_t z0, int32_t nWall1, int32_t z1, int32_t nTile)
{
    int32_t n;

    //do the x alignment
    wall[nWall1].cstat &= ~0x0108;    //Set to non-flip
    wall[nWall1].xpanning = (uint8_t)((wall[nWall0].xpanning+(wall[nWall0].xrepeat<<3))%tilesizx[nTile]);

    z1 = GetWallZPeg(nWall1);

    for (n=picsiz[nTile]>>4; (1<<n)<tilesizy[nTile]; n++);

    wall[nWall1].yrepeat = wall[nWall0].yrepeat;
    wall[nWall1].ypanning = (uint8_t)(wall[nWall0].ypanning+(((z1-z0)*wall[nWall0].yrepeat)>>(n+3)));
}

void AutoAlignWalls(int32_t nWall0, int32_t ply)
{
    int32_t z0, z1, nTile, nWall1, branch, visible, nNextSector, nSector;

    nTile = wall[nWall0].picnum;
    branch = 0;
    if (ply == 0)
    {
        //clear visited bits
        Bmemset(visited, 0, sizeof(visited));
        visited[nWall0>>3] |= (1<<(nWall0&7));
    }

    z0 = GetWallZPeg(nWall0);

    nWall1 = wall[nWall0].point2;

    //loop through walls at this vertex in CCW order
    while (1)
    {
        //break if this wall would connect us in a loop
        if (visited[nWall1>>3]&(1<<(nWall1&7)))
            break;

        visited[nWall1>>3] |= (1<<(nWall1&7));

        //break if reached back of left wall
        if (wall[nWall1].nextwall == nWall0)
            break;

        if (wall[nWall1].picnum == nTile)
        {
            z1 = GetWallZPeg(nWall1);
            visible = 0;

            nNextSector = wall[nWall1].nextsector;
            if (nNextSector < 0)
                visible = 1;
            else
            {
                //ignore two sided walls that have no visible face
                nSector = NEXTWALL(nWall1).nextsector;
                if (getceilzofslope((int16_t)nSector,wall[nWall1].x,wall[nWall1].y) <
                        getceilzofslope((int16_t)nNextSector,wall[nWall1].x,wall[nWall1].y))
                    visible = 1;

                if (getflorzofslope((int16_t)nSector,wall[nWall1].x,wall[nWall1].y) >
                        getflorzofslope((int16_t)nNextSector,wall[nWall1].x,wall[nWall1].y))
                    visible = 1;
            }

            if (visible)
            {
                branch++;
                AlignWalls(nWall0,z0,nWall1,z1,nTile);

                //if wall was 1-sided, no need to recurse
                if (wall[nWall1].nextwall < 0)
                {
                    nWall0 = nWall1;
                    z0 = GetWallZPeg(nWall0);
                    nWall1 = wall[nWall0].point2;
                    branch = 0;
                    continue;
                }
                else
                    AutoAlignWalls(nWall1,ply+1);
            }
        }

        if (wall[nWall1].nextwall < 0) break;
        nWall1 = NEXTWALL(nWall1).point2;
    }
}

#define PLAYTEST_MAPNAME "autosave_playtest.map"

void test_map(int32_t mode)
{
    if (!mode)
        updatesector(pos.x, pos.y, &cursectnum);
    else
        updatesector(startposx, startposy, &startsectnum);

    if ((!mode && cursectnum >= 0) || (mode && startsectnum >= 0))
    {
        char *param = " -map " PLAYTEST_MAPNAME " -noinstancechecking";
        char *fullparam;
        char current_cwd[BMAX_PATH];
        int32_t slen = 0;
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
            chdir(current_cwd);     // after checking if file exists.

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
        if (mode)
            saveboard(PLAYTEST_MAPNAME,&startposx,&startposy,&startposz,&startang,&startsectnum);
        else
            saveboard(PLAYTEST_MAPNAME,&pos.x,&pos.y,&pos.z,&ang,&cursectnum);

        message("Board saved to " PLAYTEST_MAPNAME ". Starting the game...");
        OSD_Printf("...as `%s'\n", fullparam);

        showframe(1);
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
            if (system(fullparam))
                message("Error launching the game!");
            chdir(current_cwd);
        }
        else system(fullparam);
#endif
        printmessage16("Game process exited");
        initmouse();
        clearkeys();

        Bfree(fullparam);
    }
    else
        printmessage16("Position must be in valid player space to test map!");
}
