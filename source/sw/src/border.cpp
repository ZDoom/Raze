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

#include "build.h"

#include "names2.h"
#include "panel.h"
#include "lists.h"
#include "game.h"
#include "network.h"
#include "misc.h"
#include "menus.h"

BEGIN_SW_NS



#define BAR_HEIGHT 48
#define XDIM 320
#define YDIM 200

short DebugBorderShade = 0;

#define f_320 FIXED(320,0)
#define f_200 FIXED(200,0)

#define X_TO_FIXED(val) (x_aspect_mul*(val))
#define Y_TO_FIXED(val) (y_aspect_mul*(val))

BORDER_INFO BorderInfoValues[] =
{
    // x,y,screensize
    {0, 0, 0},
    {0, 0, 0},
    {0, BAR_HEIGHT, 0},

    {0, BAR_HEIGHT, (1 * 16)},
    {0, BAR_HEIGHT, (2 * 16)},
    {0, BAR_HEIGHT, (3 * 16)},
    {0, BAR_HEIGHT, (4 * 16)},
    {0, BAR_HEIGHT, (5 * 16)},
    {0, BAR_HEIGHT, (6 * 16)},
    {0, BAR_HEIGHT, (7 * 16)},
    {0, BAR_HEIGHT, (8 * 16)},
    {0, BAR_HEIGHT, (9 * 16)},
    {0, BAR_HEIGHT, (10 * 16)},
    {0, BAR_HEIGHT, (11 * 16)},
    {0, BAR_HEIGHT, (12 * 16)}
};

static void BorderSetView(PLAYERp, int *Xdim, int *Ydim, int *ScreenSize)
{
    int x, x2, y, y2;
    const BORDER_INFO *b = &BorderInfoValues[gs.BorderNum];
    int f_xdim, f_ydim, x_pix_size, y_pix_size, x_aspect_mul, y_aspect_mul;

    f_xdim = FIXED(xdim, 0);
    f_ydim = FIXED(ydim, 0);

    x_pix_size = (f_320 / xdim);
    y_pix_size = (f_200 / ydim);

    x_aspect_mul = (f_xdim / 320);
    y_aspect_mul = (f_ydim / 200);


    // figure out the viewing window x and y dimensions
    *Xdim = MSW(f_xdim - X_TO_FIXED(b->Xdim));
    *Ydim = MSW(f_ydim - Y_TO_FIXED(b->Ydim));
    *ScreenSize = MSW(f_xdim - X_TO_FIXED(b->ScreenSize));

    // figure out the viewing window x and y coordinates
    x = DIV2(*Xdim) - DIV2(*ScreenSize);
    x2 = x + *ScreenSize - 1;
    y = DIV2(*Ydim) - DIV2((*ScreenSize **Ydim) / *Xdim);
    y2 = y + ((*ScreenSize **Ydim) / *Xdim) - 1;

    // avoid a one-pixel tall HOM
    if (gs.BorderNum == BORDER_BAR)
        ++y2;

    // global windowxy1, windowxy2 coords set here
    videoSetViewableArea(x, y, x2, y2);
}

//
// Redraw the whole screen
//

void SetBorder(PLAYERp pp)
{
    int Xdim, Ydim, ScreenSize;

    if (pp != Player + myconnectindex)
        return;

    if (xdim == 0) return;  // game not set up yet.

    BorderSetView(pp, &Xdim, &Ydim, &ScreenSize);
}

END_SW_NS
