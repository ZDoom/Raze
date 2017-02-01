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
#undef MAIN
#include "build.h"

#include "keys.h"
#include "names2.h"
#include "panel.h"
#include "lists.h"
#include "game.h"
#include "common_game.h"
#include "pal.h"
#include "text.h"
#include "menus.h"

#include "net.h"

#define PANEL_FONT_G 3636
#define PANEL_FONT_Y 3646
#define PANEL_FONT_R 3656

#define PANEL_SM_FONT_G 3601
#define PANEL_SM_FONT_Y 3613
#define PANEL_SM_FONT_R 3625

char *KeyDoorMessage[MAX_KEYS] =
{
    "You need a RED key for this door.",
    "You need a BLUE key for this door.",
    "You need a GREEN key for this door.",
    "You need a YELLOW key for this door.",
    "You need a GOLD key for this door.",
    "You need a SILVER key for this door.",
    "You need a BRONZE key for this door.",
    "You need a RED key for this door."
};

void
DisplaySummaryString(PLAYERp pp, short xs, short ys, short color, short shade, const char *buffer)
{
    short size,x;
    const char *ptr;
    char ch;
    PANEL_SPRITEp nsp;
    short font_pic;
    static short font_base[] = {PANEL_SM_FONT_G, PANEL_SM_FONT_Y, PANEL_SM_FONT_R};

    for (ptr = buffer, x = xs; *ptr; ptr++, x += size)
    {
        ch = *ptr;
        if (ch == ' ')
        {
            size = 4;
            continue;
        }

        switch (ch)
        {
        case '\\':
            ch = '0' - 1; // one pic before 0
            break;
        case ':':
            ch = '9' + 1; // one pic after nine
            break;
        }

        ASSERT(color < 3);
        font_pic = font_base[color] + (ch - '0');
        nsp = pSpawnFullScreenSprite(pp, font_pic, PRI_FRONT_MAX, x, ys);
        nsp->shade = shade;
        size = tilesiz[font_pic].x + 1;
    }
}

PANEL_SPRITEp
pClearTextLineID(PLAYERp pp, short id, long y, short pri)
{
    PANEL_SPRITEp nsp=NULL;
    PANEL_SPRITEp psp=NULL, next;

    TRAVERSE(&pp->PanelSpriteList, psp, next)
    {
        // early out
        if (psp->priority > pri)
            return NULL;

        if (psp->ID == id && psp->y == y && psp->priority == pri)
        {
            SetRedrawScreen(psp->PlayerP);
            //SET(psp->flags, PANF_INVISIBLE);
            pSetSuicide(psp);
        }
    }

    return NULL;
}

// only call this from menu code - it does a pKillSprite
PANEL_SPRITEp
pMenuClearTextLineID(PLAYERp pp, short id, long y, short pri)
{
    PANEL_SPRITEp nsp=NULL;
    PANEL_SPRITEp psp=NULL, next;

    TRAVERSE(&pp->PanelSpriteList, psp, next)
    {
        // early out
        if (psp->priority > pri)
            return NULL;

        if (psp->ID == id && psp->y == y && psp->priority == pri)
        {
            SetRedrawScreen(psp->PlayerP);
            pKillSprite(psp);
            //pSetSuicide(psp);
        }
    }

    return NULL;
}


void
pClearTextLine(PLAYERp pp, long y)
{
    SetRedrawScreen(pp);
    pClearTextLineID(pp, ID_TEXT, y, PRI_FRONT_MAX);
}

void
StringTimer(PANEL_SPRITEp psp)
{
    if ((psp->kill_tics -= synctics) <= 0)
    {
        SetRedrawScreen(psp->PlayerP);
        //pSetSuicide(psp);  // did not work here
        pKillSprite(psp);
        return;
    }
}

void
PutStringTimer(PLAYERp pp, short x, short y, const char *string, short seconds)
{
    int ndx, offset;
    char c;
    PANEL_SPRITEp nsp;
    extern unsigned short xlatfont[];
    long kill_tics;
    short id, ac;
    void *func;


    offset = x;

    if (seconds == 999)
    {
        pClearTextLineID(pp, ID_TEXT, y, PRI_FRONT_MAX);
        func = NULL;
        kill_tics = 0;
        id = ID_TEXT;
    }
    else
    {
        pClearTextLineID(pp, ID_TEXT, y, PRI_FRONT_MAX);
        func = StringTimer;
        kill_tics = seconds * 120;
        id = ID_TEXT;
    }

    for (ndx = 0; (c = string[ndx]) != 0; ndx++)
    {
        ac = c - '!' + STARTALPHANUM;
        if ((ac < STARTALPHANUM || ac > ENDALPHANUM)  && c != asc_Space)
            break;

        if (c > asc_Space && c < 127)
        {
            nsp = pSpawnFullViewSprite(pp, ac, PRI_FRONT_MAX, offset, y);
            nsp->PanelSpriteFunc = func;
            nsp->kill_tics = kill_tics;
            nsp->ID = id;
            offset += tilesiz[ac].x;
        }
        else if (c == asc_Space)
            offset += 4;                // Special case for space char
    }
}

void
KillString(PLAYERp pp, short y)
{
    pClearTextLineID(pp, ID_TEXT, y, PRI_FRONT_MAX);
}

PANEL_SPRITEp
pClearSpriteXY(PLAYERp pp, short x, short y)
{
    PANEL_SPRITEp nsp=NULL;
    PANEL_SPRITEp psp=NULL, next;

    TRAVERSE(&pp->PanelSpriteList, psp, next)
    {
        if (psp->x == x && psp->y == y)
            pSetSuicide(psp);
    }

    return NULL;
}

PANEL_SPRITEp
pClearSpriteID(PLAYERp pp, short id)
{
    PANEL_SPRITEp nsp=NULL;
    PANEL_SPRITEp psp=NULL, next;

    TRAVERSE(&pp->PanelSpriteList, psp, next)
    {
        if (psp->ID == id)
            pSetSuicide(psp);
    }

    return NULL;
}


void
DisplayPanelNumber(PLAYERp pp, short xs, short ys, int number)
{
    char buffer[32];
    char *ptr;
    short x, size;

    sprintf(buffer, "%03d", number);

    for (ptr = buffer, x = xs; *ptr; ptr++, x += size)
    {
        if (!isdigit(*ptr))
        {
            size = 0;
            continue;
        }

        pSpawnFullScreenSprite(pp, PANEL_FONT_G + (*ptr - '0'), PRI_FRONT_MAX, x, ys);

        size = tilesiz[PANEL_FONT_G + (*ptr - '0')].x + 1;
    }
}

void
DisplayMiniBarNumber(PLAYERp pp, short xs, short ys, int number)
{
    char buffer[32];
    char *ptr;
    short x, size;
    short pic;

    sprintf(buffer, "%03d", number);

    for (ptr = buffer, x = xs; *ptr; ptr++, x += size)
    {
        if (!isdigit(*ptr))
        {
            size = 0;
            continue;
        }

        pic = PANEL_FONT_G + (*ptr - '0');

        rotatesprite((long)x << 16, (long)ys << 16, (1 << 16), 0,
                     pic, 0, 0,
                     ROTATE_SPRITE_SCREEN_CLIP | ROTATE_SPRITE_CORNER, 0, 0, xdim - 1, ydim - 1);

        size = tilesiz[PANEL_FONT_G + (*ptr - '0')].x + 1;
    }
}

void
DisplayMiniBarSmString(PLAYERp pp, short xs, short ys, short pal, const char *buffer)
{
    short size=4,x;
    const char *ptr;
    PANEL_SPRITEp nsp;
    short pic;

#define FRAG_FIRST_ASCII ('!') //exclamation point
#define FRAG_FIRST_TILE 2930 //exclamation point

    for (ptr = buffer, x = xs; *ptr; ptr++, x += size)
    {
        if (*ptr == ' ')
            continue;

        ASSERT(*ptr >= '!' && *ptr <= '}');

        pic = FRAG_FIRST_TILE + (*ptr - FRAG_FIRST_ASCII);

        rotatesprite((long)x << 16, (long)ys << 16, (1 << 16), 0,
                     pic, 0, pal,
                     ROTATE_SPRITE_SCREEN_CLIP | ROTATE_SPRITE_CORNER, 0, 0, xdim - 1, ydim - 1);
    }
}

void
DisplaySmString(PLAYERp pp, short xs, short ys, short pal, const char *buffer)
{
    short size=4,x;
    const char *ptr;
    PANEL_SPRITEp nsp;
    // ID is base + (0-3)
    //short id = ID_TEXT + MOD4(pp->pnum);

#define FRAG_FIRST_ASCII ('!') //exclamation point
#define FRAG_FIRST_TILE 2930 //exclamation point

    for (ptr = buffer, x = xs; *ptr; ptr++, x += size)
    {
        if (*ptr == ' ')
            continue;

        ASSERT(*ptr >= '!' && *ptr <= '}');

        nsp = pSpawnFullScreenSprite(pp, FRAG_FIRST_TILE + (*ptr - FRAG_FIRST_ASCII), PRI_FRONT_MAX, x, ys);
        nsp->pal = pal;
        //nsp->ID = id;
    }
}

void
DisplayFragString(PLAYERp pp, short xs, short ys, const char *buffer)
{
    short size=4,x;
    const char *ptr;
    PANEL_SPRITEp nsp;
    // ID is base + (0-3)
    short id = ID_TEXT + MOD4(pp->pnum);

    PLAYERp my_pp = Player + myconnectindex;

#define FRAG_FIRST_ASCII ('!') //exclamation point
#define FRAG_FIRST_TILE 2930 //exclamation point

    //pClearTextLineID(my_pp, id, ys, PRI_FRONT_MAX);

    for (ptr = buffer, x = xs; *ptr; ptr++, x += size)
    {
        if (*ptr == ' ')
            continue;

        ASSERT(*ptr >= '!' && *ptr <= '}');

        nsp = pSpawnFullScreenSprite(my_pp, FRAG_FIRST_TILE + (*ptr - FRAG_FIRST_ASCII), PRI_FRONT_MAX, x, ys);
        nsp->ID = id;
        //nsp->pal = PALETTE_PLAYER0 + pp->TeamColor;
        //if (pp->SpriteP)
        nsp->pal = User[pp->SpriteP - sprite]->spal;
    }
}

void
DisplayFragNumbers(PLAYERp pp)
{
    char buffer[32];
    char *ptr;
    short x, xs, ys, size;
    short frag_bar;
    short pnum = pp - Player;

    static int xoffs[] =
    {
        69, 147, 225, 303
    };

    PLAYERp my_pp = Player + myconnectindex;

    // black tile to erase frag count
#define FRAG_ERASE_NAME 2375
#define FRAG_ERASE_NUMBER 2376
#define FRAG_YOFF 2

    //xs = FRAG_XOFF;
    ys = FRAG_YOFF;

    // frag bar 0 or 1
    frag_bar = ((pnum)/4);
    // move y down according to frag bar number
    ys = ys + (tilesiz[FRAG_BAR].y-2) * frag_bar;

    // move x over according to the number of players
    xs = xoffs[MOD4(pnum)];

    sprintf(buffer, "%03d", pp->Kills);

    // erase old kill count
    pSpawnFullScreenSprite(my_pp, FRAG_ERASE_NUMBER, PRI_MID+1, xs-1, ys);

    DisplayFragString(pp, xs, ys, buffer);
}

void
DisplayFragNames(PLAYERp pp)
{
    char *ptr;
    short x, xs, ys, size;
    short frag_bar;
    short pnum = pp - Player;

    static int xoffs[] =
    {
        7, 85, 163, 241
    };

    PLAYERp my_pp = Player + myconnectindex;

    //xs = FRAG_XOFF;
    ys = FRAG_YOFF;

    // frag bar 0 or 1
    frag_bar = ((pnum)/4);
    // move y down according to frag bar number
    ys = ys + (tilesiz[FRAG_BAR].y-2) * frag_bar;

    // move x over according to the number of players
    xs = xoffs[MOD4(pnum)];

    // erase old kill count
    pSpawnFullScreenSprite(my_pp, FRAG_ERASE_NAME, PRI_MID+1, xs-1, ys);

    DisplayFragString(pp, xs, ys, pp->PlayerName);
}

short GlobInfoStringTime = TEXT_INFO_TIME;
void PutStringInfo(PLAYERp pp, const char *string)
{
    if (pp-Player != myconnectindex)
        return;

    if (!gs.Messages)
        return;

    CON_ConMessage(string); // Put it in the console too
    PutStringInfoLine(pp, string);
}

void PutStringInfoLine(PLAYERp pp, const char *string)
{
    short x,y;
    short w,h;

    if (pp-Player != myconnectindex)
        return;

    MNU_MeasureString(string, &w, &h);

    x = TEXT_XCENTER(w);
    y = TEXT_INFO_LINE(0);

    // Move lower on this level because of boss meters
    //if ((Level == 20 && numplayers > 1) || numplayers > 4)
    //    y += 20;
    //if (numplayers > 1 && numplayers <= 4)
    //    y+= 10;

    PutStringTimer(pp, x, y, string, GlobInfoStringTime);
    // when printing info line clear the second line
    //PutStringInfoLine2(pp, "");
}

void PutStringInfoLine2(PLAYERp pp, const char *string)
{
    short x,y;
    short w,h;

    if (pp-Player != myconnectindex)
        return;

    MNU_MeasureString(string, &w, &h);

    x = TEXT_XCENTER(w);
    y = TEXT_INFO_LINE(1);

    PutStringTimer(pp, x, y, string, GlobInfoStringTime);
}

void
pMenuClearTextLine(PLAYERp pp)
{
    pMenuClearTextLineID(pp, ID_TEXT, TEXT_INFO_LINE(0), PRI_FRONT_MAX);
    pMenuClearTextLineID(pp, ID_TEXT, TEXT_INFO_LINE(1), PRI_FRONT_MAX);
}

#define TEXT_PLAYER_INFO_TIME (3)
#define TEXT_PLAYER_INFO_Y (200 - 40)

void PutStringPlayerInfo(PLAYERp pp, const char *string)
{
    short x,y;
    short w,h;

    if (pp-Player != myconnectindex)
        return;

    if (!gs.Messages)
        return;

    MNU_MeasureString(string, &w, &h);

    x = TEXT_XCENTER(w);
    y = TEXT_PLAYER_INFO_Y;

    PutStringTimer(pp, x, y, string, GlobInfoStringTime);
}
