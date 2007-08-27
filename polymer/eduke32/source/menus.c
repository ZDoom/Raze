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
#include "mouse.h"
#include "osd.h"
#include <sys/stat.h>

extern char inputloc;
extern int recfilep;
//extern char vgacompatible;
short globalskillsound=-1;
int probey=0;
static int lastsavehead=0,last_menu_pos=0,last_menu,sh,onbar,buttonstat,deletespot;
static int last_zero,last_fifty,last_onehundred,last_twoohtwo,last_threehundred = 0;

static char menunamecnt;

static CACHE1D_FIND_REC *finddirs=NULL, *findfiles=NULL, *finddirshigh=NULL, *findfileshigh=NULL;
static int numdirs=0, numfiles=0;
static int currentlist=0;

static int function, whichkey;
static int changesmade, newvidmode, curvidmode, newfullscreen;
static int vidsets[16] = { -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1 };
static int curvidset, newvidset = 0;

static char *mousebuttonnames[] = { "Left", "Right", "Middle", "Thumb", "Wheel Up", "Wheel Down" };

extern int gotvote[MAXPLAYERS], votes[MAXPLAYERS], voting;

void cmenu(int cm)
{
    current_menu = cm;

    if ((cm >= 1000 && cm <= 1009))
        return;

    if (cm == 0)
        probey = last_zero;
    else if (cm == 50)
        probey = last_fifty;
    else if (cm == 100)
        probey = last_onehundred;
    else if (cm == 202)
        probey = last_twoohtwo;
    else if (cm >= 300 && cm < 400)
        probey = last_threehundred;
    else if (cm == 110)
        probey = 1;
    else probey = 0;
    lastsavehead = -1;
}

#if 0
void savetemp(char *fn,long daptr,long dasiz)
{
    FILE *fp;

    if ((fp = fopen(fn,"wb")) == (FILE *)NULL)
        return;

    fwrite((char *)daptr,dasiz,1,fp);

    fclose(fp);
}
#endif

#define LMB (buttonstat&1)
#define RMB (buttonstat&2)
#define WHEELUP (buttonstat&16)
#define WHEELDOWN (buttonstat&32)

static ControlInfo minfo;

static short mi, mii;

static int probe_(int type,int x,int y,int i,int n)
{
    short centre, s;

    s = 1+(CONTROL_GetMouseSensitivity()>>4);

    {
        CONTROL_GetInput(&minfo);
        mi += (minfo.dpitch+minfo.dz);
        mii += minfo.dyaw;
    }

    if (x == (320>>1))
        centre = 320>>2;
    else centre = 0;

    if (!buttonstat || buttonstat == 16 || buttonstat == 32)
    {
        if (KB_KeyPressed(sc_UpArrow) || KB_KeyPressed(sc_PgUp) || KB_KeyPressed(sc_kpad_8) || mi < -8192 || WHEELUP)
        {
            mi = mii = 0;
            KB_ClearKeyDown(sc_UpArrow);
            KB_ClearKeyDown(sc_kpad_8);
            KB_ClearKeyDown(sc_PgUp);
            MOUSE_ClearButton(WHEELUP_MOUSE);
            sound(KICK_HIT);

            probey--;
            if (probey < 0) probey = n-1;
        }
        if (KB_KeyPressed(sc_DownArrow) || KB_KeyPressed(sc_PgDn) || KB_KeyPressed(sc_kpad_2) || mi > 8192 || WHEELDOWN)
        {
            mi = mii = 0;
            KB_ClearKeyDown(sc_DownArrow);
            KB_ClearKeyDown(sc_kpad_2);
            KB_ClearKeyDown(sc_PgDn);
            MOUSE_ClearButton(WHEELDOWN_MOUSE);
            sound(KICK_HIT);
            probey++;
        }
    }

    if (probey >= n)
        probey = 0;

    if (centre)
    {
        //        rotatesprite(((320>>1)+(centre)+54)<<16,(y+(probey*i)-4)<<16,65536L,0,SPINNINGNUKEICON+6-((6+(totalclock>>3))%7),sh,0,10,0,0,xdim-1,ydim-1);
        //        rotatesprite(((320>>1)-(centre)-54)<<16,(y+(probey*i)-4)<<16,65536L,0,SPINNINGNUKEICON+((totalclock>>3)%7),sh,0,10,0,0,xdim-1,ydim-1);

        rotatesprite(((320>>1)+(centre>>1)+70)<<16,(y+(probey*i)-4)<<16,65536L>>type,0,SPINNINGNUKEICON+6-((6+(totalclock>>3))%7),sh,0,10,0,0,xdim-1,ydim-1);
        rotatesprite(((320>>1)-(centre>>1)-70)<<16,(y+(probey*i)-4)<<16,65536L>>type,0,SPINNINGNUKEICON+((totalclock>>3)%7),sh,0,10,0,0,xdim-1,ydim-1);
    }
    else
        rotatesprite((x<<16)-((tilesizx[BIGFNTCURSOR]-4)<<(16-type)),(y+(probey*i)-(4>>type))<<16,65536L>>type,0,SPINNINGNUKEICON+(((totalclock>>3))%7),sh,0,10,0,0,xdim-1,ydim-1);

    if (KB_KeyPressed(sc_Space) || KB_KeyPressed(sc_kpad_Enter) || KB_KeyPressed(sc_Enter) || (LMB && !onbar))
    {
        if (current_menu != 110)
            sound(PISTOL_BODYHIT);
        KB_ClearKeyDown(sc_Enter);
        KB_ClearKeyDown(sc_Space);
        KB_ClearKeyDown(sc_kpad_Enter);
        MOUSE_ClearButton(LEFT_MOUSE);
        return(probey);
    }
    else if (KB_KeyPressed(sc_Escape) || (RMB))
    {
        onbar = 0;
        KB_ClearKeyDown(sc_Escape);
        sound(EXITMENUSOUND);
        MOUSE_ClearButton(RIGHT_MOUSE);
        return(-1);
    }
    else
    {
        if (onbar == 0) return(-probey-2);
        if (KB_KeyPressed(sc_LeftArrow) || KB_KeyPressed(sc_kpad_4) || ((buttonstat&1) && (WHEELUP || mii < -256)))
            return(probey);
        else if (KB_KeyPressed(sc_RightArrow) || KB_KeyPressed(sc_kpad_6) || ((buttonstat&1) && (WHEELDOWN || mii > 256)))
            return(probey);
        return(-probey-2);
    }
}
static inline int probe(int x,int y,int i,int n)
{
    return probe_(0,x,y,i,n);
}

static inline int probesm(int x,int y,int i,int n)
{
    return probe_(1,x,y,i,n);
}

static int menutext_(int x,int y,short s,short p,char *t)
{
    short i, ac, centre;

    y -= 12;

    i = centre = 0;

    if (x == (320>>1))
    {
        while (*(t+i))
        {
            if (*(t+i) == ' ')
            {
                centre += 5;
                i++;
                continue;
            }
            ac = 0;
            if (*(t+i) >= '0' && *(t+i) <= '9')
                ac = *(t+i) - '0' + BIGALPHANUM-10;
            else if (*(t+i) >= 'a' && *(t+i) <= 'z')
                ac = toupper(*(t+i)) - 'A' + BIGALPHANUM;
            else if (*(t+i) >= 'A' && *(t+i) <= 'Z')
                ac = *(t+i) - 'A' + BIGALPHANUM;
            else switch (*(t+i))
                {
                case '-':
                    ac = BIGALPHANUM-11;
                    break;
                case '.':
                    ac = BIGPERIOD;
                    break;
                case '\'':
                    ac = BIGAPPOS;
                    break;
                case ',':
                    ac = BIGCOMMA;
                    break;
                case '!':
                    ac = BIGX;
                    break;
                case '?':
                    ac = BIGQ;
                    break;
                case ';':
                    ac = BIGSEMI;
                    break;
                case ':':
                    ac = BIGSEMI;
                    break;
                default:
                    centre += 5;
                    i++;
                    continue;
                }

            centre += tilesizx[ac]-1;
            i++;
        }
    }

    if (centre)
        x = (320-centre-10)>>1;

    while (*t)
    {
        if (*t == ' ')
        {
            x+=5;
            t++;
            continue;
        }
        ac = 0;
        if (*t >= '0' && *t <= '9')
            ac = *t - '0' + BIGALPHANUM-10;
        else if (*t >= 'a' && *t <= 'z')
            ac = toupper(*t) - 'A' + BIGALPHANUM;
        else if (*t >= 'A' && *t <= 'Z')
            ac = *t - 'A' + BIGALPHANUM;
        else switch (*t)
            {
            case '-':
                ac = BIGALPHANUM-11;
                break;
            case '.':
                ac = BIGPERIOD;
                break;
            case ',':
                ac = BIGCOMMA;
                break;
            case '!':
                ac = BIGX;
                break;
            case '\'':
                ac = BIGAPPOS;
                break;
            case '?':
                ac = BIGQ;
                break;
            case ';':
                ac = BIGSEMI;
                break;
            case ':':
                ac = BIGCOLIN;
                break;
            default:
                x += 5;
                t++;
                continue;
            }

        rotatesprite(x<<16,y<<16,65536L,0,ac,s,p,10+16,0,0,xdim-1,ydim-1);

        x += tilesizx[ac];
        t++;
    }
    return (x);
}

inline int menutext(int x,int y,int s,int p,const char *t)
{
    return(menutext_(x,y,s,p,(char *)stripcolorcodes(t)));
}

static void bar_(int type, int x,int y,short *p,int dainc,int damodify,int s, int pa)
{
    short xloc;
    char rev;

    if (dainc < 0)
    {
        dainc = -dainc;
        rev = 1;
    }
    else rev = 0;
    y-=2;

    if (damodify)
    {
        if (rev == 0)
        {
            if (*p > 0 && (KB_KeyPressed(sc_LeftArrow) || KB_KeyPressed(sc_kpad_4) || ((buttonstat&1) && (WHEELUP || mii < -256))))         // && onbar) )
            {
                KB_ClearKeyDown(sc_LeftArrow);
                KB_ClearKeyDown(sc_kpad_4);
                MOUSE_ClearButton(WHEELUP_MOUSE);
                mii = 0;
                *p -= dainc;
                if (*p < 0)
                    *p = 0;
                sound(KICK_HIT);
            }
            if (*p < 63 && (KB_KeyPressed(sc_RightArrow) || KB_KeyPressed(sc_kpad_6) || ((buttonstat&1) && (WHEELDOWN || mii > 256))))        //&& onbar) )
            {
                KB_ClearKeyDown(sc_RightArrow);
                KB_ClearKeyDown(sc_kpad_6);
                MOUSE_ClearButton(WHEELDOWN_MOUSE);
                mii = 0;
                *p += dainc;
                if (*p > 63)
                    *p = 63;
                sound(KICK_HIT);
            }
        }
        else
        {
            if (*p > 0 && (KB_KeyPressed(sc_RightArrow) || KB_KeyPressed(sc_kpad_6) || ((buttonstat&1) && minfo.dyaw > 256)))        //&& onbar) )
            {
                KB_ClearKeyDown(sc_RightArrow);
                KB_ClearKeyDown(sc_kpad_6);

                *p -= dainc;
                if (*p < 0)
                    *p = 0;
                sound(KICK_HIT);
            }
            if (*p < 64 && (KB_KeyPressed(sc_LeftArrow) || KB_KeyPressed(sc_kpad_4) || ((buttonstat&1) && minfo.dyaw < -256)))         // && onbar) )
            {
                KB_ClearKeyDown(sc_LeftArrow);
                KB_ClearKeyDown(sc_kpad_4);

                *p += dainc;
                if (*p > 64)
                    *p = 64;
                sound(KICK_HIT);
            }
        }
    }

    xloc = *p;

    rotatesprite((x<<16)+(22<<(16-type)),(y<<16)-(3<<(16-type)),65536L>>type,0,SLIDEBAR,s,pa,10,0,0,xdim-1,ydim-1);
    if (rev == 0)
        rotatesprite((x<<16)+((xloc+1)<<(16-type)),(y<<16)+(1<<(16-type)),65536L>>type,0,SLIDEBAR+1,s,pa,10,0,0,xdim-1,ydim-1);
    else
        rotatesprite((x<<16)+((65-xloc)<<(16-type)),(y<<16)+(1<<(16-type)),65536L>>type,0,SLIDEBAR+1,s,pa,10,0,0,xdim-1,ydim-1);
}

static inline void bar(int x,int y,short *p,int dainc,int damodify,int s, int pa)
{
    bar_(0,x,y,p,dainc,damodify,s,pa);
}
static inline void barsm(int x,int y,short *p,int dainc,int damodify,int s, int pa)
{
    bar_(1,x,y,p,dainc,damodify,s,pa);
}

static void modval(int min, int max,int *p,int dainc,int damodify)
{
    char rev;

    if (dainc < 0)
    {
        dainc = -dainc;
        rev = 1;
    }
    else rev = 0;

    if (damodify)
    {
        if (rev == 0)
        {
            if (KB_KeyPressed(sc_LeftArrow) || KB_KeyPressed(sc_kpad_4) || ((buttonstat&1) && minfo.dyaw < -256))        // && onbar) )
            {
                KB_ClearKeyDown(sc_LeftArrow);
                KB_ClearKeyDown(sc_kpad_4);

                *p -= dainc;
                if (*p < min)
                {
                    *p = max;
                    if (damodify == 2)
                        *p = min;
                }
                sound(PISTOL_BODYHIT);
            }
            if (KB_KeyPressed(sc_RightArrow) || KB_KeyPressed(sc_kpad_6) || ((buttonstat&1) && minfo.dyaw > 256))       //&& onbar) )
            {
                KB_ClearKeyDown(sc_RightArrow);
                KB_ClearKeyDown(sc_kpad_6);

                *p += dainc;
                if (*p > max)
                {
                    *p = min;
                    if (damodify == 2)
                        *p = max;
                }
                sound(PISTOL_BODYHIT);
            }
        }
        else
        {
            if (KB_KeyPressed(sc_RightArrow) || KB_KeyPressed(sc_kpad_6) || ((buttonstat&1) && minfo.dyaw > 256))      //&& onbar ))
            {
                KB_ClearKeyDown(sc_RightArrow);
                KB_ClearKeyDown(sc_kpad_6);

                *p -= dainc;
                if (*p < min)
                {
                    *p = max;
                    if (damodify == 2)
                        *p = min;
                }
                sound(PISTOL_BODYHIT);
            }
            if (KB_KeyPressed(sc_LeftArrow) || KB_KeyPressed(sc_kpad_4) || ((buttonstat&1) && minfo.dyaw < -256))      // && onbar) )
            {
                KB_ClearKeyDown(sc_LeftArrow);
                KB_ClearKeyDown(sc_kpad_4);

                *p += dainc;
                if (*p > max)
                {
                    *p = min;
                    if (damodify == 2)
                        *p = max;
                }
                sound(PISTOL_BODYHIT);
            }
        }
    }
}

#define UNSELMENUSHADE 10
#define DISABLEDMENUSHADE 20
#define MENUHIGHLIGHT(x) probey==x?-(sintable[(totalclock<<4)&2047]>>12):UNSELMENUSHADE
// #define MENUHIGHLIGHT(x) probey==x?-(sintable[(totalclock<<4)&2047]>>12):probey-x>=0?(probey-x)<<2:-((probey-x)<<2)

#define SHX(X) 0
// ((x==X)*(-sh))
#define PHX(X) 0
// ((x==X)?1:2)
#define MWIN(X) rotatesprite( 320<<15,200<<15,X,0,MENUSCREEN,-16,0,10+64,0,0,xdim-1,ydim-1)
#define MWINXY(X,OX,OY) rotatesprite( ( 320+(OX) )<<15, ( 200+(OY) )<<15,X,0,MENUSCREEN,-16,0,10+64,0,0,xdim-1,ydim-1)

extern int loadpheader(char spot,struct savehead *saveh);

static struct savehead savehead;
//static int32 volnum,levnum,plrskl,numplr;
//static char brdfn[BMAX_PATH];
int lastsavedpos = -1;

static void dispnames(void)
{
    int x, c = 160;

    c += 64;
    for (x = 0;x <= 108;x += 12)
        rotatesprite((c+91-64)<<16,(x+56)<<16,65536L,0,TEXTBOX,24,0,10,0,0,xdim-1,ydim-1);

    rotatesprite(22<<16,97<<16,65536L,0,WINDOWBORDER2,24,0,10,0,0,xdim-1,ydim-1);
    rotatesprite(180<<16,97<<16,65536L,1024,WINDOWBORDER2,24,0,10,0,0,xdim-1,ydim-1);
    rotatesprite(99<<16,50<<16,65536L,512,WINDOWBORDER1,24,0,10,0,0,xdim-1,ydim-1);
    rotatesprite(103<<16,144<<16,65536L,1024+512,WINDOWBORDER1,24,0,10,0,0,xdim-1,ydim-1);

    for (x=0;x<=9;x++)
        minitext(c,48+(12*x),ud.savegame[x],2,10+16);
}

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

long quittimer = 0;

void check_player_color(int *color,int prev_color)
{
    int i, disallowed[] = { 1, 2, 3, 4, 5, 6, 7, 8, 17, 18, 19, 20, 22 };

    for (i=0;i<(signed)(sizeof(disallowed)/sizeof(disallowed[0]));i++)
    {
        while (*color == disallowed[i])
        {
            if (*color > prev_color)
                (*color)++;
            else (*color)--;
            i=0;
        }
    }
}

void sendquit(void)
{
    if (gamequit == 0 && (numplayers > 1))
    {
        if (g_player[myconnectindex].ps.gm&MODE_GAME)
        {
            gamequit = 1;
            quittimer = totalclock+120;
        }
        else
        {
            int i;

            tempbuf[0] = 254;
            tempbuf[1] = myconnectindex;

            for (i=connecthead;i >= 0;i=connectpoint2[i])
            {
                if (i != myconnectindex) sendpacket(i,tempbuf,2);
                if ((!networkmode) && (myconnectindex != connecthead)) break; //slaves in M/S mode only send to master
            }
            gameexit(" ");
        }
    }
    else if (numplayers < 2)
        gameexit(" ");

    if ((totalclock > quittimer) && (gamequit == 1))
        gameexit("Timed out.");
}

void menus(void)
{
    CACHE1D_FIND_REC *dir;
    int c,x,i;
    long l,m;
    char *p = NULL;

    getpackets();

    {
        if (buttonstat != 0 && !onbar)
        {
            x = MOUSE_GetButtons()<<3;
            if (x) buttonstat = x<<3;
            else buttonstat = 0;
        }
        else
            buttonstat = MOUSE_GetButtons();
    }

    if ((g_player[myconnectindex].ps.gm&MODE_MENU) == 0)
    {
        walock[TILE_LOADSHOT] = 1;
        return;
    }

    g_player[myconnectindex].ps.gm &= (0xff-MODE_TYPE);
    g_player[myconnectindex].ps.fta = 0;

    x = 0;

    sh = 4-(sintable[(totalclock<<4)&2047]>>11);

    if (bpp > 8)
    {
        long x,y,y1=0,y2=ydim;
        for (y=y1;y<y2;y+=tilesizy[BIGHOLE])
            for (x=0;x<xdim;x+=tilesizx[BIGHOLE])
                rotatesprite(x<<16,y<<16,65536L,0,BIGHOLE,80,0,1+8+16,0,0,xdim-1,ydim-1);
    }

    if (!(current_menu >= 1000 && current_menu <= 2999 && current_menu >= 300 && current_menu <= 369))
        vscrn();

    if (KB_KeyPressed(sc_Q))
    {
        switch (current_menu)
        {
        case 210:
        case 360:
        case 361:
        case 362:
        case 363:
        case 364:
        case 365:
        case 366:
        case 367:
        case 368:
        case 369:
        case 500:
        case 501:
        case 502:
        case 603:
        case 10001:
        case 20003:
        case 20005:
            break;
        default:
            if (current_menu >= 0)
            {
                last_menu = current_menu;
                last_menu_pos = probey;
                cmenu(502);
            }
            break;
        }
    }

    switch (current_menu)
    {
    case 25000:
        gametext(160,90,"SELECT A SAVE SPOT BEFORE",0,2+8+16);
        gametext(160,90+9,"YOU QUICK RESTORE.",0,2+8+16);

        x = probe(186,124,0,1);
        if (x >= -1)
        {
            if (ud.multimode < 2 && ud.recstat != 2)
            {
                ready2send = 1;
                totalclock = ototalclock;
            }
            g_player[myconnectindex].ps.gm &= ~MODE_MENU;
        }
        break;

    case 20000:
        x = probe(326,190,0,1);
        gametext(160,50-8,"YOU ARE PLAYING THE SHAREWARE",0,2+8+16);
        gametext(160,59-8,"VERSION OF DUKE NUKEM 3D.  WHILE",0,2+8+16);
        gametext(160,68-8,"THIS VERSION IS REALLY COOL, YOU",0,2+8+16);
        gametext(160,77-8,"ARE MISSING OVER 75%% OF THE TOTAL",0,2+8+16);
        gametext(160,86-8,"GAME, ALONG WITH OTHER GREAT EXTRAS",0,2+8+16);
        gametext(160,95-8,"AND GAMES, WHICH YOU'LL GET WHEN",0,2+8+16);
        gametext(160,104-8,"YOU ORDER THE COMPLETE VERSION AND",0,2+8+16);
        gametext(160,113-8,"GET THE FINAL TWO EPISODES.",0,2+8+16);

        gametext(160,113+8,"PLEASE READ THE 'HOW TO ORDER' ITEM",0,2+8+16);
        gametext(160,122+8,"ON THE MAIN MENU IF YOU WISH TO",0,2+8+16);
        gametext(160,131+8,"UPGRADE TO THE FULL REGISTERED",0,2+8+16);
        gametext(160,140+8,"VERSION OF DUKE NUKEM 3D.",0,2+8+16);
        gametext(160,149+16,"PRESS ANY KEY...",0,2+8+16);

        if (x >= -1) cmenu(100);
        break;

    case 20001:
        rotatesprite(160<<16,19<<16,65536L,0,MENUBAR,16,0,10,0,0,xdim-1,ydim-1);
        menutext(160,24,0,0,"NETWORK GAME");

        x = probe(160,100-18,18,3);

        if (x == -1) cmenu(0);
        else if (x == 2) cmenu(20010);
        else if (x == 1) cmenu(20020);
        else if (x == 0) cmenu(20002);

        menutext(160,100-18,MENUHIGHLIGHT(0),0,"PLAYER SETUP");
        menutext(160,100,MENUHIGHLIGHT(1),0,"JOIN GAME");
        menutext(160,100+18,MENUHIGHLIGHT(2),0,"HOST GAME");
        break;

    case 20002:
    case 20003:
        rotatesprite(160<<16,19<<16,65536L,0,MENUBAR,16,0,10,0,0,xdim-1,ydim-1);
        menutext(160,24,0,0,"PLAYER SETUP");
        c = (320>>1)-120;
        {
            int io, ii, yy = 37, d=c+140, enabled;
            char *opts[] =
            {
                "Name",
                "-",
                "Color",
                "-",
                "Team",
                "-",
                "-",
                "Auto aim",
                "Mouse aim",
                "-",
                "-",
                "Switch weapon on pickup",
                "Switch weapon when empty",
                "-",
                "-",
                "Network packets/sec",
                "-",
                "-",
                "Multiplayer macros",
                NULL
            };

            x = ud.color;

            if (probey == 2)
                x = getteampal(ud.team);

            rotatesprite((260)<<16,(24+(tilesizy[APLAYER]>>1))<<16,49152L,0,1441-((((4-(totalclock>>4)))&3)*5),0,x,10,0,0,xdim-1,ydim-1);

            for (ii=io=0; opts[ii]; ii++)
            {
                if (opts[ii][0] == '-' && !opts[ii][1])
                {
                    if (io <= probey) yy += 4;
                    continue;
                }
                if (io < probey) yy += 8;
                io++;
            }


            if (current_menu == 20002)
            {
                x = probesm(c,yy+5,0,io);

                if (x == -1)
                {
                    cmenu(202);
                    probey = 3;
                    break;
                }

                for (ii=io=0; opts[ii]; ii++)
                {
                    if (opts[ii][0] == '-' && !opts[ii][1])
                        continue;
                    enabled = 1;
                    switch (io)
                    {
                    case 0:
                        if (x == io)
                        {
                            strcpy(buf, myname);
                            inputloc = strlen(buf);
                            current_menu = 20003;

                            KB_ClearKeyDown(sc_Enter);
                            KB_ClearKeyDown(sc_kpad_Enter);
                            KB_FlushKeyboardQueue();
                        }
                        break;

                    case 1:
                        i = ud.color;
                        if (x == io)
                        {
                            ud.color++;
                            if (ud.color > 23)
                                ud.color = 0;
                            check_player_color((int *)&ud.color,-1);
                        }
                        modval(0,23,(int *)&ud.color,1,probey==1);
                        check_player_color((int *)&ud.color,i);
                        if (ud.color != i)
                            updateplayer();
                        break;

                    case 2:
                        i = ud.team;
                        if (x == io)
                        {
                            ud.team++;
                            if (ud.team == 4)
                                ud.team = 0;
                        }
                        modval(0,3,(int *)&ud.team,1,probey==2);
                        if (ud.team != i)
                            updateplayer();
                        break;

                    case 3:
                        i = ud.config.AutoAim;
                        if (x == io)
                            ud.config.AutoAim = (ud.config.AutoAim == 2) ? 0 : ud.config.AutoAim+1;
                        modval(0,2,(int *)&ud.config.AutoAim,1,probey==3);
                        if (ud.config.AutoAim != i)
                            updateplayer();
                        break;

                    case 4:
                        i = ud.mouseaiming;
                        if (x == io)
                            ud.mouseaiming = !ud.mouseaiming;
                        modval(0,1,(int *)&ud.mouseaiming,1,probey==4);
                        if (ud.mouseaiming != i)
                            updateplayer();
                        break;

                    case 5:
                        i = 0;
                        if (ud.weaponswitch & 1)
                            i = 1;
                        if (x == io)
                            i = 1-i;
                        modval(0,1,(int *)&i,1,probey==5);
                        if ((ud.weaponswitch & 1 && !i) || (!(ud.weaponswitch & 1) && i))
                        {
                            ud.weaponswitch ^= 1;
                            updateplayer();
                        }
                        break;
                    case 6:
                        i = 0;
                        if (ud.weaponswitch & 2)
                            i = 1;
                        if (x == io)
                            i = 1-i;
                        modval(0,1,(int *)&i,1,probey==6);
                        if ((ud.weaponswitch & 2 && !i) || (!(ud.weaponswitch & 2) && i))
                        {
                            ud.weaponswitch ^= 2;
                            updateplayer();
                        }
                        break;
                    case 7:
                        if (x == io)
                            packetrate = min(max(((packetrate/50)*50)+50,50),1000);
                        modval(50,1000,(int *)&packetrate,10,probey==7?2:0);
                        break;
                    case 8:
                        if (x == io)
                        {
                            cmenu(20004);
                        }
                        break;

                    default:
                        break;
                    }
                    io++;
                }
            }
            else
            {
                x = strget(d-50,37,buf,30,0);

                while (Bstrlen(stripcolorcodes(buf)) > 10)
                {
                    buf[Bstrlen(buf)-1] = '\0';
                    inputloc--;
                }

                if (x)
                {
                    if (x == 1)
                    {
                        if (buf[0] && Bstrcmp(myname,buf))
                        {
                            Bstrcpy(myname,buf);
                            updateplayer();
                        }
                        // send name update
                    }
                    KB_ClearKeyDown(sc_Enter);
                    KB_ClearKeyDown(sc_kpad_Enter);
                    KB_FlushKeyboardQueue();

                    current_menu = 20002;
                }
            }

            yy = 37;
            {
                for (ii=io=0; opts[ii]; ii++)
                {
                    if (opts[ii][0] == '-' && !opts[ii][1])
                    {
                        yy += 4;
                        continue;
                    }
                    enabled = 1;
                    switch (io)
                    {
                    case 0:
                        if (current_menu == 20002)
                        {
                            gametext(d-50,yy,myname,MENUHIGHLIGHT(io),2+8+16);
                        }
                        break;

                    case 1:
                        {
                            char *s[] = { "Auto","","","","","","","","","Blue","Red","Green","Gray","Dark gray","Dark green","Brown",
                                          "Dark blue","","","","","Bright red","","Yellow","",""
                                        };
                            gametext(d-50,yy,s[ud.color],MENUHIGHLIGHT(io),2+8+16);
                        }
                        break;

                    case 2:
                        {
                            char *s[] = { "Blue", "Red", "Green", "Gray" };
                            gametext(d-50,yy,s[ud.team],MENUHIGHLIGHT(io),2+8+16);
                        }
                        break;

                    case 3:
                        {
                            char *s[] = { "Off", "All weapons", "Hitscan only" };
                            gametext(d-50,yy,s[ud.config.AutoAim],MENUHIGHLIGHT(io),2+8+16);
                        }
                        break;

                    case 4:
                        gametext(d-50,yy,ud.mouseaiming?"Hold button":"Toggle on/off",MENUHIGHLIGHT(io),2+8+16);
                        break;

                    case 5:
                        gametext(d+70,yy,ud.weaponswitch&1?"On":"Off",MENUHIGHLIGHT(io),2+8+16);
                        break;

                    case 6:
                        gametext(d+70,yy,ud.weaponswitch&2?"On":"Off",MENUHIGHLIGHT(io),2+8+16);
                        break;
                    case 7:
                        Bsprintf(tempbuf,"%d",packetrate);
                        gametext(d+70,yy,tempbuf,MENUHIGHLIGHT(io),2+8+16);
                        break;
                    default:
                        break;
                    }
                    gametextpal(c,yy, opts[ii], enabled?MENUHIGHLIGHT(io):DISABLEDMENUSHADE, 10);
                    io++;
                    yy += 8;
                }
            }

            break;
        }
    case 20004:
    case 20005:
        rotatesprite(160<<16,19<<16,65536L,0,MENUBAR,16,0,10,0,0,xdim-1,ydim-1);
        menutext(160,24,0,0,"MULTIPLAYER MACROS");

        if (current_menu == 20004)
        {
            x = probesm(24,45,8,10);
            if (x == -1)
            {
                cmenu(20002);
                probey = 8;
            }
            else if (x >= 0 && x <= 9)
            {
                strcpy(buf, ud.ridecule[x]);
                inputloc = strlen(buf);
                last_menu_pos = probey;
                current_menu = 20005;
                KB_ClearKeyDown(sc_Enter);
                KB_ClearKeyDown(sc_kpad_Enter);
                KB_FlushKeyboardQueue();
            }
        }
        else
        {
            x = strget(26,40+(8*probey),buf,34,0);
            if (x)
            {
                if (x == 1)
                {
                    Bstrcpy(ud.ridecule[last_menu_pos],buf);
                }
                KB_ClearKeyDown(sc_Enter);
                KB_ClearKeyDown(sc_kpad_Enter);
                KB_FlushKeyboardQueue();
                current_menu = 20004;
            }
        }
        for (i=0;i<10;i++)
        {
            if (current_menu == 20005 && i == last_menu_pos) continue;
            gametextpal(26,40+(i<<3),ud.ridecule[i],MENUHIGHLIGHT(i),0);
        }

        gametext(160,144,"ACTIVATE IN-GAME WITH SHIFT-F#",0,2+8+16);

        break;

#if 0
    case 20010:
        rotatesprite(160<<16,19<<16,65536L,0,MENUBAR,16,0,10,0,0,xdim-1,ydim-1);
        menutext(160,24,0,0,"HOST NETWORK GAME");

        x = probe(46,50,80,2);

        if (x == -1)
        {
            cmenu(20001);
            probey = 2;
        }
        else if (x == 0) cmenu(20011);

        menutext(40,50,0,0,        "GAME OPTIONS");
        minitext(90,60,            "GAME TYPE"    ,2,26);
        minitext(90,60+8,          "EPISODE"      ,2,26);
        minitext(90,60+8+8,        "LEVEL"        ,2,26);
        minitext(90,60+8+8+8,      "MONSTERS"     ,2,26);
        if (ud.m_coop == 0)
            minitext(90,60+8+8+8+8,    "MARKERS"      ,2,26);
        else if (ud.m_coop == 1)
            minitext(90,60+8+8+8+8,    "FRIENDLY FIRE",2,26);
        minitext(90,60+8+8+8+8+8,  "USER MAP"     ,2,26);

        gametext(90+60,60,gametype_names[ud.m_coop],0,26);

        minitext(90+60,60+8,      volume_names[ud.m_volume_number],0,26);
        minitext(90+60,60+8+8,    level_names[MAXLEVELS*ud.m_volume_number+ud.m_level_number],0,26);
        if (ud.m_monsters_off == 0 || ud.m_player_skill > 0)
            minitext(90+60,60+8+8+8,  skill_names[ud.m_player_skill],0,26);
        else minitext(90+60,60+8+8+8,  "NONE",0,28);
        if (ud.m_coop == 0)
        {
            if (ud.m_marker) minitext(90+60,60+8+8+8+8,"ON",0,26);
            else minitext(90+60,60+8+8+8+8,"OFF",0,26);
        }
        else if (ud.m_coop == 1)
        {
            if (ud.m_ffire) minitext(90+60,60+8+8+8+8,"ON",0,26);
            else minitext(90+60,60+8+8+8+8,"OFF",0,26);
        }

        menutext(40,50+80,0,0,"LAUNCH GAME");
        break;

    case 20011:
        c = (320>>1) - 120;
        rotatesprite(160<<16,19<<16,65536L,0,MENUBAR,16,0,10,0,0,xdim-1,ydim-1);
        menutext(160,24,0,0,"NET GAME OPTIONS");

        x = probe(c,57-8,16,8);

        switch (x)
        {
        case -1:
            cmenu(20010);
            break;
        case 0:
            ud.m_coop++;
            if (ud.m_coop == 3) ud.m_coop = 0;
            break;
        case 1:
            if (!VOLUMEONE)
            {
                ud.m_volume_number++;
                if (ud.m_volume_number == num_volumes) ud.m_volume_number = 0;
                if (ud.m_volume_number == 0 && ud.m_level_number > 6)
                    ud.m_level_number = 0;
                if (ud.m_level_number > 10) ud.m_level_number = 0;
            }
            break;
        case 2:
            ud.m_level_number++;
            if (!VOLUMEONE)
            {
                if (ud.m_volume_number == 0 && ud.m_level_number > 6)
                    ud.m_level_number = 0;
            }
            else
            {
                if (ud.m_volume_number == 0 && ud.m_level_number > 5)
                    ud.m_level_number = 0;
            }
            if (ud.m_level_number > 10) ud.m_level_number = 0;
            break;
        case 3:
            if (ud.m_monsters_off == 1 && ud.m_player_skill > 0)
                ud.m_monsters_off = 0;

            if (ud.m_monsters_off == 0)
            {
                ud.m_player_skill++;
                if (ud.m_player_skill > 3)
                {
                    ud.m_player_skill = 0;
                    ud.m_monsters_off = 1;
                }
            }
            else ud.m_monsters_off = 0;

            break;

        case 4:
            if (ud.m_coop == 0)
                ud.m_marker = !ud.m_marker;
            break;

        case 5:
            if (ud.m_coop == 1)
                ud.m_ffire = !ud.m_ffire;
            break;

        case 6:
            // pick the user map
            break;

        case 7:
            cmenu(20010);
            break;
        }

        c += 40;

        //         if(ud.m_coop==1) gametext(c+70,57-7-9,"COOPERATIVE PLAY",0,2+8+16);
        //         else if(ud.m_coop==2) gametext(c+70,57-7-9,"DUKEMATCH (NO SPAWN)",0,2+8+16);
        //         else gametext(c+70,57-7-9,"DUKEMATCH (SPAWN)",0,2+8+16);
        gametext(c+70,57-7-9,gametype_names[ud.m_coop],0,26);

        gametext(c+70,57+16-7-9,volume_names[ud.m_volume_number],0,2+8+16);

        gametext(c+70,57+16+16-7-9,&level_names[MAXLEVELS*ud.m_volume_number+ud.m_level_number][0],0,2+8+16);

        if (ud.m_monsters_off == 0 || ud.m_player_skill > 0)
            gametext(c+70,57+16+16+16-7-9,skill_names[ud.m_player_skill],0,2+8+16);
        else gametext(c+70,57+16+16+16-7-9,"NONE",0,2+8+16);

        if (ud.m_coop == 0)
        {
            if (ud.m_marker)
                gametext(c+70,57+16+16+16+16-7-9,"ON",0,2+8+16);
            else gametext(c+70,57+16+16+16+16-7-9,"OFF",0,2+8+16);
        }

        if (ud.m_coop == 1)
        {
            if (ud.m_ffire)
                gametext(c+70,57+16+16+16+16+16-7-9,"ON",0,2+8+16);
            else gametext(c+70,57+16+16+16+16+16-7-9,"OFF",0,2+8+16);
        }

        c -= 44;

        menutext(c,57-9,MENUHIGHLIGHT(0),PHX(-2),"GAME TYPE");

        sprintf(tempbuf,"EPISODE %ld",ud.m_volume_number+1);
        menutext(c,57+16-9,MENUHIGHLIGHT(1),PHX(-3),tempbuf);

        sprintf(tempbuf,"LEVEL %ld",ud.m_level_number+1);
        menutext(c,57+16+16-9,MENUHIGHLIGHT(2),PHX(-4),tempbuf);

        menutext(c,57+16+16+16-9,MENUHIGHLIGHT(3),PHX(-5),"MONSTERS");

        if (ud.m_coop == 0)
            menutext(c,57+16+16+16+16-9,MENUHIGHLIGHT(4),PHX(-6),"MARKERS");
        else
            menutext(c,57+16+16+16+16-9,MENUHIGHLIGHT(4),1,"MARKERS");

        if (ud.m_coop == 1)
            menutext(c,57+16+16+16+16+16-9,MENUHIGHLIGHT(5),PHX(-6),"FR. FIRE");
        else menutext(c,57+16+16+16+16+16-9,MENUHIGHLIGHT(5),1,"FR. FIRE");

        if (VOLUMEALL)
        {
            menutext(c,57+16+16+16+16+16+16-9,MENUHIGHLIGHT(6),boardfilename[0] == 0,"USER MAP");
            if (boardfilename[0] != 0)
                gametext(c+70+44,57+16+16+16+16+16,boardfilename,0,2+8+16);
        }
        else
        {
            menutext(c,57+16+16+16+16+16+16-9,MENUHIGHLIGHT(6),1,"USER MAP");
        }

        menutext(c,57+16+16+16+16+16+16+16-9,MENUHIGHLIGHT(7),PHX(-8),"ACCEPT");
        break;

    case 20020:
    case 20021: // editing server
    case 20022: // editing port
        rotatesprite(160<<16,19<<16,65536L,0,MENUBAR,16,0,10,0,0,xdim-1,ydim-1);
        menutext(160,24,0,0,"JOIN NETWORK GAME");

        if (current_menu == 20020)
        {
            x = probe(46,50,20,3);

            if (x == -1)
            {
                cmenu(20001);
                probey = 1;
            }
            else if (x == 0)
            {
                strcpy(buf, "localhost");
                inputloc = strlen(buf);
                current_menu = 20021;
            }
            else if (x == 1)
            {
                strcpy(buf, "19014");
                inputloc = strlen(buf);
                current_menu = 20022;
            }
            else if (x == 2)
                {}
            KB_ClearKeyDown(sc_Enter);
            KB_ClearKeyDown(sc_kpad_Enter);
            KB_FlushKeyboardQueue();
        }
        else if (current_menu == 20021)
        {
            x = strget(40+100,50-9,buf,31,0);
            if (x)
            {
                if (x == 1)
                {
                    //strcpy(myname,buf);
                }

                KB_ClearKeyDown(sc_Enter);
                KB_ClearKeyDown(sc_kpad_Enter);
                KB_FlushKeyboardQueue();

                current_menu = 20020;
            }
        }
        else if (current_menu == 20022)
        {
            x = strget(40+100,50+20-9,buf,5,997);
            if (x)
            {
                if (x == 1)
                {
                    //strcpy(myname,buf);
                }

                KB_ClearKeyDown(sc_Enter);
                KB_ClearKeyDown(sc_kpad_Enter);
                KB_FlushKeyboardQueue();

                current_menu = 20020;
            }
        }

        menutext(40,50,0,0,"SERVER");
        if (current_menu != 20021) gametext(40+100,50-9,"server",0,2+8+16);

        menutext(40,50+20,0,0,"PORT");
        if (current_menu != 20022)
        {
            sprintf(tempbuf,"%d",19014);
            gametext(40+100,50+20-9,tempbuf,0,2+8+16);
        }

        menutext(160,50+20+20,0,0,"CONNECT");


        // ADDRESS
        // PORT
        // CONNECT
        break;
#endif
    case 15001:
    case 15000:

        gametext(160,90,"LOAD last game:",0,2+8+16);

        sprintf(tempbuf,"\"%s\"",ud.savegame[lastsavedpos]);
        gametext(160,99,tempbuf,0,2+8+16);

        gametext(160,99+9,"(Y/N)",0,2+8+16);

        if (KB_KeyPressed(sc_Escape) || KB_KeyPressed(sc_N) || RMB)
        {
            if (sprite[g_player[myconnectindex].ps.i].extra <= 0)
            {
                if (enterlevel(MODE_GAME)) backtomenu();
                return;
            }

            KB_ClearKeyDown(sc_N);
            KB_ClearKeyDown(sc_Escape);

            g_player[myconnectindex].ps.gm &= ~MODE_MENU;
            if (ud.multimode < 2 && ud.recstat != 2)
            {
                ready2send = 1;
                totalclock = ototalclock;
            }
        }

        if (KB_KeyPressed(sc_Space) || KB_KeyPressed(sc_Enter) || KB_KeyPressed(sc_kpad_Enter) || KB_KeyPressed(sc_Y) || LMB)
        {
            KB_FlushKeyboardQueue();
            KB_ClearKeysDown();
            FX_StopAllSounds();

            if (ud.multimode > 1)
            {
                loadplayer(-1-lastsavedpos);
                g_player[myconnectindex].ps.gm = MODE_GAME;
            }
            else
            {
                c = loadplayer(lastsavedpos);
                if (c == 0)
                    g_player[myconnectindex].ps.gm = MODE_GAME;
            }
        }

        probe(186,124+9,0,0);

        break;

    case 10000:
    case 10001:

        c = 60;
        rotatesprite(160<<16,19<<16,65536L,0,MENUBAR,16,0,10,0,0,xdim-1,ydim-1);
        menutext(160,24,0,0,"ADULT MODE");

        x = probe(60,50+16,16,2);
        if (x == -1)
        {
            cmenu(201);
            probey = 0;
            break;
        }

        menutext(c,50+16,MENUHIGHLIGHT(0),PHX(-2),"ADULT MODE");
        menutext(c,50+16+16,MENUHIGHLIGHT(1),PHX(-3),"ENTER PASSWORD");

        menutext(c+160+40,50+16,MENUHIGHLIGHT(0),0,ud.lockout?"OFF":"ON");

        if (current_menu == 10001)
        {
            gametext(160,50+16+16+16+16-12,"ENTER PASSWORD",0,2+8+16);
            x = strget((320>>1),50+16+16+16+16,buf,19, 998);

            if (x)
            {
                if (ud.pwlockout[0] == 0 || ud.lockout == 0)
                    strcpy(&ud.pwlockout[0],buf);
                else if (strcmp(buf,&ud.pwlockout[0]) == 0)
                {
                    ud.lockout = 0;
                    buf[0] = 0;

                    for (x=0;x<numanimwalls;x++)
                        if (wall[animwall[x].wallnum].picnum != W_SCREENBREAK &&
                                wall[animwall[x].wallnum].picnum != W_SCREENBREAK+1 &&
                                wall[animwall[x].wallnum].picnum != W_SCREENBREAK+2)
                            if (wall[animwall[x].wallnum].extra >= 0)
                                wall[animwall[x].wallnum].picnum = wall[animwall[x].wallnum].extra;

                }
                current_menu = 10000;
                KB_ClearKeyDown(sc_Enter);
                KB_ClearKeyDown(sc_kpad_Enter);
                KB_FlushKeyboardQueue();
            }
        }
        else
        {
            if (x == 0)
            {
                if (ud.lockout == 1)
                {
                    if (ud.pwlockout[0] == 0)
                    {
                        ud.lockout = 0;
                        for (x=0;x<numanimwalls;x++)
                            if (wall[animwall[x].wallnum].picnum != W_SCREENBREAK &&
                                    wall[animwall[x].wallnum].picnum != W_SCREENBREAK+1 &&
                                    wall[animwall[x].wallnum].picnum != W_SCREENBREAK+2)
                                if (wall[animwall[x].wallnum].extra >= 0)
                                    wall[animwall[x].wallnum].picnum = wall[animwall[x].wallnum].extra;
                    }
                    else
                    {
                        buf[0] = 0;
                        current_menu = 10001;
                        inputloc = 0;
                        KB_FlushKeyboardQueue();
                    }
                }
                else
                {
                    ud.lockout = 1;

                    for (x=0;x<numanimwalls;x++)
                        switch (dynamictostatic[wall[animwall[x].wallnum].picnum])
                        {
                        case FEMPIC1__STATIC:
                            wall[animwall[x].wallnum].picnum = BLANKSCREEN;
                            break;
                        case FEMPIC2__STATIC:
                        case FEMPIC3__STATIC:
                            wall[animwall[x].wallnum].picnum = SCREENBREAK6;
                            break;
                        }
                }
            }

            else if (x == 1)
            {
                current_menu = 10001;
                inputloc = 0;
                KB_FlushKeyboardQueue();
            }
        }

        break;

    case 1000:
    case 1001:
    case 1002:
    case 1003:
    case 1004:
    case 1005:
    case 1006:
    case 1007:
    case 1008:
    case 1009:

        rotatesprite(160<<16,200<<15,65536L,0,MENUSCREEN,16,0,10+64,0,0,xdim-1,ydim-1);
        rotatesprite(160<<16,19<<16,65536L,0,MENUBAR,16,0,10,0,0,xdim-1,ydim-1);
        menutext(160,24,0,0,"LOAD GAME");
        rotatesprite(101<<16,97<<16,65536>>1,512,TILE_LOADSHOT,-32,0,4+10+64,0,0,xdim-1,ydim-1);

        dispnames();

        sprintf(tempbuf,"PLAYERS: %-2ld                      ",savehead.numplr);
        gametext(160,156,tempbuf,0,2+8+16);

        sprintf(tempbuf,"EPISODE: %-2ld / LEVEL: %-2ld / SKILL: %-2ld",1+savehead.volnum,1+savehead.levnum,savehead.plrskl);
        gametext(160,168,tempbuf,0,2+8+16);

        if (savehead.volnum == 0 && savehead.levnum == 7)
            gametext(160,180,savehead.boardfn,0,2+8+16);

        gametext(160,90,"LOAD game:",0,2+8+16);
        sprintf(tempbuf,"\"%s\"",ud.savegame[current_menu-1000]);
        gametext(160,99,tempbuf,0,2+8+16);
        gametext(160,99+9,"(Y/N)",0,2+8+16);

        if (KB_KeyPressed(sc_Space) || KB_KeyPressed(sc_Enter) || KB_KeyPressed(sc_kpad_Enter) || KB_KeyPressed(sc_Y) || LMB)
        {
            lastsavedpos = current_menu-1000;

            KB_FlushKeyboardQueue();
            KB_ClearKeysDown();
            if (ud.multimode < 2 && ud.recstat != 2)
            {
                ready2send = 1;
                totalclock = ototalclock;
            }

            if (ud.multimode > 1)
            {
                if (g_player[myconnectindex].ps.gm&MODE_GAME)
                {
                    loadplayer(-1-lastsavedpos);
                    g_player[myconnectindex].ps.gm = MODE_GAME;
                }
                else
                {
                    tempbuf[0] = 126;
                    tempbuf[1] = lastsavedpos;
                    tempbuf[2] = myconnectindex;
                    for (x=connecthead;x>=0;x=connectpoint2[x])
                    {
                        if (x != myconnectindex) sendpacket(x,tempbuf,3);
                        if ((!networkmode) && (myconnectindex != connecthead)) break; //slaves in M/S mode only send to master
                    }
                    getpackets();

                    loadplayer(lastsavedpos);

                    multiflag = 0;
                }
            }
            else
            {
                c = loadplayer(lastsavedpos);
                if (c == 0)
                    g_player[myconnectindex].ps.gm = MODE_GAME;
            }

            break;
        }

        probe(186,124+9,0,0);

        if (KB_KeyPressed(sc_N) || KB_KeyPressed(sc_Escape) || RMB)
        {
            KB_ClearKeyDown(sc_N);
            KB_ClearKeyDown(sc_Escape);
            sound(EXITMENUSOUND);
            if (g_player[myconnectindex].ps.gm&MODE_GAME)
            {
                g_player[myconnectindex].ps.gm &= ~MODE_MENU;
                if (ud.multimode < 2 && ud.recstat != 2)
                {
                    ready2send = 1;
                    totalclock = ototalclock;
                }
            }
            else
            {
                cmenu(300);
                probey = last_threehundred;
            }
        }

        break;

    case 1500:

        if (KB_KeyPressed(sc_Space) || KB_KeyPressed(sc_Enter) || KB_KeyPressed(sc_kpad_Enter) || KB_KeyPressed(sc_Y) || LMB)
        {
            KB_FlushKeyboardQueue();
            cmenu(100);
        }
        if (KB_KeyPressed(sc_N) || KB_KeyPressed(sc_Escape) || RMB)
        {
            KB_ClearKeyDown(sc_N);
            KB_ClearKeyDown(sc_Escape);
            if (ud.multimode < 2 && ud.recstat != 2)
            {
                ready2send = 1;
                totalclock = ototalclock;
            }
            g_player[myconnectindex].ps.gm &= ~MODE_MENU;
            sound(EXITMENUSOUND);
            break;
        }
        probe(186,124,0,0);
        gametext(160,90,"ABORT this game?",0,2+8+16);
        gametext(160,90+9,"(Y/N)",0,2+8+16);

        break;

    case 2000:
    case 2001:
    case 2002:
    case 2003:
    case 2004:
    case 2005:
    case 2006:
    case 2007:
    case 2008:
    case 2009:

        rotatesprite(160<<16,200<<15,65536L,0,MENUSCREEN,16,0,10+64,0,0,xdim-1,ydim-1);
        rotatesprite(160<<16,19<<16,65536L,0,MENUBAR,16,0,10,0,0,xdim-1,ydim-1);
        menutext(160,24,0,0,"SAVE GAME");

        rotatesprite(101<<16,97<<16,65536L>>1,512,TILE_LOADSHOT,-32,0,4+10+64,0,0,xdim-1,ydim-1);
        sprintf(tempbuf,"PLAYERS: %-2ld                      ",ud.multimode);
        gametext(160,156,tempbuf,0,2+8+16);

        sprintf(tempbuf,"EPISODE: %-2ld / LEVEL: %-2ld / SKILL: %-2ld",1+ud.volume_number,1+ud.level_number,ud.player_skill);
        gametext(160,168,tempbuf,0,2+8+16);

        if (ud.volume_number == 0 && ud.level_number == 7)
            gametext(160,180,boardfilename,0,2+8+16);

        dispnames();

        gametext(160,90,"OVERWRITE previous SAVED game?",0,2+8+16);
        gametext(160,90+9,"(Y/N)",0,2+8+16);

        if (KB_KeyPressed(sc_Space) || KB_KeyPressed(sc_Enter) || KB_KeyPressed(sc_kpad_Enter) || KB_KeyPressed(sc_Y) || LMB)
        {
            inputloc = strlen(&ud.savegame[current_menu-2000][0]);

            cmenu(current_menu-2000+360);

            KB_FlushKeyboardQueue();
            break;
        }
        if (KB_KeyPressed(sc_N) || KB_KeyPressed(sc_Escape) || RMB)
        {
            KB_ClearKeyDown(sc_N);
            KB_ClearKeyDown(sc_Escape);
            cmenu(351);
            sound(EXITMENUSOUND);
        }

        probe(186,124,0,0);

        break;

    case 990:
    case 991:
    case 992:
    case 993:
    case 994:
    case 995:
    case 996:
    case 997:
    case 998:
        c = 160;
        if (!VOLUMEALL || !PLUTOPAK)
        {
            //rotatesprite(c<<16,200<<15,65536L,0,MENUSCREEN,16,0,10+64,0,0,xdim-1,ydim-1);
            rotatesprite(c<<16,19<<16,65536L,0,MENUBAR,16,0,10,0,0,xdim-1,ydim-1);
            menutext(c,24,0,0,current_menu == 998 ? "ABOUT EDUKE32" : "CREDITS");

            l = 8;
        }
        else
        {
            l = 3;
        }

        if (KB_KeyPressed(sc_LeftArrow) ||
                KB_KeyPressed(sc_kpad_4) ||
                KB_KeyPressed(sc_UpArrow) ||
                KB_KeyPressed(sc_PgUp) ||
                KB_KeyPressed(sc_kpad_8) ||
                WHEELUP)
        {
            KB_ClearKeyDown(sc_LeftArrow);
            KB_ClearKeyDown(sc_kpad_4);
            KB_ClearKeyDown(sc_UpArrow);
            KB_ClearKeyDown(sc_PgUp);
            KB_ClearKeyDown(sc_kpad_8);

            sound(KICK_HIT);
            current_menu--;
            if (current_menu < 990) current_menu = 990+l;
        }
        else if (
            KB_KeyPressed(sc_PgDn) ||
            KB_KeyPressed(sc_Enter) ||
            KB_KeyPressed(sc_Space) ||
            KB_KeyPressed(sc_kpad_Enter) ||
            KB_KeyPressed(sc_RightArrow) ||
            KB_KeyPressed(sc_DownArrow) ||
            KB_KeyPressed(sc_kpad_2) ||
            KB_KeyPressed(sc_kpad_9) ||
            KB_KeyPressed(sc_kpad_6) ||
            LMB || WHEELDOWN)
        {
            KB_ClearKeyDown(sc_PgDn);
            KB_ClearKeyDown(sc_Enter);
            KB_ClearKeyDown(sc_RightArrow);
            KB_ClearKeyDown(sc_kpad_Enter);
            KB_ClearKeyDown(sc_kpad_6);
            KB_ClearKeyDown(sc_kpad_9);
            KB_ClearKeyDown(sc_kpad_2);
            KB_ClearKeyDown(sc_DownArrow);
            KB_ClearKeyDown(sc_Space);
            sound(KICK_HIT);
            current_menu++;
            if (current_menu > 990+l) current_menu = 990;
        }

        x = probe(0,0,0,1);

        if (x == -1)
        {
            cmenu(0);
            break;
        }

        if (!VOLUMEALL || !PLUTOPAK)
        {
            switch (current_menu)
            {
            case 990:
                gametext(c,40,                      "ORIGINAL CONCEPT",0,2+8+16);
                gametext(c,40+9,                    "TODD REPLOGLE",0,2+8+16);
                gametext(c,40+9+9,                  "ALLEN H. BLUM III",0,2+8+16);

                gametext(c,40+9+9+9+9,              "PRODUCED & DIRECTED BY",0,2+8+16);
                gametext(c,40+9+9+9+9+9,            "GREG MALONE",0,2+8+16);

                gametext(c,40+9+9+9+9+9+9+9,        "EXECUTIVE PRODUCER",0,2+8+16);
                gametext(c,40+9+9+9+9+9+9+9+9,      "GEORGE BROUSSARD",0,2+8+16);

                gametext(c,40+9+9+9+9+9+9+9+9+9+9,  "BUILD ENGINE",0,2+8+16);
                gametext(c,40+9+9+9+9+9+9+9+9+9+9+9,"KEN SILVERMAN",0,2+8+16);
                break;
            case 991:
                gametext(c,40,                      "GAME PROGRAMMING",0,2+8+16);
                gametext(c,40+9,                    "TODD REPLOGLE",0,2+8+16);

                gametext(c,40+9+9+9,                "3D ENGINE/TOOLS/NET",0,2+8+16);
                gametext(c,40+9+9+9+9,              "KEN SILVERMAN",0,2+8+16);

                gametext(c,40+9+9+9+9+9+9,          "NETWORK LAYER/SETUP PROGRAM",0,2+8+16);
                gametext(c,40+9+9+9+9+9+9+9,        "MARK DOCHTERMANN",0,2+8+16);
                break;
            case 992:
                gametext(c,40,                      "MAP DESIGN",0,2+8+16);
                gametext(c,40+9,                    "ALLEN H BLUM III",0,2+8+16);
                gametext(c,40+9+9,                  "RICHARD GRAY",0,2+8+16);

                gametext(c,40+9+9+9+9,              "3D MODELING",0,2+8+16);
                gametext(c,40+9+9+9+9+9,            "CHUCK JONES",0,2+8+16);
                gametext(c,40+9+9+9+9+9+9,          "SAPPHIRE CORPORATION",0,2+8+16);

                gametext(c,40+9+9+9+9+9+9+9+9,      "ARTWORK",0,2+8+16);
                gametext(c,40+9+9+9+9+9+9+9+9+9,    "DIRK JONES, STEPHEN HORNBACK",0,2+8+16);
                gametext(c,40+9+9+9+9+9+9+9+9+9+9,  "JAMES STOREY, DAVID DEMARET",0,2+8+16);
                gametext(c,40+9+9+9+9+9+9+9+9+9+9+9,"DOUGLAS R WOOD",0,2+8+16);
                break;
            case 993:
                gametext(c,40,                      "SOUND ENGINE",0,2+8+16);
                gametext(c,40+9,                    "JIM DOSE",0,2+8+16);

                gametext(c,40+9+9+9,                "SOUND & MUSIC DEVELOPMENT",0,2+8+16);
                gametext(c,40+9+9+9+9,              "ROBERT PRINCE",0,2+8+16);
                gametext(c,40+9+9+9+9+9,            "LEE JACKSON",0,2+8+16);

                gametext(c,40+9+9+9+9+9+9+9,        "VOICE TALENT",0,2+8+16);
                gametext(c,40+9+9+9+9+9+9+9+9,      "LANI MINELLA - VOICE PRODUCER",0,2+8+16);
                gametext(c,40+9+9+9+9+9+9+9+9+9,    "JON ST. JOHN AS \"DUKE NUKEM\"",0,2+8+16);
                break;
            case 994:
                gametext(c,60,                      "GRAPHIC DESIGN",0,2+8+16);
                gametext(c,60+9,                    "PACKAGING, MANUAL, ADS",0,2+8+16);
                gametext(c,60+9+9,                  "ROBERT M. ATKINS",0,2+8+16);
                gametext(c,60+9+9+9,                "MICHAEL HADWIN",0,2+8+16);

                gametext(c,60+9+9+9+9+9,            "SPECIAL THANKS TO",0,2+8+16);
                gametext(c,60+9+9+9+9+9+9,          "STEVEN BLACKBURN, TOM HALL",0,2+8+16);
                gametext(c,60+9+9+9+9+9+9+9,        "SCOTT MILLER, JOE SIEGLER",0,2+8+16);
                gametext(c,60+9+9+9+9+9+9+9+9,      "TERRY NAGY, COLLEEN COMPTON",0,2+8+16);
                gametext(c,60+9+9+9+9+9+9+9+9+9,    "HASH INC., FORMGEN, INC.",0,2+8+16);
                break;
            case 995:
                gametext(c,49,                      "THE 3D REALMS BETA TESTERS",0,2+8+16);

                gametext(c,49+9+9,                  "NATHAN ANDERSON, WAYNE BENNER",0,2+8+16);
                gametext(c,49+9+9+9,                "GLENN BRENSINGER, ROB BROWN",0,2+8+16);
                gametext(c,49+9+9+9+9,              "ERIK HARRIS, KEN HECKBERT",0,2+8+16);
                gametext(c,49+9+9+9+9+9,            "TERRY HERRIN, GREG HIVELY",0,2+8+16);
                gametext(c,49+9+9+9+9+9+9,          "HANK LEUKART, ERIC BAKER",0,2+8+16);
                gametext(c,49+9+9+9+9+9+9+9,        "JEFF RAUSCH, KELLY ROGERS",0,2+8+16);
                gametext(c,49+9+9+9+9+9+9+9+9,      "MIKE DUNCAN, DOUG HOWELL",0,2+8+16);
                gametext(c,49+9+9+9+9+9+9+9+9+9,    "BILL BLAIR",0,2+8+16);
                break;
            case 996:
                gametext(c,32,                      "COMPANY PRODUCT SUPPORT",0,2+8+16);

                gametext(c,32+9+9,                  "THE FOLLOWING COMPANIES WERE COOL",0,2+8+16);
                gametext(c,32+9+9+9,                "ENOUGH TO GIVE US LOTS OF STUFF",0,2+8+16);
                gametext(c,32+9+9+9+9,              "DURING THE MAKING OF DUKE NUKEM 3D.",0,2+8+16);

                gametext(c,32+9+9+9+9+9+9,          "ALTEC LANSING MULTIMEDIA",0,2+8+16);
                gametext(c,32+9+9+9+9+9+9+9,        "FOR TONS OF SPEAKERS AND THE",0,2+8+16);
                gametext(c,32+9+9+9+9+9+9+9+9,      "THX-LICENSED SOUND SYSTEM",0,2+8+16);
                gametext(c,32+9+9+9+9+9+9+9+9+9,    "FOR INFO CALL 1-800-548-0620",0,2+8+16);

                gametext(c,32+9+9+9+9+9+9+9+9+9+9+9,"CREATIVE LABS, INC.",0,2+8+16);

                gametext(c,32+9+9+9+9+9+9+9+9+9+9+9+9+9,"THANKS FOR THE HARDWARE, GUYS.",0,2+8+16);
                break;
            case 997:
                gametext(c,50,                      "DUKE NUKEM IS A TRADEMARK OF",0,2+8+16);
                gametext(c,50+9,                    "3D REALMS ENTERTAINMENT",0,2+8+16);

                gametext(c,50+9+9+9,                "DUKE NUKEM",0,2+8+16);
                gametext(c,50+9+9+9+9,              "(C) 1996 3D REALMS ENTERTAINMENT",0,2+8+16);

                if (VOLUMEONE)
                {
                    gametext(c,106,                     "PLEASE READ LICENSE.DOC FOR SHAREWARE",0,2+8+16);
                    gametext(c,106+9,                   "DISTRIBUTION GRANTS AND RESTRICTIONS",0,2+8+16);
                }

                gametext(c,VOLUMEONE?134:115,       "MADE IN DALLAS, TEXAS USA",0,2+8+16);
                break;
            case 998:
                l = 10;
                goto cheat_for_port_credits;
            }
            break;
        }

        // Plutonium pak menus
        switch (current_menu)
        {
        case 990:
        case 991:
        case 992:
            rotatesprite(160<<16,200<<15,65536L,0,2504+current_menu-990,0,0,10+64,0,0,xdim-1,ydim-1);
            break;
        case 993:   // JBF 20031220
            rotatesprite(160<<16,200<<15,65536L,0,MENUSCREEN,0,0,10+64,0,0,xdim-1,ydim-1);
            rotatesprite(c<<16,19<<16,65536L,0,MENUBAR,0,0,10,0,0,xdim-1,ydim-1);
            menutext(160,24,0,0,"ABOUT EDUKE32");

cheat_for_port_credits:
            if (g_ScriptVersion == 13) l = (-2);
            gametext(160,38-l,"GAME PROGRAMMING",0,2+8+16);
            p = "Richard \"TerminX\" Gobeille";
            minitext(161-(Bstrlen(p)<<1), 39+10-l, p, 4, 10+16+128);
            minitext(160-(Bstrlen(p)<<1), 38+10-l, p, 8, 10+16+128);

            gametext(160,57-l,"\"JFDUKE3D\" AND \"JFBUILD\" CODE",0,2+8+16);
            p = "Jonathon \"JonoF\" Fowler";
            minitext(161-(Bstrlen(p)<<1), 58+10-l, p, 4, 10+16+128);
            minitext(160-(Bstrlen(p)<<1), 57+10-l, p, 8, 10+16+128);

            gametext(160,76-l,"ORIGINAL \"POLYMOST\" RENDERER",0,2+8+16);
            gametext(160,76+8-l,"NETWORKING, OTHER CODE",0,2+8+16);
            p = "Ken \"Awesoken\" Silverman";
            minitext(161-(Bstrlen(p)<<1), 77+8+10-l, p, 4, 10+16+128);
            minitext(160-(Bstrlen(p)<<1), 76+8+10-l, p, 8, 10+16+128);

            gametext(160,103-l,"RENDERING FEATURES",0,2+8+16);
            p = "Pierre-Loup \"Plagman\" Griffais";
            minitext(161-(Bstrlen(p)<<1), 104+10-l, p, 4, 10+16+128);
            minitext(160-(Bstrlen(p)<<1), 103+10-l, p, 8, 10+16+128);

            gametext(160,122-l,"LICENSE AND OTHER CONTRIBUTORS",0,2+8+16);
            {
                const char *scroller[] =
                {
                    "This program is distributed under the terms of the",
                    "GNU General Public License version 2 as published by the",
                    "Free Software Foundation. See GNU.TXT for details.",
                    "",
                    "Thanks to the following people for their contributions:",
                    "",
                    "Adam Fazakerley",
                    "Charlie Honig",
                    "Ed Coolidge",
                    "James Bentler",
                    "Javier Martinez",
                    "Jeff Hart",
                    "Jonathan Smith",
                    "Jose del Castillo",
                    "Lachlan McDonald",
                    "Matthew Palmer",
                    "Peter Green",
                    "Ryan Gordon",
                    "",
                    "EDuke originally by Matt Saettler",
                    "",
                    "--x--",
                    "",
                    "",
                    "",
                    "",
                    "",
                    ""
                };
                const int numlines = sizeof(scroller)/sizeof(char *);
                for (m=0,i=(totalclock/104)%numlines; m<6; m++,i++)
                {
                    if (i==numlines) i=0;
                    minitext(161-(Bstrlen(scroller[i])<<1), 101+10+10+8+4+(m*7)-l, (char*)scroller[i], 4, 10+16+128);
                    minitext(160-(Bstrlen(scroller[i])<<1), 100+10+10+8+4+(m*7)-l, (char*)scroller[i], 8, 10+16+128);
                }
            }

            p = "Visit www.eduke32.com for news and updates";
            minitext(161-(Bstrlen(p)<<1), 136+10+10+10+10+4-l, p, 4, 10+16+128);
            minitext(160-(Bstrlen(p)<<1), 135+10+10+10+10+4-l, p, 8, 10+16+128);
        }
        break;

    case 0:
        c = (320>>1);
        rotatesprite(c<<16,28<<16,65536L,0,INGAMEDUKETHREEDEE,0,0,10,0,0,xdim-1,ydim-1);
        if (PLUTOPAK)   // JBF 20030804
            rotatesprite((c+100)<<16,36<<16,65536L,0,PLUTOPAKSPRITE+2,(sintable[(totalclock<<4)&2047]>>11),0,2+8,0,0,xdim-1,ydim-1);
        x = probe(c,67,16,6);
        if (x >= 0)
        {
            if (ud.multimode > 1 && x == 0 && ud.recstat != 2)
            {
                if (movesperpacket == 4 && myconnectindex != connecthead)
                    break;

                last_zero = 0;
                cmenu(600);
            }
            else
            {
                last_zero = x;
                switch (x)
                {
                case 0:
                    cmenu(100);
                    break;
                    //case 1: break;//cmenu(20001);break;   // JBF 20031128: I'm taking over the TEN menu option
                case 1:
                    cmenu(202);
                    break;   // JBF 20031205: was 200
                case 2:
                    if (movesperpacket == 4 && connecthead != myconnectindex)
                        break;
                    cmenu(300);
                    break;
                case 3:
                    KB_FlushKeyboardQueue();
                    cmenu(400);
                    break;
                case 4:
                    cmenu(990);
                    break;
                case 5:
                    cmenu(500);
                    break;
                }
            }
        }

        if (KB_KeyPressed(sc_Q)) cmenu(500);

        if (x == -1 && (g_player[myconnectindex].ps.gm&MODE_GAME || ud.recstat == 2))
        {
            g_player[myconnectindex].ps.gm &= ~MODE_MENU;
            if (ud.multimode < 2 && ud.recstat != 2)
            {
                ready2send = 1;
                totalclock = ototalclock;
            }
        }

        if (movesperpacket == 4)
        {
            if (myconnectindex == connecthead)
                menutext(c,67,MENUHIGHLIGHT(0),PHX(-2),"NEW GAME");
            else
                menutext(c,67,MENUHIGHLIGHT(0),1,"NEW GAME");
        }
        else
            menutext(c,67,MENUHIGHLIGHT(0),PHX(-2),"NEW GAME");

        //    menutext(c,67+16,0,1,"NETWORK GAME");

        menutext(c,67+16/*+16*/,MENUHIGHLIGHT(1),PHX(-3),"OPTIONS");

        if (movesperpacket == 4 && connecthead != myconnectindex)
            menutext(c,67+16+16/*+16*/,MENUHIGHLIGHT(2),1,"LOAD GAME");
        else menutext(c,67+16+16/*+16*/,MENUHIGHLIGHT(2),PHX(-4),"LOAD GAME");

        if (!VOLUMEALL)
        {

            menutext(c,67+16+16+16/*+16*/,MENUHIGHLIGHT(3),PHX(-5),"HOW TO ORDER");
        }
        else
        {

            menutext(c,67+16+16+16/*+16*/,MENUHIGHLIGHT(3),PHX(-5),"HELP");
        }

        menutext(c,67+16+16+16+16/*+16*/,MENUHIGHLIGHT(4),PHX(-6),"CREDITS");


        menutext(c,67+16+16+16+16+16/*+16*/,MENUHIGHLIGHT(5),PHX(-7),"QUIT");
        break;

    case 50:
        c = (320>>1);
        rotatesprite(c<<16,32<<16,65536L,0,INGAMEDUKETHREEDEE,0,0,10,0,0,xdim-1,ydim-1);
        if (PLUTOPAK)   // JBF 20030804
            rotatesprite((c+100)<<16,36<<16,65536L,0,PLUTOPAKSPRITE+2,(sintable[(totalclock<<4)&2047]>>11),0,2+8,0,0,xdim-1,ydim-1);
        x = probe(c,67,16,7);
        switch (x)
        {
        case 0:
            if (movesperpacket == 4 && myconnectindex != connecthead)
                break;
            if (ud.multimode < 2 || ud.recstat == 2)
                cmenu(1500);
            else
            {
                cmenu(600);
                last_fifty = 0;
            }
            break;
        case 1:
            if (movesperpacket == 4 && connecthead != myconnectindex)
                break;
            if (ud.recstat != 2)
            {
                last_fifty = 1;
                cmenu(350);
                setview(0,0,xdim-1,ydim-1);
            }
            break;
        case 2:
            if (movesperpacket == 4 && connecthead != myconnectindex)
                break;
            last_fifty = 2;
            cmenu(300);
            break;
        case 3:
            last_fifty = 3;
            cmenu(202);     // JBF 20031205: was 200
            break;
        case 4:
            last_fifty = 4;
            KB_FlushKeyboardQueue();
            cmenu(400);
            break;
        case 5:
            if (numplayers < 2)
            {
                last_fifty = 5;
                cmenu(501);
            }
            break;
        case 6:
            last_fifty = 6;
            cmenu(500);
            break;
        case -1:
            g_player[myconnectindex].ps.gm &= ~MODE_MENU;
            if (ud.multimode < 2 && ud.recstat != 2)
            {
                ready2send = 1;
                totalclock = ototalclock;
            }
            break;
        }

        if (KB_KeyPressed(sc_Q))
            cmenu(500);

        if (movesperpacket == 4 && connecthead != myconnectindex)
        {
            menutext(c,67                  ,MENUHIGHLIGHT(0),1,"NEW GAME");
            menutext(c,67+16               ,MENUHIGHLIGHT(1),1,"SAVE GAME");
            menutext(c,67+16+16            ,MENUHIGHLIGHT(2),1,"LOAD GAME");
        }
        else
        {
            menutext(c,67                  ,MENUHIGHLIGHT(0),PHX(-2),"NEW GAME");
            menutext(c,67+16               ,MENUHIGHLIGHT(1),PHX(-3),"SAVE GAME");
            menutext(c,67+16+16            ,MENUHIGHLIGHT(2),PHX(-4),"LOAD GAME");
        }

        menutext(c,67+16+16+16         ,MENUHIGHLIGHT(3),PHX(-5),"OPTIONS");
        if (!VOLUMEALL)
        {
            menutext(c,67+16+16+16+16      ,MENUHIGHLIGHT(4),PHX(-6),"HOW TO ORDER");
        }
        else
        {
            menutext(c,67+16+16+16+16      ,MENUHIGHLIGHT(4),PHX(-6)," HELP");
        }
        if (numplayers > 1)
            menutext(c,67+16+16+16+16+16   ,MENUHIGHLIGHT(5),1,"QUIT TO TITLE");
        else menutext(c,67+16+16+16+16+16   ,MENUHIGHLIGHT(5),PHX(-7),"QUIT TO TITLE");
        menutext(c,67+16+16+16+16+16+16,MENUHIGHLIGHT(6),PHX(-8),"QUIT GAME");
        break;

    case 100:
        rotatesprite(160<<16,19<<16,65536L,0,MENUBAR,16,0,10,0,0,xdim-1,ydim-1);
        menutext(160,24,0,0,"SELECT AN EPISODE");
        x = probe(160,VOLUMEONE?60:60-(num_volumes*2),20,VOLUMEONE?3:num_volumes+1);
        if (x >= 0)
        {
            if (VOLUMEONE)
            {
                if (x > 0)
                    cmenu(20000);
                else
                {
                    ud.m_volume_number = x;
                    ud.m_level_number = 0;
                    last_onehundred = x;
                    cmenu(110);
                }
            }

            if (!VOLUMEONE)
            {
                if (x == num_volumes /*&& boardfilename[0]*/)
                {
                    //ud.m_volume_number = 0;
                    //ud.m_level_number = 7;
                    currentlist = 1;
                    last_onehundred = x;
                    cmenu(101);
                }
                else
                {
                    ud.m_volume_number = x;
                    ud.m_level_number = 0;
                    last_onehundred = x;
                    cmenu(110);
                }
            }
        }
        else if (x == -1)
        {
            if (g_player[myconnectindex].ps.gm&MODE_GAME) cmenu(50);
            else cmenu(0);
        }

        c = 80;
        if (VOLUMEONE)
        {
            menutext(160,60,MENUHIGHLIGHT(0),PHX(-2),volume_names[0]);
            menutext(160,60+20,MENUHIGHLIGHT(1),1,volume_names[1]);
            menutext(160,60+20+20,MENUHIGHLIGHT(2),1,volume_names[2]);
            if (PLUTOPAK)
                menutext(160,60+20+20,MENUHIGHLIGHT(3),1,volume_names[3]);
        }
        else
        {
            for (i=0;i<num_volumes;i++)
                menutext(160,60-(num_volumes*2)+(20*i),MENUHIGHLIGHT(i),PHX(-2),volume_names[i]);

            menutext(160,60-(num_volumes*2)+(20*i),MENUHIGHLIGHT(i),PHX(-6),"USER MAP");
        }
        break;

    case 101:
        if (boardfilename[0] == 0) strcpy(boardfilename, "./");
        Bcorrectfilename(boardfilename,1);
        getfilenames(boardfilename,"*.map");
        cmenu(102);
        KB_FlushKeyboardQueue();
    case 102:
        rotatesprite(160<<16,19<<16,65536L,0,MENUBAR,16,0,10,0,0,xdim-1,ydim-1);
        menutext(160,24,0,0,"SELECT A USER MAP");

        // black translucent background underneath file lists
        rotatesprite(0<<16, 0<<16, 65536l<<5, 0, BLANK, 0, 0, 10+16+1+32,
                     scale(40-4,xdim,320),scale(12+32-2,ydim,200),
                     scale(320-40+4,xdim,320)-1,scale(12+32+112+4,ydim,200)-1);

        // path
        minitext(52,32,boardfilename,0,26);

        {
            // JBF 20040208: seek to first name matching pressed character
            CACHE1D_FIND_REC *seeker = currentlist ? findfiles : finddirs;
            if ((KB_KeyPressed(sc_Home)|KB_KeyPressed(sc_End)) > 0)
            {
                while (KB_KeyPressed(sc_End)?seeker->next:seeker->prev)
                    seeker = KB_KeyPressed(sc_End)?seeker->next:seeker->prev;
                if (seeker)
                {
                    if (currentlist) findfileshigh = seeker;
                    else finddirshigh = seeker;
                    sound(KICK_HIT);
                }
            }
            else if ((KB_KeyPressed(sc_PgUp)|KB_KeyPressed(sc_PgDn)) > 0)
            {
                seeker = currentlist?findfileshigh:finddirshigh;
                i = 6;
                while (i>0)
                {
                    if (KB_KeyPressed(sc_PgDn)?seeker->next:seeker->prev)
                        seeker = KB_KeyPressed(sc_PgDn)?seeker->next:seeker->prev;
                    i--;
                }
                if (seeker)
                {
                    if (currentlist) findfileshigh = seeker;
                    else finddirshigh = seeker;
                    sound(KICK_HIT);
                }
            }
            else
            {
                char ch2, ch;
                ch = KB_Getch();
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
                        sound(KICK_HIT);
                    }
                }
            }
        }
        gametext(40+4,12+32,"DIRECTORIES",0,2+8+16);

        if (finddirshigh)
        {
            dir = finddirshigh;
            for (i=0; i<2; i++) if (!dir->prev) break;
                else dir=dir->prev;
            for (i=2; i>-2 && dir; i--, dir=dir->next)
            {
                if (dir == finddirshigh) c=0;
                else c=16;
                minitextshade(40,1+12+32+8*(3-i),dir->name,c,0,26);
            }
        }

        gametext(40+4,8+32+40+8-1,"MAP FILES",0,2+8+16);

        if (findfileshigh)
        {
            dir = findfileshigh;
            for (i=0; i<4; i++) if (!dir->prev) break;
                else dir=dir->prev;
            for (i=4; i>-4 && dir; i--, dir=dir->next)
            {
                if (dir == findfileshigh) c=0;
                else c=16;
                minitextshade(40,(8+32+8*5)+8*(6-i),dir->name,c,2,26);
            }
        }

        if (KB_KeyPressed(sc_LeftArrow) || KB_KeyPressed(sc_kpad_4) || ((buttonstat&1) && minfo.dyaw < -256) ||
                KB_KeyPressed(sc_RightArrow) || KB_KeyPressed(sc_kpad_6) || ((buttonstat&1) && minfo.dyaw > 256) ||
                KB_KeyPressed(sc_Tab))
        {
            KB_ClearKeyDown(sc_LeftArrow);
            KB_ClearKeyDown(sc_kpad_4);
            KB_ClearKeyDown(sc_RightArrow);
            KB_ClearKeyDown(sc_kpad_6);
            KB_ClearKeyDown(sc_Tab);
            currentlist = 1-currentlist;
            sound(KICK_HIT);
        }

        onbar = 0;
        probey = 2;
        if (currentlist == 0) x = probe(50,12+32+16+4,0,3);
        else x = probe(50,8+32+40+40+4,0,3);

        if (probey == 1)
        {
            if (currentlist == 0)
            {
                if (finddirshigh)
                    if (finddirshigh->prev) finddirshigh = finddirshigh->prev;
            }
            else
            {
                if (findfileshigh)
                    if (findfileshigh->prev) findfileshigh = findfileshigh->prev;
            }
        }
        else if (probey == 0)
        {
            if (currentlist == 0)
            {
                if (finddirshigh)
                    if (finddirshigh->next) finddirshigh = finddirshigh->next;
            }
            else
            {
                if (findfileshigh)
                    if (findfileshigh->next) findfileshigh = findfileshigh->next;
            }
        }

        if (x == -1)
        {
            clearfilenames();
            boardfilename[0] = 0;
            if (ud.multimode > 1)
            {
                sendboardname();
                cmenu(600);
                probey = last_menu_pos;
            }
            else cmenu(100);
        }
        else if (x >= 0)
        {
            if (currentlist == 0)
            {
                if (!finddirshigh) break;
                strcat(boardfilename, finddirshigh->name);
                strcat(boardfilename, "/");
                Bcorrectfilename(boardfilename, 1);
                cmenu(101);
                KB_FlushKeyboardQueue();
            }
            else
            {
                if (!findfileshigh) break;
                strcat(boardfilename, findfileshigh->name);
                ud.m_volume_number = 0;
                ud.m_level_number = 7;
                if (ud.multimode > 1)
                {
                    sendboardname();
                    cmenu(600);
                    probey = last_menu_pos;
                }
                else cmenu(110);
            }
            clearfilenames();
        }
        break;

    case 110:
        c = (320>>1);
        rotatesprite(c<<16,19<<16,65536L,0,MENUBAR,16,0,10,0,0,xdim-1,ydim-1);
        menutext(c,24,0,0,"SELECT SKILL");
        x = probe(c,70,19,4);
        if (x >= 0)
        {
            switch (x)
            {
            case 0:
                globalskillsound = JIBBED_ACTOR6;
                break;
            case 1:
                globalskillsound = BONUS_SPEECH1;
                break;
            case 2:
                globalskillsound = DUKE_GETWEAPON2;
                break;
            case 3:
                globalskillsound = JIBBED_ACTOR5;
                break;
            }

            sound(globalskillsound);

            ud.m_player_skill = x+1;
            if (x == 3) ud.m_respawn_monsters = 1;
            else ud.m_respawn_monsters = 0;

            ud.m_monsters_off = ud.monsters_off = 0;

            ud.m_respawn_items = 0;
            ud.m_respawn_inventory = 0;

            ud.multimode = 1;

            if (ud.m_volume_number == 3)
            {
                flushperms();
                setview(0,0,xdim-1,ydim-1);
                clearview(0L);
                nextpage();
            }

            newgame(ud.m_volume_number,ud.m_level_number,ud.m_player_skill);
            if (enterlevel(MODE_GAME)) backtomenu();
        }
        else if (x == -1)
        {
            cmenu(100);
            KB_FlushKeyboardQueue();
        }

        menutext(c,70,MENUHIGHLIGHT(0),PHX(-2),skill_names[0]);
        menutext(c,70+19,MENUHIGHLIGHT(1),PHX(-3),skill_names[1]);
        menutext(c,70+19+19,MENUHIGHLIGHT(2),PHX(-4),skill_names[2]);
        menutext(c,70+19+19+19,MENUHIGHLIGHT(3),PHX(-5),skill_names[3]);
        break;
    case 230:
#if defined(POLYMOST) && defined(USE_OPENGL)
        rotatesprite(320<<15,19<<16,65536L,0,MENUBAR,16,0,10,0,0,xdim-1,ydim-1);
        menutext(320>>1,24,0,0,"VIDEO SETUP");

        c = (320>>1)-120;

        {
            int io, ii, yy, d=c+160+40, enabled;
            char *opts[] =
            {
                "Widescreen",
                "-",
                "Anisotropic filtering",
                "-",
                "Hightile textures",
                "Precache textures",
                "GL texture compression",
                "Cache textures on disk",
                "Compress disk cache",
                "Detail mapping",
                "Glow mapping",
                "-",
                "Models",
                "-",
                "-",
                "-",
                NULL
            };

            yy = 37;
            for (ii=io=0; opts[ii]; ii++)
            {
                if (opts[ii][0] == '-' && !opts[ii][1])
                {
                    if (io <= probey) yy += 4;
                    continue;
                }
                if (io < probey) yy += 8;
                io++;
            }

            onbar = 0;
            x = probesm(c,yy+5,0,io);

            if (x == -1)
            {
                cmenu(203);
                probey = 6;
                break;
            }

            yy = 37;
            for (ii=io=0; opts[ii]; ii++)
            {
                if (opts[ii][0] == '-' && !opts[ii][1])
                {
                    yy += 4;
                    continue;
                }
                enabled = 1;
                switch (io)
                {
                case 0:
                    if (x==io) glwidescreen = 1-glwidescreen;
                    modval(0,1,(int *)&glwidescreen,1,probey==io);
                    gametextpal(d,yy, glwidescreen ? "On" : "Off", MENUHIGHLIGHT(io), 0);
                    break;
                case 1:
                    if (x==io)
                    {
                        glanisotropy *= 2;
                        if (glanisotropy > glinfo.maxanisotropy) glanisotropy = 1;
                        gltexapplyprops();
                    }
                    if (glanisotropy == 1) strcpy(tempbuf,"NONE");
                    else sprintf(tempbuf,"%ldx",glanisotropy);
                    gametextpal(d,yy, tempbuf, MENUHIGHLIGHT(io), 0);
                    break;

                case 2:
                    if (x==io) usehightile = 1-usehightile;
                    modval(0,1,(int *)&usehightile,1,probey==io);
                    gametextpal(d,yy, usehightile ? "On" : "Off", MENUHIGHLIGHT(io), 0);
                    break;
                case 3:
                    enabled = usehightile;
                    if (enabled && x==io) ud.config.useprecache = !ud.config.useprecache;
                    if (enabled) modval(0,1,(int *)&ud.config.useprecache,1,probey==io);
                    gametextpal(d,yy, ud.config.useprecache && enabled ? "On" : "Off", enabled?MENUHIGHLIGHT(io):DISABLEDMENUSHADE, 0);
                    break;
                case 4:
                    enabled = usehightile;
                    if (enabled && x==io) glusetexcompr = !glusetexcompr;
                    if (enabled) modval(0,1,(int *)&glusetexcompr,1,probey==io);
                    gametextpal(d,yy, glusetexcompr && enabled ? "On" : "Off", enabled?MENUHIGHLIGHT(io):DISABLEDMENUSHADE, 0);
                    break;
                case 5:
                    enabled = (glusetexcompr && usehightile && ud.config.useprecache);
                    if (enabled && x==io) glusetexcache = !glusetexcache;
                    if (enabled) modval(0,1,(int *)&glusetexcache,1,probey==io);
                    gametextpal(d,yy, glusetexcache && enabled ? "On" : "Off", enabled?MENUHIGHLIGHT(io):DISABLEDMENUSHADE, 0);
                    break;
                case 6:
                    enabled = (glusetexcompr && usehightile && ud.config.useprecache && glusetexcache);
                    if (enabled && x==io) glusetexcachecompression = !glusetexcachecompression;
                    if (enabled) modval(0,1,(int *)&glusetexcachecompression,1,probey==io);
                    gametextpal(d,yy, glusetexcachecompression && enabled ? "On" : "Off", enabled?MENUHIGHLIGHT(io):DISABLEDMENUSHADE, 0);
                    break;
                case 7:
                    enabled = usehightile;
                    if (enabled && x==io) r_detailmapping = !r_detailmapping;
                    if (enabled) modval(0,1,(int *)&r_detailmapping,1,probey==io);
                    gametextpal(d,yy, r_detailmapping && enabled ? "On" : "Off", enabled?MENUHIGHLIGHT(io):DISABLEDMENUSHADE, 0);
                    break;
                case 8:
                    enabled = usehightile;
                    if (enabled && x==io) r_glowmapping = !r_glowmapping;
                    if (enabled) modval(0,1,(int *)&r_glowmapping,1,probey==io);
                    gametextpal(d,yy, r_glowmapping && enabled ? "On" : "Off", enabled?MENUHIGHLIGHT(io):DISABLEDMENUSHADE, 0);
                    break;
                case 9:
                    if (x==io) usemodels = 1-usemodels;
                    modval(0,1,(int *)&usemodels,1,probey==io);
                    gametextpal(d,yy, usemodels ? "On" : "Off", MENUHIGHLIGHT(io), 0);
                    break;
                default:
                    break;
                }
                gametextpal(c,yy, opts[ii], enabled?MENUHIGHLIGHT(io):DISABLEDMENUSHADE, 10);
                io++;
                yy += 8;
            }
        }
#endif
        break;

    case 200:

        rotatesprite(320<<15,19<<16,65536L,0,MENUBAR,16,0,10,0,0,xdim-1,ydim-1);
        menutext(320>>1,24,0,0,"GAME SETUP");

        c = (320>>1)-120;

        {
            int io, ii, yy, d=c+160+40, enabled;
            char *opts[] =
            {
                "Crosshair",
                "Level stats",
                "-",
                "Screen size",
                "Status bar size",
                "-",
                "Run key style",
                "-",
                "Shadows",
                "Screen tilting",
                "-",
                "Show opponent weapon",
                "Demo playback cameras",
                "Record demo",
                "-",
                "-",
                "-",
                "-",
                "-",
                "More...",
                NULL
            };

            yy = 37;
            for (ii=io=0; opts[ii]; ii++)
            {
                if (opts[ii][0] == '-' && !opts[ii][1])
                {
                    if (io <= probey) yy += 4;
                    continue;
                }
                if (io < probey) yy += 8;
                io++;
            }

            onbar = (probey == 2 || probey == 3);
            x = probesm(c,yy+5,0,io);

            if (x == -1)
            {
                cmenu(202);
                break;
            }

            yy = 37;
            for (ii=io=0; opts[ii]; ii++)
            {
                if (opts[ii][0] == '-' && !opts[ii][1])
                {
                    yy += 4;
                    continue;
                }
                enabled = 1;
                switch (io)
                {
                case 0:
                    if (x==io) ud.crosshair = (ud.crosshair==3)?0:ud.crosshair+1;
                    modval(0,3,(int *)&ud.crosshair,1,probey==io);
                    {
                        char *s[] = { "OFF", "ON [100%]", "ON [50%]", "ON [25%]" };
                        gametextpal(d,yy,s[ud.crosshair], MENUHIGHLIGHT(io), 0);
                        break;
                    }
                case 1:
                    if (x==io) ud.levelstats = 1-ud.levelstats;
                    modval(0,1,(int *)&ud.levelstats,1,probey==io);
                    gametextpal(d,yy, ud.levelstats ? "Shown" : "Hidden", MENUHIGHLIGHT(io), 0);
                    break;
                case 2:
                    {
                        int i;
                        i = ud.screen_size;
                        barsm(d+8,yy+7, (short *)&ud.screen_size,-4,x==io,MENUHIGHLIGHT(io),PHX(-5));
                        if (i < ud.screen_size && i == 8 && ud.statusbarmode == 1 && bpp > 8)
                        {
                            ud.statusbarmode = 0;
                            if (ud.statusbarscale != 100)
                                ud.screen_size = i;
                        }
                        else if (i > ud.screen_size && i == 8 && ud.statusbarmode == 0 && bpp > 8)
                        {
                            if (ud.statusbarscale != 100)
                            {
                                ud.statusbarmode = 1;
                                ud.screen_size = i;
                            }
                        }
                    }
                    break;
                case 3:
                    {
                        short sbs, sbsl;
                        sbs = sbsl = ud.statusbarscale-37;
                        barsm(d+8,yy+7, (short *)&sbs,8,x==io,MENUHIGHLIGHT(io),PHX(-5));
                        if (x == io && sbs != sbsl)
                        {
                            sbs += 37;
                            setstatusbarscale(sbs);
                        }
                    }
                    break;
                case 4:
                    if (x==io) ud.runkey_mode = 1-ud.runkey_mode;
                    modval(0,1,(int *)&ud.runkey_mode,1,probey==io);
                    gametextpal(d,yy, ud.runkey_mode ? "Classic" : "Modern", MENUHIGHLIGHT(io), 0);
                    break;
                case 5:
                    if (x==io) ud.shadows = 1-ud.shadows;
                    modval(0,1,(int *)&ud.shadows,1,probey==io);
                    gametextpal(d,yy, ud.shadows ? "On" : "Off", MENUHIGHLIGHT(io), 0);
                    break;
                case 6:
                    if (x==io) ud.screen_tilting = 1-ud.screen_tilting;
                    modval(0,1,(int *)&ud.screen_tilting,1,probey==io);
                    gametextpal(d,yy, ud.screen_tilting ? "On" : "Off", MENUHIGHLIGHT(io), 0);
                    break;  // original had a 'full' option
                case 7:
                    if (x==io) ud.showweapons = 1-ud.showweapons;
                    modval(0,1,(int *)&ud.showweapons,1,probey==io);
                    ud.config.ShowOpponentWeapons = ud.showweapons;
                    gametextpal(d,yy, ud.config.ShowOpponentWeapons ? "On" : "Off", MENUHIGHLIGHT(io), 0);
                    break;
                case 8:
                    if (x==io) ud.democams = 1-ud.democams;
                    modval(0,1,(int *)&ud.democams,1,probey==io);
                    gametextpal(d,yy, ud.democams ? "On" : "Off", MENUHIGHLIGHT(io), 0);
                    break;
                case 9:
                    if (x==io)
                    {
                        enabled = !((g_player[myconnectindex].ps.gm&MODE_GAME) && ud.m_recstat != 1);
                        if ((g_player[myconnectindex].ps.gm&MODE_GAME)) closedemowrite();
                        else ud.m_recstat = !ud.m_recstat;
                    }
                    if ((g_player[myconnectindex].ps.gm&MODE_GAME) && ud.m_recstat != 1)
                        enabled = 0;
                    gametextpal(d,yy,ud.m_recstat?((ud.m_recstat && enabled && g_player[myconnectindex].ps.gm&MODE_GAME)?"Running":"On"):"Off",enabled?MENUHIGHLIGHT(io):DISABLEDMENUSHADE,enabled?0:1);
                    break;
                case 10:
                    if (x==io) cmenu(201);
                    break;
                default:
                    break;
                }
                gametextpal(c,yy, opts[ii], enabled?MENUHIGHLIGHT(io):DISABLEDMENUSHADE, 10);
                io++;
                yy += 8;
            }
        }
        break;

    case 201:

        rotatesprite(320<<15,19<<16,65536L,0,MENUBAR,16,0,10,0,0,xdim-1,ydim-1);
        menutext(320>>1,24,0,0,"GAME SETUP");

        c = (320>>1)-120;

        {
            int io, ii, yy, d=c+160+40, enabled;
            char *opts[] =
            {
                "Parental lock",
                "-",
                "Game messages",
                "HUD weapon",
                "FPS counter",
                "-",
                "Automatic voting",
                "Send MP messages to all",
                "Display other player IDs",
                "-",
                "Show startup window",
#ifdef _WIN32
                "Release notification",
#else
                "-",
                "-",
#endif
                "-",
                "-",
                "-",
                "-",
                "-",
                "-",
                "-",
                "-",
                "Previous page",
                NULL
            };

            yy = 37;
            for (ii=io=0; opts[ii]; ii++)
            {
                if (opts[ii][0] == '-' && !opts[ii][1])
                {
                    if (io <= probey) yy += 4;
                    continue;
                }
                if (io < probey) yy += 8;
                io++;
            }

            x = probesm(c,yy+5,0,io);

            if (x == -1)
            {
                cmenu(200);
                probey = 10;
                break;
            }

            yy = 37;
            for (ii=io=0; opts[ii]; ii++)
            {
                if (opts[ii][0] == '-' && !opts[ii][1])
                {
                    yy += 4;
                    continue;
                }
                enabled = 1;
                switch (io)
                {
                case 0:
                    if (!NAM)
                    {
                        if (x==io) cmenu(10000);
                    }
                    else enabled = 0;
                    break;
                case 1:
                    if (x==io)
                        ud.fta_on = 1-ud.fta_on;
                    modval(0,1,(int *)&ud.fta_on,1,probey==io);
                    gametextpal(d,yy, ud.fta_on ? "On" : "Off", MENUHIGHLIGHT(io), 0);
                    break;
                case 2:
                    if (x==io)
                    {
                        ud.drawweapon = (ud.drawweapon == 2) ? 0 : ud.drawweapon+1;
                    }
                    modval(0,2,(int *)&ud.drawweapon,1,probey==io);
                    {
                        char *s[] = { "Off", "On", "Icon" };
                        gametextpal(d,yy, s[ud.drawweapon], MENUHIGHLIGHT(io), 0);
                        break;
                    }
                case 3:
                    if (x==io) ud.tickrate = 1-ud.tickrate;
                    modval(0,1,(int *)&ud.tickrate,1,probey==io);
                    gametextpal(d,yy, ud.tickrate ? "On" : "Off", MENUHIGHLIGHT(io), 0);
                    break;
                case 4:
                    if (x==io)
                    {
                        ud.autovote = (ud.autovote == 2) ? 0 : ud.autovote+1;
                    }
                    modval(0,2,(int *)&ud.autovote,1,probey==io);
                    {
                        char *s[] = { "Off", "Vote No", "Vote Yes" };
                        gametextpal(d,yy, s[ud.autovote], MENUHIGHLIGHT(io), 0);
                        break;
                    }
                case 5:
                    if (x==io) ud.automsg = 1-ud.automsg;
                    modval(0,1,(int *)&ud.automsg,1,probey==io);
                    gametextpal(d,yy, ud.automsg ? "On" : "Off", MENUHIGHLIGHT(io), 0);
                    break;
                case 6:
                    if (x==io) ud.idplayers = 1-ud.idplayers;
                    modval(0,1,(int *)&ud.idplayers,1,probey==io);
                    gametextpal(d,yy, ud.idplayers ? "On" : "Off", MENUHIGHLIGHT(io), 0);
                    break;
                case 7:
                    if (x==io) ud.config.ForceSetup = 1-ud.config.ForceSetup;
                    modval(0,1,(int *)&ud.config.ForceSetup,1,probey==io);
                    gametextpal(d,yy, ud.config.ForceSetup ? "On" : "Off", MENUHIGHLIGHT(io), 0);
                    break;
#ifdef _WIN32
                case 8:
                    i = ud.config.CheckForUpdates;
                    if (x==io) ud.config.CheckForUpdates = 1-ud.config.CheckForUpdates;
                    modval(0,1,(int *)&ud.config.CheckForUpdates,1,probey==io);
                    if (ud.config.CheckForUpdates != i)
                        ud.config.LastUpdateCheck = 0;
                    gametextpal(d,yy, ud.config.CheckForUpdates ? "On" : "Off", MENUHIGHLIGHT(io), 0);
                    break;
                case 9:
#else
                case 8:
#endif
                    if (x==io) cmenu(200);
                    break;
                default:
                    break;
                }
                gametextpal(c,yy, opts[ii], enabled?MENUHIGHLIGHT(io):DISABLEDMENUSHADE, 10);
                io++;
                yy += 8;
            }
        }
        break;

        // JBF 20031205: Second level options menu selection
    case 202:
        rotatesprite(320<<15,19<<16,65536L,0,MENUBAR,16,0,10,0,0,xdim-1,ydim-1);
        menutext(320>>1,24,0,0,"OPTIONS");

        c = 50;

        onbar = 0;
        x = probe(160,c,18,7);

        last_twoohtwo = probey;

        switch (x)
        {
        case -1:
            if (g_player[myconnectindex].ps.gm&MODE_GAME) cmenu(50);
            else cmenu(0);
            break;

        case 0:
            cmenu(200);
            break;

        case 1:
            cmenu(700);
            break;

        case 2:
            {
                int dax = xdim, day = ydim, daz;
                curvidmode = newvidmode = checkvideomode(&dax,&day,bpp,fullscreen,0);
                if (newvidmode == 0x7fffffffl) newvidmode = validmodecnt;
                newfullscreen = fullscreen;
                changesmade = 0;

                dax = 0;
                for (day = 0; day < validmodecnt; day++)
                {
                    if (dax == sizeof(vidsets)/sizeof(vidsets[1])) break;
                    for (daz = 0; daz < dax; daz++)
                        if ((validmode[day].bpp|((validmode[day].fs&1)<<16)) == (vidsets[daz]&0x1ffffl)) break;
                    if (vidsets[daz] != -1) continue;
                    if (validmode[day].bpp == 8)
                    {
                        vidsets[dax++] = 8|((validmode[day].fs&1)<<16);
//                        8-bit Polymost can diaf
//                        vidsets[dax++] = 0x20000|8|((validmode[day].fs&1)<<16);
                    }
                    else
                        vidsets[dax++] = 0x20000|validmode[day].bpp|((validmode[day].fs&1)<<16);
                }
                for (dax = 0; dax < (long)(sizeof(vidsets)/sizeof(vidsets[1])) && vidsets[dax] != -1; dax++)
                    if (vidsets[dax] == (((getrendermode()>=2)<<17)|(fullscreen<<16)|bpp)) break;
                if (dax < (long)(sizeof(vidsets)/sizeof(vidsets[1]))) newvidset = dax;
                curvidset = newvidset;

                cmenu(203);
            }
            break;
        case 3:
            if (ud.recstat != 1)
                cmenu(20002);
            break;
        case 4:
            currentlist = 0;
        case 5:
        case 6:
            if (x==6 && !CONTROL_JoyPresent) break;
            cmenu(204+x-4);
            break;
        }

        menutext(160,c,                  MENUHIGHLIGHT(0),0,"GAME SETUP");
        menutext(160,c+18,               MENUHIGHLIGHT(1),0,"SOUND SETUP");
        menutext(160,c+18+18,            MENUHIGHLIGHT(2),0,"VIDEO SETUP");
        menutext(160,c+18+18+18,         MENUHIGHLIGHT(3),ud.recstat == 1,"PLAYER SETUP");
        menutext(160,c+18+18+18+18,      MENUHIGHLIGHT(4),0,"KEYBOARD SETUP");
        menutext(160,c+18+18+18+18+18,   MENUHIGHLIGHT(5),0,"MOUSE SETUP");
        menutext(160,c+18+18+18+18+18+18,MENUHIGHLIGHT(6),CONTROL_JoyPresent==0,"JOYSTICK SETUP");
        break;

        // JBF 20031206: Video settings menu
    case 203:
        rotatesprite(320<<15,19<<16,65536L,0,MENUBAR,16,0,10,0,0,xdim-1,ydim-1);
        menutext(320>>1,24,0,0,"VIDEO SETUP");

        c = (320>>1)-120;

#if defined(POLYMOST) && defined(USE_OPENGL)
        x = (6+(bpp>8));
#else
        x = 6;
#endif
        onbar = (probey == 4);
        if (probey == 0 || probey == 1 || probey == 2)
            x = probe(c,50,16,x);
        else if (probey == 3)
            x = probe(c,50+16+16+22,0,x);
        else
            x = probe(c,50+62-16-16-16,16,x);

        if (probey==0 && (KB_KeyPressed(sc_LeftArrow) || KB_KeyPressed(sc_RightArrow)))
        {
            sound(PISTOL_BODYHIT);
            x=0;
        }
        switch (x)
        {
        case -1:
            cmenu(202);
            probey = 2;
            break;

        case 0:
            do
            {
                if (KB_KeyPressed(sc_LeftArrow))
                {
                    newvidmode--;
                    if (newvidmode < 0) newvidmode = validmodecnt-1;
                }
                else
                {
                    newvidmode++;
                    if (newvidmode >= validmodecnt) newvidmode = 0;
                }
            }
            while ((validmode[newvidmode].fs&1) != ((vidsets[newvidset]>>16)&1) || validmode[newvidmode].bpp != (vidsets[newvidset] & 0x0ffff));
            //OSD_Printf("New mode is %dx%dx%d-%d %d\n",validmode[newvidmode].xdim,validmode[newvidmode].ydim,validmode[newvidmode].bpp,validmode[newvidmode].fs,newvidmode);
            if ((curvidmode == 0x7fffffffl && newvidmode == validmodecnt) || curvidmode == newvidmode)
                changesmade &= ~1;
            else
                changesmade |= 1;
            KB_ClearKeyDown(sc_LeftArrow);
            KB_ClearKeyDown(sc_RightArrow);
            break;

        case 1:
            {
                int lastvidset, lastvidmode, safevidmode = -1;
                lastvidset = newvidset;
                lastvidmode = newvidmode;
                // find the next vidset compatible with the current fullscreen setting
                while (vidsets[0] != -1)
                {
                    newvidset++;
                    if (newvidset == sizeof(vidsets)/sizeof(vidsets[0]) || vidsets[newvidset] == -1)
                    {
                        newvidset = -1;
                        continue;
                    }
                    if (((vidsets[newvidset]>>16)&1) != newfullscreen) continue;
                    break;
                }

                if ((vidsets[newvidset] & 0x0ffff) != (vidsets[lastvidset] & 0x0ffff))
                {
                    // adjust the video mode to something legal for the new vidset
                    do
                    {
                        newvidmode++;
                        if (newvidmode == lastvidmode) break;   // end of cycle
                        if (newvidmode >= validmodecnt)
                        {
                            if (safevidmode != -1)
                                break;
                            newvidmode = 0;
                        }
                        if (validmode[newvidmode].bpp == (vidsets[newvidset]&0x0ffff) &&
                                validmode[newvidmode].fs == newfullscreen &&
                                validmode[newvidmode].xdim <= validmode[lastvidmode].xdim &&
                                (safevidmode==-1?1:(validmode[newvidmode].xdim>=validmode[safevidmode].xdim)) &&
                                validmode[newvidmode].ydim <= validmode[lastvidmode].ydim &&
                                (safevidmode==-1?1:(validmode[newvidmode].ydim>=validmode[safevidmode].ydim))
                           )
                            safevidmode = newvidmode;
                    }
                    while (1);
                    if (safevidmode == -1)
                    {
                        //OSD_Printf("No best fit!\n");
                        newvidmode = lastvidmode;
                        newvidset = lastvidset;
                    }
                    else
                    {
                        //OSD_Printf("Best fit is %dx%dx%d-%d %d\n",validmode[safevidmode].xdim,validmode[safevidmode].ydim,validmode[safevidmode].bpp,validmode[safevidmode].fs,safevidmode);
                        newvidmode = safevidmode;
                    }
                }
                if (newvidset != curvidset) changesmade |= 4;
                else changesmade &= ~4;
                if (newvidmode != curvidmode) changesmade |= 1;
                else changesmade &= ~1;
            }
            break;

        case 2:
            newfullscreen = !newfullscreen;
            {
                int lastvidset, lastvidmode, safevidmode = -1, safevidset = -1;
                lastvidset = newvidset;
                lastvidmode = newvidmode;
                // find the next vidset compatible with the current fullscreen setting
                while (vidsets[0] != -1)
                {
                    newvidset++;
                    if (newvidset == lastvidset) break;
                    if (newvidset == sizeof(vidsets)/sizeof(vidsets[0]) || vidsets[newvidset] == -1)
                    {
                        newvidset = -1;
                        continue;
                    }
                    if (((vidsets[newvidset]>>16)&1) != newfullscreen) continue;
                    if ((vidsets[newvidset] & 0x2ffff) != (vidsets[lastvidset] & 0x2ffff))
                    {
                        if ((vidsets[newvidset] & 0x20000) == (vidsets[lastvidset] & 0x20000)) safevidset = newvidset;
                        continue;
                    }
                    break;
                }
                if (newvidset == lastvidset)
                {
                    if (safevidset == -1)
                    {
                        newfullscreen = !newfullscreen;
                        break;
                    }
                    else
                    {
                        newvidset = safevidset;
                    }
                }

                // adjust the video mode to something legal for the new vidset
                do
                {
                    newvidmode++;
                    if (newvidmode == lastvidmode) break;   // end of cycle
                    if (newvidmode >= validmodecnt) newvidmode = 0;
                    if (validmode[newvidmode].bpp == (vidsets[newvidset]&0x0ffff) &&
                            validmode[newvidmode].fs == newfullscreen &&
                            validmode[newvidmode].xdim <= validmode[lastvidmode].xdim &&
                            (safevidmode==-1?1:(validmode[newvidmode].xdim>=validmode[safevidmode].xdim)) &&
                            validmode[newvidmode].ydim <= validmode[lastvidmode].ydim &&
                            (safevidmode==-1?1:(validmode[newvidmode].ydim>=validmode[safevidmode].ydim))
                       )
                        safevidmode = newvidmode;
                }
                while (1);
                if (safevidmode == -1)
                {
                    //OSD_Printf("No best fit!\n");
                    newvidmode = lastvidmode;
                    newvidset = lastvidset;
                    newfullscreen = !newfullscreen;
                }
                else
                {
                    //OSD_Printf("Best fit is %dx%dx%d-%d %d\n",validmode[safevidmode].xdim,validmode[safevidmode].ydo,,validmode[safevidmode].bpp,validmode[safevidmode].fs,safevidmode);
                    newvidmode = safevidmode;
                }
                if (newvidset != curvidset) changesmade |= 4;
                else changesmade &= ~4;
                if (newvidmode != curvidmode) changesmade |= 1;
                else changesmade &= ~1;
            }
            if (newfullscreen == fullscreen) changesmade &= ~2;
            else changesmade |= 2;
            break;

        case 3:
            if (!changesmade) break;
            {
                long pxdim, pydim, pfs, pbpp, prend;
                long nxdim, nydim, nfs, nbpp, nrend;

                pxdim = xdim;
                pydim = ydim;
                pbpp = bpp;
                pfs = fullscreen;
                prend = getrendermode();
                nxdim = (newvidmode==validmodecnt)?xdim:validmode[newvidmode].xdim;
                nydim = (newvidmode==validmodecnt)?ydim:validmode[newvidmode].ydim;
                nfs   = newfullscreen;
                nbpp  = (newvidmode==validmodecnt)?bpp:validmode[newvidmode].bpp;
                nrend = (vidsets[newvidset] & 0x20000) ? (nbpp==8?2:3) : 0;

                if (setgamemode(nfs, nxdim, nydim, nbpp) < 0)
                {
                    if (setgamemode(pfs, pxdim, pydim, pbpp) < 0)
                    {
                        setrendermode(prend);
                        gameexit("Failed restoring old video mode.");
                    }
                    else onvideomodechange(pbpp > 8);
                }
                else onvideomodechange(nbpp > 8);

                restorepalette = 1;
                vscrn();
                setrendermode(nrend);

                curvidmode = newvidmode;
                curvidset = newvidset;
                changesmade = 0;

                ud.config.ScreenMode = fullscreen;
                ud.config.ScreenWidth = xdim;
                ud.config.ScreenHeight = ydim;
                ud.config.ScreenBPP = bpp;
            }
            break;

        case 4:
            break;

        case 5:
            if (bpp==8)
            {
                ud.detail = 1-ud.detail;
                break;
            }
#if defined(POLYMOST) && defined(USE_OPENGL)
            /*            switch (gltexfiltermode)
                        {
                        case 0:
                            gltexfiltermode = 3;
                            break;
                        case 3:
                            gltexfiltermode = 5;
                            break;
                        case 5:
                            gltexfiltermode = 0;
                            break;
                        default:
                            gltexfiltermode = 3;
                            break;
                        }*/
            gltexfiltermode++;
            if (gltexfiltermode > 5)
                gltexfiltermode = 0;
            gltexapplyprops();
            break;
        case 6:
            if (bpp==8) break;
            cmenu(230);
            break;
#endif
        }

        menutext(c,50,MENUHIGHLIGHT(0),0,"RESOLUTION");
        sprintf(tempbuf,"%ld x %ld",
                (newvidmode==validmodecnt)?xdim:validmode[newvidmode].xdim,
                (newvidmode==validmodecnt)?ydim:validmode[newvidmode].ydim);
        gametext(c+154,50-8,tempbuf,MENUHIGHLIGHT(0),2+8+16);

        menutext(c,50+16,MENUHIGHLIGHT(1),0,"VIDEO MODE");
        sprintf(tempbuf, "%dbit %s", vidsets[newvidset]&0x0ffff, (vidsets[newvidset]&0x20000)?"Polymost":"Classic");
        gametext(c+154,50+16-8,tempbuf,MENUHIGHLIGHT(1),2+8+16);

        menutext(c,50+16+16,MENUHIGHLIGHT(2),0,"FULLSCREEN");
        menutext(c+154,50+16+16,MENUHIGHLIGHT(2),0,newfullscreen?"YES":"NO");

        menutext(c+16,50+16+16+22,MENUHIGHLIGHT(3),changesmade==0,"APPLY CHANGES");

        menutext(c,50+62+16,MENUHIGHLIGHT(4),PHX(-6),"BRIGHTNESS");
        {
            short ss = ud.brightness;
            bar(c+171,50+62+16,&ss,8,x==4,MENUHIGHLIGHT(4),PHX(-6));
            if (x==4)
            {
                ud.brightness = ss;
                setbrightness(ud.brightness>>2,&g_player[myconnectindex].ps.palette[0],0);
            }
        }

        if (bpp == 8)
        {
            menutext(c,50+62+16+16,MENUHIGHLIGHT(5),0,"DETAIL");
            menutext(c+154,50+62+16+16,MENUHIGHLIGHT(5),0,ud.detail?"HIGH":"LOW");
            modval(0,1,(int *)&ud.detail,1,probey==5);
        }
#if defined(POLYMOST) && defined(USE_OPENGL)
        else
        {
            menutext(c,50+62+16+16,MENUHIGHLIGHT(5),bpp==8,"TEX FILTERING");
            switch (gltexfiltermode)
            {
            case 0:
                strcpy(tempbuf,"NEAREST");
                break;
            case 1:
                strcpy(tempbuf,"LINEAR");
                break;
            case 2:
                strcpy(tempbuf,"NEAR_MM_NEAR");
                break;
            case 3:
                strcpy(tempbuf,"BILINEAR");
                break;
            case 4:
                strcpy(tempbuf,"NEAR_MM_LIN");
                break;
            case 5:
                strcpy(tempbuf,"TRILINEAR");
                break;
            default:
                strcpy(tempbuf,"OTHER");
                break;
            }
            gametextpal(c+154,50+62+16+16-8,tempbuf,MENUHIGHLIGHT(5),bpp==8);
            menutext(c,50+62+16+16+16,MENUHIGHLIGHT(6),bpp==8,"MORE SETTINGS");
        }
#endif
        break;

    case 204:
        rotatesprite(320<<15,19<<16,65536L,0,MENUBAR,16,0,10,0,0,xdim-1,ydim-1);
        menutext(320>>1,24,0,0,"KEYBOARD SETUP");

        c = (320>>1)-120;

        onbar = 0;

        if (probey == NUMGAMEFUNCTIONS)
            x = probesm(60,145,0,NUMGAMEFUNCTIONS+2);
        else if (probey == NUMGAMEFUNCTIONS+1)
            x = probesm(60,152,0,NUMGAMEFUNCTIONS+2);
        else x = probesm(0,0,0,NUMGAMEFUNCTIONS+2);

        if (x==-1)
        {
            cmenu(202);
            probey = 4;
        }
        else if (x == NUMGAMEFUNCTIONS)
        {
            CONFIG_SetDefaultKeys(0);
            break;
        }
        else if (x == NUMGAMEFUNCTIONS+1)
        {
            CONFIG_SetDefaultKeys(1);
            break;
        }
        else if (x>=0)
        {
            function = probey;
            whichkey = currentlist;
            cmenu(210);
            KB_FlushKeyboardQueue();
            KB_ClearLastScanCode();
            break;
        }

        // the top of our list
        m = probey - 6;
        if (m < 0) m = 0;
        else if (m + 13 >= NUMGAMEFUNCTIONS) m = NUMGAMEFUNCTIONS-13;

        if (probey == gamefunc_Show_Console) currentlist = 0;
        else if (KB_KeyPressed(sc_LeftArrow) || KB_KeyPressed(sc_kpad_4) ||
                 KB_KeyPressed(sc_RightArrow) || KB_KeyPressed(sc_kpad_6) ||
                 KB_KeyPressed(sc_Tab))
        {
            currentlist ^= 1;
            KB_ClearKeyDown(sc_LeftArrow);
            KB_ClearKeyDown(sc_RightArrow);
            KB_ClearKeyDown(sc_kpad_4);
            KB_ClearKeyDown(sc_kpad_6);
            KB_ClearKeyDown(sc_Tab);
            sound(KICK_HIT);
        }
        else if (KB_KeyPressed(sc_Delete))
        {
            ud.config.KeyboardKeys[probey][currentlist] = 0;
            CONTROL_MapKey(probey, ud.config.KeyboardKeys[probey][0], ud.config.KeyboardKeys[probey][1]);
            sound(KICK_HIT);
            KB_ClearKeyDown(sc_Delete);
        }

        for (l=0; l < min(13,NUMGAMEFUNCTIONS); l++)
        {
            p = CONFIG_FunctionNumToName(m+l);
            if (!p) continue;

            strcpy(tempbuf, p);
            for (i=0;tempbuf[i];i++) if (tempbuf[i]=='_') tempbuf[i] = ' ';
            minitextshade(70,34+l*8,tempbuf,(m+l == probey)?0:16,1,10+16);

            //strcpy(tempbuf, KB_ScanCodeToString(ud.config.KeyboardKeys[m+l][0]));
            strcpy(tempbuf, (char *)getkeyname(ud.config.KeyboardKeys[m+l][0]));
            if (!tempbuf[0]) strcpy(tempbuf, "  -");
            minitextshade(70+100,34+l*8,tempbuf,
                          (m+l == probey && !currentlist?0:16),2,10+16);

            //strcpy(tempbuf, KB_ScanCodeToString(ud.config.KeyboardKeys[m+l][1]));
            strcpy(tempbuf, (char *)getkeyname(ud.config.KeyboardKeys[m+l][1]));
            if (!tempbuf[0]) strcpy(tempbuf, "  -");
            minitextshade(70+120+34,34+l*8,tempbuf,
                          (m+l == probey && currentlist?0:16),2,10+16);
        }

        gametextpal(160,140,  "RESET KEYS TO DEFAULTS",MENUHIGHLIGHT(NUMGAMEFUNCTIONS),10);
        gametextpal(160,140+7,"SET CLASSIC KEY LAYOUT",MENUHIGHLIGHT(NUMGAMEFUNCTIONS+1),10);
        gametext(160,144+9+3,"UP/DOWN = SELECT ACTION",0,2+8+16);
        gametext(160,144+9+9+3,"LEFT/RIGHT = SELECT LIST",0,2+8+16);
        gametext(160,144+9+9+9+3,"ENTER = MODIFY   DELETE = CLEAR",0,2+8+16);

        break;

    case 210:
        {
            int32 sc;
            rotatesprite(320<<15,19<<16,65536L,0,MENUBAR,16,0,10,0,0,xdim-1,ydim-1);
            menutext(320>>1,24,0,0,"KEYBOARD SETUP");

            gametext(320>>1,90,"PRESS THE KEY TO ASSIGN AS",0,2+8+16);
            sprintf(tempbuf,"%s FOR \"%s\"", whichkey?"SECONDARY":"PRIMARY", CONFIG_FunctionNumToName(function));
            gametext(320>>1,90+9,tempbuf,0,2+8+16);
            gametext(320>>1,90+9+9+9,"PRESS \"ESCAPE\" TO CANCEL",0,2+8+16);

            sc = KB_GetLastScanCode();
            if (sc != sc_None || RMB)
            {
                if (sc == sc_Escape || RMB)
                {
                    sound(EXITMENUSOUND);
                }
                else
                {
                    sound(PISTOL_BODYHIT);

                    ud.config.KeyboardKeys[function][whichkey] = KB_GetLastScanCode();
                    if (function == gamefunc_Show_Console)
                        OSD_CaptureKey(KB_GetLastScanCode());
                    else
                        CONTROL_MapKey(function, ud.config.KeyboardKeys[function][0], ud.config.KeyboardKeys[function][1]);
                }

                cmenu(204);

                currentlist = whichkey;
                probey = function;

                KB_ClearKeyDown(sc);
            }

            break;
        }
    case 205:
        rotatesprite(320<<15,19<<16,65536L,0,MENUBAR,16,0,10,0,0,xdim-1,ydim-1);
        menutext(320>>1,24,0,0,"MOUSE SETUP");

        c = 60-4;

        onbar = (probey == (MAXMOUSEBUTTONS-2)*2+2);
        if (probey < (MAXMOUSEBUTTONS-2)*2+2)
            x = probesm(73,40,8,(MAXMOUSEBUTTONS-2)*2+2+2+2);
        else
            x = probesm(40,128-((MAXMOUSEBUTTONS-2)*2+2)*9,9,(MAXMOUSEBUTTONS-2)*2+2+2+2);

        if (x==-1)
        {
            cmenu(202);
            probey = 5;
            break;
        }
        else if (x == (MAXMOUSEBUTTONS-2)*2+2)
        {
            // sensitivity
        }
        else if (x == (MAXMOUSEBUTTONS-2)*2+2+1)
        {
            // mouse aiming toggle
            if (!ud.mouseaiming) myaimmode = 1-myaimmode;
        }
        else if (x == (MAXMOUSEBUTTONS-2)*2+2+2)
        {
            // invert mouse aim
            ud.mouseflip = 1-ud.mouseflip;
        }
        else if (x == (MAXMOUSEBUTTONS-2)*2+2+2+1)
        {
            //advanced
            cmenu(212);
            break;
        }
        else if (x >= 0)
        {
            //set an option
            cmenu(211);
            function = 0;
            whichkey = x;
            if (x < (MAXMOUSEBUTTONS-2)*2)
                probey = ud.config.MouseFunctions[x>>1][x&1];
            else
                probey = ud.config.MouseFunctions[x-4][0];
            if (probey < 0) probey = NUMGAMEFUNCTIONS-1;
            break;
        }

        for (l=0; l < (MAXMOUSEBUTTONS-2)*2+2; l++)
        {
            tempbuf[0] = 0;
            if (l < (MAXMOUSEBUTTONS-2)*2)
            {
                if (l&1)
                {
                    Bstrcpy(tempbuf, "Double ");
                    m = ud.config.MouseFunctions[l>>1][1];
                }
                else
                    m = ud.config.MouseFunctions[l>>1][0];
                Bstrcat(tempbuf, mousebuttonnames[l>>1]);
            }
            else
            {
                Bstrcpy(tempbuf, mousebuttonnames[l-(MAXMOUSEBUTTONS-2)]);
                m = ud.config.MouseFunctions[l-(MAXMOUSEBUTTONS-2)][0];
            }

            minitextshade(c+20,36+l*8,tempbuf,(l==probey)?0:16,1,10+16);

            if (m == -1)
                minitextshade(c+100+20,36+l*8,"  -NONE-",(l==probey)?0:16,2,10+16);
            else
            {
                strcpy(tempbuf, CONFIG_FunctionNumToName(m));
                for (i=0;tempbuf[i];i++) if (tempbuf[i]=='_') tempbuf[i] = ' ';
                minitextshade(c+100+20,36+l*8,tempbuf,(l==probey)?0:16,2,10+16);
            }
        }

        gametextpal(40,122,"SENSITIVITY",MENUHIGHLIGHT((MAXMOUSEBUTTONS-2)*2+2),10);
        gametextpal(40,122+9,"MOUSE AIMING TOGGLE",!ud.mouseaiming?MENUHIGHLIGHT((MAXMOUSEBUTTONS-2)*2+2+1):DISABLEDMENUSHADE,10);
        gametextpal(40,122+9+9,"INVERT MOUSE AIM",MENUHIGHLIGHT((MAXMOUSEBUTTONS-2)*2+2+2),10);
        gametextpal(40,122+9+9+9,"ADVANCED",MENUHIGHLIGHT((MAXMOUSEBUTTONS-2)*2+2+2+1),10);

        {
            short sense;
            sense = CONTROL_GetMouseSensitivity()-1;
            barsm(248,128,&sense,2,x==(MAXMOUSEBUTTONS-2)*2+2,MENUHIGHLIGHT((MAXMOUSEBUTTONS-2)*2+2),PHX(-7));
            CONTROL_SetMouseSensitivity(sense+1);
        }

        if (!ud.mouseaiming) modval(0,1,(int *)&myaimmode,1,probey == (MAXMOUSEBUTTONS-2)*2+2+1);
        else if (probey == (MAXMOUSEBUTTONS-2)*2+2+1)
        {
            gametext(160,144+9+9,"SET MOUSE AIM TYPE TO TOGGLE ON/OFF",0,2+8+16);
            gametext(160,144+9+9+9,"IN THE PLAYER SETUP MENU TO ENABLE",0,2+8+16);
        }

        modval(0,1,(int *)&ud.mouseflip,1,probey == (MAXMOUSEBUTTONS-2)*2+2+2);

        gametextpal(240,122+9, myaimmode && !ud.mouseaiming ? "On" : "Off", !ud.mouseaiming?MENUHIGHLIGHT((MAXMOUSEBUTTONS-2)*2+2+1):DISABLEDMENUSHADE, 0);
        gametextpal(240,122+9+9, !ud.mouseflip ? "On" : "Off", MENUHIGHLIGHT((MAXMOUSEBUTTONS-2)*2+2+2), 0);

        if (probey < (MAXMOUSEBUTTONS-2)*2+2)
        {
            gametext(160,164,"UP/DOWN = SELECT BUTTON",0,2+8+16);
            gametext(160,164+9,"ENTER = MODIFY",0,2+8+16);
        }
        break;

    case 211:
        rotatesprite(320<<15,19<<16,65536L,0,MENUBAR,16,0,10,0,0,xdim-1,ydim-1);
        if (function == 0) menutext(320>>1,24,0,0,"MOUSE SETUP");
        else if (function == 1) menutext(320>>1,24,0,0,"ADVANCED MOUSE");
        else if (function == 2) menutext(320>>1,24,0,0,"JOYSTICK BUTTONS");
        else if (function == 3) menutext(320>>1,24,0,0,"JOYSTICK AXES");

        x = probe(0,0,0,NUMGAMEFUNCTIONS);

        if (x==-1)
        {
            if (function == 0)
            {
                // mouse button
                cmenu(205);
                probey = whichkey;
            }
            else if (function == 1)
            {
                // mouse digital axis
                cmenu(212);
                probey = 3+(whichkey^2);
            }
            else if (function == 2)
            {
                // joystick button/hat
                cmenu(207);
                probey = whichkey;
            }
            else if (function == 3)
            {
                // joystick digital axis
                cmenu((whichkey>>2)+208);
                probey = 1+((whichkey>>1)&1)*4+(whichkey&1);
            }
            break;
        }
        else if (x >= 0)
        {
            if (x == NUMGAMEFUNCTIONS-1) x = -1;

            if (function == 0)
            {
                if (whichkey < (MAXMOUSEBUTTONS-2)*2)
                {
                    ud.config.MouseFunctions[whichkey>>1][whichkey&1] = x;
                    CONTROL_MapButton(x, whichkey>>1, whichkey&1, controldevice_mouse);
                }
                else
                {
                    ud.config.MouseFunctions[whichkey-(MAXMOUSEBUTTONS-2)][0] = x;
                    CONTROL_MapButton(x, whichkey-(MAXMOUSEBUTTONS-2), 0, controldevice_mouse);
                }
                cmenu(205);
                probey = whichkey;
            }
            else if (function == 1)
            {
                ud.config.MouseDigitalFunctions[whichkey>>1][whichkey&1] = x;
                CONTROL_MapDigitalAxis(whichkey>>1, x, whichkey&1, controldevice_mouse);
                cmenu(212);
                probey = 3+(whichkey^2);
            }
            else if (function == 2)
            {
                if (whichkey < 2*joynumbuttons)
                {
                    ud.config.JoystickFunctions[whichkey>>1][whichkey&1] = x;
                    CONTROL_MapButton(x, whichkey>>1, whichkey&1, controldevice_joystick);
                }
                else
                {
                    ud.config.JoystickFunctions[joynumbuttons + (whichkey-2*joynumbuttons)][0] = x;
                    CONTROL_MapButton(x, joynumbuttons + (whichkey-2*joynumbuttons), 0, controldevice_joystick);
                }
                cmenu(207);
                probey = whichkey;
            }
            else if (function == 3)
            {
                ud.config.JoystickDigitalFunctions[whichkey>>1][whichkey&1] = x;
                CONTROL_MapDigitalAxis(whichkey>>1, x, whichkey&1, controldevice_joystick);
                cmenu((whichkey>>2)+208);
                probey = 1+((whichkey>>1)&1)*4+(whichkey&1);
            }
            break;
        }

        gametext(320>>1,31,"SELECT A FUNCTION TO ASSIGN",0,2+8+16);

        if (function == 0)
        {
            if (whichkey < (MAXMOUSEBUTTONS-2)*2)
                sprintf(tempbuf,"TO %s%s", (whichkey&1)?"DOUBLE-CLICKED ":"", mousebuttonnames[whichkey>>1]);
            else
                Bstrcpy(tempbuf, mousebuttonnames[whichkey-(MAXMOUSEBUTTONS-2)]);
        }
        else if (function == 1)
        {
            Bstrcpy(tempbuf,"TO DIGITAL ");
            switch (whichkey)
            {
            case 0:
                Bstrcat(tempbuf, "LEFT");
                break;
            case 1:
                Bstrcat(tempbuf, "RIGHT");
                break;
            case 2:
                Bstrcat(tempbuf, "UP");
                break;
            case 3:
                Bstrcat(tempbuf, "DOWN");
                break;
            }
        }
        else if (function == 2)
        {
            static char *directions[] =
                { "UP", "RIGHT", "DOWN", "LEFT"
                };
            if (whichkey < 2*joynumbuttons)
                Bsprintf(tempbuf,"TO %s%s", (whichkey&1)?"DOUBLE-CLICKED ":"", getjoyname(1,whichkey>>1));
            else
                Bsprintf(tempbuf,"TO HAT %s", directions[whichkey-2*joynumbuttons]);
        }
        else if (function == 3)
        {
            Bsprintf(tempbuf,"TO DIGITAL %s %s",getjoyname(0,whichkey>>1),(whichkey&1)?"POSITIVE":"NEGATIVE");
        }

        gametext(320>>1,31+9,tempbuf,0,2+8+16);

        if (KB_KeyPressed(sc_End))
        {
            KB_ClearKeyDown(sc_End);
            probey = NUMGAMEFUNCTIONS-1;
            sound(KICK_HIT);
        }
        else if (KB_KeyPressed(sc_Home))
        {
            KB_ClearKeyDown(sc_Home);
            probey = 0;
            sound(KICK_HIT);
        }

        m = probey - 6;
        if (m < 0) m = 0;
        else if (m + 13 >= NUMGAMEFUNCTIONS) m = NUMGAMEFUNCTIONS-13;

        for (l=0; l < min(13,NUMGAMEFUNCTIONS); l++)
        {
            if (l+m == NUMGAMEFUNCTIONS-1)
                strcpy(tempbuf, "  -NONE-");
            else
                strcpy(tempbuf, CONFIG_FunctionNumToName(m+l));

            for (i=0;tempbuf[i];i++) if (tempbuf[i]=='_') tempbuf[i] = ' ';
            minitext(100,51+l*8,tempbuf,(m+l == probey)?0:16,10+16);
        }

        gametext(320>>1,161,"PRESS \"ESCAPE\" TO CANCEL",0,2+8+16);

        break;

    case 212:
        rotatesprite(320<<15,19<<16,65536L,0,MENUBAR,16,0,10,0,0,xdim-1,ydim-1);
        menutext(320>>1,24,0,0,"ADVANCED MOUSE");

        c = (320>>1)-120;

        onbar = (probey == 0 || probey == 1 || probey == 2);
        if (probey < 3)
            x = probe(c,46,16,7);
        else if (probey < 7)
        {
            m=50;
            x = probesm(c+10,96+16-(10+10+10),10,7);
        }
        else
        {
            x = probe(c,146+16-(16+16+16+16+16+16),16,7);
        }

        switch (x)
        {
        case -1:
            cmenu(205);
            probey = (MAXMOUSEBUTTONS-2)*2+2+2+1;
            break;

        case 0:
            // x-axis scale
        case 1:
            // y-axis scale
        case 2:
            // mouse filter
            break;

        case 3:
            // digital up
        case 4:
            // digital down
        case 5:
            // digital left
        case 6:
            // digital right
            function = 1;
            whichkey = (x-3)^2; // flip the actual axis number
            cmenu(211);
            probey = ud.config.MouseDigitalFunctions[whichkey>>1][whichkey&1];
            if (probey < 0) probey = NUMGAMEFUNCTIONS-1;
            break;
        }

        switch (probey)
        {
        case 3:
        case 4:
        case 5:
        case 6:
            gametext(160,144+9+9,"DIGITAL AXES ARE NOT FOR MOUSE LOOK",0,2+8+16);
            break;
        }

        menutext(c,46,MENUHIGHLIGHT(0),0,"X-AXIS SCALE");
        l = (ud.config.MouseAnalogueScale[0]+262144) >> 13;
        bar(c+160+40,46,(short *)&l,1,x==0,MENUHIGHLIGHT(0),0);
        l = (l<<13)-262144;
        if (l != ud.config.MouseAnalogueScale[0])
        {
            CONTROL_SetAnalogAxisScale(0, l, controldevice_mouse);
            ud.config.MouseAnalogueScale[0] = l;
        }
        Bsprintf(tempbuf,"%s%.2f",l>=0?" ":"",(float)l/65536.0);
        gametext(c+160-16,46-8,tempbuf,MENUHIGHLIGHT(0),2+8+16);

        menutext(c,46+16,MENUHIGHLIGHT(1),0,"Y-AXIS SCALE");
        l = (ud.config.MouseAnalogueScale[1]+262144) >> 13;
        bar(c+160+40,46+16,(short *)&l,1,x==1,MENUHIGHLIGHT(1),0);
        l = (l<<13)-262144;
        if (l != ud.config.MouseAnalogueScale[1])
        {
            CONTROL_SetAnalogAxisScale(1, l, controldevice_mouse);
            ud.config.MouseAnalogueScale[1] = l;
        }
        Bsprintf(tempbuf,"%s%.2f",l>=0?" ":"",(float)l/65536.0);
        gametext(c+160-16,46+16-8,tempbuf,MENUHIGHLIGHT(1),2+8+16);

        menutext(c,46+16+16,MENUHIGHLIGHT(2),0,"DEAD ZONE");
        l = ud.config.MouseFilter>>1;
        bar(c+160+40,46+16+16,(short *)&l,2,x==2,MENUHIGHLIGHT(2),0);
        ud.config.MouseFilter = l<<1;
        menutext(c,46+16+16+16+8,/*(MENUHIGHLIGHT(3))+(MENUHIGHLIGHT(4))+(MENUHIGHLIGHT(5))+(MENUHIGHLIGHT(6))-24*/0,0,"DIGITAL AXES ACTIONS");

        if (ud.config.MouseFilter == 0)
            Bsprintf(tempbuf," OFF");
        else if (ud.config.MouseFilter < 48)
            Bsprintf(tempbuf," LOW");
        else if (ud.config.MouseFilter < 96)
            Bsprintf(tempbuf," MED");
        else if (ud.config.MouseFilter < 128)
            Bsprintf(tempbuf,"HIGH");

        gametext(c+160-16,46+16+16-8,tempbuf,MENUHIGHLIGHT(2),2+8+16);


        gametextpal(c+10,90+16,"UP:",MENUHIGHLIGHT(3),10);
        if (ud.config.MouseDigitalFunctions[1][0] < 0)
            strcpy(tempbuf, "  -NONE-");
        else
            strcpy(tempbuf, CONFIG_FunctionNumToName(ud.config.MouseDigitalFunctions[1][0]));

        for (i=0;tempbuf[i];i++) if (tempbuf[i]=='_') tempbuf[i] = ' ';
        minitextshade(c+10+60,91+16,tempbuf,MENUHIGHLIGHT(3),0,10+16);

        gametextpal(c+10,90+16+10,"DOWN:",MENUHIGHLIGHT(4),10);
        if (ud.config.MouseDigitalFunctions[1][1] < 0)
            strcpy(tempbuf, "  -NONE-");
        else
            strcpy(tempbuf, CONFIG_FunctionNumToName(ud.config.MouseDigitalFunctions[1][1]));

        for (i=0;tempbuf[i];i++) if (tempbuf[i]=='_') tempbuf[i] = ' ';
        minitextshade(c+10+60,91+16+10,tempbuf,MENUHIGHLIGHT(4),0,10+16);

        gametextpal(c+10,90+16+10+10,"LEFT:",MENUHIGHLIGHT(5),10);
        if (ud.config.MouseDigitalFunctions[0][0] < 0)
            strcpy(tempbuf, "  -NONE-");
        else
            strcpy(tempbuf, CONFIG_FunctionNumToName(ud.config.MouseDigitalFunctions[0][0]));

        for (i=0;tempbuf[i];i++) if (tempbuf[i]=='_') tempbuf[i] = ' ';
        minitextshade(c+10+60,91+16+10+10,tempbuf,MENUHIGHLIGHT(5),0,10+16);

        gametextpal(c+10,90+16+10+10+10,"RIGHT:",MENUHIGHLIGHT(6),10);
        if (ud.config.MouseDigitalFunctions[0][1] < 0)
            strcpy(tempbuf, "  -NONE-");
        else
            strcpy(tempbuf, CONFIG_FunctionNumToName(ud.config.MouseDigitalFunctions[0][1]));

        for (i=0;tempbuf[i];i++) if (tempbuf[i]=='_') tempbuf[i] = ' ';
        minitextshade(c+10+60,91+16+10+10+10,tempbuf,MENUHIGHLIGHT(6),0,10+16);

        break;

    case 206:
        rotatesprite(320<<15,19<<16,65536L,0,MENUBAR,16,0,10,0,0,xdim-1,ydim-1);
        menutext(320>>1,24,0,0,"JOYSTICK SETUP");

        x = probe(160,100-18,18,3);

        switch (x)
        {
        case -1:
            cmenu(202);
            probey = 6;
            break;
        case 0:
        case 1:
            cmenu(207+x);
            break;
        case 2:
            cmenu(213);
            break;
        }

        menutext(160,100-18,0,0,"EDIT BUTTONS");
        menutext(160,100,0,0,"EDIT AXES");
        menutext(160,100+18,0,0,"DEAD ZONES");

        break;

    case 207:
        rotatesprite(320<<15,10<<16,65536L,0,MENUBAR,16,0,10,0,0,xdim-1,ydim-1);
        menutext(320>>1,15,0,0,"JOYSTICK BUTTONS");

        c = 2*joynumbuttons + 4*(joynumhats>0);

        x = probe(0,0,0,c);

        if (x == -1)
        {
            cmenu(206);
            probey = 0;
            break;
        }
        else if (x >= 0)
        {
            function = 2;
            whichkey = x;
            cmenu(211);
            if (x < 2*joynumbuttons)
            {
                probey = ud.config.JoystickFunctions[x>>1][x&1];
            }
            else
            {
                probey = ud.config.JoystickFunctions[joynumbuttons + (x-2*joynumbuttons)][0];
            }
            if (probey < 0) probey = NUMGAMEFUNCTIONS-1;
            break;
        }

        // the top of our list
        if (c < 13) m = 0;
        else
        {
            m = probey - 6;
            if (m < 0) m = 0;
            else if (m + 13 >= c) m = c-13;
        }

        for (l=0; l<min(13,c); l++)
        {
            if (m+l < 2*joynumbuttons)
            {
                sprintf(tempbuf, "%s%s", ((l+m)&1)?"Double ":"", getjoyname(1,(l+m)>>1));
                x = ud.config.JoystickFunctions[(l+m)>>1][(l+m)&1];
            }
            else
            {
                static char *directions[] =
                    { "Up", "Right", "Down", "Left"
                    };
                sprintf(tempbuf, "Hat %s", directions[(l+m)-2*joynumbuttons]);
                x = ud.config.JoystickFunctions[joynumbuttons + ((l+m)-2*joynumbuttons)][0];
            }
            minitextshade(80-4,33+l*8,tempbuf,(m+l == probey)?0:16,1,10+16);

            if (x == -1)
                minitextshade(176,33+l*8,"  -NONE-",(m+l==probey)?0:16,2,10+16);
            else
            {
                strcpy(tempbuf, CONFIG_FunctionNumToName(x));
                for (i=0;tempbuf[i];i++) if (tempbuf[i]=='_') tempbuf[i] = ' ';
                minitextshade(176,33+l*8,tempbuf,(m+l==probey)?0:16,2,10+16);
            }
        }

        gametext(160,149,"UP/DOWN = SELECT BUTTON",0,2+8+16);
        gametext(160,149+9,"ENTER = MODIFY",0,2+8+16);
        break;

    case 208:
    case 209:
    case 217:
    case 218:
    case 219:
    case 220:
    case 221:
    case 222:
        {
            int thispage, twothispage;

            rotatesprite(320<<15,10<<16,65536L,0,MENUBAR,16,0,10,0,0,xdim-1,ydim-1);
            menutext(320>>1,15,0,0,"JOYSTICK AXES");

            thispage = (current_menu < 217) ? (current_menu-208) : (current_menu-217)+2;
            twothispage = (thispage*2+1 < joynumaxes);

            onbar = 0;
            switch (probey)
            {
            case 0:
            case 4:
                onbar = 1;
                x = probe(88,45+(probey==4)*64,0,1+(4<<twothispage));
                break;
            case 1:
            case 2:
            case 5:
            case 6:
                x = probe(172+(probey==2||probey==6)*72,45+15+(probey==5||probey==6)*64,0,1+(4<<twothispage));
                break;
            case 3:
            case 7:
                x = probe(88,45+15+15+(probey==7)*64,0,1+(4<<twothispage));
                break;
            default:
                x = probe(60,79+79*twothispage,0,1+(4<<twothispage));
                break;
            }

            switch (x)
            {
            case -1:
                cmenu(206);
                probey = 1;
                break;
            case 8:
                if (joynumaxes > 2)
                {
                    if (thispage == ((joynumaxes+1)/2)-1) cmenu(208);
                    else
                    {
                        if (current_menu == 209) cmenu(217);
                        else cmenu(current_menu+1);
                    }
                }
                break;

            case 4: // bar
                if (!twothispage && joynumaxes > 2)
                    cmenu(208);
            case 0:
                break;

            case 1: // digitals
            case 2:
            case 5:
            case 6:
                function = 3;
                whichkey = ((thispage*2+(x==5||x==6)) << 1) + (x==2||x==6);
                cmenu(211);
                probey = ud.config.JoystickDigitalFunctions[whichkey>>1][whichkey&1];
                if (probey < 0) probey = NUMGAMEFUNCTIONS-1;
                break;

            case 3: // analogues
            case 7:
                l = ud.config.JoystickAnalogueAxes[thispage*2+(x==7)];
                if (l == analog_turning) l = analog_strafing;
                else if (l == analog_strafing) l = analog_lookingupanddown;
                else if (l == analog_lookingupanddown) l = analog_moving;
                else if (l == analog_moving) l = -1;
                else l = analog_turning;
                ud.config.JoystickAnalogueAxes[thispage*2+(x==7)] = l;
                CONTROL_MapAnalogAxis(thispage*2+(x==7),l,controldevice_joystick);
                {
                    extern int32 mouseyaxismode;	// player.c
                    mouseyaxismode = -1;
                }
                break;
            default:
                break;
            }

            Bsprintf(tempbuf,(char *)getjoyname(0,thispage*2));
            menutext(42,32,0,0,tempbuf);
            if (twothispage)
            {
                Bsprintf(tempbuf,(char *)getjoyname(0,thispage*2+1));
                menutext(42,32+64,0,0,tempbuf);
            }
            gametext(76,38,"SCALE",0,2+8+16);
            l = (ud.config.JoystickAnalogueScale[thispage*2]+262144) >> 13;
            bar(140+56,38+8,(short *)&l,1,x==0,0,0);
            l = (l<<13)-262144;
            if (l != ud.config.JoystickAnalogueScale[thispage*2])
            {
                CONTROL_SetAnalogAxisScale(thispage*2, l, controldevice_joystick);
                ud.config.JoystickAnalogueScale[thispage*2] = l;
            }
            Bsprintf(tempbuf,"%s%.2f",l>=0?" ":"",(float)l/65536.0);
            gametext(140,38,tempbuf,0,2+8+16);

            gametext(76,38+15,"DIGITAL",0,2+8+16);
            if (ud.config.JoystickDigitalFunctions[thispage*2][0] < 0)
                strcpy(tempbuf, "  -NONE-");
            else
                strcpy(tempbuf, CONFIG_FunctionNumToName(ud.config.JoystickDigitalFunctions[thispage*2][0]));

            for (i=0;tempbuf[i];i++) if (tempbuf[i]=='_') tempbuf[i] = ' ';
            minitext(140+12,38+15,tempbuf,0,10+16);

            if (ud.config.JoystickDigitalFunctions[thispage*2][1] < 0)
                strcpy(tempbuf, "  -NONE-");
            else
                strcpy(tempbuf, CONFIG_FunctionNumToName(ud.config.JoystickDigitalFunctions[thispage*2][1]));

            for (i=0;tempbuf[i];i++) if (tempbuf[i]=='_') tempbuf[i] = ' ';
            minitext(140+12+72,38+15,tempbuf,0,10+16);

            gametext(76,38+15+15,"ANALOG",0,2+8+16);
            if (CONFIG_AnalogNumToName(ud.config.JoystickAnalogueAxes[thispage*2]))
            {
                p = CONFIG_AnalogNumToName(ud.config.JoystickAnalogueAxes[thispage*2]);
                if (p)
                {
                    gametext(140+12,38+15+15, strchr(p,'_')+1, 0, 2+8+16);
                }
            }

            if (twothispage)
            {
                gametext(76,38+64,"SCALE",0,2+8+16);
                l = (ud.config.JoystickAnalogueScale[thispage*2+1]+262144) >> 13;
                bar(140+56,38+8+64,(short *)&l,1,x==4,0,0);
                l = (l<<13)-262144;
                if (l != ud.config.JoystickAnalogueScale[thispage*2+1])
                {
                    CONTROL_SetAnalogAxisScale(thispage*2+1, l, controldevice_joystick);
                    ud.config.JoystickAnalogueScale[thispage*2+1] = l;
                }
                Bsprintf(tempbuf,"%s%.2f",l>=0?" ":"",(float)l/65536.0);
                gametext(140,38+64,tempbuf,0,2+8+16);

                gametext(76,38+64+15,"DIGITAL",0,2+8+16);
                if (ud.config.JoystickDigitalFunctions[thispage*2+1][0] < 0)
                    strcpy(tempbuf, "  -NONE-");
                else
                    strcpy(tempbuf, CONFIG_FunctionNumToName(ud.config.JoystickDigitalFunctions[thispage*2+1][0]));

                for (i=0;tempbuf[i];i++) if (tempbuf[i]=='_') tempbuf[i] = ' ';
                minitext(140+12,38+15+64,tempbuf,0,10+16);

                if (ud.config.JoystickDigitalFunctions[thispage*2+1][1] < 0)
                    strcpy(tempbuf, "  -NONE-");
                else
                    strcpy(tempbuf, CONFIG_FunctionNumToName(ud.config.JoystickDigitalFunctions[thispage*2+1][1]));

                for (i=0;tempbuf[i];i++) if (tempbuf[i]=='_') tempbuf[i] = ' ';
                minitext(140+12+72,38+15+64,tempbuf,0,10+16);

                gametext(76,38+64+15+15,"ANALOG",0,2+8+16);
                if (CONFIG_AnalogNumToName(ud.config.JoystickAnalogueAxes[thispage*2+1]))
                {
                    p = CONFIG_AnalogNumToName(ud.config.JoystickAnalogueAxes[thispage*2+1]);
                    if (p)
                    {
                        gametext(140+12,38+64+15+15, strchr(p,'_')+1, 0, 2+8+16);
                    }
                }
            }

            if (joynumaxes > 2)
            {
                menutext(320>>1,twothispage?158:108,SHX(-10),(joynumaxes<=2),"NEXT...");
                sprintf(tempbuf,"Page %d of %d",thispage+1,(joynumaxes+1)/2);
                gametext(320-100,158,tempbuf,0,2+8+16);
            }
            break;
        }

    case 213:
    case 214:
    case 215:
    case 216:
        {
            // Pray this is enough pages for now :-|
            int first,last;
            rotatesprite(320<<15,19<<16,65536L,0,MENUBAR,16,0,10,0,0,xdim-1,ydim-1);
            menutext(320>>1,24,0,0,"JOY DEAD ZONES");

            first = 4*(current_menu-213);
            last  = min(4*(current_menu-213)+4,joynumaxes);

            onbar = 1;
            x = probe(320,48,15,2*(last-first)+(joynumaxes>4));

            if (x==-1)
            {
                cmenu(206);
                probey = 2;
                break;
            }
            else if (x==2*(last-first) && joynumaxes>4)
            {
                cmenu((current_menu-213) == (joynumaxes/4) ? 213 : (current_menu+1));
                probey = 0;
                break;
            }

            for (m=first;m<last;m++)
            {
                unsigned short odx,dx,ody,dy;
                Bsprintf(tempbuf,(char *)getjoyname(0,m));
                menutext(32,48+30*(m-first),0,0,tempbuf);

                gametext(128,48+30*(m-first)-8,"DEAD",0,2+8+16);
                gametext(128,48+30*(m-first)-8+15,"SATU",0,2+8+16);

                dx = odx = min(64,64l*ud.config.JoystickAnalogueDead[m]/10000l);
                dy = ody = min(64,64l*ud.config.JoystickAnalogueSaturate[m]/10000l);

                bar(217,48+30*(m-first),(short *)&dx,4,x==((m-first)*2),0,0);
                bar(217,48+30*(m-first)+15,(short *)&dy,4,x==((m-first)*2+1),0,0);

                Bsprintf(tempbuf,"%3d%%",100*dx/64);
                gametext(217-49,48+30*(m-first)-8,tempbuf,0,2+8+16);
                Bsprintf(tempbuf,"%3d%%",100*dy/64);
                gametext(217-49,48+30*(m-first)-8+15,tempbuf,0,2+8+16);

                if (dx != odx) ud.config.JoystickAnalogueDead[m]     = 10000l*dx/64l;
                if (dy != ody) ud.config.JoystickAnalogueSaturate[m] = 10000l*dy/64l;
                if (dx != odx || dy != ody)
                    setjoydeadzone(m,ud.config.JoystickAnalogueDead[m],ud.config.JoystickAnalogueSaturate[m]);
            }
            //gametext(160,158,"DEAD = DEAD ZONE, SAT. = SATURATION",0,2+8+16);
            if (joynumaxes>4)
            {
                menutext(32,48+30*(last-first),0,0,"NEXT...");
                sprintf(tempbuf,"Page %d of %d", 1+(current_menu-213), (joynumaxes+3)/4);
                gametext(320-100,158,tempbuf,0,2+8+16);
            }
            break;
        }

    case 700:
    case 701:
        c = (320>>1)-120;
        rotatesprite(320<<15,19<<16,65536L,0,MENUBAR,16,0,10,0,0,xdim-1,ydim-1);
        menutext(320>>1,24,0,0,"SOUND SETUP");
        onbar = (probey == 2 || probey == 3);

        x = probe(c,50,16,7);
        switch (x)
        {
        case -1:
            if (g_player[myconnectindex].ps.gm&MODE_GAME && current_menu == 701)
            {
                g_player[myconnectindex].ps.gm &= ~MODE_MENU;
                if (ud.multimode < 2  && ud.recstat != 2)
                {
                    ready2send = 1;
                    totalclock = ototalclock;
                }
            }

            else cmenu(202);
            probey = 1;
            break;
        case 0:
            if (ud.config.FXDevice >= 0)
            {
                ud.config.SoundToggle = 1-ud.config.SoundToggle;
                if (ud.config.SoundToggle == 0)
                {
                    FX_StopAllSounds();
                    clearsoundlocks();
                }
                onbar = 0;
            }
            break;
        case 1:
            if (ud.config.MusicDevice >= 0 && (numplayers < 2 || ud.config.MusicToggle))
            {
                ud.config.MusicToggle = 1-ud.config.MusicToggle;
                if (ud.config.MusicToggle == 0) MUSIC_Pause();
                else
                {
                    if (ud.recstat != 2 && g_player[myconnectindex].ps.gm&MODE_GAME)
                    {
                        if (map[(unsigned char)music_select].musicfn != NULL)
                            playmusic(&map[(unsigned char)music_select].musicfn[0]);
                    }
                    else playmusic(&env_music_fn[0][0]);

                    MUSIC_Continue();
                }
            }
            onbar = 0;

            break;
        case 4:
            if (ud.config.SoundToggle && (ud.config.FXDevice >= 0))
            {
                ud.config.VoiceToggle = (ud.config.VoiceToggle == 2) ? 0 : ud.config.VoiceToggle+1;
            }
            onbar = 0;
            break;
        case 5:
            if (ud.config.SoundToggle && (ud.config.FXDevice >= 0)) ud.config.AmbienceToggle = 1-ud.config.AmbienceToggle;
            onbar = 0;
            break;
        case 6:
            if (ud.config.SoundToggle && (ud.config.FXDevice >= 0))
            {
                ud.config.ReverseStereo = 1-ud.config.ReverseStereo;
                FX_SetReverseStereo(ud.config.ReverseStereo);
            }
            onbar = 0;
            break;
        default:
            onbar = 1;
            break;
        }

        menutext(c+151+40,50,MENUHIGHLIGHT(0),(ud.config.FXDevice<0),ud.config.SoundToggle && ud.config.FXDevice >= 0?"ON":"OFF");

        menutext(c+151+40,50+16,MENUHIGHLIGHT(1),(ud.config.MusicDevice<0||(numplayers > 1 && !ud.config.MusicToggle)),(ud.config.MusicToggle && (ud.config.MusicDevice >= 0))?"ON":"OFF");

        menutext(c,50,MENUHIGHLIGHT(0),(ud.config.FXDevice<0),"SOUND");
        menutext(c,50+16+16,MENUHIGHLIGHT(2),(ud.config.FXDevice<0)||ud.config.SoundToggle==0,"SOUND VOLUME");
        {
            l = ud.config.FXVolume;
            ud.config.FXVolume >>= 2;
            bar(c+167+40,50+16+16,(short *)&ud.config.FXVolume,4,(ud.config.FXDevice>=0)&&x==2,MENUHIGHLIGHT(2),ud.config.SoundToggle==0||(ud.config.FXDevice<0));
            if (l != ud.config.FXVolume)
                ud.config.FXVolume <<= 2;
            if (l != ud.config.FXVolume)
                FX_SetVolume((short) ud.config.FXVolume);
        }
        menutext(c,50+16,MENUHIGHLIGHT(1),(ud.config.MusicDevice<0||(numplayers > 1 && !ud.config.MusicToggle)),"MUSIC");
        menutext(c,50+16+16+16,MENUHIGHLIGHT(3),(ud.config.MusicDevice<0)||ud.config.MusicToggle==0,"MUSIC VOLUME");
        {
            l = ud.config.MusicVolume;
            ud.config.MusicVolume >>= 2;
            bar(c+167+40,50+16+16+16,
                (short *)&ud.config.MusicVolume,4,
                (ud.config.MusicDevice>=0) && x==3,MENUHIGHLIGHT(3),
                ud.config.MusicToggle==0||(ud.config.MusicDevice<0));
            ud.config.MusicVolume <<= 2;
            if (l != ud.config.MusicVolume)
                MUSIC_SetVolume((short) ud.config.MusicVolume);

        }
        menutext(c,50+16+16+16+16,MENUHIGHLIGHT(4),(ud.config.FXDevice<0)||ud.config.SoundToggle==0,"DUKE TALK");
        menutext(c,50+16+16+16+16+16,MENUHIGHLIGHT(5),(ud.config.FXDevice<0)||ud.config.SoundToggle==0,"AMBIENCE");

        menutext(c,50+16+16+16+16+16+16,MENUHIGHLIGHT(6),(ud.config.FXDevice<0)||ud.config.SoundToggle==0,"FLIP STEREO");

        {
            char *s[] = { "OFF", "LOCAL", "ALL" };
            menutext(c+151+40,50+16+16+16+16,MENUHIGHLIGHT(4),(ud.config.FXDevice<0)||ud.config.SoundToggle==0,s[ud.config.VoiceToggle]);
        }

        menutext(c+151+40,50+16+16+16+16+16,MENUHIGHLIGHT(5),(ud.config.FXDevice<0)||ud.config.SoundToggle==0,ud.config.AmbienceToggle?"ON":"OFF");

        menutext(c+151+40,50+16+16+16+16+16+16,MENUHIGHLIGHT(6),(ud.config.FXDevice<0)||ud.config.SoundToggle==0,ud.config.ReverseStereo?"ON":"OFF");

        break;

    case 350:
        cmenu(351);
        screencapt = 1;
        displayrooms(myconnectindex,65536);
        //savetemp("duke3d.tmp",waloff[TILE_SAVESHOT],160*100);
        screencapt = 0;
        break;

    case 360:
    case 361:
    case 362:
    case 363:
    case 364:
    case 365:
    case 366:
    case 367:
    case 368:
    case 369:
    case 351:
    case 300:

        c = 320>>1;
        rotatesprite(c<<16,200<<15,65536L,0,MENUSCREEN,16,0,10+64,0,0,xdim-1,ydim-1);
        rotatesprite(c<<16,19<<16,65536L,0,MENUBAR,16,0,10,0,0,xdim-1,ydim-1);

        if (current_menu == 300) menutext(c,24,0,0,"LOAD GAME");
        else menutext(c,24,0,0,"SAVE GAME");

        if (current_menu >= 360 && current_menu <= 369)
        {
            sprintf(tempbuf,"PLAYERS: %-2ld                      ",ud.multimode);
            gametext(160,156,tempbuf,0,2+8+16);
            sprintf(tempbuf,"EPISODE: %-2ld / LEVEL: %-2ld / SKILL: %-2ld",1+ud.volume_number,1+ud.level_number,ud.player_skill);
            gametext(160,168,tempbuf,0,2+8+16);
            if (ud.volume_number == 0 && ud.level_number == 7)
                gametext(160,180,boardfilename,0,2+8+16);

            x = strget((320>>1),184,&ud.savegame[current_menu-360][0],19, 999);

            if (x == -1)
            {
                //        readsavenames();
                g_player[myconnectindex].ps.gm = MODE_GAME;
                if (ud.multimode < 2  && ud.recstat != 2)
                {
                    ready2send = 1;
                    totalclock = ototalclock;
                }
                goto DISPLAYNAMES;
            }

            if (x == 1)
            {
                if (ud.savegame[current_menu-360][0] == 0)
                {
                    KB_FlushKeyboardQueue();
                    cmenu(351);
                }
                else
                {
                    if (ud.multimode > 1)
                        saveplayer(-1-(current_menu-360));
                    else saveplayer(current_menu-360);
                    lastsavedpos = current_menu-360;
                    g_player[myconnectindex].ps.gm = MODE_GAME;

                    if (ud.multimode < 2  && ud.recstat != 2)
                    {
                        ready2send = 1;
                        totalclock = ototalclock;
                    }
                    KB_ClearKeyDown(sc_Escape);
                    sound(EXITMENUSOUND);
                }
            }

            rotatesprite(101<<16,97<<16,65536>>1,512,TILE_SAVESHOT,-32,0,2+4+8+64,0,0,xdim-1,ydim-1);
            dispnames();
            rotatesprite((c+67+strlen(&ud.savegame[current_menu-360][0])*4)<<16,(50+12*probey)<<16,32768L-10240,0,SPINNINGNUKEICON+(((totalclock)>>3)%7),0,0,10,0,0,xdim-1,ydim-1);
            break;
        }

        last_threehundred = probey;

        x = probe(c+68,54,12,10);

        if (current_menu == 300)
        {
            if (ud.savegame[probey][0])
            {
                if (lastsavehead != probey)
                {
                    loadpheader(probey,&savehead);
                    lastsavehead = probey;
                }

                rotatesprite(101<<16,97<<16,65536L>>1,512,TILE_LOADSHOT,-32,0,4+10+64,0,0,xdim-1,ydim-1);
                sprintf(tempbuf,"PLAYERS: %-2ld                      ",savehead.numplr);
                gametext(160,156,tempbuf,0,2+8+16);
                sprintf(tempbuf,"EPISODE: %-2ld / LEVEL: %-2ld / SKILL: %-2ld",1+savehead.volnum,1+savehead.levnum,savehead.plrskl);
                gametext(160,168,tempbuf,0,2+8+16);
                if (savehead.volnum == 0 && savehead.levnum == 7)
                    gametext(160,180,savehead.boardfn,0,2+8+16);
            }
            else menutext(69,70,0,0,"EMPTY");
        }
        else
        {
            if (ud.savegame[probey][0])
            {
                if (lastsavehead != probey)
                    loadpheader(probey,&savehead);
                lastsavehead = probey;
                rotatesprite(101<<16,97<<16,65536L>>1,512,TILE_LOADSHOT,-32,0,4+10+64,0,0,xdim-1,ydim-1);
            }
            else menutext(69,70,0,0,"EMPTY");
            sprintf(tempbuf,"PLAYERS: %-2ld                      ",ud.multimode);
            gametext(160,156,tempbuf,0,2+8+16);
            sprintf(tempbuf,"EPISODE: %-2ld / LEVEL: %-2ld / SKILL: %-2ld",1+ud.volume_number,1+ud.level_number,ud.player_skill);
            gametext(160,168,tempbuf,0,2+8+16);
            if (ud.volume_number == 0 && ud.level_number == 7)
                gametext(160,180,boardfilename,0,2+8+16);
        }

        switch (x)
        {
        case -1:
            if (current_menu == 300)
            {
                if ((g_player[myconnectindex].ps.gm&MODE_GAME) != MODE_GAME)
                {
                    cmenu(0);
                    break;
                }
                else
                    g_player[myconnectindex].ps.gm &= ~MODE_MENU;
            }
            else
                g_player[myconnectindex].ps.gm = MODE_GAME;

            if (ud.multimode < 2 && ud.recstat != 2)
            {
                ready2send = 1;
                totalclock = ototalclock;
            }

            break;
        case 0:
        case 1:
        case 2:
        case 3:
        case 4:
        case 5:
        case 6:
        case 7:
        case 8:
        case 9:
            if (current_menu == 300)
            {
                if (ud.savegame[x][0])
                    current_menu = (1000+x);
            }
            else
            {
                if (ud.savegame[x][0] != 0)
                    current_menu = 2000+x;
                else
                {
                    KB_FlushKeyboardQueue();
                    current_menu = (360+x);
                    ud.savegame[x][0] = 0;
                    inputloc = 0;
                }
            }
            break;
        }

DISPLAYNAMES:
        dispnames();
        break;

    case 400:
    case 401:
        if (VOLUMEALL) goto VOLUME_ALL_40x;
    case 402:
    case 403:

        c = 320>>1;

        if (KB_KeyPressed(sc_LeftArrow) ||
                KB_KeyPressed(sc_kpad_4) ||
                KB_KeyPressed(sc_UpArrow) ||
                KB_KeyPressed(sc_PgUp) ||
                KB_KeyPressed(sc_kpad_8))
        {
            KB_ClearKeyDown(sc_LeftArrow);
            KB_ClearKeyDown(sc_kpad_4);
            KB_ClearKeyDown(sc_UpArrow);
            KB_ClearKeyDown(sc_PgUp);
            KB_ClearKeyDown(sc_kpad_8);

            sound(KICK_HIT);
            current_menu--;
            if (current_menu < 400) current_menu = 403;
        }
        else if (
            KB_KeyPressed(sc_PgDn) ||
            KB_KeyPressed(sc_Enter) ||
            KB_KeyPressed(sc_kpad_Enter) ||
            KB_KeyPressed(sc_RightArrow) ||
            KB_KeyPressed(sc_DownArrow) ||
            KB_KeyPressed(sc_kpad_2) ||
            KB_KeyPressed(sc_kpad_9) ||
            KB_KeyPressed(sc_Space) ||
            KB_KeyPressed(sc_kpad_6))
        {
            KB_ClearKeyDown(sc_PgDn);
            KB_ClearKeyDown(sc_Enter);
            KB_ClearKeyDown(sc_RightArrow);
            KB_ClearKeyDown(sc_kpad_Enter);
            KB_ClearKeyDown(sc_kpad_6);
            KB_ClearKeyDown(sc_kpad_9);
            KB_ClearKeyDown(sc_kpad_2);
            KB_ClearKeyDown(sc_DownArrow);
            KB_ClearKeyDown(sc_Space);
            sound(KICK_HIT);
            current_menu++;
            if (current_menu > 403) current_menu = 400;
        }

        if (KB_KeyPressed(sc_Escape))
        {
            if (g_player[myconnectindex].ps.gm&MODE_GAME)
                cmenu(50);
            else cmenu(0);
            return;
        }

        flushperms();
        rotatesprite(0,0,65536L,0,ORDERING+current_menu-400,0,0,10+16+64,0,0,xdim-1,ydim-1);

        break;
VOLUME_ALL_40x:

        c = 320>>1;

        if (KB_KeyPressed(sc_LeftArrow) ||
                KB_KeyPressed(sc_kpad_4) ||
                KB_KeyPressed(sc_UpArrow) ||
                KB_KeyPressed(sc_PgUp) ||
                KB_KeyPressed(sc_kpad_8) ||
                WHEELDOWN)
        {
            KB_ClearKeyDown(sc_LeftArrow);
            KB_ClearKeyDown(sc_kpad_4);
            KB_ClearKeyDown(sc_UpArrow);
            KB_ClearKeyDown(sc_PgUp);
            KB_ClearKeyDown(sc_kpad_8);
            sound(KICK_HIT);
            current_menu--;
            if (current_menu < 400) current_menu = 401;
        }
        else if (
            KB_KeyPressed(sc_PgDn) ||
            KB_KeyPressed(sc_Enter) ||
            KB_KeyPressed(sc_kpad_Enter) ||
            KB_KeyPressed(sc_RightArrow) ||
            KB_KeyPressed(sc_DownArrow) ||
            KB_KeyPressed(sc_kpad_2) ||
            KB_KeyPressed(sc_kpad_9) ||
            KB_KeyPressed(sc_Space) ||
            KB_KeyPressed(sc_kpad_6) ||
            LMB || WHEELUP)
        {
            KB_ClearKeyDown(sc_PgDn);
            KB_ClearKeyDown(sc_Enter);
            KB_ClearKeyDown(sc_RightArrow);
            KB_ClearKeyDown(sc_kpad_Enter);
            KB_ClearKeyDown(sc_kpad_6);
            KB_ClearKeyDown(sc_kpad_9);
            KB_ClearKeyDown(sc_kpad_2);
            KB_ClearKeyDown(sc_DownArrow);
            KB_ClearKeyDown(sc_Space);
            sound(KICK_HIT);
            current_menu++;
            if (current_menu > 401) current_menu = 400;
        }

        x = probe(0,0,0,1);

        if (x == -1)
        {
            if (g_player[myconnectindex].ps.gm&MODE_GAME)
                cmenu(50);
            else cmenu(0);
            return;
        }

        flushperms();
        switch (current_menu)
        {
        case 400:
            rotatesprite(0,0,65536L,0,TEXTSTORY,0,0,10+16+64, 0,0,xdim-1,ydim-1);
            break;
        case 401:
            rotatesprite(0,0,65536L,0,F1HELP,0,0,10+16+64, 0,0,xdim-1,ydim-1);
            break;
        }

        break;

    case 500:
    case 502:
        c = 320>>1;

        gametext(c,90,"Are you sure you want to quit?",0,2+8+16);
        gametext(c,99,"(Y/N)",0,2+8+16);

        if (KB_KeyPressed(sc_Space) || KB_KeyPressed(sc_Enter) || KB_KeyPressed(sc_kpad_Enter) || KB_KeyPressed(sc_Y) || LMB)
        {
            KB_FlushKeyboardQueue();

            sendquit();
        }

        x = probe(186,124,0,1);
        if (x == -1 || KB_KeyPressed(sc_N) || RMB)
        {
            KB_ClearKeyDown(sc_N);
            quittimer = 0;
            if (g_player[myconnectindex].ps.gm&MODE_DEMO && ud.recstat == 2)
                g_player[myconnectindex].ps.gm = MODE_DEMO;
            else
            {
                if (current_menu == 502)
                {
                    cmenu(last_menu);
                    probey = last_menu_pos;
                }
                else if (!(g_player[myconnectindex].ps.gm & MODE_GAME || ud.recstat == 2))
                    cmenu(0);
                else g_player[myconnectindex].ps.gm &= ~MODE_MENU;
                if (ud.multimode < 2  && ud.recstat != 2)
                {
                    ready2send = 1;
                    totalclock = ototalclock;
                }
            }
        }

        break;
    case 501:
        c = 320>>1;
        gametext(c,90,"Quit to Title?",0,2+8+16);
        gametext(c,99,"(Y/N)",0,2+8+16);

        if (KB_KeyPressed(sc_Space) || KB_KeyPressed(sc_Enter) || KB_KeyPressed(sc_kpad_Enter) || KB_KeyPressed(sc_Y) || LMB)
        {
            KB_FlushKeyboardQueue();
            g_player[myconnectindex].ps.gm = MODE_DEMO;
            if (ud.recstat == 1)
                closedemowrite();
            cmenu(0);
        }

        x = probe(186,124,0,0);

        if (x == -1 || KB_KeyPressed(sc_N) || RMB)
        {
            g_player[myconnectindex].ps.gm &= ~MODE_MENU;
            if (ud.multimode < 2  && ud.recstat != 2)
            {
                ready2send = 1;
                totalclock = ototalclock;
            }
        }

        break;

    case 601:
        displayfragbar();
        rotatesprite(160<<16,29<<16,65536L,0,MENUBAR,16,0,10,0,0,xdim-1,ydim-1);
        menutext(320>>1,34,0,0,&g_player[myconnectindex].user_name[0]);

        sprintf(tempbuf,"Waiting for master");
        gametext(160,50,tempbuf,0,2+8+16);
        gametext(160,59,"to select level",0,2+8+16);

        if (KB_KeyPressed(sc_Escape))
        {
            KB_ClearKeyDown(sc_Escape);
            sound(EXITMENUSOUND);
            cmenu(0);
        }
        break;

    case 602:
        if (menunamecnt == 0)
        {
            //        getfilenames("SUBD");
            getfilenames(".","*.MAP");
            if (menunamecnt == 0)
                cmenu(600);
        }

    case 603:
        {
            int plrvotes = 0, j = 0;

            x = probe(186,124,0,0);

            if (voting != myconnectindex)
                g_player[myconnectindex].ps.gm &= ~MODE_MENU;

            if (x == -1)
            {
                if (voting == myconnectindex)
                {
                    Bmemset(votes,0,sizeof(votes));
                    Bmemset(gotvote,0,sizeof(gotvote));

                    tempbuf[0] = 18;
                    tempbuf[1] = 2;
                    tempbuf[2] = myconnectindex;

                    for (c=connecthead;c>=0;c=connectpoint2[c])
                    {
                        if (c != myconnectindex) sendpacket(c,tempbuf,3);
                        if ((!networkmode) && (myconnectindex != connecthead)) break; //slaves in M/S mode only send to master
                    }
                    voting = -1;
                }
                cmenu(0);
            }

            for (i=0;i<MAXPLAYERS;i++)
            {
                plrvotes += votes[i];
                j += gotvote[i];
            }
            if (j == numplayers || !g_player[myconnectindex].ps.i || (plrvotes > (numplayers>>1)) || (!networkmode && myconnectindex == connecthead))
            {
                if (plrvotes > (numplayers>>1) || !g_player[myconnectindex].ps.i || (!networkmode && myconnectindex == connecthead))
                {
                    if (ud.m_player_skill == 3) ud.m_respawn_monsters = 1;
                    else ud.m_respawn_monsters = 0;

                    if ((gametype_flags[ud.m_coop] & GAMETYPE_FLAG_ITEMRESPAWN)) ud.m_respawn_items = 1;
                    else ud.m_respawn_items = 0;

                    ud.m_respawn_inventory = 1;

                    for (c=connecthead;c>=0;c=connectpoint2[c])
                    {
                        resetweapons(c);
                        resetinventory(c);

                    }

                    mpchangemap(ud.m_volume_number,ud.m_level_number);

                    if (voting == myconnectindex)
                        adduserquote("VOTE SUCCEEDED");

                    newgame(ud.m_volume_number,ud.m_level_number,ud.m_player_skill+1);
                    if (enterlevel(MODE_GAME)) backtomenu();

                    return;
                }
                else if (j == numplayers)
                {
                    Bmemset(votes,0,sizeof(votes));
                    Bmemset(gotvote,0,sizeof(gotvote));
                    voting = -1;

                    tempbuf[0] = 18;
                    tempbuf[1] = 2;
                    tempbuf[2] = myconnectindex;
                    tempbuf[3] = 1;

                    for (c=connecthead;c>=0;c=connectpoint2[c])
                    {
                        if (c != myconnectindex) sendpacket(c,tempbuf,4);
                        if ((!networkmode) && (myconnectindex != connecthead)) break; //slaves in M/S mode only send to master
                    }

                    Bsprintf(fta_quotes[116],"VOTE FAILED");
                    FTA(116,&g_player[myconnectindex].ps);
                    g_player[myconnectindex].ps.gm &= ~MODE_MENU;
                }
            }
            else
            {
                gametext(160,90,"WAITING FOR VOTES",0,2);
            }
            break;
        }
    case 600:
        c = (320>>1) - 120;
        if ((g_player[myconnectindex].ps.gm&MODE_GAME) != MODE_GAME)
            displayfragbar();
        rotatesprite(160<<16,26<<16,65536L,0,MENUBAR,16,0,10,0,0,xdim-1,ydim-1);
        menutext(160,31,0,0,&g_player[myconnectindex].user_name[0]);

        x = probe(c,57-8,16,8);

        modval(0,num_gametypes-1,(int *)&ud.m_coop,1,probey==0);
        if (!VOLUMEONE)
            modval(0,num_volumes-1,(int *)&ud.m_volume_number,1,probey==1);

        i = ud.m_level_number;

        modval(0,ud.m_volume_number == 0?6+(boardfilename[0]!=0):MAXLEVELS-1,(int *)&ud.m_level_number,1,probey==2);

        if ((gametype_flags[ud.m_coop] & GAMETYPE_FLAG_MARKEROPTION))
            modval(0,1,(int *)&ud.m_marker,1,probey==4);
        if ((gametype_flags[ud.m_coop] & (GAMETYPE_FLAG_PLAYERSFRIENDLY|GAMETYPE_FLAG_TDM)))
            modval(0,1,(int *)&ud.m_ffire,1,probey==5);
        else modval(0,1,(int *)&ud.m_noexits,1,probey==5);

        if (probey == 1)
            if (ud.m_volume_number == 0 && ud.m_level_number > 6+(boardfilename[0]!=0))
                ud.m_level_number = 0;

        while (map[(ud.m_volume_number*MAXLEVELS)+ud.m_level_number].name == NULL)
        {
            if (ud.m_level_number < i || i == 0)
                ud.m_level_number--;
            else ud.m_level_number++;

            if (ud.m_level_number > MAXLEVELS-1 || ud.m_level_number < 0)
            {
                ud.m_level_number = 0;
                break;
            }
        }

        switch (x)
        {
        case -1:
            ud.m_recstat = 0;
            if (g_player[myconnectindex].ps.gm&MODE_GAME) cmenu(50);
            else cmenu(0);
            break;
        case 0:
            ud.m_coop++;
            if (ud.m_coop == num_gametypes) ud.m_coop = 0;
            break;
        case 1:
            if (!VOLUMEONE)
            {
                ud.m_volume_number++;
                if (ud.m_volume_number == num_volumes) ud.m_volume_number = 0;
                if (ud.m_volume_number == 0 && ud.m_level_number > 6+(boardfilename[0]!=0))
                    ud.m_level_number = 0;
                if (ud.m_level_number > MAXLEVELS-1) ud.m_level_number = 0;
            }
            break;
        case 2:
            ud.m_level_number++;
            if (!VOLUMEONE)
            {
                if (ud.m_volume_number == 0 && ud.m_level_number > 6+(boardfilename[0]!=0))
                    ud.m_level_number = 0;
            }
            else
            {
                if (ud.m_volume_number == 0 && ud.m_level_number > 5)
                    ud.m_level_number = 0;
            }
            if (ud.m_level_number > MAXLEVELS-1) ud.m_level_number = 0;
            break;
        case 3:
            if (ud.m_monsters_off == 1 && ud.m_player_skill > 0)
                ud.m_monsters_off = 0;

            if (ud.m_monsters_off == 0)
            {
                ud.m_player_skill++;
                if (ud.m_player_skill > 3)
                {
                    ud.m_player_skill = 0;
                    ud.m_monsters_off = 1;
                }
            }
            else ud.m_monsters_off = 0;

            break;

        case 4:
            if ((gametype_flags[ud.m_coop] & GAMETYPE_FLAG_MARKEROPTION))
                ud.m_marker = !ud.m_marker;
            break;

        case 5:
            if ((gametype_flags[ud.m_coop] & (GAMETYPE_FLAG_PLAYERSFRIENDLY|GAMETYPE_FLAG_TDM)))
                ud.m_ffire = !ud.m_ffire;
            else ud.m_noexits = !ud.m_noexits;
            break;

        case 6:
            if (VOLUMEALL)
            {
                currentlist = 1;
                last_menu_pos = probey;
                cmenu(101);
            }
            break;
        case 7:
            if (voting == -1)
            {
                if (g_player[myconnectindex].ps.i)
                {
                    Bmemset(votes,0,sizeof(votes));
                    Bmemset(gotvote,0,sizeof(gotvote));
                    votes[myconnectindex] = gotvote[myconnectindex] = 1;
                    voting = myconnectindex;

                    tempbuf[0] = 18;
                    tempbuf[1] = 1;
                    tempbuf[2] = myconnectindex;
                    tempbuf[3] = ud.m_volume_number;
                    tempbuf[4] = ud.m_level_number;

                    for (c=connecthead;c>=0;c=connectpoint2[c])
                    {
                        if (c != myconnectindex) sendpacket(c,tempbuf,5);
                        if ((!networkmode) && (myconnectindex != connecthead)) break; //slaves in M/S mode only send to master
                    }
                }
                if ((gametype_flags[ud.m_coop] & GAMETYPE_FLAG_PLAYERSFRIENDLY) && !(gametype_flags[ud.m_coop] & GAMETYPE_FLAG_TDM))
                    ud.m_noexits = 0;

                cmenu(603);
            }
            break;
        }

        c += 40;

        //if(ud.m_coop==1) gametext(c+70,57-7-9,"COOPERATIVE PLAY",0,2+8+16);
        //else if(ud.m_coop==2) gametext(c+70,57-7-9,"DUKEMATCH (NO SPAWN)",0,2+8+16);
        //else gametext(c+70,57-7-9,"DUKEMATCH (SPAWN)",0,2+8+16);
        gametext(c+70,57-7-9,gametype_names[ud.m_coop],MENUHIGHLIGHT(0),2+8+16);
        if (VOLUMEONE)
        {
            gametext(c+70,57+16-7-9,volume_names[ud.m_volume_number],MENUHIGHLIGHT(1),2+8+16);
        }
        else
        {
            gametext(c+70,57+16-7-9,volume_names[ud.m_volume_number],MENUHIGHLIGHT(1),2+8+16);
        }

        gametext(c+70,57+16+16-7-9,&map[MAXLEVELS*ud.m_volume_number+ud.m_level_number].name[0],MENUHIGHLIGHT(2),2+8+16);

        gametext(c+70,57+16+16+16-7-9,ud.m_monsters_off == 0 || ud.m_player_skill > 0?skill_names[ud.m_player_skill]:"NONE",MENUHIGHLIGHT(3),2+8+16);

        if (gametype_flags[ud.m_coop] & GAMETYPE_FLAG_MARKEROPTION)
            gametext(c+70,57+16+16+16+16-7-9,ud.m_marker?"ON":"OFF",MENUHIGHLIGHT(4),2+8+16);

        if (gametype_flags[ud.m_coop] & (GAMETYPE_FLAG_PLAYERSFRIENDLY|GAMETYPE_FLAG_TDM))
            gametext(c+70,57+16+16+16+16+16-7-9,ud.m_ffire?"ON":"OFF",MENUHIGHLIGHT(5),2+8+16);
        else gametext(c+70,57+16+16+16+16+16-7-9,ud.m_noexits?"OFF":"ON",MENUHIGHLIGHT(5),2+8+16);

        c -= 44;

        menutext(c,57-9,MENUHIGHLIGHT(0),PHX(-2),"GAME TYPE");

        if (VOLUMEONE)
        {
            sprintf(tempbuf,"EPISODE %ld",ud.m_volume_number+1);
            menutext(c,57+16-9,MENUHIGHLIGHT(1),1,tempbuf);
        }
        else
        {
            sprintf(tempbuf,"EPISODE %ld",ud.m_volume_number+1);
            menutext(c,57+16-9,MENUHIGHLIGHT(1),PHX(-3),tempbuf);
        }

        sprintf(tempbuf,"LEVEL %ld",ud.m_level_number+1);
        menutext(c,57+16+16-9,MENUHIGHLIGHT(2),PHX(-4),tempbuf);

        menutext(c,57+16+16+16-9,MENUHIGHLIGHT(3),PHX(-5),"MONSTERS");

        if (gametype_flags[ud.m_coop] & GAMETYPE_FLAG_MARKEROPTION)
            menutext(c,57+16+16+16+16-9,MENUHIGHLIGHT(4),PHX(-6),"MARKERS");
        else
            menutext(c,57+16+16+16+16-9,MENUHIGHLIGHT(4),1,"MARKERS");

        if (gametype_flags[ud.m_coop] & (GAMETYPE_FLAG_PLAYERSFRIENDLY|GAMETYPE_FLAG_TDM))
            menutext(c,57+16+16+16+16+16-9,MENUHIGHLIGHT(5),0,"FR. FIRE");
        else menutext(c,57+16+16+16+16+16-9,MENUHIGHLIGHT(5),0,"MAP EXITS");

        if (VOLUMEALL)
        {
            menutext(c,57+16+16+16+16+16+16-9,MENUHIGHLIGHT(6),0,"USER MAP");
            if (boardfilename[0] != 0)
                gametext(c+70+44,57+16+16+16+16+16,boardfilename,MENUHIGHLIGHT(6),2+8+16);
        }
        else
        {
            menutext(c,57+16+16+16+16+16+16-9,MENUHIGHLIGHT(6),1,"USER MAP");
        }

        menutext(c,57+16+16+16+16+16+16+16-9,MENUHIGHLIGHT(7),voting!=-1,"START GAME");

        break;
    }

    if ((g_player[myconnectindex].ps.gm&MODE_MENU) != MODE_MENU)
    {
        vscrn();
        cameraclock = totalclock;
        cameradist = 65536L;
    }
}
