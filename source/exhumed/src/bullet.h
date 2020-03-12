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

#ifndef __bullet_h__
#define __bullet_h__

#include "aistuff.h"

BEGIN_PS_NS

// 32 bytes
struct bulletInfo
{
    short nDamage; // 0
    short field_2; // 2
    int field_4;   // 4
    short field_8; // 8
    short nSeq; // 10
    short field_C; // 12
    short nFlags;
    short nRadius; // damage radius
    short xyRepeat;
};

extern bulletInfo BulletInfo[];

extern short nRadialBullet;
extern short lasthitsect;
extern int lasthitz;
extern int lasthitx;
extern int lasthity;

void InitBullets();
short GrabBullet();
void DestroyBullet(short nRun);
int MoveBullet(short nBullet);
void SetBulletEnemy(short nBullet, short nEnemy);
int BuildBullet(short nSprite, int nType, int ebx, int ecx, int val1, int nAngle, int val2, int val3);
void IgniteSprite(int nSprite);
void FuncBullet(int, int, int);
void BackUpBullet(int *x, int *y, short nAngle);

END_PS_NS

#endif
