//-------------------------------------------------------------------------
/*
Copyright (C) 1996, 2003 - 3D Realms Entertainment
Copyright (C) 2017-2019 Nuke.YKT

This file is part of Duke Nukem 3D version 1.5 - Atomic Edition

Duke Nukem 3D is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

Original Source: 1996 - Todd Replogle
Prepared for public release: 03/21/2003 - Charlie Wiederhold, 3D Realms
*/
//-------------------------------------------------------------------------
#include "ns.h"
#include "duke3d.h"

BEGIN_DUKE_NS

int madenoise(int snum)
{
    player_struct *p;
    p = &ps[snum];
    p->donoise = 1;
    p->noise_x = p->posx;
    p->noise_y = p->posy;
    return 1;
}

int wakeup(int i, int snum)
{
    player_struct *p;
    int radius;
    p = &ps[snum];
    if (!p->donoise)
        return 0;
    if (sprite[i].pal == 30 || sprite[i].pal == 32 || sprite[i].pal == 33 || (isRRRA() && sprite[i].pal == 8))
        return 0;

    radius = p->noise_radius;

    if (p->noise_x - radius < sprite[i].x && p->noise_x + radius > sprite[i].x
        && p->noise_y - radius < sprite[i].y && p->noise_y + radius > sprite[i].y)
        return 1;
    return 0;
}

END_DUKE_NS
