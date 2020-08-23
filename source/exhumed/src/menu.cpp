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
#include "compat.h"
#include "build.h"
#include "exhumed.h"
#include "aistuff.h"
#include "player.h"
#include "sequence.h"
#include "menu.h"
#include "names.h"
#include "engine.h"
#include "c_bind.h"
#include "status.h"
#include "sound.h"
#include "names.h"
#include "ps_input.h"
#include "view.h"
#include "raze_sound.h"
#include "menu.h"
#include "v_2ddrawer.h"
#include "gamestate.h"
#include "statistics.h"
#include "v_draw.h"
#include <string>

#include <assert.h>

BEGIN_PS_NS


#define kSaveFileName       "savgamea.sav"
#define kMaxSaveSlots		5
#define kMaxSaveSlotChars	25

GameStat GameStats;

uint8_t nCinemaSeen;

uint8_t energytile[66 * 66] = {0};

uint8_t *cur;
uint8_t *dest;


unsigned int nRandom = 0x41C6167E;
int dword_9AB57 = 0x1F;
short word_9AB5B = 0;

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
    memset(energytile, 96, sizeof(energytile));
}

void DoEnergyTile()
{
    nButtonColor += nButtonColor < 0 ? 8 : 0;

    auto energy1 = TileFiles.tileMakeWritable(kEnergy1);
    auto energy2 = TileFiles.tileMakeWritable(kEnergy2);
    uint8_t* ptr1 = energy1 + 1984;
    uint8_t* ptr2 = energy2 + 2048;

    short nColor = nButtonColor + 161;

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
                ptrW[i] = 255; // -1?
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

void menu_GameLoad2(FILE* fp, bool bIsDemo)
{
    fread(&GameStats, sizeof(GameStats), 1, fp);

    nPlayerWeapons[nLocalPlayer] = GameStats.nWeapons;

    PlayerList[nLocalPlayer].nCurrentWeapon = GameStats.nCurrentWeapon;
    nPlayerClip[nLocalPlayer] = GameStats.clip;

    int nPistolBullets = PlayerList[nLocalPlayer].nAmmo[kWeaponPistol];
    if (nPistolBullets >= 6) {
        nPistolBullets = 6;
    }

    nPistolClip[nLocalPlayer] = nPistolBullets;

    memcpy(&PlayerList[nLocalPlayer], &GameStats.player, sizeof(Player));

    nPlayerItem[nLocalPlayer] = GameStats.items;
    nPlayerLives[nLocalPlayer] = GameStats.nLives;

    SetPlayerItem(nLocalPlayer, nPlayerItem[nLocalPlayer]);
    CheckClip(nLocalPlayer);
}

short menu_GameLoad(int nSlot)
{
    memset(&GameStats, 0, sizeof(GameStats));

    FILE* fp = fopen(kSaveFileName, "rb");
    if (fp == NULL) {
        return 0;
    }

    fseek(fp, 125, SEEK_SET);
    fseek(fp, nSlot * sizeof(GameStats), SEEK_CUR);

    menu_GameLoad2(fp);
    fclose(fp);

    return GameStats.nMap;
}

void menu_GameSave2(FILE* fp)
{
    memset(&GameStats, 0, sizeof(GameStats));

    //GameStats.nMap = (uint8_t)levelnum;
    GameStats.nWeapons = nPlayerWeapons[nLocalPlayer];
    GameStats.nCurrentWeapon = PlayerList[nLocalPlayer].nCurrentWeapon;
    GameStats.clip = nPlayerClip[nLocalPlayer];
    GameStats.items = nPlayerItem[nLocalPlayer];
    GameStats.nLives = nPlayerLives[nLocalPlayer];

    memcpy(&GameStats.player, &PlayerList[nLocalPlayer], sizeof(GameStats.player));

    fwrite(&GameStats, sizeof(GameStats), 1, fp);
}

void menu_GameSave(int nSaveSlot)
{
    if (nSaveSlot < 0) {
        return;
    }

    FILE* fp = fopen(kSaveFileName, "rb+");
    if (fp != NULL)
    {
        fseek(fp, 125, SEEK_SET); // skip save slot names
        fseek(fp, sizeof(GameStat) * nSaveSlot, SEEK_CUR);
        menu_GameSave2(fp);
        fclose(fp);
    }
}

static SavegameHelper sgh("menu",
    SA(nCinemaSeen),
    SA(energytile),
    SV(nButtonColor),
    SV(word_9AB5B),
    nullptr);

END_PS_NS
