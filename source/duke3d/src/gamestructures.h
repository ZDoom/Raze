//-------------------------------------------------------------------------
/*
Copyright (C) 2004-2020 EDuke32 developers and contributors

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

#pragma once
#ifndef gamestructures_h__
#define gamestructures_h__

#include "compat.h"
#include "hash.h"

BEGIN_DUKE_NS

int32_t __fastcall VM_GetUserdef(int32_t labelNum, int const lParm2);
void    __fastcall VM_SetUserdef(int const labelNum, int const lParm2, int32_t const newValue);
int32_t __fastcall VM_GetActiveProjectile(int const spriteNum, int32_t labelNum);
void    __fastcall VM_SetActiveProjectile(int const spriteNum, int const labelNum, int32_t const newValue);
int32_t __fastcall VM_GetPlayer(int const playerNum, int32_t labelNum, int const lParm2);
void    __fastcall VM_SetPlayer(int const playerNum, int const labelNum, int const lParm2, int32_t const newValue);
int32_t __fastcall VM_GetPlayerInput(int const playerNum, int32_t labelNum);
void    __fastcall VM_SetPlayerInput(int const playerNum, int const labelNum, int32_t const newValue);
int32_t __fastcall VM_GetWall(int const wallNum, int32_t labelNum);
void    __fastcall VM_SetWall(int const wallNum, int const labelNum, int32_t const newValue);
int32_t __fastcall VM_GetSector(int const sectNum, int32_t labelNum);
void    __fastcall VM_SetSector(int const sectNum, int const labelNum, int32_t newValue);
int32_t __fastcall VM_GetSprite(int const spriteNum, int32_t labelNum, int const lParm2);
void    __fastcall VM_SetSprite(int const spriteNum, int const labelNum, int const lParm2, int32_t const newValue);
int32_t __fastcall VM_GetProjectile(int const tileNum, int32_t labelNum);
void    __fastcall VM_SetProjectile(int const tileNum, int const labelNum, int32_t const newValue);
int32_t __fastcall VM_GetTileData(int const tileNum, int32_t labelNum);
void    __fastcall VM_SetTileData(int const tileNum, int const labelNum, int32_t const newValue);
int32_t __fastcall VM_GetPalData(int const palNum, int32_t labelNum);

 typedef struct
 {
     const char *name;

     int16_t  lId;
     uint16_t flags;
     int16_t  maxParm2;
     int16_t  offset;
 } memberlabel_t;

extern memberlabel_t const ActorLabels[];
extern memberlabel_t const InputLabels[];
extern memberlabel_t const PalDataLabels[];
extern memberlabel_t const PlayerLabels[];
extern memberlabel_t const ProjectileLabels[];
extern memberlabel_t const SectorLabels[];
extern memberlabel_t const TileDataLabels[];
extern memberlabel_t const TsprLabels[];
extern memberlabel_t const UserdefsLabels[];
extern memberlabel_t const WallLabels[];

extern hashtable_t h_actor;
extern hashtable_t h_input;
extern hashtable_t h_paldata;
extern hashtable_t h_player;
extern hashtable_t h_projectile;
extern hashtable_t h_sector;
extern hashtable_t h_tiledata;
extern hashtable_t h_tsprite;
extern hashtable_t h_userdef;
extern hashtable_t h_wall;

static hashtable_t *const vmStructHashTablePtrs[] = {
    &h_actor, &h_input, &h_paldata, &h_player, &h_projectile, &h_sector, &h_tiledata, &h_tsprite, &h_userdef, &h_wall,
};

END_DUKE_NS


#endif // gamestructures_h__