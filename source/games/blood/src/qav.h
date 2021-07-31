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

class F2DDrawer;

BEGIN_BLD_NS

enum { kQavOrientationLeft = 4096 };

#pragma pack(push, 1)

// by NoOne: add sound flags
enum
{
    kFlagSoundKill = 0x01, // mute QAV sounds of same priority
    kFlagSoundKillAll = 0x02, //  mute all QAV sounds

};

struct TILE_FRAME
{
    int picnum;
    int x;
    int y;
    int z;
    int stat;
    int8_t shade;
    char palnum;
    unsigned short angle;
};

struct SOUNDINFO
{
    int sound;
    uint8_t priority;
    uint8_t sndFlags; // (by NoOne) Various sound flags
    uint8_t sndRange; // (by NoOne) Random sound range
    char reserved[1];
};

struct FRAMEINFO
{
    int nCallbackId; // 0
    SOUNDINFO sound; // 4
    TILE_FRAME tiles[8]; // 12
};

struct QAV
{
    char pad1[8]; // 0
    int nFrames; // 8
    int ticksPerFrame; // C
    int duration; // 10
    int x; // 14
    int y; // 18
    int nSprite; // 1c
    //SPRITE *pSprite; // 1c
    char pad3[3]; // 20
    char lastframetic;
    FRAMEINFO frames[1]; // 24
    void Draw(double x, double y, int ticks, int stat, int shade, int palnum, bool to3dview, double const smoothratio = 65536, bool const menudrip = false);
    void Draw(int ticks, int stat, int shade, int palnum, bool to3dview, double const smoothratio = 65536, bool const menudrip = false) { Draw(x, y, ticks, stat, shade, palnum, to3dview, smoothratio, menudrip); }
    void Play(int, int, int, void *);
    void Precache(int palette = 0);
};

#pragma pack(pop)

QAV* getQAV(int res_id);

END_BLD_NS
