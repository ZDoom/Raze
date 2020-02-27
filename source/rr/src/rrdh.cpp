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
#include "files.h"
#include "i_specialpaths.h"

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

void ghsound_ambientlooppoll(void)
{
    // TODO
}

int dword_AA2F0, dword_AA2F4, dword_AA2F8, dword_AA2FC;

void sub_53160(int a1)
{
    dword_AA2FC = a1;
    dword_AA2F8 = 1;
}

void sub_53304(void)
{
    // TODO
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
        initprintf("ghtrophy_savebestscores: error writing scores\n");
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
        initprintf("ghtrophy_loadbestscores err read scores\n");
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
        initprintf("ghtrophy_rscopysrcdest null ptr\n");
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
    if (totalclock - dword_AA38C > 30)
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

void sub_54DE0(void)
{
    // TODO
}

void ghshtgn_fire(short snum)
{
    // TODO
}

void ghrifle_fire(short snum)
{
    // TODO
}

void ghpistol_fire(short snum)
{
    // TODO
}

void ghbow_fire(short snum)
{
    // TODO
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

void ghdeploy_plrtouchedsprite(short a1, short a2)
{
    // TODO
}

void ghstatbr_registerkillinfo(short a1, int a2, int a3)
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
