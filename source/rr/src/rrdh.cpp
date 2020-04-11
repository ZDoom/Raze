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
#include "sounds.h"
#include "i_time.h"
#include "files.h"
#include "i_specialpaths.h"

// Shut up the compiler.
#ifdef _MSC_VER
#pragma warning(disable:4101)
#endif
BEGIN_RR_NS

inline bool KB_KeyPressed(int code)
{
    return inputState.GetKeyStatus(code);
}

inline void KB_ClearKeyDown(int key)
{
    inputState.ClearKeyStatus(key);
}

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


int sub_5151C(short a1)
{
    switch (a1)
    {
    case 26:
    case 27:
    case 29:
    case 30:
    case 31:
    case 32:
    case 33:
    case 34:
    case 35:
    case 73:
    case 74: 
    case 92:
    case 93:
    case 96:
    case 97:
    case 98:
    case 103:
    case 141:
    case 142:
    case 143:
    case 144:
    case 145:
    case 146:
    case 147:
    case 148:
    case 149:
    case 150:
    case 151:
    case 152:
    case 153:
    case 154:
    case 155:
    case 156:
    case 157:
    case 158:
    case 159:
    case 160:
    case 161:
    case 162:
    case 163:
    case 164:
    case 165:
        return 1;
    }
    return 0;
}

int rrgh_isatree(short s)
{
    switch (sprite[s].picnum)
    {
    case 984:
    case 985:
    case 986:
    case 988:
    case 989:
    case 990:
    case 991:
    case 1028:
    case 1029:
    case 1030:
    case 1040:
    case 1041:
    case 1050:
    case 1051:
    case 1052:
    case 1053:
    case 1054:
    case 1055:
    case 1056:
    case 1057:
    case 1058:
    case 1059:
    case 1093:
    case 1094:
    case 1095:
    case 1096:
    case 1097:
    case 1098:
    case 1099:
    case 1100:
    case 1113:
    case 1114:
    case 1115:
    case 1116:
    case 1117:
        return 1;
    }
    return 0;
}

void sub_51678(int a1, int a2, int a3, int a4, int a5, int a6)
{
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

int sub_517AC(int *a1, int *a2, short *a3)
{
    int i, x, y;
    short j;
    if (numsectors < 0 || numsectors >= MAXSECTORS)
        return 0;
    for (i = 0; i < 32; i++)
    {
        x = (((rrdh_random() & 2047) + 1) - 1024) * 100;
        y = (((rrdh_random() & 2047) + 1) - 1024) * 100;

        for (j = 0; j < numsectors; j++)
        {
            if (inside(x, y, j))
            {
                *a1 = x;
                *a2 = y;
                *a3 = j;
                return 1;
            }
        }
    }
    return 0;
}

int ghcons_findnewspot(short a1)
{
    int v20 = 0, v24 = 0;
    short v18 = 0;
    spritetype *spr;
    sectortype *sec;
    spr = &sprite[a1];
    if (sub_517AC(&v20, &v24, &v18))
    {
        sec = &sector[v18];
        switch (DYNAMICTILEMAP(spr->picnum))
        {
        case PIG__STATICRR:
        case VIXEN__STATICRR:
        case CHEER__STATICRR:
            if (sec->hitag)
    return 0;
            break;
        case DOGRUN__STATICRR:
            if (sec->hitag)
                return 0;
            break;
        }
        vec3_t pos = { v20, v24,  v18 };
        setsprite(a1, &pos);
        changespritesect(a1, v18);
        if (spr->picnum == DOGRUN)
            spr->z = -307200;
        else
            spr->z = sec->floorz;
        return 1;
    }
    return 0;
}

void sub_519E8(int a1)
{
    int vbx;
    if ((rrdh_random() & 63) == 32)
    {
        if (sub_57A60(20))
        {
            vbx = rrdh_random() % 5;
            switch (a1)
            {
            case 0:
            case 4:
                vbx += 60;
                break;
            case 1:
            case 5:
                vbx += 65;
                break;
            case 2:
                vbx += 70;
                break;
            case 3:
                vbx += 75;
                break;
            }
            S_PlaySound(vbx);
        }
    }
}

int dword_AA25C;

void ghsound_ambientlooppoll(void)
{
    if (dword_AA25C)
    {
        if (dword_AA25C < 0 || dword_AA25C >= MAXSOUNDS)
        {
            Printf("ghsound_ambientlooppoll bad index\n");
            return;
        }
        if (!A_CheckSoundPlaying(-1, dword_AA25C))
        {
            A_PlaySound(dword_AA25C, g_player[screenpeek].ps->i);
        }
    }
}

void ghsound_ambientloop(int a1)
{
    switch (a1)
    {
    case 0:
        A_PlaySound(83, g_player[screenpeek].ps->i);
        dword_AA25C = 83;
        break;
    case 1:
        S_PlaySound(84);
        dword_AA25C = 84;
        break;
    case 2:
        S_PlaySound(85);
        dword_AA25C = 85;
        break;
    default:
        dword_AA25C = 0;
        break;
    }
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

int sub_5228C(short a1)
{
    switch (a1)
    {
    case 998:
    case 999:
    case 1007:
    case 1008:
        return 1;
    }
    return 0;
}

int sub_522B8(short a1)
{
    switch (a1)
    {
    case 981:
    case 982:
        return 1;
    }
    return 0;
}

int dword_AA264;

void ghsound_footstepsound(short a1, int a2)
{
    DukePlayer_t *p;
    int i, nexti, d;
    spritetype *s;
    if (sub_535EC())
        return;

    p = g_player[a1].ps;
    if (sector[p->cursectnum].hitag == 2003)
    {
        if (a2 == 0)
        {
            if (sub_57A40(5) == 1)
                A_PlaySound(81, p->i);
        }
        else if (a2 == 1)
        {
            if (sub_57A40(5) == 1)
                A_PlaySound(80, p->i);
        }
        else
        {
            if (sub_57A40(5) == 1)
                A_PlaySound(82, p->i);
        }
    }
    else
    {
        if (a2 == 0)
        {
            if (sub_57A40(5) == 1)
            {
                switch (ud.level_number)
                {
                case 0:
                    A_PlaySound(9, p->i);
                    break;
                case 1:
                    A_PlaySound(13, p->i);
                    break;
                case 2:
                    A_PlaySound(17, p->i);
                    break;
                case 3:
                    A_PlaySound(21, p->i);
                    break;
                }
            }
        }
        else if (a2 == 1)
        {
            if (sub_57A40(5) == 1)
            {
                switch (ud.level_number)
                {
                case 0:
                    A_PlaySound(10, p->i);
                    break;
                case 1:
                    A_PlaySound(14, p->i);
                    break;
                case 2:
                    A_PlaySound(18, p->i);
                    break;
                case 3:
                    A_PlaySound(22, p->i);
                    break;
                }
            }
        }
        else
        {
            if (sub_57A40(5) == 1)
            {
                switch (ud.level_number)
                {
                case 0:
                    A_PlaySound(11, p->i);
                    break;
                case 1:
                    A_PlaySound(15, p->i);
                    break;
                case 2:
                    A_PlaySound(19, p->i);
                    break;
                case 3:
                    A_PlaySound(23, p->i);
                    break;
                }
            }
        }
    }
    i = headspritesect[p->cursectnum];
    while (i >= 0)
    {
        nexti = nextspritesect[i];
        s = &sprite[i];
        if (sub_5228C(s->picnum) && klabs(p->pos.x - s->x) + klabs(p->pos.y - s->y) < s->clipdist)
        {
            if (i != dword_AA264)
            {
                A_PlaySound(25, p->i);
                sub_5A250(4096);
                dword_AA264 = i;
            }
            return;
        }
        else if (sub_522B8(s->picnum) && klabs(p->pos.x - s->x) + klabs(p->pos.y - s->y) < 124)
        {
            sub_5A250(2048);
        }
        i = nexti;
    }
}

void ghsound_plrtouchedsprite(short a1, short a2)
{
    DukePlayer_t *p;
    spritetype *s;
    p = g_player[a2].ps;

    s = &sprite[a1];
    if (rrgh_isatree(a1))
    {
        switch (ud.level_number)
        {
        case 0:
            if (!A_CheckSoundPlaying(-1, 12))
                A_PlaySound(12, p->i);
            break;
        case 1:
            if (!A_CheckSoundPlaying(-1, 16))
                A_PlaySound(16, p->i);
            break;
        case 2:
            if (!A_CheckSoundPlaying(-1, 20))
                A_PlaySound(20, p->i);
            break;
        case 3:
            if (!A_CheckSoundPlaying(-1, 24))
                A_PlaySound(24, p->i);
            break;
        }
    }
}

unsigned short word_AA268[] = {
    9, 10, 11, 12, 25, 35, 36, 37, 41, 60, 61, 62, 63, 64, 80, 81, 82
};

unsigned short word_AA28A[] = {
    13, 14, 15, 16, 25, 26, 27, 28, 32, 33, 34, 40, 43, 65, 66, 67, 68, 69
};

unsigned short word_AA2AE[] = {
    17, 18, 19, 20, 25, 26, 27, 28, 29, 30, 31, 40, 42, 70, 71, 72, 73, 74
};

unsigned short word_AA2D2[] = {
    21, 22, 23, 24, 25, 26, 27, 28, 40, 75, 76, 77, 78, 79
};

void ghsound_preload(int a1)
{
#if 0
    unsigned short *vsi = NULL, snd;
    int i, c;
    switch (a1)
    {
    case 0:
        vsi = word_AA268;
        c = 17;
        break;
    case 1:
        vsi = word_AA28A;
        c = 18;
        break;
    case 2:
        vsi = word_AA2AE;
        c = 18;
        break;
    case 3:
        vsi = word_AA2D2;
        c = 14;
        break;
    }
    if (vsi)
    {
        for (i = 0; i < c; i++)
        {
            snd = vsi[i];
            if (snd >= MAXSOUNDS)
            {
                Printf("ERROR: ghsound_preload: sndnum out of range\n");
                continue;
            }
            if (snd > 0 && g_sounds[snd].ptr == NULL)
            {
                S_LoadSound(snd);
            }
        }
    }
    switch (g_player[myconnectindex].ps->dhat61f)
    {
    case 0:
        if (g_sounds[1].ptr == NULL)
        {
            S_LoadSound(1);
        }
        if (g_sounds[2].ptr == NULL)
        {
            S_LoadSound(2);
        }
        break;
    case 1:
    case 2:
        if (g_sounds[5].ptr == NULL)
        {
            S_LoadSound(5);
        }
        if (g_sounds[6].ptr == NULL)
        {
            S_LoadSound(6);
        }
        break;
    case 3:
        if (g_sounds[3].ptr == NULL)
        {
            S_LoadSound(3);
        }
        if (g_sounds[4].ptr == NULL)
        {
            S_LoadSound(4);
        }
        break;
    case 4:
        if (g_sounds[7].ptr == NULL)
        {
            S_LoadSound(7);
        }
        if (g_sounds[8].ptr == NULL)
        {
            S_LoadSound(8);
        }
        break;
    }
    if (g_sounds[98].ptr == NULL)
    {
        S_LoadSound(98);
    }
    if (g_sounds[99].ptr == NULL)
    {
        S_LoadSound(99);
    }
    if (g_sounds[100].ptr == NULL)
    {
        S_LoadSound(100);
    }
    if (g_sounds[101].ptr == NULL)
    {
        S_LoadSound(101);
    }
#endif
}

void ghprelvl_randkeep(short a1, unsigned int a2)
{
    short vbx;
    int v18 = 0;
    unsigned int i, vcx;
    while (1)
    {
        vcx = 0;
        for (i = 0; i < MAXSPRITES; i++)
        {
            if (sprite[i].picnum == a1)
                vcx++;
        }
        if (vcx <= a2)
            return;
        for (i = 0; i < MAXSPRITES; i++)
        {
            if (!v18)
                vbx = MAXSPRITES - 1 - i;
            else
                vbx = i;

            if (a1 == sprite[vbx].picnum)
            {
                if ((rrdh_random() % 256) < 32)
                {
                    sprite[vbx].picnum = 0;
                    deletesprite(vbx);
                    vcx--;
                    if (vcx <= a2)
                        return;
                }
            }
        }
        if (!v18)
            v18 = 1;
        else
            v18 = 0;
    }
}

char sub_52AB8(int a1, int a2)
{
    int vbx = (a1 * a2) / 100;
    return (rrdh_random() % vbx) % 256;
}

int sub_52AF0(short a1)
{
    if (a1 < 0 || a1 >= MAXSPRITES)
        return 0;
    switch (DYNAMICTILEMAP(sprite[a1].picnum))
    {
    case PIG__STATICRR:
    case DOGRUN__STATICRR:
    case VIXEN__STATICRR:
    case CHEER__STATICRR:
    case RRTILE1076__STATICRR:
        return 1;
    }
    return 0;
}

int sub_52B58(short a1)
{
    if (a1 < 0 || a1 >= MAXSPRITES)
        return 0;
    switch (sprite[a1].picnum)
    {
    case 976:
    case 977:
    case 2603:
    case 2990:
        return 1;
    }
    return 0;
}

void sub_52BA8(void)
{
    int v1c, tl, tc;
    int i;
    short sect;
    v1c = 0;
    sub_59C20();
    sub_54A2C();
    for (i = 0; i < MAXSPRITES; i++)
    {
        sprite[i].cstat = 0;
        sprite[i].pal = 0;
        changespritestat(i, 0);
        sect = sprite[i].sectnum;
        if (sub_52AF0(i))
        {
            if (sect >= 0 && sect < numsectors)
            {
                if (sector[sect].ceilingheinum == 0 && sector[sect].floorheinum == 0)
                {
                    if (klabs(sprite[i].z - sector[sect].floorz) > 1024 && sprite[i].z < sector[sect].floorz)
                    {
                        Printf("NOTICE: deleting floating sprite %i: x=%i, y=%i, z=%i, sect=%i\n", i, sprite[i].pos.x, sprite[i].pos.y, sprite[i].pos.z, sect);
                        deletesprite(i);
                        sprite[i].picnum = 0;
                        sprite[i].cstat = 0;
                    }
                }
            }
        }
        if (sub_52B58(i))
        {
            sprite[i].cstat = 1;
            sprite[i].clipdist = 8;
        }
        if (rrgh_isatree(i))
        {
            sprite[i].cstat = 1;
            sprite[i].xoffset = 0;
            sprite[i].yoffset = 0;
            sprite[i].clipdist = 8;
        }
        if (sprite[i].picnum == 998 || sprite[i].picnum == 999 || sprite[i].picnum == 1007 || sprite[i].picnum == 1008)
        {
            sprite[i].cstat = 32;
            sprite[i].clipdist = 127;
        }
        if (sprite[i].picnum)
        {
            v1c++;
        }
    }
    ghprelvl_randkeep(PIG, 4);
    ghprelvl_randkeep(VIXEN, 4);
    ghprelvl_randkeep(DOGRUN, 4);
    ghprelvl_randkeep(CHEER, 4);
    ghprelvl_randkeep(7065, 64);

    for (i = 0; i < MAXSPRITES; i++)
    {
        if (sprite[i].picnum == PIG)
        {
            sprite[i].cstat = 257;
            changespritestat(i, 1);
            sprite[i].xrepeat = 10 + sub_52AB8(5, 125);
            sprite[i].yrepeat = 10 + sub_52AB8(5, 125);
            sprite[i].clipdist = mulscale7(sprite[i].xrepeat, tilesiz[sprite[i].picnum].x);
        }
        if (sprite[i].picnum == VIXEN)
        {
            sprite[i].cstat = 257;
            changespritestat(i, 1);
            sprite[i].xrepeat = 14 + sub_52AB8(7, 100);
            sprite[i].yrepeat = 14 + sub_52AB8(7, 100);
            sprite[i].clipdist = mulscale7(sprite[i].xrepeat, tilesiz[sprite[i].picnum].x);
        }
        if (sprite[i].picnum == DOGRUN)
        {
            sprite[i].cstat = 257;
            changespritestat(i, 1);
            sprite[i].xrepeat = 8 + sub_52AB8(4, 100);
            sprite[i].yrepeat = 8 + sub_52AB8(4, 100);
            sprite[i].clipdist = mulscale7(sprite[i].xrepeat, tilesiz[sprite[i].picnum].x);
        }
        if (sprite[i].picnum == CHEER)
        {
            sprite[i].cstat = 257;
            changespritestat(i, 1);
            sprite[i].xrepeat = 8 + sub_52AB8(4, 100);
            sprite[i].yrepeat = 8 + sub_52AB8(4, 100);
            sprite[i].clipdist = mulscale7(sprite[i].xrepeat, tilesiz[sprite[i].picnum].x);
        }
        if (sprite[i].picnum == 7065)
        {
            sprite[i].cstat = 0;
            sprite[i].z -= rrdh_random() << 3;
            changespritestat(i, 801);
        }
    }

    for (i = 0; i < numwalls; i++)
    {
        if (wall[i].nextsector != -1)
        {
            wall[i].cstat &= ~64;
        }
    }
    sub_55F8C();
    sub_566F0();
    sub_558F4();
    sub_56AB8();
    sub_573C0();
    sub_59314();
    ghsound_preload(ud.level_number);
    sub_55184();
    sub_57B24();
    sub_58388();
    sub_59B50();
    sub_59F80(ud.level_number);
    sub_299C0();
}

void sub_53154(int a1)
{
    ghsound_ambientloop(a1);
}

int dword_AA2F0, dword_AA2F4, dword_AA2F8, dword_AA2FC;

void sub_53160(int a1)
{
    dword_AA2FC = a1;
    dword_AA2F8 = 1;
}

void sub_53194(void)
{
    switch (dword_AA2F0)
    {
    case 0:
        if (KB_KeyPressed(sc_S))
        {
            KB_ClearKeyDown(sc_S);
            dword_AA2F0++;
        }
        break;
    case 1:
        if (KB_KeyPressed(sc_P))
        {
            KB_ClearKeyDown(sc_P);
            dword_AA2F0++;
        }
        break;
    case 2:
        if (KB_KeyPressed(sc_O))
        {
            KB_ClearKeyDown(sc_O);
            dword_AA2F0++;
        }
        break;
    case 3:
        if (KB_KeyPressed(sc_R))
        {
            KB_ClearKeyDown(sc_R);
            dword_AA2F0++;
        }
        break;
    case 4:
        if (KB_KeyPressed(sc_K))
        {
            KB_ClearKeyDown(sc_K);
            dword_AA2F0++;
        }
        break;
    case 5:
        if (KB_KeyPressed(sc_1))
        {
            KB_ClearKeyDown(sc_1);
            dword_AA2F0 = 0;
            sub_535DC();
        }
        if (KB_KeyPressed(sc_2))
        {
            KB_ClearKeyDown(sc_2);
            dword_AA2F0 = 0;
            sub_57AC0();
        }
        if (KB_KeyPressed(sc_3))
        {
            KB_ClearKeyDown(sc_3);
            dword_AA2F0 = 0;
            //sub_15224();
        }
        break;
    }
}

void sub_53304(void)
{
    //ControlInfo info;
    //CONTROL_GetInput(&info);

    if (KB_KeyPressed(sc_RightAlt) || KB_KeyPressed(sc_LeftAlt) || KB_KeyPressed(sc_RightShift) || KB_KeyPressed(sc_LeftShift))
        return;

    sub_53194();
    if (KB_KeyPressed(sc_F1))
    {
        KB_ClearKeyDown(sc_F1);
        switch (g_player[myconnectindex].ps->dhat61f)
        {
        case 0:
            sub_566E8();
            break;
        case 1:
        case 2:
            sub_55F68();
            break;
        case 3:
            sub_558D0();
            break;
        case 4:
            sub_56AB0();
            break;
        }
    }
    if (KB_KeyPressed(sc_F2))
    {
        KB_ClearKeyDown(sc_F2);
        switch (g_player[myconnectindex].ps->dhat61f)
        {
        case 0:
            sub_56780();
            break;
        case 1:
        case 2:
            sub_56020();
            break;
        case 3:
            sub_55988();
            break;
        case 4:
            sub_56B3C();
            break;
        }
    }
    if (KB_KeyPressed(sc_F3))
    {
        KB_ClearKeyDown(sc_F3);
        switch (g_player[myconnectindex].ps->dhat61f)
        {
        case 0:
            sub_56724();
            break;
        case 1:
        case 2:
            sub_55FCC();
            break;
        case 3:
            sub_55934();
            break;
        case 4:
            sub_56AE4();
            break;
        }
    }
    if (KB_KeyPressed(sc_F4))
    {
        KB_ClearKeyDown(sc_F4);
        ghdeploy_drop(myconnectindex, ud.level_number);
    }
    if (KB_KeyPressed(sc_F5))
    {
        KB_ClearKeyDown(sc_F5);
        A_PlaySound(40, g_player[myconnectindex].ps->i);
    }
    if (KB_KeyPressed(sc_F6))
    {
        KB_ClearKeyDown(sc_F6);
        A_PlaySound(42, g_player[myconnectindex].ps->i);
    }
    if (KB_KeyPressed(sc_F7))
    {
        KB_ClearKeyDown(sc_F7);
        A_PlaySound(41, g_player[myconnectindex].ps->i);
    }
    if (KB_KeyPressed(sc_F8))
    {
        KB_ClearKeyDown(sc_F8);
        A_PlaySound(43, g_player[myconnectindex].ps->i);
    }
    //if (KB_KeyPressed(sc_S))
    //{
    //    KB_ClearKeyDown(sc_S);
    //}
}

void sub_535DC(void)
{
    dword_AA2F4 ^= 1;
}

int sub_535EC(void)
{
    return dword_AA2F4;
}

int dword_AA300;
int dword_AA304 = 1731;
int dword_AA308, dword_AA30C;

struct struct2B80E0 {
    short f_0;
    short f_2;
    int f_4;
    int f_8;
    int f_c;
    int f_10;
};

struct2B80E0 f2B80E0[20];

#pragma pack(push, 1)
typedef struct _scoretype2 {
    int f_0;
    char f_4;
    char f_5;
    char f_6;
    int f_7;
    int f_b;
} scoretype2;

typedef struct _scoretype {
    int f_0;
    int f_4;
    int f_8;
    int f_c;
    scoretype2 f_10[5];
} scoretype;
#pragma pack(pop)


scoretype bestscore = {
    0, 0, 0, 0,
    0, 65, 65, 65, 0, 0,
    0, 65, 65, 65, 0, 0,
    0, 65, 65, 65, 0, 0,
    0, 65, 65, 65, 0, 0,
    0, 65, 65, 65, 0, 0,
};

unsigned int dword_AA36C, dword_AA370, dword_AA374, dword_AA378, dword_AA37C, dword_AA380;
int dword_AA384;
char byte_AA388 = 65, byte_AA389 = 65, byte_AA38A = 65;
unsigned int dword_AA38C;

char dword_AA390[43] = "                                          ";

void ghtrophy_savebestscores(void)
{
    FileWriter *handle;

    FString filename = M_GetDocumentsPath() + "scores";

    handle = FileWriter::Open(filename);
    if (!handle)
    {
        Printf("ghtrophy_savebestscores: cannot open scores\n"); // this is not an error!
        return;
    }
    if (dword_AA36C > bestscore.f_0)
        bestscore.f_0 = dword_AA36C;
    if (dword_AA370 > bestscore.f_4)
        bestscore.f_4 = dword_AA370;
    if (dword_AA374 > bestscore.f_8)
        bestscore.f_8 = dword_AA374;
    if (dword_AA378 > bestscore.f_c)
        bestscore.f_c = dword_AA378;

    if (handle->Write(&bestscore, sizeof(bestscore)) != sizeof(bestscore))
    {
        Printf("ghtrophy_savebestscores: error writing scores\n");
        delete handle;
        return;
    }
    delete handle;
    dword_AA380 = 0;
}

void ghtrophy_loadbestscores(void)
{
    FileReader handle;
    FString filename = M_GetDocumentsPath() + "scores";
    if (!handle.OpenFile(filename))
    {
        // This is not an error.
        return;
    }
    if (handle.Read(&bestscore, sizeof(bestscore)) != sizeof(bestscore))
    {
        Printf("ghtrophy_loadbestscores err read scores\n");
        memset(&bestscore, 0, sizeof(bestscore));
    }
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

int sub_537A8(short a1, int a2)
{
    char va = rrdh_random() & 255;
    switch (DYNAMICTILEMAP(a1))
    {
    case PIG__STATICRR:
        if (a2 == 0 && va > 64)
            break;
        if (a2 == 6 && va > 80)
            break;
        if (a2 == 2 && va > 128)
            break;
        return 1;
    case VIXEN__STATICRR:
        if (a2 == 6 && va > 128)
            break;
        return 1;
    case CHEER__STATICRR:
        return 1;
    case DOGRUN__STATICRR:
        return 1;
    }
    return 0;
}

int sub_5381C(int a1, int a2)
{
    int vbx = (a1 * a2) / 100;
    return rrdh_random() % vbx;
}

void sub_53848(int a1)
{
    dword_AA37C = a1;
}

void ghtrophy_addkill(int a1)
{
    int v18 = 0, vdi = 0;
    spritetype *spr = &sprite[a1];

    if (ud.level_number > 3)
        return;

    if (sprite[a1].cstat & 32768)
        return;

    switch (DYNAMICTILEMAP(sprite[a1].picnum))
    {
    default:
        sub_5A250(4);
        break;
    case PIG__STATICRR:
    case DOGRUN__STATICRR:
    case VIXEN__STATICRR:
    case CHEER__STATICRR:
        if (!ghtrophy_isakill(a1) && sub_537A8(sprite[a1].picnum, v18))
        {
            if (dword_AA300 < 20)
            {
                f2B80E0[dword_AA300].f_0 = a1;
                f2B80E0[dword_AA300].f_2 = sprite[a1].picnum;
                switch (DYNAMICTILEMAP(sprite[a1].picnum))
                {
                case VIXEN__STATICRR:
                    f2B80E0[dword_AA300].f_4 = (sprite[a1].xrepeat * sprite[a1].yrepeat) / 40;
                    f2B80E0[dword_AA300].f_4 -= 2;
                    if (f2B80E0[dword_AA300].f_4 < 2)
                        f2B80E0[dword_AA300].f_4 = 2;
                    f2B80E0[dword_AA300].f_4 += 2;
                    if (f2B80E0[dword_AA300].f_4 > dword_AA36C)
                        dword_AA36C = f2B80E0[dword_AA300].f_4;
                    dword_AA308++;
                    vdi = dword_AA308;
                    break;
                case PIG__STATICRR:
                    f2B80E0[dword_AA300].f_4 = sprite[a1].xrepeat * sprite[a1].yrepeat;
                    f2B80E0[dword_AA300].f_4 += sub_5381C(30, 125);
                    f2B80E0[dword_AA300].f_4 += sub_5381C(10, 100);
                    if (f2B80E0[dword_AA300].f_4 > 350)
                        f2B80E0[dword_AA300].f_4 = 350;
                    if (f2B80E0[dword_AA300].f_4 > dword_AA370)
                        dword_AA370 = f2B80E0[dword_AA300].f_4;
                    dword_AA30C++;
                    vdi = dword_AA30C;
                    break;
                case DOGRUN__STATICRR:
                    f2B80E0[dword_AA300].f_4 = (sprite[a1].xrepeat * sprite[a1].yrepeat) / 40;
                    f2B80E0[dword_AA300].f_4 += sub_5381C(4, 125);
                    f2B80E0[dword_AA300].f_4 -= 3;
                    if (f2B80E0[dword_AA300].f_4 < 2)
                        f2B80E0[dword_AA300].f_4 = 2;
                    dword_AA378++;
                    vdi = dword_AA378;
                    break;
                case CHEER__STATICRR:
                    f2B80E0[dword_AA300].f_4 = (sprite[a1].xrepeat * sprite[a1].yrepeat) / 16;
                    f2B80E0[dword_AA300].f_4 += sub_5381C(10, 125);
                    if (f2B80E0[dword_AA300].f_4 < 8)
                        f2B80E0[dword_AA300].f_4 = 8;
                    dword_AA374++;
                    vdi = dword_AA374;
                    break;
                }
                ghstatbr_registerkillinfo(f2B80E0[dword_AA300].f_2, f2B80E0[dword_AA300].f_4, vdi);
                dword_AA300++;
            }
            sub_5A250(8);
        }
        break;
    }
}

void ghtrophy_rscopysrcdest(scoretype2 *a1, scoretype2 *a2)
{
    if (!a1 || !a2)
    {
        Printf("ghtrophy_rscopysrcdest null ptr\n");
        return;
    }
    a2->f_0 = a1->f_0;
    a2->f_4 = a1->f_4;
    a2->f_5 = a1->f_5;
    a2->f_6 = a1->f_6;
}

void sub_53C04(void)
{
    scoretype2 v60, v50, v40, v30, v20;
    dword_AA380 = 0;
    dword_AA384 = 0;
    dword_AA38C = (int)totalclock;
    if (dword_AA37C > bestscore.f_10[0].f_0)
    {
        v20.f_0 = dword_AA37C;
        ghtrophy_rscopysrcdest(&bestscore.f_10[0], &v50);
        ghtrophy_rscopysrcdest(&bestscore.f_10[1], &v30);
        ghtrophy_rscopysrcdest(&bestscore.f_10[2], &v40);
        ghtrophy_rscopysrcdest(&bestscore.f_10[3], &v60);
        dword_AA380 = 1;
    }
    else if (dword_AA37C > bestscore.f_10[1].f_0)
    {
        ghtrophy_rscopysrcdest(&bestscore.f_10[0], &v20);
        v50.f_0 = dword_AA37C;
        ghtrophy_rscopysrcdest(&bestscore.f_10[1], &v30);
        ghtrophy_rscopysrcdest(&bestscore.f_10[2], &v40);
        ghtrophy_rscopysrcdest(&bestscore.f_10[3], &v60);
        dword_AA380 = 2;
    }
    else if (dword_AA37C > bestscore.f_10[2].f_0)
    {
        ghtrophy_rscopysrcdest(&bestscore.f_10[0], &v20);
        ghtrophy_rscopysrcdest(&bestscore.f_10[1], &v50);
        v30.f_0 = dword_AA37C;
        ghtrophy_rscopysrcdest(&bestscore.f_10[2], &v40);
        ghtrophy_rscopysrcdest(&bestscore.f_10[3], &v60);
        dword_AA380 = 3;
    }
    else if (dword_AA37C > bestscore.f_10[3].f_0)
    {
        ghtrophy_rscopysrcdest(&bestscore.f_10[0], &v20);
        ghtrophy_rscopysrcdest(&bestscore.f_10[1], &v50);
        ghtrophy_rscopysrcdest(&bestscore.f_10[2], &v30);
        v40.f_0 = dword_AA37C;
        ghtrophy_rscopysrcdest(&bestscore.f_10[3], &v60);
        dword_AA380 = 4;
    }
    else if (dword_AA37C > bestscore.f_10[4].f_0)
    {
        ghtrophy_rscopysrcdest(&bestscore.f_10[0], &v20);
        ghtrophy_rscopysrcdest(&bestscore.f_10[1], &v50);
        ghtrophy_rscopysrcdest(&bestscore.f_10[2], &v30);
        ghtrophy_rscopysrcdest(&bestscore.f_10[3], &v40);
        v60.f_0 = dword_AA37C;
        dword_AA380 = 5;
    }
    if (dword_AA380)
    {
        ghtrophy_rscopysrcdest(&v20, &bestscore.f_10[0]);
        ghtrophy_rscopysrcdest(&v50, &bestscore.f_10[1]);
        ghtrophy_rscopysrcdest(&v30, &bestscore.f_10[2]);
        ghtrophy_rscopysrcdest(&v40, &bestscore.f_10[3]);
        ghtrophy_rscopysrcdest(&v60, &bestscore.f_10[4]);
    }
}

void sub_53E18(void)
{
    sub_53C04();
    if (dword_AA380)
    {
        sub_53160(300);
    }
}

void sub_53E4C(vec2_t const origin)
{
    int v20 = 0, v1c, v18, vsi, vdi;
    char val;
    val = sub_54B80();
    if (val & 32)
    {
        v20 = 1;
        switch (dword_AA384)
        {
        case 0:
            byte_AA388 = val;
            break;
        case 1:
            byte_AA389 = val;
            break;
        case 2:
            byte_AA38A = val;
            break;
        }
    }

    v1c = 0;
    if ((int)totalclock - dword_AA38C > 30)
    {
        v1c = 1;
        dword_AA38C = (int)totalclock;
    }

    v18 = 18;
    vsi = 100;
    vdi = 44;

    menutext_(origin.x+(160<<16), origin.y+(22<<16), 0, "TOP FIVE", 10|16, TEXT_XCENTER);

    if (dword_AA380)
    {
        if (!v1c || dword_AA384 != 0)
        {
            sprintf(dword_AA390, "%c", byte_AA388);
            gametext_simple(origin.x+(vsi<<16), origin.y+((vdi+v18*(dword_AA380-1))<<16), dword_AA390);
        }
        if (!v1c || dword_AA384 != 1)
        {
            sprintf(dword_AA390, "%c", byte_AA389);
            gametext_simple(origin.x+((vsi+14)<<16), origin.y+((vdi+v18*(dword_AA380-1))<<16), dword_AA390);
        }
        if (!v1c || dword_AA384 != 2)
        {
            sprintf(dword_AA390, "%c", byte_AA38A);
            gametext_simple(origin.x+((vsi+28)<<16), origin.y+((vdi+v18*(dword_AA380-1))<<16), dword_AA390);
        }
        switch (dword_AA380)
        {
        case 1:
            bestscore.f_10[0].f_4 = byte_AA388;
            bestscore.f_10[0].f_5 = byte_AA389;
            bestscore.f_10[0].f_6 = byte_AA38A;
            break;
        case 2:
            bestscore.f_10[1].f_4 = byte_AA388;
            bestscore.f_10[1].f_5 = byte_AA389;
            bestscore.f_10[1].f_6 = byte_AA38A;
            break;
        case 3:
            bestscore.f_10[2].f_4 = byte_AA388;
            bestscore.f_10[2].f_5 = byte_AA389;
            bestscore.f_10[2].f_6 = byte_AA38A;
            break;
        case 4:
            bestscore.f_10[3].f_4 = byte_AA388;
            bestscore.f_10[3].f_5 = byte_AA389;
            bestscore.f_10[3].f_6 = byte_AA38A;
            break;
        case 5:
            bestscore.f_10[4].f_4 = byte_AA388;
            bestscore.f_10[4].f_5 = byte_AA389;
            bestscore.f_10[4].f_6 = byte_AA38A;
            break;
        }
    }
    if (dword_AA380 != 1)
    {
        sprintf(dword_AA390, "%c", bestscore.f_10[0].f_4);
        gametext_simple(origin.x+(vsi<<16), origin.y+(vdi<<16), dword_AA390);
        sprintf(dword_AA390, "%c", bestscore.f_10[0].f_5);
        gametext_simple(origin.x+((vsi+14)<<16), origin.y+(vdi<<16), dword_AA390);
        sprintf(dword_AA390, "%c", bestscore.f_10[0].f_6);
        gametext_simple(origin.x+((vsi+28)<<16), origin.y+(vdi<<16), dword_AA390);
    }
    sprintf(dword_AA390, "%5d", bestscore.f_10[0].f_0);
    gametext_simple(origin.x+((vsi+74)<<16), origin.y+(vdi<<16), dword_AA390);
    if (dword_AA380 != 2)
    {
        sprintf(dword_AA390, "%c", bestscore.f_10[1].f_4);
        gametext_simple(origin.x+(vsi<<16), origin.y+((vdi+v18)<<16), dword_AA390);
        sprintf(dword_AA390, "%c", bestscore.f_10[1].f_5);
        gametext_simple(origin.x+((vsi+14)<<16), origin.y+((vdi+v18)<<16), dword_AA390);
        sprintf(dword_AA390, "%c", bestscore.f_10[1].f_6);
        gametext_simple(origin.x+((vsi+28)<<16), origin.y+((vdi+v18)<<16), dword_AA390);
    }
    sprintf(dword_AA390, "%5d", bestscore.f_10[1].f_0);
    gametext_simple(origin.x+((vsi+74)<<16), origin.y+((vdi+v18)<<16), dword_AA390);
    if (dword_AA380 != 3)
    {
        sprintf(dword_AA390, "%c", bestscore.f_10[2].f_4);
        gametext_simple(origin.x+(vsi<<16), origin.y+((vdi+v18*2)<<16), dword_AA390);
        sprintf(dword_AA390, "%c", bestscore.f_10[2].f_5);
        gametext_simple(origin.x+((vsi+14)<<16), origin.y+((vdi+v18*2)<<16), dword_AA390);
        sprintf(dword_AA390, "%c", bestscore.f_10[2].f_6);
        gametext_simple(origin.x+((vsi+28)<<16), origin.y+((vdi+v18*2)<<16), dword_AA390);
    }
    sprintf(dword_AA390, "%5d", bestscore.f_10[2].f_0);
    gametext_simple(origin.x+((vsi+74)<<16), origin.y+((vdi+v18*2)<<16), dword_AA390);
    if (dword_AA380 != 4)
    {
        sprintf(dword_AA390, "%c", bestscore.f_10[3].f_4);
        gametext_simple(origin.x+(vsi<<16), origin.y+((vdi+v18*3)<<16), dword_AA390);
        sprintf(dword_AA390, "%c", bestscore.f_10[3].f_5);
        gametext_simple(origin.x+((vsi+14)<<16), origin.y+((vdi+v18*3)<<16), dword_AA390);
        sprintf(dword_AA390, "%c", bestscore.f_10[3].f_6);
        gametext_simple(origin.x+((vsi+28)<<16), origin.y+((vdi+v18*3)<<16), dword_AA390);
    }
    sprintf(dword_AA390, "%5d", bestscore.f_10[3].f_0);
    gametext_simple(origin.x+((vsi+74)<<16), origin.y+((vdi+v18*3)<<16), dword_AA390);
    if (dword_AA380 != 5)
    {
        sprintf(dword_AA390, "%c", bestscore.f_10[4].f_4);
        gametext_simple(origin.x+(vsi<<16), origin.y+((vdi+v18*4)<<16), dword_AA390);
        sprintf(dword_AA390, "%c", bestscore.f_10[4].f_5);
        gametext_simple(origin.x+((vsi+14)<<16), origin.y+((vdi+v18*4)<<16), dword_AA390);
        sprintf(dword_AA390, "%c", bestscore.f_10[4].f_6);
        gametext_simple(origin.x+((vsi+28)<<16), origin.y+((vdi+v18*4)<<16), dword_AA390);
    }
    sprintf(dword_AA390, "%5d", bestscore.f_10[4].f_0);
    gametext_simple(origin.x+((vsi+74)<<16), origin.y+((vdi+v18*4)<<16), dword_AA390);

    if (KB_KeyPressed(sc_LeftArrow) || KB_KeyPressed(sc_kpad_4))
    {
        KB_ClearKeyDown(sc_LeftArrow);
        KB_ClearKeyDown(sc_kpad_4);
        dword_AA384--;
    }
    else if (v20 || KB_KeyPressed(sc_RightArrow) || KB_KeyPressed(sc_kpad_6))
    {
        KB_ClearKeyDown(sc_RightArrow);
        KB_ClearKeyDown(sc_kpad_6);
        dword_AA384++;
    }
    if (dword_AA384 < 0)
        dword_AA384 = 0;
    if (dword_AA384 > 2)
        dword_AA384 = 0;
    if (dword_AA380)
    {
        gametext_simple(origin.x+(86<<16), origin.y+(146<<16), "PRESS ESC WHEN DONE");
    }
}

void sub_5469C(vec2_t const origin, int a1)
{
    unsigned int v20, v1c, v18, v24, vdi, t;
    if (a1 == 2)
    {
        sub_53E4C(origin);
        return;
    }
    rotatesprite_fs(origin.x+(160<<16), origin.y, 65536, 0, dword_AA304, 0, 0, 64+16+10);
    if (a1 == 1)
    {
        v20 = bestscore.f_0;
        v1c = bestscore.f_4;
        v18 = bestscore.f_c;
        v24 = bestscore.f_10[0].f_0;
        vdi = bestscore.f_8;
    }
    else
    {
        v20 = dword_AA36C;
        v1c = dword_AA370;
        v18 = dword_AA378;
        v24 = dword_AA37C;
        vdi = dword_AA374;
    }
    if (v20 > 0)
    {
        rotatesprite_fs(origin.x+(4<<16), origin.y, 36864, 0, 1741, 0, 0, 16+10);
    }
    if (v1c > 0)
    {
        rotatesprite_fs(origin.x+(98<<16), origin.y+(6<<16), 36864, 0, 1740, 0, 0, 16+10);
    }
    if (vdi > 0)
    {
        rotatesprite_fs(origin.x+(172<<16), origin.y+(128<<16), 28576, 0, 1743, 0, 0, 16+10);
    }
    if (v18 > 0)
    {
        rotatesprite_fs(origin.x+(232<<16), origin.y+(128<<16), 28576, 0, 1742, 0, 0, 16+10);
    }
    if (a1 == 0)
        gametext_simple(origin.x+(4<<16), origin.y+(104<<16), "SO FAR IN THIS HUNTIN' TRIP...");
    else
        gametext_simple(origin.x+(4<<16), origin.y+(104<<16), "YOUR BEST TOTALS TO DATE...");

    sprintf(dword_AA390, "BEST DEER:    %2d PTS", v20);
    gametext_simple(origin.x+(8<<16), origin.y+(122<<16), dword_AA390);
    sprintf(dword_AA390, "BEST BOAR:    %3d  LB", v1c);
    gametext_simple(origin.x+(8<<16), origin.y+(136<<16), dword_AA390);
    sprintf(dword_AA390, "MOST TURKEYS: %2d    ", vdi);
    gametext_simple(origin.x+(8<<16), origin.y+(150<<16), dword_AA390);
    sprintf(dword_AA390, "MOST DUCKS:   %2d    ", v18);
    gametext_simple(origin.x+(8<<16), origin.y+(164<<16), dword_AA390);
    if (a1 == 0)
    {
        t = dword_AA308 + dword_AA30C + v18;
        if (t > 6)
            gametext_simple(origin.x+(8<<16), origin.y+(182<<16), "YEAH BABY !");
        else if (t > 4)
            gametext_simple(origin.x+(8<<16), origin.y+(182<<16), "NICE WORK !");
        else if (t > 1)
            gametext_simple(origin.x+(8<<16), origin.y+(182<<16), "GOOD JOB !");
        else
            gametext_simple(origin.x+(8<<16), origin.y+(182<<16), "KEEP TRYIN' !");
    }
    else
    {
        sprintf(dword_AA390, "BEST RANGE SCORE: %5d", v24);
        gametext_simple(origin.x+(8<<16), origin.y+(182<<16), dword_AA390);
    }
}

void sub_54A2C(void)
{
    int i;
    dword_AA300 = 0;
    dword_AA304 = 1731+(rrdh_random()%6);
    dword_AA308 = 0;
    dword_AA30C = 0;
    for (i = 0; i < 20; i++)
    {
        f2B80E0[i].f_0 = -1;
        f2B80E0[i].f_2 = 0;
        f2B80E0[i].f_4 = 0;
        f2B80E0[i].f_8 = 0;
        f2B80E0[i].f_c = 0;
        f2B80E0[i].f_10 = 0;
    }
    dword_AA36C = 0;
    dword_AA370 = 0;
    dword_AA374 = 0;
    dword_AA378 = 0;
    dword_AA37C = 0;
    dword_AA380 = 0;
    dword_AA384 = 0;
    byte_AA388 = 65;
    byte_AA389 = 65;
    byte_AA38A = 65;
    dword_AA38C = 0;
    ghtrophy_loadbestscores();
}

char sub_54B80(void)
{
#if 0
    switch (KB_GetLastScanCode())
    {
    case sc_A:
        KB_SetLastScanCode(sc_None);
        return 'A';
    case sc_B:
        KB_SetLastScanCode(sc_None);
        return 'B';
    case sc_C:
        KB_SetLastScanCode(sc_None);
        return 'C';
    case sc_D:
        KB_SetLastScanCode(sc_None);
        return 'D';
    case sc_E:
        KB_SetLastScanCode(sc_None);
        return 'E';
    case sc_F:
        KB_SetLastScanCode(sc_None);
        return 'F';
    case sc_G:
        KB_SetLastScanCode(sc_None);
        return 'G';
    case sc_H:
        KB_SetLastScanCode(sc_None);
        return 'H';
    case sc_I:
        KB_SetLastScanCode(sc_None);
        return 'I';
    case sc_J:
        KB_SetLastScanCode(sc_None);
        return 'J';
    case sc_K:
        KB_SetLastScanCode(sc_None);
        return 'K';
    case sc_L:
        KB_SetLastScanCode(sc_None);
        return 'L';
    case sc_M:
        KB_SetLastScanCode(sc_None);
        return 'M';
    case sc_N:
        KB_SetLastScanCode(sc_None);
        return 'N';
    case sc_O:
        KB_SetLastScanCode(sc_None);
        return 'O';
    case sc_P:
        KB_SetLastScanCode(sc_None);
        return 'P';
    case sc_Q:
        KB_SetLastScanCode(sc_None);
        return 'Q';
    case sc_R:
        KB_SetLastScanCode(sc_None);
        return 'R';
    case sc_S:
        KB_SetLastScanCode(sc_None);
        return 'S';
    case sc_T:
        KB_SetLastScanCode(sc_None);
        return 'T';
    case sc_U:
        KB_SetLastScanCode(sc_None);
        return 'U';
    case sc_V:
        KB_SetLastScanCode(sc_None);
        return 'V';
    case sc_W:
        KB_SetLastScanCode(sc_None);
        return 'W';
    case sc_X:
        KB_SetLastScanCode(sc_None);
        return 'X';
    case sc_Y:
        KB_SetLastScanCode(sc_None);
        return 'Y';
    case sc_Z:
        KB_SetLastScanCode(sc_None);
        return 'Z';
    default:
        KB_SetLastScanCode(sc_None);
        return ' ';
    }
#endif
    return 0;
}

char byte_AA394[6] = { 0, 1, 2, 3, 2, 1 };

int dword_AA39C, dword_AA3A0, dword_AA3A4, dword_AA3A8, dword_AA3AC, dword_AA3B0, dword_AA3B4, dword_AA3B8, dword_AA3BC, dword_AA3C0, dword_AA3C4;

void sub_54D90(void)
{
    rotatesprite(160<<16, 100<<16, 32768, 0, 7063, -24, 0, 32+2+1, windowxy1.x, windowxy1.y, windowxy2.x, windowxy2.y);
}

void sub_54DE0(void)
{
    dword_AA3A8 = tilesiz[7050].x;
    dword_AA3AC = tilesiz[7050].y;
#if 0
    tileCreate(7050, dword_AA3A8, dword_AA3AC);
#endif
}

void ghrender_preparescope(void)
{
#if 0
    int delta, i, j;
    char *ptr;
    dword_AA3A4 = 0;
    if (!waloff[7050])
        return;
    dword_AA3B4 = windowxy2.x - windowxy1.x + 1;
    dword_AA3B8 = windowxy2.y - windowxy1.y + 1;
    if (dword_AA3B4 <= dword_AA3A8 || dword_AA3B8 <= dword_AA3AC)
        return;
    delta = bytesperline - dword_AA3B4;
    if (delta < 0)
        G_GameExit("ghrender_preparescope: delta < 0");
    delta /= 2;

    dword_AA3B0 = (dword_AA3B8 * bytesperline) / 2;
    dword_AA3B0 -= (bytesperline >> 1);
    dword_AA3B0 -= delta;
    dword_AA3B0 -= (dword_AA3AC >> 1) * bytesperline;
    dword_AA3B0 -= (dword_AA3A8 >> 1);
    tileCopySection(7051, 0, 0, tilesiz[7051].x, tilesiz[7051].y, 7050, 0, 0);
    ptr = (char*)waloff[7050];
    if (ptr)
    {
        for (i = 0; i < dword_AA3A8; i++)
        {
            for (j = 0; j < dword_AA3AC; j++)
            {
                if (*ptr == 0)
                    *ptr = 255;
                ptr++;
            }
        }
        dword_AA3A4 = 1;
    }
#endif
}

void sub_54FA4(int a1, int a2)
{
    int i, j;
    char *ptr1, *ptr2;
    if (videoGetRenderMode() >= REND_POLYMOST)
        return;
#if 0
    if (!dword_AA3A4)
        ghrender_preparescope();
    ptr1 = (char*)waloff[7050];
    if (!ptr1)
        return;
    ptr2 = (char*)frameplace;
    if (!ptr2)
        return;
    for (i = 0; i < dword_AA3A8; i++)
    {
        for (j = 0; j < dword_AA3AC; j++)
        {
            if (*ptr1 != 255)
            {
                *ptr1 = ptr2[i*bytesperline+dword_AA3B0+j];
            }
            ptr1++;
        }
    }
    rotatesprite(a1<<16, a2<<16, 57344, 512, 7050, 0, 0, 4+2, windowxy1.x, windowxy1.y, windowxy2.x, windowxy2.y);
    rotatesprite(a1<<16, a2<<16, 57344, 512, 7050, -8, 0, 4+2+1, windowxy1.x, windowxy1.y, windowxy2.x, windowxy2.y);
    rotatesprite(a1<<16, a2<<16, 32768, 0, 7063, -24, 0, 32+2+1, windowxy1.x, windowxy1.y, windowxy2.x, windowxy2.y);
#endif
}

typedef struct _struct2B8280 {
    short f_0;
    char f_2;
} struct2B8280;

struct2B8280 f2B8280[MAXSPRITES];

void sub_550F0(void)
{
    int i, sect;
    dword_AA3A0 = 0;
    for (i = 0; i < MAXSPRITES; i++)
    {
        f2B8280[i].f_0 = -1;
        sect = sprite[i].sectnum;
        if (sect >= 0 && sect < numsectors)
        {
            if (sprite[i].cstat & 2)
            {
                if (sector[sect].floorstat & 1)
                {
                    f2B8280[dword_AA3A0].f_2 = sprite[i].xrepeat;
                    f2B8280[dword_AA3A0].f_0 = i;
                    dword_AA3A0++;
                }
            }
        }
    }
    dword_AA39C = 0;
}

void ghrender_movewatersprites(void)
{
    int i, spr;
    if (!sub_57AA0(2))
        return;
    for (i = 0; i < dword_AA3A0; i++)
    {
        spr = f2B8280[i].f_0;
        if (spr < 0 || spr >= MAXSPRITES)
        {
            Printf("ghrender_movewatersprites: bad watersprite sprnum\n");
            continue;
        }
        if (dword_AA39C < 0 || dword_AA39C >= 6)
        {
            Printf("ghrender_movewatersprites: currepeat out of range\n");
            continue;
        }
        sprite[spr].xrepeat = f2B8280[i].f_2 + byte_AA394[dword_AA39C];
    }
    dword_AA39C++;
    if (dword_AA39C >= 6)
        dword_AA39C = 0;
}

typedef struct _structAA3D0 {
    unsigned int f_0;
    int f_4;
    int f_8;
    int f_c;
    int f_10;
} structAA3D0;

void sub_55244(void)
{
}

void sub_5524C(void)
{
}

void sub_55184(void)
{
    sub_550F0();
}

structAA3D0 fAA3D0[15] = {
    1, 2860, 136, 152, 0,
    10, 2861, 136, 142, 2,
    10, 2862, 136, 142, 5,
    10, 2862, 136, 142, 4,
    10, 2861, 136, 142, 0,
    4, 2863, 144, 162, 5,
    16, 2864, 204, 160, 5,
    22, 2865, 136, 142, 8,
    22, 2869, 136, 142, 9,
    22, 2868, 136, 142, 10,
    18, 2865, 136, 142, 5,
    22, 2865, 136, 142, 12,
    22, 2866, 136, 142, 13,
    22, 2867, 136, 142, 14,
    18, 2865, 136, 142, 5
};

int ghshtgn_setmode(int a1)
{
    if (a1 < 0 || a1 >= 15)
        return -1;
    if (dword_AA3BC != a1)
    {
        if ((int)totalclock - dword_AA3C4 > fAA3D0[dword_AA3BC].f_0)
        {
            switch (dword_AA3BC)
            {
            case 7:
            case 11:
                A_PlaySound(3, g_player[myconnectindex].ps->i);
                break;
            case 10:
            case 14:
                dword_AA3C0 = 1;
                break;
            case 6:
                dword_AA3C0 = 0;
                break;
            }
            dword_AA3C4 = (int)totalclock;
            dword_AA3BC = a1;
        }
    }
    return dword_AA3BC;
}

int dword_2BB290[MAXSECTORS];

int dword_AA3C8, dword_AA3CC;

int hitscan_old(int xs, int ys, int zs, int16_t sectnum, int xv, int yv, int zv, int16_t *hitsect, int16_t *hitwall, int16_t *hitsprite,
    int *xh, int *yh, int *zh, int cm)
{
    vec3_t s = { xs, ys, zs };
    hitdata_t h = { *xh, *yh, *zh, *hitsprite, *hitwall, *hitsect };
    int ret = hitscan(&s, sectnum, xv, yv, zv, &h, cm);
    *xh = h.pos.x;
    *yh = h.pos.y;
    *zh = h.pos.z;
    *hitsprite = h.sprite;
    *hitwall = h.wall;
    *hitsect = h.sect;
    return ret;
}

void ghshtgn_fire(short snum)
{
    DukePlayer_t *p;
    short v20 = 0;
    short v1c = 0;
    short v18 = 0;
    int v44 = 0;
    int v48 = 0;
    int v4c = 0;
    int v34, v28, v40, v2c, v24, v38, vdx, vax, vbx, vdi, v3c, v30, i;
    short sect;
    if (!dword_AA3C0 && dword_AA3BC == 5)
        sub_5A250(0x10);

    if (dword_AA3BC == 5 && dword_AA3C0 && ghshtgn_setmode(6) == 6)
    {
        p = g_player[snum].ps;
        A_PlaySound(4, p->i);
        v34 = 9;
        while (v34 > 0)
        {
            v28 = p->pos.x;
            v40 = p->pos.y;
            v2c = p->pos.z + p->pyoff;
            if (dword_AA3C8 == 7)
            {
                vax = 2;
                vbx = 4;
                v24 = 8;
                vdx = 2048;
                v38 = 4096;
            }
            else
            {
                vax = 2;
                vbx = 2;
                v24 = 4;
                vdx = 512;
                v38 = 1024;
            }
            vdi = (100 - fix16_to_int(p->q16horiz)) * 2048;
            switch (v34)
            {
            default:
                return;
            case 9:
                v3c = sintable[(fix16_to_int(p->q16ang) + vax - vbx + 512) & 2047];
                v30 = sintable[(fix16_to_int(p->q16ang) + vax - vbx) & 2047];
                vdi += vdx;
                break;
            case 8:
                v3c = sintable[(fix16_to_int(p->q16ang) + vax + vbx + 512) & 2047];
                v30 = sintable[(fix16_to_int(p->q16ang) + vax + vbx) & 2047];
                vdi += vdx;
                break;
            case 7:
                v3c = sintable[(fix16_to_int(p->q16ang) + vax - vbx + 512) & 2047];
                v30 = sintable[(fix16_to_int(p->q16ang) + vax - vbx) & 2047];
                vdi -= vdx;
                break;
            case 6:
                v3c = sintable[(fix16_to_int(p->q16ang) + vax + vbx + 512) & 2047];
                v30 = sintable[(fix16_to_int(p->q16ang) + vax + vbx) & 2047];
                vdi -= vdx;
                break;
            case 5:
                v3c = sintable[(fix16_to_int(p->q16ang) + vax + 512) & 2047];
                v30 = sintable[(fix16_to_int(p->q16ang) + vax) & 2047];
                vdi -= v38;
                break;
            case 4:
                v3c = sintable[(fix16_to_int(p->q16ang) + vax + 512) & 2047];
                v30 = sintable[(fix16_to_int(p->q16ang) + vax) & 2047];
                vdi += v38;
                break;
            case 3:
                v3c = sintable[(fix16_to_int(p->q16ang) + vax - v24 + 512) & 2047];
                v30 = sintable[(fix16_to_int(p->q16ang) + vax - v24) & 2047];
                break;
            case 2:
                v3c = sintable[(fix16_to_int(p->q16ang) + vax + v24 + 512) & 2047];
                v30 = sintable[(fix16_to_int(p->q16ang) + vax + v24) & 2047];
                break;
            case 1:
                v3c = sintable[(fix16_to_int(p->q16ang) + vax + 512) & 2047];
                v30 = sintable[(fix16_to_int(p->q16ang) + vax) & 2047];
                break;
            }
            for (i = 0; i < numsectors; i++)
            {
                dword_2BB290[i] = sector[i].ceilingz;
                sector[i].ceilingz = -0x64000;
            }
            hitscan_old(v28, v40, v2c, p->cursectnum, v3c, v30, vdi, &v20, &v18, &v1c, &v44, &v48, &v4c, CLIPMASK1);
            for (i = 0; i < numsectors; i++)
            {
                sector[i].ceilingz = dword_2BB290[i];
            }
            v34--;
            if (v20 < 0)
            {
                Printf("WARNING: ghshtgn_fire hitsect < 0\n");
                return;
            }
            sub_51678(v18, v1c, v20, v44, v48, v4c);
            if (v1c >= 0)
            {
                if (sprite[v1c].cstat == 32768)
                {
                    Printf("ERROR: hit spr with cstat 32768\n");
                    return;
                }
                sect = sprite[v1c].sectnum;
                if (sector[sect].hitag == 2000)
                {
                    Printf("ERROR: hit spr in REST_AREA sector\n");
                    return;
                }
                ghtrophy_addkill(v1c);
                ghtarget_hit(v1c, dword_AA3C8);
            }
            else
                sub_5A250(4);
        }
    }
}

void sub_558D0(void)
{
    if (!dword_AA3BC)
        ghshtgn_setmode(1);
    else
        ghshtgn_setmode(3);
}

void sub_558F4(void)
{
    dword_AA3CC = 1;
    ghshtgn_setmode(0);
    dword_AA3C8 = 6;
    dword_AA3C0 = 0;
}

int sub_55928(void)
{
    return dword_AA3C8;
}

void sub_55934(void)
{
    if (dword_AA3BC == 0 || dword_AA3BC == 5)
    {
        sub_5A250(0x4000);
        dword_AA3C0 = 0;
        sub_55988();
        dword_AA3C8 = 6 + (dword_AA3C8 == 6);
    }
}

void sub_55988(void)
{
    int pframe;
    if (dword_AA3BC == 0 || dword_AA3BC == 5)
    {
        pframe = dword_AA3BC;
        if (dword_AA3CC)
        {
            if (ghshtgn_setmode(7) == 7)
                fAA3D0[10].f_10 = pframe;
            dword_AA3CC = 0;
        }
        else
        {
            if (ghshtgn_setmode(11) == 11)
                fAA3D0[14].f_10 = pframe;
        }
    }
}

unsigned int dword_AA53C;
int dword_AA540;

int dword_AA4FC[] = {
    0, -1, -2, -3, -4, -3, -2, -1
};

int dword_AA51C[] = {
    0, -1, -2, -2, -1, -2, -2, -1
};

void ghshtgn_render(short snum)
{
    int vdx;
    if (dword_AA3BC < 0 || dword_AA3BC >= 15)
    {
        Printf("ERROR: ghshtgn_draw bad index\n");
        return;
    }
    if (snum < 0 || snum >= numplayers)
    {
        Printf("ERROR: ghshtgn_render bad index\n");
        return;
    }
    DukePlayer_t* p = g_player[snum].ps;
    if (p->dhat613 || p->dhat617)
    {
        vdx = 10;
        if (p->dhat617)
            vdx = 5;
        if ((int)totalclock - dword_AA53C > vdx)
        {
            dword_AA540++;
            if (dword_AA540 >= 8)
                dword_AA540 = 0;
            dword_AA53C = (int)totalclock;
        }
    }
    else
    {
        if (dword_AA540)
            dword_AA540++;
        if (dword_AA540 >= 8)
            dword_AA540 = 0;
    }
    if (dword_AA540 >= 8)
    {
        Printf("ERROR: ghshtgn_render bobcnt out of bounds\n");
        return;
    }
    sub_54D90();
    rotatesprite_win((fAA3D0[dword_AA3BC].f_8+dword_AA4FC[dword_AA540])<<16,
        (fAA3D0[dword_AA3BC].f_c+dword_AA51C[dword_AA540]+17)<<16, 40960,
        0, fAA3D0[dword_AA3BC].f_4, 0, 0, 2);
    if (dword_AA3BC == 5 && p->dhat617)
        ghshtgn_setmode(3);
    else
        ghshtgn_setmode(fAA3D0[dword_AA3BC].f_10);
}

int dword_AA544, dword_AA548;
unsigned int dword_AA54C;

structAA3D0 fAA558[17] = {
    1, 2816, 135, 152, 0,
    10, 2822, 135, 132, 2,
    10, 2817, 135, 132, 5,
    10, 2817, 135, 132, 4,
    10, 2822, 135, 132, 0,
    4, 2818, 144, 130, 5,
    16, 2819, 144, 130, 5,
    28, 2817, 135, 132, 8,
    28, 2823, 135, 132, 9,
    28, 2824, 135, 132, 10,
    28, 2823, 135, 132, 11,
    28, 2817, 135, 132, 5,
    28, 2817, 135, 132, 13,
    28, 2820, 135, 132, 14,
    28, 2821, 135, 132, 15,
    28, 2823, 135, 132, 16,
    28, 2817, 135, 132, 5
};

structAA3D0 fAA6AC[17] = {
    1, 2830, 135, 152, 0,
    10, 2831, 135, 132, 2,
    10, 2832, 135, 132, 5,
    10, 2832, 135, 132, 4,
    10, 2831, 135, 132, 0,
    4, 2833, 146, 136, 5,
    16, 2834, 146, 136, 5,
    28, 2832, 135, 132, 8,
    28, 2837, 135, 132, 9,
    28, 2838, 135, 132, 10,
    28, 2837, 135, 132, 11,
    28, 2832, 135, 132, 5,
    28, 2832, 135, 132, 13,
    28, 2836, 135, 132, 14,
    28, 2835, 135, 132, 15,
    28, 2837, 135, 132, 16,
    28, 2832, 135, 132, 5
};

int ghrifle_setmode(int a1)
{
    if (a1 < 0 || a1 >= 17)
        return -1;
    if (dword_AA544 != a1)
    {
        if ((int)totalclock - dword_AA54C > fAA558[dword_AA544].f_0)
        {
            switch (dword_AA544)
            {
            case 7:
            case 12:
                A_PlaySound(5, g_player[myconnectindex].ps->i);
                break;
            case 11:
            case 16:
                dword_AA548 = 1;
                break;
            case 6:
                dword_AA548 = 0;
                break;
            }
            dword_AA54C = (int)totalclock;
            dword_AA544 = a1;
        }
    }
    return dword_AA544;
}

int dword_2BC2A0[MAXSECTORS];
int dword_AA550, dword_AA554;

void ghrifle_fire(short snum)
{
    DukePlayer_t *p;
    short v20 = 0;
    short v1c = 0;
    short v18 = 0;
    int v44 = 0;
    int v48 = 0;
    int v4c = 0;
    int v28, v40, v2c, vdi, v3c, v30, i;
    short sect;
    if (!dword_AA548 && dword_AA544 == 5)
        sub_5A250(0x10);

    if (dword_AA548 && dword_AA544 == 5 && ghrifle_setmode(6) == 6)
    {
        p = g_player[snum].ps;
        A_PlaySound(6, p->i);
        v28 = p->pos.x;
        v40 = p->pos.y;
        v2c = p->pos.z + p->pyoff;
        v3c = sintable[(fix16_to_int(p->q16ang) + 512) & 2047];
        v30 = sintable[(fix16_to_int(p->q16ang)) & 2047];
        vdi = (100 - fix16_to_int(p->q16horiz)) * 2048;
        for (i = 0; i < numsectors; i++)
        {
            dword_2BC2A0[i] = sector[i].ceilingz;
            sector[i].ceilingz = -0x64000;
        }
        hitscan_old(v28, v40, v2c, p->cursectnum, v3c, v30, vdi, &v20, &v18, &v1c, &v44, &v48, &v4c, CLIPMASK1);
        for (i = 0; i < numsectors; i++)
        {
            sector[i].ceilingz = dword_2BC2A0[i];
        }
        if (v20 < 0)
        {
            Printf("WARNING: ghrifle_fire hitsect < 0\n");
            return;
        }
        sub_51678(v18, v1c, v20, v44, v48, v4c);
        if (v1c >= 0)
        {
            if (sprite[v1c].cstat == 32768)
            {
                Printf("ERROR: hit spr with cstat 32768\n");
                return;
            }
            sect = sprite[v1c].sectnum;
            if (sector[sect].hitag == 2000)
            {
                Printf("ERROR: hit spr in REST_AREA sector\n");
                return;
            }
            ghtrophy_addkill(v1c);
            ghtarget_hit(v1c, dword_AA550);
        }
        else
            sub_5A250(4);
    }
}

void sub_55F68(void)
{
    if (dword_AA544 == 0)
        ghrifle_setmode(1);
    else
        ghrifle_setmode(3);
}

void sub_55F8C(void)
{
    dword_AA554 = 1;
    ghrifle_setmode(0);
    dword_AA550 = 4;
    dword_AA548 = 0;
}

int sub_55FC0(void)
{
    return dword_AA550;
}

void sub_55FCC(void)
{
    if (dword_AA544 == 0 || dword_AA544 == 5)
    {
        sub_5A250(0x4000);
        dword_AA548 = 0;
        sub_56020();
        dword_AA550 = 4 + (dword_AA550 == 4);
    }
}

void sub_56020(void)
{
    int pframe;
    if (dword_AA544 == 0 || dword_AA544 == 5)
    {
        pframe = dword_AA544;
        if (dword_AA554)
        {
            if (ghrifle_setmode(7) == 7)
                fAA558[11].f_10 = pframe;
            dword_AA554 = 0;
        }
        else
        {
            if (ghrifle_setmode(12) == 12)
                fAA558[16].f_10 = pframe;
        }
    }
}

unsigned int dword_AA840;
int dword_AA844;

int dword_AA800[] = {
    0, -1, -2, -3, -4, -3, -2, -1
};

int dword_AA820[] = {
    0, -1, -2, -2, -1, -2, -2, -1
};

void ghrifle_render(short snum, int a2)
{
    int vdx, tile, x, y;
    if (dword_AA544 < 0 || dword_AA544 >= 17)
    {
        Printf("ERROR: ghrifle_draw bad index\n");
        return;
    }
    if (snum < 0 || snum >= numplayers)
    {
        Printf("ERROR: ghrifle_render bad index\n");
        return;
    }
    DukePlayer_t* p = g_player[snum].ps;
    if (p->dhat613 || p->dhat617)
    {
        vdx = 10;
        if (p->dhat617)
            vdx = 5;
        if ((int)totalclock - dword_AA840 > vdx)
        {
            dword_AA844++;
            if (dword_AA844 >= 8)
                dword_AA844 = 0;
            dword_AA840 = (int)totalclock;
        }
    }
    else
    {
        if (dword_AA844)
            dword_AA844++;
        if (dword_AA844 >= 8)
            dword_AA844 = 0;
    }
    if (dword_AA844 >= 8)
    {
        Printf("ERROR: ghrifle_render bobcnt out of bounds\n");
        return;
    }

    if (a2 == 1)
    {
        x = fAA558[dword_AA544].f_8 + dword_AA800[dword_AA844];
        y = fAA558[dword_AA544].f_c + dword_AA820[dword_AA844] + 17;
        tile = fAA558[dword_AA544].f_4;
    }
    else
    {
        x = fAA6AC[dword_AA544].f_8 + dword_AA800[dword_AA844];
        y = fAA6AC[dword_AA544].f_c + dword_AA820[dword_AA844] + 17;
        tile = fAA6AC[dword_AA544].f_4;
    }

    if (dword_AA544 == 5 && a2 == 1)
        sub_54FA4(160+dword_AA800[dword_AA844], 100+dword_AA820[dword_AA844]);
    else
        sub_54D90();

    rotatesprite_win(x<<16, y<<16, 32768, 0, tile, 0, 0, 2);

    if (a2 == 1 && dword_AA544 == 5 && (p->dhat613 || p->dhat617))
    {
        ghrifle_setmode(3);
    }
    else
    {
        if (dword_AA544 == 5 && p->dhat617)
            ghrifle_setmode(3);
        else
            ghrifle_setmode(fAA558[dword_AA544].f_10);
    }
}

int dword_AA848;
int dword_AA84C;
unsigned int dword_AA850;
int dword_AA854;

structAA3D0 fAA858[15] = {
    1, 3328, 198, 152, 0,
    4, 3329, 198, 132, 2,
    4, 3330, 198, 132, 3,
    4, 3331, 198, 136, 4,
    4, 3332, 198, 136, 5,
    4, 3333, 198, 132, 6,
    4, 3334, 198, 132, 7,
    4, 3333, 198, 132, 8,
    4, 3332, 198, 132, 9,
    4, 3331, 198, 132, 0,
    12, 3336, 198, 132, 11,
    12, 3337, 198, 132, 12,
    12, 3338, 198, 132, 13,
    12, 3337, 198, 132, 14,
    12, 3336, 198, 132, 0
};

int dword_AA984[] = {
    0, -1, -2, -3, -4, -3, -2, -1
};
int dword_AA9A4[] = {
    0, -1, -2, -2, -1, -2, -2, -1
};

int ghpistol_setmode(int a1)
{
    if (a1 < 0 || a1 >= 15)
        return 0;
    if (a1 != dword_AA848)
    {
        if ((int)totalclock - dword_AA850 > fAA858[dword_AA848].f_0)
        {
            switch (dword_AA848)
            {
            case 10:
                A_PlaySound(1, g_player[myconnectindex].ps->i);
                break;
            case 14:
                dword_AA84C = 6;
                break;
            case 2:
                if (!dword_AA84C)
                {
                    Printf("ghpistol_setmode: pistolloaded at 0\n");
                }
                else
                    dword_AA84C--;
                break;
            }
            dword_AA850 = (int)totalclock;
            dword_AA848 = a1;
        }

    }
    return dword_AA848;
}

int dword_2BD2B0[MAXSECTORS];

void ghpistol_fire(short snum)
{
    DukePlayer_t *p;
    short v18 = 0;
    short v1c = 0;
    short v20 = 0;
    int v38 = 0;
    int v3c = 0;
    int v40 = 0;
    int v30, v34, v2c, vdx, vbx, v28, v24, vsi, i;
    if (dword_AA84C == 0 && dword_AA848 == 0)
        sub_5A250(16);
    if (dword_AA84C && dword_AA848 == 0 && ghpistol_setmode(1) == 1)
    {
        p = g_player[snum].ps;
        A_PlaySound(2, p->i);
        v30 = p->pos.x;
        v34 = p->pos.y;
        v2c = p->pos.z + p->pyoff;
        if (dword_AA854 == 3)
        {
            vdx = 4 - (rrdh_random() & 7);
            vbx = 2047 - (rrdh_random() & 2047);
        }
        else
        {
            vdx = 8 - (rrdh_random() & 15);
            vbx = 2048 - (rrdh_random() & 2048);
        }
        vdx += 2;
        v24 = sintable[(fix16_to_int(p->q16ang) + vdx + 512) & 2047];
        v28 = sintable[(fix16_to_int(p->q16ang) + vdx) & 2047];
        vsi = ((100 - fix16_to_int(p->q16horiz)) << 11) + vbx;

        for (i = 0; i < MAXSECTORS; i++)
        {
            dword_2BD2B0[i] = sector[i].ceilingz;
            sector[i].ceilingz = -0x64000;
        }
        hitscan_old(v30, v34, v2c, p->cursectnum, v24, v28, vsi, &v18, &v20, &v1c, &v38, &v3c, &v40, CLIPMASK1);
        for (i = 0; i < MAXSECTORS; i++)
        {
            sector[i].ceilingz = dword_2BD2B0[i];
        }
        if (v18 < 0)
        {
            Printf("WARNING: ghpistol_fire hitsect < 0\n");
            return;
        }
        sub_51678(v28, v1c, v18, v38, v3c, v40);
        if (v1c >= 0)
        {
            if (sprite[v1c].cstat == 32768)
            {
                Printf("ERROR: hit spr with cstat 32768\n");
                return;
            }
            if (sector[sprite[v1c].sectnum].hitag == 2000)
            {
                Printf("ERROR: hit spr in REST_AREA sector\n");
                return;
            }
            ghtrophy_addkill(v1c);
            ghtarget_hit(v1c, dword_AA854);
        }
        else
            sub_5A250(4);
    }

}

void sub_566E8(void)
{
}

void sub_566F0(void)
{
    ghpistol_setmode(0);
    dword_AA854 = 2;
    dword_AA84C = 0;
}

int sub_56718(void)
{
    return dword_AA854;
}

void sub_56724(void)
{
    if (dword_AA848 == 0)
    {
        sub_5A250(0x4000);
        dword_AA84C = 0;
        if (dword_AA848 == 0)
            ghpistol_setmode(10);
        dword_AA854 = 2 + (dword_AA854 == 2);
    }
}

void sub_56780(void)
{
    if (dword_AA848 == 0 && dword_AA84C != 6)
        ghpistol_setmode(10);
}

unsigned int dword_AA9C4;

int dword_AA9C8;

void ghpistol_render(short snum)
{
    int vdx;
    if (dword_AA848 < 0 || dword_AA848 >= 15)
    {
        Printf("ERROR: ghpistol_draw bad index\n");
        return;
    }
    if (snum < 0 || snum >= numplayers)
    {
        Printf("ERROR: ghpistol_render bad index\n");
        return;
    }
    DukePlayer_t *p = g_player[snum].ps;
    if (p->dhat613 || p->dhat617)
    {
        vdx = 10;
        if (p->dhat617)
            vdx = 5;

        if ((int)totalclock - dword_AA9C4 > vdx)
        {
            dword_AA9C8++;
            if (dword_AA9C8 >= 8)
                dword_AA9C8 = 0;
            dword_AA9C4 = (int)totalclock;
        }
    }
    else
    {
        if (dword_AA9C8)
            dword_AA9C8++;
        if (dword_AA9C8 >= 8)
            dword_AA9C8 = 0;
    }
    if (dword_AA9C8 >= 8)
    {
        Printf("ERROR: ghpistol_render bobcnt out of bounds\n");
        return;
    }
    sub_54D90();
    rotatesprite_win((fAA858[dword_AA848].f_8+dword_AA984[dword_AA9C8])<<16,
        (fAA858[dword_AA848].f_c+dword_AA9A4[dword_AA9C8]+17)<<16, 40960,
        0, fAA858[dword_AA848].f_4, 0, 0, 2);
    ghpistol_setmode(fAA858[dword_AA848].f_10);
}

int dword_AA9CC;
unsigned int dword_AA9D4;

structAA3D0 fAA9DC[7] = {
    1, 3454, 216, 158, 0,
    4, 3452, 216, 162, 1,
    4, 3453, 216, 158, 3,
    12, 3455, 216, 158, 0,
    12, 3456, 216, 132, 5,
    12, 3457, 216, 132, 6,
    12, 3458, 216, 132, 1
};

int dword_AAA68[] = {
    0, -1, -2, -3, -4, -3, -2, -1
};

int dword_AAA88[] = {
    0, -1, -2, -2, -1, -2, -2, -1
};

int dword_AA9D0;

int ghbow_setmode(int a1)
{
    if (a1 < 0 || a1 >= 7)
    {
        Printf("ERROR: ghbow_setmode %i\n", a1);
        return 0;
    }
    if (dword_AA9CC != a1)
    {
        if ((int)totalclock - dword_AA9D4 > fAA9DC[dword_AA9CC].f_0)
        {
            switch (dword_AA9CC)
            {
            case 4:
                A_PlaySound(7, g_player[myconnectindex].ps->i);
                break;
            case 6:
                dword_AA9D0 = 1;
                break;
            case 2:
                dword_AA9D0 = 0;
                break;
            }
            dword_AA9D4 = (int)totalclock;
            dword_AA9CC = a1;
        }
    }
    return dword_AA9CC;
}

void ghbow_fire(short snum)
{
    struct player_struct *p;
    if (!dword_AA9D0)
    {
        if (dword_AA9CC == 1 || dword_AA9CC == 0)
            sub_5A250(0x10);
    }
    if (dword_AA9D0 && dword_AA9CC == 1 && ghbow_setmode(2) == 2)
    {
        A_PlaySound(8, g_player[myconnectindex].ps->i);
        gharrow_spawnarrow(snum);
    }
}

void sub_56AB0(void)
{
}

int dword_AA9D8;

void sub_56AB8(void)
{
    ghbow_setmode(0);
    dword_AA9D8 = 0;
    dword_AA9D0 = 0;
}

int sub_56AD8(void)
{
    return dword_AA9D8;
}

void sub_56AE4(void)
{
    if (dword_AA9CC == 0)
    {
        sub_5A250(0x4000);
        dword_AA9D0 = 0;
        if (dword_AA9CC == 0)
            ghbow_setmode(4);
        dword_AA9D8 = (dword_AA9D8 == 0);
    }
}

void sub_56B3C(void)
{
    if (dword_AA9CC == 0 && dword_AA9D0 != 1)
        ghbow_setmode(4);
}

unsigned int dword_AAAA8;
int dword_AAAAC;

void ghbow_render(short snum)
{
    int vdx;
    if (dword_AA9CC < 0 || dword_AA9CC >= 7)
    {
        Printf("ERROR: ghbow_draw bad index\n");
        return;
    }
    if (snum < 0 || snum >= numplayers)
    {
        Printf("ERROR: ghbow_render bad index\n");
        return;
    }
    DukePlayer_t *p = g_player[snum].ps;
    if (p->dhat613 || p->dhat617)
    {
        vdx = 10;
        if (p->dhat617)
            vdx = 5;

        if ((int)totalclock - dword_AAAA8 > vdx)
        {
            dword_AAAAC++;
            if (dword_AAAAC >= 8)
                dword_AAAAC = 0;
            dword_AAAA8 = (int)totalclock;
        }
    }
    else
    {
        if (dword_AAAAC)
            dword_AAAAC++;
        if (dword_AAAAC >= 8)
            dword_AAAAC = 0;
    }
    if (dword_AAAAC >= 8)
    {
        Printf("ERROR: ghbow_render bobcnt out of bounds\n");
        return;
    }
    sub_54D90();
    rotatesprite_win((fAA9DC[dword_AA9CC].f_8+dword_AAA68[dword_AAAAC])<<16,
        (fAA9DC[dword_AA9CC].f_c+dword_AAA88[dword_AAAAC]+17)<<16, 40960,
        0, fAA9DC[dword_AA9CC].f_4, 0, 0, 2);
    ghbow_setmode(fAA9DC[dword_AA9CC].f_10);
}

int sub_56CF0(int a1)
{
    if (a1 < 0 || a1 >= numsectors)
        return 0;
    return 1;
}

void ghprecip_snowfall(void)
{
    DukePlayer_t *p;
    int i, nexti, j;
    spritetype *s;
    short sect, v18;
    int vdi, vsi, v28;
    p = g_player[screenpeek].ps;

    i = headspritestat[801];
    while (i >= 0)
    {
        nexti = nextspritestat[i];
        s = &sprite[i];
        s->z += 0x300;
        sect = s->sectnum;
        if (sect < 0 || sect >= numsectors)
        {
            Printf("ghprecip_snowfall: bad sectnum\n");
            goto BOLT;
        }
        if (s->z > sector[sect].floorz)
        {
            vdi = p->pos.x + (rrdh_random() & 0x3fff) - 0x2000;
            vsi = p->pos.y + (rrdh_random() & 0x3fff) - 0x2000;
            v28 = -0x19000;
            v18 = -1;
            for (j = 0; j < numsectors; j++)
            {
                if (inside(vdi, vsi, j))
                {
                    if (sub_56CF0(j))
                    {
                        v18 = j;
                        break;
                    }
                }
            }
            if (v18 >= 0 && v18 < numsectors)
            {
                s->x = vdi;
                s->y = vsi;
                s->z = v28;
                changespritestat(i, v18);
                if (v18 != s->sectnum)
                {
                    Printf("changespritesect failed\n");
                }
            }
        }
BOLT:
        i = nexti;
    }
}

void sub_56EA8(void)
{
    ghprecip_snowfall();
}


void sub_56EC0(void)
{
    short i;
    i = headspritestat[802];
    while (i >= 0)
    {
        sprite[i].extra++;
        i = nextspritestat[i];
    }
}

int dword_AAAB8 = 0x180;
int dword_AAAB0;

short ghtrax_getoldestdeertrax(void)
{
    short i, nexti, vcx, vsi;
    vcx = -1;
    vsi = 0;
    i = headspritestat[802];
    while (i >= 0)
    {
        nexti = nextspritestat[i];
        if (vcx < sprite[i].extra)
        {
            vcx = sprite[i].extra;
            vsi = i;
        }
        if (sprite[i].extra > dword_AAAB8)
        {
            Printf("ghtrax_getoldestdeertrax: oldest trax at %i\n", sprite[i].extra.cast());
        }
        i = nexti;
    }
    return vsi;
}

void ghtrax_deertrax(short a1)
{
    spritetype *s, *s2;
    int v24 = 0;
    int v28 = 0;
    int v2c = 0;
    int v30 = 0;
    short v18, i, nexti;
    s = &sprite[a1];
    if (dword_AAAB8 > dword_AAAB0)
    {
        v18 = insertsprite(s->sectnum, 0);
        if (v18 < 0 || v18 >= MAXSPRITES)
        {
            Printf("ghtrax_deertrax: insertsprite failed\n");
            dword_AAAB8 = dword_AAAB0;
            Printf("                 set maxtraxdeer to %i\n", dword_AAAB8);
        }
        else
            dword_AAAB0++;
    }
    else
    {
        v18 = ghtrax_getoldestdeertrax();
        if (v18 < 0 || v18 >= MAXSPRITES)
        {
            Printf("ghtrax_deertrax: invalid oldest trax sprite\n");
            return;
        }
    }
    sub_56EC0();
    s2 = &sprite[v18];
    getzrange_old(s->x, s->y, s->z, s->sectnum, &v24, &v28, &v2c, &v30, 128, CLIPMASK0);
    if (v2c < sector[s->sectnum].floorz)
        v2c = sector[s->sectnum].floorz - 8;
    vec3_t pos = { s->x, s->y, v2c };
    setsprite(a1, &pos);
    changespritestat(a1, 802);
    s2->cstat = 0x20;
    s2->extra = 0;
    s2->ang = s->ang;
    s2->owner = a1;
    s2->pal = 0;
    s2->xoffset = 0;
    s2->yoffset = 0;
    s2->xvel = 0;
    s2->yvel = 0;
    s2->zvel = 0;
    s2->shade = -28;
    s2->xrepeat = 14;
    s2->yrepeat = 18;
    s2->clipdist = 32;
    s2->picnum = 7080 + (ud.level_number != 3);
}

void sub_57140(void)
{
    short i, nexti;
    i = headspritestat[804];
    while (i >= 0)
    {
        nexti = nextspritestat[i];
        sprite[i].extra++;
        i = nexti;
    }
}

int dword_AAAC4 = 0x100;

short ghtrax_getoldestboartrax(void)
{
    short i, nexti, vcx, vsi;
    vcx = -1;
    vsi = 0;
    i = headspritestat[804];
    while (i >= 0)
    {
        nexti = nextspritestat[i];
        if (vcx < sprite[i].extra)
        {
            vcx = sprite[i].extra;
            vsi = i;
        }
        if (sprite[i].extra > dword_AAAC4)
        {
            Printf("ghtrax_getoldestdeertrax: oldest trax at %i\n", sprite[i].extra.cast());
        }
        i = nexti;
    }
    return vsi;
}

int dword_AAABC;

void ghtrax_boartrax(short a1)
{
    spritetype* s, * s2;
    int v24 = 0;
    int v28 = 0;
    int v2c = 0;
    int v30 = 0;
    short v18, i, nexti;
    s = &sprite[a1];
    if (dword_AAAC4 > dword_AAABC)
    {
        v18 = insertsprite(s->sectnum, 0);
        if (v18 < 0 || v18 >= MAXSPRITES)
        {
            Printf("ghtrax_boartrax: insertsprite failed\n");
            dword_AAAC4 = dword_AAABC;
            Printf("                 set maxtraxboar to %d\n", dword_AAAC4);
        }
        else
            dword_AAABC++;
    }
    else
    {
        v18 = ghtrax_getoldestboartrax();
        if (v18 < 0 || v18 >= MAXSPRITES)
        {
            Printf("ghtrax_boartrax: invalid oldest trax sprite\n");
            return;
        }
    }
    sub_57140();
    s2 = &sprite[v18];
    getzrange_old(s->x, s->y, s->z, s->sectnum, &v24, &v28, &v2c, &v30, 128, CLIPMASK0);
    if (v2c < sector[s->sectnum].floorz)
        v2c = sector[s->sectnum].floorz - 8;
    vec3_t pos = { s->x, s->y, v2c };
    setsprite(a1, &pos);
    changespritestat(a1, 804);
    s2->cstat = 0x20;
    s2->extra = 0;
    s2->ang = s->ang;
    s2->owner = a1;
    s2->pal = 0;
    s2->xoffset = 0;
    s2->yoffset = 0;
    s2->xvel = 0;
    s2->yvel = 0;
    s2->zvel = 0;
    s2->shade = -28;
    s2->xrepeat = 14;
    s2->yrepeat = 18;
    s2->clipdist = 32;
    s2->picnum = 7084 + (ud.level_number != 3);
}

int dword_AAAB4, dword_AAAC0;

short word_AAAC8;

void sub_573C0(void)
{
    int vdx = 0;
    int i;
    for (i = 0; i < MAXSPRITES; i++)
        if (sprite[i].picnum)
            vdx++;

    vdx = MAXSPRITES - vdx;
    if (vdx <= 640)
        Printf("not enuff sprites left for deer and boar trax\n");
    dword_AAAB0 = 0;
    dword_AAAB4 = 0;
    dword_AAAB8 = 0x180;
    dword_AAABC = 0;
    dword_AAAC0 = 0;
    dword_AAAC4 = 0x100;
    word_AAAC8 = (rrdh_random() & 2047);
}

short sub_5743C(void)
{
    return word_AAAC8;
}

int ghtrax_isplrupwind(short a1, short a2)
{
    spritetype *s;
    DukePlayer_t *p;
    s = &sprite[a1];
    p = g_player[a2].ps;

    return klabs(word_AAAC8 - (getangle(s->x -p->pos.x, s->y - p->pos.y) & 2047)) < 256;
}

void ghtrax_leavetrax(short a1)
{
    spritetype *s;
    sectortype *sc;
    short sect;
    s = &sprite[a1];
    sect = s->sectnum;
    sc = &sector[sect];

    if (klabs(sector[sect].ceilingheinum - sector[sect].floorheinum) <= 576)
    {
        switch (DYNAMICTILEMAP(sprite[a1].picnum))
        {
        case VIXEN__STATICRR:
            if (sector[sect].hitag == 0)
                ghtrax_deertrax(a1);
            break;
        case PIG__STATICRR:
            if (sector[sect].hitag == 0)
                ghtrax_boartrax(a1);
            break;
        }
    }
}

void ghtrax_deerdroppings(short a1)
{
    spritetype* s, * s2;
    int v24 = 0;
    int v28 = 0;
    int v2c = 0;
    int v30 = 0;
    short v18, i, nexti;
    s = &sprite[a1];
    if (dword_AAAB4 >= 24)
        return;
    v18 = insertsprite(s->sectnum, 0);
    if (v18 < 0 || v18 >= MAXSPRITES)
    {
        Printf("ghtrax_deerdroppings: insertsprite failed\n");
        return;
    }
    s2 = &sprite[v18];
    getzrange_old(s->x, s->y, s->z, s->sectnum, &v24, &v28, &v2c, &v30, 128, CLIPMASK0);
    if (v2c < sector[s->sectnum].floorz)
        v2c = sector[s->sectnum].floorz - 8;
    vec3_t pos = { s->x, s->y, v2c };
    setsprite(a1, &pos);
    changespritestat(a1, 803);
    s2->cstat = 0;
    s2->ang = s->ang;
    s2->owner = a1;
    s2->pal = 0;
    s2->xoffset = 0;
    s2->yoffset = 0;
    s2->xvel = 0;
    s2->yvel = 0;
    s2->zvel = 0;
    s2->shade = 8;
    s2->xrepeat = 8;
    s2->yrepeat = 5;
    s2->clipdist = 32;
    s2->extra = 0;
    s2->picnum = 981 + (ud.level_number != 3);
    dword_AAAB4++;
}

void ghtrax_boardroppings(short a1)
{
    spritetype* s, * s2;
    int v24 = 0;
    int v28 = 0;
    int v2c = 0;
    int v30 = 0;
    short v18, i, nexti;
    s = &sprite[a1];
    if (dword_AAAC0 >= 24)
        return;
    v18 = insertsprite(s->sectnum, 0);
    if (v18 < 0 || v18 >= MAXSPRITES)
    {
        Printf("ghtrax_boardroppings: insertsprite failed\n");
        return;
    }
    s2 = &sprite[v18];
    getzrange_old(s->x, s->y, s->z, s->sectnum, &v24, &v28, &v2c, &v30, 128, CLIPMASK0);
    if (v2c < sector[s->sectnum].floorz)
        v2c = sector[s->sectnum].floorz - 8;
    vec3_t pos = { s->x, s->y, v2c };
    setsprite(a1, &pos);
    changespritestat(a1, 805);
    s2->cstat = 0;
    s2->ang = s->ang;
    s2->owner = a1;
    s2->picnum = 983;
    s2->pal = 0;
    s2->xoffset = 0;
    s2->yoffset = 0;
    s2->xvel = 0;
    s2->yvel = 0;
    s2->zvel = 0;
    s2->shade = 8;
    s2->xrepeat = 8;
    s2->yrepeat = 5;
    s2->clipdist = 32;
    s2->extra = 0;
    dword_AAAC0++;
}

void ghtrax_leavedroppings(short a1)
{
    spritetype *s;
    sectortype *sc;
    short sect;
    s = &sprite[a1];
    sect = s->sectnum;
    sc = &sector[sect];

    if (klabs(sector[sect].ceilingheinum - sector[sect].floorheinum) <= 576)
    {
        switch (DYNAMICTILEMAP(sprite[a1].picnum))
        {
        case VIXEN__STATICRR:
            if (sector[sect].hitag == 0)
                ghtrax_deerdroppings(a1);
            break;
        case PIG__STATICRR:
            if (sector[sect].hitag == 0)
                ghtrax_boardroppings(a1);
            break;
        }
    }
}

int dword_AAAD0, dword_AAAD4, dword_AAAD8, dword_AAACC;
unsigned int dword_AAADC, dword_AAAE0, dword_AAAE4, dword_AAAE8;

void sub_579A0(void)
{
    dword_AAAD0++;
    dword_AAACC++;
    if (dword_AAAD0 == 4)
    {
        if (dword_AAADC > 0)
            dword_AAADC--;
        if (dword_AAAE0 > 0)
            dword_AAAE0--;
        if (dword_AAAE4 > 0)
            dword_AAAE4--;
        if (dword_AAAE8 > 0)
            dword_AAAE8--;
        dword_AAAD0 = 0;
        dword_AAAD4++;
        if (dword_AAAD4 == 10)
        {
            dword_AAAD8++;
            dword_AAAD4 = 0;
        }
    }
}

int sub_57A40(int a1)
{
    if (dword_AAADC)
        return 0;
    dword_AAADC = a1;
    return 1;
}

int sub_57A60(int a2)
{
    if (dword_AAAE0)
        return 0;
    dword_AAAE0 = a2;
    return 1;
}

int sub_57A80(int a2)
{
    if (dword_AAAE4)
        return 0;
    dword_AAAE4 = a2;
    return 1;
}

int sub_57AA0(int a2)
{
    if (dword_AAAE8)
        return 0;
    dword_AAAE8 = a2;
    return 1;
}

int dword_AAAEC;

void sub_57AC0(void)
{
    int i;
    show2dsector.SetAll(1);
    for (i = 0; i < MAXWALLS; i++)
        show2dwall[i>>3] |= 1<<(i&7);
    dword_AAAEC ^= 1;
}

void sub_57B24(void)
{
    dword_AAAEC = 0;
}

void sub_57B38(long cposx, long cposy, long czoom, short cang)
{
        long i, j, k, l, x1, y1, x2, y2, x3, y3, x4, y4, ox, oy, xoff, yoff;
        long dax, day, cosang, sinang, xspan, yspan, sprx, spry;
        long xrepeat, yrepeat, z1, z2, startwall, endwall, tilenum, daang;
        long xvect, yvect, xvect2, yvect2, xc, yc, xc2, yc2;
        short p;
        char col;
        walltype *wal, *wal2;
        spritetype *spr;

        if (!dword_AAAEC)
            return;

        xvect = sintable[(-cang)&2047] * czoom;
        yvect = sintable[(1536-cang)&2047] * czoom;
        xvect2 = mulscale16(xvect,yxaspect);
        yvect2 = mulscale16(yvect,yxaspect);
        xc = windowxy2.x - windowxy1.x;
        yc = windowxy2.y - windowxy1.y;
        xc2 = 0;
        yc2 = 0;
        
                //Draw white lines
        for(i=0;i<numsectors;i++)
        {
                if (!gFullMap && !show2dsector[i]) continue;

                startwall = sector[i].wallptr;
                endwall = sector[i].wallptr + sector[i].wallnum;

                k = -1;
                for(j=startwall,wal=&wall[startwall];j<endwall;j++,wal++)
                {
                        if (wal->nextwall >= 0) continue;

                        if ((show2dwall[j>>3]&(1<<(j&7))) == 0) continue;

                        if (tilesiz[wal->picnum].x == 0) continue;
                        if (tilesiz[wal->picnum].y == 0) continue;

                        if (j == k)
                                { x1 = x2; y1 = y2; }
                        else
                        {
                                ox = wal->x; oy = wal->y;
                                x1 = dmulscale16(ox,xvect,-oy,yvect)+(xc<<11);
                                y1 = dmulscale16(oy,xvect2,ox,yvect2)+(yc<<11);
                        }

                        k = wal->point2; wal2 = &wall[k];
                        ox = wal2->x; oy = wal2->y;
                        x2 = dmulscale16(ox,xvect,-oy,yvect)+(xc<<11);
                        y2 = dmulscale16(oy,xvect2,ox,yvect2)+(yc<<11);

                        renderDrawLine(x1,y1,x2,y2,74);
                }
        }

         for(k=0;k<MAXSPRITES;k++)
         {
             i = 0;
             switch (DYNAMICTILEMAP(sprite[k].picnum))
             {
             case VIXEN__STATICRR:
                 i = 5665;
                 break;
             case PIG__STATICRR:
                 i = 4966;
                 break;
             case DOGRUN__STATICRR:
                 i = 4311;
                 break;
             case CHEER__STATICRR:
                 i = 6777;
                 break;
             }
             if (i == 0) continue;

              ox = sprite[k].x-xc2; oy = sprite[k].y-yc2;
              x1 = dmulscale16(ox,xvect,-oy,yvect);
              y1 = dmulscale16(oy,xvect2,ox,yvect2);

              rotatesprite_win((x1<<4)+(xc<<15), (y1<<4)+(yc<<15), 8192, 0, i, 0, 0, 0);
         }

          ox = cposx-xc2; oy = cposy-yc2;
          x1 = dmulscale16(ox,xvect,-oy,yvect);
          y1 = dmulscale16(oy,xvect2,ox,yvect2);

          rotatesprite_win((x1<<4)+(xc<<15), (y1<<4)+(yc<<15), 65536, (fix16_to_int(g_player[screenpeek].ps->q16ang)+512)&2047, 7060, 0, 0, 0);
}

int sub_57FB0(void)
{
    return 64 + (rrdh_random() % 224);
}

unsigned int dword_AAAF0;
unsigned int dword_AAAF4, dword_AAAF8;
int dword_AAAFC, dword_AAB00, dword_AAB04;

void ghtarget_runningclock(void)
{
    if (dword_AAAFC != 2)
    {
        if (dword_AAAFC == 1)
        {
            if ((int)totalclock - dword_AAB04 >= 240)
            {
                sub_53E18();
                dword_AAAFC = 2;
            }
        }
        else
        {
            if ((int)totalclock - dword_AAAF8 >= 120)
            {
                if (dword_AAAF4 > 0)
                    dword_AAAF4--;
                else if (dword_AAAF0 > 0)
                {
                    dword_AAAF4 = 50;
                    dword_AAAF0--;
                }
                else
                {
                    dword_AAAFC = 1;
                    dword_AAB04 = (int)totalclock;
                    P_DoQuote(145, g_player[myconnectindex].ps);
                }
                dword_AAAF8 = (int)totalclock;
            }
        }
    }
}

void sub_580C8(void)
{
    int vsi = dword_AAAF0 % 10;
    int vd = dword_AAAF0 / 10;
    rotatesprite(292<<16, 184<<16, 17408, 0, DIGITALNUM+vd, 0, 0, 128+10, 0, 0, xdim-1, ydim-1);
    rotatesprite(296<<16, 184<<16, 17408, 0, DIGITALNUM+vsi, 0, 0, 128+10, 0, 0, xdim-1, ydim-1);
    vsi = dword_AAAF4 % 10;
    vd = dword_AAAF4 / 10;
    rotatesprite(302<<16, 184<<16, 17408, 0, DIGITALNUM+vd, 0, 0, 128+10, 0, 0, xdim-1, ydim-1);
    rotatesprite(306<<16, 184<<16, 17408, 0, DIGITALNUM+vsi, 0, 0, 128+10, 0, 0, xdim-1, ydim-1);
}

void ghtarget_move(void)
{
    int v2c = 0;
    int v28 = 0;
    int v24 = 0;
    short v18 = 0;
    int i, nexti;
    short mv;
    spritetype *s;
    i = headspritestat[808];
    while (i >= 0)
    {
        nexti = nextspritestat[i];
        s = &sprite[i];
        if (dword_AAAFC)
            mv = 0;
        else
        {
            v2c = s->x;
            v28 = s->y;
            v24 = s->z;
            v18 = s->sectnum;
            vec3_t vect = { (sintable[1536]*s->xvel)>>14, 0, 0 };
            mv = A_MoveSprite(i, &vect, CLIPMASK1);
        }
        if (mv || v18 != s->sectnum)
        {
            s->x = v2c;
            s->y = v28;
            s->z = v24;
            changespritesect(i, v18);
            s->yvel = -s->yvel;
            s->xvel = sub_57FB0();
            if (s->yvel == -1)
                s->xvel = -s->xvel;
            s->cstat ^= 4;
            ghtarget_setanimal(i);
            if (s->extra)
            {
                s->cstat |= 256;
                s->cstat &= ~32768;
                s->extra = 0;
                s->xvel = sub_57FB0();
                if (s->yvel == -1)
                    s->xvel = -s->xvel;
            }
        }
        i = nexti;
    }
    if (ud.level_number > 3)
        ghtarget_runningclock();
}

void sub_58364(int a1)
{
    dword_AAAF0 = a1;
    dword_AAAF8 = 0;
    dword_AAAF4 = 0;
    dword_AAAFC = 0;
}

void sub_58388(void)
{
    int i, vdx;
    sub_58364(0);
    dword_AAB00 = 0;
    if (ud.level_number < 4)
        return;
    for (i = 0; i < MAXSPRITES; i++)
    {
        vdx = 0;
        switch (sprite[i].picnum)
        {
        case 7110:
            sprite[i].xrepeat = 18;
            sprite[i].yrepeat = 24;
            vdx = 1;
            break;
        case 7111:
            sprite[i].xrepeat = 13;
            sprite[i].yrepeat = 14;
            vdx = 1;
            break;
        case 7112:
            sprite[i].xrepeat = 22;
            sprite[i].yrepeat = 16;
            vdx = 1;
            break;
        case 1076:
            sprite[i].cstat = 257;
            break;
        }
        if (vdx)
        {
            changespritestat(i, 808);
            sprite[i].cstat = 273;
            sprite[i].xvel = sub_57FB0();
            sprite[i].yvel = 1;
            sprite[i].extra = 0;
        }
    }
    sub_58364(3);
}

void ghtarget_setanimal(short a1)
{
    int vdx;
    vdx = rrdh_random() % 5;
    switch (vdx)
    {
    case 0:
        sprite[a1].picnum = 7110;
        sprite[a1].xrepeat = 18;
        sprite[a1].yrepeat = 24;
        break;
    case 1:
        sprite[a1].picnum = 7111;
        sprite[a1].xrepeat = 13;
        sprite[a1].yrepeat = 14;
        break;
    case 2:
        sprite[a1].picnum = 7112;
        sprite[a1].xrepeat = 22;
        sprite[a1].yrepeat = 16;
        break;
    }
}

void ghtarget_hit(short a1, int a2)
{
    unsigned short vc;
    if (dword_AAAFC)
        return;
    if (sprite[a1].picnum == 1076)
    {
        dword_AAB00++;
        sub_58A30(1);
        sub_53848(dword_AAB00);
    }
    else if (sprite[a1].statnum == 808)
    {
        vc = sprite[a1].picnum;
        switch (sprite[a1].picnum)
        {
        case 7110:
        case 7111:
            vc = 1;
            break;
        case 7112:
            vc = 4;
            break;
        }
        ghstatbr_registerkillinfo(sprite[a1].picnum, 0, 0);
        vc += klabs(sprite[a1].xvel) / 32;
        switch (sector[sprite[a1].sectnum].hitag)
        {
        case 2004:
            vc++;
            break;
        case 2005:
            vc += 2;
            break;
        case 2006:
            vc += 4;
            break;
        case 2007:
            vc += 8;
            break;
        case 2008:
            vc += 16;
            break;
        default:
            Printf("WARNING: ghtarget_hit: spr not in track\n");
            break;
        }
        switch (g_player[myconnectindex].ps->dhat61f)
        {
        case 0:
            vc++;
            break;
        case 1:
            vc += 2;
            break;
        case 3:
            vc += 2;
            break;
        case 4:
            vc += 3;
            break;
        }
        sprite[a1].cstat |= 32768;
        sprite[a1].cstat &= ~256;
        A_PlaySound(87, g_player[myconnectindex].ps->i);
        ghtarget_setanimal(a1);
        sprite[a1].extra = 1;
        if (vc > 18)
            sub_5A250(0x200);
        dword_AAB00 += vc;
        sub_58A30(vc);
        sub_53848(dword_AAB00);
    }
}

void gharrow_move(void)
{
    int i, nexti;
    short mv;
    int v24, v28, v20, vdx;
    short v18;
    spritetype *s;
    i = headspritestat[809];
    while (i >= 0)
    {
        nexti = nextspritestat[i];
        s = &sprite[i];
        if (s->sectnum < 0 || s->sectnum >= numsectors)
        {
            deletesprite(i);
        }
        else
        {
            A_GetZLimits(i);
            v24 = s->x;
            v28 = s->y;
            v20 = s->y;
            v18 = s->sectnum;
            vec3_t vec = { s->xvel, s->yvel, s->zvel };
            mv = A_MoveSprite(i, &vec, CLIPMASK1);
            if (mv)
            {
                s->x = v24;
                s->y = v28;
                s->z = v20;
                changespritesect(i, v18);
                if ((mv & 49152) == 49152)
                {
                    mv &= MAXSPRITES - 1;
                    A_PlaySound(59, mv);
                    vdx = sub_56AD8();
                    ghtrophy_addkill(mv);
                    ghtarget_hit(mv, vdx);
                }
                else
                {
                    if ((mv & 49152) == 32768 && ud.level_number > 3)
                        A_PlaySound(59, i);
                    sub_5A250(4);
                }
                deletesprite(i);
            }
        }
        i = nexti;
    }
}

void gharrow_spawnarrow(short snum)
{
    short s;
    spritetype *spr;
    DukePlayer_t *p;
    p = g_player[snum].ps;
    s = insertsprite(p->cursectnum, 809);
    if (s < 0 || s >= MAXSPRITES)
    {
        Printf("gharrow_spawnarrow: insertsprite failed\n");
        return;
    }
    spr = &sprite[s];
    if (!spr)
    {
        Printf("gharrow_spawnarrow: null sprptr\n");
        return;
    }
    spr->x = p->pos.x;
    spr->y = p->pos.y;
    spr->z = p->pos.z;
    spr->ang = (fix16_to_int(p->q16ang) + 3) & 2047;
    spr->xvel = sintable[(spr->ang + 512) & 2047];
    spr->yvel = sintable[(spr->ang) & 2047];
    spr->picnum = 3450;
    spr->cstat = 1;
    spr->owner = 0;
    spr->xrepeat = 14;
    spr->yrepeat = 14;
    spr->pal = 0;
    spr->xoffset = 0;
    spr->yoffset = 0;
    spr->lotag = 0;
    spr->hitag = 0;
    spr->extra = 0;
    spr->shade = 0;
    spr->zvel = (100 - fix16_to_int(p->q16horiz) + 1) << 9;
    spr->cstat |= 0x8001;
}

int dword_AAB08 = 1;
unsigned int dword_AAB0C;

void sub_58A30(int a1)
{
    dword_AAB0C += a1;
    if (dword_AAB0C > 99999)
        dword_AAB0C = 0;
    dword_AAB08 = 128;
}

void sub_58A5C(unsigned int a1)
{
    int t1 = a1 % 10;
    int t2 = (a1 % 100) / 10;
    int t3 = (a1 % 1000) / 100;
    int t4 = (a1 % 10000) / 1000;
    int t5 = (a1 % 100000) / 10000;
    rotatesprite(243<<16, 185<<16, 22528, 0, DIGITALNUM+t1, 0, 0, 128+64+10, 0, 0, xdim-1, ydim-1);
    rotatesprite(235<<16, 185<<16, 22528, 0, DIGITALNUM+t2, 0, 0, 128+64+10, 0, 0, xdim-1, ydim-1);
    rotatesprite(227<<16, 185<<16, 22528, 0, DIGITALNUM+t3, 0, 0, 128+64+10, 0, 0, xdim-1, ydim-1);
    rotatesprite(219<<16, 185<<16, 22528, 0, DIGITALNUM+t4, 0, 0, 128+64+10, 0, 0, xdim-1, ydim-1);
    rotatesprite(211<<16, 185<<16, 22528, 0, DIGITALNUM+t5, 0, 0, 128+64+10, 0, 0, xdim-1, ydim-1);
}

short word_AAB10;
int dword_AAB14, dword_AAB18;

void ghstatbr_registerkillinfo(short a1, int a2, int a3)
{
    if (a1 < 0 || a1 >= MAXTILES)
    {
        Printf("ERROR: ghstatbr_registerkillinfo bad pic range\n");
        return;
    }
    if (a2 < 0)
    {
        Printf("ERROR: ghstatbr_registerkillinfo neg points\n");
        return;
    }
    switch (DYNAMICTILEMAP(a1))
    {
    case VIXEN__STATICRR:
        word_AAB10 = 1720;
        break;
    case RRTILE7110__STATICRR:
        word_AAB10 = 7114;
        break;
    case PIG__STATICRR:
        word_AAB10 = 1719;
        break;
    case RRTILE7111__STATICRR:
        word_AAB10 = 7115;
        break;
    case DOGRUN__STATICRR:
        word_AAB10 = 1721;
        break;
    case RRTILE7113__STATICRR:
        word_AAB10 = 7116;
        break;
    case CHEER__STATICRR:
        word_AAB10 = 1722;
        break;
    case RRTILE7112__STATICRR:
        word_AAB10 = 7117;
        break;
    default:
        word_AAB10 = 0;
        dword_AAB14 = 0;
        return;
    }
    dword_AAB14 = a2;
    dword_AAB18 = a3;
    dword_AAB08 |= 8;
}

void sub_58D14(void)
{
    if (word_AAB10 > 0 && dword_AAB14 >= 0 && dword_AAB18 >= 0)
    {
        rotatesprite(39<<16, 185<<16, 32768, 0, word_AAB10, 0, 0, 128+64+10, 0, 0, xdim-1, ydim-1);
        if (ud.level_number < 4)
        {
            unsigned int t1 = dword_AAB14 % 10;
            unsigned int t2 = (dword_AAB14 % 100) / 10;
            unsigned int t3 = (dword_AAB14 % 1000) / 100;
            rotatesprite(64<<16, 180<<16, 18432, 0, DIGITALNUM+t1, 0, 0, 128+64+10, 0, 0, xdim-1, ydim-1);
            if (t3 || t2 > 0)
            {
                rotatesprite(58<<16, 180<<16, 18432, 0, DIGITALNUM+t2, 0, 0, 128+64+10, 0, 0, xdim-1, ydim-1);
            }
            if (t3 > 0)
            {
                rotatesprite(52<<16, 180<<16, 18432, 0, DIGITALNUM+t3, 0, 0, 128+64+10, 0, 0, xdim-1, ydim-1);
            }
            t1 = dword_AAB18 % 10;
            t2 = (dword_AAB18 % 100) / 10;
            rotatesprite(64<<16, 190<<16, 18432, 0, DIGITALNUM+t1, 0, 0, 128+64+10, 0, 0, xdim-1, ydim-1);
            if (t2 > 0)
            {
                rotatesprite(58<<16, 190<<16, 18432, 0, DIGITALNUM+t2, 0, 0, 128+64+10, 0, 0, xdim-1, ydim-1);
            }
        }
    }
}


void sub_58F40(int a1)
{
    int v18 = sub_59B44();
    int v1c, v20, i;
    switch (a1)
    {
    case 0:
        v20 = 1723;
        v1c = 24576;
        break;
    case 1:
    case 2:
    case 3:
        v20 = 1724;
        v1c = 24576;
        break;
    default:
        return;
    }
    for (i = 0; i < v18; i++)
    {
        rotatesprite((216+13*i)<<16, 184<<16, v1c, 0, v20, 0, 0, 128+10, 0, 0, xdim-1, ydim-1);
    }
}

void ghstatbr_drawammotype(void)
{
    int vbx = 0;
    switch (g_player[myconnectindex].ps->dhat61f)
    {
    case 0:
        vbx = sub_56718();
        break;
    case 1:
    case 2:
        vbx = sub_55FC0();
        break;
    case 3:
        vbx = sub_55928();
        break;
    case 4:
        vbx = sub_56AD8();
        break;
    }
    rotatesprite(97<<16, 187<<16, 32768, 0, 1711+vbx, 0, 0, 128+10, 0, 0, xdim-1, ydim-1);
}

int dword_AAB1C;

void ghstatbr_render(void)
{
    int r;
    short v18;

    if (klabs(fix16_to_int(g_player[myconnectindex].ps->q16ang) - fix16_to_int(g_player[myconnectindex].ps->oq16ang)) > 16)
        dword_AAB08 = 2;

    v18 = sub_5743C();
    r = rrdh_random() & 1023;
    if (r < 64)
    {
        v18 += r - 32;
        dword_AAB08 = 4;
        v18 &= 2047;
    }
    if (dword_AAB08)
    {
        //sub_51028(3, dword_AAB1C++);
        if (ud.level_number < 4)
        {
            rotatesprite(0<<16, 166<<16, 32768, 0, 1647, 4, 0, 128+64+16+10, 0, 0, xdim-1, ydim-1);
        }
        else
        {
            rotatesprite(0<<16, 166<<16, 32768, 0, 1710, 4, 0, 128+64+16+10, 0, 0, xdim-1, ydim-1);
        }
        sub_58D14();
        if (ud.level_number > 0 && ud.level_number < 4)
        {
            rotatesprite(216<<16, 166<<16, 32768, 0, 1725, 4, 0, 128+64+16+10, 0, 0, xdim-1, ydim-1);
        }
        ghstatbr_drawammotype();
        rotatesprite(155<<16, 185<<16, 32768, v18, 1637, 0, 0, 128+10, 0, 0, xdim-1, ydim-1);
        if (ud.level_number < 4)
            rotatesprite(301<<16, 183<<16, 32768, fix16_to_int(g_player[screenpeek].ps->q16ang), 1638, 0, 0, 128+10, 0, 0, xdim-1, ydim-1);
        if (ud.level_number > 3)
            sub_580C8();
        sub_58F40(ud.level_number);
        if (ud.level_number > 3)
            sub_58A5C(dword_AAB0C);
        dword_AAB08 = 0;
    }
}

//void sub_592F0(void)
//{
//    dword_AAB08 = 1;
//}
//
//void sub_59304(int a1)
//{
//    dword_AAB08 |= a1;
//}

void sub_59314(void)
{
    dword_AAB0C = 0;
    dword_AAB14 = 0;
    dword_AAB08 = 1;
    word_AAB10 = 0;
}

int ghdeploy_isdownwind(short a1, short a2)
{
    short ang;
    ang = getangle(sprite[a1].x - sprite[a2].x, sprite[a1].y - sprite[a2].y) & 2047;
    return klabs(sub_5743C() - ang) < 512;
}

void ghdeploy_bias(short a1)
{
    spritetype *s;
    int i, nexti, vcx, d;
    short v1c;
    s = &sprite[a1];
    vcx = 76800;
    v1c = -1;
    i = headspritestat[811];
    while (i >= 0)
    {
        nexti = nextspritestat[i];
        if (sprite[i].extra > 0)
        {
            if (sprite[i].picnum == 7073 && ghdeploy_isdownwind(a1, i))
            {
                d = klabs(sprite[a1].x-sprite[i].x) + klabs(sprite[a1].y-sprite[i].y);
                if (d < vcx)
                {
                    vcx = d;
                    v1c = i;
                }
            }
            else if (sprite[i].picnum == 7072)
            {
                if ((rrdh_random() & 255) > 32)
                {
                    d = klabs(sprite[a1].x-sprite[i].x) + klabs(sprite[a1].y-sprite[i].y);
                    if (d < vcx)
                    {
                        vcx = d;
                        v1c = i;
                    }
                }
            }
            else
            {
                d = klabs(sprite[a1].x-sprite[i].x) + klabs(sprite[a1].y-sprite[i].y);
                if (d < vcx)
                {
                    vcx = d;
                    v1c = i;
                }
            }
        }
        i = nexti;
    }
    if (vcx < 76800 && v1c != -1)
    {
        sprite[a1].ang = getangle(sprite[v1c].x-sprite[a1].x,sprite[v1c].y-sprite[a1].y) & 2047;
    }
    else
    {
        sprite[a1].ang = rrdh_random() & 2047;
    }
}

int dword_AAB24[] = {
    0, 16, 32, 48, 64, 80, 96, 112, 128, 144, 160, 176,
    192, 208, 224, 240, 256, 240, 224, 208, 192, 176, 160,
    144, 128, 112, 96, 80, 64, 48, 32, 16
};

void ghdeploy_move(void)
{
    int v1c = 0;
    int v20 = 0;
    int v24 = 0;
    int v28 = 0;
    int i, nexti;
    spritetype *s;

    i = headspritestat[811];
    while (i >= 0)
    {
        nexti = nextspritestat[i];
        s = &sprite[i];
        if (s->sectnum < 0 || s->sectnum >= numsectors)
        {
            Printf("ghdeploy_move DEPLOYED bad sect %i\n", s->sectnum.cast());
            deletesprite(i);
        }
        else if (sector[s->sectnum].hitag == 2003)
        {
            if (s->zvel < 0 || s->zvel >= 32)
                s->zvel = 0;
            else
            {
                s->z = sector[s->sectnum].floorz + 256 + dword_AAB24[s->zvel];
                s->zvel++;
            }
        }
        if (s->picnum == 7073 && s->extra > 0)
        {
            s->extra--;
        }
        i = nexti;
    }
    i = headspritestat[810];
    while (i >= 0)
    {
        nexti = nextspritestat[i];
        s = &sprite[i];
        if (s->sectnum < 0 || s->sectnum >= numsectors)
        {
            Printf("ghdeploy_move TOSS bad sect %i\n", s->sectnum.cast());
            deletesprite(i);
        }
        vec3_t vec = { s->xvel, s->yvel, 0 };
        A_MoveSprite(i, &vec, CLIPMASK1);
        getzrange_old(s->x, s->y, s->z, s->sectnum, &v1c, &v20, &v24, &v28, 32, CLIPMASK0);
        if (v24 - 2048 < sprite[i].z)
        {
            changespritestat(i, 811);
            s->extra = 9600;
            s->z = v24;
            s->zvel = 0;
            if (s->picnum == 7072)
            {
                s->cstat = 1;
                if (sector[s->sectnum].hitag == 2003)
                    A_PlaySound(90, g_player[myconnectindex].ps->i);
                if (sector[s->sectnum].hitag == 2003)
                    A_PlaySound(89, g_player[myconnectindex].ps->i);
            }
        }
        else
        {
            sprite[i].z += sprite[i].zvel;
            sprite[i].zvel += 0x200;
        }
        i = nexti;
    }
}

unsigned int dword_AAB20;
int dword_AABA4 = 4;

void ghdeploy_drop(int a1, int a2)
{
    DukePlayer_t *p;
    short va;
    spritetype *s;
    if ((int)totalclock - dword_AAB20 < 120)
        return;
    if (!dword_AABA4)
    {
        P_DoQuote(149, g_player[a1].ps);
        return;
    }
    if (a2 > 3)
        return;
    p = g_player[a1].ps;
    if (p->cursectnum < 0 || p->cursectnum >= numsectors)
    {
        Printf("ERROR: ghdeploy_drop bad plrsectr %i\n", p->cursectnum);
        return;
    }
    va = insertsprite(p->cursectnum, 810);
    if (va < 0 || va >= MAXSPRITES)
    {
        Printf("ghdeploy_drop: insertsprite failed\n");
        return;
    }
    s = &sprite[va];
    if (!s)
    {
        Printf("ghdeploy_drop: null sprptr\n");
        return;
    }
    s->x = p->pos.x + (sintable[(fix16_to_int(p->q16ang) + 512) & 2047] >> 7);
    s->y = p->pos.y + (sintable[(fix16_to_int(p->q16ang)) & 2047] >> 7);
    s->z = p->pos.z + 0x1000;
    s->ang = fix16_to_int(fix16_to_int(p->q16ang)) & 2047;
    s->xvel = (sintable[(fix16_to_int(p->q16ang) + 512) & 2047] * 5) >> 8;
    s->yvel = (sintable[(fix16_to_int(p->q16ang)) & 2047] * 5) >> 8;
    s->zvel = (80 - fix16_to_int(p->q16horiz)) << 6;
    if (a2 == 0)
    {
        s->picnum = 7072;
        s->cstat = 0;
        P_DoQuote(146, p);
    }
    else
    {
        s->cstat = 0;
        s->cstat |= 32768;
        s->picnum = 7073;
        P_DoQuote(148, p);
        A_PlaySound(88, p->i);
    }
    s->owner = 0;
    s->clipdist = 4;
    s->xrepeat = 10;
    s->yrepeat = 10;
    s->pal = 0;
    s->xoffset = 0;
    s->yoffset = 0;
    s->lotag = 0;
    s->hitag = 0;
    s->extra = 0;
    s->shade = 0;
    dword_AABA4--;
    //sub_592F0();
    dword_AAB20 = (int)totalclock;
}

int sub_59B44(void)
{
    return dword_AABA4;
}

void sub_59B50(void)
{
    dword_AABA4 = 4;
    dword_AAB20 = 0;
}

void ghdeploy_plrtouchedsprite(short a1, short a2)
{
    DukePlayer_t *p;
    spritetype *s;
    p = g_player[a2].ps;
    s = &sprite[a1];

    if (sprite[a1].statnum == 811 && sprite[a1].picnum != 7073)
    {
        deletesprite(a1);
        dword_AABA4++;
        P_DoQuote(147, p);
    }
}

short word_2BE990[68];
short word_2BEA18;
char byte_2BE350[256];

void sub_59C20(void)
{
    int i;
    word_2BEA18 = 0;
    tileDelete(7059);
    for (i = 0; i < numsectors; i++)
    {
        sector[i].extra = 256;
        if (sector[i].floorpicnum == 7059)
            word_2BE990[word_2BEA18++] = i;
    }
    for (i = 0; i < 256; i++)
        byte_2BE350[i] = i;
    paletteMakeLookupTable(2, byte_2BE350, 10*4, 10*4, 24*4, 0);
}

int dword_2BEA20, dword_2BEA24;
int dword_AABA8, dword_AABAC, dword_AABB0;

void sub_59F80(int a1)
{
    dword_2BEA20 = 0;
    dword_AABA8 = (int)totalclock;
    dword_AABAC = (int)totalclock;
    dword_2BEA24 = a1;
    dword_AABB0 = 0;
}

void ghmumble_randomsayit(int a1, int a2)
{
    if (a1 < 0 || a1 >= MAXSOUNDS)
    {
        Printf("ghmumble_randomsayit bad sndnum\n");
        return;
    }
    if (g_player[myconnectindex].ps->gm == MODE_GAME)
    {
        if ((rrdh_random() & 255) <= a2)
        {
            S_PlaySound(a1);
            dword_AABB0 = (int)totalclock;
        }
    }
}

void sub_5A02C(void)
{
    int t;
    if (dword_2BEA24 > 3)
    {
        if (dword_2BEA20 == 512)
        {
            ghmumble_randomsayit(105+(rrdh_random()%2), 164);
        }
        dword_2BEA20 = 0;
        return;
    }
    if ((int)totalclock - dword_AABB0 < 480)
    {
        dword_2BEA20 = 0;
        return;
    }
    if (dword_2BEA20 == 0)
    {
        if ((int)totalclock - dword_AABA8 > 2400)
        {
            ghmumble_randomsayit(91 + (rrdh_random() % 4), 200);
            dword_AABA8 = (int)totalclock;
            dword_AABAC = (int)totalclock;
        }
    }
    else if (dword_2BEA20 & 8)
    {
        ghmumble_randomsayit(100 + (rrdh_random() % 2), 200);
        dword_AABA8 = (int)totalclock;
        dword_AABAC = (int)totalclock;
    }
    else if (dword_2BEA20 & 4)
    {
        if (dword_2BEA20 & 32)
            ghmumble_randomsayit(98 + (rrdh_random() % 2), 216);
        dword_AABA8 = (int)totalclock;
        dword_AABAC = (int)totalclock;
    }
    else if (dword_2BEA20 & 16)
    {
        if (dword_2BEA20 & 32)
            ghmumble_randomsayit(102, 216);
        dword_AABA8 = (int)totalclock;
        dword_AABAC = (int)totalclock;
    }
    else if (dword_2BEA20 & 2048)
    {
        ghmumble_randomsayit(108, 250);
        dword_AABA8 = (int)totalclock;
    }
    else if (dword_2BEA20 & 4096)
    {
        ghmumble_randomsayit(109, 80);
        dword_AABA8 = (int)totalclock;
    }
    else if (dword_2BEA20 & 16384)
    {
        ghmumble_randomsayit(107, 255);
        dword_AABA8 = (int)totalclock;
    }
    else if (dword_2BEA20 & 256)
    {
        if ((dword_2BEA20 & 64) == 0)
        {
            if ((int)totalclock - dword_AABAC > 7200)
            {
                Printf("nosightings mumble\n");
                t = rrdh_random() % 3;
                if (t == 2 && dword_2BEA24 != 3)
                    t = 1;
                ghmumble_randomsayit(95+t, 200);
                dword_AABA8 = (int)totalclock;
                dword_AABAC = (int)totalclock;
            }
        }
    }
    dword_2BEA20 = 0;
}

void sub_5A250(int a1)
{
    if (a1 >= 0x10000)
        return;
    switch (a1)
    {
    case 1:
        dword_2BEA20 = 0;
        return;
    case 2:
        dword_AABA8 = (int)totalclock;
        dword_AABAC = (int)totalclock;
        dword_2BEA20 = 0;
        return;
    case 0x100:
        dword_AABA8 = (int)totalclock;
        break;
    case 0x10:
        break;
    case 0x20:
    case 0x40:
        dword_AABAC = (int)totalclock;
        break;
    }
    dword_2BEA20 |= a1;
}
END_RR_NS
