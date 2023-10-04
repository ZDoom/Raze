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

void FillItems(DExhumedPlayer* const pPlayer)
{
    for (int i = 0; i < 6; i++)
    {
        pPlayer->items[i] = 5;
    }

    pPlayer->nMagic = 1000;

    if (pPlayer->pnum == nLocalPlayer)
    {
        ItemFlash();
    }

    if (pPlayer->nItem == -1) {
        pPlayer->nItem = 0;
    }
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

static bool UseEye(DExhumedPlayer* const pPlayer)
{
    if (pPlayer->nInvisible >= 0) 
        pPlayer->nInvisible = 900;

    auto pActor = pPlayer->GetActor();

    pActor->spr.cstat |= CSTAT_SPRITE_INVISIBLE;

    if (pPlayer->pPlayerFloorSprite != nullptr) {
        pActor->spr.cstat |= CSTAT_SPRITE_INVISIBLE;
    }

    if (pPlayer->pnum == nLocalPlayer)
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

static bool UseMask(DExhumedPlayer* const pPlayer)
{
    pPlayer->nMaskAmount = 1350;
    pPlayer->nAir = 100;

    if (pPlayer->pnum == nLocalPlayer)
    {
        D3PlayFX(StaticSound[kSound31], pPlayer->GetActor());
    }
    return true;
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

bool UseTorch(DExhumedPlayer* const pPlayer)
{
    if (!pPlayer->nTorch) 
    {
        SetTorch(pPlayer, 1);
    }

    pPlayer->nTorch = 900;
    return true;
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

bool UseHeart(DExhumedPlayer* const pPlayer)
{
    if (pPlayer->nHealth < kMaxHealth) {
        pPlayer->nHealth = kMaxHealth;

        if (pPlayer->pnum == nLocalPlayer)
        {
            ItemFlash();
            D3PlayFX(StaticSound[kSound31], pPlayer->GetActor());
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

bool UseScarab(DExhumedPlayer* const pPlayer)
{
    if (pPlayer->invincibility >= 0 && pPlayer->invincibility < 900)
        pPlayer->invincibility = 900;

    if (pPlayer->pnum == nLocalPlayer)
    {
        ItemFlash();
        D3PlayFX(StaticSound[kSound31], pPlayer->GetActor());
    }
    return true;
}

// faster firing
static bool UseHand(DExhumedPlayer* const pPlayer)
{
    pPlayer->nDouble = 1350;

    if (pPlayer->pnum == nLocalPlayer)
    {
        ItemFlash();
        D3PlayFX(StaticSound[kSound31], pPlayer->GetActor());
    }
    return true;
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void UseItem(DExhumedPlayer* const pPlayer, int nItem)
{
    bool didit = false;
    switch (nItem)
    {
        case 0:
            didit = UseHeart(pPlayer);
            break;
        case 1:
            didit = UseScarab(pPlayer);
            break;
        case 2:
            didit = UseTorch(pPlayer);
            break;
        case 3:
            didit = UseHand(pPlayer);
            break;
        case 4:
            didit = UseEye(pPlayer);
            break;
        case 5:
            didit = UseMask(pPlayer);
            break;
        default:
            break;
    }
    if (!didit) return;

    pPlayer->items[nItem]--;
    int nItemCount = pPlayer->items[nItem];

    int nMagic = nItemMagic[nItem];

    if (!nItemCount)
    {
        for (nItem = 0; nItem < 6; nItem++)
        {
            if (pPlayer->items[nItem] > 0) {
                break;
            }
        }

        if (nItem == 6) {
            nItem = -1;
        }
    }

    pPlayer->nMagic -= nMagic;
    pPlayer->nItem = nItem;
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

int GrabItem(DExhumedPlayer* const pPlayer, int nItem)
{
    if (pPlayer->items[nItem] >= 5) {
        return 0;
    }

    pPlayer->items[nItem]++;

    if (pPlayer->nItem < 0 || nItem == pPlayer->nItem) {
        pPlayer->nItem = nItem;
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
