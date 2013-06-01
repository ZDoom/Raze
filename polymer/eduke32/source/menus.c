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
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
*/
//-------------------------------------------------------------------------

// FIXME: this file is the single worst fucking mess in all of EDuke32

#include "duke3d.h"
#include "net.h"
#include "player.h"
#include "mouse.h"
#include "joystick.h"
#include "osd.h"
#include "osdcmds.h"
#include "gamedef.h"
#include "gameexec.h"
#include "savegame.h"
#include "premap.h"
#include "demo.h"
#include "crc32.h"
#include "common.h"
#include "common_game.h"
#include "input.h"
#include "menus.h"

#include <sys/stat.h>

extern char inputloc;
int16_t g_skillSoundID=-1;
int32_t probey=0; // the row number on which the menu cursor is positioned
static int32_t lastsavehead=0,last_menu_pos=0,last_menu,sh,onbar,buttonstat;
static int32_t last_zero,last_fifty,last_onehundred,last_twoohtwo,last_threehundred = 0;

// ugly hack to get around inadequacies of calling CONTROL_GetInput() in M_Probe() and still have gamefuncs function throughout menus
// A previous solution was to add CONTROL_GetInput(&minfo); in M_DisplayMenus but that hurt mouse aiming
#define AdvanceTrigger  0x01
#define ReturnTrigger   0x02
#define EscapeTrigger   0x04
#define ProbeTriggers(x) (menutriggers&x)
#define ProbeTriggersClear(x) (menutriggers&=~x)
static int32_t menutriggers=0;

static char menunamecnt;

static fnlist_t fnlist;
static CACHE1D_FIND_REC *finddirshigh=NULL, *findfileshigh=NULL;
static int32_t currentlist=0;

static void set_findhighs(void)
{
    finddirshigh = fnlist.finddirs;
    findfileshigh = fnlist.findfiles;
    currentlist = 0;
    if (findfileshigh)
        currentlist = 1;
}


static int32_t function, whichkey;
static int32_t changesmade, newvidmode, curvidmode, newfullscreen;
static int32_t vidsets[16] = { -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1 };
static int32_t curvidset, newvidset = 0;
static int32_t soundbits, soundvoices, soundrate;
#define NUMDOUBLEMBTNS 3  // # of mouse buttons that can be double-clicked (mouse1 - mouse3)
#define NUMSINGLEMBTNS 4  // # of mouse buttons that can only be single-clicked (the rest)
#define NUMMOUSEFUNCTIONS (NUMDOUBLEMBTNS*2+NUMSINGLEMBTNS)
// NOTE: NUMMOUSEFUNCTIONS must equal MAXMOUSEBUTTONS!!!
static const char *mousebuttonnames[NUMMOUSEFUNCTIONS==MAXMOUSEBUTTONS ? MAXMOUSEBUTTONS : -1] =
{ "Mouse1", "Mouse2", "Mouse3", "Mouse4", "Wheel Up",
  "Wheel Down", "Mouse5", "Mouse6", "Mouse7", "Mouse8"
};

extern int32_t voting;

#define USERMAPENTRYLENGTH 25

#define mgametext(x,y,t,s,dabits) G_PrintGameText(2,STARTALPHANUM, x,y,t,s,0,dabits,0, 0, xdim-1, ydim-1, 65536)
#define mgametextpal(x,y,t,s,p) G_PrintGameText(2,STARTALPHANUM, x,y,t,s,p,26,0, 0, xdim-1, ydim-1, 65536)

void M_ChangeMenu(int32_t cm)
{
    if (G_HaveEvent(EVENT_CHANGEMENU))
        cm = VM_OnEvent(EVENT_CHANGEMENU, g_player[myconnectindex].ps->i, myconnectindex, -1, cm);

    if (cm >= 0)
    {
        g_currentMenu = cm;

        switch (g_currentMenu)
        {
        case MENU_MAIN:
            probey = last_zero;
            break;
        case MENU_MAIN_INGAME:
            probey = last_fifty;
            break;
        case MENU_EPISODE:
            probey = last_onehundred;
            break;
        case MENU_OPTIONS:
            probey = last_twoohtwo;
            break;
        case MENU_SKILL:
            probey = 1;
            break;
        default:
            if (cm >= MENU_LOAD && cm < MENU_STORY)
                probey = last_threehundred;
            else if ((cm >= 1000 && cm <= 1009))
                return;
            else probey = 0;
            break;
        }

        lastsavehead = -1;
    }
}

static void M_LinearPanels(int32_t firstMenu, int32_t lastMenu)
{
    int32_t changedMenu = g_currentMenu;

    if (I_PanelUp())
    {
        I_PanelUpClear();

        S_PlaySound(KICK_HIT);
        changedMenu--;
        if (changedMenu < firstMenu) changedMenu = lastMenu;
        M_ChangeMenu(changedMenu);
    }
    else if (I_PanelDown())
    {
        I_PanelDownClear();

        S_PlaySound(KICK_HIT);
        changedMenu++;
        if (changedMenu > lastMenu) changedMenu = firstMenu;
        M_ChangeMenu(changedMenu);
    }
}

#define LMB (buttonstat&LEFT_MOUSE)
#define RMB (buttonstat&RIGHT_MOUSE)
#define WHEELUP (buttonstat&WHEELUP_MOUSE)
#define WHEELDOWN (buttonstat&WHEELDOWN_MOUSE)

static ControlInfo minfo;

static int16_t mi, mii;

static int32_t probe_(int32_t type,int32_t x,int32_t y,int32_t i,int32_t n)
{
    int16_t centre;

    CONTROL_GetInput(&minfo);
    mi += (minfo.dpitch+minfo.dz);
    mii += minfo.dyaw;

    menutriggers = 0;

    if (x == (320>>1))
        centre = 320>>2;
    else centre = 0;

    if (!buttonstat || buttonstat == WHEELUP_MOUSE || buttonstat == WHEELDOWN_MOUSE)
    {
        if (KB_KeyPressed(sc_UpArrow) || KB_KeyPressed(sc_kpad_8) || mi < -8192 || WHEELUP || BUTTON(gamefunc_Move_Forward) || (JOYSTICK_GetHat(0)&HAT_UP))
        {
            mi = mii = 0;
            KB_ClearKeyDown(sc_UpArrow);
            KB_ClearKeyDown(sc_kpad_8);
            MOUSE_ClearButton(WHEELUP_MOUSE);
            CONTROL_ClearButton(gamefunc_Move_Forward);
            JOYSTICK_ClearHat(0);

            S_PlaySound(KICK_HIT);

            probey--;
            if (probey < 0)
                probey = n-1;
        }
        if (KB_KeyPressed(sc_PgUp))
        {
            // n is >= NUMGAMEFUNCTIONS from mouse/keyboard setup
            int32_t step = (n >= NUMGAMEFUNCTIONS) ? 13/2 : n/2;

            KB_ClearKeyDown(sc_PgUp);

            S_PlaySound(KICK_HIT);

//            if (probey == 0) probey = n-1; else
            probey = max(0, probey-step);
        }
        if (KB_KeyPressed(sc_Home))
        {
            // does not get in the way of special HOME handling in user map list
            KB_ClearKeyDown(sc_Home);

            S_PlaySound(KICK_HIT);

            probey = 0;
        }

        if (KB_KeyPressed(sc_DownArrow) || KB_KeyPressed(sc_kpad_2) || mi > 8192 || WHEELDOWN || BUTTON(gamefunc_Move_Backward) || (JOYSTICK_GetHat(0)&HAT_DOWN))
        {
            mi = mii = 0;
            KB_ClearKeyDown(sc_DownArrow);
            KB_ClearKeyDown(sc_kpad_2);
            KB_ClearKeyDown(sc_PgDn);
            MOUSE_ClearButton(WHEELDOWN_MOUSE);
            CONTROL_ClearButton(gamefunc_Move_Backward);
            JOYSTICK_ClearHat(0);

            S_PlaySound(KICK_HIT);

            probey++;
            if (probey >= n)
                probey = 0;
        }
        if (KB_KeyPressed(sc_PgDn))
        {
            int32_t step = (n >= NUMGAMEFUNCTIONS) ? 13/2 : n/2;

            KB_ClearKeyDown(sc_PgDn);

            S_PlaySound(KICK_HIT);

//            if (probey == n-1) probey = 0; else
            probey = min(n-1, probey+step);
        }
        if (KB_KeyPressed(sc_End))
        {
            // does not get in the way of special END handling in user map list
            KB_ClearKeyDown(sc_End);

            S_PlaySound(KICK_HIT);

            probey = n-1;
        }
    }

    // XXX: check for probey < 0 needed here? (we have M_Probe(..., 0) calls)

    if (probey >= n)  // not sure if still necessary
        probey = 0;

    if (x || y)
    {
        if (centre)
        {
            //        rotatesprite_fs(((320>>1)+(centre)+54)<<16,(y+(probey*i)-4)<<16,65536L,0,SPINNINGNUKEICON+6-((6+(totalclock>>3))%7),sh,0,10);
            //        rotatesprite_fs(((320>>1)-(centre)-54)<<16,(y+(probey*i)-4)<<16,65536L,0,SPINNINGNUKEICON+((totalclock>>3)%7),sh,0,10);

            rotatesprite_fs(((320>>1)+(centre>>1)+70)<<16,(y+(probey*i)-4)<<16,65536L>>type,0,SPINNINGNUKEICON+6-((6+(totalclock>>3))%7),sh,0,10);
            rotatesprite_fs(((320>>1)-(centre>>1)-70)<<16,(y+(probey*i)-4)<<16,65536L>>type,0,SPINNINGNUKEICON+((totalclock>>3)%7),sh,0,10);
        }
        else
            rotatesprite_fs((x<<16)-((tilesizx[BIGFNTCURSOR]-4)<<(16-type)),(y+(probey*i)-(4>>type))<<16,65536L>>type,0,SPINNINGNUKEICON+(((totalclock>>3))%7),sh,0,10);
    }

    if (I_AdvanceTrigger() && !onbar)
    {
        if (g_currentMenu != MENU_SKILL)
            S_PlaySound(PISTOL_BODYHIT);

        menutriggers |= AdvanceTrigger;

        I_AdvanceTriggerClear();

        return(probey);
    }
    else if (I_ReturnTrigger())
    {
        S_PlaySound(EXITMENUSOUND);

        menutriggers |= ReturnTrigger;

        I_ReturnTriggerClear();

        onbar = 0;
        return(-1);
    }
    else
    {
        if (onbar == 0)
            return(-probey-2);

        if (KB_KeyPressed(sc_LeftArrow) || KB_KeyPressed(sc_kpad_4) || (LMB && (WHEELUP || mii < -256)) || BUTTON(gamefunc_Turn_Left) ||
            BUTTON(gamefunc_Strafe_Left) || (JOYSTICK_GetHat(0)&HAT_LEFT))
            return(probey);
        else if (KB_KeyPressed(sc_RightArrow) || KB_KeyPressed(sc_kpad_6) || (LMB && (WHEELDOWN || mii > 256)) || BUTTON(gamefunc_Turn_Right) ||
            BUTTON(gamefunc_Strafe_Right) || (JOYSTICK_GetHat(0)&HAT_RIGHT))
            return(probey);

        return(-probey-2);
    }
}
static inline int32_t M_Probe(int32_t x,int32_t y,int32_t i,int32_t n)
{
    return probe_(0,x,y,i,n);
}

static inline int32_t probesm(int32_t x,int32_t y,int32_t i,int32_t n)
{
    return probe_(1,x,y,i,n);
}

// see menutriggers for why this is a sorry hack
// calling CONTROL_GetFunctionInput(); from input.c also would not work
// and probe_() must remain static
static int32_t Menu_EnterText(int32_t x,int32_t y,char *t,int32_t dalen,int32_t c)
{
    int32_t probeysave = probey;
    int32_t probeoutputx = M_Probe(0,0,0,1);
    probey = probeysave;

    if (ProbeTriggers(AdvanceTrigger) || I_AdvanceTrigger())
    {
        I_AdvanceTriggerClear();
        return (1);
    }

    if (probeoutputx == -1)
    {
        I_ReturnTriggerClear();
        return (-1);
    }

    return G_EnterText(x, y, t, dalen, c);
}

int32_t menutext_(int32_t x,int32_t y,int32_t s,int32_t p,char *t,int32_t bits)
{
    vec2_t dim;
    int32_t f = TEXT_BIGALPHANUM|TEXT_UPPERCASE|TEXT_LITERALESCAPE;

    if (x == 160)
        f |= TEXT_XCENTER;

    dim = G_ScreenText(BIGALPHANUM, x, y-12, 65536L, 0, 0, t, s, p, bits, 0, 5, 16, 0, 0, f, 0, 0, xdim-1, ydim-1);

    return dim.x;
}


// This function depends on the 'onbar' variable which should be set to the
// 'probey' indices where there's a slider bar.
static void sliderbar(int32_t type, int32_t x,int32_t y,int32_t *p,int32_t dainc,int32_t damodify,int32_t s, int32_t pa, int32_t min, int32_t max)
{
    int32_t xloc;
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
        if (*p >= min && *p <= max && (KB_KeyPressed(sc_LeftArrow) || KB_KeyPressed(sc_kpad_4) || (LMB && (WHEELUP || mii < -256)) ||
            BUTTON(gamefunc_Turn_Left) || BUTTON(gamefunc_Strafe_Left) || (JOYSTICK_GetHat(0)&HAT_LEFT)))         // && onbar) )
        {
            KB_ClearKeyDown(sc_LeftArrow);
            KB_ClearKeyDown(sc_kpad_4);
            MOUSE_ClearButton(WHEELUP_MOUSE);
            CONTROL_ClearButton(gamefunc_Turn_Left);
            CONTROL_ClearButton(gamefunc_Strafe_Left);
            JOYSTICK_ClearHat(0);
            mii = 0;
            if (!rev)
                *p -= dainc;
            else *p += dainc;
            if (*p < min)
                *p = min;
            if (*p > max)
                *p = max;
            S_PlaySound(KICK_HIT);
        }
        if (*p <= max && *p >= min && (KB_KeyPressed(sc_RightArrow) || KB_KeyPressed(sc_kpad_6) || (LMB && (WHEELDOWN || mii > 256)) ||
            BUTTON(gamefunc_Turn_Right) || BUTTON(gamefunc_Strafe_Right) || (JOYSTICK_GetHat(0)&HAT_RIGHT)))        //&& onbar) )
        {
            KB_ClearKeyDown(sc_RightArrow);
            KB_ClearKeyDown(sc_kpad_6);
            MOUSE_ClearButton(WHEELDOWN_MOUSE);
            CONTROL_ClearButton(gamefunc_Turn_Right);
            CONTROL_ClearButton(gamefunc_Strafe_Right);
            JOYSTICK_ClearHat(0);
            mii = 0;
            if (!rev)
                *p += dainc;
            else *p -= dainc;
            if (*p > max)
                *p = max;
            if (*p < min)
                *p = min;
            S_PlaySound(KICK_HIT);
        }
    }

    xloc = *p;
    rotatesprite_fs((x<<16)+(22<<(16-type)),(y<<16)-(3<<(16-type)),65536L>>type,0,SLIDEBAR,s,pa,10);
    if (rev == 0)
        rotatesprite_fs((x<<16)+((scale(64,xloc-min,max-min)+1)<<(16-type)),(y<<16)+(1<<(16-type)),65536L>>type,0,SLIDEBAR+1,s,pa,10);
    else
        rotatesprite_fs((x<<16)+((65-scale(64,xloc-min,max-min))<<(16-type)),(y<<16)+(1<<(16-type)),65536L>>type,0,SLIDEBAR+1,s,pa,10);
}

#define bar(x,y,p,dainc,damodify,s,pa) sliderbar(0,x,y,p,dainc,damodify,s,pa,0,63);
#define barsm(x,y,p,dainc,damodify,s,pa) sliderbar(1,x,y,p,dainc,damodify,s,pa,0,63);

static void modval(int32_t min, int32_t max,int32_t *p,int32_t dainc,int32_t damodify)
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
            if (KB_KeyPressed(sc_LeftArrow) || KB_KeyPressed(sc_kpad_4) || (LMB && minfo.dyaw < -256) || BUTTON(gamefunc_Turn_Left) ||
                BUTTON(gamefunc_Strafe_Left) || (JOYSTICK_GetHat(0)&HAT_LEFT))        // && onbar) )
            {
                KB_ClearKeyDown(sc_LeftArrow);
                KB_ClearKeyDown(sc_kpad_4);
                CONTROL_ClearButton(gamefunc_Turn_Left);
                CONTROL_ClearButton(gamefunc_Strafe_Left);
                JOYSTICK_ClearHat(0);

                *p -= dainc;
                if (*p < min)
                {
                    *p = max;
                    if (damodify == 2)
                        *p = min;
                }
                S_PlaySound(PISTOL_BODYHIT);
            }
            if (KB_KeyPressed(sc_RightArrow) || KB_KeyPressed(sc_kpad_6) || (LMB && minfo.dyaw > 256) || BUTTON(gamefunc_Turn_Right) ||
                BUTTON(gamefunc_Strafe_Right) || (JOYSTICK_GetHat(0)&HAT_RIGHT))       //&& onbar) )
            {
                KB_ClearKeyDown(sc_RightArrow);
                KB_ClearKeyDown(sc_kpad_6);
                CONTROL_ClearButton(gamefunc_Turn_Right);
                CONTROL_ClearButton(gamefunc_Strafe_Right);
                JOYSTICK_ClearHat(0);

                *p += dainc;
                if (*p > max)
                {
                    *p = min;
                    if (damodify == 2)
                        *p = max;
                }
                S_PlaySound(PISTOL_BODYHIT);
            }
        }
        else
        {
            if (KB_KeyPressed(sc_RightArrow) || KB_KeyPressed(sc_kpad_6) || (LMB && minfo.dyaw > 256) || BUTTON(gamefunc_Turn_Right) ||
                BUTTON(gamefunc_Strafe_Right) || (JOYSTICK_GetHat(0)&HAT_RIGHT))      //&& onbar ))
            {
                KB_ClearKeyDown(sc_RightArrow);
                KB_ClearKeyDown(sc_kpad_6);
                CONTROL_ClearButton(gamefunc_Turn_Right);
                CONTROL_ClearButton(gamefunc_Strafe_Right);
                JOYSTICK_ClearHat(0);

                *p -= dainc;
                if (*p < min)
                {
                    *p = max;
                    if (damodify == 2)
                        *p = min;
                }
                S_PlaySound(PISTOL_BODYHIT);
            }
            if (KB_KeyPressed(sc_LeftArrow) || KB_KeyPressed(sc_kpad_4) || (LMB && minfo.dyaw < -256) || BUTTON(gamefunc_Turn_Left) ||
                BUTTON(gamefunc_Strafe_Left) || (JOYSTICK_GetHat(0)&HAT_LEFT))      // && onbar) )
            {
                KB_ClearKeyDown(sc_LeftArrow);
                KB_ClearKeyDown(sc_kpad_4);
                CONTROL_ClearButton(gamefunc_Turn_Left);
                CONTROL_ClearButton(gamefunc_Strafe_Left);
                JOYSTICK_ClearHat(0);

                *p += dainc;
                if (*p > max)
                {
                    *p = min;
                    if (damodify == 2)
                        *p = max;
                }
                S_PlaySound(PISTOL_BODYHIT);
            }
        }
    }
}

#define UNSELMENUSHADE 10
#define DISABLEDMENUSHADE 20
#define MENUHIGHLIGHT(x) probey==x?(sintable[(totalclock<<5)&2047]>>12):UNSELMENUSHADE
// #define MENUHIGHLIGHT(x) probey==x?-(sintable[(totalclock<<4)&2047]>>12):probey-x>=0?(probey-x)<<2:-((probey-x)<<2)

#define SHX(X) 0
// ((x==X)*(-sh))
// ((x==X)?1:2)
//#define MWIN(X) rotatesprite( 320<<15,200<<15,X,0,MENUSCREEN,-16,0,10+64,0,0,xdim-1,ydim-1)
//#define MWINXY(X,OX,OY) rotatesprite( ( 320+(OX) )<<15, ( 200+(OY) )<<15,X,0,MENUSCREEN,-16,0,10+64,0,0,xdim-1,ydim-1)

//extern int32_t G_LoadSaveHeader(char spot,struct savehead_ *saveh);

#pragma pack(push,1)
static savehead_t savehead;
//static struct savehead_ savehead;
#pragma pack(pop)

//static int32_t volnum,levnum,plrskl,numplr;
//static char brdfn[BMAX_PATH];

static void M_DrawBackground(void)
{
    rotatesprite_fs(160<<16,200<<15,65536L,0,MENUSCREEN,16,0,10+64);
}


static void M_DrawTopBar(const char *caption)
{
    rotatesprite_fs(160<<16,19<<16,65536L,0,MENUBAR,16,0,10);
    menutext(160,24,0,0,caption);
}

static void M_DisplaySaveGameList(void)
{
    int32_t x, c = 160;

    c += 64;
    for (x = 0; x <= 108; x += 12)
        rotatesprite_fs((c+91-64)<<16,(x+56)<<16,65536L,0,TEXTBOX,24,0,10);

    rotatesprite_fs(22<<16,97<<16,65536L,0,WINDOWBORDER2,24,0,10);
    rotatesprite_fs(180<<16,97<<16,65536L,1024,WINDOWBORDER2,24,0,10);
    rotatesprite_fs(99<<16,50<<16,65536L,512,WINDOWBORDER1,24,0,10);
    rotatesprite_fs(103<<16,144<<16,65536L,1024+512,WINDOWBORDER1,24,0,10);

    for (x=0; x<=9; x++)
    {
        if (ud.savegame[x][0])
        {
            if (g_oldverSavegame[x] && g_currentMenu!=360+x)
            {
                // old version and not entering new name
                char buf[sizeof(ud.savegame[0])];
                Bmemcpy(buf, ud.savegame[x], sizeof(ud.savegame[0]));
                minitext(c,48+(12*x),buf,13,10+16);
            }
            else
            {
                minitext(c,48+(12*x),ud.savegame[x],2,10+16);
            }
        }
    }
}

extern int32_t g_quitDeadline;

void G_CheckPlayerColor(int32_t *color, int32_t prev_color)
{
    int32_t i, disallowed[] = { 1, 2, 3, 4, 5, 6, 7, 8, 17, 18, 19, 20, 22 };

    for (i=0; i<(signed)(sizeof(disallowed)/sizeof(disallowed[0])); i++)
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

static void Menus_LoadSave_DisplayCommon1(void)
{
    if (lastsavehead != probey)
        G_LoadSaveHeaderNew(probey, &savehead);
    lastsavehead = probey;

    rotatesprite_fs(101<<16,97<<16,65536L>>1,512,TILE_LOADSHOT,-32,0,4+10+64);

    if (g_oldverSavegame[probey])
    {
        menutext(53,70,0,0,"Version");
        menutext(48,90,0,0,"Mismatch");

        Bsprintf(tempbuf,"Saved: %d.%d.%d %d-bit", savehead.majorver, savehead.minorver,
                 savehead.bytever, 8*savehead.ptrsize);
        mgametext(31,104,tempbuf,0,2+8+16);
        Bsprintf(tempbuf,"Our: %d.%d.%d %d-bit", SV_MAJOR_VER, SV_MINOR_VER, BYTEVERSION,
                 (int32_t)(8*sizeof(intptr_t)));
        mgametext(31+16,QUOTE_SAVE_BAD_VERSION,tempbuf,0,2+8+16);
    }
}

void Menu_Main(void)
{
    int32_t margin = MENU_MARGIN_CENTER;
    int32_t x;

    enum
    {
        M_MAIN_EPISODE,
        M_MAIN_OPTIONS,
        M_MAIN_LOAD,
        M_MAIN_STORY,
        M_MAIN_CREDITS,
        M_MAIN_QUIT
    };


    rotatesprite_fs(margin<<16,28<<16,65536L,0,INGAMEDUKETHREEDEE,0,0,10);

    if (PLUTOPAK)   // JBF 20030804
        rotatesprite_fs((margin+100)<<16,36<<16,65536L,0,PLUTOPAKSPRITE+2,(sintable[(totalclock<<4)&2047]>>11),0,2+8);

    menutext(margin,67,MENUHIGHLIGHT(0),0,"New Game");

    //    menutext(c,67+16,0,1,"NETWORK GAME");

    menutext(margin,67+16/*+16*/,MENUHIGHLIGHT(1),0,"Options");
    menutext(margin,67+16+16/*+16*/,MENUHIGHLIGHT(2),0,"Load Game");

    if (!VOLUMEALL)
        menutext(margin,67+16+16+16/*+16*/,MENUHIGHLIGHT(3),0,"How To Order");
    else
        menutext(margin,67+16+16+16/*+16*/,MENUHIGHLIGHT(3),0,"Help");

    menutext(margin,67+16+16+16+16/*+16*/,MENUHIGHLIGHT(4),0,"Credits");
    menutext(margin,67+16+16+16+16+16/*+16*/,MENUHIGHLIGHT(5),0,"Quit");

    x = M_Probe(margin,67,16,6);

    if (x >= 0)
    {
        if ((g_netServer || ud.multimode > 1) && x == 0 && ud.recstat != 2)
        {
            last_zero = 0;
            M_ChangeMenu(600);
        }
        else
        {
            last_zero = x;
            switch (x)
            {
            case M_MAIN_EPISODE:
                M_ChangeMenu(MENU_EPISODE);
                break;
                //case 1: break;//ChangeToMenu(20001);break;   // JBF 20031128: I'm taking over the TEN menu option
            case M_MAIN_OPTIONS:
                M_ChangeMenu(MENU_OPTIONS);
                break;   // JBF 20031205: was 200
            case M_MAIN_LOAD:
                M_ChangeMenu(MENU_LOAD);
                break;
            case M_MAIN_STORY:
                KB_FlushKeyboardQueue();
                M_ChangeMenu(MENU_STORY);
                break;
            case M_MAIN_CREDITS:
                M_ChangeMenu(MENU_CREDITS);
                break;
            case M_MAIN_QUIT:
                M_ChangeMenu(MENU_QUIT);
                break;
            }
        }
    }

    if (KB_KeyPressed(sc_Q)) M_ChangeMenu(MENU_QUIT);

    if (x == -1 && (g_player[myconnectindex].ps->gm &MODE_GAME || ud.recstat == 2))
    {
        g_player[myconnectindex].ps->gm &= ~MODE_MENU;
        if ((!g_netServer && ud.multimode < 2) && ud.recstat != 2)
        {
            ready2send = 1;
            totalclock = ototalclock;
        }
    }
}

void M_DisplayMenus(void)
{
    CACHE1D_FIND_REC *dir;
    int32_t margin, x,i;
    int32_t l,m;
    const char *p = NULL;

    Net_GetPackets();

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

    if ((g_player[myconnectindex].ps->gm&MODE_MENU) == 0)
    {
        walock[TILE_LOADSHOT] = 1;
        return;
    }
    if (G_HaveEvent(EVENT_DISPLAYMENU))
        VM_OnEvent(EVENT_DISPLAYMENU, g_player[screenpeek].ps->i, screenpeek, -1, 0);

    g_player[myconnectindex].ps->gm &= (0xff-MODE_TYPE);
    g_player[myconnectindex].ps->fta = 0;

//    x = 0;

    sh = 4-(sintable[(totalclock<<4)&2047]>>11);

    // black translucent background
    if ((g_player[myconnectindex].ps->gm&MODE_GAME) || ud.recstat==2)
        if (g_currentMenu != 231 && g_currentMenu != 232)  // not in 'color correction' menu
            fade_screen_black(1);

    if (!(g_currentMenu >= 1000 && g_currentMenu <= 2999 && g_currentMenu >= MENU_LOAD && g_currentMenu <= 369))
        G_UpdateScreenArea();

    if (KB_KeyPressed(sc_Q))
    {
        switch (g_currentMenu)
        {
        case MENU_SELECTMAP:
        case MENU_KEYBOARDASSIGN:
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
        case MENU_QUIT:
        case MENU_QUITTOTITLE:
        case MENU_QUIT2:
        case 603:
        case MENU_ADULTPASSWORD:
        case 20003:
        case 20005:
            break;
        default:
            if (g_currentMenu >= 0)
            {
                last_menu = g_currentMenu;
                last_menu_pos = probey;
                M_ChangeMenu(MENU_QUIT2);
            }
            break;
        }
    }

    switch (g_currentMenu)
    {
    case 25000:
        mgametext(160,90,"Select a save spot before",0,2+8+16);
        mgametext(160,90+9,"you quick restore.",0,2+8+16);

        x = M_Probe(186,124,0,1);
        if (x >= -1)
        {
            if ((!g_netServer && ud.multimode < 2) && ud.recstat != 2)
            {
                ready2send = 1;
                totalclock = ototalclock;
            }
            g_player[myconnectindex].ps->gm &= ~MODE_MENU;
        }
        break;

    case MENU_BUYDUKE:
        x = M_Probe(326,190,0,1);
        mgametext(160,41-8,"You are playing the shareware",0,2+8+16);
        mgametext(160,50-8,"version of Duke Nukem 3D.  While",0,2+8+16);
        mgametext(160,59-8,"this version is really cool, you",0,2+8+16);
        mgametext(160,68-8,"are missing over 75% of the total",0,2+8+16);
        mgametext(160,77-8,"game, along with other great extras",0,2+8+16);
        mgametext(160,86-8,"and games, which you'll get when",0,2+8+16);
        mgametext(160,95-8,"you order the complete version and",0,2+8+16);
        mgametext(160,104-8,"get the final three episodes.",0,2+8+16);

        mgametext(160,104+8,"Please read the 'How To Order' item",0,2+8+16);
        mgametext(160,113+8,"on the main menu or visit",0,2+8+16);
        mgametext(160,122+8,"http://www.eduke32.com",0,2+8+16);
        mgametext(160,131+8,"to upgrade to the full registered",0,2+8+16);
        mgametext(160,139+8,"version of Duke Nukem 3D.",0,2+8+16);
        mgametext(160,148+16,"Press any key...",0,2+8+16);

        if (x >= -1) M_ChangeMenu(MENU_EPISODE);
        break;

    case 20001:
        M_DrawTopBar("Network Game");

        x = M_Probe(160,100-18,18,3);

        if (x == -1) M_ChangeMenu(MENU_MAIN);
        else if (x == 2) M_ChangeMenu(20010);
        else if (x == 1) M_ChangeMenu(20020);
        else if (x == 0) M_ChangeMenu(20002);

        menutext(160,100-18,MENUHIGHLIGHT(0),0,"Player Setup");
        menutext(160,100,MENUHIGHLIGHT(1),0,"Join Game");
        menutext(160,100+18,MENUHIGHLIGHT(2),0,"Host Game");
        break;

    case 20002:
    case 20003:
        M_DrawTopBar("Player Setup");
        margin = MENU_MARGIN_REGULAR;
        {
            int32_t io, ii, yy = 37, d=margin+140, enabled;
            const char *opts[] =
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
                "Switch weapons on pickup",
                "Switch weapons when empty",
                "-",
                "-",
                "Multiplayer macros",
                NULL
            };

            x = ud.color;

            if (probey == 2)
                x = G_GetTeamPalette(ud.team);

            rotatesprite_fs((260)<<16,(24+(tilesizy[APLAYER]>>1))<<16,49152L,0,1441-((((4-(totalclock>>4)))&3)*5),0,x,10);

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


            if (g_currentMenu == 20002)
            {
                x = probesm(margin,yy+5,0,io);

                if (x == -1)
                {
                    M_ChangeMenu(MENU_OPTIONS);
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
                            strcpy(buf, szPlayerName);
                            inputloc = strlen(buf);
                            M_ChangeMenu(20003);

                            I_AdvanceTriggerClear();
                        }
                        break;

                    case 1:
                        i = ud.color;
                        if (x == io)
                        {
                            ud.color++;
                            if (ud.color > 23)
                                ud.color = 0;
                            G_CheckPlayerColor((int32_t *)&ud.color,-1);
                        }
                        modval(0,23,(int32_t *)&ud.color,1,probey==1);
                        G_CheckPlayerColor((int32_t *)&ud.color,i);
                        if (ud.color != i)
                            G_UpdatePlayerFromMenu();
                        break;

                    case 2:
                        i = ud.team;
                        if (x == io)
                        {
                            ud.team++;
                            if (ud.team == 4)
                                ud.team = 0;
                        }
                        modval(0,3,(int32_t *)&ud.team,1,probey==2);
                        if (ud.team != i)
                            G_UpdatePlayerFromMenu();
                        break;

                    case 3:
                        i = ud.config.AutoAim;
                        if (x == io)
                            ud.config.AutoAim = (ud.config.AutoAim == 2) ? 0 : ud.config.AutoAim+1;
                        modval(0,2,(int32_t *)&ud.config.AutoAim,1,probey==3);
                        if (ud.config.AutoAim != i)
                            G_UpdatePlayerFromMenu();
                        break;

                    case 4:
                        i = ud.mouseaiming;
                        if (x == io)
                            ud.mouseaiming = !ud.mouseaiming;
                        modval(0,1,(int32_t *)&ud.mouseaiming,1,probey==4);
                        if (ud.mouseaiming != i)
                            G_UpdatePlayerFromMenu();
                        break;

                    case 5:
                        i = 0;
                        if (ud.weaponswitch & 1)
                        {
                            i = 1;
                            if (ud.weaponswitch & 4)
                                i = 2;
                        }
                        l = i;
                        if (x == io)
                            i = (i == 2) ? 0 : i+1;
                        modval(0,2,(int32_t *)&i,1,probey==5);
                        if (i != l)
                        {
                            if (i > 0)
                            {
                                ud.weaponswitch |= 1;
                                if (i == 2)
                                    ud.weaponswitch |= 4;
                                else
                                    ud.weaponswitch &= ~4;
                            }
                            else
                                ud.weaponswitch &= ~(1|4);

                            G_UpdatePlayerFromMenu();
                        }
                        break;
                    case 6:
                        i = 0;
                        if (ud.weaponswitch & 2)
                            i = 1;
                        if (x == io)
                            i = 1-i;
                        modval(0,1,(int32_t *)&i,1,probey==6);
                        if ((ud.weaponswitch & 2 && !i) || (!(ud.weaponswitch & 2) && i))
                        {
                            ud.weaponswitch ^= 2;
                            G_UpdatePlayerFromMenu();
                        }
                        break;
                    case 7:
                        if (x == io)
                        {
                            M_ChangeMenu(20004);
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
                // because OSD_StripColors needs a valid target and tempbuf is used in _EnterText()
                char dummybuf[64];
                x = Menu_EnterText(d-50,37,buf,30,0);

                while (Bstrlen(OSD_StripColors(dummybuf,buf)) > 10)
                {
                    buf[Bstrlen(buf)-1] = '\0';
                    inputloc--;
                }

                if (x)
                {
                    if (x == 1)
                    {
                        if (buf[0] && Bstrcmp(szPlayerName,buf))
                        {
                            Bstrcpy(szPlayerName,buf);
                            Net_SendClientInfo();
                        }
                        // send name update
                    }
                    I_AdvanceTriggerClear();

                    M_ChangeMenu(20002);
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
                        if (g_currentMenu == 20002)
                        {
                            mgametext(d-50,yy,szPlayerName,MENUHIGHLIGHT(io),2+8+16);
                        }
                        break;

                    case 1:
                    {
                        const char *s[] =
                            { "Auto","","","","","","","","","Blue","Red","Green","Gray","Dark gray","Dark green","Brown",
                              "Dark blue","","","","","Bright red","","Yellow","",""
                            };
                        mgametext(d-50,yy,s[ud.color],MENUHIGHLIGHT(io),2+8+16);
                    }
                    break;

                    case 2:
                    {
                        const char *s[] = { "Blue", "Red", "Green", "Gray" };
                        mgametext(d-50,yy,s[ud.team],MENUHIGHLIGHT(io),2+8+16);
                    }
                    break;

                    case 3:
                    {
                        const char *s[] = { "Off", "All weapons", "Bullets only" };
                        mgametext(d-50,yy,s[ud.config.AutoAim],MENUHIGHLIGHT(io),2+8+16);
                    }
                    break;

                    case 4:
                        mgametext(d-50,yy,ud.mouseaiming?"Hold button":"Toggle on/off",MENUHIGHLIGHT(io),2+8+16);
                        break;

                    case 5:
                    {
                        const char *s[] = { "Off", "All weapons", "Fav priority" };
                        i = 0;
                        if (ud.weaponswitch & 1)
                        {
                            i = 1;
                            if (ud.weaponswitch & 4)
                                i = 2;
                        }
                        mgametext(d+45,yy,s[i],MENUHIGHLIGHT(io),2+8+16);
                    }
                        break;

                    case 6:
                        mgametext(d+45,yy,ud.weaponswitch&2?"On":"Off",MENUHIGHLIGHT(io),2+8+16);
                        break;

                    default:
                        break;
                    }
                    mgametextpal(margin,yy, opts[ii], enabled?MENUHIGHLIGHT(io):DISABLEDMENUSHADE, 10);
                    io++;
                    yy += 8;
                }
            }

            break;
        }
    case 20004:
    case 20005:
        M_DrawTopBar("Multiplayer Macros");

        if (g_currentMenu == 20004)
        {
            x = probesm(24,45,8,10);
            if (x == -1)
            {
                M_ChangeMenu(20002);
                probey = 7;
            }
            else if (x >= 0 && x <= 9)
            {
                strcpy(buf, ud.ridecule[x]);
                inputloc = strlen(buf);
                last_menu_pos = probey;
                M_ChangeMenu(20005);
                I_AdvanceTriggerClear();
            }
        }
        else
        {
            x = Menu_EnterText(26,40+(8*probey),buf,34,0);
            if (x)
            {
                if (x == 1)
                {
                    Bstrcpy(ud.ridecule[last_menu_pos],buf);
                }
                I_AdvanceTriggerClear();
                M_ChangeMenu(20004);
            }
        }
        for (i=0; i<10; i++)
        {
            if (g_currentMenu == 20005 && i == last_menu_pos) continue;
            mgametextpal(26,40+(i<<3),ud.ridecule[i],MENUHIGHLIGHT(i),0);
        }

        mgametext(160,144,"Activate in-game with SHIFT-F#",0,2+8+16);

        break;

#if 0
    case 20010:
        M_DrawTopBar("Host Network Game");

        x = M_Probe(46,50,80,2);

        if (x == -1)
        {
            ChangeToMenu(20001);
            probey = 2;
        }
        else if (x == 0) ChangeToMenu(20011);

        menutext(40,50,0,0,        "Game Options");
        minitext(90,60,            "Game Type"    ,2,26);
        minitext(90,60+8,          "Episode"      ,2,26);
        minitext(90,60+8+8,        "Level"        ,2,26);
        minitext(90,60+8+8+8,      "Monsters"     ,2,26);
        if (ud.m_coop == 0)
            minitext(90,60+8+8+8+8,    "Markers"      ,2,26);
        else if (ud.m_coop == 1)
            minitext(90,60+8+8+8+8,    "Friendly Fire",2,26);
        minitext(90,60+8+8+8+8+8,  "User Map"     ,2,26);

        mgametext(90+60,60,GametypeNames[ud.m_coop],0,26);

        minitext(90+60,60+8,      EpisodeNames[ud.m_volume_number],0,26);
        minitext(90+60,60+8+8,    level_names[MAXLEVELS*ud.m_volume_number+ud.m_level_number],0,26);
        if (ud.m_monsters_off == 0 || ud.m_player_skill > 0)
            minitext(90+60,60+8+8+8,  SkillNames[ud.m_player_skill],0,26);
        else minitext(90+60,60+8+8+8,  "None",0,28);
        if (ud.m_coop == 0)
        {
            if (ud.m_marker) minitext(90+60,60+8+8+8+8,"On",0,26);
            else minitext(90+60,60+8+8+8+8,"Off",0,26);
        }
        else if (ud.m_coop == 1)
        {
            if (ud.m_ffire) minitext(90+60,60+8+8+8+8,"On",0,26);
            else minitext(90+60,60+8+8+8+8,"Off",0,26);
        }

        menutext(40,50+80,0,0,"Launch Game");
        break;

    case 20011:
        c = (320>>1) - 120;
        M_DrawTopBar("Net Game Options");

        x = M_Probe(c,57-8,16,8);

        switch (x)
        {
        case -1:
            ChangeToMenu(20010);
            break;
        case 0:
            ud.m_coop++;
            if (ud.m_coop == 3) ud.m_coop = 0;
            break;
        case 1:
            if (!VOLUMEONE)
            {
                ud.m_volume_number++;
                if (ud.m_volume_number == g_numVolumes) ud.m_volume_number = 0;
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
            ChangeToMenu(20010);
            break;
        }

        c += 40;

        //         if(ud.m_coop==1) mgametext(c+70,57-7-9,"Cooperative Play",0,2+8+16);
        //         else if(ud.m_coop==2) mgametext(c+70,57-7-9,"DukeMatch (No Spawn)",0,2+8+16);
        //         else mgametext(c+70,57-7-9,"DukeMatch (Spawn)",0,2+8+16);
        mgametext(c+70,57-7-9,GametypeNames[ud.m_coop],0,26);

        mgametext(c+70,57+16-7-9,EpisodeNames[ud.m_volume_number],0,2+8+16);

        mgametext(c+70,57+16+16-7-9,&level_names[MAXLEVELS*ud.m_volume_number+ud.m_level_number][0],0,2+8+16);

        if (ud.m_monsters_off == 0 || ud.m_player_skill > 0)
            mgametext(c+70,57+16+16+16-7-9,SkillNames[ud.m_player_skill],0,2+8+16);
        else mgametext(c+70,57+16+16+16-7-9,"None",0,2+8+16);

        if (ud.m_coop == 0)
        {
            if (ud.m_marker)
                mgametext(c+70,57+16+16+16+16-7-9,"On",0,2+8+16);
            else mgametext(c+70,57+16+16+16+16-7-9,"Off",0,2+8+16);
        }

        if (ud.m_coop == 1)
        {
            if (ud.m_ffire)
                mgametext(c+70,57+16+16+16+16+16-7-9,"On",0,2+8+16);
            else mgametext(c+70,57+16+16+16+16+16-7-9,"Off",0,2+8+16);
        }

        c -= 44;

        menutext(c,57-9,MENUHIGHLIGHT(0),PHX(-2),"Game Type");

        sprintf(tempbuf,"Episode %d",ud.m_volume_number+1);
        menutext(c,57+16-9,MENUHIGHLIGHT(1),PHX(-3),tempbuf);

        sprintf(tempbuf,"Level %d",ud.m_level_number+1);
        menutext(c,57+16+16-9,MENUHIGHLIGHT(2),PHX(-4),tempbuf);

        menutext(c,57+16+16+16-9,MENUHIGHLIGHT(3),PHX(-5),"Monsters");

        if (ud.m_coop == 0)
            menutext(c,57+16+16+16+16-9,MENUHIGHLIGHT(4),PHX(-6),"Markers");
        else
            menutext(c,57+16+16+16+16-9,MENUHIGHLIGHT(4),1,"Markers");

        if (ud.m_coop == 1)
            menutext(c,57+16+16+16+16+16-9,MENUHIGHLIGHT(5),PHX(-6),"Fr. Fire");
        else menutext(c,57+16+16+16+16+16-9,MENUHIGHLIGHT(5),1,"Fr. Fire");

        if (VOLUMEALL)
        {
            menutext(c,57+16+16+16+16+16+16-9,MENUHIGHLIGHT(6),boardfilename[0] == 0,"User Map");
            if (boardfilename[0] != 0)
                mgametext(c+70+44,57+16+16+16+16+16,boardfilename,0,2+8+16);
        }
        else
        {
            menutext(c,57+16+16+16+16+16+16-9,MENUHIGHLIGHT(6),1,"User Map");
        }

        menutext(c,57+16+16+16+16+16+16+16-9,MENUHIGHLIGHT(7),PHX(-8),"Accept");
        break;

    case 20020:
    case 20021: // editing server
    case 20022: // editing port
        M_DrawTopBar("Join Network Game");

        if (g_currentMenu == 20020)
        {
            x = M_Probe(46,50,20,3);

            if (x == -1)
            {
                ChangeToMenu(20001);
                probey = 1;
            }
            else if (x == 0)
            {
                strcpy(buf, "localhost");
                inputloc = strlen(buf);
                M_ChangeMenu(20021);
            }
            else if (x == 1)
            {
                strcpy(buf, "19014");
                inputloc = strlen(buf);
                M_ChangeMenu(20022);
            }
            else if (x == 2)
                {}
            I_AdvanceTriggerClear();
        }
        else if (g_currentMenu == 20021)
        {
            x = Menu_EnterText(40+100,50-9,buf,31,0);
            if (x)
            {
                if (x == 1)
                {
                    //strcpy(szPlayerName,buf);
                }

                I_AdvanceTriggerClear();

                M_ChangeMenu(20020);
            }
        }
        else if (g_currentMenu == 20022)
        {
            x = Menu_EnterText(40+100,50+20-9,buf,5,997);
            if (x)
            {
                if (x == 1)
                {
                    //strcpy(szPlayerName,buf);
                }

                I_AdvanceTriggerClear();

                M_ChangeMenu(20020);
            }
        }

        menutext(40,50,0,0,"Server");
        if (g_currentMenu != 20021) mgametext(40+100,50-9,"server",0,2+8+16);

        menutext(40,50+20,0,0,"Port");
        if (g_currentMenu != 20022)
        {
            sprintf(tempbuf,"%d",19014);
            mgametext(40+100,50+20-9,tempbuf,0,2+8+16);
        }

        menutext(160,50+20+20,0,0,"Connect");


        // ADDRESS
        // PORT
        // CONNECT
        break;
#endif
    case 15001:
    case 15000:

        mgametext(160,90,"LOAD last game:",0,2+8+16);

        Bsprintf(tempbuf,"\"%s\"",ud.savegame[g_lastSaveSlot]);
        mgametext(160,99,tempbuf,0,2+8+16);

        mgametext(160,99+9,"(Y/N)",0,2+8+16);

        x = M_Probe(186,124+9,0,0);

        if (x == -1 || KB_KeyPressed(sc_N))
        {
            if (sprite[g_player[myconnectindex].ps->i].extra <= 0)
            {
                if (G_EnterLevel(MODE_GAME)) G_BackToMenu();
                return;
            }

            KB_ClearKeyDown(sc_N);

            g_player[myconnectindex].ps->gm &= ~MODE_MENU;
            if ((!g_netServer && ud.multimode < 2) && ud.recstat != 2)
            {
                ready2send = 1;
                totalclock = ototalclock;
            }
        }

        if (ProbeTriggers(AdvanceTrigger) || I_AdvanceTrigger() || KB_KeyPressed(sc_Y))
        {
            I_AdvanceTriggerClear();
            ProbeTriggersClear(AdvanceTrigger);
            KB_ClearKeyDown(sc_Y);

            KB_FlushKeyboardQueue();
            KB_ClearKeysDown();
            FX_StopAllSounds();
            S_ClearSoundLocks();

            G_LoadPlayerMaybeMulti(g_lastSaveSlot);
        }

        break;

    case MENU_ADULTMODE:
    case MENU_ADULTPASSWORD:

        margin = 60;
        M_DrawTopBar("Adult Mode");

        x = M_Probe(60,50+16,16,2);
        if (x == -1)
        {
            M_ChangeMenu(MENU_GAMESETUP);
            probey = 0;
            break;
        }

        menutext(margin,50+16,MENUHIGHLIGHT(0),0,"Adult Mode");
        menutext(margin,50+16+16,MENUHIGHLIGHT(1),0,"Enter Password");

        menutext(margin+160+40,50+16,MENUHIGHLIGHT(0),0,ud.lockout?"Off":"On");

        if (g_currentMenu == MENU_ADULTPASSWORD)
        {
            mgametext(160,50+16+16+16+16-12,"Enter Password",0,2+8+16);
            x = Menu_EnterText((320>>1),50+16+16+16+16,buf,19, 998);

            if (x)
            {
                if (ud.pwlockout[0] == 0 || ud.lockout == 0)
                    strcpy(&ud.pwlockout[0],buf);
                else if (strcmp(buf,&ud.pwlockout[0]) == 0)
                {
                    ud.lockout = 0;
                    buf[0] = 0;
#if 0
                    for (x=0; x<g_numAnimWalls; x++)
                        if (wall[animwall[x].wallnum].picnum != W_SCREENBREAK &&
                                wall[animwall[x].wallnum].picnum != W_SCREENBREAK+1 &&
                                wall[animwall[x].wallnum].picnum != W_SCREENBREAK+2)
                            if (wall[animwall[x].wallnum].extra >= 0)
                                wall[animwall[x].wallnum].picnum = wall[animwall[x].wallnum].extra;
#endif
                }
                M_ChangeMenu(MENU_ADULTMODE);
                I_AdvanceTriggerClear();
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
#if 0
                        for (x=0; x<g_numAnimWalls; x++)
                            if (wall[animwall[x].wallnum].picnum != W_SCREENBREAK &&
                                    wall[animwall[x].wallnum].picnum != W_SCREENBREAK+1 &&
                                    wall[animwall[x].wallnum].picnum != W_SCREENBREAK+2)
                                if (wall[animwall[x].wallnum].extra >= 0)
                                    wall[animwall[x].wallnum].picnum = wall[animwall[x].wallnum].extra;
#endif
                    }
                    else
                    {
                        buf[0] = 0;
                        M_ChangeMenu(MENU_ADULTPASSWORD);
                        inputloc = 0;
                        KB_FlushKeyboardQueue();
                    }
                }
                else
                {
                    ud.lockout = 1;

                    for (x=0; x<g_numAnimWalls; x++)
                        switch (DYNAMICTILEMAP(wall[animwall[x].wallnum].picnum))
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
                M_ChangeMenu(MENU_ADULTPASSWORD);
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

        M_DrawBackground();
        M_DrawTopBar("Load Game");
        rotatesprite_fs(101<<16,97<<16,65536>>1,512,TILE_LOADSHOT,-32,0,4+10+64);

        M_DisplaySaveGameList();

        Bsprintf(tempbuf,"Players: %-2d                      ",savehead.numplayers);
        mgametext(160,156,tempbuf,0,2+8+16);

        Bsprintf(tempbuf,"Episode: %-2d / Level: %-2d / Skill: %-2d",
                 1+savehead.volnum, 1+savehead.levnum, savehead.skill);
        mgametext(160,168,tempbuf,0,2+8+16);

        if (savehead.volnum == 0 && savehead.levnum == 7)
            mgametext(160,180,savehead.boardfn,0,2+8+16);

        mgametext(160,90,"LOAD game:",0,2+8+16);
        Bsprintf(tempbuf,"\"%s\"",ud.savegame[g_currentMenu-1000]);
        mgametext(160,99,tempbuf,0,2+8+16);
        mgametext(160,99+9,"(Y/N)",0,2+8+16);

        x = M_Probe(186,124+9,0,0);

        if (ProbeTriggers(AdvanceTrigger) || I_AdvanceTrigger() || KB_KeyPressed(sc_Y))
        {
            I_AdvanceTriggerClear();
            ProbeTriggersClear(AdvanceTrigger);
            KB_ClearKeyDown(sc_Y);

            g_lastSaveSlot = g_currentMenu-1000;

            KB_FlushKeyboardQueue();
            KB_ClearKeysDown();
            if ((!g_netServer && ud.multimode < 2) && ud.recstat != 2)
            {
                ready2send = 1;
                totalclock = ototalclock;
            }

            G_LoadPlayerMaybeMulti(g_lastSaveSlot);

            break;
        }

        if (x == -1 || KB_KeyPressed(sc_N))
        {
            KB_ClearKeyDown(sc_N);
            if (g_player[myconnectindex].ps->gm&MODE_GAME)
            {
                g_player[myconnectindex].ps->gm &= ~MODE_MENU;
                if ((!g_netServer && ud.multimode < 2) && ud.recstat != 2)
                {
                    ready2send = 1;
                    totalclock = ototalclock;
                }
            }
            else
            {
                M_ChangeMenu(MENU_LOAD);
                probey = last_threehundred;
            }
        }

        break;

    case 1500:

        x = M_Probe(186,124,0,0);

        if (ProbeTriggers(AdvanceTrigger) || I_AdvanceTrigger() || KB_KeyPressed(sc_Y))
        {
            I_AdvanceTriggerClear();
            ProbeTriggersClear(AdvanceTrigger);
            KB_ClearKeyDown(sc_Y);
            KB_FlushKeyboardQueue();
            M_ChangeMenu(MENU_EPISODE);
        }
        if (x == -1 || KB_KeyPressed(sc_N))
        {
            KB_ClearKeyDown(sc_N);
            if ((!g_netServer && ud.multimode < 2) && ud.recstat != 2)
            {
                ready2send = 1;
                totalclock = ototalclock;
            }
            g_player[myconnectindex].ps->gm &= ~MODE_MENU;
            break;
        }
        mgametext(160,90,"ABORT this game?",0,2+8+16);
        mgametext(160,90+9,"(Y/N)",0,2+8+16);

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

        M_DrawBackground();
        M_DrawTopBar("Save Game");

        rotatesprite_fs(101<<16,97<<16,65536L>>1,512,TILE_LOADSHOT,-32,0,4+10+64);
        Bsprintf(tempbuf,"Players: %-2d                      ",ud.multimode);
        mgametext(160,156,tempbuf,0,2+8+16);

        Bsprintf(tempbuf,"Episode: %-2d / Level: %-2d / Skill: %-2d",1+ud.volume_number,1+ud.level_number,ud.player_skill);
        mgametext(160,168,tempbuf,0,2+8+16);

        if (ud.volume_number == 0 && ud.level_number == 7)
            mgametext(160,180,boardfilename,0,2+8+16);

        M_DisplaySaveGameList();

        mgametext(160,90,"OVERWRITE previous SAVED game?",0,2+8+16);
        mgametext(160,90+9,"(Y/N)",0,2+8+16);

        x = M_Probe(186,124,0,0);

        if (ProbeTriggers(AdvanceTrigger) || I_AdvanceTrigger() || KB_KeyPressed(sc_Y))
        {
            I_AdvanceTriggerClear();
            ProbeTriggersClear(AdvanceTrigger);
            KB_ClearKeyDown(sc_Y);

            inputloc = strlen(&ud.savegame[g_currentMenu-2000][0]);

            M_ChangeMenu(g_currentMenu-2000+360);

            KB_FlushKeyboardQueue();
            break;
        }
        if (x == -1 || KB_KeyPressed(sc_N))
        {
            KB_ClearKeyDown(sc_N);
            M_ChangeMenu(351);
        }

        break;

    case MENU_CREDITS:
    case MENU_CREDITS2:
    case MENU_CREDITS3:
    case MENU_CREDITS4:
    case MENU_CREDITS5:
    case MENU_CREDITS6:
    case MENU_CREDITS7:
    case MENU_CREDITS8:
    case MENU_CREDITS9:
    case MENU_CREDITS10:
        margin = MENU_MARGIN_CENTER;
        if (!VOLUMEALL || !PLUTOPAK)
        {
            //M_DrawBackground();
            switch (g_currentMenu)
            {
            case MENU_CREDITS9:
            case MENU_CREDITS10:
                M_DrawTopBar("About EDuke32");
                break;
            default:
                M_DrawTopBar("Credits");
                break;
            }

            l = 9;
        }
        else
        {
            l = 4;
        }

        M_LinearPanels(MENU_CREDITS, MENU_CREDITS+l);

        x = M_Probe(0,0,0,1);

        if (x == -1)
        {
            M_ChangeMenu(MENU_MAIN);
            break;
        }

        if (!VOLUMEALL || !PLUTOPAK)
        {
            switch (g_currentMenu)
            {
            case MENU_CREDITS:
                mgametext(margin,40,                      "Original Concept",0,2+8+16);
                mgametext(margin,40+9,                    "Todd Replogle",0,2+8+16);
                mgametext(margin,40+9+9,                  "Allen H. Blum III",0,2+8+16);

                mgametext(margin,40+9+9+9+9,              "Produced & Directed By",0,2+8+16);
                mgametext(margin,40+9+9+9+9+9,            "Greg Malone",0,2+8+16);

                mgametext(margin,40+9+9+9+9+9+9+9,        "Executive Producer",0,2+8+16);
                mgametext(margin,40+9+9+9+9+9+9+9+9,      "George Broussard",0,2+8+16);

                mgametext(margin,40+9+9+9+9+9+9+9+9+9+9,  "BUILD Engine",0,2+8+16);
                mgametext(margin,40+9+9+9+9+9+9+9+9+9+9+9,"Ken Silverman",0,2+8+16);
                break;
            case MENU_CREDITS2:
                mgametext(margin,40,                      "Game Programming",0,2+8+16);
                mgametext(margin,40+9,                    "Todd Replogle",0,2+8+16);

                mgametext(margin,40+9+9+9,                "3D Engine/Tools/Net",0,2+8+16);
                mgametext(margin,40+9+9+9+9,              "Ken Silverman",0,2+8+16);

                mgametext(margin,40+9+9+9+9+9+9,          "Network Layer/Setup Program",0,2+8+16);
                mgametext(margin,40+9+9+9+9+9+9+9,        "Mark Dochtermann",0,2+8+16);
                break;
            case MENU_CREDITS3:
                mgametext(margin,40,                      "Map Design",0,2+8+16);
                mgametext(margin,40+9,                    "Allen H. Blum III",0,2+8+16);
                mgametext(margin,40+9+9,                  "Richard Gray",0,2+8+16);

                mgametext(margin,40+9+9+9+9,              "3D Modeling",0,2+8+16);
                mgametext(margin,40+9+9+9+9+9,            "Chuck Jones",0,2+8+16);
                mgametext(margin,40+9+9+9+9+9+9,          "Sapphire Corporation",0,2+8+16);

                mgametext(margin,40+9+9+9+9+9+9+9+9,      "Artwork",0,2+8+16);
                mgametext(margin,40+9+9+9+9+9+9+9+9+9,    "Dirk Jones, Stephen Hornback",0,2+8+16);
                mgametext(margin,40+9+9+9+9+9+9+9+9+9+9,  "James Storey, David Demaret",0,2+8+16);
                mgametext(margin,40+9+9+9+9+9+9+9+9+9+9+9,"Douglas R. Wood",0,2+8+16);
                break;
            case MENU_CREDITS4:
                mgametext(margin,40,                      "Sound Engine",0,2+8+16);
                mgametext(margin,40+9,                    "Jim Dose",0,2+8+16);

                mgametext(margin,40+9+9+9,                "Sound & Music Development",0,2+8+16);
                mgametext(margin,40+9+9+9+9,              "Robert Prince",0,2+8+16);
                mgametext(margin,40+9+9+9+9+9,            "Lee Jackson",0,2+8+16);

                mgametext(margin,40+9+9+9+9+9+9+9,        "Voice Talent",0,2+8+16);
                mgametext(margin,40+9+9+9+9+9+9+9+9,      "Lani Minella - Voice Producer",0,2+8+16);
                mgametext(margin,40+9+9+9+9+9+9+9+9+9,    "Jon St. John as \"Duke Nukem\"",0,2+8+16);
                break;
            case MENU_CREDITS5:
                mgametext(margin,60,                      "Graphic Design",0,2+8+16);
                mgametext(margin,60+9,                    "Packaging, Manual, Ads",0,2+8+16);
                mgametext(margin,60+9+9,                  "Robert M. Atkins",0,2+8+16);
                mgametext(margin,60+9+9+9,                "Michael Hadwin",0,2+8+16);

                mgametext(margin,60+9+9+9+9+9,            "Special Thanks To",0,2+8+16);
                mgametext(margin,60+9+9+9+9+9+9,          "Steven Blackburn, Tom Hall",0,2+8+16);
                mgametext(margin,60+9+9+9+9+9+9+9,        "Scott Miller, Joe Siegler",0,2+8+16);
                mgametext(margin,60+9+9+9+9+9+9+9+9,      "Terry Nagy, Colleen Compton",0,2+8+16);
                mgametext(margin,60+9+9+9+9+9+9+9+9+9,    "HASH, Inc., FormGen, Inc.",0,2+8+16);
                break;
            case MENU_CREDITS6:
                mgametext(margin,49,                      "The 3D Realms Beta Testers",0,2+8+16);

                mgametext(margin,49+9+9,                  "Nathan Anderson, Wayne Benner",0,2+8+16);
                mgametext(margin,49+9+9+9,                "Glenn Brensinger, Rob Brown",0,2+8+16);
                mgametext(margin,49+9+9+9+9,              "Erik Harris, Ken Heckbert",0,2+8+16);
                mgametext(margin,49+9+9+9+9+9,            "Terry Herrin, Greg Hively",0,2+8+16);
                mgametext(margin,49+9+9+9+9+9+9,          "Hank Leukart, Eric Baker",0,2+8+16);
                mgametext(margin,49+9+9+9+9+9+9+9,        "Jeff Rausch, Kelly Rogers",0,2+8+16);
                mgametext(margin,49+9+9+9+9+9+9+9+9,      "Mike Duncan, Doug Howell",0,2+8+16);
                mgametext(margin,49+9+9+9+9+9+9+9+9+9,    "Bill Blair",0,2+8+16);
                break;
            case MENU_CREDITS7:
                mgametext(margin,32,                      "Company Product Support",0,2+8+16);

                mgametext(margin,32+9+9,                  "The following companies were cool",0,2+8+16);
                mgametext(margin,32+9+9+9,                "enough to give us lots of stuff",0,2+8+16);
                mgametext(margin,32+9+9+9+9,              "during the making of Duke Nukem 3D.",0,2+8+16);

                mgametext(margin,32+9+9+9+9+9+9,          "Altec Lansing Multimedia",0,2+8+16);
                mgametext(margin,32+9+9+9+9+9+9+9,        "for tons of speakers and the",0,2+8+16);
                mgametext(margin,32+9+9+9+9+9+9+9+9,      "THX-licensed sound system.",0,2+8+16);
                mgametext(margin,32+9+9+9+9+9+9+9+9+9,    "For info call 1-800-548-0620",0,2+8+16);

                mgametext(margin,32+9+9+9+9+9+9+9+9+9+9+9,"Creative Labs, Inc.",0,2+8+16);

                mgametext(margin,32+9+9+9+9+9+9+9+9+9+9+9+9+9,"Thanks for the hardware, guys.",0,2+8+16);
                break;
            case MENU_CREDITS8:
                mgametext(margin,50,                      "Duke Nukem is a trademark of",0,2+8+16);
                mgametext(margin,50+9,                    "3D Realms Entertainment",0,2+8+16);

                mgametext(margin,50+9+9+9,                "Duke Nukem",0,2+8+16);
                mgametext(margin,50+9+9+9+9,              "(C) 1996 3D Realms Entertainment",0,2+8+16);

                if (VOLUMEONE)
                {
                    mgametext(margin,106,                     "Please read LICENSE.DOC for shareware",0,2+8+16);
                    mgametext(margin,106+9,                   "distribution grants and restrictions.",0,2+8+16);
                }

                mgametext(margin,VOLUMEONE?134:115,       "Made in Dallas, Texas USA",0,2+8+16);
                break;
            case MENU_CREDITS9:
                l = 10;
                goto cheat_for_port_credits;
            case MENU_CREDITS10:
                l = 10;
                goto cheat_for_port_credits2;
            }
            break;
        }

        // Plutonium pak menus
        switch (g_currentMenu)
        {
        case MENU_CREDITS:
        case MENU_CREDITS2:
        case MENU_CREDITS3:
            rotatesprite_fs(160<<16,200<<15,65536L,0,2504+g_currentMenu-MENU_CREDITS,0,0,10+64);
            break;
        case MENU_CREDITS4:   // JBF 20031220
            M_DrawBackground();
            M_DrawTopBar("About EDuke32");

cheat_for_port_credits:
            if (g_scriptVersion != 14) l = (-2);
            mgametext(160,38-l,"Programming and Project Management",0,2+8+16);
            p = "Richard \"TerminX\" Gobeille";
            creditsminitext(160, 38+10-l, p, 8, 10+16+128);

            mgametext(160,58-l,"Polymer Rendering System",0,2+8+16);
            p = "Pierre-Loup \"Plagman\" Griffais";
            creditsminitext(160, 58+10-l, p, 8, 10+16+128);

            mgametext(160,78-l,"Additional Engine and Game Programming",0,2+8+16);
            p = "Philipp \"Helixhorned\" Kutin";
            creditsminitext(160, 78+10-l, p, 8, 10+16+128);

            mgametext(160,98-l,"\"JFDuke3D\" and \"JFBuild\" code",0,2+8+16);
            p = "Jonathon \"JonoF\" Fowler";
            creditsminitext(160, 98+10-l, p, 8, 10+16+128);

            mgametext(160,118-l,"Legacy \"NAM\", \"WWII GI\", and \"EDuke\" Code",0,2+8+16);
            p = "Matt \"Matteus\" Saettler";
            creditsminitext(160, 118+10-l, p, 8, 10+16+128);

            mgametext(160,138-l,"Core BUILD Engine functionality",0,2+8+16);
            p = "Ken \"Awesoken\" Silverman";
            creditsminitext(160, 138+10-l, p, 8, 10+16+128);

            p = "Visit www.eduke32.com for news and updates";
            creditsminitext(160, 138+10+10+10+10+4-l, p, 8, 10+16+128);
            break;

        case MENU_CREDITS5:
            M_DrawBackground();
            M_DrawTopBar("About EDuke32");

cheat_for_port_credits2:
            if (g_scriptVersion != 14) l = (-2);
            mgametext(160,38-l,"License and Other Contributors",0,2+8+16);
            {
                const char *header[] =
                {
                    "This program is distributed under the terms of the",
                    "GNU General Public License version 2 as published by the",
                    "Free Software Foundation. See GNU.TXT for details.",
                    " ",
                    "Thanks to the following people for their contributions:",
                    " ",
                };
                const char *body[] =
                {
                    "Adam Fazakerley",   // netcode NAT traversal
                    "Alan Ondra",        // testing
                    "Bioman",            // GTK work, APT repository and package upkeep
                    "Brandon Bergren",   // "Bdragon" - tiles.cfg
                    "Charlie Honig",     // "CONAN" - showview command
                    "Dan Gaskill",       // "DeeperThought" - testing
                    "David Koenig",      // "Bargle" - Merged a couple of things from duke3d_w32
                    "Ed Coolidge",       // Mapster32 improvements
                    "Evan Ramos",        // "Hendricks266" - misc stuff
                    "Ferry Landzaat",    // ? (listed on the wiki page)
                    "Hunter_rus",        // tons of stuff
                    "James Bentler",     // Mapster32 improvements
                    "Jasper Foreman",    // netcode contributions
                    "Javier Martinez",   // "Malone3D" - EDuke 2.1.1 components
                    "Jeff Hart",         // website graphics
                    "Jonathan Smith",    // "Mblackwell" - testing
                    "Jose del Castillo", // "Renegado" - EDuke 2.1.1 components
                    "Lachlan McDonald",  // official EDuke32 icon
                    "LSDNinja",          // OS X help and testing
                    "Marcus Herbert",    // "rhoenie" - OS X compatibility work
                    "Matthew Palmer",    // "Usurper" - testing and eduke32.com domain
                    "Ozkan Sezer",       // SDL/GTK version checking improvements
                    "Peter Green",       // "Plugwash" - dynamic remapping, custom gametypes
                    "Peter Veenstra",    // "Qbix" - port to 64-bit
                    "Randy Heit",        // random snippets of ZDoom here and there
                    "Robin Green",       // CON array support
                    "Ryan Gordon",       // "icculus" - icculus.org Duke3D port sound code
                    "Stephen Anthony",   // early 64-bit porting work
                    "tueidj",            // Wii port
                    " ",
                };
                const char *footer[] =
                {
                    " ",
                    "BUILD engine technology available under BUILDLIC.",
                };

                const int32_t header_numlines = sizeof(header)/sizeof(char *);
                const int32_t body_numlines = sizeof(body)/sizeof(char *);
                const int32_t footer_numlines = sizeof(footer)/sizeof(char *);

                i = 0;
                for (m=0; m<header_numlines; m++)
                    creditsminitext(160, 17+10+10+8+4+(m*7)-l, header[m], 8, 10+16+128);
                i += m;
#define CCOLUMNS 3
#define CCOLXBUF 20
                for (m=0; m<body_numlines; m++)
                    creditsminitext(CCOLXBUF+((320-CCOLXBUF*2)/(CCOLUMNS*2))  +((320-CCOLXBUF*2)/CCOLUMNS)*(m/(body_numlines/CCOLUMNS)), 17+10+10+8+4+((m%(body_numlines/CCOLUMNS))*7)+(i*7)-l, body[m], 8, 10+16+128);
                i += m/CCOLUMNS;
                for (m=0; m<footer_numlines; m++)
                    creditsminitext(160, 17+10+10+8+4+(m*7)+(i*7)-l, footer[m], 8, 10+16+128);
            }

            p = "Visit www.eduke32.com for news and updates";
            creditsminitext(160, 138+10+10+10+10+4-l, p, 8, 10+16+128);

            break;
        }
        break;

    case MENU_MAIN:
        Menu_Main();
        break;

    case MENU_MAIN_INGAME:
        margin = MENU_MARGIN_CENTER;
        rotatesprite_fs(margin<<16,32<<16,65536L,0,INGAMEDUKETHREEDEE,0,0,10);
        if (PLUTOPAK)   // JBF 20030804
            rotatesprite_fs((margin+100)<<16,36<<16,65536L,0,PLUTOPAKSPRITE+2,(sintable[(totalclock<<4)&2047]>>11),0,2+8);
        x = M_Probe(margin,67,16,7);
        switch (x)
        {
        case 0:
            if ((!g_netServer && ud.multimode < 2) || ud.recstat == 2)
                M_ChangeMenu(1500);
            else
            {
                M_ChangeMenu(600);
                last_fifty = 0;
            }
            break;
        case 1:
            if (ud.recstat != 2)
            {
                last_fifty = 1;
                M_ChangeMenu(350);
                setview(0,0,xdim-1,ydim-1);
            }
            break;
        case 2:
            last_fifty = 2;
            M_ChangeMenu(MENU_LOAD);
            break;
        case 3:
            last_fifty = 3;
            M_ChangeMenu(MENU_OPTIONS);     // JBF 20031205: was 200
            break;
        case 4:
            last_fifty = 4;
            KB_FlushKeyboardQueue();
            M_ChangeMenu(MENU_STORY);
            break;
        case 5:
            if (numplayers < 2 && !g_netServer)
            {
                last_fifty = 5;
                M_ChangeMenu(MENU_QUITTOTITLE);
            }
            break;
        case 6:
            last_fifty = 6;
            M_ChangeMenu(MENU_QUIT);
            break;
        case -1:
            g_player[myconnectindex].ps->gm &= ~MODE_MENU;
            if ((!g_netServer && ud.multimode < 2) && ud.recstat != 2)
            {
                ready2send = 1;
                totalclock = ototalclock;
            }
            break;
        }

        if (KB_KeyPressed(sc_Q))
            M_ChangeMenu(MENU_QUIT);

        menutext(margin,67                  ,MENUHIGHLIGHT(0),0,"New Game");
        menutext(margin,67+16               ,MENUHIGHLIGHT(1),0,"Save Game");
        menutext(margin,67+16+16            ,MENUHIGHLIGHT(2),0,"Load Game");

        menutext(margin,67+16+16+16         ,MENUHIGHLIGHT(3),0,"Options");
        if (!VOLUMEALL)
        {
            menutext(margin,67+16+16+16+16      ,MENUHIGHLIGHT(4),0,"How To Order");
        }
        else
        {
            menutext(margin,67+16+16+16+16      ,MENUHIGHLIGHT(4),0,"Help");
        }
        if (g_netServer || numplayers > 1)
            menutext(margin,67+16+16+16+16+16   ,MENUHIGHLIGHT(5),1,"Quit To Title");
        else menutext(margin,67+16+16+16+16+16   ,MENUHIGHLIGHT(5),0,"Quit To Title");
        menutext(margin,67+16+16+16+16+16+16,MENUHIGHLIGHT(6),0,"Quit Game");
        break;

    case MENU_EPISODE:
        M_DrawTopBar("Select An Episode");
        x = M_Probe(160,VOLUMEONE?60:60-(g_numVolumes*2),20,VOLUMEONE?3:g_numVolumes+1);
        if (x >= 0)
        {
            if (VOLUMEONE)
            {
                if (x > 0)
                    M_ChangeMenu(MENU_BUYDUKE);
                else
                {
                    ud.m_volume_number = x;
                    ud.m_level_number = 0;
                    last_onehundred = x;
                    M_ChangeMenu(MENU_SKILL);
                }
            }

            if (!VOLUMEONE)
            {
                if (x == g_numVolumes /*&& boardfilename[0]*/)
                {
                    //ud.m_volume_number = 0;
                    //ud.m_level_number = 7;
                    currentlist = 1;
                    last_onehundred = x;
                    M_ChangeMenu(MENU_USERMAP);
                }
                else
                {
                    ud.m_volume_number = x;
                    ud.m_level_number = 0;
                    last_onehundred = x;
                    M_ChangeMenu(MENU_SKILL);
                }
            }
        }
        else if (x == -1)
        {
            if (g_player[myconnectindex].ps->gm&MODE_GAME) M_ChangeMenu(MENU_MAIN_INGAME);
            else M_ChangeMenu(MENU_MAIN);
        }

        margin = 80;
        if (VOLUMEONE)
        {
            menutext(160,60,MENUHIGHLIGHT(0),0,EpisodeNames[0]);
            menutext(160,60+20,MENUHIGHLIGHT(1),1,EpisodeNames[1]);
            menutext(160,60+20+20,MENUHIGHLIGHT(2),1,EpisodeNames[2]);
            if (PLUTOPAK)
                menutext(160,60+20+20,MENUHIGHLIGHT(3),1,EpisodeNames[3]);
        }
        else
        {
            for (i=0; i<g_numVolumes; i++)
                menutext(160,60-(g_numVolumes*2)+(20*i),MENUHIGHLIGHT(i),0,EpisodeNames[i]);

            menutext(160,60-(g_numVolumes*2)+(20*i),MENUHIGHLIGHT(i),0,"User Map");
        }
        break;

    case MENU_USERMAP:
        if (boardfilename[0] == 0) strcpy(boardfilename, "./");
        Bcorrectfilename(boardfilename,1);

        fnlist_getnames(&fnlist, boardfilename, "*.map", 0, 0);
        set_findhighs();

        M_ChangeMenu(MENU_SELECTMAP);
        KB_FlushKeyboardQueue();
    case MENU_SELECTMAP:
        M_DrawTopBar("Select A User Map");

        {
            int32_t width = 160 - (40-4);

            // black translucent background underneath file lists
            rotatesprite(0<<16, 0<<16, 65536<<5, 0, /*tile*/ 0, numshades, 0, 10+16+1+32,
                         xdim/2-scale(width,(ydim*4)/3,320),scale(12+32-2,ydim,200),
                         xdim/2+scale(width,(ydim*4)/3,320)-1,scale(12+32+112+4,ydim,200)-1);
        }

        // path
        minitext(38,45,boardfilename,16,26);

        {
            // JBF 20040208: seek to first name matching pressed character
            CACHE1D_FIND_REC *seeker = currentlist ? fnlist.findfiles : fnlist.finddirs;
            if ((KB_KeyPressed(sc_Home)|KB_KeyPressed(sc_End)) > 0)
            {
                while (seeker && (KB_KeyPressed(sc_End)?seeker->next:seeker->prev))
                    seeker = KB_KeyPressed(sc_End)?seeker->next:seeker->prev;
                if (seeker)
                {
                    if (currentlist) findfileshigh = seeker;
                    else finddirshigh = seeker;
                    // clear keys, don't play the kick sound a dozen times!
                    KB_ClearKeyDown(sc_End);
                    KB_ClearKeyDown(sc_Home);
                    S_PlaySound(KICK_HIT);
                }
            }
            else if ((KB_KeyPressed(sc_PgUp)|KB_KeyPressed(sc_PgDn)) > 0)
            {
                seeker = currentlist?findfileshigh:finddirshigh;
                i = 6;
                while (i>0)
                {
                    if (seeker && (KB_KeyPressed(sc_PgDn)?seeker->next:seeker->prev))
                        seeker = KB_KeyPressed(sc_PgDn)?seeker->next:seeker->prev;
                    i--;
                }
                if (seeker)
                {
                    if (currentlist) findfileshigh = seeker;
                    else finddirshigh = seeker;
                    // clear keys, don't play the kick sound a dozen times!
                    KB_ClearKeyDown(sc_PgDn);
                    KB_ClearKeyDown(sc_PgUp);
                    S_PlaySound(KICK_HIT);
                }
            }
            else
            {
                char ch2, ch;
                ch = KB_GetCh();
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
                        S_PlaySound(KICK_HIT);
                    }
                }
            }
        }
        mgametext(40+4,32,"Directories",0,2+8+16);

        if (finddirshigh)
        {
            int32_t len;

            dir = finddirshigh;
            for (i=0; i<5; i++) if (!dir->prev) break;
                else dir=dir->prev;
            for (i=5; i>-8 && dir; i--, dir=dir->next)
            {
                if (dir == finddirshigh && currentlist == 0) margin=0;
                else margin=16;
                len = Bstrlen(dir->name);
                Bstrncpy(tempbuf,dir->name,len);
                if (len > USERMAPENTRYLENGTH)
                {
                    len = USERMAPENTRYLENGTH-3;
                    tempbuf[len] = 0;
                    while (len < USERMAPENTRYLENGTH)
                        tempbuf[len++] = '.';
                }
                tempbuf[len] = 0;
                minitextshade(40,1+12+32+8*(6-i),tempbuf,margin,0,26);
            }
        }

        mgametext(180+4,32,"Map Files",0,2+8+16);

        if (findfileshigh)
        {
            int32_t len;

            dir = findfileshigh;
            for (i=0; i<6; i++) if (!dir->prev) break;
                else dir=dir->prev;
            for (i=6; i>-8 && dir; i--, dir=dir->next)
            {
                if (dir == findfileshigh && currentlist == 1) margin=0;
                else margin=16;
                len = Bstrlen(dir->name);
                Bstrncpy(tempbuf,dir->name,len);
                if (len > USERMAPENTRYLENGTH)
                {
                    len = USERMAPENTRYLENGTH-3;
                    tempbuf[len] = 0;
                    while (len < USERMAPENTRYLENGTH)
                        tempbuf[len++] = '.';
                }
                tempbuf[len] = 0;
                minitextshade(180,1+12+32+8*(6-i),tempbuf,margin,
                              dir->source==CACHE1D_SOURCE_ZIP ? 8 : 2,
                              26);
            }
        }

        if (KB_KeyPressed(sc_LeftArrow) || KB_KeyPressed(sc_kpad_4) || (LMB && minfo.dyaw < -256) ||
                KB_KeyPressed(sc_RightArrow) || KB_KeyPressed(sc_kpad_6) || (LMB && minfo.dyaw > 256) ||
                KB_KeyPressed(sc_Tab))
        {
            KB_ClearKeyDown(sc_LeftArrow);
            KB_ClearKeyDown(sc_kpad_4);
            KB_ClearKeyDown(sc_RightArrow);
            KB_ClearKeyDown(sc_kpad_6);
            KB_ClearKeyDown(sc_Tab);
            currentlist = 1-currentlist;
            S_PlaySound(KICK_HIT);
        }

        onbar = 0;
        probey = 2;
        if (currentlist == 0) x = probesm(45,32+4+1,0,3);
        else x = probesm(185,32+4+1,0,3);

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
            fnlist_clearnames(&fnlist);

            boardfilename[0] = 0;
            if ((g_netServer || ud.multimode > 1))
            {
                Net_SendUserMapName();
                M_ChangeMenu(600);
                probey = last_menu_pos;
            }
            else M_ChangeMenu(MENU_EPISODE);
        }
        else if (x >= 0)
        {
            if (currentlist == 0)
            {
                if (!finddirshigh) break;
                strcat(boardfilename, finddirshigh->name);
                strcat(boardfilename, "/");
                Bcorrectfilename(boardfilename, 1);
                M_ChangeMenu(MENU_USERMAP);
                KB_FlushKeyboardQueue();
            }
            else
            {
                if (!findfileshigh) break;
                strcat(boardfilename, findfileshigh->name);
                ud.m_volume_number = 0;
                ud.m_level_number = 7;
                if ((g_netServer || ud.multimode > 1))
                {
                    Net_SendUserMapName();
                    M_ChangeMenu(600);
                    probey = last_menu_pos;
                }
                else M_ChangeMenu(MENU_SKILL);
            }

            fnlist_clearnames(&fnlist);
        }
        break;

    case MENU_SKILL:
    {
        // 4 skills (orig) --> 70
        const int32_t ybase = 70 + (4-g_numSkills)*6;

        margin = MENU_MARGIN_CENTER;
        M_DrawTopBar("Select Skill");
        x = M_Probe(margin,ybase,19,g_numSkills);
        if (x >= 0)
        {
            switch (x)
            {
            case 0:
                g_skillSoundID = JIBBED_ACTOR6;
                break;
            case 1:
                g_skillSoundID = BONUS_SPEECH1;
                break;
            case 2:
                g_skillSoundID = DUKE_GETWEAPON2;
                break;
            case 3:
                g_skillSoundID = JIBBED_ACTOR5;
                break;
            }

            S_PlaySound(g_skillSoundID);

            ud.m_player_skill = x+1;
            if (x == 3) ud.m_respawn_monsters = 1;
            else ud.m_respawn_monsters = 0;

            ud.m_monsters_off = ud.monsters_off = 0;

            ud.m_respawn_items = 0;
            ud.m_respawn_inventory = 0;

            ud.multimode = 1;

            if (ud.m_volume_number == 3 && (G_GetLogoFlags() & LOGO_NOE4CUTSCENE)==0)
            {
                flushperms();
                setview(0,0,xdim-1,ydim-1);
                clearview(0L);
                nextpage();
            }

            G_NewGame(ud.m_volume_number,ud.m_level_number,ud.m_player_skill);
            if (G_EnterLevel(MODE_GAME)) G_BackToMenu();
        }
        else if (x == -1)
        {
            M_ChangeMenu(MENU_EPISODE);
            KB_FlushKeyboardQueue();
        }

        for (i=0; i<g_numSkills; i++)
            menutext(margin,ybase+i*19,MENUHIGHLIGHT(i),0,SkillNames[i]);
        break;
    }

    case 230:
        M_DrawTopBar("Renderer Setup");

        margin = MENU_MARGIN_REGULAR;

        {
            int32_t io, ii, yy, d=margin+160+40, enabled;
            static const char *const opts[] =
            {
                "Aspect ratio",
                "Ambient light level",
#ifdef USE_OPENGL
                "Anisotropic filtering",
                "Use VSync",
                "-",
                "Enable hires textures",
                "Hires texture quality",
                "Pre-load map textures",
                "On disk texture cache",
                "Use detail textures",
                "-",
                "Use models",
#endif
                NULL
            };

            yy = 37;
            for (ii=io=0; opts[ii]; ii++)
            {
                if (getrendermode() == REND_CLASSIC && io >= 2)
                    break;

                if (opts[ii][0] == '-' && !opts[ii][1])
                {
                    if (io <= probey) yy += 4;
                    continue;
                }
                if (io < probey) yy += 8;
                io++;
            }

            onbar = (probey==1||probey==5);
            x = probesm(margin,yy+5,0,io);

            if (x == -1)
            {
                M_ChangeMenu(MENU_VIDEOSETUP);
                probey = 6;
                break;
            }

            yy = 37;
            for (ii=io=0; opts[ii]; ii++)
            {
                if (getrendermode() == REND_CLASSIC && io >= 2)
                    break;

                if (opts[ii][0] == '-' && !opts[ii][1])
                {
                    yy += 4;
                    continue;
                }

                enabled = 1;
                switch (io)
                {
                case 0:
                    if (getrendermode() <= REND_POLYMOST)
                    {
#ifdef USE_OPENGL
                        int32_t tmp = r_usenewaspect ? 2 : glwidescreen;

                        if (x==io)
                        {
                            tmp = tmp+1;
                            if (tmp > 2)
                                tmp = 0;
                        }
                        modval(0,2,&tmp,1,probey==io);

                        r_usenewaspect = (tmp==2);

                        // with r_usenewaspect, glwidescreen has no effect...
                        glwidescreen = (tmp < 2) ? tmp : 0;
                        mgametextpal(d,yy, r_usenewaspect ? "Auto" :
                                     (glwidescreen ? "Old wide" : "Old reg."),
                                     MENUHIGHLIGHT(io), 0);
#else
                        if (x==io)
                            r_usenewaspect = !r_usenewaspect;
                        modval(0,1,&r_usenewaspect,1,probey==io);
                        mgametextpal(d,yy, r_usenewaspect ? "Auto" : "Old reg.",
                                     MENUHIGHLIGHT(io), 0);
#endif
                    }
#ifdef POLYMER
                    else
                    {
                        double ratios[] = { 0.0, 1.33, 1.66, 1.78, 1.85, 2.35 };

                        int32_t j = (sizeof(ratios)/sizeof(ratios[0]));

                        for (i = 0; i<j; i++)
                            if (ratios[i] == pr_customaspect)
                                break;

                        modval(0,j-1,(int32_t *)&i,1,probey==io);
                        if (x == io)
                        {
                            i++;
                            if (i >= j)
                                i = 0;
                        }
                        if (i == j)
                            Bsprintf(tempbuf,"Custom");
                        else
                        {
                            if (i == 0) Bsprintf(tempbuf,"Auto");
                            else Bsprintf(tempbuf,"%.2f:1",ratios[i]);

                            if (ratios[i] != pr_customaspect)
                                pr_customaspect = ratios[i];
                        }
                        mgametextpal(d,yy,tempbuf, MENUHIGHLIGHT(io), 0);

                    }
#endif
                    break;
                case 1:
                {
                    int32_t i = (int32_t)(r_ambientlight*1024.f);
                    int32_t j = i;

                    sliderbar(1,d+8,yy+7, &i,128,x==io,MENUHIGHLIGHT(io),0,128,4096);
                    Bsprintf(tempbuf,"%.2f",r_ambientlight);
                    mgametextpal(d-35,yy, tempbuf, MENUHIGHLIGHT(io), 0);
                    if (i != j)
                    {
                        r_ambientlight = (float)i/1024.f;
                        r_ambientlightrecip = 1.f/r_ambientlight;
                    }
                    break;
                }
#ifdef USE_OPENGL
                case 2:
                {
                    int32_t dummy = glanisotropy;
                    modval(0,(int32_t)glinfo.maxanisotropy+1,(int32_t *)&dummy,1,probey==io);
                    if (dummy > glanisotropy) glanisotropy *= 2;
                    else if (dummy < glanisotropy) glanisotropy /= 2;
                    if (x==io)
                        glanisotropy *= 2;
                    if (glanisotropy > glinfo.maxanisotropy) glanisotropy = 1;
                    else if (glanisotropy < 1) glanisotropy = (int32_t)glinfo.maxanisotropy;
                    gltexapplyprops();
                    if (glanisotropy == 1) strcpy(tempbuf,"None");
                    else Bsprintf(tempbuf,"%dx",glanisotropy);
                    mgametextpal(d,yy, tempbuf, MENUHIGHLIGHT(io), 0);
                    break;
                }
                case 3:
                {
                    int32_t ovsync = vsync;
                    if (x==io) vsync++;
                    if (vsync == 2) vsync = -1;
                    modval(-1,1,(int32_t *)&vsync,1,probey==io);
                    mgametextpal(d,yy, vsync < 0 ? "Adaptive" : vsync ? "Yes" : "No", MENUHIGHLIGHT(io), 0);
                    if (vsync != ovsync)
                        setvsync(vsync);
                    break;
                }
                case 4:
                    if (x==io) usehightile = 1-usehightile;
                    modval(0,1,(int32_t *)&usehightile,1,probey==io);
                    mgametextpal(d,yy, usehightile ? "Yes" : "No", MENUHIGHLIGHT(io), 0);
                    break;
                case 5:
                {
                    i = r_downsize;
                    enabled = usehightile;
                    sliderbar(1,d+8,yy+7, &r_downsize,-1,enabled && x==io,MENUHIGHLIGHT(io),!enabled,0,2);
                    if (r_downsize != i)
                    {
                        texcache_invalidate();
                        resetvideomode();
                        if (setgamemode(fullscreen,xdim,ydim,bpp))
                            OSD_Printf("restartvid: Reset failed...\n");
                        r_downsizevar = r_downsize;
                        return;
                    }
                    break;
                }
                case 6:
                    enabled = usehightile;
                    if (enabled && x==io) ud.config.useprecache = !ud.config.useprecache;
                    if (enabled) modval(0,1,(int32_t *)&ud.config.useprecache,1,probey==io);
                    mgametextpal(d,yy, ud.config.useprecache ? "On" : "Off", enabled?MENUHIGHLIGHT(io):DISABLEDMENUSHADE, 0);
                    break;
                case 7:
                {
                    const char *s[] = { "Off", "On", "Compress" };
                    enabled = (glusetexcompr && usehightile);
                    if (enabled && x==io)
                    {
                        glusetexcache++;
                        if (glusetexcache > 2)
                            glusetexcache = 0;
                    }
                    if (enabled) modval(0,2,(int32_t *)&glusetexcache,1,probey==io);
                    mgametextpal(d,yy, s[glusetexcache], enabled?MENUHIGHLIGHT(io):DISABLEDMENUSHADE, 0);
                }
                break;
                case 8:
                    enabled = usehightile;
                    if (enabled && x==io) r_detailmapping = !r_detailmapping;
                    if (enabled) modval(0,1,(int32_t *)&r_detailmapping,1,probey==io);
                    mgametextpal(d,yy, r_detailmapping ? "Yes" : "No", enabled?MENUHIGHLIGHT(io):DISABLEDMENUSHADE, 0);
                    break;
                case 9:
                    if (x==io) usemodels = 1-usemodels;
                    modval(0,1,(int32_t *)&usemodels,1,probey==io);
                    mgametextpal(d,yy, usemodels ? "Yes" : "No", MENUHIGHLIGHT(io), 0);
                    break;
#endif
                default:
                    break;
                }
                mgametextpal(margin,yy, opts[ii], enabled?MENUHIGHLIGHT(io):DISABLEDMENUSHADE, 10);
                io++;
                yy += 8;
            }
        }
        break;
    case 231:
    case 232:
        M_DrawTopBar("Color Correction");

        margin = MENU_MARGIN_REGULAR;

        x = 4;

        onbar = (probey != 3);
        x = M_Probe(margin,probey==3?106:98,16,x);

        if (x == -1)
        {
            if (g_player[myconnectindex].ps->gm &MODE_GAME && g_currentMenu == 232)
            {
                g_player[myconnectindex].ps->gm &= ~MODE_MENU;
                if ((!g_netServer && ud.multimode < 2)  && ud.recstat != 2)
                {
                    ready2send = 1;
                    totalclock = ototalclock;
                }
            }
            else
            {
                M_ChangeMenu(MENU_VIDEOSETUP);
                probey = 4;
            }
            break;
        }

        menutext(margin,98,MENUHIGHLIGHT(0),0,"Gamma");
        menutext(margin,98+16,MENUHIGHLIGHT(1),0,"Contrast");
        menutext(margin,98+16+16,MENUHIGHLIGHT(2),0,"Brightness");
        menutext(margin,98+16+16+16+8,MENUHIGHLIGHT(3),0,"Reset To Defaults");

        Bsprintf(tempbuf,"%s%.2f",vid_gamma>=0?" ":"",vid_gamma);
        mgametext(margin+177-56,98-8,tempbuf,MENUHIGHLIGHT(0),2+8+16);
        Bsprintf(tempbuf,"%s%.2f",vid_contrast>=0?" ":"",vid_contrast);
        mgametext(margin+177-56,98+16-8,tempbuf,MENUHIGHLIGHT(1),2+8+16);
        Bsprintf(tempbuf,"%s%.2f",vid_brightness>=0?" ":"",vid_brightness);
        mgametext(margin+177-56,98+16+16-8,tempbuf,MENUHIGHLIGHT(2),2+8+16);

        rotatesprite(40<<16,24<<16,24576,0,BONUSSCREEN,0,0,2+8+16,0,scale(ydim,35,200),xdim-1,scale(ydim,80,200)-1);
        rotatesprite(160<<16,27<<16,24576,0,3290,0,0,2+8+16,0,scale(ydim,35,200),xdim-1,scale(ydim,80,200)-1);

        {
            extern int32_t gammabrightness;

            int32_t b = (int32_t)(vid_gamma*40960.f);
            sliderbar(0,margin+177,98,&b,4096,x==0,MENUHIGHLIGHT(0),0,8192,163840);

            if (b != (double)(vid_gamma*40960.f))
            {
                vid_gamma = (double)b/40960.f;
                ud.brightness = GAMMA_CALC;
                ud.brightness <<= 2;
                setbrightness(ud.brightness>>2,g_player[myconnectindex].ps->palette,0);
            }

            b = (int32_t)(vid_contrast*40960.f);
            sliderbar(0,margin+177,98+16,&b,2048,x==1,gammabrightness?MENUHIGHLIGHT(1):DISABLEDMENUSHADE,0,4096,110592);

            if (b != (vid_contrast*40960.f))
            {
                vid_contrast = (double)b/40960.f;
                setbrightness(ud.brightness>>2,g_player[myconnectindex].ps->palette,0);
            }

            b = (int32_t)(vid_brightness*40960.f);
            sliderbar(0,margin+177,98+16+16,&b,2048,x==2,gammabrightness?MENUHIGHLIGHT(2):DISABLEDMENUSHADE,0,-32768,32768);

            if (b != (vid_brightness*40960.f))
            {
                vid_brightness = (double)b/40960.f;
                setbrightness(ud.brightness>>2,g_player[myconnectindex].ps->palette,0);
            }

            if (x == 3)
            {
                vid_gamma = DEFAULT_GAMMA;
                vid_contrast = DEFAULT_CONTRAST;
                vid_brightness = DEFAULT_BRIGHTNESS;
                ud.brightness = 0;
                setbrightness(ud.brightness>>2,g_player[myconnectindex].ps->palette,0);
            }
        }

        break;

    case MENU_SETUP:

        M_DrawTopBar("Game Setup");

        margin = MENU_MARGIN_REGULAR;

        {
            int32_t io, ii, yy, d=margin+160+40, enabled;
            const char *opts[] =
            {
                "Show setup window at start",
                "Show crosshair",
                "Crosshair size",
                "-",
                "Screen size",
                "Status bar size",
                "Stats and chat text size",
                "Show level stats",
                "-",
                "Allow walk with autorun",
                "-",
                "Shadows",
                "Screen tilting",
                "-",
                "Show framerate",
                "Demo recording",
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

            onbar = (probey >= 2 && probey <= 5);
            x = probesm(margin,yy+5,0,io);

            if (x == -1)
            {
                M_ChangeMenu(MENU_OPTIONS);
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
                    if (x==io) ud.config.ForceSetup = 1-ud.config.ForceSetup;
                    modval(0,1,(int32_t *)&ud.config.ForceSetup,1,probey==io);
                    mgametextpal(d,yy, ud.config.ForceSetup ? "Yes" : "No", MENUHIGHLIGHT(io), 0);
                    break;
                case 1:
                    if (x==io) ud.crosshair = !ud.crosshair;
                    modval(0,1,(int32_t *)&ud.crosshair,1,probey==io);
                    {
                        mgametextpal(d,yy,ud.crosshair?"Yes":"No", MENUHIGHLIGHT(io), 0);
                        break;
                    }
                case 2:
                {
                    int32_t chs = ud.crosshairscale;
                    sliderbar(1,d+8,yy+7, &chs,5,x==io,MENUHIGHLIGHT(io),0,25,100);
                    ud.crosshairscale = clamp(chs, 10, 100);
                }
                break;
                case 3:
                {
                    int32_t vpsize = ud.screen_size + 4*(ud.screen_size>=8 && ud.statusbarmode==0);
                    const int32_t ovpsize = vpsize;

                    sliderbar(1,d+8,yy+7, &vpsize,-4,x==io,MENUHIGHLIGHT(io),0,0,68);

                    if (vpsize-ovpsize)
                        G_SetViewportShrink(vpsize-ovpsize);
                }
                break;
                case 4:
                {
                    int32_t sbs = ud.statusbarscale;
                    const int32_t osbs = sbs;
                    sliderbar(1,d+8,yy+7, &sbs,4,x==io,MENUHIGHLIGHT(io),0,36,100);
                    if (x == io && sbs != osbs)
                        G_SetStatusBarScale(sbs);
                }
                break;
                case 5:
                {
                    sliderbar(1,d+8,yy+7, &ud.textscale,50,enabled && x==io,MENUHIGHLIGHT(io),0,100,400);
                }
                break;
                case 6:
                    if (x==io) ud.levelstats = 1-ud.levelstats;
                    modval(0,1,(int32_t *)&ud.levelstats,1,probey==io);
                    mgametextpal(d,yy, ud.levelstats ? "Yes" : "No", MENUHIGHLIGHT(io), 0);
                    break;
                case 7:
                    if (x==io) ud.runkey_mode = 1-ud.runkey_mode;
                    modval(0,1,(int32_t *)&ud.runkey_mode,1,probey==io);
                    mgametextpal(d,yy, ud.runkey_mode ? "No" : "Yes", MENUHIGHLIGHT(io), 0);
                    break;
                case 8:
                    if (x==io) ud.shadows = 1-ud.shadows;
                    modval(0,1,(int32_t *)&ud.shadows,1,probey==io);
                    mgametextpal(d,yy, ud.shadows ? "On" : "Off", MENUHIGHLIGHT(io), 0);
                    break;
                case 9:
                    if (x==io) ud.screen_tilting = !ud.screen_tilting;
#ifdef USE_OPENGL
                    if (!ud.screen_tilting) setrollangle(0);
#endif
                    modval(0,1,(int32_t *)&ud.screen_tilting,1,probey==io);
                    mgametextpal(d,yy, ud.screen_tilting ? "On" : "Off", MENUHIGHLIGHT(io), 0);
                    break;  // original had a 'full' option
                case 10:
                    if (x==io) ud.tickrate = 1-ud.tickrate;
                    modval(0,1,(int32_t *)&ud.tickrate,1,probey==io);
                    mgametextpal(d,yy, ud.tickrate ? "Yes" : "No", MENUHIGHLIGHT(io), 0);
                    break;
                case 11:
                {
                    DukePlayer_t *ps = g_player[myconnectindex].ps;

                    if (x==io)
                    {
                        enabled = !((ps->gm&MODE_GAME) && ud.m_recstat != 1);

                        if ((ps->gm&MODE_GAME)) G_CloseDemoWrite();
                        else ud.m_recstat = !ud.m_recstat;
                    }

                    if ((ps->gm&MODE_GAME) && ud.m_recstat != 1)
                        enabled = 0;

                    mgametextpal(d, yy, ud.m_recstat ? ((enabled && ps->gm&MODE_GAME)?"Running":"On"):"Off",
                                 enabled ? MENUHIGHLIGHT(io) : DISABLEDMENUSHADE, !enabled);
                    break;
                }
                case 12:
                    if (x==io) M_ChangeMenu(MENU_GAMESETUP);
                    break;
                default:
                    break;
                }
                mgametextpal(margin,yy, opts[ii], enabled?MENUHIGHLIGHT(io):DISABLEDMENUSHADE, 10);
                io++;
                yy += 8;
            }
        }
        break;

    case MENU_GAMESETUP:

        M_DrawTopBar("Game Setup");

        margin = MENU_MARGIN_REGULAR;

        {
            int32_t io, ii, yy, d=margin+160+40, enabled;
            const char *opts[] =
            {
                "Parental lock",
                "-",
                "Show inv & pickup messages",
                "Display current weapon",
                "Upgraded status bar",
                "Camera view in demos",
                "-",
                "DM: Ignore map votes",
                "DM: Use private messages",
                "DM: Show player names",
                "DM: Show player weapons",
                "DM: Show player obituaries",
                "-",
                "Console text style",
                "-",
#ifdef _WIN32
                "Check for updates at start",
#else
                "-",
                "-",
#endif
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

            x = probesm(margin,yy+5,0,io);

            if (x == -1)
            {
                M_ChangeMenu(MENU_SETUP);
                probey = 12;
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
                        if (x==io) M_ChangeMenu(MENU_ADULTMODE);
                    }
                    else enabled = 0;
                    break;
                case 1:
                    if (x==io)
                        ud.fta_on = 1-ud.fta_on;
                    modval(0,1,(int32_t *)&ud.fta_on,1,probey==io);
                    mgametextpal(d,yy, ud.fta_on ? "Yes" : "No", MENUHIGHLIGHT(io), 0);
                    break;
                case 2:
                    if (x==io)
                    {
                        ud.drawweapon = (ud.drawweapon == 2) ? 0 : ud.drawweapon+1;
                    }
                    modval(0,2,(int32_t *)&ud.drawweapon,1,probey==io);
                    {
                        const char *s[] = { "Off", "On", "Icon only" };
                        mgametextpal(d,yy, s[ud.drawweapon], MENUHIGHLIGHT(io), 0);
                        break;
                    }
                case 3:
                    if (x==io) ud.althud = !ud.althud;
                    modval(0,1,(int32_t *)&ud.althud,1,probey==io);
                    mgametextpal(d,yy, ud.althud ? "Yes" : "No", MENUHIGHLIGHT(io), 0);
                    break;
                case 4:
                    if (x==io) ud.democams = 1-ud.democams;
                    modval(0,1,(int32_t *)&ud.democams,1,probey==io);
                    mgametextpal(d,yy, ud.democams ? "On" : "Off", MENUHIGHLIGHT(io), 0);
                    break;
                case 5:
                    if (x==io)
                    {
                        ud.autovote = (ud.autovote == 2) ? 0 : ud.autovote+1;
                    }
                    modval(0,2,(int32_t *)&ud.autovote,1,probey==io);
                    {
                        const char *s[] = { "Off", "Vote No", "Vote Yes" };
                        mgametextpal(d,yy, s[ud.autovote], MENUHIGHLIGHT(io), 0);
                        break;
                    }
                case 6:
                    if (x==io) ud.automsg = 1-ud.automsg;
                    modval(0,1,(int32_t *)&ud.automsg,1,probey==io);
                    mgametextpal(d,yy, ud.automsg ? "Off" : "On", MENUHIGHLIGHT(io), 0);
                    break;
                case 7:
                    if (x==io) ud.idplayers = 1-ud.idplayers;
                    modval(0,1,(int32_t *)&ud.idplayers,1,probey==io);
                    mgametextpal(d,yy, ud.idplayers ? "Yes" : "No", MENUHIGHLIGHT(io), 0);
                    break;
                case 8:
                    if (x==io) ud.showweapons = 1-ud.showweapons;
                    modval(0,1,(int32_t *)&ud.showweapons,1,probey==io);
                    ud.config.ShowOpponentWeapons = ud.showweapons;
                    mgametextpal(d,yy, ud.config.ShowOpponentWeapons ? "Yes" : "No", MENUHIGHLIGHT(io), 0);
                    break;
                case 9:
                    if (x==io) ud.obituaries = 1-ud.obituaries;
                    modval(0,1,(int32_t *)&ud.obituaries,1,probey==io);
                    mgametextpal(d,yy, ud.obituaries ? "Yes" : "No", MENUHIGHLIGHT(io), 0);
                    break;
                case 10:
                {
                    int32_t osdmode = OSD_GetTextMode();
                    if (x==io) osdmode = !osdmode;
                    modval(0,1,(int32_t *)&osdmode,1,probey==io);
                    mgametextpal(d,yy, osdmode? "Basic" : "Sprites", MENUHIGHLIGHT(io), 0);
                    if (OSD_GetTextMode() != osdmode)
                        OSD_SetTextMode(osdmode);
                    break;
                }
#ifdef _WIN32
                case 11:
                    i = ud.config.CheckForUpdates;
                    if (x==io) ud.config.CheckForUpdates = 1-ud.config.CheckForUpdates;
                    modval(0,1,(int32_t *)&ud.config.CheckForUpdates,1,probey==io);
                    if (ud.config.CheckForUpdates != i)
                        ud.config.LastUpdateCheck = 0;
                    mgametextpal(d,yy, ud.config.CheckForUpdates ? "Yes" : "No", MENUHIGHLIGHT(io), 0);
                    break;
                case 12:
#else
                case 11:
#endif
                    if (x==io) M_ChangeMenu(MENU_SETUP);
                    break;
                default:
                    break;
                }
                mgametextpal(margin,yy, opts[ii], enabled?MENUHIGHLIGHT(io):DISABLEDMENUSHADE, 10);
                io++;
                yy += 8;
            }
        }
        break;

        // JBF 20031205: Second level options menu selection
    case MENU_OPTIONS:
        M_DrawTopBar("Options");

        margin = 50;

        onbar = 0;
        x = M_Probe(160,margin,18,7);

        last_twoohtwo = probey;

        switch (x)
        {
        case -1:
            if (g_player[myconnectindex].ps->gm&MODE_GAME) M_ChangeMenu(MENU_MAIN_INGAME);
            else M_ChangeMenu(MENU_MAIN);
            break;

        case 0:
            M_ChangeMenu(MENU_SETUP);
            break;

        case 1:
            changesmade = 0;
            M_ChangeMenu(MENU_SOUND);
            break;

        case 2:
        {
            int32_t dax = xdim, day = ydim, daz;
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
            for (dax = 0; dax < (int32_t)(sizeof(vidsets)/sizeof(vidsets[1])) && vidsets[dax] != -1; dax++)
                if (vidsets[dax] == (((getrendermode() >= REND_POLYMOST)<<17)|(fullscreen<<16)|bpp)) break;
            if (dax < (int32_t)(sizeof(vidsets)/sizeof(vidsets[1]))) newvidset = dax;
            curvidset = newvidset;

            M_ChangeMenu(MENU_VIDEOSETUP);
        }
        break;
        case 3:
            if (ud.recstat != 1)
                M_ChangeMenu(20002);
            break;
        case 4:
            currentlist = 0;
        case 5:
        case 6:
            if (x==6 && !CONTROL_JoyPresent) break;
            M_ChangeMenu(MENU_KEYBOARDSETUP+x-4);
            break;
        }

        menutext(160,margin,                  MENUHIGHLIGHT(0),0,"Game Setup");
        menutext(160,margin+18,               MENUHIGHLIGHT(1),0,"Sound Setup");
        menutext(160,margin+18+18,            MENUHIGHLIGHT(2),0,"Video Setup");
        menutext(160,margin+18+18+18,         MENUHIGHLIGHT(3),ud.recstat == 1,"Player Setup");
        menutext(160,margin+18+18+18+18,      MENUHIGHLIGHT(4),0,"Keyboard Setup");
        menutext(160,margin+18+18+18+18+18,   MENUHIGHLIGHT(5),0,"Mouse Setup");
        menutext(160,margin+18+18+18+18+18+18,MENUHIGHLIGHT(6),CONTROL_JoyPresent==0,"Joystick Setup");
        break;

        // JBF 20031206: Video settings menu
    case MENU_VIDEOSETUP:
        M_DrawTopBar("Video Setup");

        margin = MENU_MARGIN_REGULAR;

#ifdef USE_OPENGL
        x = (7/*+(getrendermode() >= REND_POLYMOST)*/);
#else
        x = 7;
#endif
//        onbar = (getrendermode() == REND_CLASSIC && probey == 6); // (probey == 4);
        if (probey == 0 || probey == 1 || probey == 2)
            x = M_Probe(margin,50,16,x);
        else if (probey == 3)
            x = M_Probe(margin,50+16+16+22,0,x);
        else
            x = M_Probe(margin,50+62-16-16-16,16,x);

        if ((probey >= 0 && probey <= 2) && (KB_KeyPressed(sc_LeftArrow) || KB_KeyPressed(sc_RightArrow)))
        {
            S_PlaySound(PISTOL_BODYHIT);
            x=probey;
        }

        switch (x)
        {
        case -1:
            M_ChangeMenu(MENU_OPTIONS);
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
            int32_t lastvidset, lastvidmode, safevidmode = -1;
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
            KB_ClearKeyDown(sc_LeftArrow);
            KB_ClearKeyDown(sc_RightArrow);
        }
        break;

        case 2:
            newfullscreen = !newfullscreen;
            {
                int32_t lastvidset, lastvidmode, safevidmode = -1, safevidset = -1;
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
            KB_ClearKeyDown(sc_LeftArrow);
            KB_ClearKeyDown(sc_RightArrow);
            break;

        case 3:
            if (!changesmade) break;
            {
                int32_t pxdim, pydim, pfs, pbpp, prend;
                int32_t nxdim, nydim, nfs, nbpp, nrend;

                pxdim = xdim;
                pydim = ydim;
                pbpp = bpp;
                pfs = fullscreen;
                prend = getrendermode();
                nxdim = (newvidmode==validmodecnt)?xdim:validmode[newvidmode].xdim;
                nydim = (newvidmode==validmodecnt)?ydim:validmode[newvidmode].ydim;
                nfs   = newfullscreen;
                nbpp  = (newvidmode==validmodecnt)?bpp:validmode[newvidmode].bpp;
                nrend = (vidsets[newvidset] & 0x20000) ? (nbpp==8?2:
#ifdef USE_OPENGL
                        glrendmode
#else
                        0
#endif
                                                         ) : 0;

                if (setgamemode(nfs, nxdim, nydim, nbpp) < 0)
                {
                    if (setgamemode(pfs, pxdim, pydim, pbpp) < 0)
                    {
                        setrendermode(prend);
                        G_GameExit("Failed restoring old video mode.");
                    }
                    else onvideomodechange(pbpp > 8);
                }
                else onvideomodechange(nbpp > 8);

                g_restorePalette = -1;
                G_UpdateScreenArea();
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
            M_ChangeMenu(231);
            break;

        case 5:
            if (getrendermode() == REND_CLASSIC)
            {
                ud.detail = 1-ud.detail;
                break;
            }
#ifdef USE_OPENGL
            gltexfiltermode++;
            if (gltexfiltermode > 5)
                gltexfiltermode = 0;
            gltexapplyprops();
            break;
#endif
        case 6:
//            if (getrendermode() == REND_CLASSIC) break;
            M_ChangeMenu(230);
            break;
        }

        menutext(margin,50,MENUHIGHLIGHT(0),0,"Resolution");
        Bsprintf(tempbuf,"%d x %d",
                 (newvidmode==validmodecnt)?xdim:validmode[newvidmode].xdim,
                 (newvidmode==validmodecnt)?ydim:validmode[newvidmode].ydim);
        mgametext(margin+168,50-8,tempbuf,MENUHIGHLIGHT(0),2+8+16);

        menutext(margin,50+16,MENUHIGHLIGHT(1),0,"Renderer");
        if (vidsets[newvidset]&0x20000)
            Bsprintf(tempbuf,"%d-bit OpenGL", vidsets[newvidset]&0x0ffff);
        else
            Bsprintf(tempbuf,"Software");
        mgametext(margin+168,50+16-8,tempbuf,MENUHIGHLIGHT(1),2+8+16);

        menutext(margin,50+16+16,MENUHIGHLIGHT(2),0,"Fullscreen");
        menutext(margin+168,50+16+16,MENUHIGHLIGHT(2),0,newfullscreen?"Yes":"No");

        menutext(margin+16,50+16+16+22,MENUHIGHLIGHT(3),changesmade==0,"Apply Changes");

        menutext(margin,50+62+16,MENUHIGHLIGHT(4),0,"Color Correction");
        /*        {
                    short ss = ud.brightness;
                    bar(c+171,50+62+16,&ss,8,x==4,MENUHIGHLIGHT(4),PHX(-6));
                    if (x==4)
                    {
                        ud.brightness = ss;
                        setbrightness(ud.brightness>>2,&g_player[myconnectindex].ps->palette[0],0);
                    }
                }
        */
        if (getrendermode() == REND_CLASSIC)
        {
            menutext(margin,50+62+16+16,MENUHIGHLIGHT(5),0,"Pixel Doubling");
            menutext(margin+168,50+62+16+16,MENUHIGHLIGHT(5),0,ud.detail?"Off":"On");
            modval(0,1,(int32_t *)&ud.detail,1,probey==5);
        }
#ifdef USE_OPENGL
        else
        {
            int32_t filter = gltexfiltermode;
            menutext(margin,50+62+16+16,MENUHIGHLIGHT(5),0,"Texture Filter");
            switch (gltexfiltermode)
            {
            case 0:
                strcpy(tempbuf,"Nearest");
                break;
            case 1:
                strcpy(tempbuf,"Linear");
                break;
            case 2:
                strcpy(tempbuf,"Near_MM_Near");
                break;
            case 3:
                strcpy(tempbuf,"Bilinear");
                break;
            case 4:
                strcpy(tempbuf,"Near_MM_Lin");
                break;
            case 5:
                strcpy(tempbuf,"Trilinear");
                break;
            default:
                strcpy(tempbuf,"Other");
                break;
            }
            modval(0,5,(int32_t *)&gltexfiltermode,1,probey==5);
            if (gltexfiltermode != filter)
                gltexapplyprops();
            mgametextpal(margin+168,50+62+16+16-8,tempbuf,MENUHIGHLIGHT(5),getrendermode() == REND_CLASSIC);
        }
#endif
        menutext(margin,50+62+16+16+16,MENUHIGHLIGHT(6),0 /*bpp==8*/,"Renderer Setup");
        break;

    case MENU_KEYBOARDSETUP:
        M_DrawTopBar("Keyboard Setup");

        margin = MENU_MARGIN_REGULAR;

        onbar = 0;

        if (probey == NUMGAMEFUNCTIONS)
            x = probesm(60,145,0,NUMGAMEFUNCTIONS+2);
        else if (probey == NUMGAMEFUNCTIONS+1)
            x = probesm(60,152,0,NUMGAMEFUNCTIONS+2);
        else x = probesm(0,0,0,NUMGAMEFUNCTIONS+2);

        if (x==-1)
        {
            M_ChangeMenu(MENU_OPTIONS);
            probey = 4;
        }
        else if (x == NUMGAMEFUNCTIONS)
        {
            CONFIG_SetDefaultKeys((const char (*)[MAXGAMEFUNCLEN])keydefaults);
            break;
        }
        else if (x == NUMGAMEFUNCTIONS+1)
        {
            CONFIG_SetDefaultKeys(oldkeydefaults);
            break;
        }
        else if (x>=0)
        {
            function = probey;
            whichkey = currentlist;
            M_ChangeMenu(MENU_KEYBOARDASSIGN);
            KB_FlushKeyboardQueue();
            KB_ClearLastScanCode();
            break;
        }

        // the top of our list
        m = clamp(probey-6, 0, NUMGAMEFUNCTIONS-13);

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
            S_PlaySound(KICK_HIT);
        }
        else if (KB_KeyPressed(sc_Delete))
        {
            char key[2];
            key[0] = ud.config.KeyboardKeys[probey][0];
            key[1] = ud.config.KeyboardKeys[probey][1];
            ud.config.KeyboardKeys[probey][currentlist] = 0xff;
            CONFIG_MapKey(probey, ud.config.KeyboardKeys[probey][0], key[0], ud.config.KeyboardKeys[probey][1], key[1]);
            S_PlaySound(KICK_HIT);
            KB_ClearKeyDown(sc_Delete);
        }

        for (l=min(13,NUMGAMEFUNCTIONS)-1; l >= 0 ; l--)
        {
            p = CONFIG_FunctionNumToName(m+l);
            if (!p) continue;

            strcpy(tempbuf, p);
            for (i=0; tempbuf[i]; i++) if (tempbuf[i]=='_') tempbuf[i] = ' ';
            // game function name redefined --> pal 8 text
            minitextshade(70,34+l*8,tempbuf,(m+l == probey)?0:16,
                          Bstrcmp(keydefaults[3*(m+l)],oldkeydefaults[3*(m+l)]) ? 8 : 1, 10+16);

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

        mgametextpal(160,140,  "Reset Keys To Defaults",MENUHIGHLIGHT(NUMGAMEFUNCTIONS),10);
        mgametextpal(160,140+7,"Set Classic Key Layout",MENUHIGHLIGHT(NUMGAMEFUNCTIONS+1),10);
        mgametext(160,144+9+3,"Up/Down = Select Action",0,2+8+16);
        mgametext(160,144+9+9+3,"Left/Right = Select List",0,2+8+16);
        mgametext(160,144+9+9+9+3,"Enter = Modify   Delete = Clear",0,2+8+16);

        break;

    case MENU_KEYBOARDASSIGN:
    {
        int32_t sc;
        M_DrawTopBar("Keyboard Setup");

        mgametext(320>>1,90,"Press the key to assign as",0,2+8+16);
        Bsprintf(tempbuf,"%s for \"%s\"", whichkey?"secondary":"primary", CONFIG_FunctionNumToName(function));
        mgametext(320>>1,90+9,tempbuf,0,2+8+16);
        mgametext(320>>1,90+9+9+9,"Press \"Escape\" To Cancel",0,2+8+16);

        sc = KB_GetLastScanCode();
        if (sc != sc_None || RMB)
        {
            if (sc == sc_Escape || RMB)
            {
                S_PlaySound(EXITMENUSOUND);
            }
            else
            {
                char key[2];
                key[0] = ud.config.KeyboardKeys[function][0];
                key[1] = ud.config.KeyboardKeys[function][1];

                S_PlaySound(PISTOL_BODYHIT);

                ud.config.KeyboardKeys[function][whichkey] = KB_GetLastScanCode();

                CONFIG_MapKey(function, ud.config.KeyboardKeys[function][0], key[0], ud.config.KeyboardKeys[function][1], key[1]);
            }

            M_ChangeMenu(MENU_KEYBOARDSETUP);

            currentlist = whichkey;
            probey = function;

            KB_ClearKeyDown(sc);
        }

        break;
    }
    case 205:
        M_DrawTopBar("Mouse Setup");

        margin = 60-4;

        onbar = (probey == NUMMOUSEFUNCTIONS);
        if (probey < NUMMOUSEFUNCTIONS)
            x = probesm(73,38,8,NUMMOUSEFUNCTIONS+2+2+1);
        else
            x = probesm(40,123-(NUMMOUSEFUNCTIONS)*9,9,NUMMOUSEFUNCTIONS+2+2+1);

        if (x==-1)
        {
            M_ChangeMenu(MENU_OPTIONS);
            probey = 5;
            break;
        }
        else if (x == NUMMOUSEFUNCTIONS)
        {
            // sensitivity
        }
        else if (x == NUMMOUSEFUNCTIONS+1)
        {
            // mouse aiming toggle
            if (!ud.mouseaiming) g_myAimMode = 1-g_myAimMode;
        }
        else if (x == NUMMOUSEFUNCTIONS+2)
        {
            // invert mouse aim
            ud.mouseflip = 1-ud.mouseflip;
        }
        else if (x == NUMMOUSEFUNCTIONS+2+1)
        {
            //input smoothing
            ud.config.SmoothInput = !ud.config.SmoothInput;
            CONTROL_SmoothMouse = ud.config.SmoothInput;
        }
        else if (x == NUMMOUSEFUNCTIONS+2+2)
        {
            //advanced
            M_ChangeMenu(212);
            break;
        }
        else if (x >= 0)
        {
            //set an option
            M_ChangeMenu(211);
            function = 0;
            whichkey = x;
            if (x < NUMDOUBLEMBTNS*2)
                probey = ud.config.MouseFunctions[x>>1][x&1];
            else
                probey = ud.config.MouseFunctions[x-NUMDOUBLEMBTNS][0];
            if (probey < 0) probey = NUMGAMEFUNCTIONS-1;
            break;
        }

        for (l=0; l < NUMMOUSEFUNCTIONS; l++)
        {
            tempbuf[0] = 0;
            if (l < NUMDOUBLEMBTNS*2)
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
                Bstrcpy(tempbuf, mousebuttonnames[l-NUMDOUBLEMBTNS]);
                m = ud.config.MouseFunctions[l-NUMDOUBLEMBTNS][0];
            }

            minitextshade(margin+20,34+l*8,tempbuf,(l==probey)?0:16,1,10+16);

            if (m == -1)
                minitextshade(margin+100+20,34+l*8,"  -None-",(l==probey)?0:16,2,10+16);
            else
            {
                strcpy(tempbuf, CONFIG_FunctionNumToName(m));
                for (i=0; tempbuf[i]; i++) if (tempbuf[i]=='_') tempbuf[i] = ' ';
                minitextshade(margin+100+20,34+l*8,tempbuf,(l==probey)?0:16,2,10+16);
            }
        }

        mgametextpal(40,118,"Base mouse sensitivity",MENUHIGHLIGHT(NUMMOUSEFUNCTIONS),10);
        mgametextpal(40,118+9,"Use mouse aiming",!ud.mouseaiming?MENUHIGHLIGHT(NUMMOUSEFUNCTIONS+1):DISABLEDMENUSHADE,10);
        mgametextpal(40,118+9+9,"Invert mouse",MENUHIGHLIGHT(NUMMOUSEFUNCTIONS+2),10);
        mgametextpal(40,118+9+9+9,"Smooth mouse movement",MENUHIGHLIGHT(NUMMOUSEFUNCTIONS+2+1),10);
        mgametextpal(40,118+9+9+9+9,"Advanced mouse setup",MENUHIGHLIGHT(NUMMOUSEFUNCTIONS+2+2),10);

        {
            int32_t sense = (int32_t)(CONTROL_MouseSensitivity * 4.0f);
            sense = clamp(sense, 0, 63);
            barsm(248,126,&sense,2,x==NUMMOUSEFUNCTIONS,MENUHIGHLIGHT(NUMMOUSEFUNCTIONS),0);
            CONTROL_MouseSensitivity = sense / 4.0f;
        }

        if (!ud.mouseaiming) modval(0,1,(int32_t *)&g_myAimMode,1,probey == NUMMOUSEFUNCTIONS+1);
        else if (probey == NUMMOUSEFUNCTIONS+1)
        {
            mgametext(160,140+9+9+9,"Set mouse aim type to toggle on/off",0,2+8+16);
            mgametext(160,140+9+9+9+9,"in the Player Setup menu to enable",0,2+8+16);
        }

        modval(0,1,(int32_t *)&ud.mouseflip,1,probey == NUMMOUSEFUNCTIONS+2);
        modval(0,1,(int32_t *)&ud.config.SmoothInput,1,probey == NUMMOUSEFUNCTIONS+2+1);
        if (probey == NUMMOUSEFUNCTIONS+2+1)
        {
//            mgametext(160,160+9,"This option incurs a movement delay",0,2+8+16);
            CONTROL_SmoothMouse = ud.config.SmoothInput;
        }

        mgametextpal(240,118+9, g_myAimMode && !ud.mouseaiming ? "Yes" : "No",
                     !ud.mouseaiming?MENUHIGHLIGHT(NUMMOUSEFUNCTIONS+1):DISABLEDMENUSHADE, 0);
        mgametextpal(240,118+9+9, !ud.mouseflip ? "Yes" : "No", MENUHIGHLIGHT(NUMMOUSEFUNCTIONS+2), 0);
        mgametextpal(240,118+9+9+9, ud.config.SmoothInput ? "Yes" : "No", MENUHIGHLIGHT(NUMMOUSEFUNCTIONS+2+1), 0);

        if (probey < NUMMOUSEFUNCTIONS)
        {
            mgametext(160,160+9,"Up/Down = Select Button",0,2+8+16);
            mgametext(160,160+9+9,"Enter = Modify",0,2+8+16);
        }
        break;

    case 211:
        if (function == 0) M_DrawTopBar("Mouse Setup");
        else if (function == 1) M_DrawTopBar("Advanced Mouse");
        else if (function == 2) M_DrawTopBar("Joystick Buttons");
        else if (function == 3) M_DrawTopBar("Joystick Axes");

        x = M_Probe(0,0,0,NUMGAMEFUNCTIONS);

        if (x==-1)
        {
            if (function == 0)
            {
                // mouse button
                M_ChangeMenu(205);
                probey = whichkey;
            }
            else if (function == 1)
            {
                // mouse digital axis
                M_ChangeMenu(212);
                probey = 3+(whichkey^2);
            }
            else if (function == 2)
            {
                // joystick button/hat
                M_ChangeMenu(207);
                probey = whichkey;
            }
            else if (function == 3)
            {
                // joystick digital axis
                M_ChangeMenu((whichkey>>2)+208);
                probey = 1+((whichkey>>1)&1)*4+(whichkey&1);
            }
            break;
        }
        else if (x >= 0)
        {
            if (x == NUMGAMEFUNCTIONS-1) x = -1;

            if (function == 0)
            {
                if (whichkey < NUMDOUBLEMBTNS*2)
                {
                    ud.config.MouseFunctions[whichkey>>1][whichkey&1] = x;
                    CONTROL_MapButton(x, whichkey>>1, whichkey&1, controldevice_mouse);
                    CONTROL_FreeMouseBind(whichkey>>1);
                }
                else
                {
                    ud.config.MouseFunctions[whichkey-NUMDOUBLEMBTNS][0] = x;
                    CONTROL_MapButton(x, whichkey-NUMDOUBLEMBTNS, 0, controldevice_mouse);
                    CONTROL_FreeMouseBind(whichkey-NUMDOUBLEMBTNS);

                }
                M_ChangeMenu(205);
                probey = whichkey;
            }
            else if (function == 1)
            {
                ud.config.MouseDigitalFunctions[whichkey>>1][whichkey&1] = x;
                CONTROL_MapDigitalAxis(whichkey>>1, x, whichkey&1, controldevice_mouse);
                M_ChangeMenu(212);
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
                M_ChangeMenu(207);
                probey = whichkey;
            }
            else if (function == 3)
            {
                ud.config.JoystickDigitalFunctions[whichkey>>1][whichkey&1] = x;
                CONTROL_MapDigitalAxis(whichkey>>1, x, whichkey&1, controldevice_joystick);
                M_ChangeMenu((whichkey>>2)+208);
                probey = 1+((whichkey>>1)&1)*4+(whichkey&1);
            }
            break;
        }

        mgametext(320>>1,31,"Select a function to assign",0,2+8+16);

        if (function == 0)
        {
            if (whichkey < NUMDOUBLEMBTNS*2)
                Bsprintf(tempbuf,"to %s%s", (whichkey&1)?"double-clicked ":"", mousebuttonnames[whichkey>>1]);
            else
                Bstrcpy(tempbuf, mousebuttonnames[whichkey-NUMDOUBLEMBTNS]);
        }
        else if (function == 1)
        {
            Bstrcpy(tempbuf,"to digital ");
            switch (whichkey)
            {
            case 0:
                Bstrcat(tempbuf, "Left");
                break;
            case 1:
                Bstrcat(tempbuf, "Right");
                break;
            case 2:
                Bstrcat(tempbuf, "Up");
                break;
            case 3:
                Bstrcat(tempbuf, "Down");
                break;
            }
        }
        else if (function == 2)
        {
            static const char *directions[] =
            {
                "Up", "Right", "Down", "Left"
            };
            if (whichkey < 2*joynumbuttons)
                Bsprintf(tempbuf,"to %s%s", (whichkey&1)?"double-clicked ":"", getjoyname(1,whichkey>>1));
            else
                Bsprintf(tempbuf,"to hat %s", directions[whichkey-2*joynumbuttons]);
        }
        else if (function == 3)
        {
            Bsprintf(tempbuf,"to digital %s %s",getjoyname(0,whichkey>>1),(whichkey&1)?"positive":"negative");
        }

        mgametext(320>>1,31+9,tempbuf,0,2+8+16);

        m = clamp(probey-6, 0, NUMGAMEFUNCTIONS-13);

        for (l=0; l < min(13,NUMGAMEFUNCTIONS); l++)
        {
            if (l+m == NUMGAMEFUNCTIONS-1)
                strcpy(tempbuf, "  -None-");
            else
                strcpy(tempbuf, CONFIG_FunctionNumToName(m+l));

            for (i=0; tempbuf[i]; i++) if (tempbuf[i]=='_') tempbuf[i] = ' ';
            minitextshade(100,51+l*8,tempbuf,(m+l == probey)?0:16,
                          Bstrcmp(keydefaults[3*(m+l)],oldkeydefaults[3*(m+l)]) ? 8 : 1, 10+16);
        }

        mgametext(320>>1,161,"Press \"Escape\" To Cancel",0,2+8+16);

        break;

    case 212:
        M_DrawTopBar("Advanced Mouse");

        margin = MENU_MARGIN_REGULAR;

        onbar = (probey == 0 || probey == 1 || probey == 2);
        if (probey < 3)
            x = M_Probe(margin,46,16,7);
        else if (probey < 7)
        {
            m=50;
            x = probesm(margin+10,97+16-(9+9+9),9,7);
        }
        else
        {
            x = M_Probe(margin,146+16-(16+16+16+16+16+16),16,7);
        }

        switch (x)
        {
        case -1:
            M_ChangeMenu(205);
            probey = NUMMOUSEFUNCTIONS+2+2;
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
            M_ChangeMenu(211);
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
            mgametext(160,144+9+9,"Digital axes are not for mouse look",0,2+8+16);
            mgametext(160,144+9+9+9,"or for aiming up and down",0,2+8+16);
            break;
        }

        menutext(margin,46,MENUHIGHLIGHT(0),0,"X-Axis Scale");
        l = (ud.config.MouseAnalogueScale[0]+262144) >> 13;
        bar(margin+160+40,46,&l,1,x==0,MENUHIGHLIGHT(0),0);
        l = (l<<13)-262144;
        if (l != ud.config.MouseAnalogueScale[0])
        {
            CONTROL_SetAnalogAxisScale(0, l, controldevice_mouse);
            ud.config.MouseAnalogueScale[0] = l;
        }
        Bsprintf(tempbuf,"%s%.2f",l>=0?" ":"",(float)l/65536.0);
        mgametext(margin+160-16,46-8,tempbuf,MENUHIGHLIGHT(0),2+8+16);

        menutext(margin,46+16,MENUHIGHLIGHT(1),0,"Y-Axis Scale");
        l = (ud.config.MouseAnalogueScale[1]+262144) >> 13;
        bar(margin+160+40,46+16,&l,1,x==1,MENUHIGHLIGHT(1),0);
        l = (l<<13)-262144;
        if (l != ud.config.MouseAnalogueScale[1])
        {
            CONTROL_SetAnalogAxisScale(1, l, controldevice_mouse);
            ud.config.MouseAnalogueScale[1] = l;
        }
        Bsprintf(tempbuf,"%s%.2f",l>=0?" ":"",(float)l/65536.0);
        mgametext(margin+160-16,46+16-8,tempbuf,MENUHIGHLIGHT(1),2+8+16);

        menutext(margin,46+16+16,MENUHIGHLIGHT(2),0,"Dead Zone");
        l = ud.config.MouseDeadZone>>1;
        bar(margin+160+40,46+16+16,&l,2,x==2,MENUHIGHLIGHT(2),0);
        ud.config.MouseDeadZone = l<<1;
        M_DrawTopBar("Digital Axes Setup");

        if (ud.config.MouseDeadZone == 0)
            Bsprintf(tempbuf," Off");
        else if (ud.config.MouseDeadZone < 48)
            Bsprintf(tempbuf," Low");
        else if (ud.config.MouseDeadZone < 96)
            Bsprintf(tempbuf," Med");
        else if (ud.config.MouseDeadZone < 128)
            Bsprintf(tempbuf,"High");

        mgametext(margin+160-16,46+16+16-8,tempbuf,MENUHIGHLIGHT(2),2+8+16);


        mgametextpal(margin+10,92+16,"Up:",MENUHIGHLIGHT(3),10);
        if (ud.config.MouseDigitalFunctions[1][0] < 0)
            strcpy(tempbuf, "  -None-");
        else
            strcpy(tempbuf, CONFIG_FunctionNumToName(ud.config.MouseDigitalFunctions[1][0]));

        for (i=0; tempbuf[i]; i++) if (tempbuf[i]=='_') tempbuf[i] = ' ';
        minitextshade(margin+10+60,93+16,tempbuf,MENUHIGHLIGHT(3),0,10+16);

        mgametextpal(margin+10,92+16+9,"Down:",MENUHIGHLIGHT(4),10);
        if (ud.config.MouseDigitalFunctions[1][1] < 0)
            strcpy(tempbuf, "  -None-");
        else
            strcpy(tempbuf, CONFIG_FunctionNumToName(ud.config.MouseDigitalFunctions[1][1]));

        for (i=0; tempbuf[i]; i++) if (tempbuf[i]=='_') tempbuf[i] = ' ';
        minitextshade(margin+10+60,93+16+9,tempbuf,MENUHIGHLIGHT(4),0,10+16);

        mgametextpal(margin+10,92+16+9+9,"Left:",MENUHIGHLIGHT(5),10);
        if (ud.config.MouseDigitalFunctions[0][0] < 0)
            strcpy(tempbuf, "  -None-");
        else
            strcpy(tempbuf, CONFIG_FunctionNumToName(ud.config.MouseDigitalFunctions[0][0]));

        for (i=0; tempbuf[i]; i++) if (tempbuf[i]=='_') tempbuf[i] = ' ';
        minitextshade(margin+10+60,93+16+9+9,tempbuf,MENUHIGHLIGHT(5),0,10+16);

        mgametextpal(margin+10,92+16+9+9+9,"Right:",MENUHIGHLIGHT(6),10);
        if (ud.config.MouseDigitalFunctions[0][1] < 0)
            strcpy(tempbuf, "  -None-");
        else
            strcpy(tempbuf, CONFIG_FunctionNumToName(ud.config.MouseDigitalFunctions[0][1]));

        for (i=0; tempbuf[i]; i++) if (tempbuf[i]=='_') tempbuf[i] = ' ';
        minitextshade(margin+10+60,93+16+9+9+9,tempbuf,MENUHIGHLIGHT(6),0,10+16);

        break;

    case 206:
        M_DrawTopBar("Joystick Setup");

        x = M_Probe(160,100-18,18,3);

        switch (x)
        {
        case -1:
            M_ChangeMenu(MENU_OPTIONS);
            probey = 6;
            break;
        case 0:
        case 1:
            M_ChangeMenu(207+x);
            break;
        case 2:
            M_ChangeMenu(213);
            break;
        }

        menutext(160,100-18,0,0,"Edit Buttons");
        menutext(160,100,0,0,"Edit Axes");
        menutext(160,100+18,0,0,"Dead Zones");

        break;

    case 207:
        M_DrawTopBar("Joystick Buttons");

        margin = 2*joynumbuttons + 4*(joynumhats>0);

        x = M_Probe(0,0,0,margin);

        if (x == -1)
        {
            M_ChangeMenu(206);
            probey = 0;
            break;
        }
        else if (x >= 0)
        {
            function = 2;
            whichkey = x;
            M_ChangeMenu(211);
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
        if (margin < 13) m = 0;
        else
        {
            m = probey - 6;
            if (m < 0) m = 0;
            else if (m + 13 >= margin) m = margin-13;
        }

        for (l=0; l<min(13,margin); l++)
        {
            if (m+l < 2*joynumbuttons)
            {
                Bsprintf(tempbuf, "%s%s", ((l+m)&1)?"Double ":"", getjoyname(1,(l+m)>>1));
                x = ud.config.JoystickFunctions[(l+m)>>1][(l+m)&1];
            }
            else
            {
                static const char *directions[] =
                {
                    "Up", "Right", "Down", "Left"
                };
                Bsprintf(tempbuf, "Hat %s", directions[(l+m)-2*joynumbuttons]);
                x = ud.config.JoystickFunctions[joynumbuttons + ((l+m)-2*joynumbuttons)][0];
            }
            minitextshade(80-4,33+l*8,tempbuf,(m+l == probey)?0:16,1,10+16);

            if (x == -1)
                minitextshade(176,33+l*8,"  -None-",(m+l==probey)?0:16,2,10+16);
            else
            {
                strcpy(tempbuf, CONFIG_FunctionNumToName(x));
                for (i=0; tempbuf[i]; i++) if (tempbuf[i]=='_') tempbuf[i] = ' ';
                minitextshade(176,33+l*8,tempbuf,(m+l==probey)?0:16,2,10+16);
            }
        }

        mgametext(160,149,"UP/DOWN = SELECT BUTTON",0,2+8+16);
        mgametext(160,149+9,"ENTER = MODIFY",0,2+8+16);
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
        int32_t thispage, twothispage;

        M_DrawTopBar("Joystick Axes");

        thispage = (g_currentMenu < 217) ? (g_currentMenu-208) : (g_currentMenu-217)+2;
        twothispage = (thispage*2+1 < joynumaxes);

        onbar = 0;
        switch (probey)
        {
        case 0:
        case 4:
            onbar = 1;
            x = M_Probe(88,45+(probey==4)*64,0,1+(4<<twothispage));
            break;
        case 1:
        case 2:
        case 5:
        case 6:
            x = M_Probe(172+(probey==2||probey==6)*72,45+15+(probey==5||probey==6)*64,0,1+(4<<twothispage));
            break;
        case 3:
        case 7:
            x = M_Probe(88,45+15+15+(probey==7)*64,0,1+(4<<twothispage));
            break;
        default:
            x = M_Probe(60,79+79*twothispage,0,1+(4<<twothispage));
            break;
        }

        switch (x)
        {
        case -1:
            M_ChangeMenu(206);
            probey = 1;
            break;
        case 8:
            if (joynumaxes > 2)
            {
                if (thispage == ((joynumaxes+1)/2)-1) M_ChangeMenu(208);
                else
                {
                    if (g_currentMenu == 209) M_ChangeMenu(217);
                    else M_ChangeMenu(g_currentMenu+1);
                }
            }
            break;

        case 4: // bar
            if (!twothispage && joynumaxes > 2)
                M_ChangeMenu(208);
        case 0:
            break;

        case 1: // digitals
        case 2:
        case 5:
        case 6:
            function = 3;
            whichkey = ((thispage*2+(x==5||x==6)) << 1) + (x==2||x==6);
            M_ChangeMenu(211);
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
                mouseyaxismode = -1;
            }
            break;
        default:
            break;
        }

        if (getjoyname(0,thispage*2) != NULL)
        {
            Bstrcpy(tempbuf,(char *)getjoyname(0,thispage*2));
            menutext(42,32,0,0,tempbuf);
        }

        if (twothispage)
        {
            if (getjoyname(0,thispage*2+1) != NULL)
            {
                Bstrcpy(tempbuf,(char *)getjoyname(0,thispage*2+1));
                menutext(42,32+64,0,0,tempbuf);
            }
        }

        mgametext(76,38,"Scale",0,2+8+16);
        l = (ud.config.JoystickAnalogueScale[thispage*2]+262144) >> 13;
        bar(140+56,38+8,&l,1,x==0,0,0);
        l = (l<<13)-262144;
        if (l != ud.config.JoystickAnalogueScale[thispage*2])
        {
            CONTROL_SetAnalogAxisScale(thispage*2, l, controldevice_joystick);
            ud.config.JoystickAnalogueScale[thispage*2] = l;
        }
        Bsprintf(tempbuf,"%s%.2f",l>=0?" ":"",(float)l/65536.0);
        mgametext(140,38,tempbuf,0,2+8+16);

        mgametext(76,38+15,"Digital",0,2+8+16);
        if (ud.config.JoystickDigitalFunctions[thispage*2][0] < 0)
            strcpy(tempbuf, "  -None-");
        else
            strcpy(tempbuf, CONFIG_FunctionNumToName(ud.config.JoystickDigitalFunctions[thispage*2][0]));

        for (i=0; tempbuf[i]; i++) if (tempbuf[i]=='_') tempbuf[i] = ' ';
        minitext(140+12,38+15,tempbuf,0,10+16);

        if (ud.config.JoystickDigitalFunctions[thispage*2][1] < 0)
            strcpy(tempbuf, "  -None-");
        else
            strcpy(tempbuf, CONFIG_FunctionNumToName(ud.config.JoystickDigitalFunctions[thispage*2][1]));

        for (i=0; tempbuf[i]; i++) if (tempbuf[i]=='_') tempbuf[i] = ' ';
        minitext(140+12+72,38+15,tempbuf,0,10+16);

        mgametext(76,38+15+15,"Analog",0,2+8+16);
        if (CONFIG_AnalogNumToName(ud.config.JoystickAnalogueAxes[thispage*2]))
        {
            p = CONFIG_AnalogNumToName(ud.config.JoystickAnalogueAxes[thispage*2]);
            if (p)
            {
                mgametext(140+12,38+15+15, strchr(p,'_')+1, 0, 2+8+16);
            }
        }

        if (twothispage)
        {
            mgametext(76,38+64,"Scale",0,2+8+16);
            l = (ud.config.JoystickAnalogueScale[thispage*2+1]+262144) >> 13;
            bar(140+56,38+8+64,&l,1,x==4,0,0);
            l = (l<<13)-262144;
            if (l != ud.config.JoystickAnalogueScale[thispage*2+1])
            {
                CONTROL_SetAnalogAxisScale(thispage*2+1, l, controldevice_joystick);
                ud.config.JoystickAnalogueScale[thispage*2+1] = l;
            }
            Bsprintf(tempbuf,"%s%.2f",l>=0?" ":"",(float)l/65536.0);
            mgametext(140,38+64,tempbuf,0,2+8+16);

            mgametext(76,38+64+15,"Digital",0,2+8+16);
            if (ud.config.JoystickDigitalFunctions[thispage*2+1][0] < 0)
                strcpy(tempbuf, "  -None-");
            else
                strcpy(tempbuf, CONFIG_FunctionNumToName(ud.config.JoystickDigitalFunctions[thispage*2+1][0]));

            for (i=0; tempbuf[i]; i++) if (tempbuf[i]=='_') tempbuf[i] = ' ';
            minitext(140+12,38+15+64,tempbuf,0,10+16);

            if (ud.config.JoystickDigitalFunctions[thispage*2+1][1] < 0)
                strcpy(tempbuf, "  -None-");
            else
                strcpy(tempbuf, CONFIG_FunctionNumToName(ud.config.JoystickDigitalFunctions[thispage*2+1][1]));

            for (i=0; tempbuf[i]; i++) if (tempbuf[i]=='_') tempbuf[i] = ' ';
            minitext(140+12+72,38+15+64,tempbuf,0,10+16);

            mgametext(76,38+64+15+15,"Analog",0,2+8+16);
            if (CONFIG_AnalogNumToName(ud.config.JoystickAnalogueAxes[thispage*2+1]))
            {
                p = CONFIG_AnalogNumToName(ud.config.JoystickAnalogueAxes[thispage*2+1]);
                if (p)
                {
                    mgametext(140+12,38+64+15+15, strchr(p,'_')+1, 0, 2+8+16);
                }
            }
        }

        if (joynumaxes > 2)
        {
            menutext(320>>1,twothispage?158:108,SHX(-10),(joynumaxes<=2),"Next...");
            Bsprintf(tempbuf,"Page %d of %d",thispage+1,(joynumaxes+1)/2);
            mgametext(320-100,158,tempbuf,0,2+8+16);
        }
        break;
    }

    case 213:
    case 214:
    case 215:
    case 216:
    {
        // Pray this is enough pages for now :-|
        int32_t first,last;
        M_DrawTopBar("Joy Dead Zones");

        first = 4*(g_currentMenu-213);
        last  = min(4*(g_currentMenu-213)+4,joynumaxes);

        onbar = 1;
        x = M_Probe(320,48,15,2*(last-first)+(joynumaxes>4));

        if (x==-1)
        {
            M_ChangeMenu(206);
            probey = 2;
            break;
        }
        else if (x==2*(last-first) && joynumaxes>4)
        {
            M_ChangeMenu((g_currentMenu-213) == (joynumaxes/4) ? 213 : (g_currentMenu+1));
            probey = 0;
            break;
        }

        for (m = first; m < last; m++)
        {
            int32_t odx,dx,ody,dy;
            Bstrcpy(tempbuf,(char *)getjoyname(0,m));
            menutext(32,48+30*(m-first),0,0,tempbuf);

            mgametext(128,48+30*(m-first)-8,"Dead",0,2+8+16);
            mgametext(128,48+30*(m-first)-8+15,"Satu",0,2+8+16);

            dx = odx = min(64,64l*ud.config.JoystickAnalogueDead[m]/10000l);
            dy = ody = min(64,64l*ud.config.JoystickAnalogueSaturate[m]/10000l);

            bar(217,48+30*(m-first),&dx,4,x==((m-first)*2),0,0);
            bar(217,48+30*(m-first)+15,&dy,4,x==((m-first)*2+1),0,0);

            Bsprintf(tempbuf,"%3d%%",100*dx/64);
            mgametext(217-49,48+30*(m-first)-8,tempbuf,0,2+8+16);
            Bsprintf(tempbuf,"%3d%%",100*dy/64);
            mgametext(217-49,48+30*(m-first)-8+15,tempbuf,0,2+8+16);

            if (dx != odx) ud.config.JoystickAnalogueDead[m]     = 10000l*dx/64l;
            if (dy != ody) ud.config.JoystickAnalogueSaturate[m] = 10000l*dy/64l;
            if (dx != odx || dy != ody)
                setjoydeadzone(m,ud.config.JoystickAnalogueDead[m],ud.config.JoystickAnalogueSaturate[m]);
        }
        //mgametext(160,158,"Dead = Dead Zone, Sat. = Saturation",0,2+8+16);
        if (joynumaxes>4)
        {
            menutext(32,48+30*(last-first),0,0,"Next...");
            Bsprintf(tempbuf,"Page %d of %d", 1+(g_currentMenu-213), (joynumaxes+3)/4);
            mgametext(320-100,158,tempbuf,0,2+8+16);
        }
        break;
    }

    case MENU_SOUND:
    case MENU_SOUND_INGAME:

        margin = MENU_MARGIN_REGULAR;
        M_DrawTopBar("Sound Setup");

        if (!(changesmade & 8))
        {
            soundrate = ud.config.MixRate;
            soundvoices = ud.config.NumVoices;
            soundbits = ud.config.NumBits;
        }

        {
            int32_t io, ii, yy, d=margin+160+40, enabled, j;
            const char *opts[] =
            {
                "Sound",
                "Sound volume",
                "-",
                "Music",
                "Music volume",
                "-",
                "Playback sampling rate",
                "Sample size",
                "Number of voices",
                "-",
                "Restart sound system",
                "-",
                "Duke talk",
                "Dukematch player sounds",
                "Ambient sounds",
                "Reverse stereo channels",
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

            onbar = (probey == 1 || probey == 3);
            x = probesm(margin,yy+5,0,io);

            if (x == -1)
            {
                ud.config.MixRate = soundrate;
                ud.config.NumVoices = soundvoices;
                ud.config.NumBits = soundbits;

                if (g_player[myconnectindex].ps->gm &MODE_GAME && g_currentMenu == MENU_SOUND_INGAME)
                {
                    g_player[myconnectindex].ps->gm &= ~MODE_MENU;
                    if ((!g_netServer && ud.multimode < 2)  && ud.recstat != 2)
                    {
                        ready2send = 1;
                        totalclock = ototalclock;
                    }
                }

                else M_ChangeMenu(MENU_OPTIONS);
                probey = 1;
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
                    if (ud.config.FXDevice >= 0)
                    {
                        i = ud.config.SoundToggle;
                        modval(0,1,(int32_t *)&ud.config.SoundToggle,1,probey==io);
                        if (x==io)
                            ud.config.SoundToggle = 1-ud.config.SoundToggle;
                        if (i != ud.config.SoundToggle)
                        {
                            if (ud.config.SoundToggle == 0)
                            {
                                FX_StopAllSounds();
                                S_ClearSoundLocks();
                            }
                        }
                    }
                    mgametextpal(d,yy, ud.config.SoundToggle ? "On" : "Off", MENUHIGHLIGHT(io), 0);
                    break;
                case 1:
                {
                    enabled = (ud.config.SoundToggle && ud.config.FXDevice >= 0);
                    l = ud.config.FXVolume;
                    sliderbar(1,d+8,yy+7, &ud.config.FXVolume,15,probey==io,enabled?MENUHIGHLIGHT(io):UNSELMENUSHADE,!enabled,0,255);
                    if (l != ud.config.FXVolume)
                        FX_SetVolume((int16_t) ud.config.FXVolume);
                }
                break;
                case 2:
                    if (ud.config.MusicDevice >= 0)
                    {
                        i = ud.config.MusicToggle;
                        modval(0,1,(int32_t *)&ud.config.MusicToggle,1,probey==io);
                        if (x==io)
                            ud.config.MusicToggle = 1-ud.config.MusicToggle;
                        if (i != ud.config.MusicToggle)
                        {
                            if (ud.config.MusicToggle == 0) S_PauseMusic(1);
                            else
                            {
                                if (ud.recstat != 2 && g_player[myconnectindex].ps->gm&MODE_GAME)
                                {
                                    if (MapInfo[g_musicIndex].musicfn != NULL)
                                        S_PlayMusic(&MapInfo[g_musicIndex].musicfn[0],g_musicIndex);
                                }
                                else S_PlayMusic(&EnvMusicFilename[0][0],MAXVOLUMES*MAXLEVELS);

                                S_PauseMusic(0);
                            }
                        }
                    }
                    mgametextpal(d,yy, ud.config.MusicToggle ? "On" : "Off", MENUHIGHLIGHT(io), 0);
                    break;
                case 3:
                {
                    enabled = (ud.config.MusicToggle && ud.config.MusicDevice >= 0);
                    l = ud.config.MusicVolume;
                    sliderbar(1,d+8,yy+7, &ud.config.MusicVolume,15,probey==io,enabled?MENUHIGHLIGHT(io):UNSELMENUSHADE,!enabled,0,255);
                    if (l != ud.config.MusicVolume)
                        S_MusicVolume((int16_t) ud.config.MusicVolume);
                }
                break;
                case 4:
                {
                    int32_t rates[] = { 22050, 24000, 32000, 44100, 48000 };
                    int32_t j = (sizeof(rates)/sizeof(rates[0]));

                    for (i = 0; i<j; i++)
                        if (rates[i] == ud.config.MixRate)
                            break;

                    modval(0,j-1,(int32_t *)&i,1,enabled && probey==io);
                    if (x == io)
                    {
                        i++;
                        if (i >= j)
                            i = 0;
                    }
                    if (i == j)
                        Bsprintf(tempbuf,"Other");
                    else
                    {
                        Bsprintf(tempbuf,"%d Hz",rates[i]);
                        if (rates[i] != ud.config.MixRate)
                        {
                            ud.config.MixRate = rates[i];
                            changesmade |= 8;
                        }
                    }
                    mgametextpal(d,yy,tempbuf, MENUHIGHLIGHT(io), 0);
                }
                break;
                case 5:
                    i = ud.config.NumBits;
                    if (x==io)
                    {
                        if (ud.config.NumBits == 8)
                            ud.config.NumBits = 16;
                        else if (ud.config.NumBits == 16)
                            ud.config.NumBits = 8;
                    }
                    modval(8,16,(int32_t *)&ud.config.NumBits,8,probey==io);
                    if (ud.config.NumBits != i)
                        changesmade |= 8;
                    Bsprintf(tempbuf,"%d-bit",ud.config.NumBits);
                    mgametextpal(d,yy, tempbuf, MENUHIGHLIGHT(io), 0);
                    break;
                case 6:
                    i = ud.config.NumVoices;
                    if (x==io)
                    {
                        ud.config.NumVoices++;
                        if (ud.config.NumVoices > 256)
                            ud.config.NumVoices = 4;
                    }
                    modval(4,256,(int32_t *)&ud.config.NumVoices,4,probey==io);
                    if (ud.config.NumVoices != i)
                        changesmade |= 8;
                    Bsprintf(tempbuf,"%d",ud.config.NumVoices);
                    mgametextpal(d,yy, tempbuf, MENUHIGHLIGHT(io), 0);
                    break;
                case 7:
                    enabled = (changesmade&8);
                    if (!enabled) break;
                    if (x == io)
                    {
                        S_SoundShutdown();
                        S_MusicShutdown();

                        S_MusicStartup();
                        S_SoundStartup();

                        FX_StopAllSounds();
                        S_ClearSoundLocks();

                        if (ud.config.MusicToggle == 1)
                        {
                            if (ud.recstat != 2 && g_player[myconnectindex].ps->gm&MODE_GAME)
                            {
                                if (MapInfo[g_musicIndex].musicfn != NULL)
                                    S_PlayMusic(&MapInfo[g_musicIndex].musicfn[0],g_musicIndex);
                            }
                            else S_PlayMusic(&EnvMusicFilename[0][0],MAXVOLUMES*MAXLEVELS);
                        }
                        changesmade &= ~8;
                    }
                    break;
                case 8:
                    enabled = (ud.config.SoundToggle && ud.config.FXDevice >= 0);
                    i = j = (ud.config.VoiceToggle&1);
                    modval(0,1,(int32_t *)&i,1,enabled && probey==io);
                    if (x == io || j != i)
                        ud.config.VoiceToggle ^= 1;
                    mgametextpal(d,yy, ud.config.VoiceToggle&1? "Yes" : "No", enabled?MENUHIGHLIGHT(io):DISABLEDMENUSHADE, enabled?0:1);
                    break;
                case 9:
                    enabled = (ud.config.SoundToggle && ud.config.FXDevice >= 0);
                    i = j = (ud.config.VoiceToggle&4);
                    modval(0,1,(int32_t *)&i,1,enabled && probey==io);
                    if (x == io || j != i)
                        ud.config.VoiceToggle ^= 4;
                    mgametextpal(d,yy, ud.config.VoiceToggle&4? "Yes" : "No", enabled?MENUHIGHLIGHT(io):DISABLEDMENUSHADE, enabled?0:1);
                    break;
                case 10:
                    enabled = (ud.config.SoundToggle && ud.config.FXDevice >= 0);
                    modval(0,1,(int32_t *)&ud.config.AmbienceToggle,1,enabled && probey==io);
                    if (enabled && x == io)
                        ud.config.AmbienceToggle = 1-ud.config.AmbienceToggle;
                    mgametextpal(d,yy, ud.config.AmbienceToggle? "Yes" : "No", enabled?MENUHIGHLIGHT(io):DISABLEDMENUSHADE, enabled?0:1);
                    break;
                case 11:
                    enabled = (ud.config.SoundToggle && ud.config.FXDevice >= 0);
                    modval(0,1,(int32_t *)&ud.config.ReverseStereo,1,enabled && probey==io);
                    if (enabled && x == io)
                        ud.config.ReverseStereo = 1-ud.config.ReverseStereo;
                    mgametextpal(d,yy, ud.config.ReverseStereo? "Yes" : "No", enabled?MENUHIGHLIGHT(io):DISABLEDMENUSHADE, enabled?0:1);
                    break;

                default:
                    break;
                }
                mgametextpal(margin,yy, opts[ii], enabled?MENUHIGHLIGHT(io):DISABLEDMENUSHADE, enabled?10:1);
                io++;
                yy += 8;

            }
        }
        break;

    case 350:
        M_ChangeMenu(351);
        g_screenCapture = 1;
        G_DrawRooms(myconnectindex,65536);
        g_screenCapture = 0;
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
    case MENU_LOAD:

        margin = MENU_MARGIN_CENTER;
        M_DrawBackground();

        M_DrawTopBar(g_currentMenu == MENU_LOAD ? "Load Game" : "Save Game");

        if (g_currentMenu >= 360 && g_currentMenu <= 369)
        {
            static uint32_t crc = 0;

            if (!crc) crc = crc32once((uint8_t *)&ud.savegame[g_currentMenu-360][0], 19);

            Bsprintf(tempbuf,"Players: %-2d                      ",ud.multimode);
            mgametext(160,156,tempbuf,0,2+8+16);
            Bsprintf(tempbuf,"Episode: %-2d / Level: %-2d / Skill: %-2d",1+ud.volume_number,1+ud.level_number,ud.player_skill);
            mgametext(160,168,tempbuf,0,2+8+16);
            if (ud.volume_number == 0 && ud.level_number == 7)
                mgametext(160,180,currentboardfilename,0,2+8+16);

            x = Menu_EnterText((320>>1),184,&ud.savegame[g_currentMenu-360][0],20, 999);

            if (x == -1)
            {
                crc = 0;
                ReadSaveGameHeaders();
                M_ChangeMenu(351);
                goto DISPLAYNAMES;
            }

            if (x == 1)
            {
                // dirty hack... char 127 in last position indicates an auto-filled name
                if (ud.savegame[g_currentMenu-360][0] == 0 || (ud.savegame[g_currentMenu-360][20] == 127 &&
                    crc == crc32once((uint8_t *)&ud.savegame[g_currentMenu-360][0], 19)))
                {
                    Bstrncpy(&ud.savegame[g_currentMenu-360][0], MapInfo[ud.volume_number * MAXLEVELS + ud.level_number].name, 19);
                    ud.savegame[g_currentMenu-360][20] = 127;
                }

                G_SavePlayerMaybeMulti(g_currentMenu-360);

                g_lastSaveSlot = g_currentMenu-360;
                g_player[myconnectindex].ps->gm = MODE_GAME;

                if ((!g_netServer && ud.multimode < 2)  && ud.recstat != 2)
                {
                    ready2send = 1;
                    totalclock = ototalclock;
                }
                crc = 0;
            }

            rotatesprite_fs(101<<16,97<<16,65536>>1,512,TILE_SAVESHOT,-32,0,2+4+8+64);
            M_DisplaySaveGameList();
            rotatesprite_fs((margin+67+strlen(&ud.savegame[g_currentMenu-360][0])*4)<<16,(50+12*probey)<<16,32768L-10240,0,SPINNINGNUKEICON+(((totalclock)>>3)%7),0,0,10);
            break;
        }

        last_threehundred = probey;

        x = M_Probe(margin+68,54,12,10);

        if (g_currentMenu == MENU_LOAD)
        {
            // load game
            if (ud.savegame[probey][0])
            {
                Menus_LoadSave_DisplayCommon1();

                Bsprintf(tempbuf,"Players: %-2d                      ", savehead.numplayers);
                mgametext(160,156,tempbuf,0,2+8+16);
                Bsprintf(tempbuf,"Episode: %-2d / Level: %-2d / Skill: %-2d",
                         1+savehead.volnum, 1+savehead.levnum, savehead.skill);
                mgametext(160,168,tempbuf,0,2+8+16);
                if (savehead.volnum == 0 && savehead.levnum == 7)
                    mgametext(160,180,savehead.boardfn,0,2+8+16);
            }
            else
            {
                menutext(69,70,0,0,"Empty");
            }
        }
        else
        {
            // save game
            if (ud.savegame[probey][0])
            {
                Menus_LoadSave_DisplayCommon1();
            }
            else menutext(69,70,0,0,"Empty");

            Bsprintf(tempbuf,"Players: %-2d                      ",ud.multimode);
            mgametext(160,156,tempbuf,0,2+8+16);
            Bsprintf(tempbuf,"Episode: %-2d / Level: %-2d / Skill: %-2d",1+ud.volume_number,1+ud.level_number,ud.player_skill);
            mgametext(160,168,tempbuf,0,2+8+16);
            if (ud.volume_number == 0 && ud.level_number == 7)
                mgametext(160,180,currentboardfilename,0,2+8+16);
        }

        switch (x)
        {
        case -1:
            if (g_currentMenu == MENU_LOAD)
            {
                if ((g_player[myconnectindex].ps->gm&MODE_GAME) != MODE_GAME)
                {
                    M_ChangeMenu(MENU_MAIN);
                    break;
                }
                else
                    g_player[myconnectindex].ps->gm &= ~MODE_MENU;
            }
            else
                g_player[myconnectindex].ps->gm = MODE_GAME;

            if ((!g_netServer && ud.multimode < 2) && ud.recstat != 2)
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
            if (g_currentMenu == MENU_LOAD)
            {
                if (ud.savegame[x][0] && !g_oldverSavegame[x])
                    M_ChangeMenu(1000+x);
            }
            else
            {
                if (ud.savegame[x][0] != 0)
                    M_ChangeMenu(2000+x);
                else
                {
                    KB_FlushKeyboardQueue();
                    M_ChangeMenu(360+x);
                    ud.savegame[x][0] = 0;
                    inputloc = 0;
                }
            }
            break;
        }

DISPLAYNAMES:
        M_DisplaySaveGameList();
        break;

    case MENU_STORY:
    case MENU_F1HELP:
        if (VOLUMEALL) goto VOLUME_ALL_40x;
    case 402:
    case 403:

        margin = MENU_MARGIN_CENTER;

        M_LinearPanels(MENU_STORY, 403);

        x = M_Probe(0,0,0,1);

        if (x == -1)
        {
            if (g_player[myconnectindex].ps->gm&MODE_GAME)
                M_ChangeMenu(MENU_MAIN_INGAME);
            else M_ChangeMenu(MENU_MAIN);
            return;
        }

        flushperms();
        rotatesprite_fs(0,0,65536L,0,ORDERING+g_currentMenu-MENU_STORY,0,0,10+16+64);

        break;
VOLUME_ALL_40x:

        margin = MENU_MARGIN_CENTER;

        M_LinearPanels(MENU_STORY, MENU_F1HELP);

        x = M_Probe(0,0,0,1);

        if (x == -1)
        {
            if (g_player[myconnectindex].ps->gm&MODE_GAME)
            {
                switch (g_currentMenu)
                {
                case MENU_STORY:
                case MENU_F1HELP:
                    g_player[myconnectindex].ps->gm = MODE_GAME;
                    if ((!g_netServer && ud.multimode < 2)  && ud.recstat != 2)
                    {
                        ready2send = 1;
                        totalclock = ototalclock;
                    }
                    break;
                default:
                    M_ChangeMenu(MENU_MAIN_INGAME);
                    break;
                }
            }
            else M_ChangeMenu(MENU_MAIN);
            return;
        }

        flushperms();
        switch (g_currentMenu)
        {
        case MENU_STORY:
            rotatesprite_fs(0,0,65536L,0,TEXTSTORY,0,0,10+16+64);
            break;
        case MENU_F1HELP:
            rotatesprite_fs(0,0,65536L,0,F1HELP,0,0,10+16+64);
            break;
        }

        break;

    case MENU_QUIT:
    case MENU_QUIT2:
        margin = MENU_MARGIN_CENTER;

        mgametext(margin,90,"Are you sure you want to quit?",0,2+8+16);
        mgametext(margin,99,"(Y/N)",0,2+8+16);

        x = M_Probe(186,124,0,1);

        if (ProbeTriggers(AdvanceTrigger) || I_AdvanceTrigger() || KB_KeyPressed(sc_Y))
        {
            I_AdvanceTriggerClear();
            ProbeTriggersClear(AdvanceTrigger);
            KB_ClearKeyDown(sc_Y);
            KB_FlushKeyboardQueue();

            G_GameQuit();
        }
        if (x == -1 || KB_KeyPressed(sc_N))
        {
            KB_ClearKeyDown(sc_N);
            g_quitDeadline = 0;
            if (g_player[myconnectindex].ps->gm &MODE_DEMO && ud.recstat == 2)
                g_player[myconnectindex].ps->gm = MODE_DEMO;
            else
            {
                if (g_currentMenu == MENU_QUIT2)
                {
                    M_ChangeMenu(last_menu);
                    probey = last_menu_pos;
                }
                else if (!(g_player[myconnectindex].ps->gm &MODE_GAME || ud.recstat == 2))
                    M_ChangeMenu(MENU_MAIN);
                else g_player[myconnectindex].ps->gm &= ~MODE_MENU;
                if ((!g_netServer && ud.multimode < 2)  && ud.recstat != 2)
                {
                    ready2send = 1;
                    totalclock = ototalclock;
                }
            }
        }

        break;
    case MENU_QUITTOTITLE:
        margin = MENU_MARGIN_CENTER;
        mgametext(margin,90,"Quit to Title?",0,2+8+16);
        mgametext(margin,99,"(Y/N)",0,2+8+16);

        x = M_Probe(186,124,0,0);

        if (ProbeTriggers(AdvanceTrigger) || I_AdvanceTrigger() || KB_KeyPressed(sc_Y))
        {
            I_AdvanceTriggerClear();
            ProbeTriggersClear(AdvanceTrigger);
            KB_ClearKeyDown(sc_Y);

            KB_FlushKeyboardQueue();
            g_player[myconnectindex].ps->gm = MODE_DEMO;
            if (ud.recstat == 1)
                G_CloseDemoWrite();
            M_ChangeMenu(MENU_MAIN);
        }

        if (x == -1 || KB_KeyPressed(sc_N))
        {
            g_player[myconnectindex].ps->gm &= ~MODE_MENU;
            if ((!g_netServer && ud.multimode < 2)  && ud.recstat != 2)
            {
                ready2send = 1;
                totalclock = ototalclock;
            }
        }

        break;

    case 601:
        G_DrawFrags();
        M_DrawTopBar(&g_player[myconnectindex].user_name[0]);

        Bsprintf(tempbuf,"Waiting for master");
        mgametext(160,50,tempbuf,0,2+8+16);
        mgametext(160,59,"to select level",0,2+8+16);

        if (I_EscapeTrigger())
        {
            I_EscapeTriggerClear();
            S_PlaySound(EXITMENUSOUND);
            M_ChangeMenu(MENU_MAIN);
        }
        break;

    case 602:
        if (menunamecnt == 0)
        {
            //        getfilenames("SUBD");
            fnlist_getnames(&fnlist, ".", "*.MAP", 0, 0);
            set_findhighs();

            if (menunamecnt == 0)
                M_ChangeMenu(600);
        }

    case 603:
    {
        x = M_Probe(186,124,0,0);

        if (voting != myconnectindex)
		{
            g_player[myconnectindex].ps->gm &= ~MODE_MENU;
		}
		else if (x == -1)
		{
			Net_SendMapVoteCancel(0);
            M_ChangeMenu(MENU_MAIN);
		}
		else
		{
            mgametext(160,90,"Waiting for votes",0,2);
		}

		break;
    }
    case 600:
        margin = (320>>1) - 120;
        if ((g_player[myconnectindex].ps->gm&MODE_GAME) != MODE_GAME)
            G_DrawFrags();
        M_DrawTopBar(&g_player[myconnectindex].user_name[0]);

        x = M_Probe(margin,57-8,16,8);

        modval(0,g_numGametypes-1,(int32_t *)&ud.m_coop,1,probey==0);
        if (!VOLUMEONE)
            modval(0,g_numVolumes-1,(int32_t *)&ud.m_volume_number,1,probey==1);

        i = ud.m_level_number;

        modval(0,ud.m_volume_number == 0?6+(boardfilename[0]!=0):MAXLEVELS-1,(int32_t *)&ud.m_level_number,1,probey==2);

        if ((GametypeFlags[ud.m_coop] & GAMETYPE_MARKEROPTION))
            modval(0,1,(int32_t *)&ud.m_marker,1,probey==4);
        if ((GametypeFlags[ud.m_coop] & (GAMETYPE_PLAYERSFRIENDLY|GAMETYPE_TDM)))
            modval(0,1,(int32_t *)&ud.m_ffire,1,probey==5);
        else modval(0,1,(int32_t *)&ud.m_noexits,1,probey==5);

        if (probey == 1)
            if (ud.m_volume_number == 0 && ud.m_level_number > 6+(boardfilename[0]!=0))
                ud.m_level_number = 0;

        while (MapInfo[(ud.m_volume_number*MAXLEVELS)+ud.m_level_number].name == NULL)
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
            if (g_player[myconnectindex].ps->gm&MODE_GAME) M_ChangeMenu(MENU_MAIN_INGAME);
            else M_ChangeMenu(MENU_MAIN);
            break;
        case 0:
            ud.m_coop++;
            if (ud.m_coop == g_numGametypes) ud.m_coop = 0;
            break;
        case 1:
            if (!VOLUMEONE)
            {
                ud.m_volume_number++;
                if (ud.m_volume_number == g_numVolumes) ud.m_volume_number = 0;
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
            if ((GametypeFlags[ud.m_coop] & GAMETYPE_MARKEROPTION))
                ud.m_marker = !ud.m_marker;
            break;

        case 5:
            if ((GametypeFlags[ud.m_coop] & (GAMETYPE_PLAYERSFRIENDLY|GAMETYPE_TDM)))
                ud.m_ffire = !ud.m_ffire;
            else ud.m_noexits = !ud.m_noexits;
            break;

        case 6:
            if (VOLUMEALL)
            {
                currentlist = 1;
                last_menu_pos = probey;
                M_ChangeMenu(MENU_USERMAP);
            }
            break;
        case 7:
            // master does whatever it wants
            if (g_netServer)
            {
                Net_FillNewGame(&pendingnewgame, 1);
                Net_StartNewGame();
                Net_SendNewGame(1, NULL);
                break;
            }
            if (voting == -1)
            {
                Net_SendMapVoteInitiate();
                M_ChangeMenu(603);
            }
            break;
        }

        margin += 40;

        //if(ud.m_coop==1) mgametext(c+70,57-7-9,"Cooperative Play",0,2+8+16);
        //else if(ud.m_coop==2) mgametext(c+70,57-7-9,"DukeMatch (No Spawn)",0,2+8+16);
        //else mgametext(c+70,57-7-9,"DukeMatch (Spawn)",0,2+8+16);
        mgametext(margin+70,57-7-9,GametypeNames[ud.m_coop],MENUHIGHLIGHT(0),2+8+16);
        if (VOLUMEONE)
        {
            mgametext(margin+70,57+16-7-9,EpisodeNames[ud.m_volume_number],MENUHIGHLIGHT(1),2+8+16);
        }
        else
        {
            mgametext(margin+70,57+16-7-9,EpisodeNames[ud.m_volume_number],MENUHIGHLIGHT(1),2+8+16);
        }

        mgametext(margin+70,57+16+16-7-9,&MapInfo[MAXLEVELS*ud.m_volume_number+ud.m_level_number].name[0],MENUHIGHLIGHT(2),2+8+16);

        mgametext(margin+70,57+16+16+16-7-9,ud.m_monsters_off == 0 || ud.m_player_skill > 0?SkillNames[ud.m_player_skill]:"None",MENUHIGHLIGHT(3),2+8+16);

        if (GametypeFlags[ud.m_coop] & GAMETYPE_MARKEROPTION)
            mgametext(margin+70,57+16+16+16+16-7-9,ud.m_marker?"On":"Off",MENUHIGHLIGHT(4),2+8+16);

        if (GametypeFlags[ud.m_coop] & (GAMETYPE_PLAYERSFRIENDLY|GAMETYPE_TDM))
            mgametext(margin+70,57+16+16+16+16+16-7-9,ud.m_ffire?"On":"Off",MENUHIGHLIGHT(5),2+8+16);
        else mgametext(margin+70,57+16+16+16+16+16-7-9,ud.m_noexits?"Off":"On",MENUHIGHLIGHT(5),2+8+16);

        margin -= 44;

        menutext(margin,57-9,MENUHIGHLIGHT(0),0,"Game Type");

        if (VOLUMEONE)
        {
            Bsprintf(tempbuf,"Episode %d",ud.m_volume_number+1);
            menutext(margin,57+16-9,MENUHIGHLIGHT(1),1,tempbuf);
        }
        else
        {
            Bsprintf(tempbuf,"Episode %d",ud.m_volume_number+1);
            menutext(margin,57+16-9,MENUHIGHLIGHT(1),0,tempbuf);
        }

        Bsprintf(tempbuf,"Level %d",ud.m_level_number+1);
        menutext(margin,57+16+16-9,MENUHIGHLIGHT(2),0,tempbuf);

        menutext(margin,57+16+16+16-9,MENUHIGHLIGHT(3),0,"Monsters");

        if (GametypeFlags[ud.m_coop] & GAMETYPE_MARKEROPTION)
            menutext(margin,57+16+16+16+16-9,MENUHIGHLIGHT(4),0,"Markers");
        else
            menutext(margin,57+16+16+16+16-9,MENUHIGHLIGHT(4),1,"Markers");

        if (GametypeFlags[ud.m_coop] & (GAMETYPE_PLAYERSFRIENDLY|GAMETYPE_TDM))
            menutext(margin,57+16+16+16+16+16-9,MENUHIGHLIGHT(5),0,"Fr. Fire");
        else menutext(margin,57+16+16+16+16+16-9,MENUHIGHLIGHT(5),0,"Map Exits");

        if (VOLUMEALL)
        {
            menutext(margin,57+16+16+16+16+16+16-9,MENUHIGHLIGHT(6),0,"User Map");
            if (boardfilename[0] != 0)
                mgametext(margin+70+44,57+16+16+16+16+16,boardfilename,MENUHIGHLIGHT(6),2+8+16);
        }
        else
        {
            menutext(margin,57+16+16+16+16+16+16-9,MENUHIGHLIGHT(6),1,"User Map");
        }

        menutext(margin,57+16+16+16+16+16+16+16-9,MENUHIGHLIGHT(7),voting!=-1,"Start Game");

        break;
    }
    if (G_HaveEvent(EVENT_DISPLAYMENUREST))
        VM_OnEvent(EVENT_DISPLAYMENUREST, g_player[screenpeek].ps->i, screenpeek, -1, 0);

    if (I_EscapeTrigger())
        I_EscapeTriggerClear();

    if ((g_player[myconnectindex].ps->gm&MODE_MENU) != MODE_MENU)
    {
        G_UpdateScreenArea();
        CAMERACLOCK = totalclock;
        CAMERADIST = 65536;
    }
}
