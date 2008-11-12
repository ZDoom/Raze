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
#include "types.h"
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

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <shellapi.h>
#include <winsock2.h>
#include <ws2tcpip.h>
extern int getversionfromwebsite(char *buffer);
#define UPDATEINTERVAL 604800 // 1w
#else
static int usecwd = 0;
#endif /* _WIN32 */
int checkCON = 1;

#define IDFSIZE 479985668
#define IDFILENAME "DUKE3D.IDF"

#define TIMERUPDATESIZ 32

int cameradist = 0, cameraclock = 0;
static int playerswhenstarted;
static int qe,cp;
static int g_CommandSetup = 0;
int g_NoSetup = 0;
static int g_NoAutoLoad = 0;
static int g_NoSound = 0;
static int g_NoMusic = 0;
static char *CommandMap = NULL;
static char *CommandName = NULL;
#ifndef RANCID_NETWORKING
static char *CommandNet = NULL;
#endif
static int g_KeepAddr = 0;
int CommandWeaponChoice = 0;
static struct strllist
{
    struct strllist *next;
    char *str;
}
*CommandPaths = NULL, *CommandGrps = NULL;

char boardfilename[BMAX_PATH] = {0}, currentboardfilename[BMAX_PATH] = {0};
char root[BMAX_PATH];
char waterpal[768], slimepal[768], titlepal[768], drealms[768], endingpal[768], animpal[768];
static char firstdemofile[80] = { '\0' };
static int userconfiles = 0;

static int netparamcount = 0;
static char **netparam = NULL;

int voting = -1;
int vote_map = -1, vote_episode = -1;

int recfilep,totalreccnt;
int debug_on = 0,actor_tog = 0;
static char *rtsptr;

//extern char syncstate; 
extern int numlumps;

static FILE *frecfilep = (FILE *)NULL;

int restorepalette,screencapt;
static int g_NoLogoAnim = 0;
static int g_NoLogo = 0;
static int sendmessagecommand = -1;

char defaultduke3dgrp[BMAX_PATH] = "duke3d.grp";
char *duke3dgrp = defaultduke3dgrp;
char *duke3dgrpstring = NULL;
static char defaultconfilename[BMAX_PATH] = {"EDUKE.CON"};
static char *confilename = defaultconfilename;
char *duke3ddef = "duke3d.def";
char mod_dir[BMAX_PATH] = "/";
#if defined(POLYMOST)
extern char TEXCACHEDIR[BMAX_PATH];
#endif
extern int lastvisinc;

int g_Shareware = 0;
int g_GameType = 0;

#define MAXUSERQUOTES 6
static int quotebot, quotebotgoal;
static int user_quote_time[MAXUSERQUOTES];
static char user_quote[MAXUSERQUOTES][178];
// char typebuflen,typebuf[41];

static int MAXCACHE1DSIZE = (32*1048576);

int tempwallptr;

static int nonsharedtimer;

static void cameratext(short i);
static int moveloop(void);
static void doorders(void);
static void fakedomovethings(void);
static void fakedomovethingscorrect(void);
static int domovethings(void);
static int playback(void);

static char recbuf[180];

extern void computergetinput(int snum, input_t *syn);

#define USERQUOTE_LEFTOFFSET 5
#define USERQUOTE_RIGHTOFFSET 14

int althud_numbertile = 2930;
int althud_numberpal = 0;
int althud_shadows = 1;
int althud_flashing = 1;
int hud_glowingquotes = 1;
int hud_showmapname = 1;

int leveltexttime = 0;

int r_maxfps = 0;
unsigned int g_FrameDelay = 0;

#ifdef RENDERTYPEWIN
extern char forcegl;
#endif

#ifdef _WIN32
int getversionfromwebsite(char *buffer) // FIXME: this probably belongs in game land
{
    int wsainitialized = 0;
    int bytes_sent, i=0, j=0;
    struct sockaddr_in dest_addr;
    struct hostent *h;
    char *host = "eduke32.sourceforge.net";
    char *req = "GET http://eduke32.sourceforge.net/VERSION HTTP/1.0\r\n\r\n";
    char tempbuf[2048],otherbuf[16],ver[16];
    SOCKET mysock;

#ifdef _WIN32
    if (wsainitialized == 0)
    {
        WSADATA ws;

        if (WSAStartup(0x101,&ws) == SOCKET_ERROR)
        {
            initprintf("update: Winsock error in getversionfromwebsite() (%d)\n",errno);
            return(0);
        }
        wsainitialized = 1;
    }
#endif

    if ((h=gethostbyname(host)) == NULL)
    {
        initprintf("update: gethostbyname() error in getversionfromwebsite() (%d)\n",h_errno);
        return(0);
    }

    dest_addr.sin_addr.s_addr = ((struct in_addr *)(h->h_addr))->s_addr;
    dest_addr.sin_family = AF_INET;
    dest_addr.sin_port = htons(80);

    memset(&(dest_addr.sin_zero), '\0', 8);


    mysock = socket(PF_INET, SOCK_STREAM, 0);

    if (mysock == INVALID_SOCKET)
    {
        initprintf("update: socket() error in getversionfromwebsite() (%d)\n",errno);
        return(0);
    }
    initprintf("Connecting to \"http://%s\"\n",host);
    if (connect(mysock, (struct sockaddr *)&dest_addr, sizeof(struct sockaddr)) == SOCKET_ERROR)
    {
        initprintf("update: connect() error in getversionfromwebsite() (%d)\n",errno);
        return(0);
    }

    bytes_sent = send(mysock, req, strlen(req), 0);
    if (bytes_sent == SOCKET_ERROR)
    {
        initprintf("update: send() error in getversionfromwebsite() (%d)\n",errno);
        return(0);
    }

    //    initprintf("sent %d bytes\n",bytes_sent);
    recv(mysock, (char *)&tempbuf, sizeof(tempbuf), 0);
    closesocket(mysock);

    memcpy(&otherbuf,&tempbuf,sizeof(otherbuf));

    strtok(otherbuf," ");
    if (atol(strtok(NULL," ")) == 200)
    {
        for (i=0;(unsigned)i<strlen(tempbuf);i++) // HACK: all of this needs to die a fiery death; we just skip to the content
        {
            // instead of actually parsing any of the http headers
            if (i > 4)
                if (tempbuf[i-1] == '\n' && tempbuf[i-2] == '\r' && tempbuf[i-3] == '\n' && tempbuf[i-4] == '\r')
                {
                    while (j < 9)
                    {
                        ver[j] = tempbuf[i];
                        i++, j++;
                    }
                    ver[j] = '\0';
                    break;
                }
        }

        if (j)
        {
            strcpy(buffer,ver);
            return(1);
        }
    }
    return(0);
}
#endif

int kopen4loadfrommod(char *filename, char searchfirst)
{
    static char fn[BMAX_PATH];
    int r;

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

    for (i=ntokens-1;i>=0;i--)
    {
        if (!Bstrcasecmp(tok, tl[i].text))
            return tl[i].tokenid;
    }
    return T_ERROR;
}

inline void setstatusbarscale(int sc)
{
    ud.statusbarscale = min(100,max(10,sc));
    vscrn();
}

static inline int sbarx(int x)
{
    if (ud.screen_size == 4 /*|| ud.statusbarmode == 1*/) return scale(x<<16,ud.statusbarscale,100);
    return (((320l<<16) - scale(320l<<16,ud.statusbarscale,100)) >> 1) + scale(x<<16,ud.statusbarscale,100);
}

static inline int sbarxr(int x)
{
    if (ud.screen_size == 4 /*|| ud.statusbarmode == 1*/) return (320l<<16) - scale(x<<16,ud.statusbarscale,100);
    return (((320l<<16) - scale(320l<<16,ud.statusbarscale,100)) >> 1) + scale(x<<16,ud.statusbarscale,100);
}

static inline int sbary(int y)
{
    return ((200l<<16) - scale(200l<<16,ud.statusbarscale,100) + scale(y<<16,ud.statusbarscale,100));
}

static inline int sbarsc(int sc)
{
    return scale(sc,ud.statusbarscale,100);
}

static inline int textsc(int sc)
{
    // prevent ridiculousness to a degree
    if (xdim <= 320) return sc;
    else if (xdim <= 640) return scale(sc,min(200,ud.textscale),100);
    else if (xdim <= 800) return scale(sc,min(300,ud.textscale),100);
    else if (xdim <= 1024) return scale(sc,min(350,ud.textscale),100);
    return scale(sc,ud.textscale,100);
}

static void patchstatusbar(int x1, int y1, int x2, int y2)
{
    int scl, tx, ty;
    int clx1,cly1,clx2,cly2,clofx,clofy;

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

void setgamepalette(player_struct *player, char *pal, int set)
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

int gametext_z(int small, int starttile, int x,int y,const char *t,int s,int p,int orientation,int x1, int y1, int x2, int y2, int z)
{
    int ac,newx,oldx=x;
    char centre, *oldt;
    int squishtext = ((small&2)!=0);
//    int ht = usehightile;
    int shift = 16, widthx = 320, ox, oy;
    int origy = y;

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
            ox=x;oy=y;
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

int gametextlen(int x,const char *t)
{
    int ac;

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

static inline int mpgametext(int y,const char *t,int s,int dabits)
{
//    if (xdim < 640 || ydim < 480)
    //      return(gametext_z(0,STARTALPHANUM, 5,y,t,s,0,dabits,0, 0, xdim-1, ydim-1, 65536));
    return(gametext_z(4,STARTALPHANUM, 5,y,t,s,0,dabits,0, 0, xdim-1, ydim-1, 65536));
}

int minitext_(int x,int y,const char *t,int s,int p,int sb)
{
    int ac;
    char ch,cmode;
//    int ht = usehightile;

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

static void allowtimetocorrecterrorswhenquitting(void)
{
    int i, j, oldtotalclock;

    ready2send = 0;

    for (j=7;j>=0;j--)
    {
        oldtotalclock = totalclock;

        while (totalclock < oldtotalclock+TICSPERFRAME)
        {
            handleevents();
            getpackets();
        }
        if (KB_KeyPressed(sc_Escape)) return;

        packbuf[0] = 127;
        for (i=connecthead;i>=0;i=connectpoint2[i])
        {
            if (i != myconnectindex) sendpacket(i,packbuf,1);
            if ((!networkmode) && (myconnectindex != connecthead)) break; //slaves in M/S mode only send to master
        }
    }
}

void adduserquote(const char *daquote)
{
    int i;

    for (i=MAXUSERQUOTES-1;i>0;i--)
    {
        Bstrcpy(user_quote[i],user_quote[i-1]);
        user_quote_time[i] = user_quote_time[i-1];
    }
    Bstrcpy(user_quote[0],daquote);
    OSD_Printf("%s\n",daquote);

    user_quote_time[0] = ud.msgdisptime;
    pub = NUMPAGES;
}

int lastpackettime = 0;

void getpackets(void)
{
    int i, j, k, l;
    int other;
    int packbufleng;
    
    input_t *osyn, *nsyn;

    sampletimer();
    AudioUpdate();

    CONTROL_ProcessBinds();

    if (ALT_IS_PRESSED && KB_KeyPressed(sc_Enter))
    {
        if (setgamemode(!ud.config.ScreenMode,ud.config.ScreenWidth,ud.config.ScreenHeight,ud.config.ScreenBPP))
        {
            OSD_Printf(OSD_ERROR "Failed setting fullscreen video mode.\n");
            if (setgamemode(ud.config.ScreenMode, ud.config.ScreenWidth, ud.config.ScreenHeight, ud.config.ScreenBPP))
                gameexit("Failed to recover from failure to set fullscreen video mode.\n");
        }
        else ud.config.ScreenMode = !ud.config.ScreenMode;
        KB_ClearKeyDown(sc_Enter);
        restorepalette = 1;
        vscrn();
    }

    if (KB_UnBoundKeyPressed(sc_F12))
    {
        KB_ClearKeyDown(sc_F12);
        screencapture("duke0000.tga",0);
        FTA(103,g_player[myconnectindex].ps);
    }

    // only dispatch commands here when not in a game
    if (!(g_player[myconnectindex].ps->gm&MODE_GAME))
    {
        OSD_DispatchQueued();
    }

    if (qe == 0 && KB_KeyPressed(sc_LeftControl) && KB_KeyPressed(sc_LeftAlt) && (KB_KeyPressed(sc_Delete)||KB_KeyPressed(sc_End)))
    {
        qe = 1;
        gameexit("Quick Exit.");
    }

    if (numplayers < 2) return;
    while ((packbufleng = getpacket(&other,packbuf)) > 0)
    {
        lastpackettime = totalclock;
#if 0
        initprintf("RECEIVED PACKET: type: %d : len %d\n", packbuf[0], packbufleng);
#endif
        switch (packbuf[0])
        {
        case 0:  //[0] (receive master sync buffer)
            j = 1;

            if ((g_player[other].movefifoend&(TIMERUPDATESIZ-1)) == 0)
                for (i=connectpoint2[connecthead];i>=0;i=connectpoint2[i])
                {
                    if (g_player[i].playerquitflag == 0) continue;
                    if (i == myconnectindex)
                        otherminlag = (int)((signed char)packbuf[j]);
                    j++;
                }

            osyn = (input_t *)&inputfifo[(g_player[connecthead].movefifoend-1)&(MOVEFIFOSIZ-1)][0];
            nsyn = (input_t *)&inputfifo[(g_player[connecthead].movefifoend)&(MOVEFIFOSIZ-1)][0];

            k = j;
            for (i=connecthead;i>=0;i=connectpoint2[i])
                j += g_player[i].playerquitflag+g_player[i].playerquitflag;
            for (i=connecthead;i>=0;i=connectpoint2[i])
            {
                if (g_player[i].playerquitflag == 0) continue;

                l = packbuf[k]+(int)(packbuf[k+1]<<8);
                k += 2;

                if (i == myconnectindex)
                {
                    j += ((l&1)<<1)+(l&2)+((l&4)>>2)+((l&8)>>3)+((l&16)>>4)+((l&32)>>5)+((l&64)>>6)+((l&128)>>7)+((l&256)>>8)/*+((l&512)>>9)+((l&1024)>>10)+((l&2048)>>11)*/;
                    continue;
                }

                copybufbyte(&osyn[i],&nsyn[i],sizeof(input_t));
                if (l&1)   nsyn[i].fvel = packbuf[j]+((short)packbuf[j+1]<<8), j += 2;
                if (l&2)   nsyn[i].svel = packbuf[j]+((short)packbuf[j+1]<<8), j += 2;
                if (l&4)   nsyn[i].avel = (signed char)packbuf[j++];
                if (l&8)   nsyn[i].bits = ((nsyn[i].bits&0xffffff00)|((int)packbuf[j++]));
                if (l&16)  nsyn[i].bits = ((nsyn[i].bits&0xffff00ff)|((int)packbuf[j++])<<8);
                if (l&32)  nsyn[i].bits = ((nsyn[i].bits&0xff00ffff)|((int)packbuf[j++])<<16);
                if (l&64)  nsyn[i].bits = ((nsyn[i].bits&0x00ffffff)|((int)packbuf[j++])<<24);
                if (l&128) nsyn[i].horz = (signed char)packbuf[j++];
                if (l&256)  nsyn[i].extbits = (unsigned char)packbuf[j++];
                /*                if (l&256)  nsyn[i].extbits = ((nsyn[i].extbits&0xffffff00)|((int)packbuf[j++]));
                                if (l&512)  nsyn[i].extbits = ((nsyn[i].extbits&0xffff00ff)|((int)packbuf[j++])<<8);
                                if (l&1024) nsyn[i].extbits = ((nsyn[i].extbits&0xff00ffff)|((int)packbuf[j++])<<16);
                                if (l&2048) nsyn[i].extbits = ((nsyn[i].extbits&0x00ffffff)|((int)packbuf[j++])<<24); */

                if (nsyn[i].bits&(1<<26)) g_player[i].playerquitflag = 0;
                g_player[i].movefifoend++;
            }

            while (j != packbufleng)
            {
                for (i=connecthead;i>=0;i=connectpoint2[i])
                    if (i != myconnectindex)
                    {
                        g_player[i].syncval[g_player[i].syncvalhead&(MOVEFIFOSIZ-1)] = packbuf[j];
                        g_player[i].syncvalhead++;
                    }
                j++;
            }

            for (i=connecthead;i>=0;i=connectpoint2[i])
                if (i != myconnectindex)
                    for (j=movesperpacket-1;j>=1;j--)
                    {
                        copybufbyte(&nsyn[i],&inputfifo[g_player[i].movefifoend&(MOVEFIFOSIZ-1)][i],sizeof(input_t));
                        g_player[i].movefifoend++;
                    }

            movefifosendplc += movesperpacket;

            break;
        case 1:  //[1] (receive slave sync buffer)
            j = 3;
            k = packbuf[1] + (int)(packbuf[2]<<8);

            osyn = (input_t *)&inputfifo[(g_player[other].movefifoend-1)&(MOVEFIFOSIZ-1)][0];
            nsyn = (input_t *)&inputfifo[(g_player[other].movefifoend)&(MOVEFIFOSIZ-1)][0];

            copybufbyte(&osyn[other],&nsyn[other],sizeof(input_t));
            if (k&1)   nsyn[other].fvel = packbuf[j]+((short)packbuf[j+1]<<8), j += 2;
            if (k&2)   nsyn[other].svel = packbuf[j]+((short)packbuf[j+1]<<8), j += 2;
            if (k&4)   nsyn[other].avel = (signed char)packbuf[j++];
            if (k&8)   nsyn[other].bits = ((nsyn[other].bits&0xffffff00)|((int)packbuf[j++]));
            if (k&16)  nsyn[other].bits = ((nsyn[other].bits&0xffff00ff)|((int)packbuf[j++])<<8);
            if (k&32)  nsyn[other].bits = ((nsyn[other].bits&0xff00ffff)|((int)packbuf[j++])<<16);
            if (k&64)  nsyn[other].bits = ((nsyn[other].bits&0x00ffffff)|((int)packbuf[j++])<<24);
            if (k&128) nsyn[other].horz = (signed char)packbuf[j++];
            if (k&256) nsyn[other].extbits = (unsigned char)packbuf[j++];
            /*            if (k&256)  nsyn[other].extbits = ((nsyn[other].extbits&0xffffff00)|((int)packbuf[j++]));
                        if (k&512)  nsyn[other].extbits = ((nsyn[other].extbits&0xffff00ff)|((int)packbuf[j++])<<8);
                        if (k&1024) nsyn[other].extbits = ((nsyn[other].extbits&0xff00ffff)|((int)packbuf[j++])<<16);
                        if (k&2048) nsyn[other].extbits = ((nsyn[other].extbits&0x00ffffff)|((int)packbuf[j++])<<24); */
            g_player[other].movefifoend++;

            while (j != packbufleng)
            {
                g_player[other].syncval[g_player[other].syncvalhead&(MOVEFIFOSIZ-1)] = packbuf[j++];
                g_player[other].syncvalhead++;
            }

            for (i=movesperpacket-1;i>=1;i--)
            {
                copybufbyte(&nsyn[other],&inputfifo[g_player[other].movefifoend&(MOVEFIFOSIZ-1)][other],sizeof(input_t));
                g_player[other].movefifoend++;
            }

            break;

        case 16:
            g_player[other].movefifoend = movefifoplc = movefifosendplc = fakemovefifoplc = 0;
            g_player[other].syncvalhead = syncvaltottail = 0L;
        case 17:
            j = 1;

            if ((g_player[other].movefifoend&(TIMERUPDATESIZ-1)) == 0)
                if (other == connecthead)
                    for (i=connectpoint2[connecthead];i>=0;i=connectpoint2[i])
                    {
                        if (i == myconnectindex)
                            otherminlag = (int)((signed char)packbuf[j]);
                        j++;
                    }

            osyn = (input_t *)&inputfifo[(g_player[other].movefifoend-1)&(MOVEFIFOSIZ-1)][0];
            nsyn = (input_t *)&inputfifo[(g_player[other].movefifoend)&(MOVEFIFOSIZ-1)][0];

            copybufbyte(&osyn[other],&nsyn[other],sizeof(input_t));
            k = packbuf[j] + (int)(packbuf[j+1]<<8);
            j += 2;

            if (k&1)   nsyn[other].fvel = packbuf[j]+((short)packbuf[j+1]<<8), j += 2;
            if (k&2)   nsyn[other].svel = packbuf[j]+((short)packbuf[j+1]<<8), j += 2;
            if (k&4)   nsyn[other].avel = (signed char)packbuf[j++];
            if (k&8)   nsyn[other].bits = ((nsyn[other].bits&0xffffff00)|((int)packbuf[j++]));
            if (k&16)  nsyn[other].bits = ((nsyn[other].bits&0xffff00ff)|((int)packbuf[j++])<<8);
            if (k&32)  nsyn[other].bits = ((nsyn[other].bits&0xff00ffff)|((int)packbuf[j++])<<16);
            if (k&64)  nsyn[other].bits = ((nsyn[other].bits&0x00ffffff)|((int)packbuf[j++])<<24);
            if (k&128) nsyn[other].horz = (signed char)packbuf[j++];
            if (k&256) nsyn[other].extbits = (unsigned char)packbuf[j++];
            /*            if (k&256)  nsyn[other].extbits = ((nsyn[other].extbits&0xffffff00)|((int)packbuf[j++]));
                        if (k&512)  nsyn[other].extbits = ((nsyn[other].extbits&0xffff00ff)|((int)packbuf[j++])<<8);
                        if (k&1024) nsyn[other].extbits = ((nsyn[other].extbits&0xff00ffff)|((int)packbuf[j++])<<16);
                        if (k&2048) nsyn[other].extbits = ((nsyn[other].extbits&0x00ffffff)|((int)packbuf[j++])<<24); */
            g_player[other].movefifoend++;

            for (i=movesperpacket-1;i>=1;i--)
            {
                copybufbyte(&nsyn[other],&inputfifo[g_player[other].movefifoend&(MOVEFIFOSIZ-1)][other],sizeof(input_t));
                g_player[other].movefifoend++;
            }

            if (j > packbufleng)
                initprintf("INVALID GAME PACKET!!! (packet %d, %d too many bytes (%d %d))\n",packbuf[0],j-packbufleng,packbufleng,k);

            while (j < packbufleng)
            {
                g_player[other].syncval[g_player[other].syncvalhead&(MOVEFIFOSIZ-1)] = packbuf[j++];
                g_player[other].syncvalhead++;
            }

            break;
        case 127:
            break;

        case 250:
            if (g_player[other].playerreadyflag == 0)
                initprintf("Player %d is ready\n", other);
            g_player[other].playerreadyflag++;
            break;
        case 255:
            gameexit(" ");
            break;
        default:
            switch (packbuf[0])
            {
            case 4:
                //slaves in M/S mode only send to master
                if ((!networkmode) && (myconnectindex == connecthead))
                {
                    if (packbuf[1] == 255)
                    {
                        //Master re-transmits message to all others
                        for (i=connectpoint2[connecthead];i>=0;i=connectpoint2[i])
                            if (i != other)
                                sendpacket(i,packbuf,packbufleng);
                    }
                    else if (((int)packbuf[1]) != myconnectindex)
                    {
                        //Master re-transmits message not intended for master
                        sendpacket((int)packbuf[1],packbuf,packbufleng);
                        break;
                    }
                }

                Bstrcpy(recbuf,packbuf+2);
                recbuf[packbufleng-2] = 0;

                adduserquote(recbuf);
                sound(EXITMENUSOUND);

                pus = NUMPAGES;
                pub = NUMPAGES;

                break;

            case 5:
                //Slaves in M/S mode only send to master
                //Master re-transmits message to all others
                if ((!networkmode) && (myconnectindex == connecthead))
                    for (i=connectpoint2[connecthead];i>=0;i=connectpoint2[i])
                        if (i != other) sendpacket(i,packbuf,packbufleng);

                if (vote_map != -1 || vote_episode != -1 || voting != -1)
                    adduserquote("VOTE SUCCEEDED");

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

                for (i=connecthead;i>=0;i=connectpoint2[i])
                {
                    resetweapons(i);
                    resetinventory(i);
                }

                newgame(ud.volume_number,ud.level_number,ud.player_skill);
                ud.coop = ud.m_coop;

                if (enterlevel(MODE_GAME)) backtomenu();

                break;
            case 6:
                //slaves in M/S mode only send to master
                //Master re-transmits message to all others
                if ((!networkmode) && (myconnectindex == connecthead))
                    for (i=connectpoint2[connecthead];i>=0;i=connectpoint2[i])
                        if (i != other) sendpacket(i,packbuf,packbufleng);

                if (packbuf[2] != (char)atoi(s_builddate))
                    gameexit("\nYou cannot play Duke with different versions.");

                other = packbuf[1];

                for (i=3;packbuf[i];i++)
                    g_player[other].user_name[i-3] = packbuf[i];
                g_player[other].user_name[i-3] = 0;
                i++;

                g_player[other].ps->aim_mode = packbuf[i++];
                g_player[other].ps->auto_aim = packbuf[i++];
                g_player[other].ps->weaponswitch = packbuf[i++];
                g_player[other].ps->palookup = g_player[other].pcolor = packbuf[i++];
                g_player[other].pteam = packbuf[i++];

                /*            if(g_player[other].ps->team != j && sprite[g_player[other].ps->i].picnum == APLAYER)
                            {
                                hittype[g_player[other].ps->i].extra = 1000;
                                hittype[g_player[other].ps->i].picnum = APLAYERTOP;
                            } */

                break;
            case 10:
                //slaves in M/S mode only send to master
                //Master re-transmits message to all others
                if ((!networkmode) && (myconnectindex == connecthead))
                    for (i=connectpoint2[connecthead];i>=0;i=connectpoint2[i])
                        if (i != other) sendpacket(i,packbuf,packbufleng);

                other = packbuf[1];

                i = 2;

                j = i; //This used to be Duke packet #9... now concatenated with Duke packet #6
                for (;i-j<10;i++) g_player[other].wchoice[i-j] = packbuf[i];

                break;
            case 7:
                //slaves in M/S mode only send to master
                //Master re-transmits message to all others
                if ((!networkmode) && (myconnectindex == connecthead))
                    for (i=connectpoint2[connecthead];i>=0;i=connectpoint2[i])
                        if (i != other) sendpacket(i,packbuf,packbufleng);

                if (numlumps == 0) break;

                if (ud.config.SoundToggle == 0 || ud.lockout == 1 || ud.config.FXDevice < 0 || !(ud.config.VoiceToggle & 4))
                    break;
                rtsptr = (char *)RTS_GetSound(packbuf[1]-1);
                if (*rtsptr == 'C')
                    FX_PlayVOC3D(rtsptr,0,0,0,255,-packbuf[1]);
                else
                    FX_PlayWAV3D(rtsptr,0,0,0,255,-packbuf[1]);
                rtsplaying = 7;
                break;

            case 254:
                //slaves in M/S mode only send to master
                if (myconnectindex == connecthead)
                {
                    //Master re-transmits message to all others
                    for (i=connectpoint2[connecthead];i>=0;i=connectpoint2[i])
                        if (i != other)
                            sendpacket(i,packbuf,packbufleng);
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
                                adduserquote(buf);

                                if (numplayers < 2)
                                    sound(GENERIC_AMBIENCE17);

                                if(i == 0 && networkmode == 0) */
                gameexit("Game aborted from menu; disconnected.");
                //            }

                break;

            case 9:
                //slaves in M/S mode only send to master
                if (myconnectindex == connecthead)
                {
                    //Master re-transmits message to all others
                    for (i=connectpoint2[connecthead];i>=0;i=connectpoint2[i])
                        if (i != other)
                            sendpacket(i,packbuf,packbufleng);
                }

                Bstrcpy(boardfilename,packbuf+1);
                boardfilename[packbufleng-1] = 0;
                Bcorrectfilename(boardfilename,0);
                if (boardfilename[0] != 0)
                {
                    if ((i = kopen4loadfrommod(boardfilename,0)) < 0)
                    {
                        Bmemset(boardfilename,0,sizeof(boardfilename));
                        sendboardname();
                    }
                    else kclose(i);
                }

                if (ud.m_level_number == 7 && ud.m_volume_number == 0 && boardfilename[0] == 0)
                    ud.m_level_number = 0;

                break;

            case 18: // map vote

                if (myconnectindex == connecthead)
                {
                    //Master re-transmits message to all others
                    for (i=connectpoint2[connecthead];i>=0;i=connectpoint2[i])
                        if (i != other)
                            sendpacket(i,packbuf,packbufleng);
                }

                switch (packbuf[1])
                {
                case 0:
                    if (voting == myconnectindex && g_player[(unsigned char)packbuf[2]].gotvote == 0)
                    {
                        g_player[(unsigned char)packbuf[2]].gotvote = 1;
                        g_player[(unsigned char)packbuf[2]].vote = packbuf[3];
                        Bsprintf(tempbuf,"CONFIRMED VOTE FROM %s",g_player[(unsigned char)packbuf[2]].user_name);
                        adduserquote(tempbuf);
                    }
                    break;

                case 1: // call map vote
                    voting = packbuf[2];
                    vote_episode = packbuf[3];
                    vote_map = packbuf[4];
                    Bsprintf(tempbuf,"%s^00 HAS CALLED A VOTE TO CHANGE MAP TO %s (E%dL%d)",g_player[(unsigned char)packbuf[2]].user_name,map[(unsigned char)(packbuf[3]*MAXLEVELS + packbuf[4])].name,packbuf[3]+1,packbuf[4]+1);
                    adduserquote(tempbuf);
                    Bsprintf(tempbuf,"PRESS F1 TO ACCEPT, F2 TO DECLINE");
                    adduserquote(tempbuf);
                    for (i=MAXPLAYERS-1;i>=0;i--)
                    {
                        g_player[i].vote = 0;
                        g_player[i].gotvote = 0;
                    }
                    g_player[voting].gotvote = g_player[voting].vote = 1;
                    break;

                case 2: // cancel map vote
                    if (voting == packbuf[2])
                    {
                        voting = -1;
                        i = 0;
                        for (j=MAXPLAYERS-1;j>=0;j--)
                            i += g_player[j].gotvote;

                        if (i != numplayers)
                            Bsprintf(tempbuf,"%s^00 HAS CANCELED THE VOTE",g_player[(unsigned char)packbuf[2]].user_name);
                        else Bsprintf(tempbuf,"VOTE FAILED");
                        for (i=MAXPLAYERS-1;i>=0;i--)
                        {
                            g_player[i].vote = 0;
                            g_player[i].gotvote = 0;
                        }
                        adduserquote(tempbuf);
                    }
                    break;
                }
                break;

            case 126:
                //Slaves in M/S mode only send to master
                //Master re-transmits message to all others
                if ((!networkmode) && (myconnectindex == connecthead))
                    for (i=connectpoint2[connecthead];i>=0;i=connectpoint2[i])
                        if (i != other) sendpacket(i,packbuf,packbufleng);

                multiflag = 2;
                multiwhat = 0;
                multiwho = packbuf[2]; //other: need to send in m/s mode because of possible re-transmit
                multipos = packbuf[1];
                loadplayer(multipos);
                multiflag = 0;
                break;
            }
            break;
        }
    }
}

void faketimerhandler(void)
{
    int i, j, k;
    //    short who;
    input_t *osyn, *nsyn;

    if (qe == 0 && KB_KeyPressed(sc_LeftControl) && KB_KeyPressed(sc_LeftAlt) && KB_KeyPressed(sc_Delete))
    {
        qe = 1;
        gameexit("Quick Exit.");
    }

    sampletimer();
    AudioUpdate();
    if ((totalclock < ototalclock+TICSPERFRAME) || (ready2send == 0)) return;
    ototalclock += TICSPERFRAME;

    getpackets();
    if (getoutputcirclesize() >= 16) return;

    for (i=connecthead;i>=0;i=connectpoint2[i])
        if (i != myconnectindex)
            if (g_player[i].movefifoend < g_player[myconnectindex].movefifoend-200) return;

    getinput(myconnectindex);

    avgfvel += loc.fvel;
    avgsvel += loc.svel;
    avgavel += loc.avel;
    avghorz += loc.horz;
    avgbits |= loc.bits;
    avgextbits |= loc.extbits;
    if (g_player[myconnectindex].movefifoend&(movesperpacket-1))
    {
        copybufbyte(&inputfifo[(g_player[myconnectindex].movefifoend-1)&(MOVEFIFOSIZ-1)][myconnectindex],
                    &inputfifo[g_player[myconnectindex].movefifoend&(MOVEFIFOSIZ-1)][myconnectindex],sizeof(input_t));
        g_player[myconnectindex].movefifoend++;
        return;
    }
    nsyn = &inputfifo[g_player[myconnectindex].movefifoend&(MOVEFIFOSIZ-1)][myconnectindex];
    nsyn[0].fvel = avgfvel/movesperpacket;
    nsyn[0].svel = avgsvel/movesperpacket;
    nsyn[0].avel = avgavel/movesperpacket;
    nsyn[0].horz = avghorz/movesperpacket;
    nsyn[0].bits = avgbits;
    nsyn[0].extbits = avgextbits;
    avgfvel = avgsvel = avgavel = avghorz = avgbits = avgextbits = 0;
    g_player[myconnectindex].movefifoend++;

    if (numplayers < 2)
    {
        if (ud.multimode > 1) for (i=connecthead;i>=0;i=connectpoint2[i])
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

    for (i=connecthead;i>=0;i=connectpoint2[i])
        if (i != myconnectindex)
        {
            k = (g_player[myconnectindex].movefifoend-1)-g_player[i].movefifoend;
            g_player[i].myminlag = min(g_player[i].myminlag,k);
            mymaxlag = max(mymaxlag,k);
        }

    if (((g_player[myconnectindex].movefifoend-1)&(TIMERUPDATESIZ-1)) == 0)
    {
        i = mymaxlag-bufferjitter;
        mymaxlag = 0;
        if (i > 0) bufferjitter += ((3+i)>>2);
        else if (i < 0) bufferjitter -= ((1-i)>>2);
    }

    if (networkmode == 1)
    {
        packbuf[0] = 17;
        if ((g_player[myconnectindex].movefifoend-1) == 0) packbuf[0] = 16;
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
                g_player[connecthead].myminlag -= i;
                otherminlag += i;
            }

            if (myconnectindex == connecthead)
                for (i=connectpoint2[connecthead];i>=0;i=connectpoint2[i])
                    packbuf[j++] = min(max(g_player[i].myminlag,-128),127);

            for (i=connecthead;i>=0;i=connectpoint2[i])
                g_player[i].myminlag = 0x7fffffff;
        }

        osyn = (input_t *)&inputfifo[(g_player[myconnectindex].movefifoend-2)&(MOVEFIFOSIZ-1)][myconnectindex];
        nsyn = (input_t *)&inputfifo[(g_player[myconnectindex].movefifoend-1)&(MOVEFIFOSIZ-1)][myconnectindex];

        k = j;
        packbuf[j++] = 0;
        packbuf[j++] = 0;

        if (nsyn[0].fvel != osyn[0].fvel)
        {
            packbuf[j++] = (char)nsyn[0].fvel;
            packbuf[j++] = (char)(nsyn[0].fvel>>8);
            packbuf[k] |= 1;
        }
        if (nsyn[0].svel != osyn[0].svel)
        {
            packbuf[j++] = (char)nsyn[0].svel;
            packbuf[j++] = (char)(nsyn[0].svel>>8);
            packbuf[k] |= 2;
        }
        if (nsyn[0].avel != osyn[0].avel)
        {
            packbuf[j++] = (signed char)nsyn[0].avel;
            packbuf[k] |= 4;
        }
        if ((nsyn[0].bits^osyn[0].bits)&0x000000ff) packbuf[j++] = (nsyn[0].bits&255), packbuf[k] |= 8;
        if ((nsyn[0].bits^osyn[0].bits)&0x0000ff00) packbuf[j++] = ((nsyn[0].bits>>8)&255), packbuf[k] |= 16;
        if ((nsyn[0].bits^osyn[0].bits)&0x00ff0000) packbuf[j++] = ((nsyn[0].bits>>16)&255), packbuf[k] |= 32;
        if ((nsyn[0].bits^osyn[0].bits)&0xff000000) packbuf[j++] = ((nsyn[0].bits>>24)&255), packbuf[k] |= 64;
        if (nsyn[0].horz != osyn[0].horz)
        {
            packbuf[j++] = (char)nsyn[0].horz;
            packbuf[k] |= 128;
        }
//        k++;
        packbuf[++k] = 0;
        if (nsyn[0].extbits != osyn[0].extbits) packbuf[j++] = nsyn[0].extbits, packbuf[k] |= 1;
        /*        if ((nsyn[0].extbits^osyn[0].extbits)&0x000000ff) packbuf[j++] = (nsyn[0].extbits&255), packbuf[k] |= 1;
                if ((nsyn[0].extbits^osyn[0].extbits)&0x0000ff00) packbuf[j++] = ((nsyn[0].extbits>>8)&255), packbuf[k] |= 2;
                if ((nsyn[0].extbits^osyn[0].extbits)&0x00ff0000) packbuf[j++] = ((nsyn[0].extbits>>16)&255), packbuf[k] |= 4;
                if ((nsyn[0].extbits^osyn[0].extbits)&0xff000000) packbuf[j++] = ((nsyn[0].extbits>>24)&255), packbuf[k] |= 8; */

        while (g_player[myconnectindex].syncvalhead != syncvaltail)
        {
            packbuf[j++] = g_player[myconnectindex].syncval[syncvaltail&(MOVEFIFOSIZ-1)];
            syncvaltail++;
        }

        for (i=connecthead;i>=0;i=connectpoint2[i])
            if (i != myconnectindex)
                sendpacket(i,packbuf,j);

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
            g_player[connecthead].myminlag -= i;
            otherminlag += i;

            for (i=connecthead;i>=0;i=connectpoint2[i])
                g_player[i].myminlag = 0x7fffffff;
        }

        packbuf[0] = 1;
        packbuf[1] = 0;
        packbuf[2] = 0;
        j = 3;

        osyn = (input_t *)&inputfifo[(g_player[myconnectindex].movefifoend-2)&(MOVEFIFOSIZ-1)][myconnectindex];
        nsyn = (input_t *)&inputfifo[(g_player[myconnectindex].movefifoend-1)&(MOVEFIFOSIZ-1)][myconnectindex];

        if (nsyn[0].fvel != osyn[0].fvel)
        {
            packbuf[j++] = (char)nsyn[0].fvel;
            packbuf[j++] = (char)(nsyn[0].fvel>>8);
            packbuf[1] |= 1;
        }
        if (nsyn[0].svel != osyn[0].svel)
        {
            packbuf[j++] = (char)nsyn[0].svel;
            packbuf[j++] = (char)(nsyn[0].svel>>8);
            packbuf[1] |= 2;
        }
        if (nsyn[0].avel != osyn[0].avel)
        {
            packbuf[j++] = (signed char)nsyn[0].avel;
            packbuf[1] |= 4;
        }
        if ((nsyn[0].bits^osyn[0].bits)&0x000000ff) packbuf[j++] = (nsyn[0].bits&255), packbuf[1] |= 8;
        if ((nsyn[0].bits^osyn[0].bits)&0x0000ff00) packbuf[j++] = ((nsyn[0].bits>>8)&255), packbuf[1] |= 16;
        if ((nsyn[0].bits^osyn[0].bits)&0x00ff0000) packbuf[j++] = ((nsyn[0].bits>>16)&255), packbuf[1] |= 32;
        if ((nsyn[0].bits^osyn[0].bits)&0xff000000) packbuf[j++] = ((nsyn[0].bits>>24)&255), packbuf[1] |= 64;
        if (nsyn[0].horz != osyn[0].horz)
        {
            packbuf[j++] = (char)nsyn[0].horz;
            packbuf[1] |= 128;
        }
        packbuf[2] = 0;
        if (nsyn[0].extbits != osyn[0].extbits) packbuf[j++] = nsyn[0].extbits, packbuf[2] |= 1;
        /*        if ((nsyn[0].extbits^osyn[0].extbits)&0x000000ff) packbuf[j++] = (nsyn[0].extbits&255), packbuf[2] |= 1;
                if ((nsyn[0].extbits^osyn[0].extbits)&0x0000ff00) packbuf[j++] = ((nsyn[0].extbits>>8)&255), packbuf[2] |= 2;
                if ((nsyn[0].extbits^osyn[0].extbits)&0x00ff0000) packbuf[j++] = ((nsyn[0].extbits>>16)&255), packbuf[2] |= 4;
                if ((nsyn[0].extbits^osyn[0].extbits)&0xff000000) packbuf[j++] = ((nsyn[0].extbits>>24)&255), packbuf[2] |= 8; */

        while (g_player[myconnectindex].syncvalhead != syncvaltail)
        {
            packbuf[j++] = g_player[myconnectindex].syncval[syncvaltail&(MOVEFIFOSIZ-1)];
            syncvaltail++;
        }

        sendpacket(connecthead,packbuf,j);
        return;
    }

    //This allows allow packet resends
    for (i=connecthead;i>=0;i=connectpoint2[i])
        if (g_player[i].movefifoend <= movefifosendplc)
        {
            packbuf[0] = 127;
            for (i=connectpoint2[connecthead];i>=0;i=connectpoint2[i])
                sendpacket(i,packbuf,1);
            return;
        }

    while (1)  //Master
    {
        for (i=connecthead;i>=0;i=connectpoint2[i])
            if (g_player[i].playerquitflag && (g_player[i].movefifoend <= movefifosendplc)) return;

        osyn = (input_t *)&inputfifo[(movefifosendplc-1)&(MOVEFIFOSIZ-1)][0];
        nsyn = (input_t *)&inputfifo[(movefifosendplc)&(MOVEFIFOSIZ-1)][0];

        //MASTER -> SLAVE packet
        packbuf[0] = 0;
        j = 1;

        //Fix timers and buffer/jitter value
        if ((movefifosendplc&(TIMERUPDATESIZ-1)) == 0)
        {
            for (i=connectpoint2[connecthead];i>=0;i=connectpoint2[i])
                if (g_player[i].playerquitflag)
                    packbuf[j++] = min(max(g_player[i].myminlag,-128),127);

            for (i=connecthead;i>=0;i=connectpoint2[i])
                g_player[i].myminlag = 0x7fffffff;
        }

        k = j;
        for (i=connecthead;i>=0;i=connectpoint2[i])
            j += g_player[i].playerquitflag + g_player[i].playerquitflag;
        for (i=connecthead;i>=0;i=connectpoint2[i])
        {
            if (g_player[i].playerquitflag == 0) continue;

            packbuf[k] = 0;
            if (nsyn[i].fvel != osyn[i].fvel)
            {
                packbuf[j++] = (char)nsyn[i].fvel;
                packbuf[j++] = (char)(nsyn[i].fvel>>8);
                packbuf[k] |= 1;
            }
            if (nsyn[i].svel != osyn[i].svel)
            {
                packbuf[j++] = (char)nsyn[i].svel;
                packbuf[j++] = (char)(nsyn[i].svel>>8);
                packbuf[k] |= 2;
            }
            if (nsyn[i].avel != osyn[i].avel)
            {
                packbuf[j++] = (signed char)nsyn[i].avel;
                packbuf[k] |= 4;
            }
            if ((nsyn[i].bits^osyn[i].bits)&0x000000ff) packbuf[j++] = (nsyn[i].bits&255), packbuf[k] |= 8;
            if ((nsyn[i].bits^osyn[i].bits)&0x0000ff00) packbuf[j++] = ((nsyn[i].bits>>8)&255), packbuf[k] |= 16;
            if ((nsyn[i].bits^osyn[i].bits)&0x00ff0000) packbuf[j++] = ((nsyn[i].bits>>16)&255), packbuf[k] |= 32;
            if ((nsyn[i].bits^osyn[i].bits)&0xff000000) packbuf[j++] = ((nsyn[i].bits>>24)&255), packbuf[k] |= 64;
            if (nsyn[i].horz != osyn[i].horz)
            {
                packbuf[j++] = (char)nsyn[i].horz;
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

        while (g_player[myconnectindex].syncvalhead != syncvaltail)
        {
            packbuf[j++] = g_player[myconnectindex].syncval[syncvaltail&(MOVEFIFOSIZ-1)];
            syncvaltail++;
        }

        for (i=connectpoint2[connecthead];i>=0;i=connectpoint2[i])
            if (g_player[i].playerquitflag)
            {
                sendpacket(i,packbuf,j);
                if (nsyn[i].bits&(1<<26))
                    g_player[i].playerquitflag = 0;
            }

        movefifosendplc += movesperpacket;
    }
}

extern int cacnum;
typedef struct
{
    int *hand, leng;
    char *lock ;
}
cactype;
extern cactype cac[];

static void caches(void)
{
    short i,k;

    k = 0;
    for (i=cacnum-1;i>=0;i--)
        if ((*cac[i].lock) >= 200)
        {
            Bsprintf(tempbuf,"Locked- %d: Leng:%d, Lock:%d",i,cac[i].leng,*cac[i].lock);
            printext256(0L,k,31,-1,tempbuf,1);
            k += 6;
        }

    k += 6;

    for (i=10;i>=0;i--)
        if (lumplockbyte[i] >= 200)
        {
            Bsprintf(tempbuf,"RTS Locked %d:",i);
            printext256(0L,k,31,-1,tempbuf,1);
            k += 6;
        }
}

static void checksync(void)
{
    int i;

    for (i=connecthead;i>=0;i=connectpoint2[i])
        if (g_player[i].syncvalhead == syncvaltottail) break;
    if (i < 0)
    {
        syncstat = 0;
        do
        {
            for (i=connectpoint2[connecthead];i>=0;i=connectpoint2[i])
                if (g_player[i].syncval[syncvaltottail&(MOVEFIFOSIZ-1)] !=
                        g_player[connecthead].syncval[syncvaltottail&(MOVEFIFOSIZ-1)])
                    syncstat = 1;

            syncvaltottail++;
            for (i=connecthead;i>=0;i=connectpoint2[i])
                if (g_player[i].syncvalhead == syncvaltottail) break;
        }
        while (i < 0);
    }
    if (connectpoint2[connecthead] < 0) syncstat = 0;

    if (syncstat)
    {
        printext256(4L,130L,31,0,"Out Of Sync - Please restart game",0);
        printext256(4L,138L,31,0,"RUN DN3DHELP.EXE for information.",0);
    }
    #if 0
    if (syncstate)
    {
        printext256(4L,160L,31,0,"Missed Network packet!",0);
        printext256(4L,138L,31,0,"RUN DN3DHELP.EXE for information.",0);
    }
    #endif
}

void check_fta_sounds(int i)
{
    if (sprite[i].extra > 0)
        switch (dynamictostatic[PN])
        {
        case LIZTROOPONTOILET__STATIC:
        case LIZTROOPJUSTSIT__STATIC:
        case LIZTROOPSHOOT__STATIC:
        case LIZTROOPJETPACK__STATIC:
        case LIZTROOPDUCKING__STATIC:
        case LIZTROOPRUNNING__STATIC:
        case LIZTROOP__STATIC:
            spritesound(PRED_RECOG,i);
            break;
        case LIZMAN__STATIC:
        case LIZMANSPITTING__STATIC:
        case LIZMANFEEDING__STATIC:
        case LIZMANJUMP__STATIC:
            spritesound(CAPT_RECOG,i);
            break;
        case PIGCOP__STATIC:
        case PIGCOPDIVE__STATIC:
            spritesound(PIG_RECOG,i);
            break;
        case RECON__STATIC:
            spritesound(RECO_RECOG,i);
            break;
        case DRONE__STATIC:
            spritesound(DRON_RECOG,i);
            break;
        case COMMANDER__STATIC:
        case COMMANDERSTAYPUT__STATIC:
            spritesound(COMM_RECOG,i);
            break;
        case ORGANTIC__STATIC:
            spritesound(TURR_RECOG,i);
            break;
        case OCTABRAIN__STATIC:
        case OCTABRAINSTAYPUT__STATIC:
            spritesound(OCTA_RECOG,i);
            break;
        case BOSS1__STATIC:
            sound(BOS1_RECOG);
            break;
        case BOSS2__STATIC:
            if (sprite[i].pal == 1)
                sound(BOS2_RECOG);
            else sound(WHIPYOURASS);
            break;
        case BOSS3__STATIC:
            if (sprite[i].pal == 1)
                sound(BOS3_RECOG);
            else sound(RIPHEADNECK);
            break;
        case BOSS4__STATIC:
        case BOSS4STAYPUT__STATIC:
            if (sprite[i].pal == 1)
                sound(BOS4_RECOG);
            sound(BOSS4_FIRSTSEE);
            break;
        case GREENSLIME__STATIC:
            spritesound(SLIM_RECOG,i);
            break;
        }
}

int inventory(spritetype *s)
{
    switch (dynamictostatic[s->picnum])
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

inline int checkspriteflags(int iActor, int iType)
{
    if ((spriteflags[sprite[iActor].picnum]^hittype[iActor].flags) & iType) return 1;
    return 0;
}

inline int checkspriteflagsp(int iPicnum, int iType)
{
    if (spriteflags[iPicnum] & iType) return 1;
    return 0;
}

int badguypic(int pn)
{
    //this case can't be handled by the dynamictostatic system because it adds
    //stuff to the value from names.h so handling separately
    if ((pn >= GREENSLIME) && (pn <= GREENSLIME+7)) return 1;

    if (checkspriteflagsp(pn,SPRITE_FLAG_BADGUY)) return 1;

    if (actortype[pn]) return 1;

    switch (dynamictostatic[pn])
    {
    case SHARK__STATIC:
    case RECON__STATIC:
    case DRONE__STATIC:
    case LIZTROOPONTOILET__STATIC:
    case LIZTROOPJUSTSIT__STATIC:
    case LIZTROOPSTAYPUT__STATIC:
    case LIZTROOPSHOOT__STATIC:
    case LIZTROOPJETPACK__STATIC:
    case LIZTROOPDUCKING__STATIC:
    case LIZTROOPRUNNING__STATIC:
    case LIZTROOP__STATIC:
    case OCTABRAIN__STATIC:
    case COMMANDER__STATIC:
    case COMMANDERSTAYPUT__STATIC:
    case PIGCOP__STATIC:
    case EGG__STATIC:
    case PIGCOPSTAYPUT__STATIC:
    case PIGCOPDIVE__STATIC:
    case LIZMAN__STATIC:
    case LIZMANSPITTING__STATIC:
    case LIZMANFEEDING__STATIC:
    case LIZMANJUMP__STATIC:
    case ORGANTIC__STATIC:
    case BOSS1__STATIC:
    case BOSS2__STATIC:
    case BOSS3__STATIC:
    case BOSS4__STATIC:
        //case GREENSLIME:
        //case GREENSLIME+1:
        //case GREENSLIME+2:
        //case GREENSLIME+3:
        //case GREENSLIME+4:
        //case GREENSLIME+5:
        //case GREENSLIME+6:
        //case GREENSLIME+7:
    case RAT__STATIC:
    case ROTATEGUN__STATIC:
        return 1;
    }
    return 0;
}

inline int badguy(spritetype *s)
{
    return(badguypic(s->picnum));
}

void myos(int x, int y, int tilenum, int shade, int orientation)
{
    int p = sector[g_player[screenpeek].ps->cursectnum].floorpal, a = 0;

    if (orientation&4)
        a = 1024;

    rotatesprite((orientation&256)?x:(x<<16),(orientation&256)?y:(y<<16),65536L,a,tilenum,shade,p,2|orientation,windowx1,windowy1,windowx2,windowy2);
}

void myospal(int x, int y, int tilenum, int shade, int orientation, int p)
{
    int a = 0;

    if (orientation&4)
        a = 1024;

    rotatesprite((orientation&256)?x:(x<<16),(orientation&256)?y:(y<<16),65536L,a,tilenum,shade,p,2|orientation,windowx1,windowy1,windowx2,windowy2);
}

void myosx(int x, int y, int tilenum, int shade, int orientation)
{
    int p = sector[g_player[screenpeek].ps->cursectnum].floorpal, a = 0;

    if (orientation&4)
        a = 1024;

    rotatesprite((orientation&256)?x:(x<<16),(orientation&256)?y:(y<<16),32768L,a,tilenum,shade,p,2|orientation,windowx1,windowy1,windowx2,windowy2);
}

void myospalx(int x, int y, int tilenum, int shade, int orientation, int p)
{
    int a = 0;

    if (orientation&4)
        a = 1024;

    rotatesprite((orientation&256)?x:(x<<16),(orientation&256)?y:(y<<16),32768L,a,tilenum,shade,p,2|orientation,windowx1,windowy1,windowx2,windowy2);
}

static void invennum(int x,int y,char num1,char ha,char sbits)
{
    char dabuf[80] = {0};
    int shd = (x < 0);

    if (shd) x = -x;

    Bsprintf(dabuf,"%d",num1);
    if (num1 > 99)
    {
        if (shd && althud_shadows)
        {
            rotatesprite(sbarx(x-4+1),sbary(y+1),sbarsc(65536L),0,THREEBYFIVE+dabuf[0]-'0',ha,4,1|sbits,0,0,xdim-1,ydim-1);
            rotatesprite(sbarx(x+1),sbary(y+1),sbarsc(65536L),0,THREEBYFIVE+dabuf[1]-'0',ha,4,1|sbits,0,0,xdim-1,ydim-1);
            rotatesprite(sbarx(x+4+1),sbary(y+1),sbarsc(65536L),0,THREEBYFIVE+dabuf[2]-'0',ha,4,1|sbits,0,0,xdim-1,ydim-1);
        }
        rotatesprite(sbarx(x-4),sbary(y),sbarsc(65536L),0,THREEBYFIVE+dabuf[0]-'0',ha,0,sbits,0,0,xdim-1,ydim-1);
        rotatesprite(sbarx(x),sbary(y),sbarsc(65536L),0,THREEBYFIVE+dabuf[1]-'0',ha,0,sbits,0,0,xdim-1,ydim-1);
        rotatesprite(sbarx(x+4),sbary(y),sbarsc(65536L),0,THREEBYFIVE+dabuf[2]-'0',ha,0,sbits,0,0,xdim-1,ydim-1);
        return;
    }
    if (num1 > 9)
    {
        if (shd && althud_shadows)
        {
            rotatesprite(sbarx(x+1),sbary(y+1),sbarsc(65536L),0,THREEBYFIVE+dabuf[0]-'0',ha,4,1|sbits,0,0,xdim-1,ydim-1);
            rotatesprite(sbarx(x+4+1),sbary(y+1),sbarsc(65536L),0,THREEBYFIVE+dabuf[1]-'0',ha,4,1|sbits,0,0,xdim-1,ydim-1);
        }

        rotatesprite(sbarx(x),sbary(y),sbarsc(65536L),0,THREEBYFIVE+dabuf[0]-'0',ha,0,sbits,0,0,xdim-1,ydim-1);
        rotatesprite(sbarx(x+4),sbary(y),sbarsc(65536L),0,THREEBYFIVE+dabuf[1]-'0',ha,0,sbits,0,0,xdim-1,ydim-1);
        return;
    }
    rotatesprite(sbarx(x+4+1),sbary(y+1),sbarsc(65536L),0,THREEBYFIVE+dabuf[0]-'0',ha,4,sbits,0,0,xdim-1,ydim-1);
    rotatesprite(sbarx(x+4),sbary(y),sbarsc(65536L),0,THREEBYFIVE+dabuf[0]-'0',ha,0,sbits,0,0,xdim-1,ydim-1);
}

static void weaponnum(short ind,int x,int y,int num1, int num2,char ha)
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

static void weaponnum999(char ind,int x,int y,int num1, int num2,char ha)
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

static void weapon_amounts(player_struct *p,int x,int y,int u)
{
    int cw = p->curr_weapon;

    if (u&4)
    {
        if (u != -1) patchstatusbar(88,178,88+37,178+6); //original code: (96,178,96+12,178+6);
        weaponnum999(PISTOL_WEAPON,x,y,
                     p->ammo_amount[PISTOL_WEAPON],p->max_ammo_amount[PISTOL_WEAPON],
                     12-20*(cw == PISTOL_WEAPON));
    }
    if (u&8)
    {
        if (u != -1) patchstatusbar(88,184,88+37,184+6); //original code: (96,184,96+12,184+6);
        weaponnum999(SHOTGUN_WEAPON,x,y+6,
                     p->ammo_amount[SHOTGUN_WEAPON],p->max_ammo_amount[SHOTGUN_WEAPON],
                     (!p->gotweapon[SHOTGUN_WEAPON]*9)+12-18*
                     (cw == SHOTGUN_WEAPON));
    }
    if (u&16)
    {
        if (u != -1) patchstatusbar(88,190,88+37,190+6); //original code: (96,190,96+12,190+6);
        weaponnum999(CHAINGUN_WEAPON,x,y+12,
                     p->ammo_amount[CHAINGUN_WEAPON],p->max_ammo_amount[CHAINGUN_WEAPON],
                     (!p->gotweapon[CHAINGUN_WEAPON]*9)+12-18*
                     (cw == CHAINGUN_WEAPON));
    }
    if (u&32)
    {
        if (u != -1) patchstatusbar(127,178,127+29,178+6); //original code: (135,178,135+8,178+6);
        weaponnum(RPG_WEAPON,x+39,y,
                  p->ammo_amount[RPG_WEAPON],p->max_ammo_amount[RPG_WEAPON],
                  (!p->gotweapon[RPG_WEAPON]*9)+12-19*
                  (cw == RPG_WEAPON));
    }
    if (u&64)
    {
        if (u != -1) patchstatusbar(127,184,127+29,184+6); //original code: (135,184,135+8,184+6);
        weaponnum(HANDBOMB_WEAPON,x+39,y+6,
                  p->ammo_amount[HANDBOMB_WEAPON],p->max_ammo_amount[HANDBOMB_WEAPON],
                  (((!p->ammo_amount[HANDBOMB_WEAPON])|(!p->gotweapon[HANDBOMB_WEAPON]))*9)+12-19*
                  ((cw == HANDBOMB_WEAPON) || (cw == HANDREMOTE_WEAPON)));
    }
    if (u&128)
    {
        if (u != -1) patchstatusbar(127,190,127+29,190+6); //original code: (135,190,135+8,190+6);

        if (p->subweapon&(1<<GROW_WEAPON))
            weaponnum(SHRINKER_WEAPON,x+39,y+12,
                      p->ammo_amount[GROW_WEAPON],p->max_ammo_amount[GROW_WEAPON],
                      (!p->gotweapon[GROW_WEAPON]*9)+12-18*
                      (cw == GROW_WEAPON));
        else
            weaponnum(SHRINKER_WEAPON,x+39,y+12,
                      p->ammo_amount[SHRINKER_WEAPON],p->max_ammo_amount[SHRINKER_WEAPON],
                      (!p->gotweapon[SHRINKER_WEAPON]*9)+12-18*
                      (cw == SHRINKER_WEAPON));
    }
    if (u&256)
    {
        if (u != -1) patchstatusbar(158,178,162+29,178+6); //original code: (166,178,166+8,178+6);

        weaponnum(DEVISTATOR_WEAPON,x+70,y,
                  p->ammo_amount[DEVISTATOR_WEAPON],p->max_ammo_amount[DEVISTATOR_WEAPON],
                  (!p->gotweapon[DEVISTATOR_WEAPON]*9)+12-18*
                  (cw == DEVISTATOR_WEAPON));
    }
    if (u&512)
    {
        if (u != -1) patchstatusbar(158,184,162+29,184+6); //original code: (166,184,166+8,184+6);

        weaponnum(TRIPBOMB_WEAPON,x+70,y+6,
                  p->ammo_amount[TRIPBOMB_WEAPON],p->max_ammo_amount[TRIPBOMB_WEAPON],
                  (!p->gotweapon[TRIPBOMB_WEAPON]*9)+12-18*
                  (cw == TRIPBOMB_WEAPON));
    }

    if (u&65536L)
    {
        if (u != -1) patchstatusbar(158,190,162+29,190+6); //original code: (166,190,166+8,190+6);

        weaponnum(-1,x+70,y+12,
                  p->ammo_amount[FREEZE_WEAPON],p->max_ammo_amount[FREEZE_WEAPON],
                  (!p->gotweapon[FREEZE_WEAPON]*9)+12-18*
                  (cw == FREEZE_WEAPON));
    }
}

static void digitalnumber(int x,int y,int n,char s,char cs)
{
    int i, j = 0, k, p, c;
    char b[10];

    Bsnprintf(b,10,"%d",n);
    i = Bstrlen(b);

    for (k=i-1;k>=0;k--)
    {
        p = DIGITALNUM+*(b+k)-'0';
        j += tilesizx[p]+1;
    }
    c = x-(j>>1);

    j = 0;
    for (k=0;k<i;k++)
    {
        p = DIGITALNUM+*(b+k)-'0';
        rotatesprite(sbarx(c+j),sbary(y),sbarsc(65536L),0,p,s,0,cs,0,0,xdim-1,ydim-1);
        j += tilesizx[p]+1;
    }
}

void txdigitalnumberz(int starttile, int x,int y,int n,int s,int pal,int cs,int x1, int y1, int x2, int y2, int z)
{
    int i, j = 0, k, p, c;
    char b[10];
    int shift = (cs&256)?0:16;

    //ltoa(n,b,10);
    Bsnprintf(b,10,"%d",n);
    i = Bstrlen(b);

    for (k=i-1;k>=0;k--)
    {
        p = starttile+*(b+k)-'0';
        j += (tilesizx[p]+1)*z/65536;
    }
    if (cs&256) j<<=16;
    c = x-(j>>1);

    j = 0;
    for (k=0;k<i;k++)
    {
        p = starttile+*(b+k)-'0';
        rotatesprite((c+j)<<shift,y<<shift,z,0,p,s,pal,2|cs,x1,y1,x2,y2);
        /*        rotatesprite((c+j)<<16,y<<16,65536L,0,p,s,pal,cs,0,0,xdim-1,ydim-1);
                rotatesprite(x<<16,y<<16,32768L,a,tilenum,shade,p,2|orientation,windowx1,windowy1,windowx2,windowy2);*/
        j += ((tilesizx[p]+1)*z/65536)<<((cs&256)?16:0);
    }
}

static void altdigitalnumber(int x,int y,int n,char s,char cs)
{
    int i, j = 0, k, p, c;
    char b[10];
    int rev = (x < 0);
    int shd = (y < 0);

    if (rev) x = -x;
    if (shd) y = -y;

    Bsnprintf(b,10,"%d",n);
    i = Bstrlen(b);

    for (k=i-1;k>=0;k--)
    {
        p = althud_numbertile+*(b+k)-'0';
        j += tilesizx[p]+1;
    }
    c = x-(j>>1);

    if (rev)
    {
//        j = 0;
        for (k=0;k<i;k++)
        {
            p = althud_numbertile+*(b+k)-'0';
            if (shd && althud_shadows)
                rotatesprite(sbarxr(c+j-1),sbary(y+1),sbarsc(65536L),0,p,s,4,cs|1|32,0,0,xdim-1,ydim-1);
            rotatesprite(sbarxr(c+j),sbary(y),sbarsc(65536L),0,p,s,althud_numberpal,cs,0,0,xdim-1,ydim-1);
            j -= tilesizx[p]+1;
        }
        return;
    }
    j = 0;
    for (k=0;k<i;k++)
    {
        p = althud_numbertile+*(b+k)-'0';
        if (shd && althud_shadows)
            rotatesprite(sbarx(c+j+1),sbary(y+1),sbarsc(65536L),0,p,s,4,cs|1|32,0,0,xdim-1,ydim-1);
        rotatesprite(sbarx(c+j),sbary(y),sbarsc(65536L),0,p,s,althud_numberpal,cs,0,0,xdim-1,ydim-1);
        j += tilesizx[p]+1;
    }
}

static void displayinventory(player_struct *p)
{
    int n, j = 0, xoff = 0, y;

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

void displayfragbar(void)
{
    int i, j = 0;

    for (i=connecthead;i>=0;i=connectpoint2[i])
        if (i > j) j = i;

    rotatesprite(0,0,65600L,0,FRAGBAR,0,0,2+8+16+64,0,0,xdim-1,ydim-1);
    if (j >= 4) rotatesprite(319,(8)<<16,65600L,0,FRAGBAR,0,0,10+16+64,0,0,xdim-1,ydim-1);
    if (j >= 8) rotatesprite(319,(16)<<16,65600L,0,FRAGBAR,0,0,10+16+64,0,0,xdim-1,ydim-1);
    if (j >= 12) rotatesprite(319,(24)<<16,65600L,0,FRAGBAR,0,0,10+16+64,0,0,xdim-1,ydim-1);

    for (i=connecthead;i>=0;i=connectpoint2[i])
    {
        minitext(21+(73*(i&3)),2+((i&28)<<1),&g_player[i].user_name[0],/*sprite[g_player[i].ps->i].pal*/g_player[i].ps->palookup,2+8+16);
        Bsprintf(tempbuf,"%d",g_player[i].ps->frag-g_player[i].ps->fraggedself);
        minitext(17+50+(73*(i&3)),2+((i&28)<<1),tempbuf,/*sprite[g_player[i].ps->i].pal*/g_player[i].ps->palookup,2+8+16);
    }
}

#define SBY (200-tilesizy[BOTTOMSTATUSBAR])

static void coolgaugetext(int snum)
{
    player_struct *p = g_player[snum].ps;
    int i, j, o, ss = ud.screen_size, u;
    int permbit = 0;

    if (ss < 4) return;

    if (g_player[snum].ps->gm&MODE_MENU)
        if ((current_menu >= 400  && current_menu <= 405))
            return;

    if (getrendermode() >= 3) pus = NUMPAGES;   // JBF 20040101: always redraw in GL

    if (ud.multimode > 1 && (gametype_flags[ud.coop] & GAMETYPE_FLAG_FRAGBAR))
    {
        if (pus)
        {
            displayfragbar();
        }
        else
        {
            for (i=connecthead;i>=0;i=connectpoint2[i])
                if (g_player[i].ps->frag != sbar.frag[i])
                {
                    displayfragbar();
                    break;
                }

        }
        for (i=connecthead;i>=0;i=connectpoint2[i])
            if (i != myconnectindex)
                sbar.frag[i] = g_player[i].ps->frag;
    }

    if (ss == 4)   //DRAW MINI STATUS BAR:
    {
        if (ud.althud) // althud
        {
            static int ammo_sprites[MAX_WEAPONS];

            if (!ammo_sprites[0])
            {
                /* this looks stupid but it lets us initialize static memory to dynamic values
                   these values can be changed from the CONs with dynamic tile remapping
                   but we don't want to have to recreate the values in memory every time
                   the HUD is drawn */

                int asprites[MAX_WEAPONS] = { BOOTS, AMMO, SHOTGUNAMMO,
                                              BATTERYAMMO, RPGAMMO, HBOMBAMMO, CRYSTALAMMO, DEVISTATORAMMO,
                                              TRIPBOMBSPRITE, FREEZEAMMO+1, HBOMBAMMO, GROWAMMO
                                            };
                Bmemcpy(ammo_sprites,asprites,sizeof(ammo_sprites));
            }

//            rotatesprite(sbarx(5+1),sbary(200-25+1),sbarsc(49152L),0,SIXPAK,0,4,10+16+1+32,0,0,xdim-1,ydim-1);
//            rotatesprite(sbarx(5),sbary(200-25),sbarsc(49152L),0,SIXPAK,0,0,10+16,0,0,xdim-1,ydim-1);
            if (althud_shadows)
                rotatesprite(sbarx(2+1),sbary(200-21+1),sbarsc(49152L),0,COLA,0,4,10+16+1+32,0,0,xdim-1,ydim-1);
            rotatesprite(sbarx(2),sbary(200-21),sbarsc(49152L),0,COLA,0,0,10+16,0,0,xdim-1,ydim-1);

            if (sprite[p->i].pal == 1 && p->last_extra < 2)
                altdigitalnumber(40,-(200-22),1,-16,10+16);
            else if (!althud_flashing || p->last_extra > (p->max_player_health>>2) || totalclock&32)
            {
                int s = -8;
                if (althud_flashing && p->last_extra > p->max_player_health)
                    s += (sintable[(totalclock<<5)&2047]>>10);
                altdigitalnumber(40,-(200-22),p->last_extra,s,10+16);
            }

            if (althud_shadows)
                rotatesprite(sbarx(62+1),sbary(200-25+1),sbarsc(49152L),0,SHIELD,0,4,10+16+1+32,0,0,xdim-1,ydim-1);
            rotatesprite(sbarx(62),sbary(200-25),sbarsc(49152L),0,SHIELD,0,0,10+16,0,0,xdim-1,ydim-1);

            altdigitalnumber(105,-(200-22),p->shield_amount,-16,10+16);

            if (althud_shadows)
            {
                if (p->got_access&1) rotatesprite(sbarxr(39-1),sbary(200-43+1),sbarsc(32768),0,ACCESSCARD,0,4,10+16+32+1,0,0,xdim-1,ydim-1);
                if (p->got_access&4) rotatesprite(sbarxr(34-1),sbary(200-41+1),sbarsc(32768),0,ACCESSCARD,0,4,10+16+32+1,0,0,xdim-1,ydim-1);
                if (p->got_access&2) rotatesprite(sbarxr(29-1),sbary(200-39+1),sbarsc(32768),0,ACCESSCARD,0,4,10+16+32+1,0,0,xdim-1,ydim-1);
            }

            if (p->got_access&1) rotatesprite(sbarxr(39),sbary(200-43),sbarsc(32768),0,ACCESSCARD,0,0,10+16,0,0,xdim-1,ydim-1);
            if (p->got_access&4) rotatesprite(sbarxr(34),sbary(200-41),sbarsc(32768),0,ACCESSCARD,0,23,10+16,0,0,xdim-1,ydim-1);
            if (p->got_access&2) rotatesprite(sbarxr(29),sbary(200-39),sbarsc(32768),0,ACCESSCARD,0,21,10+16,0,0,xdim-1,ydim-1);

            i = 32768;
            if (p->curr_weapon == PISTOL_WEAPON) i = 16384;
            if (althud_shadows)
                rotatesprite(sbarxr(57-1),sbary(200-15+1),sbarsc(i),0,ammo_sprites[p->curr_weapon],0,4,2+1+32,0,0,xdim-1,ydim-1);
            rotatesprite(sbarxr(57),sbary(200-15),sbarsc(i),0,ammo_sprites[p->curr_weapon],0,0,2,0,0,xdim-1,ydim-1);

            if (p->curr_weapon == HANDREMOTE_WEAPON) i = HANDBOMB_WEAPON;
            else i = p->curr_weapon;
            if (p->curr_weapon != KNEE_WEAPON &&
                    (!althud_flashing || totalclock&32 || p->ammo_amount[i] > (p->max_ammo_amount[i]/10)))
                altdigitalnumber(-20,-(200-22),p->ammo_amount[i],-16,10+16);

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
                    if (althud_shadows)
                        rotatesprite(sbarx(231-o+1),sbary(200-21-2+1),sbarsc(65536L),0,i,0,4,10+16+permbit+1+32,0,0,xdim-1,ydim-1);
                    rotatesprite(sbarx(231-o),sbary(200-21-2),sbarsc(65536L),0,i,0,0,10+16+permbit,0,0,xdim-1,ydim-1);
                }

                if (althud_shadows)
                    minitext(292-30-o+1,190-3+1,"%",4,1+10+16+permbit + 256);
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
                invennum(-(284-30-o),200-6-3,(char)i,0,10+permbit);
                if (j > 0)
                {
                    if (althud_shadows)
                        minitext(288-30-o+1,180-3+1,"ON",4,1+10+16+permbit + 256);
                    minitext(288-30-o,180-3,"ON",0,10+16+permbit + 256);
                }
                else if ((unsigned int)j != 0x80000000)
                {
                    if (althud_shadows)
                        minitext(284-30-o+1,180-3+1,"OFF",4,1+10+16+permbit + 256);
                    minitext(284-30-o,180-3,"OFF",2,10+16+permbit + 256);
                }
                if (p->inven_icon >= 6)
                {
                    if (althud_shadows)
                        minitext(284-35-o+1,180-3+1,"AUTO",4,1+10+16+permbit + 256);
                    minitext(284-35-o,180-3,"AUTO",2,10+16+permbit + 256);
                }

            }
            return;
        }
        rotatesprite(sbarx(5),sbary(200-28),sbarsc(65536L),0,HEALTHBOX,0,21,10+16,0,0,xdim-1,ydim-1);
        if (p->inven_icon)
            rotatesprite(sbarx(69),sbary(200-30),sbarsc(65536L),0,INVENTORYBOX,0,21,10+16,0,0,xdim-1,ydim-1);

        if (sprite[p->i].pal == 1 && p->last_extra < 2)
            digitalnumber(20,200-17,1,-16,10+16);
        else digitalnumber(20,200-17,p->last_extra,-16,10+16);

        rotatesprite(sbarx(37),sbary(200-28),sbarsc(65536L),0,AMMOBOX,0,21,10+16,0,0,xdim-1,ydim-1);

        if (p->curr_weapon == HANDREMOTE_WEAPON) i = HANDBOMB_WEAPON;
        else i = p->curr_weapon;
        digitalnumber(53,200-17,p->ammo_amount[i],-16,10+16);

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
            invennum(284-30-o,200-6,(char)i,0,10+permbit);
            if (j > 0) minitext(288-30-o,180,"ON",0,10+16+permbit + 256);
            else if ((unsigned int)j != 0x80000000) minitext(284-30-o,180,"OFF",2,10+16+permbit + 256);
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
        int lAmount=GetGameVar("PLR_MORALE",-1, p->i, snum);
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

    for (i=1;i<MAX_WEAPONS;i++)
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
        patchstatusbar(0,0,320,200);
        if (ud.multimode > 1 && (gametype_flags[ud.coop] & GAMETYPE_FLAG_FRAGBAR))
            rotatesprite(sbarx(277+1),sbary(SBY+7-1),sbarsc(65536L),0,KILLSICON,0,0,10+16,0,0,xdim-1,ydim-1);
    }
    if (ud.multimode > 1 && (gametype_flags[ud.coop] & GAMETYPE_FLAG_FRAGBAR))
    {
        if (u&32768)
        {
            if (u != -1) patchstatusbar(276,SBY+17,299,SBY+17+10);
            digitalnumber(287,SBY+17,max(p->frag-p->fraggedself,0),-16,10+16);
        }
    }
    else
    {
        if (u&16384)
        {
            if (u != -1) patchstatusbar(275,SBY+18,299,SBY+18+12);
            if (p->got_access&4) rotatesprite(sbarx(275),sbary(SBY+16),sbarsc(65536L),0,ACCESS_ICON,0,23,10+16,0,0,xdim-1,ydim-1);
            if (p->got_access&2) rotatesprite(sbarx(288),sbary(SBY+16),sbarsc(65536L),0,ACCESS_ICON,0,21,10+16,0,0,xdim-1,ydim-1);
            if (p->got_access&1) rotatesprite(sbarx(281),sbary(SBY+23),sbarsc(65536L),0,ACCESS_ICON,0,0,10+16,0,0,xdim-1,ydim-1);
        }
    }
    if (u&(4+8+16+32+64+128+256+512+65536L)) weapon_amounts(p,96,SBY+16,u);

    if (u&1)
    {
        if (u != -1) patchstatusbar(20,SBY+17,43,SBY+17+11);
        if (sprite[p->i].pal == 1 && p->last_extra < 2)
            digitalnumber(32,SBY+17,1,-16,10+16);
        else digitalnumber(32,SBY+17,p->last_extra,-16,10+16);
    }
    if (u&2)
    {
        int lAmount=GetGameVar("PLR_MORALE",-1, p->i, snum);
        if (u != -1) patchstatusbar(52,SBY+17,75,SBY+17+11);
        if (lAmount == -1)
            digitalnumber(64,SBY+17,p->shield_amount,-16,10+16);
        else
            digitalnumber(64,SBY+17,lAmount,-16,10+16);
    }

    if (u&1024)
    {
        if (u != -1) patchstatusbar(196,SBY+17,219,SBY+17+11);
        if (p->curr_weapon != KNEE_WEAPON)
        {
            if (p->curr_weapon == HANDREMOTE_WEAPON) i = HANDBOMB_WEAPON;
            else i = p->curr_weapon;
            digitalnumber(230-22,SBY+17,p->ammo_amount[i],-16,10+16);
        }
    }

    if (u&(2048+4096+8192))
    {
        if (u != -1)
        {
            if (u&(2048+4096))
            {
                patchstatusbar(231,SBY+13,265,SBY+13+18);
            }
            else
            {
                patchstatusbar(250,SBY+24,261,SBY+24+6);
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
                else if ((unsigned int)j != 0x80000000) minitext(284-30-o,SBY+14,"OFF",2,10+16+permbit + 256);
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
                invennum(284-30-o,SBY+28,(char)i,0,10+permbit);
            }
        }
    }
}

#define COLOR_RED 248
#define COLOR_WHITE 31
#define LOW_FPS 30

static void ShowFrameRate(void)
{
    // adapted from ZDoom because I like it better than what we had
    // applicable ZDoom code available under GPL from csDoom
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
        if (ud.tickrate)
        {
            int chars = Bsprintf(tempbuf, "%2u ms (%3u fps)", howlong, LastCount);

            printext256(windowx2-(chars<<(3-x))+1,windowy1+2,0,-1,tempbuf,x);
            printext256(windowx2-(chars<<(3-x)),windowy1+1,(LastCount < LOW_FPS) ? COLOR_RED : COLOR_WHITE,-1,tempbuf,x);

            if (numplayers > 1 && (totalclock - lastpackettime) > 1)
            {
                for (howlong = (totalclock - lastpackettime);howlong>0 && howlong<(xdim>>2);howlong--)
                    printext256(4L*howlong,0,COLOR_WHITE,-1,".",0);
            }
        }

        if (LastSec < thisSec)
        {
            framerate = LastCount = FrameCount / (thisSec - LastSec);
            LastSec = thisSec;
            FrameCount = 0;
        }
        FrameCount++;
    }
    LastMS = ms;
}

static void ShowCoordinates(int snum)
{
    int y = 16;

    if ((gametype_flags[ud.coop] & GAMETYPE_FLAG_FRAGBAR))
    {
        if (ud.multimode > 4)
            y = 32;
        else if (ud.multimode > 1)
            y = 24;
    }
    sprintf(tempbuf,"XYZ= (%d,%d,%d)",g_player[snum].ps->posx,g_player[snum].ps->posy,g_player[snum].ps->posz);
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

static void operatefta(void)
{
    int i, j, k, l;

    k = 1;
    if (GTFLAGS(GAMETYPE_FLAG_FRAGBAR) && ud.screen_size > 0 && ud.multimode > 1)
    {
        j = 0;
        k += 8;
        for (i=connecthead;i>=0;i=connectpoint2[i])
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
    for (i=MAXUSERQUOTES-1;i>=0;i--)
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
        l = gametextlen(USERQUOTE_LEFTOFFSET,stripcolorcodes(tempbuf,user_quote[i]));
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

    if (fta_quotes[g_player[screenpeek].ps->ftq] == NULL)
    {
        OSD_Printf(OSD_ERROR "%s %d null quote %d\n",__FILE__,__LINE__,g_player[screenpeek].ps->ftq);
        return;
    }

    k = 0;

    if (g_player[screenpeek].ps->ftq == 115 || g_player[screenpeek].ps->ftq == 116 || g_player[screenpeek].ps->ftq == 117)
    {
        k = 140;//quotebot-8-4;
    }
    else if (GTFLAGS(GAMETYPE_FLAG_FRAGBAR) && ud.screen_size > 0 && ud.multimode > 1)
    {
        j = 0;
        k = 8;
        for (i=connecthead;i>=0;i=connectpoint2[i])
            if (i > j) j = i;

        if (j >= 4 && j <= 8) k += 8;
        else if (j > 8 && j <= 12) k += 16;
        else if (j > 12) k += 24;
    }

    j = g_player[screenpeek].ps->fta;
    if (!hud_glowingquotes)
    {
        if (j > 4) gametext(320>>1,k,fta_quotes[g_player[screenpeek].ps->ftq],0,2+8+16);
        else if (j > 2) gametext(320>>1,k,fta_quotes[g_player[screenpeek].ps->ftq],0,2+8+16+1);
        else gametext(320>>1,k,fta_quotes[g_player[screenpeek].ps->ftq],0,2+8+16+1+32);
        return;
    }
    if (j > 4) gametext(320>>1,k,fta_quotes[g_player[screenpeek].ps->ftq],(sintable[(totalclock<<5)&2047]>>11),2+8+16);
    else if (j > 2) gametext(320>>1,k,fta_quotes[g_player[screenpeek].ps->ftq],(sintable[(totalclock<<5)&2047]>>11),2+8+16+1);
    else gametext(320>>1,k,fta_quotes[g_player[screenpeek].ps->ftq],(sintable[(totalclock<<5)&2047]>>11),2+8+16+1+32);
}

void FTA(int q, player_struct *p)
{
    int cq = 0;

    if (q & MAXQUOTES)
    {
        cq = 1;
        q &= ~MAXQUOTES;
    }

    if (fta_quotes[q] == NULL)
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
                if (cq) OSD_Printf(OSDTEXT_BLUE "%s\n",fta_quotes[q]);
                else OSD_Printf("%s\n",fta_quotes[q]);
            }

        p->ftq = q;
        pub = NUMPAGES;
        pus = NUMPAGES;
    }
}

void fadepal(int r, int g, int b, int start, int end, int step)
{
    if (getrendermode() >= 3) return;
    if (step > 0) for (; start < end; start += step) palto(r,g,b,start);
    else for (; start >= end; start += step) palto(r,g,b,start);
}

static void showtwoscreens(void)
{
    int flags = GetGameVar("LOGO_FLAGS",255, -1, -1);

    MUSIC_StopSong();
    FX_StopAllSounds();

    if (!VOLUMEALL || flags & LOGO_FLAG_SHAREWARESCREENS)
    {
        setview(0,0,xdim-1,ydim-1);
        flushperms();
        //g_player[myconnectindex].ps->palette = palette;
        setgamepalette(g_player[myconnectindex].ps, palette, 1);    // JBF 20040308
        fadepal(0,0,0, 0,64,7);
        KB_FlushKeyboardQueue();
        rotatesprite(0,0,65536L,0,3291,0,0,2+8+16+64, 0,0,xdim-1,ydim-1);
        IFISSOFTMODE fadepal(0,0,0, 63,0,-7);
        else nextpage();
        while (!KB_KeyWaiting())
        {
            handleevents();
            getpackets();
        }

        fadepal(0,0,0, 0,64,7);
        KB_FlushKeyboardQueue();
        rotatesprite(0,0,65536L,0,3290,0,0,2+8+16+64, 0,0,xdim-1,ydim-1);
        IFISSOFTMODE fadepal(0,0,0, 63,0,-7);
        else nextpage();
        while (!KB_KeyWaiting())
        {
            handleevents();
            getpackets();
        }
    }

    if (flags & LOGO_FLAG_TENSCREEN)
    {
        setview(0,0,xdim-1,ydim-1);
        flushperms();
        //g_player[myconnectindex].ps->palette = palette;
        setgamepalette(g_player[myconnectindex].ps, palette, 1);    // JBF 20040308
        fadepal(0,0,0, 0,64,7);
        KB_FlushKeyboardQueue();
        rotatesprite(0,0,65536L,0,TENSCREEN,0,0,2+8+16+64, 0,0,xdim-1,ydim-1);
        IFISSOFTMODE fadepal(0,0,0, 63,0,-7);
        else nextpage();
        while (!KB_KeyWaiting() && totalclock < 2400)
        {
            handleevents();
            getpackets();
        }
    }
}

extern int qsetmode;
extern int doquicksave;

void gameexit(const char *t)
{
    if (*t != 0) g_player[myconnectindex].ps->palette = (char *) &palette[0];

    if (numplayers > 1)
        allowtimetocorrecterrorswhenquitting();

    uninitmultiplayers();

    if (ud.recstat == 1) closedemowrite();
    else if (ud.recstat == 2)
    {
        if (frecfilep) fclose(frecfilep);
    } // JBF: fixes crash on demo playback

    if (!qe && !cp)
    {
        if (playerswhenstarted > 1 && g_player[myconnectindex].ps->gm&MODE_GAME && GTFLAGS(GAMETYPE_FLAG_SCORESHEET) && *t == ' ')
        {
            dobonus(1);
            setgamemode(ud.config.ScreenMode,ud.config.ScreenWidth,ud.config.ScreenHeight,ud.config.ScreenBPP);
        }

        if (*t != 0 && *(t+1) != 'V' && *(t+1) != 'Y')
            showtwoscreens();
    }

    if (*t != 0) initprintf("%s\n",t);

    if (qsetmode == 200)
        Shutdown();

    if (*t != 0)
    {
        //setvmode(0x3);    // JBF
        //binscreen();
        //            if(*t == ' ' && *(t+1) == 0) *t = 0;
        //printf("\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n");
        if (!(t[0] == ' ' && t[1] == 0))
        {
            char titlebuf[256];
            Bsprintf(titlebuf,HEAD2 " %s",s_builddate);
            wm_msgbox(titlebuf, (char *)t);
        }
    }

    uninitgroupfile();

    //unlink("duke3d.tmp");

    exit(0);
}

char inputloc = 0;

static int strget_(int small,int x,int y,char *t,int dalen,int c)
{
    char ch;
    int i;

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
        for (ii=0;ii<inputloc;ii++)
            b[(unsigned char)ii] = '*';
        b[(unsigned char)inputloc] = 0;
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

    i = gametextlen(USERQUOTE_LEFTOFFSET,stripcolorcodes(tempbuf,t));
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

inline int strget(int x,int y,char *t,int dalen,int c)
{
    return(strget_(0,x,y,t,dalen,c));
}

inline int mpstrget(int x,int y,char *t,int dalen,int c)
{
    return(strget_(1,x,y,t,dalen,c));
}

static void typemode(void)
{
    short ch, hitstate, i, j, l;

    if (g_player[myconnectindex].ps->gm&MODE_SENDTOWHOM)
    {
        if (sendmessagecommand != -1 || ud.multimode < 3 || movesperpacket == 4)
        {
            tempbuf[0] = 4;
            tempbuf[2] = 0;
            recbuf[0]  = 0;

            if (ud.multimode < 3)
                sendmessagecommand = 2;

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

            if (sendmessagecommand >= ud.multimode || movesperpacket == 4)
            {
                tempbuf[1] = 255;
                for (ch=connecthead;ch >= 0;ch=connectpoint2[ch])
                {
                    if (ch != myconnectindex) sendpacket(ch,tempbuf,j+2);
                    if ((!networkmode) && (myconnectindex != connecthead)) break; //slaves in M/S mode only send to master
                }
                adduserquote(recbuf);
                quotebot += 8;
                l = gametextlen(USERQUOTE_LEFTOFFSET,stripcolorcodes(tempbuf,recbuf));
                while (l > (ud.config.ScreenWidth - USERQUOTE_RIGHTOFFSET))
                {
                    l -= (ud.config.ScreenWidth - USERQUOTE_RIGHTOFFSET);
                    quotebot += 8;
                }
                quotebotgoal = quotebot;
            }
            else if (sendmessagecommand >= 0)
            {
                tempbuf[1] = (char)sendmessagecommand;
                if ((!networkmode) && (myconnectindex != connecthead))
                    sendmessagecommand = connecthead;
                sendpacket(sendmessagecommand,tempbuf,j+2);
            }

            sendmessagecommand = -1;
            g_player[myconnectindex].ps->gm &= ~(MODE_TYPE|MODE_SENDTOWHOM);
        }
        else if (sendmessagecommand == -1)
        {
            j = 50;
            gametext(320>>1,j,"SEND MESSAGE TO...",0,2+8+16);
            j += 8;
            for (i=connecthead;i>=0;i=connectpoint2[i])
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
                    sendmessagecommand = ud.multimode;
                else if (i >= '1' || i <= (ud.multimode + '1'))
                    sendmessagecommand = i - '1';
                else
                {
                    sendmessagecommand = ud.multimode;
                    if (i == 27)
                    {
                        g_player[myconnectindex].ps->gm &= ~(MODE_TYPE|MODE_SENDTOWHOM);
                        sendmessagecommand = -1;
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
        hitstate = mpstrget(320>>1,j,typebuf,120,1);

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
                if (SHIFTS_IS_PRESSED) sendmessagecommand = -1;
                else sendmessagecommand = ud.multimode;
            }
            g_player[myconnectindex].ps->gm |= MODE_SENDTOWHOM;
        }
        else if (hitstate == -1)
            g_player[myconnectindex].ps->gm &= ~(MODE_TYPE|MODE_SENDTOWHOM);
        else pub = NUMPAGES;
    }
}

static void moveclouds(void)
{
    if (totalclock > cloudtotalclock || totalclock < (cloudtotalclock-7))
    {
        int i;

        cloudtotalclock = totalclock+6;

        for (i=numclouds-1;i>=0;i--)
        {
            cloudx[i] += (sintable[(g_player[screenpeek].ps->ang+512)&2047]>>9);
            cloudy[i] += (sintable[g_player[screenpeek].ps->ang&2047]>>9);

            sector[clouds[i]].ceilingxpanning = cloudx[i]>>6;
            sector[clouds[i]].ceilingypanning = cloudy[i]>>6;
        }
    }
}

static void drawoverheadmap(int cposx, int cposy, int czoom, short cang)
{
    int i, j, k, l, x1, y1, x2=0, y2=0, x3, y3, x4, y4, ox, oy, xoff, yoff;
    int dax, day, cosang, sinang, xspan, yspan, sprx, spry;
    int xrepeat, yrepeat, z1, z2, startwall, endwall, tilenum, daang;
    int xvect, yvect, xvect2, yvect2;
    short p;
    char col;
    walltype *wal, *wal2;
    spritetype *spr;

    xvect = sintable[(-cang)&2047] * czoom;
    yvect = sintable[(1536-cang)&2047] * czoom;
    xvect2 = mulscale16(xvect,yxaspect);
    yvect2 = mulscale16(yvect,yxaspect);

    //Draw red lines
    for (i=numsectors-1;i>=0;i--)
    {
        if (!(show2dsector[i>>3]&(1<<(i&7)))) continue;

        startwall = sector[i].wallptr;
        endwall = sector[i].wallptr + sector[i].wallnum;

        z1 = sector[i].ceilingz;
        z2 = sector[i].floorz;

        for (j=startwall,wal=&wall[startwall];j<endwall;j++,wal++)
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
    for (i=numsectors-1;i>=0;i--)
    {
        if (!(show2dsector[i>>3]&(1<<(i&7)))) continue;
        for (j=headspritesect[i];j>=0;j=nextspritesect[j])
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
                        xoff = (int)((signed char)((picanm[tilenum]>>8)&255))+((int)spr->xoffset);
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
                    xoff = (int)((signed char)((picanm[tilenum]>>8)&255))+((int)spr->xoffset);
                    yoff = (int)((signed char)((picanm[tilenum]>>16)&255))+((int)spr->yoffset);
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
    for (i=numsectors-1;i>=0;i--)
    {
        if (!(show2dsector[i>>3]&(1<<(i&7)))) continue;

        startwall = sector[i].wallptr;
        endwall = sector[i].wallptr + sector[i].wallnum;

        k = -1;
        for (j=startwall,wal=&wall[startwall];j<endwall;j++,wal++)
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

    for (p=connecthead;p >= 0;p=connectpoint2[p])
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

        if (p == screenpeek || GTFLAGS(GAMETYPE_FLAG_OTHERPLAYERSINMAP))
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

extern int getclosestcol(int r, int g, int b);
palette_t crosshair_colors = { 255, 255, 255, 0 };
palette_t default_crosshair_colors = { 0, 0, 0, 0 };
int crosshair_sum;

void GetCrosshairColor(void)
{
    if (default_crosshair_colors.f == 0)
    {
        // use the brightest color in the original 8-bit tile
        int bri = 0, j = 0, i;
        int ii;
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
                i = curpalette[(int)*ptr].r+curpalette[(int)*ptr].g+curpalette[(int)*ptr].b;
                if (i > j) { j = i; bri = *ptr; }
            }
            ptr++;
            ii--;
        }
        while (ii > 0);

        default_crosshair_colors.r = crosshair_colors.r = curpalette[bri].r;
        default_crosshair_colors.g = crosshair_colors.g = curpalette[bri].g;
        default_crosshair_colors.b = crosshair_colors.b = curpalette[bri].b;
        default_crosshair_colors.f = 1; // this flag signifies that the color has been detected
    }
}

void SetCrosshairColor(int r, int g, int b)
{
    char *ptr = (char *)waloff[CROSSHAIR];
    int i, ii;

    if (default_crosshair_colors.f == 0 || crosshair_sum == r+(g<<1)+(b<<2)) return;

    crosshair_sum = r+(g<<1)+(b<<2);
    crosshair_colors.r = r;
    crosshair_colors.g = g;
    crosshair_colors.b = b;

    if (waloff[CROSSHAIR] == 0)
    {
        loadtile(CROSSHAIR);
        ptr = (char *)waloff[CROSSHAIR];
    }

    ii = tilesizx[CROSSHAIR]*tilesizy[CROSSHAIR];
    if (ii <= 0) return;

    if (getrendermode() < 3)
        i = getclosestcol(crosshair_colors.r>>2, crosshair_colors.g>>2, crosshair_colors.b>>2);
    else i = getclosestcol(63, 63, 63); // use white in GL so we can tint it to the right color

    do
    {
        if (*ptr != 255)
            *ptr = i;
        ptr++;
        ii--;
    }
    while (ii > 0);

    for (i = 255; i >= 0; i--)
        tempbuf[i] = i;

    makepalookup(CROSSHAIR_PAL,tempbuf,crosshair_colors.r>>2, crosshair_colors.g>>2, crosshair_colors.b>>2,1);

#if defined(USE_OPENGL) && defined(POLYMOST)
    hictinting[CROSSHAIR_PAL].r = crosshair_colors.r;
    hictinting[CROSSHAIR_PAL].g = crosshair_colors.g;
    hictinting[CROSSHAIR_PAL].b = crosshair_colors.b;
    hictinting[CROSSHAIR_PAL].f = 9;
#endif
    invalidatetile(CROSSHAIR, -1, -1);
}

void palto(int r,int g,int b,int e)
{
    int tc;
    /*
        for(i=0;i<768;i+=3)
        {
            temparray[i  ] =
                g_player[myconnectindex].ps->palette[i+0]+((((int)r-(int)g_player[myconnectindex].ps->palette[i+0])*(int)(e&127))>>6);
            temparray[i+1] =
                g_player[myconnectindex].ps->palette[i+1]+((((int)g-(int)g_player[myconnectindex].ps->palette[i+1])*(int)(e&127))>>6);
            temparray[i+2] =
                g_player[myconnectindex].ps->palette[i+2]+((((int)b-(int)g_player[myconnectindex].ps->palette[i+2])*(int)(e&127))>>6);
        }
    */

    //setbrightness(ud.brightness>>2,temparray);
    setpalettefade(r,g,b,e&127);
    if (getrendermode() >= 3) pus = pub = NUMPAGES; // JBF 20040110: redraw the status bar next time
    if ((e&128) == 0)
    {
        nextpage();
        for (tc = totalclock; totalclock < tc + 4; handleevents(), getpackets());
    }
}

void displayrest(int smoothratio)
{
    int a, i, j;
    char fader=0,fadeg=0,fadeb=0,fadef=0,tintr=0,tintg=0,tintb=0,tintf=0,dotint=0;

    player_struct *pp = g_player[screenpeek].ps;
    walltype *wal;
    int cposx,cposy,cang;

#if defined(USE_OPENGL) && defined(POLYMOST)
    // this takes care of fullscreen tint for OpenGL
    if (getrendermode() >= 3)
    {
#if 0
        if (pp->palette == waterpal) tintr=0,tintg=0,tintb=63,tintf=8;
        else if (pp->palette == slimepal) tintr=20,tintg=63,tintb=20,tintf=8;
#else
        if (pp->palette == waterpal)
        {
            if (hictinting[MAXPALOOKUPS-2].r == 255 && hictinting[MAXPALOOKUPS-2].g == 255 && hictinting[MAXPALOOKUPS-2].b == 255)
            {
                hictinting[MAXPALOOKUPS-1].r = 192;
                hictinting[MAXPALOOKUPS-1].g = 192;
                hictinting[MAXPALOOKUPS-1].b = 255;
            }
            else Bmemcpy(&hictinting[MAXPALOOKUPS-1],&hictinting[MAXPALOOKUPS-2],sizeof(hictinting[0]));
        }
        else if (pp->palette == slimepal)
        {
            if (hictinting[MAXPALOOKUPS-3].r == 255 && hictinting[MAXPALOOKUPS-3].g == 255 && hictinting[MAXPALOOKUPS-3].b == 255)
            {
                hictinting[MAXPALOOKUPS-1].r = 208;
                hictinting[MAXPALOOKUPS-1].g = 255;
                hictinting[MAXPALOOKUPS-1].b = 192;
            }
            else Bmemcpy(&hictinting[MAXPALOOKUPS-1],&hictinting[MAXPALOOKUPS-3],sizeof(hictinting[0]));
        }
        else
        {
            hictinting[MAXPALOOKUPS-1].r = 255;
            hictinting[MAXPALOOKUPS-1].g = 255;
            hictinting[MAXPALOOKUPS-1].b = 255;
        }
#endif
    }
#endif /* USE_OPENGL && POLYMOST */
    // this does pain tinting etc from the CON
    if (pp->pals_time >= 0 && pp->loogcnt == 0) // JBF 20040101: pals_time > 0 now >= 0
    {
        fader = pp->pals[0];
        fadeg = pp->pals[1];
        fadeb = pp->pals[2];
        fadef = pp->pals_time;
        restorepalette = 1;     // JBF 20040101
        dotint = 1;
    }
    // reset a normal palette
    else if (restorepalette)
    {
        //setbrightness(ud.brightness>>2,&pp->palette[0],0);
        setgamepalette(pp,pp->palette,2);
        restorepalette = 0;
    }
    // loogies courtesy of being snotted on
    else if (pp->loogcnt > 0)
    {
        //palto(0,64,0,(pp->loogcnt>>1)+128);
        fader = 0;
        fadeg = 64;
        fadeb = 0;
        fadef = pp->loogcnt>>1;
        dotint = 1;
    }
    if (fadef > tintf)
    {
        tintr = fader;
        tintg = fadeg;
        tintb = fadeb;
        tintf = fadef;
    }

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
            vscrn();
        }
        if (tintf > 0 || dotint) palto(tintr,tintg,tintb,tintf|128);
        return;
    }

    i = pp->cursectnum;
    if (i > -1)
    {
        show2dsector[i>>3] |= (1<<(i&7));
        wal = &wall[sector[i].wallptr];
        for (j=sector[i].wallnum;j>0;j--,wal++)
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
                cameratext(pp->newowner);
            else
            {
                displayweapon(screenpeek);
                if (pp->over_shoulder_on == 0)
                    displaymasks(screenpeek);
            }
            moveclouds();
        }

        if (ud.overhead_on > 0)
        {
            smoothratio = min(max(smoothratio,0),65536);
            dointerpolations(smoothratio);
            if (ud.scrollmode == 0)
            {
                if (pp->newowner == -1 && !ud.pause_on)
                {
                    if (screenpeek == myconnectindex && numplayers > 1)
                    {
                        cposx = omyx+mulscale16((int)(myx-omyx),smoothratio);
                        cposy = omyy+mulscale16((int)(myy-omyy),smoothratio);
                        cang = omyang+mulscale16((int)(((myang+1024-omyang)&2047)-1024),smoothratio);
                    }
                    else
                    {
                        cposx = pp->oposx+mulscale16((int)(pp->posx-pp->oposx),smoothratio);
                        cposy = pp->oposy+mulscale16((int)(pp->posy-pp->oposy),smoothratio);
                        cang = pp->oang+mulscale16((int)(((pp->ang+1024-pp->oang)&2047)-1024),smoothratio);
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
            drawoverheadmap(cposx,cposy,pp->zoom,cang);

            restoreinterpolations();

            if (ud.overhead_on == 2)
            {
                if (ud.screen_size > 0) a = 147;
                else a = 179;
                minitext(5,a+6,volume_names[ud.volume_number],0,2+8+16);
                minitext(5,a+6+6,map[ud.volume_number*MAXLEVELS + ud.level_number].name,0,2+8+16);
            }
        }
    }

    if (pp->invdisptime > 0) displayinventory(pp);

    SetGameVarID(g_iReturnVarID,0,g_player[screenpeek].ps->i,screenpeek);
    OnEvent(EVENT_DISPLAYSBAR, g_player[screenpeek].ps->i, screenpeek, -1);
    if (GetGameVarID(g_iReturnVarID,g_player[screenpeek].ps->i,screenpeek) == 0)
        coolgaugetext(screenpeek);

    operatefta();

    if (ud.show_level_text && hud_showmapname && leveltexttime > 1)
    {
        int bits = 10+16;

        if (leveltexttime > 4)
            bits = bits;
        else if (leveltexttime > 2)
            bits |= 1;
        else bits |= 1+32;
        if (map[(ud.volume_number*MAXLEVELS) + ud.level_number].name != NULL)
        {
            if (currentboardfilename[0] != 0 && ud.volume_number == 0 && ud.level_number == 7)
                menutext_(160,75,-leveltexttime+22/*(sintable[(totalclock<<5)&2047]>>11)*/,0,currentboardfilename,bits);
            else menutext_(160,75,-leveltexttime+22/*(sintable[(totalclock<<5)&2047]>>11)*/,0,map[(ud.volume_number*MAXLEVELS) + ud.level_number].name,bits);
        }
    }

    if (KB_KeyPressed(sc_Escape) && ud.overhead_on == 0
            && ud.show_help == 0
            && g_player[myconnectindex].ps->newowner == -1)
    {
        if ((g_player[myconnectindex].ps->gm&MODE_MENU) == MODE_MENU && current_menu < 51)
        {
            KB_ClearKeyDown(sc_Escape);
            g_player[myconnectindex].ps->gm &= ~MODE_MENU;
            if (ud.multimode < 2 && ud.recstat != 2)
            {
                ready2send = 1;
                totalclock = ototalclock;
                cameraclock = totalclock;
                cameradist = 65536L;
            }
            walock[TILE_SAVESHOT] = 199;
            vscrn();
        }
        else if ((g_player[myconnectindex].ps->gm&MODE_MENU) != MODE_MENU &&
                 g_player[myconnectindex].ps->newowner == -1 &&
                 (g_player[myconnectindex].ps->gm&MODE_TYPE) != MODE_TYPE)
        {
            KB_ClearKeyDown(sc_Escape);
            FX_StopAllSounds();
            clearsoundlocks();

            intomenusounds();

            g_player[myconnectindex].ps->gm |= MODE_MENU;

            if (ud.multimode < 2 && ud.recstat != 2) ready2send = 0;

            if (g_player[myconnectindex].ps->gm&MODE_GAME) cmenu(50);
            else cmenu(0);
            screenpeek = myconnectindex;
        }
    }

    OnEvent(EVENT_DISPLAYREST, g_player[screenpeek].ps->i, screenpeek, -1);

    if (g_player[myconnectindex].ps->newowner == -1 && ud.overhead_on == 0 && ud.crosshair && ud.camerasprite == -1)
    {
        SetGameVarID(g_iReturnVarID,0,g_player[screenpeek].ps->i,screenpeek);
        OnEvent(EVENT_DISPLAYCROSSHAIR, g_player[screenpeek].ps->i, screenpeek, -1);
        if (GetGameVarID(g_iReturnVarID,g_player[screenpeek].ps->i,screenpeek) == 0)
            rotatesprite((160L-(g_player[myconnectindex].ps->look_ang>>1))<<16,100L<<16,scale(65536,ud.crosshairscale,100),0,CROSSHAIR,0,CROSSHAIR_PAL,2+1,windowx1,windowy1,windowx2,windowy2);
    }
#if 0
    if (gametype_flags[ud.coop] & GAMETYPE_FLAG_TDM)
    {
        for (i=0;i<ud.multimode;i++)
        {
            if (g_player[i].ps->team == g_player[myconnectindex].ps->team)
            {
                j = min(max((getincangle(getangle(g_player[i].ps->posx-g_player[myconnectindex].ps->posx,g_player[i].ps->posy-g_player[myconnectindex].ps->posy),g_player[myconnectindex].ps->ang))>>1,-160),160);
                rotatesprite((160-j)<<16,100L<<16,65536L,0,DUKEICON,0,0,2+1,windowx1,windowy1,windowx2,windowy2);
            }
        }
    }
#endif

    if (ud.pause_on==1 && (g_player[myconnectindex].ps->gm&MODE_MENU) == 0)
        menutext(160,100,0,0,"GAME PAUSED");

    if (ud.coords)
        ShowCoordinates(screenpeek);

#if defined(POLYMOST) && defined(USE_OPENGL)
    {
        extern int mdpause;

        mdpause = 0;
        if (ud.pause_on || (g_player[myconnectindex].ps->gm&MODE_MENU && numplayers < 2))
            mdpause = 1;
    }
#endif

    ShowFrameRate();

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
                 (g_player[myconnectindex].ps->player_par/(26*60)),
                 (g_player[myconnectindex].ps->player_par/26)%60,
                 ((g_player[myconnectindex].ps->player_par%26)*38)/10
                );
        gametext_z(13,STARTALPHANUM, j,scale(200-i,ud.config.ScreenHeight,200)-textsc(21),tempbuf,0,10,26,0, 0, xdim-1, ydim-1, 65536);

        if (ud.player_skill > 3 || (ud.multimode > 1 && !GTFLAGS(GAMETYPE_FLAG_PLAYERSFRIENDLY)))
            Bsprintf(tempbuf,"K:^15%d",(ud.multimode>1 &&!GTFLAGS(GAMETYPE_FLAG_PLAYERSFRIENDLY))?g_player[myconnectindex].ps->frag-g_player[myconnectindex].ps->fraggedself:g_player[myconnectindex].ps->actors_killed);
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
        typemode();
    else
        menus();

    if (tintf > 0 || dotint) palto(tintr,tintg,tintb,tintf|128);
}

static void view(player_struct *pp, int *vx, int *vy,int *vz,short *vsectnum, int ang, int horiz)
{
    spritetype *sp = &sprite[pp->i];
    int i, hx, hy, hitx, hity, hitz;
    int nx = (sintable[(ang+1536)&2047]>>4);
    int ny = (sintable[(ang+1024)&2047]>>4);
    int nz = (horiz-100)*128;
    short hitsect, hitwall, hitsprite, daang;
    short bakcstat = sp->cstat;

    sp->cstat &= (short)~0x101;

    updatesectorz(*vx,*vy,*vz,vsectnum);
    hitscan(*vx,*vy,*vz,*vsectnum,nx,ny,nz,&hitsect,&hitwall,&hitsprite,&hitx,&hity,&hitz,CLIPMASK1);

    if (*vsectnum < 0)
    {
        sp->cstat = bakcstat;
        return;
    }

    hx = hitx-(*vx);
    hy = hity-(*vy);
    if (klabs(nx)+klabs(ny) > klabs(hx)+klabs(hy))
    {
        *vsectnum = hitsect;
        if (hitwall >= 0)
        {
            daang = getangle(wall[wall[hitwall].point2].x-wall[hitwall].x,
                             wall[wall[hitwall].point2].y-wall[hitwall].y);

            i = nx*sintable[daang]+ny*sintable[(daang+1536)&2047];
            if (klabs(nx) > klabs(ny)) hx -= mulscale28(nx,i);
            else hy -= mulscale28(ny,i);
        }
        else if (hitsprite < 0)
        {
            if (klabs(nx) > klabs(ny)) hx -= (nx>>5);
            else hy -= (ny>>5);
        }
        if (klabs(nx) > klabs(ny)) i = divscale16(hx,nx);
        else i = divscale16(hy,ny);
        if (i < cameradist) cameradist = i;
    }
    *vx = (*vx)+mulscale16(nx,cameradist);
    *vy = (*vy)+mulscale16(ny,cameradist);
    *vz = (*vz)+mulscale16(nz,cameradist);

    cameradist = min(cameradist+((totalclock-cameraclock)<<10),65536);
    cameraclock = totalclock;

    updatesectorz(*vx,*vy,*vz,vsectnum);

    sp->cstat = bakcstat;
}

//REPLACE FULLY
void drawbackground(void)
{
    int dapicnum;
    int x,y,x1,y1,x2,y2,rx;

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
        if (ud.screen_size == 8 && ud.statusbarmode == 0)
            y1 = scale(ydim,200-scale(tilesizy[BOTTOMSTATUSBAR],ud.statusbarscale,100),200);
        else if (gametype_flags[ud.coop] & GAMETYPE_FLAG_FRAGBAR)
        {
            if (ud.multimode > 1) y1 += scale(ydim,8,200);
            if (ud.multimode > 4) y1 += scale(ydim,8,200);
        }
    }
    else
    {
        // when not rendering a game, fullscreen wipe
#define MENUTILE (!getrendermode()?MENUSCREEN:LOADSCREEN)
        SetGameVarID(g_iReturnVarID,tilesizx[MENUTILE]==320&&tilesizy[MENUTILE]==200?MENUTILE:BIGHOLE, -1, -1);
        OnEvent(EVENT_GETMENUTILE, -1, myconnectindex, -1);
        if (GetGameVar("MENU_TILE", tilesizx[MENUTILE]==320&&tilesizy[MENUTILE]==200?0:1, -1, -1))
        {
            for (y=y1;y<y2;y+=tilesizy[GetGameVarID(g_iReturnVarID, -1, -1)])
                for (x=0;x<xdim;x+=tilesizx[GetGameVarID(g_iReturnVarID, -1, -1)])
                    rotatesprite(x<<16,y<<16,65536L,0,GetGameVarID(g_iReturnVarID, -1, -1),bpp==8?16:8,0,8+16+64,0,0,xdim-1,ydim-1);
        }
        else rotatesprite(320<<15,200<<15,65536L,0,GetGameVarID(g_iReturnVarID, -1, -1),bpp==8?16:8,0,2+8+64,0,0,xdim-1,ydim-1);
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
    }

    if (ud.screen_size > 8)
    {
        y = 0;
        if (gametype_flags[ud.coop] & GAMETYPE_FLAG_FRAGBAR)
        {
            if (ud.multimode > 1) y += 8;
            if (ud.multimode > 4) y += 8;
        }

        x1 = max(windowx1-4,0);
        y1 = max(windowy1-4,y);
        x2 = min(windowx2+4,xdim-1);
        y2 = min(windowy2+4,scale(ydim,200-scale(tilesizy[BOTTOMSTATUSBAR],ud.statusbarscale,100),200)-1);

        for (y=y1+4;y<y2-4;y+=64)
        {
            rotatesprite(x1<<16,y<<16,65536L,0,VIEWBORDER,0,0,8+16+64,x1,y1,x2,y2);
            rotatesprite((x2+1)<<16,(y+64)<<16,65536L,1024,VIEWBORDER,0,0,8+16+64,x1,y1,x2,y2);
        }

        for (x=x1+4;x<x2-4;x+=64)
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

#define SE40

#ifdef SE40
// Floor Over Floor

// If standing in sector with SE42 or SE44
// then draw viewing to SE41 and raise all =hi SE43 cielings.

// If standing in sector with SE43 or SE45
// then draw viewing to SE40 and lower all =hi SE42 floors.

static void SE40_Draw(int spnum,int x,int y,int z,int a,int h,int smoothratio)
{
    static int tempsectorz[MAXSECTORS];
    static int tempsectorpicnum[MAXSECTORS];

    int i=0,j=0,k=0;
    int floor1=0,floor2=0,ok=0,fofmode=0,draw_both=0;
    int offx,offy,offz;

    if (sprite[spnum].ang!=512) return;

    // Things are a little different now, as we allow for masked transparent
    // floors and ceilings. So the FOF textures is no longer required
    // Additionally names.h also defines FOF as 13 which isn't useful for us
    // so we'll use 562 instead
    tilesizx[562] = 0;
    tilesizy[562] = 0;

    floor1=spnum;

    if (sprite[spnum].lotag==42) fofmode=40;
    if (sprite[spnum].lotag==43) fofmode=41;
    if (sprite[spnum].lotag==44) fofmode=40;
    if (sprite[spnum].lotag==45) fofmode=41;

    // fofmode=sprite[spnum].lotag-2;

    // sectnum=sprite[j].sectnum;
    // sectnum=cursectnum;
    ok++;

    /*  recursive? - Not at the moment
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

    for (j=headspritestat[15];j>=0;j=nextspritestat[j])
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

    for (j=headspritestat[15];j>=0;j=nextspritestat[j])
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

    for (j=headspritestat[15];j>=0;j=nextspritestat[j]) // raise ceiling or floor
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
    animatesprites(x,y,a,smoothratio);
    drawmasks();

    if (draw_both)
    {
        for (j=headspritestat[15];j>=0;j=nextspritestat[j]) // restore ceiling or floor for the draw both sectors
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
        animatesprites(x,y,a,smoothratio);
        drawmasks();
    }
} // end SE40

void se40code(int x,int y,int z,int a,int h, int smoothratio)
{
    int i= headspritestat[15];

    while (i >= 0)
    {
        int t = sprite[i].lotag;
        switch (t)
        {
            //            case 40:
            //            case 41:
            //                SE40_Draw(i,x,y,a,smoothratio);
            //                break;
        case 42:
        case 43:
        case 44:
        case 45:
            if (g_player[screenpeek].ps->cursectnum == sprite[i].sectnum)
                SE40_Draw(i,x,y,z,a,h,smoothratio);
            break;
        }
        i = nextspritestat[i];
    }
}
#endif /* SE40 */

static int oyrepeat=-1;
extern float r_ambientlight;

void displayrooms(int snum,int smoothratio)
{
    int dst,j,fz,cz;
    int tposx,tposy,i;
    short k;
    player_struct *p = g_player[snum].ps;
    short tang;
    int tiltcx,tiltcy,tiltcs=0;    // JBF 20030807

    if (pub > 0 || getrendermode() >= 3) // JBF 20040101: redraw background always
    {
        if (getrendermode() >= 3 || ud.screen_size > 8 || (ud.screen_size == 8 && ud.statusbarscale<100))
            drawbackground();
        pub = 0;
    }

    if (ud.overhead_on == 2 || ud.show_help || (p->cursectnum == -1 && getrendermode() < 3))
        return;

    smoothratio = min(max(smoothratio,0),65536);

    visibility = (int)(p->visibility*(numplayers>1?1.f:r_ambientlightrecip));

    if (ud.pause_on || g_player[snum].ps->on_crane > -1) smoothratio = 65536;

    ud.camerasect = p->cursectnum;

    if (getrendermode() < 3 && (ud.camerasect < 0 || ud.camerasect >= MAXSECTORS)) return;

    dointerpolations(smoothratio);

    animatecamsprite();

    if (ud.camerasprite >= 0)
    {
        spritetype *s = &sprite[ud.camerasprite];

        if (s->yvel < 0) s->yvel = -100;
        else if (s->yvel > 199) s->yvel = 300;

        ud.cameraang = hittype[ud.camerasprite].tempang+mulscale16((int)(((s->ang+1024-hittype[ud.camerasprite].tempang)&2047)-1024),smoothratio);
#ifdef SE40
        se40code(s->x,s->y,s->z,ud.cameraang,s->yvel,smoothratio);
#endif
#ifdef POLYMER
        if (getrendermode() == 4)
            polymer_setanimatesprites(animatesprites, s->x, s->y, ud.cameraang, smoothratio);
#endif
        drawrooms(s->x,s->y,s->z-(4<<8),ud.cameraang,s->yvel,s->sectnum);
        animatesprites(s->x,s->y,ud.cameraang,smoothratio);
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

        if (screencapt)
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
                for (i=((60*tiltcs)>>(1-ud.detail))-1;i>=0;i--)
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
            ud.camerax = omyx+mulscale16((int)(myx-omyx),smoothratio);
            ud.cameray = omyy+mulscale16((int)(myy-omyy),smoothratio);
            ud.cameraz = omyz+mulscale16((int)(myz-omyz),smoothratio);
            ud.cameraang = omyang+mulscale16((int)(((myang+1024-omyang)&2047)-1024),smoothratio);
            ud.camerahoriz = omyhoriz+omyhorizoff+mulscale16((int)(myhoriz+myhorizoff-omyhoriz-omyhorizoff),smoothratio);
            ud.camerasect = mycursectnum;
        }
        else
        {
            ud.camerax = p->oposx+mulscale16((int)(p->posx-p->oposx),smoothratio);
            ud.cameray = p->oposy+mulscale16((int)(p->posy-p->oposy),smoothratio);
            ud.cameraz = p->oposz+mulscale16((int)(p->posz-p->oposz),smoothratio);
            ud.cameraang = p->oang+mulscale16((int)(((p->ang+1024-p->oang)&2047)-1024),smoothratio);
            ud.camerahoriz = p->ohoriz+p->ohorizoff+mulscale16((int)(p->horiz+p->horizoff-p->ohoriz-p->ohorizoff),smoothratio);
        }
        ud.cameraang += p->look_ang;

        if (p->newowner >= 0)
        {
            ud.cameraang = p->ang+p->look_ang;
            ud.camerahoriz = p->horiz+p->horizoff;
            ud.camerax = p->posx;
            ud.cameray = p->posy;
            ud.cameraz = p->posz;
            ud.camerasect = sprite[p->newowner].sectnum;
            smoothratio = 65536L;
        }
        else if (p->over_shoulder_on == 0)
        {
            if (ud.viewbob)
                ud.cameraz += p->opyoff+mulscale16((int)(p->pyoff-p->opyoff),smoothratio);
        }
        else view(p,&ud.camerax,&ud.cameray,&ud.cameraz,&ud.camerasect,ud.cameraang,ud.camerahoriz);

        cz = hittype[p->i].ceilingz;
        fz = hittype[p->i].floorz;

        if (earthquaketime > 0 && p->on_ground == 1)
        {
            ud.cameraz += 256-(((earthquaketime)&1)<<9);
            ud.cameraang += (2-((earthquaketime)&2))<<2;
        }

        if (sprite[p->i].pal == 1) ud.cameraz -= (18<<8);

        if (p->newowner >= 0)
            ud.camerahoriz = 100+sprite[p->newowner].shade;
        else if (p->spritebridge == 0)
        {
            if (ud.cameraz < (p->truecz + (4<<8))) ud.cameraz = cz + (4<<8);
            else if (ud.cameraz > (p->truefz - (4<<8))) ud.cameraz = fz - (4<<8);
        }

        if (ud.camerasect >= 0)
        {
            getzsofslope(ud.camerasect,ud.camerax,ud.cameray,&cz,&fz);
            if (ud.cameraz < cz+(4<<8)) ud.cameraz = cz+(4<<8);
            if (ud.cameraz > fz-(4<<8)) ud.cameraz = fz-(4<<8);
        }

        if (ud.camerahoriz > HORIZ_MAX) ud.camerahoriz = HORIZ_MAX;
        else if (ud.camerahoriz < HORIZ_MIN) ud.camerahoriz = HORIZ_MIN;

        OnEvent(EVENT_DISPLAYROOMS, g_player[screenpeek].ps->i, screenpeek, -1);

#ifdef SE40
        se40code(ud.camerax,ud.cameray,ud.cameraz,ud.cameraang,ud.camerahoriz,smoothratio);
#endif
        if (((gotpic[MIRROR>>3]&(1<<(MIRROR&7))) > 0)
#if defined(POLYMOST) && defined(USE_OPENGL)
                && (getrendermode() != 4)
#endif
           )
        {
            dst = 0x7fffffff;
            i = 0;
            for (k=mirrorcnt-1;k>=0;k--)
            {
                j = klabs(wall[mirrorwall[k]].x-ud.camerax);
                j += klabs(wall[mirrorwall[k]].y-ud.cameray);
                if (j < dst) dst = j, i = k;
            }

            if (wall[mirrorwall[i]].overpicnum == MIRROR)
            {
                preparemirror(ud.camerax,ud.cameray,ud.cameraz,ud.cameraang,ud.camerahoriz,mirrorwall[i],mirrorsector[i],&tposx,&tposy,&tang);

                j = visibility;
                visibility = (j>>1) + (j>>2);

                drawrooms(tposx,tposy,ud.cameraz,tang,ud.camerahoriz,mirrorsector[i]+MAXSECTORS);

                display_mirror = 1;
                animatesprites(tposx,tposy,tang,smoothratio);
                display_mirror = 0;

                drawmasks();
                completemirror();   //Reverse screen x-wise in this function
                visibility = j;
            }
            gotpic[MIRROR>>3] &= ~(1<<(MIRROR&7));
        }

#ifdef POLYMER
        if (getrendermode() == 4)
            polymer_setanimatesprites(animatesprites, ud.camerax,ud.cameray,ud.cameraang,smoothratio);
#endif
        drawrooms(ud.camerax,ud.cameray,ud.cameraz,ud.cameraang,ud.camerahoriz,ud.camerasect);
        animatesprites(ud.camerax,ud.cameray,ud.cameraang,smoothratio);
        drawmasks();

        if (screencapt == 1)
        {
            setviewback();
            screencapt = 0;
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

    restoreinterpolations();

    if (totalclock < lastvisinc)
    {
        if (klabs(p->visibility-ud.const_visibility) > 8)
            p->visibility += (ud.const_visibility-p->visibility)>>2;
    }
    else p->visibility = ud.const_visibility;
}

static void dumpdebugdata(void)
{
    int i,j,x;
    FILE * fp=fopen("debug.con","w");
    for (i=0;i<MAX_WEAPONS;i++)
    {
        for (j=0;j<numplayers;j++)
        {
            fprintf(fp,"Player %d\n\n",j);
            fprintf(fp,"WEAPON%d_CLIP %" PRIdPTR "\n",i,aplWeaponClip[i][j]);
            fprintf(fp,"WEAPON%d_RELOAD %" PRIdPTR "\n",i,aplWeaponReload[i][j]);
            fprintf(fp,"WEAPON%d_FIREDELAY %" PRIdPTR "\n",i,aplWeaponFireDelay[i][j]);
            fprintf(fp,"WEAPON%d_TOTALTIME %" PRIdPTR "\n",i,aplWeaponTotalTime[i][j]);
            fprintf(fp,"WEAPON%d_HOLDDELAY %" PRIdPTR "\n",i,aplWeaponHoldDelay[i][j]);
            fprintf(fp,"WEAPON%d_FLAGS %" PRIdPTR "\n",i,aplWeaponFlags[i][j]);
            fprintf(fp,"WEAPON%d_SHOOTS %" PRIdPTR "\n",i,aplWeaponShoots[i][j]);
            fprintf(fp,"WEAPON%d_SPAWNTIME %" PRIdPTR "\n",i,aplWeaponSpawnTime[i][j]);
            fprintf(fp,"WEAPON%d_SPAWN %" PRIdPTR "\n",i,aplWeaponSpawn[i][j]);
            fprintf(fp,"WEAPON%d_SHOTSPERBURST %" PRIdPTR "\n",i,aplWeaponShotsPerBurst[i][j]);
            fprintf(fp,"WEAPON%d_WORKSLIKE %" PRIdPTR "\n",i,aplWeaponWorksLike[i][j]);
            fprintf(fp,"WEAPON%d_INITIALSOUND %" PRIdPTR "\n",i,aplWeaponInitialSound[i][j]);
            fprintf(fp,"WEAPON%d_FIRESOUND %" PRIdPTR "\n",i,aplWeaponFireSound[i][j]);
            fprintf(fp,"WEAPON%d_SOUND2TIME %" PRIdPTR "\n",i,aplWeaponSound2Time[i][j]);
            fprintf(fp,"WEAPON%d_SOUND2SOUND %" PRIdPTR "\n",i,aplWeaponSound2Sound[i][j]);
            fprintf(fp,"WEAPON%d_RELOADSOUND1 %" PRIdPTR "\n",i,aplWeaponReloadSound1[i][j]);
            fprintf(fp,"WEAPON%d_RELOADSOUND2 %" PRIdPTR "\n",i,aplWeaponReloadSound2[i][j]);
        }
        fprintf(fp,"\n");
    }
    for (x=0;x<MAXSTATUS;x++)
    {
        j = headspritestat[x];
        while (j >= 0)
        {
            fprintf(fp,"Sprite %d (%d,%d,%d) (picnum: %d)\n",j,sprite[j].x,sprite[j].y,sprite[j].z,sprite[j].picnum);
            for (i=0;i<iGameVarCount;i++)
            {
                if (aGameVars[i].dwFlags & (GAMEVAR_FLAG_PERACTOR))
                {
                    if (aGameVars[i].plValues[j] != aGameVars[i].lDefault)
                    {
                        fprintf(fp,"gamevar %s ",aGameVars[i].szLabel);
                        fprintf(fp,"%" PRIdPTR "",aGameVars[i].plValues[j]);
                        fprintf(fp," GAMEVAR_FLAG_PERACTOR");
                        if (aGameVars[i].dwFlags != GAMEVAR_FLAG_PERACTOR)
                        {
                            fprintf(fp," // ");
                            if (aGameVars[i].dwFlags & (GAMEVAR_FLAG_SYSTEM))
                            {
                                fprintf(fp," (system)");
                            }
                        }
                        fprintf(fp,"\n");
                    }
                }
            }
            fprintf(fp,"\n");
            j = nextspritestat[j];
        }
    }
    DumpGameVars(fp);
    fclose(fp);
    saveboard("debug.map",&g_player[myconnectindex].ps->posx,&g_player[myconnectindex].ps->posy,&g_player[myconnectindex].ps->posz,&g_player[myconnectindex].ps->ang,&g_player[myconnectindex].ps->cursectnum);
}

int EGS(int whatsect,int s_x,int s_y,int s_z,int s_pn,int s_s,int s_xr,int s_yr,int s_a,int s_ve,int s_zv,int s_ow,int s_ss)
{
    int i = insertsprite(whatsect,s_ss);
    int p;
    spritetype *s = &sprite[i];

    if (i < 0)
    {
        dumpdebugdata();
        OSD_Printf("Failed spawning sprite with tile %d from sprite %d (%d) at x:%d,y:%d,z:%d,sector:%d\n",s_pn,s_ow,sprite[s_ow].picnum,s_x,s_y,s_z,whatsect);
        gameexit("Too many sprites spawned.");
    }

    hittype[i].bposx = s_x;
    hittype[i].bposy = s_y;
    hittype[i].bposz = s_z;

    s->x = s_x;
    s->y = s_y;
    s->z = s_z;
    s->cstat = 0;
    s->picnum = s_pn;
    s->shade = s_s;
    s->xrepeat = s_xr;
    s->yrepeat = s_yr;
    s->pal = 0;

    s->ang = s_a;
    s->xvel = s_ve;
    s->zvel = s_zv;
    s->owner = s_ow;
    s->xoffset = 0;
    s->yoffset = 0;
    s->yvel = 0;
    s->clipdist = 0;
    s->pal = 0;
    s->lotag = 0;

    if (s_ow > -1 && s_ow < MAXSPRITES)
    {
        hittype[i].picnum = sprite[s_ow].picnum;
        hittype[i].floorz = hittype[s_ow].floorz;
        hittype[i].ceilingz = hittype[s_ow].ceilingz;
    }

    hittype[i].lastvx = 0;
    hittype[i].lastvy = 0;

    hittype[i].timetosleep = 0;
    hittype[i].actorstayput = -1;
    hittype[i].extra = -1;
    hittype[i].owner = s_ow;
    hittype[i].cgg = 0;
    hittype[i].movflag = 0;
    hittype[i].tempang = 0;
    hittype[i].dispicnum = 0;

    T1=T3=T4=T6=T7=T8=T9=0;

    hittype[i].flags = 0;

    sprpos[i].ang = sprpos[i].oldang = sprite[i].ang;

    if (actorscrptr[s_pn])
    {
        s->extra = *actorscrptr[s_pn];
        T5 = *(actorscrptr[s_pn]+1);
        T2 = *(actorscrptr[s_pn]+2);
        s->hitag = *(actorscrptr[s_pn]+3);
    }
    else
    {
        T2=T5=0;
        s->extra = 0;
        s->hitag = 0;
    }

    if (show2dsector[SECT>>3]&(1<<(SECT&7))) show2dsprite[i>>3] |= (1<<(i&7));
    else show2dsprite[i>>3] &= ~(1<<(i&7));

    clearbufbyte(&spriteext[i], sizeof(spriteexttype), 0);
    clearbufbyte(&spritesmooth[i], sizeof(spritesmoothtype), 0);

    /*
        if(s->sectnum < 0)
        {
            s->xrepeat = s->yrepeat = 0;
            changespritestat(i,5);
        }
    */
    ResetActorGameVars(i);
    hittype[i].flags = 0;

    if (apScriptGameEvent[EVENT_EGS])
    {
        int pl=findplayer(&sprite[i],&p);
        OnEvent(EVENT_EGS,i, pl, p);
    }

    return(i);
}

int wallswitchcheck(int i)
{
    int j;
    //MULTISWITCH has 4 states so deal with it separately
    if ((PN >= MULTISWITCH) && (PN <=MULTISWITCH+3)) return 1;
    // ACCESSSWITCH and ACCESSSWITCH2 are only active in 1 state so deal with them separately
    if ((PN == ACCESSSWITCH) || (PN == ACCESSSWITCH2)) return 1;
    //loop to catch both states of switches
    for (j=0;j<=1;j++)
    {
        switch (dynamictostatic[PN-j])
        {
        case HANDPRINTSWITCH__STATIC:
            //case HANDPRINTSWITCH+1:
        case ALIENSWITCH__STATIC:
            //case ALIENSWITCH+1:
        case MULTISWITCH__STATIC:
            //case MULTISWITCH+1:
            //case MULTISWITCH+2:
            //case MULTISWITCH+3:
            //case ACCESSSWITCH:
            //case ACCESSSWITCH2:
        case PULLSWITCH__STATIC:
            //case PULLSWITCH+1:
        case HANDSWITCH__STATIC:
            //case HANDSWITCH+1:
        case SLOTDOOR__STATIC:
            //case SLOTDOOR+1:
        case LIGHTSWITCH__STATIC:
            //case LIGHTSWITCH+1:
        case SPACELIGHTSWITCH__STATIC:
            //case SPACELIGHTSWITCH+1:
        case SPACEDOORSWITCH__STATIC:
            //case SPACEDOORSWITCH+1:
        case FRANKENSTINESWITCH__STATIC:
            //case FRANKENSTINESWITCH+1:
        case LIGHTSWITCH2__STATIC:
            //case LIGHTSWITCH2+1:
        case POWERSWITCH1__STATIC:
            //case POWERSWITCH1+1:
        case LOCKSWITCH1__STATIC:
            //case LOCKSWITCH1+1:
        case POWERSWITCH2__STATIC:
            //case POWERSWITCH2+1:
        case DIPSWITCH__STATIC:
            //case DIPSWITCH+1:
        case DIPSWITCH2__STATIC:
            //case DIPSWITCH2+1:
        case TECHSWITCH__STATIC:
            //case TECHSWITCH+1:
        case DIPSWITCH3__STATIC:
            //case DIPSWITCH3+1:
            return 1;
        }
    }
    return 0;
}

int spawn(int j, int pn)
{
    int i, s, startwall, endwall, sect, clostest=0;
    int x, y, d, p;
    spritetype *sp;

    if (j >= 0)
    {
        i = EGS(sprite[j].sectnum,sprite[j].x,sprite[j].y,sprite[j].z
                ,pn,0,0,0,0,0,0,j,0);
        hittype[i].picnum = sprite[j].picnum;
    }
    else
    {
        i = pn;
        hittype[i].picnum = PN;
        hittype[i].timetosleep = 0;
        hittype[i].extra = -1;

        hittype[i].bposx = SX;
        hittype[i].bposy = SY;
        hittype[i].bposz = SZ;

        OW = hittype[i].owner = i;
        hittype[i].cgg = 0;
        hittype[i].movflag = 0;
        hittype[i].tempang = 0;
        hittype[i].dispicnum = 0;
        hittype[i].floorz = sector[SECT].floorz;
        hittype[i].ceilingz = sector[SECT].ceilingz;

        hittype[i].lastvx = 0;
        hittype[i].lastvy = 0;
        hittype[i].actorstayput = -1;

        T1 = T2 = T3 = T4 = T5 = T6 = T7 = T8 = T9 = 0;

        hittype[i].flags = 0;

        sprpos[i].ang = sprpos[i].oldang = sprite[i].ang;

        if (PN != SPEAKER && PN != LETTER && PN != DUCK && PN != TARGET && PN != TRIPBOMB && PN != VIEWSCREEN && PN != VIEWSCREEN2 && (CS&48))
            if (!(PN >= CRACK1 && PN <= CRACK4))
            {
                if (SS == 127) return i;
                if (wallswitchcheck(i) == 1 && (CS&16))
                {
                    if (PN != ACCESSSWITCH && PN != ACCESSSWITCH2 && sprite[i].pal)
                    {
                        if ((ud.multimode < 2) || (ud.multimode > 1 && !GTFLAGS(GAMETYPE_FLAG_DMSWITCHES)))
                        {
                            sprite[i].xrepeat = sprite[i].yrepeat = 0;
                            sprite[i].cstat = SLT = SHT = 0;
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
                    SH = impact_damage;
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

            if (camerashitable) sp->cstat = 257;
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
                changespritestat(i,1);
            }
        }
    }
    else switch (dynamictostatic[sp->picnum])
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

                if (actortype[sp->picnum] & 3)
                {
                    if (ud.monsters_off == 1)
                    {
                        sp->xrepeat=sp->yrepeat=0;
                        changespritestat(i,5);
                        break;
                    }

                    makeitfall(i);

                    if (actortype[sp->picnum] & 2)
                        hittype[i].actorstayput = sp->sectnum;

                    g_player[myconnectindex].ps->max_actors_killed++;
                    sp->clipdist = 80;
                    if (j >= 0)
                    {
                        if (sprite[j].picnum == RESPAWN)
                            hittype[i].tempang = sprite[i].pal = sprite[j].pal;
                        changespritestat(i,1);
                    }
                    else changespritestat(i,2);
                }
                else
                {
                    sp->clipdist = 40;
                    sp->owner = i;
                    changespritestat(i,1);
                }

                hittype[i].timetosleep = 0;

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
                setsprite(i,sprite[j].x,sprite[j].y,sprite[j].z);
                sp->xrepeat = sp->yrepeat = 8+(TRAND&7);
            }
            else sp->xrepeat = sp->yrepeat = 16+(TRAND&15);

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
            sp->zvel = 256-(TRAND&511);
            sp->xvel = 64-(TRAND&127);
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
                if (sprite[j].statnum == 4)
                {
                    sp->xrepeat = 8;
                    sp->yrepeat = 8;
                }
                else
                {
                    sp->xrepeat = 48;
                    sp->yrepeat = 64;
                    if (sprite[j].statnum == 10 || badguy(&sprite[j]))
                        sp->z -= (32<<8);
                }
            }

            sp->shade = -127;
            sp->cstat = 128|2;
            sp->ang = sprite[j].ang;

            sp->xvel = 128;
            changespritestat(i,5);
            ssp(i,CLIPMASK0);
            setsprite(i,sp->x,sp->y,sp->z);
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

            if (lasermode == 1)
                sp->cstat = 16 + 2;
            else if (lasermode == 0 || lasermode == 2)
                sp->cstat = 16;
            else
            {
                sp->xrepeat = 0;
                sp->yrepeat = 0;
            }

            if (j >= 0) sp->ang = hittype[j].temp_data[5]+512;
            changespritestat(i,5);
            break;

        case FORCESPHERE__STATIC:
            if (j == -1)
            {
                sp->cstat = (short) 32768;
                changespritestat(i,2);
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
            short s1;
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
            sp->xrepeat = 7+(TRAND&7);
            sp->yrepeat = 7+(TRAND&7);
            sp->z -= (16<<8);
            if (j >= 0 && sprite[j].pal == 6)
                sp->pal = 6;
            insertspriteq(i);
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

            sp->owner = i;
            sp->hitag = i;

            sp->xvel = 16;
            ssp(i,CLIPMASK0);
            hittype[i].temp_data[0] = 17;
            hittype[i].temp_data[2] = 0;
            hittype[i].temp_data[5] = sp->ang;

        case SPACEMARINE__STATIC:
            if (sp->picnum == SPACEMARINE)
            {
                sp->extra = 20;
                sp->cstat |= 257;
            }
            changespritestat(i,2);
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
                short s1;
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

            insertspriteq(i);
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

            changespritestat(i,2);
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
                if (j >= 0) sp->z = hittype[j].floorz; // -(1<<4);
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
            changespritestat(i,1);
            break;

        case SPOTLITE__STATIC:
            T1 = sp->x;
            T2 = sp->y;
            break;
        case BULLETHOLE__STATIC:
            sp->xrepeat = sp->yrepeat = 3;
            sp->cstat = 16+(krand()&12);
            insertspriteq(i);
        case MONEY__STATIC:
        case MAIL__STATIC:
        case PAPER__STATIC:
            if (sp->picnum == MONEY || sp->picnum == MAIL || sp->picnum == PAPER)
            {
                hittype[i].temp_data[0] = TRAND&2047;
                sp->cstat = TRAND&12;
                sp->xrepeat = sp->yrepeat = 8;
                sp->ang = TRAND&2047;
            }
            changespritestat(i,5);
            break;

        case VIEWSCREEN__STATIC:
        case VIEWSCREEN2__STATIC:
            sp->owner = i;
            sp->lotag = 1;
            sp->extra = 1;
            changespritestat(i,6);
            break;

        case SHELL__STATIC: //From the player
        case SHOTGUNSHELL__STATIC:
            if (j >= 0)
            {
                int snum,a;

                if (sprite[j].picnum == APLAYER)
                {
                    snum = sprite[j].yvel;
                    a = g_player[snum].ps->ang-(TRAND&63)+8;  //Fine tune

                    T1 = TRAND&1;
                    sp->z = (3<<8)+g_player[snum].ps->pyoff+g_player[snum].ps->posz-
                            ((g_player[snum].ps->horizoff+g_player[snum].ps->horiz-100)<<4);
                    if (sp->picnum == SHOTGUNSHELL)
                        sp->z += (3<<8);
                    sp->zvel = -(TRAND&255);
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
            sp->cstat = (short)32768;
            changespritestat(i,11);
            break;

        case EXPLOSION2__STATIC:
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
                sp->cstat = 128|(TRAND&4);
            }

            if (sp->picnum == EXPLOSION2 || sp->picnum == EXPLOSION2BOT)
            {
                sp->xrepeat = 48;
                sp->yrepeat = 48;
                sp->shade = -127;
                sp->cstat |= 128;
            }
            else if (sp->picnum == SHRINKEREXPLOSION)
            {
                sp->xrepeat = 32;
                sp->yrepeat = 32;
            }
            else if (sp->picnum == SMALLSMOKE)
            {
                // 64 "money"
                sp->xrepeat = 24;
                sp->yrepeat = 24;
            }
            else if (sp->picnum == BURNING || sp->picnum == BURNING2)
            {
                sp->xrepeat = 4;
                sp->yrepeat = 4;
            }

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
            j=(gametype_flags[ud.coop] & GAMETYPE_FLAG_COOPSPAWN) / GAMETYPE_FLAG_COOPSPAWN ;
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

            s = headspritestat[0];
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

                    setsprite(s,sprite[s].x,sprite[s].y,sprite[s].z);
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
            if (j >= 0 && (sprite[j].statnum == 10 || sprite[j].statnum == 1))
            {
                sp->shade = 32;
                if (sprite[j].pal != 1)
                {
                    sp->pal = 2;
                    sp->z -= (18<<8);
                }
                else sp->z -= (13<<8);
                sp->ang = getangle(g_player[connecthead].ps->posx-sp->x,g_player[connecthead].ps->posy-sp->y);
                sp->xvel = 48-(TRAND&31);
                ssp(i,CLIPMASK0);
            }
            else if (j == -1)
            {
                sp->z += (4<<8);
                T1 = sp->z;
                T2 = TRAND&127;
            }
        case TRASH__STATIC:

            if (sp->picnum != WATERDRIP)
                sp->ang = TRAND&2047;

        case WATERDRIPSPLASH__STATIC:

            sp->xrepeat = 24;
            sp->yrepeat = 24;

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
                // JBF 20030913: Pisses off move(), eg. in bobsp2
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
            changespritestat(i,1);
            break;
        case OCTABRAINSTAYPUT__STATIC:
        case LIZTROOPSTAYPUT__STATIC:
        case PIGCOPSTAYPUT__STATIC:
        case LIZMANSTAYPUT__STATIC:
        case BOSS1STAYPUT__STATIC:
        case PIGCOPDIVE__STATIC:
        case COMMANDERSTAYPUT__STATIC:
        case BOSS4STAYPUT__STATIC:
            hittype[i].actorstayput = sp->sectnum;
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
                switch (dynamictostatic[sp->picnum])
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
                    sp->xrepeat = 40;
                    sp->yrepeat = 40;
                }
                else
                {
                    sp->xrepeat = 80;
                    sp->yrepeat = 80;
                    sp->clipdist = 164;
                }
            }
            else
            {
                if (sp->picnum != SHARK)
                {
                    sp->xrepeat = 40;
                    sp->yrepeat = 40;
                    sp->clipdist = 80;
                }
                else
                {
                    sp->xrepeat = 60;
                    sp->yrepeat = 60;
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
                makeitfall(i);

                if (sp->picnum == RAT)
                {
                    sp->ang = TRAND&2047;
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
                    hittype[i].timetosleep = 0;
                    check_fta_sounds(i);
                    changespritestat(i,1);
                }
                else changespritestat(i,2);
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
            sp->cstat = (short) 32768;
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
                insertspriteq(i);
            }

            changespritestat(i,1);

            getglobalz(i);

            j = (hittype[i].floorz-hittype[i].ceilingz)>>9;

            sp->yrepeat = j;
            sp->xrepeat = 25-(j>>1);
            sp->cstat |= (TRAND&4);

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
                hittype[i].temp_data[5] = 0;
                if (ud.monsters_off == 1)
                {
                    sp->xrepeat = sp->yrepeat = 0;
                    changespritestat(i,5);
                    break;
                }
                sp->extra = 130;
            }

            if (sp->picnum == REACTOR || sp->picnum == REACTOR2)
                sp->extra = impact_damage;

            CS |= 257; // Make it hitable

            if (ud.multimode < 2 && sp->pal != 0)
            {
                sp->xrepeat = sp->yrepeat = 0;
                changespritestat(i,5);
                break;
            }
            sp->pal = 0;
            SS = -17;

            changespritestat(i,2);
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
                ssp(i,CLIPMASK0);
                sp->cstat = TRAND&4;
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

            if (ud.multimode > 1 && !GTFLAGS(GAMETYPE_FLAG_ACCESSCARDSPRITES) && sp->picnum == ACCESSCARD)
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

            if (j >= 0) changespritestat(i,1);
            else
            {
                changespritestat(i,2);
                makeitfall(i);
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
            sp->extra = impact_damage<<2;
            changespritestat(i,2);
            break;

        case STEAM__STATIC:
            if (j >= 0)
            {
                sp->ang = sprite[j].ang;
                sp->cstat = 16+128+2;
                sp->xrepeat=sp->yrepeat=1;
                sp->xvel = -8;
                ssp(i,CLIPMASK0);
            }
        case CEILINGSTEAM__STATIC:
            changespritestat(i,6);
            break;

        case SECTOREFFECTOR__STATIC:
            sp->yvel = sector[sect].extra;
            sp->cstat |= 32768;
            sp->xrepeat = sp->yrepeat = 0;

            switch (sp->lotag)
            {
            case 28:
                T6 = 65;// Delay for lightning
                break;
            case 7: // Transporters!!!!
            case 23:// XPTR END
                if (sp->lotag != 23)
                {
                    for (j=0;j<MAXSPRITES;j++)
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
                setinterpolation(&sector[sect].ceilingz);
                break;
            case 35:
                sector[sect].ceilingz = sp->z;
                break;
            case 27:
                if (ud.recstat == 1)
                {
                    sp->xrepeat=sp->yrepeat=64;
                    sp->cstat &= 32767;
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
                        for (j=startwall;j<endwall;j++)
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
                    setinterpolation(&sector[sect].floorz);
                    setinterpolation(&sector[sect].ceilingz);
                }

                break;

            case 24:
                sp->yvel <<= 1;
            case 36:
                break;

            case 20:
            {
                int q;

                startwall = sector[sect].wallptr;
                endwall = startwall+sector[sect].wallnum;

                //find the two most clostest wall x's and y's
                q = 0x7fffffff;

                for (s=startwall;s<endwall;s++)
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

                for (s=startwall;s<endwall;s++)
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

                for (s=startwall;s<endwall;s++)
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

                for (s=startwall;s<endwall;s++)
                    if (wall[s].hitag == 0) wall[s].hitag = 9999;

                setinterpolation(&sector[sect].floorz);

                break;
            case 32:
                T2 = sector[sect].ceilingz;
                T3 = sp->hitag;
                if (sp->ang != 1536) sector[sect].ceilingz = sp->z;

                startwall = sector[sect].wallptr;
                endwall = startwall+sector[sect].wallnum;

                for (s=startwall;s<endwall;s++)
                    if (wall[s].hitag == 0) wall[s].hitag = 9999;

                setinterpolation(&sector[sect].ceilingz);

                break;

            case 4: //Flashing lights

                T3 = sector[sect].floorshade;

                startwall = sector[sect].wallptr;
                endwall = startwall+sector[sect].wallnum;

                sp->owner = sector[sect].ceilingpal<<8;
                sp->owner |= sector[sect].floorpal;

                for (s=startwall;s<endwall;s++)
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

                for (s=startwall;s<endwall;s++)
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

                    for (j = MAXSPRITES-1;j>=0;j--)
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
//                        gameexit(tempbuf);
                        OSD_Printf(OSD_ERROR "Found lonely Sector Effector (lotag 0) at (%d,%d)\n",sp->x,sp->y);
                        changespritestat(i,1);
                        if (apScriptGameEvent[EVENT_SPAWN])
                        {
                            int pl=findplayer(&sprite[i],&p);
                            OnEvent(EVENT_SPAWN,i, pl, p);
                        }
                        return i;
                    }
                    sp->owner = j;
                }

                startwall = sector[sect].wallptr;
                endwall = startwall+sector[sect].wallnum;

                T2 = tempwallptr;
                for (s=startwall;s<endwall;s++)
                {
                    msx[tempwallptr] = wall[s].x-sp->x;
                    msy[tempwallptr] = wall[s].y-sp->y;
                    tempwallptr++;
                    if (tempwallptr > 2047)
                    {
                        Bsprintf(tempbuf,"Too many moving sectors at (%d,%d).\n",wall[s].x,wall[s].y);
                        gameexit(tempbuf);
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

                    for (s=startwall;s<endwall;s++)
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
                        gameexit(tempbuf);
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
                j = callsound(sect,i);
                if (j == -1) j = SUBWAY;
                hittype[i].lastvx = j;
            case 30:
                if (numplayers > 1) break;
            case 0:
            case 1:
            case 5:
            case 11:
            case 15:
            case 16:
            case 26:
                setsectinterpolate(i);
                break;
            }

            switch (sprite[i].lotag)
            {
            case 40:
            case 41:
            case 42:
            case 43:
            case 44:
            case 45:
                changespritestat(i,15);
                break;
            default:
                changespritestat(i,3);
                break;
            }

            break;

        case SEENINE__STATIC:
        case OOZFILTER__STATIC:

            sp->shade = -16;
            if (sp->xrepeat <= 8)
            {
                sp->cstat = (short)32768;
                sp->xrepeat=sp->yrepeat=0;
            }
            else sp->cstat = 1+256;
            sp->extra = impact_damage<<2;
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
                sp->extra = impact_damage<<2;
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
            ssp(i,CLIPMASK0);
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
            makeitfall(i);
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
                sp->cstat = 257|(TRAND&4);
                changespritestat(i,2);
            }
            break;
        case TOILETWATER__STATIC:
            sp->shade = -16;
            changespritestat(i,6);
            break;
        }

    if (apScriptGameEvent[EVENT_SPAWN])
    {
        int pl=findplayer(&sprite[i],&p);
        OnEvent(EVENT_SPAWN,i, pl, p);
    }

    return i;
}

#ifdef _MSC_VER
// Visual C thought this was a bit too hard to optimise so we'd better
// tell it not to try... such a pussy it is.
//#pragma auto_inline(off)
#pragma optimize("g",off)
#endif
void animatesprites(int x,int y,int a,int smoothratio)
{
    int i, j, k, p, sect;
    intptr_t l, t1,t3,t4;
    spritetype *s,*t;
    int switchpic;

    if (!spritesortcnt) return;

    for (j=spritesortcnt-1;j>=0; j--)
    {
        t = &tsprite[j];
        i = t->owner;
        s = &sprite[t->owner];

        //greenslime can't be handled through the dynamictostatic system due to addition on constant
        if ((t->picnum >= GREENSLIME)&&(t->picnum <= GREENSLIME+7))
            {}
        else switch (dynamictostatic[t->picnum])
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
                if (((t->cstat&16)) || (badguy(t) && t->extra > 0) || t->statnum == 10)
                    continue;
            }

        if (checkspriteflags(t->owner,SPRITE_FLAG_NOSHADE))
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

    for (j=spritesortcnt-1;j>=0; j--) //Between drawrooms() and drawmasks()
    {
        //is the perfect time to animate sprites
        t = &tsprite[j];
        i = t->owner;
        s = &sprite[i];

        switch (dynamictostatic[s->picnum])
        {
        case SECTOREFFECTOR__STATIC:
            if (t->lotag == 27 && ud.recstat == 1)
            {
                t->picnum = 11+((totalclock>>3)&1);
                t->cstat |= 128;
            }
            else
                t->xrepeat = t->yrepeat = 0;
            break;
        case NATURALLIGHTNING__STATIC:
            t->shade = -127;
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
        if (s->statnum != 1 && s->picnum == APLAYER && g_player[s->yvel].ps->newowner == -1 && s->owner >= 0)
        {
            t->x -= mulscale16(65536-smoothratio,g_player[s->yvel].ps->posx-g_player[s->yvel].ps->oposx);
            t->y -= mulscale16(65536-smoothratio,g_player[s->yvel].ps->posy-g_player[s->yvel].ps->oposy);
            t->z = g_player[s->yvel].ps->oposz + mulscale16(smoothratio,g_player[s->yvel].ps->posz-g_player[s->yvel].ps->oposz);
            t->z += (40<<8);
        }
        else if ((s->statnum == 0 && s->picnum != CRANEPOLE) || s->statnum == 10 || s->statnum == 6 || s->statnum == 4 || s->statnum == 5 || s->statnum == 1)
        {
            t->x -= mulscale16(65536-smoothratio,s->x-hittype[i].bposx);
            t->y -= mulscale16(65536-smoothratio,s->y-hittype[i].bposy);
            t->z -= mulscale16(65536-smoothratio,s->z-hittype[i].bposz);
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

        switch (dynamictostatic[switchpic])
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
                short sqa,sqb;

                sqa =
                    getangle(
                        sprite[s->owner].x-g_player[screenpeek].ps->posx,
                        sprite[s->owner].y-g_player[screenpeek].ps->posy);
                sqb =
                    getangle(
                        sprite[s->owner].x-t->x,
                        sprite[s->owner].y-t->y);

                if (klabs(getincangle(sqa,sqb)) > 512)
                    if (ldist(&sprite[s->owner],t) < ldist(&sprite[g_player[screenpeek].ps->i],&sprite[s->owner]))
                        t->xrepeat = t->yrepeat = 0;
            }
            continue;
        case BURNING__STATIC:
        case BURNING2__STATIC:
            if (sprite[s->owner].statnum == 10)
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
            if (camsprite >= 0 && hittype[OW].temp_data[0] == 1)
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
            if (getrendermode() >= 3 && usemodels && md_tilehasmodel(t->picnum,t->pal) >= 0 && !(spriteext[i].flags&SPREXT_NOTMD))
            {
                int v=getangle(t->xvel,t->zvel>>4);
                if (v>1023)v-=2048;
                spriteext[i].pitch=v;

                t->cstat &= ~4;
                break;
            }
#endif
            k = getangle(s->x-x,s->y-y);
            k = (((s->ang+3072+128-k)&2047)/170);
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
            k = getangle(s->x-x,s->y-y);
            if (T1 < 4)
                k = (((s->ang+3072+128-k)&2047)/170);
            else k = (((s->ang+3072+128-k)&2047)/170);

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
                t->cstat |= 2;
                if (screenpeek == myconnectindex && numplayers >= 2)
                {
                    t->x = omyx+mulscale16((int)(myx-omyx),smoothratio);
                    t->y = omyy+mulscale16((int)(myy-omyy),smoothratio);
                    t->z = omyz+mulscale16((int)(myz-omyz),smoothratio)+(40<<8);
                    t->ang = omyang+mulscale16((int)(((myang+1024-omyang)&2047)-1024),smoothratio);
                    t->sectnum = mycursectnum;
                }
            }

            if (ud.multimode > 1 && (display_mirror || screenpeek != p || s->owner == -1))
            {
                if (ud.showweapons && sprite[g_player[p].ps->i].extra > 0 && g_player[p].ps->curr_weapon > 0)
                {
                    memcpy((spritetype *)&tsprite[spritesortcnt],(spritetype *)t,sizeof(spritetype));

                    tsprite[spritesortcnt].statnum = TSPR_TEMP;

                    /*                    tsprite[spritesortcnt].yrepeat = (t->yrepeat>>3);
                                        if (t->yrepeat < 4) t->yrepeat = 4; */

                    tsprite[spritesortcnt].shade = t->shade;
                    tsprite[spritesortcnt].cstat = 0;
                    tsprite[spritesortcnt].pal = 0;

                    tsprite[spritesortcnt].picnum = (g_player[p].ps->curr_weapon==GROW_WEAPON?GROWSPRITEICON:weapon_sprites[g_player[p].ps->curr_weapon]);

                    if (s->owner >= 0)
                        tsprite[spritesortcnt].z = g_player[p].ps->posz-(12<<8);
                    else tsprite[spritesortcnt].z = s->z-(51<<8);

                    if (tsprite[spritesortcnt].picnum == HEAVYHBOMB)
                    {
                        tsprite[spritesortcnt].xrepeat = 10;
                        tsprite[spritesortcnt].yrepeat = 10;
                    }
                    else
                    {
                        tsprite[spritesortcnt].xrepeat = 16;
                        tsprite[spritesortcnt].yrepeat = 16;
                    }
                    spritesortcnt++;
                }

                if (g_player[p].sync->extbits & (1<<7) && !ud.pause_on)
                {
                    memcpy((spritetype *)&tsprite[spritesortcnt],(spritetype *)t,sizeof(spritetype));

                    tsprite[spritesortcnt].statnum = TSPR_TEMP;

                    tsprite[spritesortcnt].yrepeat = (t->yrepeat>>3);
                    if (t->yrepeat < 4) t->yrepeat = 4;

                    tsprite[spritesortcnt].cstat = 0;

                    tsprite[spritesortcnt].picnum = RESPAWNMARKERGREEN;

                    if (s->owner >= 0)
                        tsprite[spritesortcnt].z = g_player[p].ps->posz-(20<<8);
                    else tsprite[spritesortcnt].z = s->z-(96<<8);
                    tsprite[spritesortcnt].xrepeat = 32;
                    tsprite[spritesortcnt].yrepeat = 32;
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

                if (sector[t->sectnum].lotag == 2) k += 1795-1405;
                else if ((hittype[i].floorz-s->z) > (64<<8)) k += 60;

                t->picnum += k;
                t->pal = g_player[p].ps->palookup;

                goto PALONLY;
            }

            if (g_player[p].ps->on_crane == -1 && (sector[s->sectnum].lotag&0x7ff) != 1)
            {
                l = s->z-hittype[g_player[p].ps->i].floorz+(3<<8);
                if (l > 1024 && s->yrepeat > 32 && s->extra > 0)
                    s->yoffset = (signed char)(l/(s->yrepeat<<2));
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
                        t->owner = -1;
                        t->xrepeat = t->yrepeat = 0;
                        continue;
                    }

PALONLY:

            if (sector[sect].floorpal && sector[sect].floorpal < g_NumPalettes && !checkspriteflags(t->owner,SPRITE_FLAG_NOPAL))
                t->pal = sector[sect].floorpal;

            if (s->owner == -1) continue;

            if (t->z > hittype[i].floorz && t->xrepeat < 32)
                t->z = hittype[i].floorz;

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
            if (hittype[i].picnum == BLIMP && t->picnum == SCRAP1 && s->yvel >= 0)
                t->picnum = s->yvel;
            else t->picnum += T1;
            t->shade -= 6;

            if (sector[sect].floorpal && sector[sect].floorpal < g_NumPalettes && !checkspriteflags(t->owner,SPRITE_FLAG_NOPAL))
                t->pal = sector[sect].floorpal;
            break;

        case WATERBUBBLE__STATIC:
            if (sector[t->sectnum].floorpicnum == FLOORSLIME)
            {
                t->pal = 7;
                break;
            }
        default:
            if (sector[sect].floorpal && sector[sect].floorpal < g_NumPalettes && !checkspriteflags(t->owner,SPRITE_FLAG_NOPAL))
                t->pal = sector[sect].floorpal;
            break;
        }

        if (actorscrptr[s->picnum])
        {
            if (ud.angleinterpolation)
            {
                if (sprpos[i].ang != sprpos[i].oldang)
                    t->ang = (sprpos[i].oldang + (mulscale16((int)(sprpos[i].angdif),smoothratio) * sprpos[i].angdir)) & 2047;
                else
                    t->ang = sprpos[i].ang;
            }

            if (t4)
            {
                l = *(((intptr_t *)t4)+2); //For TerminX: was *(int *)(t4+8)

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

                if (hittype[i].dispicnum >= 0)
                    hittype[i].dispicnum = t->picnum;
            }
            else if (display_mirror == 1)
                t->cstat |= 4;
        }

        if (g_player[screenpeek].ps->heat_amount > 0 && g_player[screenpeek].ps->heat_on && (badguy(s) || checkspriteflags(t->owner,SPRITE_FLAG_NVG) || s->picnum == APLAYER || s->statnum == 13))
        {
            t->pal = 6;
            t->shade = 0;
        }

        if (s->statnum == 13 || badguy(s) || checkspriteflags(t->owner,SPRITE_FLAG_SHADOW) || (s->picnum == APLAYER && s->owner >= 0))
            if (t->statnum != TSPR_TEMP && s->picnum != EXPLOSION2 && s->picnum != HANGLIGHT && s->picnum != DOMELITE)
                if (s->picnum != HOTMEAT)
                {
                    if (hittype[i].dispicnum < 0)
                    {
                        hittype[i].dispicnum++;
                        continue;
                    }
                    else if (ud.shadows && spritesortcnt < (MAXSPRITESONSCREEN-2))
                    {
                        int daz,xrep,yrep;

                        if ((sector[sect].lotag&0xff) > 2 || s->statnum == 4 || s->statnum == 5 || s->picnum == DRONE || s->picnum == COMMANDER)
                            daz = sector[sect].floorz;
                        else
                            daz = hittype[i].floorz;

                        if ((s->z-daz) < (8<<8))
                            if (g_player[screenpeek].ps->posz < daz)
                            {
                                memcpy((spritetype *)&tsprite[spritesortcnt],(spritetype *)t,sizeof(spritetype));

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
                                    int ii;

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

        switch (dynamictostatic[s->picnum])
        {
        case LASERLINE__STATIC:
            if (sector[t->sectnum].lotag == 2) t->pal = 8;
            t->z = sprite[s->owner].z-(3<<8);
            if (lasermode == 2 && g_player[screenpeek].ps->heat_on == 0)
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
                //restorepalette = 1;   // JBF 20040101: why?
            }
            t->shade = -127;
            break;
        case FIRE__STATIC:
        case FIRE2__STATIC:
        case BURNING__STATIC:
        case BURNING2__STATIC:
            if (sprite[s->owner].picnum != TREE1 && sprite[s->owner].picnum != TREE2)
                t->z = sector[t->sectnum].floorz;
            t->shade = -127;
            break;
        case COOLEXPLOSION1__STATIC:
            t->shade = -127;
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
                    if (!hittype[s->owner].dispicnum)
                        t->picnum = hittype[i].temp_data[1];
                    else t->picnum = hittype[s->owner].dispicnum;
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

        hittype[i].dispicnum = t->picnum;
        if (sector[t->sectnum].floorpicnum == MIRROR)
            t->xrepeat = t->yrepeat = 0;
    }

    j = spritesortcnt-1;
    do
    {
        if (display_mirror) tsprite[j].statnum = TSPR_MIRROR;
        if (tsprite[j].owner < MAXSPRITES && tsprite[j].owner > 0 && spriteext[tsprite[j].owner].flags & SPREXT_TSPRACCESS)
        {
            spriteext[tsprite[j].owner].tspr = (spritetype *)&tsprite[j];
            OnEvent(EVENT_ANIMATESPRITES,tsprite[j].owner, myconnectindex, -1);
        }
    }
    while (j--);

    if (j < 0) return;

    if (display_mirror) tsprite[j].statnum = TSPR_MIRROR;
    if (tsprite[j].owner > 0 && tsprite[j].owner < MAXSPRITES && spriteext[tsprite[j].owner].flags & SPREXT_TSPRACCESS)
    {
        spriteext[tsprite[j].owner].tspr = (spritetype *)&tsprite[j];
        OnEvent(EVENT_ANIMATESPRITES,tsprite[j].owner, myconnectindex, -1);
    }
}
#ifdef _MSC_VER
//#pragma auto_inline()
#pragma optimize("",on)
#endif

char cheatstrings[][MAXCHEATLEN] =
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

void CheatGetInventory(void)
{
    SetGameVarID(g_iReturnVarID, 400, g_player[myconnectindex].ps->i, myconnectindex);
    OnEvent(EVENT_CHEATGETSTEROIDS, g_player[myconnectindex].ps->i, myconnectindex, -1);
    if (GetGameVarID(g_iReturnVarID, g_player[myconnectindex].ps->i, myconnectindex) >=0)
    {
        g_player[myconnectindex].ps->steroids_amount =
            GetGameVarID(g_iReturnVarID, g_player[myconnectindex].ps->i, myconnectindex);
    }

    SetGameVarID(g_iReturnVarID, 1200, g_player[myconnectindex].ps->i, myconnectindex);
    OnEvent(EVENT_CHEATGETHEAT, g_player[myconnectindex].ps->i, myconnectindex, -1);
    if (GetGameVarID(g_iReturnVarID, g_player[myconnectindex].ps->i, myconnectindex) >=0)
    {
        g_player[myconnectindex].ps->heat_amount     =
            GetGameVarID(g_iReturnVarID, g_player[myconnectindex].ps->i, myconnectindex);
    }

    SetGameVarID(g_iReturnVarID, 200, g_player[myconnectindex].ps->i, myconnectindex);
    OnEvent(EVENT_CHEATGETBOOT, g_player[myconnectindex].ps->i, myconnectindex, -1);
    if (GetGameVarID(g_iReturnVarID, g_player[myconnectindex].ps->i, myconnectindex) >=0)
    {
        g_player[myconnectindex].ps->boot_amount          =
            GetGameVarID(g_iReturnVarID, g_player[myconnectindex].ps->i, myconnectindex);
    }

    SetGameVarID(g_iReturnVarID, 100, g_player[myconnectindex].ps->i, myconnectindex);
    OnEvent(EVENT_CHEATGETSHIELD, g_player[myconnectindex].ps->i, myconnectindex, -1);
    if (GetGameVarID(g_iReturnVarID, g_player[myconnectindex].ps->i, myconnectindex) >=0)
    {
        g_player[myconnectindex].ps->shield_amount =
            GetGameVarID(g_iReturnVarID, g_player[myconnectindex].ps->i, myconnectindex);
    }

    SetGameVarID(g_iReturnVarID, 6400, g_player[myconnectindex].ps->i, myconnectindex);
    OnEvent(EVENT_CHEATGETSCUBA, g_player[myconnectindex].ps->i, myconnectindex, -1);
    if (GetGameVarID(g_iReturnVarID, g_player[myconnectindex].ps->i, myconnectindex) >=0)
    {
        g_player[myconnectindex].ps->scuba_amount =
            GetGameVarID(g_iReturnVarID, g_player[myconnectindex].ps->i, myconnectindex);
    }

    SetGameVarID(g_iReturnVarID, 2400, g_player[myconnectindex].ps->i, myconnectindex);
    OnEvent(EVENT_CHEATGETHOLODUKE, g_player[myconnectindex].ps->i, myconnectindex, -1);
    if (GetGameVarID(g_iReturnVarID, g_player[myconnectindex].ps->i, myconnectindex) >=0)
    {
        g_player[myconnectindex].ps->holoduke_amount =
            GetGameVarID(g_iReturnVarID, g_player[myconnectindex].ps->i, myconnectindex);
    }

    SetGameVarID(g_iReturnVarID, 1600, g_player[myconnectindex].ps->i, myconnectindex);
    OnEvent(EVENT_CHEATGETJETPACK, g_player[myconnectindex].ps->i, myconnectindex, -1);
    if (GetGameVarID(g_iReturnVarID, g_player[myconnectindex].ps->i, myconnectindex) >=0)
    {
        g_player[myconnectindex].ps->jetpack_amount =
            GetGameVarID(g_iReturnVarID, g_player[myconnectindex].ps->i, myconnectindex);
    }

    SetGameVarID(g_iReturnVarID, g_player[myconnectindex].ps->max_player_health, g_player[myconnectindex].ps->i, myconnectindex);
    OnEvent(EVENT_CHEATGETFIRSTAID, g_player[myconnectindex].ps->i, myconnectindex, -1);
    if (GetGameVarID(g_iReturnVarID, g_player[myconnectindex].ps->i, myconnectindex) >=0)
    {
        g_player[myconnectindex].ps->firstaid_amount =
            GetGameVarID(g_iReturnVarID, g_player[myconnectindex].ps->i, myconnectindex);
    }
}

signed char cheatbuf[MAXCHEATLEN],cheatbuflen;

static void cheats(void)
{
    short ch, i, j, k=0, weapon;
    static int z=0;
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
        Bstrcpy(cheatstrings[2],"scotty##");
        Bstrcpy(cheatstrings[6],"<RESERVED>");
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
                //                             FTA(46,g_player[myconnectindex].ps);
                return;
            }

            cheatbuf[cheatbuflen++] = ch;
            cheatbuf[cheatbuflen] = 0;
            //            KB_ClearKeysDown();

            if (cheatbuflen > MAXCHEATLEN)
            {
                g_player[myconnectindex].ps->cheat_phase = 0;
                return;
            }

            for (k=0;k < NUMCHEATCODES;k++)
            {
                for (j = 0;j<cheatbuflen;j++)
                {
                    if (cheatbuf[j] == cheatstrings[k][j] || (cheatstrings[k][j] == '#' && ch >= '0' && ch <= '9'))
                    {
                        if (cheatstrings[k][j+1] == 0) goto FOUNDCHEAT;
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

                    for (weapon = PISTOL_WEAPON;weapon < MAX_WEAPONS-j;weapon++)
                    {
                        addammo(weapon, g_player[myconnectindex].ps, g_player[myconnectindex].ps->max_ammo_amount[weapon]);
                        g_player[myconnectindex].ps->gotweapon[weapon]  = 1;
                    }

                    KB_FlushKeyBoardQueue();
                    g_player[myconnectindex].ps->cheat_phase = 0;
                    FTA(119,g_player[myconnectindex].ps);
                    return;

                case CHEAT_INVENTORY:
                    KB_FlushKeyBoardQueue();
                    g_player[myconnectindex].ps->cheat_phase = 0;
                    CheatGetInventory();
                    FTA(120,g_player[myconnectindex].ps);
                    g_player[myconnectindex].ps->cheat_phase = 0;
                    return;

                case CHEAT_KEYS:
                    g_player[myconnectindex].ps->got_access =  7;
                    KB_FlushKeyBoardQueue();
                    g_player[myconnectindex].ps->cheat_phase = 0;
                    FTA(121,g_player[myconnectindex].ps);
                    return;

                case CHEAT_DEBUG:
                    debug_on = 1-debug_on;
                    KB_FlushKeyBoardQueue();
                    g_player[myconnectindex].ps->cheat_phase = 0;

                    dumpdebugdata();
                    Bsprintf(tempbuf,"GAMEVARS DUMPED TO DEBUG.CON");
                    adduserquote(tempbuf);
                    Bsprintf(tempbuf,"MAP DUMPED TO DEBUG.MAP");
                    adduserquote(tempbuf);
                    break;

                case CHEAT_CLIP:
                    ud.clipping = 1-ud.clipping;
                    KB_FlushKeyBoardQueue();
                    g_player[myconnectindex].ps->cheat_phase = 0;
                    FTA(112+ud.clipping,g_player[myconnectindex].ps);
                    return;

                case CHEAT_RESERVED2:
                    g_player[myconnectindex].ps->gm = MODE_EOL;
                    g_player[myconnectindex].ps->cheat_phase = 0;
                    KB_FlushKeyBoardQueue();
                    return;

                case CHEAT_ALLEN:
                    FTA(79,g_player[myconnectindex].ps);
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

                        hittype[g_player[myconnectindex].ps->i].temp_data[0] = 0;
                        hittype[g_player[myconnectindex].ps->i].temp_data[1] = 0;
                        hittype[g_player[myconnectindex].ps->i].temp_data[2] = 0;
                        hittype[g_player[myconnectindex].ps->i].temp_data[3] = 0;
                        hittype[g_player[myconnectindex].ps->i].temp_data[4] = 0;
                        hittype[g_player[myconnectindex].ps->i].temp_data[5] = 0;

                        sprite[g_player[myconnectindex].ps->i].hitag = 0;
                        sprite[g_player[myconnectindex].ps->i].lotag = 0;
                        sprite[g_player[myconnectindex].ps->i].pal = g_player[myconnectindex].ps->palookup;

                        FTA(17,g_player[myconnectindex].ps);
                    }
                    else
                    {
                        ud.god = 0;
                        sprite[g_player[myconnectindex].ps->i].extra = g_player[myconnectindex].ps->max_player_health;
                        hittype[g_player[myconnectindex].ps->i].extra = -1;
                        g_player[myconnectindex].ps->last_extra = g_player[myconnectindex].ps->max_player_health;
                        FTA(18,g_player[myconnectindex].ps);
                    }

                    sprite[g_player[myconnectindex].ps->i].extra = g_player[myconnectindex].ps->max_player_health;
                    hittype[g_player[myconnectindex].ps->i].extra = 0;
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

                        hittype[g_player[myconnectindex].ps->i].temp_data[0] = 0;
                        hittype[g_player[myconnectindex].ps->i].temp_data[1] = 0;
                        hittype[g_player[myconnectindex].ps->i].temp_data[2] = 0;
                        hittype[g_player[myconnectindex].ps->i].temp_data[3] = 0;
                        hittype[g_player[myconnectindex].ps->i].temp_data[4] = 0;
                        hittype[g_player[myconnectindex].ps->i].temp_data[5] = 0;

                        sprite[g_player[myconnectindex].ps->i].hitag = 0;
                        sprite[g_player[myconnectindex].ps->i].lotag = 0;
                        sprite[g_player[myconnectindex].ps->i].pal = g_player[myconnectindex].ps->palookup;
                        Bstrcpy(fta_quotes[122],"Scream for me, Long Beach!");
                        FTA(122,g_player[myconnectindex].ps);
                        CheatGetInventory();
                        for (weapon = PISTOL_WEAPON;weapon < MAX_WEAPONS;weapon++)
                            g_player[myconnectindex].ps->gotweapon[weapon]  = 1;

                        for (weapon = PISTOL_WEAPON;
                                weapon < (MAX_WEAPONS);
                                weapon++)
                            addammo(weapon, g_player[myconnectindex].ps, g_player[myconnectindex].ps->max_ammo_amount[weapon]);
                        g_player[myconnectindex].ps->got_access = 7;
                    }
                    else
                    {
                        sprite[g_player[myconnectindex].ps->i].extra = g_player[myconnectindex].ps->max_player_health;
                        hittype[g_player[myconnectindex].ps->i].extra = -1;
                        g_player[myconnectindex].ps->last_extra = g_player[myconnectindex].ps->max_player_health;
                        FTA(18,g_player[myconnectindex].ps);
                    }

                    sprite[g_player[myconnectindex].ps->i].extra = g_player[myconnectindex].ps->max_player_health;
                    hittype[g_player[myconnectindex].ps->i].extra = 0;
                    g_player[myconnectindex].ps->cheat_phase = 0;
                    KB_FlushKeyBoardQueue();
                    return;

                case CHEAT_STUFF:

                    j = 0;

                    if (VOLUMEONE)
                        j = 6;

                    for (weapon = PISTOL_WEAPON;weapon < MAX_WEAPONS-j;weapon++)
                        g_player[myconnectindex].ps->gotweapon[weapon]  = 1;

                    for (weapon = PISTOL_WEAPON;
                            weapon < (MAX_WEAPONS-j);
                            weapon++)
                        addammo(weapon, g_player[myconnectindex].ps, g_player[myconnectindex].ps->max_ammo_amount[weapon]);
                    CheatGetInventory();
                    g_player[myconnectindex].ps->got_access =              7;
                    FTA(5,g_player[myconnectindex].ps);
                    g_player[myconnectindex].ps->cheat_phase = 0;

                    //                        FTA(21,g_player[myconnectindex].ps);
                    g_player[myconnectindex].ps->cheat_phase = 0;
                    KB_FlushKeyBoardQueue();
                    g_player[myconnectindex].ps->inven_icon = 1;
                    return;

                case CHEAT_SCOTTY:
                case CHEAT_SKILL:
                    if (k == CHEAT_SCOTTY)
                    {
                        i = Bstrlen(cheatstrings[k])-3+VOLUMEONE;
                        if (!consolecheat)
                        {
                            // JBF 20030914
                            short volnume,levnume;
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

                            if ((VOLUMEONE && volnume > 0) || volnume > num_volumes-1 ||
                                    levnume >= MAXLEVELS || map[volnume*MAXLEVELS+levnume].filename == NULL)
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
                        i = Bstrlen(cheatstrings[k])-1;
                        ud.m_player_skill = ud.player_skill = cheatbuf[i] - '1';
                    }
                    if (numplayers > 1 && myconnectindex == connecthead)
                        mpchangemap(ud.m_volume_number,ud.m_level_number);
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
                        cameradist = 0;
                        cameraclock = totalclock;
                    }
                    FTA(22,g_player[myconnectindex].ps);
                    g_player[myconnectindex].ps->cheat_phase = 0;
                    KB_FlushKeyBoardQueue();
                    return;

                case CHEAT_TIME:

                    FTA(21,g_player[myconnectindex].ps);
                    g_player[myconnectindex].ps->cheat_phase = 0;
                    KB_FlushKeyBoardQueue();
                    return;

                case CHEAT_UNLOCK:
                    if (VOLUMEONE) return;

                    for (i=numsectors-1;i>=0;i--) //Unlock
                    {
                        j = sector[i].lotag;
                        if (j == -1 || j == 32767) continue;
                        if ((j & 0x7fff) > 2)
                        {
                            if (j&(0xffff-16384))
                                sector[i].lotag &= (0xffff-16384);
                            operatesectors(i,g_player[myconnectindex].ps->i);
                        }
                    }
                    operateforcefields(g_player[myconnectindex].ps->i,-1);

                    FTA(100,g_player[myconnectindex].ps);
                    g_player[myconnectindex].ps->cheat_phase = 0;
                    KB_FlushKeyBoardQueue();
                    return;

                case CHEAT_CASHMAN:
                    ud.cashman = 1-ud.cashman;
                    KB_ClearKeyDown(sc_N);
                    g_player[myconnectindex].ps->cheat_phase = 0;
                    return;

                case CHEAT_ITEMS:
                    CheatGetInventory();
                    g_player[myconnectindex].ps->got_access =              7;
                    FTA(5,g_player[myconnectindex].ps);
                    g_player[myconnectindex].ps->cheat_phase = 0;
                    KB_FlushKeyBoardQueue();
                    return;

                case CHEAT_SHOWMAP: // SHOW ALL OF THE MAP TOGGLE;
                    ud.showallmap = 1-ud.showallmap;
                    if (ud.showallmap)
                    {
                        for (i=0;i<(MAXSECTORS>>3);i++)
                            show2dsector[i] = 255;
                        for (i=0;i<(MAXWALLS>>3);i++)
                            show2dwall[i] = 255;
                        FTA(111,g_player[myconnectindex].ps);
                    }
                    else
                    {
                        for (i=0;i<(MAXSECTORS>>3);i++)
                            show2dsector[i] = 0;
                        for (i=0;i<(MAXWALLS>>3);i++)
                            show2dwall[i] = 0;
                        FTA(1,g_player[myconnectindex].ps);
                    }
                    g_player[myconnectindex].ps->cheat_phase = 0;
                    KB_FlushKeyBoardQueue();
                    return;

                case CHEAT_TODD:
                    FTA(99,g_player[myconnectindex].ps);
                    g_player[myconnectindex].ps->cheat_phase = 0;
                    KB_FlushKeyBoardQueue();
                    return;

                case CHEAT_RATE:
                    ud.tickrate = !ud.tickrate;
                    g_player[myconnectindex].ps->cheat_phase = 0;
                    KB_FlushKeyBoardQueue();
                    return;

                case CHEAT_BETA:
                    FTA(105,g_player[myconnectindex].ps);
                    KB_ClearKeyDown(sc_H);
                    g_player[myconnectindex].ps->cheat_phase = 0;
                    KB_FlushKeyBoardQueue();
                    return;

                case CHEAT_HYPER:
                    g_player[myconnectindex].ps->steroids_amount = 399;
                    g_player[myconnectindex].ps->heat_amount = 1200;
                    g_player[myconnectindex].ps->cheat_phase = 0;
                    FTA(37,g_player[myconnectindex].ps);
                    KB_FlushKeyBoardQueue();
                    return;

                case CHEAT_MONSTERS:
                {
                    char *s[] = { "ON", "OFF", "ON" };

                    actor_tog++;
                    if (actor_tog == 3) actor_tog = 0;
                    g_player[screenpeek].ps->cheat_phase = 0;
                    Bsprintf(fta_quotes[122],"MONSTERS: %s",s[(unsigned char)actor_tog]);
                    FTA(122,g_player[myconnectindex].ps);
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
        if (KB_KeyPressed((unsigned char)cheatkey[0]))
        {
            if (g_player[myconnectindex].ps->cheat_phase >= 0 && numplayers < 2 && ud.recstat == 0)
            {
                if (cheatkey[0] == cheatkey[1])
                    KB_ClearKeyDown((unsigned char)cheatkey[0]);
                g_player[myconnectindex].ps->cheat_phase = -1;
            }
        }

        if (KB_KeyPressed((unsigned char)cheatkey[1]))
        {
            if (g_player[myconnectindex].ps->cheat_phase == -1)
            {
                if (ud.player_skill == 4)
                {
                    FTA(22,g_player[myconnectindex].ps);
                    g_player[myconnectindex].ps->cheat_phase = 0;
                }
                else
                {
                    g_player[myconnectindex].ps->cheat_phase = 1;
                    //                    FTA(25,g_player[myconnectindex].ps);
                    cheatbuflen = 0;
                }
                KB_FlushKeyboardQueue();
            }
            else if (g_player[myconnectindex].ps->cheat_phase != 0)
            {
                g_player[myconnectindex].ps->cheat_phase = 0;
                KB_ClearKeyDown((unsigned char)cheatkey[0]);
                KB_ClearKeyDown((unsigned char)cheatkey[1]);
            }
        }
    }
}

static void nonsharedkeys(void)
{
    int i,ch;
    int j;

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
            tempbuf[0] = 18;
            tempbuf[1] = 0;
            tempbuf[2] = myconnectindex;
            tempbuf[3] = (KB_UnBoundKeyPressed(sc_F1) || ud.autovote?ud.autovote-1:0);

            for (i=connecthead;i >= 0;i=connectpoint2[i])
            {
                if (i != myconnectindex) sendpacket(i,tempbuf,4);
                if ((!networkmode) && (myconnectindex != connecthead)) break; //slaves in M/S mode only send to master
            }
            adduserquote("VOTE CAST");
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
                    sound(THUD);
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
                setstatusbarscale(ud.statusbarscale);
            }
            vscrn();
        }

        if (BUTTON(gamefunc_Shrink_Screen))
        {
            CONTROL_ClearButton(gamefunc_Shrink_Screen);
            if (!SHIFTS_IS_PRESSED)
            {
                if (ud.screen_size < 64) sound(THUD);
                if (getrendermode() >= 3 && ud.screen_size == 8 && ud.statusbarmode == 1)
                    ud.statusbarmode = 0;
                else ud.screen_size += 4;
            }
            else
            {
                ud.statusbarscale -= 4;
                if (ud.statusbarscale < 37)
                    ud.statusbarscale = 37;
                setstatusbarscale(ud.statusbarscale);
            }
            vscrn();
        }
    }

    if (g_player[myconnectindex].ps->cheat_phase == 1 || (g_player[myconnectindex].ps->gm&(MODE_MENU|MODE_TYPE))) return;

    if (BUTTON(gamefunc_See_Coop_View) && (GTFLAGS(GAMETYPE_FLAG_COOPVIEW) || ud.recstat == 2))
    {
        CONTROL_ClearButton(gamefunc_See_Coop_View);
        screenpeek = connectpoint2[screenpeek];
        if (screenpeek == -1) screenpeek = connecthead;
        restorepalette = 1;
    }

    if (ud.multimode > 1 && BUTTON(gamefunc_Show_Opponents_Weapon))
    {
        CONTROL_ClearButton(gamefunc_Show_Opponents_Weapon);
        ud.showweapons = 1-ud.showweapons;
        ud.config.ShowOpponentWeapons = ud.showweapons;
        FTA(82-ud.showweapons,g_player[screenpeek].ps);
    }

    if (BUTTON(gamefunc_Toggle_Crosshair))
    {
        CONTROL_ClearButton(gamefunc_Toggle_Crosshair);
        ud.crosshair = !ud.crosshair;
        if (ud.crosshair)
            FTA(20,g_player[screenpeek].ps);
        else FTA(21,g_player[screenpeek].ps);
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
        FTA(83+ud.scrollmode,g_player[myconnectindex].ps);
    }

    if (SHIFTS_IS_PRESSED || ALT_IS_PRESSED)
    {
        i = 0;
        if (KB_UnBoundKeyPressed(sc_F1))
        {
            KB_ClearKeyDown(sc_F1);
            i = 1;
        }
        if (KB_UnBoundKeyPressed(sc_F2))
        {
            KB_ClearKeyDown(sc_F2);
            i = 2;
        }
        if (KB_UnBoundKeyPressed(sc_F3))
        {
            KB_ClearKeyDown(sc_F3);
            i = 3;
        }
        if (KB_UnBoundKeyPressed(sc_F4))
        {
            KB_ClearKeyDown(sc_F4);
            i = 4;
        }
        if (KB_UnBoundKeyPressed(sc_F5))
        {
            KB_ClearKeyDown(sc_F5);
            i = 5;
        }
        if (KB_UnBoundKeyPressed(sc_F6))
        {
            KB_ClearKeyDown(sc_F6);
            i = 6;
        }
        if (KB_UnBoundKeyPressed(sc_F7))
        {
            KB_ClearKeyDown(sc_F7);
            i = 7;
        }
        if (KB_UnBoundKeyPressed(sc_F8))
        {
            KB_ClearKeyDown(sc_F8);
            i = 8;
        }
        if (KB_UnBoundKeyPressed(sc_F9))
        {
            KB_ClearKeyDown(sc_F9);
            i = 9;
        }
        if (KB_UnBoundKeyPressed(sc_F10))
        {
            KB_ClearKeyDown(sc_F10);
            i = 10;
        }

        if (i)
        {
            if (SHIFTS_IS_PRESSED)
            {
                if (i == 5 && g_player[myconnectindex].ps->fta > 0 && g_player[myconnectindex].ps->ftq == 26)
                {
                    i = (VOLUMEALL?MAXVOLUMES*MAXLEVELS:6);
                    music_select++;
                    while ((map[(unsigned char)music_select].musicfn == NULL) && music_select < i)
                        music_select++;
                    if (music_select == i)
                        music_select = 0;
                    if (map[(unsigned char)music_select].musicfn != NULL)
                    {
                        if (playmusic(&map[(unsigned char)music_select].musicfn[0],music_select))
                            Bsprintf(fta_quotes[26],"PLAYING %s",&map[(unsigned char)music_select].musicfn1[0]);
                        else
                            Bsprintf(fta_quotes[26],"PLAYING %s",&map[(unsigned char)music_select].musicfn[0]);
                        FTA(26,g_player[myconnectindex].ps);
                    }
                    return;
                }

                adduserquote(ud.ridecule[i-1]);

                ch = 0;

                tempbuf[ch] = 4;
                tempbuf[ch+1] = 255;
                tempbuf[ch+2] = 0;
                Bstrcat(tempbuf+2,ud.ridecule[i-1]);

                i = 2+strlen(ud.ridecule[i-1]);

                if (ud.multimode > 1)
                    for (ch=connecthead;ch>=0;ch=connectpoint2[ch])
                    {
                        if (ch != myconnectindex) sendpacket(ch,tempbuf,i);
                        if ((!networkmode) && (myconnectindex != connecthead)) break; //slaves in M/S mode only send to master
                    }

                pus = NUMPAGES;
                pub = NUMPAGES;

                return;

            }

            if (ud.lockout == 0)
                if (ud.config.SoundToggle && ALT_IS_PRESSED && (RTS_NumSounds() > 0) && rtsplaying == 0 && (ud.config.VoiceToggle & 1))
                {
                    rtsptr = (char *)RTS_GetSound(i-1);
                    if (*rtsptr == 'C')
                        FX_PlayVOC3D(rtsptr,0,0,0,255,-i);
                    else FX_PlayWAV3D(rtsptr,0,0,0,255,-i);

                    rtsplaying = 7;

                    if (ud.multimode > 1)
                    {
                        tempbuf[0] = 7;
                        tempbuf[1] = i;

                        for (ch=connecthead;ch>=0;ch=connectpoint2[ch])
                        {
                            if (ch != myconnectindex) sendpacket(ch,tempbuf,2);
                            if ((!networkmode) && (myconnectindex != connecthead)) break; //slaves in M/S mode only send to master
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
                vscrn();
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

                if (movesperpacket == 4 && connecthead != myconnectindex)
                    return;

FAKE_F2:
                if (sprite[g_player[myconnectindex].ps->i].extra <= 0)
                {
                    FTA(118,g_player[myconnectindex].ps);
                    return;
                }
                cmenu(350);
                screencapt = 1;
                displayrooms(myconnectindex,65536);
                //savetemp("duke3d.tmp",waloff[TILE_SAVESHOT],160*100);
                screencapt = 0;
                FX_StopAllSounds();
                clearsoundlocks();

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

                if (movesperpacket == 4 && connecthead != myconnectindex)
                    return;
FAKE_F3:
                cmenu(300);
                FX_StopAllSounds();
                clearsoundlocks();

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
            clearsoundlocks();

            g_player[myconnectindex].ps->gm |= MODE_MENU;
            if (ud.multimode < 2 && ud.recstat != 2)
            {
                ready2send = 0;
                totalclock = ototalclock;
            }
            cmenu(701);

        }

        if ((KB_UnBoundKeyPressed(sc_F6) || doquicksave == 1) && (g_player[myconnectindex].ps->gm&MODE_GAME))
        {
            KB_ClearKeyDown(sc_F6);
            doquicksave = 0;

            if (movesperpacket == 4 && connecthead != myconnectindex)
                return;

            if (lastsavedpos == -1) goto FAKE_F2;

            KB_FlushKeyboardQueue();

            if (sprite[g_player[myconnectindex].ps->i].extra <= 0)
            {
                FTA(118,g_player[myconnectindex].ps);
                return;
            }
            screencapt = 1;
            displayrooms(myconnectindex,65536);
            //savetemp("duke3d.tmp",waloff[TILE_SAVESHOT],160*100);
            screencapt = 0;
            if (lastsavedpos >= 0)
            {
                /*                inputloc = Bstrlen(&ud.savegame[lastsavedpos][0]);
                                current_menu = 360+lastsavedpos;
                                probey = lastsavedpos; */
                if (ud.multimode > 1)
                    saveplayer(-1-(lastsavedpos));
                else saveplayer(lastsavedpos);
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
                cameradist = 0;
                cameraclock = totalclock;
            }
            FTA(109+g_player[myconnectindex].ps->over_shoulder_on,g_player[myconnectindex].ps);
        }

        if (KB_UnBoundKeyPressed(sc_F5) && ud.config.MusicDevice >= 0)
        {
            KB_ClearKeyDown(sc_F5);
            if (map[(unsigned char)music_select].musicfn1 != NULL)
                Bstrcpy(fta_quotes[26],&map[(unsigned char)music_select].musicfn1[0]);
            else if (map[(unsigned char)music_select].musicfn != NULL)
            {
                Bstrcpy(fta_quotes[26],&map[(unsigned char)music_select].musicfn[0]);
                Bstrcat(fta_quotes[26],".  USE SHIFT-F5 TO CHANGE.");
            }
            else fta_quotes[26][0] = '\0';
            FTA(26,g_player[myconnectindex].ps);
        }

        if (KB_UnBoundKeyPressed(sc_F8))
        {
            KB_ClearKeyDown(sc_F8);
            ud.fta_on = !ud.fta_on;
            if (ud.fta_on) FTA(23,g_player[myconnectindex].ps);
            else
            {
                ud.fta_on = 1;
                FTA(24,g_player[myconnectindex].ps);
                ud.fta_on = 0;
            }
        }

        if ((KB_UnBoundKeyPressed(sc_F9) || doquicksave == 2) && (g_player[myconnectindex].ps->gm&MODE_GAME))
        {
            KB_ClearKeyDown(sc_F9);
            doquicksave = 0;

            if (movesperpacket == 4 && myconnectindex != connecthead)
                return;

            if (lastsavedpos == -1) goto FAKE_F3;

            if (lastsavedpos >= 0)
            {
                KB_FlushKeyboardQueue();
                KB_ClearKeysDown();
                FX_StopAllSounds();

                if (ud.multimode > 1)
                {
                    loadplayer(-1-lastsavedpos);
                    g_player[myconnectindex].ps->gm = MODE_GAME;
                }
                else
                {
                    i = loadplayer(lastsavedpos);
                    if (i == 0)
                        g_player[myconnectindex].ps->gm = MODE_GAME;
                }
            }
        }

        if (KB_UnBoundKeyPressed(sc_F10))
        {
            KB_ClearKeyDown(sc_F10);
            cmenu(500);
            FX_StopAllSounds();
            clearsoundlocks();
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
        vscrn();
    }

    if (BUTTON(gamefunc_AutoRun))
    {
        CONTROL_ClearButton(gamefunc_AutoRun);
        ud.auto_run = 1-ud.auto_run;
        ud.config.RunMode = ud.auto_run;
        FTA(85+ud.auto_run,g_player[myconnectindex].ps);
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
        restorepalette = 1;
        vscrn();
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
            if (ud.brightness < 40) FTA(29 + (ud.brightness>>3) ,g_player[myconnectindex].ps);
            else if (ud.brightness < 80) FTA(96 + (ud.brightness>>3) - 5,g_player[myconnectindex].ps);
        }
    */
    if (KB_UnBoundKeyPressed(sc_F11))
    {
        KB_ClearKeyDown(sc_F11);
        cmenu(232);
        FX_StopAllSounds();
        clearsoundlocks();
        g_player[myconnectindex].ps->gm |= MODE_MENU;
        if (ud.multimode < 2 && ud.recstat != 2)
        {
            ready2send = 0;
            totalclock = ototalclock;
        }
    }
}

static void comlinehelp(void)
{
    char *s = "Usage: eduke32 [FILES] [OPTIONS]\n"
              "Example: eduke32 -q4 -a -m -tx -map nukeland.map\n\n"
              "FILES\n"
              "can be *.grp, *.zip, *.con, *.def\n"
              ""
              "OPTIONS\n"
              "-NUM\t\tLoad and run a game from slot NUM (0-9)\n"
              "-a\t\tUse fake player AI (fake multiplayer only)\n"
              "-cNUM\t\tUse MP mode NUM, 1 = DukeMatch(spawn), 2 = Coop, 3 = Dukematch(no spawn)\n"
              "-cfg FILE\tUse configuration file FILE\n"
              "-dFILE\t\tStart to play demo FILE\n"
              /*              "-fNUM\t\tSend fewer packets in multiplayer (1, 2, 4) (deprecated)\n" */
              "-game_dir DIR\tSee -j\n"
              "-gFILE, -grp FILE\tUse extra group file FILE\n"
              "-gamegrp FILE\tUse FILE as a default GRP\n"
              "-nam\t\tRun in Nam-compatible mode\n"
              "-ww2gi\t\tRun in ww2gi-compatible mode\n"
              "-hFILE\t\tUse definitions file FILE\n"
              "-iNUM\t\tUse networking mode NUM (1/0) (multiplayer only) (default == 1)\n"
              "-jDIR\t\tAdds DIR to the file path stack\n"
              "-lNUM\t\tWarp to level NUM (1-11), see -v\n"
              "-m\t\tDisable monsters\n"
              "-map FILE\tUse user map FILE\n"
              "-name NAME\tUse NAME as multiplayer name\n"
              "-nD\t\tDump default gamevars to gamevars.txt\n"
              "-net PARAMETERS\tEnable network play (see documentation for parameters)\n"
              "-nm\t\tDisable music\n"
              "-ns\t\tDisable sound\n"
              "-qNUM\t\tUse NUM players for fake multiplayer (2-8)\n"
              "-r\t\tRecord demo\n"
              "-rmnet FILE\tUse FILE for network play configuration (see documentation)\n"
              "-keepaddr\n"
              "-stun\n"
              "-w\t\tShow coordinates"
              "-noautoload\tDo not use the autoload directory\n"
              "-nologo\t\tSkip the logo anim\n"
              "-cachesize SIZE\tUse SIZE Kb for cache\n"
              "-sNUM\t\tUse skill level NUM (1-4)\n"
#if defined RENDERTYPEWIN || (defined RENDERTYPESDL && !defined __APPLE__ && defined HAVE_GTK2)
              "-setup\t\tDisplays the configuration dialog\n"

              "-nosetup\t\t\n"
#endif
              "-tNUM\t\tUse respawn mode NUM, 1 = Monsters, 2 = Items, 3 = Inventory, x = All\n"
              "-u#########\tUser's favorite weapon order (default: 3425689071)\n"
#if !defined(_WIN32)
              "-usecwd\t\tRead game data and configuration file from working directory\n"
#endif
              "-sloppycmd\t\tAllows EDuke to execute unsafe commands. (for compatibility only)\n"
              "-vNUM\t\tWarp to volume NUM (1-4), see -l\n"
              "-xFILE\t\tLoad CON script FILE (default EDUKE.CON/GAME.CON)\n"
              "-zNUM,\n-condebug\tLine-by-line CON compilation debugging, NUM is verbosity\n"
              "\n-?, -help, --help\tDisplay this help message and exit"
              ;
#if defined RENDERTYPEWIN
    Bsprintf(tempbuf,HEAD2 " %s",s_builddate);
    wm_msgbox(tempbuf,s);
#else
    initprintf("%s\n",s);
#endif
}

#ifndef RANCID_NETWORKING
static signed int rancid_players = 0;
static char rancid_ip_strings[MAXPLAYERS][32], rancid_local_port_string[8];

extern int getexternaladdress(char *buffer, const char *host, int port);

static int load_rancid_net(const char *fn)
{
    int tokn;
    char *cmdtokptr;

    tokenlist rancidtokens[] =
    {
        { "interface",       T_INTERFACE       },
        { "mode",            T_MODE            },
        { "allow",           T_ALLOW           },
    };

    scriptfile *script;

    script = scriptfile_fromfile((char *)fn);
    if (!script) return -1;

    while (1)
    {
        tokn = getatoken(script,rancidtokens,sizeof(rancidtokens)/sizeof(tokenlist));
        cmdtokptr = script->ltextptr;
        switch (tokn)
        {
        case T_INTERFACE:
        {
            char *ip;

            if (scriptfile_getstring(script,&ip)) break;
            Bstrcpy(rancid_ip_strings[MAXPLAYERS-1],ip);
            Bstrcpy(rancid_ip_strings[rancid_players++],ip);
            if (strtok(ip,":"))
            {
                char *p = strtok(NULL,":");

                if (p != NULL)
                {
                    if (atoi(p) > 1024)
                        Bsprintf(rancid_local_port_string,"-p %s",p);
                }
            }
        }
        break;
        case T_MODE:
        {
            char *mode;

            if (scriptfile_getstring(script,&mode)) break;
        }
        break;
        case T_ALLOW:
        {
            char *ip;

            if (scriptfile_getstring(script,&ip)) break;
            Bstrcpy(rancid_ip_strings[rancid_players++],ip);
        }
        break;
        case T_EOF:
            return(0);
        default:
            break;
        }
    }

    scriptfile_close(script);
    scriptfile_clearsymbols();

    return 0;
}

static inline int stringsort(const char *p1, const char *p2)
{
    return Bstrcmp(&p1[0],&p2[0]);
}

// Not supported with the enet network backend currently
static void setup_rancid_net(const char *fn)
{
    int i;

    if (load_rancid_net(fn) != -1)
    {
        char tmp[32];

        if (!Bstrlen(rancid_ip_strings[MAXPLAYERS-1])||!Bstrlen(rancid_ip_strings[1]))
        {
            if (!Bstrlen(rancid_ip_strings[MAXPLAYERS-1]))
                initprintf("rmnet: Interface not defined!\n");
            if (!Bstrlen(rancid_ip_strings[1]))
                initprintf("rmnet: No peers configured!\n");
            gameexit("Malformed network configuration file!");
            return;
        }

        if (g_KeepAddr == 0)
        {
            for (i=0;i<rancid_players;i++)
            {
                if (Bstrcmp(rancid_ip_strings[i],rancid_ip_strings[MAXPLAYERS-1]))
                {
                    Bstrncpy(tempbuf,rancid_ip_strings[i], 8);
                    Bstrcpy(tmp,strtok(tempbuf,"."));
                    if (Bstrcmp(tmp,"10") == 0)
                    {
                        i = 0;
                        break;
                    }
                    else if (Bstrcmp(tmp,"192") == 0)
                    {
                        Bstrcpy(tmp,strtok(NULL,"."));
                        if (Bstrcmp(tmp,"168") == 0)
                        {
                            i = 0;
                            break;
                        }
                    }
                    else if (Bstrcmp(tmp,"172") == 0)
                    {
                        Bstrcpy(tmp,strtok(NULL,"."));
                        if (Bstrcmp(tmp,"16") == 0)
                        {
                            i = 0;
                            break;
                        }
                    }
                    else if (Bstrcmp(tmp,"169") == 0)
                    {
                        Bstrcpy(tmp,strtok(NULL,"."));
                        if (Bstrcmp(tmp,"254") == 0)
                        {
                            i = 0;
                            break;
                        }
                    }
                }
            }

            Bstrcpy(tempbuf,rancid_ip_strings[MAXPLAYERS-1]);
            Bstrcpy(tmp,strtok(tempbuf,"."));
            if (i == rancid_players && ((Bstrcmp(tmp,"192") == 0) || (Bstrcmp(tmp,"172") == 0) || (Bstrcmp(tmp,"169") == 0) || (Bstrcmp(tmp,"10") == 0)))
            {
                int ii = getexternaladdress(tempbuf, "checkip.dyndns.org", 8245);
                if (!ii) ii = getexternaladdress(tempbuf, "checkip.dyndns.org", 80);
                if (ii)
                {
                    if (tempbuf[0])
                    {
                        for (i=0;i<rancid_players;i++)
                        {
                            if (Bstrcmp(rancid_ip_strings[i],rancid_ip_strings[MAXPLAYERS-1]) == 0)
                            {
                                Bstrcpy(rancid_ip_strings[MAXPLAYERS-1],tempbuf);
                                Bstrcpy(rancid_ip_strings[i],tempbuf);
                            }
                        }
                    }
                }
                else initprintf("rmnet: Unable to get external interface address!  Expect problems...\n");
            }
        }
        qsort((char *)rancid_ip_strings, rancid_players, sizeof(rancid_ip_strings[0]), (int(*)(const void*,const void*))stringsort);

        networkmode = 1;
        netparamcount = rancid_players;
        if (rancid_local_port_string[0] == '-')
            netparamcount++;
        netparam = (char **)Bcalloc(netparamcount, sizeof(char **));

        for (i=0;i<rancid_players;i++)
        {
            if (Bstrcmp(rancid_ip_strings[i],rancid_ip_strings[MAXPLAYERS-1]) == 0)
                Bsprintf(rancid_ip_strings[i],"/n1");
            netparam[i] = (char *)&rancid_ip_strings[i];
        }
        if (i != netparamcount)
            netparam[i] = (char *)&rancid_local_port_string;
    }
}
#endif


static CACHE1D_FIND_REC *finddirs=NULL, *findfiles=NULL, *finddirshigh=NULL, *findfileshigh=NULL;
static int numdirs=0, numfiles=0;
static int currentlist=0;

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

static void autoloadgrps(const char *fn)
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

static char *makename(char *destname, char *OGGname, char *origname)
{
    if (!origname)
        return destname;

    if (destname)
        Bfree(destname);
    destname=Bcalloc(Bstrlen(OGGname)+Bstrlen(origname)+1,sizeof(char));
    if (!destname)
        return NULL;

    Bstrcpy(destname,(*OGGname)?OGGname:origname);

    if (*OGGname&&OGGname[Bstrlen(OGGname)-1]=='/')
    {
        while (*origname=='/')
            origname++;
        Bstrcat(destname,origname);
    }

    OGGname=Bstrchr(destname,'.');
    if (OGGname)
        Bstrcpy(OGGname,".ogg");
    else Bstrcat(destname,".ogg");

    return destname;
}

static int AL_DefineSound(int ID,char *name)
{
    if (ID>=MAXSOUNDS)
        return 1;
    g_sounds[ID].filename1=makename(g_sounds[ID].filename1,name,g_sounds[ID].filename);
//    initprintf("(%s)(%s)(%s)\n",g_sounds[ID].filename1,name,g_sounds[ID].filename);
//    loadsound(ID);
    return 0;
}

static int AL_DefineMusic(char *ID,char *name)
{
    int lev,ep,sel;char b1,b2;

    if (!ID)
        return 1;
    if (!Bstrcmp(ID,"intro"))
    {
        sel=MAXVOLUMES*MAXLEVELS;  ID=env_music_fn[0];
    }
    else if (!Bstrcmp(ID,"briefing"))
    {
        sel=MAXVOLUMES*MAXLEVELS+1;
        ID=env_music_fn[1];
    }
    else if (!Bstrcmp(ID,"loading"))
    {
        sel=MAXVOLUMES*MAXLEVELS+2;
        ID=env_music_fn[2];
    }
    else
    {
        sscanf(ID,"%c%d%c%d",&b1,&ep,&b2,&lev);
        lev--;
        ep--;
        if (toupper(b1)!='E'||toupper(b2)!='L'||lev>=MAXLEVELS||ep>=MAXVOLUMES)
            return 1;
        sel=(ep*MAXLEVELS)+lev;
        ID=map[sel].musicfn;
    }

    map[sel].musicfn1=makename(map[sel].musicfn1,name,ID);
//    initprintf("%-15s | ",ID);
//    initprintf("%3d %2d %2d | %s\n",sel,ep,lev,map[sel].musicfn1);
//    playmusic(ID,sel);
    return 0;
}

static int parsedefinitions_game(scriptfile *script, const int preload)
{
    int tokn;
    char *cmdtokptr;

    tokenlist tokens[] =
    {
        { "include",         T_INCLUDE          },
        { "#include",        T_INCLUDE          },
        { "loadgrp",         T_LOADGRP          },
        { "cachesize",       T_CACHESIZE        },
        { "noautload",       T_NOAUTOLOAD       },
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
                int j = initgroupfile(fn);

                if (j == -1)
                    initprintf("Could not find group file '%s'.\n",fn);
                else
                {
                    initprintf("Using group file '%s'.\n",fn);
                    if (!g_NoAutoLoad)
                        autoloadgrps(fn);
                }

            }
            pathsearchmode = 0;
        }
        break;
        case T_CACHESIZE:
        {
            int j;
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
                g_NoAutoLoad = 1;
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
                int i;
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
            int num=-1;
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
                int i;

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

static int loaddefinitions_game(const char *fn, int preload)
{
    scriptfile *script;

    script = scriptfile_fromfile((char *)fn);
    if (!script) return -1;

    parsedefinitions_game(script, preload);

    scriptfile_close(script);
    scriptfile_clearsymbols();

    return 0;
}

static void addgroup(const char *buffer)
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

static void addgamepath(const char *buffer)
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


static void checkcommandline(int argc, const char **argv)
{
    short i, j;
    char *c;
    int firstnet = 0;
    char *k;

    i = 1;

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
                    comlinehelp();
                    exit(0);
                }

                if (!Bstrcasecmp(c+1,"grp") || !Bstrcasecmp(c+1,"g"))
                {
                    if (argc > i+1)
                    {
                        addgroup(argv[i+1]);
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
                        addgamepath(argv[i+1]);
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
                    strcpy(defaultduke3dgrp, "nam.grp");
                    strcpy(defaultconfilename, "nam.con");
                    g_GameType = GAMENAM;
                    i++;
                    continue;
                }
                if (!Bstrcasecmp(c+1,"ww2gi"))
                {
                    strcpy(defaultduke3dgrp, "ww2gi.grp");
                    strcpy(defaultconfilename, "ww2gi.con");
                    g_GameType = GAMEWW2;
                    i++;
                    continue;
                }
                if (!Bstrcasecmp(c+1,"setup"))
                {
                    g_CommandSetup = TRUE;
                    i++;
                    continue;
                }
                if (!Bstrcasecmp(c+1,"nosetup"))
                {
                    g_NoSetup = 1;
                    g_CommandSetup = 0;
                    i++;
                    continue;
                }
                if (!Bstrcasecmp(c+1,"noautoload"))
                {
                    initprintf("Autoload disabled\n");
                    g_NoAutoLoad = 1;
                    i++;
                    continue;
                }
                if (!Bstrcasecmp(c+1,"keepaddr"))
                {
                    g_KeepAddr = 1;
                    i++;
                    continue;
                }
                if (!Bstrcasecmp(c+1,"stun"))
                {
                    natfree = 1; //Addfaz NatFree
                    i++;
                    continue;
                }
#ifndef RANCID_NETWORKING
                if (!Bstrcasecmp(c+1,"rmnet"))
                {
                    if (argc > i+1)
                    {
                        g_NoSetup = TRUE;
                        networkmode = 1;
                        CommandNet = (char *)argv[i+1];
                        i++;
                    }
                    i++;
                    continue;
                }
#endif
                if (!Bstrcasecmp(c+1,"net")
#ifdef RANCID_NETWORKING
                    || !Bstrcasecmp(c+1,"rmnet")
#endif
                    )
                {
                    g_NoSetup = TRUE;
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
                    g_ScriptDebug = 1;
                    i++;
                    continue;
                }
                if (!Bstrcasecmp(c+1,"nologo"))
                {
                    g_NoLogo = 1;
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
                if (!Bstrcasecmp(c+1,"sloppycmd"))
                {
                    checkCON = 0;
                    i++;
                    continue;
                }
                if (!Bstrcasecmp(c+1,"cachesize"))
                {
                    if (argc > i+1)
                    {
                        unsigned int j = atol((char *)argv[i+1]);
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
#ifdef RENDERTYPEWIN
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
                        networkmode = 0;
                        initprintf("Network mode: master/slave\n");
                    }
                    else if (((c[0] == 'n') || (c[0] == 'N')) && (c[1] == '1'))
                    {
                        networkmode = 1;
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
                switch (*c)
                {
                case 'a':
                case 'A':
                    ud.playerai = 1;
                    initprintf("Other player AI.\n");
                    break;
                case 'c':
                case 'C':

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
                case 'D':
                    c++;
                    if (strchr(c,'.') == 0)
                        Bstrcat(c,".dmo");
                    initprintf("Play demo %s.\n",c);
                    Bstrcpy(firstdemofile,c);
                    break;
                case 'f':
                case 'F':
                    c++;
                    if (*c == '1')
                        movesperpacket = 1;
                    if (*c == '2')
                        movesperpacket = 2;
                    if (*c == '4')
                    {
                        movesperpacket = 4;
                        setpackettimeout(0x3fffffff,0x3fffffff); // this doesn't do anything anymore
                    }
                    break;
                case 'g':
                case 'G':
                    c++;
                    if (!*c) break;
                    addgroup(c);
                    break;
                case 'h':
                case 'H':
                    c++;
                    if (*c)
                    {
                        duke3ddef = c;
                        initprintf("Using DEF file: %s.\n",duke3ddef);
                    }
                    break;
                case 'i':
                case 'I':
                    c++;
                    if (*c == '0') networkmode = 0;
                    if (*c == '1') networkmode = 1;
                    initprintf("Network Mode %d\n",networkmode);
                    break;
                case 'j':
                case 'J':
                    c++;
                    if (!*c) break;
                    addgamepath(c);
                    break;
                case 'l':
                case 'L':
                    ud.warp_on = 1;
                    c++;
                    ud.m_level_number = ud.level_number = (atoi(c)-1)%MAXLEVELS;
                    break;
                case 'm':
                case 'M':
                    if (*(c+1) != 'a' && *(c+1) != 'A')
                    {
                        ud.m_monsters_off = 1;
                        ud.m_player_skill = ud.player_skill = 0;
                        initprintf("Monsters off.\n");
                    }
                    break;
                case 'n':
                case 'N':
                    c++;
                    if (*c == 's' || *c == 'S')
                    {
                        g_NoSound = 2;
                        initprintf("Sound off.\n");
                    }
                    else if (*c == 'm' || *c == 'M')
                    {
                        g_NoMusic = 1;
                        initprintf("Music off.\n");
                    }
                    else if (*c == 'd' || *c == 'D')
                    {
                        FILE * fp=fopen("gamevars.txt","w");
                        InitGameVars();
                        DumpGameVars(fp);
                        fclose(fp);
                        initprintf("Game variables saved to gamevars.txt.\n");
                    }
                    else
                    {
                        comlinehelp();
                        exit(-1);
                    }
                    break;
                case 'q':
                case 'Q':
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
                case 'R':
                    ud.m_recstat = 1;
                    initprintf("Demo record mode on.\n");
                    break;
                case 's':
                case 'S':
                    c++;
                    ud.m_player_skill = ud.player_skill = (atoi(c)%5);
                    if (ud.m_player_skill == 4)
                        ud.m_respawn_monsters = ud.respawn_monsters = 1;
                    break;
                case 't':
                case 'T':
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
                case 'U':
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
                case 'V':
                    c++;
                    ud.warp_on = 1;
                    ud.m_volume_number = ud.volume_number = atoi(c)-1;
                    break;
                case 'w':
                case 'W':
                    ud.coords = 1;
                    break;
                case 'x':
                case 'X':
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
                case 'Z':
                    c++;
                    g_ScriptDebug = atoi(c);
                    if (!g_ScriptDebug)
                        g_ScriptDebug = 1;
                    break;
                }
            }
            else
            {
                k = Bstrchr(c,'.');
                if (k)
                {
                    if (!Bstrcasecmp(k,".map"))
                    {
                        CommandMap = (char *)argv[i++];
                        continue;
                    }
                    if (!Bstrcasecmp(k,".grp") || !Bstrcasecmp(k,".zip"))
                    {
                        addgroup(argv[i++]);
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

static void Logo(void)
{
    int soundanm = 0;
    int logoflags=GetGameVar("LOGO_FLAGS",255, -1, -1);

    ready2send = 0;

    KB_FlushKeyboardQueue();
    KB_ClearKeysDown(); // JBF

    setview(0,0,xdim-1,ydim-1);
    clearview(0L);
    IFISSOFTMODE palto(0,0,0,63);

    flushperms();
    nextpage();

    Bsprintf(tempbuf,"%s - " APPNAME,duke3dgrpstring);
    wm_setapptitle(tempbuf);

    MUSIC_StopSong();
    FX_StopAllSounds(); // JBF 20031228
    clearsoundlocks();  // JBF 20031228
    if (ud.multimode < 2 && (logoflags & LOGO_FLAG_ENABLED) && !g_NoLogo)
    {
        if (VOLUMEALL && (logoflags & LOGO_FLAG_PLAYANIM))
        {

            if (!KB_KeyWaiting() && g_NoLogoAnim == 0)
            {
                getpackets();
                playanm("logo.anm",5);
                IFISSOFTMODE palto(0,0,0,63);
                KB_FlushKeyboardQueue();
                KB_ClearKeysDown(); // JBF
            }

            clearview(0L);
            nextpage();
        }

        if (logoflags & LOGO_FLAG_PLAYMUSIC)
        {
            music_select = -1; // hack
            playmusic(&env_music_fn[0][0],MAXVOLUMES*MAXLEVELS);
        }

        if (!NAM)
        {
            fadepal(0,0,0, 0,64,7);
            //g_player[myconnectindex].ps->palette = drealms;
            //palto(0,0,0,63);
            if (logoflags & LOGO_FLAG_3DRSCREEN)
            {
                setgamepalette(g_player[myconnectindex].ps, drealms, 11);    // JBF 20040308
                rotatesprite(0,0,65536L,0,DREALMS,0,0,2+8+16+64, 0,0,xdim-1,ydim-1);
                nextpage();
                fadepal(0,0,0, 63,0,-7);
                totalclock = 0;
                while (totalclock < (120*7) && !KB_KeyWaiting() && !MOUSE_GetButtons()&LEFT_MOUSE)
                {
                    handleevents();
                    getpackets();
                    if (restorepalette)
                    {
                        setgamepalette(g_player[myconnectindex].ps,g_player[myconnectindex].ps->palette,0);
                        restorepalette = 0;
                    }
                }
            }
            KB_ClearKeysDown(); // JBF
            MOUSE_ClearButton(LEFT_MOUSE);
        }

        fadepal(0,0,0, 0,64,7);
        clearview(0L);
        nextpage();
        if (logoflags & LOGO_FLAG_TITLESCREEN)
        {
            //g_player[myconnectindex].ps->palette = titlepal;
            setgamepalette(g_player[myconnectindex].ps, titlepal, 11);   // JBF 20040308
            flushperms();
            rotatesprite(0,0,65536L,0,BETASCREEN,0,0,2+8+16+64,0,0,xdim-1,ydim-1);
            KB_FlushKeyboardQueue();
            fadepal(0,0,0, 63,0,-7);
            totalclock = 0;

            while (totalclock < (860+120) && !KB_KeyWaiting() && !MOUSE_GetButtons()&LEFT_MOUSE)
            {
                rotatesprite(0,0,65536L,0,BETASCREEN,0,0,2+8+16+64,0,0,xdim-1,ydim-1);
                if (logoflags & LOGO_FLAG_DUKENUKEM)
                {
                    if (totalclock > 120 && totalclock < (120+60))
                    {
                        if (soundanm == 0)
                        {
                            soundanm = 1;
                            sound(PIPEBOMB_EXPLODE);
                        }
                        rotatesprite(160<<16,104<<16,(totalclock-120)<<10,0,DUKENUKEM,0,0,2+8,0,0,xdim-1,ydim-1);
                    }
                    else if (totalclock >= (120+60))
                        rotatesprite(160<<16,(104)<<16,60<<10,0,DUKENUKEM,0,0,2+8,0,0,xdim-1,ydim-1);
                }
                else soundanm = 1;
                if (logoflags & LOGO_FLAG_THREEDEE)
                {
                    if (totalclock > 220 && totalclock < (220+30))
                    {
                        if (soundanm == 1)
                        {
                            soundanm = 2;
                            sound(PIPEBOMB_EXPLODE);
                        }

                        rotatesprite(160<<16,(104)<<16,60<<10,0,DUKENUKEM,0,0,2+8,0,0,xdim-1,ydim-1);
                        rotatesprite(160<<16,(129)<<16,(totalclock - 220)<<11,0,THREEDEE,0,0,2+8,0,0,xdim-1,ydim-1);
                    }
                    else if (totalclock >= (220+30))
                        rotatesprite(160<<16,(129)<<16,30<<11,0,THREEDEE,0,0,2+8,0,0,xdim-1,ydim-1);
                }
                else soundanm = 2;
                if (PLUTOPAK && (logoflags & LOGO_FLAG_PLUTOPAKSPRITE))
                {
                    // JBF 20030804
                    if (totalclock >= 280 && totalclock < 395)
                    {
                        rotatesprite(160<<16,(151)<<16,(410-totalclock)<<12,0,PLUTOPAKSPRITE+1,0,0,2+8,0,0,xdim-1,ydim-1);
                        if (soundanm == 2)
                        {
                            soundanm = 3;
                            sound(FLY_BY);
                        }
                    }
                    else if (totalclock >= 395)
                    {
                        if (soundanm == 3)
                        {
                            soundanm = 4;
                            sound(PIPEBOMB_EXPLODE);
                        }
                        rotatesprite(160<<16,(151)<<16,30<<11,0,PLUTOPAKSPRITE+1,0,0,2+8,0,0,xdim-1,ydim-1);
                    }
                }
                OnEvent(EVENT_LOGO, -1, screenpeek, -1);
                handleevents();
                getpackets();
                if (restorepalette)
                {
                    setgamepalette(g_player[myconnectindex].ps,g_player[myconnectindex].ps->palette,0);
                    restorepalette = 0;
                }
                nextpage();
            }
        }
        KB_ClearKeysDown(); // JBF
        MOUSE_ClearButton(LEFT_MOUSE);
    }

    if (ud.multimode > 1)
    {
        setgamepalette(g_player[myconnectindex].ps, titlepal, 11);
        rotatesprite(0,0,65536L,0,BETASCREEN,0,0,2+8+16+64,0,0,xdim-1,ydim-1);

        rotatesprite(160<<16,(104)<<16,60<<10,0,DUKENUKEM,0,0,2+8,0,0,xdim-1,ydim-1);
        rotatesprite(160<<16,(129)<<16,30<<11,0,THREEDEE,0,0,2+8,0,0,xdim-1,ydim-1);
        if (PLUTOPAK)   // JBF 20030804
            rotatesprite(160<<16,(151)<<16,30<<11,0,PLUTOPAKSPRITE+1,0,0,2+8,0,0,xdim-1,ydim-1);

        gametext(160,190,"WAITING FOR PLAYERS",14,2);
        nextpage();
    }

    waitforeverybody();

    flushperms();
    clearview(0L);
    nextpage();

    //g_player[myconnectindex].ps->palette = palette;
    setgamepalette(g_player[myconnectindex].ps, palette, 0);    // JBF 20040308
    sound(NITEVISION_ONOFF);

    //palto(0,0,0,0);
    clearview(0L);
}

static void loadtmb(void)
{
    unsigned char tmb[8000];
    int fil, l;

    fil = kopen4load("d3dtimbr.tmb",0);
    if (fil == -1) return;
    l = kfilelength(fil);
    kread(fil,(char *)tmb,l);
    MUSIC_RegisterTimbreBank(tmb);
    kclose(fil);
}

void freehash();
static void freeconmem(void)
{
    int i;
    extern char *bitptr;

    for (i=(MAXLEVELS*(MAXVOLUMES+1))-1;i>=0;i--) // +1 volume for "intro", "briefing" music
    {
        if (map[i].name != NULL)
            Bfree(map[i].name);
        if (map[i].filename != NULL)
            Bfree(map[i].filename);
        if (map[i].musicfn != NULL)
            Bfree(map[i].musicfn);
        if (map[i].musicfn1 != NULL)
            Bfree(map[i].musicfn1);
        if (map[i].savedstate != NULL)
            FreeMapState(i);
    }

    for (i=MAXQUOTES-1;i>=0;i--)
    {
        if (fta_quotes[i] != NULL)
            Bfree(fta_quotes[i]);
        if (redefined_quotes[i] != NULL)
            Bfree(redefined_quotes[i]);
    }

    for (i=iGameVarCount-1;i>=0;i--)
    {
        if (aGameVars[i].szLabel != NULL)
            Bfree(aGameVars[i].szLabel);
        if (aGameVars[i].plValues != NULL)
            Bfree(aGameVars[i].plValues);
    }

    for (i=iGameArrayCount-1;i>=0;i--)
    {
        if (aGameArrays[i].szLabel != NULL)
            Bfree(aGameArrays[i].szLabel);
        if (aGameArrays[i].plValues != NULL)
            Bfree(aGameArrays[i].plValues);
    }

    for (i=MAXPLAYERS-1;i>=0;i--)
    {
        if (g_player[i].ps != NULL)
            Bfree(g_player[i].ps);
        if (g_player[i].sync != NULL)
            Bfree(g_player[i].sync);
    }

    for (i=MAXSOUNDS-1;i>=0;i--)
    {
        if (g_sounds[i].filename != NULL)
            Bfree(g_sounds[i].filename);
        if (g_sounds[i].filename1 != NULL)
            Bfree(g_sounds[i].filename1);
    }

    if (label != NULL)
        Bfree(label);
    if (labelcode != NULL)
        Bfree(labelcode);
    if (script != NULL)
        Bfree(script);
    if (bitptr != NULL)
        Bfree(bitptr);

    freehash();
    HASH_free(&gamefuncH);
}

/*
===================
=
= ShutDown
=
===================
*/

void Shutdown(void)
{
    SoundShutdown();
    MusicShutdown();
    uninittimer();
    CONTROL_Shutdown();
    CONFIG_WriteSetup();
    KB_Shutdown();
    freeconmem();
    uninitengine();
}

/*
===================
=
= Startup
=
===================
*/

static void compilecons(void)
{
    int i, psm = pathsearchmode;
    label     = (char *)&sprite[0]; // V8: 16384*44/64 = 11264  V7: 4096*44/64 = 2816
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
    loadefs(confilename);

    if (loadfromgrouponly)
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
        loadefs(confilename);
    }

    if ((unsigned int)labelcnt > MAXSPRITES*sizeof(spritetype)/64)   // see the arithmetic above for why
        gameexit("Error: too many labels defined!");
    else
    {
        char *newlabel;
        intptr_t *newlabelcode;

        newlabel     = (char *)malloc(labelcnt<<6);
        newlabelcode = (intptr_t *)malloc(labelcnt*sizeof(intptr_t));

        if (!newlabel || !newlabelcode)
        {
            gameexit("Error: out of memory retaining labels\n");
        }

        copybuf(label,     newlabel, (labelcnt*64)/4);
        copybuf(labelcode, newlabelcode, (labelcnt*sizeof(intptr_t))/4);

        label = newlabel;
        labelcode = newlabelcode;
    }
    clearbufbyte(&sprite[0], sizeof(spritetype) * MAXSPRITES, 0);
    clearbufbyte(&sector[0], sizeof(sectortype) * MAXSECTORS, 0);
    clearbufbyte(&wall[0], sizeof(walltype) * MAXWALLS, 0);

    OnEvent(EVENT_INIT, -1, -1, -1);
    pathsearchmode = psm;
}

static void sanitizegametype()
{
    //    initprintf("ud.m_coop=%i before sanitization\n",ud.m_coop);
    if (ud.m_coop >= num_gametypes || ud.m_coop < 0)
    {
        ud.m_coop = 0;
    }
    Bsprintf(tempbuf,"%s\n",gametype_names[ud.m_coop]);
    initprintf(tempbuf);
    if (gametype_flags[ud.m_coop] & GAMETYPE_FLAG_ITEMRESPAWN)
        ud.m_respawn_items = ud.m_respawn_inventory = 1;
    //     initprintf("ud.m_coop=%i after sanitisation\n",ud.m_coop);
}

static void genspriteremaps(void)
{
    int j,fp;
    signed char look_pos;
    char *lookfn = "lookup.dat";

    fp = kopen4loadfrommod(lookfn,0);
    if (fp != -1)
        kread(fp,(char *)&g_NumPalettes,1);
    else
        gameexit("\nERROR: File 'lookup.dat' not found.");

#if defined(__APPLE__) && B_BIG_ENDIAN != 0
    // this is almost as bad as just setting the value to 25 :P
    g_NumPalettes = (g_NumPalettes * (uint64)0x0202020202 & (uint64)0x010884422010) % 1023;
#endif

    for (j=g_NumPalettes-1;j>=0;j--)
    {
        kread(fp,(signed char *)&look_pos,1);
        kread(fp,tempbuf,256);
        makepalookup((int)look_pos,tempbuf,0,0,0,1);
    }

    for (j = 255; j>=0; j--)
        tempbuf[j] = j;
    g_NumPalettes++;
    makepalookup(g_NumPalettes, tempbuf, 15, 15, 15, 1);
    makepalookup(g_NumPalettes + 1, tempbuf, 15, 0, 0, 1);
    makepalookup(g_NumPalettes + 2, tempbuf, 0, 15, 0, 1);
    makepalookup(g_NumPalettes + 3, tempbuf, 0, 0, 15, 1);

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

extern int startwin_run(void);
static void SetupGameButtons(void);

static void Startup(void)
{
    int i;

    inittimer(TICRATE);

    compilecons();

    CONFIG_ReadKeys(); // we re-read the keys after compiling the CONs

    if (initengine())
    {
        wm_msgbox("Build Engine Initialization Error",
                  "There was a problem initializing the Build engine: %s", engineerrstr);
        freeconmem();
        exit(1);
    }

    setupdynamictostatic();

    if (ud.multimode > 1) sanitizegametype();

    if (g_NoSound) ud.config.SoundToggle = 0;
    if (g_NoMusic) ud.config.MusicToggle = 0;

    if (CommandName)
    {
        //        Bstrncpy(myname, CommandName, 9);
        //        myname[10] = '\0';
        Bstrcpy(tempbuf,CommandName);

        while (Bstrlen(stripcolorcodes(tempbuf,tempbuf)) > 10)
            tempbuf[Bstrlen(tempbuf)-1] = '\0';

        Bstrncpy(myname,tempbuf,sizeof(myname)-1);
        myname[sizeof(myname)-1] = '\0';
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

    for (i=0;i<MAXPLAYERS;i++)
        g_player[i].playerreadyflag = 0;

    #ifndef RANCID_NETWORKING
    // enet regression
    if (CommandNet)
    {
        setup_rancid_net(CommandNet);
        if (Bstrlen(rancid_ip_strings[MAXPLAYERS-1]))
        {
            initprintf("rmnet: Using %s as sort IP\n",rancid_ip_strings[MAXPLAYERS-1]);
            initprintf("rmnet: %d players\n",rancid_players);
        }
        CommandNet = NULL;
    }
    #endif

    #ifdef RANCID_NETWORKING
    // TODO: split this up in the same fine-grained manner as eduke32 network backend, to
    // allow for event handling
    initmultiplayers(netparamcount,netparam);

    if (quitevent)
    {
        Shutdown();
        return;
    }

    #else
    if (initmultiplayersparms(netparamcount,netparam))
    {
        initprintf("Waiting for players...\n");
        while (initmultiplayerscycle())
        {
            handleevents();
            if (quitevent)
            {
                Shutdown();
                return;
            }
        }
    }
    #endif

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
                    gameexit("Failed loading art.");
            }
            chdir(cwd);
        }
        else if (loadpics("tiles000.art",MAXCACHE1DSIZE) < 0)
            gameexit("Failed loading art.");
    }

//    initprintf("Loading palette/lookups...\n");
    genspriteremaps();

    readsavenames();

    tilesizx[MIRROR] = tilesizy[MIRROR] = 0;

    screenpeek = myconnectindex;

    if (networkmode == 255)
        networkmode = 1;
}

void sendscore(const char *s)
{
    UNREFERENCED_PARAMETER(s);
//    if (numplayers > 1)
//        genericmultifunction(-1,(char *)s,strlen(s)+1,5);
}

static void sendwchoice(void)
{
    int i,l;

    buf[0] = 10;
    buf[1] = myconnectindex;
    l = 2;

    for (i=0;i<10;i++)
    {
        g_player[myconnectindex].wchoice[i] = g_player[0].wchoice[i];
        buf[l++] = (char)g_player[0].wchoice[i];
    }

    for (i=connecthead;i>=0;i=connectpoint2[i])
    {
        if (i != myconnectindex) sendpacket(i,&buf[0],l);
        if ((!networkmode) && (myconnectindex != connecthead)) break; //slaves in M/S mode only send to master
    }
}

static void sendplayerupdate(void)
{
    int i,l;

    buf[0] = 6;
    buf[1] = myconnectindex;
    buf[2] = (char)atoi(s_builddate);
    l = 3;

    //null terminated player name to send
    for (i=0;myname[i];i++) buf[l++] = Btoupper(myname[i]);
    buf[l++] = 0;

    buf[l++] = g_player[myconnectindex].ps->aim_mode = ud.mouseaiming;
    buf[l++] = g_player[myconnectindex].ps->auto_aim = ud.config.AutoAim;
    buf[l++] = g_player[myconnectindex].ps->weaponswitch = ud.weaponswitch;
    buf[l++] = g_player[myconnectindex].ps->palookup = g_player[myconnectindex].pcolor = ud.color;

    buf[l++] = g_player[myconnectindex].pteam = ud.team;

    for (i=connecthead;i>=0;i=connectpoint2[i])
    {
        if (i != myconnectindex) sendpacket(i,&buf[0],l);
        if ((!networkmode) && (myconnectindex != connecthead)) break; //slaves in M/S mode only send to master
    }
}

void sendboardname(void)
{
    if (numplayers > 1)
    {
        int j;
        int ch;

        packbuf[0] = 9;
        packbuf[1] = 0;

        Bcorrectfilename(boardfilename,0);

        j = Bstrlen(boardfilename);
        boardfilename[j] = 0;
        Bstrcat(packbuf+1,boardfilename);

        for (ch=connecthead;ch >= 0;ch=connectpoint2[ch])
        {
            if (ch != myconnectindex) sendpacket(ch,packbuf,j+1);
            if ((!networkmode) && (myconnectindex != connecthead)) break; //slaves in M/S mode only send to master
        }
    }
}

void mpchangemap(int volume, int level)
{
    int i;

    packbuf[0] = 5;
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

    for (i=connecthead;i>=0;i=connectpoint2[i])
    {
        if (i != myconnectindex) sendpacket(i,packbuf,12);
        if ((!networkmode) && (myconnectindex != connecthead)) break; //slaves in M/S mode only send to master
    }
}

static void getnames(void)
{
    int l;

    for (l=0;(unsigned)l<sizeof(myname)-1;l++)
        g_player[myconnectindex].user_name[l] = Btoupper(myname[l]);

    if (numplayers > 1)
    {
        sendplayerupdate();
        sendwchoice();
        sendboardname();
        getpackets();
        waitforeverybody();
    }

    if (cp == 1 && numplayers < 2)
        gameexit("Please put the Duke Nukem 3D Atomic Edition CD in the CD-ROM drive.");
}

void updateplayer(void)
{
    int l;

    if (ud.recstat != 0)
        return;

    for (l=0;(unsigned)l<sizeof(myname)-1;l++)
        g_player[myconnectindex].user_name[l] = Btoupper(myname[l]);

    if (numplayers > 1)
    {
        sendplayerupdate();
        if (sprite[g_player[myconnectindex].ps->i].picnum == APLAYER && sprite[g_player[myconnectindex].ps->i].pal != 1)
            sprite[g_player[myconnectindex].ps->i].pal = g_player[myconnectindex].pcolor;
    }
    else
    {
        int j;

        g_player[myconnectindex].ps->aim_mode = ud.mouseaiming;
        g_player[myconnectindex].ps->auto_aim = ud.config.AutoAim;
        g_player[myconnectindex].ps->weaponswitch = ud.weaponswitch;
        g_player[myconnectindex].ps->palookup = g_player[myconnectindex].pcolor = ud.color;
        j = g_player[myconnectindex].ps->team;
        g_player[myconnectindex].pteam = ud.team;

        if (sprite[g_player[myconnectindex].ps->i].picnum == APLAYER && sprite[g_player[myconnectindex].ps->i].pal != 1)
            sprite[g_player[myconnectindex].ps->i].pal = g_player[myconnectindex].pcolor;
    }
}

#if 0
void writestring(int a1,int a2,int a3,short a4,int vx,int vy,int vz)
{

    FILE *fp;

    fp = (FILE *)fopen("debug.txt","rt+");

    fprintf(fp,"%d %d %d %d %d %d %d\n",a1,a2,a3,a4,vx,vy,vz);
    fclose(fp);
}
#endif

#if 0
char testcd(char *fn, int testsiz);

// JBF: various hacks here
static void copyprotect(void)
{
    //    FILE *fp;
    //    char idfile[256];

    cp = 0;

#ifdef NOCOPYPROTECT
    return;
#endif
    if (VOLUMEONE) return;

    if (testcd(IDFILENAME, IDFSIZE))
    {
        cp = 1;
        return;
    }
}
#endif

void backtomenu(void)
{
    boardfilename[0] = 0;
    if (ud.recstat == 1) closedemowrite();
    ud.warp_on = 0;
    g_player[myconnectindex].ps->gm = MODE_MENU;
    cmenu(0);
    KB_FlushKeyboardQueue();
    Bsprintf(tempbuf,APPNAME " - %s",duke3dgrpstring);
    wm_setapptitle(tempbuf);
}

#ifdef RENDERTYPEWIN
void app_crashhandler(void)
{
    closedemowrite();
    scriptinfo();
    sendquit();
}
#endif

void app_main(int argc,const char **argv)
{
    int i = 0, j;
    char cwd[BMAX_PATH];
//    extern char datetimestring[];

#ifdef RENDERTYPEWIN
    if (argc > 1)
    {
        for (;i<argc;i++)
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
        (int(*)(void))GetTime,
        GAME_onshowosd
    );
    Bsprintf(tempbuf,HEAD2 " %s",s_builddate);
    wm_setapptitle(tempbuf);

    initprintf("%s\n",apptitle);
//    initprintf("Compiled %s\n",datetimestring);
    initprintf("Copyright (c) 1996, 2003 3D Realms Entertainment\n");
    initprintf("Copyright (c) 2008 EDuke32 team and contributors\n");

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

    checkcommandline(argc,argv);

#ifdef RENDERTYPEWIN
    if (forcegl) initprintf("GL driver blacklist disabled.\n");
#endif

    g_player[0].ps = (player_struct *) Bcalloc(1, sizeof(player_struct));
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

#if defined(POLYMOST) && defined(USE_OPENGL)
    glusetexcache = glusetexcachecompression = -1;
#endif

#ifdef _WIN32
    ud.config.CheckForUpdates = -1;
#endif

    HASH_init(&gamefuncH);
    for (i=NUMGAMEFUNCTIONS-1;i>=0;i--)
        HASH_add(&gamefuncH,gamefunctions[i],i);

    i = CONFIG_ReadSetup();
    if (getenv("DUKE3DGRP")) duke3dgrp = getenv("DUKE3DGRP");

#if defined(POLYMOST) && defined(USE_OPENGL)
    if (glusetexcache == -1 || glusetexcachecompression == -1)
    {
        i=wm_ynbox("Texture Caching",
                   "Would you like to enable the on-disk texture cache? This feature will use an undetermined amount of space "
                   "on your hard disk to store textures in your video card's native format, enabling them to load dramatically "
                   "faster after the first time they are loaded.\n\n"
                   "You will generally want to say 'yes' here, especially if using the HRP.");
        if (i) ud.config.useprecache = glusetexcompr = glusetexcache = glusetexcachecompression = 1;
        else glusetexcache = glusetexcachecompression = 0;
    }
#endif

#ifdef _WIN32
    if (ud.config.CheckForUpdates == -1)
    {
        i=wm_ynbox("Automatic Release Notification",
                   "Would you like EDuke32 to automatically check for new releases "
                   "at startup?");
        ud.config.CheckForUpdates = 0;
        if (i) ud.config.CheckForUpdates = 1;
    }

//    initprintf("build %d\n",(char)atoi(BUILDDATE));

    if (ud.config.CheckForUpdates == 1)
    {
        if (time(NULL) - ud.config.LastUpdateCheck > UPDATEINTERVAL)
        {
            initprintf("Checking for updates...\n");
            if (getversionfromwebsite(tempbuf))
            {
                initprintf("Current version is %d",atoi(tempbuf));
                ud.config.LastUpdateCheck = time(NULL);

                if (atoi(tempbuf) > atoi(s_builddate))
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
                        CONFIG_SetupMouse();
                        CONFIG_SetupJoystick();
                        CONFIG_WriteSetup();
                        gameexit(" ");
                    }
                }
                else initprintf("... no upgrade available\n");
            }
            else initprintf("update: failed to check for updates\n");
        }
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
        // if it is not found, choose the first GRP from the list of
        struct grpfile *fg, *first = NULL;
        int i;
        for (fg = foundgrps; fg; fg=fg->next)
        {
            for (i = 0; i<numgrpfiles; i++) if (fg->crcval == grpfiles[i].crcval) break;
            if (i == numgrpfiles) continue; // unrecognised grp file
            fg->game = grpfiles[i].game;
            if (!first) first = fg;
            if (!Bstrcasecmp(fg->name, defaultduke3dgrp))
            {
                g_GameType = grpfiles[i].game;
                duke3dgrpstring = (char *)grpfiles[i].name;
                break;
            }
        }
        if (!fg && first)
        {
            Bstrcpy(defaultduke3dgrp, first->name);
            g_GameType = first->game;
            duke3dgrpstring = (char *)grpfiles[0].name;
        }
        else if (!fg) duke3dgrpstring = "Unknown GRP";
    }

#if (defined RENDERTYPEWIN || (defined RENDERTYPESDL && !defined __APPLE__ && defined HAVE_GTK2))
    if (i < 0 || (!g_NoSetup && (ud.configversion != BYTEVERSION_JF || ud.config.ForceSetup)) || g_CommandSetup)
    {
        if (quitevent || !startwin_run())
        {
            uninitengine();
            exit(0);
        }
    }
#endif

    FreeGroups();

    if (WW2GI)
    {
        // overwrite the default GRP and CON so that if the user chooses
        // something different, they get what they asked for
        Bsprintf(defaultduke3dgrp,"ww2gi.grp");
        Bsprintf(defaultconfilename, "ww2gi.con");
        Bsprintf(gametype_names[0],"GRUNTMATCH (SPAWN)");
        Bsprintf(gametype_names[2],"GRUNTMATCH (NO SPAWN)");
    }
    else if (NAM)
    {
        // overwrite the default GRP and CON so that if the user chooses
        // something different, they get what they asked for
        Bsprintf(defaultduke3dgrp,"nam.grp");
        Bsprintf(defaultconfilename, "nam.con");
        Bsprintf(gametype_names[0],"GRUNTMATCH (SPAWN)");
        Bsprintf(gametype_names[2],"GRUNTMATCH (NO SPAWN)");
    }

    if (mod_dir[0] != '/')
    {
        Bstrcat(root,mod_dir);
        addsearchpath(root);
//        addsearchpath(mod_dir);
#if defined(POLYMOST) && defined(USE_OPENGL)
        Bsprintf(tempbuf,"%s/%s",mod_dir,TEXCACHEDIR);
        Bstrcpy(TEXCACHEDIR,tempbuf);
#endif
    }

    i = initgroupfile(duke3dgrp);

    if (i == -1)
        initprintf("Warning: could not find group file '%s'.\n",duke3dgrp);
    else
        initprintf("Using group file '%s' as main group file.\n", duke3dgrp);

    if (!g_NoAutoLoad)
    {
        getfilenames("autoload","*.grp");
        while (findfiles) { Bsprintf(tempbuf,"autoload/%s",findfiles->name); initprintf("Using group file '%s'.\n",tempbuf); initgroupfile(tempbuf); findfiles = findfiles->next; }
        getfilenames("autoload","*.zip");
        while (findfiles) { Bsprintf(tempbuf,"autoload/%s",findfiles->name); initprintf("Using group file '%s'.\n",tempbuf); initgroupfile(tempbuf); findfiles = findfiles->next; }
        getfilenames("autoload","*.pk3");
        while (findfiles) { Bsprintf(tempbuf,"autoload/%s",findfiles->name); initprintf("Using group file '%s'.\n",tempbuf); initgroupfile(tempbuf); findfiles = findfiles->next; }

        if (i != -1)
            autoloadgrps(duke3dgrp);
    }

    if (mod_dir[0] != '/')
    {
        Bsprintf(tempbuf,"%s/",mod_dir);
        getfilenames(tempbuf,"*.grp");
        while (findfiles) { Bsprintf(tempbuf,"%s/%s",mod_dir,findfiles->name); initprintf("Using group file '%s'.\n",tempbuf); initgroupfile(tempbuf); findfiles = findfiles->next; }
        Bsprintf(tempbuf,"%s/",mod_dir);
        getfilenames(tempbuf,"*.zip");
        while (findfiles) { Bsprintf(tempbuf,"%s/%s",mod_dir,findfiles->name); initprintf("Using group file '%s'.\n",tempbuf); initgroupfile(tempbuf); findfiles = findfiles->next; }
        Bsprintf(tempbuf,"%s/",mod_dir);
        getfilenames(tempbuf,"*.pk3");
        while (findfiles) { Bsprintf(tempbuf,"%s/%s",mod_dir,findfiles->name); initprintf("Using group file '%s'.\n",tempbuf); initgroupfile(tempbuf); findfiles = findfiles->next; }
    }

    loaddefinitions_game(duke3ddef, TRUE);

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
                groupfile = j;
                initprintf("Using group file '%s'.\n",CommandGrps->str);
                if (!g_NoAutoLoad)
                    autoloadgrps(CommandGrps->str);
            }

            free(CommandGrps->str);
            free(CommandGrps);
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

#if 0
    copyprotect();
    if (cp) return;
#endif

    if (netparamcount > 0) _buildargc = (argc -= netparamcount+1);  // crop off the net parameters

    // gotta set the proper title after we compile the CONs if this is the full version

    Bsprintf(tempbuf,"%s - " APPNAME,duke3dgrpstring);
    wm_setapptitle(tempbuf);


//    initprintf("\n");

    if (g_ScriptDebug)
        initprintf("CON debugging activated (level %d).\n",g_ScriptDebug);

    RegisterShutdownFunction(Shutdown);

    if (VOLUMEONE)
    {
        initprintf("Distribution of shareware Duke Nukem 3D is restricted in certain ways.\n");
        initprintf("Please read LICENSE.DOC for more details.\n");
    }

    Startup(); // a bunch of stuff including compiling cons

    if (numplayers > 1)
        ud.multimode = numplayers;

    for (i=1;i<ud.multimode;i++)
    {
        g_player[i].ps = (player_struct *) Bcalloc(1,sizeof(player_struct));
        g_player[i].sync = (input_t *) Bcalloc(1,sizeof(input_t));
    }

    g_player[myconnectindex].ps->palette = (char *) &palette[0];

    i = 1;
    for (j=numplayers;j<ud.multimode;j++)
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
    //     initprintf("numplayers=%i\n",numplayers);

    if (numplayers > 1)
    {
        sendlogon();
    }
    else if (boardfilename[0] != 0)
    {
        ud.m_level_number = 7;
        ud.m_volume_number = 0;
        ud.warp_on = 1;
    }

    getnames();

    if (ud.multimode > 1)
    {
        playerswhenstarted = ud.multimode;

        if (ud.warp_on == 0)
        {
            ud.m_monsters_off = 1;
            ud.m_player_skill = 0;
        }
    }

    ud.last_level = -1;

    if (Bstrcasecmp(ud.rtsname,"DUKE.RTS") == 0 ||
            Bstrcasecmp(ud.rtsname,"WW2GI.RTS") == 0 ||
            Bstrcasecmp(ud.rtsname,"NAM.RTS") == 0)
    {
        // ud.last_level is used as a flag here to reset the string to DUKE.RTS after load
        if (WW2GI)
        {
            ud.last_level = 1;
            Bstrcpy(ud.rtsname, "WW2GI.RTS");
        }
        else if (NAM)
        {
            ud.last_level = 1;
            Bstrcpy(ud.rtsname, "NAM.RTS");
        }
        else
        {
            ud.last_level = 1;
            Bstrcpy(ud.rtsname, "DUKE.RTS");
        }
    }

    RTS_Init(ud.rtsname);
    if (numlumps) initprintf("Using .RTS file '%s'\n",ud.rtsname);

    if (ud.last_level == 1)
    {
        ud.last_level = -1;
        Bstrcpy(ud.rtsname, "DUKE.RTS");
    }

    initprintf("Initializing OSD...\n");

    Bsprintf(tempbuf,HEAD2 " %s",s_builddate);
    OSD_SetVersionString(tempbuf, 10,0);
    registerosdcommands();

    if (CONTROL_Startup(1, &GetTime, TICRATE))
    {
        uninitengine();
        exit(1);
    }
    SetupGameButtons();
    CONFIG_SetupMouse();
    CONFIG_SetupJoystick();

    CONTROL_JoystickEnabled = (ud.config.UseJoystick && CONTROL_JoyPresent);
    CONTROL_MouseEnabled = (ud.config.UseMouse && CONTROL_MousePresent);

    // JBF 20040215: evil and nasty place to do this, but joysticks are evil and nasty too
    for (i=0;i<joynumaxes;i++)
        setjoydeadzone(i,ud.config.JoystickAnalogueDead[i],ud.config.JoystickAnalogueSaturate[i]);

    if (setgamemode(ud.config.ScreenMode,ud.config.ScreenWidth,ud.config.ScreenHeight,ud.config.ScreenBPP) < 0)
    {
        int i = 0;
        int xres[] = {ud.config.ScreenWidth,800,640,320};
        int yres[] = {ud.config.ScreenHeight,600,480,240};
        int bpp[] = {32,16,8};

        initprintf("Failure setting video mode %dx%dx%d %s! Attempting safer mode...\n",
                   ud.config.ScreenWidth,ud.config.ScreenHeight,ud.config.ScreenBPP,ud.config.ScreenMode?"fullscreen":"windowed");

#if defined(POLYMOST) && defined(USE_OPENGL)
        {
            int j = 0;
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
                    gameexit("Unable to set failsafe video mode!");
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

    initprintf("Initializing music...\n");
    MusicStartup();
    initprintf("Initializing sound...\n");
    SoundStartup();
    loadtmb();

    /*    if (VOLUMEONE)
        {
            if (numplayers > 4 || ud.multimode > 4)
                gameexit(" The full version of Duke Nukem 3D supports 5 or more players.");
        } */

    setbrightness(ud.brightness>>2,&g_player[myconnectindex].ps->palette[0],0);

    //    if(KB_KeyPressed( sc_Escape ) ) gameexit(" ");

    FX_StopAllSounds();
    clearsoundlocks();

    OSD_Exec("autoexec.cfg");

    {
        char *ptr = Bstrdup(setupfilename);
        Bsprintf(tempbuf,"%s_binds.cfg",strtok(ptr,"."));
        Bfree(ptr);
    }

    OSD_Exec(tempbuf);

    if (ud.warp_on > 1 && ud.multimode < 2)
    {
        clearview(0L);
        //g_player[myconnectindex].ps->palette = palette;
        //palto(0,0,0,0);
        setgamepalette(g_player[myconnectindex].ps, palette, 0);    // JBF 20040308
        rotatesprite(320<<15,200<<15,65536L,0,LOADSCREEN,0,0,2+8+64,0,0,xdim-1,ydim-1);
        menutext(160,105,0,0,"LOADING SAVED GAME...");
        nextpage();

        if (loadplayer(ud.warp_on-2))
            ud.warp_on = 0;
    }

    //    getpackets();

MAIN_LOOP_RESTART:

    if (ud.warp_on == 0)
    {
        if (ud.multimode > 1 && boardfilename[0] != 0)
        {
            ud.m_level_number = 7;
            ud.m_volume_number = 0;

            if (ud.m_player_skill == 4)
                ud.m_respawn_monsters = 1;
            else ud.m_respawn_monsters = 0;

            waitforeverybody();

            for (i=connecthead;i>=0;i=connectpoint2[i])
            {
                resetweapons(i);
                resetinventory(i);
            }

            newgame(ud.m_volume_number,ud.m_level_number,ud.m_player_skill);

            if (enterlevel(MODE_GAME)) backtomenu();
        }
        else Logo();
    }
    else if (ud.warp_on == 1)
    {
        newgame(ud.m_volume_number,ud.m_level_number,ud.m_player_skill);

        if (enterlevel(MODE_GAME)) backtomenu();
    }
    else vscrn();

    if (ud.warp_on == 0 && playback())
    {
        FX_StopAllSounds();
        clearsoundlocks();
        g_NoLogoAnim = 1;
        goto MAIN_LOOP_RESTART;
    }

    ud.auto_run = ud.config.RunMode;
    ud.showweapons = ud.config.ShowOpponentWeapons;
    g_player[myconnectindex].ps->aim_mode = ud.mouseaiming;
    g_player[myconnectindex].ps->auto_aim = ud.config.AutoAim;
    g_player[myconnectindex].ps->weaponswitch = ud.weaponswitch;
    g_player[myconnectindex].pteam = ud.team;

    if (gametype_flags[ud.coop] & GAMETYPE_FLAG_TDM)
        g_player[myconnectindex].ps->palookup = g_player[myconnectindex].pcolor = getteampal(g_player[myconnectindex].pteam);
    else
    {
        if (ud.color) g_player[myconnectindex].ps->palookup = g_player[myconnectindex].pcolor = ud.color;
        else g_player[myconnectindex].ps->palookup = g_player[myconnectindex].pcolor;
    }

    ud.warp_on = 0;
    KB_KeyDown[sc_Pause] = 0;   // JBF: I hate the pause key

    while (!(g_player[myconnectindex].ps->gm&MODE_END)) //The whole loop!!!!!!!!!!!!!!!!!!
    {
        if (handleevents())
        {
            // JBF
            if (quitevent)
            {
                KB_KeyDown[sc_Escape] = 1;
                quitevent = 0;
            }
        }

        AudioUpdate();

        // only allow binds to function if the player is actually in a game (not in a menu, typing, et cetera) or demo
        bindsenabled = (g_player[myconnectindex].ps->gm == MODE_GAME || g_player[myconnectindex].ps->gm == MODE_DEMO);

        OSD_DispatchQueued();

        if (ud.recstat == 2 || ud.multimode > 1 || (ud.show_help == 0 && (g_player[myconnectindex].ps->gm&MODE_MENU) != MODE_MENU))
            if (g_player[myconnectindex].ps->gm&MODE_GAME)
                if (moveloop()) continue;

        if (g_player[myconnectindex].ps->gm&MODE_EOL || g_player[myconnectindex].ps->gm&MODE_RESTART)
        {
            setgamepalette(g_player[myconnectindex].ps, palette, 0);
            setpal(g_player[myconnectindex].ps);

            if (g_player[myconnectindex].ps->gm&MODE_EOL)
            {
                closedemowrite();

                ready2send = 0;

                if (ud.display_bonus_screen == 1)
                {
                    i = ud.screen_size;
                    ud.screen_size = 0;
                    vscrn();
                    ud.screen_size = i;
                    dobonus(0);
                }
                if (ud.eog)
                {
                    ud.eog = 0;
                    if (ud.multimode < 2)
                    {
                        if (!VOLUMEALL)
                        {
                            doorders();
                        }
                        g_player[myconnectindex].ps->gm = MODE_MENU;
                        cmenu(0);
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
            if (numplayers > 1) g_player[myconnectindex].ps->gm = MODE_GAME;
            if (enterlevel(g_player[myconnectindex].ps->gm))
            {
                backtomenu();
                goto MAIN_LOOP_RESTART;
            }
            continue;
        }

        cheats();
        nonsharedkeys();

        if ((ud.show_help == 0 && ud.multimode < 2 && !(g_player[myconnectindex].ps->gm&MODE_MENU)) || ud.multimode > 1 || ud.recstat == 2)
            i = min(max((totalclock-ototalclock)*(65536L/TICSPERFRAME),0),65536);
        else
            i = 65536;

        if (ud.statusbarmode == 1 && (ud.statusbarscale == 100 || !getrendermode()))
        {
            ud.statusbarmode = 0;
            vscrn();
        }

        {
            static unsigned int nextrender = 0;
            unsigned int j = getticks();

            if (j > nextrender+g_FrameDelay)
                nextrender = j;

            if (r_maxfps == 0 || j >= nextrender)
            {
                nextrender += g_FrameDelay;

                displayrooms(screenpeek,i);
                if (getrendermode() >= 3)
                    drawbackground();
                displayrest(i);

                if (g_player[myconnectindex].gotvote == 0 && voting != -1 && voting != myconnectindex)
                {
                    Bsprintf(tempbuf,"%s^00 HAS CALLED A VOTE FOR MAP",g_player[voting].user_name);
                    gametext(160,40,tempbuf,0,2+8+16);
                    Bsprintf(tempbuf,"%s (E%dL%d)",map[vote_episode*MAXLEVELS + vote_map].name,vote_episode+1,vote_map+1);
                    gametext(160,48,tempbuf,0,2+8+16);
                    gametext(160,70,"PRESS F1 TO ACCEPT, F2 TO DECLINE",0,2+8+16);
                }

                if (debug_on) caches();

                checksync();

                if (VOLUMEONE)
                {
                    if (ud.show_help == 0 && show_shareware > 0 && (g_player[myconnectindex].ps->gm&MODE_MENU) == 0)
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

    gameexit(" ");
}

static int demo_version;

static int opendemoread(int which_demo) // 0 = mine
{
    char d[13];
    char ver;
    int i;

    Bstrcpy(d, "demo_.dmo");

    if (which_demo == 10)
        d[4] = 'x';
    else
        d[4] = '0' + which_demo;

    ud.reccnt = 0;

    if (which_demo == 1 && firstdemofile[0] != 0)
    {
        if ((recfilep = kopen4loadfrommod(firstdemofile,loadfromgrouponly)) == -1) return(0);
    }
    else
        if ((recfilep = kopen4loadfrommod(d,loadfromgrouponly)) == -1) return(0);

    if (kread(recfilep,&ud.reccnt,sizeof(int)) != sizeof(int)) goto corrupt;
    if (kread(recfilep,&ver,sizeof(char)) != sizeof(char)) goto corrupt;

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

    if (kread(recfilep,(char *)&ud.volume_number,sizeof(char)) != sizeof(char)) goto corrupt;
    OSD_Printf("ud.volume_number: %d\n",ud.volume_number);
    if (kread(recfilep,(char *)&ud.level_number,sizeof(char)) != sizeof(char)) goto corrupt;
    OSD_Printf("ud.level_number: %d\n",ud.level_number);
    if (kread(recfilep,(char *)&ud.player_skill,sizeof(char)) != sizeof(char)) goto corrupt;
    OSD_Printf("ud.player_skill: %d\n",ud.player_skill);
    if (kread(recfilep,(char *)&ud.m_coop,sizeof(char)) != sizeof(char)) goto corrupt;
    OSD_Printf("ud.m_coop: %d\n",ud.m_coop);
    if (kread(recfilep,(char *)&ud.m_ffire,sizeof(char)) != sizeof(char)) goto corrupt;
    OSD_Printf("ud.m_ffire: %d\n",ud.m_ffire);
    if (kread(recfilep,(short *)&ud.multimode,sizeof(short)) != sizeof(short)) goto corrupt;
    OSD_Printf("ud.multimode: %d\n",ud.multimode);
    if (kread(recfilep,(short *)&ud.m_monsters_off,sizeof(short)) != sizeof(short)) goto corrupt;
    OSD_Printf("ud.m_monsters_off: %d\n",ud.m_monsters_off);
    if (kread(recfilep,(int *)&ud.m_respawn_monsters,sizeof(int)) != sizeof(int)) goto corrupt;
    OSD_Printf("ud.m_respawn_monsters: %d\n",ud.m_respawn_monsters);
    if (kread(recfilep,(int *)&ud.m_respawn_items,sizeof(int)) != sizeof(int)) goto corrupt;
    OSD_Printf("ud.m_respawn_items: %d\n",ud.m_respawn_items);
    if (kread(recfilep,(int *)&ud.m_respawn_inventory,sizeof(int)) != sizeof(int)) goto corrupt;
    OSD_Printf("ud.m_respawn_inventory: %d\n",ud.m_respawn_inventory);
    if (kread(recfilep,(int *)&ud.playerai,sizeof(int)) != sizeof(int)) goto corrupt;
    OSD_Printf("ud.playerai: %d\n",ud.playerai);
    if (kread(recfilep,(int *)&i,sizeof(int)) != sizeof(int)) goto corrupt;

    if (kread(recfilep,(char *)boardfilename,sizeof(boardfilename)) != sizeof(boardfilename)) goto corrupt;

    if (boardfilename[0] != 0)
    {
        ud.m_level_number = 7;
        ud.m_volume_number = 0;
    }

    if (kread(recfilep,(int *)&ud.m_noexits,sizeof(int)) != sizeof(int)) goto corrupt;

    for (i=0;i<ud.multimode;i++)
    {
        if (!g_player[i].ps)
            g_player[i].ps = (player_struct *) Bcalloc(1,sizeof(player_struct));
        if (!g_player[i].sync)
            g_player[i].sync = (input_t *) Bcalloc(1,sizeof(input_t));

        if (kread(recfilep,(char *)g_player[i].user_name,sizeof(g_player[i].user_name)) != sizeof(g_player[i].user_name)) goto corrupt;
        OSD_Printf("ud.user_name: %s\n",g_player[i].user_name);
        if (kread(recfilep,(int *)&g_player[i].ps->aim_mode,sizeof(int)) != sizeof(int)) goto corrupt;
        if (kread(recfilep,(int *)&g_player[i].ps->auto_aim,sizeof(int)) != sizeof(int)) goto corrupt;  // JBF 20031126
        if (kread(recfilep,(int *)&g_player[i].ps->weaponswitch,sizeof(int)) != sizeof(int)) goto corrupt;
        if (kread(recfilep,(int *)&g_player[i].pcolor,sizeof(int)) != sizeof(int)) goto corrupt;
        g_player[i].ps->palookup = g_player[i].pcolor;
        if (kread(recfilep,(int *)&g_player[i].pteam,sizeof(int)) != sizeof(int)) goto corrupt;
        g_player[i].ps->team = g_player[i].pteam;
    }
    i = ud.reccnt/((TICRATE/TICSPERFRAME)*ud.multimode);
    OSD_Printf("demo duration: %d min %d sec\n", i/60, i%60);

    ud.god = ud.cashman = ud.eog = ud.showallmap = 0;
    ud.clipping = ud.scrollmode = ud.overhead_on = ud.pause_on = 0;

    newgame(ud.volume_number,ud.level_number,ud.player_skill);
    return(1);
corrupt:
    OSD_Printf(OSD_ERROR "Demo %d header is corrupt.\n",which_demo);
    ud.reccnt = 0;
    kclose(recfilep);
    return 0;
}

void opendemowrite(void)
{
    char d[13];
    int dummylong = 0, demonum=1;
    char ver;
    short i;

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
    fwrite(&dummylong,4,1,frecfilep);
    fwrite(&ver,sizeof(char),1,frecfilep);
    fwrite((char *)&ud.volume_number,sizeof(char),1,frecfilep);
    fwrite((char *)&ud.level_number,sizeof(char),1,frecfilep);
    fwrite((char *)&ud.player_skill,sizeof(char),1,frecfilep);
    fwrite((char *)&ud.m_coop,sizeof(char),1,frecfilep);
    fwrite((char *)&ud.m_ffire,sizeof(char),1,frecfilep);
    fwrite((short *)&ud.multimode,sizeof(short),1,frecfilep);
    fwrite((short *)&ud.m_monsters_off,sizeof(short),1,frecfilep);
    fwrite((int *)&ud.m_respawn_monsters,sizeof(int),1,frecfilep);
    fwrite((int *)&ud.m_respawn_items,sizeof(int),1,frecfilep);
    fwrite((int *)&ud.m_respawn_inventory,sizeof(int),1,frecfilep);
    fwrite((int *)&ud.playerai,sizeof(int),1,frecfilep);
    fwrite((int *)&ud.auto_run,sizeof(int),1,frecfilep);
    fwrite((char *)boardfilename,sizeof(boardfilename),1,frecfilep);
    fwrite((int *)&ud.m_noexits,sizeof(int),1,frecfilep);

    for (i=0;i<ud.multimode;i++)
    {
        fwrite((char *)&g_player[i].user_name,sizeof(g_player[i].user_name),1,frecfilep);
        fwrite((int *)&g_player[i].ps->aim_mode,sizeof(int),1,frecfilep);
        fwrite((int *)&g_player[i].ps->auto_aim,sizeof(int),1,frecfilep);       // JBF 20031126
        fwrite(&g_player[i].ps->weaponswitch,sizeof(int),1,frecfilep);
        fwrite(&g_player[i].pcolor,sizeof(int),1,frecfilep);
        fwrite(&g_player[i].pteam,sizeof(int),1,frecfilep);
    }

    totalreccnt = 0;
    ud.reccnt = 0;
}

static void record(void)
{
    short i;

    for (i=connecthead;i>=0;i=connectpoint2[i])
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

void closedemowrite(void)
{
    if (ud.recstat == 1)
    {
        if (ud.reccnt > 0)
        {
            dfwrite(recsync,sizeof(input_t)*ud.multimode,ud.reccnt/ud.multimode,frecfilep);

            fseek(frecfilep,SEEK_SET,0L);
            fwrite(&totalreccnt,sizeof(int),1,frecfilep);
            ud.recstat = ud.m_recstat = 0;
        }
        fclose(frecfilep);
    }
}

static int which_demo = 1;
static int in_menu = 0;

// extern int syncs[];
static int playback(void)
{
    int i,j,k,l;
    int foundemo = 0;

    if (ready2send) return 0;

RECHECK:

    in_menu = g_player[myconnectindex].ps->gm&MODE_MENU;

    pub = NUMPAGES;
    pus = NUMPAGES;

    flushperms();

    if (ud.multimode < 2) foundemo = opendemoread(which_demo);

    if (foundemo == 0)
    {
        if (which_demo > 1)
        {
            which_demo = 1;
            goto RECHECK;
        }
        fadepal(0,0,0, 0,63,7);
        setgamepalette(g_player[myconnectindex].ps, palette, 1);    // JBF 20040308
        drawbackground();
        menus();
        //g_player[myconnectindex].ps->palette = palette;
        nextpage();
        fadepal(0,0,0, 63,0,-7);
        ud.reccnt = 0;
    }
    else
    {
        ud.recstat = 2;
        which_demo++;
        if (which_demo == 10) which_demo = 1;
        if (enterlevel(MODE_DEMO)) ud.recstat = foundemo = 0;
    }

    if (foundemo == 0 || in_menu || KB_KeyWaiting() || numplayers > 1)
    {
        FX_StopAllSounds();
        clearsoundlocks();
        g_player[myconnectindex].ps->gm |= MODE_MENU;
    }

    ready2send = 0;
    i = 0;

    KB_FlushKeyboardQueue();

    k = 0;

    while (ud.reccnt > 0 || foundemo == 0)
    {
        if (foundemo)
            while (totalclock >= (lockclock+TICSPERFRAME))
            {
                if ((i == 0) || (i >= RECSYNCBUFSIZ))
                {
                    i = 0;
                    l = min(ud.reccnt,RECSYNCBUFSIZ);
                    if (kdfread(recsync,sizeof(input_t)*ud.multimode,l/ud.multimode,recfilep) != l/ud.multimode)
                    {
                        OSD_Printf(OSD_ERROR "Demo %d is corrupt.\n", which_demo-1);
                        foundemo = 0;
                        ud.reccnt = 0;
                        kclose(recfilep);
                        g_player[myconnectindex].ps->gm |= MODE_MENU;
                        goto RECHECK;
                    }
                }

                for (j=connecthead;j>=0;j=connectpoint2[j])
                {
                    copybufbyte(&recsync[i],&inputfifo[g_player[j].movefifoend&(MOVEFIFOSIZ-1)][j],sizeof(input_t));
                    g_player[j].movefifoend++;
                    i++;
                    ud.reccnt--;
                }
                domovethings();
            }

        if (foundemo == 0)
            drawbackground();
        else
        {
            nonsharedkeys();

            j = min(max((totalclock-lockclock)*(65536/TICSPERFRAME),0),65536);
            displayrooms(screenpeek,j);
            displayrest(j);

            if (ud.multimode > 1 && g_player[myconnectindex].ps->gm)
                getpackets();

            if (g_player[myconnectindex].gotvote == 0 && voting != -1 && voting != myconnectindex)
                gametext(160,60,"PRESS F1 TO ACCEPT, F2 TO DECLINE",0,2+8+16);
        }

        if ((g_player[myconnectindex].ps->gm&MODE_MENU) && (g_player[myconnectindex].ps->gm&MODE_EOL))
            goto RECHECK;

        if (KB_KeyPressed(sc_Escape) && (g_player[myconnectindex].ps->gm&MODE_MENU) == 0 && (g_player[myconnectindex].ps->gm&MODE_TYPE) == 0)
        {
            KB_ClearKeyDown(sc_Escape);
            FX_StopAllSounds();
            clearsoundlocks();
            g_player[myconnectindex].ps->gm |= MODE_MENU;
            cmenu(0);
            intomenusounds();
        }

        if (g_player[myconnectindex].ps->gm&MODE_TYPE)
        {
            typemode();
            if ((g_player[myconnectindex].ps->gm&MODE_TYPE) != MODE_TYPE)
                g_player[myconnectindex].ps->gm = MODE_MENU;
        }
        else
        {
            if (ud.recstat != 2)
                menus();
            if (ud.multimode > 1  && current_menu != 20003 && current_menu != 20005 && current_menu != 210)
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

        operatefta();

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
        getpackets();
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
        unsigned int crcv;
        // sync checker
        +       initcrc32table();
        crc32init(&crcv);
        crc32block(&crcv, (unsigned char *)wall, sizeof(wall));
        crc32block(&crcv, (unsigned char *)sector, sizeof(sector));
        crc32block(&crcv, (unsigned char *)sprite, sizeof(sprite));
        crc32finish(&crcv);
        initprintf("Checksum = %08X\n",crcv);
    }
#endif

    if (g_player[myconnectindex].ps->gm&MODE_MENU) goto RECHECK;
    return 1;
}

static int moveloop()
{
    int i;

    if (numplayers > 1)
        while (fakemovefifoplc < g_player[myconnectindex].movefifoend) fakedomovethings();

    getpackets();

    if (numplayers < 2) bufferjitter = 0;
    while (g_player[myconnectindex].movefifoend-movefifoplc > bufferjitter)
    {
        for (i=connecthead;i>=0;i=connectpoint2[i])
            if (movefifoplc == g_player[i].movefifoend) break;
        if (i >= 0) break;
        if (domovethings()) return 1;
    }
    return 0;
}

static void fakedomovethingscorrect(void)
{
    int i;
    player_struct *p;

    if (numplayers < 2) return;

    i = ((movefifoplc-1)&(MOVEFIFOSIZ-1));
    p = g_player[myconnectindex].ps;

    if (p->posx == myxbak[i] && p->posy == myybak[i] && p->posz == myzbak[i]
            && p->horiz == myhorizbak[i] && p->ang == myangbak[i]) return;

    myx = p->posx;
    omyx = p->oposx;
    myxvel = p->posxv;
    myy = p->posy;
    omyy = p->oposy;
    myyvel = p->posyv;
    myz = p->posz;
    omyz = p->oposz;
    myzvel = p->poszv;
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

    fakemovefifoplc = movefifoplc;
    while (fakemovefifoplc < g_player[myconnectindex].movefifoend)
        fakedomovethings();
}

static void fakedomovethings(void)
{
    input_t *syn;
    player_struct *p;
    int i, j, k, doubvel, fz, cz, hz, lz, x, y;
    unsigned int sb_snum;
    short psect, psectlotag, tempsect, backcstat;
    char shrunk, spritebridge;

    syn = (input_t *)&inputfifo[fakemovefifoplc&(MOVEFIFOSIZ-1)][myconnectindex];

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
        myx = omyx;
        myy = omyy;
    }
    else
    {
        omyx = myx;
        omyy = myy;
    }

    omyhoriz = myhoriz;
    omyhorizoff = myhorizoff;
    omyz = myz;
    omyang = myang;

    getzrange(myx,myy,myz,psect,&cz,&hz,&fz,&lz,163L,CLIPMASK0);

    j = getflorzofslope(psect,myx,myy);

    if ((lz&49152) == 16384 && psectlotag == 1 && klabs(myz-j) > PHEIGHT+(16<<8))
        psectlotag = 0;

    if (p->aim_mode == 0 && myonground && psectlotag != 2 && (sector[psect].floorstat&2))
    {
        x = myx+(sintable[(myang+512)&2047]>>5);
        y = myy+(sintable[myang&2047]>>5);
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
        if (sprite[hz].statnum == 1 && sprite[hz].extra >= 0)
        {
            hz = 0;
            cz = getceilzofslope(psect,myx,myy);
        }
    }

    if (lz >= 0 && (lz&49152) == 49152)
    {
        j = lz&(MAXSPRITES-1);
        if ((sprite[j].cstat&33) == 33)
        {
            psectlotag = 0;
            spritebridge = 1;
        }
        if (badguy(&sprite[j]) && sprite[j].xrepeat > 24 && klabs(sprite[p->i].z-sprite[j].z) < (84<<8))
        {
            j = getangle(sprite[j].x-myx,sprite[j].y-myy);
            myxvel -= sintable[(j+512)&2047]<<4;
            myyvel -= sintable[j&2047]<<4;
        }
    }

    if (sprite[p->i].extra <= 0)
    {
        if (psectlotag == 2)
        {
            if (p->on_warping_sector == 0)
            {
                if (klabs(myz-fz) > (PHEIGHT>>1))
                    myz += 348;
            }
            clipmove(&myx,&myy,&myz,&mycursectnum,0,0,164L,(4L<<8),(4L<<8),CLIPMASK0);
        }

        updatesector(myx,myy,&mycursectnum);
        pushmove(&myx,&myy,&myz,&mycursectnum,128L,(4L<<8),(20L<<8),CLIPMASK0);

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

        if (sb_snum&1)
        {
            if (myzvel > 0) myzvel = 0;
            myzvel -= 348;
            if (myzvel < -(256*6)) myzvel = -(256*6);
        }
        else if (sb_snum&(1<<1))
        {
            if (myzvel < 0) myzvel = 0;
            myzvel += 348;
            if (myzvel > (256*6)) myzvel = (256*6);
        }
        else
        {
            if (myzvel < 0)
            {
                myzvel += 256;
                if (myzvel > 0)
                    myzvel = 0;
            }
            if (myzvel > 0)
            {
                myzvel -= 256;
                if (myzvel < 0)
                    myzvel = 0;
            }
        }

        if (myzvel > 2048) myzvel >>= 1;

        myz += myzvel;

        if (myz > (fz-(15<<8)))
            myz += ((fz-(15<<8))-myz)>>1;

        if (myz < (cz+(4<<8)))
        {
            myz = cz+(4<<8);
            myzvel = 0;
        }
    }

    else if (p->jetpack_on)
    {
        myonground = 0;
        myjumpingcounter = 0;
        myhardlanding = 0;

        if (p->jetpack_on < 11)
            myz -= (p->jetpack_on<<7); //Goin up

        if (shrunk) j = 512;
        else j = 2048;

        if (sb_snum&1)                            //A
            myz -= j;
        if (sb_snum&(1<<1))                       //Z
            myz += j;

        if (shrunk == 0 && (psectlotag == 0 || psectlotag == 2)) k = 32;
        else k = 16;

        if (myz > (fz-(k<<8)))
            myz += ((fz-(k<<8))-myz)>>1;
        if (myz < (cz+(18<<8)))
            myz = cz+(18<<8);
    }
    else if (psectlotag != 2)
    {
        if (psectlotag == 1 && p->spritebridge == 0)
        {
            if (shrunk == 0) i = 34;
            else i = 12;
        }
        if (myz < (fz-(i<<8)) && (floorspace(psect)|ceilingspace(psect)) == 0) //falling
        {
            if ((sb_snum&3) == 0 && myonground && (sector[psect].floorstat&2) && myz >= (fz-(i<<8)-(16<<8)))
                myz = fz-(i<<8);
            else
            {
                myonground = 0;

                myzvel += (gc+80);

                if (myzvel >= (4096+2048)) myzvel = (4096+2048);
            }
        }

        else
        {
            if (psectlotag != 1 && psectlotag != 2 && myonground == 0 && myzvel > (6144>>1))
                myhardlanding = myzvel>>10;
            myonground = 1;

            if (i==40)
            {
                //Smooth on the ground

                k = ((fz-(i<<8))-myz)>>1;
                if (klabs(k) < 256) k = 0;
                myz += k; // ((fz-(i<<8))-myz)>>1;
                myzvel -= 768; // 412;
                if (myzvel < 0) myzvel = 0;
            }
            else if (myjumpingcounter == 0)
            {
                myz += ((fz-(i<<7))-myz)>>1; //Smooth on the water
                if (p->on_warping_sector == 0 && myz > fz-(16<<8))
                {
                    myz = fz-(16<<8);
                    myzvel >>= 1;
                }
            }

            if (sb_snum&2)
                myz += (2048+768);

            if ((sb_snum&1) == 0 && myjumpingtoggle == 1)
                myjumpingtoggle = 0;

            else if ((sb_snum&1) && myjumpingtoggle == 0)
            {
                if (myjumpingcounter == 0)
                    if ((fz-cz) > (56<<8))
                    {
                        myjumpingcounter = 1;
                        myjumpingtoggle = 1;
                    }
            }
            if (myjumpingcounter && (sb_snum&1) == 0)
                myjumpingcounter = 0;
        }

        if (myjumpingcounter)
        {
            if ((sb_snum&1) == 0 && myjumpingtoggle == 1)
                myjumpingtoggle = 0;

            if (myjumpingcounter < (1024+256))
            {
                if (psectlotag == 1 && myjumpingcounter > 768)
                {
                    myjumpingcounter = 0;
                    myzvel = -512;
                }
                else
                {
                    myzvel -= (sintable[(2048-128+myjumpingcounter)&2047])/12;
                    myjumpingcounter += 180;

                    myonground = 0;
                }
            }
            else
            {
                myjumpingcounter = 0;
                myzvel = 0;
            }
        }

        myz += myzvel;

        if (myz < (cz+(4<<8)))
        {
            myjumpingcounter = 0;
            if (myzvel < 0) myxvel = myyvel = 0;
            myzvel = 128;
            myz = cz+(4<<8);
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
        myxvel = 0;
        myyvel = 0;
    }
    else if (syn->avel)          //p->ang += syncangvel * constant
    {
        //ENGINE calculates angvel for you
        int tempang = syn->avel<<1;

        if (psectlotag == 2)
            myang += (tempang-(tempang>>3))*ksgn(doubvel);
        else myang += (tempang)*ksgn(doubvel);
        myang &= 2047;
    }

    if (myxvel || myyvel || syn->fvel || syn->svel)
    {
        if (p->jetpack_on == 0 && p->steroids_amount > 0 && p->steroids_amount < 400)
            doubvel <<= 1;

        myxvel += ((syn->fvel*doubvel)<<6);
        myyvel += ((syn->svel*doubvel)<<6);

        if ((p->curr_weapon == KNEE_WEAPON && p->kickback_pic > 10 && myonground) || (myonground && (sb_snum&2)))
        {
            myxvel = mulscale16(myxvel,p->runspeed-0x2000);
            myyvel = mulscale16(myyvel,p->runspeed-0x2000);
        }
        else
        {
            if (psectlotag == 2)
            {
                myxvel = mulscale16(myxvel,p->runspeed-0x1400);
                myyvel = mulscale16(myyvel,p->runspeed-0x1400);
            }
            else
            {
                myxvel = mulscale16(myxvel,p->runspeed);
                myyvel = mulscale16(myyvel,p->runspeed);
            }
        }

        if (klabs(myxvel) < 2048 && klabs(myyvel) < 2048)
            myxvel = myyvel = 0;

        if (shrunk)
        {
            myxvel =
                mulscale16(myxvel,(p->runspeed)-(p->runspeed>>1)+(p->runspeed>>2));
            myyvel =
                mulscale16(myyvel,(p->runspeed)-(p->runspeed>>1)+(p->runspeed>>2));
        }
    }

FAKEHORIZONLY:
    if (psectlotag == 1 || spritebridge == 1) i = (4L<<8);
    else i = (20L<<8);

    clipmove(&myx,&myy,&myz,&mycursectnum,myxvel,myyvel,164L,4L<<8,i,CLIPMASK0);
    pushmove(&myx,&myy,&myz,&mycursectnum,164L,4L<<8,4L<<8,CLIPMASK0);

    if (p->jetpack_on == 0 && psectlotag != 1 && psectlotag != 2 && shrunk)
        myz += 30<<8;

    if ((sb_snum&(1<<18)) || myhardlanding)
        myreturntocenter = 9;

    if (sb_snum&(1<<13))
    {
        myreturntocenter = 9;
        if (sb_snum&(1<<5)) myhoriz += 6;
        myhoriz += 6;
    }
    else if (sb_snum&(1<<14))
    {
        myreturntocenter = 9;
        if (sb_snum&(1<<5)) myhoriz -= 6;
        myhoriz -= 6;
    }
    else if (sb_snum&(1<<3))
    {
        if (sb_snum&(1<<5)) myhoriz += 6;
        myhoriz += 6;
    }
    else if (sb_snum&(1<<4))
    {
        if (sb_snum&(1<<5)) myhoriz -= 6;
        myhoriz -= 6;
    }

    if (myreturntocenter > 0)
        if ((sb_snum&(1<<13)) == 0 && (sb_snum&(1<<14)) == 0)
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

    OnEvent(EVENT_FAKEDOMOVETHINGS, g_player[myconnectindex].ps->i, myconnectindex, -1);

    myxbak[fakemovefifoplc&(MOVEFIFOSIZ-1)] = myx;
    myybak[fakemovefifoplc&(MOVEFIFOSIZ-1)] = myy;
    myzbak[fakemovefifoplc&(MOVEFIFOSIZ-1)] = myz;
    myangbak[fakemovefifoplc&(MOVEFIFOSIZ-1)] = myang;
    myhorizbak[fakemovefifoplc&(MOVEFIFOSIZ-1)] = myhoriz;
    fakemovefifoplc++;

    sprite[p->i].cstat = backcstat;
}

static int domovethings(void)
{
    int i, j;
    char ch;

    for (i=connecthead;i>=0;i=connectpoint2[i])
        if (g_player[i].sync->bits&(1<<17))
        {
            multiflag = 2;
            multiwhat = (g_player[i].sync->bits>>18)&1;
            multipos = (unsigned)(g_player[i].sync->bits>>19)&15;
            multiwho = i;

            if (multiwhat)
            {
                saveplayer(multipos);
                multiflag = 0;

                if (multiwho != myconnectindex)
                {
                    Bsprintf(fta_quotes[122],"%s^00 SAVED A MULTIPLAYER GAME",&g_player[multiwho].user_name[0]);
                    FTA(122,g_player[myconnectindex].ps);
                }
                else
                {
                    Bstrcpy(fta_quotes[122],"MULTIPLAYER GAME SAVED");
                    FTA(122,g_player[myconnectindex].ps);
                }
                break;
            }
            else
            {
                //            waitforeverybody();

                j = loadplayer(multipos);

                multiflag = 0;

                if (j == 0)
                {
                    if (multiwho != myconnectindex)
                    {
                        Bsprintf(fta_quotes[122],"%s^00 LOADED A MULTIPLAYER GAME",&g_player[multiwho].user_name[0]);
                        FTA(122,g_player[myconnectindex].ps);
                    }
                    else
                    {
                        Bstrcpy(fta_quotes[122],"MULTIPLAYER GAME LOADED");
                        FTA(122,g_player[myconnectindex].ps);
                    }
                    return 1;
                }
            }
        }

    ud.camerasprite = -1;
    lockclock += TICSPERFRAME;

    if (earthquaketime > 0) earthquaketime--;
    if (rtsplaying > 0) rtsplaying--;

    for (i=0;i<MAXUSERQUOTES;i++)
        if (user_quote_time[i])
        {
            user_quote_time[i]--;
            if (user_quote_time[i] > ud.msgdisptime)
                user_quote_time[i] = ud.msgdisptime;
            if (!user_quote_time[i]) pub = NUMPAGES;
        }

    if (ud.idplayers && ud.multimode > 1)
    {
        int sx,sy,sz;
        short sect,hw,hs;

        for (i=0;i<ud.multimode;i++)
            if (g_player[i].ps->holoduke_on != -1)
                sprite[g_player[i].ps->holoduke_on].cstat ^= 256;

        hitscan(g_player[screenpeek].ps->posx,g_player[screenpeek].ps->posy,g_player[screenpeek].ps->posz,g_player[screenpeek].ps->cursectnum,
                sintable[(g_player[screenpeek].ps->ang+512)&2047],
                sintable[g_player[screenpeek].ps->ang&2047],
                (100-g_player[screenpeek].ps->horiz-g_player[screenpeek].ps->horizoff)<<11,&sect,&hw,&hs,&sx,&sy,&sz,0xffff0030);

        for (i=0;i<ud.multimode;i++)
            if (g_player[i].ps->holoduke_on != -1)
                sprite[g_player[i].ps->holoduke_on].cstat ^= 256;

        if ((hs >= 0) && !(g_player[myconnectindex].ps->gm & MODE_MENU) && sprite[hs].picnum == APLAYER && sprite[hs].yvel != screenpeek && g_player[sprite[hs].yvel].ps->dead_flag == 0)
        {
            if (g_player[screenpeek].ps->fta == 0 || g_player[screenpeek].ps->ftq == 117)
            {
                if (ldist(&sprite[g_player[screenpeek].ps->i],&sprite[hs]) < 9216)
                {
                    Bsprintf(fta_quotes[117],"%s",&g_player[sprite[hs].yvel].user_name[0]);
                    g_player[screenpeek].ps->fta = 12, g_player[screenpeek].ps->ftq = 117;
                }
            }
            else if (g_player[screenpeek].ps->fta > 2) g_player[screenpeek].ps->fta -= 3;
        }
    }

    if (show_shareware > 0)
    {
        show_shareware--;
        if (show_shareware == 0)
        {
            pus = NUMPAGES;
            pub = NUMPAGES;
        }
    }

    everyothertime++;

    for (i=connecthead;i>=0;i=connectpoint2[i])
        copybufbyte(&inputfifo[movefifoplc&(MOVEFIFOSIZ-1)][i],g_player[i].sync,sizeof(input_t));
    movefifoplc++;

    updateinterpolations();

    j = -1;
    for (i=connecthead;i>=0;i=connectpoint2[i])
    {
        if ((g_player[i].sync->bits&(1<<26)) == 0)
        {
            j = i;
            continue;
        }

        closedemowrite();

        if (i == myconnectindex) gameexit(" ");
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
            sound(GENERIC_AMBIENCE17);

        pub = NUMPAGES;
        pus = NUMPAGES;
        vscrn();

        quickkill(g_player[i].ps);
        deletesprite(g_player[i].ps->i);

        Bsprintf(buf,"%s^00 is history!",g_player[i].user_name);
        adduserquote(buf);
        Bstrcpy(fta_quotes[116],buf);

        if (voting == i)
        {
            for (i=0;i<MAXPLAYERS;i++)
            {
                g_player[i].vote = 0;
                g_player[i].gotvote = 0;
            }
            voting = -1;
        }

        g_player[myconnectindex].ps->ftq = 116, g_player[myconnectindex].ps->fta = 180;

        if (j < 0 && networkmode == 0)
            gameexit("The server/master player just quit the game; disconnected.");
    }

    if ((numplayers >= 2) && ((movefifoplc&7) == 7))
    {
        ch = (char)(randomseed&255);
        for (i=connecthead;i>=0;i=connectpoint2[i])
            ch += ((g_player[i].ps->posx+g_player[i].ps->posy+g_player[i].ps->posz+g_player[i].ps->ang+g_player[i].ps->horiz)&255);
        g_player[myconnectindex].syncval[g_player[myconnectindex].syncvalhead&(MOVEFIFOSIZ-1)] = ch;
        g_player[myconnectindex].syncvalhead++;
    }

    if (ud.recstat == 1) record();

    if (ud.pause_on == 0)
    {
        global_random = TRAND;
        movedummyplayers();//ST 13
    }

    for (i=connecthead;i>=0;i=connectpoint2[i])
    {
        if (g_player[i].sync->extbits&(1<<6))
        {
            g_player[i].ps->team = g_player[i].pteam;
            if (gametype_flags[ud.coop] & GAMETYPE_FLAG_TDM)
            {
                hittype[g_player[i].ps->i].picnum = APLAYERTOP;
                quickkill(g_player[i].ps);
            }
        }
        if (gametype_flags[ud.coop] & GAMETYPE_FLAG_TDM)
            g_player[i].ps->palookup = g_player[i].pcolor = getteampal(g_player[i].ps->team);

        if (sprite[g_player[i].ps->i].pal != 1)
            sprite[g_player[i].ps->i].pal = g_player[i].pcolor;

        sharedkeys(i);

        if (ud.pause_on == 0)
        {
            processinput(i);
            checksectors(i);
        }
    }

    if (ud.pause_on == 0)
        moveobjects();

    fakedomovethingscorrect();

    if ((everyothertime&1) == 0)
    {
        animatewalls();
        movecyclers();
        pan3dsound();
    }

#ifdef POLYMER
    //polymer invalidate
    updatesectors = 1;
#endif

    return 0;
}

static void doorders(void)
{
    setview(0,0,xdim-1,ydim-1);

    fadepal(0,0,0, 0,63,7);
    //g_player[myconnectindex].ps->palette = palette;
    setgamepalette(g_player[myconnectindex].ps, palette, 1);    // JBF 20040308
    KB_FlushKeyboardQueue();
    rotatesprite(0,0,65536L,0,ORDERING,0,0,2+8+16+64, 0,0,xdim-1,ydim-1);
    fadepal(0,0,0, 63,0,-7);
    while (!KB_KeyWaiting())
    {
        handleevents();
        getpackets();
    }

    fadepal(0,0,0, 0,63,7);
    KB_FlushKeyboardQueue();
    rotatesprite(0,0,65536L,0,ORDERING+1,0,0,2+8+16+64, 0,0,xdim-1,ydim-1);
    fadepal(0,0,0, 63,0,-7);
    while (!KB_KeyWaiting())
    {
        handleevents();
        getpackets();
    }

    fadepal(0,0,0, 0,63,7);
    KB_FlushKeyboardQueue();
    rotatesprite(0,0,65536L,0,ORDERING+2,0,0,2+8+16+64, 0,0,xdim-1,ydim-1);
    fadepal(0,0,0, 63,0,-7);
    while (!KB_KeyWaiting())
    {
        handleevents();
        getpackets();
    }

    fadepal(0,0,0, 0,63,7);
    KB_FlushKeyboardQueue();
    rotatesprite(0,0,65536L,0,ORDERING+3,0,0,2+8+16+64, 0,0,xdim-1,ydim-1);
    fadepal(0,0,0, 63,0,-7);
    while (!KB_KeyWaiting())
    {
        handleevents();
        getpackets();
    }
}

void dobonus(int bonusonly)
{
    int t, tinc,gfx_offset;
    int i, y,xfragtotal,yfragtotal;
    int bonuscnt;
    int clockpad = 2;
    char *lastmapname;
    int playerbest = -1;

    int breathe[] =
    {
        0,  30,VICTORY1+1,176,59,
        30,  60,VICTORY1+2,176,59,
        60,  90,VICTORY1+1,176,59,
        90, 120,0         ,176,59
    };

    int bossmove[] =
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
    else lastmapname = map[(ud.volume_number*MAXLEVELS)+ud.last_level-1].name;

    bonuscnt = 0;

    fadepal(0,0,0, 0,64,7);
    setview(0,0,xdim-1,ydim-1);
    clearview(0L);
    nextpage();
    flushperms();

    FX_StopAllSounds();
    clearsoundlocks();
    FX_SetReverb(0L);
    bindsenabled = 1; // so you can use your screenshot bind on the score screens

    if (bonusonly) goto FRAGBONUS;

    if (numplayers < 2 && ud.eog && ud.from_bonus == 0)
        switch (ud.volume_number)
        {
        case 0:
            if (ud.lockout == 0)
            {
                setgamepalette(g_player[myconnectindex].ps, endingpal, 11); // JBF 20040308
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
                        for (t=0;t<35;t+=5) if (bossmove[t+2] && (totalclock%390) > bossmove[t] && (totalclock%390) <= bossmove[t+1])
                            {
                                if (t==10 && bonuscnt == 1)
                                {
                                    sound(SHOTGUN_FIRE);
                                    sound(SQUISHED);
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
                                sound(DUKETALKTOBOSS);
                                bonuscnt++;
                            }

                        }
                        for (t=0;t<20;t+=5)
                            if (breathe[t+2] && (totalclock%120) > breathe[t] && (totalclock%120) <= breathe[t+1])
                            {
                                if (t==5 && bonuscnt == 0)
                                {
                                    sound(BOSSTALKTODUKE);
                                    bonuscnt++;
                                }
                                rotatesprite(breathe[t+3]<<16,breathe[t+4]<<16,65536L,0,breathe[t+2],0,0,2+8+16+64+128,0,0,xdim-1,ydim-1);
                            }
                    }
                    handleevents();
                    getpackets();
                    nextpage();
                    if (KB_KeyWaiting()) break;
                }
            }

            fadepal(0,0,0, 0,64,1);

            KB_FlushKeyboardQueue();
            //g_player[myconnectindex].ps->palette = palette;
            setgamepalette(g_player[myconnectindex].ps, palette, 11);   // JBF 20040308

            rotatesprite(0,0,65536L,0,3292,0,0,2+8+16+64, 0,0,xdim-1,ydim-1);
            IFISSOFTMODE fadepal(0,0,0, 63,0,-1);
            else nextpage();
            while (!KB_KeyWaiting() && !MOUSE_GetButtons()&LEFT_MOUSE)
            {
                handleevents();
                getpackets();
            }
            fadepal(0,0,0, 0,64,1);
            MUSIC_StopSong();
            FX_StopAllSounds();
            clearsoundlocks();
            break;
        case 1:
            MUSIC_StopSong();
            clearview(0L);
            nextpage();

            if (ud.lockout == 0)
            {
                playanm("cineov2.anm",1);
                KB_FlushKeyBoardQueue();
                clearview(0L);
                nextpage();
            }

            sound(PIPEBOMB_EXPLODE);

            fadepal(0,0,0, 0,64,1);
            setview(0,0,xdim-1,ydim-1);
            KB_FlushKeyboardQueue();
            //g_player[myconnectindex].ps->palette = palette;
            setgamepalette(g_player[myconnectindex].ps, palette, 11);   // JBF 20040308
            rotatesprite(0,0,65536L,0,3293,0,0,2+8+16+64, 0,0,xdim-1,ydim-1);
            IFISSOFTMODE fadepal(0,0,0, 63,0,-1);
            else nextpage();
            while (!KB_KeyWaiting() && !MOUSE_GetButtons()&LEFT_MOUSE)
            {
                handleevents();
                getpackets();
            }
            IFISSOFTMODE fadepal(0,0,0, 0,64,1);

            break;

        case 3:

            setview(0,0,xdim-1,ydim-1);

            MUSIC_StopSong();
            clearview(0L);
            nextpage();

            if (ud.lockout == 0)
            {
                KB_FlushKeyboardQueue();
                playanm("vol4e1.anm",8);
                clearview(0L);
                nextpage();
                playanm("vol4e2.anm",10);
                clearview(0L);
                nextpage();
                playanm("vol4e3.anm",11);
                clearview(0L);
                nextpage();
            }

            FX_StopAllSounds();
            clearsoundlocks();
            sound(ENDSEQVOL3SND4);
            KB_FlushKeyBoardQueue();

            //g_player[myconnectindex].ps->palette = palette;
            setgamepalette(g_player[myconnectindex].ps, palette, 11);   // JBF 20040308
            IFISSOFTMODE palto(0,0,0,63);
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
                getpackets();
            }
            fadepal(0,0,0, 0,64,3);

            clearview(0L);
            nextpage();
            MOUSE_ClearButton(LEFT_MOUSE);

            playanm("DUKETEAM.ANM",4);

            KB_FlushKeyBoardQueue();
            while (!KB_KeyWaiting() && !MOUSE_GetButtons()&LEFT_MOUSE)
            {
                handleevents();
                getpackets();
            }

            clearview(0L);
            nextpage();
            IFISSOFTMODE palto(0,0,0,63);

            FX_StopAllSounds();
            clearsoundlocks();
            KB_FlushKeyBoardQueue();
            MOUSE_ClearButton(LEFT_MOUSE);

            break;

        case 2:

            MUSIC_StopSong();
            clearview(0L);
            nextpage();
            if (ud.lockout == 0)
            {
                fadepal(0,0,0, 63,0,-1);
                playanm("cineov3.anm",2);
                KB_FlushKeyBoardQueue();
                ototalclock = totalclock+200;
                while (totalclock < ototalclock)
                {
                    handleevents();
                    getpackets();
                }
                clearview(0L);
                nextpage();

                FX_StopAllSounds();
                clearsoundlocks();
            }

            playanm("RADLOGO.ANM",3);

            if (ud.lockout == 0 && !KB_KeyWaiting() && !MOUSE_GetButtons()&LEFT_MOUSE)
            {
                sound(ENDSEQVOL3SND5);
                while (issoundplaying(-1,ENDSEQVOL3SND5))
                {
                    handleevents();
                    getpackets();
                }
                if (KB_KeyWaiting() || MOUSE_GetButtons()&LEFT_MOUSE) goto ENDANM;
                sound(ENDSEQVOL3SND6);
                while (issoundplaying(-1,ENDSEQVOL3SND6))
                {
                    handleevents();
                    getpackets();
                }
                if (KB_KeyWaiting() || MOUSE_GetButtons()&LEFT_MOUSE) goto ENDANM;
                sound(ENDSEQVOL3SND7);
                while (issoundplaying(-1,ENDSEQVOL3SND7))
                {
                    handleevents();
                    getpackets();
                }
                if (KB_KeyWaiting() || MOUSE_GetButtons()&LEFT_MOUSE) goto ENDANM;
                sound(ENDSEQVOL3SND8);
                while (issoundplaying(-1,ENDSEQVOL3SND8))
                {
                    handleevents();
                    getpackets();
                }
                if (KB_KeyWaiting() || MOUSE_GetButtons()&LEFT_MOUSE) goto ENDANM;
                sound(ENDSEQVOL3SND9);
                while (issoundplaying(-1,ENDSEQVOL3SND9))
                {
                    handleevents();
                    getpackets();
                }

            }
            MOUSE_ClearButton(LEFT_MOUSE);
            KB_FlushKeyBoardQueue();
            totalclock = 0;
            while (!KB_KeyWaiting() && totalclock < 120 && !MOUSE_GetButtons()&LEFT_MOUSE)
            {
                handleevents();
                getpackets();
            }

ENDANM:
            MOUSE_ClearButton(LEFT_MOUSE);
            FX_StopAllSounds();
            clearsoundlocks();

            KB_FlushKeyBoardQueue();

            clearview(0L);

            break;
        }

FRAGBONUS:

    //g_player[myconnectindex].ps->palette = palette;
    setgamepalette(g_player[myconnectindex].ps, palette, 11);   // JBF 20040308
    IFISSOFTMODE palto(0,0,0,63);   // JBF 20031228
    KB_FlushKeyboardQueue();
    totalclock = 0;
    tinc = 0;
    bonuscnt = 0;

    MUSIC_StopSong();
    FX_StopAllSounds();
    clearsoundlocks();

    if (playerswhenstarted > 1 && (gametype_flags[ud.coop]&GAMETYPE_FLAG_SCORESHEET))
    {
        if (!(ud.config.MusicToggle == 0 || ud.config.MusicDevice < 0))
            sound(BONUSMUSIC);

        rotatesprite(0,0,65536L,0,MENUSCREEN,16,0,2+8+16+64,0,0,xdim-1,ydim-1);
        rotatesprite(160<<16,34<<16,65536L,0,INGAMEDUKETHREEDEE,0,0,10,0,0,xdim-1,ydim-1);
        if (PLUTOPAK)   // JBF 20030804
            rotatesprite((260)<<16,36<<16,65536L,0,PLUTOPAKSPRITE+2,0,0,2+8,0,0,xdim-1,ydim-1);
        gametext(160,58+2,"MULTIPLAYER TOTALS",0,2+8+16);
        gametext(160,58+10,map[(ud.volume_number*MAXLEVELS)+ud.last_level-1].name,0,2+8+16);

        gametext(160,165,"PRESS ANY KEY TO CONTINUE",0,2+8+16);

        t = 0;
        minitext(23,80,"   NAME                                           KILLS",8,2+8+16+128);
        for (i=0;i<playerswhenstarted;i++)
        {
            Bsprintf(tempbuf,"%-4d",i+1);
            minitext(92+(i*23),80,tempbuf,3,2+8+16+128);
        }

        for (i=0;i<playerswhenstarted;i++)
        {
            xfragtotal = 0;
            Bsprintf(tempbuf,"%d",i+1);

            minitext(30,90+t,tempbuf,0,2+8+16+128);
            minitext(38,90+t,g_player[i].user_name,g_player[i].ps->palookup,2+8+16+128);

            for (y=0;y<playerswhenstarted;y++)
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

        for (y=0;y<playerswhenstarted;y++)
        {
            yfragtotal = 0;
            for (i=0;i<playerswhenstarted;i++)
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
        while (KB_KeyWaiting()==0)
        {
            handleevents();
            getpackets();
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
        sound(BONUSMUSIC);

    nextpage();
    KB_FlushKeyboardQueue();
    fadepal(0,0,0, 63,0,-1);
    bonuscnt = 0;
    totalclock = 0;
    tinc = 0;

    playerbest = CONFIG_GetMapBestTime(map[ud.volume_number*MAXLEVELS+ud.last_level-1].filename);

    if (g_player[myconnectindex].ps->player_par < playerbest || playerbest < 0)
    {
        CONFIG_SetMapBestTime(map[ud.volume_number*MAXLEVELS+ud.last_level-1].filename, g_player[myconnectindex].ps->player_par);
        //        if(playerbest != -1)
        //            playerbest = g_player[myconnectindex].ps->player_par;
    }

    {
        int ii, ij;

        for (ii=g_player[myconnectindex].ps->player_par/(26*60), ij=1; ii>9; ii/=10, ij++) ;
        clockpad = max(clockpad,ij);
        if (!(ud.volume_number == 0 && ud.last_level-1 == 7))
        {
            for (ii=map[ud.volume_number*MAXLEVELS+ud.last_level-1].partime/(26*60), ij=1; ii>9; ii/=10, ij++) ;
            clockpad = max(clockpad,ij);
            if (!NAM)
            {
                for (ii=map[ud.volume_number*MAXLEVELS+ud.last_level-1].designertime/(26*60), ij=1; ii>9; ii/=10, ij++) ;
                clockpad = max(clockpad,ij);
            }
        }
        if (playerbest > 0) for (ii=playerbest/(26*60), ij=1; ii>9; ii/=10, ij++) ;
        clockpad = max(clockpad,ij);
    }

    while (1)
    {
        int yy = 0, zz;

        getpackets();
        handleevents();
        AudioUpdate();

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
                        sound(SHOTGUN_COCK);
                        switch (rand()&3)
                        {
                        case 0:
                            sound(BONUS_SPEECH1);
                            break;
                        case 1:
                            sound(BONUS_SPEECH2);
                            break;
                        case 2:
                            sound(BONUS_SPEECH3);
                            break;
                        case 3:
                            sound(BONUS_SPEECH4);
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
                        sound(PIPEBOMB_EXPLODE);
                    }

                    Bsprintf(tempbuf,"%0*d:%02d.%02d",clockpad,
                             (g_player[myconnectindex].ps->player_par/(26*60)),
                             (g_player[myconnectindex].ps->player_par/26)%60,
                             ((g_player[myconnectindex].ps->player_par%26)*38)/10
                            );
                    gametext((320>>2)+71,yy+9,tempbuf,0,2+8+16);
                    if (g_player[myconnectindex].ps->player_par < playerbest)
                        gametext((320>>2)+89+(clockpad*24),yy+9,"New record!",0,2+8+16);
                    yy+=10;

                    if (!(ud.volume_number == 0 && ud.last_level-1 == 7))
                    {
                        Bsprintf(tempbuf,"%0*d:%02d",clockpad,
                                 (map[ud.volume_number*MAXLEVELS+ud.last_level-1].partime/(26*60)),
                                 (map[ud.volume_number*MAXLEVELS+ud.last_level-1].partime/26)%60);
                        gametext((320>>2)+71,yy+9,tempbuf,0,2+8+16);
                        yy+=10;

                        if (!NAM)
                        {
                            Bsprintf(tempbuf,"%0*d:%02d",clockpad,
                                     (map[ud.volume_number*MAXLEVELS+ud.last_level-1].designertime/(26*60)),
                                     (map[ud.volume_number*MAXLEVELS+ud.last_level-1].designertime/26)%60);
                            gametext((320>>2)+71,yy+9,tempbuf,0,2+8+16);
                            yy+=10;
                        }
                    }

                    if (playerbest > 0)
                    {
                        sprintf(tempbuf,"%0*d:%02d.%02d",clockpad,
                                (playerbest/(26*60)),
                                (playerbest/26)%60,
                                ((playerbest%26)*38)/10
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
                    sound(FLY_BY);
                }

                yy = zz;

                if (totalclock > (60*7))
                {
                    if (bonuscnt == 3)
                    {
                        bonuscnt++;
                        sound(PIPEBOMB_EXPLODE);
                    }
                    sprintf(tempbuf,"%-3d",g_player[myconnectindex].ps->actors_killed);
                    gametext((320>>2)+70,yy+9,tempbuf,0,2+8+16);
                    yy += 10;
                    if (ud.player_skill > 3)
                    {
                        sprintf(tempbuf,"N/A");
                        gametext((320>>2)+70,yy+9,tempbuf,0,2+8+16);
                        yy += 10;
                    }
                    else
                    {
                        if ((g_player[myconnectindex].ps->max_actors_killed-g_player[myconnectindex].ps->actors_killed) < 0)
                            sprintf(tempbuf,"%-3d",0);
                        else sprintf(tempbuf,"%-3d",g_player[myconnectindex].ps->max_actors_killed-g_player[myconnectindex].ps->actors_killed);
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
                        sound(PIPEBOMB_EXPLODE);
                    }
                    sprintf(tempbuf,"%-3d",g_player[myconnectindex].ps->secret_rooms);
                    gametext((320>>2)+70,yy+9,tempbuf,0,2+8+16);
                    yy += 10;
                    if (g_player[myconnectindex].ps->secret_rooms > 0)
                        sprintf(tempbuf,"%-3d%%",(100*g_player[myconnectindex].ps->secret_rooms/g_player[myconnectindex].ps->max_secret_rooms));
                    sprintf(tempbuf,"%-3d",g_player[myconnectindex].ps->max_secret_rooms-g_player[myconnectindex].ps->secret_rooms);
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
        OnEvent(EVENT_DISPLAYBONUSSCREEN, g_player[screenpeek].ps->i, screenpeek, -1);
        nextpage();
    }
}

static void cameratext(short i)
{
    char flipbits;
    int x , y;

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
        for (x=0;x<394;x+=64)
            for (y=0;y<200;y+=64)
                rotatesprite(x<<16,y<<16,65536L,0,STATIC,0,0,2+flipbits,windowx1,windowy1,windowx2,windowy2);
    }
}

#if 0
void vglass(int x,int y,short a,short wn,short n)
{
    int z, zincs;
    short sect;

    sect = wall[wn].nextsector;
    if (sect == -1) return;
    zincs = (sector[sect].floorz-sector[sect].ceilingz) / n;

    for (z = sector[sect].ceilingz;z < sector[sect].floorz; z += zincs)
        EGS(sect,x,y,z-(TRAND&8191),GLASSPIECES+(z&(TRAND%3)),-32,36,36,a+128-(TRAND&255),16+(TRAND&31),0,-1,5);
}
#endif

void lotsofglass(int i,int wallnum,int n)
{
    int j, xv, yv, z, x1, y1;
    short sect;
    int a;

    sect = -1;

    if (wallnum < 0)
    {
        for (j=n-1; j >= 0 ;j--)
        {
            a = SA-256+(TRAND&511)+1024;
            EGS(SECT,SX,SY,SZ,GLASSPIECES+(j%3),-32,36,36,a,32+(TRAND&63),1024-(TRAND&1023),i,5);
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

    for (j=n;j>0;j--)
    {
        x1 += xv;
        y1 += yv;

        updatesector(x1,y1,&sect);
        if (sect >= 0)
        {
            z = sector[sect].floorz-(TRAND&(klabs(sector[sect].ceilingz-sector[sect].floorz)));
            if (z < -(32<<8) || z > (32<<8))
                z = SZ-(32<<8)+(TRAND&((64<<8)-1));
            a = SA-1024;
            EGS(SECT,x1,y1,z,GLASSPIECES+(j%3),-32,36,36,a,32+(TRAND&63),-(TRAND&1023),i,5);
        }
    }
}

void spriteglass(int i,int n)
{
    int j, k, a, z;

    for (j=n;j>0;j--)
    {
        a = TRAND&2047;
        z = SZ-((TRAND&16)<<8);
        k = EGS(SECT,SX,SY,z,GLASSPIECES+(j%3),TRAND&15,36,36,a,32+(TRAND&63),-512-(TRAND&2047),i,5);
        sprite[k].pal = sprite[i].pal;
    }
}

void ceilingglass(int i,int sectnum,int n)
{
    int j, xv, yv, z, x1, y1, a,s;
    int startwall = sector[sectnum].wallptr;
    int endwall = startwall+sector[sectnum].wallnum;

    for (s=startwall;s<(endwall-1);s++)
    {
        x1 = wall[s].x;
        y1 = wall[s].y;

        xv = (wall[s+1].x-x1)/(n+1);
        yv = (wall[s+1].y-y1)/(n+1);

        for (j=n;j>0;j--)
        {
            x1 += xv;
            y1 += yv;
            a = TRAND&2047;
            z = sector[sectnum].ceilingz+((TRAND&15)<<8);
            EGS(sectnum,x1,y1,z,GLASSPIECES+(j%3),-32,36,36,a,(TRAND&31),0,i,5);
        }
    }
}

void lotsofcolourglass(int i,int wallnum,int n)
{
    int j, xv, yv, z, x1, y1;
    short sect = -1;
    int a, k;

    if (wallnum < 0)
    {
        for (j=n-1; j >= 0 ;j--)
        {
            a = TRAND&2047;
            k = EGS(SECT,SX,SY,SZ-(TRAND&(63<<8)),GLASSPIECES+(j%3),-32,36,36,a,32+(TRAND&63),1024-(TRAND&2047),i,5);
            sprite[k].pal = TRAND&15;
        }
        return;
    }

    j = n+1;
    x1 = wall[wallnum].x;
    y1 = wall[wallnum].y;

    xv = (wall[wall[wallnum].point2].x-wall[wallnum].x)/j;
    yv = (wall[wall[wallnum].point2].y-wall[wallnum].y)/j;

    for (j=n;j>0;j--)
    {
        x1 += xv;
        y1 += yv;

        updatesector(x1,y1,&sect);
        z = sector[sect].floorz-(TRAND&(klabs(sector[sect].ceilingz-sector[sect].floorz)));
        if (z < -(32<<8) || z > (32<<8))
            z = SZ-(32<<8)+(TRAND&((64<<8)-1));
        a = SA-1024;
        k = EGS(SECT,x1,y1,z,GLASSPIECES+(j%3),-32,36,36,a,32+(TRAND&63),-(TRAND&2047),i,5);
        sprite[k].pal = TRAND&7;
    }
}

static void SetupGameButtons(void)
{
    CONTROL_DefineFlag(gamefunc_Move_Forward,false);
    CONTROL_DefineFlag(gamefunc_Move_Backward,false);
    CONTROL_DefineFlag(gamefunc_Turn_Left,false);
    CONTROL_DefineFlag(gamefunc_Turn_Right,false);
    CONTROL_DefineFlag(gamefunc_Strafe,false);
    CONTROL_DefineFlag(gamefunc_Fire,false);
    CONTROL_DefineFlag(gamefunc_Open,false);
    CONTROL_DefineFlag(gamefunc_Run,false);
    CONTROL_DefineFlag(gamefunc_AutoRun,false);
    CONTROL_DefineFlag(gamefunc_Jump,false);
    CONTROL_DefineFlag(gamefunc_Crouch,false);
    CONTROL_DefineFlag(gamefunc_Look_Up,false);
    CONTROL_DefineFlag(gamefunc_Look_Down,false);
    CONTROL_DefineFlag(gamefunc_Look_Left,false);
    CONTROL_DefineFlag(gamefunc_Look_Right,false);
    CONTROL_DefineFlag(gamefunc_Strafe_Left,false);
    CONTROL_DefineFlag(gamefunc_Strafe_Right,false);
    CONTROL_DefineFlag(gamefunc_Aim_Up,false);
    CONTROL_DefineFlag(gamefunc_Aim_Down,false);
    CONTROL_DefineFlag(gamefunc_Weapon_1,false);
    CONTROL_DefineFlag(gamefunc_Weapon_2,false);
    CONTROL_DefineFlag(gamefunc_Weapon_3,false);
    CONTROL_DefineFlag(gamefunc_Weapon_4,false);
    CONTROL_DefineFlag(gamefunc_Weapon_5,false);
    CONTROL_DefineFlag(gamefunc_Weapon_6,false);
    CONTROL_DefineFlag(gamefunc_Weapon_7,false);
    CONTROL_DefineFlag(gamefunc_Weapon_8,false);
    CONTROL_DefineFlag(gamefunc_Weapon_9,false);
    CONTROL_DefineFlag(gamefunc_Weapon_10,false);
    CONTROL_DefineFlag(gamefunc_Inventory,false);
    CONTROL_DefineFlag(gamefunc_Inventory_Left,false);
    CONTROL_DefineFlag(gamefunc_Inventory_Right,false);
    CONTROL_DefineFlag(gamefunc_Holo_Duke,false);
    CONTROL_DefineFlag(gamefunc_Jetpack,false);
    CONTROL_DefineFlag(gamefunc_NightVision,false);
    CONTROL_DefineFlag(gamefunc_MedKit,false);
    CONTROL_DefineFlag(gamefunc_TurnAround,false);
    CONTROL_DefineFlag(gamefunc_SendMessage,false);
    CONTROL_DefineFlag(gamefunc_Map,false);
    CONTROL_DefineFlag(gamefunc_Shrink_Screen,false);
    CONTROL_DefineFlag(gamefunc_Enlarge_Screen,false);
    CONTROL_DefineFlag(gamefunc_Center_View,false);
    CONTROL_DefineFlag(gamefunc_Holster_Weapon,false);
    CONTROL_DefineFlag(gamefunc_Show_Opponents_Weapon,false);
    CONTROL_DefineFlag(gamefunc_Map_Follow_Mode,false);
    CONTROL_DefineFlag(gamefunc_See_Coop_View,false);
    CONTROL_DefineFlag(gamefunc_Mouse_Aiming,false);
    CONTROL_DefineFlag(gamefunc_Toggle_Crosshair,false);
    CONTROL_DefineFlag(gamefunc_Steroids,false);
    CONTROL_DefineFlag(gamefunc_Quick_Kick,false);
    CONTROL_DefineFlag(gamefunc_Next_Weapon,false);
    CONTROL_DefineFlag(gamefunc_Previous_Weapon,false);
}

/*
===================
=
= GetTime
=
===================
*/

inline int GetTime(void)
{
    return totalclock;
}
