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

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void operateforcefields_d(int s, int low)
{
    operateforcefields_common(s, low, { W_FORCEFIELD, W_FORCEFIELD + 1, W_FORCEFIELD + 2, BIGFORCE });
}

//---------------------------------------------------------------------------
//
// how NOT to implement switch animations...
//
//---------------------------------------------------------------------------

bool checkhitswitch_d(int snum, int w, int switchtype)
{
    uint8_t switchpal;
    int i, x, lotag, hitag, picnum, correctdips, numdips;
    int sx, sy;

    if (w < 0) return 0;
    correctdips = 1;
    numdips = 0;

    if (switchtype == SWITCH_SPRITE) // A wall sprite
    {
        lotag = sprite[w].lotag; if (lotag == 0) return 0;
        hitag = sprite[w].hitag;
        sx = sprite[w].x;
        sy = sprite[w].y;
        picnum = sprite[w].picnum;
        switchpal = sprite[w].pal;
    }
    else
    {
        lotag = wall[w].lotag; if (lotag == 0) return 0;
        hitag = wall[w].hitag;
        sx = wall[w].x;
        sy = wall[w].y;
        picnum = wall[w].picnum;
        switchpal = wall[w].pal;
    }

    switch (picnum)
    {
    case DIPSWITCH:
    case DIPSWITCH + 1:
    case TECHSWITCH:
    case TECHSWITCH + 1:
    case ALIENSWITCH:
    case ALIENSWITCH + 1:
        break;
    case ACCESSSWITCH:
    case ACCESSSWITCH2:
        if (ps[snum].access_incs == 0)
        {
            if (switchpal == 0)
            {
                if ((ps[snum].got_access & 1))
                    ps[snum].access_incs = 1;
                else FTA(70, &ps[snum]);
            }

            else if (switchpal == 21)
            {
                if (ps[snum].got_access & 2)
                    ps[snum].access_incs = 1;
                else FTA(71, &ps[snum]);
            }

            else if (switchpal == 23)
            {
                if (ps[snum].got_access & 4)
                    ps[snum].access_incs = 1;
                else FTA(72, &ps[snum]);
            }

            if (ps[snum].access_incs == 1)
            {
                if (switchtype == SWITCH_WALL)
                    ps[snum].access_wallnum = w;
                else
                    ps[snum].access_spritenum = w;
            }

            return 0;
        }
    case DIPSWITCH2:
    case DIPSWITCH2 + 1:
    case DIPSWITCH3:
    case DIPSWITCH3 + 1:
    case MULTISWITCH:
    case MULTISWITCH + 1:
    case MULTISWITCH + 2:
    case MULTISWITCH + 3:
    case PULLSWITCH:
    case PULLSWITCH + 1:
    case HANDSWITCH:
    case HANDSWITCH + 1:
    case SLOTDOOR:
    case SLOTDOOR + 1:
    case LIGHTSWITCH:
    case LIGHTSWITCH + 1:
    case SPACELIGHTSWITCH:
    case SPACELIGHTSWITCH + 1:
    case SPACEDOORSWITCH:
    case SPACEDOORSWITCH + 1:
    case FRANKENSTINESWITCH:
    case FRANKENSTINESWITCH + 1:
    case LIGHTSWITCH2:
    case LIGHTSWITCH2 + 1:
    case POWERSWITCH1:
    case POWERSWITCH1 + 1:
    case LOCKSWITCH1:
    case LOCKSWITCH1 + 1:
    case POWERSWITCH2:
    case POWERSWITCH2 + 1:
        if (check_activator_motion(lotag)) return 0;
        break;
    default:
        if (isadoorwall(picnum) == 0) return 0;
        break;
    }

    i = headspritestat[0];
    while (i >= 0)
    {
        if (lotag == sprite[i].lotag) switch (sprite[i].picnum)
        {
        case DIPSWITCH:
        case TECHSWITCH:
        case ALIENSWITCH:
            if (switchtype == SWITCH_SPRITE && w == i) sprite[i].picnum++;
            else if (sprite[i].hitag == 0) correctdips++;
            numdips++;
            break;
        case TECHSWITCH + 1:
        case DIPSWITCH + 1:
        case ALIENSWITCH + 1:
            if (switchtype == SWITCH_SPRITE && w == i) sprite[i].picnum--;
            else if (sprite[i].hitag == 1) correctdips++;
            numdips++;
            break;
        case MULTISWITCH:
        case MULTISWITCH + 1:
        case MULTISWITCH + 2:
        case MULTISWITCH + 3:
            sprite[i].picnum++;
            if (sprite[i].picnum > (MULTISWITCH + 3))
                sprite[i].picnum = MULTISWITCH;
            break;
        case ACCESSSWITCH:
        case ACCESSSWITCH2:
        case SLOTDOOR:
        case LIGHTSWITCH:
        case SPACELIGHTSWITCH:
        case SPACEDOORSWITCH:
        case FRANKENSTINESWITCH:
        case LIGHTSWITCH2:
        case POWERSWITCH1:
        case LOCKSWITCH1:
        case POWERSWITCH2:
        case HANDSWITCH:
        case PULLSWITCH:
        case DIPSWITCH2:
        case DIPSWITCH3:
            sprite[i].picnum++;
            break;
        case PULLSWITCH + 1:
        case HANDSWITCH + 1:
        case LIGHTSWITCH2 + 1:
        case POWERSWITCH1 + 1:
        case LOCKSWITCH1 + 1:
        case POWERSWITCH2 + 1:
        case SLOTDOOR + 1:
        case LIGHTSWITCH + 1:
        case SPACELIGHTSWITCH + 1:
        case SPACEDOORSWITCH + 1:
        case FRANKENSTINESWITCH + 1:
        case DIPSWITCH2 + 1:
        case DIPSWITCH3 + 1:
            sprite[i].picnum--;
            break;
        }
        i = nextspritestat[i];
    }

    for (i = 0; i < numwalls; i++)
    {
        x = i;
        if (lotag == wall[x].lotag)
            switch (wall[x].picnum)
            {
            case DIPSWITCH:
            case TECHSWITCH:
            case ALIENSWITCH:
                if (switchtype == SWITCH_WALL && i == w) wall[x].picnum++;
                else if (wall[x].hitag == 0) correctdips++;
                numdips++;
                break;
            case DIPSWITCH + 1:
            case TECHSWITCH + 1:
            case ALIENSWITCH + 1:
                if (switchtype == SWITCH_WALL && i == w) wall[x].picnum--;
                else if (wall[x].hitag == 1) correctdips++;
                numdips++;
                break;
            case MULTISWITCH:
            case MULTISWITCH + 1:
            case MULTISWITCH + 2:
            case MULTISWITCH + 3:
                wall[x].picnum++;
                if (wall[x].picnum > (MULTISWITCH + 3))
                    wall[x].picnum = MULTISWITCH;
                break;
            case ACCESSSWITCH:
            case ACCESSSWITCH2:
            case SLOTDOOR:
            case LIGHTSWITCH:
            case SPACELIGHTSWITCH:
            case SPACEDOORSWITCH:
            case LIGHTSWITCH2:
            case POWERSWITCH1:
            case LOCKSWITCH1:
            case POWERSWITCH2:
            case PULLSWITCH:
            case HANDSWITCH:
            case DIPSWITCH2:
            case DIPSWITCH3:
                wall[x].picnum++;
                break;
            case HANDSWITCH + 1:
            case PULLSWITCH + 1:
            case LIGHTSWITCH2 + 1:
            case POWERSWITCH1 + 1:
            case LOCKSWITCH1 + 1:
            case POWERSWITCH2 + 1:
            case SLOTDOOR + 1:
            case LIGHTSWITCH + 1:
            case SPACELIGHTSWITCH + 1:
            case SPACEDOORSWITCH + 1:
            case DIPSWITCH2 + 1:
            case DIPSWITCH3 + 1:
                wall[x].picnum--;
                break;
            }
    }

    if (lotag == (short)65535)
    {
        ps[myconnectindex].gm = MODE_EOL;
        if (ud.from_bonus)
        {
            ud.level_number = ud.from_bonus;
            ud.m_level_number = ud.level_number;
            ud.from_bonus = 0;
        }
        else
        {
            // fixme: This needs to be taken from the level definitions.
            ud.level_number = (++ud.level_number < MAXLEVELS) ? ud.level_number : 0;
            ud.m_level_number = ud.level_number;
        }
        return 1;
    }

    vec3_t v = { sx, sy, ps[snum].posz };
    switch (picnum)
    {
    default:
        if (isadoorwall(picnum) == 0) break;
    case DIPSWITCH:
    case DIPSWITCH + 1:
    case TECHSWITCH:
    case TECHSWITCH + 1:
    case ALIENSWITCH:
    case ALIENSWITCH + 1:
        if (picnum == DIPSWITCH || picnum == DIPSWITCH + 1 ||
            picnum == ALIENSWITCH || picnum == ALIENSWITCH + 1 ||
            picnum == TECHSWITCH || picnum == TECHSWITCH + 1)
        {
            if (picnum == ALIENSWITCH || picnum == ALIENSWITCH + 1)
            {
                if (switchtype == SWITCH_SPRITE)
                    S_PlaySound3D(ALIEN_SWITCH1, w, &v);
                else S_PlaySound3D(ALIEN_SWITCH1, ps[snum].i, &v);
            }
            else
            {
                if (switchtype == SWITCH_SPRITE)
                    S_PlaySound3D(SWITCH_ON, w, &v);
                else S_PlaySound3D(SWITCH_ON, ps[snum].i, &v);
            }
            if (numdips != correctdips) break;
            S_PlaySound3D(END_OF_LEVEL_WARN, ps[snum].i, &v);
        }
    case DIPSWITCH2:
    case DIPSWITCH2 + 1:
    case DIPSWITCH3:
    case DIPSWITCH3 + 1:
    case MULTISWITCH:
    case MULTISWITCH + 1:
    case MULTISWITCH + 2:
    case MULTISWITCH + 3:
    case ACCESSSWITCH:
    case ACCESSSWITCH2:
    case SLOTDOOR:
    case SLOTDOOR + 1:
    case LIGHTSWITCH:
    case LIGHTSWITCH + 1:
    case SPACELIGHTSWITCH:
    case SPACELIGHTSWITCH + 1:
    case SPACEDOORSWITCH:
    case SPACEDOORSWITCH + 1:
    case FRANKENSTINESWITCH:
    case FRANKENSTINESWITCH + 1:
    case LIGHTSWITCH2:
    case LIGHTSWITCH2 + 1:
    case POWERSWITCH1:
    case POWERSWITCH1 + 1:
    case LOCKSWITCH1:
    case LOCKSWITCH1 + 1:
    case POWERSWITCH2:
    case POWERSWITCH2 + 1:
    case HANDSWITCH:
    case HANDSWITCH + 1:
    case PULLSWITCH:
    case PULLSWITCH + 1:

        if (picnum == MULTISWITCH || picnum == (MULTISWITCH + 1) ||
            picnum == (MULTISWITCH + 2) || picnum == (MULTISWITCH + 3))
            lotag += picnum - MULTISWITCH;

        x = headspritestat[3];
        while (x >= 0)
        {
            if (((sprite[x].hitag) == lotag))
            {
                switch (sprite[x].lotag)
                {
                case SE_12_LIGHT_SWITCH:
                    sector[sprite[x].sectnum].floorpal = 0;
                    hittype[x].temp_data[0]++;
                    if (hittype[x].temp_data[0] == 2)
                        hittype[x].temp_data[0]++;

                    break;
                case SE_24_CONVEYOR:
                case SE_34:
                case SE_25_PISTON:
                    hittype[x].temp_data[4] = !hittype[x].temp_data[4];
                    if (hittype[x].temp_data[4])
                        FTA(15, &ps[snum]);
                    else FTA(2, &ps[snum]);
                    break;
                case SE_21_DROP_FLOOR:
                    FTA(2, &ps[screenpeek]);
                    break;
                }
            }
            x = nextspritestat[x];
        }

        operateactivators(lotag, snum);
        operateforcefields(ps[snum].i, lotag);
        operatemasterswitches(lotag);

        if (picnum == DIPSWITCH || picnum == DIPSWITCH + 1 ||
            picnum == ALIENSWITCH || picnum == ALIENSWITCH + 1 ||
            picnum == TECHSWITCH || picnum == TECHSWITCH + 1) return 1;

        if (hitag == 0 && isadoorwall(picnum) == 0)
        {
            if (switchtype == SWITCH_SPRITE)
                S_PlaySound3D(SWITCH_ON, w, &v);
            else S_PlaySound3D(SWITCH_ON, ps[snum].i, &v);
        }
        else if (hitag != 0)
        {
            auto flags = S_GetUserFlags(hitag);

            if (switchtype == SWITCH_SPRITE && (flags & SF_TALK) == 0)
                S_PlaySound3D(hitag, w, &v);
            else
                A_PlaySound(hitag, ps[snum].i);
        }

        return 1;
    }
    return 0;
}



END_DUKE_NS
