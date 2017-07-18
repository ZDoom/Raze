// "Build Engine & Tools" Copyright (c) 1993-1997 Ken Silverman
// Ken Silverman's official web site: "http://www.advsys.net/ken"
// See the included license file "BUILDLIC.TXT" for license info.
//
// This file has been modified from Ken Silverman's original release
// by Jonathon Fowler (jf@jonof.id.au)

#include "build.h"
#include "compat.h"
#include "pragmas.h"
#include "osd.h"
#include "cache1d.h"
#include "editor.h"
#include "common.h"
#include "colmatch.h"
#include "palette.h"

#include "baselayer.h"
#include "renderlayer.h"

#ifdef _WIN32
# include "winbits.h"
#endif


#include "m32script.h"

char levelname[BMAX_PATH] = {0};

#define TIMERINTSPERSECOND 120

#define updatecrc16(crc,dat) (crc = (((crc<<8)&65535)^crctable[((((uint16_t)crc)>>8)&65535)^dat]))
static int32_t crctable[256];
static char kensig[64];

static const char *CallExtGetVer(void);
static int32_t CallExtInit(void);
static int32_t CallExtPreInit(int32_t argc,char const * const * argv);
static int32_t CallExtPostStartupWindow(void);
static void CallExtPostInit(void);
static void CallExtUnInit(void);
static void CallExtPreCheckKeys(void);
static void CallExtAnalyzeSprites(int32_t, int32_t, int32_t, int32_t);
static void CallExtCheckKeys(void);
static void CallExtPreLoadMap(void);
static void CallExtSetupMapFilename(const char *mapname);
static void CallExtLoadMap(const char *mapname);
static int32_t CallExtPreSaveMap(void);
static void CallExtSaveMap(const char *mapname);
static inline const char *CallExtGetSectorCaption(int16_t sectnum) { return ExtGetSectorCaption(sectnum); }
static inline const char *CallExtGetWallCaption(int16_t wallnum) { return ExtGetWallCaption(wallnum); }
static inline const char *CallExtGetSpriteCaption(int16_t spritenum) { return ExtGetSpriteCaption(spritenum); }
static void CallExtShowSectorData(int16_t sectnum);
static void CallExtShowWallData(int16_t wallnum);
static void CallExtShowSpriteData(int16_t spritenum);
static void CallExtEditSectorData(int16_t sectnum);
static void CallExtEditWallData(int16_t wallnum);
static void CallExtEditSpriteData(int16_t spritenum);
// static const char *CallExtGetSectorType(int32_t lotag);

int8_t m32_clipping=2;
static int32_t m32_rotateang = 0;

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

// Start position
vec3_t startpos;
int16_t startang, startsectnum;

// Current position
vec3_t pos;
int32_t horiz = 100;
int16_t ang, cursectnum;
static int32_t hvel, vel, svel, angvel;
int32_t g_doHardcodedMovement = 1;

static int32_t mousexsurp = 0, mouseysurp = 0;
double msens = 1.0;

int32_t grponlymode = 0;
int32_t graphicsmode = 0;

int32_t synctics = 0, lockclock = 0;

// those ones save the respective 3d video vars while in 2d mode
// so that exiting from mapster32 in 2d mode saves the correct ones
float vid_gamma_3d=-1, vid_contrast_3d=-1, vid_brightness_3d=-1;

int32_t xdim2d = 640, ydim2d = 480, xdimgame = 640, ydimgame = 480, bppgame = 8;
int32_t forcesetup = 1;

#ifndef GEKKO
int32_t g_maxCacheSize = 24<<20;
#else
int32_t g_maxCacheSize = 8<<20;
#endif

static int16_t oldmousebstatus = 0;

char game_executable[BMAX_PATH] = {0};

int32_t zlock = 0x7fffffff, zmode = 0, kensplayerheight = 32;
//int16_t defaultspritecstat = 0;

int16_t localartfreq[MAXTILES];
int16_t localartlookup[MAXTILES], localartlookupnum;

char tempbuf[4096];

char names[MAXTILES][25];
const char *g_namesFileName = "NAMES.H";

int16_t asksave = 0;
int32_t osearchx, osearchy;                               //old search input

int32_t grid = 0, autogrid = 1, gridlock = 1, showtags = 2;
int32_t zoom = 768, gettilezoom = 1, ztarget = 768;
int32_t lastpm16time = 0;

extern int32_t mapversion;

int16_t highlight[MAXWALLS+MAXSPRITES];
int16_t highlightsector[MAXSECTORS], highlightsectorcnt = -1;
extern char textfont[128][8];

int32_t tempsectornum = -1;  // for auto ceiling/floor alignment
int32_t temppicnum, tempcstat, templotag, temphitag, tempextra;
uint32_t temppal, tempvis, tempxrepeat, tempyrepeat, tempxpanning=0, tempypanning=0;
int32_t tempshade, tempxvel, tempyvel, tempzvel;
int32_t tempstatnum=0, tempblend=0;
char somethingintab = 255;

// Only valid when highlightsectorcnt>0 and no structural
// modifications (deleting/inserting sectors or points, setting new firstwall)
// have been made:
static int16_t onextwall[MAXWALLS];  // onextwall[i]>=0 implies wall[i].nextwall < 0
static void mkonwvalid(void) { chsecptr_onextwall = onextwall; }
static void mkonwinvalid(void) { chsecptr_onextwall = NULL; tempsectornum=-1; }
static void mkonwinvalid_keeptempsect(void) { chsecptr_onextwall = NULL; }
static int32_t onwisvalid(void) { return chsecptr_onextwall != NULL; }

int32_t mlook = 0, mskip=0;
int32_t revertCTRL=0,scrollamount=3;
int32_t unrealedlook=1, quickmapcycling=1; //PK

char program_origcwd[BMAX_PATH];
const char *mapster32_fullpath;
char *testplay_addparam = 0;

static char boardfilename[BMAX_PATH], selectedboardfilename[BMAX_PATH];
//extern char levelname[BMAX_PATH];  // in astub.c   XXX: clean up this mess!!!

void B_SetBoardFileName(const char *fn)
{
    Bstrncpyz(boardfilename, fn, BMAX_PATH);
}

static fnlist_t fnlist;
static CACHE1D_FIND_REC *finddirshigh=NULL, *findfileshigh=NULL;
static int32_t currentlist=0;

//static int32_t repeatcountx, repeatcounty;

static int32_t fillist[640];
// used for fillsector, batch point insertion, backup_highlighted_map
static int32_t tempxyar[MAXWALLS][2];

static int32_t mousx, mousy;
int16_t prefixtiles[10] = { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10 };
uint8_t hlsectorbitmap[MAXSECTORS>>3];  // show2dsector is already taken...
static int32_t minhlsectorfloorz, numhlsecwalls;
int32_t searchlock = 0;

// used for:
//  - hl_all_bunch_sectors_p
//  - AlignWalls
//  - trace_loop
static uint8_t visited[MAXWALLS>>3];

int32_t m32_2d3dmode = 0;
int32_t m32_2d3dsize = 4;
vec2_t m32_2d3d = { 0xffff, 4 };

typedef struct
{
    int16_t numsectors, numwalls, numsprites;
#ifdef YAX_ENABLE
    int16_t numyaxbunches;
    int16_t *bunchnum;  // [numsectors][2]
    int16_t *ynextwall;  // [numwalls][2]
#endif
    usectortype *sector;
    uwalltype *wall;
    uspritetype *sprite;
} mapinfofull_t;

int32_t g_doScreenShot;

#define eitherALT   (keystatus[0x38]|keystatus[0xb8])
#define eitherCTRL  (keystatus[0x1d]|keystatus[0x9d])
#define eitherSHIFT (keystatus[0x2a]|keystatus[0x36])

#define DOWN_BK(BuildKey) (keystatus[buildkeys[BK_##BuildKey]])

int32_t pk_turnaccel=16;
int32_t pk_turndecel=12;
int32_t pk_uedaccel=3;

int8_t keeptexturestretch = 1;
int8_t sideview_reversehrot = 0;

int16_t pointhighlightdist = 256;
int16_t linehighlightdist = 1024;

char lastpm16buf[156];

//static int32_t checksectorpointer_warn = 0;
static int32_t saveboard_savedtags, saveboard_fixedsprites;
static int32_t saveboard_canceled;

static int32_t backup_highlighted_map(mapinfofull_t *mapinfo);
static int32_t restore_highlighted_map(mapinfofull_t *mapinfo, int32_t forreal);
static void SaveBoardAndPrintMessage(const char *fn);

static int32_t adjustmark(int32_t *xplc, int32_t *yplc, int16_t danumwalls);
static void locktogrid(int32_t *dax, int32_t *day);
static int32_t checkautoinsert(int32_t dax, int32_t day, int16_t danumwalls);
static void keytimerstuff(void);
static void flipwalls(int16_t numwalls, int16_t newnumwalls);
static int32_t insertpoint(int16_t linehighlight, int32_t dax, int32_t day, int32_t *mapwallnum);
static void deletepoint(int16_t point, int32_t runi);
static int32_t deletesector(int16_t sucksect);
static int16_t whitelinescan(int16_t sucksect, int16_t dalinehighlight);
static void printcoords16(int32_t posxe, int32_t posye, int16_t ange);
static void overheadeditor(void);
static int32_t getlinehighlight(int32_t xplc, int32_t yplc, int32_t line, int8_t ignore_pointhighlight);
static int32_t movewalls(int32_t start, int32_t offs);
static void loadnames(const char *namesfile);
static void getclosestpointonwall(int32_t x, int32_t y, int32_t dawall, int32_t *nx, int32_t *ny,
                                  int32_t maybe_screen_coord_p);
static void initcrc(void);

static int32_t menuselect(void);
static int32_t menuselect_auto(int, int); //PK

static int32_t insert_sprite_common(int32_t sucksect, int32_t dax, int32_t day);
static void correct_ornamented_sprite(int32_t i, int32_t hitw);

static int32_t getfilenames(const char *path, const char *kind);

// Get basename of BUILD file name (forward slashes as directory separators).
static const char *getbasefn(const char *fn)
{
    const char *slash = Bstrrchr(fn, '/');
    return slash ? slash+1 : fn;
}

void clearkeys(void)
{
    Bmemset(keystatus,0,sizeof(keystatus));
}

#ifdef USE_OPENGL
int32_t osdcmd_restartvid(osdfuncparm_t const * const UNUSED(parm))
{
    UNREFERENCED_CONST_PARAMETER(parm);

    if (!in3dmode()) return OSDCMD_OK;

    resetvideomode();
    if (setgamemode(fullscreen,xdim,ydim,bpp))
        OSD_Printf("restartvid: Reset failed...\n");

    return OSDCMD_OK;
}
#endif

static int32_t osdcmd_vidmode(osdfuncparm_t const * const parm)
{
    int32_t newx = xdim, newy = ydim, newbpp = bpp, newfullscreen = fullscreen;
#ifdef USE_OPENGL
    int32_t tmp;
#endif

    switch (parm->numparms)
    {
#ifdef USE_OPENGL
    case 1:	// bpp switch
        tmp = Batol(parm->parms[0]);
        if (!(tmp==8 || tmp==16 || tmp==32))
            return OSDCMD_SHOWHELP;
        newbpp = tmp;
        break;
    case 4:	// fs, res, bpp switch
        newfullscreen = (Batol(parm->parms[3]) != 0);
        fallthrough__;
    case 3:	// res & bpp switch
        tmp = Batol(parm->parms[2]);
        if (!(tmp==8 || tmp==16 || tmp==32))
            return OSDCMD_SHOWHELP;
        newbpp = tmp;
        fallthrough__;
#endif
    case 2: // res switch
        newx = Batol(parm->parms[0]);
        newy = Batol(parm->parms[1]);
        break;
    default:
        return OSDCMD_SHOWHELP;
    }

    if (!in3dmode())
    {
        qsetmodeany(newx,newy);
        xdim2d = xdim;
        ydim2d = ydim;

        begindrawing();	//{{{
        CLEARLINES2D(0, ydim16, 0);
        enddrawing();	//}}}

        ydim16 = ydim-STATUS2DSIZ2;

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


#ifdef M32_SHOWDEBUG
char m32_debugstr[64][128];
int32_t m32_numdebuglines=0;

static void M32_drawdebug(void)
{
    int32_t i;
    int32_t x=4, y=8;

#if 0
    {
        static char tstr[128];
        Bsprintf(tstr, "search... stat=%d, sector=%d, wall=%d (%d), isbottom=%d, asksave=%d",
                 searchstat, searchsector, searchwall, searchbottomwall, searchisbottom, asksave);
        printext256(x,y,whitecol,0,tstr,xdimgame>640?0:1);
    }
#endif
    if (m32_numdebuglines>0)
    {
        begindrawing();
        for (i=0; i<m32_numdebuglines && y<ydim-8; i++, y+=8)
            printext256(x,y,whitecol,0,m32_debugstr[i],xdimgame>640?0:1);
        enddrawing();
    }
    m32_numdebuglines=0;
}
#endif

#ifdef YAX_ENABLE
// Check whether bunchnum has exactly one corresponding floor and ceiling
// and return it in this case. If not 1-to-1, return -1.
int32_t yax_is121(int16_t bunchnum, int16_t getfloor)
{
    int32_t i;
    i = headsectbunch[0][bunchnum];
    if (i<0 || nextsectbunch[0][i]>=0)
        return -1;
    i = headsectbunch[1][bunchnum];
    if (i<0 || nextsectbunch[1][i]>=0)
        return -1;

    return headsectbunch[getfloor][bunchnum];
}

static int32_t yax_numsectsinbunch(int16_t bunchnum, int16_t cf)
{
    int32_t i, n=0;

    if (bunchnum<0 || bunchnum>=numyaxbunches)
        return -1;

    for (SECTORS_OF_BUNCH(bunchnum, cf, i))
        n++;

    return n;
}

static void yax_fixreverselinks(int16_t oldwall, int16_t newwall)
{
    int32_t cf, ynw;
    for (cf=0; cf<2; cf++)
    {
        ynw = yax_getnextwall(oldwall, cf);
        if (ynw >= 0)
            yax_setnextwall(ynw, !cf, newwall);
    }
}

static void yax_tweakwalls(int16_t start, int16_t offs)
{
    int32_t i, nw, cf;
    for (i=0; i<numwalls; i++)
        for (cf=0; cf<2; cf++)
        {
            nw = yax_getnextwall(i, cf);
            if (nw >= start)
                yax_setnextwall(i, cf, nw+offs);
        }
}

static void yax_resetbunchnums(void)
{
    int32_t i;

    for (i=0; i<MAXSECTORS; i++)
        yax_setbunches(i, -1, -1);
    yax_update(1);
    yax_updategrays(pos.z);
}

// Whether a wall is constrained by sector extensions.
// If false, it's a wall that you can freely move around,
// attach points to, etc...
static int32_t yax_islockedwall(int16_t line)
{
#ifdef NEW_MAP_FORMAT
    return (wall[line].upwall>=0 || wall[line].dnwall>=0);
#else
    return !!(wall[line].cstat&YAX_NEXTWALLBITS);
#endif
}

# define DEFAULT_YAX_HEIGHT (2048<<4)
#endif

static void reset_default_mapstate(void)
{
    pos.x = 32768;          //new board!
    pos.y = 32768;
    pos.z = 0;
    ang = 1536;
    cursectnum = -1;

    numsectors = 0;
    numwalls = 0;

    editorzrange[0] = INT32_MIN;
    editorzrange[1] = INT32_MAX;

    initspritelists();
    taglab_init();
    E_MapArt_Clear();
#ifdef YAX_ENABLE
    yax_resetbunchnums();
#endif
    g_loadedMapVersion = -1;
}

static void m32_keypresscallback(int32_t code, int32_t downp)
{
    UNREFERENCED_PARAMETER(downp);

    g_iReturnVar = code;
    VM_OnEvent(EVENT_KEYPRESS, -1);
}

void M32_ResetFakeRORTiles(void)
{
#ifdef POLYMER
# ifdef YAX_ENABLE
        // END_TWEAK ceiling/floor fake 'TROR' pics, see BEGIN_TWEAK in engine.c
        if (getrendermode() == REND_POLYMER && showinvisibility)
        {
            int32_t i;

            for (i=0; i<numyaxbunches; i++)
            {
                yax_tweakpicnums(i, YAX_CEILING, 1);
                yax_tweakpicnums(i, YAX_FLOOR, 1);
            }
        }
# endif
#endif
}

void M32_DrawRoomsAndMasks(void)
{
    static int srchwall = -1;
    const int32_t tmpyx=yxaspect, tmpvr=viewingrange;

    if (r_usenewaspect)
    {
        newaspect_enable = 1;
        setaspect_new();
    }

    VM_OnEvent(EVENT_PREDRAW3DSCREEN, -1);

    yax_preparedrawrooms();
    drawrooms(pos.x,pos.y,pos.z,ang,horiz,cursectnum);
    yax_drawrooms(CallExtAnalyzeSprites, cursectnum, 0, 0);

    const int osearchwall=searchwall, osearchstat=searchstat;
    if (srchwall >= 0)
    {
        // a.m32 states 'tduprot' and 'tduplin' need searchstat to check for
        // whether we've hit a sprite, but these would be only set after the
        // drawmasks(). Hence this hackish workaround.
        searchstat = 3;
        searchwall = srchwall;
    }
    CallExtAnalyzeSprites(0,0,0,0);
    searchwall = osearchwall, searchstat=osearchstat;

    drawmasks();
    srchwall = (searchstat == 3) ? searchwall : -1;
    M32_ResetFakeRORTiles();

#ifdef POLYMER
    if (getrendermode() == REND_POLYMER && searchit == 2)
    {
        polymer_editorpick();
        drawrooms(pos.x,pos.y,pos.z,ang,horiz,cursectnum);
        CallExtAnalyzeSprites(0,0,0,0);
        drawmasks();
        M32_ResetFakeRORTiles();
    }
#endif

    VM_OnEvent(EVENT_DRAW3DSCREEN, -1);

    if (g_doScreenShot)
    {
        screencapture("mcapxxxx.tga", 0);
        g_doScreenShot = 0;
    }

    if (r_usenewaspect)
    {
        newaspect_enable = 0;
        setaspect(tmpvr, tmpyx);
    }
}

void M32_OnShowOSD(int32_t shown)
{
    AppGrabMouse((!shown) + 2);
}

static void M32_FatalEngineError(void)
{
    wm_msgbox("Build Engine Initialization Error",
              "There was a problem initializing the Build engine: %s", engineerrstr);
    ERRprintf("app_main: There was a problem initializing the Build engine: %s\n", engineerrstr);
    exit(2);
}

int app_main(int argc, char const * const * argv)
{
#ifdef STARTUP_SETUP_WINDOW
    char cmdsetup = 0;
#endif
    char quitflag;
    int32_t i;

    pathsearchmode = 1;		// unrestrict findfrompath so that full access to the filesystem can be had

#ifdef USE_OPENGL
    OSD_RegisterFunction("restartvid","restartvid: reinitialize the video mode",osdcmd_restartvid);
    OSD_RegisterFunction("vidmode","vidmode <xdim> <ydim> <bpp> <fullscreen>: immediately change the video mode",osdcmd_vidmode);
    baselayer_osdcmd_vidmode_func = osdcmd_vidmode;
#else
    OSD_RegisterFunction("vidmode","vidmode <xdim> <ydim>: immediately change the video mode",osdcmd_vidmode);
#endif

    wm_setapptitle(AppProperName);

    editstatus = 1;

    if ((i = CallExtPreInit(argc,argv)) < 0) return -1;

#ifdef _WIN32
    backgroundidle = 1;
#endif

    for (i=1; i<argc; i++)
    {
        if (argv[i][0] == '-')
        {
#ifdef STARTUP_SETUP_WINDOW
            if (!Bstrcmp(argv[i], "-setup")) cmdsetup = 1;
            else
#endif
            if (!Bstrcmp(argv[i], "-help") || !Bstrcmp(argv[i], "--help") || !Bstrcmp(argv[i], "-?"))
            {
#ifdef WM_MSGBOX_WINDOW
                wm_msgbox(AppProperName,
#else
                Bprintf(
#endif
                    "%s\n"
                    "Syntax: %s [options] mapname\n"
                    "Options:\n"
                    "\t-grp\tUse an extra GRP or ZIP file.\n"
                    "\t-g\tSame as above.\n"
#ifdef STARTUP_SETUP_WINDOW
                    "\t-setup\tDisplays the configuration dialogue box before entering the editor.\n"
#endif
                    , AppProperName, AppTechnicalName);
                return 0;
            }
            continue;
        }
    }

    if (boardfilename[0] == 0)
        Bstrcpy(boardfilename,"newboard.map");
    else if (Bstrchr(boardfilename,'.') == 0)
        Bstrcat(boardfilename, ".map");
    //Bcanonicalisefilename(boardfilename,0);

    OSD_SetFunctions(
        NULL, NULL, NULL, NULL, NULL,
        COMMON_clearbackground,
        BGetTime,
        M32_OnShowOSD
    );

    if (!getcwd(program_origcwd,BMAX_PATH))
        program_origcwd[0] = '\0';

    Bstrncpy(game_executable, DefaultGameLocalExec, sizeof(game_executable));

    if (preinitengine())
        M32_FatalEngineError();

    if ((i = CallExtInit()) < 0) return -1;

#ifdef STARTUP_SETUP_WINDOW
    if (i || forcesetup || cmdsetup)
    {
        if (quitevent || !startwin_run())
        {
            uninitengine();
            Bexit(0);
        }
    }
#endif

    if (CallExtPostStartupWindow() < 0) return -1;

    loadnames(g_namesFileName);

    if (initinput()) return -1;

    initmouse();

    inittimer(TIMERINTSPERSECOND);
    installusertimercallback(keytimerstuff);

    loadpics("tiles000.art", g_maxCacheSize);

    Bstrcpy(kensig,"Uses BUILD technology by Ken Silverman");
    initcrc();

    const char *defsfile = G_DefFile();

    if (!loaddefinitionsfile(defsfile))
        initprintf("Definitions file \"%s\" loaded.\n",defsfile);

    for (i=0; i < g_defModulesNum; ++i)
        Bfree(g_defModules[i]);
    DO_FREE_AND_NULL(g_defModules);
    g_defModulesNum = 0;

    if (E_PostInit())
        M32_FatalEngineError();

    CallExtPostInit();

#ifdef YAX_ENABLE
    // init dummy texture for YAX
    // must be after loadpics(), which inits BUILD's cache

    i = MAXTILES-1;
    if (tilesiz[i].x==0 && tilesiz[i].y==0)
    {
        static char R[8*16] = { //
            0, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 0, 0,
            0, 1, 0, 0, 1, 0, 0, 0, 0, 0, 0, 1, 0, 0, 1, 0,
            0, 1, 0, 0, 1, 0, 0, 0, 0, 0, 0, 1, 0, 0, 1, 0,
            0, 1, 1, 1, 0, 0, 0, 1, 1, 0, 0, 1, 1, 1, 0, 0,
            0, 1, 0, 1, 0, 0, 1, 0, 0, 1, 0, 1, 0, 0, 1, 0,
            0, 1, 0, 0, 1, 0, 1, 0, 0, 1, 0, 1, 0, 0, 1, 0,
            0, 1, 0, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 0, 1, 0,
        };

        char *newtile;
        int32_t sx=32, sy=32, col, j;

        walock[i] = 255; // permanent tile
        picsiz[i] = 5 + (5<<4);
        tilesiz[i].x = sx; tilesiz[i].y = sy;
        allocache(&waloff[i], sx*sy, &walock[i]);
        newtile = (char *)waloff[i];

        col = getclosestcol(128, 128, 0);
        for (j=0; j<(signed)sizeof(R); j++)
            R[j] *= col;

        Bmemset(newtile, 0, sx*sy);
        for (j=0; j<8; j++)
            Bmemcpy(&newtile[32*j], &R[16*j], 16);
    }
#endif

#ifdef HAVE_CLIPSHAPE_FEATURE
    int k = clipmapinfo_load();
    if (k>0)
        initprintf("There was an error loading the sprite clipping map (status %d).\n", k);

    for (i=0; i < g_clipMapFilesNum; ++i)
        Bfree(g_clipMapFiles[i]);
    DO_FREE_AND_NULL(g_clipMapFiles);
    g_clipMapFilesNum = 0;
#endif

    taglab_init();

    mkonwinvalid();

    // executed once per init
    OSD_Exec("m32_autoexec.cfg");

    if (LoadBoard(boardfilename, 1))
        reset_default_mapstate();

    totalclock = 0;

    updatesector(pos.x,pos.y,&cursectnum);

    setkeypresscallback(&m32_keypresscallback);
    M32_OnShowOSD(0);  // make sure the desktop's mouse cursor is hidden

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
            CallExtUnInit();
            uninitengine();
            Bprintf("%d * %d not supported in this graphics mode\n",xdim2d,ydim2d);
            Bexit(0);
        }

        system_getcvars();

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
            CallExtUnInit();
            uninitengine();
            Bprintf("%d * %d not supported in this graphics mode\n",xdim,ydim);
            Bexit(0);
        }

        system_getcvars();

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

        nextpage();
        synctics = totalclock-lockclock;
        lockclock += synctics;

        CallExtPreCheckKeys();

        M32_DrawRoomsAndMasks();

        inputchecked = 1;

#ifdef M32_SHOWDEBUG
        if (searchstat>=0 && (searchwall<0 || searchsector<0))
        {
            if (m32_numdebuglines<64)
                Bsprintf(m32_debugstr[m32_numdebuglines++], "inconsistent search variables!");
            searchstat = -1;
        }

        M32_drawdebug();
#endif
        CallExtCheckKeys();


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

                SaveBoard(NULL, M32_SB_ASKOV);

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


    CallExtUnInit();
//    clearfilenames();
    uninitengine();

    return 0;
}

static int32_t mhk=0;
static void loadmhk(int32_t domessage)
{
    char levname[BMAX_PATH];

    if (!mhk)
        return;

    Bstrcpy(levname, boardfilename);
    append_ext_UNSAFE(levname, ".mhk");

    if (!loadmaphack(levname))
    {
        if (domessage)
            message("Loaded map hack file \"%s\"",levname);
        else
            initprintf("Loaded map hack file \"%s\"\n",levname);
    }
    else
    {
        mhk=2;
        if (domessage)
            message("No maphack found for map \"%s\"",boardfilename);
    }
}

// this is spriteon{ceiling,ground}z from astub.c packed into
// one convenient function
void spriteoncfz(int32_t i, int32_t *czptr, int32_t *fzptr)
{
    int32_t height, zofs;

    getzsofslope(sprite[i].sectnum, sprite[i].x,sprite[i].y, czptr, fzptr);
    if ((sprite[i].cstat&48)==32)
        return;

    zofs = spriteheightofs(i, &height, 0);

    *czptr += height - zofs;
    *fzptr -= zofs;
}

static void move_and_update(int32_t xvect, int32_t yvect, int32_t addshr)
{
    if (m32_clipping==0)
    {
        pos.x += xvect>>(14+addshr);
        pos.y += yvect>>(14+addshr);
        updatesector(pos.x,pos.y, &cursectnum);
    }
    else
    {
        clipmove(&pos,&cursectnum, xvect>>addshr,yvect>>addshr,
                 128,4<<8,4<<8, (m32_clipping==1) ? 0 : CLIPMASK0);
    }

    if (in3dmode())
    {
        silentmessage("x:%d y:%d z:%d ang:%d horiz:%d", pos.x, pos.y, pos.z, ang, horiz);
        getmessagetimeoff = totalclock+30;
    }
}

static void mainloop_move(void)
{
    int32_t xvect, yvect, doubvel;

    if (angvel != 0)  //ang += angvel * constant
    {
        if (eitherCTRL && m32_2d3dmode)
        {
            int x = m32_2d3d.x + (angvel / 32);
            int xx = m32_2d3d.x + XSIZE_2D3D + (angvel / 32);

            if (x > 4 && xx < xdim2d - 4)
            {
                silentmessage("2d3d x:%d y:%d", m32_2d3d.x, m32_2d3d.y);
                m32_2d3d.x += (angvel / 32);
            }
        }
        else
        {
            //ENGINE calculates angvel for you

            //Lt. shift makes turn velocity 50% faster
            doubvel = (synctics + DOWN_BK(RUN)*(synctics>>1));

            ang += ((angvel*doubvel)>>4);
            ang &= 2047;

            if (in3dmode())
            {
                silentmessage("x:%d y:%d z:%d ang:%d horiz:%d", pos.x, pos.y, pos.z, ang, horiz);
                getmessagetimeoff = totalclock+30;
            }
        }
    }
    if ((vel|svel) != 0)
    {
        if (eitherCTRL && m32_2d3dmode)
        {
            int y = m32_2d3d.y - (vel / 64);
            int yy = m32_2d3d.y + YSIZE_2D3D - (vel / 64);

            if (y > 4 && yy < ydim2d - STATUS2DSIZ2 - 4)
            {
                silentmessage("2d3d x:%d y:%d", m32_2d3d.x, m32_2d3d.y);
                m32_2d3d.y -= (vel / 64);
            }
        }
        else

        {
            //Lt. shift doubles forward velocity
            doubvel = (1+(DOWN_BK(RUN)))*synctics;

            xvect = 0;
            yvect = 0;

            if (vel != 0)
            {
                xvect += ((vel*doubvel)>>3)*(int32_t) sintable[(ang+2560)&2047];
                yvect += ((vel*doubvel)>>3)*(int32_t) sintable[(ang+2048)&2047];
            }
            if (svel != 0)
            {
                xvect += ((svel*doubvel)>>3)*(int32_t) sintable[(ang+2048)&2047];
                yvect += ((svel*doubvel)>>3)*(int32_t) sintable[(ang+1536)&2047];
            }

            move_and_update(xvect, yvect, 0);
        }
    }
}

static void handle_sprite_in_clipboard(int32_t i)
{
    if (somethingintab == 3)
    {
        int32_t j, k;

        sprite[i].picnum = temppicnum;
        if (tilesiz[temppicnum].x <= 0 || tilesiz[temppicnum].y <= 0)
        {
            j = 0;
            for (k=0; k<MAXTILES; k++)
                if (tilesiz[k].x > 0 && tilesiz[k].y > 0)
                {
                    j = k;
                    break;
                }
            sprite[i].picnum = j;
        }
        sprite[i].shade = tempshade;
        sprite[i].blend = tempblend;
        sprite[i].pal = temppal;
        sprite[i].xrepeat = max(tempxrepeat, 1);
        sprite[i].yrepeat = max(tempyrepeat, 1);
        sprite[i].cstat = tempcstat;
    }
}


void editinput(void)
{
    int32_t mousz, bstatus;
    int32_t i, tempint=0;
    int32_t goalz, xvect, yvect, hiz, loz, oposz;
    int32_t hihit, lohit, omlook=mlook;

// 3B  3C  3D  3E   3F  40  41  42   43  44  57  58          46
// F1  F2  F3  F4   F5  F6  F7  F8   F9 F10 F11 F12        SCROLL

    mousz = 0;
    getmousevalues(&mousx,&mousy,&bstatus);
    mousx = (mousx<<16) + mousexsurp;
    mousy = (mousy<<16) + mouseysurp;

    if (unrealedlook && !mskip)
    {
        if (mlook==0 && (bstatus&(1|2|4))==2)
            mlook = 3;
        else if ((bstatus&(1|2|4))==1)
            mlook = 3;
    }

    {
        ldiv_t ld;
        if (mlook)
        {
            ld = ldiv(mousx, (int32_t)((1<<16)/(msens*0.5f))); mousx = ld.quot; mousexsurp = ld.rem;
            ld = ldiv(mousy, (int32_t)((1<<16)/(msens*0.25f))); mousy = ld.quot; mouseysurp = ld.rem;
        }
        else
        {
            ld = ldiv(mousx, (int32_t)((1<<16)/msens)); mousx = ld.quot; mousexsurp = ld.rem;
            ld = ldiv(mousy, (int32_t)((1<<16)/msens)); mousy = ld.quot; mouseysurp = ld.rem;
        }
    }

    if (mlook == 3)
        mlook = omlook;

    // UnrealEd:
    // rmb: mouselook
    // lbm: x:turn y:fwd/back local x
    // lmb&rmb: x:strafe y:up/dn (move in local yz plane)
    // mmb: fwd/back in viewing vector

    if (unrealedlook && !mskip)    //PK
    {
        if ((bstatus&(1|2|4))==1)
        {
            ang += mousx;
            xvect = -((mousy*(int32_t)sintable[(ang+2560)&2047])<<(3+pk_uedaccel));
            yvect = -((mousy*(int32_t)sintable[(ang+2048)&2047])<<(3+pk_uedaccel));

            move_and_update(xvect, yvect, 0);
        }
        else if (!mlook && (bstatus&(1|2|4))==2)
        {
            mlook=2;
        }
        else if ((bstatus&(1|2|4))==(1|2))
        {
            zmode = 2;
            xvect = -((mousx*(int32_t)sintable[(ang+2048)&2047])<<pk_uedaccel);
            yvect = -((mousx*(int32_t)sintable[(ang+1536)&2047])<<pk_uedaccel);
            pos.z += mousy<<(4+pk_uedaccel);

            move_and_update(xvect, yvect, 0);
        }
        else if ((bstatus&(1|2|4))==4)
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

            pos.z += mousy*(((int32_t)sintable[(tempint+2048)&2047])>>(10-pk_uedaccel));

            move_and_update(xvect, yvect, 2);
        }
    }

    if (mskip)
    {
        // mskip was set in astub.c to not trigger UEd mouse movements.
        // Reset now.
        mskip = 0;
    }
    else
    {
        if (mlook && (unrealedlook==0 || (bstatus&(1|4))==0))
        {
            ang += mousx;
            horiz -= mousy;

            /*
            if (mousy && !(mousy/4))
                horiz--;
            if (mousx && !(mousx/2))
                ang++;
            */

            inpclamp(&horiz, -99, 299);

            if (mlook == 1)
            {
                searchx = xdim>>1;
                searchy = ydim>>1;
            }
            osearchx = searchx-mousx;
            osearchy = searchy-mousy;

            if (mousx || mousy)
            {
                silentmessage("x:%d y:%d z:%d ang:%d horiz:%d", pos.x, pos.y, pos.z, ang, horiz);
                getmessagetimeoff = totalclock+30;
            }
        }
        else if (unrealedlook==0 || (bstatus&(1|2|4))==0)
        {
            osearchx = searchx;
            osearchy = searchy;
            searchx += mousx;
            searchy += mousy;

            inpclamp(&searchx, 12, xdim-13);
            inpclamp(&searchy, 12, ydim-13);
        }
    }

//    showmouse();

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

    mainloop_move();

    getzrange(&pos,cursectnum, &hiz,&hihit, &loz,&lohit, 128, (m32_clipping==1)?0:CLIPMASK0);
/*
{
    int32_t his = !(hihit&32768), los = !(lohit&32768);
    if (m32_numdebuglines<64)
        Bsprintf(m32_debugstr[m32_numdebuglines++], "s%d: cf[%s%d, %s%d] z(%d, %d)", cursectnum,
                 his?"s":"w",hihit&16383, los?"s":"w",lohit&16383, hiz,loz);
}
*/
    oposz = pos.z;
    if (zmode == 0)
    {
        goalz = loz-(kensplayerheight<<8);  //playerheight pixels above floor
        if (goalz < hiz+(16<<8))  //ceiling&floor too close
            goalz = (loz+hiz)>>1;
        goalz += mousz;

        if (DOWN_BK(MOVEUP))  //A (stand high)
        {
            goalz -= (16<<8);
            if (DOWN_BK(RUN))
                goalz -= (24<<8);
        }
        if (DOWN_BK(MOVEDOWN))  //Z (stand low)
        {
            goalz += (12<<8);
            if (DOWN_BK(RUN))
                goalz += (12<<8);
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

        if (m32_clipping)
            inpclamp(&goalz, hiz+(4<<8), loz-(4<<8));

        if (zmode == 1) goalz = loz-zlock;
        if (m32_clipping && (goalz < hiz+(4<<8)))
            goalz = ((loz+hiz)>>1);  //ceiling&floor too close
        if (zmode == 1) pos.z = goalz;

        if (goalz != pos.z)
        {
            //if (pos.z < goalz) hvel += (32<<DOWN_BK(RUN));
            //if (pos.z > goalz) hvel -= (32<<DOWN_BK(RUN));
            if (pos.z < goalz)
                hvel = ((192*synctics)<<DOWN_BK(RUN));
            else
                hvel = -((192*synctics)<<DOWN_BK(RUN));

            pos.z += hvel;

            if (m32_clipping)
            {
                if (pos.z > loz-(4<<8)) pos.z = loz-(4<<8), hvel = 0;
                if (pos.z < hiz+(4<<8)) pos.z = hiz+(4<<8), hvel = 0;
            }
        }
        else
            hvel = 0;
    }

    {
        int16_t ocursectnum = cursectnum;
        updatesectorz(pos.x,pos.y,pos.z, &cursectnum);
        if (cursectnum<0)
        {
            if (zmode != 2)
                pos.z = oposz;  // don't allow to fall into infinity when in void space
            cursectnum = ocursectnum;
        }
    }

    if (pos.z != oposz && in3dmode())
    {
        silentmessage("x:%d y:%d z:%d ang:%d horiz:%d", pos.x, pos.y, pos.z, ang, horiz);
        getmessagetimeoff = totalclock+30;
    }

    searchit = 2;
    if (searchstat >= 0)
    {
        if ((bstatus&(1|2|4)) || keystatus[0x39])  // SPACE
            searchit = 0;

        if (keystatus[0x1f])  //S (insert sprite) (3D)
        {
            hitdata_t hit;

            vec2_t da = { 16384, divscale14(searchx-(xdim>>1), xdim>>1) };

            rotatepoint(zerovec, da, ang, &da);

#ifdef USE_OPENGL
            if (getrendermode() == REND_POLYMOST)
                hit = polymost_hitdata;
            else
#endif
                hitscan((const vec3_t *)&pos,cursectnum,              //Start position
                    da.x,da.y,(scale(searchy,200,ydim)-horiz)*2000, //vector of 3D ang
                    &hit,CLIPMASK1);

            if (hit.sect >= 0)
            {
                da.x = hit.pos.x;
                da.y = hit.pos.y;
                if (gridlock && grid > 0)
                {
                    if (AIMING_AT_WALL || AIMING_AT_MASKWALL)
                        hit.pos.z &= 0xfffffc00;
                    else
                        locktogrid(&da.x, &da.y);
                }

                i = insert_sprite_common(hit.sect, da.x, da.y);

                if (i < 0)
                    message("Couldn't insert sprite.");
                else
                {
                    int32_t cz, fz;

                    handle_sprite_in_clipboard(i);

                    spriteoncfz(i, &cz, &fz);
                    sprite[i].z = clamp2(hit.pos.z, cz, fz);

                    if (AIMING_AT_WALL || AIMING_AT_MASKWALL)
                    {
                        sprite[i].cstat &= ~48;
                        sprite[i].cstat |= (16+64);

                        correct_ornamented_sprite(i, hit.wall);
                    }
                    else
                        sprite[i].cstat |= (tilesiz[sprite[i].picnum].y>=32);

                    correct_sprite_yoffset(i);

                    asksave = 1;

                    VM_OnEvent(EVENT_INSERTSPRITE3D, i);
                }
            }

            keystatus[0x1f] = 0;
        }

        if (keystatus[0x3f]||keystatus[0x40])  //F5,F6
        {
            switch (searchstat)
            {
            case SEARCH_CEILING:
            case SEARCH_FLOOR:
                CallExtShowSectorData(searchsector); break;
            case SEARCH_WALL:
            case SEARCH_MASKWALL:
                CallExtShowWallData(searchwall); break;
            case SEARCH_SPRITE:
                CallExtShowSpriteData(searchwall); break;
            }

            keystatus[0x3f] = keystatus[0x40] = 0;
        }
        if (keystatus[0x41]||keystatus[0x42])  //F7,F8
        {
            switch (searchstat)
            {
            case SEARCH_CEILING:
            case SEARCH_FLOOR:
                CallExtEditSectorData(searchsector); break;
            case SEARCH_WALL:
            case SEARCH_MASKWALL:
                CallExtEditWallData(searchwall); break;
            case SEARCH_SPRITE:
                CallExtEditSpriteData(searchwall); break;
            }

            keystatus[0x41] = keystatus[0x42] = 0;
        }

    }

    if (keystatus[buildkeys[BK_MODE2D_3D]] && !m32_is2d3dmode())  // Enter
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
    return dachar;
}


////////////////////// OVERHEADEDITOR //////////////////////

// some 2d mode state
static struct overheadstate
{
    // number of backed up drawn walls
    int32_t bak_wallsdrawn;

    // state related to line drawing
    int16_t suckwall, split;
    int16_t splitsect;
    int16_t splitstartwall;
} ovh;


static int32_t inside_editor(const vec3_t *pos, int32_t searchx, int32_t searchy, int32_t zoom,
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

int32_t inside_editor_curpos(int16_t sectnum)
{
    // TODO: take care: mous[xy]plc global vs overheadeditor auto
    return inside_editor(&pos, searchx,searchy, zoom, mousxplc,mousyplc, sectnum);
}


static inline void drawline16base(int32_t bx, int32_t by, int32_t x1, int32_t y1, int32_t x2, int32_t y2, char col)
{
    drawline16(bx+x1, by+y1, bx+x2, by+y2, col);
}

void drawsmallabel(const char *text, char col, char backcol, char border, int32_t dax, int32_t day, int32_t daz)
{
    screencoords(&dax,&day, dax-pos.x,day-pos.y, zoom);

    if (m32_sideview)
        day += getscreenvdisp(daz-pos.z, zoom);

    int32_t const x1 = halfxdim16+dax-(Bstrlen(text)<<1);
    int32_t const y1 = midydim16+day-4;
    int32_t const x2 = x1 + (Bstrlen(text)<<2)+2;
    int32_t const y2 = y1 + 7;

    int f = mulscale8(x2-x1, zoom);

    if ((x1 <= -f) || (x2 >= xdim + f) || (y1 <= -f) || (y2 >= ydim16 + f))
        return;

    printext16(x1,y1, col,backcol, text,1);

    drawline16(x1-2, y1-2, x2-2, y1-2, border);
    drawline16(x1-2, y2+1, x2-2, y2+1, border);

    drawline16(x1-3, y1-1, x1-3, y2+0, border);
    drawline16(x2-1, y1-1, x2-1, y2+0, border);

    drawline16(x1-1,y1-1, x2-3,y1-1, backcol);
    drawline16(x1-1,y2+0, x2-3,y2+0, backcol);

    drawline16(x1-2,y1+0, x1-2,y2-1, backcol);
    drawline16(x2-2,y1+0, x2-2,y2-1, backcol);
    drawline16(x2-3,y1+0, x2-3,y2+0, backcol);

    begindrawing(); //{{{

    if ((unsigned)y1-1 < ydim16+0u && (unsigned) (x1-2) < xdim2d+0u && (unsigned) (x2-2) < xdim2d+0u)
    {
        drawpixel((char *) (frameplace + ((y1-1) * bytesperline) + (x1-2)), border);
        drawpixel((char *) (frameplace + ((y1-1) * bytesperline) + (x2-2)), border);
    }

    if ((unsigned) y2 < ydim16+0u && (unsigned) (x1-2) < xdim2d+0u && (unsigned) (x2-2) < xdim2d+0u)
    {
        drawpixel((char *) (frameplace + ((y2) * bytesperline) + (x1-2)), border);
        drawpixel((char *) (frameplace + ((y2) * bytesperline) + (x2-2)), border);
    }

    enddrawing();
}

// backup highlighted sectors with sprites as mapinfo for later restoration
// return values:
//  -1: highlightsectorcnt<=0
//   0: ok
static int32_t backup_highlighted_map(mapinfofull_t *mapinfo)
{
    int32_t i, j, k, m, tmpnumwalls=0, tmpnumsprites=0;
    int16_t *const otonsect = (int16_t *)tempxyar;  // STRICTALIASING
    int16_t *const otonwall = ((int16_t *)tempxyar) + MAXWALLS;
#ifdef YAX_ENABLE
    int16_t otonbunch[YAX_MAXBUNCHES];
    int16_t numsectsofbunch[YAX_MAXBUNCHES];  // ceilings + floors
#endif

    if (highlightsectorcnt <= 0)
        return -1;

#ifdef YAX_ENABLE
    for (i=0; i<numyaxbunches; i++)
        numsectsofbunch[i] = 0;
#endif

    // set up old-->new mappings
    j = 0;
    k = 0;
    for (i=0; i<numsectors; i++)
    {
        int32_t startwall, endwall;

        if (hlsectorbitmap[i>>3]&(1<<(i&7)))
        {
#ifdef YAX_ENABLE
            int16_t bn[2], cf;

            yax_getbunches(i, &bn[0], &bn[1]);
            for (cf=0; cf<2; cf++)
                if (bn[cf] >= 0)
                    numsectsofbunch[bn[cf]]++;
#endif
            otonsect[i] = j++;

            for (WALLS_OF_SECTOR(i, m))
                otonwall[m] = k++;
        }
        else
        {
            otonsect[i] = -1;

            for (WALLS_OF_SECTOR(i, m))
                otonwall[m] = -1;
        }
    }

#ifdef YAX_ENABLE
    j = 0;
    for (i=0; i<numyaxbunches; i++)
    {
        // only back up complete bunches
        if (numsectsofbunch[i] == yax_numsectsinbunch(i, 0)+yax_numsectsinbunch(i, 1))
            otonbunch[i] = j++;  // kept bunch
        else
            otonbunch[i] = -1;  // discarded bunch
    }
    mapinfo->numyaxbunches = j;
#endif

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
    mapinfo->sector = (usectortype *)Xmalloc(highlightsectorcnt * sizeof(sectortype));
    mapinfo->wall = (uwalltype *)Xmalloc(tmpnumwalls * sizeof(walltype));

#ifdef YAX_ENABLE
    if (mapinfo->numyaxbunches > 0)
    {
        mapinfo->bunchnum = (int16_t *)Xmalloc(highlightsectorcnt*2*sizeof(int16_t));
        mapinfo->ynextwall = (int16_t *)Xmalloc(tmpnumwalls*2*sizeof(int16_t));
    }
    else
    {
        mapinfo->bunchnum = mapinfo->ynextwall = NULL;
    }
#endif

    if (tmpnumsprites>0)
    {
        mapinfo->sprite = (uspritetype *)Xmalloc(tmpnumsprites * sizeof(spritetype));
    }
    else
    {
        // would never be accessed because mapinfo->numsprites is 0, but cleaner
        mapinfo->sprite = NULL;
    }


    // copy everything over
    tmpnumwalls = 0;
    tmpnumsprites = 0;
    for (i=0; i<highlightsectorcnt; i++)
    {
        k = highlightsector[i];
        Bmemcpy(&mapinfo->sector[i], &sector[k], sizeof(sectortype));
        mapinfo->sector[i].wallptr = tmpnumwalls;

#ifdef YAX_ENABLE
        if (mapinfo->numyaxbunches > 0 || numyaxbunches > 0)
        {
            int16_t bn[2];

            yax_getbunches(k, &bn[0], &bn[1]);
            for (j=0; j<2; j++)
            {
                // old bunchnum, new bunchnum
                int32_t obn=bn[j], nbn=(obn>=0) ? otonbunch[obn] : -1;

                if (mapinfo->numyaxbunches > 0)
                    mapinfo->bunchnum[2*i + j] = nbn;

                if (obn >= 0 && nbn < 0)
                {
                    // A bunch was discarded.
                    usectortype *const sec = &mapinfo->sector[i];
# if !defined NEW_MAP_FORMAT
                    uint16_t *const cs = j==YAX_CEILING ? &sec->ceilingstat : &sec->floorstat;
                    uint8_t *const xp = j==YAX_CEILING ? &sec->ceilingxpanning : &sec->floorxpanning;

                    *cs &= ~YAX_BIT;
                    *xp = 0;
# else
                    if (j == YAX_CEILING)
                        sec->ceilingbunch = -1;
                    else
                        sec->floorbunch = -1;
# endif
                }
            }
        }
#endif

        for (j=0; j<sector[k].wallnum; j++)
        {
            m = sector[k].wallptr;
            Bmemcpy(&mapinfo->wall[tmpnumwalls+j], &wall[m+j], sizeof(walltype));
            mapinfo->wall[tmpnumwalls+j].point2 += (tmpnumwalls-m);

#ifdef YAX_ENABLE
            if (mapinfo->numyaxbunches > 0 || numyaxbunches > 0)
            {
                int32_t cf;

                for (cf=0; cf<2; cf++)
                {
                    const int32_t ynw = yax_getnextwall(m+j, cf);
                    const int32_t nynw = (ynw >= 0) ? otonwall[ynw] : -1;

                    if (mapinfo->numyaxbunches > 0)
                        mapinfo->ynextwall[2*(tmpnumwalls+j) + cf] = nynw;

                    if (ynw >= 0 && nynw < 0)  // CLEAR_YNEXTWALLS
                        YAX_PTRNEXTWALL(mapinfo->wall, tmpnumwalls+j, cf) = YAX_NEXTWALLDEFAULT(cf);
                }
            }
#endif
            m = mapinfo->wall[tmpnumwalls+j].nextsector;
            if (m < 0 || otonsect[m] < 0)
            {
                mapinfo->wall[tmpnumwalls+j].nextsector = -1;
                mapinfo->wall[tmpnumwalls+j].nextwall = -1;
            }
            else
            {
                mapinfo->wall[tmpnumwalls+j].nextsector = otonsect[m];
                m = mapinfo->wall[tmpnumwalls+j].nextwall;
                mapinfo->wall[tmpnumwalls+j].nextwall = otonwall[m];
            }
        }
        tmpnumwalls += j;

        m = headspritesect[highlightsector[i]];
        while (m != -1)
        {
            Bmemcpy(&mapinfo->sprite[tmpnumsprites], &sprite[m], sizeof(spritetype));
            mapinfo->sprite[tmpnumsprites].sectnum = otonsect[highlightsector[i]];
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
#ifdef YAX_ENABLE
    if (mapinfo->numyaxbunches > 0)
    {
        Bfree(mapinfo->bunchnum);
        Bfree(mapinfo->ynextwall);
    }
#endif
    Bfree(mapinfo->wall);
    if (mapinfo->numsprites>0)
        Bfree(mapinfo->sprite);
}

// restore map saved with backup_highlighted_map, also
// frees mapinfo's sector, wall, (sprite) in any case.
// return values:
//  -1: limits exceeded
//   0: ok
// forreal: if 0, only test if we have enough space (same return values)
static int32_t restore_highlighted_map(mapinfofull_t *mapinfo, int32_t forreal)
{
    int32_t i, j, onumsectors=numsectors, newnumsectors, newnumwalls;

    if (numsectors+mapinfo->numsectors>MAXSECTORS || numwalls+mapinfo->numwalls>MAXWALLS
#ifdef YAX_ENABLE
            || numyaxbunches+mapinfo->numyaxbunches > YAX_MAXBUNCHES
#endif
            || Numsprites+mapinfo->numsprites>MAXSPRITES)
    {
        mapinfofull_free(mapinfo);
        return -1;
    }

    if (!forreal)
        return 0;

    newnumsectors = numsectors + mapinfo->numsectors;
    newnumwalls = numwalls + mapinfo->numwalls;

    // copy sectors & walls
    Bmemcpy(&sector[numsectors], mapinfo->sector, mapinfo->numsectors*sizeof(sectortype));
    Bmemcpy(&wall[numwalls], mapinfo->wall, mapinfo->numwalls*sizeof(walltype));

    // tweak index members
    for (i=numwalls; i<newnumwalls; i++)
    {
        wall[i].point2 += numwalls;

        if (wall[i].nextsector >= 0)
        {
            wall[i].nextsector += numsectors;
            wall[i].nextwall += numwalls;
        }
#ifdef YAX_ENABLE
        for (j=0; j<2; j++)
        {
            if (mapinfo->numyaxbunches > 0)
            {
                yax_setnextwall(i, j, mapinfo->ynextwall[2*(i-numwalls) + j]>=0 ?
                                numwalls+mapinfo->ynextwall[2*(i-numwalls) + j] : -1);
            }
            else
            {
# if !defined NEW_MAP_FORMAT
                // XXX: When copying a TROR portion into a non-TROR map (e.g. a
                // new one), tags denoting ynextwalls are left in place.
                wall[i].cstat &= ~YAX_NEXTWALLBIT(j);  // CLEAR_YNEXTWALLS
# else
                yax_setnextwall(i, j, -1);
# endif
            }
        }
#endif
    }
    for (i=numsectors; i<newnumsectors; i++)
        sector[i].wallptr += numwalls;

    // highlight copied sectors

    numsectors = newnumsectors;

    Bmemset(hlsectorbitmap, 0, sizeof(hlsectorbitmap));
    for (i=onumsectors; i<newnumsectors; i++)
    {
        hlsectorbitmap[i>>3] |= (1<<(i&7));

#ifdef YAX_ENABLE
        for (j=0; j<2; j++)
        {
            if (mapinfo->numyaxbunches > 0)
            {
                int32_t bn = mapinfo->bunchnum[2*(i-onumsectors)+j];
                yax_setbunch(i, j, bn>=0 ? numyaxbunches+bn : -2);
                // -2 clears forward yax-nextwall links.
                // XXX: still may wrongly reset xpanning.
            }
            else
                Bassert(yax_getbunch(i, j) < 0);
        }
#endif
    }

    // insert sprites
    for (i=0; i<mapinfo->numsprites; i++)
    {
        const uspritetype *srcspr = &mapinfo->sprite[i];
        int32_t sect = onumsectors + srcspr->sectnum;

        j = insertsprite(sect, srcspr->statnum);
        Bassert(j >= 0);
        Bmemcpy(&sprite[j], srcspr, sizeof(spritetype));
        sprite[j].sectnum = sect;
    }

    mapinfofull_free(mapinfo);

    numwalls = newnumwalls;

    update_highlightsector();

#ifdef YAX_ENABLE
    if (mapinfo->numyaxbunches > 0)
        yax_update(0);
#endif
    yax_updategrays(pos.z);

    return 0;
}


static int32_t newnumwalls=-1;

void ovh_whiteoutgrab(int32_t restoreredwalls)
{
    int32_t i, j, k, startwall, endwall;
#if 0
//def YAX_ENABLE
    int16_t cb, fb;
#endif

    if (restoreredwalls)
    {
        // restore onextwalls first
        for (i=0; i<numsectors; i++)
            for (WALLS_OF_SECTOR(i, j))
                checksectorpointer(j, i);
    }

    for (i=0; i<MAXWALLS; i++)
        onextwall[i] = -1;

    //White out all bordering lines of grab that are
    //not highlighted on both sides
    for (i=highlightsectorcnt-1; i>=0; i--)
        for (WALLS_OF_SECTOR(highlightsector[i], j))
        {
            if (wall[j].nextwall < 0)
                continue;

            k = wall[j].nextsector;

            if (hlsectorbitmap[k>>3]&(1<<(k&7)))
                continue;
#if 0
//def YAX_ENABLE
            // internal red walls are kept red
            yax_getbunches(highlightsector[i], &cb, &fb);
            if (cb>=0 && yax_getbunch(k, YAX_CEILING)>=0)
                continue;
            if (fb>=0 && yax_getbunch(k, YAX_FLOOR)>=0)
                continue;
#endif
            onextwall[j] = wall[j].nextwall;

            NEXTWALL(j).nextwall = -1;
            NEXTWALL(j).nextsector = -1;
            wall[j].nextwall = -1;
            wall[j].nextsector = -1;
        }

    if (highlightsectorcnt > 0)
        mkonwvalid();
    else
        mkonwinvalid_keeptempsect();
}

static void duplicate_selected_sectors(void)
{
    mapinfofull_t mapinfo;
    int32_t i, j, onumsectors;
#ifdef YAX_ENABLE
    int32_t onumyaxbunches;
#endif
    int32_t minx=INT32_MAX, maxx=INT32_MIN, miny=INT32_MAX, maxy=INT32_MIN, dx, dy;

    i = backup_highlighted_map(&mapinfo);

    if (i < 0)
    {
        message("Out of memory!");
        return;
    }

    i = restore_highlighted_map(&mapinfo, 0);
    if (i < 0)
    {
        // XXX: no, might be another limit too.  Better message needed.
        printmessage16("Copying sectors would exceed sector or wall limit.");
        return;
    }

    // restoring would succeed, tweak things...
    Bmemset(hlsectorbitmap, 0, sizeof(hlsectorbitmap));
    for (i=0; i<highlightsectorcnt; i++)
    {
        int32_t startwall, endwall;

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
    }

    // displace walls & sprites of new sectors by a small amount:
    // calculate displacement
    if (grid>0 && grid<9)
        dx = max(2048>>grid, 128);
    else
        dx = 512;
    dy = -dx;
    if (maxx+dx >= editorgridextent) dx*=-1;
    if (minx+dx <= -editorgridextent) dx*=-1;
    if (maxy+dy >= editorgridextent) dy*=-1;
    if (miny+dy <= -editorgridextent) dy*=-1;

    onumsectors = numsectors;
#ifdef YAX_ENABLE
    onumyaxbunches = numyaxbunches;
#endif
    // restore! this will not fail.
    restore_highlighted_map(&mapinfo, 1);

    // displace
    for (i=onumsectors; i<numsectors; i++)
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

#ifdef YAX_ENABLE
    if (numyaxbunches > onumyaxbunches)
        printmessage16("Sectors duplicated, creating %d new bunches.", numyaxbunches-onumyaxbunches);
    else
#endif
        printmessage16("Sectors duplicated.");
    asksave = 1;

#ifdef YAX_ENABLE
    if (numyaxbunches > onumyaxbunches)
        yax_update(0);
#endif
    yax_updategrays(pos.z);
}


static void duplicate_selected_sprites(void)
{
    int32_t i, j, k=0;

    for (i=0; i<highlightcnt; i++)
        if ((highlight[i]&0xc000) == 16384)
            k++;

    if (Numsprites + k <= MAXSPRITES)
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

        printmessage16("Sprites duplicated.");
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
    {
        sprite[i].ang = (getangle(POINT2(hitw).x-wall[hitw].x,
                                  POINT2(hitw).y-wall[hitw].y)+512)&2047;

        //Make sure sprite's in right sector
        if (inside(sprite[i].x, sprite[i].y, sprite[i].sectnum) != 1)
        {
            j = wall[hitw].point2;
            sprite[i].x -= ksgn(wall[j].y-wall[hitw].y);
            sprite[i].y += ksgn(wall[j].x-wall[hitw].x);
        }
    }
}

void DoSpriteOrnament(int32_t i)
{
    hitdata_t hit;

    hitscan((const vec3_t *)&sprite[i],sprite[i].sectnum,
            sintable[(sprite[i].ang+1536)&2047],
            sintable[(sprite[i].ang+1024)&2047],
            0,
            &hit,CLIPMASK1);

    if (hit.sect == -1)
        return;

    sprite[i].x = hit.pos.x;
    sprite[i].y = hit.pos.y;
    sprite[i].z = hit.pos.z;
    changespritesect(i, hit.sect);

    correct_ornamented_sprite(i, hit.wall);
}

void update_highlight(void)
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

void update_highlightsector(void)
{
    int32_t i;

    minhlsectorfloorz = INT32_MAX;
    numhlsecwalls = 0;

    highlightsectorcnt = 0;
    for (i=0; i<numsectors; i++)
        if (hlsectorbitmap[i>>3]&(1<<(i&7)))
        {
            highlightsector[highlightsectorcnt++] = i;
            minhlsectorfloorz = min(minhlsectorfloorz, sector[i].floorz);
            numhlsecwalls += sector[i].wallnum;
        }

    if (highlightsectorcnt==0)
    {
        minhlsectorfloorz = 0;
        highlightsectorcnt = -1;
    }
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

static int32_t insert_sprite_common(int32_t sectnum, int32_t dax, int32_t day)
{
    int32_t i, j, k;

    i = insertsprite(sectnum,0);
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
        if (sprite[k].statnum < MAXSTATUS && k!=i)
            localartfreq[sprite[k].picnum]++;

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

void correct_sprite_yoffset(int32_t i)
{
    int32_t tileyofs = picanm[sprite[i].picnum].yofs;
    int32_t tileysiz = tilesiz[sprite[i].picnum].y;

    if (klabs(tileyofs) >= tileysiz)
    {
        tileyofs *= -1;
        if (tileyofs == 128)
            tileyofs = 127;

        sprite[i].yoffset = tileyofs;
    }
    else
        sprite[i].yoffset = 0;
}

// keepcol >= 0 && <256: keep that idx-color
// keepcol < 0: keep none
// keepcol >= 256: 0x00ffffff is mask for 3 colors
void fade_editor_screen(int32_t keepcol)
{
    char blackcol=0, greycol=whitecol-25, *cp;
    int32_t pix, i, threecols = (keepcol >= 256);
    char cols[3] = {(char)(keepcol&0xff), (char)((keepcol>>8)&0xff), (char)((keepcol>>16)&0xff)};

    begindrawing();
    cp = (char *)frameplace;
    for (i=0; i<bytesperline*(ydim-STATUS2DSIZ2); i++, cp++)
    {
        pix = (uint8_t)(*cp);

        if (!threecols && pix == keepcol)
            continue;
        if (threecols)
            if (pix==cols[0] || pix==cols[1] || pix==cols[2])
                continue;

        if (*cp==greycol)
            *cp = blackcol;
        else if (*cp != blackcol)
            *cp = greycol;
    }
    enddrawing();
    showframe(1);
}

static void copy_some_wall_members(int16_t dst, int16_t src, int32_t reset_some)
{
    static uwalltype nullwall;
    walltype * const dstwal = &wall[dst];
    const uwalltype *srcwal = src >= 0 ? (uwalltype *)&wall[src] : &nullwall;

    memset(&nullwall, 0, sizeof(nullwall));
    nullwall.yrepeat = 8;
    nullwall.extra = -1;

    if (reset_some)
    {
        dstwal->cstat = srcwal->cstat;
    }
    else
    {
        dstwal->cstat &= ~(4+8+256);
        dstwal->cstat |= (srcwal->cstat&(4+8+256));
    }
    dstwal->shade = srcwal->shade;
    dstwal->yrepeat = srcwal->yrepeat;
    fixrepeats(dst);  // xrepeat
    dstwal->picnum = srcwal->picnum;
    dstwal->overpicnum = srcwal->overpicnum;

    dstwal->pal = srcwal->pal;
    dstwal->xpanning = srcwal->xpanning;
    dstwal->ypanning = srcwal->ypanning;

    if (reset_some)
    {
        dstwal->nextwall = -1;
        dstwal->nextsector = -1;

        dstwal->lotag = 0; //srcwal->lotag;
        dstwal->hitag = 0; //srcwal->hitag;
        dstwal->extra = -1; //srcwal->extra;
#ifdef YAX_ENABLE
        yax_setnextwall(dst, YAX_CEILING, -1);
        yax_setnextwall(dst, YAX_FLOOR, -1);
#endif
    }
}

static void init_new_wall1(int16_t *suckwall_ret, int32_t mousxplc, int32_t mousyplc)
{
    int32_t i;

    Bmemset(&wall[newnumwalls], 0, sizeof(walltype));
    wall[newnumwalls].extra = -1;

    wall[newnumwalls].x = mousxplc;
    wall[newnumwalls].y = mousyplc;
    wall[newnumwalls].nextsector = -1;
    wall[newnumwalls].nextwall = -1;

    for (i=0; i<numwalls; i++)
    {
        YAX_SKIPWALL(i);
        if (wall[i].nextwall >= 0)
            YAX_SKIPWALL(wall[i].nextwall);

        if (wall[i].x == mousxplc && wall[i].y == mousyplc)
            *suckwall_ret = i;
    }

    wall[newnumwalls].point2 = newnumwalls+1;
    newnumwalls++;
}

// helpers for often needed ops:
static int32_t do_while_copyloop1(int16_t startwall, int16_t endwall,
                                  int16_t *danumwalls, int16_t lastpoint2)
{
    int32_t m = startwall;

    do
    {
        if (*danumwalls >= MAXWALLS + M32_FIXME_WALLS)
            return 1;

        Bmemcpy(&wall[*danumwalls], &wall[m], sizeof(walltype));
        wall[*danumwalls].point2 = *danumwalls+1;
        (*danumwalls)++;
        m = wall[m].point2;
    }
    while (m != endwall);

    if (lastpoint2 >= 0)
        wall[(*danumwalls)-1].point2 = lastpoint2;

    return 0;
}

static void updatesprite1(int16_t i)
{
    setsprite(i, (vec3_t *)&sprite[i]);

    if (sprite[i].sectnum>=0)
    {
        int32_t cz, fz;
        spriteoncfz(i, &cz, &fz);
        inpclamp(&sprite[i].z, cz, fz);
    }
}

#ifdef YAX_ENABLE
// highlighted OR grayed-out sectors:
static uint8_t hlorgraysectbitmap[MAXSECTORS>>3];
static int32_t ask_above_or_below(void);
#else
# define hlorgraysectbitmap hlsectorbitmap
#endif

// returns:
//  0: continue
// >0: newnumwalls
// <0: error
// ignore_ret and refsect_ret are for the 'auto-red-wall' feature
static int32_t trace_loop(int32_t j, uint8_t *visitedwall, int16_t *ignore_ret, int16_t *refsect_ret,
                          int16_t trace_loop_yaxcf)
{
    int16_t refsect, ignore;
    int32_t k, n, refwall;
#if 0
//def YAX_ENABLE
    int32_t yaxp = (ignore_ret==NULL);  // bleh
#else
    UNREFERENCED_PARAMETER(trace_loop_yaxcf);
#endif

    if (wall[j].nextwall>=0 || (visitedwall[j>>3]&(1<<(j&7))))
        return 0;

    n=2*MAXWALLS;  // simple inf loop check
    refwall = j;
    k = numwalls;

    ignore = 0;

    if (ignore_ret)
    {
        refsect = -1;
        updatesectorexclude(wall[j].x, wall[j].y, &refsect, hlorgraysectbitmap);
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

            if (ignore_ret)  // auto-red wall feature
                onextwall[k] = onextwall[j];

            Bmemcpy(&wall[k], &wall[j], sizeof(walltype));
            wall[k].point2 = k+1;
// TODO: protect lotag/extra; see also hl-sector copying stuff
            wall[k].nextsector = wall[k].nextwall = wall[k].extra = -1;
#ifdef YAX_ENABLE
            if (trace_loop_yaxcf >= 0)
                yax_setnextwall(k, trace_loop_yaxcf, j);
#endif
            k++;
        }

        j = wall[j].point2;
        n--;

        while (wall[j].nextwall>=0 && n>0)
        {
#if 0
//def YAX_ENABLE
            if (yaxp)
            {
                int32_t ns = wall[j].nextsector;
                if ((hlsectorbitmap[ns>>3]&(1<<(ns&7)))==0)
                    break;
            }
#endif
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

// Backup drawn walls for carrying out other operations in the middle.
//  0: back up, set newnumwalls to -1
//  1: restore drawn walls and free mem
//  2: only free memory needed for backing up walls but don't restore walls
//     (use this if the map has been mangled too much for a safe restoration)
// Context that needs special treatment: suckwall, splitsect, splitstartwall
static int32_t backup_drawn_walls(int32_t restore)
{
    static uwalltype *tmpwall;

    // back up
    if (restore==0)
    {
        // ovh.bak_wallsdrawn should be 0 here

        if (newnumwalls != -1)
        {
            if (newnumwalls <= numwalls)  // shouldn't happen
                return 2;

            Bfree(tmpwall);
            tmpwall = (uwalltype *)Xmalloc((newnumwalls-numwalls) * sizeof(walltype));

            ovh.bak_wallsdrawn = newnumwalls-numwalls;

            Bmemcpy(tmpwall, &wall[numwalls], ovh.bak_wallsdrawn*sizeof(walltype));
            newnumwalls = -1;
        }

        return 0;
    }

    // restore/clear
    if (tmpwall)
    {
        if (restore==1)  // really restore
        {
            const int32_t nnumwalls = numwalls + ovh.bak_wallsdrawn;

            if (nnumwalls < MAXWALLS)  // else, silently discard drawn walls
            {
                int32_t i;

                Bmemcpy(&wall[numwalls], tmpwall, ovh.bak_wallsdrawn*sizeof(walltype));

                newnumwalls = nnumwalls;
                for (i=numwalls; i<newnumwalls; i++)
                    wall[i].point2 = i+1;
            }
        }

        DO_FREE_AND_NULL(tmpwall);

        ovh.bak_wallsdrawn = 0;
    }

    return 0;
}

// VARIOUS RESETTING FUNCTIONS
#define RESET_EDITOR_VARS() do { \
    sectorhighlightstat = -1; \
    newnumwalls = -1; \
    joinsector[0] = -1; \
    circlewall = -1; \
    circlepoints = 7; \
    } while (0)

void reset_highlightsector(void)
{
    Bmemset(hlsectorbitmap, 0, sizeof(hlsectorbitmap));
    update_highlightsector();
}

void reset_highlight(void)  // walls and sprites
{
    Bmemset(show2dwall, 0, sizeof(show2dwall));
    Bmemset(show2dsprite, 0, sizeof(show2dsprite));
    update_highlight();
}

#ifdef YAX_ENABLE
static int32_t collnumsects[2];
static int16_t collsectlist[2][MAXSECTORS];
static uint8_t collsectbitmap[2][MAXSECTORS>>3];

static void collect_sectors1(int16_t *sectlist, uint8_t *sectbitmap, int32_t *numsectptr,
                             int16_t startsec, int32_t alsoyaxnext, int32_t alsoonw)
{
    int32_t j, startwall, endwall, sectcnt;

    bfirst_search_init(sectlist, sectbitmap, numsectptr, MAXSECTORS, startsec);

    for (sectcnt=0; sectcnt<*numsectptr; sectcnt++)
    {
        for (WALLS_OF_SECTOR(sectlist[sectcnt], j))
        {
            if (wall[j].nextsector >= 0)
                bfirst_search_try(sectlist, sectbitmap, numsectptr, wall[j].nextsector);
            else if (alsoonw && onextwall[j]>=0)
                bfirst_search_try(sectlist, sectbitmap, numsectptr, sectorofwall(onextwall[j]));
        }

        if (alsoyaxnext)
        {
            int16_t bn[2], cf;
            yax_getbunches(sectlist[sectcnt], &bn[0], &bn[1]);
            for (cf=0; cf<2; cf++)
                if (bn[cf]>=0)
                {
                    for (SECTORS_OF_BUNCH(bn[cf], !cf, j))
                        bfirst_search_try(sectlist, sectbitmap, numsectptr, j);
                }
        }
    }
}


static int32_t sectors_components(int16_t hlsectcnt, const int16_t *hlsectors, int32_t alsoyaxnext, int32_t alsoonw);
static int32_t highlighted_sectors_components(int32_t alsoyaxnext, int32_t alsoonw)
{
    return sectors_components(highlightsectorcnt, highlightsector, alsoyaxnext, alsoonw);
}

// whether all highlighted sectors are in one (returns 1), two (2)
// or more (>2) connected components wrt the nextsector relation
// -1 means error
//  alsoyaxnext: also consider "yax-nextsector" relation
//  alsoonw: also consider "old-nextwall" relation (must be valid)
static int32_t sectors_components(int16_t hlsectcnt, const int16_t *hlsector, int32_t alsoyaxnext, int32_t alsoonw)
{
    int32_t j, k, tmp;

    if (hlsectcnt<1)
        return 0;

    collect_sectors1(collsectlist[0], collsectbitmap[0], &collnumsects[0],
                     hlsector[0], alsoyaxnext, alsoonw);

    for (k=1; k<hlsectcnt; k++)
    {
        j = hlsector[k];
        if ((collsectbitmap[0][j>>3]&(1<<(j&7)))==0)
        {
            // sector j not collected --> more than 1 conn. comp.
            collect_sectors1(collsectlist[1], collsectbitmap[1], &collnumsects[1],
                             j, alsoyaxnext, alsoonw);
            break;
        }
    }

    if (k == hlsectcnt)
        return 1;

    for (k=0; k<hlsectcnt; k++)
    {
        j = hlsector[k];
        tmp = (((collsectbitmap[0][j>>3]&(1<<(j&7)))!=0) + (((collsectbitmap[1][j>>3]&(1<<(j&7)))!=0)<<1));

        if (tmp==3)
            return -1;  // components only weakly connected

        if (tmp==0)
            return 3;  // sector j not reached
    }

    return 2;
}

static int cmpgeomwal1(const void *w1, const void *w2)
{
    uwalltype const * const wal1 = (uwalltype *)&wall[B_UNBUF16(w1)];
    uwalltype const * const wal2 = (uwalltype *)&wall[B_UNBUF16(w2)];

    if (wal1->x == wal2->x)
        return wal1->y - wal2->y;

    return wal1->x - wal2->x;
}

static void sort_walls_geometrically(int16_t *wallist, int32_t nwalls)
{
    qsort(wallist, nwalls, sizeof(int16_t), &cmpgeomwal1);
}
#endif

void SetFirstWall(int32_t sectnum, int32_t wallnum, int32_t alsoynw)
{
#ifdef YAX_ENABLE
    int32_t i, j, k=0;
#endif
    const sectortype *sec = &sector[sectnum];

    if (sec->wallptr == wallnum)
    {
        message("Wall %d already first wall of sector %d", wallnum, sectnum);
        return;
    }

#ifdef YAX_ENABLE
    if (alsoynw)
    {
        // Also consider upper/lower TROR neighbor walls.
        int32_t startwall, endwall;
        int16_t cf;

        for (i=0; i<numwalls; i++)
            wall[i].cstat &= ~(1<<14);

        for (cf=0; cf<2; cf++)
        {
            int16_t bunchnum;
            int32_t tempsect=sectnum, tempwall=wallnum;

            while ((bunchnum = yax_getbunch(tempsect, cf)) >= 0 &&
                   (tempsect=yax_is121(bunchnum, cf)) >= 0)
            {
                tempwall = yax_getnextwall(tempwall, cf);
                if (tempwall < 0)
                    break;  // corrupt!
                wall[tempwall].cstat |= (1<<14);
            }
        }

        for (i=0; i<numsectors; i++)
            for (WALLS_OF_SECTOR(i, j))
            {
                if (wall[j].cstat & (1<<14))
                {
                    setfirstwall(i, j);
                    k++;
                    break;
                }
            }
    }
    else
    {
        // Only consider aimed at wall <wallnum>.
        int16_t cb = yax_getbunch(sectnum, YAX_CEILING);
        int16_t fb = yax_getbunch(sectnum, YAX_FLOOR);

        if ((cb>=0 && (sec->ceilingstat&2)) || (fb >= 0 && (sec->floorstat&2)))
        {
            message("Extended ceilings/floors must not be sloped to set first wall");
            return;
        }
    }

    if (k > 0)
        message("Set first walls (sector[].wallptr) for %d sectors", k+1);

    if (k == 0)
#endif
        message("This wall now sector %d's first wall (sector[].wallptr)", sectnum);

    setfirstwall(sectnum, wallnum);

    mkonwinvalid_keeptempsect();

    asksave = 1;
}

void handlesecthighlight1(int32_t i, int32_t sub, int32_t nograycheck)
{
    int32_t j;

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
    {
        if (nograycheck || (graysectbitmap[i>>3]&(1<<(i&7)))==0)
            hlsectorbitmap[i>>3] |= (1<<(i&7));
    }
}

#ifdef YAX_ENABLE
// 1: good, 0: bad
static int32_t hl_all_bunch_sectors_p()
{
    uint8_t *const havebunch = visited;
    int16_t cf, cb, fb;
    int32_t i, j;

    if (numyaxbunches > 0)
    {
        Bmemset(havebunch, 0, (numyaxbunches+7)>>3);
        for (i=0; i<highlightsectorcnt; i++)
        {
            yax_getbunches(highlightsector[i], &cb, &fb);
            if (cb>=0)
                havebunch[cb>>3] |= (1<<(cb&7));
            if (fb>=0)
                havebunch[fb>>3] |= (1<<(fb&7));
        }

        for (i=0; i<numyaxbunches; i++)
        {
            if ((havebunch[i>>3] & (1<<(i&7)))==0)
                continue;

            for (cf=0; cf<2; cf++)
                for (SECTORS_OF_BUNCH(i,cf, j))
                    if ((hlsectorbitmap[j>>3]&(1<<(j&7)))==0)
                        return 0;
        }
    }

    return 1;
}
#endif

static int32_t find_nextwall(int32_t sectnum, int32_t sectnum2)
{
    int32_t j, startwall, endwall;

    if (sectnum<0 || sectnum2<0)
        return -1;

    for (WALLS_OF_SECTOR(sectnum, j))
        if (wall[j].nextsector == sectnum2)
            return j;

    return -1;
}

static int32_t bakframe_fillandfade(char **origframeptr, int32_t sectnum, const char *querystr)
{
    if (!*origframeptr)
    {
        *origframeptr = (char *)Xmalloc(xdim*ydim);

        begindrawing();
        Bmemcpy(*origframeptr, (char *)frameplace, xdim*ydim);
        enddrawing();
    }
    else
    {
        begindrawing();
        Bmemcpy((char *)frameplace, *origframeptr, xdim*ydim);
        enddrawing();
    }

    fillsector_notrans(sectnum, editorcolors[9]);
    fade_editor_screen(editorcolors[9]);

    return ask_if_sure(querystr, 0);
}

#ifdef YAX_ENABLE
static void M32_MarkPointInsertion(int32_t thewall)
{
    int32_t i, tmpcf;
    int32_t nextw = wall[thewall].nextwall;

    // round 1
    for (YAX_ITER_WALLS(thewall, i, tmpcf))
        wall[i].cstat |= (1<<14);
    if (nextw >= 0)
        for (YAX_ITER_WALLS(nextw, i, tmpcf))
            wall[i].cstat |= (1<<14);
    // round 2 (enough?)
    for (YAX_ITER_WALLS(thewall, i, tmpcf))
        if (wall[i].nextwall >= 0 && (wall[wall[i].nextwall].cstat&(1<<14))==0)
            wall[wall[i].nextwall].cstat |= (1<<14);
    if (nextw >= 0)
        for (YAX_ITER_WALLS(nextw, i, tmpcf))
            if (wall[i].nextwall >= 0 && (wall[wall[i].nextwall].cstat&(1<<14))==0)
                wall[wall[i].nextwall].cstat |= (1<<14);
}
#endif

// High-level insert point, handles TROR constrained walls too
//  onewnumwalls: old numwalls + drawn walls.
// <mapwallnum>: see insertpoint()
// Returns:
//  0 if wall limit would be reached.
//  1 if inserted point on a plain white or 2 points on a plain red wall.
//  N >= 2 if inserted N points on TROR-constrained wall.
//  N|(EXPECTED<<16) if inserted N points but EXPECTED walls were expected.
static int32_t M32_InsertPoint(int32_t thewall, int32_t dax, int32_t day, int32_t onewnumwalls, int32_t *mapwallnum)
{
#ifdef YAX_ENABLE
    int32_t nextw = wall[thewall].nextwall;
    int32_t i, j, k, m;

    if (yax_islockedwall(thewall) || (nextw>=0 && yax_islockedwall(nextw)))
    {
        // yax'ed wall -- first find out which walls are affected
        for (i=0; i<numwalls; i++)
            wall[i].cstat &= ~(1<<14);

        M32_MarkPointInsertion(thewall);

        for (i=0; i < numwalls; i++)
            if (wall[i].cstat&(1<<14))
                M32_MarkPointInsertion(i);

        j = 0;
        for (i=0; i<numwalls; i++)
            j += !!(wall[i].cstat&(1<<14));
        if (max(numwalls,onewnumwalls)+j > MAXWALLS)
        {
            return 0;  // no points inserted, would exceed limits
        }

        // the actual insertion!
        m = 0;
        for (i=0; i<numwalls /* rises with ins. */; i++)
        {
            if (wall[i].cstat&(1<<14))
                if (wall[i].nextwall<0 || i<wall[i].nextwall) // || !(NEXTWALL(i).cstat&(1<<14)) ??
                {
                    m += insertpoint(i, dax,day, mapwallnum);
                }
        }

        for (i=0; i<numwalls; i++)
        {
            if (wall[i].cstat&(1<<14))
            {
                wall[i].cstat &= ~(1<<14);
                k = yax_getnextwall(i+1, YAX_CEILING);
                if (k >= 0)
                    yax_setnextwall(i+1, YAX_CEILING, k+1);
                k = yax_getnextwall(i+1, YAX_FLOOR);
                if (k >= 0)
                    yax_setnextwall(i+1, YAX_FLOOR, k+1);
            }
        }

        if (m==j)
            return m;
        else
            return m|(j<<16);
    }
    else
#endif
    {
        insertpoint(thewall, dax,day, mapwallnum);
        return 1;
    }
}


// based on lineintersect in engine.c, but lines are considered as infinitely
// extending
void inflineintersect(int32_t x1, int32_t y1, int32_t x2, int32_t y2,
                      int32_t x3, int32_t y3, int32_t x4, int32_t y4,
                      int32_t *intx, int32_t *inty, int32_t *sign12, int32_t *sign34)
{
    //p1 to p2 is a line segment
    int64_t x21, y21, x34, y34, x31, y31, bot, topt, topu, t;

    x21 = x2-x1; x34 = x3-x4;
    y21 = y2-y1; y34 = y3-y4;
    bot = x21*y34 - y21*x34;
    if (bot >= 0)
    {
        if (bot == 0) { *sign12 = *sign34 = 0; return; };
        x31 = x3-x1; y31 = y3-y1;
        topt = x31*y34 - y31*x34;
        topu = x21*y31 - y21*x31;
    }
    else
    {
        x31 = x3-x1; y31 = y3-y1;
        topt = x31*y34 - y31*x34;
        topu = x21*y31 - y21*x31;
    }

    t = (topt*(1<<24))/bot;
    *intx = x1 + ((x21*t)>>24);
    *inty = y1 + ((y21*t)>>24);

    *sign12 = topt < 0 ? -1 : 1;
    *sign34 = topu < 0 ? -1 : 1;

    return;
}

static int32_t lineintersect2v(const vec2_t *p1, const vec2_t *p2,  // line segment 1
                               const vec2_t *q1, const vec2_t *q2,  // line segment 2
                               vec2_t *pint)
{
    int32_t intz;
    return lintersect(p1->x, p1->y, 0, p2->x, p2->y, 0,
                         q1->x, q1->y, q2->x, q2->y,
                         &pint->x, &pint->y, &intz);
}

static int32_t vec2eq(const vec2_t *v1, const vec2_t *v2)
{
    return (v1->x==v2->x && v1->y==v2->y);
}

#ifdef YAX_ENABLE
// After auto-creating inner sector <ns> in existing sector <os>, we need to
// see if some sprites contained in <os> need to change their sector.
static void CorrectSpriteSectnums(int32_t os, int32_t ns)
{
    int32_t i, ni;

    for (SPRITES_OF_SECT_SAFE(os, i, ni))
    {
        if (inside(sprite[i].x, sprite[i].y, ns)==1)
            changespritesect(i, ns);
    }
}
#endif

// precondition: [numwalls, newnumwalls-1] form a new loop (may be of wrong orientation)
// ret_ofirstwallofs: if != NULL, *ret_ofirstwallofs will contain the offset of the old
//                    first wall from the new first wall of the sector k, and the automatic
//                    restoring of the old first wll will not be carried out
// returns:
//  -1, -2: errors
//   0,  1: OK, 1 means it was an extended sector and an inner loop has been added automatically
static int32_t AddLoopToSector(int32_t k, int32_t *ret_ofirstwallofs)
{
    int32_t extendedSector=0, firstwall, i, j;
#ifdef YAX_ENABLE
    int16_t cbunch, fbunch;
    int32_t newnumwalls2;

    yax_getbunches(k, &cbunch, &fbunch);
    extendedSector = (cbunch>=0 || fbunch>=0);
#endif
    j = newnumwalls-numwalls;
#ifdef YAX_ENABLE
    newnumwalls2 = newnumwalls + j;

    if (extendedSector)
    {
        if ((cbunch>=0 && (sector[k].ceilingstat&2))
            || (fbunch>=0 && (sector[k].floorstat&2)))
        {
            printmessage16("Sloped extended sectors cannot be subdivided.");
            newnumwalls--;
            return -1;
        }

        if (newnumwalls + j > MAXWALLS || numsectors+1 > MAXSECTORS)
        {
            message("Automatically adding inner sector to new extended sector would exceed limits!");
            newnumwalls--;
            return -2;
        }
    }
#endif
    if (clockdir(numwalls) == CLOCKDIR_CW)
        flipwalls(numwalls,newnumwalls);

    sector[k].wallnum += j;
    for (i=k+1; i<numsectors; i++)
        sector[i].wallptr += j;
    firstwall = sector[k].wallptr;

    for (i=0; i<numwalls; i++)
    {
        if (wall[i].nextwall >= firstwall)
            wall[i].nextwall += j;
        if (wall[i].point2 >= firstwall)
            wall[i].point2 += j;
    }
#ifdef YAX_ENABLE
    yax_tweakwalls(firstwall, j);
#endif

    Bmemmove(&wall[firstwall+j], &wall[firstwall], (newnumwalls-firstwall)*sizeof(walltype));
    // add new loop to beginning of sector
    Bmemmove(&wall[firstwall], &wall[newnumwalls], j*sizeof(walltype));

    for (i=firstwall; i<firstwall+j; i++)
    {
        wall[i].point2 += (firstwall-numwalls);

        copy_some_wall_members(i, firstwall+j, 1);
        wall[i].cstat &= ~(1+16+32+64);
    }

    numwalls = newnumwalls;
    newnumwalls = -1;
#ifdef YAX_ENABLE
    if (extendedSector)
    {
        newnumwalls = whitelinescan(k, firstwall);
        if (newnumwalls != newnumwalls2)
            message("AddLoopToSector: newnumwalls != newnumwalls2!!! WTF?");
        for (i=numwalls; i<newnumwalls; i++)
        {
            NEXTWALL(i).nextwall = i;
            NEXTWALL(i).nextsector = numsectors;
        }

        yax_setbunches(numsectors, cbunch, fbunch);

        numwalls = newnumwalls;
        newnumwalls = -1;
        numsectors++;

        CorrectSpriteSectnums(k, numsectors-1);
    }
#endif
    if (ret_ofirstwallofs)
        *ret_ofirstwallofs = j;
    else
        setfirstwall(k, firstwall+j);  // restore old first wall

    return extendedSector;
}

int32_t select_sprite_tag(int32_t spritenum)
{
    int32_t lt = taglab_linktags(1, spritenum);
    spritetype *spr = &sprite[spritenum];

    if (lt==0)
        return INT32_MIN;

    if (lt&1)
        return spr->lotag;
    if (lt&2)
        return spr->hitag;
    if (lt&4)
        return spr->extra;
    if (lt&8)
        return spr->xvel;
    if (lt&16)
        return spr->yvel;
    if (lt&32)
        return spr->zvel;
    if (lt&64)
        return spr->extra;

    return INT32_MIN;
}

static void drawlinebetween(const vec3_t *v1, const vec3_t *v2, int32_t col, uint32_t pat)
{
    // based on m32exec.c/drawline*
    const int32_t xofs=halfxdim16, yofs=midydim16;
    const uint32_t opat=drawlinepat;

    int32_t x1, x2, y1, y2;

    screencoords(&x1,&y1, v1->x-pos.x,v1->y-pos.y, zoom);
    screencoords(&x2,&y2, v2->x-pos.x,v2->y-pos.y, zoom);

    if (m32_sideview)
    {
        y1 += getscreenvdisp(v1->z-pos.z,zoom);
        y2 += getscreenvdisp(v2->z-pos.z,zoom);
    }

    drawlinepat = pat;
    drawline16(xofs+x1,yofs+y1, xofs+x2,yofs+y2, col);
    drawlinepat = opat;
}

// world -> screen coords for overhead mode
void ovhscrcoords(int32_t x, int32_t y, int32_t *scrx, int32_t *scry)
{
    *scrx = halfxdim16 + mulscale14(x-pos.x, zoom);
    *scry = midydim16 + mulscale14(y-pos.y, zoom);
}

static void draw_cross(int32_t centerx, int32_t centery, int32_t radius, int32_t col)
{
    int32_t dax, day;
    ovhscrcoords(centerx, centery, &dax, &day);
    drawline16base(dax, day, -radius,-radius, +radius,+radius, col);
    drawline16base(dax, day, -radius,+radius, +radius,-radius, col);
}

static void draw_square(int32_t dax, int32_t day, int32_t ps, int32_t col)
{
    ovhscrcoords(dax, day, &dax, &day);
    drawline16base(dax, day, -ps,-ps, +ps,-ps, col);
    drawline16base(dax, day, +ps,-ps, +ps,+ps, col);
    drawline16base(dax, day, +ps,+ps, -ps,+ps, col);
    drawline16base(dax, day, -ps,+ps, -ps,-ps, col);
}

//// Interactive Scaling
static struct {
    int8_t active, rotatep;
    vec2_t piv;  // pivot point
    int32_t dragx, dragy;  // dragged point
    int32_t xsc, ysc, ang;
} isc;

static void isc_transform(int32_t *x, int32_t *y)
{
    if (!isc.rotatep)
    {
        *x = isc.piv.x + mulscale16(*x-isc.piv.x, isc.xsc);
        *y = isc.piv.y + mulscale16(*y-isc.piv.y, isc.ysc);
    }
    else
    {
        vec2_t v = { *x, *y };
        rotatepoint(isc.piv, v, isc.ang, &v);
        *x = v.x;
        *y = v.y;
    }
}

static void drawspritelabel(int i)
{
    // XXX: oob 'i' may happen, such as passing pointhighlight-16384 when
    // pointhighlight == -1.
    if ((unsigned)i >= MAXSPRITES)
        return;

    const char *dabuffer = CallExtGetSpriteCaption(i);

    if (!dabuffer[0])
        return;

    // KEEPINSYNC drawscreen_drawsprite()
    uspritetype const * s = (uspritetype *)&sprite[i];
    uint8_t const spritecol = spritecol2d[s->picnum][(s->cstat&1)];
    int col = spritecol ? editorcolors[spritecol] : getspritecol(i);
    int const blocking = s->cstat & 1;
    int bordercol = blocking ? editorcolors[5] : col;

    // group selection
    if (show2dsprite[i>>3]&pow2char[i&7])
    {
        bordercol = editorcolors[14];
        col = bordercol - (M32_THROB>>1);
    }
    else if (i == pointhighlight - 16384)
    {
        if (spritecol >= 8 && spritecol <= 15)
            col -= M32_THROB>>1;
        else col += M32_THROB>>2;

        if (bordercol > col && !blocking)
            bordercol = col;
    }

    else if (s->sectnum < 0)
        col = bordercol = editorcolors[4];  // red

    drawsmallabel(dabuffer, editorcolors[0], col, bordercol, s->x, s->y, s->z);
}

#define EDITING_MAP_P() (newnumwalls>=0 || joinsector[0]>=0 || circlewall>=0 || (bstatus&1) || isc.active)

#define HLMEMBERX(Hl, Member) (*(((Hl)&16384) ? &sprite[(Hl)&16383].Member : &wall[Hl].Member))
#define HLMEMBER(Hlidx, Member) HLMEMBERX(highlight[Hlidx], Member)

void overheadeditor(void)
{
    char buffer[80];
    const char *dabuffer;
    int32_t i, j, k, m=0, mousxplc, mousyplc, firstx=0, firsty=0, oposz, col;
    int32_t numwalls_bak;
    int32_t startwall=0, endwall, dax, day, x1, y1, x2, y2, x3, y3; //, x4, y4;
    int32_t highlightx1, highlighty1, highlightx2, highlighty2;
    int16_t bad, joinsector[2];
    int32_t bstatus, mousewaitmask=0;
    int16_t circlepoints;
    int32_t sectorhighlightx=0, sectorhighlighty=0;
    int16_t cursectorhighlight, sectorhighlightstat;
    int32_t prefixarg = 0, tsign;
    int32_t resetsynctics = 0, lasttick=getticks(), waitdelay=totalclock, lastdraw=getticks();
    int32_t olen[2] = {0, 0}, dragwall[2] = {-1, -1};
    int16_t linehighlight2 = -1;

    ovh.suckwall = -1;
    ovh.split = 0;
    ovh.splitsect = -1;
    ovh.splitstartwall = -1;

    qsetmodeany(xdim2d,ydim2d);
    xdim2d = xdim;
    ydim2d = ydim;

    osearchx = searchx;
    osearchy = searchy;

    searchx = clamp(scale(searchx,xdim2d,xdimgame), 8, xdim2d-8-1);
    searchy = clamp(scale(searchy,ydim2d-STATUS2DSIZ2,ydimgame), 8, ydim2d-STATUS2DSIZ-8-1);
    oposz = pos.z;

    yax_updategrays(pos.z);

    begindrawing(); //{{{
    CLEARLINES2D(0, ydim, 0);
    enddrawing(); //}}}

    ydim16 = ydim-STATUS2DSIZ2;

    cursectorhighlight = -1;
    lastpm16time = -1;

    update_highlightsector();
    ovh_whiteoutgrab(0);

    highlightcnt = -1;
    Bmemset(show2dwall, 0, sizeof(show2dwall));  //Clear all highlights
    Bmemset(show2dsprite, 0, sizeof(show2dsprite));

    RESET_EDITOR_VARS();
    bstatus = 0;

    while ((keystatus[buildkeys[BK_MODE2D_3D]]>>1) == 0)
    {
        int32_t mousx, mousy;

        if (zoom < ztarget)
        {
            if ((ztarget - zoom) >> 3)
                zoom += synctics * ((ztarget - zoom) >> 3);
            else zoom++;
            zoom = min(zoom, ztarget);
        }
        else if (zoom > ztarget)
        {
            if ((zoom - ztarget) >> 3)
                zoom -= synctics * ((zoom - ztarget) >> 3);
            else zoom--;
            zoom = max(zoom, ztarget);
        }

        if (!((vel|angvel|svel) || m32_is2d3dmode() || ztarget != zoom//DOWN_BK(MOVEFORWARD) || DOWN_BK(MOVEBACKWARD) || DOWN_BK(TURNLEFT) || DOWN_BK(TURNRIGHT)
                || DOWN_BK(MOVEUP) || DOWN_BK(MOVEDOWN) || keystatus[0x10] || keystatus[0x11]
                || keystatus[0x48] || keystatus[0x4b] || keystatus[0x4d] || keystatus[0x50]  // keypad keys
                || bstatus || OSD_IsMoving()))
        {
            if (totalclock > waitdelay)
            {
                uint32_t ms = 50;// (highlightsectorcnt>0) ? 75 : 200;
                // wait for event, timeout after 200 ms - (last loop time)
                idle_waitevent_timeout(ms - min(getticks()-lasttick, ms));
                // have synctics reset to 0 after we've slept to avoid zooming out to the max instantly
                resetsynctics = 1;
            }
        }
        else waitdelay = totalclock + 6; // should be 50 ms

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

        if (!m32_is2d3dmode())
        {
            oldmousebstatus = bstatus;
            getmousevalues(&mousx, &mousy, &bstatus);

            {
                int32_t bs = bstatus;
                bstatus &= ~mousewaitmask;
                mousewaitmask &= bs;
            }

            mousx = (mousx<<16)+mousexsurp;
            mousy = (mousy<<16)+mouseysurp;
            {
                ldiv_t ld;
                ld = ldiv(mousx, 1<<16); mousx = ld.quot; mousexsurp = ld.rem;
                ld = ldiv(mousy, 1<<16); mousy = ld.quot; mouseysurp = ld.rem;
            }
            searchx += mousx;
            searchy += mousy;

            inpclamp(&searchx, 8, xdim-8-1);
            inpclamp(&searchy, 8, ydim-8-1);

            mainloop_move();

            getpoint(searchx, searchy, &mousxplc, &mousyplc);
            linehighlight = getlinehighlight(mousxplc, mousyplc, linehighlight, 0);
            linehighlight2 = getlinehighlight(mousxplc, mousyplc, linehighlight, 1);
        }

        if (newnumwalls >= numwalls)
        {
            // if we're in the process of drawing a wall, set the end point's coordinates
            dax = mousxplc;
            day = mousyplc;
            adjustmark(&dax,&day,numwalls+!ovh.split);
            wall[newnumwalls].x = dax;
            wall[newnumwalls].y = day;
        }

        ydim16 = ydim;// - STATUS2DSIZ2;
        midydim16 = ydim>>1;

        numwalls_bak = numwalls;
        numwalls = newnumwalls;
        if (numwalls < 0)
            numwalls = numwalls_bak;

        if ((getticks() - lastdraw) >= 5 || (vel|angvel|svel) || DOWN_BK(MOVEUP) || DOWN_BK(MOVEDOWN)
                || mousx || mousy || bstatus || keystatus[0x10] || keystatus[0x11]
                || newnumwalls>=0 || OSD_IsMoving())
        {
            lastdraw = getticks();

            clear2dscreen();

            setup_sideview_sincos();

            VM_OnEvent(EVENT_PREDRAW2DSCREEN, -1);

            if (graphicsmode && (!m32_sideview || m32_sideelev == 512))
            {
                Bmemset(show2dsector, 0, sizeof(show2dsector));
                for (i=0; i<numsectors; i++)
                {
                    YAX_SKIPSECTOR(i);
                    show2dsector[i>>3] |= (1<<(i&7));
                }

                setview(0, 0, xdim-1, ydim16-1);

                if (graphicsmode == 2)
                    totalclocklock = totalclock;

                drawmapview(pos.x, pos.y, zoom, m32_sideview ? (3584 - m32_sideang) & 2047: 1536);
            }

            draw2dgrid(pos.x,pos.y,pos.z,cursectnum,ang,zoom,grid);
            CallExtPreCheckKeys();
            draw2dscreen(&pos,cursectnum,ang,zoom,grid);

            // Draw brown arrow (start)
            screencoords(&x2, &y2, startpos.x-pos.x,startpos.y-pos.y, zoom);
            if (m32_sideview)
                y2 += getscreenvdisp(startpos.z-pos.z, zoom);

            int32_t cx = halfxdim16+x2;
            int32_t cy = midydim16+y2;

            begindrawing();	//{{{  LOCK_FRAME_1

            if ((cx >= 2 && cx <= xdim-3) && (cy >= 2 && cy <= ydim16-3))
            {
                int16_t angofs = m32_sideview ? m32_sideang : 0;
                x1 = mulscale11(sintable[(startang+angofs+2560)&2047],zoom) / 768;
                y1 = mulscale11(sintable[(startang+angofs+2048)&2047],zoom) / 768;
                i = scalescreeny(x1);
                j = scalescreeny(y1);
                drawline16base(cx,cy, x1,j, -x1,-j, editorcolors[6]);
                drawline16base(cx,cy, x1,j, +y1,-i, editorcolors[6]);
                drawline16base(cx,cy, x1,j, -y1,+i, editorcolors[6]);
            }

            if (keystatus[0x2a] && (pointhighlight&16384) && highlightcnt<=0)  // LShift
            {
                // draw lines to linking sprites
                const int32_t refspritenum = pointhighlight&16383;
                const int32_t reftag = select_sprite_tag(refspritenum);

                if (reftag != INT32_MIN)
                {
                    for (i=0; i<numsectors; i++)
                        for (SPRITES_OF_SECT(i, j))
                            if (reftag==select_sprite_tag(j))
                                drawlinebetween((vec3_t *)&sprite[refspritenum], (vec3_t *)&sprite[j],
                                                editorcolors[12], 0x33333333);
                }
            }

            if (showtags)
            {
                if (zoom >= 768)
                {
                    for (i=0; i<numsectors; i++)
                    {
                        int16_t secshort = i;

                        YAX_SKIPSECTOR(i);

                        dabuffer = CallExtGetSectorCaption(i);
                        if (dabuffer[0] == 0)
                            continue;

                        get_sectors_center(&secshort, 1, &dax, &day);

                        drawsmallabel(dabuffer, editorcolors[0], editorcolors[7], editorcolors[7] - 3, dax, day, getflorzofslope(i,dax,day));
                    }
                }

                x3 = pos.x + divscale14(-halfxdim16,zoom);
                y3 = pos.y + divscale14(-(midydim16-4),zoom);
//                x4 = pos.x + divscale14(halfxdim16,zoom);
//                y4 = pos.y + divscale14(ydim16-(midydim16-4),zoom);

                if (newnumwalls >= 0)
                {
                    for (i=newnumwalls; i>=numwalls_bak; i--)
                        wall[i].cstat |= (1<<14);
                }

                i = numwalls-1;
                j = numsectors-1;  // might be -1 if empty map!
                if (newnumwalls >= 0)
                    i = newnumwalls-1;
                for (; i>=0; i--)
                {
                    walltype const * const wal = &wall[i];

                    if (j>=0 && sector[j].wallptr > i)
                        j--;

                    if (zoom < 768 && !(wal->cstat & (1<<14)))
                        continue;

                    YAX_SKIPWALL(i);

                    //Get average point of wall
//                    if ((dax > x3) && (dax < x4) && (day > y3) && (day < y4))
                    {
                        dabuffer = CallExtGetWallCaption(i);
                        if (dabuffer[0] == 0)
                            continue;

                        dax = (wal->x+wall[wal->point2].x)>>1;
                        day = (wal->y+wall[wal->point2].y)>>1;
                        drawsmallabel(dabuffer, editorcolors[0], editorcolors[31], editorcolors[31] - 3, dax, day, (i >= numwalls || j<0) ? 0 : getflorzofslope(j, dax,day));
                    }
                }

                if (zoom >= 768)
                {
                    int32_t alwaysshowgray = get_alwaysshowgray();

                    for (i=0, k=0; (m32_sideview && k<m32_swcnt) || (!m32_sideview && i<MAXSPRITES); i++, k++)
                    {
                        if (m32_sideview)
                        {
                            i = m32_wallsprite[k];
                            if (i<MAXWALLS)
                                continue;
                            i = i-MAXWALLS;
                        }
                        else
                            if (sprite[i].statnum == MAXSTATUS)
                                continue;

                        if ((!m32_sideview || !alwaysshowgray) && sprite[i].sectnum >= 0)
                            YAX_SKIPSECTOR(sprite[i].sectnum);

                        drawspritelabel(i);
                    }

                    if (pointhighlight & 16384)
                        drawspritelabel(pointhighlight - 16384);
                }
            }

            // stick this event right between begin- end enddrawing()...
            // also after the above label stuff so users can redefine them
            VM_OnEvent(EVENT_DRAW2DSCREEN, -1);

            printcoords16(pos.x,pos.y,ang);

            numwalls = numwalls_bak;

            if (highlightsectorcnt >= 0)
            {
                for (i=0; i<numsectors; i++)
                    if (hlsectorbitmap[i>>3]&(1<<(i&7)))
                        fillsector(i, -1);
            }

            if (keystatus[0x2a])  // LShift
            {
                if (!m32_is2d3dmode() && (m32_sideview || highlightcnt <= 0))
                {
                    drawlinepat = 0x00ff00ff;
                    drawline16(searchx,0, searchx,ydim2d-1, editorcolors[15]);
                    drawline16(0,searchy, xdim2d-1,searchy, editorcolors[15]);
                    drawlinepat = 0xffffffff;

                    _printmessage16("(%d,%d)",mousxplc,mousyplc);
                }
                else
                {
                    // do interactive scaling
                    if (!isc.active)
                    {
                        if (pointhighlight >= 0 && (bstatus&3))
                        {
                            // initialize by finding pivot point
                            int32_t minx=INT32_MAX, miny=INT32_MAX;
                            int32_t maxx=INT32_MIN, maxy=INT32_MIN;

                            isc.rotatep = ((bstatus&3)==2);
                            bstatus &= ~3;

                            for (i=0; i<highlightcnt; i++)
                            {
                                minx = min(minx, HLMEMBER(i, x));
                                miny = min(miny, HLMEMBER(i, y));
                                maxx = max(maxx, HLMEMBER(i, x));
                                maxy = max(maxy, HLMEMBER(i, y));
                            }

                            isc.piv.x = (minx+maxx)/2;
                            isc.piv.y = (miny+maxy)/2;

                            isc.dragx = HLMEMBERX(pointhighlight, x);
                            isc.dragy = HLMEMBERX(pointhighlight, y);

                            isc.xsc = isc.ysc = 1<<16;
                            isc.ang = 0;

                            isc.active = 1;
                        }
                    }
                    else
                    {
                        if (bstatus&3)
                        {
                            // drag/rotate the reference point
                            const int32_t pivx=isc.piv.x, pivy=isc.piv.y;
                            const int32_t dragx=isc.dragx, dragy=isc.dragy;
                            int32_t mxplc=mousxplc, myplc=mousyplc, xsc=1<<16, ysc=1<<16;

                            const int32_t dx=dragx-pivx, dy=dragy-pivy;
                            int32_t mdx, mdy;

                            bstatus &= ~3;

                            draw_cross(pivx, pivy, 3, editorcolors[14]);

                            adjustmark(&mxplc, &myplc, numwalls);
                            mdx = mxplc-pivx;
                            mdy = myplc-pivy;

                            if (!isc.rotatep)
                            {
                                if (mdx != 0 && dx != 0 && klabs(dx) >= 8)
                                    xsc = min(klabs(divscale16(mdx, dx)), 1<<18);
                                if (mdy != 0 && dy != 0 && klabs(dy) >= 8)
                                    ysc = min(klabs(divscale16(mdy, dy)), 1<<18);

                                if (eitherCTRL)
                                    xsc = ysc = max(xsc, ysc);

                                isc.xsc = xsc;
                                isc.ysc = ysc;

                                printmessage16("scale x=%.3f y=%.3f", (double)xsc/65536, (double)ysc/65536);
                            }
                            else
                            {
                                isc.ang = getangle(mdx, mdy) - getangle(dx, dy);
                                printmessage16("rotate ang %d", isc.ang);
                            }

                            for (i=0; i<highlightcnt; i++)
                            {
                                int32_t x=HLMEMBER(i, x), y=HLMEMBER(i, y);

                                isc_transform(&x, &y);

                                draw_square(x, y, 2, editorcolors[15]);

                                if ((highlight[i]&16384)==0)
                                {
                                    walltype const * const wal = &wall[highlight[i]];
                                    const int32_t p2=wal->point2, hlp=(show2dwall[p2>>3]&(1<<(p2&7)));
                                    vec3_t v1 = { x, y, 0 }, v2 = { wall[p2].x, wall[p2].y, 0 };

                                    isc_transform(&v2.x, &v2.y);
                                    if (!hlp)
                                    {
                                        v2.x = wall[p2].x;
                                        v2.y = wall[p2].y;
                                    }

                                    drawlinebetween(&v1, &v2, !hlp ? 8 :
                                                    editorcolors[wal->nextwall >= 0 ? 12 : 7],
                                                    0x11111111);
                                }
                            }
                        }
                        else
                        {
                            // finish interactive scaling
                            isc.active = 0;

                            if ((!isc.rotatep && (isc.xsc!=1<<16 || isc.ysc!=1<<16)) ||
                                (isc.rotatep && (isc.ang!=0)))
                            {
                                for (i=0; i<highlightcnt; i++)
                                {
                                    int32_t *x=&HLMEMBER(i, x), *y=&HLMEMBER(i, y);

                                    isc_transform(x, y);

                                    if (isc.rotatep && (highlight[i]&16384))
                                    {
                                        spritetype *spr = &sprite[highlight[i]&16383];
                                        spr->ang = (spr->ang + isc.ang)&2047;
                                    }
                                }

                                if (!isc.rotatep)
                                    message("Highlights scaled by x=%.3f y=%.3f",
                                            (double)isc.xsc/65536, (double)isc.ysc/65536);
                                else
                                    message("Highlights rotated by %d BUILD degrees", isc.ang);

                                asksave = 1;
                            }
                            else
                                printmessage16(" ");
                        }
                    }
                }
            }
            else
            {
                if (isc.active)
                {
                    isc.active = 0;

                    printmessage16("Aborted interactive %s.", isc.rotatep ? "rotation" : "scaling");
                    bstatus &= ~3;
                    mousewaitmask = 3;
                    pointhighlight = -1;
                }
            }

            drawline16(searchx,0, searchx,8, editorcolors[15]);
            drawline16(0,searchy, 8,searchy, editorcolors[15]);

            // 2d3d mode
            if (m32_2d3dmode && m32_2d3d_resolutions_match())
            {
#ifdef USE_OPENGL
                int bakrendmode = rendmode;
#endif
                vec2_t bdim = { xdim, ydim };

                xdim = xdim2d;
                ydim = ydim2d;
#ifdef USE_OPENGL
                rendmode = REND_CLASSIC;
#endif

                if (m32_2d3d.x + XSIZE_2D3D > xdim2d - 4)
                    m32_2d3d.x = xdim2d - 4 - XSIZE_2D3D;

                if (m32_2d3d.y + YSIZE_2D3D > ydim2d - 4 - STATUS2DSIZ2)
                    m32_2d3d.y = ydim2d - 4 - YSIZE_2D3D - STATUS2DSIZ2;

                updatesectorz(pos.x, pos.y, pos.z, &cursectnum);

                if (cursectnum == -1)
                    updatesector(pos.x, pos.y, &cursectnum);

                if (cursectnum != -1)
                {
                    int32_t cz, fz;

                    getzsofslope(cursectnum, pos.x, pos.y, &cz, &fz);

                    inpclamp(&pos.z, cz+(4<<8), fz-(4<<8));

                    enddrawing();
                    setview(m32_2d3d.x, m32_2d3d.y, m32_2d3d.x + XSIZE_2D3D, m32_2d3d.y + YSIZE_2D3D);
                    clearview(-1);

                    vec2_t osearch = { searchx, searchy };

                    searchx -= m32_2d3d.x;
                    searchy -= m32_2d3d.y;

                    M32_DrawRoomsAndMasks();
                    setview(0, 0, xdim2d-1, ydim2d-1);

#ifdef USE_OPENGL
                    rendmode = bakrendmode;
#endif
                    xdim = bdim.x;
                    ydim = bdim.y;
                    searchx = osearch.x;
                    searchy = osearch.y;

                    begindrawing();
                    drawline16(m32_2d3d.x, m32_2d3d.y, m32_2d3d.x + XSIZE_2D3D, m32_2d3d.y, editorcolors[15]);
                    drawline16(m32_2d3d.x + XSIZE_2D3D, m32_2d3d.y, m32_2d3d.x + XSIZE_2D3D, m32_2d3d.y  + YSIZE_2D3D, editorcolors[15]);
                    drawline16(m32_2d3d.x, m32_2d3d.y, m32_2d3d.x, m32_2d3d.y + YSIZE_2D3D, editorcolors[15]);
                    drawline16(m32_2d3d.x, m32_2d3d.y + YSIZE_2D3D, m32_2d3d.x + XSIZE_2D3D, m32_2d3d.y + YSIZE_2D3D, editorcolors[15]);
                }
            }

            if (!m32_is2d3dmode())
            {
                ////// draw mouse pointer

                col = editorcolors[0];

                drawline16base(searchx+1, searchy+1, +0, -8, +0, -1, col);
                drawline16base(searchx+1, searchy+1, +0, 1, +0, 8, col);

                drawline16base(searchx+1, searchy+1, -8, 0, -1, 0, col);
                drawline16base(searchx+1, searchy+1, 1, 0, 8, 0, col);

                col = searchlock ? editorcolors[13] : editorcolors[15 - 3*gridlock];

                if (joinsector[0] >= 0)
                    col = editorcolors[11];

                if (numcorruptthings>0)
                {
                    static char cbuf[64];

                    if ((pointhighlight&16384)==0)
                    {
                        // If aiming at wall, check whether it is corrupt, and print a
                        // warning message near the mouse pointer if that is the case.
                        for (i=0; i<numcorruptthings; i++)
                            if ((corruptthings[i]&CORRUPT_MASK)==CORRUPT_WALL &&
                                (corruptthings[i]&(MAXWALLS-1))==pointhighlight)
                            {
                                col = editorcolors[13];
                                printext16(searchx+6, searchy-6-8, editorcolors[13], editorcolors[0], "corrupt wall", 0);
                                break;
                            }
                    }

                    Bsprintf(cbuf, "Map corrupt (level %d): %s%d errors", corruptlevel,
                        numcorruptthings>=MAXCORRUPTTHINGS ? ">=" : "", numcorruptthings);
                    printext16(8, 8, editorcolors[13]+(M32_THROB>>2), editorcolors[0], cbuf, 0);
                }

                if (highlightsectorcnt==0 || highlightcnt==0)
                {
                    if (keystatus[0x27] || keystatus[0x28])  // ' and ;
                    {
                        col = editorcolors[14];

                        drawline16base(searchx+16, searchy-16, -4, 0, +4, 0, col);
                        if (keystatus[0x28])
                            drawline16base(searchx+16, searchy-16, 0, -4, 0, +4, col);
                    }

                    if (highlightsectorcnt == 0)
                        if (keystatus[0x36])
                            printext16(searchx+6, searchy-2+8, editorcolors[12], -1, "ALL", 0);

                    if (highlightcnt == 0)
                    {
                        if (eitherCTRL && (highlightx1!=highlightx2 || highlighty1!=highlighty2))
                            printext16(searchx+6, searchy-6-8, editorcolors[12], -1, "SPR ONLY", 0);
#ifdef YAX_ENABLE
                        if (keystatus[0xcf])  // End
                            printext16(searchx+6, searchy-2+8, editorcolors[12], -1, "ALL", 0);
#endif
                    }
                }

                drawline16base(searchx, searchy, +0, -8, +0, -1, col);
                drawline16base(searchx, searchy, +0, 1, +0, 8, col);

                drawline16base(searchx, searchy, -8, 0, -1, 0, col);
                drawline16base(searchx, searchy, 1, 0, 8, 0, col);

                ////// Draw the white pixel closest to mouse cursor on linehighlight
                if (linehighlight>=0)
                {
                    char col = wall[linehighlight].nextsector >= 0 ? editorcolors[15] : editorcolors[5];

                    if (m32_sideview)
                    {
                        getclosestpointonwall(searchx, searchy, linehighlight, &dax, &day, 1);
                        drawline16base(dax, day, 0, 0, 0, 0, col);
                    }
                    else
                    {
                        getclosestpointonwall(mousxplc, mousyplc, linehighlight, &dax, &day, 0);
                        ovhscrcoords(dax, day, &x2, &y2);
                        drawline16base(x2, y2, 0, 0, 0, 0, col);
                    }
                }
            }

            enddrawing();	//}}} LOCK_FRAME_1

            OSD_Draw();
        }

        inputchecked = 1;


        VM_OnEvent(EVENT_PREKEYS2D, -1);
        CallExtCheckKeys(); // TX 20050101, it makes more sense to have this here so keys can be overwritten with new functions in bstub.c

        // 2d3d mode
        if (m32_is2d3dmode())
            goto nokeys;

        // Flip/mirror sector Ed Coolidge
        if (keystatus[0x2d] || keystatus[0x15])  // X or Y (2D)
        {
            int32_t about_x=keystatus[0x2d];
            int32_t doMirror = eitherALT;  // mirror walls and wall/floor sprites

#ifdef YAX_ENABLE
            if (highlightsectorcnt > 0 && !hl_all_bunch_sectors_p())
            {
                printmessage16("To flip extended sectors, all sectors of a bunch must be selected");
                keystatus[0x2d] = keystatus[0x15] = 0;
            }
            else
#endif
            if (highlightsectorcnt > 0)
            {
                int16_t *const otonwall = onextwall;  // OK, since we make old-nextwalls invalid

                mkonwinvalid();

                keystatus[0x2d] = keystatus[0x15] = 0;

                for (j=0; j<numwalls; j++)
                    otonwall[j] = j;

                get_sectors_center(highlightsector, highlightsectorcnt, &dax, &day);

                if (gridlock && grid > 0)
                    locktogrid(&dax, &day);

                for (i=0; i<highlightsectorcnt; i++)
                {
                    int32_t startofloop, endofloop;
                    int32_t numtoswap = -1;
                    int32_t w=0;
                    uwalltype tempwall;

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

                                otonwall[startofloop+w] = endofloop-w+1;
                                otonwall[endofloop-w+1] = startofloop+w;
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

                // finally, construct the nextwalls and yax-nextwalls
                // for the new arrangement!
                for (i=0; i<highlightsectorcnt; i++)
                {
                    for (WALLS_OF_SECTOR(highlightsector[i], j))
                    {
                        if (wall[j].nextwall >= 0)
                            wall[j].nextwall = otonwall[wall[j].nextwall];
#ifdef YAX_ENABLE
                        {
                            int32_t cf, ynw;
                            for (cf=0; cf<2; cf++)
                                if ((ynw = yax_getnextwall(j, cf)) >= 0)
                                    yax_setnextwall(j, cf, otonwall[ynw]);
                        }
#endif
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
            linehighlight = getlinehighlight(mousxplc, mousyplc, linehighlight, 0);

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
                linehighlight = getlinehighlight(mousxplc, mousyplc, linehighlight, 0);
                if (linehighlight >= 0)
                    SetFirstWall(sectorofwall(linehighlight), linehighlight, 1);
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
#ifdef YAX_ENABLE
            if (highlightsectorcnt > 0 && !hl_all_bunch_sectors_p())
            {
                printmessage16("To rotate ext. sectors, all sectors of a bunch must be selected");
            }
            else
#endif
            if (highlightsectorcnt > 0)
            {
                int32_t smoothRotation = eitherSHIFT, manualAngle = eitherALT;
                vec2_t da = { dax, day };

                if (manualAngle)
                {
                    tsign = getnumber16("Rotation BUILD angle: ", 0, 2047, 1);
                    if (tsign==0)
                    {
                        printmessage16(" ");
                        goto rotate_hlsect_out;
                    }

                    printmessage16("Rotated highlighted sectors by %d BUILD degrees", tsign);
                    tsign &= 2047;
                    smoothRotation = 1;
                }

                get_sectors_center(highlightsector, highlightsectorcnt, &da.x, &da.y);

                if (!smoothRotation)
                {
                    if (gridlock && grid > 0)
                        locktogrid(&da.x, &da.y);

                    tsign *= 512;
                }


                for (i=0; i<highlightsectorcnt; i++)
                {
                    for (WALLS_OF_SECTOR(highlightsector[i], j))
                        rotatepoint(da, *(vec2_t *)&wall[j], tsign&2047, (vec2_t *)&wall[j].x);

                    for (j=headspritesect[highlightsector[i]]; j != -1; j=nextspritesect[j])
                    {
                        rotatepoint(da, *(vec2_t *)&sprite[j], tsign&2047, (vec2_t *)&sprite[j].x);
                        sprite[j].ang = (sprite[j].ang+tsign)&2047;
                    }
                }

                m32_rotateang += tsign;
                m32_rotateang &= 2047;
                asksave = 1;
rotate_hlsect_out:
                if (!smoothRotation || manualAngle)
                    keystatus[0x33] = keystatus[0x34] = 0;

                mouseb &= ~(16|32);
                bstatus &= ~(16|32);
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
                }
            }
        }

        if (keystatus[0x46])  //Scroll lock (set starting position)
        {
            startpos = pos;
            startang = ang;
            startsectnum = cursectnum;
            keystatus[0x46] = 0;
            asksave = 1;

            printmessage16("Set starting position");
        }
#if 1
        if (keystatus[0x3f])  //F5
        {
            CallExtShowSectorData(0);
        }
        if (keystatus[0x40])  //F6
        {
            if (pointhighlight >= 16384)
                CallExtShowSpriteData(pointhighlight-16384);
            else if (linehighlight >= 0)
                CallExtShowWallData(linehighlight);
            else
                CallExtShowWallData(0);
        }
        if (keystatus[0x41])  //F7
        {
            keystatus[0x41] = 0;

            for (i=0; i<numsectors; i++)
                if (inside_editor_curpos(i) == 1)
                {
                    YAX_SKIPSECTOR(i);

                    CallExtEditSectorData(i);
                    break;
                }
        }
        if (keystatus[0x42])  //F8
        {
            keystatus[0x42] = 0;

            if (pointhighlight >= 16384)
                CallExtEditSpriteData(pointhighlight-16384);
            else if (linehighlight >= 0)
                CallExtEditWallData(linehighlight);
        }
#endif

        if (keystatus[0x23])  //H (Hi 16 bits of tag)
        {
            keystatus[0x23] = 0;
            if (eitherCTRL)  //Ctrl-H
            {
                pointhighlight = getpointhighlight(mousxplc, mousyplc, pointhighlight);
                linehighlight = getlinehighlight(mousxplc, mousyplc, linehighlight, 0);

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
                    j = taglab_linktags(1, i);
                    j = 2*(j&2);
                    Bsprintf(buffer, "Sprite (%d) Hi-tag: ", i);
                    sprite[i].hitag = getnumber16(buffer, sprite[i].hitag, BTAG_MAX, 0+j);
                }
                else if (linehighlight >= 0)
                {
                    i = linehighlight;
                    j = taglab_linktags(1, i);
                    j = 2*(j&2);
                    Bsprintf(buffer, "Wall (%d) Hi-tag: ", i);
                    wall[i].hitag = getnumber16(buffer, wall[i].hitag, BTAG_MAX, 0+j);
                }
            }
            else
            {
                for (i=0; i<numsectors; i++)
                    if (inside_editor_curpos(i) == 1)
                    {
                        YAX_SKIPSECTOR(i);

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
                if (inside_editor_curpos(i) == 1)
                {
                    YAX_SKIPSECTOR(i);

                    Bsprintf(buffer, "Sector (%d) Ceilingpal: ", i);
                    sector[i].ceilingpal = getnumber16(buffer, sector[i].ceilingpal, M32_MAXPALOOKUPS, 0);

                    Bsprintf(buffer, "Sector (%d) Floorpal: ", i);
                    sector[i].floorpal = getnumber16(buffer, sector[i].floorpal, M32_MAXPALOOKUPS, 0);
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
                }
            }
#ifdef YAX_ENABLE
            else if (highlightsectorcnt > 0 && newnumwalls < 0)
            {
                ////////// YAX //////////
                static const char *cfs[2] = {"ceiling", "floor"};

                int32_t cf, thez, ulz[2]={0,0};
                int16_t bn, sandwichbunch=-1;

                if (numyaxbunches==YAX_MAXBUNCHES)
                {
                    message("Bunch limit of %d reached, cannot extend", YAX_MAXBUNCHES);
                    goto end_yax;
                }

                if (highlighted_sectors_components(0,0) != 1)
                {
                    message("Sectors to extend must be in one connected component");
                    goto end_yax;
                }

                cf = ask_above_or_below();
                if (cf==-1)
                    goto end_yax;

                thez = SECTORFLD(highlightsector[0],z, cf);
                for (i=0; i<highlightsectorcnt; i++)
                {
                    bn = yax_getbunch(highlightsector[i], cf);

                    if (sandwichbunch >= 0 && bn!=sandwichbunch)
                    {
                        message("When sandwiching extension, must select only sectors of one bunch");
                        goto end_yax;
                    }

                    if (bn >= 0)
                    {
                        if (cf==YAX_FLOOR)
                        {
                            if (sandwichbunch < 0 && i!=0)
                            {
                                message("When sandwiching extension, must select only sectors of the bunch");
                                goto end_yax;
                            }
                            sandwichbunch = bn;
                        }
                        else
                        {
                            message("Sector %d's %s is already extended", highlightsector[i], cfs[cf]);
                            goto end_yax;
                        }
                    }

                    if (SECTORFLD(highlightsector[i],z, cf) != thez)
                    {
                        message("Sector %d's %s height doesn't match sector %d's",
                                highlightsector[i], cfs[cf], highlightsector[0]);
                        goto end_yax;
                    }

                    if ((sandwichbunch>=0 || highlightsectorcnt>1) && SECTORFLD(highlightsector[i],stat, cf)&2)
                    {
                        message("Sector %ss must not be sloped%s", cfs[cf],
                                sandwichbunch>=0 ? "" : "if extending more than one");
                        goto end_yax;
                    }
                }

                if (sandwichbunch >= 0)
                {
                    // cf==YAX_FLOOR here

                    int32_t tempz, oldfz, swsecheight = DEFAULT_YAX_HEIGHT/4;
                    // highest floor z of lower sectors, lowest ceiling z of these sectors
                    int32_t minfloorz = INT32_MAX, maxceilz = INT32_MIN;

                    // some preparation for making the sandwich
                    if (highlightsectorcnt != yax_numsectsinbunch(sandwichbunch, YAX_FLOOR))
                    {
                        message("When sandwiching extension, must select all sectors of the bunch");
                        goto end_yax;
                    }

                    // "for i in sectors of sandwichbunch(floor)" is now the same as
                    // "for i in highlighted sectors"

                    oldfz = sector[highlightsector[0]].floorz;

                    // check if enough room in z
                    for (SECTORS_OF_BUNCH(sandwichbunch, YAX_CEILING, i))
                        for (WALLS_OF_SECTOR(i, j))
                        {
                            tempz = getflorzofslope(i, wall[j].x, wall[j].y);
                            minfloorz = min(minfloorz, tempz);
                        }
                    for (SECTORS_OF_BUNCH(sandwichbunch, YAX_FLOOR, i))
                        for (WALLS_OF_SECTOR(i, j))
                        {
                            tempz = getceilzofslope(i, wall[j].x, wall[j].y);
                            maxceilz = max(maxceilz, tempz);
                        }

                    if (minfloorz - maxceilz < 2*swsecheight)
                    {
                        message("Too little z headroom for sandwiching, need at least %d",
                                2*swsecheight);
                        goto end_yax;
                    }

                    if (maxceilz >= oldfz || oldfz >= minfloorz)
                    {
                        message("Internal error while sandwiching: oldfz out of bounds");
                        goto end_yax;
                    }

                    // maxceilz   ---|
                    //   ^           |
                    // ulz[0]        ^
                    //   ^          oldfz
                    // ulz[1]        ^
                    //   ^           |
                    // minfloorz  ---|

                    ulz[0] = (int32_t)(oldfz - swsecheight*((double)(oldfz-maxceilz)/(minfloorz-maxceilz)));
                    ulz[0] &= ~255;
                    ulz[1] = ulz[0] + swsecheight;

                    if (maxceilz >= ulz[0] || ulz[1] >= minfloorz)
                    {
                        message("Too little z headroom for sandwiching");
                        goto end_yax;
                    }
                }

                m = numwalls;
                Bmemset(visited, 0, sizeof(visited));
                // construct!
                for (i=0; i<highlightsectorcnt; i++)
                    for (WALLS_OF_SECTOR(highlightsector[i], j))
                    {
                        k = trace_loop(j, visited, NULL, NULL, !cf);
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

                for (i=m; i<numwalls; i++)  // try
                {
                    j = YAX_NEXTWALL(i, !cf);
                    if (j < 0)
                    {
                        message("Internal error while constructing sector: "
                                "YAX_NEXTWALL(%d, %d)<0!", i, !cf);
                        numwalls = m;
                        goto end_yax;
                    }
                    if (sandwichbunch >= 0)
                    {
                        if (YAX_NEXTWALL(j, cf) < 0)
                        {
                            message("Internal error while sandwiching (2): "
                                    "YAX_NEXTWALL(%d, %d)<0!", j, cf);
                            numwalls = m;
                            goto end_yax;
                        }
                    }
                }
                for (i=m; i<numwalls; i++)  // do!
                {
                    j = YAX_NEXTWALL(i, !cf);

                    if (sandwichbunch >= 0)
                    {
                        int16_t oynw = YAX_NEXTWALL(j, cf);
                        yax_setnextwall(j, cf, i);
                        yax_setnextwall(i, cf, oynw);
                        yax_setnextwall(oynw, !cf, i);
                    }
                    else
                    {
                        yax_setnextwall(j, cf, i);
                    }
                }

                // create new sector based on first highlighted one
                i = highlightsector[0];
                Bmemcpy(&sector[numsectors], &sector[i], sizeof(sectortype));
                sector[numsectors].wallptr = m;
                sector[numsectors].wallnum = numwalls-m;

                if (sandwichbunch < 0)
                {
                    if (SECTORFLD(i,stat, cf)&2)
                        setslope(numsectors, !cf, SECTORFLD(i,heinum, cf));
                    else
                        setslope(numsectors, !cf, 0);
                    setslope(numsectors, cf, 0);

                    SECTORFLD(numsectors,z, !cf) = SECTORFLD(i,z, cf);
                    SECTORFLD(numsectors,z, cf) = SECTORFLD(i,z, cf) - (1-2*cf)*DEFAULT_YAX_HEIGHT;
                }
                else
                {
                    for (SECTORS_OF_BUNCH(sandwichbunch, cf, i))
                        sector[i].floorz = ulz[0];
                    sector[numsectors].ceilingz = ulz[0];
                    sector[numsectors].floorz = ulz[1];
                    for (SECTORS_OF_BUNCH(sandwichbunch, !cf, i))
                        sector[i].ceilingz = ulz[1];
                }

                newnumwalls = numwalls;
                numwalls = m;

                SECTORFLD(numsectors,stat, !cf) &= ~1;  // no plax

                // restore red walls of the selected sectors
                for (i=0; i<highlightsectorcnt; i++)
                {
                    SECTORFLD(highlightsector[i],stat, cf) &= ~1;  // no plax

                    for (WALLS_OF_SECTOR(highlightsector[i], j))
                        if (wall[j].nextwall < 0)
                            checksectorpointer(j, highlightsector[i]);
                }

                // link
                if (sandwichbunch < 0)
                {
                    yax_setbunch(numsectors, !cf, numyaxbunches);
                    for (i=0; i<highlightsectorcnt; i++)
                        yax_setbunch(highlightsector[i], cf, numyaxbunches);
                }
                else
                {
                    yax_setbunch(numsectors, !cf, sandwichbunch);
                    // also relink
                    yax_setbunch(numsectors, cf, numyaxbunches);
                    for (SECTORS_OF_BUNCH(sandwichbunch, !cf, i))
                        yax_setbunch(i, !cf, numyaxbunches);
                }

                numwalls = newnumwalls;
                newnumwalls = -1;

                numsectors++;
                yax_update(0);
                yax_updategrays(pos.z);

                reset_highlightsector();

                if (sandwichbunch < 0)
                    message("Extended %ss of highlighted sectors, creating bunch %d",
                            cfs[cf], numyaxbunches-1);
                else
                    message("Sandwiched bunch %d, creating bunch %d",
                            sandwichbunch, numyaxbunches-1);
                asksave = 1;
            }
            else if (highlightcnt > 0)
            {
                /// 'punch' wall loop through extension

                int32_t loopstartwall = -1, numloopwalls, cf;
                int32_t srcsect, dstsect, ofirstwallofs;
                int16_t cb, fb, bunchnum;

                if (EDITING_MAP_P())
                {
                    printmessage16("Must not be editing map to punch loop");
                    goto end_yax;
                }

                if (numyaxbunches >= YAX_MAXBUNCHES)
                {
                    message("TROR bunch limit reached, cannot punch loop");
                    goto end_yax;
                }

                // determine start wall
                for (i=0; i<highlightcnt; i++)
                {
                    j = highlight[i];

                    if (j&16384)
                        continue;

                    // we only want loop-starting walls
                    if (j>0 && wall[j-1].point2==j)
                        continue;

                    if (clockdir(j)==CLOCKDIR_CCW)
                    {
                        YAX_SKIPWALL(j);

                        if (loopstartwall >= 0)
                        {
                            message("Must have a unique highlighted CCW loop to punch");
                            goto end_yax;
                        }

                        loopstartwall = j;
                    }
                }

                if (loopstartwall == -1)
                {
                    message("Didn't find any non-grayed out CCW loop start walls");
                    goto end_yax;
                }

                // determine sector
                srcsect = sectorofwall(loopstartwall);
                yax_getbunches(srcsect, &cb, &fb);
                if (cb < 0 && fb < 0)
                {
                    message("Ceiling or floor must be extended to punch loop");
                    goto end_yax;
                }

                /// determine c/f
                cf = -1;
                if (fb < 0)
                    cf = YAX_CEILING;
                else if (cb < 0)
                    cf = YAX_FLOOR;

                fade_editor_screen(-1);

                // query top/bottom
                if (cf == -1)
                {
                    char dachars[2] = {'a', 'z'};
                    cf = editor_ask_function("Punch loop above (a) or below (z)?", dachars, 2);
                    if (cf == -1)
                        goto end_yax;
                }
                else
                {
                    // ask even if only one choice -- I find it more
                    // consistent with 'extend sector' this way
                    if (-1 == editor_ask_function(cf==YAX_CEILING ? "Punch loop above (a)?" :
                                                  "Punch loop below (z)?", cf==YAX_CEILING?"a":"z", 1))
                        goto end_yax;
                }

                bunchnum = (cf==YAX_CEILING) ? cb : fb;

                // check 1
                j = loopstartwall;  // will be real start wall of loop
                numloopwalls = 1;  // will be number of walls in loop
                for (i=wall[loopstartwall].point2; i!=loopstartwall; i=wall[i].point2)
                {
                    numloopwalls++;
                    if (i < j)
                        j = i;

                    if ((show2dwall[i>>3]&(1<<(i&7)))==0)
                    {
                        message("All loop points must be highlighted to punch");
                        goto end_yax;
                    }

                    if (yax_getnextwall(loopstartwall, cf) >= 0 || yax_getnextwall(i, cf) >= 0)
                    {
                        // somewhat redundant, since it would also be caught by check 2
                        message("Loop walls must not already have TROR neighbors");
                        goto end_yax;
                    }

                    if (wall[loopstartwall].nextwall < 0 || wall[i].nextwall < 0)
                    {
                        message("INTERNAL ERROR: All loop walls are expected to be red");
                        goto end_yax;
                    }
                }
                loopstartwall = j;

                if (numwalls + 2*numloopwalls > MAXWALLS || numsectors+1 > MAXSECTORS)
                {
                    message("Punching loop through extension would exceed limits");
                    goto end_yax;
                }

                // get other-side sector, j==loopstartwall
                dstsect = yax_getneighborsect(wall[j].x, wall[j].y, srcsect, cf);
                if (dstsect < 0)
                {
                    message("Punch loop INTERNAL ERROR: dstsect < 0. Map corrupt?");
                    goto end_yax;
                }

                // check 2
                i = loopstartwall;
                do
                {
                    j = wall[i].point2;

                    for (WALLS_OF_SECTOR(dstsect, k))
                    {
                        vec2_t pint;
                        if (lineintersect2v((vec2_t *)&wall[i], (vec2_t *)&wall[j],
                                                (vec2_t *)&wall[k], (vec2_t *)&POINT2(k), &pint))
                        {
                            message("Loop lines must not intersect any destination sector's walls");
                            goto end_yax;
                        }
                    }
                }
                while ((i = j) != loopstartwall);

                // construct new loop and (dummy yet) sector
                Bmemcpy(&wall[numwalls], &wall[loopstartwall], numloopwalls*sizeof(walltype));
                newnumwalls = numwalls+numloopwalls;

                for (i=numwalls; i<newnumwalls; i++)
                {
                    wall[i].point2 += (numwalls - loopstartwall);
                    wall[i].nextsector = wall[i].nextwall = -1;
                }

                sector[numsectors].wallptr = numwalls;
                sector[numsectors].wallnum = numloopwalls;
                numsectors++; // temp

                // check 3
                for (SECTORS_OF_BUNCH(bunchnum, !cf, i))
                    for (WALLS_OF_SECTOR(i, j))
                    {
                        if (inside(wall[j].x, wall[j].y, numsectors-1)==1)
                        {
                            numsectors--;
                            newnumwalls = -1;
                            message("A point of bunch %d's sectors lies inside the loop to punch",
                                    bunchnum);
                            goto end_yax;
                        }
                    }

                numsectors--;

                // clear wall & sprite highlights
                //  TODO: see about consistency with update_highlight() after other ops
                reset_highlight();

                // construct the loop!
                i = AddLoopToSector(dstsect, &ofirstwallofs);

                if (i <= 0)
                {
                    message("Punch loop INTERNAL ERROR with AddLoopToSector!");
                }
                else
                {
                    int32_t oneinnersect = -1, innerdstsect = numsectors-1;

                    if (dstsect < srcsect)
                        loopstartwall += numloopwalls;

                    /// handle bunchnums! (specifically, create a new one)

                    // collect sectors inside source loop; for that, first break the
                    // inner->outer nextwall links
                    for (i=loopstartwall; i<loopstartwall+numloopwalls; i++)
                    {
                        // all src loop walls are red!
                        NEXTWALL(i).nextwall = NEXTWALL(i).nextsector = -1;
                        oneinnersect = wall[i].nextsector;
                    }

                    // vvv
                    // expect oneinnersect >= 0 here!  Assumption: we collect exactly
                    // one connected component of sectors
                    collect_sectors1(collsectlist[0], collsectbitmap[0],
                                     &collnumsects[0], oneinnersect, 0, 0);

                    // set new bunchnums
                    for (i=0; i<collnumsects[0]; i++)
                        yax_setbunch(collsectlist[0][i], cf, numyaxbunches);
                    yax_setbunch(innerdstsect, !cf, numyaxbunches);
                    // ^^^

                    // restore inner->outer nextwall links
                    for (i=loopstartwall; i<loopstartwall+numloopwalls; i++)
                    {
                        NEXTWALL(i).nextwall = i;
                        NEXTWALL(i).nextsector = srcsect;

                        // set yax-nextwalls!
                        j = (i-loopstartwall) + sector[dstsect].wallptr;
                        yax_setnextwall(i, cf, j);
                        yax_setnextwall(j, !cf, i);

                        yax_setnextwall(wall[i].nextwall, cf, wall[j].nextwall);
                        yax_setnextwall(wall[j].nextwall, !cf, wall[i].nextwall);
                    }

                    setfirstwall(dstsect, sector[dstsect].wallptr+ofirstwallofs);

                    message("Punched loop starting w/ wall %d into %s sector %d%s",
                            loopstartwall, cf==YAX_CEILING?"upper":"lower", dstsect,
                            (oneinnersect>=0) ? "" : " (ERRORS)");
                }

                mkonwinvalid();
                asksave = 1;

                yax_update(0);
                yax_updategrays(pos.z);
            }
end_yax: ;
#endif
        }

        if (highlightsectorcnt < 0)
        {
            if (((bstatus & 5) == 1 && highlightcnt <= 0) || ((bstatus & 5) == 1 && pointhighlight == -1) || keystatus[0x36])  //Right shift (point highlighting)
            {
                if (highlightcnt == 0)
                {
                    int32_t xx[] = { highlightx1, highlightx1, searchx, searchx, highlightx1 };
                    int32_t yy[] = { highlighty1, searchy, searchy, highlighty1, highlighty1 };

                    highlightx2 = searchx;
                    highlighty2 = searchy;
                    ydim16 = ydim-STATUS2DSIZ2;

                    plotlines2d(xx, yy, 5, -editorcolors[14]);
                }
                else if (pointhighlight == -1 || keystatus[0x36])
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
                    int32_t add=keystatus[0x28], sub=(!add && keystatus[0x27]), setop=(add||sub);

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
                        if (!setop)
                        {
                            Bmemset(show2dwall, 0, sizeof(show2dwall));
                            Bmemset(show2dsprite, 0, sizeof(show2dsprite));
                        }

                        if (linehighlight >= 0 && linehighlight < MAXWALLS)
                        {
                            i = linehighlight;
                            do
                            {
                                if (!sub)
                                    show2dwall[i>>3] |= (1<<(i&7));
                                else
                                    show2dwall[i>>3] &= ~(1<<(i&7));

                                // XXX: this selects too many walls, need something more like
                                //      those of dragpoint() -- could be still too many for
                                //      loop punching though
                                for (j=0; j<numwalls; j++)
                                    if (j!=i && wall[j].x==wall[i].x && wall[j].y==wall[i].y)
                                    {
                                        if (!sub)
                                            show2dwall[j>>3] |= (1<<(j&7));
                                        else
                                            show2dwall[j>>3] &= ~(1<<(j&7));
                                    }

                                i = wall[i].point2;
                            }
                            while (i != linehighlight);
                        }

                        update_highlight();
                    }
                    else
                    {
                        int32_t tx, ty, onlySprites=eitherCTRL;
                        int32_t accum_dragged_verts = 0;

                        if (!setop)
                        {
                            Bmemset(show2dwall, 0, sizeof(show2dwall));
                            Bmemset(show2dsprite, 0, sizeof(show2dsprite));
                        }

                        for (i=0; i<numwalls; i++)
                            wall[i].cstat &= ~(1<<14);

                        for (i=0; i<numwalls; i++)
                        {
                            if (onlySprites)
                                break;

                            YAX_SKIPWALL(i);

                            if (!m32_sideview)
                            {
                                tx = wall[i].x;
                                ty = wall[i].y;
//                                wall[i].cstat &= ~(1<<14);
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
                                {
                                    if (numgraysects > 0 || m32_sideview)
                                    {
                                        // Only called to find out which walls would get dragged:
                                        dragpoint(i, wall[i].x, wall[i].y, accum_dragged_verts);
                                        accum_dragged_verts = 1;
                                    }
                                    else
                                        show2dwall[i>>3] |= (1<<(i&7));
                                }
                                else
                                    show2dwall[i>>3] &= ~(1<<(i&7));
                            }
                        }

                        if (!sub && (numgraysects > 0 || m32_sideview))
                        {
                            for (i=0; i<numwalls; i++)
                                if (wall[i].cstat&(1<<14))
                                    show2dwall[i>>3] |= (1<<(i&7));
                        }

                        for (i=0; i<MAXSPRITES; i++)
                        {
                            if (sprite[i].statnum == MAXSTATUS)
                                continue;

                            //  v v v: if END pressed, also permit sprites from grayed out sectors
                            if (!keystatus[0xcf] && (unsigned)sprite[i].sectnum < MAXSECTORS)
                                YAX_SKIPSECTOR(sprite[i].sectnum);

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

                        for (i=0; i<numwalls; i++)
                            wall[i].cstat &= ~(1<<14);
                    }
                }
            }
        }

        if (highlightcnt < 0)
        {
            if (((bstatus & 4) && highlightsectorcnt <= 0) || ((bstatus & 4) && linehighlight == -1)
                || keystatus[0xb8])  //Right alt (sector highlighting)
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

                        plotlines2d(xx, yy, 5, -editorcolors[10]);
                    }
                }
                else
                {
                    // didmakered: 'bad'!
                    int32_t didmakered = (highlightsectorcnt<0) || eitherCTRL, hadouterpoint=0;
#ifdef YAX_ENABLE
                    for (i=0; i<MAXSECTORS>>3; i++)
                        hlorgraysectbitmap[i] = hlsectorbitmap[i]|graysectbitmap[i];
#endif
                    for (i=0; i<highlightsectorcnt; i++)
                    {
                        int16_t tmpsect = -1;

                        for (WALLS_OF_SECTOR(highlightsector[i], j))
                        {
//                            if (wall[j].nextwall >= 0)
//                                checksectorpointer(wall[j].nextwall,wall[j].nextsector);
                            if (wall[j].nextwall < 0)
                                didmakered |= !!checksectorpointer(j, highlightsector[i]);

                            if (!didmakered)
                            {
                                updatesectorexclude(wall[j].x, wall[j].y, &tmpsect, hlorgraysectbitmap);
                                if (tmpsect<0)
                                    hadouterpoint = 1;
                            }
                        }
#ifdef YAX_ENABLE
                        int16_t cb, fb;

                        yax_getbunches(highlightsector[i], &cb, &fb);
                        if (cb>=0 || fb>=0)
                        {
                            // TROR stuff in the pasted sectors would really
                            // complicate things, so don't allow this
                            didmakered=1;
                        }
#endif
                    }

                    if (!didmakered && !hadouterpoint && newnumwalls<0)
                    {
                        // fade the screen to have the user's attention
                        fade_editor_screen(-1);

                        didmakered |= !ask_if_sure("Insert outer loop and make red walls? (Y/N)", 0);
                        clearkeys();
                    }

                    if (!didmakered && !hadouterpoint && newnumwalls<0)
                    {
                        int16_t ignore, refsect;
                        int32_t n;
#ifdef YAX_ENABLE
                        int16_t refsectbn[2]={-1,-1};
                        int32_t refextcf=-1;
#endif
                        Bmemset(visited, 0, sizeof(visited));

                        for (i=0; i<highlightsectorcnt; i++)
                        {
                            for (WALLS_OF_SECTOR(highlightsector[i], j))
                            {
                                k = trace_loop(j, visited, &ignore, &refsect, -1);
                                if (k == 0)
                                    continue;
                                else if (k < 0)
                                    goto end_autoredwall;

                                if (ignore)
                                    continue;

                                // done tracing one outer loop
#ifdef YAX_ENABLE
                                yax_getbunches(refsect, &refsectbn[0], &refsectbn[1]);
                                if (refsectbn[0]>=0 || refsectbn[1]>=0)
                                {
                                    if (refsectbn[0]>=0 && refsectbn[1]>=0)
                                    {
                                        // at least one of ceiling/floor must be non-extended
                                        didmakered = 1;
                                    }
                                    else
                                    {
                                        // ... and the other must be non-sloped
                                        refextcf = (refsectbn[1]>=0);
                                        if (SECTORFLD(refsect,stat, !refextcf)&2)
                                            didmakered = 1;
                                    }
                                }

                                if (didmakered)
                                    goto end_autoredwall;

                                if (refextcf >= 0)
                                {
                                    int32_t refz = SECTORFLD(refsect,z, refextcf), tmpsect;
                                    int32_t neededzofs=0;

                                    // the reference sector is extended on one side
                                    // (given by refextcf) and non-sloped on the other
                                    if (highlighted_sectors_components(0,0) != 1)
                                    {
                                        message("Highlighted sectors must be in one connected component");
                                        goto end_autoredwall;
                                    }

                                    for (m=0; m<highlightsectorcnt; m++)
                                    {
                                        tmpsect = highlightsector[m];
                                        yax_setbunch(tmpsect, refextcf, refsectbn[refextcf]);
                                        // walls: not needed, since they're all inner to the bunch

                                        SECTORFLD(tmpsect,z, refextcf) = refz;
                                        setslope(tmpsect, refextcf, 0);
                                        if (refextcf==0)
                                            neededzofs = max(neededzofs, refz-sector[tmpsect].floorz);
                                        else
                                            neededzofs = max(neededzofs, sector[tmpsect].ceilingz-refz);
                                    }

                                    if (neededzofs > 0)
                                    {
                                        neededzofs += ksgn(neededzofs)*(512<<4);
                                        neededzofs &= ~((256<<4)-1);
                                        if (refextcf==1)
                                            neededzofs *= -1;
                                        for (m=0; m<highlightsectorcnt; m++)
                                            SECTORFLD(highlightsector[m],z, !refextcf) += neededzofs;
                                    }
                                }
#endif
                                wall[k-1].point2 = numwalls;  // close the loop
                                newnumwalls = k;
                                n = (newnumwalls-numwalls);  // number of walls in just constructed loop

                                if (clockdir(numwalls)==CLOCKDIR_CW)
                                {
                                    int16_t begwalltomove = sector[refsect].wallptr+sector[refsect].wallnum;
                                    int32_t onwwasvalid = onwisvalid();

                                    flipwalls(numwalls, newnumwalls);

                                    sector[refsect].wallnum += n;
                                    if (refsect != numsectors-1)
                                    {
                                        uwalltype *tmpwall = (uwalltype *)Xmalloc(n * sizeof(walltype));
                                        int16_t *tmponw = (int16_t *)Xmalloc(n * sizeof(int16_t));

                                        for (m=0; m<numwalls; m++)
                                        {
                                            if (wall[m].nextwall >= begwalltomove)
                                                wall[m].nextwall += n;
                                        }
#ifdef YAX_ENABLE
                                        yax_tweakwalls(begwalltomove, n);
#endif
                                        for (m=refsect+1; m<numsectors; m++)
                                            sector[m].wallptr += n;
                                        for (m=begwalltomove; m<numwalls; m++)
                                            wall[m].point2 += n;
                                        for (m=numwalls; m<newnumwalls; m++)
                                            wall[m].point2 += (begwalltomove-numwalls);

                                        Bmemcpy(tmponw, &onextwall[numwalls], n*sizeof(int16_t));
                                        Bmemmove(&onextwall[begwalltomove+n], &onextwall[begwalltomove],
                                                 (numwalls-begwalltomove)*sizeof(int16_t));
                                        Bmemcpy(&onextwall[begwalltomove], tmponw, n*sizeof(int16_t));

                                        Bmemcpy(tmpwall, &wall[numwalls], n*sizeof(walltype));
                                        Bmemmove(&wall[begwalltomove+n], &wall[begwalltomove],
                                                 (numwalls-begwalltomove)*sizeof(walltype));
                                        Bmemcpy(&wall[begwalltomove], tmpwall, n*sizeof(walltype));

                                        Bfree(tmpwall);
                                        Bfree(tmponw);
                                    }
                                    numwalls = newnumwalls;
                                    newnumwalls = -1;

                                    mkonwinvalid();

                                    for (m=begwalltomove; m<begwalltomove+n; m++)
                                        if (checksectorpointer(m, refsect) > 0)
                                            if (onwwasvalid && onextwall[wall[m].nextwall]>=0)
                                            {
//initprintf("%d %d\n", m, onextwall[wall[m].nextwall]);
                                                copy_some_wall_members(m, onextwall[wall[m].nextwall], 0);
                                            }

#ifndef YAX_ENABLE
                                    message("Attached new inner loop to sector %d", refsect);
#else
                                    {
                                        const char *cfstr[2] = {"ceiling","floor"};
                                        message("Attached new inner loop to %s%ssector %d",
                                                refextcf>=0 ? cfstr[refextcf] : "",
                                                refextcf>=0 ? "-extended " : "", refsect);
                                    }

                                    asksave = 1;

                                    if (refextcf >= 0)
                                    {
                                        yax_update(0);
                                        goto end_autoredwall;
                                    }
#endif
                                }
                            }
                        }
end_autoredwall:
                        newnumwalls = -1;
#ifdef YAX_ENABLE
                        yax_updategrays(pos.z);
#endif
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
                    int32_t const add=keystatus[0x28], sub=(!add && keystatus[0x27]), setop=(add||sub);
                    int32_t const pointsel = eitherCTRL;
                    int32_t tx,ty;
#ifdef YAX_ENABLE
                    // home: ceilings, end: floors
                    int32_t fb, bunchsel = keystatus[0xcf] ? 1 : (keystatus[0xc7] ? 0 : -1);
                    uint8_t bunchbitmap[YAX_MAXBUNCHES>>3];
                    Bmemset(bunchbitmap, 0, sizeof(bunchbitmap));
#endif
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
#ifdef YAX_ENABLE
                            if (bunchsel!=-1 && (fb = yax_getbunch(i, YAX_FLOOR))>=0)
                            {
                                if ((sub || (graysectbitmap[i>>3]&(1<<(i&7)))==0) &&
                                        (bunchbitmap[fb>>3]&(1<<(fb&7)))==0)
                                {
                                    bunchbitmap[fb>>3] |= (1<<(fb&7));
                                    for (SECTORS_OF_BUNCH(fb, bunchsel, j))
                                        handlesecthighlight1(j, sub, 1);
                                }
                            }
                            else
#endif
                                handlesecthighlight1(i, sub, eitherSHIFT);
                        }
                    }

                    update_highlightsector();
                    ovh_whiteoutgrab(0);
                }
            }
        }

        if (((bstatus&1) < (oldmousebstatus&1)) && highlightsectorcnt < 0)  //after dragging
        {
            int32_t runi, numdelpoints=0;
            int32_t havedrawnwalls = (newnumwalls!=-1), restorestat=1;

            // restorestat is set to 2 whenever the drawn walls should NOT be
            // restored afterwards

            int32_t err = backup_drawn_walls(0);

            if (err)
            {
                message("Error backing up drawn walls (code %d)!", err);
                goto end_after_dragging;
            }

            j = 1;
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

                for (i=0; i<2; i++)
                {
                    if (dragwall[i] < 0)
                        break;

                    if (keeptexturestretch && olen[i] != 0)
                    {
#ifndef YAX_ENABLE
                        j = dragwall[i];
#else
                        int32_t cf;
                        for (YAX_ITER_WALLS(dragwall[i], j, cf))
#endif
                        {
                            int32_t nw = wall[j].nextwall;

                            k = getlenbyrep(olen[i], wall[j].xrepeat);
                            fixxrepeat(j, k);
                            if (nw >= 0)
                            {
                                k = getlenbyrep(olen[i], wall[nw].xrepeat);
                                fixxrepeat(nw, k);
                            }
                        }
                    }
                }
            }
            else if ((pointhighlight&0xc000) == 16384)
            {
                dax = sprite[pointhighlight&16383].x;
                day = sprite[pointhighlight&16383].y;
            }

            dragwall[0] = dragwall[1] = -1;

            // attempt to delete some points
            for (runi=0; runi<3; runi++)  // check, tweak, carry out
                for (i=numwalls-1; i>=0; i--)
                {
                    if (runi==0)
                        wall[i].cstat &= ~(1<<14);

                    if (wall[i].x == POINT2(i).x && wall[i].y == POINT2(i).y)
                    {
                        if (havedrawnwalls)
                        {
                            if (i==ovh.suckwall || (ovh.split && i==ovh.splitstartwall))
                            {
                                // if we're about to delete a wall that participates
                                // in splitting, discard the already drawn walls
                                restorestat = 2;
                            }
                            else if (runi == 1)
                            {
                                // correct drawn wall anchors
                                if (ovh.suckwall > i)
                                    ovh.suckwall--;
                                if (ovh.split && ovh.splitstartwall > i)
                                    ovh.splitstartwall--;
                            }
                        }

                        if (runi == 0)
                        {
                            int32_t sectnum = sectorofwall(i);
                            if (sector[sectnum].wallnum <= 3)
                            {
                                message("Deleting wall %d would leave sector %d with %d walls.",
                                        i, sectnum, sector[sectnum].wallnum-1);
                                goto end_after_dragging;
                            }

                            sectnum = wall[i].nextsector;
                            if (sectnum >= 0 && sector[sectnum].wallnum <= 3)
                            {
                                message("Deleting wall %d would leave sector %d with %d walls.",
                                        i, sectnum, sector[sectnum].wallnum-1);
                                goto end_after_dragging;
                            }
                        }
                        else
                        {
                            deletepoint(i, runi);
                            if (runi==2)
                                numdelpoints++;
                        }
                    }
                }

            if (numdelpoints)
            {
                if (numdelpoints > 1)
                    message("Deleted %d points%s", numdelpoints,
                            (havedrawnwalls && restorestat==2) ? " and cleared drawn walls":"");
                else
                    printmessage16("Point deleted%s", (havedrawnwalls && restorestat==2) ?
                                   ", cleared drawn walls":"");
                asksave = 1;
            }
            else
            {
                for (i=0; i<numwalls; i++)     //make new red lines?
                {
                    YAX_SKIPWALL(i);

                    if ((wall[i].x == dax && wall[i].y == day)
                        || (POINT2(i).x == dax && POINT2(i).y == day))
                    {
                        checksectorpointer(i, sectorofwall(i));
//                    fixrepeats(i);
                        asksave = 1;
                    }
                }
            }
#ifdef YAX_ENABLE
            yax_update(0);
            yax_updategrays(pos.z);
#endif
end_after_dragging:
            backup_drawn_walls(restorestat);
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
                        if (inside_editor_curpos(highlightsector[i])==1)
                        {
                            cursectorhighlight = highlightsector[i];
                            break;
                        }

                    if (cursectorhighlight >= 0 && cursectorhighlight < numsectors)
                    {
                        //You clicked inside one of the flashing sectors!
                        sectorhighlightstat = 1;

                        dax = mousxplc;
                        day = mousyplc;
                        if (gridlock && grid > 0)
                            locktogrid(&dax, &day);

                        sectorhighlightx = dax;
                        sectorhighlighty = day;
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
#ifdef YAX_ENABLE
                    if (!hl_all_bunch_sectors_p())
                        printmessage16("To drag extended sectors, all sectors of a bunch must be selected");
                    else
#endif
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
                {
                    pointhighlight = getpointhighlight(mousxplc, mousyplc, pointhighlight);

                    if (pointhighlight >= 0 && (pointhighlight&0xc000)==0)
                    {
                        dragwall[0] = lastwall(pointhighlight);
                        dragwall[1] = pointhighlight;
                        olen[0] = wallength(dragwall[0]);
                        olen[1] = wallength(dragwall[1]);
                    }
                }

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
                                spritetype *daspr = &sprite[highlight[i]&16383];

                                daspr->x += dax;
                                daspr->y += day;
                                setspritez(daspr-sprite, (const vec3_t *)daspr);
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

                            dragpoint(pointhighlight,dax,day,2);
                            wall[lastwall(pointhighlight)].cstat |= (1<<14);
                        }
                        else if ((pointhighlight&0xc000) == 16384)
                        {
                            int32_t daspr=pointhighlight&16383;
                            int16_t osec=sprite[daspr].sectnum, nsec=osec;
                            vec3_t vec, ovec;

                            Bmemcpy(&ovec, (vec3_t *)&sprite[daspr], sizeof(vec3_t));
                            vec.x = dax;
                            vec.y = day;
                            vec.z = sprite[daspr].z;
                            if (setspritez(daspr, &vec) == -1 && osec>=0)
                            {
                                updatesectorbreadth(dax, day, &nsec);

                                if (nsec >= 0)
                                {
                                    sprite[daspr].x = dax;
                                    sprite[daspr].y = day;
                                    // z updating is after we released the mouse button
                                    if (sprite[daspr].sectnum != nsec)
                                        changespritesect(daspr, nsec);
                                }
                                else
                                    Bmemcpy(&sprite[daspr], &ovec, sizeof(vec3_t));
                            }
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

        if (bstatus&(2) && (!(bstatus&5) || pointhighlight > 0 || highlightcnt > 0 || highlightsectorcnt > 0))  // change arrow position
        {
            if (eitherCTRL)
            {
                int16_t cursectornum;

                for (cursectornum=0; cursectornum<numsectors; cursectornum++)
                    if (inside_editor_curpos(cursectornum) == 1)
                        break;

                if (cursectornum < numsectors)
                {
                    if (pointhighlight >= 16384)
                        CallExtEditSpriteData(pointhighlight-16384);
                    else if ((linehighlight >= 0) && ((bstatus&1) || sectorofwall(linehighlight) == cursectornum))
                        CallExtEditWallData(linehighlight);
                    else if (cursectornum >= 0)
                        CallExtEditSectorData(cursectornum);
                }

                bstatus &= ~2;
            }
            else
            {
                if (m32_sideview && (bstatus&4))
                {
                    pos.z += divscale18(searchy-midydim16,zoom);
                    getpoint(searchx,midydim16, &pos.x, &pos.y);
#ifdef YAX_ENABLE
                    yax_updategrays(pos.z);
#endif
                }
                else
                {
                    pos.x = mousxplc;
                    pos.y = mousyplc;
                }

                if (m32_sideview)
                {
                    int32_t opat=drawlinepat;

                    y1 = INT32_MAX;

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
                    if (y1 != INT32_MAX)
                        drawline16base(halfxdim16,midydim16, 0,0, 0,y1, editorcolors[14]);
//                    else
//                        drawline16base(halfxdim16,midydim16, 0,0, 0,getscreenvdisp(-pos.z, zoom), editorcolors[14]);
                    drawlinepat = opat;
                }

                searchx = halfxdim16;
                searchy = midydim16;
            }
        }
//        else if ((oldmousebstatus&6) > 0)
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
            if (!m32_sideview && EDITING_MAP_P())
                message("Must not be editing map while switching to side view mode.");
            else
            {
                m32_sideview = !m32_sideview;
                printmessage16("Side view %s", m32_sideview?"enabled":"disabled");
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

        if (m32_sideview && (eitherSHIFT || (bstatus&(16|32))))
        {
            if ((DOWN_BK(MOVEUP) || (bstatus&16)) && m32_sideelev < 512)
            {
                if (DOWN_BK(MOVEUP))
                    m32_sideelev += synctics<<1;
                if (bstatus&16)
                    m32_sideelev += 4<<1;

                if (m32_sideelev > 512)
                    m32_sideelev = 512;
                _printmessage16("Sideview elevation: %d", m32_sideelev);
            }
            if ((DOWN_BK(MOVEDOWN) || (bstatus&32)) && m32_sideelev > 0)
            {
                if (DOWN_BK(MOVEDOWN))
                    m32_sideelev -= synctics<<1;
                if (bstatus&32)
                    m32_sideelev -= 4<<1;

                if (m32_sideelev < 0)
                    m32_sideelev = 0;
                _printmessage16("Sideview elevation: %d", m32_sideelev);
            }
        }
        else
        {
            int32_t didzoom=0;

            if ((DOWN_BK(MOVEUP) || (bstatus&16)) && zoom < 39936)
            {
                if (DOWN_BK(MOVEUP))
                {
                    ztarget += (synctics*(ztarget>>4))>>(eitherSHIFT<<1);

                    if (zoom < 64)
                        ztarget += (synctics*(ztarget>>4)) * (eitherSHIFT);
                }
                if (bstatus&16)
                    ztarget += 4*(ztarget>>4);

                if (zoom < 24) zoom += 2;
                didzoom = 1;
            }
            if ((DOWN_BK(MOVEDOWN) || (bstatus&32)) && zoom > 16)
            {
                if (DOWN_BK(MOVEDOWN))
                {
                    ztarget -= (synctics*(ztarget>>4))>>(eitherSHIFT<<1);

                    if (zoom < 64)
                        ztarget -= (synctics * (ztarget >> 4)) * (eitherSHIFT);
                }
                if (bstatus&32)
                    ztarget -= 4*(ztarget>>4);

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
                ztarget = clamp(ztarget, 16, 39936);

                _printmessage16("Zoom: %d",ztarget);
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

            if (eitherSHIFT)
            {
                searchlock = 1-searchlock;
                silentmessage("Selection lock %s", searchlock ? "on" : "off");
            }
            else
            {
                gridlock = !gridlock;
                silentmessage("Grid locking %s", gridlock ? "on" : "off");
            }
        }

        if (keystatus[0x24])  // J (join sectors)
        {
            keystatus[0x24] = 0;

            if (newnumwalls >= 0)
            {
                printmessage16("Can't join sectors while editing.");
                goto end_join_sectors;
            }

#ifdef YAX_ENABLE
            if (highlightsectorcnt > 0 && eitherCTRL)
            {
                // [component][ceiling(0) or floor(1)]
                // compstat: &1: "has extension", &2: "differ in z", &4: "sloped", -1: "uninited"
                int32_t cf, comp, compstat[2][2]={{-1,-1},{-1,-1}}, compcfz[2][2];

                // joinstat: join what to what?
                //  &1: ceil(comp 0) <-> flor(comp 1),  &2: flor(comp 0) <-> ceil(comp 1)
                //  (doesn't yet say which is stationary)
                // movestat: which component can be displaced?
                //  &1: first,  &2: second
                int32_t askres, joinstat, needsdisp, moveonwp;
                int32_t movestat, dx=0,dy=0,dz, delayerr=0;

                int32_t numouterwalls[2] = {0,0}, numowals;
                static int16_t outerwall[2][MAXWALLS];
                const uwalltype *wal0, *wal1, *wal0p2, *wal1p2;

                // join sector ceilings/floors to a new bunch
                if (numyaxbunches==YAX_MAXBUNCHES)
                {
                    message("Bunch limit of %d reached, cannot join", YAX_MAXBUNCHES);
                    goto end_join_sectors;
                }

                // first, see whether we have exactly two connected components
                // wrt wall[].nextsector
                if (highlighted_sectors_components(0,0) != 2)
                {
                    message("Sectors must be partitioned in two components to join");
                    goto end_join_sectors;
                }

                for (k=0; k<highlightsectorcnt; k++)
                {
                    j = highlightsector[k];
                    comp = !!(collsectbitmap[1][j>>3]&(1<<(j&7)));

                    for (cf=0; cf<2; cf++)
                    {
                        if (compstat[comp][cf]==-1)
                        {
                            compstat[comp][cf] = 0;
                            compcfz[comp][cf] = SECTORFLD(j,z, cf);
                        }

                        if (yax_getbunch(j, cf)>=0)
                            compstat[comp][cf] |= 1;
                        if (SECTORFLD(j,z, cf) != compcfz[comp][cf])
                            compstat[comp][cf] |= 2;
                        if (SECTORFLD(j,stat, cf)&2)
                            compstat[comp][cf] |= 4;

                        compcfz[comp][cf] = SECTORFLD(j,z, cf);
                    }
                }

                // check for consistency
                joinstat = 0;
                if (!compstat[0][YAX_CEILING] && !compstat[1][YAX_FLOOR])
                    joinstat |= 1;
                if (!compstat[0][YAX_FLOOR] && !compstat[1][YAX_CEILING])
                    joinstat |= 2;

                if (joinstat==0)
                {
                    message("No consistent joining combination found");
                    OSD_Printf("comp0: c=%d,f=%d;  comp1: c=%d,f=%d  (1:extended, 2:z mismatch, 4:sloped)\n",
                               compstat[0][YAX_CEILING], compstat[0][YAX_FLOOR],
                               compstat[1][YAX_CEILING], compstat[1][YAX_FLOOR]);
                    //for (i=0; i<2; i++) for (j=0; j<2; j++) message("%d", compstat[i][j]);
                    goto end_join_sectors;
                }
                if (joinstat==3)
                {
                    if (compcfz[0][YAX_CEILING] != compstat[1][YAX_FLOOR])
                        joinstat &= 1;
                    if (compcfz[0][YAX_CEILING] != compstat[1][YAX_FLOOR])
                        joinstat &= 2;

                    if (joinstat == 0)
                        joinstat = 3;  // we couldn't disambiguate
                }

                for (comp=0; comp<2; comp++)
                    for (k=0; k<collnumsects[comp]; k++)
                    {
                        i = collsectlist[comp][k];
                        for (WALLS_OF_SECTOR(i, j))
                            if (wall[j].nextwall < 0)
                                outerwall[comp][numouterwalls[comp]++] = j;
                    }

                if (numouterwalls[0] != numouterwalls[1])
                {
                    message("Number of outer walls must be equal for both components"
                            " (have %d and %d)", numouterwalls[0], numouterwalls[1]);
                    if (numouterwalls[0]>0 && numouterwalls[1]>0)
                        delayerr = 1;
                    else
                        goto end_join_sectors;
                }
                numowals = min(numouterwalls[0], numouterwalls[1]);

                // now sort outer walls 'geometrically'
                for (comp=0; comp<2; comp++)
                    sort_walls_geometrically(outerwall[comp], numouterwalls[comp]);

                for (k=0; k<numowals; k++)
                {
                    wal0 = (uwalltype *)&wall[outerwall[0][k]];
                    wal1 = (uwalltype *)&wall[outerwall[1][k]];

                    wal0p2 = (uwalltype *)&wall[wal0->point2];
                    wal1p2 = (uwalltype *)&wall[wal1->point2];

                    if (k==0)
                    {
                        dx = wal1->x - wal0->x;
                        dy = wal1->y - wal0->y;
                    }

                    if (wal1->x - wal0->x != dx || wal1->y - wal0->y != dy ||
                            wal1p2->x - wal0p2->x != dx || wal1p2->y - wal0p2->y != dy)
                    {
                        pos.x = wal0->x + (wal0p2->x - wal0->x)/4;
                        pos.y = wal0->y + (wal0p2->y - wal0->y)/4;
                        pos.z = getflorzofslope(sectorofwall(wal0-(uwalltype *)wall), pos.x, pos.y);

                        if (!delayerr)
                            message("Outer wall coordinates must coincide for both components");
                        OSD_Printf("wal0:%d (%d,%d)--(%d,%d)\n",(int)(wal0-(uwalltype *)wall),
                                   wal0->x,wal0->y, wal0p2->x,wal0p2->y);
                        OSD_Printf("wal1:%d (%d,%d)--(%d,%d)\n",(int)(wal1-(uwalltype *)wall),
                                   wal1->x,wal1->y, wal1p2->x,wal1p2->y);

                        goto end_join_sectors;
                    }
                }

                if (delayerr)
                    goto end_join_sectors;

                if (joinstat == 3)
                {
                    char askchars[2] = {'1', 'v'};

                    // now is a good time to ask...
                    for (comp=0; comp<2; comp++)
                        for (k=0; k<collnumsects[comp]; k++)
                            fillsector_notrans(collsectlist[comp][k], comp==0 ? 159 : editorcolors[11]);

                    fade_editor_screen(editorcolors[11] | (159<<8));

                    char buffer[128];
                    Bsnprintf(buffer, sizeof(buffer), "Z differences | yellow ceiling w/ blue floor: %d, vice versa: %d",
                              compcfz[YAX_CEILING][0]-compcfz[YAX_FLOOR][1],
                              compcfz[YAX_CEILING][1]-compcfz[YAX_FLOOR][0]);
                    printext16(8, ydim-STATUS2DSIZ2-12, editorcolors[15], -1, buffer, 0);

                    askres = editor_ask_function("Connect yellow ceiling w/ blue floor (1) or (v)ice versa?", askchars, 2);
                    if (askres==-1)
                        goto end_join_sectors;
                    joinstat &= (1<<askres);
                }

                joinstat--;  // 0:ceil(0)<->flor(1), 1:ceil(1)<->flor(0)

                dz = compcfz[1][!joinstat] - compcfz[0][joinstat];
                needsdisp = (dx || dy || dz);

                if (needsdisp)
                {
                    // a component is more likely to be displaced if it's not
                    // extended on the non-joining side
                    movestat = (!(compstat[0][!joinstat]&1)) | ((!(compstat[1][joinstat]&1))<<1);
                    if (!movestat)
                    {
                        movestat = 3;
//                        message("Internal error while TROR-joining: movestat inconsistent!");
//                        goto end_join_sectors;
                    }

                    if (movestat==3)
                    {
                        char askchars[2] = {'y', 'b'};

                        for (comp=0; comp<2; comp++)
                            for (k=0; k<collnumsects[comp]; k++)
                                fillsector_notrans(collsectlist[comp][k], comp==0 ? 159 : editorcolors[11]);

                        fade_editor_screen(editorcolors[11] | (159<<8));
                        askres = editor_ask_function("Move (y)ellow or (b)lue component?", askchars, 2);
                        if (askres==-1)
                            goto end_join_sectors;
                        movestat &= (1<<askres);
                    }

                    movestat--;  // 0:move 1st, 1:move 2nd component
                    if (movestat==1)
                        dx*=-1, dy*=-1, dz*=-1;

                    moveonwp = 0;
                    if (onwisvalid())
                    {
                        static int16_t ocollsectlist[MAXSECTORS];
                        static uint8_t tcollbitmap[MAXSECTORS>>3];
                        int16_t ocollnumsects=collnumsects[movestat], tmpsect;

                        Bmemcpy(ocollsectlist, collsectlist[movestat], ocollnumsects*sizeof(int16_t));
                        Bmemset(tcollbitmap, 0, sizeof(tcollbitmap));

                        for (k=0; k<ocollnumsects; k++)
                            for (WALLS_OF_SECTOR(ocollsectlist[k], j))
                            {
                                if (onextwall[j] < 0)
                                    continue;

                                tmpsect = sectorofwall(onextwall[j]);
                                sectors_components(1, &tmpsect, 1,0);

                                for (m=0; m<(numsectors+7)>>3; m++)
                                    tcollbitmap[m] |= collsectbitmap[0][m];
                                moveonwp = 1;
                            }

                        if (moveonwp)
                        {
                            int32_t movecol = movestat==0 ? 159 : editorcolors[11];
                            for (i=0; i<numsectors; i++)
                                if (tcollbitmap[i>>3]&(1<<(i&7)))
                                    fillsector_notrans(i, editorcolors[12]);

                            fade_editor_screen(editorcolors[12] | (movecol<<8));
                            moveonwp = ask_if_sure("Also move formerly wall-connected sectors?",0);
                            if (moveonwp==-1)
                                goto end_join_sectors;
                        }
                    }

                    // now need to collect them wrt. the nextsector but also
                    // the yax-nextsector relation
                    if (highlighted_sectors_components(1,moveonwp) != 2)
                    {
                        message("Must not have TROR connections between the two components");
                        goto end_join_sectors;
                    }

                    // displace!
                    for (k=0; k<collnumsects[movestat]; k++)
                    {
                        i = collsectlist[movestat][k];

                        sector[i].floorz += dz;
                        sector[i].ceilingz += dz;

                        for (WALLS_OF_SECTOR(i, j))
                        {
                            wall[j].x += dx;
                            wall[j].y += dy;
                        }

                        for (j=headspritesect[i]; j>=0; j=nextspritesect[j])
                        {
                            sprite[j].x += dx;
                            sprite[j].y += dy;
                            sprite[j].z += dz;
                        }
                    }

                    // restore old components, i.e. only the bunch sectors
                    highlighted_sectors_components(0,0);

                }  // end if (needsdisp)

                /*** construct the YAX connection! ***/
                for (comp=0; comp<2; comp++)
                {
                    // walls
                    for (j=0; j<numowals; j++)
                        yax_setnextwall(outerwall[comp][j], comp^joinstat, outerwall[!comp][j]);

                    // sectors
                    for (k=0; k<collnumsects[comp]; k++)
                    {
                        i = collsectlist[comp][k];
                        yax_setbunch(i, comp^joinstat, numyaxbunches);
                        SECTORFLD(i,stat, comp^joinstat) &= ~1;  // no plax

                        // restore red walls AFTER setting nextwalls
                        // (see checksectorpointer() for why)
                        for (WALLS_OF_SECTOR(i, j))
                            if (wall[j].nextwall < 0)
                                checksectorpointer(j, i);
                    }
                }

                reset_highlightsector();

                yax_update(0);
                yax_updategrays(pos.z);

                message("Joined highlighted sectors to new bunch %d", numyaxbunches);
                asksave = 1;
            }
            else
#endif  // defined YAX_ENABLE
            if (joinsector[0] < 0)
            {
                int32_t numjoincandidates = 0;
                char *origframe=NULL;

                for (i=0; i<numsectors; i++)
                {
                    YAX_SKIPSECTOR(i);
                    numjoincandidates += (inside_editor_curpos(i) == 1);
                }

                for (i=0; i<numsectors; i++)
                {
                    YAX_SKIPSECTOR(i);
                    if (inside_editor_curpos(i) == 1)
                    {
                        if (numjoincandidates > 1)
                        {
                            if (!bakframe_fillandfade(&origframe, i, "Use this as first joining sector? (Y/N)"))
                                continue;
                        }

                        joinsector[0] = i;
                        printmessage16("Join sector - press J again on sector to join with.");
                        break;
                    }
                }

                Bfree(origframe);
            }
            else
            {
                char *origframe = NULL;
                int32_t numjoincandidates = 0;

                joinsector[1] = -1;

                for (i=0; i<numsectors; i++)
                {
                    YAX_SKIPSECTOR(i);
                    numjoincandidates += (inside_editor_curpos(i) == 1);
                }

                for (i=0; i<numsectors; i++)
                {
                    YAX_SKIPSECTOR(i);

                    if (inside_editor_curpos(i) == 1)
                    {
                        if (numjoincandidates > 1)
                        {
                            if (!bakframe_fillandfade(&origframe, i, "Use this as second joining sector? (Y/N)"))
                                continue;

                            DO_FREE_AND_NULL(origframe);
                        }

                        joinsector[1] = i;

                        const int s1to0wall = find_nextwall(i, joinsector[0]);
                        const int s0to1wall = s1to0wall == -1 ? -1 : wall[s1to0wall].nextwall;
#ifdef YAX_ENABLE
                        int16_t jbn[2][2];  // [join index][c/f]

                        for (k=0; k<2; k++)
                            yax_getbunches(joinsector[k], &jbn[k][YAX_CEILING], &jbn[k][YAX_FLOOR]);
#endif
                        // pressing J into the same sector is the same as saying 'no'
                        //                     v----------------v
                        if (s1to0wall == -1 && i != joinsector[0])
                        {
                            int32_t good = 1;
#ifdef YAX_ENABLE
                            if (jbn[0][0]>=0 || jbn[0][1]>=0 || jbn[1][0]>=0 || jbn[1][1]>=0)
                            {
                                message("Joining non-adjacent extended sectors not allowed!");
                                good = 0;
                            }
#endif
                            if (!m32_script_expertmode)
                            {
                                message("Joining non-adjacent disabled in non-expert mode");
                                good = 0;
                            }

                            if (!good)
                            {
                                joinsector[0] = joinsector[1] = -1;
                                goto end_join_sectors;
                            }

                            {
                                fillsector_notrans(i, editorcolors[9]);
                                fillsector_notrans(joinsector[0], editorcolors[9]);
                                fade_editor_screen(editorcolors[9]);

                                if (!ask_if_sure("Really join non-adjacent sectors? (Y/N)", 0))
                                    joinsector[1] = joinsector[0];
                            }
                        }
#ifdef YAX_ENABLE
                        // unequal bunchnums (bitmap): 1:above, 2:below
                        int uneqbn =
                            (jbn[0][YAX_CEILING] != jbn[1][YAX_CEILING]) |
                            ((jbn[0][YAX_FLOOR] != jbn[1][YAX_FLOOR])<<1);

                        if (uneqbn)
                        {
                            const int32_t cf=YAX_FLOOR;
                            int32_t whybad=0;

                            if (uneqbn == 1)
                            {
                                OSD_Printf("Can't join two sectors with different ceiling bunchnums."
                                           " To make them equal, join their upper neighbor's floors.\n");
                                printmessage16("Can't join two sectors with different ceiling bunchnums. See OSD");
                                joinsector[0] = joinsector[1] = -1;
                                goto end_join_sectors;
                            }
                            if (s0to1wall < 0)
                            {
                                printmessage16("INTERNAL ERROR: nextwalls inconsistent!");
                                joinsector[0] = joinsector[1] = -1;
                                goto end_join_sectors;
                            }

                            // both must be extended
                            if (jbn[0][cf]<0 || jbn[1][cf]<0)
                                uneqbn &= ~(1<<cf), whybad|=1;
                            // if any sloped, can't join
                            if ((SECTORFLD(joinsector[0],stat, cf)&2) || (SECTORFLD(joinsector[1],stat, cf)&2))
                                uneqbn &= ~(1<<cf), whybad|=2;
                            // if on unequal heights, can't join either
                            if (SECTORFLD(joinsector[0],z, cf) != SECTORFLD(joinsector[1],z, cf))
                                uneqbn &= ~(1<<cf), whybad|=4;

                            // check whether the lower neighbors have a red-wall link to each other
                            const int jsynw[2] = {
                                yax_getnextwall(s0to1wall, cf),
                                yax_getnextwall(s1to0wall, cf)
                            };

                            if (jsynw[0]<0 || jsynw[1]<0)  // this shouldn't happen
                            {
                                uneqbn &= ~(1<<cf), whybad|=8;
                            }
                            else if (wall[jsynw[1]].nextwall != jsynw[0])
                            {
//                                if (find_nextwall(sectorofwall(jsynw[1]), sectorofwall(jsynw[0])) < 0)
                                    uneqbn &= ~(1<<cf), whybad|=16;
                            }

                            if ((uneqbn&2)==0)
                            {
#if 0
                                if (whybad==1+8 && jbn[0][cf]>=0 && jbn[1][cf]<0)
                                {
                                    // 1st join sector extended, 2nd not... let's see
                                    // if the latter is inner to the former one

                                    int32_t lowerstartsec = yax_vnextsec(s0to1wall, cf);

                                    m = (lowerstartsec < 0)<<1;
                                    for (WALLS_OF_SECTOR(joinsector[1], k))
                                    {
                                        if (m) break;

                                        m |= (wall[k].nextsector>=0 && wall[k].nextsector != joinsector[0]);
                                        m |= (wall[k].nextwall>=0 && yax_vnextsec(wall[k].nextwall, cf)!=lowerstartsec)<<1;
                                    }

                                    if (m==0)
                                    {
                                        yax_setbunch(joinsector[1], YAX_FLOOR, jbn[0][cf]);
                                        yax_update(0);
                                        yax_updategrays(pos.z);
                                        asksave = 1;

                                        printmessage16("Added sector %d's floor to bunch %d",
                                                       joinsector[1], jbn[0][cf]);
                                    }
                                    else
                                    {
                                        if (m&1)
                                        {
                                            message("Can't add sector %d's floor to bunch %d: not inner to sector %d",
                                                    joinsector[1], jbn[0][cf], joinsector[0]);
                                        }
                                        else // if (m&2)
                                        {
                                            message("Can't add sector %d's floor to bunch %d: must have lower neighbor",
                                                    joinsector[1], jbn[0][cf]);
                                        }
                                    }
                                }
                                else
#endif
                                {
                                    if (whybad&1)
                                        message("Can't make floor bunchnums equal: both floors must be extended");
                                    else if (whybad&2)
                                        message("Can't make floor bunchnums equal: both floors must be non-sloped");
                                    else if (whybad&4)
                                        message("Can't make floor bunchnums equal: both floors must have equal height");
                                    else if (whybad&8)
                                        message("Can't make floor bunchnums equal: INTERNAL ERROR");
                                    else if (whybad&16)
                                        message("Can't make floor bunchnums equal: lower neighbors must be linked");
                                }
                            }
                            else
                            {
                                int32_t vcf, newbn, ynw;

                                // we're good to go for making floor bunchnums equal
                                for (SECTORS_OF_BUNCH(jbn[1][cf], YAX_FLOOR, k))
                                    yax_setbunch(k, YAX_FLOOR, jbn[0][cf]);
                                for (SECTORS_OF_BUNCH(jbn[1][cf], YAX_CEILING, k))
                                    yax_setbunch(k, YAX_CEILING, jbn[0][cf]);

                                yax_update(0);
                                // now we can iterate the sectors with the new bunchnums
                                newbn = yax_getbunch(joinsector[0], cf);

                                // clear all yax-nextwall links on walls that are inside the bunch
                                for (vcf=0; vcf<2; vcf++)
                                    for (SECTORS_OF_BUNCH(newbn, vcf, k))
                                        for (WALLS_OF_SECTOR(k, m))
                                        {
                                            ynw = yax_getnextwall(m, vcf);
                                            if (ynw < 0 || wall[m].nextsector < 0)
                                                continue;

                                            if (yax_getbunch(wall[m].nextsector, vcf) == newbn)
                                            {
                                                yax_setnextwall(ynw, !vcf, -1);
                                                yax_setnextwall(m, vcf, -1);
                                            }
                                        }

                                // shouldn't be needed again for the editor, but can't harm either:
                                yax_update(0);
                                yax_updategrays(pos.z);

                                printmessage16("Made sector %d and %d floor bunchnums equal",
                                               joinsector[0], joinsector[1]);
                                asksave = 1;
                            }

                            joinsector[0] = joinsector[1] = -1;
                            goto end_join_sectors;
                        }
#endif
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
                        int joink = k;
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
#ifdef YAX_ENABLE
                            yax_fixreverselinks(newnumwalls, newnumwalls);
#endif
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
                            message("internal error while joining sectors: infloop!");
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

                    // clean out nextwall links for deletesector
                    for (k=0; k<2; k++)
                        for (WALLS_OF_SECTOR(joinsector[k], j))
                        {
                            wall[j].nextwall = wall[j].nextsector = -1;
#ifdef YAX_ENABLE
                            yax_setnextwall(j, YAX_CEILING, -1);
                            yax_setnextwall(j, YAX_FLOOR, -1);
#endif
                        }

                    deletesector(joinsector[0]);
                    if (joinsector[0] < joinsector[1])
                        joinsector[1]--;
                    deletesector(joinsector[1]);

                    printmessage16("Sectors joined.");
                    mkonwinvalid();
                    asksave = 1;
#ifdef YAX_ENABLE
                    yax_update(0);
                    yax_updategrays(pos.z);
#endif
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
                newnumwalls = whitelinescan(sectorofwall(linehighlight), linehighlight);
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
#ifdef YAX_ENABLE
                        yax_setnextwall(i, YAX_CEILING, -1);
                        yax_setnextwall(i, YAX_FLOOR, -1);
#endif
                    }
#ifdef YAX_ENABLE
                    yax_setbunches(numsectors, -1, -1);
                    yax_update(0);
                    yax_updategrays(pos.z);
#endif
                    numwalls = newnumwalls;
                    newnumwalls = -1;
                    numsectors++;

                    asksave = 1;
                    printmessage16("Inner loop made into new sector.");
                }
            }
        }
        else if (keystatus[0x1f])  //S
        {
            int16_t sucksect = -1;

            keystatus[0x1f] = 0;

            for (i=0; i<numsectors; i++)
            {
                YAX_SKIPSECTOR(i);
                if (inside_editor_curpos(i) == 1)
                {
                    sucksect = i;
                    break;
                }
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
// PK
                    if (prefixarg)
                    {
                        sprite[i].picnum = prefixarg;
                        sprite[i].xrepeat = sprite[i].yrepeat = 48;
                        prefixarg = 0;
                    }
                    else handle_sprite_in_clipboard(i);

                    if (tilesiz[sprite[i].picnum].y >= 32)
                        sprite[i].cstat |= 1;

                    correct_sprite_yoffset(i);

                    printmessage16("Sprite inserted.");

                    asksave = 1;

                    VM_OnEvent(EVENT_INSERTSPRITE2D, i);
                }
            }
        }

        if (keystatus[0x2e])  // C (make circle of points)
        {
            if (highlightsectorcnt > 0)
                duplicate_selected_sectors();
            else if (highlightcnt > 0)
                duplicate_selected_sprites();
            else if (circlewall >= 0)
            {
                circlewall = -1;
            }
            else if (!m32_sideview)
            {
                if (linehighlight >= 0)
                {
#if 0 //def YAX_ENABLE
                    j = linehighlight;

                    if (yax_islockedwall(j) ||
                            (wall[j].nextwall >= 0 && yax_islockedwall(wall[j].nextwall)))
                        printmessage16("Can't make circle in wall constrained by sector extension.");
                    else
#endif
                    circlewall = linehighlight;
                }
            }

            keystatus[0x2e] = 0;
        }

        bad = keystatus[0x39] && (!m32_sideview || m32_sideelev == 512);  //Gotta do this to save lots of 3 spaces!

        if (keystatus[0x39] && !bad)
            message("Unable to create sectors in angled sideview mode.");

        if (circlewall >= 0)
        {
            int32_t tempint1, tempint2;

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
                int32_t goodtogo, err=0;

                const int32_t centerx = ((x1+x2) + scale(y1-y2,tempint1,tempint2))>>1;
                const int32_t centery = ((y1+y2) + scale(x2-x1,tempint1,tempint2))>>1;
                const int32_t circlerad = ksqrt(dmulscale4(centerx-x1,centerx-x1, centery-y1,centery-y1))<<2;

                const int32_t circleang1 = getangle(x1-centerx,y1-centery);
                const int32_t circleang2 = getangle(x2-centerx,y2-centery);

                const int32_t redw = (int32_t)(wall[circlewall].nextwall >= 0);
                int32_t insdpoints = 0;

                draw_cross(centerx, centery, 2, editorcolors[14]);

                k = ((circleang2-circleang1)&2047);
                if (mulscale4(x3-x1,y2-y1) < mulscale4(x2-x1,y3-y1))
                {
                    k = -((circleang1-circleang2)&2047);
                }

                // XXX: Still too permissive for TROR insertion
                goodtogo = (numwalls+(1+redw)*circlepoints <= MAXWALLS);

                if (bad > 0 && goodtogo)
                {
                    err = backup_drawn_walls(0);

                    if (err)
                    {
                        message("Error backing up drawn walls (code %d)!", err);
                        goodtogo = 0;
                    }
                }

                for (i=circlepoints; i>0; i--)
                {
                    const int32_t ps = 2;

                    j = (circleang1 + scale(i,k,circlepoints+1))&2047;
                    dax = centerx + mulscale14(sintable[(j+512)&2047],circlerad);
                    day = centery + mulscale14(sintable[j],circlerad);

                    inpclamp(&dax, -editorgridextent, editorgridextent);
                    inpclamp(&day, -editorgridextent, editorgridextent);

                    if (bad > 0 && goodtogo)
                    {
                        int32_t inspts = M32_InsertPoint(circlewall, dax,day, -1, &circlewall);

                        if (inspts==0)
                        {
                            message("Wall limit exceeded while inserting points.");
                            goto end_circle_insertion;
                        }
                        else if (inspts >= 65536)
                        {
                            message("ERR: Inserted %d points for constr. wall (exp. %d; %d already ins'd)",
                                    inspts&65535, inspts>>16, insdpoints);
                            goto end_circle_insertion;
                        }

                        insdpoints += inspts;
                    }

                    draw_square(dax, day, ps, editorcolors[14]);
                }

                if (bad > 0 && goodtogo)
                    backup_drawn_walls(1);

                if (bad > 0)
                {
                    if (goodtogo)
                    {
                        asksave = 1;
                        printmessage16("Circle points inserted.");
end_circle_insertion:
                        circlewall = -1;
                        mkonwinvalid();
                    }
                    else
                        printmessage16("Inserting circle points would exceed wall limit.");
                }
            }

            bad = 0;
            keystatus[0x39] = 0;
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
                ovh.suckwall = -1;
                ovh.split = 0;

                init_new_wall1(&ovh.suckwall, mousxplc, mousyplc);

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
                                YAX_SKIPSECTOR(i);
                                if (inside(dax,day,i) != 1)
                                    continue;

                                //check if first point at point of sector
                                m = -1;
                                for (WALLS_OF_SECTOR(i, k))
                                    if (wall[k].x==wall[numwalls].x && wall[k].y==wall[numwalls].y)
                                    {
                                        YAX_SKIPWALL(k);

                                        m = k;
                                        break;
                                    }

                                // if the second insertion is not on a neighboring point of the first one...
                                if (m>=0 && (POINT2(k).x != mousxplc || POINT2(k).y != mousyplc))
                                    if (wall[lastwall(k)].x != mousxplc || wall[lastwall(k)].y != mousyplc)
                                    {
                                        ovh.split = 1;
                                        ovh.splitsect = i;
                                        ovh.splitstartwall = m;
                                        break;
                                    }
                            }
                        }

                        //make new point

                        //make sure not drawing over old red line
                        bad = 0;
                        for (i=0; i<numwalls; i++)
                        {
                            YAX_SKIPWALL(i);

                            if (wall[i].nextwall >= 0)
                            {
                                int32_t lastwalx = wall[newnumwalls-1].x;
                                int32_t lastwaly = wall[newnumwalls-1].y;

                                YAX_SKIPWALL(wall[i].nextwall);

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
                            init_new_wall1(&ovh.suckwall, mousxplc, mousyplc);
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
                if (!ovh.split && newnumwalls >= numwalls+3
                        && firstx==mousxplc && firsty==mousyplc)
                {
                    wall[newnumwalls-1].point2 = numwalls;

                    if (ovh.suckwall == -1)  //if no connections to other sectors
                    {
                        k = -1;
                        for (i=0; i<numsectors; i++)
                        {
                            YAX_SKIPSECTOR(i);

                            if (inside(firstx,firsty,i) == 1)
                            {
                                // if all points inside that one sector i,
                                // will add an inner loop
                                for (j=numwalls+1; j<newnumwalls; j++)
                                {
                                    if (inside(wall[j].x, wall[j].y, i) != 1)
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
                            if (clockdir(numwalls) == CLOCKDIR_CCW)
                                flipwalls(numwalls,newnumwalls);

                            Bmemset(&sector[numsectors], 0, sizeof(sectortype));
                            sector[numsectors].extra = -1;

                            sector[numsectors].wallptr = numwalls;
                            sector[numsectors].wallnum = newnumwalls-numwalls;
                            sector[numsectors].ceilingz = -(32<<8);
                            sector[numsectors].floorz = (32<<8);

                            for (i=numwalls; i<newnumwalls; i++)
                                copy_some_wall_members(i, -1, 1);
#ifdef YAX_ENABLE
                            yax_setbunches(numsectors, -1, -1);
#endif
                            headspritesect[numsectors] = -1;
                            numsectors++;

                            numwalls = newnumwalls;
                            newnumwalls = -1;

                            printmessage16("Created new sector %d", numsectors-1);
                        }
                        else       //else add loop to sector
                        {
                            int32_t ret = AddLoopToSector(k, NULL);

                            if (ret < 0)
                                goto end_space_handling;
#ifdef YAX_ENABLE
                            else if (ret > 0)
                                printmessage16("Added inner loop to sector %d and made new inner sector", k);
                            else
#endif
                                printmessage16("Added inner loop to sector %d", k);
                            mkonwinvalid();
                            asksave = 1;
                        }
                    }
                    else  // if connected to at least one other sector
                    {
                        int16_t sucksect;

                        //add new sector with connections

                        if (clockdir(numwalls) == CLOCKDIR_CCW)
                            flipwalls(numwalls,newnumwalls);

                        for (i=numwalls; i<newnumwalls; i++)
                        {
                            copy_some_wall_members(i, ovh.suckwall, 1);
                            wall[i].cstat &= ~(1+16+32+64);

                            if (checksectorpointer(i, numsectors) > 0)
                            {
                                // if new red line, prefer the other-side wall as base
                                ovh.suckwall = wall[i].nextwall;
                            }
                        }
                        sucksect = sectorofwall(ovh.suckwall);

                        if (numsectors != sucksect)
                            Bmemcpy(&sector[numsectors], &sector[sucksect], sizeof(sectortype));

                        sector[numsectors].wallptr = numwalls;
                        sector[numsectors].wallnum = newnumwalls-numwalls;

                        sector[numsectors].extra = -1;
                        sector[numsectors].lotag = sector[numsectors].hitag = 0;

                        setslope(numsectors, YAX_CEILING, 0);
                        setslope(numsectors, YAX_FLOOR, 0);

                        sector[numsectors].ceilingpal = sector[numsectors].floorpal = 0;
#ifdef YAX_ENABLE
                        yax_setbunches(numsectors, -1, -1);
#endif
                        headspritesect[numsectors] = -1;
                        numsectors++;

                        numwalls = newnumwalls;
                        newnumwalls = -1;

                        message("Created new sector %d based on sector %d", numsectors-1, sucksect);
                    }

                    asksave = 1;
#ifdef YAX_ENABLE
                    yax_update(0);
                    yax_updategrays(pos.z);
#endif

                    goto end_space_handling;
                }
                ////////// split sector //////////
                else if (ovh.split == 1)
                {
                    int16_t danumwalls, splitendwall, doSectorSplit;
                    int16_t secondstartwall=-1;  // used only with splitting
                    int32_t expectedNumwalls = numwalls+2*(newnumwalls-numwalls-1), loopnum;
                    int32_t firstwallflag;
#ifdef YAX_ENABLE
                    int16_t cb, fb;
#endif
                    startwall = sector[ovh.splitsect].wallptr;
                    endwall = startwall + sector[ovh.splitsect].wallnum - 1;

                    firstwallflag = (startwall==ovh.splitstartwall || startwall==lastwall(ovh.splitstartwall));

//                    OSD_Printf("numwalls: %d, newnumwalls: %d\n", numwalls, newnumwalls);
                    i = -1;
                    for (k=startwall; k<=endwall; k++)
                        if (wall[k].x == wall[newnumwalls-1].x && wall[k].y == wall[newnumwalls-1].y)
                        {
                            i = k;
                            break;
                        }
                    //           vvvv shouldn't happen, but you never know...
                    if (i==-1 || k==ovh.splitstartwall)
                        goto end_space_handling;

                    splitendwall = k;
                    doSectorSplit = (loopnumofsector(ovh.splitsect,ovh.splitstartwall)
                                     == loopnumofsector(ovh.splitsect,splitendwall));

                    if (expectedNumwalls > MAXWALLS)
                    {
                        printmessage16("%s would exceed wall limit.", bad==0 ?
                                       "Splitting sector" : "Joining sector loops");
                        newnumwalls--;
                        goto end_space_handling;
                    }
#ifdef YAX_ENABLE
                    yax_getbunches(ovh.splitsect, &cb, &fb);

                    if ((cb>=0 && (sector[ovh.splitsect].ceilingstat&2))
                        || (fb>=0 && (sector[ovh.splitsect].floorstat&2)))
                    {
                        printmessage16("Sloped extended sectors cannot be split.");
                        newnumwalls--;
                        goto end_space_handling;
                    }
#endif
                    ////////// common code for splitting/loop joining //////////

                    newnumwalls--;  //first fix up the new walls
                    for (i=numwalls; i<newnumwalls; i++)
                    {
                        copy_some_wall_members(i, startwall, 1);
                        wall[i].point2 = i+1;
                    }

                    danumwalls = newnumwalls;  //where to add more walls

                    if (doSectorSplit)
                    {
                        // Copy outer loop of first sector
                        if (do_while_copyloop1(splitendwall, ovh.splitstartwall, &danumwalls, numwalls))
                            goto split_not_enough_walls;

                        //Add other loops for 1st sector
                        i = loopnum = loopnumofsector(ovh.splitsect,ovh.splitstartwall);

                        for (j=startwall; j<=endwall; j++)
                        {
                            k = loopnumofsector(ovh.splitsect,j);
                            if (k == i)
                                continue;

                            if (k == loopnum)
                                continue;

                            i = k;

                            if (loopinside(wall[j].x,wall[j].y, numwalls) != 1)
                                continue;

                            if (do_while_copyloop1(j, j, &danumwalls, danumwalls))
                                goto split_not_enough_walls;
                        }

                        secondstartwall = danumwalls;
                    }
                    else
                    {
                        if (do_while_copyloop1(splitendwall, splitendwall, &danumwalls, -1))
                            goto split_not_enough_walls;
                    }

                    //copy split points for other sector backwards
                    for (j=newnumwalls; j>numwalls; j--)
                    {
                        Bmemcpy(&wall[danumwalls], &wall[j], sizeof(walltype));
                        wall[danumwalls].nextwall = -1;
                        wall[danumwalls].nextsector = -1;
                        wall[danumwalls].point2 = danumwalls+1;
#ifdef YAX_ENABLE
                        yax_setnextwall(danumwalls,YAX_CEILING, -1);
                        yax_setnextwall(danumwalls,YAX_FLOOR, -1);
#endif
                        danumwalls++;
                    }

                    //copy rest of loop next
                    if (doSectorSplit)
                    {
                        if (do_while_copyloop1(ovh.splitstartwall, splitendwall, &danumwalls, secondstartwall))
                            goto split_not_enough_walls;
                    }
                    else
                    {
                        if (do_while_copyloop1(ovh.splitstartwall, ovh.splitstartwall, &danumwalls, numwalls))
                            goto split_not_enough_walls;
                    }

                    //Add other loops for 2nd sector
                    i = loopnum = loopnumofsector(ovh.splitsect,ovh.splitstartwall);

                    for (j=startwall; j<=endwall; j++)
                    {
                        k = loopnumofsector(ovh.splitsect, j);
                        if (k==i)
                            continue;

                        if (doSectorSplit && k==loopnum)
                            continue;
                        if (!doSectorSplit && (k == loopnumofsector(ovh.splitsect,ovh.splitstartwall) ||
                                               k == loopnumofsector(ovh.splitsect,splitendwall)))
                            continue;

                        i = k;

                        // was loopinside(... , secondstartwall) != 1, but this way there are
                        // no duplicate or left-out loops (can happen with convoluted geometry)
                        if (doSectorSplit && (loopinside(wall[j].x,wall[j].y, numwalls) != 0))
                            continue;

                        if (do_while_copyloop1(j, j, &danumwalls, danumwalls))
                            goto split_not_enough_walls;
                    }

                    //fix all next pointers on old sector line
                    for (j=numwalls; j<danumwalls; j++)
                    {
#ifdef YAX_ENABLE
//                        if (doSectorSplit || (j!=numwalls && j!=danumwalls-1))
                            yax_fixreverselinks(j, j);
#endif
                        if (wall[j].nextwall >= 0)
                        {
                            NEXTWALL(j).nextwall = j;

                            if (!doSectorSplit || j < secondstartwall)
                                NEXTWALL(j).nextsector = numsectors;
                            else
                                NEXTWALL(j).nextsector = numsectors+1;
                        }
                    }

                    //copy sector attributes & fix wall pointers
                    Bmemcpy(&sector[numsectors], &sector[ovh.splitsect], sizeof(sectortype));
                    sector[numsectors].wallptr = numwalls;
                    sector[numsectors].wallnum = (doSectorSplit?secondstartwall:danumwalls) - numwalls;

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

                        Bmemcpy(&sector[numsectors+1], &sector[ovh.splitsect], sizeof(sectortype));
                        sector[numsectors+1].wallptr = secondstartwall;
                        sector[numsectors+1].wallnum = danumwalls-secondstartwall;
                    }

                    //fix sprites
                    j = headspritesect[ovh.splitsect];
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

                    k = danumwalls-numwalls;  //Back up number of walls of new sector for later
                    numwalls = danumwalls;

                    //clear out old sector's next pointers for clean deletesector
                    for (j=startwall; j<=endwall; j++)
                    {
#ifdef YAX_ENABLE
                        // same thing for yax-nextwalls (only forward links!)
                        yax_setnextwall(j, YAX_CEILING, -1);
                        yax_setnextwall(j, YAX_FLOOR, -1);
#endif
                        wall[j].nextwall = wall[j].nextsector = -1;
                    }
                    deletesector(ovh.splitsect);

                    //Check pointers
                    for (j=numwalls-k; j<numwalls; j++)
                    {
                        if (wall[j].nextwall >= 0)
                            checksectorpointer(wall[j].nextwall, wall[j].nextsector);
                        checksectorpointer(j, sectorofwall(j));
                    }

                    //k now safe to use as temp

                    if (numwalls==expectedNumwalls)
                    {
                        if (doSectorSplit && cb<0 && fb<0)
                        {
                            if (firstwallflag)
                            {
                                int32_t rhsnew1stwall = sector[numsectors-2].wallptr;
                                int32_t lhsotherwall = wall[rhsnew1stwall].nextwall;

                                Bassert(lhsotherwall >= 0);

                                setfirstwall(numsectors-2, lastwall(rhsnew1stwall));
                                setfirstwall(numsectors-1, wall[lhsotherwall].point2);
                            }
                        }

                        message("%s", doSectorSplit ? "Sector split." : "Loops joined.");
                    }
                    else
                    {
                        message("%s WARNING: CREATED %d MORE WALLS THAN EXPECTED!",
                                doSectorSplit ? "Sector split." : "Loops joined.",
                                numwalls-expectedNumwalls);
                        // this would display 'num* out of bounds' but this corruption
                        // is almost as bad... (shouldn't happen anymore)
                        if (numcorruptthings < MAXCORRUPTTHINGS)
                            corruptthings[numcorruptthings++] = 0;
                        corruptlevel = 5;
                    }

                    if (0)
                    {
split_not_enough_walls:
                        message("%s failed: not enough space beyond wall[]",
                                doSectorSplit ? "Splitting sectors" : "Joining loops");
                    }

                    newnumwalls = -1;
                    asksave = 1;

                    mkonwinvalid();
#ifdef YAX_ENABLE
                    yax_update(0);
                    yax_updategrays(pos.z);
#endif
                }
            }
        }
end_space_handling:

        if (keystatus[0x1c]) //Left Enter
        {
            keystatus[0x1c] = 0;
            if (keystatus[0x2a] && keystatus[0x1d])  // LCtrl+LShift
            {
#ifdef YAX_ENABLE
                if (numyaxbunches == 0 ||
                    (fade_editor_screen(-1), ask_if_sure("Really check all wall pointers in TROR map?", 0)))
#endif
                {
                    printmessage16("CHECKING ALL POINTERS!");
                    for (i=0; i<numsectors; i++)
                    {
                        startwall = sector[i].wallptr;
                        for (j=startwall; j<numwalls; j++)
                            if (startwall > wall[j].point2)
                                startwall = wall[j].point2;
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
            }
            else  // NOT (LCtrl + LShift)
            {
                if (newnumwalls > numwalls)  // batch insert points
                {
                    const int32_t numdrawnwalls = newnumwalls-numwalls;
                    vec2_t *point = (vec2_t *)tempxyar;  // [MAXWALLS][2]
                    int32_t insdpoints = 0;

                    // back up the points of the line strip
                    for (i=0; i<numdrawnwalls+1; i++)
                    {
                        point[i].x = wall[numwalls+i].x;
                        point[i].y = wall[numwalls+i].y;
                    }

                    newnumwalls = -1;

                    for (i=0; i<numdrawnwalls; i++)
                    {
                        for (j=numwalls-1; j>=0; j--)  /* j may be modified in loop */
                        {
                            vec2_t pint;
                            int32_t inspts;

                            YAX_SKIPWALL(j);

                            if (!lineintersect2v((vec2_t *)&wall[j], (vec2_t *)&POINT2(j),
                                                 &point[i], &point[i+1], &pint))
                                continue;

                            if (vec2eq(&pint, (vec2_t *)&wall[j]) || vec2eq(&pint, (vec2_t *)&POINT2(j)))
                                continue;

                            inspts = M32_InsertPoint(j, pint.x, pint.y, -1, &j);  /* maybe modify j */

                            if (inspts==0)
                            {
                                printmessage16("Wall limit exceeded while inserting points.");
                                goto end_batch_insert_points;
                            }
                            else if (inspts >= 65536)
                            {
                                message("ERR: Inserted %d points for constr. wall (exp. %d; %d already ins'd)",
                                        inspts&65535, inspts>>16, insdpoints);
                                goto end_batch_insert_points;
                            }

                            insdpoints += inspts;
                        }
                    }

                    message("Batch-inserted %d points in total", insdpoints);
end_batch_insert_points:

                    if (insdpoints != 0)
                    {
#ifdef YAX_ENABLE
                        yax_updategrays(pos.z);
#endif
                        mkonwinvalid_keeptempsect();
                        asksave = 1;
                    }
                }
                else if (linehighlight >= 0)
                {
                    checksectorpointer(linehighlight,sectorofwall(linehighlight));
                    printmessage16("Checked pointers of highlighted line.");
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

        if (keystatus[0xd3] && eitherCTRL && numwalls > 0)  //sector delete
        {
            int32_t numdelsectors = 0;
            char *origframe=NULL;

#ifdef YAX_ENABLE
            int16_t cb, fb;
            uint8_t bunchbitmap[YAX_MAXBUNCHES>>3];
            Bmemset(bunchbitmap, 0, sizeof(bunchbitmap));
#endif
            keystatus[0xd3] = 0;

            for (i=0; i<numsectors; i++)
            {
                YAX_SKIPSECTOR(i);
                numdelsectors += (inside_editor_curpos(i) == 1);
            }

            for (i=0; i<numsectors; i++)
            {
                if (highlightsectorcnt <= 0 || !keystatus[0x2a])  // LShift
                {
                    YAX_SKIPSECTOR(i);

                    if (inside_editor_curpos(i) != 1)
                        continue;
                }

                k = 0;
                if (highlightsectorcnt > 0)
                {
                    // LShift: force highlighted sector deleting
                    if (keystatus[0x2a] || (hlsectorbitmap[i>>3]&(1<<(i&7))))
                    {
                        for (j=highlightsectorcnt-1; j>=0; j--)
                        {
#ifdef YAX_ENABLE
                            yax_getbunches(highlightsector[j], &cb, &fb);
                            if (cb>=0) bunchbitmap[cb>>3] |= (1<<(cb&7));
                            if (fb>=0) bunchbitmap[fb>>3] |= (1<<(fb&7));
#endif
                            deletesector(highlightsector[j]);
                            for (k=j-1; k>=0; k--)
                                if (highlightsector[k] >= highlightsector[j])
                                    highlightsector[k]--;
                        }

                        printmessage16("Highlighted sectors deleted.");
                        mkonwinvalid();
                        k = 1;
                    }
                }

                if (k == 0)
                {
                    if (numdelsectors > 1)
                    {
                        if (!bakframe_fillandfade(&origframe, i, "Delete this sector? (Y/N)"))
                            continue;
                    }

#ifdef YAX_ENABLE
                    yax_getbunches(i, &cb, &fb);
                    if (cb>=0) bunchbitmap[cb>>3] |= (1<<(cb&7));
                    if (fb>=0) bunchbitmap[fb>>3] |= (1<<(fb&7));
#endif
                    deletesector(i);
                    mkonwinvalid();
                    printmessage16("Sector deleted.");
                }

                Bfree(origframe);

#ifdef YAX_ENABLE
                for (j=0; j<numsectors; j++)
                {
                    yax_getbunches(j, &cb, &fb);
                    if (cb>=0 && (bunchbitmap[cb>>3] & (1<<(cb&7))))
                        yax_setbunch(j, YAX_CEILING, -1);
                    if (fb>=0 && (bunchbitmap[fb>>3] & (1<<(fb&7))))
                        yax_setbunch(j, YAX_FLOOR, -1);
                }
#endif
                reset_highlightsector();
                reset_highlight();

                newnumwalls = -1;
                asksave = 1;
#ifdef YAX_ENABLE
                yax_update(0);
                yax_updategrays(pos.z);
#endif
                break;
            }
        }

        if (keystatus[0xd3] && (pointhighlight >= 0))
        {
            if ((pointhighlight&0xc000) == 16384)   //Sprite Delete
            {
                deletesprite(pointhighlight&16383);
                printmessage16("Sprite deleted.");

                update_highlight();
                asksave = 1;
            }
            keystatus[0xd3] = 0;
        }

        if (keystatus[0xd2] || keystatus[0x17])  //InsertPoint
        {
            if (highlightsectorcnt > 0)
                duplicate_selected_sectors();
            else if (highlightcnt > 0)
                duplicate_selected_sprites();
            else if (linehighlight2 >= 0)
            {
                int32_t onewnumwalls = newnumwalls;
                int32_t wallis2sided = (wall[linehighlight2].nextwall>=0);

                int32_t err = backup_drawn_walls(0);

                if (err)
                {
                    message("Error backing up drawn walls (code %d)!", err);
                }
                else if (max(numwalls,onewnumwalls) >= MAXWALLS-wallis2sided)
                {
                    printmessage16("Inserting point would exceed wall limit.");
                }
                else
                {
                    getclosestpointonwall(m32_sideview?searchx:mousxplc, m32_sideview?searchy:mousyplc,
                                          linehighlight2, &dax,&day, 1);
                    i = linehighlight2;
                    if (m32_sideview)
                    {
                        int32_t y_p, d, dx, dy, frac;

                        dx = dax - m32_wallscreenxy[i][0];
                        dy = day - m32_wallscreenxy[i][1];
                        d = max(dx, dy);
                        y_p = (dy>dx);

                        if (d==0)
                            goto point_not_inserted;

                        frac = divscale24(d, m32_wallscreenxy[wall[i].point2][y_p]-m32_wallscreenxy[i][y_p]);
                        dax = POINT2(i).x - wall[i].x;
                        day = POINT2(i).y - wall[i].y;
                        dax = wall[i].x + mulscale24(dax,frac);
                        day = wall[i].y + mulscale24(day,frac);
                    }

                    adjustmark(&dax,&day, newnumwalls);
                    if ((wall[i].x == dax && wall[i].y == day) || (POINT2(i).x == dax && POINT2(i).y == day))
                    {
point_not_inserted:
                        printmessage16("Point not inserted.");
                    }
                    else
                    {
                        int32_t insdpoints = M32_InsertPoint(linehighlight2, dax, day, onewnumwalls, NULL);

                        if (insdpoints == 0)
                        {
                            printmessage16("Inserting points would exceed wall limit.");
                            goto end_insert_points;
                        }
                        else if (insdpoints == 1)
                        {
                            printmessage16("Point inserted.");
                        }
                        else if (insdpoints > 1 && insdpoints < 65536)
                        {
                            message("Inserted %d points for constrained wall.", insdpoints);
                        }
                        else  // insdpoints >= 65536
                        {
                            message("Inserted %d points for constrained wall (expected %d, WTF?).",
                                    insdpoints&65535, insdpoints>>16);
                        }

#ifdef YAX_ENABLE
                        yax_updategrays(pos.z);
#endif
                        mkonwinvalid_keeptempsect();
                    }
                }

end_insert_points:
                backup_drawn_walls(1);

                asksave = 1;
            }
            keystatus[0xd2] = keystatus[0x17] = 0;
        }

        /*j = 0;
        for(i=22-1;i>=0;i--) updatecrc16(j,kensig[i]);
        if ((j&0xffff) != 0xebf)
        {
        	printf("Don't screw with my name.\n");
        	Bexit(0);
        }*/
        //printext16(9L,336+9L,4,-1,kensig,0);
        //printext16(8L,336+8L,12,-1,kensig,0);

    nokeys:

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
                if (asksave)
                    message("You have unsaved changes.");
                else
                {
                    int skip = 0;
                nextmap:
                    //				bad = 0;
                    i = menuselect_auto(keystatus[0x2a] ? 0 : 1, skip); // LShift: prev map
                    if (i < 0)
                    {
                        if (i == -1)
                            message("No more map files.");
                        else if (i == -2)
                            message("No .MAP files found.");
                    }
                    else
                    {
                        if (LoadBoard(NULL, 4))
                        {
                            skip = 2;
                            goto nextmap;
                        }

                        RESET_EDITOR_VARS();
                        oposz = pos.z;
                    }
                    showframe(1);
                    keystatus[0x1c] = 0;

                    keystatus[0x2d]=keystatus[0x13]=0;

                }
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
            keystatus[1] = 0;
#if M32_UNDO
            _printmessage16("(N)ew, (L)oad, (S)ave, save (A)s, (T)est map, (U)ndo, (R)edo, (Q)uit");
#else
            _printmessage16("(N)ew, (L)oad, (S)ave, save (A)s, (T)est map, (Q)uit");
#endif
            printext16(16*8, ydim-STATUS2DSIZ2-12, editorcolors[15], -1, GetSaveBoardFilename(NULL), 0);

            showframe(1);
            bflushchars();
            bad = 1;
            while (bad == 1)
            {
                char ch;

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

                    if (ask_if_sure("Are you sure you want to start a new board? (Y/N)", 0))
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

                        RESET_EDITOR_VARS();
                        mkonwinvalid();

                        reset_default_mapstate();

                        Bstrcpy(boardfilename,"newboard.map");
                        CallExtLoadMap(boardfilename);
#if M32_UNDO
                        map_undoredo_free();
#endif
                        if (bakstat==0)
                        {
                            bakstat = restore_highlighted_map(&bakmap, 1);
                            if (bakstat == -1)
                            {
                                message("Can't copy highlighted portion of old map: limits exceeded.");
                                reset_highlightsector();
                            }
                        }

                        CheckMapCorruption(4, 0);

                    }

                    break;
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
                        int32_t bakstat=-1, ret;
                        mapinfofull_t bakmap;

                        if (highlightsectorcnt > 0)
                            bakstat = backup_highlighted_map(&bakmap);

                        ret = LoadBoard(NULL, 4);
                        if (ret)
                        {
                            message("^13Invalid map format, nothing loaded (code %d).", ret);
                            if (bakstat==0)
                                mapinfofull_free(&bakmap);
                        }
                        else
                        {
                            RESET_EDITOR_VARS();
                            oposz = pos.z;

                            if (bakstat==0)
                            {
                                bakstat = restore_highlighted_map(&bakmap, 1);
                                if (bakstat == -1)
                                {
                                    message("Can't copy highlighted portion of old map: limits exceeded.");
                                    reset_highlightsector();
                                }
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

                    // Back up full name.
                    Bstrcpy(selectedboardfilename, boardfilename);

                    // Get base name.
                    {
                        const char *basefn = getbasefn(boardfilename);

                        if (basefn != boardfilename)
                            Bmemmove(boardfilename, basefn, Bstrlen(basefn)+1);
                    }

                    i = 0;
                    while (boardfilename[i] != 0 && i < 64)
                        i++;
                    if (i >= 4 && boardfilename[i-4] == '.')
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
                        char *slash;

                        keystatus[0x1c] = 0;

                        Bstrcpy(&boardfilename[i], ".map");

                        // Update full name with new basename.
                        slash = Bstrrchr(selectedboardfilename,'/');
                        Bstrcpy(slash ? slash+1 : selectedboardfilename, boardfilename);

                        SaveBoardAndPrintMessage(selectedboardfilename);

                        Bstrcpy(boardfilename, selectedboardfilename);
                        CallExtSetupMapFilename(boardfilename);
                    }
                    bad = 0;
                }
                else if (ch == 's' || ch == 'S')  //S
                {
                    bad = 0;

                    if (CheckMapCorruption(4, 0)>=4)
                    {
                        fade_editor_screen(-1);
                        if (!ask_if_sure("Map is corrupt. Are you sure you want to save? (Y/N)", 0))
                            break;
                    }

                    SaveBoardAndPrintMessage(NULL);

                    showframe(1);
                }
                else if (ch == 't' || ch == 'T')
                {
                    bad = 0;
                    test_map(0);
                }
#if M32_UNDO
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
#endif
                else if (ch == 'q' || ch == 'Q')  //Q
                {
                    bad = 0;

                    if (ask_if_sure("Are you sure you want to quit?", 0))
                    {
                        //QUIT!

                        int32_t corrupt = CheckMapCorruption(4, 0);

                        if (ask_if_sure(corrupt<4?"Save changes?":"Map corrupt. Save changes?", 2+(corrupt>=4)))
                            SaveBoard(NULL, M32_SB_ASKOV);

                        while (keystatus[1] || keystatus[0x2e])
                        {
                            keystatus[1] = 0;
                            keystatus[0x2e] = 0;
                            quitevent = 0;
                            printmessage16("Operation cancelled");
                            showframe(1);
                            goto CANCEL;
                        }

                        CallExtUnInit();
//                        clearfilenames();
                        uninitengine();

                        Bexit(0);
                    }

                    // printmessage16("");
                    showframe(1);
                }
            }

            clearkeys();
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
    mkonwinvalid_keeptempsect();

    fixspritesectors();

    if (setgamemode(fullscreen,xdimgame,ydimgame,bppgame) < 0)
    {
        initprintf("%d * %d not supported in this graphics mode\n",xdim,ydim);
        CallExtUnInit();
//        clearfilenames();
        uninitengine();
        Bexit(1);
    }

    setbrightness(GAMMA_CALC,0,0);

    pos.z = oposz;

    searchx = clamp(scale(searchx,xdimgame,xdim2d), 8, xdimgame-8-1);
    searchy = clamp(scale(searchy,ydimgame,ydim2d-STATUS2DSIZ), 8, ydimgame-8-1);

    VM_OnEvent(EVENT_ENTER3DMODE, -1);
}

// flags:
// 1: quit_is_yes
// 2: don't clear keys on return
int32_t ask_if_sure(const char *query, uint32_t flags)
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

int32_t editor_ask_function(const char *question, const char *dachars, int32_t numchars)
{
    char ch;
    int32_t i, ret=-1;

    _printmessage16("%s", question);

    showframe(1);
    bflushchars();

    // 'c' is cancel too, but can be overridden
    while ((keystatus[1]|keystatus[0x2e]) == 0 && ret==-1)
    {
        if (handleevents())
            quitevent = 0;

        idle();
        ch = bgetchar();

        for (i=0; i<numchars; i++)
            if (ch==Btolower(dachars[i]) || ch==Btoupper(dachars[i]))
                ret = i;
    }

    clearkeys();

    return ret;
}

#ifdef YAX_ENABLE
static int32_t ask_above_or_below(void)
{
    char dachars[2] = {'a', 'z'};
    return editor_ask_function("Extend above (a) or below (z)?", dachars, 2);
}
#endif

static void SaveBoardAndPrintMessage(const char *fn)
{
    const char *f;

    _printmessage16("Saving board...");
    showframe(1);

    f = SaveBoard(fn, M32_SB_ASKOV);

    if (f)
    {
        if (saveboard_fixedsprites)
            message("Saved board %sto %s (changed sectnums of %d sprites).",
                    saveboard_savedtags?"and tags ":"", f, saveboard_fixedsprites);
        else
            message("Saved board %sto %s.", saveboard_savedtags?"and tags ":"", f);
    }
    else
    {
        if (!saveboard_canceled)
        {
            if (saveboard_fixedsprites)
                message("^13SAVING BOARD FAILED (changed sectnums of %d sprites).",
                        saveboard_fixedsprites);
            else
                message("^13SAVING BOARD FAILED.");
        }
    }
}

// get the file name of the file that would be written if SaveBoard(fn, 0) was called
const char *GetSaveBoardFilename(const char *fn)
{
    if (!fn)
        fn = boardfilename;

    if (pathsearchmode)
        return fn;

    // virtual filesystem mode can't save to directories so drop the file into
    // the current directory
    return getbasefn(fn);
}

// flags: see enum SaveBoardFlags.
// returns: NULL on failure, file name on success.
const char *SaveBoard(const char *fn, uint32_t flags)
{
    int32_t ret;
    const char *f = GetSaveBoardFilename(fn);

    saveboard_canceled = 0;
#ifdef NEW_MAP_FORMAT
    if ((flags&M32_SB_ASKOV) && mapversion>=10 &&
            g_loadedMapVersion != -1 && g_loadedMapVersion < mapversion)
    {
        char question[128];
        // XXX: This message is potentially confusing if the user is "Saving
        // As" to a new file name.
        Bsnprintf(question, sizeof(question), "Are you sure to overwrite a version "
                  "V%d map with a V%d map-text one?", g_loadedMapVersion, mapversion);

        if (AskIfSure(question))
        {
            message("Cancelled saving board");
            saveboard_canceled = 1;
            return NULL;
        }
    }
#endif

    saveboard_savedtags = 0;
    saveboard_fixedsprites = CallExtPreSaveMap();

    ret = saveboard(f, &startpos, startang, startsectnum);
    if ((flags&M32_SB_NOEXT)==0)
    {
        CallExtSaveMap(f);
        saveboard_savedtags = !taglab_save(f);
    }

    return (ret==0) ? f : NULL;
}

// flags:  1: for running on Mapster32 init
//         4: passed to loadboard flags (no polymer_loadboard); implies no maphack loading
// returns:
//     0 on success,
//    <0 on failure.
int32_t LoadBoard(const char *filename, uint32_t flags)
{
    int32_t i, tagstat;
    const int32_t loadingflags = (!pathsearchmode && grponlymode) ? 2 : 0;

    if (!filename)
        filename = selectedboardfilename;

    editorzrange[0] = INT32_MIN;
    editorzrange[1] = INT32_MAX;

    CallExtPreLoadMap();
    i = loadboard(filename, (flags&4)|loadingflags, &pos, &ang, &cursectnum);
    if (i == -2)
        i = loadoldboard(filename,loadingflags, &pos, &ang, &cursectnum);

    if (i < 0)
    {
//        printmessage16("Invalid map format.");
        return i;
    }

    // Success, so copy the file name.
    if (filename != boardfilename)
        Bstrcpy(boardfilename, filename);

    mkonwinvalid();

    highlightcnt = -1;
    Bmemset(show2dwall, 0, sizeof(show2dwall));  //Clear all highlights
    Bmemset(show2dsprite, 0, sizeof(show2dsprite));

    if ((flags&4)==0)
        loadmhk(0);

    tagstat = taglab_load(boardfilename, loadingflags);
    CallExtLoadMap(boardfilename);

    {
        char msgtail[64];
        const int32_t ci = CheckMapCorruption(4, 0);

        if (ci == 5)
            Bstrcpy(msgtail, "^12(EXTREME corruption)");
        else if (ci == 4)
            Bstrcpy(msgtail, "^12(HEAVY corruption)");
        else if (i > 0)
            Bsprintf(msgtail, "^14(removed %d sprites)", i);
        else if (ci >= 1 && ci < 4)
            Bstrcpy(msgtail, "^14(moderate corruption)");
        else
            Bstrcpy(msgtail, "successfully");

        message("Loaded V%d map %s%s %s", g_loadedMapVersion,
                boardfilename, tagstat==0?" w/tags":"", msgtail);
    }

    startpos = pos;      //this is same
    startang = ang;
    startsectnum = cursectnum;
    asksave = 0;

    return 0;
}

void getpoint(int32_t searchxe, int32_t searchye, int32_t *x, int32_t *y)
{
    inpclamp(&pos.x, -editorgridextent, editorgridextent);
    inpclamp(&pos.y, -editorgridextent, editorgridextent);

    searchxe -= halfxdim16;
    searchye -= midydim16;

    vec2_t svec = { searchxe ,searchye };

    if (m32_sideview)
    {
        if (m32_sidesin!=0)
            svec.y = divscale14(svec.y, m32_sidesin);
        rotatepoint(zerovec, svec, -m32_sideang, &svec);
    }

    *x = pos.x + divscale14(svec.x,zoom);
    *y = pos.y + divscale14(svec.y,zoom);

    inpclamp(x, -editorgridextent, editorgridextent);
    inpclamp(y, -editorgridextent, editorgridextent);
}

static int32_t getlinehighlight(int32_t xplc, int32_t yplc, int32_t line, int8_t ignore_pointhighlight)
{
    int32_t i, j, dst, dist, closest, x1, y1, x2, y2, nx, ny;
    int32_t daxplc, dayplc;

    if (numwalls == 0)
        return -1;

    if (mouseb & 1 || searchlock)
        return line;

    if (!ignore_pointhighlight && (pointhighlight&0xc000) == 16384)
        return -1;

    dist = linehighlightdist;
    if (m32_sideview)
    {
        daxplc = searchx;
        dayplc = searchy;
        dist = mulscale14(dist, zoom);
    }
    else
    {
        daxplc = xplc;
        dayplc = yplc;
    }

    closest = -1;
    for (i=0; i<numwalls; i++)
    {
        if (!m32_sideview)
            YAX_SKIPWALL(i);

        getclosestpointonwall(daxplc,dayplc, i, &nx,&ny, 1);
        dst = klabs(daxplc-nx) + klabs(dayplc-ny);
        if (dst <= dist)
        {
            dist = dst;
            closest = i;
        }
    }

    if (closest>=0 && (j = wall[closest].nextwall) >= 0)
#ifdef YAX_ENABLE
    if (m32_sideview || ((graywallbitmap[j>>3]&(1<<(j&7)))==0))
#endif
    {
        //if red line, allow highlighting of both sides
        if (m32_sideview)
        {
            x1 = m32_wallscreenxy[closest][0];
            y1 = m32_wallscreenxy[closest][1];
            x2 = m32_wallscreenxy[wall[closest].point2][0];
            y2 = m32_wallscreenxy[wall[closest].point2][1];
        }
        else
        {
            x1 = wall[closest].x;
            y1 = wall[closest].y;
            x2 = POINT2(closest).x;
            y2 = POINT2(closest).y;
        }

        i = wall[closest].nextwall;
        if (!m32_sideview ||
            ((B_UNBUF64(m32_wallscreenxy[closest]) == B_UNBUF64(m32_wallscreenxy[wall[j].point2])) &&
             (B_UNBUF64(m32_wallscreenxy[wall[closest].point2]) == B_UNBUF64(m32_wallscreenxy[j]))))
            if (dmulscale32(daxplc-x1,y2-y1,-(x2-x1),dayplc-y1) >= 0)
                closest = j;
    }

    return closest;
}

int32_t getpointhighlight(int32_t xplc, int32_t yplc, int32_t point)
{
    int32_t i, j, dst, dist = pointhighlightdist, closest = -1;
    int32_t dax,day;
    int32_t alwaysshowgray = get_alwaysshowgray();

    if (numwalls == 0)
        return -1;

    if (mouseb & 1 || searchlock)
        return point;

    if (grid < 1)
        dist = 0;

    for (i=0; i<numsectors; i++)
    {
        if (!m32_sideview || !alwaysshowgray)
            YAX_SKIPSECTOR(i);

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
    }

    if (zoom >= 256)
        for (i=0; i<MAXSPRITES; i++)
            if (sprite[i].statnum < MAXSTATUS)
            {
                if ((!m32_sideview || !alwaysshowgray) && sprite[i].sectnum >= 0)
                    YAX_SKIPSECTOR(sprite[i].sectnum);

                if (!m32_sideview)
                {
                    dst = klabs(xplc-sprite[i].x) + klabs(yplc-sprite[i].y);
                }
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

    return closest;
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
        pointlockdist = (256>>grid);

    dist = pointlockdist;
    dax = *xplc;
    day = *yplc;

    for (i=0; i<danumwalls; i++)
    {
        YAX_SKIPWALL(i);

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

    return 0;
}

static int32_t checkautoinsert(int32_t dax, int32_t day, int16_t danumwalls)
{
    int32_t i, x1, y1, x2, y2;

    if (danumwalls < 0)
        danumwalls = numwalls;

    for (i=0; i<danumwalls; i++)    // Check if a point should be inserted
    {
        YAX_SKIPWALL(i);

        x1 = wall[i].x;
        y1 = wall[i].y;
        x2 = POINT2(i).x;
        y2 = POINT2(i).y;

        if ((x1 != dax || y1 != day) && (x2 != dax || y2 != day))
            if ((x1 <= dax && dax <= x2) || (x2 <= dax && dax <= x1))
                if ((y1 <= day && day <= y2) || (y2 <= day && day <= y1))
                    if ((dax-x1)*(y2-y1) == (day-y1)*(x2-x1))
                        return 1;          //insertpoint((short)i,dax,day,NULL);
    }

    return 0;
}

// <wallstart> has to be the starting (i.e. least index) wall of a loop!
// Returns: CLOCKDIR_CW or CLOCKDIR_CCW.
int32_t clockdir(int32_t wallstart)
{
    int32_t tempint, x0, x1, x2, y0, y1, y2;

    int32_t minx = 0x7fffffff;
    int32_t themin = -1;
    int32_t i = wallstart-1;

    do
    {
        i++;
        if (POINT2(i).x < minx)
        {
            minx = POINT2(i).x;
            themin = i;
        }
    }
    while (wall[i].point2 != wallstart && i < MAXWALLS-1);
    // NOTE: the i < MAXWALLS-1 check is really only safety against either
    //  - a really corrupt map, or
    //  - misuse of clockdir() where <wallstart> is not the starting wall of
    //    the very last loop

    x0 = wall[themin].x;
    y0 = wall[themin].y;
    x1 = POINT2(themin).x;
    y1 = POINT2(themin).y;
    x2 = POINT2(wall[themin].point2).x;
    y2 = POINT2(wall[themin].point2).y;

    if (y2 <= y1 && y1 <= y0)
        return CLOCKDIR_CW;
    if (y0 <= y1 && y1 <= y2)
        return CLOCKDIR_CCW;

    tempint = (x0-x1)*(y2-y1) - (x2-x1)*(y0-y1);
    if (tempint < 0)
        return CLOCKDIR_CW;
    else
        return CLOCKDIR_CCW;
}

static void flipwalls(int16_t numwalls, int16_t newnumwalls)
{
    int32_t i, j, nume;

    nume = newnumwalls-numwalls;

    for (i=numwalls; i<numwalls+(nume>>1); i++)
    {
        j = numwalls+newnumwalls-i-1;

        swapshort(&onextwall[i], &onextwall[j]);

        swaplong(&wall[i].x, &wall[j].x);
        swaplong(&wall[i].y, &wall[j].y);
    }
}

static void do_insertpoint(int32_t w, int32_t dax, int32_t day, int32_t *mapwallnum)
{
    int32_t i;
    const int32_t sucksect = sectorofwall(w);
    const uint32_t lenbyrep = getlenbyrep(wallength(w), wall[w].xrepeat);

    sector[sucksect].wallnum++;
    for (i=sucksect+1; i<numsectors; i++)
        sector[i].wallptr++;

    if (mapwallnum && *mapwallnum >= w+1)
        (*mapwallnum)++;

    movewalls(w+1, +1);
    Bmemcpy(&wall[w+1], &wall[w], sizeof(walltype));
#ifdef YAX_ENABLE
    wall[w+1].cstat &= ~(1<<14);
#endif
    wall[w].point2 = w+1;
    wall[w+1].x = dax;
    wall[w+1].y = day;

    fixxrepeat(w, lenbyrep);
    AlignWallPoint2(w);
    fixxrepeat(w+1, lenbyrep);
}

// Returns number of points inserted (1; or 2 if wall had a nextwall).
//  *mapwallnum is set to the new wallnum of the former (pre-insertpoint) *mapwallnum
//  (the new one can only be >= than the old one; ptr may be NULL if we don't care)
static int32_t insertpoint(int16_t linehighlight, int32_t dax, int32_t day, int32_t *mapwallnum)
{
    int32_t j = linehighlight;

    do_insertpoint(j, dax, day, mapwallnum);

    if (wall[j].nextwall >= 0)
    {
        int32_t k = wall[j].nextwall;

        do_insertpoint(k, dax, day, mapwallnum);

        j = wall[k].nextwall;
        wall[j].nextwall = k+1;
        wall[j+1].nextwall = k;
        wall[k].nextwall = j+1;
        wall[k+1].nextwall = j;

        return 2;
    }

    return 1;
}

// runi: 0=check (forbidden), 1=prepare, 2=do!
static void deletepoint(int16_t point, int32_t runi)
{
    int32_t i, j, sucksect;

    Bassert(runi != 0);

    if (runi==1)
    {
        i = wall[point].nextwall;
        if (i >= 0)
        {
            NEXTWALL(i).nextwall = NEXTWALL(i).nextsector = -1;
            wall[i].nextwall = wall[i].nextsector = -1;
        }

        return;
    }

    sucksect = sectorofwall(point);

    sector[sucksect].wallnum--;
    for (i=sucksect+1; i<numsectors; i++)
        sector[i].wallptr--;

    j = lastwall(point);
    wall[j].point2 = wall[point].point2;

#if 0
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
#endif
    movewalls(point, -1);

//    checksectorpointer(j, sucksect);

    return;
}

static int32_t deletesector(int16_t sucksect)
{
    int32_t i, j, k, nextk, startwall, endwall;

    while (headspritesect[sucksect] >= 0)
        deletesprite(headspritesect[sucksect]);

    startwall = sector[sucksect].wallptr;
    endwall = startwall + sector[sucksect].wallnum - 1;
    j = sector[sucksect].wallnum;

#ifdef YAX_ENABLE
    yax_setbunches(sucksect, -1, -1);
#endif

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
    {
        if (wall[i].nextwall >= 0)
        {
            NEXTWALL(i).nextwall = -1;
            NEXTWALL(i).nextsector = -1;
        }
    }

    movewalls(startwall, -j);
    for (i=0; i<numwalls; i++)
        if (wall[i].nextwall >= startwall)
            wall[i].nextsector--;

    return 0;
}

int32_t fixspritesectors(void)
{
    int32_t i;
    int32_t numfixedsprites = 0, printfirsttime = 0;

    for (i=numsectors-1; i>=0; i--)
        if (sector[i].wallnum <= 0 || sector[i].wallptr >= numwalls)
        {
            // XXX: This is not the best course of action for
            //  such great corruption.
            deletesector(i);
            mkonwinvalid();
            initprintf("NOTE: Deleted sector %d which had corrupt .wallnum or .wallptr\n", i);
        }

    if (m32_script_expertmode)
        return 0;

    for (i=0; i<MAXSPRITES; i++)
        if (sprite[i].statnum < MAXSTATUS)
        {
            const int32_t dax=sprite[i].x, day=sprite[i].y;

            if (inside(dax,day,sprite[i].sectnum) != 1)
            {
                int32_t j, cz, fz;

                spriteoncfz(i, &cz, &fz);

                for (j=0; j<numsectors; j++)
                {
                    if (cz <= sprite[i].z && sprite[i].z <= fz && inside(dax,day, j) == 1)
                    {
                        if (fixmaponsave_sprites || (unsigned)sprite[i].sectnum >= numsectors+0u)
                        {
                            if (printfirsttime == 0)
                            {
                                initprintf("--------------------\n");
                                printfirsttime = 1;
                            }
                            initprintf("Changed sectnum of sprite #%d from %d to %d\n",
                                              i, TrackerCast(sprite[i].sectnum), j);

                            changespritesect(i, j);
                            numfixedsprites++;
                        }
                        break;
                    }
                }
            }
        }

    return numfixedsprites;
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

        if (ovh.bak_wallsdrawn > 0)
        {
            if (ovh.suckwall >= start)
                ovh.suckwall += offs;
            if (ovh.splitstartwall >= start)
                ovh.splitstartwall += offs;
        }
    }

    numwalls += offs;
    for (i=0; i<numwalls; i++)
    {
        if (wall[i].nextwall >= start) wall[i].nextwall += offs;
        if (wall[i].point2 >= start) wall[i].point2 += offs;
    }
#ifdef YAX_ENABLE
    yax_tweakwalls(start, offs);
#endif

    return 0;
}

int32_t wallength(int16_t i)
{
    int64_t dax = POINT2(i).x - wall[i].x;
    int64_t day = POINT2(i).y - wall[i].y;
#if 1 //def POLYMOST
    int64_t hypsq = dax*dax + day*day;
    if (hypsq > (int64_t)INT32_MAX)
        return (int32_t)sqrt((double)hypsq);
    else
        return ksqrt((uint32_t)hypsq);
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

uint32_t getlenbyrep(int32_t len, int32_t repeat)
{
    if (repeat <= 0)
        return ((uint32_t)len)<<12;

    return divscale12(len, repeat);
}

void fixxrepeat(int16_t wallnum, uint32_t lenrepquot)  // lenrepquot: divscale12(wallength,xrepeat)
{
    if (lenrepquot != 0)
    {
        uint32_t res = (((wallength(wallnum)<<12)+(1<<11))/lenrepquot);
        wall[wallnum].xrepeat = clamp(res, 1, 255);
    }
}


int32_t overridepm16y = -1;

void clearmidstatbar16(void)
{
    int32_t y = overridepm16y<0 ? STATUS2DSIZ : overridepm16y;

    begindrawing();
    CLEARLINES2D(ydim-y+25, STATUS2DSIZ+2-(25<<1), 0);
    enddrawing();
}

static void clearministatbar16(void)
{
    int32_t i, col = editorcolors[25];

    begindrawing();

    for (i=ydim-STATUS2DSIZ2; i<ydim; i+=2)
    {
//        drawline256(0, i<<12, xdim<<12, i<<12, col);
        CLEARLINES2D(i, 1, (col<<24)|(col<<16)|(col<<8)|col);
        CLEARLINES2D(i+1, 1, (col<<24)|(col<<16)|(col<<8)|col);
        col--;
        if (col <= 0) break;
    }

    CLEARLINES2D(i, ydim-i, 0);

    if (xdim >= 800)
    {
        Bsnprintf(tempbuf, sizeof(tempbuf), "%s %s", AppProperName, CallExtGetVer());
        printext16(xdim2d-(Bstrlen(tempbuf)<<3)-3, ydim2d-STATUS2DSIZ2+10, editorcolors[4],-1, tempbuf, 0);
        printext16(xdim2d-(Bstrlen(tempbuf)<<3)-2, ydim2d-STATUS2DSIZ2+9, editorcolors[12],-1, tempbuf, 0);
    }

    enddrawing();
}

// <startwall> has to be the starting wall of a loop!
//
// Assuming that <startwall> indicates a CW (outer) loop, the return value can
// be seen as a boolean of whether (x,y) is inside it.
//
// XXX: this function suffers from asymmetry issues in degenerate cases,
// similar to how inside() did before r3898.
int32_t loopinside(int32_t x, int32_t y, int16_t startwall)
{
    bssize_t cnt = clockdir(startwall);
    int32_t i = startwall;

    do
    {
        int32_t x1 = wall[i].x;
        int32_t x2 = POINT2(i).x;

        if (x <= x1 || x <= x2)
        {
            int32_t y1 = wall[i].y;
            int32_t y2 = POINT2(i).y;

            if (y1 > y2)
            {
                swaplong(&x1, &x2);
                swaplong(&y1, &y2);
            }

            if (y1 <= y && y < y2)
                if ((uint64_t)x1*(y-y2) + (uint64_t)x2*(y1-y) <= (uint64_t)x*(y1-y2))
                    cnt ^= 1;
        }

        i = wall[i].point2;
    }
    while (i != startwall);

    return cnt;
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
    return numloops;
}
#endif

int32_t getnumber_internal1(char ch, int32_t *danumptr, int32_t maxnumber, char sign)
{
    int32_t danum = *danumptr;

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
            if (nbig >= (int64_t)(-maxnumber)) danum = nbig;
        }
    }
    else if (ch == 8 || ch == 127)  	// backspace
    {
        danum /= 10;
    }
    else if (ch == 13)
    {
        return 1;
    }
    else if (ch == '-' && sign)  	// negate
    {
        danum = -danum;
    }

    *danumptr = danum;
    return 0;
}

int32_t getnumber_autocomplete(const char *namestart, char ch, int32_t *danum, int32_t flags)
{
    if (flags!=1 && flags!=2)
        return 0;

    if (flags==2 && *danum<0)
        return 0;

    if (isalpha(ch))
    {
        char b[2];
        const char *gotstr;
        int32_t i, diddel;

        b[0] = ch;
        b[1] = 0;
        gotstr = getstring_simple(namestart, b, (flags==1)?sizeof(names[0])-1:TAGLAB_MAX-1, flags);
        if (!gotstr || !gotstr[0])
            return 0;

        if (flags==1)
        {
            for (i=0; i<MAXTILES; i++)
                if (!Bstrcasecmp(names[i], gotstr))
                {
                    *danum = i;
                    return 1;
                }
        }
        else
        {
            i = taglab_gettag(gotstr);
//initprintf("taglab: s=%s, i=%d\n",gotstr,i);
            if (i > 0)
            {
                *danum = i;
                return 1;
            }
            else
            {
                // insert new tag
                if (*danum > 0 && *danum<32768)
                {
                    diddel = taglab_add(gotstr, *danum);
                    message("Added label \"%s\" for tag %d%s%s", gotstr, *danum,
                            diddel?", deleting old ":"",
                            (!diddel)?"":(diddel==1?"label":"tag"));
                    return 1;
                }
                else if (*danum==0)
                {
                    i = taglab_getnextfreetag(NULL);
                    if (i >= 1)
                    {
                        *danum = i;
                        diddel = taglab_add(gotstr, *danum);
                        message("%sadded label \"%s\" for tag %d%s%s",
                                diddel?"Auto-":"Automatically ", gotstr, *danum,
                                diddel?", deleting old ":"",
                                (!diddel)?"":(diddel==1?"label":"tag"));
                        return 1;
                    }
                }
            }
        }
    }

    return 0;
}

// sign is now used for more than one flag (also _getnumber256):
//  1: sign
//  2: autocomplete names
//  4: autocomplete taglabels
//  8: return -1 if cancelled
int32_t _getnumber16(const char *namestart, int32_t num, int32_t maxnumber, char sign, const char *(func)(int32_t))
{
    char buffer[80], ournamestart[80-17], ch;
    int32_t n, danum, oldnum;
    uint8_t flags = (sign&(2|4|8))>>1;
    sign &= 1;

    danum = num;
    oldnum = danum;

    // need to have 4+11+2==17 chars room at the end
    // ("^011", max. string length of an int32, "_ ")
    Bstrncpyz(ournamestart, namestart, sizeof(ournamestart));

    bflushchars();
    while (keystatus[0x1] == 0)
    {
        if (handleevents())
            quitevent = 0;

        idle();
        ch = bgetchar();

        Bsprintf(buffer, "%s^011%d", ournamestart, danum);
        n = Bstrlen(buffer);  // maximum is 62+4+11 == 77
        if (totalclock & 32)
            Bstrcat(buffer,"_ ");
        // max strlen now 79
        _printmessage16("%s", buffer);

        if (func != NULL)
        {
            Bsnprintf(buffer, sizeof(buffer), "%s", func(danum));
            // printext16(200L-24, ydim-STATUS2DSIZ+20L, editorcolors[9], editorcolors[0], buffer, 0);
            printext16(n<<3, ydim-STATUS2DSIZ+128, editorcolors[11], -1, buffer,0);
        }

        showframe(1);

        n = 0;
        if (getnumber_internal1(ch, &danum, maxnumber, sign) ||
            (n=getnumber_autocomplete(ournamestart, ch, &danum, flags&(1+2))))
        {
            if (flags==1 || n==0)
                printmessage16("%s", buffer);
            if (danum != oldnum)
                asksave = 1;
            oldnum = danum;
            break;
        }
    }

    if (keystatus[0x1] && (flags&4))
        oldnum = -1;

    clearkeys();

    return oldnum;
}

static void getnumber_clearline(void)
{
    char cbuf[128];
    int32_t i;
    for (i=0; i<min(xdim>>3, (signed)sizeof(cbuf)-1); i++)
        cbuf[i] = ' ';
    cbuf[i] = 0;
    printext256(0, 0, whitecol, 0, cbuf, 0);
}

// sign: |16: don't draw scene
int32_t _getnumber256(const char *namestart, int32_t num, int32_t maxnumber, char sign, const char *(func)(int32_t))
{
    char buffer[80], ournamestart[80-13], ch;
    int32_t danum, oldnum;
    uint8_t flags = (sign&(2|4|8|16))>>1;
    sign &= 1;

    danum = num;
    oldnum = danum;

    // need to have 11+2==13 chars room at the end
    // (max. string length of an int32, "_ ")
    Bstrncpyz(ournamestart, namestart, sizeof(ournamestart));

    bflushchars();
    while (keystatus[0x1] == 0)
    {
        if (handleevents())
            quitevent = 0;

        if ((flags&8)==0)
            M32_DrawRoomsAndMasks();

        ch = bgetchar();
        if (keystatus[0x1])
            break;
        clearkeys();

        mouseb = 0;
        searchx = osearchx;
        searchy = osearchy;

        inputchecked = 1;

        if ((flags&8)==0)
            CallExtCheckKeys();

        getnumber_clearline();

        Bsprintf(buffer,"%s%d",ournamestart,danum);
        // max strlen now 66+11==77
        if (totalclock & 32)
            Bstrcat(buffer,"_ ");
        // max strlen now 79
        printmessage256(0, 0, buffer);
        if (func != NULL)
        {
            Bsnprintf(buffer, sizeof(buffer), "%s", func(danum));
            printmessage256(0, 9, buffer);
        }

        showframe(1);

        if (getnumber_internal1(ch, &danum, maxnumber, sign) ||
            getnumber_autocomplete(ournamestart, ch, &danum, flags&(1+2)))
        {
            if (danum != oldnum)
                asksave = 1;
            oldnum = danum;
            break;
        }
    }

    if (keystatus[0x1] && (flags&4))
        oldnum = -1;

    clearkeys();

    lockclock = totalclock;  //Reset timing

    return oldnum;
}

// querystr: e.g. "Name: ", must be !=NULL
// defaultstr: can be NULL
//  NO overflow checks are done when copying them!
// maxlen: maximum length of entry string, if ==1, enter single char
// completion: 0=none, 1=names[][], 2=taglabels
const char *getstring_simple(const char *querystr, const char *defaultstr, int32_t maxlen, int32_t completion)
{
    static char buf[128];
    int32_t ei=0, qrylen=0, maxidx, havecompl=0;
    char ch;

    bflushchars();
    clearkeys();

    Bmemset(buf, 0, sizeof(buf));

    qrylen = Bstrlen(querystr);
    Bmemcpy(buf, querystr, qrylen);

    if (maxlen==0)
        maxlen = 64;

    maxidx = min((signed)sizeof(buf), xdim>>3);

    ei = qrylen;

    if (defaultstr)
    {
        int32_t deflen = Bstrlen(defaultstr);
        Bmemcpy(&buf[ei], defaultstr, deflen);
        ei += deflen;
    }

    buf[ei] = '_';
    buf[ei+1] = 0;

    while (1)
    {
        if (in3dmode())
            getnumber_clearline();

        if (in3dmode())
            printext256(0, 0, whitecol, 0, buf, 0);
        else
            _printmessage16("%s", buf);

        showframe(1);

        if (handleevents())
            quitevent = 0;

        idle();
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
            return completion ? NULL : defaultstr;
        }

        if (maxlen!=1)
        {
            // blink...
            if (totalclock&32)
            {
                buf[ei] = '_';
                buf[ei+1] = 0;
            }
            else
            {
                buf[ei] = 0;
            }

            if (ei>qrylen && (ch==8 || ch==127))
            {
                buf[ei--] = 0;
                buf[ei] = '_';
                havecompl = 0;
            }
            else if (ei<maxidx-2 && ei-qrylen<maxlen && isprint(ch))
            {
                if (completion==2 && ch==' ')
                    ch = '_';
                buf[ei++] = ch;
                buf[ei] = '_';
                buf[ei+1] = 0;
            }

            if (completion && ((ei>qrylen && ch==9) || havecompl))  // tab: maybe do auto-completion
            {
                char cmpbuf[128];
                char completions[3][16];
                const char *cmpstr;
                int32_t len=ei-qrylen, i, j, k=len, first=1, numcompl=0;

                Bmemcpy(cmpbuf, &buf[qrylen], len);
                cmpbuf[len] = 0;

                for (i=(completion!=1); i<((completion==1)?MAXTILES:32768); i++)
                {
                    cmpstr = (completion==1) ? names[i] : taglab_getlabel(i);
                    if (!cmpstr)
                        continue;

                    if (Bstrncasecmp(cmpbuf, cmpstr, len) || Bstrlen(cmpstr)==(unsigned)len)  // compare the prefix
                        continue;

                    if (ch==9)
                    {
                        if (first)
                        {
                            Bstrncpy(cmpbuf+len, cmpstr+len, sizeof(cmpbuf)-len);
                            cmpbuf[sizeof(cmpbuf)-1] = 0;
                            first = 0;
                        }
                        else
                        {
                            for (k=len; cmpstr[k] && cmpbuf[k] && Btolower(cmpstr[k])==Btolower(cmpbuf[k]); k++)
                                /* nop */;
                            cmpbuf[k] = 0;
                        }
                    }

                    if (numcompl<3)
                    {
                        Bstrncpyz(completions[numcompl], cmpstr+len, sizeof(completions[0]));

                        for (k=0; completions[numcompl][k]; k++)
                            completions[numcompl][k] = Btolower(completions[numcompl][k]);
                        numcompl++;
                    }

                    for (k=len; cmpbuf[k]; k++)
                        cmpbuf[k] = Btolower(cmpbuf[k]);
                }

                ei = qrylen;
                for (i=0; i<k && i<maxlen && ei<maxidx-2; i++)
                    buf[ei++] = cmpbuf[i];

                if (k==len && numcompl>0)  // no chars autocompleted/completion request
                {
                    buf[ei] = '{';
                    buf[ei+1] = 0;

                    i = ei+1;
                    for (k=0; k<numcompl; k++)
                    {
                        j = 0;
                        while (i<maxidx-1 && completions[k][j])
                            buf[i++] = completions[k][j++];
                        if (i<maxidx-1)
                            buf[i++] = (k==numcompl-1) ? '}' : ',';
                    }
                    buf[i] = 0;

                    havecompl = 1;
                }
                else
                {
                    buf[ei] = '_';
                    buf[ei+1] = 0;
                }
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

static int32_t getfilenames(const char *path, const char *kind)
{
    const int32_t addflags = (!pathsearchmode && grponlymode ? CACHE1D_OPT_NOSTACK : 0);

    fnlist_getnames(&fnlist, path, kind, addflags|CACHE1D_FIND_DRIVE, addflags);

    finddirshigh = fnlist.finddirs;
    findfileshigh = fnlist.findfiles;
    currentlist = (findfileshigh != NULL);

    return 0;
}

static void tweak_sboardfilename(void)
{
    if (pathsearchmode)
        Bcanonicalisefilename(selectedboardfilename, 1);  // clips off the last token and compresses relative path
    else
        Bcorrectfilename(selectedboardfilename, 1);
}

////////// FILE SELECTION MENU //////////

static char g_oldpath[BMAX_PATH];

static void menuselect_try_findlast(void)
{
    // PK 20080103: start with last selected map
    const char *boardbasename = getbasefn(boardfilename);

    for (; findfileshigh; findfileshigh=findfileshigh->next)
        if (!Bstrcmp(findfileshigh->name, boardbasename))
            break;

    if (!findfileshigh)
        findfileshigh = fnlist.findfiles;
}

// vvv PK ------------------------------------
// copied off menuselect

static int32_t menuselect_auto(int direction, int skip) // 20080104: jump to next (direction!=0) or prev (direction==0) file
{
    Bstrcpy(selectedboardfilename, g_oldpath);
    tweak_sboardfilename();

    getfilenames(selectedboardfilename, "*.map");
    if (fnlist.numfiles==0)
        return -2;

    menuselect_try_findlast();

    do
    {
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
    } while (skip--);

    Bstrcat(selectedboardfilename, findfileshigh->name);

    return 0;
}
// ^^^ PK ------------------------------------

static int32_t menuselect(void)
{
    int32_t listsize;
    int32_t i;
    char ch, buffer[96];
    const int32_t bakpathsearchmode = pathsearchmode;

    Bstrcpy(selectedboardfilename, g_oldpath);
    tweak_sboardfilename();

    getfilenames(selectedboardfilename, "*.map");

    menuselect_try_findlast();

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
            Bsnprintf(buffer, sizeof(buffer),
                      "Game filesystem %smode.  Ctrl-F: local filesystem, Ctrl-G: %s",
                      grponlymode?"GRP-only ":"", grponlymode?"all files":"GRP contents only");

        printext16(halfxdim16-(8*Bstrlen(buffer)/2), 4, editorcolors[12],editorcolors[0],buffer,0);

        Bsnprintf(buffer, sizeof(buffer), "(%d dirs, %d files) %s",
                  fnlist.numdirs, fnlist.numfiles, selectedboardfilename);

        printext16(8,ydim16-8-1,editorcolors[8],editorcolors[0],buffer,0);

        if (finddirshigh)
        {
            const CACHE1D_FIND_REC *dir = finddirshigh;

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
            const CACHE1D_FIND_REC *dir = findfileshigh;

            for (i=(listsize/2)-1; i>=0; i--)
            {
                if (!dir->prev) break;
                else dir=dir->prev;
            }
            for (i=0; (i<listsize && dir); i++, dir=dir->next)
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
        keystatus[0x0e] = 0;    //backspace
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
                CACHE1D_FIND_REC *seeker = currentlist ? fnlist.findfiles : fnlist.finddirs;
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
            Bstrcpy(g_oldpath, selectedboardfilename);
        }
        else if (ch==7)  // Ctrl-G
        {
            if (!pathsearchmode)
            {
                grponlymode = 1-grponlymode;
                getfilenames(selectedboardfilename, "*.map");
                Bstrcpy(g_oldpath, selectedboardfilename);
            }
        }
        else if (ch == 9)
        {
            if ((currentlist == 0 && fnlist.findfiles) || (currentlist == 1 && fnlist.finddirs))
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
        else if (keystatus[0x0e]) // backspace
        {
            Bstrcat(selectedboardfilename, "../");
            tweak_sboardfilename();
            Bstrcpy(g_oldpath, selectedboardfilename);
            getfilenames(selectedboardfilename, "*.map");
            keystatus[0x0e] = 0;
        }
        else if (ch == 13 && currentlist == 0)
        {
            if (finddirshigh->type == CACHE1D_FIND_DRIVE)
                Bstrcpy(selectedboardfilename, finddirshigh->name);
            else
                Bstrcat(selectedboardfilename, finddirshigh->name);

            Bstrcat(selectedboardfilename, "/");
            tweak_sboardfilename();

            Bstrcpy(g_oldpath, selectedboardfilename);
            //printf("Changing directories to: %s\n", selectedboardfilename);

            if (finddirshigh == fnlist.finddirs)
            {
                getfilenames(selectedboardfilename, "*.map");
                currentlist = 0;
            }
            else getfilenames(selectedboardfilename, "*.map");;


            ch = 0;

            begindrawing();
            CLEARLINES2D(0, ydim16, 0);
            enddrawing();
            showframe(1);
        }

        if (ch == 13 && !findfileshigh) ch = 0;
    }
    while (ch != 13 && ch != 27);

    if (ch == 13)
    {
        Bstrcat(selectedboardfilename, findfileshigh->name);
        //printf("Selected file: %s\n", selectedboardfilename);

        return 0;
    }

    pathsearchmode = bakpathsearchmode;

    return -1;
}

#if 0
static inline int32_t imod(int32_t a, int32_t b)
{
    if (a >= 0)
        return a%b;

    return ((a+1)%b)+b-1;
}
#endif

int32_t fillsector_maybetrans(int16_t sectnum, int32_t fillcolor, uint8_t dotrans)
{
    if (sectnum == -1)
        return 0;

    int32_t x1, x2, y1, y2, sy, y, daminy;
    int32_t lborder, rborder, uborder, dborder, miny, maxy, dax;
    int16_t z, zz, startwall, endwall, fillcnt;

    char col;

    if (fillcolor == -1)
        col = editorcolors[14]-(M32_THROB>>1);
    else
        col = fillcolor;

    lborder = 0; rborder = xdim;
    y = OSD_GetRowsCur();
    uborder = (y>=0)?(y+1)*8:0; dborder = ydim16-STATUS2DSIZ2;


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
            tempxyar[z][0] = x1;
            tempxyar[z][1] = y1;
        }

        miny = min(miny, y1);
        maxy = max(maxy, y1);
    }

    if (miny < uborder) miny = uborder;
    if (maxy >= dborder) maxy = dborder-1;

    daminy = miny;// +2 - imod(miny+2, 3);
    if (sector[sectnum].floorz > minhlsectorfloorz)
        daminy++;

//+((totalclock>>2)&3)
    for (sy=daminy; sy<=maxy; sy++)	// JBF 20040116: numframes%3 -> (totalclock>>2)&3
    {
        y = pos.y + ((sy-midydim16)<<14)/zoom;

        fillist[0] = lborder; fillcnt = 1;
        for (z=startwall; z<=endwall; z++)
        {
            if (m32_sideview)
            {
                x1 = tempxyar[z][0];
                y1 = tempxyar[z][1];
                x2 = tempxyar[wall[z].point2][0];
                y2 = tempxyar[wall[z].point2][1];

                if (y1 > y2)
                {
                    swaplong(&x1, &x2);
                    swaplong(&y1, &y2);
                }

                if (y1 <= sy && sy < y2)
                {
                    if (fillcnt == ARRAY_SIZE(fillist))
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
                        if (fillcnt == ARRAY_SIZE(fillist))
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

                drawline16(fillist[z]+1,sy, fillist[z+1]-1,sy, dotrans ? -col : col);  //editorcolors[fillcolor]
            }
        }
    }

    return 0;
}

static int16_t whitelinescan(int16_t sucksect, int16_t dalinehighlight)
{
    int32_t i, j, k;
    int16_t tnewnumwalls;

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

    return (clockdir(numwalls) == CLOCKDIR_CCW) ? -1 : tnewnumwalls;
}

//
// names.h loading
//

enum {
    T_INCLUDE = 0,
    T_DEFINE = 1,

    T_DUMMY,
};

static int32_t parsenamesfile(scriptfile *script)
{
    int32_t syms = 0;

    tokenlist const tokens[] =
    {
        { "include",         T_INCLUDE          },
        { "#include",        T_INCLUDE          },
        { "define",          T_DEFINE           },
        { "#define",         T_DEFINE           },

        { "dynamicremap",    T_DUMMY            },
    };

    while (1)
    {
        int32_t tokn = getatoken(script,tokens,ARRAY_SIZE(tokens));
        char *cmdtokptr = script->ltextptr;
        switch (tokn)
        {
        case T_INCLUDE:
        {
            char *fn;
            if (scriptfile_getstring(script,&fn))
            {
                initprintf("Error: Malformed include on line %s:%d\n",
                           script->filename,scriptfile_getlinum(script,cmdtokptr));
                break;
            }

            scriptfile *included;

            included = scriptfile_fromfile(fn);
            if (!included)
            {
                initprintf("Error: Failed including %s on line %s:%d\n",
                           fn, script->filename,scriptfile_getlinum(script,cmdtokptr));
                break;
            }

            initprintf("Including: %s\n", fn);

            syms += parsenamesfile(included);
            scriptfile_close(included);

            break;
        }
        case T_DEFINE:
        {
            char *name;
            int32_t number;

            if (scriptfile_getstring(script,&name))
            {
                initprintf("Error: Malformed define on line %s:%d\n",
                           script->filename, scriptfile_getlinum(script,cmdtokptr));
                break;
            }

            if (scriptfile_getsymbol(script,&number))
            {
                initprintf("Error: No number given for name \"%s\" on line %s:%d\n",
                           name, script->filename, scriptfile_getlinum(script,cmdtokptr));
                break;
            }

            if ((unsigned)number >= MAXTILES)
            {
                initprintf("Error: Constant %d for name \"%s\" out of range on line %s:%d\n",
                           number, name, script->filename, scriptfile_getlinum(script,cmdtokptr));
                break;
            }

            if (Bstrlen(name) > 24)
                initprintf("Warning: Truncating name \"%s\" to 24 characters on line %s:%d\n",
                           name, script->filename, scriptfile_getlinum(script,cmdtokptr));

            Bstrncpyz(names[number], name, 25);
            name = names[number];

            ++syms;

            if (scriptfile_addsymbolvalue(name,number) < 0)
                initprintf("Warning: Symbol %s was NOT redefined to %d on line %s:%d\n",
                           name, number, script->filename, scriptfile_getlinum(script,cmdtokptr));
            break;
        }
        case T_EOF:
            return syms;
        default:
            break;
        }
    }

    return syms;
}

static void loadnames(const char *namesfile)
{
    scriptfile *script = scriptfile_fromfile(namesfile);

    if (!script)
        return;

    initprintf("Loading names file: %s\n", namesfile);

    int32_t const syms = parsenamesfile(script);
    initprintf("Loaded %d names.\n", syms);

    scriptfile_close(script);

    scriptfile_clearsymbols();
}


void printcoords16(int32_t posxe, int32_t posye, int16_t ange)
{
    char snotbuf[128];
    int32_t i, m;
    int32_t v8 = (numsectors > MAXSECTORSV7 || numwalls > MAXWALLSV7 ||
                  Numsprites > MAXSPRITESV7 || numyaxbunches > 0);
#if M32_UNDO
    Bsprintf(snotbuf,"x:%d y:%d ang:%d r%d",posxe,posye,ange,map_revision-1);
#else
    Bsprintf(snotbuf,"x:%d y:%d ang:%d",posxe,posye,ange);
#endif
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
        m = Bsprintf(snotbuf,"%d/%d %s. %d",
                     numsectors, v8?MAXSECTORSV8:MAXSECTORSV7,
                     numyaxbunches>0 ? "SEC":"sec", numwalls);
        if (numyaxbunches > 0)
        {
            if (xdim >= 800)
                Bsprintf(&snotbuf[m], "/%d wal. %d/16k spr. %d/256 bn.",
                         MAXWALLSV8, Numsprites, numyaxbunches);
            else
                Bsprintf(&snotbuf[m], " wal. %d spr. %d/256 bn.",
                         Numsprites, numyaxbunches);
        }
        else
        {
            if (xdim >= 800)
                Bsprintf(&snotbuf[m], "/%d wal. %d/%d spr.",
                         v8?MAXWALLSV8:MAXWALLSV7, Numsprites,
                         v8?MAXSPRITESV8:MAXSPRITESV7);
            else
                Bsprintf(&snotbuf[m], "/%dk wal. %d/%dk spr.",
                         (v8?MAXWALLSV8:MAXWALLSV7)/1000, Numsprites,
                         (v8?MAXSPRITESV8:MAXSPRITESV7)/1000);
        }
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
        {
            int32_t ii=Bsprintf(snotbuf, "%d sectors with a total of %d walls selected",
                                highlightsectorcnt, numhlsecwalls);
            if (m32_rotateang)
                Bsprintf(&snotbuf[ii], " (ang=%d)", m32_rotateang);
        }
        else
        {
            snotbuf[0] = 0;
        }

        v8 = 1;  // yellow color
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

#define DOPRINT(Yofs, fmt, ...) do { \
    Bsprintf(snotbuf, fmt, ## __VA_ARGS__); \
    printext16(8+col*200, ydim/*-(row*96)*/-STATUS2DSIZ+Yofs, color, -1, snotbuf, 0); \
    } while (0)

void showsectordata(int16_t sectnum, int16_t small)
{
    char snotbuf[80];
    int32_t col=0;  //,row = 0;
    int32_t color = small ? whitecol : editorcolors[11];

    const sectortype *const sec = &sector[sectnum];

    if (small)
    {
        _printmessage16("^10Sector %d %s ^O(F7 to edit)", sectnum, CallExtGetSectorCaption(sectnum));
        return;
    }

    DOPRINT(32, "^10Sector %d", sectnum);
    DOPRINT(48, "Firstwall: %d", TrackerCast(sec->wallptr));
    DOPRINT(56, "Numberofwalls: %d", TrackerCast(sec->wallnum));
    DOPRINT(64, "Firstsprite: %d", headspritesect[sectnum]);
    DOPRINT(72, "Tags: %d, %d", TrackerCast(sec->hitag), TrackerCast(sec->lotag));
    DOPRINT(80, "     (0x%x), (0x%x)", TrackerCast(sec->hitag), TrackerCast(sec->lotag));
    DOPRINT(88, "Extra: %d", TrackerCast(sec->extra));
    DOPRINT(96, "Visibility: %d", TrackerCast(sec->visibility));
    DOPRINT(104, "Pixel height: %d", (sec->floorz-sec->ceilingz)>>8);

    col++;

    DOPRINT(32, "^10CEILING:^O");
    DOPRINT(48, "Flags (hex): %x", TrackerCast(sec->ceilingstat));
    {
        int32_t xp=sec->ceilingxpanning, yp=sec->ceilingypanning;
#ifdef YAX_ENABLE__COMPAT
        if (yax_getbunch(sectnum, YAX_CEILING) >= 0)
            xp = yp = 0;
#endif
        DOPRINT(56, "(X,Y)pan: %d, %d", xp, yp);
    }
    DOPRINT(64, "Shade byte: %d", TrackerCast(sec->ceilingshade));
    DOPRINT(72, "Z-coordinate: %d", TrackerCast(sec->ceilingz));
    DOPRINT(80, "Tile number: %d", TrackerCast(sec->ceilingpicnum));
    DOPRINT(88, "Ceiling heinum: %d", TrackerCast(sec->ceilingheinum));
    DOPRINT(96, "Palookup number: %d", TrackerCast(sec->ceilingpal));
#ifdef YAX_ENABLE
    DOPRINT(104, "Bunch number: %d", yax_getbunch(sectnum, YAX_CEILING));
#endif

    col++;

    DOPRINT(32, "^10FLOOR:^O");
    DOPRINT(48, "Flags (hex): %x", TrackerCast(sec->floorstat));
    {
        int32_t xp=sec->floorxpanning, yp=sec->floorypanning;
#ifdef YAX_ENABLE__COMPAT
        if (yax_getbunch(sectnum, YAX_FLOOR) >= 0)
            xp = yp = 0;
#endif
        DOPRINT(56, "(X,Y)pan: %d, %d", xp, yp);
    }
    DOPRINT(64, "Shade byte: %d", TrackerCast(sec->floorshade));
    DOPRINT(72, "Z-coordinate: %d", TrackerCast(sec->floorz));
    DOPRINT(80, "Tile number: %d", TrackerCast(sec->floorpicnum));
    DOPRINT(88, "Floor heinum: %d", TrackerCast(sec->floorheinum));
    DOPRINT(96, "Palookup number: %d", TrackerCast(sec->floorpal));
#ifdef YAX_ENABLE
    DOPRINT(104, "Bunch number: %d", yax_getbunch(sectnum, YAX_FLOOR));
#endif
}

void showwalldata(int16_t wallnum, int16_t small)
{
    walltype const * const wal = &wall[wallnum];
    int32_t sec;
    char snotbuf[80];
    int32_t col=0; //, row = 0;
    int32_t color = small ? whitecol : editorcolors[11];

    if (small)
    {
        _printmessage16("^10Wall %d %s ^O(F8 to edit)", wallnum,
                        CallExtGetWallCaption(wallnum));
        return;
    }

    DOPRINT(32, "^10Wall %d", wallnum);
    DOPRINT(48, "X-coordinate: %d", TrackerCast(wal->x));
    DOPRINT(56, "Y-coordinate: %d", TrackerCast(wal->y));
    DOPRINT(64, "Point2: %d", TrackerCast(wal->point2));
    DOPRINT(72, "Sector: ^010%d", sectorofwall(wallnum));

    DOPRINT(88, "Tags: %d,  %d", TrackerCast(wal->hitag), TrackerCast(wal->lotag));
    DOPRINT(96, "     (0x%x),  (0x%x)", TrackerCast(wal->hitag), TrackerCast(wal->lotag));

    col++;

    DOPRINT(32, "^10%s^O", (wal->picnum>=0 && wal->picnum<MAXTILES) ? names[wal->picnum] : "!INVALID!");
    DOPRINT(48, "Flags (hex): %x", TrackerCast(wal->cstat));
    DOPRINT(56, "Shade: %d", TrackerCast(wal->shade));
    DOPRINT(64, "Pal: %d", TrackerCast(wal->pal));
    DOPRINT(72, "(X,Y)repeat: %d, %d", TrackerCast(wal->xrepeat), TrackerCast(wal->yrepeat));
    DOPRINT(80, "(X,Y)pan: %d, %d", TrackerCast(wal->xpanning), TrackerCast(wal->ypanning));
    DOPRINT(88, "Tile number: %d", TrackerCast(wal->picnum));
    DOPRINT(96, "OverTile number: %d", TrackerCast(wal->overpicnum));

    col++;

    DOPRINT(48-(small?16:0), "nextsector: %d", TrackerCast(wal->nextsector));
    DOPRINT(56-(small?16:0), "nextwall: %d", TrackerCast(wal->nextwall));

    DOPRINT(72-(small?16:0), "Extra: %d", TrackerCast(wal->extra));

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
        _printmessage16("^10Sprite %d %s ^O(F8 to edit)",spritenum, CallExtGetSpriteCaption(spritenum));
        return;
    }

    DOPRINT(32, "^10Sprite %d", spritenum);
    DOPRINT(48, "X-coordinate: %d", TrackerCast(spr->x));
    DOPRINT(56, "Y-coordinate: %d", TrackerCast(spr->y));
    DOPRINT(64, "Z-coordinate: %d", TrackerCast(spr->z));

    DOPRINT(72, "Sectnum: ^010%d", TrackerCast(spr->sectnum));
    DOPRINT(80, "Statnum: %d", TrackerCast(spr->statnum));

    DOPRINT(96, "Tags: %d,  %d", TrackerCast(spr->hitag), TrackerCast(spr->lotag));
    DOPRINT(104, "     (0x%x),  (0x%x)", TrackerCast(spr->hitag), TrackerCast(spr->lotag));

    col++;

    DOPRINT(32, "^10,0                        ^O");  // 24 blanks
    DOPRINT(32, "^10%s^O", (spr->picnum>=0 && spr->picnum<MAXTILES) ? names[spr->picnum] : "!INVALID!");
    DOPRINT(48, "Flags (hex): %x", TrackerCast(spr->cstat));
    DOPRINT(56, "Shade: %d", TrackerCast(spr->shade));
    DOPRINT(64, "Pal: %d", TrackerCast(spr->pal));
    DOPRINT(72, "Blend: %d", TrackerCast(spr->blend));
    DOPRINT(80, "(X,Y)repeat: %d, %d", TrackerCast(spr->xrepeat), TrackerCast(spr->yrepeat));
    DOPRINT(88, "(X,Y)offset: %d, %d", TrackerCast(spr->xoffset), TrackerCast(spr->yoffset));
    DOPRINT(96, "Tile number: %d", TrackerCast(spr->picnum));

    col++;

    DOPRINT(48, "Angle (2048 degrees): %d", TrackerCast(spr->ang));
    DOPRINT(56, "X-Velocity: %d", TrackerCast(spr->xvel));
    DOPRINT(64, "Y-Velocity: %d", TrackerCast(spr->yvel));
    DOPRINT(72, "Z-Velocity: %d", TrackerCast(spr->zvel));
    DOPRINT(80, "Owner: %d", TrackerCast(spr->owner));
    DOPRINT(88, "Clipdist: %d", TrackerCast(spr->clipdist));
    DOPRINT(96, "Extra: %d", TrackerCast(spr->extra));
}

#undef DOPRINT

// gets called once per totalclock increment since last call
static void keytimerstuff(void)
{
    if (!g_doHardcodedMovement)
        return;

    if (DOWN_BK(STRAFE) == 0)
    {
        if (DOWN_BK(TURNLEFT)) angvel = max(angvel-pk_turnaccel, -127);
        if (DOWN_BK(TURNRIGHT)) angvel = min(angvel+pk_turnaccel, 127);
    }
    else
    {
        if (DOWN_BK(TURNLEFT)) svel = min(svel+16, 255); // svel and vel aren't even chars...
        if (DOWN_BK(TURNRIGHT)) svel = max(svel-16, -255);
    }
    if (DOWN_BK(MOVEFORWARD))  vel = min(vel+16, 255);
    if (DOWN_BK(MOVEBACKWARD)) vel = max(vel-16, -255);
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
    Bvsnprintf(tmpstr, sizeof(tmpstr), fmt, va);
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

    ybase = ydim-STATUS2DSIZ+128-8;

    printext16(8, ybase+8, whitecol, -1, snotbuf, 0);
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
static void getclosestpointonwall(int32_t x, int32_t y, int32_t dawall, int32_t *nx, int32_t *ny,
                                  int32_t maybe_screen_coord_p)
{
    int64_t i, j, wx,wy, wx2,wy2, dx, dy;

    if (m32_sideview && maybe_screen_coord_p)
    {
        wx = m32_wallscreenxy[dawall][0];
        wy = m32_wallscreenxy[dawall][1];
        wx2 = m32_wallscreenxy[wall[dawall].point2][0];
        wy2 = m32_wallscreenxy[wall[dawall].point2][1];
    }
    else
    {
        wx = wall[dawall].x;
        wy = wall[dawall].y;
        wx2 = POINT2(dawall).x;
        wy2 = POINT2(dawall).y;
    }

    dx = wx2 - wx;
    dy = wy2 - wy;
    i = dx*(x-wx) + dy*(y-wy);
    if (i <= 0) { *nx = wx; *ny = wy; return; }
    j = dx*dx + dy*dy;
    if (i >= j) { *nx = wx2; *ny = wy2; return; }
    i=((i<<15)/j)<<15;
    *nx = wx + ((dx*i)>>30);
    *ny = wy + ((dy*i)>>30);
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

static int32_t GetWallBaseZ(int32_t wallnum)
{
    int32_t z=0;

    const int32_t sectnum = sectorofwall(wallnum);
    const int32_t nextsec = wall[wallnum].nextsector;

    if (nextsec == -1)  //1-sided wall
    {
        if (wall[wallnum].cstat&4)  // floor-aligned
            z = sector[sectnum].floorz;
        else
            z = sector[sectnum].ceilingz;
    }
    else  //2-sided wall
    {
        if (wall[wallnum].cstat&4)
            z = sector[sectnum].ceilingz;
        else
        {
            if (sector[nextsec].ceilingz > sector[sectnum].ceilingz)
                z = sector[nextsec].ceilingz;   //top step
            if (sector[nextsec].floorz < sector[sectnum].floorz)
                z = sector[nextsec].floorz;   //bottom step
        }
    }

    return z;
}


////////// AUTOMATIC WALL ALIGNMENT //////////

static void AlignWalls_(int32_t tilenum, int32_t z0, int32_t z1, int32_t doxpanning,
                        int32_t w0_pan, int32_t w0_rep, int32_t w1_pan, int32_t w1_rep)
{
    int32_t n;

    if (tilesiz[tilenum].x==0 || tilesiz[tilenum].y==0)
        return;

    //do the x alignment
    if (doxpanning)
        wall[w1_pan].xpanning = (uint8_t)((wall[w0_pan].xpanning + (wall[w0_rep].xrepeat<<3))%tilesiz[tilenum].x);

    for (n=picsiz[tilenum]>>4; (1<<n)<tilesiz[tilenum].y; n++) { }

    wall[w1_rep].yrepeat = wall[w0_rep].yrepeat;
    wall[w1_pan].ypanning = (uint8_t)(wall[w0_pan].ypanning + (((z1-z0)*wall[w0_rep].yrepeat)>>(n+3)));
}

static void AlignWalls(int32_t w0, int32_t z0, int32_t w1, int32_t z1, int32_t doxpanning)
{
    AlignWalls_(wall[w0].picnum, z0, z1, doxpanning, w0, w0, w1, w1);
}

void AlignWallPoint2(int32_t w0)
{
    int32_t w1 = wall[w0].point2;
    AlignWalls(w0,GetWallBaseZ(w0), w1,GetWallBaseZ(w1), 1);
}

#define ALIGN_WALLS_CSTAT_MASK (4+8+256)

static int32_t AlignGetWall(int32_t botswap, int32_t w)
{
    return botswap && (wall[w].cstat&2) && wall[w].nextwall >= 0 ? wall[w].nextwall : w;
}

// flags:
//  1: more than once
//  2: (unused)
//  4: carry pixel width from first wall over to the rest
//  8: align TROR nextwalls
// 16: iterate lastwall()s (point2 in reverse)
// 32: use special logic for 'bottom-swapped' walls
int32_t AutoAlignWalls(int32_t w0, uint32_t flags, int32_t nrecurs)
{
    static int32_t numaligned, wall0, cstat0;
    static uint32_t lenrepquot;

    const int32_t totheleft = flags&16;
    const int32_t botswap = flags&32;

    int32_t z0 = GetWallBaseZ(w0);
    int32_t w1 = totheleft ? lastwall(w0) : wall[w0].point2;

    int32_t w0b = AlignGetWall(botswap, w0);
    const int32_t tilenum = wall[w0b].picnum;

    if (nrecurs == 0)
    {
        //clear visited bits
        Bmemset(visited, 0, sizeof(visited));
        visited[w0>>3] |= (1<<(w0&7));
        numaligned = 0;
        lenrepquot = getlenbyrep(wallength(w0), wall[w0].xrepeat);
        wall0 = w0;
        cstat0 = wall[w0b].cstat & ALIGN_WALLS_CSTAT_MASK;  // top/bottom orientation; x/y-flip
    }

    //loop through walls at this vertex in point2 order
    while (1)
    {
        int32_t w1b = AlignGetWall(botswap, w1);

        //break if this wall would connect us in a loop
        if (visited[w1>>3]&(1<<(w1&7)))
            break;

        visited[w1>>3] |= (1<<(w1&7));

#ifdef YAX_ENABLE
        if (flags&8)
        {
            int32_t cf, ynw;

            for (cf=0; cf<2; cf++)
            {
                ynw = yax_getnextwall(w0, cf);

                if (ynw >= 0 && wall[ynw].picnum==tilenum && (visited[ynw>>3]&(1<<(ynw&7)))==0)
                {
                    wall[ynw].xrepeat = wall[w0].xrepeat;
                    wall[ynw].xpanning = wall[w0].xpanning;
                    AlignWalls(w0,z0, ynw,GetWallBaseZ(ynw), 0);  // initial vertical alignment
                }
            }
        }
#endif

        //break if reached back of left wall
        if (wall[w1].nextwall == w0)
            break;

        if (wall[w1b].picnum == tilenum)
        {
            int32_t visible = 1;
            const int32_t nextsec = wall[w1].nextsector;

            if (nextsec >= 0)
            {
                int32_t cz,fz, czn,fzn;
                const int32_t sectnum = NEXTWALL(w1).nextsector;

                //ignore two sided walls that have no visible face
                // TODO: can be more precise (i.e. taking into account the wall face)
                //  ... needs to be factored out from some engine code maybe...
                // as is the whole "z base" determination.
                getzsofslope(sectnum, wall[w1].x,wall[w1].y, &cz, &fz);
                getzsofslope(nextsec, wall[w1].x,wall[w1].y, &czn, &fzn);

                if (czn <= cz && fzn >= fz)
                    visible = 0;
            }

            if (visible)
            {
                const int32_t z1 = GetWallBaseZ(w1);

                if ((flags&4) && w1!=wall0)
                    fixxrepeat(w1, lenrepquot);
                AlignWalls_(tilenum,z0, z1, 1, w0b, w0, w1b, w1);
                wall[w1b].cstat &= ~ALIGN_WALLS_CSTAT_MASK;
                wall[w1b].cstat |= cstat0;
                numaligned++;

                if ((flags&1)==0)
                    return 1;

                //if wall was 1-sided, no need to recurse
                if (wall[w1].nextwall < 0)
                {
                    w0 = w1;
                    w0b = AlignGetWall(botswap, w0);
                    z0 = GetWallBaseZ(w0);
                    w1 = totheleft ? lastwall(w0) : wall[w0].point2;

                    continue;
                }

                AutoAlignWalls(w1, flags, nrecurs+1);
            }
        }

        if (wall[w1].nextwall < 0)
            break;
        w1 = totheleft ? lastwall(wall[w1].nextwall) : NEXTWALL(w1).point2;
    }

    return numaligned;
}

#define PLAYTEST_MAPNAME "autosave_playtest.map"

void test_map(int32_t mode)
{
    if (!mode)
        updatesector(pos.x, pos.y, &cursectnum);
    else
        updatesector(startpos.x, startpos.y, &startsectnum);

#ifdef _WIN32
    if (fullscreen)
    {
        printmessage16("Must be in windowed mode to test map!");
        return;
    }
#endif

    if ((!mode && cursectnum >= 0) || (mode && startsectnum >= 0))
    {
        char const *param = " -map " PLAYTEST_MAPNAME " -noinstancechecking";
        char current_cwd[BMAX_PATH];
        int32_t slen = 0;
        BFILE *fp;

        if ((program_origcwd[0] == '\0') || !getcwd(current_cwd, BMAX_PATH))
            current_cwd[0] = '\0';
        else // Before we check if file exists, for the case there's no absolute path.
            Bchdir(program_origcwd);

        fp = fopen(game_executable, "rb"); // File exists?
        if (fp != NULL)
            fclose(fp);
        else
        {
            char const * lastslash = (char const *)Bstrrchr(mapster32_fullpath, '/');
#ifdef _WIN32
            char const * lastbackslash = (char const *)Bstrrchr(mapster32_fullpath, '\\');
            lastslash = max(lastslash, lastbackslash);
#endif
            if (lastslash)
            {
                slen = lastslash-mapster32_fullpath+1;
                Bstrncpy(game_executable, mapster32_fullpath, slen);
                Bstrncpy(game_executable+slen, DefaultGameExec, sizeof(game_executable)-slen);
            }
            else
                Bstrncpy(game_executable, DefaultGameLocalExec, sizeof(game_executable));
        }

        if (current_cwd[0] != '\0') // Temporarily changing back,
            Bchdir(current_cwd);     // after checking if file exists.

        if (testplay_addparam)
            slen = Bstrlen(testplay_addparam);

        // Considering the NULL character, quatation marks
        // and a possible extra space not in testplay_addparam,
        // the length should be Bstrlen(game_executable)+Bstrlen(param)+(slen+1)+2+1.

        char *fullparam = (char *)Xmalloc(Bstrlen(game_executable)+Bstrlen(param)+slen+4);
        Bsprintf(fullparam,"\"%s\"",game_executable);

        if (testplay_addparam)
        {
            Bstrcat(fullparam, " ");
            Bstrcat(fullparam, testplay_addparam);
        }
        Bstrcat(fullparam, param);

        CallExtPreSaveMap();
        if (mode)
            saveboard(PLAYTEST_MAPNAME, &startpos, startang, startsectnum);
        else
            saveboard(PLAYTEST_MAPNAME, &pos, ang, cursectnum);

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
            Bchdir(program_origcwd);
            if (system(fullparam))
                message("Error launching the game!");
            Bchdir(current_cwd);
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

void app_crashhandler(void)
{
    if (levelname[0])
    {
        append_ext_UNSAFE(levelname, "_crash.map");
        SaveBoard(levelname, M32_SB_NOEXT);
    }
}

// These will be more useful in the future...
static const char *CallExtGetVer(void)
{
    return ExtGetVer();
}
static int32_t CallExtInit(void)
{
    return ExtInit();
}
static int32_t CallExtPreInit(int32_t argc,char const * const * argv)
{
    return ExtPreInit(argc, argv);
}
static int32_t CallExtPostStartupWindow(void)
{
    return ExtPostStartupWindow();
}
static void CallExtPostInit(void)
{
    return ExtPostInit();
}
static void CallExtUnInit(void)
{
    ExtUnInit();
}
static void CallExtPreCheckKeys(void)
{
    ExtPreCheckKeys();
}
static void CallExtAnalyzeSprites(int32_t ourx, int32_t oury, int32_t oura, int32_t smoothr)
{
    ExtAnalyzeSprites(ourx, oury, oura, smoothr);
    VM_OnEvent(EVENT_ANALYZESPRITES, -1);
}
static void CallExtCheckKeys(void)
{
    ExtCheckKeys();
}
static void CallExtPreLoadMap(void)
{
    VM_OnEvent(EVENT_PRELOADMAP, -1);
    ExtPreLoadMap();
}
static void CallExtSetupMapFilename(const char *mapname)
{
    Bstrncpy(levelname, mapname, sizeof(levelname));

    Bsnprintf(tempbuf, sizeof(tempbuf), "%s - %s", AppProperName, mapname);
    wm_setapptitle(tempbuf);

    ExtSetupMapFilename(mapname);
}
static void CallExtLoadMap(const char *mapname)
{
    CallExtSetupMapFilename(mapname);
    ExtLoadMap(mapname);
    VM_OnEvent(EVENT_LOADMAP, -1);
}
static int32_t CallExtPreSaveMap(void)
{
    VM_OnEvent(EVENT_PRESAVEMAP, -1);
    return ExtPreSaveMap();
}
static void CallExtSaveMap(const char *mapname)
{
    ExtSaveMap(mapname);
    saveboard("backup.map", &pos, ang, cursectnum);
    VM_OnEvent(EVENT_SAVEMAP, -1);
}
static void CallExtShowSectorData(int16_t sectnum)
{
    ExtShowSectorData(sectnum);
}
static void CallExtShowWallData(int16_t wallnum)
{
    ExtShowWallData(wallnum);
}
static void CallExtShowSpriteData(int16_t spritenum)
{
    ExtShowSpriteData(spritenum);
}
static void CallExtEditSectorData(int16_t sectnum)
{
    ExtEditSectorData(sectnum);
}
static void CallExtEditWallData(int16_t wallnum)
{
    ExtEditWallData(wallnum);
}
static void CallExtEditSpriteData(int16_t spritenum)
{
    ExtEditSpriteData(spritenum);
}
#if 0
static const char *CallExtGetSectorType(int32_t lotag)
{
    return ExtGetSectorType(lotag);
}
#endif
