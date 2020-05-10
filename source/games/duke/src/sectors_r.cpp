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
#include "global.h"
#include "sounds_common.h"
#include "names_rr.h"

// PRIMITIVE
BEGIN_DUKE_NS

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

bool isadoorwall_r(int dapic)
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
        case RRTILE1856:
        case RRTILE1877:
            return 1;
    }
    return 0;
}

bool isablockdoor(int dapic)
{
    switch (dapic)
    {
        case RRTILE1792:
        case RRTILE1801:
        case RRTILE1805:
        case RRTILE1807:
        case RRTILE1808:
        case RRTILE1812:
        case RRTILE1821:
        case RRTILE1826:
        case RRTILE1850:
        case RRTILE1851:
        case RRTILE1856:
        case RRTILE1877:
        case RRTILE1938:
        case RRTILE1942:
        case RRTILE1944:
        case RRTILE1945:
        case RRTILE1951:
        case RRTILE1961:
        case RRTILE1964:
        case RRTILE1985:
        case RRTILE1995:
        case RRTILE2022:
        case RRTILE2052:
        case RRTILE2053:
        case RRTILE2060:
        case RRTILE2074:
        case RRTILE2132:
        case RRTILE2136:
        case RRTILE2139:
        case RRTILE2150:
        case RRTILE2178:
        case RRTILE2186:
        case RRTILE2319:
        case RRTILE2321:
        case RRTILE2326:
        case RRTILE2329:
        case RRTILE2578:
        case RRTILE2581:
        case RRTILE2610:
        case RRTILE2613:
        case RRTILE2621:
        case RRTILE2622:
        case RRTILE2676:
        case RRTILE2732:
        case RRTILE2831:
        case RRTILE2832:
        case RRTILE2842:
        case RRTILE2940:
        case RRTILE2970:
        case RRTILE3083:
        case RRTILE3100:
        case RRTILE3155:
        case RRTILE3195:
        case RRTILE3232:
        case RRTILE3600:
        case RRTILE3631:
        case RRTILE3635:
        case RRTILE3637:
        case RRTILE3643+2:
        case RRTILE3643+3:
        case RRTILE3647:
        case RRTILE3652:
        case RRTILE3653:
        case RRTILE3671:
        case RRTILE3673:
        case RRTILE3684:
        case RRTILE3708:
        case RRTILE3714:
        case RRTILE3716:
        case RRTILE3723:
        case RRTILE3725:
        case RRTILE3737:
        case RRTILE3754:
        case RRTILE3762:
        case RRTILE3763:
        case RRTILE3764:
        case RRTILE3765:
        case RRTILE3767:
        case RRTILE3793:
        case RRTILE3814:
        case RRTILE3815:
        case RRTILE3819:
        case RRTILE3827:
        case RRTILE3837:
			return true;
			
        case RRTILE1996:
        case RRTILE2382:
        case RRTILE2961:
        case RRTILE3804:
        case RRTILE7430:
        case RRTILE7467:
        case RRTILE7469:
        case RRTILE7470:
        case RRTILE7475:
        case RRTILE7566:
        case RRTILE7576:
        case RRTILE7716:
        case RRTILE8063:
        case RRTILE8067:
        case RRTILE8076:
        case RRTILE8106:
        case RRTILE8379:
        case RRTILE8380:
        case RRTILE8565:
        case RRTILE8605:
            return isRRRA();
    }
    return false;
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void animatewalls_r(void)
{
    int i, j, p, t;

    if (isRRRA() &&ps[screenpeek].sea_sick_stat == 1)
    {
        for (i = 0; i < MAXWALLS; i++)
        {
            if (wall[i].picnum == RRTILE7873)
                wall[i].xpanning += 6;
            else if (wall[i].picnum == RRTILE7870)
                wall[i].xpanning += 6;
        }
    }

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

            if ((krand() & 255) < 16)
            {
                animwall[p].tag = wall[i].picnum;
                wall[i].picnum = SCREENBREAK6;
            }

            continue;

        case SCREENBREAK6:
        case SCREENBREAK7:
        case SCREENBREAK8:

            if (animwall[p].tag >= 0)
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

void operaterespawns_r(int low)
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
        case RRTILE7424:
            if (isRRRA() && !ud.monsters_off)
                changespritestat(i, 119);
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

void operateforcefields_r(int s, int low)
{
    operateforcefields_common(s, low, { BIGFORCE });
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

bool checkhitswitch_r(int snum, int w, int switchtype)
{
    char switchpal;
    short i, x, lotag, hitag, picnum, correctdips, numdips;
    int sx, sy;

    if (w < 0) return 0;
    correctdips = 1;
    numdips = 0;

    if (switchtype == 1) // A wall sprite
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
                if (ps[snum].keys[1])
                    ps[snum].access_incs = 1;
                else
                {
                    FTA(70, &ps[snum]);
                    if (isRRRA()) spritesound(99, w);
                }
            }

            else if (switchpal == 21)
            {
                if (ps[snum].keys[2])
                    ps[snum].access_incs = 1;
                else
                {
                    FTA(71, &ps[snum]);
                    if (isRRRA()) spritesound(99, w);
                }
            }

            else if (switchpal == 23)
            {
                if (ps[snum].keys[3])
                    ps[snum].access_incs = 1;
                else
                {
                    FTA(72, &ps[snum]);
                    if (isRRRA()) spritesound(99, w);
                }
            }

            if (ps[snum].access_incs == 1)
            {
                if (switchtype == 0)
                    ps[snum].access_wallnum = w;
                else
                    ps[snum].access_spritenum = w;
            }

            return 0;
        }
    case MULTISWITCH2:
    case MULTISWITCH2 + 1:
    case MULTISWITCH2 + 2:
    case MULTISWITCH2 + 3:
    case RRTILE8464:
    case RRTILE8660:
        if (isRRRA()) break;
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
    case NUKEBUTTON:
    case NUKEBUTTON + 1:
    case RRTILE2214:
    case RRTILE2697:
    case RRTILE2697 + 1:
    case RRTILE2707:
    case RRTILE2707 + 1:
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
            if (switchtype == 1 && w == i) sprite[i].picnum++;
            else if (sprite[i].hitag == 0) correctdips++;
            numdips++;
            break;
        case TECHSWITCH + 1:
        case DIPSWITCH + 1:
        case ALIENSWITCH + 1:
            if (switchtype == 1 && w == i) sprite[i].picnum--;
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
        case MULTISWITCH2:
        case MULTISWITCH2 + 1:
        case MULTISWITCH2 + 2:
        case MULTISWITCH2 + 3:
            if (!isRRRA()) break;
            sprite[i].picnum++;
            if (sprite[i].picnum > (MULTISWITCH2 + 3))
                sprite[i].picnum = MULTISWITCH2;
            break;

        case RRTILE2214:
            if (ud.level_number > 6)
                ud.level_number = 0;
            sprite[i].picnum++;
            break;
        case RRTILE8660:
            if (!isRRRA()) break;
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
        case NUKEBUTTON:
        case RRTILE2697:
        case RRTILE2707:
            if (sprite[i].picnum == DIPSWITCH3)
                if (sprite[i].hitag == 999)
                {
                    short j, nextj;
                    j = headspritestat[107];
                    while (j >= 0)
                    {
                        nextj = nextspritestat[j];
                        if (sprite[j].picnum == RRTILE3410)
                        {
                            sprite[j].picnum++;
                            sprite[j].hitag = 100;
                            sprite[j].extra = 0;
                            spritesound(474, j);
                        }
                        else if (sprite[j].picnum == RRTILE295)
                            deletesprite(j);
                        j = nextj;
                    }
                    sprite[i].picnum++;
                    break;
                }
            if (sprite[i].picnum == NUKEBUTTON)
                chickenplant = 0;
            if (sprite[i].picnum == RRTILE8660)
            {
                BellTime = 132;
                word_119BE0 = i;
            }
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
        case NUKEBUTTON + 1:
        case RRTILE2697 + 1:
        case RRTILE2707 + 1:
            if (sprite[i].picnum == NUKEBUTTON + 1)
                chickenplant = 1;
            if (sprite[i].hitag != 999)
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
                if (switchtype == 0 && i == w) wall[x].picnum++;
                else if (wall[x].hitag == 0) correctdips++;
                numdips++;
                break;
            case DIPSWITCH + 1:
            case TECHSWITCH + 1:
            case ALIENSWITCH + 1:
                if (switchtype == 0 && i == w) wall[x].picnum--;
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
            case MULTISWITCH2:
            case MULTISWITCH2 + 1:
            case MULTISWITCH2 + 2:
            case MULTISWITCH2 + 3:
                if (!isRRRA()) break;
                wall[x].picnum++;
                if (wall[x].picnum > (MULTISWITCH2 + 3))
                    wall[x].picnum = MULTISWITCH2;
                break;
            case RRTILE8660:
                if (!isRRRA()) break;
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
            case RRTILE2697:
            case RRTILE2707:
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
            case RRTILE2697 + 1:
            case RRTILE2707 + 1:
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
            if (isRRRA() && ud.level_number == 6 && ud.volume_number == 0)
                g_RAendEpisode = 1; // hack to force advancing to episode 2.
            ud.level_number = (++ud.level_number < MAXLEVELS) ? ud.level_number : 0;
            ud.m_level_number = ud.level_number;
        }
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
    case MULTISWITCH2:
    case MULTISWITCH2 + 1:
    case MULTISWITCH2 + 2:
    case MULTISWITCH2 + 3:
    case RRTILE8464:
    case RRTILE8660:
        if (!isRRRA()) break;
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
    case RRTILE2697:
    case RRTILE2697 + 1:
    case RRTILE2707:
    case RRTILE2707 + 1:
        if (isRRRA())
        {
            if (picnum == RRTILE8660)
            {
                BellTime = 132;
                word_119BE0 = w;
                sprite[w].picnum++;
            }
            else if (picnum == RRTILE8464)
            {
                sprite[w].picnum = sprite[w].picnum + 1;
                if (hitag == 10001)
                {
                    if (ps[snum].SeaSick == 0)
                        ps[snum].SeaSick = 350;
                    operateactivators(668, ps[snum].i);
                    operatemasterswitches(668);
                    spritesound(328, ps[snum].i);
                    return 1;
                }
            }
            else if (hitag == 10000)
            {
                if (picnum == MULTISWITCH || picnum == (MULTISWITCH + 1) ||
                    picnum == (MULTISWITCH + 2) || picnum == (MULTISWITCH + 3) ||
                    picnum == MULTISWITCH2 || picnum == (MULTISWITCH2 + 1) ||
                    picnum == (MULTISWITCH2 + 2) || picnum == (MULTISWITCH2 + 3))
                {
                    int var6c[3], var54, j;
                    short jpn, jht;
                    var54 = 0;
                    S_PlaySound3D(SWITCH_ON, w, &v);
                    for (j = 0; j < MAXSPRITES; j++)
                    {
                        jpn = sprite[j].picnum;
                        jht = sprite[j].hitag;
                        if ((jpn == MULTISWITCH || jpn == MULTISWITCH2) && jht == 10000)
                        {
                            if (var54 < 3)
                            {
                                var6c[var54] = j;
                                var54++;
                            }
                        }
                    }
                    if (var54 == 3)
                    {
                        S_PlaySound3D(78, w, &v);
                        for (j = 0; j < var54; j++)
                        {
                            sprite[var6c[j]].hitag = 0;
                            if (picnum >= MULTISWITCH2)
                                sprite[var6c[j]].picnum = MULTISWITCH2 + 3;
                            else
                                sprite[var6c[j]].picnum = MULTISWITCH + 3;
                            checkhitswitch_r(snum, var6c[j], 1);
                        }
                    }
                    return 1;
                }
            }
        }
        if (picnum == MULTISWITCH || picnum == (MULTISWITCH + 1) ||
            picnum == (MULTISWITCH + 2) || picnum == (MULTISWITCH + 3))
            lotag += picnum - MULTISWITCH;
        if (isRRRA())
        {
            if (picnum == MULTISWITCH2 || picnum == (MULTISWITCH2 + 1) ||
                picnum == (MULTISWITCH2 + 2) || picnum == (MULTISWITCH2 + 3))
                lotag += picnum - MULTISWITCH2;
        }

        x = headspritestat[3];
        while (x >= 0)
        {
            if (((sprite[x].hitag) == lotag))
            {
                switch (sprite[x].lotag)
                {
                case 46:
                case 47:
                case 48:
                    if (!isRRRA()) break;
                case 12:
                    sector[sprite[x].sectnum].floorpal = 0;
                    hittype[x].temp_data[0]++;
                    if (hittype[x].temp_data[0] == 2)
                        hittype[x].temp_data[0]++;

                    break;
                case 24:
                case 34:
                case 25:
                    hittype[x].temp_data[4] = !hittype[x].temp_data[4];
                    if (hittype[x].temp_data[4])
                        FTA(15, &ps[snum]);
                    else FTA(2, &ps[snum]);
                    break;
                case 21:
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
