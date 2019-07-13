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
#include "callback.h"
#define kMaxChannels 4096

struct RXBUCKET
{
    unsigned int index : 13;
    unsigned int type : 3;
};
extern RXBUCKET rxBucket[];
extern unsigned short bucketHead[];

enum COMMAND_ID {
    COMMAND_ID_0 = 0,
    COMMAND_ID_1,
    COMMAND_ID_2,
    COMMAND_ID_3,
    COMMAND_ID_4,
    COMMAND_ID_5,
    COMMAND_ID_6,
    COMMAND_ID_7,
    COMMAND_ID_8,
    COMMAND_ID_9,

    kCommandCallback = 20,
    COMMAND_ID_21,
    kGDXCommandPaste = 53, // used by some new GDX types
    kGDXCommandSpriteDamage, // used by sprite damager GDX type
    COMMAND_ID_64 = 64,
};

struct EVENT {
    unsigned int index : 14; // index
    unsigned int type : 3; // type
    unsigned int cmd : 8; // cmd
    unsigned int funcID : 8; // callback
};

void evInit(void);
char evGetSourceState(int nType, int nIndex);
void evSend(int nIndex, int nType, int rxId, COMMAND_ID command);
void evPost(int nIndex, int nType, unsigned int nDelta, COMMAND_ID command);
void evPost(int nIndex, int nType, unsigned int nDelta, CALLBACK_ID a4);
void evProcess(unsigned int nTime);
void evKill(int a1, int a2);
void evKill(int a1, int a2, CALLBACK_ID a3);