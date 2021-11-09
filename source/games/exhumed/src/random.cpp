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
#include "ns.h"
#include "engine.h"
#include "exhumed.h"

BEGIN_PS_NS

int randA = 0;
int randB = 0x11111111;
int randC = 0x1010101;

void SerializeRand(FSerializer& arc)
{
    if (arc.BeginObject("rand"))
    {
        arc("a", randA)
            ("b", randB)
            ("c", randC)
            .EndObject();
    }
}

void InitRandom()
{
    randA = 0;
    randB = 0x11111111;
    randC = 0x1010101;
}

// TODO - checkme
int RandomBit()
{
    randA = (randA >> 1) | (((randA ^ ((randA >> 1) ^ (randA >> 2) ^ (randA >> 31) ^ (randA >> 6) ^ (randA >> 4))) & 1) << 31);
    randB = (randB >> 1) | ((((randB >> 2) ^ (randB >> 30)) & 1) << 30);
    randC = (randC >> 1) | ((((randC >> 1) ^ (randC >> 28)) & 1) << 28);
    return (((randA == 0) & randC) | (randB & randA)) & 1;
}

uint8_t RandomByte()
{
    uint8_t randByte = RandomBit() << 7;
    randByte |= RandomBit() << 6;
    randByte |= RandomBit() << 5;
    randByte |= RandomBit() << 4;
    randByte |= RandomBit() << 3;
    randByte |= RandomBit() << 2;
    randByte |= RandomBit() << 1;
    randByte |= RandomBit();
    return randByte;
}

uint16_t RandomWord()
{
    uint16_t randWord = RandomByte() << 8;
    randWord |= RandomByte();
    return randWord;
}

int RandomLong()
{
    int randLong = IntToFixed(RandomWord());
    randLong |= RandomWord();
    return randLong;
}

int RandomSize(int nSize)
{
    int randSize = 0;

    while (nSize > 0)
    {
        randSize = randSize * 2 | RandomBit();
        nSize--;
    }

    return randSize;
}
END_PS_NS
