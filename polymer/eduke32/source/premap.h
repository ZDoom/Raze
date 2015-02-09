//-------------------------------------------------------------------------
/*
Copyright (C) 2010 EDuke32 developers and contributors

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

#ifndef premap_h_
#define premap_h_

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    int32_t x1, y1;
    int32_t xdimen, ydimen;
} halfdimen_t;

extern halfdimen_t g_halfScreen;
extern int32_t g_halveScreenArea;

extern int32_t g_levelTextTime;
extern int32_t voting,vote_map,vote_episode;
extern palette_t CrosshairColors;
void G_SetupFilenameBasedMusic(char *levnamebuf, const char *boardfilename, int32_t level_number);
int32_t G_EnterLevel(int32_t g);
int32_t G_FindLevelByFile(const char *fn);
void G_CacheMapData(void);
void G_FreeMapState(int32_t mapnum);
void G_NewGame(int32_t vn,int32_t ln,int32_t sk);
void G_ResetTimers(uint8_t keepgtics);
void G_SetCrosshairColor(int32_t r,int32_t g,int32_t b);
void G_UpdateScreenArea(void);
void G_SetViewportShrink(int32_t dir);
void P_RandomSpawnPoint(int32_t snum);
void P_ResetInventory(int32_t snum);
void P_ResetPlayer(int32_t snum);
void P_ResetStatus(int32_t snum);
void P_ResetWeapons(int32_t snum);
void G_ClearFIFO(void);
void G_ResetInterpolations(void);

#ifdef __cplusplus
}
#endif

#endif
