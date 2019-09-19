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

#include "player.h"

class CGameMessageMgr
{
public:
    struct messageStruct
    {
        int at0;
        char at4[81];
    };
    char at0;
    int at1;
    int at5;
    int at9;
    int atd;
    int at11;
    int at15;
    int at19;
    int at1d;
    char at21;
    int at22;
    int at26;
    int at2a;
    messageStruct at2e[16];
    CGameMessageMgr();
    void SetState(char state);
    void Add(const char *, char);
    void Display(void);
    void Clear();
    void SetMaxMessages(int nMessages);
    void SetFont(int nFont);
    void SetCoordinates(int x, int y);
    void SetMessageTime(int nTime);
    void SetMessageFlags(unsigned int nFlags);
};


class CPlayerMsg
{
public:
    int at0;
    char at4[41];
    CPlayerMsg() { at0 = 0; at4[0] = 0; }
    void Clear(void);
    void Term(void);
    void Draw(void);
    bool AddChar(char);
    void DelChar(void);
    void Set(const char *pzString);
    void Send(void);
    void ProcessKeys(void);
};

class CCheatMgr
{
public:
    static bool m_bPlayerCheated;
    enum CHEATCODE
    {
        kCheatNone = 0,
        kCheat1, // refills ammo, no cheat code for it
        kCheatGriswold,
        kCheatSatchel,
        kCheatEvaGalli,
        kCheatMpkfa,
        kCheatCapInMyAss,
        kCheatNoCapInMyAss,
        kCheatIdaho,
        kCheatKevorkian,
        kCheatMcGee,
        kCheatEdmark,
        kCheatKrueger,
        kCheatSterno,
        kCheat14, // quake effect, not used
        kCheatSpork,
        kCheatGoonies,
        kCheatClarice,
        kCheatFrankenstein,
        kCheatCheeseHead,
        kCheatTequila,
        kCheatFunkyShoes,
        kCheatKeyMaster,
        kCheatOneRing,
        kCheatVoorhees,
        kCheatJoJo,
        kCheatGateKeeper,
        kCheatRate,
        kCheatMario,
        kCheatLaraCroft,
        kCheatHongKong,
        kCheatMontana,
        kCheatBunz,
        kCheatCousteau,
        kCheatForkYou,
        kCheatLieberMan,
        kCheatSpielberg,
        kCheatCalgon,
        kCheatMax
    };
    struct CHEATINFO
    {
        const char* pzString;
        CHEATCODE id;
        int flags;
    };
    static CHEATINFO s_CheatInfo[];
    CCheatMgr() {}
    bool Check(char *pzString);
    void Process(CHEATCODE nCheatCode, char* pzArgs);
    void sub_5BCF4(void);
};

extern CPlayerMsg gPlayerMsg;
extern CCheatMgr gCheatMgr;

void SetAmmo(bool stat);
void SetWeapons(bool stat);
void SetToys(bool stat);
void SetArmor(bool stat);
void SetKeys(bool stat);
void SetGodMode(bool god);
void SetClipMode(bool noclip);
