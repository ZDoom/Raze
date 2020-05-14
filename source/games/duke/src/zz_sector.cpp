//-------------------------------------------------------------------------
/*
Copyright (C) 2010 EDuke32 developers and contributors

This file is part of EDuke32.

EDuke32 is free software; you can redistribute it and/or
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
#include "ns.h"	// Must come before everything else!

#define sector_c_

#include "duke3d.h"

#include "secrets.h"
#include "v_video.h"
#include "glbackend/glbackend.h"

BEGIN_DUKE_NS

// PRIMITIVE
void operatejaildoors(int hitag);

static int g_haltSoundHack = 0;

uint8_t shadedsector[MAXSECTORS];

int S_FindMusicSFX(int sectNum, int* sndptr)
{
    for (bssize_t SPRITES_OF_SECT(sectNum, spriteNum))
    {
        const int32_t snd = sprite[spriteNum].lotag;
        EDUKE32_STATIC_ASSERT(MAXSOUNDS >= 1000);

        if (PN(spriteNum) == MUSICANDSFX && (unsigned)snd < 1000)  // XXX: in other places, 999
        {
            *sndptr = snd;
            return spriteNum;
        }
    }

    *sndptr = -1;
    return -1;
}


static void G_SetupCamTile(int spriteNum, int smoothRatio)
{
    int const viewscrTile = TILE_VIEWSCR;
    TileFiles.MakeCanvas(viewscrTile, tilesiz[PN(spriteNum)].x, tilesiz[PN(spriteNum)].y);

    vec3_t const camera = G_GetCameraPosition(spriteNum, smoothRatio);
    int const    saveMirror = display_mirror;

    auto canvas = renderSetTarget(viewscrTile);
    if (!canvas) return;

    screen->RenderTextureView(canvas, [=](IntRect& rect)
        {
            yax_preparedrawrooms();
            drawrooms(camera.x, camera.y, camera.z, SA(spriteNum), 100 + sprite[spriteNum].shade, SECT(spriteNum));
            yax_drawrooms(G_DoSpriteAnimations, SECT(spriteNum), 0, smoothRatio);

            display_mirror = 3;
            G_DoSpriteAnimations(camera.x, camera.y, camera.z, SA(spriteNum), smoothRatio);
            display_mirror = saveMirror;
            renderDrawMasks();

        });
    renderRestoreTarget();
    
}

void G_AnimateCamSprite(int smoothRatio)
{
#ifdef DEBUG_VALGRIND_NO_SMC
    return;
#endif

    if (g_curViewscreen < 0)
        return;

    int const spriteNum = g_curViewscreen;

    if (totalclock >= T1(spriteNum) + ud.camera_time)
    {
        DukePlayer_t const *const pPlayer = g_player[screenpeek].ps;

        if (pPlayer->newowner >= 0)
            OW(spriteNum) = pPlayer->newowner;

        if (OW(spriteNum) >= 0 && dist(&sprite[pPlayer->i], &sprite[spriteNum]) < VIEWSCREEN_ACTIVE_DISTANCE)
        {

            G_SetupCamTile(OW(spriteNum), smoothRatio);
#ifdef POLYMER
            // Force texture update on viewscreen sprite in Polymer!
            if (videoGetRenderMode() == REND_POLYMER)
                polymer_invalidatesprite(spriteNum);
#endif
        }

        T1(spriteNum) = (int32_t) totalclock;
    }
}

void P_HandleSharedKeys(int playerNum)
{
    DukePlayer_t *const pPlayer = g_player[playerNum].ps;

    if (pPlayer->cheat_phase == 1) return;

    uint32_t playerBits = g_player[playerNum].input->bits;
    int32_t weaponNum;

    // 1<<0  =  jump
    // 1<<1  =  crouch
    // 1<<2  =  fire
    // 1<<3  =  aim up
    // 1<<4  =  aim down
    // 1<<5  =  run
    // 1<<6  =  look left
    // 1<<7  =  look right
    // 15<<8 = !weapon selection (bits 8-11)
    // 1<<12 = !steroids
    // 1<<13 =  look up
    // 1<<14 =  look down
    // 1<<15 = !nightvis
    // 1<<16 = !medkit
    // 1<<17 =  (multiflag==1) ? changes meaning of bits 18 and 19
    // 1<<18 =  centre view
    // 1<<19 = !holster weapon
    // 1<<20 = !inventory left
    // 1<<21 = !pause
    // 1<<22 = !quick kick
    // 1<<23 =  aim mode
    // 1<<24 = !holoduke
    // 1<<25 = !jetpack
    // 1<<26 =  g_gameQuit
    // 1<<27 = !inventory right
    // 1<<28 = !turn around
    // 1<<29 = !open
    // 1<<30 = !inventory
    // 1<<31 = !escape

    int const aimMode = pPlayer->aim_mode;

    pPlayer->aim_mode = (playerBits>>SK_AIMMODE)&1;
    if (pPlayer->aim_mode < aimMode)
        pPlayer->return_to_center = 9;

    if (RR)
    {
        if (TEST_SYNC_KEY(playerBits, SK_QUICK_KICK) && pPlayer->last_pissed_time == 0
            && (!RRRA || sprite[pPlayer->i].extra > 0))
        {
            pPlayer->last_pissed_time = 4000;
            if (!adult_lockout)
                A_PlaySound(437, pPlayer->i);
            if (sprite[pPlayer->i].extra <= max_player_health - max_player_health / 10)
            {
                sprite[pPlayer->i].extra += 2;
                pPlayer->last_extra = sprite[pPlayer->i].extra;
            }
            else if (sprite[pPlayer->i].extra < max_player_health)
                sprite[pPlayer->i].extra = max_player_health;
        }
    }
    else
    {

        if (TEST_SYNC_KEY(playerBits, SK_QUICK_KICK) && pPlayer->quick_kick == 0)
            if (pPlayer->curr_weapon != KNEE_WEAPON || pPlayer->kickback_pic == 0)
            {
                if (VM_OnEvent(EVENT_QUICKKICK,g_player[playerNum].ps->i,playerNum) == 0)
                {
                    pPlayer->quick_kick = 14;
                    if (pPlayer->fta == 0 || pPlayer->ftq == 80)
                        P_DoQuote(QUOTE_MIGHTY_FOOT,pPlayer);
                }
            }
    }

    if (!(playerBits & ((15u<<SK_WEAPON_BITS)|BIT(SK_STEROIDS)|BIT(SK_NIGHTVISION)|BIT(SK_MEDKIT)|BIT(SK_QUICK_KICK)| \
                   BIT(SK_HOLSTER)|BIT(SK_INV_LEFT)|BIT(SK_PAUSE)|BIT(SK_HOLODUKE)|BIT(SK_JETPACK)|BIT(SK_INV_RIGHT)| \
                   BIT(SK_TURNAROUND)|BIT(SK_OPEN)|BIT(SK_INVENTORY)|BIT(SK_ESCAPE))))
        pPlayer->interface_toggle_flag = 0;
    else if (pPlayer->interface_toggle_flag == 0)
    {
        pPlayer->interface_toggle_flag = 1;

        if (TEST_SYNC_KEY(playerBits, SK_PAUSE))
        {
            inputState.ClearKeyStatus(sc_Pause);
            if (ud.pause_on)
                ud.pause_on = 0;
            else ud.pause_on = 1+SHIFTS_IS_PRESSED;
            if (ud.pause_on)
            {
                Mus_SetPaused(true);
                S_PauseSounds(true);
            }
            else
            {
                Mus_SetPaused(false);

                S_PauseSounds(false);

                pub = NUMPAGES;
                pus = NUMPAGES;
            }
        }

        if (ud.pause_on) return;

        if (sprite[pPlayer->i].extra <= 0) return;		// if dead...

        if (TEST_SYNC_KEY(playerBits, SK_INVENTORY) && pPlayer->newowner == -1)	// inventory button generates event for selected item
        {
            if (VM_OnEvent(EVENT_INVENTORY,g_player[playerNum].ps->i,playerNum) == 0)
            {
                switch (pPlayer->inven_icon)
                {
                    case ICON_JETPACK: playerBits |= BIT(SK_JETPACK); break;
                    case ICON_HOLODUKE: playerBits |= BIT(SK_HOLODUKE); break;
                    case ICON_HEATS: playerBits |= BIT(SK_NIGHTVISION); break;
                    case ICON_FIRSTAID: playerBits |= BIT(SK_MEDKIT); break;
                    case ICON_STEROIDS: playerBits |= BIT(SK_STEROIDS); break;
                }
            }
        }

        if (!RR && TEST_SYNC_KEY(playerBits, SK_NIGHTVISION))
        {
            if (VM_OnEvent(EVENT_USENIGHTVISION,g_player[playerNum].ps->i,playerNum) == 0
                    &&  pPlayer->inv_amount[GET_HEATS] > 0)
            {
                pPlayer->heat_on = !pPlayer->heat_on;
                P_UpdateScreenPal(pPlayer);
                pPlayer->inven_icon = ICON_HEATS;
                A_PlaySound(NITEVISION_ONOFF,pPlayer->i);
                P_DoQuote(QUOTE_NVG_OFF-!!pPlayer->heat_on,pPlayer);
            }
        }

        if (TEST_SYNC_KEY(playerBits, SK_STEROIDS))
        {
            if (VM_OnEvent(EVENT_USESTEROIDS,g_player[playerNum].ps->i,playerNum) == 0)
            {
                if (pPlayer->inv_amount[GET_STEROIDS] == 400)
                {
                    pPlayer->inv_amount[GET_STEROIDS]--;
                    A_PlaySound(DUKE_TAKEPILLS,pPlayer->i);
                    P_DoQuote(QUOTE_USED_STEROIDS,pPlayer);
                }
                if (pPlayer->inv_amount[GET_STEROIDS] > 0)
                    pPlayer->inven_icon = ICON_STEROIDS;
            }
            return;		// is there significance to returning?
        }
        if (WW2GI && pPlayer->refresh_inventory)
            playerBits |= BIT(SK_INV_LEFT);   // emulate move left...

        if (pPlayer->newowner == -1 && (TEST_SYNC_KEY(playerBits, SK_INV_LEFT) || TEST_SYNC_KEY(playerBits, SK_INV_RIGHT)) || (!WW2GI && pPlayer->refresh_inventory))
        {
            pPlayer->invdisptime = GAMETICSPERSEC*2;

            int const inventoryRight = !!(TEST_SYNC_KEY(playerBits, SK_INV_RIGHT));

            if (pPlayer->refresh_inventory) pPlayer->refresh_inventory = 0;
            int32_t inventoryIcon = pPlayer->inven_icon;

            int i = 0;

CHECKINV1:
            if (i < 9)
            {
                i++;

                switch (inventoryIcon)
                {
                    case ICON_JETPACK:
                    case ICON_SCUBA:
                    case ICON_STEROIDS:
                    case ICON_HOLODUKE:
                    case ICON_HEATS:
                        if (pPlayer->inv_amount[icon_to_inv[inventoryIcon]] > 0 && i > 1)
                            break;
                        if (inventoryRight)
                            inventoryIcon++;
                        else
                            inventoryIcon--;
                        goto CHECKINV1;
                    case ICON_NONE:
                    case ICON_FIRSTAID:
                        if (pPlayer->inv_amount[GET_FIRSTAID] > 0 && i > 1)
                            break;
                        inventoryIcon = inventoryRight ? 2 : 7;
                        goto CHECKINV1;
                    case ICON_BOOTS:
                        if (pPlayer->inv_amount[GET_BOOTS] > 0 && i > 1)
                            break;
                        inventoryIcon = inventoryRight ? 1 : 6;
                        goto CHECKINV1;
                }
            }
            else inventoryIcon = 0;

            if (TEST_SYNC_KEY(playerBits, SK_INV_LEFT))   // Inventory_Left
            {
                /*Gv_SetVar(g_iReturnVarID,dainv,g_player[snum].ps->i,snum);*/
                inventoryIcon = VM_OnEventWithReturn(EVENT_INVENTORYLEFT,g_player[playerNum].ps->i,playerNum, inventoryIcon);
            }
            else if (TEST_SYNC_KEY(playerBits, SK_INV_RIGHT))   // Inventory_Right
            {
                /*Gv_SetVar(g_iReturnVarID,dainv,g_player[snum].ps->i,snum);*/
                inventoryIcon = VM_OnEventWithReturn(EVENT_INVENTORYRIGHT,g_player[playerNum].ps->i,playerNum, inventoryIcon);
            }

            if (inventoryIcon >= 1)
            {
                pPlayer->inven_icon = inventoryIcon;

                if (inventoryIcon || pPlayer->inv_amount[GET_FIRSTAID])
                {
                    static const int32_t invQuotes[7] = { QUOTE_MEDKIT, QUOTE_STEROIDS, QUOTE_HOLODUKE,
                        QUOTE_JETPACK, QUOTE_NVG, QUOTE_SCUBA, QUOTE_BOOTS };
                    if (inventoryIcon-1 < ARRAY_SSIZE(invQuotes))
                        P_DoQuote(invQuotes[inventoryIcon-1], pPlayer);
                }
            }
        }

        weaponNum = ((playerBits&(15<<SK_WEAPON_BITS))>>SK_WEAPON_BITS) - 1;
        if (weaponNum > 0 && pPlayer->kickback_pic > 0)
        {
            pPlayer->wantweaponfire = weaponNum;
        }

        if (pPlayer->last_pissed_time <= (GAMETICSPERSEC * 218) && pPlayer->show_empty_weapon == 0 &&
            pPlayer->kickback_pic == 0 && pPlayer->quick_kick == 0 && sprite[pPlayer->i].xrepeat > (RR ? 8 :32) && pPlayer->access_incs == 0 &&
            pPlayer->knee_incs == 0)
        {
            if(  (pPlayer->weapon_pos == 0 || (pPlayer->holster_weapon && pPlayer->weapon_pos == WEAPON_POS_LOWER ) ))
            {
                if (weaponNum == 10 || weaponNum == 11)
                {
                    int currentWeapon = pPlayer->curr_weapon;

                    if (RRRA)
                    {
                        if (currentWeapon == CHICKEN_WEAPON) currentWeapon = CROSSBOW_WEAPON;
                        else if (currentWeapon == GROW_WEAPON) currentWeapon = SHRINKER_WEAPON;
                        else if (currentWeapon == SLINGBLADE_WEAPON) currentWeapon = KNEE_WEAPON;
                    }

                    weaponNum = (weaponNum == 10 ? -1 : 1);  // JBF: prev (-1) or next (1) weapon choice
                    int i = 0;

                    while ((currentWeapon >= 0 && currentWeapon < 10) || (!RR && currentWeapon == GROW_WEAPON && (pPlayer->subweapon&(1 << GROW_WEAPON))))
                    {
                        if (!RR)
                        {
                            if (currentWeapon == GROW_WEAPON)
                            {
                                if (weaponNum == -1)
                                    currentWeapon = HANDBOMB_WEAPON;
                                else currentWeapon = DEVISTATOR_WEAPON;

                            }
                            else
                            {
                                currentWeapon += weaponNum;
                                if (currentWeapon == SHRINKER_WEAPON && pPlayer->subweapon&(1 << GROW_WEAPON))
                                    currentWeapon = GROW_WEAPON;
                            }
                        }
                        else
                            currentWeapon += weaponNum;

                        if (currentWeapon == -1) currentWeapon = TIT_WEAPON;
                        else if (currentWeapon == 10) currentWeapon = KNEE_WEAPON;

                        if ((pPlayer->gotweapon[currentWeapon]) && pPlayer->ammo_amount[currentWeapon] > 0)
                        {
                            if (!RR && currentWeapon == SHRINKER_WEAPON && pPlayer->subweapon&(1<<GROW_WEAPON))
                                currentWeapon = GROW_WEAPON;
                            weaponNum = currentWeapon;
                            break;
                        }
                        else if (!RR && currentWeapon == GROW_WEAPON && pPlayer->ammo_amount[GROW_WEAPON] == 0
                            && (pPlayer->gotweapon[SHRINKER_WEAPON]) && pPlayer->ammo_amount[SHRINKER_WEAPON] > 0)
                        {
                            weaponNum = SHRINKER_WEAPON;
                            pPlayer->subweapon &= ~(1<<GROW_WEAPON);
                            break;
                        }
                        else if (!RR && currentWeapon == SHRINKER_WEAPON && pPlayer->ammo_amount[SHRINKER_WEAPON] == 0
                            && (pPlayer->gotweapon[SHRINKER_WEAPON]) && pPlayer->ammo_amount[GROW_WEAPON] > 0)
                        {
                            weaponNum = GROW_WEAPON;
                            pPlayer->subweapon |= (1<<GROW_WEAPON);
                            break;
                        }

                        i++;

                        if (i == currentWeapon) // absolutely no weapons, so use foot
                        {
                            weaponNum = KNEE_WEAPON;
                            break;
                        }
                    }
                }

                if (weaponNum == HANDBOMB_WEAPON && pPlayer->ammo_amount[HANDBOMB_WEAPON] == 0)
                {
                    int spriteNum = headspritestat[STAT_ACTOR];
                    while (spriteNum >= 0)
                    {
                        if (sprite[spriteNum].picnum == TILE_HEAVYHBOMB && sprite[spriteNum].owner == pPlayer->i)
                        {
                            pPlayer->gotweapon.Set(HANDREMOTE_WEAPON);
                            weaponNum = HANDREMOTE_WEAPON;
                            break;
                        }
                        spriteNum = nextspritestat[spriteNum];
                    }
                }
                else if (RRRA)
                {
                    if (weaponNum == KNEE_WEAPON)
                    {
                        if(screenpeek == playerNum) pus = NUMPAGES;

                        if (pPlayer->curr_weapon == KNEE_WEAPON)
                        {
                            pPlayer->subweapon = 2;
                            weaponNum = SLINGBLADE_WEAPON;
                        }
                        else if(pPlayer->subweapon&2)
                        {
                            pPlayer->subweapon = 0;
                            weaponNum = KNEE_WEAPON;
                        }
                    }
                    else if (weaponNum == CROSSBOW_WEAPON)
                    {
                        if(screenpeek == playerNum) pus = NUMPAGES;

                        if (pPlayer->curr_weapon == CROSSBOW_WEAPON || pPlayer->ammo_amount[CROSSBOW_WEAPON] == 0)
                        {
                            if (pPlayer->ammo_amount[CHICKEN_WEAPON] == 0)
                                return;
                            pPlayer->subweapon = 4;
                            weaponNum = CHICKEN_WEAPON;
                        }
                        else if((pPlayer->subweapon&4) || pPlayer->ammo_amount[CHICKEN_WEAPON] == 0)
                        {
                            pPlayer->subweapon = 0;
                            weaponNum = CROSSBOW_WEAPON;
                        }
                    }
                }
                if (RR)
                {
                    if(weaponNum == SHRINKER_WEAPON)
                    {
                        if(screenpeek == playerNum) pus = NUMPAGES;

                        if (pPlayer->curr_weapon == SHRINKER_WEAPON || pPlayer->ammo_amount[SHRINKER_WEAPON] == 0)
                        {
                            pPlayer->subweapon = (1<<GROW_WEAPON);
                            weaponNum = GROW_WEAPON;
                        }
                        else if((pPlayer->subweapon&(1<<GROW_WEAPON)) || pPlayer->ammo_amount[GROW_WEAPON] == 0)
                        {
                            pPlayer->subweapon = 0;
                            weaponNum = SHRINKER_WEAPON;
                        }
                    }
                    else if(weaponNum == TRIPBOMB_WEAPON)
                    {
                        if(screenpeek == playerNum) pus = NUMPAGES;

                        if (pPlayer->curr_weapon == TRIPBOMB_WEAPON || pPlayer->ammo_amount[TRIPBOMB_WEAPON] == 0)
                        {
                            pPlayer->subweapon = (1<<BOWLING_WEAPON);
                            weaponNum = BOWLING_WEAPON;
                        }
                        else if((pPlayer->subweapon&(1<<BOWLING_WEAPON)) || pPlayer->ammo_amount[BOWLING_WEAPON] == 0)
                        {
                            pPlayer->subweapon = 0;
                            weaponNum = TRIPBOMB_WEAPON;
                        }
                    }
                }

                if (!RR && weaponNum == SHRINKER_WEAPON)
                {
                    if (screenpeek == playerNum) pus = NUMPAGES;

                    if (pPlayer->curr_weapon != GROW_WEAPON && pPlayer->curr_weapon != SHRINKER_WEAPON)
                    {
                        if (pPlayer->ammo_amount[GROW_WEAPON] > 0)
                        {
                            if ((pPlayer->subweapon&(1 << GROW_WEAPON)) == (1 << GROW_WEAPON))
                                weaponNum = GROW_WEAPON;
                            else if (pPlayer->ammo_amount[SHRINKER_WEAPON] == 0)
                            {
                                weaponNum = GROW_WEAPON;
                                pPlayer->subweapon |= (1 << GROW_WEAPON);
                            }
                        }
                        else if (pPlayer->ammo_amount[SHRINKER_WEAPON] > 0)
                            pPlayer->subweapon &= ~(1 << GROW_WEAPON);
                    }
                    else if (pPlayer->curr_weapon == SHRINKER_WEAPON)
                    {
                        pPlayer->subweapon |= (1 << GROW_WEAPON);
                        weaponNum = GROW_WEAPON;
                    }
                    else
                        pPlayer->subweapon &= ~(1 << GROW_WEAPON);
                }

                if (pPlayer->holster_weapon)
                {
                    playerBits |= BIT(SK_HOLSTER);
                    pPlayer->weapon_pos = WEAPON_POS_LOWER;
                }
                else if ((uint32_t)weaponNum < MAX_WEAPONS && (pPlayer->gotweapon[weaponNum]) && pPlayer->curr_weapon != weaponNum)
                    switch (DYNAMICWEAPONMAP(weaponNum))
                    {
                    case SLINGBLADE_WEAPON__STATIC:
                        if (!RRRA) break;
                        A_PlaySound(496,g_player[screenpeek].ps->i);
                        P_AddWeapon(pPlayer, weaponNum);
                        break;
                    case CHICKEN_WEAPON__STATIC:
                        if (!RRRA) break;
                        fallthrough__;
                    case BOWLINGBALL_WEAPON__STATIC:
                        if (!RR) break;
                        fallthrough__;
                    case PISTOL_WEAPON__STATIC:
                    case SHOTGUN_WEAPON__STATIC:
                    case CHAINGUN_WEAPON__STATIC:
                    case RPG_WEAPON__STATIC:
                    case DEVISTATOR_WEAPON__STATIC:
                    case FREEZE_WEAPON__STATIC:
                    case GROW_WEAPON__STATIC:
                    case SHRINKER_WEAPON__STATIC:
rrtripbomb_case:
                        if (pPlayer->ammo_amount[weaponNum] == 0 && pPlayer->show_empty_weapon == 0)
                        {
                            pPlayer->last_full_weapon = pPlayer->curr_weapon;
                            pPlayer->show_empty_weapon = 32;
                        }
                        fallthrough__;
                    case KNEE_WEAPON__STATIC:
                        P_AddWeapon(pPlayer, weaponNum);
                        break;
                    case HANDREMOTE_WEAPON__STATIC:
                        pPlayer->curr_weapon = HANDREMOTE_WEAPON;
                        pPlayer->last_weapon = -1;
                        pPlayer->weapon_pos = WEAPON_POS_RAISE;
                        break;
                    case HANDBOMB_WEAPON__STATIC:
                    case TRIPBOMB_WEAPON__STATIC:
                        if (RR && weaponNum == TILE_TRIPBOMB) goto rrtripbomb_case;
                        if (pPlayer->ammo_amount[weaponNum] > 0 && (pPlayer->gotweapon[weaponNum]))
                            P_AddWeapon(pPlayer, weaponNum);
                        break;
                    case MOTORCYCLE_WEAPON__STATIC:
                    case BOAT_WEAPON__STATIC:
                        if (!RRRA) break;
                        if (pPlayer->ammo_amount[weaponNum] == 0 && pPlayer->show_empty_weapon == 0)
                            pPlayer->show_empty_weapon = 32;
                        P_AddWeapon(pPlayer, weaponNum);
                        break;
                    }
            }

            if (TEST_SYNC_KEY(playerBits, SK_HOLSTER))
            {
                if (pPlayer->curr_weapon > KNEE_WEAPON)
                {
                    if (pPlayer->holster_weapon == 0 && pPlayer->weapon_pos == 0)
                    {
                        pPlayer->holster_weapon = 1;
                        pPlayer->weapon_pos = -1;
                        P_DoQuote(QUOTE_WEAPON_LOWERED, pPlayer);
                    }
                    else if (pPlayer->holster_weapon == 1 && pPlayer->weapon_pos == WEAPON_POS_LOWER)
                    {
                        pPlayer->holster_weapon = 0;
                        pPlayer->weapon_pos = WEAPON_POS_RAISE;
                        P_DoQuote(QUOTE_WEAPON_RAISED, pPlayer);
                    }
                }
            }
        }

        if (TEST_SYNC_KEY(playerBits, SK_HOLODUKE) && (RR || pPlayer->newowner == -1))
        {
            if (RR)
            {
                if (pPlayer->inv_amount[GET_HOLODUKE] > 0 && sprite[pPlayer->i].extra < max_player_health)
                {
                    pPlayer->inv_amount[GET_HOLODUKE] -= 400;
                    sprite[pPlayer->i].extra += 5;
                    if (sprite[pPlayer->i].extra > max_player_health)
                        sprite[pPlayer->i].extra = max_player_health;

                    pPlayer->drink_amt += 5;
                    pPlayer->inven_icon = 3;
                    if (pPlayer->inv_amount[GET_HOLODUKE] == 0)
                        P_SelectNextInvItem(pPlayer);

                    if (pPlayer->drink_amt < 99)
                        if (!A_CheckSoundPlaying(pPlayer->i, 425))
                            A_PlaySound(425, pPlayer->i);
                }
            }
            else
            {
                if (pPlayer->holoduke_on == -1)
                {
                    if (VM_OnEvent(EVENT_HOLODUKEON, g_player[playerNum].ps->i, playerNum) == 0)
                    {
                        if (pPlayer->inv_amount[GET_HOLODUKE] > 0)
                        {
                            pPlayer->inven_icon = ICON_HOLODUKE;

                            if (pPlayer->cursectnum > -1)
                            {
                                int const i = A_InsertSprite(pPlayer->cursectnum, pPlayer->pos.x, pPlayer->pos.y,
                                    pPlayer->pos.z+(30<<8), TILE_APLAYER, -64, 0, 0, fix16_to_int(pPlayer->q16ang), 0, 0, -1, 10);
                                pPlayer->holoduke_on = i;
                                T4(i) = T5(i) = 0;
                                sprite[i].yvel = playerNum;
                                sprite[i].extra = 0;
                                P_DoQuote(QUOTE_HOLODUKE_ON,pPlayer);
                                A_PlaySound(TELEPORTER,pPlayer->holoduke_on);
                            }
                        }
                        else P_DoQuote(QUOTE_HOLODUKE_NOT_FOUND,pPlayer);
                    }
                }
                else
                {
                    if (VM_OnEvent(EVENT_HOLODUKEOFF,g_player[playerNum].ps->i,playerNum) == 0)
                    {
                        A_PlaySound(TELEPORTER,pPlayer->holoduke_on);
                        pPlayer->holoduke_on = -1;
                        P_DoQuote(QUOTE_HOLODUKE_OFF,pPlayer);
                    }
                }
            }
        }

        if (RR && TEST_SYNC_KEY(playerBits, SK_NIGHTVISION) && pPlayer->newowner == -1 && pPlayer->yehaa_timer == 0)
        {
            pPlayer->yehaa_timer = 126;
            A_PlaySound(390, pPlayer->i);
            pPlayer->noise_radius = 16384;
            P_MadeNoise(playerNum);
            if (sector[pPlayer->cursectnum].lotag == 857)
            {
                if (sprite[pPlayer->i].extra <= max_player_health)
                {
                    sprite[pPlayer->i].extra += 10;
                    if (sprite[pPlayer->i].extra >= max_player_health)
                        sprite[pPlayer->i].extra = max_player_health;
                }
            }
            else
            {
                if (sprite[pPlayer->i].extra + 1 <= max_player_health)
                {
                    sprite[pPlayer->i].extra++;
                }
            }
        }

        if (TEST_SYNC_KEY(playerBits, SK_MEDKIT))
        {
            if (VM_OnEvent(EVENT_USEMEDKIT,g_player[playerNum].ps->i,playerNum) == 0)
            {
                if (pPlayer->inv_amount[GET_FIRSTAID] > 0 && sprite[pPlayer->i].extra < max_player_health)
                {
                    int healthDiff = max_player_health-sprite[pPlayer->i].extra;

                    if (RR) healthDiff = 10;

                    if (pPlayer->inv_amount[GET_FIRSTAID] > healthDiff)
                    {
                        pPlayer->inv_amount[GET_FIRSTAID] -= healthDiff;
                        if (RR)
                            sprite[pPlayer->i].extra += healthDiff;
                        if (!RR || sprite[pPlayer->i].extra > max_player_health)
                            sprite[pPlayer->i].extra = max_player_health;
                        pPlayer->inven_icon = ICON_FIRSTAID;
                    }
                    else
                    {
                        sprite[pPlayer->i].extra += pPlayer->inv_amount[GET_FIRSTAID];
                        pPlayer->inv_amount[GET_FIRSTAID] = 0;
                        P_SelectNextInvItem(pPlayer);
                    }
                    if (RR)
                    {
                        if (sprite[pPlayer->i].extra > max_player_health)
                            sprite[pPlayer->i].extra = max_player_health;
                        pPlayer->drink_amt += 10;
                    }
                    if (!RR || (pPlayer->drink_amt <= 100 && !A_CheckSoundPlaying(pPlayer->i, DUKE_USEMEDKIT)))
                        A_PlaySound(DUKE_USEMEDKIT,pPlayer->i);
                }
            }
        }

        if ((pPlayer->newowner == -1 || RR) && TEST_SYNC_KEY(playerBits, SK_JETPACK))
        {
            if (RR)
            {
                if (VM_OnEvent(EVENT_USEJETPACK,g_player[playerNum].ps->i,playerNum) == 0)
                {
                    if (pPlayer->inv_amount[GET_JETPACK] > 0 && sprite[pPlayer->i].extra < max_player_health)
                    {
                        if (!A_CheckSoundPlaying(pPlayer->i, 429))
                            A_PlaySound(429, pPlayer->i);

                        pPlayer->inv_amount[GET_JETPACK] -= 100;
                        if (pPlayer->drink_amt > 0)
                        {
                            pPlayer->drink_amt -= 5;
                            if (pPlayer->drink_amt < 0)
                                pPlayer->drink_amt = 0;
                        }

                        if (pPlayer->eat < 100)
                        {
                            pPlayer->eat += 5;
                            if (pPlayer->eat > 100)
                                pPlayer->eat = 100;
                        }

                        sprite[pPlayer->i].extra += 5;

                        pPlayer->inven_icon = 4;

                        if (sprite[pPlayer->i].extra > max_player_health)
                            sprite[pPlayer->i].extra = max_player_health;

                        if (pPlayer->inv_amount[GET_JETPACK] <= 0)
                            P_SelectNextInvItem(pPlayer);
                    }
                }
            }
            else
            {
                if (pPlayer->inv_amount[GET_JETPACK] > 0)
                {
                    pPlayer->jetpack_on = !pPlayer->jetpack_on;
                    if (pPlayer->jetpack_on)
                    {
                        pPlayer->inven_icon = ICON_JETPACK;
                        S_StopEnvSound(-1, pPlayer->i, CHAN_VOICE);

                        A_PlaySound(DUKE_JETPACK_ON,pPlayer->i);

                        P_DoQuote(QUOTE_JETPACK_ON,pPlayer);
                    }
                    else
                    {
                        pPlayer->hard_landing = 0;
                        pPlayer->vel.z = 0;
                        A_PlaySound(DUKE_JETPACK_OFF,pPlayer->i);
                        S_StopEnvSound(DUKE_JETPACK_IDLE,pPlayer->i);
                        S_StopEnvSound(DUKE_JETPACK_ON,pPlayer->i);
                        P_DoQuote(QUOTE_JETPACK_OFF,pPlayer);
                    }
                }
                else P_DoQuote(QUOTE_JETPACK_NOT_FOUND,pPlayer);
            }
        }

        if (TEST_SYNC_KEY(playerBits, SK_TURNAROUND) && pPlayer->one_eighty_count == 0)
            if (VM_OnEvent(EVENT_TURNAROUND,pPlayer->i,playerNum) == 0)
                pPlayer->one_eighty_count = -1024;
    }
}

int hitasprite(int spriteNum, int16_t *hitSprite)
{
    hitdata_t hitData;
    int32_t   zOffset = 0;

    if (A_CheckEnemySprite(&sprite[spriteNum]))
        zOffset = (42 << 8);
    else if (PN(spriteNum) == TILE_APLAYER)
        zOffset = (39 << 8);

    SZ(spriteNum) -= zOffset;
    hitscan((const vec3_t *)&sprite[spriteNum], SECT(spriteNum), sintable[(SA(spriteNum) + 512) & 2047],
            sintable[SA(spriteNum) & 2047], 0, &hitData, CLIPMASK1);
    SZ(spriteNum) += zOffset;

    if (hitSprite)
        *hitSprite = hitData.sprite;

    if (hitData.wall >= 0 && (wall[hitData.wall].cstat&16) && A_CheckEnemySprite( &sprite[spriteNum]))
        return 1<<30;

    return FindDistance2D(hitData.pos.x-SX(spriteNum),hitData.pos.y-SY(spriteNum));
}

int hitawall(DukePlayer_t *pPlayer, int *hitWall)
{
    hitdata_t hitData;

    hitscan((const vec3_t *)pPlayer, pPlayer->cursectnum, sintable[(fix16_to_int(pPlayer->q16ang) + 512) & 2047],
            sintable[fix16_to_int(pPlayer->q16ang) & 2047], 0, &hitData, CLIPMASK0);

    *hitWall = hitData.wall;

    if (hitData.wall < 0)
        return INT32_MAX;

    return FindDistance2D(hitData.pos.x - pPlayer->pos.x, hitData.pos.y - pPlayer->pos.y);
}


END_DUKE_NS
