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

    int var_30 = 0;
    int var_40;

    if (pPlayer->nHealth >= 800)
    {
        var_30 = 2;
    }

    if (pPlayer->nMagic >= 1000)
    {
        var_30 |= 1;
    }

    DExhumedActor* pPickupActor = feebtag(pPlayerActor->spr.pos, pPlayerActor->sector(), var_30, 48);

    if (pPickupActor != nullptr && pPickupActor->spr.statnum >= 900)
    {
        int var_8C = 16;
        int var_88 = 9;

        int var_70 = pPickupActor->spr.statnum - 900;
        int var_44 = 0;

        // item lotags start at 6 (1-5 reserved?) so 0-offset them
        int itemtype = var_70 - 6;

        if (itemtype <= 54)
        {
            switch (itemtype)
            {
            do_default:
            default:
            {
                // loc_1B3C7

                // CHECKME - is order of evaluation correct?
                if (!mplevel || (var_70 >= 25 && (var_70 <= 25 || var_70 == 50)))
                {
                    // If this is an anim we need to properly destroy it so we need to do some proper detection and not wild guesses.
                    if (pPickupActor->nRun == pPickupActor->nDamage && pPickupActor->nRun != 0 && pPickupActor->nPhase == ITEM_MAGIC)
                        DestroyAnim(pPickupActor);
                    else
                        DeleteActor(pPickupActor);
                }
                else
                {
                    StartRegenerate(pPickupActor);
                }
            do_default_b:
                // loc_1BA74
                if (pPlayer->nPlayer == nLocalPlayer)
                {
                    if (nItemText[var_70] > -1 && nTotalPlayers == 1)
                    {
                        pickupMessage(var_70);
                    }

                    TintPalette(var_44 * 4, var_8C * 4, 0);

                    if (var_88 > -1)
                    {
                        PlayLocalSound(var_88, 0);
                    }
                }

                break;
            }
            case 0: // Speed Loader
            {
                if (AddAmmo(pPlayer->nPlayer, 1, pPickupActor->spr.hitag))
                {
                    var_88 = StaticSound[kSoundAmmoPickup];
                    goto do_default;
                }

                break;
            }
            case 1: // Fuel Canister
            {
                if (AddAmmo(pPlayer->nPlayer, 3, pPickupActor->spr.hitag))
                {
                    var_88 = StaticSound[kSoundAmmoPickup];
                    goto do_default;
                }
                break;
            }
            case 2: // M - 60 Ammo Belt
            {
                if (AddAmmo(pPlayer->nPlayer, 2, pPickupActor->spr.hitag))
                {
                    var_88 = StaticSound[kSoundAmmoPickup];
                    CheckClip(pPlayer->nPlayer);
                    goto do_default;
                }
                break;
            }
            case 3: // Grenade
            case 21:
            case 49:
            {
                if (AddAmmo(pPlayer->nPlayer, 4, 1))
                {
                    var_88 = StaticSound[kSoundAmmoPickup];
                    if (!(pPlayer->nPlayerWeapons & 0x10))
                    {
                        pPlayer->nPlayerWeapons |= 0x10;
                        SetNewWeaponIfBetter(pPlayer->nPlayer, 4);
                    }

                    if (var_70 == 55)
                    {
                        pPickupActor->spr.cstat = CSTAT_SPRITE_INVISIBLE;
                        DestroyItemAnim(pPickupActor);

                        // loc_1BA74: - repeated block, see in default case
                        if (pPlayer->nPlayer == nLocalPlayer)
                        {
                            if (nItemText[var_70] > -1 && nTotalPlayers == 1)
                            {
                                pickupMessage(var_70);
                            }

                            TintPalette(var_44 * 4, var_8C * 4, 0);

                            if (var_88 > -1)
                            {
                                PlayLocalSound(var_88, 0);
                            }
                        }
                        break;
                    }
                    else
                    {
                        goto do_default;
                    }
                }
                break;
            }

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
            {
                goto do_default;
            }

            case 5: // Map
            {
                GrabMap();
                goto do_default;
            }

            case 6: // Berry Twig
            {
                if (pPickupActor->spr.hitag == 0) {
                    break;
                }

                var_88 = 20;
                int edx = 40;

                if (edx <= 0 || (!(var_30 & 2)))
                {
                    if (!pPlayer->invincibility || edx > 0)
                    {
                        pPlayer->nHealth += edx;
                        if (pPlayer->nHealth > 800)
                        {
                            pPlayer->nHealth = 800;
                        }
                        else
                        {
                            if (pPlayer->nHealth < 0)
                            {
                                var_88 = -1;
                                StartDeathSeq(pPlayer->nPlayer, 0);
                            }
                        }
                    }

                    if (var_70 == 12)
                    {
                        pPickupActor->spr.hitag = 0;
                        pPickupActor->spr.picnum++;

                        ChangeActorStat(pPickupActor, 0);

                        // loc_1BA74: - repeated block, see in default case
                        if (pPlayer->nPlayer == nLocalPlayer)
                        {
                            if (nItemText[var_70] > -1 && nTotalPlayers == 1)
                            {
                                pickupMessage(var_70);
                            }

                            TintPalette(var_44 * 4, var_8C * 4, 0);

                            if (var_88 > -1)
                            {
                                PlayLocalSound(var_88, 0);
                            }
                        }

                        break;
                    }
                    else
                    {
                        if (var_70 != 14)
                        {
                            var_88 = 21;
                        }
                        else
                        {
                            var_44 = var_8C;
                            var_88 = 22;
                            var_8C = 0;
                        }

                        goto do_default;
                    }
                }

                break;
            }

            case 7: // Blood Bowl
            {
                int edx = 160;

                // Same code as case 6 now till break
                if (edx <= 0 || (!(var_30 & 2)))
                {
                    if (!pPlayer->invincibility || edx > 0)
                    {
                        pPlayer->nHealth += edx;
                        if (pPlayer->nHealth > 800)
                        {
                            pPlayer->nHealth = 800;
                        }
                        else
                        {
                            if (pPlayer->nHealth < 0)
                            {
                                var_88 = -1;
                                StartDeathSeq(pPlayer->nPlayer, 0);
                            }
                        }
                    }

                    if (var_70 == 12)
                    {
                        pPickupActor->spr.hitag = 0;
                        pPickupActor->spr.picnum++;

                        ChangeActorStat(pPickupActor, 0);

                        // loc_1BA74: - repeated block, see in default case
                        if (pPlayer->nPlayer == nLocalPlayer)
                        {
                            if (nItemText[var_70] > -1 && nTotalPlayers == 1)
                            {
                                pickupMessage(var_70);
                            }

                            TintPalette(var_44 * 4, var_8C * 4, 0);

                            if (var_88 > -1)
                            {
                                PlayLocalSound(var_88, 0);
                            }
                        }

                        break;
                    }
                    else
                    {
                        if (var_70 != 14)
                        {
                            var_88 = 21;
                        }
                        else
                        {
                            var_44 = var_8C;
                            var_88 = 22;
                            var_8C = 0;
                        }

                        goto do_default;
                    }
                }

                break;
            }

            case 8: // Cobra Venom Bowl
            {
                int edx = -200;

                // Same code as case 6 and 7 from now till break
                if (edx <= 0 || (!(var_30 & 2)))
                {
                    if (!pPlayer->invincibility || edx > 0)
                    {
                        pPlayer->nHealth += edx;
                        if (pPlayer->nHealth > 800)
                        {
                            pPlayer->nHealth = 800;
                        }
                        else
                        {
                            if (pPlayer->nHealth < 0)
                            {
                                var_88 = -1;
                                StartDeathSeq(pPlayer->nPlayer, 0);
                            }
                        }
                    }

                    if (var_70 == 12)
                    {
                        pPickupActor->spr.hitag = 0;
                        pPickupActor->spr.picnum++;

                        ChangeActorStat(pPickupActor, 0);

                        // loc_1BA74: - repeated block, see in default case
                        if (pPlayer->nPlayer == nLocalPlayer)
                        {
                            if (nItemText[var_70] > -1 && nTotalPlayers == 1)
                            {
                                pickupMessage(var_70);
                            }

                            TintPalette(var_44 * 4, var_8C * 4, 0);

                            if (var_88 > -1)
                            {
                                PlayLocalSound(var_88, 0);
                            }
                        }

                        break;
                    }
                    else
                    {
                        if (var_70 != 14)
                        {
                            var_88 = 21;
                        }
                        else
                        {
                            var_44 = var_8C;
                            var_88 = 22;
                            var_8C = 0;
                        }

                        goto do_default;
                    }
                }

                break;
            }

            case 11: // Bubble Nest
            {
                pPlayer->nAir += 10;
                if (pPlayer->nAir > 100) {
                    pPlayer->nAir = 100; // TODO - constant
                }

                if (pPlayer->nBreathTimer < 89)
                {
                    D3PlayFX(StaticSound[kSound13], pPlayerActor);
                }

                pPlayer->nBreathTimer = 90;
                break;
            }

            case 12: // Still Beating Heart
            {
                if (GrabItem(pPlayer->nPlayer, kItemHeart)) {
                    goto do_default;
                }

                break;
            }

            case 13: // Scarab amulet(Invicibility)
            {
                if (GrabItem(pPlayer->nPlayer, kItemInvincibility)) {
                    goto do_default;
                }

                break;
            }

            case 14: // Severed Slave Hand(double damage)
            {
                if (GrabItem(pPlayer->nPlayer, kItemDoubleDamage)) {
                    goto do_default;
                }

                break;
            }

            case 15: // Unseen eye(Invisibility)
            {
                if (GrabItem(pPlayer->nPlayer, kItemInvisibility)) {
                    goto do_default;
                }

                break;
            }

            case 16: // Torch
            {
                if (GrabItem(pPlayer->nPlayer, kItemTorch)) {
                    goto do_default;
                }

                break;
            }

            case 17: // Sobek Mask
            {
                if (GrabItem(pPlayer->nPlayer, kItemMask)) {
                    goto do_default;
                }

                break;
            }

            case 19: // Extra Life
            {
                var_88 = -1;

                if (pPlayer->nLives >= kMaxPlayerLives) {
                    break;
                }

                pPlayer->nLives++;

                var_8C = 32;
                var_44 = 32;
                goto do_default;
            }

            // FIXME - lots of repeated code from here down!!
            case 20: // sword pickup??
            {
                var_40 = 0;
                int ebx = 0;

                // loc_1B75D
                int var_18 = 1 << var_40;

                int weapons = pPlayer->nPlayerWeapons;

                if (weapons & var_18)
                {
                    if (mplevel)
                    {
                        AddAmmo(pPlayer->nPlayer, WeaponInfo[var_40].nAmmoType, ebx);
                    }
                }
                else
                {
                    weapons = var_40;

                    SetNewWeaponIfBetter(pPlayer->nPlayer, weapons);

                    pPlayer->nPlayerWeapons |= var_18;

                    AddAmmo(pPlayer->nPlayer, WeaponInfo[weapons].nAmmoType, ebx);

                    var_88 = StaticSound[kSound72];
                }

                if (var_40 == 2) {
                    CheckClip(pPlayer->nPlayer);
                }

                if (var_70 <= 50) {
                    goto do_default;
                }

                pPickupActor->spr.cstat = CSTAT_SPRITE_INVISIBLE;
                DestroyItemAnim(pPickupActor);
                ////
                        // loc_1BA74: - repeated block, see in default case
                if (pPlayer->nPlayer == nLocalPlayer)
                {
                    if (nItemText[var_70] > -1 && nTotalPlayers == 1)
                    {
                        pickupMessage(var_70);
                    }

                    TintPalette(var_44 * 4, var_8C * 4, 0);

                    if (var_88 > -1)
                    {
                        PlayLocalSound(var_88, 0);
                    }
                }

                break;
                /////
            }

            case 22: // .357 Magnum Revolver
            case 46:
            {
                var_40 = 1;
                int ebx = 6;

                // loc_1B75D
                int var_18 = 1 << var_40;

                int weapons = pPlayer->nPlayerWeapons;

                if (weapons & var_18)
                {
                    if (mplevel)
                    {
                        AddAmmo(pPlayer->nPlayer, WeaponInfo[var_40].nAmmoType, ebx);
                    }
                }
                else
                {
                    weapons = var_40;

                    SetNewWeaponIfBetter(pPlayer->nPlayer, weapons);

                    pPlayer->nPlayerWeapons |= var_18;

                    AddAmmo(pPlayer->nPlayer, WeaponInfo[weapons].nAmmoType, ebx);

                    var_88 = StaticSound[kSound72];
                }

                if (var_40 == 2) {
                    CheckClip(pPlayer->nPlayer);
                }

                if (var_70 <= 50) {
                    goto do_default;
                }

                pPickupActor->spr.cstat = CSTAT_SPRITE_INVISIBLE;
                DestroyItemAnim(pPickupActor);
                ////
                        // loc_1BA74: - repeated block, see in default case
                if (pPlayer->nPlayer == nLocalPlayer)
                {
                    if (nItemText[var_70] > -1 && nTotalPlayers == 1)
                    {
                        pickupMessage(var_70);
                    }

                    TintPalette(var_44 * 4, var_8C * 4, 0);

                    if (var_88 > -1)
                    {
                        PlayLocalSound(var_88, 0);
                    }
                }

                break;
                /////
            }

            case 23: // M - 60 Machine Gun
            case 47:
            {
                var_40 = 2;
                int ebx = 24;

                // loc_1B75D
                int var_18 = 1 << var_40;

                int weapons = pPlayer->nPlayerWeapons;

                if (weapons & var_18)
                {
                    if (mplevel)
                    {
                        AddAmmo(pPlayer->nPlayer, WeaponInfo[var_40].nAmmoType, ebx);
                    }
                }
                else
                {
                    weapons = var_40;

                    SetNewWeaponIfBetter(pPlayer->nPlayer, weapons);

                    pPlayer->nPlayerWeapons |= var_18;

                    AddAmmo(pPlayer->nPlayer, WeaponInfo[weapons].nAmmoType, ebx);

                    var_88 = StaticSound[kSound72];
                }

                if (var_40 == 2) {
                    CheckClip(pPlayer->nPlayer);
                }

                if (var_70 <= 50) {
                    goto do_default;
                }

                pPickupActor->spr.cstat = CSTAT_SPRITE_INVISIBLE;
                DestroyItemAnim(pPickupActor);
                ////
                        // loc_1BA74: - repeated block, see in default case
                if (pPlayer->nPlayer == nLocalPlayer)
                {
                    if (nItemText[var_70] > -1 && nTotalPlayers == 1)
                    {
                        pickupMessage(var_70);
                    }

                    TintPalette(var_44 * 4, var_8C * 4, 0);

                    if (var_88 > -1)
                    {
                        PlayLocalSound(var_88, 0);
                    }
                }

                break;
                /////
            }

            case 24: // Flame Thrower
            case 48:
            {
                var_40 = 3;
                int ebx = 100;

                // loc_1B75D
                int var_18 = 1 << var_40;

                int weapons = pPlayer->nPlayerWeapons;

                if (weapons & var_18)
                {
                    if (mplevel)
                    {
                        AddAmmo(pPlayer->nPlayer, WeaponInfo[var_40].nAmmoType, ebx);
                    }
                }
                else
                {
                    weapons = var_40;

                    SetNewWeaponIfBetter(pPlayer->nPlayer, weapons);

                    pPlayer->nPlayerWeapons |= var_18;

                    AddAmmo(pPlayer->nPlayer, WeaponInfo[weapons].nAmmoType, ebx);

                    var_88 = StaticSound[kSound72];
                }

                if (var_40 == 2) {
                    CheckClip(pPlayer->nPlayer);
                }

                if (var_70 <= 50) {
                    goto do_default;
                }

                pPickupActor->spr.cstat = CSTAT_SPRITE_INVISIBLE;
                DestroyItemAnim(pPickupActor);
                ////
                        // loc_1BA74: - repeated block, see in default case
                if (pPlayer->nPlayer == nLocalPlayer)
                {
                    if (nItemText[var_70] > -1 && nTotalPlayers == 1)
                    {
                        pickupMessage(var_70);
                    }

                    TintPalette(var_44 * 4, var_8C * 4, 0);

                    if (var_88 > -1)
                    {
                        PlayLocalSound(var_88, 0);
                    }
                }

                break;
                /////
            }

            case 26: // Cobra Staff
            case 50:
            {
                var_40 = 5;
                int ebx = 20;

                // loc_1B75D
                int var_18 = 1 << var_40;

                int weapons = pPlayer->nPlayerWeapons;

                if (weapons & var_18)
                {
                    if (mplevel)
                    {
                        AddAmmo(pPlayer->nPlayer, WeaponInfo[var_40].nAmmoType, ebx);
                    }
                }
                else
                {
                    weapons = var_40;

                    SetNewWeaponIfBetter(pPlayer->nPlayer, weapons);

                    pPlayer->nPlayerWeapons |= var_18;

                    AddAmmo(pPlayer->nPlayer, WeaponInfo[weapons].nAmmoType, ebx);

                    var_88 = StaticSound[kSound72];
                }

                if (var_40 == 2) {
                    CheckClip(pPlayer->nPlayer);
                }

                if (var_70 <= 50) {
                    goto do_default;
                }

                pPickupActor->spr.cstat = CSTAT_SPRITE_INVISIBLE;
                DestroyItemAnim(pPickupActor);
                ////
                        // loc_1BA74: - repeated block, see in default case
                if (pPlayer->nPlayer == nLocalPlayer)
                {
                    if (nItemText[var_70] > -1 && nTotalPlayers == 1)
                    {
                        pickupMessage(var_70);
                    }

                    TintPalette(var_44 * 4, var_8C * 4, 0);

                    if (var_88 > -1)
                    {
                        PlayLocalSound(var_88, 0);
                    }
                }

                break;
                /////
            }

            case 27: // Eye of Ra Gauntlet
            case 51:
            {
                var_40 = 6;
                int ebx = 2;

                // loc_1B75D
                int var_18 = 1 << var_40;

                int weapons = pPlayer->nPlayerWeapons;

                if (weapons & var_18)
                {
                    if (mplevel)
                    {
                        AddAmmo(pPlayer->nPlayer, WeaponInfo[var_40].nAmmoType, ebx);
                    }
                }
                else
                {
                    weapons = var_40;

                    SetNewWeaponIfBetter(pPlayer->nPlayer, weapons);

                    pPlayer->nPlayerWeapons |= var_18;

                    AddAmmo(pPlayer->nPlayer, WeaponInfo[weapons].nAmmoType, ebx);

                    var_88 = StaticSound[kSound72];
                }

                if (var_40 == 2) {
                    CheckClip(pPlayer->nPlayer);
                }

                if (var_70 <= 50) {
                    goto do_default;
                }

                pPickupActor->spr.cstat = CSTAT_SPRITE_INVISIBLE;
                DestroyItemAnim(pPickupActor);
                ////
                        // loc_1BA74: - repeated block, see in default case
                if (pPlayer->nPlayer == nLocalPlayer)
                {
                    if (nItemText[var_70] > -1 && nTotalPlayers == 1)
                    {
                        pickupMessage(var_70);
                    }

                    TintPalette(var_44 * 4, var_8C * 4, 0);

                    if (var_88 > -1)
                    {
                        PlayLocalSound(var_88, 0);
                    }
                }

                break;
                /////
            }

            case 31: // Cobra staff ammo
            {
                if (AddAmmo(pPlayer->nPlayer, 5, 1)) {
                    var_88 = StaticSound[kSoundAmmoPickup];
                    goto do_default;
                }

                break;
            }

            case 32: // Raw Energy
            {
                if (AddAmmo(pPlayer->nPlayer, 6, pPickupActor->spr.hitag)) {
                    var_88 = StaticSound[kSoundAmmoPickup];
                    goto do_default;
                }

                break;
            }

            case 39: // Power key
            case 40: // Time key
            case 41: // War key
            case 42: // Earth key
            {
                int keybit = 4096 << (itemtype - 39);

                var_88 = -1;

                if (!(pPlayer->keys & keybit))
                {
                    pPlayer->keys |= keybit;

                    if (nTotalPlayers > 1)
                    {
                        goto do_default_b;
                    }
                    else
                    {
                        goto do_default;
                    }
                }

                break;
            }

            case 43: // Magical Essence
            case 44: // ?
            {
                if (pPlayer->nMagic >= 1000) {
                    break;
                }

                var_88 = StaticSound[kSoundMana1];

                pPlayer->nMagic += 100;
                if (pPlayer->nMagic >= 1000) {
                    pPlayer->nMagic = 1000;
                }

                goto do_default;
            }

            case 53: // Scarab (Checkpoint)
            {
                if (nLocalPlayer == pPlayer->nPlayer)
                {
                    pPickupActor->nIndex2++;
                    pPickupActor->nAction &= 0xEF;
                    pPickupActor->nIndex = 0;

                    ChangeActorStat(pPickupActor, 899);
                }

                SetSavePoint(pPlayer->nPlayer, pPlayerActor->spr.pos, pPlayerActor->sector(), pPlayerActor->spr.Angles.Yaw);
                break;
            }

            case 54: // Golden Sarcophagus (End Level)
            {
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
}

END_PS_NS
