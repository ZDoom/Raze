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

#pragma once 

#include "compat.h"
#include "packet.h"

BEGIN_PS_NS

enum {
    kButtonCheatGuns = 0x20,
    kButtonCheatGodMode = 0x40,
    kButtonCheatKeys = 0x80,
    kButtonCheatItems = 0x100,
};

// 32 bytes
struct PlayerInput
{
    DExhumedActor* pTarget;
    int xVel;
    int yVel;
    uint16_t buttons;
    float nAngle;
    float pan;
    int8_t nItem;
    ESyncBits actions;

    int getNewWeapon() const
    {
        return (actions & SB_WEAPONMASK_BITS).GetValue();
    }

    void SetNewWeapon(int weap)
    {
        actions = (actions & ~SB_WEAPONMASK_BITS) | (ESyncBits::FromInt(weap) & SB_WEAPONMASK_BITS);
    }

};

void ClearSpaceBar(short nPlayer);

int GetLocalInput();

extern PlayerInput sPlayerInput[];
extern InputPacket localInput;
extern int lLocalCodes;

END_PS_NS

