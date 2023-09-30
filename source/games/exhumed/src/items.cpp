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
#include "engine.h"
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

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

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

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void BuildItemAnim(DExhumedActor* pActor)
{
    int nItem = pActor->spr.statnum - 906;

    if (nItemAnimInfo[nItem].a >= 0)
    {
        auto pAnimActor = BuildAnim(pActor, "items", nItemAnimInfo[nItem].a, pActor->spr.pos, pActor->sector(), nItemAnimInfo[nItem].repeat * REPEAT_SCALE, 20);

        if (nItem == 44) {
            pAnimActor->spr.cstat |= CSTAT_SPRITE_TRANSLUCENT;
        }

        ChangeActorStat(pAnimActor, pActor->spr.statnum);
        pAnimActor->spr.hitag = pActor->spr.hitag;
        pActor->spr.intowner = 0;
    }
    else
    {
        pActor->spr.intowner = -1;
		double s = nItemAnimInfo[nItem].repeat * REPEAT_SCALE;
        pActor->spr.scale = DVector2(s, s);
	}
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void DestroyItemAnim(DExhumedActor* actor)
{
    if (actor && actor->spr.intowner >= 0) 
        DestroyAnim(actor);
}

void ItemFlash()
{
    TintPalette(16, 16, 16);
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

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

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

static bool UseEye(int nPlayer)
{
    if (PlayerList[nPlayer].nInvisible >= 0) 
        PlayerList[nPlayer].nInvisible = 900;

    auto pActor = PlayerList[nPlayer].GetActor();

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

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

static bool UseMask(int nPlayer)
{
    PlayerList[nPlayer].nMaskAmount = 1350;
    PlayerList[nPlayer].nAir = 100;

    if (nPlayer == nLocalPlayer)
    {
        D3PlayFX(StaticSound[kSound31], PlayerList[nPlayer].GetActor());
    }
    return true;
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

bool UseTorch(int nPlayer)
{
    if (!PlayerList[nPlayer].nTorch) 
    {
        SetTorch(nPlayer, 1);
    }

    PlayerList[nPlayer].nTorch = 900;
    return true;
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

bool UseHeart(int nPlayer)
{
    if (PlayerList[nPlayer].nHealth < kMaxHealth) {
        PlayerList[nPlayer].nHealth = kMaxHealth;

        if (nPlayer == nLocalPlayer)
        {
            ItemFlash();
            D3PlayFX(StaticSound[kSound31], PlayerList[nPlayer].GetActor());
        }
        return true;
    }
    return false;
}

//---------------------------------------------------------------------------
//
// invincibility
//
//---------------------------------------------------------------------------

bool UseScarab(int nPlayer)
{
    if (PlayerList[nPlayer].invincibility >= 0 && PlayerList[nPlayer].invincibility < 900)
        PlayerList[nPlayer].invincibility = 900;

    if (nPlayer == nLocalPlayer)
    {
        ItemFlash();
        D3PlayFX(StaticSound[kSound31], PlayerList[nPlayer].GetActor());
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
        D3PlayFX(StaticSound[kSound31], PlayerList[nPlayer].GetActor());
    }
    return true;
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

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

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

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

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void DropMagic(DExhumedActor* pActor)
{
    if (lFinaleStart) {
        return;
    }

    nMagicCount--;

    if (nMagicCount <= 0)
    {
        if (const auto pAnimActor = BuildAnim(nullptr, "magic2", 0, pActor->spr.pos, pActor->sector(), 0.75,4))
        {
            AddFlash(pAnimActor->sector(), pAnimActor->spr.pos, 128);
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

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void StartRegenerate(DExhumedActor* pActor)
{
    auto pos = Regenerates.Find(pActor);
    if (pos >= Regenerates.Size())
    {
       // ?? CHECKME
        pActor->spr.xint = int16_t(pActor->spr.scale.X * INV_REPEAT_SCALE);
        pActor->spr.inittype = pActor->spr.shade;
        pActor->spr.yint = pActor->spr.pal;
    }
    else
    {
        Regenerates.Delete(pos);
    }

    pActor->spr.extra = 1350;

    if (!(currentLevel->gameflags & LEVEL_EX_MULTI))
    {
        pActor->spr.intangle /= 5; // what is this?
    }

    pActor->spr.cstat = CSTAT_SPRITE_INVISIBLE;
    pActor->spr.scale = DVector2(REPEAT_SCALE, REPEAT_SCALE);
    pActor->spr.pal = 1;

    Regenerates.Push(pActor);
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void DoRegenerates()
{
    for(unsigned i = 0; i < Regenerates.Size(); i++)
    {
        DExhumedActor* pActor = GC::ReadBarrier(Regenerates[i]);
		double s = pActor->spr.xint * REPEAT_SCALE;
        if (pActor->spr.extra > 0)
        {
            pActor->spr.extra--;

            if (pActor->spr.extra <= 0)
            {
                BuildAnim(nullptr, "firepoof", 0, pActor->spr.pos, pActor->sector(), 1, 4);
                D3PlayFX(StaticSound[kSoundTorchOn], pActor);
            }
            else {
                continue;
            }
        }
        else
        {
            if (pActor->spr.scale.X < s)
            {
				pActor->spr.scale.X += (0.03125);
				pActor->spr.scale.Y += (0.03125);
                continue;
            }
        }

		pActor->spr.scale = DVector2(s, s);
        pActor->spr.pal  = (uint8_t)pActor->spr.yint;
        pActor->spr.yint = 0;
        pActor->spr.xint = 0;

        pActor->vel.Y = 0;
        pActor->vel.X = 0;
        pActor->vel.Z = 0;


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
