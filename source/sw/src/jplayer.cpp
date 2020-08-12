//-------------------------------------------------------------------------
/*
Copyright (C) 1997, 2005 - 3D Realms Entertainment

This file is part of Shadow Warrior version 1.2

Shadow Warrior is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.

Original Source: 1997 - Frank Maddin and Jim Norwood
Prepared for public release: 03/28/2005 - Charlie Wiederhold, 3D Realms
*/
//-------------------------------------------------------------------------

// JPLAYER.C
// Copyright (c) 1996 by Jim Norwood
#include "ns.h"

#include "build.h"

#include "mytypes.h"
#include "names2.h"
#include "panel.h"
#include "game.h"
#include "tags.h"
#include "player.h"
#include "lists.h"
#include "quake.h"

#include "gamecontrol.h"

#include "menus.h"
#include "network.h"
#include "pal.h"

#include "bots.h"

BEGIN_SW_NS

SWBOOL WeaponOK(PLAYERp pp);

#define MAXANGVEL 80

// From build.h
#define CLIPMASK0 (((1L)<<16)+1L)
#define CLIPMASK1 (((256L)<<16)+64L)


// PLAYER QUOTES TO OTHER PLAYERS ////////////////////////////////////////////////////////////

#define STARTALPHANUM 4608  // New SW font for typing in stuff, It's in ASCII order.
#define ENDALPHANUM   4701
#define MINIFONT      2930  // Start of small font, it's blue for good palette swapping

#define NUMPAGES 1
#define NUMOFFIRSTTIMEACTIVE 100  // You can save up to 100 strings in the message history queue

char pus, pub;  // Global text vars
char fta_quotes[NUMOFFIRSTTIMEACTIVE][64];


int gametext(int x,int y,char *t,char s,short dabits)
{
    short ac,newx;
    char centre, *oldt;

    centre = (x == (320>>1));
    newx = 0;
    oldt = t;

    if (centre)
    {
        while (*t)
        {
            if (*t == 32) {newx+=5; t++; continue; }
            else ac = *t - '!' + STARTALPHANUM;

            if (ac < STARTALPHANUM || ac > ENDALPHANUM) break;

            if (*t >= '0' && *t <= '9')
                newx += 8;
            else newx += tilesiz[ac].x;
            t++;
        }

        t = oldt;
        x = (320>>1)-(newx>>1);
    }

    while (*t)
    {
        if (*t == 32) {x+=5; t++; continue; }
        else ac = *t - '!' + STARTALPHANUM;

        if (ac < STARTALPHANUM || ac > ENDALPHANUM)
            break;

        rotatesprite(x<<16,y<<16,65536L,0,ac,s,16,dabits,0,0,xdim-1,ydim-1);

        if (*t >= '0' && *t <= '9')
            x += 8;
        else x += tilesiz[ac].x;

        t++;
    }

    return x;
}

int minigametext(int x,int y,const char *t,short dabits)
{
    short ac,newx;
    char centre;
    const char *oldt;

    centre = (x == (320>>1));
    newx = 0;
    oldt = t;

    if (centre)
    {
        while (*t)
        {
            if (*t == 32) {newx+=4; t++; continue; }
            else ac = *t - '!' + 2930;

            if (*t > asc_Space && *t < 127)
            {
                newx += tilesiz[ac].x;
            }
            else
                x += 4;

            t++;
        }

        t = oldt;
        x = (320>>1)-(newx>>1);
    }

    while (*t)
    {
        if (*t == 32) {x+=4; t++; continue; }
        else ac = *t - '!' + 2930;

        if (*t > asc_Space && *t < 127)
        {
            rotatesprite(x<<16,y<<16,65536L,0,ac,-128,17,dabits,0,0,xdim-1,ydim-1);
            x += tilesiz[ac].x;
        }
        else
            x += 4;

        t++;
    }

    return x;
}

int minitext(int x,int y,char *t,char p,char sb)
{
    short ac;

    while (*t)
    {
        *t = toupper(*t);
        if (*t == 32) {x+=5; t++; continue; }
        else ac = *t - '!' + MINIFONT;

        rotatesprite(x<<16,y<<16,65536L,0,ac,0,p,sb,0,0,xdim-1,ydim-1);
        x += 4; // tilesiz[ac].x+1;

        t++;
    }
    return x;
}

int minitextshade(int x,int y,char *t,char s,char p,char sb)
{
    short ac;

    while (*t)
    {
        *t = toupper(*t);
        if (*t == 32) {x+=5; t++; continue; }
        else ac = *t - '!' + MINIFONT;

        rotatesprite(x<<16,y<<16,65536L,0,ac,s,p,sb,0,0,xdim-1,ydim-1);
        x += 4; // tilesiz[ac].x+1;

        t++;
    }
    return x;
}

int quotebot, quotebotgoal;
short user_quote_time[MAXUSERQUOTES];
char user_quote[MAXUSERQUOTES][256];

void adduserquote(const char *daquote)
{
    int i;

    SetRedrawScreen(Player+myconnectindex);

    for (i=MAXUSERQUOTES-1; i>0; i--)
    {
        strcpy(user_quote[i],user_quote[i-1]);
        user_quote_time[i] = user_quote_time[i-1];
    }
    strcpy(user_quote[0],daquote);
    user_quote_time[0] = 180;
}

void operatefta(void)
{
    int i, j, k;

    j=MESSAGE_LINE; // Base line position on screen
    quotebot = min(quotebot,j);
    quotebotgoal = min(quotebotgoal,j);
    if (MessageInputMode)
        j -= 6; // Bump all lines up one to make room for new line
    quotebotgoal = j;
    j = quotebot;

    for (i=0; i<MAXUSERQUOTES; i++)
    {
        k = user_quote_time[i];
        if (k <= 0)
            break;

        if (gs.BorderNum <= BORDER_BAR+1)
        {
            // dont fade out
            if (k > 4)
                minigametext(320>>1,j,user_quote[i],2+8);
            else if (k > 2)
                minigametext(320>>1,j,user_quote[i],2+8+1);
            else
                minigametext(320>>1,j,user_quote[i],2+8+1+32);
        }
        else
        {
            // dont fade out
            minigametext(320>>1,j,user_quote[i],2+8);
        }

        j -= 6;
    }
}

//////////// Console Message Queue ////////////////////////////////////
int conbot, conbotgoal;
char con_quote[MAXCONQUOTES][256];

void addconquote(const char *daquote)
{
    int i;

    for (i=MAXCONQUOTES-1; i>0; i--)
    {
        strcpy(con_quote[i],con_quote[i-1]);
    }
    strcpy(con_quote[0],daquote);
}

#define CON_ROT_FLAGS (ROTATE_SPRITE_CORNER|ROTATE_SPRITE_SCREEN_CLIP|ROTATE_SPRITE_NON_MASK)
void operateconfta(void)
{
    int i, j;

    if (!ConPanel) return; // If panel isn't up, don't draw anything

    // Draw the background console pic
    rotatesprite((0)<<16,(0)<<16,65536L,0,5119,0,0,CON_ROT_FLAGS,0,0,xdim-1,ydim-1);

    j=99; // Base line position on screen
    conbot = min(conbot,j);
    conbotgoal = min(conbotgoal,j);
    if (ConInputMode) j -= 6; // Bump all lines up one to make room for new line
    conbotgoal = j; j = conbot;

    for (i=0; i<MAXCONQUOTES; i++)
    {
        MNU_DrawSmallString(27, j, con_quote[i], 0, 17); // 17 = white
        j -= 6;
    }
}

END_SW_NS
