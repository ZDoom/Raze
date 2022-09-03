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
BEGIN_SW_NS

#define RECT_CLIP 1

Collision MultiClipMove(PLAYER* pp, double z, double floor_dist);
int MultiClipTurn(PLAYER* pp, DAngle new_ang, double zz, double floordist);
int RectClipMove(PLAYER* pp, DVector2* qpos);
int testpointinquad(const DVector2& pt, const DVector2* quad);
//short RectClipTurn(PLAYER* pp, short new_ang, int z, int floor_dist, int *qx, int *qy);
short RectClipTurn(PLAYER* pp, short new_ang, int *qx, int *qy, int *ox, int *oy);
END_SW_NS
