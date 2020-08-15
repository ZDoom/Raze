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
#include "ns.h"
#undef MAIN
#include "build.h"

#include "names2.h"
#include "panel.h"
#include "lists.h"
#include "game.h"
#include "pal.h"
#include "misc.h"
#include "menus.h"

#include "network.h"

BEGIN_SW_NS

#define PANEL_FONT_G 3636
#define PANEL_FONT_Y 3646
#define PANEL_FONT_R 3656

#define PANEL_SM_FONT_G 3601
#define PANEL_SM_FONT_Y 3613
#define PANEL_SM_FONT_R 3625

PANEL_SPRITEp pClearTextLineID(PLAYERp pp, short id, int y, short pri)
{
    PANEL_SPRITEp psp=NULL, next;

    TRAVERSE(&pp->PanelSpriteList, psp, next)
    {
        // early out
        if (psp->priority > pri)
            return NULL;

        if (psp->ID == id && psp->y == y && psp->priority == pri)
        {
            pSetSuicide(psp);
        }
    }

    return NULL;
}

// only call this from menu code - it does a pKillSprite
PANEL_SPRITEp pMenuClearTextLineID(PLAYERp pp, short id, int y, short pri)
{
    PANEL_SPRITEp psp=NULL, next;

    TRAVERSE(&pp->PanelSpriteList, psp, next)
    {
        // early out
        if (psp->priority > pri)
            return NULL;

        if (psp->ID == id && psp->y == y && psp->priority == pri)
        {
            pKillSprite(psp);
        }
    }

    return NULL;
}


void pClearTextLine(PLAYERp pp, int y)
{
    pClearTextLineID(pp, ID_TEXT, y, PRI_FRONT_MAX);
}

void StringTimer(PANEL_SPRITEp psp)
{
    if ((psp->kill_tics -= synctics) <= 0)
    {
        pKillSprite(psp);
        return;
    }
}

void PutStringTimer(PLAYERp pp, short x, short y, const char *string, short seconds)
{
    int ndx, offset;
    char c;
    PANEL_SPRITEp nsp;
    long kill_tics;
    short id, ac;
    PANEL_SPRITE_FUNCp func;


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

	// Temporarily disabled because this uses a feature of the panel system that needs to be removed before this can be refactored.
#if 0
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
#endif
}

void KillString(PLAYERp pp, short y)
{
    pClearTextLineID(pp, ID_TEXT, y, PRI_FRONT_MAX);
}

PANEL_SPRITEp pClearSpriteXY(PLAYERp pp, short x, short y)
{
    PANEL_SPRITEp psp=NULL, next;

    TRAVERSE(&pp->PanelSpriteList, psp, next)
    {
        if (psp->x == x && psp->y == y)
            pSetSuicide(psp);
    }

    return NULL;
}

PANEL_SPRITEp pClearSpriteID(PLAYERp pp, short id)
{
    PANEL_SPRITEp psp=NULL, next;

    TRAVERSE(&pp->PanelSpriteList, psp, next)
    {
        if (psp->ID == id)
            pSetSuicide(psp);
    }

    return NULL;
}



void
DisplayMiniBarSmString(short xs, short ys, short pal, const char *buffer)
{
    short size=4,x;
    const char *ptr;
    short pic;

#define FRAG_FIRST_ASCII ('!') //exclamation point
#define FRAG_FIRST_TILE 2930 //exclamation point

    for (ptr = buffer, x = xs; *ptr; ptr++, x += size)
    {
        if (*ptr == ' ')
            continue;

        ASSERT(*ptr >= '!' && *ptr <= '}');

        pic = FRAG_FIRST_TILE + (*ptr - FRAG_FIRST_ASCII);

        rotatesprite((int)x << 16, (int)ys << 16, (1 << 16), 0, pic, 0, pal,
                     ROTATE_SPRITE_SCREEN_CLIP | RS_TOPLEFT | RS_ALIGN_L,
                     0, 0, xdim - 1, ydim - 1);
    }
}


////////////////////////////////////////////////
// Measure the pixel width of a graphic string
////////////////////////////////////////////////
static char lg_xlat_num[] = { 0,1,2,3,4,5,6,7,8,9 };
#define FONT_LARGE_ALPHA 3706
#define FONT_LARGE_DIGIT 3732
#define MenuDrawFlags (ROTATE_SPRITE_SCREEN_CLIP)
#define MZ              65536
#define MENU_SHADE_DEFAULT 0
#define MENU_SHADE_INACTIVE 20

void MNU_MeasureStringLarge(const char* string, short* w, short* h)
{
    short ndx, width, height;
    char c;
    short pic;

    width = 0;
    height = *h;

    for (ndx = 0; (c = string[ndx]) != 0; ndx++)
    {
        if (isalpha(c))
        {
            c = toupper(c);
            pic = FONT_LARGE_ALPHA + (c - 'A');
        }
        else if (isdigit(c))
        {
            pic = FONT_LARGE_DIGIT + lg_xlat_num[(c - '0')];
        }
        else if (c == ' ')
        {
            width += 10;                 // Special case for space char
            continue;
        }
        else
        {
            continue;
        }

        width += tilesiz[pic].x + 1;
        if (height < tilesiz[pic].y)
            height = tilesiz[pic].y;
    }

    *w = width;
    *h = height;
}

////////////////////////////////////////////////
// Draw a string using a graphic font
////////////////////////////////////////////////
void MNU_DrawStringLarge(short x, short y, const char* string, int shade)
{
    int ndx, offset;
    char c;
    short pic;

    offset = x;

    for (ndx = 0; (c = string[ndx]) != 0; ndx++)
    {
        if (isalpha(c))
        {
            c = toupper(c);
            pic = FONT_LARGE_ALPHA + (c - 'A');
        }
        else if (isdigit(c))
        {
            pic = FONT_LARGE_DIGIT + lg_xlat_num[(c - '0')];
        }
        else if (c == ' ')
        {
            offset += 10;
            continue;
        }
        else
        {
            continue;
        }

        rotatesprite(offset << 16, y << 16, MZ, 0, pic, shade, 0, MenuDrawFlags | RS_TOPLEFT, 0, 0, xdim - 1, ydim - 1);
        offset += tilesiz[pic].x + 1;
    }

}


////////////////////////////////////////////////
// Measure the pixel width of a graphic string
////////////////////////////////////////////////
void MNU_MeasureString(const char* string, short* w, short* h)
{
    short ndx, width, height;
    char c;
    short ac;

    if (string[0] == '^')
    {
        MNU_MeasureStringLarge(&string[1], w, h);
        return;
    }

    width = 0;
    height = *h;

    for (ndx = 0; (c = string[ndx]) != 0; ndx++)
    {
        ac = c - '!' + STARTALPHANUM;
        if ((ac < STARTALPHANUM || ac > ENDALPHANUM) && c != asc_Space)
            break;

        if (c > asc_Space && c < 127)
        {
            width += tilesiz[ac].x;
            if (height < tilesiz[ac].y)
                height = tilesiz[ac].y;
        }
        else if (c == asc_Space)
            width += 4;                 // Special case for space char
    }

    *w = width;
    *h = height;
}

////////////////////////////////////////////////
// Draw a string using a graphic font
//
// MenuTextShade and MenuDrawFlags
////////////////////////////////////////////////
void MNU_DrawString(short x, short y, const char* string, short shade, short pal, int align)
{
    int ndx, offset;
    char c;
    short ac;

    if (string[0] == '^')
    {
        MNU_DrawStringLarge(x, y, &string[1]);
        return;
    }

    if (align > -1)
    {
        short w, h;
        MNU_MeasureString(string, &w, &h);
        if (align == 0) x -= w / 2;
        else x -= w;
    }

    offset = x;

    for (ndx = 0; (c = string[ndx]) != 0; ndx++)
    {
        ac = c - '!' + STARTALPHANUM;
        if ((ac < STARTALPHANUM || ac > ENDALPHANUM) && c != asc_Space)
            break;

        if (c > asc_Space && c < 127)
        {
            rotatesprite(offset << 16, y << 16, MZ, 0, ac, shade, pal, MenuDrawFlags, 0, 0, xdim - 1, ydim - 1);
            offset += tilesiz[ac].x;
        }
        else if (c == asc_Space)
            offset += 4;                // Special case for space char
    }

}

////////////////////////////////////////////////
// Measure the pixel width of a small font string
////////////////////////////////////////////////
void MNU_MeasureSmallString(const char* string, short* w, short* h)
{
    short ndx, width, height;
    char c;
    short ac;

    width = 0;
    height = *h;

    for (ndx = 0; (c = string[ndx]) != 0; ndx++)
    {
        ac = (c - '!') + 2930;
        if ((ac < 2930 || ac > 3023) && c != asc_Space)
            break;

        if (c > asc_Space && c < 127)
        {
            width += tilesiz[ac].x;
            if (height < tilesiz[ac].y)
                height = tilesiz[ac].y;
        }
        else if (c == asc_Space)
            width += 4;                 // Special case for space char
    }

    *w = width;
    *h = height;
}

////////////////////////////////////////////////
// Draw a string using a small graphic font
////////////////////////////////////////////////
void MNU_DrawSmallString(short x, short y, const char* string, short shade, short pal)
{
    int ndx;
    char c;
    short ac, offset;


    offset = x;

    for (ndx = 0; (c = string[ndx]) != 0; ndx++)
    {
        ac = c - '!' + 2930;
        if ((ac < 2930 || ac > 3023) && c != asc_Space)
            break;

        if (c > asc_Space && c < 127)
        {
            rotatesprite(offset << 16, y << 16, MZ, 0, ac, shade, pal, MenuDrawFlags, 0, 0, xdim - 1, ydim - 1);

            offset += tilesiz[ac].x;

        }
        else if (c == asc_Space)
        {
            offset += 4;                // Special case for space char
        }
    }

}




short GlobInfoStringTime = TEXT_INFO_TIME;
void PutStringInfo(PLAYERp pp, const char *string)
{
    if (pp-Player != myconnectindex)
        return;

    if (!hud_messages)
        return;

    Printf(PRINT_MEDIUM|PRINT_NOTIFY, "%s\n", string); // Put it in the console too
    if (hud_messages == 1) PutStringInfoLine(pp, string);
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

void pMenuClearTextLine(PLAYERp pp)
{
    pMenuClearTextLineID(pp, ID_TEXT, TEXT_INFO_LINE(0), PRI_FRONT_MAX);
    pMenuClearTextLineID(pp, ID_TEXT, TEXT_INFO_LINE(1), PRI_FRONT_MAX);
}

#define TEXT_PLAYER_INFO_TIME (3)
#define TEXT_PLAYER_INFO_Y (200 - 40)

#include "saveable.h"

static saveable_code saveable_text_code[] =
{
    SAVE_CODE(StringTimer),
};

saveable_module saveable_text =
{
    // code
    saveable_text_code,
    SIZ(saveable_text_code),

    // data
    NULL,0
};

END_SW_NS
