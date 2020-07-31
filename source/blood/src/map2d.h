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
#include "build.h"
#include "fix16.h"

BEGIN_BLD_NS

class CViewMap {
public:
    char bActive;
    int x, y, nZoom;
    short angle;
    char bFollowMode;
    int forward, strafe;
    fix16_t turn;
    CViewMap();
    void sub_25C38(int, int, int, short, char);
    void sub_25C74(void);
    void sub_25DB0(spritetype *pSprite);
    void sub_25E84(int *, int*);
    void FollowMode(char);
};

extern CViewMap gViewMap;

END_BLD_NS
