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

enum
{
    kQAVNone = -1,

    kQAVFORKUP = 0,
    kQAVFORKIDLE = 1,
    kQAVPFORK = 2,
    kQAVFORKDOWN = 3,

    kQAVLITEOPEN = 4,
    kQAVLITEFLAM = 5,
    kQAVLITEIDLE = 6,
    kQAVLITECLO2 = 7,

    kQAVCANPREF = 8,
    kQAVCANIDLE = 9,
    kQAVCANFIRE = 10,
    kQAVCANDOWN = 11,
    kQAVCANFIRE2 = 12,
    kQAVCANDROP = 13,
    kQAVCANTHRO = 14,
    kQAVCANBOOM = 15,

    kQAVBUNUP = 16,
    kQAVBUNDOWN = 17,
    kQAVBUNUP2 = 18,
    kQAVBUNDOWN2 = 19,
    kQAVBUNIDLE = 20,
    kQAVBUNFUSE = 21,
    kQAVBUNDROP = 22,
    kQAVBUNTHRO = 23,

    kQAVDYNEXPLO = 24,

    kQAVPROXUP = 25,
    kQAVPROXDOWN = 26,
    kQAVPROXIDLE = 27,
    kQAVPROXDROP = 28,
    kQAVPROXTHRO = 29,

    kQAVREMUP1 = 30,
    kQAVREMUP2 = 31,
    kQAVREMUP3 = 32,
    kQAVREMDOWN1 = 33,
    kQAVREMDOWN2 = 34,
    kQAVREMDOWN3 = 35,
    kQAVREMIDLE1 = 36,
    kQAVREMIDLE2 = 37,
    kQAVREMDROP = 38,
    kQAVREMTHRO = 39,
    kQAVREMFIRE = 40,

    kQAVFLARUP = 41,
    kQAVFLARIDLE = 42,
    kQAVFLARFIR2 = 43,
    kQAVFLARDOWN = 44,

    kQAVFLAR2UP = 45,
    kQAVFLAR2I = 46,
    kQAVFLAR2F = 47,
    kQAVFLAR2FIR = 48,
    kQAVFLAR2DWN = 49,

    kQAVSHOTUP = 50,
    kQAVSHOTI3 = 51,
    kQAVSHOTI2 = 52,
    kQAVSHOTI1 = 53,
    kQAVSHOTF1 = 54,
    kQAVSHOTF2 = 55,
    kQAVSHOTF3 = 56,
    kQAVSHOTL1 = 57,
    kQAVSHOTDOWN = 58,

    kQAV2SHOTUP = 59,
    kQAV2SHOTI = 60,
    kQAV2SHOTF2 = 61,
    kQAV2SHOTFIR = 62,
    kQAV2SHOTDWN = 63,

    kQAVTOMUP = 64,
    kQAVTOMIDLE = 65,
    kQAVTOMFIRE = 66,
    kQAVTOMSPRED = 67,
    kQAVTOMDOWN = 68,

    kQAV2TOMUP = 69,
    kQAV2TOMIDLE = 70,
    kQAV2TOMFIRE = 71,
    kQAV2TOMDOWN = 72,
    kQAV2TOMALT = 73,

    kQAVSGUNUP = 74,
    kQAVSGUNIDL1 = 75,
    kQAVSGUNIDL2 = 76,
    kQAVSGUNFIR1 = 77,
    kQAVSGUNFIR4 = 78,
    kQAVSGUNPRE = 79,
    kQAVSGUNPOST = 80,
    kQAVSGUNDOWN = 81,

    kQAV2SGUNUP = 82,
    kQAV2SGUNIDL = 83,
    kQAV2SGUNFIR = 84,
    kQAV2SGUNALT = 85,
    kQAV2SGUNPRE = 86,
    kQAV2SGUNPST = 87,
    kQAV2SGUNDWN = 88,

    kQAVNAPUP = 89,
    kQAVNAPIDLE = 90,
    kQAVNAPFIRE = 91,
    kQAVNAPDOWN = 92,

    kQAVBSTUP = 93,
    kQAVBSTIDLE = 94,
    kQAVBSTATAK1 = 95,
    kQAVBSTATAK2 = 96,
    kQAVBSTATAK3 = 97,
    kQAVBSTATAK4 = 98,
    kQAVBSTDOWN = 99,

    kQAVVDUP = 100,
    kQAVVDIDLE1 = 101,
    kQAVVDIDLE2 = 102,
    kQAVVDFIRE1 = 103,
    kQAVVDFIRE2 = 104,
    kQAVVDFIRE3 = 105,
    kQAVVDFIRE4 = 106,
    kQAVVDFIRE5 = 107,
    kQAVVDFIRE6 = 108,
    kQAVVDDOWN = 109,
    kQAVVDSPEL1 = 110,

    kQAVSTAFUP = 111,
    kQAVSTAFIDL1 = 112,
    kQAVSTAFIDL3 = 113,
    kQAVSTAFIRE1 = 114,
    kQAVSTAFIRE2 = 115,
    kQAVSTAFIRE4 = 116,
    kQAVSTAFPRE = 117,
    kQAVSTAFPOST = 118,
    kQAVSTAFDOWN = 119,

    kQAV2NAPUP = 120,
    kQAV2NAPIDLE = 121,
    kQAV2NAPFIRE = 122,
    kQAV2NAPFIR2 = 123,
    kQAV2NAPDOWN = 124,

    kQAVEnd = 125,

    kQAVBDRIP = 256,
};

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
    double ticrate; // 0
    int nFrames; // 8
    int ticksPerFrame; // C
    int duration; // 10
    int x; // 14
    int y; // 18
    unsigned short res_id;
    FRAMEINFO frames[1]; // 24
    void Draw(double x, double y, int ticks, int stat, int shade, int palnum, bool to3dview, double const smoothratio = 65536);
    void Draw(int ticks, int stat, int shade, int palnum, bool to3dview, double const smoothratio = 65536) { Draw(x, y, ticks, stat, shade, palnum, to3dview, smoothratio); }
    void Play(int, int, int, PLAYER *);
    void Precache(int palette = 0);
};

QAV* getQAV(int res_id);
void qavProcessTicker(QAV* const pQAV, int* duration, int* lastTick);
void qavProcessTimer(PLAYER* const pPlayer, QAV* const pQAV, int* duration, double* smoothratio, bool const fixedduration = false, bool const ignoreWeaponTimer = false);

inline bool qavIsOriginal(const int& res_id)
{
    auto const lump = fileSystem.FindResource(res_id, "QAV");
    return lump >= 0 && fileSystem.GetFileContainer(lump) < fileSystem.GetMaxIwadNum();
}

inline int qavGetCorrectID(const int& res_id)
{
    return cl_bloodweapinterp && qavIsOriginal(res_id) && fileSystem.FindResource(res_id + 10000, "QAV") != -1 ? res_id + 10000 : res_id;
}

END_BLD_NS
