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
#include "player.h"

BEGIN_PS_NS

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

static DExhumedActor* feebtag(const DVector3& pos, sectortype* pSector, int nVal2, double deflen)
{
    DExhumedActor* pPickupActor = nullptr;

    auto startwall = pSector->walls.Data();
    int nWalls = pSector->walls.Size();

    int var_20 = nVal2 & 2;
    int var_14 = nVal2 & 1;

    while (1)
    {
        if (pSector != nullptr)
        {
            ExhumedSectIterator it(pSector);
            while (auto itActor = it.Next())
            {
                int nStat = itActor->spr.statnum;

                if (nStat >= 900 && !(itActor->spr.cstat & CSTAT_SPRITE_INVISIBLE))
                {
                    auto diff = itActor->spr.pos - pos;

                    if (diff.Z < 20 && diff.Z > -100)
                    {
                        double len = diff.XY().Length();

                        if (len < deflen && ((nStat != 950 && nStat != 949) || !(var_14 & 1)) && ((nStat != 912 && nStat != 913) || !(var_20 & 2)))
                        {
                            deflen = len;
                            pPickupActor = itActor;
                        }
                    }
                }
            }
        }

        nWalls--;
        if (nWalls < 0)
            return pPickupActor;

        pSector = startwall->nextSector();
        startwall++;
    }

    return pPickupActor;
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

static void doPickupNotification(Player* const pPlayer, const int nItem, const int nSound = -1, const int tintRed = 0, const int tintGreen = 16)
{
    if (pPlayer->nPlayer == nLocalPlayer)
    {
        if (nItemText[nItem] > -1 && nTotalPlayers == 1)
            pickupMessage(nItem);

        if (nSound > -1)
            PlayLocalSound(nSound, 0);

        TintPalette(tintRed * 4, tintGreen * 4, 0);
    }
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

static void doPickupDestroy(DExhumedActor* const pPickupActor, const int nItem)
{
    if (!(currentLevel->gameflags & LEVEL_EX_MULTI) || (nItem >= 25 && (nItem <= 25 || nItem == 50)))
    {
        // If this is an anim we need to properly destroy it so we need to do some proper detection and not wild guesses.
        if (pPickupActor->nRun == pPickupActor->nDamage && pPickupActor->nRun != 0 && pPickupActor->nPhase == ITEM_MAGIC)
        {
            DestroyAnim(pPickupActor);
        }
        else
        {
            DeleteActor(pPickupActor);
        }
    }
    else
    {
        StartRegenerate(pPickupActor);
    }
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

static void doPickupWeapon(Player* pPlayer, DExhumedActor* pPickupActor, int nItem, int nWeapon, int nAmount, int nSound = kSound72)
{
    const int weapFlag = 1 << nWeapon;

    if (pPlayer->nPlayerWeapons & weapFlag)
    {
        if (currentLevel->gameflags & LEVEL_EX_MULTI)
        {
            AddAmmo(pPlayer->nPlayer, WeaponInfo[nWeapon].nAmmoType, nAmount);
        }
    }
    else
    {
        SetNewWeaponIfBetter(pPlayer->nPlayer, nWeapon);
        pPlayer->nPlayerWeapons |= weapFlag;
        AddAmmo(pPlayer->nPlayer, WeaponInfo[nWeapon].nAmmoType, nAmount);
    }

    if (nWeapon == 2)
        CheckClip(pPlayer->nPlayer);

    if (nItem > 50)
    {
        pPickupActor->spr.cstat = CSTAT_SPRITE_INVISIBLE;
        DestroyItemAnim(pPickupActor);
    }
    else
    {
        doPickupDestroy(pPickupActor, nItem);
    }

    doPickupNotification(pPlayer, nItem, StaticSound[nSound]);
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

static void doPickupHealth(Player* pPlayer, DExhumedActor* pPickupActor, int nItem, const int nAmount, int nSound)
{
    if (nAmount <= 0 || pPlayer->nHealth < 800)
    {
        int tintRed = 0, tintGreen = 16;

        if (!pPlayer->invincibility || nAmount > 0)
        {
            pPlayer->nHealth += nAmount;

            if (pPlayer->nHealth > 800)
            {
                pPlayer->nHealth = 800;
            }
            else if (pPlayer->nHealth < 0)
            {
                nSound = -1;
                StartDeathSeq(pPlayer->nPlayer, 0);
            }
        }

        if (nItem == 12)
        {
            pPickupActor->spr.hitag = 0;
            pPickupActor->spr.picnum++;
            ChangeActorStat(pPickupActor, 0);
        }
        else
        {
            if (nItem == 14)
            {
                tintRed = tintGreen;
                tintGreen = 0;
            }

            doPickupDestroy(pPickupActor, nItem);
        }

        doPickupNotification(pPlayer, nItem, nSound, tintRed, tintGreen);
    }
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void doPlayerItemPickups(Player* const pPlayer)
{
    const auto pPlayerActor = pPlayer->pActor;
    const auto nFlags = (pPlayer->nMagic >= 1000) + 2 * (pPlayer->nHealth >= 800);
    const auto pPickupActor = feebtag(pPlayerActor->spr.pos, pPlayerActor->sector(), nFlags, 48);

    if (pPickupActor != nullptr && pPickupActor->spr.statnum >= 900)
    {
        const int nItem = pPickupActor->spr.statnum - 900;

        if (nItem <= 60)
        {
            static constexpr int itemArray[] = {kItemHeart, kItemInvincibility, kItemDoubleDamage, kItemInvisibility, kItemTorch, kItemMask};
            static constexpr int weapArray[] = {6, 24, 100, 20, 2};
            static constexpr int healArray[] = {40, 160, -200};
            static constexpr int ammoArray[] = {1, 3, 2};

            switch (nItem)
            {
            case 6: // Speed Loader
            case 7: // Fuel Canister
            case 8: // M - 60 Ammo Belt
                if (AddAmmo(pPlayer->nPlayer, ammoArray[nItem - 6], pPickupActor->spr.hitag))
                {
                    if (nItem == 8) CheckClip(pPlayer->nPlayer);
                    doPickupDestroy(pPickupActor, nItem);
                    doPickupNotification(pPlayer, nItem, StaticSound[kSoundAmmoPickup]);
                }
                break;

            case 9: // Grenade
            case 27: // May not be grenade, needs confirmation
            case 55:
                doPickupWeapon(pPlayer, pPickupActor, nItem, 4, 1, kSoundAmmoPickup);
                break;

            case 10: // Pickable item
            case 15: // Pickable item
            case 16: // Reserved
            case 24:
            case 31: // Check whether is grenade or not as it matches sequence for weapons below
            case 34:
            case 35:
            case 36:
            case 39:
            case 40:
            case 41:
            case 42:
            case 43:
            case 44:
            case 51:
            case 58:
                doPickupDestroy(pPickupActor, nItem);
                doPickupNotification(pPlayer, nItem);
                break;

            case 11: // Map
                GrabMap();
                doPickupDestroy(pPickupActor, nItem);
                doPickupNotification(pPlayer, nItem);
                break;

            case 12: // Berry Twig
            case 13: // Blood Bowl
            case 14: // Cobra Venom Bowl
                if (pPickupActor->spr.hitag != 0) 
                    doPickupHealth(pPlayer, pPickupActor, nItem, healArray[nItem - 12], nItem + 8);
                break;

            case 17: // Bubble Nest
                pPlayer->nAir += 10;

                if (pPlayer->nAir > 100)
                    pPlayer->nAir = 100; // TODO - constant

                if (pPlayer->nBreathTimer < 89)
                    D3PlayFX(StaticSound[kSound13], pPlayerActor);

                pPlayer->nBreathTimer = 90;
                break;

            case 18: // Still Beating Heart
            case 19: // Scarab amulet(Invicibility)
            case 20: // Severed Slave Hand(double damage)
            case 21: // Unseen eye(Invisibility)
            case 22: // Torch
            case 23: // Sobek Mask
                if (GrabItem(pPlayer->nPlayer, itemArray[nItem - 18]))
                {
                    doPickupDestroy(pPickupActor, nItem);
                    doPickupNotification(pPlayer, nItem);
                }
                break;

            case 25: // Extra Life
                if (pPlayer->nLives < kMaxPlayerLives)
                {
                    pPlayer->nLives++;
                    doPickupDestroy(pPickupActor, nItem);
                    doPickupNotification(pPlayer, nItem, -1, 32, 32);
                }
                break;

            case 26: // sword pickup??
                doPickupWeapon(pPlayer, pPickupActor, nItem, 0, 0);
                break;

            case 28: // .357 Magnum Revolver
            case 52:
            case 29: // M - 60 Machine Gun
            case 53:
            case 30: // Flame Thrower
            case 54:
            case 32: // Cobra Staff
            case 56:
            case 33: // Eye of Ra Gauntlet
            case 57:
            {
                const int index = nItem - 28 - 24 * (nItem > 50);
                doPickupWeapon(pPlayer, pPickupActor, nItem, index + 1, weapArray[index]);
                break;
            }

            case 37: // Cobra staff ammo
            case 38: // Raw Energy
                if (AddAmmo(pPlayer->nPlayer, nItem - 32, (nItem == 38) ? pPickupActor->spr.hitag : 1))
                {
                    doPickupDestroy(pPickupActor, nItem);
                    doPickupNotification(pPlayer, nItem, StaticSound[kSoundAmmoPickup]);
                }
                break;

            case 45: // Power key
            case 46: // Time key
            case 47: // War key
            case 48: // Earth key
            {
                const int keybit = 4096 << (nItem - 45);
                if (!(pPlayer->keys & keybit))
                {
                    pPlayer->keys |= keybit;
                    doPickupDestroy(pPickupActor, nItem);
                    doPickupNotification(pPlayer, nItem);
                }
                break;
            }

            case 49: // Magical Essence
            case 50: // ?
                if (pPlayer->nMagic < 1000)
                {
                    pPlayer->nMagic += 100;

                    if (pPlayer->nMagic >= 1000)
                        pPlayer->nMagic = 1000;

                    doPickupDestroy(pPickupActor, nItem);
                    doPickupNotification(pPlayer, nItem, StaticSound[kSoundMana1]);
                }
                break;

            case 59: // Scarab (Checkpoint)
                if (nLocalPlayer == pPlayer->nPlayer)
                {
                    pPickupActor->nIndex2++;
                    pPickupActor->nAction &= 0xEF;
                    pPickupActor->nIndex = 0;
                    ChangeActorStat(pPickupActor, 899);
                }
                SetSavePoint(pPlayer->nPlayer, pPlayerActor->spr.pos, pPlayerActor->sector(), pPlayerActor->spr.Angles.Yaw);
                break;

            case 60: // Golden Sarcophagus (End Level)
                if (!bInDemo) LevelFinished();
                DestroyItemAnim(pPickupActor);
                DeleteActor(pPickupActor);
                break;
            }
        }
    }
}

END_PS_NS
