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
#include "aistuff.h"
#include "player.h"
#include "exhumed.h"
#include "sound.h"
#include "status.h"
#include "engine.h"
#include "ps_input.h"
#include "mapinfo.h"

BEGIN_PS_NS

struct AnimInfo
{
    short a;
    short repeat;
};

AnimInfo nItemAnimInfo[] = {
    { -1, 32 },
    { -1, 32 },
    { -1, 32 },
    { -1, 32 },
    { -1, 32 },
    { 6, 64 },
    { -1, 48 },
    { 0, 64 },
    { 1, 64 },
    { -1, 32 },
    { 4, 64 },
    { 5, 64 },
    { 16, 64 },
    { 10, 64 },
    { -1, 32 },
    { 8, 64 },
    { 9, 64 },
    { -1, 40 },
    { -1, 32 },
    { 7, 64 },
    { -1, 32 },
    { -1, 32 },
    { -1, 32 },
    { -1, 32 },
    { -1, 32 },
    { -1, 32 },
    { -1, 32 },
    { -1, 32 },
    { -1, 32 },
    { -1, 32 },
    { -1, 32 },
    { 14, 64 },
    { 15, 32 },
    { -1, 32 },
    { -1, 32 },
    { -1, 32 },
    { -1, 32 },
    { -1, 32 },
    { -1, 32 },
    { 17, 48 },
    { 18, 48 },
    { 19, 48 },
    { 20, 48 },
    { 24, 64 },
    { 21, 64 },
    { 23, 32 },
    { -1, 32 },
    { -1, 32 },
    { -1, 32 },
    { -1, 32 },
    { -1, 32 },
    { -1, 32 },
    { -1, 32 },
    { 11, 30 },
    { -1, 32 },
    { -1, 32 },
    { -1, 32 },
    { -1, 32 },
    { -1, 32 },
    { -1, 32 }
};

short nItemMagic[] = { 500, 1000, 100, 500, 400, 200, 700, 0 };

/*

short something
short x/y repeat

*/

short nRegenerates;
short nFirstRegenerate;
short nMagicCount;

static SavegameHelper sghitems("items",
    SV(nRegenerates),
    SV(nFirstRegenerate),
    SV(nMagicCount),
    nullptr);


void BuildItemAnim(short nSprite)
{
    int nItem = sprite[nSprite].statnum - 906;

    if (nItemAnimInfo[nItem].a >= 0)
    {
        int nAnim = BuildAnim(nSprite, 41, nItemAnimInfo[nItem].a, sprite[nSprite].x, sprite[nSprite].y, sprite[nSprite].z, sprite[nSprite].sectnum, nItemAnimInfo[nItem].repeat, 20);
        int nAnimSprite = GetAnimSprite(nAnim);

        if (nItem == 44) {
            sprite[nAnimSprite].cstat |= 2;
        }

        changespritestat(nAnimSprite, sprite[nSprite].statnum);

        sprite[nAnimSprite].owner = nAnim;
        sprite[nAnimSprite].hitag = sprite[nSprite].hitag;
    }
    else
    {
        sprite[nSprite].owner = -1;
        sprite[nSprite].yrepeat = nItemAnimInfo[nItem].repeat;
        sprite[nSprite].xrepeat = nItemAnimInfo[nItem].repeat;
    }
}

void DestroyItemAnim(short nSprite)
{
    short nAnim = sprite[nSprite].owner;

    if (nAnim >= 0) {
        DestroyAnim(nAnim);
    }
}

void ItemFlash()
{
    TintPalette(16, 16, 16);
}

void FillItems(short nPlayer)
{
    for (int i = 0; i < 6; i++)
    {
        PlayerList[nPlayer].items[i] = 5;
    }

    PlayerList[nPlayer].nMagic = 1000;

    if (nPlayer == nLocalPlayer)
    {
        ItemFlash();
        SetMagicFrame();
    }

    if (nPlayerItem[nPlayer] == -1) {
        SetPlayerItem(nPlayer, 0);
    }
}

static bool UseEye(short nPlayer)
{
    if (nPlayerInvisible[nPlayer] >= 0) 
        nPlayerInvisible[nPlayer] = 900;

    int nSprite = PlayerList[nPlayer].nSprite;

    sprite[nSprite].cstat |= 0x8000;

    if (nPlayerFloorSprite[nPlayer] >= 0) {
        sprite[nSprite].cstat |= 0x8000;
    }

    if (nPlayer == nLocalPlayer)
    {
        ItemFlash();
        D3PlayFX(StaticSound[kSound31], nSprite);
    }
    return true;
}

static bool UseMask(short nPlayer)
{
    PlayerList[nPlayer].nMaskAmount = 1350;
    PlayerList[nPlayer].nAir = 100;

    if (nPlayer == nLocalPlayer)
    {
        SetAirFrame();
        D3PlayFX(StaticSound[kSound31], PlayerList[nPlayer].nSprite);
    }
    return true;
}

bool UseTorch(short nPlayer)
{
    if (!nPlayerTorch[nPlayer]) 
    {
        SetTorch(nPlayer, 1);
    }

    nPlayerTorch[nPlayer] = 900;
    return true;
}

bool UseHeart(short nPlayer)
{
    if (PlayerList[nPlayer].nHealth < kMaxHealth) {
        PlayerList[nPlayer].nHealth = kMaxHealth;

        if (nPlayer == nLocalPlayer)
        {
            ItemFlash();
            SetHealthFrame(1);
            D3PlayFX(StaticSound[kSound31], PlayerList[nPlayer].nSprite);
        }
        return true;
    }
    return false;
}

// invincibility
bool UseScarab(short nPlayer)
{
    if (PlayerList[nPlayer].invincibility > 0 && PlayerList[nPlayer].invincibility < 900)
        PlayerList[nPlayer].invincibility = 900;

    if (nPlayer == nLocalPlayer)
    {
        ItemFlash();
        D3PlayFX(StaticSound[kSound31], PlayerList[nPlayer].nSprite);
    }
    return true;
}

// faster firing
static bool UseHand(short nPlayer)
{
    nPlayerDouble[nPlayer] = 1350;

    if (nPlayer == nLocalPlayer)
    {
        ItemFlash();
        D3PlayFX(StaticSound[kSound31], PlayerList[nPlayer].nSprite);
    }
    return true;
}

void UseItem(short nPlayer, short nItem)
{
    bool didit = false;
    switch (nItem)
    {
        case 0:
            didit = UseHeart(nPlayer);
            break;
        case 1:
            didit = UseScarab(nPlayer);
            break;
        case 2:
            didit = UseTorch(nPlayer);
            break;
        case 3:
            didit = UseHand(nPlayer);
            break;
        case 4:
            didit = UseEye(nPlayer);
            break;
        case 5:
            didit = UseMask(nPlayer);
            break;
        default:
            break;
    }
    if (!didit) return;

    PlayerList[nPlayer].items[nItem]--;
    int nItemCount = PlayerList[nPlayer].items[nItem];

    int nMagic = nItemMagic[nItem];

    if (nPlayer == nLocalPlayer)
    {
        BuildStatusAnim(156 + (nItemCount * 2), 0);
    }

    if (!nItemCount)
    {
        for (nItem = 0; nItem < 6; nItem++)
        {
            if (PlayerList[nPlayer].items[nItem] > 0) {
                break;
            }
        }

        if (nItem == 6) {
            nItem = -1;
        }
    }

    PlayerList[nPlayer].nMagic -= nMagic;
    SetPlayerItem(nPlayer, nItem);

    if (nPlayer == nLocalPlayer) {
        SetMagicFrame();
    }
}

// TODO - bool return type?
int GrabItem(short nPlayer, short nItem)
{
    if (PlayerList[nPlayer].items[nItem] >= 5) {
        return 0;
    }

    PlayerList[nPlayer].items[nItem]++;

    if (nPlayerItem[nPlayer] < 0 || nItem == nPlayerItem[nPlayer]) {
        SetPlayerItem(nPlayer, nItem);
    }

    return 1;
}

void DropMagic(short nSprite)
{
    if (lFinaleStart) {
        return;
    }

    nMagicCount--;

    if (nMagicCount <= 0)
    {
        int nAnim = BuildAnim(
            -1,
            64,
            0,
            sprite[nSprite].x,
            sprite[nSprite].y,
            sprite[nSprite].z,
            sprite[nSprite].sectnum,
            48,
            4);

        int nAnimSprite = GetAnimSprite(nAnim);

        sprite[nAnimSprite].owner = nAnim;

        AddFlash(sprite[nAnimSprite].sectnum, sprite[nAnimSprite].x, sprite[nAnimSprite].y, sprite[nAnimSprite].z, 128);
        changespritestat(nAnimSprite, 950);

        nMagicCount = RandomSize(2);
    }
}

void InitItems()
{
    nRegenerates = 0;
    nFirstRegenerate = -1;
    nMagicCount = 0;
}

void StartRegenerate(short nSprite)
{
    spritetype *pSprite = &sprite[nSprite];

    int edi = -1;

    int nReg = nFirstRegenerate;

    int i = 0;

//	for (int i = 0; i < nRegenerates; i++)
    while (1)
    {
        if (i >= nRegenerates)
        {
            // ?? CHECKME
            pSprite->xvel = pSprite->xrepeat;
            pSprite->zvel = pSprite->shade;
            pSprite->yvel = pSprite->pal;
            break;
        }
        else
        {
            if (nReg != nSprite)
            {
                edi = nReg;
                nReg = sprite[nReg].ang;
                i++;
                continue;
            }
            else
            {
                if (edi == -1)
                {
                    nFirstRegenerate = pSprite->ang;
                }
                else
                {
                    sprite[edi].ang = sprite[nSprite].ang;
                }

                nRegenerates--;
            }
        }
    }

    pSprite->extra = 1350;
    pSprite->ang = nFirstRegenerate;

    if (currentLevel->levelNumber <= kMap20)
    {
        pSprite->ang /= 5;
    }

    pSprite->cstat = 0x8000;
    pSprite->xrepeat = 1;
    pSprite->yrepeat = 1;
    pSprite->pal = 1;

    nRegenerates++;
    nFirstRegenerate = nSprite;
}

void DoRegenerates()
{
    int nSprite = nFirstRegenerate;

    for (int i = nRegenerates; i > 0; i--, nSprite = sprite[nSprite].ang)
    {
        if (sprite[nSprite].extra > 0)
        {
            sprite[nSprite].extra--;

            if (sprite[nSprite].extra <= 0)
            {
                BuildAnim(-1, 38, 0, sprite[nSprite].x, sprite[nSprite].y, sprite[nSprite].z, sprite[nSprite].sectnum, 64, 4);
                D3PlayFX(StaticSound[kSoundTorchOn], i);
            }
            else {
                continue;
            }
        }
        else
        {
            if (sprite[nSprite].xrepeat < sprite[nSprite].xvel)
            {
                sprite[nSprite].xrepeat += 2;
                sprite[nSprite].yrepeat += 2;
                continue;
            }
        }

        sprite[nSprite].zvel = 0;
        sprite[nSprite].yrepeat = sprite[nSprite].xvel;
        sprite[nSprite].xrepeat = sprite[nSprite].xvel;
        sprite[nSprite].pal  = sprite[nSprite].yvel;
        sprite[nSprite].yvel = sprite[nSprite].zvel; // setting to 0
        sprite[nSprite].xvel = sprite[nSprite].zvel; // setting to 0
        nRegenerates--;

        if (sprite[nSprite].statnum == kStatExplodeTrigger) {
            sprite[nSprite].cstat = 0x101;
        }
        else {
            sprite[nSprite].cstat = 0;
        }

        if (nRegenerates == 0) {
            nFirstRegenerate = -1;
        }
    }
}
END_PS_NS
