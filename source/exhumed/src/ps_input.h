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

#ifndef __input_h__
#define __input_h__

#include "compat.h"
#include "packet.h"

BEGIN_PS_NS

// 32 bytes
struct PlayerInput
{
    int xVel;
    int yVel;
    fix16_t nAngle;
    uint16_t buttons;
    short nTarget;
    fix16_t horizon;
    int8_t nItem;
};

void InitInput();

void UpdateInputs();

void ClearSpaceBar(short nPlayer);

int GetLocalInput();

extern PlayerInput sPlayerInput[];
extern InputPacket localInput;
extern int nNetMoves;
extern int lLocalCodes;

END_PS_NS

#endif
