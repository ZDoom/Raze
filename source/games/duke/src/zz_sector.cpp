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

uint8_t g_shadedSector[MAXSECTORS];

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
            if (sprite[pPlayer->i].extra <= pPlayer->max_player_health - pPlayer->max_player_health / 10)
            {
                sprite[pPlayer->i].extra += 2;
                pPlayer->last_extra = sprite[pPlayer->i].extra;
            }
            else if (sprite[pPlayer->i].extra < pPlayer->max_player_health)
                sprite[pPlayer->i].extra = pPlayer->max_player_health;
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
                if (pPlayer->inv_amount[GET_HOLODUKE] > 0 && sprite[pPlayer->i].extra < pPlayer->max_player_health)
                {
                    pPlayer->inv_amount[GET_HOLODUKE] -= 400;
                    sprite[pPlayer->i].extra += 5;
                    if (sprite[pPlayer->i].extra > pPlayer->max_player_health)
                        sprite[pPlayer->i].extra = pPlayer->max_player_health;

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
                if (sprite[pPlayer->i].extra <= pPlayer->max_player_health)
                {
                    sprite[pPlayer->i].extra += 10;
                    if (sprite[pPlayer->i].extra >= pPlayer->max_player_health)
                        sprite[pPlayer->i].extra = pPlayer->max_player_health;
                }
            }
            else
            {
                if (sprite[pPlayer->i].extra + 1 <= pPlayer->max_player_health)
                {
                    sprite[pPlayer->i].extra++;
                }
            }
        }

        if (TEST_SYNC_KEY(playerBits, SK_MEDKIT))
        {
            if (VM_OnEvent(EVENT_USEMEDKIT,g_player[playerNum].ps->i,playerNum) == 0)
            {
                if (pPlayer->inv_amount[GET_FIRSTAID] > 0 && sprite[pPlayer->i].extra < pPlayer->max_player_health)
                {
                    int healthDiff = pPlayer->max_player_health-sprite[pPlayer->i].extra;

                    if (RR) healthDiff = 10;

                    if (pPlayer->inv_amount[GET_FIRSTAID] > healthDiff)
                    {
                        pPlayer->inv_amount[GET_FIRSTAID] -= healthDiff;
                        if (RR)
                            sprite[pPlayer->i].extra += healthDiff;
                        if (!RR || sprite[pPlayer->i].extra > pPlayer->max_player_health)
                            sprite[pPlayer->i].extra = pPlayer->max_player_health;
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
                        if (sprite[pPlayer->i].extra > pPlayer->max_player_health)
                            sprite[pPlayer->i].extra = pPlayer->max_player_health;
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
                    if (pPlayer->inv_amount[GET_JETPACK] > 0 && sprite[pPlayer->i].extra < pPlayer->max_player_health)
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

                        if (pPlayer->eat_amt < 100)
                        {
                            pPlayer->eat_amt += 5;
                            if (pPlayer->eat_amt > 100)
                                pPlayer->eat_amt = 100;
                        }

                        sprite[pPlayer->i].extra += 5;

                        pPlayer->inven_icon = 4;

                        if (sprite[pPlayer->i].extra > pPlayer->max_player_health)
                            sprite[pPlayer->i].extra = pPlayer->max_player_health;

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

int A_CheckHitSprite(int spriteNum, int16_t *hitSprite)
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

static int P_FindWall(DukePlayer_t *pPlayer, int *hitWall)
{
    hitdata_t hitData;

    hitscan((const vec3_t *)pPlayer, pPlayer->cursectnum, sintable[(fix16_to_int(pPlayer->q16ang) + 512) & 2047],
            sintable[fix16_to_int(pPlayer->q16ang) & 2047], 0, &hitData, CLIPMASK0);

    *hitWall = hitData.wall;

    if (hitData.wall < 0)
        return INT32_MAX;

    return FindDistance2D(hitData.pos.x - pPlayer->pos.x, hitData.pos.y - pPlayer->pos.y);
}

// returns 1 if sprite i should not be considered by neartag
static int32_t our_neartag_blacklist(int32_t UNUSED(spriteNum))
{
    return 0;
}

static void G_ClearCameras(DukePlayer_t *p)
{
    G_ClearCameraView(p);
}

void P_CheckSectors(int playerNum)
{
    DukePlayer_t *const pPlayer = g_player[playerNum].ps;

    if (pPlayer->cursectnum > -1)
    {
        sectortype *const pSector = &sector[pPlayer->cursectnum];
        switch ((uint16_t)pSector->lotag)
        {
            case 32767:
                pSector->lotag = 0;
                if (RR && !RRRA)
                    g_canSeePlayer = 0;
                P_DoQuote(QUOTE_FOUND_SECRET, pPlayer);
				SECRET_Trigger(pPlayer->cursectnum);
				pPlayer->secret_rooms++;
                return;

            case UINT16_MAX:
                pSector->lotag = 0;
                for (bssize_t TRAVERSE_CONNECT(playerNum))
                    g_player[playerNum].ps->gm = MODE_EOL;

                if (!RRRA || !g_RAendLevel)
                {
                    if (ud.from_bonus)
                    {
                        ud.level_number = ud.from_bonus;
                        m_level_number = ud.level_number;
                        ud.from_bonus = 0;
                    }
                    else
                    {

                        if (RRRA && ud.level_number == 6 && ud.volume_number == 0)
                            g_RAendEpisode = 1;
                        ud.level_number = (++ud.level_number < MAXLEVELS) ? ud.level_number : 0;
                        m_level_number = ud.level_number;
                    }
                    g_RAendLevel = 1;
                }
                return;

            case UINT16_MAX-1:
                pSector->lotag           = 0;
                pPlayer->timebeforeexit  = GAMETICSPERSEC * 8;
                pPlayer->customexitsound = pSector->hitag;
                return;

            default:
                if (pSector->lotag >= 10000 && (RR || pSector->lotag < 16383))
                {
                    if (playerNum == screenpeek || (g_gametypeFlags[ud.coop] & GAMETYPE_COOPSOUND))
                    {
                        if (RR && !RRRA)
                            g_canSeePlayer = -1;
                        A_PlaySound(pSector->lotag - 10000, pPlayer->i);
                    }
                    pSector->lotag = 0;
                }
                break;
        }
    }

    //After this point the the player effects the map with space

    if (pPlayer->gm &MODE_TYPE || sprite[pPlayer->i].extra <= 0)
        return;

    if (ud.cashman && TEST_SYNC_KEY(g_player[playerNum].input->bits, SK_OPEN))
    {
        if (RR && !RRRA)
            g_canSeePlayer = -1;
        A_SpawnMultiple(pPlayer->i, TILE_MONEY, 2);
    }

    if (!RR && pPlayer->newowner >= 0)
    {
        if (klabs(g_player[playerNum].input->svel) > 768 || klabs(g_player[playerNum].input->fvel) > 768)
        {
            G_ClearCameras(pPlayer);
            return;
        }
    }

    if (!TEST_SYNC_KEY(g_player[playerNum].input->bits, SK_OPEN) && !TEST_SYNC_KEY(g_player[playerNum].input->bits, SK_ESCAPE))
        pPlayer->toggle_key_flag = 0;
    else if (!pPlayer->toggle_key_flag)
    {
        int foundWall;

        int16_t nearSector, nearWall, nearSprite;
        int32_t nearDist;

        if (!RR && TEST_SYNC_KEY(g_player[playerNum].input->bits, SK_ESCAPE))
        {
            if (pPlayer->newowner >= 0)
                G_ClearCameras(pPlayer);
            return;
        }

        nearSprite = -1;
        pPlayer->toggle_key_flag = 1;
        foundWall = -1;

        if (RR && !RRRA)
        {
            hitdata_t hitData;
            hitscan((const vec3_t *)pPlayer, pPlayer->cursectnum, sintable[(fix16_to_int(pPlayer->q16ang) + 512) & 2047],
                    sintable[fix16_to_int(pPlayer->q16ang) & 2047], 0, &hitData, CLIPMASK0);
            g_canSeePlayer &= ~0xffff;
            g_canSeePlayer |= hitData.sect;
        }

        int wallDist = P_FindWall(pPlayer, &foundWall);

        if (RRRA)
        {
            if (foundWall >= 0 && wall[foundWall].overpicnum == TILE_MIRROR && playerNum == screenpeek)
                if (!g_netServer && numplayers == 1)
                {
                    if (A_CheckSoundPlaying(pPlayer->i,27) == 0 && A_CheckSoundPlaying(pPlayer->i,28) == 0 && A_CheckSoundPlaying(pPlayer->i,29) == 0
                        && A_CheckSoundPlaying(pPlayer->i,257) == 0 && A_CheckSoundPlaying(pPlayer->i,258) == 0)
                    {
                        int snd = krand2() % 5;
                        if (snd == 0)
                            A_PlaySound(27, pPlayer->i);
                        else if (snd == 1)
                            A_PlaySound(28, pPlayer->i);
                        else if (snd == 2)
                            A_PlaySound(29, pPlayer->i);
                        else if (snd == 3)
                            A_PlaySound(257, pPlayer->i);
                        else if (snd == 4)
                            A_PlaySound(258, pPlayer->i);
                    }
                    return;
                }
        }
        else if (foundWall >= 0 && (RR || wallDist < 1280) && wall[foundWall].overpicnum == TILE_MIRROR)
            if (wall[foundWall].lotag > 0 && !A_CheckSoundPlaying(pPlayer->i,wall[foundWall].lotag) && playerNum == screenpeek)
            {
                if (RR)
                    g_canSeePlayer = -1;
                A_PlaySound(wall[foundWall].lotag,pPlayer->i);
                return;
            }

        if (foundWall >= 0 && (wall[foundWall].cstat&16))
        {
            if (RRRA)
                g_canSeePlayer = foundWall*32;
            if (wall[foundWall].lotag)
                return;
        }

        int const intang = fix16_to_int(pPlayer->oq16ang);

        if (RRRA)
        {
            if (pPlayer->OnMotorcycle)
            {
                if (pPlayer->moto_speed < 20)
                    G_OffMotorcycle(pPlayer);
                return;
            }
            if (pPlayer->OnBoat)
            {
                if (pPlayer->moto_speed < 20)
                    G_OffBoat(pPlayer);
                return;
            }
            neartag(pPlayer->opos.x, pPlayer->opos.y, pPlayer->opos.z, sprite[pPlayer->i].sectnum, intang, &nearSector,
                &nearWall, &nearSprite, &nearDist, 1280, 1, our_neartag_blacklist);
        }

        if (RR && !RRRA)
            g_canSeePlayer = -1;

        if (pPlayer->newowner >= 0)
            neartag(pPlayer->opos.x, pPlayer->opos.y, pPlayer->opos.z, sprite[pPlayer->i].sectnum, intang, &nearSector,
                &nearWall, &nearSprite, &nearDist, 1280, 1, our_neartag_blacklist);
        else
        {
            neartag(pPlayer->pos.x, pPlayer->pos.y, pPlayer->pos.z, sprite[pPlayer->i].sectnum, intang, &nearSector,
                &nearWall, &nearSprite, &nearDist, 1280, 1, our_neartag_blacklist);
            if (nearSprite == -1 && nearWall == -1 && nearSector == -1)
                neartag(pPlayer->pos.x, pPlayer->pos.y, pPlayer->pos.z+ZOFFSET3, sprite[pPlayer->i].sectnum, intang, &nearSector,
                    &nearWall, &nearSprite, &nearDist, 1280, 1, our_neartag_blacklist);
            if (nearSprite == -1 && nearWall == -1 && nearSector == -1)
                neartag(pPlayer->pos.x, pPlayer->pos.y, pPlayer->pos.z+ZOFFSET2, sprite[pPlayer->i].sectnum, intang, &nearSector,
                    &nearWall, &nearSprite, &nearDist, 1280, 1, our_neartag_blacklist);
            if (nearSprite == -1 && nearWall == -1 && nearSector == -1)
            {
                neartag(pPlayer->pos.x, pPlayer->pos.y, pPlayer->pos.z+ZOFFSET2, sprite[pPlayer->i].sectnum, intang, &nearSector,
                    &nearWall, &nearSprite, &nearDist, 1280, 3, our_neartag_blacklist);
                if (nearSprite >= 0)
                {
                    switch (DYNAMICTILEMAP(sprite[nearSprite].picnum))
                    {
                        case PODFEM1__STATIC:
                        case FEM1__STATIC:
                        case FEM2__STATIC:
                        case FEM3__STATIC:
                        case FEM4__STATIC:
                        case FEM5__STATIC:
                        case FEM6__STATIC:
                        case FEM7__STATIC:
                        case FEM8__STATIC:
                        case FEM9__STATIC:
                            if (RR) break;
                            fallthrough__;
                        case FEM10__STATIC:
                        case NAKED1__STATIC:
                        case STATUE__STATIC:
                        case TOUGHGAL__STATIC: return;
                        case COW__STATICRR:
                            g_spriteExtra[nearSprite] = 1;
                            return;
                    }
                }

                nearSprite = -1;
                nearWall   = -1;
                nearSector = -1;
            }
        }

        if (pPlayer->newowner == -1 && nearSprite == -1 && nearSector == -1 && nearWall == -1)
        {
            if (isanunderoperator(sector[sprite[pPlayer->i].sectnum].lotag))
                nearSector = sprite[pPlayer->i].sectnum;
        }

        if (nearSector >= 0 && (sector[nearSector].lotag&16384))
            return;

        if (nearSprite == -1 && nearWall == -1)
        {
            if (pPlayer->cursectnum >= 0 && sector[pPlayer->cursectnum].lotag == 2)
            {
                if (A_CheckHitSprite(pPlayer->i, &nearSprite) > 1280)
                    nearSprite = -1;
            }
        }

        if (nearSprite >= 0)
        {
            if (RR && !RRRA)
                g_canSeePlayer = playerNum;
            if (checkhitswitch(playerNum, nearSprite, 1))
                return;

            switch (DYNAMICTILEMAP(sprite[nearSprite].picnum))
            {
            case RRTILE8448__STATICRR:
                if (!RRRA) break;
                if (!A_CheckSoundPlaying(nearSprite, 340))
                    A_PlaySound(340, nearSprite);
                return;
            case RRTILE8704__STATICRR:
                if (!RRRA) break;
                if (!g_netServer && numplayers == 1)
                {
                    static int soundPlayed = 0;
                    if (S_CheckSoundPlaying(nearSprite, 445) == 0 && soundPlayed == 0)
                    {
                        A_PlaySound(445, nearSprite);
                        soundPlayed = 1;
                    }
                    else if (S_CheckSoundPlaying(nearSprite, 445) == 0 && S_CheckSoundPlaying(nearSprite, 446) == 0
                        && S_CheckSoundPlaying(nearSprite, 447) == 0 && soundPlayed == 0)
                    {
                        if ((krand2()%2) == 1)
                            A_PlaySound(446, nearSprite);
                        else
                            A_PlaySound(447, nearSprite);
                    }
                }
                return;
            case EMPTYBIKE__STATICRR:
                if (!RRRA) break;
                G_OnMotorcycle(pPlayer, nearSprite);
                return;
            case EMPTYBOAT__STATICRR:
                if (!RRRA) break;
                G_OnBoat(pPlayer, nearSprite);
                return;
            case RRTILE8164__STATICRR:
            case RRTILE8165__STATICRR:
            case RRTILE8166__STATICRR:
            case RRTILE8167__STATICRR:
            case RRTILE8168__STATICRR:
            case RRTILE8591__STATICRR:
            case RRTILE8592__STATICRR:
            case RRTILE8593__STATICRR:
            case RRTILE8594__STATICRR:
            case RRTILE8595__STATICRR:
                if (!RRRA) break;
                sprite[nearSprite].extra = 60;
                A_PlaySound(235, nearSprite);
                return;
            case TOILET__STATIC:
            case STALL__STATIC:
            case RRTILE2121__STATICRR:
            case RRTILE2122__STATICRR:
                if (pPlayer->last_pissed_time == 0)
                {
                    if (adult_lockout == 0)
                        A_PlaySound(RR ? 435 : DUKE_URINATE, pPlayer->i);

                    pPlayer->last_pissed_time = GAMETICSPERSEC * 220;
                    pPlayer->transporter_hold = 29 * 2;

                    if (pPlayer->holster_weapon == 0)
                    {
                        pPlayer->holster_weapon = 1;
                        pPlayer->weapon_pos     = -1;
                    }

                    if (sprite[pPlayer->i].extra <= (pPlayer->max_player_health - (pPlayer->max_player_health / 10)))
                    {
                        sprite[pPlayer->i].extra += pPlayer->max_player_health / 10;
                        pPlayer->last_extra = sprite[pPlayer->i].extra;
                    }
                    else if (sprite[pPlayer->i].extra < pPlayer->max_player_health)
                        sprite[pPlayer->i].extra = pPlayer->max_player_health;
                }
                else if (!A_CheckSoundPlaying(nearSprite,RR ? DUKE_GRUNT : FLUSH_TOILET))
                {
                    if (RR && !RRRA)
                        g_canSeePlayer = -1;

                    A_PlaySound(RR ? DUKE_GRUNT : FLUSH_TOILET,nearSprite);
                }
                return;

            case NUKEBUTTON__STATIC:
            {
                if (RR) break;
                int wallNum;

                P_FindWall(pPlayer, &wallNum);

                if (wallNum >= 0 && wall[wallNum].overpicnum == 0)
                {
                    if (actor[nearSprite].t_data[0] == 0)
                    {
                        if (ud.noexits && (g_netServer || ud.multimode > 1))
                        {
                            // TILE_NUKEBUTTON frags the player
                            actor[pPlayer->i].picnum = TILE_NUKEBUTTON;
                            actor[pPlayer->i].extra  = 250;
                        }
                        else
                        {
                            actor[nearSprite].t_data[0] = 1;
                            sprite[nearSprite].owner    = pPlayer->i;
                            // assignment of buttonpalette here is not a bug
                            ud.secretlevel =
                            (pPlayer->buttonpalette = sprite[nearSprite].pal) ? sprite[nearSprite].lotag : 0;
                        }
                    }
                }
                return;
            }

            case WATERFOUNTAIN__STATIC:
                if (actor[nearSprite].t_data[0] != 1)
                {
                    actor[nearSprite].t_data[0] = 1;
                    sprite[nearSprite].owner    = pPlayer->i;

                    if (sprite[pPlayer->i].extra < pPlayer->max_player_health)
                    {
                        sprite[pPlayer->i].extra++;
                        if (RR && !RRRA)
                            g_canSeePlayer = -1;
                        A_PlaySound(DUKE_DRINKING,pPlayer->i);
                    }
                }
                return;

            case PLUG__STATIC:
                if (RR && !RRRA)
                    g_canSeePlayer = -1;
                A_PlaySound(SHORT_CIRCUIT, pPlayer->i);
                sprite[pPlayer->i].extra -= 2+(krand2()&3);

                P_PalFrom(pPlayer, 32, 48,48,64);
                break;

            case VIEWSCREEN__STATIC:
            case VIEWSCREEN2__STATIC:
                if (RR) break;
                // Try to find a camera sprite for the viewscreen.
                for (bssize_t SPRITES_OF(STAT_ACTOR, spriteNum))
                {
                    if (PN(spriteNum) == TILE_CAMERA1 && SP(spriteNum) == 0 && sprite[nearSprite].hitag == SLT(spriteNum))
                    {
                        sprite[spriteNum].yvel   = 1;  // Using this camera
                        A_PlaySound(MONITOR_ACTIVE, pPlayer->i);
                        sprite[nearSprite].owner = spriteNum;
                        sprite[nearSprite].yvel  = 1;  // VIEWSCREEN_YVEL
                        g_curViewscreen          = nearSprite;

                        int const playerSectnum = pPlayer->cursectnum;
                        pPlayer->cursectnum     = SECT(spriteNum);
                        P_UpdateScreenPal(pPlayer);
                        pPlayer->cursectnum     = playerSectnum;
                        pPlayer->newowner       = spriteNum;

                        //P_UpdatePosWhenViewingCam(pPlayer);

                        return;
                    }
                }

                G_ClearCameras(pPlayer);
                return;
            }  // switch
        }

        if (TEST_SYNC_KEY(g_player[playerNum].input->bits, SK_OPEN) == 0)
            return;

        if (!RR && pPlayer->newowner >= 0)
        {
            G_ClearCameras(pPlayer);
            return;
        }
        if (RR && !RRRA && nearWall == -1 && nearSector == -1 && nearSprite == -1)
            g_canSeePlayer = playerNum;

        if (nearWall == -1 && nearSector == -1 && nearSprite == -1)
        {
            if (klabs(A_GetHitscanRange(pPlayer->i)) < 512)
            {
                if (RR && !RRRA)
                    g_canSeePlayer = -1;
                A_PlaySound(((krand2()&255) < 16) ? DUKE_SEARCH2 : DUKE_SEARCH, pPlayer->i);
                return;
            }
        }

        if (nearWall >= 0)
        {
            if (wall[nearWall].lotag > 0 && isadoorwall(wall[nearWall].picnum))
            {
                if (foundWall == nearWall || foundWall == -1)
                {
                    if (RR && !RRRA)
                        g_canSeePlayer = playerNum;
                    checkhitswitch(playerNum,nearWall,0);
                }
                return;
            }
            else if (!RR && pPlayer->newowner >= 0)
            {
                G_ClearCameras(pPlayer);
                return;
            }
        }

        if (nearSector >= 0 && (sector[nearSector].lotag&16384) == 0 &&
                isanearoperator(sector[nearSector].lotag))
        {
            for (bssize_t SPRITES_OF_SECT(nearSector, spriteNum))
            {
                if (PN(spriteNum) == TILE_ACTIVATOR || PN(spriteNum) == TILE_MASTERSWITCH)
                    return;
            }

            if (!RR || P_HasKey(nearSector, playerNum))
            {
                if (RR && !RRRA)
                    g_canSeePlayer = -1;
                operatesectors(nearSector,pPlayer->i);
            }
            else if (RR)
            {
                if (g_sectorExtra[nearSector] > 3)
                    A_PlaySound(99,pPlayer->i);
                else
                    A_PlaySound(419,pPlayer->i);
                if (RR && !RRRA)
                    g_canSeePlayer = -1;
                P_DoQuote(41,pPlayer);
            }
        }
        else if ((sector[sprite[pPlayer->i].sectnum].lotag&16384) == 0)
        {
            if (isanunderoperator(sector[sprite[pPlayer->i].sectnum].lotag))
            {
                for (bssize_t SPRITES_OF_SECT(sprite[pPlayer->i].sectnum, spriteNum))
                {
                    if (PN(spriteNum) == TILE_ACTIVATOR || PN(spriteNum) == TILE_MASTERSWITCH)
                        return;
                }
                
                if (!RR || P_HasKey(sprite[pPlayer->i].sectnum, playerNum))
                {
                    if (RR && !RRRA)
                        g_canSeePlayer = -1;
                    operatesectors(sprite[pPlayer->i].sectnum,pPlayer->i);
                }
                else if (RR)
                {
                    if (g_sectorExtra[sprite[pPlayer->i].sectnum] > 3)
                        A_PlaySound(99,pPlayer->i);
                    else
                        A_PlaySound(419,pPlayer->i);
                    if (RR && !RRRA)
                        g_canSeePlayer = -1;
                    P_DoQuote(41,pPlayer);
                }
            }
            else checkhitswitch(playerNum,nearWall,0);
        }
    }
}

void G_DoFurniture(int wallNum, int sectNum, int playerNum)
{
    int startwall, endwall;
    int insideCheck, i;
    int32_t max_x, min_x, max_y, min_y, speed;
    startwall = sector[wall[wallNum].nextsector].wallptr;
    endwall = startwall+sector[wall[wallNum].nextsector].wallnum;

    insideCheck = 1;
    max_x = max_y = -(2<<16);
    min_x = min_y = 2<<16;
    speed = sector[sectNum].hitag;
    if (speed > 16)
        speed = 16;
    else if (speed == 0)
        speed = 4;
    for (i = startwall; i < endwall; i++)
    {
        if (max_x < wall[i].x)
            max_x = wall[i].x;
        if (max_y < wall[i].y)
            max_y = wall[i].y;
        if (min_x > wall[i].x)
            min_x = wall[i].x;
        if (min_y > wall[i].y)
            min_y = wall[i].y;
    }
    max_x += speed+1;
    max_y += speed+1;
    min_x -= speed+1;
    min_y -= speed+1;
    if (!inside(max_x, max_y, sectNum))
        insideCheck = 0;
    if (!inside(max_x, min_y, sectNum))
        insideCheck = 0;
    if (!inside(min_x, min_y, sectNum))
        insideCheck = 0;
    if (!inside(min_x, max_y, sectNum))
        insideCheck = 0;
    if (insideCheck)
    {
        if (!S_CheckSoundPlaying(g_player[playerNum].ps->i, 389))
            A_PlaySound(389, g_player[playerNum].ps->i);
        for (i = startwall; i < endwall; i++)
        {
            int32_t x, y;
            x = wall[i].x;
            y = wall[i].y;
            switch (wall[wallNum].lotag)
            {
                case 42:
                    dragpoint(i,x,y+speed,0);
                    break;
                case 41:
                    dragpoint(i,x-speed,y,0);
                    break;
                case 40:
                    dragpoint(i,x,y-speed,0);
                    break;
                case 43:
                    dragpoint(i,x+speed,y,0);
                    break;
            }
        }
    }
    else
    {
        speed -= 2;
        for (i = startwall; i < endwall; i++)
        {
            int32_t x, y;
            x = wall[i].x;
            y = wall[i].y;
            switch (wall[wallNum].lotag)
            {
                case 42:
                    dragpoint(i,x,y-speed,0);
                    break;
                case 41:
                    dragpoint(i,x+speed,y,0);
                    break;
                case 40:
                    dragpoint(i,x,y+speed,0);
                    break;
                case 43:
                    dragpoint(i,x-speed,y,0);
                    break;
            }
        }
    }
}


END_DUKE_NS
