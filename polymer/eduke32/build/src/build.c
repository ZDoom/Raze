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

#define VERSION "("__DATE__" "__TIME__")"

#include "baselayer.h"
#ifdef RENDERTYPEWIN
#include "winlayer.h"
#endif


#define TIMERINTSPERSECOND 120

#define updatecrc16(crc,dat) (crc = (((crc<<8)&65535)^crctable[((((unsigned short)crc)>>8)&65535)^dat]))
static int crctable[256];
static char kensig[64];

extern int ExtInit(void);
extern int ExtPreInit(int argc,const char **argv);
extern void ExtUnInit(void);
extern void ExtPreCheckKeys(void);
#ifdef SUPERBUILD
extern void ExtAnalyzeSprites(void);
#endif
extern void ExtCheckKeys(void);
extern void ExtLoadMap(const char *mapname);
extern void ExtSaveMap(const char *mapname);
extern const char *ExtGetSectorCaption(short sectnum);
extern const char *ExtGetWallCaption(short wallnum);
extern const char *ExtGetSpriteCaption(short spritenum);
extern void ExtShowSectorData(short sectnum);
extern void ExtShowWallData(short wallnum);
extern void ExtShowSpriteData(short spritenum);
extern void ExtEditSectorData(short sectnum);
extern void ExtEditWallData(short wallnum);
extern void ExtEditSpriteData(short spritenum);
extern const char *ExtGetSectorType(int lotag);

extern char spritecol2d[MAXTILES][2];

char noclip=0;

int vel, svel, angvel;

// 0   1     2     3      4       5      6      7
// up, down, left, right, lshift, rctrl, lctrl, space
// 8  9  10    11    12   13
// a, z, pgdn, pgup, [,], [.]
// 14       15     16 17 18   19
// kpenter, enter, =, -, tab, `
unsigned char buildkeys[NUMBUILDKEYS] =
{
    0xc8,0xd0,0xcb,0xcd,0x2a,0x9d,0x1d,0x39,
    0x1e,0x2c,0xd1,0xc9,0x33,0x34,
    0x9c,0x1c,0xd,0xc,0xf,0x29
};

int posx, posy, posz, horiz = 100;
int mousexsurp = 0, mouseysurp = 0;
short ang, cursectnum;
int hvel;

int grponlymode = 0;
extern int editorgridextent;	// in engine.c
extern double msens;

int synctics = 0, lockclock = 0;

extern char vgacompatible;

extern char picsiz[MAXTILES];
extern int startposx, startposy, startposz;
extern short startang, startsectnum;
extern int ydim16, halfxdim16, midydim16;
extern intptr_t frameplace;
int xdim2d = 640, ydim2d = 480, xdimgame = 640, ydimgame = 480, bppgame = 8;
int forcesetup = 1;

extern int cachesize, artsize;

static short oldmousebstatus = 0;
short brightness = 0;
char game_executable[BMAX_PATH] = DEFAULT_GAME_LOCAL_EXEC;
int zlock = 0x7fffffff, zmode = 0, whitecol, kensplayerheight = 32;
short defaultspritecstat = 0;

short localartfreq[MAXTILES];
short localartlookup[MAXTILES], localartlookupnum;

char tempbuf[4096];

char names[MAXTILES][25];

short asksave = 0;
extern short editstatus, searchit;
extern int searchx, searchy;                          //search input
extern short searchsector, searchwall, searchstat;     //search output
int osearchx, osearchy;                               //old search input

extern short pointhighlight, linehighlight, highlightcnt;
short grid = 3, gridlock = 1, showtags = 1;
int zoom = 768, gettilezoom = 1;
int lastpm16time;

int numsprites;
extern int mapversion;

short highlight[MAXWALLS];
short highlightsector[MAXSECTORS], highlightsectorcnt = -1;
extern char textfont[128][8];

char pskysearch[MAXSECTORS];

int temppicnum, tempcstat, templotag, temphitag, tempextra;
unsigned int temppal, tempvis, tempxrepeat, tempyrepeat;
int tempshade, tempxvel, tempyvel, tempzvel;
unsigned char somethingintab = 255;

int mlook = 0,mskip=0;
int revertCTRL=0,scrollamount=3;
int unrealedlook=1, quickmapcycling=1; //PK

static char boardfilename[BMAX_PATH], selectedboardfilename[BMAX_PATH];

static CACHE1D_FIND_REC *finddirs=NULL, *findfiles=NULL, *finddirshigh=NULL, *findfileshigh=NULL;
static int numdirs=0, numfiles=0;
static int currentlist=0;

//static int repeatcountx, repeatcounty;

static int fillist[640];

int mousx, mousy;
short prefixtiles[16] = { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10 };
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
int pk_turnaccel=16;
int pk_turndecel=12;
int pk_uedaccel=3;

char changechar(char dachar, int dadir, char smooshyalign, char boundcheck);
int adjustmark(int *xplc, int *yplc, short danumwalls);
int checkautoinsert(int dax, int day, short danumwalls);
void keytimerstuff(void);
int clockdir(short wallstart);
void flipwalls(short numwalls, short newnumwalls);
void insertpoint(short linehighlight, int dax, int day);
void deletepoint(short point);
int deletesector(short sucksect);
int checksectorpointer(short i, short sectnum);
void fixrepeats(short i);
short loopinside(int x, int y, short startwall);
int fillsector(short sectnum, char fillcolor);
short whitelinescan(short dalinehighlight);
void printcoords16(int posxe, int posye, short ange);
void copysector(short soursector, short destsector, short deststartwall, char copystat);
void showsectordata(short sectnum);
void showwalldata(short wallnum);
void showspritedata(short spritenum);
int drawtilescreen(int pictopleft, int picbox);
void overheadeditor(void);
int getlinehighlight(int xplc, int yplc, int line);
void fixspritesectors(void);
int movewalls(int start, int offs);
int loadnames(void);
void updatenumsprites(void);
void getclosestpointonwall(int x, int y, int dawall, int *nx, int *ny);
void initcrc(void);
void AutoAlignWalls(int nWall0, int ply);
int gettile(int tilenum);

int menuselect(void);

int menuselect_pk(int); //PK

int getfilenames(char *path, char *kind);
void clearfilenames(void);
void loadmhk();

void clearkeys(void) { memset(keystatus,0,sizeof(keystatus)); }

static int osdcmd_restartvid(const osdfuncparm_t *parm)
{
    extern int qsetmode;

    UNREFERENCED_PARAMETER(parm);

    if (qsetmode != 200) return OSDCMD_OK;

    resetvideomode();
    if (setgamemode(fullscreen,xdim,ydim,bpp))
        OSD_Printf("restartvid: Reset failed...\n");

    return OSDCMD_OK;
}

static int osdcmd_vidmode(const osdfuncparm_t *parm)
{
    int newx = xdim, newy = ydim, newbpp = bpp, newfullscreen = fullscreen;
    extern int qsetmode;

    switch (parm->numparms)
    {
    case 1:	// bpp switch
        newbpp = Batol(parm->parms[0]);
        break;
    case 4:	// fs, res, bpp switch
        newfullscreen = (Batol(parm->parms[3]) != 0);
    case 3:	// res & bpp switch
        newbpp = Batol(parm->parms[2]);
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
        clearbuf((char *)(frameplace + (ydim16*bytesperline)), (bytesperline*STATUS2DSIZ) >> 2, 0x00000000l);
        clearbuf((char *)frameplace, (ydim16*bytesperline) >> 2, 0L);

        ydim16 = ydim;
        drawline16(0,ydim-STATUS2DSIZ,xdim-1,ydim-STATUS2DSIZ,1);
        /*        drawline16(0,ydim-1,xdim-1,ydim-1,1);
                drawline16(0,ydim-STATUS2DSIZ,0,ydim-1,1);
                drawline16(xdim-1,ydim-STATUS2DSIZ,xdim-1,ydim-1,1);
                drawline16(0,ydim-STATUS2DSIZ+24,xdim-1,ydim-STATUS2DSIZ+24,1);
                drawline16(192-24,ydim-STATUS2DSIZ,192-24,ydim-STATUS2DSIZ+24,1);
                drawline16(0,ydim-1-20,xdim-1,ydim-1-20,1);
                drawline16(256,ydim-1-20,256,ydim-1,1); */
        ydim16 = ydim-STATUS2DSIZ;
        enddrawing();	//}}}
        return OSDCMD_OK;
    }

    if (setgamemode(newfullscreen,newx,newy,newbpp))
        OSD_Printf("vidmode: Mode change failed!\n");
    xdimgame = newx; ydimgame = newy; bppgame = newbpp; fullscreen = newfullscreen;
    return OSDCMD_OK;
}

extern int startwin_run(void);

extern char *defsfilename;	// set in bstub.c

int app_main(int argc, const char **argv)
{
    char quitflag, cmdsetup = 0;
    int i, j, k;

    pathsearchmode = 1;		// unrestrict findfrompath so that full access to the filesystem can be had

#ifdef USE_OPENGL
    OSD_RegisterFunction("restartvid","restartvid: reinitialize the video mode",osdcmd_restartvid);
    OSD_RegisterFunction("vidmode","vidmode <xdim> <ydim> <bpp> <fullscreen>: immediately change the video mode",osdcmd_vidmode);
#endif

    wm_setapptitle("Mapster32");

    if ((i = ExtPreInit(argc,argv)) < 0) return -1;

#ifdef RENDERTYPEWIN
    backgroundidle = 1;
#endif

    editstatus = 1;
    boardfilename[0] = 0;
    for (i=1; i<argc; i++)
    {
        if (argv[i][0] == '-')
        {
            if (!strcmp(argv[i], "-setup")) cmdsetup = 1;
            else if (!strcmp(argv[i], "-help") || !strcmp(argv[i], "--help") || !strcmp(argv[i], "-?"))
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
                wm_msgbox("Mapster32",s);
#else
                puts(s);
#endif
                return 0;
            }
            continue;
        }
        if (!boardfilename[0])
        {
            strncpy(boardfilename, argv[i], BMAX_PATH-4-1);
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
        if (quitevent || !startwin_run()) return -1;
    }
#endif

    if (initinput()) return -1;
    // if (option[3] != 0) moustat =
    initmouse();

    inittimer(TIMERINTSPERSECOND);
    installusertimercallback(keytimerstuff);

    loadpics("tiles000.art",1048576*16);
    loadnames();

    Bstrcpy(kensig,"Uses BUILD technology by Ken Silverman");
    initcrc();

    if (!loaddefinitionsfile(defsfilename)) initprintf("Definitions file loaded.\n");

    if (setgamemode(fullscreen,xdimgame,ydimgame,bppgame) < 0)
    {
        ExtUnInit();
        uninitengine();
        Bprintf("%d * %d not supported in this graphics mode\n",xdim,ydim);
        exit(0);
    }
    setbrightness(brightness,palette,0);

    k = 0;
    for (i=0;i<256;i++)
    {
        j = ((int)palette[i*3])+((int)palette[i*3+1])+((int)palette[i*3+2]);
        if (j > k) { k = j; whitecol = i; }
    }

    for (i=0;i<MAXSECTORS;i++) sector[i].extra = -1;
    for (i=0;i<MAXWALLS;i++) wall[i].extra = -1;
    for (i=0;i<MAXSPRITES;i++) sprite[i].extra = -1;

    ExtPreLoadMap();
    i = loadboard(boardfilename,(!pathsearchmode&&grponlymode?2:0),&posx,&posy,&posz,&ang,&cursectnum);
    loadmhk();
    if (i == -2) i = loadoldboard(boardfilename,(!pathsearchmode&&grponlymode?2:0),&posx,&posy,&posz,&ang,&cursectnum);
    if (i < 0)
    {
        initspritelists();
        posx = 32768;
        posy = 32768;
        posz = 0;
        ang = 1536;
        numsectors = 0;
        numwalls = 0;
        cursectnum = -1;
    }
    else
    {
        ExtLoadMap(boardfilename);
    }

    updatenumsprites();

    startposx = posx;
    startposy = posy;
    startposz = posz;
    startang = ang;
    startsectnum = cursectnum; // TX 20050225: moved to loadboard

    totalclock = 0;

    updatesector(posx,posy,&cursectnum);

    if (cursectnum == -1)
    {
        double gamma = vid_gamma;
        vid_gamma = 1.0;
        setbrightness(0,palette,0);
        overheadeditor();
        keystatus[buildkeys[BK_MODE2D_3D]] = 0;
        vid_gamma = gamma;
        setbrightness(brightness,palette,0);
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

        drawrooms(posx,posy,posz,ang,horiz,cursectnum);
#ifdef SUPERBUILD
        ExtAnalyzeSprites();
#endif
        drawmasks();

        ExtCheckKeys();

        nextpage();
        synctics = totalclock-lockclock;
        lockclock += synctics;

        if (keystatus[1])
        {
            keystatus[1] = 0;
            begindrawing();	//{{{
            printext256(0,0,whitecol,0,"Are you sure you want to quit?",0);
            enddrawing();	//}}}

            showframe(1);
            synctics = totalclock-lockclock;
            lockclock += synctics;

            while ((keystatus[1]|keystatus[0x1c]|keystatus[0x39]|keystatus[0x31]) == 0)
            {
                if (handleevents())
                {
                    if (quitevent)
                    {
                        quitflag = 1;
                        break;
                    }
                }
                idle();

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
        begindrawing();	//{{{
        printext256(0,8,whitecol,0,"Save changes?",0);
        showframe(1);	//}}}

        while ((keystatus[1]|keystatus[0x1c]|keystatus[0x39]|keystatus[0x31]|keystatus[0x2e]) == 0)
        {
            if (handleevents()) { if (quitevent) break;	} // like saying no
            idle();

            if (keystatus[0x15] || keystatus[0x1c]) // Y or ENTER
            {
                char *f;
                keystatus[0x15] = 0;
                keystatus[0x1c] = 0;
                fixspritesectors();
                updatesector(startposx,startposy,&startsectnum);
                ExtPreSaveMap();
                if (pathsearchmode) f = boardfilename;
                else
                {
                    // virtual filesystem mode can't save to directories so drop the file into
                    // the current directory
                    f = strrchr(boardfilename, '/');
                    if (!f) f = boardfilename; else f++;
                }
                saveboard(f,&startposx,&startposy,&startposz,&startang,&startsectnum);
                ExtSaveMap(f);
                break;
            }
        }
        while (keystatus[1]||keystatus[0x2e])
        {
            keystatus[1] = 0;
            keystatus[0x2e] = 0;
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
    int i;

    for (i=1;i<=4;i++)
    {
        plotpixel(searchx+i,searchy,whitecol);
        plotpixel(searchx-i,searchy,whitecol);
        plotpixel(searchx,searchy-i,whitecol);
        plotpixel(searchx,searchy+i,whitecol);
    }
}*/

int mhk=0;
void loadmhk()
{
    char *p;char levname[BMAX_PATH];

    if (!mhk)return;
    strcpy(levname, boardfilename);
    p = Bstrrchr(levname,'.');
    if (!p) strcat(levname,".mhk");
    else
    {
        p[1]='m';
        p[2]='h';
        p[3]='k';
        p[4]=0;
    }
    if (!loadmaphack(levname))initprintf("Loaded map hack file '%s'\n",levname);else mhk=2;
}

void editinput(void)
{
//    char smooshyalign, repeatpanalign, buffer[80];
//    short sectnum, nextsectnum, startwall, endwall, dasector, daang;
    int mousz, bstatus;
    int i, j, k, /*cnt,*/ tempint=0, doubvel/*, changedir, wallfind[2], daz[2]*/;
    int /*dashade[2],*/ goalz, xvect, yvect,/*PK*/ zvect, hiz, loz;
    short hitsect, hitwall, hitsprite;
    int hitx, hity, hitz, dax, day, hihit, lohit;

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
            screencapture("captxxxx.tga",keystatus[0x2a]|keystatus[0x36]);
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
            ld = ldiv((int)(mousx), (1<<16)/(msens*0.5f)); mousx = ld.quot; mousexsurp = ld.rem;
            ld = ldiv((int)(mousy), (1<<16)/(msens*0.25f)); mousy = ld.quot; mouseysurp = ld.rem;
        }
        else
        {
            ld = ldiv((int)(mousx), (1<<16)/msens); mousx = ld.quot; mousexsurp = ld.rem;
            ld = ldiv((int)(mousy), (1<<16)/msens); mousy = ld.quot; mouseysurp = ld.rem;
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
            xvect = -((mousy*(int)sintable[(ang+2560)&2047])<<(3+pk_uedaccel));
            yvect = -((mousy*(int)sintable[(ang+2048)&2047])<<(3+pk_uedaccel));

            if (noclip)
            {
                posx += xvect>>14;
                posy += yvect>>14;
                updatesector(posx,posy,&cursectnum);
            }
            else clipmove(&posx,&posy,&posz,&cursectnum,xvect,yvect,128L,4L<<8,4L<<8,CLIPMASK0);
        }
        else if (!mlook && (bstatus&2) && !(bstatus&(1|4)))
        {
            mlook=2;
        }
        else if ((bstatus&1) && (bstatus&2) && !(bstatus&4))
        {
            zmode = 2;
            xvect = -((mousx*(int)sintable[(ang+2048)&2047])<<pk_uedaccel);
            yvect = -((mousx*(int)sintable[(ang+1536)&2047])<<pk_uedaccel);
            posz += mousy<<(4+pk_uedaccel);
            if (noclip)
            {
                posx += xvect>>14;
                posy += yvect>>14;
                updatesectorz(posx,posy,posz,&cursectnum);
            }
            else clipmove(&posx,&posy,&posz,&cursectnum,xvect,yvect,128L,4L<<8,4L<<8,CLIPMASK0);
        }
        else if (bstatus&4)
        {
            zmode = 2;

            // horiz-100 of 200 is viewing at 326.4 build angle units (=atan(200/128)) upward
            tempint = getangle(128, horiz-100);

            xvect = -((mousy*
                       ((int)sintable[(ang+2560)&2047]>>6)*
                       ((int)sintable[(tempint+512)&2047])>>6)
                      <<pk_uedaccel);
            yvect = -((mousy*
                       ((int)sintable[(ang+2048)&2047]>>6)*
                       ((int)sintable[(tempint+512)&2047])>>6)
                      <<pk_uedaccel);

            zvect = mousy*(((int)sintable[(tempint+2048)&2047])>>(10-pk_uedaccel));

            posz += zvect;
            if (noclip)
            {
                posx += xvect>>16;
                posy += yvect>>16;
                updatesectorz(posx,posy,posz,&cursectnum);
            }
            else clipmove(&posx,&posy,&posz,&cursectnum,xvect>>2,yvect>>2,128L,4L<<8,4L<<8,CLIPMASK0);
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
            if (horiz > 299)
                horiz = 299;
            if (horiz < -99)
                horiz = -99;
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
            if (searchx < 12) searchx = 12;
            if (searchy < 12) searchy = 12;
            if (searchx > xdim-13) searchx = xdim-13;
            if (searchy > ydim-13) searchy = ydim-13;
        }
    }

//    showmouse();

//    if (keystatus[0x3b] > 0) posx--;
//    if (keystatus[0x3c] > 0) posx++;
//    if (keystatus[0x3d] > 0) posy--;
//    if (keystatus[0x3e] > 0) posy++;
//    if (keystatus[0x43] > 0) ang--;
//    if (keystatus[0x44] > 0) ang++;

    if (keystatus[0x43])
    {
        mhk=1;
        loadmhk();
        keystatus[0x43] = 0;
    }
    if (keystatus[0x44])
    {
        memset(spriteext, 0, sizeof(spriteexttype) * MAXSPRITES);
        memset(spritesmooth, 0, sizeof(spritesmooth));
        mhk=0;
        initprintf("Maphacks disabled\n");
        keystatus[0x44] = 0;
    }
    begindrawing();     //{{{
    if (mhk)printext256(0,16,whitecol,0,(mhk==1)?"Maphacks ON":"Maphacks ON (not found)",0);
    enddrawing();       //}}}

    if (angvel != 0)          //ang += angvel * constant
    {
        //ENGINE calculates angvel for you
        doubvel = synctics;
        if (keystatus[buildkeys[BK_RUN]])  //Lt. shift makes turn velocity 50% faster
            doubvel += (synctics>>1);
        ang += ((angvel*doubvel)>>4);
        ang = (ang+2048)&2047;
    }
    if ((vel|svel) != 0)
    {
        doubvel = synctics;
        if (keystatus[buildkeys[BK_RUN]])     //Lt. shift doubles forward velocity
            doubvel += synctics;
        xvect = 0, yvect = 0;
        if (vel != 0)
        {
            xvect += ((vel*doubvel*(int)sintable[(ang+2560)&2047])>>3);
            yvect += ((vel*doubvel*(int)sintable[(ang+2048)&2047])>>3);
        }
        if (svel != 0)
        {
            xvect += ((svel*doubvel*(int)sintable[(ang+2048)&2047])>>3);
            yvect += ((svel*doubvel*(int)sintable[(ang+1536)&2047])>>3);
        }
        if (noclip)
        {
            posx += xvect>>14;
            posy += yvect>>14;
            updatesector(posx,posy,&cursectnum);
        }
        else clipmove(&posx,&posy,&posz,&cursectnum,xvect,yvect,128L,4L<<8,4L<<8,CLIPMASK0);
    }
    getzrange(posx,posy,posz,cursectnum,&hiz,&hihit,&loz,&lohit,128L,CLIPMASK0);

    /*    if (keystatus[0x3a] > 0)
        {
            zmode++;
            if (zmode == 3) zmode = 0;
            if (zmode == 1) zlock = (loz-posz)&0xfffffc00;
            keystatus[0x3a] = 0;
    	}*/

    if (zmode == 0)
    {
        goalz = loz-(kensplayerheight<<8);   //playerheight pixels above floor
        if (goalz < hiz+(16<<8))   //ceiling&floor too close
            goalz = ((loz+hiz)>>1);
        goalz += mousz;
        if (keystatus[buildkeys[BK_MOVEUP]])							//A (stand high)
        {
            if (keystatus[0x1d]|keystatus[0x9d])
                horiz = max(-100,horiz-((keystatus[buildkeys[BK_RUN]]+1)*synctics*2));
            else
            {
                goalz -= (16<<8);
                if (keystatus[buildkeys[BK_RUN]])    //Either shift key
                    goalz -= (24<<8);
            }
        }
        if (keystatus[buildkeys[BK_MOVEDOWN]])  						  //Z (stand low)
        {
            if (keystatus[0x1d]|keystatus[0x9d])
                horiz = min(300,horiz+((keystatus[buildkeys[BK_RUN]]+1)*synctics*2));
            else
            {
                goalz += (12<<8);
                if (keystatus[buildkeys[BK_RUN]])    //Either shift key
                    goalz += (12<<8);
            }
        }

        if (goalz != posz)
        {
            if (posz < goalz) hvel += 64;
            if (posz > goalz) hvel = ((goalz-posz)>>3);

            posz += hvel;
            if (posz > loz-(4<<8)) posz = loz-(4<<8), hvel = 0;
            if (posz < hiz+(4<<8)) posz = hiz+(4<<8), hvel = 0;
        }
    }
    else
    {
        goalz = posz;
        if (keystatus[buildkeys[BK_MOVEUP]])							//A
        {
            if (keystatus[0x1d]|keystatus[0x9d])
            {
                horiz = max(-100,horiz-((keystatus[buildkeys[BK_RUN]]+1)*synctics*2));
            }
            else
            {
                if (zmode != 1)
                    goalz -= (8<<8);
                else
                {
                    zlock += (4<<8);
                    keystatus[buildkeys[BK_MOVEUP]] = 0;
                }
            }
        }
        if (keystatus[buildkeys[BK_MOVEDOWN]])  						  //Z (stand low)
        {
            if (keystatus[0x1d]|keystatus[0x9d])
            {
                horiz = min(300,horiz+((keystatus[buildkeys[BK_RUN]]+1)*synctics*2));
            }
            else
            {
                if (zmode != 1)
                    goalz += (8<<8);
                else if (zlock > 0)
                {
                    zlock -= (4<<8);
                    keystatus[buildkeys[BK_MOVEDOWN]] = 0;
                }
            }
        }

        if (!noclip)
        {
            if (goalz < hiz+(4<<8)) goalz = hiz+(4<<8);
            if (goalz > loz-(4<<8)) goalz = loz-(4<<8);
        }
        if (zmode == 1) goalz = loz-zlock;
        if (!noclip && (goalz < hiz+(4<<8))) goalz = ((loz+hiz)>>1);  //ceiling&floor too close
        if (zmode == 1) posz = goalz;

        if (goalz != posz)
        {
            //if (posz < goalz) hvel += (32<<keystatus[buildkeys[BK_RUN]]);
            //if (posz > goalz) hvel -= (32<<keystatus[buildkeys[BK_RUN]]);
            if (posz < goalz) hvel = ((synctics* 192)<<keystatus[buildkeys[BK_RUN]]);
            if (posz > goalz) hvel = ((synctics*-192)<<keystatus[buildkeys[BK_RUN]]);

            posz += hvel;

            if (!noclip)
            {
                if (posz > loz-(4<<8)) posz = loz-(4<<8), hvel = 0;
                if (posz < hiz+(4<<8)) posz = hiz+(4<<8), hvel = 0;
            }
        }
        else
            hvel = 0;
    }

    searchit = 2;
    if (searchstat >= 0)
    {
        if ((bstatus&(1|2|4)) > 0)
            searchit = 0;

        if (keystatus[0x1f])  //S (insert sprite) (3D)
        {
            dax = 16384;
            day = divscale14(searchx-(xdim>>1),xdim>>1);
            rotatepoint(0,0,dax,day,ang,&dax,&day);

            hitscan(posx,posy,posz,cursectnum,               //Start position
                    dax,day,(scale(searchy,200,ydim)-horiz)*2000, //vector of 3D ang
                    &hitsect,&hitwall,&hitsprite,&hitx,&hity,&hitz,CLIPMASK1);

            if (hitsect >= 0)
            {
                dax = hitx;
                day = hity;
                if ((gridlock > 0) && (grid > 0))
                {
                    if ((searchstat == 0) || (searchstat == 4))
                    {
                        hitz = (hitz&0xfffffc00);
                    }
                    else
                    {
                        dax = ((dax+(1024>>grid))&(0xffffffff<<(11-grid)));
                        day = ((day+(1024>>grid))&(0xffffffff<<(11-grid)));
                    }
                }

                i = insertsprite(hitsect,0);
                sprite[i].x = dax, sprite[i].y = day;
                sprite[i].cstat = defaultspritecstat;
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

                for (k=0;k<MAXTILES;k++)
                    localartfreq[k] = 0;
                for (k=0;k<MAXSPRITES;k++)
                    if (sprite[k].statnum < MAXSTATUS)
                        localartfreq[sprite[k].picnum]++;
                j = 0;
                for (k=0;k<MAXTILES;k++)
                    if (localartfreq[k] > localartfreq[j])
                        j = k;
                if (localartfreq[j] > 0)
                    sprite[i].picnum = j;
                else
                    sprite[i].picnum = 0;

                if (somethingintab == 3)
                {
                    sprite[i].picnum = temppicnum;
                    if ((tilesizx[temppicnum] <= 0) || (tilesizy[temppicnum] <= 0))
                    {
                        j = 0;
                        for (k=0;k<MAXTILES;k++)
                            if ((tilesizx[k] > 0) && (tilesizy[k] > 0))
                            {
                                j = k;
                                break;
                            }
                        sprite[i].picnum = j;
                    }
                    sprite[i].shade = tempshade;
                    sprite[i].pal = temppal;
                    sprite[i].xrepeat = tempxrepeat;
                    sprite[i].yrepeat = tempyrepeat;
                    if (sprite[i].xrepeat < 1) sprite[i].xrepeat = 1;
                    if (sprite[i].yrepeat < 1) sprite[i].yrepeat = 1;
                    sprite[i].cstat = tempcstat;
                }

                j = ((tilesizy[sprite[i].picnum]*sprite[i].yrepeat)<<1);
                if ((sprite[i].cstat&128) == 0)
                    sprite[i].z = min(max(hitz,getceilzofslope(hitsect,hitx,hity)+(j<<1)),getflorzofslope(hitsect,hitx,hity));
                else
                    sprite[i].z = min(max(hitz,getceilzofslope(hitsect,hitx,hity)+j),getflorzofslope(hitsect,hitx,hity)-j);

                if ((searchstat == 0) || (searchstat == 4))
                {
                    sprite[i].cstat = (sprite[i].cstat&~48)|(16+64);
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
                else
                {
                    if (tilesizy[sprite[i].picnum] >= 32) sprite[i].cstat |= 1;
                }

                updatenumsprites();
                asksave = 1;
            }

            keystatus[0x1f] = 0;
        }
        /*  	  if (keystatus[0xd3] > 0)
                {
                    if (searchstat == 3)
                    {
                        deletesprite(searchwall);
                        updatenumsprites();
                        asksave = 1;
                    }
                    keystatus[0xd3] = 0;
        		}*/

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
        double gamma = vid_gamma;
        vid_gamma = 1.0;
        setbrightness(0,palette,0);
        overheadeditor();
        keystatus[buildkeys[BK_MODE2D_3D]] = 0;
        vid_gamma = gamma;
        setbrightness(brightness,palette,0);
    }
}

char changechar(char dachar, int dadir, char smooshyalign, char boundcheck)
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
#if 0
/*
int gettile(int tilenum)
{
    char snotbuf[80], ch;
    int i, j, k, otilenum, topleft, gap, temp, tempint;
    int xtiles, ytiles, tottiles;

    if (tilenum < 0) tilenum = 0;

    xtiles = (xdim>>6); ytiles = (ydim>>6); tottiles = xtiles*ytiles;
    otilenum = tilenum;

    keystatus[0x2f] = 0;
    for (i=0;i<MAXTILES;i++)
    {
        localartfreq[i] = 0;
        localartlookup[i] = i;
    }
    if ((searchstat == 1) || (searchstat == 2))
        for (i=0;i<numsectors;i++)
        {
            localartfreq[sector[i].ceilingpicnum]++;
            localartfreq[sector[i].floorpicnum]++;
        }
    if (searchstat == 0)
        for (i=0;i<numwalls;i++)
            localartfreq[wall[i].picnum]++;
    if (searchstat == 4)
        for (i=0;i<numwalls;i++)
            localartfreq[wall[i].overpicnum]++;
    if (searchstat == 3)
        for (i=0;i<MAXSPRITES;i++)
            if (sprite[i].statnum < MAXSTATUS)
                localartfreq[sprite[i].picnum]++;
    gap = (MAXTILES>>1);
    do
    {
        for (i=0;i<MAXTILES-gap;i++)
        {
            temp = i;
            while ((localartfreq[temp] < localartfreq[temp+gap]) && (temp >= 0))
            {
                tempint = localartfreq[temp];
                localartfreq[temp] = localartfreq[temp+gap];
                localartfreq[temp+gap] = tempint;
                tempint = localartlookup[temp];
                localartlookup[temp] = localartlookup[temp+gap];
                localartlookup[temp+gap] = tempint;

                if (tilenum == temp)
                    tilenum = temp+gap;
                else if (tilenum == temp+gap)
                    tilenum = temp;

                temp -= gap;
            }
        }
        gap >>= 1;
    }
    while (gap > 0);
    localartlookupnum = 0;
    while (localartfreq[localartlookupnum] > 0)
        localartlookupnum++;

    if (localartfreq[0] == 0)
    {
        tilenum = otilenum;
        localartlookupnum = MAXTILES;
        for (i=0;i<MAXTILES;i++)
            localartlookup[i] = i;
    }

    topleft = ((tilenum/(xtiles<<gettilezoom))*(xtiles<<gettilezoom))-(xtiles<<gettilezoom);
    if (topleft < 0) topleft = 0;
    if (topleft > MAXTILES-(tottiles<<(gettilezoom<<1))) topleft = MAXTILES-(tottiles<<(gettilezoom<<1));
    while ((keystatus[0x1c]|keystatus[1]) == 0)
    {
        drawtilescreen(topleft,tilenum);

        if (handleevents())
        {
            if (quitevent) quitevent = 0;
        }
        idle();

        synctics = totalclock-lockclock;
        lockclock += synctics;

        if ((keystatus[0x37] > 0) && (gettilezoom < 2))
        {
            gettilezoom++;
            topleft = ((tilenum/(xtiles<<gettilezoom))*(xtiles<<gettilezoom))-(xtiles<<gettilezoom);
            if (topleft < 0) topleft = 0;
            if (topleft > MAXTILES-(tottiles<<(gettilezoom<<1))) topleft = MAXTILES-(tottiles<<(gettilezoom<<1));
            keystatus[0x37] = 0;
        }
        if ((keystatus[0xb5] > 0) && (gettilezoom > 0))
        {
            gettilezoom--;
            topleft = ((tilenum/(xtiles<<gettilezoom))*(xtiles<<gettilezoom))-(xtiles<<gettilezoom);
            if (topleft < 0) topleft = 0;
            if (topleft > MAXTILES-(tottiles<<(gettilezoom<<1))) topleft = MAXTILES-(tottiles<<(gettilezoom<<1));
            keystatus[0xb5] = 0;
        }
        if ((keystatus[0xcb] > 0) && (tilenum > 0))
            tilenum--, keystatus[0xcb] = 0;
        if ((keystatus[0xcd] > 0) && (tilenum < MAXTILES-1))
            tilenum++, keystatus[0xcd] = 0;
        if ((keystatus[0xc8] > 0) && (tilenum >= (xtiles<<gettilezoom)))
            tilenum-=(xtiles<<gettilezoom), keystatus[0xc8] = 0;
        if ((keystatus[0xd0] > 0) && (tilenum < MAXTILES-(xtiles<<gettilezoom)))
            tilenum+=(xtiles<<gettilezoom), keystatus[0xd0] = 0;
        if ((keystatus[0xc9] > 0) && (tilenum >= (xtiles<<gettilezoom)))
        {
            tilenum-=(tottiles<<(gettilezoom<<1));
            if (tilenum < 0) tilenum = 0;
            keystatus[0xc9] = 0;
        }
        if ((keystatus[0xd1] > 0) && (tilenum < MAXTILES-(xtiles<<gettilezoom)))
        {
            tilenum+=(tottiles<<(gettilezoom<<1));
            if (tilenum >= MAXTILES) tilenum = MAXTILES-1;
            keystatus[0xd1] = 0;
        }
        if (keystatus[0x2f] > 0)   //V
        {
            keystatus[0x2f] = 0;
            if (tilenum < localartlookupnum)
                tilenum = localartlookup[tilenum];
            else
                tilenum = 0;
            localartlookupnum = MAXTILES;
            for (i=0;i<MAXTILES;i++)
                localartlookup[i] = i;
        }
        if (keystatus[0x22] > 0)       //G (goto)
        {
            if (tilenum < localartlookupnum)         //Automatically press 'V'
                tilenum = localartlookup[tilenum];
            else
                tilenum = 0;
            localartlookupnum = MAXTILES;
            for (i=0;i<MAXTILES;i++)
                localartlookup[i] = i;

            keystatus[0x22] = 0;
            bflushchars();

            j = tilenum;
            while (keystatus[1] == 0)
            {
                if (handleevents())
                {
                    if (quitevent) quitevent = 0;
                }
                idle();

                ch = bgetchar();

                //drawtilescreen(topleft,tilenum);
                Bsprintf(snotbuf,"Goto tile: %d_ ",j);
                printext256(0,0,whitecol,0,snotbuf,0);
                showframe(1);

                if (ch >= '0' && ch <= '9')
                {
                    i = (j*10)+(ch-'0');
                    if (i < MAXTILES) j = i;
                }
                else if (ch == 8)
                {
                    j /= 10;
                }
                else if (ch == 13)
                {
                    tilenum = j;
                    break;
                }
            }
            clearkeys();
        }
        while (tilenum < topleft) topleft -= (xtiles<<gettilezoom);
        while (tilenum >= topleft+(tottiles<<(gettilezoom<<1))) topleft += (xtiles<<gettilezoom);
        if (topleft < 0) topleft = 0;
        if (topleft > MAXTILES-(tottiles<<(gettilezoom<<1))) topleft = MAXTILES-(tottiles<<(gettilezoom<<1));
    }

    if (keystatus[0x1c] == 0)
    {
        tilenum = otilenum;
    }
    else
    {
        if (tilenum < localartlookupnum)
        {
            tilenum = localartlookup[tilenum];
            if ((tilesizx[tilenum] == 0) || (tilesizy[tilenum] == 0))
                tilenum = otilenum;
        }
        else
            tilenum = otilenum;
    }
    keystatus[0x1] = 0;
    keystatus[0x1c] = 0;
    return(tilenum);
}

int drawtilescreen(int pictopleft, int picbox)
{
    int i, j, vidpos, vidpos2, dat, wallnum, xdime, ydime, cnt, pinc;
    int dax, day, scaledown, xtiles, ytiles, tottiles;
    char *picptr, snotbuf[80];

    xtiles = (xdim>>6); ytiles = (ydim>>6); tottiles = xtiles*ytiles;

    begindrawing(); //{{{

    setpolymost2dview();	// JBF 20040205: set to 2d rendering

    pinc = ylookup[1];
    clearview(0L);
    for (cnt=0;cnt<(tottiles<<(gettilezoom<<1));cnt++)        //draw the 5*3 grid of tiles
    {
        wallnum = cnt+pictopleft;
        if (wallnum < localartlookupnum)
        {
            wallnum = localartlookup[wallnum];
            if ((tilesizx[wallnum] != 0) && (tilesizy[wallnum] != 0))
            {
                if (waloff[wallnum] == 0) loadtile(wallnum);
                picptr = (char *)(waloff[wallnum]);
                xdime = tilesizx[wallnum];
                ydime = tilesizy[wallnum];

                dax = ((cnt%(xtiles<<gettilezoom))<<(6-gettilezoom));
                day = ((cnt/(xtiles<<gettilezoom))<<(6-gettilezoom));
                if (polymost_drawtilescreen(dax, day, wallnum, 64>>gettilezoom, 0))
                {
                    vidpos = ylookup[day]+dax+frameplace;
                    if ((xdime <= (64>>gettilezoom)) && (ydime <= (64>>gettilezoom)))
                    {
                        for (i=0;i<xdime;i++)
                        {
                            vidpos2 = vidpos+i;
                            for (j=0;j<ydime;j++)
                            {
                                *(char *)vidpos2 = *picptr++;
                                vidpos2 += pinc;
                            }
                        }
                    }
                    else                          //if 1 dimension > 64
                    {
                        if (xdime > ydime)
                            scaledown = ((xdime+(63>>gettilezoom))>>(6-gettilezoom));
                        else
                            scaledown = ((ydime+(63>>gettilezoom))>>(6-gettilezoom));

                        for (i=0;i<xdime;i+=scaledown)
                        {
                            if (waloff[wallnum] == 0) loadtile(wallnum);
                            picptr = (char *)(waloff[wallnum]) + ydime*i;
                            vidpos2 = vidpos;
                            for (j=0;j<ydime;j+=scaledown)
                            {
                                *(char *)vidpos2 = *picptr;
                                picptr += scaledown;
                                vidpos2 += pinc;
                            }
                            vidpos++;
                        }
                    }
                }
                if (localartlookupnum < MAXTILES)
                {
                    dax = ((cnt%(xtiles<<gettilezoom))<<(6-gettilezoom));
                    day = ((cnt/(xtiles<<gettilezoom))<<(6-gettilezoom));
                    Bsprintf(snotbuf,"%d",localartfreq[cnt+pictopleft]);
                    printext256(dax,day,whitecol,-1,snotbuf,1);
                }
            }
        }
    }

    cnt = picbox-pictopleft;    //draw open white box
    dax = ((cnt%(xtiles<<gettilezoom))<<(6-gettilezoom));
    day = ((cnt/(xtiles<<gettilezoom))<<(6-gettilezoom));

    for (i=0;i<(64>>gettilezoom);i++)
    {
        plotpixel(dax+i,day,whitecol);
        plotpixel(dax+i,day+(63>>gettilezoom),whitecol);
        plotpixel(dax,day+i,whitecol);
        plotpixel(dax+(63>>gettilezoom),day+i,whitecol);
    }

    i = localartlookup[picbox];
    Bsprintf(snotbuf,"%d",i);
    printext256(0L,ydim-8,whitecol,-1,snotbuf,0);
    printext256(xdim-(Bstrlen(names[i])<<3),ydim-8,whitecol,-1,names[i],0);

    Bsprintf(snotbuf,"%dx%d",tilesizx[i],tilesizy[i]);
    printext256(xdim>>2,ydim-8,whitecol,-1,snotbuf,0);

    enddrawing();	//}}}
    showframe(1);

    return(0);
}
*/
#endif

void overheadeditor(void)
{
    char buffer[80], *dabuffer, ch;
    int i, j, k, m=0, mousxplc, mousyplc, firstx=0, firsty=0, oposz, col;
    int tempint, tempint1, tempint2, doubvel;
    int startwall=0, endwall, dax, day, daz, x1, y1, x2, y2, x3, y3, x4, y4;
    int highlightx1, highlighty1, highlightx2, highlighty2, xvect, yvect;
    short pag, suckwall=0, sucksect, newnumwalls, newnumsectors, split=0, bad;
    short splitsect=0, danumwalls, secondstartwall, joinsector[2], joinsectnum;
    short splitstartwall=0, splitendwall, loopnum;
    int mousx, mousy, bstatus;
    int centerx, centery, circlerad;
    short circlepoints, circleang1, circleang2, circleangdir;
    int sectorhighlightx=0, sectorhighlighty=0;
    short cursectorhighlight, sectorhighlightstat;
    short hitsect, hitwall, hitsprite;
    int hitx, hity, hitz;
    walltype *wal;
    int prefixarg = 0;

    //qsetmode640480();
    qsetmodeany(xdim2d,ydim2d);
    xdim2d = xdim;
    ydim2d = ydim;

    osearchx = searchx;
    osearchy = searchy;

    searchx = scale(searchx,xdim2d,xdimgame);
    searchy = scale(searchy,ydim2d-STATUS2DSIZ,ydimgame);
    oposz = posz;

    begindrawing();	//{{{
    clearbuf((char *)(frameplace + (ydim16*bytesperline)), (bytesperline*STATUS2DSIZ) >> 2, 0x00000000l);
    clearbuf((char *)frameplace, (ydim16*bytesperline) >> 2, 0L);

    ydim16 = ydim;
    drawline16(0,ydim-STATUS2DSIZ,xdim-1,ydim-STATUS2DSIZ,1);
    /*    drawline16(0,ydim-1,xdim-1,ydim-1,1);
        drawline16(0,ydim-STATUS2DSIZ,0,ydim-1,1);
        drawline16(xdim-1,ydim-STATUS2DSIZ,xdim-1,ydim-1,1);
        drawline16(0,ydim-STATUS2DSIZ+24,xdim-1,ydim-STATUS2DSIZ+24,1);
        drawline16(192-24,ydim-STATUS2DSIZ,192-24,ydim-STATUS2DSIZ+24,1); */
    if (totalclock < 120*5)
    {
        printmessage16("Press F1 for help");
        printext16(8L,ydim-STATUS2DSIZ+32L,9,-1,kensig,0);
    }

    //  printmessage16("Version: "VERSION);
//    drawline16(0,ydim-1-20,xdim-1,ydim-1-20,1);
//    drawline16(256,ydim-1-20,256,ydim-1,1);
    ydim16 = ydim-STATUS2DSIZ;
    enddrawing();	//}}}

    pag = 0;
    highlightcnt = -1;
    cursectorhighlight = -1;
    lastpm16time = -1;

    //White out all bordering lines of grab that are
    //not highlighted on both sides
    for (i=highlightsectorcnt-1;i>=0;i--)
    {
        startwall = sector[highlightsector[i]].wallptr;
        endwall = startwall + sector[highlightsector[i]].wallnum;
        for (j=startwall;j<endwall;j++)
        {
            if (wall[j].nextwall >= 0)
            {
                for (k=highlightsectorcnt-1;k>=0;k--)
                    if (highlightsector[k] == wall[j].nextsector)
                        break;
                if (k < 0)
                {
                    wall[wall[j].nextwall].nextwall = -1;
                    wall[wall[j].nextwall].nextsector = -1;
                    wall[j].nextwall = -1;
                    wall[j].nextsector = -1;
                }
            }
        }
    }

    for (i=0;i<(MAXWALLS>>3);i++)  //Clear all highlights
        show2dwall[i] = 0;
    for (i=0;i<(MAXSPRITES>>3);i++)
        show2dsprite[i] = 0;

    sectorhighlightstat = -1;
    newnumwalls = -1;
    joinsector[0] = -1;
    circlewall = -1;
    circlepoints = 7;
    bstatus = 0;
    keystatus[buildkeys[BK_MODE2D_3D]] = 0;
    while ((keystatus[buildkeys[BK_MODE2D_3D]]>>1) == 0)
    {
        if (handleevents())
        {
            if (quitevent)
            {
                keystatus[1] = 1;
                quitevent = 0;
            }
        }

        idle();
        OSD_DispatchQueued();

        oldmousebstatus = bstatus;
        getmousevalues(&mousx,&mousy,&bstatus);
        mousx = (mousx<<16)+mousexsurp;
        mousy = (mousy<<16)+mouseysurp;
        {
            ldiv_t ld;
            ld = ldiv((int)(mousx), (1<<16)); mousx = ld.quot; mousexsurp = ld.rem;
            ld = ldiv((int)(mousy), (1<<16)); mousy = ld.quot; mouseysurp = ld.rem;
        }
        searchx += mousx;
        searchy += mousy;
        if (searchx < 8) searchx = 8;
        if (searchx > xdim-8-1) searchx = xdim-8-1;
        if (searchy < 8) searchy = 8;
        if (searchy > ydim-8-1) searchy = ydim-8-1;

        /*      if (keystatus[0x3b] > 0) posx--, keystatus[0x3b] = 0;
        		if (keystatus[0x3c] > 0) posx++, keystatus[0x3c] = 0;
        		if (keystatus[0x3d] > 0) posy--, keystatus[0x3d] = 0;
        		if (keystatus[0x3e] > 0) posy++, keystatus[0x3e] = 0;
        		if (keystatus[0x43] > 0) ang--, keystatus[0x43] = 0;
                if (keystatus[0x44] > 0) ang++, keystatus[0x44] = 0; */
        if (angvel != 0)          //ang += angvel * constant
        {
            //ENGINE calculates angvel for you
            doubvel = synctics;
            if (keystatus[buildkeys[BK_RUN]])  //Lt. shift makes turn velocity 50% faster
                doubvel += (synctics>>1);
            ang += ((angvel*doubvel)>>4);
            ang = (ang+2048)&2047;
        }
        if ((vel|svel) != 0)
        {
            doubvel = synctics;
            if (keystatus[buildkeys[BK_RUN]])     //Lt. shift doubles forward velocity
                doubvel += synctics;
            xvect = 0, yvect = 0;
            if (vel != 0)
            {
                xvect += ((vel*doubvel*(int)sintable[(ang+2560)&2047])>>3);
                yvect += ((vel*doubvel*(int)sintable[(ang+2048)&2047])>>3);
            }
            if (svel != 0)
            {
                xvect += ((svel*doubvel*(int)sintable[(ang+2048)&2047])>>3);
                yvect += ((svel*doubvel*(int)sintable[(ang+1536)&2047])>>3);
            }
            if (noclip)
            {
                posx += xvect>>14;
                posy += yvect>>14;
                updatesector(posx,posy,&cursectnum);
            }
            else clipmove(&posx,&posy,&posz,&cursectnum,xvect,yvect,128L,4L<<8,4L<<8,CLIPMASK0);
        }

        getpoint(searchx,searchy,&mousxplc,&mousyplc);
        linehighlight = getlinehighlight(mousxplc, mousyplc, linehighlight);

        if (newnumwalls >= numwalls)
        {
            dax = mousxplc;
            day = mousyplc;
            adjustmark(&dax,&day,newnumwalls);
            wall[newnumwalls].x = dax;
            wall[newnumwalls].y = day;
        }

        ydim16 = ydim - STATUS2DSIZ;

        tempint = numwalls;
        numwalls = newnumwalls;
        if (numwalls < 0) numwalls = tempint;

        clear2dscreen();
        draw2dgrid(posx,posy,ang,zoom,grid);

        x2 = mulscale14(startposx-posx,zoom);          //Draw brown arrow (start)
        y2 = mulscale14(startposy-posy,zoom);
        if (((halfxdim16+x2) >= 2) && ((halfxdim16+x2) <= xdim-3))
            if (((midydim16+y2) >= 2) && ((midydim16+y2) <= ydim16-3))
            {
                x1 = mulscale11(sintable[(startang+2560)&2047],zoom) / 768;
                y1 = mulscale11(sintable[(startang+2048)&2047],zoom) / 768;
                begindrawing();	//{{{
                drawline16((halfxdim16+x2)+x1,(midydim16+y2)+y1,(halfxdim16+x2)-x1,(midydim16+y2)-y1,2);
                drawline16((halfxdim16+x2)+x1,(midydim16+y2)+y1,(halfxdim16+x2)+y1,(midydim16+y2)-x1,2);
                drawline16((halfxdim16+x2)+x1,(midydim16+y2)+y1,(halfxdim16+x2)-y1,(midydim16+y2)+x1,2);
                enddrawing();	//}}}
            }

        draw2dscreen(posx,posy,ang,zoom,grid);

        begindrawing();	//{{{
        if (showtags == 1)
        {
            if (zoom >= 768)
                for (i=0;i<numsectors;i++)
                {
                    dabuffer = (char *)ExtGetSectorCaption(i);
                    if (dabuffer[0] != 0)
                    {
                        dax = 0;   //Get average point of sector
                        day = 0;
                        startwall = sector[i].wallptr;
                        endwall = startwall + sector[i].wallnum - 1;
                        for (j=startwall;j<=endwall;j++)
                        {
                            dax += wall[j].x;
                            day += wall[j].y;
                        }
                        if (endwall > startwall)
                        {
                            dax /= (endwall-startwall+1);
                            day /= (endwall-startwall+1);
                        }

                        dax = mulscale14(dax-posx,zoom);
                        day = mulscale14(day-posy,zoom);

                        x1 = halfxdim16+dax-(Bstrlen(dabuffer)<<1);
                        y1 = midydim16+day-4;
                        x2 = x1 + (Bstrlen(dabuffer)<<2)+2;
                        y2 = y1 + 7;
                        if ((x1 > 3) && (x2 < xdim) && (y1 > 1) && (y2 < ydim16))
                        {
                            printext16(x1,y1,0,7,dabuffer,1);
                            drawline16(x1-1,y1-1,x2-3,y1-1,7);
                            drawline16(x1-1,y2+1,x2-3,y2+1,7);

                            drawline16(x1-2,y1,x1-2,y2,7);
                            drawline16(x2-2,y1,x2-2,y2,7);
                            drawline16(x2-3,y1,x2-3,y2,7);
                        }
                    }
                }

            x3 = divscale14(-halfxdim16,zoom)+posx;
            y3 = divscale14(-(midydim16-4),zoom)+posy;
            x4 = divscale14(halfxdim16,zoom)+posx;
            y4 = divscale14(ydim16-(midydim16-4),zoom)+posy;

            if (newnumwalls >= 0)
            {
                for (i=newnumwalls;i>=tempint;i--)
                    wall[i].cstat |= (1<<14);
            }

            i = numwalls-1;
            if (newnumwalls >= 0) i = newnumwalls-1;
            for (wal=&wall[i];i>=0;i--,wal--)
            {
                if (zoom < 768 && !(wal->cstat & (1<<14))) continue;
                //Get average point of wall
                dax = ((wal->x+wall[wal->point2].x)>>1);
                day = ((wal->y+wall[wal->point2].y)>>1);
                if ((dax > x3) && (dax < x4) && (day > y3) && (day < y4))
                {
                    dabuffer = (char *)ExtGetWallCaption(i);
                    if (dabuffer[0] != 0)
                    {
                        dax = mulscale14(dax-posx,zoom);
                        day = mulscale14(day-posy,zoom);
                        x1 = halfxdim16+dax-(Bstrlen(dabuffer)<<1);
                        y1 = midydim16+day-4;
                        x2 = x1 + (Bstrlen(dabuffer)<<2)+2;
                        y2 = y1 + 7;
                        if ((x1 > 3) && (x2 < xdim) && (y1 > 1) && (y2 < ydim16))
                        {
                            printext16(x1,y1,0,31,dabuffer,1);
                            drawline16(x1-1,y1-1,x2-3,y1-1,31);
                            drawline16(x1-1,y2+1,x2-3,y2+1,31);

                            drawline16(x1-2,y1,x1-2,y2,31);
                            drawline16(x2-2,y1,x2-2,y2,31);
                            drawline16(x2-3,y1,x2-3,y2,31);
                        }
                    }
                }
            }

            i = 0; j = numsprites;
            if (zoom >= 768)
                while ((j > 0) && (i < MAXSPRITES))
                {
                    if (sprite[i].statnum < MAXSTATUS)
                    {
                        dabuffer = (char *)ExtGetSpriteCaption(i);
                        if (dabuffer[0] != 0)
                        {
                            //Get average point of sprite
                            dax = sprite[i].x;
                            day = sprite[i].y;

                            dax = mulscale14(dax-posx,zoom);
                            day = mulscale14(day-posy,zoom);

                            x1 = halfxdim16+dax-(Bstrlen(dabuffer)<<1);
                            y1 = midydim16+day-4;
                            x2 = x1 + (Bstrlen(dabuffer)<<2)+2;
                            y2 = y1 + 7;
                            if ((x1 > 3) && (x2 < xdim) && (y1 > 1) && (y2 < ydim16))
                            {
                                col = 3;
                                if (spritecol2d[sprite[i].picnum][0])
                                    col = spritecol2d[sprite[i].picnum][0];
                                if ((sprite[i].cstat&1) > 0)
                                {
                                    col = 5;
                                    if (spritecol2d[sprite[i].picnum][1])
                                        col = spritecol2d[sprite[i].picnum][1];
                                }

                                if ((i == pointhighlight-16384) && (totalclock & 32)) col += (2<<2);

                                printext16(x1,y1,0,col,dabuffer,1);

                                drawline16(x1-1,y1-1,x2-3,y1-1,col);
                                drawline16(x1-1,y2+1,x2-3,y2+1,col);

                                drawline16(x1-2,y1,x1-2,y2,col);
                                drawline16(x2-2,y1,x2-2,y2,col);
                                drawline16(x2-3,y1,x2-3,y2,col);
                            }
                        }
                        j--;
                    }
                    i++;
                }
        }

        printcoords16(posx,posy,ang);

        numwalls = tempint;

        if (highlightsectorcnt > 0)
            for (i=0;i<highlightsectorcnt;i++)
                fillsector(highlightsector[i],2);

        if (keystatus[0x2a])
        {
            drawlinepat = 0x00ff00ff;
            drawline16(searchx,0,searchx,ydim2d-1,15);
            drawline16(0,searchy,xdim2d-1,searchy,15);
            drawlinepat = 0xffffffff;

            Bsprintf(tempbuf,"(%d,%d)",mousxplc,mousyplc);
            i = (Bstrlen(tempbuf)<<3)+6;
            if ((searchx+i) < (xdim2d-1))
                i = 0;
            else i = (searchx+i)-(xdim2d-1);
            if ((searchy+16) < (ydim2d-STATUS2DSIZ-1))
                j = 0;
            else j = (searchy+16)-(ydim2d-STATUS2DSIZ-1);
            printext16(searchx+6-i,searchy+6-j,11,-1,tempbuf,0);
        }
        drawline16(searchx,0,searchx,8,15);
        drawline16(0,searchy,8,searchy,15);

        col = 15-((gridlock<<1)+gridlock);
        if (joinsector[0] >= 0)col = 11;
        drawline16(searchx,searchy-8,searchx,searchy-1,col);
        drawline16(searchx+1,searchy-8,searchx+1,searchy-1,col);
        drawline16(searchx,searchy+2,searchx,searchy+9,col);
        drawline16(searchx+1,searchy+2,searchx+1,searchy+9,col);
        drawline16(searchx-8,searchy,searchx-1,searchy,col);
        drawline16(searchx-8,searchy+1,searchx-1,searchy+1,col);
        drawline16(searchx+2,searchy,searchx+9,searchy,col);
        drawline16(searchx+2,searchy+1,searchx+9,searchy+1,col);

        //Draw the white pixel closest to mouse cursor on linehighlight
        if (linehighlight>=0)
        {
            getclosestpointonwall(mousxplc,mousyplc,(int)linehighlight,&dax,&day);
            x2 = mulscale14(dax-posx,zoom);
            y2 = mulscale14(day-posy,zoom);
            if (wall[linehighlight].nextsector >= 0)
                drawline16(halfxdim16+x2,midydim16+y2,halfxdim16+x2,midydim16+y2,15);
            else
                drawline16(halfxdim16+x2,midydim16+y2,halfxdim16+x2,midydim16+y2,5);
        }
        enddrawing();	//}}}

        OSD_Draw();

        ExtCheckKeys(); // TX 20050101, it makes more sense to have this here so keys can be overwritten with new functions in bstub.c

        // Flip/mirror sector Ed Coolidge
        if (keystatus[0x2d])  // X (2D)
        {
            if (highlightsectorcnt > 0)
            {
                k = 0;
                dax = 0;
                day = 0;
                for (i=0;i<highlightsectorcnt;i++)
                {
                    startwall = sector[highlightsector[i]].wallptr;
                    endwall = startwall+sector[highlightsector[i]].wallnum-1;
                    for (j=startwall;j<=endwall;j++)
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

                k = (keystatus[0x38]|keystatus[0xb8]); //ALT-X mirror sector in x

                if ((gridlock > 0) && (grid > 0))
                {
                    dax = ((dax+(1024>>grid))&(0xffffffff<<(11-grid)));
                    day = ((day+(1024>>grid))&(0xffffffff<<(11-grid)));
                    //          dax = ((dax+(GRIDMAX>>grid))&(0xffffffff-(GRIDMAX>>(grid-1))+1));
                    //          day = ((day+(GRIDMAX>>grid))&(0xffffffff-(GRIDMAX>>(grid-1))+1));
                }

                for (i=0;i<highlightsectorcnt;i++)
                {
                    int startofloop;
                    int endofloop;
                    int numtoswap = -1;
                    int w=0;
                    walltype tempwall;

                    startofloop = startwall = sector[highlightsector[i]].wallptr;
                    endofloop = endwall = startwall+sector[highlightsector[i]].wallnum-1;

                    /*
                                        if (k != 0)
                                        {
                                           //mirror sector textures
                                        sector[highlightsector[i]].ceilingstat ^= 0x10;
                                        sector[highlightsector[i]].floorstat ^= 0x10;
                                    }
                    */
                    //save position of wall at start of loop
                    x3 = wall[startofloop].x;
                    y3 = wall[startofloop].y;
                    for (j=startwall;j<=endwall;j++)
                    {
                        //fix position of walls
                        wall[j].x = dax-wall[wall[j].point2].x+dax; //flip wall.x about dax
                        wall[j].y = wall[wall[j].point2].y;

                        if (k != 0)
                        {
                            //mirror walls about dax
                            wall[j].cstat ^= 8;
                        }

                        if (wall[j].point2==startofloop) //check if j is end of loop
                        {
                            endofloop = j;
                            wall[endofloop].x = dax-x3+dax; //flip wall.x about dax
                            wall[endofloop].y = y3;
                            //correct order of walls in loop to maintain player space (right-hand rule)
                            numtoswap = (endofloop-startofloop)>>1;
                            for (w=1;w<=numtoswap;w++)
                            {
                                Bmemcpy(&tempwall,&wall[startofloop+w],sizeof(walltype));
                                Bmemcpy(&wall[startofloop+w],&wall[endofloop-w+1],sizeof(walltype));
                                Bmemcpy(&wall[endofloop-w+1],&tempwall,sizeof(walltype));
                            }

                            //make point2 point to next wall in loop
                            for (w=startofloop;w<endofloop;w++)
                            {
                                wall[w].point2 = w+1;
                            }
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
                        x3 = sprite[j].x;
                        sprite[j].x = dax-x3+dax; //flip sprite.x about dax
                        sprite[j].ang = ((1024+2048-sprite[j].ang)&2047); //flip ang about 512
                        if (k != 0)
                        {
                            //don't mirror monsters
                            if ((sprite[j].cstat & 0x30) != 0)
                            {
                                //mirror sprites about day
                                sprite[j].cstat ^= 4;
                            }
                        }

                        j = nextspritesect[j];
                    }
                }
                /*if (k == 0)*/
                keystatus[0x2d] = 0;
                printmessage16("Selected sector(s) flipped");
                asksave = 1;
            }
        }
        if (keystatus[0x15])  // Y (2D)
        {
            if (highlightsectorcnt > 0)
            {
                k = 0;
                dax = 0;
                day = 0;
                for (i=0;i<highlightsectorcnt;i++)
                {
                    startwall = sector[highlightsector[i]].wallptr;
                    endwall = startwall+sector[highlightsector[i]].wallnum-1;
                    for (j=startwall;j<=endwall;j++)
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

                k = (keystatus[0x38]|keystatus[0xb8]); //ALT-Y mirror sectors in y

                if ((gridlock > 0) && (grid > 0))
                {
                    dax = ((dax+(1024>>grid))&(0xffffffff<<(11-grid)));
                    day = ((day+(1024>>grid))&(0xffffffff<<(11-grid)));
                    //          dax = ((dax+(GRIDMAX>>grid))&(0xffffffff-(GRIDMAX>>(grid-1))+1));
                    //          day = ((day+(GRIDMAX>>grid))&(0xffffffff-(GRIDMAX>>(grid-1))+1));
                }

                for (i=0;i<highlightsectorcnt;i++)
                {
                    int startofloop;
                    int endofloop;
                    int numtoswap = -1;
                    int w=0;
                    walltype tempwall;

                    startofloop = startwall = sector[highlightsector[i]].wallptr;
                    endofloop = endwall = startwall+sector[highlightsector[i]].wallnum-1;
                    /*
                                        if (k != 0)
                                        {
                                           //mirror sector textures
                                        sector[highlightsector[i]].ceilingstat ^= 0x10;
                                        sector[highlightsector[i]].floorstat ^= 0x10;
                                    }
                    */
                    //save position of wall at start of loop
                    x3 = wall[startofloop].x;
                    y3 = wall[startofloop].y;
                    for (j=startwall;j<=endwall;j++)
                    {
                        //fix position of walls
                        wall[j].x = wall[wall[j].point2].x;
                        wall[j].y = day-wall[wall[j].point2].y+day; //flip wall.y about day
                        if (k != 0)
                        {
                            //mirror walls about day
                            wall[j].cstat ^= 8;
                        }

                        if (wall[j].point2==startofloop) //check if j is end of loop
                        {
                            endofloop = j;
                            wall[endofloop].x = x3;
                            wall[endofloop].y = day-y3+day; //flip wall.y about dax
                            //correct order of walls in loop to maintain player space (right-hand rule)
                            numtoswap = (endofloop-startofloop)>>1;
                            for (w=1;w<=numtoswap;w++)
                            {
                                Bmemcpy(&tempwall,&wall[startofloop+w],sizeof(walltype));
                                Bmemcpy(&wall[startofloop+w],&wall[endofloop-w+1],sizeof(walltype));
                                Bmemcpy(&wall[endofloop-w+1],&tempwall,sizeof(walltype));
                            }

                            //make point2 point to next wall in loop
                            for (w=startofloop;w<endofloop;w++)
                            {
                                wall[w].point2 = w+1;
                            }
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
                        y3 = sprite[j].y;
                        sprite[j].y = day-y3+day; //flip sprite.y about day
                        sprite[j].ang = ((2048-sprite[j].ang)&2047); //flip ang about 512
                        if (k != 0)
                        {
                            //don't mirror monsters
                            if ((sprite[j].cstat & 0x30) != 0)
                            {
                                //mirror sprites about day
                                sprite[j].cstat ^= 4;
                            }
                        }

                        j = nextspritesect[j];
                    }
                }
                /*if (k == 0)*/
                keystatus[0x15] = 0;
                printmessage16("Selected sector(s) flipped");
                asksave = 1;
            }
        }
        // end edit for sector flip

        if (keystatus[88])   //F12
        {
            keystatus[88] = 0;
            /*
            j = ydim16; ydim16 = ydim;
            clear2dscreen();
            draw2dgrid(posx,posy,ang,zoom,grid);
            draw2dscreen(posx,posy,ang,zoom,grid);
            */

            screencapture("captxxxx.tga",keystatus[0x2a]|keystatus[0x36]);

            /*
            ydim16 = j;
            clear2dscreen();
            draw2dgrid(posx,posy,ang,zoom,grid);
            draw2dscreen(posx,posy,ang,zoom,grid);
            */
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
                if ((wall[linehighlight].nextwall >= 0) && ((keystatus[0x2a]|keystatus[0x36]) == 0))
                {
                    wall[wall[linehighlight].nextwall].cstat &= ~(1+64);
                    wall[wall[linehighlight].nextwall].cstat |= (wall[linehighlight].cstat&1);
                }
                asksave = 1;
            }
            keystatus[0x30] = 0;
        }
        if (keystatus[0x21])  //F (F alone does nothing in 2D right now)
        {
            keystatus[0x21] = 0;
            if (keystatus[0x38]|keystatus[0xb8])  //ALT-F (relative alignmment flip)
            {
                linehighlight = getlinehighlight(mousxplc, mousyplc, linehighlight);
                if (linehighlight >= 0)
                {
                    setfirstwall(sectorofwall(linehighlight),linehighlight);
                    asksave = 1;
                    printmessage16("This wall now sector's first wall (sector[].wallptr)");
                }
            }
        }

        if (keystatus[0x18])  // O (ornament onto wall) (2D)
        {
            keystatus[0x18] = 0;
            if ((pointhighlight&0xc000) == 16384)
            {
                asksave = 1;
                i = (pointhighlight&16383);

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
        }

        if (keystatus[0x33] || (bstatus&1 && bstatus&32))  // , (2D)
        {
            if (highlightsectorcnt > 0)
            {
                k = 0;
                dax = 0;
                day = 0;
                for (i=0;i<highlightsectorcnt;i++)
                {
                    startwall = sector[highlightsector[i]].wallptr;
                    endwall = startwall+sector[highlightsector[i]].wallnum-1;
                    for (j=startwall;j<=endwall;j++)
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

                k = (keystatus[0x2a]|keystatus[0x36]);

                if (k == 0)
                {
                    if ((gridlock > 0) && (grid > 0))
                    {
                        dax = ((dax+(1024>>grid))&(0xffffffff<<(11-grid)));
                        day = ((day+(1024>>grid))&(0xffffffff<<(11-grid)));
                    }
                }

                for (i=0;i<highlightsectorcnt;i++)
                {
                    startwall = sector[highlightsector[i]].wallptr;
                    endwall = startwall+sector[highlightsector[i]].wallnum-1;
                    for (j=startwall;j<=endwall;j++)
                    {
                        if (k == 0)
                        {
                            x3 = wall[j].x;
                            y3 = wall[j].y;
                            wall[j].x = dax+day-y3;
                            wall[j].y = day+x3-dax;
                        }
                        else
                        {
                            rotatepoint(dax,day,wall[j].x,wall[j].y,1,&wall[j].x,&wall[j].y);
                        }
                    }

                    j = headspritesect[highlightsector[i]];
                    while (j != -1)
                    {
                        if (k == 0)
                        {
                            x3 = sprite[j].x;
                            y3 = sprite[j].y;
                            sprite[j].x = dax+day-y3;
                            sprite[j].y = day+x3-dax;
                            sprite[j].ang = ((sprite[j].ang+512)&2047);
                        }
                        else
                        {
                            rotatepoint(dax,day,sprite[j].x,sprite[j].y,1,&sprite[j].x,&sprite[j].y);
                            sprite[j].ang = ((sprite[j].ang+1)&2047);
                        }

                        j = nextspritesect[j];
                    }
                }
                if (k == 0) keystatus[0x33] = 0;
                mouseb &= ~32;
                bstatus &= ~32;
                asksave = 1;
            }
            else
            {
                if (pointhighlight >= 16384)
                {
                    i = pointhighlight-16384;
                    if (keystatus[0x2a]|keystatus[0x36])
                        sprite[i].ang = ((sprite[i].ang+2048-1)&2047);
                    else
                    {
                        sprite[i].ang = ((sprite[i].ang+2048-128)&2047);
                        keystatus[0x33] = 0;
                    }

                    mouseb &= ~32;
                    bstatus &= ~32;

                    clearmidstatbar16();
                    showspritedata((short)pointhighlight-16384);
                }
            }
        }
        if (keystatus[0x34] || (bstatus&1 && bstatus&16))  // .  (2D)
        {
            if (highlightsectorcnt > 0)
            {
                k = 0;
                dax = 0;
                day = 0;
                for (i=0;i<highlightsectorcnt;i++)
                {
                    startwall = sector[highlightsector[i]].wallptr;
                    endwall = startwall+sector[highlightsector[i]].wallnum-1;
                    for (j=startwall;j<=endwall;j++)
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

                k = (keystatus[0x2a]|keystatus[0x36]);

                if (k == 0)
                {
                    if ((gridlock > 0) && (grid > 0))
                    {
                        dax = ((dax+(1024>>grid))&(0xffffffff<<(11-grid)));
                        day = ((day+(1024>>grid))&(0xffffffff<<(11-grid)));
                    }
                }

                for (i=0;i<highlightsectorcnt;i++)
                {
                    startwall = sector[highlightsector[i]].wallptr;
                    endwall = startwall+sector[highlightsector[i]].wallnum-1;
                    for (j=startwall;j<=endwall;j++)
                    {
                        if (k == 0)
                        {
                            x3 = wall[j].x;
                            y3 = wall[j].y;
                            wall[j].x = dax+y3-day;
                            wall[j].y = day+dax-x3;
                        }
                        else
                        {
                            rotatepoint(dax,day,wall[j].x,wall[j].y,2047,&wall[j].x,&wall[j].y);
                        }
                    }

                    j = headspritesect[highlightsector[i]];
                    while (j != -1)
                    {
                        if (k == 0)
                        {
                            x3 = sprite[j].x;
                            y3 = sprite[j].y;
                            sprite[j].x = dax+y3-day;
                            sprite[j].y = day+dax-x3;
                            sprite[j].ang = ((sprite[j].ang+1536)&2047);
                        }
                        else
                        {
                            rotatepoint(dax,day,sprite[j].x,sprite[j].y,2047,&sprite[j].x,&sprite[j].y);
                            sprite[j].ang = ((sprite[j].ang+2047)&2047);
                        }

                        j = nextspritesect[j];
                    }
                }
                if (k == 0) keystatus[0x34] = 0;
                mouseb &= ~16;
                bstatus &= ~16;
                asksave = 1;
            }
            else
            {
                if (pointhighlight >= 16384)
                {
                    i = pointhighlight-16384;
                    if (keystatus[0x2a]|keystatus[0x36])
                        sprite[i].ang = ((sprite[i].ang+2048+1)&2047);
                    else
                    {
                        sprite[i].ang = ((sprite[i].ang+2048+128)&2047);
                        keystatus[0x34] = 0;
                    }

                    mouseb &= ~16;
                    bstatus &= ~16;
                    clearmidstatbar16();
                    showspritedata((short)pointhighlight-16384);
                }
            }
        }
        if (keystatus[0x46])  //Scroll lock (set starting position)
        {
            startposx = posx;
            startposy = posy;
            startposz = posz;
            startang = ang;
            startsectnum = cursectnum;
            keystatus[0x46] = 0;
            asksave = 1;
        }

        if (keystatus[0x3f])  //F5
        {
            keystatus[0x3f] = 0;

            for (i=0;i<numsectors;i++)
                if (inside(mousxplc,mousyplc,i) == 1)
                {
                    ydim16 = STATUS2DSIZ;
                    ExtShowSectorData((short)i);
                    ydim16 = ydim-STATUS2DSIZ;
                    break;
                }
        }
        if (keystatus[0x40])  //F6
        {
            keystatus[0x40] = 0;

            if (pointhighlight >= 16384)
            {
                i = pointhighlight-16384;

                ydim16 = STATUS2DSIZ;
                ExtShowSpriteData((short)i);
                ydim16 = ydim-STATUS2DSIZ;
            }
            else if (linehighlight >= 0)
            {
                i = linehighlight;

                ydim16 = STATUS2DSIZ;
                ExtShowWallData((short)i);
                ydim16 = ydim-STATUS2DSIZ;
            }
        }
        if (keystatus[0x41])  //F7
        {
            keystatus[0x41] = 0;

            for (i=0;i<numsectors;i++)
                if (inside(mousxplc,mousyplc,i) == 1)
                {
                    ydim16 = STATUS2DSIZ;
                    ExtEditSectorData((short)i);
                    ydim16 = ydim-STATUS2DSIZ;
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
                ExtEditSpriteData((short)i);
                ydim16 = ydim-STATUS2DSIZ;
            }
            else if (linehighlight >= 0)
            {
                i = linehighlight;

                ydim16 = STATUS2DSIZ;
                ExtEditWallData((short)i);
                ydim16 = ydim-STATUS2DSIZ;
            }
        }

        if (keystatus[0x23])  //H (Hi 16 bits of tag)
        {
            keystatus[0x23] = 0;
            if (keystatus[0x1d]|keystatus[0x9d])  //Ctrl-H
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
                    if ((wall[linehighlight].nextwall >= 0) && ((keystatus[0x2a]|keystatus[0x36]) == 0))
                    {
                        wall[wall[linehighlight].nextwall].cstat &= ~64;
                        wall[wall[linehighlight].nextwall].cstat |= (wall[linehighlight].cstat&64);
                    }
                    asksave = 1;
                }
            }
            else if (keystatus[0x38]|keystatus[0xb8])  //ALT
            {
                if (pointhighlight >= 16384)
                {
                    i = pointhighlight-16384;
                    Bsprintf(buffer,"Sprite (%d) Hi-tag: ",i);
                    sprite[i].hitag = getnumber16(buffer,sprite[i].hitag,65536L,0);
                    clearmidstatbar16();
                    showspritedata((short)i);
                }
                else if (linehighlight >= 0)
                {
                    i = linehighlight;
                    Bsprintf(buffer,"Wall (%d) Hi-tag: ",i);
                    wall[i].hitag = getnumber16(buffer,wall[i].hitag,65536L,0);
                    clearmidstatbar16();
                    showwalldata((short)i);
                }
            }
            else
            {
                for (i=0;i<numsectors;i++)
                    if (inside(mousxplc,mousyplc,i) == 1)
                    {
                        Bsprintf(buffer,"Sector (%d) Hi-tag: ",i);
                        sector[i].hitag = getnumber16(buffer,sector[i].hitag,65536L,0);
                        clearmidstatbar16();
                        showsectordata((short)i);
                        break;
                    }
            }
            printmessage16("");
        }
        if (keystatus[0x19])  // P (palookup #)
        {
            keystatus[0x19] = 0;

            for (i=0;i<numsectors;i++)
                if (inside(mousxplc,mousyplc,i) == 1)
                {
                    Bsprintf(buffer,"Sector (%d) Ceilingpal: ",i);
                    sector[i].ceilingpal = getnumber16(buffer,sector[i].ceilingpal,256L,0);
                    clearmidstatbar16();
                    showsectordata((short)i);

                    Bsprintf(buffer,"Sector (%d) Floorpal: ",i);
                    sector[i].floorpal = getnumber16(buffer,sector[i].floorpal,256L,0);
                    clearmidstatbar16();
                    showsectordata((short)i);

                    printmessage16("");
                    break;
                }
        }
        if (keystatus[0x12])  // E (status list)
        {
            if (pointhighlight >= 16384)
            {
                i = pointhighlight-16384;
                Bsprintf(buffer,"Sprite (%d) Status list: ",i);
                changespritestat(i,getnumber16(buffer,sprite[i].statnum,65536L,0));
                clearmidstatbar16();
                showspritedata((short)i);
            }

            printmessage16("");

            keystatus[0x12] = 0;
        }

        if (keystatus[0x0f])  //TAB
        {
            clearmidstatbar16();

            if ((keystatus[0x38]|keystatus[0xb8]|keystatus[0x1d]|keystatus[0x9d]) > 0)  //ALT or CTRL
            {
                if (pointhighlight >= 16384)
                    showspritedata((short)pointhighlight-16384);
                else if (linehighlight >= 0)
                    showwalldata((short)linehighlight);
            }
            else
            {
                for (i=0;i<numsectors;i++)
                    if (inside(mousxplc,mousyplc,i) == 1)
                    {
                        showsectordata((short)i);
                        break;
                    }
            }
            keystatus[0x0f] = 0;
        }


        if (highlightsectorcnt < 0)
        {
            if (keystatus[0x36])  //Right shift (point highlighting)
            {
                if (highlightcnt == 0)
                {
                    highlightx2 = searchx, highlighty2 = searchy;
                    ydim16 = ydim-STATUS2DSIZ;
                    drawline16(highlightx2,highlighty1,highlightx1,highlighty1,5);
                    drawline16(highlightx2,highlighty2,highlightx1,highlighty2,5);
                    drawline16(highlightx1,highlighty2,highlightx1,highlighty1,5);
                    drawline16(highlightx2,highlighty2,highlightx2,highlighty1,5);
                }
                if (highlightcnt != 0)
                {
                    highlightx1 = searchx;
                    highlighty1 = searchy;
                    highlightx2 = searchx;
                    highlighty2 = searchx;
                    highlightcnt = 0;

                    for (i=0;i<(MAXWALLS>>3);i++)  //Clear all highlights
                        show2dwall[i] = 0;
                    for (i=0;i<(MAXSPRITES>>3);i++)
                        show2dsprite[i] = 0;
                }
            }
            else
            {
                if (highlightcnt == 0)
                {
                    getpoint(highlightx1,highlighty1,&highlightx1,&highlighty1);
                    getpoint(highlightx2,highlighty2,&highlightx2,&highlighty2);
                    if (highlightx1 > highlightx2)
                    {
                        tempint = highlightx1; highlightx1 = highlightx2; highlightx2 = tempint;
                    }
                    if (highlighty1 > highlighty2)
                    {
                        tempint = highlighty1; highlighty1 = highlighty2; highlighty2 = tempint;
                    }

                    if (keystatus[0x1d]|keystatus[0x9d])
                    {
                        if ((linehighlight >= 0) && (linehighlight < MAXWALLS))
                        {
                            i = linehighlight;
                            do
                            {
                                highlight[highlightcnt++] = i;
                                show2dwall[i>>3] |= (1<<(i&7));

                                for (j=0;j<numwalls;j++)
                                    if (wall[j].x == wall[i].x)
                                        if (wall[j].y == wall[i].y)
                                            if (i != j)
                                            {
                                                highlight[highlightcnt++] = j;
                                                show2dwall[j>>3] |= (1<<(j&7));
                                            }

                                i = wall[i].point2;
                            }
                            while (i != linehighlight);
                        }
                    }
                    else
                    {
                        for (i=0;i<numwalls;i++)
                            if ((wall[i].x >= highlightx1) && (wall[i].x <= highlightx2))
                                if ((wall[i].y >= highlighty1) && (wall[i].y <= highlighty2))
                                {
                                    highlight[highlightcnt++] = i;
                                    show2dwall[i>>3] |= (1<<(i&7));
                                }
                        for (i=0;i<MAXSPRITES;i++)
                            if (sprite[i].statnum < MAXSTATUS)
                                if ((sprite[i].x >= highlightx1) && (sprite[i].x <= highlightx2))
                                    if ((sprite[i].y >= highlighty1) && (sprite[i].y <= highlighty2))
                                    {
                                        highlight[highlightcnt++] = i+16384;
                                        show2dsprite[i>>3] |= (1<<(i&7));
                                    }
                    }

                    if (highlightcnt <= 0)
                        highlightcnt = -1;
                }
            }
        }
        if (highlightcnt < 0)
        {
            if (keystatus[0xb8])  //Right alt (sector highlighting)
            {
                if (highlightsectorcnt == 0)
                {
                    highlightx2 = searchx, highlighty2 = searchy;
                    ydim16 = ydim-STATUS2DSIZ;
                    begindrawing();	//{{{
                    drawline16(highlightx2,highlighty1,highlightx1,highlighty1,10);
                    drawline16(highlightx2,highlighty2,highlightx1,highlighty2,10);
                    drawline16(highlightx1,highlighty2,highlightx1,highlighty1,10);
                    drawline16(highlightx2,highlighty2,highlightx2,highlighty1,10);
                    enddrawing();	//}}}
                }
                if (highlightsectorcnt != 0)
                {
                    for (i=0;i<highlightsectorcnt;i++)
                    {
                        startwall = sector[highlightsector[i]].wallptr;
                        endwall = startwall+sector[highlightsector[i]].wallnum-1;
                        for (j=startwall;j<=endwall;j++)
                        {
                            if (wall[j].nextwall >= 0)
                                checksectorpointer(wall[j].nextwall,wall[j].nextsector);
                            checksectorpointer((short)j,highlightsector[i]);
                        }
                    }
                    highlightx1 = searchx;
                    highlighty1 = searchy;
                    highlightx2 = searchx;
                    highlighty2 = searchx;
                    highlightsectorcnt = 0;
                }
            }
            else
            {
                if (highlightsectorcnt == 0)
                {
                    getpoint(highlightx1,highlighty1,&highlightx1,&highlighty1);
                    getpoint(highlightx2,highlighty2,&highlightx2,&highlighty2);
                    if (highlightx1 > highlightx2)
                    {
                        tempint = highlightx1; highlightx1 = highlightx2; highlightx2 = tempint;
                    }
                    if (highlighty1 > highlighty2)
                    {
                        tempint = highlighty1; highlighty1 = highlighty2; highlighty2 = tempint;
                    }

                    for (i=0;i<numsectors;i++)
                    {
                        startwall = sector[i].wallptr;
                        endwall = startwall + sector[i].wallnum;
                        bad = 0;
                        for (j=startwall;j<endwall;j++)
                        {
                            if (wall[j].x < highlightx1) bad = 1;
                            if (wall[j].x > highlightx2) bad = 1;
                            if (wall[j].y < highlighty1) bad = 1;
                            if (wall[j].y > highlighty2) bad = 1;
                            if (bad == 1) break;
                        }
                        if (bad == 0)
                            highlightsector[highlightsectorcnt++] = i;
                    }
                    if (highlightsectorcnt <= 0)
                        highlightsectorcnt = -1;

                    //White out all bordering lines of grab that are
                    //not highlighted on both sides
                    for (i=highlightsectorcnt-1;i>=0;i--)
                    {
                        startwall = sector[highlightsector[i]].wallptr;
                        endwall = startwall + sector[highlightsector[i]].wallnum;
                        for (j=startwall;j<endwall;j++)
                        {
                            if (wall[j].nextwall >= 0)
                            {
                                for (k=highlightsectorcnt-1;k>=0;k--)
                                    if (highlightsector[k] == wall[j].nextsector)
                                        break;
                                if (k < 0)
                                {
                                    wall[wall[j].nextwall].nextwall = -1;
                                    wall[wall[j].nextwall].nextsector = -1;
                                    wall[j].nextwall = -1;
                                    wall[j].nextsector = -1;
                                }
                            }
                        }
                    }

                }
            }
        }

        if (((bstatus&1) < (oldmousebstatus&1)) && (highlightsectorcnt < 0))  //after dragging
        {
            int wallsdrawn = newnumwalls-numwalls;

            if (newnumwalls != -1)
            {
                newnumwalls = -1;
                Bmemcpy(&wall[MAXWALLS-wallsdrawn],&wall[numwalls],sizeof(walltype) * wallsdrawn);
            }
            else wallsdrawn = -1;

            j = 1;
            if (highlightcnt > 0)
                for (i=0;i<highlightcnt;i++)
                    if (pointhighlight == highlight[i])
                    {
                        j = 0;
                        break;
                    }

            if (j == 0)
            {
                for (i=0;i<highlightcnt;i++)
                {
                    if ((highlight[i]&0xc000) == 16384)
                    {
                        j = (highlight[i]&16383);

                        setsprite(j,sprite[j].x,sprite[j].y,sprite[j].z);

                        tempint = ((tilesizy[sprite[j].picnum]*sprite[j].yrepeat)<<2);
                        sprite[j].z = max(sprite[j].z,getceilzofslope(sprite[j].sectnum,sprite[j].x,sprite[j].y)+tempint);
                        sprite[j].z = min(sprite[j].z,getflorzofslope(sprite[j].sectnum,sprite[j].x,sprite[j].y));
                    }
                }
            }
            else if ((pointhighlight&0xc000) == 16384)
            {
                j = (pointhighlight&16383);

                setsprite(j,sprite[j].x,sprite[j].y,sprite[j].z);

                tempint = ((tilesizy[sprite[j].picnum]*sprite[j].yrepeat)<<2);

                sprite[j].z = max(sprite[j].z,getceilzofslope(sprite[j].sectnum,sprite[j].x,sprite[j].y)+tempint);
                sprite[j].z = min(sprite[j].z,getflorzofslope(sprite[j].sectnum,sprite[j].x,sprite[j].y));
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

            for (i=numwalls-1;i>=0;i--)    //delete points
            {
                if (wall[i].x == wall[wall[i].point2].x)
                    if (wall[i].y == wall[wall[i].point2].y)
                        if (sector[sectorofwall((short)i)].wallnum > 3)
                            if (sector[sectorofwall(wall[i].point2)].wallnum > 3)
                                if (!(wall[i].nextwall == -1 ||
                                        (sector[sectorofwall(wall[i].nextwall)].wallnum > 3 &&
                                         sector[sectorofwall(wall[wall[i].nextwall].point2)].wallnum > 3)))
                                {
                                    printmessage16("Invalid operation, delete or join sector instead.");
                                    goto SKIP;
                                }
            }

            for (i=numwalls-1;i>=0;i--)    //delete points
            {
                if (wall[i].x == wall[wall[i].point2].x)
                    if (wall[i].y == wall[wall[i].point2].y)
                        if (sector[sectorofwall((short)i)].wallnum > 3)
                            if (sector[sectorofwall(wall[i].point2)].wallnum > 3)
                                if (wall[i].nextwall == -1 ||
                                        (sector[sectorofwall(wall[i].nextwall)].wallnum > 3 &&
                                         sector[sectorofwall(wall[wall[i].nextwall].point2)].wallnum > 3))
                                {
                                    deletepoint((short)i);
                                    printmessage16("Point deleted.");
                                    asksave = 1;
                                }
            }

            for (i=0;i<numwalls;i++)       //make new red lines?
            {
                if ((wall[i].x == dax) && (wall[i].y == day))
                {
                    checksectorpointer((short)i,sectorofwall((short)i));
                    fixrepeats((short)i);
                    asksave = 1;
                }
                else if ((wall[wall[i].point2].x == dax) && (wall[wall[i].point2].y == day))
                {
                    checksectorpointer((short)i,sectorofwall((short)i));
                    fixrepeats((short)i);
                    asksave = 1;
                }
            }

            if (wallsdrawn != -1)
            {
                Bmemcpy(&wall[numwalls],&wall[MAXWALLS-wallsdrawn],sizeof(walltype) * wallsdrawn);
                newnumwalls = numwalls + wallsdrawn;
                for (i=numwalls;i<newnumwalls;i++)
                    wall[i].point2 = i+1;
            }
        }

SKIP:
        if ((bstatus&1) > 0)                //drag points
        {
            if (highlightsectorcnt > 0)
            {
                if ((bstatus&1) > (oldmousebstatus&1))
                {
                    newnumwalls = -1;
                    sectorhighlightstat = -1;
                    updatesector(mousxplc,mousyplc,&cursectorhighlight);

                    if ((cursectorhighlight >= 0) && (cursectorhighlight < numsectors))
                    {
                        for (i=0;i<highlightsectorcnt;i++)
                            if (cursectorhighlight == highlightsector[i])
                            {
                                //You clicked inside one of the flashing sectors!
                                sectorhighlightstat = 1;

                                dax = mousxplc;
                                day = mousyplc;
                                if ((gridlock > 0) && (grid > 0))
                                {
                                    dax = ((dax+(1024>>grid))&(0xffffffff<<(11-grid)));
                                    day = ((day+(1024>>grid))&(0xffffffff<<(11-grid)));
                                }
                                sectorhighlightx = dax;
                                sectorhighlighty = day;
                                break;
                            }
                    }
                }
                else if (sectorhighlightstat == 1)
                {
                    dax = mousxplc;
                    day = mousyplc;
                    if ((gridlock > 0) && (grid > 0))
                    {
                        dax = ((dax+(1024>>grid))&(0xffffffff<<(11-grid)));
                        day = ((day+(1024>>grid))&(0xffffffff<<(11-grid)));
                    }

                    dax -= sectorhighlightx;
                    day -= sectorhighlighty;
                    sectorhighlightx += dax;
                    sectorhighlighty += day;

                    for (i=0;i<highlightsectorcnt;i++)
                    {
                        startwall = sector[highlightsector[i]].wallptr;
                        endwall = startwall+sector[highlightsector[i]].wallnum-1;
                        for (j=startwall;j<=endwall;j++)
                            { wall[j].x += dax; wall[j].y += day; }

                        for (j=headspritesect[highlightsector[i]];j>=0;j=nextspritesect[j])
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
            else
            {
                if ((bstatus&1) > (oldmousebstatus&1))
                    pointhighlight = getpointhighlight(mousxplc, mousyplc, pointhighlight);

                if (pointhighlight >= 0)
                {
                    dax = mousxplc;
                    day = mousyplc;
                    if ((gridlock > 0) && (grid > 0))
                    {
                        dax = ((dax+(1024>>grid))&(0xffffffff<<(11-grid)));
                        day = ((day+(1024>>grid))&(0xffffffff<<(11-grid)));
                    }

                    j = 1;
                    if (highlightcnt > 0)
                        for (i=0;i<highlightcnt;i++)
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
                        for (i=0;i<highlightcnt;i++)
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
                            dragpoint(pointhighlight,dax,day);
                        else if ((pointhighlight&0xc000) == 16384)
                        {
                            daz = ((tilesizy[sprite[pointhighlight&16383].picnum]*sprite[pointhighlight&16383].yrepeat)<<2);

                            for (i=0;i<numsectors;i++)
                                if (inside(dax,day,i) == 1)
                                    if (sprite[pointhighlight&16383].z >= getceilzofslope(i,dax,day))
                                        if (sprite[pointhighlight&16383].z-daz <= getflorzofslope(i,dax,day))
                                        {
                                            sprite[pointhighlight&16383].x = dax;
                                            sprite[pointhighlight&16383].y = day;
                                            if (sprite[pointhighlight&16383].sectnum != i)
                                                changespritesect(pointhighlight&16383,(short)i);
                                            break;
                                        }
                        }
                    }
                    asksave = 1;
                }
            }
        }
        else
        {
            pointhighlight = getpointhighlight(mousxplc, mousyplc, pointhighlight);
            sectorhighlightstat = -1;
        }

        if ((bstatus&6) > 0)
        {
            searchx = halfxdim16;
            searchy = midydim16;
            posx = mousxplc;
            posy = mousyplc;
        }

        if (circlewall != -1 && (keystatus[0x4a] || (bstatus&32 && !(keystatus[0x1d]|keystatus[0x9d]))))  // -
        {
            if (circlepoints > 1)
                circlepoints--;
            keystatus[0x4a] = 0;
            mouseb &= ~32;
            bstatus &= ~32;
        }
        if (circlewall != -1 && (keystatus[0x4e] || (bstatus&16 && !(keystatus[0x1d]|keystatus[0x9d]))))  // +
        {
            if (circlepoints < 63)
                circlepoints++;
            keystatus[0x4e] = 0;
            mouseb &= ~16;
            bstatus &= ~16;
        }

        if ((keystatus[buildkeys[BK_MOVEUP]] || (bstatus&16)) && (zoom < 16384))
        {
            zoom += synctics*(zoom>>4);
            if (zoom < 24) zoom += 2;
            if (bstatus&16 && (keystatus[0x38] || keystatus[0xb8]))
            {
                searchx = halfxdim16;
                searchy = midydim16;
                posx = mousxplc;
                posy = mousyplc;
            }
        }
        if ((keystatus[buildkeys[BK_MOVEDOWN]] || (bstatus&32)) && (zoom > 8))
        {
            zoom -= synctics*(zoom>>4);
            if (bstatus&32 && (keystatus[0x38] || keystatus[0xb8]))
            {
                searchx = halfxdim16;
                searchy = midydim16;
                posx = mousxplc;
                posy = mousyplc;
            }
        }
        if (zoom < 8) zoom = 8;
        if (zoom > 16384) zoom = 16384;

        if (keystatus[0x22])  // G (grid on/off)
        {
            grid++;
            if (grid == 7) grid = 0;
            keystatus[0x22] = 0;
        }
        if (keystatus[0x26])  // L (grid lock)
        {
            gridlock = 1-gridlock, keystatus[0x26] = 0;
            if (gridlock == 0)
                printmessage16("Grid locking off");
            else
                printmessage16("Grid locking on");
        }

        if (keystatus[0x24])  // J (join sectors)
        {
            if (joinsector[0] >= 0)
            {
                joinsector[1] = -1;

                for (i=0;i<numsectors;i++)
                {
                    if (inside(mousxplc,mousyplc,i) == 1)
                    {
                        startwall = sector[i].wallptr;
                        endwall = startwall + sector[i].wallnum - 1;
                        for (j=startwall;j<=endwall;j++)
                        {
                            if (wall[j].nextsector == joinsector[0])
                                break;
                        }

                        if (j >= endwall)
                        {
                            printmessage16("Join non-adjacent sectors? (Y/N)");
                            showframe(1);
                            bflushchars(); ch = 0;
                            while (keystatus[1] == 0)
                            {
                                if (handleevents())
                                {
                                    if (quitevent)
                                    {
                                        quitevent = 0;
                                    }
                                }
                                idle();

                                ch = bgetchar();

                                if (ch == 'Y' || ch == 'y')
                                {
                                    joinsector[1] = i;
                                    break;
                                }
                                else if (ch == 'N' || ch == 'n' || ch == 13 || ch == ' ')
                                {
                                    joinsector[1] = joinsector[0];
                                    break;
                                }
                            }
                            break;
                        }
                        else
                        {
                            joinsector[1] = i;
                            break;
                        }
                    }
                }

                if ((joinsector[1] >= 0) && (joinsector[0] != joinsector[1]))
                {
                    newnumwalls = numwalls;

                    for (k=0;k<2;k++)
                    {
                        startwall = sector[joinsector[k]].wallptr;
                        endwall = startwall + sector[joinsector[k]].wallnum - 1;
                        for (j=startwall;j<=endwall;j++)
                        {
                            if (wall[j].cstat == 255)
                                continue;
                            joinsectnum = k;
                            if (wall[j].nextsector == joinsector[1-joinsectnum])
                            {
                                wall[j].cstat = 255;
                                continue;
                            }

                            i = j;
                            m = newnumwalls;
                            do
                            {
                                Bmemcpy(&wall[newnumwalls],&wall[i],sizeof(walltype));
                                wall[newnumwalls].point2 = newnumwalls+1;
                                newnumwalls++;
                                wall[i].cstat = 255;

                                i = wall[i].point2;
                                if (wall[i].nextsector == joinsector[1-joinsectnum])
                                {
                                    i = wall[wall[i].nextwall].point2;
                                    joinsectnum = 1 - joinsectnum;
                                }
                            }
                            while ((wall[i].cstat != 255) && (wall[i].nextsector != joinsector[1-joinsectnum]));
                            wall[newnumwalls-1].point2 = m;
                        }
                    }

                    if (newnumwalls > numwalls)
                    {
                        Bmemcpy(&sector[numsectors],&sector[joinsector[0]],sizeof(sectortype));
                        sector[numsectors].wallptr = numwalls;
                        sector[numsectors].wallnum = newnumwalls-numwalls;

                        //fix sprites
                        for (i=0;i<2;i++)
                        {
                            j = headspritesect[joinsector[i]];
                            while (j != -1)
                            {
                                k = nextspritesect[j];
                                changespritesect(j,numsectors);
                                j = k;
                            }
                        }

                        numsectors++;

                        for (i=numwalls;i<newnumwalls;i++)
                        {
                            if (wall[i].nextwall >= 0)
                            {
                                wall[wall[i].nextwall].nextwall = i;
                                wall[wall[i].nextwall].nextsector = numsectors-1;
                            }
                        }

                        numwalls = newnumwalls;
                        newnumwalls = -1;

                        for (k=0;k<2;k++)
                        {
                            startwall = sector[joinsector[k]].wallptr;
                            endwall = startwall + sector[joinsector[k]].wallnum - 1;
                            for (j=startwall;j<=endwall;j++)
                            {
                                wall[j].nextwall = -1;
                                wall[j].nextsector = -1;
                            }
                        }

                        deletesector((short)joinsector[0]);
                        if (joinsector[0] < joinsector[1])
                            joinsector[1]--;
                        deletesector((short)joinsector[1]);
                        printmessage16("Sectors joined.");
                    }
                }
                else
                {
                    printmessage16("No sectors joined.");
                }
                joinsector[0] = -1;
            }
            else
            {
                joinsector[0] = -1;
                for (i=0;i<numsectors;i++)
                    if (inside(mousxplc,mousyplc,i) == 1)
                    {
                        joinsector[0] = i;
                        printmessage16("Join sector - press J again on sector to join with.");
                        break;
                    }
            }
            keystatus[0x24] = 0;
        }

// PK
        for (i=0x02; i<=0x0b; i++)  // keys '1' to '0' on the upper row
            if (keystatus[i])
            {
                prefixarg = prefixtiles[i-2];
                break;
            }

        if ((keystatus[0x38]||keystatus[0xb8])&&keystatus[0x1f]) //ALT-S
        {
            if ((linehighlight >= 0) && (wall[linehighlight].nextwall == -1))
            {
                if ((newnumwalls = whitelinescan(linehighlight)) < numwalls)
                {
                    printmessage16("Can't make a sector out there.");
                }
                else
                {
                    for (i=numwalls;i<newnumwalls;i++)
                    {
                        wall[wall[i].nextwall].nextwall = i;
                        wall[wall[i].nextwall].nextsector = numsectors;
                    }
                    numwalls = newnumwalls;
                    newnumwalls = -1;
                    numsectors++;
                    printmessage16("Inner loop made into new sector.");
                }
            }
            keystatus[0x1f] = 0;
        }
        else if (keystatus[0x1f])  //S
        {
            sucksect = -1;
            for (i=0;i<numsectors;i++)
                if (inside(mousxplc,mousyplc,i) == 1)
                {
                    sucksect = i;
                    break;
                }

            if (sucksect >= 0)
            {
                dax = mousxplc;
                day = mousyplc;
                if ((gridlock > 0) && (grid > 0))
                {
                    dax = ((dax+(1024>>grid))&(0xffffffff<<(11-grid)));
                    day = ((day+(1024>>grid))&(0xffffffff<<(11-grid)));
                }

                i = insertsprite(sucksect,0);
                sprite[i].x = dax, sprite[i].y = day;
                sprite[i].cstat = defaultspritecstat;
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

                sprite[i].z = getflorzofslope(sucksect,dax,day);
                if ((sprite[i].cstat&128) != 0)
                    sprite[i].z -= ((tilesizy[sprite[i].picnum]*sprite[i].yrepeat)<<1);

                for (k=0;k<MAXTILES;k++)
                    localartfreq[k] = 0;
                for (k=0;k<MAXSPRITES;k++)
                    if (sprite[k].statnum < MAXSTATUS)
                        localartfreq[sprite[k].picnum]++;
                j = 0;
                for (k=0;k<MAXTILES;k++)
                    if (localartfreq[k] > localartfreq[j])
                        j = k;
                if (localartfreq[j] > 0)
                    sprite[i].picnum = j;
                else
                    sprite[i].picnum = 0;

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
                    if ((tilesizx[temppicnum] <= 0) || (tilesizy[temppicnum] <= 0))
                    {
                        j = 0;
                        for (k=0;k<MAXTILES;k++)
                            if ((tilesizx[k] > 0) && (tilesizy[k] > 0))
                            {
                                j = k;
                                break;
                            }
                        sprite[i].picnum = j;
                    }
                    sprite[i].shade = tempshade;
                    sprite[i].pal = temppal;
                    sprite[i].xrepeat = tempxrepeat;
                    sprite[i].yrepeat = tempyrepeat;
                    if (sprite[i].xrepeat < 1) sprite[i].xrepeat = 1;
                    if (sprite[i].yrepeat < 1) sprite[i].yrepeat = 1;
                    sprite[i].cstat = tempcstat;
                }

                if (tilesizy[sprite[i].picnum] >= 32)
                    sprite[i].cstat |= 1;

                printmessage16("Sprite inserted.");
                updatenumsprites();
                asksave = 1;
            }

            keystatus[0x1f] = 0;
        }

        if (keystatus[0x2e])  // C (make circle of points)
        {
            if (highlightsectorcnt >= 0)
            {
                newnumsectors = numsectors;
                newnumwalls = numwalls;
                for (i=0;i<highlightsectorcnt;i++)
                {
                    copysector(highlightsector[i],newnumsectors,newnumwalls,1);
                    newnumsectors++;
                    newnumwalls += sector[highlightsector[i]].wallnum;
                }

                for (i=0;i<highlightsectorcnt;i++)
                {
                    startwall = sector[highlightsector[i]].wallptr;
                    endwall = startwall+sector[highlightsector[i]].wallnum-1;
                    for (j=startwall;j<=endwall;j++)
                    {
                        if (wall[j].nextwall >= 0)
                            checksectorpointer(wall[j].nextwall,wall[j].nextsector);
                        checksectorpointer((short)j,highlightsector[i]);
                    }
                    highlightsector[i] = numsectors+i;
                }
                numsectors = newnumsectors;
                numwalls = newnumwalls;

                newnumwalls = -1;
                newnumsectors = -1;

                updatenumsprites();
                printmessage16("Sectors duplicated and stamped.");
                asksave = 1;
            }
            else if (highlightcnt >= 0)
            {
                for (i=0;i<highlightcnt;i++)
                    if ((highlight[i]&0xc000) == 16384)
                    {
                        //duplicate sprite
                        k = (highlight[i]&16383);
                        j = insertsprite(sprite[k].sectnum,sprite[k].statnum);
                        Bmemcpy(&sprite[j],&sprite[k],sizeof(spritetype));
                        sprite[j].sectnum = sprite[k].sectnum;   //Don't let memcpy overwrite sector!
                        setsprite(j,sprite[j].x,sprite[j].y,sprite[j].z);
                    }
                updatenumsprites();
                printmessage16("Sprites duplicated and stamped.");
                asksave = 1;
            }

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

        bad = (keystatus[0x39]);  //Gotta do this to save lots of 3 spaces!

        if (circlewall >= 0)
        {
            x1 = wall[circlewall].x;
            y1 = wall[circlewall].y;
            x2 = wall[wall[circlewall].point2].x;
            y2 = wall[wall[circlewall].point2].y;
            x3 = mousxplc;
            y3 = mousyplc;
            adjustmark(&x3,&y3,newnumwalls);
            tempint1 = dmulscale4(x3-x2,x1-x3,y1-y3,y3-y2);
            tempint2 = dmulscale4(y1-y2,x1-x3,y1-y3,x2-x1);
            if (tempint2 != 0)
            {
                int pointsize = 2;

                centerx = (((x1+x2) + scale(y1-y2,tempint1,tempint2))>>1);
                centery = (((y1+y2) + scale(x2-x1,tempint1,tempint2))>>1);

                dax = mulscale14(centerx-posx,zoom);
                day = mulscale14(centery-posy,zoom);
                drawline16(halfxdim16+dax-2,midydim16+day-2,halfxdim16+dax+2,midydim16+day+2,14);
                drawline16(halfxdim16+dax-2,midydim16+day+2,halfxdim16+dax+2,midydim16+day-2,14);

                circleang1 = getangle(x1-centerx,y1-centery);
                circleang2 = getangle(x2-centerx,y2-centery);

                circleangdir = 1;
                k = ((circleang2-circleang1)&2047);
                if (mulscale4(x3-x1,y2-y1) < mulscale4(x2-x1,y3-y1))
                {
                    circleangdir = -1;
                    k = -((circleang1-circleang2)&2047);
                }

                circlerad = (ksqrt(dmulscale4(centerx-x1,centerx-x1,centery-y1,centery-y1))<<2);

                for (i=circlepoints;i>0;i--)
                {
                    j = ((circleang1 + scale(i,k,circlepoints+1))&2047);
                    dax = centerx+mulscale14(sintable[(j+512)&2047],circlerad);
                    day = centery+mulscale14(sintable[j],circlerad);

                    if (dax <= -editorgridextent) dax = -editorgridextent;
                    if (dax >= editorgridextent) dax = editorgridextent;
                    if (day <= -editorgridextent) day = -editorgridextent;
                    if (day >= editorgridextent) day = editorgridextent;

                    if (bad > 0)
                    {
                        m = 0;
                        if (wall[circlewall].nextwall >= 0)
                            if (wall[circlewall].nextwall < circlewall) m = 1;
                        insertpoint(circlewall,dax,day);
                        circlewall += m;
                    }
                    dax = mulscale14(dax-posx,zoom);
                    day = mulscale14(day-posy,zoom);
                    drawline16(halfxdim16+dax-pointsize,midydim16+day-pointsize,halfxdim16+dax+pointsize,midydim16+day-pointsize,14);
                    drawline16(halfxdim16+dax+pointsize,midydim16+day-pointsize,halfxdim16+dax+pointsize,midydim16+day+pointsize,14);
                    drawline16(halfxdim16+dax+pointsize,midydim16+day+pointsize,halfxdim16+dax-pointsize,midydim16+day+pointsize,14);
                    drawline16(halfxdim16+dax-pointsize,midydim16+day+pointsize,halfxdim16+dax-pointsize,midydim16+day-pointsize,14);
//                    drawcircle16(halfxdim16+dax, midydim16+day, 3, 14);
                }
                if (bad > 0)
                {
                    bad = 0;
                    keystatus[0x39] = 0;
                    asksave = 1;
                    printmessage16("Circle points inserted.");
                    circlewall = -1;
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
            if ((newnumwalls < numwalls) && (numwalls < MAXWALLS-1))
            {
                firstx = mousxplc, firsty = mousyplc;  //Make first point
                newnumwalls = numwalls;
                suckwall = -1;
                split = 0;

                //clearbufbyte(&wall[newnumwalls],sizeof(walltype),0L);
                memset(&wall[newnumwalls],0,sizeof(walltype));
                wall[newnumwalls].extra = -1;

                wall[newnumwalls].x = mousxplc;
                wall[newnumwalls].y = mousyplc;
                wall[newnumwalls].nextsector = -1;
                wall[newnumwalls].nextwall = -1;
                for (i=0;i<numwalls;i++)
                    if ((wall[i].x == mousxplc) && (wall[i].y == mousyplc))
                        suckwall = i;
                wall[newnumwalls].point2 = newnumwalls+1;
                printmessage16("Sector drawing started.");
                newnumwalls++;
            }
            else
            {
                //if not back to first point
                if ((firstx != mousxplc) || (firsty != mousyplc))  //nextpoint
                {
                    j = 0;
                    for (i=numwalls;i<newnumwalls;i++)
                        if ((mousxplc == wall[i].x) && (mousyplc == wall[i].y))
                            j = 1;
                    if (j == 0)
                    {
                        //check if starting to split a sector
                        if (newnumwalls == numwalls+1)
                        {
                            dax = ((wall[numwalls].x+mousxplc)>>1);
                            day = ((wall[numwalls].y+mousyplc)>>1);
                            for (i=0;i<numsectors;i++)
                                if (inside(dax,day,i) == 1)
                                {
                                    //check if first point at point of sector
                                    m = -1;
                                    startwall = sector[i].wallptr;
                                    endwall = startwall + sector[i].wallnum - 1;
                                    for (k=startwall;k<=endwall;k++)
                                        if (wall[k].x == wall[numwalls].x)
                                            if (wall[k].y == wall[numwalls].y)
                                            {
                                                m = k;
                                                break;
                                            }
                                    if (m >= 0)
                                        if ((wall[wall[k].point2].x != mousxplc) || (wall[wall[k].point2].y != mousyplc))
                                            if ((wall[lastwall((short)k)].x != mousxplc) || (wall[lastwall((short)k)].y != mousyplc))
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
                        for (i=0;i<numwalls;i++)
                        {
                            if (wall[i].nextwall >= 0)
                            {
                                if ((wall[i].x == mousxplc) && (wall[i].y == mousyplc))
                                    if ((wall[wall[i].point2].x == wall[newnumwalls-1].x) && (wall[wall[i].point2].y == wall[newnumwalls-1].y))
                                        bad = 1;
                                if ((wall[i].x == wall[newnumwalls-1].x) && (wall[i].y == wall[newnumwalls-1].y))
                                    if ((wall[wall[i].point2].x == mousxplc) && (wall[wall[i].point2].y == mousyplc))
                                        bad = 1;
                            }
                        }

                        if (bad == 0)
                        {
                            //clearbufbyte(&wall[newnumwalls],sizeof(walltype),0L);
                            memset(&wall[newnumwalls],0,sizeof(walltype));
                            wall[newnumwalls].extra = -1;

                            wall[newnumwalls].x = mousxplc;
                            wall[newnumwalls].y = mousyplc;
                            wall[newnumwalls].nextsector = -1;
                            wall[newnumwalls].nextwall = -1;
                            for (i=0;i<numwalls;i++)
                                if ((wall[i].x == mousxplc) && (wall[i].y == mousyplc))
                                    suckwall = i;
                            wall[newnumwalls].point2 = newnumwalls+1;
                            newnumwalls++;
                        }
                        else
                        {
                            printmessage16("You can't draw new lines over red lines.");
                        }
                    }
                }

                //if not split and back to first point
                if ((split == 0) && (firstx == mousxplc) && (firsty == mousyplc) && (newnumwalls >= numwalls+3))
                {
                    wall[newnumwalls-1].point2 = numwalls;

                    if (suckwall == -1)  //if no connections to other sectors
                    {
                        k = -1;
                        for (i=0;i<numsectors;i++)
                            if (inside(firstx,firsty,i) == 1)
                                k = i;
                        if (k == -1)   //if not inside another sector either
                        {
                            //add island sector
                            if (clockdir(numwalls) == 1)
                                flipwalls(numwalls,newnumwalls);

                            //clearbufbyte(&sector[numsectors],sizeof(sectortype),0L);
                            memset(&sector[numsectors],0,sizeof(sectortype));
                            sector[numsectors].extra = -1;

                            sector[numsectors].wallptr = numwalls;
                            sector[numsectors].wallnum = newnumwalls-numwalls;
                            sector[numsectors].ceilingz = (-32<<8);
                            sector[numsectors].floorz = (32<<8);
                            for (i=numwalls;i<newnumwalls;i++)
                            {
                                wall[i].cstat = 0;
                                wall[i].shade = 0;
                                wall[i].yrepeat = 8;
                                fixrepeats((short)i);
                                wall[i].picnum = 0;
                                wall[i].overpicnum = 0;
                                wall[i].nextsector = -1;
                                wall[i].nextwall = -1;
                            }
                            headspritesect[numsectors] = -1;
                            numsectors++;
                        }
                        else       //else add loop to sector
                        {
                            if (clockdir(numwalls) == 0)
                                flipwalls(numwalls,newnumwalls);

                            j = newnumwalls-numwalls;

                            sector[k].wallnum += j;
                            for (i=k+1;i<numsectors;i++)
                                sector[i].wallptr += j;
                            suckwall = sector[k].wallptr;

                            for (i=0;i<numwalls;i++)
                            {
                                if (wall[i].nextwall >= suckwall)
                                    wall[i].nextwall += j;
                                if (wall[i].point2 >= suckwall)
                                    wall[i].point2 += j;
                            }

                            for (i=newnumwalls-1;i>=suckwall;i--)
                                Bmemcpy(&wall[i+j],&wall[i],sizeof(walltype));
                            for (i=0;i<j;i++)
                                Bmemcpy(&wall[i+suckwall],&wall[i+newnumwalls],sizeof(walltype));

                            for (i=suckwall;i<suckwall+j;i++)
                            {
                                wall[i].point2 += (suckwall-numwalls);

                                wall[i].cstat = wall[suckwall+j].cstat&~(1+16+32+64);
                                wall[i].shade = wall[suckwall+j].shade;
                                wall[i].yrepeat = wall[suckwall+j].yrepeat;
                                fixrepeats((short)i);
                                wall[i].picnum = wall[suckwall+j].picnum;
                                wall[i].overpicnum = wall[suckwall+j].overpicnum;

                                wall[i].nextsector = -1;
                                wall[i].nextwall = -1;
                            }
                        }
                    }
                    else
                    {
                        //add new sector with connections
                        if (clockdir(numwalls) == 1)
                            flipwalls(numwalls,newnumwalls);

                        //clearbufbyte(&sector[numsectors],sizeof(sectortype),0L);
                        memset(&sector[numsectors],0,sizeof(sectortype));
                        sector[numsectors].extra = -1;

                        sector[numsectors].wallptr = numwalls;
                        sector[numsectors].wallnum = newnumwalls-numwalls;
                        sucksect = sectorofwall(suckwall);
                        sector[numsectors].ceilingstat = sector[sucksect].ceilingstat;
                        sector[numsectors].floorstat = sector[sucksect].floorstat;
                        sector[numsectors].ceilingxpanning = sector[sucksect].ceilingxpanning;
                        sector[numsectors].floorxpanning = sector[sucksect].floorxpanning;
                        sector[numsectors].ceilingshade = sector[sucksect].ceilingshade;
                        sector[numsectors].floorshade = sector[sucksect].floorshade;
                        sector[numsectors].ceilingz = sector[sucksect].ceilingz;
                        sector[numsectors].floorz = sector[sucksect].floorz;
                        sector[numsectors].ceilingpicnum = sector[sucksect].ceilingpicnum;
                        sector[numsectors].floorpicnum = sector[sucksect].floorpicnum;
                        sector[numsectors].ceilingheinum = sector[sucksect].ceilingheinum;
                        sector[numsectors].floorheinum = sector[sucksect].floorheinum;
                        for (i=numwalls;i<newnumwalls;i++)
                        {
                            wall[i].cstat = wall[suckwall].cstat;
                            wall[i].shade = wall[suckwall].shade;
                            wall[i].yrepeat = wall[suckwall].yrepeat;
                            fixrepeats((short)i);
                            wall[i].picnum = wall[suckwall].picnum;
                            wall[i].overpicnum = wall[suckwall].overpicnum;
                            checksectorpointer((short)i,(short)numsectors);
                        }
                        headspritesect[numsectors] = -1;
                        numsectors++;
                    }
                    numwalls = newnumwalls;
                    newnumwalls = -1;
                    asksave = 1;
                }
                if (split == 1)
                {
                    //split sector
                    startwall = sector[splitsect].wallptr;
                    endwall = startwall + sector[splitsect].wallnum - 1;
                    for (k=startwall;k<=endwall;k++)
                        if (wall[k].x == wall[newnumwalls-1].x)
                            if (wall[k].y == wall[newnumwalls-1].y)
                            {
                                bad = 0;
                                if (loopnumofsector(splitsect,splitstartwall) != loopnumofsector(splitsect,(short)k))
                                    bad = 1;

                                if (bad == 0)
                                {
                                    //SPLIT IT!
                                    //Split splitsect given: startwall,
                                    //   new points: numwalls to newnumwalls-2

                                    splitendwall = k;
                                    newnumwalls--;  //first fix up the new walls
                                    for (i=numwalls;i<newnumwalls;i++)
                                    {
                                        wall[i].cstat = wall[startwall].cstat;
                                        wall[i].shade = wall[startwall].shade;
                                        wall[i].yrepeat = wall[startwall].yrepeat;
                                        fixrepeats((short)i);
                                        wall[i].picnum = wall[startwall].picnum;
                                        wall[i].overpicnum = wall[startwall].overpicnum;

                                        wall[i].nextwall = -1;
                                        wall[i].nextsector = -1;
                                        wall[i].point2 = i+1;
                                    }

                                    danumwalls = newnumwalls;  //where to add more walls
                                    m = splitendwall;          //copy rest of loop next
                                    while (m != splitstartwall)
                                    {
                                        Bmemcpy(&wall[danumwalls],&wall[m],sizeof(walltype));
                                        wall[danumwalls].point2 = danumwalls+1;
                                        danumwalls++;
                                        m = wall[m].point2;
                                    }
                                    wall[danumwalls-1].point2 = numwalls;

                                    //Add other loops for 1st sector
                                    loopnum = loopnumofsector(splitsect,splitstartwall);
                                    i = loopnum;
                                    for (j=startwall;j<=endwall;j++)
                                    {
                                        k = loopnumofsector(splitsect,(short)j);
                                        if ((k != i) && (k != loopnum))
                                        {
                                            i = k;
                                            if (loopinside(wall[j].x,wall[j].y,numwalls) == 1)
                                            {
                                                m = j;          //copy loop
                                                k = danumwalls;
                                                do
                                                {
                                                    Bmemcpy(&wall[danumwalls],&wall[m],sizeof(walltype));
                                                    wall[danumwalls].point2 = danumwalls+1;
                                                    danumwalls++;
                                                    m = wall[m].point2;
                                                }
                                                while (m != j);
                                                wall[danumwalls-1].point2 = k;
                                            }
                                        }
                                    }

                                    secondstartwall = danumwalls;
                                    //copy split points for other sector backwards
                                    for (j=newnumwalls;j>numwalls;j--)
                                    {
                                        Bmemcpy(&wall[danumwalls],&wall[j],sizeof(walltype));
                                        wall[danumwalls].nextwall = -1;
                                        wall[danumwalls].nextsector = -1;
                                        wall[danumwalls].point2 = danumwalls+1;
                                        danumwalls++;
                                    }
                                    m = splitstartwall;     //copy rest of loop next
                                    while (m != splitendwall)
                                    {
                                        Bmemcpy(&wall[danumwalls],&wall[m],sizeof(walltype));
                                        wall[danumwalls].point2 = danumwalls+1;
                                        danumwalls++;
                                        m = wall[m].point2;
                                    }
                                    wall[danumwalls-1].point2 = secondstartwall;

                                    //Add other loops for 2nd sector
                                    loopnum = loopnumofsector(splitsect,splitstartwall);
                                    i = loopnum;
                                    for (j=startwall;j<=endwall;j++)
                                    {
                                        k = loopnumofsector(splitsect,(short)j);
                                        if ((k != i) && (k != loopnum))
                                        {
                                            i = k;
                                            if (loopinside(wall[j].x,wall[j].y,secondstartwall) == 1)
                                            {
                                                m = j;          //copy loop
                                                k = danumwalls;
                                                do
                                                {
                                                    Bmemcpy(&wall[danumwalls],&wall[m],sizeof(walltype));
                                                    wall[danumwalls].point2 = danumwalls+1;
                                                    danumwalls++;
                                                    m = wall[m].point2;
                                                }
                                                while (m != j);
                                                wall[danumwalls-1].point2 = k;
                                            }
                                        }
                                    }

                                    //fix all next pointers on old sector line
                                    for (j=numwalls;j<danumwalls;j++)
                                    {
                                        if (wall[j].nextwall >= 0)
                                        {
                                            wall[wall[j].nextwall].nextwall = j;
                                            if (j < secondstartwall)
                                                wall[wall[j].nextwall].nextsector = numsectors;
                                            else
                                                wall[wall[j].nextwall].nextsector = numsectors+1;
                                        }
                                    }
                                    //set all next pointers on split
                                    for (j=numwalls;j<newnumwalls;j++)
                                    {
                                        m = secondstartwall+(newnumwalls-1-j);
                                        wall[j].nextwall = m;
                                        wall[j].nextsector = numsectors+1;
                                        wall[m].nextwall = j;
                                        wall[m].nextsector = numsectors;
                                    }
                                    //copy sector attributes & fix wall pointers
                                    Bmemcpy(&sector[numsectors],&sector[splitsect],sizeof(sectortype));
                                    Bmemcpy(&sector[numsectors+1],&sector[splitsect],sizeof(sectortype));
                                    sector[numsectors].wallptr = numwalls;
                                    sector[numsectors].wallnum = secondstartwall-numwalls;
                                    sector[numsectors+1].wallptr = secondstartwall;
                                    sector[numsectors+1].wallnum = danumwalls-secondstartwall;

                                    //fix sprites
                                    j = headspritesect[splitsect];
                                    while (j != -1)
                                    {
                                        k = nextspritesect[j];
                                        if (loopinside(sprite[j].x,sprite[j].y,numwalls) == 1)
                                            changespritesect(j,numsectors);
                                        //else if (loopinside(sprite[j].x,sprite[j].y,secondstartwall) == 1)
                                        else  //Make sure no sprites get left out & deleted!
                                            changespritesect(j,numsectors+1);
                                        j = k;
                                    }

                                    numsectors+=2;

                                    //Back of number of walls of new sector for later
                                    k = danumwalls-numwalls;

                                    //clear out old sector's next pointers for clean deletesector
                                    numwalls = danumwalls;
                                    for (j=startwall;j<=endwall;j++)
                                    {
                                        wall[j].nextwall = -1;
                                        wall[j].nextsector = -1;
                                    }
                                    deletesector(splitsect);

                                    //Check pointers
                                    for (j=numwalls-k;j<numwalls;j++)
                                    {
                                        if (wall[j].nextwall >= 0)
                                            checksectorpointer(wall[j].nextwall,wall[j].nextsector);
                                        checksectorpointer((short)j,sectorofwall((short)j));
                                    }

                                    //k now safe to use as temp

                                    for (m=numsectors-2;m<numsectors;m++)
                                    {
                                        j = headspritesect[m];
                                        while (j != -1)
                                        {
                                            k = nextspritesect[j];
                                            setsprite(j,sprite[j].x,sprite[j].y,sprite[j].z);
                                            j = k;
                                        }
                                    }

                                    newnumwalls = -1;
                                    printmessage16("Sector split.");
                                    break;
                                }
                                else
                                {
                                    //Sector split - actually loop joining

                                    splitendwall = k;
                                    newnumwalls--;  //first fix up the new walls
                                    for (i=numwalls;i<newnumwalls;i++)
                                    {
                                        wall[i].cstat = wall[startwall].cstat;
                                        wall[i].shade = wall[startwall].shade;
                                        wall[i].yrepeat = wall[startwall].yrepeat;
                                        fixrepeats((short)i);
                                        wall[i].picnum = wall[startwall].picnum;
                                        wall[i].overpicnum = wall[startwall].overpicnum;

                                        wall[i].nextwall = -1;
                                        wall[i].nextsector = -1;
                                        wall[i].point2 = i+1;
                                    }

                                    danumwalls = newnumwalls;  //where to add more walls
                                    m = splitendwall;          //copy rest of loop next
                                    do
                                    {
                                        Bmemcpy(&wall[danumwalls],&wall[m],sizeof(walltype));
                                        wall[danumwalls].point2 = danumwalls+1;
                                        danumwalls++;
                                        m = wall[m].point2;
                                    }
                                    while (m != splitendwall);

                                    //copy split points for other sector backwards
                                    for (j=newnumwalls;j>numwalls;j--)
                                    {
                                        Bmemcpy(&wall[danumwalls],&wall[j],sizeof(walltype));
                                        wall[danumwalls].nextwall = -1;
                                        wall[danumwalls].nextsector = -1;
                                        wall[danumwalls].point2 = danumwalls+1;
                                        danumwalls++;
                                    }

                                    m = splitstartwall;     //copy rest of loop next
                                    do
                                    {
                                        Bmemcpy(&wall[danumwalls],&wall[m],sizeof(walltype));
                                        wall[danumwalls].point2 = danumwalls+1;
                                        danumwalls++;
                                        m = wall[m].point2;
                                    }
                                    while (m != splitstartwall);
                                    wall[danumwalls-1].point2 = numwalls;

                                    //Add other loops to sector
                                    loopnum = loopnumofsector(splitsect,splitstartwall);
                                    i = loopnum;
                                    for (j=startwall;j<=endwall;j++)
                                    {
                                        k = loopnumofsector(splitsect,(short)j);
                                        if ((k != i) && (k != loopnumofsector(splitsect,splitstartwall)) && (k != loopnumofsector(splitsect,splitendwall)))
                                        {
                                            i = k;
                                            m = j; k = danumwalls;     //copy loop
                                            do
                                            {
                                                Bmemcpy(&wall[danumwalls],&wall[m],sizeof(walltype));
                                                wall[danumwalls].point2 = danumwalls+1;
                                                danumwalls++;
                                                m = wall[m].point2;
                                            }
                                            while (m != j);
                                            wall[danumwalls-1].point2 = k;
                                        }
                                    }

                                    //fix all next pointers on old sector line
                                    for (j=numwalls;j<danumwalls;j++)
                                    {
                                        if (wall[j].nextwall >= 0)
                                        {
                                            wall[wall[j].nextwall].nextwall = j;
                                            wall[wall[j].nextwall].nextsector = numsectors;
                                        }
                                    }

                                    //copy sector attributes & fix wall pointers
                                    Bmemcpy(&sector[numsectors],&sector[splitsect],sizeof(sectortype));
                                    sector[numsectors].wallptr = numwalls;
                                    sector[numsectors].wallnum = danumwalls-numwalls;

                                    //fix sprites
                                    j = headspritesect[splitsect];
                                    while (j != -1)
                                    {
                                        k = nextspritesect[j];
                                        changespritesect(j,numsectors);
                                        j = k;
                                    }

                                    numsectors++;

                                    //Back of number of walls of new sector for later
                                    k = danumwalls-numwalls;

                                    //clear out old sector's next pointers for clean deletesector
                                    numwalls = danumwalls;
                                    for (j=startwall;j<=endwall;j++)
                                    {
                                        wall[j].nextwall = -1;
                                        wall[j].nextsector = -1;
                                    }
                                    deletesector(splitsect);

                                    //Check pointers
                                    for (j=numwalls-k;j<numwalls;j++)
                                    {
                                        if (wall[j].nextwall >= 0)
                                            checksectorpointer(wall[j].nextwall,wall[j].nextsector);
                                        checksectorpointer((short)j,numsectors-1);
                                    }

                                    newnumwalls = -1;
                                    printmessage16("Loops joined.");
                                    break;
                                }
                            }
                }
            }
        }

        if (keystatus[0x1c]) //Left Enter
        {
            keystatus[0x1c] = 0;
            if (keystatus[0x2a]&keystatus[0x1d])
            {
                printmessage16("CHECKING ALL POINTERS!");
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

        if (keystatus[0x0e] && (newnumwalls >= numwalls)) //Backspace
        {
            if (newnumwalls > numwalls)
            {
                newnumwalls--;
                asksave = 1;
                keystatus[0x0e] = 0;
            }
            if (newnumwalls == numwalls)
            {
                newnumwalls = -1;
                asksave = 1;
                keystatus[0x0e] = 0;
            }
        }

        if (keystatus[0xd3] && (keystatus[0x1d]||keystatus[0x9d]) && (numwalls >= 0))
        {
            //sector delete
            keystatus[0xd3] = 0;

            sucksect = -1;
            for (i=0;i<numsectors;i++)
                if (inside(mousxplc,mousyplc,i) == 1)
                {
                    k = 0;
                    if (highlightsectorcnt >= 0)
                        for (j=0;j<highlightsectorcnt;j++)
                            if (highlightsector[j] == i)
                            {
                                for (j=highlightsectorcnt-1;j>=0;j--)
                                {
                                    deletesector(highlightsector[j]);
                                    for (k=j-1;k>=0;k--)
                                        if (highlightsector[k] >= highlightsector[j])
                                            highlightsector[k]--;
                                }
                                printmessage16("Highlighted sectors deleted.");
                                newnumwalls = -1;
                                k = 1;
                                highlightsectorcnt = -1;
                                break;
                            }
                    if (k == 0)
                    {
                        deletesector((short)i);
                        highlightsectorcnt = -1;
                        printmessage16("Sector deleted.");
                    }
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
                asksave = 1;
            }
            keystatus[0xd3] = 0;
        }

        if (keystatus[0xd2] || keystatus[0x17])  //InsertPoint
        {
            if (highlightsectorcnt >= 0)
            {
                newnumsectors = numsectors;
                newnumwalls = numwalls;
                for (i=0;i<highlightsectorcnt;i++)
                {
                    copysector(highlightsector[i],newnumsectors,newnumwalls,1);
                    newnumsectors++;
                    newnumwalls += sector[highlightsector[i]].wallnum;
                }

                for (i=0;i<highlightsectorcnt;i++)
                {
                    startwall = sector[highlightsector[i]].wallptr;
                    endwall = startwall+sector[highlightsector[i]].wallnum-1;
                    for (j=startwall;j<=endwall;j++)
                    {
                        if (wall[j].nextwall >= 0)
                            checksectorpointer(wall[j].nextwall,wall[j].nextsector);
                        checksectorpointer((short)j,highlightsector[i]);
                    }
                    highlightsector[i] = numsectors+i;
                }
                numsectors = newnumsectors;
                numwalls = newnumwalls;

                newnumwalls = -1;
                newnumsectors = -1;

                updatenumsprites();
                printmessage16("Sectors duplicated and stamped.");
                asksave = 1;
            }
            else if (highlightcnt >= 0)
            {
                for (i=0;i<highlightcnt;i++)
                    if ((highlight[i]&0xc000) == 16384)
                    {
                        //duplicate sprite
                        k = (highlight[i]&16383);
                        j = insertsprite(sprite[k].sectnum,sprite[k].statnum);
                        Bmemcpy(&sprite[j],&sprite[k],sizeof(spritetype));
                        sprite[j].sectnum = sprite[k].sectnum;   //Don't let memcpy overwrite sector!
                        setsprite(j,sprite[j].x,sprite[j].y,sprite[j].z);
                    }
                updatenumsprites();
                printmessage16("Sprites duplicated and stamped.");
                asksave = 1;
            }
            else if (linehighlight >= 0)
            {
                int wallsdrawn = newnumwalls-numwalls;

                if (newnumwalls != -1)
                {
                    newnumwalls = -1;
                    Bmemcpy(&wall[MAXWALLS-wallsdrawn],&wall[numwalls],sizeof(walltype) * wallsdrawn);
                }
                else wallsdrawn = -1;

                getclosestpointonwall(mousxplc,mousyplc,(int)linehighlight,&dax,&day);
                adjustmark(&dax,&day,newnumwalls);
                insertpoint(linehighlight,dax,day);
                printmessage16("Point inserted.");

                j = 0;
                //Check to see if point was inserted over another point
                for (i=numwalls-1;i>=0;i--)    //delete points
                    if (wall[i].x == wall[wall[i].point2].x)
                        if (wall[i].y == wall[wall[i].point2].y)
                        {
                            deletepoint((short)i);
                            j++;
                        }
                for (i=0;i<numwalls;i++)       //make new red lines?
                {
                    if ((wall[i].x == dax) && (wall[i].y == day))
                    {
                        checksectorpointer((short)i,sectorofwall((short)i));
                        fixrepeats((short)i);
                    }
                    else if ((wall[wall[i].point2].x == dax) && (wall[wall[i].point2].y == day))
                    {
                        checksectorpointer((short)i,sectorofwall((short)i));
                        fixrepeats((short)i);
                    }
                }
                //if (j != 0)
                //{
                //   dax = ((wall[linehighlight].x + wall[wall[linehighlight].point2].x)>>1);
                //   day = ((wall[linehighlight].y + wall[wall[linehighlight].point2].y)>>1);
                //   if ((dax != wall[linehighlight].x) || (day != wall[linehighlight].y))
                //      if ((dax != wall[wall[linehighlight].point2].x) || (day != wall[wall[linehighlight].point2].y))
                //      {
                //         insertpoint(linehighlight,dax,day);
                //         printmessage16("Point inserted at midpoint.");
                //      }
                //}

                if (wallsdrawn != -1)
                {
                    Bmemcpy(&wall[numwalls],&wall[MAXWALLS-wallsdrawn],sizeof(walltype) * wallsdrawn);
                    newnumwalls = numwalls + wallsdrawn;
                    for (i=numwalls;i<newnumwalls;i++)
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
            updatesector(posx,posy,&cursectnum);
            if (cursectnum >= 0)
                keystatus[buildkeys[BK_MODE2D_3D]] = 2;
            else
                printmessage16("Arrow must be inside a sector before entering 3D mode.");
        }

// vvv PK ------------------------------------ (LShift) Ctrl-X: (prev) next map
// this is copied from 'L' (load map), but without copying the highlighted sectors

        if (quickmapcycling && (keystatus[0x2d] > 0))   //X
        {
            if ((keystatus[0x1d]|keystatus[0x9d]) > 0)  //Ctrl
            {
nextmap:
//				bad = 0;
                i = menuselect_pk(keystatus[0x2a]>0 ? 0:1); // Left Shift: prev map
                if (i < 0)
                {
                    if (i == -2)
                        printmessage16("No .MAP files found.");
                    else if (i == -3)
                        printmessage16("Load map first!");
                }
                else
                {
                    Bstrcpy(boardfilename, selectedboardfilename);

                    highlightcnt = -1;
                    sectorhighlightstat = -1;
                    newnumwalls = -1;
                    joinsector[0] = -1;
                    circlewall = -1;
                    circlepoints = 7;

                    for (i=0;i<MAXSECTORS;i++) sector[i].extra = -1;
                    for (i=0;i<MAXWALLS;i++) wall[i].extra = -1;
                    for (i=0;i<MAXSPRITES;i++) sprite[i].extra = -1;

                    ExtPreLoadMap();
                    i = loadboard(boardfilename,(!pathsearchmode&&grponlymode?2:0),&posx,&posy,&posz,&ang,&cursectnum);
                    if (i == -2) i = loadoldboard(boardfilename,(!pathsearchmode&&grponlymode?2:0),&posx,&posy,&posz,&ang,&cursectnum);
                    oposz = posz;
                    if (i < 0)
                    {
//                        printmessage16("Invalid map format.");
                        goto nextmap;
                    }
                    else
                    {
                        ExtLoadMap(boardfilename);

                        if (mapversion < 7) printmessage16("Map %s loaded successfully and autoconverted to V7!",boardfilename);
                        else printmessage16("Map %s loaded successfully.",boardfilename);
                    }
                    updatenumsprites();
                    startposx = posx;      //this is same
                    startposy = posy;
                    startposz = posz;
                    startang = ang;
                    startsectnum = cursectnum;
                }
                showframe(1);
                keystatus[0x1c] = 0;

                keystatus[0x2d]=keystatus[0x13]=0;

            }
        }
// ^^^ PK ------------------------------------

CANCEL:
        if (keystatus[1] && joinsector[0] >= 0) {keystatus[1]=0;joinsector[0]=-1;}
        if (keystatus[1])
        {
            keystatus[1] = 0;
            printmessage16("(N)ew, (L)oad, (S)ave, save (A)s, (Q)uit");
//            printext16(200L+248, ydim-STATUS2DSIZ+20L, 9, 0, "(U)ndo, (R)edo", 0);
            showframe(1);
            bflushchars();
            bad = 1;
            while (bad == 1)
            {
                if (handleevents())
                {
                    if (quitevent)
                    {
                        quitevent = 0;
                    }
                }
                idle();

                ch = bgetchar();

                if (keystatus[1])
                {
                    keystatus[1] = 0;
                    bad = 0;
                    printmessage16("");
                }
                else if (ch == 'n' || ch == 'N')  //N
                {
                    bad = 0;
                    printmessage16("Are you sure you want to start a new board? (Y/N)");
                    showframe(1);
                    bflushchars(); ch = 0;
                    while (keystatus[1] == 0)
                    {
                        if (handleevents())
                        {
                            if (quitevent)
                            {
                                quitevent = 0;
                            }
                        }
                        idle();

                        ch = bgetchar();

                        if (ch == 'Y' || ch == 'y')
                        {
                            highlightsectorcnt = -1;
                            highlightcnt = -1;

                            for (i=0;i<(MAXWALLS>>3);i++)  //Clear all highlights
                                show2dwall[i] = 0;
                            for (i=0;i<(MAXSPRITES>>3);i++)
                                show2dsprite[i] = 0;

                            for (i=0;i<MAXSECTORS;i++) sector[i].extra = -1;
                            for (i=0;i<MAXWALLS;i++) wall[i].extra = -1;
                            for (i=0;i<MAXSPRITES;i++) sprite[i].extra = -1;

                            sectorhighlightstat = -1;
                            newnumwalls = -1;
                            joinsector[0] = -1;
                            circlewall = -1;
                            circlepoints = 7;

                            posx = 32768;          //new board!
                            posy = 32768;
                            posz = 0;
                            ang = 1536;
                            numsectors = 0;
                            numwalls = 0;
                            numsprites = 0;
                            cursectnum = -1;
                            initspritelists();
                            Bstrcpy(boardfilename,"newboard.map");
                            break;
                        }
                        else if (ch == 'N' || ch == 'n' || ch == 13 || ch == ' ')
                        {
                            break;
                        }
                    }
                    printmessage16("");
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
                        Bstrcpy(boardfilename, selectedboardfilename);

                        if (highlightsectorcnt >= 0)
                        {
                            j = 0; k = 0;
                            for (i=0;i<highlightsectorcnt;i++)
                            {
                                j += sector[highlightsector[i]].wallnum;

                                m = headspritesect[highlightsector[i]];
                                while (m != -1)
                                {
                                    k++;
                                    m = nextspritesect[m];
                                }
                            }

                            updatenumsprites();
                            if ((numsectors+highlightsectorcnt > MAXSECTORS) || (numwalls+j > MAXWALLS) || (numsprites+k > MAXSPRITES))
                            {
                                highlightsectorcnt = -1;
                            }
                            else
                            {
                                //Put sectors&walls to end of lists
                                j = MAXWALLS;
                                for (i=0;i<highlightsectorcnt;i++)
                                {
                                    j -= sector[highlightsector[i]].wallnum;
                                    copysector(highlightsector[i],(short)(MAXSECTORS-highlightsectorcnt+i),(short)j,0);
                                }

                                //Put sprites to end of list
                                //DONT USE m BETWEEN HERE AND SPRITE RE-ATTACHING!
                                m = MAXSPRITES;
                                for (i=MAXSPRITES-1;i>=0;i--)
                                    if (sprite[i].statnum < MAXSTATUS)
                                    {
                                        k = sprite[i].sectnum;
                                        for (j=0;j<highlightsectorcnt;j++)
                                            if (highlightsector[j] == k)
                                            {
                                                m--;
                                                if (i != m)
                                                    Bmemcpy(&sprite[m],&sprite[i],sizeof(spritetype));

                                                //HACK - THESE 2 buffers back up .sectnum and .statnum
                                                //for initspritelists() inside the loadboard call
                                                //tsprite[m].picnum = MAXSECTORS-highlightsectorcnt+j;
                                                //tsprite[m].owner = sprite[i].statnum;

                                                // JBF: I see your hack and raise you another
                                                spriteext[m].mdanimcur = MAXSECTORS-highlightsectorcnt+j;
                                                spriteext[m].angoff = sprite[i].statnum;

                                                break;
                                            }
                                    }
                            }
                        }

                        highlightcnt = -1;
                        sectorhighlightstat = -1;
                        newnumwalls = -1;
                        joinsector[0] = -1;
                        circlewall = -1;
                        circlepoints = 7;

                        for (i=0;i<MAXSECTORS;i++) sector[i].extra = -1;
                        for (i=0;i<MAXWALLS;i++) wall[i].extra = -1;
                        for (i=0;i<MAXSPRITES;i++) sprite[i].extra = -1;

                        ExtPreLoadMap();
                        i = loadboard(boardfilename,(!pathsearchmode&&grponlymode?2:0),&posx,&posy,&posz,&ang,&cursectnum);
                        loadmhk();
                        if (i == -2) i = loadoldboard(boardfilename,(!pathsearchmode&&grponlymode?2:0),&posx,&posy,&posz,&ang,&cursectnum);
                        oposz = posz;
                        if (i < 0)
                        {
                            printmessage16("Invalid map format.");
                        }
                        else
                        {
                            ExtLoadMap(boardfilename);

                            if (highlightsectorcnt >= 0)
                            {
                                if ((numsectors+highlightsectorcnt > MAXSECTORS) || (sector[MAXSECTORS-highlightsectorcnt].wallptr < numwalls))
                                {
                                    highlightsectorcnt = -1;
                                }
                                else
                                {
                                    //Re-attach sectors&walls
                                    for (i=0;i<highlightsectorcnt;i++)
                                    {
                                        copysector((short)(MAXSECTORS-highlightsectorcnt+i),numsectors,numwalls,0);
                                        highlightsector[i] = numsectors;
                                        numwalls += sector[numsectors].wallnum;
                                        numsectors++;
                                    }
                                    //Re-attach sprites
                                    while (m < MAXSPRITES)
                                    {
                                        //HACK - THESE 2 buffers back up .sectnum and .statnum
                                        //for initspritelists() inside the loadboard call
                                        //tsprite[m].picnum = sprite[i].sectnum;
                                        //tsprite[m].owner = sprite[i].statnum;

                                        j = insertsprite(spriteext[m].mdanimcur+(numsectors-MAXSECTORS),spriteext[m].angoff);
                                        Bmemcpy(&sprite[j],&sprite[m],sizeof(spritetype));
                                        //sprite[j].sectnum = tsprite[m].picnum+(numsectors-MAXSECTORS);
                                        //sprite[j].statnum = tsprite[m].owner;

                                        // JBF: I see your hack and raise you another
                                        sprite[j].sectnum = spriteext[m].mdanimcur+(numsectors-MAXSECTORS);
                                        sprite[j].statnum = spriteext[m].angoff;
                                        spriteext[m].mdanimcur = spriteext[m].angoff = 0;

                                        m++;
                                    }

                                    for (i=0;i<highlightsectorcnt;i++)
                                    {
                                        startwall = sector[highlightsector[i]].wallptr;
                                        endwall = startwall+sector[highlightsector[i]].wallnum-1;
                                        for (j=startwall;j<=endwall;j++)
                                        {
                                            if (wall[j].nextwall >= 0)
                                                checksectorpointer(wall[j].nextwall,wall[j].nextsector);
                                            checksectorpointer((short)j,highlightsector[i]);
                                        }
                                    }

                                }
                            }

                            if (mapversion < 7) printmessage16("Map %s loaded successfully and autoconverted to V7!",boardfilename);
                            else printmessage16("Map %s loaded successfully.",boardfilename);
                        }
                        updatenumsprites();
                        startposx = posx;      //this is same
                        startposy = posy;
                        startposz = posz;
                        startang = ang;
                        startsectnum = cursectnum;
                    }
                    showframe(1);
                    keystatus[0x1c] = 0;
                }
                else if (ch == 'a' || ch == 'A')  //A
                {
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
                        Bsprintf(buffer,"Save as: ^011%s", boardfilename);
                        if (totalclock & 32)
                            Bstrcat(buffer,"_");
                        printmessage16(buffer);
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
                    if (bad == 2)
                    {
                        char *f;int res;
                        keystatus[0x1c] = 0;

                        boardfilename[i] = '.';
                        boardfilename[i+1] = 'm';
                        boardfilename[i+2] = 'a';
                        boardfilename[i+3] = 'p';
                        boardfilename[i+4] = 0;

                        if (Bstrrchr(selectedboardfilename,'/'))
                            Bstrcpy(Bstrrchr(selectedboardfilename, '/')+1, boardfilename);
                        else
                            Bstrcpy(selectedboardfilename, boardfilename);
                        if (pathsearchmode) f = selectedboardfilename;
                        else
                        {
                            // virtual filesystem mode can't save to directories so drop the file into
                            // the current directory
                            f = strrchr(selectedboardfilename, '/');
                            if (!f) f = selectedboardfilename; else f++;
                        }
                        Bsprintf(buffer,"Saving to %s...",f);
                        printmessage16(buffer);
                        showframe(1);

                        fixspritesectors();   //Do this before saving!
                        updatesector(startposx,startposy,&startsectnum);
                        ExtPreSaveMap();
                        res=saveboard(f,&startposx,&startposy,&startposz,&startang,&startsectnum);
                        ExtSaveMap(f);
                        printmessage16((res==0)?"Board saved.":"Saving board failed.");
                        Bstrcpy(boardfilename, selectedboardfilename);
                    }
                    bad = 0;
                }
                else if (ch == 's' || ch == 'S')  //S
                {
                    char *f;
                    int res;
                    bad = 0;
                    printmessage16("Saving board...");
                    showframe(1);
                    fixspritesectors();   //Do this before saving!
                    updatesector(startposx,startposy,&startsectnum);
                    if (pathsearchmode) f = boardfilename;
                    else
                    {
                        // virtual filesystem mode can't save to directories so drop the file into
                        // the current directory
                        f = strrchr(boardfilename, '/');
                        if (!f) f = boardfilename; else f++;
                    }
                    ExtPreSaveMap();
                    res=saveboard(f,&startposx,&startposy,&startposz,&startang,&startsectnum);
                    ExtSaveMap(f);
                    printmessage16((res==0)?"Board saved.":"Saving board failed.");
                    showframe(1);
                }
                else if (ch == 'q' || ch == 'Q')  //Q
                {
                    bad = 0;
                    printmessage16("Are you sure you want to quit?");
                    showframe(1);
                    bflushchars();
                    while ((keystatus[1]|keystatus[0x2e]) == 0)
                    {
                        if (handleevents())
                        {
                            if (quitevent) quitevent = 0;
                        }
                        idle();

                        ch = bgetchar();

                        if (ch == 'y' || ch == 'Y')
                        {
                            //QUIT!

                            printmessage16("Save changes?");
                            showframe(1);
                            while ((keystatus[1]|keystatus[0x2e]) == 0)
                            {
                                if (handleevents())
                                {
                                    if (quitevent) break;	// like saying no
                                }
                                idle();

                                ch = bgetchar();

                                if (ch == 'y' || ch == 'Y')
                                {
                                    char *f;
                                    fixspritesectors();   //Do this before saving!
                                    updatesector(startposx,startposy,&startsectnum);
                                    ExtPreSaveMap();
                                    if (pathsearchmode) f = boardfilename;
                                    else
                                    {
                                        // virtual filesystem mode can't save to directories so drop the file into
                                        // the current directory
                                        f = strrchr(boardfilename, '/');
                                        if (!f) f = boardfilename; else f++;
                                    }
                                    saveboard(f,&startposx,&startposy,&startposz,&startang,&startsectnum);
                                    ExtSaveMap(f);
                                    break;
                                }
                                else if (ch == 'n' || ch == 'N' || ch == 13 || ch == ' ')
                                {
                                    break;
                                }
                            }
                            while (keystatus[1] || keystatus[0x2e])
                            {
                                keystatus[1] = 0;
                                keystatus[0x2e] = 0;
                                quitevent = 0;
                                printmessage16("Operation cancelled");
                                showframe(1);
                                goto CANCEL;
                            }
                            clearfilenames();
                            uninittimer();
                            uninitinput();
                            ExtUnInit();
                            uninitengine();
                            exit(0);
                        }
                        else if (ch == 'n' || ch == 'N' || ch == 13 || ch == ' ')
                        {
                            break;
                        }
                    }
                    printmessage16("");
                    showframe(1);
                }
            }
            clearkeys();
        }

        //nextpage();
    }

    for (i=0;i<highlightsectorcnt;i++)
    {
        startwall = sector[highlightsector[i]].wallptr;
        endwall = startwall+sector[highlightsector[i]].wallnum-1;
        for (j=startwall;j<=endwall;j++)
        {
            if (wall[j].nextwall >= 0)
                checksectorpointer(wall[j].nextwall,wall[j].nextsector);
            checksectorpointer((short)j,highlightsector[i]);
        }
    }

    fixspritesectors();

    if (setgamemode(fullscreen,xdimgame,ydimgame,bppgame) < 0)
    {
        ExtUnInit();
        uninitinput();
        uninittimer();
        initprintf("%d * %d not supported in this graphics mode\n",xdim,ydim);
        uninitsystem();
        clearfilenames();
        exit(0);
    }

    setbrightness(brightness,palette,0);

    posz = oposz;
    searchx = scale(searchx,xdimgame,xdim2d);
    searchy = scale(searchy,ydimgame,ydim2d-STATUS2DSIZ);
}

void getpoint(int searchxe, int searchye, int *x, int *y)
{
    if (posx <= -editorgridextent) posx = -editorgridextent;
    if (posx >= editorgridextent) posx = editorgridextent;
    if (posy <= -editorgridextent) posy = -editorgridextent;
    if (posy >= editorgridextent) posy = editorgridextent;

    *x = posx + divscale14(searchxe-halfxdim16,zoom);
    *y = posy + divscale14(searchye-midydim16,zoom);

    if (*x <= -editorgridextent) *x = -editorgridextent;
    if (*x >= editorgridextent) *x = editorgridextent;
    if (*y <= -editorgridextent) *y = -editorgridextent;
    if (*y >= editorgridextent) *y = editorgridextent;
}

int getlinehighlight(int xplc, int yplc, int line)
{
    int i, dst, dist, closest, x1, y1, x2, y2, nx, ny;

    if (numwalls == 0)
        return(-1);

    if (mouseb & 1) return line;

    dist = 1024;
    closest = -1;
    for (i=0;i<numwalls;i++)
    {
        getclosestpointonwall(xplc,yplc,i,&nx,&ny);
        dst = klabs(xplc-nx)+klabs(yplc-ny);
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
        x2 = wall[wall[closest].point2].x;
        y2 = wall[wall[closest].point2].y;
        if (dmulscale32(xplc-x1,y2-y1,-(x2-x1),yplc-y1) >= 0)
            closest = wall[closest].nextwall;
    }
    if ((pointhighlight&0xc000) == 16384) return (-1);
    return(closest);
}

int getpointhighlight(int xplc, int yplc, int point)
{
    int i, dst, dist = 512, closest = -1;

    if (numwalls == 0)
        return(-1);

    if (mouseb & 1) return point;

    if (grid < 1)
        dist = 0;

    for (i=0;i<numwalls;i++)
    {
        dst = klabs(xplc-wall[i].x) + klabs(yplc-wall[i].y);
        if (dst <= dist)
            dist = dst, closest = i;
    }
    if (zoom >= 256)
        for (i=0;i<MAXSPRITES;i++)
            if (sprite[i].statnum < MAXSTATUS)
            {
                dst = klabs(xplc-sprite[i].x) + klabs(yplc-sprite[i].y);
                // was (dst <= dist), but this way, when duplicating sprites,
                // the selected ones are dragged first
                if (dst < dist)
                    dist = dst, closest = i+16384;
            }
    return(closest);
}

int adjustmark(int *xplc, int *yplc, short danumwalls)
{
    int i, dst, dist, dax, day, pointlockdist;

    if (danumwalls < 0)
        danumwalls = numwalls;

    pointlockdist = 0;
    if ((grid > 0) && (gridlock > 0))
        pointlockdist = (128>>grid);

    dist = pointlockdist;
    dax = *xplc;
    day = *yplc;
    for (i=0;i<danumwalls;i++)
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
        if ((gridlock > 0) && (grid > 0))
        {
            dax = ((dax+(1024>>grid))&(0xffffffff<<(11-grid)));
            day = ((day+(1024>>grid))&(0xffffffff<<(11-grid)));
        }

    *xplc = dax;
    *yplc = day;
    return(0);
}

int checkautoinsert(int dax, int day, short danumwalls)
{
    int i, x1, y1, x2, y2;

    if (danumwalls < 0)
        danumwalls = numwalls;
    for (i=0;i<danumwalls;i++)      // Check if a point should be inserted
    {
        x1 = wall[i].x;
        y1 = wall[i].y;
        x2 = wall[wall[i].point2].x;
        y2 = wall[wall[i].point2].y;

        if ((x1 != dax) || (y1 != day))
            if ((x2 != dax) || (y2 != day))
                if (((x1 <= dax) && (dax <= x2)) || ((x2 <= dax) && (dax <= x1)))
                    if (((y1 <= day) && (day <= y2)) || ((y2 <= day) && (day <= y1)))
                        if ((dax-x1)*(y2-y1) == (day-y1)*(x2-x1))
                            return(1);          //insertpoint((short)i,dax,day);
    }
    return(0);
}

int clockdir(short wallstart)   //Returns: 0 is CW, 1 is CCW
{
    short i, themin;
    int minx, tempint, x0, x1, x2, y0, y1, y2;

    minx = 0x7fffffff;
    themin = -1;
    i = wallstart-1;
    do
    {
        i++;
        if (wall[wall[i].point2].x < minx)
        {
            minx = wall[wall[i].point2].x;
            themin = i;
        }
    }
    while ((wall[i].point2 != wallstart) && (i < MAXWALLS));

    x0 = wall[themin].x;
    y0 = wall[themin].y;
    x1 = wall[wall[themin].point2].x;
    y1 = wall[wall[themin].point2].y;
    x2 = wall[wall[wall[themin].point2].point2].x;
    y2 = wall[wall[wall[themin].point2].point2].y;

    if ((y1 >= y2) && (y1 <= y0)) return(0);
    if ((y1 >= y0) && (y1 <= y2)) return(1);

    tempint = (x0-x1)*(y2-y1) - (x2-x1)*(y0-y1);
    if (tempint < 0)
        return(0);
    else
        return(1);
}

void flipwalls(short numwalls, short newnumwalls)
{
    int i, j, nume, tempint;

    nume = newnumwalls-numwalls;

    for (i=numwalls;i<numwalls+(nume>>1);i++)
    {
        j = numwalls+newnumwalls-i-1;
        tempint = wall[i].x; wall[i].x = wall[j].x; wall[j].x = tempint;
        tempint = wall[i].y; wall[i].y = wall[j].y; wall[j].y = tempint;
    }
}

void insertpoint(short linehighlight, int dax, int day)
{
    short sucksect;
    int i, j, k;

    j = linehighlight;
    sucksect = sectorofwall((short)j);

    sector[sucksect].wallnum++;
    for (i=sucksect+1;i<numsectors;i++)
        sector[i].wallptr++;

    movewalls((int)j+1,+1L);
    Bmemcpy(&wall[j+1],&wall[j],sizeof(walltype));

    wall[j].point2 = j+1;
    wall[j+1].x = dax;
    wall[j+1].y = day;
    fixrepeats((short)j);
    fixrepeats((short)j+1);

    if (wall[j].nextwall >= 0)
    {
        k = wall[j].nextwall;

        sucksect = sectorofwall((short)k);

        sector[sucksect].wallnum++;
        for (i=sucksect+1;i<numsectors;i++)
            sector[i].wallptr++;

        movewalls((int)k+1,+1L);
        Bmemcpy(&wall[k+1],&wall[k],sizeof(walltype));

        wall[k].point2 = k+1;
        wall[k+1].x = dax;
        wall[k+1].y = day;
        fixrepeats((short)k);
        fixrepeats((short)k+1);

        j = wall[k].nextwall;
        wall[j].nextwall = k+1;
        wall[j+1].nextwall = k;
        wall[k].nextwall = j+1;
        wall[k+1].nextwall = j;
    }
}

void deletepoint(short point)
{
    int i, j, k, sucksect;

    sucksect = sectorofwall(point);

    sector[sucksect].wallnum--;
    for (i=sucksect+1;i<numsectors;i++)
        sector[i].wallptr--;

    j = lastwall(point);
    k = wall[point].point2;
    wall[j].point2 = k;

    if (wall[j].nextwall >= 0)
    {
        wall[wall[j].nextwall].nextwall = -1;
        wall[wall[j].nextwall].nextsector = -1;
    }
    if (wall[point].nextwall >= 0)
    {
        wall[wall[point].nextwall].nextwall = -1;
        wall[wall[point].nextwall].nextsector = -1;
    }
    movewalls((int)point,-1L);

    checksectorpointer((short)j,(short)sucksect);
}

int deletesector(short sucksect)
{
    int i, j, k, nextk, startwall, endwall;

    while (headspritesect[sucksect] >= 0)
        deletesprite(headspritesect[sucksect]);
    updatenumsprites();

    startwall = sector[sucksect].wallptr;
    endwall = startwall + sector[sucksect].wallnum - 1;
    j = sector[sucksect].wallnum;

    for (i=sucksect;i<numsectors-1;i++)
    {
        k = headspritesect[i+1];
        while (k != -1)
        {
            nextk = nextspritesect[k];
            changespritesect((short)k,(short)i);
            k = nextk;
        }

        Bmemcpy(&sector[i],&sector[i+1],sizeof(sectortype));
        sector[i].wallptr -= j;
    }
    numsectors--;

    j = endwall-startwall+1;
    for (i=startwall;i<=endwall;i++)
        if (wall[i].nextwall != -1)
        {
            wall[wall[i].nextwall].nextwall = -1;
            wall[wall[i].nextwall].nextsector = -1;
        }
    movewalls(startwall,-j);
    for (i=0;i<numwalls;i++)
        if (wall[i].nextwall >= startwall)
            wall[i].nextsector--;
    return(0);
}

void fixspritesectors(void)
{
    int i, j, dax, day, daz;

    for (i=numsectors-1;i>=0;i--)
        if ((sector[i].wallnum <= 0) || (sector[i].wallptr >= numwalls))
            deletesector((short)i);

    for (i=0;i<MAXSPRITES;i++)
        if (sprite[i].statnum < MAXSTATUS)
        {
            dax = sprite[i].x;
            day = sprite[i].y;
            if (inside(dax,day,sprite[i].sectnum) != 1)
            {
                daz = ((tilesizy[sprite[i].picnum]*sprite[i].yrepeat)<<2);

                for (j=0;j<numsectors;j++)
                    if (inside(dax,day,(short)j) == 1)
                        if (sprite[i].z >= getceilzofslope(j,dax,day))
                            if (sprite[i].z-daz <= getflorzofslope(j,dax,day))
                            {
                                changespritesect((short)i,(short)j);
                                break;
                            }
            }
        }
}

int movewalls(int start, int offs)
{
    int i;

    if (offs < 0)  //Delete
    {
        for (i=start;i<numwalls+offs;i++)
            Bmemcpy(&wall[i],&wall[i-offs],sizeof(walltype));
    }
    else if (offs > 0)  //Insert
    {
        for (i=numwalls+offs-1;i>=start+offs;i--)
            Bmemcpy(&wall[i],&wall[i-offs],sizeof(walltype));
    }
    numwalls += offs;
    for (i=0;i<numwalls;i++)
    {
        if (wall[i].nextwall >= start) wall[i].nextwall += offs;
        if (wall[i].point2 >= start) wall[i].point2 += offs;
    }
    return(0);
}

int checksectorpointer(short i, short sectnum)
{
    int j, k, startwall, endwall, x1, y1, x2, y2;

    x1 = wall[i].x;
    y1 = wall[i].y;
    x2 = wall[wall[i].point2].x;
    y2 = wall[wall[i].point2].y;

    if (wall[i].nextwall >= 0)          //Check for early exit
    {
        k = wall[i].nextwall;
        if ((wall[k].x == x2) && (wall[k].y == y2))
            if ((wall[wall[k].point2].x == x1) && (wall[wall[k].point2].y == y1))
                return(0);
    }

    wall[i].nextsector = -1;
    wall[i].nextwall = -1;
    for (j=0;j<numsectors;j++)
    {
        startwall = sector[j].wallptr;
        endwall = startwall + sector[j].wallnum - 1;
        for (k=startwall;k<=endwall;k++)
        {
            if ((wall[k].x == x2) && (wall[k].y == y2))
                if ((wall[wall[k].point2].x == x1) && (wall[wall[k].point2].y == y1))
                    if (j != sectnum)
                    {
                        wall[i].nextsector = j;
                        wall[i].nextwall = k;
                        wall[k].nextsector = sectnum;
                        wall[k].nextwall = i;
                    }
        }
    }
    return(0);
}

void fixrepeats(short i)
{
    int dax, day, dist;

    dax = wall[wall[i].point2].x-wall[i].x;
    day = wall[wall[i].point2].y-wall[i].y;
    dist = ksqrt(dax*dax+day*day);
    dax = wall[i].xrepeat; day = wall[i].yrepeat;
    wall[i].xrepeat = (char)min(max(mulscale10(dist,day),1),255);
}


int overridepm16y = -1;

void clearmidstatbar16(void)
{
    begindrawing();
    ydim16 = ydim;
    //  clearbuf((char *)(frameplace + (bytesperline*(ydim-STATUS2DSIZ+25L))),(bytesperline*(STATUS2DSIZ-1-(25<<1))) >> 2, 0x08080808l);
    if (overridepm16y < 0)
        clearbuf((char *)(frameplace + (bytesperline*(ydim-STATUS2DSIZ+25L))),(bytesperline*(STATUS2DSIZ+2-(25<<1))) >> 2, 0x00000000l);
    else
        clearbuf((char *)(frameplace + (bytesperline*(ydim-overridepm16y+25L))),(bytesperline*(overridepm16y+2-(25<<1))) >> 2, 0x00000000l);
    drawline16(0,ydim-STATUS2DSIZ,0,ydim-1,7);
    drawline16(xdim-1,ydim-STATUS2DSIZ,xdim-1,ydim-1,7);
    ydim16 = ydim-STATUS2DSIZ;
    enddrawing();
}

short loopinside(int x, int y, short startwall)
{
    int x1, y1, x2, y2, tempint;
    short i, cnt;

    cnt = clockdir(startwall);
    i = startwall;
    do
    {
        x1 = wall[i].x; x2 = wall[wall[i].point2].x;
        if ((x1 >= x) || (x2 >= x))
        {
            y1 = wall[i].y; y2 = wall[wall[i].point2].y;
            if (y1 > y2)
            {
                tempint = x1, x1 = x2, x2 = tempint;
                tempint = y1, y1 = y2, y2 = tempint;
            }
            if ((y1 <= y) && (y2 > y))
                if (x1*(y-y2)+x2*(y1-y) <= x*(y1-y2))
                    cnt ^= 1;
        }
        i = wall[i].point2;
    }
    while (i != startwall);
    return(cnt);
}

int numloopsofsector(short sectnum)
{
    int i, numloops, startwall, endwall;

    numloops = 0;
    startwall = sector[sectnum].wallptr;
    endwall = startwall + sector[sectnum].wallnum;
    for (i=startwall;i<endwall;i++)
        if (wall[i].point2 < i) numloops++;
    return(numloops);
}

int _getnumber16(char *namestart, int num, int maxnumber, char sign, void *(func)(int))
{
    char buffer[80], ch;
    int n, danum, oldnum;

    danum = (int)num;
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
        if (totalclock & 32) Bstrcat(buffer,"_ ");
        printmessage16(buffer);
        if (func != NULL)
        {
            Bsprintf(buffer,"^011%s",(char *)func((int)danum));
            printext16(200L-24, ydim-STATUS2DSIZ+20L, 9, 0, buffer, 0);
        }

        showframe(1);

        if (ch >= '0' && ch <= '9')
        {
            if (danum >= 0)
            {
                n = (danum*10)+(ch-'0');
                if (n <= maxnumber) danum = n;
            }
            else if (sign) // this extra check isn't hurting anything
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
    }
    clearkeys();
    return(oldnum);
}

int _getnumber256(char *namestart, int num, int maxnumber, char sign, void *(func)(int))
{
    char buffer[80], ch;
    int n, danum, oldnum;

    danum = num;
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
    }
    clearkeys();

    lockclock = totalclock;  //Reset timing

    return(oldnum);
}

void clearfilenames(void)
{
    klistfree(finddirs);
    klistfree(findfiles);
    finddirs = findfiles = NULL;
    numfiles = numdirs = 0;
}

int getfilenames(char *path, char *kind)
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

const char *g_oldpath=NULL;
int menuselect_pk(int direction) // 20080104: jump to next (direction!=0) or prev (direction==0) file
{
    const char *chptr;

    if (!g_oldpath) return -3;
    else Bmemcpy(selectedboardfilename, g_oldpath, BMAX_PATH);

    if (pathsearchmode)
        Bcanonicalisefilename(selectedboardfilename, 1);  // clips off the last token and compresses relative path
    else
        Bcorrectfilename(selectedboardfilename, 1);

    getfilenames(selectedboardfilename, "*.map");

    chptr = strrchr(boardfilename,'/'); // PK
    if (!chptr) chptr=boardfilename; else chptr++;
    for (; findfileshigh; findfileshigh=findfileshigh->next)
    {
        if (!Bstrcmp(findfileshigh->name,chptr)) break;
    }

    if (!findfileshigh) findfileshigh=findfiles;

    if (direction)
    {
        if (findfileshigh->next) findfileshigh=findfileshigh->next;
    }
    else
    {
        if (findfileshigh->prev) findfileshigh=findfileshigh->prev;
    }

    Bstrcat(selectedboardfilename, findfileshigh->name);

    return(0);
}
// ^^^ PK ------------------------------------

int menuselect(void)
{
    int listsize = (ydim16-32)/8;
    int i;
    char ch, buffer[96], /*PK*/ *chptr;
    static char oldpath[BMAX_PATH];
    CACHE1D_FIND_REC *dir;
    int bakpathsearchmode = pathsearchmode;

    g_oldpath=oldpath; //PK: need it in menuselect_pk

    Bstrcpy(selectedboardfilename, oldpath);
    if (pathsearchmode)
        Bcanonicalisefilename(selectedboardfilename, 1);		// clips off the last token and compresses relative path
    else
        Bcorrectfilename(selectedboardfilename, 1);

    getfilenames(selectedboardfilename, "*.map");

    // PK 20080103: start with last selected map
    chptr = strrchr(boardfilename,'/');
    if (!chptr) chptr=boardfilename; else chptr++;
    for (; findfileshigh; findfileshigh=findfileshigh->next)
    {
        if (!Bstrcmp(findfileshigh->name,chptr)) break;
    }
    if (!findfileshigh) findfileshigh=findfiles;

    begindrawing();
    printmessage16("Select map file with arrow keys and enter.");
    enddrawing();

    do
    {
        begindrawing();
        clearbuf((char *)frameplace, (bytesperline*ydim16) >> 2, 0l);

        if (pathsearchmode)
        {
            strcpy(buffer,"Local filesystem mode; press F for game filesystem.");
        }
        else
        {
            sprintf(buffer,"Game filesystem %smode; press F for local filesystem or G for %s.",
                    grponlymode?"GRP-only ":"", grponlymode?"all files":"GRP contents only");
        }
        printext16(halfxdim16-(8*strlen(buffer)/2), 4, 12,0,buffer,0);

        Bsnprintf(buffer,sizeof(buffer)-1,"(%d dirs, %d files) %s",numdirs,numfiles,selectedboardfilename);
        buffer[sizeof(buffer)-1] = 0;
        printext16(1,ydim16-8-1,8,0,buffer,0);

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
                int c = (dir->type == CACHE1D_FIND_DIR ? 2 : 3); //PK
                memset(buffer,0,sizeof(buffer));
                strncpy(buffer,dir->name,25);
                if (strlen(buffer) == 25)
                    buffer[21] = buffer[22] = buffer[23] = '.', buffer[24] = 0;
                if (dir == finddirshigh)
                {
                    if (currentlist == 0) printext16(8,16+8*i,c|8,0,"->",0);
                    printext16(32,16+8*i,c|8,0,buffer,0);
                }
                else
                {
                    printext16(32,16+8*i,c,0,buffer,0);
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
                    if (currentlist == 1) printext16(240,16+8*i,7|8,0,"->",0);
                    printext16(240+24,16+8*i,7|8,0,dir->name,0);
                }
                else
                {
                    printext16(240+24,16+8*i,7,0,dir->name,0);
                }
            }
        }
        enddrawing();
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
            {
                if (quitevent)
                {
                    keystatus[1] = 1;
                    quitevent = 0;
                }
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
                    i = (ydim2d-STATUS2DSIZ-48)>>5/*3*/;  //PK

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
                    if (ch > 0 && ((ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z') || (ch >= '0' && ch <= '9')))
                    {
                        if (ch >= 'a') ch -= ('a'-'A');
                        while (seeker)
                        {
                            ch2 = seeker->name[0];
                            if (ch2 >= 'a' && ch2 <= 'z') ch2 -= ('a'-'A');
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
        if (ch == 'f' || ch == 'F')
        {
            currentlist = 0;
            pathsearchmode = 1-pathsearchmode;
            if (pathsearchmode)
            {
                strcpy(selectedboardfilename, "");
                Bcanonicalisefilename(selectedboardfilename, 0);
            }
            else strcpy(selectedboardfilename, "/");
            getfilenames(selectedboardfilename, "*.map");
            Bstrcpy(oldpath,selectedboardfilename);
        }
        else if (ch == 'g' || ch == 'G')
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
                if (finddirshigh && finddirshigh->prev) finddirshigh = finddirshigh->prev;
            }
            else
            {
                if (findfileshigh && findfileshigh->prev) findfileshigh = findfileshigh->prev;
            }
        }
        else if (keystatus[0xd0] /*(ch == 77) || (ch == 80)*/)
        {
            if (currentlist == 0)
            {
                if (finddirshigh && finddirshigh->next) finddirshigh = finddirshigh->next;
            }
            else
            {
                if (findfileshigh && findfileshigh->next) findfileshigh = findfileshigh->next;
            }
        }
        else if ((ch == 13) && (currentlist == 0))
        {
            if (finddirshigh->type == CACHE1D_FIND_DRIVE)
            {
                strcpy(selectedboardfilename, finddirshigh->name);
            }
            else
            {
                strcat(selectedboardfilename, finddirshigh->name);
            }
            strcat(selectedboardfilename, "/");
            if (pathsearchmode)
                Bcanonicalisefilename(selectedboardfilename, 1);
            else
                Bcorrectfilename(selectedboardfilename, 1);

            Bstrcpy(oldpath,selectedboardfilename);
            //printf("Changing directories to: %s\n", selectedboardfilename);

            getfilenames(selectedboardfilename, "*.map");
            ch = 0;

            begindrawing();
            clearbuf((char *)frameplace, (bytesperline*ydim16) >> 2, 0l);
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

int fillsector(short sectnum, char fillcolor)
{
    int x1, x2, y1, y2, sy, y, tempint;
    int lborder, rborder, uborder, dborder, miny, maxy, dax;
    short z, zz, startwall, endwall, fillcnt;

    lborder = 0; rborder = xdim;
    uborder = 0; dborder = ydim16;

    if (sectnum == -1)
        return(0);
    miny = dborder-1;
    maxy = uborder;
    startwall = sector[sectnum].wallptr;
    endwall = startwall + sector[sectnum].wallnum - 1;
    for (z=startwall;z<=endwall;z++)
    {
        y1 = (((wall[z].y-posy)*zoom)>>14)+midydim16;
        y2 = (((wall[wall[z].point2].y-posy)*zoom)>>14)+midydim16;
        if (y1 < miny) miny = y1;
        if (y2 < miny) miny = y2;
        if (y1 > maxy) maxy = y1;
        if (y2 > maxy) maxy = y2;
    }
    if (miny < uborder) miny = uborder;
    if (maxy >= dborder) maxy = dborder-1;

    for (sy=miny+((totalclock>>2)&3);sy<=maxy;sy+=3)	// JBF 20040116: numframes%3 -> (totalclock>>2)&3
    {
        y = posy+(((sy-midydim16)<<14)/zoom);

        fillist[0] = lborder; fillcnt = 1;
        for (z=startwall;z<=endwall;z++)
        {
            x1 = wall[z].x; x2 = wall[wall[z].point2].x;
            y1 = wall[z].y; y2 = wall[wall[z].point2].y;
            if (y1 > y2)
            {
                tempint = x1; x1 = x2; x2 = tempint;
                tempint = y1; y1 = y2; y2 = tempint;
            }
            if ((y1 <= y) && (y2 > y))
                //if (x1*(y-y2) + x2*(y1-y) <= 0)
            {
                dax = x1+scale(y-y1,x2-x1,y2-y1);
                dax = (((dax-posx)*zoom)>>14)+halfxdim16;
                if (dax >= lborder)
                    fillist[fillcnt++] = dax;
            }
        }
        if (fillcnt > 0)
        {
            for (z=1;z<fillcnt;z++)
                for (zz=0;zz<z;zz++)
                    if (fillist[z] < fillist[zz])
                    {
                        tempint = fillist[z]; fillist[z] = fillist[zz]; fillist[zz] = tempint;
                    }

            for (z=(fillcnt&1);z<fillcnt-1;z+=2)
            {
                if (fillist[z] > rborder) break;
                if (fillist[z+1] > rborder)
                    fillist[z+1] = rborder;
                drawline16(fillist[z],sy,fillist[z+1],sy,fillcolor);
            }
        }
    }
    return(0);
}

short whitelinescan(short dalinehighlight)
{
    int i, j, k;
    short sucksect, newnumwalls;

    sucksect = sectorofwall(dalinehighlight);

    Bmemcpy(&sector[numsectors],&sector[sucksect],sizeof(sectortype));
    sector[numsectors].wallptr = numwalls;
    sector[numsectors].wallnum = 0;
    i = dalinehighlight;
    newnumwalls = numwalls;
    do
    {
        j = lastwall((short)i);
        if (wall[j].nextwall >= 0)
        {
            j = wall[j].point2;
            for (k=0;k<numwalls;k++)
            {
                if (wall[wall[k].point2].x == wall[j].x)
                    if (wall[wall[k].point2].y == wall[j].y)
                        if (wall[k].nextwall == -1)
                        {
                            j = k;
                            break;
                        }
            }
        }

        Bmemcpy(&wall[newnumwalls],&wall[i],sizeof(walltype));

        wall[newnumwalls].nextwall = j;
        wall[newnumwalls].nextsector = sectorofwall((short)j);

        newnumwalls++;
        sector[numsectors].wallnum++;

        i = j;
    }
    while (i != dalinehighlight);

    for (i=numwalls;i<newnumwalls-1;i++)
        wall[i].point2 = i+1;
    wall[newnumwalls-1].point2 = numwalls;

    if (clockdir(numwalls) == 1)
        return(-1);
    else
        return(newnumwalls);
}
/*
#define loadbyte(fil,tempbuf,bufplc,dat)        \
{                                               \
	if (bufplc == 0)                             \
	{                                            \
		for(bufplc=0;bufplc<4096;bufplc++)        \
			tempbuf[bufplc] = 0;                   \
		bufplc = 0;                               \
		read(fil,tempbuf,4096);                   \
	}                                            \
	dat = tempbuf[bufplc];                       \
	bufplc = ((bufplc+1)&4095);                  \
}                                               \

int loadnames(void)
{
	char buffer[80], firstch, ch;
	int fil, i, num, buffercnt, bufplc;

	if ((fil = open("names.h",O_BINARY|O_RDWR,S_IREAD)) == -1) return(-1);
	bufplc = 0;
	do { loadbyte(fil,tempbuf,bufplc,firstch); } while (firstch != '#');

	while ((firstch == '#') || (firstch == '/'))
	{
		do { loadbyte(fil,tempbuf,bufplc,ch); } while (ch > 32);

		buffercnt = 0;
		do
		{
			loadbyte(fil,tempbuf,bufplc,ch);
			if (ch > 32) buffer[buffercnt++] = ch;
		}
		while (ch > 32);

		num = 0;
		do
		{
			loadbyte(fil,tempbuf,bufplc,ch);
			if ((ch >= 48) && (ch <= 57)) num = num*10+(ch-48);
		}
		while (ch != 13);
		for(i=0;i<buffercnt;i++) names[num][i] = buffer[i];
		names[num][buffercnt] = 0;

		loadbyte(fil,tempbuf,bufplc,firstch);
		if (firstch == 10) loadbyte(fil,tempbuf,bufplc,firstch);
	}
	close(fil);
	return(0);
}
*/

int loadnames(void)
{
    char buffer[1024], *p, *name, *number, *endptr;
    int num, syms=0, line=0, a, comment=0;
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
    memset(names,0,sizeof(names));

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
            if (*(p+1) == '*') {comment++;continue;}
            if (*(p+1) == '/') continue;	// comment
        }
        else if (*p == '*' && p[1] == '/')
        {
            comment--;continue;
        }
        else if (comment)continue;
badline:
        initprintf("Error: Invalid statement found at character %d on line %d\n", (p-buffer), line-1);
    }
    initprintf("Read %d lines, loaded %d names.\n", line, syms);

    Bfclose(fp);
    return 0;
}

void printcoords16(int posxe, int posye, short ange)
{
    char snotbuf[80];
    int i,m;

    Bsprintf(snotbuf,"x:%d y:%d ang:%d",posxe,posye,ange);
    i = 0;
    while ((snotbuf[i] != 0) && (i < 30))
        i++;
    while (i < 30)
    {
        snotbuf[i] = 32;
        i++;
    }
    snotbuf[30] = 0;

    printext16(8, ydim-STATUS2DSIZ+128, 9, 0, snotbuf,0);

    m = (numsectors > MAXSECTORSV7 || numwalls > MAXWALLSV7 || numsprites > MAXSPRITESV7);

    Bsprintf(snotbuf,"%d/%d sect. %d/%d walls %d/%d spri.",
                 numsectors,m?MAXSECTORSV8:MAXSECTORSV7,
                 numwalls,m?MAXWALLSV8:MAXWALLSV7,
                 numsprites,m?MAXSPRITESV8:MAXSPRITESV7);

    i = 0;
    while ((snotbuf[i] != 0) && (i < 46))
        i++;
    while (i < 46)
    {
        snotbuf[i] = 32;
        i++;
    }
    snotbuf[46] = 0;

    printext16(264, ydim-STATUS2DSIZ+128, 9+m, 0, snotbuf,0);
}

void updatenumsprites(void)
{
    int i;

    numsprites = 0;
    for (i=0;i<MAXSPRITES;i++)
        if (sprite[i].statnum < MAXSTATUS)
            numsprites++;
}

void copysector(short soursector, short destsector, short deststartwall, char copystat)
{
    short j, k, m, newnumwalls, startwall, endwall;

    newnumwalls = deststartwall;  //erase existing sector fragments

    //duplicate walls
    startwall = sector[soursector].wallptr;
    endwall = startwall + sector[soursector].wallnum;
    for (j=startwall;j<endwall;j++)
    {
        Bmemcpy(&wall[newnumwalls],&wall[j],sizeof(walltype));
        wall[newnumwalls].point2 += deststartwall-startwall;
        if (wall[newnumwalls].nextwall >= 0)
        {
            wall[newnumwalls].nextwall += deststartwall-startwall;
            wall[newnumwalls].nextsector += destsector-soursector;
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
                k = nextspritesect[j];

                m = insertsprite(destsector,sprite[j].statnum);
                Bmemcpy(&sprite[m],&sprite[j],sizeof(spritetype));
                sprite[m].sectnum = destsector;   //Don't let memcpy overwrite sector!

                j = k;
            }
        }

    }
}

void showsectordata(short sectnum)
{
    char snotbuf[80];

    Bsprintf(snotbuf,"^10Sector %d ^O(F7 to edit)",sectnum);
    printext16(8,ydim-STATUS2DSIZ+32,11,-1,snotbuf,0);
    Bsprintf(snotbuf,"Firstwall: %d",sector[sectnum].wallptr);
    printext16(8,ydim-STATUS2DSIZ+48,11,-1,snotbuf,0);
    Bsprintf(snotbuf,"Numberofwalls: %d",sector[sectnum].wallnum);
    printext16(8,ydim-STATUS2DSIZ+56,11,-1,snotbuf,0);
    Bsprintf(snotbuf,"Firstsprite: %d",headspritesect[sectnum]);
    printext16(8,ydim-STATUS2DSIZ+64,11,-1,snotbuf,0);
    Bsprintf(snotbuf,"Tags: %d, %d",sector[sectnum].hitag,sector[sectnum].lotag);
    printext16(8,ydim-STATUS2DSIZ+72,11,-1,snotbuf,0);
    Bsprintf(snotbuf,"     (0x%x), (0x%x)",sector[sectnum].hitag,sector[sectnum].lotag);
    printext16(8,ydim-STATUS2DSIZ+80,11,-1,snotbuf,0);
    Bsprintf(snotbuf,"Extra: %d",sector[sectnum].extra);
    printext16(8,ydim-STATUS2DSIZ+88,11,-1,snotbuf,0);
    Bsprintf(snotbuf,"Visibility: %d",sector[sectnum].visibility);
    printext16(8,ydim-STATUS2DSIZ+96,11,-1,snotbuf,0);
    Bsprintf(snotbuf,"Pixel height: %d",(sector[sectnum].floorz-sector[sectnum].ceilingz)>>8);
    printext16(8,ydim-STATUS2DSIZ+104,11,-1,snotbuf,0);

    printext16(200,ydim-STATUS2DSIZ+32,11,-1,"^10CEILINGS:^O",0);
    Bsprintf(snotbuf,"Flags (hex): %x",sector[sectnum].ceilingstat);
    printext16(200,ydim-STATUS2DSIZ+48,11,-1,snotbuf,0);
    Bsprintf(snotbuf,"(X,Y)pan: %d, %d",sector[sectnum].ceilingxpanning,sector[sectnum].ceilingypanning);
    printext16(200,ydim-STATUS2DSIZ+56,11,-1,snotbuf,0);
    Bsprintf(snotbuf,"Shade byte: %d",sector[sectnum].ceilingshade);
    printext16(200,ydim-STATUS2DSIZ+64,11,-1,snotbuf,0);
    Bsprintf(snotbuf,"Z-coordinate: %d",sector[sectnum].ceilingz);
    printext16(200,ydim-STATUS2DSIZ+72,11,-1,snotbuf,0);
    Bsprintf(snotbuf,"Tile number: %d",sector[sectnum].ceilingpicnum);
    printext16(200,ydim-STATUS2DSIZ+80,11,-1,snotbuf,0);
    Bsprintf(snotbuf,"Ceiling heinum: %d",sector[sectnum].ceilingheinum);
    printext16(200,ydim-STATUS2DSIZ+88,11,-1,snotbuf,0);
    Bsprintf(snotbuf,"Palookup number: %d",sector[sectnum].ceilingpal);
    printext16(200,ydim-STATUS2DSIZ+96,11,-1,snotbuf,0);

    printext16(400,ydim-STATUS2DSIZ+32,11,-1,"^10FLOORS:^O",0);
    Bsprintf(snotbuf,"Flags (hex): %x",sector[sectnum].floorstat);
    printext16(400,ydim-STATUS2DSIZ+48,11,-1,snotbuf,0);
    Bsprintf(snotbuf,"(X,Y)pan: %d, %d",sector[sectnum].floorxpanning,sector[sectnum].floorypanning);
    printext16(400,ydim-STATUS2DSIZ+56,11,-1,snotbuf,0);
    Bsprintf(snotbuf,"Shade byte: %d",sector[sectnum].floorshade);
    printext16(400,ydim-STATUS2DSIZ+64,11,-1,snotbuf,0);
    Bsprintf(snotbuf,"Z-coordinate: %d",sector[sectnum].floorz);
    printext16(400,ydim-STATUS2DSIZ+72,11,-1,snotbuf,0);
    Bsprintf(snotbuf,"Tile number: %d",sector[sectnum].floorpicnum);
    printext16(400,ydim-STATUS2DSIZ+80,11,-1,snotbuf,0);
    Bsprintf(snotbuf,"Floor heinum: %d",sector[sectnum].floorheinum);
    printext16(400,ydim-STATUS2DSIZ+88,11,-1,snotbuf,0);
    Bsprintf(snotbuf,"Palookup number: %d",sector[sectnum].floorpal);
    printext16(400,ydim-STATUS2DSIZ+96,11,-1,snotbuf,0);
}

void showwalldata(short wallnum)
{
    int dax, day, dist;
    char snotbuf[80];

    Bsprintf(snotbuf,"^10Wall %d ^O(F8 to edit)",wallnum);
    printext16(8,ydim-STATUS2DSIZ+32,11,-1,snotbuf,0);
    Bsprintf(snotbuf,"X-coordinate: %d",wall[wallnum].x);
    printext16(8,ydim-STATUS2DSIZ+48,11,-1,snotbuf,0);
    Bsprintf(snotbuf,"Y-coordinate: %d",wall[wallnum].y);
    printext16(8,ydim-STATUS2DSIZ+56,11,-1,snotbuf,0);
    Bsprintf(snotbuf,"Point2: %d",wall[wallnum].point2);
    printext16(8,ydim-STATUS2DSIZ+64,11,-1,snotbuf,0);
    Bsprintf(snotbuf,"Sector: %d",sectorofwall(wallnum));
    printext16(8,ydim-STATUS2DSIZ+72,11,-1,snotbuf,0);

    Bsprintf(snotbuf,"Tags: %d, %d",wall[wallnum].hitag,wall[wallnum].lotag);
    printext16(8,ydim-STATUS2DSIZ+88,11,-1,snotbuf,0);
    Bsprintf(snotbuf,"     (0x%x), (0x%x)",wall[wallnum].hitag,wall[wallnum].lotag);
    printext16(8,ydim-STATUS2DSIZ+96,11,-1,snotbuf,0);

    printext16(200,ydim-STATUS2DSIZ+32,11,-1,names[wall[wallnum].picnum],0);
    Bsprintf(snotbuf,"Flags (hex): %x",wall[wallnum].cstat);
    printext16(200,ydim-STATUS2DSIZ+48,11,-1,snotbuf,0);
    Bsprintf(snotbuf,"Shade: %d",wall[wallnum].shade);
    printext16(200,ydim-STATUS2DSIZ+56,11,-1,snotbuf,0);
    Bsprintf(snotbuf,"Pal: %d",wall[wallnum].pal);
    printext16(200,ydim-STATUS2DSIZ+64,11,-1,snotbuf,0);
    Bsprintf(snotbuf,"(X,Y)repeat: %d, %d",wall[wallnum].xrepeat,wall[wallnum].yrepeat);
    printext16(200,ydim-STATUS2DSIZ+72,11,-1,snotbuf,0);
    Bsprintf(snotbuf,"(X,Y)pan: %d, %d",wall[wallnum].xpanning,wall[wallnum].ypanning);
    printext16(200,ydim-STATUS2DSIZ+80,11,-1,snotbuf,0);
    Bsprintf(snotbuf,"Tile number: %d",wall[wallnum].picnum);
    printext16(200,ydim-STATUS2DSIZ+88,11,-1,snotbuf,0);
    Bsprintf(snotbuf,"OverTile number: %d",wall[wallnum].overpicnum);
    printext16(200,ydim-STATUS2DSIZ+96,11,-1,snotbuf,0);

    Bsprintf(snotbuf,"nextsector: %d",wall[wallnum].nextsector);
    printext16(400,ydim-STATUS2DSIZ+48,11,-1,snotbuf,0);
    Bsprintf(snotbuf,"nextwall: %d",wall[wallnum].nextwall);
    printext16(400,ydim-STATUS2DSIZ+56,11,-1,snotbuf,0);

    Bsprintf(snotbuf,"Extra: %d",wall[wallnum].extra);
    printext16(400,ydim-STATUS2DSIZ+72,11,-1,snotbuf,0);

    dax = wall[wallnum].x-wall[wall[wallnum].point2].x;
    day = wall[wallnum].y-wall[wall[wallnum].point2].y;
    dist = ksqrt(dax*dax+day*day);

    // TX 20050102 I'm not sure what unit dist<<4 is supposed to be, but dist itself is correct in terms of game coordinates as one would expect

    Bsprintf(snotbuf,"Wall length: %d",dist);
    printext16(400,ydim-STATUS2DSIZ+96,11,-1,snotbuf,0);

    dax = (int)sectorofwall(wallnum);
    Bsprintf(snotbuf,"Pixel height: %d",(sector[dax].floorz-sector[dax].ceilingz)>>8);
    printext16(400,ydim-STATUS2DSIZ+104,11,-1,snotbuf,0);
}

void showspritedata(short spritenum)
{
    char snotbuf[80];

    Bsprintf(snotbuf,"^10Sprite %d ^O(F8 to edit)",spritenum);
    printext16(8,ydim-STATUS2DSIZ+32,11,-1,snotbuf,0);
    Bsprintf(snotbuf,"X-coordinate: %d",sprite[spritenum].x);
    printext16(8,ydim-STATUS2DSIZ+48,11,-1,snotbuf,0);
    Bsprintf(snotbuf,"Y-coordinate: %d",sprite[spritenum].y);
    printext16(8,ydim-STATUS2DSIZ+56,11,-1,snotbuf,0);
    Bsprintf(snotbuf,"Z-coordinate: %d",sprite[spritenum].z);
    printext16(8,ydim-STATUS2DSIZ+64,11,-1,snotbuf,0);

    Bsprintf(snotbuf,"Sectnum: %d",sprite[spritenum].sectnum);
    printext16(8,ydim-STATUS2DSIZ+72,11,-1,snotbuf,0);
    Bsprintf(snotbuf,"Statnum: %d",sprite[spritenum].statnum);
    printext16(8,ydim-STATUS2DSIZ+80,11,-1,snotbuf,0);

    Bsprintf(snotbuf,"Tags: %d, %d",sprite[spritenum].hitag,sprite[spritenum].lotag);
    printext16(8,ydim-STATUS2DSIZ+96,11,-1,snotbuf,0);
    Bsprintf(snotbuf,"     (0x%x), (0x%x)",sprite[spritenum].hitag,sprite[spritenum].lotag);
    printext16(8,ydim-STATUS2DSIZ+104,11,-1,snotbuf,0);

    Bsprintf(snotbuf,"^10%s^O",names[sprite[spritenum].picnum]);
    printext16(200,ydim-STATUS2DSIZ+32,11,-1,snotbuf,0);
    Bsprintf(snotbuf,"Flags (hex): %x",sprite[spritenum].cstat);
    printext16(200,ydim-STATUS2DSIZ+48,11,-1,snotbuf,0);
    Bsprintf(snotbuf,"Shade: %d",sprite[spritenum].shade);
    printext16(200,ydim-STATUS2DSIZ+56,11,-1,snotbuf,0);
    Bsprintf(snotbuf,"Pal: %d",sprite[spritenum].pal);
    printext16(200,ydim-STATUS2DSIZ+64,11,-1,snotbuf,0);
    Bsprintf(snotbuf,"(X,Y)repeat: %d, %d",sprite[spritenum].xrepeat,sprite[spritenum].yrepeat);
    printext16(200,ydim-STATUS2DSIZ+72,11,-1,snotbuf,0);
    Bsprintf(snotbuf,"(X,Y)offset: %d, %d",sprite[spritenum].xoffset,sprite[spritenum].yoffset);
    printext16(200,ydim-STATUS2DSIZ+80,11,-1,snotbuf,0);
    Bsprintf(snotbuf,"Tile number: %d",sprite[spritenum].picnum);
    printext16(200,ydim-STATUS2DSIZ+88,11,-1,snotbuf,0);

    Bsprintf(snotbuf,"Angle (2048 degrees): %d",sprite[spritenum].ang);
    printext16(400,ydim-STATUS2DSIZ+48,11,-1,snotbuf,0);
    Bsprintf(snotbuf,"X-Velocity: %d",sprite[spritenum].xvel);
    printext16(400,ydim-STATUS2DSIZ+56,11,-1,snotbuf,0);
    Bsprintf(snotbuf,"Y-Velocity: %d",sprite[spritenum].yvel);
    printext16(400,ydim-STATUS2DSIZ+64,11,-1,snotbuf,0);
    Bsprintf(snotbuf,"Z-Velocity: %d",sprite[spritenum].zvel);
    printext16(400,ydim-STATUS2DSIZ+72,11,-1,snotbuf,0);
    Bsprintf(snotbuf,"Owner: %d",sprite[spritenum].owner);
    printext16(400,ydim-STATUS2DSIZ+80,11,-1,snotbuf,0);
    Bsprintf(snotbuf,"Clipdist: %d",sprite[spritenum].clipdist);
    printext16(400,ydim-STATUS2DSIZ+88,11,-1,snotbuf,0);
    Bsprintf(snotbuf,"Extra: %d",sprite[spritenum].extra);
    printext16(400,ydim-STATUS2DSIZ+96,11,-1,snotbuf,0);
}

void keytimerstuff(void)
{
    static int ltotalclock=0;
    if (totalclock == ltotalclock) return;
    ltotalclock=totalclock;

    if (keystatus[buildkeys[BK_STRAFE]] == 0)
    {
        // PK: With GCC and Polymost, keyboard turning lags
        if (keystatus[buildkeys[BK_TURNLEFT]]) angvel = max(angvel-pk_turnaccel /* 16 */,-128);
        if (keystatus[buildkeys[BK_TURNRIGHT]]) angvel = min(angvel+pk_turnaccel /* 16 */,127);
    }
    else
    {
        if (keystatus[buildkeys[BK_TURNLEFT]]) svel = min(svel+16,255); // svel and vel aren't even chars...
        if (keystatus[buildkeys[BK_TURNRIGHT]]) svel = max(svel-16,-256);
    }
    if (keystatus[buildkeys[BK_MOVEFORWARD]])  vel = min(vel+16,255);
    if (keystatus[buildkeys[BK_MOVEBACKWARD]]) vel = max(vel-16,-256);
    /*  if (keystatus[buildkeys[BK_STRAFELEFT]])  svel = min(svel+8,127);
    	if (keystatus[buildkeys[BK_STRAFERIGHT]]) svel = max(svel-8,-128); */

    if (angvel < 0) angvel = min(angvel+pk_turndecel /*12*/,0);
    if (angvel > 0) angvel = max(angvel-pk_turndecel /*12*/,0);
    if (svel < 0) svel = min(svel+6,0);
    if (svel > 0) svel = max(svel-6,0);
    if (vel < 0) vel = min(vel+6,0);
    if (vel > 0) vel = max(vel-6,0);
    /*    if(mlook)
            posz -= (horiz-101)*(vel/40); */
}

void _printmessage16(const char *fmt, ...)
{
    int i, ybase;
    char snotbuf[60];
    char tmpstr[64];
    va_list va;

    va_start(va, fmt);
    Bvsnprintf(tmpstr, 64, fmt, va);
    va_end(va);

    i = 0;
    while ((tmpstr[i] != 0) && (i < 54))
    {
        snotbuf[i] = tmpstr[i];
        i++;
    }
    while (i < 54)
    {
        snotbuf[i] = 32;
        i++;
    }
    snotbuf[54] = 0;
    begindrawing();
    ybase = (overridepm16y >= 0) ? ydim-overridepm16y : ydim-STATUS2DSIZ;
    printext16(200L-24, ybase+8L, 9, 0, snotbuf, 0);
    i = 0;
    while (i < 54)
    {
        snotbuf[i] = 32;
        i++;
    }
    snotbuf[54] = 0;
    printext16(200L-24, ybase+20L, 9, 0, snotbuf, 0);
    enddrawing();
}

void printmessage256(int x, int y, char *name)
{
    char snotbuf[64];
    int i;

    i = 0;
    while ((name[i] != 0) && (i < 62))
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
void getclosestpointonwall(int x, int y, int dawall, int *nx, int *ny)
{
    walltype *wal;
    int64 i, j, dx, dy;

    wal = &wall[dawall];
    dx = wall[wal->point2].x-wal->x;
    dy = wall[wal->point2].y-wal->y;
    i = dx*(x-wal->x) + dy*(y-wal->y);
    if (i <= 0) { *nx = wal->x; *ny = wal->y; return; }
    j = dx*dx+dy*dy;
    if (i >= j) { *nx = wal->x+dx; *ny = wal->y+dy; return; }
    i=((i<<15)/j)<<15;
    *nx = wal->x + ((dx*i)>>30);
    *ny = wal->y + ((dy*i)>>30);
}

void initcrc(void)
{
    int i, j, k, a;

    for (j=0;j<256;j++)     //Calculate CRC table
    {
        k = (j<<8); a = 0;
        for (i=7;i>=0;i--)
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

static char visited[MAXWALLS];

int GetWallZPeg(int nWall)
{
    int z=0, nSector, nNextSector;

    nSector = sectorofwall((short)nWall);
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

void AlignWalls(int nWall0, int z0, int nWall1, int z1, int nTile)
{
    int n;

    //do the x alignment
    wall[nWall1].cstat &= ~0x0108;    //Set to non-flip
    wall[nWall1].xpanning = (char)((wall[nWall0].xpanning+(wall[nWall0].xrepeat<<3))%tilesizx[nTile]);

    z1 = GetWallZPeg(nWall1);

    for (n=(picsiz[nTile]>>4);((1<<n)<tilesizy[nTile]);n++);

    wall[nWall1].yrepeat = wall[nWall0].yrepeat;
    wall[nWall1].ypanning = (char)(wall[nWall0].ypanning+(((z1-z0)*wall[nWall0].yrepeat)>>(n+3)));
}

void AutoAlignWalls(int nWall0, int ply)
{
    int z0, z1, nTile, nWall1, branch, visible, nNextSector, nSector;

    nTile = wall[nWall0].picnum;
    branch = 0;
    if (ply == 0)
    {
        //clear visited bits
        memset(visited,0,sizeof(visited));
        visited[nWall0] = 1;
    }

    z0 = GetWallZPeg(nWall0);

    nWall1 = wall[nWall0].point2;

    //loop through walls at this vertex in CCW order
    while (1)
    {
        //break if this wall would connect us in a loop
        if (visited[nWall1]) break;

        visited[nWall1] = 1;

        //break if reached back of left wall
        if (wall[nWall1].nextwall == nWall0) break;

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
                nSector = wall[wall[nWall1].nextwall].nextsector;
                if (getceilzofslope((short)nSector,wall[nWall1].x,wall[nWall1].y) <
                        getceilzofslope((short)nNextSector,wall[nWall1].x,wall[nWall1].y))
                    visible = 1;

                if (getflorzofslope((short)nSector,wall[nWall1].x,wall[nWall1].y) >
                        getflorzofslope((short)nNextSector,wall[nWall1].x,wall[nWall1].y))
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
        nWall1 = wall[wall[nWall1].nextwall].point2;
    }
}


/*
 * vim:ts=4:
 */

