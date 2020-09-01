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

#include "common_game.h"
#include "player.h"
#include "qav.h"

BEGIN_BLD_NS

class CChoke
{
public:
    CChoke()
    {
        at0 = NULL;
        at8 = NULL;
        atc = 0;
        at10 = 0;
        at1c = NULL;
        at14 = 0;
        at18 = 0;
    };
    void init(int a1, void(*a2)(PLAYER*));
    void animateChoke(int x, int y, int smoothratio);
	void reset() { at10 = 0; }
    char *at0;
    QAV *at8;
    int atc;
    int at10;
    int at14;
    int at18;
    void(*at1c)(PLAYER *);
};

void sub_84230(PLAYER*);

extern CChoke gChoke;

END_BLD_NS
