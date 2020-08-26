//-------------------------------------------------------------------------
/*
Copyright (C) 2010-2019 EDuke32 developers and contributors
Copyright (C) 2019 Nuke.YKT

This file is part of NBlood.

NBlood is free software; you can redistribute it and/or
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

BEGIN_BLD_NS

#pragma pack(push, 1)

enum
{
    flag_buttonmask = 127,
    flag_buttonmask_norun = 126
};

union SYNCFLAGS
{
    uint32_t value;
    struct
    {
        unsigned int run : 1;
        unsigned int jump : 1;
        unsigned int crouch : 1;
        unsigned int shoot : 1;
        unsigned int shoot2 : 1;
        unsigned int lookUp : 1;
        unsigned int lookDown : 1;
        unsigned int action : 1;
        unsigned int jab : 1;
        unsigned int prevItem : 1;
        unsigned int nextItem : 1;
        unsigned int useItem : 1;
        unsigned int prevWeapon : 1;
        unsigned int nextWeapon : 1;
        unsigned int holsterWeapon : 1;
        unsigned int lookCenter : 1;
        unsigned int lookLeft : 1;
        unsigned int lookRight : 1;
        unsigned int spin180 : 1;
        unsigned int pause : 1;
        unsigned int quit : 1;
        unsigned int restart : 1;
        unsigned int useBeastVision : 1;
        unsigned int useCrystalBall : 1;
        unsigned int useJumpBoots : 1;
        unsigned int useMedKit : 1;
        unsigned int newWeapon : 4;
    };
};
struct GINPUT
{
    SYNCFLAGS syncFlags;
    int16_t forward;
    int16_t strafe;
    fix16_t q16turn;
    fix16_t q16mlook;
};

#pragma pack(pop)

extern GINPUT gInput, gNetInput;
extern bool bSilentAim;

extern fix16_t gViewLook, gViewAngle;
extern float gViewAngleAdjust;
extern float gViewLookAdjust;
extern int gViewLookRecenter;

void ctrlInit();
void ctrlGetInput();

END_BLD_NS
