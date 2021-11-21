//-------------------------------------------------------------------------
/*
Copyright (C) 2010-2019 EDuke32 developers and contributors
Copyright (C) 2019 sirlemonhead, Nuke.YKT
This file is part of PCExhumed.
PCExhumed is free software; you can redistribute it and/or
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
#include "build.h"
#include "exhumed.h"
#include "aistuff.h"
#include "player.h"
#include "sequence.h"
#include "names.h"
#include "engine.h"
#include "c_bind.h"
#include "status.h"
#include "sound.h"
#include "names.h"
#include "input.h"
#include "view.h"
#include "raze_sound.h"
#include "v_2ddrawer.h"
#include "gamestate.h"
#include "statistics.h"
#include "v_draw.h"
#include <string>
#include "mapinfo.h"

#include <assert.h>

BEGIN_PS_NS


uint8_t energytile[66 * 66] = {0};

uint8_t *cur;
uint8_t *dest;


unsigned int nRandom = 0x41C6167E;
int dword_9AB57 = 0x1F;
int word_9AB5B = 0;

int keytimer = 0;


unsigned int menu_RandomBit2()
{
    unsigned int result = nRandom & 1;

    if (--dword_9AB57 > 0)
    {
        nRandom = (result << 31) | (nRandom >> 1);
    }
    else
    {
        dword_9AB57 = 31;
        nRandom ^= nRandom >> 4;
    }
    return result;
}

void InitEnergyTile()
{
    word_9AB5B = 0;
    memset(energytile, 96, sizeof(energytile));
}

void DoEnergyTile()
{
    nButtonColor += nButtonColor < 0 ? 8 : 0;

    auto energy1 = TileFiles.tileMakeWritable(kEnergy1);
    auto energy2 = TileFiles.tileMakeWritable(kEnergy2);
    uint8_t* ptr1 = energy1 + 1984;
    uint8_t* ptr2 = energy1 + 2048;

    int nColor = nButtonColor + 161;

    int i, j;

    for (i = 0; i < 32; i++)
    {
        memset(ptr1, nColor, 64);
        memset(ptr2, nColor, 64);

        ptr1 -= 64;
        ptr2 += 64;

        nColor++;

        if (nColor >= 168) {
            nColor = 160;
        }
    }

    TileFiles.InvalidateTile(kEnergy1);

    if (nSmokeSparks)
    {
        uint8_t* c = &energytile[67]; // skip a line
        uint8_t* ptrW = energy2;

        for (i = 0; i < 64; i++)
        {
            for (j = 0; j < 64; j++)
            {
                uint8_t val = *c;

                if (val != 96)
                {
                    if (val > 158) {
                        *ptrW = val - 1;
                    }
                    else {
                        *ptrW = 96;
                    }
                }
                else
                {
                    if (menu_RandomBit2()) {
                        *ptrW = *c;
                    }
                    else
                    {
                        uint8_t al = *(c + 1);
                        uint8_t ah = *(c - 1);

                        if (al <= ah) {
                            al = ah;
                        }

                        uint8_t cl = al;

                        al = *(c - 66);
                        if (cl <= al) {
                            cl = al;
                        }

                        al = *(c + 66);
                        if (cl <= al) {
                            cl = al;
                        }

                        al = *(c + 66);
                        if (cl <= al) {
                            cl = al;
                        }

                        al = *(c + 66);
                        if (cl <= al) {
                            cl = al;
                        }

                        al = *(c - 65);
                        if (cl <= al) {
                            cl = al;
                        }

                        al = *(c - 67);
                        if (cl > al) {
                            al = cl;
                        }

                        cl = al;

                        if (al <= 159) {
                            *ptrW = 96;
                        }
                        else
                        {
                            if (!menu_RandomBit2()) {
                                cl--;
                            }

                            *ptrW = cl;
                        }
                    }
                }

                c++;
                ptrW++;
            }

            c += 2;
        }

        c = &energytile[67];
        ptrW = energy2;

        // copy back to energytile[]
        for (i = 0; i < 64; i++)
        {
            memcpy(c, ptrW, 64);
            c += 66;
            ptrW += 64;
        }

        ptrW = energy2;

        // kEnergy2 is 64 x 64
        for (i = 0; i < 4096; i++)
        {
            if (ptrW[i] == 96) {
                ptrW[i] = 0; // -1?
            }
        }

        word_9AB5B--;
        if (word_9AB5B <= 0)
        {
            int randSize = (RandomSize(5) & 0x1F) + 16;
            int randSize2 = (RandomSize(5) & 0x1F) + 16;

            int val = randSize << 5;
            val += randSize;
            val *= 2;
            val += randSize2;

            assert(val < 4356);

            energytile[val] = 175;
            word_9AB5B = 1;
        }
        TileFiles.InvalidateTile(kEnergy2);
    }
}

END_PS_NS
