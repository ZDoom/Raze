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

#ifndef __init_h__
#define __init_h__

#include "compat.h"

BEGIN_PS_NS

#define kMap20	20

enum {
    kSectUnderwater = 0x2000,
    kSectLava = 0x4000,
};

extern ClockTicks ototalclock;

extern int initx;
extern int inity;
extern int initz;
extern short inita;
extern short initsect;

extern short nCurChunkNum;
extern short nBodyGunSprite[50];
extern int movefifoend;
extern int movefifopos;
extern short nCurBodyGunNum;

void SnapSectors(short nSectorA, short nSectorB, short b);

extern short SectSound[];
extern short SectDamage[];
extern short SectSpeed[];
extern int SectBelow[];
extern short SectFlag[];
extern int SectDepth[];
extern short SectSoundSect[];
extern int SectAbove[];

uint8_t LoadLevel(int nMap);
void LoadObjects();

int myloadconfig();
int mysaveconfig();

END_PS_NS

#endif
