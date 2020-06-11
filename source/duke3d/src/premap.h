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

BEGIN_EDUKE_NS

extern int32_t g_levelTextTime;
extern int32_t voting,vote_map,vote_episode;
int G_EnterLevel(int gameMode);
int G_FindLevelByFile(const char *fileName);
void G_CacheMapData(void);
void G_FreeMapState(int levelNum);
void G_NewGame(int volumeNum, int levelNum, int skillNum);
void G_ResetTimers(bool saveMoveCnt);
void G_UpdateScreenArea(void);
void P_MoveToRandomSpawnPoint(int playerNum);
void P_ResetInventory(int playerNum);
void P_ResetMultiPlayer(int playerNum);
void P_ResetPlayer(int playerNum);
void P_ResetWeapons(int playerNum);
void G_ClearFIFO(void);
void G_ResetInterpolations(void);
int fragbarheight(void);

END_EDUKE_NS

#endif
