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
#include "input.h"
#include "mapinfo.h"

BEGIN_PS_NS

struct AnimInfo
{
    int16_t a;
    int16_t repeat;
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

const int16_t nItemMagic[] = { 500, 1000, 100, 500, 400, 200, 700, 0 };

TArray<DExhumedActor*> Regenerates; // must handle read barriers manually!
int nMagicCount;

size_t MarkItems()
{
    GC::MarkArray(Regenerates);
    return Regenerates.Size();
}

void SerializeItems(FSerializer& arc)
{
    if (arc.BeginObject("items"))
    {
        arc("regenerates", Regenerates)
            ("magiccount", nMagicCount)
            .EndObject();
    }
}

void BuildItemAnim(DExhumedActor* pActor)
{
    int nItem = pActor->spr.statnum - 906;

    if (nItemAnimInfo[nItem].a >= 0)
    {
        auto pAnimActor = BuildAnim(pActor, 41, nItemAnimInfo[nItem].a, pActor->spr.pos.X, pActor->spr.pos.Y, pActor->spr.pos.Z, pActor->sector(), nItemAnimInfo[nItem].repeat, 20);

        if (nItem == 44) {
            pAnimActor->spr.cstat |= CSTAT_SPRITE_TRANSLUCENT;
        }

        ChangeActorStat(pAnimActor, pActor->spr.statnum);
        pAnimActor->spr.hitag = pActor->spr.hitag;
        pActor->spr.owner = 0;
    }
    else
    {
        pActor->spr.owner = -1;
        pActor->spr.yrepeat = (uint8_t)nItemAnimInfo[nItem].repeat;
        pActor->spr.xrepeat = (uint8_t)nItemAnimInfo[nItem].repeat;
    }
}

void DestroyItemAnim(DExhumedActor* actor)
{
    if (actor && actor->spr.owner >= 0) 
        DestroyAnim(actor);
}

void ItemFlash()
{
    TintPalette(16, 16, 16);
}

void FillItems(int nPlayer)
{
    for (int i = 0; i < 6; i++)
    {
        PlayerList[nPlayer].items[i] = 5;
    }

    PlayerList[nPlayer].nMagic = 1000;

    if (nPlayer == nLocalPlayer)
    {
        ItemFlash();
    }

    if (PlayerList[nPlayer].nItem == -1) {
        PlayerList[nPlayer].nItem = 0;
    }
}

static bool UseEye(int nPlayer)
{
    if (PlayerList[nPlayer].nInvisible >= 0) 
        PlayerList[nPlayer].nInvisible = 900;

    auto pActor = PlayerList[nPlayer].pActor;

    pActor->spr.cstat |= CSTAT_SPRITE_INVISIBLE;

    if (PlayerList[nPlayer].pPlayerFloorSprite != nullptr) {
        pActor->spr.cstat |= CSTAT_SPRITE_INVISIBLE;
    }

    if (nPlayer == nLocalPlayer)
    {
        ItemFlash();
        D3PlayFX(StaticSound[kSound31], pActor);
    }
    return true;
}

static bool UseMask(int nPlayer)
{
    PlayerList[nPlayer].nMaskAmount = 1350;
    PlayerList[nPlayer].nAir = 100;

    if (nPlayer == nLocalPlayer)
    {
        D3PlayFX(StaticSound[kSound31], PlayerList[nPlayer].pActor);
    }
    return true;
}

bool UseTorch(int nPlayer)
{
    if (!PlayerList[nPlayer].nTorch) 
    {
        SetTorch(nPlayer, 1);
    }

    PlayerList[nPlayer].nTorch = 900;
    return true;
}

bool UseHeart(int nPlayer)
{
    if (PlayerList[nPlayer].nHealth < kMaxHealth) {
        PlayerList[nPlayer].nHealth = kMaxHealth;

        if (nPlayer == nLocalPlayer)
        {
            ItemFlash();
            D3PlayFX(StaticSound[kSound31], PlayerList[nPlayer].pActor);
        }
        return true;
    }
    return false;
}

// invincibility
bool UseScarab(int nPlayer)
{
    if (PlayerList[nPlayer].invincibility >= 0 && PlayerList[nPlayer].invincibility < 900)
        PlayerList[nPlayer].invincibility = 900;

    if (nPlayer == nLocalPlayer)
    {
        ItemFlash();
        D3PlayFX(StaticSound[kSound31], PlayerList[nPlayer].pActor);
    }
    return true;
}

// faster firing
static bool UseHand(int nPlayer)
{
    PlayerList[nPlayer].nDouble = 1350;

    if (nPlayer == nLocalPlayer)
    {
        ItemFlash();
        D3PlayFX(StaticSound[kSound31], PlayerList[nPlayer].pActor);
    }
    return true;
}

void UseItem(int nPlayer, int nItem)
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
    PlayerList[nPlayer].nItem = nItem;
}

// TODO - bool return type?
int GrabItem(int nPlayer, int nItem)
{
    if (PlayerList[nPlayer].items[nItem] >= 5) {
        return 0;
    }

    PlayerList[nPlayer].items[nItem]++;

    if (PlayerList[nPlayer].nItem < 0 || nItem == PlayerList[nPlayer].nItem) {
        PlayerList[nPlayer].nItem = nItem;
    }

    return 1;
}

void DropMagic(DExhumedActor* pActor)
{
    if (lFinaleStart) {
        return;
    }

    nMagicCount--;

    if (nMagicCount <= 0)
    {
        auto pAnimActor = BuildAnim(
            nullptr,
            64,
            0,
            pActor->spr.pos.X,
            pActor->spr.pos.Y,
            pActor->spr.pos.Z,
            pActor->sector(),
            48,
            4);

        if (pAnimActor)
        {
            AddFlash(pAnimActor->sector(), pAnimActor->spr.pos.X, pAnimActor->spr.pos.Y, pAnimActor->spr.pos.Z, 128);
            ChangeActorStat(pAnimActor, 950);
        }
        nMagicCount = RandomSize(2);
    }
}

void InitItems()
{
    Regenerates.Clear();
    nMagicCount = 0;
}

void StartRegenerate(DExhumedActor* pActor)
{
    auto pos = Regenerates.Find(pActor);
    if (pos >= Regenerates.Size())
    {
        // ?? CHECKME
        pActor->spr.xvel = pActor->spr.xrepeat;
        pActor->spr.zvel = pActor->spr.shade;
        pActor->spr.yvel = pActor->spr.pal;
    }
    else
    {
        Regenerates.Delete(pos);
    }

    pActor->spr.extra = 1350;

    if (!(currentLevel->gameflags & LEVEL_EX_MULTI))
    {
        pActor->spr.ang /= 5;
    }

    pActor->spr.cstat = CSTAT_SPRITE_INVISIBLE;
    pActor->spr.xrepeat = 1;
    pActor->spr.yrepeat = 1;
    pActor->spr.pal = 1;

    Regenerates.Push(pActor);
}

void DoRegenerates()
{
    for(unsigned i = 0; i < Regenerates.Size(); i++)
    {
        DExhumedActor* pActor = GC::ReadBarrier(Regenerates[i]);
        if (pActor->spr.extra > 0)
        {
            pActor->spr.extra--;

            if (pActor->spr.extra <= 0)
            {
                BuildAnim(nullptr, 38, 0, pActor->spr.pos.X, pActor->spr.pos.Y, pActor->spr.pos.Z, pActor->sector(), 64, 4);
                D3PlayFX(StaticSound[kSoundTorchOn], pActor);
            }
            else {
                continue;
            }
        }
        else
        {
            if (pActor->spr.xrepeat < pActor->spr.xvel)
            {
                pActor->spr.xrepeat += 2;
                pActor->spr.yrepeat += 2;
                continue;
            }
        }

        pActor->spr.zvel = 0;
        pActor->spr.yrepeat = (uint8_t)pActor->spr.xvel;
        pActor->spr.xrepeat = (uint8_t)pActor->spr.xvel;
        pActor->spr.pal  = (uint8_t)pActor->spr.yvel;
        pActor->spr.yvel = pActor->spr.zvel; // setting to 0
        pActor->spr.xvel = pActor->spr.zvel; // setting to 0

        if (pActor->spr.statnum == kStatExplodeTrigger) {
            pActor->spr.cstat = CSTAT_SPRITE_BLOCK_ALL;
        }
        else {
            pActor->spr.cstat = 0;
        }
        Regenerates.Delete(i);
        i--;
    }
}
END_PS_NS
