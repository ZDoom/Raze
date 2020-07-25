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


struct SEQFRAME {
    unsigned int tile : 12;
    unsigned int at1_4 : 1; // transparent
    unsigned int at1_5 : 1; // transparent
    unsigned int at1_6 : 1; // blockable
    unsigned int at1_7 : 1; // hittable
    unsigned int at2_0 : 8; // xrepeat
    unsigned int at3_0 : 8; // yrepeat
    signed int at4_0 : 8; // shade
    unsigned int at5_0 : 5; // palette
    unsigned int at5_5 : 1; //
    unsigned int at5_6 : 1; //
    unsigned int at5_7 : 1; //
    unsigned int at6_0 : 1; //
    unsigned int at6_1 : 1; //
    unsigned int at6_2 : 1; // invisible
    unsigned int at6_3 : 1; //
    unsigned int at6_4 : 1; //
    unsigned int tile2 : 4;
    unsigned soundRange : 4; // (by NoOne) random sound range relative to global SEQ sound
    unsigned surfaceSound : 1; // (by NoOne) trigger surface sound when moving / touching
    unsigned reserved : 2;
};

struct Seq {
    char signature[4];
    short version;
    short nFrames; // at6
    short at8;
    short ata;
    int atc;
    SEQFRAME frames[1];
    void Preload(void);
    void Precache(void);
};

struct ACTIVE
{
    unsigned char type;
    unsigned short xindex;
};

struct SEQINST
{
    Seq *pSequence;
    int at8;
    int atc;
    short at10;
    unsigned char frameIndex;
    char at13;
    void Update(ACTIVE *pActive);
};

inline int seqGetTile(SEQFRAME* pFrame)
{
    return pFrame->tile+(pFrame->tile2<<12);
}

int seqRegisterClient(void(*pClient)(int, int));
void seqPrecacheId(int id);
SEQINST * GetInstance(int a1, int a2);
void UnlockInstance(SEQINST *pInst);
void seqSpawn(int a1, int a2, int a3, int a4 = -1);
void seqKill(int a1, int a2);
void seqKillAll(void);
int seqGetStatus(int a1, int a2);
int seqGetID(int a1, int a2);
void seqProcess(int a1);

Seq* getSequence(int res_id);


END_BLD_NS
