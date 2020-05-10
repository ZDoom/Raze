//-------------------------------------------------------------------------
/*
Copyright (C) 1996, 2003 - 3D Realms Entertainment
Copyright (C) 2000, 2003 - Matt Saettler (EDuke Enhancements)

This file is part of Enhanced Duke Nukem 3D version 1.5 - Atomic Edition

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

EDuke enhancements integrated: 04/13/2003 - Matt Saettler

Note: EDuke source was in transition.  Changes are in-progress in the
source as it is released.

*/
//-------------------------------------------------------------------------

#include "ns.h"
#include "global.h"
#include "sounds_common.h"
#include "names.h"

// PRIMITIVE
BEGIN_DUKE_NS

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

bool isadoorwall_d(int dapic)
{
    switch(dapic)
    {
        case DOORTILE1:
        case DOORTILE2:
        case DOORTILE3:
        case DOORTILE4:
        case DOORTILE5:
        case DOORTILE6:
        case DOORTILE7:
        case DOORTILE8:
        case DOORTILE9:
        case DOORTILE10:
        case DOORTILE11:
        case DOORTILE12:
        case DOORTILE14:
        case DOORTILE15:
        case DOORTILE16:
        case DOORTILE17:
        case DOORTILE18:
        case DOORTILE19:
        case DOORTILE20:
        case DOORTILE21:
        case DOORTILE22:
        case DOORTILE23:
            return 1;
    }
    return 0;
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void animatewalls_d(void)
{
    int i, j, p, t;

    for (p = 0; p < numanimwalls; p++)
    {
        i = animwall[p].wallnum;
        j = wall[i].picnum;

        switch (j)
        {
        case SCREENBREAK1:
        case SCREENBREAK2:
        case SCREENBREAK3:
        case SCREENBREAK4:
        case SCREENBREAK5:

        case SCREENBREAK9:
        case SCREENBREAK10:
        case SCREENBREAK11:
        case SCREENBREAK12:
        case SCREENBREAK13:
        case SCREENBREAK14:
        case SCREENBREAK15:
        case SCREENBREAK16:
        case SCREENBREAK17:
        case SCREENBREAK18:
        case SCREENBREAK19:

            if ((krand() & 255) < 16)
            {
                animwall[p].tag = wall[i].picnum;
                wall[i].picnum = SCREENBREAK6;
            }

            continue;

        case SCREENBREAK6:
        case SCREENBREAK7:
        case SCREENBREAK8:

            if (animwall[p].tag >= 0 && wall[i].extra != FEMPIC2 && wall[i].extra != FEMPIC3)
                wall[i].picnum = animwall[p].tag;
            else
            {
                wall[i].picnum++;
                if (wall[i].picnum == (SCREENBREAK6 + 3))
                    wall[i].picnum = SCREENBREAK6;
            }
            continue;

        }

        if (wall[i].cstat & 16)
            switch (wall[i].overpicnum)
            {
            case W_FORCEFIELD:
            case W_FORCEFIELD + 1:
            case W_FORCEFIELD + 2:

                t = animwall[p].tag;

                if (wall[i].cstat & 254)
                {
                    wall[i].xpanning -= t >> 10; // sintable[(t+512)&2047]>>12;
                    wall[i].ypanning -= t >> 10; // sintable[t&2047]>>12;

                    if (wall[i].extra == 1)
                    {
                        wall[i].extra = 0;
                        animwall[p].tag = 0;
                    }
                    else
                        animwall[p].tag += 128;

                    if (animwall[p].tag < (128 << 4))
                    {
                        if (animwall[p].tag & 128)
                            wall[i].overpicnum = W_FORCEFIELD;
                        else wall[i].overpicnum = W_FORCEFIELD + 1;
                    }
                    else
                    {
                        if ((krand() & 255) < 32)
                            animwall[p].tag = 128 << (krand() & 3);
                        else wall[i].overpicnum = W_FORCEFIELD + 1;
                    }
                }

                break;
            }
    }
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void operaterespawns_d(int low)
{
    short i, j, nexti;

    i = headspritestat[11];
    while (i >= 0)
    {
        nexti = nextspritestat[i];
        if (sprite[i].lotag == low) switch (sprite[i].picnum)
        {
        case RESPAWN:
            if (badguypic(sprite[i].hitag) && ud.monsters_off) break;

            j = spawn(i, TRANSPORTERSTAR);
            sprite[j].z -= (32 << 8);

            sprite[i].extra = 66 - 12;   // Just a way to killit
            break;
        }
        i = nexti;
    }
}


END_DUKE_NS
