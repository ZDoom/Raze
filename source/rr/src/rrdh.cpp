//-------------------------------------------------------------------------
/*
Copyright (C) 2016 EDuke32 developers and contributors
Copyright (C) 2020 Nuke.YKT

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
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/
//-------------------------------------------------------------------------

#include "ns.h"


#include "compat.h"
#include "baselayer.h"
#include "duke3d.h"
#include "i_time.h"

BEGIN_RR_NS

int rrdh_randseed = 1;

int rrdh_random(void)
{
    static int seedinit = 0;
    if (!seedinit)
    {
        rrdh_randseed = (int)I_nsTime();
        seedinit = 1;
    }
    rrdh_randseed = (rrdh_randseed*1103515245)+12345;
    return (rrdh_randseed>>16)&0x7fff;
}


int ghcons_isanimalescapewall(short w)
{
    walltype *wl = &wall[w];
    switch (wall[w].picnum)
    {
    case 1182:
    case 1183:
    case 1184:
    case 1185:
    case 2347:
    case 3802:
    case 3803:
    case 7870:
    case 7871:
    case 7872:
        return 1;
    }
    return 0;
}

int ghcons_isanimalescapesect(short s)
{
    sectortype *sc = &sector[s];
    return sector[s].hitag == 2001;
}
int ghcons_findnewspot(short a1)
{
    // TODO
    return 0;
}

int dword_AA260;

int sub_51B68(void)
{
    int r;
    if ((int)totalclock - dword_AA260 < 200)
        return 0;
    if ((rrdh_random() & 127) != 64)
        return 0;

    dword_AA260 = (int)totalclock;

    r = 60 + (rrdh_random() % 15);
    S_PlaySound(r);
    return r;
}

int ghsound_pmadecall(spritetype *a1, short a2)
{
    switch (DYNAMICTILEMAP(a1->picnum))
    {
    case DOGRUN__STATICRR:
        if (A_CheckSoundPlaying(-1, 41))
            return 1;
    case VIXEN__STATICRR:
        if (A_CheckSoundPlaying(-1, 40))
            return 1;
    case PIG__STATICRR:
        if (A_CheckSoundPlaying(-1, 42))
            return 1;
    case CHEER__STATICRR:
        if (A_CheckSoundPlaying(-1, 43))
            return 1;
    }
    return 0;
}

int ghsound_pmadesound(spritetype *a1, short a2)
{
    int d = klabs(g_player[a2].ps->pos.x - a1->x) + klabs(g_player[a2].ps->pos.y - a1->y);
    if (A_CheckSoundPlaying(-1, 1) && d < 23040)
        return 1;
    if (A_CheckSoundPlaying(-1, 2))
        return 1;
    if (A_CheckSoundPlaying(-1, 3) && d < 23040)
        return 1;
    if (A_CheckSoundPlaying(-1, 4))
        return 1;
    if (A_CheckSoundPlaying(-1, 5) && d < 23040)
        return 1;
    if (A_CheckSoundPlaying(-1, 6))
        return 1;
    if (A_CheckSoundPlaying(-1, 7) && d < 23040)
        return 1;
    if (A_CheckSoundPlaying(-1, 8) && d < 23040)
        return 1;
    if (A_CheckSoundPlaying(-1, 56) && d < 23040)
        return 1;
    if (A_CheckSoundPlaying(-1, 57) && d < 23040)
        return 1;
    if (A_CheckSoundPlaying(-1, 58) && d < 23040)
        return 1;
    if (A_CheckSoundPlaying(-1, 59) && d < 23040)
        return 1;
    if (A_CheckSoundPlaying(-1, 25) && d < 21504)
        return 1;
    if (A_CheckSoundPlaying(-1, 11) && d < 10752)
        return 1;
    if (A_CheckSoundPlaying(-1, 9) && d < 15360)
        return 1;
    if (A_CheckSoundPlaying(-1, 10) && d < 30720)
        return 1;
    if (A_CheckSoundPlaying(-1, 12) && d < 19968)
        return 1;
    if (A_CheckSoundPlaying(-1, 15) && d < 10752)
        return 1;
    if (A_CheckSoundPlaying(-1, 13) && d < 15360)
        return 1;
    if (A_CheckSoundPlaying(-1, 14) && d < 30720)
        return 1;
    if (A_CheckSoundPlaying(-1, 16) && d < 19968)
        return 1;
    if (A_CheckSoundPlaying(-1, 19) && d < 10752)
        return 1;
    if (A_CheckSoundPlaying(-1, 17) && d < 15360)
        return 1;
    if (A_CheckSoundPlaying(-1, 18) && d < 30720)
        return 1;
    if (A_CheckSoundPlaying(-1, 20) && d < 19968)
        return 1;
    if (A_CheckSoundPlaying(-1, 23) && d < 10752)
        return 1;
    if (A_CheckSoundPlaying(-1, 21) && d < 15360)
        return 1;
    if (A_CheckSoundPlaying(-1, 22) && d < 30720)
        return 1;
    if (A_CheckSoundPlaying(-1, 24) && d < 19968)
        return 1;
    if (A_CheckSoundPlaying(-1, 81) && d < 15360)
        return 1;
    if (A_CheckSoundPlaying(-1, 80) && d < 30720)
        return 1;
    if (A_CheckSoundPlaying(-1, 41) && d < 23040)
        return 1;
    if (A_CheckSoundPlaying(-1, 40) && d < 23040)
        return 1;
    if (A_CheckSoundPlaying(-1, 42) && d < 23040)
        return 1;
    if (A_CheckSoundPlaying(-1, 43) && d < 23040)
        return 1;
    if (A_CheckSoundPlaying(-1, 88) && d < 10752)
        return 1;
    if (A_CheckSoundPlaying(-1, 89) && d < 15360)
        return 1;
    if (A_CheckSoundPlaying(-1, 90) && d < 23040)
        return 1;
    if (A_CheckSoundPlaying(-1, 27) && d < 30720)
        return 1;
    if (A_CheckSoundPlaying(-1, 36) && d < 30720)
        return 1;
    if (A_CheckSoundPlaying(-1, 30) && d < 30720)
        return 1;
    if (A_CheckSoundPlaying(-1, 33) && d < 30720)
        return 1;
    return 0;
}

int ghsound_pfiredgunnear(spritetype *a1, short a2)
{
    if (A_CheckSoundPlaying(-1, 2))
        return 1;
    if (A_CheckSoundPlaying(-1, 4))
        return 1;
    if (A_CheckSoundPlaying(-1, 6))
        return 1;
    if (A_CheckSoundPlaying(-1, 8) && klabs(g_player[a2].ps->pos.x - a1->x) + klabs(g_player[a2].ps->pos.y - a1->y) < 23040)
        return 1;
    return 0;
}

void ghsound_ambientlooppoll(void)
{
    // TODO
}

void sub_53304(void)
{
    // TODO
}

int dword_AA2F4;

void sub_535DC(void)
{
    dword_AA2F4 ^= 1;
}

int sub_535EC(void)
{
    return dword_AA2F4;
}

int dword_AA300;

struct struct2B80E0 {
    short f_0;
    short f_2;
    int f_4;
    int f_8;
    int f_c;
    int f_10;
};

struct2B80E0 f2B80E0[20];

void ghtrophy_loadbestscores(void)
{
    // TODO
}

int ghtrophy_isakill(short a1)
{
    spritetype *spr = &sprite[a1];
    for (int i = 0; i < dword_AA300; i++)
    {
        if (f2B80E0[i].f_0 == a1)
            return 1;
    }
    return 0;
}

void sub_54DE0(void)
{
    // TODO
}

void sub_579A0(void)
{
    // TODO
}

int ghtrax_isplrupwind(short a1, short a2)
{
    // TODO
    return 0;
}

void ghtrax_leavetrax(short a1)
{
    // TODO
}
void ghtrax_leavedroppings(short a1)
{
    // TODO
}

void ghdeploy_bias(short a1)
{
    // TODO
}

void sub_5A02C(void)
{
    // TODO
}

void sub_5A250(int a1)
{
    // TODO
}
END_RR_NS
