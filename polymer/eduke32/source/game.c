//-------------------------------------------------------------------------
/*
Copyright (C) 1996, 2003 - 3D Realms Entertainment
Copyright (C) 2000, 2003 - Matt Saettler (EDuke Enhancements)
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

#include "duke3d.h"

#include "scriplib.h"
#include "file_lib.h"
#include "mathutil.h"
#include "gamedefs.h"
#include "keyboard.h"
#include "mouse.h"  // JBF 20030809
#include "function.h"
#include "control.h"
#include "fx_man.h"
#include "sounds.h"
#include "config.h"
#include "osd.h"
#include "osdfuncs.h"
#include "osdcmds.h"
#include "scriptfile.h"
#include "grpscan.h"
#include "gamedef.h"
#include "kplib.h"
//#include "crc32.h"
#include "util_lib.h"
#include "hightile.h"

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <shellapi.h>
extern int32_t G_GetVersionFromWebsite(char *buffer);
#define UPDATEINTERVAL 604800 // 1w
#else
static int32_t usecwd = 0;
#endif /* _WIN32 */
int32_t g_scriptSanityChecks = 1;

#define IDFSIZE 479985668
#define IDFILENAME "DUKE3D.IDF"

#define TIMERUPDATESIZ 32

int32_t g_cameraDistance = 0, g_cameraClock = 0;
static int32_t qe;
static int32_t g_commandSetup = 0;
int32_t g_noSetup = 0;
static int32_t g_noAutoLoad = 0;
static int32_t g_noSound = 0;
static int32_t g_noMusic = 0;
static char *CommandMap = NULL;
static char *CommandName = NULL;
static int32_t g_keepAddr = 0;
int32_t CommandWeaponChoice = 0;
static struct strllist
{
    struct strllist *next;
    char *str;
}
*CommandPaths = NULL, *CommandGrps = NULL;

char boardfilename[BMAX_PATH] = {0}, currentboardfilename[BMAX_PATH] = {0};
char root[BMAX_PATH];
uint8_t waterpal[768], slimepal[768], titlepal[768], drealms[768], endingpal[768], animpal[768];
static char firstdemofile[80] = { '\0' };
static int32_t userconfiles = 0;

static int32_t netparamcount = 0;
static char **netparam = NULL;

int32_t voting = -1;
int32_t vote_map = -1, vote_episode = -1;

int32_t recfilep,totalreccnt;
int32_t debug_on = 0,g_noEnemies = 0;
static char *rtsptr;

//extern char syncstate;
extern int32_t numlumps;

static FILE *frecfilep = (FILE *)NULL;
static int32_t demo_goalreccnt=0, demo_startreccnt=0, demo_oldsoundtoggle, demo_showstats=1;
static int32_t demo_paused=0;

int32_t g_restorePalette, g_screenCapture;
static int32_t g_noLogoAnim = 0;
static int32_t g_noLogo = 0;
static int32_t g_chatPlayer = -1;

char defaultduke3dgrp[BMAX_PATH] = "duke3d.grp";
static char defaultduke3ddef[BMAX_PATH] = "duke3d.def";
static char defaultconfilename[BMAX_PATH] = {"EDUKE.CON"};

char *duke3dgrp = defaultduke3dgrp;
char *duke3ddef = defaultduke3ddef;
static char *confilename = defaultconfilename;

char *duke3dgrpstring = NULL;
char mod_dir[BMAX_PATH] = "/";

#if defined(POLYMOST)
extern char TEXCACHEFILE[BMAX_PATH];
#endif

extern int32_t lastvisinc;

int32_t g_Shareware = 0;
int32_t g_gameType = 0;

#define MAXUSERQUOTES 6
static int32_t quotebot, quotebotgoal;
static int32_t user_quote_time[MAXUSERQUOTES];
static char user_quote[MAXUSERQUOTES][178];
// char typebuflen,typebuf[41];

static int32_t MAXCACHE1DSIZE = (32*1048576);

int32_t tempwallptr;

static int32_t nonsharedtimer;

static void G_DrawCameraText(int16_t i);
static inline int32_t G_MoveLoop(void);
static void G_DoOrderScreen(void);
static void Net_DoPrediction(void);
static void Net_CorrectPrediction(void);
static int32_t G_DoMoveThings(void);
static int32_t G_PlaybackDemo(void);

static char recbuf[180];

extern void computergetinput(int32_t snum, input_t *syn);

#define USERQUOTE_LEFTOFFSET 5
#define USERQUOTE_RIGHTOFFSET 14

int32_t althud_numbertile = 2930;
int32_t althud_numberpal = 0;
int32_t althud_shadows = 1;
int32_t althud_flashing = 1;
int32_t hud_glowingquotes = 1;
int32_t hud_showmapname = 1;

int32_t g_levelTextTime = 0;

int32_t r_maxfps = 0;
uint32_t g_frameDelay = 0;

#if defined(RENDERTYPEWIN) && defined(USE_OPENGL)
extern char forcegl;
#endif

int32_t kopen4loadfrommod(char *filename, char searchfirst)
{
    static char fn[BMAX_PATH];
    int32_t r;

    Bsprintf(fn,"%s/%s",mod_dir,filename);
    r = kopen4load(fn,searchfirst);
    if (r < 0)
        r = kopen4load(filename,searchfirst);
    return r;
}

enum
{
    T_EOF = -2,
    T_ERROR = -1,
    T_INCLUDE = 0,
    T_INTERFACE = 0,
    T_LOADGRP = 1,
    T_MODE = 1,
    T_CACHESIZE = 2,
    T_ALLOW = 2,
    T_NOAUTOLOAD,
    T_MUSIC,
    T_SOUND,
    T_FILE,
    T_ID
};

typedef struct
{
    char *text;
    int32_t tokenid;
}
tokenlist;

static int32_t getatoken(scriptfile *sf, tokenlist *tl, int32_t ntokens)
{
    char *tok;
    int32_t i;

    if (!sf) return T_ERROR;
    tok = scriptfile_gettoken(sf);
    if (!tok) return T_EOF;

    for (i=ntokens-1; i>=0; i--)
    {
        if (!Bstrcasecmp(tok, tl[i].text))
            return tl[i].tokenid;
    }
    return T_ERROR;
}

inline void G_SetStatusBarScale(int32_t sc)
{
    ud.statusbarscale = min(100,max(10,sc));
    G_UpdateScreenArea();
}

static inline int32_t sbarx(int32_t x)
{
    if (ud.screen_size == 4 /*|| ud.statusbarmode == 1*/) return scale(x<<16,ud.statusbarscale,100);
    return (((320l<<16) - scale(320l<<16,ud.statusbarscale,100)) >> 1) + scale(x<<16,ud.statusbarscale,100);
}

static inline int32_t sbarxr(int32_t x)
{
    if (ud.screen_size == 4 /*|| ud.statusbarmode == 1*/) return (320l<<16) - scale(x<<16,ud.statusbarscale,100);
    return (((320l<<16) - scale(320l<<16,ud.statusbarscale,100)) >> 1) + scale(x<<16,ud.statusbarscale,100);
}

static inline int32_t sbary(int32_t y)
{
    return ((200l<<16) - scale(200l<<16,ud.statusbarscale,100) + scale(y<<16,ud.statusbarscale,100));
}

static inline int32_t sbarsc(int32_t sc)
{
    return scale(sc,ud.statusbarscale,100);
}

static inline int32_t textsc(int32_t sc)
{
    // prevent ridiculousness to a degree
    if (xdim <= 320) return sc;
    else if (xdim <= 640) return scale(sc,min(200,ud.textscale),100);
    else if (xdim <= 800) return scale(sc,min(300,ud.textscale),100);
    else if (xdim <= 1024) return scale(sc,min(350,ud.textscale),100);
    return scale(sc,ud.textscale,100);
}

static void G_PatchStatusBar(int32_t x1, int32_t y1, int32_t x2, int32_t y2)
{
    int32_t scl, tx, ty;
    int32_t clx1,cly1,clx2,cly2,clofx,clofy;

    scl = sbarsc(65536);
    tx = sbarx(0);
    ty = sbary(200-tilesizy[BOTTOMSTATUSBAR]);

    clx1 = scale(scale(x1,xdim,320),ud.statusbarscale,100);
    cly1 = scale(scale(y1,ydim,200),ud.statusbarscale,100);
    clx2 = scale(scale(x2,xdim,320),ud.statusbarscale,100);
    cly2 = scale(scale(y2,ydim,200),ud.statusbarscale,100);
    clofx = (xdim - scale(xdim,ud.statusbarscale,100)) >> 1;
    clofy = (ydim - scale(ydim,ud.statusbarscale,100));

//    if (ud.statusbarmode == 0)
    rotatesprite(tx,ty,scl,0,BOTTOMSTATUSBAR,4,0,10+16+64,clx1+clofx,cly1+clofy,clx2+clofx-1,cly2+clofy-1);
//    else rotatesprite(tx,ty,scl,0,BOTTOMSTATUSBAR,4,0,10+16+64,clx1,cly1,clx2+clofx-1,cly2+clofy-1);
}

void P_SetGamePalette(DukePlayer_t *player, uint8_t *pal, int32_t set)
{
    if (player != g_player[screenpeek].ps)
    {
        // another head
        player->palette = pal;
        return;
    }

    if (!(pal == palette || pal == waterpal || pal == slimepal || pal == drealms || pal == titlepal || pal == endingpal || pal == animpal))
        pal = palette;

    setbrightness(ud.brightness>>2, pal, set);
    player->palette = pal;
}

int32_t gametext_z(int32_t small, int32_t starttile, int32_t x,int32_t y,const char *t,int32_t s,int32_t p,int32_t orientation,int32_t x1, int32_t y1, int32_t x2, int32_t y2, int32_t z)
{
    int32_t ac,newx,oldx=x;
    char centre;
    char *oldt;
    int32_t squishtext = ((small&2)!=0);
//    int32_t ht = usehightile;
    int32_t shift = 16, widthx = 320, ox, oy;
    int32_t origy = y;

    if (orientation & 256)
    {
        widthx = 320<<16;
        shift = 0;
    }
    centre = (x == (widthx>>1));
    newx = 0;
    oldt = (char *)t;

    if (t == NULL)
        return -1;

    if (centre)
    {
        do
        {
            if (*t == '^' && isdigit(*(t+1)))
            {
                t++;
                if (isdigit(*t)) t++;
                continue;
            }
            if (*t == 32)
            {
                if (small&8)
                    newx+=(8-squishtext)*z/65536;
                else
                    newx+=(5-squishtext)*z/65536;
                continue;
            }
            else ac = *t - '!' + starttile;

            if (ac < starttile || ac > (starttile + 93)) break;

            if (*t >= '0' && *t <= '9')
                newx += (8)*z/65536;
            else
            {
                if (small&8)
                    newx += (8-squishtext)*z/65536;
                else newx += (tilesizx[ac]-squishtext)*z/65536;
            }
        }
        while (*(++t));

        t = oldt;
        if (small&4)
            x = (xres>>1)-textsc(newx>>1);
        else x = (widthx>>1)-((orientation & 256)?newx<<15:newx>>1);
    }
//    usehightile = (ht && r_downsize < 2);
    ox=x;
    oy=y;
    do
    {
        if (*t == '^' && isdigit(*(t+1)))
        {
            char smallbuf[4];
            if (!isdigit(*(++t+1)))
            {
                smallbuf[0] = *(t);
                smallbuf[1] = '\0';
                p = atoi(smallbuf);
                continue;
            }
            smallbuf[0] = *(t++);
            smallbuf[1] = *(t);
            smallbuf[2] = '\0';
            p = atoi(smallbuf);
            continue;
        }
        if (*t == 32)
        {
            if (small&8)
                x+=(8-squishtext)*z/65536;
            else
                x+=(5-squishtext)*z/65536;
            continue;
        }
        else ac = *t - '!' + starttile;

        if (ac < starttile || ac > (starttile + 93))
            break;

        if (orientation&256)
        {
            x+=(x-ox)<<16;
            y+=(y-oy)<<16;
            ox=x; oy=y;
        }

        if (small&4)
        {
            rotatesprite(textsc(x<<shift),(origy<<shift)+textsc((y-origy)<<shift),textsc(z),0,ac,s,p,(8|16|(orientation&1)|(orientation&32)),x1,y1,x2,y2);
        }
        else
        {
            rotatesprite(x<<shift,(y<<shift),z,0,ac,s,p,(small&1)?(8|16|(orientation&1)|(orientation&32)):(2|orientation),x1,y1,x2,y2);
        }

        if ((*t >= '0' && *t <= '9'))
            x += (8)*z/65536;
        else if (small&8)
            x += (8-squishtext)*z/65536;//(tilesizx[ac]>>small);
        else x += (tilesizx[ac]-squishtext)*z/65536;//(tilesizx[ac]>>small);

        if ((orientation&256) == 0) //  warpping long strings doesn't work for precise coordinates due to overflow
        {
            if (small&4)
            {
                if (textsc(x) > (ud.config.ScreenWidth - USERQUOTE_RIGHTOFFSET))
                    oldt = (char *)t, x = oldx, y+=8*z/65536;
            }
            else if (x > (ud.config.ScreenWidth - USERQUOTE_RIGHTOFFSET))
                oldt = (char *)t, x = oldx, y+=8*z/65536;
        }
    }
    while (*(++t));
//    usehightile = ht;
    return (x);
}

int32_t G_GameTextLen(int32_t x,const char *t)
{
    int32_t ac;

    if (t == NULL)
        return -1;

    do
    {
        if (*t == 32)
        {
            x+=5;
            continue;
        }
        else ac = *t - '!' + STARTALPHANUM;

        if (ac < STARTALPHANUM || ac > (STARTALPHANUM + 93))
            break;

        if ((*t >= '0' && *t <= '9'))
            x += 8;
        else x += tilesizx[ac];
    }
    while (*(++t));
    return (textsc(x));
}

static inline int32_t mpgametext(int32_t y,const char *t,int32_t s,int32_t dabits)
{
//    if (xdim < 640 || ydim < 480)
    //      return(gametext_z(0,STARTALPHANUM, 5,y,t,s,0,dabits,0, 0, xdim-1, ydim-1, 65536));
    return(gametext_z(4,STARTALPHANUM, 5,y,t,s,0,dabits,0, 0, xdim-1, ydim-1, 65536));
}

int32_t minitext_(int32_t x,int32_t y,const char *t,int32_t s,int32_t p,int32_t sb)
{
    int32_t ac;
    char ch, cmode;
//    int32_t ht = usehightile;

    cmode = (sb&256)!=0;
    sb &= 255;

//    usehightile = (ht && !r_downsize);
    do
    {
        if (*t == '^' && isdigit(*(t+1)))
        {
            char smallbuf[4];
            if (!isdigit(*(++t+1)))
            {
                smallbuf[0] = *(t);
                smallbuf[1] = '\0';
                p = atoi(smallbuf);
                continue;
            }
            smallbuf[0] = *(t++);
            smallbuf[1] = *(t);
            smallbuf[2] = '\0';
            p = atoi(smallbuf);
            continue;
        }
        ch = Btoupper(*t);
        if (ch == 32)
        {
            x+=5;
            continue;
        }
        else ac = ch - '!' + MINIFONT;

        if (cmode) rotatesprite(sbarx(x),sbary(y),sbarsc(65536L),0,ac,s,p,sb,0,0,xdim-1,ydim-1);
        else rotatesprite(x<<16,y<<16,65536L,0,ac,s,p,sb,0,0,xdim-1,ydim-1);
        x += 4; // tilesizx[ac]+1;

    }
    while (*(++t));
//    usehightile = ht;
    return (x);
}

void G_AddUserQuote(const char *daquote)
{
    int32_t i;

    for (i=MAXUSERQUOTES-1; i>0; i--)
    {
        Bstrcpy(user_quote[i],user_quote[i-1]);
        user_quote_time[i] = user_quote_time[i-1];
    }
    Bstrcpy(user_quote[0],daquote);
    OSD_Printf("%s\n",daquote);

    user_quote_time[0] = ud.msgdisptime;
    pub = NUMPAGES;
}

int32_t lastpackettime = 0;

#include "sync.c"

void G_HandleSpecialKeys(void)
{
    CONTROL_ProcessBinds();

    if (ALT_IS_PRESSED && KB_KeyPressed(sc_Enter))
    {
        if (setgamemode(!ud.config.ScreenMode,ud.config.ScreenWidth,ud.config.ScreenHeight,ud.config.ScreenBPP))
        {
            OSD_Printf(OSD_ERROR "Failed setting fullscreen video mode.\n");
            if (setgamemode(ud.config.ScreenMode, ud.config.ScreenWidth, ud.config.ScreenHeight, ud.config.ScreenBPP))
                G_GameExit("Failed to recover from failure to set fullscreen video mode.\n");
        }
        else ud.config.ScreenMode = !ud.config.ScreenMode;
        KB_ClearKeyDown(sc_Enter);
        g_restorePalette = 1;
        G_UpdateScreenArea();
    }

    if (KB_UnBoundKeyPressed(sc_F12))
    {
        KB_ClearKeyDown(sc_F12);
        screencapture("duke0000.tga",0);
        P_DoQuote(103,g_player[myconnectindex].ps);
    }

    // only dispatch commands here when not in a game
    if (!(g_player[myconnectindex].ps->gm&MODE_GAME))
    {
        OSD_DispatchQueued();
    }

    if (qe == 0 && KB_KeyPressed(sc_LeftControl) && KB_KeyPressed(sc_LeftAlt) && (KB_KeyPressed(sc_Delete)||KB_KeyPressed(sc_End)))
    {
        qe = 1;
        G_GameExit("Quick Exit.");
    }
}

void Net_GetPackets(void)
{
    int32_t i, j, k, l;
    int32_t other;
    int32_t packbufleng;

    input_t *osyn, *nsyn;

    sampletimer();
    MUSIC_Update();

    G_HandleSpecialKeys();

    if (numplayers < 2) return;
    while ((packbufleng = mmulti_getpacket(&other,packbuf)) > 0)
    {
        lastpackettime = totalclock;
#if 0
        initprintf("RECEIVED PACKET: type: %d : len %d\n", packbuf[0], packbufleng);
#endif
        switch (packbuf[0])
        {
        case PACKET_MASTER_TO_SLAVE:  //[0] (receive master sync buffer)
            j = 1;

            if ((g_player[other].movefifoend&(TIMERUPDATESIZ-1)) == 0)
                for (i=connectpoint2[connecthead]; i>=0; i=connectpoint2[i])
                {
                    if (g_player[i].playerquitflag == 0) continue;
                    if (i == myconnectindex)
                        otherminlag = (int32_t)((int8_t)packbuf[j]);
                    j++;
                }

            osyn = (input_t *)&inputfifo[(g_player[connecthead].movefifoend-1)&(MOVEFIFOSIZ-1)][0];
            nsyn = (input_t *)&inputfifo[(g_player[connecthead].movefifoend)&(MOVEFIFOSIZ-1)][0];

            k = j;
            TRAVERSE_CONNECT(i)
            j += g_player[i].playerquitflag+g_player[i].playerquitflag;
            TRAVERSE_CONNECT(i)
            {
                if (g_player[i].playerquitflag == 0) continue;

                l = packbuf[k]+(int32_t)(packbuf[k+1]<<8);
                k += 2;

                if (i == myconnectindex)
                {
                    j += ((l&1)<<1)+(l&2)+((l&4)>>2)+((l&8)>>3)+((l&16)>>4)+((l&32)>>5)+((l&64)>>6)+((l&128)>>7)+((l&256)>>8)/*+((l&512)>>9)+((l&1024)>>10)+((l&2048)>>11)*/;
                    continue;
                }

                copybufbyte(&osyn[i],&nsyn[i],sizeof(input_t));
                if (l&1)   nsyn[i].fvel = packbuf[j]+((int16_t)packbuf[j+1]<<8), j += 2;
                if (l&2)   nsyn[i].svel = packbuf[j]+((int16_t)packbuf[j+1]<<8), j += 2;
                if (l&4)   nsyn[i].avel = (int8_t)packbuf[j++];
                if (l&8)   nsyn[i].bits = ((nsyn[i].bits&0xffffff00)|((int32_t)packbuf[j++]));
                if (l&16)  nsyn[i].bits = ((nsyn[i].bits&0xffff00ff)|((int32_t)packbuf[j++])<<8);
                if (l&32)  nsyn[i].bits = ((nsyn[i].bits&0xff00ffff)|((int32_t)packbuf[j++])<<16);
                if (l&64)  nsyn[i].bits = ((nsyn[i].bits&0x00ffffff)|((int32_t)packbuf[j++])<<24);
                if (l&128) nsyn[i].horz = (int8_t)packbuf[j++];
                if (l&256)  nsyn[i].extbits = (uint8_t)packbuf[j++];
                /*                if (l&256)  nsyn[i].extbits = ((nsyn[i].extbits&0xffffff00)|((int32_t)packbuf[j++]));
                                if (l&512)  nsyn[i].extbits = ((nsyn[i].extbits&0xffff00ff)|((int32_t)packbuf[j++])<<8);
                                if (l&1024) nsyn[i].extbits = ((nsyn[i].extbits&0xff00ffff)|((int32_t)packbuf[j++])<<16);
                                if (l&2048) nsyn[i].extbits = ((nsyn[i].extbits&0x00ffffff)|((int32_t)packbuf[j++])<<24); */

                if (TEST_SYNC_KEY(nsyn[i].bits,SK_GAMEQUIT)) g_player[i].playerquitflag = 0;
                g_player[i].movefifoend++;
            }

            Net_GetSyncData(packbuf, packbufleng, &j, other);

            TRAVERSE_CONNECT(i)
            if (i != myconnectindex)
                for (j=g_movesPerPacket-1; j>=1; j--)
                {
                    copybufbyte(&nsyn[i],&inputfifo[g_player[i].movefifoend&(MOVEFIFOSIZ-1)][i],sizeof(input_t));
                    g_player[i].movefifoend++;
                }

            movefifosendplc += g_movesPerPacket;

            break;
        case PACKET_SLAVE_TO_MASTER:  //[1] (receive slave sync buffer)
            j = 3;
            k = packbuf[1] + (int32_t)(packbuf[2]<<8);

            osyn = (input_t *)&inputfifo[(g_player[other].movefifoend-1)&(MOVEFIFOSIZ-1)][0];
            nsyn = (input_t *)&inputfifo[(g_player[other].movefifoend)&(MOVEFIFOSIZ-1)][0];

            copybufbyte(&osyn[other],&nsyn[other],sizeof(input_t));
            if (k&1)   nsyn[other].fvel = packbuf[j]+((int16_t)packbuf[j+1]<<8), j += 2;
            if (k&2)   nsyn[other].svel = packbuf[j]+((int16_t)packbuf[j+1]<<8), j += 2;
            if (k&4)   nsyn[other].avel = (int8_t)packbuf[j++];
            if (k&8)   nsyn[other].bits = ((nsyn[other].bits&0xffffff00)|((int32_t)packbuf[j++]));
            if (k&16)  nsyn[other].bits = ((nsyn[other].bits&0xffff00ff)|((int32_t)packbuf[j++])<<8);
            if (k&32)  nsyn[other].bits = ((nsyn[other].bits&0xff00ffff)|((int32_t)packbuf[j++])<<16);
            if (k&64)  nsyn[other].bits = ((nsyn[other].bits&0x00ffffff)|((int32_t)packbuf[j++])<<24);
            if (k&128) nsyn[other].horz = (int8_t)packbuf[j++];
            if (k&256) nsyn[other].extbits = (uint8_t)packbuf[j++];
            /*            if (k&256)  nsyn[other].extbits = ((nsyn[other].extbits&0xffffff00)|((int32_t)packbuf[j++]));
                        if (k&512)  nsyn[other].extbits = ((nsyn[other].extbits&0xffff00ff)|((int32_t)packbuf[j++])<<8);
                        if (k&1024) nsyn[other].extbits = ((nsyn[other].extbits&0xff00ffff)|((int32_t)packbuf[j++])<<16);
                        if (k&2048) nsyn[other].extbits = ((nsyn[other].extbits&0x00ffffff)|((int32_t)packbuf[j++])<<24); */
            g_player[other].movefifoend++;

            Net_GetSyncData(packbuf, packbufleng, &j, other);

            for (i=g_movesPerPacket-1; i>=1; i--)
            {
                copybufbyte(&nsyn[other],&inputfifo[g_player[other].movefifoend&(MOVEFIFOSIZ-1)][other],sizeof(input_t));
                g_player[other].movefifoend++;
            }

            break;

        case PACKET_BROADCAST:
            g_player[other].movefifoend = movefifoplc = movefifosendplc = predictfifoplc = 0;
            g_player[other].syncvalhead = syncvaltottail = 0L;
        case SERVER_GENERATED_BROADCAST:
            j = 1;

            if ((g_player[other].movefifoend&(TIMERUPDATESIZ-1)) == 0)
                if (other == connecthead)
                    for (i=connectpoint2[connecthead]; i>=0; i=connectpoint2[i])
                    {
                        if (i == myconnectindex)
                            otherminlag = (int32_t)((int8_t)packbuf[j]);
                        j++;
                    }

            osyn = (input_t *)&inputfifo[(g_player[other].movefifoend-1)&(MOVEFIFOSIZ-1)][0];
            nsyn = (input_t *)&inputfifo[(g_player[other].movefifoend)&(MOVEFIFOSIZ-1)][0];

            copybufbyte(&osyn[other],&nsyn[other],sizeof(input_t));
            k = packbuf[j] + (int32_t)(packbuf[j+1]<<8);
            j += 2;

            if (k&1)   nsyn[other].fvel = packbuf[j]+((int16_t)packbuf[j+1]<<8), j += 2;
            if (k&2)   nsyn[other].svel = packbuf[j]+((int16_t)packbuf[j+1]<<8), j += 2;
            if (k&4)   nsyn[other].avel = (int8_t)packbuf[j++];
            if (k&8)   nsyn[other].bits = ((nsyn[other].bits&0xffffff00)|((int32_t)packbuf[j++]));
            if (k&16)  nsyn[other].bits = ((nsyn[other].bits&0xffff00ff)|((int32_t)packbuf[j++])<<8);
            if (k&32)  nsyn[other].bits = ((nsyn[other].bits&0xff00ffff)|((int32_t)packbuf[j++])<<16);
            if (k&64)  nsyn[other].bits = ((nsyn[other].bits&0x00ffffff)|((int32_t)packbuf[j++])<<24);
            if (k&128) nsyn[other].horz = (int8_t)packbuf[j++];
            if (k&256) nsyn[other].extbits = (uint8_t)packbuf[j++];
            /*            if (k&256)  nsyn[other].extbits = ((nsyn[other].extbits&0xffffff00)|((int32_t)packbuf[j++]));
                        if (k&512)  nsyn[other].extbits = ((nsyn[other].extbits&0xffff00ff)|((int32_t)packbuf[j++])<<8);
                        if (k&1024) nsyn[other].extbits = ((nsyn[other].extbits&0xff00ffff)|((int32_t)packbuf[j++])<<16);
                        if (k&2048) nsyn[other].extbits = ((nsyn[other].extbits&0x00ffffff)|((int32_t)packbuf[j++])<<24); */
            g_player[other].movefifoend++;

            for (i=g_movesPerPacket-1; i>=1; i--)
            {
                copybufbyte(&nsyn[other],&inputfifo[g_player[other].movefifoend&(MOVEFIFOSIZ-1)][other],sizeof(input_t));
                g_player[other].movefifoend++;
            }

            Net_GetSyncData(packbuf, packbufleng, &j, other);

            if (j > packbufleng)
                initprintf("INVALID GAME PACKET!!! (packet %d, %d too many bytes (%d %d))\n",packbuf[0],j-packbufleng,packbufleng,k);

            break;
        case PACKET_NULL_PACKET:
            break;

        case PACKET_PLAYER_READY:
            if (g_player[other].playerreadyflag == 0)
                initprintf("Player %d is ready\n", other);
            g_player[other].playerreadyflag++;
            return;
        case PACKET_QUIT:
            G_GameExit(" ");
            break;
        default:
            switch (packbuf[0])
            {
            case PACKET_MESSAGE:
                //slaves in M/S mode only send to master
                if ((!g_networkBroadcastMode) && (myconnectindex == connecthead))
                {
                    if (packbuf[1] == 255)
                    {
                        //Master re-transmits message to all others
                        for (i=connectpoint2[connecthead]; i>=0; i=connectpoint2[i])
                            if (i != other)
                                mmulti_sendpacket(i,packbuf,packbufleng);
                    }
                    else if (((int32_t)packbuf[1]) != myconnectindex)
                    {
                        //Master re-transmits message not intended for master
                        mmulti_sendpacket((int32_t)packbuf[1],packbuf,packbufleng);
                        break;
                    }
                }

                Bstrcpy(recbuf,packbuf+2);
                recbuf[packbufleng-2] = 0;

                G_AddUserQuote(recbuf);
                S_PlaySound(EXITMENUSOUND);

                pus = NUMPAGES;
                pub = NUMPAGES;

                break;

            case PACKET_NEW_GAME:
                //Slaves in M/S mode only send to master
                //Master re-transmits message to all others
                if ((!g_networkBroadcastMode) && (myconnectindex == connecthead))
                    for (i=connectpoint2[connecthead]; i>=0; i=connectpoint2[i])
                        if (i != other) mmulti_sendpacket(i,packbuf,packbufleng);

                if (vote_map != -1 || vote_episode != -1 || voting != -1)
                    G_AddUserQuote("VOTE SUCCEEDED");

                ud.m_level_number = ud.level_number = packbuf[1];
                ud.m_volume_number = ud.volume_number = packbuf[2];
                ud.m_player_skill = ud.player_skill = packbuf[3];
                ud.m_monsters_off = ud.monsters_off = packbuf[4];
                ud.m_respawn_monsters = ud.respawn_monsters = packbuf[5];
                ud.m_respawn_items = ud.respawn_items = packbuf[6];
                ud.m_respawn_inventory = ud.respawn_inventory = packbuf[7];
                ud.m_coop = packbuf[8];
                ud.m_marker = ud.marker = packbuf[9];
                ud.m_ffire = ud.ffire = packbuf[10];
                ud.m_noexits = ud.noexits = packbuf[11];

                TRAVERSE_CONNECT(i)
                {
                    P_ResetWeapons(i);
                    P_ResetInventory(i);
                }

                G_NewGame(ud.volume_number,ud.level_number,ud.player_skill);
                ud.coop = ud.m_coop;

                if (G_EnterLevel(MODE_GAME)) G_BackToMenu();

                break;

            case PACKET_VERSION:
                //slaves in M/S mode only send to master
                //Master re-transmits message to all others
                if ((!g_networkBroadcastMode) && (myconnectindex == connecthead))
                    for (i=connectpoint2[connecthead]; i>=0; i=connectpoint2[i])
                        if (i != other) mmulti_sendpacket(i,packbuf,packbufleng);

                if (packbuf[2] != (uint8_t)atoi(s_buildDate))
                {
                    initprintf("Player %d has version %d, expecting %d\n",packbuf[2],(uint8_t)atoi(s_buildDate));
                    G_GameExit("You cannot play with different versions of EDuke32!");
                }
                if (packbuf[3] != BYTEVERSION)
                {
                    initprintf("Player %d has version %d, expecting %d\n",packbuf[3],BYTEVERSION);
                    G_GameExit("You cannot play Duke with different versions!");
                }
                if (packbuf[4] > g_numSyncBytes)
                {
                    initprintf("Sync debugging enabled\n");
                    g_numSyncBytes = packbuf[4];
                }

                break;

            case PACKET_PLAYER_OPTIONS:
                //slaves in M/S mode only send to master
                //Master re-transmits message to all others
                if ((!g_networkBroadcastMode) && (myconnectindex == connecthead))
                    for (i=connectpoint2[connecthead]; i>=0; i=connectpoint2[i])
                        if (i != other) mmulti_sendpacket(i,packbuf,packbufleng);

                other = packbuf[1];
                i = 2;

                g_player[other].ps->aim_mode = packbuf[i++];
                g_player[other].ps->auto_aim = packbuf[i++];
                g_player[other].ps->weaponswitch = packbuf[i++];
                g_player[other].ps->palookup = g_player[other].pcolor = packbuf[i++];
                g_player[other].pteam = packbuf[i++];

                break;

            case PACKET_PLAYER_NAME:
                //slaves in M/S mode only send to master
                //Master re-transmits message to all others
                if ((!g_networkBroadcastMode) && (myconnectindex == connecthead))
                    for (i=connectpoint2[connecthead]; i>=0; i=connectpoint2[i])
                        if (i != other) mmulti_sendpacket(i,packbuf,packbufleng);

                other = packbuf[1];

                for (i=2; packbuf[i]; i++)
                    g_player[other].user_name[i-2] = packbuf[i];
                g_player[other].user_name[i-2] = 0;
                i++;

                break;

            case PACKET_WEAPON_CHOICE:
                //slaves in M/S mode only send to master
                //Master re-transmits message to all others
                if ((!g_networkBroadcastMode) && (myconnectindex == connecthead))
                    for (i=connectpoint2[connecthead]; i>=0; i=connectpoint2[i])
                        if (i != other) mmulti_sendpacket(i,packbuf,packbufleng);

                other = packbuf[1];

                i = 2;

                j = i; //This used to be Duke packet #9... now concatenated with Duke packet #6
                for (; i-j<10; i++) g_player[other].wchoice[i-j] = packbuf[i];

                break;

            case PACKET_RTS:
                //slaves in M/S mode only send to master
                //Master re-transmits message to all others
                if ((!g_networkBroadcastMode) && (myconnectindex == connecthead))
                    for (i=connectpoint2[connecthead]; i>=0; i=connectpoint2[i])
                        if (i != other) mmulti_sendpacket(i,packbuf,packbufleng);

                if (numlumps == 0) break;

                if (ud.config.SoundToggle == 0 || ud.lockout == 1 || ud.config.FXDevice < 0 || !(ud.config.VoiceToggle & 4))
                    break;
                rtsptr = (char *)RTS_GetSound(packbuf[1]-1);
                FX_PlayAuto3D(rtsptr,RTS_SoundLength(packbuf[1]-1),0,0,0,255,-packbuf[1]);
                g_RTSPlaying = 7;

                break;

            case PACKET_MENU_LEVEL_QUIT:
                //slaves in M/S mode only send to master
                if (myconnectindex == connecthead)
                {
                    //Master re-transmits message to all others
                    for (i=connectpoint2[connecthead]; i>=0; i=connectpoint2[i])
                        if (i != other)
                            mmulti_sendpacket(i,packbuf,packbufleng);
                }
                /*
                            j = packbuf[1];
                            playerquitflag[j] = 0;

                            j = -1;
                            for(i=connecthead;i>=0;i=connectpoint2[i])
                            {
                                if (g_player[i].playerquitflag) { j = i; continue; }

                                if (i == connecthead) connecthead = connectpoint2[connecthead];
                                else connectpoint2[j] = connectpoint2[i];

                                numplayers--;
                                ud.multimode--;

                                Bsprintf(buf,"%s is history!",g_player[i].user_name);
                                G_AddUserQuote(buf);

                                if (numplayers < 2)
                                    S_PlaySound(GENERIC_AMBIENCE17);

                                if(i == 0 && g_networkBroadcastMode == 0) */
                G_GameExit("Game aborted from menu; disconnected.");
                //            }

                break;

            case PACKET_USER_MAP:
                //slaves in M/S mode only send to master
                if (myconnectindex == connecthead)
                {
                    //Master re-transmits message to all others
                    for (i=connectpoint2[connecthead]; i>=0; i=connectpoint2[i])
                        if (i != other)
                            mmulti_sendpacket(i,packbuf,packbufleng);
                }

                Bstrcpy(boardfilename,packbuf+1);
                boardfilename[packbufleng-1] = 0;
                Bcorrectfilename(boardfilename,0);
                if (boardfilename[0] != 0)
                {
                    if ((i = kopen4loadfrommod(boardfilename,0)) < 0)
                    {
                        Bmemset(boardfilename,0,sizeof(boardfilename));
                        Net_SendUserMapName();
                    }
                    else kclose(i);
                }

                if (ud.m_level_number == 7 && ud.m_volume_number == 0 && boardfilename[0] == 0)
                    ud.m_level_number = 0;

                break;

            case PACKET_MAP_VOTE:
            case PACKET_MAP_VOTE_INITIATE:
            case PACKET_MAP_VOTE_CANCEL:

                if (myconnectindex == connecthead)
                {
                    //Master re-transmits message to all others
                    for (i=connectpoint2[connecthead]; i>=0; i=connectpoint2[i])
                        if (i != other)
                            mmulti_sendpacket(i,packbuf,packbufleng);
                }

                switch (packbuf[0])
                {
                case PACKET_MAP_VOTE:
                    if (voting == myconnectindex && g_player[(uint8_t)packbuf[1]].gotvote == 0)
                    {
                        g_player[(uint8_t)packbuf[1]].gotvote = 1;
                        g_player[(uint8_t)packbuf[1]].vote = packbuf[2];
                        Bsprintf(tempbuf,"CONFIRMED VOTE FROM %s",g_player[(uint8_t)packbuf[1]].user_name);
                        G_AddUserQuote(tempbuf);
                    }
                    break;

                case PACKET_MAP_VOTE_INITIATE: // call map vote
                    /*                    if (g_networkBroadcastMode == 0 && packbuf[1] == connecthead)
                                            break; // ignore this from master */
                    voting = packbuf[1];
                    vote_episode = packbuf[2];
                    vote_map = packbuf[3];

                    Bsprintf(tempbuf,"%s^00 HAS CALLED A VOTE TO CHANGE MAP TO %s (E%dL%d)",
                             g_player[(uint8_t)packbuf[1]].user_name,
                             MapInfo[(uint8_t)(packbuf[2]*MAXLEVELS + packbuf[3])].name,
                             packbuf[2]+1,packbuf[3]+1);
                    G_AddUserQuote(tempbuf);

                    Bsprintf(tempbuf,"PRESS F1 TO ACCEPT, F2 TO DECLINE");
                    G_AddUserQuote(tempbuf);

                    for (i=MAXPLAYERS-1; i>=0; i--)
                    {
                        g_player[i].vote = 0;
                        g_player[i].gotvote = 0;
                    }
                    g_player[voting].gotvote = g_player[voting].vote = 1;
                    break;

                case PACKET_MAP_VOTE_CANCEL: // cancel map vote
                    if (voting == packbuf[1])
                    {
                        voting = -1;
                        i = 0;
                        for (j=MAXPLAYERS-1; j>=0; j--)
                            i += g_player[j].gotvote;

                        if (i != numplayers)
                            Bsprintf(tempbuf,"%s^00 HAS CANCELED THE VOTE",g_player[(uint8_t)packbuf[1]].user_name);
                        else Bsprintf(tempbuf,"VOTE FAILED");
                        for (i=MAXPLAYERS-1; i>=0; i--)
                        {
                            g_player[i].vote = 0;
                            g_player[i].gotvote = 0;
                        }
                        G_AddUserQuote(tempbuf);
                    }
                    break;
                }
                break;

            case PACKET_LOAD_GAME:
                //Slaves in M/S mode only send to master
                //Master re-transmits message to all others
                if ((!g_networkBroadcastMode) && (myconnectindex == connecthead))
                    for (i=connectpoint2[connecthead]; i>=0; i=connectpoint2[i])
                        if (i != other) mmulti_sendpacket(i,packbuf,packbufleng);

                multiflag = 2;
                multiwhat = 0;
                multiwho = packbuf[2]; //other: need to send in m/s mode because of possible re-transmit
                multipos = packbuf[1];
                G_LoadPlayer(multipos);
                multiflag = 0;
                break;
            }
            break;
        }
    }
}

void faketimerhandler(void)
{
    int32_t i, j, k;
    //    short who;
    input_t *osyn, *nsyn;

    if (qe == 0 && KB_KeyPressed(sc_LeftControl) && KB_KeyPressed(sc_LeftAlt) && KB_KeyPressed(sc_Delete))
    {
        qe = 1;
        G_GameExit("Quick Exit.");
    }

    sampletimer();
    MUSIC_Update();

    if ((totalclock < ototalclock+TICSPERFRAME) || (ready2send == 0)) return;
    ototalclock += TICSPERFRAME;

    Net_GetPackets();

    if (g_player[myconnectindex].movefifoend - movefifoplc >= 100)
        return;

    getinput(myconnectindex);

    avgfvel += loc.fvel;
    avgsvel += loc.svel;
    avgavel += loc.avel;
    avghorz += loc.horz;
    avgbits |= loc.bits;
    avgextbits |= loc.extbits;

    if (g_player[myconnectindex].movefifoend&(g_movesPerPacket-1))
    {
        copybufbyte(&inputfifo[(g_player[myconnectindex].movefifoend-1)&(MOVEFIFOSIZ-1)][myconnectindex],
                    &inputfifo[g_player[myconnectindex].movefifoend&(MOVEFIFOSIZ-1)][myconnectindex],sizeof(input_t));
        g_player[myconnectindex].movefifoend++;
        return;
    }

    nsyn = &inputfifo[g_player[myconnectindex].movefifoend&(MOVEFIFOSIZ-1)][myconnectindex];
    nsyn[0].fvel = avgfvel/g_movesPerPacket;
    nsyn[0].svel = avgsvel/g_movesPerPacket;
    nsyn[0].avel = avgavel/g_movesPerPacket;
    nsyn[0].horz = avghorz/g_movesPerPacket;
    nsyn[0].bits = avgbits;
    nsyn[0].extbits = avgextbits;
    avgfvel = avgsvel = avgavel = avghorz = avgbits = avgextbits = 0;
    g_player[myconnectindex].movefifoend++;

    if (numplayers < 2)
    {
        if (ud.multimode > 1)
            TRAVERSE_CONNECT(i)
            if (i != myconnectindex)
            {
                //clearbufbyte(&inputfifo[g_player[i].movefifoend&(MOVEFIFOSIZ-1)][i],sizeof(input_t),0L);
                if (ud.playerai)
                    computergetinput(i,&inputfifo[g_player[i].movefifoend&(MOVEFIFOSIZ-1)][i]);
                inputfifo[g_player[i].movefifoend&(MOVEFIFOSIZ-1)][i].svel++;
                inputfifo[g_player[i].movefifoend&(MOVEFIFOSIZ-1)][i].fvel++;
                g_player[i].movefifoend++;
            }
        return;
    }

    TRAVERSE_CONNECT(i)
    if (i != myconnectindex)
    {
        k = (g_player[myconnectindex].movefifoend-1)-g_player[i].movefifoend;
        g_player[i].myminlag = min(g_player[i].myminlag,k);
        mymaxlag = max(mymaxlag,k);
    }

#if 0
    if (((g_player[myconnectindex].movefifoend - 1) & (TIMERUPDATESIZ - 1)) == 0)
    {
        i = mymaxlag - bufferjitter;
        mymaxlag = 0;
        if (i > 0)
            bufferjitter += ((2 + i) >> 2);
        else if (i < 0)
            bufferjitter -= ((2 - i) >> 2);
    }
#else
    if (((g_player[myconnectindex].movefifoend-1)&(TIMERUPDATESIZ-1)) == 0)
    {
        i = mymaxlag-bufferjitter; mymaxlag = 0;
        if (i > 0) bufferjitter += ((3+i)>>2);
        else if (i < 0) bufferjitter -= ((1-i)>>2);
    }
#endif

    if (g_networkBroadcastMode == 1)
    {
        packbuf[0] = SERVER_GENERATED_BROADCAST;
        if ((g_player[myconnectindex].movefifoend-1) == 0) packbuf[0] = PACKET_BROADCAST;
        j = 1;

        //Fix timers and buffer/jitter value
        if (((g_player[myconnectindex].movefifoend-1)&(TIMERUPDATESIZ-1)) == 0)
        {
            if (myconnectindex != connecthead)
            {
                i = g_player[connecthead].myminlag-otherminlag;
                if (klabs(i) > 8) i >>= 1;
                else if (klabs(i) > 2) i = ksgn(i);
                else i = 0;

                totalclock -= TICSPERFRAME*i;
                g_player[connecthead].myminlag -= i; otherminlag += i;
            }
            else
            {
                for (i=connectpoint2[connecthead]; i>=0; i=connectpoint2[i])
                    packbuf[j++] = min(max(g_player[i].myminlag,-128),127);
            }

            TRAVERSE_CONNECT(i)
            g_player[i].myminlag = 0x7fffffff;
        }

        osyn = (input_t *)&inputfifo[(g_player[myconnectindex].movefifoend-2)&(MOVEFIFOSIZ-1)][myconnectindex];
        nsyn = (input_t *)&inputfifo[(g_player[myconnectindex].movefifoend-1)&(MOVEFIFOSIZ-1)][myconnectindex];

        k = j;
        packbuf[j++] = 0;
        packbuf[j++] = 0;

        if (nsyn[0].fvel != osyn[0].fvel)
        {
            packbuf[j++] = (uint8_t)nsyn[0].fvel;
            packbuf[j++] = (uint8_t)(nsyn[0].fvel>>8);
            packbuf[k] |= 1;
        }
        if (nsyn[0].svel != osyn[0].svel)
        {
            packbuf[j++] = (uint8_t)nsyn[0].svel;
            packbuf[j++] = (uint8_t)(nsyn[0].svel>>8);
            packbuf[k] |= 2;
        }
        if (nsyn[0].avel != osyn[0].avel)
        {
            packbuf[j++] = (int8_t)nsyn[0].avel;
            packbuf[k] |= 4;
        }
        if ((nsyn[0].bits^osyn[0].bits)&0x000000ff) packbuf[j++] = (nsyn[0].bits&255), packbuf[k] |= 8;
        if ((nsyn[0].bits^osyn[0].bits)&0x0000ff00) packbuf[j++] = ((nsyn[0].bits>>8)&255), packbuf[k] |= 16;
        if ((nsyn[0].bits^osyn[0].bits)&0x00ff0000) packbuf[j++] = ((nsyn[0].bits>>16)&255), packbuf[k] |= 32;
        if ((nsyn[0].bits^osyn[0].bits)&0xff000000) packbuf[j++] = ((nsyn[0].bits>>24)&255), packbuf[k] |= 64;
        if (nsyn[0].horz != osyn[0].horz)
        {
            packbuf[j++] = (uint8_t)nsyn[0].horz;
            packbuf[k] |= 128;
        }
        packbuf[++k] = 0;

        if (nsyn[0].extbits != osyn[0].extbits) packbuf[j++] = nsyn[0].extbits, packbuf[k] |= 1;
        /*        if ((nsyn[0].extbits^osyn[0].extbits)&0x000000ff) packbuf[j++] = (nsyn[0].extbits&255), packbuf[k] |= 1;
                if ((nsyn[0].extbits^osyn[0].extbits)&0x0000ff00) packbuf[j++] = ((nsyn[0].extbits>>8)&255), packbuf[k] |= 2;
                if ((nsyn[0].extbits^osyn[0].extbits)&0x00ff0000) packbuf[j++] = ((nsyn[0].extbits>>16)&255), packbuf[k] |= 4;
                if ((nsyn[0].extbits^osyn[0].extbits)&0xff000000) packbuf[j++] = ((nsyn[0].extbits>>24)&255), packbuf[k] |= 8; */

        /*        while (g_player[myconnectindex].syncvalhead != syncvaltail)
                {
                    packbuf[j++] = g_player[myconnectindex].syncval[syncvaltail&(MOVEFIFOSIZ-1)];
                    syncvaltail++;
                } */

        Net_AddSyncData(&j);

        TRAVERSE_CONNECT(i)
        if (i != myconnectindex)
            mmulti_sendpacket(i,packbuf,j);

        return;
    }
    if (myconnectindex != connecthead)   //Slave
    {
        //Fix timers and buffer/jitter value
        if (((g_player[myconnectindex].movefifoend-1)&(TIMERUPDATESIZ-1)) == 0)
        {
            i = g_player[connecthead].myminlag-otherminlag;
            if (klabs(i) > 8) i >>= 1;
            else if (klabs(i) > 2) i = ksgn(i);
            else i = 0;

            totalclock -= TICSPERFRAME*i;
            g_player[connecthead].myminlag -= i; otherminlag += i;

            TRAVERSE_CONNECT(i)
            g_player[i].myminlag = 0x7fffffff;
        }

        packbuf[0] = PACKET_SLAVE_TO_MASTER;
        packbuf[1] = 0;
        packbuf[2] = 0;
        j = 3;

        osyn = (input_t *)&inputfifo[(g_player[myconnectindex].movefifoend-2)&(MOVEFIFOSIZ-1)][myconnectindex];
        nsyn = (input_t *)&inputfifo[(g_player[myconnectindex].movefifoend-1)&(MOVEFIFOSIZ-1)][myconnectindex];

        if (nsyn[0].fvel != osyn[0].fvel)
        {
            packbuf[j++] = (uint8_t)nsyn[0].fvel;
            packbuf[j++] = (uint8_t)(nsyn[0].fvel>>8);
            packbuf[1] |= 1;
        }
        if (nsyn[0].svel != osyn[0].svel)
        {
            packbuf[j++] = (uint8_t)nsyn[0].svel;
            packbuf[j++] = (uint8_t)(nsyn[0].svel>>8);
            packbuf[1] |= 2;
        }
        if (nsyn[0].avel != osyn[0].avel)
        {
            packbuf[j++] = (int8_t)nsyn[0].avel;
            packbuf[1] |= 4;
        }
        if ((nsyn[0].bits^osyn[0].bits)&0x000000ff) packbuf[j++] = (nsyn[0].bits&255), packbuf[1] |= 8;
        if ((nsyn[0].bits^osyn[0].bits)&0x0000ff00) packbuf[j++] = ((nsyn[0].bits>>8)&255), packbuf[1] |= 16;
        if ((nsyn[0].bits^osyn[0].bits)&0x00ff0000) packbuf[j++] = ((nsyn[0].bits>>16)&255), packbuf[1] |= 32;
        if ((nsyn[0].bits^osyn[0].bits)&0xff000000) packbuf[j++] = ((nsyn[0].bits>>24)&255), packbuf[1] |= 64;
        if (nsyn[0].horz != osyn[0].horz)
        {
            packbuf[j++] = (uint8_t)nsyn[0].horz;
            packbuf[1] |= 128;
        }
        packbuf[2] = 0;
        if (nsyn[0].extbits != osyn[0].extbits) packbuf[j++] = nsyn[0].extbits, packbuf[2] |= 1;
        /*        if ((nsyn[0].extbits^osyn[0].extbits)&0x000000ff) packbuf[j++] = (nsyn[0].extbits&255), packbuf[2] |= 1;
                if ((nsyn[0].extbits^osyn[0].extbits)&0x0000ff00) packbuf[j++] = ((nsyn[0].extbits>>8)&255), packbuf[2] |= 2;
                if ((nsyn[0].extbits^osyn[0].extbits)&0x00ff0000) packbuf[j++] = ((nsyn[0].extbits>>16)&255), packbuf[2] |= 4;
                if ((nsyn[0].extbits^osyn[0].extbits)&0xff000000) packbuf[j++] = ((nsyn[0].extbits>>24)&255), packbuf[2] |= 8; */

        /*        while (g_player[myconnectindex].syncvalhead != syncvaltail)
                {
                    packbuf[j++] = g_player[myconnectindex].syncval[syncvaltail&(MOVEFIFOSIZ-1)];
                    syncvaltail++;
                } */
        Net_AddSyncData(&j);

        mmulti_sendpacket(connecthead,packbuf,j);
        return;
    }

    //This allows allow packet resends
    TRAVERSE_CONNECT(i)
    if (g_player[i].movefifoend <= movefifosendplc)
    {
        packbuf[0] = PACKET_NULL_PACKET;
        for (i=connectpoint2[connecthead]; i>=0; i=connectpoint2[i])
            mmulti_sendpacket(i,packbuf,1);
        return;
    }

    while (1)  //Master
    {
        TRAVERSE_CONNECT(i)
        if (g_player[i].playerquitflag && (g_player[i].movefifoend <= movefifosendplc)) return;

        osyn = (input_t *)&inputfifo[(movefifosendplc-1)&(MOVEFIFOSIZ-1)][0];
        nsyn = (input_t *)&inputfifo[(movefifosendplc)&(MOVEFIFOSIZ-1)][0];

        //MASTER -> SLAVE packet
        packbuf[0] = PACKET_MASTER_TO_SLAVE;
        j = 1;

        //Fix timers and buffer/jitter value
        if ((movefifosendplc&(TIMERUPDATESIZ-1)) == 0)
        {
            for (i=connectpoint2[connecthead]; i>=0; i=connectpoint2[i])
                if (g_player[i].playerquitflag)
                    packbuf[j++] = min(max(g_player[i].myminlag,-128),127);

            TRAVERSE_CONNECT(i)
            g_player[i].myminlag = 0x7fffffff;
        }

        k = j;

        TRAVERSE_CONNECT(i)
        j += g_player[i].playerquitflag + g_player[i].playerquitflag;

        TRAVERSE_CONNECT(i)
        {
            if (g_player[i].playerquitflag == 0) continue;

            packbuf[k] = 0;
            if (nsyn[i].fvel != osyn[i].fvel)
            {
                packbuf[j++] = (uint8_t)nsyn[i].fvel;
                packbuf[j++] = (uint8_t)(nsyn[i].fvel>>8);
                packbuf[k] |= 1;
            }
            if (nsyn[i].svel != osyn[i].svel)
            {
                packbuf[j++] = (uint8_t)nsyn[i].svel;
                packbuf[j++] = (uint8_t)(nsyn[i].svel>>8);
                packbuf[k] |= 2;
            }
            if (nsyn[i].avel != osyn[i].avel)
            {
                packbuf[j++] = (int8_t)nsyn[i].avel;
                packbuf[k] |= 4;
            }
            if ((nsyn[i].bits^osyn[i].bits)&0x000000ff) packbuf[j++] = (nsyn[i].bits&255), packbuf[k] |= 8;
            if ((nsyn[i].bits^osyn[i].bits)&0x0000ff00) packbuf[j++] = ((nsyn[i].bits>>8)&255), packbuf[k] |= 16;
            if ((nsyn[i].bits^osyn[i].bits)&0x00ff0000) packbuf[j++] = ((nsyn[i].bits>>16)&255), packbuf[k] |= 32;
            if ((nsyn[i].bits^osyn[i].bits)&0xff000000) packbuf[j++] = ((nsyn[i].bits>>24)&255), packbuf[k] |= 64;
            if (nsyn[i].horz != osyn[i].horz)
            {
                packbuf[j++] = (uint8_t)nsyn[i].horz;
                packbuf[k] |= 128;
            }
            k++;
            packbuf[k] = 0;
            if (nsyn[i].extbits != osyn[i].extbits) packbuf[j++] = nsyn[i].extbits, packbuf[k] |= 1;
            /*
            if ((nsyn[i].extbits^osyn[i].extbits)&0x000000ff) packbuf[j++] = (nsyn[i].extbits&255), packbuf[k] |= 1;
            if ((nsyn[i].extbits^osyn[i].extbits)&0x0000ff00) packbuf[j++] = ((nsyn[i].extbits>>8)&255), packbuf[k] |= 2;
            if ((nsyn[i].extbits^osyn[i].extbits)&0x00ff0000) packbuf[j++] = ((nsyn[i].extbits>>16)&255), packbuf[k] |= 4;
            if ((nsyn[i].extbits^osyn[i].extbits)&0xff000000) packbuf[j++] = ((nsyn[i].extbits>>24)&255), packbuf[k] |= 8; */
            k++;
        }

        /*        while (g_player[myconnectindex].syncvalhead != syncvaltail)
                {
                    packbuf[j++] = g_player[myconnectindex].syncval[syncvaltail&(MOVEFIFOSIZ-1)];
                    syncvaltail++;
                } */
        Net_AddSyncData(&j);

        for (i=connectpoint2[connecthead]; i>=0; i=connectpoint2[i])
            if (g_player[i].playerquitflag)
            {
                mmulti_sendpacket(i,packbuf,j);
                if (TEST_SYNC_KEY(nsyn[i].bits,SK_GAMEQUIT))
                    g_player[i].playerquitflag = 0;
            }

        movefifosendplc += g_movesPerPacket;
    }
}

extern int32_t cacnum;
typedef struct { intptr_t *hand; int32_t leng; char *lock ; } cactype;
extern cactype cac[];

static void G_ShowCacheLocks(void)
{
    int16_t i,k;

    k = 0;
    for (i=cacnum-1; i>=0; i--)
        if ((*cac[i].lock) >= 200)
        {
            Bsprintf(tempbuf,"Locked- %d: Leng:%d, Lock:%d",i,cac[i].leng,*cac[i].lock);
            printext256(0L,k,31,-1,tempbuf,1);
            k += 6;
        }

    k += 6;

    for (i=10; i>=0; i--)
        if (lumplockbyte[i] >= 200)
        {
            Bsprintf(tempbuf,"RTS Locked %d:",i);
            printext256(0L,k,31,-1,tempbuf,1);
            k += 6;
        }
}

int32_t A_CheckInventorySprite(spritetype *s)
{
    switch (DynamicTileMap[s->picnum])
    {
    case FIRSTAID__STATIC:
    case STEROIDS__STATIC:
    case HEATSENSOR__STATIC:
    case BOOTS__STATIC:
    case JETPACK__STATIC:
    case HOLODUKE__STATIC:
    case AIRTANK__STATIC:
        return 1;
    }
    return 0;
}

void G_DrawTile(int32_t x, int32_t y, int32_t tilenum, int32_t shade, int32_t orientation)
{
    int32_t p = sector[g_player[screenpeek].ps->cursectnum].floorpal, a = 0;

    if (orientation&4)
        a = 1024;

    rotatesprite((orientation&256)?x:(x<<16),(orientation&256)?y:(y<<16),65536L,a,tilenum,shade,p,2|orientation,windowx1,windowy1,windowx2,windowy2);
}

void G_DrawTilePal(int32_t x, int32_t y, int32_t tilenum, int32_t shade, int32_t orientation, int32_t p)
{
    int32_t a = 0;

    if (orientation&4)
        a = 1024;

    rotatesprite((orientation&256)?x:(x<<16),(orientation&256)?y:(y<<16),65536L,a,tilenum,shade,p,2|orientation,windowx1,windowy1,windowx2,windowy2);
}

void G_DrawTileSmall(int32_t x, int32_t y, int32_t tilenum, int32_t shade, int32_t orientation)
{
    int32_t p = sector[g_player[screenpeek].ps->cursectnum].floorpal, a = 0;

    if (orientation&4)
        a = 1024;

    rotatesprite((orientation&256)?x:(x<<16),(orientation&256)?y:(y<<16),32768L,a,tilenum,shade,p,2|orientation,windowx1,windowy1,windowx2,windowy2);
}

void G_DrawTilePalSmall(int32_t x, int32_t y, int32_t tilenum, int32_t shade, int32_t orientation, int32_t p)
{
    int32_t a = 0;

    if (orientation&4)
        a = 1024;

    rotatesprite((orientation&256)?x:(x<<16),(orientation&256)?y:(y<<16),32768L,a,tilenum,shade,p,2|orientation,windowx1,windowy1,windowx2,windowy2);
}

#define POLYMOSTTRANS (1)
#define POLYMOSTTRANS2 (1|32)

// draws inventory numbers in the HUD for both the full and mini status bars
static void G_DrawInvNum(int32_t x,int32_t y,char num1,char ha,char sbits)
{
    char dabuf[80] = {0};
    int32_t shd = (x < 0);

    if (shd) x = -x;

    Bsprintf(dabuf,"%d",num1);
    if (num1 > 99)
    {
        if (shd && ud.screen_size == 4 && getrendermode() >= 3 && althud_shadows)
        {
            rotatesprite(sbarx(x-4+1),sbary(y+1),sbarsc(65536L),0,THREEBYFIVE+dabuf[0]-'0',ha,4,POLYMOSTTRANS|sbits,0,0,xdim-1,ydim-1);
            rotatesprite(sbarx(x+1),sbary(y+1),sbarsc(65536L),0,THREEBYFIVE+dabuf[1]-'0',ha,4,POLYMOSTTRANS|sbits,0,0,xdim-1,ydim-1);
            rotatesprite(sbarx(x+4+1),sbary(y+1),sbarsc(65536L),0,THREEBYFIVE+dabuf[2]-'0',ha,4,POLYMOSTTRANS|sbits,0,0,xdim-1,ydim-1);
        }
        rotatesprite(sbarx(x-4),sbary(y),sbarsc(65536L),0,THREEBYFIVE+dabuf[0]-'0',ha,0,sbits,0,0,xdim-1,ydim-1);
        rotatesprite(sbarx(x),sbary(y),sbarsc(65536L),0,THREEBYFIVE+dabuf[1]-'0',ha,0,sbits,0,0,xdim-1,ydim-1);
        rotatesprite(sbarx(x+4),sbary(y),sbarsc(65536L),0,THREEBYFIVE+dabuf[2]-'0',ha,0,sbits,0,0,xdim-1,ydim-1);
        return;
    }
    if (num1 > 9)
    {
        if (shd && ud.screen_size == 4 && getrendermode() >= 3 && althud_shadows)
        {
            rotatesprite(sbarx(x+1),sbary(y+1),sbarsc(65536L),0,THREEBYFIVE+dabuf[0]-'0',ha,4,POLYMOSTTRANS|sbits,0,0,xdim-1,ydim-1);
            rotatesprite(sbarx(x+4+1),sbary(y+1),sbarsc(65536L),0,THREEBYFIVE+dabuf[1]-'0',ha,4,POLYMOSTTRANS|sbits,0,0,xdim-1,ydim-1);
        }

        rotatesprite(sbarx(x),sbary(y),sbarsc(65536L),0,THREEBYFIVE+dabuf[0]-'0',ha,0,sbits,0,0,xdim-1,ydim-1);
        rotatesprite(sbarx(x+4),sbary(y),sbarsc(65536L),0,THREEBYFIVE+dabuf[1]-'0',ha,0,sbits,0,0,xdim-1,ydim-1);
        return;
    }
    rotatesprite(sbarx(x+4+1),sbary(y+1),sbarsc(65536L),0,THREEBYFIVE+dabuf[0]-'0',ha,4,sbits,0,0,xdim-1,ydim-1);
    rotatesprite(sbarx(x+4),sbary(y),sbarsc(65536L),0,THREEBYFIVE+dabuf[0]-'0',ha,0,sbits,0,0,xdim-1,ydim-1);
}

static void G_DrawWeapNum(int16_t ind,int32_t x,int32_t y,int32_t num1, int32_t num2,char ha)
{
    char dabuf[80] = {0};

    rotatesprite(sbarx(x-7),sbary(y),sbarsc(65536L),0,THREEBYFIVE+ind+1,ha-10,7,10,0,0,xdim-1,ydim-1);
    rotatesprite(sbarx(x-3),sbary(y),sbarsc(65536L),0,THREEBYFIVE+10,ha,0,10,0,0,xdim-1,ydim-1);

    if (VOLUMEONE && (ind > HANDBOMB_WEAPON || ind < 0))
    {
        minitextshade(x+1,y-4,"ORDER",20,11,2+8+16+256);
        return;
    }

    rotatesprite(sbarx(x+9),sbary(y),sbarsc(65536L),0,THREEBYFIVE+11,ha,0,10,0,0,xdim-1,ydim-1);

    if (num1 > 99) num1 = 99;
    if (num2 > 99) num2 = 99;

    Bsprintf(dabuf,"%d",num1);
    if (num1 > 9)
    {
        rotatesprite(sbarx(x),sbary(y),sbarsc(65536L),0,THREEBYFIVE+dabuf[0]-'0',ha,0,10,0,0,xdim-1,ydim-1);
        rotatesprite(sbarx(x+4),sbary(y),sbarsc(65536L),0,THREEBYFIVE+dabuf[1]-'0',ha,0,10,0,0,xdim-1,ydim-1);
    }
    else rotatesprite(sbarx(x+4),sbary(y),sbarsc(65536L),0,THREEBYFIVE+dabuf[0]-'0',ha,0,10,0,0,xdim-1,ydim-1);

    Bsprintf(dabuf,"%d",num2);
    if (num2 > 9)
    {
        rotatesprite(sbarx(x+13),sbary(y),sbarsc(65536L),0,THREEBYFIVE+dabuf[0]-'0',ha,0,10,0,0,xdim-1,ydim-1);
        rotatesprite(sbarx(x+17),sbary(y),sbarsc(65536L),0,THREEBYFIVE+dabuf[1]-'0',ha,0,10,0,0,xdim-1,ydim-1);
        return;
    }
    rotatesprite(sbarx(x+13),sbary(y),sbarsc(65536L),0,THREEBYFIVE+dabuf[0]-'0',ha,0,10,0,0,xdim-1,ydim-1);
}

static void G_DrawWeapNum2(char ind,int32_t x,int32_t y,int32_t num1, int32_t num2,char ha)
{
    char dabuf[80] = {0};

    rotatesprite(sbarx(x-7),sbary(y),sbarsc(65536L),0,THREEBYFIVE+ind+1,ha-10,7,10,0,0,xdim-1,ydim-1);
    rotatesprite(sbarx(x-4),sbary(y),sbarsc(65536L),0,THREEBYFIVE+10,ha,0,10,0,0,xdim-1,ydim-1);
    rotatesprite(sbarx(x+13),sbary(y),sbarsc(65536L),0,THREEBYFIVE+11,ha,0,10,0,0,xdim-1,ydim-1);

    Bsprintf(dabuf,"%d",num1);
    if (num1 > 99)
    {
        rotatesprite(sbarx(x),sbary(y),sbarsc(65536L),0,THREEBYFIVE+dabuf[0]-'0',ha,0,10,0,0,xdim-1,ydim-1);
        rotatesprite(sbarx(x+4),sbary(y),sbarsc(65536L),0,THREEBYFIVE+dabuf[1]-'0',ha,0,10,0,0,xdim-1,ydim-1);
        rotatesprite(sbarx(x+8),sbary(y),sbarsc(65536L),0,THREEBYFIVE+dabuf[2]-'0',ha,0,10,0,0,xdim-1,ydim-1);
    }
    else if (num1 > 9)
    {
        rotatesprite(sbarx(x+4),sbary(y),sbarsc(65536L),0,THREEBYFIVE+dabuf[0]-'0',ha,0,10,0,0,xdim-1,ydim-1);
        rotatesprite(sbarx(x+8),sbary(y),sbarsc(65536L),0,THREEBYFIVE+dabuf[1]-'0',ha,0,10,0,0,xdim-1,ydim-1);
    }
    else rotatesprite(sbarx(x+8),sbary(y),sbarsc(65536L),0,THREEBYFIVE+dabuf[0]-'0',ha,0,10,0,0,xdim-1,ydim-1);

    Bsprintf(dabuf,"%d",num2);
    if (num2 > 99)
    {
        rotatesprite(sbarx(x+17),sbary(y),sbarsc(65536L),0,THREEBYFIVE+dabuf[0]-'0',ha,0,10,0,0,xdim-1,ydim-1);
        rotatesprite(sbarx(x+21),sbary(y),sbarsc(65536L),0,THREEBYFIVE+dabuf[1]-'0',ha,0,10,0,0,xdim-1,ydim-1);
        rotatesprite(sbarx(x+25),sbary(y),sbarsc(65536L),0,THREEBYFIVE+dabuf[2]-'0',ha,0,10,0,0,xdim-1,ydim-1);
    }
    else if (num2 > 9)
    {
        rotatesprite(sbarx(x+17),sbary(y),sbarsc(65536L),0,THREEBYFIVE+dabuf[0]-'0',ha,0,10,0,0,xdim-1,ydim-1);
        rotatesprite(sbarx(x+21),sbary(y),sbarsc(65536L),0,THREEBYFIVE+dabuf[1]-'0',ha,0,10,0,0,xdim-1,ydim-1);
        return;
    }
    else
        rotatesprite(sbarx(x+25),sbary(y),sbarsc(65536L),0,THREEBYFIVE+dabuf[0]-'0',ha,0,10,0,0,xdim-1,ydim-1);
}

static void G_DrawWeapAmounts(DukePlayer_t *p,int32_t x,int32_t y,int32_t u)
{
    int32_t cw = p->curr_weapon;

    if (u&4)
    {
        if (u != -1) G_PatchStatusBar(88,178,88+37,178+6); //original code: (96,178,96+12,178+6);
        G_DrawWeapNum2(PISTOL_WEAPON,x,y,
                       p->ammo_amount[PISTOL_WEAPON],p->max_ammo_amount[PISTOL_WEAPON],
                       12-20*(cw == PISTOL_WEAPON));
    }
    if (u&8)
    {
        if (u != -1) G_PatchStatusBar(88,184,88+37,184+6); //original code: (96,184,96+12,184+6);
        G_DrawWeapNum2(SHOTGUN_WEAPON,x,y+6,
                       p->ammo_amount[SHOTGUN_WEAPON],p->max_ammo_amount[SHOTGUN_WEAPON],
                       (!p->gotweapon[SHOTGUN_WEAPON]*9)+12-18*
                       (cw == SHOTGUN_WEAPON));
    }
    if (u&16)
    {
        if (u != -1) G_PatchStatusBar(88,190,88+37,190+6); //original code: (96,190,96+12,190+6);
        G_DrawWeapNum2(CHAINGUN_WEAPON,x,y+12,
                       p->ammo_amount[CHAINGUN_WEAPON],p->max_ammo_amount[CHAINGUN_WEAPON],
                       (!p->gotweapon[CHAINGUN_WEAPON]*9)+12-18*
                       (cw == CHAINGUN_WEAPON));
    }
    if (u&32)
    {
        if (u != -1) G_PatchStatusBar(127,178,127+29,178+6); //original code: (135,178,135+8,178+6);
        G_DrawWeapNum(RPG_WEAPON,x+39,y,
                      p->ammo_amount[RPG_WEAPON],p->max_ammo_amount[RPG_WEAPON],
                      (!p->gotweapon[RPG_WEAPON]*9)+12-19*
                      (cw == RPG_WEAPON));
    }
    if (u&64)
    {
        if (u != -1) G_PatchStatusBar(127,184,127+29,184+6); //original code: (135,184,135+8,184+6);
        G_DrawWeapNum(HANDBOMB_WEAPON,x+39,y+6,
                      p->ammo_amount[HANDBOMB_WEAPON],p->max_ammo_amount[HANDBOMB_WEAPON],
                      (((!p->ammo_amount[HANDBOMB_WEAPON])|(!p->gotweapon[HANDBOMB_WEAPON]))*9)+12-19*
                      ((cw == HANDBOMB_WEAPON) || (cw == HANDREMOTE_WEAPON)));
    }
    if (u&128)
    {
        if (u != -1) G_PatchStatusBar(127,190,127+29,190+6); //original code: (135,190,135+8,190+6);

        if (p->subweapon&(1<<GROW_WEAPON))
            G_DrawWeapNum(SHRINKER_WEAPON,x+39,y+12,
                          p->ammo_amount[GROW_WEAPON],p->max_ammo_amount[GROW_WEAPON],
                          (!p->gotweapon[GROW_WEAPON]*9)+12-18*
                          (cw == GROW_WEAPON));
        else
            G_DrawWeapNum(SHRINKER_WEAPON,x+39,y+12,
                          p->ammo_amount[SHRINKER_WEAPON],p->max_ammo_amount[SHRINKER_WEAPON],
                          (!p->gotweapon[SHRINKER_WEAPON]*9)+12-18*
                          (cw == SHRINKER_WEAPON));
    }
    if (u&256)
    {
        if (u != -1) G_PatchStatusBar(158,178,162+29,178+6); //original code: (166,178,166+8,178+6);

        G_DrawWeapNum(DEVISTATOR_WEAPON,x+70,y,
                      p->ammo_amount[DEVISTATOR_WEAPON],p->max_ammo_amount[DEVISTATOR_WEAPON],
                      (!p->gotweapon[DEVISTATOR_WEAPON]*9)+12-18*
                      (cw == DEVISTATOR_WEAPON));
    }
    if (u&512)
    {
        if (u != -1) G_PatchStatusBar(158,184,162+29,184+6); //original code: (166,184,166+8,184+6);

        G_DrawWeapNum(TRIPBOMB_WEAPON,x+70,y+6,
                      p->ammo_amount[TRIPBOMB_WEAPON],p->max_ammo_amount[TRIPBOMB_WEAPON],
                      (!p->gotweapon[TRIPBOMB_WEAPON]*9)+12-18*
                      (cw == TRIPBOMB_WEAPON));
    }

    if (u&65536L)
    {
        if (u != -1) G_PatchStatusBar(158,190,162+29,190+6); //original code: (166,190,166+8,190+6);

        G_DrawWeapNum(-1,x+70,y+12,
                      p->ammo_amount[FREEZE_WEAPON],p->max_ammo_amount[FREEZE_WEAPON],
                      (!p->gotweapon[FREEZE_WEAPON]*9)+12-18*
                      (cw == FREEZE_WEAPON));
    }
}

static void G_DrawDigiNum(int32_t x,int32_t y,int32_t n,char s,char cs)
{
    int32_t i, j = 0, k, p, c;
    char b[10];

    Bsnprintf(b,10,"%d",n);
    i = Bstrlen(b);

    for (k=i-1; k>=0; k--)
    {
        p = DIGITALNUM+*(b+k)-'0';
        j += tilesizx[p]+1;
    }
    c = x-(j>>1);

    j = 0;
    for (k=0; k<i; k++)
    {
        p = DIGITALNUM+*(b+k)-'0';
        rotatesprite(sbarx(c+j),sbary(y),sbarsc(65536L),0,p,s,0,cs,0,0,xdim-1,ydim-1);
        j += tilesizx[p]+1;
    }
}

void G_DrawTXDigiNumZ(int32_t starttile, int32_t x,int32_t y,int32_t n,int32_t s,int32_t pal,int32_t cs,int32_t x1, int32_t y1, int32_t x2, int32_t y2, int32_t z)
{
    int32_t i, j = 0, k, p, c;
    char b[10];
    int32_t shift = (cs&256)?0:16;

    //ltoa(n,b,10);
    Bsnprintf(b,10,"%d",n);
    i = Bstrlen(b);

    for (k=i-1; k>=0; k--)
    {
        p = starttile+*(b+k)-'0';
        j += (tilesizx[p]+1)*z/65536;
    }
    if (cs&256) j<<=16;
    c = x-(j>>1);

    j = 0;
    for (k=0; k<i; k++)
    {
        p = starttile+*(b+k)-'0';
        rotatesprite((c+j)<<shift,y<<shift,z,0,p,s,pal,2|cs,x1,y1,x2,y2);
        /*        rotatesprite((c+j)<<16,y<<16,65536L,0,p,s,pal,cs,0,0,xdim-1,ydim-1);
                rotatesprite(x<<16,y<<16,32768L,a,tilenum,shade,p,2|orientation,windowx1,windowy1,windowx2,windowy2);*/
        j += ((tilesizx[p]+1)*z/65536)<<((cs&256)?16:0);
    }
}

static void G_DrawAltDigiNum(int32_t x,int32_t y,int32_t n,char s,char cs)
{
    int32_t i, j = 0, k, p, c;
    char b[10];
    int32_t rev = (x < 0);
    int32_t shd = (y < 0);

    if (rev) x = -x;
    if (shd) y = -y;

    Bsnprintf(b,10,"%d",n);
    i = Bstrlen(b);

    for (k=i-1; k>=0; k--)
    {
        p = althud_numbertile+*(b+k)-'0';
        j += tilesizx[p]+1;
    }
    c = x-(j>>1);

    if (rev)
    {
//        j = 0;
        for (k=0; k<i; k++)
        {
            p = althud_numbertile+*(b+k)-'0';
            if (shd && getrendermode() >= 3 && althud_shadows)
                rotatesprite(sbarxr(c+j-1),sbary(y+1),sbarsc(65536L),0,p,s,4,cs|POLYMOSTTRANS2,0,0,xdim-1,ydim-1);
            rotatesprite(sbarxr(c+j),sbary(y),sbarsc(65536L),0,p,s,althud_numberpal,cs,0,0,xdim-1,ydim-1);
            j -= tilesizx[p]+1;
        }
        return;
    }
    j = 0;
    for (k=0; k<i; k++)
    {
        p = althud_numbertile+*(b+k)-'0';
        if (shd && getrendermode() >= 3 && althud_shadows)
            rotatesprite(sbarx(c+j+1),sbary(y+1),sbarsc(65536L),0,p,s,4,cs|POLYMOSTTRANS2,0,0,xdim-1,ydim-1);
        rotatesprite(sbarx(c+j),sbary(y),sbarsc(65536L),0,p,s,althud_numberpal,cs,0,0,xdim-1,ydim-1);
        j += tilesizx[p]+1;
    }
}

static void G_DrawInventory(DukePlayer_t *p)
{
    int32_t n, j = 0, xoff = 0, y;

    n = (p->jetpack_amount > 0)<<3;
    if (n&8) j++;
    n |= (p->scuba_amount > 0)<<5;
    if (n&32) j++;
    n |= (p->steroids_amount > 0)<<1;
    if (n&2) j++;
    n |= (p->holoduke_amount > 0)<<2;
    if (n&4) j++;
    n |= (p->firstaid_amount > 0);
    if (n&1) j++;
    n |= (p->heat_amount > 0)<<4;
    if (n&16) j++;
    n |= (p->boot_amount > 0)<<6;
    if (n&64) j++;

    xoff = 160-(j*11);

    j = 0;

//    if (ud.screen_size > 4)
    y = 154;
//    else y = (ud.drawweapon == 2?150:172);

    /*    if (ud.screen_size == 4 && ud.drawweapon != 2)
        {
            xoff += 65;
            if (ud.multimode > 1)
                xoff -= 9;
        }
    */
    while (j <= 9)
    {
        if (n&(1<<j))
        {
            switch (n&(1<<j))
            {
            case   1:
                rotatesprite(xoff<<16,y<<16,65536L,0,FIRSTAID_ICON,0,0,2+16,windowx1,windowy1,windowx2,windowy2);
                break;
            case   2:
                rotatesprite((xoff+1)<<16,y<<16,65536L,0,STEROIDS_ICON,0,0,2+16,windowx1,windowy1,windowx2,windowy2);
                break;
            case   4:
                rotatesprite((xoff+2)<<16,y<<16,65536L,0,HOLODUKE_ICON,0,0,2+16,windowx1,windowy1,windowx2,windowy2);
                break;
            case   8:
                rotatesprite(xoff<<16,y<<16,65536L,0,JETPACK_ICON,0,0,2+16,windowx1,windowy1,windowx2,windowy2);
                break;
            case  16:
                rotatesprite(xoff<<16,y<<16,65536L,0,HEAT_ICON,0,0,2+16,windowx1,windowy1,windowx2,windowy2);
                break;
            case  32:
                rotatesprite(xoff<<16,y<<16,65536L,0,AIRTANK_ICON,0,0,2+16,windowx1,windowy1,windowx2,windowy2);
                break;
            case 64:
                rotatesprite(xoff<<16,(y-1)<<16,65536L,0,BOOT_ICON,0,0,2+16,windowx1,windowy1,windowx2,windowy2);
                break;
            }

            xoff += 22;

            if (p->inven_icon == j+1)
                rotatesprite((xoff-2)<<16,(y+19)<<16,65536L,1024,ARROW,-32,0,2+16,windowx1,windowy1,windowx2,windowy2);
        }

        j++;
    }
}

void G_DrawFrags(void)
{
    int32_t i, j = 0;

    TRAVERSE_CONNECT(i)
    if (i > j) j = i;

    rotatesprite(0,0,65600L,0,FRAGBAR,0,0,2+8+16+64,0,0,xdim-1,ydim-1);
    if (j >= 4) rotatesprite(319,(8)<<16,65600L,0,FRAGBAR,0,0,10+16+64,0,0,xdim-1,ydim-1);
    if (j >= 8) rotatesprite(319,(16)<<16,65600L,0,FRAGBAR,0,0,10+16+64,0,0,xdim-1,ydim-1);
    if (j >= 12) rotatesprite(319,(24)<<16,65600L,0,FRAGBAR,0,0,10+16+64,0,0,xdim-1,ydim-1);

    TRAVERSE_CONNECT(i)
    {
        minitext(21+(73*(i&3)),2+((i&28)<<1),&g_player[i].user_name[0],/*sprite[g_player[i].ps->i].pal*/g_player[i].ps->palookup,2+8+16);
        Bsprintf(tempbuf,"%d",g_player[i].ps->frag-g_player[i].ps->fraggedself);
        minitext(17+50+(73*(i&3)),2+((i&28)<<1),tempbuf,/*sprite[g_player[i].ps->i].pal*/g_player[i].ps->palookup,2+8+16);
    }
}

#define SBY (200-tilesizy[BOTTOMSTATUSBAR])

static void G_DrawStatusBar(int32_t snum)
{
    DukePlayer_t *p = g_player[snum].ps;
    int32_t i, j, o, ss = ud.screen_size, u;
    int32_t permbit = 0;

    if (ss < 4) return;

    if (g_player[snum].ps->gm&MODE_MENU)
        if ((g_currentMenu >= 400  && g_currentMenu <= 405))
            return;

    if (getrendermode() >= 3) pus = NUMPAGES;   // JBF 20040101: always redraw in GL

    if (ud.multimode > 1 && (GametypeFlags[ud.coop] & GAMETYPE_FRAGBAR))
    {
        if (pus)
        {
            G_DrawFrags();
        }
        else
        {
            TRAVERSE_CONNECT(i)
            if (g_player[i].ps->frag != sbar.frag[i])
            {
                G_DrawFrags();
                break;
            }

        }
        TRAVERSE_CONNECT(i)
        if (i != myconnectindex)
            sbar.frag[i] = g_player[i].ps->frag;
    }

    if (ss == 4)   //DRAW MINI STATUS BAR:
    {
        if (ud.althud) // althud
        {
            static int32_t ammo_sprites[MAX_WEAPONS];

            if (!ammo_sprites[0])
            {
                /* this looks stupid but it lets us initialize static memory to dynamic values
                   these values can be changed from the CONs with dynamic tile remapping
                   but we don't want to have to recreate the values in memory every time
                   the HUD is drawn */

                int32_t asprites[MAX_WEAPONS] = { BOOTS, AMMO, SHOTGUNAMMO,
                                                  BATTERYAMMO, RPGAMMO, HBOMBAMMO, CRYSTALAMMO, DEVISTATORAMMO,
                                                  TRIPBOMBSPRITE, FREEZEAMMO+1, HBOMBAMMO, GROWAMMO
                                                };
                Bmemcpy(ammo_sprites,asprites,sizeof(ammo_sprites));
            }

//            rotatesprite(sbarx(5+1),sbary(200-25+1),sbarsc(49152L),0,SIXPAK,0,4,10+16+1+32,0,0,xdim-1,ydim-1);
//            rotatesprite(sbarx(5),sbary(200-25),sbarsc(49152L),0,SIXPAK,0,0,10+16,0,0,xdim-1,ydim-1);
            if (getrendermode() >= 3 && althud_shadows)
                rotatesprite(sbarx(2+1),sbary(200-21+1),sbarsc(49152L),0,COLA,0,4,10+16+POLYMOSTTRANS2,0,0,xdim-1,ydim-1);
            rotatesprite(sbarx(2),sbary(200-21),sbarsc(49152L),0,COLA,0,0,10+16,0,0,xdim-1,ydim-1);

            if (sprite[p->i].pal == 1 && p->last_extra < 2)
                G_DrawAltDigiNum(40,-(200-22),1,-16,10+16);
            else if (!althud_flashing || p->last_extra > (p->max_player_health>>2) || totalclock&32)
            {
                int32_t s = -8;
                if (althud_flashing && p->last_extra > p->max_player_health)
                    s += (sintable[(totalclock<<5)&2047]>>10);
                G_DrawAltDigiNum(40,-(200-22),p->last_extra,s,10+16);
            }

            if (getrendermode() >= 3 && althud_shadows)
                rotatesprite(sbarx(62+1),sbary(200-25+1),sbarsc(49152L),0,SHIELD,0,4,10+16+POLYMOSTTRANS2,0,0,xdim-1,ydim-1);
            rotatesprite(sbarx(62),sbary(200-25),sbarsc(49152L),0,SHIELD,0,0,10+16,0,0,xdim-1,ydim-1);

            {
                int32_t lAmount=Gv_GetVarByLabel("PLR_MORALE",-1, p->i, snum);
                if (lAmount == -1) lAmount = p->shield_amount;
                G_DrawAltDigiNum(105,-(200-22),lAmount,-16,10+16);
            }

            if (getrendermode() >= 3 && althud_shadows)
            {
                if (p->got_access&1) rotatesprite(sbarxr(39-1),sbary(200-43+1),sbarsc(32768),0,ACCESSCARD,0,4,10+16+POLYMOSTTRANS2,0,0,xdim-1,ydim-1);
                if (p->got_access&4) rotatesprite(sbarxr(34-1),sbary(200-41+1),sbarsc(32768),0,ACCESSCARD,0,4,10+16+POLYMOSTTRANS2,0,0,xdim-1,ydim-1);
                if (p->got_access&2) rotatesprite(sbarxr(29-1),sbary(200-39+1),sbarsc(32768),0,ACCESSCARD,0,4,10+16+POLYMOSTTRANS2,0,0,xdim-1,ydim-1);
            }

            if (p->got_access&1) rotatesprite(sbarxr(39),sbary(200-43),sbarsc(32768),0,ACCESSCARD,0,0,10+16,0,0,xdim-1,ydim-1);
            if (p->got_access&4) rotatesprite(sbarxr(34),sbary(200-41),sbarsc(32768),0,ACCESSCARD,0,23,10+16,0,0,xdim-1,ydim-1);
            if (p->got_access&2) rotatesprite(sbarxr(29),sbary(200-39),sbarsc(32768),0,ACCESSCARD,0,21,10+16,0,0,xdim-1,ydim-1);

            i = 32768;
            if (p->curr_weapon == PISTOL_WEAPON) i = 16384;
            if (getrendermode() >= 3 && althud_shadows)
                rotatesprite(sbarxr(57-1),sbary(200-15+1),sbarsc(i),0,ammo_sprites[p->curr_weapon],0,4,2+POLYMOSTTRANS2,0,0,xdim-1,ydim-1);
            rotatesprite(sbarxr(57),sbary(200-15),sbarsc(i),0,ammo_sprites[p->curr_weapon],0,0,2,0,0,xdim-1,ydim-1);

            if (p->curr_weapon == HANDREMOTE_WEAPON) i = HANDBOMB_WEAPON;
            else i = p->curr_weapon;
            if (p->curr_weapon != KNEE_WEAPON &&
                    (!althud_flashing || totalclock&32 || p->ammo_amount[i] > (p->max_ammo_amount[i]/10)))
                G_DrawAltDigiNum(-20,-(200-22),p->ammo_amount[i],-16,10+16);

            o = 102;
            permbit = 0;

            if (p->inven_icon)
            {
                switch (p->inven_icon)
                {
                case 1:
                    i = FIRSTAID_ICON;
                    break;
                case 2:
                    i = STEROIDS_ICON;
                    break;
                case 3:
                    i = HOLODUKE_ICON;
                    break;
                case 4:
                    i = JETPACK_ICON;
                    break;
                case 5:
                    i = HEAT_ICON;
                    break;
                case 6:
                    i = AIRTANK_ICON;
                    break;
                case 7:
                    i = BOOT_ICON;
                    break;
                default:
                    i = -1;
                }
                if (i >= 0)
                {
                    if (getrendermode() >= 3 && althud_shadows)
                        rotatesprite(sbarx(231-o+1),sbary(200-21-2+1),sbarsc(65536L),0,i,0,4,10+16+permbit+POLYMOSTTRANS2,0,0,xdim-1,ydim-1);
                    rotatesprite(sbarx(231-o),sbary(200-21-2),sbarsc(65536L),0,i,0,0,10+16+permbit,0,0,xdim-1,ydim-1);
                }

                if (getrendermode() >= 3 && althud_shadows)
                    minitext(292-30-o+1,190-3+1,"%",4,POLYMOSTTRANS+10+16+permbit + 256);
                minitext(292-30-o,190-3,"%",6,10+16+permbit + 256);

                j = 0x80000000;
                switch (p->inven_icon)
                {
                case 1:
                    i = p->firstaid_amount;
                    break;
                case 2:
                    i = ((p->steroids_amount+3)>>2);
                    break;
                case 3:
                    i = ((p->holoduke_amount+15)/24);
                    j = p->holoduke_on;
                    break;
                case 4:
                    i = ((p->jetpack_amount+15)>>4);
                    j = p->jetpack_on;
                    break;
                case 5:
                    i = p->heat_amount/12;
                    j = p->heat_on;
                    break;
                case 6:
                    i = ((p->scuba_amount+63)>>6);
                    break;
                case 7:
                    i = (p->boot_amount>>1);
                    break;
                }
                G_DrawInvNum(-(284-30-o),200-6-3,(uint8_t)i,0,10+permbit);
                if (j > 0)
                {
                    if (getrendermode() >= 3 && althud_shadows)
                        minitext(288-30-o+1,180-3+1,"ON",4,POLYMOSTTRANS+10+16+permbit + 256);
                    minitext(288-30-o,180-3,"ON",0,10+16+permbit + 256);
                }
                else if ((uint32_t)j != 0x80000000)
                {
                    if (getrendermode() >= 3 && althud_shadows)
                        minitext(284-30-o+1,180-3+1,"OFF",4,POLYMOSTTRANS+10+16+permbit + 256);
                    minitext(284-30-o,180-3,"OFF",2,10+16+permbit + 256);
                }
                if (p->inven_icon >= 6)
                {
                    if (getrendermode() >= 3 && althud_shadows)
                        minitext(284-35-o+1,180-3+1,"AUTO",4,POLYMOSTTRANS+10+16+permbit + 256);
                    minitext(284-35-o,180-3,"AUTO",2,10+16+permbit + 256);
                }

            }
            return;
        }
        rotatesprite(sbarx(5),sbary(200-28),sbarsc(65536L),0,HEALTHBOX,0,21,10+16,0,0,xdim-1,ydim-1);
        if (p->inven_icon)
            rotatesprite(sbarx(69),sbary(200-30),sbarsc(65536L),0,INVENTORYBOX,0,21,10+16,0,0,xdim-1,ydim-1);

        if (sprite[p->i].pal == 1 && p->last_extra < 2) // frozen
            G_DrawDigiNum(20,200-17,1,-16,10+16);
        else G_DrawDigiNum(20,200-17,p->last_extra,-16,10+16);

        rotatesprite(sbarx(37),sbary(200-28),sbarsc(65536L),0,AMMOBOX,0,21,10+16,0,0,xdim-1,ydim-1);

        if (p->curr_weapon == HANDREMOTE_WEAPON) i = HANDBOMB_WEAPON;
        else i = p->curr_weapon;
        G_DrawDigiNum(53,200-17,p->ammo_amount[i],-16,10+16);

        o = 158;
        permbit = 0;
        if (p->inven_icon)
        {
            switch (p->inven_icon)
            {
            case 1:
                i = FIRSTAID_ICON;
                break;
            case 2:
                i = STEROIDS_ICON;
                break;
            case 3:
                i = HOLODUKE_ICON;
                break;
            case 4:
                i = JETPACK_ICON;
                break;
            case 5:
                i = HEAT_ICON;
                break;
            case 6:
                i = AIRTANK_ICON;
                break;
            case 7:
                i = BOOT_ICON;
                break;
            default:
                i = -1;
            }
            if (i >= 0) rotatesprite(sbarx(231-o),sbary(200-21),sbarsc(65536L),0,i,0,0,10+16+permbit,0,0,xdim-1,ydim-1);

            minitext(292-30-o,190,"%",6,10+16+permbit + 256);

            j = 0x80000000;
            switch (p->inven_icon)
            {
            case 1:
                i = p->firstaid_amount;
                break;
            case 2:
                i = ((p->steroids_amount+3)>>2);
                break;
            case 3:
                i = ((p->holoduke_amount+15)/24);
                j = p->holoduke_on;
                break;
            case 4:
                i = ((p->jetpack_amount+15)>>4);
                j = p->jetpack_on;
                break;
            case 5:
                i = p->heat_amount/12;
                j = p->heat_on;
                break;
            case 6:
                i = ((p->scuba_amount+63)>>6);
                break;
            case 7:
                i = (p->boot_amount>>1);
                break;
            }
            G_DrawInvNum(284-30-o,200-6,(uint8_t)i,0,10+permbit);
            if (j > 0) minitext(288-30-o,180,"ON",0,10+16+permbit + 256);
            else if ((uint32_t)j != 0x80000000) minitext(284-30-o,180,"OFF",2,10+16+permbit + 256);
            if (p->inven_icon >= 6) minitext(284-35-o,180,"AUTO",2,10+16+permbit + 256);
        }
        return;
    }

    //DRAW/UPDATE FULL STATUS BAR:

    if (pus)
    {
        pus = 0;
        u = -1;
    }
    else u = 0;

    if (sbar.frag[myconnectindex] != p->frag)
    {
        sbar.frag[myconnectindex] = p->frag;
        u |= 32768;
    }
    if (sbar.got_access != p->got_access)
    {
        sbar.got_access = p->got_access;
        u |= 16384;
    }

    if (sbar.last_extra != p->last_extra)
    {
        sbar.last_extra = p->last_extra;
        u |= 1;
    }

    {
        int32_t lAmount=Gv_GetVarByLabel("PLR_MORALE",-1, p->i, snum);
        if (lAmount == -1)
        {
            if (sbar.shield_amount != p->shield_amount)
            {
                sbar.shield_amount = p->shield_amount;
                u |= 2;
            }

        }
        else
        {
            if (sbar.shield_amount != lAmount)
            {
                sbar.shield_amount = lAmount;
                u |= 2;
            }

        }
    }

    if (sbar.curr_weapon != p->curr_weapon)
    {
        sbar.curr_weapon = p->curr_weapon;
        u |= (4+8+16+32+64+128+256+512+1024+65536L);
    }

    for (i=1; i<MAX_WEAPONS; i++)
    {
        if (sbar.ammo_amount[i] != p->ammo_amount[i])
        {
            sbar.ammo_amount[i] = p->ammo_amount[i];
            if (i < 9)
                u |= ((2<<i)+1024);
            else u |= 65536L+1024;
        }

        if (sbar.gotweapon[i] != p->gotweapon[i])
        {
            sbar.gotweapon[i] = p->gotweapon[i];
            if (i < 9)
                u |= ((2<<i)+1024);
            else u |= 65536L+1024;
        }
    }
    if (sbar.inven_icon != p->inven_icon)
    {
        sbar.inven_icon = p->inven_icon;
        u |= (2048+4096+8192);
    }
    if (sbar.holoduke_on != p->holoduke_on)
    {
        sbar.holoduke_on = p->holoduke_on;
        u |= (4096+8192);
    }
    if (sbar.jetpack_on != p->jetpack_on)
    {
        sbar.jetpack_on = p->jetpack_on;
        u |= (4096+8192);
    }
    if (sbar.heat_on != p->heat_on)
    {
        sbar.heat_on = p->heat_on;
        u |= (4096+8192);
    }
    if (sbar.firstaid_amount != p->firstaid_amount)
    {
        sbar.firstaid_amount = p->firstaid_amount;
        u |= 8192;
    }
    if (sbar.steroids_amount != p->steroids_amount)
    {
        sbar.steroids_amount = p->steroids_amount;
        u |= 8192;
    }
    if (sbar.holoduke_amount != p->holoduke_amount)
    {
        sbar.holoduke_amount = p->holoduke_amount;
        u |= 8192;
    }
    if (sbar.jetpack_amount != p->jetpack_amount)
    {
        sbar.jetpack_amount = p->jetpack_amount;
        u |= 8192;
    }
    if (sbar.heat_amount != p->heat_amount)
    {
        sbar.heat_amount = p->heat_amount;
        u |= 8192;
    }
    if (sbar.scuba_amount != p->scuba_amount)
    {
        sbar.scuba_amount = p->scuba_amount;
        u |= 8192;
    }
    if (sbar.boot_amount != p->boot_amount)
    {
        sbar.boot_amount = p->boot_amount;
        u |= 8192;
    }
    if (u == 0) return;

    //0 - update health
    //1 - update armor
    //2 - update PISTOL_WEAPON ammo
    //3 - update SHOTGUN_WEAPON ammo
    //4 - update CHAINGUN_WEAPON ammo
    //5 - update RPG_WEAPON ammo
    //6 - update HANDBOMB_WEAPON ammo
    //7 - update SHRINKER_WEAPON ammo
    //8 - update DEVISTATOR_WEAPON ammo
    //9 - update TRIPBOMB_WEAPON ammo
    //10 - update ammo display
    //11 - update inventory icon
    //12 - update inventory on/off
    //13 - update inventory %
    //14 - update keys
    //15 - update kills
    //16 - update FREEZE_WEAPON ammo

    if (u == -1)
    {
        G_PatchStatusBar(0,0,320,200);
        if (ud.multimode > 1 && (GametypeFlags[ud.coop] & GAMETYPE_FRAGBAR))
            rotatesprite(sbarx(277+1),sbary(SBY+7-1),sbarsc(65536L),0,KILLSICON,0,0,10+16,0,0,xdim-1,ydim-1);
    }
    if (ud.multimode > 1 && (GametypeFlags[ud.coop] & GAMETYPE_FRAGBAR))
    {
        if (u&32768)
        {
            if (u != -1) G_PatchStatusBar(276,SBY+17,299,SBY+17+10);
            G_DrawDigiNum(287,SBY+17,max(p->frag-p->fraggedself,0),-16,10+16);
        }
    }
    else
    {
        if (u&16384)
        {
            if (u != -1) G_PatchStatusBar(275,SBY+18,299,SBY+18+12);
            if (p->got_access&4) rotatesprite(sbarx(275),sbary(SBY+16),sbarsc(65536L),0,ACCESS_ICON,0,23,10+16,0,0,xdim-1,ydim-1);
            if (p->got_access&2) rotatesprite(sbarx(288),sbary(SBY+16),sbarsc(65536L),0,ACCESS_ICON,0,21,10+16,0,0,xdim-1,ydim-1);
            if (p->got_access&1) rotatesprite(sbarx(281),sbary(SBY+23),sbarsc(65536L),0,ACCESS_ICON,0,0,10+16,0,0,xdim-1,ydim-1);
        }
    }
    if (u&(4+8+16+32+64+128+256+512+65536L)) G_DrawWeapAmounts(p,96,SBY+16,u);

    if (u&1)
    {
        if (u != -1) G_PatchStatusBar(20,SBY+17,43,SBY+17+11);
        if (sprite[p->i].pal == 1 && p->last_extra < 2)
            G_DrawDigiNum(32,SBY+17,1,-16,10+16);
        else G_DrawDigiNum(32,SBY+17,p->last_extra,-16,10+16);
    }
    if (u&2)
    {
        int32_t lAmount=Gv_GetVarByLabel("PLR_MORALE",-1, p->i, snum);
        if (u != -1) G_PatchStatusBar(52,SBY+17,75,SBY+17+11);
        if (lAmount == -1)
            G_DrawDigiNum(64,SBY+17,p->shield_amount,-16,10+16);
        else
            G_DrawDigiNum(64,SBY+17,lAmount,-16,10+16);
    }

    if (u&1024)
    {
        if (u != -1) G_PatchStatusBar(196,SBY+17,219,SBY+17+11);
        if (p->curr_weapon != KNEE_WEAPON)
        {
            if (p->curr_weapon == HANDREMOTE_WEAPON) i = HANDBOMB_WEAPON;
            else i = p->curr_weapon;
            G_DrawDigiNum(230-22,SBY+17,p->ammo_amount[i],-16,10+16);
        }
    }

    if (u&(2048+4096+8192))
    {
        if (u != -1)
        {
            if (u&(2048+4096))
            {
                G_PatchStatusBar(231,SBY+13,265,SBY+13+18);
            }
            else
            {
                G_PatchStatusBar(250,SBY+24,261,SBY+24+6);
            }

        }
        if (p->inven_icon)
        {
            o = 0;
//            permbit = 128;

            if (u&(2048+4096))
            {
                switch (p->inven_icon)
                {
                case 1:
                    i = FIRSTAID_ICON;
                    break;
                case 2:
                    i = STEROIDS_ICON;
                    break;
                case 3:
                    i = HOLODUKE_ICON;
                    break;
                case 4:
                    i = JETPACK_ICON;
                    break;
                case 5:
                    i = HEAT_ICON;
                    break;
                case 6:
                    i = AIRTANK_ICON;
                    break;
                case 7:
                    i = BOOT_ICON;
                    break;
                }
                rotatesprite(sbarx(231-o),sbary(SBY+13),sbarsc(65536L),0,i,0,0,10+16+permbit,0,0,xdim-1,ydim-1);
                minitext(292-30-o,SBY+24,"%",6,10+16+permbit + 256);
                if (p->inven_icon >= 6) minitext(284-35-o,SBY+14,"AUTO",2,10+16+permbit + 256);
            }
            if (u&(2048+4096))
            {
                switch (p->inven_icon)
                {
                case 3:
                    j = p->holoduke_on;
                    break;
                case 4:
                    j = p->jetpack_on;
                    break;
                case 5:
                    j = p->heat_on;
                    break;
                default:
                    j = 0x80000000;
                }
                if (j > 0) minitext(288-30-o,SBY+14,"ON",0,10+16+permbit + 256);
                else if ((uint32_t)j != 0x80000000) minitext(284-30-o,SBY+14,"OFF",2,10+16+permbit + 256);
            }
            if (u&8192)
            {
                switch (p->inven_icon)
                {
                case 1:
                    i = p->firstaid_amount;
                    break;
                case 2:
                    i = ((p->steroids_amount+3)>>2);
                    break;
                case 3:
                    i = ((p->holoduke_amount+15)/24);
                    break;
                case 4:
                    i = ((p->jetpack_amount+15)>>4);
                    break;
                case 5:
                    i = p->heat_amount/12;
                    break;
                case 6:
                    i = ((p->scuba_amount+63)>>6);
                    break;
                case 7:
                    i = (p->boot_amount>>1);
                    break;
                }
                G_DrawInvNum(284-30-o,SBY+28,(uint8_t)i,0,10+permbit);
            }
        }
    }
}

#define COLOR_RED 248
#define COLOR_WHITE 31
#define LOW_FPS 30

static void G_PrintFPS(void)
{
    // adapted from ZDoom because I like it better than what we had
    // applicable ZDoom code available under GPL from csDoom
    static int32_t FrameCount = 0;
    static int32_t LastCount = 0;
    static int32_t LastSec = 0;
    static int32_t LastMS = 0;
    int32_t ms = getticks();
    int32_t howlong = ms - LastMS;
    if (howlong >= 0)
    {
        int32_t thisSec = ms/1000;
        int32_t x = (xdim <= 640);
        if (ud.tickrate)
        {
            int32_t chars = Bsprintf(tempbuf, "%2u ms (%3u fps)", howlong, LastCount);

            printext256(windowx2-(chars<<(3-x))+1,windowy1+2,0,-1,tempbuf,x);
            printext256(windowx2-(chars<<(3-x)),windowy1+1,
                        (LastCount < LOW_FPS) ? COLOR_RED : COLOR_WHITE,-1,tempbuf,x);

            // lag meter
            if (numplayers > 1 && (totalclock - lastpackettime) > 1)
            {
                for (howlong = (totalclock - lastpackettime); howlong>0 && howlong<(xdim>>2); howlong--)
                    printext256(4L*howlong,0,COLOR_WHITE,-1,".",0);
            }
        }

        if (LastSec < thisSec)
        {
            g_currentFrameRate = LastCount = FrameCount / (thisSec - LastSec);
            LastSec = thisSec;
            FrameCount = 0;
        }
        FrameCount++;
    }
    LastMS = ms;
}

static void G_PrintCoords(int32_t snum)
{
    int32_t y = 16;

    if ((GametypeFlags[ud.coop] & GAMETYPE_FRAGBAR))
    {
        if (ud.multimode > 4)
            y = 32;
        else if (ud.multimode > 1)
            y = 24;
    }
    Bsprintf(tempbuf,"XYZ= (%d,%d,%d)",g_player[snum].ps->posx,g_player[snum].ps->posy,g_player[snum].ps->posz);
    printext256(250L,y,31,-1,tempbuf,0);
    Bsprintf(tempbuf,"A/H= %d,%d",g_player[snum].ps->ang,g_player[snum].ps->horiz);
    printext256(250L,y+9L,31,-1,tempbuf,0);
    Bsprintf(tempbuf,"ZV= %d",g_player[snum].ps->poszv);
    printext256(250L,y+18L,31,-1,tempbuf,0);
    Bsprintf(tempbuf,"OG= %d",g_player[snum].ps->on_ground);
    printext256(250L,y+27L,31,-1,tempbuf,0);
    Bsprintf(tempbuf,"SECTL= %d",sector[g_player[snum].ps->cursectnum].lotag);
    printext256(250L,y+36L,31,-1,tempbuf,0);
    Bsprintf(tempbuf,"SEED= %d",randomseed);
    printext256(250L,y+45L,31,-1,tempbuf,0);
    Bsprintf(tempbuf,"THOLD= %d",g_player[snum].ps->transporter_hold);
    printext256(250L,y+54L+7,31,-1,tempbuf,0);
}

// this handles both multiplayer and item pickup message type text
// both are passed on to gametext

static void G_PrintGameQuotes(void)
{
    int32_t i, j, k, l;

    k = 1;
    if (GTFLAGS(GAMETYPE_FRAGBAR) && ud.screen_size > 0 && ud.multimode > 1)
    {
        j = 0;
        k += 8;
        TRAVERSE_CONNECT(i)
        if (i > j) j = i;

        if (j >= 4 && j <= 8) k += 8;
        else if (j > 8 && j <= 12) k += 16;
        else if (j > 12) k += 24;
    }

    if (g_player[screenpeek].ps->fta > 1 && (g_player[screenpeek].ps->ftq < 115 || g_player[screenpeek].ps->ftq > 117))
    {
        if (g_player[screenpeek].ps->fta > 6)
            k += 7;
        else k += g_player[screenpeek].ps->fta; /*if (g_player[screenpeek].ps->fta > 2)
            k += 3;
        else k += 1; */
    }

    /*    if (xdim >= 640 && ydim >= 480)
            k = scale(k,ydim,200); */

    j = k;

//    quotebot = min(quotebot,j);
    //  quotebotgoal = min(quotebotgoal,j);
//    if (g_player[myconnectindex].ps->gm&MODE_TYPE) j -= 8;
    //quotebotgoal = j;
    //j = quotebot;
    j = scale(j,ydim,200);
    for (i=MAXUSERQUOTES-1; i>=0; i--)
    {
        if (user_quote_time[i] <= 0) continue;
        k = user_quote_time[i];
        if (hud_glowingquotes)
        {
            if (k > 4) { mpgametext(j,user_quote[i],(sintable[((totalclock+(i<<2))<<5)&2047]>>11),2+8+16); j += textsc(8); }
            else if (k > 2) { mpgametext(j,user_quote[i],(sintable[((totalclock+(i<<2))<<5)&2047]>>11),2+8+16+1); j += textsc(k<<1); }
            else { mpgametext(j,user_quote[i],(sintable[((totalclock+(i<<2))<<5)&2047]>>11),2+8+16+1+32); j += textsc(k<<1); }
        }
        else
        {
            if (k > 4) { mpgametext(j,user_quote[i],0,2+8+16); j += textsc(8); }
            else if (k > 2) { mpgametext(j,user_quote[i],0,2+8+16+1); j += textsc(k<<1); }
            else { mpgametext(j,user_quote[i],0,2+8+16+1+32); j += textsc(k<<1); }
        }
        l = G_GameTextLen(USERQUOTE_LEFTOFFSET,stripcolorcodes(tempbuf,user_quote[i]));
        while (l > (ud.config.ScreenWidth - USERQUOTE_RIGHTOFFSET))
        {
            l -= (ud.config.ScreenWidth-USERQUOTE_RIGHTOFFSET);
            if (k > 4) j += textsc(8);
            else j += textsc(k<<1);

        }
    }

    if ((klabs(quotebotgoal-quotebot) <= 16) && (ud.screen_size <= 8))
        quotebot += ksgn(quotebotgoal-quotebot);
    else
        quotebot = quotebotgoal;

    if (g_player[screenpeek].ps->fta <= 1) return;

    if (ScriptQuotes[g_player[screenpeek].ps->ftq] == NULL)
    {
        OSD_Printf(OSD_ERROR "%s %d null quote %d\n",__FILE__,__LINE__,g_player[screenpeek].ps->ftq);
        return;
    }

    k = 0;

    if (g_player[screenpeek].ps->ftq == 115 || g_player[screenpeek].ps->ftq == 116 || g_player[screenpeek].ps->ftq == 117)
    {
        k = 140;//quotebot-8-4;
    }
    else if (GTFLAGS(GAMETYPE_FRAGBAR) && ud.screen_size > 0 && ud.multimode > 1)
    {
        j = 0;
        k = 8;
        TRAVERSE_CONNECT(i)
        if (i > j) j = i;

        if (j >= 4 && j <= 8) k += 8;
        else if (j > 8 && j <= 12) k += 16;
        else if (j > 12) k += 24;
    }

    j = g_player[screenpeek].ps->fta;
    if (!hud_glowingquotes)
    {
        if (j > 4) gametext(320>>1,k,ScriptQuotes[g_player[screenpeek].ps->ftq],0,2+8+16);
        else if (j > 2) gametext(320>>1,k,ScriptQuotes[g_player[screenpeek].ps->ftq],0,2+8+16+1);
        else gametext(320>>1,k,ScriptQuotes[g_player[screenpeek].ps->ftq],0,2+8+16+1+32);
        return;
    }
    if (j > 4) gametext(320>>1,k,ScriptQuotes[g_player[screenpeek].ps->ftq],(sintable[(totalclock<<5)&2047]>>11),2+8+16);
    else if (j > 2) gametext(320>>1,k,ScriptQuotes[g_player[screenpeek].ps->ftq],(sintable[(totalclock<<5)&2047]>>11),2+8+16+1);
    else gametext(320>>1,k,ScriptQuotes[g_player[screenpeek].ps->ftq],(sintable[(totalclock<<5)&2047]>>11),2+8+16+1+32);
}

void P_DoQuote(int32_t q, DukePlayer_t *p)
{
    int32_t cq = 0;

    if (q & MAXQUOTES)
    {
        cq = 1;
        q &= ~MAXQUOTES;
    }

    if (ScriptQuotes[q] == NULL)
    {
        OSD_Printf(OSD_ERROR "%s %d null quote %d\n",__FILE__,__LINE__,q);
        return;
    }

    if (ud.fta_on == 0)
        return;

    if (p->fta > 0 && q != 115 && q != 116)
        if (p->ftq == 115 || p->ftq == 116) return;

    p->fta = 100;

    //            if(p->ftq != q || q == 26)
    // || q == 26 || q == 115 || q ==116 || q == 117 || q == 122)
    {
        if (p->ftq != q)
            if (p == g_player[screenpeek].ps)
            {
                if (cq) OSD_Printf(OSDTEXT_BLUE "%s\n",ScriptQuotes[q]);
                else OSD_Printf("%s\n",ScriptQuotes[q]);
            }

        p->ftq = q;
        pub = NUMPAGES;
        pus = NUMPAGES;
    }
}

void fadepal(int32_t r, int32_t g, int32_t b, int32_t start, int32_t end, int32_t step)
{
    if (getrendermode() >= 3) return;
    if (step > 0) for (; start < end; start += step) G_FadePalette(r,g,b,start);
    else for (; start >= end; start += step) G_FadePalette(r,g,b,start);
}

static void G_DisplayExtraScreens(void)
{
    int32_t flags = Gv_GetVarByLabel("LOGO_FLAGS",255, -1, -1);

    S_StopMusic();
    FX_StopAllSounds();

    if (!VOLUMEALL || flags & LOGO_SHAREWARESCREENS)
    {
        setview(0,0,xdim-1,ydim-1);
        flushperms();
        //g_player[myconnectindex].ps->palette = palette;
        P_SetGamePalette(g_player[myconnectindex].ps, palette, 1);    // JBF 20040308
        fadepal(0,0,0, 0,64,7);
        KB_FlushKeyboardQueue();
        rotatesprite(0,0,65536L,0,3291,0,0,2+8+16+64, 0,0,xdim-1,ydim-1);
        IFISSOFTMODE fadepal(0,0,0, 63,0,-7);
        else nextpage();
        while (!KB_KeyWaiting())
        {
            handleevents();
            Net_GetPackets();
        }

        fadepal(0,0,0, 0,64,7);
        KB_FlushKeyboardQueue();
        rotatesprite(0,0,65536L,0,3290,0,0,2+8+16+64, 0,0,xdim-1,ydim-1);
        IFISSOFTMODE fadepal(0,0,0, 63,0,-7);
        else nextpage();
        while (!KB_KeyWaiting())
        {
            handleevents();
            Net_GetPackets();
        }
    }

    if (flags & LOGO_TENSCREEN)
    {
        setview(0,0,xdim-1,ydim-1);
        flushperms();
        //g_player[myconnectindex].ps->palette = palette;
        P_SetGamePalette(g_player[myconnectindex].ps, palette, 1);    // JBF 20040308
        fadepal(0,0,0, 0,64,7);
        KB_FlushKeyboardQueue();
        rotatesprite(0,0,65536L,0,TENSCREEN,0,0,2+8+16+64, 0,0,xdim-1,ydim-1);
        IFISSOFTMODE fadepal(0,0,0, 63,0,-7);
        else nextpage();
        while (!KB_KeyWaiting() && totalclock < 2400)
        {
            handleevents();
            Net_GetPackets();
        }
    }
}

extern int32_t qsetmode;
extern int32_t g_doQuickSave;

void G_GameExit(const char *t)
{
    if (*t != 0) g_player[myconnectindex].ps->palette = (uint8_t *) &palette[0];

    if (numplayers > 1)
    {
        int32_t i, j, oldtotalclock;

        ready2send = 0;

        for (j=MAXPLAYERS-1; j>=0; j--)
        {
            oldtotalclock = totalclock;

            while (totalclock < oldtotalclock+TICSPERFRAME)
            {
                handleevents();
                Net_GetPackets();
            }
            if (KB_KeyPressed(sc_Escape)) return;

            packbuf[0] = PACKET_NULL_PACKET;
            TRAVERSE_CONNECT(i)
            {
                if (i != myconnectindex) mmulti_sendpacket(i,packbuf,1);
                if ((!g_networkBroadcastMode) && (myconnectindex != connecthead)) break; //slaves in M/S mode only send to master
            }
        }
    }

    mmulti_uninitmultiplayers();

    if (ud.recstat == 1) G_CloseDemoWrite();
    else if (ud.recstat == 2)
    {
        if (frecfilep) fclose(frecfilep);
    } // JBF: fixes crash on demo playback

    if (!qe)
    {
        if (playerswhenstarted > 1 && g_player[myconnectindex].ps->gm&MODE_GAME && GTFLAGS(GAMETYPE_SCORESHEET) && *t == ' ')
        {
            G_BonusScreen(1);
            setgamemode(ud.config.ScreenMode,ud.config.ScreenWidth,ud.config.ScreenHeight,ud.config.ScreenBPP);
        }

        if (*t != 0 && *(t+1) != 'V' && *(t+1) != 'Y')
            G_DisplayExtraScreens();
    }

    if (*t != 0) initprintf("%s\n",t);

    if (qsetmode == 200)
        G_Shutdown();

    if (*t != 0)
    {
        //setvmode(0x3);    // JBF
        //binscreen();
        //            if(*t == ' ' && *(t+1) == 0) *t = 0;
        //printf("\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n");
        if (!(t[0] == ' ' && t[1] == 0))
        {
            char titlebuf[256];
            Bsprintf(titlebuf,HEAD2 " %s",s_buildDate);
            wm_msgbox(titlebuf, (char *)t);
        }
    }

    uninitgroupfile();

    //unlink("duke3d.tmp");

    exit(0);
}

char inputloc = 0;

int32_t _EnterText(int32_t small,int32_t x,int32_t y,char *t,int32_t dalen,int32_t c)
{
    char ch;
    int32_t i;

    while ((ch = KB_Getch()) != 0 || (g_player[myconnectindex].ps->gm&MODE_MENU && MOUSE_GetButtons()&RIGHT_MOUSE))
    {
        if (ch == asc_BackSpace)
        {
            if (inputloc > 0)
            {
                inputloc--;
                *(t+inputloc) = 0;
            }
        }
        else
        {
            if (ch == asc_Enter)
            {
                KB_ClearKeyDown(sc_Enter);
                KB_ClearKeyDown(sc_kpad_Enter);
                return (1);
            }
            else if (ch == asc_Escape || (g_player[myconnectindex].ps->gm&MODE_MENU && MOUSE_GetButtons()&RIGHT_MOUSE))
            {
                KB_ClearKeyDown(sc_Escape);
                MOUSE_ClearButton(RIGHT_MOUSE);
                return (-1);
            }
            else if (ch >= 32 && inputloc < dalen && ch < 127)
            {
                ch = Btoupper(ch);
                if (c != 997 || (ch >= '0' && ch <= '9'))
                {
                    // JBF 20040508: so we can have numeric only if we want
                    *(t+inputloc) = ch;
                    *(t+inputloc+1) = 0;
                    inputloc++;
                }
            }
        }
    }

    if (c == 999) return(0);
    if (c == 998)
    {
        char b[91],ii;
        for (ii=0; ii<inputloc; ii++)
            b[(uint8_t)ii] = '*';
        b[(uint8_t)inputloc] = 0;
        if (g_player[myconnectindex].ps->gm&MODE_TYPE)
            x = mpgametext(y,b,c,2+8+16);
        else x = gametext(x,y,b,c,2+8+16);
    }
    else
    {
        if (g_player[myconnectindex].ps->gm&MODE_TYPE)
            x = mpgametext(y,t,c,2+8+16);
        else x = gametext(x,y,t,c,2+8+16);
    }
    c = 4-(sintable[(totalclock<<4)&2047]>>11);

    i = G_GameTextLen(USERQUOTE_LEFTOFFSET,stripcolorcodes(tempbuf,t));
    while (i > (ud.config.ScreenWidth - USERQUOTE_RIGHTOFFSET))
    {
        i -= (ud.config.ScreenWidth - USERQUOTE_RIGHTOFFSET);
        if (small&1)
            y += textsc(6);
        y += 8;
    }

    if (small&1)
        rotatesprite(textsc(x)<<16,(y<<16),32768,0,SPINNINGNUKEICON+((totalclock>>3)%7),c,0,(small&1)?(8|16):2+8,0,0,xdim-1,ydim-1);
    else rotatesprite((x+((small&1)?4:8))<<16,((y+((small&1)?0:4))<<16),32768,0,SPINNINGNUKEICON+((totalclock>>3)%7),c,0,(small&1)?(8|16):2+8,0,0,xdim-1,ydim-1);
    return (0);
}

static void Net_EnterMessage(void)
{
    int16_t ch, hitstate, i, j, l;

    if (g_player[myconnectindex].ps->gm&MODE_SENDTOWHOM)
    {
        if (g_chatPlayer != -1 || ud.multimode < 3 || g_movesPerPacket == 4)
        {
            tempbuf[0] = PACKET_MESSAGE;
            tempbuf[2] = 0;
            recbuf[0]  = 0;

            if (ud.multimode < 3)
                g_chatPlayer = 2;

            if (typebuf[0] == '/' && Btoupper(typebuf[1]) == 'M' && Btoupper(typebuf[2]) == 'E')
            {
                Bstrcat(recbuf,"* ");
                i = 3, j = Bstrlen(typebuf);
                Bstrcpy(tempbuf,typebuf);
                while (i < j)
                {
                    typebuf[i-3] = tempbuf[i];
                    i++;
                }
                typebuf[i-3] = '\0';
                Bstrcat(recbuf,g_player[myconnectindex].user_name);
            }
            else
            {
                Bstrcat(recbuf,g_player[myconnectindex].user_name);
                Bstrcat(recbuf,": ");
            }

            Bstrcat(recbuf,"^00");
            Bstrcat(recbuf,typebuf);
            j = Bstrlen(recbuf);
            recbuf[j] = 0;
            Bstrcat(tempbuf+2,recbuf);

            if (g_chatPlayer >= ud.multimode || g_movesPerPacket == 4)
            {
                tempbuf[1] = 255;
                TRAVERSE_CONNECT(ch)
                {
                    if (ch != myconnectindex) mmulti_sendpacket(ch,tempbuf,j+2);
                    if ((!g_networkBroadcastMode) && (myconnectindex != connecthead)) break; //slaves in M/S mode only send to master
                }
                G_AddUserQuote(recbuf);
                quotebot += 8;
                l = G_GameTextLen(USERQUOTE_LEFTOFFSET,stripcolorcodes(tempbuf,recbuf));
                while (l > (ud.config.ScreenWidth - USERQUOTE_RIGHTOFFSET))
                {
                    l -= (ud.config.ScreenWidth - USERQUOTE_RIGHTOFFSET);
                    quotebot += 8;
                }
                quotebotgoal = quotebot;
            }
            else if (g_chatPlayer >= 0)
            {
                tempbuf[1] = (uint8_t)g_chatPlayer;
                if ((!g_networkBroadcastMode) && (myconnectindex != connecthead))
                    g_chatPlayer = connecthead;
                mmulti_sendpacket(g_chatPlayer,tempbuf,j+2);
            }

            g_chatPlayer = -1;
            g_player[myconnectindex].ps->gm &= ~(MODE_TYPE|MODE_SENDTOWHOM);
        }
        else if (g_chatPlayer == -1)
        {
            j = 50;
            gametext(320>>1,j,"SEND MESSAGE TO...",0,2+8+16);
            j += 8;
            TRAVERSE_CONNECT(i)
            {
                if (i == myconnectindex)
                {
                    minitextshade((320>>1)-40+1,j+1,"A/ENTER - ALL",26,0,2+8+16);
                    minitext((320>>1)-40,j,"A/ENTER - ALL",0,2+8+16);
                    j += 7;
                }
                else
                {
                    Bsprintf(buf,"      %d - %s",i+1,g_player[i].user_name);
                    minitextshade((320>>1)-40-6+1,j+1,buf,26,0,2+8+16);
                    minitext((320>>1)-40-6,j,buf,0,2+8+16);
                    j += 7;
                }
            }
            minitextshade((320>>1)-40-4+1,j+1,"    ESC - Abort",26,0,2+8+16);
            minitext((320>>1)-40-4,j,"    ESC - Abort",0,2+8+16);
            j += 7;

            if (ud.screen_size > 0) j = 200-45;
            else j = 200-8;
            mpgametext(j,typebuf,0,2+8+16);

            if (KB_KeyWaiting())
            {
                i = KB_GetCh();

                if (i == 'A' || i == 'a' || i == 13)
                    g_chatPlayer = ud.multimode;
                else if (i >= '1' || i <= (ud.multimode + '1'))
                    g_chatPlayer = i - '1';
                else
                {
                    g_chatPlayer = ud.multimode;
                    if (i == 27)
                    {
                        g_player[myconnectindex].ps->gm &= ~(MODE_TYPE|MODE_SENDTOWHOM);
                        g_chatPlayer = -1;
                    }
                    else
                        typebuf[0] = 0;
                }

                KB_ClearKeyDown(sc_1);
                KB_ClearKeyDown(sc_2);
                KB_ClearKeyDown(sc_3);
                KB_ClearKeyDown(sc_4);
                KB_ClearKeyDown(sc_5);
                KB_ClearKeyDown(sc_6);
                KB_ClearKeyDown(sc_7);
                KB_ClearKeyDown(sc_8);
                KB_ClearKeyDown(sc_A);
                KB_ClearKeyDown(sc_Escape);
                KB_ClearKeyDown(sc_Enter);
            }
        }
    }
    else
    {
        if (ud.screen_size > 1) j = 200-45;
        else j = 200-8;
        if (xdim >= 640 && ydim >= 480)
            j = scale(j,ydim,200);
        hitstate = Net_EnterText(320>>1,j,typebuf,120,1);

        if (hitstate == 1)
        {
            KB_ClearKeyDown(sc_Enter);
            if (Bstrlen(typebuf) == 0)
            {
                g_player[myconnectindex].ps->gm &= ~(MODE_TYPE|MODE_SENDTOWHOM);
                return;
            }
            if (ud.automsg)
            {
                if (SHIFTS_IS_PRESSED) g_chatPlayer = -1;
                else g_chatPlayer = ud.multimode;
            }
            g_player[myconnectindex].ps->gm |= MODE_SENDTOWHOM;
        }
        else if (hitstate == -1)
            g_player[myconnectindex].ps->gm &= ~(MODE_TYPE|MODE_SENDTOWHOM);
        else pub = NUMPAGES;
    }
}

static inline void G_MoveClouds(void)
{
    if (totalclock > cloudtotalclock || totalclock < (cloudtotalclock-7))
    {
        int32_t i;

        cloudtotalclock = totalclock+6;

        for (i=g_numClouds-1; i>=0; i--)
        {
            cloudx[i] += (sintable[(g_player[screenpeek].ps->ang+512)&2047]>>9);
            cloudy[i] += (sintable[g_player[screenpeek].ps->ang&2047]>>9);

            sector[clouds[i]].ceilingxpanning = cloudx[i]>>6;
            sector[clouds[i]].ceilingypanning = cloudy[i]>>6;
        }
    }
}

static void G_DrawOverheadMap(int32_t cposx, int32_t cposy, int32_t czoom, int16_t cang)
{
    int32_t i, j, k, l, x1, y1, x2=0, y2=0, x3, y3, x4, y4, ox, oy, xoff, yoff;
    int32_t dax, day, cosang, sinang, xspan, yspan, sprx, spry;
    int32_t xrepeat, yrepeat, z1, z2, startwall, endwall, tilenum, daang;
    int32_t xvect, yvect, xvect2, yvect2;
    int16_t p;
    char col;
    walltype *wal, *wal2;
    spritetype *spr;

    xvect = sintable[(-cang)&2047] * czoom;
    yvect = sintable[(1536-cang)&2047] * czoom;
    xvect2 = mulscale16(xvect,yxaspect);
    yvect2 = mulscale16(yvect,yxaspect);

    //Draw red lines
    for (i=numsectors-1; i>=0; i--)
    {
        if (!(show2dsector[i>>3]&(1<<(i&7)))) continue;

        startwall = sector[i].wallptr;
        endwall = sector[i].wallptr + sector[i].wallnum;

        z1 = sector[i].ceilingz;
        z2 = sector[i].floorz;

        for (j=startwall,wal=&wall[startwall]; j<endwall; j++,wal++)
        {
            k = wal->nextwall;
            if (k < 0) continue;

            //if ((show2dwall[j>>3]&(1<<(j&7))) == 0) continue;
            //if ((k > j) && ((show2dwall[k>>3]&(1<<(k&7))) > 0)) continue;

            if (sector[wal->nextsector].ceilingz == z1)
                if (sector[wal->nextsector].floorz == z2)
                    if (((wal->cstat|wall[wal->nextwall].cstat)&(16+32)) == 0) continue;

            col = 139; //red
            if ((wal->cstat|wall[wal->nextwall].cstat)&1) col = 234; //magenta

            if (!(show2dsector[wal->nextsector>>3]&(1<<(wal->nextsector&7))))
                col = 24;
            else continue;

            ox = wal->x-cposx;
            oy = wal->y-cposy;
            x1 = dmulscale16(ox,xvect,-oy,yvect)+(xdim<<11);
            y1 = dmulscale16(oy,xvect2,ox,yvect2)+(ydim<<11);

            wal2 = &wall[wal->point2];
            ox = wal2->x-cposx;
            oy = wal2->y-cposy;
            x2 = dmulscale16(ox,xvect,-oy,yvect)+(xdim<<11);
            y2 = dmulscale16(oy,xvect2,ox,yvect2)+(ydim<<11);

            drawline256(x1,y1,x2,y2,col);
        }
    }

    //Draw sprites
    k = g_player[screenpeek].ps->i;
    for (i=numsectors-1; i>=0; i--)
    {
        if (!(show2dsector[i>>3]&(1<<(i&7)))) continue;
        for (j=headspritesect[i]; j>=0; j=nextspritesect[j])
            //if ((show2dsprite[j>>3]&(1<<(j&7))) > 0)
        {
            spr = &sprite[j];

            if (j == k || (spr->cstat&0x8000) || spr->cstat == 257 || spr->xrepeat == 0) continue;

            col = 71; //cyan
            if (spr->cstat&1) col = 234; //magenta

            sprx = spr->x;
            spry = spr->y;

            if ((spr->cstat&257) != 0) switch (spr->cstat&48)
                {
                case 0:
                    break;

                    ox = sprx-cposx;
                    oy = spry-cposy;
                    x1 = dmulscale16(ox,xvect,-oy,yvect);
                    y1 = dmulscale16(oy,xvect2,ox,yvect2);

                    ox = (sintable[(spr->ang+512)&2047]>>7);
                    oy = (sintable[(spr->ang)&2047]>>7);
                    x2 = dmulscale16(ox,xvect,-oy,yvect);
                    y2 = dmulscale16(oy,xvect,ox,yvect);

                    x3 = mulscale16(x2,yxaspect);
                    y3 = mulscale16(y2,yxaspect);

                    drawline256(x1-x2+(xdim<<11),y1-y3+(ydim<<11),
                                x1+x2+(xdim<<11),y1+y3+(ydim<<11),col);
                    drawline256(x1-y2+(xdim<<11),y1+x3+(ydim<<11),
                                x1+x2+(xdim<<11),y1+y3+(ydim<<11),col);
                    drawline256(x1+y2+(xdim<<11),y1-x3+(ydim<<11),
                                x1+x2+(xdim<<11),y1+y3+(ydim<<11),col);
                    break;

                case 16:
                    if (spr->picnum == LASERLINE)
                    {
                        x1 = sprx;
                        y1 = spry;
                        tilenum = spr->picnum;
                        xoff = (int32_t)((int8_t)((picanm[tilenum]>>8)&255))+((int32_t)spr->xoffset);
                        if ((spr->cstat&4) > 0) xoff = -xoff;
                        k = spr->ang;
                        l = spr->xrepeat;
                        dax = sintable[k&2047]*l;
                        day = sintable[(k+1536)&2047]*l;
                        l = tilesizx[tilenum];
                        k = (l>>1)+xoff;
                        x1 -= mulscale16(dax,k);
                        x2 = x1+mulscale16(dax,l);
                        y1 -= mulscale16(day,k);
                        y2 = y1+mulscale16(day,l);

                        ox = x1-cposx;
                        oy = y1-cposy;
                        x1 = dmulscale16(ox,xvect,-oy,yvect);
                        y1 = dmulscale16(oy,xvect2,ox,yvect2);

                        ox = x2-cposx;
                        oy = y2-cposy;
                        x2 = dmulscale16(ox,xvect,-oy,yvect);
                        y2 = dmulscale16(oy,xvect2,ox,yvect2);

                        drawline256(x1+(xdim<<11),y1+(ydim<<11),
                                    x2+(xdim<<11),y2+(ydim<<11),col);
                    }

                    break;

                case 32:

                    tilenum = spr->picnum;
                    xoff = (int32_t)((int8_t)((picanm[tilenum]>>8)&255))+((int32_t)spr->xoffset);
                    yoff = (int32_t)((int8_t)((picanm[tilenum]>>16)&255))+((int32_t)spr->yoffset);
                    if ((spr->cstat&4) > 0) xoff = -xoff;
                    if ((spr->cstat&8) > 0) yoff = -yoff;

                    k = spr->ang;
                    cosang = sintable[(k+512)&2047];
                    sinang = sintable[k];
                    xspan = tilesizx[tilenum];
                    xrepeat = spr->xrepeat;
                    yspan = tilesizy[tilenum];
                    yrepeat = spr->yrepeat;

                    dax = ((xspan>>1)+xoff)*xrepeat;
                    day = ((yspan>>1)+yoff)*yrepeat;
                    x1 = sprx + dmulscale16(sinang,dax,cosang,day);
                    y1 = spry + dmulscale16(sinang,day,-cosang,dax);
                    l = xspan*xrepeat;
                    x2 = x1 - mulscale16(sinang,l);
                    y2 = y1 + mulscale16(cosang,l);
                    l = yspan*yrepeat;
                    k = -mulscale16(cosang,l);
                    x3 = x2+k;
                    x4 = x1+k;
                    k = -mulscale16(sinang,l);
                    y3 = y2+k;
                    y4 = y1+k;

                    ox = x1-cposx;
                    oy = y1-cposy;
                    x1 = dmulscale16(ox,xvect,-oy,yvect);
                    y1 = dmulscale16(oy,xvect2,ox,yvect2);

                    ox = x2-cposx;
                    oy = y2-cposy;
                    x2 = dmulscale16(ox,xvect,-oy,yvect);
                    y2 = dmulscale16(oy,xvect2,ox,yvect2);

                    ox = x3-cposx;
                    oy = y3-cposy;
                    x3 = dmulscale16(ox,xvect,-oy,yvect);
                    y3 = dmulscale16(oy,xvect2,ox,yvect2);

                    ox = x4-cposx;
                    oy = y4-cposy;
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

                    break;
                }
        }
    }

    //Draw white lines
    for (i=numsectors-1; i>=0; i--)
    {
        if (!(show2dsector[i>>3]&(1<<(i&7)))) continue;

        startwall = sector[i].wallptr;
        endwall = sector[i].wallptr + sector[i].wallnum;

        k = -1;
        for (j=startwall,wal=&wall[startwall]; j<endwall; j++,wal++)
        {
            if (wal->nextwall >= 0) continue;

            //if ((show2dwall[j>>3]&(1<<(j&7))) == 0) continue;

            if (tilesizx[wal->picnum] == 0) continue;
            if (tilesizy[wal->picnum] == 0) continue;

            if (j == k)
            {
                x1 = x2;
                y1 = y2;
            }
            else
            {
                ox = wal->x-cposx;
                oy = wal->y-cposy;
                x1 = dmulscale16(ox,xvect,-oy,yvect)+(xdim<<11);
                y1 = dmulscale16(oy,xvect2,ox,yvect2)+(ydim<<11);
            }

            k = wal->point2;
            wal2 = &wall[k];
            ox = wal2->x-cposx;
            oy = wal2->y-cposy;
            x2 = dmulscale16(ox,xvect,-oy,yvect)+(xdim<<11);
            y2 = dmulscale16(oy,xvect2,ox,yvect2)+(ydim<<11);

            drawline256(x1,y1,x2,y2,24);
        }
    }

    TRAVERSE_CONNECT(p)
    {
        if (ud.scrollmode && p == screenpeek) continue;

        ox = sprite[g_player[p].ps->i].x-cposx;
        oy = sprite[g_player[p].ps->i].y-cposy;
        daang = (sprite[g_player[p].ps->i].ang-cang)&2047;
        if (p == screenpeek)
        {
            ox = 0;
            oy = 0;
            daang = 0;
        }
        x1 = mulscale(ox,xvect,16) - mulscale(oy,yvect,16);
        y1 = mulscale(oy,xvect2,16) + mulscale(ox,yvect2,16);

        if (p == screenpeek || GTFLAGS(GAMETYPE_OTHERPLAYERSINMAP))
        {
            if (sprite[g_player[p].ps->i].xvel > 16 && g_player[p].ps->on_ground)
                i = APLAYERTOP+((totalclock>>4)&3);
            else
                i = APLAYERTOP;

            j = klabs(g_player[p].ps->truefz-g_player[p].ps->posz)>>8;
            j = mulscale(czoom*(sprite[g_player[p].ps->i].yrepeat+j),yxaspect,16);

            if (j < 22000) j = 22000;
            else if (j > (65536<<1)) j = (65536<<1);

            rotatesprite((x1<<4)+(xdim<<15),(y1<<4)+(ydim<<15),j,daang,i,sprite[g_player[p].ps->i].shade,
                         (g_player[p].ps->cursectnum > -1)?sector[g_player[p].ps->cursectnum].floorpal:0,
                         (sprite[g_player[p].ps->i].cstat&2)>>1,windowx1,windowy1,windowx2,windowy2);
        }
    }
}

#define CROSSHAIR_PAL (MAXPALOOKUPS-RESERVEDPALS-1)

palette_t CrosshairColors = { 255, 255, 255, 0 };
palette_t DefaultCrosshairColors = { 0, 0, 0, 0 };
int32_t g_crosshairSum = 0;

void G_GetCrosshairColor(void)
{
    if (DefaultCrosshairColors.f == 0)
    {
        // use the brightest color in the original 8-bit tile
        int32_t bri = 0, j = 0, i;
        int32_t ii;
        char *ptr = (char *)waloff[CROSSHAIR];

        if (waloff[CROSSHAIR] == 0)
        {
            loadtile(CROSSHAIR);
            ptr = (char *)waloff[CROSSHAIR];
        }

        ii = tilesizx[CROSSHAIR]*tilesizy[CROSSHAIR];

        if (ii <= 0) return;

        do
        {
            if (*ptr != 255)
            {
                i = curpalette[(int32_t)*ptr].r+curpalette[(int32_t)*ptr].g+curpalette[(int32_t)*ptr].b;
                if (i > j) { j = i; bri = *ptr; }
            }
            ptr++;
            ii--;
        }
        while (ii);

        Bmemcpy(&CrosshairColors, &curpalette[bri], sizeof(palette_t));
        Bmemcpy(&DefaultCrosshairColors, &curpalette[bri], sizeof(palette_t));
        DefaultCrosshairColors.f = 1; // this flag signifies that the color has been detected
    }
}

void G_SetCrosshairColor(int32_t r, int32_t g, int32_t b)
{
    char *ptr = (char *)waloff[CROSSHAIR];
    int32_t i, ii;

    if (DefaultCrosshairColors.f == 0 || g_crosshairSum == r+(g<<1)+(b<<2)) return;

    g_crosshairSum = r+(g<<1)+(b<<2);
    CrosshairColors.r = r;
    CrosshairColors.g = g;
    CrosshairColors.b = b;
    
    if (waloff[CROSSHAIR] == 0)
    {
        loadtile(CROSSHAIR);
        ptr = (char *)waloff[CROSSHAIR];
    }

    ii = tilesizx[CROSSHAIR]*tilesizy[CROSSHAIR];
    if (ii <= 0) return;

    if (getrendermode() < 3)
        i = getclosestcol(CrosshairColors.r>>2, CrosshairColors.g>>2, CrosshairColors.b>>2);
    else i = getclosestcol(63, 63, 63); // use white in GL so we can tint it to the right color

    do
    {
        if (*ptr != 255)
            *ptr = i;
        ptr++;
        ii--;
    }
    while (ii);

    for (i = 255; i >= 0; i--)
        tempbuf[i] = i;

    makepalookup(CROSSHAIR_PAL,tempbuf,CrosshairColors.r>>2, CrosshairColors.g>>2, CrosshairColors.b>>2,1);

#if defined(USE_OPENGL) && defined(POLYMOST)
    Bmemcpy(&hictinting[CROSSHAIR_PAL], &CrosshairColors, sizeof(palette_t));
    hictinting[CROSSHAIR_PAL].f = 9;
#endif
    invalidatetile(CROSSHAIR, -1, -1);
}

void G_FadePalette(int32_t r,int32_t g,int32_t b,int32_t e)
{
    int32_t tc;

    setpalettefade(r,g,b,e&127);
    if (getrendermode() >= 3) pus = pub = NUMPAGES; // JBF 20040110: redraw the status bar next time
    if ((e&128) == 0)
    {
        nextpage();
        for (tc = totalclock; totalclock < tc + 4; handleevents(), Net_GetPackets());
    }
}

void G_DisplayRest(int32_t smoothratio)
{
    int32_t a, i, j;
    int32_t applyTint=0;
    palette_t tempFade = { 0, 0, 0, 0 };
    palette_t tempTint = { 0, 0, 0, 0 };

    DukePlayer_t *pp = g_player[screenpeek].ps;
    walltype *wal;
    int32_t cposx, cposy, cang;

#if defined(USE_OPENGL) && defined(POLYMOST)

    // this takes care of fullscreen tint for OpenGL
    if (getrendermode() >= 3)
    {
        if (pp->palette == waterpal)
        {
//            if ((*(uint32_t *)&hictinting[MAXPALOOKUPS-2].r)|0x000000FF) == 0xFFFFFFFF)
            {
                static palette_t wp = { 224, 192, 255, 0 };
                Bmemcpy(&hictinting[MAXPALOOKUPS-1], &wp, sizeof(palette_t));
            }
//            else Bmemcpy(&hictinting[MAXPALOOKUPS-1], &hictinting[MAXPALOOKUPS-2], sizeof(palette_t));
        }
        else if (pp->palette == slimepal)
        {
//            if ((*(uint32_t *)&hictinting[MAXPALOOKUPS-3].r)|0x000000FF) == 0xFFFFFFFF)
                {
                    static palette_t sp = { 208, 255, 192, 0 };
                    Bmemcpy(&hictinting[MAXPALOOKUPS-1], &sp, sizeof(palette_t));
                }

//            else Bmemcpy(&hictinting[MAXPALOOKUPS-1], &hictinting[MAXPALOOKUPS-3], sizeof(palette_t));
        }
        else
        {
            *(uint32_t *)&hictinting[MAXPALOOKUPS-1].r = 0xFFFFFFFF;
        }
    }
#endif /* USE_OPENGL && POLYMOST */
    // this does pain tinting etc from the CON
    if (pp->pals_time >= 0 && pp->loogcnt == 0) // JBF 20040101: pals_time > 0 now >= 0
    {
//        tempFade = (palette_t) { pp->pals[0], pp->pals[1], pp->pals[2], pp->pals_time };
        Bmemcpy(&tempFade, &pp->pals, sizeof(palette_t));
        tempFade.f = pp->pals_time;
        g_restorePalette = 1;     // JBF 20040101
        applyTint = 1;
    }
    // reset a normal palette
    else if (g_restorePalette)
    {
        //setbrightness(ud.brightness>>2,&pp->palette[0],0);
        P_SetGamePalette(pp,pp->palette,2);
        g_restorePalette = 0;
    }
    // loogies courtesy of being snotted on
    else if (pp->loogcnt > 0)
    {
        //G_FadePalette(0,64,0,(pp->loogcnt>>1)+128);
        palette_t lp = { 0, 64, 0, pp->loogcnt>>1 };
        Bmemcpy(&tempFade, &lp, sizeof(palette_t));
        applyTint = 1;
    }
    if (tempFade.f > tempTint.f)
        Bmemcpy(&tempTint, &tempFade, sizeof(palette_t));

    if (ud.show_help)
    {
        switch (ud.show_help)
        {
        case 1:
            rotatesprite(0,0,65536L,0,TEXTSTORY,0,0,10+16+64, 0,0,xdim-1,ydim-1);
            break;
        case 2:
            rotatesprite(0,0,65536L,0,F1HELP,0,0,10+16+64, 0,0,xdim-1,ydim-1);
            break;
        }

        if (KB_KeyPressed(sc_Escape) || MOUSE_GetButtons()&RIGHT_MOUSE)
        {
            KB_ClearKeyDown(sc_Escape);
            MOUSE_ClearButton(RIGHT_MOUSE);
            ud.show_help = 0;
            if (ud.multimode < 2 && ud.recstat != 2)
            {
                ready2send = 1;
                totalclock = ototalclock;
            }
            G_UpdateScreenArea();
        }
        if (tempTint.f > 0 || applyTint) G_FadePalette(tempTint.r,tempTint.g,tempTint.b,tempTint.f|128);
        return;
    }

    i = pp->cursectnum;
    if (i > -1)
    {
        show2dsector[i>>3] |= (1<<(i&7));
        wal = &wall[sector[i].wallptr];
        for (j=sector[i].wallnum; j>0; j--,wal++)
        {
            i = wal->nextsector;
            if (i < 0) continue;
            if (wal->cstat&0x0071) continue;
            if (wall[wal->nextwall].cstat&0x0071) continue;
            if (sector[i].lotag == 32767) continue;
            if (sector[i].ceilingz >= sector[i].floorz) continue;
            show2dsector[i>>3] |= (1<<(i&7));
        }
    }

    if (ud.camerasprite == -1)
    {
        if (ud.overhead_on != 2)
        {
            if (pp->newowner >= 0)
                G_DrawCameraText(pp->newowner);
            else
            {
                P_DisplayWeapon(screenpeek);
                if (pp->over_shoulder_on == 0)
                    P_DisplayScubaMask(screenpeek);
            }
            G_MoveClouds();
        }

        if (ud.overhead_on > 0)
        {
            // smoothratio = min(max(smoothratio,0),65536);
            smoothratio = min(max((totalclock - ototalclock) * (65536 / 4),0),65536);
            G_DoInterpolations(smoothratio);
            if (ud.scrollmode == 0)
            {
                if (pp->newowner == -1 && !ud.pause_on)
                {
                    if (screenpeek == myconnectindex && numplayers > 1)
                    {
                        cposx = omy.x+mulscale16((int32_t)(my.x-omy.x),smoothratio);
                        cposy = omy.y+mulscale16((int32_t)(my.y-omy.y),smoothratio);
                        cang = omyang+mulscale16((int32_t)(((myang+1024-omyang)&2047)-1024),smoothratio);
                    }
                    else
                    {
                        cposx = pp->oposx+mulscale16((int32_t)(pp->posx-pp->oposx),smoothratio);
                        cposy = pp->oposy+mulscale16((int32_t)(pp->posy-pp->oposy),smoothratio);
                        cang = pp->oang+mulscale16((int32_t)(((pp->ang+1024-pp->oang)&2047)-1024),smoothratio);
                    }
                }
                else
                {
                    cposx = pp->oposx;
                    cposy = pp->oposy;
                    cang = pp->oang;
                }
            }
            else
            {
                if (!ud.pause_on)
                {
                    ud.fola += ud.folavel>>3;
                    ud.folx += (ud.folfvel*sintable[(512+2048-ud.fola)&2047])>>14;
                    ud.foly += (ud.folfvel*sintable[(512+1024-512-ud.fola)&2047])>>14;
                }
                cposx = ud.folx;
                cposy = ud.foly;
                cang = ud.fola;
            }

            if (ud.overhead_on == 2)
            {
                clearview(0L);
                drawmapview(cposx,cposy,pp->zoom,cang);
            }
            G_DrawOverheadMap(cposx,cposy,pp->zoom,cang);

            G_RestoreInterpolations();

            if (ud.overhead_on == 2)
            {
                if (ud.screen_size > 0) a = 147;
                else a = 179;
                minitext(5,a+6,EpisodeNames[ud.volume_number],0,2+8+16);
                minitext(5,a+6+6,MapInfo[ud.volume_number*MAXLEVELS + ud.level_number].name,0,2+8+16);
            }
        }
    }

    if (pp->invdisptime > 0) G_DrawInventory(pp);

    aGameVars[g_iReturnVarID].val.lValue = 0;
    if (apScriptGameEvent[EVENT_DISPLAYSBAR])
        X_OnEvent(EVENT_DISPLAYSBAR, g_player[screenpeek].ps->i, screenpeek, -1);
    if (aGameVars[g_iReturnVarID].val.lValue == 0)
        G_DrawStatusBar(screenpeek);

    G_PrintGameQuotes();

    if (ud.show_level_text && hud_showmapname && g_levelTextTime > 1)
    {
        int32_t bits = 10+16;

        if (g_levelTextTime < 3)
            bits |= 1+32;
        else if (g_levelTextTime < 5)
            bits |= 1;

        if (MapInfo[(ud.volume_number*MAXLEVELS) + ud.level_number].name != NULL)
        {
            if (currentboardfilename[0] != 0 && ud.volume_number == 0 && ud.level_number == 7)
                menutext_(160,75,-g_levelTextTime+22/*(sintable[(totalclock<<5)&2047]>>11)*/,0,currentboardfilename,bits);
            else menutext_(160,75,-g_levelTextTime+22/*(sintable[(totalclock<<5)&2047]>>11)*/,0,MapInfo[(ud.volume_number*MAXLEVELS) + ud.level_number].name,bits);
        }
    }

    if (KB_KeyPressed(sc_Escape) && ud.overhead_on == 0
            && ud.show_help == 0
            && g_player[myconnectindex].ps->newowner == -1)
    {
        if ((g_player[myconnectindex].ps->gm&MODE_MENU) == MODE_MENU && g_currentMenu < 51)
        {
            KB_ClearKeyDown(sc_Escape);
            g_player[myconnectindex].ps->gm &= ~MODE_MENU;
            if (ud.multimode < 2 && ud.recstat != 2)
            {
                ready2send = 1;
                totalclock = ototalclock;
                g_cameraClock = totalclock;
                g_cameraDistance = 65536L;
            }
            walock[TILE_SAVESHOT] = 199;
            G_UpdateScreenArea();
        }
        else if ((g_player[myconnectindex].ps->gm&MODE_MENU) != MODE_MENU &&
                 g_player[myconnectindex].ps->newowner == -1 &&
                 (g_player[myconnectindex].ps->gm&MODE_TYPE) != MODE_TYPE)
        {
            KB_ClearKeyDown(sc_Escape);
            FX_StopAllSounds();
            S_ClearSoundLocks();

            S_MenuSound();

            g_player[myconnectindex].ps->gm |= MODE_MENU;

            if (ud.multimode < 2 && ud.recstat != 2) ready2send = 0;

            if (g_player[myconnectindex].ps->gm&MODE_GAME) ChangeToMenu(50);
            else ChangeToMenu(0);
            screenpeek = myconnectindex;
        }
    }

    if (apScriptGameEvent[EVENT_DISPLAYREST])
        X_OnEvent(EVENT_DISPLAYREST, g_player[screenpeek].ps->i, screenpeek, -1);

    if (g_player[myconnectindex].ps->newowner == -1 && ud.overhead_on == 0 && ud.crosshair && ud.camerasprite == -1)
    {
        aGameVars[g_iReturnVarID].val.lValue = 0;
        if (apScriptGameEvent[EVENT_DISPLAYCROSSHAIR])
            X_OnEvent(EVENT_DISPLAYCROSSHAIR, g_player[screenpeek].ps->i, screenpeek, -1);
        if (aGameVars[g_iReturnVarID].val.lValue == 0)
            rotatesprite((160L-(g_player[myconnectindex].ps->look_ang>>1))<<16,100L<<16,scale(65536,ud.crosshairscale,100),0,CROSSHAIR,0,CROSSHAIR_PAL,2+1,windowx1,windowy1,windowx2,windowy2);
    }
#if 0
    if (GametypeFlags[ud.coop] & GAMETYPE_TDM)
    {
        for (i=0; i<ud.multimode; i++)
        {
            if (g_player[i].ps->team == g_player[myconnectindex].ps->team)
            {
                j = min(max((G_GetAngleDelta(getangle(g_player[i].ps->posx-g_player[myconnectindex].ps->posx,g_player[i].ps->posy-g_player[myconnectindex].ps->posy),g_player[myconnectindex].ps->ang))>>1,-160),160);
                rotatesprite((160-j)<<16,100L<<16,65536L,0,DUKEICON,0,0,2+1,windowx1,windowy1,windowx2,windowy2);
            }
        }
    }
#endif

    if (ud.pause_on==1 && (g_player[myconnectindex].ps->gm&MODE_MENU) == 0)
        menutext(160,100,0,0,"GAME PAUSED");

    if (ud.coords)
        G_PrintCoords(screenpeek);

#if defined(POLYMOST) && defined(USE_OPENGL)
    {
        extern int32_t mdpause;

        mdpause = 0;
        if (ud.pause_on || (ud.recstat==2 && (demo_paused && demo_goalreccnt==0)) || (g_player[myconnectindex].ps->gm&MODE_MENU && numplayers < 2))
            mdpause = 1;
    }
#endif

    G_PrintFPS();

    // JBF 20040124: display level stats in screen corner
    if ((ud.overhead_on != 2 && ud.levelstats) && (g_player[myconnectindex].ps->gm&MODE_MENU) == 0)
    {
        if (ud.screen_size == 4)
        {
            i = scale(ud.althud?tilesizy[BIGALPHANUM]+10:tilesizy[INVENTORYBOX]+2,ud.statusbarscale,100);
//            j = scale(scale(6,ud.config.ScreenWidth,320),ud.statusbarscale,100);
        }
        else if (ud.screen_size > 2)
        {
            i = scale(tilesizy[BOTTOMSTATUSBAR]+1,ud.statusbarscale,100);
            //          j = scale(2,ud.config.ScreenWidth,320);
        }
        else
        {
            i = 2;
            //        j = scale(2,ud.config.ScreenWidth,320);
        }
        j = scale(2,ud.config.ScreenWidth,320);

        Bsprintf(tempbuf,"T:^15%d:%02d.%02d",
                 (g_player[myconnectindex].ps->player_par/(GAMETICSPERSEC*60)),
                 (g_player[myconnectindex].ps->player_par/GAMETICSPERSEC)%60,
                 ((g_player[myconnectindex].ps->player_par%GAMETICSPERSEC)*38)/10
                );
        gametext_z(13,STARTALPHANUM, j,scale(200-i,ud.config.ScreenHeight,200)-textsc(21),tempbuf,0,10,26,0, 0, xdim-1, ydim-1, 65536);

        if (ud.player_skill > 3 || (ud.multimode > 1 && !GTFLAGS(GAMETYPE_PLAYERSFRIENDLY)))
            Bsprintf(tempbuf,"K:^15%d",(ud.multimode>1 &&!GTFLAGS(GAMETYPE_PLAYERSFRIENDLY))?g_player[myconnectindex].ps->frag-g_player[myconnectindex].ps->fraggedself:g_player[myconnectindex].ps->actors_killed);
        else
        {
            if (g_player[myconnectindex].ps->actors_killed >= g_player[myconnectindex].ps->max_actors_killed)
                Bsprintf(tempbuf,"K:%d/%d",g_player[myconnectindex].ps->actors_killed,
                         g_player[myconnectindex].ps->max_actors_killed>g_player[myconnectindex].ps->actors_killed?
                         g_player[myconnectindex].ps->max_actors_killed:g_player[myconnectindex].ps->actors_killed);
            else
                Bsprintf(tempbuf,"K:^15%d/%d",g_player[myconnectindex].ps->actors_killed,
                         g_player[myconnectindex].ps->max_actors_killed>g_player[myconnectindex].ps->actors_killed?
                         g_player[myconnectindex].ps->max_actors_killed:g_player[myconnectindex].ps->actors_killed);
        }
        gametext_z(13,STARTALPHANUM, j,scale(200-i,ud.config.ScreenHeight,200)-textsc(14),tempbuf,0,10,26,0, 0, xdim-1, ydim-1, 65536);

        if (g_player[myconnectindex].ps->secret_rooms == g_player[myconnectindex].ps->max_secret_rooms)
            Bsprintf(tempbuf,"S:%d/%d", g_player[myconnectindex].ps->secret_rooms,g_player[myconnectindex].ps->max_secret_rooms);
        else Bsprintf(tempbuf,"S:^15%d/%d", g_player[myconnectindex].ps->secret_rooms,g_player[myconnectindex].ps->max_secret_rooms);
        gametext_z(13,STARTALPHANUM, j,scale(200-i,ud.config.ScreenHeight,200)-textsc(7),tempbuf,0,10,26,0, 0, xdim-1, ydim-1, 65536);
    }

    if (g_player[myconnectindex].ps->gm&MODE_TYPE)
        Net_EnterMessage();
    else
        M_DisplayMenus();

    if (tempTint.f > 0 || applyTint)
        G_FadePalette(tempTint.r,tempTint.g,tempTint.b,tempTint.f|128);
}

static void G_DoThirdPerson(DukePlayer_t *pp, vec3_t *vect,int16_t *vsectnum, int32_t ang, int32_t horiz)
{
    spritetype *sp = &sprite[pp->i];
    int32_t i, hx, hy;
    int32_t nx = (sintable[(ang+1536)&2047]>>4);
    int32_t ny = (sintable[(ang+1024)&2047]>>4);
    int32_t nz = (horiz-100)*128;
    int32_t daang;
    int32_t bakcstat = sp->cstat;
    hitdata_t hitinfo;

    sp->cstat &= (int16_t)~0x101;

    updatesectorz(vect->x,vect->y,vect->z,vsectnum);
    hitscan((const vec3_t *)vect,*vsectnum,nx,ny,nz,&hitinfo,CLIPMASK1);

    if (*vsectnum < 0)
    {
        sp->cstat = bakcstat;
        return;
    }

    hx = hitinfo.pos.x-(vect->x);
    hy = hitinfo.pos.y-(vect->y);
    if (klabs(nx)+klabs(ny) > klabs(hx)+klabs(hy))
    {
        *vsectnum = hitinfo.hitsect;
        if (hitinfo.hitwall >= 0)
        {
            daang = getangle(wall[wall[hitinfo.hitwall].point2].x-wall[hitinfo.hitwall].x,
                             wall[wall[hitinfo.hitwall].point2].y-wall[hitinfo.hitwall].y);

            i = nx*sintable[daang]+ny*sintable[(daang+1536)&2047];
            if (klabs(nx) > klabs(ny)) hx -= mulscale28(nx,i);
            else hy -= mulscale28(ny,i);
        }
        else if (hitinfo.hitsprite < 0)
        {
            if (klabs(nx) > klabs(ny)) hx -= (nx>>5);
            else hy -= (ny>>5);
        }
        if (klabs(nx) > klabs(ny)) i = divscale16(hx,nx);
        else i = divscale16(hy,ny);
        if (i < g_cameraDistance) g_cameraDistance = i;
    }
    vect->x += mulscale16(nx,g_cameraDistance);
    vect->y += mulscale16(ny,g_cameraDistance);
    vect->z += mulscale16(nz,g_cameraDistance);

    g_cameraDistance = min(g_cameraDistance+((totalclock-g_cameraClock)<<10),65536);
    g_cameraClock = totalclock;

    updatesectorz(vect->x,vect->y,vect->z,vsectnum);

    sp->cstat = bakcstat;
}

//REPLACE FULLY
void G_DrawBackground(void)
{
    int32_t dapicnum;
    int32_t x,y,x1,y1,x2,y2,rx;

    flushperms();

    switch (ud.m_volume_number)
    {
    default:
        dapicnum = BIGHOLE;
        break;
    case 1:
        dapicnum = BIGHOLE;
        break;
    case 2:
        dapicnum = BIGHOLE;
        break;
    }

    if (tilesizx[dapicnum] == 0 || tilesizy[dapicnum] == 0)
    {
        pus = pub = NUMPAGES;
        return;
    }

    y1 = 0;
    y2 = ydim;
    if (g_player[myconnectindex].ps->gm & MODE_GAME || ud.recstat == 2)
        //if (ud.recstat == 0 || ud.recstat == 1 || (ud.recstat == 2 && ud.reccnt > 0)) // JBF 20040717
    {
//        if (ud.screen_size == 8 && ud.statusbarmode == 0)
        //          y1 = scale(ydim,200-scale(tilesizy[BOTTOMSTATUSBAR],ud.statusbarscale,100),200);
        //else
        if (GametypeFlags[ud.coop] & GAMETYPE_FRAGBAR)
        {
            if (ud.multimode > 1) y1 += scale(ydim,8,200);
            if (ud.multimode > 4) y1 += scale(ydim,8,200);
        }
    }
    else
    {
        // when not rendering a game, fullscreen wipe
#define MENUTILE (!getrendermode()?MENUSCREEN:LOADSCREEN)
//        Gv_SetVar(g_iReturnVarID,tilesizx[MENUTILE]==320&&tilesizy[MENUTILE]==200?MENUTILE:BIGHOLE, -1, -1);
        aGameVars[g_iReturnVarID].val.lValue = (tilesizx[MENUTILE]==320&&tilesizy[MENUTILE]==200?MENUTILE:BIGHOLE);
        if (apScriptGameEvent[EVENT_GETMENUTILE])
            X_OnEvent(EVENT_GETMENUTILE, -1, myconnectindex, -1);
        if (Gv_GetVarByLabel("MENU_TILE", tilesizx[MENUTILE]==320&&tilesizy[MENUTILE]==200?0:1, -1, -1))
        {
            for (y=y1; y<y2; y+=tilesizy[aGameVars[g_iReturnVarID].val.lValue])
                for (x=0; x<xdim; x+=tilesizx[aGameVars[g_iReturnVarID].val.lValue])
                    rotatesprite(x<<16,y<<16,65536L,0,aGameVars[g_iReturnVarID].val.lValue,bpp==8?16:8,0,8+16+64,0,0,xdim-1,ydim-1);
        }
        else rotatesprite(320<<15,200<<15,65536L,0,aGameVars[g_iReturnVarID].val.lValue,bpp==8?16:8,0,2+8+64,0,0,xdim-1,ydim-1);
        return;
    }

    y2 = scale(ydim,200-scale(tilesizy[BOTTOMSTATUSBAR],ud.statusbarscale,100),200);

    if (ud.screen_size > 8)
    {
        // across top
        for (y=0; y<windowy1; y+=tilesizy[dapicnum])
            for (x=0; x<xdim; x+=tilesizx[dapicnum])
                rotatesprite(x<<16,y<<16,65536L,0,dapicnum,8,0,8+16+64,0,y1,xdim-1,windowy1-1);

        // sides
        rx = windowx2-windowx2%tilesizx[dapicnum];
        for (y=windowy1-windowy1%tilesizy[dapicnum]; y<windowy2; y+=tilesizy[dapicnum])
            for (x=0; x<windowx1 || x+rx<xdim; x+=tilesizx[dapicnum])
            {
                rotatesprite(x<<16,y<<16,65536L,0,dapicnum,8,0,8+16+64,0,windowy1,windowx1-1,windowy2-1);
                rotatesprite((x+rx)<<16,y<<16,65536L,0,dapicnum,8,0,8+16+64,windowx2,windowy1,xdim-1,windowy2-1);
            }

        // along bottom
        for (y=windowy2-(windowy2%tilesizy[dapicnum]); y<y2; y+=tilesizy[dapicnum])
            for (x=0; x<xdim; x+=tilesizx[dapicnum])
                rotatesprite(x<<16,y<<16,65536L,0,dapicnum,8,0,8+16+64,0,windowy2,xdim-1,y2-1);
    }

    // draw in the bits to the left and right of the non-fullsize status bar
    if (ud.statusbarscale < 100 && ud.screen_size >= 8 && ud.statusbarmode == 0)
    {
        /*
        y1 = y2;
        x2 = (xdim - scale(xdim,ud.statusbarscale,100)) >> 1;
        x1 = xdim-x2;
        x1 -= x1%tilesizx[dapicnum];
        for (y=y1-y1%tilesizy[dapicnum]; y<y2; y+=tilesizy[dapicnum])
            for (x=0;x<x2 || x1+x<xdim; x+=tilesizx[dapicnum])
            {
                rotatesprite(x<<16,y<<16,65536L,0,dapicnum,8,0,8+16+64,0,y1,x2-1,ydim-1);
                rotatesprite((x+x1)<<16,y<<16,65536L,0,dapicnum,8,0,8+16+64,xdim-x2,y1,xdim-1,ydim-1);
            }
            */
        // when not rendering a game, fullscreen wipe
        x2 = (xdim - scale(xdim,ud.statusbarscale,100)) >> 1;
        for (y=y2-y2%tilesizy[dapicnum]; y<ydim; y+=tilesizy[dapicnum])
            for (x=0; x<xdim>>1; x+=tilesizx[dapicnum])
            {
                rotatesprite(x<<16,y<<16,65536L,0,dapicnum,8,0,8+16+64,0,y2,x2,ydim-1);
                rotatesprite((xdim-x)<<16,y<<16,65536L,0,dapicnum,8,0,8+16+64,xdim-x2-1,y2,xdim-1,ydim-1);
            }

    }

    if (ud.screen_size > 8)
    {
        y = 0;
        if (GametypeFlags[ud.coop] & GAMETYPE_FRAGBAR)
        {
            if (ud.multimode > 1) y += 8;
            if (ud.multimode > 4) y += 8;
        }

        x1 = max(windowx1-4,0);
        y1 = max(windowy1-4,y);
        x2 = min(windowx2+4,xdim-1);
        y2 = min(windowy2+4,scale(ydim,200-scale(tilesizy[BOTTOMSTATUSBAR],ud.statusbarscale,100),200)-1);

        for (y=y1+4; y<y2-4; y+=64)
        {
            rotatesprite(x1<<16,y<<16,65536L,0,VIEWBORDER,0,0,8+16+64,x1,y1,x2,y2);
            rotatesprite((x2+1)<<16,(y+64)<<16,65536L,1024,VIEWBORDER,0,0,8+16+64,x1,y1,x2,y2);
        }

        for (x=x1+4; x<x2-4; x+=64)
        {
            rotatesprite((x+64)<<16,y1<<16,65536L,512,VIEWBORDER,0,0,8+16+64,x1,y1,x2,y2);
            rotatesprite(x<<16,(y2+1)<<16,65536L,1536,VIEWBORDER,0,0,8+16+64,x1,y1,x2,y2);
        }

        rotatesprite(x1<<16,y1<<16,65536L,0,VIEWBORDER+1,0,0,8+16+64,x1,y1,x2,y2);
        rotatesprite((x2+1)<<16,y1<<16,65536L,512,VIEWBORDER+1,0,0,8+16+64,x1,y1,x2,y2);
        rotatesprite((x2+1)<<16,(y2+1)<<16,65536L,1024,VIEWBORDER+1,0,0,8+16+64,x1,y1,x2,y2);
        rotatesprite(x1<<16,(y2+1)<<16,65536L,1536,VIEWBORDER+1,0,0,8+16+64,x1,y1,x2,y2);
    }

    pus = pub = NUMPAGES;
}

static int32_t ror_sprite = -1;

static int32_t oyrepeat=-1;
extern float r_ambientlight;

char ror_protectedsectors[MAXSECTORS];
int32_t drawing_ror = 0;

void G_SE40(int32_t smoothratio)
{
    if (getrendermode() != 4 && ror_sprite != -1)
    {
        int32_t x, y, z;
        int16_t sect;
        int32_t level = 0;
        spritetype *sp = &sprite[ror_sprite];
        int32_t sprite2 = sp->yvel;

        if (klabs(sector[sp->sectnum].floorz - sp->z) < klabs(sector[sprite[sprite2].sectnum].floorz - sprite[sprite2].z))
            level = 1;

        x = ud.camera.x - sp->x;
        y = ud.camera.y - sp->y;
        z = ud.camera.z - (level ? sector[sp->sectnum].floorz : sector[sp->sectnum].ceilingz);

        sect = sprite[sprite2].sectnum;
        updatesector(sprite[sprite2].x + x, sprite[sprite2].y + y, &sect);

        if (sect != -1)
        {
            int32_t renderz, picnum;
            int16_t backupstat[MAXSECTORS];
            int32_t backupz[MAXSECTORS];
            int32_t i;
            int32_t pix_diff, newz;
            //                initprintf("drawing ror\n");

            if (level)
            {
                // renderz = sector[sprite[sprite2].sectnum].ceilingz;
                renderz = sprite[sprite2].z - (sprite[sprite2].yrepeat * tilesizy[sprite[sprite2].picnum]<<1);
                picnum = sector[sprite[sprite2].sectnum].ceilingpicnum;
                sector[sprite[sprite2].sectnum].ceilingpicnum = 562;
                tilesizx[562] = tilesizy[562] = 0;

                pix_diff = klabs(z) >> 8;
                newz = - ((pix_diff / 128) + 1) * (128<<8);

                for (i = 0; i < numsectors; i++)
                {
                    backupstat[i] = sector[i].ceilingstat;
                    backupz[i] = sector[i].ceilingz;
                    if (!ror_protectedsectors[i] || (ror_protectedsectors[i] && sp->lotag == 41))
                    {
                        sector[i].ceilingstat = 1;
                        sector[i].ceilingz += newz;
                    }
                }
            }
            else
            {
                // renderz = sector[sprite[sprite2].sectnum].floorz;
                renderz = sprite[sprite2].z;
                picnum = sector[sprite[sprite2].sectnum].floorpicnum;
                sector[sprite[sprite2].sectnum].floorpicnum = 562;
                tilesizx[562] = tilesizy[562] = 0;

                pix_diff = klabs(z) >> 8;
                newz = ((pix_diff / 128) + 1) * (128<<8);

                for (i = 0; i < numsectors; i++)
                {
                    backupstat[i] = sector[i].floorstat;
                    backupz[i] = sector[i].floorz;
                    if (!ror_protectedsectors[i] || (ror_protectedsectors[i] && sp->lotag == 41))
                    {
                        sector[i].floorstat = 1;
                        sector[i].floorz = +newz;
                    }
                }
            }

            drawrooms(sprite[sprite2].x + x, sprite[sprite2].y + y,
                      z + renderz, ud.cameraang, ud.camerahoriz, sect);
            drawing_ror = 1 + level;

            // dupe the sprites touching the portal to the other sector

            if (drawing_ror == 2) // viewing from top
            {
                int32_t k = headspritesect[sp->sectnum];

                while (k != -1)
                {
                    if (sprite[k].picnum != SECTOREFFECTOR && (sprite[k].z >= sp->z))
                    {
                        Bmemcpy((spritetype *)&tsprite[spritesortcnt],(spritetype *)&sprite[k],sizeof(spritetype));

                        tsprite[spritesortcnt].x += (sprite[sp->yvel].x-sp->x);
                        tsprite[spritesortcnt].y += (sprite[sp->yvel].y-sp->y);
                        tsprite[spritesortcnt].z = tsprite[spritesortcnt].z - sp->z + ActorExtra[sp->yvel].ceilingz;
                        tsprite[spritesortcnt].sectnum = sprite[sp->yvel].sectnum;
                        tsprite[spritesortcnt].owner = k;

                        //OSD_Printf("duped sprite of pic %d at %d %d %d\n",tsprite[spritesortcnt].picnum,tsprite[spritesortcnt].x,tsprite[spritesortcnt].y,tsprite[spritesortcnt].z);
                        spritesortcnt++;
                    }
                    k = nextspritesect[k];
                }
            }

            G_DoSpriteAnimations(ud.camera.x,ud.camera.y,ud.cameraang,smoothratio);
            drawmasks();

            if (level)
            {
                sector[sprite[sprite2].sectnum].ceilingpicnum = picnum;
                for (i = 0; i < numsectors; i++)
                {
                    sector[i].ceilingstat = backupstat[i];
                    sector[i].ceilingz = backupz[i];
                }
            }
            else
            {
                sector[sprite[sprite2].sectnum].floorpicnum = picnum;

                for (i = 0; i < numsectors; i++)
                {
                    sector[i].floorstat = backupstat[i];
                    sector[i].floorz = backupz[i];
                }
            }
        }
    }
}

void G_DrawRooms(int32_t snum, int32_t smoothratio)
{
    int32_t dst,j,fz,cz;
    int32_t tposx,tposy,i;
    int16_t k;
    DukePlayer_t *p = g_player[snum].ps;
    int16_t tang;
    int32_t tiltcx,tiltcy,tiltcs=0;    // JBF 20030807

    if (pub > 0 || getrendermode() >= 3) // JBF 20040101: redraw background always
    {
        if (getrendermode() >= 3 || ud.screen_size > 8 || (ud.screen_size == 8 && ud.statusbarscale<100))
            G_DrawBackground();
        pub = 0;
    }

    if (ud.overhead_on == 2 || ud.show_help || (p->cursectnum == -1 && getrendermode() < 3))
        return;

//    smoothratio = min(max(smoothratio,0),65536);
    smoothratio = min(max((totalclock - ototalclock) * (65536 / 4),0),65536);

    visibility = (int32_t)(p->visibility*(numplayers>1?1.f:r_ambientlightrecip));

    if (ud.pause_on || g_player[snum].ps->on_crane > -1) smoothratio = 65536;

    ud.camerasect = p->cursectnum;

    if (getrendermode() < 3 && (ud.camerasect < 0 || ud.camerasect >= MAXSECTORS)) return;

    G_DoInterpolations(smoothratio);

    G_AnimateCamSprite();

    if (ud.camerasprite >= 0)
    {
        spritetype *s = &sprite[ud.camerasprite];

        if (s->yvel < 0) s->yvel = -100;
        else if (s->yvel > 199) s->yvel = 300;

        ud.cameraang = ActorExtra[ud.camerasprite].tempang+mulscale16((int32_t)(((s->ang+1024-ActorExtra[ud.camerasprite].tempang)&2047)-1024),smoothratio);
#ifdef POLYMER
        if (getrendermode() == 4)
            polymer_setanimatesprites(G_DoSpriteAnimations, s->x, s->y, ud.cameraang, smoothratio);
#endif

        G_SE40(smoothratio);

        drawrooms(s->x,s->y,s->z-(4<<8),ud.cameraang,s->yvel,s->sectnum);
        G_DoSpriteAnimations(s->x,s->y,ud.cameraang,smoothratio);
        drawmasks();
    }
    else
    {
        i = divscale22(1,sprite[p->i].yrepeat+28);
        if (i != oyrepeat)
        {
            oyrepeat = i;
            setaspect(oyrepeat,yxaspect);
        }

        if (g_screenCapture)
        {
            walock[TILE_SAVESHOT] = 199;
            if (waloff[TILE_SAVESHOT] == 0)
                allocache(&waloff[TILE_SAVESHOT],200*320,&walock[TILE_SAVESHOT]);
            setviewtotile(TILE_SAVESHOT,200L,320L);
        }
        else if (getrendermode() == 0 && ((ud.screen_tilting && p->rotscrnang) || ud.detail==0))
        {
            if (ud.screen_tilting) tang = p->rotscrnang;
            else tang = 0;

            if (xres <= 320 && yres <= 240)
            {
                // JBF 20030807: Increased tilted-screen quality
                tiltcs = 1;
                tiltcx = 320;
                tiltcy = 200;
            }
            else
            {
                tiltcs = 2;
                tiltcx = 640;
                tiltcy = 480;
            }

            walock[TILE_TILT] = 255;
            if (waloff[TILE_TILT] == 0)
                allocache(&waloff[TILE_TILT],tiltcx*tiltcx,&walock[TILE_TILT]);
            if ((tang&1023) == 0)
                setviewtotile(TILE_TILT,tiltcy>>(1-ud.detail),tiltcx>>(1-ud.detail));
            else
                setviewtotile(TILE_TILT,tiltcx>>(1-ud.detail),tiltcx>>(1-ud.detail));
            if ((tang&1023) == 512)
            {
                //Block off unscreen section of 90ø tilted screen
                j = ((tiltcx-(60*tiltcs))>>(1-ud.detail));
                for (i=((60*tiltcs)>>(1-ud.detail))-1; i>=0; i--)
                {
                    startumost[i] = 1;
                    startumost[i+j] = 1;
                    startdmost[i] = 0;
                    startdmost[i+j] = 0;
                }
            }

            i = (tang&511);
            if (i > 256) i = 512-i;
            i = sintable[i+512]*8 + sintable[i]*5L;
            setaspect(i>>1,yxaspect);
        }
        else if (getrendermode() > 0 && ud.screen_tilting /*&& (p->rotscrnang || p->orotscrnang)*/)
        {
#ifdef POLYMOST
            setrollangle(p->orotscrnang + mulscale16(((p->rotscrnang - p->orotscrnang + 1024)&2047)-1024,smoothratio));
#endif
            p->orotscrnang = p->rotscrnang; // JBF: save it for next time
        }

        if ((snum == myconnectindex) && (numplayers > 1))
        {
            vec3_t cam = { omy.x+mulscale16((int32_t)(my.x-omy.x),smoothratio),
                                   omy.y+mulscale16((int32_t)(my.y-omy.y),smoothratio),
                                   omy.z+mulscale16((int32_t)(my.z-omy.z),smoothratio) };

            Bmemcpy(&ud.camera, &cam, sizeof(vec3_t));
            ud.cameraang = omyang+mulscale16((int32_t)(((myang+1024-omyang)&2047)-1024),smoothratio);
            ud.camerahoriz = omyhoriz+omyhorizoff+mulscale16((int32_t)(myhoriz+myhorizoff-omyhoriz-omyhorizoff),smoothratio);
            ud.camerasect = mycursectnum;
        }
        else
        {
            vec3_t cam = { p->oposx+mulscale16((int32_t)(p->posx-p->oposx),smoothratio),
                                   p->oposy+mulscale16((int32_t)(p->posy-p->oposy),smoothratio),
                                   p->oposz+mulscale16((int32_t)(p->posz-p->oposz),smoothratio) };

            Bmemcpy(&ud.camera, &cam, sizeof(vec3_t));
            ud.cameraang = p->oang+mulscale16((int32_t)(((p->ang+1024-p->oang)&2047)-1024),smoothratio);
            ud.camerahoriz = p->ohoriz+p->ohorizoff+mulscale16((int32_t)(p->horiz+p->horizoff-p->ohoriz-p->ohorizoff),smoothratio);
        }
        ud.cameraang += p->look_ang;

        if (p->newowner >= 0)
        {
            ud.cameraang = p->ang+p->look_ang;
            ud.camerahoriz = p->horiz+p->horizoff;
            Bmemcpy(&ud.camera, p, sizeof(vec3_t));
            ud.camerasect = sprite[p->newowner].sectnum;
            smoothratio = 65536L;
        }
        else if (ud.viewbob) // if (p->over_shoulder_on == 0)
        {
            if (p->over_shoulder_on)
                ud.camera.z += (p->opyoff+mulscale16((int32_t)(p->pyoff-p->opyoff),smoothratio))>>3;
            else ud.camera.z += p->opyoff+mulscale16((int32_t)(p->pyoff-p->opyoff),smoothratio);
        }
        if (p->over_shoulder_on)
        {
            ud.camera.z -= 3072;
            G_DoThirdPerson(p,(vec3_t *)&ud,&ud.camerasect,ud.cameraang,ud.camerahoriz);
        }

        cz = ActorExtra[p->i].ceilingz;
        fz = ActorExtra[p->i].floorz;

        if (g_earthquakeTime > 0 && p->on_ground == 1)
        {
            ud.camera.z += 256-(((g_earthquakeTime)&1)<<9);
            ud.cameraang += (2-((g_earthquakeTime)&2))<<2;
        }

        if (sprite[p->i].pal == 1) ud.camera.z -= (18<<8);

        if (p->newowner >= 0)
            ud.camerahoriz = 100+sprite[p->newowner].shade;
        else if (p->spritebridge == 0)
        {
            if (ud.camera.z < (p->truecz + (4<<8))) ud.camera.z = cz + (4<<8);
            else if (ud.camera.z > (p->truefz - (4<<8))) ud.camera.z = fz - (4<<8);
        }

        if (ud.camerasect >= 0)
        {
            getzsofslope(ud.camerasect,ud.camera.x,ud.camera.y,&cz,&fz);
            if (ud.camera.z < cz+(4<<8)) ud.camera.z = cz+(4<<8);
            if (ud.camera.z > fz-(4<<8)) ud.camera.z = fz-(4<<8);
        }

        if (ud.camerahoriz > HORIZ_MAX) ud.camerahoriz = HORIZ_MAX;
        else if (ud.camerahoriz < HORIZ_MIN) ud.camerahoriz = HORIZ_MIN;

        if (apScriptGameEvent[EVENT_DISPLAYROOMS])
            X_OnEvent(EVENT_DISPLAYROOMS, g_player[screenpeek].ps->i, screenpeek, -1);

        if (((gotpic[MIRROR>>3]&(1<<(MIRROR&7))) > 0)
#if defined(POLYMOST) && defined(USE_OPENGL)
                && (getrendermode() != 4)
#endif
           )
        {
            dst = 0x7fffffff;
            i = 0;
            for (k=g_mirrorCount-1; k>=0; k--)
            {
                j = klabs(wall[g_mirrorWall[k]].x-ud.camera.x);
                j += klabs(wall[g_mirrorWall[k]].y-ud.camera.y);
                if (j < dst) dst = j, i = k;
            }

            if (wall[g_mirrorWall[i]].overpicnum == MIRROR)
            {
                preparemirror(ud.camera.x,ud.camera.y,ud.camera.z,ud.cameraang,ud.camerahoriz,g_mirrorWall[i],g_mirrorSector[i],&tposx,&tposy,&tang);

                j = visibility;
                visibility = (j>>1) + (j>>2);

                drawrooms(tposx,tposy,ud.camera.z,tang,ud.camerahoriz,g_mirrorSector[i]+MAXSECTORS);

                display_mirror = 1;
                G_DoSpriteAnimations(tposx,tposy,tang,smoothratio);
                display_mirror = 0;

                drawmasks();
                completemirror();   //Reverse screen x-wise in this function
                visibility = j;
            }
            gotpic[MIRROR>>3] &= ~(1<<(MIRROR&7));
        }

#ifdef POLYMER
        if (getrendermode() == 4)
        {
            polymer_setanimatesprites(G_DoSpriteAnimations, ud.camera.x,ud.camera.y,ud.cameraang,smoothratio);
        }
#endif
        G_SE40(smoothratio);

        drawrooms(ud.camera.x,ud.camera.y,ud.camera.z,ud.cameraang,ud.camerahoriz,ud.camerasect);

        // dupe the sprites touching the portal to the other sector

        if (ror_sprite != -1)
        {
            spritetype *sp = &sprite[ror_sprite];

            // viewing from bottom
            if (drawing_ror == 1)
            {
                int32_t k = headspritesect[sp->sectnum];

                while (k != -1)
                {
                    if (sprite[k].picnum != SECTOREFFECTOR && (sprite[k].z >= sp->z))
                    {
                        Bmemcpy((spritetype *)&tsprite[spritesortcnt],(spritetype *)&sprite[k],sizeof(spritetype));

                        tsprite[spritesortcnt].x += (sprite[sp->yvel].x-sp->x);
                        tsprite[spritesortcnt].y += (sprite[sp->yvel].y-sp->y);
                        tsprite[spritesortcnt].z = tsprite[spritesortcnt].z - sp->z + ActorExtra[sp->yvel].ceilingz;
                        tsprite[spritesortcnt].sectnum = sprite[sp->yvel].sectnum;
                        tsprite[spritesortcnt].owner = k;

                        //OSD_Printf("duped sprite of pic %d at %d %d %d\n",tsprite[spritesortcnt].picnum,tsprite[spritesortcnt].x,tsprite[spritesortcnt].y,tsprite[spritesortcnt].z);
                        spritesortcnt++;
                    }
                    k = nextspritesect[k];
                }
            }
        }

        G_DoSpriteAnimations(ud.camera.x,ud.camera.y,ud.cameraang,smoothratio);
        drawing_ror = 0;
        drawmasks();

        if (g_screenCapture == 1)
        {
            setviewback();
            g_screenCapture = 0;
            //            walock[TILE_SAVESHOT] = 1;
        }
        else if (getrendermode() == 0 && ((ud.screen_tilting && p->rotscrnang) || ud.detail==0))
        {
            if (ud.screen_tilting) tang = p->rotscrnang;
            else tang = 0;

            if (getrendermode() == 0)
            {
                setviewback();
                picanm[TILE_TILT] &= 0xff0000ff;
                i = (tang&511);
                if (i > 256) i = 512-i;
                i = sintable[i+512]*8 + sintable[i]*5L;
                if ((1-ud.detail) == 0) i >>= 1;
                i>>=(tiltcs-1); // JBF 20030807
                rotatesprite(160<<16,100<<16,i,tang+512,TILE_TILT,0,0,4+2+64,windowx1,windowy1,windowx2,windowy2);
                walock[TILE_TILT] = 199;
            }
        }
    }

    G_RestoreInterpolations();

    if (totalclock < lastvisinc)
    {
        if (klabs(p->visibility-ud.const_visibility) > 8)
            p->visibility += (ud.const_visibility-p->visibility)>>2;
    }
    else p->visibility = ud.const_visibility;
}

static void G_DumpDebugInfo(void)
{
    int32_t i,j,x;
    //    FILE * fp=fopen("condebug.log","w");

    OSD_Printf("Current gamevar values:\n");
    for (i=0; i<MAX_WEAPONS; i++)
    {
        for (j=0; j<numplayers; j++)
        {
            OSD_Printf("Player %d\n\n",j);
            OSD_Printf("WEAPON%d_CLIP %" PRIdPTR "\n",i,aplWeaponClip[i][j]);
            OSD_Printf("WEAPON%d_RELOAD %" PRIdPTR "\n",i,aplWeaponReload[i][j]);
            OSD_Printf("WEAPON%d_FIREDELAY %" PRIdPTR "\n",i,aplWeaponFireDelay[i][j]);
            OSD_Printf("WEAPON%d_TOTALTIME %" PRIdPTR "\n",i,aplWeaponTotalTime[i][j]);
            OSD_Printf("WEAPON%d_HOLDDELAY %" PRIdPTR "\n",i,aplWeaponHoldDelay[i][j]);
            OSD_Printf("WEAPON%d_FLAGS %" PRIdPTR "\n",i,aplWeaponFlags[i][j]);
            OSD_Printf("WEAPON%d_SHOOTS %" PRIdPTR "\n",i,aplWeaponShoots[i][j]);
            OSD_Printf("WEAPON%d_SPAWNTIME %" PRIdPTR "\n",i,aplWeaponSpawnTime[i][j]);
            OSD_Printf("WEAPON%d_SPAWN %" PRIdPTR "\n",i,aplWeaponSpawn[i][j]);
            OSD_Printf("WEAPON%d_SHOTSPERBURST %" PRIdPTR "\n",i,aplWeaponShotsPerBurst[i][j]);
            OSD_Printf("WEAPON%d_WORKSLIKE %" PRIdPTR "\n",i,aplWeaponWorksLike[i][j]);
            OSD_Printf("WEAPON%d_INITIALSOUND %" PRIdPTR "\n",i,aplWeaponInitialSound[i][j]);
            OSD_Printf("WEAPON%d_FIRESOUND %" PRIdPTR "\n",i,aplWeaponFireSound[i][j]);
            OSD_Printf("WEAPON%d_SOUND2TIME %" PRIdPTR "\n",i,aplWeaponSound2Time[i][j]);
            OSD_Printf("WEAPON%d_SOUND2SOUND %" PRIdPTR "\n",i,aplWeaponSound2Sound[i][j]);
            OSD_Printf("WEAPON%d_RELOADSOUND1 %" PRIdPTR "\n",i,aplWeaponReloadSound1[i][j]);
            OSD_Printf("WEAPON%d_RELOADSOUND2 %" PRIdPTR "\n",i,aplWeaponReloadSound2[i][j]);
            OSD_Printf("WEAPON%d_SELECTSOUND %" PRIdPTR "\n",i,aplWeaponSelectSound[i][j]);
            OSD_Printf("WEAPON%d_FLASHCOLOR %" PRIdPTR "\n",i,aplWeaponFlashColor[i][j]);
        }
        OSD_Printf("\n");
    }
    for (x=0; x<MAXSTATUS; x++)
    {
        j = headspritestat[x];
        while (j >= 0)
        {
            OSD_Printf("Sprite %d (%d,%d,%d) (picnum: %d)\n",j,sprite[j].x,sprite[j].y,sprite[j].z,sprite[j].picnum);
            for (i=0; i<g_gameVarCount; i++)
            {
                if (aGameVars[i].dwFlags & (GAMEVAR_PERACTOR))
                {
                    if (aGameVars[i].val.plValues[j] != aGameVars[i].lDefault)
                    {
                        OSD_Printf("gamevar %s ",aGameVars[i].szLabel);
                        OSD_Printf("%" PRIdPTR "",aGameVars[i].val.plValues[j]);
                        OSD_Printf(" GAMEVAR_PERACTOR");
                        if (aGameVars[i].dwFlags != GAMEVAR_PERACTOR)
                        {
                            OSD_Printf(" // ");
                            if (aGameVars[i].dwFlags & (GAMEVAR_SYSTEM))
                            {
                                OSD_Printf(" (system)");
                            }
                        }
                        OSD_Printf("\n");
                    }
                }
            }
            OSD_Printf("\n");
            j = nextspritestat[j];
        }
    }
    Gv_DumpValues();
//    fclose(fp);
    saveboard("debug.map",&g_player[myconnectindex].ps->posx,&g_player[myconnectindex].ps->posy,&g_player[myconnectindex].ps->posz,&g_player[myconnectindex].ps->ang,&g_player[myconnectindex].ps->cursectnum);
}

int32_t A_InsertSprite(int32_t whatsect,int32_t s_x,int32_t s_y,int32_t s_z,int32_t s_pn,int32_t s_s,int32_t s_xr,int32_t s_yr,int32_t s_a,int32_t s_ve,int32_t s_zv,int32_t s_ow,int32_t s_ss)
{
    int32_t p, i = insertsprite(whatsect,s_ss);
    spritetype *s = &sprite[i];
    spritetype spr_temp = { s_x, s_y, s_z, 0, s_pn, s_s, 0, 0, 0, s_xr, s_yr, 0, 0,
        whatsect, s_ss, s_a, s_ow, s_ve, 0, s_zv, 0, 0, 0 };

    if (i < 0)
    {
        G_DumpDebugInfo();
        OSD_Printf("Failed spawning pic %d spr from pic %d spr %d at x:%d,y:%d,z:%d,sect:%d\n",
            s_pn,sprite[s_ow].picnum,s_ow,s_x,s_y,s_z,whatsect);
        G_GameExit("Too many sprites spawned.");
    }

    Bmemcpy(s, &spr_temp, sizeof(spritetype));
    Bmemset(&ActorExtra[i], 0, sizeof(ActorData_t));
    Bmemcpy(&ActorExtra[i].bposx, s, sizeof(vec3_t)); // update bposx/y/z

    ActorExtra[i].projectile = &SpriteProjectile[i];

    if (s_ow > -1 && s_ow < MAXSPRITES)
    {
        ActorExtra[i].picnum = sprite[s_ow].picnum;
        ActorExtra[i].floorz = ActorExtra[s_ow].floorz;
        ActorExtra[i].ceilingz = ActorExtra[s_ow].ceilingz;
    }

    ActorExtra[i].actorstayput = ActorExtra[i].extra = ActorExtra[i].lightId = -1;
    ActorExtra[i].owner = s_ow;

    // sprpos[i].ang = sprpos[i].oldang = sprite[i].ang;

    if (actorscrptr[s_pn])
    {
        s->extra = *actorscrptr[s_pn];
        T5 = *(actorscrptr[s_pn]+1);
        T2 = *(actorscrptr[s_pn]+2);
        s->hitag = *(actorscrptr[s_pn]+3);
    }

    if (show2dsector[SECT>>3]&(1<<(SECT&7))) show2dsprite[i>>3] |= (1<<(i&7));
    else show2dsprite[i>>3] &= ~(1<<(i&7));

    clearbufbyte(&spriteext[i], sizeof(spriteext_t), 0);
    clearbufbyte(&spritesmooth[i], sizeof(spritesmooth_t), 0);

    A_ResetVars(i);

    if (apScriptGameEvent[EVENT_EGS])
    {
        extern int32_t block_deletesprite;
        int32_t pl=A_FindPlayer(s, &p);

        block_deletesprite++;
        X_OnEvent(EVENT_EGS, i, pl, p);
        block_deletesprite--;
    }

    return(i);
}

int32_t A_Spawn(int32_t j, int32_t pn)
{
    int32_t i, s, startwall, endwall, sect, clostest=0;
    int32_t x, y, d, p;
    spritetype *sp;

    if (j >= 0)
    {
        i = A_InsertSprite(sprite[j].sectnum,sprite[j].x,sprite[j].y,sprite[j].z
                           ,pn,0,0,0,0,0,0,j,0);
        ActorExtra[i].picnum = sprite[j].picnum;
    }
    else
    {
        i = pn;

        Bmemset(&ActorExtra[i], 0, sizeof(ActorData_t));
        Bmemcpy(&ActorExtra[i].bposx, &sprite[i], sizeof(vec3_t));

        ActorExtra[i].picnum = PN;

        if (PN == SECTOREFFECTOR && SLT == 50)
            ActorExtra[i].picnum = OW;

        ActorExtra[i].projectile = &SpriteProjectile[i];

        OW = ActorExtra[i].owner = i;

        ActorExtra[i].floorz = sector[SECT].floorz;
        ActorExtra[i].ceilingz = sector[SECT].ceilingz;

        ActorExtra[i].actorstayput = ActorExtra[i].lightId = ActorExtra[i].extra = -1;

//        sprpos[i].ang = sprpos[i].oldang = sprite[i].ang;

        if (PN != SPEAKER && PN != LETTER && PN != DUCK && PN != TARGET && PN != TRIPBOMB && PN != VIEWSCREEN && PN != VIEWSCREEN2 && (CS&48))
            if (!(PN >= CRACK1 && PN <= CRACK4))
            {
                if (SS == 127) return i;
                if (A_CheckSwitchTile(i) == 1 && (CS&16))
                {
                    if (PN != ACCESSSWITCH && PN != ACCESSSWITCH2 && sprite[i].pal)
                    {
                        if ((ud.multimode < 2) || (ud.multimode > 1 && !GTFLAGS(GAMETYPE_DMSWITCHES)))
                        {
                            sprite[i].xrepeat = sprite[i].yrepeat = 0;
                            SLT = SHT = 0;
                            sprite[i].cstat = 32768;
                            return i;
                        }
                    }
                    CS |= 257;
                    if (sprite[i].pal && PN != ACCESSSWITCH && PN != ACCESSSWITCH2)
                        sprite[i].pal = 0;
                    return i;
                }

                if (SHT)
                {
                    changespritestat(i,12);
                    CS |=  257;
                    SH = g_impactDamage;
                    return i;
                }
            }

        s = PN;

        if (CS&1) CS |= 256;

        if (actorscrptr[s])
        {
            SH = *(actorscrptr[s]);
            T5 = *(actorscrptr[s]+1);
            T2 = *(actorscrptr[s]+2);
            if (*(actorscrptr[s]+3) && SHT == 0)
                SHT = *(actorscrptr[s]+3);
        }
        else T2 = T5 = 0;
    }

    sp = &sprite[i];
    sect = sp->sectnum;

    //some special cases that can't be handled through the dynamictostatic system.
    if (((sp->picnum >= BOLT1)&&(sp->picnum <= BOLT1+3))||((sp->picnum >= SIDEBOLT1)&&(sp->picnum <= SIDEBOLT1+3)))
    {
        T1 = sp->xrepeat;
        T2 = sp->yrepeat;
        sp->yvel = 0;
        changespritestat(i,6);
    }
    else if (((sp->picnum >= CAMERA1)&&(sp->picnum <= CAMERA1+3))||(sp->picnum==CAMERAPOLE)||(sp->picnum==GENERICPOLE))
    {
        if (sp->picnum != GENERICPOLE)
        {
            sp->extra = 1;

            if (g_damageCameras) sp->cstat = 257;
            else sp->cstat = 0;
        }
        if (ud.multimode < 2 && sp->pal != 0)
        {
            sp->xrepeat = sp->yrepeat = 0;
            changespritestat(i,5);
        }
        else
        {
            sp->pal = 0;
            if (!(sp->picnum == CAMERAPOLE || sp->picnum == GENERICPOLE))
            {
                sp->picnum = CAMERA1;
                changespritestat(i, STAT_ACTOR);
            }
        }
    }
    else switch (DynamicTileMap[sp->picnum])
        {
        default:
            if (actorscrptr[sp->picnum])
            {
                if (j == -1 && sp->lotag > ud.player_skill)
                {
                    sp->xrepeat=sp->yrepeat=0;
                    changespritestat(i,5);
                    break;
                }

                //  Init the size
                if (sp->xrepeat == 0 || sp->yrepeat == 0)
                    sp->xrepeat = sp->yrepeat = 1;

                if (ActorType[sp->picnum] & 3)
                {
                    if (ud.monsters_off == 1)
                    {
                        sp->xrepeat=sp->yrepeat=0;
                        changespritestat(i,5);
                        break;
                    }

                    A_Fall(i);

                    if (ActorType[sp->picnum] & 2)
                        ActorExtra[i].actorstayput = sp->sectnum;

                    g_player[myconnectindex].ps->max_actors_killed++;
                    sp->clipdist = 80;
                    if (j >= 0)
                    {
                        if (sprite[j].picnum == RESPAWN)
                            ActorExtra[i].tempang = sprite[i].pal = sprite[j].pal;
                        changespritestat(i, STAT_ACTOR);
                    }
                    else changespritestat(i, STAT_ZOMBIEACTOR);
                }
                else
                {
                    sp->clipdist = 40;
                    sp->owner = i;
                    changespritestat(i, STAT_ACTOR);
                }

                ActorExtra[i].timetosleep = 0;

                if (j >= 0)
                    sp->ang = sprite[j].ang;
            }
            break;
        case FOF__STATIC:
            sp->xrepeat = sp->yrepeat = 0;
            changespritestat(i,5);
            break;
        case WATERSPLASH2__STATIC:
            if (j >= 0)
            {
                setsprite(i,(vec3_t *)&sprite[j]);
                sp->xrepeat = sp->yrepeat = 8+(krand()&7);
            }
            else sp->xrepeat = sp->yrepeat = 16+(krand()&15);

            sp->shade = -16;
            sp->cstat |= 128;
            if (j >= 0)
            {
                if (sector[sprite[j].sectnum].lotag == 2)
                {
                    sp->z = getceilzofslope(SECT,SX,SY)+(16<<8);
                    sp->cstat |= 8;
                }
                else if (sector[sprite[j].sectnum].lotag == 1)
                    sp->z = getflorzofslope(SECT,SX,SY);
            }

            if (sector[sect].floorpicnum == FLOORSLIME ||
                    sector[sect].ceilingpicnum == FLOORSLIME)
                sp->pal = 7;
        case NEON1__STATIC:
        case NEON2__STATIC:
        case NEON3__STATIC:
        case NEON4__STATIC:
        case NEON5__STATIC:
        case NEON6__STATIC:
        case DOMELITE__STATIC:
            if (sp->picnum != WATERSPLASH2)
                sp->cstat |= 257;
        case NUKEBUTTON__STATIC:
            if (sp->picnum == DOMELITE)
                sp->cstat |= 257;
        case JIBS1__STATIC:
        case JIBS2__STATIC:
        case JIBS3__STATIC:
        case JIBS4__STATIC:
        case JIBS5__STATIC:
        case JIBS6__STATIC:
        case HEADJIB1__STATIC:
        case ARMJIB1__STATIC:
        case LEGJIB1__STATIC:
        case LIZMANHEAD1__STATIC:
        case LIZMANARM1__STATIC:
        case LIZMANLEG1__STATIC:
        case DUKETORSO__STATIC:
        case DUKEGUN__STATIC:
        case DUKELEG__STATIC:
            changespritestat(i,5);
            break;
        case TONGUE__STATIC:
            if (j >= 0)
                sp->ang = sprite[j].ang;
            sp->z -= 38<<8;
            sp->zvel = 256-(krand()&511);
            sp->xvel = 64-(krand()&127);
            changespritestat(i,4);
            break;
        case NATURALLIGHTNING__STATIC:
            sp->cstat &= ~257;
            sp->cstat |= 32768;
            break;
        case TRANSPORTERSTAR__STATIC:
        case TRANSPORTERBEAM__STATIC:
            if (j == -1) break;
            if (sp->picnum == TRANSPORTERBEAM)
            {
                sp->xrepeat = 31;
                sp->yrepeat = 1;
                sp->z = sector[sprite[j].sectnum].floorz-(40<<8);
            }
            else
            {
                if (sprite[j].statnum == STAT_PROJECTILE)
                    sp->xrepeat = sp->yrepeat = 8;
                else
                {
                    sp->xrepeat = 48;
                    sp->yrepeat = 64;
                    if (sprite[j].statnum == STAT_PLAYER || A_CheckEnemySprite(&sprite[j]))
                        sp->z -= (32<<8);
                }
            }

            sp->shade = -127;
            sp->cstat = 128|2;
            sp->ang = sprite[j].ang;

            sp->xvel = 128;
            changespritestat(i,5);
            A_SetSprite(i,CLIPMASK0);
            setsprite(i,(vec3_t *)sp);
            break;

        case FRAMEEFFECT1_13__STATIC:
            if (PLUTOPAK) break;
        case FRAMEEFFECT1__STATIC:
            if (j >= 0)
            {
                sp->xrepeat = sprite[j].xrepeat;
                sp->yrepeat = sprite[j].yrepeat;
                T2 = sprite[j].picnum;
            }
            else sp->xrepeat = sp->yrepeat = 0;

            changespritestat(i,5);

            break;

        case LASERLINE__STATIC:
            sp->yrepeat = 6;
            sp->xrepeat = 32;

            if (g_tripbombLaserMode == 1)
                sp->cstat = 16 + 2;
            else if (g_tripbombLaserMode == 0 || g_tripbombLaserMode == 2)
                sp->cstat = 16;
            else
            {
                sp->xrepeat = 0;
                sp->yrepeat = 0;
            }

            if (j >= 0) sp->ang = ActorExtra[j].temp_data[5]+512;
            changespritestat(i,5);
            break;

        case FORCESPHERE__STATIC:
            if (j == -1)
            {
                sp->cstat = (int16_t) 32768;
                changespritestat(i, STAT_ZOMBIEACTOR);
            }
            else
            {
                sp->xrepeat = sp->yrepeat = 1;
                changespritestat(i,5);
            }
            break;

        case BLOOD__STATIC:
            sp->xrepeat = sp->yrepeat = 16;
            sp->z -= (26<<8);
            if (j >= 0 && sprite[j].pal == 6)
                sp->pal = 6;
            changespritestat(i,5);
            break;
        case BLOODPOOL__STATIC:
        case PUKE__STATIC:
        {
            int16_t s1;
            s1 = sp->sectnum;

            updatesector(sp->x+108,sp->y+108,&s1);
            if (s1 >= 0 && sector[s1].floorz == sector[sp->sectnum].floorz)
            {
                updatesector(sp->x-108,sp->y-108,&s1);
                if (s1 >= 0 && sector[s1].floorz == sector[sp->sectnum].floorz)
                {
                    updatesector(sp->x+108,sp->y-108,&s1);
                    if (s1 >= 0 && sector[s1].floorz == sector[sp->sectnum].floorz)
                    {
                        updatesector(sp->x-108,sp->y+108,&s1);
                        if (s1 >= 0 && sector[s1].floorz != sector[sp->sectnum].floorz)
                        {
                            sp->xrepeat = sp->yrepeat = 0;
                            changespritestat(i,5);
                            break;
                        }

                    }
                    else
                    {
                        sp->xrepeat = sp->yrepeat = 0;
                        changespritestat(i,5);
                        break;
                    }

                }
                else
                {
                    sp->xrepeat = sp->yrepeat = 0;
                    changespritestat(i,5);
                    break;
                }

            }
            else
            {
                sp->xrepeat = sp->yrepeat = 0;
                changespritestat(i,5);
                break;
            }

        }

        if (sector[SECT].lotag == 1)
        {
            changespritestat(i,5);
            break;
        }

        if (j >= 0 && sp->picnum != PUKE)
        {
            if (sprite[j].pal == 1)
                sp->pal = 1;
            else if (sprite[j].pal != 6 && sprite[j].picnum != NUKEBARREL && sprite[j].picnum != TIRE)
            {
                if (sprite[j].picnum == FECES)
                    sp->pal = 7; // Brown
                else sp->pal = 2; // Red
            }
            else sp->pal = 0;  // green

            if (sprite[j].picnum == TIRE)
                sp->shade = 127;
        }
        sp->cstat |= 32;
        case FECES__STATIC:
            if (j >= 0)
                sp->xrepeat = sp->yrepeat = 1;
            changespritestat(i,5);
            break;

        case BLOODSPLAT1__STATIC:
        case BLOODSPLAT2__STATIC:
        case BLOODSPLAT3__STATIC:
        case BLOODSPLAT4__STATIC:
            sp->cstat |= 16;
            sp->xrepeat = 7+(krand()&7);
            sp->yrepeat = 7+(krand()&7);
            sp->z -= (16<<8);
            if (j >= 0 && sprite[j].pal == 6)
                sp->pal = 6;
            A_AddToDeleteQueue(i);
            changespritestat(i,5);
            break;

        case TRIPBOMB__STATIC:
            if (sp->lotag > ud.player_skill)
            {
                sp->xrepeat=sp->yrepeat=0;
                changespritestat(i,5);
                break;
            }

            sp->xrepeat=4;
            sp->yrepeat=5;

            sp->owner = sp->hitag = i;

            sp->xvel = 16;
            A_SetSprite(i,CLIPMASK0);
            ActorExtra[i].temp_data[0] = 17;
            ActorExtra[i].temp_data[2] = 0;
            ActorExtra[i].temp_data[5] = sp->ang;

        case SPACEMARINE__STATIC:
            if (sp->picnum == SPACEMARINE)
            {
                sp->extra = 20;
                sp->cstat |= 257;
            }
            changespritestat(i, STAT_ZOMBIEACTOR);
            break;

        case HYDRENT__STATIC:
        case PANNEL1__STATIC:
        case PANNEL2__STATIC:
        case SATELITE__STATIC:
        case FUELPOD__STATIC:
        case SOLARPANNEL__STATIC:
        case ANTENNA__STATIC:
        case GRATE1__STATIC:
        case CHAIR1__STATIC:
        case CHAIR2__STATIC:
        case CHAIR3__STATIC:
        case BOTTLE1__STATIC:
        case BOTTLE2__STATIC:
        case BOTTLE3__STATIC:
        case BOTTLE4__STATIC:
        case BOTTLE5__STATIC:
        case BOTTLE6__STATIC:
        case BOTTLE7__STATIC:
        case BOTTLE8__STATIC:
        case BOTTLE10__STATIC:
        case BOTTLE11__STATIC:
        case BOTTLE12__STATIC:
        case BOTTLE13__STATIC:
        case BOTTLE14__STATIC:
        case BOTTLE15__STATIC:
        case BOTTLE16__STATIC:
        case BOTTLE17__STATIC:
        case BOTTLE18__STATIC:
        case BOTTLE19__STATIC:
        case OCEANSPRITE1__STATIC:
        case OCEANSPRITE2__STATIC:
        case OCEANSPRITE3__STATIC:
        case OCEANSPRITE5__STATIC:
        case MONK__STATIC:
        case INDY__STATIC:
        case LUKE__STATIC:
        case JURYGUY__STATIC:
        case SCALE__STATIC:
        case VACUUM__STATIC:
        case FANSPRITE__STATIC:
        case CACTUS__STATIC:
        case CACTUSBROKE__STATIC:
        case HANGLIGHT__STATIC:
        case FETUS__STATIC:
        case FETUSBROKE__STATIC:
        case CAMERALIGHT__STATIC:
        case MOVIECAMERA__STATIC:
        case IVUNIT__STATIC:
        case POT1__STATIC:
        case POT2__STATIC:
        case POT3__STATIC:
        case TRIPODCAMERA__STATIC:
        case SUSHIPLATE1__STATIC:
        case SUSHIPLATE2__STATIC:
        case SUSHIPLATE3__STATIC:
        case SUSHIPLATE4__STATIC:
        case SUSHIPLATE5__STATIC:
        case WAITTOBESEATED__STATIC:
        case VASE__STATIC:
        case PIPE1__STATIC:
        case PIPE2__STATIC:
        case PIPE3__STATIC:
        case PIPE4__STATIC:
        case PIPE5__STATIC:
        case PIPE6__STATIC:
            sp->clipdist = 32;
            sp->cstat |= 257;
        case OCEANSPRITE4__STATIC:
            changespritestat(i,0);
            break;
        case FEMMAG1__STATIC:
        case FEMMAG2__STATIC:
            sp->cstat &= ~257;
            changespritestat(i,0);
            break;
        case DUKETAG__STATIC:
        case SIGN1__STATIC:
        case SIGN2__STATIC:
            if (ud.multimode < 2 && sp->pal)
            {
                sp->xrepeat = sp->yrepeat = 0;
                changespritestat(i,5);
            }
            else sp->pal = 0;
            break;
        case MASKWALL1__STATIC:
        case MASKWALL2__STATIC:
        case MASKWALL3__STATIC:
        case MASKWALL4__STATIC:
        case MASKWALL5__STATIC:
        case MASKWALL6__STATIC:
        case MASKWALL7__STATIC:
        case MASKWALL8__STATIC:
        case MASKWALL9__STATIC:
        case MASKWALL10__STATIC:
        case MASKWALL11__STATIC:
        case MASKWALL12__STATIC:
        case MASKWALL13__STATIC:
        case MASKWALL14__STATIC:
        case MASKWALL15__STATIC:
            j = sp->cstat&60;
            sp->cstat = j|1;
            changespritestat(i,0);
            break;
        case FOOTPRINTS__STATIC:
        case FOOTPRINTS2__STATIC:
        case FOOTPRINTS3__STATIC:
        case FOOTPRINTS4__STATIC:
            if (j >= 0)
            {
                int16_t s1;
                s1 = sp->sectnum;

                updatesector(sp->x+84,sp->y+84,&s1);
                if (s1 >= 0 && sector[s1].floorz == sector[sp->sectnum].floorz)
                {
                    updatesector(sp->x-84,sp->y-84,&s1);
                    if (s1 >= 0 && sector[s1].floorz == sector[sp->sectnum].floorz)
                    {
                        updatesector(sp->x+84,sp->y-84,&s1);
                        if (s1 >= 0 && sector[s1].floorz == sector[sp->sectnum].floorz)
                        {
                            updatesector(sp->x-84,sp->y+84,&s1);
                            if (s1 >= 0 && sector[s1].floorz != sector[sp->sectnum].floorz)
                            {
                                sp->xrepeat = sp->yrepeat = 0;
                                changespritestat(i,5);
                                break;
                            }

                        }
                        else
                        {
                            sp->xrepeat = sp->yrepeat = 0;
                            break;
                        }

                    }
                    else
                    {
                        sp->xrepeat = sp->yrepeat = 0;
                        break;
                    }

                }
                else
                {
                    sp->xrepeat = sp->yrepeat = 0;
                    break;
                }

                sp->cstat = 32+((g_player[sprite[j].yvel].ps->footprintcount&1)<<2);
                sp->ang = sprite[j].ang;
            }

            sp->z = sector[sect].floorz;
            if (sector[sect].lotag != 1 && sector[sect].lotag != 2)
                sp->xrepeat = sp->yrepeat = 32;

            A_AddToDeleteQueue(i);
            changespritestat(i,5);
            break;

        case FEM1__STATIC:
        case FEM2__STATIC:
        case FEM3__STATIC:
        case FEM4__STATIC:
        case FEM5__STATIC:
        case FEM6__STATIC:
        case FEM7__STATIC:
        case FEM8__STATIC:
        case FEM9__STATIC:
        case FEM10__STATIC:
        case PODFEM1__STATIC:
        case NAKED1__STATIC:
        case STATUE__STATIC:
        case TOUGHGAL__STATIC:
            sp->yvel = sp->hitag;
            sp->hitag = -1;
            if (sp->picnum == PODFEM1) sp->extra <<= 1;
        case BLOODYPOLE__STATIC:

        case QUEBALL__STATIC:
        case STRIPEBALL__STATIC:

            if (sp->picnum == QUEBALL || sp->picnum == STRIPEBALL)
            {
                sp->cstat = 256;
                sp->clipdist = 8;
            }
            else
            {
                sp->cstat |= 257;
                sp->clipdist = 32;
            }

            changespritestat(i, STAT_ZOMBIEACTOR);
            break;

        case DUKELYINGDEAD__STATIC:
            if (j >= 0 && sprite[j].picnum == APLAYER)
            {
                sp->xrepeat = sprite[j].xrepeat;
                sp->yrepeat = sprite[j].yrepeat;
                sp->shade = sprite[j].shade;
                sp->pal = g_player[sprite[j].yvel].ps->palookup;
            }
        case DUKECAR__STATIC:
        case HELECOPT__STATIC:
            //                if(sp->picnum == HELECOPT || sp->picnum == DUKECAR) sp->xvel = 1024;
            sp->cstat = 0;
            sp->extra = 1;
            sp->xvel = 292;
            sp->zvel = 360;
        case RESPAWNMARKERRED__STATIC:
        case BLIMP__STATIC:

            if (sp->picnum == RESPAWNMARKERRED)
            {
                sp->xrepeat = sp->yrepeat = 24;
                if (j >= 0) sp->z = ActorExtra[j].floorz; // -(1<<4);
            }
            else
            {
                sp->cstat |= 257;
                sp->clipdist = 128;
            }
        case MIKE__STATIC:
            if (sp->picnum == MIKE)
            {
                sp->yvel = sp->hitag;
                sp->hitag = 0;
            }
        case WEATHERWARN__STATIC:
            changespritestat(i, STAT_ACTOR);
            break;

        case SPOTLITE__STATIC:
            T1 = sp->x;
            T2 = sp->y;
            break;
        case BULLETHOLE__STATIC:
            sp->xrepeat = sp->yrepeat = 3;
            sp->cstat = 16+(krand()&12);
            A_AddToDeleteQueue(i);
        case MONEY__STATIC:
        case MAIL__STATIC:
        case PAPER__STATIC:
            if (sp->picnum == MONEY || sp->picnum == MAIL || sp->picnum == PAPER)
            {
                ActorExtra[i].temp_data[0] = krand()&2047;
                sp->cstat = krand()&12;
                sp->xrepeat = sp->yrepeat = 8;
                sp->ang = krand()&2047;
            }
            changespritestat(i,5);
            break;

        case VIEWSCREEN__STATIC:
        case VIEWSCREEN2__STATIC:
            sp->owner = i;
            sp->lotag = sp->extra = 1;
            changespritestat(i,6);
            break;

        case SHELL__STATIC: //From the player
        case SHOTGUNSHELL__STATIC:
            if (j >= 0)
            {
                int32_t snum,a;

                if (sprite[j].picnum == APLAYER)
                {
                    snum = sprite[j].yvel;
                    a = g_player[snum].ps->ang-(krand()&63)+8;  //Fine tune

                    T1 = krand()&1;
                    sp->z = (3<<8)+g_player[snum].ps->pyoff+g_player[snum].ps->posz-
                            ((g_player[snum].ps->horizoff+g_player[snum].ps->horiz-100)<<4);
                    if (sp->picnum == SHOTGUNSHELL)
                        sp->z += (3<<8);
                    sp->zvel = -(krand()&255);
                }
                else
                {
                    a = sp->ang;
                    sp->z = sprite[j].z-PHEIGHT+(3<<8);
                }

                sp->x = sprite[j].x+(sintable[(a+512)&2047]>>7);
                sp->y = sprite[j].y+(sintable[a&2047]>>7);

                sp->shade = -8;

                if (sp->yvel == 1 || NAM)
                {
                    sp->ang = a+512;
                    sp->xvel = 30;
                }
                else
                {
                    sp->ang = a-512;
                    sp->xvel = 20;
                }
                sp->xrepeat=sp->yrepeat=4;

                changespritestat(i,5);
            }
            break;

        case RESPAWN__STATIC:
            sp->extra = 66-13;
        case MUSICANDSFX__STATIC:
            if (ud.multimode < 2 && sp->pal == 1)
            {
                sp->xrepeat = sp->yrepeat = 0;
                changespritestat(i,5);
                break;
            }
            sp->cstat = (int16_t)32768;
            changespritestat(i,11);
            break;

        case EXPLOSION2__STATIC:
            if (sp->yrepeat > 32)
            {
                G_AddGameLight(0, i, ((sp->yrepeat*tilesizy[sp->picnum])<<1), 32768, 255+(95<<8),PR_LIGHT_PRIO_MAX_GAME);
                ActorExtra[i].lightcount = 2;
            }
        case EXPLOSION2BOT__STATIC:
        case BURNING__STATIC:
        case BURNING2__STATIC:
        case SMALLSMOKE__STATIC:
        case SHRINKEREXPLOSION__STATIC:
        case COOLEXPLOSION1__STATIC:

            if (j >= 0)
            {
                sp->ang = sprite[j].ang;
                sp->shade = -64;
                sp->cstat = 128|(krand()&4);
            }

            if (sp->picnum == EXPLOSION2 || sp->picnum == EXPLOSION2BOT)
            {
                sp->xrepeat = sp->yrepeat = 48;
                sp->shade = -127;
                sp->cstat |= 128;
            }
            else if (sp->picnum == SHRINKEREXPLOSION)
                sp->xrepeat = sp->yrepeat = 32;
            else if (sp->picnum == SMALLSMOKE)
            {
                // 64 "money"
                sp->xrepeat = sp->yrepeat = 24;
            }
            else if (sp->picnum == BURNING || sp->picnum == BURNING2)
                sp->xrepeat = sp->yrepeat = 4;

            sp->cstat |= 8192;

            if (j >= 0)
            {
                x = getflorzofslope(sp->sectnum,sp->x,sp->y);
                if (sp->z > x-(12<<8))
                    sp->z = x-(12<<8);
            }

            changespritestat(i,5);

            break;

        case PLAYERONWATER__STATIC:
            if (j >= 0)
            {
                sp->xrepeat = sprite[j].xrepeat;
                sp->yrepeat = sprite[j].yrepeat;
                sp->zvel = 128;
                if (sector[sp->sectnum].lotag != 2)
                    sp->cstat |= 32768;
            }
            changespritestat(i,13);
            break;

        case APLAYER__STATIC:
            sp->xrepeat = sp->yrepeat = 0;
            //j = ud.coop;
            //if(j == 2) j = 0;
            j=(GametypeFlags[ud.coop] & GAMETYPE_COOPSPAWN) / GAMETYPE_COOPSPAWN ;
            if (ud.multimode < 2 || (ud.multimode > 1 && j != sp->lotag))
                changespritestat(i,5);
            else
                changespritestat(i,10);
            break;
        case WATERBUBBLE__STATIC:
            if (j >= 0 && sprite[j].picnum == APLAYER)
                sp->z -= (16<<8);
            if (sp->picnum == WATERBUBBLE)
            {
                if (j >= 0)
                    sp->ang = sprite[j].ang;
                sp->xrepeat = sp->yrepeat = 4;
            }
            else sp->xrepeat = sp->yrepeat = 32;

            changespritestat(i,5);
            break;

        case CRANE__STATIC:

            sp->cstat |= 64|257;

            sp->picnum += 2;
            sp->z = sector[sect].ceilingz+(48<<8);
            T5 = tempwallptr;

            msx[tempwallptr] = sp->x;
            msy[tempwallptr] = sp->y;
            msx[tempwallptr+2] = sp->z;

            s = headspritestat[STAT_DEFAULT];
            while (s >= 0)
            {
                if (sprite[s].picnum == CRANEPOLE && SHT == (sprite[s].hitag))
                {
                    msy[tempwallptr+2] = s;

                    T2 = sprite[s].sectnum;

                    sprite[s].xrepeat = 48;
                    sprite[s].yrepeat = 128;

                    msx[tempwallptr+1] = sprite[s].x;
                    msy[tempwallptr+1] = sprite[s].y;

                    sprite[s].x = sp->x;
                    sprite[s].y = sp->y;
                    sprite[s].z = sp->z;
                    sprite[s].shade = sp->shade;

                    setsprite(s,(vec3_t *)&sprite[s]);
                    break;
                }
                s = nextspritestat[s];
            }

            tempwallptr += 3;
            sp->owner = -1;
            sp->extra = 8;
            changespritestat(i,6);
            break;

        case WATERDRIP__STATIC:
            if (j >= 0 && (sprite[j].statnum == STAT_PLAYER || sprite[j].statnum == STAT_ACTOR))
            {
                sp->shade = 32;
                if (sprite[j].pal != 1)
                {
                    sp->pal = 2;
                    sp->z -= (18<<8);
                }
                else sp->z -= (13<<8);
                sp->ang = getangle(g_player[connecthead].ps->posx-sp->x,g_player[connecthead].ps->posy-sp->y);
                sp->xvel = 48-(krand()&31);
                A_SetSprite(i,CLIPMASK0);
            }
            else if (j == -1)
            {
                sp->z += (4<<8);
                T1 = sp->z;
                T2 = krand()&127;
            }
        case TRASH__STATIC:

            if (sp->picnum != WATERDRIP)
                sp->ang = krand()&2047;

        case WATERDRIPSPLASH__STATIC:
            sp->xrepeat = sp->yrepeat = 24;
            changespritestat(i,6);
            break;

        case PLUG__STATIC:
            sp->lotag = 9999;
            changespritestat(i,6);
            break;
        case TOUCHPLATE__STATIC:
            T3 = sector[sect].floorz;
            if (sector[sect].lotag != 1 && sector[sect].lotag != 2)
                sector[sect].floorz = sp->z;
            if (sp->pal && ud.multimode > 1)
            {
                sp->xrepeat=sp->yrepeat=0;
                changespritestat(i,5);
                break;
            }
        case WATERBUBBLEMAKER__STATIC:
            if (sp->hitag && sp->picnum == WATERBUBBLEMAKER)
            {
                // JBF 20030913: Pisses off X_Move(), eg. in bobsp2
                OSD_Printf(OSD_ERROR "WARNING: WATERBUBBLEMAKER %d @ %d,%d with hitag!=0. Applying fixup.\n",
                           i,sp->x,sp->y);
                sp->hitag = 0;
            }
            sp->cstat |= 32768;
            changespritestat(i,6);
            break;
            //case BOLT1:
            //case BOLT1+1:
            //case BOLT1+2:
            //case BOLT1+3:
            //case SIDEBOLT1:
            //case SIDEBOLT1+1:
            //case SIDEBOLT1+2:
            //case SIDEBOLT1+3:
            //    T1 = sp->xrepeat;
            //    T2 = sp->yrepeat;
        case MASTERSWITCH__STATIC:
            if (sp->picnum == MASTERSWITCH)
                sp->cstat |= 32768;
            sp->yvel = 0;
            changespritestat(i,6);
            break;
        case TARGET__STATIC:
        case DUCK__STATIC:
        case LETTER__STATIC:
            sp->extra = 1;
            sp->cstat |= 257;
            changespritestat(i, STAT_ACTOR);
            break;
        case OCTABRAINSTAYPUT__STATIC:
        case LIZTROOPSTAYPUT__STATIC:
        case PIGCOPSTAYPUT__STATIC:
        case LIZMANSTAYPUT__STATIC:
        case BOSS1STAYPUT__STATIC:
        case PIGCOPDIVE__STATIC:
        case COMMANDERSTAYPUT__STATIC:
        case BOSS4STAYPUT__STATIC:
            ActorExtra[i].actorstayput = sp->sectnum;
        case BOSS1__STATIC:
        case BOSS2__STATIC:
        case BOSS3__STATIC:
        case BOSS4__STATIC:
        case ROTATEGUN__STATIC:
        case GREENSLIME__STATIC:
            if (sp->picnum == GREENSLIME)
                sp->extra = 1;
        case DRONE__STATIC:
        case LIZTROOPONTOILET__STATIC:
        case LIZTROOPJUSTSIT__STATIC:
        case LIZTROOPSHOOT__STATIC:
        case LIZTROOPJETPACK__STATIC:
        case LIZTROOPDUCKING__STATIC:
        case LIZTROOPRUNNING__STATIC:
        case LIZTROOP__STATIC:
        case OCTABRAIN__STATIC:
        case COMMANDER__STATIC:
        case PIGCOP__STATIC:
        case LIZMAN__STATIC:
        case LIZMANSPITTING__STATIC:
        case LIZMANFEEDING__STATIC:
        case LIZMANJUMP__STATIC:
        case ORGANTIC__STATIC:
        case RAT__STATIC:
        case SHARK__STATIC:

            if (sp->pal == 0)
            {
                switch (DynamicTileMap[sp->picnum])
                {
                case LIZTROOPONTOILET__STATIC:
                case LIZTROOPSHOOT__STATIC:
                case LIZTROOPJETPACK__STATIC:
                case LIZTROOPDUCKING__STATIC:
                case LIZTROOPRUNNING__STATIC:
                case LIZTROOPSTAYPUT__STATIC:
                case LIZTROOPJUSTSIT__STATIC:
                case LIZTROOP__STATIC:
                    sp->pal = 22;
                    break;
                }
            }

            if (sp->picnum == BOSS4STAYPUT || sp->picnum == BOSS1 || sp->picnum == BOSS2 || sp->picnum == BOSS1STAYPUT || sp->picnum == BOSS3 || sp->picnum == BOSS4)
            {
                if (j >= 0 && sprite[j].picnum == RESPAWN)
                    sp->pal = sprite[j].pal;
                if (sp->pal)
                {
                    sp->clipdist = 80;
                    sp->xrepeat = sp->yrepeat = 40;
                }
                else
                {
                    sp->xrepeat = sp->yrepeat = 80;
                    sp->clipdist = 164;
                }
            }
            else
            {
                if (sp->picnum != SHARK)
                {
                    sp->xrepeat = sp->yrepeat = 40;
                    sp->clipdist = 80;
                }
                else
                {
                    sp->xrepeat = sp->yrepeat = 60;
                    sp->clipdist = 40;
                }
            }

            if (j >= 0) sp->lotag = 0;

            if ((sp->lotag > ud.player_skill) || ud.monsters_off == 1)
            {
                sp->xrepeat=sp->yrepeat=0;
                changespritestat(i,5);
                break;
            }
            else
            {
                A_Fall(i);

                if (sp->picnum == RAT)
                {
                    sp->ang = krand()&2047;
                    sp->xrepeat = sp->yrepeat = 48;
                    sp->cstat = 0;
                }
                else
                {
                    sp->cstat |= 257;

                    if (sp->picnum != SHARK)
                        g_player[myconnectindex].ps->max_actors_killed++;
                }

                if (sp->picnum == ORGANTIC) sp->cstat |= 128;

                if (j >= 0)
                {
                    ActorExtra[i].timetosleep = 0;
                    A_PlayAlertSound(i);
                    changespritestat(i, STAT_ACTOR);
                }
                else changespritestat(i, STAT_ZOMBIEACTOR);
            }

            if (sp->picnum == ROTATEGUN)
                sp->zvel = 0;

            break;

        case LOCATORS__STATIC:
            sp->cstat |= 32768;
            changespritestat(i,7);
            break;

        case ACTIVATORLOCKED__STATIC:
        case ACTIVATOR__STATIC:
            sp->cstat = (int16_t) 32768;
            if (sp->picnum == ACTIVATORLOCKED)
                sector[sp->sectnum].lotag |= 16384;
            changespritestat(i,8);
            break;

        case DOORSHOCK__STATIC:
            sp->cstat |= 1+256;
            sp->shade = -12;
            changespritestat(i,6);
            break;

        case OOZ__STATIC:
        case OOZ2__STATIC:
            sp->shade = -12;

            if (j >= 0)
            {
                if (sprite[j].picnum == NUKEBARREL)
                    sp->pal = 8;
                A_AddToDeleteQueue(i);
            }

            changespritestat(i, STAT_ACTOR);

            A_GetZLimits(i);

            j = (ActorExtra[i].floorz-ActorExtra[i].ceilingz)>>9;

            sp->yrepeat = j;
            sp->xrepeat = 25-(j>>1);
            sp->cstat |= (krand()&4);

            break;

        case HEAVYHBOMB__STATIC:
            if (j >= 0)
                sp->owner = j;
            else sp->owner = i;
            sp->xrepeat = sp->yrepeat = 9;
            sp->yvel = 4;
        case REACTOR2__STATIC:
        case REACTOR__STATIC:
        case RECON__STATIC:

            if (sp->picnum == RECON)
            {
                if (sp->lotag > ud.player_skill)
                {
                    sp->xrepeat = sp->yrepeat = 0;
                    changespritestat(i,5);
                    return i;
                }
                g_player[myconnectindex].ps->max_actors_killed++;
                ActorExtra[i].temp_data[5] = 0;
                if (ud.monsters_off == 1)
                {
                    sp->xrepeat = sp->yrepeat = 0;
                    changespritestat(i,5);
                    break;
                }
                sp->extra = 130;
                CS |= 256; // Make it hitable
            }
            else CS |= 257;

            if (sp->picnum == REACTOR || sp->picnum == REACTOR2)
                sp->extra = g_impactDamage;

            if (ud.multimode < 2 && sp->pal != 0)
            {
                sp->xrepeat = sp->yrepeat = 0;
                changespritestat(i,5);
                break;
            }
            sp->pal = 0;
            SS = -17;

            changespritestat(i, STAT_ZOMBIEACTOR);
            break;

        case ATOMICHEALTH__STATIC:
        case STEROIDS__STATIC:
        case HEATSENSOR__STATIC:
        case SHIELD__STATIC:
        case AIRTANK__STATIC:
        case TRIPBOMBSPRITE__STATIC:
        case JETPACK__STATIC:
        case HOLODUKE__STATIC:

        case FIRSTGUNSPRITE__STATIC:
        case CHAINGUNSPRITE__STATIC:
        case SHOTGUNSPRITE__STATIC:
        case RPGSPRITE__STATIC:
        case SHRINKERSPRITE__STATIC:
        case FREEZESPRITE__STATIC:
        case DEVISTATORSPRITE__STATIC:

        case SHOTGUNAMMO__STATIC:
        case FREEZEAMMO__STATIC:
        case HBOMBAMMO__STATIC:
        case CRYSTALAMMO__STATIC:
        case GROWAMMO__STATIC:
        case BATTERYAMMO__STATIC:
        case DEVISTATORAMMO__STATIC:
        case RPGAMMO__STATIC:
        case BOOTS__STATIC:
        case AMMO__STATIC:
        case AMMOLOTS__STATIC:
        case COLA__STATIC:
        case FIRSTAID__STATIC:
        case SIXPAK__STATIC:

            if (j >= 0)
            {
                sp->lotag = 0;
                sp->z -= (32<<8);
                sp->zvel = -1024;
                A_SetSprite(i,CLIPMASK0);
                sp->cstat = krand()&4;
            }
            else
            {
                sp->owner = i;
                sp->cstat = 0;
            }

            if ((ud.multimode < 2 && sp->pal != 0) || (sp->lotag > ud.player_skill))
            {
                sp->xrepeat = sp->yrepeat = 0;
                changespritestat(i,5);
                break;
            }

            sp->pal = 0;

        case ACCESSCARD__STATIC:

            if (sp->picnum == ATOMICHEALTH)
                sp->cstat |= 128;

            if (ud.multimode > 1 && !GTFLAGS(GAMETYPE_ACCESSCARDSPRITES) && sp->picnum == ACCESSCARD)
            {
                sp->xrepeat = sp->yrepeat = 0;
                changespritestat(i,5);
                break;
            }
            else
            {
                if (sp->picnum == AMMO)
                    sp->xrepeat = sp->yrepeat = 16;
                else sp->xrepeat = sp->yrepeat = 32;
            }

            sp->shade = -17;

            if (j >= 0) changespritestat(i, STAT_ACTOR);
            else
            {
                changespritestat(i, STAT_ZOMBIEACTOR);
                A_Fall(i);
            }
            break;

        case WATERFOUNTAIN__STATIC:
            SLT = 1;

        case TREE1__STATIC:
        case TREE2__STATIC:
        case TIRE__STATIC:
        case CONE__STATIC:
        case BOX__STATIC:
            CS = 257; // Make it hitable
            sprite[i].extra = 1;
            changespritestat(i,6);
            break;

        case FLOORFLAME__STATIC:
            sp->shade = -127;
            changespritestat(i,6);
            break;

        case BOUNCEMINE__STATIC:
            sp->owner = i;
            sp->cstat |= 1+256; //Make it hitable
            sp->xrepeat = sp->yrepeat = 24;
            sp->shade = -127;
            sp->extra = g_impactDamage<<2;
            changespritestat(i, STAT_ZOMBIEACTOR);
            break;

        case STEAM__STATIC:
            if (j >= 0)
            {
                sp->ang = sprite[j].ang;
                sp->cstat = 16+128+2;
                sp->xrepeat=sp->yrepeat=1;
                sp->xvel = -8;
                A_SetSprite(i,CLIPMASK0);
            }
        case CEILINGSTEAM__STATIC:
            changespritestat(i,6);
            break;

        case SECTOREFFECTOR__STATIC:
            sp->cstat |= 32768;
            sp->xrepeat = sp->yrepeat = 0;

            switch (sp->lotag)
            {
            case 40:
            case 41:
                sp->cstat = 32;
                sp->xrepeat = sp->yrepeat = 64;
                changespritestat(i, STAT_EFFECTOR);
                for (j=0; j < MAXSPRITES; j++)
                    if (sprite[j].picnum == SECTOREFFECTOR && (sprite[j].lotag == 40 || sprite[j].lotag == 41) &&
                            sprite[j].hitag == sp->hitag && i != j)
                    {
//                        initprintf("found ror match\n");
                        sp->yvel = j;
                        break;
                    }
                goto SPAWN_END;
                break;
            case 46:
                ror_protectedsectors[sp->sectnum] = 1;
            case 49:
            case 50:
            {
                int32_t j, nextj;

                TRAVERSE_SPRITE_SECT(headspritesect[sp->sectnum], j, nextj)
                if (sprite[j].picnum == ACTIVATOR || sprite[j].picnum == ACTIVATORLOCKED)
                    ActorExtra[i].flags |= SPRITE_USEACTIVATOR;
            }
            changespritestat(i, STAT_EFFECTOR);
            goto SPAWN_END;
            break;
            }

            sp->yvel = sector[sect].extra;

            switch (sp->lotag)
            {
            case 28:
                T6 = 65;// Delay for lightning
                break;
            case 7: // Transporters!!!!
            case 23:// XPTR END
                if (sp->lotag != 23)
                {
                    for (j=0; j<MAXSPRITES; j++)
                        if (sprite[j].statnum < MAXSTATUS && sprite[j].picnum == SECTOREFFECTOR && (sprite[j].lotag == 7 || sprite[j].lotag == 23) && i != j && sprite[j].hitag == SHT)
                        {
                            OW = j;
                            break;
                        }
                }
                else OW = i;

                T5 = sector[sect].floorz == SZ;
                sp->cstat = 0;
                changespritestat(i,9);
                return i;
            case 1:
                sp->owner = -1;
                T1 = 1;
                break;
            case 18:

                if (sp->ang == 512)
                {
                    T2 = sector[sect].ceilingz;
                    if (sp->pal)
                        sector[sect].ceilingz = sp->z;
                }
                else
                {
                    T2 = sector[sect].floorz;
                    if (sp->pal)
                        sector[sect].floorz = sp->z;
                }

                sp->hitag <<= 2;
                break;

            case 19:
                sp->owner = -1;
                break;
            case 25: // Pistons
                T4 = sector[sect].ceilingz;
                T5 = 1;
                sector[sect].ceilingz = sp->z;
                G_SetInterpolation(&sector[sect].ceilingz);
                break;
            case 35:
                sector[sect].ceilingz = sp->z;
                break;
            case 27:
                if (ud.recstat == 1)
                {
                    sp->xrepeat=sp->yrepeat=64;
                    sp->cstat &= 32768;
                }
                break;
            case 12:

                T2 = sector[sect].floorshade;
                T3 = sector[sect].ceilingshade;
                break;

            case 13:

                T1 = sector[sect].ceilingz;
                T2 = sector[sect].floorz;

                if (klabs(T1-sp->z) < klabs(T2-sp->z))
                    sp->owner = 1;
                else sp->owner = 0;

                if (sp->ang == 512)
                {
                    if (sp->owner)
                        sector[sect].ceilingz = sp->z;
                    else
                        sector[sect].floorz = sp->z;
                }
                else
                    sector[sect].ceilingz = sector[sect].floorz = sp->z;

                if (sector[sect].ceilingstat&1)
                {
                    sector[sect].ceilingstat ^= 1;
                    T4 = 1;

                    if (!sp->owner && sp->ang==512)
                    {
                        sector[sect].ceilingstat ^= 1;
                        T4 = 0;
                    }

                    sector[sect].ceilingshade =
                        sector[sect].floorshade;

                    if (sp->ang==512)
                    {
                        startwall = sector[sect].wallptr;
                        endwall = startwall+sector[sect].wallnum;
                        for (j=startwall; j<endwall; j++)
                        {
                            x = wall[j].nextsector;
                            if (x >= 0)
                                if (!(sector[x].ceilingstat&1))
                                {
                                    sector[sect].ceilingpicnum =
                                        sector[x].ceilingpicnum;
                                    sector[sect].ceilingshade =
                                        sector[x].ceilingshade;
                                    break; //Leave earily
                                }
                        }
                    }
                }

                break;

            case 17:

                T3 = sector[sect].floorz; //Stopping loc

                j = nextsectorneighborz(sect,sector[sect].floorz,-1,-1);
                T4 = sector[j].ceilingz;

                j = nextsectorneighborz(sect,sector[sect].ceilingz,1,1);
                T5 = sector[j].floorz;

                if (numplayers < 2)
                {
                    G_SetInterpolation(&sector[sect].floorz);
                    G_SetInterpolation(&sector[sect].ceilingz);
                }

                break;

            case 24:
                sp->yvel <<= 1;
            case 36:
                break;

            case 20:
            {
                int32_t q;

                startwall = sector[sect].wallptr;
                endwall = startwall+sector[sect].wallnum;

                //find the two most clostest wall x's and y's
                q = 0x7fffffff;

                for (s=startwall; s<endwall; s++)
                {
                    x = wall[s].x;
                    y = wall[s].y;

                    d = FindDistance2D(sp->x-x,sp->y-y);
                    if (d < q)
                    {
                        q = d;
                        clostest = s;
                    }
                }

                T2 = clostest;

                q = 0x7fffffff;

                for (s=startwall; s<endwall; s++)
                {
                    x = wall[s].x;
                    y = wall[s].y;

                    d = FindDistance2D(sp->x-x,sp->y-y);
                    if (d < q && s != T2)
                    {
                        q = d;
                        clostest = s;
                    }
                }

                T3 = clostest;
            }

            break;

            case 3:

                T4=sector[sect].floorshade;

                sector[sect].floorshade = sp->shade;
                sector[sect].ceilingshade = sp->shade;

                sp->owner = sector[sect].ceilingpal<<8;
                sp->owner |= sector[sect].floorpal;

                //fix all the walls;

                startwall = sector[sect].wallptr;
                endwall = startwall+sector[sect].wallnum;

                for (s=startwall; s<endwall; s++)
                {
                    if (!(wall[s].hitag&1))
                        wall[s].shade=sp->shade;
                    if ((wall[s].cstat&2) && wall[s].nextwall >= 0)
                        wall[wall[s].nextwall].shade = sp->shade;
                }
                break;

            case 31:
                T2 = sector[sect].floorz;
                //    T3 = sp->hitag;
                if (sp->ang != 1536) sector[sect].floorz = sp->z;

                startwall = sector[sect].wallptr;
                endwall = startwall+sector[sect].wallnum;

                for (s=startwall; s<endwall; s++)
                    if (wall[s].hitag == 0) wall[s].hitag = 9999;

                G_SetInterpolation(&sector[sect].floorz);

                break;
            case 32:
                T2 = sector[sect].ceilingz;
                T3 = sp->hitag;
                if (sp->ang != 1536) sector[sect].ceilingz = sp->z;

                startwall = sector[sect].wallptr;
                endwall = startwall+sector[sect].wallnum;

                for (s=startwall; s<endwall; s++)
                    if (wall[s].hitag == 0) wall[s].hitag = 9999;

                G_SetInterpolation(&sector[sect].ceilingz);

                break;

            case 4: //Flashing lights

                T3 = sector[sect].floorshade;

                startwall = sector[sect].wallptr;
                endwall = startwall+sector[sect].wallnum;

                sp->owner = sector[sect].ceilingpal<<8;
                sp->owner |= sector[sect].floorpal;

                for (s=startwall; s<endwall; s++)
                    if (wall[s].shade > T4)
                        T4 = wall[s].shade;

                break;

            case 9:
                if (sector[sect].lotag &&
                        labs(sector[sect].ceilingz-sp->z) > 1024)
                    sector[sect].lotag |= 32768; //If its open
            case 8:
                //First, get the ceiling-floor shade

                T1 = sector[sect].floorshade;
                T2 = sector[sect].ceilingshade;

                startwall = sector[sect].wallptr;
                endwall = startwall+sector[sect].wallnum;

                for (s=startwall; s<endwall; s++)
                    if (wall[s].shade > T3)
                        T3 = wall[s].shade;

                T4 = 1; //Take Out;

                break;

            case 11://Pivitor rotater
                if (sp->ang>1024) T4 = 2;
                else T4 = -2;
            case 0:
            case 2://Earthquakemakers
            case 5://Boss Creature
            case 6://Subway
            case 14://Caboos
            case 15://Subwaytype sliding door
            case 16://That rotating blocker reactor thing
            case 26://ESCELATOR
            case 30://No rotational subways

                if (sp->lotag == 0)
                {
                    if (sector[sect].lotag == 30)
                    {
                        if (sp->pal) sprite[i].clipdist = 1;
                        else sprite[i].clipdist = 0;
                        T4 = sector[sect].floorz;
                        sector[sect].hitag = i;
                    }

                    for (j = MAXSPRITES-1; j>=0; j--)
                    {
                        if (sprite[j].statnum < MAXSTATUS)
                            if (sprite[j].picnum == SECTOREFFECTOR &&
                                    sprite[j].lotag == 1 &&
                                    sprite[j].hitag == sp->hitag)
                            {
                                if (sp->ang == 512)
                                {
                                    sp->x = sprite[j].x;
                                    sp->y = sprite[j].y;
                                }
                                break;
                            }
                    }
                    if (j == -1)
                    {
//                        Bsprintf(tempbuf,"Found lonely Sector Effector (lotag 0) at (%d,%d)\n",sp->x,sp->y);
//                        G_GameExit(tempbuf);
                        OSD_Printf(OSD_ERROR "Found lonely Sector Effector (lotag 0) at (%d,%d)\n",sp->x,sp->y);
                        changespritestat(i, STAT_ACTOR);
                        if (apScriptGameEvent[EVENT_SPAWN])
                        {
                            int32_t pl=A_FindPlayer(&sprite[i],&p);
                            X_OnEvent(EVENT_SPAWN,i, pl, p);
                        }
                        return i;
                    }
                    sp->owner = j;
                }

                startwall = sector[sect].wallptr;
                endwall = startwall+sector[sect].wallnum;

                T2 = tempwallptr;
                for (s=startwall; s<endwall; s++)
                {
                    msx[tempwallptr] = wall[s].x-sp->x;
                    msy[tempwallptr] = wall[s].y-sp->y;
                    tempwallptr++;
                    if (tempwallptr > 2047)
                    {
                        Bsprintf(tempbuf,"Too many moving sectors at (%d,%d).\n",wall[s].x,wall[s].y);
                        G_GameExit(tempbuf);
                    }
                }
                if (sp->lotag == 30 || sp->lotag == 6 || sp->lotag == 14 || sp->lotag == 5)
                {

                    startwall = sector[sect].wallptr;
                    endwall = startwall+sector[sect].wallnum;

                    if (sector[sect].hitag == -1)
                        sp->extra = 0;
                    else sp->extra = 1;

                    sector[sect].hitag = i;

                    j = 0;

                    for (s=startwall; s<endwall; s++)
                    {
                        if (wall[ s ].nextsector >= 0 &&
                                sector[ wall[ s ].nextsector].hitag == 0 &&
                                sector[ wall[ s ].nextsector].lotag < 3)
                        {
                            s = wall[s].nextsector;
                            j = 1;
                            break;
                        }
                    }

                    if (j == 0)
                    {
                        Bsprintf(tempbuf,"Subway found no zero'd sectors with locators\nat (%d,%d).\n",sp->x,sp->y);
                        G_GameExit(tempbuf);
                    }

                    sp->owner = -1;
                    T1 = s;

                    if (sp->lotag != 30)
                        T4 = sp->hitag;
                }

                else if (sp->lotag == 16)
                    T4 = sector[sect].ceilingz;

                else if (sp->lotag == 26)
                {
                    T4 = sp->x;
                    T5 = sp->y;
                    if (sp->shade==sector[sect].floorshade) //UP
                        sp->zvel = -256;
                    else
                        sp->zvel = 256;

                    sp->shade = 0;
                }
                else if (sp->lotag == 2)
                {
                    T6 = sector[sp->sectnum].floorheinum;
                    sector[sp->sectnum].floorheinum = 0;
                }
            }

            switch (sp->lotag)
            {
            case 6:
            case 14:
                j = A_CallSound(sect,i);
                if (j == -1) j = SUBWAY;
                ActorExtra[i].lastvx = j;
            case 30:
                if (numplayers > 1) break;
            case 0:
            case 1:
            case 5:
            case 11:
            case 15:
            case 16:
            case 26:
                Sect_SetInterpolation(i);
                break;
            }

            changespritestat(i, STAT_EFFECTOR);
            break;

        case SEENINE__STATIC:
        case OOZFILTER__STATIC:
            sp->shade = -16;
            if (sp->xrepeat <= 8)
            {
                sp->cstat = (int16_t)32768;
                sp->xrepeat=sp->yrepeat=0;
            }
            else sp->cstat = 1+256;
            sp->extra = g_impactDamage<<2;
            sp->owner = i;

            changespritestat(i,6);
            break;

        case CRACK1__STATIC:
        case CRACK2__STATIC:
        case CRACK3__STATIC:
        case CRACK4__STATIC:
        case FIREEXT__STATIC:
            if (sp->picnum == FIREEXT)
            {
                sp->cstat = 257;
                sp->extra = g_impactDamage<<2;
            }
            else
            {
                sp->cstat |= (sp->cstat & 48) ? 1 : 17;
                sp->extra = 1;
            }

            if (ud.multimode < 2 && sp->pal != 0)
            {
                sp->xrepeat = sp->yrepeat = 0;
                changespritestat(i,5);
                break;
            }

            sp->pal = 0;
            sp->owner = i;
            changespritestat(i,6);
            sp->xvel = 8;
            A_SetSprite(i,CLIPMASK0);
            break;

        case TOILET__STATIC:
        case STALL__STATIC:
            sp->lotag = 1;
            sp->cstat |= 257;
            sp->clipdist = 8;
            sp->owner = i;
            break;

        case CANWITHSOMETHING__STATIC:
        case CANWITHSOMETHING2__STATIC:
        case CANWITHSOMETHING3__STATIC:
        case CANWITHSOMETHING4__STATIC:
        case RUBBERCAN__STATIC:
            sp->extra = 0;
        case EXPLODINGBARREL__STATIC:
        case HORSEONSIDE__STATIC:
        case FIREBARREL__STATIC:
        case NUKEBARREL__STATIC:
        case FIREVASE__STATIC:
        case NUKEBARRELDENTED__STATIC:
        case NUKEBARRELLEAKED__STATIC:
        case WOODENHORSE__STATIC:
            if (j >= 0)
                sp->xrepeat = sp->yrepeat = 32;
            sp->clipdist = 72;
            A_Fall(i);
            if (j >= 0)
                sp->owner = j;
            else sp->owner = i;
        case EGG__STATIC:
            if (ud.monsters_off == 1 && sp->picnum == EGG)
            {
                sp->xrepeat = sp->yrepeat = 0;
                changespritestat(i,5);
            }
            else
            {
                if (sp->picnum == EGG)
                    sp->clipdist = 24;
                sp->cstat = 257|(krand()&4);
                changespritestat(i, STAT_ZOMBIEACTOR);
            }
            break;

        case TOILETWATER__STATIC:
            sp->shade = -16;
            changespritestat(i,6);
            break;
        }

SPAWN_END:
    if (apScriptGameEvent[EVENT_SPAWN])
    {
        int32_t pl=A_FindPlayer(&sprite[i],&p);
        X_OnEvent(EVENT_SPAWN,i, pl, p);
    }

    return i;
}

#if 0 // def _MSC_VER
// Visual C thought this was a bit too hard to optimise so we'd better
// tell it not to try... such a pussy it is.
//#pragma auto_inline(off)
#pragma optimize("g",off)
#endif
void G_DoSpriteAnimations(int32_t x,int32_t y,int32_t a,int32_t smoothratio)
{
    int32_t i, j, k, p, sect;
    intptr_t l, t1,t3,t4;
    spritetype *s,*t;
    int32_t switchpic;

    if (!spritesortcnt) return;

    ror_sprite = -1;

    for (j=spritesortcnt-1; j>=0; j--)
    {
        t = &tsprite[j];
        i = t->owner;
        s = &sprite[i];

        switch (DynamicTileMap[s->picnum])
        {
        case SECTOREFFECTOR__STATIC:
            if (s->lotag == 40 || s->lotag == 41)
            {
                t->cstat = 32768;

                if (ror_sprite == -1) ror_sprite = i;
            }

            if (t->lotag == 27 && ud.recstat == 1)
            {
                t->picnum = 11+((totalclock>>3)&1);
                t->cstat |= 128;
            }
            else
                t->xrepeat = t->yrepeat = 0;
            break;
        }
    }

    for (j=spritesortcnt-1; j>=0; j--)
    {
        t = &tsprite[j];
        i = t->owner;
        s = &sprite[t->owner];

        //greenslime can't be handled through the dynamictostatic system due to addition on constant
        if ((t->picnum >= GREENSLIME)&&(t->picnum <= GREENSLIME+7))
            {}
        else switch (DynamicTileMap[t->picnum])
            {
            case BLOODPOOL__STATIC:
            case PUKE__STATIC:
            case FOOTPRINTS__STATIC:
            case FOOTPRINTS2__STATIC:
            case FOOTPRINTS3__STATIC:
            case FOOTPRINTS4__STATIC:
                if (t->shade == 127) continue;
                break;
            case RESPAWNMARKERRED__STATIC:
            case RESPAWNMARKERYELLOW__STATIC:
            case RESPAWNMARKERGREEN__STATIC:
                if (ud.marker == 0)
                    t->xrepeat = t->yrepeat = 0;
                continue;
            case CHAIR3__STATIC:
#if defined(POLYMOST) && defined(USE_OPENGL)
                if (getrendermode() >= 3 && usemodels && md_tilehasmodel(t->picnum,t->pal) >= 0 && !(spriteext[i].flags&SPREXT_NOTMD))
                {
                    t->cstat &= ~4;
                    break;
                }
#endif
                k = (((t->ang+3072+128-a)&2047)>>8)&7;
                if (k>4)
                {
                    k = 8-k;
                    t->cstat |= 4;
                }
                else t->cstat &= ~4;
                t->picnum = s->picnum+k;
                break;
            case BLOODSPLAT1__STATIC:
            case BLOODSPLAT2__STATIC:
            case BLOODSPLAT3__STATIC:
            case BLOODSPLAT4__STATIC:
                if (ud.lockout) t->xrepeat = t->yrepeat = 0;
                else if (t->pal == 6)
                {
                    t->shade = -127;
                    continue;
                }
            case BULLETHOLE__STATIC:
            case CRACK1__STATIC:
            case CRACK2__STATIC:
            case CRACK3__STATIC:
            case CRACK4__STATIC:
                t->shade = 16;
                continue;
            case NEON1__STATIC:
            case NEON2__STATIC:
            case NEON3__STATIC:
            case NEON4__STATIC:
            case NEON5__STATIC:
            case NEON6__STATIC:
                continue;
                //case GREENSLIME:
                //case GREENSLIME+1:
                //case GREENSLIME+2:
                //case GREENSLIME+3:
                //case GREENSLIME+4:
                //case GREENSLIME+5:
                //case GREENSLIME+6:
                //case GREENSLIME+7:
                //    break;
            default:
                if (((t->cstat&16)) || (A_CheckEnemySprite(t) && t->extra > 0) || t->statnum == STAT_PLAYER)
                    continue;
            }

        if (A_CheckSpriteFlags(t->owner,SPRITE_NOSHADE))
            l = sprite[t->owner].shade;
        else
        {
            if (sector[t->sectnum].ceilingstat&1)
                l = sector[t->sectnum].ceilingshade;
            else
                l = sector[t->sectnum].floorshade;
            if (l < -127) l = -127;
            if (l > 128) l =  127;
        }
        t->shade = l;
    }

    for (j=spritesortcnt-1; j>=0; j--) //Between drawrooms() and drawmasks()
    {
        //is the perfect time to animate sprites
        t = &tsprite[j];
        i = t->owner;
        s = (i < 0 ? &tsprite[j] : &sprite[i]);

        switch (DynamicTileMap[s->picnum])
        {
        case NATURALLIGHTNING__STATIC:
            t->shade = -127;
            t->cstat |= 8192;
            break;
        case FEM1__STATIC:
        case FEM2__STATIC:
        case FEM3__STATIC:
        case FEM4__STATIC:
        case FEM5__STATIC:
        case FEM6__STATIC:
        case FEM7__STATIC:
        case FEM8__STATIC:
        case FEM9__STATIC:
        case FEM10__STATIC:
        case MAN__STATIC:
        case MAN2__STATIC:
        case WOMAN__STATIC:
        case NAKED1__STATIC:
        case PODFEM1__STATIC:
        case FEMMAG1__STATIC:
        case FEMMAG2__STATIC:
        case FEMPIC1__STATIC:
        case FEMPIC2__STATIC:
        case FEMPIC3__STATIC:
        case FEMPIC4__STATIC:
        case FEMPIC5__STATIC:
        case FEMPIC6__STATIC:
        case FEMPIC7__STATIC:
        case BLOODYPOLE__STATIC:
        case FEM6PAD__STATIC:
        case STATUE__STATIC:
        case STATUEFLASH__STATIC:
        case OOZ__STATIC:
        case OOZ2__STATIC:
        case WALLBLOOD1__STATIC:
        case WALLBLOOD2__STATIC:
        case WALLBLOOD3__STATIC:
        case WALLBLOOD4__STATIC:
        case WALLBLOOD5__STATIC:
        case WALLBLOOD7__STATIC:
        case WALLBLOOD8__STATIC:
        case SUSHIPLATE1__STATIC:
        case SUSHIPLATE2__STATIC:
        case SUSHIPLATE3__STATIC:
        case SUSHIPLATE4__STATIC:
        case FETUS__STATIC:
        case FETUSJIB__STATIC:
        case FETUSBROKE__STATIC:
        case HOTMEAT__STATIC:
        case FOODOBJECT16__STATIC:
        case DOLPHIN1__STATIC:
        case DOLPHIN2__STATIC:
        case TOUGHGAL__STATIC:
        case TAMPON__STATIC:
        case XXXSTACY__STATIC:
            if (ud.lockout)
            {
                t->xrepeat = t->yrepeat = 0;
                continue;
            }
        }
        switch (s->picnum)
        {

        case 4946:
        case 4947:
        case 693:
        case 2254:
        case 4560:
        case 4561:
        case 4562:
        case 4498:
        case 4957:
            if (ud.lockout)
            {
                t->xrepeat = t->yrepeat = 0;
                continue;
            }
        }

        if (t->statnum == TSPR_TEMP) continue;
        if (s->statnum != STAT_ACTOR && s->picnum == APLAYER && g_player[s->yvel].ps->newowner == -1 && s->owner >= 0)
        {
            t->x -= mulscale16(65536-smoothratio,g_player[s->yvel].ps->posx-g_player[s->yvel].ps->oposx);
            t->y -= mulscale16(65536-smoothratio,g_player[s->yvel].ps->posy-g_player[s->yvel].ps->oposy);
            t->z += /*g_player[s->yvel].ps->oposz +*/ mulscale16(smoothratio,g_player[s->yvel].ps->posz-g_player[s->yvel].ps->oposz) - PHEIGHT;
            t->z += (40<<8);
        }
        else if ((s->statnum == STAT_DEFAULT && s->picnum != CRANEPOLE) || s->statnum == STAT_PLAYER ||
            s->statnum == STAT_STANDABLE || s->statnum == STAT_PROJECTILE || s->statnum == STAT_MISC || s->statnum == STAT_ACTOR)
        {
            t->x -= mulscale16(65536-smoothratio,s->x-ActorExtra[i].bposx);
            t->y -= mulscale16(65536-smoothratio,s->y-ActorExtra[i].bposy);
            t->z -= mulscale16(65536-smoothratio,s->z-ActorExtra[i].bposz);
        }

        sect = s->sectnum;
        t1 = T2;
        t3 = T4;
        t4 = T5;
        switchpic = s->picnum;
        //some special cases because dynamictostatic system can't handle addition to constants
        if ((s->picnum >= SCRAP6)&&(s->picnum<=SCRAP6+7))
            switchpic = SCRAP5;
        else if ((s->picnum==MONEY+1)||(s->picnum==MAIL+1)||(s->picnum==PAPER+1))
            switchpic--;

        switch (DynamicTileMap[switchpic])
        {
        case DUKELYINGDEAD__STATIC:
            t->z += (24<<8);
            break;
        case BLOODPOOL__STATIC:
        case FOOTPRINTS__STATIC:
        case FOOTPRINTS2__STATIC:
        case FOOTPRINTS3__STATIC:
        case FOOTPRINTS4__STATIC:
            if (t->pal == 6)
                t->shade = -127;
        case PUKE__STATIC:
        case MONEY__STATIC:
            //case MONEY+1__STATIC:
        case MAIL__STATIC:
            //case MAIL+1__STATIC:
        case PAPER__STATIC:
            //case PAPER+1__STATIC:
            if (ud.lockout && s->pal == 2)
            {
                t->xrepeat = t->yrepeat = 0;
                continue;
            }
            break;
        case TRIPBOMB__STATIC:
            continue;
        case FORCESPHERE__STATIC:
            if (t->statnum == 5)
            {
                int16_t sqa,sqb;

                sqa =
                    getangle(
                        sprite[s->owner].x-g_player[screenpeek].ps->posx,
                        sprite[s->owner].y-g_player[screenpeek].ps->posy);
                sqb =
                    getangle(
                        sprite[s->owner].x-t->x,
                        sprite[s->owner].y-t->y);

                if (klabs(G_GetAngleDelta(sqa,sqb)) > 512)
                    if (ldist(&sprite[s->owner],t) < ldist(&sprite[g_player[screenpeek].ps->i],&sprite[s->owner]))
                        t->xrepeat = t->yrepeat = 0;
            }
            continue;
        case BURNING__STATIC:
        case BURNING2__STATIC:
            if (sprite[s->owner].statnum == STAT_PLAYER)
            {
                if (display_mirror == 0 && sprite[s->owner].yvel == screenpeek && g_player[sprite[s->owner].yvel].ps->over_shoulder_on == 0)
                    t->xrepeat = 0;
                else
                {
                    t->ang = getangle(x-t->x,y-t->y);
                    t->x = sprite[s->owner].x;
                    t->y = sprite[s->owner].y;
                    t->x += sintable[(t->ang+512)&2047]>>10;
                    t->y += sintable[t->ang&2047]>>10;
                }
            }
            break;

        case ATOMICHEALTH__STATIC:
            t->z -= (4<<8);
            break;
        case CRYSTALAMMO__STATIC:
            t->shade = (sintable[(totalclock<<4)&2047]>>10);
            continue;
        case VIEWSCREEN__STATIC:
        case VIEWSCREEN2__STATIC:
            if (camsprite >= 0 && ActorExtra[OW].temp_data[0] == 1)
            {
                t->picnum = STATIC;
                t->cstat |= (rand()&12);
                t->xrepeat += 8;
                t->yrepeat += 8;
            }
            else if (camsprite >= 0 && waloff[TILE_VIEWSCR] && walock[TILE_VIEWSCR] > 200)
            {
                t->picnum = TILE_VIEWSCR;
            }
            break;

        case SHRINKSPARK__STATIC:
            t->picnum = SHRINKSPARK+((totalclock>>4)&3);
            break;
        case GROWSPARK__STATIC:
            t->picnum = GROWSPARK+((totalclock>>4)&3);
            break;
        case RPG__STATIC:
#if defined(POLYMOST) && defined(USE_OPENGL)
            if (getrendermode() >= 3 && usemodels && md_tilehasmodel(t->picnum,t->pal) >= 0 &&
                    !(spriteext[i].flags & SPREXT_NOTMD))
            {
                int32_t v = getangle(t->xvel, t->zvel>>4);

                spriteext[i].pitch = (v > 1023 ? v-2048 : v);
                t->cstat &= ~4;
                break;
            }
#endif
            k = (((s->ang+3072+128-getangle(s->x-x,s->y-y))&2047)/170);
            if (k > 6)
            {
                k = 12-k;
                t->cstat |= 4;
            }
            else t->cstat &= ~4;
            t->picnum = RPG+k;
            break;

        case RECON__STATIC:

#if defined(POLYMOST) && defined(USE_OPENGL)
            if (getrendermode() >= 3 && usemodels && md_tilehasmodel(t->picnum,t->pal) >= 0 && !(spriteext[i].flags&SPREXT_NOTMD))
            {
                t->cstat &= ~4;
                break;
            }
#endif
            k = (((s->ang+3072+128-getangle(s->x-x,s->y-y))&2047)/170);

            if (k>6)
            {
                k = 12-k;
                t->cstat |= 4;
            }
            else t->cstat &= ~4;

            if (klabs(t3) > 64) k += 7;
            t->picnum = RECON+k;

            break;

        case APLAYER__STATIC:

            p = s->yvel;

            if (t->pal == 1) t->z -= (18<<8);

            if (g_player[p].ps->over_shoulder_on > 0 && g_player[p].ps->newowner < 0)
            {
                if (screenpeek == myconnectindex && numplayers >= 2)
                {
                    t->x = omy.x+mulscale16((int32_t)(my.x-omy.x),smoothratio);
                    t->y = omy.y+mulscale16((int32_t)(my.y-omy.y),smoothratio);
                    t->z = omy.z+mulscale16((int32_t)(my.z-omy.z),smoothratio)+(40<<8);
                    t->ang = omyang+mulscale16((int32_t)(((myang+1024-omyang)&2047)-1024),smoothratio);
                    t->sectnum = mycursectnum;
                }
                else t->ang = g_player[p].ps->ang+mulscale16((int32_t)(((g_player[p].ps->ang+1024- g_player[p].ps->oang)&2047)-1024),smoothratio);
#if defined(POLYMOST) && defined(USE_OPENGL)
                if (bpp > 8 && usemodels && md_tilehasmodel(t->picnum, t->pal) >= 0)
                {
                    static int32_t targetang = 0;

                    if (g_player[p].sync->extbits&(1<<1))
                    {
                        if (g_player[p].sync->extbits&(1<<2))targetang += 16;
                        else if (g_player[p].sync->extbits&(1<<3)) targetang -= 16;
                        else if (targetang > 0) targetang -= targetang>>2;
                        else if (targetang < 0) targetang += (-targetang)>>2;
                    }
                    else
                    {
                        if (g_player[p].sync->extbits&(1<<2))targetang -= 16;
                        else if (g_player[p].sync->extbits&(1<<3)) targetang += 16;
                        else if (targetang > 0) targetang -= targetang>>2;
                        else if (targetang < 0) targetang += (-targetang)>>2;
                    }

                    targetang = clamp(targetang, -128, 128);
                    t->ang += targetang;
                }
                else
#endif
                    t->cstat |= 2;
            }

            if (ud.multimode > 1 && (display_mirror || screenpeek != p || s->owner == -1))
            {
                if (ud.showweapons && sprite[g_player[p].ps->i].extra > 0 && g_player[p].ps->curr_weapon > 0)
                {
                    Bmemcpy((spritetype *)&tsprite[spritesortcnt],(spritetype *)t,sizeof(spritetype));

                    tsprite[spritesortcnt].statnum = TSPR_TEMP;

                    /*                    tsprite[spritesortcnt].yrepeat = (t->yrepeat>>3);
                                        if (t->yrepeat < 4) t->yrepeat = 4; */

                    tsprite[spritesortcnt].shade = t->shade;
                    tsprite[spritesortcnt].cstat = tsprite[spritesortcnt].pal = 0;

                    tsprite[spritesortcnt].picnum = (g_player[p].ps->curr_weapon==GROW_WEAPON?GROWSPRITEICON:WeaponPickupSprites[g_player[p].ps->curr_weapon]);

                    if (s->owner >= 0)
                        tsprite[spritesortcnt].z = g_player[p].ps->posz-(12<<8);
                    else tsprite[spritesortcnt].z = s->z-(51<<8);

                    if (tsprite[spritesortcnt].picnum == HEAVYHBOMB)
                        tsprite[spritesortcnt].xrepeat = tsprite[spritesortcnt].yrepeat = 10;
                    else
                        tsprite[spritesortcnt].xrepeat = tsprite[spritesortcnt].yrepeat = 16;

                    spritesortcnt++;
                }

                if (g_player[p].sync->extbits & (1<<7) && !ud.pause_on)
                {
                    Bmemcpy((spritetype *)&tsprite[spritesortcnt],(spritetype *)t,sizeof(spritetype));

                    tsprite[spritesortcnt].statnum = TSPR_TEMP;

                    tsprite[spritesortcnt].yrepeat = (t->yrepeat>>3);
                    if (tsprite[spritesortcnt].yrepeat < 4) tsprite[spritesortcnt].yrepeat = 4;

                    tsprite[spritesortcnt].cstat = 0;
                    tsprite[spritesortcnt].picnum = RESPAWNMARKERGREEN;

                    if (s->owner >= 0)
                        tsprite[spritesortcnt].z = g_player[p].ps->posz-(20<<8);
                    else
                        tsprite[spritesortcnt].z = s->z-(96<<8);

                    tsprite[spritesortcnt].xrepeat = tsprite[spritesortcnt].yrepeat = 32;
                    tsprite[spritesortcnt].pal = 20;
                    spritesortcnt++;
                }
            }

            if (s->owner == -1)
            {

#if defined(POLYMOST) && defined(USE_OPENGL)
                if (getrendermode() >= 3 && usemodels && md_tilehasmodel(s->picnum,t->pal) >= 0 && !(spriteext[i].flags&SPREXT_NOTMD))
                {
                    k = 0;
                    t->cstat &= ~4;
                }
                else
#endif
                {
                    k = (((s->ang+3072+128-a)&2047)>>8)&7;
                    if (k>4)
                    {
                        k = 8-k;
                        t->cstat |= 4;
                    }
                    else t->cstat &= ~4;
                }

                if (sector[s->sectnum].lotag == 2) k += 1795-1405;
                else if ((ActorExtra[i].floorz-s->z) > (64<<8)) k += 60;

                t->picnum += k;
                t->pal = g_player[p].ps->palookup;

                goto PALONLY;
            }

            if (g_player[p].ps->on_crane == -1 && (sector[s->sectnum].lotag&0x7ff) != 1)
            {
                l = s->z-ActorExtra[g_player[p].ps->i].floorz+(3<<8);
                if (l > 1024 && s->yrepeat > 32 && s->extra > 0)
                    s->yoffset = (int8_t)(l/(s->yrepeat<<2));
                else s->yoffset=0;
            }

            if (g_player[p].ps->newowner > -1)
            {
                t4 = *(actorscrptr[APLAYER]+1);
                t3 = 0;
                t1 = *(actorscrptr[APLAYER]+2);
            }

            if (ud.camerasprite == -1 && g_player[p].ps->newowner == -1)
                if (s->owner >= 0 && display_mirror == 0 && g_player[p].ps->over_shoulder_on == 0)
                    if (ud.multimode < 2 || (ud.multimode > 1 && p == screenpeek))
                    {
                        if (getrendermode() == 4)
                            t->cstat |= 16384;
                        else
                        {
                            t->owner = -1;
                            t->xrepeat = t->yrepeat = 0;
                            continue;
                        }

#if defined(POLYMOST) && defined(USE_OPENGL)
                        if (getrendermode() >= 3 && usemodels && md_tilehasmodel(s->picnum,t->pal) >= 0 && !(spriteext[i].flags&SPREXT_NOTMD))
                        {
                            k = 0;
                            t->cstat &= ~4;
                        }
                        else
#endif
                        {
                            k = (((s->ang+3072+128-a)&2047)>>8)&7;
                            if (k>4)
                            {
                                k = 8-k;
                                t->cstat |= 4;
                            }
                            else t->cstat &= ~4;
                        }

                        if (sector[t->sectnum].lotag == 2) k += 1795-1405;
                        else if ((ActorExtra[i].floorz-s->z) > (64<<8)) k += 60;

                        t->picnum += k;
                        t->pal = g_player[p].ps->palookup;
                    }
PALONLY:

            if (sector[sect].floorpal && sector[sect].floorpal < g_numRealPalettes && !A_CheckSpriteFlags(t->owner,SPRITE_NOPAL))
                t->pal = sector[sect].floorpal;

            if (s->owner == -1) continue;

            if (t->z > ActorExtra[i].floorz && t->xrepeat < 32)
                t->z = ActorExtra[i].floorz;

            break;

        case JIBS1__STATIC:
        case JIBS2__STATIC:
        case JIBS3__STATIC:
        case JIBS4__STATIC:
        case JIBS5__STATIC:
        case JIBS6__STATIC:
        case HEADJIB1__STATIC:
        case LEGJIB1__STATIC:
        case ARMJIB1__STATIC:
        case LIZMANHEAD1__STATIC:
        case LIZMANARM1__STATIC:
        case LIZMANLEG1__STATIC:
        case DUKELEG__STATIC:
        case DUKEGUN__STATIC:
        case DUKETORSO__STATIC:
            if (ud.lockout)
            {
                t->xrepeat = t->yrepeat = 0;
                continue;
            }
            if (t->pal == 6) t->shade = -120;
        case SCRAP1__STATIC:
        case SCRAP2__STATIC:
        case SCRAP3__STATIC:
        case SCRAP4__STATIC:
        case SCRAP5__STATIC:
            if (ActorExtra[i].picnum == BLIMP && t->picnum == SCRAP1 && s->yvel >= 0)
                t->picnum = s->yvel;
            else t->picnum += T1;
            t->shade -= 6;

            if (sector[sect].floorpal && sector[sect].floorpal < g_numRealPalettes && !A_CheckSpriteFlags(t->owner,SPRITE_NOPAL))
                t->pal = sector[sect].floorpal;
            break;

        case WATERBUBBLE__STATIC:
            if (sector[t->sectnum].floorpicnum == FLOORSLIME)
            {
                t->pal = 7;
                break;
            }
        default:
            if (sector[sect].floorpal && sector[sect].floorpal < g_numRealPalettes && !A_CheckSpriteFlags(t->owner,SPRITE_NOPAL))
                t->pal = sector[sect].floorpal;
            break;
        }

        if (actorscrptr[s->picnum])
        {
            /*
            if (ud.angleinterpolation)
            {
                if (sprpos[i].ang != sprpos[i].oldang)
                    t->ang = (sprpos[i].oldang + (mulscale16((int32_t)(sprpos[i].angdif),smoothratio) * sprpos[i].angdir)) & 2047;
                else
                    t->ang = sprpos[i].ang;
            }
            */

            if (t4 && t4 != 1)
            {
                l = *(((intptr_t *)t4)+2); //For TerminX: was *(int32_t *)(t4+8)

#if defined(POLYMOST) && defined(USE_OPENGL)
                if (getrendermode() >= 3 && usemodels && md_tilehasmodel(s->picnum,t->pal) >= 0 && !(spriteext[i].flags&SPREXT_NOTMD))
                {
                    k = 0;
                    t->cstat &= ~4;
                }
                else
#endif
                    switch (l)
                    {
                    case 2:
                        k = (((s->ang+3072+128-a)&2047)>>8)&1;
                        break;

                    case 3:
                    case 4:
                        k = (((s->ang+3072+128-a)&2047)>>7)&7;
                        if (k > 3)
                        {
                            t->cstat |= 4;
                            k = 7-k;
                        }
                        else t->cstat &= ~4;
                        break;

                    case 5:
                        k = getangle(s->x-x,s->y-y);
                        k = (((s->ang+3072+128-k)&2047)>>8)&7;
                        if (k>4)
                        {
                            k = 8-k;
                            t->cstat |= 4;
                        }
                        else t->cstat &= ~4;
                        break;
                    case 7:
                        k = getangle(s->x-x,s->y-y);
                        k = (((s->ang+3072+128-k)&2047)/170);
                        if (k>6)
                        {
                            k = 12-k;
                            t->cstat |= 4;
                        }
                        else t->cstat &= ~4;
                        break;
                    case 8:
                        k = (((s->ang+3072+128-a)&2047)>>8)&7;
                        t->cstat &= ~4;
                        break;
                    default:
                        k = 0;
                        break;
                    }

                t->picnum += k + (*(intptr_t *)t4) + l * t3;

                if (l > 0) while (tilesizx[t->picnum] == 0 && t->picnum > 0)
                        t->picnum -= l;       //Hack, for actors

                if (ActorExtra[i].dispicnum >= 0)
                    ActorExtra[i].dispicnum = t->picnum;
            }
            else if (display_mirror == 1)
                t->cstat |= 4;
        }

        if (g_player[screenpeek].ps->heat_amount > 0 && g_player[screenpeek].ps->heat_on &&
                (A_CheckEnemySprite(s) || A_CheckSpriteFlags(t->owner,SPRITE_NVG) || s->picnum == APLAYER || s->statnum == STAT_DUMMYPLAYER))
        {
            t->pal = 6;
            t->shade = 0;
        }

        if (s->statnum == STAT_DUMMYPLAYER || A_CheckEnemySprite(s) || A_CheckSpriteFlags(t->owner,SPRITE_SHADOW) || (s->picnum == APLAYER && s->owner >= 0))
            if (t->statnum != TSPR_TEMP && s->picnum != EXPLOSION2 && s->picnum != HANGLIGHT && s->picnum != DOMELITE)
                if (s->picnum != HOTMEAT)
                {
                    if (ActorExtra[i].dispicnum < 0)
                    {
                        ActorExtra[i].dispicnum++;
                        continue;
                    }
                    else if (ud.shadows && spritesortcnt < (MAXSPRITESONSCREEN-2) && getrendermode() != 4)
                    {
                        int32_t daz,xrep,yrep;

                        if ((sector[sect].lotag&0xff) > 2 || s->statnum == STAT_PROJECTILE || s->statnum == 5 || s->picnum == DRONE || s->picnum == COMMANDER)
                            daz = sector[sect].floorz;
                        else
                            daz = ActorExtra[i].floorz;

                        if ((s->z-daz) < (8<<8))
                            if (g_player[screenpeek].ps->posz < daz)
                            {
                                Bmemcpy((spritetype *)&tsprite[spritesortcnt],(spritetype *)t,sizeof(spritetype));

                                tsprite[spritesortcnt].statnum = TSPR_TEMP;

                                tsprite[spritesortcnt].yrepeat = (t->yrepeat>>3);
                                if (t->yrepeat < 4) t->yrepeat = 4;

                                tsprite[spritesortcnt].shade = 127;
                                tsprite[spritesortcnt].cstat |= 2;

                                tsprite[spritesortcnt].z = daz;
                                xrep = tsprite[spritesortcnt].xrepeat;// - (klabs(daz-t->z)>>11);
                                tsprite[spritesortcnt].xrepeat = xrep;
                                tsprite[spritesortcnt].pal = 4;

                                yrep = tsprite[spritesortcnt].yrepeat;// - (klabs(daz-t->z)>>11);
                                tsprite[spritesortcnt].yrepeat = yrep;

#if defined(POLYMOST) && defined(USE_OPENGL)
                                if (getrendermode() >= 3 && usemodels && md_tilehasmodel(t->picnum,t->pal) >= 0)
                                {
                                    tsprite[spritesortcnt].yrepeat = 0;
                                    // 512:trans reverse
                                    //1024:tell MD2SPRITE.C to use Z-buffer hacks to hide overdraw issues
                                    tsprite[spritesortcnt].cstat |= (512+1024);
                                }
                                else if (getrendermode() >= 3)
                                {
                                    int32_t ii;

                                    ii = getangle(tsprite[spritesortcnt].x-g_player[screenpeek].ps->posx,
                                                  tsprite[spritesortcnt].y-g_player[screenpeek].ps->posy);

                                    tsprite[spritesortcnt].x += sintable[(ii+2560)&2047]>>9;
                                    tsprite[spritesortcnt].y += sintable[(ii+2048)&2047]>>9;
                                }
#endif
                                spritesortcnt++;
                            }
                    }
                }

        switch (DynamicTileMap[s->picnum])
        {
        case LASERLINE__STATIC:
            if (sector[t->sectnum].lotag == 2) t->pal = 8;
            t->z = sprite[s->owner].z-(3<<8);
            if (g_tripbombLaserMode == 2 && g_player[screenpeek].ps->heat_on == 0)
                t->yrepeat = 0;
        case EXPLOSION2__STATIC:
        case EXPLOSION2BOT__STATIC:
        case FREEZEBLAST__STATIC:
        case ATOMICHEALTH__STATIC:
        case FIRELASER__STATIC:
        case SHRINKSPARK__STATIC:
        case GROWSPARK__STATIC:
        case CHAINGUN__STATIC:
        case SHRINKEREXPLOSION__STATIC:
        case RPG__STATIC:
        case FLOORFLAME__STATIC:
            if (t->picnum == EXPLOSION2)
            {
                g_player[screenpeek].ps->visibility = -127;
                lastvisinc = totalclock+32;
                //g_restorePalette = 1;   // JBF 20040101: why?
            }
            t->shade = -127;
            t->cstat |= 8192;
            break;
        case FIRE__STATIC:
        case FIRE2__STATIC:
        case BURNING__STATIC:
        case BURNING2__STATIC:
            if (sprite[s->owner].picnum != TREE1 && sprite[s->owner].picnum != TREE2)
                t->z = sector[t->sectnum].floorz;
            t->shade = -127;
            t->cstat |= 8192;
            break;
        case COOLEXPLOSION1__STATIC:
            t->shade = -127;
            t->cstat |= 8192;
            t->picnum += (s->shade>>1);
            break;
        case PLAYERONWATER__STATIC:
#if defined(POLYMOST) && defined(USE_OPENGL)
            if (getrendermode() >= 3 && usemodels && md_tilehasmodel(s->picnum,s->pal) >= 0 && !(spriteext[i].flags&SPREXT_NOTMD))
            {
                k = 0;
                t->cstat &= ~4;
            }
            else
#endif
            {
                k = (((t->ang+3072+128-a)&2047)>>8)&7;
                if (k>4)
                {
                    k = 8-k;
                    t->cstat |= 4;
                }
                else t->cstat &= ~4;
            }

            t->picnum = s->picnum+k+((T1<4)*5);
            t->shade = sprite[s->owner].shade;

            break;

        case WATERSPLASH2__STATIC:
            t->picnum = WATERSPLASH2+t1;
            break;
        case REACTOR2__STATIC:
            t->picnum = s->picnum + T3;
            break;
        case SHELL__STATIC:
            t->picnum = s->picnum+(T1&1);
        case SHOTGUNSHELL__STATIC:
            t->cstat |= 12;
            if (T1 > 1) t->cstat &= ~4;
            if (T1 > 2) t->cstat &= ~12;
            break;
        case FRAMEEFFECT1_13__STATIC:
            if (PLUTOPAK) break;
        case FRAMEEFFECT1__STATIC:
            if (s->owner >= 0 && sprite[s->owner].statnum < MAXSTATUS)
            {
                if (sprite[s->owner].picnum == APLAYER)
                    if (ud.camerasprite == -1)
                        if (screenpeek == sprite[s->owner].yvel && display_mirror == 0)
                        {
                            t->owner = -1;
                            break;
                        }
                if ((sprite[s->owner].cstat&32768) == 0)
                {
                    if (!ActorExtra[s->owner].dispicnum)
                        t->picnum = ActorExtra[i].temp_data[1];
                    else t->picnum = ActorExtra[s->owner].dispicnum;
                    t->pal = sprite[s->owner].pal;
                    t->shade = sprite[s->owner].shade;
                    t->ang = sprite[s->owner].ang;
                    t->cstat = 2|sprite[s->owner].cstat;
                }
            }
            break;

        case CAMERA1__STATIC:
        case RAT__STATIC:

#if defined(POLYMOST) && defined(USE_OPENGL)
            if (getrendermode() >= 3 && usemodels && md_tilehasmodel(s->picnum,s->pal) >= 0 && !(spriteext[i].flags&SPREXT_NOTMD))
            {
                t->cstat &= ~4;
                break;
            }
#endif

            k = (((t->ang+3072+128-a)&2047)>>8)&7;
            if (k>4)
            {
                k = 8-k;
                t->cstat |= 4;
            }
            else t->cstat &= ~4;
            t->picnum = s->picnum+k;
            break;
        }

        ActorExtra[i].dispicnum = t->picnum;
        if (sector[t->sectnum].floorpicnum == MIRROR)
            t->xrepeat = t->yrepeat = 0;
    }

    if (apScriptGameEvent[EVENT_ANIMATESPRITES])
    {
        j = spritesortcnt-1;
        do
        {
            if (display_mirror) tsprite[j].statnum = TSPR_MIRROR;
            if (tsprite[j].owner < MAXSPRITES && tsprite[j].owner >= 0 && spriteext[tsprite[j].owner].flags & SPREXT_TSPRACCESS)
            {
                spriteext[tsprite[j].owner].tspr = (spritetype *)&tsprite[j];
                X_OnEvent(EVENT_ANIMATESPRITES,tsprite[j].owner, myconnectindex, -1);
            }
        }
        while (j--);

        if (j < 0) return;

        if (display_mirror) tsprite[j].statnum = TSPR_MIRROR;
        if (tsprite[j].owner >= 0 && tsprite[j].owner < MAXSPRITES && spriteext[tsprite[j].owner].flags & SPREXT_TSPRACCESS)
        {
            spriteext[tsprite[j].owner].tspr = (spritetype *)&tsprite[j];
            X_OnEvent(EVENT_ANIMATESPRITES,tsprite[j].owner, myconnectindex, -1);
        }
    }
}
#if 0 // def _MSC_VER
//#pragma auto_inline()
#pragma optimize("",on)
#endif

char CheatStrings[][MAXCHEATLEN] =
{
    "cornholio",    // 0
    "stuff",        // 1
    "scotty###",    // 2
    "coords",       // 3
    "view",         // 4
    "time",         // 5
    "unlock",       // 6
    "cashman",      // 7
    "items",        // 8
    "rate",         // 9
    "skill#",       // 10
    "beta",         // 11
    "hyper",        // 12
    "monsters",     // 13
    "<RESERVED>",   // 14
    "<RESERVED>",   // 15
    "todd",         // 16
    "showmap",      // 17
    "kroz",         // 18
    "allen",        // 19
    "clip",         // 20
    "weapons",      // 21
    "inventory",    // 22
    "keys",         // 23
    "debug",        // 24
    "<RESERVED>",   // 25
    "sfm",          // 26
};

enum cheats
{
    CHEAT_CORNHOLIO,
    CHEAT_STUFF,
    CHEAT_SCOTTY,
    CHEAT_COORDS,
    CHEAT_VIEW,
    CHEAT_TIME,
    CHEAT_UNLOCK,
    CHEAT_CASHMAN,
    CHEAT_ITEMS,
    CHEAT_RATE,
    CHEAT_SKILL,
    CHEAT_BETA,
    CHEAT_HYPER,
    CHEAT_MONSTERS,
    CHEAT_RESERVED,
    CHEAT_RESERVED2,
    CHEAT_TODD,
    CHEAT_SHOWMAP,
    CHEAT_KROZ,
    CHEAT_ALLEN,
    CHEAT_CLIP,
    CHEAT_WEAPONS,
    CHEAT_INVENTORY,
    CHEAT_KEYS,
    CHEAT_DEBUG,
    CHEAT_RESERVED3,
    CHEAT_SCREAMFORME,
};

void G_CheatGetInv(void)
{
    Gv_SetVar(g_iReturnVarID, 400, g_player[myconnectindex].ps->i, myconnectindex);
    X_OnEvent(EVENT_CHEATGETSTEROIDS, g_player[myconnectindex].ps->i, myconnectindex, -1);
    if (aGameVars[g_iReturnVarID].val.lValue >=0)
    {
        g_player[myconnectindex].ps->steroids_amount =
            aGameVars[g_iReturnVarID].val.lValue;
    }

    Gv_SetVar(g_iReturnVarID, 1200, g_player[myconnectindex].ps->i, myconnectindex);
    X_OnEvent(EVENT_CHEATGETHEAT, g_player[myconnectindex].ps->i, myconnectindex, -1);
    if (aGameVars[g_iReturnVarID].val.lValue >=0)
    {
        g_player[myconnectindex].ps->heat_amount     =
            aGameVars[g_iReturnVarID].val.lValue;
    }

    Gv_SetVar(g_iReturnVarID, 200, g_player[myconnectindex].ps->i, myconnectindex);
    X_OnEvent(EVENT_CHEATGETBOOT, g_player[myconnectindex].ps->i, myconnectindex, -1);
    if (aGameVars[g_iReturnVarID].val.lValue >=0)
    {
        g_player[myconnectindex].ps->boot_amount          =
            aGameVars[g_iReturnVarID].val.lValue;
    }

    Gv_SetVar(g_iReturnVarID, 100, g_player[myconnectindex].ps->i, myconnectindex);
    X_OnEvent(EVENT_CHEATGETSHIELD, g_player[myconnectindex].ps->i, myconnectindex, -1);
    if (aGameVars[g_iReturnVarID].val.lValue >=0)
    {
        g_player[myconnectindex].ps->shield_amount =
            aGameVars[g_iReturnVarID].val.lValue;
    }

    Gv_SetVar(g_iReturnVarID, 6400, g_player[myconnectindex].ps->i, myconnectindex);
    X_OnEvent(EVENT_CHEATGETSCUBA, g_player[myconnectindex].ps->i, myconnectindex, -1);
    if (aGameVars[g_iReturnVarID].val.lValue >=0)
    {
        g_player[myconnectindex].ps->scuba_amount =
            aGameVars[g_iReturnVarID].val.lValue;
    }

    Gv_SetVar(g_iReturnVarID, 2400, g_player[myconnectindex].ps->i, myconnectindex);
    X_OnEvent(EVENT_CHEATGETHOLODUKE, g_player[myconnectindex].ps->i, myconnectindex, -1);
    if (aGameVars[g_iReturnVarID].val.lValue >=0)
    {
        g_player[myconnectindex].ps->holoduke_amount =
            aGameVars[g_iReturnVarID].val.lValue;
    }

    Gv_SetVar(g_iReturnVarID, 1600, g_player[myconnectindex].ps->i, myconnectindex);
    X_OnEvent(EVENT_CHEATGETJETPACK, g_player[myconnectindex].ps->i, myconnectindex, -1);
    if (aGameVars[g_iReturnVarID].val.lValue >=0)
    {
        g_player[myconnectindex].ps->jetpack_amount =
            aGameVars[g_iReturnVarID].val.lValue;
    }

    Gv_SetVar(g_iReturnVarID, g_player[myconnectindex].ps->max_player_health, g_player[myconnectindex].ps->i, myconnectindex);
    X_OnEvent(EVENT_CHEATGETFIRSTAID, g_player[myconnectindex].ps->i, myconnectindex, -1);
    if (aGameVars[g_iReturnVarID].val.lValue >=0)
    {
        g_player[myconnectindex].ps->firstaid_amount =
            aGameVars[g_iReturnVarID].val.lValue;
    }
}

int8_t cheatbuf[MAXCHEATLEN],cheatbuflen;

static void G_DoCheats(void)
{
    int32_t ch, i, j, k=0, weapon;
    static int32_t z=0;
    char consolecheat = 0;  // JBF 20030914

    if (osdcmd_cheatsinfo_stat.cheatnum != -1)
    {
        // JBF 20030914
        k = osdcmd_cheatsinfo_stat.cheatnum;
        osdcmd_cheatsinfo_stat.cheatnum = -1;
        consolecheat = 1;
    }

    if ((g_player[myconnectindex].ps->gm&MODE_TYPE) || (g_player[myconnectindex].ps->gm&MODE_MENU))
        return;

    if (VOLUMEONE && !z)
    {
        Bstrcpy(CheatStrings[2],"scotty##");
        Bstrcpy(CheatStrings[6],"<RESERVED>");
        z=1;
    }

    if (consolecheat && numplayers < 2 && ud.recstat == 0)
        goto FOUNDCHEAT;

    if (g_player[myconnectindex].ps->cheat_phase == 1)
    {
        while (KB_KeyWaiting())
        {
            ch = Btolower(KB_Getch());

            if (!((ch >= 'a' && ch <= 'z') || (ch >= '0' && ch <= '9')))
            {
                g_player[myconnectindex].ps->cheat_phase = 0;
                //                             P_DoQuote(46,g_player[myconnectindex].ps);
                return;
            }

            cheatbuf[cheatbuflen++] = (int8_t)ch;
            cheatbuf[cheatbuflen] = 0;
            //            KB_ClearKeysDown();

            if (cheatbuflen > MAXCHEATLEN)
            {
                g_player[myconnectindex].ps->cheat_phase = 0;
                return;
            }

            for (k=0; k < NUMCHEATCODES; k++)
            {
                for (j = 0; j<cheatbuflen; j++)
                {
                    if (cheatbuf[j] == CheatStrings[k][j] || (CheatStrings[k][j] == '#' && ch >= '0' && ch <= '9'))
                    {
                        if (CheatStrings[k][j+1] == 0) goto FOUNDCHEAT;
                        if (j == cheatbuflen-1) return;
                    }
                    else break;
                }
            }

            g_player[myconnectindex].ps->cheat_phase = 0;
            return;

FOUNDCHEAT:
            {
                switch (k)
                {
                case CHEAT_WEAPONS:

                    j = 0;

                    if (VOLUMEONE)
                        j = 6;

                    for (weapon = PISTOL_WEAPON; weapon < MAX_WEAPONS-j; weapon++)
                    {
                        P_AddAmmo(weapon, g_player[myconnectindex].ps, g_player[myconnectindex].ps->max_ammo_amount[weapon]);
                        g_player[myconnectindex].ps->gotweapon[weapon]  = 1;
                    }

                    KB_FlushKeyBoardQueue();
                    g_player[myconnectindex].ps->cheat_phase = 0;
                    P_DoQuote(119,g_player[myconnectindex].ps);
                    return;

                case CHEAT_INVENTORY:
                    KB_FlushKeyBoardQueue();
                    g_player[myconnectindex].ps->cheat_phase = 0;
                    G_CheatGetInv();
                    P_DoQuote(120,g_player[myconnectindex].ps);
                    g_player[myconnectindex].ps->cheat_phase = 0;
                    return;

                case CHEAT_KEYS:
                    g_player[myconnectindex].ps->got_access =  7;
                    KB_FlushKeyBoardQueue();
                    g_player[myconnectindex].ps->cheat_phase = 0;
                    P_DoQuote(121,g_player[myconnectindex].ps);
                    return;

                case CHEAT_DEBUG:
                    debug_on = 1-debug_on;
                    KB_FlushKeyBoardQueue();
                    g_player[myconnectindex].ps->cheat_phase = 0;

                    G_DumpDebugInfo();
                    Bsprintf(tempbuf,"GAMEVARS DUMPED TO LOG");
                    G_AddUserQuote(tempbuf);
                    Bsprintf(tempbuf,"MAP DUMPED TO DEBUG.MAP");
                    G_AddUserQuote(tempbuf);
                    break;

                case CHEAT_CLIP:
                    ud.clipping = 1-ud.clipping;
                    KB_FlushKeyBoardQueue();
                    g_player[myconnectindex].ps->cheat_phase = 0;
                    P_DoQuote(112+ud.clipping,g_player[myconnectindex].ps);
                    return;

                case CHEAT_RESERVED2:
                    g_player[myconnectindex].ps->gm = MODE_EOL;
                    g_player[myconnectindex].ps->cheat_phase = 0;
                    KB_FlushKeyBoardQueue();
                    return;

                case CHEAT_ALLEN:
                    P_DoQuote(79,g_player[myconnectindex].ps);
                    g_player[myconnectindex].ps->cheat_phase = 0;
                    KB_ClearKeyDown(sc_N);
                    return;

                case CHEAT_CORNHOLIO:
                case CHEAT_KROZ:
                    ud.god = 1-ud.god;

                    if (ud.god)
                    {
                        pus = 1;
                        pub = 1;
                        sprite[g_player[myconnectindex].ps->i].cstat = 257;

                        ActorExtra[g_player[myconnectindex].ps->i].temp_data[0] = 0;
                        ActorExtra[g_player[myconnectindex].ps->i].temp_data[1] = 0;
                        ActorExtra[g_player[myconnectindex].ps->i].temp_data[2] = 0;
                        ActorExtra[g_player[myconnectindex].ps->i].temp_data[3] = 0;
                        ActorExtra[g_player[myconnectindex].ps->i].temp_data[4] = 0;
                        ActorExtra[g_player[myconnectindex].ps->i].temp_data[5] = 0;

                        sprite[g_player[myconnectindex].ps->i].hitag = 0;
                        sprite[g_player[myconnectindex].ps->i].lotag = 0;
                        sprite[g_player[myconnectindex].ps->i].pal = g_player[myconnectindex].ps->palookup;

                        P_DoQuote(17,g_player[myconnectindex].ps);
                    }
                    else
                    {
                        ud.god = 0;
                        sprite[g_player[myconnectindex].ps->i].extra = g_player[myconnectindex].ps->max_player_health;
                        ActorExtra[g_player[myconnectindex].ps->i].extra = -1;
                        g_player[myconnectindex].ps->last_extra = g_player[myconnectindex].ps->max_player_health;
                        P_DoQuote(18,g_player[myconnectindex].ps);
                    }

                    sprite[g_player[myconnectindex].ps->i].extra = g_player[myconnectindex].ps->max_player_health;
                    ActorExtra[g_player[myconnectindex].ps->i].extra = 0;
                    g_player[myconnectindex].ps->cheat_phase = 0;
                    KB_FlushKeyBoardQueue();
                    return;

                case CHEAT_SCREAMFORME:
                    ud.god = 1-ud.god;

                    if (ud.god)
                    {
                        pus = 1;
                        pub = 1;
                        sprite[g_player[myconnectindex].ps->i].cstat = 257;

                        ActorExtra[g_player[myconnectindex].ps->i].temp_data[0] = 0;
                        ActorExtra[g_player[myconnectindex].ps->i].temp_data[1] = 0;
                        ActorExtra[g_player[myconnectindex].ps->i].temp_data[2] = 0;
                        ActorExtra[g_player[myconnectindex].ps->i].temp_data[3] = 0;
                        ActorExtra[g_player[myconnectindex].ps->i].temp_data[4] = 0;
                        ActorExtra[g_player[myconnectindex].ps->i].temp_data[5] = 0;

                        sprite[g_player[myconnectindex].ps->i].hitag = 0;
                        sprite[g_player[myconnectindex].ps->i].lotag = 0;
                        sprite[g_player[myconnectindex].ps->i].pal = g_player[myconnectindex].ps->palookup;
                        Bstrcpy(ScriptQuotes[122],"Scream for me, Long Beach!");
                        P_DoQuote(122,g_player[myconnectindex].ps);
                        G_CheatGetInv();
                        for (weapon = PISTOL_WEAPON; weapon < MAX_WEAPONS; weapon++)
                            g_player[myconnectindex].ps->gotweapon[weapon]  = 1;

                        for (weapon = PISTOL_WEAPON;
                                weapon < (MAX_WEAPONS);
                                weapon++)
                            P_AddAmmo(weapon, g_player[myconnectindex].ps, g_player[myconnectindex].ps->max_ammo_amount[weapon]);
                        g_player[myconnectindex].ps->got_access = 7;
                    }
                    else
                    {
                        sprite[g_player[myconnectindex].ps->i].extra = g_player[myconnectindex].ps->max_player_health;
                        ActorExtra[g_player[myconnectindex].ps->i].extra = -1;
                        g_player[myconnectindex].ps->last_extra = g_player[myconnectindex].ps->max_player_health;
                        P_DoQuote(18,g_player[myconnectindex].ps);
                    }

                    sprite[g_player[myconnectindex].ps->i].extra = g_player[myconnectindex].ps->max_player_health;
                    ActorExtra[g_player[myconnectindex].ps->i].extra = 0;
                    g_player[myconnectindex].ps->cheat_phase = 0;
                    KB_FlushKeyBoardQueue();
                    return;

                case CHEAT_STUFF:

                    j = 0;

                    if (VOLUMEONE)
                        j = 6;

                    for (weapon = PISTOL_WEAPON; weapon < MAX_WEAPONS-j; weapon++)
                        g_player[myconnectindex].ps->gotweapon[weapon]  = 1;

                    for (weapon = PISTOL_WEAPON;
                            weapon < (MAX_WEAPONS-j);
                            weapon++)
                        P_AddAmmo(weapon, g_player[myconnectindex].ps, g_player[myconnectindex].ps->max_ammo_amount[weapon]);
                    G_CheatGetInv();
                    g_player[myconnectindex].ps->got_access =              7;
                    P_DoQuote(5,g_player[myconnectindex].ps);
                    g_player[myconnectindex].ps->cheat_phase = 0;

                    //                        P_DoQuote(21,g_player[myconnectindex].ps);
                    g_player[myconnectindex].ps->cheat_phase = 0;
                    KB_FlushKeyBoardQueue();
                    g_player[myconnectindex].ps->inven_icon = 1;
                    return;

                case CHEAT_SCOTTY:
                case CHEAT_SKILL:
                    if (k == CHEAT_SCOTTY)
                    {
                        i = Bstrlen(CheatStrings[k])-3+VOLUMEONE;
                        if (!consolecheat)
                        {
                            // JBF 20030914
                            int16_t volnume,levnume;
                            if (VOLUMEALL)
                            {
                                volnume = cheatbuf[i] - '0';
                                levnume = (cheatbuf[i+1] - '0')*10+(cheatbuf[i+2]-'0');
                            }
                            else
                            {
                                volnume =  cheatbuf[i] - '0';
                                levnume =  cheatbuf[i+1] - '0';
                            }

                            volnume--;
                            levnume--;

                            if ((VOLUMEONE && volnume > 0) || volnume > g_numVolumes-1 ||
                                    levnume >= MAXLEVELS || MapInfo[volnume*MAXLEVELS+levnume].filename == NULL)
                            {
                                g_player[myconnectindex].ps->cheat_phase = 0;
                                KB_FlushKeyBoardQueue();
                                return;
                            }

                            ud.m_volume_number = ud.volume_number = volnume;
                            ud.m_level_number = ud.level_number = levnume;
                        }
                        else
                        {
                            // JBF 20030914
                            ud.m_volume_number = ud.volume_number = osdcmd_cheatsinfo_stat.volume;
                            ud.m_level_number = ud.level_number = osdcmd_cheatsinfo_stat.level;
                        }
                    }
                    else
                    {
                        i = Bstrlen(CheatStrings[k])-1;
                        ud.m_player_skill = ud.player_skill = cheatbuf[i] - '1';
                    }
                    if (numplayers > 1 && myconnectindex == connecthead)
                        Net_NewGame(ud.m_volume_number,ud.m_level_number);
                    else g_player[myconnectindex].ps->gm |= MODE_RESTART;

                    g_player[myconnectindex].ps->cheat_phase = 0;
                    KB_FlushKeyBoardQueue();
                    return;

                case CHEAT_COORDS:
                    g_player[myconnectindex].ps->cheat_phase = 0;
                    ud.coords = 1-ud.coords;
                    KB_FlushKeyBoardQueue();
                    return;

                case CHEAT_VIEW:
                    if (g_player[myconnectindex].ps->over_shoulder_on)
                        g_player[myconnectindex].ps->over_shoulder_on = 0;
                    else
                    {
                        g_player[myconnectindex].ps->over_shoulder_on = 1;
                        g_cameraDistance = 0;
                        g_cameraClock = totalclock;
                    }
                    P_DoQuote(22,g_player[myconnectindex].ps);
                    g_player[myconnectindex].ps->cheat_phase = 0;
                    KB_FlushKeyBoardQueue();
                    return;

                case CHEAT_TIME:

                    P_DoQuote(21,g_player[myconnectindex].ps);
                    g_player[myconnectindex].ps->cheat_phase = 0;
                    KB_FlushKeyBoardQueue();
                    return;

                case CHEAT_UNLOCK:
                    if (VOLUMEONE) return;

                    for (i=numsectors-1; i>=0; i--) //Unlock
                    {
                        j = sector[i].lotag;
                        if (j == -1 || j == 32767) continue;
                        if ((j & 0x7fff) > 2)
                        {
                            if (j&(0xffff-16384))
                                sector[i].lotag &= (0xffff-16384);
                            G_OperateSectors(i,g_player[myconnectindex].ps->i);
                        }
                    }
                    G_OperateForceFields(g_player[myconnectindex].ps->i,-1);

                    P_DoQuote(100,g_player[myconnectindex].ps);
                    g_player[myconnectindex].ps->cheat_phase = 0;
                    KB_FlushKeyBoardQueue();
                    return;

                case CHEAT_CASHMAN:
                    ud.cashman = 1-ud.cashman;
                    KB_ClearKeyDown(sc_N);
                    g_player[myconnectindex].ps->cheat_phase = 0;
                    return;

                case CHEAT_ITEMS:
                    G_CheatGetInv();
                    g_player[myconnectindex].ps->got_access =              7;
                    P_DoQuote(5,g_player[myconnectindex].ps);
                    g_player[myconnectindex].ps->cheat_phase = 0;
                    KB_FlushKeyBoardQueue();
                    return;

                case CHEAT_SHOWMAP: // SHOW ALL OF THE MAP TOGGLE;
                    ud.showallmap = 1-ud.showallmap;
                    if (ud.showallmap)
                    {
                        for (i=0; i<(MAXSECTORS>>3); i++)
                            show2dsector[i] = 255;
                        for (i=0; i<(MAXWALLS>>3); i++)
                            show2dwall[i] = 255;
                        P_DoQuote(111,g_player[myconnectindex].ps);
                    }
                    else
                    {
                        for (i=0; i<(MAXSECTORS>>3); i++)
                            show2dsector[i] = 0;
                        for (i=0; i<(MAXWALLS>>3); i++)
                            show2dwall[i] = 0;
                        P_DoQuote(1,g_player[myconnectindex].ps);
                    }
                    g_player[myconnectindex].ps->cheat_phase = 0;
                    KB_FlushKeyBoardQueue();
                    return;

                case CHEAT_TODD:
                    P_DoQuote(99,g_player[myconnectindex].ps);
                    g_player[myconnectindex].ps->cheat_phase = 0;
                    KB_FlushKeyBoardQueue();
                    return;

                case CHEAT_RATE:
                    ud.tickrate = !ud.tickrate;
                    g_player[myconnectindex].ps->cheat_phase = 0;
                    KB_FlushKeyBoardQueue();
                    return;

                case CHEAT_BETA:
                    P_DoQuote(105,g_player[myconnectindex].ps);
                    KB_ClearKeyDown(sc_H);
                    g_player[myconnectindex].ps->cheat_phase = 0;
                    KB_FlushKeyBoardQueue();
                    return;

                case CHEAT_HYPER:
                    g_player[myconnectindex].ps->steroids_amount = 399;
                    g_player[myconnectindex].ps->heat_amount = 1200;
                    g_player[myconnectindex].ps->cheat_phase = 0;
                    P_DoQuote(37,g_player[myconnectindex].ps);
                    KB_FlushKeyBoardQueue();
                    return;

                case CHEAT_MONSTERS:
                {
                    char *s[] = { "ON", "OFF", "ON" };

                    g_noEnemies++;
                    if (g_noEnemies == 3) g_noEnemies = 0;
                    g_player[screenpeek].ps->cheat_phase = 0;
                    Bsprintf(ScriptQuotes[122],"MONSTERS: %s",s[(uint8_t)g_noEnemies]);
                    P_DoQuote(122,g_player[myconnectindex].ps);
                    KB_FlushKeyBoardQueue();
                    return;
                }
                case CHEAT_RESERVED:
                case CHEAT_RESERVED3:
                    ud.eog = 1;
                    g_player[myconnectindex].ps->gm |= MODE_EOL;
                    KB_FlushKeyBoardQueue();
                    return;
                }
            }
        }
    }
    else
    {
        if (KB_KeyPressed((uint8_t)CheatKeys[0]))
        {
            if (g_player[myconnectindex].ps->cheat_phase >= 0 && numplayers < 2 && ud.recstat == 0)
            {
                if (CheatKeys[0] == CheatKeys[1])
                    KB_ClearKeyDown((uint8_t)CheatKeys[0]);
                g_player[myconnectindex].ps->cheat_phase = -1;
            }
        }

        if (KB_KeyPressed((uint8_t)CheatKeys[1]))
        {
            if (g_player[myconnectindex].ps->cheat_phase == -1)
            {
                if (ud.player_skill == 4)
                {
                    P_DoQuote(22,g_player[myconnectindex].ps);
                    g_player[myconnectindex].ps->cheat_phase = 0;
                }
                else
                {
                    g_player[myconnectindex].ps->cheat_phase = 1;
                    //                    P_DoQuote(25,g_player[myconnectindex].ps);
                    cheatbuflen = 0;
                }
                KB_FlushKeyboardQueue();
            }
            else if (g_player[myconnectindex].ps->cheat_phase != 0)
            {
                g_player[myconnectindex].ps->cheat_phase = 0;
                KB_ClearKeyDown((uint8_t)CheatKeys[0]);
                KB_ClearKeyDown((uint8_t)CheatKeys[1]);
            }
        }
    }
}

#define SCORESHEETOFFSET -20

static void G_ShowScores(void)
{
    int32_t t, i, y,xfragtotal,yfragtotal;

    if (playerswhenstarted > 1 && (GametypeFlags[ud.coop]&GAMETYPE_SCORESHEET))
    {
        /*
                rotatesprite(160<<16,34<<16,65536L,0,INGAMEDUKETHREEDEE,0,0,10,0,0,xdim-1,ydim-1);
                if (PLUTOPAK)   // JBF 20030804
                    rotatesprite((260)<<16,36<<16,65536L,0,PLUTOPAKSPRITE+2,0,0,2+8,0,0,xdim-1,ydim-1);
        */
        gametext(160,SCORESHEETOFFSET+58+2,"MULTIPLAYER TOTALS",0,2+8+16);
        gametext(160,SCORESHEETOFFSET+58+10,MapInfo[(ud.volume_number*MAXLEVELS)+ud.last_level-1].name,0,2+8+16);

//        gametext(160,165,"PRESS ANY KEY TO CONTINUE",0,2+8+16);

        t = 0;
        minitext(23,SCORESHEETOFFSET+80,"   NAME                                           KILLS",8,2+8+16+128);
        for (i=playerswhenstarted-1; i>=0; i--)
        {
            Bsprintf(tempbuf,"%-4d",i+1);
            minitext(92+(i*23),SCORESHEETOFFSET+80,tempbuf,3,2+8+16+128);
        }

        for (i=playerswhenstarted-1; i>=0; i--)
        {
            xfragtotal = 0;
            Bsprintf(tempbuf,"%d",i+1);

            minitext(30,SCORESHEETOFFSET+90+t,tempbuf,0,2+8+16+128);
            minitext(38,SCORESHEETOFFSET+90+t,g_player[i].user_name,g_player[i].ps->palookup,2+8+16+128);

            for (y=playerswhenstarted-1; y>=0; y--)
            {
                if (i == y)
                {
                    Bsprintf(tempbuf,"%-4d",g_player[y].ps->fraggedself);
                    minitext(92+(y*23),SCORESHEETOFFSET+90+t,tempbuf,2,2+8+16+128);
                    xfragtotal -= g_player[y].ps->fraggedself;
                }
                else
                {
                    Bsprintf(tempbuf,"%-4d",g_player[i].frags[y]);
                    minitext(92+(y*23),SCORESHEETOFFSET+90+t,tempbuf,0,2+8+16+128);
                    xfragtotal += g_player[i].frags[y];
                }

                if (myconnectindex == connecthead)
                {
                    Bsprintf(tempbuf,"stats %d killed %d %d\n",i+1,y+1,g_player[i].frags[y]);
                    sendscore(tempbuf);
                }
            }

            Bsprintf(tempbuf,"%-4d",xfragtotal);
            minitext(101+(8*23),SCORESHEETOFFSET+90+t,tempbuf,2,2+8+16+128);

            t += 7;
        }

        for (y=playerswhenstarted-1; y>=0; y--)
        {
            yfragtotal = 0;
            for (i=playerswhenstarted-1; i>=0; i--)
            {
                if (i == y)
                    yfragtotal += g_player[i].ps->fraggedself;
                yfragtotal += g_player[i].frags[y];
            }
            Bsprintf(tempbuf,"%-4d",yfragtotal);
            minitext(92+(y*23),SCORESHEETOFFSET+96+(8*7),tempbuf,2,2+8+16+128);
        }

        minitext(45,SCORESHEETOFFSET+96+(8*7),"DEATHS",8,2+8+16+128);
    }
}

#undef SCORESHEETOFFSET

static void G_HandleLocalKeys(void)
{
    int32_t i,ch;
    int32_t j;

    CONTROL_ProcessBinds();

    if (ud.recstat == 2)
    {
        ControlInfo noshareinfo;
        CONTROL_GetInput(&noshareinfo);
    }

    if (g_player[myconnectindex].gotvote == 0 && voting != -1 && voting != myconnectindex)
    {
        if (KB_UnBoundKeyPressed(sc_F1) || KB_UnBoundKeyPressed(sc_F2) || ud.autovote)
        {
            tempbuf[0] = PACKET_MAP_VOTE;
            tempbuf[1] = myconnectindex;
            tempbuf[2] = (KB_UnBoundKeyPressed(sc_F1) || ud.autovote ? ud.autovote-1 : 0);

            TRAVERSE_CONNECT(i)
            {
                if (i != myconnectindex) mmulti_sendpacket(i,tempbuf,3);
                if ((!g_networkBroadcastMode) && (myconnectindex != connecthead)) break; //slaves in M/S mode only send to master
            }
            G_AddUserQuote("VOTE CAST");
            g_player[myconnectindex].gotvote = 1;
            KB_ClearKeyDown(sc_F1);
            KB_ClearKeyDown(sc_F2);
            voting = -1;
        }
    }

    if (!ALT_IS_PRESSED && ud.overhead_on == 0 && (g_player[myconnectindex].ps->gm & MODE_TYPE) == 0)
    {
        if (BUTTON(gamefunc_Enlarge_Screen))
        {
            CONTROL_ClearButton(gamefunc_Enlarge_Screen);
            if (!SHIFTS_IS_PRESSED)
            {
                if (ud.screen_size > 0)
                    S_PlaySound(THUD);
                if (getrendermode() >= 3 && ud.screen_size == 8 && ud.statusbarmode == 0)
                    ud.statusbarmode = 1;
                else ud.screen_size -= 4;

                if (ud.statusbarscale == 100 && ud.statusbarmode == 1)
                {
                    ud.statusbarmode = 0;
                    ud.screen_size -= 4;
                }
            }
            else
            {
                ud.statusbarscale += 4;
                G_SetStatusBarScale(ud.statusbarscale);
            }
            G_UpdateScreenArea();
        }

        if (BUTTON(gamefunc_Shrink_Screen))
        {
            CONTROL_ClearButton(gamefunc_Shrink_Screen);
            if (!SHIFTS_IS_PRESSED)
            {
                if (ud.screen_size < 64) S_PlaySound(THUD);
                if (getrendermode() >= 3 && ud.screen_size == 8 && ud.statusbarmode == 1)
                    ud.statusbarmode = 0;
                else ud.screen_size += 4;
            }
            else
            {
                ud.statusbarscale -= 4;
                if (ud.statusbarscale < 37)
                    ud.statusbarscale = 37;
                G_SetStatusBarScale(ud.statusbarscale);
            }
            G_UpdateScreenArea();
        }
    }

    if (g_player[myconnectindex].ps->cheat_phase == 1 || (g_player[myconnectindex].ps->gm&(MODE_MENU|MODE_TYPE))) return;

    if (BUTTON(gamefunc_See_Coop_View) && (GTFLAGS(GAMETYPE_COOPVIEW) || ud.recstat == 2))
    {
        CONTROL_ClearButton(gamefunc_See_Coop_View);
        screenpeek = connectpoint2[screenpeek];
        if (screenpeek == -1) screenpeek = connecthead;
        g_restorePalette = 1;
    }

    if (ud.multimode > 1 && BUTTON(gamefunc_Show_Opponents_Weapon))
    {
        CONTROL_ClearButton(gamefunc_Show_Opponents_Weapon);
        ud.config.ShowOpponentWeapons = ud.showweapons = 1-ud.showweapons;
        P_DoQuote(82-ud.showweapons,g_player[screenpeek].ps);
    }

    if (BUTTON(gamefunc_Toggle_Crosshair))
    {
        CONTROL_ClearButton(gamefunc_Toggle_Crosshair);
        ud.crosshair = !ud.crosshair;
        P_DoQuote(21-ud.crosshair,g_player[screenpeek].ps);
    }

    if (ud.overhead_on && BUTTON(gamefunc_Map_Follow_Mode))
    {
        CONTROL_ClearButton(gamefunc_Map_Follow_Mode);
        ud.scrollmode = 1-ud.scrollmode;
        if (ud.scrollmode)
        {
            ud.folx = g_player[screenpeek].ps->oposx;
            ud.foly = g_player[screenpeek].ps->oposy;
            ud.fola = g_player[screenpeek].ps->oang;
        }
        P_DoQuote(83+ud.scrollmode,g_player[myconnectindex].ps);
    }

    if (ud.recstat == 2)
    {
        if (KB_KeyPressed(sc_Space))
        {
            KB_ClearKeyDown(sc_Space);
            demo_paused = !demo_paused;
        }

        if (KB_KeyPressed(sc_Tab))
        {
            KB_ClearKeyDown(sc_Tab);
            demo_showstats = !demo_showstats;
        }

        if (KB_KeyPressed(sc_kpad_Plus))
        {
            if (g_timerTicsPerSecond != 240)
            {
                uninittimer();
                inittimer(240);
                g_timerTicsPerSecond = 240;
            }
        }
        else if (KB_KeyPressed(sc_kpad_Minus))
        {
            if (g_timerTicsPerSecond != 60)
            {
                uninittimer();
                inittimer(60);
                g_timerTicsPerSecond = 60;
            }
        }
        else if (g_timerTicsPerSecond != 120)
        {
            uninittimer();
            inittimer(120);
            g_timerTicsPerSecond = 120;
        }

        if (KB_KeyPressed(sc_kpad_6))
        {
            KB_ClearKeyDown(sc_kpad_6);
            j = ALT_IS_PRESSED ? 30 : 10;
            demo_goalreccnt = demo_paused ? ud.reccnt-ud.multimode : ud.reccnt-(TICRATE/TICSPERFRAME)*ud.multimode*j;
            demo_oldsoundtoggle = ud.config.SoundToggle;

//            j=(demo_startreccnt-ud.reccnt)/(ud.multimode*(TICRATE/TICSPERFRAME));
//            OSD_Printf("  FF %d s from %02d:%02d\n", ALT_IS_PRESSED ? 30 : 10, j/60, j%60);

            if (demo_goalreccnt <= 0)
                demo_goalreccnt = 0;
            else
            {
                ud.config.SoundToggle = 0;
                FX_StopAllSounds();
                S_ClearSoundLocks();
            }
        
        }
        else if (KB_KeyPressed(sc_kpad_4))
        {
            KB_ClearKeyDown(sc_kpad_4);
            j = ALT_IS_PRESSED ? 30 : 10;
            demo_goalreccnt = demo_paused ? ud.reccnt+ud.multimode : ud.reccnt+(TICRATE/TICSPERFRAME)*ud.multimode*j;
            demo_oldsoundtoggle = ud.config.SoundToggle;

//            j=(demo_startreccnt-ud.reccnt)/(ud.multimode*TICRATE/TICSPERFRAME);
//            OSD_Printf("  RW %d s from %02d:%02d\n", ALT_IS_PRESSED ? 30 : 10, j/60, j%60);

            if (demo_goalreccnt > demo_startreccnt)
                demo_goalreccnt = demo_startreccnt;

            demo_goalreccnt = -demo_goalreccnt;
            ud.config.SoundToggle = 0;
            FX_StopAllSounds();
            S_ClearSoundLocks();
        }

#if 0
// just what is wrong with that?
        if (KB_KeyPressed(sc_Return) && ud.multimode==1)
        {
            KB_ClearKeyDown(sc_Return);
            ud.reccnt = 0;
            ud.recstat = 0;
//            kclose(recfilep);
            g_player[myconnectindex].ps->gm = MODE_GAME;
            ready2send=0;
            screenpeek=myconnectindex;
            demo_paused=0;
        }
#endif
    }

    if (SHIFTS_IS_PRESSED || ALT_IS_PRESSED)
    {
        i = 0;
        j = sc_F1;

        do 
        {
            if (KB_UnBoundKeyPressed(j))
            {
                KB_ClearKeyDown(j);
                i = j - sc_F1 + 1;
            }
        }
        while (++j < sc_F11);
        
        if (i)
        {
            if (SHIFTS_IS_PRESSED)
            {
                if (i == 5 && g_player[myconnectindex].ps->fta > 0 && g_player[myconnectindex].ps->ftq == 26)
                {
                    i = (VOLUMEALL?MAXVOLUMES*MAXLEVELS:6);
                    g_musicIndex = (g_musicIndex+1)%i;
                    while (MapInfo[(uint8_t)g_musicIndex].musicfn == NULL)
                    {
                        g_musicIndex++;
                        if (g_musicIndex >= i)
                            g_musicIndex = 0;
                    }
                    if (MapInfo[(uint8_t)g_musicIndex].musicfn != NULL)
                    {
                        if (S_PlayMusic(&MapInfo[(uint8_t)g_musicIndex].musicfn[0],g_musicIndex))
                            Bsprintf(ScriptQuotes[26],"PLAYING %s",&MapInfo[(uint8_t)g_musicIndex].alt_musicfn[0]);
                        else
                            Bsprintf(ScriptQuotes[26],"PLAYING %s",&MapInfo[(uint8_t)g_musicIndex].musicfn[0]);
                        P_DoQuote(26,g_player[myconnectindex].ps);
                    }
                    return;
                }

                G_AddUserQuote(ud.ridecule[i-1]);

                ch = 0;

                tempbuf[ch] = PACKET_MESSAGE;
                tempbuf[ch+1] = 255;
                tempbuf[ch+2] = 0;
                Bstrcat(tempbuf+2,ud.ridecule[i-1]);

                i = 2+strlen(ud.ridecule[i-1]);

                if (ud.multimode > 1)
                    TRAVERSE_CONNECT(ch)
                {
                    if (ch != myconnectindex) mmulti_sendpacket(ch,tempbuf,i);
                    if ((!g_networkBroadcastMode) && (myconnectindex != connecthead)) break; //slaves in M/S mode only send to master
                }

                pus = NUMPAGES;
                pub = NUMPAGES;

                return;

            }

            if (ud.lockout == 0)
                if (ud.config.SoundToggle && ALT_IS_PRESSED && (RTS_NumSounds() > 0) && g_RTSPlaying == 0 && (ud.config.VoiceToggle & 1))
                {
                    rtsptr = (char *)RTS_GetSound(i-1);
                    FX_PlayAuto3D(rtsptr,RTS_SoundLength(i-1),0,0,0,255,-i);

                    g_RTSPlaying = 7;

                    if (ud.multimode > 1)
                    {
                        tempbuf[0] = PACKET_RTS;
                        tempbuf[1] = i;

                        TRAVERSE_CONNECT(ch)
                        {
                            if (ch != myconnectindex) mmulti_sendpacket(ch,tempbuf,2);
                            if ((!g_networkBroadcastMode) && (myconnectindex != connecthead)) break; //slaves in M/S mode only send to master
                        }
                    }

                    pus = NUMPAGES;
                    pub = NUMPAGES;

                    return;
                }
        }
    }

    if (!ALT_IS_PRESSED && !SHIFTS_IS_PRESSED)
    {

        if (ud.multimode > 1 && BUTTON(gamefunc_SendMessage))
        {
            KB_FlushKeyboardQueue();
            CONTROL_ClearButton(gamefunc_SendMessage);
            g_player[myconnectindex].ps->gm |= MODE_TYPE;
            typebuf[0] = 0;
            inputloc = 0;
        }

        if (KB_UnBoundKeyPressed(sc_F1) || (ud.show_help && (KB_KeyPressed(sc_Space) || KB_KeyPressed(sc_Enter) || KB_KeyPressed(sc_kpad_Enter) || MOUSE_GetButtons()&LEFT_MOUSE)))
        {
            KB_ClearKeyDown(sc_F1);
            KB_ClearKeyDown(sc_Space);
            KB_ClearKeyDown(sc_kpad_Enter);
            KB_ClearKeyDown(sc_Enter);
            MOUSE_ClearButton(LEFT_MOUSE);
            ud.show_help ++;

            if (ud.show_help > 2)
            {
                ud.show_help = 0;
                if (ud.multimode < 2 && ud.recstat != 2) ready2send = 1;
                G_UpdateScreenArea();
            }
            else
            {
                setview(0,0,xdim-1,ydim-1);
                if (ud.multimode < 2 && ud.recstat != 2)
                {
                    ready2send = 0;
                    totalclock = ototalclock;
                }
            }
        }

        //        if(ud.multimode < 2)
        {
            if (ud.recstat != 2 && KB_UnBoundKeyPressed(sc_F2))
            {
                KB_ClearKeyDown(sc_F2);

                if (g_movesPerPacket == 4 && connecthead != myconnectindex)
                    return;

FAKE_F2:
                if (sprite[g_player[myconnectindex].ps->i].extra <= 0)
                {
                    P_DoQuote(118,g_player[myconnectindex].ps);
                    return;
                }
                ChangeToMenu(350);
                g_screenCapture = 1;
                G_DrawRooms(myconnectindex,65536);
                //savetemp("duke3d.tmp",waloff[TILE_SAVESHOT],160*100);
                g_screenCapture = 0;
                FX_StopAllSounds();
                S_ClearSoundLocks();

                //                setview(0,0,xdim-1,ydim-1);
                g_player[myconnectindex].ps->gm |= MODE_MENU;

                if (ud.multimode < 2)
                {
                    ready2send = 0;
                    totalclock = ototalclock;
                    screenpeek = myconnectindex;
                }
            }

            if (KB_UnBoundKeyPressed(sc_F3))
            {
                KB_ClearKeyDown(sc_F3);

                if (g_movesPerPacket == 4 && connecthead != myconnectindex)
                    return;
FAKE_F3:
                ChangeToMenu(300);
                FX_StopAllSounds();
                S_ClearSoundLocks();

                //                setview(0,0,xdim-1,ydim-1);
                g_player[myconnectindex].ps->gm |= MODE_MENU;
                if (ud.multimode < 2 && ud.recstat != 2)
                {
                    ready2send = 0;
                    totalclock = ototalclock;
                }
                screenpeek = myconnectindex;
            }
        }

        if (KB_UnBoundKeyPressed(sc_F4) && ud.config.FXDevice >= 0)
        {
            KB_ClearKeyDown(sc_F4);
            FX_StopAllSounds();
            S_ClearSoundLocks();

            g_player[myconnectindex].ps->gm |= MODE_MENU;
            if (ud.multimode < 2 && ud.recstat != 2)
            {
                ready2send = 0;
                totalclock = ototalclock;
            }
            ChangeToMenu(701);

        }

        if ((KB_UnBoundKeyPressed(sc_F6) || g_doQuickSave == 1) && (g_player[myconnectindex].ps->gm&MODE_GAME))
        {
            KB_ClearKeyDown(sc_F6);
            g_doQuickSave = 0;

            if (g_movesPerPacket == 4 && connecthead != myconnectindex)
                return;

            if (g_lastSaveSlot == -1) goto FAKE_F2;

            KB_FlushKeyboardQueue();

            if (sprite[g_player[myconnectindex].ps->i].extra <= 0)
            {
                P_DoQuote(118,g_player[myconnectindex].ps);
                return;
            }
            g_screenCapture = 1;
            G_DrawRooms(myconnectindex,65536);
            //savetemp("duke3d.tmp",waloff[TILE_SAVESHOT],160*100);
            g_screenCapture = 0;
            if (g_lastSaveSlot >= 0)
            {
                /*                inputloc = Bstrlen(&ud.savegame[g_lastSaveSlot][0]);
                                g_currentMenu = 360+g_lastSaveSlot;
                                probey = g_lastSaveSlot; */
                if (ud.multimode > 1)
                    G_SavePlayer(-1-(g_lastSaveSlot));
                else G_SavePlayer(g_lastSaveSlot);
            }
        }

        if (KB_UnBoundKeyPressed(sc_F7))
        {
            KB_ClearKeyDown(sc_F7);
            if (g_player[myconnectindex].ps->over_shoulder_on)
                g_player[myconnectindex].ps->over_shoulder_on = 0;
            else
            {
                g_player[myconnectindex].ps->over_shoulder_on = 1;
                g_cameraDistance = 0;
                g_cameraClock = totalclock;
            }
            P_DoQuote(109+g_player[myconnectindex].ps->over_shoulder_on,g_player[myconnectindex].ps);
        }

        if (KB_UnBoundKeyPressed(sc_F5) && ud.config.MusicDevice >= 0)
        {
            KB_ClearKeyDown(sc_F5);
            if (MapInfo[(uint8_t)g_musicIndex].alt_musicfn != NULL)
                Bstrcpy(ScriptQuotes[26],&MapInfo[(uint8_t)g_musicIndex].alt_musicfn[0]);
            else if (MapInfo[(uint8_t)g_musicIndex].musicfn != NULL)
            {
                Bstrcpy(ScriptQuotes[26],&MapInfo[(uint8_t)g_musicIndex].musicfn[0]);
                Bstrcat(ScriptQuotes[26],".  USE SHIFT-F5 TO CHANGE.");
            }
            else ScriptQuotes[26][0] = '\0';
            P_DoQuote(26,g_player[myconnectindex].ps);
        }

        if (KB_UnBoundKeyPressed(sc_F8))
        {
            KB_ClearKeyDown(sc_F8);
            ud.fta_on = !ud.fta_on;
            if (ud.fta_on) P_DoQuote(23,g_player[myconnectindex].ps);
            else
            {
                ud.fta_on = 1;
                P_DoQuote(24,g_player[myconnectindex].ps);
                ud.fta_on = 0;
            }
        }

        if ((KB_UnBoundKeyPressed(sc_F9) || g_doQuickSave == 2) && (g_player[myconnectindex].ps->gm&MODE_GAME))
        {
            KB_ClearKeyDown(sc_F9);
            g_doQuickSave = 0;

            if (g_movesPerPacket == 4 && myconnectindex != connecthead)
                return;

            if (g_lastSaveSlot == -1) goto FAKE_F3;

            if (g_lastSaveSlot >= 0)
            {
                KB_FlushKeyboardQueue();
                KB_ClearKeysDown();
                FX_StopAllSounds();

                if (ud.multimode > 1)
                {
                    G_LoadPlayer(-1-g_lastSaveSlot);
                    g_player[myconnectindex].ps->gm = MODE_GAME;
                }
                else
                {
                    i = G_LoadPlayer(g_lastSaveSlot);
                    if (i == 0)
                        g_player[myconnectindex].ps->gm = MODE_GAME;
                }
            }
        }

        if (KB_UnBoundKeyPressed(sc_F10))
        {
            KB_ClearKeyDown(sc_F10);
            ChangeToMenu(500);
            FX_StopAllSounds();
            S_ClearSoundLocks();
            g_player[myconnectindex].ps->gm |= MODE_MENU;
            if (ud.multimode < 2 && ud.recstat != 2)
            {
                ready2send = 0;
                totalclock = ototalclock;
            }
        }

        if (ud.overhead_on != 0)
        {

            j = totalclock-nonsharedtimer;
            nonsharedtimer += j;
            if (BUTTON(gamefunc_Enlarge_Screen))
                g_player[myconnectindex].ps->zoom += mulscale6(j,max(g_player[myconnectindex].ps->zoom,256));
            if (BUTTON(gamefunc_Shrink_Screen))
                g_player[myconnectindex].ps->zoom -= mulscale6(j,max(g_player[myconnectindex].ps->zoom,256));

            if ((g_player[myconnectindex].ps->zoom > 2048))
                g_player[myconnectindex].ps->zoom = 2048;
            if ((g_player[myconnectindex].ps->zoom < 48))
                g_player[myconnectindex].ps->zoom = 48;

        }
    }

    if (KB_KeyPressed(sc_Escape) && ud.overhead_on && g_player[myconnectindex].ps->newowner == -1)
    {
        KB_ClearKeyDown(sc_Escape);
        ud.last_overhead = ud.overhead_on;
        ud.overhead_on = 0;
        ud.scrollmode = 0;
        G_UpdateScreenArea();
    }

    if (BUTTON(gamefunc_AutoRun))
    {
        CONTROL_ClearButton(gamefunc_AutoRun);
        ud.auto_run = 1-ud.auto_run;
        ud.config.RunMode = ud.auto_run;
        P_DoQuote(85+ud.auto_run,g_player[myconnectindex].ps);
    }

    if (BUTTON(gamefunc_Map))
    {
        CONTROL_ClearButton(gamefunc_Map);
        if (ud.last_overhead != ud.overhead_on && ud.last_overhead)
        {
            ud.overhead_on = ud.last_overhead;
            ud.last_overhead = 0;
        }
        else
        {
            ud.overhead_on++;
            if (ud.overhead_on == 3) ud.overhead_on = 0;
            ud.last_overhead = ud.overhead_on;
        }
        g_restorePalette = 1;
        G_UpdateScreenArea();
    }
    /*
        if (KB_UnBoundKeyPressed(sc_F11))
        {
            KB_ClearKeyDown(sc_F11);
            ud.brightness+=8;
            if (SHIFTS_IS_PRESSED) ud.brightness-=16;

            if (ud.brightness > (7<<3))
                ud.brightness = 0;
            else if (ud.brightness < 0)
                ud.brightness = (7<<3);

            setbrightness(ud.brightness>>2,&g_player[myconnectindex].ps->palette[0],0);
            if (ud.brightness < 40) P_DoQuote(29 + (ud.brightness>>3) ,g_player[myconnectindex].ps);
            else if (ud.brightness < 80) P_DoQuote(96 + (ud.brightness>>3) - 5,g_player[myconnectindex].ps);
        }
    */
    if (KB_UnBoundKeyPressed(sc_F11))
    {
        KB_ClearKeyDown(sc_F11);
        ChangeToMenu(232);
        FX_StopAllSounds();
        S_ClearSoundLocks();
        g_player[myconnectindex].ps->gm |= MODE_MENU;
        if (ud.multimode < 2 && ud.recstat != 2)
        {
            ready2send = 0;
            totalclock = ototalclock;
        }
    }
}

static void G_ShowParameterHelp(void)
{
    char *s = "Usage: eduke32 [files] [options]\n"
              "Example: eduke32 -q4 -a -m -tx -map nukeland.map\n\n"
              "Files can be *.grp/zip/con/def\n"
              "\n"
              "-cfg [file.cfg]\tUse an alternate configuration file\n"
              "-c#\t\tUse MP mode #, 1 = Dukematch, 2 = Coop, 3 = Dukematch(no spawn)\n"
              "-d[file.dmo]\tPlay a demo\n"
              "-g[file.grp]\tUse an extra group file\n"
              "-h[file.def]\tUse an alternate def\n"
              "-j[dir]\t\tAdds a directory to EDuke32's search list\n"
              "-l#\t\tWarp to level #, see -v\n"
              "-map [file.map]\tLoads a map\n"
              "-m\t\tDisable monsters\n"
              "-nam/-ww2gi\tRun in NAM or WW2GI-compatible mode\n"
              "-net\t\tEnable multiplayer (see documentation)\n"
              "-r\t\tRecord demo\n"
              "-s#\t\tSet skill level (1-4)\n"
#if defined RENDERTYPEWIN || (defined RENDERTYPESDL && !defined __APPLE__ && defined HAVE_GTK2)
              "-setup/nosetup\tEnables/disables startup window\n"
#endif
              "-t#\t\tSet respawn mode: 1 = Monsters, 2 = Items, 3 = Inventory, x = All\n"
#if !defined(_WIN32)
              "-usecwd\t\tRead game data and configuration file from working directory\n"
#endif
              "-u#########\tUser's favorite weapon order (default: 3425689071)\n"
              "-v#\t\tWarp to volume #, see -l\n"
              "-x[game.con]\tLoad custom CON script\n"
              "-#\t\tLoad and run a game from slot # (0-9)\n"
//              "\n-?/--help\tDisplay this help message and exit\n"
              "\nSee eduke32 -debughelp for debug parameters"
              ;
#if defined RENDERTYPEWIN
    Bsprintf(tempbuf,HEAD2 " %s",s_buildDate);
    wm_msgbox(tempbuf,s);
#else
    initprintf("%s\n",s);
#endif
}

static void G_ShowDebugHelp(void)
{
    char *s = "Usage: eduke32 [files] [options]\n"
              "\n"
              "-a\t\tUse fake player AI (fake multiplayer only)\n"
              "-cachesize #\tSets cache size, in Kb\n"
              "-fNUM\t\tSend fewer packets in multiplayer (1, 2, 4) (deprecated)\n"
              "-game_dir [dir]\tDuke3d_w32 compatibility option, see -j\n"
              "-gamegrp   \tSelects which file to use as main grp\n"
              "-i#\t\tUse networking mode (1/0)\n"
              "-name [name]\tPlayer name in multiplay\n"
              "-nD\t\tDump default gamevars to gamevars.txt\n"
              "-noautoload\tDisable loading content from autoload dir\n"
              "-nologo\t\tSkip the logo anim\n"
              "-ns/-nm\t\tDisable sound or music\n"
              "-q#\t\tFake multiplayer with # (2-8) players\n"
              "-rmnet\t\tUse network config file (OBSOLETE, see -net)\n"
              "-stun\t\tUse UDP hole punching for multiplayer connections\n"
              /*"-unstable   \tForce EDuke32 to execute unsafe CON commands (and crash)\n"*/
              "-w\t\tShow coordinates\n"
              "-z#/-condebug\tEnable line-by-line CON compile debugging at level #\n"
              ;
#if defined RENDERTYPEWIN
    Bsprintf(tempbuf,HEAD2 " %s",s_buildDate);
    wm_msgbox(tempbuf,s);
#else
    initprintf("%s\n",s);
#endif
}

static CACHE1D_FIND_REC *finddirs=NULL, *findfiles=NULL, *finddirshigh=NULL, *findfileshigh=NULL;
static int32_t numdirs=0, numfiles=0;
static int32_t currentlist=0;

static void clearfilenames(void)
{
    klistfree(finddirs);
    klistfree(findfiles);
    finddirs = findfiles = NULL;
    numfiles = numdirs = 0;
}

static int32_t getfilenames(const char *path, char kind[])
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

static char *autoloadmasks[] = { "*.grp", "*.zip", "*.pk3" };
#define NUMAUTOLOADMASKS 3

static void G_DoAutoload(const char *fn)
{
    int32_t i;

    for (i=0; i<NUMAUTOLOADMASKS; i++)
    {
        Bsprintf(tempbuf,"autoload/%s",fn);
        getfilenames(tempbuf,autoloadmasks[i]);
        while (findfiles)
        {
            Bsprintf(tempbuf,"autoload/%s/%s",fn,findfiles->name);
            initprintf("Using group file '%s'.\n",tempbuf);
            initgroupfile(tempbuf);
            findfiles = findfiles->next;
        }
    }
}

static char *makename(char *destname, char *OGGname, char *origname)
{
    if (!origname)
        return destname;

    destname = Brealloc(destname, Bstrlen(OGGname) + Bstrlen(origname) + 1);

    if (!destname)
        return NULL;

    Bstrcpy(destname, (*OGGname)?OGGname:origname);

    if (*OGGname && OGGname[Bstrlen(OGGname)-1] == '/')
    {
        while (*origname == '/')
            origname++;
        Bstrcat(destname, origname);
    }

    OGGname = Bstrchr(destname, '.');

    if (OGGname)
        Bstrcpy(OGGname, ".ogg");
    else Bstrcat(destname, ".ogg");

    return destname;
}

static int32_t AL_DefineSound(int32_t ID,char *name)
{
    if (ID>=MAXSOUNDS)
        return 1;
    g_sounds[ID].filename1=makename(g_sounds[ID].filename1,name,g_sounds[ID].filename);
//    initprintf("(%s)(%s)(%s)\n",g_sounds[ID].filename1,name,g_sounds[ID].filename);
//    S_LoadSound(ID);
    return 0;
}

static int32_t AL_DefineMusic(char *ID,char *name)
{
    int32_t lev,ep,sel; char b1,b2;

    if (!ID)
        return 1;
    if (!Bstrcmp(ID,"intro"))
    {
        sel=MAXVOLUMES*MAXLEVELS;  ID=EnvMusicFilename[0];
    }
    else if (!Bstrcmp(ID,"briefing"))
    {
        sel=MAXVOLUMES*MAXLEVELS+1;
        ID=EnvMusicFilename[1];
    }
    else if (!Bstrcmp(ID,"loading"))
    {
        sel=MAXVOLUMES*MAXLEVELS+2;
        ID=EnvMusicFilename[2];
    }
    else
    {
        sscanf(ID,"%c%d%c%d",&b1,&ep,&b2,&lev);
        lev--;
        ep--;
        if (toupper(b1)!='E'||toupper(b2)!='L'||lev>=MAXLEVELS||ep>=MAXVOLUMES)
            return 1;
        sel=(ep*MAXLEVELS)+lev;
        ID=MapInfo[sel].musicfn;
    }

    MapInfo[sel].alt_musicfn=makename(MapInfo[sel].alt_musicfn,name,ID);
//    initprintf("%-15s | ",ID);
//    initprintf("%3d %2d %2d | %s\n",sel,ep,lev,MapInfo[sel].alt_musicfn);
//    S_PlayMusic(ID,sel);
    return 0;
}

static int32_t parsedefinitions_game(scriptfile *script, const int32_t preload)
{
    int32_t tokn;
    char *cmdtokptr;

    tokenlist tokens[] =
    {
        { "include",         T_INCLUDE          },
        { "#include",        T_INCLUDE          },
        { "loadgrp",         T_LOADGRP          },
        { "cachesize",       T_CACHESIZE        },
        { "noautoload",      T_NOAUTOLOAD       },
        { "music",           T_MUSIC            },
        { "sound",           T_SOUND            },
    };

    tokenlist sound_musictokens[] =
    {
        { "id",   T_ID  },
        { "file", T_FILE },
    };

    while (1)
    {
        tokn = getatoken(script,tokens,sizeof(tokens)/sizeof(tokenlist));
        cmdtokptr = script->ltextptr;
        switch (tokn)
        {
        case T_LOADGRP:
        {
            char *fn;

            pathsearchmode = 1;
            if (!scriptfile_getstring(script,&fn) && preload)
            {
                int32_t j = initgroupfile(fn);

                if (j == -1)
                    initprintf("Could not find group file '%s'.\n",fn);
                else
                {
                    initprintf("Using group file '%s'.\n",fn);
                    if (!g_noAutoLoad && !ud.config.NoAutoLoad)
                        G_DoAutoload(fn);
                }

            }
            pathsearchmode = 0;
        }
        break;
        case T_CACHESIZE:
        {
            int32_t j;
            if (scriptfile_getnumber(script,&j) || !preload) break;

            if (j > 0) MAXCACHE1DSIZE = j<<10;
        }
        break;
        case T_INCLUDE:
        {
            char *fn;
            if (!scriptfile_getstring(script,&fn))
            {
                scriptfile *included = scriptfile_fromfile(fn);

                if (!included)
                {
//                    initprintf("Warning: Failed including %s on line %s:%d\n",
//                               fn, script->filename,scriptfile_getlinum(script,cmdtokptr));
                }
                else
                {
                    parsedefinitions_game(included, preload);
                    scriptfile_close(included);
                }
            }
            break;
        }
        case T_NOAUTOLOAD:
            if (preload)
                g_noAutoLoad = 1;
            break;
        case T_MUSIC:
        {
            char *tinttokptr = script->ltextptr;
            char *ID=NULL,*fn="",*tfn = NULL;
            char *musicend;

            if (scriptfile_getbraces(script,&musicend)) break;
            while (script->textptr < musicend)
            {
                switch (getatoken(script,sound_musictokens,sizeof(sound_musictokens)/sizeof(tokenlist)))
                {
                case T_ID:
                    scriptfile_getstring(script,&ID);
                    break;
                case T_FILE:
                    scriptfile_getstring(script,&fn);
                    break;
                }
            }
            if (!preload)
            {
                int32_t i;
                if (ID==NULL)
                {
                    initprintf("Error: missing ID for music definition near line %s:%d\n", script->filename, scriptfile_getlinum(script,tinttokptr));
                    break;
                }

                i = pathsearchmode;
                pathsearchmode = 1;
                if (findfrompath(fn,&tfn) < 0)
                {
                    char buf[BMAX_PATH];

                    Bstrcpy(buf,fn);
                    kzfindfilestart(buf);
                    if (!kzfindfile(buf))
                    {
                        initprintf("Error: file '%s' does not exist\n",fn);
                        pathsearchmode = i;
                        break;
                    }
                }
                else Bfree(tfn);
                pathsearchmode = i;

                if (AL_DefineMusic(ID,fn))
                    initprintf("Error: invalid music ID on line %s:%d\n", script->filename, scriptfile_getlinum(script,tinttokptr));
            }
        }
        break;

        case T_SOUND:
        {
            char *tinttokptr = script->ltextptr;
            char *fn="", *tfn = NULL;
            int32_t num=-1;
            char *musicend;

            if (scriptfile_getbraces(script,&musicend)) break;
            while (script->textptr < musicend)
            {
                switch (getatoken(script,sound_musictokens,sizeof(sound_musictokens)/sizeof(tokenlist)))
                {
                case T_ID:
                    scriptfile_getsymbol(script,&num);
                    break;
                case T_FILE:
                    scriptfile_getstring(script,&fn);
                    break;
                }
            }
            if (!preload)
            {
                int32_t i;

                if (num==-1)
                {
                    initprintf("Error: missing ID for sound definition near line %s:%d\n", script->filename, scriptfile_getlinum(script,tinttokptr));
                    break;
                }

                i = pathsearchmode;
                pathsearchmode = 1;
                if (findfrompath(fn,&tfn) < 0)
                {
                    char buf[BMAX_PATH];

                    Bstrcpy(buf,fn);
                    kzfindfilestart(buf);
                    if (!kzfindfile(buf))
                    {
                        initprintf("Error: file '%s' does not exist\n",fn);
                        pathsearchmode = i;
                        break;
                    }
                }
                else Bfree(tfn);
                pathsearchmode = i;

                if (AL_DefineSound(num,fn))
                    initprintf("Error: invalid sound ID on line %s:%d\n", script->filename, scriptfile_getlinum(script,tinttokptr));
            }
        }
        break;
        case T_EOF:
            return(0);
        default:
            break;
        }
    }
    return 0;
}

static int32_t loaddefinitions_game(const char *fn, int32_t preload)
{
    scriptfile *script;

    script = scriptfile_fromfile((char *)fn);
    if (!script) return -1;

    parsedefinitions_game(script, preload);

    scriptfile_close(script);
    scriptfile_clearsymbols();

    return 0;
}

static void G_AddGroup(const char *buffer)
{
    struct strllist *s;
    char buf[BMAX_PATH];

    s = (struct strllist *)Bcalloc(1,sizeof(struct strllist));

    Bstrcpy(buf, buffer);

    if (Bstrchr(buf,'.') == 0)
        Bstrcat(buf,".grp");

    s->str = Bstrdup(buf);

    if (CommandGrps)
    {
        struct strllist *t;
        for (t = CommandGrps; t->next; t=t->next) ;
        t->next = s;
        return;
    }
    CommandGrps = s;
}

static void G_AddPath(const char *buffer)
{
    struct strllist *s;
    s = (struct strllist *)Bcalloc(1,sizeof(struct strllist));
    s->str = Bstrdup(buffer);

    if (CommandPaths)
    {
        struct strllist *t;
        for (t = CommandPaths; t->next; t=t->next) ;
        t->next = s;
        return;
    }
    CommandPaths = s;
}


static void G_CheckCommandLine(int32_t argc, const char **argv)
{
    int32_t firstnet = 0;
    int16_t i = 1, j;
    char *c, *k;

    ud.fta_on = 1;
    ud.god = 0;
    ud.m_respawn_items = 0;
    ud.m_respawn_monsters = 0;
    ud.m_respawn_inventory = 0;
    ud.warp_on = 0;
    ud.cashman = 0;
    ud.m_player_skill = ud.player_skill = 2;
    g_player[0].wchoice[0] = 3;
    g_player[0].wchoice[1] = 4;
    g_player[0].wchoice[2] = 5;
    g_player[0].wchoice[3] = 7;
    g_player[0].wchoice[4] = 8;
    g_player[0].wchoice[5] = 6;
    g_player[0].wchoice[6] = 0;
    g_player[0].wchoice[7] = 2;
    g_player[0].wchoice[8] = 9;
    g_player[0].wchoice[9] = 1;

    if (argc > 1)
    {
        initprintf("Application parameters: ");
        while (i < argc)
            initprintf("%s ",argv[i++]);
        initprintf("\n");

        i = 1;
        while (i < argc)
        {
            c = (char *)argv[i];
            if (((*c == '/') || (*c == '-')) && (!firstnet))
            {
                if (!Bstrcasecmp(c+1,"?") || !Bstrcasecmp(c+1,"help") || !Bstrcasecmp(c+1,"-help"))
                {
                    G_ShowParameterHelp();
                    exit(0);
                }
                if (!Bstrcasecmp(c+1,"?") || !Bstrcasecmp(c+1,"debughelp") || !Bstrcasecmp(c+1,"-debughelp"))
                {
                    G_ShowDebugHelp();
                    exit(0);
                }
                if (!Bstrcasecmp(c+1,"grp") || !Bstrcasecmp(c+1,"g"))
                {
                    if (argc > i+1)
                    {
                        G_AddGroup(argv[i+1]);
                        i++;
                    }
                    i++;
                    continue;
                }
                if (!Bstrcasecmp(c+1,"game_dir"))
                {
                    if (argc > i+1)
                    {
                        Bstrcpy(mod_dir,argv[i+1]);
                        G_AddPath(argv[i+1]);
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
                        i++;
                    }
                    i++;
                    continue;
                }
                if (!Bstrcasecmp(c+1,"nam"))
                {
                    Bsprintf(defaultduke3dgrp, "nam.grp");
                    Bsprintf(defaultduke3ddef, "nam.def");
                    Bsprintf(defaultconfilename, "nam.con");
                    g_gameType = GAMENAM;
                    i++;
                    continue;
                }
                if (!Bstrcasecmp(c+1,"ww2gi"))
                {
                    Bsprintf(defaultduke3dgrp, "ww2gi.grp");
                    Bsprintf(defaultduke3ddef, "ww2gi.def");
                    Bsprintf(defaultconfilename, "ww2gi.con");
                    g_gameType = GAMEWW2;
                    i++;
                    continue;
                }
                if (!Bstrcasecmp(c+1,"setup"))
                {
                    g_commandSetup = TRUE;
                    i++;
                    continue;
                }
                if (!Bstrcasecmp(c+1,"nosetup"))
                {
                    g_noSetup = 1;
                    g_commandSetup = 0;
                    i++;
                    continue;
                }
                if (!Bstrcasecmp(c+1,"noautoload"))
                {
                    initprintf("Autoload disabled\n");
                    g_noAutoLoad = 1;
                    i++;
                    continue;
                }
                if (!Bstrcasecmp(c+1,"keepaddr"))
                {
                    g_keepAddr = 1;
                    i++;
                    continue;
                }
                if (!Bstrcasecmp(c+1,"stun"))
                {
                    natfree = 1; //Addfaz NatFree
                    i++;
                    continue;
                }
                if (!Bstrcasecmp(c+1,"debugsync"))
                {
                    g_numSyncBytes = 6;
                    i++;
                    continue;
                }
                if (!Bstrcasecmp(c+1,"net") || !Bstrcasecmp(c+1,"rmnet"))
                {
                    g_noSetup = TRUE;
                    firstnet = i;
                    netparamcount = argc - i - 1;
                    netparam = (char **)Bcalloc(netparamcount, sizeof(char**));
                    i++;
                    continue;
                }
                if (!Bstrcasecmp(c+1,"name"))
                {
                    if (argc > i+1)
                    {
                        CommandName = (char *)argv[i+1];
                        i++;
                    }
                    i++;
                    continue;
                }
                if (!Bstrcasecmp(c+1,"map"))
                {
                    if (argc > i+1)
                    {
                        CommandMap = (char *)argv[i+1];
                        i++;
                    }
                    i++;
                    continue;
                }
                if (!Bstrcasecmp(c+1,"condebug"))
                {
                    g_scriptDebug = 1;
                    i++;
                    continue;
                }
                if (!Bstrcasecmp(c+1,"nologo"))
                {
                    g_noLogo = 1;
                    i++;
                    continue;
                }
#if !defined(_WIN32)
                if (!Bstrcasecmp(c+1,"usecwd"))
                {
                    usecwd = 1;
                    i++;
                    continue;
                }
#endif
                /*
                                if (!Bstrcasecmp(c+1,"unstable"))
                                {
                                    initprintf("WARNING WARNING WARNING WARNING\n"
                                               "EDuke32's runtime script error detection has been disabled via "
                                               "the '-unstable' command line parameter.  Bug reports from this "
                                               "mode are NOT welcome and you should expect crashes in certain "
                                               "mods.  Please run EDuke32 without '-unstable' before sending "
                                               "any bug reports.\n");
                                    g_scriptSanityChecks = 0;
                                    i++;
                                    continue;
                                }
                */
                if (!Bstrcasecmp(c+1,"cachesize"))
                {
                    if (argc > i+1)
                    {
                        uint32_t j = atol((char *)argv[i+1]);
                        MAXCACHE1DSIZE = j<<10;
                        initprintf("Cache size: %dkB\n",j);
                        i++;
                    }
                    i++;
                    continue;
                }
                if (!Bstrcasecmp(c+1,"noinstancechecking"))
                {
                    i++;
                    continue;
                }
#if defined(RENDERTYPEWIN) && defined(USE_OPENGL)
                if (!Bstrcasecmp(c+1,"forcegl"))
                {
                    forcegl = 1;
                    i++;
                    continue;
                }
#endif
            }

            if (firstnet > 0)
            {
                if (*c == '-' || *c == '/')
                {
                    c++;
                    if (((c[0] == 'n') || (c[0] == 'N')) && (c[1] == '0'))
                    {
                        g_networkBroadcastMode = 0;
                        initprintf("Network mode: master/slave\n");
                    }
                    else if (((c[0] == 'n') || (c[0] == 'N')) && (c[1] == '1'))
                    {
                        g_networkBroadcastMode = 1;
                        initprintf("Network mode: peer-to-peer\n");
                    }

                }
                netparam[i-firstnet-1] = (char *)argv[i];
                i++;
                continue;
            }

            if ((*c == '/') || (*c == '-'))
            {
                c++;
                switch (Btolower(*c))
                {
                case 'a':
                    ud.playerai = 1;
                    initprintf("Other player AI.\n");
                    break;
                case 'c':

                    c++;
                    //if(*c == '1' || *c == '2' || *c == '3')
                    //    ud.m_coop = *c - '0' - 1;
                    //else ud.m_coop = 0;

                    ud.m_coop = 0;
                    while ((*c >= '0')&&(*c <= '9'))
                    {
                        ud.m_coop *= 10;
                        ud.m_coop += *c - '0';
                        c++;
                    }
                    ud.m_coop--;
                    //switch(ud.m_coop)
                    //{
                    //case 0:
                    //    initprintf("Dukematch (spawn).\n");
                    //    break;
                    //case 1:
                    //    initprintf("Cooperative play.\n");
                    //    break;
                    //case 2:
                    //    initprintf("Dukematch (no spawn).\n");
                    //    break;
                    //}
                    break;
                case 'd':
                    c++;
                    if (strchr(c,'.') == 0)
                        Bstrcat(c,".dmo");
                    initprintf("Play demo %s.\n",c);
                    Bstrcpy(firstdemofile,c);
                    break;
                case 'f':
                    c++;
                    if (*c == '1')
                        g_movesPerPacket = 1;
                    if (*c == '2')
                        g_movesPerPacket = 2;
                    if (*c == '4')
                    {
                        g_movesPerPacket = 4;
                        mmulti_setpackettimeout(0x3fffffff,0x3fffffff); // this doesn't do anything anymore
                    }
                    break;
                case 'g':
                    c++;
                    if (!*c) break;
                    G_AddGroup(c);
                    break;
                case 'h':
                    c++;
                    if (*c)
                    {
                        duke3ddef = c;
                        initprintf("Using DEF file: %s.\n",duke3ddef);
                    }
                    break;
                case 'i':
                    c++;
                    if (*c == '0') g_networkBroadcastMode = 0;
                    if (*c == '1') g_networkBroadcastMode = 1;
                    initprintf("Network Mode %d\n",g_networkBroadcastMode);
                    break;
                case 'j':
                    c++;
                    if (!*c) break;
                    G_AddPath(c);
                    break;
                case 'l':
                    ud.warp_on = 1;
                    c++;
                    ud.m_level_number = ud.level_number = (atoi(c)-1)%MAXLEVELS;
                    break;
                case 'm':
                    if (*(c+1) != 'a' && *(c+1) != 'A')
                    {
                        ud.m_monsters_off = 1;
                        ud.m_player_skill = ud.player_skill = 0;
                        initprintf("Monsters off.\n");
                    }
                    break;
                case 'n':
                    c++;
                    if (*c == 's' || *c == 'S')
                    {
                        g_noSound = 2;
                        initprintf("Sound off.\n");
                    }
                    else if (*c == 'm' || *c == 'M')
                    {
                        g_noMusic = 1;
                        initprintf("Music off.\n");
                    }
                    /*
                                        else if (*c == 'd' || *c == 'D')
                                        {
                                            FILE * fp=fopen("gamevars.txt","w");
                                            Gv_Init();
                                            Gv_DumpValues(fp);
                                            fclose(fp);
                                            initprintf("Game variables saved to gamevars.txt.\n");
                                        }
                    */
                    else
                    {
                        G_ShowParameterHelp();
                        exit(-1);
                    }
                    break;
                case 'q':
                    initprintf("Fake multiplayer mode.\n");
                    if (*(++c) == 0) ud.multimode = 1;
                    else ud.multimode = atoi(c)%17;
                    ud.m_coop = ud.coop = 0;
                    ud.m_marker = ud.marker = 1;
                    ud.m_respawn_monsters = ud.respawn_monsters = 1;
                    ud.m_respawn_items = ud.respawn_items = 1;
                    ud.m_respawn_inventory = ud.respawn_inventory = 1;
                    break;
                case 'r':
                    ud.m_recstat = 1;
                    initprintf("Demo record mode on.\n");
                    break;
                case 's':
                    c++;
                    ud.m_player_skill = ud.player_skill = (atoi(c)%5);
                    if (ud.m_player_skill == 4)
                        ud.m_respawn_monsters = ud.respawn_monsters = 1;
                    break;
                case 't':
                    c++;
                    if (*c == '1') ud.m_respawn_monsters = 1;
                    else if (*c == '2') ud.m_respawn_items = 1;
                    else if (*c == '3') ud.m_respawn_inventory = 1;
                    else
                    {
                        ud.m_respawn_monsters = 1;
                        ud.m_respawn_items = 1;
                        ud.m_respawn_inventory = 1;
                    }
                    initprintf("Respawn on.\n");
                    break;
                case 'u':
                    CommandWeaponChoice = 1;
                    c++;
                    j = 0;
                    if (*c)
                    {
                        initprintf("Using favorite weapon order(s).\n");
                        while (*c)
                        {
                            g_player[0].wchoice[j] = *c-'0';
                            c++;
                            j++;
                        }
                        while (j < 10)
                        {
                            if (j == 9)
                                g_player[0].wchoice[9] = 1;
                            else
                                g_player[0].wchoice[j] = 2;

                            j++;
                        }
                    }
                    else
                    {
                        initprintf("Using default weapon orders.\n");
                        g_player[0].wchoice[0] = 3;
                        g_player[0].wchoice[1] = 4;
                        g_player[0].wchoice[2] = 5;
                        g_player[0].wchoice[3] = 7;
                        g_player[0].wchoice[4] = 8;
                        g_player[0].wchoice[5] = 6;
                        g_player[0].wchoice[6] = 0;
                        g_player[0].wchoice[7] = 2;
                        g_player[0].wchoice[8] = 9;
                        g_player[0].wchoice[9] = 1;
                    }
                    break;
                case 'v':
                    c++;
                    ud.warp_on = 1;
                    ud.m_volume_number = ud.volume_number = atoi(c)-1;
                    break;
                case 'w':
                    ud.coords = 1;
                    break;
                case 'x':
                    c++;
                    if (*c)
                    {
                        confilename = c;
                        userconfiles = 1;
                        initprintf("Using CON file '%s'.\n",confilename);
                    }
                    break;
                case '0':
                case '1':
                case '2':
                case '3':
                case '4':
                case '5':
                case '6':
                case '7':
                case '8':
                case '9':
                    ud.warp_on = 2 + (*c) - '0';
                    break;
                case 'z':
                    c++;
                    g_scriptDebug = atoi(c);
                    if (!g_scriptDebug)
                        g_scriptDebug = 1;
                    break;
                }
            }
            else
            {
                k = Bstrrchr(c,'.');
                if (k)
                {
                    if (!Bstrcasecmp(k,".map"))
                    {
                        CommandMap = (char *)argv[i++];
                        continue;
                    }
                    if (!Bstrcasecmp(k,".grp") || !Bstrcasecmp(k,".zip") || !Bstrcasecmp(k,".pk3"))
                    {
                        G_AddGroup(argv[i++]);
                        continue;
                    }
                    if (!Bstrcasecmp(k,".con"))
                    {
                        confilename = (char *)argv[i++];
                        userconfiles = 1;
                        initprintf("Using CON file '%s'.\n",confilename);
                        continue;
                    }
                    if (!Bstrcasecmp(k,".def"))
                    {
                        duke3ddef = (char *)argv[i++];
                        initprintf("Using DEF file: %s.\n",duke3ddef);
                        continue;
                    }
                }
            }
            i++;
        }
    }
}

static void G_DisplayLogo(void)
{
    int32_t soundanm = 0;
    int32_t logoflags=Gv_GetVarByLabel("LOGO_FLAGS",255, -1, -1);

    ready2send = 0;

    KB_FlushKeyboardQueue();
    KB_ClearKeysDown(); // JBF

    setview(0,0,xdim-1,ydim-1);
    clearview(0L);
    IFISSOFTMODE G_FadePalette(0,0,0,63);

    flushperms();
    nextpage();

    Bsprintf(tempbuf,"%s - " APPNAME,duke3dgrpstring);
    wm_setapptitle(tempbuf);

    S_StopMusic();
    FX_StopAllSounds(); // JBF 20031228
    S_ClearSoundLocks();  // JBF 20031228
    if (ud.multimode < 2 && (logoflags & LOGO_ENABLED) && !g_noLogo)
    {
        if (VOLUMEALL && (logoflags & LOGO_PLAYANIM))
        {

            if (!KB_KeyWaiting() && g_noLogoAnim == 0)
            {
                Net_GetPackets();
                G_PlayAnim("logo.anm",5);
                IFISSOFTMODE G_FadePalette(0,0,0,63);
                KB_FlushKeyboardQueue();
                KB_ClearKeysDown(); // JBF
            }

            clearview(0L);
            nextpage();
        }

        if (logoflags & LOGO_PLAYMUSIC)
        {
            g_musicIndex = -1; // hack
            S_PlayMusic(&EnvMusicFilename[0][0],MAXVOLUMES*MAXLEVELS);
        }

        if (!NAM)
        {
            fadepal(0,0,0, 0,64,7);
            //g_player[myconnectindex].ps->palette = drealms;
            //G_FadePalette(0,0,0,63);
            if (logoflags & LOGO_3DRSCREEN)
            {
                P_SetGamePalette(g_player[myconnectindex].ps, drealms, 11);    // JBF 20040308
                rotatesprite(0,0,65536L,0,DREALMS,0,0,2+8+16+64, 0,0,xdim-1,ydim-1);
                nextpage();
                fadepal(0,0,0, 63,0,-7);
                totalclock = 0;
                while (totalclock < (120*7) && !KB_KeyWaiting() && !MOUSE_GetButtons()&LEFT_MOUSE)
                {
                    handleevents();
                    Net_GetPackets();
                    if (g_restorePalette)
                    {
                        P_SetGamePalette(g_player[myconnectindex].ps,g_player[myconnectindex].ps->palette,0);
                        g_restorePalette = 0;
                    }
                }
            }
            KB_ClearKeysDown(); // JBF
            MOUSE_ClearButton(LEFT_MOUSE);
        }

        fadepal(0,0,0, 0,64,7);
        clearview(0L);
        nextpage();
        if (logoflags & LOGO_TITLESCREEN)
        {
            //g_player[myconnectindex].ps->palette = titlepal;
            P_SetGamePalette(g_player[myconnectindex].ps, titlepal, 11);   // JBF 20040308
            flushperms();
            rotatesprite(0,0,65536L,0,BETASCREEN,0,0,2+8+16+64,0,0,xdim-1,ydim-1);
            KB_FlushKeyboardQueue();
            fadepal(0,0,0, 63,0,-7);
            totalclock = 0;

            while (totalclock < (860+120) && !KB_KeyWaiting() && !MOUSE_GetButtons()&LEFT_MOUSE)
            {
                rotatesprite(0,0,65536L,0,BETASCREEN,0,0,2+8+16+64,0,0,xdim-1,ydim-1);
                if (logoflags & LOGO_DUKENUKEM)
                {
                    if (totalclock > 120 && totalclock < (120+60))
                    {
                        if (soundanm == 0)
                        {
                            soundanm = 1;
                            S_PlaySound(PIPEBOMB_EXPLODE);
                        }
                        rotatesprite(160<<16,104<<16,(totalclock-120)<<10,0,DUKENUKEM,0,0,2+8,0,0,xdim-1,ydim-1);
                    }
                    else if (totalclock >= (120+60))
                        rotatesprite(160<<16,(104)<<16,60<<10,0,DUKENUKEM,0,0,2+8,0,0,xdim-1,ydim-1);
                }
                else soundanm = 1;
                if (logoflags & LOGO_THREEDEE)
                {
                    if (totalclock > 220 && totalclock < (220+30))
                    {
                        if (soundanm == 1)
                        {
                            soundanm = 2;
                            S_PlaySound(PIPEBOMB_EXPLODE);
                        }

                        rotatesprite(160<<16,(104)<<16,60<<10,0,DUKENUKEM,0,0,2+8,0,0,xdim-1,ydim-1);
                        rotatesprite(160<<16,(129)<<16,(totalclock - 220)<<11,0,THREEDEE,0,0,2+8,0,0,xdim-1,ydim-1);
                    }
                    else if (totalclock >= (220+30))
                        rotatesprite(160<<16,(129)<<16,30<<11,0,THREEDEE,0,0,2+8,0,0,xdim-1,ydim-1);
                }
                else soundanm = 2;
                if (PLUTOPAK && (logoflags & LOGO_PLUTOPAKSPRITE))
                {
                    // JBF 20030804
                    if (totalclock >= 280 && totalclock < 395)
                    {
                        rotatesprite(160<<16,(151)<<16,(410-totalclock)<<12,0,PLUTOPAKSPRITE+1,0,0,2+8,0,0,xdim-1,ydim-1);
                        if (soundanm == 2)
                        {
                            soundanm = 3;
                            S_PlaySound(FLY_BY);
                        }
                    }
                    else if (totalclock >= 395)
                    {
                        if (soundanm == 3)
                        {
                            soundanm = 4;
                            S_PlaySound(PIPEBOMB_EXPLODE);
                        }
                        rotatesprite(160<<16,(151)<<16,30<<11,0,PLUTOPAKSPRITE+1,0,0,2+8,0,0,xdim-1,ydim-1);
                    }
                }
                X_OnEvent(EVENT_LOGO, -1, screenpeek, -1);
                handleevents();
                Net_GetPackets();
                if (g_restorePalette)
                {
                    P_SetGamePalette(g_player[myconnectindex].ps,g_player[myconnectindex].ps->palette,0);
                    g_restorePalette = 0;
                }
                nextpage();
            }
        }
        KB_ClearKeysDown(); // JBF
        MOUSE_ClearButton(LEFT_MOUSE);
    }

    Net_WaitForEverybody();

    flushperms();
    clearview(0L);
    nextpage();

    //g_player[myconnectindex].ps->palette = palette;
    P_SetGamePalette(g_player[myconnectindex].ps, palette, 0);    // JBF 20040308
    S_PlaySound(NITEVISION_ONOFF);

    //G_FadePalette(0,0,0,0);
    clearview(0L);
}

/*
static void loadtmb(void)
{
    char tmb[8000];
    int32_t fil, l;

    fil = kopen4load("d3dtimbr.tmb",0);
    if (fil == -1) return;
    l = kfilelength(fil);
    kread(fil,(char *)tmb,l);
    MUSIC_RegisterTimbreBank(tmb);
    kclose(fil);
}
*/

extern void C_FreeHashes();

static void G_FreeMemory(void)
{
    int32_t i;
    extern char *bitptr;

    for (i=(MAXLEVELS*(MAXVOLUMES+1))-1; i>=0; i--) // +1 volume for "intro", "briefing" music
    {
        if (MapInfo[i].name != NULL) Bfree(MapInfo[i].name);
        if (MapInfo[i].filename != NULL) Bfree(MapInfo[i].filename);
        if (MapInfo[i].musicfn != NULL) Bfree(MapInfo[i].musicfn);
        if (MapInfo[i].alt_musicfn != NULL) Bfree(MapInfo[i].alt_musicfn);
        if (MapInfo[i].savedstate != NULL) G_FreeMapState(i);
    }

    for (i=MAXQUOTES-1; i>=0; i--)
    {
        if (ScriptQuotes[i] != NULL) Bfree(ScriptQuotes[i]);
        if (ScriptQuoteRedefinitions[i] != NULL) Bfree(ScriptQuoteRedefinitions[i]);
    }

    for (i=MAXPLAYERS-1; i>=0; i--)
    {
        if (g_player[i].ps != NULL) Bfree(g_player[i].ps);
        if (g_player[i].sync != NULL) Bfree(g_player[i].sync);
    }

    for (i=MAXSOUNDS-1; i>=0; i--)
    {
        if (g_sounds[i].filename != NULL) Bfree(g_sounds[i].filename);
        if (g_sounds[i].filename1 != NULL) Bfree(g_sounds[i].filename1);
    }

    if (label != NULL) Bfree(label);
    if (labelcode != NULL) Bfree(labelcode);
    if (script != NULL) Bfree(script);
    if (bitptr != NULL) Bfree(bitptr);

//    if (MusicPtr != NULL) Bfree(MusicPtr);

    // C_FreeHashes();
    hash_free(&gamefuncH);
}

/*
===================
=
= ShutDown
=
===================
*/

void G_Shutdown(void)
{
    S_SoundShutdown();
    S_MusicShutdown();
    uninittimer();
    CONTROL_Shutdown();
    CONFIG_WriteSetup();
    KB_Shutdown();
    G_FreeMemory();
    uninitengine();
}

/*
===================
=
= G_Startup
=
===================
*/

static void G_CompileScripts(void)
{
    int32_t i, psm = pathsearchmode;

    label     = (char *)&sprite[0];     // V8: 16384*44/64 = 11264  V7: 4096*44/64 = 2816
    labelcode = (intptr_t *)&sector[0]; // V8: 4096*40/4 = 40960    V7: 1024*40/4 = 10240
    labeltype = (intptr_t *)&wall[0];   // V8: 16384*32/4 = 131072  V7: 8192*32/4 = 65536

    Bcorrectfilename(confilename,0);
    // if we compile for a V7 engine wall[] should be used for label names since it's bigger
    pathsearchmode = 1;
    if (userconfiles == 0)
    {
        i = kopen4loadfrommod(confilename,0);
        if (i!=-1)
            kclose(i);
        else Bsprintf(confilename,"GAME.CON");
    }
    C_Compile(confilename);

    if (g_loadFromGroupOnly)
    {
        if (userconfiles == 0)
        {
            i = kopen4loadfrommod("EDUKE.CON",1);
            if (i!=-1)
            {
                Bsprintf(confilename,"EDUKE.CON");
                kclose(i);
            }
            else Bsprintf(confilename,"GAME.CON");
        }
        C_Compile(confilename);
    }

    if ((uint32_t)g_numLabels > MAXSPRITES*sizeof(spritetype)/64)   // see the arithmetic above for why
        G_GameExit("Error: too many labels defined!");
    else
    {
        char *newlabel;
        intptr_t *newlabelcode;

        newlabel     = (char *)Bmalloc(g_numLabels<<6);
        newlabelcode = (intptr_t *)Bmalloc(g_numLabels*sizeof(intptr_t));

        if (!newlabel || !newlabelcode)
        {
            G_GameExit("Error: out of memory retaining labels\n");
        }

        copybuf(label,     newlabel, (g_numLabels*64)/4);
        copybuf(labelcode, newlabelcode, (g_numLabels*sizeof(intptr_t))/4);

        label = newlabel;
        labelcode = newlabelcode;
    }
    clearbufbyte(&sprite[0], sizeof(spritetype) * MAXSPRITES, 0);
    clearbufbyte(&sector[0], sizeof(sectortype) * MAXSECTORS, 0);
    clearbufbyte(&wall[0], sizeof(walltype) * MAXWALLS, 0);

    X_OnEvent(EVENT_INIT, -1, -1, -1);
    pathsearchmode = psm;
}

static inline void G_CheckGametype(void)
{
    //    initprintf("ud.m_coop=%i before sanitization\n",ud.m_coop);
    ud.m_coop = clamp(ud.m_coop, 0, g_numGametypes-1);
    Bsprintf(tempbuf,"%s\n",GametypeNames[ud.m_coop]);
    initprintf(tempbuf);
    if (GametypeFlags[ud.m_coop] & GAMETYPE_ITEMRESPAWN)
        ud.m_respawn_items = ud.m_respawn_inventory = 1;
    //     initprintf("ud.m_coop=%i after sanitisation\n",ud.m_coop);
}

static void G_LoadExtraPalettes(void)
{
    int32_t j,fp;
    int8_t look_pos;
    char *lookfn = "lookup.dat";

    fp = kopen4loadfrommod(lookfn,0);
    if (fp != -1)
        kread(fp,(char *)&g_numRealPalettes,1);
    else
        G_GameExit("\nERROR: File 'lookup.dat' not found.");

#if defined(__APPLE__) && B_BIG_ENDIAN != 0
    // this is almost as bad as just setting the value to 25 :P
    g_numRealPalettes = ((g_numRealPalettes * 0x80200802ULL) & 0x0884422110ULL) * 0x0101010101ULL >> 32;

#endif

    for (j = 0; j < 256; j++)
        tempbuf[j] = j;

    for (j=g_numRealPalettes+1; j<MAXPALOOKUPS; j++)
        makepalookup(j,tempbuf,0,0,0,1);

    for (j=g_numRealPalettes-1; j>=0; j--)
    {
        kread(fp,(char *)&look_pos,1);
        kread(fp,tempbuf,256);
        makepalookup((int32_t)look_pos,tempbuf,0,0,0,1);
    }

    for (j = 255; j>=0; j--)
        tempbuf[j] = j;
    g_numRealPalettes++;
    makepalookup(g_numRealPalettes, tempbuf, 15, 15, 15, 1);
    makepalookup(g_numRealPalettes + 1, tempbuf, 15, 0, 0, 1);
    makepalookup(g_numRealPalettes + 2, tempbuf, 0, 15, 0, 1);
    makepalookup(g_numRealPalettes + 3, tempbuf, 0, 0, 15, 1);

    kread(fp,&waterpal[0],768);
    kread(fp,&slimepal[0],768);
    kread(fp,&titlepal[0],768);
    kread(fp,&drealms[0],768);
    kread(fp,&endingpal[0],768);

    palette[765] = palette[766] = palette[767] = 0;
    slimepal[765] = slimepal[766] = slimepal[767] = 0;
    waterpal[765] = waterpal[766] = waterpal[767] = 0;

    kclose(fp);
}

extern int32_t startwin_run(void);
static void G_SetupGameButtons(void);

static void G_Startup(void)
{
    int32_t i;

    inittimer(TICRATE);

    G_CompileScripts();

    CONFIG_ReadKeys(); // we re-read the keys after compiling the CONs

    if (initengine())
    {
        wm_msgbox("Build Engine Initialization Error",
                  "There was a problem initializing the Build engine: %s", engineerrstr);
        G_FreeMemory();
        exit(1);
    }

    G_InitDynamicTiles();

    if (ud.multimode > 1) G_CheckGametype();

    if (g_noSound) ud.config.SoundToggle = 0;
    if (g_noMusic) ud.config.MusicToggle = 0;

    if (CommandName)
    {
        //        Bstrncpy(szPlayerName, CommandName, 9);
        //        szPlayerName[10] = '\0';
        Bstrcpy(tempbuf,CommandName);

        while (Bstrlen(stripcolorcodes(tempbuf,tempbuf)) > 10)
            tempbuf[Bstrlen(tempbuf)-1] = '\0';

        Bstrncpy(szPlayerName,tempbuf,sizeof(szPlayerName)-1);
        szPlayerName[sizeof(szPlayerName)-1] = '\0';
    }

    if (CommandMap)
    {
        if (VOLUMEONE)
        {
            initprintf("The -map option is available in the registered version only!\n");
            boardfilename[0] = 0;
        }
        else
        {
            char *dot, *slash;

            boardfilename[0] = '/';
            boardfilename[1] = 0;
            Bstrcat(boardfilename, CommandMap);

            dot = Bstrrchr(boardfilename,'.');
            slash = Bstrrchr(boardfilename,'/');
            if (!slash) slash = Bstrrchr(boardfilename,'\\');

            if ((!slash && !dot) || (slash && dot < slash))
                Bstrcat(boardfilename,".map");

            Bcorrectfilename(boardfilename,0);

            i = kopen4loadfrommod(boardfilename,0);
            if (i!=-1)
            {
                initprintf("Using level: '%s'.\n",boardfilename);
                kclose(i);
            }
            else
            {
                initprintf("Level '%s' not found.\n",boardfilename);
                boardfilename[0] = 0;
            }
        }
    }

    if (VOLUMEONE)
    {
        initprintf("*** You have run Duke Nukem 3D %d times. ***\n\n",ud.executions);
        if (ud.executions >= 50) initprintf("IT IS NOW TIME TO UPGRADE TO THE COMPLETE VERSION!!!\n");
    }

    for (i=0; i<MAXPLAYERS; i++)
        g_player[i].playerreadyflag = 0;

    // TODO: split this up in the same fine-grained manner as eduke32 network backend, to
    // allow for event handling
    mmulti_initmultiplayers(netparamcount,netparam);

    if (quitevent)
    {
        G_Shutdown();
        return;
    }

    if (netparam) Bfree(netparam);
    netparam = NULL;
    netparamcount = 0;

    if (numplayers > 1)
        initprintf("Multiplayer initialized.\n");

    //initprintf("* Hold Esc to Abort. *\n");
//    initprintf("Loading art header...\n");

    {
        char cwd[BMAX_PATH];

        if (getcwd(cwd,BMAX_PATH) && mod_dir[0] != '/')
        {
            chdir(mod_dir);
//            initprintf("root '%s'\nmod '%s'\ncwd '%s'\n",root,mod_dir,cwd);
            if (loadpics("tiles000.art",MAXCACHE1DSIZE) < 0)
            {
                chdir(cwd);
                if (loadpics("tiles000.art",MAXCACHE1DSIZE) < 0)
                    G_GameExit("Failed loading art.");
            }
            chdir(cwd);
        }
        else if (loadpics("tiles000.art",MAXCACHE1DSIZE) < 0)
            G_GameExit("Failed loading art.");
    }

//    initprintf("Loading palette/lookups...\n");
    G_LoadExtraPalettes();

    ReadSaveGameHeaders();

    tilesizx[MIRROR] = tilesizy[MIRROR] = 0;

    screenpeek = myconnectindex;

    if (g_networkBroadcastMode == 255)
        g_networkBroadcastMode = 1;
}

void sendscore(const char *s)
{
    UNREFERENCED_PARAMETER(s);
//    if (numplayers > 1)
//        genericmultifunction(-1,(char *)s,strlen(s)+1,5);
}

int32_t quittimer = 0;

void Net_SendQuit(void)
{
    if (g_gameQuit == 0 && (numplayers > 1))
    {
        if (g_player[myconnectindex].ps->gm&MODE_GAME)
        {
            g_gameQuit = 1;
            quittimer = totalclock+120;
        }
        else
        {
            int32_t i;

            tempbuf[0] = PACKET_MENU_LEVEL_QUIT;
            tempbuf[1] = myconnectindex;

            TRAVERSE_CONNECT(i)
            {
                if (i != myconnectindex) mmulti_sendpacket(i,tempbuf,2);
                if ((!g_networkBroadcastMode) && (myconnectindex != connecthead)) break; //slaves in M/S mode only send to master
            }
            G_GameExit(" ");
        }
    }
    else if (numplayers < 2)
        G_GameExit(" ");

    if ((totalclock > quittimer) && (g_gameQuit == 1))
        G_GameExit("Timed out.");
}

static void Net_SendWeaponChoice(void)
{
    int32_t i,l;

    buf[0] = PACKET_WEAPON_CHOICE;
    buf[1] = myconnectindex;
    l = 2;

    for (i=0; i<10; i++)
    {
        g_player[myconnectindex].wchoice[i] = g_player[0].wchoice[i];
        buf[l++] = (uint8_t)g_player[0].wchoice[i];
    }

    TRAVERSE_CONNECT(i)
    {
        if (i != myconnectindex) mmulti_sendpacket(i,&buf[0],l);
        if ((!g_networkBroadcastMode) && (myconnectindex != connecthead)) break; //slaves in M/S mode only send to master
    }
}

static void Net_SendVersion(void)
{
    int32_t i;

    if (numplayers < 2) return;

    buf[0] = PACKET_VERSION;
    buf[1] = myconnectindex;
    buf[2] = (uint8_t)atoi(s_buildDate);
    buf[3] = BYTEVERSION;
    buf[4] = g_numSyncBytes;

    TRAVERSE_CONNECT(i)
    {
        if (i != myconnectindex) mmulti_sendpacket(i,&buf[0],5);
        if ((!g_networkBroadcastMode) && (myconnectindex != connecthead)) break; //slaves in M/S mode only send to master
    }
    Net_WaitForEverybody();
}

static void Net_SendPlayerOptions(void)
{
    int32_t i,l;

    buf[0] = PACKET_PLAYER_OPTIONS;
    buf[1] = myconnectindex;
    l = 2;

    //null terminated player name to send
//    for (i=0;szPlayerName[i];i++) buf[l++] = Btoupper(szPlayerName[i]);
//    buf[l++] = 0;

    buf[l++] = g_player[myconnectindex].ps->aim_mode = ud.mouseaiming;
    buf[l++] = g_player[myconnectindex].ps->auto_aim = ud.config.AutoAim;
    buf[l++] = g_player[myconnectindex].ps->weaponswitch = ud.weaponswitch;
    buf[l++] = g_player[myconnectindex].ps->palookup = g_player[myconnectindex].pcolor = ud.color;

    buf[l++] = g_player[myconnectindex].pteam = ud.team;

    TRAVERSE_CONNECT(i)
    {
        if (i != myconnectindex) mmulti_sendpacket(i,&buf[0],l);
        if ((!g_networkBroadcastMode) && (myconnectindex != connecthead)) break; //slaves in M/S mode only send to master
    }
}

void Net_SendPlayerName(void)
{
    int32_t i,l;

    for (l=0; (unsigned)l<sizeof(szPlayerName)-1; l++)
        g_player[myconnectindex].user_name[l] = Btoupper(szPlayerName[l]);

    if (numplayers < 2) return;

    buf[0] = PACKET_PLAYER_NAME;
    buf[1] = myconnectindex;
    l = 2;

    //null terminated player name to send
    for (i=0; szPlayerName[i]; i++) buf[l++] = Btoupper(szPlayerName[i]);
    buf[l++] = 0;

    TRAVERSE_CONNECT(i)
    {
        if (i != myconnectindex) mmulti_sendpacket(i,&buf[0],l);
        if ((!g_networkBroadcastMode) && (myconnectindex != connecthead)) break; //slaves in M/S mode only send to master
    }
}

void Net_SendUserMapName(void)
{
    if (numplayers > 1)
    {
        int32_t j;
        int32_t ch;

        packbuf[0] = PACKET_USER_MAP;
        packbuf[1] = 0;

        Bcorrectfilename(boardfilename,0);

        j = Bstrlen(boardfilename);
        boardfilename[j++] = 0;
        Bstrcat(packbuf+1,boardfilename);

        TRAVERSE_CONNECT(ch)
        {
            if (ch != myconnectindex) mmulti_sendpacket(ch,packbuf,j);
            if ((!g_networkBroadcastMode) && (myconnectindex != connecthead)) break; //slaves in M/S mode only send to master
        }
    }
}

void Net_NewGame(int32_t volume, int32_t level)
{
    int32_t i;

    packbuf[0] = PACKET_NEW_GAME;
    packbuf[1] = ud.m_level_number = level;
    packbuf[2] = ud.m_volume_number = volume;
    packbuf[3] = ud.m_player_skill+1;
    packbuf[4] = ud.m_monsters_off;
    packbuf[5] = ud.m_respawn_monsters;
    packbuf[6] = ud.m_respawn_items;
    packbuf[7] = ud.m_respawn_inventory;
    packbuf[8] = ud.m_coop;
    packbuf[9] = ud.m_marker;
    packbuf[10] = ud.m_ffire;
    packbuf[11] = ud.m_noexits;

    TRAVERSE_CONNECT(i)
    {
        if (i != myconnectindex) mmulti_sendpacket(i,packbuf,12);
        if ((!g_networkBroadcastMode) && (myconnectindex != connecthead)) break; //slaves in M/S mode only send to master
    }
}

void G_UpdatePlayerFromMenu(void)
{
    if (ud.recstat != 0)
        return;

    if (numplayers > 1)
    {
        Net_SendPlayerOptions();
        if (sprite[g_player[myconnectindex].ps->i].picnum == APLAYER && sprite[g_player[myconnectindex].ps->i].pal != 1)
            sprite[g_player[myconnectindex].ps->i].pal = g_player[myconnectindex].pcolor;
    }
    else
    {
        /*int32_t j = g_player[myconnectindex].ps->team;*/

        g_player[myconnectindex].ps->aim_mode = ud.mouseaiming;
        g_player[myconnectindex].ps->auto_aim = ud.config.AutoAim;
        g_player[myconnectindex].ps->weaponswitch = ud.weaponswitch;
        g_player[myconnectindex].ps->palookup = g_player[myconnectindex].pcolor = ud.color;

        g_player[myconnectindex].pteam = ud.team;

        if (sprite[g_player[myconnectindex].ps->i].picnum == APLAYER && sprite[g_player[myconnectindex].ps->i].pal != 1)
            sprite[g_player[myconnectindex].ps->i].pal = g_player[myconnectindex].pcolor;
    }
}

void G_BackToMenu(void)
{
    boardfilename[0] = 0;
    if (ud.recstat == 1) G_CloseDemoWrite();
    ud.warp_on = 0;
    g_player[myconnectindex].ps->gm = MODE_MENU;
    ChangeToMenu(0);
    KB_FlushKeyboardQueue();
    Bsprintf(tempbuf,APPNAME " - %s",duke3dgrpstring);
    wm_setapptitle(tempbuf);
}

#ifdef RENDERTYPEWIN
void app_crashhandler(void)
{
    G_CloseDemoWrite();
    X_ScriptInfo();
    Net_SendQuit();
}
#endif

void app_main(int32_t argc,const char **argv)
{
    int32_t i = 0, j;
    char cwd[BMAX_PATH];
//    extern char datetimestring[];

#ifdef RENDERTYPEWIN
    if (argc > 1)
    {
        for (; i<argc; i++)
        {
            if (Bstrcasecmp("-noinstancechecking",*(&argv[i])) == 0)
                break;
        }
    }
    if (i == argc && win_checkinstance())
    {
        if (!wm_ynbox("EDuke32","Another Build game is currently running. "
                      "Do you wish to continue starting this copy?"))
            return;
    }
#endif

#ifdef RENDERTYPEWIN
    backgroundidle = 0;
#endif

#ifdef _WIN32
    tempbuf[GetModuleFileName(NULL,root,BMAX_PATH)] = 0;
    Bcorrectfilename(root,1);
    chdir(root);
#else
    getcwd(root,BMAX_PATH);
    strcat(root,"/");
#endif

    OSD_SetLogFile("eduke32.log");
    OSD_SetParameters(0,0, 0,12, 2,12);
    OSD_SetFunctions(
        GAME_drawosdchar,
        GAME_drawosdstr,
        GAME_drawosdcursor,
        GAME_getcolumnwidth,
        GAME_getrowheight,
        GAME_clearbackground,
        (int32_t(*)(void))GetTime,
        GAME_onshowosd
    );
    Bsprintf(tempbuf,HEAD2 " %s",s_buildDate);
    wm_setapptitle(tempbuf);

    initprintf("%s\n",apptitle);
//    initprintf("Compiled %s\n",datetimestring);
//    initprintf("Copyright (c) 1996, 2003 3D Realms Entertainment\n");
//    initprintf("Copyright (c) 2008 EDuke32 team and contributors\n");

#if defined(__linux__) || defined(__FreeBSD__) || defined(__NetBSD__) || defined(__OpenBSD__)
    addsearchpath("/usr/share/games/jfduke3d");
    addsearchpath("/usr/local/share/games/jfduke3d");
    addsearchpath("/usr/share/games/eduke32");
    addsearchpath("/usr/local/share/games/eduke32");
#elif defined(__APPLE__)
    addsearchpath("/Library/Application Support/JFDuke3D");
    addsearchpath("/Library/Application Support/EDuke32");
#endif

    ud.multimode = 1;

    initsynccrc();

    G_CheckCommandLine(argc,argv);

#if defined(RENDERTYPEWIN) && defined(USE_OPENGL)
    if (forcegl) initprintf("GL driver blacklist disabled.\n");
#endif

    g_player[0].ps = (DukePlayer_t *) Bcalloc(1, sizeof(DukePlayer_t));
    g_player[0].sync = (input_t *) Bcalloc(1, sizeof(input_t));

    if (getcwd(cwd,BMAX_PATH))
    {
        addsearchpath(cwd);
#if defined(__APPLE__)
        /* Dirty hack on OS X to also look for gamedata inside the application bundle - rhoenie 08/08 */
        char seekinappcontainer[BMAX_PATH];
        Bsnprintf(seekinappcontainer,sizeof(seekinappcontainer),"%s/EDuke32.app/", cwd);
        addsearchpath(seekinappcontainer);
#endif
    }

    if (CommandPaths)
    {
        struct strllist *s;
        while (CommandPaths)
        {
            s = CommandPaths->next;
            addsearchpath(CommandPaths->str);

            Bfree(CommandPaths->str);
            Bfree(CommandPaths);
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
        int32_t asperr;

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

    // used with binds for fast function lookup
    hash_init(&gamefuncH);
    for (i=NUMGAMEFUNCTIONS-1; i>=0; i--)
    {
        char *str = strtolower(Bstrdup(gamefunctions[i]),Bstrlen(gamefunctions[i]));
        hash_add(&gamefuncH,gamefunctions[i],i);
        hash_add(&gamefuncH,str,i);
        Bfree(str);
    }

#if defined(POLYMOST) && defined(USE_OPENGL)
    glusetexcache = -1;
#endif

    i = CONFIG_ReadSetup();
    if (getenv("DUKE3DGRP")) duke3dgrp = getenv("DUKE3DGRP");

#ifdef _WIN32

//    initprintf("build %d\n",(uint8_t)atoi(BUILDDATE));

    if (ud.config.CheckForUpdates == 1)
    {
        if (time(NULL) - ud.config.LastUpdateCheck > UPDATEINTERVAL)
        {
            initprintf("Checking for updates...\n");
            if (G_GetVersionFromWebsite(tempbuf))
            {
                initprintf("Current version is %d",atoi(tempbuf));
                ud.config.LastUpdateCheck = time(NULL);

                if (atoi(tempbuf) > atoi(s_buildDate))
                {
                    if (wm_ynbox("EDuke32","A new version of EDuke32 is available. "
                                 "Browse to http://eduke32.sourceforge.net now?"))
                    {
                        SHELLEXECUTEINFOA sinfo;
                        char *p = "http://eduke32.sourceforge.net";

                        Bmemset(&sinfo, 0, sizeof(sinfo));
                        sinfo.cbSize = sizeof(sinfo);
                        sinfo.fMask = SEE_MASK_CLASSNAME;
                        sinfo.lpVerb = "open";
                        sinfo.lpFile = p;
                        sinfo.nShow = SW_SHOWNORMAL;
                        sinfo.lpClass = "http";

                        if (!ShellExecuteExA(&sinfo))
                            initprintf("update: error launching browser!\n");
                    }
                }
                else initprintf("... no updates available\n");
            }
            else initprintf("update: failed to check for updates\n");
        }
    }
#endif

#if defined(POLYMOST) && defined(USE_OPENGL)
    if (glusetexcache == -1)
    {
        ud.config.useprecache = glusetexcompr = 1;
        glusetexcache = 2;
    }
#endif

    if (preinitengine())
    {
        wm_msgbox("Build Engine Initialization Error",
                  "There was a problem initializing the Build engine: %s", engineerrstr);
        exit(1);
    }

    initprintf("Using config file '%s'.\n",setupfilename);

    ScanGroups();
    {
        // try and identify the 'defaultduke3dgrp' in the set of GRPs.
        // if it is found, set up the environment accordingly for the game it represents.
        // if it is not found, choose the first GRP from the list
        struct grpfile *fg, *first = NULL;
        int32_t i;
        for (fg = foundgrps; fg; fg=fg->next)
        {
            for (i = 0; i<numgrpfiles; i++) if (fg->crcval == grpfiles[i].crcval) break;
            if (i == numgrpfiles) continue; // unrecognised grp file
            fg->game = grpfiles[i].game;
            if (!first) first = fg;
            if (!Bstrcasecmp(fg->name, defaultduke3dgrp))
            {
                g_gameType = grpfiles[i].game;
                duke3dgrpstring = (char *)grpfiles[i].name;
                break;
            }
        }
        if (!fg && first)
        {
            Bstrcpy(defaultduke3dgrp, first->name);
            g_gameType = first->game;
            duke3dgrpstring = (char *)grpfiles[0].name;
        }
        else if (!fg) duke3dgrpstring = "Unknown GRP";
    }

#if (defined RENDERTYPEWIN || (defined RENDERTYPESDL && !defined __APPLE__ && defined HAVE_GTK2))
    if (i < 0 || (!g_noSetup && (ud.configversion != BYTEVERSION_JF || ud.config.ForceSetup)) || g_commandSetup)
    {
        if (quitevent || !startwin_run())
        {
            uninitengine();
            exit(0);
        }
    }
#endif

    FreeGroups();

    if (WW2GI || NAM)
    {
        // overwrite the default GRP and CON so that if the user chooses
        // something different, they get what they asked for
        if (WW2GI)
        {
            Bsprintf(defaultduke3dgrp,"ww2gi.grp");
            Bsprintf(defaultduke3ddef,"ww2gi.def");
            Bsprintf(defaultconfilename, "ww2gi.con");
        }
        else
        {
            Bsprintf(defaultduke3dgrp,"nam.grp");
            Bsprintf(defaultduke3ddef,"nam.def");
            Bsprintf(defaultconfilename, "nam.con");
        }

        Bsprintf(GametypeNames[0],"GRUNTMATCH (SPAWN)");
        Bsprintf(GametypeNames[2],"GRUNTMATCH (NO SPAWN)");
    }

    if (mod_dir[0] != '/')
    {
        char cwd[BMAX_PATH];

        Bstrcat(root,mod_dir);
        addsearchpath(root);
//        addsearchpath(mod_dir);

        if (getcwd(cwd,BMAX_PATH))
        {
            Bsprintf(cwd,"%s/%s",cwd,mod_dir);
            if (!Bstrcmp(root, cwd))
            {
                if (addsearchpath(cwd) == -2)
                    if (Bmkdir(cwd,S_IRWXU) == 0) addsearchpath(cwd);
            }
        }

#if defined(POLYMOST) && defined(USE_OPENGL)
        Bsprintf(tempbuf,"%s/%s",mod_dir,TEXCACHEFILE);
        Bstrcpy(TEXCACHEFILE,tempbuf);
#endif
    }

    // shitcan the old cache directory
#if defined(POLYMOST) && defined(USE_OPENGL)
    {
        struct stat st;
        char dir[BMAX_PATH];

        if (mod_dir[0] != '/')
            Bsprintf(dir,"%s/",mod_dir);
        else dir[0] = '\0';

        Bsprintf(tempbuf,"%stexcache",dir);
        if (stat(tempbuf, &st) >= 0)
        {
            if ((st.st_mode & S_IFDIR) == S_IFDIR)
            {
                Bsprintf(tempbuf,"EDuke32 has located an obsolete texture cache in the \"%stexcache\" directory.\n\n"
                         "Would you like EDuke32 to purge the contents of this directory?",dir);

                if (wm_ynbox("Obsolete Texture Cache Detected",tempbuf))
                {
                    int32_t recursion = 0;

                    Bsprintf(tempbuf,"%stexcache",dir);
                    getfilenames(tempbuf,"*");
CLEAN_DIRECTORY:
                    // initprintf("Cleaning '%s'\n",tempbuf);
                    while (findfiles)
                    {
                        Bsprintf(g_szBuf,"%s/%s",tempbuf,findfiles->name);
                        if (unlink(g_szBuf))
                            initprintf("ERROR: couldn't remove '%s': %s\n",g_szBuf,strerror(errno));
                        findfiles = findfiles->next;
                    }
                    while (finddirs)
                    {
                        if (!Bstrcmp(finddirs->name, ".") || !Bstrcmp(finddirs->name, ".."))
                        {
                            finddirs = finddirs->next;
                            continue;
                        }
                        Bsprintf(g_szBuf,"%s/%s",tempbuf,finddirs->name);
                        if (rmdir(g_szBuf))
                        {
                            if (errno == EEXIST || errno == ENOTEMPTY)
                            {
                                recursion = 1;
                                Bstrcpy(tempbuf,g_szBuf);
                                getfilenames(tempbuf,"*");
                                goto CLEAN_DIRECTORY;
                            }
                            else
                            {
                                initprintf("ERROR: couldn't remove '%s': %s\n",g_szBuf,strerror(errno));
                            }
                        }
                        else
                        {
                            initprintf("Removed '%s'\n",g_szBuf);
                            finddirs = finddirs->next;
                        }
                    }

                    if (recursion)
                    {
                        Bsprintf(tempbuf,"%stexcache",dir);
                        getfilenames(tempbuf,"*");
                        recursion = 0;
                        goto CLEAN_DIRECTORY;
                    }

                    Bsprintf(tempbuf,"%stexcache",dir);
                    if (rmdir(tempbuf))
                        initprintf("ERROR: couldn't remove '%s': %s\n",tempbuf,strerror(errno));
                    else initprintf("Removed '%s'\n",tempbuf);
                }
            }
        }
    }
#endif

    i = initgroupfile(duke3dgrp);

    if (i == -1)
        initprintf("Warning: could not find main group file '%s'!\n",duke3dgrp);
    else
        initprintf("Using group file '%s' as main group file.\n", duke3dgrp);

    if (!g_noAutoLoad && !ud.config.NoAutoLoad)
    {
        int32_t ii;

        for (ii=0; ii<NUMAUTOLOADMASKS; ii++)
        {
            getfilenames("autoload",autoloadmasks[ii]);
            while (findfiles)
            {
                Bsprintf(tempbuf,"autoload/%s",findfiles->name);
                initprintf("Using group file '%s'.\n",tempbuf);
                initgroupfile(tempbuf);
                findfiles = findfiles->next;
            }
        }

        if (i != -1)
            G_DoAutoload(duke3dgrp);
    }

    if (mod_dir[0] != '/')
    {
        int32_t ii;

        for (ii=0; ii<NUMAUTOLOADMASKS; ii++)
        {
            Bsprintf(tempbuf,"%s/",mod_dir);
            getfilenames(tempbuf,autoloadmasks[ii]);
            while (findfiles)
            {
                Bsprintf(tempbuf,"%s/%s",mod_dir,findfiles->name);
                initprintf("Using group file '%s'.\n",tempbuf);
                initgroupfile(tempbuf);
                findfiles = findfiles->next;
            }
        }
    }

    flushlogwindow = 0;
    loaddefinitions_game(duke3ddef, TRUE);
//    flushlogwindow = 1;

    {
        struct strllist *s;

        pathsearchmode = 1;
        while (CommandGrps)
        {
            s = CommandGrps->next;
            j = initgroupfile(CommandGrps->str);
            if (j == -1) initprintf("Could not find group file '%s'.\n",CommandGrps->str);
            else
            {
                g_groupFileHandle = j;
                initprintf("Using group file '%s'.\n",CommandGrps->str);
                if (!g_noAutoLoad && !ud.config.NoAutoLoad)
                    G_DoAutoload(CommandGrps->str);
            }

            Bfree(CommandGrps->str);
            Bfree(CommandGrps);
            CommandGrps = s;
        }
        pathsearchmode = 0;
    }

    i = kopen4load("DUKESW.BIN",1); // JBF 20030810
    if (i!=-1)
    {
        g_Shareware = 1;
        kclose(i);
    }

    if (netparamcount > 0) _buildargc = (argc -= netparamcount+1);  // crop off the net parameters

    // gotta set the proper title after we compile the CONs if this is the full version

    Bsprintf(tempbuf,"%s - " APPNAME,duke3dgrpstring);
    wm_setapptitle(tempbuf);

    if (g_scriptDebug)
        initprintf("CON debugging activated (level %d).\n",g_scriptDebug);

    RegisterShutdownFunction(G_Shutdown);

    G_Startup(); // a bunch of stuff including compiling cons

    if (numplayers > 1)
        ud.multimode = numplayers;

    for (i=1; i<ud.multimode; i++)
    {
        g_player[i].ps = (DukePlayer_t *) Bcalloc(1,sizeof(DukePlayer_t));
        g_player[i].sync = (input_t *) Bcalloc(1,sizeof(input_t));
    }

    g_player[myconnectindex].ps->palette = (uint8_t *) &palette[0];

    i = 1;
    for (j=numplayers; j<ud.multimode; j++)
    {
        Bsprintf(g_player[j].user_name,"PLAYER %d",j+1);
        g_player[j].ps->team = g_player[j].pteam = i;
        g_player[j].ps->weaponswitch = 3;
        g_player[j].ps->auto_aim = 0;
        i = 1-i;
    }

    if (quitevent) return;

    if (!loaddefinitionsfile(duke3ddef))
    {
        initprintf("Definitions file '%s' loaded.\n",duke3ddef);
        loaddefinitions_game(duke3ddef, FALSE);
    }

    if (numplayers > 1)
    {
        Net_SendVersion();
        Net_SendPlayerName();
        Net_SendPlayerOptions();
        Net_SendWeaponChoice();
        Net_SendUserMapName();
        Net_GetPackets();
        Net_WaitForEverybody();
    }
    else if (boardfilename[0] != 0)
    {
        ud.m_level_number = 7;
        ud.m_volume_number = 0;
        ud.warp_on = 1;
    }

    // getnames();

    if (ud.multimode > 1)
    {
        if (ud.warp_on == 0)
        {
            ud.m_monsters_off = 1;
            ud.m_player_skill = 0;
        }
    }

    playerswhenstarted = ud.multimode;
    ud.last_level = 0;

    if (!Bstrcasecmp(ud.rtsname,"DUKE.RTS") ||
        !Bstrcasecmp(ud.rtsname,"WW2GI.RTS") ||
        !Bstrcasecmp(ud.rtsname,"NAM.RTS"))
    {
        // ud.last_level is used as a flag here to reset the string to DUKE.RTS after load
        if (WW2GI)
            ud.last_level = (Bstrcpy(ud.rtsname, "WW2GI.RTS") == ud.rtsname);
        else if (NAM)
            ud.last_level = (Bstrcpy(ud.rtsname, "NAM.RTS") == ud.rtsname);
        else
            ud.last_level = (Bstrcpy(ud.rtsname, "DUKE.RTS") == ud.rtsname);
    }

    RTS_Init(ud.rtsname);

    if (numlumps)
        initprintf("Using .RTS file '%s'\n",ud.rtsname);

    if (ud.last_level)
        Bstrcpy(ud.rtsname, "DUKE.RTS");

    ud.last_level = -1;

    initprintf("Initializing OSD...\n");

    Bsprintf(tempbuf, HEAD2 " %s", s_buildDate);
    OSD_SetVersionString(tempbuf, 10,0);
    registerosdcommands();

    if (CONTROL_Startup(1, &GetTime, TICRATE))
    {
        uninitengine();
        exit(1);
    }

    G_SetupGameButtons();
    CONFIG_SetupMouse();
    CONFIG_SetupJoystick();

    CONTROL_JoystickEnabled = (ud.config.UseJoystick && CONTROL_JoyPresent);
    CONTROL_MouseEnabled = (ud.config.UseMouse && CONTROL_MousePresent);

    // JBF 20040215: evil and nasty place to do this, but joysticks are evil and nasty too
    for (i=0; i<joynumaxes; i++)
        setjoydeadzone(i,ud.config.JoystickAnalogueDead[i],ud.config.JoystickAnalogueSaturate[i]);

    {
        char *ptr = Bstrdup(setupfilename), *p = strtok(ptr,".");
        Bsprintf(tempbuf,"%s_binds.cfg",p);
        OSD_Exec(tempbuf);
        Bfree(ptr);
    }

    OSD_Exec("autoexec.cfg");

    if (setgamemode(ud.config.ScreenMode,ud.config.ScreenWidth,ud.config.ScreenHeight,ud.config.ScreenBPP) < 0)
    {
        int32_t i = 0;
        int32_t xres[] = {ud.config.ScreenWidth,800,640,320};
        int32_t yres[] = {ud.config.ScreenHeight,600,480,240};
        int32_t bpp[] = {32,16,8};

        initprintf("Failure setting video mode %dx%dx%d %s! Attempting safer mode...\n",
                   ud.config.ScreenWidth,ud.config.ScreenHeight,ud.config.ScreenBPP,ud.config.ScreenMode?"fullscreen":"windowed");

#if defined(POLYMOST) && defined(USE_OPENGL)
        {
            int32_t j = 0;
            while (setgamemode(0,xres[i],yres[i],bpp[j]) < 0)
            {
                initprintf("Failure setting video mode %dx%dx%d windowed! Attempting safer mode...\n",xres[i],yres[i],bpp[i]);
                j++;
                if (j == 3)
                {
                    i++;
                    j = 0;
                }
                if (i == 4)
                    G_GameExit("Unable to set failsafe video mode!");
            }
        }
#else
        while (setgamemode(0,xres[i],yres[i],8) < 0)
        {
            initprintf("Failure setting video mode %dx%dx%d windowed! Attempting safer mode...\n",xres[i],yres[i],8);
            i++;
        }
#endif
        ud.config.ScreenWidth = xres[i];
        ud.config.ScreenHeight = yres[i];
        ud.config.ScreenBPP = bpp[i];
    }

    setbrightness(ud.brightness>>2,&g_player[myconnectindex].ps->palette[0],0);

    S_MusicStartup();
    S_SoundStartup();
//    loadtmb();

    if (ud.warp_on > 1 && ud.multimode < 2)
    {
        clearview(0L);
        //g_player[myconnectindex].ps->palette = palette;
        //G_FadePalette(0,0,0,0);
        P_SetGamePalette(g_player[myconnectindex].ps, palette, 0);    // JBF 20040308
        rotatesprite(320<<15,200<<15,65536L,0,LOADSCREEN,0,0,2+8+64,0,0,xdim-1,ydim-1);
        menutext(160,105,0,0,"LOADING SAVED GAME...");
        nextpage();

        if (G_LoadPlayer(ud.warp_on-2))
            ud.warp_on = 0;
    }

    FX_StopAllSounds();
    S_ClearSoundLocks();

    //    getpackets();

MAIN_LOOP_RESTART:

    G_GetCrosshairColor();
    G_SetCrosshairColor(CrosshairColors.r, CrosshairColors.g, CrosshairColors.b);

    if (ud.warp_on == 0)
    {
        if (ud.multimode > 1 && boardfilename[0] != 0)
        {
            ud.m_level_number = 7;
            ud.m_volume_number = 0;

            if (ud.m_player_skill == 4)
                ud.m_respawn_monsters = 1;
            else ud.m_respawn_monsters = 0;

            Net_WaitForEverybody();

            TRAVERSE_CONNECT(i)
            {
                P_ResetWeapons(i);
                P_ResetInventory(i);
            }

            G_NewGame(ud.m_volume_number,ud.m_level_number,ud.m_player_skill);

            if (G_EnterLevel(MODE_GAME)) G_BackToMenu();
        }
        else G_DisplayLogo();

        if (G_PlaybackDemo())
        {
            FX_StopAllSounds();
            S_ClearSoundLocks();
            g_noLogoAnim = 1;
            goto MAIN_LOOP_RESTART;
        }
    }
    else if (ud.warp_on == 1)
    {
        G_NewGame(ud.m_volume_number,ud.m_level_number,ud.m_player_skill);

        if (G_EnterLevel(MODE_GAME)) G_BackToMenu();
    }
    else G_UpdateScreenArea();

    ud.auto_run = ud.config.RunMode;
    ud.showweapons = ud.config.ShowOpponentWeapons;
    g_player[myconnectindex].ps->aim_mode = ud.mouseaiming;
    g_player[myconnectindex].ps->auto_aim = ud.config.AutoAim;
    g_player[myconnectindex].ps->weaponswitch = ud.weaponswitch;
    g_player[myconnectindex].pteam = ud.team;

    if (GametypeFlags[ud.coop] & GAMETYPE_TDM)
        g_player[myconnectindex].ps->palookup = g_player[myconnectindex].pcolor = G_GetTeamPalette(g_player[myconnectindex].pteam);
    else
    {
        if (ud.color) g_player[myconnectindex].ps->palookup = g_player[myconnectindex].pcolor = ud.color;
        else g_player[myconnectindex].ps->palookup = g_player[myconnectindex].pcolor;
    }

    ud.warp_on = 0;
    KB_KeyDown[sc_Pause] = 0;   // JBF: I hate the pause key

    do //main loop
    {
        if (handleevents() && quitevent)
        {
            // JBF
            KB_KeyDown[sc_Escape] = 1;
            quitevent = 0;
        }

        MUSIC_Update();

        // only allow binds to function if the player is actually in a game (not in a menu, typing, et cetera) or demo
        bindsenabled = (g_player[myconnectindex].ps->gm == MODE_GAME || g_player[myconnectindex].ps->gm == MODE_DEMO);

        OSD_DispatchQueued();

        if (((ud.show_help == 0 && (g_player[myconnectindex].ps->gm&MODE_MENU) != MODE_MENU) || ud.recstat == 2 || ud.multimode > 1) &&
                (g_player[myconnectindex].ps->gm&MODE_GAME) && G_MoveLoop())
            continue;

        if (g_player[myconnectindex].ps->gm & (MODE_EOL|MODE_RESTART))
        {
            P_SetGamePalette(g_player[myconnectindex].ps, palette, 0);
            P_UpdateScreenPal(g_player[myconnectindex].ps);

            if (g_player[myconnectindex].ps->gm&MODE_EOL)
            {
                G_CloseDemoWrite();
                Net_WaitForEverybody();

                ready2send = 0;

                if (ud.display_bonus_screen == 1)
                {
                    i = ud.screen_size;
                    ud.screen_size = 0;
                    G_UpdateScreenArea();
                    ud.screen_size = i;
                    G_BonusScreen(0);
                }
                if (ud.eog)
                {
                    ud.eog = 0;
                    if (ud.multimode < 2)
                    {
                        if (!VOLUMEALL)
                            G_DoOrderScreen();
                        g_player[myconnectindex].ps->gm = MODE_MENU;
                        ChangeToMenu(0);
                        probey = 0;
                        goto MAIN_LOOP_RESTART;
                    }
                    else
                    {
                        ud.m_level_number = 0;
                        ud.level_number = 0;
                    }
                }
            }
            ud.display_bonus_screen = 1;
            ready2send = 0;
            Net_WaitForEverybody();
            if (numplayers > 1) g_player[myconnectindex].ps->gm = MODE_GAME;
            if (G_EnterLevel(g_player[myconnectindex].ps->gm))
            {
                G_BackToMenu();
                goto MAIN_LOOP_RESTART;
            }
            continue;
        }

        G_DoCheats();
        G_HandleLocalKeys();

        if ((ud.show_help == 0 && ud.multimode < 2 && !(g_player[myconnectindex].ps->gm&MODE_MENU)) || ud.multimode > 1 || ud.recstat == 2)
            i = min(max((totalclock-ototalclock)*(65536L/TICSPERFRAME),0),65536);
        else
            i = 65536;

        if (ud.statusbarmode == 1 && (ud.statusbarscale == 100 || !getrendermode()))
        {
            ud.statusbarmode = 0;
            G_UpdateScreenArea();
        }

        {
            static uint32_t nextrender = 0;
            uint32_t j = getticks();

            if (j > nextrender+g_frameDelay)
                nextrender = j;

            if (r_maxfps == 0 || j >= nextrender)
            {
                nextrender += g_frameDelay;

                G_DrawRooms(screenpeek,i);
                if (getrendermode() >= 3)
                    G_DrawBackground();
                G_DisplayRest(i);
                S_Pan3D();

                if (g_player[myconnectindex].gotvote == 0 && voting != -1 && voting != myconnectindex)
                {
                    Bsprintf(tempbuf,"%s^00 HAS CALLED A VOTE FOR MAP",g_player[voting].user_name);
                    gametext(160,40,tempbuf,0,2+8+16);
                    Bsprintf(tempbuf,"%s (E%dL%d)",MapInfo[vote_episode*MAXLEVELS + vote_map].name,vote_episode+1,vote_map+1);
                    gametext(160,48,tempbuf,0,2+8+16);
                    gametext(160,70,"PRESS F1 TO ACCEPT, F2 TO DECLINE",0,2+8+16);
                }

                if (BUTTON(gamefunc_Show_DukeMatch_Scores)) G_ShowScores();

                if (debug_on) G_ShowCacheLocks();

                Net_DisplaySyncMsg();

                if (VOLUMEONE)
                {
                    if (ud.show_help == 0 && g_showShareware > 0 && (g_player[myconnectindex].ps->gm&MODE_MENU) == 0)
                        rotatesprite((320-50)<<16,9<<16,65536L,0,BETAVERSION,0,0,2+8+16+128,0,0,xdim-1,ydim-1);
                }

                nextpage();
            }
        }

        if (g_player[myconnectindex].ps->gm&MODE_DEMO)
            goto MAIN_LOOP_RESTART;

        while (!(g_player[myconnectindex].ps->gm&MODE_MENU) && ready2send && totalclock >= ototalclock+TICSPERFRAME)
            faketimerhandler();
    }
    while (!(g_player[myconnectindex].ps->gm&MODE_END));

    G_GameExit(" ");
}

static int32_t demo_version;

static int32_t G_OpenDemoRead(int32_t g_whichDemo) // 0 = mine
{
    char d[13];
    char ver;
    int32_t i;

    Bstrcpy(d, "demo_.dmo");

    if (g_whichDemo == 10)
        d[4] = 'x';
    else
        d[4] = '0' + g_whichDemo;

    ud.reccnt = 0;

    if (g_whichDemo == 1 && firstdemofile[0] != 0)
    {
        if ((recfilep = kopen4loadfrommod(firstdemofile,g_loadFromGroupOnly)) == -1) return(0);
    }
    else if ((recfilep = kopen4loadfrommod(d,g_loadFromGroupOnly)) == -1) return(0);

    if (kread(recfilep,&ud.reccnt,sizeof(int32_t)) != sizeof(int32_t)) goto corrupt;
    if (kread(recfilep,&ver,sizeof(uint8_t)) != sizeof(uint8_t)) goto corrupt;

    if (ver != BYTEVERSION /*&& ver != 116 && ver != 117*/)
    {
        /* old demo playback */
        if (ver == BYTEVERSION_JF)   initprintf("Demo %s is for Regular edition.\n", d);
        else if (ver == BYTEVERSION_JF+1) initprintf("Demo %s is for Atomic edition.\n", d);
        else if (ver == BYTEVERSION_JF+2) initprintf("Demo %s is for Shareware version.\n", d);
//        else OSD_Printf("Demo %s is of an incompatible version (%d).\n", d, ver);
        kclose(recfilep);
        ud.reccnt=0;
        demo_version = 0;
        return 0;
    }
    else
    {
        demo_version = ver;
        OSD_Printf("Demo %s is of version %d.\n", d, ver);
    }

    if (kread(recfilep,(char *)&ud.volume_number,sizeof(uint8_t)) != sizeof(uint8_t)) goto corrupt;
    OSD_Printf("ud.volume_number: %d\n",ud.volume_number);
    if (kread(recfilep,(char *)&ud.level_number,sizeof(uint8_t)) != sizeof(uint8_t)) goto corrupt;
    OSD_Printf("ud.level_number: %d\n",ud.level_number);
    if (kread(recfilep,(char *)&ud.player_skill,sizeof(uint8_t)) != sizeof(uint8_t)) goto corrupt;
    OSD_Printf("ud.player_skill: %d\n",ud.player_skill);
    if (kread(recfilep,(char *)&ud.m_coop,sizeof(uint8_t)) != sizeof(uint8_t)) goto corrupt;
    OSD_Printf("ud.m_coop: %d\n",ud.m_coop);
    if (kread(recfilep,(char *)&ud.m_ffire,sizeof(uint8_t)) != sizeof(uint8_t)) goto corrupt;
    OSD_Printf("ud.m_ffire: %d\n",ud.m_ffire);
    if (kread(recfilep,(int16_t *)&ud.multimode,sizeof(int16_t)) != sizeof(int16_t)) goto corrupt;
    OSD_Printf("ud.multimode: %d\n",ud.multimode);
    if (kread(recfilep,(int16_t *)&ud.m_monsters_off,sizeof(int16_t)) != sizeof(int16_t)) goto corrupt;
    OSD_Printf("ud.m_monsters_off: %d\n",ud.m_monsters_off);
    if (kread(recfilep,(int32_t *)&ud.m_respawn_monsters,sizeof(int32_t)) != sizeof(int32_t)) goto corrupt;
    OSD_Printf("ud.m_respawn_monsters: %d\n",ud.m_respawn_monsters);
    if (kread(recfilep,(int32_t *)&ud.m_respawn_items,sizeof(int32_t)) != sizeof(int32_t)) goto corrupt;
    OSD_Printf("ud.m_respawn_items: %d\n",ud.m_respawn_items);
    if (kread(recfilep,(int32_t *)&ud.m_respawn_inventory,sizeof(int32_t)) != sizeof(int32_t)) goto corrupt;
    OSD_Printf("ud.m_respawn_inventory: %d\n",ud.m_respawn_inventory);
    if (kread(recfilep,(int32_t *)&ud.playerai,sizeof(int32_t)) != sizeof(int32_t)) goto corrupt;
    OSD_Printf("ud.playerai: %d\n",ud.playerai);
    if (kread(recfilep,(int32_t *)&i,sizeof(int32_t)) != sizeof(int32_t)) goto corrupt;

    if (kread(recfilep,(char *)boardfilename,sizeof(boardfilename)) != sizeof(boardfilename)) goto corrupt;

    if (boardfilename[0] != 0)
    {
        ud.m_level_number = 7;
        ud.m_volume_number = 0;
    }

    if (kread(recfilep,(int32_t *)&ud.m_noexits,sizeof(int32_t)) != sizeof(int32_t)) goto corrupt;

    for (i=0; i<ud.multimode; i++)
    {
        if (!g_player[i].ps)
            g_player[i].ps = (DukePlayer_t *) Bcalloc(1,sizeof(DukePlayer_t));
        if (!g_player[i].sync)
            g_player[i].sync = (input_t *) Bcalloc(1,sizeof(input_t));

        if (kread(recfilep,(char *)g_player[i].user_name,sizeof(g_player[i].user_name)) != sizeof(g_player[i].user_name)) goto corrupt;
        OSD_Printf("ud.user_name: %s\n",g_player[i].user_name);
        if (kread(recfilep,(int32_t *)&g_player[i].ps->aim_mode,sizeof(int32_t)) != sizeof(int32_t)) goto corrupt;
        if (kread(recfilep,(int32_t *)&g_player[i].ps->auto_aim,sizeof(int32_t)) != sizeof(int32_t)) goto corrupt;  // JBF 20031126
        if (kread(recfilep,(int32_t *)&g_player[i].ps->weaponswitch,sizeof(int32_t)) != sizeof(int32_t)) goto corrupt;
        if (kread(recfilep,(int32_t *)&g_player[i].pcolor,sizeof(int32_t)) != sizeof(int32_t)) goto corrupt;
        g_player[i].ps->palookup = g_player[i].pcolor;
        if (kread(recfilep,(int32_t *)&g_player[i].pteam,sizeof(int32_t)) != sizeof(int32_t)) goto corrupt;
        g_player[i].ps->team = g_player[i].pteam;
    }
    i = ud.reccnt/((TICRATE/TICSPERFRAME)*ud.multimode);
    OSD_Printf("demo duration: %d min %d sec\n", i/60, i%60);
    demo_startreccnt = ud.reccnt;

    ud.god = ud.cashman = ud.eog = ud.showallmap = 0;
    ud.clipping = ud.scrollmode = ud.overhead_on = ud.pause_on = 0;

    G_NewGame(ud.volume_number,ud.level_number,ud.player_skill);
    return(1);
corrupt:
    OSD_Printf(OSD_ERROR "Demo %d header is corrupt.\n",g_whichDemo);
    ud.reccnt = 0;
    kclose(recfilep);
    return 0;
}

void G_OpenDemoWrite(void)
{
    char d[13];
    int32_t dummylong = 0, demonum=1;
    char ver;
    int16_t i;

    if (ud.recstat == 2) kclose(recfilep);

    ver = BYTEVERSION;

    while (1)
    {
        if (demonum == 10000) return;
        Bsprintf(d, "demo%d.dmo", demonum++);
        frecfilep = fopen(d, "rb");
        if (frecfilep == NULL) break;
        Bfclose(frecfilep);
    }

    if ((frecfilep = fopen(d,"wb")) == NULL) return;
    fwrite(&dummylong,sizeof(dummylong),1,frecfilep);
    fwrite(&ver,sizeof(uint8_t),1,frecfilep);
    fwrite((char *)&ud.volume_number,sizeof(uint8_t),1,frecfilep);
    fwrite((char *)&ud.level_number,sizeof(uint8_t),1,frecfilep);
    fwrite((char *)&ud.player_skill,sizeof(uint8_t),1,frecfilep);
    fwrite((char *)&ud.m_coop,sizeof(uint8_t),1,frecfilep);
    fwrite((char *)&ud.m_ffire,sizeof(uint8_t),1,frecfilep);
    fwrite((int16_t *)&ud.multimode,sizeof(int16_t),1,frecfilep);
    fwrite((int16_t *)&ud.m_monsters_off,sizeof(int16_t),1,frecfilep);
    fwrite((int32_t *)&ud.m_respawn_monsters,sizeof(int32_t),1,frecfilep);
    fwrite((int32_t *)&ud.m_respawn_items,sizeof(int32_t),1,frecfilep);
    fwrite((int32_t *)&ud.m_respawn_inventory,sizeof(int32_t),1,frecfilep);
    fwrite((int32_t *)&ud.playerai,sizeof(int32_t),1,frecfilep);
    fwrite((int32_t *)&ud.auto_run,sizeof(int32_t),1,frecfilep);
    fwrite((char *)boardfilename,sizeof(boardfilename),1,frecfilep);
    fwrite((int32_t *)&ud.m_noexits,sizeof(int32_t),1,frecfilep);

    for (i=0; i<ud.multimode; i++)
    {
        fwrite((char *)&g_player[i].user_name,sizeof(g_player[i].user_name),1,frecfilep);
        fwrite((int32_t *)&g_player[i].ps->aim_mode,sizeof(int32_t),1,frecfilep);
        fwrite((int32_t *)&g_player[i].ps->auto_aim,sizeof(int32_t),1,frecfilep);       // JBF 20031126
        fwrite(&g_player[i].ps->weaponswitch,sizeof(int32_t),1,frecfilep);
        fwrite(&g_player[i].pcolor,sizeof(int32_t),1,frecfilep);
        fwrite(&g_player[i].pteam,sizeof(int32_t),1,frecfilep);
    }

    totalreccnt = 0;
    ud.reccnt = 0;
}

static void G_DemoRecord(void)
{
    int16_t i;

    TRAVERSE_CONNECT(i)
    {
        copybufbyte(g_player[i].sync,&recsync[ud.reccnt],sizeof(input_t));
        ud.reccnt++;
        totalreccnt++;
        if (ud.reccnt >= RECSYNCBUFSIZ)
        {
            dfwrite(recsync,sizeof(input_t)*ud.multimode,ud.reccnt/ud.multimode,frecfilep);
            ud.reccnt = 0;
        }
    }
}

void G_CloseDemoWrite(void)
{
    if (ud.recstat == 1)
    {
        if (ud.reccnt > 0)
        {
            dfwrite(recsync,sizeof(input_t)*ud.multimode,ud.reccnt/ud.multimode,frecfilep);

            fseek(frecfilep,SEEK_SET,0L);
            fwrite(&totalreccnt,sizeof(int32_t),1,frecfilep);
            ud.recstat = ud.m_recstat = 0;
        }
        fclose(frecfilep);
    }
}

static int32_t g_whichDemo = 1;
extern int32_t premap_quickenterlevel;

// extern int32_t syncs[];
static int32_t G_PlaybackDemo(void)
{
    int32_t i,j,k,l;
    int32_t foundemo = 0;
    static int32_t in_menu = 0;

    if (ready2send) return 0;

RECHECK:

    in_menu = g_player[myconnectindex].ps->gm&MODE_MENU;

    pub = NUMPAGES;
    pus = NUMPAGES;

    flushperms();
    if (ud.multimode < 2) foundemo = G_OpenDemoRead(g_whichDemo);
    if (foundemo == 0)
    {
        if (g_whichDemo > 1)
        {
            g_whichDemo = 1;
            premap_quickenterlevel=0;
            goto RECHECK;
        }
        fadepal(0,0,0, 0,63,7);
        P_SetGamePalette(g_player[myconnectindex].ps, palette, 1);    // JBF 20040308
        G_DrawBackground();
        M_DisplayMenus();
        //g_player[myconnectindex].ps->palette = palette;
        nextpage();
        fadepal(0,0,0, 63,0,-7);
        ud.reccnt = 0;
    }
    else
    {
        ud.recstat = 2;
        g_whichDemo++;
        if (g_whichDemo == 10) g_whichDemo = 1;

        if (G_EnterLevel(MODE_DEMO)) ud.recstat = foundemo = 0;
    }

    if (!premap_quickenterlevel)
    {
        if (foundemo == 0 || in_menu || KB_KeyWaiting() || numplayers > 1)
        {
            FX_StopAllSounds();
            S_ClearSoundLocks();
            g_player[myconnectindex].ps->gm |= MODE_MENU;
        }
    }

    ready2send = 0;
    i = 0;

    KB_FlushKeyboardQueue();

    k = 0;

    while (ud.reccnt > 0 || foundemo == 0)
    {
        if (foundemo && (!demo_paused || demo_goalreccnt!=0))
        {
            if (demo_goalreccnt < 0)
            {
                demo_goalreccnt = -demo_goalreccnt;

                if (g_whichDemo > 1)  // load the same demo again and FF from beginning... yay!
                    g_whichDemo--;
                foundemo = 0;
                ud.reccnt = 0;
                kclose(recfilep);
                premap_quickenterlevel=1;
                goto RECHECK;
            }

            while (totalclock >= (lockclock+TICSPERFRAME)
                   || (ud.reccnt > (TICRATE/TICSPERFRAME)*2 && ud.pause_on)
                   || (demo_goalreccnt>0 && demo_goalreccnt<ud.reccnt))
            {
                if ((i == 0) || (i >= RECSYNCBUFSIZ))
                {
                    i = 0;
                    l = min(ud.reccnt,RECSYNCBUFSIZ);
                    if (kdfread(recsync,sizeof(input_t)*ud.multimode,l/ud.multimode,recfilep) != l/ud.multimode)
                    {
                        OSD_Printf(OSD_ERROR "Demo %d is corrupt.\n", g_whichDemo-1);
                        foundemo = 0;
                        ud.reccnt = 0;
                        kclose(recfilep);
                        g_player[myconnectindex].ps->gm |= MODE_MENU;
                        goto RECHECK;
                    }
                }

                TRAVERSE_CONNECT(j)
                {
                    copybufbyte(&recsync[i],&inputfifo[g_player[j].movefifoend&(MOVEFIFOSIZ-1)][j],sizeof(input_t));
                    g_player[j].movefifoend++;
                    i++;
                    ud.reccnt--;
                }
                G_DoMoveThings();
                ototalclock += TICSPERFRAME;

                if (demo_goalreccnt > 0)
                {
                    if (demo_goalreccnt<ud.reccnt || (ud.reccnt > (TICRATE/TICSPERFRAME)*2 && ud.pause_on))
                        totalclock += TICSPERFRAME;
                }
            }

            if (demo_goalreccnt > 0 && ud.reccnt<=demo_goalreccnt)
            {
                demo_goalreccnt = 0;
                ud.config.SoundToggle = demo_oldsoundtoggle;
                premap_quickenterlevel = 0;
            }
        }
        else if (foundemo && demo_paused && demo_goalreccnt==0)
        {
            lockclock = ototalclock = totalclock;
        }

        if (foundemo == 0)
            G_DrawBackground();
        else
        {
            G_HandleLocalKeys();

//            j = min(max((totalclock-lockclock)*(65536/TICSPERFRAME),0),65536);
            j = min(max((totalclock - ototalclock) * (65536 / 4),0),65536);
            G_DrawRooms(screenpeek,j);
            G_DisplayRest(j);

            if (demo_showstats && (g_player[myconnectindex].ps->gm&MODE_MENU) == 0)
            {
                j=(demo_startreccnt-ud.reccnt)/(TICRATE/TICSPERFRAME);
                Bsprintf(buf, "%02d:%02d", j/60, j%60);
                gametext(18,16,buf,0,2+8+16);

                rotatesprite(60<<16,16<<16,32768,0,SLIDEBAR,0,0,2+8+16,0,0,(xdim*95)/320,ydim-1);
                rotatesprite(90<<16,16<<16,32768,0,SLIDEBAR,0,0,2+8+16,(xdim*95)/320,0,(xdim*125)/320,ydim-1);
                rotatesprite(120<<16,16<<16,32768,0,SLIDEBAR,0,0,2+8+16,(xdim*125)/320,0,(xdim*155)/320,ydim-1);
                rotatesprite(150<<16,16<<16,32768,0,SLIDEBAR,0,0,2+8+16,(xdim*155)/320,0,xdim-1,ydim-1);

                j = (182<<16) - ((((120*ud.reccnt)<<4)/demo_startreccnt)<<12);
                rotatesprite(j,(16<<16)+(1<<15),32768,0,SLIDEBAR+1,0,0,2+8+16,0,0,xdim-1,ydim-1);

                j=ud.reccnt/(TICRATE/TICSPERFRAME);
                Bsprintf(buf, "-%02d:%02d%s", j/60, j%60, demo_paused?"   ^15PAUSED":"");
                gametext(194,16,buf,0,2+8+16);
            }

            if (ud.multimode > 1 && g_player[myconnectindex].ps->gm)
                Net_GetPackets();

            if (g_player[myconnectindex].gotvote == 0 && voting != -1 && voting != myconnectindex)
                gametext(160,60,"PRESS F1 TO ACCEPT, F2 TO DECLINE",0,2+8+16);
        }

        if ((g_player[myconnectindex].ps->gm&MODE_MENU) && (g_player[myconnectindex].ps->gm&MODE_EOL))
            goto RECHECK;

        if (KB_KeyPressed(sc_Escape) && (g_player[myconnectindex].ps->gm&MODE_MENU) == 0 && (g_player[myconnectindex].ps->gm&MODE_TYPE) == 0)
        {
            KB_ClearKeyDown(sc_Escape);
            FX_StopAllSounds();
            S_ClearSoundLocks();
            g_player[myconnectindex].ps->gm |= MODE_MENU;
            ChangeToMenu(0);
            S_MenuSound();
        }

        if (g_player[myconnectindex].ps->gm&MODE_TYPE)
        {
            Net_EnterMessage();
            if ((g_player[myconnectindex].ps->gm&MODE_TYPE) != MODE_TYPE)
                g_player[myconnectindex].ps->gm = MODE_MENU;
        }
        else
        {
            if (ud.recstat != 2)
                M_DisplayMenus();
            if (ud.multimode > 1  && g_currentMenu != 20003 && g_currentMenu != 20005 && g_currentMenu != 210)
            {
                ControlInfo noshareinfo;
                CONTROL_GetInput(&noshareinfo);
                if (BUTTON(gamefunc_SendMessage))
                {
                    KB_FlushKeyboardQueue();
                    CONTROL_ClearButton(gamefunc_SendMessage);
                    g_player[myconnectindex].ps->gm = MODE_TYPE;
                    typebuf[0] = 0;
                    inputloc = 0;
                }
            }
        }

        G_PrintGameQuotes();

        if (ud.last_camsprite != ud.camerasprite)
        {
            ud.last_camsprite = ud.camerasprite;
            ud.camera_time = totalclock+(TICRATE*2);
        }

        if (VOLUMEONE)
        {
            if (ud.show_help == 0 && (g_player[myconnectindex].ps->gm&MODE_MENU) == 0)
                rotatesprite((320-50)<<16,9<<16,65536L,0,BETAVERSION,0,0,2+8+16+128,0,0,xdim-1,ydim-1);
        }
        handleevents();
        Net_GetPackets();
        nextpage();

        if (g_player[myconnectindex].ps->gm==MODE_END || g_player[myconnectindex].ps->gm==MODE_GAME)
        {
            if (foundemo)
                kclose(recfilep);
            return 0;
        }
    }
    ud.multimode = numplayers;  // fixes 2 infinite loops after watching demo
    kclose(recfilep);

#if 0
    {
        uint32_t crcv;
        // sync checker
        +       initcrc32table();
        crc32init(&crcv);
        crc32block(&crcv, (char *)wall, sizeof(wall));
        crc32block(&crcv, (char *)sector, sizeof(sector));
        crc32block(&crcv, (char *)sprite, sizeof(sprite));
        crc32finish(&crcv);
        initprintf("Checksum = %08X\n",crcv);
    }
#endif

    if (g_player[myconnectindex].ps->gm&MODE_MENU) goto RECHECK;
    return 1;
}

static inline int32_t G_MoveLoop()
{
    int32_t i;

    if (numplayers > 1)
        while (predictfifoplc < g_player[myconnectindex].movefifoend) Net_DoPrediction();

    Net_GetPackets();

    if (numplayers < 2) bufferjitter = 0;
    while (g_player[myconnectindex].movefifoend-movefifoplc > bufferjitter)
    {
        TRAVERSE_CONNECT(i)
        if (movefifoplc == g_player[i].movefifoend) break;
        if (i >= 0) break;
        if (G_DoMoveThings()) return 1;
    }
    return 0;
}

static void Net_CorrectPrediction(void)
{
    int32_t i;
    DukePlayer_t *p;

    if (numplayers < 2) return;

    i = ((movefifoplc-1)&(MOVEFIFOSIZ-1));
    p = g_player[myconnectindex].ps;

    if (p->posx == myxbak[i] && p->posy == myybak[i] && p->posz == myzbak[i]
    && p->horiz == myhorizbak[i] && p->ang == myangbak[i])
        return;

    Bmemcpy(&my, p, sizeof(vec3_t));
    Bmemcpy(&omy, &p->oposx, sizeof(vec3_t));
    Bmemcpy(&myvel, &p->posxv, sizeof(vec3_t));

    myang = p->ang;
    omyang = p->oang;
    mycursectnum = p->cursectnum;
    myhoriz = p->horiz;
    omyhoriz = p->ohoriz;
    myhorizoff = p->horizoff;
    omyhorizoff = p->ohorizoff;
    myjumpingcounter = p->jumping_counter;
    myjumpingtoggle = p->jumping_toggle;
    myonground = p->on_ground;
    myhardlanding = p->hard_landing;
    myreturntocenter = p->return_to_center;

    predictfifoplc = movefifoplc;
    while (predictfifoplc < g_player[myconnectindex].movefifoend)
        Net_DoPrediction();
}

static void Net_DoPrediction(void)
{
    input_t *syn;
    DukePlayer_t *p;
    int32_t i, j, k, doubvel, fz, cz, hz, lz, x, y;
    uint32_t sb_snum;
    int16_t psect, psectlotag, tempsect, backcstat;
    char shrunk, spritebridge;

    syn = (input_t *)&inputfifo[predictfifoplc&(MOVEFIFOSIZ-1)][myconnectindex];

    p = g_player[myconnectindex].ps;

    backcstat = sprite[p->i].cstat;
    sprite[p->i].cstat &= ~257;

    sb_snum = syn->bits;

    psect = mycursectnum;
    psectlotag = sector[psect].lotag;
    spritebridge = 0;

    shrunk = (sprite[p->i].yrepeat < 32);

    if (ud.clipping == 0 && (sector[psect].floorpicnum == MIRROR || psect < 0 || psect >= MAXSECTORS))
    {
        my.x = omy.x;
        my.y = omy.y;
    }
    else
    {
        omy.x = my.x;
        omy.y = my.y;
    }

    omyhoriz = myhoriz;
    omyhorizoff = myhorizoff;
    omy.z = my.z;
    omyang = myang;

    getzrange(&my,psect,&cz,&hz,&fz,&lz,163L,CLIPMASK0);

    j = getflorzofslope(psect,my.x,my.y);

    if ((lz&49152) == 16384 && psectlotag == 1 && klabs(my.z-j) > PHEIGHT+(16<<8))
        psectlotag = 0;

    if (p->aim_mode == 0 && myonground && psectlotag != 2 && (sector[psect].floorstat&2))
    {
        x = my.x+(sintable[(myang+512)&2047]>>5);
        y = my.y+(sintable[myang&2047]>>5);
        tempsect = psect;
        updatesector(x,y,&tempsect);
        if (tempsect >= 0)
        {
            k = getflorzofslope(psect,x,y);
            if (psect == tempsect)
                myhorizoff += mulscale16(j-k,160);
            else if (klabs(getflorzofslope(tempsect,x,y)-k) <= (4<<8))
                myhorizoff += mulscale16(j-k,160);
        }
    }
    if (myhorizoff > 0) myhorizoff -= ((myhorizoff>>3)+1);
    else if (myhorizoff < 0) myhorizoff += (((-myhorizoff)>>3)+1);

    if (hz >= 0 && (hz&49152) == 49152)
    {
        hz &= (MAXSPRITES-1);
        if (sprite[hz].statnum == STAT_ACTOR && sprite[hz].extra >= 0)
        {
            hz = 0;
            cz = getceilzofslope(psect,my.x,my.y);
        }
    }

    if (lz >= 0 && (lz&49152) == 49152)
    {
        j = lz&(MAXSPRITES-1);
        if ((sprite[j].cstat&33) == 33 || (sprite[j].cstat&17) == 17)
        {
            psectlotag = 0;
            spritebridge = 1;
        }
        if (A_CheckEnemySprite(&sprite[j]) && sprite[j].xrepeat > 24 && klabs(sprite[p->i].z-sprite[j].z) < (84<<8))
        {
            j = getangle(sprite[j].x-my.x,sprite[j].y-my.y);
            myvel.x -= sintable[(j+512)&2047]<<4;
            myvel.y -= sintable[j&2047]<<4;
        }
    }

    if (sprite[p->i].extra <= 0)
    {
        if (psectlotag == 2)
        {
            if (p->on_warping_sector == 0)
            {
                if (klabs(my.z-fz) > (PHEIGHT>>1))
                    my.z += 348;
            }
            clipmove(&my,&mycursectnum,0,0,164L,(4L<<8),(4L<<8),CLIPMASK0);
        }

        updatesector(my.x,my.y,&mycursectnum);
        pushmove(&my,&mycursectnum,128L,(4L<<8),(20L<<8),CLIPMASK0);

        myhoriz = 100;
        myhorizoff = 0;

        goto ENDFAKEPROCESSINPUT;
    }

    doubvel = TICSPERFRAME;

    if (p->on_crane >= 0) goto FAKEHORIZONLY;

    if (p->one_eighty_count < 0) myang += 128;

    i = 40;

    if (psectlotag == 2)
    {
        myjumpingcounter = 0;

        if (TEST_SYNC_KEY(sb_snum, SK_JUMP))
        {
            if (myvel.z > 0) myvel.z = 0;
            myvel.z -= 348;
            if (myvel.z < -(256*6)) myvel.z = -(256*6);
        }
        else if (TEST_SYNC_KEY(sb_snum, SK_CROUCH))
        {
            if (myvel.z < 0) myvel.z = 0;
            myvel.z += 348;
            if (myvel.z > (256*6)) myvel.z = (256*6);
        }
        else
        {
            if (myvel.z < 0)
            {
                myvel.z += 256;
                if (myvel.z > 0)
                    myvel.z = 0;
            }
            if (myvel.z > 0)
            {
                myvel.z -= 256;
                if (myvel.z < 0)
                    myvel.z = 0;
            }
        }

        if (myvel.z > 2048) myvel.z >>= 1;

        my.z += myvel.z;

        if (my.z > (fz-(15<<8)))
            my.z += ((fz-(15<<8))-my.z)>>1;

        /*
                if (my.z < (cz+(4<<8)))
                {
                    my.z = cz+(4<<8);
                    myvel.z = 0;
                }
        */
    }

    else if (p->jetpack_on)
    {
        myonground = 0;
        myjumpingcounter = 0;
        myhardlanding = 0;

        if (p->jetpack_on < 11)
            my.z -= (p->jetpack_on<<7); //Goin up

        if (shrunk) j = 512;
        else j = 2048;

        if (TEST_SYNC_KEY(sb_snum, SK_JUMP))                            //A
            my.z -= j;
        if (TEST_SYNC_KEY(sb_snum, SK_CROUCH))                       //Z
            my.z += j;

        if (shrunk == 0 && (psectlotag == 0 || psectlotag == 2)) k = 32;
        else k = 16;

        if (my.z > (fz-(k<<8)))
            my.z += ((fz-(k<<8))-my.z)>>1;
        if (my.z < (cz+(18<<8)))
            my.z = cz+(18<<8);
    }
    else if (psectlotag != 2)
    {
        if (psectlotag == 1 && p->spritebridge == 0)
        {
            if (shrunk == 0) i = 34;
            else i = 12;
        }
        if (my.z < (fz-(i<<8)) && (G_CheckForSpaceFloor(psect)|G_CheckForSpaceCeiling(psect)) == 0) //falling
        {
            if (!TEST_SYNC_KEY(sb_snum, SK_JUMP) && !TEST_SYNC_KEY(sb_snum, SK_CROUCH) &&
                    myonground && (sector[psect].floorstat&2) && my.z >= (fz-(i<<8)-(16<<8)))
                my.z = fz-(i<<8);
            else
            {
                myonground = 0;

                myvel.z += (g_spriteGravity+80);

                if (myvel.z >= (4096+2048)) myvel.z = (4096+2048);
            }
        }

        else
        {
            if (psectlotag != 1 && psectlotag != 2 && myonground == 0 && myvel.z > (6144>>1))
                myhardlanding = myvel.z>>10;
            myonground = 1;

            if (i==40)
            {
                //Smooth on the ground

                k = ((fz-(i<<8))-my.z)>>1;
                if (klabs(k) < 256) k = 0;
                my.z += k; // ((fz-(i<<8))-my.z)>>1;
                myvel.z -= 768; // 412;
                if (myvel.z < 0) myvel.z = 0;
            }
            else if (myjumpingcounter == 0)
            {
                my.z += ((fz-(i<<7))-my.z)>>1; //Smooth on the water
                if (p->on_warping_sector == 0 && my.z > fz-(16<<8))
                {
                    my.z = fz-(16<<8);
                    myvel.z >>= 1;
                }
            }

            if (TEST_SYNC_KEY(sb_snum, SK_CROUCH))
                my.z += (2048+768);

            if (TEST_SYNC_KEY(sb_snum, SK_JUMP) == 0 && myjumpingtoggle == 1)
                myjumpingtoggle = 0;

            else if (TEST_SYNC_KEY(sb_snum, SK_JUMP) && myjumpingtoggle == 0)
            {
                if (myjumpingcounter == 0)
                    if ((fz-cz) > (56<<8))
                    {
                        myjumpingcounter = 1;
                        myjumpingtoggle = 1;
                    }
            }
            if (myjumpingcounter && TEST_SYNC_KEY(sb_snum, SK_JUMP) == 0)
                myjumpingcounter = 0;
        }

        if (myjumpingcounter)
        {
            if (TEST_SYNC_KEY(sb_snum, SK_JUMP) == 0 && myjumpingtoggle == 1)
                myjumpingtoggle = 0;

            if (myjumpingcounter < (1024+256))
            {
                if (psectlotag == 1 && myjumpingcounter > 768)
                {
                    myjumpingcounter = 0;
                    myvel.z = -512;
                }
                else
                {
                    myvel.z -= (sintable[(2048-128+myjumpingcounter)&2047])/12;
                    myjumpingcounter += 180;

                    myonground = 0;
                }
            }
            else
            {
                myjumpingcounter = 0;
                myvel.z = 0;
            }
        }

        my.z += myvel.z;

        if (my.z < (cz+(4<<8)))
        {
            myjumpingcounter = 0;
            if (myvel.z < 0) myvel.x = myvel.y = 0;
            myvel.z = 128;
            my.z = cz+(4<<8);
        }
    }

    if (p->fist_incs ||
            p->transporter_hold > 2 ||
            myhardlanding ||
            p->access_incs > 0 ||
            p->knee_incs > 0 ||
            (p->curr_weapon == TRIPBOMB_WEAPON &&
             p->kickback_pic > 1 &&
             p->kickback_pic < 4))
    {
        doubvel = 0;
        myvel.x = 0;
        myvel.y = 0;
    }
    else if (syn->avel)          //p->ang += syncangvel * constant
    {
        //ENGINE calculates angvel for you
        int32_t tempang = syn->avel<<1;

        if (psectlotag == 2)
            myang += (tempang-(tempang>>3))*ksgn(doubvel);
        else myang += (tempang)*ksgn(doubvel);
        myang &= 2047;
    }

    if (myvel.x || myvel.y || syn->fvel || syn->svel)
    {
        if (p->jetpack_on == 0 && p->steroids_amount > 0 && p->steroids_amount < 400)
            doubvel <<= 1;

        myvel.x += ((syn->fvel*doubvel)<<6);
        myvel.y += ((syn->svel*doubvel)<<6);

        if ((p->curr_weapon == KNEE_WEAPON && p->kickback_pic > 10 && myonground) || (myonground && TEST_SYNC_KEY(sb_snum, SK_CROUCH)))
        {
            myvel.x = mulscale16(myvel.x,p->runspeed-0x2000);
            myvel.y = mulscale16(myvel.y,p->runspeed-0x2000);
        }
        else
        {
            if (psectlotag == 2)
            {
                myvel.x = mulscale16(myvel.x,p->runspeed-0x1400);
                myvel.y = mulscale16(myvel.y,p->runspeed-0x1400);
            }
            else
            {
                myvel.x = mulscale16(myvel.x,p->runspeed);
                myvel.y = mulscale16(myvel.y,p->runspeed);
            }
        }

        if (klabs(myvel.x) < 2048 && klabs(myvel.y) < 2048)
            myvel.x = myvel.y = 0;

        if (shrunk)
        {
            myvel.x =
                mulscale16(myvel.x,(p->runspeed)-(p->runspeed>>1)+(p->runspeed>>2));
            myvel.y =
                mulscale16(myvel.y,(p->runspeed)-(p->runspeed>>1)+(p->runspeed>>2));
        }
    }

FAKEHORIZONLY:
    if (psectlotag == 1 || spritebridge == 1) i = (4L<<8);
    else i = (20L<<8);

    clipmove(&my,&mycursectnum,myvel.x,myvel.y,164L,4L<<8,i,CLIPMASK0);
    pushmove(&my,&mycursectnum,164L,4L<<8,4L<<8,CLIPMASK0);

    if (p->jetpack_on == 0 && psectlotag != 1 && psectlotag != 2 && shrunk)
        my.z += 30<<8;

    if (TEST_SYNC_KEY(sb_snum, SK_CENTER_VIEW) || myhardlanding)
        myreturntocenter = 9;

    if (TEST_SYNC_KEY(sb_snum, SK_LOOK_UP))
    {
        myreturntocenter = 9;
        if (TEST_SYNC_KEY(sb_snum, SK_RUN)) myhoriz += 6;
        myhoriz += 6;
    }
    else if (TEST_SYNC_KEY(sb_snum, SK_LOOK_DOWN))
    {
        myreturntocenter = 9;
        if (TEST_SYNC_KEY(sb_snum, SK_RUN)) myhoriz -= 6;
        myhoriz -= 6;
    }
    else if (TEST_SYNC_KEY(sb_snum, SK_AIM_UP))
    {
        if (TEST_SYNC_KEY(sb_snum, SK_RUN)) myhoriz += 6;
        myhoriz += 6;
    }
    else if (TEST_SYNC_KEY(sb_snum, SK_AIM_DOWN))
    {
        if (TEST_SYNC_KEY(sb_snum, SK_RUN)) myhoriz -= 6;
        myhoriz -= 6;
    }

    if (myreturntocenter > 0)
        if (TEST_SYNC_KEY(sb_snum, SK_LOOK_UP) == 0 && TEST_SYNC_KEY(sb_snum, SK_LOOK_DOWN) == 0)
        {
            myreturntocenter--;
            myhoriz += 33-(myhoriz/3);
        }

    if (p->aim_mode)
        myhoriz += syn->horz;
    else
    {
        if (myhoriz > 95 && myhoriz < 105) myhoriz = 100;
        if (myhorizoff > -5 && myhorizoff < 5) myhorizoff = 0;
    }

    if (myhardlanding > 0)
    {
        myhardlanding--;
        myhoriz -= (myhardlanding<<4);
    }

    if (myhoriz > HORIZ_MAX) myhoriz = HORIZ_MAX;
    else if (myhoriz < HORIZ_MIN) myhoriz = HORIZ_MIN;

    if (p->knee_incs > 0)
    {
        myhoriz -= 48;
        myreturntocenter = 9;
    }

ENDFAKEPROCESSINPUT:

    if (apScriptGameEvent[EVENT_FAKEDOMOVETHINGS])
        X_OnEvent(EVENT_FAKEDOMOVETHINGS, g_player[myconnectindex].ps->i, myconnectindex, -1);

    myxbak[predictfifoplc&(MOVEFIFOSIZ-1)] = my.x;
    myybak[predictfifoplc&(MOVEFIFOSIZ-1)] = my.y;
    myzbak[predictfifoplc&(MOVEFIFOSIZ-1)] = my.z;
    myangbak[predictfifoplc&(MOVEFIFOSIZ-1)] = myang;
    myhorizbak[predictfifoplc&(MOVEFIFOSIZ-1)] = myhoriz;
    predictfifoplc++;

    sprite[p->i].cstat = backcstat;
}

static int32_t G_DoMoveThings(void)
{
    int32_t i, j;
//    char ch;

    TRAVERSE_CONNECT(i)
    if (TEST_SYNC_KEY(g_player[i].sync->bits, SK_MULTIFLAG))
    {
        multiflag = 2;
        multiwhat = (g_player[i].sync->bits>>(SK_MULTIFLAG+1))&1;
        multipos = (unsigned)(g_player[i].sync->bits>>(SK_MULTIFLAG+2))&15;
        multiwho = i;

        if (multiwhat)
        {
            G_SavePlayer(multipos);
            multiflag = 0;

            if (multiwho != myconnectindex)
            {
                Bsprintf(ScriptQuotes[122],"%s^00 SAVED A MULTIPLAYER GAME",&g_player[multiwho].user_name[0]);
                P_DoQuote(122,g_player[myconnectindex].ps);
            }
            else
            {
                Bstrcpy(ScriptQuotes[122],"MULTIPLAYER GAME SAVED");
                P_DoQuote(122,g_player[myconnectindex].ps);
            }
            break;
        }
        else
        {
            //            waitforeverybody();

            j = G_LoadPlayer(multipos);

            multiflag = 0;

            if (j == 0)
            {
                if (multiwho != myconnectindex)
                {
                    Bsprintf(ScriptQuotes[122],"%s^00 LOADED A MULTIPLAYER GAME",&g_player[multiwho].user_name[0]);
                    P_DoQuote(122,g_player[myconnectindex].ps);
                }
                else
                {
                    Bstrcpy(ScriptQuotes[122],"MULTIPLAYER GAME LOADED");
                    P_DoQuote(122,g_player[myconnectindex].ps);
                }
                return 1;
            }
        }
    }

    ud.camerasprite = -1;
    lockclock += TICSPERFRAME;

    if (g_earthquakeTime > 0) g_earthquakeTime--;
    if (g_RTSPlaying > 0) g_RTSPlaying--;

    for (i=0; i<MAXUSERQUOTES; i++)
        if (user_quote_time[i])
        {
            user_quote_time[i]--;
            if (user_quote_time[i] > ud.msgdisptime)
                user_quote_time[i] = ud.msgdisptime;
            if (!user_quote_time[i]) pub = NUMPAGES;
        }

    if (ud.idplayers && ud.multimode > 1)
    {
        hitdata_t hitinfo;

        for (i=0; i<ud.multimode; i++)
            if (g_player[i].ps->holoduke_on != -1)
                sprite[g_player[i].ps->holoduke_on].cstat ^= 256;

        hitscan((vec3_t *)g_player[screenpeek].ps,g_player[screenpeek].ps->cursectnum,
                sintable[(g_player[screenpeek].ps->ang+512)&2047],
                sintable[g_player[screenpeek].ps->ang&2047],
                (100-g_player[screenpeek].ps->horiz-g_player[screenpeek].ps->horizoff)<<11,&hitinfo,0xffff0030);

        for (i=0; i<ud.multimode; i++)
            if (g_player[i].ps->holoduke_on != -1)
                sprite[g_player[i].ps->holoduke_on].cstat ^= 256;

        if ((hitinfo.hitsprite >= 0) && !(g_player[myconnectindex].ps->gm & MODE_MENU) &&
                sprite[hitinfo.hitsprite].picnum == APLAYER && sprite[hitinfo.hitsprite].yvel != screenpeek &&
                g_player[sprite[hitinfo.hitsprite].yvel].ps->dead_flag == 0)
        {
            if (g_player[screenpeek].ps->fta == 0 || g_player[screenpeek].ps->ftq == 117)
            {
                if (ldist(&sprite[g_player[screenpeek].ps->i],&sprite[hitinfo.hitsprite]) < 9216)
                {
                    Bsprintf(ScriptQuotes[117],"%s",&g_player[sprite[hitinfo.hitsprite].yvel].user_name[0]);
                    g_player[screenpeek].ps->fta = 12, g_player[screenpeek].ps->ftq = 117;
                }
            }
            else if (g_player[screenpeek].ps->fta > 2) g_player[screenpeek].ps->fta -= 3;
        }
    }

    if (g_showShareware > 0)
    {
        g_showShareware--;
        if (g_showShareware == 0)
        {
            pus = NUMPAGES;
            pub = NUMPAGES;
        }
    }

    everyothertime++;

    TRAVERSE_CONNECT(i)
    copybufbyte(&inputfifo[movefifoplc&(MOVEFIFOSIZ-1)][i],g_player[i].sync,sizeof(input_t));
    movefifoplc++;

    G_UpdateInterpolations();

    j = -1;
    TRAVERSE_CONNECT(i)
    {
        if (TEST_SYNC_KEY(g_player[i].sync->bits,SK_GAMEQUIT) == 0)
        {
            j = i;
            continue;
        }

        G_CloseDemoWrite();

        if (i == myconnectindex) G_GameExit(" ");
        if (screenpeek == i)
        {
            screenpeek = connectpoint2[i];
            if (screenpeek < 0) screenpeek = connecthead;
        }

        if (i == connecthead) connecthead = connectpoint2[connecthead];
        else connectpoint2[j] = connectpoint2[i];

        numplayers--;
        ud.multimode--;

        if (numplayers < 2)
            S_PlaySound(GENERIC_AMBIENCE17);

        pub = NUMPAGES;
        pus = NUMPAGES;
        G_UpdateScreenArea();

        P_QuickKill(g_player[i].ps);
        deletesprite(g_player[i].ps->i);

        Bsprintf(buf,"%s^00 is history!",g_player[i].user_name);
        G_AddUserQuote(buf);
        Bstrcpy(ScriptQuotes[116],buf);

        if (voting == i)
        {
            for (i=0; i<MAXPLAYERS; i++)
            {
                g_player[i].vote = 0;
                g_player[i].gotvote = 0;
            }
            voting = -1;
        }

        g_player[myconnectindex].ps->ftq = 116, g_player[myconnectindex].ps->fta = 180;

        if (j < 0 && g_networkBroadcastMode == 0)
            G_GameExit("Server terminated");
    }
    /*
        if ((numplayers >= 2) && ((movefifoplc&7) == 7))
        {
            ch = (uint8_t)(randomseed&255);
            for (i=connecthead;i>=0;i=connectpoint2[i])
                ch += ((g_player[i].ps->posx+g_player[i].ps->posy+g_player[i].ps->posz+g_player[i].ps->ang+g_player[i].ps->horiz)&255);
            g_player[myconnectindex].syncval[g_player[myconnectindex].syncvalhead&(MOVEFIFOSIZ-1)] = ch;
            g_player[myconnectindex].syncvalhead++;
        }
    */

    Net_GetSyncStat();

    g_moveThingsCount++;

    if (ud.recstat == 1) G_DemoRecord();

#ifdef POLYMER
    if (ud.pause_on == 0)
        gamelightcount = 0;
#endif

    if (ud.pause_on == 0)
    {
        g_globalRandom = krand();
        A_MoveDummyPlayers();//ST 13
    }

    TRAVERSE_CONNECT(i)
    {
        if (g_player[i].sync->extbits&(1<<6))
        {
            g_player[i].ps->team = g_player[i].pteam;
            if (GametypeFlags[ud.coop] & GAMETYPE_TDM)
            {
                ActorExtra[g_player[i].ps->i].picnum = APLAYERTOP;
                P_QuickKill(g_player[i].ps);
            }
        }
        if (GametypeFlags[ud.coop] & GAMETYPE_TDM)
            g_player[i].ps->palookup = g_player[i].pcolor = G_GetTeamPalette(g_player[i].ps->team);

        if (sprite[g_player[i].ps->i].pal != 1)
            sprite[g_player[i].ps->i].pal = g_player[i].pcolor;

        G_HandleSharedKeys(i);

        if (ud.pause_on == 0)
        {
            P_ProcessInput(i);
            P_CheckSectors(i);
        }
    }

    if (ud.pause_on == 0)
        G_MoveWorld();

    Net_CorrectPrediction();

    if ((everyothertime&1) == 0)
    {
        G_AnimateWalls();
        A_MoveCyclers();
    }

    return 0;
}

static void G_DoOrderScreen(void)
{
    setview(0,0,xdim-1,ydim-1);

    fadepal(0,0,0, 0,63,7);
    //g_player[myconnectindex].ps->palette = palette;
    P_SetGamePalette(g_player[myconnectindex].ps, palette, 1);    // JBF 20040308
    KB_FlushKeyboardQueue();
    rotatesprite(0,0,65536L,0,ORDERING,0,0,2+8+16+64, 0,0,xdim-1,ydim-1);
    fadepal(0,0,0, 63,0,-7);
    while (!KB_KeyWaiting())
    {
        handleevents();
        Net_GetPackets();
    }

    fadepal(0,0,0, 0,63,7);
    KB_FlushKeyboardQueue();
    rotatesprite(0,0,65536L,0,ORDERING+1,0,0,2+8+16+64, 0,0,xdim-1,ydim-1);
    fadepal(0,0,0, 63,0,-7);
    while (!KB_KeyWaiting())
    {
        handleevents();
        Net_GetPackets();
    }

    fadepal(0,0,0, 0,63,7);
    KB_FlushKeyboardQueue();
    rotatesprite(0,0,65536L,0,ORDERING+2,0,0,2+8+16+64, 0,0,xdim-1,ydim-1);
    fadepal(0,0,0, 63,0,-7);
    while (!KB_KeyWaiting())
    {
        handleevents();
        Net_GetPackets();
    }

    fadepal(0,0,0, 0,63,7);
    KB_FlushKeyboardQueue();
    rotatesprite(0,0,65536L,0,ORDERING+3,0,0,2+8+16+64, 0,0,xdim-1,ydim-1);
    fadepal(0,0,0, 63,0,-7);
    while (!KB_KeyWaiting())
    {
        handleevents();
        Net_GetPackets();
    }
}

void G_BonusScreen(int32_t bonusonly)
{
    int32_t t, tinc,gfx_offset;
    int32_t i, y,xfragtotal,yfragtotal;
    int32_t bonuscnt;
    int32_t clockpad = 2;
    char *lastmapname;
    int32_t playerbest = -1;

    int32_t breathe[] =
    {
        0,  30,VICTORY1+1,176,59,
        30,  60,VICTORY1+2,176,59,
        60,  90,VICTORY1+1,176,59,
        90, 120,0         ,176,59
    };

    int32_t bossmove[] =
    {
        0, 120,VICTORY1+3,86,59,
        220, 260,VICTORY1+4,86,59,
        260, 290,VICTORY1+5,86,59,
        290, 320,VICTORY1+6,86,59,
        320, 350,VICTORY1+7,86,59,
        350, 380,VICTORY1+8,86,59
    };

    Bsprintf(tempbuf,"%s - " APPNAME,duke3dgrpstring);
    wm_setapptitle(tempbuf);

    if (ud.volume_number == 0 && ud.last_level == 8 && boardfilename[0])
    {
        lastmapname = Bstrrchr(boardfilename,'\\');
        if (!lastmapname) lastmapname = Bstrrchr(boardfilename,'/');
        if (!lastmapname) lastmapname = boardfilename;
    }
    else lastmapname = MapInfo[(ud.volume_number*MAXLEVELS)+ud.last_level-1].name;

    bonuscnt = 0;

    fadepal(0,0,0, 0,64,7);
    setview(0,0,xdim-1,ydim-1);
    clearview(0L);
    nextpage();
    flushperms();

    FX_StopAllSounds();
    S_ClearSoundLocks();
    FX_SetReverb(0L);
    bindsenabled = 1; // so you can use your screenshot bind on the score screens

    if (bonusonly) goto FRAGBONUS;

    if (numplayers < 2 && ud.eog && ud.from_bonus == 0)
        switch (ud.volume_number)
        {
        case 0:
            if (ud.lockout == 0)
            {
                P_SetGamePalette(g_player[myconnectindex].ps, endingpal, 11); // JBF 20040308
                clearview(0L);
                rotatesprite(0,50<<16,65536L,0,VICTORY1,0,0,2+8+16+64+128,0,0,xdim-1,ydim-1);
                nextpage();
                //g_player[myconnectindex].ps->palette = endingpal;
                fadepal(0,0,0, 63,0,-1);

                KB_FlushKeyboardQueue();
                totalclock = 0;
                tinc = 0;
                while (1)
                {
                    clearview(0L);
                    rotatesprite(0,50<<16,65536L,0,VICTORY1,0,0,2+8+16+64+128,0,0,xdim-1,ydim-1);

                    // boss
                    if (totalclock > 390 && totalclock < 780)
                        for (t=0; t<35; t+=5) if (bossmove[t+2] && (totalclock%390) > bossmove[t] && (totalclock%390) <= bossmove[t+1])
                            {
                                if (t==10 && bonuscnt == 1)
                                {
                                    S_PlaySound(SHOTGUN_FIRE);
                                    S_PlaySound(SQUISHED);
                                    bonuscnt++;
                                }
                                rotatesprite(bossmove[t+3]<<16,bossmove[t+4]<<16,65536L,0,bossmove[t+2],0,0,2+8+16+64+128,0,0,xdim-1,ydim-1);
                            }

                    // Breathe
                    if (totalclock < 450 || totalclock >= 750)
                    {
                        if (totalclock >= 750)
                        {
                            rotatesprite(86<<16,59<<16,65536L,0,VICTORY1+8,0,0,2+8+16+64+128,0,0,xdim-1,ydim-1);
                            if (totalclock >= 750 && bonuscnt == 2)
                            {
                                S_PlaySound(DUKETALKTOBOSS);
                                bonuscnt++;
                            }

                        }
                        for (t=0; t<20; t+=5)
                            if (breathe[t+2] && (totalclock%120) > breathe[t] && (totalclock%120) <= breathe[t+1])
                            {
                                if (t==5 && bonuscnt == 0)
                                {
                                    S_PlaySound(BOSSTALKTODUKE);
                                    bonuscnt++;
                                }
                                rotatesprite(breathe[t+3]<<16,breathe[t+4]<<16,65536L,0,breathe[t+2],0,0,2+8+16+64+128,0,0,xdim-1,ydim-1);
                            }
                    }
                    handleevents();
                    Net_GetPackets();
                    nextpage();
                    if (KB_KeyWaiting()) break;
                }
            }

            fadepal(0,0,0, 0,64,1);

            KB_FlushKeyboardQueue();
            //g_player[myconnectindex].ps->palette = palette;
            P_SetGamePalette(g_player[myconnectindex].ps, palette, 11);   // JBF 20040308

            rotatesprite(0,0,65536L,0,3292,0,0,2+8+16+64, 0,0,xdim-1,ydim-1);
            IFISSOFTMODE fadepal(0,0,0, 63,0,-1);
            else nextpage();
            while (!KB_KeyWaiting() && !MOUSE_GetButtons()&LEFT_MOUSE)
            {
                handleevents();
                Net_GetPackets();
            }
            fadepal(0,0,0, 0,64,1);
            S_StopMusic();
            FX_StopAllSounds();
            S_ClearSoundLocks();
            break;
        case 1:
            S_StopMusic();
            clearview(0L);
            nextpage();

            if (ud.lockout == 0)
            {
                G_PlayAnim("cineov2.anm",1);
                KB_FlushKeyBoardQueue();
                clearview(0L);
                nextpage();
            }

            S_PlaySound(PIPEBOMB_EXPLODE);

            fadepal(0,0,0, 0,64,1);
            setview(0,0,xdim-1,ydim-1);
            KB_FlushKeyboardQueue();
            //g_player[myconnectindex].ps->palette = palette;
            P_SetGamePalette(g_player[myconnectindex].ps, palette, 11);   // JBF 20040308
            rotatesprite(0,0,65536L,0,3293,0,0,2+8+16+64, 0,0,xdim-1,ydim-1);
            IFISSOFTMODE fadepal(0,0,0, 63,0,-1);
            else nextpage();
            while (!KB_KeyWaiting() && !MOUSE_GetButtons()&LEFT_MOUSE)
            {
                handleevents();
                Net_GetPackets();
            }
            IFISSOFTMODE fadepal(0,0,0, 0,64,1);

            break;

        case 3:

            setview(0,0,xdim-1,ydim-1);

            S_StopMusic();
            clearview(0L);
            nextpage();

            if (ud.lockout == 0)
            {
                KB_FlushKeyboardQueue();
                G_PlayAnim("vol4e1.anm",8);
                clearview(0L);
                nextpage();
                G_PlayAnim("vol4e2.anm",10);
                clearview(0L);
                nextpage();
                G_PlayAnim("vol4e3.anm",11);
                clearview(0L);
                nextpage();
            }

            FX_StopAllSounds();
            S_ClearSoundLocks();
            S_PlaySound(ENDSEQVOL3SND4);
            KB_FlushKeyBoardQueue();

            //g_player[myconnectindex].ps->palette = palette;
            P_SetGamePalette(g_player[myconnectindex].ps, palette, 11);   // JBF 20040308
            IFISSOFTMODE G_FadePalette(0,0,0,63);
            clearview(0L);
            menutext(160,60,0,0,"THANKS TO ALL OUR");
            menutext(160,60+16,0,0,"FANS FOR GIVING");
            menutext(160,60+16+16,0,0,"US BIG HEADS.");
            menutext(160,70+16+16+16,0,0,"LOOK FOR A DUKE NUKEM 3D");
            menutext(160,70+16+16+16+16,0,0,"SEQUEL SOON.");
            nextpage();

            fadepal(0,0,0, 63,0,-3);
            KB_FlushKeyboardQueue();
            while (!KB_KeyWaiting() && !MOUSE_GetButtons()&LEFT_MOUSE)
            {
                handleevents();
                Net_GetPackets();
            }
            fadepal(0,0,0, 0,64,3);

            clearview(0L);
            nextpage();
            MOUSE_ClearButton(LEFT_MOUSE);

            G_PlayAnim("DUKETEAM.ANM",4);

            KB_FlushKeyBoardQueue();
            while (!KB_KeyWaiting() && !MOUSE_GetButtons()&LEFT_MOUSE)
            {
                handleevents();
                Net_GetPackets();
            }

            clearview(0L);
            nextpage();
            IFISSOFTMODE G_FadePalette(0,0,0,63);

            FX_StopAllSounds();
            S_ClearSoundLocks();
            KB_FlushKeyBoardQueue();
            MOUSE_ClearButton(LEFT_MOUSE);

            break;

        case 2:

            S_StopMusic();
            clearview(0L);
            nextpage();
            if (ud.lockout == 0)
            {
                fadepal(0,0,0, 63,0,-1);
                G_PlayAnim("cineov3.anm",2);
                KB_FlushKeyBoardQueue();
                ototalclock = totalclock+200;
                while (totalclock < ototalclock)
                {
                    handleevents();
                    Net_GetPackets();
                }
                clearview(0L);
                nextpage();

                FX_StopAllSounds();
                S_ClearSoundLocks();
            }

            G_PlayAnim("RADLOGO.ANM",3);

            if (ud.lockout == 0 && !KB_KeyWaiting() && !MOUSE_GetButtons()&LEFT_MOUSE)
            {
                S_PlaySound(ENDSEQVOL3SND5);
                while (S_CheckSoundPlaying(-1,ENDSEQVOL3SND5))
                {
                    handleevents();
                    Net_GetPackets();
                }
                if (KB_KeyWaiting() || MOUSE_GetButtons()&LEFT_MOUSE) goto ENDANM;
                S_PlaySound(ENDSEQVOL3SND6);
                while (S_CheckSoundPlaying(-1,ENDSEQVOL3SND6))
                {
                    handleevents();
                    Net_GetPackets();
                }
                if (KB_KeyWaiting() || MOUSE_GetButtons()&LEFT_MOUSE) goto ENDANM;
                S_PlaySound(ENDSEQVOL3SND7);
                while (S_CheckSoundPlaying(-1,ENDSEQVOL3SND7))
                {
                    handleevents();
                    Net_GetPackets();
                }
                if (KB_KeyWaiting() || MOUSE_GetButtons()&LEFT_MOUSE) goto ENDANM;
                S_PlaySound(ENDSEQVOL3SND8);
                while (S_CheckSoundPlaying(-1,ENDSEQVOL3SND8))
                {
                    handleevents();
                    Net_GetPackets();
                }
                if (KB_KeyWaiting() || MOUSE_GetButtons()&LEFT_MOUSE) goto ENDANM;
                S_PlaySound(ENDSEQVOL3SND9);
                while (S_CheckSoundPlaying(-1,ENDSEQVOL3SND9))
                {
                    handleevents();
                    Net_GetPackets();
                }

            }
            MOUSE_ClearButton(LEFT_MOUSE);
            KB_FlushKeyBoardQueue();
            totalclock = 0;
            while (!KB_KeyWaiting() && totalclock < 120 && !MOUSE_GetButtons()&LEFT_MOUSE)
            {
                handleevents();
                Net_GetPackets();
            }

ENDANM:
            MOUSE_ClearButton(LEFT_MOUSE);
            FX_StopAllSounds();
            S_ClearSoundLocks();

            KB_FlushKeyBoardQueue();

            clearview(0L);

            break;
        }

FRAGBONUS:

    //g_player[myconnectindex].ps->palette = palette;
    P_SetGamePalette(g_player[myconnectindex].ps, palette, 11);   // JBF 20040308
    IFISSOFTMODE G_FadePalette(0,0,0,63);   // JBF 20031228
    KB_FlushKeyboardQueue();
    totalclock = 0;
    tinc = 0;
    bonuscnt = 0;

    S_StopMusic();
    FX_StopAllSounds();
    S_ClearSoundLocks();

    if (playerswhenstarted > 1 && (GametypeFlags[ud.coop]&GAMETYPE_SCORESHEET))
    {
        if (!(ud.config.MusicToggle == 0 || ud.config.MusicDevice < 0))
            S_PlaySound(BONUSMUSIC);

        rotatesprite(0,0,65536L,0,MENUSCREEN,16,0,2+8+16+64,0,0,xdim-1,ydim-1);
        rotatesprite(160<<16,34<<16,65536L,0,INGAMEDUKETHREEDEE,0,0,10,0,0,xdim-1,ydim-1);
        if (PLUTOPAK)   // JBF 20030804
            rotatesprite((260)<<16,36<<16,65536L,0,PLUTOPAKSPRITE+2,0,0,2+8,0,0,xdim-1,ydim-1);
        gametext(160,58+2,"MULTIPLAYER TOTALS",0,2+8+16);
        gametext(160,58+10,MapInfo[(ud.volume_number*MAXLEVELS)+ud.last_level-1].name,0,2+8+16);

        gametext(160,165,"PRESS ANY KEY TO CONTINUE",0,2+8+16);

        t = 0;
        minitext(23,80,"   NAME                                           KILLS",8,2+8+16+128);
        for (i=0; i<playerswhenstarted; i++)
        {
            Bsprintf(tempbuf,"%-4d",i+1);
            minitext(92+(i*23),80,tempbuf,3,2+8+16+128);
        }

        for (i=0; i<playerswhenstarted; i++)
        {
            xfragtotal = 0;
            Bsprintf(tempbuf,"%d",i+1);

            minitext(30,90+t,tempbuf,0,2+8+16+128);
            minitext(38,90+t,g_player[i].user_name,g_player[i].ps->palookup,2+8+16+128);

            for (y=0; y<playerswhenstarted; y++)
            {
                if (i == y)
                {
                    Bsprintf(tempbuf,"%-4d",g_player[y].ps->fraggedself);
                    minitext(92+(y*23),90+t,tempbuf,2,2+8+16+128);
                    xfragtotal -= g_player[y].ps->fraggedself;
                }
                else
                {
                    Bsprintf(tempbuf,"%-4d",g_player[i].frags[y]);
                    minitext(92+(y*23),90+t,tempbuf,0,2+8+16+128);
                    xfragtotal += g_player[i].frags[y];
                }

                if (myconnectindex == connecthead)
                {
                    Bsprintf(tempbuf,"stats %d killed %d %d\n",i+1,y+1,g_player[i].frags[y]);
                    sendscore(tempbuf);
                }
            }

            Bsprintf(tempbuf,"%-4d",xfragtotal);
            minitext(101+(8*23),90+t,tempbuf,2,2+8+16+128);

            t += 7;
        }

        for (y=0; y<playerswhenstarted; y++)
        {
            yfragtotal = 0;
            for (i=0; i<playerswhenstarted; i++)
            {
                if (i == y)
                    yfragtotal += g_player[i].ps->fraggedself;
                yfragtotal += g_player[i].frags[y];
            }
            Bsprintf(tempbuf,"%-4d",yfragtotal);
            minitext(92+(y*23),96+(8*7),tempbuf,2,2+8+16+128);
        }

        minitext(45,96+(8*7),"DEATHS",8,2+8+16+128);
        nextpage();

        fadepal(0,0,0, 63,0,-7);

        KB_FlushKeyboardQueue();

        {
            int32_t tc = totalclock;
            while (KB_KeyWaiting()==0)
            {
                // continue after 10 seconds...
                if (totalclock > tc + (120*10)) break;
                handleevents();
                Net_GetPackets();
            }
        }

        if (bonusonly || ud.multimode > 1) return;

        fadepal(0,0,0, 0,64,7);
    }

    if (bonusonly || ud.multimode > 1) return;

    switch (ud.volume_number)
    {
    case 1:
        gfx_offset = 5;
        break;
    default:
        gfx_offset = 0;
        break;
    }

    rotatesprite(0,0,65536L,0,BONUSSCREEN+gfx_offset,0,0,2+8+16+64+128,0,0,xdim-1,ydim-1);

    menutext(160,20-6,0,0,lastmapname);
    menutext(160,36-6,0,0,"COMPLETED");

    gametext(160,192,"PRESS ANY KEY TO CONTINUE",16,2+8+16);

    if (!(ud.config.MusicToggle == 0 || ud.config.MusicDevice < 0))
        S_PlaySound(BONUSMUSIC);

    nextpage();
    KB_FlushKeyboardQueue();
    fadepal(0,0,0, 63,0,-1);
    bonuscnt = 0;
    totalclock = 0;
    tinc = 0;

    playerbest = CONFIG_GetMapBestTime(MapInfo[ud.volume_number*MAXLEVELS+ud.last_level-1].filename);

    if (g_player[myconnectindex].ps->player_par < playerbest || playerbest < 0)
    {
        CONFIG_SetMapBestTime(MapInfo[ud.volume_number*MAXLEVELS+ud.last_level-1].filename, g_player[myconnectindex].ps->player_par);
        //        if(playerbest != -1)
        //            playerbest = g_player[myconnectindex].ps->player_par;
    }

    {
        int32_t ii, ij;

        for (ii=g_player[myconnectindex].ps->player_par/(GAMETICSPERSEC*60), ij=1; ii>9; ii/=10, ij++) ;
        clockpad = max(clockpad,ij);
        if (!(ud.volume_number == 0 && ud.last_level-1 == 7))
        {
            for (ii=MapInfo[ud.volume_number*MAXLEVELS+ud.last_level-1].partime/(GAMETICSPERSEC*60), ij=1; ii>9; ii/=10, ij++) ;
            clockpad = max(clockpad,ij);
            if (!NAM)
            {
                for (ii=MapInfo[ud.volume_number*MAXLEVELS+ud.last_level-1].designertime/(GAMETICSPERSEC*60), ij=1; ii>9; ii/=10, ij++) ;
                clockpad = max(clockpad,ij);
            }
        }
        if (playerbest > 0) for (ii=playerbest/(GAMETICSPERSEC*60), ij=1; ii>9; ii/=10, ij++) ;
        clockpad = max(clockpad,ij);
    }

    while (1)
    {
        int32_t yy = 0, zz;

        Net_GetPackets();
        handleevents();
        MUSIC_Update();

        if (g_player[myconnectindex].ps->gm&MODE_EOL)
        {
            rotatesprite(0,0,65536L,0,BONUSSCREEN+gfx_offset,0,0,2+8+16+64+128,0,0,xdim-1,ydim-1);

            if (totalclock > (1000000000L) && totalclock < (1000000320L))
            {
                switch ((totalclock>>4)%15)
                {
                case 0:
                    if (bonuscnt == 6)
                    {
                        bonuscnt++;
                        S_PlaySound(SHOTGUN_COCK);
                        switch (rand()&3)
                        {
                        case 0:
                            S_PlaySound(BONUS_SPEECH1);
                            break;
                        case 1:
                            S_PlaySound(BONUS_SPEECH2);
                            break;
                        case 2:
                            S_PlaySound(BONUS_SPEECH3);
                            break;
                        case 3:
                            S_PlaySound(BONUS_SPEECH4);
                            break;
                        }
                    }
                case 1:
                case 4:
                case 5:
                    rotatesprite(199<<16,31<<16,65536L,0,BONUSSCREEN+3+gfx_offset,0,0,2+8+16+64+128,0,0,xdim-1,ydim-1);
                    break;
                case 2:
                case 3:
                    rotatesprite(199<<16,31<<16,65536L,0,BONUSSCREEN+4+gfx_offset,0,0,2+8+16+64+128,0,0,xdim-1,ydim-1);
                    break;
                }
            }
            else if (totalclock > (10240+120L)) break;
            else
            {
                switch ((totalclock>>5)&3)
                {
                case 1:
                case 3:
                    rotatesprite(199<<16,31<<16,65536L,0,BONUSSCREEN+1+gfx_offset,0,0,2+8+16+64+128,0,0,xdim-1,ydim-1);
                    break;
                case 2:
                    rotatesprite(199<<16,31<<16,65536L,0,BONUSSCREEN+2+gfx_offset,0,0,2+8+16+64+128,0,0,xdim-1,ydim-1);
                    break;
                }
            }

            menutext(160,20-6,0,0,lastmapname);
            menutext(160,36-6,0,0,"COMPLETED");

            gametext(160,192,"PRESS ANY KEY TO CONTINUE",16,2+8+16);

            if (totalclock > (60*3))
            {
                yy = zz = 59;
                gametext(10,yy+9,"Your Time:",0,2+8+16);
                yy+=10;
                if (!(ud.volume_number == 0 && ud.last_level-1 == 7))
                {
                    gametext(10,yy+9,"Par Time:",0,2+8+16);
                    yy+=10;
                    if (!NAM)
                    {
                        gametext(10,yy+9,"3D Realms' Time:",0,2+8+16);
                        yy+=10;
                    }

                }
                if (playerbest > 0)
                {
                    gametext(10,yy+9,g_player[myconnectindex].ps->player_par<playerbest?"Prev Best Time:":"Your Best Time:",0,2+8+16);
                    yy += 10;
                }

                if (bonuscnt == 0)
                    bonuscnt++;

                yy = zz;
                if (totalclock > (60*4))
                {
                    if (bonuscnt == 1)
                    {
                        bonuscnt++;
                        S_PlaySound(PIPEBOMB_EXPLODE);
                    }

                    Bsprintf(tempbuf,"%0*d:%02d.%02d",clockpad,
                             (g_player[myconnectindex].ps->player_par/(GAMETICSPERSEC*60)),
                             (g_player[myconnectindex].ps->player_par/GAMETICSPERSEC)%60,
                             ((g_player[myconnectindex].ps->player_par%GAMETICSPERSEC)*38)/10
                            );
                    gametext((320>>2)+71,yy+9,tempbuf,0,2+8+16);
                    if (g_player[myconnectindex].ps->player_par < playerbest)
                        gametext((320>>2)+89+(clockpad*24),yy+9,"New record!",0,2+8+16);
                    yy+=10;

                    if (!(ud.volume_number == 0 && ud.last_level-1 == 7))
                    {
                        Bsprintf(tempbuf,"%0*d:%02d",clockpad,
                                 (MapInfo[ud.volume_number*MAXLEVELS+ud.last_level-1].partime/(GAMETICSPERSEC*60)),
                                 (MapInfo[ud.volume_number*MAXLEVELS+ud.last_level-1].partime/GAMETICSPERSEC)%60);
                        gametext((320>>2)+71,yy+9,tempbuf,0,2+8+16);
                        yy+=10;

                        if (!NAM)
                        {
                            Bsprintf(tempbuf,"%0*d:%02d",clockpad,
                                     (MapInfo[ud.volume_number*MAXLEVELS+ud.last_level-1].designertime/(GAMETICSPERSEC*60)),
                                     (MapInfo[ud.volume_number*MAXLEVELS+ud.last_level-1].designertime/GAMETICSPERSEC)%60);
                            gametext((320>>2)+71,yy+9,tempbuf,0,2+8+16);
                            yy+=10;
                        }
                    }

                    if (playerbest > 0)
                    {
                        Bsprintf(tempbuf,"%0*d:%02d.%02d",clockpad,
                                 (playerbest/(GAMETICSPERSEC*60)),
                                 (playerbest/GAMETICSPERSEC)%60,
                                 ((playerbest%GAMETICSPERSEC)*38)/10
                                );
                        gametext((320>>2)+71,yy+9,tempbuf,0,2+8+16);
                        yy+=10;
                    }
                }
            }

            zz = yy += 5;
            if (totalclock > (60*6))
            {
                gametext(10,yy+9,"Enemies Killed:",0,2+8+16);
                yy += 10;
                gametext(10,yy+9,"Enemies Left:",0,2+8+16);
                yy += 10;

                if (bonuscnt == 2)
                {
                    bonuscnt++;
                    S_PlaySound(FLY_BY);
                }

                yy = zz;

                if (totalclock > (60*7))
                {
                    if (bonuscnt == 3)
                    {
                        bonuscnt++;
                        S_PlaySound(PIPEBOMB_EXPLODE);
                    }
                    Bsprintf(tempbuf,"%-3d",g_player[myconnectindex].ps->actors_killed);
                    gametext((320>>2)+70,yy+9,tempbuf,0,2+8+16);
                    yy += 10;
                    if (ud.player_skill > 3)
                    {
                        Bsprintf(tempbuf,"N/A");
                        gametext((320>>2)+70,yy+9,tempbuf,0,2+8+16);
                        yy += 10;
                    }
                    else
                    {
                        if ((g_player[myconnectindex].ps->max_actors_killed-g_player[myconnectindex].ps->actors_killed) < 0)
                            Bsprintf(tempbuf,"%-3d",0);
                        else Bsprintf(tempbuf,"%-3d",g_player[myconnectindex].ps->max_actors_killed-g_player[myconnectindex].ps->actors_killed);
                        gametext((320>>2)+70,yy+9,tempbuf,0,2+8+16);
                        yy += 10;
                    }
                }
            }

            zz = yy += 5;
            if (totalclock > (60*9))
            {
                gametext(10,yy+9,"Secrets Found:",0,2+8+16);
                yy += 10;
                gametext(10,yy+9,"Secrets Missed:",0,2+8+16);
                yy += 10;
                if (bonuscnt == 4) bonuscnt++;

                yy = zz;
                if (totalclock > (60*10))
                {
                    if (bonuscnt == 5)
                    {
                        bonuscnt++;
                        S_PlaySound(PIPEBOMB_EXPLODE);
                    }
                    Bsprintf(tempbuf,"%-3d",g_player[myconnectindex].ps->secret_rooms);
                    gametext((320>>2)+70,yy+9,tempbuf,0,2+8+16);
                    yy += 10;
                    if (g_player[myconnectindex].ps->secret_rooms > 0)
                        Bsprintf(tempbuf,"%-3d%%",(100*g_player[myconnectindex].ps->secret_rooms/g_player[myconnectindex].ps->max_secret_rooms));
                    Bsprintf(tempbuf,"%-3d",g_player[myconnectindex].ps->max_secret_rooms-g_player[myconnectindex].ps->secret_rooms);
                    gametext((320>>2)+70,yy+9,tempbuf,0,2+8+16);
                    yy += 10;
                }
            }

            if (totalclock > 10240 && totalclock < 10240+10240)
                totalclock = 1024;

            if (((MOUSE_GetButtons()&7) || KB_KeyWaiting()) && totalclock > (60*2)) // JBF 20030809
            {
                MOUSE_ClearButton(7);
                if (totalclock < (60*13))
                {
                    KB_FlushKeyboardQueue();
                    totalclock = (60*13);
                }
                else if (totalclock < (1000000000L))
                    totalclock = (1000000000L);
            }
        }
        else break;
        X_OnEvent(EVENT_DISPLAYBONUSSCREEN, g_player[screenpeek].ps->i, screenpeek, -1);
        nextpage();
    }
}

static void G_DrawCameraText(int16_t i)
{
    char flipbits;
    int32_t x , y;

    if (!T1)
    {
        rotatesprite(24<<16,33<<16,65536L,0,CAMCORNER,0,0,2,windowx1,windowy1,windowx2,windowy2);
        rotatesprite((320-26)<<16,34<<16,65536L,0,CAMCORNER+1,0,0,2,windowx1,windowy1,windowx2,windowy2);
        rotatesprite(22<<16,163<<16,65536L,512,CAMCORNER+1,0,0,2+4,windowx1,windowy1,windowx2,windowy2);
        rotatesprite((310-10)<<16,163<<16,65536L,512,CAMCORNER+1,0,0,2,windowx1,windowy1,windowx2,windowy2);
        if (totalclock&16)
            rotatesprite(46<<16,32<<16,65536L,0,CAMLIGHT,0,0,2,windowx1,windowy1,windowx2,windowy2);
    }
    else
    {
        flipbits = (totalclock<<1)&48;
        for (x=0; x<394; x+=64)
            for (y=0; y<200; y+=64)
                rotatesprite(x<<16,y<<16,65536L,0,STATIC,0,0,2+flipbits,windowx1,windowy1,windowx2,windowy2);
    }
}

#if 0
void vglass(int32_t x,int32_t y,short a,short wn,short n)
{
    int32_t z, zincs;
    short sect;

    sect = wall[wn].nextsector;
    if (sect == -1) return;
    zincs = (sector[sect].floorz-sector[sect].ceilingz) / n;

    for (z = sector[sect].ceilingz; z < sector[sect].floorz; z += zincs)
        A_InsertSprite(sect,x,y,z-(krand()&8191),GLASSPIECES+(z&(krand()%3)),-32,36,36,a+128-(krand()&255),16+(krand()&31),0,-1,5);
}
#endif

void A_SpawnWallGlass(int32_t i,int32_t wallnum,int32_t n)
{
    int32_t j, xv, yv, z, x1, y1;
    int16_t sect;
    int32_t a;

    sect = -1;

    if (wallnum < 0)
    {
        for (j=n-1; j >= 0 ; j--)
        {
            a = SA-256+(krand()&511)+1024;
            A_InsertSprite(SECT,SX,SY,SZ,GLASSPIECES+(j%3),-32,36,36,a,32+(krand()&63),1024-(krand()&1023),i,5);
        }
        return;
    }

    j = n+1;

    x1 = wall[wallnum].x;
    y1 = wall[wallnum].y;

    xv = wall[wall[wallnum].point2].x-x1;
    yv = wall[wall[wallnum].point2].y-y1;

    x1 -= ksgn(yv);
    y1 += ksgn(xv);

    xv /= j;
    yv /= j;

    for (j=n; j>0; j--)
    {
        x1 += xv;
        y1 += yv;

        updatesector(x1,y1,&sect);
        if (sect >= 0)
        {
            z = sector[sect].floorz-(krand()&(klabs(sector[sect].ceilingz-sector[sect].floorz)));
            if (z < -(32<<8) || z > (32<<8))
                z = SZ-(32<<8)+(krand()&((64<<8)-1));
            a = SA-1024;
            A_InsertSprite(SECT,x1,y1,z,GLASSPIECES+(j%3),-32,36,36,a,32+(krand()&63),-(krand()&1023),i,5);
        }
    }
}

void A_SpawnGlass(int32_t i,int32_t n)
{
    int32_t j, k, a, z;

    for (j=n; j>0; j--)
    {
        a = krand()&2047;
        z = SZ-((krand()&16)<<8);
        k = A_InsertSprite(SECT,SX,SY,z,GLASSPIECES+(j%3),krand()&15,36,36,a,32+(krand()&63),-512-(krand()&2047),i,5);
        sprite[k].pal = sprite[i].pal;
    }
}

void A_SpawnCeilingGlass(int32_t i,int32_t sectnum,int32_t n)
{
    int32_t j, xv, yv, z, x1, y1, a,s;
    int32_t startwall = sector[sectnum].wallptr;
    int32_t endwall = startwall+sector[sectnum].wallnum;

    for (s=startwall; s<(endwall-1); s++)
    {
        x1 = wall[s].x;
        y1 = wall[s].y;

        xv = (wall[s+1].x-x1)/(n+1);
        yv = (wall[s+1].y-y1)/(n+1);

        for (j=n; j>0; j--)
        {
            x1 += xv;
            y1 += yv;
            a = krand()&2047;
            z = sector[sectnum].ceilingz+((krand()&15)<<8);
            A_InsertSprite(sectnum,x1,y1,z,GLASSPIECES+(j%3),-32,36,36,a,(krand()&31),0,i,5);
        }
    }
}

void A_SpawnRandomGlass(int32_t i,int32_t wallnum,int32_t n)
{
    int32_t j, xv, yv, z, x1, y1;
    int16_t sect = -1;
    int32_t a, k;

    if (wallnum < 0)
    {
        for (j=n-1; j >= 0 ; j--)
        {
            a = krand()&2047;
            k = A_InsertSprite(SECT,SX,SY,SZ-(krand()&(63<<8)),GLASSPIECES+(j%3),-32,36,36,a,32+(krand()&63),1024-(krand()&2047),i,5);
            sprite[k].pal = krand()&15;
        }
        return;
    }

    j = n+1;
    x1 = wall[wallnum].x;
    y1 = wall[wallnum].y;

    xv = (wall[wall[wallnum].point2].x-wall[wallnum].x)/j;
    yv = (wall[wall[wallnum].point2].y-wall[wallnum].y)/j;

    for (j=n; j>0; j--)
    {
        x1 += xv;
        y1 += yv;

        updatesector(x1,y1,&sect);
        z = sector[sect].floorz-(krand()&(klabs(sector[sect].ceilingz-sector[sect].floorz)));
        if (z < -(32<<8) || z > (32<<8))
            z = SZ-(32<<8)+(krand()&((64<<8)-1));
        a = SA-1024;
        k = A_InsertSprite(SECT,x1,y1,z,GLASSPIECES+(j%3),-32,36,36,a,32+(krand()&63),-(krand()&2047),i,5);
        sprite[k].pal = krand()&7;
    }
}

static void G_SetupGameButtons(void)
{
    CONTROL_DefineFlag(gamefunc_Move_Forward,FALSE);
    CONTROL_DefineFlag(gamefunc_Move_Backward,FALSE);
    CONTROL_DefineFlag(gamefunc_Turn_Left,FALSE);
    CONTROL_DefineFlag(gamefunc_Turn_Right,FALSE);
    CONTROL_DefineFlag(gamefunc_Strafe,FALSE);
    CONTROL_DefineFlag(gamefunc_Fire,FALSE);
    CONTROL_DefineFlag(gamefunc_Open,FALSE);
    CONTROL_DefineFlag(gamefunc_Run,FALSE);
    CONTROL_DefineFlag(gamefunc_AutoRun,FALSE);
    CONTROL_DefineFlag(gamefunc_Jump,FALSE);
    CONTROL_DefineFlag(gamefunc_Crouch,FALSE);
    CONTROL_DefineFlag(gamefunc_Look_Up,FALSE);
    CONTROL_DefineFlag(gamefunc_Look_Down,FALSE);
    CONTROL_DefineFlag(gamefunc_Look_Left,FALSE);
    CONTROL_DefineFlag(gamefunc_Look_Right,FALSE);
    CONTROL_DefineFlag(gamefunc_Strafe_Left,FALSE);
    CONTROL_DefineFlag(gamefunc_Strafe_Right,FALSE);
    CONTROL_DefineFlag(gamefunc_Aim_Up,FALSE);
    CONTROL_DefineFlag(gamefunc_Aim_Down,FALSE);
    CONTROL_DefineFlag(gamefunc_Weapon_1,FALSE);
    CONTROL_DefineFlag(gamefunc_Weapon_2,FALSE);
    CONTROL_DefineFlag(gamefunc_Weapon_3,FALSE);
    CONTROL_DefineFlag(gamefunc_Weapon_4,FALSE);
    CONTROL_DefineFlag(gamefunc_Weapon_5,FALSE);
    CONTROL_DefineFlag(gamefunc_Weapon_6,FALSE);
    CONTROL_DefineFlag(gamefunc_Weapon_7,FALSE);
    CONTROL_DefineFlag(gamefunc_Weapon_8,FALSE);
    CONTROL_DefineFlag(gamefunc_Weapon_9,FALSE);
    CONTROL_DefineFlag(gamefunc_Weapon_10,FALSE);
    CONTROL_DefineFlag(gamefunc_Inventory,FALSE);
    CONTROL_DefineFlag(gamefunc_Inventory_Left,FALSE);
    CONTROL_DefineFlag(gamefunc_Inventory_Right,FALSE);
    CONTROL_DefineFlag(gamefunc_Holo_Duke,FALSE);
    CONTROL_DefineFlag(gamefunc_Jetpack,FALSE);
    CONTROL_DefineFlag(gamefunc_NightVision,FALSE);
    CONTROL_DefineFlag(gamefunc_MedKit,FALSE);
    CONTROL_DefineFlag(gamefunc_TurnAround,FALSE);
    CONTROL_DefineFlag(gamefunc_SendMessage,FALSE);
    CONTROL_DefineFlag(gamefunc_Map,FALSE);
    CONTROL_DefineFlag(gamefunc_Shrink_Screen,FALSE);
    CONTROL_DefineFlag(gamefunc_Enlarge_Screen,FALSE);
    CONTROL_DefineFlag(gamefunc_Center_View,FALSE);
    CONTROL_DefineFlag(gamefunc_Holster_Weapon,FALSE);
    CONTROL_DefineFlag(gamefunc_Show_Opponents_Weapon,FALSE);
    CONTROL_DefineFlag(gamefunc_Map_Follow_Mode,FALSE);
    CONTROL_DefineFlag(gamefunc_See_Coop_View,FALSE);
    CONTROL_DefineFlag(gamefunc_Mouse_Aiming,FALSE);
    CONTROL_DefineFlag(gamefunc_Toggle_Crosshair,FALSE);
    CONTROL_DefineFlag(gamefunc_Steroids,FALSE);
    CONTROL_DefineFlag(gamefunc_Quick_Kick,FALSE);
    CONTROL_DefineFlag(gamefunc_Next_Weapon,FALSE);
    CONTROL_DefineFlag(gamefunc_Previous_Weapon,FALSE);
}

/*
===================
=
= GetTime
=
===================
*/

int32_t GetTime(void)
{
    return totalclock;
}
