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

#define BF_TOUGH (BIT(0))
#define BF_KILL  (BIT(1))
#define BF_BURN  (BIT(2))
#define BF_OVERRIDE_BLOCK  (BIT(3))
#define BF_FIRE_FALL (BIT(4))
#define BF_LEAVE_BREAK (BIT(5))

typedef struct
{
    short picnum, breaknum, shrap_type;
    short flags, shrap_amt;
} BREAK_INFO, *BREAK_INFOp;

BREAK_INFOp FindWallBreakInfo(short picnum);
BREAK_INFOp FindSpriteBreakInfo(short picnum);
void SortBreakInfo(void);
BREAK_INFOp SetupWallForBreak(WALLp wallp);
BREAK_INFOp SetupSpriteForBreak(SPRITEp sp);
short FindBreakSpriteMatch(short match);
SWBOOL HitBreakWall(WALLp wp, int, int, int, short ang, short type);
int HitBreakSprite(short BreakSprite, short type);
SWBOOL CheckBreakToughness(BREAK_INFOp break_info, short ID);
int WallBreakPosition(short hit_wall, short *sectnum, int *x, int *y, int *z, short *ang);
void SortBreakInfo(void);

#endif
