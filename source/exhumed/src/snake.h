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

#ifndef __snake_h__
#define __snake_h__

#include "aistuff.h"

BEGIN_PS_NS

#define kSnakeSprites	8 // or rename to kSnakeParts?

// 32bytes
struct Snake
{
    short nEnemy;	 // nRun
    short nSprites[kSnakeSprites];

    short sC;
    short nRun;

    // array?
    char c[8];
    /*
    char c1;
    char c2;
    char c3;
    char c4;
    char c5;
    char c6;
    char c7;
    char c8;
    */

    short sE;
};

extern Snake SnakeList[];

void InitSnakes();
short GrabSnake();
int BuildSnake(short nPlayer, short zVal);
void FuncSnake(int, int, int);

END_PS_NS

#endif
