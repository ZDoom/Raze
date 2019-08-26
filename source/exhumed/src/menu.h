
#ifndef __menu_h__
#define __menu_h__

#include "player.h"
#include "typedefs.h"
#include <stdio.h>

#pragma pack(1)
// should be 75 bytes
struct GameStat
{
	uchar nMap;
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
void menu_GameLoad2(FILE *fp);
void menu_GameSave2(FILE *fp);
void menu_GameSave(int nSaveSlot);

int menu_DrawTheMap(int nLevel, int param_B, int param_C);

void DoEnergyTile();

int LoadCinemaPalette(int nPal);

void CinemaFadeIn();

void ReadyCinemaText(ushort nVal);
BOOL AdvanceCinemaText();

void DoFailedFinalScene();

void DoLastLevelCinema();
void DoAfterCinemaScene(int nLevel);

void InitEnergyTile();

#endif
