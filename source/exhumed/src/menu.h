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

#ifndef __menu_h__
#define __menu_h__

#include "player.h"
#include "typedefs.h"
#include <stdio.h>

BEGIN_PS_NS

#pragma pack(1)
// should be 75 bytes
struct GameStat
{
    uint8_t nMap;
    short nWeapons;
    short nCurrentWeapon;
    short clip;
    short items;

    Player player;

    short nLives;
};
#pragma pack()

extern GameStat GameStats;

extern unsigned char cinemapal[];

extern short SavePosition;

int showmap(short nLevel, short nLevelNew, short nLevelBest);

void ClearCinemaSeen();
void menu_DoPlasma();
int menu_Menu(int val);
void menu_AdjustVolume();
short menu_GameLoad(int nSlot);
void menu_GameLoad2(FILE *fp, bool bIsDemo = false);
void menu_GameSave2(FILE *fp);
void menu_GameSave(int nSaveSlot);

int menu_DrawTheMap(int nLevel, int param_B, int param_C);

void DoEnergyTile();

int LoadCinemaPalette(int nPal);

void CinemaFadeIn();

void ReadyCinemaText(uint16_t nVal);
bool AdvanceCinemaText();

void DoFailedFinalScene();

void DoLastLevelCinema();
void DoAfterCinemaScene(int nLevel);

void InitEnergyTile();

END_PS_NS

#endif
