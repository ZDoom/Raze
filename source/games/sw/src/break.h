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

#ifndef BREAK_PUBLIC_
#define BREAK_PUBLIC_

BEGIN_SW_NS

enum
{
    BF_TOUGH = BIT(0),
    BF_KILL = BIT(1),
    BF_BURN = BIT(2),
    BF_OVERRIDE_BLOCK = BIT(3),
    BF_FIRE_FALL = BIT(4),
    BF_LEAVE_BREAK = BIT(5),
};

struct BREAK_INFO
{
    int16_t picnum, breaknum, shrap_type;
    int16_t flags, shrap_amt;
};

BREAK_INFO* FindWallBreakInfo(int picnum);
BREAK_INFO* FindSpriteBreakInfo(int picnum);
void SortBreakInfo(void);
BREAK_INFO* SetupWallForBreak(walltype* wallp);
BREAK_INFO* SetupSpriteForBreak(DSWActor* actor);
bool HitBreakWall(walltype* wp, int, int, int, int ang, int type);
bool CheckBreakToughness(BREAK_INFO* break_info, int ID);
int WallBreakPosition(walltype* wp, sectortype** sectp, int* x, int* y, int* z, int* ang);
void SortBreakInfo(void);
void DoWallBreakMatch(int match);

END_SW_NS

#endif
