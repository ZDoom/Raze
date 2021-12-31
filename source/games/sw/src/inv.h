//-------------------------------------------------------------------------
/*
Copyright (C) 1997, 2005 - 3D Realms Entertainment

This file is part of Shadow Warrior version 1.2

Shadow Warrior is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.

Original Source: 1997 - Frank Maddin and Jim Norwood
Prepared for public release: 03/28/2005 - Charlie Wiederhold, 3D Realms
*/
//-------------------------------------------------------------------------

enum InventoryNames
{
	INVENTORY_MEDKIT,
	INVENTORY_REPAIR_KIT,
	INVENTORY_CLOAK,        // de-cloak when firing
	INVENTORY_NIGHT_VISION,
	INVENTORY_CHEMBOMB,
	INVENTORY_FLASHBOMB,
	INVENTORY_CALTROPS,
	MAX_INVENTORY
};

struct INVENTORY_DATA
{
	const char* Name;
	void (*Init)(PLAYER*);
	void (*Stop)(PLAYER*, short);
	short DecPerSec;
	short MaxInv;
	int  Scale;
	short Flags;
};

extern INVENTORY_DATA InventoryData[MAX_INVENTORY + 1];

enum EInvFlags
{
	INVF_AUTO_USE = 1,
	INVF_TIMED = 2,
	INVF_COUNT = 4
};

void PlayerUpdateInventory(PLAYER* pp, short InventoryNum);
void InventoryKeys(PLAYER* pp);
void UseInventoryRepairKit(PLAYER* pp);
void InventoryTimer(PLAYER* pp);
