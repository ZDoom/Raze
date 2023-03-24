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

void doPlayerItemPickups(Player* const pPlayer)
{
    const auto pPlayerActor = pPlayer->pActor;
    const bool mplevel = (currentLevel->gameflags & LEVEL_EX_MULTI);
    const auto nFlags = (pPlayer->nMagic >= 1000) + 2 * (pPlayer->nHealth >= 800);
    const auto pPickupActor = feebtag(pPlayerActor->spr.pos, pPlayerActor->sector(), nFlags, 48);

    if (pPickupActor != nullptr && pPickupActor->spr.statnum >= 900)
    {
        // item lotags start at 6 (1-5 reserved?) so 0-offset them
        const int statBase = pPickupActor->spr.statnum - 900;
        const int itemtype = statBase - 6;

        if (itemtype <= 54)
        {
            static constexpr int itemArray[] = {kItemHeart, kItemInvincibility, kItemDoubleDamage, kItemInvisibility, kItemTorch, kItemMask};

            const auto doConsoleMessage = [=](const int nSound = -1, const int tintRed = 0, const int tintGreen = 16)
            {
                if (pPlayer->nPlayer == nLocalPlayer)
                {
                    if (nItemText[statBase] > -1 && nTotalPlayers == 1)
                        pickupMessage(statBase);

                    if (nSound > -1)
                        PlayLocalSound(nSound, 0);

                    TintPalette(tintRed * 4, tintGreen * 4, 0);
                }
            };
            const auto doProcessPickup = [=]()
            {
                if (!mplevel || (statBase >= 25 && (statBase <= 25 || statBase == 50)))
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
            };
            const auto doPickupWeapon = [=](const int nWeapon, const int nAmount)
            {
                const int weapFlag = 1 << nWeapon;

                if (pPlayer->nPlayerWeapons & weapFlag)
                {
                    if (mplevel)
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

                if (statBase <= 50)
                {
                    doProcessPickup();
                }
                else
                {
                    pPickupActor->spr.cstat = CSTAT_SPRITE_INVISIBLE;
                    DestroyItemAnim(pPickupActor);
                }

                doConsoleMessage(StaticSound[kSound72]);
            };
            const auto doPickupHealth = [=](const int nAmount, int nSound = -1)
            {
                if (nAmount <= 0 || (!(nFlags & 2)))
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
                            StartDeathSeq(pPlayer->nPlayer, 0);
                        }
                    }

                    if (statBase == 12)
                    {
                        pPickupActor->spr.hitag = 0;
                        pPickupActor->spr.picnum++;
                        ChangeActorStat(pPickupActor, 0);
                    }
                    else
                    {
                        if (statBase != 14)
                        {
                            nSound = 21;
                        }
                        else
                        {
                            tintRed = tintGreen;
                            nSound = 22;
                            tintGreen = 0;
                        }

                        doProcessPickup();
                    }

                    doConsoleMessage(nSound, tintRed, tintGreen);
                }
            };

            switch (itemtype)
            {
            case 0: // Speed Loader
                if (AddAmmo(pPlayer->nPlayer, 1, pPickupActor->spr.hitag))
                {
                    doProcessPickup();
                    doConsoleMessage(StaticSound[kSoundAmmoPickup]);
                }
                break;

            case 1: // Fuel Canister
                if (AddAmmo(pPlayer->nPlayer, 3, pPickupActor->spr.hitag))
                {
                    doProcessPickup();
                    doConsoleMessage(StaticSound[kSoundAmmoPickup]);
                }
                break;

            case 2: // M - 60 Ammo Belt
                if (AddAmmo(pPlayer->nPlayer, 2, pPickupActor->spr.hitag))
                {
                    CheckClip(pPlayer->nPlayer);
                    doProcessPickup();
                    doConsoleMessage(StaticSound[kSoundAmmoPickup]);
                }
                break;

            case 3: // Grenade
            case 21:
            case 49:
                if (AddAmmo(pPlayer->nPlayer, 4, 1))
                {
                    if (!(pPlayer->nPlayerWeapons & 0x10))
                    {
                        pPlayer->nPlayerWeapons |= 0x10;
                        SetNewWeaponIfBetter(pPlayer->nPlayer, 4);
                    }

                    if (statBase == 55)
                    {
                        pPickupActor->spr.cstat = CSTAT_SPRITE_INVISIBLE;
                        DestroyItemAnim(pPickupActor);
                    }
                    else
                    {
                        doProcessPickup();
                    }

                    doConsoleMessage(StaticSound[kSoundAmmoPickup]);
                }
                break;

            case 4: // Pickable item
            case 9: // Pickable item
            case 10: // Reserved
            case 18:
            case 25:
            case 28:
            case 29:
            case 30:
            case 33:
            case 34:
            case 35:
            case 36:
            case 37:
            case 38:
            case 45:
            case 52:
                doProcessPickup();
                doConsoleMessage();
                break;

            case 5: // Map
                GrabMap();
                doProcessPickup();
                doConsoleMessage();
                break;

            case 6: // Berry Twig
                if (pPickupActor->spr.hitag != 0) 
                {
                    doPickupHealth(40, 20);
                }
                break;

            case 7: // Blood Bowl
                doPickupHealth(160);
                break;

            case 8: // Cobra Venom Bowl
                doPickupHealth(-200);
                break;

            case 11: // Bubble Nest
                pPlayer->nAir += 10;

                if (pPlayer->nAir > 100)
                    pPlayer->nAir = 100; // TODO - constant

                if (pPlayer->nBreathTimer < 89)
                    D3PlayFX(StaticSound[kSound13], pPlayerActor);

                pPlayer->nBreathTimer = 90;
                break;

            case 12: // Still Beating Heart
            case 13: // Scarab amulet(Invicibility)
            case 14: // Severed Slave Hand(double damage)
            case 15: // Unseen eye(Invisibility)
            case 16: // Torch
            case 17: // Sobek Mask
                if (GrabItem(pPlayer->nPlayer, itemArray[itemtype - 12]))
                {
                    doProcessPickup();
                    doConsoleMessage();
                }
                break;

            case 19: // Extra Life
                if (pPlayer->nLives < kMaxPlayerLives)
                {
                    pPlayer->nLives++;
                    doProcessPickup();
                    doConsoleMessage(-1, 32, 32);
                }
                break;

            case 20: // sword pickup??
                doPickupWeapon(0, 0);
                break;

            case 22: // .357 Magnum Revolver
            case 46:
                doPickupWeapon(1, 6);
                break;

            case 23: // M - 60 Machine Gun
            case 47:
                doPickupWeapon(2, 24);
                break;

            case 24: // Flame Thrower
            case 48:
                doPickupWeapon(3, 100);
                break;

            case 26: // Cobra Staff
            case 50:
                doPickupWeapon(5, 20);
                break;

            case 27: // Eye of Ra Gauntlet
            case 51:
                doPickupWeapon(6, 2);
                break;

            case 31: // Cobra staff ammo
                if (AddAmmo(pPlayer->nPlayer, 5, 1))
                {
                    doProcessPickup();
                    doConsoleMessage(StaticSound[kSoundAmmoPickup]);
                }
                break;

            case 32: // Raw Energy
                if (AddAmmo(pPlayer->nPlayer, 6, pPickupActor->spr.hitag))
                {
                    doProcessPickup();
                    doConsoleMessage(StaticSound[kSoundAmmoPickup]);
                }
                break;

            case 39: // Power key
            case 40: // Time key
            case 41: // War key
            case 42: // Earth key
            {
                const int keybit = 4096 << (itemtype - 39);

                if (!(pPlayer->keys & keybit))
                {
                    pPlayer->keys |= keybit;
                    doProcessPickup();
                    doConsoleMessage();
                }
                break;
            }

            case 43: // Magical Essence
            case 44: // ?
                if (pPlayer->nMagic < 1000)
                {
                    pPlayer->nMagic += 100;

                    if (pPlayer->nMagic >= 1000)
                    {
                        pPlayer->nMagic = 1000;
                    }

                    doProcessPickup();
                    doConsoleMessage(StaticSound[kSoundMana1]);
                }
                break;

            case 53: // Scarab (Checkpoint)
                if (nLocalPlayer == pPlayer->nPlayer)
                {
                    pPickupActor->nIndex2++;
                    pPickupActor->nAction &= 0xEF;
                    pPickupActor->nIndex = 0;

                    ChangeActorStat(pPickupActor, 899);
                }

                SetSavePoint(pPlayer->nPlayer, pPlayerActor->spr.pos, pPlayerActor->sector(), pPlayerActor->spr.Angles.Yaw);
                break;

            case 54: // Golden Sarcophagus (End Level)
                if (!bInDemo)
                {
                    LevelFinished();
                }

                DestroyItemAnim(pPickupActor);
                DeleteActor(pPickupActor);
                break;
            }
        }
    }
}

END_PS_NS
